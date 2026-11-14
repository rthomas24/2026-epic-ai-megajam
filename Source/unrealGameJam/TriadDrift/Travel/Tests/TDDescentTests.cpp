// ENG-38 stage 1 (T1): the white-cover route and arrival from the entry point.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Travel.Descent

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Squad/TDSquadIntegration.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Travel/TDPlanetTravelRig.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/World/TDPlanetEntry.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDFlightFeedbackComponent.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "Camera/CameraComponent.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
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
	const FCase Cases[] = { { 20.f, 0.f, FVector(0.f, 1.f, -1.f) }, { 20.f, 90.f, FVector(1.f, 0.f, -1.f) }, { 20.f, 180.f, FVector(0.f, -1.f, -1.f) }, { 20.f, -90.f, FVector(-1.f, 0.f, -1.f) }, { -60.f, 45.f, FVector(1.f, 1.f, -1.f) } };
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
		TestTrue(*FString::Printf(TEXT("lon %.0f: within 6 km of the pad (clear of the edge warning)"), C.Lon), FVector2D(Pad.InverseTransformPosition(Xf.GetLocation())).Size() <= FTDPlanetEntry::MaxArrivalRadius + 1.f);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1DescentHeat, "TriadDrift.Travel.Descent.HullHeatGlowOnSquadShips", TD_TEST_FLAGS)
bool FTDT1DescentHeat::RunTest(const FString& Parameters)
{
	UMaterialInterface* ShipMI = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TriadDrift/Gameplay/Ships/Lancer/MI_Lancer.MI_Lancer"));
	if (!TestNotNull(TEXT("MI_Lancer (M_TD_Ship child)"), ShipMI)) { return false; }
	float Def = -1.f;
	if (!TestTrue(TEXT("M_TD_Ship exposes HeatGlow"), ShipMI->GetScalarParameterValue(FHashedMaterialParameterInfo(UTDDescentSubsystem::HeatGlowParam), Def))) { return false; }

	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDDescentSubsystem* D = UTDDescentSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("descent"), D)) { return false; }
	auto MakeShip = [&](UMaterialInterface* Mat, float Y)
	{
		ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, Y, 20000.f));
		TInlineComponentArray<UMeshComponent*> Meshes(Ship);
		for (UMeshComponent* M : Meshes) { for (int32 i = 0; i < M->GetNumMaterials(); ++i) { M->SetMaterial(i, Mat); } }
		return Ship;
	};
	auto HeatOf = [](ATDShip* Ship)
	{
		TInlineComponentArray<UMeshComponent*> Meshes(Ship);
		for (UMeshComponent* M : Meshes) { for (int32 i = 0; i < M->GetNumMaterials(); ++i)
		{
			float V = 0.f;
			if (UMaterialInterface* Mat = M->GetMaterial(i); Mat && Mat->GetScalarParameterValue(FHashedMaterialParameterInfo(UTDDescentSubsystem::HeatGlowParam), V)) { return V; }
		} }
		return -1.f;
	};
	ATDShip* Player = MakeShip(ShipMI, 0.f);
	ATDShip* Wingman = MakeShip(ShipMI, 3000.f);
	ATDShip* Plain = MakeShip(UMaterial::GetDefaultMaterial(MD_Surface), 6000.f);
	if (!TestNotNull(TEXT("ships"), Player) || !Wingman || !Plain) { return false; }
	TestEqual(TEXT("cold: no dynamic instance made"), ATDShip::StaticClass() ? D->ApplyHeatToShip(Player, 0.f) : 0, 0);

	// The dive feeds it (never down while entering); every squad ship gets it, a material without HeatGlow is untouched.
	D->SetEntryHeat(0.6f);
	D->SetEntryHeat(0.3f);
	D->Tick(1.f / 60.f);
	TestEqual(TEXT("player ship glows with the dive"), HeatOf(Player), 0.6f, 0.02f);
	TestEqual(TEXT("the wingman too"), HeatOf(Wingman), 0.6f, 0.02f);
	TestEqual(TEXT("no HeatGlow parameter: left alone (no instance)"), HeatOf(Plain), -1.f);
	TestFalse(TEXT("... still its own material"), Plain->FindComponentByClass<UMeshComponent>() && Cast<UMaterialInstanceDynamic>(Plain->FindComponentByClass<UMeshComponent>()->GetMaterial(0)) != nullptr);
	// A ship that appears mid-entry (a switch, the planet squad) picks up the current value.
	ATDShip* Late = MakeShip(ShipMI, 9000.f);
	D->SetEntryHeat(1.f);
	D->Tick(1.f / 60.f);
	TestEqual(TEXT("a new squad ship gets the same heat"), HeatOf(Late), 1.f, 0.02f);
	// Not fed and no cover holding it: 2 s decay.
	for (int32 i = 0; i < 60; ++i) { D->Tick(1.f / 60.f); }
	TestEqual(TEXT("1 s after the last feed: 0.25 s held, then 0.75 s of the 2 s decay"), HeatOf(Player), 0.625f, 0.03f);
	for (int32 i = 0; i < 90; ++i) { D->Tick(1.f / 60.f); }
	TestEqual(TEXT("gone after 2 s"), HeatOf(Player), 0.f, 0.001f);
	// An abort clears it at once.
	D->SetEntryHeat(0.8f);
	D->Tick(1.f / 60.f);
	D->ClearHeat();
	TestEqual(TEXT("abort: cold now"), HeatOf(Wingman), 0.f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1DescentSquadCarry, "TriadDrift.Travel.Descent.FlownShipAndHullsCarriedThroughTravel", TD_TEST_FLAGS)
