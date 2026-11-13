// Triad Drift - the main menu's backdrop. Owner: Teammate 3.

#include "TriadDrift/Director/Menu/TDMenuBackdrop.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "Camera/PlayerCameraManager.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Flow/TDPlaceholderSky.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/World/TDSkySphere.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"

static TAutoConsoleVariable<int32> CVarTDMenuBackdrop(
	TEXT("td.MenuBackdrop"), 1,
	TEXT("Main menu backdrop (sky, Verdance, drifting camera, squad fly-bys): 1 = on (default), 0 = off."),
	ECVF_Default);

const FName ATDMenuBackdrop::MenuSceneTag(TEXT("TD.Menu.Scene"));

namespace TDMenuBackdrop
{
	static float Ease(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}
}

ATDMenuBackdrop::ATDMenuBackdrop()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

ATDMenuBackdrop* ATDMenuBackdrop::SpawnFor(AGameModeBase* MenuMode, ETDBackdropScene InScene)
{
	UWorld* World = MenuMode ? MenuMode->GetWorld() : nullptr;
	if (!World || CVarTDMenuBackdrop.GetValueOnGameThread() == 0)
	{
		return nullptr;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(MenuSceneTag))
		{
			UE_LOG(LogTriadDrift, Log, TEXT("Menu backdrop: the map has its own scene (%s); off"), *It->GetName());
			return nullptr;
		}
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	Params.bDeferConstruction = true;
	ATDMenuBackdrop* B = World->SpawnActor<ATDMenuBackdrop>(ATDMenuBackdrop::StaticClass(), FTransform::Identity, Params);
	if (B)
	{
		B->Scene = InScene;
		B->FinishSpawning(FTransform::Identity);
	}
	return B;
}

void ATDMenuBackdrop::BeginPlay()
{
	Super::BeginPlay();
	Build();
}

