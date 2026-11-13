// ENG-36 engineering space HUD, P1 (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.UI.FlightHUD
// One test per binding (value in -> FView out), spec docs/eng/hud-redesign.md section 7.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Input/HittestGrid.h"
#include "Layout/WidgetPath.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SWindow.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Input/TDHintComponent.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Core/TDEventBus.h"
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
	// The squad manager's "WAVE 1 · CLEAR ..." form: the wave moves to the detail line, the line keeps the instruction.
	F.Src->Objective = FText::FromString(TEXT("WAVE 3 · Clear the Null Armada"));
	F.HUD->Sample(.016f);
	TestEqual(TEXT("wave prefix moved off the line"), F.V().Objective, FString(TEXT("CLEAR THE NULL ARMADA")));
	F.Src->Objective = FText::FromString(TEXT("Jump to Verdance"));
	F.HUD->Sample(.016f);
	F.HUD->Sample(.4f);
	TestEqual(TEXT("new objective: expands again"), F.V().ObjectiveExpand, 1.f);
	return true;
}

// P2: hostiles. A current target gets the bracket (48..160) and distance; others ticks; off screen = chevron on the
// safe frame along the marker's direction; missiles flagged; locked and current carried through.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDMarks, "TriadDrift.UI.FlightHUD.P2.TargetsAndEdgeArrows", TD_TEST_FLAGS)
bool FTDFlightHUDMarks::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	FTDTargetMarker& Cur = F.Src->AddMarker(true, FVector2D(.6f, .4f), FVector2D(.3f, -.2f), 41200.f);
	Cur.bIsCurrentTarget = true;
	Cur.bLocked = true;
	F.Src->AddMarker(true, FVector2D(.3f, .3f), FVector2D(-.5f, -.5f), 90000.f);
	F.Src->AddMarker(false, FVector2D::ZeroVector, FVector2D(1.f, 0.f), 120000.f);					// off right
	F.Src->AddMarker(false, FVector2D::ZeroVector, FVector2D(0.f, -1.f), 50000.f).bIncomingMissile = true;	// missile above
	F.HUD->Sample(.016f);
	const auto& Marks = F.V().Marks;
	if (!TestEqual(TEXT("four marks"), Marks.Num(), 4))
	{
		return false;
	}
	TestTrue(TEXT("current: on screen at 60% / 40% (design space)"), Marks[0].bOnScreen && Marks[0].Pos.Equals(FVector2D(1152, 432), 1.f));
	TestTrue(TEXT("current: locked"), Marks[0].bCurrent && Marks[0].bLocked);
	TestTrue(TEXT("current: bracket within 48..160"), Marks[0].Size >= UTDFlightHUDWidget::BracketMin && Marks[0].Size <= UTDFlightHUDWidget::BracketMax);
	TestEqual(TEXT("current: distance in metres"), Marks[0].Distance, 412.f, .5f);
	TestTrue(TEXT("other hostile: a tick, not the bracket"), Marks[1].bOnScreen && !Marks[1].bCurrent);
	TestFalse(TEXT("off right: edge chevron"), Marks[2].bOnScreen);
	TestEqual(TEXT("off right: on the safe frame's right edge"), (float)Marks[2].Pos.X, 1920.f - UTDFlightHUDWidget::SafeX - 12.f, 1.f); // inset so the whole marker is inside
	TestEqual(TEXT("off right: pointing right"), Marks[2].Angle, 0.f, .01f);
	TestTrue(TEXT("missile above: flagged, on the top edge"), Marks[3].bMissile && FMath::IsNearlyEqual((float)Marks[3].Pos.Y, UTDFlightHUDWidget::SafeY + 12.f, 1.f));
	return true;
}

// P2: hit arcs (direction, strength, shield-only, fade), the low-hull vignette, missile warning states.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDHits, "TriadDrift.UI.FlightHUD.P2.HitsVignetteMissile", TD_TEST_FLAGS)
bool FTDFlightHUDHits::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	FTDDamageIndicator Right;
	Right.WorldDirection = FVector(0, 1, 0);	// from the right (no controller: the view looks down +X)
	Right.Strength01 = 1.f;
	FTDDamageIndicator Behind;
	Behind.WorldDirection = FVector(-1, 0, 0);
	Behind.Strength01 = 0.f;
	Behind.bShieldOnly = true;
	Behind.AgeSeconds = .6f;
	FTDDamageIndicator Old;
	Old.AgeSeconds = 2.f;
	F.Src->Hits = { Right, Behind, Old };
	F.HUD->Sample(.016f);
	const auto& Hits = F.V().Hits;
	if (!TestEqual(TEXT("expired hit dropped"), Hits.Num(), 2))
	{
		return false;
	}
	TestEqual(TEXT("from the right: arc on the right"), Hits[0].Angle, 0.f, .01f);
	TestEqual(TEXT("full strength, fresh: opaque"), Hits[0].Opacity, 1.f, .01f);
	TestEqual(TEXT("from behind: arc at the bottom"), Hits[1].Angle, PI * .5f, .01f);
	TestTrue(TEXT("shield-only"), Hits[1].bShieldOnly);
	TestEqual(TEXT("weak, half faded"), Hits[1].Opacity, .4f * .5f, .01f);

	TestEqual(TEXT("healthy: no vignette"), F.V().Vignette, 0.f);
	F.Src->Card(ETDShipId::Lancer).Health01 = .2f;
	F.HUD->Sample(.016f);
	TestEqual(TEXT("hull under 25%: vignette"), F.V().Vignette, 1.f);

	F.Src->Missile = ETDMissileWarning::Imminent;
	F.HUD->Sample(.016f);
	TestEqual(TEXT("missile warning carried"), F.V().Missile, ETDMissileWarning::Imminent);
	return true;
}

