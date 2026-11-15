// Triad Drift - the on-foot barks (ENG-43, dialogs.md's T5 rows). Owner: Teammate 5.

#include "TriadDrift/Ground/TDGroundBarks.h"

#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Ground/Planets/TDPlanetProps.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"

namespace TDGroundBarkIds
{
	static const FName TakeOff(TEXT("Bark_TakeOff"));
	static const FName Contact(TEXT("Bark_Foot_Contact"));
	static const FName Deflect(TEXT("Bark_Foot_Deflect"));
	static const FName Perfect(TEXT("Bark_Foot_Perfect"));
	static const FName GuardBroken(TEXT("Bark_Foot_GuardBroken"));
	static const FName Hurt(TEXT("Bark_Foot_Hurt"));
	static const FName LowHealth(TEXT("Bark_Foot_LowHealth"));
	static const FName Heal(TEXT("Bark_Foot_Heal"));
	static const FName Kill(TEXT("Bark_Foot_Kill"));
	static const FName LastOne(TEXT("Bark_Foot_LastOne"));
	static const FName Dash(TEXT("Bark_Foot_Dash"));
	static const FName Slam(TEXT("Bark_Foot_Slam"));
	static const FName ObjectiveNear(TEXT("Bark_Foot_ObjectiveNear"));
	static const FName Done(TEXT("Bark_Foot_Done"));
	static const FName DoorLocked(TEXT("Bark_Foot_DoorLocked"));
	static const FName DoorOpen(TEXT("Bark_Foot_DoorOpen"));
	static const FName Hazard(TEXT("Bark_Foot_Hazard"));
	static const FName Idle(TEXT("Bark_Foot_Idle"));
	static const FName Boarded(TEXT("Bark_Dom_Boarded"));
	static const FName CoreShield(TEXT("Bark_Dom_CoreShield"));
	static const FName CoreOpen(TEXT("Bark_Dom_CoreOpen"));
	static const FName EscapeStart(TEXT("Bark_Dom_Escape_Start"));
	static const FName Escape30(TEXT("Bark_Dom_Escape_30"));
	static const FName ArriveVerdance(TEXT("Bark_Planet_Arrive_Verdance"));
	static const FName ArriveCinder(TEXT("Bark_Planet_Arrive_Cinder"));
	static const FName Escape10(TEXT("Bark_Dom_Escape_10"));
	static const FName Welcome(TEXT("Bark_Cmd_Welcome"));
}

UTDGroundBarks* UTDGroundBarks::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDGroundBarks>() : nullptr;
}

FName UTDGroundBarks::SpeakerName(ETDFootSpeaker Who) const
{
	const FName Pilot = ATDFirstPersonCharacter::GetOnFootPilotName(this);
	switch (Who)
	{
	case ETDFootSpeaker::Pilot: return Pilot;
	case ETDFootSpeaker::Kato: return TEXT("Kato");
	case ETDFootSpeaker::Moss: return TEXT("Moss");
	case ETDFootSpeaker::Wren: return TEXT("Wren"); // #209-allies
	case ETDFootSpeaker::Command: return TEXT("Command");
	default: break;
	}
	// A wingman on the radio: the first squadmate who isn't the one on foot.
	for (const TCHAR* P : { TEXT("Kato"), TEXT("Moss"), TEXT("Wren") })
	{
		if (Pilot != P) { return P; }
	}
	return TEXT("Kato");
}

bool UTDGroundBarks::IsStoryOnAir() const
{
	const UTDStorySubsystem* Story = UTDStorySubsystem::Get(this);
	return Story && (Story->GetCurrentTransmission().bActive || Story->IsSequencePlaying());
}

bool UTDGroundBarks::Say(FName BarkId, ETDFootSpeaker Who, ETDBarkPriority Priority, float CooldownSeconds)
{
	if (IsStoryOnAir())
	{
		return false; // never over a must-hear line
	}
	if (CooldownSeconds > 0.f)
	{
		Tuning.CooldownById.Add(BarkId, CooldownSeconds);
	}
	const UWorld* World = GetWorld();
	if (!World || !Limiter.TryConsume(Tuning, BarkId, Priority, World->GetRealTimeSeconds()))
	{
		return false;
	}
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		Flow->RequestBark(BarkId, SpeakerName(Who));
	}
	Said.Add(BarkId);
	LastSpeaker.Add(BarkId, SpeakerName(Who));
	return true;
}

