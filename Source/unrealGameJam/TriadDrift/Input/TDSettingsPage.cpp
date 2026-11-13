// Triad Drift - the code-built Settings page (ENG-32). Owner: Teammate 1 (Input).

#include "TriadDrift/Input/TDSettingsPage.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TDSettingsPage"

namespace TDSettingsPage
{
	static bool IsAny(const FKey& K, std::initializer_list<FKey> Keys)
	{
		for (const FKey& X : Keys) { if (K == X) { return true; } }
		return false;
	}

	static float Step(float Value, float Delta, float Min, float Max)
	{
		// Snap to the step so repeated presses land on round numbers.
		return FMath::Clamp(FMath::RoundToFloat((Value + Delta) / FMath::Abs(Delta)) * FMath::Abs(Delta), Min, Max);
	}

	static FText OnOff(bool b) { return b ? LOCTEXT("On", "On") : LOCTEXT("Off", "Off"); }
	static FText Percent(float V) { return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(V * 100.f))); }
	static FText Times(float V) { return FText::FromString(FString::Printf(TEXT("%.2fx"), V)); }
}

// ------------------------------------------------------------------------------------------------ model

FTDSettingsPageModel::FTDSettingsPageModel(UTDSettingsSubsystem* InSettings)
	: Settings(InSettings)
{
	Resolutions.Add(FIntPoint::ZeroValue);
	if (InSettings)
	{
		for (const FIntPoint& R : InSettings->GetSupportedResolutions())
		{
			Resolutions.AddUnique(R);
		}
	}
	if (Resolutions.Num() == 1)
	{
		Resolutions.Append({ FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080), FIntPoint(2560, 1440) });
	}
}

void FTDSettingsPageModel::Move(int32 Direction)
{
	const int32 N = NumRows();
	Selected = ((Selected + Direction) % N + N) % N;
}

void FTDSettingsPageModel::AdjustRow(ETDSettingsRow Row, int32 Direction)
{
	UTDSettingsSubsystem* S = Settings.Get();
	if (!S || Direction == 0)
	{
		return;
	}
	const FTDPlayerSettings& P = S->GetSettings();
	const float D = Direction > 0 ? 1.f : -1.f;
	switch (Row)
	{
	case ETDSettingsRow::OnFootCamera: S->ToggleOnFootCameraMode(); break;
	case ETDSettingsRow::MouseSensitivity: S->SetMouseSensitivity(TDSettingsPage::Step(P.MouseSensitivity, 0.25f * D, 0.25f, 3.f)); break;
	case ETDSettingsRow::InvertMouseY: S->SetInvertMouseY(!P.bInvertMouseY); break;
	case ETDSettingsRow::GamepadSensitivity: S->SetGamepadSensitivity(TDSettingsPage::Step(P.GamepadSensitivity, 0.25f * D, 0.25f, 3.f)); break;
	case ETDSettingsRow::GamepadDeadzone: S->SetGamepadDeadzone(TDSettingsPage::Step(P.GamepadDeadzone, 0.05f * D, 0.f, 0.5f)); break;
	case ETDSettingsRow::InvertGamepadY: S->SetInvertGamepadY(!P.bInvertGamepadY); break;
	case ETDSettingsRow::Hints: S->SetHintsEnabled(!P.bHintsEnabled); break;
	case ETDSettingsRow::MasterVolume: S->SetMasterVolume(TDSettingsPage::Step(P.MasterVolume, 0.1f * D, 0.f, 1.f)); break;
	case ETDSettingsRow::MusicVolume: S->SetMusicVolume(TDSettingsPage::Step(P.MusicVolume, 0.1f * D, 0.f, 1.f)); break;
	case ETDSettingsRow::EffectsVolume: S->SetEffectsVolume(TDSettingsPage::Step(P.EffectsVolume, 0.1f * D, 0.f, 1.f)); break;
	case ETDSettingsRow::WindowMode:
	{
		const int32 M = (((int32)P.WindowMode + (Direction > 0 ? 1 : -1)) % 3 + 3) % 3;
		S->SetWindowMode((ETDWindowMode)M);
		break;
	}
	case ETDSettingsRow::Resolution:
	{
		if (Resolutions.Num() == 0) { break; }
		int32 I = Resolutions.IndexOfByKey(P.Resolution);
		I = I == INDEX_NONE ? 0 : ((I + (Direction > 0 ? 1 : -1)) % Resolutions.Num() + Resolutions.Num()) % Resolutions.Num();
		S->SetResolution(Resolutions[I]);
		break;
	}
	default: break;
	}
}

bool FTDSettingsPageModel::Activate()
{
	UTDSettingsSubsystem* S = Settings.Get();
	switch (GetSelectedRow())
	{
	case ETDSettingsRow::Reset: if (S) { S->ResetToDefaults(); } return false;
	case ETDSettingsRow::Back: return true;
	default: Adjust(+1); return false;
	}
}

