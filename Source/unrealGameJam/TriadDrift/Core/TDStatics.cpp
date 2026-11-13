#include "TriadDrift/Core/TDStatics.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Camera/CameraShakeBase.h"
#include "Components/AudioComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDDamageTypes.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/GCObject.h"

// ---------------------------------------------------------------------------------------------- teams

ETDTeam UTDStatics::GetTeam(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return ETDTeam::Neutral;
	}
	if (const ITDTeam* TeamActor = Cast<ITDTeam>(Actor))
	{
		return TeamActor->GetTeam();
	}
	if (const UTDHealthComponent* Health = Actor->FindComponentByClass<UTDHealthComponent>())
	{
		return Health->DefaultTeam;
	}
	return ETDTeam::Neutral;
}

bool UTDStatics::AreTeamsHostile(ETDTeam A, ETDTeam B)
{
	return (A == ETDTeam::Player && B == ETDTeam::Enemy) || (A == ETDTeam::Enemy && B == ETDTeam::Player);
}

bool UTDStatics::AreHostile(const AActor* A, const AActor* B)
{
	return AreTeamsHostile(GetTeam(A), GetTeam(B));
}

UTDHealthComponent* UTDStatics::GetHealth(const AActor* Actor)
{
	return IsValid(Actor) ? Actor->FindComponentByClass<UTDHealthComponent>() : nullptr;
}

// --------------------------------------------------------------------------------------------- damage

float UTDStatics::ApplyTDDamage(AActor* Target, float Amount, ETDDamageKind Kind, AActor* Instigator, AActor* Causer,
	FVector HitLocation, FVector HitDirection, bool bIgnoreShield)
{
	if (!IsValid(Target) || Amount <= 0.f)
	{
		return 0.f;
	}

	// Friendly fire is off. The causer's team wins (a deflected projectile hurts its old team).
	ETDTeam SourceTeam = GetTeam(Causer);
	if (SourceTeam == ETDTeam::Neutral)
	{
		SourceTeam = GetTeam(Instigator);
	}
	if (SourceTeam != ETDTeam::Neutral && SourceTeam == GetTeam(Target))
	{
		return 0.f;
	}

	if (UTDHealthComponent* Health = GetHealth(Target))
	{
		FTDDamageInfo Info;
		Info.Amount = Amount;
		Info.Kind = Kind;
		Info.Instigator = Instigator;
		Info.Causer = Causer;
		Info.HitLocation = HitLocation.IsZero() ? Target->GetActorLocation() : HitLocation;
		Info.HitDirection = HitDirection;
		Info.bIgnoreShield = bIgnoreShield;
		return Health->ApplyDamage(Info);
	}

	// No health component: fall back to the engine path (destructible props, template actors).
	return UGameplayStatics::ApplyDamage(Target, Amount, nullptr, Causer ? Causer : Instigator, UTDDamageType::ClassFor(Kind));
}

int32 UTDStatics::ApplyTDRadialDamage(const UObject* WorldContextObject, FVector Origin, float Radius, float Amount,
	ETDDamageKind Kind, ETDTeam SourceTeam, AActor* Instigator, AActor* Causer, bool bFalloff, AActor* IgnoreActor)
{
	const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(WorldContextObject);
	if (!Registry || Radius <= 0.f || Amount <= 0.f)
	{
		return 0;
	}

	// Copy: applying damage can kill actors and mutate the registry.
	TArray<AActor*, TInlineAllocator<32>> Victims;
	for (const TWeakObjectPtr<AActor>& Entry : Registry->GetAll())
	{
		AActor* Actor = Entry.Get();
		if (!IsValid(Actor) || Actor == IgnoreActor)
		{
			continue;
		}
		const ETDTeam Team = GetTeam(Actor);
		const bool bHostile = (SourceTeam == ETDTeam::Neutral) ? true : AreTeamsHostile(SourceTeam, Team);
		if (bHostile)
		{
			Victims.Add(Actor);
		}
	}

	int32 HitCount = 0;
	for (AActor* Victim : Victims)
	{
		FVector BoundsOrigin, BoundsExtent;
		Victim->GetActorBounds(false, BoundsOrigin, BoundsExtent);
		if (BoundsExtent.IsNearlyZero())
		{
			// No primitive components (logic-only actors, tests): measure to the actor itself.
			BoundsOrigin = Victim->GetActorLocation();
		}
		const float SurfaceDistance = FMath::Max(0.f, static_cast<float>(FVector::Dist(Origin, BoundsOrigin)) - static_cast<float>(BoundsExtent.GetMin()));
		if (SurfaceDistance > Radius)
		{
			continue;
		}
		const float Scale = bFalloff ? FMath::Lerp(1.f, 0.25f, SurfaceDistance / Radius) : 1.f;
		const FVector Direction = (BoundsOrigin - Origin).GetSafeNormal();

		UTDHealthComponent* Health = GetHealth(Victim);
		if (!Health || Health->IsDead())
		{
			continue;
		}
		FTDDamageInfo Info;
		Info.Amount = Amount * Scale;
		Info.Kind = Kind;
		Info.Instigator = Instigator;
		Info.Causer = Causer;
		Info.HitLocation = BoundsOrigin;
		Info.HitDirection = Direction;
		if (Health->ApplyDamage(Info) > 0.f)
		{
			++HitCount;
		}
	}
	return HitCount;
}

