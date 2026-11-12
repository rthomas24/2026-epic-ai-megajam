// ENG-27 difficulty presets. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Waves.Difficulty

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDDifficultyTest
{
	struct FScopedDifficulty
	{
		ETDDifficulty Prev;
		explicit FScopedDifficulty(ETDDifficulty D) : Prev(UTDDifficultySettings::GetCurrent()) { UTDDifficultySettings::SetCurrent(D); }
		~FScopedDifficulty() { UTDDifficultySettings::SetCurrent(Prev); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDifficultyScales, "TriadDrift.Waves.Difficulty.ScalesEnemiesAndWaves", TD_TEST_FLAGS)
bool FTDDifficultyScales::RunTest(const FString& Parameters)
{
	using namespace TDDifficultyTest;
	TestTrue(TEXT("Standard is the features.md baseline"), UTDDifficultySettings::GetScaleFor(ETDDifficulty::Standard).EnemyHull == 1.f
		&& UTDDifficultySettings::GetScaleFor(ETDDifficulty::Standard).WaveSize == 1.f);
	TestTrue(TEXT("Story is easier, Ace harder"), UTDDifficultySettings::GetScaleFor(ETDDifficulty::Story).EnemyDamage < 1.f
		&& UTDDifficultySettings::GetScaleFor(ETDDifficulty::Ace).EnemyDamage > 1.f);

	for (const ETDDifficulty Diff : { ETDDifficulty::Story, ETDDifficulty::Standard, ETDDifficulty::Ace })
	{
		FScopedDifficulty Scope(Diff);
		const FTDDifficultyScale S = UTDDifficultySettings::GetScale();
		FTDTestWorld W;

		ATDEnemyShip* G = W.Spawn<ATDEnemyShip>();
		G->InitFromSpec(TDEnemyIds::WallGunship);
		const FTDEnemySpec& Base = UTDEnemyLibrary::Get(TDEnemyIds::WallGunship);
		TestEqual(TEXT("gunship hull scaled"), G->GetHealth()->MaxHull, Base.Hull * S.EnemyHull, 0.01f);
		TestEqual(TEXT("frontal shield scaled"), G->GetSpec().FrontalShield, Base.FrontalShield * S.EnemyHull, 0.01f);
		TestEqual(TEXT("aim error scaled"), G->GetSpec().AimErrorDeg, Base.AimErrorDeg * S.EnemyAimError, 0.001f);

		ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
		D->SetSpawnFactory([](UWorld* World, FName, const FTransform& T) -> AActor*
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			return World->SpawnActor<ATDWaveTestEnemy>(ATDWaveTestEnemy::StaticClass(), T, P);
		});
		D->StartWave(0);	// 2 x 4 fighters at Standard
		const int32 Expected = 2 * UTDDifficultySettings::ScaleCount(4, S.WaveSize);
		TestEqual(*FString::Printf(TEXT("wave 1 size at difficulty %d"), (int32)Diff), D->GetEnemiesTotal(), Expected);
		W.Advance(D->WarpInSeconds + 2.5f);
		TestEqual(TEXT("and that many really spawn"), D->GetEnemiesAlive(), Expected);
		D->StartWave(4);	// carriers stay 2 at every difficulty
		int32 Carriers = 0;
		for (const FTDWaveGroup& Gr : D->GetWaveDef(4)->Groups) { if (Gr.EnemyId == TDEnemyIds::HiveCarrier) { Carriers += Gr.Total(); } }
		TestEqual(TEXT("big ships aren't multiplied"), Carriers, 2);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
