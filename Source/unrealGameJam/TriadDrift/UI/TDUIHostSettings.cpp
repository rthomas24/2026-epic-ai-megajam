// Triad Drift - UI host settings (Teammate 2).

#include "TriadDrift/UI/TDUIHostSettings.h"

#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarTDUINativeHUD(
	TEXT("TD.UI.NativeHUD"), -1,
	TEXT("Nick's native space HUD: -1 = use Project Settings (Triad Drift UI Host), 0 = force off, 1 = force on."),
	ECVF_Default);

bool UTDUIHostSettings::IsNativeSpaceHUDEnabled()
{
	const int32 Override = CVarTDUINativeHUD.GetValueOnGameThread();
	if (Override >= 0)
	{
		return Override > 0;
	}
	return Get()->bUseNativeSpaceHUD;
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
