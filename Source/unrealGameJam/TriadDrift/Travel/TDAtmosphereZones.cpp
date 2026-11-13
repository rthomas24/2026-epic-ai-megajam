// Triad Drift - atmosphere entry and atmosphere flight zone (ENG-20). Owner: Teammate 1.

#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/World/TDTerrainField.h"
#include "EngineUtils.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"

namespace
{
	/** Every live squad ship (player team), without GetAllActorsOfClass. */
	void TDAtmosphere_GetSquadShips(const UObject* Context, TArray<ATDShip*>& Out)
	{
		Out.Reset();
		if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(Context))
		{
			TArray<AActor*> Actors;
			Registry->GetActorsOfTeam(ETDTeam::Player, Actors, true);
			for (AActor* Actor : Actors)
			{
				if (ATDShip* Ship = Cast<ATDShip>(Actor))
				{
					Out.Add(Ship);
				}
			}
		}
	}
}

// ====================================================================== UTDAtmosphereEntryComponent

const FName UTDAtmosphereEntryComponent::OverrideOwner(TEXT("AtmosphereEntry"));

UTDAtmosphereEntryComponent::UTDAtmosphereEntryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTDAtmosphereEntryComponent::BeginPlay()
{
	Super::BeginPlay();
	if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(GetOwner()))
	{
		if (UTDHealthComponent* Health = Craft->GetHealth())
		{
			Health->OnDeath.AddDynamic(this, &UTDAtmosphereEntryComponent::HandleOwnerDeath);
		}
	}
	// A CUT retake restarts the scene: an entry in progress is abandoned (no travel, flight handed back).
	if (UTDGameInstance* GI = GetWorld() ? Cast<UTDGameInstance>(GetWorld()->GetGameInstance()) : nullptr)
	{
		SceneRestartHandle = GI->OnSceneRestart.AddWeakLambda(this, [this](FName, int32) { AbortEntry(); });
	}
	if (!TravelFX)
	{
		TravelFX = UTDTravelSubsystem::GetDefaultTravelFX(this);
	}
}

float UTDAtmosphereEntryComponent::GetEntry01() const
{
	if (EntryState == ETDAtmosphereEntryState::Idle)
	{
		return 0.f;
	}
	return EntrySeconds > 0.f ? FMath::Clamp(EntryTime / EntrySeconds, 0.f, 1.f) : 1.f;
}

