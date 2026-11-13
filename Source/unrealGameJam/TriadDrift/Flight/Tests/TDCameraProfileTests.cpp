// Issue #23 camera profiles: read the real chase camera component (FOV, rotation, post-process blur).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight.CameraProfile

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Flight/TDFlightFeedbackComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "Camera/CameraComponent.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCameraProfiles, "TriadDrift.Flight.CameraProfile.DefaultTunedAuditionOnTheCamera", TD_TEST_FLAGS)
bool FTDCameraProfiles::RunTest(const FString& Parameters)
{
	using EP = UTDFlightFeedbackComponent::ECamProfile;
	ON_SCOPE_EXIT { UTDFlightFeedbackComponent::CamProfile = EP::Default; };
	TestEqual(TEXT("Default is the default"), (int32)UTDFlightFeedbackComponent::CamProfile, (int32)EP::Default);

	struct FResult { TArray<float> FOVs; float MaxFOV = 0.f; float MaxBlur = 0.f; float TurnRoll = 0.f; float SettledRoll = 0.f; float CapFOV = 0.f; };
	auto Run = [this](EP Profile)
	{
		FResult R;
		UTDFlightFeedbackComponent::CamProfile = Profile;
		FTDTestWorld W;
		ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
		if (!Ship) { return R; }
		Ship->SetPlayerControlled(true);
		UCameraComponent* Cam = Ship->GetChaseCamera();
		R.CapFOV = Ship->GetCameraTuning().MaxFOV;
		FTDFlightInput In;
		In.Throttle = 1.f;
		Ship->SetFlightInput(In);
		W.Advance(2.f);
		// Boost start (the FOV punch) and sustain.
		In.bBoost = true;
		Ship->SetFlightInput(In);
		for (int32 i = 0; i < 120; ++i)
		{
			W.Tick();
			R.MaxFOV = FMath::Max(R.MaxFOV, Cam->FieldOfView);
			R.FOVs.Add(Cam->FieldOfView);
			R.MaxBlur = FMath::Max(R.MaxBlur, Cam->PostProcessSettings.MotionBlurAmount);
		}
		// A hard turn, then release: the roll the camera itself carries (relative to the arm), then settling.
		In.bBoost = false;
		In.Aim = FVector2D(1.f, 0.f);
		Ship->SetFlightInput(In);
		for (int32 i = 0; i < 90; ++i) { W.Tick(); R.TurnRoll = FMath::Max(R.TurnRoll, FMath::Abs(Cam->GetRelativeRotation().Roll)); }
		In.Aim = FVector2D::ZeroVector;
		Ship->SetFlightInput(In);
		W.Advance(2.5f);
		R.SettledRoll = FMath::Abs(Cam->GetRelativeRotation().Roll);
		return R;
	};

	const FResult D = Run(EP::Default);
	const FResult T = Run(EP::Tuned);
	const FResult A = Run(EP::Audition);
	AddInfo(FString::Printf(TEXT("max FOV  Default %.1f  Tuned %.1f  Audition %.1f (speed cap %.0f)"), D.MaxFOV, T.MaxFOV, A.MaxFOV, D.CapFOV));
	AddInfo(FString::Printf(TEXT("max blur Default %.3f  Tuned %.3f  Audition %.3f"), D.MaxBlur, T.MaxBlur, A.MaxBlur));
	AddInfo(FString::Printf(TEXT("turn roll Default %.2f  Tuned %.2f  Audition %.2f; settled %.3f / %.3f / %.3f"), D.TurnRoll, T.TurnRoll, A.TurnRoll, D.SettledRoll, T.SettledRoll, A.SettledRoll));

	TestTrue(TEXT("Default: the camera stays within the speed FOV (as on main)"), D.MaxFOV <= D.CapFOV + 0.1f);
	// Same simulation, frame by frame: the camera's FOV over Default's is the punch that now reaches it.
	float Punch = 0.f;
	for (int32 i = 0; i < FMath::Min(D.FOVs.Num(), T.FOVs.Num()); ++i) { Punch = FMath::Max(Punch, T.FOVs[i] - D.FOVs[i]); }
	AddInfo(FString::Printf(TEXT("boost punch on the camera: Tuned over Default by up to %.2f deg"), Punch));
	TestTrue(TEXT("Tuned: the boost FOV punch reaches the camera (finding 2 fixed)"), Punch > 3.f);
	TestTrue(TEXT("Default and Tuned: no motion blur"), D.MaxBlur < 0.001f && T.MaxBlur < 0.001f);
	TestTrue(TEXT("Audition: boost blur reaches the camera, within its cap (finding 1 fixed)"), A.MaxBlur > 0.01f && A.MaxBlur <= 0.04f + 0.001f);
	TestTrue(TEXT("Audition: the camera tilts into a turn (up to 3 deg)"), A.TurnRoll > 1.f && A.TurnRoll <= 3.f + 0.5f);
	TestTrue(TEXT("Default: no added turn tilt"), D.TurnRoll < 0.5f);
	TestTrue(TEXT("Audition: settles when steering stops (no idle bob)"), A.SettledRoll < 0.15f);
	return true;
}

#endif
