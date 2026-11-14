// Copyright Epic Games, Inc. All Rights Reserved.

#include "unrealGameJam.h"
#include "Modules/ModuleManager.h"

#include "TriadDrift/Flow/TDBootLoadingScreen.h"

/** The game module: on start-up, the boot loading screen for the first map load (T6; the game window was black for
 *  about 10 s between the Windows splash and the menu). */
class FTDGameModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		TDBootLoadingScreen::Start();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FTDGameModule, unrealGameJam, "unrealGameJam" );

DEFINE_LOG_CATEGORY(LogunrealGameJam)