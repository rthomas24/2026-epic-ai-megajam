// Triad Drift - pause screen (Teammate 2).

#include "TriadDrift/UI/TDPauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/UI/TDUIPaint.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDPauseMenu, Log, All);

namespace TDPauseMenu
{
	const float PanelX = 700.f, PanelY = 290.f, PanelW = 520.f, PanelH = 470.f;
	const float RowX = 732.f, RowW = 456.f, RowH = 58.f, RowTop = 426.f, RowStep = 70.f;
}

bool UTDPauseMenuWidget::Initialize()
{
	const bool bOk = Super::Initialize();
	// A code-built widget has no designer tree: the root is the blur, NativePaint draws the dim and the panel over it.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("FrameBlur"));
		Blur->SetBlurStrength(7.f);
		WidgetTree->RootWidget = Blur;
	}
	SetIsFocusable(true);
	return bOk;
}

void UTDPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Selected = 0;
	bConfirming = false;
	DifficultyLine = ReadDifficultyInPlay(); // fresh each opening: it may have changed through SETTINGS
	if (APlayerController* PC = GetOwningPlayer())
	{
		SetUserFocus(PC);
		SetKeyboardFocus();
	}
}

FString UTDPauseMenuWidget::ReadDifficultyInPlay()
{
	// T1's static UFUNCTION GetDifficultyInPlay() -> ETDDifficulty, by name so this file builds before it lands.
	UFunction* Get = UTDSettingsSubsystem::StaticClass()->FindFunctionByName(TEXT("GetDifficultyInPlay"));
	const FProperty* Ret = Get ? Get->GetReturnProperty() : nullptr;
	if (!Ret)
	{
		return FString();
	}
	TArray<uint8> Params;
	Params.SetNumZeroed(FMath::Max<int32>(Get->ParmsSize, 8));
	UTDSettingsSubsystem::StaticClass()->GetDefaultObject()->ProcessEvent(Get, Params.GetData());
	int64 Value = 0;
	const UEnum* Enum = nullptr;
	if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Ret))
	{
		Value = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(EnumProp->ContainerPtrToValuePtr<void>(Params.GetData()));
		Enum = EnumProp->GetEnum();
	}
	else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Ret))
	{
		Value = ByteProp->GetPropertyValue_InContainer(Params.GetData());
		Enum = ByteProp->Enum;
	}
	if (!Enum)
	{
		return FString();
	}
	FString Name = Enum->GetDisplayNameTextByValue(Value).ToString();
	if (Name.IsEmpty())
	{
		Name = Enum->GetNameStringByValue(Value);
	}
	return Name.ToUpper();
}

FString UTDPauseMenuWidget::ItemLabel(ETDPauseItem Item)
{
	switch (Item)
	{
	case ETDPauseItem::Resume: return TEXT("RESUME");
	case ETDPauseItem::Settings: return TEXT("SETTINGS");
	case ETDPauseItem::RestartScene: return TEXT("RESTART SCENE");
	default: return TEXT("QUIT TO MENU");
	}
}

FBox2D UTDPauseMenuWidget::ItemRect(int32 Index)
{
	using namespace TDPauseMenu;
	const float Y = RowTop + Index * RowStep;
	return FBox2D(FVector2D(RowX, Y), FVector2D(RowX + RowW, Y + RowH));
}

FBox2D UTDPauseMenuWidget::ConfirmRect(int32 Choice)
{
	using namespace TDPauseMenu;
	const float W = (RowW - 16.f) * .5f;
	const float X = RowX + Choice * (W + 16.f);
	const float Y = RowTop + 2 * RowStep;
	return FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + RowH));
}

// ------------------------------------------------------------------------------------------ navigation

