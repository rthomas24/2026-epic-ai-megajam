// Triad Drift - Squad (Teammate 3). ENG-04: gameplay stats of the three squad ships.

#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Squad/TDSquadIntegration.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Powers/TDAbilityComponent.h"
#include "TriadDrift/Powers/TDBastionPowers.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDRamPower.h"

#define LOCTEXT_NAMESPACE "TDShipLibrary"

namespace TDShipLibraryPrivate
{
	static FTDShipSpec MakeRift()
	{
		FTDShipSpec S;
		S.Ship = ETDShipId::Rift;
		S.ShipName = LOCTEXT("RiftName", "RIFT");
		S.PilotName = LOCTEXT("RiftPilot", "WREN");
		S.RoleName = LOCTEXT("RiftRole", "Trickster");
		S.MaxHull = 100.f;
		S.MaxShield = 100.f;
		// T4's regen sweep (Nov 13, Lead-approved): Rift lost the most ships in waves 3-5; 2 s / 35% brings it to Bastion.
		S.ShieldRegenDelay = 2.f;
		S.ShieldRegenFractionPerSecond = 0.35f;
		S.CruiseSpeed = 6000.f;
		S.BoostSpeed = 11000.f;
		S.TurnRateDeg = 120.f;
		S.BoostMeterSeconds = 3.f;
		S.BoostRegenSeconds = 2.5f;
		S.PrimaryWeapon = TEXT("PhasePulseLasers");
		S.SecondaryWeapon = TEXT("SplitterBolt");
		S.SignaturePower = TEXT("PortalBlaster");
		S.Signature1PowerClass = UTDPortalBlasterPower::StaticClass();
		S.AbilityComponentClass = UTDAbilityComponent::StaticClass();
		S.UnlockableAbility = ETDAbilityId::Cloak;
		S.SquadColor = UTDStatics::ShipColor(ETDShipId::Rift); // magenta
		S.Visuals = TSoftObjectPtr<UTDShipVisuals>(FSoftObjectPath(TEXT("/Game/TriadDrift/Presentation/DA_Rift_Visuals.DA_Rift_Visuals")));
		return S;
	}

	static FTDShipSpec MakeLancer()
	{
		FTDShipSpec S;
		S.Ship = ETDShipId::Lancer;
		S.ShipName = LOCTEXT("LancerName", "LANCER");
		S.PilotName = LOCTEXT("LancerPilot", "KATO");
		S.RoleName = LOCTEXT("LancerRole", "Interceptor");
		S.MaxHull = 70.f;
		S.MaxShield = 96.f;	// x1.2 (T4 sweep, Lead-approved Nov 13): wave-5 downs 5.2 -> 3.5, near Bastion; hull stays fragile
		S.CruiseSpeed = 7500.f;
		S.BoostSpeed = 15000.f;
		S.TurnRateDeg = 100.f;
		S.BoostMeterSeconds = 4.f;
		S.BoostRegenSeconds = 1.5f; // "fast regen"
		S.PrimaryWeapon = TEXT("NeedleCannons");
		S.SecondaryWeapon = TEXT("SpikeRockets");
		S.SignaturePower = TEXT("AfterburnerRam");
		S.Signature1PowerClass = UTDRamPower::StaticClass();
		S.AbilityComponentClass = UTDAbilityComponent::StaticClass();
		S.UnlockableAbility = ETDAbilityId::EMPPulse;
		S.SquadColor = UTDStatics::ShipColor(ETDShipId::Lancer); // cyan
		S.Visuals = TSoftObjectPtr<UTDShipVisuals>(FSoftObjectPath(TEXT("/Game/TriadDrift/Presentation/DA_Lancer_Visuals.DA_Lancer_Visuals")));
		return S;
	}

	static FTDShipSpec MakeBastion()
	{
		FTDShipSpec S;
		S.Ship = ETDShipId::Bastion;
		S.ShipName = LOCTEXT("BastionName", "BASTION");
		S.PilotName = LOCTEXT("BastionPilot", "MOSS");
		S.RoleName = LOCTEXT("BastionRole", "Heavy");
		S.MaxHull = 180.f;
		S.MaxShield = 160.f;
		S.CruiseSpeed = 4800.f;
		S.BoostSpeed = 8500.f;
		S.TurnRateDeg = 75.f;
		S.BoostMeterSeconds = 2.5f;
		S.BoostRegenSeconds = 2.5f;
		S.PrimaryWeapon = TEXT("BreakerBlaster");
		S.SecondaryWeapon = TEXT("FlakBurst");
		S.SignaturePower = TEXT("MissileSwarm");
		S.Signature1PowerClass = UTDMissileSwarmPower::StaticClass();
		S.Signature2PowerClass = UTDShieldDomePower::StaticClass();
		S.AbilityComponentClass = UTDAbilityComponent::StaticClass();
		S.UnlockableAbility = ETDAbilityId::GravityWell;
		S.SquadColor = UTDStatics::ShipColor(ETDShipId::Bastion); // amber
		S.Visuals = TSoftObjectPtr<UTDShipVisuals>(FSoftObjectPath(TEXT("/Game/TriadDrift/Presentation/DA_Bastion_Visuals.DA_Bastion_Visuals")));
		return S;
	}
}

const FTDShipSpec& UTDShipLibrary::Get(ETDShipId Ship)
{
	static const FTDShipSpec Rift = TDShipLibraryPrivate::MakeRift();
	static const FTDShipSpec Lancer = TDShipLibraryPrivate::MakeLancer();
	static const FTDShipSpec Bastion = TDShipLibraryPrivate::MakeBastion();

	switch (Ship)
	{
	case ETDShipId::Lancer:  return Lancer;
	case ETDShipId::Bastion: return Bastion;
	case ETDShipId::Rift:
	default:                 return Rift;
	}
}

FTDShipSpec UTDShipLibrary::GetShipSpec(ETDShipId Ship)
{
	return Get(Ship);
}

const TArray<ETDShipId>& UTDShipLibrary::AllShips()
{
	static const TArray<ETDShipId> Ships = { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion };
	return Ships;
}

void UTDShipLibrary::ApplySpecToShip(ATDShip* Ship, const FTDShipSpec& Spec)
{
	if (!Ship)
	{
		return;
	}
	TDSquad::ApplySpec(Ship, Spec);
}

UTDShipVisuals* UTDShipLibrary::LoadVisuals(const FTDShipSpec& Spec)
{
	if (Spec.Visuals.IsNull())
	{
		return nullptr;
	}
	if (UTDShipVisuals* Loaded = Spec.Visuals.Get())
	{
		return Loaded;
	}
	// Core's single loader: quiet when Nick hasn't authored the asset yet, remembers misses, and works
	// in packaged builds (the Presentation folder is always cooked).
	return UTDStatics::LoadPresentation<UTDShipVisuals>(Spec.Visuals.GetAssetName());
}

FLinearColor UTDShipLibrary::GetSquadColor(const FTDShipSpec& Spec)
{
	if (const UTDShipVisuals* V = LoadVisuals(Spec))
	{
		if (!V->SquadColor.Equals(FLinearColor::White))
		{
			return V->SquadColor;
		}
	}
	return Spec.SquadColor;
}

void UTDShipLibrary::ApplyDefaultSpecToShip(ATDShip* Ship, ETDShipId ShipId)
{
	ApplySpecToShip(Ship, Get(ShipId));
}

#undef LOCTEXT_NAMESPACE
