// World review test gaps: the full Dominion assault through REAL projectiles at 30 and 120 fps, the hull
// as cover, layout keep-out and asteroid collision. Run: .\Tools\td-test.ps1 -Filter TriadDrift.World

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"

#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/World/Tests/TDFinaleTestHelpers.h"

#include "Components/InstancedStaticMeshComponent.h"

namespace TDWorldRealPathTest
{
	/** A real player bolt flying from From at Target (direct hit, no fuse, no splash). */
	ATDProjectile* Shoot(FTDTestWorld& W, const FVector& From, const FVector& To, float Damage, ETDTeam Team = ETDTeam::Player, AActor* Shooter = nullptr)
	{
		FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::EnemyHeavyBolt).Projectile;
		Spec.Damage = Damage;
		Spec.DamageKind = ETDDamageKind::Explosive;
		Spec.SplashRadius = 0.f;
		Spec.ProximityFuseRadius = 0.f;
		Spec.bHoming = false;
		Spec.Speed = 40000.f;
		Spec.Range = 40000.f;
		Spec.bInheritShooterVelocity = false;
		return ATDProjectile::Spawn(W.GetWorld(), Spec, FTransform((To - From).Rotation(), From), Shooter, Team);
	}

	/** Fire at a target from Offset away until it dies or MaxShots is reached; returns true if it died. */
	bool ShootUntilDead(FTDTestWorld& W, AActor* Target, const FVector& Offset, float Damage, float Step, int32 MaxShots, TFunctionRef<bool()> IsAlive)
	{
		for (int32 i = 0; i < MaxShots && IsAlive(); ++i)
		{
			Shoot(W, Target->GetActorLocation() + Offset, Target->GetActorLocation(), Damage);
			W.Advance(0.4f, Step);
		}
		return !IsAlive();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldAssaultRealPath, "TriadDrift.World.Dominion.FullAssaultWithRealShots", TD_TEST_FLAGS)
