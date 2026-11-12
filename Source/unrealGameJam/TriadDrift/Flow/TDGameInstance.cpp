// Triad Drift - game instance (ENG-19, C-03 #3). Owner: Teammate 5.

#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace TDFlow
{
	static const FName ReasonLevelEntry(TEXT("LevelEntry"));
	static const FName ReasonWaveCleared(TEXT("WaveCleared"));
	static const FName ReasonObjective(TEXT("ObjectiveComplete"));
	static const FName ReasonManual(TEXT("Manual"));
	static const FName StatDeaths(TEXT("Deaths"));
}

UTDGameInstance* UTDGameInstance::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? Cast<UTDGameInstance>(World->GetGameInstance()) : nullptr;
}

void UTDGameInstance::Init()
{
	Super::Init();
	RunTimerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UTDGameInstance::TickRunTimer));
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UTDGameInstance::HandlePostLoadMap);
	BindEventBus(); // subsystems are initialized by Super::Init()
	StartNewRun();
}

void UTDGameInstance::BindEventBus()
{
	UTDEventBus* Bus = GetBus();
	if (!Bus)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("UTDGameInstance: no event bus; kills/deaths and wave scenes won't be tracked."));
		return;
	}
	Bus->OnActorKilled.AddUniqueDynamic(this, &UTDGameInstance::HandleActorKilled);
	Bus->OnWaveStarted.AddUniqueDynamic(this, &UTDGameInstance::HandleWaveStarted);
	Bus->OnWaveCleared.AddUniqueDynamic(this, &UTDGameInstance::HandleWaveCleared);
}

void UTDGameInstance::HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind)
{
	if (!Victim)
	{
		return;
	}
	const ETDTeam VictimTeam = UTDStatics::GetTeam(Victim);
	if (VictimTeam == ETDTeam::Enemy && Killer && UTDStatics::GetTeam(Killer) == ETDTeam::Player)
	{
		AddStat(TEXT("Kills"), 1);
	}
	else if (VictimTeam == ETDTeam::Player)
	{
		// Only the pawn the player is piloting counts as a death; wingmen being downed don't.
		const APawn* Pawn = Cast<APawn>(Victim);
		if (Pawn && Pawn->IsPlayerControlled())
		{
			AddStat(TDFlow::StatDeaths, 1);
		}
	}
}

void UTDGameInstance::HandleWaveStarted(int32 WaveNumber, float Intensity01)
{
	BeginScene(WaveSceneId(WaveNumber));
}

void UTDGameInstance::HandleWaveCleared(int32 WaveNumber)
{
	ReportWaveCleared(WaveNumber);
}

FName UTDGameInstance::WaveSceneId(int32 WaveNumber)
{
	return WaveNumber > 0 ? FName(*FString::Printf(TEXT("Wave%d"), WaveNumber)) : NAME_None;
}

void UTDGameInstance::Shutdown()
{
	FTSTicker::GetCoreTicker().RemoveTicker(RunTimerHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	GetTimerManager().ClearTimer(RetakeTimer);
	Super::Shutdown();
}

UTDEventBus* UTDGameInstance::GetBus() const
{
	return GetSubsystem<UTDEventBus>();
}

// ------------------------------------------------------------------------------------------------
// ITDGameFlow
// ------------------------------------------------------------------------------------------------

bool UTDGameInstance::TravelTo(ETDDestination Destination, FName ReturnPoint)
{
	const UTDFlowSettings* Settings = UTDFlowSettings::Get();
	FString PackageName;
	if (!Settings->ResolveDestinationMap(Destination, PackageName))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("TravelTo(%s): no map configured or the map doesn't exist yet; staying put."),
			*UEnum::GetValueAsString(Destination));
		return false;
	}
	if (bTravelPending)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("TravelTo(%s): travel already in progress; ignored."), *UEnum::GetValueAsString(Destination));
		return false;
	}

	Progress.ReturnPoint = ReturnPoint;
	bTravelPending = true;
	PendingDestination = Destination;
	SetRunTimerPaused(true);
	GetTimerManager().ClearTimer(RetakeTimer);
	bRetakePending = false;

	// Fade out on the local player, then change level.
	UWorld* World = GetWorld();
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	const float Fade = Settings->TravelFadeSeconds;
	if (PC && PC->PlayerCameraManager && Fade > 0.f)
	{
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, Fade, FLinearColor::Black, false, true);
		FTimerHandle Handle;
		GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateUObject(this, &UTDGameInstance::OpenDestination, Destination, PackageName), Fade, false);
	}
	else
	{
		OpenDestination(Destination, PackageName);
	}
	return true;
}