// ----------------------------------------------------------------------------------------------- flow

ITDGameFlow* UTDStatics::GetGameFlow(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
	{
		return nullptr;
	}
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return Cast<ITDGameFlow>(GameInstance);
}

// --------------------------------------------------------------------------------------- presentation

namespace
{
	void PlayCueShake(const UObject* WorldContextObject, const FTDFXCue& Cue, const FVector& Location)
	{
		if (Cue.Shake.IsNull() || !GEngine)
		{
			return;
		}
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
		UClass* ShakeClass = Cue.Shake.LoadSynchronous();
		if (!World || !ShakeClass)
		{
			return;
		}
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (!PC || !PC->IsLocalController())
			{
				continue;
			}
			if (Cue.ShakeRadius > 0.f)
			{
				FVector ViewLocation;
				FRotator ViewRotation;
				PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
				if (FVector::DistSquared(ViewLocation, Location) > FMath::Square(Cue.ShakeRadius))
				{
					continue;
				}
			}
			PC->ClientStartCameraShake(ShakeClass, Cue.ShakeScale);
		}
	}
}

UNiagaraComponent* UTDStatics::PlayCue(const UObject* WorldContextObject, const FTDFXCue& Cue, FVector Location, FRotator Rotation)
{
	if (!WorldContextObject || Cue.IsEmpty())
	{
		return nullptr;
	}

	UNiagaraComponent* Component = nullptr;
	if (!Cue.VFX.IsNull())
	{
		if (UNiagaraSystem* System = Cue.VFX.LoadSynchronous())
		{
			Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContextObject, System, Location, Rotation, Cue.VFXScale,
				true, true, ENCPoolMethod::AutoRelease, true);
		}
	}
	if (!Cue.Sound.IsNull())
	{
		if (USoundBase* Sound = Cue.Sound.LoadSynchronous())
		{
			UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Sound, Location, Rotation);
		}
	}
	PlayCueShake(WorldContextObject, Cue, Location);
	return Component;
}

UNiagaraComponent* UTDStatics::PlayCueAttached(const FTDFXCue& Cue, USceneComponent* AttachTo, FName Socket)
{
	UAudioComponent* Audio = nullptr;
	return PlayCueAttachedLoop(Cue, AttachTo, Socket, Audio);
}

UNiagaraComponent* UTDStatics::PlayCueAttachedLoop(const FTDFXCue& Cue, USceneComponent* AttachTo, FName Socket,
	UAudioComponent*& OutAudio)
{
	OutAudio = nullptr;
	if (!IsValid(AttachTo) || Cue.IsEmpty())
	{
		return nullptr;
	}

	UNiagaraComponent* Component = nullptr;
	if (!Cue.VFX.IsNull())
	{
		if (UNiagaraSystem* System = Cue.VFX.LoadSynchronous())
		{
			Component = UNiagaraFunctionLibrary::SpawnSystemAttached(System, AttachTo, Socket, FVector::ZeroVector,
				FRotator::ZeroRotator, Cue.VFXScale, EAttachLocation::SnapToTarget, true, ENCPoolMethod::None, true);
		}
	}
	if (!Cue.Sound.IsNull())
	{
		if (USoundBase* Sound = Cue.Sound.LoadSynchronous())
		{
			// Stops with its owner: a looping sound must never outlive the ship or prop it was attached to.
			OutAudio = UGameplayStatics::SpawnSoundAttached(Sound, AttachTo, Socket, FVector::ZeroVector,
				EAttachLocation::KeepRelativeOffset, /*bStopWhenAttachedToDestroyed*/ true);
		}
	}
	PlayCueShake(AttachTo, Cue, AttachTo->GetComponentLocation());
	return Component;
}