void ATDMenuBackdrop::Build()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// The engineering nebula sky and the sun that shines from it (fixed exposure, bloom: the galaxy's look).
	if (AActor* Sky = ATDSkySphere::SpawnIfAvailable(World))
	{
		Spawned.Add(Sky);
	}
	if (AActor* Lights = ATDPlaceholderSky::SpawnIfNeeded(World, ETDPlaceholderSkyStyle::Space))
	{
		Spawned.Add(Lights);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;

	// Ringed Verdance, with the galaxy's own look, three-quarter lit by that sun.
	const FTDGalaxyLayoutData Layout = ATDGalaxyLayout::MakeDefaultLayout();
	const FTDPlanetSpec* V = Layout.Planets.FindByPredicate([](const FTDPlanetSpec& P) { return P.Destination == ETDDestination::Verdance; });
	PlanetRadius = V ? V->PlanetRadius : 60000.f;
	PlanetAt = GetActorLocation();
	{
		const FTransform T(FRotator::ZeroRotator, PlanetAt);
		ATDPlanetAtmosphere* P = World->SpawnActorDeferred<ATDPlanetAtmosphere>(ATDPlanetAtmosphere::StaticClass(), T, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (P)
		{
			P->Configure(ETDDestination::Verdance, NAME_None, PlanetRadius, V ? V->AtmosphereRadius : PlanetRadius * 1.2f, V ? V->Color : FLinearColor(0.1f, 0.9f, 0.5f));
			if (V)
			{
				P->Look = V->Look;
			}
			P->FinishSpawning(T);
			P->SetActorEnableCollision(false); // scenery: no atmosphere trigger in the menu
			Planet = P;
			Spawned.Add(P);
		}
	}

	// The lens: 65 degrees round from the sun (the planet three-quarter lit), the planet on the right third.
	const FVector ToSun = FRotator(ATDSkySphere::SunPitchDeg, ATDSkySphere::SunYawDeg, 0.f).Vector();
	const FVector CamDir = ToSun.RotateAngleAxis(-65.f, FVector::UpVector).GetSafeNormal2D();
	// Far enough that the whole ring (2.3 planet radii) sits in the right third of a 40 degree frame.
	CamBase = PlanetAt + (CamDir + FVector::UpVector * 0.22f).GetSafeNormal() * (PlanetRadius * 14.f);
	const FVector ToPlanet = (PlanetAt - CamBase).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ToPlanet).GetSafeNormal();
	LookBase = PlanetAt - Right * (PlanetRadius * 2.6f);
	Camera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform((LookBase - CamBase).Rotation(), CamBase), Params);
	if (Camera)
	{
		UCameraComponent* Cam = Camera->GetCameraComponent();
		Cam->bConstrainAspectRatio = false;
		Cam->SetFieldOfView(40.f);
		Cam->PostProcessBlendWeight = 1.f;
		Cam->PostProcessSettings.bOverride_FilmGrainIntensity = true;
		Cam->PostProcessSettings.FilmGrainIntensity = 0.08f;
		Cam->PostProcessSettings.bOverride_VignetteIntensity = true;
		Cam->PostProcessSettings.VignetteIntensity = 0.5f;
		Spawned.Add(Camera);
	}

	// The squad's art, for the fly-bys (hidden between passes).
	AActor* Squad = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (Squad)
	{
		USceneComponent* Root = NewObject<USceneComponent>(Squad, TEXT("Root"));
		Squad->SetRootComponent(Root);
		Root->RegisterComponent();
		for (ETDShipId Id : UTDShipLibrary::AllShips())
		{
			const FTDShipSpec Spec = UTDShipLibrary::GetShipSpec(Id);
			const UTDShipVisuals* Visuals = UTDShipLibrary::LoadVisuals(Spec);
			UStaticMesh* Mesh = Visuals ? Visuals->Mesh.LoadSynchronous() : nullptr;
			if (!Mesh)
			{
				continue;
			}
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Squad);
			C->SetupAttachment(Root);
			C->SetStaticMesh(Mesh);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetUsingAbsoluteLocation(true);
			C->SetUsingAbsoluteRotation(true);
			for (int32 i = 0; i < Visuals->MaterialOverrides.Num(); ++i)
			{
				if (UMaterialInterface* M = Visuals->MaterialOverrides[i].LoadSynchronous())
				{
					C->SetMaterial(i, M);
				}
			}
			C->SetVisibility(false);
			C->RegisterComponent();
			SquadMeshes.Add(C);
		}
		Spawned.Add(Squad);
	}
	if (Scene == ETDBackdropScene::Victory)
	{
		BuildVictory(World);
	}
	NextFlyby = FirstFlybySeconds;
	if (APlayerController* PC = World->GetFirstPlayerController(); PC && Camera)
	{
		PC->SetViewTarget(Camera);
	}
	UpdateCamera();
	UE_LOG(LogTriadDrift, Log, TEXT("Menu backdrop (%s): sky, Verdance, camera and %d squad ships%s"),
		Scene == ETDBackdropScene::Victory ? TEXT("victory") : TEXT("menu"), SquadMeshes.Num(),
		Dominion ? TEXT(", the Dominion breaking up") : TEXT(""));
}

void ATDMenuBackdrop::BuildVictory(UWorld* World)
{
	// That's a wrap: the Dominion breaking up in the middle distance beside ringed Verdance, fires walking along its
	// hull; the squad flies away from it, home past the lens; the camera pushes in slowly.
	const FVector ToPlanet = (PlanetAt - CamBase).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ToPlanet).GetSafeNormal();
	DominionRadius = UTDIntroDirector::DominionRadius();
	const FVector At = FMath::Lerp(CamBase, PlanetAt, 0.5f) - Right * (PlanetRadius * 1.9f) - FVector::UpVector * (PlanetRadius * 0.35f);
	// Three-quarter on to the lens, its bow toward the frame's left edge.
	const FRotator Facing = (-Right * 0.8f - ToPlanet * 0.6f).Rotation();
	DominionStart = FTransform(Facing, At);
	Dominion = UTDIntroDirector::SpawnDominionVisual(World, DominionStart);
	if (Dominion)
	{
		Spawned.Add(Dominion);
	}
	// The lens looks between the wreck and the planet; the stats sit in the sky above.
	LookBase = FMath::Lerp(At, PlanetAt, 0.45f) + FVector::UpVector * (PlanetRadius * 0.4f);
	FireRng.Initialize(20261115);
	NextFire = 0.2f;
	FirstFlybySeconds = 2.f;
	FlybySeconds = 10.f;
	FlybyEverySeconds = 16.f;
}

