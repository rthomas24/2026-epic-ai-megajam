// Flight feel against real enemies (T4's fighters shooting back): does flying evasively actually reduce the hits a
// pilot takes? A measurement for the feel pass, with a floor so a regression (dodging stops mattering) shows up.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight.Feel

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "EngineUtils.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"

// T4 is adding ATDEnemyShip::SetAimRandomSeed (its aim-error stream is seeded from the object id today); call it when
// present so this measurement is fully seeded, without depending on it to compile.
template <typename T>
concept TDFlightFeel_HasAimSeed = requires(T* E) { E->SetAimRandomSeed(0); };
template <typename T>
static void TDFlightFeel_SeedAimImpl(T* E, int32 Seed)
{
	if constexpr (TDFlightFeel_HasAimSeed<T>) { E->SetAimRandomSeed(Seed); }
}
static void TDFlightFeel_SeedAim(ATDEnemyShip* E, int32 Seed) { TDFlightFeel_SeedAimImpl(E, Seed); }

namespace TDFlightFeel
{
	enum class EStyle : uint8 { Straight, Weave, Drift, Strafe, Dodge };

	/** Damage the player takes in Seconds against three fighters, flying one style. Same seed for every style. */
	float DamageTaken(EStyle Style, float Seconds, float Fps, int32 Seed, FString* OutBreakdown = nullptr, float DodgeSpeed = -1.f, float DodgeCooldown = -1.f)
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
		if (DodgeSpeed > 0.f || DodgeCooldown > 0.f)
		{
			FTDFlightTuning T = Player->GetShipMovement()->GetFlightTuning();
			if (DodgeSpeed > 0.f) { T.DodgeSpeed = DodgeSpeed; }
			if (DodgeCooldown > 0.f) { T.DodgeCooldown = DodgeCooldown; }
			Player->GetShipMovement()->ApplyFlightTuning(T);
		}
		Player->SetPlayerControlled(true);
		const FVector Spots[] = { FVector(15000.f, 6000.f, 1000.f), FVector(12000.f, -7000.f, -1500.f), FVector(-9000.f, 9000.f, 2000.f) };
		int32 Index = 0;
		for (const FVector& At : Spots)
		{
			ATDEnemyShip* E = W.Spawn<ATDEnemyShip>(At, (FVector::ZeroVector - At).Rotation());
			E->InitFromSpec(TDEnemyIds::ShardFighter);
			// Seed every random stream we can reach so the number is stable between runs.
			const int32 S = Seed * 100 + Index++;
			if (UTDWeaponComponent* Wp = E->GetWeapons()) { Wp->SetRandomSeed(S); }
			if (ATDEnemyAIController* AI = Cast<ATDEnemyAIController>(E->GetController())) { AI->SetRandomSeed(S + 50); }
			TDFlightFeel_SeedAim(E, S + 77);
		}
		const float Hull0 = Player->GetHealth()->GetHull();
		UTDFlightEventCounter* Rec = NewObject<UTDFlightEventCounter>();
		Rec->AddToRoot();
		ON_SCOPE_EXIT { Rec->RemoveFromRoot(); };
		Player->GetHealth()->OnDamaged.AddDynamic(Rec, &UTDFlightEventCounter::HandleDamaged);
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
			case EStyle::Dodge:
			{
				// Evasive flying as a pilot does it: weave and boost (as Weave) and, on top, a well-timed dodge: watch the
				// shooter, not the bolt. When an enemy within 500 m has its nose on the player (about to fire with lead),
				// dodge away from its line.
				In.Aim = FVector2D(FMath::Fmod(T, 1.2f) < 0.6f ? 1.f : -1.f, FMath::Fmod(T, 1.8f) < 0.9f ? 0.6f : -0.6f);
				In.bBoost = FMath::Fmod(T, 2.4f) < 0.8f;
				for (TActorIterator<ATDEnemyShip> It(W.GetWorld()); It; ++It)
				{
					const FVector ToPlayer = Player->GetActorLocation() - It->GetActorLocation();
					const float Dist = ToPlayer.Size();
					if (Dist < 50000.f && Dist > 1.f && FVector::DotProduct(It->GetActorForwardVector(), ToPlayer / Dist) > FMath::Cos(FMath::DegreesToRadians(6.f)))
					{
						const float Side = FVector::DotProduct(It->GetActorRightVector(), Player->GetActorRightVector()) > 0.f ? 1.f : -1.f;
						Player->TryDodge(Side);
						break;
					}
				}
				break;
			}
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
		if (OutBreakdown)
		{
			*OutBreakdown = FString::Printf(TEXT("hits %d: energy %.0f kinetic %.0f explosive %.0f ram %.0f env %.0f"), Rec->DamageHits,
				Rec->DamageByKind[0], Rec->DamageByKind[1], Rec->DamageByKind[2], Rec->DamageByKind[4], Rec->DamageByKind[5]);
		}
		return FMath::Max(0.f, Hull0 - Player->GetHealth()->GetHull());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightFeelDodging, "TriadDrift.Flight.Feel.EvasiveFlyingReducesHits", TD_TEST_FLAGS)
