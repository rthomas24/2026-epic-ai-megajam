// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "GameFramework/Controller.h"

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TriadDrift/Waves/TDWaveEnemy.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Waves/TDWarpInMarker.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TriadDrift/Profile/TDProfileSubsystem.h"

#define LOCTEXT_NAMESPACE "TDWaveDirector"

namespace
{
	FTDWaveGroup TDWaveDir_Group(FName EnemyId, int32 Packs, int32 PerPack, ETDSpawnZone Zone, float Delay = 0.f)
	{
		FTDWaveGroup G;
		G.EnemyId = EnemyId;
		G.Packs = Packs;
		G.PerPack = PerPack;
		G.Zone = Zone;
		G.Delay = Delay;
		return G;
	}

	bool TDWaveDir_IsActorAlive(const AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return false;
		}
		const UTDHealthComponent* Health = UTDStatics::GetHealth(Actor);
		return !Health || Health->IsAlive();
	}
}

ATDWaveDirector::ATDWaveDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

ATDWaveDirector* ATDWaveDirector::Get(const UObject* WorldContext)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}
	// Cheap: directors register themselves (the HUD calls this every frame).
	for (const TWeakObjectPtr<ATDWaveDirector>& Weak : Instances())
	{
		if (ATDWaveDirector* D = Weak.Get())
		{
			if (D->GetWorld() == World && !D->IsActorBeingDestroyed()) { return D; }
		}
	}
	return nullptr;
}

TArray<TWeakObjectPtr<ATDWaveDirector>>& ATDWaveDirector::Instances()
{
	static TArray<TWeakObjectPtr<ATDWaveDirector>> List;
	return List;
}

// ------------------------------------------------------------------------------------- wave table

TArray<FTDWaveDef> ATDWaveDirector::MakeDefaultWaves()
{
	using namespace TDEnemyIds;
	TArray<FTDWaveDef> Out;

	// 1: 2 x 4 fighters, 0.2. Teaches switching.
	{
		FTDWaveDef W;
		W.SceneId = TEXT("Wave1");
		W.Title = LOCTEXT("Wave1", "First Contact");
		W.Intensity01 = 0.2f;
		W.Phase = ETDGamePhase::Waves1to2;
		W.StartTip = TDTipIds::Switching;
		W.Groups.Add(TDWaveDir_Group(ShardFighter, 1, 4, ETDSpawnZone::FlankLeft));
		W.Groups.Add(TDWaveDir_Group(ShardFighter, 1, 4, ETDSpawnZone::FlankRight, 2.f));
		Out.Add(W);
	}
	// 2: 3 x 5 fighters + 2 interceptors, 0.35. Verdance marker appears.
	{
		FTDWaveDef W;
		W.SceneId = TEXT("Wave2");
		W.Title = LOCTEXT("Wave2", "Hunters");
		W.Intensity01 = 0.35f;
		W.Phase = ETDGamePhase::Waves1to2;
		W.StartTip = TDTipIds::Drift;
		// ENG-27: the three packs arrive spaced (0 / 12 / 24 s), not all at once: same count, more readable fight.
		W.Groups.Add(TDWaveDir_Group(ShardFighter, 1, 5, ETDSpawnZone::AnyFlank));
		W.Groups.Add(TDWaveDir_Group(HookInterceptor, 1, 1, ETDSpawnZone::Behind, 8.f));
		W.Groups.Add(TDWaveDir_Group(ShardFighter, 1, 5, ETDSpawnZone::AnyFlank, 12.f));
		W.Groups.Add(TDWaveDir_Group(ShardFighter, 1, 5, ETDSpawnZone::AnyFlank, 24.f));
		// Storyline 13 phase 04: the heavy contact ("That one's bigger. Focus it together!"), then the withdrawal.
		W.Groups.Add(TDWaveDir_Group(WallGunship, 1, 1, ETDSpawnZone::Behind, 20.f));
		W.bTutorialWithdrawal = true;
		W.bRevealsMarker = true;
		W.MarkerRevealed = ETDDestination::Verdance;
		Out.Add(W);
	}
	// 3: 2 gunships + 3 x 5 fighters, 0.55.
	{
		FTDWaveDef W;
		W.SceneId = TEXT("Wave3");
		W.Title = LOCTEXT("Wave3", "The Wall");
		W.Intensity01 = 0.55f;
		W.Phase = ETDGamePhase::Waves3to4;
		W.StartTip = TDTipIds::Gunships;
		W.Groups.Add(TDWaveDir_Group(WallGunship, 2, 1, ETDSpawnZone::FlankRight));
		W.Groups.Add(TDWaveDir_Group(ShardFighter, 3, 5, ETDSpawnZone::AnyFlank, 2.f));
		Out.Add(W);
	}
	// 4: 2 gunships, 2 bombers, 4 interceptors, fighters, 0.7. Cinder marker appears.
	{
		FTDWaveDef W;
		W.SceneId = TEXT("Wave4");
		W.Title = LOCTEXT("Wave4", "Minefield");
		W.Intensity01 = 0.7f;
		W.Phase = ETDGamePhase::Waves3to4;
		W.Groups.Add(TDWaveDir_Group(WallGunship, 2, 1, ETDSpawnZone::FlankLeft));
		W.Groups.Add(TDWaveDir_Group(MaulBomber, 2, 1, ETDSpawnZone::AnyFlank, 1.f));
		W.Groups.Add(TDWaveDir_Group(HookInterceptor, 3, 1, ETDSpawnZone::Behind, 5.f));	// ENG-27: 4 -> 3
		W.Groups.Add(TDWaveDir_Group(ShardFighter, 2, 4, ETDSpawnZone::AnyFlank, 3.f));
		W.bRevealsMarker = true;
		W.MarkerRevealed = ETDDestination::Cinder;
		Out.Add(W);
	}
	// 5: "Dominion's Guard": 2 carriers + escorts + bombers, 0.9. Dominion reveal.
	{
		FTDWaveDef W;
		W.SceneId = TEXT("Wave5");
		W.Title = LOCTEXT("Wave5", "Dominion's Guard");
		W.Intensity01 = 0.9f;
		W.Phase = ETDGamePhase::Wave5;
		W.StartTip = TDTipIds::Carriers;
		W.Groups.Add(TDWaveDir_Group(HiveCarrier, 2, 1, ETDSpawnZone::Behind));
		W.Groups.Add(TDWaveDir_Group(ShardFighter, 2, 4, ETDSpawnZone::AnyFlank, 1.f));
		W.Groups.Add(TDWaveDir_Group(HookInterceptor, 2, 1, ETDSpawnZone::Behind, 3.f));
		W.Groups.Add(TDWaveDir_Group(MaulBomber, 2, 1, ETDSpawnZone::AnyFlank, 6.f));
		W.bRevealsDominion = true;
		Out.Add(W);
	}
	return Out;
}

