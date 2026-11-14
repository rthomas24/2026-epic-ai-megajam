#include "TDMainMenuWidget.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMenuSettingsContract,"TriadDrift.UI.Menu.EngineeringSettings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDMenuSettingsContract::RunTest(const FString&)
{
    bool Result=true;
    TestFalse(TEXT("null settings context refused"),UTDMainMenuWidget::InvokeSettingsPage(nullptr,TEXT("OpenSettingsPage"),Result));
    TestFalse(TEXT("no fabricated settings success"),Result);
    TestFalse(TEXT("settings bridge cannot dispatch unrelated subsystem actions"),
        UTDMainMenuWidget::InvokeSettingsPage(GetTransientPackage(),TEXT("ResetToDefaults"),Result));
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("valid settings world context"),World)) return false;
    TestTrue(TEXT("current engineering public settings getter is compatible"),
        UTDMainMenuWidget::InvokeSettingsPage(World,TEXT("IsSettingsPageOpen"),Result));
    TestFalse(TEXT("no game instance means no settings page"),Result);
    TestTrue(TEXT("open bridge is valid even when no viewport exists"),
        UTDMainMenuWidget::InvokeSettingsPage(World,TEXT("OpenSettingsPage"),Result));
    TestFalse(TEXT("valid bridge reports no-viewport open failure without fabricated success"),Result);
    World->DestroyWorld(false);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMenuHostContract,"TriadDrift.UI.Menu.HostContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDMenuHostContract::RunTest(const FString&)
{
    UClass* Host=FindObject<UClass>(nullptr,TEXT("/Script/unrealGameJam.TDMenuGameMode"));
    if(!TestNotNull(TEXT("existing engineering menu host is loaded"),Host)) return false;
    for(const TCHAR* Name:{TEXT("NewGame"),TEXT("ContinueGame"),TEXT("CanContinue"),TEXT("IsShowingVictory")})
    {
        UFunction* Fn=Host->FindFunctionByName(Name);
        if(!TestNotNull(Name,Fn)) continue;
        TestNotNull(TEXT("public action returns bool"),CastField<FBoolProperty>(Fn->GetReturnProperty()));
        for(TFieldIterator<FProperty> It(Fn);It;++It)
            TestFalse(TEXT("zero-input action contract"),It->HasAnyPropertyFlags(CPF_Parm) && !It->HasAnyPropertyFlags(CPF_ReturnParm));
    }
    for(const TCHAR* Name:{TEXT("QuitGame"),TEXT("ReturnToMenu")})
    {
        UFunction* Fn=Host->FindFunctionByName(Name);
        if(TestNotNull(Name,Fn)) TestNull(TEXT("void action contract"),Fn->GetReturnProperty());
    }
    const FClassProperty* Slot=FindFProperty<FClassProperty>(Host,TEXT("MenuWidgetClass"));
    TestNotNull(TEXT("reuse existing widget slot"),Slot);
    if(Slot) TestTrue(TEXT("slot accepts native UMG menu"),UTDMainMenuWidget::StaticClass()->IsChildOf(Slot->MetaClass));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMenuRejectsBadHost,"TriadDrift.UI.Menu.RejectsIncompatibleHost",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDMenuRejectsBadHost::RunTest(const FString&)
{
    bool Result=true;
    TestFalse(TEXT("null host rejected"),UTDMainMenuWidget::InvokeHost(nullptr,TEXT("NewGame"),Result));
    TestFalse(TEXT("null host cannot report success"),Result);
    TestFalse(TEXT("missing method rejected"),UTDMainMenuWidget::InvokeHost(GetTransientPackage(),TEXT("NewGame"),Result));
    TestFalse(TEXT("no fabricated successful travel"),Result);
    UClass* Host=FindObject<UClass>(nullptr,TEXT("/Script/unrealGameJam.TDMenuGameMode"));
    if(Host)
    {
        TestTrue(TEXT("real CanContinue getter can be read without gameplay module dependency"),
            UTDMainMenuWidget::InvokeHost(Host->GetDefaultObject(),TEXT("CanContinue"),Result));
        TestFalse(TEXT("no save/game instance does not enable Continue"),Result);
        TestFalse(TEXT("parameter-taking inherited method refused"),
            UTDMainMenuWidget::InvokeHost(Host->GetDefaultObject(),TEXT("K2_SetActorLocation"),Result));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMenuMotionAccess,"TriadDrift.UI.Menu.MotionAccessibility",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDMenuMotionAccess::RunTest(const FString&)
{
    TStrongObjectPtr<UTDMainMenuWidget> Menu(NewObject<UTDMainMenuWidget>());
    Menu->SetReducedMenuMotion(true);
    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    TestTrue(TEXT("reduced motion remains valid through GC"),Menu->bReducedMenuMotion);
    TestFalse(TEXT("settings never start travel"),Menu->IsTransitionPending());
    Menu->SetReducedMenuMotion(false);
    TestFalse(TEXT("motion can be restored"),Menu->bReducedMenuMotion);
    return true;
}
#endif
