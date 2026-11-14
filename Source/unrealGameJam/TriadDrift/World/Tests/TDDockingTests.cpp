// ENG-29 docking: the boarding finale opens the bay when the generators fall, the tractor pulls a ship in the cone
// (never locking it before the mouth), the glide hands off to the interior exactly once, and the space ending stays
// the fallback. Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Docking

#include "Misc/AutomationTest.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "NiagaraSystem.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/World/TDWorldBarks.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDockingBay.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Story/TDStorySubsystem.h"

namespace TDDockingTest
{
	/** A revealed Dominion with its generators destroyed, in BoardingMode Mode. */
	ATDDominion* DefensesDown(FTDTestWorld& W, ETDBoardingMode Mode)
	{
		UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
		GI->Progress.bDominionRevealed = true;
		GI->Progress.Phase = ETDGamePhase::DominionAssault;
		ATDDominion* D = W.Spawn<ATDDominion>(FVector(400000.f, 0.f, 0.f));
		D->BoardingMode = Mode;
		AActor* Killer = W.Spawn<AActor>(FVector(300000.f, 0.f, 0.f));
		W.Advance(D->ProgressPollSeconds + D->WarpInSeconds + 0.5f);
		for (ATDShieldGenerator* G : D->GetGenerators()) { UTDStatics::ApplyTDDamage(G, 1000000.f, ETDDamageKind::Explosive, Killer, Killer); W.Advance(0.2f); }
		W.Advance(0.5f);
		return D;
	}

	/** A ship at Along cm out of the bay's mouth, Lateral cm off the axis, nose YawOffDeg off the mouth. */
	ATDShip* ShipInCone(FTDTestWorld& W, ATDDockingBay* Bay, float Along, float Lateral, float YawOffDeg)
	{
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Bay->GetOutward()).GetSafeNormal();
		const FVector Loc = Bay->GetMouth() + Bay->GetOutward() * Along + Right * Lateral;
		ATDShip* Ship = W.Spawn<ATDShip>(Loc);
		if (!Ship) { return nullptr; }
		const FRotator Face = (Bay->GetMouth() - Loc).Rotation() + FRotator(0.f, YawOffDeg, 0.f);
		Ship->SetActorRotation(Face);
		Ship->SetPlayerControlled(true);
		Bay->SetShipOverride(Ship);
		return Ship;
	}

	float LateralOf(const ATDDockingBay* Bay, const FVector& P)
	{
		const FVector D = P - Bay->GetMouth();
		return (D - FVector::DotProduct(D, Bay->GetOutward()) * Bay->GetOutward()).Size();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingFallback, "TriadDrift.World.Docking.SpaceEndingStaysTheFallback", TD_TEST_FLAGS)