void ATDMenuBackdrop::UpdateBreakup(float Dt)
{
	if (!Dominion)
	{
		return;
	}
	// The ending stinger (storyline.md act 5): after the breakup, the squad's scanners take a distress signal bearing
	// their own call signs, dated decades ahead. Cut to black under it; the victory panel stays usable.
	if (!bStingerPlayed && Time >= StingerSeconds)
	{
		bStingerPlayed = true;
		UTDStorySubsystem::Fire(this, TEXT("A5_DistressSignal"));
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr; PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.35f, FLinearColor::Black, false, /*bHoldWhenFinished*/ true);
		}
	}
	// The hull lists and sinks as it dies, over the first minute, then holds.
	const float A = FMath::Clamp(Time / 60.f, 0.f, 1.f);
	const FRotator List = DominionStart.Rotator() + FRotator(-7.f * A, 0.f, 11.f * A);
	Dominion->SetActorLocationAndRotation(DominionStart.GetLocation() - FVector::UpVector * (DominionRadius * 0.4f * A), List);
	// Fires: small bursts walking along the hull, now and then a big one.
	if (Time < NextFire)
	{
		return;
	}
	NextFire = Time + FireRng.FRandRange(0.18f, 0.6f);
	UTDProjectileSubsystem* P = UTDProjectileSubsystem::Get(this);
	if (!P)
	{
		return;
	}
	// On the lens-facing skin: aim from the lens at a random point inside the hull's box and burn where the ray enters
	// it (a point inside the hull would be hidden by it).
	const FBox Box = Dominion->GetComponentsBoundingBox(true).ExpandBy(-0.15f * DominionRadius);
	const FVector Inner(FireRng.FRandRange(Box.Min.X, Box.Max.X), FireRng.FRandRange(Box.Min.Y, Box.Max.Y),
		FireRng.FRandRange(Box.Min.Z, Box.Max.Z));
	const FVector Eye = Camera ? Camera->GetActorLocation() : Inner;
	FVector Where = Inner;
	{
		const FVector D = Inner - Eye;
		float TEnter = 0.f;
		for (int32 Ax = 0; Ax < 3; ++Ax)
		{
			if (FMath::Abs(D[Ax]) > KINDA_SMALL_NUMBER)
			{
				const float T0 = (Box.Min[Ax] - Eye[Ax]) / D[Ax];
				const float T1 = (Box.Max[Ax] - Eye[Ax]) / D[Ax];
				TEnter = FMath::Max(TEnter, FMath::Min(T0, T1));
			}
		}
		Where = Eye + D * FMath::Clamp(TEnter, 0.f, 1.f);
	}
	// The arcade explosion placeholder (additive, soft-edged, cooling through orange; Nick's FX Death when it lands):
	// small bursts walking along the hull, now and then a big one.
	const bool bBig = FireRng.FRand() < 0.12f;
	const FLinearColor Hot = FireRng.FRand() < 0.5f ? FLinearColor(1.f, 0.42f, 0.08f) : FLinearColor(1.f, 0.2f, 0.05f);
	P->PlayPlaceholderExplosion(Where, DominionRadius * (bBig ? 0.22f : FireRng.FRandRange(0.05f, 0.1f)), Hot, bBig ? 6 : 0);
	++Fires;
}

void ATDMenuBackdrop::UpdateCamera()
{
	if (!Camera)
	{
		return;
	}
	if (Scene == ETDBackdropScene::Victory)
	{
		// A slow push-in toward the wreck, eased over 40 s, then held.
		const float A = FMath::Clamp(Time / 40.f, 0.f, 1.f);
		const float E = A * A * (3.f - 2.f * A);
		const FVector Cam = FMath::Lerp(CamBase, LookBase, 0.12f * E) + FVector::UpVector * FMath::Sin(Time * 0.19f) * 500.f;
		Camera->SetActorLocationAndRotation(Cam, (LookBase - Cam).Rotation());
		return;
	}
	// A slow drift: a few degrees round the planet and a breath of push, over a minute.
	const float Orbit = FMath::Sin(Time * UE_TWO_PI / 70.f) * 4.f;
	const float Push = 1.f - 0.03f * (0.5f - 0.5f * FMath::Cos(Time * UE_TWO_PI / 45.f));
	const FVector Offset = (CamBase - PlanetAt).RotateAngleAxis(Orbit, FVector::UpVector) * Push;
	const FVector Cam = PlanetAt + Offset + FVector::UpVector * FMath::Sin(Time * 0.21f) * 600.f;
	const FVector Look = PlanetAt + (LookBase - PlanetAt).RotateAngleAxis(Orbit, FVector::UpVector);
	Camera->SetActorLocationAndRotation(Cam, (Look - Cam).Rotation());
}

