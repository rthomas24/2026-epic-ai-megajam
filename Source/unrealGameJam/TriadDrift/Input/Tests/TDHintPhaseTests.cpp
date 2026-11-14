// T3's ENG-40 teaching phases: the control hint on screen matches the phase. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.Hints.Phase

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDHintComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1HintPhase, "TriadDrift.Input.Hints.PhaseOrdersTheControlHints", TD_TEST_FLAGS)
bool FTDT1HintPhase::RunTest(const FString& Parameters)
{
	using H = ETDHint;
	using P = ETDTeachPhase;
	struct FRow { P Phase; TArray<H> Yes; TArray<H> No; };
	const TArray<FRow> Rows = {
		{ P::Evade, { H::Fly, H::Boost, H::Dodge, H::Drift, H::Hyperdrive, H::Ability }, { H::Fire, H::TargetCycle, H::Switch, H::Power } },
		{ P::TargetAndFire, { H::Fire, H::TargetCycle, H::Drift }, { H::Fly, H::Boost, H::Dodge, H::Switch, H::Power } },
		{ P::SquadSystems, { H::Switch, H::Power, H::Hyperdrive }, { H::Fly, H::Fire, H::Boost, H::Dodge, H::TargetCycle } },
		{ P::None, { H::Fly, H::Fire, H::Switch, H::Power, H::Boost, H::Dodge }, {} },
		{ P::Done, { H::Fly, H::Fire, H::Switch, H::Power, H::Boost, H::Dodge }, {} },
	};
	for (const FRow& R : Rows)
	{
		for (const H Hint : R.Yes)
		{
			TestTrue(*FString::Printf(TEXT("%s shows %s"), *UEnum::GetValueAsString(R.Phase), *UEnum::GetValueAsString(Hint)), UTDHintComponent::FitsTeachPhase(Hint, R.Phase));
		}
		for (const H Hint : R.No)
		{
			TestFalse(*FString::Printf(TEXT("%s holds %s back"), *UEnum::GetValueAsString(R.Phase), *UEnum::GetValueAsString(Hint)), UTDHintComponent::FitsTeachPhase(Hint, R.Phase));
		}
	}
	return true;
}

#endif
