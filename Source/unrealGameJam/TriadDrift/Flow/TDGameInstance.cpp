// Triad Drift - game instance (ENG-19, C-03 #3). Owner: Teammate 5.

#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDDebugMenu.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
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
	static const FName StatKills(TEXT("Kills"));

	/** An engine save file starts with the 'GVAS' tag; anything else (truncated, garbage) is rejected. */
	static bool LooksLikeSaveGame(const TArray<uint8>& Bytes)
	{
		return Bytes.Num() > 64 && Bytes[0] == 'G' && Bytes[1] == 'V' && Bytes[2] == 'A' && Bytes[3] == 'S';
	}

	static bool SamePackage(const FString& A, const FString& B)
	{
		if (A.IsEmpty() || B.IsEmpty())
		{
			return false;
		}
		// PIE prefixes loaded packages ("UEDPIE_0_L_Galaxy"); compare short names, suffix-tolerant.
		const FString SA = FPackageName::GetShortName(A);
		const FString SB = FPackageName::GetShortName(B);
		return A == B || SA == SB || SA.EndsWith(TEXT("_") + SB);
	}
}

UTDGameInstance::UTDGameInstance()
{
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
	ResetRunState();
	bRunCommitted = false; // a fresh boot must not overwrite an existing save
}

void UTDGameInstance::Shutdown()
{
	FTSTicker::GetCoreTicker().RemoveTicker(RunTimerHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	if (DebugMenu)
	{
		DebugMenu->Hide();
		DebugMenu.Reset();
	}
	Super::Shutdown();
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
	Bus->OnObjectiveUpdated.AddUniqueDynamic(this, &UTDGameInstance::HandleObjectiveUpdated);
}

void UTDGameInstance::HandleObjectiveUpdated(FName ObjectiveId, int32 Step, FText Text)
{
	LastObjective.Id = ObjectiveId;
	LastObjective.Step = Step;
	LastObjective.Text = Text;
}

TArray<AActor*> UTDGameInstance::GetFlowMarkers() const
{
	TArray<AActor*> Out;
	for (const TWeakObjectPtr<AActor>& A : FlowMarkers)
	{
		if (A.IsValid())
		{
			Out.Add(A.Get());
		}
	}
	return Out;
}

void UTDGameInstance::SyncMarkers(bool bClearAll)
{
	TArray<AActor*> Want;
	if (!bClearAll)
	{
		FTDObjectiveText::ComputeMarkers(GetWorld(), Progress, CurrentDestination, Want);
	}
	for (const TWeakObjectPtr<AActor>& Old : FlowMarkers)
	{
		if (Old.IsValid() && !Want.Contains(Old.Get()))
		{
			Old->Tags.Remove(TDTags::ObjectiveMarker);
		}
	}
	FlowMarkers.Reset();
	for (AActor* A : Want)
	{
		A->Tags.AddUnique(TDTags::ObjectiveMarker);
		FlowMarkers.Add(A);
	}
}

void UTDGameInstance::RefreshObjective(bool bForce)
{
	UWorld* World = GetWorld();
	UTDEventBus* Bus = GetBus();
	if (!World || !Bus || IsTravelPending())
	{
		if (IsTravelPending())
		{
			SyncMarkers(true); // nothing is the objective mid-travel
		}
		return;
	}
	SyncMarkers(IsRetakePending() || IsMenuDestination(CurrentDestination));
	FTDObjectiveLine Line;
	if (FTDObjectiveText::Compute(World, Progress, CurrentDestination, Line))
	{
		if (bForce || Line != LastObjective)
		{
			Bus->BroadcastObjectiveUpdated(Line.Id, Line.Step, Line.Text); // HandleObjectiveUpdated stores it
		}
	}
	else if (bForce && LastObjective.IsSet() && !IsMenuDestination(CurrentDestination))
	{
		// On foot with a planet objective running: repeat its current step for a new HUD.
		Bus->BroadcastObjectiveUpdated(LastObjective.Id, LastObjective.Step, LastObjective.Text);
	}
}

UTDEventBus* UTDGameInstance::GetBus() const
{
	return GetSubsystem<UTDEventBus>();
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
		AddStat(TDFlow::StatKills, 1);
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
	// The wave director owns scene ids (its wave table can rename them); "WaveN" is only the fallback.
	FName Scene = NAME_None;
	if (const ATDWaveDirector* Director = ATDWaveDirector::Get(this))
	{
		const FTDWaveInfo Info = Director->GetWaveInfo();
		if (Info.WaveNumber == WaveNumber)
		{
			Scene = Info.SceneId;
		}
	}
	BeginScene(Scene.IsNone() ? WaveSceneId(WaveNumber) : Scene);
}

void UTDGameInstance::HandleWaveCleared(int32 WaveNumber)
{
	ReportWaveCleared(WaveNumber);
}

FName UTDGameInstance::WaveSceneId(int32 WaveNumber)
{
	return WaveNumber > 0 ? FName(*FString::Printf(TEXT("Wave%d"), WaveNumber)) : NAME_None;
}

// ------------------------------------------------------------------------------------------------
// Travel
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
	if (IsTravelPending())
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("TravelTo(%s): travel already in progress; ignored."), *UEnum::GetValueAsString(Destination));
		return false;
	}

	Progress.ReturnPoint = ReturnPoint;
	PendingDestination = Destination;
	PendingPackage = PackageName;
	TravelElapsed = 0.f;
	RetakeTimeLeft = -1.f; // leaving: no retake
	SetRunTimerPaused(true);

	// Quitting from the pause menu: unpause so the level change and camera fade run.
	UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	const float Fade = Settings->TravelFadeSeconds;
	if (PC && PC->PlayerCameraManager && Fade > 0.f)
	{
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, Fade, FLinearColor::Black, false, true);
		TravelStage = ETravelStage::FadingOut;
		TravelStageTimeLeft = Fade;
	}
	else
	{
		TravelStage = ETravelStage::FadingOut;
		TravelStageTimeLeft = 0.f; // next flow tick opens the level
	}
	return true;
}

