// Triad Drift - per-ship accuracy (issue #192; Teammate 2). See the header.

#include "TriadDrift/Combat/TDAccuracyStats.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDTypes.h"

int32 FTDAccuracyStats::Shots[3][2] = {};
int32 FTDAccuracyStats::Hits[3][2] = {};
int32 FTDAccuracyStats::Kills[3][2] = {};
double FTDAccuracyStats::FlownSeconds[3] = {};
double FTDAccuracyStats::WindowStart = 0.0;
TSet<uint32> FTDAccuracyStats::HitRoots;

void FTDAccuracyStats::Reset(double Now)
{
	FMemory::Memzero(Shots);
	FMemory::Memzero(Hits);
	FMemory::Memzero(Kills);
	FMemory::Memzero(FlownSeconds);
	HitRoots.Reset();
	WindowStart = Now;
}

void FTDAccuracyStats::NoteShot(int32 Ship, int32 Slot)
{
	if (Ship >= 0 && Ship < 3 && Slot >= 0 && Slot < 2)
	{
		++Shots[Ship][Slot];
	}
}

void FTDAccuracyStats::NoteHit(int32 Ship, int32 Slot, uint32 Root, bool bKill)
{
	if (Ship < 0 || Ship >= 3 || Slot < 0 || Slot >= 2)
	{
		return;
	}
	bool bSeen = false;
	HitRoots.Add(Root, &bSeen);
	if (!bSeen)
	{
		++Hits[Ship][Slot];
	}
	if (bKill)
	{
		++Kills[Ship][Slot];
	}
}

void FTDAccuracyStats::NoteFlown(int32 Ship, double Seconds)
{
	if (Ship >= 0 && Ship < 3)
	{
		FlownSeconds[Ship] += Seconds;
	}
}

FString FTDAccuracyStats::Report()
{
	static const TCHAR* Names[3] = { TEXT("Rift"), TEXT("Lancer"), TEXT("Bastion") };
	FString Out;
	for (int32 Ship = 0; Ship < 3; ++Ship)
	{
		const int32 S = Shots[Ship][0] + Shots[Ship][1];
		const int32 H = Hits[Ship][0] + Hits[Ship][1];
		const int32 K = Kills[Ship][0] + Kills[Ship][1];
		const double Min = FlownSeconds[Ship] / 60.0;
		Out += FString::Printf(TEXT("\n  %-7s flown %5.1f s | primary %4d shots %4d hits %5.1f%% %3d kills | secondary %4d shots %4d hits %5.1f%% %3d kills | all %5.1f%% hits, %.1f kills/min"),
			Names[Ship], FlownSeconds[Ship],
			Shots[Ship][0], Hits[Ship][0], Shots[Ship][0] > 0 ? 100.0 * Hits[Ship][0] / Shots[Ship][0] : 0.0, Kills[Ship][0],
			Shots[Ship][1], Hits[Ship][1], Shots[Ship][1] > 0 ? 100.0 * Hits[Ship][1] / Shots[Ship][1] : 0.0, Kills[Ship][1],
			S > 0 ? 100.0 * H / S : 0.0, Min > 0.0 ? K / Min : 0.0);
	}
	return Out;
}

#if !UE_BUILD_SHIPPING
namespace TDAccuracyDebug
{
	static FAutoConsoleCommandWithWorldAndArgs CmdAccuracy(TEXT("TD.Weapons.Accuracy"),
		TEXT("TD.Weapons.Accuracy [reset]: the flown ship's shots, hits and kills per weapon since the last reset (#192)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const double Now = World ? World->GetTimeSeconds() : 0.0;
			if (Args.Num() > 0 && Args[0].Equals(TEXT("reset"), ESearchCase::IgnoreCase))
			{
				FTDAccuracyStats::Reset(Now);
				UE_LOG(LogTriadDrift, Display, TEXT("TD.Weapons.Accuracy: reset"));
				return;
			}
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Weapons.Accuracy over %.1f s:%s"), Now - FTDAccuracyStats::WindowStart, *FTDAccuracyStats::Report());
		}));
}
#endif
