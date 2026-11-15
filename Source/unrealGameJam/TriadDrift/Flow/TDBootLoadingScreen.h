// Triad Drift - the boot loading screen (T6, packaging launch check of a946ca93: after the Windows splash closed at 3.3 s
// the game window stayed black until the menu at 13.4 s).
//
// Started from the game module's StartupModule, before GEngine->Start() loads the menu map: a MoviePlayer loading screen
// (it renders on its own thread, so the window paints and answers while the game thread loads) showing the same picture as
// the splash (Content/Splash/Splash.bmp, staged with the game, decoded to pixels: a path brush drew white in Shipping) and "LOADING". The engine's own WaitForMovieToFinish after
// the load ends it (auto-complete). Game builds only; never in the editor or a commandlet; -NoBootLoadingScreen turns it off.

#pragma once

#include "CoreMinimal.h"

namespace TDBootLoadingScreen
{
	/** Sets up and starts the loading screen for the first map load. Safe to call when the movie player is unavailable. */
	void Start();

	/** Lets the splash texture go (the game module's ShutdownModule, while Slate is still up). */
	void Shutdown();
}
