// ENG-38 stage 1 (T1): the white-cover route and arrival from the entry point.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Travel.Descent

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Travel/TDPlanetTravelRig.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/World/TDPlanetEntry.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"

namespace TDDescentTest
{
	static const TCHAR* MapA = TEXT("/Engine/Maps/Entry.Entry");
	static const TCHAR* MapT = TEXT("/Engine/Maps/Templates/Template_Default.Template_Default");

	void SetLegacy(int32 V)
	{
		if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Descent.Legacy"))) { C->Set(V, ECVF_SetByCode); }
	}

	/** A standalone game instance with every destination on an engine map and a resolvable transition map. */
	struct FGI
	{
		UTDFlowSettings* S = GetMutableDefault<UTDFlowSettings>();
		TMap<ETDDestination, FSoftObjectPath> Maps = S->DestinationMaps;
		FSoftObjectPath Transition = S->TransitionMap;
		float Fade = S->TravelFadeSeconds;
		float MinT = S->MinTransitionSeconds;
		UTDGameInstance* GI = nullptr;
		TArray<FString> Opened;

		FGI()
		{
			for (const ETDDestination D : { ETDDestination::MainMenu, ETDDestination::Galaxy, ETDDestination::Verdance, ETDDestination::Cinder, ETDDestination::DominionInterior, ETDDestination::Victory })
			{
				S->DestinationMaps.Add(D, FSoftObjectPath(MapA));
			}
			S->TransitionMap = FSoftObjectPath(MapT);
			S->TravelFadeSeconds = 0.6f;
			S->MinTransitionSeconds = 1.f;
			GI = NewObject<UTDGameInstance>(GEngine);
			GI->AddToRoot();
			GI->SetSaveSlotOverride(TEXT("TriadDrift_DescentTest"));
			static int32 N = 0;
			GI->InitializeStandalone(FName(*FString::Printf(TEXT("TDDescentTestWorld_%d"), ++N)));
			GI->SetSaveSlotOverride(TEXT("TriadDrift_DescentTest"));
			GI->OpenLevelOverride = [this](const FString& P) { Opened.Add(P); };
		}
		~FGI()
		{
			GI->OpenLevelOverride = nullptr;
			GI->DeleteSave();
			UWorld* World = GI->GetWorld();
			GI->Shutdown();
			if (World) { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); }
			GI->RemoveFromRoot();
			S->DestinationMaps = Maps;
			S->TransitionMap = Transition;
			S->TravelFadeSeconds = Fade;
			S->MinTransitionSeconds = MinT;
		}
		UTDDescentSubsystem* D() const { return GI->GetSubsystem<UTDDescentSubsystem>(); }

