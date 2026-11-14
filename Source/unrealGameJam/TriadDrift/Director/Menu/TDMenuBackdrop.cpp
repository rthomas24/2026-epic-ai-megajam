// Triad Drift - the main menu's backdrop. Owner: Teammate 3.

#include "TriadDrift/Director/Menu/TDMenuBackdrop.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"

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
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Core/TDStatics.h"
#include "NiagaraComponent.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/World/TDSkySphere.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"

static float MenuPlanetOffsetRadii = 3.2f;
static FAutoConsoleVariableRef CVarTDMenuPlanetOffset(TEXT("td.Menu.PlanetOffset"), MenuPlanetOffsetRadii,
	TEXT("Menu backdrop: how far right of the frame's centre Verdance sits, in planet radii (read when the menu opens)."));

static TAutoConsoleVariable<int32> CVarTDMenuBackdrop(
	TEXT("td.MenuBackdrop"), 1,
	TEXT("Main menu backdrop (sky, Verdance, drifting camera, squad fly-bys): 1 = on (default), 0 = off."),
	ECVF_Default);

const FName ATDMenuBackdrop::MenuSceneTag(TEXT("TD.Menu.Scene"));
ATDMenuBackdrop::FTDOnVictoryBackdrop ATDMenuBackdrop::OnVictoryBegan;
ATDMenuBackdrop::FTDOnVictoryBackdrop ATDMenuBackdrop::OnStinger;
ATDMenuBackdrop::FTDOnVictoryBackdrop ATDMenuBackdrop::OnVictoryEnded;

void ATDMenuBackdrop::FadeStingerCaption()
{
	if (CaptionFadeStart <= 0.0)
	{
		CaptionFadeStart = FPlatformTime::Seconds();
		if (Scene == ETDBackdropScene::Victory)
		{
			OnVictoryEnded.Broadcast(this); // the menu is up (T7 fades the credits music)
		}
	}
}

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
	// Nicholas #90: the planet's body clear of the centred menu panel (ends ~0.66 of the width), through the drift.
	LookBase = PlanetAt - Right * (PlanetRadius * MenuPlanetOffsetRadii);
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
			// The ship's visual parts (canopies): attached, so they fly, bank and hide with the hull.
			for (const FTDShipVisualPart& PartSpec : Visuals->Parts)
			{
				UStaticMesh* PartMesh = PartSpec.Mesh.IsNull() ? nullptr : PartSpec.Mesh.LoadSynchronous();
				if (!PartMesh) { continue; }
				UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Squad);
				Part->SetupAttachment(C, C->DoesSocketExist(PartSpec.ParentSocket) ? PartSpec.ParentSocket : NAME_None);
				Part->SetRelativeTransform(PartSpec.RelativeTransform);
				Part->SetStaticMesh(PartMesh);
				Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				for (int32 i = 0; i < PartSpec.MaterialOverrides.Num(); ++i)
				{
					if (UMaterialInterface* M = PartSpec.MaterialOverrides[i].LoadSynchronous()) { Part->SetMaterial(i, M); }
				}
				Part->SetVisibility(false);
				Part->RegisterComponent();
			}
			// The ship's own thrusters (NS_TD_Engine_<ship>): one per nozzle socket, the hull origin without; no sound
			// in the menu, a short cruising trail.
			FTDFXCue Trail = Visuals->EngineTrail;
			Trail.Sound.Reset();
			Trail.Shake.Reset();
			if (!Trail.VFX.IsNull())
			{
				TArray<FName> Nozzles;
				ATDShip::GetNozzleSockets(C, Nozzles);
				if (Nozzles.Num() == 0) { Nozzles.Add(NAME_None); }
				for (const FName& Socket : Nozzles)
				{
					if (UNiagaraComponent* FX = UTDStatics::PlayCueAttached(Trail, C, Socket))
					{
						ATDShip::SetExhaustParamIfExposed(FX, TEXT("Throttle"), 0.8f);
						ATDShip::SetExhaustParamIfExposed(FX, TEXT("Boost"), 0.25f);
						FX->SetAutoDestroy(false); // it is switched off between passes: it must survive that
						FX->Deactivate();
						FX->SetVisibility(false);
						SquadTrails.Add(FX);
					}
				}
			}
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
	if (Scene == ETDBackdropScene::Victory)
	{
		OnVictoryBegan.Broadcast(this); // the victory screen and its credits roll are up (T7's credits music)
	}
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
		// A wreck, not the ship the player just destroyed intact (Lead): its parts drift apart and tumble from the first
		// frame, and dark debris scatters out of the hull.
		FRandomStream R(41);
		TInlineComponentArray<UStaticMeshComponent*> Parts(Dominion);
		UMaterialInterface* Armour = nullptr;
		for (UStaticMeshComponent* C : Parts)
		{
			if (!C)
			{
				continue;
			}
			Armour = Armour ? Armour : C->GetMaterial(0);
			const bool bHull = C->GetName().Contains(TEXT("Hull"));
			FWreckPiece W;
			W.Component = C;
			W.Start = C->GetRelativeTransform();
			W.Drift = bHull ? FVector::ZeroVector : R.GetUnitVector() * DominionRadius * R.FRandRange(0.02f, 0.05f);
			W.Spin = bHull ? FRotator::ZeroRotator : FRotator(R.FRandRange(-4.f, 4.f), R.FRandRange(-4.f, 4.f), R.FRandRange(-6.f, 6.f));
			W.Head = bHull ? 0.f : 8.f; // seconds of breakup already behind it at the first frame
			Wreck.Add(W);
		}
		if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
		{
			for (int32 i = 0; i < 14; ++i)
			{
				UStaticMeshComponent* D = NewObject<UStaticMeshComponent>(Dominion, *FString::Printf(TEXT("Debris%d"), i));
				D->SetupAttachment(Dominion->GetRootComponent());
				D->SetStaticMesh(Cube);
				D->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				D->SetCastShadow(false);
				if (Armour)
				{
					D->SetMaterial(0, Armour);
				}
				D->RegisterComponent();
				const float Size = DominionRadius * R.FRandRange(0.004f, 0.02f);
				FWreckPiece W;
				W.Component = D;
				W.Start = FTransform(R.GetUnitVector().Rotation(), R.GetUnitVector() * DominionRadius * R.FRandRange(0.1f, 0.4f),
					FVector(Size / 100.f) * FVector(R.FRandRange(0.6f, 1.6f), R.FRandRange(0.4f, 1.2f), R.FRandRange(0.2f, 0.8f)));
				W.Drift = W.Start.GetLocation().GetSafeNormal() * DominionRadius * R.FRandRange(0.02f, 0.06f);
				W.Spin = FRotator(R.FRandRange(-30.f, 30.f), R.FRandRange(-30.f, 30.f), R.FRandRange(-30.f, 30.f));
				W.Head = 6.f;
				Wreck.Add(W);
			}
		}
	}
	// The lens looks between the wreck and the planet; the stats sit in the sky above.
	LookBase = FMath::Lerp(At, PlanetAt, 0.45f) + FVector::UpVector * (PlanetRadius * 0.4f);
	FireRng.Initialize(20261115);
	NextFire = 0.2f;
	FirstFlybySeconds = 2.f;
	FlybySeconds = 10.f;
	FlybyEverySeconds = 16.f;
}