FTDSettingsPageModel::EKeyResult FTDSettingsPageModel::HandleKey(const FKey& K)
{
	using namespace TDSettingsPage;
	if (IsAny(K, { EKeys::Escape, EKeys::BackSpace, EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_Special_Right }))
	{
		return EKeyResult::Close;
	}
	if (IsAny(K, { EKeys::Up, EKeys::W, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_LeftStick_Up }))
	{
		Move(-1);
		return EKeyResult::Handled;
	}
	if (IsAny(K, { EKeys::Down, EKeys::S, EKeys::Tab, EKeys::Gamepad_DPad_Down, EKeys::Gamepad_LeftStick_Down }))
	{
		Move(+1);
		return EKeyResult::Handled;
	}
	if (IsAny(K, { EKeys::Left, EKeys::A, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_LeftStick_Left }))
	{
		if (GetSelectedRow() != ETDSettingsRow::Reset && GetSelectedRow() != ETDSettingsRow::Back) { Adjust(-1); }
		return EKeyResult::Handled;
	}
	if (IsAny(K, { EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_LeftStick_Right }))
	{
		if (GetSelectedRow() != ETDSettingsRow::Reset && GetSelectedRow() != ETDSettingsRow::Back) { Adjust(+1); }
		return EKeyResult::Handled;
	}
	if (IsAny(K, { EKeys::Enter, EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom }))
	{
		return Activate() ? EKeyResult::Close : EKeyResult::Handled;
	}
	return EKeyResult::Unhandled;
}

FText FTDSettingsPageModel::RowLabel(ETDSettingsRow Row)
{
	switch (Row)
	{
	case ETDSettingsRow::OnFootCamera: return LOCTEXT("Camera", "On-foot camera");
	case ETDSettingsRow::MouseSensitivity: return LOCTEXT("MouseSens", "Mouse sensitivity");
	case ETDSettingsRow::InvertMouseY: return LOCTEXT("InvertMouse", "Invert mouse Y");
	case ETDSettingsRow::GamepadSensitivity: return LOCTEXT("PadSens", "Gamepad sensitivity");
	case ETDSettingsRow::GamepadDeadzone: return LOCTEXT("PadDead", "Gamepad dead zone");
	case ETDSettingsRow::InvertGamepadY: return LOCTEXT("InvertPad", "Invert gamepad Y");
	case ETDSettingsRow::Hints: return LOCTEXT("Hints", "Control hints");
	case ETDSettingsRow::MasterVolume: return LOCTEXT("Master", "Master volume");
	case ETDSettingsRow::MusicVolume: return LOCTEXT("Music", "Music volume");
	case ETDSettingsRow::EffectsVolume: return LOCTEXT("Effects", "Effects volume");
	case ETDSettingsRow::WindowMode: return LOCTEXT("Window", "Window mode");
	case ETDSettingsRow::Resolution: return LOCTEXT("Resolution", "Resolution");
	case ETDSettingsRow::Reset: return LOCTEXT("Reset", "Reset to defaults");
	case ETDSettingsRow::Back: return LOCTEXT("Back", "Back");
	default: return FText::GetEmpty();
	}
}

FText FTDSettingsPageModel::RowValue(ETDSettingsRow Row) const
{
	const UTDSettingsSubsystem* S = Settings.Get();
	if (!S)
	{
		return FText::GetEmpty();
	}
	const FTDPlayerSettings& P = S->GetSettings();
	switch (Row)
	{
	case ETDSettingsRow::OnFootCamera:
		return P.OnFootCameraMode == ETDOnFootCamera::ThirdPerson ? LOCTEXT("Third", "Third person") : LOCTEXT("First", "First person");
	case ETDSettingsRow::MouseSensitivity: return TDSettingsPage::Times(P.MouseSensitivity);
	case ETDSettingsRow::InvertMouseY: return TDSettingsPage::OnOff(P.bInvertMouseY);
	case ETDSettingsRow::GamepadSensitivity: return TDSettingsPage::Times(P.GamepadSensitivity);
	case ETDSettingsRow::GamepadDeadzone: return TDSettingsPage::Percent(P.GamepadDeadzone);
	case ETDSettingsRow::InvertGamepadY: return TDSettingsPage::OnOff(P.bInvertGamepadY);
	case ETDSettingsRow::Hints: return TDSettingsPage::OnOff(P.bHintsEnabled);
	case ETDSettingsRow::MasterVolume: return TDSettingsPage::Percent(P.MasterVolume);
	case ETDSettingsRow::MusicVolume: return TDSettingsPage::Percent(P.MusicVolume);
	case ETDSettingsRow::EffectsVolume: return TDSettingsPage::Percent(P.EffectsVolume);
	case ETDSettingsRow::WindowMode:
		return P.WindowMode == ETDWindowMode::Fullscreen ? LOCTEXT("Full", "Fullscreen")
			: P.WindowMode == ETDWindowMode::Windowed ? LOCTEXT("Windowed", "Windowed") : LOCTEXT("Borderless", "Windowed fullscreen");
	case ETDSettingsRow::Resolution:
		return (P.Resolution.X > 0 && P.Resolution.Y > 0) ? FText::FromString(FString::Printf(TEXT("%d x %d"), P.Resolution.X, P.Resolution.Y))
			: LOCTEXT("Desktop", "Desktop");
	default: return FText::GetEmpty();
	}
}