bool FTDWorldAssaultRealPath::RunTest(const FString& Parameters)
{
	using namespace TDWorldRealPathTest;
	for (const float Step : { 1.f / 30.f, 1.f / 120.f })
	{
		const FString Fps = FString::Printf(TEXT("%.0f fps"), 1.f / Step);
		FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
		UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
		GI->Progress.bDominionRevealed = true; GI->Progress.Phase = ETDGamePhase::DominionAssault;
		ATDDominion* D = W.Spawn<ATDDominion>(FVector::ZeroVector);
		D->MaxLaunchedAlive = 0;
		D->Stages[0].LaunchInterval = 0.f;	// keep the sky clear of stray fighters for a clean count
		W.Advance(D->WarpInSeconds + 0.2f, Step);
		ATDDominionReactor* Reactor = D->GetReactor();

		// The main shield blocks a real shot at the reactor.
		const FVector Above(0.f, 0.f, 6000.f);
		Shoot(W, Reactor->GetActorLocation() + Above, Reactor->GetActorLocation(), 500.f);
		W.Advance(0.4f, Step);
		TestEqual(*(Fps + TEXT(": shielded reactor untouched by a real shot")), Reactor->GetTotalHealth01(), 1.f);

		// Four generators down with real shots from outside.
		for (ATDShieldGenerator* G : D->GetGenerators())
		{
			const FVector Out = (G->GetActorLocation() - D->GetActorLocation()).GetSafeNormal() * 8000.f;
			ShootUntilDead(W, G, Out, 300.f, Step, 8, [G]() { return G->IsAlive(); });
		}
		TestEqual(*(Fps + TEXT(": all generators destroyed by real shots")), D->GetGeneratorsRemaining(), 0);
		TestTrue(*(Fps + TEXT(": reactor exposed")), D->GetState() == ETDDominionState::ReactorExposed);

		// A real shot down into the trench reaches the reactor.
		const float Before = Reactor->GetTotalHealth01();
		Shoot(W, Reactor->GetActorLocation() + Above, Reactor->GetActorLocation(), 200.f);
		W.Advance(0.4f, Step);
		TestTrue(*(Fps + TEXT(": real shot down the trench hits the reactor")), Reactor->GetTotalHealth01() < Before);

		// Three phases, real shots only (the invulnerable beat just costs a shot or two).
		ShootUntilDead(W, Reactor, Above, 600.f, Step, 40, [Reactor]() { return Reactor->IsAlive(); });
		TestFalse(*(Fps + TEXT(": reactor destroyed through its three phases")), Reactor->IsAlive());
		W.Advance(D->BreakupSeconds + 2.5f, Step);
		TestTrue(*(Fps + TEXT(": Victory")), GI->Progress.Phase == ETDGamePhase::Victory);

		// A real shot destroys a Dominion turret too.
		if (D->GetTurrets().Num() > 0)
		{
			ATDEnemyTurret* T = D->GetTurrets()[0];
			T->GetHealth()->SetInvulnerable(false);
			const FVector Out = FVector(0.f, 0.f, 5000.f);
			ShootUntilDead(W, T, Out, 500.f, Step, 4, [T]() { return T->IsAlive(); });
			TestFalse(*(Fps + TEXT(": a real shot kills a Dominion turret")), T->IsAlive());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldHullCover, "TriadDrift.World.Dominion.HullBlocksEveryonesShots", TD_TEST_FLAGS)
bool FTDWorldHullCover::RunTest(const FString& Parameters)
{
	using namespace TDWorldRealPathTest;
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	Cast<UTDWaveTestGameInstance>(W.GetGameInstance())->Progress.bDominionRevealed = true; Cast<UTDWaveTestGameInstance>(W.GetGameInstance())->Progress.Phase = ETDGamePhase::DominionAssault;
	ATDDominion* D = W.Spawn<ATDDominion>(FVector::ZeroVector);
	D->MaxLaunchedAlive = 0;
	W.Advance(D->WarpInSeconds + 0.2f);

	// An ENEMY shot fired down at the hull top must stop on it (cover for the squad in the trench).
	const FVector Top(-D->HullExtent.X * 0.3f, 0.f, D->HullExtent.Z + 4000.f);
	ATDProjectile* EnemyShot = Shoot(W, Top, Top - FVector(0.f, 0.f, 20000.f), 10.f, ETDTeam::Enemy);
	const int32 Serial = EnemyShot ? EnemyShot->GetLaunchSerial() : -1;
	W.Advance(0.6f);
	const bool bStopped = !EnemyShot || EnemyShot->GetLaunchSerial() != Serial || EnemyShot->IsHidden() || EnemyShot->GetActorLocation().Z > -D->HullExtent.Z;
	TestTrue(TEXT("enemy shots don't pass through the Dominion's own hull"), bStopped);
	TestTrue(TEXT("the hull itself is neutral (its parts are Enemy)"), D->GetTeam() == ETDTeam::Neutral);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldKeepOut, "TriadDrift.World.Layout.KeepOutAndUniformAsteroids", TD_TEST_FLAGS)
bool FTDWorldKeepOut::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDGalaxyLayout* G = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
	const UInstancedStaticMeshComponent* Ast = G->FindComponentByClass<UInstancedStaticMeshComponent>();
	if (!TestNotNull(TEXT("asteroids"), Ast)) { return false; }
	const FTDGalaxyLayoutData& L = G->GetLayout();

	TArray<FVector> Keep = { L.PlayerStart.GetLocation() };
	for (const TPair<FName, FTransform>& RP : L.ReturnPoints) { Keep.Add(RP.Value.GetLocation()); }
	for (const FTDGatePairSpec& Pair : L.Gates) { Keep.Add(Pair.A.GetLocation()); Keep.Add(Pair.B.GetLocation()); }

	int32 Violations = 0, NonUniform = 0;
	for (int32 i = 0; i < Ast->GetInstanceCount(); ++i)
	{
		FTransform T;
		Ast->GetInstanceTransform(i, T, /*bWorldSpace*/ false);
		const FVector S = T.GetScale3D();
		if (!FMath::IsNearlyEqual(S.X, S.Y, 0.001f) || !FMath::IsNearlyEqual(S.X, S.Z, 0.001f)) { ++NonUniform; }
		const float Radius = S.X * 50.f;
		for (const FVector& K : Keep)
		{
			if (FVector::Dist(T.GetLocation(), K) - Radius < G->AsteroidKeepOut) { ++Violations; }
		}
	}
	TestEqual(TEXT("no asteroid inside a gate / return point / player-start keep-out (default seed)"), Violations, 0);
	TestEqual(TEXT("asteroid scale is uniform (visible rock = its collision sphere)"), NonUniform, 0);
	TestTrue(TEXT("still a proper belt"), Ast->GetInstanceCount() > 100);
	return true;
}

// T1 / T5 playtest: ~3 s after arriving back at a planet orbit the ship popped to near the centre. Cause: the
// orbit gate sat on the straight line home. With the real game instance and the real galaxy, a player ship
// left hands-off at each orbit return point (the wave starting) never moves more than its own flight in a frame.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldNoPopAfterReturn, "TriadDrift.World.RealPath.NoPositionJumpAfterReturningFromAPlanet", TD_TEST_FLAGS)
bool FTDWorldNoPopAfterReturn::RunTest(const FString& Parameters)
{
	for (const FName Return : { TDReturnPoints::VerdanceOrbit, TDReturnPoints::CinderOrbit })
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		if (!TestNotNull(TEXT("game instance"), GI)) { return false; }
		GI->SetPhase(ETDGamePhase::Waves3to4);	// back from the planet: the next waves are on
		ATDGalaxyLayout* G = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
		FTransform Orbit;
		if (!TestTrue(TEXT("orbit return point"), G && G->FindReturnPoint(Return, Orbit))) { return false; }
		ATDShip* Ship = W.Spawn<ATDShip>(Orbit.GetLocation(), Orbit.Rotator());
		Ship->SetPlayerControlled(true);
		W.SpawnPlayerController(Ship);
		FTDFlightInput In;
		In.Throttle = 0.f;	// cruise, hands off
		Ship->SetFlightInput(In);
		if (UTDHealthComponent* H = Ship->GetHealth()) { H->SetInvulnerable(true); }
		float MaxJump = 0.f;
		FVector Prev = Ship->GetActorLocation();
		const float Step = 1.f / 60.f;
		for (int32 i = 0; i < 60 * 12; ++i)
		{
			W.Tick(Step);
			const float Moved = FVector::Dist(Prev, Ship->GetActorLocation());
			MaxJump = FMath::Max(MaxJump, Moved - Ship->GetVelocity().Size() * Step);
			Prev = Ship->GetActorLocation();
		}
		TestTrue(*FString::Printf(TEXT("%s: never jumps (largest move beyond its own flight: %.0f cm)"), *Return.ToString(), MaxJump), MaxJump < 500.f);
		TestEqual(*FString::Printf(TEXT("%s: no silent teleport (T1's watchdog)"), *Return.ToString()), Ship->GetShipMovement()->GetSilentMoveCount(), 0);
		TestTrue(*FString::Printf(TEXT("%s: still on its way home, not at the centre"), *Return.ToString()), FVector::Dist(Ship->GetActorLocation(), Orbit.GetLocation()) < 120000.f);
	}
	return true;
}


