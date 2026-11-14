// Triad Drift - the combat warm-up list (Teammate 2, issue #89). See the header.

#include "TriadDrift/Combat/TDCombatAssetList.h"

#include "Engine/World.h"
#include "TriadDrift/Combat/Pickups/TDPickup.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Powers/TDAbilityPowers.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/World/TDShieldGenerator.h"

namespace TDCombatAssetListImpl
{
	void Add(TArray<FSoftObjectPath>& Out, const FSoftObjectPath& Path)
	{
		if (Path.IsValid())
		{
			Out.AddUnique(Path);
		}
	}

	void AddClass(TArray<FSoftObjectPath>& Out, const UClass* Class)
	{
		if (Class)
		{
			Add(Out, FSoftClassPath(Class));
		}
	}
}

void FTDCombatAssetList::CollectFor(bool bSpace, TArray<FSoftObjectPath>& Out)
{
	using namespace TDCombatAssetListImpl;
	// Everywhere: shots, flashes, pickups-free placeholders, the shared glow materials.
	AddClass(Out, ATDProjectile::StaticClass());
	AddClass(Out, ATDCombatFlash::StaticClass());
	AddClass(Out, ATDPlaceholderFlash::StaticClass());
	Add(Out, FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Materials/M_TD_FlashGlow.M_TD_FlashGlow")));
	Add(Out, FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Materials/M_TD_TrailGlow.M_TD_TrailGlow")));
	Add(Out, FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Materials/M_TD_SmokePuff.M_TD_SmokePuff")));
	Add(Out, FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Characters/Barks/DT_BarksFallback.DT_BarksFallback")));
	if (bSpace)
	{
		// The wave table's classes and their parts, the squad's power actors, pickups, the Dominion's generators.
		AddClass(Out, ATDEnemyShip::StaticClass());
		AddClass(Out, ATDEnemyTurret::StaticClass());
		AddClass(Out, ATDCarrierBay::StaticClass());
		AddClass(Out, ATDShieldGenerator::StaticClass());
		AddClass(Out, ATDPickup::StaticClass());
		AddClass(Out, ATDGravityWell::StaticClass());
		AddClass(Out, ATDPortal::StaticClass());
		AddClass(Out, ATDShieldDome::StaticClass());
		Add(Out, FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Materials/M_TD_PortalMembrane.M_TD_PortalMembrane")));
		Add(Out, FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Engine_Enemy.NS_TD_Engine_Enemy")));
		Add(Out, FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Engine_Lancer.NS_TD_Engine_Lancer")));
	}
	else
	{
		// On foot: the crew (their CDO pulls the default looks and sounds), their bolts, the drone's thruster.
		AddClass(Out, ATDCrewCharacter::StaticClass());
		Add(Out, FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Characters/FX/DA_CrewBlaster_FX.DA_CrewBlaster_FX")));
		Add(Out, FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Drone_Thruster.NS_TD_Drone_Thruster")));
	}
}

void FTDCombatAssetList::Collect(const UWorld& World, TArray<FSoftObjectPath>& Out)
{
	// The galaxy map fights in space (waves, the Dominion assault and escape); planets and the Dominion interior on foot.
	const FString Map = World.GetMapName();
	CollectFor(Map.Contains(TEXT("Galaxy")), Out);
}