// P2: ability pip, Bastion's second power, ammo, heat, Switch Strike, boost hide timer, dodge notch, hyperdrive bar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDContext, "TriadDrift.UI.FlightHUD.P2.ContextualReadouts", TD_TEST_FLAGS)
bool FTDFlightHUDContext::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.HUD->Sample(.016f);
	TestFalse(TEXT("no ability unlocked: no pip (never a padlock in flight)"), F.V().bHasAbility);
	TestFalse(TEXT("Lancer: no second power"), F.V().bHasPower2);
	TestFalse(TEXT("boost full at start: hidden"), F.V().bShowBoost);

	FTDShipHUD& L = F.Src->Card(ETDShipId::Lancer);
	L.Ability = ETDAbilityId::EMPPulse;
	L.AbilityCooldown01 = .5f;
	L.SecondaryAmmo = 6;
	L.Heat01 = .95f;
	L.Boost01 = .4f;
	L.Dodge01 = .2f;
	F.Src->SwitchStrike = .7f;
	F.Src->Hyperdrive = .3f;
	F.HUD->Sample(.016f);
	TestTrue(TEXT("ability pip, cooling"), F.V().bHasAbility && F.V().Ability == .5f);
	TestEqual(TEXT("ability glyph E"), F.V().AbilityGlyph, FString(TEXT("E")));
	TestEqual(TEXT("ammo"), F.V().Ammo, 6);
	TestEqual(TEXT("heat"), F.V().Heat, .95f);
	TestTrue(TEXT("boosting: bar shown"), F.V().bShowBoost && F.V().Boost == .4f);
	TestFalse(TEXT("dodge cooling: notch dark"), F.V().bDodgeReady);
	TestEqual(TEXT("switch strike ring"), F.V().SwitchStrike, .7f);
	TestEqual(TEXT("hyperdrive charging"), F.V().Hyperdrive, .3f);

	L.Boost01 = 1.f;
	F.HUD->Sample(.016f);
	TestTrue(TEXT("just refilled: still shown"), F.V().bShowBoost);
	F.HUD->Sample(UTDFlightHUDWidget::BoostHideSeconds + .1f);
	TestFalse(TEXT("1.5 s after full: hidden"), F.V().bShowBoost);

	F.Src->SetSquad(ETDShipId::Bastion);
	F.Src->Card(ETDShipId::Bastion).Power2Cooldown01 = .25f;
	F.HUD->Sample(.016f);
	TestTrue(TEXT("Bastion: the dome pip"), F.V().bHasPower2 && F.V().Power2 == .25f);
	return true;
}

// P2: the lower lane shows one message: prompt > blocked jump (3 s after a press only) > hint; plus boundary and altitude.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDLane, "TriadDrift.UI.FlightHUD.P2.LowerLaneAndWarnings", TD_TEST_FLAGS)
bool FTDFlightHUDLane::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.Src->Blocked = FText::FromString(TEXT("Hyperdrive locked - enemies in range"));
	F.HUD->Sample(.016f);
	TestNotEqual(TEXT("blocked but no jump pressed: not shown"), F.V().Lane, UTDFlightHUDWidget::ELane::HyperdriveBlocked);
	F.HUD->HandleHyperdriveInput(true);
	F.HUD->Sample(.016f);
	TestEqual(TEXT("jump pressed while blocked: shown"), F.V().Lane, UTDFlightHUDWidget::ELane::HyperdriveBlocked);
	TestEqual(TEXT("caps"), F.V().LaneText, FString(TEXT("HYPERDRIVE LOCKED - ENEMIES IN RANGE")));
	F.Src->Prompt = FText::FromString(TEXT("Press F to land"));
	F.HUD->Sample(.016f);
	TestEqual(TEXT("a prompt outranks it"), F.V().Lane, UTDFlightHUDWidget::ELane::Prompt);
	F.Src->Prompt = FText::GetEmpty();
	F.HUD->Sample(UTDFlightHUDWidget::BlockedSeconds);
	TestNotEqual(TEXT("3 s later: gone"), F.V().Lane, UTDFlightHUDWidget::ELane::HyperdriveBlocked);
	F.HUD->HandleHyperdriveInput(false);
	F.HUD->Sample(.016f);
	TestNotEqual(TEXT("a release does not show it"), F.V().Lane, UTDFlightHUDWidget::ELane::HyperdriveBlocked);

	F.Src->Boundary = FText::FromString(TEXT("Edge of the combat zone"));
	F.Src->Altitude = 45.f;
	F.HUD->Sample(.016f);
	TestEqual(TEXT("boundary warning"), F.V().Boundary, FString(TEXT("EDGE OF THE COMBAT ZONE")));
	TestEqual(TEXT("altitude"), F.V().Altitude, 45.f);
	return true;
}

