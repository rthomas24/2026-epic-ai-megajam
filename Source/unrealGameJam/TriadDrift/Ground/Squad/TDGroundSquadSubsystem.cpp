// Triad Drift - the on-foot squad (#209). Owner: ground squad builder.

#include "TriadDrift/Ground/Squad/TDGroundSquadSubsystem.h"
#include "TriadDrift/Ground/Squad/TDGroundAlly.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/Crew/TDFutureCrew.h"
#include "TriadDrift/Ground/TDGroundBarks.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarTDGroundAllies(TEXT("TD.Ground.Allies"), 1,
	TEXT("1: the two other pilots walk and fight beside the player on foot (#209); 0: the player is alone, as before."));
static TAutoConsoleVariable<float> CVarTDGroundSquadScale(TEXT("td.Ground.SquadScale"), 1.6f,
	TEXT("On-foot encounter size multiplier while the allies are on (enemy count, rounded; same types, same places)."));
static bool GTDGroundSquadForcedForTests = false;

static FAutoConsoleCommandWithWorldAndArgs CmdTDGroundAlliesDump(TEXT("TD.Ground.AlliesDump"),
	TEXT("Log the on-foot allies: pilot, state, health, target, distance to the player, counters."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		if (const UTDGroundSquadSubsystem* Squad = UTDGroundSquadSubsystem::Get(World))
		{
			Squad->DumpToLog();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs CmdTDGroundAllyDown(TEXT("TD.Ground.AllyDown"),
	TEXT("TD.Ground.AllyDown <0|1>: put that ally down now (it gets up by itself)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const UTDGroundSquadSubsystem* Squad = UTDGroundSquadSubsystem::Get(World);
		ATDGroundAlly* Ally = Squad ? Squad->GetAlly(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0) : nullptr;
		if (Ally)
		{
			Ally->GoDown();
		}
		else
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Ground.AllyDown: no such ally (on foot, with TD.Ground.Allies 1?)"));
		}
	}));

UTDGroundSquadSubsystem* UTDGroundSquadSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UTDGroundSquadSubsystem>() : nullptr;
}

bool UTDGroundSquadSubsystem::AreAlliesEnabled()
{
	return CVarTDGroundAllies.GetValueOnGameThread() != 0 && (!GIsAutomationTesting || GTDGroundSquadForcedForTests);
}

float UTDGroundSquadSubsystem::GetEncounterScale()
{
	return AreAlliesEnabled() ? FMath::Clamp(CVarTDGroundSquadScale.GetValueOnGameThread(), 0.5f, 3.f) : 1.f;
}

void UTDGroundSquadSubsystem::SetForcedForTests(bool bForce)
{
	GTDGroundSquadForcedForTests = bForce;
}

bool UTDGroundSquadSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTDGroundSquadSubsystem::Deinitialize()
{
	Allies.Reset();
	Super::Deinitialize();
}

void UTDGroundSquadSubsystem::GetAllyStatus(TArray<FTDGroundAllyStatus>& OutAllies) const
{
	OutAllies.Reset();
	for (ATDGroundAlly* Ally : Allies)
	{
		if (!IsValid(Ally))
		{
			continue;
		}
		FTDGroundAllyStatus& S = OutAllies.AddDefaulted_GetRef();
		S.Pilot = Ally->GetPilot();
		S.Ship = Ally->GetShipId();
		S.Color = Ally->GetSquadColor();
		S.Health01 = Ally->GetHealth01();
		S.bDown = Ally->IsDown();
		S.GetUp01 = Ally->GetDown01();
		S.bFighting = Ally->GetFightTarget() != nullptr;
		S.Actor = Ally;
	}
}

bool UTDGroundSquadSubsystem::IsTargetTaken(const AActor* Target, const ATDGroundAlly* Asker) const
{
	for (const ATDGroundAlly* Ally : Allies)
	{
		if (IsValid(Ally) && Ally != Asker && Ally->GetFightTarget() == Target)
		{
			return true;
		}
	}
	return false;
}

