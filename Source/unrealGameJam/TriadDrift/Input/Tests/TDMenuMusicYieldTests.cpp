// The menu music steps aside while another score holds the exclusive score (Nick's victory score on the menu map, N-053).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.Music.MenuYields

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Input/TDMusicDirectorSubsystem.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1MenuMusicYields, "TriadDrift.Input.Music.MenuYieldsToAnExclusiveScore", TD_TEST_FLAGS)
bool FTDT1MenuMusicYields::RunTest(const FString& Parameters)
{
	USoundBase* TestSound = LoadObject<USoundBase>(nullptr, TEXT("/Engine/EngineSounds/WhiteNoise.WhiteNoise"));
	if (!TestSound) { TestSound = NewObject<USoundWave>(GetTransientPackage()); }
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDUIStyle* Style = NewObject<UTDUIStyle>(GetTransientPackage());
	Style->MenuMusic = TestSound;
	ATDMenuGameMode* Menu = W.GetWorld()->SpawnActorDeferred<ATDMenuGameMode>(ATDMenuGameMode::StaticClass(), FTransform::Identity);
	Menu->StyleOverride = Style;
	Menu->FinishSpawning(FTransform::Identity);
	Menu->StartPlay();
	UTDMusicDirectorSubsystem* Director = UTDMusicDirectorSubsystem::GetMusicDirector(W.GetWorld());
	if (!TestNotNull(TEXT("menu music"), Menu->GetMusic()) || !TestNotNull(TEXT("director"), Director)) { return false; }
	W.Advance(0.2f, 1.f / 30.f);
	TestFalse(TEXT("no claim: the menu music plays on"), Menu->IsMusicYielded());

	Director->RequestExclusiveScore(TEXT("NickVictoryScore"));
	W.Advance(0.2f, 1.f / 30.f);
	TestTrue(TEXT("a claim: the menu music steps aside"), Menu->IsMusicYielded());
	const int32 Loops = Menu->GetMusicLoops();
	W.Advance(2.f, 1.f / 30.f);
	TestTrue(TEXT("still aside while held"), Menu->IsMusicYielded());
	TestEqual(TEXT("no loop restart while held"), Menu->GetMusicLoops(), Loops);

	Director->ReleaseExclusiveScore(TEXT("NickVictoryScore"), 0.5f);
	W.Advance(0.2f, 1.f / 30.f);
	TestTrue(TEXT("held through the claim's own fade"), Menu->IsMusicYielded());
	W.Advance(1.f, 1.f / 30.f);
	TestFalse(TEXT("released: the menu music comes back"), Menu->IsMusicYielded());
	if (GEngine && GEngine->GetMainAudioDevice())
	{
		TestTrue(TEXT("playing again (audio device present)"), Menu->GetMusic()->IsPlaying());
	}
	return true;
}

#endif