bool FTDT1DescentSquadCarry::RunTest(const FString& Parameters)
{
	// Bastion flies the squad into the planet with Rift at 50 % (shield 30 %), Lancer downed, Bastion at 80 %.
	FTDSquadCarry Carry;
	Carry.Active = ETDShipId::Bastion;
	Carry.Hull01 = { { ETDShipId::Rift, 0.5f }, { ETDShipId::Lancer, 0.f }, { ETDShipId::Bastion, 0.8f } };
	Carry.Shield01 = { { ETDShipId::Rift, 0.3f }, { ETDShipId::Lancer, 0.f } }; // Bastion's shield missing: arrives full
	auto Hull = [](ATDSquadManager* Sq, ETDShipId Id) { ATDShip* S = Sq->GetShip(Id); return S && S->GetHealth() ? S->GetHealth()->GetHull01() : -1.f; };
	auto Shield = [](ATDSquadManager* Sq, ETDShipId Id) { ATDShip* S = Sq->GetShip(Id); return S && S->GetHealth() ? S->GetHealth()->GetShield01() : -1.f; };

	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDDescentSubsystem* D = UTDDescentSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("descent"), D)) { return false; }
	D->RecordSquadForTests(Carry);
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	APlayerController* PC = W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass());
	if (!TestNotNull(TEXT("squad"), Squad) || !TestNotNull(TEXT("pc"), PC)) { return false; }
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform(FVector(0.f, 0.f, 20000.f)));
	const float RiftShieldAtSpawn = Shield(Squad, ETDShipId::Rift); // before any regeneration tick
	W.Advance(0.2f);
	TestEqual(TEXT("arrives flying the ship that went in"), Squad->GetActiveShipId(), ETDShipId::Bastion);
	TestTrue(TEXT("the player controls it"), PC->GetPawn() == Squad->GetShip(ETDShipId::Bastion));
	const FTDSquadSlot* Active = Squad->GetSlots().FindByPredicate([](const FTDSquadSlot& S) { return S.ShipId == ETDShipId::Bastion; });
	TestTrue(TEXT("the active slot is alive (T3)"), Active && Active->State == ETDSquadSlotState::Alive);
	const UTDDirectorSubsystem* Dir = W.GetWorld()->GetSubsystem<UTDDirectorSubsystem>();
	TestFalse(TEXT("no director shot or switch cut on arrival (T3)"), Dir && Dir->IsShotActive());
	TestEqual(TEXT("Rift hull carried"), Hull(Squad, ETDShipId::Rift), 0.5f, 0.01f);
	TestEqual(TEXT("Rift shield carried"), RiftShieldAtSpawn, 0.3f, 0.01f);
	TestEqual(TEXT("downed Lancer arrives alive at 25 %"), Hull(Squad, ETDShipId::Lancer), 0.25f, 0.01f);
	TestEqual(TEXT("Bastion hull carried"), Hull(Squad, ETDShipId::Bastion), 0.8f, 0.01f);
	TestEqual(TEXT("a shield missing from the record arrives full"), Shield(Squad, ETDShipId::Bastion), 1.f, 0.01f);

	// A retake in the same visit: still Bastion, hulls full as before.
	Squad->SpawnSquad(FTransform(FVector(0.f, 0.f, 20000.f)));
	W.Advance(0.2f);
	TestEqual(TEXT("retake: same ship"), Squad->GetActiveShipId(), ETDShipId::Bastion);
	TestEqual(TEXT("retake: full hull"), Hull(Squad, ETDShipId::Rift), 1.f, 0.01f);

	// The next map with no new record: as before (and the record is gone).
	{
		FTDTestWorld Next;
		FTDSquadCarry Out; bool bFirst = false;
		TestFalse(TEXT("a later map without a new record gets nothing"), D->ConsumeSquadCarry(Next.GetWorld(), Out, bFirst));
	}

	// Recording from a live squad writes the progress's LastShip (saves and Continue carry the flown ship; T5).
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	Squad->RequestSwitchTo(ETDShipId::Lancer);
	W.Advance(0.5f);
	D->RecordSquad(W.GetWorld());
	TestEqual(TEXT("recorded: the flown ship"), GI ? GI->GetProgressRef().LastShip : ETDShipId::Rift, Squad->GetActiveShipId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1DescentBurnFeel, "TriadDrift.Travel.Descent.BurnFeelAndHUDHoldOnTheCurve", TD_TEST_FLAGS)
