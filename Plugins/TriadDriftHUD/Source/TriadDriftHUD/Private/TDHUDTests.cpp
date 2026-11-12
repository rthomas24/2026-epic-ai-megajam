#include "Misc/AutomationTest.h"
#include "TDSpaceHUDWidget.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/KismetTextLibrary.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/StructOnScope.h"

namespace TDHUD { bool Text(UObject* O,FName Name,FText& Out); }

namespace TDHUDTest
{
// Actual gameplay classes are resolved at runtime; the plugin has no game-module dependency.
struct FObjectiveWorld
{
    UGameInstance* Instance=nullptr;
    UWorld* World=nullptr;
    FObjectiveWorld()
    {
        if(!GEngine) return;
        Instance=NewObject<UGameInstance>(GEngine); Instance->AddToRoot();
        Instance->InitializeStandalone(); World=Instance->GetWorld();
        if(!World) return;
        if(AWorldSettings* Settings=World->GetWorldSettings()) Settings->DefaultGameMode=AGameModeBase::StaticClass();
        const FURL URL;
        World->SetGameMode(URL); World->InitializeActorsForPlay(URL); World->BeginPlay();
    }
    ~FObjectiveWorld()
    {
        if(World)
        {
            World->BeginTearingDown(); World->EndPlay(EEndPlayReason::LevelTransition);
            if(GEngine) GEngine->ShutdownWorldNetDriver(World);
            World->DestroyWorld(false);
            if(GEngine) GEngine->DestroyWorldContext(World);
        }
        if(Instance) { Instance->Shutdown(); Instance->RemoveFromRoot(); }
        CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHUDContractTest,"TriadDrift.UI.ContractSchema",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDHUDContractTest::RunTest(const FString& Parameters)
{
    UClass* Contract=FindObject<UClass>(nullptr,TEXT("/Script/unrealGameJam.TDHUDData"));
    if(!TestNotNull(TEXT("Ryan's actual HUD interface is loaded"),Contract)) return false;
    UFunction* Getter=Contract->FindFunctionByName(TEXT("GetSquadHUD"));
    if(!TestNotNull(TEXT("actual squad getter is reflected"),Getter)) return false;
    const FArrayProperty* Array=CastField<FArrayProperty>(Getter->GetReturnProperty());
    const FStructProperty* Item=Array?CastField<FStructProperty>(Array->Inner):nullptr;
    if(!TestNotNull(TEXT("actual getter returns a struct array"),Item)) return false;
    TestEqual(TEXT("use original gameplay struct"),Item->Struct->GetName(),FString(TEXT("TDShipHUD")));
    const TCHAR* Numeric[]={TEXT("Health01"),TEXT("Shield01"),TEXT("Boost01"),TEXT("Heat01"),TEXT("PowerCooldown01"),TEXT("Power2Cooldown01"),TEXT("AbilityCooldown01"),TEXT("RespawnSeconds"),TEXT("SecondaryAmmo")};
    for(const TCHAR* Field:Numeric) TestNotNull(Field,FindFProperty<FNumericProperty>(Item->Struct,Field));
    TestNotNull(TEXT("actual ship identity is enum"),FindFProperty<FEnumProperty>(Item->Struct,TEXT("Ship")));
    TestNotNull(TEXT("actual active flag is bool"),FindFProperty<FBoolProperty>(Item->Struct,TEXT("bPlayerControlled")));
    TestNotNull(TEXT("actual alive flag is bool"),FindFProperty<FBoolProperty>(Item->Struct,TEXT("bAlive")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHUDInterceptTest,"TriadDrift.UI.Intercept",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDHUDInterceptTest::RunTest(const FString& Parameters)
{
    FVector Aim;
    TestFalse(TEXT("unknown projectile speed hides lead"),UTDSpaceHUDWidget::SolveIntercept({1000,0,0},{0,100,0},0,Aim));
    TestTrue(TEXT("stationary target intercepts"),UTDSpaceHUDWidget::SolveIntercept({1000,0,0},FVector::ZeroVector,1000,Aim));
    TestTrue(TEXT("stationary aim remains on target"),Aim.Equals({1000,0,0},.1));
    TestTrue(TEXT("crossing target intercepts"),UTDSpaceHUDWidget::SolveIntercept({1000,0,0},{0,100,0},1000,Aim));
    TestTrue(TEXT("lead advances crossing target"),Aim.Y>100 && Aim.Y<102);
    TestFalse(TEXT("receding faster target has no positive solution"),UTDSpaceHUDWidget::SolveIntercept({1000,0,0},{2000,0,0},1000,Aim));
    TestTrue(TEXT("linear equal-speed case intercepts approaching target"),UTDSpaceHUDWidget::SolveIntercept({1000,0,0},{-1000,0,0},1000,Aim));
    TestTrue(TEXT("linear intercept is halfway"),Aim.Equals({500,0,0},.1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHUDSnapshotTest,"TriadDrift.UI.SnapshotSafety",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDHUDSnapshotTest::RunTest(const FString& Parameters)
{
    UTDSpaceHUDWidget* HUD=NewObject<UTDSpaceHUDWidget>();
    TArray<FTDUIShip> Input; Input.SetNum(3);
    Input[0].bAvailable=Input[0].bAlive=Input[0].bActive=true;
    Input[0].Hull=2; Input[0].Shield=-1; Input[0].Power=0; Input[0].Ammo=-4;
    Input[1].bAvailable=Input[1].bAlive=Input[1].bActive=true;
    HUD->ApplySquadSnapshot(Input);
    auto S=HUD->GetSnapshot();
    TestEqual(TEXT("overshoot hull clamped"),S[0].Hull,1.f);
    TestEqual(TEXT("negative shield clamped"),S[0].Shield,0.f);
    TestEqual(TEXT("zero readiness stays zero"),S[0].Power,0.f);
    TestEqual(TEXT("negative ammo clamped"),S[0].Ammo,0);
    TestFalse(TEXT("only one active ship"),S[1].bActive);
    HUD->SetDataSource(nullptr);
    TestFalse(TEXT("missing source remains unavailable"),HUD->GetSnapshot()[0].bAvailable);
    HUD->SetPreviewMode(true); HUD->PreviewSelectShip(2); HUD->PreviewDamage(); HUD->PreviewPower();
    TestTrue(TEXT("mock switch selects Bastion"),HUD->GetSnapshot()[2].bActive);
    TestEqual(TEXT("mock damage changes hull"),HUD->GetSnapshot()[2].Hull,.22f);
    TestEqual(TEXT("mock power starts cooling down"),HUD->GetSnapshot()[2].Power,0.f);
    HUD->SetPreviewMode(false);
    TestFalse(TEXT("leaving preview never leaks mock telemetry"),HUD->GetSnapshot()[2].bAvailable);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHUDLancerIsolationTest,"TriadDrift.UI.LancerBenchmarkIsolation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDHUDLancerIsolationTest::RunTest(const FString& Parameters)
{
    UTDSpaceHUDWidget* HUD=NewObject<UTDSpaceHUDWidget>();
    HUD->SetPreviewMode(true);
    TestTrue(TEXT("approved M03 is the default for controlled Lancer"),HUD->IsLancerBenchmarkActive());
    HUD->Style.bLancerVisualBenchmark=false;
    TestFalse(TEXT("explicit opt-out restores the M02 baseline"),HUD->IsLancerBenchmarkActive());
    HUD->Style.bLancerVisualBenchmark=true;
    TestTrue(TEXT("explicit opt-in applies to controlled Lancer"),HUD->IsLancerBenchmarkActive());
    HUD->PreviewSelectShip(0);
    TestFalse(TEXT("Rift keeps approved baseline"),HUD->IsLancerBenchmarkActive());
    HUD->PreviewSelectShip(2);
    TestFalse(TEXT("Bastion keeps approved baseline"),HUD->IsLancerBenchmarkActive());
    HUD->PreviewSelectShip(1);
    TestTrue(TEXT("switching back restores Lancer benchmark"),HUD->IsLancerBenchmarkActive());
    HUD->SetPreviewMode(false);
    TestFalse(TEXT("unavailable telemetry never leaves the benchmark active"),HUD->IsLancerBenchmarkActive());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHUDObjectiveTextTest,"TriadDrift.UI.ObjectiveLateAttach",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDHUDObjectiveTextTest::RunTest(const FString& Parameters)
{
    TDHUDTest::FObjectiveWorld F;
    if(!TestNotNull(TEXT("transient gameplay world"),F.World)) return false;
    UClass* SquadClass=FindObject<UClass>(nullptr,TEXT("/Script/unrealGameJam.TDSquadManager"));
    UClass* BusClass=FindObject<UClass>(nullptr,TEXT("/Script/unrealGameJam.TDEventBus"));
    if(!TestNotNull(TEXT("actual squad class"),SquadClass) || !TestNotNull(TEXT("actual event bus class"),BusClass)) return false;
    AActor* Squad=F.World->SpawnActor<AActor>(SquadClass,FVector::ZeroVector,FRotator::ZeroRotator);
    UObject* Bus=F.Instance->GetSubsystemBase(BusClass);
    if(!TestNotNull(TEXT("actual squad"),Squad) || !TestNotNull(TEXT("actual event bus"),Bus)) return false;
    UFunction* Broadcast=Bus->FindFunction(TEXT("BroadcastObjectiveUpdated"));
    if(!TestNotNull(TEXT("public objective broadcast"),Broadcast)) return false;
    const FNameProperty* Id=FindFProperty<FNameProperty>(Broadcast,TEXT("ObjectiveId"));
    const FIntProperty* Step=FindFProperty<FIntProperty>(Broadcast,TEXT("Step"));
    const FTextProperty* Text=FindFProperty<FTextProperty>(Broadcast,TEXT("Text"));
    if(!TestNotNull(TEXT("objective id input"),Id) || !TestNotNull(TEXT("step input"),Step) || !TestNotNull(TEXT("text input"),Text)) return false;
    const auto Send=[&](const FText& Value)
    {
        FStructOnScope Args(Broadcast);
        Id->SetPropertyValue_InContainer(Args.GetStructMemory(),FName(TEXT("HUDObjectiveTest")));
        Step->SetPropertyValue_InContainer(Args.GetStructMemory(),1);
        Text->SetPropertyValue_InContainer(Args.GetStructMemory(),Value);
        Bus->ProcessEvent(Broadcast,Args.GetStructMemory());
    };

    // The actual producer receives this before a widget exists: an event-only HUD misses it.
    Send(FText::FromString(TEXT("Clear the ambush")));
    UTDSpaceHUDWidget* HUD=CreateWidget<UTDSpaceHUDWidget>(F.Instance);
    if(!TestNotNull(TEXT("late-created HUD"),HUD)) return false;
    HUD->SetDataSource(Squad);
    TestTrue(TEXT("uses the real gameplay contract"),HUD->HasValidSource());
    TestEqual(TEXT("late attachment immediately reads current mission"),HUD->ObjectivePrompt.ToString(),FString(TEXT("Clear the ambush")));
    HUD->BindEvents(true);
    Send(FText::FromString(TEXT("Fly to Verdance")));
    TestEqual(TEXT("existing event binding still updates immediately"),HUD->ObjectivePrompt.ToString(),FString(TEXT("Fly to Verdance")));
    HUD->BindEvents(false);
    Send(FText::GetEmpty());
    HUD->RefreshData();
    TestTrue(TEXT("valid empty getter clears a stale mission without an event"),HUD->ObjectivePrompt.IsEmpty());
    Send(FText::FromString(TEXT("Land on Cinder")));
    HUD->RefreshData();
    TestEqual(TEXT("continuous sampling follows authoritative changes"),HUD->ObjectivePrompt.ToString(),FString(TEXT("Land on Cinder")));
    UTDSpaceHUDWidget* Recreated=CreateWidget<UTDSpaceHUDWidget>(F.Instance);
    if(!TestNotNull(TEXT("recreated HUD"),Recreated)) return false;
    Recreated->SetDataSource(Squad);
    TestEqual(TEXT("recreation reads the latest mission"),Recreated->ObjectivePrompt.ToString(),FString(TEXT("Land on Cinder")));

    HUD->SetDataSource(nullptr);
    TestTrue(TEXT("detachment clears previous provider text"),HUD->ObjectivePrompt.IsEmpty());
    HUD->BindEvents(true);
    Send(FText::FromString(TEXT("Event fallback")));
    HUD->RefreshData();
    TestEqual(TEXT("without a getter the event fallback survives sampling"),HUD->ObjectivePrompt.ToString(),FString(TEXT("Event fallback")));
    HUD->BindEvents(false);
    HUD->SetPreviewMode(true); HUD->RefreshData();
    TestEqual(TEXT("preview fixture remains isolated"),HUD->ObjectivePrompt.ToString(),FString(TEXT("INTERCEPT THE NULL ARMADA")));

    FText Unchanged=FText::FromString(TEXT("Preserved fallback"));
    TestFalse(TEXT("absent getter is optional"),TDHUD::Text(HUD,TEXT("GetObjectiveText"),Unchanged));
    TestFalse(TEXT("null source is safe"),TDHUD::Text(nullptr,TEXT("GetObjectiveText"),Unchanged));
    TestNotNull(TEXT("wrong-return fixture is reflected"),HUD->FindFunction(TEXT("GetIsEnabled")));
    TestFalse(TEXT("wrong return type is refused"),TDHUD::Text(HUD,TEXT("GetIsEnabled"),Unchanged));
    UObject* TextLibrary=UKismetTextLibrary::StaticClass()->GetDefaultObject();
    TestNotNull(TEXT("text-with-input fixture is reflected"),TextLibrary->FindFunction(TEXT("Conv_StringToText")));
    TestFalse(TEXT("FText return with input parameters is refused"),TDHUD::Text(TextLibrary,TEXT("Conv_StringToText"),Unchanged));
    TestEqual(TEXT("failed reads never overwrite event text"),Unchanged.ToString(),FString(TEXT("Preserved fallback")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHUDInteractionTest,"TriadDrift.UI.InteractionPromptLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FTDHUDInteractionTest::RunTest(const FString& Parameters)
{
    TDHUDTest::FObjectiveWorld F;
    if(!TestNotNull(TEXT("transient gameplay world"),F.World)) return false;
    UClass* SquadClass=FindObject<UClass>(nullptr,TEXT("/Script/unrealGameJam.TDSquadManager"));
    UClass* PadClass=FindObject<UClass>(nullptr,TEXT("/Script/unrealGameJam.TDLandingPad"));
    if(!TestNotNull(TEXT("actual squad class"),SquadClass) || !TestNotNull(TEXT("actual landing pad class"),PadClass)) return false;
    AActor* Pad=F.World->SpawnActor<AActor>(PadClass,FVector::ZeroVector,FRotator::ZeroRotator);
    AActor* Squad=F.World->SpawnActor<AActor>(SquadClass,FVector::ZeroVector,FRotator::ZeroRotator);
    APlayerController* PC=F.World->SpawnActor<APlayerController>();
    if(!TestNotNull(TEXT("real pad"),Pad) || !TestNotNull(TEXT("real squad"),Squad) || !TestNotNull(TEXT("controller"),PC)) return false;

    // Public gameplay setup only; never write a ship's private state or duplicate its landing rules.
    UFunction* SetController=Squad->FindFunction(TEXT("SetSquadPlayerController"));
    UFunction* Spawn=Squad->FindFunction(TEXT("SpawnSquad"));
    if(!TestNotNull(TEXT("public controller setter"),SetController) || !TestNotNull(TEXT("public squad spawn"),Spawn)) return false;
    const FObjectPropertyBase* ControllerArg=FindFProperty<FObjectPropertyBase>(SetController,TEXT("InController"));
    const FStructProperty* OriginArg=FindFProperty<FStructProperty>(Spawn,TEXT("Origin"));
    if(!TestNotNull(TEXT("controller input"),ControllerArg) || !TestNotNull(TEXT("spawn transform input"),OriginArg)) return false;
    if(!TestTrue(TEXT("spawn input is a transform"),OriginArg->Struct==TBaseStructure<FTransform>::Get())) return false;
    FStructOnScope ControllerParams(SetController);
    ControllerArg->SetObjectPropertyValue_InContainer(ControllerParams.GetStructMemory(),PC);
    Squad->ProcessEvent(SetController,ControllerParams.GetStructMemory());
    FStructOnScope SpawnParams(Spawn);
    *OriginArg->ContainerPtrToValuePtr<FTransform>(SpawnParams.GetStructMemory())=FTransform(FVector(-300000,0,0));
    Squad->ProcessEvent(Spawn,SpawnParams.GetStructMemory());
    for(int32 I=0;I<32;++I) { ++GFrameCounter; F.World->Tick(LEVELTICK_All,1.f/60.f); }
    AActor* FirstShip=PC->GetPawn();
    UTDSpaceHUDWidget* HUD=CreateWidget<UTDSpaceHUDWidget>(F.Instance);
    if(!TestNotNull(TEXT("controlled ship"),FirstShip) || !TestNotNull(TEXT("HUD"),HUD)) return false;
    HUD->SetDataSource(Squad);
    TestTrue(TEXT("far from pad has no guidance"),HUD->InteractPrompt.IsEmpty());
    HUD->ShowWarning(FText::FromString(TEXT("Combat warning")));
    FirstShip->SetActorLocationAndRotation(FVector(-1500,0,1500),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
    HUD->RefreshData();
    TestEqual(TEXT("real pad guidance reaches the native HUD"),HUD->InteractPrompt.ToString(),FString(TEXT("Hold F to land")));
    TestTrue(TEXT("landing guidance does not replace mission text"),HUD->ObjectivePrompt.IsEmpty());
    TestEqual(TEXT("landing guidance does not replace a combat warning"),HUD->Warning.ToString(),FString(TEXT("Combat warning")));
    TestTrue(TEXT("combat warning duration remains active"),HUD->WarningLeft>0);

    UFunction* Switch=Squad->FindFunction(TEXT("RequestSwitchTo"));
    if(!TestNotNull(TEXT("public ship switch"),Switch)) return false;
    const FEnumProperty* ShipArg=FindFProperty<FEnumProperty>(Switch,TEXT("Ship"));
    const FBoolProperty* Accepted=CastField<FBoolProperty>(Switch->GetReturnProperty());
    if(!TestNotNull(TEXT("ship id input"),ShipArg) || !TestNotNull(TEXT("switch result"),Accepted)) return false;
    FStructOnScope SwitchParams(Switch);
    ShipArg->GetUnderlyingProperty()->SetIntPropertyValue(ShipArg->ContainerPtrToValuePtr<void>(SwitchParams.GetStructMemory()),int64(2));
    Squad->ProcessEvent(Switch,SwitchParams.GetStructMemory());
    if(!TestTrue(TEXT("Bastion switch accepted"),Accepted->GetPropertyValue_InContainer(SwitchParams.GetStructMemory()))) return false;
    HUD->RefreshData();
    TestTrue(TEXT("switching to a distant ship removes the previous ship's hint"),HUD->InteractPrompt.IsEmpty());
    AActor* SecondShip=PC->GetPawn();
    if(!TestNotNull(TEXT("new controlled ship"),SecondShip)) return false;
    TestTrue(TEXT("control actually changed"),SecondShip!=FirstShip);
    SecondShip->SetActorLocationAndRotation(FVector(-1500,0,1500),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
    HUD->RefreshData();
    TestEqual(TEXT("new controlled ship gets its own real guidance"),HUD->InteractPrompt.ToString(),FString(TEXT("Hold F to land")));
    SecondShip->SetActorLocation(FVector(-300000,0,0));
    HUD->RefreshData();
    TestTrue(TEXT("leaving pad range clears guidance"),HUD->InteractPrompt.IsEmpty());
    SecondShip->SetActorLocation(FVector(-1500,0,1500)); HUD->RefreshData();
    TestFalse(TEXT("guidance restored before detachment"),HUD->InteractPrompt.IsEmpty());
    HUD->SetDataSource(NewObject<UTDSpaceHUDWidget>());
    TestTrue(TEXT("provider change to an incompatible object clears guidance"),HUD->InteractPrompt.IsEmpty());
    HUD->SetDataSource(Squad);
    TestFalse(TEXT("real provider reattachment restores current guidance"),HUD->InteractPrompt.IsEmpty());
    HUD->SetDataSource(nullptr);
    TestTrue(TEXT("source removal clears guidance immediately"),HUD->InteractPrompt.IsEmpty());
    HUD->InteractPrompt=FText::FromString(TEXT("stale")); HUD->RefreshData();
    TestTrue(TEXT("unavailable optional getter cannot preserve stale guidance"),HUD->InteractPrompt.IsEmpty());
    HUD->InteractPrompt=FText::FromString(TEXT("stale")); HUD->SetPreviewMode(true);
    TestTrue(TEXT("preview never leaks landing guidance"),HUD->InteractPrompt.IsEmpty());
    return true;
}
#endif
