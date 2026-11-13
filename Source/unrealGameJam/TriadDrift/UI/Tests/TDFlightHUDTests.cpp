// ENG-36 engineering space HUD, P1 (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.UI.FlightHUD
// One test per binding (value in -> FView out), spec docs/eng/hud-redesign.md section 7.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/UI/TDFlightHUDWidget.h"
#include "TriadDrift/UI/TDUIHostSettings.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "TriadDrift/UI/Tests/TDFlightHUDTestSource.h"

namespace TDFlightHUDTests
{
	/** Sets a console variable for a test's scope and restores it. */
	struct FTDFlightScopedCVar
	{
		IConsoleVariable* Var = nullptr;
		int32 Previous = -1;
		FTDFlightScopedCVar(const TCHAR* Name, int32 Value)
		{
			Var = IConsoleManager::Get().FindConsoleVariable(Name);
			if (Var)
			{
				Previous = Var->GetInt();
				Var->Set(Value, ECVF_SetByCode);
			}
		}
		void Set(int32 Value) const
		{
			if (Var)
			{
				Var->Set(Value, ECVF_SetByCode);
			}
		}
		~FTDFlightScopedCVar()
		{
			if (Var)
			{
				Var->Set(Previous, ECVF_SetByCode);
			}
		}
	};

	/** A widget reading a test source, keyboard glyphs unless told otherwise. */
	struct FTDFlightHUDFixture
	{
		FTDTestWorld W;
		ATDFlightHUDTestSource* Src = nullptr;
		UTDFlightHUDWidget* HUD = nullptr;
		FTDFlightHUDFixture()
		{
			Src = W.Spawn<ATDFlightHUDTestSource>();
			Src->SetSquad(ETDShipId::Lancer);
			HUD = CreateWidget<UTDFlightHUDWidget>(W.GetWorld(), UTDFlightHUDWidget::StaticClass());
			HUD->ForceGamepad = 0;
			HUD->SetDataSource(Src);
		}
		const UTDFlightHUDWidget::FView& V() const { return HUD->GetView(); }
		const UTDFlightHUDWidget::FChip* Chip(ETDShipId Ship) const
		{
			return V().Chips.FindByPredicate([Ship](const UTDFlightHUDWidget::FChip& C) { return C.Ship == Ship; });
		}
	};
}

using namespace TDFlightHUDTests;

// The HUD choice: engineering / Nick / dev, with both console overrides.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDChoice, "TriadDrift.UI.FlightHUD.ChoiceSetting", TD_TEST_FLAGS)
bool FTDFlightHUDChoice::RunTest(const FString& Parameters)
{
	FTDFlightScopedCVar Space(TEXT("TD.UI.SpaceHUD"), -1);
	FTDFlightScopedCVar Native(TEXT("TD.UI.NativeHUD"), -1);
	UTDUIHostSettings* Settings = GetMutableDefault<UTDUIHostSettings>();
	const ETDSpaceHUDChoice SavedChoice = Settings->SpaceHUD;
	const bool bSavedNative = Settings->bUseNativeSpaceHUD;

	Settings->SpaceHUD = ETDSpaceHUDChoice::Unset;
	Settings->bUseNativeSpaceHUD = true;
	TestEqual(TEXT("unset + legacy on = Nick"), UTDUIHostSettings::GetSpaceHUDChoice(), ETDSpaceHUDChoice::Nick);
	Settings->bUseNativeSpaceHUD = false;
	TestEqual(TEXT("unset + legacy off = dev"), UTDUIHostSettings::GetSpaceHUDChoice(), ETDSpaceHUDChoice::Dev);
	Settings->SpaceHUD = ETDSpaceHUDChoice::Engineering;
	TestEqual(TEXT("setting = engineering"), UTDUIHostSettings::GetSpaceHUDChoice(), ETDSpaceHUDChoice::Engineering);
	TestFalse(TEXT("engineering is not Nick's"), UTDUIHostSettings::IsNativeSpaceHUDEnabled());
	Native.Set(1);
	TestEqual(TEXT("TD.UI.NativeHUD 1 still shows Nick's"), UTDUIHostSettings::GetSpaceHUDChoice(), ETDSpaceHUDChoice::Nick);
	Native.Set(0);
	TestEqual(TEXT("TD.UI.NativeHUD 0 leaves engineering alone"), UTDUIHostSettings::GetSpaceHUDChoice(), ETDSpaceHUDChoice::Engineering);
	Settings->SpaceHUD = ETDSpaceHUDChoice::Nick;
	TestEqual(TEXT("TD.UI.NativeHUD 0 turns Nick's off (dev)"), UTDUIHostSettings::GetSpaceHUDChoice(), ETDSpaceHUDChoice::Dev);
	for (const TPair<int32, ETDSpaceHUDChoice> Case : TArray<TPair<int32, ETDSpaceHUDChoice>>{
			 { 0, ETDSpaceHUDChoice::Dev }, { 1, ETDSpaceHUDChoice::Nick }, { 2, ETDSpaceHUDChoice::Engineering } })
	{
		Space.Set(Case.Key);
		TestEqual(FString::Printf(TEXT("TD.UI.SpaceHUD %d wins"), Case.Key), UTDUIHostSettings::GetSpaceHUDChoice(), Case.Value);
	}
	Settings->SpaceHUD = SavedChoice;
	Settings->bUseNativeSpaceHUD = bSavedNative;
	return true;
}