// Combat states: a Switch Strike window opening plays the reward pulse and drains; a focus order shows its own line.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDCombatStates, "TriadDrift.UI.FlightHUD.P2.SwitchStrikeAndFocus", TD_TEST_FLAGS)
bool FTDFlightHUDCombatStates::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.HUD->Sample(.016f);
	TestEqual(TEXT("no window: no pulse"), F.V().SwitchStrikePulse, 0.f);
	F.Src->SwitchStrike = 1.f;
	F.HUD->Sample(.016f);
	F.HUD->Sample(.1f);
	TestTrue(TEXT("window opens: reward pulse running"), F.V().SwitchStrikePulse > 0.f && F.V().SwitchStrikePulse < 1.f);
	F.Src->SwitchStrike = .6f;
	F.HUD->Sample(UTDFlightHUDWidget::StrikePulseSeconds);
	TestEqual(TEXT("draining: the pulse is over"), F.V().SwitchStrikePulse, 0.f);
	TestEqual(TEXT("draining value"), F.V().SwitchStrike, .6f);
	F.Src->SwitchStrike = 1.f;
	F.HUD->Sample(.016f);
	TestTrue(TEXT("re-armed: pulses again"), F.V().SwitchStrikePulse > 0.f);
	F.Src->Focus = .4f;
	F.HUD->Sample(.016f);
	TestEqual(TEXT("focus order"), F.V().Focus, .4f);
	return true;
}

// Review M3: objective waypoints (planets, gates, generators, the landing pad) are drawn by this HUD, with range.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDWaypoints, "TriadDrift.UI.FlightHUD.Review.ObjectiveWaypoints", TD_TEST_FLAGS)
bool FTDFlightHUDWaypoints::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	AActor* Pad = F.W.Spawn<AStaticMeshActor>(FVector(1600000.f, 0.f, 0.f)); // 16 km
	AActor* Gate = F.W.Spawn<AStaticMeshActor>(FVector(0.f, 45000.f, 0.f));
	F.Src->Waypoints = { Pad, Gate, nullptr };
	F.HUD->Sample(.016f);
	const auto& Wps = F.V().Waypoints;
	if (!TestEqual(TEXT("two waypoints (null skipped)"), Wps.Num(), 2))
	{
		return false;
	}
	TestEqual(TEXT("pad range, metres"), Wps[0].Distance, 16000.f, 1.f);
	TestEqual(TEXT("gate range"), Wps[1].Distance, 450.f, 1.f);
	for (const auto& Wp : Wps)
	{
		const bool bInFrame = Wp.Pos.X >= UTDFlightHUDWidget::SafeX - .5f && Wp.Pos.X <= 1920.f - UTDFlightHUDWidget::SafeX + .5f
			&& Wp.Pos.Y >= UTDFlightHUDWidget::SafeY - .5f && Wp.Pos.Y <= 1080.f - UTDFlightHUDWidget::SafeY + .5f;
		TestTrue(TEXT("off-screen waypoint sits on the safe frame"), Wp.bOnScreen || bInFrame);
	}
	TestTrue(TEXT("claims the ObjectiveMarkers surface"), GetDefault<UTDFlightHUDWidget>()->bCoversObjectiveMarkers);
	return true;
}

// The pad dead ahead sits behind the player's own ship: the waypoint is lifted just above the ship zone (Lead, Nov 13).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDWaypointLift, "TriadDrift.UI.FlightHUD.Review.WaypointLiftedAboveShip", TD_TEST_FLAGS)
bool FTDFlightHUDWaypointLift::RunTest(const FString& Parameters)
{
	FVector2D Draw;
	// Dead ahead, just below the reticle: where the chase camera's ship is (the Verdance capture at 5 km).
	TestTrue(TEXT("pad behind the ship: lifted"), UTDFlightHUDWidget::LiftWaypoint(FVector2D(960, 700), Draw));
	TestTrue(TEXT("drawn above the ship zone"), Draw.Y < UTDFlightHUDWidget::ShipZone().Min.Y);
	TestEqual(TEXT("same column as the true point"), (float)Draw.X, 960.f, .01f);
	TestTrue(TEXT("lifted point stays below the reticle cluster's arcs"), Draw.Y > UTDFlightHUDWidget::CenterY + UTDFlightHUDWidget::ShieldRadius);
	TestFalse(TEXT("a waypoint elsewhere on screen: drawn where it is"), UTDFlightHUDWidget::LiftWaypoint(FVector2D(400, 300), Draw));
	TestTrue(TEXT("...at its own spot"), Draw.Equals(FVector2D(400, 300)));
	return true;
}

