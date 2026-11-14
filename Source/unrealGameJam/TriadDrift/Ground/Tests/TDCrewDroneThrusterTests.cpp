// The crew drone plays its visuals' EngineTrail (the effects builder's NS_TD_Drone_Thruster) while alive, one per
// thruster socket or one at the hull origin, and stops it on death (Teammate 2, Lead's ask). Headless runs spawn no
// Niagara component (FApp::CanEverRender is false): there the test checks the path runs and the death stops cleanly.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/App.h"
#include "NiagaraComponent.h"
#include "UObject/StrongObjectPtr.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCrewDroneThrusters, "TriadDrift.Ground.Crew.DroneThrustersPlayAndStop", TD_TEST_FLAGS)
bool FTDCrewDroneThrusters::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TStrongObjectPtr<UTDEnemyVisuals> Look(NewObject<UTDEnemyVisuals>(GetTransientPackage()));
	Look->EngineTrail.VFX = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Drone_Thruster.NS_TD_Drone_Thruster")));
	TestFalse(TEXT("the thruster system exists"), Look->EngineTrail.VFX.LoadSynchronous() == nullptr);
	const FTransform Xf(FRotator::ZeroRotator, FVector(800.f, 0.f, 300.f));
	ATDCrewCharacter* D = W.GetWorld()->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("drone"), D))
	{
		return false;
	}
	D->CrewType = ETDCrewType::Drone;
	D->Visuals = Look.Get();
	D->FinishSpawning(Xf);
	W.Advance(.2f);
	const int32 Thrusters = D->GetDroneThrusters().Num();
	if (FApp::CanEverRender())
	{
		TestTrue(FString::Printf(TEXT("thrusters playing (%d)"), Thrusters), Thrusters >= 1);
		for (const UNiagaraComponent* FX : D->GetDroneThrusters())
		{
			TestTrue(TEXT("attached to the drone"), FX && FX->GetAttachParent() && FX->GetAttachParent()->GetOwner() == D);
		}
	}
	else
	{
		AddInfo(FString::Printf(TEXT("headless: %d thruster components (Niagara does not spawn without rendering)"), Thrusters));
	}
	if (UTDHealthComponent* H = UTDStatics::GetHealth(D))
	{
		H->Kill(nullptr);
	}
	W.Advance(.1f);
	TestEqual(TEXT("death stops them"), D->GetDroneThrusters().Num(), 0);

	// No EngineTrail: nothing, and nothing breaks.
	TStrongObjectPtr<UTDEnemyVisuals> Plain(NewObject<UTDEnemyVisuals>(GetTransientPackage()));
	ATDCrewCharacter* D2 = W.GetWorld()->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	D2->CrewType = ETDCrewType::Drone;
	D2->Visuals = Plain.Get();
	D2->FinishSpawning(Xf);
	W.Advance(.1f);
	TestEqual(TEXT("no EngineTrail: no thrusters"), D2->GetDroneThrusters().Num(), 0);
	return true;
}

#endif