bool FTDT1DescentBurnFeel::RunTest(const FString& Parameters)
{
	// The burn: FOV push, shake and motion blur on the descent curve (scaled by the motion sliders), the HUD held.
	using FB = UTDFlightFeedbackComponent;
	const float OldShake = FB::UserShakeScale, OldSpeed = FB::UserSpeedFOVScale;
	ON_SCOPE_EXIT { FB::UserShakeScale = OldShake; FB::UserSpeedFOVScale = OldSpeed; FB::DescentIntensity = 0.f; };
	struct FOut { float FOV = 0.f, Blur = 0.f, Shake = 0.f; bool bHeld = false; };
	auto Run = [](float Intensity, float Motion)
	{
		FOut O;
		FTDTestWorld W(UTDGameInstance::StaticClass());
		ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
		if (!Ship) { return O; }
		Ship->SetPlayerControlled(true);
		FB::UserShakeScale = Motion; FB::UserSpeedFOVScale = Motion; // after the world starts (the settings apply then)
		UTDDescentSubsystem* D = UTDDescentSubsystem::Get(W.GetWorld());
		FB::ComposedPeaks = FB::FComposedPeaks();
		for (int32 i = 0; i < 90; ++i)
		{
			if (D && Intensity > 0.f) { D->SetEntryHeat(Intensity); }
			if (D) { D->Tick(1.f / 60.f); } // the engine ticks game-instance tickables in the game; the test world does not
			W.Tick();
			O.FOV = FMath::Max(O.FOV, Ship->GetChaseCamera()->FieldOfView);
			O.Blur = FMath::Max(O.Blur, Ship->GetChaseCamera()->PostProcessSettings.MotionBlurAmount);
			O.bHeld = O.bHeld || UTDUIHostSubsystem::IsCombatPresentationHeld(W.GetWorld());
		}
		O.Shake = FB::ComposedPeaks.Shake;
		if (D) { D->ClearHeat(); }
		return O;
	};
	const FOut Calm = Run(0.f, 1.f), Burn = Run(1.f, 1.f), Reduced = Run(1.f, 0.f);
	AddInfo(FString::Printf(TEXT("FOV %.1f -> %.1f (reduced %.1f); blur %.3f -> %.3f (reduced %.3f); shake %.2f -> %.2f (reduced %.2f)"),
		Calm.FOV, Burn.FOV, Reduced.FOV, Calm.Blur, Burn.Blur, Reduced.Blur, Calm.Shake, Burn.Shake, Reduced.Shake));
	TestTrue(TEXT("the burn pushes the FOV"), Burn.FOV > Calm.FOV + 3.f);
	TestTrue(TEXT("the burn blurs"), Burn.Blur > 0.2f);
	TestTrue(TEXT("the burn shakes"), Burn.Shake > Calm.Shake + 0.3f);
	TestTrue(TEXT("reduced motion: no FOV push, blur or shake"), Reduced.FOV <= Calm.FOV + 0.1f && Reduced.Blur < 0.001f && Reduced.Shake < 0.001f);
	TestTrue(TEXT("the HUD is held (dimmed, combat away) through the burn"), Burn.bHeld);
	TestFalse(TEXT("... and not without it"), Calm.bHeld);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1DescentBurnShot, "TriadDrift.Travel.Descent.DirectorShotFramesTheShipFromAheadAndTheSide", TD_TEST_FLAGS)