void UTDGameInstance::OpenLevelByName(const FString& PackageName)
{
	if (OpenLevelOverride)
	{
		OpenLevelOverride(PackageName);
		return;
	}
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::OpenLevel(World, FName(*PackageName));
	}
}

void UTDGameInstance::BeginLoad(const FString& PackageName)
{
	TravelStage = ETravelStage::Loading;
	OpenLevelByName(PackageName);
}

void UTDGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (LoadedWorld && LoadedWorld->GetGameInstance() == this)
	{
		NotifyMapLoaded(LoadedWorld->GetOutermost()->GetName());
	}
}

void UTDGameInstance::NotifyMapLoaded(const FString& LoadedPackageName)
{
	if (!IsTravelPending())
	{
		return;
	}
	if (TDFlow::SamePackage(LoadedPackageName, PendingPackage))
	{
		FinishTravel(true);
		return;
	}
	const FString Transition = UTDFlowSettings::Get()->TransitionMap.GetLongPackageName();
	if (TravelStage == ETravelStage::Loading && TDFlow::SamePackage(LoadedPackageName, Transition))
	{
		// On the loading screen: hold MinTransitionSeconds (real time), then load the target once.
		TravelStage = ETravelStage::OnTransitionMap;
		TravelStageTimeLeft = FMath::Max(0.f, UTDFlowSettings::Get()->MinTransitionSeconds);
		return;
	}
	// Some other level loaded (another system travelled): our travel is void.
	UE_LOG(LogTriadDrift, Warning, TEXT("Travel to %s superseded by %s."), *PendingPackage, *LoadedPackageName);
	FinishTravel(false);
}

void UTDGameInstance::FinishTravel(bool bArrived)
{
	if (bArrived)
	{
		CurrentDestination = PendingDestination;
	}
	TravelStage = ETravelStage::None;
	TravelStageTimeLeft = 0.f;
	TravelElapsed = 0.f;
	PendingPackage.Reset();
	SetRunTimerPaused(IsMenuDestination(CurrentDestination));
}