// ------------------------------------------------------------------------------------------------ Slate page

void STDSettingsPage::Construct(const FArguments& InArgs)
{
	Model = MakeShared<FTDSettingsPageModel>(InArgs._Settings.Get());
	OnClose = InArgs._OnClose;

	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	Rows->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 18.f)
	[
		SNew(STextBlock).Text(LOCTEXT("Title", "SETTINGS")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 40)).ColorAndOpacity(FLinearColor(0.4f, 0.9f, 1.f))
	];
	for (int32 i = 0; i < Model->NumRows(); ++i)
	{
		Rows->AddSlot().AutoHeight().Padding(0.f, 2.f)[ MakeRow((ETDSettingsRow)i) ];
	}
	Rows->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 16.f, 0.f, 0.f)
	[
		SNew(STextBlock)
			.Text(LOCTEXT("Help", "UP / DOWN  SELECT     LEFT / RIGHT  CHANGE     ENTER / A  SELECT     ESC / B  BACK"))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(FLinearColor(0.7f, 0.75f, 0.8f))
	];

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.02f, 0.82f))
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(780.f)[ Rows ]
		]
	];
}

TSharedRef<SWidget> STDSettingsPage::MakeRow(ETDSettingsRow Row)
{
	const int32 Index = (int32)Row;
	TWeakPtr<FTDSettingsPageModel> WeakModel = Model;
	const bool bAction = Row == ETDSettingsRow::Reset || Row == ETDSettingsRow::Back;
	auto Selected = [WeakModel, Index]() { const TSharedPtr<FTDSettingsPageModel> M = WeakModel.Pin(); return M && M->GetSelected() == Index; };
	auto Arrow = [this, WeakModel, Row, Index](int32 Dir, const TCHAR* Glyph) -> TSharedRef<SWidget>
	{
		return SNew(SButton).IsFocusable(false).ButtonColorAndOpacity(FLinearColor(0.1f, 0.12f, 0.16f, 1.f))
			.OnClicked_Lambda([this, WeakModel, Row, Index, Dir]()
			{
				if (const TSharedPtr<FTDSettingsPageModel> M = WeakModel.Pin()) { M->Select(Index); M->AdjustRow(Row, Dir); }
				FocusPage();
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(Glyph)).Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			];
	};

	TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(14.f, 6.f)
		[
			SNew(STextBlock).Text(FTDSettingsPageModel::RowLabel(Row)).Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
			.ColorAndOpacity_Lambda([Selected]() { return Selected() ? FSlateColor(FLinearColor(1.f, 0.85f, 0.4f)) : FSlateColor(FLinearColor::White); })
		];
	if (!bAction)
	{
		Line->AddSlot().AutoWidth().VAlign(VAlign_Center)[ Arrow(-1, TEXT("<")) ];
		Line->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f)
		[
			SNew(SBox).WidthOverride(270.f).HAlign(HAlign_Center)
			[
				SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
				.Text_Lambda([WeakModel, Row]() { const TSharedPtr<FTDSettingsPageModel> M = WeakModel.Pin(); return M ? M->RowValue(Row) : FText::GetEmpty(); })
			]
		];
		Line->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)[ Arrow(+1, TEXT(">")) ];
	}

	return SNew(SButton).IsFocusable(false)
		.ButtonColorAndOpacity_Lambda([Selected]() { return Selected() ? FLinearColor(0.15f, 0.3f, 0.4f, 0.95f) : FLinearColor(0.05f, 0.06f, 0.09f, 0.9f); })
		.OnClicked_Lambda([this, WeakModel, Index, bAction]()
		{
			if (const TSharedPtr<FTDSettingsPageModel> M = WeakModel.Pin())
			{
				const bool bWasSelected = M->GetSelected() == Index;
				M->Select(Index);
				if (bAction || bWasSelected)
				{
					if (M->Activate()) { OnClose.ExecuteIfBound(); return FReply::Handled(); }
				}
			}
			FocusPage();
			return FReply::Handled();
		})
		[ Line ];
}

