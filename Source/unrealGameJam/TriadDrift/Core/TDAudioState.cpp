#include "TriadDrift/Core/TDAudioState.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"

namespace TDAudioState
{
	// Smoothing rates, in units per real second.
	constexpr float MusicRise = 0.5f;    // 0 -> 1 in 2 s
	constexpr float MusicFall = 0.2f;    // 1 -> 0 in 5 s
	constexpr float CombatRise = 1.5f;
	constexpr float CombatFall = 0.35f;
	constexpr float DangerRate = 2.f;

	constexpr float DangerHealth = 0.35f;   // hull fraction where Danger01 starts rising
	constexpr float NeverSeconds = 1000.f;

	float Approach(float Current, float Target, float Rise, float Fall, float RealDt)
	{
		return FMath::FInterpConstantTo(Current, Target, RealDt, Target > Current ? Rise : Fall);
	}

	/** 1 right after a hit or a kill, fading to 0 over a few seconds. */
	float RecentActivity01(const FTDAudioState& S)
	{
		const float FromHit = 1.f - S.SecondsSinceHit / 4.f;
		const float FromKill = 1.f - S.SecondsSinceKill / 6.f;
		return FMath::Clamp(FMath::Max(FromHit, FromKill), 0.f, 1.f);
	}
}

UTDAudioStateSubsystem* UTDAudioStateSubsystem::Get(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
	{
		return nullptr;
	}
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UTDAudioStateSubsystem>() : nullptr;
}

bool UTDAudioStateSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UTDAudioStateSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	BindBus();
	if (const ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		State.Phase = Flow->GetProgress().Phase;
	}
}

void UTDAudioStateSubsystem::BindBus()
{
	if (bBusBound)
	{
		return;
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnPhaseChanged.AddUniqueDynamic(this, &UTDAudioStateSubsystem::HandlePhaseChanged);
		Bus->OnShipSwitched.AddUniqueDynamic(this, &UTDAudioStateSubsystem::HandleShipSwitched);
		Bus->OnHitConfirmed.AddUniqueDynamic(this, &UTDAudioStateSubsystem::HandleHitConfirmed);
		Bus->OnHyperdriveState.AddUniqueDynamic(this, &UTDAudioStateSubsystem::HandleHyperdriveState);
		Bus->OnHighlightMoment.AddUniqueDynamic(this, &UTDAudioStateSubsystem::HandleHighlight);
		Bus->OnSceneCut.AddUniqueDynamic(this, &UTDAudioStateSubsystem::HandleSceneCut);
		Bus->OnTakeStarted.AddUniqueDynamic(this, &UTDAudioStateSubsystem::HandleTakeStarted);
		bBusBound = true;
	}
}

void UTDAudioStateSubsystem::Deinitialize()
{
	// The bus lives on the game instance and outlives this world: never leave a binding behind.
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnPhaseChanged.RemoveAll(this);
		Bus->OnShipSwitched.RemoveAll(this);
		Bus->OnHitConfirmed.RemoveAll(this);
		Bus->OnHyperdriveState.RemoveAll(this);
		Bus->OnHighlightMoment.RemoveAll(this);
		Bus->OnSceneCut.RemoveAll(this);
		Bus->OnTakeStarted.RemoveAll(this);
	}
	bBusBound = false;
	Super::Deinitialize();
}

void UTDAudioStateSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Real seconds, so slow motion and hit-stop do not slow the reaction of music and mixes.
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetRealTimeSeconds() : 0.0;
	const float RealDt = LastRefreshRealTime < 0.0 ? 0.f : FMath::Clamp(static_cast<float>(Now - LastRefreshRealTime), 0.f, 0.25f);
	LastRefreshRealTime = Now;
	Refresh(RealDt);
}

TStatId UTDAudioStateSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDAudioStateSubsystem, STATGROUP_Tickables);
}