void UTDGameInstance::TickFlow(float RealDeltaSeconds)
{
	if (RealDeltaSeconds <= 0.f)
	{
		return;
	}

	// CUT! -> retake.
	if (RetakeTimeLeft >= 0.f)
	{
		RetakeTimeLeft -= RealDeltaSeconds;
		if (RetakeTimeLeft < 0.f)
		{
			ExecuteRetake();
		}
	}

	// Victory: after the hold, to the victory screen (the menu map shows it, then the menu).
	if (VictoryTravelLeft >= 0.f && !IsTravelPending())
	{
		VictoryTravelLeft -= RealDeltaSeconds;
		if (VictoryTravelLeft < 0.f)
		{
			TravelTo(ETDDestination::Victory, NAME_None);
		}
	}

	// Objective line for flow-owned scenes (cheap; only broadcasts on change or when forced).
	ObjectiveCheckLeft -= RealDeltaSeconds;
	bool bForceObjective = false;
	if (ObjectiveForceLeft >= 0.f)
	{
		ObjectiveForceLeft -= RealDeltaSeconds;
		bForceObjective = ObjectiveForceLeft < 0.f;
	}
	if (ObjectiveCheckLeft <= 0.f || bForceObjective)
	{
		ObjectiveCheckLeft = 0.25f;
		RefreshObjective(bForceObjective);
	}

	// Travel.
	if (TravelStage != ETravelStage::None)
	{
		TravelElapsed += RealDeltaSeconds;
		if (TravelElapsed > UTDFlowSettings::Get()->TravelTimeoutSeconds)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("Travel to %s timed out; giving control back."), *PendingPackage);
			FinishTravel(false);
			return;
		}
		if (TravelStage == ETravelStage::FadingOut || TravelStage == ETravelStage::OnTransitionMap)
		{
			TravelStageTimeLeft -= RealDeltaSeconds;
			if (TravelStageTimeLeft <= 0.f)
			{
				const FString Transition = UTDFlowSettings::Get()->TransitionMap.GetLongPackageName();
				const bool bUseTransition = TravelStage == ETravelStage::FadingOut && !Transition.IsEmpty()
					&& !TDFlow::SamePackage(Transition, PendingPackage) && FPackageName::DoesPackageExist(Transition);
				BeginLoad(bUseTransition ? Transition : PendingPackage);
			}
		}
	}
}

bool UTDGameInstance::TickRunTimer(float DeltaSeconds)
{
	TickFlow(DeltaSeconds); // real time: not paused, not dilated
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && !World->IsPaused())
	{
		// Real time, not dilated: director-cam slow motion shouldn't stretch the run clock.
		AdvanceRunTime(DeltaSeconds);
	}
#if !UE_BUILD_SHIPPING
	TickDebugMenu();
#endif
	return true;
}

void UTDGameInstance::TickDebugMenu()
{
	UWorld* World = GetWorld();
	APlayerController* PC = (World && World->IsGameWorld()) ? World->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->IsLocalController() || !PC->WasInputKeyJustPressed(EKeys::F10))
	{
		return;
	}
	if (!DebugMenu)
	{
		DebugMenu = MakeShared<FTDDebugMenu>();
	}
	DebugMenu->Toggle(World);
}

// ------------------------------------------------------------------------------------------------
// ITDGameFlow
// ------------------------------------------------------------------------------------------------

FTDProgress UTDGameInstance::GetProgress() const
{
	return Progress;
}

void UTDGameInstance::BroadcastPhase()
{
	if (UTDEventBus* Bus = GetBus())
	{
		Bus->BroadcastPhaseChanged(Progress.Phase);
	}
}

void UTDGameInstance::SetPhase(ETDGamePhase NewPhase)
{
	if (Progress.Phase == NewPhase)
	{
		return;
	}
	Progress.Phase = NewPhase;
	if (NewPhase == ETDGamePhase::Victory)
	{
		CancelPendingRetake(); // a win is a win
		EndScene();
		if (!IsMenuDestination(CurrentDestination))
		{
			VictoryTravelLeft = UTDFlowSettings::Get()->VictoryHoldSeconds; // then the victory screen
		}
	}
	SetRunTimerPaused(NewPhase == ETDGamePhase::Victory || IsMenuDestination(CurrentDestination));
	// Story state is never rolled back by a retake: patch the checkpoint and the slot.
	if (LastCheckpoint.bValid)
	{
		LastCheckpoint.Progress.Phase = NewPhase;
		WriteSaveSlot();
	}
	BroadcastPhase();
	BeginDominionSceneIfAssault();
}

const FName UTDGameInstance::DominionScene(TEXT("Dominion"));

