// Triad Drift - combat game-thread timing (Teammate 2; Ryan's performance instruction, issue #89). A few named
// accumulators, off by default, switched on by the combat benchmark test (TriadDrift.Perf.CombatWave5) or
// TD.Perf.Combat 1 in a running game (TD.Perf.CombatDump prints and resets them). One branch when off.

#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformTime.h"

enum class ETDCombatPerf : uint8
{
	Projectiles,		// ATDProjectile::Tick (flight, sweeps, homing, impacts)
	ProjectileLaunch,	// ATDProjectile::Spawn / Launch (pool, activate, look)
	Weapons,			// UTDWeaponComponent::TickComponent (fire, heat, ammo)
	Targeting,			// UTDTargetingComponent::TickComponent
	Powers,				// UTDPowerComponent / missile swarm / ram ticks
	Pickups,			// UTDPickupSubsystem::Tick
	FlightHUD,			// UTDFlightHUDWidget::Sample
	Flashes,			// combat flashes and placeholder flashes
	Count
};

struct UNREALGAMEJAM_API FTDCombatPerf
{
	static bool bEnabled;
	static uint64 Cycles[static_cast<int32>(ETDCombatPerf::Count)];
	static uint32 Calls[static_cast<int32>(ETDCombatPerf::Count)];

	static void Reset();
	static double Milliseconds(ETDCombatPerf Which) { return FPlatformTime::ToMilliseconds64(Cycles[static_cast<int32>(Which)]); }
	static const TCHAR* Name(ETDCombatPerf Which);
	/** One line per accumulator: total ms, calls, ms per frame over Frames. */
	static FString Report(int32 Frames);
};

/** Times its scope into Which while FTDCombatPerf::bEnabled. */
struct FTDCombatPerfScope
{
	ETDCombatPerf Which;
	uint64 Start = 0;
	explicit FTDCombatPerfScope(ETDCombatPerf InWhich) : Which(InWhich)
	{
		if (FTDCombatPerf::bEnabled) { Start = FPlatformTime::Cycles64(); }
	}
	~FTDCombatPerfScope()
	{
		if (Start)
		{
			FTDCombatPerf::Cycles[static_cast<int32>(Which)] += FPlatformTime::Cycles64() - Start;
			++FTDCombatPerf::Calls[static_cast<int32>(Which)];
		}
	}
};

#define TD_COMBAT_PERF(Which) FTDCombatPerfScope UE_JOIN(TDCombatPerf_, __LINE__)(ETDCombatPerf::Which)
