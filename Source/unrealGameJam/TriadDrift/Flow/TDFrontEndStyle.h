// Triad Drift - the focused look for code-built front-end buttons (main menu, victory, settings page). A pad or
// keyboard player must always see which button Enter / A presses (T1's pad pass, Nov 13). Nick's widgets, when present,
// replace the code-built screens entirely and bring their own style. Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/CoreStyle.h"

namespace TDFrontEndStyle
{
	/** The squad-colour accent of the focus frame (Lancer cyan, the title colour). */
	inline FLinearColor FocusColor() { return FLinearColor(0.4f, 0.9f, 1.f, 1.f); }

	/** True while the button has the keyboard / pad focus of any user, or the mouse is over it. */
	inline bool IsHighlighted(const TSharedPtr<SButton>& B)
	{
		return B.IsValid() && B->IsEnabled() && (B->HasAnyUserFocus().IsSet() || B->HasKeyboardFocus() || B->IsHovered());
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
}
