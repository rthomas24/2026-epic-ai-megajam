// Triad Drift - Squad. Shadow mode (issue #201): the tunables and the dev commands.
//
//   TD.Squad.Shadow [0|1]   set shadow mode (no argument: toggle)
//   TD.Squad.Dump           log each wingman's mode, state, target, distance to the player and power states
//
// Distances are measured from the game as it was before shadow mode: wingmen flew 15-25 m off the leader, picked
// enemies within 250 m of it and broke off 350 m out; guns reach 200 m; the radar's edge is 1500 m.

#include "TriadDrift/Squad/TDShadowMode.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

namespace TDShadowPrivate
{
	// --- Shadow ON ---
	static TAutoConsoleVariable<float> CVarLeash(TEXT("td.Squad.Shadow.Leash"), 18000.f,
		TEXT("Shadow ON: a wingman further than this from the player's ship (cm) breaks off and returns. Default 18000 (180 m)."), ECVF_Default);
	static TAutoConsoleVariable<float> CVarEngage(TEXT("td.Squad.Shadow.EngageRange"), 22000.f,
		TEXT("Shadow ON: wingmen pick only enemies within this distance of the player's ship (cm). Default 22000 (guns reach 200 m)."), ECVF_Default);

	// --- Shadow OFF ---
	static TAutoConsoleVariable<float> CVarSoloScale(TEXT("td.Squad.Solo.LeashScale"), 3.f,
		TEXT("Shadow OFF: the leash is the shadow leash times this. Default 3."), ECVF_Default);
	static TAutoConsoleVariable<float> CVarSoloCap(TEXT("td.Squad.Solo.LeashCap"), 60000.f,
		TEXT("Shadow OFF: hard cap on the leash (cm), whatever the scale. Default 60000 (600 m; the radar's edge is 1500 m)."), ECVF_Default);
	static TAutoConsoleVariable<float> CVarSoloHunt(TEXT("td.Squad.Solo.HuntRange"), 30000.f,
		TEXT("Shadow OFF: a wingman picks its own targets within this distance of itself (cm). Default 30000."), ECVF_Default);
	static TAutoConsoleVariable<float> CVarSoloDamage(TEXT("td.Squad.Solo.DamageMult"), 1.35f,
		TEXT("Shadow OFF: wingman weapon damage multiplier. Default 1.35."), ECVF_Default);
	static TAutoConsoleVariable<float> CVarSoloAim(TEXT("td.Squad.Solo.AimErrorScale"), 0.5f,
		TEXT("Shadow OFF: wingman aim error multiplier (below 1 = tighter). Default 0.5."), ECVF_Default);
	static TAutoConsoleVariable<float> CVarSoloSecondary(TEXT("td.Squad.Solo.SecondaryIntervalScale"), 0.8f,
		TEXT("Shadow OFF: time between a wingman's secondary shots is multiplied by this. Default 0.8 (about +20% more often)."), ECVF_Default);

	// --- Wingman powers (both modes) ---
	static TAutoConsoleVariable<int32> CVarPowers(TEXT("td.Squad.Powers"), 1,
		TEXT("1: wingmen use their signature powers and unlocked abilities (default). 0: basic attacks only."), ECVF_Default);
	static TAutoConsoleVariable<float> CVarPowerPad(TEXT("td.Squad.Powers.CooldownPad"), 1.3f,
		TEXT("A wingman waits the power's cooldown times this before using it again. Default 1.3."), ECVF_Default);
	static TAutoConsoleVariable<float> CVarPowerGap(TEXT("td.Squad.Powers.Gap"), 1.5f,
		TEXT("Seconds between two power uses of the same wingman. Default 1.5."), ECVF_Default);
	static TAutoConsoleVariable<int32> CVarCluster(TEXT("td.Squad.Powers.Cluster"), 3,
		TEXT("Enemies needed in reach for a wingman's EMP, cluster swarm or gravity well. Default 3."), ECVF_Default);
	static TAutoConsoleVariable<float> CVarTough(TEXT("td.Squad.Powers.ToughHull"), 250.f,
		TEXT("A single enemy with at least this much max hull is worth a wingman's missile swarm. Default 250."), ECVF_Default);
	static TAutoConsoleVariable<int32> CVarPortals(TEXT("td.Squad.Powers.Portals"), 0,
		TEXT("1: Rift's wingman places a portal pair in front of a missile homing on it. 0: never (default: not yet seen working in play; missiles are noticed too close)."), ECVF_Default);

