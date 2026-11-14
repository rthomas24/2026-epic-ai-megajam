// Triad Drift - objective lines for flow-owned scenes. Owner: Teammate 5.

#include "TriadDrift/Flow/TDObjectiveText.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Story/TDTutorialPhases.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDockingBay.h"
#include "TriadDrift/Ground/Planets/TDPlanetDirector.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "Engine/World.h"
#include "EngineUtils.h"

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
		case ETDGamePhase::Escape:
		{
			// The escape flight (T4, ENG-31): fly clear of the breaking Dominion; the seconds left from its clock.
			const ATDDominion* D = ATDDominion::Get(World);
			const int32 Left = D && D->GetState() == ETDDominionState::EscapeFlight ? FMath::CeilToInt(D->GetEscapeTimeLeft()) : 0;
			Out = Left > 0 ? Line(TEXT("Escape_Flight"), 0, FText::Format(LOCTEXT("EscapeFlightLeft", "GET CLEAR OF THE DOMINION · {0}"), Left))
				: Line(TEXT("Escape_Flight"), 0, LOCTEXT("EscapeFlight", "GET CLEAR OF THE DOMINION"));
			return true;
		}
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
			case ETDDominionState::DockingOpen:
				Out = Line(TEXT("Dominion_Dock"), 0, LOCTEXT("Dock", "THE SHIELD IS DOWN · DOCK WITH THE DOMINION"));
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
		if (Director && Director->IsAwaitingCombatZone())
		{
			Out = Line(TEXT("Return_CombatZone"), 0, LOCTEXT("ReturnToZone", "RETURN TO THE COMBAT ZONE"));
			return true;
		}
		const FTDWaveInfo Info = Director ? Director->GetWaveInfo() : FTDWaveInfo();
		// The tutorial's end (T7's play, Nov 14: the line still read CLEAR THE AREA under Command's order to pull back):
		// from the order, the withdrawal; past the line, the jump point (T4's waypoint TD.Tutorial.JumpPoint) until the jump.
		// Placeholder words; the storyline's are Nicholas's.
		if (Director && Director->GetTutorialPhase() == ETDTutorialPhase::Withdrawal)
		{
			Out = Line(TEXT("Tutorial_Withdraw"), Info.WaveNumber, LOCTEXT("Withdraw", "WITHDRAW · FOLLOW THE MARKER"));
			return true;
		}
		if (Director && Director->GetTutorialPhase() == ETDTutorialPhase::Escaped && Info.WaveNumber <= 2)
		{
			bool bJumpPoint = false;
			for (TActorIterator<AActor> It(World); It && !bJumpPoint; ++It) { bJumpPoint = It->ActorHasTag(TEXT("TD.Tutorial.JumpPoint")); }
			if (bJumpPoint || !Info.bWaveActive)
			{
				Out = Line(TEXT("Tutorial_JumpPoint"), Info.WaveNumber, LOCTEXT("JumpPoint", "REACH THE JUMP POINT"));
				return true;
			}
		}
		if (Info.bWaveActive && Info.WaveNumber > 0)
		{
			const FName Scene = Info.SceneId.IsNone() ? FName(*FString::Printf(TEXT("Wave%d"), Info.WaveNumber)) : Info.SceneId;
			// ENG-40 (T3): the tutorial's taught phase prompt over waves 1-2, while one is up (PromptId is for HUDs that want it).
			{
				FName PromptId;
				FText Prompt;
				if (UTDTutorialPhases::GetPrompt(World, PromptId, Prompt))
				{
					Out.Id = Scene; // the scene stays the wave (slates, takes); only the line changes
					Out.Step = Info.WaveNumber;
					Out.Text = Prompt;
					return true;
				}
			}
			Out.Id = Scene;
			Out.Step = Info.WaveNumber;
			// storyline.md section 13: nobody names the enemy (and Nicholas has no approved wave line yet).
			// T4's per-wave objective (126b238d: wave 5 "BREAK THE DOMINION GUARD"), else the generic line.
			Out.Text = !Info.ObjectiveText.IsEmpty() ? Info.ObjectiveText : FText::Format(LOCTEXT("Wave", "WAVE {0} · CLEAR THE AREA"), Info.WaveNumber);
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
			// Between objectives: "HEAD TO A SIGNAL · THE WRECK OR THE RELAY".
			TArray<AActor*> Unused;
			if (const ATDPlanetCourse* Course = Planet->GetCourse(); Course && Course->GetIdleLine(Out.Text, Unused))
			{
				Out.Id = Destination == ETDDestination::Verdance ? FName(TEXT("Choose_Verdance")) : FName(TEXT("Choose_Cinder"));
				Out.Step = 0;
				return true;
			}
			return false; // the running objective owns the line
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
	case ETDDestination::DominionInterior:
	{
		// Between rooms: "HEAD TO THE FLIGHT DECK" (the running room's objective owns the line otherwise).
		TArray<AActor*> Unused;
		const ATDPlanetCourse* Interior = ATDPlanetCourse::Get(World);
		if (Interior && Interior->IsInterior() && Interior->GetIdleLine(Out.Text, Unused))
		{
			Out.Id = TEXT("Choose_Interior");
			Out.Step = 0;
			return true;
		}
		return false;
	}
	default:
		return false;
	}
}

