// Triad Drift - scripted input for agent playtests (dev builds only). Owner: Teammate 1.
//
// Two ways for an agent to fly the ship in a running game (Tools/td-play-mcp.ps1 + Tools/td_mcp.py):
//  1. Unreal's InputToolset (ApplyInput): works once ATDSpacePlayerController::BeginPlay has activated the
//     toolset's local-player subsystem (see TDActivateInputToolset below).
//  2. These console commands (ConsoleToolset RunConsoleCommand), through the same APlayerController::InputKey
//     path real hardware uses, so every Enhanced Input mapping, trigger and modifier applies:
//       TD.Input.Key <Key> [Seconds=0.1]          press, hold, release (e.g. TD.Input.Key LeftShift 2)
//       TD.Input.Keys <K1+K2+...> [Seconds=0.1]   several keys held together (e.g. TD.Input.Keys W+LeftShift 3)
//       TD.Input.Axis <Key> <Value> [Seconds=0.1] analog every frame (e.g. TD.Input.Axis Gamepad_RightX 1 0.5)
//       TD.Input.Mouse <DX> <DY> [Seconds=0.1]    mouse delta per frame (virtual joystick)
//       TD.Input.ReleaseAll                       release everything this tool pressed
// Keys use FKey names (EKeys): W A S D Q E F H C T V Tab SpaceBar LeftShift One Two Three LeftMouseButton...

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Subsystems/SubsystemCollection.h"
#include "TriadDrift/Core/TDTypes.h"

namespace TDInputDebug
{
	struct FHeld
	{
		TWeakObjectPtr<APlayerController> PC;
		FKey Key;
		bool bAxis = false;
		float Value = 0.f;
		FVector2D MouseDelta = FVector2D::ZeroVector;
		double EndTime = 0.0;
	};

	TArray<FHeld>& GetHeld()
	{
		static TArray<FHeld> Held;
		return Held;
	}

	FTSTicker::FDelegateHandle& GetTickerHandle()
	{
		static FTSTicker::FDelegateHandle Handle;
		return Handle;
	}

	void Send(APlayerController* PC, const FKey& Key, EInputEvent Event, float Value)
	{
		if (PC)
		{
			FInputKeyEventArgs Args = FInputKeyEventArgs::CreateSimulated(Key, Event, Value, Event == IE_Axis ? 1 : 0);
			Args.DeltaTime = PC->GetWorld() ? PC->GetWorld()->GetDeltaSeconds() : (1.f / 60.f);
			PC->InputKey(Args);
		}
	}

	bool Tick(float)
	{
		const double Now = FPlatformTime::Seconds();
		TArray<FHeld>& Held = GetHeld();
		for (int32 i = Held.Num() - 1; i >= 0; --i)
		{
			FHeld& H = Held[i];
			APlayerController* PC = H.PC.Get();
			if (!PC)
			{
				Held.RemoveAt(i);
				continue;
			}
			if (Now >= H.EndTime)
			{
				if (H.bAxis)
				{
					Send(PC, H.Key, IE_Axis, 0.f);
				}
				else if (!H.Key.IsAnalog())
				{
					Send(PC, H.Key, IE_Released, 0.f);
				}
				Held.RemoveAt(i);
				continue;
			}
			if (H.bAxis)
			{
				if (H.Key == EKeys::Mouse2D)
				{
					Send(PC, EKeys::MouseX, IE_Axis, H.MouseDelta.X);
					Send(PC, EKeys::MouseY, IE_Axis, H.MouseDelta.Y);
				}
				else
				{
					Send(PC, H.Key, IE_Axis, H.Value);
				}
			}
		}
		if (Held.Num() == 0)
		{
			GetTickerHandle().Reset();
			return false; // ticker removes itself
		}
		return true;
	}

