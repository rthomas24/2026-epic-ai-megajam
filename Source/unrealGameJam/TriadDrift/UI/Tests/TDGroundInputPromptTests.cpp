#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/UI/TDGroundHUDWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace TDGroundInputPromptTests
{
	void Press(ATDSpacePlayerController* PC, const FKey& Key)
	{
		static_cast<APlayerController*>(PC)->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f));
		static_cast<APlayerController*>(PC)->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundInstructionDeviceSwitch,
	"TriadDrift.UI.GroundHUD.InstructionDeviceSwitchAndPawnRebind", TD_TEST_FLAGS)
bool FTDGroundInstructionDeviceSwitch::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDFirstPersonCharacter* Walker = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Walker, ATDSpacePlayerController::StaticClass()));
	const TStrongObjectPtr<UTDGroundHUDWidget> Keep(PC ? CreateWidget<UTDGroundHUDWidget>(PC, UTDGroundHUDWidget::StaticClass()) : nullptr);
	UTDGroundHUDWidget* HUD = Keep.Get();
	if (!TestNotNull(TEXT("walker"), Walker) || !TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("ground HUD"), HUD))
	{
		return false;
	}
	const FString Objective(TEXT("RECOVER THE STABILISING CRYSTALS · HOLD [E]"));
	const FString Tip(TEXT("RETURN THEIR FIRE · BLOCK [RMB] AS IT HITS"));
	HUD->ForceObjective = Objective;
	Walker->ShowTip(FText::FromString(Tip), 60.f);
	TDGroundInputPromptTests::Press(PC, EKeys::F);
	HUD->Sample(0.f);
	TestEqual(TEXT("keyboard objective"), HUD->GetView().Objective, Objective);
	TestEqual(TEXT("keyboard prompt"), HUD->GetView().Prompt, Tip);

	// Same raw source text and same widget: a device change must refresh the labels at once.
	TDGroundInputPromptTests::Press(PC, EKeys::Gamepad_DPad_Up);
	TestTrue(TEXT("real controller input path tracks pad"), PC->IsUsingGamepad());
	HUD->Sample(0.f);
	TestEqual(TEXT("pad interact is X"), HUD->GetView().Objective, FString(TEXT("RECOVER THE STABILISING CRYSTALS · HOLD [X]")));
	TestEqual(TEXT("pad block is LT"), HUD->GetView().Prompt, FString(TEXT("RETURN THEIR FIRE · BLOCK [LT] AS IT HITS")));
	TestEqual(TEXT("canonical objective remains unchanged"), HUD->ForceObjective, Objective);
	TestEqual(TEXT("canonical pawn text remains unchanged"), Walker->GetInteractPrompt().ToString(), Tip);

	TDGroundInputPromptTests::Press(PC, EKeys::F);
	HUD->Sample(0.f);
	TestEqual(TEXT("same objective restores E on keyboard"), HUD->GetView().Objective, Objective);
	TestEqual(TEXT("same prompt restores RMB on keyboard"), HUD->GetView().Prompt, Tip);

	ATDFirstPersonCharacter* NextWalker = W.Spawn<ATDFirstPersonCharacter>(FVector(200.f, 0.f, 95.f));
	if (!TestNotNull(TEXT("replacement pawn"), NextWalker)) { return false; }
	NextWalker->ShowTip(FText::FromString(TEXT("HOLD [E]")), 60.f);
	PC->Possess(NextWalker);
	TDGroundInputPromptTests::Press(PC, EKeys::Gamepad_DPad_Up);
	HUD->Sample(0.f);
	TestEqual(TEXT("existing widget follows replacement pawn"), HUD->GetView().Prompt, FString(TEXT("HOLD [X]")));
	TDGroundInputPromptTests::Press(PC, EKeys::F);
	HUD->Sample(0.f);
	TestEqual(TEXT("replacement pawn also restores keyboard"), HUD->GetView().Prompt, FString(TEXT("HOLD [E]")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDGroundInstructionLiteralScope,
	"TriadDrift.UI.GroundHUD.InstructionLiteralScope", TD_TEST_FLAGS)
bool FTDGroundInstructionLiteralScope::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDFirstPersonCharacter* Walker = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Walker, ATDSpacePlayerController::StaticClass()));
	const TStrongObjectPtr<UTDGroundHUDWidget> Keep(PC ? CreateWidget<UTDGroundHUDWidget>(PC, UTDGroundHUDWidget::StaticClass()) : nullptr);
	UTDGroundHUDWidget* HUD = Keep.Get();
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("ground HUD"), HUD)) { return false; }
	HUD->ForceObjective = TEXT("ENERGY [ECHO] · E / RMB · [E] [RMB] [F]");
	TDGroundInputPromptTests::Press(PC, EKeys::Gamepad_DPad_Up);
	HUD->Sample(0.f);
	TestEqual(TEXT("only recognized ground tokens change"), HUD->GetView().Objective,
		FString(TEXT("ENERGY [ECHO] · E / RMB · [X] [LT] [F]")));
	return true;
}

#endif