bool UTDPauseMenuWidget::PlayUISound(const TCHAR* Which)
{
	++UISoundRequests.FindOrAdd(Which);
	// T1's static UFUNCTION PlayUISound(WorldContext, ETDUISound), by name so this builds before it reaches main.
	UFunction* Play = UTDSettingsSubsystem::StaticClass()->FindFunctionByName(TEXT("PlayUISound"));
	if (!Play)
	{
		return false;
	}
	TArray<uint8> Params;
	Params.SetNumZeroed(FMath::Max<int32>(Play->ParmsSize, 16));
	bool bSoundSet = false;
	for (TFieldIterator<FProperty> It(Play); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}
		void* Value = It->ContainerPtrToValuePtr<void>(Params.GetData());
		if (const FObjectPropertyBase* Obj = CastField<FObjectPropertyBase>(*It))
		{
			Obj->SetObjectPropertyValue(Value, this);
		}
		else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(*It))
		{
			const int64 V = EnumProp->GetEnum()->GetValueByNameString(Which);
			if (V != INDEX_NONE)
			{
				EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(Value, V);
				bSoundSet = true;
			}
		}
		else if (const FByteProperty* ByteProp = CastField<FByteProperty>(*It); ByteProp && ByteProp->Enum)
		{
			const int64 V = ByteProp->Enum->GetValueByNameString(Which);
			if (V != INDEX_NONE)
			{
				ByteProp->SetPropertyValue(Value, static_cast<uint8>(V));
				bSoundSet = true;
			}
		}
	}
	if (!bSoundSet)
	{
		return false; // the enum has no such entry: stay silent rather than play the wrong sound
	}
	UTDSettingsSubsystem::StaticClass()->GetDefaultObject()->ProcessEvent(Play, Params.GetData());
	return true;
}

void UTDPauseMenuWidget::MoveSelection(int32 Delta)
{
	if (bConfirming)
	{
		const int32 Before = ConfirmChoice;
		ConfirmChoice = (ConfirmChoice + (Delta != 0 ? 1 : 0)) % 2;
		if (ConfirmChoice != Before)
		{
			PlayUISound(TEXT("Move"));
		}
		return;
	}
	const int32 Before = Selected;
	Selected = (Selected + Delta % ItemCount + ItemCount) % ItemCount;
	if (Selected != Before)
	{
		PlayUISound(TEXT("Move"));
	}
}

void UTDPauseMenuWidget::Back()
{
	if (bConfirming)
	{
		bConfirming = false;
		PlayUISound(TEXT("Back"));
		return;
	}
	Execute(ETDPauseItem::Resume); // closing unpauses: PauseOut plays from the controller
}

void UTDPauseMenuWidget::Activate()
{
	PlayUISound(TEXT("Confirm"));
	const ETDPauseItem Item = static_cast<ETDPauseItem>(Selected);
	if (bConfirming)
	{
		bConfirming = false;
		if (ConfirmChoice == 0)
		{
			Execute(Item);
		}
		return;
	}
	if (Item == ETDPauseItem::RestartScene || Item == ETDPauseItem::QuitToMenu)
	{
		bConfirming = true;
		ConfirmChoice = 1; // CANCEL first: a stray press never throws the scene away
		return;
	}
	Execute(Item);
}

bool UTDPauseMenuWidget::HandleKey(const FKey& Key)
{
	if (Key == EKeys::W || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		MoveSelection(-1);
		return true;
	}
	if (Key == EKeys::S || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		MoveSelection(1);
		return true;
	}
	if (bConfirming && (Key == EKeys::A || Key == EKeys::D || Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Left
			|| Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Left || Key == EKeys::Gamepad_LeftStick_Right))
	{
		MoveSelection(1);
		return true;
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Virtual_Gamepad_Accept.GetVirtualKey() || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		Activate();
		return true;
	}
	if (Key == EKeys::BackSpace || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Virtual_Gamepad_Back.GetVirtualKey())
	{
		Back();
		return true;
	}
	// Escape / Start: the controller's pause input (bTriggerWhenPaused) resumes and the host closes this screen.
	return false;
}

FReply UTDPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	return HandleKey(InKeyEvent.GetKey()) ? FReply::Handled() : Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

