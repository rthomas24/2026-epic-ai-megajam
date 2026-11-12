// Director's Cut money shots from REAL producers (review B1): T4's shield generators, reactor and
// carrier, T2's portal missile return and Lancer's ram. Each must yield exactly one
// OnHighlightMoment, one FTDDirectorShot and running slow-mo. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Director.Real

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/PlayerController.h"
#include "Misc/ScopeExit.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/Tests/TDSquadTestListener.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/World/TDShieldGenerator.h"

namespace TDDirectorRealTests
{
	/** Counts bus highlights and director shots; checks slow-mo. */
	struct FWatch
	{
		UTDSquadTestListener* L = nullptr;
		UTDDirectorSubsystem* Director = nullptr;
		UWorld* World = nullptr;

		explicit FWatch(UWorld* InWorld) : World(InWorld)
		{
			L = NewObject<UTDSquadTestListener>();
			L->AddToRoot();
			L->Listen(UTDEventBus::Get(World));
			Director = UTDDirectorSubsystem::Get(World);
			if (Director)
			{
				Director->OnDirectorShot.AddDynamic(L, &UTDSquadTestListener::OnDirectorShot);
			}
		}
		~FWatch()
		{
			if (Director)
			{
				Director->OnDirectorShot.RemoveAll(L);
			}
			L->StopListening();
			L->RemoveFromRoot();
		}
		bool SlowMo() const
		{
			const UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(World);
			return Feel && Feel->IsSlowMoActive();
		}
	};

	/** A player-team actor to be the killer (no squad in these worlds, so any player kill counts). */
	AActor* SpawnPlayerKiller(FTDTestWorld& W, const FVector& At)
	{
		return TDPowerTest::SpawnBody(W, ETDTeam::Player, At, FVector(100.f), 100.f, 100.f);
	}
}

using namespace TDDirectorRealTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorRealDominion, "TriadDrift.Director.Real.GeneratorAndReactorKills", TD_TEST_FLAGS)
bool FTDDirectorRealDominion::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("game instance"), GI))
	{
		return false;
	}
	FWatch Watch(W.GetWorld());
	AActor* Player = SpawnPlayerKiller(W, FVector(150000.f, 0.f, 0.f));
	GI->SetDominionState(/*bRevealed*/ true, /*bDefensesDown*/ false);
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
	W.Advance(D->ProgressPollSeconds + D->WarpInSeconds + 0.3f);
	TArray<ATDShieldGenerator*> Generators = D->GetGenerators();
	if (!TestEqual(TEXT("four real generators"), Generators.Num(), 4))
	{
		return false;
	}

	// Each generator kill is a story beat: exactly one highlight + one shot + slow-mo, back to back.
	for (int32 i = 0; i < Generators.Num(); ++i)
	{
		const int32 H0 = Watch.L->Highlights;
		const int32 S0 = Watch.L->Shots;
		UTDStatics::ApplyTDDamage(Generators[i], 100000.f, ETDDamageKind::Explosive, Player, Player);
		W.Tick();
		TestEqual(FString::Printf(TEXT("generator %d: one OnHighlightMoment"), i), Watch.L->Highlights - H0, 1);
		TestEqual(FString::Printf(TEXT("generator %d: one director shot"), i), Watch.L->Shots - S0, 1);
		TestTrue(FString::Printf(TEXT("generator %d: GeneratorKill"), i), Watch.L->LastHighlight == ETDHighlightKind::GeneratorKill);
		TestTrue(FString::Printf(TEXT("generator %d: slow-mo"), i), Watch.SlowMo());
		W.Advance(0.3f);
	}

	W.Advance(2.f);
	ATDDominionReactor* Reactor = D->GetReactor();
	if (!TestNotNull(TEXT("reactor"), Reactor) || !TestTrue(TEXT("reactor exposed after the generators"), Reactor->IsExposed()))
	{
		return false;
	}
	const int32 H0 = Watch.L->Highlights;
	const int32 S0 = Watch.L->Shots;
	for (int32 i = 0; i < 20 && UTDStatics::GetHealth(Reactor) && UTDStatics::GetHealth(Reactor)->IsAlive(); ++i)
	{
		UTDStatics::ApplyTDDamage(Reactor, 100000.f, ETDDamageKind::Explosive, Player, Player);
		W.Advance(0.5f);
	}
	TestTrue(TEXT("reactor destroyed"), UTDStatics::GetHealth(Reactor) && UTDStatics::GetHealth(Reactor)->IsDead());
	TestEqual(TEXT("reactor: one OnHighlightMoment"), Watch.L->Highlights - H0, 1);
	TestEqual(TEXT("reactor: one director shot"), Watch.L->Shots - S0, 1);
	TestTrue(TEXT("ReactorKill at importance 1"), Watch.L->LastHighlight == ETDHighlightKind::ReactorKill && Watch.L->LastImportance == 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorRealCarrier, "TriadDrift.Director.Real.CarrierKill", TD_TEST_FLAGS)
