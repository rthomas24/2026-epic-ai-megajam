// ENG-27 balance run: plays waves 1-5 with a scripted pilot + T3's wingmen at each difficulty and writes
// Saved/Balance/balance_<Difficulty>_<time>.csv / .md. Not in the default suite (it takes minutes).
// Run: .\Tools\td-test.ps1 -Filter TDBalance            (all three)   or  -Filter TDBalance.Standard

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TriadDrift/Waves/Balance/TDBalanceRecorder.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Tests/TDTestWorld.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/IConsoleManager.h"

namespace TDBalanceRun
{
	const TCHAR* DifficultyName(ETDDifficulty D)
	{
		return D == ETDDifficulty::Story ? TEXT("Story") : (D == ETDDifficulty::Ace ? TEXT("Ace") : TEXT("Standard"));
	}

	struct FRunResult
	{
		TArray<FTDBalanceWaveStats> Waves;
		float TotalSeconds = 0.f;
	};

	/** Plays every wave; a squad wipe counts as a new take of that wave. */
	FRunResult Play(FAutomationTestBase& Test, ETDDifficulty Difficulty, const FTDPilotSkill& Skill, float Step, float MaxSecondsPerWave, int32 Seed = 0)
	{
		ATDEnemyShip::SpawnSeedSalt() = Seed;
		FRunResult Result;
		const ETDDifficulty Prev = UTDDifficultySettings::GetCurrent();
		UTDDifficultySettings::SetCurrent(Difficulty);

		FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
		UTDBalanceRecorder* Rec = NewObject<UTDBalanceRecorder>();
		Rec->AddToRoot();
		Rec->Begin(W.GetWorld());

		APlayerController* PC = W.SpawnPlayerController();
		ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
		Squad->SetSquadPlayerController(PC);
		Squad->SpawnSquad(FTransform::Identity);
		ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
		if (Seed != 0) { D->RandomSeed = 1337 + 7 * Seed; }
		W.Tick(Step);
		Rec->WatchSquad(Squad);

		FTDBalancePilot Pilot;
		Pilot.Reseed(2026 + 13 * Seed);
		Pilot.Skill = Skill;
		for (int32 Index = 0; Index < D->GetWaveCount(); ++Index)
		{
			Rec->Current = FTDBalanceWaveStats();
			Rec->Current.WaveNumber = Index + 1;
			const int32 WipesBefore = Rec->Wipes;
			D->StartWave(Index);
			float T = 0.f;
			while (T < MaxSecondsPerWave)
			{
				Pilot.Tick(W.GetWorld(), Squad, Step, Rec->Current);
				W.Tick(Step);
				T += Step;
				Rec->WatchSquad(Squad);	// respawned ships
				const ETDWaveState S = D->GetState();
				if (S != ETDWaveState::Active && S != ETDWaveState::WarpingIn)
				{
					Rec->Current.bCleared = true;
					break;
				}
				if (Rec->Wipes > WipesBefore + (Rec->Current.Takes - 1))
				{
					// "CUT!": next take of this wave, squad back at full strength.
					++Rec->Current.Takes;
					Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, Squad->GetActiveShip() ? Squad->GetActiveShip()->GetActorLocation() : FVector::ZeroVector));
					D->RestartCurrentWave();
				}
			}
			Rec->Current.Seconds = T;
			Rec->Current.Enemies = D->GetEnemiesTotal();
			Result.TotalSeconds += T;
			Result.Waves.Add(Rec->Current);
			Test.AddInfo(FString::Printf(TEXT("%s wave %d: %s in %.0f s, takes %d, kills %d/%d, damage %.0f, downs %d"),
				DifficultyName(Difficulty), Index + 1, Rec->Current.bCleared ? TEXT("cleared") : TEXT("TIMED OUT"), T,
				Rec->Current.Takes, Rec->Current.Kills, Rec->Current.Enemies, Rec->Current.DamageTaken, Rec->Current.ShipDowns));
			D->StopAndClear();
			// A breather's worth of repair between waves, as in the real game.
			W.Advance(D->BreatherSeconds, Step);
		}

		Rec->End();
		Rec->RemoveFromRoot();
		ATDEnemyShip::SpawnSeedSalt() = 0;
		UTDDifficultySettings::SetCurrent(Prev);
		return Result;
	}

	void Write(ETDDifficulty Difficulty, const FTDPilotSkill& Skill, const FRunResult& R, float Step)
	{
		const FString Dir = FPaths::ProjectSavedDir() / TEXT("Balance");
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Dir);
		const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
		const FString Base = Dir / FString::Printf(TEXT("balance_%s_%s_%s"), DifficultyName(Difficulty), *Skill.Name.ToString(), *Stamp);

		auto Ship = [](const FTDBalanceWaveStats& S, ETDShipId Id) { return S.KillsByShip.FindRef(Id); };
		auto Kind = [](const FTDBalanceWaveStats& S, ETDDamageKind K) { return S.KillsByKind.FindRef(K); };

		FString Csv = TEXT("difficulty,wave,cleared,seconds,takes,enemies,kills,damage_taken,ship_downs,kills_rift,kills_lancer,kills_bastion,kills_energy,kills_kinetic,kills_explosive,kills_ram\n");
		FString Md = FString::Printf(TEXT("# Balance run: %s, %s pilot (%.0f fps, scripted player + AI wingmen)\n\n| Wave | Cleared | Time (s) | Takes | Kills / enemies | Damage taken | Ship downs | Rift / Lancer / Bastion kills | Bays / carriers killed | Pilot time on bays / ships (s) | Enemy on screen (time / avg count / longest empty / first contact) | Damage by source |\n|---|---|---|---|---|---|---|---|---|---|---|---|\n"),
			DifficultyName(Difficulty), *Skill.Name.ToString(), 1.f / Step);
		for (const FTDBalanceWaveStats& S : R.Waves)
		{
			Csv += FString::Printf(TEXT("%s,%d,%d,%.1f,%d,%d,%d,%.0f,%d,%d,%d,%d,%d,%d,%d,%d\n"), DifficultyName(Difficulty), S.WaveNumber, S.bCleared ? 1 : 0, S.Seconds, S.Takes,
				S.Enemies, S.Kills, S.DamageTaken, S.ShipDowns, Ship(S, ETDShipId::Rift), Ship(S, ETDShipId::Lancer), Ship(S, ETDShipId::Bastion),
				Kind(S, ETDDamageKind::Energy), Kind(S, ETDDamageKind::Kinetic), Kind(S, ETDDamageKind::Explosive), Kind(S, ETDDamageKind::Ram));
			FString Sources;
			TArray<TPair<FName, float>> Sorted;
			for (const TPair<FName, float>& KV : S.DamageBySource) { Sorted.Add(KV); }
			Sorted.Sort([](const TPair<FName, float>& A, const TPair<FName, float>& B) { return A.Value > B.Value; });
			for (int32 i = 0; i < FMath::Min(3, Sorted.Num()); ++i)
			{
				Sources += FString::Printf(TEXT("%s%s %.0f"), i ? TEXT(", ") : TEXT(""), *Sorted[i].Key.ToString(), Sorted[i].Value);
			}
			const float OnScreen = S.SecondsTotal > 0.f ? 100.f * S.SecondsEnemyOnScreen / S.SecondsTotal : 0.f;
			Md += FString::Printf(TEXT("| %d | %s | %.0f | %d | %d / %d | %.0f | %d | %d / %d / %d | %d / %d | %.0f / %.0f | %.0f%% / %.1f / %.0f s / %.0f s | %s |\n"), S.WaveNumber, S.bCleared ? TEXT("yes") : TEXT("**no**"), S.Seconds, S.Takes,
				S.Kills, S.Enemies, S.DamageTaken, S.ShipDowns, Ship(S, ETDShipId::Rift), Ship(S, ETDShipId::Lancer), Ship(S, ETDShipId::Bastion),
				S.BaysKilled, S.CarriersKilled, S.SecondsOnBays, S.SecondsOnShips, OnScreen,
				S.SecondsTotal > 0.f ? S.EnemyOnScreenCountSeconds / S.SecondsTotal : 0.f, S.LongestEmptySeconds, S.FirstContactSeconds, *Sources);
		}
		Md += FString::Printf(TEXT("\n**Space combat total: %.1f min** (waves only; the 20-30 min target also covers both planets, travel and the Dominion).\n"), R.TotalSeconds / 60.f);
		FFileHelper::SaveStringToFile(Csv, *(Base + TEXT(".csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		FFileHelper::SaveStringToFile(Md, *(Base + TEXT(".md")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	bool RunOne(FAutomationTestBase& Test, ETDDifficulty Difficulty, const FTDPilotSkill& Skill, float MaxSecondsPerWave = 300.f)
	{
		const float Step = 1.f / 30.f;
		const FRunResult R = Play(Test, Difficulty, Skill, Step, MaxSecondsPerWave);
		Write(Difficulty, Skill, R, Step);
		Test.TestEqual(TEXT("played all five waves"), R.Waves.Num(), 5);
		for (const FTDBalanceWaveStats& S : R.Waves)
		{
			// Story must be beatable by a dumb bot; harder difficulties report (that's the tuning data).
			if (Difficulty == ETDDifficulty::Story && Skill.Name != TEXT("Weak"))
			{
				Test.TestTrue(*FString::Printf(TEXT("Story wave %d can be cleared by a simple bot"), S.WaveNumber), S.bCleared);
			}
			else if (!S.bCleared)
			{
				Test.AddInfo(FString::Printf(TEXT("%s wave %d not cleared by the bot within the time limit (tuning signal)"), DifficultyName(Difficulty), S.WaveNumber));
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceStoryWeak, "TDBalance.Story.Weak", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceStoryWeak::RunTest(const FString& Parameters) { return TDBalanceRun::RunOne(*this, ETDDifficulty::Story, FTDPilotSkill::Weak()); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceStoryAverage, "TDBalance.Story.Average", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceStoryAverage::RunTest(const FString& Parameters) { return TDBalanceRun::RunOne(*this, ETDDifficulty::Story, FTDPilotSkill::Average()); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceStoryStrong, "TDBalance.Story.Strong", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceStoryStrong::RunTest(const FString& Parameters) { return TDBalanceRun::RunOne(*this, ETDDifficulty::Story, FTDPilotSkill::Strong()); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceStandardWeak, "TDBalance.Standard.Weak", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceStandardWeak::RunTest(const FString& Parameters) { return TDBalanceRun::RunOne(*this, ETDDifficulty::Standard, FTDPilotSkill::Weak()); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceStandardAverage, "TDBalance.Standard.Average", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceStandardAverage::RunTest(const FString& Parameters) { return TDBalanceRun::RunOne(*this, ETDDifficulty::Standard, FTDPilotSkill::Average()); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceStandardStrong, "TDBalance.Standard.Strong", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceStandardStrong::RunTest(const FString& Parameters) { return TDBalanceRun::RunOne(*this, ETDDifficulty::Standard, FTDPilotSkill::Strong()); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceAceWeak, "TDBalance.Ace.Weak", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceAceWeak::RunTest(const FString& Parameters) { return TDBalanceRun::RunOne(*this, ETDDifficulty::Ace, FTDPilotSkill::Weak()); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceAceAverage, "TDBalance.Ace.Average", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceAceAverage::RunTest(const FString& Parameters) { return TDBalanceRun::RunOne(*this, ETDDifficulty::Ace, FTDPilotSkill::Average()); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceAceStrong, "TDBalance.Ace.Strong", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceAceStrong::RunTest(const FString& Parameters) { return TDBalanceRun::RunOne(*this, ETDDifficulty::Ace, FTDPilotSkill::Strong()); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceStandardPassive, "TDBalance.Standard.Passive", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceStandardPassive::RunTest(const FString& Parameters)
{
	// Readability worst case: the player doesn't steer. Waves won't clear (the wingmen fight alone); the
	// numbers that matter are the on-screen columns.
	return TDBalanceRun::RunOne(*this, ETDDifficulty::Standard, FTDPilotSkill::Passive(), /*MaxSecondsPerWave*/ 90.f);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceStandardAverageNoCross, "TDBalance.Baseline.StandardAverageNoCrossInFront", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceStandardAverageNoCross::RunTest(const FString& Parameters)
{
	// Readability baseline: enemies attack without cutting across the player's nose.
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("TD.EnemyCrossInFront"));
	const float Prev = CVar ? CVar->GetFloat() : 5000.f;
	if (CVar) { CVar->Set(0.f); }
	FTDPilotSkill Skill = FTDPilotSkill::Average();
	Skill.Name = TEXT("AverageNoCross");
	const bool bOk = TDBalanceRun::RunOne(*this, ETDDifficulty::Standard, Skill);
	if (CVar) { CVar->Set(Prev); }
	return bOk;
}

// ENG-27 (Lead, Nov 12): is the game able to take a ship off the player? Ten seeded runs of waves 1-5 on
// Standard with the Average pilot; per wave: the share of runs where at least one squad ship went down, the
// share with a squad wipe ("CUT!"), and hull lost per minute by the ship the player was flying.
// Writes Saved/Balance/distribution_Standard_Average_<time>.md. Measurement only: no assertions on the numbers.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceDistribution, "TDBalance.Distribution.StandardAverage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceDistribution::RunTest(const FString& Parameters)
{
	constexpr int32 Runs = 10;
	const float Step = 1.f / 30.f;
	struct FAgg { int32 Runs = 0; int32 WithDown = 0; int32 WithWipe = 0; int32 Cleared = 0; float HullPerMin = 0.f; float Seconds = 0.f; float Downs = 0.f; };
	TArray<FAgg> Agg;
	Agg.SetNum(5);
	for (int32 Seed = 1; Seed <= Runs; ++Seed)
	{
		const TDBalanceRun::FRunResult R = TDBalanceRun::Play(*this, ETDDifficulty::Standard, FTDPilotSkill::Average(), Step, 300.f, Seed);
		for (const FTDBalanceWaveStats& S : R.Waves)
		{
			if (!Agg.IsValidIndex(S.WaveNumber - 1)) { continue; }
			FAgg& A = Agg[S.WaveNumber - 1];
			++A.Runs;
			A.WithDown += S.ShipDowns > 0 ? 1 : 0;
			A.WithWipe += S.Takes > 1 ? 1 : 0;
			A.Cleared += S.bCleared ? 1 : 0;
			A.Downs += S.ShipDowns;
			A.Seconds += S.Seconds;
			A.HullPerMin += S.SecondsTotal > 0.f ? 100.f * S.ActiveHullLost01 / (S.SecondsTotal / 60.f) : 0.f;
		}
	}
	FString Md = FString::Printf(TEXT("# Ship-down distribution: Standard, Average pilot, %d seeded runs\n\n| Wave | Runs with a ship down | Runs with a wipe (CUT) | Avg ship downs | Active ship hull lost / min | Avg time (s) | Cleared |\n|---|---|---|---|---|---|---|\n"), Runs);
	for (int32 i = 0; i < Agg.Num(); ++i)
	{
		const FAgg& A = Agg[i];
		const float N = FMath::Max(1, A.Runs);
		Md += FString::Printf(TEXT("| %d | %d of %d (%.0f%%) | %d of %d (%.0f%%) | %.1f | %.0f%% | %.0f | %d of %d |\n"), i + 1, A.WithDown, A.Runs, 100.f * A.WithDown / N,
			A.WithWipe, A.Runs, 100.f * A.WithWipe / N, A.Downs / N, A.HullPerMin / N, A.Seconds / N, A.Cleared, A.Runs);
	}
	const FString Path = FPaths::ProjectSavedDir() / TEXT("Balance") / FString::Printf(TEXT("distribution_Standard_Average_%s.md"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	FFileHelper::SaveStringToFile(Md, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	AddInfo(FString::Printf(TEXT("Distribution report: %s"), *Path));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
