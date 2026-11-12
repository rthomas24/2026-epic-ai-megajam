// Triad Drift - Director (Teammate 3). C-03 #2 "Director cam money shots".

#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Director/TDDirectorCamera.h"
#include "TriadDrift/Director/TDCutPresentation.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

FTDDirectorTuning::FTDDirectorTuning()
{
	// Bigger story beats matter more; everything is tunable.
	BaseImportance.Add(ETDHighlightKind::ReactorKill, 1.0f);
	BaseImportance.Add(ETDHighlightKind::GeneratorKill, 0.85f);
	BaseImportance.Add(ETDHighlightKind::CarrierKill, 0.8f);
	BaseImportance.Add(ETDHighlightKind::RamKill, 0.6f);
	BaseImportance.Add(ETDHighlightKind::PerfectDeflect, 0.6f);
	BaseImportance.Add(ETDHighlightKind::MissileReturn, 0.55f);
	BaseImportance.Add(ETDHighlightKind::MultiKill, 0.5f);
	BaseImportance.Add(ETDHighlightKind::SwitchStrikeKill, 0.5f);
	BaseImportance.Add(ETDHighlightKind::AbilityCombo, 0.5f);

	DurationOverrideSeconds.Add(ETDHighlightKind::PerfectDeflect, 0.15f);
}

UTDDirectorSubsystem* UTDDirectorSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDDirectorSubsystem>() : nullptr;
}

bool UTDDirectorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTDDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CameraRig = NewObject<UTDDirectorCameraRig>(this);
	CameraRig->SetWorld(GetWorld());
	CutPresentation = NewObject<UTDCutPresentation>(this);
}

TStatId UTDDirectorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDDirectorSubsystem, STATGROUP_Tickables);
}

void UTDDirectorSubsystem::Tick(float DeltaTime)
{
	// Real (undilated) time: slow-mo must never stretch the camera blends.
	const double Now = RealNow();
	const float RealDelta = LastTickRealTime < 0.0 ? 0.f : static_cast<float>(Now - LastTickRealTime);
	LastTickRealTime = Now;
	if (CutPresentation)
	{
		CutPresentation->SetHandledExternally(bCutPresentationHandledExternally);
		const UTDGameInstance* GI = GetWorld() ? Cast<UTDGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
		if (GI && GI->IsTravelPending())
		{
			CutPresentation->ClearAll(); // nothing of this level's presentation survives travel
		}
		else
		{
			CutPresentation->Tick(Now);
		}
	}
	if (!CameraRig)
	{
		return;
	}
	if (CameraRig->IsActive() && !CanTriggerHighlights())
	{
		CameraRig->Cancel(bSceneCutPending ? ETDDirectorCameraCancel::SceneCut : ETDDirectorCameraCancel::Travel);
	}
	CameraRig->Tick(Now, RealDelta);
}

void UTDDirectorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	BindBus();
	if (CutPresentation)
	{
		CutPresentation->Init(&InWorld);
	}
}

void UTDDirectorSubsystem::Deinitialize()
{
	UnbindBus();
	if (CutPresentation)
	{
		CutPresentation->Shutdown(); // world teardown (travel): the overlay goes with it
	}
	if (CameraRig)
	{
		CameraRig->Cancel(ETDDirectorCameraCancel::Travel);
	}
	Super::Deinitialize();
}

void UTDDirectorSubsystem::BindBus()
{
	if (bBusBound)
	{
		return;
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(GetWorld()))
	{
		Bus->OnActorKilled.AddUniqueDynamic(this, &UTDDirectorSubsystem::HandleActorKilled);
		Bus->OnSceneCut.AddUniqueDynamic(this, &UTDDirectorSubsystem::HandleSceneCut);
		Bus->OnTakeStarted.AddUniqueDynamic(this, &UTDDirectorSubsystem::HandleTakeStarted);
		bBusBound = true;
	}
}