// The host shows the engineering widget when chosen, switches live to Nick's and to none (dev), and feeds the squad.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDHost, "TriadDrift.UI.FlightHUD.HostShowsTheChosenHUD", TD_TEST_FLAGS)
bool FTDFlightHUDHost::RunTest(const FString& Parameters)
{
	FTDFlightScopedCVar Space(TEXT("TD.UI.SpaceHUD"), 2);
	FTDTestWorld W;
	APlayerController* PC = W.SpawnLocalPlayerController();
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform::Identity);
	UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("host"), Host))
	{
		return false;
	}
	Host->bAddToViewport = false;
	W.Tick();
	Host->Evaluate();
	UTDFlightHUDWidget* HUD = Cast<UTDFlightHUDWidget>(Host->GetSpaceHUD());
	if (!TestNotNull(TEXT("engineering HUD shown"), HUD))
	{
		return false;
	}
	TestTrue(TEXT("fed by the squad manager"), HUD->GetDataSource() == Squad);
	HUD->RefreshData();
	TestEqual(TEXT("three chips"), HUD->GetView().Chips.Num(), 3);
	TestEqual(TEXT("one active chip"), HUD->GetView().Chips.FilterByPredicate([](const UTDFlightHUDWidget::FChip& C) { return C.bActive; }).Num(), 1);
	TestFalse(TEXT("its objective is read"), HUD->GetView().Objective.IsEmpty() && !Squad->GetObjectiveText().IsEmpty());

	Space.Set(0);
	Host->Evaluate();
	TestNull(TEXT("dev: no widget"), Host->GetSpaceHUD());
	Space.Set(1);
	Host->Evaluate();
	if (UTDUIHostSubsystem::FindNativeWidgetClass())
	{
		TestTrue(TEXT("Nick's: his widget, not ours"), Host->GetSpaceHUD() && !Host->GetSpaceHUD()->IsA<UTDFlightHUDWidget>());
	}
	Space.Set(2);
	Host->Evaluate();
	TestTrue(TEXT("back to engineering"), Host->GetSpaceHUD() && Host->GetSpaceHUD()->IsA<UTDFlightHUDWidget>());
	return true;
}

// Reticle cluster: the flown ship's hull, shield and colour; hull bands; arc rest opacity.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDCluster, "TriadDrift.UI.FlightHUD.ReticleCluster", TD_TEST_FLAGS)
bool FTDFlightHUDCluster::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.HUD->Sample(0.f);
	TestEqual(TEXT("flown = Lancer"), F.V().Flown, ETDShipId::Lancer);
	TestEqual(TEXT("full hull"), F.V().Hull, 1.f);
	F.HUD->Sample(UTDFlightHUDWidget::ArcAwakeSeconds + .1f);
	TestEqual(TEXT("full and unchanged: arcs rest at 55%"), F.V().ArcOpacity, UTDFlightHUDWidget::ArcRestOpacity);

	F.Src->Card(ETDShipId::Lancer).Shield01 = .6f;
	F.HUD->Sample(.016f);
	TestEqual(TEXT("shield hit: arcs wake"), F.V().ArcOpacity, 1.f);
	TestEqual(TEXT("shield value"), F.V().Shield, .6f);
	F.Src->Card(ETDShipId::Lancer).Shield01 = 1.f;
	F.HUD->Sample(.016f);
	F.HUD->Sample(1.f);
	TestEqual(TEXT("refilled but changed under 3 s ago: still awake"), F.V().ArcOpacity, 1.f);
	F.HUD->Sample(2.2f);
	TestEqual(TEXT("3 s later: rest"), F.V().ArcOpacity, UTDFlightHUDWidget::ArcRestOpacity);

	TestTrue(TEXT("hull 60%: white"), FTDFlightHUDTokens::Hull(.6f).Equals(FTDFlightHUDTokens::Neutral()));
	TestTrue(TEXT("hull 40%: amber"), FTDFlightHUDTokens::Hull(.4f).Equals(FTDFlightHUDTokens::Bastion()));
	TestTrue(TEXT("hull 20%: red"), FTDFlightHUDTokens::Hull(.2f).Equals(FTDFlightHUDTokens::Danger()));
	F.Src->Card(ETDShipId::Lancer).Health01 = .3f;
	F.HUD->Sample(.016f);
	TestEqual(TEXT("hull below 100%: arcs stay at full"), F.V().ArcOpacity, 1.f);

	// Switching recolours the cluster in the same sample.
	F.Src->SetSquad(ETDShipId::Bastion);
	F.HUD->Sample(.016f);
	TestEqual(TEXT("switch: cluster follows in the same frame"), F.V().Flown, ETDShipId::Bastion);
	TestEqual(TEXT("switch: Bastion's own hull"), F.V().Hull, 1.f);
	return true;
}

