// Triad Drift - zero-asset playable space sandbox (ENG-19). Owner: Teammate 5.

#include "TriadDrift/Flow/TDSpaceGameMode.h"
#include "TriadDrift/Flow/TDJourney.h"
#include "TriadDrift/Core/TDStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDFlowReflection.h"
#include "TriadDrift/Flow/TDCheatManager.h"
#include "TriadDrift/Flow/TDPlaceholderSky.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/PostProcessVolume.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"
#include "TriadDrift/World/TDSkySphere.h"

namespace TDSandbox
{
	static const TCHAR* SphereMesh = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	static const TCHAR* CubeMesh = TEXT("/Engine/BasicShapes/Cube.Cube");
	static const TCHAR* ConeMesh = TEXT("/Engine/BasicShapes/Cone.Cone");
	static const TCHAR* LitMaterial = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");

	static const FName FnSpawnSquad(TEXT("SpawnSquad"));
	static const FName FnGetActiveShip(TEXT("GetActiveShip"));
	static const FName FnStartFromProgress(TEXT("StartFromProgress"));
	static const FName FnStartWave(TEXT("StartWave"));

	template <typename T>
	T* LoadEngineObject(const TCHAR* Path)
	{
		return LoadObject<T>(nullptr, Path, nullptr, LOAD_NoWarn);
	}

	template <typename T>
	bool WorldHasActor(UWorld* World)
	{
		for (TActorIterator<T> It(World); It; ++It)
		{
			return true;
		}
		return false;
	}
}

ATDSpaceGameMode::ATDSpaceGameMode()
{
	// Fallback pawn if the squad manager isn't merged yet: a free-fly camera so the sandbox is usable.
	// InitGame clears it when the squad exists (the squad possesses the lead ship itself).
	DefaultPawnClass = ADefaultPawn::StaticClass();
	SquadOrigin = FTransform(FRotator::ZeroRotator, FVector::ZeroVector);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // after the camera moved this frame
}

void ATDSpaceGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	// Resolve T1's controller before any player logs in.
	if (UClass* PCClass = TDFlowReflection::ResolveClass(UTDFlowSettings::Get()->SpacePlayerControllerClass))
	{
		if (PCClass->IsChildOf(APlayerController::StaticClass()))
		{
			PlayerControllerClass = PCClass;
		}
	}
	else
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Sandbox: ATDSpacePlayerController not found; using the engine PlayerController."));
	}
	if (TDFlowReflection::ResolveClass(UTDFlowSettings::Get()->SquadManagerClass))
	{
		DefaultPawnClass = nullptr; // no login pawn: the player flies the squad's lead ship
	}
	Super::InitGame(MapName, Options, ErrorMessage);
}

void ATDSpaceGameMode::StartPlay()
{
	if (bUseGalaxyLayout)
	{
		// T4's ENG-17 layout: reuses a placed one (L_Galaxy) or spawns the default galaxy. It replaces
		// our placeholder asteroids and spawns the Dominion; return points are valid right away.
		GalaxyLayout = ATDGalaxyLayout::SpawnDefaultGalaxy(this);
		if (GalaxyLayout)
		{
			SquadOrigin = ResolveSquadOrigin();
			UE_LOG(LogTriadDrift, Log, TEXT("Space mode: squad origin at level start %s"), *SquadOrigin.GetLocation().ToCompactString());
		}
	}
	if (bSpawnEnvironment)
	{
		SpawnEnvironment();
	}
	SpawnSquadManager();
	SpawnWaveDirector();

	Super::StartPlay(); // BeginPlay on actors

	// The layout spawns the Dominion in its BeginPlay: the Dominion stage points resolve only now.
	if (GalaxyLayout)
	{
		const FTransform Resolved = ResolveSquadOrigin();
		if (!Resolved.GetLocation().Equals(SquadOrigin.GetLocation(), 1.f) && SquadManager)
		{
			UE_LOG(LogTriadDrift, Log, TEXT("Space mode: squad origin re-resolved after StartPlay %s -> %s"), *SquadOrigin.GetLocation().ToCompactString(), *Resolved.GetLocation().ToCompactString());
			SquadOrigin = Resolved;
			TDFlowReflection::CallTransform(SquadManager, TDSandbox::FnSpawnSquad, SquadOrigin);
		}
	}

	// Local players logged in before StartPlay: hand them the active ship, clear any login pawn.
	HandOverPlayers();

	EnterLevel();
}

