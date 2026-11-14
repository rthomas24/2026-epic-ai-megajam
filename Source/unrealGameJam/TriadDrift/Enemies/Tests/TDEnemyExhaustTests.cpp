// ENG-37 enemy exhaust hook: one EngineTrail effect per hull nozzle socket (T1's shared ATDShip::GetNozzleSockets), the hull
// origin when there are none, fed Niagara user floats Throttle (0..1) and Boost (0/1); all exhausts stop when the ship dies.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Enemies.Exhaust

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "NiagaraSystem.h"
#include "GameFramework/Controller.h"
#include "Misc/App.h"

namespace TDEnemyExhaustTest
{
	// The engineering enemy engine effect (Gameplay/VFX is always cooked): a real system, so components really spawn.
	const TCHAR* EngineFX = TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Engine_Enemy.NS_TD_Engine_Enemy");

	ATDEnemyShip* Spawn(FTDTestWorld& W, UStaticMesh* Hull, const FVector& Loc)
	{
		UTDEnemyVisuals* V = NewObject<UTDEnemyVisuals>(GetTransientPackage());
		V->EngineTrail.VFX = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(EngineFX));
		if (Hull) { V->Mesh = Hull; }
		FTDEnemySpec Spec = UTDEnemyLibrary::Get(TDEnemyIds::ShardFighter);
		Spec.Visuals = V;
		ATDEnemyShip* Ship = W.Spawn<ATDEnemyShip>(Loc);
		if (!Ship) { return nullptr; }
		if (AController* C = Ship->GetController()) { C->UnPossess(); C->Destroy(); }
		Ship->InitFromSpecData(Spec);
		return Ship;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemyExhaustNozzles, "TriadDrift.Enemies.Exhaust.OnePerNozzleSocketElseTheHull", TD_TEST_FLAGS)
bool FTDEnemyExhaustNozzles::RunTest(const FString&)
{
	if (!FPackageName::DoesPackageExist(FSoftObjectPath(TDEnemyExhaustTest::EngineFX).GetLongPackageName()))
	{
		AddWarning(TEXT("NS_TD_Engine_Enemy is missing: the exhaust count can't be checked"));
		return true;
	}
	FTDTestWorld W;
	// No nozzle sockets (the placeholder hull): one exhaust at the hull origin.
	ATDEnemyShip* Plain = TDEnemyExhaustTest::Spawn(W, nullptr, FVector(0.f, 0.f, 20000.f));
	if (!TestNotNull(TEXT("ship"), Plain)) { return false; }
	// Headless runs (null RHI) spawn no Niagara components, so components are only counted when this run can render.
	const bool bRenders = FApp::CanEverRender();
	TestEqual(TEXT("no sockets: one exhaust, at the hull origin"), Plain->GetExhaustNozzleCount(), 1);
	if (bRenders) { TestEqual(TEXT("no sockets: one effect"), Plain->GetExhaustEffectCount(), 1); }

	// A hull with "Engine" and Engine_0..Engine_2: four exhausts.
	UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), TEXT("TD_TestEnemyFourNozzleHull"));
	for (int32 i = -1; i < 3; ++i)
	{
		UStaticMeshSocket* S = NewObject<UStaticMeshSocket>(Mesh);
		S->SocketName = i < 0 ? FName(TEXT("Engine")) : FName(*FString::Printf(TEXT("Engine_%d"), i));
		S->RelativeLocation = FVector(-300.f, -150.f + 100.f * (i + 1), 0.f);
		Mesh->AddSocket(S);
	}
	ATDEnemyShip* Four = TDEnemyExhaustTest::Spawn(W, Mesh, FVector(0.f, 30000.f, 20000.f));
	if (!TestNotNull(TEXT("four-nozzle ship"), Four)) { return false; }
	TestTrue(TEXT("the art hull is the socketed mesh"), Four->GetHullMesh() && Four->GetHullMesh()->GetStaticMesh() == Mesh);
	TestEqual(TEXT("four nozzle sockets: four exhausts"), Four->GetExhaustNozzleCount(), 4);
	if (bRenders) { TestEqual(TEXT("four nozzle sockets: four effects"), Four->GetExhaustEffectCount(), 4); }

	// Throttle follows speed against the cruise cap; no boost at cruise.
	W.Advance(2.f);
	TestTrue(*FString::Printf(TEXT("throttle in 0..1 (%.2f)"), Four->GetExhaustThrottle01()), Four->GetExhaustThrottle01() >= 0.f && Four->GetExhaustThrottle01() <= 1.f);
	TestFalse(TEXT("cruise: boost 0"), Four->GetExhaustBoost());

	// Death stops every exhaust (the hulk tumbles dark).
	UTDStatics::ApplyTDDamage(Four, 1000000.f, ETDDamageKind::Explosive, nullptr, nullptr);
	W.Advance(0.1f);
	TestEqual(TEXT("dead: no exhausts"), Four->GetExhaustNozzleCount(), 0);
	TestEqual(TEXT("dead: no effects"), Four->GetExhaustEffectCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCarrierSocketMounts, "TriadDrift.Enemies.Carrier.PartsFollowTheHullSocketsElseTheSphere", TD_TEST_FLAGS)