UTDStatics::UTDStatics()
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(
		TEXT("/Game/TriadDrift/Gameplay/Materials/M_TD_PlaceholderGlow.M_TD_PlaceholderGlow"));
	PlaceholderGlowMaterial = Glow.Object;
}

UMaterialInterface* UTDStatics::GetPlaceholderGlowMaterial()
{
	return GetDefault<UTDStatics>()->PlaceholderGlowMaterial;
}

UMaterialInstanceDynamic* UTDStatics::TintPlaceholder(UMeshComponent* Mesh, FLinearColor Color, float EmissiveBoost)
{
	if (!IsValid(Mesh))
	{
		return nullptr;
	}

	// LoadObject is a cheap find after the first call; weak pointers survive a level change safely.
	static TWeakObjectPtr<UMaterialInterface> LitBase;
	static TWeakObjectPtr<UMaterialInterface> GlowBase;
	if (!LitBase.IsValid())
	{
		LitBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	if (EmissiveBoost > 0.f && !GlowBase.IsValid())
	{
		GlowBase = GetPlaceholderGlowMaterial();
	}

	// Falls back to the lit material if the project glow material is ever missing.
	const bool bGlow = EmissiveBoost > 0.f && GlowBase.IsValid();
	UMaterialInterface* Base = bGlow ? GlowBase.Get() : LitBase.Get();
	if (!Base)
	{
		return nullptr;
	}

	const FLinearColor Final = bGlow ? Color * EmissiveBoost : Color;
	UMaterialInstanceDynamic* First = nullptr;
	const int32 SlotCount = FMath::Max(1, Mesh->GetNumMaterials());
	for (int32 Slot = 0; Slot < SlotCount; ++Slot)
	{
		UMaterialInstanceDynamic* Instance = Mesh->CreateDynamicMaterialInstance(Slot, Base);
		if (!Instance)
		{
			continue;
		}
		Instance->SetVectorParameterValue(TEXT("Color"), Final);
		if (bGlow)
		{
			Instance->SetVectorParameterValue(TEXT("EmissiveColor"), Final);
		}
		First = First ? First : Instance;
	}
	return First;
}

FLinearColor UTDStatics::ShipColor(ETDShipId Ship)
{
	switch (Ship)
	{
	case ETDShipId::Rift: return FLinearColor(1.f, 0.17f, 0.84f);
	case ETDShipId::Lancer: return FLinearColor(0.25f, 0.91f, 1.f);
	default: return FLinearColor(1.f, 0.7f, 0.12f);
	}
}

FLinearColor UTDStatics::TeamColor(ETDTeam Team)
{
	switch (Team)
	{
	case ETDTeam::Player: return FLinearColor(0.55f, 1.f, 0.7f);
	case ETDTeam::Enemy: return FLinearColor(1.f, 0.16f, 0.08f);
	default: return FLinearColor(0.5f, 0.5f, 0.5f);
	}
}

FString UTDStatics::GetPresentationFolder()
{
	return TEXT("/Game/TriadDrift/Presentation");
}

namespace
{
	// Keeps presentation assets, their fallbacks and the merged copies alive for the session WITHOUT rooting them: a
	// rooted object cannot be marked as garbage (tests do that to their temporary assets, and the editor does it when an
	// asset is deleted). Referenced objects that are marked as garbage are simply dropped at the next collection.
	class FTDPresentationKeepAlive : public FGCObject
	{
	public:
		static FTDPresentationKeepAlive& Get()
		{
			static FTDPresentationKeepAlive* Instance = new FTDPresentationKeepAlive();	// never deleted: outlives shutdown order
			return *Instance;
		}
		void Keep(UObject* Object)
		{
			if (Object)
			{
				Objects.AddUnique(Object);
			}
		}
		void Release(UObject* Object) { Objects.Remove(Object); }
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			Objects.RemoveAll([](const TObjectPtr<UObject>& O) { return O == nullptr; });
			Collector.AddReferencedObjects(Objects);
		}
		virtual FString GetReferencerName() const override { return TEXT("FTDPresentationKeepAlive"); }

	private:
		TArray<TObjectPtr<UObject>> Objects;
	};

	bool TDIsReferenceProperty(const FProperty* Prop)
	{
		return Prop->IsA<FObjectPropertyBase>() || Prop->IsA<FArrayProperty>() || Prop->IsA<FMapProperty>()
			|| Prop->IsA<FSetProperty>();
	}

	bool TDIsSetInContainer(const FProperty* Prop, const void* Container, const void* Defaults)
	{
		for (int32 Index = 0; Index < Prop->ArrayDim; ++Index)
		{
			if (!Prop->Identical_InContainer(Container, Defaults, Index))
			{
				return true;
			}
		}
		return false;
	}

	// One cue: the scale belongs to the effect and the shake numbers to the shake, so each group travels together.
	// The presentation side's effect is never shown at the fallback's scale.
	void TDFillCueFrom(FTDFXCue& Dest, const FTDFXCue& Src)
	{
		if (Dest.VFX.IsNull() && !Src.VFX.IsNull())
		{
			Dest.VFX = Src.VFX;
			Dest.VFXScale = Src.VFXScale;
		}
		if (Dest.Sound.IsNull())
		{
			Dest.Sound = Src.Sound;
		}
		if (Dest.Shake.IsNull() && !Src.Shake.IsNull())
		{
			Dest.Shake = Src.Shake;
			Dest.ShakeScale = Src.ShakeScale;
			Dest.ShakeRadius = Src.ShakeRadius;
		}
	}

	// Fills what Dest left unset from Src, level by level:
	//  - a reference (asset pointer, class, array, map, set) is taken from Src when Dest left it at the default;
	//    arrays, maps and sets as a whole, never element by element;
	//  - plain values at one level (numbers, colours, transforms, names) are taken from Src only when Dest set NO
	//    reference at that level, so the fallback's scale or offset never lands on the presentation side's mesh;
	//  - structs are walked member by member; a cue keeps its groups together (TDFillCueFrom).
	void TDFillUnsetFrom(const UStruct* Struct, void* Dest, const void* Src, const void* Defaults)
	{
		bool bDestSetAReference = false;
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			if (TDIsReferenceProperty(*It) && TDIsSetInContainer(*It, Dest, Defaults))
			{
				bDestSetAReference = true;
				break;
			}
		}
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			const FProperty* Prop = *It;
			if (Prop->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient))
			{
				continue;
			}
			if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop); StructProp && Prop->ArrayDim == 1)
			{
				void* DestValue = StructProp->ContainerPtrToValuePtr<void>(Dest);
				const void* SrcValue = StructProp->ContainerPtrToValuePtr<void>(Src);
				if (StructProp->Struct == FTDFXCue::StaticStruct())
				{
					TDFillCueFrom(*static_cast<FTDFXCue*>(DestValue), *static_cast<const FTDFXCue*>(SrcValue));
				}
				else if (StructProp->Struct->IsChildOf(TBaseStructure<FVector>::Get())
					|| StructProp->Struct->IsChildOf(TBaseStructure<FRotator>::Get())
					|| StructProp->Struct->IsChildOf(TBaseStructure<FTransform>::Get())
					|| StructProp->Struct->IsChildOf(TBaseStructure<FLinearColor>::Get())
					|| StructProp->Struct->IsChildOf(TBaseStructure<FColor>::Get())
					|| StructProp->Struct->IsChildOf(TBaseStructure<FVector2D>::Get()))
				{
					// A plain value that happens to be a struct: all or nothing, under the plain-value rule.
					if (!bDestSetAReference && Prop->Identical_InContainer(Dest, Defaults) && !Prop->Identical_InContainer(Src, Defaults))
					{
						Prop->CopyCompleteValue_InContainer(Dest, Src);
					}
				}
				else
				{
					TDFillUnsetFrom(StructProp->Struct, DestValue, SrcValue, StructProp->ContainerPtrToValuePtr<void>(Defaults));
				}
				continue;
			}
			if (!TDIsReferenceProperty(Prop) && bDestSetAReference)
			{
				continue;
			}
			if (!TDIsSetInContainer(Prop, Dest, Defaults) && TDIsSetInContainer(Prop, Src, Defaults))
			{
				Prop->CopyCompleteValue_InContainer(Dest, Src);
			}
		}
	}

	// Finds an engineering fallback of that name under the always-cooked Gameplay/Characters folder.
	UObject* TDFindFallbackPresentationAsset(const FString& AssetName)
	{
		IAssetRegistry* Registry = IAssetRegistry::Get();
		if (!Registry)
		{
			return nullptr;
		}
		// One lookup per name for the session (the music director and every hit confirm ask through here): the
		// found asset is rooted, so it is neither searched for nor loaded again.
		static TMap<FString, TWeakObjectPtr<UObject>> FallbackCache;
		if (const TWeakObjectPtr<UObject>* Cached = FallbackCache.Find(AssetName); Cached && Cached->IsValid())
		{
			return Cached->Get();
		}
		const FName Wanted(*AssetName);
		auto Search = [Registry, &Wanted]() -> UObject*
		{
			TArray<FAssetData> Found;
			Registry->GetAssetsByPath(FName(TEXT("/Game/TriadDrift/Gameplay/Characters")), Found, /*bRecursive*/ true);
			for (const FAssetData& Data : Found)
			{
				if (Data.AssetName == Wanted)
				{
					return Data.GetSoftObjectPath().TryLoad();
				}
			}
			return nullptr;
		};
		UObject* Asset = Search();
		// A -game / first-start world can begin play (and spawn the squad) before the background scan has gathered
		// this folder: scan it synchronously once and look again, so the first lookup never misses for good.
		static bool bCharactersScanned = false;
		if (!Asset && !bCharactersScanned)
		{
			bCharactersScanned = true;
			Registry->ScanPathsSynchronous({ TEXT("/Game/TriadDrift/Gameplay/Characters") }, /*bForceRescan*/ false);
			Asset = Search();
		}
		if (Asset)
		{
			FTDPresentationKeepAlive::Get().Keep(Asset);
			FallbackCache.Add(AssetName, Asset);
		}
		return Asset;
	}
}