void ATDWaveDirector::SetWaveTable(const TArray<FTDWaveDef>& InWaves)
{
	Waves = InWaves;
	bWaveTableLoaded = true;
}

void ATDWaveDirector::LoadWaveTable()
{
	if (bWaveTableLoaded)
	{
		return;
	}
	bWaveTableLoaded = true;

	Waves.Reset();
	if (WaveTableOverride)
	{
		if (WaveTableOverride->GetRowStruct() == FTDWaveDef::StaticStruct())
		{
			for (const FName& RowName : WaveTableOverride->GetRowNames())
			{
				if (const FTDWaveDef* Row = WaveTableOverride->FindRow<FTDWaveDef>(RowName, TEXT("TDWaveDirector")))
				{
					Waves.Add(*Row);
				}
			}
		}
		else
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("ATDWaveDirector: WaveTableOverride '%s' is not an FTDWaveDef table; using defaults."), *WaveTableOverride->GetName());
		}
	}
	if (Waves.Num() == 0)
	{
		Waves = MakeDefaultWaves();
	}
	else if (Waves.Num() != MakeDefaultWaves().Num())
	{
		// The flow maps "phase after wave N" for waves 2, 4 and 5 (UTDFlowSettings::PhaseAfterWaveCleared).
		UE_LOG(LogTriadDrift, Warning, TEXT("ATDWaveDirector: WaveTableOverride has %d waves, the flow expects %d; check PhaseAfterWaveCleared."),
			Waves.Num(), MakeDefaultWaves().Num());
	}
}

const FTDWaveDef* ATDWaveDirector::GetWaveDef(int32 Index) const
{
	return Waves.IsValidIndex(Index) ? &Waves[Index] : nullptr;
}

FName ATDWaveDirector::GetSceneId(int32 Index) const
{
	const FTDWaveDef* Def = GetWaveDef(Index);
	if (Def && !Def->SceneId.IsNone())
	{
		return Def->SceneId;
	}
	return Index >= 0 ? FName(*FString::Printf(TEXT("Wave%d"), Index + 1)) : NAME_None;
}

// ---------------------------------------------------------------------------------------- lifecycle

void ATDWaveDirector::BeginPlay()
{
	Super::BeginPlay();
	Instances().RemoveAll([](const TWeakObjectPtr<ATDWaveDirector>& W) { return !W.IsValid(); });
	Instances().AddUnique(this);
	Rng.Initialize(RandomSeed);
	LoadWaveTable();
	if (!TravelFX)
	{
		TravelFX = UTDStatics::LoadPresentation<UTDTravelFX>(TEXT("DA_Travel_FX"));	// Nick's EnemyWarpIn cue
	}

	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnTakeStarted.AddUniqueDynamic(this, &ATDWaveDirector::HandleTakeStarted);
		Bus->OnSceneCut.AddUniqueDynamic(this, &ATDWaveDirector::HandleSceneCut);
	}

	if (bAutoStartFromProgress)
	{
		StartFromProgress();
	}
}

void ATDWaveDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnTakeStarted.RemoveDynamic(this, &ATDWaveDirector::HandleTakeStarted);
		Bus->OnSceneCut.RemoveDynamic(this, &ATDWaveDirector::HandleSceneCut);
	}
	for (const TWeakObjectPtr<AActor>& Weak : Living)
	{
		if (AActor* A = Weak.Get())
		{
			A->OnDestroyed.RemoveDynamic(this, &ATDWaveDirector::HandleEnemyDestroyed);
			if (UTDHealthComponent* H = UTDStatics::GetHealth(A))
			{
				H->OnDeath.RemoveDynamic(this, &ATDWaveDirector::HandleEnemyDeath);
			}
		}
	}
	Living.Reset();
	Pending.Reset();
	SetWaveHyperdriveLock(false);	// never leave the player locked in
	Instances().Remove(this);
	Super::EndPlay(EndPlayReason);
}

void ATDWaveDirector::SetState(ETDWaveState NewState, float Duration)
{
	StateRemaining = Duration;
	if (State != NewState)
	{
		State = NewState;
		OnStateChanged.Broadcast(NewState);
		if (ATDGalaxyLayout* Layout = ATDGalaxyLayout::Get(this))
		{
			Layout->UpdateGatePhases(); // the planet orbit gates close the moment a wave starts (MAJOR B)
		}
	}
}

bool ATDWaveDirector::PhaseMatches(ETDGamePhase WavePhase, ETDGamePhase ProgressPhase) const
{
	if (WavePhase == ProgressPhase)
	{
		return true;
	}
	// A fresh run sits in Intro until the first wave starts.
	return ProgressPhase == ETDGamePhase::Intro && WavePhase == ETDGamePhase::Waves1to2;
}

void ATDWaveDirector::StartFromProgress()
{
	LoadWaveTable();
	if (State == ETDWaveState::WarpingIn || State == ETDWaveState::Active || State == ETDWaveState::Breather)
	{
		return;	// already running
	}

	FTDProgress Progress;
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		Progress = Flow->GetProgress();
	}

	// After a Continue in a planet phase, re-announce that planet's marker (the HUD lost it).
	if (Progress.Phase == ETDGamePhase::Verdance || Progress.Phase == ETDGamePhase::Cinder)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(this))
		{
			Bus->BroadcastMarkerRevealed(Progress.Phase == ETDGamePhase::Verdance ? ETDDestination::Verdance : ETDDestination::Cinder);
		}
	}

	const int32 Next = FMath::Max(0, Progress.WavesCleared);	// WavesCleared waves done -> next 0-based index
	if (Next >= Waves.Num())
	{
		CurrentWave = Waves.Num() - 1;
		SetState(ETDWaveState::AllComplete);
		return;
	}
	if (!PhaseMatches(Waves[Next].Phase, Progress.Phase))
	{
		SetState(ETDWaveState::Idle);	// e.g. on a planet, or waiting for the flow to change phase
		return;
	}
	if (WaveNeedsCombatZone(Next) && !IsPlayerInCombatZone())
	{
		AwaitCombatZone(Next);
		return;
	}
	StartWave(Next);
}

bool ATDWaveDirector::WaveNeedsCombatZone(int32 Index) const
{
	return Waves.IsValidIndex(Index) && (Waves[Index].Phase == ETDGamePhase::Waves3to4 || Waves[Index].Phase == ETDGamePhase::Wave5);
}

bool ATDWaveDirector::IsPlayerInCombatZone() const
{
	const ATDGalaxyLayout* Layout = ATDGalaxyLayout::Get(this);
	const AActor* Player = SpawnAnchorOverride ? SpawnAnchorOverride.Get() : UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Layout || !Player)
	{
		return true;
	}
	const FTDGalaxyLayoutData& L = Layout->GetLayout();
	const FVector Centre = Layout->GetActorTransform().TransformPosition(L.CombatZoneCenter);
	return FVector::DistSquared(Player->GetActorLocation(), Centre) <= FMath::Square(L.CombatZoneRadius);
}