int32 UTDPauseMenuWidget::HitTest(const FGeometry& Geometry, const FVector2D& ScreenPos, bool& bOutConfirm) const
{
	const float Scale = FMath::Min(Geometry.GetLocalSize().X / 1920.f, Geometry.GetLocalSize().Y / 1080.f);
	if (Scale <= 0.f)
	{
		return INDEX_NONE;
	}
	const FVector2D Origin = (Geometry.GetLocalSize() - FVector2D(1920, 1080) * Scale) * .5;
	const FVector2D Design = (Geometry.AbsoluteToLocal(ScreenPos) - Origin) / Scale;
	bOutConfirm = bConfirming;
	if (bConfirming)
	{
		for (int32 C = 0; C < 2; ++C)
		{
			if (ConfirmRect(C).IsInside(Design))
			{
				return C;
			}
		}
		return INDEX_NONE;
	}
	for (int32 I = 0; I < ItemCount; ++I)
	{
		if (ItemRect(I).IsInside(Design))
		{
			return I;
		}
	}
	return INDEX_NONE;
}

FReply UTDPauseMenuWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	bool bConfirm = false;
	const int32 Hit = HitTest(InGeometry, InMouseEvent.GetScreenSpacePosition(), bConfirm);
	if (Hit != INDEX_NONE)
	{
		int32& Row = bConfirm ? ConfirmChoice : Selected;
		if (Row != Hit)
		{
			Row = Hit;
			PlayUISound(TEXT("Move")); // only when the highlight actually moves, not on every mouse event
		}
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UTDPauseMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bool bConfirm = false;
		const int32 Hit = HitTest(InGeometry, InMouseEvent.GetScreenSpacePosition(), bConfirm);
		if (Hit != INDEX_NONE)
		{
			(bConfirm ? ConfirmChoice : Selected) = Hit;
			Activate();
			return FReply::Handled();
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

// ------------------------------------------------------------------------------------------ actions

void UTDPauseMenuWidget::Execute(ETDPauseItem Item)
{
	APlayerController* PC = GetOwningPlayer();
	auto Unpause = [this, PC]()
	{
		if (ATDSpacePlayerController* SPC = Cast<ATDSpacePlayerController>(PC))
		{
			SPC->SetPlayerPaused(false);
		}
		else
		{
			UGameplayStatics::SetGamePaused(this, false);
		}
	};
	switch (Item)
	{
	case ETDPauseItem::Resume:
		LastAction = TEXT("resume");
		if (bExecuteActions)
		{
			Unpause();
		}
		break;
	case ETDPauseItem::Settings:
		// T1's settings page (UTDUIStyle::SettingsWidgetClass if Nick sets one, else T1's code-built page), over this screen.
		LastAction = TEXT("settings");
		if (bExecuteActions && !UTDSettingsSubsystem::OpenSettingsPage(this))
		{
			LastAction = TEXT("settings-missing");
			Toast = TEXT("SETTINGS COULD NOT OPEN");
			ToastUntil = FPlatformTime::Seconds() + 2.0;
			UE_LOG(LogTDPauseMenu, Warning, TEXT("UTDSettingsSubsystem::OpenSettingsPage refused (no viewport)."));
		}
		break;
	case ETDPauseItem::RestartScene:
		LastAction = TEXT("restart");
		if (bExecuteActions)
		{
			Unpause();
			if (UTDGameInstance* GI = Cast<UTDGameInstance>(UGameplayStatics::GetGameInstance(this)))
			{
				GI->RestartSceneNow();
			}
		}
		break;
	case ETDPauseItem::QuitToMenu:
		LastAction = TEXT("quit");
		if (bExecuteActions)
		{
			Unpause();
			if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
			{
				Flow->TravelTo(ETDDestination::MainMenu, NAME_None);
			}
		}
		break;
	}
}

// ------------------------------------------------------------------------------------------ paint

int32 UTDPauseMenuWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Culling, FSlateWindowElementList& List,
	int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	using namespace TDPauseMenu;
	const int32 Base = Super::NativePaint(Args, G, Culling, List, LayerId, Style, bParentEnabled);
	const FTDUIPainter P(G, List, Base + 1);
	if (P.Scale <= 0.f)
	{
		return Base;
	}
	const FLinearColor Accent = FTDUIPalette::Accent();
	const FLinearColor Ink = FTDUIPalette::Ink();
	const FLinearColor Dim = FTDUIPalette::Muted();
	const FLinearColor Hostile = FTDUIPalette::Hostile();

	// Dim the (blurred) frame over the whole widget, letterbox included.
	FSlateDrawElement::MakeBox(List, Base + 1, G.ToPaintGeometry(), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None,
		FLinearColor(.004f, .01f, .02f, .55f));

	P.Glass(PanelX, PanelY, PanelW, PanelH, Accent, true);
	P.Label(RowX, PanelY + 22, TEXT("SCENE ON HOLD"), 11, Accent, 150);
	if (!DifficultyLine.IsEmpty())
	{
		// The difficulty in play, on the same line, right side (T1, at the Lead's request).
		P.Label(RowX + RowW - 190, PanelY + 22, TEXT("DIFFICULTY"), 11, Dim, 120);
		P.Label(RowX + RowW - 82, PanelY + 22, DifficultyLine, 11, Ink, 120);
	}
	P.Title(RowX, PanelY + 46, TEXT("PAUSED"), 40, Ink);
	P.Line({ RowX, PanelY + 112 }, { RowX + RowW, PanelY + 112 }, Accent.CopyWithNewOpacity(.2f));

	auto Row = [&](const FBox2D& R, const FString& Label, bool bSelected, bool bDanger)
	{
		const FVector2D Min = R.Min, Size = R.GetSize();
		const FLinearColor C = bDanger ? Hostile : Accent;
		if (bSelected)
		{
			P.Gradient(Min.X, Min.Y, Size.X, Size.Y, C.CopyWithNewOpacity(.30f), C.CopyWithNewOpacity(.04f), false);
			P.Box(Min.X, Min.Y, 4, Size.Y, C);
			P.Frame(Min.X, Min.Y, Size.X, Size.Y, C, .55f);
			P.Line({ Min.X + Size.X - 34, Min.Y + Size.Y * .5f - 6 }, { Min.X + Size.X - 26, Min.Y + Size.Y * .5f }, Ink, 2);
			P.Line({ Min.X + Size.X - 26, Min.Y + Size.Y * .5f }, { Min.X + Size.X - 34, Min.Y + Size.Y * .5f + 6 }, Ink, 2);
		}
		else
		{
			P.Box(Min.X, Min.Y, Size.X, Size.Y, FLinearColor(.02f, .05f, .08f, .55f));
			P.Frame(Min.X, Min.Y, Size.X, Size.Y, Accent, .14f);
		}
		P.Title(Min.X + 22, Min.Y + 14, Label, 22, bSelected ? Ink : Dim);
	};

	if (bConfirming)
	{
		const bool bQuit = static_cast<ETDPauseItem>(Selected) == ETDPauseItem::QuitToMenu;
		P.Label(RowX, RowTop + 4, bQuit ? TEXT("QUIT TO MENU") : TEXT("RESTART SCENE"), 11, Hostile, 120);
		P.Title(RowX, RowTop + 30, bQuit ? TEXT("LEAVE THIS TAKE?") : TEXT("CALL A NEW TAKE?"), 26, Ink);
		P.Text(RowX, RowTop + 72, bQuit ? TEXT("Progress since the last scene is lost.") : TEXT("The scene restarts from its checkpoint."), 14, Dim);
		Row(ConfirmRect(0), TEXT("CONFIRM"), ConfirmChoice == 0, true);
		Row(ConfirmRect(1), TEXT("CANCEL"), ConfirmChoice == 1, false);
	}
	else
	{
		for (int32 I = 0; I < ItemCount; ++I)
		{
			Row(ItemRect(I), ItemLabel(static_cast<ETDPauseItem>(I)), I == Selected, false);
		}
	}

	if (!Toast.IsEmpty() && FPlatformTime::Seconds() < ToastUntil)
	{
		P.CenteredText(PanelX + PanelW * .5f, PanelY + PanelH - 70, Toast, 13, Hostile, true);
	}
	P.Line({ RowX, PanelY + PanelH - 44 }, { RowX + RowW, PanelY + PanelH - 44 }, Accent.CopyWithNewOpacity(.15f));
	P.Text(RowX, PanelY + PanelH - 34, bConfirming ? TEXT("[A/D] CHOOSE     [ENTER] OK     [BACKSPACE] BACK") : TEXT("[W/S] SELECT     [ENTER] CHOOSE     [ESC] RESUME"),
		12, Dim);
	return Base + 3;
}
