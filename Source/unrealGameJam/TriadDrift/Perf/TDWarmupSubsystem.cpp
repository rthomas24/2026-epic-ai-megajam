// Copyright (c) 2026 Triad Drift team. Epic AI MegaJam entry.
#include "TriadDrift/Perf/TDWarmupSubsystem.h"

#include "TriadDrift/Combat/TDCombatAssetList.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/AssetManager.h"
#include "Engine/DataAsset.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/UnrealType.h"

int32 UTDWarmupSubsystem::EffectsPerFrame = 3;

namespace
{
	int32 GTDWarmupEnabled = 1;
	FAutoConsoleVariableRef CVarTDWarmup(TEXT("TD.Warmup"), GTDWarmupEnabled,
		TEXT("1 = at map start, stream every presentation / fallback asset and what it points at in the background and activate each effect once off-screen (first-use hitches, #89). 0 = off (before/after measurements)."));
	FAutoConsoleVariableRef CVarTDWarmupPerFrame(TEXT("TD.Warmup.PerFrame"), UTDWarmupSubsystem::EffectsPerFrame,
		TEXT("Effects the warm pass activates per frame."));

	// Far below the playfield, out of every camera's view: the components are created (render proxies, PSO precache
	// requests, pooled instances) but nothing is seen.
	const FVector WarmLocation(0.f, 0.f, -2000000.f);
	constexpr float WarmSeconds = 0.3f;
	// The map's own BeginPlay / first-frame loads (squad, intro, HUD) happen before the stream starts.
	constexpr float StartDelaySeconds = 1.0f;

	bool IsSkippedSlot(const FProperty* Prop)
	{
		const FString Name = Prop->GetName();
		return Name.Contains(TEXT("Music")) || Name.Contains(TEXT("Ambience")) || Name.Contains(TEXT("Score"));
	}

	void CollectFromValue(const FProperty* Prop, const void* Value, TArray<FSoftObjectPath>& Out, int32 Depth);

	void CollectFromStruct(const UStruct* Struct, const void* Data, TArray<FSoftObjectPath>& Out, int32 Depth)
	{
		if (Depth > 8)
		{
			return;
		}
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			if (IsSkippedSlot(*It))
			{
				continue;
			}
			for (int32 i = 0; i < It->ArrayDim; ++i)
			{
				CollectFromValue(*It, It->ContainerPtrToValuePtr<void>(Data, i), Out, Depth + 1);
			}
		}
	}

	void CollectFromValue(const FProperty* Prop, const void* Value, TArray<FSoftObjectPath>& Out, int32 Depth)
	{
		if (const FSoftObjectProperty* Soft = CastField<FSoftObjectProperty>(Prop))
		{
			const FSoftObjectPath Path = Soft->GetPropertyValue(Value).ToSoftObjectPath();
			// Never a map: a data asset may name a level to travel to.
			if (Path.IsValid() && !Path.GetLongPackageName().Contains(TEXT("/Maps/")))
			{
				Out.AddUnique(Path);
			}
		}
		else if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		{
			CollectFromStruct(StructProp->Struct, Value, Out, Depth);
		}
		else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
		{
			FScriptArrayHelper Helper(ArrayProp, Value);
			for (int32 i = 0; i < Helper.Num(); ++i)
			{
				CollectFromValue(ArrayProp->Inner, Helper.GetRawPtr(i), Out, Depth + 1);
			}
		}
		else if (const FSetProperty* SetProp = CastField<FSetProperty>(Prop))
		{
			FScriptSetHelper Helper(SetProp, Value);
			for (FScriptSetHelper::FIterator It(Helper); It; ++It)
			{
				CollectFromValue(SetProp->ElementProp, Helper.GetElementPtr(It), Out, Depth + 1);
			}
		}
		else if (const FMapProperty* MapProp = CastField<FMapProperty>(Prop))
		{
			FScriptMapHelper Helper(MapProp, Value);
			for (FScriptMapHelper::FIterator It(Helper); It; ++It)
			{
				CollectFromValue(MapProp->KeyProp, Helper.GetKeyPtr(It), Out, Depth + 1);
				CollectFromValue(MapProp->ValueProp, Helper.GetValuePtr(It), Out, Depth + 1);
			}
		}
	}
}