bool UTDGroundBarks::SayOnce(FName BarkId, ETDFootSpeaker Who, ETDBarkPriority Priority)
{
	if (SaidOnce.Contains(BarkId))
	{
		return false;
	}
	if (Say(BarkId, Who, Priority, 3600.f))
	{
		SaidOnce.Add(BarkId);
		return true;
	}
	return false;
}

void UTDGroundBarks::NotifyEncounterStarted(const ATDEncounterVolume* Encounter)
{
	Say(TDGroundBarkIds::Contact, ETDFootSpeaker::Wingman, ETDBarkPriority::High, 20.f);
	if (!Encounter)
	{
		return;
	}
	for (const TWeakObjectPtr<AActor>& M : Encounter->GetSpawned())
	{
		if (const ATDCrewCharacter* C = Cast<ATDCrewCharacter>(M.Get()))
		{
			const ETDCrewType T = C->CrewType;
			if (T == ETDCrewType::Trooper || T == ETDCrewType::Heavy || T == ETDCrewType::Drone || T == ETDCrewType::Warden)
			{
				SayOnce(FName(*FString::Printf(TEXT("Bark_Foot_%s"), *FTDCrewLibrary::TypeName(T).ToString())), ETDFootSpeaker::Kato, ETDBarkPriority::High);
			}
		}
	}
}

void UTDGroundBarks::NotifyEncounterCleared(const ATDEncounterVolume* Encounter)
{
	if (Bound.IsValid() && !Bound->IsDead())
	{
		Say(TDGroundBarkIds::LastOne, ETDFootSpeaker::Pilot, ETDBarkPriority::High, 15.f);
	}
}

void UTDGroundBarks::NotifyObjectiveDone() { Say(TDGroundBarkIds::Done, ETDFootSpeaker::Pilot, ETDBarkPriority::High, 10.f); }
void UTDGroundBarks::NotifyTakeOff() { Say(TDGroundBarkIds::TakeOff, ETDFootSpeaker::Pilot, ETDBarkPriority::Normal, 30.f); }
void UTDGroundBarks::NotifyConduitDown() { Say(TDGroundBarkIds::CoreShield, ETDFootSpeaker::Pilot, ETDBarkPriority::High, 6.f); }
void UTDGroundBarks::NotifyCoreExposed() { SayOnce(TDGroundBarkIds::CoreOpen, ETDFootSpeaker::Pilot, ETDBarkPriority::High); }

void UTDGroundBarks::Bind(ATDFirstPersonCharacter* Pilot)
{
	Bound = Pilot;
	LastHull01 = 1.f;
	bLowArmed = true;
	OnFootSeconds = 0.f;
	IdleSeconds = 0.f;
	if (!Pilot)
	{
		return;
	}
	if (UTDHealthComponent* H = Pilot->GetHealth())
	{
		H->OnDamaged.AddUniqueDynamic(this, &UTDGroundBarks::HandlePilotDamaged);
		LastHull01 = H->GetHull01();
	}
	if (UTDEnergyBladeComponent* B = Pilot->GetBlade())
	{
		B->OnDeflect.AddWeakLambda(this, [this](AActor*, bool bPerfect)
		{
			bPerfect ? (void)Say(TDGroundBarkIds::Perfect, ETDFootSpeaker::Pilot, ETDBarkPriority::High, 12.f)
				: (void)Say(TDGroundBarkIds::Deflect, ETDFootSpeaker::Pilot, ETDBarkPriority::Normal, 10.f);
		});
		B->OnGuardBroken.AddWeakLambda(this, [this]() { Say(TDGroundBarkIds::GuardBroken, ETDFootSpeaker::Pilot, ETDBarkPriority::Critical, 8.f); });
		B->OnBladeHit.AddWeakLambda(this, [this, B](AActor*, float, bool bKill)
		{
			if (B->GetBladeState() == ETDBladeState::AirSlam)
			{
				SlamHits = SlamWindowLeft > 0.f ? SlamHits + 1 : 1;
				SlamWindowLeft = 0.4f;
				if (SlamHits == 2) { Say(TDGroundBarkIds::Slam, ETDFootSpeaker::Pilot, ETDBarkPriority::Flavour, 20.f); }
			}
			if (bKill && (++Kills % 3) == 0) // one kill in three
			{
				Say(TDGroundBarkIds::Kill, ETDFootSpeaker::Pilot, ETDBarkPriority::Flavour, 10.f);
			}
		});
		B->OnSwing.AddWeakLambda(this, [this](ETDBladeState Move)
		{
			if (Move == ETDBladeState::DashStrike && FMath::FRand() < 0.34f)
			{
				Say(TDGroundBarkIds::Dash, ETDFootSpeaker::Pilot, ETDBarkPriority::Flavour, 20.f);
			}
		});
	}
}

