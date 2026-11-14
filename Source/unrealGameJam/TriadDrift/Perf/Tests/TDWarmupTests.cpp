// Copyright (c) 2026 Triad Drift team. Epic AI MegaJam entry.
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Perf/TDWarmupSubsystem.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWarmupCollectsCues, "TriadDrift.Perf.Warmup.CollectsEveryCueButNotScores", TD_TEST_FLAGS)
bool FTDWarmupCollectsCues::RunTest(const FString& Parameters)
{
	// The warm pass streams what a presentation asset points at: every FX cue (inside structs) is found, the music
	// and ambience slots are not (they stream on their own), and an empty slot adds nothing.
	UTDTravelFX* FX = NewObject<UTDTravelFX>();
	FX->HyperdriveSpool.VFX = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/T7Test/NS_A.NS_A")));
	FX->HyperdriveDropOut.Sound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/T7Test/S_B.S_B")));
	TArray<FSoftObjectPath> Paths;
	UTDWarmupSubsystem::CollectSoftPaths(FX, Paths);
	TestTrue(TEXT("the spool's effect"), Paths.Contains(FSoftObjectPath(TEXT("/Game/T7Test/NS_A.NS_A"))));
	TestTrue(TEXT("the drop-out's sound"), Paths.Contains(FSoftObjectPath(TEXT("/Game/T7Test/S_B.S_B"))));
	TestEqual(TEXT("nothing else (empty slots add nothing)"), Paths.Num(), 2);

	UTDUIStyle* Style = NewObject<UTDUIStyle>();
	Style->IdleSpaceMusic = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/T7Test/MUS_C.MUS_C")));
	Paths.Reset();
	UTDWarmupSubsystem::CollectSoftPaths(Style, Paths);
	TestFalse(TEXT("a music slot is not warmed"), Paths.Contains(FSoftObjectPath(TEXT("/Game/T7Test/MUS_C.MUS_C"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWarmupFindsTheGameData, "TriadDrift.Perf.Warmup.FindsTheGameData", TD_TEST_FLAGS)
bool FTDWarmupFindsTheGameData::RunTest(const FString& Parameters)
{
	// The folders the warm pass reads hold the game's data assets, and between them they name effects to warm.
	IAssetRegistry* Registry = IAssetRegistry::Get();
	if (!TestNotNull(TEXT("asset registry"), Registry)) { return false; }
	int32 DataAssets = 0;
	TArray<FSoftObjectPath> Content;
	for (const FName& Folder : UTDWarmupSubsystem::GetWarmFolders())
	{
		Registry->ScanPathsSynchronous({ Folder.ToString() }, false);
		TArray<FAssetData> Found;
		Registry->GetAssetsByPath(Folder, Found, true);
		for (const FAssetData& Data : Found)
		{
			if (Data.AssetName.ToString().StartsWith(TEXT("DA_")))
			{
				++DataAssets;
				UTDWarmupSubsystem::CollectSoftPaths(Data.GetAsset(), Content);
			}
		}
	}
	int32 Effects = 0;
	for (const FSoftObjectPath& P : Content)
	{
		const FAssetData D = Registry->GetAssetByObjectPath(P);
		Effects += D.IsValid() && D.AssetClassPath.GetAssetName() == TEXT("NiagaraSystem") ? 1 : 0;
	}
	AddInfo(FString::Printf(TEXT("%d data assets, %d referenced assets, %d effects"), DataAssets, Content.Num(), Effects));
	TestTrue(TEXT("data assets found"), DataAssets >= 10);
	TestTrue(TEXT("effects to warm"), Effects >= 5);

	// A test world never warms (each test loads only what it needs).
	FTDTestWorld W;
	UTDWarmupSubsystem* Warm = UTDWarmupSubsystem::Get(W.GetWorld());
	if (TestNotNull(TEXT("subsystem in a game world"), Warm))
	{
		TestEqual(TEXT("idle under automation"), Warm->GetStage(), ETDWarmupStage::Idle);
	}
	return true;
}

#endif