void UTDGameInstance::OpenDestination(ETDDestination Destination, const FString& PackageName)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		bTravelPending = false;
		return;
	}
	const UTDFlowSettings* Settings = UTDFlowSettings::Get();
	const FString TransitionPackage = Settings->TransitionMap.GetLongPackageName();
	if (!TransitionPackage.IsEmpty() && TransitionPackage != PackageName && FPackageName::DoesPackageExist(TransitionPackage))
	{
		UGameplayStatics::OpenLevel(World, FName(*TransitionPackage));
		return; // HandlePostLoadMap continues to the destination.
	}
	UGameplayStatics::OpenLevel(World, FName(*PackageName));
}

void UTDGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!bTravelPending || !LoadedWorld || LoadedWorld->GetGameInstance() != this)
	{
		return;
	}
	const UTDFlowSettings* Settings = UTDFlowSettings::Get();
	const FString LoadedPackage = LoadedWorld->GetOutermost()->GetName();
	FString TargetPackage;
	if (!Settings->ResolveDestinationMap(PendingDestination, TargetPackage))
	{
		bTravelPending = false;
		return;
	}
	if (LoadedPackage == TargetPackage || LoadedPackage.EndsWith(FPackageName::GetShortName(TargetPackage)))
	{
		bTravelPending = false;
		CurrentDestination = PendingDestination;
		SetRunTimerPaused(false);
		return;
	}
	// We're on the transition map: hold for MinTransitionSeconds, then go on.
	const ETDDestination Dest = PendingDestination;
	FTimerHandle Handle;
	GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Dest, TargetPackage]()
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::OpenLevel(World, FName(*TargetPackage));
		}
	}), FMath::Max(0.01f, Settings->MinTransitionSeconds), false);
}

FTDProgress UTDGameInstance::GetProgress() const
{
	return Progress;
}

void UTDGameInstance::SetPhase(ETDGamePhase NewPhase)
{
	if (Progress.Phase == NewPhase)
	{
		return;
	}
	Progress.Phase = NewPhase;
	SetRunTimerPaused(NewPhase == ETDGamePhase::Victory);
	if (UTDEventBus* Bus = GetBus())
	{
		Bus->BroadcastPhaseChanged(NewPhase);
	}
}

void UTDGameInstance::ReportWaveCleared(int32 WaveNumber)
{
	// 1-based wave numbers. Idempotent: the wave director may both call this and broadcast OnWaveCleared.
	if (WaveNumber <= 0 || WaveNumber <= Progress.WavesCleared)
	{
		return;
	}
	Progress.WavesCleared = WaveNumber;
	EndScene();
	if (const ETDGamePhase* Next = UTDFlowSettings::Get()->PhaseAfterWaveCleared.Find(WaveNumber))
	{
		SetPhase(*Next);
	}
	TakeCheckpoint(TDFlow::ReasonWaveCleared);
}

void UTDGameInstance::CompleteObjective(FName ObjectiveId, ETDAbilityId Reward, ETDShipId ForShip)
{
	if (ObjectiveId.IsNone())
	{
		return;
	}
	const bool bAlreadyDone = Progress.CompletedObjectives.Contains(ObjectiveId);
	Progress.CompletedObjectives.Add(ObjectiveId);
	if (!bAlreadyDone && Reward != ETDAbilityId::None)
	{
		UnlockAbility(Reward, ForShip);
	}
	EndScene();
	TakeCheckpoint(TDFlow::ReasonObjective);
}

void UTDGameInstance::SetDominionState(bool bRevealed, bool bDefensesDown)
{
	Progress.bDominionRevealed = bRevealed;
	Progress.bDominionDefensesDown = bDefensesDown;
}

