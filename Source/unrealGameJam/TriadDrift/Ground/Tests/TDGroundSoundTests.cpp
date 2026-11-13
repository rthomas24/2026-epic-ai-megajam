// Triad Drift - ENG-35 on-foot sound hooks: every hook fires at the right moment (checked through the recorded events,
// so no audio device is needed) and the slots point at the engineering fallback set. Owner: Teammate 5.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "Sound/SoundBase.h"
#include "TriadDrift/Flow/TDGameInstance.h"

namespace TDGroundSoundTest
{
	static int32 Count(const TArray<FName>& Events, const TCHAR* Name)
	{
		int32 N = 0;
		for (const FName& E : Events) { N += E == FName(Name) ? 1 : 0; }
		return N;
	}

	static ATDCrewCharacter* SpawnCrew(FTDTestWorld& W, ETDCrewType Type, const FVector& At, const FRotator& Facing = FRotator::ZeroRotator)
	{
		const FTransform Xf(Facing, At);
		ATDCrewCharacter* C = W.GetWorld()->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (C)
		{
			C->CrewType = Type;
			C->FinishSpawning(Xf);
		}
		return C;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPilotSounds, "TriadDrift.Ground.Sound.PilotStepsJumpLandDashHurt", TD_TEST_FLAGS)
bool FTDPilotSounds::RunTest(const FString& Parameters)
{
	using namespace TDGroundSoundTest;
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	if (!TestNotNull(TEXT("pilot"), P)) { return false; }
	TestEqual(TEXT("soil steps from the fallback set"), P->StepSoilSound.ToSoftObjectPath().ToString(), FString(TEXT("/Game/TriadDrift/Gameplay/Audio/Foot/MSS_Foot_Step_Soil.MSS_Foot_Step_Soil")));
	TestEqual(TEXT("hurt from the fallback set"), P->HurtSound.ToSoftObjectPath().ToString(), FString(TEXT("/Game/TriadDrift/Gameplay/Audio/Foot/MSS_Player_Hurt.MSS_Player_Hurt")));
	TestFalse(TEXT("not a Cinder map: soil"), P->bRockyGround);
	W.Advance(0.3f);

	// Running 2 s at 650 cm/s is ~1300 cm: about 7 steps at a 170 cm stride (the first starts from a standstill).
	P->SetMoveInput(FVector2D(0.f, 1.f));
	W.Advance(2.f);
	P->SetMoveInput(FVector2D::ZeroVector);
	const int32 Steps = Count(P->GetSoundEvents(), TEXT("StepSoil"));
	TestTrue(*FString::Printf(TEXT("steps by stride while running (%d)"), Steps), Steps >= 5 && Steps <= 9);
	W.Advance(0.5f);
	const int32 Standing = Count(P->GetSoundEvents(), TEXT("StepSoil"));
	W.Advance(1.f);
	TestEqual(TEXT("no steps standing still"), Count(P->GetSoundEvents(), TEXT("StepSoil")), Standing);

	TestTrue(TEXT("jump"), P->TryJump());
	TestEqual(TEXT("jump sound"), Count(P->GetSoundEvents(), TEXT("Jump")), 1);
	W.Advance(1.5f);
	TestTrue(TEXT("landing sound"), Count(P->GetSoundEvents(), TEXT("Land")) >= 1);

	TestTrue(TEXT("dash"), P->TryDash());
	TestEqual(TEXT("dash sound"), Count(P->GetSoundEvents(), TEXT("Dash")), 1);
	W.Advance(1.f);

	// A burst of hits is one hurt sound; a hit later is another.
	UTDStatics::ApplyTDDamage(P, 5.f, ETDDamageKind::Energy, nullptr, nullptr);
	UTDStatics::ApplyTDDamage(P, 5.f, ETDDamageKind::Energy, nullptr, nullptr);
	UTDStatics::ApplyTDDamage(P, 5.f, ETDDamageKind::Energy, nullptr, nullptr);
	TestEqual(TEXT("one hurt for a burst"), Count(P->GetSoundEvents(), TEXT("Hurt")), 1);
	W.Advance(0.4f);
	UTDStatics::ApplyTDDamage(P, 5.f, ETDDamageKind::Energy, nullptr, nullptr);
	TestEqual(TEXT("a later hit hurts again"), Count(P->GetSoundEvents(), TEXT("Hurt")), 2);

	// On rock (Cinder): rock steps.
	P->bRockyGround = true;
	P->SetMoveInput(FVector2D(0.f, 1.f));
	W.Advance(1.f);
	P->SetMoveInput(FVector2D::ZeroVector);
	TestTrue(TEXT("rock steps on Cinder"), Count(P->GetSoundEvents(), TEXT("StepRock")) >= 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewSounds, "TriadDrift.Ground.Sound.CrewRifleShotAndDroneHum", TD_TEST_FLAGS)
bool FTDCrewSounds::RunTest(const FString& Parameters)
{
	using namespace TDGroundSoundTest;
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W);
	if (!TestNotNull(TEXT("pilot"), P)) { return false; }
	ATDCrewCharacter* Trooper = SpawnCrew(W, ETDCrewType::Trooper, FVector(1000.f, 0.f, 95.f), FRotator(0.f, 180.f, 0.f));
	if (!TestNotNull(TEXT("trooper"), Trooper)) { return false; }
	TestEqual(TEXT("rifle shot from the fallback set"), Trooper->RifleShotSound.ToSoftObjectPath().ToString(), FString(TEXT("/Game/TriadDrift/Gameplay/Audio/Crew/MSS_Crew_RifleShot.MSS_Crew_RifleShot")));
	W.Advance(5.f);
	TestTrue(*FString::Printf(TEXT("the trooper's shots sound (%d)"), Count(Trooper->GetSoundEvents(), TEXT("RifleShot"))), Count(Trooper->GetSoundEvents(), TEXT("RifleShot")) >= 1);
	TestEqual(TEXT("a trooper does not hum"), Count(Trooper->GetSoundEvents(), TEXT("DroneHum")), 0);

	ATDCrewCharacter* Drone = SpawnCrew(W, ETDCrewType::Drone, FVector(0.f, 1200.f, 300.f));
	if (!TestNotNull(TEXT("drone"), Drone)) { return false; }
	TestEqual(TEXT("the drone hums from the start"), Count(Drone->GetSoundEvents(), TEXT("DroneHum")), 1);
	UTDStatics::ApplyTDDamage(Drone, 100000.f, ETDDamageKind::Melee, P, P);
	TestEqual(TEXT("and stops when it dies"), Count(Drone->GetSoundEvents(), TEXT("DroneHumStop")), 1);
	TestNull(TEXT("no hum left playing"), Drone->GetDroneHum());
	return true;
}

// The director's objective-complete sting (DA_UI_Style::ObjectiveComplete) plays once per objective, never on a repeat.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDObjectiveSting, "TriadDrift.Ground.Sound.ObjectiveCompleteStingOncePerObjective", TD_TEST_FLAGS)
bool FTDObjectiveSting::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("game instance"), GI)) { return false; }
	const int32 Before = GI->GetObjectiveCompleteSounds();
	GI->CompleteObjective(TEXT("Obj_SoundTest"), ETDAbilityId::None, ETDShipId::Rift);
	TestEqual(TEXT("a completed objective stings"), GI->GetObjectiveCompleteSounds(), Before + 1);
	GI->CompleteObjective(TEXT("Obj_SoundTest"), ETDAbilityId::None, ETDShipId::Rift);
	TestEqual(TEXT("not again for the same objective"), GI->GetObjectiveCompleteSounds(), Before + 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
