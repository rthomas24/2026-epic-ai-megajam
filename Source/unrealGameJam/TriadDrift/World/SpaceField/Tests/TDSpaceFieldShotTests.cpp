// Triad Drift - space field: real shots and explosions through T2's ITDFieldDamageable (Teammate 6).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.SpaceField

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Combat/TDFieldDamageable.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDFlightTypes.h"

namespace TDSpaceFieldShotTest
{
	/** The first alive, non-special small piece with nothing else within Clear of it. */
	static int32 FindLoneSmall(const ATDSpaceField* F, float Clear)
	{
		const TArray<FTDSpaceFieldPiece>& P = F->GetPieces();
		for (int32 i = 0; i < P.Num(); ++i)
		{
			if (!F->IsPieceAlive(i) || F->GetPieceSizeClass(i) != 0 || P[i].Kind == ETDSpaceFieldKind::Wreck) { continue; }
			bool bLone = true;
			for (int32 j = 0; j < P.Num() && bLone; ++j)
			{
				bLone = j == i || !F->IsPieceAlive(j) || FVector::Dist(P[i].Location, P[j].Location) > Clear + P[j].Size;
			}
			if (bLone) { return i; }
		}
		return INDEX_NONE;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldRealShot, "TriadDrift.World.SpaceField.Break.RealShotsBreakASmallRocketSplashBreaks", TD_TEST_FLAGS)
bool FTDSpaceFieldRealShot::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDGalaxyLayout* Layout = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
	W.Advance(0.1f);
	ATDSpaceField* Field = ATDSpaceField::SpawnFor(Layout);
	W.Advance(0.1f);
	if (!TestNotNull(TEXT("field"), Field)) { return false; }
	TArray<AActor*> Fields;
	FTDFieldDamage::GetFields(W.GetWorld(), Fields);
	TestTrue(TEXT("the field is registered for rams and explosions"), Fields.Contains(Field));

	// Cannon shots from 40 m straight at a lone small piece: it breaks within three shots.
	const int32 S = TDSpaceFieldShotTest::FindLoneSmall(Field, 6000.f);
	if (!TestTrue(TEXT("a lone small piece"), S != INDEX_NONE)) { return false; }
	const FVector At = Field->GetPieces()[S].Location;
	const FVector From = At - FVector(4000.f, 0.f, 0.f);
	AActor* Shooter = W.Spawn<AActor>(From - FVector(500.f, 0.f, 0.f));
	const FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::PhasePulseLasers).Projectile;
	int32 Shots = 0;
	for (; Shots < 3 && Field->IsPieceAlive(S); ++Shots)
	{
		ATDProjectile::Launch(W.GetWorld(), Spec, FTransform((At - From).Rotation(), From), Shooter, ETDTeam::Player, nullptr, FVector::ZeroVector, 1.f, true);
		W.Advance(0.5f);
	}
	AddInfo(FString::Printf(TEXT("laser damage %.1f, small health %.1f: broke after %d shots"), Spec.Damage, Field->SmallHealth, Shots));
	TestFalse(TEXT("real cannon shots break a small piece"), Field->IsPieceAlive(S));
	TestTrue(TEXT("the field counted it"), Field->GetBrokenCount() >= 1);

	// A rocket detonating beside another lone small breaks it through the splash path.
	const int32 S2 = TDSpaceFieldShotTest::FindLoneSmall(Field, 6000.f);
	if (!TestTrue(TEXT("another lone small"), S2 != INDEX_NONE)) { return false; }
	const FVector At2 = Field->GetPieces()[S2].Location;
	const FTDProjectileSpec Rocket = UTDWeaponLibrary::Get(TDWeaponIds::SpikeRockets).Projectile;
	if (ATDProjectile* R = ATDProjectile::Launch(W.GetWorld(), Rocket, FTransform(FRotator::ZeroRotator, At2 + FVector(0.f, 0.f, 3000.f)), Shooter, ETDTeam::Player,
		nullptr, FVector::ZeroVector, 1.f, true))
	{
		R->TeleportProjectile(At2 + FVector(0.f, 0.f, 300.f), FVector::ZeroVector);
		R->Detonate();
	}
	TestFalse(TEXT("a rocket's splash breaks a small piece"), Field->IsPieceAlive(S2));
	return true;
}

// A ship at full boost into a large rock stops at its surface (T1's swept move against the hidden collision sphere) and
// is never inside it, at 30 and 120 fps. The rock stands (a large never breaks on one crash).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldCrash, "TriadDrift.World.SpaceField.Break.FullBoostIntoALargeStopsNeverInside", TD_TEST_FLAGS)
bool FTDSpaceFieldCrash::RunTest(const FString& Parameters)
{
	for (const float Dt : { 1.f / 30.f, 1.f / 120.f })
	{
		FTDTestWorld W;
		ATDGalaxyLayout* Layout = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
		W.Advance(0.1f);
		ATDSpaceField* Field = ATDSpaceField::SpawnFor(Layout);
		W.Advance(0.1f);
		if (!TestNotNull(TEXT("field"), Field)) { return false; }
		// A large rock with a clear 300 m run-up along -X.
		int32 L = INDEX_NONE;
		const TArray<FTDSpaceFieldPiece>& P = Field->GetPieces();
		for (int32 i = 0; i < P.Num() && L == INDEX_NONE; ++i)
		{
			if (!Field->IsPieceAlive(i) || Field->GetPieceSizeClass(i) != 2 || P[i].Kind == ETDSpaceFieldKind::Wreck) { continue; }
			const FVector Start = P[i].Location - FVector(P[i].Size * 0.4f + 30000.f, 0.f, 0.f);
			bool bClear = true;
			for (int32 j = 0; j < P.Num() && bClear; ++j)
			{
				bClear = j == i || !Field->IsPieceAlive(j) || FMath::PointDistToSegment(P[j].Location, Start, P[i].Location) > P[j].Size * 0.4f + 1500.f;
			}
			if (bClear) { L = i; }
		}
		if (!TestTrue(TEXT("a large rock with a clear run-up"), L != INDEX_NONE)) { return false; }
		const FVector C = P[L].Location;
		const float R = P[L].Size * 0.4f;
		ATDShip* Ship = W.Spawn<ATDShip>(C - FVector(R + 30000.f, 0.f, 0.f));
		if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
		FTDFlightInput In;
		In.Throttle = 1.f;
		In.bBoost = true;
		float MinDist = TNumericLimits<float>::Max();
		for (float T = 0.f; T < 6.f; T += Dt)
		{
			Ship->SetFlightInput(In);
			W.Advance(Dt, Dt);
			MinDist = FMath::Min(MinDist, (float)FVector::Dist(Ship->GetActorLocation(), C));
		}
		AddInfo(FString::Printf(TEXT("%.0f fps: rock collision radius %.0f cm, closest ship centre %.0f cm, rock alive %d, health %.0f"), 1.f / Dt, R, MinDist,
			Field->IsPieceAlive(L), Field->GetPieceHealth(L)));
		TestTrue(*FString::Printf(TEXT("%.0f fps: the ship reached the rock"), 1.f / Dt), MinDist < R + 5000.f);
		TestTrue(*FString::Printf(TEXT("%.0f fps: never inside the rock (%.0f >= %.0f)"), 1.f / Dt, MinDist, R), MinDist >= R);
		TestTrue(*FString::Printf(TEXT("%.0f fps: a large rock stands one crash"), 1.f / Dt), Field->IsPieceAlive(L));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
