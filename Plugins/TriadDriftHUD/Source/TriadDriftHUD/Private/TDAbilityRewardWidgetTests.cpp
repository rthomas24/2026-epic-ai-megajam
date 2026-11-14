#include "TDAbilityRewardWidget.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/GarbageCollection.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRewardNativeLifecycle,"TriadDrift.UI.Reward.OnFootDismissAndNextUnlock",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDRewardNativeLifecycle::RunTest(const FString&)
{
    TStrongObjectPtr<UTDAbilityRewardWidget> W(NewObject<UTDAbilityRewardWidget>());
    if(!TestTrue(TEXT("native widget initializes"),W->Initialize())) return false;
    const TSharedRef<SWidget> Slate=W->TakeWidget();
    FTDAbilityRewardDisplay D;
    D.AbilityId=TEXT("GravityWell");D.AbilityName=FText::FromString(TEXT("GRAVITY WELL"));
    D.ShipName=FText::FromString(TEXT("BASTION"));D.ActivationKey=FText::FromString(TEXT("E"));
    TestTrue(TEXT("on-foot reward presents even when unusable"),W->PresentReward(D));
    TestTrue(TEXT("visible on foot"),W->GetVisibility()==ESlateVisibility::SelfHitTestInvisible);
    TestTrue(TEXT("unusable award suppresses the supplied activation key"),W->GetActivationKey().IsEmpty());
    TestFalse(TEXT("on-foot availability still readable"),W->GetAvailabilityText().IsEmpty());
    FTDAbilityRewardDisplay Next=D;Next.AbilityId=TEXT("Cloak");Next.AbilityName=FText::FromString(TEXT("CLOAK"));Next.ShipName=FText::FromString(TEXT("RIFT"));
    TestFalse(TEXT("cannot silently overwrite an undismissed award"),W->PresentReward(Next));
    W->RequestDismiss();W->RequestDismiss();
    TestEqual(TEXT("dismiss notifies exactly once"),W->GetDismissNotificationCount(),1);
    TestFalse(TEXT("dismissed identity does not reopen on refresh"),W->PresentReward(D));
    TestTrue(TEXT("a different unlock still presents"),W->PresentReward(Next));
    TestTrue(TEXT("next reward is active"),W->IsRewardActive());
    Next.bCanUseNow=true;
    for(const TCHAR* Key:{TEXT("E"),TEXT("Y")})
    {
        Next.ActivationKey=FText::FromString(Key);W->PresentReward(Next);
        TestEqual(TEXT("adapter can refresh keyboard/controller binding"),W->GetActivationKey().ToString(),FString(Key));
    }
    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    TestEqual(TEXT("reward survives GC while owned"),W->GetPresentedAbilityId(),FName(TEXT("Cloak")));
    W->ResetPresentation();
    TestEqual(TEXT("suppression/reset does not acknowledge an award"),W->GetDismissNotificationCount(),1);
    TestFalse(TEXT("reset hides reward"),W->IsRewardActive());
    TestTrue(TEXT("owner may re-present pending identity after suppression"),W->PresentReward(Next));
    return true;
}
#endif