FVector UTDGroundSquadSubsystem::GetFormationSlot(int32 Slot) const
{
	const ATDFirstPersonCharacter* L = Leader.Get();
	const UWorld* World = GetWorld();
	if (!L || !World)
	{
		return FVector::ZeroVector;
	}
	const FRotator Frame(0.f, FormationYaw, 0.f);
	const FVector From = L->GetActorLocation();
	FVector Want = From + Frame.RotateVector(FVector(-SlotBack, Slot == 0 ? -SlotSide : SlotSide, 0.f));
	// Narrow ground (a corridor under about 4 m, a doorway, a wall on either side): single file behind the player.
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDGroundSquadSlot), false, L);
	const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
	const FVector SideReach = Frame.RotateVector(FVector(0.f, SingleFileHalfWidth, 0.f));
	const bool bNarrow = World->LineTraceSingleByObjectType(Hit, From, From - SideReach, Solid, Params)
		|| World->LineTraceSingleByObjectType(Hit, From, From + SideReach, Solid, Params)
		|| World->LineTraceSingleByObjectType(Hit, From, Want, Solid, Params);
	if (bNarrow)
	{
		Want = From + Frame.RotateVector(FVector(-(SingleFileBack + Slot * SingleFileGap), 0.f, 0.f));
		if (World->LineTraceSingleByObjectType(Hit, From, Want, Solid, Params))
		{
			Want = Hit.ImpactPoint + (From - Want).GetSafeNormal() * 60.f; // a wall close behind: as far back as there is room
		}
	}
	return Want;
}

FVector UTDGroundSquadSubsystem::FindSpotNearLeader(int32 Slot) const
{
	const ATDFirstPersonCharacter* L = Leader.Get();
	const UWorld* World = GetWorld();
	if (!L || !World)
	{
		return FVector::ZeroVector;
	}
	const float Sign = Slot == 0 ? -1.f : 1.f;
	const FVector From = L->GetActorLocation();
	const FRotator Frame(0.f, FormationYaw, 0.f);
	const FVector Offsets[] = {
		FVector(-SlotBack, Sign * SlotSide, 0.f), FVector(-220.f, Sign * 150.f, 0.f), FVector(-300.f - Slot * 120.f, 0.f, 0.f),
		FVector(-130.f, Sign * 110.f, 0.f), FVector(0.f, Sign * 140.f, 0.f), FVector(-140.f - Slot * 100.f, 0.f, 0.f) };
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDGroundSquadSpot), false, L);
	for (const ATDGroundAlly* Ally : Allies)
	{
		Params.AddIgnoredActor(Ally);
	}
	const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
	const float HalfHeight = 90.f;
	for (const FVector& Off : Offsets)
	{
		const FVector At = From + Frame.RotateVector(Off);
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, From, At, Solid, Params))
		{
			continue; // through a wall
		}
		if (!World->LineTraceSingleByObjectType(Hit, At + FVector(0.f, 0.f, 60.f), At - FVector(0.f, 0.f, 350.f), Solid, Params))
		{
			continue; // nothing to stand on
		}
		const FVector Stand = Hit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 3.f);
		if (!World->OverlapAnyTestByObjectType(Stand + FVector(0.f, 0.f, 8.f), FQuat::Identity, Solid, FCollisionShape::MakeCapsule(30.f, HalfHeight - 6.f), Params))
		{
			return Stand;
		}
	}
	return From; // nowhere clear: on the player (they pass through each other)
}

