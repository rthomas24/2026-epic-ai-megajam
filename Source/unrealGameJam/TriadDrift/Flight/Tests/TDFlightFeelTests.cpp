// Flight feel against real enemies (T4's fighters shooting back): does flying evasively actually reduce the hits a
// pilot takes? A measurement for the feel pass, with a floor so a regression (dodging stops mattering) shows up.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight.Feel

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDFlightFeel
{
	enum class EStyle : uint8 { Straight, Weave, Drift, Strafe };

	/** Damage the player takes in Seconds against three fighters, flying one style. Same seed for every style. */
	float DamageTaken(EStyle Style, float Seconds, float Fps, int32 Seed)
	{
		FMath::RandInit(Seed);
		FTDTestWorld W;
		ATDShip* Player = W.Spawn<ATDShip>(FVector::ZeroVector);
		Player->SetTeam(ETDTeam::Player);
		if (UTDHealthComponent* H = Player->GetHealth())
		{
			H->DefaultTeam = ETDTeam::Player;
			H->InitMaxValues(100000.f, 0.f, true); // never dies: we only count what lands
		}
		W.SpawnPlayerController(Player);
		Player->SetPlayerControlled(true);
		const FVector Spots[] = { FVector(15000.f, 6000.f, 1000.f), FVector(12000.f, -7000.f, -1500.f), FVector(-9000.f, 9000.f, 2000.f) };
		for (const FVector& At : Spots)
		{
			ATDEnemyShip* E = W.Spawn<ATDEnemyShip>(At, (FVector::ZeroVector - At).Rotation());
			E->InitFromSpec(TDEnemyIds::ShardFighter);
		}
		const float Hull0 = Player->GetHealth()->GetHull();
		const float Dt = 1.f / Fps;
		const int32 Frames = FMath::RoundToInt(Seconds * Fps);
		for (int32 i = 0; i < Frames; ++i)
		{
			const float T = i * Dt;
			FTDFlightInput In;
			In.Throttle = 1.f;
			switch (Style)
			{
			case EStyle::Weave:
				// Jink every 0.6 s, both axes, with a boost burst every other jink.
				In.Aim = FVector2D(FMath::Fmod(T, 1.2f) < 0.6f ? 1.f : -1.f, FMath::Fmod(T, 1.8f) < 0.9f ? 0.6f : -0.6f);
				In.bBoost = FMath::Fmod(T, 2.4f) < 0.8f;
				break;
			case EStyle::Drift:
				// Drift-turn in alternating directions (the slide the flight model is built around).
				In.Aim = FVector2D(FMath::Fmod(T, 2.f) < 1.f ? 1.f : -1.f, 0.f);
				In.bDrift = FMath::Fmod(T, 2.f) < 1.4f;
				break;
			case EStyle::Strafe:
				// Side-slip without turning: strafe left / right / up / down every 0.7 s.
				In.Strafe = FVector2D(FMath::Fmod(T, 1.4f) < 0.7f ? 1.f : -1.f, FMath::Fmod(T, 2.1f) < 1.05f ? 1.f : -1.f);
				break;
			default:
				break;
			}
			Player->SetFlightInput(In);
			W.Tick(Dt);
		}
		return FMath::Max(0.f, Hull0 - Player->GetHealth()->GetHull());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightFeelDodging, "TriadDrift.Flight.Feel.EvasiveFlyingReducesHits", TD_TEST_FLAGS)
bool FTDFlightFeelDodging::RunTest(const FString& Parameters)
{
	using namespace TDFlightFeel;
	float Straight = 0.f, Weave = 0.f, Drift = 0.f, Strafe = 0.f;
	const int32 Seeds[] = { 11, 23, 37 };
	for (const int32 Seed : Seeds)
	{
		Straight += DamageTaken(EStyle::Straight, 12.f, 60.f, Seed);
		Weave += DamageTaken(EStyle::Weave, 12.f, 60.f, Seed);
		Drift += DamageTaken(EStyle::Drift, 12.f, 60.f, Seed);
		Strafe += DamageTaken(EStyle::Strafe, 12.f, 60.f, Seed);
	}
	auto Pct = [Straight](float V) { return Straight > 0.f ? 100.f * V / Straight : 0.f; };
	AddInfo(FString::Printf(TEXT("damage over 3 x 12 s vs 3 fighters: straight %.0f, weave+boost %.0f (%.0f%%), drift %.0f (%.0f%%), strafe %.0f (%.0f%%)"),
		Straight, Weave, Pct(Weave), Drift, Pct(Drift), Strafe, Pct(Strafe)));
	TestTrue(TEXT("the fighters land hits on a straight flyer (they shoot back)"), Straight > 0.f);
	TestTrue(*FString::Printf(TEXT("weaving takes clearly fewer hits than flying straight (%.0f vs %.0f)"), Weave, Straight), Weave < Straight * 0.8f);
	// Drift is a turning tool, not a dodge (velocity is locked while the nose swings): it must not be worse than straight.
	TestTrue(*FString::Printf(TEXT("drifting is no worse than flying straight (%.0f vs %.0f)"), Drift, Straight), Drift <= Straight * 1.05f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
