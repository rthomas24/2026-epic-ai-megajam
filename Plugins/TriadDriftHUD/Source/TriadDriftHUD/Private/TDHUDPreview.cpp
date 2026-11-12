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
    // Opt-in automated visual QA. One hidden game process, no editor GUI or shared map edits.
    if(FParse::Param(FCommandLine::Get(),TEXT("TDHUDCapture")))
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
            else if(Params.Key==EKeys::H || Params.Key==EKeys::Gamepad_FaceButton_Left) Widget->PreviewDamage();
            else if(Params.Key==EKeys::K || Params.Key==EKeys::Gamepad_FaceButton_Top) Widget->ConfirmHit(true);
            else if(Params.Key==EKeys::P || Params.Key==EKeys::Gamepad_FaceButton_Bottom) Widget->PreviewPower();
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