void ATDWaveDirector::AwaitCombatZone(int32 Index)
{
	SetState(ETDWaveState::Idle);
	SetWaveHyperdriveLock(false);
	if (!bAwaitingCombatZone)
	{
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Waves: wave %d waits for the squad to return to the combat zone"), Index + 1);
	}
	bAwaitingCombatZone = true;
	CombatZoneCheckTimer = 0.25f;
	if (!CombatZoneMarker)
	{
		if (const ATDGalaxyLayout* Layout = ATDGalaxyLayout::Get(this))
		{
			// An invisible point for the HUD's direction marker (the flow tags it, as it does planets and generators).
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags |= RF_Transient;
			const FVector Centre = Layout->GetActorTransform().TransformPosition(Layout->GetLayout().CombatZoneCenter);
			if (AActor* Marker = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Centre), Params))
			{
				USceneComponent* Root = NewObject<USceneComponent>(Marker, TEXT("Root"));
				Marker->SetRootComponent(Root);
				Root->RegisterComponent();
				Marker->SetActorLocation(Centre);
				CombatZoneMarker = Marker;
			}
		}
	}
}

void ATDWaveDirector::StopAwaitingCombatZone()
{
	bAwaitingCombatZone = false;
	if (CombatZoneMarker)
	{
		CombatZoneMarker->Destroy();
		CombatZoneMarker = nullptr;
	}
}

void ATDWaveDirector::StartWave(int32 Index)
{
	LoadWaveTable();
	if (!Waves.IsValidIndex(Index))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("ATDWaveDirector::StartWave: index %d out of range (0..%d)"), Index, Waves.Num() - 1);
		return;
	}
	StopAwaitingCombatZone();
	ClearEnemies();
	if (CurrentWave != Index)
	{
		CurrentTake = 1;
	}
	BeginWave(Index);
}

void ATDWaveDirector::RestartCurrentWave()
{
	if (!Waves.IsValidIndex(CurrentWave))
	{
		return;
	}
	ClearEnemies();
	++CurrentTake;
	BeginWave(CurrentWave);
}

void ATDWaveDirector::BeginWave(int32 Index)
{
	const FTDWaveDef& Wave = Waves[Index];
	CurrentWave = Index;
	ResetTutorial();	// a new wave or a retake starts the tutorial ending over
	// The tutorial opening: wave 1 only (a retake of wave 1 gets it again).
	bOpeningGrace = Index == 0 && OpeningGraceSeconds > 0.f;
	bOpeningFloor = bOpeningGrace;
	OpeningGraceLeft = OpeningGraceSeconds;
	OpeningGraceShotBaseline = PlayerShotsFired();

	// ENG-27 difficulty scales pack sizes (and small-ship pack counts); big ships stay as designed.
	const float SizeScale = UTDDifficultySettings::GetScale().WaveSize;
	EnemiesTotal = 0;
	Pending.Reset();
	for (FTDWaveGroup G : Wave.Groups)
	{
		if (!FMath::IsNearlyEqual(SizeScale, 1.f))
		{
			const FTDEnemySpec& GroupSpec = UTDEnemyLibrary::Get(G.EnemyId);
			if (G.PerPack > 1)
			{
				G.PerPack = UTDDifficultySettings::ScaleCount(G.PerPack, SizeScale);
			}
			else if (GroupSpec.IsValid() && GroupSpec.Size == ETDEnemySize::Small)
			{
				G.Packs = UTDDifficultySettings::ScaleCount(G.Packs, SizeScale);
			}
		}
		EnemiesTotal += G.Total();
		if (G.EnemyId.IsNone() || G.Total() <= 0)
		{
			continue;
		}
		FPendingGroup P;
		P.Group = G;
		P.Remaining = G.Delay + WarpInSeconds;
		// The spawn point is picked when the warp-in is announced (relative to where the player is
		// THEN), not now: delayed groups must not appear where the player was seconds ago.
		Pending.Add(P);
	}

	SetState(ETDWaveState::WarpingIn);
	SetWaveHyperdriveLock(true);	// no jumping out of a fight (breathers and planet phases are free)

	// Sync the take with the flow BEFORE broadcasting: the flow answers OnWaveStarted with
	// OnTakeStarted, and a take number above ours would otherwise restart this wave re-entrantly.
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		CurrentTake = FMath::Max(CurrentTake, Flow->GetProgress().Takes.FindRef(GetSceneId(Index)));
	}

	const int32 WaveNumber = Index + 1;
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->BroadcastWaveStarted(WaveNumber, Wave.Intensity01);
	}
	AnnounceScene(Wave);
	FireTips(Wave);

	// A wave with nothing to spawn clears immediately (data error, but never soft-lock).
	if (Pending.Num() == 0)
	{
		FinishWave();
	}
}

void ATDWaveDirector::AnnounceScene(const FTDWaveDef& Wave)
{
	// With T5's game instance present, it owns takes: it hears OnWaveStarted, calls BeginScene and
	// fires OnTakeStarted. Without it (tests, bare worlds) we announce the take ourselves.
	if (UTDStatics::GetGameFlow(this))
	{
		return;
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->BroadcastTakeStarted(GetSceneId(CurrentWave), CurrentTake);
	}
}

void ATDWaveDirector::HandleSceneCut(FName SceneId, int32 /*NextTakeNumber*/, float /*DelaySeconds*/)
{
	bCutPending = CurrentWave != INDEX_NONE && SceneId == GetSceneId(CurrentWave)
		&& (State == ETDWaveState::WarpingIn || State == ETDWaveState::Active);
}

void ATDWaveDirector::HandleTakeStarted(FName SceneId, int32 TakeNumber)
{
	bCutPending = false;
	if (SceneId != GetSceneId(CurrentWave))
	{
		return;
	}
	const bool bRunning = State == ETDWaveState::WarpingIn || State == ETDWaveState::Active;
	// The last enemy can die during the CUT delay (an asteroid, a mine): the wave then finishes, the flow voids
	// the clear because the scene is restarting, and the retake arrives here with the wave already over. If
	// the flow doesn't count this wave as cleared, it is still being played: rebuild it (no soft-lock).
	bool bClearVoided = false;
	if (!bRunning && CurrentWave != INDEX_NONE)
	{
		if (const ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
		{
			bClearVoided = Flow->GetProgress().WavesCleared < CurrentWave + 1;
		}
	}
	if (TakeNumber > CurrentTake && (bRunning || bClearVoided))
	{
		// "CUT!" -> next take: rebuild the scene.
		CurrentTake = TakeNumber;
		ClearEnemies();
		BeginWave(CurrentWave);
		return;
	}
	CurrentTake = FMath::Max(CurrentTake, TakeNumber);
	// A retake that doesn't run a wave here must not leave our lock behind.
	SetWaveHyperdriveLock(bRunning);
}

void ATDWaveDirector::FireTips(const FTDWaveDef& Wave)
{
	if (Wave.StartTip.IsNone())
	{
		return;
	}
	if (Wave.StartTip == TDTipIds::Switching)
	{
		if (bSwitchTipShown) { return; }
		bSwitchTipShown = true;
	}
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		Flow->ShowTip(Wave.StartTip);
	}
	else if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->BroadcastTipRequested(Wave.StartTip);
	}
}

