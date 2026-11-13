// Director camera (C-03 #2): the view really cuts, returns, and never sticks. Real producers only.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Director.Camera

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Director/TDDirectorCamera.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/World/TDWorldSubsystem.h"

namespace TDDirectorCameraTests
{
	static const float GSteps[] = { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f };

	/** Sets td.DirectorCam for the scope, then restores "use the setting". */
	struct FScopedCamMode
	{
		explicit FScopedCamMode(int32 Mode) { Set(Mode); }
		~FScopedCamMode() { Set(-1); }
		static void Set(int32 Mode)
		{
			if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("td.DirectorCam")))
			{
				V->Set(Mode, ECVF_SetByCode);
			}
		}
	};

	struct FCamFixture
	{
		FTDTestWorld W;
		APlayerController* PC = nullptr;
		ATDSquadManager* Squad = nullptr;
		UTDDirectorSubsystem* Director = nullptr;

		explicit FCamFixture(TSubclassOf<UGameInstance> GIClass = nullptr, ETDShipId Start = ETDShipId::Lancer)
			: W(GIClass)
		{
			FScopedCamMode::Set(2);
			PC = W.SpawnPlayerController();
			Squad = W.Spawn<ATDSquadManager>();
			Squad->SetSquadPlayerController(PC);
			Squad->StartingShip = Start;
			Squad->SpawnSquad(FTransform::Identity);
			Director = UTDDirectorSubsystem::Get(W.GetWorld());
			W.Tick();
		}
		~FCamFixture() { FScopedCamMode::Set(-1); }

		UTDDirectorCameraRig* Rig() const { return Director ? Director->GetCameraRig() : nullptr; }
		ATDShip* Player() const { return Squad->GetActiveShip(); }
		bool ViewIsPawn() const { return PC->GetViewTarget() == PC->GetPawn(); }
		bool ViewIsDirector() const { return Rig() && Rig()->GetCameraActor() && PC->GetViewTarget() == Rig()->GetCameraActor(); }

		/** The real producer: the player's Lancer rams a real, parked Shard Fighter. */
		ATDEnemyShip* RamKill(float Step = 1.f / 60.f)
		{
			ATDShip* Lancer = Squad->GetShip(ETDShipId::Lancer);
			ATDEnemyShip* Fighter = TDPowerTest::SpawnEnemyShip(W, TDEnemyIds::ShardFighter,
				Lancer->GetActorLocation() + Lancer->GetActorForwardVector() * 9000.f);
			Fighter->SetFlightParked(true);
			UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true);
			W.Tick(Step);
			UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
			for (int32 i = 0; i < FMath::CeilToInt(3.f / Step); ++i)
			{
				W.Tick(Step);
				if (UTDStatics::GetHealth(Fighter)->IsDead())
				{
					W.Tick(Step);
					break;
				}
			}
			return Fighter;
		}

		/** Advance in real time until the rig is idle (or MaxSeconds). */
		void RunUntilIdle(float MaxSeconds, float Step = 1.f / 60.f)
		{
			for (int32 i = 0; i < FMath::CeilToInt(MaxSeconds / Step) && Rig() && Rig()->IsActive(); ++i)
			{
				W.Tick(Step);
			}
		}
	};
}

