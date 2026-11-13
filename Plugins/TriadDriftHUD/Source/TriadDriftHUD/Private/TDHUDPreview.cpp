#include "TDHUDPreview.h"
#include "TDSpaceHUDWidget.h"
#include "InputCoreTypes.h"
#include "Engine/World.h"
#include "GameFramework/InputSettings.h"
#include "Engine/GameViewportClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "TimerManager.h"
#include "InputKeyEventArgs.h"

ATDHUDPreviewGameMode::ATDHUDPreviewGameMode()
{
    PlayerControllerClass=ATDHUDPreviewController::StaticClass();
    DefaultPawnClass=nullptr;
}
void ATDHUDPreviewController::BeginPlay()
{
    Super::BeginPlay();
    UClass* WidgetClass=LoadClass<UTDSpaceHUDWidget>(nullptr,TEXT("/Game/TriadDrift/UI/TriadHUD/WBP_SpaceHUD.WBP_SpaceHUD_C"));
    if(!WidgetClass) WidgetClass=UTDSpaceHUDWidget::StaticClass();
    UE_LOG(LogTemp,Display,TEXT("TDHUD_PREVIEW_CLASS %s"),*WidgetClass->GetPathName());
    Widget=CreateWidget<UTDSpaceHUDWidget>(this,WidgetClass);
    Widget->AddToViewport(); Widget->SetPreviewMode(true);
    SetInputMode(FInputModeGameOnly());
    bShowMouseCursor=false;
    Widget->Style.bLancerVisualBenchmark=FParse::Param(FCommandLine::Get(),TEXT("TDHUDLancerBenchmark"));
    if(FParse::Param(FCommandLine::Get(),TEXT("TDHUDLancerCapture")))
    {
        ConsoleCommand(TEXT("t.MaxFPS 60"));
        const FString Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HUDLancerM03"));
        IFileManager::Get().MakeDirectory(*Directory,true);
        auto At=[this](float Seconds,TFunction<void()> Action)
        { FTimerHandle Handle; GetWorldTimerManager().SetTimer(Handle,FTimerDelegate::CreateWeakLambda(this,[Action=MoveTemp(Action)](){Action();}),Seconds,false); };
        auto Fixture=[this](bool Refined)
        {
            Widget->Style.bLancerVisualBenchmark=Refined; Widget->Style.bReducedMotion=false;
            Widget->SetPreviewMode(true);
            auto Snapshot=Widget->GetSnapshot(); for(auto& Ship:Snapshot) Ship.Power=1;
            Widget->ApplySquadSnapshot(Snapshot); Widget->bGamepadGlyphs=false;
        };
        auto Shot=[this,Directory](const TCHAR* Name)
        { FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,Name),true,false); };
        auto Press=[this](const FKey& Key)
        {
            const bool Handled=InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Pressed,1.f));
            const auto& Squad=Widget->GetSnapshot();
            int32 Active=INDEX_NONE; for(int32 I=0;I<Squad.Num();++I) if(Squad[I].bActive) Active=I;
            bool Passed=Handled;
            if(Key==EKeys::One) Passed&=Active==0 && !Widget->IsLancerBenchmarkActive();
            if(Key==EKeys::Two) Passed&=Active==1 && Widget->IsLancerBenchmarkActive();
            if(Key==EKeys::Three) Passed&=Active==2 && !Widget->IsLancerBenchmarkActive();
            if(Key==EKeys::Q || Key==EKeys::Gamepad_FaceButton_Left) Passed&=Squad.IsValidIndex(Active) && Squad[Active].Power==0;
            if(Key==EKeys::H) Passed&=Squad.IsValidIndex(Active) && Squad[Active].Hull==.22f && Squad[Active].Shield==.12f;
            if(Key==EKeys::F6) Passed&=Widget->Style.bReducedMotion;
            if(Passed) { UE_LOG(LogTemp,Display,TEXT("TDHUD_LANCER_INPUT PASS %s active=%d refined=%d"),*Key.ToString(),Active,Widget->IsLancerBenchmarkActive()); }
            else { UE_LOG(LogTemp,Error,TEXT("TDHUD_LANCER_INPUT FAILED %s active=%d"),*Key.ToString(),Active); }
        };
        At(1,[Fixture](){Fixture(false);});
        At(2,[this](){Widget->ResetBenchmarkPaintMetrics();});
        At(4,[this,Shot](){UE_LOG(LogTemp,Display,TEXT("TDHUD_LANCER_PAINT BEFORE %s"),*Widget->GetBenchmarkPaintMetrics());Shot(TEXT("01-before-lancer-1080p.png"));});
        At(5,[Fixture](){Fixture(true);});
        At(6,[this](){Widget->ResetBenchmarkPaintMetrics();});
        At(8,[this,Shot](){UE_LOG(LogTemp,Display,TEXT("TDHUD_LANCER_PAINT AFTER %s"),*Widget->GetBenchmarkPaintMetrics());Shot(TEXT("02-after-lancer-1080p.png"));});
        At(9,[Press](){Press(EKeys::One);});
        At(10,[Shot](){Shot(TEXT("03-rift-unchanged-1080p.png"));});
        At(11,[Press](){Press(EKeys::Three);});
        At(12,[Shot](){Shot(TEXT("04-bastion-unchanged-1080p.png"));});
        At(13,[Press](){Press(EKeys::Two); Press(EKeys::Q); Press(EKeys::H); Press(EKeys::K);});
        At(13.12f,[Shot](){Shot(TEXT("05-lancer-activation-damage-1080p.png"));});
        At(13.4f,[this](){ConsoleCommand(TEXT("r.SetRes 1280x720w"));});
        At(14.4f,[this](){Widget->ConfirmHit(true);});
        At(14.6f,[Shot](){Shot(TEXT("06-lancer-cooldown-720p.png"));});
        At(15.5f,[Press](){Press(EKeys::F6);});
        At(16,[this](){ConsoleCommand(TEXT("r.SetRes 2560x1080w"));});
        At(17,[Shot](){Shot(TEXT("07-lancer-reduced-motion-ultrawide.png"));});
        At(21,[this,Shot](){const auto& S=Widget->GetSnapshot(); if(S.IsValidIndex(1) && S[1].Power>=.999f) {UE_LOG(LogTemp,Display,TEXT("TDHUD_LANCER_READY PASS"));} else {UE_LOG(LogTemp,Error,TEXT("TDHUD_LANCER_READY FAILED"));} Shot(TEXT("08-lancer-ready-ultrawide.png"));});
        At(22,[Press](){Press(EKeys::Gamepad_FaceButton_Left);});
        At(23,[Shot](){Shot(TEXT("09-lancer-gamepad-cooldown-ultrawide.png"));});
        At(24,[this](){Widget->SetPreviewMode(false);});
        At(25,[Shot](){Shot(TEXT("10-telemetry-unavailable.png"));});
        At(26,[this](){ConsoleCommand(TEXT("quit"));});
    }
    else if(FParse::Param(FCommandLine::Get(),TEXT("TDHUDReadabilityCapture")))
    {
        const FString Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HUDReadabilityM02"));
        IFileManager::Get().MakeDirectory(*Directory,true);
        auto At=[this](float Seconds,TFunction<void()> Action)
        { FTimerHandle Handle; GetWorldTimerManager().SetTimer(Handle,FTimerDelegate::CreateWeakLambda(this,[Action=MoveTemp(Action)](){Action();}),Seconds,false); };
        auto Select=[this](int32 Index)
        {
            Widget->SetPreviewMode(true); Widget->PreviewSelectShip(Index);
            auto Snapshot=Widget->GetSnapshot(); Snapshot[Index].Power=1.f;
            Widget->ApplySquadSnapshot(Snapshot); Widget->bGamepadGlyphs=false;
        };
        auto Trigger=[this](const FKey& Key,int32 Index)
        {
            const FInputKeyEventArgs Event=FInputKeyEventArgs::CreateSimulated(Key,IE_Pressed,1.f);
            const bool Handled=InputKey(Event);
            const bool Passed=Handled && Widget->GetSnapshot().IsValidIndex(Index) && Widget->GetSnapshot()[Index].Power==0.f;
            if(Passed) { UE_LOG(LogTemp,Display,TEXT("TDHUD_READABILITY_INPUT PASS %s -> power cooldown"),*Key.ToString()); }
            else { UE_LOG(LogTemp,Error,TEXT("TDHUD_READABILITY_INPUT FAILED %s"),*Key.ToString()); }
        };
        At(2,[Select](){Select(1);});
        At(3,[this,Directory](){Widget->bGamepadGlyphs=false;FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,TEXT("01-lancer-ready-1080p.png")),true,false);});
        At(4,[Select](){Select(0);});
        At(5,[this,Directory](){Widget->bGamepadGlyphs=false;FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,TEXT("02-rift-ready-1080p.png")),true,false);});
        At(6,[Select](){Select(2);});
        At(7,[this,Directory](){Widget->bGamepadGlyphs=true;FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,TEXT("03-bastion-ready-pad-1080p.png")),true,false);});
        At(8,[Trigger](){Trigger(EKeys::Gamepad_FaceButton_Left,2);});
        At(8.1f,[this](){Widget->ConfirmHit(true);const FGeometry& Geometry=Widget->GetCachedGeometry();UE_LOG(LogTemp,Display,TEXT("TDHUD_READABILITY_DPI 1080p %.4f local %.0fx%.0f"),Geometry.GetAccumulatedLayoutTransform().GetScale(),Geometry.GetLocalSize().X,Geometry.GetLocalSize().Y);});
        At(8.2f,[this,Directory](){Widget->bGamepadGlyphs=false;FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,TEXT("04-bastion-cooldown-1080p.png")),true,false);});
        At(9,[this,Select,Trigger](){Select(0);Widget->PreviewDamage();Trigger(EKeys::Q,0);Widget->Style.bReducedMotion=true;ConsoleCommand(TEXT("r.SetRes 1280x720w"));});
        At(9.8f,[this](){Widget->ConfirmHit(true);const FGeometry& Geometry=Widget->GetCachedGeometry();UE_LOG(LogTemp,Display,TEXT("TDHUD_READABILITY_DPI 720p %.4f local %.0fx%.0f"),Geometry.GetAccumulatedLayoutTransform().GetScale(),Geometry.GetLocalSize().X,Geometry.GetLocalSize().Y);});
        At(10,[this,Directory](){Widget->bGamepadGlyphs=false;FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,TEXT("05-rift-low-health-720p.png")),true,false);});
        At(11,[this](){ConsoleCommand(TEXT("r.SetRes 2560x1080w"));});
        At(12,[this,Directory](){Widget->bGamepadGlyphs=false;FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,TEXT("06-rift-cooldown-ultrawide.png")),true,false);});
        At(13,[this](){Widget->SetPreviewMode(false);});
        At(14,[this,Directory](){Widget->bGamepadGlyphs=false;FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,TEXT("07-unavailable.png")),true,false);});
        At(15,[this](){ConsoleCommand(TEXT("quit"));});
    }
    // Opt-in automated visual QA. One hidden game process, no editor GUI or shared map edits.
    else if(FParse::Param(FCommandLine::Get(),TEXT("TDHUDCapture")))
    {
        const FString Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HUDReview"));
        IFileManager::Get().MakeDirectory(*Directory,true);
        auto At=[this](float Seconds,TFunction<void()> Action)
        { FTimerHandle Handle; GetWorldTimerManager().SetTimer(Handle,FTimerDelegate::CreateWeakLambda(this,[Action=MoveTemp(Action)](){Action();}),Seconds,false); };
        At(3,[this,Directory](){FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,TEXT("01-squad.png")),true,false);});
        At(4,[this](){Widget->PreviewSelectShip(0);Widget->PreviewDamage();Widget->PreviewPower();Widget->ConfirmHit(true);});
        At(4.15f,[Directory](){FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,TEXT("02-warning.png")),true,false);});
        At(5,[this](){Widget->PreviewDowned();Widget->Style.bReducedMotion=true;ConsoleCommand(TEXT("r.SetRes 1280x720w"));});
        At(6,[Directory](){FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,TEXT("03-720p-respawn.png")),true,false);});
        At(7,[this](){Widget->SetPreviewMode(true);ConsoleCommand(TEXT("r.SetRes 2560x1080w"));});
        At(8,[Directory](){FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,TEXT("04-ultrawide.png")),true,false);});
        At(9,[this](){Widget->SetPreviewMode(false);});
        At(10,[Directory](){FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,TEXT("05-unavailable.png")),true,false);});
        At(11,[this](){ConsoleCommand(TEXT("quit"));});
    }
}
bool ATDHUDPreviewController::InputKey(const FInputKeyEventArgs& Params)
{
    if(Widget)
    {
        Widget->bGamepadGlyphs=Params.Key.IsGamepadKey();
        if(Params.Event==IE_Pressed)
        {
            if(Params.Key==EKeys::One) Widget->PreviewSelectShip(0);
            else if(Params.Key==EKeys::Two) Widget->PreviewSelectShip(1);
            else if(Params.Key==EKeys::Three) Widget->PreviewSelectShip(2);
            else if(Params.Key==EKeys::H || Params.Key==EKeys::Gamepad_FaceButton_Right) Widget->PreviewDamage();
            else if(Params.Key==EKeys::K || Params.Key==EKeys::Gamepad_FaceButton_Top) Widget->ConfirmHit(true);
            else if(Params.Key==EKeys::Q || Params.Key==EKeys::P || Params.Key==EKeys::Gamepad_FaceButton_Left || Params.Key==EKeys::Gamepad_FaceButton_Bottom) Widget->PreviewPower();
            else if(Params.Key==EKeys::R) Widget->PreviewDowned();
            else if(Params.Key==EKeys::F5) Widget->SetPreviewMode(true);
            else if(Params.Key==EKeys::F6) Widget->Style.bReducedMotion=!Widget->Style.bReducedMotion;
            else if(Params.Key==EKeys::Gamepad_LeftShoulder || Params.Key==EKeys::Gamepad_RightShoulder)
            {
                const auto& Ships=Widget->GetSnapshot(); int32 Active=0;
                for(int32 I=0;I<Ships.Num();++I) if(Ships[I].bActive) Active=I;
                const int32 Step=Params.Key==EKeys::Gamepad_RightShoulder?1:2;
                Widget->PreviewSelectShip((Active+Step)%3);
            }
            else return Super::InputKey(Params);
            return true;
        }
    }
    return Super::InputKey(Params);
}