// ----------------------------------------------------------------------------------------- spawning

const FName ATDWaveDirector::ShowcaseTag(TEXT("TD.Showcase"));

ATDEnemyShip* ATDWaveDirector::SpawnShowcaseShip(UWorld* World, FName EnemyId, const FTransform& Where)
{
	if (!World || !UTDEnemyLibrary::HasEnemy(EnemyId))
	{
		return nullptr;
	}
	ATDEnemyShip* E = World->SpawnActorDeferred<ATDEnemyShip>(ATDEnemyShip::StaticClass(), Where, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!E)
	{
		return nullptr;
	}
	E->AutoPossessAI = EAutoPossessAI::Disabled;	// no pilot: it never flies, fires or launches
	E->AIControllerClass = nullptr;
	E->Tags.Add(ShowcaseTag);
	E->FinishSpawning(Where);
	E->InitFromSpec(EnemyId);
	auto LockDown = [](AActor* A)
	{
		if (!A) { return; }
		A->Tags.AddUnique(ShowcaseTag);
		if (UTDHealthComponent* H = A->FindComponentByClass<UTDHealthComponent>())
		{
			H->SetInvulnerable(true);
			H->SetTargetable(false);
		}
		if (UTDWeaponComponent* Wp = A->FindComponentByClass<UTDWeaponComponent>())
		{
			Wp->SetWeaponsBlocked(ShowcaseTag, true);
		}
		A->SetActorEnableCollision(false);
	};
	LockDown(E);
	TArray<AActor*> Parts;
	E->GetAttachedActors(Parts, true, true);
	for (AActor* P : Parts) { LockDown(P); }
	if (UTDShipMovementComponent* Move = E->GetShipMovement())
	{
		Move->SetComponentTickEnabled(false);	// stays where the shot puts it
	}
	return E;
}

void ATDWaveDirector::PlayShowcaseWarpIn(UWorld* World, const FVector& Location, float Size)
{
	if (!World)
	{
		return;
	}
	UTDTravelFX* FX = UTDStatics::LoadPresentation<UTDTravelFX>(TEXT("DA_Travel_FX"));
	FRotator Facing = FRotator::ZeroRotator;
	FVector LookAt = Location + FVector::ForwardVector;
	if (APlayerController* PC = World->GetFirstPlayerController(); PC && PC->PlayerCameraManager)
	{
		LookAt = PC->PlayerCameraManager->GetCameraLocation();
		Facing = (LookAt - Location).Rotation();
	}
	if (FX)
	{
		UTDStatics::PlayCue(World, FX->EnemyWarpIn, Location, Facing);
	}
	if (!FX || !FTDCombatPlaceholders::HasVisualArt(FX->EnemyWarpIn))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ATDWarpInMarker* M = World->SpawnActor<ATDWarpInMarker>(ATDWarpInMarker::StaticClass(), Location, FRotator::ZeroRotator, Params))
		{
			M->Tags.Add(ShowcaseTag);
			M->Begin(GetDefault<ATDWaveDirector>()->WarpInSeconds, FMath::Max(500.f, Size), LookAt);
		}
	}
}

void ATDWaveDirector::SpawnWarpInMarker(const FVector& Location, float EnemyRadius)
{
	UWorld* World = GetWorld();
	if (!World || !bWarpInPlaceholder)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATDWarpInMarker* M = World->SpawnActor<ATDWarpInMarker>(ATDWarpInMarker::StaticClass(), Location, FRotator::ZeroRotator, Params))
	{
		const AActor* Anchor = GetSpawnAnchor();
		M->Begin(WarpInSeconds, FMath::Max(2500.f, EnemyRadius * 6.f), Anchor ? Anchor->GetActorLocation() : Location + FVector::ForwardVector);
	}
}

int32 ATDWaveDirector::PlayerShotsFired() const
{
	const ATDSpacecraft* Craft = Cast<ATDSpacecraft>(UGameplayStatics::GetPlayerPawn(this, 0));
	const UTDWeaponComponent* Wp = Craft ? Craft->GetWeapons() : nullptr;
	return Wp ? Wp->GetSlotState(ETDWeaponSlot::Primary).ShotsFired + Wp->GetSlotState(ETDWeaponSlot::Secondary).ShotsFired : 0;
}

void ATDWaveDirector::TickOpeningGrace(float DeltaSeconds)
{
	OpeningGraceLeft -= DeltaSeconds;
	const ATDSpacecraft* Player = Cast<ATDSpacecraft>(UGameplayStatics::GetPlayerPawn(this, 0));
	const bool bPlayerActed = Player && (Player->IsBoosting() || PlayerShotsFired() > OpeningGraceShotBaseline);
	const float Elapsed = OpeningGraceSeconds - OpeningGraceLeft;
	if (bPlayerActed) { bOpeningFloor = false; }	// the floor ends the moment the player fires or boosts
	const bool bWaveOver = CurrentWave != 0 || (State != ETDWaveState::WarpingIn && State != ETDWaveState::Active);
	const bool bOver = bWaveOver || OpeningGraceLeft <= 0.f || (!bOpeningFloor && Elapsed >= OpeningGraceMinDamageSeconds);
	if (bOver)
	{
		EndOpeningGrace();
		return;
	}
	if (!bOpeningFloor)
	{
		return;	// reduced damage only, until OpeningGraceMinDamageSeconds
	}
	// The floor: no squad ship below OpeningGraceHullFloor01 while the player hasn't acted yet.
	if (const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		TArray<AActor*> Squad;
		Registry->GetActorsOfTeam(ETDTeam::Player, Squad, true);
		for (AActor* A : Squad)
		{
			UTDHealthComponent* H = A && A->IsA<ATDSpacecraft>() ? UTDStatics::GetHealth(A) : nullptr;
			if (H && H->IsAlive() && H->GetHull() < H->MaxHull * OpeningGraceHullFloor01)
			{
				H->RepairHull(H->MaxHull * OpeningGraceHullFloor01 - H->GetHull());
			}
		}
	}
}

void ATDWaveDirector::EndOpeningGrace()
{
	bOpeningGrace = false;
	bOpeningFloor = false;
	for (const TWeakObjectPtr<AActor>& Weak : Living)
	{
		if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Weak.Get()))
		{
			if (UTDWeaponComponent* Wp = Craft->GetWeapons()) { Wp->SetOutgoingDamageMultiplier(TEXT("OpeningGrace"), 1.f); }
		}
	}
}

AActor* ATDWaveDirector::GetSpawnAnchor() const
{
	if (SpawnAnchorOverride)
	{
		return SpawnAnchorOverride;
	}
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return Pawn;
	}
	return const_cast<ATDWaveDirector*>(this);
}