void UTDGroundSquadSubsystem::SpawnAllies(ATDFirstPersonCharacter* InLeader)
{
	UWorld* World = GetWorld();
	if (!World || !InLeader)
	{
		return;
	}
	Leader = InLeader;
	LeaderPilot = ATDFirstPersonCharacter::GetOnFootPilotName(this);
	FormationYaw = InLeader->GetControlRotation().Yaw;
	const TArray<FName> Others = TDFutureCrew::OthersThan(LeaderPilot);
	for (int32 i = 0; i < Others.Num(); ++i)
	{
		const FTransform At(FRotator(0.f, FormationYaw, 0.f), FindSpotNearLeader(i));
		ATDGroundAlly* Ally = World->SpawnActorDeferred<ATDGroundAlly>(ATDGroundAlly::StaticClass(), At, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Ally)
		{
			continue;
		}
		Ally->SetPilot(Others[i], i);
		Ally->FinishSpawning(At);
		// Friends never body-block each other or the player (doorways, corridors); traces and shots still see them.
		Ally->GetCapsuleComponent()->IgnoreActorWhenMoving(InLeader, true);
		InLeader->GetCapsuleComponent()->IgnoreActorWhenMoving(Ally, true);
		for (ATDGroundAlly* Other : Allies)
		{
			Other->GetCapsuleComponent()->IgnoreActorWhenMoving(Ally, true);
			Ally->GetCapsuleComponent()->IgnoreActorWhenMoving(Other, true);
		}
		Allies.Add(Ally);
		NotifyAllyEvent(Ally, ETDGroundSquadEvent::Joined);
	}
	bBoarding = false;
	bLeaderWasDead = InLeader->IsDead();
	LastLeaderLocation = InLeader->GetActorLocation();
	UE_LOG(LogTriadDrift, Display, TEXT("GroundSquad: %s is on foot; %d allies beside them (%s, %s); encounter scale x%.2f"),
		*LeaderPilot.ToString(), Allies.Num(), Others.Num() > 0 ? *Others[0].ToString() : TEXT("-"), Others.Num() > 1 ? *Others[1].ToString() : TEXT("-"), GetEncounterScale());
}

void UTDGroundSquadSubsystem::DespawnAllies()
{
	if (Allies.Num() == 0)
	{
		return;
	}
	ATDFirstPersonCharacter* L = Leader.Get();
	for (ATDGroundAlly* Ally : Allies)
	{
		if (IsValid(Ally))
		{
			if (bBoarding)
			{
				NotifyAllyEvent(Ally, ETDGroundSquadEvent::Boarded);
			}
			if (L)
			{
				L->GetCapsuleComponent()->IgnoreActorWhenMoving(Ally, false);
			}
			Ally->Destroy();
		}
	}
	Allies.Reset();
	bBoarding = false;
	UE_LOG(LogTriadDrift, Display, TEXT("GroundSquad: allies gone (the pilot boarded, or allies are off)"));
}

void UTDGroundSquadSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	ATDFirstPersonCharacter* Pilot = PC ? Cast<ATDFirstPersonCharacter>(PC->GetPawn()) : nullptr;
	const bool bWanted = AreAlliesEnabled() && Pilot && !Pilot->IsHidden();
	if (!bWanted)
	{
		DespawnAllies();
		Leader = nullptr;
		return;
	}
	Allies.RemoveAll([](const ATDGroundAlly* A) { return !IsValid(A); });
	if (Allies.Num() > 0 && (Leader.Get() != Pilot || LeaderPilot != ATDFirstPersonCharacter::GetOnFootPilotName(this)))
	{
		DespawnAllies(); // another pilot is on foot now: the other two change with them
	}
	Leader = Pilot;
	if (Allies.Num() == 0)
	{
		// They step out once the pilot is down from the hatch.
		if (!Pilot->IsInShipTransition() && !Pilot->IsDead())
		{
			SpawnAllies(Pilot);
		}
		return;
	}
	// The formation frame follows the view slowly.
	FormationYaw = FMath::FixedTurn(FormationYaw, Pilot->GetControlRotation().Yaw, FormationTurnRate * DeltaTime);

	// Climbing back into the ship: the allies run in after the pilot (they are removed when the pilot is stowed).
	const bool bTransition = Pilot->IsInShipTransition();
	if (bTransition != bBoarding)
	{
		bBoarding = bTransition;
		for (ATDGroundAlly* Ally : Allies)
		{
			if (bBoarding) { Ally->BeginBoarding(); } else { Ally->EndBoarding(); }
		}
	}
	// A new take (alive again after dying), or the pilot was put somewhere else (checkpoint, respawn): the allies
	// reset with the scene, standing beside the pilot at full health.
	const bool bDead = Pilot->IsDead();
	const bool bJumped = FVector::Dist(Pilot->GetActorLocation(), LastLeaderLocation) > 1500.f;
	if ((bLeaderWasDead && !bDead) || bJumped)
	{
		FormationYaw = Pilot->GetControlRotation().Yaw;
		for (ATDGroundAlly* Ally : Allies)
		{
			Ally->ResetBesideLeader();
		}
		UE_LOG(LogTriadDrift, Display, TEXT("GroundSquad: allies reset beside the pilot (%s)"), bJumped ? TEXT("the pilot was moved") : TEXT("a new take"));
	}
	bLeaderWasDead = bDead;
	LastLeaderLocation = Pilot->GetActorLocation();
}

