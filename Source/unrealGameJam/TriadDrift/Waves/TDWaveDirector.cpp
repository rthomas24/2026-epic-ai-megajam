// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Waves/TDWaveDirector.h"

#include "TriadDrift/Waves/TDWaveEnemy.h"
#include "TriadDrift/Waves/TDWarpInMarker.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
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
		W.Groups.Add(TDWaveDir_Group(ShardFighter, 3, 5, ETDSpawnZone::AnyFlank));
		W.Groups.Add(TDWaveDir_Group(HookInterceptor, 2, 1, ETDSpawnZone::Behind, 4.f));
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
		W.Groups.Add(TDWaveDir_Group(HookInterceptor, 4, 1, ETDSpawnZone::Behind, 5.f));
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
	StartWave(Next);
}

void ATDWaveDirector::StartWave(int32 Index)
{
	LoadWaveTable();
	if (!Waves.IsValidIndex(Index))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("ATDWaveDirector::StartWave: index %d out of range (0..%d)"), Index, Waves.Num() - 1);
		return;
	}
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

void ATDWaveDirector::HandleTakeStarted(FName SceneId, int32 TakeNumber)
{
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
	if (Living.Num() == 0)
	{
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
		SetState(ETDWaveState::Breather, BreatherSeconds);
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
	if (Wave.bRevealsDominion && Flow)
	{
		Flow->SetDominionState(true, false);
	}
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

void ATDWaveDirector::RepairSquadHulls(float DeltaSeconds)
{
	UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	if (!Registry || BreatherHullRepairPerSecond01 <= 0.f)
	{
		return;
	}
	ScratchActors.Reset();
	Registry->GetActorsOfTeam(ETDTeam::Player, ScratchActors, true);
	for (AActor* A : ScratchActors)
	{
		if (UTDHealthComponent* H = UTDStatics::GetHealth(A))
		{
			if (H->IsAlive() && H->GetHull01() < 1.f)
			{
				H->RepairHull(H->MaxHull * BreatherHullRepairPerSecond01 * DeltaSeconds);
			}
		}
	}
}

void ATDWaveDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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
		RepairSquadHulls(DeltaSeconds);
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
