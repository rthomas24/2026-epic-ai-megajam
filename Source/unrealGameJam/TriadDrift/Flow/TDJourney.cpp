// Triad Drift - journey stages. Owner: Teammate 5.

#include "TriadDrift/Flow/TDJourney.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "Engine/World.h"

namespace TDJourneyImpl
{
	static void AddVerdance(FTDProgress& P)
	{
		for (const FName& Id : UTDFlowSettings::Get()->VerdanceObjectives) { P.CompletedObjectives.Add(Id); }
		P.UnlockedAbilities.Add(ETDAbilityId::GravityWell, ETDShipId::Bastion);
		P.UnlockedAbilities.Add(ETDAbilityId::Cloak, ETDShipId::Rift);
	}
	static void AddCinder(FTDProgress& P)
	{
		for (const FName& Id : UTDFlowSettings::Get()->CinderObjectives) { P.CompletedObjectives.Add(Id); }
		P.UnlockedAbilities.Add(ETDAbilityId::EMPPulse, ETDShipId::Lancer);
	}
}

const FName FTDJourney::DominionApproach(TEXT("Dominion_Approach"));
const FName FTDJourney::DominionTrench(TEXT("Dominion_Trench"));
const FName FTDJourney::InteriorHangar(TEXT("Hangar"));

bool FTDJourney::FindStageStart(UWorld* World, FName ReturnPoint, FTransform& Out)
{
	if (!World || ReturnPoint.IsNone())
	{
		return false;
	}
	if (ReturnPoint == DominionApproach || ReturnPoint == DominionTrench)
	{
		if (const ATDDominion* Dominion = ATDDominion::Get(World))
		{
			// From the hull's real size (the r8 art hull is much bigger than the old box): T4's ATDDominion owns it.
			Out = Dominion->GetStageStart(ReturnPoint == DominionTrench);
			return true;
		}
	}
	if (ReturnPoint == TDReturnPoints::DominionHangarExit)
	{
		if (const ATDDominion* Dominion = ATDDominion::Get(World))
		{
			// ENG-31: the escape flight starts out of the real bay mouth (T4's ATDDominion owns it).
			Out = Dominion->GetEscapeStart();
			return true;
		}
	}
	const ATDGalaxyLayout* Layout = ATDGalaxyLayout::Get(World);
	return Layout && Layout->FindReturnPoint(ReturnPoint, Out);
}

FTDProgress FTDJourney::MakeProgress(ETDJourneyStage Stage, const FTDProgress& Keep)
{
	FTDProgress P;
	P.Takes = Keep.Takes;
	P.Stats = Keep.Stats;
	P.RunTimeSeconds = Keep.RunTimeSeconds;
	P.LastShip = Keep.LastShip;
	P.ReturnPoint = ReturnPointOf(Stage);
	switch (Stage)
	{
	case ETDJourneyStage::Waves1to2:
		P.Phase = ETDGamePhase::Waves1to2;
		break;
	case ETDJourneyStage::VerdanceArrival:
	case ETDJourneyStage::VerdanceOnFoot:
		P.Phase = ETDGamePhase::Verdance; P.WavesCleared = 2;
		break;
	case ETDJourneyStage::Waves3to4:
		P.Phase = ETDGamePhase::Waves3to4; P.WavesCleared = 2;
		TDJourneyImpl::AddVerdance(P);
		break;
	case ETDJourneyStage::CinderArrival:
	case ETDJourneyStage::CinderOnFoot:
		P.Phase = ETDGamePhase::Cinder; P.WavesCleared = 4;
		TDJourneyImpl::AddVerdance(P);
		break;
	case ETDJourneyStage::Wave5:
		P.Phase = ETDGamePhase::Wave5; P.WavesCleared = 4;
		TDJourneyImpl::AddVerdance(P); TDJourneyImpl::AddCinder(P);
		break;
	case ETDJourneyStage::DominionAssault:
		P.Phase = ETDGamePhase::DominionAssault; P.WavesCleared = 5; P.bDominionRevealed = true;
		TDJourneyImpl::AddVerdance(P); TDJourneyImpl::AddCinder(P);
		break;
	case ETDJourneyStage::ReactorExposed:
		P.Phase = ETDGamePhase::DominionAssault; P.WavesCleared = 5; P.bDominionRevealed = true; P.bDominionDefensesDown = true;
		TDJourneyImpl::AddVerdance(P); TDJourneyImpl::AddCinder(P);
		break;
	case ETDJourneyStage::DominionDocking:
		P.Phase = ETDGamePhase::DominionAssault; P.WavesCleared = 5; P.bDominionRevealed = true; P.bDominionDefensesDown = true;
		TDJourneyImpl::AddVerdance(P); TDJourneyImpl::AddCinder(P);
		break;
	case ETDJourneyStage::DominionInterior:
		P.Phase = ETDGamePhase::DominionInterior; P.WavesCleared = 5; P.bDominionRevealed = true; P.bDominionDefensesDown = true;
		P.bDominionBoarded = true;
		TDJourneyImpl::AddVerdance(P); TDJourneyImpl::AddCinder(P);
		break;
	case ETDJourneyStage::EscapeFlight:
		P.Phase = ETDGamePhase::Escape; P.WavesCleared = 5; P.bDominionRevealed = true; P.bDominionDefensesDown = true;
		P.bDominionBoarded = true; P.bReactorDestroyed = true;
		TDJourneyImpl::AddVerdance(P); TDJourneyImpl::AddCinder(P);
		break;
	case ETDJourneyStage::Victory:
		P.Phase = ETDGamePhase::Victory; P.WavesCleared = 5; P.bDominionRevealed = true; P.bDominionDefensesDown = true;
		TDJourneyImpl::AddVerdance(P); TDJourneyImpl::AddCinder(P);
		break;
	}
	return P;
}