bool FTDDockingFallback::RunTest(const FString&)
{
	for (const ETDBoardingMode Mode : { ETDBoardingMode::Auto, ETDBoardingMode::ForceOff })
	{
		// Auto in a test world (/Temp) and ForceOff (td.Finale.Boarding 0) both keep today's reactor ending.
		FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
		ATDDominion* D = TDDockingTest::DefensesDown(W, Mode);
		TestFalse(TEXT("no boarding"), D->UsesBoardingFinale());
		TestEqual(TEXT("reactor exposed"), (int32)D->GetState(), (int32)ETDDominionState::ReactorExposed);
		TestNull(TEXT("no docking bay"), D->GetDockingBay());
		TestTrue(TEXT("the reactor is open to fire"), D->GetReactor() && D->GetReactor()->IsExposed());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingOpens, "TriadDrift.World.Docking.GeneratorsDownOpenTheBay", TD_TEST_FLAGS)
bool FTDDockingOpens::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	TestEqual(TEXT("docking open"), (int32)D->GetState(), (int32)ETDDominionState::DockingOpen);
	ATDDockingBay* Bay = D->GetDockingBay();
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	TestTrue(TEXT("bay open"), Bay->IsOpen());
	TestTrue(TEXT("the reactor stays shielded (the core fight is inside)"), !D->GetReactor() || !D->GetReactor()->IsExposed());
	// The mouth is on the bow face, pointing out along the Dominion's nose.
	const FVector Local = D->GetActorTransform().InverseTransformPosition(Bay->GetMouth());
	if (D->GetHullMesh() && D->GetHullMesh()->DoesSocketExist(D->BaySocket))
	{
		// An art hull with a Bay socket (r2): the mouth is where the art says.
		const FVector Sock = D->GetActorTransform().InverseTransformPosition(D->GetHullMesh()->GetSocketLocation(D->BaySocket));
		TestTrue(*FString::Printf(TEXT("mouth on the Bay socket (%s vs %s)"), *Local.ToCompactString(), *Sock.ToCompactString()), Local.Equals(Sock, 10.f));
	}
	else
	{
		TestTrue(*FString::Printf(TEXT("mouth on the bow (x %.0f vs hull max %.0f)"), Local.X, D->GetLocalHullBounds().Max.X), FMath::IsNearlyEqual(Local.X, D->GetLocalHullBounds().Max.X, 10.f));
	}
	TestTrue(TEXT("faces out of the bow"), FVector::DotProduct(Bay->GetOutward(), D->GetActorForwardVector()) > 0.99f);
	// T1 (docking2/H2_10): the glide flew the camera through a solid lit frame slab. The opening must stay clear.
	TestTrue(TEXT("no lit frame part crosses the mouth within 15 m of the axis"), Bay->IsMouthClear(1500.f));
	// The approach stays a fight: launches and the hyperdrive lock carry on.
	TestTrue(TEXT("still a hot approach (stage 1)"), D->GetStage() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingTractor, "TriadDrift.World.Docking.TractorPullsButNeverLocksBeforeTheMouth", TD_TEST_FLAGS)
bool FTDDockingTractor::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	ATDDockingBay* Bay = D ? D->GetDockingBay() : nullptr;
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	TestEqual(TEXT("outside the cone: no pull"), Bay->GetTractorStrength(Bay->GetMouth() + Bay->GetOutward() * 100000.f + FVector(0.f, 0.f, 200000.f)), 0.f);
	TestEqual(TEXT("behind the mouth: no pull"), Bay->GetTractorStrength(Bay->GetMouth() - Bay->GetOutward() * 1000.f), 0.f);

	// 1.8 km out, 250 m off the axis, nose 20 degrees off: the tractor slows, softens and lines the ship up.
	ATDShip* Ship = TDDockingTest::ShipInCone(W, Bay, 180000.f, 25000.f, 20.f);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	const float Lat0 = TDDockingTest::LateralOf(Bay, Ship->GetActorLocation());
	W.Advance(1.f);
	TestTrue(*FString::Printf(TEXT("held (pull %.2f)"), Bay->GetCurrentPull()), Bay->GetCurrentPull() > 0.2f);
	TestTrue(TEXT("speed override on"), Ship->GetShipMovement() && Ship->GetShipMovement()->HasSpeedOverrideFor(ATDDockingBay::TractorOverride));
	TestFalse(TEXT("never locked before the mouth"), Ship->IsFlightParked());
	W.Advance(2.f);
	TestTrue(*FString::Printf(TEXT("lined up (%.0f -> %.0f cm off the axis)"), Lat0, TDDockingTest::LateralOf(Bay, Ship->GetActorLocation())), TDDockingTest::LateralOf(Bay, Ship->GetActorLocation()) < Lat0 * 0.9f);

	// Abort: the pilot flies out of the cone and gets full control back.
	Ship->SetActorLocation(Bay->GetMouth() + Bay->GetOutward() * 150000.f + FVector(0.f, 0.f, 250000.f));
	W.Advance(0.2f);
	TestEqual(TEXT("out of the cone: released"), Bay->GetCurrentPull(), 0.f);
	TestFalse(TEXT("override cleared"), Ship->GetShipMovement()->HasSpeedOverrideFor(ATDDockingBay::TractorOverride));
	TestEqual(TEXT("still open, nobody docked"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Open);
	return true;
}

// T7's playtest (Nov 14): a human approach came in from the side, outside the 35 degree cone, and never docked. Near the
// mouth the tractor takes a ship up to CaptureHalfAngleDeg off the axis; farther out the cone stays as it was. And the
// open bay shows an approach waypoint on its axis.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingCapture, "TriadDrift.World.Docking.SideApproachNearTheMouthIsCaptured", TD_TEST_FLAGS)
bool FTDDockingCapture::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	ATDDockingBay* Bay = D ? D->GetDockingBay() : nullptr;
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	const FVector Side = FVector::CrossProduct(Bay->GetOutward(), FVector::UpVector).GetSafeNormal();
	auto At = [Bay, Side](float Dist, float Deg) { const float R = FMath::DegreesToRadians(Deg); return Bay->GetMouth() + (Bay->GetOutward() * FMath::Cos(R) + Side * FMath::Sin(R)) * Dist; };
	TestTrue(TEXT("300 m out, 60 degrees off the axis: captured"), Bay->GetTractorStrength(At(30000.f, 60.f)) > 0.f);
	TestEqual(TEXT("1 km out, 60 degrees off the axis: the plain cone, no pull"), Bay->GetTractorStrength(At(100000.f, 60.f)), 0.f);
	TestEqual(TEXT("behind the bow stays out"), Bay->GetTractorStrength(At(30000.f, 100.f)), 0.f);
	bool bMarker = false;
	TArray<AActor*> Attached;
	Bay->GetAttachedActors(Attached);
	for (const AActor* A : Attached) { if (A && A->ActorHasTag(TDTags::ObjectiveMarker)) { bMarker = true; } }
	TestTrue(TEXT("the open bay shows an approach waypoint on its axis"), bMarker);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingHandOff, "TriadDrift.World.Docking.GlideHandsOffToTheInteriorOnce", TD_TEST_FLAGS)