void ATDWaveDirector::PickSpawnPoint(ETDSpawnZone Zone, bool bLarge, FVector& OutLocation, FRotator& OutFacing)
{
	const AActor* Anchor = GetSpawnAnchor();
	// Lead the anchor by the warp-in time: the group appears around where the player WILL be.
	const FVector Origin = Anchor->GetActorLocation() + Anchor->GetVelocity() * WarpInSeconds;
	const FVector Fwd = Anchor->GetActorForwardVector();
	const FVector Right = Anchor->GetActorRightVector();
	const FVector Up = Anchor->GetActorUpVector();

	if (Zone == ETDSpawnZone::AnyFlank)
	{
		const ETDSpawnZone Options[] = { ETDSpawnZone::Behind, ETDSpawnZone::FlankLeft, ETDSpawnZone::FlankRight };
		Zone = Options[Rng.RandHelper(UE_ARRAY_COUNT(Options))];
	}

	FVector Dir;
	switch (Zone)
	{
	case ETDSpawnZone::Behind:     Dir = -Fwd + Up * 0.15f; break;
	case ETDSpawnZone::FlankLeft:  Dir = -Right * 0.85f - Fwd * 0.35f; break;
	case ETDSpawnZone::FlankRight: Dir = Right * 0.85f - Fwd * 0.35f; break;
	case ETDSpawnZone::Above:      Dir = Up - Fwd * 0.3f; break;
	case ETDSpawnZone::Below:      Dir = -Up - Fwd * 0.3f; break;
	case ETDSpawnZone::Ahead:      Dir = Fwd; break;
	default:                       Dir = -Fwd; break;
	}
	Dir = Dir.GetSafeNormal(UE_SMALL_NUMBER, -Fwd);

	const float Distance = SpawnDistance * (bLarge ? LargeShipSpawnDistanceScale : 1.f);
	const FVector Jitter = Rng.GetUnitVector() * (Rng.FRand() * SpawnJitter);
	OutLocation = Origin + Dir * Distance + Jitter;
	OutFacing = (Origin - OutLocation).Rotation();
}

AActor* ATDWaveDirector::SpawnEnemy(FName EnemyId, const FTransform& Transform)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AActor* Spawned = nullptr;
	if (SpawnFactory)
	{
		Spawned = SpawnFactory(World, EnemyId, Transform);
	}
	else
	{
		UClass* Class = EnemyClass.Get();
		if (!Class)
		{
			// The enemy ship lives in Enemies/; looked up by name so Waves/ has no hard dependency.
			Class = FindObject<UClass>(nullptr, TEXT("/Script/unrealGameJam.TDEnemyShip"));
		}
		if (!Class)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("ATDWaveDirector: no enemy class to spawn '%s'"), *EnemyId.ToString());
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Spawned = World->SpawnActor<AActor>(Class, Transform, Params);
	}

	// ENG-27 attack slots for this wave (how many pilots may press the same squad ship at once).
	if (bOpeningGrace)
	{
		if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Spawned))
		{
			if (UTDWeaponComponent* Wp = Craft->GetWeapons()) { Wp->SetOutgoingDamageMultiplier(TEXT("OpeningGrace"), OpeningGraceDamage); }
		}
	}
	if (const APawn* Pawn = Cast<APawn>(Spawned))
	{
		if (ATDEnemyAIController* AI = Cast<ATDEnemyAIController>(Pawn->GetController()))
		{
			AI->AttackSlotsPerTarget = AttackSlotsByWave.IsValidIndex(CurrentWave) ? AttackSlotsByWave[CurrentWave] : 0;
		}
	}
	if (ITDWaveEnemy* WaveEnemy = Cast<ITDWaveEnemy>(Spawned))
	{
		WaveEnemy->InitWaveEnemy(EnemyId);
	}
	return Spawned;
}

void ATDWaveDirector::SpawnGroup(const FPendingGroup& P)
{
	const FTDWaveGroup& G = P.Group;
	const FRotationMatrix Basis(P.Facing);
	const FVector Right = Basis.GetUnitAxis(EAxis::Y);
	const FVector Back = -Basis.GetUnitAxis(EAxis::X);

	for (int32 PackIdx = 0; PackIdx < G.Packs; ++PackIdx)
	{
		// Packs spread side by side.
		const float PackSide = (PackIdx - (G.Packs - 1) * 0.5f) * FormationSpacing * 4.f;
		const FVector PackOrigin = P.Location + Right * PackSide;

		AActor* Leader = nullptr;
		for (int32 Slot = 0; Slot < G.PerPack; ++Slot)
		{
			// V formation: leader at the tip, then alternating left/right, each row further back.
			const int32 Row = (Slot + 1) / 2;
			const float Side = (Slot == 0) ? 0.f : ((Slot % 2) ? -1.f : 1.f);
			const FVector LocalOffset(-Row * FormationSpacing, Side * Row * FormationSpacing, 0.f);
			const FVector WorldLoc = PackOrigin + Back * Row * FormationSpacing + Right * Side * Row * FormationSpacing;

			AActor* Enemy = SpawnEnemy(G.EnemyId, FTransform(P.Facing, WorldLoc));
			if (!Enemy)
			{
				--EnemiesTotal;	// keep the HUD honest if a spawn fails
				continue;
			}
			// The tutorial's heavy contact: the gunship of the withdrawal wave (by the group's id, so stand-ins count).
			if (G.EnemyId == TDEnemyIds::WallGunship && !bHeavySeen && IsTutorialWithdrawalWave())
			{
				HeavyContact = Enemy;
				bHeavySeen = true;
				HeavyTime = 0.f;
				UTDStorySubsystem::Fire(this, TEXT("TB_Wren_Bigger"));
			}

			if (ITDWaveEnemy* WaveEnemy = Cast<ITDWaveEnemy>(Enemy))
			{
				if (Leader)
				{
					WaveEnemy->JoinFormation(Leader, LocalOffset);
				}
				WaveEnemy->PlayWarpIn();
			}
			if (!Leader)
			{
				Leader = Enemy;
			}

			// Spawned enemies are already in EnemiesTotal; RegisterEnemy would count them twice.
			Living.Add(Enemy);
			Enemy->OnDestroyed.AddUniqueDynamic(this, &ATDWaveDirector::HandleEnemyDestroyed);
			if (UTDHealthComponent* H = UTDStatics::GetHealth(Enemy))
			{
				H->OnDeath.AddUniqueDynamic(this, &ATDWaveDirector::HandleEnemyDeath);
			}
			OnEnemySpawned.Broadcast(Enemy);
		}
	}
	EnemiesTotal = FMath::Max(EnemiesTotal, 0);
}