void UTDGameInstance::ReportPlayerDefeated()
{
	if (bRetakePending || bTravelPending)
	{
		return; // already cutting
	}
	// Deaths are counted from OnActorKilled (player-piloted pawn deaths); defeat is the scene failing.
	bRetakePending = true;

	const float Delay = UTDFlowSettings::Get()->RetakeDelaySeconds;
	if (Delay <= 0.f)
	{
		ExecuteRetake();
	}
	else
	{
		GetTimerManager().SetTimer(RetakeTimer, FTimerDelegate::CreateUObject(this, &UTDGameInstance::ExecuteRetake), Delay, false);
	}
}

void UTDGameInstance::RestartSceneNow()
{
	GetTimerManager().ClearTimer(RetakeTimer);
	bRetakePending = true;
	ExecuteRetake();
}

void UTDGameInstance::ExecuteRetake()
{
	bRetakePending = false;
	const FName Scene = CurrentSceneId;

	// Takes, stats and run time are the record of the run: they survive the restore.
	const TMap<FName, int32> KeptTakes = Progress.Takes;
	const FTDRunStats KeptStats = Progress.Stats;
	const float KeptRunTime = Progress.RunTimeSeconds;

	if (LastCheckpoint.bValid)
	{
		Progress = LastCheckpoint.Progress;
	}
	Progress.Takes = KeptTakes;
	Progress.Stats = KeptStats;
	Progress.RunTimeSeconds = KeptRunTime;

	int32 Take = 1;
	if (!Scene.IsNone())
	{
		int32& Count = Progress.Takes.FindOrAdd(Scene);
		Count = FMath::Max(1, Count) + 1;
		Take = Count;
		SceneOrder.AddUnique(Scene);
	}
	LastAnnouncedScene = Scene;
	LastAnnouncedTake = Take;

	OnCheckpointRestored.Broadcast(LastCheckpoint);
	OnSceneRestart.Broadcast(Scene, Take);
	if (!Scene.IsNone())
	{
		if (UTDEventBus* Bus = GetBus())
		{
			Bus->BroadcastTakeStarted(Scene, Take);
		}
	}
	UE_LOG(LogTriadDrift, Log, TEXT("CUT! Scene %s restarts as take %d."), *Scene.ToString(), Take);
}

void UTDGameInstance::AddStat(FName StatName, int32 Amount)
{
	if (StatName.IsNone() || Amount == 0)
	{
		return;
	}
	// Resolve the field by name so new FTDRunStats fields work without touching Flow.
	const UScriptStruct* Struct = FTDRunStats::StaticStruct();
	const FProperty* Prop = Struct->FindPropertyByName(StatName);
	if (!Prop)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("AddStat: FTDRunStats has no field '%s'."), *StatName.ToString());
		return;
	}
	void* Field = Prop->ContainerPtrToValuePtr<void>(&Progress.Stats);
	if (const FIntProperty* IntProp = CastField<FIntProperty>(Prop))
	{
		IntProp->SetPropertyValue(Field, IntProp->GetPropertyValue(Field) + Amount);
	}
	else if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
	{
		FloatProp->SetPropertyValue(Field, FloatProp->GetPropertyValue(Field) + static_cast<float>(Amount));
	}
	else
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("AddStat: FTDRunStats.%s is not numeric."), *StatName.ToString());
	}
}

void UTDGameInstance::RequestBark(FName BarkId, FName Speaker)
{
	if (UTDEventBus* Bus = GetBus())
	{
		Bus->BroadcastBarkRequested(BarkId, Speaker);
	}
}

void UTDGameInstance::ShowTip(FName TipId)
{
	if (UTDEventBus* Bus = GetBus())
	{
		Bus->BroadcastTipRequested(TipId);
	}
}

void UTDGameInstance::SaveCheckpoint()
{
	TakeCheckpoint(TDFlow::ReasonManual);
}

// ------------------------------------------------------------------------------------------------
// Run lifecycle
// ------------------------------------------------------------------------------------------------