bool FTDDockingHandOff::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	ATDDockingBay* Bay = D ? D->GetDockingBay() : nullptr;
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	ATDShip* Ship = TDDockingTest::ShipInCone(W, Bay, 20000.f, 1000.f, 0.f);	// 200 m out, near the axis
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	W.Advance(0.1f);
	TestEqual(TEXT("gliding"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Gliding);
	TestTrue(TEXT("controls locked for the glide"), Ship->IsFlightParked());
	const UTDHealthComponent* H = UTDStatics::GetHealth(Ship);
	TestTrue(TEXT("invulnerable in the glide"), H && H->IsInvulnerable());
	W.Advance(Bay->GlideSeconds + Bay->DockingLinesHoldSeconds + 1.f);
	TestEqual(TEXT("docked"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Docked);
	TestEqual(TEXT("exactly one travel request"), Bay->GetTravelRequests(), 1);
	TestTrue(TEXT("travels to the interior"), GI->Travels.Num() == 1 && GI->Travels[0] == ETDDestination::DominionInterior);
	TestTrue(TEXT("phase DominionInterior"), GI->Progress.Phase == ETDGamePhase::DominionInterior);
	W.Advance(2.f);
	TestEqual(TEXT("still one travel"), GI->Travels.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingRetake, "TriadDrift.World.Docking.RetakeKeepsTheBayOpenAndLetsGo", TD_TEST_FLAGS)
bool FTDDockingRetake::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	ATDDockingBay* Bay = D ? D->GetDockingBay() : nullptr;
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	ATDShip* Ship = TDDockingTest::ShipInCone(W, Bay, 20000.f, 1000.f, 0.f);
	W.Advance(0.1f);
	TestEqual(TEXT("gliding"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Gliding);
	Bay->ResetForNewTake();	// a CUT lands mid-glide (the take restart)
	TestEqual(TEXT("back to open"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Open);
	TestFalse(TEXT("controls back"), Ship && Ship->IsFlightParked());
	TestEqual(TEXT("no travel"), Bay->GetTravelRequests(), 0);
	TestEqual(TEXT("still the boarding state"), (int32)D->GetState(), (int32)ETDDominionState::DockingOpen);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDominionStoryBeats, "TriadDrift.World.Dominion.GeneratorsFireTheirTransmissions", TD_TEST_FLAGS)
bool FTDDominionStoryBeats::RunTest(const FString&)
{
	// T3's story beats: A4_GeneratorDown1..4 by count down as the generators fall.
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOff);
	const UTDStorySubsystem* Story = UTDStorySubsystem::Get(D);
	if (!TestNotNull(TEXT("story subsystem"), Story)) { return false; }
	// Nov 14 (T3 / T4): the first generator speaks the screenplay's G6_Kato_ShieldWeakening; A4_GeneratorDown1 is retired.
	TestTrue(TEXT("G6_Kato_ShieldWeakening played (first generator)"), Story->HasPlayed(TEXT("G6_Kato_ShieldWeakening")));
	TestFalse(TEXT("A4_GeneratorDown1 retired"), Story->HasPlayed(TEXT("A4_GeneratorDown1")));
	for (int32 i = 2; i <= 4; ++i)
	{
		const FName Id(*FString::Printf(TEXT("A4_GeneratorDown%d"), i));
		TestTrue(*FString::Printf(TEXT("%s played"), *Id.ToString()), Story->HasPlayed(Id));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingWingmen, "TriadDrift.World.Docking.WingmenHoldOutsideThePlayerEntersAlone", TD_TEST_FLAGS)
bool FTDDockingWingmen::RunTest(const FString&)
{
	// Gameplay 06: when docking commits, the wingmen break off and orbit outside the bay; a retake hands them back.
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	ATDDockingBay* Bay = D ? D->GetDockingBay() : nullptr;
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	APlayerController* PC = W.SpawnLocalPlayerController();
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	Squad->SetSquadPlayerController(PC);
	const FVector Start = Bay->GetMouth() + Bay->GetOutward() * 20000.f;
	Squad->SpawnSquad(FTransform((-Bay->GetOutward()).Rotation(), Start));
	W.Advance(0.3f);
	ATDShip* Lead = Squad->GetActiveShip();
	if (!TestNotNull(TEXT("lead"), Lead)) { return false; }
	Lead->SetActorLocation(Start);
	Bay->SetShipOverride(Lead);
	W.Advance(0.1f);
	TestEqual(TEXT("gliding"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Gliding);
	TestEqual(TEXT("both wingmen held outside"), Bay->GetHeldWingmenCount(), 2);
	W.Advance(0.5f);	// mid-glide (a docked bay ignores retakes: the travel is under way)
	for (const ETDShipId Id : { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion })
	{
		const ATDShip* S = Squad->GetShip(Id);
		if (!S || S == Lead) { continue; }
		const float Out = FVector::DotProduct(S->GetActorLocation() - Bay->GetMouth(), Bay->GetOutward());
		TestTrue(*FString::Printf(TEXT("a wingman stays out of the bay (%.0f m out)"), Out / 100.f), Out > 15000.f);
	}
	Bay->ResetForNewTake();	// a CUT before the travel: they fly again
	TestEqual(TEXT("released on a retake"), Bay->GetHeldWingmenCount(), 0);
	for (const ETDShipId Id : { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion })
	{
		const ATDShip* S = Squad->GetShip(Id);
		if (S && S != Lead) { TestFalse(TEXT("a released wingman flies"), S->IsFlightParked()); }
	}
	return true;
}


// ENG-43: dialogs.md Bark_Dom_GeneratorDown (each but the last, whose line is the field's), Bark_Dom_FieldDown (all four
// down) and Bark_Dom_Tractor (the tractor takes hold, once per take).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDominionBarks, "TriadDrift.World.Barks.GeneratorsFieldAndTractor", TD_TEST_FLAGS)
bool FTDDominionBarks::RunTest(const FString&)
{
	FTDCombatBarks::ResetRequestCounts();
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	GI->Progress.bDominionRevealed = true; GI->Progress.Phase = ETDGamePhase::DominionAssault;
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
	D->BoardingMode = ETDBoardingMode::ForceOn;
	W.Advance(D->ProgressPollSeconds + D->WarpInSeconds + 0.5f);
	for (ATDShieldGenerator* G : D->GetGenerators())	// one at a time, as in play
	{
		if (G && G->GetHealth()) { G->GetHealth()->Kill(); }
		W.Advance(0.5f);
	}
	ATDDockingBay* Bay = D->GetDockingBay();
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	TestEqual(TEXT("Bark_Dom_GeneratorDown retired (Command's A4_GeneratorDown2-4 and Kato's G6 line cover the count)"), FTDCombatBarks::GetRequestCount(TDWorldBarkIds::DomGeneratorDown), 0);
	if (const UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld()))
	{
		TestTrue(TEXT("G6_Kato_MarkingTargets when the generators showed"), Story->HasPlayed(TEXT("G6_Kato_MarkingTargets")));
		TestTrue(TEXT("G6_Kato_ShieldWeakening on the first generator down"), Story->HasPlayed(TEXT("G6_Kato_ShieldWeakening")));
	}
	TestEqual(TEXT("Bark_Dom_FieldDown once"), FTDCombatBarks::GetRequestCount(TDWorldBarkIds::DomFieldDown), 1);
	ATDShip* Ship = TDDockingTest::ShipInCone(W, Bay, 180000.f, 25000.f, 20.f);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	W.Advance(1.f);
	TestEqual(TEXT("Bark_Dom_Tractor once as the pull takes hold"), FTDCombatBarks::GetRequestCount(TDWorldBarkIds::DomTractor), 1);
	W.Advance(1.f);
	TestEqual(TEXT("not repeated while held"), FTDCombatBarks::GetRequestCount(TDWorldBarkIds::DomTractor), 1);
	return true;
}

// ENG-43: dialogs.md Bark_Dom_GeneratorHit (a generator at half health, once each).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGeneratorHitBark, "TriadDrift.World.Barks.GeneratorAtHalf", TD_TEST_FLAGS)
bool FTDGeneratorHitBark::RunTest(const FString&)
{
	FTDCombatBarks::ResetRequestCounts();
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	GI->Progress.bDominionRevealed = true; GI->Progress.Phase = ETDGamePhase::DominionAssault;
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
	W.Advance(D->ProgressPollSeconds + D->WarpInSeconds + 0.5f);
	ATDShieldGenerator* G = D->GetGenerators().Num() > 0 ? D->GetGenerators()[0] : nullptr;
	if (!TestNotNull(TEXT("generator"), G)) { return false; }
	// Real player bolts from outside (the generators only take hits that reach them).
	FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::EnemyHeavyBolt).Projectile;
	Spec.SplashRadius = 0.f; Spec.ProximityFuseRadius = 0.f; Spec.bHoming = false; Spec.Speed = 40000.f; Spec.Range = 40000.f;
	Spec.bInheritShooterVelocity = false; Spec.DamageKind = ETDDamageKind::Explosive;
	const FVector Out = (G->GetActorLocation() - D->GetActorLocation()).GetSafeNormal() * 8000.f;
	auto Shoot = [&](float Amount) { Spec.Damage = Amount; ATDProjectile::Spawn(W.GetWorld(), Spec, FTransform((-Out).Rotation(), G->GetActorLocation() + Out), nullptr, ETDTeam::Player); W.Advance(0.5f); };
	Shoot(G->GetHealth()->MaxHull * 0.3f);
	AddInfo(FString::Printf(TEXT("generator hull after one hit: %.2f"), G->GetHealth()->GetHull01()));
	TestEqual(TEXT("not at 70 %"), FTDCombatBarks::GetRequestCount(TDWorldBarkIds::DomGeneratorHit), 0);
	Shoot(G->GetHealth()->MaxHull * 0.3f);
	TestEqual(TEXT("Bark_Dom_GeneratorHit at half health"), FTDCombatBarks::GetRequestCount(TDWorldBarkIds::DomGeneratorHit), 1);
	Shoot(10.f);
	TestEqual(TEXT("once per generator"), FTDCombatBarks::GetRequestCount(TDWorldBarkIds::DomGeneratorHit), 1);
	return true;
}


// ENG-37: the TractorBeam cue plays as a loop at the bay mouth while the tractor holds the flown ship, and stops when it lets go.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTractorBeamCue, "TriadDrift.World.Docking.TractorBeamPlaysWhileHeld", TD_TEST_FLAGS)
bool FTDTractorBeamCue::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	ATDDockingBay* Bay = D ? D->GetDockingBay() : nullptr;
	if (!TestNotNull(TEXT("bay"), Bay) || !TestNotNull(TEXT("visuals"), D->Visuals.Get())) { return false; }
	D->Visuals = DuplicateObject(D->Visuals.Get(), GetTransientPackage());	// never the shared asset
	D->Visuals->TractorBeam.VFX = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Ambient_Verdance.NS_TD_Ambient_Verdance")));
	TestFalse(TEXT("no beam before a ship is held"), Bay->IsBeamActive());
	ATDShip* Ship = TDDockingTest::ShipInCone(W, Bay, 180000.f, 25000.f, 20.f);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	W.Advance(1.f);
	TestTrue(TEXT("the beam plays while the tractor holds"), Bay->IsBeamActive());
	Ship->SetActorLocation(Bay->GetMouth() + Bay->GetOutward() * 150000.f + FVector(0.f, 0.f, 250000.f));
	W.Advance(0.3f);
	TestFalse(TEXT("and stops when it lets go"), Bay->IsBeamActive());
	return true;
}

// ART-18: the AftermathLightRow cue starts at the hull face nearest the escape line, +X along its outward normal, and
// stops at the hyperdrive jump.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDAftermathLightRow, "TriadDrift.World.Dominion.AftermathLightRowStartsAndStopsAtTheJump", TD_TEST_FLAGS)
bool FTDAftermathLightRow::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	GI->Progress.Phase = ETDGamePhase::Waves1to2;
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(600000.f, 0.f, 0.f));
	W.Advance(D->ProgressPollSeconds + 0.2f);
	if (!TestNotNull(TEXT("visuals"), D->Visuals.Get())) { return false; }
	D->Visuals = DuplicateObject(D->Visuals.Get(), GetTransientPackage());
	D->Visuals->AftermathLightRow.VFX = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Ambient_Verdance.NS_TD_Ambient_Verdance")));
	// The escape line off the Dominion's port side, 5 km away.
	D->StartAftermathLightRow(D->GetActorLocation() - D->GetActorRightVector() * 500000.f);
	TestTrue(TEXT("the light row plays"), D->IsAftermathLightRowActive());
	const FTransform T = D->GetAftermathLightRowTransform();
	TestTrue(*FString::Printf(TEXT("on the port face, facing out (+X = %s)"), *T.GetRotation().GetForwardVector().ToCompactString()),
		T.GetRotation().GetForwardVector().Y < -0.99f && T.GetLocation().Y <= D->GetLocalHullBounds().Min.Y + 1.f);
	if (UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld())) { Bus->OnHyperdriveState.Broadcast(ETDHyperdriveState::Jumping); }
	TestFalse(TEXT("deactivated at the hyperdrive jump"), D->IsAftermathLightRowActive());
	return true;
}


