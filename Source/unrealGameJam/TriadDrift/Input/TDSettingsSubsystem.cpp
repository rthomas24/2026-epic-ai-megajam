// Triad Drift - player options. Owner: Teammate 1 (Input).

#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Input/TDHintComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "AudioDevice.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectGlobals.h"

namespace TDSettings
{
	static const TCHAR* MusicClassPath = TEXT("/Game/TriadDrift/Audio/Mix/SC_TD_Music.SC_TD_Music");
	static const TCHAR* EffectsClassPath = TEXT("/Game/TriadDrift/Audio/Mix/SC_TD_SFX.SC_TD_SFX");

	static USoundClass* LoadClassIfPresent(const TCHAR* ObjectPath)
	{
		const FString Package = FPackageName::ObjectPathToPackageName(FString(ObjectPath));
		if (!FPackageName::DoesPackageExist(Package))
		{
			return nullptr;
		}
		return LoadObject<USoundClass>(nullptr, ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	static FTDPlayerSettings Clamped(FTDPlayerSettings S)
	{
		S.MouseSensitivity = FMath::Clamp(S.MouseSensitivity, 0.25f, 3.f);
		S.GamepadSensitivity = FMath::Clamp(S.GamepadSensitivity, 0.25f, 3.f);
		S.GamepadDeadzone = FMath::Clamp(S.GamepadDeadzone, 0.f, 0.5f);
		S.MasterVolume = FMath::Clamp(S.MasterVolume, 0.f, 1.f);
		S.MusicVolume = FMath::Clamp(S.MusicVolume, 0.f, 1.f);
		S.EffectsVolume = FMath::Clamp(S.EffectsVolume, 0.f, 1.f);
		S.Resolution.X = FMath::Max(0, S.Resolution.X);
		S.Resolution.Y = FMath::Max(0, S.Resolution.Y);
		return S;
	}
}

void UTDSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (GIsAutomationTesting)
	{
		SlotName = TEXT("TD_Settings_Test"); // tests never read or write the player's options
		bApplyVideo = false;
	}
	Load();
	ApplyVideo(true);
	ApplyAudio();
	// A level travel (menu -> galaxy, planet trips) never carries the page into the next map.
	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddWeakLambda(this, [this](const FString&) { ClosePageInternal(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr); });
}

void UTDSettingsSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
	ClosePageInternal(nullptr);
	Super::Deinitialize();
}

UTDSettingsSubsystem* UTDSettingsSubsystem::GetTDSettings(const UObject* WorldContextObject)
{
	const UWorld* World = (GEngine && WorldContextObject) ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UTDSettingsSubsystem>() : nullptr;
}

void UTDSettingsSubsystem::UseSlotForTests(const FString& InSlotName)
{
	SlotName = InSlotName;
	bApplyVideo = false;
	Settings = FTDPlayerSettings();
}

void UTDSettingsSubsystem::Load()
{
	Settings = FTDPlayerSettings();
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		return;
	}
	const UTDSettingsSave* SaveObj = Cast<UTDSettingsSave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (SaveObj && SaveObj->Version == UTDSettingsSave::CurrentVersion)
	{
		Settings = TDSettings::Clamped(SaveObj->Settings);
	}
}

void UTDSettingsSubsystem::Save() const
{
	if (UTDSettingsSave* SaveObj = Cast<UTDSettingsSave>(UGameplayStatics::CreateSaveGameObject(UTDSettingsSave::StaticClass())))
	{
		SaveObj->Settings = Settings;
		UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, 0);
	}
}

void UTDSettingsSubsystem::Changed(bool bVideo, bool bAudio)
{
	Settings = TDSettings::Clamped(Settings);
	if (bVideo) { ApplyVideo(); }
	if (bAudio) { ApplyAudio(); }
	ApplyControls();
	Save();
}

// ---- Setters

void UTDSettingsSubsystem::SetMouseSensitivity(float V) { Settings.MouseSensitivity = V; Changed(); }
void UTDSettingsSubsystem::SetInvertMouseY(bool b) { Settings.bInvertMouseY = b; Changed(); }
void UTDSettingsSubsystem::SetGamepadSensitivity(float V) { Settings.GamepadSensitivity = V; Changed(); }
void UTDSettingsSubsystem::SetGamepadDeadzone(float V) { Settings.GamepadDeadzone = V; Changed(); }
void UTDSettingsSubsystem::SetInvertGamepadY(bool b) { Settings.bInvertGamepadY = b; Changed(); }
void UTDSettingsSubsystem::SetHintsEnabled(bool b) { Settings.bHintsEnabled = b; Changed(); }
void UTDSettingsSubsystem::SetMasterVolume(float V) { Settings.MasterVolume = V; Changed(false, true); }
void UTDSettingsSubsystem::SetMusicVolume(float V) { Settings.MusicVolume = V; Changed(false, true); }
void UTDSettingsSubsystem::SetEffectsVolume(float V) { Settings.EffectsVolume = V; Changed(false, true); }
void UTDSettingsSubsystem::SetWindowMode(ETDWindowMode Mode) { Settings.WindowMode = Mode; Settings.bVideoChosen = true; Changed(true); }
void UTDSettingsSubsystem::SetResolution(FIntPoint Resolution) { Settings.Resolution = Resolution; Settings.bVideoChosen = true; Changed(true); }