void UTDGameInstance::StartNewRun()
{
	Progress = FTDProgress();
	LastCheckpoint = FTDCheckpoint();
	SceneOrder.Reset();
	CurrentSceneId = NAME_None;
	LastAnnouncedScene = NAME_None;
	LastAnnouncedTake = 0;
	bRetakePending = false;
	bRunTimerPaused = false;
	GetTimerManager().ClearTimer(RetakeTimer);
}

bool UTDGameInstance::HasContinue() const
{
	UTDSaveGame* Save = nullptr;
	return ReadSaveSlot(Save);
}

bool UTDGameInstance::ContinueRun()
{
	UTDSaveGame* Save = nullptr;
	if (!ReadSaveSlot(Save) || !ApplySave(*Save))
	{
		return false;
	}
	if (!TravelTo(LastCheckpoint.Destination, Progress.ReturnPoint))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("ContinueRun: progress restored, but travel to the saved destination failed."));
	}
	return true;
}

void UTDGameInstance::DeleteSave()
{
	const FString Slot = GetSaveSlotName();
	if (UGameplayStatics::DoesSaveGameExist(Slot, UTDFlowSettings::Get()->SaveUserIndex))
	{
		UGameplayStatics::DeleteGameInSlot(Slot, UTDFlowSettings::Get()->SaveUserIndex);
	}
}

void UTDGameInstance::NotifyLevelEntered(ETDDestination Destination)
{
	CurrentDestination = Destination;
	SetRunTimerPaused(Destination == ETDDestination::MainMenu || Destination == ETDDestination::Victory);
	TakeCheckpoint(TDFlow::ReasonLevelEntry);
}

// ------------------------------------------------------------------------------------------------
// Scenes and takes
// ------------------------------------------------------------------------------------------------

void UTDGameInstance::BeginScene(FName SceneId)
{
	if (SceneId.IsNone())
	{
		return;
	}
	CurrentSceneId = SceneId;
	int32& Take = Progress.Takes.FindOrAdd(SceneId);
	Take = FMath::Max(1, Take);
	SceneOrder.AddUnique(SceneId);

	// A retake already announced this take; the restarted wave/objective calling BeginScene again
	// must not double-fire the slate.
	if (LastAnnouncedScene == SceneId && LastAnnouncedTake == Take)
	{
		return;
	}
	LastAnnouncedScene = SceneId;
	LastAnnouncedTake = Take;
	if (UTDEventBus* Bus = GetBus())
	{
		Bus->BroadcastTakeStarted(SceneId, Take);
	}
}

void UTDGameInstance::EndScene()
{
	CurrentSceneId = NAME_None;
	LastAnnouncedScene = NAME_None;
	LastAnnouncedTake = 0;
}

int32 UTDGameInstance::GetTakeNumber(FName SceneId) const
{
	const int32* Take = Progress.Takes.Find(SceneId);
	return Take ? *Take : 0;
}

int32 UTDGameInstance::GetSceneNumber(FName SceneId) const
{
	const int32 Index = SceneOrder.IndexOfByKey(SceneId);
	return Index == INDEX_NONE ? 0 : Index + 1;
}

TArray<FTDSceneTakes> UTDGameInstance::GetSceneTakes() const
{
	TArray<FTDSceneTakes> Out;
	Out.Reserve(SceneOrder.Num());
	for (int32 i = 0; i < SceneOrder.Num(); ++i)
	{
		FTDSceneTakes& Row = Out.AddDefaulted_GetRef();
		Row.SceneId = SceneOrder[i];
		Row.SceneNumber = i + 1;
		Row.Takes = GetTakeNumber(SceneOrder[i]);
	}
	return Out;
}

// ------------------------------------------------------------------------------------------------
// Checkpoints & save slot
// ------------------------------------------------------------------------------------------------

void UTDGameInstance::TakeCheckpoint(FName Reason)
{
	LastCheckpoint.Progress = Progress;
	LastCheckpoint.Destination = CurrentDestination;
	LastCheckpoint.SceneId = CurrentSceneId;
	LastCheckpoint.Reason = Reason;
	LastCheckpoint.bValid = true;
	if (UTDFlowSettings::Get()->bWriteSlotOnCheckpoint)
	{
		WriteSaveSlot();
	}
}

