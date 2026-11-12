#include "Misc/AutomationTest.h"
#include "TDSpaceHUDWidget.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
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
#endif
