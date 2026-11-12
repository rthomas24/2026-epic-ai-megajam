#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "TDAudioV1.generated.h"
class UAudioComponent; class USoundBase; class USoundClass; class USoundMix;
class USubmixEffectDynamicsProcessorPreset;
UCLASS()
class TDAUDIOV1_API ATDAudioV1GameMode : public AGameModeBase {
 GENERATED_BODY()
 public:
 ATDAudioV1GameMode();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 void Trigger(const FString& Id);
 void SelectMusic(int32 State);
 void SetBus(int32 Bus,float Value);
 void ToggleEngine();
 FString Status() const;
 float Throttle=0.f, SFXVolume=0.8f, UIVolume=0.8f, MusicVolume=0.6f;
 float DistanceCm=1000.f;
 bool bSpatial=false, bEngine=false, bRapid=false;
 FString LastEvent=TEXT("Ready");
 private:
 UPROPERTY() TMap<FString,TObjectPtr<USoundBase>> Sounds;
 UPROPERTY() TArray<TObjectPtr<UAudioComponent>> Voices;
 UPROPERTY() TObjectPtr<UAudioComponent> Engine;
 UPROPERTY() TArray<TObjectPtr<UAudioComponent>> Music;
 UPROPERTY() TArray<TObjectPtr<USoundClass>> Classes;
 UPROPERTY() TObjectPtr<USoundMix> SettingsMix;
 UPROPERTY() TObjectPtr<USoundClass> CineClass;
 UPROPERTY() TObjectPtr<USubmixEffectDynamicsProcessorPreset> Limiter;
 float MusicAngle=0.f, MusicTargetAngle=0.f;
 int32 MusicState=-1, SelfStep=0, PeakVoices=0;
 double StartSeconds=0, RapidAt=0;
 bool bSelfTest=false,bRecording=false,bSawMusicOverlap=false,bSawEngine=false;
 double TickSeconds=0;
 int32 TickSamples=0;
 float WorstTick=0;
 TArray<FString> Missing;
 void UpdateMusic(float DeltaSeconds);
 void WriteSelfTest();
 int32 ActiveVoices() const;
};
UCLASS()
class TDAUDIOV1_API ATDAudioV1HUD : public AHUD {
 GENERATED_BODY()
 public: virtual void DrawHUD() override;
};

