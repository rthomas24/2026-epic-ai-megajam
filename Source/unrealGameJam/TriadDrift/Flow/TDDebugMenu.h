// Triad Drift - F10 debug menu (ENG-19). Owner: Teammate 5. Dev builds only; plain Slate, no assets.

#pragma once

#include "CoreMinimal.h"

class SWidget;
class UWorld;

/**
 * A viewport overlay with one button per TD.* cheat: phases, ability unlocks, god mode, kill all,
 * skip wave, force retake, and travel. Toggled with F10 by UTDGameInstance (any player controller).
 * Every button calls the same UTDCheatManager::Do* function as the console command.
 */
class UNREALGAMEJAM_API FTDDebugMenu : public TSharedFromThis<FTDDebugMenu>
{
public:
	~FTDDebugMenu();

	void Toggle(UWorld* World);
	void Show(UWorld* World);
	void Hide();
	bool IsVisible() const { return Widget.IsValid(); }

private:
	TSharedRef<SWidget> Build();
	void SetCursor(bool bVisible);

	TSharedPtr<SWidget> Widget;
	TWeakObjectPtr<UWorld> WorldPtr;
};