// Review M4: the onboarding card through the real hint component: the second essential (Fire) reads "2 / 4".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDHintCard, "TriadDrift.UI.FlightHUD.Review.HintCardStep", TD_TEST_FLAGS)
bool FTDFlightHUDHintCard::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("squad"), Squad) || !TestNotNull(TEXT("controller"), PC))
	{
		return false;
	}
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 20000.f)));
	UTDHintComponent* Hints = PC->GetHints();
	if (!TestNotNull(TEXT("hints"), Hints))
	{
		return false;
	}
	Hints->UseMemoryOnlyForTests();
	Hints->TimeoutSeconds = 2.f;
	W.Advance(.5f);
	UTDFlightHUDWidget* HUD = CreateWidget<UTDFlightHUDWidget>(PC, UTDFlightHUDWidget::StaticClass());
	HUD->ForceGamepad = 0;
	HUD->SetDataSource(Squad);
	if (UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld()))
	{
		Bus->BroadcastWaveStarted(1, .2f);
	}
	auto WaitFor = [&](ETDHint Hint)
	{
		for (float T = 0.f; T < 5.f && !(Hints->GetCurrentHint() == Hint && Hints->GetCurrentHintState() == ETDHintState::Showing); T += 1.f / 60.f)
		{
			W.Tick();
		}
		return Hints->GetCurrentHint() == Hint;
	};
	TestTrue(TEXT("fly first"), WaitFor(ETDHint::Fly));
	W.Advance(2.1f); // fly ignored: it comes back later
	TestTrue(TEXT("fire next"), WaitFor(ETDHint::Fire));
	HUD->Sample(.016f);
	const auto& V = HUD->GetView();
	TestTrue(TEXT("card shown"), V.bHintCard && V.Lane == UTDFlightHUDWidget::ELane::Hint);
	TestEqual(TEXT("card key"), V.HintKey, FString(TEXT("LMB")));
	TestEqual(TEXT("card action"), V.HintAction, FString(TEXT("FIRE")));
	TestTrue(FString::Printf(TEXT("card counter reads 2 / 4 (got %d / %d)"), V.HintStep, V.HintCount), V.HintStep == 2 && V.HintCount == 4);
	FTDTestWorld::InjectKey(PC, EKeys::LeftMouseButton, true);
	W.Advance(.1f);
	FTDTestWorld::InjectKey(PC, EKeys::LeftMouseButton, false);
	W.Tick();
	HUD->Sample(.016f);
	TestTrue(TEXT("fired: the card ticks off"), HUD->GetView().bHintDone);
	return true;
}

// Review M5 and m9: a ship with no secondary ammo shows no number; with no flown ship nothing stale is drawn.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDNoAmmoNoFlown, "TriadDrift.UI.FlightHUD.Review.NoAmmoAndNoFlownShip", TD_TEST_FLAGS)
bool FTDFlightHUDNoAmmoNoFlown::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.HUD->Sample(.016f);
	TestFalse(TEXT("no ammo on this ship: no number"), F.V().bUsesAmmo);
	F.Src->Card(ETDShipId::Lancer).SecondaryAmmo = 2;
	F.HUD->Sample(.016f);
	TestTrue(TEXT("ammo: shown"), F.V().bUsesAmmo && F.V().Ammo == 2);
	F.Src->Card(ETDShipId::Lancer).SecondaryAmmo = 0;
	F.HUD->Sample(.016f);
	TestTrue(TEXT("ran dry: still shown (as 0)"), F.V().bUsesAmmo && F.V().Ammo == 0);
	F.Src->SetSquad(ETDShipId::Rift);
	F.HUD->Sample(.016f);
	TestFalse(TEXT("Rift never had ammo: no number"), F.V().bUsesAmmo);

	for (FTDShipHUD& Card : F.Src->Cards)
	{
		Card.bPlayerControlled = false; // the flown ship is down before the retake
	}
	F.Src->Card(ETDShipId::Rift).bAlive = false;
	F.HUD->Sample(.016f);
	TestFalse(TEXT("no flown ship"), F.V().bHasFlown);
	TestFalse(TEXT("no flown ship: no ability, dome, ammo or boost"), F.V().bHasAbility || F.V().bHasPower2 || F.V().bUsesAmmo || F.V().bShowBoost);
	return true;
}

// Review: every surface has exactly one drawer. Engineering HUD: it claims all but subtitles and pause (dev HUD /
// pause screen draw those). Dev HUD: no widget, the dev HUD's full read-out draws everything.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDCoverMatrix, "TriadDrift.UI.FlightHUD.Review.CoverMatrix", TD_TEST_FLAGS)
bool FTDFlightHUDCoverMatrix::RunTest(const FString& Parameters)
{
	// What ATDDebugHUD draws in supplement mode when a surface is NOT claimed (Core/TDDebugHUD.cpp DrawHUD); the pause
	// line is the pause screen's (its widget claims Pause while open).
	const TSet<ETDHUDSurface> DevSupplement = { ETDHUDSurface::DamageIndicators, ETDHUDSurface::SquadMarkers, ETDHUDSurface::HyperdriveBlockText,
		ETDHUDSurface::Altitude, ETDHUDSurface::Subtitles, ETDHUDSurface::Pause, ETDHUDSurface::CombatStates, ETDHUDSurface::ObjectiveMarkers };
	const UEnum* Surfaces = StaticEnum<ETDHUDSurface>();
	const UTDFlightHUDWidget* CDO = GetDefault<UTDFlightHUDWidget>();
	for (int32 I = 0; I < Surfaces->NumEnums() - 1; ++I)
	{
		const ETDHUDSurface Surface = static_cast<ETDHUDSurface>(Surfaces->GetValueByIndex(I));
		const FBoolProperty* Claim = FindFProperty<FBoolProperty>(UTDFlightHUDWidget::StaticClass(), UTDUIHostSubsystem::GetCoverPropertyName(Surface));
		const bool bWidget = Claim && Claim->GetPropertyValue_InContainer(CDO);
		const bool bDev = !bWidget && DevSupplement.Contains(Surface);
		TestTrue(FString::Printf(TEXT("engineering HUD: %s has exactly one drawer (widget %d, dev %d)"), *Surfaces->GetNameStringByIndex(I),
					 bWidget ? 1 : 0, bDev ? 1 : 0),
			bWidget != bDev);
	}
	return true;
}