FText ATDMenuBackdrop::GetStingerCaption() const
{
	FTDTransmission E;
	return UTDStorySubsystem::FindEntry(TEXT("A5_DistressSignal"), E) ? E.Text : FText::GetEmpty();
}

float ATDMenuBackdrop::PlayStinger()
{
	if (bStingerPlayed)
	{
		return 0.f;
	}
	bStingerPlayed = true;
	OnStinger.Broadcast(this);

	if (UTDStorySubsystem* Story = UTDStorySubsystem::Get(this))
	{
		Story->SetSubtitleSuppressed(TEXT("A5_DistressSignal"), true); // the caption carries its words: once on screen
	}
	UTDStorySubsystem::Fire(this, TEXT("A5_DistressSignal"));
	// Cut to black: a black layer over the scene, under the victory panel (z 10) and the transmission line (905).
	StingerStart = FPlatformTime::Seconds();
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->GetWorld() == GetWorld() && !StingerBlack.IsValid())
	{
		TWeakObjectPtr<ATDMenuBackdrop> Weak(this);
		// The caption is held on the black (top third, above the victory panel) while the three lines play under it.
		const FText Caption = GetStingerCaption();
		StingerBlack = SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.Visibility(EVisibility::HitTestInvisible)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top)
			.Padding(FMargin(40.f, 120.f, 40.f, 0.f))
			.BorderBackgroundColor_Lambda([Weak]()
			{
				const ATDMenuBackdrop* B = Weak.Get();
				const float A = B ? FMath::Clamp(static_cast<float>(FPlatformTime::Seconds() - B->StingerStart) / 0.35f, 0.f, 1.f) : 1.f;
				return FSlateColor(FLinearColor(0.f, 0.f, 0.f, A));
			})
			[
				SNew(STextBlock)
				.Text(Caption)
				.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 20))
				.ColorAndOpacity_Lambda([Weak]()
				{
					// Fades over 1 s once the menu comes back (FadeStingerCaption), so the menu comes up clean.
					const ATDMenuBackdrop* B = Weak.Get();
					const float A = (B && B->CaptionFadeStart > 0.0) ? 1.f - FMath::Clamp(static_cast<float>(FPlatformTime::Seconds() - B->CaptionFadeStart), 0.f, 1.f) : 1.f;
					return FSlateColor(FLinearColor(1.f, 0.55f, 0.5f, A));
				})
				.Justification(ETextJustify::Center)
				.AutoWrapText(true)
			];
		GEngine->GameViewport->AddViewportWidgetContent(StingerBlack.ToSharedRef(), 5);
	}
	// Then the squad reads it: identifiers, the source, the date (Cinematic 09, the last three lines).
	float Seconds = 0.35f;
	for (const TCHAR* Id : { TEXT("A5_DistressSignal"), TEXT("C9_Kato_Identifiers"), TEXT("C9_Wren_Source"), TEXT("C9_Kato_Decades") })
	{
		if (Id != FString(TEXT("A5_DistressSignal")))
		{
			UTDStorySubsystem::Fire(this, Id);
		}
		FTDTransmission E;
		if (UTDStorySubsystem::FindEntry(Id, E))
		{
			Seconds += UTDStorySubsystem::SecondsFor(E) + 0.5f;
		}
	}
	return Seconds;
}