namespace
{
	// T2's combat list (#89): the combat classes and the assets combat code loads by path, per map (Galaxy: space
	// combat and the Dominion; elsewhere: on foot). Registered here so the Combat module needs no warm-up code.
	struct FTDRegisterCombatWarmup
	{
		FTDRegisterCombatWarmup()
		{
			UTDWarmupSubsystem::RegisterProvider(TEXT("Combat"),
				[](const UWorld& W, TArray<FSoftObjectPath>& Out) { FTDCombatAssetList::Collect(W, Out); });
		}
	} GTDRegisterCombatWarmup;
}

TMap<FName, UTDWarmupSubsystem::FProvider>& UTDWarmupSubsystem::Providers()
{
	static TMap<FName, FProvider> Map;
	return Map;
}

void UTDWarmupSubsystem::RegisterProvider(FName Name, FProvider Provider)
{
	Providers().Add(Name, MoveTemp(Provider));
}

void UTDWarmupSubsystem::UnregisterProvider(FName Name)
{
	Providers().Remove(Name);
}

UTDWarmupSubsystem* UTDWarmupSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDWarmupSubsystem>() : nullptr;
}

bool UTDWarmupSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

TStatId UTDWarmupSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDWarmupSubsystem, STATGROUP_Tickables);
}

TArray<FName> UTDWarmupSubsystem::GetWarmFolders()
{
	return { FName(*UTDStatics::GetPresentationFolder()), FName(TEXT("/Game/TriadDrift/Gameplay/Characters")) };
}

bool UTDWarmupSubsystem::IsWantedForMap(const FString& PackagePath, const FString& MapName)
{
	// Per map (#89: warming everything everywhere cost about 650 MB of RAM). The galaxy has no one on foot; the planets
	// and the Dominion interior have no fleet and no Dominion hull. Shared folders (FX, barks, squad, presentation) are
	// warmed everywhere. A folder left out is only loaded on first use, as before the warm-up.
	static const TCHAR* OnFoot[] = { TEXT("/Crew"), TEXT("/Pilot"), TEXT("/NullGrunt"), TEXT("/NullHeavy"), TEXT("/NullTrooper"),
		TEXT("/NullDrone"), TEXT("/Warden") };
	static const TCHAR* SpaceOnly[] = { TEXT("/NullFleet"), TEXT("/Dominion") };
	const bool bGalaxy = MapName.Contains(TEXT("Galaxy"));
	const FString Characters = TEXT("/Game/TriadDrift/Gameplay/Characters");
	if (!PackagePath.StartsWith(Characters))
	{
		return true;
	}
	const FString Rest = PackagePath.Mid(Characters.Len());
	for (const TCHAR* F : (bGalaxy ? MakeArrayView(OnFoot) : MakeArrayView(SpaceOnly)))
	{
		if (Rest.StartsWith(F))
		{
			return false;
		}
	}
	return true;
}

bool UTDWarmupSubsystem::IsWantedOnThisMap(const FString& PackagePath) const
{
	const UWorld* World = GetWorld();
	return IsWantedForMap(PackagePath, World ? World->GetMapName() : FString());
}

void UTDWarmupSubsystem::CollectSoftPaths(const UObject* Object, TArray<FSoftObjectPath>& Out)
{
	if (Object)
	{
		CollectFromStruct(Object->GetClass(), Object, Out, 0);
	}
}

void UTDWarmupSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Automation worlds (headless tests) never warm: they load what each test needs.
	if (GTDWarmupEnabled == 0 || GIsAutomationTesting)
	{
		return;
	}
	Stage = ETDWarmupStage::Waiting;
	WaitLeft = StartDelaySeconds;
}

