// Triad Drift - F10 debug menu (ENG-19). Owner: Teammate 5.

#include "TriadDrift/Flow/TDDebugMenu.h"
#include "TriadDrift/Flow/TDCheatManager.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Flow/TDJourney.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

FTDDebugMenu::~FTDDebugMenu()
{
	Hide();
}

void FTDDebugMenu::Toggle(UWorld* World)
{
	if (IsVisible())
	{
		Hide();
	}
	else
	{
		Show(World);
	}
}

void FTDDebugMenu::Show(UWorld* World)
{
#if !UE_BUILD_SHIPPING
	if (IsVisible() || !World || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	WorldPtr = World;
	Widget = Build();
	GEngine->GameViewport->AddViewportWidgetContent(Widget.ToSharedRef(), 1000);
	SetCursor(true);
#endif
}

void FTDDebugMenu::Hide()
{
	if (Widget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Widget.ToSharedRef());
	}
	if (Widget.IsValid())
	{
		SetCursor(false);
	}
	Widget.Reset();
}

void FTDDebugMenu::SetCursor(bool bVisible)
{
	UWorld* World = WorldPtr.Get();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}
	PC->SetShowMouseCursor(bVisible);
	if (bVisible)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

TSharedRef<SWidget> FTDDebugMenu::Build()
{
	TWeakPtr<FTDDebugMenu> WeakThis = AsShared();
	auto World = [WeakThis]() -> UWorld*
	{
		const TSharedPtr<FTDDebugMenu> Self = WeakThis.Pin();
		return Self ? Self->WorldPtr.Get() : nullptr;
	};
	auto MakeButton = [](const FString& Label, TFunction<void()> Action) -> TSharedRef<SWidget>
	{
		return SNew(SBox).Padding(2.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
				[
					SNew(STextBlock).Text(FText::FromString(Label))
				]
			];
	};
	auto Header = [](const FString& Text) -> TSharedRef<SWidget>
	{
		return SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle("Bold", 13)).ColorAndOpacity(FLinearColor(1.f, 0.8f, 0.3f));
	};

	TSharedRef<SWrapBox> Actions = SNew(SWrapBox).UseAllottedSize(true);
	Actions->AddSlot()[MakeButton(TEXT("God mode"), [World]() { UTDCheatManager::DoGod(World()); })];
	Actions->AddSlot()[MakeButton(TEXT("Kill all"), [World]() { UTDCheatManager::DoKillAll(World()); })];
	Actions->AddSlot()[MakeButton(TEXT("Skip wave"), [World]() { UTDCheatManager::DoSkipWave(World()); })];
	Actions->AddSlot()[MakeButton(TEXT("CUT! (retake)"), [World]() { UTDCheatManager::DoTake(World()); })];

	TSharedRef<SWrapBox> Journey = SNew(SWrapBox).UseAllottedSize(true);
	if (const UEnum* E = StaticEnum<ETDJourneyStage>())
	{
		for (int32 i = 0; i < E->NumEnums() - 1; ++i)
		{
			const FString Name = E->GetNameStringByIndex(i);
			Journey->AddSlot()[MakeButton(Name, [World, Name]() { UTDCheatManager::DoStage(World(), Name); })];
		}
	}
	Actions->AddSlot()[MakeButton(TEXT("Unlock all"), [World]() { UTDCheatManager::DoUnlock(World(), TEXT("All"), FString()); })];
	for (const TCHAR* D : { TEXT("Story"), TEXT("Standard"), TEXT("Ace") })
	{
		const FString Name(D);
		Actions->AddSlot()[MakeButton(FString::Printf(TEXT("Difficulty: %s"), D), [World, Name]() { UTDCheatManager::DoDifficulty(World(), Name); })];
	}

	TSharedRef<SWrapBox> Phases = SNew(SWrapBox).UseAllottedSize(true);
	if (const UEnum* E = StaticEnum<ETDGamePhase>())
	{
		for (int32 i = 0; i < E->NumEnums() - 1; ++i)
		{
			const FString Name = E->GetNameStringByIndex(i);
			Phases->AddSlot()[MakeButton(Name, [World, Name]() { UTDCheatManager::DoPhase(World(), Name); })];
		}
	}

	TSharedRef<SWrapBox> Unlocks = SNew(SWrapBox).UseAllottedSize(true);
	if (const UEnum* E = StaticEnum<ETDAbilityId>())
	{
		for (int32 i = 1; i < E->NumEnums() - 1; ++i) // skip None
		{
			const FString Name = E->GetNameStringByIndex(i);
			Unlocks->AddSlot()[MakeButton(Name, [World, Name]() { UTDCheatManager::DoUnlock(World(), Name, FString()); })];
		}
	}

	TSharedRef<SWrapBox> Travel = SNew(SWrapBox).UseAllottedSize(true);
	if (const UEnum* E = StaticEnum<ETDDestination>())
	{
		for (int32 i = 0; i < E->NumEnums() - 1; ++i)
		{
			const FString Name = E->GetNameStringByIndex(i);
			Travel->AddSlot()[MakeButton(Name, [World, Name]() { UTDCheatManager::DoTravel(World(), Name); })];
		}
	}

	return SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(FMargin(24.f, 24.f))
		.WidthOverride(560.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.04f, 0.88f))
			.Padding(12.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()[Header(TEXT("TRIAD DRIFT DEBUG (F10)"))]
				+ SScrollBox::Slot().Padding(0, 6)[Actions]
				+ SScrollBox::Slot()[Header(TEXT("Journey stage (progress + world)"))]
				+ SScrollBox::Slot().Padding(0, 6)[Journey]
				+ SScrollBox::Slot()[Header(TEXT("Phase (progress only)"))]
				+ SScrollBox::Slot().Padding(0, 6)[Phases]
				+ SScrollBox::Slot()[Header(TEXT("Unlock ability"))]
				+ SScrollBox::Slot().Padding(0, 6)[Unlocks]
				+ SScrollBox::Slot()[Header(TEXT("Travel"))]
				+ SScrollBox::Slot().Padding(0, 6)[Travel]
			]
		];
}
