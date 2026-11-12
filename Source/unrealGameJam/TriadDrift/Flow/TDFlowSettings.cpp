// Triad Drift - game flow settings (ENG-19). Owner: Teammate 5.

#include "TriadDrift/Flow/TDFlowSettings.h"
#include "Misc/PackageName.h"

UTDFlowSettings::UTDFlowSettings()
{
	// Planned maps (features.md C-01). They don't exist yet; travel fails safely until they do.
	DestinationMaps.Add(ETDDestination::MainMenu, FSoftObjectPath(TEXT("/Game/TriadDrift/Maps/L_MainMenu.L_MainMenu")));
	DestinationMaps.Add(ETDDestination::Galaxy, FSoftObjectPath(TEXT("/Game/TriadDrift/Maps/L_Galaxy.L_Galaxy")));
	DestinationMaps.Add(ETDDestination::Verdance, FSoftObjectPath(TEXT("/Game/TriadDrift/Maps/L_Planet_Verdance.L_Planet_Verdance")));
	DestinationMaps.Add(ETDDestination::Cinder, FSoftObjectPath(TEXT("/Game/TriadDrift/Maps/L_Planet_Cinder.L_Planet_Cinder")));
	DestinationMaps.Add(ETDDestination::DominionInterior, FSoftObjectPath(TEXT("/Game/TriadDrift/Maps/L_Dominion_Interior.L_Dominion_Interior")));
	TransitionMap = FSoftObjectPath(TEXT("/Game/TriadDrift/Maps/L_Transition.L_Transition"));

	// Ability owners (features.md §0): Verdance -> Gravity Well (Bastion), Cloak (Rift); Cinder -> EMP (Lancer).
	AbilityOwners.Add(ETDAbilityId::GravityWell, ETDShipId::Bastion);
	AbilityOwners.Add(ETDAbilityId::Cloak, ETDShipId::Rift);
	AbilityOwners.Add(ETDAbilityId::EMPPulse, ETDShipId::Lancer);

	// Objective ids shared with ENG-25 (Verdance) and ENG-26 (Cinder).
	VerdanceObjectives = { TEXT("Obj_WreckCore"), TEXT("Obj_Relay") };
	CinderObjectives = { TEXT("Obj_Outpost"), TEXT("Obj_Jammer") };

	// 1-based wave numbers.
	PhaseAfterWaveCleared.Add(2, ETDGamePhase::Verdance);
	PhaseAfterWaveCleared.Add(4, ETDGamePhase::Cinder);
	PhaseAfterWaveCleared.Add(5, ETDGamePhase::DominionAssault);

	SpacePlayerControllerClass =FSoftClassPath(TEXT("/Script/unrealGameJam.TDSpacePlayerController"));
	SquadManagerClass = FSoftClassPath(TEXT("/Script/unrealGameJam.TDSquadManager"));
	WaveDirectorClass = FSoftClassPath(TEXT("/Script/unrealGameJam.TDWaveDirector"));
}

bool UTDFlowSettings::ResolveDestinationMap(ETDDestination Destination, FString& OutPackageName) const
{
	OutPackageName.Reset();
	const FSoftObjectPath* Path = DestinationMaps.Find(Destination);
	if (!Path || Path->IsNull())
	{
		return false;
	}
	const FString PackageName = Path->GetLongPackageName();
	if (PackageName.IsEmpty() || !FPackageName::DoesPackageExist(PackageName))
	{
		return false;
	}
	OutPackageName = PackageName;
	return true;
}
