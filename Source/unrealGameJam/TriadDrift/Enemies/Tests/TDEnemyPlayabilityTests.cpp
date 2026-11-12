// Playability tests from the review (M1 leash / fresh spawn points, M2 ramming / wrecks).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Enemies.Play

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

#include "Components/SphereComponent.h"

namespace
{
	ATDSpacecraft* TDEnemyPlay_SpawnPlayerShip(FTDTestWorld& W, const FVector& Loc, float Speed)
	{
		ATDSpacecraft* S = W.Spawn<ATDSpacecraft>(Loc);
		S->SetTeam(ETDTeam::Player);
		if (UTDHealthComponent* H = S->GetHealth())
		{
			H->DefaultTeam = ETDTeam::Player;
			H->InitMaxValues(100000.f, 0.f, true);	// survive the whole test
		}
		W.SpawnPlayerController(S);
		S->SetSpeedOverride(Speed, 0.f);	// fly straight
		return S;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPlaySpawnFresh, "TriadDrift.Enemies.Play.SpawnRelativeToMovingPlayer", TD_TEST_FLAGS)
bool FTDPlaySpawnFresh::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Anchor = W.Spawn<AActor>();
	USceneComponent* Root = NewObject<USceneComponent>(Anchor, TEXT("Root"));
	Anchor->SetRootComponent(Root);
	Root->RegisterComponent();

	ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
	D->SpawnAnchorOverride = Anchor;
	D->SetSpawnFactory([](UWorld* World, FName, const FTransform& T) -> AActor*
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<ATDWaveTestEnemy>(ATDWaveTestEnemy::StaticClass(), T, P);
	});
	FTDWaveDef Wave;
	Wave.SceneId = TEXT("Wave1");
	FTDWaveGroup Late;
	Late.EnemyId = TDEnemyIds::MaulBomber;
	Late.Zone = ETDSpawnZone::Behind;
	Late.Delay = 6.f;
	Wave.Groups.Add(Late);
	D->SetWaveTable({ Wave });
	D->StartWave(0);

	// The player cruises 60 000 cm away during the delay.
	for (int32 i = 0; i < 60 * 8; ++i)
	{
		Anchor->SetActorLocation(Anchor->GetActorLocation() + FVector(7500.f / 60.f, 0.f, 0.f));
		W.Tick();
	}
	TArray<AActor*> Living = D->GetLivingEnemies();
	if (!TestEqual(TEXT("delayed bomber spawned"), Living.Num(), 1)) { return false; }
	const float Dist = FVector::Dist(Living[0]->GetActorLocation(), Anchor->GetActorLocation());
	TestTrue(*FString::Printf(TEXT("spawned near where the player is now (%.0f cm)"), Dist), Dist < D->SpawnDistance + D->SpawnJitter + 7500.f * D->WarpInSeconds + 4.f * D->FormationSpacing);	// this anchor reports no velocity, so no lead
	TestTrue(TEXT("spawned behind the player"), Living[0]->GetActorLocation().X < Anchor->GetActorLocation().X);

	// No wave in the default table spawns ahead of the player.
	for (const FTDWaveDef& Def : ATDWaveDirector::MakeDefaultWaves())
	{
		for (const FTDWaveGroup& G : Def.Groups)
		{
			TestTrue(*FString::Printf(TEXT("%s %s not Ahead"), *Def.SceneId.ToString(), *G.EnemyId.ToString()), G.Zone != ETDSpawnZone::Ahead);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPlayFlee, "TriadDrift.Enemies.Play.WaveEngagesAFleeingPlayer", TD_TEST_FLAGS)
bool FTDPlayFlee::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	// A cruising Lancer (7500 cm/s) that flies straight away for 30 s.
	ATDSpacecraft* Player = TDEnemyPlay_SpawnPlayerShip(W, FVector::ZeroVector, 7500.f);
	ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
	D->StartWave(0);

	float MinDist = TNumericLimits<float>::Max();
	bool bAnyCatchingUp = false;
	for (int32 i = 0; i < 30 * 30; ++i)	// 30 s at 30 fps
	{
		W.Tick(1.f / 30.f);
		for (AActor* E : D->GetLivingEnemies())
		{
			MinDist = FMath::Min(MinDist, FVector::Dist(E->GetActorLocation(), Player->GetActorLocation()));
			if (const ATDEnemyAIController* AI = Cast<ATDEnemyAIController>(Cast<APawn>(E)->GetController()))
			{
				bAnyCatchingUp |= AI->IsCatchingUp();
			}
		}
	}
	const float GunRange = UTDEnemyLibrary::Get(TDEnemyIds::ShardFighter).Guns.Range;
	TestTrue(TEXT("stragglers use catch-up speed"), bAnyCatchingUp);
	TestTrue(*FString::Printf(TEXT("fighters reach gun range of a fleeing Lancer (closest %.0f, guns %.0f)"), MinDist, GunRange), MinDist < GunRange);
	TestTrue(TEXT("and actually hit it"), Player->GetHealth()->GetHull() < 100000.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPlayNoRam, "TriadDrift.Enemies.Play.BreaksOffBeforeImpact", TD_TEST_FLAGS)
bool FTDPlayNoRam::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	// Head-on: the player flies at the fighter at cruise, the fighter attacks.
	ATDSpacecraft* Player = TDEnemyPlay_SpawnPlayerShip(W, FVector(15000.f, 0.f, 0.f), 6000.f);
	Player->SetActorRotation(FRotator(0.f, 180.f, 0.f));
	ATDEnemyShip* F = W.Spawn<ATDEnemyShip>(FVector::ZeroVector);
	F->InitFromSpec(TDEnemyIds::ShardFighter);
	ATDEnemyAIController* AI = Cast<ATDEnemyAIController>(F->GetController());
	if (!TestNotNull(TEXT("AI"), AI)) { return false; }

	float MinDist = TNumericLimits<float>::Max();
	float DistAtBreak = -1.f;
	bool bSawRun = false;
	for (int32 i = 0; i < 120 * 4; ++i)
	{
		W.Tick(1.f / 120.f);
		const float Dist = FVector::Dist(F->GetActorLocation(), Player->GetActorLocation());
		MinDist = FMath::Min(MinDist, Dist);
		bSawRun |= AI->GetAIState() == ETDEnemyAIState::AttackRun;
		if (bSawRun && DistAtBreak < 0.f && AI->GetAIState() == ETDEnemyAIState::BreakOff)
		{
			DistAtBreak = Dist;
		}
	}
	TestTrue(TEXT("made an attack run"), bSawRun);
	TestTrue(*FString::Printf(TEXT("broke off with room to spare (%.0f cm)"), DistAtBreak), DistAtBreak > 4000.f);
	TestTrue(*FString::Printf(TEXT("never collided (closest %.0f cm)"), MinDist), MinDist > 1000.f);
	TestEqual(TEXT("player took no collision damage"), Player->GetHealth()->GetHull(), 100000.f, 200.f);	// a few pulse hits are fine
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPlayWreck, "TriadDrift.Enemies.Play.WrecksAreNotSolid", TD_TEST_FLAGS)
bool FTDPlayWreck::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDEnemyShip* G = W.Spawn<ATDEnemyShip>(FVector::ZeroVector);
	G->InitFromSpec(TDEnemyIds::WallGunship);
	TestTrue(TEXT("alive ships block ships"), G->GetCollisionSphere()->GetCollisionResponseToChannel(TDCollision::Ship) == ECR_Block);
	G->GetHealth()->Kill();
	TestTrue(TEXT("dying"), G->IsDying());
	TestTrue(TEXT("the tumbling wreck ignores ships"), G->GetCollisionSphere()->GetCollisionResponseToChannel(TDCollision::Ship) == ECR_Ignore);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