ETDDestination FTDJourney::DestinationOf(ETDJourneyStage Stage)
{
	switch (Stage)
	{
	case ETDJourneyStage::VerdanceArrival:
	case ETDJourneyStage::VerdanceOnFoot: return ETDDestination::Verdance;
	case ETDJourneyStage::CinderArrival:
	case ETDJourneyStage::CinderOnFoot: return ETDDestination::Cinder;
	case ETDJourneyStage::DominionInterior: return ETDDestination::DominionInterior;
	default: return ETDDestination::Galaxy;
	}
}

FName FTDJourney::ReturnPointOf(ETDJourneyStage Stage)
{
	switch (Stage)
	{
	case ETDJourneyStage::VerdanceOnFoot: return TEXT("Verdance_Pad");
	case ETDJourneyStage::CinderOnFoot: return TEXT("Cinder_Pad");
	case ETDJourneyStage::Waves3to4: return TDReturnPoints::VerdanceOrbit;
	case ETDJourneyStage::Wave5: return TDReturnPoints::CinderOrbit;
	case ETDJourneyStage::DominionAssault:
	case ETDJourneyStage::DominionDocking: return DominionApproach;
	case ETDJourneyStage::DominionInterior: return InteriorHangar;
	case ETDJourneyStage::EscapeFlight: return TDReturnPoints::DominionHangarExit;
	case ETDJourneyStage::ReactorExposed:
	case ETDJourneyStage::Victory: return DominionTrench;
	default: return NAME_None;
	}
}

bool FTDJourney::Go(UWorld* World, ETDJourneyStage Stage, int32 Wave)
{
	UTDGameInstance* GI = World ? Cast<UTDGameInstance>(World->GetGameInstance()) : nullptr;
	if (!GI)
	{
		return false;
	}
	const ETDDestination Dest = DestinationOf(Stage);
	const FTDProgress Keep = GI->GetProgressRef();
	GI->StartNewRun(); // a cheat-built run may write the slot like any run
	GI->GetMutableProgress() = MakeProgress(Stage, Keep);
	if (Wave > 0)
	{
		GI->GetMutableProgress().WavesCleared = Wave - 1; // the director starts the next uncleared wave
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(World))
	{
		Bus->BroadcastPhaseChanged(GI->GetProgressRef().Phase);
	}

	// The escape flight needs a fresh galaxy (T4: the Dominion switches to EscapeFlight only from Hidden).
	if (Dest == ETDDestination::Galaxy && GI->GetCurrentDestination() == ETDDestination::Galaxy && Stage != ETDJourneyStage::EscapeFlight)
	{
		ApplyGalaxyInPlace(World, Stage);
		GI->SaveCheckpoint();
		return true;
	}
	return GI->TravelTo(Dest, GI->GetProgressRef().ReturnPoint);
}

