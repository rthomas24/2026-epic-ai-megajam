// Triad Drift - Squad (Teammate 3). Cross-area calls, isolated here (see TDSquadIntegration.h).

#include "TriadDrift/Squad/TDSquadIntegration.h"
#include "TriadDrift/Enemies/TDArmadaForesight.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/TDWingmanAIController.h"

#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDPresentation.h"

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "Camera/CameraComponent.h"

#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"

#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "TriadDrift/World/TDPortalGate.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Director/TDDirectorCamera.h"
#include "EngineUtils.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"

#include "TriadDrift/Input/TDSpacePlayerController.h" // ETDPowerInput
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Powers/TDAbilityComponent.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Powers/TDBastionPowers.h"

namespace TDSquadIntegrationPrivate
{
	static UTDWeaponComponent* Weapons(const ATDShip* Ship)
	{
		return Ship ? Ship->FindComponentByClass<UTDWeaponComponent>() : nullptr;
	}

	static UTDHealthComponent* Health(const AActor* Actor)
	{
		return Actor ? UTDStatics::GetHealth(Actor) : nullptr;
	}

	static UTDShipMovementComponent* Movement(const ATDShip* Ship)
	{
		return Ship ? Ship->GetShipMovement() : nullptr;
	}

}

using namespace TDSquadIntegrationPrivate;

namespace TDSquad
{
	// ------------------------------------------------------------------------------------ setup

