// Wingmen-in-frame measurement (T3). Not in the default suite (it plays five waves, twice).
// Run: .\Tools\td-test.ps1 -Filter TDSquadFrame
//
// The same pilot every run (T4's seeded FTDBalancePilot, Weak skill: never switches, so the player is
// always Rift and Lancer / Bastion are always the AI wingmen) and a projection from the player's real
// chase camera, independent of any window. Reports, per wave and per targeting mode
// (td.Wingmen.InViewTargeting 0 / 1): engaging samples (an enemy inside the frame), the share of those
// with at least one wingman inside the frame, and how often a wingman sat on the aim line.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Waves/Balance/TDBalanceRecorder.h"
#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Squad/Tests/TDSquadTestListener.h"
#include "TriadDrift/Core/TDEventBus.h"

namespace TDSquadFrameMeasure
{
	/** -1..1 on each axis inside the 16:9 frame of Leader's chase camera; false when behind it. */
	static bool Project(const ATDShip* Leader, const FVector& P, FVector2D& Out)
	{
		const FTransform Cam = Leader->GetChaseCameraTransformNow();
		const FVector L = Cam.InverseTransformPositionNoScale(P);
		if (L.X <= 1.f)
		{
			return false;
		}
		const float TanH = FMath::Tan(FMath::DegreesToRadians(Leader->GetChaseCameraFOVNow() * 0.5f));
		const float TanV = TanH / (16.f / 9.f);
		Out = FVector2D(L.Y / (L.X * TanH), L.Z / (L.X * TanV));
		return true;
	}

	static bool InFrame(const ATDShip* Leader, const FVector& P, FVector2D& Out)
	{
		return Project(Leader, P, Out) && FMath::Abs(Out.X) <= 1.f && FMath::Abs(Out.Y) <= 1.f;
	}

	struct FWaveSample
	{
		int32 Engaging = 0;		// samples with an enemy in frame
		int32 WingSeen = 0;		// ... of which at least one wingman in frame too
		int32 OnAimLine = 0;	// samples with a wingman within ~8 deg of the screen centre, in front
		int32 Samples = 0;
	};