bool UTDAtmosphereEntryComponent::BeginEntry(ETDDestination InDestination, FName InReturnPoint, FVector PlanetCenter, float PlanetSurfaceRadius)
{
	ATDSpacecraft* Craft = Cast<ATDSpacecraft>(GetOwner());
	if (!Craft || EntryState != ETDAtmosphereEntryState::Idle)
	{
		return false;
	}
	// Only the ship the player flies takes the squad down; never twice, never over a pending travel / retake.
	const ATDShip* AsShip = Cast<ATDShip>(Craft);
	if (!AsShip || !AsShip->IsPlayerFlown())
	{
		return false;
	}
	if (const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		if (!Travel->CanRequestLevelTravel())
		{
			return false;
		}
	}
	if (const UTDHealthComponent* Health = Craft->GetHealth())
	{
		if (Health->IsDead())
		{
			return false;
		}
	}
	EntryState = ETDAtmosphereEntryState::Entering;
	EntryTime = 0.f;
	Destination = InDestination;
	ReturnPoint = InReturnPoint;

	if (ATDShip* Ship = Cast<ATDShip>(Craft))
	{
		if (UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
		{
			Hyper->SetHyperdriveEnabled(false);
		}
	}
	// Committed: point into the planet if we're grazing it backwards, then hold speed with reduced steering.
	const FVector ToPlanet = (PlanetCenter - Craft->GetActorLocation()).GetSafeNormal();
	if (!ToPlanet.IsNearlyZero() && FVector::DotProduct(ToPlanet, Craft->GetActorForwardVector()) < 0.f)
	{
		Craft->TeleportSpacecraft(Craft->GetActorLocation(), ToPlanet.Rotation(), true);
	}
	// Don't dive into the planet body before the level change: the speed is limited so the committed
	// path ends EntrySurfaceClearance above the surface.
	float Speed = EntrySpeed;
	if (PlanetSurfaceRadius > 0.f && EntrySeconds > 0.f)
	{
		const float ToSurface = FVector::Dist(PlanetCenter, Craft->GetActorLocation()) - PlanetSurfaceRadius - EntrySurfaceClearance;
		Speed = FMath::Clamp(ToSurface / EntrySeconds, EntryMinSpeed, EntrySpeed);
	}
	if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
	{
		Move->SetSpeedOverrideFor(OverrideOwner, Speed, EntrySteeringScale);
	}
	if (UTDHealthComponent* Health = Craft->GetHealth())
	{
		Health->SetInvulnerable(true); // committed: nothing should kill the ship during the white-out
	}
	if (TravelFX)
	{
		UAudioComponent* Audio = nullptr;
		EntryFX = UTDStatics::PlayCueAttachedLoop(TravelFX->AtmosphericEntry, Craft->GetRootComponent(), NAME_None, Audio);
		EntryAudio = Audio;
	}
	OnEntryStarted.Broadcast(this);
	if (EntrySeconds <= 0.f)
	{
		FinishEntry();
	}
	return true;
}

void UTDAtmosphereEntryComponent::FinishEntry()
{
	EntryState = ETDAtmosphereEntryState::Travelling;
	ATDSpacecraft* EnteringCraft = Cast<ATDSpacecraft>(GetOwner());
	OnEntryCompleted.Broadcast(this);

	bool bTravelled = false;
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		bTravelled = Travel->RequestLevelTravel(Destination, ReturnPoint); // once per level, never over a retake
	}
	if (!bTravelled)
	{
		// No flow (tests, sandbox) or the planet map is missing: hand the ship back.
		UE_LOG(LogTriadDrift, Warning, TEXT("Atmosphere entry: TravelTo(%s) did not happen; returning control."),
			*StaticEnum<ETDDestination>()->GetNameStringByValue((int64)Destination));
		RestoreFlight();
		EntryState = ETDAtmosphereEntryState::Idle;
	}
}

void UTDAtmosphereEntryComponent::StopEntryCue()
{
	if (UNiagaraComponent* FX = EntryFX.Get()) { FX->Deactivate(); }
	if (UAudioComponent* Audio = EntryAudio.Get()) { Audio->FadeOut(0.3f, 0.f); }
	EntryFX.Reset();
	EntryAudio.Reset();
}

void UTDAtmosphereEntryComponent::RestoreFlight()
{
	StopEntryCue(); // aborted or travel failed: the roar and plasma must stop
	if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(GetOwner()))
	{
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			Move->ClearSpeedOverrideFor(OverrideOwner);
		}
		if (UTDHealthComponent* Health = Craft->GetHealth())
		{
			Health->SetInvulnerable(false);
		}
		if (ATDShip* Ship = Cast<ATDShip>(Craft))
		{
			if (UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
			{
				Hyper->SetHyperdriveEnabled(true);
			}
		}
	}
}

void UTDAtmosphereEntryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDGameInstance* GI = GetWorld() ? Cast<UTDGameInstance>(GetWorld()->GetGameInstance()) : nullptr)
	{
		GI->OnSceneRestart.Remove(SceneRestartHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UTDAtmosphereEntryComponent::AbortEntry()
{
	if (EntryState == ETDAtmosphereEntryState::Entering)
	{
		RestoreFlight();
		EntryState = ETDAtmosphereEntryState::Idle;
		EntryTime = 0.f;
	}
}

void UTDAtmosphereEntryComponent::HandleOwnerDeath(UTDHealthComponent* HealthComp, const FTDDamageInfo& KillingBlow)
{
	AbortEntry(); // a kill that bypasses invulnerability (cheat, script) must not travel or leave a stuck state
}

void UTDAtmosphereEntryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (EntryState == ETDAtmosphereEntryState::Entering)
	{
		EntryTime += DeltaTime;
		if (EntryTime >= EntrySeconds)
		{
			FinishEntry();
		}
	}
	else if (EntryState == ETDAtmosphereEntryState::Travelling)
	{
		// The load was voided or timed out and we are still here (review minor 6): give the ship back
		// (override, invulnerability, hyperdrive, switch lock) so the player can fly in again.
		const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this);
		if (!Travel || !Travel->HasRequestedLevelTravel())
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("Atmosphere entry: the level travel fell through; returning control."));
			RestoreFlight();
			EntryState = ETDAtmosphereEntryState::Idle;
			EntryTime = 0.f;
		}
	}
}