void UTDSettingsSubsystem::ResetToDefaults()
{
	const ETDOnFootCamera Before = Settings.OnFootCameraMode;
	Settings = FTDPlayerSettings();
	Changed(true, true);
	BroadcastCameraIfChanged(Before);
}

void UTDSettingsSubsystem::ApplyAll(const FTDPlayerSettings& NewSettings)
{
	const ETDOnFootCamera Before = Settings.OnFootCameraMode;
	Settings = NewSettings;
	Changed(true, true);
	BroadcastCameraIfChanged(Before);
}

void UTDSettingsSubsystem::SetOnFootCameraMode(ETDOnFootCamera Mode)
{
	const ETDOnFootCamera Before = Settings.OnFootCameraMode;
	Settings.OnFootCameraMode = Mode;
	Changed();
	BroadcastCameraIfChanged(Before);
}

void UTDSettingsSubsystem::ToggleOnFootCameraMode()
{
	SetOnFootCameraMode(Settings.OnFootCameraMode == ETDOnFootCamera::FirstPerson ? ETDOnFootCamera::ThirdPerson : ETDOnFootCamera::FirstPerson);
}

void UTDSettingsSubsystem::BroadcastCameraIfChanged(ETDOnFootCamera Before)
{
	if (Settings.OnFootCameraMode != Before)
	{
		UE_LOG(LogTriadDrift, Log, TEXT("TD settings: on-foot camera %s"), *UEnum::GetValueAsString(Settings.OnFootCameraMode));
		OnOnFootCameraModeChanged.Broadcast(Settings.OnFootCameraMode);
	}
}

TArray<FIntPoint> UTDSettingsSubsystem::GetSupportedResolutions() const
{
	TArray<FIntPoint> Out;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Out);
	return Out;
}

// ---- Applying

void UTDSettingsSubsystem::ApplyControls()
{
	const UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	for (const ULocalPlayer* LP : GI->GetLocalPlayers())
	{
		if (ATDSpacePlayerController* PC = LP ? Cast<ATDSpacePlayerController>(LP->GetPlayerController(nullptr)) : nullptr)
		{
			ApplyToController(PC);
		}
	}
	// Test worlds and menus may hold a space controller without a local player entry.
	if (const UWorld* World = GI->GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(It->Get()))
			{
				ApplyToController(PC);
			}
		}
	}
}

void UTDSettingsSubsystem::ApplyToController(ATDSpacePlayerController* PC) const
{
	if (!PC)
	{
		return;
	}
	PC->ApplyPlayerSettings(Settings);
	if (UTDHintComponent* Hints = PC->GetHints())
	{
		Hints->SetHintsEnabled(Settings.bHintsEnabled);
	}
}

void UTDSettingsSubsystem::ApplyAudio()
{
	FAudioDevice* Device = GEngine ? GEngine->GetMainAudioDeviceRaw() : nullptr;
	AppliedMaster = Settings.MasterVolume;
	if (!Device)
	{
		return; // -nosound / headless: kept and applied when a device exists
	}
	Device->SetTransientPrimaryVolume(Settings.MasterVolume);
	if (!MusicClass) { MusicClass = TDSettings::LoadClassIfPresent(TDSettings::MusicClassPath); }
	if (!EffectsClass) { EffectsClass = TDSettings::LoadClassIfPresent(TDSettings::EffectsClassPath); }
	if (!MusicClass && !EffectsClass)
	{
		return; // Nick's sound classes don't exist yet: master only
	}
	if (!VolumeMix)
	{
		VolumeMix = NewObject<USoundMix>(this, TEXT("TD_OptionsVolumeMix"));
		Device->PushSoundMixModifier(VolumeMix);
	}
	if (MusicClass) { Device->SetSoundMixClassOverride(VolumeMix, MusicClass, Settings.MusicVolume, 1.f, 0.f, true); }
	if (EffectsClass) { Device->SetSoundMixClassOverride(VolumeMix, EffectsClass, Settings.EffectsVolume, 1.f, 0.f, true); }
}

