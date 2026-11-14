// Triad Drift - World (environment builder). The planet's cloud deck (see TDCloudDeck.h).

#include "TriadDrift/World/TDCloudDeck.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/Core/TDTypes.h"

const FName ATDCloudDeck::CloudDeckTag(TEXT("TD.Engineering.CloudDeck"));

static TAutoConsoleVariable<int32> CVarTDCloudDeck(TEXT("td.CloudDeck"), 1,
	TEXT("Planet cloud deck (environment builder): 1 = on, 0 = none (the look before it). Read when a planet map starts."));

namespace TDCloudDeckImpl
{
	// Ring radii as fractions of DeckRadius: dense near the middle (where the camera is), coarse far out.
	static const float RingFractions[] = { 0.f, 0.02f, 0.045f, 0.075f, 0.11f, 0.16f, 0.22f, 0.3f, 0.4f, 0.52f, 0.66f, 0.82f, 1.f };
	static constexpr int32 Segments = 64;
}

ATDCloudDeck::ATDCloudDeck()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Deck"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->bUseAsyncCooking = true;
	Tags.Add(CloudDeckTag);
}

ATDCloudDeck* ATDCloudDeck::EnsureForWorld(UWorld* World, ETDDestination InPlanet)
{
	if (!World || CVarTDCloudDeck.GetValueOnGameThread() == 0)
	{
		return nullptr;
	}
	for (TActorIterator<ATDCloudDeck> It(World); It; ++It) { if (!It->IsActorBeingDestroyed()) { return *It; } }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATDCloudDeck* D = World->SpawnActor<ATDCloudDeck>(ATDCloudDeck::StaticClass(), FTransform::Identity, Params);
	if (D)
	{
		D->Planet = InPlanet;
	}
	return D;
}

void ATDCloudDeck::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bBuilt)
	{
		SetActorTickEnabled(false);
		return;
	}
	// Wait for the planet terrain to finish its setup at the course origin (it waits for the course itself).
	WaitSeconds += DeltaSeconds;
	bool bTerrainReady = false;
	for (TActorIterator<ATDPlanetTerrain> It(GetWorld()); It; ++It) { bTerrainReady = It->IsBuilt(); break; }
	if (bTerrainReady || WaitSeconds > TerrainWaitSeconds)
	{
		Build();
	}
}

void ATDCloudDeck::Build()
{
	const ATDPlanetTerrain* Terrain = nullptr;
	for (TActorIterator<ATDPlanetTerrain> It(GetWorld()); It; ++It) { if (It->IsBuilt()) { Terrain = *It; } break; }
	const float Alt = GetAltitude();
	// ENG-38: the deck clears the zone's highest ground (T1 arrives 300 m above it); the per-planet altitude is the fallback.
	SetActorLocation(Terrain ? FVector(Terrain->GetActorLocation().X, Terrain->GetActorLocation().Y, Terrain->GetCloudDeckZ()) : FVector(0.f, 0.f, Alt));

	using namespace TDCloudDeckImpl;
	const int32 Rings = UE_ARRAY_COUNT(RingFractions);
	TArray<FVector> V;
	TArray<int32> Tri;
	TArray<FVector> Nrm;
	V.Reserve(1 + (Rings - 1) * Segments);
	V.Add(FVector::ZeroVector);
	for (int32 R = 1; R < Rings; ++R)
	{
		const float Radius = RingFractions[R] * DeckRadius;
		for (int32 S = 0; S < Segments; ++S)
		{
			const float A = 2.f * PI * S / Segments;
			const float X = Radius * FMath::Cos(A), Y = Radius * FMath::Sin(A);
			// The same bend as the terrain's far ring, so the deck and the ground sink under the horizon together.
			V.Add(FVector(X, Y, Terrain ? -Terrain->CurvatureDrop(X, Y) : 0.f));
		}
	}
	for (int32 S = 0; S < Segments; ++S)
	{
		Tri.Append({ 0, 1 + (S + 1) % Segments, 1 + S });
	}
	for (int32 R = 1; R < Rings - 1; ++R)
	{
		const int32 In = 1 + (R - 1) * Segments, Out = 1 + R * Segments;
		for (int32 S = 0; S < Segments; ++S)
		{
			const int32 S1 = (S + 1) % Segments;
			Tri.Append({ In + S, In + S1, Out + S });
			Tri.Append({ In + S1, Out + S1, Out + S });
		}
	}
	Nrm.Init(FVector::UpVector, V.Num());
	Mesh->CreateMeshSection(0, V, Tri, Nrm, TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	const TCHAR* Path = Planet == ETDDestination::Cinder
		? TEXT("/Game/TriadDrift/Gameplay/Props/Sky/MI_CloudDeck_Cinder.MI_CloudDeck_Cinder")
		: TEXT("/Game/TriadDrift/Gameplay/Props/Sky/MI_CloudDeck_Verdance.MI_CloudDeck_Verdance");
	if (UMaterialInterface* M = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Path)).LoadSynchronous())
	{
		Mesh->SetMaterial(0, M);
	}
	else
	{
		Mesh->SetVisibility(false);	// no material (a stripped build): draw nothing rather than a grey disc
	}
	bBuilt = true;
	UE_LOG(LogTriadDrift, Log, TEXT("ATDCloudDeck: %s deck at %.0f m over the pad, %.0f km radius%s"),
		Planet == ETDDestination::Cinder ? TEXT("Cinder") : TEXT("Verdance"), Alt / 100.f, DeckRadius / 100000.f,
		Terrain ? TEXT(", bent with the terrain") : TEXT(" (no terrain: flat)"));
}