	/** 80% of the radar's range (UI/TDHUDModel.h RadarRange = 1500 m): beyond it a wingman sits on the rim. */
	constexpr float RadarSafeRange = 120000.f;

	static void SayOnScreen(const FString& Text, const FColor& Color)
	{
		UE_LOG(LogTriadDrift, Display, TEXT("%s"), *Text);
#if !UE_BUILD_SHIPPING
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.f, Color, Text);
		}
#endif
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdShadow(TEXT("TD.Squad.Shadow"),
		TEXT("TD.Squad.Shadow [0|1]: shadow mode off / on (no argument toggles). ON: wingmen shadow the player. OFF: they hunt alone, stronger."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDSquadManager* S = ATDSquadManager::Get(World);
			if (!S)
			{
				SayOnScreen(TEXT("TD.Squad.Shadow: no squad in this world"), FColor::Red);
				return;
			}
			if (Args.Num() == 0)
			{
				S->ToggleShadowMode();
			}
			else if (Args[0] == TEXT("0") || Args[0] == TEXT("1"))
			{
				S->SetShadowMode(Args[0] == TEXT("1"));
			}
			else
			{
				SayOnScreen(FString::Printf(TEXT("TD.Squad.Shadow: '%s' is not 0 or 1"), *Args[0]), FColor::Red);
				return;
			}
			SayOnScreen(FString::Printf(TEXT("TD.Squad.Shadow: shadow mode %s"), S->IsShadowMode() ? TEXT("ON") : TEXT("OFF")), FColor::Cyan);
		}));

	static FAutoConsoleCommandWithWorldAndArgs CmdDump(TEXT("TD.Squad.Dump"),
		TEXT("TD.Squad.Dump: log each wingman's mode, state, target, distance to the player and power states."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (const ATDSquadManager* S = ATDSquadManager::Get(World))
			{
				TArray<FString> Lines;
				S->DumpSquad(Lines);
				for (const FString& Line : Lines)
				{
					SayOnScreen(Line, FColor::White);
				}
			}
			else
			{
				SayOnScreen(TEXT("TD.Squad.Dump: no squad in this world"), FColor::Red);
			}
		}));
}

namespace TDShadow
{
	float ShadowLeash() { return FMath::Max(2000.f, TDShadowPrivate::CVarLeash.GetValueOnGameThread()); }
	float ShadowEngageRange() { return FMath::Max(2000.f, TDShadowPrivate::CVarEngage.GetValueOnGameThread()); }

	float SoloLeash()
	{
		const float Scaled = ShadowLeash() * FMath::Max(1.f, TDShadowPrivate::CVarSoloScale.GetValueOnGameThread());
		return FMath::Min3(Scaled, FMath::Max(2000.f, TDShadowPrivate::CVarSoloCap.GetValueOnGameThread()), TDShadowPrivate::RadarSafeRange);
	}

	float SoloHuntRange() { return FMath::Max(2000.f, TDShadowPrivate::CVarSoloHunt.GetValueOnGameThread()); }
	float SoloDamageMultiplier() { return FMath::Clamp(TDShadowPrivate::CVarSoloDamage.GetValueOnGameThread(), 1.f, 5.f); }
	float SoloAimErrorScale() { return FMath::Clamp(TDShadowPrivate::CVarSoloAim.GetValueOnGameThread(), 0.f, 1.f); }
	float SoloSecondaryIntervalScale() { return FMath::Clamp(TDShadowPrivate::CVarSoloSecondary.GetValueOnGameThread(), 0.2f, 1.f); }

	bool PowersEnabled() { return TDShadowPrivate::CVarPowers.GetValueOnGameThread() != 0; }
	float PowerCooldownPad() { return FMath::Max(1.f, TDShadowPrivate::CVarPowerPad.GetValueOnGameThread()); }
	float PowerGlobalGap() { return FMath::Max(0.f, TDShadowPrivate::CVarPowerGap.GetValueOnGameThread()); }
	int32 ClusterCount() { return FMath::Max(1, TDShadowPrivate::CVarCluster.GetValueOnGameThread()); }
	float ToughHull() { return FMath::Max(1.f, TDShadowPrivate::CVarTough.GetValueOnGameThread()); }
	bool PortalsEnabled() { return TDShadowPrivate::CVarPortals.GetValueOnGameThread() != 0; }
}
