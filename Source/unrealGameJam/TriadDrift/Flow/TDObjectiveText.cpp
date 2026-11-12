// Triad Drift - objective lines for flow-owned scenes. Owner: Teammate 5.

#include "TriadDrift/Flow/TDObjectiveText.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/Ground/Planets/TDPlanetDirector.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "TDObjectiveText"

namespace TDObjectiveTextImpl
{
	static FTDObjectiveLine Line(const TCHAR* Id, int32 Step, const FText& Text)
	{
		FTDObjectiveLine L;
		L.Id = FName(Id);
		L.Step = Step;
		L.Text = Text;
		return L;
	}

	static bool PlanetDone(const FTDProgress& P, ETDDestination Planet)
	{
		const UTDFlowSettings* S = UTDFlowSettings::Get();
		const TArray<FName>& Ids = Planet == ETDDestination::Verdance ? S->VerdanceObjectives : S->CinderObjectives;
		for (const FName& Id : Ids)
		{
			if (!P.CompletedObjectives.Contains(Id))
			{
				return false;
			}
		}
		return Ids.Num() > 0;
	}

	static bool Galaxy(UWorld* World, const FTDProgress& P, FTDObjectiveLine& Out)
	{
		switch (P.Phase)
		{
		case ETDGamePhase::Verdance:
			Out = Line(TEXT("Travel_Verdance"), 0, LOCTEXT("JumpVerdance", "JUMP TO VERDANCE"));
			return true;
		case ETDGamePhase::Cinder:
			Out = Line(TEXT("Travel_Cinder"), 0, LOCTEXT("JumpCinder", "JUMP TO CINDER"));
			return true;
		case ETDGamePhase::DominionAssault:
		{
			const ATDDominion* D = ATDDominion::Get(World);
			const ETDDominionState State = D ? D->GetState() : ETDDominionState::ShieldsUp;
			switch (State)
			{
			case ETDDominionState::Hidden:
			case ETDDominionState::WarpingIn:
				Out = Line(TEXT("Dominion_Reveal"), 0, LOCTEXT("DominionArrives", "THE DOMINION IS HERE · STAND BY"));
				return true;
			case ETDDominionState::ReactorExposed:
				Out = Line(TEXT("Dominion_Reactor"), 0, LOCTEXT("Reactor", "FLY THE TRENCH · DESTROY THE REACTOR"));
				return true;
			case ETDDominionState::Breakup:
			case ETDDominionState::Finished:
				Out = Line(TEXT("Dominion_Breakup"), 0, LOCTEXT("Breakup", "GET CLEAR OF THE DOMINION"));
				return true;
			default:
			{
				const int32 Count = FMath::Max(D ? D->GetGenerators().Num() : 0, 4);
				const int32 Down = D ? FMath::Clamp(Count - D->GetGeneratorsRemaining(), 0, Count) : 0;
				Out = Line(TEXT("Dominion_Shields"), Down, FText::Format(LOCTEXT("Generators", "DESTROY THE SHIELD GENERATORS  {0} / {1}"), Down, Count));
				return true;
			}
			}
		}
		case ETDGamePhase::Victory:
			Out = Line(TEXT("Victory"), 0, LOCTEXT("Victory", "VICTORY · THAT'S A WRAP"));
			return true;
		default:
			break;
		}
		// Wave phases (and Intro).
		const ATDWaveDirector* Director = ATDWaveDirector::Get(World);
		const FTDWaveInfo Info = Director ? Director->GetWaveInfo() : FTDWaveInfo();
		if (Info.bWaveActive && Info.WaveNumber > 0)
		{
			const FName Scene = Info.SceneId.IsNone() ? FName(*FString::Printf(TEXT("Wave%d"), Info.WaveNumber)) : Info.SceneId;
			Out.Id = Scene;
			Out.Step = Info.WaveNumber;
			Out.Text = FText::Format(LOCTEXT("Wave", "WAVE {0} · CLEAR THE NULL ARMADA"), Info.WaveNumber);
			return true;
		}
		const int32 Next = FMath::Max(Info.WaveNumber, P.WavesCleared) + 1;
		Out.Id = FName(*FString::Printf(TEXT("Wave%d_Incoming"), Next));
		Out.Step = Next;
		Out.Text = FText::Format(LOCTEXT("Incoming", "WAVE {0} INCOMING"), Next);
		return true;
	}
}

bool FTDObjectiveText::Compute(UWorld* World, const FTDProgress& Progress, ETDDestination Destination, FTDObjectiveLine& Out)
{
	using namespace TDObjectiveTextImpl;
	if (!World)
	{
		return false;
	}
	if (Destination == ETDDestination::Victory || Progress.Phase == ETDGamePhase::Victory)
	{
		Out = Line(TEXT("Victory"), 0, LOCTEXT("Victory", "VICTORY · THAT'S A WRAP"));
		return true;
	}
	switch (Destination)
	{
	case ETDDestination::Galaxy:
		return Galaxy(World, Progress, Out);
	case ETDDestination::Verdance:
	case ETDDestination::Cinder:
	{
		const bool bDone = PlanetDone(Progress, Destination);
		const ATDPlanetDirector* Planet = ATDPlanetDirector::Get(World);
		if (Planet && Planet->IsOnFoot())
		{
			if (bDone)
			{
				Out = Line(Destination == ETDDestination::Verdance ? TEXT("Return_Verdance") : TEXT("Return_Cinder"), 0, LOCTEXT("ReturnToShip", "RETURN TO YOUR SHIP"));
				return true;
			}
			return false; // the planet's objective actors own the line
		}
		if (bDone)
		{
			Out = Line(Destination == ETDDestination::Verdance ? TEXT("Leave_Verdance") : TEXT("Leave_Cinder"), 0, LOCTEXT("TakeOff", "TAKE OFF · CLIMB TO ORBIT"));
		}
		else
		{
			Out = Line(Destination == ETDDestination::Verdance ? TEXT("Land_Verdance") : TEXT("Land_Cinder"), 0, LOCTEXT("Land", "LAND AT THE BEACON"));
		}
		return true;
	}
	default:
		return false;
	}
}

#undef LOCTEXT_NAMESPACE