void UTDGroundBarks::HandlePilotDamaged(UTDHealthComponent* Health, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage)
{
	if (!Health || Health->IsDead())
	{
		return;
	}
	const float H = Health->GetHull01();
	if (bLowArmed && H < 0.3f)
	{
		bLowArmed = false;
		Say(TDGroundBarkIds::LowHealth, ETDFootSpeaker::Pilot, ETDBarkPriority::Critical, 20.f);
	}
	else if (Health->MaxHull > 0.f && HullDamage >= 0.15f * Health->MaxHull)
	{
		Say(TDGroundBarkIds::Hurt, ETDFootSpeaker::Pilot, ETDBarkPriority::Normal, 6.f); // hit hard
	}
}

void UTDGroundBarks::TickBarks(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	// The on-foot pilot (bound once per body).
	ATDFirstPersonCharacter* Pilot = nullptr;
	for (TActorIterator<ATDFirstPersonCharacter> It(World); It; ++It) { if (!It->IsDead()) { Pilot = *It; break; } }
	if (Pilot != Bound.Get())
	{
		Bind(Pilot);
	}
	const ATDPlanetCourse* Course = nullptr;
	for (TActorIterator<ATDPlanetCourse> It(World); It; ++It) { Course = *It; break; }

	// Continue from a save: Command welcomes the squad back once (in whatever level the run resumes).
	if (const UTDGameInstance* GI = UTDGameInstance::Get(this); GI && GI->IsRunFromContinue() && World->GetTimeSeconds() > 2.f)
	{
		SayOnce(TDGroundBarkIds::Welcome, ETDFootSpeaker::Command, ETDBarkPriority::High);
	}
	if (!Pilot)
	{
		return;
	}
	OnFootSeconds += DeltaTime;
	SlamWindowLeft = FMath::Max(0.f, SlamWindowLeft - DeltaTime);
	UTDHealthComponent* Health = Pilot->GetHealth();

	// The first view on foot / the first step in the hangar.
	if (OnFootSeconds > 3.f && Course)
	{
		if (Course->IsInterior())
		{
			SayOnce(TDGroundBarkIds::Boarded, ETDFootSpeaker::Wingman, ETDBarkPriority::High);
		}
		else
		{
			// Bark_Planet_Arrive_Verdance / _Cinder: once per run, on the first time on foot there (never on a retake, a
			// reload or a staged start after it has played; T2's sweep, Nov 14).
			const FName Arrive = Course->PlanetId == TEXT("Verdance") ? TDGroundBarkIds::ArriveVerdance
				: Course->PlanetId == TEXT("Cinder") ? TDGroundBarkIds::ArriveCinder : NAME_None;
			UTDGameInstance* RunGI = UTDGameInstance::Get(this);
			if (!Arrive.IsNone() && (!RunGI || !RunGI->HasRunOnceBark(Arrive)) && SayOnce(Arrive, ETDFootSpeaker::Pilot, ETDBarkPriority::Flavour) && RunGI)
			{
				RunGI->ClaimRunOnceBark(Arrive);
			}
		}
	}
	// Health back (a checkpoint's refill or a pickup), and the low-health line re-arms once well above it.
	if (Health)
	{
		const float H = Health->GetHull01();
		if (H - LastHull01 >= 0.25f) { Say(TDGroundBarkIds::Heal, ETDFootSpeaker::Pilot, ETDBarkPriority::Flavour, 20.f); }
		if (H > 0.55f) { bLowArmed = true; }
		LastHull01 = H;
	}
	// Any fight running nearby?
	bool bFighting = false;
	for (TActorIterator<ATDEncounterVolume> It(World); It && !bFighting; ++It) { bFighting = It->IsActive(); }
	// Idle: 25 s standing still with nobody to fight.
	IdleSeconds = (!bFighting && Pilot->GetVelocity().Size2D() < 10.f) ? IdleSeconds + DeltaTime : 0.f;
	if (IdleSeconds > 25.f)
	{
		IdleSeconds = 0.f;
		Say(TDGroundBarkIds::Idle, ETDFootSpeaker::Wingman, ETDBarkPriority::Flavour, 60.f);
	}
	// Within 30 m of an objective marker (once per marker).
	for (AActor* M : Pilot->GetObjectiveMarkers())
	{
		if (M && !NearMarkers.Contains(M) && FVector::Dist2D(M->GetActorLocation(), Pilot->GetActorLocation()) < 3000.f)
		{
			NearMarkers.Add(M);
			Say(TDGroundBarkIds::ObjectiveNear, ETDFootSpeaker::Wingman, ETDBarkPriority::Normal, 20.f);
		}
	}
	// Doors: one opening near the pilot; pushing against a shut one.
	bool bPushing = false;
	for (TActorIterator<ATDDoor> It(World); It; ++It)
	{
		const float D = (float)FVector::Dist2D(It->GetActorLocation(), Pilot->GetActorLocation());
		const bool bWas = DoorWasOpen.FindRef(*It);
		DoorWasOpen.Add(*It, It->IsOpen());
		if (It->IsOpen() && !bWas && D < 1500.f)
		{
			Say(TDGroundBarkIds::DoorOpen, ETDFootSpeaker::Pilot, ETDBarkPriority::Normal, 15.f);
		}
		bPushing |= !It->IsOpen() && D < 260.f && Pilot->GetLastMovementInputVector().SizeSquared() > 0.1f;
	}
	DoorPushSeconds = bPushing ? DoorPushSeconds + DeltaTime : 0.f;
	if (DoorPushSeconds > 1.f)
	{
		DoorPushSeconds = -20.f; // not again straight away
		Say(TDGroundBarkIds::DoorLocked, ETDFootSpeaker::Pilot, ETDBarkPriority::Normal, 20.f);
	}
	// Near a magma's edge the first time (T7's Cinder play, Nov 14: 21 CUTs in the river): a wingman warns and the tip
	// line says how across. Placeholder words; the storyline's are Nicholas's.
	for (TActorIterator<ATDMagmaVolume> It(World); It; ++It)
	{
		if (!It->Box || MagmaWarned.Contains(*It))
		{
			continue;
		}
		const FBox B = It->Box->Bounds.GetBox();
		const FVector P = Pilot->GetActorLocation();
		const double Dx = FMath::Max3(B.Min.X - P.X, 0.0, P.X - B.Max.X), Dy = FMath::Max3(B.Min.Y - P.Y, 0.0, P.Y - B.Max.Y);
		if (FMath::Sqrt(Dx * Dx + Dy * Dy) < MagmaWarnDistance)
		{
			MagmaWarned.Add(*It);
			Say(TDGroundBarkIds::Hazard, ETDFootSpeaker::Wingman, ETDBarkPriority::Critical, 10.f);
			Pilot->ShowTip(NSLOCTEXT("TDGroundBarks", "MagmaEdge", "MAGMA KILLS · FOLLOW THE MARKER ACROSS"), 5.f);
		}
	}
	// In lava.
	for (TActorIterator<ATDMagmaVolume> It(World); It; ++It)
	{
		if (Pilot->IsOverlappingActor(*It))
		{
			Say(TDGroundBarkIds::Hazard, ETDFootSpeaker::Wingman, ETDBarkPriority::Critical, 10.f);
			break;
		}
	}
	// The escape clock (the Dominion): its start, 30 s and 10 s left.
	const float Left = Course ? Course->GetEscapeSecondsLeft() : -1.f;
	if (Left >= 0.f)
	{
		if (EscapeLastSeconds < 0.f) { Say(TDGroundBarkIds::EscapeStart, ETDFootSpeaker::Wingman, ETDBarkPriority::Critical, 5.f); }
		// N-093 (Nicholas's approved script): Moss, "Thirty seconds. Keep going."
		if (EscapeLastSeconds > 30.f && Left <= 30.f) { Say(TDGroundBarkIds::Escape30, ETDFootSpeaker::Moss, ETDBarkPriority::Critical, 5.f); }
		if (EscapeLastSeconds > 10.f && Left <= 10.f) { Say(TDGroundBarkIds::Escape10, ETDFootSpeaker::Wingman, ETDBarkPriority::Critical, 5.f); }
	}
	EscapeLastSeconds = Left;
}

void UTDGroundBarks::ResetForTests()
{
	Limiter.Reset();
	SaidOnce.Reset();
	Said.Reset();
	NearMarkers.Reset();
	MagmaWarned.Reset();
	Kills = 0;
}