// ========================================================================== ATDAtmosphereFlightZone

ATDAtmosphereFlightZone::ATDAtmosphereFlightZone()
{
	PrimaryActorTick.bCanEverTick = true;
	ZoneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBox"));
	ZoneBox->SetBoxExtent(FVector(300000.f, 300000.f, 80000.f));
	ZoneBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // the flight model does the walls
	ZoneBox->ShapeColor = FColor(120, 255, 140);
	RootComponent = ZoneBox;
}

FBox ATDAtmosphereFlightZone::GetZoneBox() const
{
	const FVector Extent = ZoneBox ? ZoneBox->GetScaledBoxExtent() : FVector::ZeroVector;
	const FVector Center = GetActorLocation();
	return FBox(Center - Extent, Center + Extent);
}

void ATDAtmosphereFlightZone::SetZoneExtent(const FVector& Extent)
{
	if (ZoneBox)
	{
		ZoneBox->SetBoxExtent(Extent.ComponentMax(FVector(100.f)));
	}
}

bool ATDAtmosphereFlightZone::IsShipInZone(const ATDShip* Ship) const
{
	return Ship && GetZoneBox().ExpandBy(FVector(0.f, 0.f, ExitHeightAboveCeiling * 4.f)).IsInsideOrOn(Ship->GetActorLocation());
}

bool ATDAtmosphereFlightZone::IsExitAllowed() const
{
	if (bExitOverride)
	{
		return bExitAllowedOverride;
	}
	if (RequiredObjectives.Num() == 0)
	{
		return true;
	}
	auto AllDone = [this](const FTDProgress& Progress)
	{
		for (const FName& Id : RequiredObjectives)
		{
			if (!Progress.CompletedObjectives.Contains(Id))
			{
				return false;
			}
		}
		return true;
	};
	// Every frame: read the game instance's progress in place instead of copying it (review minor 8).
	const UWorld* World = GetWorld();
	if (const UTDGameInstance* GI = World ? Cast<UTDGameInstance>(World->GetGameInstance()) : nullptr)
	{
		return AllDone(GI->GetProgressRef());
	}
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		return AllDone(Flow->GetProgress());
	}
	return false;
}