// The space ending as a player meets it (ENG-47; effects builder, Nov 14: no hit ever landed): from the trench stage start,
// the reactor is ahead, the line to it is open, and real bolts fired from the start along the ship's nose kill it.
// Both ways into the stage: generators shot down, and a resume / TD.Stage with the defenses already down.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldTrenchRun, "TriadDrift.World.Dominion.TrenchRunKillsTheReactor", TD_TEST_FLAGS)
bool FTDWorldTrenchRun::RunTest(const FString& Parameters)
{
	using namespace TDWorldRealPathTest;
	for (const bool bResume : { false, true })
	{
		const FString Way = bResume ? TEXT("resume") : TEXT("generators shot");
		FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
		UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
		GI->Progress.bDominionRevealed = true; GI->Progress.Phase = ETDGamePhase::DominionAssault;
		GI->Progress.bDominionDefensesDown = bResume;
		ATDDominion* D = W.Spawn<ATDDominion>(FVector(150000.f, -40000.f, 20000.f), FRotator(0.f, -90.f, 0.f));
		D->MaxLaunchedAlive = 0;
		W.Advance(D->WarpInSeconds + 0.2f);
		if (!bResume) { D->DebugDropShields(); W.Advance(0.2f); }
		if (!TestEqual(*(Way + TEXT(": reactor exposed")), (int32)D->GetState(), (int32)ETDDominionState::ReactorExposed)) { continue; }
		ATDDominionReactor* Reactor = D->GetReactor();
		const FTransform Start = D->GetStageStart(true);
		const FVector ToReactor = Reactor->GetActorLocation() - Start.GetLocation();
		const float Dot = (float)FVector::DotProduct(Start.GetRotation().GetForwardVector(), ToReactor.GetSafeNormal());
		TestTrue(*FString::Printf(TEXT("%s: the reactor is straight ahead of the stage start (dot %.3f, %.0f m)"), *Way, Dot, ToReactor.Size() / 100.f), Dot > 0.98f);
		FHitResult Block;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(TDTrenchRunLine), false);
		Q.AddIgnoredActor(Reactor);
		const bool bBlocked = W.GetWorld()->LineTraceSingleByChannel(Block, Start.GetLocation(), Reactor->GetActorLocation(), ECC_Visibility, Q);
		TestFalse(*FString::Printf(TEXT("%s: nothing between the start and the reactor (blocked by %s)"), *Way,
			bBlocked && Block.GetComponent() ? *Block.GetComponent()->GetReadableName() : TEXT("-")), bBlocked);
		// Fire along the nose from the start, as a player holding the trigger would.
		const FVector Aim = Start.GetLocation() + Start.GetRotation().GetForwardVector() * ToReactor.Size();
		const float Before = Reactor->GetTotalHealth01();
		for (int32 i = 0; i < 40 && Reactor->IsAlive(); ++i)
		{
			Shoot(W, Start.GetLocation(), Aim, 600.f);
			W.Advance(0.4f);
			if (i == 2)
			{
				// ~370 m at 400 m/s: the first bolt lands after ~0.9 s.
				TestTrue(*FString::Printf(TEXT("%s: the first bolts from the start hit the reactor (%.3f -> %.3f)"), *Way, Before, Reactor->GetTotalHealth01()), Reactor->GetTotalHealth01() < Before);
			}
		}
		TestFalse(*(Way + TEXT(": the reactor dies to real bolts from the trench start")), Reactor->IsAlive());
	}
	return true;
}