void ATDSpaceGameMode::EnterLevel()
{
	// ENG-33: New Game plays the cinematic intro once. The run stays in Intro, and the first waves wait for it.
	// When it does not play (Continue, a retake, td.Intro 0, headless) everything below runs exactly as before.
	bool bIntroPlaying = false;
	if (UTDGameInstance* GI = UTDGameInstance::Get(this); GI && GI->ConsumeIntroPending() && GI->GetProgress().Phase == ETDGamePhase::Intro)
	{
		UTDIntroDirector* Intro = UTDIntroDirector::Get(this);
		bIntroPlaying = Intro && Intro->Play(FSimpleDelegate::CreateWeakLambda(this, [this]() { BeginFirstWaves(); }));
	}

	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		if (!bIntroPlaying && GI->GetProgress().Phase == ETDGamePhase::Intro)
		{
			GI->SetPhase(ETDGamePhase::Waves1to2);
		}
		GI->NotifyLevelEntered(Destination);
		GI->OnSceneRestart.Remove(SceneRestartHandle);
		SceneRestartHandle = GI->OnSceneRestart.AddUObject(this, &ATDSpaceGameMode::HandleSceneRestart);
		if (bIntroPlaying)
		{
			bRunTimerPausedBeforeIntro = GI->IsRunTimerPaused();
			GI->SetRunTimerPaused(true); // the intro is not play time: watched or skipped, the run time is the same
		}
	}
	else
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Sandbox: GameInstanceClass is not UTDGameInstance; progress, checkpoints and takes are off."));
	}

	if (!bIntroPlaying)
	{
		ArmFirstWaveTimer();
	}
}

void ATDSpaceGameMode::BeginFirstWaves()
{
	// The intro's OnFinished (watched or skipped): the same phase change and timer as a run without it.
	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		if (GI->GetProgress().Phase == ETDGamePhase::Intro)
		{
			GI->SetPhase(ETDGamePhase::Waves1to2);
		}
		GI->SetRunTimerPaused(bRunTimerPausedBeforeIntro); // the run time starts now, exactly as without the intro
	}
	ArmFirstWaveTimer();
}

void ATDSpaceGameMode::ArmFirstWaveTimer()
{
	GetWorldTimerManager().SetTimer(FirstWaveTimer, this, &ATDSpaceGameMode::StartWaves, FMath::Max(0.01f, FirstWaveDelaySeconds), false);
}

void ATDSpaceGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		GI->OnSceneRestart.Remove(SceneRestartHandle);
	}
	GetWorldTimerManager().ClearTimer(FirstWaveTimer);
	Super::EndPlay(EndPlayReason);
}

void ATDSpaceGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UTDCheatManager::InstallOn(NewPlayer);
	HandOverPlayers();
}

void ATDSpaceGameMode::ApplyStartingShip()
{
	// Resume (Continue / retake) in the ship the player was last flying.
	ATDSquadManager* Squad = Cast<ATDSquadManager>(SquadManager);
	const UTDGameInstance* GI = UTDGameInstance::Get(this);
	if (Squad && GI)
	{
		Squad->StartingShip = GI->GetProgressRef().LastShip;
	}
}

void ATDSpaceGameMode::HandOverPlayers()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		UTDCheatManager::InstallOn(It->Get());
		if (APlayerController* PC = It->Get(); PC && PC->IsLocalController())
		{
			PC->SetInputMode(FInputModeGameOnly()); // never inherit the menu's UI-only input
			PC->SetShowMouseCursor(false);
		}
		PossessActiveShip(It->Get());
	}
	if (!SquadManager)
	{
		return; // the free-fly pawn is the fallback: keep it
	}
	// The squad possesses the lead ship itself; any login/default pawn left over is a stray that the
	// lead ship would spawn inside on every take.
	TArray<ADefaultPawn*, TInlineAllocator<4>> Strays;
	for (TActorIterator<ADefaultPawn> It(World); It; ++It)
	{
		if (!It->GetController())
		{
			Strays.Add(*It);
		}
	}
	for (ADefaultPawn* Stray : Strays)
	{
		Stray->Destroy();
	}
}