using namespace TDDirectorCameraTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorCamRam, "TriadDrift.Director.Camera.RamKillCutsAndReturns", TD_TEST_FLAGS)
bool FTDDirectorCamRam::RunTest(const FString& Parameters)
{
	for (const float Step : GSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FCamFixture F;
		TestTrue(TEXT("starts on the ship") + Fps, F.ViewIsPawn());
		F.RamKill(Step);
		TestTrue(TEXT("a ram kill takes the view") + Fps, F.ViewIsDirector());
		TestTrue(TEXT("rig active") + Fps, F.Rig()->IsActive());
		// Blend in (~0.12 s real), then hold: the camera is somewhere else, never inside the ship.
		for (int32 i = 0; i < FMath::CeilToInt(0.25f / Step); ++i)
		{
			F.W.Tick(Step);
		}
		const ATDDirectorCameraActor* Cam = F.Rig()->GetCameraActor();
		TestTrue(TEXT("holding the shot") + Fps, F.Rig()->GetPhase() == ETDDirectorCameraPhase::Hold);
		TestTrue(TEXT("camera away from the player's ship") + Fps, FVector::Dist(Cam->GetActorLocation(), F.Player()->GetActorLocation()) > 900.f);
		TestTrue(TEXT("camera is not the chase camera") + Fps,
			FVector::Dist(Cam->GetActorLocation(), F.Player()->GetChaseCameraTransformNow().GetLocation()) > 500.f);
		// A tracking shot: the camera keeps its distance to the (fast) subject instead of being left behind.
		const AActor* Subject = F.Director->GetCurrentShot().Subject.Get();
		const float D0 = Subject ? FVector::Dist(Cam->GetActorLocation(), Subject->GetActorLocation()) : 0.f;
		for (int32 i = 0; i < FMath::CeilToInt(0.3f / Step); ++i)
		{
			F.W.Tick(Step);
		}
		const float D1 = Subject ? FVector::Dist(Cam->GetActorLocation(), Subject->GetActorLocation()) : 0.f;
		TestTrue(FString::Printf(TEXT("camera tracks the subject (%.0f -> %.0f cm)"), D0, D1) + Fps, Subject && D1 < D0 * 1.5f + 500.f && D1 > D0 * 0.5f);
		// The player is still flying: the movement keeps integrating under the shot.
		const FVector Before = F.Player()->GetActorLocation();
		F.W.Tick(Step);
		TestTrue(TEXT("player keeps flying during the shot") + Fps, !F.Player()->GetActorLocation().Equals(Before, 1.f));
		// Shot length (<= 1 s real) + blend out (0.2 s): back on the ship, exactly on its chase camera.
		F.RunUntilIdle(2.f, Step);
		TestFalse(TEXT("rig idle") + Fps, F.Rig()->IsActive());
		TestTrue(TEXT("view back on the ship") + Fps, F.ViewIsPawn());
		TestTrue(TEXT("returned onto the live chase camera (no pop)") + Fps,
			FVector::Dist(Cam->GetActorLocation(), F.Player()->GetChaseCameraTransformNow().GetLocation()) < 400.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorCamWingmen, "TriadDrift.Director.Camera.WingmenStayOutOfTheShot", TD_TEST_FLAGS)
bool FTDDirectorCamWingmen::RunTest(const FString& Parameters)
{
	// Live capture: in a ram-kill shot the two wingmen filled the foreground, because the tracking
	// camera pulled in in front of whichever wingman crossed its line. Now the camera keeps its
	// distance and the wingmen keep clear of the lens.
	for (const float Step : GSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FCamFixture F;
		F.W.Advance(6.f, Step); // wingmen settled in the forward wedge
		F.RamKill(Step);
		if (!TestTrue(TEXT("shot running") + Fps, F.Rig()->IsActive()))
		{
			continue;
		}
		const ATDDirectorCameraActor* Cam = F.Rig()->GetCameraActor();
		const float D0 = FVector::Dist(Cam->GetActorLocation(), F.Rig()->GetLastFocus());
		float MinWing = BIG_NUMBER, MinFocus = BIG_NUMBER;
		int32 Frames = 0;
		for (int32 i = 0; i < FMath::CeilToInt(2.f / Step) && F.Rig()->IsActive(); ++i)
		{
			F.W.Tick(Step);
			if (F.Rig()->GetPhase() != ETDDirectorCameraPhase::Hold)
			{
				continue;
			}
			++Frames;
			MinFocus = FMath::Min(MinFocus, (float)FVector::Dist(Cam->GetActorLocation(), F.Rig()->GetLastFocus()));
			for (const ETDShipId Id : { ETDShipId::Rift, ETDShipId::Bastion })
			{
				MinWing = FMath::Min(MinWing, (float)FVector::Dist(Cam->GetActorLocation(), F.Squad->GetShip(Id)->GetActorLocation()));
			}
		}
		TestTrue(TEXT("held the shot") + Fps, Frames > 0);
		TestTrue(FString::Printf(TEXT("no wingman in the lens (closest %.0f cm)"), MinWing) + Fps, MinWing > 1200.f);
		TestTrue(FString::Printf(TEXT("camera never yanked in toward the subject (%.0f of %.0f cm)"), MinFocus, D0) + Fps, MinFocus > D0 * 0.6f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorCamCarrier, "TriadDrift.Director.Camera.CarrierAndGeneratorKills", TD_TEST_FLAGS)
bool FTDDirectorCamCarrier::RunTest(const FString& Parameters)
{
	{
		FCamFixture F;
		ATDEnemyShip* Carrier = TDPowerTest::SpawnEnemyShip(F.W, TDEnemyIds::HiveCarrier, FVector(40000.f, 0.f, 0.f));
		F.W.Advance(0.5f);
		UTDStatics::ApplyTDDamage(Carrier, 1000000.f, ETDDamageKind::Explosive, F.Player(), F.Player());
		F.W.Tick();
		TestTrue(TEXT("carrier kill takes the view"), F.ViewIsDirector());
		F.RunUntilIdle(3.f);
		TestTrue(TEXT("and gives it back"), F.ViewIsPawn() && !F.Rig()->IsActive());
	}
	{
		FCamFixture F(UTDGameInstance::StaticClass());
		Cast<UTDGameInstance>(F.W.GetGameInstance())->SetDominionState(true, false);
		ATDDominion* D = F.W.Spawn<ATDDominion>(FVector(150000.f, 0.f, 0.f));
		F.W.Advance(D->ProgressPollSeconds + D->WarpInSeconds + 0.3f);
		F.RunUntilIdle(3.f);
		ATDShieldGenerator* G = D->GetGenerators().Num() > 0 ? D->GetGenerators()[0] : nullptr;
		if (!TestNotNull(TEXT("real generator"), G))
		{
			return false;
		}
		UTDStatics::ApplyTDDamage(G, 100000.f, ETDDamageKind::Explosive, F.Player(), F.Player());
		F.W.Tick();
		TestTrue(TEXT("generator kill takes the view"), F.ViewIsDirector());
		F.RunUntilIdle(3.f);
		TestTrue(TEXT("and gives it back"), F.ViewIsPawn() && !F.Rig()->IsActive());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorCamPortal, "TriadDrift.Director.Camera.PortalMissileReturn", TD_TEST_FLAGS)
bool FTDDirectorCamPortal::RunTest(const FString& Parameters)
{
	FCamFixture F(nullptr, ETDShipId::Rift);
	ATDShip* Rift = F.Player();
	Rift->SetFlightParked(true);
	UTDPortalBlasterPower* Portal = Cast<UTDPortalBlasterPower>(UTDPowerComponent::FindPower(Rift, ETDPowerInput::Signature1));
	if (!TestNotNull(TEXT("portal blaster"), Portal))
	{
		return false;
	}
	Portal->HandleInputPressed();
	Rift->SetActorRotation(FRotator(0.f, 90.f, 0.f));
	Portal->HandleInputPressed();
	Rift->SetActorRotation(FRotator::ZeroRotator);
	F.W.Tick();
	AActor* Interceptor = TDPowerTest::SpawnBody(F.W, ETDTeam::Enemy, FVector(20000.f, 0.f, 0.f), FVector(150.f), 100.f, 100.f);
	TDPowerTest::LaunchMissile(F.W, FVector(15000.f, 0.f, 0.f), -FVector::ForwardVector, Interceptor, Rift);
	const float At = TDPowerTest::AdvanceUntil(F.W, [&] { return F.Rig()->IsActive(); }, 3.f);
	TestTrue(TEXT("missile return takes the view"), At > 0.f && F.ViewIsDirector());
	F.RunUntilIdle(3.f);
	TestTrue(TEXT("and gives it back"), F.ViewIsPawn() && !F.Rig()->IsActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorCamSubjectGone, "TriadDrift.Director.Camera.SubjectDiesMidShotHoldsThenBlends", TD_TEST_FLAGS)
bool FTDDirectorCamSubjectGone::RunTest(const FString& Parameters)
{
	// Review minor 7: a missile return frames the enemy shooter; its death is the payoff, so the view
	// holds where it died and blends out at the end of the shot, never snapping back.
	FCamFixture F(nullptr, ETDShipId::Rift);
	ATDShip* Rift = F.Player();
	Rift->SetFlightParked(true);
	UTDPortalBlasterPower* Portal = Cast<UTDPortalBlasterPower>(UTDPowerComponent::FindPower(Rift, ETDPowerInput::Signature1));
	if (!TestNotNull(TEXT("portal blaster"), Portal))
	{
		return false;
	}
	Portal->HandleInputPressed();
	Rift->SetActorRotation(FRotator(0.f, 90.f, 0.f));
	Portal->HandleInputPressed();
	Rift->SetActorRotation(FRotator::ZeroRotator);
	F.W.Tick();
	AActor* Interceptor = TDPowerTest::SpawnBody(F.W, ETDTeam::Enemy, FVector(20000.f, 0.f, 0.f), FVector(150.f), 100.f, 100.f);
	TDPowerTest::LaunchMissile(F.W, FVector(15000.f, 0.f, 0.f), -FVector::ForwardVector, Interceptor, Rift);
	TDPowerTest::AdvanceUntil(F.W, [&] { return F.Rig()->IsActive(); }, 3.f);
	if (!TestTrue(TEXT("shot running on the shooter"), F.Rig()->IsActive() && F.Director->GetCurrentShot().Subject.Get() == Interceptor))
	{
		return false;
	}
	const FVector Cam0 = F.Rig()->GetCameraActor()->GetActorLocation();
	UTDStatics::GetHealth(Interceptor)->Kill();
	F.W.Tick();
	TestTrue(TEXT("still in the shot after the subject died"), F.ViewIsDirector() && F.Rig()->IsActive());
	TestTrue(TEXT("no jump on the death frame"), FVector::Dist(F.Rig()->GetCameraActor()->GetActorLocation(), Cam0) < 1500.f);
	bool bSawBlendOut = false;
	for (int32 i = 0; i < 300 && F.Rig()->IsActive(); ++i)
	{
		F.W.Tick();
		bSawBlendOut |= F.Rig()->GetPhase() == ETDDirectorCameraPhase::BlendOut;
	}
	TestTrue(TEXT("ended with a blend out, not a snap"), bSawBlendOut);
	TestTrue(TEXT("view back on the ship"), F.ViewIsPawn());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorCamViewTaken, "TriadDrift.Director.Camera.ViewTakenOverLetsGo", TD_TEST_FLAGS)
bool FTDDirectorCamViewTaken::RunTest(const FString& Parameters)
{
	// Someone else (a cinematic, travel, a menu) takes the view mid-shot: the rig lets go and never
	// pulls the view back to the pawn.
	FCamFixture F;
	F.RamKill();
	TestTrue(TEXT("shot running"), F.ViewIsDirector() && F.Rig()->IsActive());
	AActor* Other = F.W.Spawn<AActor>(FVector(0.f, 50000.f, 0.f));
	F.PC->SetViewTarget(Other);
	F.W.Tick();
	TestFalse(TEXT("rig idle"), F.Rig()->IsActive());
	TestTrue(TEXT("right reason"), F.Rig()->GetLastCancel() == ETDDirectorCameraCancel::ViewTakenOver);
	F.W.Advance(1.5f);
	TestTrue(TEXT("the other view is left alone"), F.PC->GetViewTarget() == Other);
	return true;
}

namespace TDDirectorFraming
{
	/** Checks the running shot's frame: the focus is well inside the picture, the camera is far enough
	 *  for the subject's size, and the player's ship is not in the way (unless it IS the subject). */
	static void CheckFrame(FAutomationTestBase& T, FCamFixture& F, const FString& Kind)
	{
		for (int32 i = 0; i < 60 && F.Rig()->IsActive() && F.Rig()->GetPhase() != ETDDirectorCameraPhase::Hold; ++i)
		{
			F.W.Tick(1.f / 60.f);
		}
		if (!T.TestTrue(Kind + TEXT(": a shot is holding"), F.Rig()->IsActive() && F.Rig()->GetPhase() == ETDDirectorCameraPhase::Hold))
		{
			return;
		}
		F.W.Tick(1.f / 60.f);
		const ATDDirectorCameraActor* Cam = F.Rig()->GetCameraActor();
		const FVector C = Cam->GetActorLocation();
		const FVector Focus = F.Rig()->GetLastFocus();
		const FVector L = Cam->GetActorTransform().InverseTransformPositionNoScale(Focus);
		const float TanH = FMath::Tan(FMath::DegreesToRadians(Cam->GetCameraComponent()->FieldOfView * 0.5f));
		const FVector2D P(L.Y / (L.X * TanH), L.Z / (L.X * TanH / (16.f / 9.f)));
		T.TestTrue(FString::Printf(TEXT("%s: the focus is in front of the lens and inside the middle 70%% (%.2f, %.2f)"), *Kind, P.X, P.Y),
			L.X > 0.f && FMath::Abs(P.X) < 0.7f && FMath::Abs(P.Y) < 0.7f);
		const AActor* Subject = F.Director->GetCurrentShot().Subject.Get();
		if (IsValid(Subject))
		{
			FVector O, E;
			Subject->GetActorBounds(true, O, E);
			const float Dist = FVector::Dist(C, Focus);
			T.TestTrue(FString::Printf(TEXT("%s: camera far enough for the subject (%.0f cm vs radius %.0f)"), *Kind, Dist, E.Size()), Dist >= E.Size() * 2.f);
		}
		const ATDShip* Player = F.Player();
		if (Player && Subject != Player)
		{
			const FVector Closest = FMath::ClosestPointOnSegment(Player->GetActorLocation(), C, Focus);
			T.TestTrue(FString::Printf(TEXT("%s: the player's ship is not in the way"), *Kind),
				FVector::Dist(Closest, Player->GetActorLocation()) > 400.f || FVector::Dist(Closest, Focus) < 400.f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorCamFraming, "TriadDrift.Director.Camera.EveryKindFramesItsSubject", TD_TEST_FLAGS)
bool FTDDirectorCamFraming::RunTest(const FString& Parameters)
{
	// Lead item 1: each money shot frames what it's about, at a distance its size needs.
	using namespace TDDirectorFraming;
	{
		FCamFixture F;
		F.RamKill();
		CheckFrame(*this, F, TEXT("ram kill"));
	}
	{
		FCamFixture F;
		ATDShip* P = F.Player();
		ATDEnemyShip* Carrier = TDPowerTest::SpawnEnemyShip(F.W, TDEnemyIds::HiveCarrier, P->GetActorLocation() + P->GetActorForwardVector() * 18000.f);
		Carrier->SetFlightParked(true);
		F.W.Advance(0.3f);
		UTDStatics::ApplyTDDamage(Carrier, 10000000.f, ETDDamageKind::Explosive, P, P);
		F.W.Tick();
		CheckFrame(*this, F, TEXT("carrier kill"));
	}
	{
		FCamFixture F;
		ATDShip* P = F.Player();
		FVector Centroid = FVector::ZeroVector;
		for (int32 i = 0; i < 3; ++i)
		{
			const FVector At = P->GetActorLocation() + P->GetActorForwardVector() * (6000.f + 1500.f * i) + P->GetActorRightVector() * (i - 1) * 1500.f;
			Centroid += At / 3.f;
			ATDEnemyShip* E = TDPowerTest::SpawnEnemyShip(F.W, TDEnemyIds::ShardFighter, At);
			E->SetFlightParked(true);
			UTDStatics::ApplyTDDamage(E, 1000000.f, ETDDamageKind::Energy, P, P);
		}
		CheckFrame(*this, F, TEXT("multi-kill"));
		// Its own framing (Lead): on the cluster of kills, pulled back, not a chase view of the killer.
		if (!TestTrue(TEXT("multi-kill: a shot is running"), F.Rig()->IsActive() && F.Rig()->GetCameraActor() != nullptr))
		{
			return false;
		}
		TestTrue(FString::Printf(TEXT("multi-kill: holds on the cluster of kills (%.0f cm off)"), FVector::Dist(F.Rig()->GetLastFocus(), Centroid)), FVector::Dist(F.Rig()->GetLastFocus(), Centroid) < 800.f);
		TestTrue(TEXT("multi-kill: pulled well back"), FVector::Dist(F.Rig()->GetCameraActor()->GetActorLocation(), Centroid) > 5000.f);
		const FVector BehindPlayer = F.Player()->GetChaseCameraTransformNow().GetLocation();
		TestTrue(TEXT("multi-kill: not the chase camera"), FVector::Dist(F.Rig()->GetCameraActor()->GetActorLocation(), BehindPlayer) > 2000.f);
	}
	{
		FCamFixture F;
		ATDShip* P = F.Player();
		UTDEventBus::Get(F.W.GetWorld())->BroadcastPowerUsed(F.Squad->GetActiveShipId(), TEXT("Test"));
		F.W.Advance(0.5f);
		TestTrue(TEXT("switch"), F.Squad->RequestSwitchTo(ETDShipId::Bastion));
		TestTrue(TEXT("Switch Strike armed"), F.Squad->IsSwitchStrikeActive());
		ATDShip* B = F.Player();
		ATDEnemyShip* E = TDPowerTest::SpawnEnemyShip(F.W, TDEnemyIds::ShardFighter, B->GetActorLocation() + B->GetActorForwardVector() * 7000.f);
		E->SetFlightParked(true);
		UTDStatics::ApplyTDDamage(E, 1000000.f, ETDDamageKind::Energy, B, B);
		F.W.Tick();
		CheckFrame(*this, F, TEXT("Switch Strike kill"));
		(void)P;
	}
	{
		FCamFixture F(UTDGameInstance::StaticClass());
		Cast<UTDGameInstance>(F.W.GetGameInstance())->SetDominionState(true, false);
		ATDDominion* D = F.W.Spawn<ATDDominion>(FVector(150000.f, 0.f, 0.f));
		F.W.Advance(D->ProgressPollSeconds + D->WarpInSeconds + 0.3f);
		F.RunUntilIdle(3.f);
		ATDShieldGenerator* G = D->GetGenerators().Num() > 0 ? D->GetGenerators()[0] : nullptr;
		if (TestNotNull(TEXT("generator"), G))
		{
			UTDStatics::ApplyTDDamage(G, 100000.f, ETDDamageKind::Explosive, F.Player(), F.Player());
			F.W.Tick();
			CheckFrame(*this, F, TEXT("generator kill"));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorCamCancels, "TriadDrift.Director.Camera.EveryCancelPathReturnsTheView", TD_TEST_FLAGS)
bool FTDDirectorCamCancels::RunTest(const FString& Parameters)
{
	for (const float Step : GSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		auto Check = [&](FCamFixture& F, ETDDirectorCameraCancel Expected, const TCHAR* Name)
		{
			F.W.Tick(Step);
			TestFalse(FString(Name) + TEXT(": rig idle") + Fps, F.Rig()->IsActive());
			TestTrue(FString(Name) + TEXT(": view back on the pawn") + Fps, F.ViewIsPawn());
			TestTrue(FString(Name) + TEXT(": right reason") + Fps, F.Rig()->GetLastCancel() == Expected);
			F.W.Advance(1.5f, Step);
			TestTrue(FString(Name) + TEXT(": nothing stuck afterwards") + Fps, F.ViewIsPawn() && !F.Rig()->IsActive());
		};

		{	// "CUT!"
			FCamFixture F;
			F.RamKill(Step);
			TestTrue(TEXT("shot running") + Fps, F.ViewIsDirector());
			UTDEventBus::Get(F.W.GetWorld())->BroadcastSceneCut(TEXT("Wave1"), 2, 1.5f);
			Check(F, ETDDirectorCameraCancel::SceneCut, TEXT("CUT"));
		}
		{	// ship switch (auto-switch on death goes the same way)
			FCamFixture F;
			F.RamKill(Step);
			F.W.Advance(0.5f, Step);
			// Review minor 9: assert the shot is still running, so the switch path can't be skipped silently.
			TestTrue(TEXT("still in the shot before the switch") + Fps, F.ViewIsDirector() && F.Rig()->IsActive());
			TestTrue(TEXT("switch") + Fps, F.Squad->RequestSwitchTo(ETDShipId::Rift));
			Check(F, ETDDirectorCameraCancel::ShipSwitch, TEXT("switch"));
			TestTrue(TEXT("view on the NEW ship") + Fps, F.PC->GetViewTarget() == F.Squad->GetShip(ETDShipId::Rift));
		}
		{	// player's ship dies mid-shot
			FCamFixture F;
			F.RamKill(Step);
			UTDStatics::GetHealth(F.Player())->Kill();
			Check(F, ETDDirectorCameraCancel::PlayerDown, TEXT("player down"));
		}
		{	// accessibility switch flipped mid-shot
			FCamFixture F;
			F.RamKill(Step);
			FScopedCamMode::Set(0);
			Check(F, ETDDirectorCameraCancel::Disabled, TEXT("disabled"));
		}
	}

	{	// travel
		FCamFixture F(UTDGameInstance::StaticClass());
		UTDFlowSettings* Settings = GetMutableDefault<UTDFlowSettings>();
		const TMap<ETDDestination, FSoftObjectPath> SavedMaps = Settings->DestinationMaps;
		const FSoftObjectPath SavedTransition = Settings->TransitionMap;
		ON_SCOPE_EXIT { Settings->DestinationMaps = SavedMaps; Settings->TransitionMap = SavedTransition; };
		Settings->DestinationMaps.Add(ETDDestination::Verdance, FSoftObjectPath(TEXT("/Engine/Maps/Entry.Entry")));
		Settings->TransitionMap = FSoftObjectPath(TEXT("/Engine/Maps/Entry.Entry"));
		UTDGameInstance* GI = Cast<UTDGameInstance>(F.W.GetGameInstance());
		GI->OpenLevelOverride = [](const FString&) {};
		F.RamKill();
		TestTrue(TEXT("shot running"), F.ViewIsDirector());
		TestTrue(TEXT("travel starts"), GI->TravelTo(ETDDestination::Verdance, NAME_None));
		F.W.Tick();
		TestTrue(TEXT("travel: view back on the pawn"), F.ViewIsPawn() && !F.Rig()->IsActive());
		TestTrue(TEXT("travel: right reason"), F.Rig()->GetLastCancel() == ETDDirectorCameraCancel::Travel);
	}

	{	// a bigger shot takes over mid-shot: still active, re-placed, then returns cleanly
		FCamFixture F;
		F.RamKill();
		const FVector First = F.Rig()->GetShotLocation();
		AActor* Big = TDPowerTest::SpawnBody(F.W, ETDTeam::Enemy, FVector(30000.f, 20000.f, 0.f), FVector(500.f), 100.f, 0.f);
		TestTrue(TEXT("reactor-tier shot accepted mid-shot"), F.Director->ReportHighlight(ETDHighlightKind::ReactorKill, Big, 1.f, F.Player()));
		F.W.Tick();
		TestTrue(TEXT("still the director's view"), F.ViewIsDirector());
		TestFalse(TEXT("re-placed for the new subject"), F.Rig()->GetShotLocation().Equals(First, 10.f));
		F.RunUntilIdle(3.f);
		TestTrue(TEXT("returns cleanly"), F.ViewIsPawn() && !F.Rig()->IsActive());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorCamGeometry, "TriadDrift.Director.Camera.NeverInsideGeometry", TD_TEST_FLAGS)
bool FTDDirectorCamGeometry::RunTest(const FString& Parameters)
{
	FCamFixture F;
	const FVector Focus(20000.f, 0.f, 0.f);
	const FVector Preferred = Focus + FVector(0.f, 4000.f, 1000.f);

	// A wall on the preferred side, between the focus and the preferred spot.
	AActor* Wall = F.W.Spawn<AActor>(Focus + FVector(0.f, 2000.f, 0.f));
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall, TEXT("Box"));
	Box->SetBoxExtent(FVector(3000.f, 200.f, 3000.f));
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionResponseToAllChannels(ECR_Block);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Wall->SetRootComponent(Box);
	Box->RegisterComponent();
	Wall->SetActorLocation(Focus + FVector(0.f, 2000.f, 0.f));

	FVector Out;
	TArray<const AActor*> Ignore;
	TestTrue(TEXT("found a spot"), UTDDirectorCameraRig::ResolveCameraLocation(F.W.GetWorld(), Focus, Preferred, Ignore, F.Player(), Out));
	TestTrue(TEXT("used the OTHER side of the wall"), (Out - Focus).Y < 0.f);
	TestTrue(TEXT("full distance, not pulled into the wall"), FVector::Dist(Out, Focus) > 3500.f);

	// T4's obstacle registry: an asteroid sitting on the preferred spot.
	Wall->Destroy();
	F.W.Tick();
	F.W.GetWorld()->GetSubsystem<UTDWorldSubsystem>()->AddObstacle(Preferred, 1500.f);
	TestTrue(TEXT("found a spot"), UTDDirectorCameraRig::ResolveCameraLocation(F.W.GetWorld(), Focus, Preferred, Ignore, F.Player(), Out));
	TestFalse(TEXT("not inside the asteroid"), FVector::Dist(Out, Preferred) < 1500.f);

	// Never inside / against the player's own ship.
	TestTrue(TEXT("found a spot"), UTDDirectorCameraRig::ResolveCameraLocation(F.W.GetWorld(), F.Player()->GetActorLocation() + FVector(5000.f, 0, 0),
		F.Player()->GetActorLocation() + FVector(0.f, 0.f, 200.f), Ignore, F.Player(), Out));
	TestTrue(TEXT("at least 900 cm from the player's ship"), FVector::Dist(Out, F.Player()->GetActorLocation()) >= 900.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorCamModes, "TriadDrift.Director.Camera.ReducedAndOffModes", TD_TEST_FLAGS)
bool FTDDirectorCamModes::RunTest(const FString& Parameters)
{
	{
		FCamFixture F;
		FScopedCamMode::Set(1);
		F.RamKill();
		TestTrue(TEXT("reduced: the view never leaves the ship"), F.ViewIsPawn() && !F.Rig()->IsActive());
		TestTrue(TEXT("reduced: slow-mo still"), UTDFeelSubsystem::Get(F.W.GetWorld())->IsSlowMoActive());
		const UTDDirectorFOVPunch* Punch = F.Rig()->GetFOVPunch();
		TestTrue(TEXT("reduced: FOV punch running"), Punch && Punch->GetCurrentPunch() > 0.f);
		F.W.Advance(0.5f);
		TestEqual(TEXT("reduced: punch over in real time"), Punch ? Punch->GetCurrentPunch() : -1.f, 0.f);
	}
	{
		FCamFixture F;
		FScopedCamMode::Set(0);
		F.RamKill();
		TestTrue(TEXT("off: no takeover"), F.ViewIsPawn() && !F.Rig()->IsActive());
		TestNull(TEXT("off: no FOV punch"), F.Rig()->GetFOVPunch());
		TestTrue(TEXT("off: the money shot still happened (slow-mo + event)"), UTDFeelSubsystem::Get(F.W.GetWorld())->IsSlowMoActive());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