	static TArray<FWaveSample> Play(int32 Mode, float Step, float MaxSecondsPerWave)
	{
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Wingmen.InViewTargeting"));
		const int32 PrevMode = CVar ? CVar->GetInt() : 1;
		if (CVar)
		{
			CVar->Set(Mode, ECVF_SetByCode);
		}
		const ETDDifficulty PrevDiff = UTDDifficultySettings::GetCurrent();
		UTDDifficultySettings::SetCurrent(ETDDifficulty::Standard);

		TArray<FWaveSample> Out;
		{
			FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
			APlayerController* PC = W.SpawnPlayerController();
			ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
			Squad->SetSquadPlayerController(PC);
			Squad->SpawnSquad(FTransform::Identity);
			ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
			W.Tick(Step);
			FTDBalancePilot Pilot;
			Pilot.Skill = FTDPilotSkill::Weak();
			FTDBalanceWaveStats Ignored;
			const float AimCone = FMath::Tan(FMath::DegreesToRadians(8.f)) / FMath::Tan(FMath::DegreesToRadians(45.f));

			for (int32 Index = 0; Index < D->GetWaveCount(); ++Index)
			{
				FWaveSample S;
				D->StartWave(Index);
				float T = 0.f, NextSample = 0.f;
				while (T < MaxSecondsPerWave)
				{
					Pilot.Tick(W.GetWorld(), Squad, Step, Ignored);
					W.Tick(Step);
					T += Step;
					const ETDWaveState State = D->GetState();
					if (State != ETDWaveState::Active && State != ETDWaveState::WarpingIn)
					{
						break;
					}
					const ATDShip* Leader = Squad->GetActiveShip();
					if (T < NextSample || !Leader || !Squad->IsShipAlive(Squad->GetActiveShipId()))
					{
						continue;
					}
					NextSample = T + 0.25f;
					++S.Samples;
					bool bEnemy = false;
					for (TActorIterator<ATDEnemyShip> It(W.GetWorld()); It && !bEnemy; ++It)
					{
						const UTDHealthComponent* H = UTDStatics::GetHealth(*It);
						FVector2D P;
						bEnemy = (!H || !H->IsDead()) && InFrame(Leader, It->GetActorLocation(), P);
					}
					bool bWing = false, bAim = false;
					for (const ATDShip* Mate : Squad->GetLivingShips())
					{
						FVector2D P;
						if (Mate != Leader && InFrame(Leader, Mate->GetActorLocation(), P))
						{
							bWing = true;
							bAim |= P.Size() < AimCone;
						}
					}
					if (bEnemy)
					{
						++S.Engaging;
						S.WingSeen += bWing ? 1 : 0;
					}
					S.OnAimLine += bAim ? 1 : 0;
				}
				D->StopAndClear();
				W.Advance(D->BreatherSeconds, Step);
				Out.Add(S);
			}
		}
		UTDDifficultySettings::SetCurrent(PrevDiff);
		if (CVar)
		{
			CVar->Set(PrevMode, ECVF_SetByCode);
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadFrameMeasure, "TDSquadFrame.Standard.Weak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDSquadFrameMeasure::RunTest(const FString& Parameters)
{
	using namespace TDSquadFrameMeasure;
	for (const int32 Mode : { 0, 1 })
	{
		const TArray<FWaveSample> Waves = Play(Mode, 1.f / 30.f, 180.f);
		for (int32 i = 0; i < Waves.Num(); ++i)
		{
			const FWaveSample& S = Waves[i];
			AddInfo(FString::Printf(TEXT("TDFRAME mode=%d wave=%d engaging=%d wingSeen=%d (%.0f%%) aimLine=%d/%d"),
				Mode, i + 1, S.Engaging, S.WingSeen, S.Engaging > 0 ? 100.f * S.WingSeen / S.Engaging : 0.f, S.OnAimLine, S.Samples));
		}
	}
	return true;
}

// ------------------------------------------------------------------------------------------ survival

namespace TDSquadSurvivalMeasure
{
	struct FWave { int32 WingDowns = 0; int32 ActiveDowns = 0; int32 WingKills = 0; int32 PlayerKills = 0; float Seconds = 0.f; };

	static TArray<FWave> Play(int32 Mode, const FTDPilotSkill& Skill, float Step, float MaxSecondsPerWave)
	{
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Wingmen.Survival"));
		const int32 Prev = CVar ? CVar->GetInt() : 1;
		if (CVar) { CVar->Set(Mode, ECVF_SetByCode); }
		const ETDDifficulty PrevDiff = UTDDifficultySettings::GetCurrent();
		UTDDifficultySettings::SetCurrent(ETDDifficulty::Standard);
		TArray<FWave> Out;
		{
			FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
			UTDSquadTestListener* L = NewObject<UTDSquadTestListener>();
			L->AddToRoot();
			L->Listen(UTDEventBus::Get(W.GetWorld()));
			APlayerController* PC = W.SpawnPlayerController();
			ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
			Squad->SetSquadPlayerController(PC);
			Squad->SpawnSquad(FTransform::Identity);
			ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
			W.Tick(Step);
			FTDBalancePilot Pilot;
			Pilot.Skill = Skill;
			FTDBalanceWaveStats Ignored;
			for (int32 Index = 0; Index < D->GetWaveCount(); ++Index)
			{
				FWave S;
				L->SquadForKills = Squad;
				L->PlayerKills = 0;
				L->WingKills = 0;
				TMap<ETDShipId, bool> WasAlive;
				for (const ETDShipId Id : UTDShipLibrary::AllShips()) { WasAlive.Add(Id, Squad->IsShipAlive(Id)); }
				ETDShipId WasActive = Squad->GetActiveShipId();
				D->StartWave(Index);
				float T = 0.f;
				while (T < MaxSecondsPerWave)
				{
					Pilot.Tick(W.GetWorld(), Squad, Step, Ignored);
					W.Tick(Step);
					T += Step;
					for (const ETDShipId Id : UTDShipLibrary::AllShips())
					{
						const bool bAlive = Squad->IsShipAlive(Id);
						if (WasAlive[Id] && !bAlive)
						{
							(Id == WasActive ? S.ActiveDowns : S.WingDowns)++;
						}
						WasAlive[Id] = bAlive;
					}
					WasActive = Squad->GetActiveShipId();
					const ETDWaveState State = D->GetState();
					if (State != ETDWaveState::Active && State != ETDWaveState::WarpingIn) { break; }
					if (Squad->GetLivingShips().Num() == 0)
					{
						Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector::ZeroVector));
						D->RestartCurrentWave();
					}
				}
				S.Seconds = T;
				S.PlayerKills = L->PlayerKills;
				S.WingKills = L->WingKills;
				D->StopAndClear();
				W.Advance(D->BreatherSeconds, Step);
				Out.Add(S);
			}
			L->StopListening();
			L->RemoveFromRoot();
		}
		UTDDifficultySettings::SetCurrent(PrevDiff);
		if (CVar) { CVar->Set(Prev, ECVF_SetByCode); }
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadSurvivalMeasure, "TDSquadSurvival.Standard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FTDSquadSurvivalMeasure::RunTest(const FString& Parameters)
{
	// Wingman survival on/off with T4's pilots: wingman downs, player-ship downs and kills per wave.
	const int32 Runs = 5;
	for (const FTDPilotSkill& Skill : { FTDPilotSkill::Weak(), FTDPilotSkill::Average() })
	{
		for (const int32 Mode : { 0, 1 })
		{
			TArray<TDSquadSurvivalMeasure::FWave> Sum;
			Sum.SetNum(5);
			for (int32 r = 0; r < Runs; ++r)
			{
				const TArray<TDSquadSurvivalMeasure::FWave> One = TDSquadSurvivalMeasure::Play(Mode, Skill, 1.f / 30.f, 180.f);
				for (int32 w = 0; w < One.Num() && w < 5; ++w)
				{
					Sum[w].WingDowns += One[w].WingDowns; Sum[w].ActiveDowns += One[w].ActiveDowns;
					Sum[w].WingKills += One[w].WingKills; Sum[w].PlayerKills += One[w].PlayerKills; Sum[w].Seconds += One[w].Seconds;
				}
			}
			for (int32 w = 0; w < 5; ++w)
			{
				const TDSquadSurvivalMeasure::FWave& S = Sum[w];
				AddInfo(FString::Printf(TEXT("TDSURV pilot=%s mode=%d wave=%d runs=%d wingDowns=%d activeDowns=%d wingKills=%d playerKills=%d wingKPM=%.2f sec=%.0f"),
					*Skill.Name.ToString(), Mode, w + 1, Runs, S.WingDowns, S.ActiveDowns, S.WingKills, S.PlayerKills,
					S.Seconds > 0.f ? S.WingKills / (S.Seconds / 60.f) : 0.f, S.Seconds));
			}
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