void UTDDirectorSubsystem::UnbindBus()
{
	if (!bBusBound)
	{
		return;
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(GetWorld()))
	{
		Bus->OnActorKilled.RemoveAll(this);
		Bus->OnSceneCut.RemoveAll(this);
		Bus->OnTakeStarted.RemoveAll(this);
	}
	bBusBound = false;
}

double UTDDirectorSubsystem::RealNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetRealTimeSeconds() : 0.0;
}

double UTDDirectorSubsystem::GameNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

float UTDDirectorSubsystem::GetBaseImportance(ETDHighlightKind Kind) const
{
	const float* V = Tuning.BaseImportance.Find(Kind);
	return V ? *V : 0.5f;
}

bool UTDDirectorSubsystem::IsShotActive() const
{
	return RealNow() < ShotEndRealTime;
}

bool UTDDirectorSubsystem::CanTriggerHighlights() const
{
	if (bSceneCutPending)
	{
		return false;
	}
	const UWorld* World = GetWorld();
	if (const UTDGameInstance* GI = World ? Cast<UTDGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (GI->IsRetakePending() || GI->IsTravelPending())
		{
			return false;
		}
	}
	return true;
}

// ---------------------------------------------------------------------------------------------
// Highlights
// ---------------------------------------------------------------------------------------------

bool UTDDirectorSubsystem::ReportHighlight(ETDHighlightKind Kind, AActor* Subject, float Importance01, AActor* Secondary)
{
	UWorld* World = GetWorld();
	if (!World || !Subject || !CanTriggerHighlights())
	{
		return false; // m12: no subject = nothing to frame
	}
	const float Importance = FMath::Clamp(Importance01 < 0.f ? GetBaseImportance(Kind) : Importance01, 0.f, 1.f);
	if (Importance < Tuning.MinImportance)
	{
		return false;
	}

	const double Now = RealNow();
	const bool bBypass = Importance >= Tuning.CooldownBypassImportance;
	if (IsShotActive())
	{
		// A running shot is replaced by something clearly bigger (generator kill during a ram kill),
		// and story beats (bypass level: generator, carrier, reactor) are never dropped: they take
		// over any shot that isn't more important, so all four generator kills get their beat.
		const bool bClearlyBigger = Importance >= CurrentShot.Importance01 + Tuning.PreemptMargin;
		const bool bStoryBeat = bBypass && Importance >= CurrentShot.Importance01;
		if (!bClearlyBigger && !bStoryBeat)
		{
			return false;
		}
	}
	else if (!bBypass)
	{
		if (Now < GlobalCooldownEndRealTime)
		{
			return false;
		}
		if (const double* KindEnd = KindCooldownEnd.Find(Kind); KindEnd && Now < *KindEnd)
		{
			return false;
		}
	}

	// Accept: build the shot.
	FTDDirectorShot Shot;
	Shot.Kind = Kind;
	Shot.Subject = Subject;
	Shot.Secondary = Secondary;
	Shot.Importance01 = Importance;
	const float* Override = Tuning.DurationOverrideSeconds.Find(Kind);
	Shot.DurationSeconds = Override ? *Override : FMath::Lerp(Tuning.MinSlowMoSeconds, Tuning.MaxSlowMoSeconds, Importance);
	Shot.Dilation = FMath::Lerp(Tuning.DilationAtMinImportance, Tuning.DilationAtMaxImportance, Importance);
	Shot.StartRealTime = static_cast<float>(Now);
	BuildCameraHint(Shot);

	CurrentShot = Shot;
	ShotEndRealTime = Now + Shot.DurationSeconds;
	GlobalCooldownEndRealTime = ShotEndRealTime + Tuning.GlobalCooldownSeconds;
	KindCooldownEnd.Add(Kind, Now + Tuning.SameKindCooldownSeconds);
	++ShotCount;

	if (UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(World))
	{
		Feel->RequestSlowMo(Shot.DurationSeconds, Shot.Dilation);
	}
	if (CutPresentation)
	{
		CutPresentation->HandleShotStarted(Shot.DurationSeconds + 0.2f); // thin bars for the shot (+ return)
	}
	// C-03 #2: the camera. Full = cut to the director camera; Reduced = FOV punch only; Off = none.
	if (CameraRig)
	{
		switch (UTDDirectorSettings::GetEffectiveMode())
		{
		case ETDDirectorCameraMode::Full:
			CameraRig->BeginShot(Shot, ShotEndRealTime);
			break;
		case ETDDirectorCameraMode::Reduced:
			CameraRig->PunchFOV();
			break;
		default:
			break;
		}
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(World))
	{
		Bus->BroadcastHighlightMoment(Kind, Subject, Importance);
	}
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(World))
	{
		Flow->AddStat(TEXT("HighlightMoments"), 1);
	}
	OnDirectorShot.Broadcast(CurrentShot);
	return true;
}