// T1's live review and the Lead (Nov 14): cone edge to commit in about 12 s flown straight (it took ~40 s under fire), and
// the glide never faster than the ship's boost (it lunged at ~340 m/s).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingPace, "TriadDrift.World.Docking.ApproachIsBriskAndTheGlideNeverLunges", TD_TEST_FLAGS)
bool FTDDockingPace::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	ATDDockingBay* Bay = D ? D->GetDockingBay() : nullptr;
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	ATDShip* Ship = TDDockingTest::ShipInCone(W, Bay, Bay->ConeDepth * 0.98f, 0.f, 0.f);	// at the cone's edge, on the axis
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	float T = 0.f, PeakGlide = 0.f, Arrive = 0.f;
	FVector Prev = Ship->GetActorLocation();
	while (T < 40.f && Bay->GetDockingState() != ETDDockingState::Docked)
	{
		const bool bGliding = Bay->GetDockingState() == ETDDockingState::Gliding;
		W.Tick(1.f / 60.f);
		T += 1.f / 60.f;
		const float Speed = (float)FVector::Dist(Prev, Ship->GetActorLocation()) * 60.f;
		Prev = Ship->GetActorLocation();
		if (!bGliding && Bay->GetDockingState() == ETDDockingState::Gliding) { Arrive = Speed; AddInfo(FString::Printf(TEXT("glide starts after %.1f s at %.0f m/s"), T, Speed / 100.f)); TestTrue(*FString::Printf(TEXT("cone edge to commit in about 12 s flown straight (%.1f s)"), T), T <= 12.5f); }
		if (bGliding) { PeakGlide = FMath::Max(PeakGlide, Speed); }
	}
	AddInfo(FString::Printf(TEXT("glide peak %.0f m/s, docked after %.1f s"), PeakGlide / 100.f, T));
	TestTrue(TEXT("docked"), Bay->GetDockingState() == ETDDockingState::Docked);
	TestTrue(*FString::Printf(TEXT("the glide never goes faster than the ship's boost (peak %.0f m/s, arrival %.0f m/s)"), PeakGlide / 100.f, Arrive / 100.f),
		Ship->GetShipMovement() && PeakGlide <= Ship->GetShipMovement()->GetFlightTuning().BoostSpeed * 1.05f);
	return true;
}


