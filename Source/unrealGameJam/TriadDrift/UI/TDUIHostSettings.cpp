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
