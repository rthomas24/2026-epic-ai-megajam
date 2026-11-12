// Triad Drift - Squad (Teammate 3). Cross-area calls, isolated here (see TDSquadIntegration.h).

#include "TriadDrift/Squad/TDSquadIntegration.h"
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
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"

#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"

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
			Ship->ApplyFlightTuning(Tuning);
		}

		if (UTDHealthComponent* H = Health(Ship))
		{
			H->InitMaxValues(Spec.MaxHull, Spec.MaxShield, /*bRefill*/ true);
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
				Power->ResetCooldown();
			}
		}
	}

	void ResetWeapons(ATDShip* Ship)
	{
		if (UTDWeaponComponent* W = Weapons(Ship))
		{
			W->ResetWeapons();
		}
	}

	void GetPowerCooldowns(const ATDShip* Ship, float& OutPower1, float& OutPower2, float& OutAbility)
	{
		auto Cooldown = [Ship](ETDPowerInput Slot)
		{
			const UTDPowerComponent* Power = Ship ? UTDPowerComponent::FindPower(Ship, Slot) : nullptr;
			return Power ? Power->GetCooldown01() : 1.f;
		};
		OutPower1 = Cooldown(ETDPowerInput::Signature1);
		OutPower2 = Cooldown(ETDPowerInput::Signature2);
		OutAbility = Cooldown(ETDPowerInput::Ability);
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
		}
	}

	void UnbindWaveEvents(ATDSquadManager* Squad)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(Squad))
		{
			Bus->OnWaveCleared.RemoveDynamic(Squad, &ATDSquadManager::HandleWaveCleared);
			Bus->OnWaveStarted.RemoveDynamic(Squad, &ATDSquadManager::HandleWaveStarted);
		}
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
		const UCameraComponent* Cam = Ship->GetChaseCamera();
		return Cam ? Cam->GetComponentTransform() : Ship->GetActorTransform();
	}
}
