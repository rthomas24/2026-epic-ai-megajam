// Triad Drift - the focused look for code-built front-end buttons (main menu, victory, settings page). A pad or
// keyboard player must always see which button Enter / A presses (T1's pad pass, Nov 13). Nick's widgets, when present,
// replace the code-built screens entirely and bring their own style. Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SCompoundWidget.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

namespace TDFrontEndStyle
{
	/** The squad-colour accent of the focus frame (Lancer cyan, the title colour). */
	inline FLinearColor FocusColor() { return FLinearColor(0.4f, 0.9f, 1.f, 1.f); }

	/**
	 * True while the button has the keyboard / pad focus of any user. The mouse does not light a button by itself: hovering
	 * a menu button moves the focus to it (TakeFocusOnHover), and W / S take it back, so exactly one button is lit at a time
	 * (T7's Shipping pass, Nov 14: a hovered button and the focused one were both lit).
	 */
	inline bool IsHighlighted(const TSharedPtr<SButton>& B)
	{
		return B.IsValid() && B->IsEnabled() && (B->HasAnyUserFocus().IsSet() || B->HasKeyboardFocus());
	}

	/** The mouse entering a menu button gives it the focus of every user (the keys then continue from where the mouse is). */
	inline void TakeFocusOnHover(const TSharedRef<SButton>& Button)
	{
		const TWeakPtr<SButton> Weak = Button;
		Button->SetOnHovered(FSimpleDelegate::CreateLambda([Weak]()
		{
			const TSharedPtr<SButton> B = Weak.Pin();
			if (B.IsValid() && B->IsEnabled() && FSlateApplication::IsInitialized() && !B->HasAnyUserFocus().IsSet())
			{
				FSlateApplication::Get().SetAllUserFocus(B, EFocusCause::Mouse);
			}
		}));
	}

	/**
	 * Wrap a button in a frame that lights up in the squad colour and grows 6% while it is the one Enter / A presses.
	 * Unfocused: a faint outline, so the layout never jumps.
	 */
	inline TSharedRef<SWidget> FocusFrame(const TSharedRef<SButton>& Button)
	{
		const TWeakPtr<SButton> Weak = Button;
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.Padding(FMargin(3.f))
			.BorderBackgroundColor_Lambda([Weak]()
			{
				return IsHighlighted(Weak.Pin()) ? FSlateColor(FocusColor()) : FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.08f));
			})
			.RenderTransformPivot(FVector2D(0.5f, 0.5f))
			.RenderTransform_Lambda([Weak]()
			{
				return IsHighlighted(Weak.Pin()) ? TOptional<FSlateRenderTransform>(FSlateRenderTransform(FScale2D(1.06f))) : TOptional<FSlateRenderTransform>();
			})
			[
				Button
			];
	}
	// ---- #142 (T1, Nov 14; direction from Nick's Lead): the native menu's own look - a restrained dark translucent
	// button with clear normal / hover-or-focus / pressed / disabled states, the Settings page's colours, no default grey.
	inline FLinearColor Ink() { return FLinearColor(0.88f, 0.95f, 1.f, 1.f); }
	inline FLinearColor Cyan() { return FLinearColor(0.40f, 0.75f, 0.81f, 1.f); }

	inline const FButtonStyle& MenuButtonStyle()
	{
		static FButtonStyle Style;
		static bool bBuilt = false;
		if (!bBuilt)
		{
			bBuilt = true;
			const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
			auto Tint = [White](const FLinearColor& C) { FSlateBrush B = *White; B.TintColor = FSlateColor(C); return B; };
			Style.SetNormal(Tint(FLinearColor(0.05f, 0.06f, 0.09f, 0.72f)));
			// Hovered = normal: hovering moves the focus, so the hover's look is the focus look (frame, grow, white label); a
			// separate hover fill would light a second button once W / S moved the focus away.
			Style.SetHovered(Tint(FLinearColor(0.05f, 0.06f, 0.09f, 0.72f)));
			Style.SetPressed(Tint(FLinearColor(0.16f, 0.38f, 0.48f, 0.98f)));
			Style.SetDisabled(Tint(FLinearColor(0.04f, 0.045f, 0.06f, 0.40f)));
			Style.SetNormalPadding(FMargin(18.f, 9.f));
			Style.SetPressedPadding(FMargin(18.f, 10.f, 18.f, 8.f));
		}
		return Style;
	}

	/** The label colour of a menu button: white when it is the one Enter / A presses, soft ink otherwise, dim when disabled. */
	inline FSlateColor MenuLabelColor(const TWeakPtr<SButton>& Weak)
	{
		const TSharedPtr<SButton> B = Weak.Pin();
		if (!B.IsValid() || !B->IsEnabled()) { return FSlateColor(FLinearColor(0.88f, 0.95f, 1.f, 0.32f)); }
		return IsHighlighted(B) ? FSlateColor(FLinearColor::White) : FSlateColor(FLinearColor(0.80f, 0.90f, 0.97f, 0.92f));
	}

	/**
	 * The root of a code-built menu page: W / S move the selection like the arrows (T7's Shipping run), and
	 * Esc / Backspace / pad B go back when the page has a way back. Everything else reaches the buttons untouched.
	 */
	class STDMenuRoot : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(STDMenuRoot) {}
			SLATE_DEFAULT_SLOT(FArguments, Content)
			SLATE_EVENT(FSimpleDelegate, OnBack)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			OnBack = InArgs._OnBack;
			ChildSlot[ InArgs._Content.Widget ];
		}

		virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override
		{
			const FKey Key = KeyEvent.GetKey();
			if (Key == EKeys::W) { return FReply::Handled().SetNavigation(EUINavigation::Up, ENavigationGenesis::Keyboard); }
			if (Key == EKeys::S) { return FReply::Handled().SetNavigation(EUINavigation::Down, ENavigationGenesis::Keyboard); }
			if (OnBack.IsBound() && !KeyEvent.IsRepeat()
				&& (Key == EKeys::Escape || Key == EKeys::BackSpace || Key == EKeys::Gamepad_FaceButton_Right))
			{
				OnBack.Execute();
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

	private:
		FSimpleDelegate OnBack;
	};
}