AActor* UTDAudioStateSubsystem::FindSquadData()
{
	if (AActor* Cached = CachedSquadData.Get())
	{
		return Cached;
	}
	const UWorld* World = GetWorld();
	if (!World || World->GetRealTimeSeconds() < NextSearchRealTime)
	{
		return nullptr;
	}
	// The squad manager is the only non-pawn actor that serves HUD data. Looked for twice a second.
	NextSearchRealTime = World->GetRealTimeSeconds() + 0.5;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (Cast<ITDHUDData>(*It) && !It->IsA<APawn>())
		{
			CachedSquadData = *It;
			return *It;
		}
	}
	return nullptr;
}

void UTDAudioStateSubsystem::Refresh(float RealDeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	BindBus();   // the bus can appear after this subsystem in a test world

	const float Dt = FMath::Max(RealDeltaSeconds, 0.f);
	State.SecondsSinceHit = FMath::Min(State.SecondsSinceHit + Dt, TDAudioState::NeverSeconds);
	State.SecondsSinceKill = FMath::Min(State.SecondsSinceKill + Dt, TDAudioState::NeverSeconds);
	State.SecondsSinceHighlight = FMath::Min(State.SecondsSinceHighlight + Dt, TDAudioState::NeverSeconds);

	const APlayerController* PC = World->GetFirstPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const ITDHUDData* PawnData = Cast<ITDHUDData>(Pawn);
	const bool bOnFoot = Pawn && Pawn->IsA<ACharacter>();
	const ITDHUDData* SquadData = Cast<ITDHUDData>(FindSquadData());

	// ---- where
	if (bOnFoot)
	{
		State.Scene = ETDAudioScene::Ground;
	}
	else if (Pawn && SquadData)
	{
		State.Scene = ETDAudioScene::Space;
	}
	else
	{
		State.Scene = ETDAudioScene::FrontEnd;
	}

	// ---- the player
	if (State.Scene == ETDAudioScene::Space)
	{
		State.Speed01 = FMath::Clamp(SquadData->GetSpeed01(), 0.f, 1.f);
		State.bOutOfBounds = SquadData->IsOutOfBounds();
		State.HyperdriveCharge01 = FMath::Clamp(SquadData->GetHyperdriveCharge01(), 0.f, 1.f);
		State.MissileWarning = SquadData->GetMissileWarning();

		int32 Alive = 0;
		for (const FTDShipHUD& Card : SquadData->GetSquadHUD())
		{
			Alive += Card.bAlive ? 1 : 0;
			if (Card.bPlayerControlled)
			{
				State.ActiveShip = Card.Ship;
				State.Boost01 = FMath::Clamp(Card.Boost01, 0.f, 1.f);
				State.Heat01 = FMath::Clamp(Card.Heat01, 0.f, 1.f);
				State.Health01 = FMath::Clamp(Card.Health01, 0.f, 1.f);
				State.Shield01 = FMath::Clamp(Card.Shield01, 0.f, 1.f);
			}
		}
		State.SquadAlive = Alive;
	}
	else
	{
		State.Speed01 = 0.f;
		State.Boost01 = 1.f;
		State.Heat01 = 0.f;
		State.Shield01 = 1.f;
		State.SquadAlive = 3;
		State.bOutOfBounds = false;
		State.HyperdriveCharge01 = 0.f;
		State.MissileWarning = ETDMissileWarning::None;
		State.Health01 = (bOnFoot && PawnData) ? FMath::Clamp(PawnData->GetHealth01(), 0.f, 1.f) : 1.f;
	}

	// ---- the fight (the wave director reports through the squad's HUD data; none on foot or in menus)
	const FTDWaveInfo Wave = (State.Scene == ETDAudioScene::Space) ? SquadData->GetWaveInfo() : FTDWaveInfo();
	State.WaveNumber = Wave.WaveNumber;
	State.WaveIntensity01 = FMath::Clamp(Wave.Intensity01, 0.f, 1.f);
	State.EnemiesAlive = FMath::Max(Wave.EnemiesAlive, 0);
	State.EnemiesTotal = FMath::Max(Wave.EnemiesTotal, 0);
	State.bWaveActive = Wave.bWaveActive;
	State.bInBreather = Wave.bInBreather;

	// ---- time
	const AWorldSettings* Settings = World->GetWorldSettings();
	State.TimeDilation = Settings ? Settings->GetEffectiveTimeDilation() : 1.f;
	const UTDFeelSubsystem* Feel = World->GetSubsystem<UTDFeelSubsystem>();
	State.bSlowMotion = Feel && Feel->IsSlowMoActive();

	// ---- smoothed values
	const float DangerTarget = State.Scene == ETDAudioScene::FrontEnd
		? 0.f
		: FMath::Clamp((TDAudioState::DangerHealth - State.Health01) / TDAudioState::DangerHealth, 0.f, 1.f);
	State.Danger01 = TDAudioState::Approach(State.Danger01, DangerTarget, TDAudioState::DangerRate, TDAudioState::DangerRate, Dt);
	State.Combat01 = TDAudioState::Approach(State.Combat01, ComputeCombatTarget(), TDAudioState::CombatRise, TDAudioState::CombatFall, Dt);
	State.MusicIntensity01 = TDAudioState::Approach(State.MusicIntensity01, GetMusicIntensityTarget(),
		TDAudioState::MusicRise, TDAudioState::MusicFall, Dt);
}