FReply STDSettingsPage::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	switch (Model->HandleKey(InKeyEvent.GetKey()))
	{
	case FTDSettingsPageModel::EKeyResult::Close: OnClose.ExecuteIfBound(); return FReply::Handled();
	case FTDSettingsPageModel::EKeyResult::Handled: return FReply::Handled();
	default: return FReply::Handled(); // the page is modal: nothing leaks to the game or the menu behind it
	}
}

void STDSettingsPage::FocusPage()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocus(SharedThis(this), EFocusCause::SetDirectly);
	}
}

void STDSettingsPage::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}
	for (TSharedPtr<SWidget> W = FSlateApplication::Get().GetUserFocusedWidget(0); W.IsValid(); W = W->GetParentWidget())
	{
		if (W.Get() == this)
		{
			return;
		}
	}
	FocusPage(); // the viewport or a menu behind took focus: take it back while the page is up
}

// ------------------------------------------------------------------------------------------------ opening

UClass* UTDSettingsSubsystem::ResolveSettingsWidgetClass(const UObject* Style)
{
	// Read by name so this works before and after UTDUIStyle::SettingsWidgetClass exists in Core.
	if (!Style)
	{
		return nullptr;
	}
	const FSoftClassProperty* Prop = CastField<FSoftClassProperty>(Style->GetClass()->FindPropertyByName(TEXT("SettingsWidgetClass")));
	if (!Prop)
	{
		return nullptr;
	}
	const FSoftObjectPtr& Soft = Prop->GetPropertyValue_InContainer(Style);
	if (Soft.IsNull())
	{
		return nullptr;
	}
	UClass* Class = Cast<UClass>(Soft.LoadSynchronous());
	return (Class && Class->IsChildOf(UUserWidget::StaticClass())) ? Class : nullptr;
}

bool UTDSettingsSubsystem::OpenSettingsPage(const UObject* WorldContextObject)
{
	UTDSettingsSubsystem* S = GetTDSettings(WorldContextObject);
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!S || !World || !GEngine || !GEngine->GameViewport)
	{
		return false;
	}
	if (IsSettingsPageOpen(WorldContextObject))
	{
		return true;
	}
	APlayerController* PC = World->GetFirstPlayerController();
	if (PC)
	{
		S->bPageCursorWasShown = PC->ShouldShowMouseCursor();
		PC->SetShowMouseCursor(true);
	}
	// Nick's styled page, when the UI style names one.
	if (UClass* WidgetClass = ResolveSettingsWidgetClass(UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"))))
	{
		UUserWidget* Widget = PC ? CreateWidget<UUserWidget>(PC, WidgetClass) : CreateWidget<UUserWidget>(World, WidgetClass);
		if (Widget)
		{
			Widget->AddToViewport(50);
			Widget->SetKeyboardFocus();
			S->PageWidget = Widget;
			UE_LOG(LogTriadDrift, Log, TEXT("TD settings: page opened (%s)"), *WidgetClass->GetName());
			return true;
		}
	}
	TWeakObjectPtr<UTDSettingsSubsystem> WeakS = S;
	const UObject* Context = WorldContextObject;
	TWeakObjectPtr<const UObject> WeakContext = Context;
	S->Page = SNew(STDSettingsPage).Settings(WeakS).OnClose_Lambda([WeakContext]() { CloseSettingsPage(WeakContext.Get()); });
	GEngine->GameViewport->AddViewportWidgetContent(S->Page.ToSharedRef(), 50);
	S->Page->FocusPage();
	UE_LOG(LogTriadDrift, Log, TEXT("TD settings: page opened (code-built)"));
	return true;
}

void UTDSettingsSubsystem::CloseSettingsPage(const UObject* WorldContextObject)
{
	if (UTDSettingsSubsystem* S = GetTDSettings(WorldContextObject))
	{
		S->ClosePageInternal(WorldContextObject ? WorldContextObject->GetWorld() : nullptr);
	}
}

void UTDSettingsSubsystem::ClosePageInternal(UWorld* World)
{
	UTDSettingsSubsystem* S = this;
	const bool bWasOpen = S->Page.IsValid() || S->PageWidget != nullptr;
	if (S->Page.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(S->Page.ToSharedRef());
	}
	S->Page.Reset();
	if (S->PageWidget)
	{
		S->PageWidget->RemoveFromParent();
		S->PageWidget = nullptr;
	}
	if (!bWasOpen)
	{
		return;
	}
	if (APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
	{
		PC->SetShowMouseCursor(S->bPageCursorWasShown);
	}
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport(); // a menu behind takes its focus back on its next tick
	}
	UE_LOG(LogTriadDrift, Log, TEXT("TD settings: page closed"));
}

bool UTDSettingsSubsystem::IsSettingsPageOpen(const UObject* WorldContextObject)
{
	const UTDSettingsSubsystem* S = GetTDSettings(WorldContextObject);
	return S && (S->Page.IsValid() || S->PageWidget != nullptr);
}

#undef LOCTEXT_NAMESPACE