void UTDDirectorSubsystem::BuildCameraHint(FTDDirectorShot& Shot)
{
	const AActor* Subject = Shot.Subject.Get();
	const AActor* Other = Shot.Secondary.Get();
	FVector Focus = Subject ? Subject->GetActorLocation() : (Other ? Other->GetActorLocation() : FVector::ZeroVector);
	if (Subject && Other && FVector::Dist(Subject->GetActorLocation(), Other->GetActorLocation()) <= Tuning.MaxTwoShotDistance)
	{
		// Two-shot: frame between them, weighted to the subject.
		Focus = FMath::Lerp(Subject->GetActorLocation(), Other->GetActorLocation(), 0.35f);
	}
	Shot.FocusLocation = Focus;

	// Travel direction of the action (subject's velocity, else its facing, else world forward).
	FVector Travel = Subject ? Subject->GetVelocity() : FVector::ZeroVector;
	if (Travel.SizeSquared() < 1.f)
	{
		Travel = Subject ? Subject->GetActorForwardVector() : FVector::ForwardVector;
	}
	Travel = Travel.GetSafeNormal();
	FVector Side = FVector::CrossProduct(Travel, FVector::UpVector).GetSafeNormal();
	if (Side.IsNearlyZero())
	{
		Side = FVector::RightVector;
	}
	if (bCameraSideFlip)
	{
		Side = -Side;
	}
	bCameraSideFlip = !bCameraSideFlip; // alternate sides shot to shot

	const float Dist = FMath::Lerp(Tuning.CameraDistanceMin, Tuning.CameraDistanceMax, Shot.Importance01);
	// Side-on, a little ahead and above: the action crosses the frame toward the lens.
	Shot.SuggestedCameraLocation = Focus + Side * Dist * 0.85f + Travel * Dist * 0.3f + FVector::UpVector * Dist * 0.3f;
	Shot.SuggestedCameraRotation = (Focus - Shot.SuggestedCameraLocation).Rotation();
}

// ---------------------------------------------------------------------------------------------
// Automatic detection from kills
// ---------------------------------------------------------------------------------------------