void ATDWaveDirector::RegisterEnemy(AActor* Enemy)
{
	if (!IsValid(Enemy) || Living.Contains(Enemy))
	{
		return;
	}
	if (State != ETDWaveState::WarpingIn && State != ETDWaveState::Active)
	{
		return;	// stragglers outside a wave don't hold anything up
	}
	Living.Add(Enemy);
	++EnemiesTotal;

	Enemy->OnDestroyed.AddUniqueDynamic(this, &ATDWaveDirector::HandleEnemyDestroyed);
	if (UTDHealthComponent* H = UTDStatics::GetHealth(Enemy))
	{
		H->OnDeath.AddUniqueDynamic(this, &ATDWaveDirector::HandleEnemyDeath);
	}
	OnEnemySpawned.Broadcast(Enemy);
}

// ------------------------------------------------------------------------------------------ deaths

void ATDWaveDirector::HandleEnemyDeath(UTDHealthComponent* Health, const FTDDamageInfo& /*KillingBlow*/)
{
	if (Health)
	{
		Health->OnDeath.RemoveDynamic(this, &ATDWaveDirector::HandleEnemyDeath);
		Living.Remove(Health->GetOwner());
	}
	CheckWaveCleared();
}

void ATDWaveDirector::HandleEnemyDestroyed(AActor* Actor)
{
	Living.Remove(Actor);
	CheckWaveCleared();
}

int32 ATDWaveDirector::GetEnemiesAlive() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<AActor>& Weak : Living)
	{
		if (TDWaveDir_IsActorAlive(Weak.Get())) { ++N; }
	}
	return N;
}

TArray<AActor*> ATDWaveDirector::GetLivingEnemies() const
{
	TArray<AActor*> Out;
	for (const TWeakObjectPtr<AActor>& Weak : Living)
	{
		if (AActor* A = Weak.Get())
		{
			if (TDWaveDir_IsActorAlive(A)) { Out.Add(A); }
		}
	}
	return Out;
}

void ATDWaveDirector::CheckWaveCleared()
{
	if (State != ETDWaveState::WarpingIn && State != ETDWaveState::Active)
	{
		return;
	}
	if (Pending.Num() > 0)
	{
		return;
	}
	Living.RemoveAll([](const TWeakObjectPtr<AActor>& W) { return !TDWaveDir_IsActorAlive(W.Get()); });
	// A "CUT!" is pending for this wave (the squad wiped): the last kill during the delay doesn't clear it. No
	// broadcasts, no checkpoint, no marker: the retake rebuilds the wave (review: a voided clear still ran
	// FinishWave's side effects).
	if (Living.Num() == 0 && !bCutPending)
	{
		if (IsTutorialWithdrawalWave() && TutorialPhase != ETDTutorialPhase::Escaped)
		{
			// Every enemy down is not the end of the tutorial (Nicholas): Command orders the withdrawal instead.
			if (TutorialPhase == ETDTutorialPhase::Battle) { BeginWithdrawal(); }
			return;
		}
		FinishWave();
	}
}

void ATDWaveDirector::FinishWave()
{
	if (!Waves.IsValidIndex(CurrentWave))
	{
		return;
	}
	const FTDWaveDef& Wave = Waves[CurrentWave];
	const int32 WaveNumber = CurrentWave + 1;
	SetWaveHyperdriveLock(false);

	// Decide the next state first so listeners of OnWaveCleared see a consistent director.
	const int32 NextIndex = CurrentWave + 1;
	if (!Waves.IsValidIndex(NextIndex))
	{
		SetState(ETDWaveState::AllComplete);
	}
	else if (Waves[NextIndex].Phase == Wave.Phase)
	{
		SetState(ETDWaveState::Breather, FMath::Max(BreatherSeconds, 2.f));	// never shorter than the CUT delay
	}
	else
	{
		SetState(ETDWaveState::PhaseComplete);
	}

	UTDEventBus* Bus = UTDEventBus::Get(this);
	ITDGameFlow* Flow = UTDStatics::GetGameFlow(this);

	if (Bus)
	{
		Bus->BroadcastWaveCleared(WaveNumber);
	}
	if (Flow)
	{
		Flow->ReportWaveCleared(WaveNumber);	// idempotent on T5's side (also bound to the bus)
	}

	if (Wave.bRevealsMarker && Wave.MarkerRevealed != ETDDestination::Galaxy)
	{
		if (Bus)
		{
			Bus->BroadcastMarkerRevealed(Wave.MarkerRevealed);
		}
		// First planet marker = the player now has to hyperdrive there.
		if (Wave.MarkerRevealed == ETDDestination::Verdance)
		{
			if (Flow) { Flow->ShowTip(TDTipIds::Hyperdrive); }
			else if (Bus) { Bus->BroadcastTipRequested(TDTipIds::Hyperdrive); }
		}
	}
	// Story rule (Ryan, accepted by Nicholas): the approach's distortion field opens only when Cinder is finished AND
	// wave 5 is cleared. In the normal flow wave 5 cannot start before Cinder (its phase is set when Cinder's
	// objectives complete); a stage, a cheat or an old save could reach this clear without Cinder, so check both.
	if (Wave.bRevealsDominion && Flow && IsCinderFinished(Flow->GetProgress()))
	{
		Flow->SetDominionState(true, false);
	}
}

bool ATDWaveDirector::IsCinderFinished(const FTDProgress& Progress)
{
	const UTDFlowSettings* Settings = UTDFlowSettings::Get();
	if (!Settings)
	{
		return true;
	}
	for (const FName& Id : Settings->CinderObjectives)
	{
		if (!Progress.CompletedObjectives.Contains(Id)) { return false; }
	}
	return true;
}

void ATDWaveDirector::AdvanceAfterBreather()
{
	const int32 NextIndex = CurrentWave + 1;
	if (Waves.IsValidIndex(NextIndex))
	{
		CurrentTake = 1;
		BeginWave(NextIndex);
	}
	else
	{
		SetState(ETDWaveState::AllComplete);
	}
}

void ATDWaveDirector::SkipWave()
{
	LoadWaveTable();
	switch (State)
	{
	case ETDWaveState::WarpingIn:
	case ETDWaveState::Active:
		ClearEnemies();
		FinishWave();
		break;
	case ETDWaveState::Breather:
		AdvanceAfterBreather();
		break;
	case ETDWaveState::Idle:
	case ETDWaveState::PhaseComplete:
		// Debug convenience: jump to the next wave regardless of phase.
		if (Waves.IsValidIndex(CurrentWave + 1))
		{
			StartWave(CurrentWave + 1);
		}
		break;
	default:
		break;
	}
}

void ATDWaveDirector::ClearEnemies()
{
	Pending.Reset();
	TArray<TWeakObjectPtr<AActor>> ToRemove = MoveTemp(Living);
	Living.Reset();
	for (const TWeakObjectPtr<AActor>& Weak : ToRemove)
	{
		if (AActor* A = Weak.Get())
		{
			A->OnDestroyed.RemoveDynamic(this, &ATDWaveDirector::HandleEnemyDestroyed);
			if (UTDHealthComponent* H = UTDStatics::GetHealth(A))
			{
				H->OnDeath.RemoveDynamic(this, &ATDWaveDirector::HandleEnemyDeath);
			}
			A->Destroy();
		}
	}
}

