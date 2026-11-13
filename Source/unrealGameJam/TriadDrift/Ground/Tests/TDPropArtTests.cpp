// Triad Drift - art for the code-built shapes (TDPropArt): fitted to the shape's box, the shape keeps its collision,
// the art follows visibility switches, and nothing changes without art. Owner: Teammate 5.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Ground/Planets/TDPlanetProps.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPropArtDress, "TriadDrift.Ground.PropArt.FittedToTheShapeWhichKeepsItsCollision", TD_TEST_FLAGS)
bool FTDPropArtDress::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Art = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")); // stands in for a prop mesh
	if (!TestNotNull(TEXT("shapes"), Cube) || !TestNotNull(TEXT("art"), Art)) { return false; }
	AActor* Owner = W.Spawn<AActor>(FVector::ZeroVector);
	UStaticMeshComponent* Shape = NewObject<UStaticMeshComponent>(Owner);
	Shape->SetStaticMesh(Cube);
	Shape->SetMobility(EComponentMobility::Movable);
	Owner->SetRootComponent(Shape);
	Shape->SetRelativeScale3D(FVector(6.f, 1.5f, 4.f)); // a 600 x 150 x 400 cm wall
	Shape->SetCollisionProfileName(TEXT("BlockAll"));
	Shape->RegisterComponent();

	TestNull(TEXT("no art: nothing changes"), TDPropArt::Dress(Shape, nullptr));
	TestFalse(TEXT("no art: the shape still shows"), Shape->bHiddenInGame);

	UStaticMeshComponent* Dressed = TDPropArt::Dress(Shape, Art);
	if (!TestNotNull(TEXT("dressed"), Dressed)) { return false; }
	TestTrue(TEXT("the shape is hidden in game"), Shape->bHiddenInGame);
	TestEqual(TEXT("the shape keeps its collision"), (int32)Shape->GetCollisionEnabled(), (int32)ECollisionEnabled::QueryAndPhysics);
	TestEqual(TEXT("the art has none"), (int32)Dressed->GetCollisionEnabled(), (int32)ECollisionEnabled::NoCollision);
	const FVector ShapeSize = Shape->Bounds.GetBox().GetSize();
	const FVector ArtSize = Dressed->Bounds.GetBox().GetSize();
	TestTrue(*FString::Printf(TEXT("the art fills the shape's box (%s vs %s)"), *ArtSize.ToCompactString(), *ShapeSize.ToCompactString()), ArtSize.Equals(ShapeSize, 2.f));
	TestTrue(TEXT("centred on it"), Dressed->Bounds.Origin.Equals(Shape->Bounds.Origin, 1.f));

	Shape->SetVisibility(false, true);
	TestFalse(TEXT("a visibility switch on the shape reaches the art"), Dressed->IsVisible());
	Shape->SetVisibility(true, true);
	TestTrue(TEXT("and back"), Dressed->IsVisible());

	Shape->SetRelativeScale3D(FVector(3.f, 1.5f, 4.f)); // resized later (doors, conduits)
	Shape->UpdateComponentToWorld();
	TestTrue(TEXT("a later resize carries over"), Dressed->Bounds.GetBox().GetSize().Equals(Shape->Bounds.GetBox().GetSize(), 2.f));
	return true;
}

// Every slot names a default under Gameplay/Props/Planet (or an existing kit mesh), and a course with no art on disk
// still plays exactly as before (the planet tests cover the gameplay; this checks the slots are wired).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPropArtSlots, "TriadDrift.Ground.PropArt.EverySlotHasADefault", TD_TEST_FLAGS)
bool FTDPropArtSlots::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("default path"), TDPropArt::Default(TEXT("SM_Test")).ToSoftObjectPath().ToString(), FString(TEXT("/Game/TriadDrift/Gameplay/Props/Planet/SM_Test.SM_Test")));
	TestNull(TEXT("a missing asset loads as null, quietly"), TDPropArt::Load(TDPropArt::Default(TEXT("SM_DoesNotExist_T5"))));
	TestFalse(TEXT("console"), GetDefault<ATDInteractableActor>()->ConsoleMesh.IsNull());
	TestFalse(TEXT("door"), GetDefault<ATDDoor>()->DoorMesh.IsNull());
	TestFalse(TEXT("beacon"), GetDefault<ATDStepTrigger>()->BeaconMesh.IsNull());
	TestFalse(TEXT("core"), GetDefault<ATDExtractionSite>()->CoreMesh.IsNull());
	TestFalse(TEXT("magma"), GetDefault<ATDMagmaVolume>()->SurfaceMesh.IsNull());
	TestFalse(TEXT("geyser"), GetDefault<ATDGeyser>()->VentMesh.IsNull() || GetDefault<ATDGeyser>()->PlumeMesh.IsNull());
	TestFalse(TEXT("crystal"), GetDefault<ATDShatterCrystal>()->CrystalMesh.IsNull());
	TestFalse(TEXT("bridge slab"), GetDefault<ATDCollapsingPlatform>()->SlabMesh.IsNull());
	TestFalse(TEXT("debris"), GetDefault<ATDCollapseRun>()->DebrisMesh.IsNull());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