void UTDGameInstance::BeginDominionSceneIfAssault()
{
	if (Progress.Phase != ETDGamePhase::DominionAssault || CurrentDestination != ETDDestination::Galaxy || CurrentSceneId == DominionScene)
	{
		return;
	}
	BeginScene(DominionScene); // slate "SCENE n · TAKE t"
	if (LastCheckpoint.bValid)
	{
		LastCheckpoint.SceneId = DominionScene; // a wipe restarts the assault as the next take
	}
}

void UTDGameInstance::CancelPendingRetake()
{
	if (!IsRetakePending())
	{
		return;
	}
	RetakeTimeLeft = -1.f;
	RetakeScene = NAME_None;
	UE_LOG(LogTriadDrift, Log, TEXT("Pending CUT cancelled (victory)."));
}

void UTDGameInstance::ReportWaveCleared(int32 WaveNumber)
{
	// 1-based wave numbers. Idempotent: the wave director may both call this and broadcast OnWaveCleared.
	// A clear that lands during the CUT delay is void: the scene is about to restart.
	if (WaveNumber <= 0 || WaveNumber <= Progress.WavesCleared || IsRetakePending())
	{
		return;
	}
	Progress.WavesCleared = WaveNumber;
	EndScene();
	bRunCommitted = true; // real progress
	TakeCheckpoint(TDFlow::ReasonWaveCleared);
	if (const ETDGamePhase* Next = UTDFlowSettings::Get()->PhaseAfterWaveCleared.Find(WaveNumber))
	{
		SetPhase(*Next); // patches the checkpoint just taken
	}
}

void UTDGameInstance::CompleteObjective(FName ObjectiveId, ETDAbilityId Reward, ETDShipId ForShip)
{
	if (ObjectiveId.IsNone() || IsRetakePending())
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
	bRunCommitted = true;
	TakeCheckpoint(TDFlow::ReasonObjective);
}

void UTDGameInstance::SetDominionState(bool bRevealed, bool bDefensesDown)
{
	if (IsRetakePending())
	{
		return; // like a wave clear inside the CUT delay: void (the scene is about to restart)
	}
	if (Progress.bDominionRevealed == bRevealed && Progress.bDominionDefensesDown == bDefensesDown)
	{
		return;
	}
	Progress.bDominionRevealed = bRevealed;
	Progress.bDominionDefensesDown = bDefensesDown;
	if (LastCheckpoint.bValid)
	{
		LastCheckpoint.Progress.bDominionRevealed = bRevealed;
		LastCheckpoint.Progress.bDominionDefensesDown = bDefensesDown;
		WriteSaveSlot();
	}
}

void UTDGameInstance::ReportPlayerDefeated()
{
	if (IsRetakePending() || IsTravelPending())
	{
		return; // already cutting / leaving
	}
	// Deaths are counted from OnActorKilled; this is the scene failing.
	RetakeScene = CurrentSceneId;
	const int32 NextTake = RetakeScene.IsNone() ? 0 : FMath::Max(1, GetTakeNumber(RetakeScene)) + 1;
	const float Delay = FMath::Max(0.f, UTDFlowSettings::Get()->RetakeDelaySeconds);
	if (UTDEventBus* Bus = GetBus())
	{
		Bus->BroadcastSceneCut(RetakeScene, NextTake, Delay); // Nick's "CUT!" card
	}
	if (Delay <= 0.f)
	{
		ExecuteRetake();
	}
	else
	{
		RetakeTimeLeft = Delay;
	}
}

void UTDGameInstance::RestartSceneNow()
{
	RetakeScene = CurrentSceneId;
	ExecuteRetake();
}

void UTDGameInstance::ExecuteRetake()
{
	RetakeTimeLeft = -1.f;
	const FName Scene = RetakeScene.IsNone() ? CurrentSceneId : RetakeScene;
	RetakeScene = NAME_None;
	const ETDGamePhase PhaseBefore = Progress.Phase;

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

	int32 Take = 0;
	if (!Scene.IsNone())
	{
		int32& Count = Progress.Takes.FindOrAdd(Scene);
		Count = FMath::Max(1, Count) + 1;
		Take = Count;
		SceneOrder.AddUnique(Scene);
	}
	CurrentSceneId = Scene;
	LastAnnouncedScene = Scene;
	LastAnnouncedTake = Take;

	WriteSaveSlot(); // keep takes/stats if the player quits now

	if (Progress.Phase != PhaseBefore)
	{
		BroadcastPhase();
	}
	OnCheckpointRestored.Broadcast(LastCheckpoint);
	OnSceneRestart.Broadcast(Scene, Take);
	// Always emitted, even with no open scene (SceneId None, take 0), so a CUT card always clears.
	if (UTDEventBus* Bus = GetBus())
	{
		Bus->BroadcastTakeStarted(Scene, Take);
	}
	ObjectiveForceLeft = 0.3f; // after the rebuild: the objective line comes back with the new take
	UE_LOG(LogTriadDrift, Log, TEXT("CUT! Scene %s restarts as take %d."), *Scene.ToString(), Take);
}