UObject* UTDStatics::MergePresentationWithFallback(const UObject* Primary, const UObject* Fallback)
{
	if (!Primary)
	{
		return nullptr;
	}
	UObject* Merged = DuplicateObject<UObject>(Primary, GetTransientPackage());
	if (Merged && Fallback && Fallback != Primary && Fallback->GetClass() == Primary->GetClass())
	{
		TDFillUnsetFrom(Merged->GetClass(), Merged, Fallback, Merged->GetClass()->GetDefaultObject());
	}
	return Merged;
}

UObject* UTDStatics::LoadPresentationAsset(const FString& AssetName, TSubclassOf<UObject> ExpectedClass)
{
	if (AssetName.IsEmpty())
	{
		return nullptr;
	}

	// Remembered misses: asking again every frame for art that does not exist yet must cost nothing.
	static TMap<FString, double> MissTimes;
	const double Now = FPlatformTime::Seconds();
	if (const double* MissedAt = MissTimes.Find(AssetName))
	{
		if (!GIsEditor || Now - *MissedAt < 10.0)
		{
			return nullptr;
		}
	}

	const FString PackagePath = GetPresentationFolder() / AssetName;
	const FSoftObjectPath ObjectPath(PackagePath + TEXT(".") + AssetName);

	// Ask the asset registry first (silent, and it is what a cooked build has). While the editor is
	// still scanning, fall back to the package file check.
	bool bExists = false;
	if (IAssetRegistry* Registry = IAssetRegistry::Get())
	{
		bExists = Registry->GetAssetByObjectPath(ObjectPath).IsValid();
		if (!bExists && Registry->IsLoadingAssets())
		{
			bExists = FPackageName::DoesPackageExist(PackagePath);
		}
	}
	else
	{
		bExists = FPackageName::DoesPackageExist(PackagePath);
	}

	UObject* Asset = bExists ? ObjectPath.TryLoad() : nullptr;

	// Second place to look: engineering-made fallbacks (R-019, N-021, R-024) live in an always-cooked Gameplay
	// folder. Presentation wins FIELD BY FIELD (R-029): whatever the Presentation asset leaves at its default is
	// filled from the fallback of the same name, so a sound-only or music-only asset from the presentation side
	// never switches off the effects, sounds or meshes engineering supplies for the other fields.
	if (!Asset)
	{
		Asset = TDFindFallbackPresentationAsset(AssetName);
	}
	else
	{
		struct FMergedEntry
		{
			TWeakObjectPtr<UObject> Primary;
			TWeakObjectPtr<UObject> Merged;
		};
		static TMap<FString, FMergedEntry> MergedCache;
		FMergedEntry& Entry = MergedCache.FindOrAdd(AssetName);
		if (Entry.Primary.Get() != Asset || !Entry.Merged.IsValid())
		{
			if (UObject* Old = Entry.Merged.Get(); Old && Old != Entry.Primary.Get())
			{
				FTDPresentationKeepAlive::Get().Release(Old);
			}
			FTDPresentationKeepAlive::Get().Keep(Asset);	// the original stays loaded too, or every collection would reload and re-merge it
			Entry.Primary = Asset;
			Entry.Merged = Asset;	// nothing to merge: hand out the asset itself
			const UObject* Fallback = TDFindFallbackPresentationAsset(AssetName);
			if (Fallback && Fallback != Asset && Fallback->GetClass() == Asset->GetClass())
			{
				if (UObject* Merged = MergePresentationWithFallback(Asset, Fallback))
				{
					FTDPresentationKeepAlive::Get().Keep(Merged);	// lives for the session; callers keep raw pointers to presentation assets
					Entry.Merged = Merged;
				}
			}
		}
		if (Entry.Merged.IsValid())
		{
			Asset = Entry.Merged.Get();
		}
	}
	if (Asset && ExpectedClass && !Asset->IsA(ExpectedClass))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Presentation asset %s is a %s, expected %s"), *AssetName,
			*Asset->GetClass()->GetName(), *ExpectedClass->GetName());
		Asset = nullptr;
	}
	if (!Asset)
	{
		const IAssetRegistry* Registry = IAssetRegistry::Get();
		if (!Registry || !Registry->IsLoadingAssets())
		{
			MissTimes.Add(AssetName, Now); // never remember a miss while the registry is still scanning
		}
		return nullptr;
	}
	MissTimes.Remove(AssetName);
	return Asset;
}

