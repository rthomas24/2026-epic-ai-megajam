// Automation tests for Core. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Core

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace
{
	/** A bare actor with a health component on the given team. */
	AActor* SpawnCombatant(FTDTestWorld& W, ETDTeam Team, const FVector& Location = FVector::ZeroVector,
		float MaxHull = 100.f, float MaxShield = 100.f)
	{
		AActor* Actor = W.Spawn<AActor>(Location);
		if (!Actor)
		{
			return nullptr;
		}
		USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
		Actor->SetRootComponent(Root);
		Root->RegisterComponent();
		Actor->SetActorLocation(Location);

		UTDHealthComponent* Health = NewObject<UTDHealthComponent>(Actor, TEXT("Health"));
		Health->MaxHull = MaxHull;
		Health->MaxShield = MaxShield;
		Health->DefaultTeam = Team;
		Health->RegisterComponent();
		return Actor;
	}

	float Hit(AActor* Target, float Amount, AActor* Instigator = nullptr)
	{
		return UTDStatics::ApplyTDDamage(Target, Amount, ETDDamageKind::Energy, Instigator, Instigator);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCoreTestWorldTicks, "TriadDrift.Core.TestWorld.TicksAndBeginsPlay", TD_TEST_FLAGS)
bool FTDCoreTestWorldTicks::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TestTrue(TEXT("world created"), W.IsValid());
	AActor* Actor = SpawnCombatant(W, ETDTeam::Enemy);
	TestNotNull(TEXT("actor spawned"), Actor);
	TestTrue(TEXT("actor has begun play"), Actor && Actor->HasActorBegunPlay());
	W.Advance(1.f);
	TestEqual(TEXT("one second elapsed"), W.GetTimeSeconds(), 1.f, 0.02f);
	TestNotNull(TEXT("event bus exists"), UTDEventBus::Get(W.GetWorld()));
	TestNotNull(TEXT("combat registry exists"), UTDCombatRegistry::Get(W.GetWorld()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCoreShieldAbsorbsFirst, "TriadDrift.Core.Health.ShieldAbsorbsFirst", TD_TEST_FLAGS)
bool FTDCoreShieldAbsorbsFirst::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Target = SpawnCombatant(W, ETDTeam::Enemy);
	UTDHealthComponent* Health = UTDStatics::GetHealth(Target);

	TestEqual(TEXT("30 damage dealt"), Hit(Target, 30.f), 30.f);
	TestEqual(TEXT("shield took it"), Health->GetShield(), 70.f);
	TestEqual(TEXT("hull untouched"), Health->GetHull(), 100.f);

	Hit(Target, 100.f);
	TestEqual(TEXT("shield empty"), Health->GetShield(), 0.f);
	TestEqual(TEXT("overflow reached the hull"), Health->GetHull(), 70.f);

	Hit(Target, 500.f);
	TestTrue(TEXT("dead"), Health->IsDead());
	TestEqual(TEXT("no damage after death"), Hit(Target, 10.f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCoreShieldRegen, "TriadDrift.Core.Health.ShieldRegenAfter3sAt25PercentPerSecond", TD_TEST_FLAGS)
bool FTDCoreShieldRegen::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Target = SpawnCombatant(W, ETDTeam::Enemy);
	UTDHealthComponent* Health = UTDStatics::GetHealth(Target);

	Hit(Target, 80.f); // shield 20
	W.Advance(2.9f);
	TestEqual(TEXT("no regen before 3 s"), Health->GetShield(), 20.f, 0.01f);

	W.Advance(1.1f); // 4.0 s since the hit: 1 s of regen at 25/s
	TestEqual(TEXT("25%/s after the delay"), Health->GetShield(), 45.f, 1.0f);

	W.Advance(10.f);
	TestEqual(TEXT("caps at max"), Health->GetShield(), 100.f, 0.01f);
	TestEqual(TEXT("hull never regenerates by default"), Health->GetHull(), 100.f);

	// Same result at 30 fps (frame-rate independence).
	FTDTestWorld W30;
	AActor* Target30 = SpawnCombatant(W30, ETDTeam::Enemy);
	UTDHealthComponent* Health30 = UTDStatics::GetHealth(Target30);
	Hit(Target30, 80.f);
	W30.Advance(4.f, 1.f / 30.f);
	TestEqual(TEXT("30 fps matches 60 fps"), Health30->GetShield(), 45.f, 1.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCoreInvulnerability, "TriadDrift.Core.Health.InvulnerabilityWindow", TD_TEST_FLAGS)
bool FTDCoreInvulnerability::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Target = SpawnCombatant(W, ETDTeam::Player);
	UTDHealthComponent* Health = UTDStatics::GetHealth(Target);

	Health->SetInvulnerableFor(2.f);
	TestEqual(TEXT("blocked while invulnerable"), Hit(Target, 50.f), 0.f);
	W.Advance(1.9f);
	TestEqual(TEXT("still blocked at 1.9 s"), Hit(Target, 50.f), 0.f);
	W.Advance(0.2f);
	TestEqual(TEXT("vulnerable after 2 s"), Hit(Target, 50.f), 50.f);

	Health->SetInvulnerable(true);
	TestEqual(TEXT("flag blocks"), Hit(Target, 50.f), 0.f);
	Health->SetInvulnerable(false);

	Health->Kill();
	TestTrue(TEXT("Kill ignores everything"), Health->IsDead());
	Health->Revive(1.f, 0.5f);
	TestTrue(TEXT("revived"), Health->IsAlive());
	TestEqual(TEXT("revive shield fraction"), Health->GetShield(), 50.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCoreFriendlyFire, "TriadDrift.Core.Damage.FriendlyFireOff", TD_TEST_FLAGS)
bool FTDCoreFriendlyFire::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* PlayerA = SpawnCombatant(W, ETDTeam::Player);
	AActor* PlayerB = SpawnCombatant(W, ETDTeam::Player, FVector(500, 0, 0));
	AActor* Enemy = SpawnCombatant(W, ETDTeam::Enemy, FVector(1000, 0, 0));

	TestEqual(TEXT("same team: no damage"), Hit(PlayerB, 25.f, PlayerA), 0.f);
	TestEqual(TEXT("hostile: damage"), Hit(Enemy, 25.f, PlayerA), 25.f);
	TestEqual(TEXT("enemy hits player"), Hit(PlayerA, 25.f, Enemy), 25.f);
	TestEqual(TEXT("no instigator: environmental damage applies"), Hit(PlayerA, 5.f, nullptr), 5.f);
	TestTrue(TEXT("AreHostile"), UTDStatics::AreHostile(PlayerA, Enemy));
	TestFalse(TEXT("teammates are not hostile"), UTDStatics::AreHostile(PlayerA, PlayerB));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCoreDamageFilter, "TriadDrift.Core.Damage.FilterAndShieldSuppress", TD_TEST_FLAGS)
bool FTDCoreDamageFilter::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Target = SpawnCombatant(W, ETDTeam::Enemy);
	UTDHealthComponent* Health = UTDStatics::GetHealth(Target);

	// Block everything arriving from the front (+X travelling hits are "from behind" here).
	Health->DamageFilter.BindLambda([](const FTDDamageInfo& Info)
	{
		return Info.HitDirection.X < 0.f ? 0.f : Info.Amount;
	});
	const float Front = UTDStatics::ApplyTDDamage(Target, 40.f, ETDDamageKind::Energy, nullptr, nullptr, FVector::ZeroVector, FVector(-1, 0, 0));
	const float Rear = UTDStatics::ApplyTDDamage(Target, 40.f, ETDDamageKind::Energy, nullptr, nullptr, FVector::ZeroVector, FVector(1, 0, 0));
	TestEqual(TEXT("frontal hit blocked"), Front, 0.f);
	TestEqual(TEXT("rear hit lands"), Rear, 40.f);
	Health->DamageFilter.Unbind();

	Health->SuppressShield(5.f);
	TestEqual(TEXT("shield dropped"), Health->GetShield(), 0.f);
	W.Advance(4.5f);
	TestEqual(TEXT("still suppressed at 4.5 s"), Health->GetShield(), 0.f);
	W.Advance(1.5f);
	TestTrue(TEXT("regenerating after suppression ends"), Health->GetShield() > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCoreRegistry, "TriadDrift.Core.Registry.TeamsAndNearest", TD_TEST_FLAGS)
bool FTDCoreRegistry::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Player = SpawnCombatant(W, ETDTeam::Player);
	AActor* Near = SpawnCombatant(W, ETDTeam::Enemy, FVector(1000, 0, 0));
	AActor* Far = SpawnCombatant(W, ETDTeam::Enemy, FVector(5000, 0, 0));
	UTDCombatRegistry* Registry = UTDCombatRegistry::Get(W.GetWorld());

	TestEqual(TEXT("two enemies alive"), Registry->CountAlive(ETDTeam::Enemy), 2);
	TestEqual(TEXT("one player alive"), Registry->CountAlive(ETDTeam::Player), 1);

	TArray<AActor*> Hostiles;
	Registry->GetHostilesOf(ETDTeam::Player, Hostiles);
	TestEqual(TEXT("player sees two hostiles"), Hostiles.Num(), 2);
	TestTrue(TEXT("nearest is the near one"), Registry->FindNearestHostile(ETDTeam::Player, Player->GetActorLocation()) == Near);
	TestNull(TEXT("range limit respected"), Registry->FindNearestHostile(ETDTeam::Player, Player->GetActorLocation(), 500.f));

	UTDStatics::GetHealth(Near)->SetTargetable(false); // cloak
	TestTrue(TEXT("untargetable skipped"), Registry->FindNearestHostile(ETDTeam::Player, Player->GetActorLocation()) == Far);
	UTDStatics::GetHealth(Near)->SetTargetable(true);

	UTDStatics::GetHealth(Near)->Kill(Player);
	TestEqual(TEXT("dead actors drop out"), Registry->CountAlive(ETDTeam::Enemy), 1);

	const int32 HitCount = UTDStatics::ApplyTDRadialDamage(W.GetWorld(), FVector(5000, 0, 0), 800.f, 50.f,
		ETDDamageKind::Explosive, ETDTeam::Player, Player, Player);
	TestEqual(TEXT("splash hits only the enemy in range"), HitCount, 1);
	TestEqual(TEXT("player untouched by own splash"), UTDStatics::GetHealth(Player)->GetHull01(), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCoreHitStop, "TriadDrift.Core.Feel.HitStopAndSlowMoRestore", TD_TEST_FLAGS)
bool FTDCoreHitStop::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(W.GetWorld());
	TestNotNull(TEXT("feel subsystem"), Feel);
	if (!Feel)
	{
		return false;
	}

	Feel->RequestHitStop(0.04f, 0.05f);
	TestEqual(TEXT("dilated immediately"), Feel->GetCurrentDilation(), 0.05f, 0.001f);
	W.Advance(0.1f);
	TestEqual(TEXT("restored after 40 ms of real time"), Feel->GetCurrentDilation(), 1.f, 0.001f);

	Feel->RequestSlowMo(0.5f, 0.3f);
	Feel->RequestHitStop(0.04f, 0.05f);
	TestEqual(TEXT("slowest wins"), Feel->GetCurrentDilation(), 0.05f, 0.001f);
	W.Advance(0.1f);
	TestEqual(TEXT("falls back to slow-mo"), Feel->GetCurrentDilation(), 0.3f, 0.001f);
	W.Advance(0.6f);
	TestEqual(TEXT("fully restored"), Feel->GetCurrentDilation(), 1.f, 0.001f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