// Signature power pip: ready / cooling, the ready pulse, the device glyph; no pulse on a switch.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDPip, "TriadDrift.UI.FlightHUD.PowerPip", TD_TEST_FLAGS)
bool FTDFlightHUDPip::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.HUD->Sample(0.f);
	TestTrue(TEXT("ready at start"), F.V().bPowerReady);
	TestEqual(TEXT("no pulse at start"), F.V().PowerPulse, 0.f);
	TestEqual(TEXT("keyboard glyph Q"), F.V().PowerGlyph, FString(TEXT("Q")));
	F.Src->Card(ETDShipId::Lancer).PowerCooldown01 = .4f;
	F.HUD->Sample(.016f);
	TestFalse(TEXT("cooling"), F.V().bPowerReady);
	TestEqual(TEXT("cooling fraction"), F.V().Power, .4f);
	F.Src->Card(ETDShipId::Lancer).PowerCooldown01 = 1.f;
	F.HUD->Sample(.016f);
	F.HUD->Sample(.1f);
	TestTrue(TEXT("ready again: pulsing"), F.V().PowerPulse > 0.f && F.V().PowerPulse < 1.f);
	F.HUD->Sample(UTDFlightHUDWidget::PulseSeconds);
	TestEqual(TEXT("pulse over after 300 ms"), F.V().PowerPulse, 0.f);

	// Switch from a cooling ship to a ready one: no pulse (the pulse means "came off cooldown").
	F.Src->Card(ETDShipId::Lancer).PowerCooldown01 = .2f;
	F.HUD->Sample(.016f);
	F.Src->SetSquad(ETDShipId::Rift);
	F.HUD->Sample(.016f);
	TestTrue(TEXT("switch onto a ready power: ready"), F.V().bPowerReady);
	TestEqual(TEXT("switch onto a ready power: no pulse"), F.V().PowerPulse, 0.f);

	F.HUD->ForceGamepad = 1;
	F.HUD->Sample(.016f);
	TestEqual(TEXT("pad glyph X"), F.V().PowerGlyph, FString(TEXT("X")));
	return true;
}