bool UTDStatics::ShouldDrawPlaceholders()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return true;
#endif
}

// ----------------------------------------------------------------------------------------------- feel

UTDFeelSubsystem* UTDFeelSubsystem::Get(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
	{
		return nullptr;
	}
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UTDFeelSubsystem>() : nullptr;
}

void UTDFeelSubsystem::RequestHitStop(float RealSeconds, float Dilation)
{
	const UWorld* World = GetWorld();
	if (!World || RealSeconds <= 0.f)
	{
		return;
	}
	const double Now = World->GetRealTimeSeconds();
	HitStopEnd = FMath::Max(HitStopEnd, Now + RealSeconds);
	HitStopDilation = FMath::Clamp(Dilation, 0.001f, 1.f);
	Apply();
}

void UTDFeelSubsystem::RequestSlowMo(float RealSeconds, float Dilation)
{
	const UWorld* World = GetWorld();
	if (!World || RealSeconds <= 0.f)
	{
		return;
	}
	const double Now = World->GetRealTimeSeconds();
	const float Clamped = FMath::Clamp(Dilation, 0.001f, 1.f);
	if (Now < SlowMoEnd)
	{
		// Overlapping requests: keep the slowest and the latest end.
		SlowMoDilation = FMath::Min(SlowMoDilation, Clamped);
		SlowMoEnd = FMath::Max(SlowMoEnd, Now + RealSeconds);
	}
	else
	{
		SlowMoDilation = Clamped;
		SlowMoEnd = Now + RealSeconds;
	}
	Apply();
}

void UTDFeelSubsystem::CancelAll()
{
	HitStopEnd = 0.0;
	SlowMoEnd = 0.0;
	Apply();
}

bool UTDFeelSubsystem::IsSlowMoActive() const
{
	const UWorld* World = GetWorld();
	return World && World->GetRealTimeSeconds() < SlowMoEnd;
}

void UTDFeelSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Apply();
}

TStatId UTDFeelSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDFeelSubsystem, STATGROUP_Tickables);
}

void UTDFeelSubsystem::Apply()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetRealTimeSeconds();
	float Desired = 1.f;
	if (Now < HitStopEnd)
	{
		Desired = FMath::Min(Desired, HitStopDilation);
	}
	if (Now < SlowMoEnd)
	{
		Desired = FMath::Min(Desired, SlowMoDilation);
	}
	if (!FMath::IsNearlyEqual(Desired, CurrentDilation, KINDA_SMALL_NUMBER))
	{
		CurrentDilation = Desired;
		if (AWorldSettings* Settings = World->GetWorldSettings())
		{
			Settings->SetTimeDilation(Desired);
		}
	}
}