// Review: layout at 1280x720, 2560x1080 and 3440x1440: every drawn element inside the safe frame, the vignette at the
// screen edge (the whole widget), coverage in budget.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDLayouts, "TriadDrift.UI.FlightHUD.Review.LayoutsAndVignette", TD_TEST_FLAGS)
bool FTDFlightHUDLayouts::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.Src->Card(ETDShipId::Lancer).Health01 = .2f;	// vignette on
	F.Src->Card(ETDShipId::Lancer).SecondaryAmmo = 3;
	F.Src->Card(ETDShipId::Lancer).Boost01 = .5f;
	F.Src->Objective = FText::FromString(TEXT("Clear the Null Armada"));
	F.Src->Wave.WaveNumber = 2;
	F.Src->Wave.bWaveActive = true;
	F.Src->Wave.EnemiesAlive = 6;
	F.Src->AddMarker(true, FVector2D(.55f, .45f), FVector2D(.3f, -.2f), 30000.f).bIsCurrentTarget = true;
	F.Src->AddMarker(false, FVector2D::ZeroVector, FVector2D(-1.f, .1f), 90000.f);
	F.Src->AddMarker(false, FVector2D::ZeroVector, FVector2D(.1f, 1.f), 90000.f);	// bottom edge: must avoid the strip
	F.Src->Missile = ETDMissileWarning::Tracking;
	F.Src->Boundary = FText::FromString(TEXT("Edge of the combat zone"));
	F.Src->Hyperdrive = .4f;
	F.Src->Waypoints = { F.W.Spawn<AStaticMeshActor>(FVector(-50000.f, 0.f, 0.f)) };
	F.HUD->HandleHyperdriveInput(false);
	F.Src->Blocked = FText::FromString(TEXT("Hyperdrive locked - enemies in range"));
	F.HUD->HandleHyperdriveInput(true);
	TSharedRef<SWidget> Slate = F.HUD->TakeWidget();
	for (const FVector2D Res : { FVector2D(1280, 720), FVector2D(2560, 1080), FVector2D(3440, 1440) })
	{
		const FString Tag = FString::Printf(TEXT(" @ %.0fx%.0f"), Res.X, Res.Y);
		F.HUD->Sample(.016f);
		F.HUD->Sample(4.f); // objective collapsed
		TSharedRef<SWindow> Window = SNew(SWindow).ClientSize(Res);
		FSlateWindowElementList List(Window);
		FHittestGrid Grid;
		const FPaintArgs Args(nullptr, Grid, FVector2D::ZeroVector, FPlatformTime::Seconds(), .016f);
		Slate->SlatePrepass(1.f);
		Slate->Paint(Args, FGeometry::MakeRoot(Res, FSlateLayoutTransform()), FSlateRect(0, 0, Res.X, Res.Y), List, 0, FWidgetStyle(), true);
		const FBox2D Safe(FVector2D(UTDFlightHUDWidget::SafeX - .5f, UTDFlightHUDWidget::SafeY - .5f),
			FVector2D(1920.f - UTDFlightHUDWidget::SafeX + .5f, 1080.f - UTDFlightHUDWidget::SafeY + .5f));
		for (const TPair<FName, FBox2D>& Box : F.HUD->LastBoxes)
		{
			if (Box.Key == TEXT("hitarc") || Box.Key == TEXT("switchstrike"))
			{
				continue; // rings around the reticle, centred well inside the frame
			}
			TestTrue(FString::Printf(TEXT("%s inside the safe frame%s"), *Box.Key.ToString(), *Tag), Safe.IsInside(Box.Value));
		}
		const FBox2D Vig = F.HUD->LastVignetteRect;
		TestTrue(TEXT("vignette spans the whole screen") + Tag, Vig.bIsValid && Vig.Min.IsNearlyZero() && Vig.Max.Equals(Res, 1.f));
		// The strip and an edge arrow never overlap.
		FBox2D Strip(ForceInit);
		for (const auto& Box : F.HUD->LastBoxes)
		{
			if (Box.Key == TEXT("chip")) { Strip += Box.Value; }
		}
		for (const auto& Box : F.HUD->LastBoxes)
		{
			if (Box.Key == TEXT("edge") || Box.Key == TEXT("waypoint_edge"))
			{
				TestFalse(TEXT("edge marker clear of the squad strip") + Tag, Strip.Intersect(Box.Value));
			}
		}
		TestTrue(TEXT("coverage under 8%") + Tag, F.HUD->GetCoverage01() < .08f);
	}
	return true;
}