void UTDDirectorSubsystem::HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind)
{
	if (!Victim || !Killer)
	{
		return;
	}
	// Only the player's side earns money shots, and only for killing hostiles.
	if (UTDStatics::GetTeam(Killer) != ETDTeam::Player || !UTDStatics::AreTeamsHostile(ETDTeam::Player, UTDStatics::GetTeam(Victim)))
	{
		return;
	}
	// M8: kills by the ship the player flies (with a squad present); wingman kills don't count.
	const ATDSquadManager* SquadForKills = ATDSquadManager::Get(this);
	const bool bPlayerKill = !SquadForKills || !SquadForKills->GetActiveShip() || Killer == SquadForKills->GetActiveShip();

	// Multi-kill bookkeeping, on REAL time (M8: slow-mo must not stretch the window).
	const double Now = RealNow();
	if (bPlayerKill)
	{
		RecentKillTimes.Add(Now);
	}
	while (RecentKillTimes.Num() > 0 && Now - RecentKillTimes[0] > Tuning.MultiKillWindowSeconds)
	{
		RecentKillTimes.RemoveAt(0, EAllowShrinking::No);
	}
	if (RecentKillTimes.Num() > 32)
	{
		RecentKillTimes.RemoveAt(0, RecentKillTimes.Num() - 32, EAllowShrinking::No);
	}

	// Pick the single best reading of this kill.
	ETDHighlightKind BestKind = ETDHighlightKind::RamKill;
	float BestImportance = -1.f;
	AActor* BestSubject = nullptr;
	AActor* BestSecondary = nullptr;
	auto Consider = [&](ETDHighlightKind K, float Importance, AActor* Subject, AActor* Secondary)
	{
		if (Importance > BestImportance)
		{
			BestKind = K;
			BestImportance = Importance;
			BestSubject = Subject;
			BestSecondary = Secondary;
		}
	};

	if (Victim->ActorHasTag(TDDirectorTags::Reactor))
	{
		Consider(ETDHighlightKind::ReactorKill, GetBaseImportance(ETDHighlightKind::ReactorKill), Victim, Killer);
	}
	if (Victim->ActorHasTag(TDDirectorTags::Generator))
	{
		Consider(ETDHighlightKind::GeneratorKill, GetBaseImportance(ETDHighlightKind::GeneratorKill), Victim, Killer);
	}
	if (Victim->ActorHasTag(TDDirectorTags::Carrier))
	{
		Consider(ETDHighlightKind::CarrierKill, GetBaseImportance(ETDHighlightKind::CarrierKill), Victim, Killer);
	}
	if (Kind == ETDDamageKind::Ram)
	{
		Consider(ETDHighlightKind::RamKill, GetBaseImportance(ETDHighlightKind::RamKill), Killer, Victim);
	}
	const int32 Streak = RecentKillTimes.Num();
	if (bPlayerKill && Streak >= Tuning.MultiKillCount)
	{
		// M8: never into the story tier (cap below the cooldown-bypass level).
		const float Extra = Tuning.MultiKillImportancePerExtraKill * static_cast<float>(Streak - Tuning.MultiKillCount);
		const float Cap = FMath::Min(Tuning.MultiKillMaxImportance, Tuning.CooldownBypassImportance - 0.05f);
		Consider(ETDHighlightKind::MultiKill, FMath::Min(Cap, GetBaseImportance(ETDHighlightKind::MultiKill) + Extra), Killer, Victim);
	}
	if (const ATDSquadManager* Squad = ATDSquadManager::Get(this))
	{
		if (Squad->IsSwitchStrikeActive() && Squad->GetDamageMultiplier(Killer) > 1.f)
		{
			Consider(ETDHighlightKind::SwitchStrikeKill, GetBaseImportance(ETDHighlightKind::SwitchStrikeKill), Killer, Victim);
		}
	}

	if (BestImportance >= 0.f && ReportHighlight(BestKind, BestSubject, BestImportance, BestSecondary))
	{
		if (BestKind == ETDHighlightKind::MultiKill)
		{
			RecentKillTimes.Reset(); // M8: the next multi-kill needs a fresh run of kills
		}
	}
}

void UTDDirectorSubsystem::HandleSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds)
{
	// "CUT!": no money shots until the next take starts, and stop any running one.
	bSceneCutPending = true;
	if (CameraRig)
	{
		CameraRig->Cancel(ETDDirectorCameraCancel::SceneCut); // the retake starts on the player's own view
	}
	if (CutPresentation)
	{
		CutPresentation->HandleSceneCut(DelaySeconds);
	}
	if (IsShotActive())
	{
		if (UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(GetWorld()))
		{
			Feel->CancelAll(); // the retake runs at normal speed
		}
	}
	ShotEndRealTime = -1000.0;
	RecentKillTimes.Reset();
}

void UTDDirectorSubsystem::HandleTakeStarted(FName SceneId, int32 TakeNumber)
{
	if (CutPresentation)
	{
		CutPresentation->HandleTakeStarted(SceneId, TakeNumber);
	}
	bSceneCutPending = false;
}
