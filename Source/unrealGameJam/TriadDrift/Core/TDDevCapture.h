// Triad Drift - unattended screenshot capture for visual verification (dev builds only). Lead-owned.
//
// Launch the game with:
//   -TDShots=2,5,8          take a screenshot 2 s, 5 s and 8 s after the first world begins play
//   -TDShotDir=<folder>     where the PNGs go (default: <Project>/Saved/TDShots)
//   -TDQuit=10              exit 10 s after play begins (default: 1 s after the last shot)
//   -TDExec="cmd1;cmd2"     console commands to run once play begins (e.g. TD.God)
//   -TDExecAt=4:"TD.KillAll" run a console command at a given time (repeatable)
// Tools/td-shot.ps1 wraps this.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TDDevCapture.generated.h"

UCLASS()
class UNREALGAMEJAM_API UTDDevCapture : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	bool Tick(float DeltaSeconds);

	struct FTimedCommand
	{
		float Time = 0.f;
		FString Command;
		bool bDone = false;
	};

	FTSTicker::FDelegateHandle TickHandle;
	TArray<float> ShotTimes;
	TArray<FTimedCommand> Commands;
	FString ShotDir;
	float QuitTime = -1.f;
	float Elapsed = 0.f;
	int32 NextShot = 0;
	bool bStarted = false;
};