void FTDJourney::ApplyGalaxyInPlace(UWorld* World, ETDJourneyStage Stage)
{
	ATDWaveDirector* Director = ATDWaveDirector::Get(World);
	if (Director)
	{
		Director->StopAndClear(); // no wave 2 left running under a later stage
	}
	if (ATDDominion* Dominion = ATDDominion::Get(World))
	{
		Dominion->SyncWithProgress(); // warps in (assault) or straight to the reactor (defenses down)
		if (Stage == ETDJourneyStage::ReactorExposed || Stage == ETDJourneyStage::DominionDocking || Stage == ETDJourneyStage::Victory)
		{
			if (Dominion->GetState() == ETDDominionState::ShieldsUp || Dominion->GetState() == ETDDominionState::WarpingIn)
			{
				Dominion->DebugDropShields();
			}
		}
	}
	// The squad goes where the stage starts.
	if (ATDSquadManager* Squad = ATDSquadManager::Get(World))
	{
		FTransform Where;
		const ATDGalaxyLayout* Layout = ATDGalaxyLayout::Get(World);
		const FName Point = ReturnPointOf(Stage);
		// A staged wave 3-5 is fought where waves are fought: the combat zone. (MAJOR B holds a wave until the squad is
		// there; the real return from a planet still lands at its orbit point, from the progress' ReturnPoint.)
		const bool bWaveStage = Stage == ETDJourneyStage::Waves3to4 || Stage == ETDJourneyStage::Wave5;
		if (Layout && (bWaveStage || !FindStageStart(World, Point, Where)))
		{
			Where = Layout->GetPlayerStart();
		}
		if (Layout)
		{
			Squad->SpawnSquad(Where);
		}
	}
	if (Stage == ETDJourneyStage::Victory)
	{
		if (UTDGameInstance* GI = Cast<UTDGameInstance>(World->GetGameInstance()))
		{
			GI->GetMutableProgress().Phase = ETDGamePhase::DominionAssault; // so SetPhase broadcasts and stops the clock
			GI->SetPhase(ETDGamePhase::Victory);
		}
		return;
	}
	if (Director)
	{
		Director->StartFromProgress(); // the stage's own wave (or idle for the assault)
	}
	if (UTDGameInstance* GI = Cast<UTDGameInstance>(World->GetGameInstance()))
	{
		GI->BeginDominionSceneIfAssault(); // the assault stages play as the "Dominion" scene
	}
}

bool FTDJourney::Parse(const FString& Name, ETDJourneyStage& Out, int32* OutWave)
{
	if (OutWave)
	{
		*OutWave = 0;
	}
	if (Name.StartsWith(TEXT("Wave"), ESearchCase::IgnoreCase) && Name.Len() == 5 && FChar::IsDigit(Name[4]))
	{
		const int32 N = Name[4] - TEXT('0');
		if (N < 1 || N > 5)
		{
			return false;
		}
		Out = N <= 2 ? ETDJourneyStage::Waves1to2 : (N <= 4 ? ETDJourneyStage::Waves3to4 : ETDJourneyStage::Wave5);
		if (OutWave)
		{
			*OutWave = N;
		}
		return true;
	}
	const UEnum* E = StaticEnum<ETDJourneyStage>();
	for (int32 i = 0; E && i < E->NumEnums() - 1; ++i)
	{
		if (E->GetNameStringByIndex(i).Equals(Name, ESearchCase::IgnoreCase))
		{
			Out = static_cast<ETDJourneyStage>(E->GetValueByIndex(i));
			return true;
		}
	}
	return false;
}

FString FTDJourney::ListStages()
{
	const UEnum* E = StaticEnum<ETDJourneyStage>();
	TArray<FString> Names;
	for (int32 i = 0; E && i < E->NumEnums() - 1; ++i)
	{
		Names.Add(E->GetNameStringByIndex(i));
	}
	return FString::Join(Names, TEXT(", "));
}