void UTDGameInstance::AddStat(FName StatName, int32 Amount)
{
	if (StatName.IsNone() || Amount == 0)
	{
		return;
	}
	// Resolve the field by name so new FTDRunStats fields work without touching Flow.
	const FProperty* Prop = FTDRunStats::StaticStruct()->FindPropertyByName(StatName);
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

void UTDGameInstance::ResetRunState()
{
	Progress = FTDProgress();
	LastCheckpoint = FTDCheckpoint();
	SceneOrder.Reset();
	LastObjective = FTDObjectiveLine();
	VictoryTravelLeft = -1.f;
	CurrentSceneId = NAME_None;
	LastAnnouncedScene = NAME_None;
	LastAnnouncedTake = 0;
	RetakeTimeLeft = -1.f;
	RetakeScene = NAME_None;
	bRunTimerPaused = false;
}

void UTDGameInstance::StartNewRun()
{
	ResetRunState();
	bRunCommitted = true;
}

bool UTDGameInstance::IsPristine() const
{
	return Progress.Phase == ETDGamePhase::Intro && Progress.WavesCleared == 0 && Progress.CompletedObjectives.Num() == 0
		&& Progress.UnlockedAbilities.Num() == 0 && Progress.Takes.Num() == 0 && !Progress.bDominionRevealed;
}

bool UTDGameInstance::HasContinue() const
{
	if (!bContinueCacheValid)
	{
		UTDSaveGame* Save = nullptr;
		// A won run never continues into the fight: there is nothing left to play.
		bContinueCache = ReadSaveSlot(Save) && Save && Save->Checkpoint.Progress.Phase != ETDGamePhase::Victory;
		bContinueCacheValid = true;
	}
	return bContinueCache;
}

bool UTDGameInstance::ContinueRun()
{
	UTDSaveGame* Save = nullptr;
	if (!ReadSaveSlot(Save) || (Save && Save->Checkpoint.Progress.Phase == ETDGamePhase::Victory) || !ApplySave(*Save))
	{
		return false;
	}
	bRunCommitted = true;
	BroadcastPhase();
	if (!TravelTo(LastCheckpoint.Destination, Progress.ReturnPoint))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("ContinueRun: progress restored, but travel to the saved destination failed."));
		return false;
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
	bContinueCacheValid = false;
}

void UTDGameInstance::ApplyStoryTransitionsOnEntry(ETDDestination Destination)
{
	if (Destination != ETDDestination::Galaxy)
	{
		return;
	}
	const UTDFlowSettings* Settings = UTDFlowSettings::Get();
	auto AllDone = [this](const TArray<FName>& Ids)
	{
		if (Ids.Num() == 0)
		{
			return false;
		}
		for (const FName& Id : Ids)
		{
			if (!Progress.CompletedObjectives.Contains(Id))
			{
				return false;
			}
		}
		return true;
	};
	// Back from a planet with its objectives done: the next wave block starts.
	if (Progress.Phase == ETDGamePhase::Verdance && AllDone(Settings->VerdanceObjectives))
	{
		SetPhase(ETDGamePhase::Waves3to4);
	}
	else if (Progress.Phase == ETDGamePhase::Cinder && AllDone(Settings->CinderObjectives))
	{
		SetPhase(ETDGamePhase::Wave5);
	}
}

