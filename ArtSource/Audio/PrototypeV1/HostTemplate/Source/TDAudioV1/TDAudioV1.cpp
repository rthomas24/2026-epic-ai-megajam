#include "TDAudioV1.h"
#include "Modules/ModuleManager.h"
#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "AudioMixerBlueprintLibrary.h"
#include "SubmixEffects/AudioMixerSubmixEffectDynamicsProcessor.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "InputCoreTypes.h"
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl,TDAudioV1,"TDAudioV1");
static const FString Root=TEXT("/Game/TriadDrift/Audio/PrototypeV1/");
template<class T> static T* LoadV1(const FString& Path) {
 return LoadObject<T>(nullptr,*(Path+TEXT(".")+FPaths::GetBaseFilename(Path)));
}
ATDAudioV1GameMode::ATDAudioV1GameMode() {
 PrimaryActorTick.bCanEverTick=true; DefaultPawnClass=nullptr; HUDClass=ATDAudioV1HUD::StaticClass();
}
void ATDAudioV1GameMode::BeginPlay() {
 Super::BeginPlay(); StartSeconds=FPlatformTime::Seconds();
 bSelfTest=FParse::Param(FCommandLine::Get(),TEXT("AudioV1SelfTest"));
 const TCHAR* Ids[]={TEXT("Engine"),TEXT("Acceleration"),TEXT("Primary"),TEXT("Missile"),TEXT("Explosion"),TEXT("Shield"),TEXT("Hull"),TEXT("Rift"),TEXT("Lancer"),TEXT("Bastion"),TEXT("Target"),TEXT("Ability"),TEXT("Explore"),TEXT("Combat")};
 for(const TCHAR* Id:Ids) {
  USoundBase* S=LoadV1<USoundBase>(Root+TEXT("Sources/MSS_V1_")+Id);
  if(S) Sounds.Add(Id,S); else Missing.Add(Id);
 }
 for(const TCHAR* Bus:{TEXT("SFX"),TEXT("UI"),TEXT("Music")}) Classes.Add(LoadV1<USoundClass>(Root+TEXT("Mix/SC_V1_")+Bus));
 SettingsMix=LoadV1<USoundMix>(Root+TEXT("Mix/MIX_V1_Settings"));
 CineClass=LoadV1<USoundClass>(Root+TEXT("Mix/SC_V1_Cine"));
 if(!SettingsMix || !CineClass || Classes.Contains(nullptr)) Missing.Add(TEXT("Mix/classes"));
 if(SettingsMix) UGameplayStatics::PushSoundMixModifier(this,SettingsMix);
 SetBus(0,SFXVolume);SetBus(1,UIVolume);SetBus(2,MusicVolume);
 Limiter=NewObject<USubmixEffectDynamicsProcessorPreset>(this);
 FSubmixEffectDynamicsProcessorSettings C;
 C.DynamicsProcessorType=ESubmixEffectDynamicsProcessorType::Limiter;
 C.ThresholdDb=-3.f;C.LookAheadMsec=2.f;C.AttackTimeMsec=1.f;C.ReleaseTimeMsec=100.f;C.KneeBandwidthDb=0.f;
 Limiter->SetSettings(C);UAudioMixerBlueprintLibrary::AddMasterSubmixEffect(this,Limiter);
 if(APlayerController* PC=UGameplayStatics::GetPlayerController(this,0)) {
  ACameraActor* Camera=GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector,FRotator::ZeroRotator);
  PC->SetViewTarget(Camera);PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;
 }
 Music.SetNum(2);
 UE_LOG(LogTemp,Display,TEXT("AUDIOV1 READY loaded=%d missing=%d"),Sounds.Num(),Missing.Num());
 if(bSelfTest){UAudioMixerBlueprintLibrary::StartRecordingOutput(this,24.f);bRecording=true;}
}
void ATDAudioV1GameMode::SetBus(int32 Bus,float Value) {
 Value=FMath::Clamp(Value,0.f,1.f);
 if(Bus==0) SFXVolume=Value; else if(Bus==1) UIVolume=Value;else MusicVolume=Value;
 if(SettingsMix && Classes.IsValidIndex(Bus) && Classes[Bus])
  UGameplayStatics::SetSoundMixClassOverride(this,SettingsMix,Classes[Bus],Value,1.f,0.15f,true);
 if(Bus==1 && SettingsMix && CineClass)
  UGameplayStatics::SetSoundMixClassOverride(this,SettingsMix,CineClass,Value,1.f,0.15f,true);
}
void ATDAudioV1GameMode::Trigger(const FString& Id) {
 USoundBase* S=Sounds.FindRef(Id); if(!S){Missing.AddUnique(Id);return;}
 bool World=Id==TEXT("Primary")||Id==TEXT("Missile")||Id==TEXT("Explosion")||Id==TEXT("Shield")||Id==TEXT("Hull");
 UAudioComponent* C=(World && bSpatial)?UGameplayStatics::SpawnSoundAtLocation(this,S,FVector(DistanceCm*.89442719f,DistanceCm*.4472136f,0.f)):UGameplayStatics::SpawnSound2D(this,S);
 if(C) Voices.Add(C);
 LastEvent=Id; UE_LOG(LogTemp,Display,TEXT("AUDIOV1 EVENT %s spatial=%d distance=%.0f"),*Id,bSpatial,DistanceCm);
}
void ATDAudioV1GameMode::ToggleEngine() {
 bEngine=!bEngine;
 if(!bEngine){if(IsValid(Engine))Engine->FadeOut(.3f,0.f);return;}
 if(USoundBase* S=Sounds.FindRef(TEXT("Engine"))) {
  Engine=UGameplayStatics::SpawnSound2D(this,S,1.f,1.f,0.f,nullptr,false,true);
  if(Engine){Engine->SetFloatParameter(TEXT("Speed01"),Throttle);Engine->FadeIn(.15f,1.f);}
 }
}
void ATDAudioV1GameMode::SelectMusic(int32 State) {
 if(State<0){for(UAudioComponent* C:Music)if(C)C->FadeOut(2.f,0.f);MusicState=-1;return;}
 State=FMath::Clamp(State,0,1);bool WasOff=MusicState<0;
 for(int32 I=0;I<2;++I) {
  if(!Music[I]||!Music[I]->IsPlaying()) {
   if(IsValid(Music[I]))Music[I]->DestroyComponent();
   USoundBase* S=Sounds.FindRef(I==0?TEXT("Explore"):TEXT("Combat"));
   Music[I]=UGameplayStatics::SpawnSound2D(this,S,.0001f,1.f,0.f,nullptr,false,false);
  }
 }
 MusicState=State; MusicTargetAngle=State*HALF_PI;
 if(WasOff) {MusicAngle=MusicTargetAngle;for(int32 I=0;I<2;++I) if(Music[I])Music[I]->SetVolumeMultiplier(I==State?0.001f:0.0001f);if(Music[State])Music[State]->FadeIn(2.f,1.f);}
 LastEvent=State?TEXT("Combat music"):TEXT("Exploration music");
}
void ATDAudioV1GameMode::UpdateMusic(float D) {
 if(MusicState<0)return;
 MusicAngle=FMath::FInterpConstantTo(MusicAngle,MusicTargetAngle,D,HALF_PI/2.f);
 float G[]={FMath::Cos(MusicAngle),FMath::Sin(MusicAngle)};
 for(int32 I=0;I<2;++I) if(Music[I])Music[I]->SetVolumeMultiplier(FMath::Max(G[I],0.0001f));
 bSawMusicOverlap|=MusicAngle>.05f&&MusicAngle<HALF_PI-.05f&&Music[0]&&Music[1]&&Music[0]->IsPlaying()&&Music[1]->IsPlaying();
}
int32 ATDAudioV1GameMode::ActiveVoices() const {int32 N=0;for(UAudioComponent* V:Voices)if(IsValid(V)&&V->IsPlaying())N++;return N;}
void ATDAudioV1GameMode::Tick(float D) {
 Super::Tick(D);double T=FPlatformTime::Seconds()-StartSeconds;
 if(T>2){TickSeconds+=D;TickSamples++;WorstTick=FMath::Max(WorstTick,D);}
 Voices.RemoveAll([](const UAudioComponent* C){return !IsValid(C)||!C->IsPlaying();});
 PeakVoices=FMath::Max(PeakVoices,ActiveVoices());UpdateMusic(D);
 if(IsValid(Engine)&&Engine->IsPlaying()){Engine->SetFloatParameter(TEXT("Speed01"),Throttle);bSawEngine=true;}
 if(bRapid&&T>=RapidAt){Trigger(TEXT("Primary"));RapidAt=T+.09;}
 if(bSelfTest) {
  const float At[]={.5f,1.f,2.f,3.f,4.f,4.5f,5.f,5.5f,6.f,6.4f,6.8f,7.2f,8.f,10.5f,13.f,15.5f,16.f,17.f,17.5f,19.f,20.f};
  if(SelfStep<UE_ARRAY_COUNT(At)&&T>=At[SelfStep]) {
   switch(SelfStep) {
    case 0:ToggleEngine();break;case 1:Throttle=1.f;Trigger(TEXT("Acceleration"));break;
    case 2:bRapid=true;break;case 3:bRapid=false;break;
    case 4:Trigger(TEXT("Missile"));break;case 5:Trigger(TEXT("Explosion"));break;
    case 6:Trigger(TEXT("Shield"));break;case 7:Trigger(TEXT("Hull"));break;
    case 8:Trigger(TEXT("Rift"));break;case 9:Trigger(TEXT("Lancer"));break;case 10:Trigger(TEXT("Bastion"));break;
    case 11:Trigger(TEXT("Target"));Trigger(TEXT("Ability"));break;
    case 12:SelectMusic(0);FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("AudioV1_HUD.png"),false,false);break;case 13:SelectMusic(1);break;case 14:SelectMusic(0);break;
    case 15:SelectMusic(-1);if(bEngine)ToggleEngine();break;
    case 16:Trigger(TEXT("Target"));break;
    case 17:SetBus(0,0);Trigger(TEXT("Primary"));break;
    case 18:SetBus(1,0);Trigger(TEXT("Target"));break;
    case 19:UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("AudioV1_RuntimeMix"),TEXT("PrototypeV1"));bRecording=false;break;
    case 20:WriteSelfTest();FPlatformMisc::RequestExit(false);break;
   } SelfStep++;
  }
  return;
 }
 APlayerController* PC=UGameplayStatics::GetPlayerController(this,0);if(!PC)return;
 const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,EKeys::Nine};
 const TCHAR* Names[]={TEXT("Primary"),TEXT("Missile"),TEXT("Explosion"),TEXT("Shield"),TEXT("Hull"),TEXT("Rift"),TEXT("Lancer"),TEXT("Bastion"),TEXT("Target")};
 for(int32 I=0;I<9;++I)if(PC->WasInputKeyJustPressed(Keys[I]))Trigger(Names[I]);
 if(PC->WasInputKeyJustPressed(EKeys::Zero))Trigger(TEXT("Ability"));
 if(PC->WasInputKeyJustPressed(EKeys::E))ToggleEngine();
 if(PC->WasInputKeyJustPressed(EKeys::B))Trigger(TEXT("Acceleration"));
 if(PC->WasInputKeyJustPressed(EKeys::R))bRapid=!bRapid;
 if(PC->WasInputKeyJustPressed(EKeys::P))bSpatial=!bSpatial;
 if(PC->WasInputKeyJustPressed(EKeys::F1))SelectMusic(0);
 if(PC->WasInputKeyJustPressed(EKeys::F2))SelectMusic(1);
 if(PC->WasInputKeyJustPressed(EKeys::F3))SelectMusic(-1);
 if(PC->IsInputKeyDown(EKeys::W))Throttle=FMath::Min(1.f,Throttle+D*.5f);
 if(PC->IsInputKeyDown(EKeys::S))Throttle=FMath::Max(0.f,Throttle-D*.5f);
 if(PC->WasInputKeyJustPressed(EKeys::PageUp))DistanceCm=FMath::Min(100000.f,DistanceCm*2);
 if(PC->WasInputKeyJustPressed(EKeys::PageDown))DistanceCm=FMath::Max(1000.f,DistanceCm*.5f);
 if(PC->IsInputKeyDown(EKeys::Z))SetBus(0,SFXVolume-D*.4f);
 if(PC->IsInputKeyDown(EKeys::X))SetBus(0,SFXVolume+D*.4f);
 if(PC->IsInputKeyDown(EKeys::C))SetBus(1,UIVolume-D*.4f);
 if(PC->IsInputKeyDown(EKeys::V))SetBus(1,UIVolume+D*.4f);
 if(PC->IsInputKeyDown(EKeys::N))SetBus(2,MusicVolume-D*.4f);
 if(PC->IsInputKeyDown(EKeys::M))SetBus(2,MusicVolume+D*.4f);
 if(PC->WasInputKeyJustPressed(EKeys::BackSpace)){bRapid=false;SelectMusic(-1);if(bEngine)ToggleEngine();for(UAudioComponent* C:Voices)if(C)C->FadeOut(.05f,0.f);}
}
FString ATDAudioV1GameMode::Status() const {
 return FString::Printf(TEXT("Engine %s | throttle %.0f%% | SFX %.0f%% | UI %.0f%% | Music %.0f%%\n%s | emitter %.0f m | active one-shots %d | last: %s"),
 bEngine?TEXT("ON"):TEXT("OFF"),Throttle*100,SFXVolume*100,UIVolume*100,MusicVolume*100,bSpatial?TEXT("3D positional"):TEXT("2D player perspective"),DistanceCm/100,ActiveVoices(),*LastEvent);
}
void ATDAudioV1GameMode::WriteSelfTest() {
 TSharedRef<FJsonObject> J=MakeShared<FJsonObject>();
 J->SetNumberField(TEXT("loaded_assets"),Sounds.Num());J->SetNumberField(TEXT("missing_assets"),Missing.Num());
 J->SetBoolField(TEXT("engine_playing_observed"),bSawEngine);J->SetBoolField(TEXT("music_crossfade_overlap_observed"),bSawMusicOverlap);
 J->SetNumberField(TEXT("peak_one_shot_components"),PeakVoices);
 J->SetNumberField(TEXT("average_frame_ms"),TickSamples?1000*TickSeconds/TickSamples:0);
 J->SetNumberField(TEXT("worst_frame_ms"),WorstTick*1000);
 J->SetBoolField(TEXT("technical_pass"),Missing.IsEmpty()&&bSawEngine&&bSawMusicOverlap);
 FString Out;auto Writer=TJsonWriterFactory<>::Create(&Out);FJsonSerializer::Serialize(J,Writer);
 FFileHelper::SaveStringToFile(Out,*(FPaths::ProjectSavedDir()/TEXT("AudioV1_SelfTest.json")));
 UE_LOG(LogTemp,Display,TEXT("AUDIOV1 SELFTEST %s"),*Out);
}
void ATDAudioV1GameMode::EndPlay(const EEndPlayReason::Type R) {
 for(UAudioComponent* C:Voices)if(IsValid(C))C->Stop();
 for(UAudioComponent* C:Music)if(IsValid(C))C->Stop();
 if(IsValid(Engine)){Engine->Stop();Engine->DestroyComponent();}
 for(UAudioComponent* C:Music)if(IsValid(C))C->DestroyComponent();
 if(SettingsMix)UGameplayStatics::PopSoundMixModifier(this,SettingsMix);
 if(Limiter)UAudioMixerBlueprintLibrary::RemoveMasterSubmixEffect(this,Limiter);
 Super::EndPlay(R);
}
void ATDAudioV1HUD::DrawHUD() {
 Super::DrawHUD();auto* GM=Cast<ATDAudioV1GameMode>(GetWorld()->GetAuthGameMode());if(!GM||!Canvas)return;
 DrawRect(FLinearColor(.018f,.035f,.065f,1),0,0,Canvas->SizeX,Canvas->SizeY);
 float X=40,Y=28;auto Line=[&](const FString& S,FLinearColor Col=FLinearColor(.8f,.88f,.95f)){DrawText(S,Col,X,Y,GEngine->GetMediumFont(),1.15f);Y+=31;};
 Line(TEXT("TRIAD DRIFT  |  AUDIO PROTOTYPE V1"),FLinearColor(.3f,.85f,1.f));
 Y+=12;Line(TEXT("1 Primary   2 Missile   3 Explosion   4 Shield impact   5 Hull impact"));
 Line(TEXT("6 Rift switch   7 Lancer switch   8 Bastion switch   9 Target   0 Ability"));
 Line(TEXT("E Engine on/off   W/S Throttle   B Acceleration   R Rapid-fire stress"));
 Line(TEXT("F1 Exploration   F2 Combat   F3 Fade music out"));
 Line(TEXT("Z/X SFX volume   C/V UI volume   N/M Music volume"));
 Line(TEXT("P 2D/3D shots   PageUp/PageDown Emitter distance   Backspace Stop"));
 Y+=18;Line(GM->Status(),FLinearColor(1.f,.8f,.45f));
 Y+=50;Line(TEXT("Existing licensed sources. Prototype substitutions remain marked in the mapping."));
 Line(TEXT("Listen for smooth fades, repeat fatigue, clear impacts and music balance."));
 Line(TEXT("Isolated demo only. Shared gameplay, maps and settings are unchanged."));
}