// Director's Cut moments: CUT turns the HUD off, the slate holds it at 40%, it fades back in over 0.25 s; pause dims it
// to 30%; a money shot leaves only the reticle and the arcs.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDDirector, "TriadDrift.UI.FlightHUD.P3.DirectorMoments", TD_TEST_FLAGS)
bool FTDFlightHUDDirector::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.HUD->ForceMoneyShot = 0;
	F.HUD->ForcePaused = 0;
	F.HUD->Sample(.016f);
	TestEqual(TEXT("normal: full opacity"), F.V().HUDOpacity, 1.f);

	F.HUD->HandleSceneCut(TEXT("Wave2"), 2, 2.f);
	F.HUD->Sample(.016f);
	TestTrue(TEXT("CUT: off at once"), F.V().bInCut && F.V().HUDOpacity == 0.f);
	F.HUD->Sample(1.f);
	TestEqual(TEXT("still off during the cut delay"), F.V().HUDOpacity, 0.f);
	F.HUD->HandleTakeStarted(TEXT("Wave2"), 2);
	F.HUD->Sample(.05f);
	F.HUD->Sample(.3f);
	TestTrue(TEXT("take starts: slate, HUD at 40%"), F.V().bInSlate && FMath::IsNearlyEqual(F.V().HUDOpacity, UTDFlightHUDWidget::SlateOpacity, .01f));
	F.HUD->Sample(UTDFlightHUDWidget::SlateSeconds - .35f - .01f); // just before the slate ends (0.35 s already sampled)
	TestTrue(TEXT("still behind the slate"), F.V().bInSlate);
	F.HUD->Sample(.1f);
	TestTrue(TEXT("slate over: fading back in"), !F.V().bInSlate && F.V().HUDOpacity > UTDFlightHUDWidget::SlateOpacity && F.V().HUDOpacity < 1.f);
	F.HUD->Sample(UTDFlightHUDWidget::CutFadeInSeconds);
	TestEqual(TEXT("back to full after the 0.25 s fade"), F.V().HUDOpacity, 1.f);

	F.HUD->ForcePaused = 1;
	F.HUD->Sample(.5f);
	TestTrue(TEXT("paused: dimmed to 30%"), F.V().bPaused && FMath::IsNearlyEqual(F.V().HUDOpacity, UTDFlightHUDWidget::PauseOpacity, .01f));
	F.HUD->ForcePaused = 0;
	F.HUD->Sample(.5f);

	F.HUD->ForceMoneyShot = 1;
	F.HUD->Sample(.05f);
	TestTrue(TEXT("money shot: easing in"), F.V().MoneyShot > 0.f && F.V().MoneyShot < 1.f);
	F.HUD->Sample(UTDFlightHUDWidget::MoneyShotEaseSeconds);
	TestEqual(TEXT("money shot: in"), F.V().MoneyShot, 1.f);
	TSharedRef<SWidget> Slate = F.HUD->TakeWidget();
	TSharedRef<SWindow> Window = SNew(SWindow).ClientSize(FVector2D(1920, 1080));
	FSlateWindowElementList List(Window);
	FHittestGrid Grid;
	Slate->SlatePrepass(1.f);
	Slate->Paint(FPaintArgs(nullptr, Grid, FVector2D::ZeroVector, FPlatformTime::Seconds(), .016f), FGeometry::MakeRoot(FVector2D(1920, 1080), FSlateLayoutTransform()),
		FSlateRect(0, 0, 1920, 1080), List, 0, FWidgetStyle(), true);
	TestTrue(TEXT("money shot: reticle kept"), F.HUD->LastBoxes.ContainsByPredicate([](const TPair<FName, FBox2D>& B) { return B.Key == TEXT("reticle"); }));
	TestTrue(TEXT("money shot: arcs kept"), F.HUD->LastBoxes.ContainsByPredicate([](const TPair<FName, FBox2D>& B) { return B.Key == TEXT("arcs"); }));
	TestFalse(TEXT("money shot: strip, objective and pip hidden"), F.HUD->LastBoxes.ContainsByPredicate([](const TPair<FName, FBox2D>& B)
		{ return B.Key == TEXT("chip") || B.Key == TEXT("objective") || B.Key == TEXT("pip"); }));
	F.HUD->ForceMoneyShot = 0;
	F.HUD->Sample(UTDFlightHUDWidget::MoneyShotEaseSeconds + .01f);
	TestEqual(TEXT("money shot over: back within 150 ms"), F.V().MoneyShot, 0.f);
	return true;
}

// The real event bus drives it (not only the handlers called directly).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDDirectorBus, "TriadDrift.UI.FlightHUD.P3.DirectorMomentsFromTheBus", TD_TEST_FLAGS)
bool FTDFlightHUDDirectorBus::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.HUD->ForceMoneyShot = 0;
	F.HUD->ForcePaused = 0;
	F.HUD->Sample(.016f); // binds the bus
	UTDEventBus* Bus = UTDEventBus::Get(F.W.GetWorld());
	if (!TestNotNull(TEXT("bus"), Bus))
	{
		return false;
	}
	Bus->OnSceneCut.Broadcast(TEXT("Wave1"), 2, 1.f);
	F.HUD->Sample(.016f);
	TestTrue(TEXT("bus CUT: off"), F.V().bInCut);
	Bus->OnTakeStarted.Broadcast(TEXT("Wave1"), 2);
	F.HUD->Sample(.5f);
	TestTrue(TEXT("bus take: slate"), F.V().bInSlate);
	return true;
}

// Hit feel at the reticle: a hit tick, a kill X that a later hit never cuts short, both timing out.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDHitMarks, "TriadDrift.UI.FlightHUD.HitAndKillMarkers", TD_TEST_FLAGS)
bool FTDFlightHUDHitMarks::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.HUD->Sample(.016f);
	TestEqual(TEXT("idle: no marks"), F.V().HitMark + F.V().KillMark, 0.f);
	F.HUD->HandleHitConfirmed(nullptr, false);
	F.HUD->Sample(.05f);
	TestTrue(TEXT("hit: tick showing"), F.V().HitMark > 0.f && F.V().HitMark < 1.f);
	F.HUD->Sample(UTDFlightHUDWidget::HitMarkSeconds);
	TestEqual(TEXT("hit tick gone after 0.15 s"), F.V().HitMark, 0.f);
	F.HUD->HandleHitConfirmed(F.Src, true);
	F.HUD->Sample(.05f);
	TestTrue(TEXT("kill: X showing"), F.V().KillMark > 0.f);
	F.HUD->HandleHitConfirmed(nullptr, false);
	F.HUD->Sample(.05f);
	TestTrue(TEXT("a hit during the kill X does not replace it"), F.V().KillMark > 0.f && F.V().HitMark == 0.f);
	F.HUD->Sample(UTDFlightHUDWidget::KillMarkSeconds);
	TestEqual(TEXT("kill X gone after 0.35 s"), F.V().KillMark, 0.f);
	return true;
}