// ENG-47 balance: the real squad, from the trench stage start, attacks the exposed reactor with its real weapons and NO
// invulnerability. A competent run kills it inside 45 s, and the flown ship is still flying when it does.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldTrenchSurvivable, "TriadDrift.World.Dominion.TrenchRunIsSurvivable", TD_TEST_FLAGS)
bool FTDWorldTrenchSurvivable::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
	APlayerController* PC = W.SpawnPlayerController();
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector(150000.f, 0.f, 3000.f)));
	GI->Progress.bDominionRevealed = true;
	GI->Progress.Phase = ETDGamePhase::DominionAssault;
	W.Advance(D->ProgressPollSeconds + D->WarpInSeconds + 0.5f);
	D->DebugDropShields();
	W.Advance(0.2f);
	if (!TestEqual(TEXT("reactor exposed"), (int32)D->GetState(), (int32)ETDDominionState::ReactorExposed)) { return false; }
	ATDDominionReactor* Reactor = D->GetReactor();
	const FTransform Start = D->GetStageStart(true);
	Squad->SpawnSquad(Start);
	ATDShip* Lead = TDFinaleCombat::ActiveShip(W);
	if (!TestNotNull(TEXT("flown ship"), Lead)) { return false; }
	const double T0 = W.GetWorld()->GetTimeSeconds();
	const bool bKilled = TDFinaleCombat::Attack(W, Reactor, Start.GetLocation(), 30.f, 45.f, [Reactor] { return !Reactor->IsAlive(); }, /*bInvulnerable*/ false);
	TDFinaleCombat::Release(W);
	const float Took = (float)(W.GetWorld()->GetTimeSeconds() - T0);
	ATDShip* Now = TDFinaleCombat::ActiveShip(W);
	const UTDHealthComponent* H = Lead->GetHealth();
	AddInfo(FString::Printf(TEXT("trench run: reactor %s after %.1f s; flown ship %s (hull %.0f, shield %.0f)"), bKilled ? TEXT("destroyed") : TEXT("alive"),
		Took, Now == Lead ? TEXT("the same") : TEXT("switched / retaken"), H ? H->GetHull() : -1.f, H ? H->GetShield() : -1.f));
	TestTrue(TEXT("the squad kills the reactor from the trench start inside 45 s, real weapons, no god mode"), bKilled);
	TestTrue(TEXT("the flown ship survives the run (no switch or retake)"), Now == Lead && H && H->IsAlive());
	return true;
}

