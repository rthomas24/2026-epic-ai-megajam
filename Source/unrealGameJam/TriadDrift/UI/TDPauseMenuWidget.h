// Triad Drift - pause screen (Teammate 2). See docs/eng/ui-host.md.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDPauseMenuWidget.generated.h"

UENUM(BlueprintType)
enum class ETDPauseItem : uint8
{
	Resume,
	Settings,
	RestartScene,
	QuitToMenu
};

/**
 * The pause screen in the space HUD's language: the frame dimmed and blurred (a UBackgroundBlur root), a glass panel
 * with RESUME, SETTINGS (UTDSettingsSubsystem::OpenSettingsPage), RESTART SCENE (the next take) and QUIT TO MENU.
 * Restart and quit ask to confirm. Keyboard (W/S or arrows, Enter / Space), gamepad (d-pad or left stick, A, B = resume)
 * and mouse (hover, click). Escape / Start are left to the controller's pause toggle, which closes this screen.
 * Shown by UTDUIHostSubsystem on the controller's OnPauseChanged; Nick can replace it through UTDUIStyle::PauseWidgetClass.
 */
UCLASS()
class UNREALGAMEJAM_API UTDPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	static constexpr int32 ItemCount = 4;
	static FString ItemLabel(ETDPauseItem Item);

	int32 GetSelected() const { return Selected; }
	bool IsConfirming() const { return bConfirming; }
	/** In a confirm prompt: 0 = CONFIRM, 1 = CANCEL. */
	int32 GetConfirmChoice() const { return ConfirmChoice; }

	/** Navigation, as the key handler drives it (public for tests). */
	void MoveSelection(int32 Delta);
	/** Enter / A / click on the selected item (or the selected confirm choice). */
	void Activate();
	/** B / back: leaves a confirm prompt, else resumes. */
	void Back();
	/** Handles a key as the widget would; true when consumed. */
	bool HandleKey(const FKey& Key);

	/** What the last activation did (tests): "resume", "settings", "settings-missing", "restart", "quit". */
	FString LastAction;

	/** False in tests: activations are recorded but not executed (no unpause, travel or retake). */
	bool bExecuteActions = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversPause = true;

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	/** Design-space rows (1920x1080). */
	static FBox2D ItemRect(int32 Index);
	static FBox2D ConfirmRect(int32 Choice);
	int32 HitTest(const FGeometry& Geometry, const FVector2D& ScreenPos, bool& bOutConfirm) const;
	void Execute(ETDPauseItem Item);

	int32 Selected = 0;
	bool bConfirming = false;
	int32 ConfirmChoice = 1;
	FString Toast;
	double ToastUntil = 0.0;
};
