// Triad Drift - player aim assist (issues #192 / #201; Teammate 2).

#include "TriadDrift/Combat/TDAimAssist.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarTDAimEnabled(TEXT("td.Aim.Enabled"), 1, TEXT("#201: the player's aim assist (soft lock, convergence, Rift/Lancer homing). 0 = off."));
static TAutoConsoleVariable<int32> CVarTDAimLegacy(TEXT("TD.Weapons.AimAssist"), 1, TEXT("#192: same as td.Aim.Enabled (kept for older scripts); both must be 1."));
static TAutoConsoleVariable<float> CVarTDAimPad(TEXT("td.Aim.PadScale"), 1.35f, TEXT("#201: assist strength when the player last used a gamepad."));
static TAutoConsoleVariable<float> CVarTDAimMouse(TEXT("td.Aim.MouseScale"), 1.f, TEXT("#201: assist strength with mouse and keyboard."));
static TAutoConsoleVariable<float> CVarTDAimStory(TEXT("td.Aim.StoryScale"), 1.3f, TEXT("#201: assist strength on Story (easiest, strongest)."));
static TAutoConsoleVariable<float> CVarTDAimStandard(TEXT("td.Aim.StandardScale"), 1.f, TEXT("#201: assist strength on Standard."));
static TAutoConsoleVariable<float> CVarTDAimAce(TEXT("td.Aim.AceScale"), .75f, TEXT("#201: assist strength on Ace."));
static TAutoConsoleVariable<float> CVarTDAimLockCone(TEXT("td.Aim.LockConeDeg"), 16.f, TEXT("#201: soft-lock cone half-angle (deg) before the strength scale."));
static TAutoConsoleVariable<float> CVarTDAimLockConeMax(TEXT("td.Aim.MaxLockConeDeg"), 24.f, TEXT("#201: hard cap on the soft-lock cone (deg)."));
static TAutoConsoleVariable<float> CVarTDAimKeep(TEXT("td.Aim.LockKeepMarginDeg"), 6.f, TEXT("#201: the lock stays while within its cone plus this margin (deg)."));
static TAutoConsoleVariable<float> CVarTDAimSticky(TEXT("td.Aim.StickySeconds"), .6f, TEXT("#201: seconds the lock holds after the reticle leaves it (no flicker)."));
static TAutoConsoleVariable<float> CVarTDAimConverge(TEXT("td.Aim.ConvergeDeg"), 15.f, TEXT("#201: muzzle convergence cone (deg) before the strength scale: shots leave toward the lock's lead point."));
static TAutoConsoleVariable<float> CVarTDAimConvergeMax(TEXT("td.Aim.MaxConvergeDeg"), 22.f, TEXT("#201: hard cap on the convergence cone (deg)."));
static TAutoConsoleVariable<float> CVarTDAimTurn(TEXT("td.Aim.TurnScale"), 2.f, TEXT("#201: multiplier on the Rift's / Lancer's homing turn rate."));
static TAutoConsoleVariable<float> CVarTDAimMaxTurn(TEXT("td.Aim.MaxTurnDeg"), 45.f, TEXT("#201: cap on a shot's total in-flight bend (deg): a hard-turning enemy can still shake it."));

bool FTDAimAssist::IsEnabled()
{
	return CVarTDAimEnabled.GetValueOnGameThread() != 0 && CVarTDAimLegacy.GetValueOnGameThread() != 0;
}

bool FTDAimAssist::IsPlayerShip(const AActor* Ship)
{
	const ATDShip* S = Cast<ATDShip>(Ship);
	return S && S->IsPlayerFlown();
}

float FTDAimAssist::Strength(const AActor* Ship)
{
	const APawn* Pawn = Cast<APawn>(Ship);
	const ATDSpacePlayerController* PC = Pawn ? Cast<ATDSpacePlayerController>(Pawn->GetController()) : nullptr;
	const float Input = PC && PC->IsUsingGamepad() ? CVarTDAimPad.GetValueOnGameThread() : CVarTDAimMouse.GetValueOnGameThread();
	const ETDDifficulty D = UTDDifficultySettings::GetCurrent();
	const float Diff = D == ETDDifficulty::Story ? CVarTDAimStory.GetValueOnGameThread()
		: D == ETDDifficulty::Ace ? CVarTDAimAce.GetValueOnGameThread() : CVarTDAimStandard.GetValueOnGameThread();
	return FMath::Max(0.f, Input * Diff);
}

float FTDAimAssist::LockConeDeg(const AActor* Ship)
{
	return FMath::Clamp(CVarTDAimLockCone.GetValueOnGameThread() * Strength(Ship), 0.f, CVarTDAimLockConeMax.GetValueOnGameThread());
}

float FTDAimAssist::LockKeepMarginDeg()
{
	return FMath::Max(0.f, CVarTDAimKeep.GetValueOnGameThread());
}

float FTDAimAssist::StickySeconds()
{
	return FMath::Max(0.f, CVarTDAimSticky.GetValueOnGameThread());
}

float FTDAimAssist::ConvergeDeg(const AActor* Ship)
{
	return FMath::Clamp(CVarTDAimConverge.GetValueOnGameThread() * Strength(Ship), 0.f, CVarTDAimConvergeMax.GetValueOnGameThread());
}

float FTDAimAssist::HomingTurnScale(const AActor* Ship)
{
	return FMath::Max(0.f, CVarTDAimTurn.GetValueOnGameThread() * Strength(Ship));
}

float FTDAimAssist::HomingMaxTurnDeg()
{
	return FMath::Clamp(CVarTDAimMaxTurn.GetValueOnGameThread(), 0.f, 90.f);
}