// The host's cover answer both ways: the engineering HUD claims CombatStates (the dev HUD's line goes), Nick's does not.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDCombatCover, "TriadDrift.UI.FlightHUD.P2.CombatStatesCoverBothWays", TD_TEST_FLAGS)
bool FTDFlightHUDCombatCover::RunTest(const FString& Parameters)
{
	FTDFlightScopedCVar Space(TEXT("TD.UI.SpaceHUD"), -1);
	TestEqual(TEXT("the default is the engineering HUD"), UTDUIHostSettings::GetSpaceHUDChoice(), ETDSpaceHUDChoice::Engineering);
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
	Space.Set(2);
	Host->Evaluate();
	TestTrue(TEXT("engineering HUD up"), Host->GetSpaceHUD() && Host->GetSpaceHUD()->IsA<UTDFlightHUDWidget>());
	TestTrue(TEXT("engineering HUD: CombatStates covered (dev line off)"), UTDUIHostSubsystem::IsSurfaceCoveredByNativeHUD(W.GetWorld(), ETDHUDSurface::CombatStates));
	Space.Set(1);
	Host->Evaluate();
	if (Host->GetSpaceHUD())
	{
		TestFalse(TEXT("Nick's HUD: CombatStates not covered (dev line stays)"), UTDUIHostSubsystem::IsSurfaceCoveredByNativeHUD(W.GetWorld(), ETDHUDSurface::CombatStates));
	}
	Space.Set(0);
	Host->Evaluate();
	TestFalse(TEXT("dev HUD: nothing covered"), UTDUIHostSubsystem::IsSurfaceCoveredByNativeHUD(W.GetWorld(), ETDHUDSurface::CombatStates));
	return true;
}

// The widget claims the dev HUD's surfaces it now draws (and leaves subtitles and pause alone).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDCovers, "TriadDrift.UI.FlightHUD.P2.ClaimsDevHUDSurfaces", TD_TEST_FLAGS)
bool FTDFlightHUDCovers::RunTest(const FString& Parameters)
{
	const UTDFlightHUDWidget* CDO = GetDefault<UTDFlightHUDWidget>();
	for (const ETDHUDSurface Surface : { ETDHUDSurface::DamageIndicators, ETDHUDSurface::SquadMarkers, ETDHUDSurface::InteractPrompt,
			 ETDHUDSurface::HyperdriveBlockText, ETDHUDSurface::Altitude, ETDHUDSurface::BoundaryWarning, ETDHUDSurface::CombatStates })
	{
		const FBoolProperty* Claim = FindFProperty<FBoolProperty>(UTDFlightHUDWidget::StaticClass(), UTDUIHostSubsystem::GetCoverPropertyName(Surface));
		TestTrue(FString::Printf(TEXT("claims %s"), *UTDUIHostSubsystem::GetCoverPropertyName(Surface).ToString()), Claim && Claim->GetPropertyValue_InContainer(CDO));
	}
	for (const ETDHUDSurface Surface : { ETDHUDSurface::Subtitles, ETDHUDSurface::Pause })
	{
		TestNull(FString::Printf(TEXT("leaves %s"), *UTDUIHostSubsystem::GetCoverPropertyName(Surface).ToString()),
			FindFProperty<FBoolProperty>(UTDFlightHUDWidget::StaticClass(), UTDUIHostSubsystem::GetCoverPropertyName(Surface)));
	}
	return true;
}

