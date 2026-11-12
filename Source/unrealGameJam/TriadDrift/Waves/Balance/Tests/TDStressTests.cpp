// Task B stress test: wave-5 population (40 enemy ships of every type) plus the revealed Dominion with
// its turrets and generators, the squad and wingmen, the default galaxy (asteroids, gates, planets),
// all shooting. Measures game-thread time per World->Tick headless, then attributes it by switching off
// one group of tick functions at a time (ablation).
// Writes Saved/Balance/stress_<time>.md. Not in the default suite (about a minute).
// Run: .\Tools\td-test.ps1 -Filter TDPerf

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/TDWingmanAIController.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"

#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace TDStressRun
{
	struct FWindow
	{
		FString Label;
		double AvgMs = 0, P95Ms = 0, MaxMs = 0;
		float AvgProjectiles = 0;
		int32 MaxProjectiles = 0;
		int32 Ships = 0;
	};

	void MakeInvulnerable(AActor* A)
	{
		if (UTDHealthComponent* H = A ? A->FindComponentByClass<UTDHealthComponent>() : nullptr)
		{
			H->SetInvulnerable(true);
		}
	}

	/** Every actor of Class plus its components: tick on or off. */
	void SetGroupTick(UWorld* World, UClass* Class, bool bOn, UClass* OnlyComponent = nullptr, bool bActorToo = true)
	{
		for (TActorIterator<AActor> It(World, Class); It; ++It)
		{
			if (bActorToo) { It->SetActorTickEnabled(bOn); }
			for (UActorComponent* C : It->GetComponents())
			{
				if (C && C->PrimaryComponentTick.bCanEverTick && (!OnlyComponent || C->IsA(OnlyComponent))) { C->SetComponentTickEnabled(bOn); }
			}
		}
	}

	/** Keeps at least MinLive projectiles in flight: enemy bolts from random enemy ships at the squad
	 *  (the real fight only sustains ~15, because shots resolve fast; the task asks for 100+). */
	void TopUp(UWorld* World, int32 MinLive, FRandomStream& Rng)
	{
		UTDProjectileSubsystem* Proj = UTDProjectileSubsystem::Get(World);
		if (!Proj || Proj->GetLiveCount() >= MinLive) { return; }
		TArray<AActor*> Shooters, Targets;
		for (TActorIterator<ATDEnemyShip> It(World); It; ++It) { Shooters.Add(*It); }
		for (TActorIterator<ATDShip> It(World); It; ++It) { Targets.Add(*It); }
		if (Shooters.Num() == 0 || Targets.Num() == 0) { return; }
		FTDProjectileSpec Spec;
		Spec.Speed = 9000.f;
		Spec.Range = 20000.f;
		Spec.Damage = 1.f;
		for (int32 n = Proj->GetLiveCount(); n < MinLive; ++n)
		{
			AActor* From = Shooters[Rng.RandHelper(Shooters.Num())];
			AActor* To = Targets[Rng.RandHelper(Targets.Num())];
			const FVector Dir = ((To->GetActorLocation() + Rng.GetUnitVector() * 3000.f) - From->GetActorLocation()).GetSafeNormal();
			ATDProjectile::Launch(World, Spec, FTransform(Dir.Rotation(), From->GetActorLocation() + Dir * 600.f), From, ETDTeam::Enemy,
				nullptr, FVector::ZeroVector, 1.f, false);
		}
	}

	FWindow Measure(FTDTestWorld& W, const FString& Label, int32 Frames, int32 MinLive, FRandomStream& Rng)
	{
		TArray<double> Ms;
		Ms.Reserve(Frames);
		double ProjSum = 0;
		FWindow Out;
		Out.Label = Label;
		UTDProjectileSubsystem* Proj = UTDProjectileSubsystem::Get(W.GetWorld());
		for (int32 i = 0; i < Frames; ++i)
		{
			TopUp(W.GetWorld(), MinLive, Rng);	// outside the timed region
			const double T0 = FPlatformTime::Seconds();
			W.Tick(1.f / 60.f);
			Ms.Add((FPlatformTime::Seconds() - T0) * 1000.0);
			const int32 Live = Proj ? Proj->GetLiveCount() : 0;
			ProjSum += Live;
			Out.MaxProjectiles = FMath::Max(Out.MaxProjectiles, Live);
		}
		double Sum = 0;
		for (double V : Ms) { Sum += V; Out.MaxMs = FMath::Max(Out.MaxMs, V); }
		Ms.Sort();
		Out.AvgMs = Sum / Frames;
		Out.P95Ms = Ms[FMath::Clamp(FMath::FloorToInt(Frames * 0.95), 0, Frames - 1)];
		Out.AvgProjectiles = ProjSum / Frames;
		for (TActorIterator<ATDSpacecraft> It(W.GetWorld()); It; ++It) { ++Out.Ships; }
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPerfStress, "TDPerf.Stress.Wave5PlusDominion", TD_TEST_FLAGS)
bool FTDPerfStress::RunTest(const FString& Parameters)
{
	using namespace TDStressRun;
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	UWorld* World = W.GetWorld();
	if (UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance()))
	{
		GI->Progress.bDominionRevealed = true; GI->Progress.Phase = ETDGamePhase::DominionAssault;
	}
	ATDGalaxyLayout* Galaxy = ATDGalaxyLayout::SpawnDefaultGalaxy(World);
	ATDDominion* Dom = Galaxy ? Galaxy->GetDominion() : nullptr;
	if (!TestNotNull(TEXT("galaxy with a Dominion"), Dom)) { return false; }
	Dom->WarpIn();	// no-op if the layout already brought it in
	W.Advance(Dom->WarpInSeconds + 0.5f);

	// The squad 15 000 cm off the Dominion's bow, facing it: inside turret range.
	const FVector SquadAt = Dom->GetActorLocation() + Dom->GetActorForwardVector() * 15000.f + FVector(0, 0, 3000.f);
	APlayerController* PC = W.SpawnPlayerController();
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform((Dom->GetActorLocation() - SquadAt).Rotation(), SquadAt));
	W.Tick();

	// 40 enemies (wave-5 mix: 16 fighters, 8 interceptors, 6 gunships, 6 bombers, 4 carriers), on a shell around the squad.
	const TArray<TPair<FName, int32>> Mix = {
		{ TDEnemyIds::ShardFighter, 16 }, { TDEnemyIds::HookInterceptor, 8 }, { TDEnemyIds::WallGunship, 6 },
		{ TDEnemyIds::MaulBomber, 6 }, { TDEnemyIds::HiveCarrier, 4 } };
	FRandomStream Rng(5);
	int32 Spawned = 0;
	for (const TPair<FName, int32>& Entry : Mix)
	{
		for (int32 i = 0; i < Entry.Value; ++i)
		{
			const FVector Loc = SquadAt + Rng.GetUnitVector() * Rng.FRandRange(6000.f, 16000.f);
			ATDEnemyShip* E = W.Spawn<ATDEnemyShip>(Loc, (SquadAt - Loc).Rotation());
			if (!E) { continue; }
			E->InitFromSpec(Entry.Key);
			++Spawned;
		}
	}
	TestEqual(TEXT("40 enemies spawned"), Spawned, 40);

	// Nobody dies, so the load stays constant for the whole measurement (carrier launches add more).
	for (TActorIterator<AActor> It(World); It; ++It) { MakeInvulnerable(*It); }
	// Warm up until the carriers have launched up to their alive cap, so the load is steady.
	W.Advance(15.f);
	for (TActorIterator<AActor> It(World); It; ++It) { MakeInvulnerable(*It); }	// launched fighters too

	constexpr int32 Frames = 300;
	TArray<FWindow> Windows;
	Windows.Add(Measure(W, TEXT("Everything"), Frames, 0, Rng));
	for (TActorIterator<AActor> It(World); It; ++It) { MakeInvulnerable(*It); }
	Windows.Add(Measure(W, TEXT("Everything + 120 projectiles"), Frames, 120, Rng));
	TestTrue(*FString::Printf(TEXT("100+ projectiles in flight (avg %.0f)"), Windows.Last().AvgProjectiles), Windows.Last().AvgProjectiles >= 100.f);
	const double LoadedMs = Windows.Last().AvgMs;

	struct FGroup { const TCHAR* Name; UClass* Class; UClass* OnlyComponent = nullptr; bool bActorToo = true; };
	const FGroup Groups[] = {
		{ TEXT("Enemy ship movement only"), ATDEnemyShip::StaticClass(), UTDShipMovementComponent::StaticClass(), false },
		{ TEXT("Enemy ship weapons only"), ATDEnemyShip::StaticClass(), UTDWeaponComponent::StaticClass(), false },
		{ TEXT("Enemy ship actor tick only"), ATDEnemyShip::StaticClass(), UTDHealthComponent::StaticClass() /*not ticking: actor only*/, true },
		{ TEXT("Enemy AI controllers"), ATDEnemyAIController::StaticClass() },
		{ TEXT("Enemy ships (actor + components: flight, weapons)"), ATDEnemyShip::StaticClass() },
		{ TEXT("Enemy turrets"), ATDEnemyTurret::StaticClass() },
		{ TEXT("Projectiles"), ATDProjectile::StaticClass() },
		{ TEXT("Wingman AI controllers"), ATDWingmanAIController::StaticClass() },
		{ TEXT("Squad ships"), ATDShip::StaticClass() },
		{ TEXT("Dominion + generators + reactor"), ATDDominion::StaticClass() },
		{ TEXT("Galaxy layout"), ATDGalaxyLayout::StaticClass() },
	};
	for (const FGroup& G : Groups)
	{
		SetGroupTick(World, G.Class, false, G.OnlyComponent, G.bActorToo);
		if (G.Class == ATDDominion::StaticClass())
		{
			SetGroupTick(World, ATDShieldGenerator::StaticClass(), false);
			SetGroupTick(World, ATDDominionReactor::StaticClass(), false);
		}
		Windows.Add(Measure(W, FString::Printf(TEXT("without %s"), G.Name), Frames / 2, 120, Rng));
		SetGroupTick(World, G.Class, true, G.OnlyComponent, G.bActorToo);
		if (G.Class == ATDDominion::StaticClass())
		{
			SetGroupTick(World, ATDShieldGenerator::StaticClass(), true);
			SetGroupTick(World, ATDDominionReactor::StaticClass(), true);
		}
		W.Advance(0.5f);	// settle
	}

	TestTrue(*FString::Printf(TEXT("loaded game-thread tick under 16.6 ms headless (avg %.2f)"), LoadedMs), LoadedMs < 16.6);
	FString Md = TEXT("# Stress: wave-5 population + Dominion (headless, 60 Hz steps)\n\n");
	Md += TEXT("| Window | avg ms | p95 ms | max ms | saved vs loaded (avg ms) | avg / max projectiles | spacecraft |\n|---|---|---|---|---|---|---|\n");
	for (const FWindow& Win : Windows)
	{
		Md += FString::Printf(TEXT("| %s | %.2f | %.2f | %.2f | %.2f | %.0f / %d | %d |\n"), *Win.Label, Win.AvgMs, Win.P95Ms, Win.MaxMs,
			LoadedMs - Win.AvgMs, Win.AvgProjectiles, Win.MaxProjectiles, Win.Ships);
	}
	const FString Path = FPaths::ProjectSavedDir() / TEXT("Balance") / FString::Printf(TEXT("stress_%s.md"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	FFileHelper::SaveStringToFile(Md, *Path);
	AddInfo(FString::Printf(TEXT("Stress report: %s"), *Path));
	UE_LOG(LogTemp, Display, TEXT("%s"), *Md);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
