// Triad Drift - the code-built Settings page (ENG-32). Owner: Teammate 1 (Input).
//
// FTDSettingsPageModel is the page without Slate: its rows, the selected row, and what each key does. Tests drive it
// with real key names. STDSettingsPage draws the model, keeps keyboard / pad focus on itself, and lets the mouse click
// rows and their < > arrows. Every change goes straight to UTDSettingsSubsystem (applied and saved at once).
//
// Keys: Up / Down (W / S, D-pad, left stick) select; Left / Right (A / D, D-pad, left stick) change; Enter / Space / A
// activate (toggle, reset, back); Escape / Backspace / B close.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "InputCoreTypes.h"

class UTDSettingsSubsystem;

enum class ETDSettingsRow : uint8
{
	OnFootCamera,
	MouseSensitivity,
	InvertMouseY,
	GamepadSensitivity,
	GamepadDeadzone,
	InvertGamepadY,
	Hints,
	Difficulty,
	MasterVolume,
	MusicVolume,
	EffectsVolume,
	WindowMode,
	Resolution,
	Reset,
	Back,
	Count
};

class UNREALGAMEJAM_API FTDSettingsPageModel
{
public:
	enum class EKeyResult : uint8 { Unhandled, Handled, Close };

	explicit FTDSettingsPageModel(UTDSettingsSubsystem* InSettings);

	int32 NumRows() const { return (int32)ETDSettingsRow::Count; }
	int32 GetSelected() const { return Selected; }
	ETDSettingsRow GetSelectedRow() const { return (ETDSettingsRow)Selected; }
	void Select(int32 Row) { Selected = FMath::Clamp(Row, 0, NumRows() - 1); }

	/** Up / down (wraps). */
	void Move(int32 Direction);
	/** Left / right on the selected row (or a given row). */
	void Adjust(int32 Direction) { AdjustRow((ETDSettingsRow)Selected, Direction); }
	void AdjustRow(ETDSettingsRow Row, int32 Direction);
	/** Enter: toggles a two-state row, steps a value row forward, resets, or (Back) asks to close. */
	bool Activate();

	EKeyResult HandleKey(const FKey& Key);

	static FText RowLabel(ETDSettingsRow Row);
	FText RowValue(ETDSettingsRow Row) const;
	/** One line under the rows about the selected row (difficulty), else empty. */
	FText SelectedRowHint() const;

	/** The resolutions offered (0,0 = desktop first). Tests may replace the list. */
	TArray<FIntPoint> Resolutions;

private:
	TWeakObjectPtr<UTDSettingsSubsystem> Settings;
	int32 Selected = 0;
};

class UNREALGAMEJAM_API STDSettingsPage : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STDSettingsPage) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UTDSettingsSubsystem>, Settings)
		SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	FTDSettingsPageModel& GetModel() { return *Model; }
	/** Gives the page keyboard / pad focus (it takes it back every tick if something else grabs it). */
	void FocusPage();

private:
	TSharedRef<SWidget> MakeRow(ETDSettingsRow Row);
	TSharedPtr<FTDSettingsPageModel> Model;
	FSimpleDelegate OnClose;
};
