// ENG-27 balance run: plays waves 1-5 with a scripted pilot + T3's wingmen at each difficulty and writes
// Saved/Balance/balance_<Difficulty>_<time>.csv / .md. Not in the default suite (it takes minutes).
// Run: .\Tools\td-test.ps1 -Filter TDBalance            (all three)   or  -Filter TDBalance.Standard

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TriadDrift/Waves/Balance/TDBalanceRecorder.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Tests/TDTestWorld.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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
	FRunResult Play(FAutomationTestBase& Test, ETDDifficulty Difficulty, float Step, float MaxSecondsPerWave)
	{
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
		W.Tick(Step);
		Rec->WatchSquad(Squad);

		FTDBalancePilot Pilot;
		for (int32 Index = 0; Index < D->GetWaveCount(); ++Index)
		{
			Rec->Current = FTDBalanceWaveStats();
			Rec->Current.WaveNumber = Index + 1;
			const int32 WipesBefore = Rec->Wipes;
			D->StartWave(Index);
			float T = 0.f;
			while (T < MaxSecondsPerWave)
			{
				Pilot.Tick(W.GetWorld(), Squad, Step);
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
		UTDDifficultySettings::SetCurrent(Prev);
		return Result;
	}

	void Write(ETDDifficulty Difficulty, const FRunResult& R, float Step)
	{
		const FString Dir = FPaths::ProjectSavedDir() / TEXT("Balance");
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Dir);
		const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
		const FString Base = Dir / FString::Printf(TEXT("balance_%s_%s"), DifficultyName(Difficulty), *Stamp);

		auto Ship = [](const FTDBalanceWaveStats& S, ETDShipId Id) { return S.KillsByShip.FindRef(Id); };
		auto Kind = [](const FTDBalanceWaveStats& S, ETDDamageKind K) { return S.KillsByKind.FindRef(K); };

		FString Csv = TEXT("difficulty,wave,cleared,seconds,takes,enemies,kills,damage_taken,ship_downs,kills_rift,kills_lancer,kills_bastion,kills_energy,kills_kinetic,kills_explosive,kills_ram\n");
		FString Md = FString::Printf(TEXT("# Balance run: %s (%.0f fps, scripted pilot + AI wingmen)\n\n| Wave | Cleared | Time (s) | Takes | Kills / enemies | Damage taken | Ship downs | Rift / Lancer / Bastion kills |\n|---|---|---|---|---|---|---|---|\n"),
			DifficultyName(Difficulty), 1.f / Step);
		for (const FTDBalanceWaveStats& S : R.Waves)
		{
			Csv += FString::Printf(TEXT("%s,%d,%d,%.1f,%d,%d,%d,%.0f,%d,%d,%d,%d,%d,%d,%d,%d\n"), DifficultyName(Difficulty), S.WaveNumber, S.bCleared ? 1 : 0, S.Seconds, S.Takes,
				S.Enemies, S.Kills, S.DamageTaken, S.ShipDowns, Ship(S, ETDShipId::Rift), Ship(S, ETDShipId::Lancer), Ship(S, ETDShipId::Bastion),
				Kind(S, ETDDamageKind::Energy), Kind(S, ETDDamageKind::Kinetic), Kind(S, ETDDamageKind::Explosive), Kind(S, ETDDamageKind::Ram));
			Md += FString::Printf(TEXT("| %d | %s | %.0f | %d | %d / %d | %.0f | %d | %d / %d / %d |\n"), S.WaveNumber, S.bCleared ? TEXT("yes") : TEXT("**no**"), S.Seconds, S.Takes,
				S.Kills, S.Enemies, S.DamageTaken, S.ShipDowns, Ship(S, ETDShipId::Rift), Ship(S, ETDShipId::Lancer), Ship(S, ETDShipId::Bastion));
		}
		Md += FString::Printf(TEXT("\n**Space combat total: %.1f min** (waves only; the 20-30 min target also covers both planets, travel and the Dominion).\n"), R.TotalSeconds / 60.f);
		FFileHelper::SaveStringToFile(Csv, *(Base + TEXT(".csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		FFileHelper::SaveStringToFile(Md, *(Base + TEXT(".md")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	bool RunOne(FAutomationTestBase& Test, ETDDifficulty Difficulty)
	{
		const float Step = 1.f / 30.f;
		const FRunResult R = Play(Test, Difficulty, Step, /*MaxSecondsPerWave*/ 300.f);
		Write(Difficulty, R, Step);
		Test.TestEqual(TEXT("played all five waves"), R.Waves.Num(), 5);
		for (const FTDBalanceWaveStats& S : R.Waves)
		{
			// Story must be beatable by a dumb bot; harder difficulties report (that's the tuning data).
			if (Difficulty == ETDDifficulty::Story)
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceStory, "TDBalance.Story", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceStory::RunTest(const FString& Parameters) { return TDBalanceRun::RunOne(*this, ETDDifficulty::Story); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceStandard, "TDBalance.Standard", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceStandard::RunTest(const FString& Parameters) { return TDBalanceRun::RunOne(*this, ETDDifficulty::Standard); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDBalanceAce, "TDBalance.Ace", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDBalanceAce::RunTest(const FString& Parameters) { return TDBalanceRun::RunOne(*this, ETDDifficulty::Ace); }

#endif // WITH_DEV_AUTOMATION_TESTS
