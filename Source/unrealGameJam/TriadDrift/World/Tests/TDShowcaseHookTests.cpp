// ENG-33 hooks for T3's intro / showcase shots: the Dominion silhouette, a camera-only enemy ship, the warp-in
// telegraph and the gate transit flare, each with no gameplay side effects.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Showcase

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDPortalGate.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Waves/TDWarpInMarker.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "EngineUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDShowcaseDominion, "TriadDrift.World.Showcase.DominionSilhouetteWithoutTheAssault", TD_TEST_FLAGS)
bool FTDShowcaseDominion::RunTest(const FString&)
{
	{
		FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
		ATDDominion* D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
		if (!TestNotNull(TEXT("dominion"), D)) { return false; }
		W.Advance(0.5f);
		TestTrue(TEXT("hidden before the reveal"), D->IsHidden() && D->GetState() == ETDDominionState::Hidden);
		const FVector Home = D->GetActorLocation();
		const FTransform Shot(FRotator(0.f, 30.f, 0.f), FVector(50000.f, 20000.f, 0.f));
		TestTrue(TEXT("showcase accepted while hidden"), D->SetShowcase(true, &Shot));
		TestFalse(TEXT("visible for the shot"), D->IsHidden());
		TestTrue(TEXT("at the shot's transform"), D->GetActorLocation().Equals(Shot.GetLocation(), 1.f));
		TestFalse(TEXT("no collision in a showcase"), D->GetActorEnableCollision());
		W.Advance(1.f);
		TestTrue(TEXT("no state change"), D->GetState() == ETDDominionState::Hidden);
		TestEqual(TEXT("no generators built"), D->GetGenerators().Num(), 0);
		TestTrue(TEXT("restored"), D->SetShowcase(false));
		TestTrue(TEXT("hidden again"), D->IsHidden());
		TestTrue(TEXT("back home"), D->GetActorLocation().Equals(Home, 1.f));
		TestFalse(TEXT("showcase over"), D->IsShowcasing());
	}
	{
		// During the assault the real ship owns everything.
		FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
		UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
		GI->Progress.bDominionRevealed = true;
		GI->Progress.Phase = ETDGamePhase::DominionAssault;
		ATDDominion* D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
		W.Advance(D->WarpInSeconds + 0.5f);
		const FVector Before = D->GetActorLocation();
		const FTransform Shot(FVector::ZeroVector);
		TestFalse(TEXT("refused during the assault"), D->SetShowcase(true, &Shot));
		TestTrue(TEXT("did not move"), D->GetActorLocation().Equals(Before, 1.f));
		TestFalse(TEXT("not showcasing"), D->IsShowcasing());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDShowcaseShip, "TriadDrift.World.Showcase.ShipIsForTheCameraOnly", TD_TEST_FLAGS)
bool FTDShowcaseShip::RunTest(const FString&)
{
	FTDTestWorld W;
	for (const FName Id : { TDEnemyIds::ShardFighter, TDEnemyIds::WallGunship, TDEnemyIds::HiveCarrier })
	{
		const FTransform Where(FRotator(0.f, 90.f, 0.f), FVector(10000.f, 0.f, 0.f));
		ATDEnemyShip* E = ATDWaveDirector::SpawnShowcaseShip(W.GetWorld(), Id, Where);
		const FString N = Id.ToString();
		if (!TestNotNull(*FString::Printf(TEXT("%s spawned"), *N), E)) { continue; }
		TestNull(*FString::Printf(TEXT("%s has no pilot"), *N), E->GetController());
		TestTrue(*FString::Printf(TEXT("%s tagged"), *N), E->ActorHasTag(ATDWaveDirector::ShowcaseTag));
		UTDHealthComponent* H = E->FindComponentByClass<UTDHealthComponent>();
		TestTrue(*FString::Printf(TEXT("%s invulnerable and untargetable"), *N), H && H->IsInvulnerable() && !H->bTargetable);
		const UTDWeaponComponent* Wp = E->FindComponentByClass<UTDWeaponComponent>();
		TestTrue(*FString::Printf(TEXT("%s cannot fire"), *N), !Wp || Wp->AreWeaponsBlocked());
		TestFalse(*FString::Printf(TEXT("%s has no collision"), *N), E->GetActorEnableCollision());
		const float HullBefore = H ? H->GetHull() : 0.f;
		UTDStatics::ApplyTDDamage(E, 99999.f, ETDDamageKind::Explosive, nullptr, nullptr);
		W.Advance(2.f);
		TestTrue(*FString::Printf(TEXT("%s unharmed"), *N), IsValid(E) && H && H->GetHull() == HullBefore);
		TestTrue(*FString::Printf(TEXT("%s stays where the shot put it"), *N), IsValid(E) && E->GetActorLocation().Equals(Where.GetLocation(), 5.f));
		TArray<AActor*> Parts;
		if (IsValid(E)) { E->GetAttachedActors(Parts, true, true); }
		for (AActor* P : Parts)
		{
			const UTDHealthComponent* PH = P->FindComponentByClass<UTDHealthComponent>();
			TestTrue(*FString::Printf(TEXT("%s part %s locked down"), *N, *P->GetName()), !P->GetActorEnableCollision() && (!PH || (PH->IsInvulnerable() && !PH->bTargetable)));
		}
		if (IsValid(E)) { E->Destroy(); }
	}
	TestNull(TEXT("unknown id spawns nothing"), ATDWaveDirector::SpawnShowcaseShip(W.GetWorld(), TEXT("NoSuchEnemy"), FTransform::Identity));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDShowcaseFX, "TriadDrift.World.Showcase.WarpInAndTransitFlareOnly", TD_TEST_FLAGS)
bool FTDShowcaseFX::RunTest(const FString&)
{
	FTDTestWorld W;
	ATDWaveDirector::PlayShowcaseWarpIn(W.GetWorld(), FVector(5000.f, 0.f, 0.f), 3000.f);
	const UTDTravelFX* FX = UTDStatics::LoadPresentation<UTDTravelFX>(TEXT("DA_Travel_FX"));
	if (!FX || FX->EnemyWarpIn.VFX.IsNull())
	{
		int32 Markers = 0;
		for (TActorIterator<ATDWarpInMarker> It(W.GetWorld()); It; ++It) { Markers += It->ActorHasTag(ATDWaveDirector::ShowcaseTag) ? 1 : 0; }
		TestEqual(TEXT("placeholder warp-in marker plays"), Markers, 1);
	}
	int32 Ships = 0;
	for (TActorIterator<ATDEnemyShip> It(W.GetWorld()); It; ++It) { ++Ships; }
	TestEqual(TEXT("a warp-in alone spawns no ship"), Ships, 0);

	ATDPortalGate* Gate = W.Spawn<ATDPortalGate>(FVector(20000.f, 0.f, 0.f));
	if (TestNotNull(TEXT("gate"), Gate))
	{
		Gate->PlayTransitFlare(Gate->GetActorLocation());	// must not need a partner, a traveller or a cooldown
		W.Advance(0.2f);
		TestTrue(TEXT("gate unaffected"), IsValid(Gate));
	}
	return true;
}

#endif