void ATDWaveDirector::SetWaveHyperdriveLock(bool bLocked)
{
	UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this);
	if (!Travel)
	{
		return;
	}
	if (bLocked)
	{
		Travel->AddHyperdriveLock(TDHyperdriveLocks::Wave, NSLOCTEXT("TD", "HyperLockWave", "HYPERDRIVE LOCKED - ENEMIES IN RANGE"), this);	// pruned if we die
	}
	else
	{
		Travel->RemoveHyperdriveLock(TDHyperdriveLocks::Wave);
	}
}

void ATDWaveDirector::StopAndClear()
{
	StopAwaitingCombatZone();
	SetWaveHyperdriveLock(false);
	ClearEnemies();
	// Leftover enemy mines and shots go too (T2's pool).
	if (UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(this))
	{
		Projectiles->KillAll(false);
	}
	SetState(ETDWaveState::Idle);
}

// --------------------------------------------------------------------------------------------- tick

bool ATDWaveDirector::IsTutorialWithdrawalWave() const
{
	if (!Waves.IsValidIndex(CurrentWave) || !Waves[CurrentWave].bTutorialWithdrawal || TutorialMode == ETDTutorialMode::ForceOff)
	{
		return false;
	}
	if (TutorialMode == ETDTutorialMode::ForceOn)
	{
		return true;
	}
	// Real levels only: test worlds and simulations keep clearing this wave by kills.
	const UWorld* World = GetWorld();
	return World && !World->GetOutermost()->GetName().StartsWith(TEXT("/Temp/"));
}

bool ATDWaveDirector::GetEscapeBoundary(FVector& OutPoint, FVector& OutNormal) const
{
	OutPoint = EscapePoint;
	OutNormal = EscapeNormal;
	return TutorialPhase != ETDTutorialPhase::Battle;
}

void ATDWaveDirector::ResetTutorial()
{
	for (const TWeakObjectPtr<AActor>& W : HeldPursuers)
	{
		if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(W.Get()))
		{
			Craft->SetFlightParked(false);
			if (AController* C = Craft->GetController()) { C->SetActorTickEnabled(true); }
		}
	}
	HeldPursuers.Reset();
	if (AActor* M = EscapeMarker.Get()) { M->Destroy(); }
	EscapeMarker.Reset();
	TutorialPhase = ETDTutorialPhase::Battle;
	HeavyContact.Reset();
	bHeavySeen = false;
	bHeavyEngaged = false;
	HeavyTime = 0.f;
}

void ATDWaveDirector::BeginWithdrawal()
{
	if (TutorialPhase != ETDTutorialPhase::Battle)
	{
		return;
	}
	TutorialPhase = ETDTutorialPhase::Withdrawal;
	// The boundary: EscapeBoundaryDistance from the battle centre (the galaxy's combat zone, else where the flown ship
	// is), on the bearing to Verdance - the way the fleet is heading and the story goes next.
	const AActor* Anchor = GetSpawnAnchor();
	FVector Centre = Anchor ? Anchor->GetActorLocation() : GetActorLocation();
	FVector Toward = FVector::ForwardVector;
	if (const ATDGalaxyLayout* Layout = ATDGalaxyLayout::Get(this))
	{
		Centre = Layout->GetActorTransform().TransformPosition(Layout->Layout.CombatZoneCenter);
		for (const ATDPlanetAtmosphere* P : Layout->GetPlanets())
		{
			if (P && P->GetDestination() == ETDDestination::Verdance) { Toward = (P->GetPlanetCenter() - Centre).GetSafeNormal(); }
		}
	}
	EscapeNormal = Toward.IsNearlyZero() ? FVector::ForwardVector : Toward;
	EscapePoint = Centre + EscapeNormal * EscapeBoundaryDistance;
	// The corridor marker: T2's HUD draws an objective waypoint on TD.Objective.Marker.
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AActor* M = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(EscapeNormal.Rotation(), EscapePoint), P))
	{
		USceneComponent* Root = NewObject<USceneComponent>(M, TEXT("Root"));
		M->SetRootComponent(Root);
		Root->RegisterComponent();
		M->SetActorLocationAndRotation(EscapePoint, EscapeNormal.Rotation());
		M->Tags.Add(TDTags::ObjectiveMarker);
		M->Tags.Add(TEXT("TD.Tutorial.Escape"));
		EscapeMarker = M;
	}
	UTDStorySubsystem::Fire(this, TEXT("TB_Command_FallBack"));
	OnTutorialWithdrawalOrdered.Broadcast();
	UE_LOG(LogTriadDrift, Log, TEXT("TD.Waves: tutorial withdrawal ordered - escape boundary %.0f m out toward Verdance"), EscapeBoundaryDistance / 100.f);
}

void ATDWaveDirector::CompleteEscape()
{
	if (TutorialPhase == ETDTutorialPhase::Escaped)
	{
		return;
	}
	TutorialPhase = ETDTutorialPhase::Escaped;
	if (AActor* M = EscapeMarker.Get()) { M->Destroy(); }
	EscapeMarker.Reset();
	HeldPursuers.Reset();
	UTDStorySubsystem::Fire(this, TEXT("TB_Kato_BreakingOff"));
	UTDStorySubsystem::Fire(this, TEXT("TB_Escaped"));
	OnTutorialEscaped.Broadcast();
	// T7's profile: the tutorial counts as done, so a later New Game may skip it. Never from a test world (/Temp/), which
	// would write the player's real profile slot.
	const UWorld* World = GetWorld();
	if (World && !World->GetOutermost()->GetName().StartsWith(TEXT("/Temp/")))
	{
		if (UTDProfileSubsystem* Profile = UTDProfileSubsystem::Get(this))
		{
			Profile->NotifyTutorialCompleted(TEXT("escape boundary"));
		}
	}
	UE_LOG(LogTriadDrift, Log, TEXT("TD.Waves: tutorial complete - the flown ship crossed the escape boundary"));
	// What remains stands down (the gunship may still be alive), then the wave clears the normal way.
	Pending.Reset();
	ClearEnemies();
	FinishWave();
}

