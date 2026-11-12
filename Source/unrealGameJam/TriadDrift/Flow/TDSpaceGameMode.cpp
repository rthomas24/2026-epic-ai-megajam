// Triad Drift - zero-asset playable space sandbox (ENG-19). Owner: Teammate 5.

#include "TriadDrift/Flow/TDSpaceGameMode.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDFlowReflection.h"
#include "TriadDrift/Flow/TDCheatManager.h"
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
#include "EngineUtils.h"
#include "TimerManager.h"

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
			const UTDGameInstance* GI = UTDGameInstance::Get(this);
			FTransform Where;
			SquadOrigin = (GI && GalaxyLayout->FindReturnPoint(GI->GetProgressRef().ReturnPoint, Where)) ? Where : GalaxyLayout->GetPlayerStart();
		}
	}
	if (bSpawnEnvironment)
	{
		SpawnEnvironment();
	}
	SpawnSquadManager();
	SpawnWaveDirector();

	Super::StartPlay(); // BeginPlay on actors

	// Local players logged in before StartPlay: hand them the active ship, clear any login pawn.
	HandOverPlayers();

	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		if (GI->GetProgress().Phase == ETDGamePhase::Intro)
		{
			GI->SetPhase(ETDGamePhase::Waves1to2);
		}
		GI->NotifyLevelEntered(Destination);
		SceneRestartHandle = GI->OnSceneRestart.AddUObject(this, &ATDSpaceGameMode::HandleSceneRestart);
	}
	else
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Sandbox: GameInstanceClass is not UTDGameInstance; progress, checkpoints and takes are off."));
	}

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

void ATDSpaceGameMode::RestartScene()
{
	// New take: SpawnSquad is T3's retake call (revive all three, reposition, give the player the lead
	// ship). The wave director hears the bus OnTakeStarted for its scene and rebuilds its own wave, so we
	// don't touch waves here.
	if (SquadManager)
	{
		ApplyStartingShip();
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
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (!TDSandbox::WorldHasActor<ADirectionalLight>(World))
	{
		// Key light: a warm "sun".
		ADirectionalLight* Sun = World->SpawnActorDeferred<ADirectionalLight>(ADirectionalLight::StaticClass(), FTransform(FRotator(-35.f, 40.f, 0.f)));
		if (Sun)
		{
			Sun->GetRootComponent()->SetMobility(EComponentMobility::Movable);
			Sun->FinishSpawning(FTransform(FRotator(-35.f, 40.f, 0.f)));
			if (UDirectionalLightComponent* L = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
			{
				L->SetIntensity(8.f);
				L->SetForwardShadingPriority(1); // the sun owns forward shading
				L->SetLightColor(FLinearColor(1.f, 0.93f, 0.82f));
			}
			SpawnedEnvironment.Add(Sun);
		}
		// Fill light from the opposite side, nebula-tinted, so the dark side of ships stays readable.
		ADirectionalLight* Fill = World->SpawnActorDeferred<ADirectionalLight>(ADirectionalLight::StaticClass(), FTransform(FRotator(25.f, 220.f, 0.f)));
		if (Fill)
		{
			Fill->GetRootComponent()->SetMobility(EComponentMobility::Movable);
			Fill->FinishSpawning(FTransform(FRotator(25.f, 220.f, 0.f)));
			if (UDirectionalLightComponent* L = Cast<UDirectionalLightComponent>(Fill->GetLightComponent()))
			{
				L->SetIntensity(1.5f);
				L->SetForwardShadingPriority(0); // fill never competes with the sun for forward shading
				L->SetLightColor(FLinearColor(0.55f, 0.35f, 1.f));
				L->SetCastShadows(false);
				L->SetAtmosphereSunLight(false);
			}
			SpawnedEnvironment.Add(Fill);
		}
	}

	if (!TDSandbox::WorldHasActor<ASkyLight>(World))
	{
		ASkyLight* Sky = World->SpawnActorDeferred<ASkyLight>(ASkyLight::StaticClass(), FTransform::Identity);
		if (Sky)
		{
			Sky->GetRootComponent()->SetMobility(EComponentMobility::Movable);
			Sky->FinishSpawning(FTransform::Identity);
			if (USkyLightComponent* L = Sky->GetLightComponent())
			{
				L->SetIntensity(1.f);
				L->bLowerHemisphereIsBlack = false;
				L->SetLightColor(FLinearColor(0.6f, 0.7f, 1.f));
				L->RecaptureSky();
			}
			SpawnedEnvironment.Add(Sky);
		}
	}

	if (!TDSandbox::WorldHasActor<APostProcessVolume>(World))
	{
		// Fixed exposure: auto exposure in a black void pumps badly when bright explosions appear.
		APostProcessVolume* PP = World->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FTransform::Identity, Params);
		if (PP)
		{
			PP->bUnbound = true;
			FPostProcessSettings& S = PP->Settings;
			S.bOverride_AutoExposureMethod = true;
			S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
			S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
			S.AutoExposureApplyPhysicalCameraExposure = false;
			S.bOverride_AutoExposureBias = true;
			S.AutoExposureBias = 0.f;
			S.bOverride_BloomIntensity = true;
			S.BloomIntensity = 1.2f;
			SpawnedEnvironment.Add(PP);
		}
	}

	SpawnStarBackdrop();
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
	UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(Stars, TEXT("Stars"));
	ISM->SetMobility(EComponentMobility::Movable);
	ISM->SetStaticMesh(Sphere);
	ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ISM->SetCastShadow(false);
	ISM->SetCanEverAffectNavigation(false);
	// BasicShapeMaterial (cooked with /Engine/BasicShapes), tinted near-white and very bright.
	if (UMaterialInterface* Mat = TDSandbox::LoadEngineObject<UMaterialInterface>(TDSandbox::LitMaterial))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, Stars))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(4.f, 4.f, 5.f));
			ISM->SetMaterial(0, MID);
		}
	}
	Stars->SetRootComponent(ISM);
	ISM->RegisterComponent();

	FRandomStream Rng(LayoutSeed ^ 0x5EED);
	TArray<FTransform> Instances;
	Instances.Reserve(StarCount);
	for (int32 i = 0; i < StarCount; ++i)
	{
		const FVector Dir = Rng.GetUnitVector();
		const float Dist = StarSphereRadius * Rng.FRandRange(0.85f, 1.f);
		// Most stars are pinpricks; a few are big bright ones.
		const float Size = (Rng.FRand() < 0.06f) ? Rng.FRandRange(14.f, 26.f) : Rng.FRandRange(3.f, 9.f);
		Instances.Emplace(FRotator::ZeroRotator, SquadOrigin.GetLocation() + Dir * Dist, FVector(Size));
	}
	ISM->AddInstances(Instances, false, true);
	SpawnedEnvironment.Add(Stars);
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
