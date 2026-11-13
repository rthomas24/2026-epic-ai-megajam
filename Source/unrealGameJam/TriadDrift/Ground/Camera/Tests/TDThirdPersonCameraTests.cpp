// Triad Drift - tests for the on-foot third-person camera (ENG-32). Owner: Teammate 3.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TriadDrift/Ground/Camera/TDThirdPersonCameraComponent.h"
#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Director/TDDirectorCamera.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/IConsoleManager.h"
#include "Engine/OverlapResult.h"

namespace TDThirdPersonCamTest
{
	struct FScopedDirectorMode
	{
		explicit FScopedDirectorMode(int32 Mode) { Set(Mode); }
		~FScopedDirectorMode() { Set(-1); }
		static void Set(int32 Mode)
		{
			if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("td.DirectorCam")))
			{
				V->Set(Mode, ECVF_SetByCode);
			}
		}
	};

	/** A pilot on a floor, possessed by a local player, with the rig added the way T5 will (attached to the capsule). */
	struct FPilot
	{
		FTDTestWorld& W;
		ATDFirstPersonCharacter* Pilot = nullptr;
		APlayerController* PC = nullptr;
		UTDThirdPersonCameraComponent* Rig = nullptr;

		explicit FPilot(FTDTestWorld& InW, const FVector& At = FVector(0.f, 0.f, 95.f), bool bFloor = true) : W(InW)
		{
			if (bFloor)
			{
				TDGroundTest::SpawnFloor(W);
			}
			Pilot = W.Spawn<ATDFirstPersonCharacter>(At);
			PC = Pilot ? W.SpawnLocalPlayerController(Pilot) : nullptr;
			if (Pilot)
			{
				Rig = Pilot->GetThirdPersonRig(); // T5 wired it into the pilot (ENG-32): the real one, on the capsule
			}
			if (PC)
			{
				PC->SetControlRotation(FRotator::ZeroRotator);
			}
			W.Advance(0.3f);
		}

		bool Ok() const { return Pilot && PC && Rig && Rig->GetThirdPersonCamera(); }

		/** What the pawn's own view is (the active camera component), as the camera manager sees it. */
		FVector ViewLocation() const
		{
			FMinimalViewInfo View;
			Pilot->CalcCamera(0.f, View);
			return View.Location;
		}

		FVector Lens() const { return Rig->GetThirdPersonCamera()->GetComponentLocation(); }

		bool LensClear(FString& Why) const
		{
			UWorld* World = W.GetWorld();
			FCollisionQueryParams Q(SCENE_QUERY_STAT(TDThirdPersonTest), false, Pilot);
			const FCollisionShape Dot = FCollisionShape::MakeSphere(4.f);
			FCollisionObjectQueryParams Pawns;
			Pawns.AddObjectTypesToQuery(ECC_Pawn);
			// Level geometry on the camera channel. Pawns are checked by their capsules below: a headless test never
			// poses an unrendered skeletal mesh, so a crewman's mesh bodies can sit where he spawned, metres away.
			TArray<FOverlapResult> Overlaps;
			World->OverlapMultiByChannel(Overlaps, Lens(), FQuat::Identity, ECC_Camera, Dot, Q);
			Overlaps.RemoveAll([](const FOverlapResult& O) { return !O.bBlockingHit || !O.GetComponent() || Cast<APawn>(O.GetActor()); });
			if (Overlaps.Num())
			{
				const UPrimitiveComponent* In = Overlaps[0].GetComponent();
				Why = FString::Printf(TEXT("lens inside level geometry (%s.%s, %s, extent %s, at %s, lens %s, pivot %s, arm %.0f)"),
					*GetNameSafe(In ? In->GetOwner() : nullptr), *GetNameSafe(In), In ? *In->GetCollisionProfileName().ToString() : TEXT("-"),
					In ? *In->Bounds.BoxExtent.ToCompactString() : TEXT("-"), In ? *In->GetComponentLocation().ToCompactString() : TEXT("-"),
					*Lens().ToCompactString(), *Rig->GetComponentLocation().ToCompactString(), Rig->GetCurrentArmLength());
				return false;
			}
			TArray<FOverlapResult> InPawns;
			World->OverlapMultiByObjectType(InPawns, Lens(), FQuat::Identity, Pawns, Dot, Q);
			InPawns.RemoveAll([](const FOverlapResult& O) { return !Cast<UCapsuleComponent>(O.GetComponent()); });
			if (InPawns.Num())
			{
				Why = FString::Printf(TEXT("lens inside a pawn (%s)"), *GetNameSafe(InPawns[0].GetActor()));
				return false;
			}
			// A clear line through the level (a crewman in between may hide the pilot for a moment; that is allowed).
			TArray<FHitResult> Hits;
			World->LineTraceMultiByChannel(Hits, Rig->GetComponentLocation(), Lens(), ECC_Camera, Q);
			for (const FHitResult& Hit : Hits)
			{
				if (Hit.bBlockingHit && !Cast<APawn>(Hit.GetActor()))
				{
					Why = FString::Printf(TEXT("no line from the pilot to the lens (%s)"), *GetNameSafe(Hit.GetActor()));
					return false;
				}
			}
			return true;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDThirdPersonFacing, "TriadDrift.Ground.ThirdPerson.FacingRule", TD_TEST_FLAGS)
bool FTDThirdPersonFacing::RunTest(const FString& Parameters)
{
	using C = UTDThirdPersonCameraComponent;
	// Just running: face the movement, wherever the camera looks.
	TestNearlyEqual(TEXT("run +Y faces +Y"), C::GetDesiredActorYaw(FVector(0.f, 1.f, 0.f), 0.f, false, 0.f), 90.f, 0.01f);
	TestNearlyEqual(TEXT("run back faces back"), C::GetDesiredActorYaw(FVector(-1.f, 0.f, 0.f), 0.f, false, 0.f), 180.f, 0.01f);
	// Attacking / blocking / aiming a deflect: face the camera (strafe), even while running sideways.
	TestNearlyEqual(TEXT("combat while running: camera yaw"), C::GetDesiredActorYaw(FVector(0.f, 1.f, 0.f), 30.f, true, 90.f), 30.f, 0.01f);
	TestNearlyEqual(TEXT("combat standing: camera yaw"), C::GetDesiredActorYaw(FVector::ZeroVector, -120.f, true, 0.f), -120.f, 0.01f);
	// Standing still: keep facing unless the camera is more than 60 deg off.
	TestNearlyEqual(TEXT("idle, camera 40 deg off: keep"), C::GetDesiredActorYaw(FVector::ZeroVector, 40.f, false, 0.f), 0.f, 0.01f);
	TestNearlyEqual(TEXT("idle, camera 61 deg off: turn"), C::GetDesiredActorYaw(FVector::ZeroVector, 61.f, false, 0.f), 61.f, 0.01f);
	TestNearlyEqual(TEXT("idle across the wrap: 170 vs -170 is 20 deg: keep"), C::GetDesiredActorYaw(FVector::ZeroVector, -170.f, false, 170.f), 170.f, 0.01f);
	TestNearlyEqual(TEXT("idle across the wrap: 150 vs -100 is 110 deg: turn"), C::GetDesiredActorYaw(FVector::ZeroVector, -100.f, false, 150.f), -100.f, 0.01f);
	// A tiny stick drift is not movement.
	TestNearlyEqual(TEXT("dead-zone drift is idle"), C::GetDesiredActorYaw(FVector(0.05f, 0.05f, 0.f), 10.f, false, 0.f), 0.f, 0.01f);
	TestNearlyEqual(TEXT("vertical move only is idle"), C::GetDesiredActorYaw(FVector(0.f, 0.f, 1.f), 0.f, false, 25.f), 25.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDThirdPersonSwitch, "TriadDrift.Ground.ThirdPerson.SwitchesWithAShortBlend", TD_TEST_FLAGS)
bool FTDThirdPersonSwitch::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDThirdPersonCamTest::FPilot P(W);
	if (!TestTrue(TEXT("pilot, player and rig"), P.Ok()))
	{
		return false;
	}
	UCameraComponent* FP = P.Pilot->GetCamera();
	TestTrue(TEXT("first person by default"), !P.Rig->IsThirdPersonActive() && FP->IsActive());
	TestTrue(TEXT("the view is the first-person eye"), P.ViewLocation().Equals(FP->GetComponentLocation(), 1.f));
	TestTrue(TEXT("aim from the first-person camera"), P.Rig->GetViewCamera() == FP);

	P.Rig->SetThirdPerson(true);
	W.Tick(1.f / 60.f);
	TestTrue(TEXT("third person on the next frame"), P.Rig->IsThirdPersonActive() && P.Rig->GetThirdPersonCamera()->IsActive() && !FP->IsActive());
	TestTrue(TEXT("aim from the chase camera"), P.Rig->GetViewCamera() == P.Rig->GetThirdPersonCamera());
	const float EarlyBack = -FVector::DotProduct(P.Lens() - P.Rig->GetComponentLocation(), FVector::ForwardVector);
	TestTrue(FString::Printf(TEXT("blending, not a cut (%.0f cm back after one frame)"), EarlyBack), P.Rig->IsBlending() && EarlyBack < 200.f);
	W.Advance(0.4f);
	TestFalse(TEXT("the blend is over in 0.15 s"), P.Rig->IsBlending());
	const FVector Off = P.Lens() - P.Rig->GetComponentLocation();
	TestNearlyEqual(TEXT("arm 320 cm in the open"), P.Rig->GetCurrentArmLength(), 320.f, 2.f);
	TestNearlyEqual(TEXT("over the right shoulder (55)"), static_cast<float>(Off.Y), 55.f, 3.f);
	TestNearlyEqual(TEXT("up 65"), static_cast<float>(Off.Z), 65.f, 3.f);
	TestNearlyEqual(TEXT("behind (-320)"), static_cast<float>(Off.X), -320.f, 3.f);
	TestNearlyEqual(TEXT("FOV 85"), P.Rig->GetThirdPersonCamera()->FieldOfView, 85.f, 0.1f);
	TestTrue(TEXT("the pawn's view is the chase lens"), P.ViewLocation().Equals(P.Lens(), 1.f));
	TestTrue(TEXT("looks along the aim (crosshair centred)"),
		P.Rig->GetThirdPersonCamera()->GetForwardVector().Equals(P.PC->GetControlRotation().Vector(), 0.01f));
	TestFalse(TEXT("pilot shown at 3 m"), P.Rig->IsBodyHiddenByCamera() || P.Pilot->GetMesh()->bHiddenInGame);

	// Rotation lag: a fast turn is followed, not snapped.
	P.PC->SetControlRotation(FRotator(0.f, 90.f, 0.f));
	W.Tick(1.f / 60.f);
	const float Yaw1 = P.Rig->GetThirdPersonCamera()->GetComponentRotation().Yaw;
	TestTrue(FString::Printf(TEXT("rotation lag (%.1f deg after one frame)"), Yaw1), Yaw1 > 5.f && Yaw1 < 60.f);
	W.Advance(0.6f);
	TestNearlyEqual(TEXT("catches up"), static_cast<float>(P.Rig->GetThirdPersonCamera()->GetComponentRotation().Yaw), 90.f, 1.f);

	P.Rig->SetThirdPerson(false);
	W.Tick(1.f / 60.f);
	TestFalse(TEXT("first person asked"), P.Rig->IsThirdPersonActive());
	W.Advance(0.4f);
	TestTrue(TEXT("back in first person"), FP->IsActive() && !P.Rig->GetThirdPersonCamera()->IsActive());
	TestTrue(TEXT("the view is the eye again"), P.ViewLocation().Equals(FP->GetComponentLocation(), 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDThirdPersonWall, "TriadDrift.Ground.ThirdPerson.PullsInAtAWallAndEasesOut", TD_TEST_FLAGS)
bool FTDThirdPersonWall::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDThirdPersonCamTest::FPilot P(W);
	if (!TestTrue(TEXT("pilot, player and rig"), P.Ok()))
	{
		return false;
	}
	// A wall 2 m behind the pilot (face at X = -175).
	AActor* Wall = TDGroundTest::SpawnBox(W, FVector(-200.f, 0.f, 300.f), FVector(50.f, 2000.f, 800.f));
	P.Rig->SetThirdPerson(true);
	W.Advance(0.4f);
	FString Why;
	TestTrue(TEXT("lens clear of the wall: ") + Why, P.LensClear(Why));
	TestTrue(FString::Printf(TEXT("in front of the wall face (lens X %.0f)"), P.Lens().X), P.Lens().X > -175.f + 10.f);
	const float Blocked = P.Rig->GetCurrentArmLength();
	TestTrue(FString::Printf(TEXT("arm pulled in (%.0f cm)"), Blocked), Blocked < 200.f);

	// Sweep the view all round and up and down: never inside.
	for (int32 Yaw = 0; Yaw < 360; Yaw += 30)
	{
		for (const float Pitch : { -60.f, -20.f, 0.f, 30.f, 70.f })
		{
			P.PC->SetControlRotation(FRotator(Pitch, Yaw, 0.f));
			W.Tick(1.f / 60.f);
			W.Tick(1.f / 60.f);
			if (!P.LensClear(Why))
			{
				AddError(FString::Printf(TEXT("yaw %d pitch %.0f: %s"), Yaw, Pitch, *Why));
			}
		}
	}

	// The wall goes: the arm eases back out, smoothly (not a jump).
	P.PC->SetControlRotation(FRotator::ZeroRotator);
	W.Advance(0.5f);
	const float Before = P.Rig->GetCurrentArmLength();
	Wall->Destroy();
	W.Tick(1.f / 60.f);
	W.Tick(1.f / 60.f);
	const float Step = P.Rig->GetCurrentArmLength();
	TestTrue(FString::Printf(TEXT("eases out (%.0f -> %.0f in two frames)"), Before, Step), Step > Before && Step < Before + 60.f);
	W.Advance(1.5f);
	TestNearlyEqual(TEXT("back to the full arm"), P.Rig->GetCurrentArmLength(), 320.f, 3.f);

	// Crew standing behind: the lens stays out of them too (pawns ignore the camera channel by default).
	TDGroundTest::SpawnDummy(W, FVector(-200.f, 30.f, 95.f));
	W.Advance(0.3f);
	TestTrue(TEXT("lens clear of the crewman: ") + Why, P.LensClear(Why));
	TestTrue(FString::Printf(TEXT("pulled in in front of the crewman (%.0f cm)"), P.Rig->GetCurrentArmLength()), P.Rig->GetCurrentArmLength() < 200.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDThirdPersonBodyHide, "TriadDrift.Ground.ThirdPerson.HidesThePilotWhenTooClose", TD_TEST_FLAGS)
bool FTDThirdPersonBodyHide::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDThirdPersonCamTest::FPilot P(W);
	if (!TestTrue(TEXT("pilot, player and rig"), P.Ok()))
	{
		return false;
	}
	// A wall right behind (face at X = -60): the lens can only be a hand's width from the pilot.
	const bool bVisibleBefore = P.Pilot->GetMesh()->GetVisibleFlag();
	AActor* Wall = TDGroundTest::SpawnBox(W, FVector(-80.f, 0.f, 300.f), FVector(40.f, 2000.f, 800.f));
	P.Rig->SetThirdPerson(true);
	W.Advance(0.4f);
	const UCapsuleComponent* Capsule = P.Pilot->GetCapsuleComponent();
	const float FromBody = FVector::Dist2D(P.Lens(), Capsule->GetComponentLocation()) - Capsule->GetScaledCapsuleRadius();
	TestTrue(FString::Printf(TEXT("lens close to the pilot (%.0f cm)"), FromBody), FromBody < 60.f);
	TestTrue(TEXT("pilot hidden under 60 cm"), P.Rig->IsBodyHiddenByCamera() && P.Pilot->GetMesh()->bHiddenInGame);
	TestTrue(TEXT("the blade too"), P.Pilot->GetBladeMesh()->bHiddenInGame);
	TestEqual(TEXT("T5's own visibility flag untouched"), P.Pilot->GetMesh()->GetVisibleFlag(), bVisibleBefore);

	Wall->Destroy();
	W.Advance(2.f);
	TestFalse(TEXT("shown again in the open"), P.Rig->IsBodyHiddenByCamera() || P.Pilot->GetMesh()->bHiddenInGame || P.Pilot->GetBladeMesh()->bHiddenInGame);

	// Back to first person gives everything back too.
	TestTrue(TEXT("wall again"), TDGroundTest::SpawnBox(W, FVector(-80.f, 0.f, 300.f), FVector(40.f, 2000.f, 800.f)) != nullptr);
	W.Advance(0.3f);
	TestTrue(TEXT("hidden again"), P.Rig->IsBodyHiddenByCamera());
	P.Rig->SetThirdPerson(false);
	W.Advance(0.4f);
	TestFalse(TEXT("first person: nothing left hidden"), P.Rig->IsBodyHiddenByCamera() || P.Pilot->GetMesh()->bHiddenInGame || P.Pilot->GetBladeMesh()->bHiddenInGame);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDThirdPersonDirector, "TriadDrift.Ground.ThirdPerson.NoViewChangeWhileTheDirectorOwnsIt", TD_TEST_FLAGS)
bool FTDThirdPersonDirector::RunTest(const FString& Parameters)
{
	TDThirdPersonCamTest::FScopedDirectorMode Full(2);
	FTDTestWorld W(UTDGameInstance::StaticClass());
	TDThirdPersonCamTest::FPilot P(W);
	if (!TestTrue(TEXT("pilot, player and rig"), P.Ok()))
	{
		return false;
	}
	if (!P.Pilot->GetMesh()->GetSkeletalMeshAsset())
	{
		P.Pilot->GetMesh()->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr, TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube")));
	}
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	UTDDirectorCameraRig* DirectorRig = UTDDirectorSubsystem::Get(W.GetWorld())->GetCameraRig();

	// The on-foot death cam owns the view during the CUT.
	UTDStatics::GetHealth(P.Pilot)->Kill();
	W.Tick();
	if (!TestTrue(TEXT("death cam on"), DirectorRig->IsDeathCam()))
	{
		return false;
	}
	const AActor* DirectorView = P.PC->GetViewTarget();
	P.Rig->SetThirdPerson(true);
	W.Advance(0.5f);
	TestTrue(TEXT("asked for, waiting"), P.Rig->WantsThirdPerson() && P.Rig->IsChangePending() && !P.Rig->IsThirdPersonActive());
	TestTrue(TEXT("the director keeps the view"), P.PC->GetViewTarget() == DirectorView && DirectorRig->IsDeathCam());
	TestTrue(TEXT("the first-person camera still active underneath"), P.Pilot->GetCamera()->IsActive());

	// The retake gives the view back: the change applies then.
	for (int32 i = 0; i < 60 * 8 && GI && GI->IsRetakePending(); ++i)
	{
		W.Tick(1.f / 60.f);
		GI->TickFlow(1.f / 60.f);
	}
	W.Advance(0.4f);
	TestTrue(TEXT("view back on the pilot"), P.PC->GetViewTarget() == P.Pilot);
	TestTrue(TEXT("third person applied after the director let go"), P.Rig->IsThirdPersonActive() && !P.Rig->IsChangePending());
	TestTrue(TEXT("the pawn's view is the chase lens"), P.ViewLocation().Equals(P.Lens(), 1.f));

	// Anything else taking the view (a cutscene camera) holds a change the same way.
	AActor* Other = W.Spawn<AActor>(FVector(0.f, 0.f, 1000.f));
	P.PC->SetViewTarget(Other);
	P.Rig->SetThirdPerson(false);
	W.Advance(0.3f);
	TestTrue(TEXT("held while someone else has the view"), P.Rig->IsChangePending() && P.PC->GetViewTarget() == Other);
	P.PC->SetViewTarget(P.Pilot);
	W.Advance(0.4f);
	TestTrue(TEXT("applied once it is back"), !P.Rig->IsChangePending() && P.Pilot->GetCamera()->IsActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDThirdPersonCourses, "TriadDrift.Ground.ThirdPerson.NeverInsideThePlanetCourses", TD_TEST_FLAGS)
bool FTDThirdPersonCourses::RunTest(const FString& Parameters)
{
	// The planet courses' real geometry (T5's builders): stand beside walls, rocks, crates and colliding trees,
	// swing the camera round, and check the lens every time.
	for (const TCHAR* Planet : { TEXT("Verdance"), TEXT("Cinder") })
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		ATDPlanetCourse* Course = W.GetWorld()->SpawnActorDeferred<ATDPlanetCourse>(ATDPlanetCourse::StaticClass(), FTransform::Identity);
		if (!TestNotNull(TEXT("course"), Course))
		{
			return false;
		}
		Course->PlanetId = Planet;
		Course->FinishSpawning(FTransform::Identity);
		W.Tick();
		TDThirdPersonCamTest::FPilot P(W, FVector(0.f, 0.f, 5000.f), false);
		if (!TestTrue(TEXT("pilot, player and rig"), P.Ok()))
		{
			return false;
		}
		P.Rig->SetThirdPerson(true);
		W.Advance(0.3f);

		// Colliding pieces of a person's size or more (not the ground slabs).
		TArray<FBox> Pieces;
		for (const TObjectPtr<AActor>& A : Course->GetSpawned())
		{
			TArray<UPrimitiveComponent*> Prims;
			if (A)
			{
				A->GetComponents(Prims);
			}
			for (const UPrimitiveComponent* Prim : Prims)
			{
				if (!Prim || !Prim->IsCollisionEnabled() || Prim->GetCollisionResponseToChannel(ECC_Camera) != ECR_Block)
				{
					continue;
				}
				const FBox B = Prim->Bounds.GetBox();
				const FVector E = B.GetExtent();
				if (FMath::Max(E.X, E.Y) < 3000.f && FMath::Max(E.X, E.Y) > 30.f && E.Z > 60.f)
				{
					Pieces.Add(B);
				}
			}
		}
		TestTrue(FString::Printf(TEXT("%s: something to stand beside (%d pieces)"), Planet, Pieces.Num()), Pieces.Num() > 5);

		int32 Poses = 0;
		int32 PulledIn = 0;
		int32 Errors = 0;
		const int32 Stride = FMath::Max(1, Pieces.Num() / 20);
		const FCollisionShape Body = FCollisionShape::MakeCapsule(P.Pilot->GetCapsuleComponent()->GetScaledCapsuleRadius(), P.Pilot->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		FCollisionQueryParams Q(SCENE_QUERY_STAT(TDThirdPersonCourse), false, P.Pilot);
		for (int32 i = 0; i < Pieces.Num(); i += Stride)
		{
			const FBox& B = Pieces[i];
			FVector Dir = (-B.GetCenter()).GetSafeNormal2D();
			if (Dir.IsNearlyZero())
			{
				Dir = FVector::ForwardVector;
			}
			const FVector XY = B.GetCenter() + Dir * (FMath::Max(B.GetExtent().X, B.GetExtent().Y) + 90.f);
			FHitResult Ground;
			if (!W.GetWorld()->LineTraceSingleByChannel(Ground, FVector(XY.X, XY.Y, B.Max.Z + 3000.f), FVector(XY.X, XY.Y, B.Min.Z - 3000.f), ECC_Visibility, Q))
			{
				continue;
			}
			const FVector Stand = Ground.ImpactPoint + FVector(0.f, 0.f, Body.GetCapsuleHalfHeight() + 5.f);
			if (W.GetWorld()->OverlapBlockingTestByChannel(Stand, FQuat::Identity, ECC_Pawn, Body, Q))
			{
				continue; // no room to stand here
			}
			P.Pilot->SetActorLocation(Stand, false, nullptr, ETeleportType::TeleportPhysics);
			const float ToPiece = (B.GetCenter() - Stand).Rotation().Yaw;
			for (const float YawOff : { 180.f, 90.f, -90.f, 0.f, 135.f })
			{
				for (const float Pitch : { -35.f, 0.f, 40.f })
				{
					P.PC->SetControlRotation(FRotator(Pitch, ToPiece + YawOff, 0.f));
					W.Tick(1.f / 60.f);
					W.Tick(1.f / 60.f);
					W.Tick(1.f / 60.f);
					++Poses;
					PulledIn += P.Rig->GetCurrentArmLength() < 300.f ? 1 : 0;
					FString Why;
					if (!P.LensClear(Why) && ++Errors <= 8)
					{
						AddError(FString::Printf(TEXT("%s: beside %s, yaw %.0f pitch %.0f: %s"), Planet, *B.GetCenter().ToCompactString(), ToPiece + YawOff, Pitch, *Why));
					}
				}
			}
		}
		AddInfo(FString::Printf(TEXT("%s: %d poses beside %d pieces, %d pulled in, %d inside"), Planet, Poses, Pieces.Num(), PulledIn, Errors));
		TestTrue(FString::Printf(TEXT("%s: enough poses (%d)"), Planet, Poses), Poses >= 60);
		TestTrue(FString::Printf(TEXT("%s: the geometry really pushed the camera (%d pulled in)"), Planet, PulledIn), PulledIn >= 10);
		TestEqual(FString::Printf(TEXT("%s: lens never inside"), Planet), Errors, 0);
	}
	return true;
}

#endif