void UTDGroundSquadSubsystem::NotifyAllyEvent(ATDGroundAlly* Ally, ETDGroundSquadEvent Event, AActor* Other)
{
	if (!IsValid(Ally))
	{
		return;
	}
	FTDGroundSquadEventInfo Info;
	Info.Event = Event;
	Info.PossessedPilot = LeaderPilot;
	Info.Subject = Ally->GetPilot();
	Info.bSubjectIsFollower = true;
	Info.bDown = Ally->IsDown();
	Info.SubjectActor = Ally;
	Info.Other = Other;
	static const TCHAR* Names[] = { TEXT("Joined"), TEXT("Engaged"), TEXT("Kill"), TEXT("Down"), TEXT("Recovered"), TEXT("Boarded") };
	UE_LOG(LogTriadDrift, Display, TEXT("GroundSquad: event %s subject=%s (follower) possessed=%s down=%d other=%s"),
		Names[(int32)Event], *Info.Subject.ToString(), *Info.PossessedPilot.ToString(), Info.bDown ? 1 : 0, Other ? *Other->GetName() : TEXT("-"));
	OnSquadEvent.Broadcast(Info);

	// Existing dialogs.md ids only, said by the follower it is about (never "a wingman" picked by order). Going down
	// says nothing: no existing line fits a temporary knock-down without reading as a loss.
	UTDGroundBarks* Barks = UTDGroundBarks::Get(this);
	if (!Barks)
	{
		return;
	}
	const ETDFootSpeaker Who = Info.Subject == TEXT("Kato") ? ETDFootSpeaker::Kato : Info.Subject == TEXT("Moss") ? ETDFootSpeaker::Moss : ETDFootSpeaker::Wren;
	if (Event == ETDGroundSquadEvent::Kill)
	{
		Barks->Say(TEXT("Bark_SquadWingmanKill"), Who, ETDBarkPriority::Normal, 12.f); // "a wingman destroys an enemy", that wingman
	}
	else if (Event == ETDGroundSquadEvent::Recovered)
	{
		Barks->Say(TEXT("Bark_SquadRespawn"), Who, ETDBarkPriority::High, 10.f); // "a downed pilot is back", that pilot
	}
}

void UTDGroundSquadSubsystem::DumpToLog() const
{
	const ATDFirstPersonCharacter* L = Leader.Get();
	UE_LOG(LogTriadDrift, Display, TEXT("GroundSquad: enabled=%d leader=%s (%s) allies=%d scale=x%.2f formationYaw=%.0f"),
		AreAlliesEnabled() ? 1 : 0, L ? *L->GetName() : TEXT("none"), *LeaderPilot.ToString(), Allies.Num(), GetEncounterScale(), FormationYaw);
	static const TCHAR* States[] = { TEXT("Follow"), TEXT("Fight"), TEXT("Down"), TEXT("Boarding") };
	for (const ATDGroundAlly* A : Allies)
	{
		if (!IsValid(A))
		{
			continue;
		}
		const FLinearColor C = A->GetSquadColor();
		const AActor* T = A->GetFightTarget();
		UE_LOG(LogTriadDrift, Display, TEXT("GroundSquad:   [%d] %s state=%s health=%.2f down01=%.2f target=%s distToPilot=%.0f distToSlot=%.0f speed=%.0f colour=(%.2f %.2f %.2f) blade=%d hits=%d kills=%d deflects=%d downs=%d teleports=%d loc=%s"),
			A->GetSlot(), *A->GetPilot().ToString(), States[(int32)A->GetState()], A->GetHealth01(), A->GetDown01(), T ? *T->GetName() : TEXT("none"),
			L ? (float)FVector::Dist(L->GetActorLocation(), A->GetActorLocation()) : -1.f, (float)FVector::Dist2D(GetFormationSlot(A->GetSlot()), A->GetActorLocation()),
			(float)A->GetVelocity().Size2D(), C.R, C.G, C.B, A->GetBladeBeam() ? 1 : 0, A->GetSwingsLanded(), A->GetKills(), A->GetDeflects(), A->GetTimesDown(), A->GetTeleports(),
			*A->GetActorLocation().ToCompactString());
	}
}