void ATDAtmosphereFlightZone::ApplyToShip(ATDShip* Ship, bool bExitOpen)
{
	UTDShipMovementComponent* Move = Ship->GetShipMovement();
	if (!Move)
	{
		return;
	}
	FBox Box = GetZoneBox();
	if (bExitOpen)
	{
		Box.Max.Z += ExitHeightAboveCeiling * 4.f; // let the player climb through the ceiling
	}
	if (!Move->IsInAtmosphere())
	{
		Move->SetAtmosphereMode(true, Gravity, SpeedCapFraction);
		if (UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
		{
			Hyper->CancelSpool();
		}
	}
	Move->SetBoxBoundary(Box);
	if (ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(Ship->GetController()))
	{
		PC->SetFlightContext(ETDFlightContext::Atmosphere);
	}
	ManagedShips.AddUnique(Ship);
}

void ATDAtmosphereFlightZone::FitToTerrain(const ATDPlanetTerrain* Terrain)
{
	const FTDTerrainField* Field = Terrain ? Terrain->GetField() : nullptr;
	if (!Field)
	{
		return;
	}
	const float Half = FMath::Max(10000.f, Field->GetStyle().CoreHalfSize - TerrainEdgeMargin);
	const FVector O = Terrain->GetActorLocation();
	// Highest ground in the zone (a coarse grid: ~200 m spacing; the clearance covers what falls between samples).
	float MaxZ = O.Z;
	constexpr int32 N = 80;
	for (int32 iy = 0; iy <= N; ++iy)
	{
		for (int32 ix = 0; ix <= N; ++ix)
		{
			const FVector P(O.X - Half + 2.f * Half * ix / N, O.Y - Half + 2.f * Half * iy / N, O.Z);
			MaxZ = FMath::Max(MaxZ, Terrain->GetHeightAt(P));
		}
	}
	FBox Box = GetZoneBox();
	Box.Min.X = FMath::Min(Box.Min.X, O.X - Half);
	Box.Min.Y = FMath::Min(Box.Min.Y, O.Y - Half);
	Box.Max.X = FMath::Max(Box.Max.X, O.X + Half);
	Box.Max.Y = FMath::Max(Box.Max.Y, O.Y + Half);
	Box.Max.Z = FMath::Max(Box.Max.Z, MaxZ + TerrainCeilingClearance);
	SetActorLocation(Box.GetCenter());
	SetZoneExtent(Box.GetExtent());
	bFittedToTerrain = true;
	UE_LOG(LogTriadDrift, Log, TEXT("TD travel: flight zone fitted to the terrain: %.1f x %.1f km, ground up to %.0f m, ceiling %.0f m (pad-relative heights from the zone floor)"),
		Box.GetSize().X / 100000.f, Box.GetSize().Y / 100000.f, (MaxZ - O.Z) / 100.f, (Box.Max.Z - O.Z) / 100.f);
}

void ATDAtmosphereFlightZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TerrainPoll -= DeltaSeconds;
	if (!bFittedToTerrain && TerrainPoll <= 0.f)
	{
		TerrainPoll = 1.f;
		for (TActorIterator<ATDPlanetTerrain> It(GetWorld()); It; ++It)
		{
			FitToTerrain(*It);
			break;
		}
	}
	const bool bExitOpen = IsExitAllowed();
	const FBox Box = GetZoneBox();

	// A travel that was voided or timed out frees the exit for another climb-out (review minor 6).
	if (bExitTriggered)
	{
		const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this);
		if (Travel && !Travel->HasRequestedLevelTravel() && Travel->GetWorld() && Cast<UTDGameInstance>(Travel->GetWorld()->GetGameInstance()))
		{
			bExitTriggered = false;
		}
	}

	TArray<ATDShip*> Ships;
	TDAtmosphere_GetSquadShips(this, Ships);
	for (ATDShip* Ship : Ships)
	{
		if (!IsShipInZone(Ship))
		{
			continue;
		}
		ApplyToShip(Ship, bExitOpen);

		if (!Ship->IsPlayerFlown() || bExitTriggered)
		{
			continue;
		}
		const bool bLanding = Ship->GetLanding() && Ship->GetLanding()->IsBusy();
		const float Z = Ship->GetActorLocation().Z;
		if (bExitOpen && !bLanding && Z >= Box.Max.Z + ExitHeightAboveCeiling)
		{
			// Latched only on a successful request (review minor 6); a refused request retries next frame.
			UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this);
			const bool bRequested = Travel && Travel->RequestLevelTravel(ETDDestination::Galaxy, FName(*FString::Printf(TEXT("%s_Orbit"), *PlanetId.ToString())));
			// No game flow (tests, sandbox): still report the exit, as the entry does.
			if (bRequested || !Travel || !UTDStatics::GetGameFlow(this))
			{
				bExitTriggered = true;
				OnExitTriggered.Broadcast(Ship);
			}
		}
		else if (!bExitOpen && Z >= Box.Max.Z - 500.f)
		{
			if (!bTipShown && CeilingBlockedTip != NAME_None)
			{
				bTipShown = true;
				if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
				{
					Flow->ShowTip(CeilingBlockedTip);
				}
			}
		}
		else if (Z < Box.Max.Z - 5000.f)
		{
			bTipShown = false; // show it again on the next approach
		}
	}
}