FString UTDGameInstance::GetSaveSlotName() const
{
	return SaveSlotOverride.IsEmpty() ? UTDFlowSettings::Get()->SaveSlotName : SaveSlotOverride;
}

bool UTDGameInstance::WriteSaveSlot() const
{
	if (!LastCheckpoint.bValid)
	{
		return false;
	}
	UTDSaveGame* Save = Cast<UTDSaveGame>(UGameplayStatics::CreateSaveGameObject(UTDSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}
	Save->Version = UTDSaveGame::CurrentVersion;
	Save->Checkpoint = LastCheckpoint;
	// The slot always carries the latest takes/stats, even when the checkpoint is older.
	Save->Checkpoint.Progress.Takes = Progress.Takes;
	Save->Checkpoint.Progress.Stats = Progress.Stats;
	Save->Checkpoint.Progress.RunTimeSeconds = Progress.RunTimeSeconds;
	Save->SceneOrder = SceneOrder;
	Save->SavedAtUtc = FDateTime::UtcNow();
	const bool bOk = UGameplayStatics::SaveGameToSlot(Save, GetSaveSlotName(), UTDFlowSettings::Get()->SaveUserIndex);
	if (!bOk)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Failed to write save slot '%s'."), *GetSaveSlotName());
	}
	return bOk;
}

bool UTDGameInstance::ReadSaveSlot(UTDSaveGame*& OutSave) const
{
	OutSave = nullptr;
	const FString Slot = GetSaveSlotName();
	const int32 User = UTDFlowSettings::Get()->SaveUserIndex;
	if (!UGameplayStatics::DoesSaveGameExist(Slot, User))
	{
		return false;
	}
	UTDSaveGame* Save = Cast<UTDSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, User));
	if (!Save)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Save slot '%s' is not a Triad Drift save; ignoring it."), *Slot);
		return false;
	}
	if (!Save->IsCompatible())
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Save slot '%s' has version %d (expected %d); ignoring it."), *Slot, Save->Version, UTDSaveGame::CurrentVersion);
		return false;
	}
	OutSave = Save;
	return true;
}

bool UTDGameInstance::ApplySave(const UTDSaveGame& Save)
{
	if (!Save.IsCompatible())
	{
		return false;
	}
	GetTimerManager().ClearTimer(RetakeTimer);
	bRetakePending = false;
	LastCheckpoint = Save.Checkpoint;
	Progress = Save.Checkpoint.Progress;
	SceneOrder = Save.SceneOrder;
	CurrentDestination = Save.Checkpoint.Destination;
	CurrentSceneId = Save.Checkpoint.SceneId;
	LastAnnouncedScene = NAME_None;
	LastAnnouncedTake = 0;
	return true;
}

// ------------------------------------------------------------------------------------------------
// Progress helpers
// ------------------------------------------------------------------------------------------------

bool UTDGameInstance::IsAbilityUnlocked(ETDAbilityId Ability) const
{
	return Ability != ETDAbilityId::None && Progress.UnlockedAbilities.Contains(Ability);
}

void UTDGameInstance::UnlockAbility(ETDAbilityId Ability, ETDShipId ForShip)
{
	if (Ability == ETDAbilityId::None)
	{
		return;
	}
	const ETDShipId* Existing = Progress.UnlockedAbilities.Find(Ability);
	if (Existing && *Existing == ForShip)
	{
		return;
	}
	Progress.UnlockedAbilities.Add(Ability, ForShip);
	if (UTDEventBus* Bus = GetBus())
	{
		Bus->BroadcastAbilityUnlocked(Ability, ForShip);
	}
}

void UTDGameInstance::SetLastShip(ETDShipId Ship)
{
	Progress.LastShip = Ship;
}

void UTDGameInstance::AdvanceRunTime(float DeltaSeconds)
{
	if (!bRunTimerPaused && !bTravelPending && DeltaSeconds > 0.f)
	{
		Progress.RunTimeSeconds += DeltaSeconds;
	}
}

bool UTDGameInstance::TickRunTimer(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && !World->IsPaused())
	{
		// Real time, not dilated: director-cam slow motion shouldn't stretch the run clock.
		AdvanceRunTime(DeltaSeconds);
	}
	return true;
}
