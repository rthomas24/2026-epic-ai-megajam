// Range readability (Lead, art audit): an enemy's visual mesh grows with camera distance so a ship at 250 m reads
// as a ship, while its collision, hit zones and aim point never change.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Enemies.RangeScale

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "TriadDrift/Core/TDPresentation.h"

namespace TDEnemyRangeScaleTest
{
	ATDEnemyShip* SpawnShip(FTDTestWorld& W, FName Id)
	{
		ATDEnemyShip* E = W.Spawn<ATDEnemyShip>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (E) { E->InitFromSpec(Id); }
		return E;
	}

	/** Steps the smoothing until it settles (real seconds at 60 fps). */
	void Settle(ATDEnemyShip* E, const FVector& View, bool bAllowed, float Seconds = 4.f)
	{
		for (float T = 0.f; T < Seconds; T += 1.f / 60.f) { E->UpdateRangeScale(View, bAllowed, 1.f / 60.f); }
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemyRangeScaleCurve, "TriadDrift.Enemies.RangeScale.CurveIsOneAt60mAndCappedAt250m", TD_TEST_FLAGS)
bool FTDEnemyRangeScaleCurve::RunTest(const FString&)
{
	const float N = 6000.f, F = 25000.f, M = 2.5f;
	TestEqual(TEXT("1x up close"), ATDEnemyShip::RangeScaleFor(1000.f, N, F, M), 1.f);
	TestEqual(TEXT("1x at 60 m"), ATDEnemyShip::RangeScaleFor(N, N, F, M), 1.f);
	TestTrue(TEXT("halfway is about 1.75x"), FMath::IsNearlyEqual(ATDEnemyShip::RangeScaleFor((N + F) * 0.5f, N, F, M), 1.75f, 0.001f));
	TestEqual(TEXT("2.5x at 250 m"), ATDEnemyShip::RangeScaleFor(F, N, F, M), 2.5f);
	TestEqual(TEXT("capped beyond 250 m"), ATDEnemyShip::RangeScaleFor(80000.f, N, F, M), 2.5f);
	float Prev = 0.f;
	bool bMonotonic = true;
	for (float D = 0.f; D <= 40000.f; D += 500.f)
	{
		const float S = ATDEnemyShip::RangeScaleFor(D, N, F, M);
		bMonotonic &= S >= Prev;
		Prev = S;
	}
	TestTrue(TEXT("never shrinks as distance grows"), bMonotonic);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemyRangeScaleCollision, "TriadDrift.Enemies.RangeScale.CollisionAndAimPointNeverChange", TD_TEST_FLAGS)
bool FTDEnemyRangeScaleCollision::RunTest(const FString&)
{
	using namespace TDEnemyRangeScaleTest;
	FTDTestWorld W;
	for (const FName Id : { TDEnemyIds::ShardFighter, TDEnemyIds::HookInterceptor, TDEnemyIds::MaulBomber })
	{
		ATDEnemyShip* E = SpawnShip(W, Id);
		if (!TestNotNull(*FString::Printf(TEXT("%s spawned"), *Id.ToString()), E)) { continue; }
		USphereComponent* Sphere = E->GetCollisionSphere();
		const float Radius = Sphere->GetScaledSphereRadius();
		const FVector SphereScale = Sphere->GetComponentScale();
		const FVector Centre = E->GetActorLocation();
		const FVector HullAt = E->GetHullMesh() ? E->GetHullMesh()->GetComponentLocation() : FVector::ZeroVector;
		const FVector BaseScale = E->GetHullMesh() ? E->GetHullMesh()->GetComponentScale() : FVector::OneVector;

		Settle(E, FVector(30000.f, 0.f, 0.f), true);	// 300 m away: full range scale
		TestTrue(*FString::Printf(TEXT("%s: visual reaches 2.5x at range"), *Id.ToString()), FMath::IsNearlyEqual(E->GetRangeScale(), 2.5f, 0.02f));
		if (E->GetHullMesh())
		{
			const FVector Grown = E->GetHullMesh()->GetComponentScale();
			TestTrue(*FString::Printf(TEXT("%s: hull mesh grew"), *Id.ToString()), Grown.X > BaseScale.X * 2.4f);
			// The hull sits on the pivot at the actor centre, so its offset grows with the scale but its centre stays the aim point.
			TestTrue(*FString::Printf(TEXT("%s: scaled about the centre"), *Id.ToString()),
				FVector::Dist(Centre, E->GetHullMesh()->GetComponentLocation()) <= FVector::Dist(Centre, HullAt) * 2.6f + 1.f);
		}
		TestEqual(*FString::Printf(TEXT("%s: collision radius unchanged"), *Id.ToString()), Sphere->GetScaledSphereRadius(), Radius);
		TestEqual(*FString::Printf(TEXT("%s: collision scale unchanged"), *Id.ToString()), Sphere->GetComponentScale(), SphereScale);
		TestEqual(*FString::Printf(TEXT("%s: actor (aim point) did not move"), *Id.ToString()), E->GetActorLocation(), Centre);
		bool bVisualsCollide = false;
		TInlineComponentArray<UPrimitiveComponent*> Prims(E);
		for (UPrimitiveComponent* P : Prims)
		{
			bVisualsCollide |= P != Sphere && P->IsCollisionEnabled();
		}
		TestFalse(*FString::Printf(TEXT("%s: only the gameplay sphere collides"), *Id.ToString()), bVisualsCollide);

		Settle(E, FVector(30000.f, 0.f, 0.f), false);	// a director shot / the intro: true scale
		TestTrue(*FString::Printf(TEXT("%s: back to true scale when not allowed"), *Id.ToString()), FMath::IsNearlyEqual(E->GetRangeScale(), 1.f, 0.01f));
		if (E->GetHullMesh())
		{
			TestTrue(*FString::Printf(TEXT("%s: hull back to its base scale"), *Id.ToString()), E->GetHullMesh()->GetComponentScale().Equals(BaseScale, 0.01f));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemyRangeScaleArt, "TriadDrift.Enemies.RangeScale.ArtMeshScalesAndHidesThePlaceholderParts", TD_TEST_FLAGS)
bool FTDEnemyRangeScaleArt::RunTest(const FString&)
{
	using namespace TDEnemyRangeScaleTest;
	FTDTestWorld W;
	int32 Checked = 0;
	for (const FName Id : { TDEnemyIds::ShardFighter, TDEnemyIds::HookInterceptor, TDEnemyIds::MaulBomber })
	{
		ATDEnemyShip* E = SpawnShip(W, Id);
		if (!E || !E->GetHullMesh() || E->GetHullMesh()->GetStaticMesh() == nullptr) { continue; }
		const UTDEnemyVisuals* V = ATDEnemyShip::ResolveVisuals(UTDEnemyLibrary::Get(Id));
		if (!V || V->Mesh.IsNull()) { continue; }
		++Checked;
		const FString N = Id.ToString();
		TestFalse(*FString::Printf(TEXT("%s: no white engine-glow cube on the art"), *N), E->GetEngineGlow() && E->GetEngineGlow()->IsVisible());
		const FVector Base = E->GetHullMesh()->GetComponentScale();
		const float Radius = E->GetCollisionSphere()->GetScaledSphereRadius();
		Settle(E, FVector(30000.f, 0.f, 0.f), true);
		TestTrue(*FString::Printf(TEXT("%s: the art mesh grows at range"), *N), E->GetHullMesh()->GetComponentScale().X > Base.X * 2.4f);
		TestEqual(*FString::Printf(TEXT("%s: collision unchanged"), *N), E->GetCollisionSphere()->GetScaledSphereRadius(), Radius);
	}
	if (Checked == 0) { AddInfo(TEXT("no enemy visuals asset in this checkout: art path not exercised")); }
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemyRangeScaleSmooth, "TriadDrift.Enemies.RangeScale.SmoothedNeverPops", TD_TEST_FLAGS)
bool FTDEnemyRangeScaleSmooth::RunTest(const FString&)
{
	using namespace TDEnemyRangeScaleTest;
	FTDTestWorld W;
	ATDEnemyShip* E = SpawnShip(W, TDEnemyIds::ShardFighter);
	if (!TestNotNull(TEXT("fighter"), E)) { return false; }
	// Jump the camera from point blank to 400 m: the scale must ease, never jump more than 0.12 in one 60 fps frame.
	float Prev = E->GetRangeScale();
	float MaxStep = 0.f;
	for (int32 i = 0; i < 240; ++i)
	{
		E->UpdateRangeScale(FVector(40000.f, 0.f, 0.f), true, 1.f / 60.f);
		MaxStep = FMath::Max(MaxStep, FMath::Abs(E->GetRangeScale() - Prev));
		Prev = E->GetRangeScale();
	}
	TestTrue(*FString::Printf(TEXT("largest per-frame step %.3f <= 0.12"), MaxStep), MaxStep <= 0.12f);
	TestTrue(TEXT("settles at the cap"), FMath::IsNearlyEqual(Prev, 2.5f, 0.02f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemyRangeScaleParts, "TriadDrift.Enemies.RangeScale.ShipsWithHitPartsKeepTrueScale", TD_TEST_FLAGS)
bool FTDEnemyRangeScaleParts::RunTest(const FString&)
{
	using namespace TDEnemyRangeScaleTest;
	FTDTestWorld W;
	ATDEnemyShip* Carrier = SpawnShip(W, TDEnemyIds::HiveCarrier);
	if (!TestNotNull(TEXT("carrier"), Carrier)) { return false; }
	TArray<AActor*> Parts;
	Carrier->GetAttachedActors(Parts);
	if (Parts.Num() == 0)
	{
		AddInfo(TEXT("carrier mounts its parts later; nothing to check here"));
		return true;
	}
	Settle(Carrier, FVector(60000.f, 0.f, 0.f), true);
	TestEqual(TEXT("a carrier with bays stays at true scale (bays sit on the hull)"), Carrier->GetRangeScale(), 1.f);
	return true;
}

#endif