void UTDWarmupKeep::Keep(const TArray<TObjectPtr<UObject>>& Assets)
{
	for (UObject* Asset : Assets)
	{
		if (Asset) { Kept.Add(Asset); }
	}
}

void UTDWarmupSubsystem::HurryUnderCover()
{
	if (Stage == ETDWarmupStage::Waiting)
	{
		WaitLeft = 0.f;
		StartDataLoad();
	}
	if (Stage == ETDWarmupStage::LoadingData || Stage == ETDWarmupStage::LoadingContent)
	{
		FlushAsyncLoading();	// the stream's own callbacks move the stage on
	}
}

void UTDWarmupSubsystem::StartDataLoad()
{
	IAssetRegistry* Registry = IAssetRegistry::Get();
	if (!Registry)
	{
		return;
	}
	StartSeconds = FPlatformTime::Seconds();
	for (const FName& Folder : GetWarmFolders())
	{
		TArray<FAssetData> Found;
		Registry->GetAssetsByPath(Folder, Found, /*bRecursive*/ true);
		for (const FAssetData& Data : Found)
		{
			// Data assets, plus every animation: the pilot's and the crew's moves are looked up by path string the first
			// time they play (first swing, first death), which was a synchronous load in the middle of a fight.
			if (!IsWantedOnThisMap(Data.PackagePath.ToString()))
			{
				continue;
			}
			const FName Class = Data.AssetClassPath.GetAssetName();
			if (Data.AssetName.ToString().StartsWith(TEXT("DA_")) || Class == TEXT("AnimSequence") || Class == TEXT("AnimMontage")
				|| Class == TEXT("BlendSpace") || Class == TEXT("BlendSpace1D"))
			{
				DataPaths.AddUnique(Data.GetSoftObjectPath());
			}
		}
	}
	if (DataPaths.Num() == 0)
	{
		Stage = ETDWarmupStage::Done;
		return;
	}
	Stage = ETDWarmupStage::LoadingData;
	DataHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(DataPaths,
		FStreamableDelegate::CreateUObject(this, &UTDWarmupSubsystem::OnDataLoaded), FStreamableManager::DefaultAsyncLoadPriority);
	if (!DataHandle.IsValid())
	{
		OnDataLoaded(); // everything was already in memory
	}
}

void UTDWarmupSubsystem::OnDataLoaded()
{
	if (Stage != ETDWarmupStage::LoadingData)
	{
		return;
	}
	for (const FSoftObjectPath& Path : DataPaths)
	{
		if (UObject* Asset = Path.ResolveObject())
		{
			Held.AddUnique(Asset);
			CollectSoftPaths(Asset, ContentPaths);
		}
	}
	Stage = ETDWarmupStage::LoadingContent;
	UE_LOG(LogTriadDrift, Log, TEXT("TD warmup: %d data assets in %.2f s; streaming %d referenced assets"),
		Held.Num(), FPlatformTime::Seconds() - StartSeconds, ContentPaths.Num());
	if (const UWorld* World = GetWorld())
	{
		for (const TPair<FName, FProvider>& P : Providers())
		{
			const int32 Before = ContentPaths.Num();
			TArray<FSoftObjectPath> Extra;
			P.Value(*World, Extra);
			for (const FSoftObjectPath& Path : Extra)
			{
				if (Path.IsValid()) { ContentPaths.AddUnique(Path); }
			}
			UE_LOG(LogTriadDrift, Log, TEXT("TD warmup: provider %s added %d assets"), *P.Key.ToString(), ContentPaths.Num() - Before);
		}
	}
	if (ContentPaths.Num() == 0)
	{
		OnContentLoaded();
		return;
	}
	ContentHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ContentPaths,
		FStreamableDelegate::CreateUObject(this, &UTDWarmupSubsystem::OnContentLoaded), FStreamableManager::DefaultAsyncLoadPriority);
	if (!ContentHandle.IsValid())
	{
		OnContentLoaded();
	}
}