bool FTDT1DescentBurnShot::RunTest(const FString& Parameters)
{
	// The Lead's shot: ahead of, beside and below the ship (the climb out: the other side, above), looking back at it, with
	// a slow push-in (none with reduced motion); td.Descent.DirectorShot 0 keeps the chase camera.
	FTDTestWorld W(UTDGameInstance::StaticClass());
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	TDSquad::ApplySpec(Ship, UTDShipLibrary::Get(ETDShipId::Bastion)); // the real hull: the shot scales to it
	for (const bool bExit : { false, true })
	{
		const FString Tag = bExit ? TEXT("climb out: ") : TEXT("entry: ");
		const FTransform A0 = UTDDescentSubsystem::BurnShotCamera(Ship, bExit, 0.f, false);
		const FTransform A1 = UTDDescentSubsystem::BurnShotCamera(Ship, bExit, 1.f, false);
		const FTransform R1 = UTDDescentSubsystem::BurnShotCamera(Ship, bExit, 1.f, true);
		const FVector L = A0.GetLocation();
		TestTrue(*(Tag + TEXT("ahead of the ship")), L.X > 1000.f);
		TestTrue(*(Tag + TEXT("to the side")), FMath::Abs(L.Y) > 700.f && (bExit ? L.Y < 0.f : L.Y > 0.f));
		TestTrue(*(Tag + (bExit ? TEXT("above") : TEXT("below"))), bExit ? L.Z > 0.f : L.Z < 0.f);
		const FVector Hull = UTDLandingComponent::VisibleHullBoxLocal(Ship).GetCenter();
		TestTrue(*(Tag + TEXT("looks back at the ship")), FVector::DotProduct(A0.GetRotation().GetForwardVector(), (Hull - L).GetSafeNormal()) > 0.99f);
		TestEqual(*(Tag + TEXT("slow push-in: 15 % closer at the end")), (float)(A1.GetLocation().Size() / L.Size()), 0.85f, 0.01f);
		TestEqual(*(Tag + TEXT("reduced motion: no push-in")), (float)(R1.GetLocation().Size() / L.Size()), 1.f, 0.01f);
	}
	IConsoleVariable* Shot = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Descent.DirectorShot"));
	if (!TestNotNull(TEXT("td.Descent.DirectorShot"), Shot)) { return false; }
	ON_SCOPE_EXIT { Shot->Set(1, ECVF_SetByCode); };
	Shot->Set(0, ECVF_SetByCode);
	UTDDescentSubsystem* D = UTDDescentSubsystem::Get(W.GetWorld());
	TestFalse(TEXT("td.Descent.DirectorShot 0: no shot (the chase camera)"), D && D->BeginBurnShot(Ship, false, 3.f));
	return true;
}

#endif