AActor* ATDSpaceGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (AActor* Found = Super::ChoosePlayerStart_Implementation(Player))
	{
		return Found;
	}
	// Empty map (/Engine/Maps/Entry has no PlayerStart): make one behind the squad origin.
	if (!FallbackStart)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector Loc = SquadOrigin.GetLocation() - SquadOrigin.GetRotation().GetForwardVector() * 1500.f;
		FallbackStart = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), Loc, SquadOrigin.Rotator(), Params);
	}
	return FallbackStart;
}

void ATDSpaceGameMode::PossessActiveShip(APlayerController* PC)
{
	if (!PC || !SquadManager)
	{
		return;
	}
	APawn* Ship = Cast<APawn>(TDFlowReflection::CallReturnObject(SquadManager, TDSandbox::FnGetActiveShip));
	if (Ship && PC->GetPawn() != Ship)
	{
		APawn* Old = PC->GetPawn();
		PC->Possess(Ship);
		if (Old && Old->IsA<ADefaultPawn>())
		{
			Old->Destroy();
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Squad and waves (late-bound: T3 / T4)
// ------------------------------------------------------------------------------------------------

void ATDSpaceGameMode::SpawnSquadManager()
{
	UClass* Class = TDFlowReflection::ResolveClass(UTDFlowSettings::Get()->SquadManagerClass);
	if (!Class || !Class->IsChildOf(AActor::StaticClass()))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Sandbox: ATDSquadManager not found; flying a free camera instead."));
		return;
	}
	// Reuse one placed in the map if there is one.
	for (TActorIterator<AActor> It(GetWorld(), Class); It; ++It)
	{
		SquadManager = *It;
		break;
	}
	if (!SquadManager)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SquadManager = GetWorld()->SpawnActor<AActor>(Class, SquadOrigin, Params);
	}
	ApplyStartingShip();
	if (SquadManager && !TDFlowReflection::CallTransform(SquadManager, TDSandbox::FnSpawnSquad, SquadOrigin))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Sandbox: squad manager has no UFUNCTION SpawnSquad(FTransform)."));
	}
}

void ATDSpaceGameMode::SpawnWaveDirector()
{
	UClass* Class = TDFlowReflection::ResolveClass(UTDFlowSettings::Get()->WaveDirectorClass);
	if (!Class || !Class->IsChildOf(AActor::StaticClass()))
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Sandbox: ATDWaveDirector not found; no enemy waves."));
		return;
	}
	for (TActorIterator<AActor> It(GetWorld(), Class); It; ++It)
	{
		WaveDirector = *It;
		break;
	}
	if (!WaveDirector)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		WaveDirector = GetWorld()->SpawnActor<AActor>(Class, SquadOrigin, Params);
	}
}

void ATDSpaceGameMode::StartWaves()
{
	if (!WaveDirector)
	{
		return;
	}
	if (!TDFlowReflection::Call(WaveDirector, TDSandbox::FnStartFromProgress))
	{
		const UTDGameInstance* GI = UTDGameInstance::Get(this);
		TDFlowReflection::CallInt(WaveDirector, TDSandbox::FnStartWave, GI ? GI->GetProgressRef().WavesCleared : 0); // StartWave takes a 0-based table index
	}
}

void ATDSpaceGameMode::HandleSceneRestart(FName SceneId, int32 TakeNumber)
{
	RestartScene();
}

FTransform ATDSpaceGameMode::ResolveSquadOrigin() const
{
	const UTDGameInstance* GI = UTDGameInstance::Get(this);
	FName Point = GI ? GI->GetProgressRef().ReturnPoint : NAME_None;
	if (GI && GI->GetProgressRef().Phase == ETDGamePhase::DominionAssault)
	{
		Point = GI->GetProgressRef().bDominionDefensesDown ? FTDJourney::DominionTrench : FTDJourney::DominionApproach;
	}
	FTransform Where;
	if (FTDJourney::FindStageStart(GetWorld(), Point, Where))
	{
		return Where;
	}
	return GalaxyLayout ? GalaxyLayout->GetPlayerStart() : SquadOrigin;
}