// Lead (Nov 14, M6 played live): from the trench start, holding forward rammed the reactor after ~6 s at cruise and
// threw the ship off it. The reactor hold eases the flown ship to rest short of the core; firing kills it from there.
namespace TDReactorHoldTest
{
	ATDDominion* Exposed(FTDTestWorld& W)
	{
		UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
		GI->Progress.bDominionRevealed = true; GI->Progress.Phase = ETDGamePhase::DominionAssault;
		ATDDominion* D = W.Spawn<ATDDominion>(FVector(150000.f, -40000.f, 20000.f), FRotator(0.f, -90.f, 0.f));
		D->MaxLaunchedAlive = 0;
		D->BoardingMode = ETDBoardingMode::ForceOff;
		W.Advance(D->WarpInSeconds + 0.2f);
		D->DebugDropShields();
		W.Advance(0.2f);
		return D;
	}

	ATDShip* FlownAtTrenchStart(FTDTestWorld& W, ATDDominion* D)
	{
		const FTransform Start = D->GetStageStart(true);
		ATDShip* Ship = W.Spawn<ATDShip>(Start.GetLocation(), Start.Rotator());
		if (!Ship) { return nullptr; }
		Ship->SetPlayerControlled(true);
		D->SetFlownShipOverride(Ship);
		FTDFlightInput In;
		In.Throttle = 1.f;	// holding forward
		Ship->SetFlightInput(In);
		return Ship;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReactorHoldNoContact, "TriadDrift.World.Dominion.ReactorHoldStraightRunNeverContactsAndFireKills", TD_TEST_FLAGS)
bool FTDReactorHoldNoContact::RunTest(const FString& Parameters)
{
	using namespace TDWorldRealPathTest;
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	ATDDominion* D = TDReactorHoldTest::Exposed(W);
	if (!TestEqual(TEXT("reactor exposed"), (int32)D->GetState(), (int32)ETDDominionState::ReactorExposed)) { return false; }
	ATDDominionReactor* Reactor = D->GetReactor();
	ATDShip* Ship = TDReactorHoldTest::FlownAtTrenchStart(W, D);
	if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("reactor"), Reactor)) { return false; }
	// 1) Straight at cruise, no fire, for 30 s: the ship comes to rest short of the core.
	float MinGap = BIG_NUMBER;
	bool bHeld = false;
	for (int32 i = 0; i < 30 * 60; ++i)
	{
		W.Tick(1.f / 60.f);
		MinGap = FMath::Min(MinGap, D->GetReactorSurfaceDistance(Ship->GetActorLocation()));
		bHeld |= D->IsReactorHoldActive();
	}
	const float Speed = (float)Ship->GetVelocity().Size();
	AddInfo(FString::Printf(TEXT("closest %.0f m from the reactor's surface, speed after 30 s %.1f m/s"), MinGap / 100.f, Speed / 100.f));
	TestTrue(TEXT("the hold took the flown ship"), bHeld);
	TestTrue(*FString::Printf(TEXT("never contacts the reactor (closest %.0f m)"), MinGap / 100.f), MinGap > 1000.f);
	TestTrue(*FString::Printf(TEXT("on station (%.1f m/s)"), Speed / 100.f), Speed < 300.f);
	// 2) A competent run: holding fire along the nose from where it holds kills the reactor inside 45 s.
	float T = 0.f;
	while (Reactor->IsAlive() && T < 45.f)
	{
		const FVector From = Ship->GetActorLocation() + Ship->GetActorForwardVector() * 800.f;
		Shoot(W, From, From + Ship->GetActorForwardVector() * 30000.f, 600.f);
		W.Advance(0.4f);
		T += 0.4f;
	}
	AddInfo(FString::Printf(TEXT("reactor %s after %.1f s of fire from the hold"), Reactor->IsAlive() ? TEXT("alive") : TEXT("destroyed"), T));
	TestFalse(TEXT("firing from the hold kills the reactor inside 45 s"), Reactor->IsAlive());
	TestFalse(TEXT("the hold lets go once the reactor is dead"), D->IsReactorHoldActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReactorHoldRelease, "TriadDrift.World.Dominion.ReactorHoldReleasesOnTurnAwayAndBoost", TD_TEST_FLAGS)
