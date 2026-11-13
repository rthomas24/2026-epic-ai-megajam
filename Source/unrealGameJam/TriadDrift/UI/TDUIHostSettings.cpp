// Triad Drift - UI host settings (Teammate 2).

#include "TriadDrift/UI/TDUIHostSettings.h"

#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarTDUINativeHUD(
	TEXT("TD.UI.NativeHUD"), -1,
	TEXT("Nick's native space HUD: -1 = use Project Settings (Triad Drift UI Host), 0 = force off, 1 = force on."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTDUISpaceHUD(
	TEXT("TD.UI.SpaceHUD"), -1,
	TEXT("Space HUD: -1 = use Project Settings (Triad Drift UI Host), 0 = dev HUD, 1 = Nick's HUD, 2 = engineering HUD (ENG-36)."),
	ECVF_Default);

ETDSpaceHUDChoice UTDUIHostSettings::GetSpaceHUDChoice()
{
	switch (CVarTDUISpaceHUD.GetValueOnGameThread())
	{
	case 0: return ETDSpaceHUDChoice::Dev;
	case 1: return ETDSpaceHUDChoice::Nick;
	case 2: return ETDSpaceHUDChoice::Engineering;
	default: break;
	}
	const UTDUIHostSettings* Settings = Get();
	ETDSpaceHUDChoice Choice = Settings->SpaceHUD;
	if (Choice == ETDSpaceHUDChoice::Unset)
	{
		Choice = Settings->bUseNativeSpaceHUD ? ETDSpaceHUDChoice::Nick : ETDSpaceHUDChoice::Dev;
	}
	// The older switch still works: 1 = Nick's HUD whatever the setting, 0 = not Nick's (the dev HUD instead).
	const int32 Native = CVarTDUINativeHUD.GetValueOnGameThread();
	if (Native > 0)
	{
		return ETDSpaceHUDChoice::Nick;
	}
	if (Native == 0 && Choice == ETDSpaceHUDChoice::Nick)
	{
		return ETDSpaceHUDChoice::Dev;
	}
	return Choice;
}

bool UTDUIHostSettings::IsNativeSpaceHUDEnabled()
{
	return GetSpaceHUDChoice() == ETDSpaceHUDChoice::Nick;
}

static TAutoConsoleVariable<int32> CVarTDUIGroundHUD(
	TEXT("TD.UI.GroundHUD"), -1,
	TEXT("Engineering on-foot HUD: -1 = use Project Settings (Triad Drift UI Host), 0 = force off (dev HUD text), 1 = force on."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTDUIPauseScreen(
	TEXT("TD.UI.PauseScreen"), -1,
	TEXT("Pause screen: -1 = use Project Settings (Triad Drift UI Host), 0 = force off (dev HUD PAUSED line), 1 = force on."),
	ECVF_Default);

bool UTDUIHostSettings::IsNativeGroundHUDEnabled()
{
	const int32 Override = CVarTDUIGroundHUD.GetValueOnGameThread();
	return Override >= 0 ? Override > 0 : Get()->bUseNativeGroundHUD;
}

bool UTDUIHostSettings::IsNativePauseScreenEnabled()
{
	const int32 Override = CVarTDUIPauseScreen.GetValueOnGameThread();
	return Override >= 0 ? Override > 0 : Get()->bUseNativePauseScreen;
}
