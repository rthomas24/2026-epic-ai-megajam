// Hitting the world (Ryan, Nov 13): "the harder you hit it, the more it will do damage ... you shouldn't be able to go
// through them". Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight.Impact

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "Components/SphereComponent.h"
#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "EngineUtils.h"

namespace TDImpactTest
{
	struct FResult { float Damage = 0.f; float ClosestGap = 1e9f; int32 Received = 0; bool bThrough = false; float SpeedAtHit = 0.f; };

	/** A ship flying at Speed straight (or at Angle deg off straight) into a 50 m ball at Fps; flown for 2 s. */
	FResult Fly(float Speed, float Fps, float AngleDeg = 0.f, bool bBreaks = false)
	{
		FResult R;
		FTDTestWorld W;
		const float Radius = 5000.f;
		ATDTestRock* Rock = W.Spawn<ATDTestRock>(FVector(0.f, 0.f, 20000.f));
		ATDShip* Ship = W.Spawn<ATDShip>(FVector(-Radius - 6000.f, 0.f, 20000.f));
		if (!Rock || !Ship) { return R; }
		Rock->Ball->SetSphereRadius(Radius);
		Rock->bBreaks = bBreaks;
		UTDFlightEventCounter* Counter = NewObject<UTDFlightEventCounter>();
		Counter->AddToRoot();
		Counter->Bind(Ship->GetShipMovement());
		// Aim at a point on the ball so the hit is AngleDeg off head-on.
		const FVector Target = Rock->GetActorLocation() + FVector(-FMath::Cos(FMath::DegreesToRadians(AngleDeg)), FMath::Sin(FMath::DegreesToRadians(AngleDeg)), 0.f) * Radius;
		Ship->TeleportSpacecraft(Ship->GetActorLocation(), (Target - Ship->GetActorLocation()).Rotation(), false);
		UTDShipMovementComponent* Move = Ship->GetShipMovement();
		Move->SetSpeedOverrideFor(TEXT("TDImpactTest"), Speed, 1.f); // hold the speed up to the first contact
		Move->SetForwardSpeedImmediate(Speed);
		const float Shield = Ship->GetHealth()->GetShield(), Hull = Ship->GetHealth()->GetHull();
		const float Dt = 1.f / Fps;
		float After = -1.f;
		for (float T = 0.f; T < 3.f && After < 0.4f; T += Dt)
		{
			FTDFlightInput In;
			In.Throttle = 0.f;
			Ship->SetFlightInput(In);
			W.Tick(Dt);
			if (After < 0.f && (Counter->Impacts > 0 || Rock->Received > 0)) { After = 0.f; Move->ClearSpeedOverrideFor(TEXT("TDImpactTest")); }
			else if (After >= 0.f) { After += Dt; }
			const float Gap = FVector::Dist(Ship->GetActorLocation(), Rock->GetActorLocation()) - Radius - Ship->GetCollisionSphere()->GetScaledSphereRadius();
			R.ClosestGap = FMath::Min(R.ClosestGap, Gap);
		}
		R.Damage = (Shield - Ship->GetHealth()->GetShield()) + (Hull - Ship->GetHealth()->GetHull());
		R.Received = Rock->Received;
		R.SpeedAtHit = Rock->LastSpeed;
		R.bThrough = Ship->GetActorLocation().X > Rock->GetActorLocation().X - Radius + 1500.f; // well inside where the rock stood
		Counter->RemoveFromRoot();
		return R;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDImpactBoostHeadOn, "TriadDrift.Flight.Impact.HarderHitsHurtMoreAndNeverPassThrough", TD_TEST_FLAGS)
bool FTDImpactBoostHeadOn::RunTest(const FString& Parameters)
{
	using namespace TDImpactTest;
	FTDTestWorld Probe;
	ATDShip* Ref = Probe.Spawn<ATDShip>(FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship"), Ref)) { return false; }
	const FTDFlightTuning& T = Ref->GetShipMovement()->GetFlightTuning();
	const float MaxHull = Ref->GetHealth()->MaxHull;
	for (const float Fps : { 30.f, 120.f })
	{
		const FString Tag = FString::Printf(TEXT("@%.0f fps: "), Fps);
		const FResult Boost = Fly(T.BoostSpeed, Fps);
		const FResult Cruise = Fly(T.CruiseSpeed, Fps);
		const FResult Scrape = Fly(1000.f, Fps); // 10 m/s straight in
		AddInfo(FString::Printf(TEXT("%sboost %.0f dmg (gap %.0f cm), cruise %.0f dmg, 10 m/s %.0f dmg (max hull %.0f)"), *Tag, Boost.Damage, Boost.ClosestGap, Cruise.Damage, Scrape.Damage, MaxHull));
		TestTrue(*(Tag + TEXT("full boost head-on: stopped outside the rock (never through it)")), Boost.ClosestGap > -60.f && !Boost.bThrough);
		TestTrue(*(Tag + TEXT("full boost head-on: takes most of the hull (60-90 %)")), Boost.Damage >= 0.6f * MaxHull && Boost.Damage <= 0.9f * MaxHull + 1.f);
		TestTrue(*(Tag + TEXT("cruise hurts less than boost")), Cruise.Damage > 0.f && Cruise.Damage < Boost.Damage * 0.6f);
		TestEqual(*(Tag + TEXT("a 10 m/s bump does no damage")), Scrape.Damage, 0.f, 0.01f);
		TestTrue(*(Tag + TEXT("the rock was told (before the ship's response)")), Boost.Received > 0);
		TestEqual(*(Tag + TEXT("... with the speed into it")), Boost.SpeedAtHit, T.BoostSpeed, T.BoostSpeed * 0.15f);
		TestEqual(*(Tag + TEXT("a 10 m/s bump doesn't tell the rock")), Scrape.Received, 0);
		// A glancing hit at boost speed (70 deg off head-on): a slide, little damage.
		const FResult Glance = Fly(T.BoostSpeed, Fps, 70.f);
		TestTrue(*(Tag + FString::Printf(TEXT("a glancing boost hit slides off cheaply (%.0f dmg)"), Glance.Damage)), Glance.Damage < 0.2f * MaxHull && Glance.ClosestGap > -60.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDImpactBreaks, "TriadDrift.Flight.Impact.ABrokenPieceLetsTheShipThrough", TD_TEST_FLAGS)
bool FTDImpactBreaks::RunTest(const FString& Parameters)
{
	using namespace TDImpactTest;
	FTDTestWorld Probe;
	ATDShip* Ref = Probe.Spawn<ATDShip>(FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship"), Ref)) { return false; }
	const float Boost = Ref->GetShipMovement()->GetFlightTuning().BoostSpeed;
	for (const float Fps : { 30.f, 120.f })
	{
		const FResult R = Fly(Boost, Fps, 0.f, /*bBreaks*/ true);
		TestTrue(*FString::Printf(TEXT("@%.0f fps: the rock broke: no damage, the ship carries on through (dmg %.0f)"), Fps, R.Damage), R.Damage < 0.01f && R.bThrough && R.Received == 1);
	}
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDImpactRealField, "TriadDrift.Flight.Impact.ARealFieldRockTakesTheCrash", TD_TEST_FLAGS)
bool FTDImpactRealField::RunTest(const FString& Parameters)
{
	// The Lead (T6's live crash, Nov 13): the ship bounced off a large rock that took no damage. The real galaxy and field,
	// the flown ship at boost straight into a lone large piece: the piece is damaged, the ship stops outside it.
	FTDTestWorld W;
	ATDGalaxyLayout* Layout = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
	W.Advance(0.1f);
	ATDSpaceField* Field = ATDSpaceField::SpawnFor(Layout);
	W.Advance(0.1f);
	if (!TestNotNull(TEXT("field"), Field)) { return false; }
	const TArray<FTDSpaceFieldPiece>& P = Field->GetPieces();
	int32 Big = INDEX_NONE;
	for (int32 i = 0; i < P.Num() && Big == INDEX_NONE; ++i)
	{
		if (!Field->IsPieceAlive(i) || Field->GetPieceSizeClass(i) != 2 || P[i].Kind == ETDSpaceFieldKind::Wreck) { continue; }
		bool bLone = true;
		// Clear on the approach side only (-X), 100 m out: the real field packs its large rocks close together.
		const FVector Approach = P[i].Location - FVector(P[i].Size + 9000.f, 0.f, 0.f);
		for (int32 j = 0; j < P.Num() && bLone; ++j)
		{
			if (j == i || !Field->IsPieceAlive(j)) { continue; }
			const FVector Seg = FMath::ClosestPointOnSegment(P[j].Location, Approach, P[i].Location);
			bLone = FVector::Dist(Seg, P[j].Location) > P[j].Size + 2000.f;
		}
		if (bLone) { Big = i; }
	}
	if (!TestTrue(TEXT("a lone large piece"), Big != INDEX_NONE)) { return false; }
	const FVector At = P[Big].Location;
	const FVector From = At - FVector(P[Big].Size + 9000.f, 0.f, 0.f);
	ATDShip* Ship = W.Spawn<ATDShip>(From);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	Ship->SetPlayerControlled(true);
	Ship->TeleportSpacecraft(From, (At - From).Rotation(), false);
	UTDShipMovementComponent* Move = Ship->GetShipMovement();
	const float Boost = Move->GetFlightTuning().BoostSpeed;
	Move->SetSpeedOverrideFor(TEXT("TDImpactTest"), Boost, 0.f);
	Move->SetForwardSpeedImmediate(Boost);
	UTDFlightEventCounter* Counter = NewObject<UTDFlightEventCounter>();
	Counter->AddToRoot();
	Counter->Bind(Move);
	const float Health0 = Field->GetPieceHealth(Big);
	const float Hull0 = Ship->GetHealth()->GetHull() + Ship->GetHealth()->GetShield();
	for (int32 i = 0; i < 240 && Counter->Impacts == 0; ++i) { W.Tick(1.f / 60.f); }
	Move->ClearSpeedOverrideFor(TEXT("TDImpactTest"));
	W.Advance(0.3f);
	const float Health1 = Field->IsPieceAlive(Big) ? Field->GetPieceHealth(Big) : 0.f;
	const float ShipLost = Hull0 - (Ship->GetHealth()->GetHull() + Ship->GetHealth()->GetShield());
	AddInfo(FString::Printf(TEXT("large piece %d (size %.0f m): health %.0f -> %.0f; impacts %d, ship lost %.0f, gap to centre %.0f m"), Big, P[Big].Size / 100.f, Health0, Health1,
		Counter->Impacts, ShipLost, (FVector::Dist(Ship->GetActorLocation(), At) - P[Big].Size) / 100.f));
	TestTrue(TEXT("the ship hit the rock"), Counter->Impacts > 0);
	TestTrue(TEXT("the rock took the crash (its health dropped)"), Health1 < Health0);
	TestTrue(TEXT("the ship took the crash too (most of the hull at boost)"), ShipLost >= 40.f);
	Counter->RemoveFromRoot();
	return true;
}

#endif