bool FTDCarrierSocketMounts::RunTest(const FString&)
{
	FTDTestWorld W;
	auto SpawnCarrier = [&W](UStaticMesh* Hull, const FVector& Loc) -> ATDEnemyShip*
	{
		UTDEnemyVisuals* V = NewObject<UTDEnemyVisuals>(GetTransientPackage());
		if (Hull) { V->Mesh = Hull; }
		FTDEnemySpec Spec = UTDEnemyLibrary::Get(TDEnemyIds::HiveCarrier);
		Spec.Visuals = V;
		ATDEnemyShip* Ship = W.Spawn<ATDEnemyShip>(Loc);
		if (!Ship) { return nullptr; }
		if (AController* C = Ship->GetController()) { C->UnPossess(); C->Destroy(); }
		Ship->InitFromSpecData(Spec);
		return Ship;
	};
	// No sockets: today's placement, outside the blocking sphere.
	ATDEnemyShip* Old = SpawnCarrier(nullptr, FVector(0.f, 0.f, 20000.f));
	if (!TestNotNull(TEXT("carrier"), Old)) { return false; }
	TestFalse(TEXT("no sockets: sphere placement"), Old->HasSocketMountedParts());
	TestTrue(TEXT("bays spawned"), Old->GetBays().Num() > 0);
	for (const ATDCarrierBay* Bay : Old->GetBays())
	{
		TestTrue(TEXT("a bay sits outside the sphere"), FVector::Dist(Bay->GetActorLocation(), Old->GetActorLocation()) > Old->GetSpec().Radius);
	}

	// A slim hull with Bay_0..2 and Turret_0..3: every part on its socket.
	UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), TEXT("TD_TestSocketCarrier"));
	auto Socket = [Mesh](const TCHAR* Name, const FVector& At, const FRotator& Facing)
	{
		UStaticMeshSocket* S = NewObject<UStaticMeshSocket>(Mesh);
		S->SocketName = Name;
		S->RelativeLocation = At;
		S->RelativeRotation = Facing;
		Mesh->AddSocket(S);
	};
	Socket(TEXT("Bay_0"), FVector(0.f, -1100.f, 0.f), FRotator(0.f, -90.f, 0.f));
	Socket(TEXT("Bay_1"), FVector(0.f, 1100.f, 0.f), FRotator(0.f, 90.f, 0.f));
	Socket(TEXT("Bay_2"), FVector(-600.f, 0.f, 880.f), FRotator(90.f, 0.f, 0.f));
	for (int32 i = 0; i < 4; ++i) { Socket(*FString::Printf(TEXT("Turret_%d"), i), FVector(-1500.f + i * 1000.f, (i % 2) ? 700.f : -700.f, 850.f), FRotator(60.f, 0.f, 0.f)); }
	ATDEnemyShip* New = SpawnCarrier(Mesh, FVector(0.f, 60000.f, 20000.f));
	if (!TestNotNull(TEXT("socket carrier"), New)) { return false; }
	TestTrue(TEXT("socket placement"), New->HasSocketMountedParts());
	const UStaticMeshComponent* Hull = New->GetHullMesh();
	// Each part stands on its socket's outward +X line, its half-extent plus the clearance out (this test mesh has no
	// collision, so no socket counts as buried), facing out along the socket.
	auto OnSocket = [&](const AActor* Part, FName Name, float Half) -> bool
	{
		const FTransform S = Hull->GetSocketTransform(Name);
		const FVector Want = S.GetLocation() + S.GetUnitAxis(EAxis::X) * (Half + New->CarrierPartClearance);
		return Part->GetActorLocation().Equals(Want, 1.f) && FVector::DotProduct(Part->GetActorForwardVector(), S.GetUnitAxis(EAxis::X)) > 0.999f;
	};
	for (int32 i = 0; i < New->GetBays().Num(); ++i)
	{
		const FName Name(*FString::Printf(TEXT("Bay_%d"), i));
		TestTrue(*FString::Printf(TEXT("bay %d on its socket"), i), OnSocket(New->GetBays()[i], Name, New->GetBays()[i]->GetHalfExtent()));
	}
	for (int32 i = 0; i < New->GetTurrets().Num(); ++i)
	{
		const FName Name(*FString::Printf(TEXT("Turret_%d"), i));
		TestTrue(*FString::Printf(TEXT("turret %d on its socket"), i), OnSocket(New->GetTurrets()[i], Name, 120.f));
	}
	// The parts move with the hull.
	New->SetActorLocation(New->GetActorLocation() + FVector(5000.f, 0.f, 0.f));
	TestTrue(TEXT("parts ride along"), New->GetBays().Num() == 0 || OnSocket(New->GetBays()[0], TEXT("Bay_0"), New->GetBays()[0]->GetHalfExtent()));
	return true;
}

#endif