void UTDGameInstance::NotifyLevelEntered(ETDDestination Destination)
{
	CurrentDestination = Destination;
	if (IsMenuDestination(Destination))
	{
		SetRunTimerPaused(true); // menus never checkpoint and never touch the save
		return;
	}
	SetRunTimerPaused(Progress.Phase == ETDGamePhase::Victory);
	const ETDGamePhase PhaseBeforeEntry = Progress.Phase;
	ApplyStoryTransitionsOnEntry(Destination);
	if (Progress.Phase == PhaseBeforeEntry)
	{
		BroadcastPhase(); // the new level's listeners (HUD, music state) learn the phase on every level entry
	}
	BeginDominionSceneIfAssault(); // arriving (or continuing) into the assault opens its scene
	LastObjective = FTDObjectiveLine(); // the old level's line is stale; on-foot objectives re-announce themselves
	ObjectiveForceLeft = 0.5f; // once the new level's HUD exists
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
	if (IsMenuDestination(CurrentDestination))
	{
		return;
	}
	LastCheckpoint.Progress = Progress;
	LastCheckpoint.Destination = CurrentDestination;
	LastCheckpoint.SceneId = CurrentSceneId;
	LastCheckpoint.Reason = Reason;
	LastCheckpoint.bValid = true;
	WriteSaveSlot();
}

FString UTDGameInstance::GetSaveSlotName() const
{
	if (!SaveSlotOverride.IsEmpty())
	{
		return SaveSlotOverride;
	}
	// Automation never touches the player's run: every test world that checkpoints would otherwise write (or a save
	// test delete) the real slot. Found when a full suite wiped a playtest's Verdance checkpoint (CONTINUE went to wave 1).
	return GIsAutomationTesting ? UTDFlowSettings::Get()->SaveSlotName + TEXT("_Automation") : UTDFlowSettings::Get()->SaveSlotName;
}

bool UTDGameInstance::WriteSaveSlot(bool bForce)
{
	if (!LastCheckpoint.bValid || IsMenuDestination(LastCheckpoint.Destination))
	{
		return false;
	}
	if (!bForce && (!UTDFlowSettings::Get()->bWriteSlotOnCheckpoint || !bRunCommitted || IsPristine()))
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
	Save->Difficulty = UTDDifficultySettings::GetCurrent();
	const bool bOk = UGameplayStatics::SaveGameToSlot(Save, GetSaveSlotName(), UTDFlowSettings::Get()->SaveUserIndex);
	if (!bOk)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Failed to write save slot '%s'."), *GetSaveSlotName());
	}
	bContinueCacheValid = false;
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
	// Read the bytes first: the engine's loader asserts on garbage, so a corrupt file must never reach it.
	TArray<uint8> Bytes;
	if (!UGameplayStatics::LoadDataFromSlot(Bytes, Slot, User) || !TDFlow::LooksLikeSaveGame(Bytes))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Save slot '%s' is corrupt or not a Triad Drift save; ignoring it."), *Slot);
		return false;
	}
	UTDSaveGame* Save = Cast<UTDSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!Save)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Save slot '%s' is corrupt or not a Triad Drift save; ignoring it."), *Slot);
		return false;
	}
	if (!Save->IsCompatible() || IsMenuDestination(Save->Checkpoint.Destination))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Save slot '%s' has version %d (expected %d) or no gameplay checkpoint; ignoring it."),
			*Slot, Save->Version, UTDSaveGame::CurrentVersion);
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
	RetakeTimeLeft = -1.f;
	RetakeScene = NAME_None;
	LastCheckpoint = Save.Checkpoint;
	Progress = Save.Checkpoint.Progress;
	SceneOrder = Save.SceneOrder;
	CurrentDestination = Save.Checkpoint.Destination;
	CurrentSceneId = Save.Checkpoint.SceneId;
	LastAnnouncedScene = NAME_None;
	LastAnnouncedTake = 0;
	UTDDifficultySettings::SetCurrent(Save.Difficulty);
	return true;
}

void UTDGameInstance::SetDifficulty(ETDDifficulty Difficulty)
{
	UTDDifficultySettings::SetCurrent(Difficulty);
	WriteSaveSlot(); // a committed run keeps it; a fresh boot / menu writes nothing (save safety)
}

ETDDifficulty UTDGameInstance::GetDifficulty() const
{
	return UTDDifficultySettings::GetCurrent();
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
	if (LastCheckpoint.bValid)
	{
		LastCheckpoint.Progress.UnlockedAbilities.Add(Ability, ForShip);
		WriteSaveSlot();
	}
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
	if (!bRunTimerPaused && !IsTravelPending() && DeltaSeconds > 0.f)
	{
		Progress.RunTimeSeconds += DeltaSeconds;
	}
}
