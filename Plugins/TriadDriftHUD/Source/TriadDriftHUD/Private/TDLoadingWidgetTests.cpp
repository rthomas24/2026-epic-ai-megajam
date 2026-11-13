#include "TDLoadingWidget.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/GarbageCollection.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDLoadingHostSlot,"TriadDrift.UI.Loading.HostSlot",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDLoadingHostSlot::RunTest(const FString&)
{
    UClass* Host=FindObject<UClass>(nullptr,TEXT("/Script/unrealGameJam.TDTransitionGameMode"));
    if(!TestNotNull(TEXT("existing engineering transition host"),Host)) return false;
    FClassProperty* Slot=FindFProperty<FClassProperty>(Host,TEXT("LoadingWidgetClass"));
    if(!TestNotNull(TEXT("existing loading override slot"),Slot)) return false;
    TestTrue(TEXT("native widget fits existing slot without another host"),UTDLoadingWidget::StaticClass()->IsChildOf(Slot->MetaClass));
    TStrongObjectPtr<UTDLoadingWidget> Widget(NewObject<UTDLoadingWidget>());
    // Exercise actual Slate construction; this is where input visibility/focus is configured.
    // NewObject bypasses CreateWidget; initialize its WidgetTree before Slate navigation.
    if(!TestTrue(TEXT("initialize native UUserWidget lifecycle"),Widget->Initialize())) return false;
    const TSharedRef<SWidget> Slate=Widget->TakeWidget();
    TestFalse(TEXT("transition presentation cannot take keyboard focus"),Widget->IsFocusable());
    TestTrue(TEXT("transition presentation cannot intercept mouse input"),Widget->GetVisibility()==ESlateVisibility::HitTestInvisible);
    Widget->SceneTitle=FText::FromString(TEXT("CINDER / OUTPOST"));
    Widget->StatusText=FText::FromString(TEXT("Preparing the next scene..."));
    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    TestEqual(TEXT("host-supplied scene survives GC while Slate exists"),Widget->SceneTitle.ToString(),FString(TEXT("CINDER / OUTPOST")));
    return true;
}
#endif