	void ApplySpec(ATDShip* Ship, const FTDShipSpec& Spec)
	{
		if (!Ship)
		{
			return;
		}
		Ship->SetShipId(Spec.Ship);
		Ship->SetTeam(ETDTeam::Player);

		if (UTDShipMovementComponent* Move = Movement(Ship))
		{
			// Keep T1's feel defaults; override only the ENG-04 per-ship numbers.
			FTDFlightTuning Tuning = Move->GetFlightTuning();
			Tuning.MinSpeed = Spec.MinSpeed;
			Tuning.CruiseSpeed = Spec.CruiseSpeed;
			Tuning.BoostSpeed = Spec.BoostSpeed;
			Tuning.TurnRate = Spec.TurnRateDeg;
			Tuning.RollRate = Spec.RollRateDeg;
			Tuning.BoostDuration = Spec.BoostMeterSeconds;
			Tuning.BoostRegenDelay = Spec.BoostRegenDelay;
			Tuning.BoostRegenTime = Spec.BoostRegenSeconds;
			// T1's ship-feel A/B (TD.Flight.ShipFeel, OFF by default): per-ship handling weight, else the shared defaults.
			const FTDFlightTuning Shared;
			Tuning.Acceleration = ATDShip::bPerShipFeel ? Spec.FeelAcceleration : Shared.Acceleration;
			Tuning.AngularResponse = ATDShip::bPerShipFeel ? Spec.FeelAngularResponse : Shared.AngularResponse;
			Tuning.DriftTurnMultiplier = ATDShip::bPerShipFeel ? Spec.FeelDriftTurnMultiplier : Shared.DriftTurnMultiplier;
			Tuning.DodgeCooldown = ATDShip::bPerShipFeel ? Spec.FeelDodgeCooldown : Shared.DodgeCooldown;
			Ship->ApplyFlightTuning(Tuning);
		}

		if (UTDHealthComponent* H = Health(Ship))
		{
			H->InitMaxValues(Spec.MaxHull, Spec.MaxShield, /*bRefill*/ true);
			H->ShieldRegenDelay = Spec.ShieldRegenDelay;
			H->ShieldRegenFractionPerSecond = Spec.ShieldRegenFractionPerSecond;
		}

		UTDWeaponComponent* W = Weapons(Ship);
		if (!W)
		{
			// Until T1 adds the "Weapons" default subobject to ATDSpacecraft, add one at runtime.
			W = NewObject<UTDWeaponComponent>(Ship, TEXT("Weapons"));
			if (W)
			{
				Ship->AddInstanceComponent(W);
				W->RegisterComponent();
			}
		}
		if (W)
		{
			W->EquipWeapon(ETDWeaponSlot::Primary, Spec.PrimaryWeapon);
			W->EquipWeapon(ETDWeaponSlot::Secondary, Spec.SecondaryWeapon);
		}
		else
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("Squad: %s has no UTDWeaponComponent; weapons not equipped"), *GetNameSafe(Ship));
		}

		InstallPowers(Ship, Spec);

		// M2: Nick's visuals (hides the placeholder and tints from the asset); null = placeholder look.
		Ship->SetShipVisuals(UTDShipLibrary::LoadVisuals(Spec));
	}

	void SetPlayerControlled(ATDShip* Ship, bool bPlayerControlled)
	{
		if (Ship)
		{
			Ship->SetPlayerControlled(bPlayerControlled);
			if (bPlayerControlled)
			{
				Ship->SnapCamera(); // the switch is a hard cut (C-03 #1)
			}
		}
	}

	// ----------------------------------------------------------------------------------- flight

	FVector GetVelocity(const ATDShip* Ship)
	{
		const UTDShipMovementComponent* Move = Movement(Ship);
		return Move ? Move->Velocity : FVector::ZeroVector;
	}

	void SetVelocity(ATDShip* Ship, const FVector& Velocity)
	{
		if (UTDShipMovementComponent* Move = Movement(Ship))
		{
			Move->SetVelocityImmediate(Velocity);
		}
	}

	float GetSpeed(const ATDShip* Ship)
	{
		const UTDShipMovementComponent* Move = Movement(Ship);
		return Move ? Move->GetSpeed() : 0.f;
	}

	void GetSpeedRange(const ATDShip* Ship, float& OutMin, float& OutCruise, float& OutBoost)
	{
		if (const UTDShipMovementComponent* Move = Movement(Ship))
		{
			const FTDFlightTuning& T = Move->GetFlightTuning();
			OutMin = T.MinSpeed;
			OutCruise = T.CruiseSpeed;
			OutBoost = T.BoostSpeed;
			return;
		}
		OutMin = 1500.f;
		OutCruise = 6000.f;
		OutBoost = 11000.f;
	}

	void SetCatchUpTopSpeed(ATDShip* Ship, float TopSpeed)
	{
		if (UTDShipMovementComponent* Move = Movement(Ship))
		{
			if (TopSpeed > Move->GetFlightTuning().CruiseSpeed)
			{
				Move->SetThrottleTopSpeed(TopSpeed);
			}
			else
			{
				Move->ClearThrottleTopSpeed();
			}
		}
	}

	void SendFlightInput(ATDShip* Ship, float Pitch, float Yaw, float Roll, float TargetSpeed, bool bForceBoost)
	{
		if (!Ship)
		{
			return;
		}
		float MinS, CruiseS, BoostS;
		GetSpeedRange(Ship, MinS, CruiseS, BoostS);
		// Throttle +1 flies at the top speed (cruise, or the catch-up allowance when one is set).
		UTDShipMovementComponent* Move = Movement(Ship);
		const float TopS = Move ? FMath::Max(CruiseS, Move->GetThrottleTopSpeed()) : CruiseS;

		FTDFlightInput In;
		In.Aim = FVector2D(FMath::Clamp(Yaw, -1.f, 1.f), FMath::Clamp(Pitch, -1.f, 1.f));
		In.Roll = FMath::Clamp(Roll, -1.f, 1.f);

		// Throttle -1..+1 maps linearly to MinSpeed..TopSpeed (T1). Beyond that only boost helps.
		const float Range = FMath::Max(1.f, TopS - MinS);
		const float Throttle01 = FMath::Clamp((TargetSpeed - MinS) / Range, 0.f, 1.f);
		In.Throttle = Throttle01 * 2.f - 1.f;
		In.bBoost = bForceBoost || TargetSpeed > TopS + 0.15f * FMath::Max(0.f, BoostS - TopS);
		In.bBrake = TargetSpeed < MinS * 0.9f;
		Ship->SetFlightInput(In);
	}

	void SetFlightParked(ATDShip* Ship, bool bParked)
	{
		if (Ship)
		{
			Ship->SetFlightParked(bParked);
		}
	}

	bool IsFlightParked(const ATDShip* Ship)
	{
		return Ship && Ship->IsFlightParked();
	}

	void ResetFlightState(ATDShip* Ship, bool bKeepVelocity)
	{
		if (Ship)
		{
			Ship->ResetFlightState(bKeepVelocity);
		}
	}

	void SendNeutralInput(ATDShip* Ship)
	{
		if (Ship)
		{
			Ship->SetFlightInput(FTDFlightInput());
		}
	}

	ETDHyperdriveState GetHyperdriveState(const ATDShip* Ship)
	{
		const UTDHyperdriveComponent* H = Ship ? Ship->GetHyperdrive() : nullptr;
		return H ? H->GetState() : ETDHyperdriveState::Idle;
	}

	bool IsLandingBusy(const ATDShip* Ship)
	{
		const UTDLandingComponent* L = Ship ? Ship->GetLanding() : nullptr;
		return L && L->IsBusy();
	}

	bool IsSwitchLocked(const ATDShip* Ship, FText& OutReason)
	{
		return Ship && Ship->IsSwitchLocked(OutReason);
	}

	void ResetTravelForRetake(ATDShip* Ship)
	{
		if (!Ship)
		{
			return;
		}
		if (UTDHyperdriveComponent* H = Ship->GetHyperdrive())
		{
			H->ResetHyperdrive();
		}
		if (UTDLandingComponent* L = Ship->GetLanding())
		{
			L->AbortToFlying(); // un-parks; RestoreShipForFlight follows
		}
	}

	void AbortLanding(ATDShip* Ship)
	{
		if (UTDLandingComponent* L = Ship ? Ship->GetLanding() : nullptr)
		{
			L->AbortToFlying();
		}
	}

	float GetHyperdriveCharge01(const ATDShip* Ship)
	{
		const UTDHyperdriveComponent* H = Ship ? Ship->GetHyperdrive() : nullptr;
		return H ? H->GetCharge01() : 0.f;
	}

	FText GetHyperdriveBlockText(const ATDShip* Ship)
	{
		if (const UTDHyperdriveComponent* H = Ship ? Ship->GetHyperdrive() : nullptr)
		{
			const FText Reason = H->GetBlockReasonText();
			if (!Reason.IsEmpty())
			{
				return Reason;
			}
		}
		const UTDTravelSubsystem* Travel = Ship ? UTDTravelSubsystem::Get(Ship) : nullptr;
		return Travel ? Travel->GetHyperdriveLockText() : FText::GetEmpty();
	}

	float GetAltitudeMeters(const ATDShip* Ship)
	{
		return Ship ? Ship->GetAltitudeMeters() : -1.f;
	}

	void CarryManualTarget(const ATDShip* From, ATDShip* To)
	{
		const UTDTargetingComponent* Old = From ? From->GetTargeting() : nullptr;
		UTDTargetingComponent* New = To ? To->GetTargeting() : nullptr;
		if (Old && New && Old->IsManualTarget())
		{
			New->SetManualTarget(Old->GetCurrentTarget());
		}
	}

	void StopHyperdrive(ATDShip* Ship)
	{
		if (UTDHyperdriveComponent* H = Ship ? Ship->GetHyperdrive() : nullptr)
		{
			H->SetHyperdriveEnabled(false); // cancels a spool; a jump drops out
		}
	}

	void EnableHyperdrive(ATDShip* Ship)
	{
		if (UTDHyperdriveComponent* H = Ship ? Ship->GetHyperdrive() : nullptr)
		{
			H->SetHyperdriveEnabled(true);
		}
	}

	void IgnoreEachOther(ATDShip* A, ATDShip* B)
	{
		if (!A || !B)
		{
			return;
		}
		if (UPrimitiveComponent* RootA = Cast<UPrimitiveComponent>(A->GetRootComponent()))
		{
			RootA->IgnoreActorWhenMoving(B, true);
		}
		if (UPrimitiveComponent* RootB = Cast<UPrimitiveComponent>(B->GetRootComponent()))
		{
			RootB->IgnoreActorWhenMoving(A, true);
		}
	}

	FVector FindSafeLocation(const UObject* WorldContext, const FVector& Desired, const AActor* Ignore)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		if (!World)
		{
			return Desired;
		}
		const UTDWorldSubsystem* WorldSub = World->GetSubsystem<UTDWorldSubsystem>();
		constexpr float ShipRadius = 500.f;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TDSquadSafeSpot), false, Ignore);
		FCollisionObjectQueryParams Objects;
		Objects.AddObjectTypesToQuery(ECC_WorldStatic);
		Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
		const FVector Steps[] = { FVector::ZeroVector, FVector(0, 0, 3000), FVector(0, 0, -3000), FVector(-4000, 0, 0),
			FVector(0, 4000, 0), FVector(0, -4000, 0), FVector(0, 0, 8000), FVector(-10000, 0, 4000) };
		for (const FVector& Step : Steps)
		{
			const FVector P = Desired + Step;
			const bool bInObstacle = WorldSub && WorldSub->IsInsideObstacle(P, ShipRadius);
			const bool bOverlaps = World->OverlapAnyTestByObjectType(P, FQuat::Identity, Objects, FCollisionShape::MakeSphere(ShipRadius), Params);
			if (!bInObstacle && !bOverlaps)
			{
				return P;
			}
		}
		return Desired;
	}

	float GetBoundsRadius(const AActor* Actor)
	{
		if (!Actor)
		{
			return 0.f;
		}
		// The collision cylinder includes attached parts (carrier bays); actor bounds don't (T4).
		float Radius = 0.f, HalfHeight = 0.f;
		Actor->GetSimpleCollisionCylinder(Radius, HalfHeight);
		return FMath::Max(Radius, HalfHeight);
	}

	void TeleportShip(ATDShip* Ship, const FVector& Location, const FRotator& Facing, bool bKeepSpeed)
	{
		if (Ship)
		{
			Ship->TeleportSpacecraft(Location, Facing, bKeepSpeed);
		}
	}

	// ----------------------------------------------------------------------------------- health

	bool IsDead(const AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return true;
		}
		const UTDHealthComponent* H = Health(Actor);
		return H ? H->IsDead() : false;
	}

	float GetHull01(const AActor* Actor)
	{
		const UTDHealthComponent* H = Health(Actor);
		return (H && H->IsAlive()) ? H->GetHull01() : 0.f;
	}

	float GetShield01(const AActor* Actor)
	{
		const UTDHealthComponent* H = Health(Actor);
		return (H && H->IsAlive()) ? H->GetShield01() : 0.f;
	}

	float GetHealth01(const AActor* Actor)
	{
		const UTDHealthComponent* H = Health(Actor);
		if (!H || H->IsDead())
		{
			return 0.f;
		}
		const float Max = H->MaxHull + H->MaxShield;
		return Max > 0.f ? (H->GetHull() + H->GetShield()) / Max : 0.f;
	}

	void SetInvulnerable(AActor* Actor, float Seconds)
	{
		if (UTDHealthComponent* H = Health(Actor))
		{
			if (Seconds > 0.f)
			{
				H->SetInvulnerableFor(Seconds);
			}
		}
	}

	float GetMaxHull(const AActor* Actor)
	{
		const UTDHealthComponent* H = Health(Actor);
		return H ? H->MaxHull : 0.f;
	}

	void RepairHull(AActor* Actor, float Amount)
	{
		UTDHealthComponent* H = Health(Actor);
		if (H && H->IsAlive() && Amount > 0.f && H->GetHull01() < 1.f)
		{
			H->RepairHull(Amount);
		}
	}

	void RestoreFullHealth(AActor* Actor)
	{
		if (UTDHealthComponent* H = Health(Actor))
		{
			H->Revive(1.f, 1.f);
		}
	}

	// ---------------------------------------------------------------------------------- weapons

	void SetTriggerHeld(ATDShip* Ship, ETDWeaponSlot Slot, bool bHeld)
	{
		if (UTDWeaponComponent* W = Weapons(Ship))
		{
			W->SetTriggerHeld(Slot, bHeld);
		}
	}

	void SetAim(ATDShip* Ship, AActor* Target, const FVector& WorldDir)
	{
		if (UTDWeaponComponent* W = Weapons(Ship))
		{
			W->SetAimTarget(Target);
			W->SetAimDirection(WorldDir);
		}
	}

	void ClearAim(ATDShip* Ship)
	{
		if (UTDWeaponComponent* W = Weapons(Ship))
		{
			W->SetAimTarget(nullptr);
			W->SetAimDirection(FVector::ZeroVector);
		}
	}

	void SetWeaponsBlocked(ATDShip* Ship, FName SourceKey, bool bBlocked)
	{
		if (UTDWeaponComponent* W = Weapons(Ship))
		{
			W->SetWeaponsBlocked(SourceKey, bBlocked);
		}
	}

	bool IsOverheated(const ATDShip* Ship, ETDWeaponSlot Slot)
	{
		const UTDWeaponComponent* W = Weapons(Ship);
		return W ? W->IsOverheated(Slot) : false;
	}

	float GetHeat01(const ATDShip* Ship, ETDWeaponSlot Slot)
	{
		const UTDWeaponComponent* W = Weapons(Ship);
		return W ? W->GetHeat01(Slot) : 0.f;
	}

	int32 GetAmmo(const ATDShip* Ship, ETDWeaponSlot Slot)
	{
		const UTDWeaponComponent* W = Weapons(Ship);
		return W ? W->GetAmmo(Slot) : 0;
	}

	float GetPrimaryProjectileSpeed(const ATDShip* Ship)
	{
		const UTDWeaponComponent* W = Weapons(Ship);
		const float Speed = W ? W->GetSpec(ETDWeaponSlot::Primary).Projectile.Speed : 0.f;
		return Speed > 1.f ? Speed : 40000.f;
	}

	void SetOutgoingDamageMultiplier(ATDShip* Ship, FName SourceKey, float Multiplier)
	{
		if (UTDWeaponComponent* W = Weapons(Ship))
		{
			W->SetOutgoingDamageMultiplier(SourceKey, Multiplier);
		}
	}

	void ClearOutgoingDamageMultiplier(ATDShip* Ship, FName SourceKey)
	{
		if (UTDWeaponComponent* W = Weapons(Ship))
		{
			W->ClearOutgoingDamageMultiplier(SourceKey);
		}
	}

	// -------------------------------------------------------------------------------- awareness

	void GetPlayerHostiles(const UWorld* World, TArray<AActor*>& OutHostiles)
	{
		OutHostiles.Reset();
		if (const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(World))
		{
			Registry->GetHostilesOf(ETDTeam::Player, OutHostiles, /*bIncludeUntargetable*/ false);
		}
	}

	bool GetDirectorShotCamera(const AActor* WorldContext, FVector& OutLocation)
	{
		const UTDDirectorSubsystem* Director = WorldContext ? UTDDirectorSubsystem::Get(WorldContext) : nullptr;
		const UTDDirectorCameraRig* Rig = Director ? Director->GetCameraRig() : nullptr;
		if (!Rig || !Rig->IsActive())
		{
			return false;
		}
		OutLocation = Rig->GetTrackedLocation();
		return true;
	}

	FText GetCurrentObjectiveText(const UObject* WorldContext)
	{
		const UTDGameInstance* GI = UTDGameInstance::Get(WorldContext);
		return GI ? GI->GetCurrentObjective().Text : FText::GetEmpty();
	}

	FVector GetAimDirection(const ATDShip* Ship)
	{
		if (!Ship)
		{
			return FVector::ForwardVector;
		}
		const UTDWeaponComponent* W = Ship->FindComponentByClass<UTDWeaponComponent>();
		const FVector Dir = W ? W->GetAimDirection() : FVector::ZeroVector;
		return Dir.IsNearlyZero() ? Ship->GetActorForwardVector() : Dir.GetSafeNormal();
	}

	void BindShipDamaged(ATDSquadManager* Squad, ATDShip* Ship)
	{
		if (UTDHealthComponent* H = Health(Ship))
		{
			H->OnDamaged.AddUniqueDynamic(Squad, &ATDSquadManager::HandleShipDamaged);
			H->OnShieldBroken.AddUniqueDynamic(Squad, &ATDSquadManager::HandleShieldBroken);
		}
	}

	float GetMaxHullAndShield(const AActor* Actor)
	{
		const UTDHealthComponent* H = Health(Actor);
		return H ? H->MaxHull + H->MaxShield : 0.f;
	}

	float GetDodgeReady01(const ATDShip* Ship)
	{
		const UTDShipMovementComponent* Move = Ship ? Ship->GetShipMovement() : nullptr;
		return Move ? Move->GetDodgeReady01() : 1.f;
	}

	bool PlayShipDeathPlaceholder(ATDShip* Ship, ETDShipId Id, bool bTrailPuff)
	{
		if (!Ship)
		{
			return false;
		}
		const UTDShipVisuals* V = GetShipVisuals(Ship);
		if (V && FTDCombatPlaceholders::HasVisualArt(V->Explosion))
		{
			return false; // Nick's art plays through the cue
		}
		UTDProjectileSubsystem* P = UTDProjectileSubsystem::Get(Ship);
		if (!P)
		{
			return false;
		}
		const float R = GetBoundsRadius(Ship);
		const FLinearColor Color = UTDStatics::ShipColor(Id);
		const FVector Loc = Ship->GetActorLocation();
		if (bTrailPuff)
		{
			if (Ship->IsPlayerFlown())
			{
				// The player's chase camera follows the tumble's path, so a lingering puff ends up in its face
				// (live dth6: a screen-sized disc). Brief sparks only, gone before the camera gets there.
				return P->PlayPlaceholderFlash(Loc, 60.f, FMath::Lerp(Color, FLinearColor::White, 0.5f), 0.12f) != nullptr;
			}
			return P->PlayPlaceholderExplosion(Loc, FMath::Clamp(R * 0.2f, 60.f, 150.f), Color, 1) > 0;
		}
		const float Radius = FMath::Max(600.f, R * 2.5f);
		if (!Ship->IsPlayerFlown())
		{
			// A wingman, seen from a distance: T2's full placeholder explosion.
			return P->PlayPlaceholderExplosion(Loc, Radius, Color, 12) > 0;
		}
		// The player's own ship, seen from its chase camera (Lead playtest 8: the 0.6 s shell at 600 cm
		// filled half the screen as a flat pink wipe). The shell never reaches the camera: capped to 45% of
		// the camera distance and gone in 0.22 s; a small hot core; the debris does the work.
		const float CamDist = FVector::Dist(Ship->GetChaseCameraTransformNow().GetLocation(), Loc);
		// Live (dth5_*): even at 45% the opaque placeholder shell read as a flat disc, and it darkens as
		// it fades. So: a small, brief shell (20% of the camera distance, 0.12 s), a hot white core, and
		// the debris (sized to the 45% limit) carries the blast.
		const float Blast = FMath::Min(Radius, CamDist * 0.45f);
		const float Shell = FMath::Min(Blast, CamDist * 0.2f);
		const float Core = CamDist * 0.07f;
		int32 Used = 0;
		Used += P->PlayPlaceholderFlash(Loc, Core, FMath::Lerp(Color, FLinearColor::White, 0.75f), 0.15f) ? 1 : 0;
		Used += P->PlayPlaceholderFlash(Loc, Shell, Color, 0.12f) ? 1 : 0;
		const int32 Debris = 16;
		for (int32 I = 0; I < Debris; ++I)
		{
			const float Y = 1.f - 2.f * (I + 0.5f) / Debris;
			const float RingR = FMath::Sqrt(FMath::Max(0.f, 1.f - Y * Y));
			const float Phi = I * 2.39996f + FMath::FRandRange(-0.3f, 0.3f);
			const FVector Dir(FMath::Cos(Phi) * RingR, FMath::Sin(Phi) * RingR, Y);
			if (ATDCombatFlash* Flash = P->PlayPlaceholderFlash(Loc, 1.f, Color, 0.01f))
			{
				Flash->PlayDebris(Loc + Dir * Shell * 0.3f, Dir * Blast * FMath::FRandRange(3.f, 4.5f), Blast * 0.5f,
					FMath::Clamp(Blast * 0.05f, 15.f, 60.f), FMath::Lerp(Color, FLinearColor::White, 0.25f), FMath::FRandRange(0.5f, 0.8f));
				++Used;
			}
		}
		UE_LOG(LogTriadDrift, Display, TEXT("TDSQUAD death placeholder (player): %s camera %.0f cm -> shell %.0f cm, core %.0f cm, %d flashes"),
			*UEnum::GetDisplayValueAsText(Id).ToString(), CamDist, Shell, Core, Used);
		return Used > 0;
	}

	void PlayPlaceholderPulse(AActor* At, float Radius, const FLinearColor& Color, float Seconds)
	{
		UTDProjectileSubsystem* P = At ? UTDProjectileSubsystem::Get(At) : nullptr;
		if (P)
		{
			P->PlayPlaceholderFlash(At->GetActorLocation(), Radius, Color, Seconds, At->GetRootComponent());
		}
	}

	FText GetLandPrompt(const ATDShip* Ship)
	{
		const UTDLandingComponent* Landing = Ship ? Ship->GetLanding() : nullptr;
		return Landing ? Landing->GetLandPrompt() : FText::GetEmpty();
	}

	bool HoldOutOfGates(const AActor* Ship, float LookAheadSeconds)
	{
		UWorld* World = Ship ? Ship->GetWorld() : nullptr;
		UTDPowerSubsystem* Powers = UTDPowerSubsystem::Get(Ship);
		if (!World || !Powers)
		{
			return false;
		}
		const FVector Loc = Ship->GetActorLocation();
		const FVector Vel = Ship->GetVelocity();
		// From one step behind to LookAhead ahead: covers this frame's integration whichever ticks first.
		const FVector From = Loc - Vel * 0.05f;
		const FVector To = Loc + Vel * LookAheadSeconds;
		// Review minor 6: the gate list is cached per world (gates are level actors; refreshed every 2 s).
		static TWeakObjectPtr<UWorld> CachedWorld;
		static TArray<TWeakObjectPtr<ATDPortalGate>> Gates;
		static double NextRefresh = 0.0;
		const double T = World->GetTimeSeconds();
		if (CachedWorld.Get() != World || T >= NextRefresh || NextRefresh - T > 2.0)
		{
			CachedWorld = World;
			NextRefresh = T + 2.0;
			Gates.Reset();
			for (TActorIterator<ATDPortalGate> It(World); It; ++It)
			{
				Gates.Add(*It);
			}
		}
		for (const TWeakObjectPtr<ATDPortalGate>& Weak : Gates)
		{
			const ATDPortalGate* G = Weak.Get();
			bool bForward = true;
			if (G && G->IsGateActive() && ATDPortalGate::SegmentCrossesGate(G->GetActorTransform(), G->Radius * 1.25f, From, To, bForward))
			{
				Powers->MarkTransited(Ship);
				return true;
			}
		}
		return false;
	}

	bool FindRegisteredAvoidance(const UWorld* World, const FVector& Origin, const FVector& Direction, float ProbeDistance, float ShipRadius, FVector& OutAvoidDirection)
	{
		const UTDWorldSubsystem* WorldSub = World ? World->GetSubsystem<UTDWorldSubsystem>() : nullptr;
		return WorldSub && WorldSub->FindAvoidance(Origin, Direction, ProbeDistance, ShipRadius, OutAvoidDirection);
	}

	AActor* GetLastAttacker(const AActor* Actor)
	{
		const UTDHealthComponent* H = Health(Actor);
		return H ? H->GetLastAttacker() : nullptr;
	}

	float GetTimeSinceLastDamage(const AActor* Actor)
	{
		const UTDHealthComponent* H = Health(Actor);
		return H ? H->GetTimeSinceLastDamage() : 1000.f;
	}

	bool IsMissileIncoming(const ATDShip* Ship)
	{
		if (!Ship)
		{
			return false;
		}
		const UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(Ship);
		if (!Projectiles)
		{
			return false;
		}
		static TArray<ATDProjectile*> Scratch; // game thread only; avoids a per-call allocation
		Scratch.Reset();
		Projectiles->FindMissilesHomingOn(Ship, Scratch);
		for (const ATDProjectile* P : Scratch)
		{
			if (P && UTDStatics::AreTeamsHostile(P->GetTeam(), ETDTeam::Player))
			{
				Scratch.Reset();
				return true;
			}
		}
		Scratch.Reset();
		return false;
	}

	bool GetNearbyShieldDome(const UWorld* World, const FVector& Location, float MaxDistance, FVector& OutCenter, float& OutRadius)
	{
		const UTDPowerSubsystem* Powers = World ? UTDPowerSubsystem::Get(World) : nullptr;
		const ATDShieldDome* Dome = Powers ? Powers->GetNearestDome(Location, ETDTeam::Player) : nullptr;
		if (!Dome || FVector::DistSquared(Dome->GetActorLocation(), Location) > FMath::Square(MaxDistance))
		{
			return false;
		}
		OutCenter = Dome->GetActorLocation();
		OutRadius = Dome->GetRadius();
		return true;
	}

	static void AddPower(ATDShip* Ship, UClass* PowerClass, ETDPowerInput Slot)
	{
		if (!Ship || !PowerClass || UTDPowerComponent::FindPower(Ship, Slot))
		{
			return; // never a second component for a slot: powers keep running across switches
		}
		UTDPowerComponent* Power = NewObject<UTDPowerComponent>(Ship, PowerClass);
		if (Power)
		{
			Power->Slot = Slot;
			Ship->AddInstanceComponent(Power);
			Power->RegisterComponent();
		}
	}

	void InstallPowers(ATDShip* Ship, const FTDShipSpec& Spec)
	{
		if (!Ship)
		{
			return;
		}
		AddPower(Ship, Spec.Signature1PowerClass.Get(), ETDPowerInput::Signature1);
		AddPower(Ship, Spec.Signature2PowerClass.Get(), ETDPowerInput::Signature2);

		// Unlockable ability slot (ENG-12): the component installs whatever progress has unlocked.
		UTDAbilityComponent* Abilities = Ship->FindComponentByClass<UTDAbilityComponent>();
		if (!Abilities && Spec.AbilityComponentClass)
		{
			Abilities = NewObject<UTDAbilityComponent>(Ship, Spec.AbilityComponentClass.Get(), TEXT("Abilities"));
			if (Abilities)
			{
				Ship->AddInstanceComponent(Abilities);
				Abilities->RegisterComponent();
			}
		}
		if (Abilities)
		{
			Abilities->RefreshFromProgress();
		}
	}

	void ResetPowers(ATDShip* Ship)
	{
		if (!Ship)
		{
			return;
		}
		TInlineComponentArray<UTDPowerComponent*> Powers(Ship);
		for (UTDPowerComponent* Power : Powers)
		{
			if (Power)
			{
				Power->ResetPower(); // ends any active phase safely and readies it (T2)
			}
		}
	}

	void ReleasePlayerHolds(ATDShip* Ship)
	{
		if (!Ship)
		{
			return;
		}
		for (const ETDPowerInput Slot : { ETDPowerInput::Signature1, ETDPowerInput::Signature2, ETDPowerInput::Ability })
		{
			if (UTDPowerComponent::FindPower(Ship, Slot))
			{
				UTDPowerComponent::RoutePowerInput(Ship, Slot, false);
			}
		}
		if (UTDWeaponComponent* W = Weapons(Ship))
		{
			W->SetTriggerHeld(ETDWeaponSlot::Primary, false);
			W->SetTriggerHeld(ETDWeaponSlot::Secondary, false);
			W->CancelCharge(ETDWeaponSlot::Secondary);
		}
	}

	void ResetWeapons(ATDShip* Ship)
	{
		if (UTDWeaponComponent* W = Weapons(Ship))
		{
			W->ResetWeapons();
		}
	}

	void GetPowerCooldowns(const ATDShip* Ship, float& OutPower1, float& OutPower2)
	{
		// Review minor 8: a power the ship doesn't have reads 0, never "ready".
		auto Cooldown = [Ship](ETDPowerInput Slot)
		{
			const UTDPowerComponent* Power = Ship ? UTDPowerComponent::FindPower(Ship, Slot) : nullptr;
			return Power ? Power->GetCooldown01() : 0.f;
		};
		OutPower1 = Cooldown(ETDPowerInput::Signature1);
		OutPower2 = Cooldown(ETDPowerInput::Signature2);
	}

	void GetEarnedAbility(const ATDShip* Ship, ETDAbilityId& OutAbility, float& OutReadiness01)
	{
		OutAbility = ETDAbilityId::None;
		OutReadiness01 = 0.f;
		const UTDAbilityComponent* Abilities = Ship ? Ship->FindComponentByClass<UTDAbilityComponent>() : nullptr;
		if (!Abilities)
		{
			return;
		}
		// T2's accessors: None / 0 while locked, never "ready" for an absent power.
		OutAbility = Abilities->HasAbility() ? Abilities->GetAbility() : ETDAbilityId::None;
		OutReadiness01 = Abilities->GetReadiness01();
	}

	// ------------------------------------------------------------------------------------- HUD

	float GetBoost01(const ATDShip* Ship)
	{
		return Ship ? Ship->GetBoost01() : 0.f;
	}

	float GetSpeed01(const ATDShip* Ship)
	{
		return Ship ? Ship->GetSpeed01() : 0.f;
	}

	bool IsOutOfBounds(const ATDShip* Ship)
	{
		return Ship ? Ship->IsOutOfBounds() : false;
	}

	FText GetBoundaryWarningText(const ATDShip* Ship)
	{
		const UTDShipMovementComponent* Move = Ship ? Ship->GetShipMovement() : nullptr;
		return Move ? Move->GetBoundaryWarningText() : FText::GetEmpty();
	}

	AActor* GetCurrentTarget(const ATDShip* Ship)
	{
		const UTDTargetingComponent* T = Ship ? Ship->FindComponentByClass<UTDTargetingComponent>() : nullptr;
		return T ? T->GetCurrentTarget() : nullptr;
	}

	void GetLockedTargets(const ATDShip* Ship, TArray<AActor*>& Out)
	{
		Out.Reset();
		if (const UTDTargetingComponent* T = Ship ? Ship->FindComponentByClass<UTDTargetingComponent>() : nullptr)
		{
			Out = T->GetLockedTargets();
		}
	}

	static const UTDTargetingComponent* Targeting(const ATDShip* Ship)
	{
		return Ship ? Ship->GetTargeting() : nullptr;
	}

	void GetTargetMarkers(const ATDShip* Ship, TArray<FTDTargetMarker>& Out)
	{
		Out.Reset();
		if (const UTDTargetingComponent* T = Targeting(Ship))
		{
			Out = T->GetMarkers();
		}
	}

	ETDMissileWarning GetMissileWarning(const ATDShip* Ship)
	{
		const UTDTargetingComponent* T = Targeting(Ship);
		return T ? T->GetMissileWarning() : ETDMissileWarning::None;
	}

	bool GetLeadIndicator(const ATDShip* Ship, FVector& OutWorldLocation)
	{
		const UTDTargetingComponent* T = Targeting(Ship);
		return T ? T->GetLeadIndicator(OutWorldLocation) : false;
	}

	FTDWaveInfo GetWaveInfo(const UObject* WorldContext, FWaveDirectorCache& Cache, double RetrySeconds)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		if (!World)
		{
			return FTDWaveInfo();
		}
		const ATDWaveDirector* Director = Cast<ATDWaveDirector>(Cache.Director.Get());
		if (!Director || Director->GetWorld() != World)
		{
			const double Now = World->GetRealTimeSeconds();
			if (Now < Cache.NextLookupTime)
			{
				return FTDWaveInfo();
			}
			Cache.NextLookupTime = Now + RetrySeconds;
			Director = ATDWaveDirector::Get(WorldContext);
			Cache.Director = const_cast<ATDWaveDirector*>(Director);
		}
		return Director ? Director->GetWaveInfo() : FTDWaveInfo();
	}

	// --------------------------------------------------------------------------- events & flow

	void BroadcastShipSwitched(const UObject* WorldContext, ETDShipId NewShip, ETDShipId OldShip)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(WorldContext))
		{
			Bus->BroadcastShipSwitched(NewShip, OldShip);
		}
	}

	void BroadcastShipDowned(const UObject* WorldContext, ETDShipId Ship)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(WorldContext))
		{
			Bus->BroadcastShipDowned(Ship);
		}
	}

	void BroadcastShipRespawned(const UObject* WorldContext, ETDShipId Ship)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(WorldContext))
		{
			Bus->BroadcastShipRespawned(Ship);
		}
	}

	void BroadcastSquadWiped(const UObject* WorldContext)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(WorldContext))
		{
			Bus->BroadcastSquadWiped();
		}
	}

	bool ReportPlayerDefeated(const UObject* WorldContext)
	{
		if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(WorldContext))
		{
			Flow->ReportPlayerDefeated();
			return true;
		}
		return false;
	}

	void RequestBark(const UObject* WorldContext, FName BarkId, FName Speaker)
	{
		if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(WorldContext))
		{
			Flow->RequestBark(BarkId, Speaker);
		}
		else if (UTDEventBus* Bus = UTDEventBus::Get(WorldContext))
		{
			Bus->BroadcastBarkRequested(BarkId, Speaker);
		}
	}

	void BindActorKilled(ATDSquadManager* Squad)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(Squad))
		{
			Bus->OnActorKilled.AddUniqueDynamic(Squad, &ATDSquadManager::HandleActorKilled);
			Bus->OnObjectiveUpdated.AddUniqueDynamic(Squad, &ATDSquadManager::HandleObjectiveUpdated);
		}
	}

	void UnbindActorKilled(ATDSquadManager* Squad)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(Squad))
		{
			Bus->OnActorKilled.RemoveDynamic(Squad, &ATDSquadManager::HandleActorKilled);
			Bus->OnObjectiveUpdated.RemoveDynamic(Squad, &ATDSquadManager::HandleObjectiveUpdated);
		}
	}

	void AddShipSwitchStat(const UObject* WorldContext)
	{
		if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(WorldContext))
		{
			Flow->AddStat(TEXT("ShipSwitches"), 1);
		}
	}

	void BindPowerUsed(ATDSquadManager* Squad)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(Squad))
		{
			Bus->OnPowerUsed.AddUniqueDynamic(Squad, &ATDSquadManager::HandlePowerUsed);
		}
	}

	void UnbindPowerUsed(ATDSquadManager* Squad)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(Squad))
		{
			Bus->OnPowerUsed.RemoveDynamic(Squad, &ATDSquadManager::HandlePowerUsed);
		}
	}

	void BindTeleported(ATDWingmanAIController* Wingman)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(Wingman))
		{
			Bus->OnSpacecraftTeleported.AddUniqueDynamic(Wingman, &ATDWingmanAIController::HandleSpacecraftTeleported);
		}
	}

	void UnbindTeleported(ATDWingmanAIController* Wingman)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(Wingman))
		{
			Bus->OnSpacecraftTeleported.RemoveDynamic(Wingman, &ATDWingmanAIController::HandleSpacecraftTeleported);
		}
	}

	void BindWaveEvents(ATDSquadManager* Squad)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(Squad))
		{
			Bus->OnWaveCleared.AddUniqueDynamic(Squad, &ATDSquadManager::HandleWaveCleared);
			Bus->OnWaveStarted.AddUniqueDynamic(Squad, &ATDSquadManager::HandleWaveStarted);
			Bus->OnTakeStarted.AddUniqueDynamic(Squad, &ATDSquadManager::HandleTakeStartedForBarks);
			Bus->OnAbilityUnlocked.AddUniqueDynamic(Squad, &ATDSquadManager::HandleAbilityUnlockedForBarks);
			Bus->OnObjectiveUpdated.AddUniqueDynamic(Squad, &ATDSquadManager::HandleObjectiveForBarks);
		}
		if (UTDArmadaForesight* Foresight = UTDArmadaForesight::Get(Squad))
		{
			Foresight->OnAnticipated.AddUniqueDynamic(Squad, &ATDSquadManager::HandleAnticipatedForBarks);
		}
	}

	void UnbindWaveEvents(ATDSquadManager* Squad)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(Squad))
		{
			Bus->OnWaveCleared.RemoveDynamic(Squad, &ATDSquadManager::HandleWaveCleared);
			Bus->OnWaveStarted.RemoveDynamic(Squad, &ATDSquadManager::HandleWaveStarted);
			Bus->OnTakeStarted.RemoveDynamic(Squad, &ATDSquadManager::HandleTakeStartedForBarks);
			Bus->OnAbilityUnlocked.RemoveDynamic(Squad, &ATDSquadManager::HandleAbilityUnlockedForBarks);
			Bus->OnObjectiveUpdated.RemoveDynamic(Squad, &ATDSquadManager::HandleObjectiveForBarks);
		}
		if (UTDArmadaForesight* Foresight = UTDArmadaForesight::Get(Squad))
		{
			Foresight->OnAnticipated.RemoveDynamic(Squad, &ATDSquadManager::HandleAnticipatedForBarks);
		}
	}

	const UTDShipVisuals* GetShipVisuals(const ATDShip* Ship)
	{
		return Ship ? Ship->GetShipVisuals() : nullptr;
	}

	void PlayShipCue(const ATDShip* Ship, const FTDFXCue& Cue, bool bAttached)
	{
		if (!Ship)
		{
			return;
		}
		if (bAttached)
		{
			UTDStatics::PlayCueAttached(Cue, Ship->GetRootComponent());
		}
		else
		{
			UTDStatics::PlayCue(Ship, Cue, Ship->GetActorLocation(), Ship->GetActorRotation());
		}
	}

	FTransform GetChaseCameraTransform(const ATDShip* Ship)
	{
		if (!Ship)
		{
			return FTransform::Identity;
		}
		// T1: lag-free, valid even for an AI-flown ship whose spring arm doesn't tick (m10).
		return Ship->GetChaseCameraTransformNow();
	}
}