	void EnsureTicker()
	{
		if (!GetTickerHandle().IsValid())
		{
			GetTickerHandle() = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick));
		}
	}

	APlayerController* GetPC(UWorld* World)
	{
		return World ? World->GetFirstPlayerController() : nullptr;
	}

	float ParseSeconds(const TArray<FString>& Args, int32 Index)
	{
		return Args.IsValidIndex(Index) ? FMath::Max(0.f, FCString::Atof(*Args[Index])) : 0.1f;
	}

	void PressKeys(const TArray<FString>& Names, float Seconds, UWorld* World)
	{
		APlayerController* PC = GetPC(World);
		if (!PC)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Input: no player controller"));
			return;
		}
		for (const FString& Name : Names)
		{
			const FKey Key(*Name);
			if (!Key.IsValid())
			{
				UE_LOG(LogTriadDrift, Warning, TEXT("TD.Input: unknown key '%s'"), *Name);
				continue;
			}
			Send(PC, Key, IE_Pressed, 1.f);
			FHeld H;
			H.PC = PC;
			H.Key = Key;
			H.EndTime = FPlatformTime::Seconds() + Seconds;
			GetHeld().Add(H);
		}
		EnsureTicker();
	}

	void Key(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Input.Key <Key> [Seconds]"));
			return;
		}
		PressKeys({ Args[0] }, ParseSeconds(Args, 1), World);
	}

	void Keys(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Input.Keys <K1+K2+...> [Seconds]"));
			return;
		}
		TArray<FString> Names;
		Args[0].ParseIntoArray(Names, TEXT("+"), true);
		PressKeys(Names, ParseSeconds(Args, 1), World);
	}

	void Axis(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = GetPC(World);
		if (!PC || Args.Num() < 2)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Input.Axis <Key> <Value> [Seconds]"));
			return;
		}
		FHeld H;
		H.PC = PC;
		H.Key = FKey(*Args[0]);
		H.bAxis = true;
		H.Value = FCString::Atof(*Args[1]);
		H.EndTime = FPlatformTime::Seconds() + ParseSeconds(Args, 2);
		if (!H.Key.IsValid())
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Input.Axis: unknown key '%s'"), *Args[0]);
			return;
		}
		GetHeld().Add(H);
		EnsureTicker();
	}

	void Mouse(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = GetPC(World);
		if (!PC || Args.Num() < 2)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Input.Mouse <DX> <DY> [Seconds]"));
			return;
		}
		FHeld H;
		H.PC = PC;
		H.Key = EKeys::Mouse2D;
		H.bAxis = true;
		H.MouseDelta = FVector2D(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]));
		H.EndTime = FPlatformTime::Seconds() + ParseSeconds(Args, 2);
		GetHeld().Add(H);
		EnsureTicker();
	}

	void ReleaseAll(const TArray<FString>& Args, UWorld* World)
	{
		for (FHeld& H : GetHeld())
		{
			H.EndTime = 0.0;
		}
		Tick(0.f);
	}
}

static FAutoConsoleCommandWithWorldAndArgs GTDInputKeyCmd(TEXT("TD.Input.Key"),
	TEXT("Press, hold and release a key through the real input path: TD.Input.Key <Key> [Seconds=0.1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDInputDebug::Key));
static FAutoConsoleCommandWithWorldAndArgs GTDInputKeysCmd(TEXT("TD.Input.Keys"),
	TEXT("Hold several keys together: TD.Input.Keys W+LeftShift [Seconds=0.1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDInputDebug::Keys));
static FAutoConsoleCommandWithWorldAndArgs GTDInputAxisCmd(TEXT("TD.Input.Axis"),
	TEXT("Feed an analog key every frame: TD.Input.Axis <Key> <Value> [Seconds=0.1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDInputDebug::Axis));
static FAutoConsoleCommandWithWorldAndArgs GTDInputMouseCmd(TEXT("TD.Input.Mouse"),
	TEXT("Mouse delta every frame (virtual joystick): TD.Input.Mouse <DX> <DY> [Seconds=0.1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDInputDebug::Mouse));
static FAutoConsoleCommandWithWorldAndArgs GTDInputReleaseCmd(TEXT("TD.Input.ReleaseAll"),
	TEXT("Release every key / axis TD.Input pressed"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDInputDebug::ReleaseAll));

/**
 * UInputToolset (engine plugin, PostEngineInit) is a local-player subsystem; in a -game session the local
 * player exists before the plugin loads, so it never got an instance and every ApplyInput failed with
 * "dispatch target could not be resolved". Activate it for existing local players.
 */
void TDActivateInputToolset(ULocalPlayer* LocalPlayer)
{
	if (!LocalPlayer)
	{
		return;
	}
	UClass* ToolsetClass = FindObject<UClass>(nullptr, TEXT("/Script/InputToolset.InputToolset"));
	if (ToolsetClass && ToolsetClass->IsChildOf(ULocalPlayerSubsystem::StaticClass())
		&& !LocalPlayer->GetSubsystemBase(TSubclassOf<ULocalPlayerSubsystem>(ToolsetClass)))
	{
		FSubsystemCollectionBase::ActivateExternalSubsystem(ToolsetClass);
		UE_LOG(LogTriadDrift, Log, TEXT("TDInput: activated InputToolset for the local player (agent input injection)."));
	}
}

#else

class ULocalPlayer;
void TDActivateInputToolset(ULocalPlayer*) {}

#endif // !UE_BUILD_SHIPPING
