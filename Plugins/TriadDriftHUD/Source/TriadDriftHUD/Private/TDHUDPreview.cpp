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
    if(FParse::Param(FCommandLine::Get(),TEXT("TDHUDReadabilityCapture")))
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