void UTDSettingsSubsystem::ApplyVideo(bool bStartup)
{
	if (!bApplyVideo || GIsEditor || IsRunningCommandlet() || !GEngine || !Settings.bVideoChosen)
	{
		return; // never resize the editor or a headless run; never force a mode the player didn't choose
	}
	UGameUserSettings* GUS = GEngine->GetGameUserSettings();
	if (!GUS)
	{
		return;
	}
	const EWindowMode::Type Mode = Settings.WindowMode == ETDWindowMode::Fullscreen ? EWindowMode::Fullscreen
		: Settings.WindowMode == ETDWindowMode::Windowed ? EWindowMode::Windowed : EWindowMode::WindowedFullscreen;
	const FIntPoint Res = (Settings.Resolution.X > 0 && Settings.Resolution.Y > 0) ? Settings.Resolution : GUS->GetDesktopResolution();
	GUS->SetFullscreenMode(Mode);
	GUS->SetScreenResolution(Res);
	GUS->ApplyResolutionSettings(bStartup); // at start-up, -windowed / -ResX on the command line still win
	GUS->SaveSettings(); // the engine's own start-up then opens the window this way too
	UE_LOG(LogTriadDrift, Log, TEXT("TD settings: video %s %dx%d"), *UEnum::GetValueAsString(Settings.WindowMode), Res.X, Res.Y);
}

// ---- Dev command

namespace TDSettings
{
	static FAutoConsoleCommandWithWorldAndArgs CmdStatus(TEXT("TD.Settings"),
		TEXT("Log the player options (TD.Settings) or set one: TD.Settings <MouseSens|InvertMouse|PadSens|PadDeadzone|InvertPad|Hints|Master|Music|Effects|Window 0-2> <value>, or TD.Settings Res <W> <H>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UTDSettingsSubsystem* S = UTDSettingsSubsystem::GetTDSettings(World);
			if (!S) { UE_LOG(LogTriadDrift, Log, TEXT("TD.Settings: no settings subsystem")); return; }
			if (Args.Num() >= 2)
			{
				const FString& K = Args[0];
				const float V = FCString::Atof(*Args[1]);
				const bool B = V != 0.f;
				if (K == TEXT("MouseSens")) { S->SetMouseSensitivity(V); }
				else if (K == TEXT("InvertMouse")) { S->SetInvertMouseY(B); }
				else if (K == TEXT("PadSens")) { S->SetGamepadSensitivity(V); }
				else if (K == TEXT("PadDeadzone")) { S->SetGamepadDeadzone(V); }
				else if (K == TEXT("InvertPad")) { S->SetInvertGamepadY(B); }
				else if (K == TEXT("Hints")) { S->SetHintsEnabled(B); }
				else if (K == TEXT("Master")) { S->SetMasterVolume(V); }
				else if (K == TEXT("Music")) { S->SetMusicVolume(V); }
				else if (K == TEXT("Effects")) { S->SetEffectsVolume(V); }
				else if (K == TEXT("Camera")) { S->SetOnFootCameraMode(B ? ETDOnFootCamera::ThirdPerson : ETDOnFootCamera::FirstPerson); }
				else if (K == TEXT("Window")) { S->SetWindowMode((ETDWindowMode)FMath::Clamp(FCString::Atoi(*Args[1]), 0, 2)); }
				else if (K == TEXT("Res") && Args.Num() >= 3) { S->SetResolution(FIntPoint(FCString::Atoi(*Args[1]), FCString::Atoi(*Args[2]))); }
			}
			const FTDPlayerSettings& P = S->GetSettings();
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Settings: on-foot camera %s | page open %d"), *UEnum::GetValueAsString(P.OnFootCameraMode),
				UTDSettingsSubsystem::IsSettingsPageOpen(World) ? 1 : 0);
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Settings: mouse %.2f invert %d | pad %.2f deadzone %.2f invert %d | hints %d | master %.2f music %.2f effects %.2f (classes: music %d sfx %d) | video %s %dx%d"),
				P.MouseSensitivity, P.bInvertMouseY ? 1 : 0, P.GamepadSensitivity, P.GamepadDeadzone, P.bInvertGamepadY ? 1 : 0,
				P.bHintsEnabled ? 1 : 0, P.MasterVolume, P.MusicVolume, P.EffectsVolume, S->HasMusicClass() ? 1 : 0, S->HasEffectsClass() ? 1 : 0,
				*UEnum::GetValueAsString(P.WindowMode), P.Resolution.X, P.Resolution.Y);
			if (GEngine && GEngine->GameViewport)
			{
				FVector2D Size;
				GEngine->GameViewport->GetViewportSize(Size);
				UE_LOG(LogTriadDrift, Log, TEXT("TD.Settings: viewport now %.0fx%.0f"), Size.X, Size.Y);
			}
		}));
}