void ATDWaveDirector::TickTutorial(float DeltaSeconds)
{
	const AActor* Flown = GetSpawnAnchor();
	if (TutorialPhase == ETDTutorialPhase::Battle)
	{
		// Phase 04 -> 05: the heavy contact engaged long enough, worn down to half, or destroyed.
		AActor* Heavy = HeavyContact.Get();
		if (!bHeavySeen)
		{
			return;
		}
		const UTDHealthComponent* H = Heavy ? UTDStatics::GetHealth(Heavy) : nullptr;
		const bool bDown = !Heavy || !H || H->IsDead();
		if (!bDown)
		{
			HeavyTime += DeltaSeconds;
			bHeavyEngaged |= H->GetHull01() < 0.999f || (Flown && FVector::Dist(Flown->GetActorLocation(), Heavy->GetActorLocation()) <= HeavyEngageRange);
		}
		if (bDown || H->GetHull01() <= HeavyHullTrigger || (bHeavyEngaged && HeavyTime >= HeavyEngagedSeconds))
		{
			BeginWithdrawal();
		}
		return;
	}
	if (TutorialPhase != ETDTutorialPhase::Withdrawal)
	{
		return;
	}
	// Pursuers chase to the perimeter and stop there, deliberately: parked, guns off, facing the squad.
	for (const TWeakObjectPtr<AActor>& Weak : Living)
	{
		AActor* A = Weak.Get();
		if (!A || HeldPursuers.Contains(A)) { continue; }
		if (FVector::DotProduct(A->GetActorLocation() - EscapePoint, EscapeNormal) > -PursuerStopMargin)
		{
			HeldPursuers.Add(A);
			if (ATDEnemyShip* Ship = Cast<ATDEnemyShip>(A))
			{
				Ship->SetGunsFiring(false, nullptr);
				if (AController* C = Ship->GetController()) { C->SetActorTickEnabled(false); }
				Ship->SetFlightParked(true);
				if (Flown) { Ship->SetActorRotation((Flown->GetActorLocation() - Ship->GetActorLocation()).Rotation()); }
			}
		}
	}
	// The flown ship crossing completes the tutorial (wingmen follow; the player owns the moment).
	if (Flown && FVector::DotProduct(Flown->GetActorLocation() - EscapePoint, EscapeNormal) > 0.f)
	{
		CompleteEscape();
	}
}

void ATDWaveDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if ((State == ETDWaveState::WarpingIn || State == ETDWaveState::Active) && IsTutorialWithdrawalWave())
	{
		TickTutorial(DeltaSeconds);
	}

	if (bOpeningGrace)
	{
		TickOpeningGrace(DeltaSeconds);
	}

	// MAJOR B: a held wave starts once the player is back in the combat zone (re-checked from progress, so a phase
	// change meanwhile is honoured).
	if (bAwaitingCombatZone && State == ETDWaveState::Idle)
	{
		CombatZoneCheckTimer -= DeltaSeconds;
		if (CombatZoneCheckTimer <= 0.f)
		{
			CombatZoneCheckTimer = 0.25f;
			if (IsPlayerInCombatZone())
			{
				StopAwaitingCombatZone();
				StartFromProgress();
			}
		}
	}

	// The wave lock follows the state: re-asserted a few times a second while a wave runs, so a retake or
	// anything else that clears locks can't open the hyperdrive mid-wave (whatever order handlers run in).
	if (State == ETDWaveState::WarpingIn || State == ETDWaveState::Active)
	{
		LockSyncTimer -= DeltaSeconds;
		if (LockSyncTimer <= 0.f)
		{
			LockSyncTimer = 0.25f;
			SetWaveHyperdriveLock(true);
		}
	}

	switch (State)
	{
	case ETDWaveState::WarpingIn:
	case ETDWaveState::Active:
	{
		for (int32 i = Pending.Num() - 1; i >= 0; --i)
		{
			FPendingGroup& P = Pending[i];
			P.Remaining -= DeltaSeconds;
			if (!P.bWarpAnnounced && P.Remaining <= WarpInSeconds)
			{
				P.bWarpAnnounced = true;
				const FTDEnemySpec& Spec = UTDEnemyLibrary::Get(P.Group.EnemyId);
				PickSpawnPoint(P.Group.Zone, Spec.IsValid() && Spec.Size >= ETDEnemySize::Large, P.Location, P.Facing);
				OnWarpInStarted.Broadcast(P.Location, P.Group.EnemyId);
#if !UE_BUILD_SHIPPING
				if (const AActor* Anchor = GetSpawnAnchor())
				{
					UE_LOG(LogTriadDrift, Display, TEXT("TD.Waves: wave %d warps in %s %.0f cm from %s"), CurrentWave + 1, *P.Group.EnemyId.ToString(),
						FVector::Dist(P.Location, Anchor->GetActorLocation()), *Anchor->GetName());
				}
#endif
				if (TravelFX)
				{
					UTDStatics::PlayCue(this, TravelFX->EnemyWarpIn, P.Location, P.Facing);
				}
				if (!TravelFX || TravelFX->EnemyWarpIn.VFX.IsNull())
				{
					// Placeholder telegraph until Nick's VFX lands: the player must see where a wave arrives.
					SpawnWarpInMarker(P.Location, Spec.IsValid() ? Spec.Radius : 300.f);
				}
			}
			if (P.Remaining <= 0.f)
			{
				if (!P.bWarpAnnounced)
				{
					const FTDEnemySpec& Spec = UTDEnemyLibrary::Get(P.Group.EnemyId);
					PickSpawnPoint(P.Group.Zone, Spec.IsValid() && Spec.Size >= ETDEnemySize::Large, P.Location, P.Facing);
				}
				const FPendingGroup Ready = P;
				Pending.RemoveAt(i);
				SpawnGroup(Ready);
			}
		}
		if (State == ETDWaveState::WarpingIn && Living.Num() > 0)
		{
			SetState(ETDWaveState::Active);
		}
		CheckWaveCleared();
		break;
	}
	case ETDWaveState::Breather:
		// Squad hull repair between waves belongs to the squad manager (ENG-03, 8%/s): it reads bInBreather.
		StateRemaining -= DeltaSeconds;
		if (StateRemaining <= 0.f)
		{
			AdvanceAfterBreather();
		}
		break;
	default:
		break;
	}
}

FTDWaveInfo ATDWaveDirector::GetWaveInfo() const
{
	FTDWaveInfo Info;
	Info.WaveCount = Waves.Num();
	if (const FTDWaveDef* Def = GetWaveDef(CurrentWave))
	{
		Info.WaveNumber = CurrentWave + 1;
		Info.Intensity01 = Def->Intensity01;
		Info.SceneId = GetSceneId(CurrentWave);
	}
	Info.EnemiesAlive = GetEnemiesAlive();
	Info.EnemiesTotal = EnemiesTotal;
	Info.bWaveActive = State == ETDWaveState::WarpingIn || State == ETDWaveState::Active;
	Info.bInBreather = State == ETDWaveState::Breather;
	Info.BreatherRemaining = GetBreatherRemaining();
	if (State == ETDWaveState::AllComplete)
	{
		// All waves done (the Dominion assault and after): no wave number, nothing to count. The objective
		// line (T5) carries the finale; a HUD shows its wave block only while bWaveActive or bInBreather.
		Info.WaveNumber = 0;
		Info.SceneId = NAME_None;
		Info.EnemiesAlive = 0;
		Info.EnemiesTotal = 0;
		Info.Intensity01 = 0.f;
	}
	return Info;
}

#undef LOCTEXT_NAMESPACE