bool FTDDirectorRealCarrier::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	FWatch Watch(W.GetWorld());
	AActor* Player = SpawnPlayerKiller(W, FVector(0.f, 0.f, 0.f));
	ATDEnemyShip* Carrier = TDPowerTest::SpawnEnemyShip(W, TDEnemyIds::HiveCarrier, FVector(60000.f, 0.f, 0.f));
	if (!TestNotNull(TEXT("real carrier"), Carrier))
	{
		return false;
	}
	W.Advance(1.f);
	UTDStatics::ApplyTDDamage(Carrier, 1000000.f, ETDDamageKind::Explosive, Player, Player);
	W.Tick();
	TestEqual(TEXT("one OnHighlightMoment"), Watch.L->Highlights, 1);
	TestEqual(TEXT("one director shot"), Watch.L->Shots, 1);
	TestTrue(TEXT("CarrierKill"), Watch.L->LastHighlight == ETDHighlightKind::CarrierKill);
	TestTrue(TEXT("slow-mo"), Watch.SlowMo());
	TestTrue(TEXT("shot frames the carrier"), Watch.Director->GetCurrentShot().Subject.Get() == Carrier);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorRealMissileReturn, "TriadDrift.Director.Real.PortalMissileReturn", TD_TEST_FLAGS)
bool FTDDirectorRealMissileReturn::RunTest(const FString& Parameters)
{
	// The squad's real Rift (player-flown) opens a real portal pair; an enemy missile flies into it.
	FTDTestWorld W;
	FWatch Watch(W.GetWorld());
	APlayerController* PC = W.SpawnPlayerController();
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform::Identity);
	W.Tick();
	ATDShip* Rift = Squad->GetShip(ETDShipId::Rift);
	Rift->SetFlightParked(true); // hold it at the origin so the geometry is exact
	UTDPortalBlasterPower* Portal = Cast<UTDPortalBlasterPower>(UTDPowerComponent::FindPower(Rift, ETDPowerInput::Signature1));
	if (!TestNotNull(TEXT("Rift's portal blaster"), Portal))
	{
		return false;
	}
	Portal->HandleInputPressed(); // portal A straight ahead (+X)
	Rift->SetActorRotation(FRotator(0.f, 90.f, 0.f));
	Portal->HandleInputPressed(); // portal B to the left (+Y)
	Rift->SetActorRotation(FRotator::ZeroRotator);
	W.Tick();
	const int32 H0 = Watch.L->Highlights;
	const int32 S0 = Watch.L->Shots;

	AActor* Interceptor = TDPowerTest::SpawnBody(W, ETDTeam::Enemy, FVector(20000.f, 0.f, 0.f), FVector(150.f), 100.f, 100.f);
	ATDProjectile* Missile = TDPowerTest::LaunchMissile(W, FVector(15000.f, 0.f, 0.f), -FVector::ForwardVector, Interceptor, Rift);
	TestNotNull(TEXT("missile"), Missile);
	const float At = TDPowerTest::AdvanceUntil(W, [&] { return Watch.L->Highlights > H0; }, 3.f);
	TestTrue(TEXT("the missile went through the portal and was returned"), At > 0.f);
	TestEqual(TEXT("one OnHighlightMoment"), Watch.L->Highlights - H0, 1);
	TestEqual(TEXT("one director shot"), Watch.L->Shots - S0, 1);
	TestTrue(TEXT("MissileReturn"), Watch.L->LastHighlight == ETDHighlightKind::MissileReturn);
	TestTrue(TEXT("slow-mo"), Watch.SlowMo());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorRealRamKill, "TriadDrift.Director.Real.RamKill", TD_TEST_FLAGS)
bool FTDDirectorRealRamKill::RunTest(const FString& Parameters)
{
	// The player flies Lancer and rams a real Shard Fighter with the real Afterburner Ram.
	FTDTestWorld W;
	FWatch Watch(W.GetWorld());
	APlayerController* PC = W.SpawnPlayerController();
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	Squad->SetSquadPlayerController(PC);
	Squad->StartingShip = ETDShipId::Lancer;
	Squad->SpawnSquad(FTransform::Identity);
	W.Tick();
	ATDShip* Lancer = Squad->GetShip(ETDShipId::Lancer);
	ATDEnemyShip* Fighter = TDPowerTest::SpawnEnemyShip(W, TDEnemyIds::ShardFighter, Lancer->GetActorLocation() + Lancer->GetActorForwardVector() * 9000.f);
	if (!TestNotNull(TEXT("real fighter"), Fighter))
	{
		return false;
	}
	Fighter->SetFlightParked(true); // a sitting duck, so the ram line is exact
	UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true);
	W.Tick();
	UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
	const float At = TDPowerTest::AdvanceUntil(W, [&] { return UTDStatics::GetHealth(Fighter) && UTDStatics::GetHealth(Fighter)->IsDead(); }, 3.f);
	TestTrue(TEXT("the ram killed the fighter"), At > 0.f);
	W.Tick();
	TestEqual(TEXT("exactly one OnHighlightMoment (no double ram highlight)"), Watch.L->Highlights, 1);
	TestEqual(TEXT("one director shot"), Watch.L->Shots, 1);
	TestTrue(TEXT("RamKill"), Watch.L->LastHighlight == ETDHighlightKind::RamKill);
	TestTrue(TEXT("slow-mo"), Watch.SlowMo());
	TestTrue(TEXT("shot frames the rammer"), Watch.Director->GetCurrentShot().Subject.Get() == Lancer);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