// Squad strip: fixed order, sizes, keys, the power-ready dot, wingman trouble, down + respawn, inside the safe frame.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDStrip, "TriadDrift.UI.FlightHUD.SquadStrip", TD_TEST_FLAGS)
bool FTDFlightHUDStrip::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.Src->Card(ETDShipId::Rift).Health01 = .3f;			// wingman in trouble
	F.Src->Card(ETDShipId::Rift).PowerCooldown01 = 1.f;		// its power is ready
	F.Src->Card(ETDShipId::Bastion).bAlive = false;			// down
	F.Src->Card(ETDShipId::Bastion).RespawnSeconds = 4.2f;
	F.Src->Card(ETDShipId::Lancer).Health01 = .2f;			// the flown ship low: no trouble badge on the active chip
	F.Src->Card(ETDShipId::Lancer).PowerCooldown01 = .5f;
	F.HUD->Sample(0.f);
	const auto& Chips = F.V().Chips;
	if (!TestEqual(TEXT("three chips"), Chips.Num(), 3))
	{
		return false;
	}
	TestTrue(TEXT("order 1 2 3 = Rift Lancer Bastion"), Chips[0].Ship == ETDShipId::Rift && Chips[1].Ship == ETDShipId::Lancer && Chips[2].Ship == ETDShipId::Bastion);
	TestEqual(TEXT("keys"), Chips[0].Key + Chips[1].Key + Chips[2].Key, FString(TEXT("123")));
	TestTrue(TEXT("Lancer active and wide"), Chips[1].bActive && Chips[1].Size.X == UTDFlightHUDWidget::ActiveChipWidth);
	TestEqual(TEXT("wingman chip width"), (float)Chips[0].Size.X, UTDFlightHUDWidget::WingChipWidth, .01f);
	TestTrue(TEXT("Rift: trouble"), Chips[0].bTrouble);
	TestTrue(TEXT("Rift: power-ready dot"), Chips[0].bPowerReady);
	TestFalse(TEXT("Lancer (flown): no trouble badge"), Chips[1].bTrouble);
	TestFalse(TEXT("Lancer: power cooling, no dot"), Chips[1].bPowerReady);
	TestFalse(TEXT("Bastion: down"), Chips[2].bAlive);
	TestFalse(TEXT("down is not trouble"), Chips[2].bTrouble);
	TestEqual(TEXT("Bastion: respawn seconds"), Chips[2].RespawnSeconds, 4.2f);
	TestFalse(TEXT("down: no ready dot"), Chips[2].bPowerReady);

	const float Left = Chips[0].Pos.X;
	const float Right = Chips[2].Pos.X + Chips[2].Size.X;
	const float Bottom = Chips[0].Pos.Y + Chips[0].Size.Y;
	AddInfo(FString::Printf(TEXT("strip %.0f x %.0f at (%.0f, %.0f)"), Right - Left, Chips[0].Size.Y, Left, Chips[0].Pos.Y));
	TestTrue(TEXT("strip about 470 x 58 (Lead: 1.3x)"), Right - Left <= 475.f && FMath::IsNearlyEqual(Chips[0].Size.Y, 57.2f, 1.f));
	TestTrue(TEXT("inside the safe frame"), Left >= UTDFlightHUDWidget::SafeX && Bottom <= 1080.f - UTDFlightHUDWidget::SafeY + .01f);

	// The switch changes the active chip in the same sample; the order never changes.
	F.Src->SetSquad(ETDShipId::Rift);
	F.HUD->Sample(.016f);
	TestTrue(TEXT("switch: Rift active now"), F.V().Chips[0].bActive && !F.V().Chips[1].bActive);
	TestEqual(TEXT("switch: order kept"), F.V().Chips[0].Ship, ETDShipId::Rift);

	F.HUD->ForceGamepad = 1;
	F.HUD->Sample(.016f);
	TestTrue(TEXT("pad: no number keycaps"), F.V().Chips[0].Key.IsEmpty());
	return true;
}

// Objective line: caps text, enemies-left counter, expands for 3 s when the text changes, wave detail line.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDObjective, "TriadDrift.UI.FlightHUD.ObjectiveLine", TD_TEST_FLAGS)
bool FTDFlightHUDObjective::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.HUD->Sample(0.f);
	TestTrue(TEXT("no objective: empty"), F.V().Objective.IsEmpty());
	TestEqual(TEXT("no objective: collapsed"), F.V().ObjectiveExpand, 0.f);

	F.Src->Objective = FText::FromString(TEXT("Clear the Null Armada"));
	F.Src->Wave.WaveNumber = 3;
	F.Src->Wave.WaveCount = 5;
	F.Src->Wave.bWaveActive = true;
	F.Src->Wave.EnemiesAlive = 5;
	F.HUD->Sample(.016f);
	TestEqual(TEXT("caps"), F.V().Objective, FString(TEXT("CLEAR THE NULL ARMADA")));
	TestEqual(TEXT("counter"), F.V().Counter, FString(TEXT("5 LEFT")));
	TestEqual(TEXT("detail"), F.V().ObjectiveDetail, FString(TEXT("WAVE 3 OF 5")));
	TestTrue(TEXT("expanding"), F.V().ObjectiveExpand >= 0.f && F.V().ObjectiveExpand < 1.f);
	F.HUD->Sample(.5f);
	TestEqual(TEXT("expanded"), F.V().ObjectiveExpand, 1.f);
	F.HUD->Sample(2.6f);
	TestTrue(TEXT("collapsing after 3 s"), F.V().ObjectiveExpand < 1.f);
	F.HUD->Sample(.4f);
	TestEqual(TEXT("collapsed"), F.V().ObjectiveExpand, 0.f);

	F.Src->Wave.EnemiesAlive = 2;
	F.HUD->Sample(.016f);
	TestEqual(TEXT("counter follows"), F.V().Counter, FString(TEXT("2 LEFT")));
	TestEqual(TEXT("a counter change does not expand"), F.V().ObjectiveExpand, 0.f);
	F.Src->Wave.bWaveActive = false;
	F.HUD->Sample(.016f);
	TestTrue(TEXT("between waves: no counter"), F.V().Counter.IsEmpty());
	F.Src->Objective = FText::FromString(TEXT("Jump to Verdance"));
	F.HUD->Sample(.016f);
	F.HUD->Sample(.4f);
	TestEqual(TEXT("new objective: expands again"), F.V().ObjectiveExpand, 1.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
