// Triad Drift - tests for the main menu backdrop. Owner: Teammate 3.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TriadDrift/Director/Menu/TDMenuBackdrop.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace TDMenuBackdropTest
{
	static ATDMenuBackdrop* Find(UWorld* World)
	{
		for (TActorIterator<ATDMenuBackdrop> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	/** In the lens's frame (inside the horizontal field of view and in front). */
	static bool InFrame(const ACameraActor* Cam, const FVector& At)
	{
		const FVector D = (At - Cam->GetActorLocation()).GetSafeNormal();
		const float Half = Cam->GetCameraComponent()->FieldOfView * 0.5f;
		return FVector::DotProduct(D, Cam->GetActorForwardVector()) > FMath::Cos(FMath::DegreesToRadians(Half));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMenuBackdropScene, "TriadDrift.Director.MenuBackdrop.SceneAndFlybys", TD_TEST_FLAGS)
bool FTDMenuBackdropScene::RunTest(const FString& Parameters)
{
	// The real menu game mode spawns it (T5's StartPlay hook): sky, Verdance, the lens, the squad gliding past.
	FTDTestWorld W(nullptr, ATDMenuGameMode::StaticClass());
	APlayerController* PC = W.SpawnLocalPlayerController(); // in the game the player exists first; here a frame later
	W.Tick();
	W.Tick();
	ATDMenuBackdrop* B = TDMenuBackdropTest::Find(W.GetWorld());
	if (!TestNotNull(TEXT("the menu mode spawned the backdrop"), B) || !TestNotNull(TEXT("player"), PC))
	{
		return false;
	}
	TestNotNull(TEXT("Verdance"), B->GetPlanet());
	TestTrue(TEXT("the lens has the view"), B->GetCamera() && PC->GetViewTarget() == B->GetCamera());
	TestTrue(TEXT("Verdance in frame"), TDMenuBackdropTest::InFrame(B->GetCamera(), B->GetPlanet()->GetActorLocation()));
	TestEqual(TEXT("three squad ships"), B->GetSquadMeshes().Num(), 3);
	TestFalse(TEXT("no fly-by yet"), B->IsFlybyActive());

	// The first pass, a few seconds in: every ship visible and inside the frame for most of it.
	W.Advance(B->FirstFlybySeconds + 0.5f, 1.f / 30.f);
	TestTrue(TEXT("first fly-by"), B->IsFlybyActive() && B->GetFlybyCount() == 1);
	int32 InFrameTicks = 0;
	int32 Ticks = 0;
	for (float T = 0.f; T < B->FlybySeconds - 0.6f; T += 1.f / 30.f)
	{
		W.Tick(1.f / 30.f);
		++Ticks;
		bool bAll = true;
		for (const UStaticMeshComponent* C : B->GetSquadMeshes())
		{
			bAll = bAll && C->IsVisible() && TDMenuBackdropTest::InFrame(B->GetCamera(), C->GetComponentLocation());
		}
		InFrameTicks += bAll ? 1 : 0;
	}
	TestTrue(FString::Printf(TEXT("the squad in frame for most of the pass (%d of %d frames)"), InFrameTicks, Ticks), InFrameTicks > Ticks / 2);
	W.Advance(1.5f, 1.f / 30.f);
	TestFalse(TEXT("pass over: hidden"), B->IsFlybyActive() || B->GetSquadMeshes()[0]->IsVisible());
	// Every ~20 s.
	W.Advance(B->FlybyEverySeconds, 1.f / 30.f);
	TestEqual(TEXT("again about 20 s later"), B->GetFlybyCount(), 2);

	// The view stays the scene's: nothing resetting the menu view drops it.
	PC->SetViewTarget(PC);
	W.Tick();
	TestTrue(TEXT("view kept"), PC->GetViewTarget() == B->GetCamera());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMenuBackdropOff, "TriadDrift.Director.MenuBackdrop.OffWhenTheMapHasItsOwnScene", TD_TEST_FLAGS)
bool FTDMenuBackdropOff::RunTest(const FString& Parameters)
{
	FTDTestWorld W(nullptr, ATDMenuGameMode::StaticClass());
	AGameModeBase* Mode = W.GetWorld()->GetAuthGameMode();
	ATDMenuBackdrop* First = TDMenuBackdropTest::Find(W.GetWorld());
	if (!TestNotNull(TEXT("menu mode"), Mode) || !TestNotNull(TEXT("backdrop"), First))
	{
		return false;
	}
	// Nick's menu scene in the map: no backdrop of ours.
	AActor* NickScene = W.Spawn<AActor>(FVector::ZeroVector);
	NickScene->Tags.Add(ATDMenuBackdrop::MenuSceneTag);
	TestNull(TEXT("the map brings its own scene: off"), ATDMenuBackdrop::SpawnFor(Mode));
	NickScene->Destroy();
	// The switch.
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("td.MenuBackdrop"));
	CVar->Set(0, ECVF_SetByCode);
	TestNull(TEXT("td.MenuBackdrop 0: off"), ATDMenuBackdrop::SpawnFor(Mode));
	CVar->Set(1, ECVF_SetByCode);

	// Everything it spawned goes with it.
	const AActor* Planet = First->GetPlanet();
	const AActor* Cam = First->GetCamera();
	First->Destroy();
	W.Tick();
	TestTrue(TEXT("planet and lens gone with the backdrop"), !IsValid(Planet) && !IsValid(Cam));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMenuBackdropVictory, "TriadDrift.Director.MenuBackdrop.VictoryDominionBreaksUp", TD_TEST_FLAGS)
bool FTDMenuBackdropVictory::RunTest(const FString& Parameters)
{
	// After a win the same scene plays the wrap: the Dominion burning and listing beside Verdance, the squad flying away
	// from it past the lens, a slow push-in.
	FTDTestWorld W(nullptr, ATDMenuGameMode::StaticClass());
	APlayerController* PC = W.SpawnLocalPlayerController();
	AGameModeBase* Mode = W.GetWorld()->GetAuthGameMode();
	if (ATDMenuBackdrop* Menu = TDMenuBackdropTest::Find(W.GetWorld()))
	{
		TestNull(TEXT("the menu scene has no Dominion"), Menu->GetDominion());
		Menu->Destroy();
	}
	W.Tick();
	ATDMenuBackdrop* B = ATDMenuBackdrop::SpawnFor(Mode, ETDBackdropScene::Victory);
	W.Tick();
	if (!TestNotNull(TEXT("victory backdrop"), B) || !TestNotNull(TEXT("player"), PC))
	{
		return false;
	}
	AActor* Dominion = B->GetDominion();
	if (!TestNotNull(TEXT("the Dominion"), Dominion))
	{
		return false;
	}
	const ACameraActor* Cam = B->GetCamera();
	TestTrue(TEXT("the lens has the view"), Cam && PC->GetViewTarget() == Cam);
	TestTrue(TEXT("the Dominion in frame"), TDMenuBackdropTest::InFrame(Cam, Dominion->GetActorLocation()));
	TestTrue(TEXT("Verdance in frame"), TDMenuBackdropTest::InFrame(Cam, B->GetPlanet()->GetActorLocation()));
	const FVector Cam0 = Cam->GetActorLocation();
	const FRotator Hull0 = Dominion->GetActorRotation();
	const float Dist0 = FVector::Dist(Cam0, Dominion->GetActorLocation());

	// The squad leaves the wreck and comes home past the lens.
	W.Advance(B->FirstFlybySeconds + 0.2f, 1.f / 30.f);
	TestTrue(TEXT("the squad flies"), B->IsFlybyActive());
	const float Start = FVector::Dist(B->GetSquadMeshes()[0]->GetComponentLocation(), Dominion->GetActorLocation());
	int32 InFrameTicks = 0;
	int32 Ticks = 0;
	for (float T = 0.f; T < B->FlybySeconds - 0.6f; T += 1.f / 30.f)
	{
		W.Tick(1.f / 30.f);
		++Ticks;
		InFrameTicks += TDMenuBackdropTest::InFrame(Cam, B->GetSquadMeshes()[0]->GetComponentLocation()) ? 1 : 0;
	}
	const float End = FVector::Dist(B->GetSquadMeshes()[0]->GetComponentLocation(), Dominion->GetActorLocation());
	TestTrue(FString::Printf(TEXT("away from the Dominion (%.0f -> %.0f)"), Start, End), End > Start);
	TestTrue(FString::Printf(TEXT("in frame for most of it (%d of %d)"), InFrameTicks, Ticks), InFrameTicks > Ticks / 2);

	// Fires along the hull; it lists; the lens pushes in.
	W.Advance(20.f, 1.f / 30.f);
	TestTrue(FString::Printf(TEXT("fires on the hull (%d)"), B->GetFireCount()), B->GetFireCount() >= 30);
	TestTrue(TEXT("the hull lists"), !Dominion->GetActorRotation().Equals(Hull0, 2.f));
	const float Dist1 = FVector::Dist(Cam->GetActorLocation(), Dominion->GetActorLocation());
	TestTrue(FString::Printf(TEXT("pushing in (%.0f -> %.0f)"), Dist0, Dist1), Dist1 < Dist0);
	TestTrue(TEXT("the Dominion still in frame"), TDMenuBackdropTest::InFrame(Cam, Dominion->GetActorLocation()));

	B->Destroy();
	W.Tick();
	TestFalse(TEXT("the Dominion goes with the backdrop"), IsValid(Dominion));
	return true;
}

#endif