bool FTDReactorHoldRelease::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	ATDDominion* D = TDReactorHoldTest::Exposed(W);
	ATDShip* Ship = TDReactorHoldTest::FlownAtTrenchStart(W, D);
	if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("reactor"), D->GetReactor())) { return false; }
	for (int32 i = 0; i < 20 * 60 && !D->IsReactorHoldActive(); ++i) { W.Tick(1.f / 60.f); }
	if (!TestTrue(TEXT("held near the reactor"), D->IsReactorHoldActive())) { return false; }
	const UTDShipMovementComponent* Move = Ship->GetShipMovement();
	// Turn away: released on the next tick.
	const FRotator Facing = Ship->GetActorRotation();
	Ship->SetActorRotation(Facing + FRotator(0.f, 180.f, 0.f));
	W.Tick(1.f / 60.f);
	TestFalse(TEXT("turning away releases the hold at once"), D->IsReactorHoldActive());
	TestFalse(TEXT("no speed override left on the ship"), Move && Move->HasSpeedOverrideFor(TEXT("ReactorHold")));
	// Face it again: held again; boost: released.
	Ship->SetActorRotation(Facing);
	W.Tick(1.f / 60.f);
	TestTrue(TEXT("facing it again re-engages"), D->IsReactorHoldActive());
	FTDFlightInput In;
	In.Throttle = 1.f;
	In.bBoost = true;
	Ship->SetFlightInput(In);
	W.Tick(1.f / 60.f);
	TestFalse(TEXT("boost releases the hold at once"), D->IsReactorHoldActive());
	TestFalse(TEXT("no speed override left after boost"), Move && Move->HasSpeedOverrideFor(TEXT("ReactorHold")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