// T1 (live, Nov 14): commit to load is ~5 s now, and the load cut the Docking lines. The ship holds over the pad until
// the sequence has finished (or is skipped), at most DockingLinesHoldSeconds, then travels. Story lines run on real time,
// which a headless tick loop outruns, so the test holds through the glide, then skips the sequence.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingWaitsForLines, "TriadDrift.World.Docking.TravelWaitsForTheDockingLines", TD_TEST_FLAGS)
bool FTDDockingWaitsForLines::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	ATDDockingBay* Bay = D ? D->GetDockingBay() : nullptr;
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("story"), Story)) { return false; }
	ATDShip* Ship = TDDockingTest::ShipInCone(W, Bay, 20000.f, 1000.f, 0.f);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	W.Advance(0.2f);
	TestEqual(TEXT("gliding"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Gliding);
	TestTrue(TEXT("the Docking lines start at the commit"), Story->IsSequencePlaying(TEXT("Docking")));
	W.Advance(Bay->GlideSeconds + 0.5f);	// past the glide, inside the hold
	TestTrue(TEXT("still on air"), Story->IsSequencePlaying(TEXT("Docking")));
	TestEqual(TEXT("no travel while the Docking lines are on air"), Bay->GetTravelRequests(), 0);
	TestEqual(TEXT("held over the pad"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Gliding);
	Story->SkipAllowedAfterSeconds = 0.f;	// the ten-second skip guard runs on real time too
	TestTrue(TEXT("skipped"), Story->SkipSequence());
	W.Advance(0.2f);
	TestEqual(TEXT("travels once the lines are done (skipped)"), Bay->GetTravelRequests(), 1);
	TestEqual(TEXT("docked"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Docked);
	return true;
}

#endif
