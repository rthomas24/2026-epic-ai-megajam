// Automation tests for ship presentation hooks: modular visual parts (N-001) and flight cues.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight.Visuals

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

namespace TDShipVisualsTest
{
	/** Adds a socket to an engine mesh for the duration of a test (in memory only, never saved). */
	struct FScopedSocket
	{
		UStaticMesh* Mesh = nullptr;
		UStaticMeshSocket* Socket = nullptr;
		FScopedSocket(UStaticMesh* InMesh, FName Name, const FVector& Location, const FRotator& Rotation)
			: Mesh(InMesh)
		{
			if (Mesh)
			{
				Socket = NewObject<UStaticMeshSocket>(Mesh);
				Socket->SocketName = Name;
				Socket->RelativeLocation = Location;
				Socket->RelativeRotation = Rotation;
				Mesh->Sockets.Add(Socket);
			}
		}
		~FScopedSocket()
		{
			if (Mesh && Socket)
			{
				Mesh->Sockets.Remove(Socket);
			}
		}
	};

	UStaticMesh* LoadCube() { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")); }
	UStaticMesh* LoadSphere() { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")); }

	FTDShipVisualPart MakePart(FName Name, UStaticMesh* Mesh, FName Socket, const FTransform& Relative)
	{
		FTDShipVisualPart Part;
		Part.PartName = Name;
		Part.Mesh = Mesh;
		Part.ParentSocket = Socket;
		Part.RelativeTransform = Relative;
		return Part;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDShipVisualParts, "TriadDrift.Flight.Visuals.ModularPartsAttachAndTearDown", TD_TEST_FLAGS)
bool FTDShipVisualParts::RunTest(const FString& Parameters)
{
	UStaticMesh* Cube = TDShipVisualsTest::LoadCube();
	UStaticMesh* Sphere = TDShipVisualsTest::LoadSphere();
	if (!TestNotNull(TEXT("engine cube"), Cube) || !TestNotNull(TEXT("engine sphere"), Sphere)) { return false; }
	TDShipVisualsTest::FScopedSocket PodSocket(Cube, TEXT("Pod_L"), FVector(10.f, -40.f, 5.f), FRotator(0.f, 30.f, 0.f));

	UTDShipVisuals* Visuals = NewObject<UTDShipVisuals>();
	Visuals->Mesh = Cube;
	// 10 parts on the ship root plus 1 on a socket of the primary mesh = 11 (Lancer-style assembly).
	for (int32 i = 0; i < 10; ++i)
	{
		Visuals->Parts.Add(TDShipVisualsTest::MakePart(*FString::Printf(TEXT("Part_%d"), i), Sphere, NAME_None,
			FTransform(FRotator(0.f, 10.f * i, 0.f), FVector(-100.f * i, 900.f, 50.f), FVector(0.5f))));
	}
	Visuals->Parts.Add(TDShipVisualsTest::MakePart(TEXT("Engine"), Sphere, TEXT("Pod_L"), FTransform(FVector(20.f, 0.f, 0.f))));

	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(1000.f, 2000.f, 300.f), FRotator(0.f, 45.f, 0.f));
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	FTDFlightInput Idle;
	Idle.Throttle = -1.f;
	Ship->SetFlightInput(Idle);
	Ship->GetShipMovement()->SetForwardSpeedImmediate(0.f);
	Ship->SetFlightParked(true); // hold still so world transforms are exact
	Ship->SetShipVisuals(Visuals);
	W.Tick();

	TestTrue(TEXT("real art active"), Ship->HasRealArt());
	TestEqual(TEXT("11 parts built"), Ship->GetVisualParts().Num(), 11);
	TestTrue(TEXT("hull mesh is the primary"), Ship->GetHullMesh()->GetStaticMesh() == Cube);
	TestFalse(TEXT("placeholder wings hidden"), Ship->FindComponentByClass<UStaticMeshComponent>() == nullptr);
	bool bPlaceholdersHidden = true;
	for (UActorComponent* C : Ship->GetComponents())
	{
		const FString N = C->GetName();
		if ((N == TEXT("PlaceholderWings") || N == TEXT("EngineGlow")) && Cast<USceneComponent>(C)->IsVisible())
		{
			bPlaceholdersHidden = false;
		}
	}
	TestTrue(TEXT("wings and engine block hidden"), bPlaceholdersHidden);

	// Root parts: ship root (hull pivot) x relative.
	const FTransform RootXf = Ship->GetHullMesh()->GetAttachParent()->GetComponentTransform();
	for (int32 i = 0; i < 10; ++i)
	{
		UStaticMeshComponent* Part = Ship->FindVisualPart(*FString::Printf(TEXT("Part_%d"), i));
		if (!TestNotNull(*FString::Printf(TEXT("Part_%d found"), i), Part)) { continue; }
		const FTransform Expected = Visuals->Parts[i].RelativeTransform * RootXf;
		TestTrue(*FString::Printf(TEXT("Part_%d world location"), i), Part->GetComponentLocation().Equals(Expected.GetLocation(), 0.5f));
		TestTrue(*FString::Printf(TEXT("Part_%d world rotation"), i), Part->GetComponentQuat().Equals(Expected.GetRotation(), 0.001f));
		TestEqual(*FString::Printf(TEXT("Part_%d no collision"), i), Part->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestFalse(*FString::Printf(TEXT("Part_%d doesn't tick"), i), Part->IsComponentTickEnabled());
	}
	// Socketed part: socket transform x relative.
	UStaticMeshComponent* Engine = Ship->FindVisualPart(TEXT("Engine"));
	if (TestNotNull(TEXT("socketed Engine part"), Engine))
	{
		const FTransform SocketXf = Ship->GetHullMesh()->GetSocketTransform(TEXT("Pod_L"));
		const FTransform Expected = FTransform(FVector(20.f, 0.f, 0.f)) * SocketXf;
		TestTrue(TEXT("attached to the Pod_L socket"), Engine->GetAttachSocketName() == FName(TEXT("Pod_L")) && Engine->GetAttachParent() == Ship->GetHullMesh());
		TestTrue(FString::Printf(TEXT("socketed world location (%s vs %s)"), *Engine->GetComponentLocation().ToString(), *Expected.GetLocation().ToString()),
			Engine->GetComponentLocation().Equals(Expected.GetLocation(), 0.5f));
	}
	TestNull(TEXT("unknown part name"), Ship->FindVisualPart(TEXT("Nope")));

	// A real projectile: flies through a part (900 cm off the hull) but hits the hull sphere.
	const FTDProjectileSpec& Spec = UTDWeaponLibrary::Get(TDWeaponIds::NeedleCannons).Projectile;
	UTDHealthComponent* Health = Ship->GetHealth();
	const float Before = Health->GetHull() + Health->GetShield();
	UStaticMeshComponent* FarPart = Ship->FindVisualPart(TEXT("Part_0"));
	const FVector PartAt = FarPart->GetComponentLocation();
	// Along the nose, through the part: the line passes 900 cm to the side of the hull sphere.
	const FVector Dir = Ship->GetActorForwardVector();
	ATDProjectile::Spawn(W.GetWorld(), Spec, FTransform(Dir.Rotation(), PartAt - Dir * 3000.f), nullptr, ETDTeam::Enemy);
	W.Advance(1.f);
	TestEqual(TEXT("a shot through a visual part does no damage"), Health->GetHull() + Health->GetShield(), Before, 0.01f);
	ATDProjectile::Spawn(W.GetWorld(), Spec, FTransform(Dir.Rotation(), Ship->GetActorLocation() - Dir * 3000.f), nullptr, ETDTeam::Enemy);
	W.Advance(1.f);
	TestTrue(TEXT("a shot at the hull sphere hits"), Health->GetHull() + Health->GetShield() < Before);

	// Teardown: null restores the placeholder.
	Ship->SetShipVisuals(nullptr);
	TestEqual(TEXT("parts torn down"), Ship->GetVisualParts().Num(), 0);
	TestFalse(TEXT("placeholder back"), Ship->HasRealArt());
	TestTrue(TEXT("placeholder cone on the hull"), Ship->GetHullMesh()->GetStaticMesh() && Ship->GetHullMesh()->GetStaticMesh() != Cube && Ship->GetHullMesh()->IsVisible());
	bool bPlaceholdersVisible = true;
	for (UActorComponent* C : Ship->GetComponents())
	{
		const FString N = C->GetName();
		if ((N == TEXT("PlaceholderWings") || N == TEXT("EngineGlow")) && !Cast<USceneComponent>(C)->IsVisible())
		{
			bPlaceholdersVisible = false;
		}
		TestFalse(TEXT("no leftover part components"), N.StartsWith(TEXT("VisualPart_")) && IsValid(C) && C->IsRegistered());
	}
	TestTrue(TEXT("wings and engine block visible again"), bPlaceholdersVisible);

	// Parts only: the first part is the fuselage; sockets resolve on it; hull component hidden.
	UTDShipVisuals* PartsOnly = NewObject<UTDShipVisuals>();
	PartsOnly->Parts.Add(TDShipVisualsTest::MakePart(TEXT("Fuselage"), Cube, NAME_None, FTransform::Identity));
	PartsOnly->Parts.Add(TDShipVisualsTest::MakePart(TEXT("Pod"), Sphere, TEXT("Pod_L"), FTransform::Identity));
	Ship->SetShipVisuals(PartsOnly);
	TestTrue(TEXT("parts-only is real art"), Ship->HasRealArt());
	TestFalse(TEXT("hull component hidden for parts-only"), Ship->GetHullMesh()->IsVisible());
	UStaticMeshComponent* Pod = Ship->FindVisualPart(TEXT("Pod"));
	TestTrue(TEXT("parts-only socket resolves on the fuselage part"), Pod && Pod->GetAttachParent() == Ship->FindVisualPart(TEXT("Fuselage")) && Pod->GetAttachSocketName() == FName(TEXT("Pod_L")));

	// Re-assigning replaces, never stacks.
	Ship->SetShipVisuals(Visuals);
	Ship->SetShipVisuals(Visuals);
	TestEqual(TEXT("re-assign rebuilds exactly 11"), Ship->GetVisualParts().Num(), 11);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDShipVisualCues, "TriadDrift.Flight.Visuals.FlightCuesRequested", TD_TEST_FLAGS)
bool FTDShipVisualCues::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	TestEqual(TEXT("no visuals: no cue requests"), Ship->GetCueRequestCount(TEXT("EngineTrail")), 0);

	UTDShipVisuals* Visuals = NewObject<UTDShipVisuals>(); // empty slots: requests happen, nothing plays
	Ship->SetShipVisuals(Visuals);
	TestTrue(TEXT("engine trail starts with the visuals"), Ship->GetCueRequestCount(TEXT("EngineTrail")) >= 1);

	FTDFlightInput In;
	In.Throttle = 1.f;
	In.bBoost = true;
	Ship->SetFlightInput(In);
	W.Advance(0.3f);
	TestEqual(TEXT("BoostStart on boost"), Ship->GetCueRequestCount(TEXT("BoostStart")), 1);
	TestEqual(TEXT("BoostLoop on boost"), Ship->GetCueRequestCount(TEXT("BoostLoop")), 1);

	In.bBoost = false;
	In.bDrift = true;
	In.Aim = FVector2D(1.f, 0.f);
	Ship->SetFlightInput(In);
	W.Advance(0.5f);
	TestEqual(TEXT("DriftLoop on drift"), Ship->GetCueRequestCount(TEXT("DriftLoop")), 1);
	In.bDrift = false;
	Ship->SetFlightInput(In);
	W.Tick();
	TestEqual(TEXT("DriftRelease on an earned burst"), Ship->GetCueRequestCount(TEXT("DriftRelease")), 1);

	// Scrape cue on a glancing hit.
	AActor* Wall = W.Spawn<AActor>(FVector(3000.f, 0.f, 0.f));
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall, TEXT("Box"));
	Box->SetBoxExtent(FVector(200.f, 40000.f, 40000.f));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionResponseToAllChannels(ECR_Block);
	Wall->SetRootComponent(Box);
	Box->RegisterComponent();
	Wall->SetActorLocation(FVector(3000.f, 0.f, 0.f));
	Ship->TeleportSpacecraft(FVector(2000.f, 0.f, 0.f), FRotator(0.f, 70.f, 0.f), false);
	Ship->GetShipMovement()->SetForwardSpeedImmediate(6000.f);
	FTDFlightInput Straight;
	Straight.Throttle = 1.f;
	Ship->SetFlightInput(Straight);
	W.Advance(1.f);
	TestTrue(TEXT("CollisionScrape on a glancing hit"), Ship->GetCueRequestCount(TEXT("CollisionScrape")) >= 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
