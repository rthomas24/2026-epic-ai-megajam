// Per-hull boost plume size (T7, Nov 14; the ship builder found Lancer's boost plumes swamp its small hull):
// UTDShipVisuals::ExhaustBoostScale reaches the running EngineTrail effect as its "Boost" user parameter.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight.Exhaust

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/StrongObjectPtr.h"

namespace TDExhaustBoostScaleTest
{
	static const TCHAR* LancerTrail = TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Engine_Lancer.NS_TD_Engine_Lancer");

	/** The Boost the effect component actually holds (user parameter override), or -1 when it has none. */
	static float TrailBoost(const UNiagaraComponent* FX)
	{
		if (!FX)
		{
			return -1.f;
		}
		const FNiagaraVariable Var(FNiagaraTypeDefinition::GetFloatDef(), TEXT("User.Boost"));
		const FNiagaraParameterStore& Store = FX->GetOverrideParameters();
		return Store.IndexOf(Var) != INDEX_NONE ? Store.GetParameterValue<float>(Var) : -1.f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDShipExhaustBoostScale, "TriadDrift.Flight.Exhaust.BoostScalePerHullReachesTheTrail", TD_TEST_FLAGS)
bool FTDShipExhaustBoostScale::RunTest(const FString& Parameters)
{
	using namespace TDExhaustBoostScaleTest;
	UNiagaraSystem* Trail = LoadObject<UNiagaraSystem>(nullptr, LancerTrail);
	if (!Trail)
	{
		AddWarning(TEXT("NS_TD_Engine_Lancer is not on disk; only the value is checked"));
	}

	// Default: no visuals -> the authored 1 while boosting, 0 at cruise.
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	TestEqual(TEXT("no visuals, cruise: Boost 0"), Ship->GetExhaustBoostValue(), 0.f);

	// A Lancer-like visuals asset with the real Lancer engine trail and a 0.6 boost plume.
	TStrongObjectPtr<UTDShipVisuals> Visuals(NewObject<UTDShipVisuals>());
	TestEqual(TEXT("default scale 1 (every existing ship unchanged)"), Visuals->ExhaustBoostScale, 1.f);
	Visuals->ExhaustBoostScale = 0.6f;
	if (Trail)
	{
		Visuals->EngineTrail.VFX = Trail;
	}
	Ship->SetShipVisuals(Visuals.Get());
	Ship->SetPlayerControlled(true);
	// A headless run spawns no Niagara (it cannot render): give the ship a component on the real Lancer trail system, which
	// the ship's own per-frame exhaust update then feeds, exactly as it feeds a spawned one.
	UNiagaraComponent* Injected = nullptr;
	if (Trail && !Ship->GetEngineTrailEffect())
	{
		Injected = NewObject<UNiagaraComponent>(Ship);
		Injected->SetAsset(Trail);
		Ship->SetEngineTrailEffectForTests(Injected);
	}

	FTDFlightInput In;
	In.Throttle = 1.f;
	Ship->SetFlightInput(In);
	W.Advance(2.f);
	TestFalse(TEXT("cruise: not boosting"), Ship->GetExhaustBoost());
	TestEqual(TEXT("cruise: Boost 0"), Ship->GetExhaustBoostValue(), 0.f);
	if (Trail)
	{
		TestTrue(TEXT("the ship has a trail to feed"), Ship->GetEngineTrailEffect() != nullptr);
		TestEqual(TEXT("cruise: the trail holds Boost 0"), TrailBoost(Ship->GetEngineTrailEffect()), 0.f, 0.001f);
	}

	In.bBoost = true;
	Ship->SetFlightInput(In);
	W.Advance(0.5f);
	TestTrue(TEXT("boosting"), Ship->GetExhaustBoost());
	TestEqual(TEXT("boosting: Boost = the hull's 0.6"), Ship->GetExhaustBoostValue(), 0.6f, 0.001f);
	if (Trail)
	{
		TestEqual(TEXT("boosting: the running trail holds Boost 0.6"), TrailBoost(Ship->GetEngineTrailEffect()), 0.6f, 0.001f);
	}

	// Back to the authored plume: 1.
	Visuals->ExhaustBoostScale = 1.f;
	W.Advance(0.2f);
	TestEqual(TEXT("scale 1: Boost 1"), Ship->GetExhaustBoostValue(), 1.f, 0.001f);
	if (Trail)
	{
		TestEqual(TEXT("scale 1: the trail holds Boost 1"), TrailBoost(Ship->GetEngineTrailEffect()), 1.f, 0.001f);
	}
	return true;
}

#endif