		/** Travel From -> To; returns the first map opened (after ticking the flow 1 s). */
		FString FirstOpened(ETDDestination From, ETDDestination To)
		{
			Opened.Reset();
			GI->NotifyLevelEntered(From);
			if (!GI->TravelTo(To, NAME_None)) { return TEXT("refused"); }
			for (int32 i = 0; i < 10 && Opened.Num() == 0; ++i) { GI->TickFlow(0.1f); }
			const FString First = Opened.Num() ? Opened[0] : FString();
			if (First == FSoftObjectPath(MapT).GetLongPackageName())
			{
				GI->NotifyMapLoaded(First); // on the loading screen: hold, then the target
				for (int32 i = 0; i < 20 && Opened.Num() < 2; ++i) { GI->TickFlow(0.1f); }
			}
			GI->NotifyMapLoaded(FSoftObjectPath(MapA).GetLongPackageName());
			return First;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1DescentRoute, "TriadDrift.Travel.Descent.CoveredRouteOnlyForPlanetsAndLegacySwitch", TD_TEST_FLAGS)
bool FTDT1DescentRoute::RunTest(const FString& Parameters)
{
	using namespace TDDescentTest;
	ON_SCOPE_EXIT { SetLegacy(0); };
	SetLegacy(0);
	FGI G;
	if (!TestNotNull(TEXT("descent subsystem"), G.D())) { return false; }
	const FString Map = FSoftObjectPath(MapA).GetLongPackageName();
	const FString Trans = FSoftObjectPath(MapT).GetLongPackageName();

	// Covered: galaxy <-> planet go straight to the map (no L_Transition), with the cover up instead of a camera fade.
	G.GI->NotifyLevelEntered(ETDDestination::Galaxy);
	TestTrue(TEXT("galaxy -> Verdance starts"), G.GI->TravelTo(ETDDestination::Verdance, NAME_None));
	TestTrue(TEXT("covered travel"), G.GI->IsCoveredTravel());
	TestTrue(TEXT("cover ramping or held"), G.D()->GetCoverPhase() == ETDDescentCover::RampIn || G.D()->GetCoverPhase() == ETDDescentCover::Held);
	G.GI->TickFlow(0.5f);
	TestEqual(TEXT("galaxy -> Verdance opens the planet first (no L_Transition)"), G.Opened.Num() ? G.Opened[0] : FString(), Map);
	G.GI->NotifyMapLoaded(Map);
	TestFalse(TEXT("arrived"), G.GI->IsTravelPending());
	TestEqual(TEXT("Verdance -> galaxy: no L_Transition"), G.FirstOpened(ETDDestination::Verdance, ETDDestination::Galaxy), Map);
	TestEqual(TEXT("galaxy -> Cinder: no L_Transition"), G.FirstOpened(ETDDestination::Galaxy, ETDDestination::Cinder), Map);
	// Unchanged: the interior, the menu and victory keep the fade and the loading screen.
	TestEqual(TEXT("galaxy -> interior: L_Transition as before"), G.FirstOpened(ETDDestination::Galaxy, ETDDestination::DominionInterior), Trans);
	TestEqual(TEXT("galaxy -> menu: L_Transition as before"), G.FirstOpened(ETDDestination::Galaxy, ETDDestination::MainMenu), Trans);
	TestEqual(TEXT("galaxy -> victory: L_Transition as before"), G.FirstOpened(ETDDestination::Galaxy, ETDDestination::Victory), Trans);

	// td.Descent.Legacy 1: today's route for the planets too.
	SetLegacy(1);
	TestEqual(TEXT("legacy: galaxy -> Verdance through L_Transition"), G.FirstOpened(ETDDestination::Galaxy, ETDDestination::Verdance), Trans);
	TestEqual(TEXT("legacy: Verdance -> galaxy through L_Transition"), G.FirstOpened(ETDDestination::Verdance, ETDDestination::Galaxy), Trans);
	SetLegacy(0);

	// A travel that falls through drops the cover (never stuck white).
	AddExpectedMessage(TEXT("superseded"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
	G.GI->NotifyLevelEntered(ETDDestination::Galaxy);
	TestTrue(TEXT("travel again"), G.GI->TravelTo(ETDDestination::Verdance, NAME_None));
	G.GI->NotifyMapLoaded(TEXT("/Game/Somewhere/Else"));
	TestFalse(TEXT("superseded"), G.GI->IsTravelPending());
	TestEqual(TEXT("cover dropping after a failed travel"), G.D()->GetCoverPhase(), ETDDescentCover::Aborting);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1DescentArrival, "TriadDrift.Travel.Descent.ArrivalFromTheEntryPointConsumedOncePerVisit", TD_TEST_FLAGS)
bool FTDT1DescentArrival::RunTest(const FString& Parameters)
{
	using namespace TDDescentTest;
	FGI G;
	UTDDescentSubsystem* D = G.D();
	if (!TestNotNull(TEXT("descent"), D)) { return false; }
	const FTransform Pad(FRotator(0.f, 30.f, 0.f), FVector(1000.f, -2000.f, 50.f));

	// Four sides of the globe, each going in along a different surface heading: four arrival points and headings.
	struct FCase { float Lat, Lon; FVector Vel; };
	const FCase Cases[] = { { 20.f, 0.f, FVector(0.f, 1.f, -1.f) }, { 20.f, 90.f, FVector(1.f, 0.f, -1.f) }, { 20.f, 180.f, FVector(0.f, -1.f, -1.f) }, { 20.f, -90.f, FVector(-1.f, 0.f, -1.f) } };
	TArray<FTransform> Got;
	for (const FCase& C : Cases)
	{
		FTDTestWorld W;
		FTDGlobePoint P; P.LatDeg = C.Lat; P.LonDeg = C.Lon;
		D->RecordEntryGlobe(ETDDestination::Verdance, P, C.Vel);
		FTransform Xf;
		if (!TestTrue(TEXT("arrival from the record"), D->GetArrival(W.GetWorld(), ETDDestination::Verdance, Pad, Xf))) { return false; }
		const FTransform Expect = FTDPlanetEntry::ArrivalFromGlobe(ETDDestination::Verdance, P, C.Vel, UTDDescentSubsystem::ArrivalAltitudeCm) * Pad;
		TestTrue(*FString::Printf(TEXT("lon %.0f: position = T4's mapping through the pad"), C.Lon), Xf.GetLocation().Equals(Expect.GetLocation(), 1.f));
		TestEqual(*FString::Printf(TEXT("lon %.0f: heading = T4's mapping"), C.Lon), (float)Xf.Rotator().Yaw, (float)Expect.Rotator().Yaw, 0.1f);
		TestTrue(*FString::Printf(TEXT("lon %.0f: inside the 7 km arrival ring"), C.Lon), FVector2D(Pad.InverseTransformPosition(Xf.GetLocation())).Size() <= FTDPlanetEntry::MaxArrivalRadius + 1.f);
		// A retake in the same visit: the same arrival; a later visit with no new entry: the fixed start.
		FTransform Again;
		TestTrue(TEXT("retake: same world, same arrival"), D->GetArrival(W.GetWorld(), ETDDestination::Verdance, Pad, Again) && Again.Equals(Xf));
		{
			FTDTestWorld Later;
			FTransform None;
			TestFalse(TEXT("a later visit (Continue) with no new entry: fixed start"), D->GetArrival(Later.GetWorld(), ETDDestination::Verdance, Pad, None));
		}
		Got.Add(Xf);
	}
	for (int32 i = 0; i < Got.Num(); ++i)
	for (int32 j = i + 1; j < Got.Num(); ++j)
	{
		TestTrue(*FString::Printf(TEXT("arrivals %d and %d differ in place"), i, j), FVector::Dist2D(Got[i].GetLocation(), Got[j].GetLocation()) > 50000.f);
	}
	float MaxYawSpread = 0.f;
	for (int32 i = 0; i < Got.Num(); ++i)
	for (int32 j = i + 1; j < Got.Num(); ++j)
	{
		MaxYawSpread = FMath::Max(MaxYawSpread, (float)FMath::Abs(FRotator::NormalizeAxis(Got[i].Rotator().Yaw - Got[j].Rotator().Yaw)));
	}
	AddInfo(FString::Printf(TEXT("arrival yaws %.0f / %.0f / %.0f / %.0f"), Got[0].Rotator().Yaw, Got[1].Rotator().Yaw, Got[2].Rotator().Yaw, Got[3].Rotator().Yaw));
	TestTrue(TEXT("the headings follow the entries (not one fixed heading)"), MaxYawSpread > 20.f);
	// The wrong planet never uses the record.
	{
		FTDTestWorld W;
		D->RecordEntryGlobe(ETDDestination::Cinder, FTDGlobePoint(), FVector::DownVector);
		FTransform Xf;
		TestFalse(TEXT("a Cinder record is not a Verdance arrival"), D->GetArrival(W.GetWorld(), ETDDestination::Verdance, Pad, Xf));
		D->ClearArrival();
	}
	// The rig uses it: its descent start is the arrival; without a record, today's fixed start.
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDDescentSubsystem* WD = UTDDescentSubsystem::Get(W.GetWorld());
		ATDPlanetTravelRig* Rig = W.Spawn<ATDPlanetTravelRig>(FVector::ZeroVector);
		if (!TestNotNull(TEXT("rig"), Rig) || !TestNotNull(TEXT("world descent"), WD)) { return false; }
		const FTransform Fixed = Rig->GetDescentStart();
		TestTrue(TEXT("no record: the fixed start (600 m behind, 300 m up)"), Fixed.GetLocation().Equals(Rig->GetActorTransform().TransformPosition(Rig->DescentStartOffset), 1.f));
		FTDGlobePoint P; P.LatDeg = 10.f; P.LonDeg = 45.f;
		WD->RecordEntryGlobe(ETDDestination::Verdance, P, FVector(1.f, 1.f, -1.f));
		const FTransform Arr = Rig->GetDescentStart();
		TestFalse(TEXT("with a record: not the fixed start"), Arr.GetLocation().Equals(Fixed.GetLocation(), 100.f));
		TestTrue(TEXT("... and stable for retakes"), Rig->GetDescentStart().Equals(Arr));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1DescentCoverAbort, "TriadDrift.Travel.Descent.CoverRampsWithTheEntryAndDropsOnAbort", TD_TEST_FLAGS)
bool FTDT1DescentCoverAbort::RunTest(const FString& Parameters)
{
	using namespace TDDescentTest;
	FGI G;
	UTDDescentSubsystem* D = G.D();
	if (!TestNotNull(TEXT("descent"), D)) { return false; }
	D->SetEntryCover(0.4f);
	TestEqual(TEXT("the entry ramps the cover"), D->GetCoverAlpha(), 0.4f, 0.001f);
	D->SetEntryCover(0.2f);
	TestEqual(TEXT("never down while entering"), D->GetCoverAlpha(), 0.4f, 0.001f);
	// Retake or death mid-entry: the entry hands back, the cover and roar go.
	D->AbortCover(TEXT("test: entry handed back"));
	TestEqual(TEXT("aborting"), D->GetCoverPhase(), ETDDescentCover::Aborting);
	for (int32 i = 0; i < 30; ++i) { D->Tick(1.f / 60.f); }
	TestEqual(TEXT("gone within 0.5 s"), D->GetCoverPhase(), ETDDescentCover::Off);
	TestEqual(TEXT("alpha 0"), D->GetCoverAlpha(), 0.f, 0.001f);
	// A travel that starts at full white (the entry's end) needs no further ramp.
	D->SetEntryCover(1.f);
	TestEqual(TEXT("full white: open the map at once"), D->BeginCoveredTravel(ETDDestination::Verdance), 0.f, 0.001f);
	TestEqual(TEXT("held"), D->GetCoverPhase(), ETDDescentCover::Held);
	D->AbortCover(TEXT("test cleanup"));
	return true;
}

#endif