bool FTDFlightFeelDodging::RunTest(const FString& Parameters)
{
	using namespace TDFlightFeel;
	float Straight = 0.f, Weave = 0.f, Drift = 0.f, Strafe = 0.f, Dodge = 0.f;
	// 6 seeds x 12 s per style. The enemy aim-error stream is not seedable yet (T4: SetAimRandomSeed), so expect
	// about +-10 points between runs; the asserts leave room for that.
	const int32 Seeds[] = { 11, 23, 37, 41, 53, 67 };
	for (const int32 Seed : Seeds)
	{
		Straight += DamageTaken(EStyle::Straight, 12.f, 60.f, Seed);
		Weave += DamageTaken(EStyle::Weave, 12.f, 60.f, Seed);
		Drift += DamageTaken(EStyle::Drift, 12.f, 60.f, Seed);
		Strafe += DamageTaken(EStyle::Strafe, 12.f, 60.f, Seed);
		Dodge += DamageTaken(EStyle::Dodge, 12.f, 60.f, Seed);
	}
	auto Pct = [Straight](float V) { return Straight > 0.f ? 100.f * V / Straight : 0.f; };
	AddInfo(FString::Printf(TEXT("damage over 6 x 12 s vs 3 fighters: straight %.0f, weave+boost %.0f (%.0f%%), drift %.0f (%.0f%%), strafe %.0f (%.0f%%), weave+boost+timed dodge %.0f (%.0f%%)"),
		Straight, Weave, Pct(Weave), Drift, Pct(Drift), Strafe, Pct(Strafe), Dodge, Pct(Dodge)));
	TestTrue(TEXT("the fighters land hits on a straight flyer (they shoot back)"), Straight > 0.f);
	TestTrue(*FString::Printf(TEXT("weaving takes clearly fewer hits than flying straight (%.0f%%)"), Pct(Weave)), Weave < Straight * 0.8f);
	// Lead target: 35%. Measured (Nov 12, 8 seeds): weave alone ~60-67%, weave + timed dodge ~50% at 100 m/s, ~41% at
	// 120 m/s. The assert guards the gain the dodge actually gives; the 35% target is reported, not asserted.
	TestTrue(*FString::Printf(TEXT("weaving with well-timed dodges takes clearly less than straight (%.0f%%; target 35%%)"), Pct(Dodge)), Dodge <= Straight * 0.62f);
	// Drift is a turning tool, not a dodge (velocity is locked while the nose swings): it must not be worse than straight.
	// An observation, not a requirement (Lead, at integration): drift is an aiming tool, and this number moves with enemy tuning.
	AddInfo(FString::Printf(TEXT("drifting vs flying straight: %.0f%%"), Pct(Drift)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightFeelDodgeSweep, "TDExplore.Flight.DodgeSweep", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTDFlightFeelDodgeSweep::RunTest(const FString& Parameters)
{
	// Exploration only (outside the TriadDrift filter; run with -Filter TDExplore): damage vs dodge speed / cooldown.
	using namespace TDFlightFeel;
	struct FVariant { float Speed; float Cooldown; };
	const FVariant Variants[] = { { 8000.f, 1.5f }, { 10000.f, 1.5f }, { 10000.f, 1.0f }, { 12000.f, 1.5f } };
	float Straight = 0.f;
	for (int32 Seed = 201; Seed < 209; ++Seed) { Straight += DamageTaken(EStyle::Straight, 12.f, 60.f, Seed); }
	for (const FVariant& V : Variants)
	{
		float Dodge = 0.f;
		for (int32 Seed = 201; Seed < 209; ++Seed)
		{
			Dodge += DamageTaken(EStyle::Dodge, 12.f, 60.f, Seed, nullptr, V.Speed, V.Cooldown);
		}
		AddInfo(FString::Printf(TEXT("SWEEP speed %.0f cooldown %.1f: weave+timed dodge %.0f%% of straight (%.0f)"), V.Speed / 100.f, V.Cooldown, 100.f * Dodge / FMath::Max(Straight, 1.f), Straight));
	}
	float Weave = 0.f;
	for (int32 Seed = 201; Seed < 209; ++Seed) { Weave += DamageTaken(EStyle::Weave, 12.f, 60.f, Seed); }
	AddInfo(FString::Printf(TEXT("SWEEP weave+boost alone %.0f%%"), 100.f * Weave / FMath::Max(Straight, 1.f)));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