void FTDObjectiveText::ComputeMarkers(UWorld* World, const FTDProgress& Progress, ETDDestination Destination, TArray<AActor*>& Out)
{
	Out.Reset();
	if (!World)
	{
		return;
	}
	if (Destination == ETDDestination::Galaxy && (Progress.Phase == ETDGamePhase::Verdance || Progress.Phase == ETDGamePhase::Cinder))
	{
		// "JUMP TO VERDANCE": the planet's atmosphere (its centre) is where to go.
		const ETDDestination Planet = Progress.Phase == ETDGamePhase::Verdance ? ETDDestination::Verdance : ETDDestination::Cinder;
		if (const ATDGalaxyLayout* Layout = ATDGalaxyLayout::Get(World))
		{
			for (ATDPlanetAtmosphere* A : Layout->GetPlanets())
			{
				if (A && A->GetDestination() == Planet)
				{
					Out.Add(A);
				}
			}
		}
		return;
	}
	if (Destination == ETDDestination::Galaxy)
	{
		if (const ATDWaveDirector* Director = ATDWaveDirector::Get(World); Director && Director->GetCombatZoneMarker())
		{
			Out.Add(Director->GetCombatZoneMarker()); // "RETURN TO THE COMBAT ZONE"
			return;
		}
	}
	if (Destination == ETDDestination::Galaxy && Progress.Phase == ETDGamePhase::DominionAssault)
	{
		if (const ATDDominion* D = ATDDominion::Get(World))
		{
			if (D->GetState() == ETDDominionState::ShieldsUp)
			{
				for (ATDShieldGenerator* G : D->GetGenerators())
				{
					if (G && G->IsAlive())
					{
						Out.Add(G);
					}
				}
			}
			else if (D->GetState() == ETDDominionState::DockingOpen)
			{
				if (AActor* Bay = (AActor*)D->GetDockingBay()) { Out.Add(Bay); }	// ENG-29: the marker leads to the bay
			}
			else if (D->GetState() == ETDDominionState::ReactorExposed)
			{
				ATDDominionReactor* R = D->GetReactor();
				if (R && R->IsAlive())
				{
					Out.Add(R);
				}
			}
		}
		return;
	}
	if (Destination == ETDDestination::Verdance || Destination == ETDDestination::Cinder)
	{
		const ATDPlanetDirector* Planet = ATDPlanetDirector::Get(World);
		if (!Planet || !Planet->IsOnFoot())
		{
			return; // flying: T1 tags the pad
		}
		if (TDObjectiveTextImpl::PlanetDone(Progress, Destination))
		{
			if (ATDBoardShipInteractable* Board = Planet->GetBoardPoint())
			{
				Out.Add(Board);
			}
		}
		else if (const UTDObjectiveSubsystem* Objectives = UTDObjectiveSubsystem::Get(World))
		{
			Objectives->GetActiveWaypoints(Out);
			if (Out.Num() == 0 && Planet->GetCourse())
			{
				FText Unused;
				Planet->GetCourse()->GetIdleLine(Unused, Out); // the trailheads still open
			}
		}
	}
	if (Destination == ETDDestination::DominionInterior)
	{
		if (const UTDObjectiveSubsystem* Objectives = UTDObjectiveSubsystem::Get(World))
		{
			Objectives->GetActiveWaypoints(Out);
		}
		const ATDPlanetCourse* Interior = ATDPlanetCourse::Get(World);
		if (Out.Num() == 0 && Interior && Interior->IsInterior())
		{
			FText Unused;
			Interior->GetIdleLine(Unused, Out); // the next room's door
		}
	}
}

#undef LOCTEXT_NAMESPACE