void ATDMenuBackdrop::UpdateFlyby(float Dt)
{
	if (FlybyT < 0.f && Time >= NextFlyby && Camera)
	{
		// Across the frame left to right through the lower third, under the menu, at a glide. In the lens's own space
		// (x forward, y right, z up): the camera drifts kilometres round the planet, so the squad flies with it.
		FlybyFrom = FVector(8000.f, -4200.f, -1300.f);
		FlybyTo = FVector(10500.f, 4400.f, -700.f);
		if (Scene == ETDBackdropScene::Victory)
		{
			// Away from the burning Dominion (deep in frame, left) and home past the lens on the right.
			FlybyFrom = FVector(70000.f, -14000.f, -6000.f);
			FlybyTo = FVector(4500.f, 5200.f, -900.f);
		}
		FlybyT = 0.f;
		++Flybys;
		for (UStaticMeshComponent* C : SquadMeshes)
		{
			C->SetVisibility(true);
		}
	}
	if (FlybyT < 0.f)
	{
		return;
	}
	FlybyT += Dt;
	const float A = FlybyT / FMath::Max(FlybySeconds, 0.1f);
	if (A >= 1.f)
	{
		FlybyT = -1.f;
		NextFlyby = Time + FMath::Max(1.f, FlybyEverySeconds - FlybySeconds);
		for (UStaticMeshComponent* C : SquadMeshes)
		{
			C->SetVisibility(false);
		}
		return;
	}
	const FTransform Lens = Camera ? Camera->GetActorTransform() : FTransform::Identity;
	const FVector Dir = Lens.TransformVectorNoScale((FlybyTo - FlybyFrom).GetSafeNormal());
	const FRotator Facing = Dir.Rotation();
	const FVector Lead = Lens.TransformPosition(FMath::Lerp(FlybyFrom, FlybyTo, A));
	// A loose wedge, each ship on its own gentle bank.
	const FVector Wedge[3] = { FVector::ZeroVector, FVector(-900.f, -800.f, 180.f), FVector(-1100.f, 850.f, -120.f) };
	for (int32 i = 0; i < SquadMeshes.Num(); ++i)
	{
		const FVector At = Lead + Facing.RotateVector(Wedge[i % 3]) + FVector(0.f, 0.f, FMath::Sin(Time * 1.1f + i) * 40.f);
		const FRotator Bank = Facing + FRotator(0.f, 0.f, FMath::Sin(Time * 0.7f + i * 1.9f) * 8.f - 6.f);
		SquadMeshes[i]->SetWorldLocationAndRotation(At, Bank);
	}
}

void ATDMenuBackdrop::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	UpdateCamera();
	UpdateFlyby(DeltaSeconds);
	UpdateBreakup(DeltaSeconds);
	// Keep the view: something resetting the menu's view target (a respawned spectator) must not drop the scene.
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr; PC && Camera)
	{
		AActor* View = PC->GetViewTarget();
		if (View != Camera && (!View || View == PC || View == PC->GetPawn() || View == PC->GetSpectatorPawn()))
		{
			PC->SetViewTarget(Camera);
		}
	}
}

void ATDMenuBackdrop::DebugSetTime(float Seconds)
{
	// Replays the loop to that moment in 1/30 s steps (deterministic captures).
	Time = 0.f;
	FlybyT = -1.f;
	NextFlyby = FirstFlybySeconds;
	Flybys = 0;
	FireRng.Initialize(20261115);
	NextFire = Seconds - 1.f; // only the last second's fires are replayed
	for (UStaticMeshComponent* C : SquadMeshes)
	{
		C->SetVisibility(false);
	}
	for (float T = 0.f; T < Seconds; T += 1.f / 30.f)
	{
		Time += 1.f / 30.f;
		UpdateCamera();
		UpdateFlyby(1.f / 30.f);
		UpdateBreakup(1.f / 30.f);
	}
}

void ATDMenuBackdrop::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AActor* A : Spawned)
	{
		if (A && !A->IsActorBeingDestroyed())
		{
			A->Destroy();
		}
	}
	Spawned.Reset();
	Super::EndPlay(EndPlayReason);
}