float UTDAudioStateSubsystem::ComputeCombatTarget() const
{
	if (State.Scene == ETDAudioScene::FrontEnd || State.bSceneCut)
	{
		return 0.f;
	}
	const float Activity = TDAudioState::RecentActivity01(State);
	float Target = 0.3f * Activity;
	if (State.EnemiesAlive > 0)
	{
		// Enemies present: never silent. Twelve or more reads as a full fight.
		Target += 0.35f + 0.25f * FMath::Clamp(State.EnemiesAlive / 12.f, 0.f, 1.f);
	}
	if (State.MissileWarning == ETDMissileWarning::Imminent)
	{
		Target += 0.15f;
	}
	else if (State.MissileWarning == ETDMissileWarning::Tracking)
	{
		Target += 0.08f;
	}
	return FMath::Clamp(Target, 0.f, 1.f);
}

float UTDAudioStateSubsystem::GetMusicIntensityTarget() const
{
	if (State.Scene == ETDAudioScene::FrontEnd)
	{
		return 0.f;
	}
	if (State.bSceneCut)
	{
		return 0.1f;   // "CUT!": the music drops out before the next take
	}
	if (State.Phase == ETDGamePhase::Victory)
	{
		return 0.3f;
	}

	float Target;
	if (State.Scene == ETDAudioScene::Ground)
	{
		Target = 0.3f + 0.4f * TDAudioState::RecentActivity01(State);
	}
	else if (State.bWaveActive)
	{
		Target = 0.4f + 0.35f * State.WaveIntensity01 + 0.15f * State.Combat01;
	}
	else
	{
		Target = State.bInBreather ? 0.25f : 0.15f;
	}

	if (State.Scene == ETDAudioScene::Space && State.Phase == ETDGamePhase::DominionAssault)
	{
		Target = FMath::Max(Target, 0.8f + 0.2f * State.Combat01);
	}
	return FMath::Clamp(Target + 0.1f * State.Danger01, 0.f, 1.f);
}

void UTDAudioStateSubsystem::HandlePhaseChanged(ETDGamePhase Phase)
{
	State.Phase = Phase;
}

void UTDAudioStateSubsystem::HandleShipSwitched(ETDShipId NewShip, ETDShipId OldShip)
{
	State.ActiveShip = NewShip;
}

void UTDAudioStateSubsystem::HandleHitConfirmed(AActor* Target, bool bKill)
{
	State.SecondsSinceHit = 0.f;
	if (bKill)
	{
		State.SecondsSinceKill = 0.f;
	}
}

void UTDAudioStateSubsystem::HandleHyperdriveState(ETDHyperdriveState NewState)
{
	State.Hyperdrive = NewState;
}

void UTDAudioStateSubsystem::HandleHighlight(ETDHighlightKind Kind, AActor* Subject, float Importance01)
{
	State.LastHighlight = Kind;
	State.SecondsSinceHighlight = 0.f;
}

void UTDAudioStateSubsystem::HandleSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds)
{
	State.bSceneCut = true;
}

void UTDAudioStateSubsystem::HandleTakeStarted(FName SceneId, int32 TakeNumber)
{
	State.bSceneCut = false;
}