// Paints the widget once through Slate's real draw path in every P1 state (the first live run crashed in NativePaint, which
// the sampling tests never reach). No renderer is needed: elements go to a window element list.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightHUDPaints, "TriadDrift.UI.FlightHUD.PaintsEveryState", TD_TEST_FLAGS)
bool FTDFlightHUDPaints::RunTest(const FString& Parameters)
{
	FTDFlightHUDFixture F;
	F.Src->Card(ETDShipId::Rift).Health01 = .3f;			// trouble badge + pulsing outline
	F.Src->Card(ETDShipId::Bastion).bAlive = false;			// crossed icon + respawn seconds
	F.Src->Card(ETDShipId::Bastion).RespawnSeconds = 3.f;
	F.Src->Card(ETDShipId::Lancer).Health01 = .2f;			// red hull band
	F.Src->Card(ETDShipId::Lancer).Shield01 = 0.f;
	F.Src->Card(ETDShipId::Lancer).PowerCooldown01 = .3f;	// cooling pip
	F.Src->Objective = FText::FromString(TEXT("WAVE 2 · Clear the Null Armada and keep the wing alive"));
	F.Src->Wave.WaveNumber = 2;
	F.Src->Wave.WaveCount = 5;
	F.Src->Wave.bWaveActive = true;
	F.Src->Wave.EnemiesAlive = 12;
	// P2 elements, all at once.
	FTDTargetMarker& Cur = F.Src->AddMarker(true, FVector2D(.6f, .4f), FVector2D(.3f, -.2f), 41200.f);
	Cur.bIsCurrentTarget = true;
	Cur.bLocked = true;
	F.Src->AddMarker(true, FVector2D(.3f, .3f), FVector2D(-.5f, -.5f), 90000.f).bLocked = true;
	F.Src->AddMarker(false, FVector2D::ZeroVector, FVector2D(1.f, .2f), 120000.f);
	F.Src->AddMarker(false, FVector2D::ZeroVector, FVector2D(-.2f, -1.f), 50000.f).bIncomingMissile = true;
	FTDDamageIndicator Hit;
	Hit.WorldDirection = FVector(0, -1, 0);
	Hit.Strength01 = .7f;
	F.Src->Hits = { Hit };
	F.Src->Missile = ETDMissileWarning::Imminent;
	F.Src->Blocked = FText::FromString(TEXT("Hyperdrive locked - enemies in range"));
	F.Src->Boundary = FText::FromString(TEXT("Edge of the combat zone"));
	F.Src->SwitchStrike = .5f;
	F.Src->Altitude = 30.f;
	F.Src->Hyperdrive = .6f;
	F.Src->Card(ETDShipId::Lancer).Ability = ETDAbilityId::EMPPulse;
	F.Src->Card(ETDShipId::Lancer).Heat01 = .95f;
	F.Src->Card(ETDShipId::Lancer).Boost01 = .5f;
	F.Src->Card(ETDShipId::Lancer).SecondaryAmmo = 4;
	F.HUD->HandleHyperdriveInput(true);
	F.HUD->HandleHitConfirmed(F.Src, true); // the kill X
	F.Src->SwitchStrike = 1.f;				// the Switch Strike burst
	F.Src->Focus = .5f;

	TSharedRef<SWidget> Slate = F.HUD->TakeWidget();
	TSharedRef<SWindow> Window = SNew(SWindow).ClientSize(FVector2D(1920, 1080));
	auto PaintOnce = [&](const TCHAR* Label)
	{
		FSlateWindowElementList List(Window);
		FHittestGrid Grid;
		const FGeometry Geometry = FGeometry::MakeRoot(FVector2D(1920, 1080), FSlateLayoutTransform());
		const FPaintArgs Args(nullptr, Grid, FVector2D::ZeroVector, FPlatformTime::Seconds(), .016f);
		Slate->SlatePrepass(1.f);
		const int32 Layer = Slate->Paint(Args, Geometry, FSlateRect(0, 0, 1920, 1080), List, 0, FWidgetStyle(), true);
		TestTrue(FString::Printf(TEXT("%s: painted (layer %d)"), Label, Layer), Layer > 0);
	};
	// Each element must actually be drawn (review: Layer > 0 alone would pass on an empty widget).
	auto Drew = [&](const TCHAR* Label, std::initializer_list<const TCHAR*> Names)
	{
		for (const TCHAR* Name : Names)
		{
			TestTrue(FString::Printf(TEXT("%s: drew %s"), Label, Name),
				F.HUD->LastBoxes.ContainsByPredicate([Name](const TPair<FName, FBox2D>& B) { return B.Key == FName(Name); }));
		}
	};
	F.HUD->Sample(.016f);
	PaintOnce(TEXT("expanding objective, trouble, down, cooling"));
	Drew(TEXT("alert state"), { TEXT("reticle"), TEXT("arcs"), TEXT("pip"), TEXT("chip"), TEXT("objective"), TEXT("bracket"), TEXT("tick"), TEXT("edge"), TEXT("missile_edge"), TEXT("hitarc"), TEXT("missile"), TEXT("boundary"), TEXT("boost"), TEXT("hyperdrive"), TEXT("lane"), TEXT("ability"), TEXT("heat"), TEXT("ammo"), TEXT("altitude"), TEXT("switchstrike"), TEXT("focus"), TEXT("killmark") });
	F.HUD->Sample(1.f);
	PaintOnce(TEXT("objective fully expanded"));
	F.HUD->Sample(3.f);
	PaintOnce(TEXT("collapsed"));
	AddInfo(FString::Printf(TEXT("coverage with every alert element up: %.2f%%"), F.HUD->GetCoverage01() * 100.f));
	TestTrue(TEXT("coverage under the 8% budget even in the alert state"), F.HUD->GetCoverage01() > 0.f && F.HUD->GetCoverage01() < .08f);
	F.Src->Card(ETDShipId::Lancer).PowerCooldown01 = 1.f;
	F.HUD->Sample(.016f);
	F.HUD->Sample(.1f);
	PaintOnce(TEXT("ready pulse"));
	F.HUD->ForceGamepad = 1;
	F.HUD->Sample(.016f);
	PaintOnce(TEXT("pad glyphs, no keycaps"));
	F.Src->SetSquad(ETDShipId::Bastion);
	F.HUD->Sample(.016f);
	PaintOnce(TEXT("Bastion: second power pip"));
	Drew(TEXT("Bastion"), { TEXT("power2"), TEXT("pip") });
	F.Src->Cards.Reset();
	F.HUD->Sample(.016f);
	PaintOnce(TEXT("no squad"));
	TestFalse(TEXT("no squad: no arcs or pip"), F.HUD->LastBoxes.ContainsByPredicate([](const TPair<FName, FBox2D>& B) { return B.Key == TEXT("arcs") || B.Key == TEXT("pip"); }));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
