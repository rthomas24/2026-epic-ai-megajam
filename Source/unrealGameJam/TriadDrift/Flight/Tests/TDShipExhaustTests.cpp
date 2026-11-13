// ENG-37 exhaust hook: one exhaust per nozzle socket, fed Niagara user parameters Throttle (0..1) and Boost (0/1).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight.Exhaust

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDShipExhaustNozzles, "TriadDrift.Flight.Exhaust.OnePerNozzleWithThrottleAndBoost", TD_TEST_FLAGS)
bool FTDShipExhaustNozzles::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
	if (!TestNotNull(TEXT("ship"), Ship) || !Ship->GetHullMesh()) { return false; }

	// No nozzle sockets: the single engine attach.
	TArray<TPair<USceneComponent*, FName>> Nozzles;
	Ship->GetEngineNozzles(Nozzles);
	TestEqual(TEXT("placeholder: one exhaust"), Nozzles.Num(), 1);

	// A hull with Engine_0..Engine_3 (a four-nozzle Bastion-style mesh): four exhausts, in order.
	UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), TEXT("TD_TestFourNozzleHull"));
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshSocket* S = NewObject<UStaticMeshSocket>(Mesh);
		S->SocketName = FName(*FString::Printf(TEXT("Engine_%d"), i));
		S->RelativeLocation = FVector(-300.f, -150.f + 100.f * i, 0.f);
		Mesh->AddSocket(S);
	}
	Ship->GetHullMesh()->SetStaticMesh(Mesh);
	Ship->GetEngineNozzles(Nozzles);
	TestEqual(TEXT("four nozzle sockets: four exhausts"), Nozzles.Num(), 4);
	TestTrue(TEXT("in socket order"), Nozzles.Num() == 4 && Nozzles[0].Value == FName(TEXT("Engine_0")) && Nozzles[3].Value == FName(TEXT("Engine_3")));
	TArray<FName> Sockets;
	ATDShip::GetNozzleSockets(Ship->GetHullMesh(), Sockets);
	TestEqual(TEXT("the shared static scan finds the same four"), Sockets.Num(), 4);
	ATDShip::GetNozzleSockets(nullptr, Sockets);
	TestEqual(TEXT("... and nothing for no mesh"), Sockets.Num(), 0);

	// The parameters the exhaust is fed: Throttle 0..1 against the cruise cap, Boost while boosting.
	Ship->SetPlayerControlled(true);
	FTDFlightInput In;
	In.Throttle = -1.f;
	Ship->SetFlightInput(In);
	W.Advance(3.f);
	const float Slow = Ship->GetExhaustThrottle01();
	In.Throttle = 1.f;
	Ship->SetFlightInput(In);
	W.Advance(3.f);
	TestTrue(FString::Printf(TEXT("throttle rises with speed (%.2f -> %.2f)"), Slow, Ship->GetExhaustThrottle01()), Ship->GetExhaustThrottle01() > Slow + 0.3f && Ship->GetExhaustThrottle01() <= 1.f);
	TestFalse(TEXT("cruise: boost 0"), Ship->GetExhaustBoost());
	In.bBoost = true;
	Ship->SetFlightInput(In);
	W.Advance(0.5f);
	TestTrue(TEXT("boosting: boost 1"), Ship->GetExhaustBoost());
	TestEqual(TEXT("boosting: throttle full"), Ship->GetExhaustThrottle01(), 1.f, 0.01f);

	// A system that lacks the parameters (or none at all) is left alone.
	TestFalse(TEXT("no component: nothing set"), ATDShip::SetExhaustParamIfExposed(nullptr, TEXT("Throttle"), 1.f));
	TestEqual(TEXT("no EngineTrail VFX in tests: no effect components"), Ship->GetExhaustEffectCount(), 0);
	return true;
}

#endif