void ATDSpaceGameMode::RestartScene()
{
	// New take: SpawnSquad is T3's retake call (revive all three, reposition, give the player the lead
	// ship). The wave director hears the bus OnTakeStarted for its scene and rebuilds its own wave, so we
	// don't touch waves here.
	if (SquadManager)
	{
		if (GalaxyLayout)
		{
			SquadOrigin = ResolveSquadOrigin(); // the phase may have moved on since level entry (the assault)
		}
		ApplyStartingShip();
		UE_LOG(LogTriadDrift, Log, TEXT("Space mode: retake respawns the squad at %s"), *SquadOrigin.GetLocation().ToCompactString());
		TDFlowReflection::CallTransform(SquadManager, TDSandbox::FnSpawnSquad, SquadOrigin);
	}
	HandOverPlayers();
	// If the retake landed before the first wave ever started, keep the pending first-wave timer.
}

// ------------------------------------------------------------------------------------------------
// Environment (engine basic shapes only)
// ------------------------------------------------------------------------------------------------

void ATDSpaceGameMode::SpawnEnvironment()
{
	UWorld* World = GetWorld();
	// Placeholder sun / fill / sky light / exposure only where the map has none of its own (N-004):
	// Nick's sky actor (tagged TD.Presentation.Sky, or any directional light / unbound post-process)
	// switches them off, also when it streams in later.
	if (ATDPlaceholderSky* Sky = ATDPlaceholderSky::SpawnIfNeeded(World, ETDPlaceholderSkyStyle::Space))
	{
		SpawnedEnvironment.Add(Sky);
	}
	// A real skybox (Nick's, or the galaxy layout's engineering nebula sky) brings its own stars.
	const bool bEngineeringSky = GalaxyLayout && GalaxyLayout->bSpawnSky && ATDSkySphere::IsArtAvailable();
	if (!ATDPlaceholderSky::Scan(World).bTaggedSky && !bEngineeringSky)
	{
		SpawnStarBackdrop();
	}
	if (!GalaxyLayout)
	{
		SpawnAsteroids();
	}
}
void ATDSpaceGameMode::SpawnStarBackdrop()
{
	UStaticMesh* Sphere = TDSandbox::LoadEngineObject<UStaticMesh>(TDSandbox::SphereMesh);
	if (!Sphere || StarCount <= 0)
	{
		return;
	}
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Stars = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!Stars)
	{
		return;
	}
#if WITH_EDITOR
	Stars->SetActorLabel(TEXT("TD_Sandbox_Stars"));
#endif
	USceneComponent* Root = NewObject<USceneComponent>(Stars, TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	Stars->SetRootComponent(Root);
	Root->RegisterComponent();

	// Unlit glow, so every star reads from every direction (a lit sphere is dark on its far side from the sun).
	// Three tints, blue-white / white / warm, each its own instanced layer.
	struct FLayer { const TCHAR* Name; FLinearColor Color; float Share; };
	const FLayer StarLayers[] = {
		{ TEXT("StarsCool"), FLinearColor(0.7f, 0.82f, 1.f), 0.4f },
		{ TEXT("StarsWhite"), FLinearColor(1.f, 1.f, 1.f), 0.4f },
		{ TEXT("StarsWarm"), FLinearColor(1.f, 0.82f, 0.6f), 0.2f },
	};
	FRandomStream Rng(LayoutSeed ^ 0x5EED);
	for (const FLayer& L : StarLayers)
	{
		UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(Stars, L.Name);
		ISM->SetMobility(EComponentMobility::Movable);
		ISM->SetStaticMesh(Sphere);
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISM->SetCastShadow(false);
		ISM->SetCanEverAffectNavigation(false);
		ISM->SetupAttachment(Root);
		ISM->RegisterComponent();
		Stars->AddInstanceComponent(ISM);
		StarGlowMaterial = UTDStatics::GetPlaceholderGlowMaterial(); // instanced-mesh usage, always cooked (Core)
		if (StarGlowMaterial)
		{
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(StarGlowMaterial, Stars))
			{
				MID->SetVectorParameterValue(TEXT("Color"), L.Color * 6.f);
				MID->SetVectorParameterValue(TEXT("EmissiveColor"), L.Color * 6.f);
				ISM->SetMaterial(0, MID);
			}
		}
		else
		{
			UTDStatics::TintPlaceholder(ISM, L.Color, 6.f); // path load fallback (editor / tests)
		}
		const int32 Count = FMath::Max(1, FMath::RoundToInt(StarCount * L.Share));
		TArray<FTransform> Instances;
		Instances.Reserve(Count);
		for (int32 i = 0; i < Count; ++i)
		{
			const FVector Dir = Rng.GetUnitVector();
			const float Dist = StarSphereRadius * Rng.FRandRange(0.85f, 1.f);
			// Mostly pinpricks; a few big bright ones.
			const float Size = (Rng.FRand() < 0.06f) ? Rng.FRandRange(14.f, 26.f) : Rng.FRandRange(3.f, 9.f);
			Instances.Emplace(FRotator::ZeroRotator, Dir * Dist, FVector(Size)); // relative: the actor follows the camera
		}
		ISM->AddInstances(Instances, false, false);
	}
	Stars->SetActorLocation(SquadOrigin.GetLocation());
	Stars->Tags.Add(TEXT("TD.Placeholder.Stars"));
	SpawnedEnvironment.Add(Stars);
	StarBackdrop = Stars;
	GetWorldTimerManager().SetTimer(StarWatchTimer, this, &ATDSpaceGameMode::CheckStarBackdrop, 0.5f, true);
}

void ATDSpaceGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (StarBackdrop)
	{
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC && PC->PlayerCameraManager)
		{
			StarBackdrop->SetActorLocation(PC->PlayerCameraManager->GetCameraLocation());
		}
	}
}

void ATDSpaceGameMode::CheckStarBackdrop()
{
	if (!StarBackdrop || StarBackdrop->IsActorBeingDestroyed())
	{
		GetWorldTimerManager().ClearTimer(StarWatchTimer);
		return;
	}
	if (ATDPlaceholderSky::Scan(GetWorld()).bTaggedSky)
	{
		StarBackdrop->Destroy();
		StarBackdrop = nullptr;
		GetWorldTimerManager().ClearTimer(StarWatchTimer);
	}
}

void ATDSpaceGameMode::SpawnAsteroids()
{
	if (AsteroidCount <= 0)
	{
		return;
	}
	UStaticMesh* Meshes[] = {
		TDSandbox::LoadEngineObject<UStaticMesh>(TDSandbox::SphereMesh),
		TDSandbox::LoadEngineObject<UStaticMesh>(TDSandbox::CubeMesh),
		TDSandbox::LoadEngineObject<UStaticMesh>(TDSandbox::ConeMesh),
	};
	UMaterialInterface* BaseMat = TDSandbox::LoadEngineObject<UMaterialInterface>(TDSandbox::LitMaterial);

	UWorld* World = GetWorld();
	FRandomStream Rng(LayoutSeed);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Rock palette: dusky violets and teals so they read against the black and don't look like enemies.
	const FLinearColor Palette[] = {
		FLinearColor(0.18f, 0.14f, 0.24f), FLinearColor(0.12f, 0.20f, 0.22f), FLinearColor(0.24f, 0.20f, 0.18f),
	};

	for (int32 i = 0; i < AsteroidCount; ++i)
	{
		UStaticMesh* Mesh = Meshes[Rng.RandRange(0, UE_ARRAY_COUNT(Meshes) - 1)];
		if (!Mesh)
		{
			continue;
		}
		const FVector Loc = SquadOrigin.GetLocation() + Rng.GetUnitVector() * Rng.FRandRange(AsteroidRadiusRange.X, AsteroidRadiusRange.Y);
		const FRotator Rot(Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f));
		const float Base = Rng.FRandRange(AsteroidScaleRange.X, AsteroidScaleRange.Y);
		const FVector Scale(Base * Rng.FRandRange(0.7f, 1.3f), Base * Rng.FRandRange(0.7f, 1.3f), Base * Rng.FRandRange(0.7f, 1.3f));

		AStaticMeshActor* Rock = World->SpawnActorDeferred<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(Rot, Loc, Scale));
		if (!Rock)
		{
			continue;
		}
		UStaticMeshComponent* SMC = Rock->GetStaticMeshComponent();
		SMC->SetMobility(EComponentMobility::Movable);
		SMC->SetStaticMesh(Mesh);
		SMC->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Rock->FinishSpawning(FTransform(Rot, Loc, Scale));
		if (BaseMat)
		{
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, Rock))
			{
				MID->SetVectorParameterValue(TEXT("Color"), Palette[i % UE_ARRAY_COUNT(Palette)]);
				SMC->SetMaterial(0, MID);
			}
		}
#if WITH_EDITOR
		Rock->SetActorLabel(FString::Printf(TEXT("TD_Sandbox_Asteroid_%02d"), i));
#endif
		SpawnedEnvironment.Add(Rock);
	}
}