void ATDMenuBackdrop::UpdateBreakup(float Dt)
{
	if (!Dominion)
	{
		return;
	}
	// Cinematic 09: Moss, as the breakup completes behind the squad.
	if (!bTakeThatPlayed && Time >= TakeThatSeconds && !bStingerPlayed)
	{
		bTakeThatPlayed = true;
		UTDStorySubsystem::Fire(this, TEXT("C9_Moss_TakeThat"));
	}
	// The ending stinger (Cinematic 09): the distress signal bearing their own identifiers, dated decades ahead.
	if (!bStingerPlayed && Time >= StingerSeconds)
	{
		PlayStinger();
	}
	// The parts drift apart and tumble (a wreck from the first frame).
	for (const FWreckPiece& W : Wreck)
	{
		if (UStaticMeshComponent* C = W.Component.Get())
		{
			const float T = Time + W.Head;
			C->SetRelativeLocationAndRotation(W.Start.GetLocation() + W.Drift * T, (W.Start.Rotator() + W.Spin * T).Quaternion());
		}
	}
	// The hull lists hard and sinks as it dies, then holds.
	const float A = FMath::Clamp(0.35f + Time / 60.f, 0.f, 1.f);
	const FRotator List = DominionStart.Rotator() + FRotator(-12.f * A, 0.f, 25.f * A);
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
		// Nicholas #90 (3): both ends far enough out that the trailing wingmen (900-1100 cm behind the lead) are past the
		// frame edge too, at 16:9 and at 21:9 (the 40-degree lens is ~2900 cm half-wide at 8 km, ~5300 at 11 km on 21:9).
		FlybyFrom = FVector(7500.f, -8200.f, -1450.f);
		FlybyTo = FVector(11000.f, 9000.f, -620.f);
		if (Scene == ETDBackdropScene::Victory)
		{
			// Away from the burning Dominion (deep in frame, left) and home past the lens on the right.
			FlybyFrom = FVector(70000.f, -14000.f, -6000.f);
			FlybyTo = FVector(4000.f, 8000.f, -900.f);
		}
		FlybyT = 0.f;
		++Flybys;
		ShowSquad(true);
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
		ShowSquad(false);
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
	if (bRestartTrails)
	{
		// After the ships are at the start of the pass: no streak from where the last pass ended.
		bRestartTrails = false;
		for (UNiagaraComponent* FX : SquadTrails)
		{
			if (FX) { FX->SetVisibility(true); FX->Activate(true); }
		}
	}
}

void ATDMenuBackdrop::ShowSquad(bool bShow)
{
	for (UStaticMeshComponent* C : SquadMeshes)
	{
		if (C) { C->SetVisibility(bShow, true); } // and its parts
	}
	bRestartTrails = bShow;
	if (!bShow)
	{
		for (UNiagaraComponent* FX : SquadTrails)
		{
			if (FX) { FX->Deactivate(); FX->SetVisibility(false); }
		}
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
	ShowSquad(false);
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
	if (StingerBlack.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(StingerBlack.ToSharedRef());
	}
	StingerBlack.Reset();
	Super::EndPlay(EndPlayReason);
}