void UTDWarmupSubsystem::OnContentLoaded()
{
	if (Stage != ETDWarmupStage::LoadingContent)
	{
		return;
	}
	int32 Loaded = 0;
	for (const FSoftObjectPath& Path : ContentPaths)
	{
		if (UObject* Asset = Path.ResolveObject())
		{
			Held.AddUnique(Asset);
			++Loaded;
			if (UNiagaraSystem* System = Cast<UNiagaraSystem>(Asset))
			{
				ToActivate.AddUnique(System);
			}
		}
	}
	UE_LOG(LogTriadDrift, Log, TEXT("TD warmup: %d of %d referenced assets loaded at %.2f s; warming %d effects"),
		Loaded, ContentPaths.Num(), FPlatformTime::Seconds() - StartSeconds, ToActivate.Num());
	Stage = ETDWarmupStage::Activating;
	// For the rest of the run (the next level's warm-up finds these already in memory).
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UTDWarmupKeep* KeepSub = GI->GetSubsystem<UTDWarmupKeep>())
			{
				KeepSub->Keep(Held);
				UE_LOG(LogTriadDrift, Log, TEXT("TD warmup: %d assets kept for the run"), KeepSub->GetKeptCount());
			}
		}
	}
}

void UTDWarmupSubsystem::Tick(float DeltaTime)
{
	if (Stage == ETDWarmupStage::Waiting)
	{
		WaitLeft -= DeltaTime;
		if (WaitLeft <= 0.f)
		{
			StartDataLoad();
		}
		return;
	}
	if (Stage != ETDWarmupStage::Activating)
	{
		return;
	}
	UWorld* World = GetWorld();
	for (int32 i = Active.Num() - 1; i >= 0; --i)
	{
		ActiveAge[i] += DeltaTime;
		if (!IsValid(Active[i]) || ActiveAge[i] >= WarmSeconds)
		{
			if (IsValid(Active[i]))
			{
				Active[i]->Deactivate(); // completes and goes back to the pool (AutoRelease), warm for its first real use
			}
			Active.RemoveAtSwap(i);
			ActiveAge.RemoveAtSwap(i);
		}
	}
	for (int32 n = 0; n < FMath::Max(1, EffectsPerFrame) && ToActivate.Num() > 0 && World; ++n)
	{
		UNiagaraSystem* System = ToActivate.Pop(EAllowShrinking::No);
		if (UNiagaraComponent* C = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, WarmLocation, FRotator::ZeroRotator,
			FVector(1.f), /*bAutoDestroy*/ true, /*bAutoActivate*/ true, ENCPoolMethod::AutoRelease, /*bPreCullCheck*/ false))
		{
			Active.Add(C);
			ActiveAge.Add(0.f);
			++WarmedEffects;
		}
	}
	if (ToActivate.Num() == 0 && Active.Num() == 0)
	{
		Stage = ETDWarmupStage::Done;
		UE_LOG(LogTriadDrift, Log, TEXT("TD warmup: done at %.2f s (%d assets held, %d effects warmed)"),
			FPlatformTime::Seconds() - StartSeconds, Held.Num(), WarmedEffects);
	}
}

void UTDWarmupSubsystem::FinishActivations(bool bAll)
{
	for (UNiagaraComponent* C : Active)
	{
		if (IsValid(C))
		{
			C->DeactivateImmediate();
		}
	}
	Active.Reset();
	ActiveAge.Reset();
	if (bAll)
	{
		ToActivate.Reset();
	}
}

void UTDWarmupSubsystem::Deinitialize()
{
	FinishActivations(true);
	if (DataHandle.IsValid()) { DataHandle->CancelHandle(); }
	if (ContentHandle.IsValid()) { ContentHandle->CancelHandle(); }
	Super::Deinitialize();
}
