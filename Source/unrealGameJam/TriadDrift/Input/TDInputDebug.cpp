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
//       TD.Input.Tap <Key> [Count=2] [Interval=0.15] [AxisValue=1]   taps in one go (double-tap dodge: TD.Input.Tap D 2;
//                                                 stick flick: TD.Input.Tap Gamepad_LeftX 2 0.15 -1)
//       TD.Input.TurnToward <Yaw> <Pitch> [Seconds=3]  steer the nose onto a world bearing through the real mouse
//                                                 input (virtual stick), then let go (agent pilots: T2 / T4)
//       TD.Input.ReleaseAll                       release everything this tool pressed
// Keys use FKey names (EKeys): W A S D Q E F H C T V Tab SpaceBar LeftShift One Two Three LeftMouseButton...

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Input/TDHintComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/TDAbilityComponent.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"

#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Subsystems/SubsystemCollection.h"
#include "TriadDrift/Core/TDTypes.h"

#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "EngineUtils.h"

/**
 * One frame of steering toward a world bearing, through the real mouse path (APlayerController::InputKey with
 * MouseX / MouseY deltas onto the virtual stick). Returns true once the nose is within 2 degrees (stick released).
 */
bool TDInputDebug_TurnTowardStep(APlayerController* PC, const FRotator& Target)
{
	ATDSpacePlayerController* SPC = Cast<ATDSpacePlayerController>(PC);
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!SPC || !Pawn)
	{
		return true;
	}
	const FVector Local = Pawn->GetActorQuat().UnrotateVector(Target.Vector());
	const float YawErr = FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
	const float PitchErr = FMath::RadiansToDegrees(FMath::Atan2(Local.Z, FVector2D(Local.X, Local.Y).Size()));
	// Settled = on the bearing AND no longer turning (else the turn's momentum carries the nose past it).
	FVector Rate = FVector::ZeroVector;
	if (const ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Pawn))
	{
		if (const UTDShipMovementComponent* Move = Craft->GetShipMovement()) { Rate = Move->GetAngularRate(); }
	}
	const bool bOnBearing = FMath::Abs(YawErr) < 2.f && FMath::Abs(PitchErr) < 2.f
		&& FMath::Abs(Rate.Y) < 10.f && FMath::Abs(Rate.Z) < 10.f;
	// Wanted stick: proportional to the error with rate damping (full beyond 25 deg), centred once settled.
	const float Damp = 0.15f; // stick per (deg/s) of current turn, opposing it
	const FVector2D Want = bOnBearing ? FVector2D::ZeroVector
		: FVector2D(FMath::Clamp(YawErr / 25.f - Rate.Z * Damp / 25.f, -1.f, 1.f), FMath::Clamp(PitchErr / 25.f - Rate.Y * Damp / 25.f, -1.f, 1.f));
	const FVector2D Delta = (Want - SPC->GetVirtualStick()) / FMath::Max(SPC->GetMouseSensitivity(), 1e-4f);
	auto SendAxis = [PC](const FKey& Key, float V)
	{
		FInputKeyEventArgs A = FInputKeyEventArgs::CreateSimulated(Key, IE_Axis, V, 1);
		A.DeltaTime = PC->GetWorld() ? PC->GetWorld()->GetDeltaSeconds() : 1.f / 60.f;
		PC->InputKey(A);
	};
	SendAxis(EKeys::MouseX, Delta.X);
	SendAxis(EKeys::MouseY, SPC->IsMousePitchInverted() ? -Delta.Y : Delta.Y);
	return bOnBearing;
}

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
		/** Taps scheduled ahead: pressed at StartTime (not yet pressed while bPending). */
		double StartTime = 0.0;
		bool bPending = false;
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
			if (H.bPending)
			{
				if (Now >= H.StartTime)
				{
					if (!H.bAxis)
					{
						Send(PC, H.Key, IE_Pressed, 1.f);
					}
					H.bPending = false; // an axis tap is driven every frame below until EndTime
				}
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

	void Tap(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = GetPC(World);
		if (!PC || Args.Num() < 1 || !FKey(*Args[0]).IsValid())
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Input.Tap <Key> [Count=2] [Interval=0.15]"));
			return;
		}
		const int32 Count = Args.IsValidIndex(1) ? FMath::Clamp(FCString::Atoi(*Args[1]), 1, 10) : 2;
		const float Interval = Args.IsValidIndex(2) ? FMath::Max(0.04f, FCString::Atof(*Args[2])) : 0.15f;
		// Analog keys (sticks, triggers) tap as an axis value: TD.Input.Tap Gamepad_LeftX 2 0.15 -1 = double flick left.
		const FKey TapKey(*Args[0]);
		const float AxisValue = Args.IsValidIndex(3) ? FCString::Atof(*Args[3]) : 1.f;
		const double Now = FPlatformTime::Seconds();
		for (int32 i = 0; i < Count; ++i)
		{
			FHeld H;
			H.PC = PC;
			H.Key = FKey(*Args[0]);
			H.StartTime = Now + i * Interval;
			H.EndTime = H.StartTime + Interval * 0.4f;
			H.bPending = true;
			H.bAxis = TapKey.IsAnalog();
			H.Value = AxisValue;
			GetHeld().Add(H);
		}
		EnsureTicker();
	}

	struct FTurn
	{
		TWeakObjectPtr<APlayerController> PC;
		FRotator Target;
		double EndTime = 0.0;
	};

	TArray<FTurn>& GetTurns()
	{
		static TArray<FTurn> Turns;
		return Turns;
	}

	bool TurnTick(float)
	{
		TArray<FTurn>& Turns = GetTurns();
		const double Now = FPlatformTime::Seconds();
		for (int32 i = Turns.Num() - 1; i >= 0; --i)
		{
			APlayerController* PC = Turns[i].PC.Get();
			const bool bDone = !PC || Now >= Turns[i].EndTime || TDInputDebug_TurnTowardStep(PC, Turns[i].Target);
			if (bDone)
			{
				Turns.RemoveAt(i);
			}
		}
		return Turns.Num() > 0;
	}

	void TurnToward(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = GetPC(World);
		if (!PC || Args.Num() < 2)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Input.TurnToward <Yaw> <Pitch> [Seconds=3] (world degrees)"));
			return;
		}
		FTurn T;
		T.PC = PC;
		T.Target = FRotator(FCString::Atof(*Args[1]), FCString::Atof(*Args[0]), 0.f);
		T.EndTime = FPlatformTime::Seconds() + (Args.IsValidIndex(2) ? FMath::Max(0.1f, FCString::Atof(*Args[2])) : 3.f);
		const bool bStart = GetTurns().Num() == 0;
		GetTurns().RemoveAll([PC](const FTurn& X) { return X.PC.Get() == PC; });
		GetTurns().Add(T);
		if (bStart)
		{
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TurnTick));
		}
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
static FAutoConsoleCommandWithWorldAndArgs GTDInputTapCmd(TEXT("TD.Input.Tap"),
	TEXT("Tap a key Count times, Interval apart, in one go: TD.Input.Tap <Key> [Count=2] [Interval=0.15]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDInputDebug::Tap));
static FAutoConsoleCommandWithWorldAndArgs GTDInputTurnTowardCmd(TEXT("TD.Input.TurnToward"),
	TEXT("Steer the player's nose onto a world bearing with real mouse input: TD.Input.TurnToward <Yaw> <Pitch> [Seconds=3]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDInputDebug::TurnToward));
// ---- Mouse-flight probe (Ryan, Nov 14: "easier to control the ships with the mouse"): one fixed "hand" drives the
// real mouse path (InputKey MouseX / MouseY) toward a target and logs the numbers, the same before and after a change.
// The hand moves the mouse in proportion to the aim error (the reticle's world direction to the target), at most
// MaxCountsPerSec; it never reads the virtual stick (a player sees the reticle and the target, not the stick).
namespace TDMouseProbe
{
	struct FRun
	{
		TWeakObjectPtr<APlayerController> PC;
		FString Test;
		FVector TargetDir = FVector::ForwardVector;	// fixed world direction (turns, acquire)
		TWeakObjectPtr<AActor> TargetActor;			// enemy: follows it
		double Start = 0.0, Last = 0.0;
		float Angle = 0.f;							// for turns: the yaw to reach
		float StartYaw = 0.f;
		float NoseReach = -1.f, AimSettleStart = -1.f, Settled = -1.f, MaxPast = 0.f, Counts = 0.f, PeakStick = 0.f;
		float FirstErr = 0.f;
		bool bCrossed = false;
		FVector2D Carry = FVector2D::ZeroVector;
	};
	static TArray<FRun>& Runs() { static TArray<FRun> R; return R; }
	static float Gain = 60.f;			// counts/s per degree of error
	static float MaxCountsPerSec = 1500.f;	// a brisk sweep (about 2 in/s at 800 dpi)

	static void Finish(FRun& R, const TCHAR* Why)
	{
		const APawn* Pawn = R.PC.IsValid() ? R.PC->GetPawn() : nullptr;
		UE_LOG(LogTriadDrift, Display, TEXT("TDMOUSEPROBE %s ship=%s firstErr=%.1f noseReach=%.2f settled=%.2f overshoot=%.1f counts=%.0f peakStick=%.2f (%s)"),
			*R.Test, Pawn ? *Pawn->GetName() : TEXT("?"), R.FirstErr, R.NoseReach, R.Settled, R.MaxPast, R.Counts, R.PeakStick, Why);
	}

	static bool Tick(float)
	{
		TArray<FRun>& All = Runs();
		const double Now = FPlatformTime::Seconds();
		for (int32 i = All.Num() - 1; i >= 0; --i)
		{
			FRun& R = All[i];
			ATDSpacePlayerController* SPC = Cast<ATDSpacePlayerController>(R.PC.Get());
			const APawn* Pawn = SPC ? SPC->GetPawn() : nullptr;
			if (!SPC || !Pawn) { Finish(R, TEXT("no pawn")); All.RemoveAt(i); continue; }
			const float Dt = (float)FMath::Clamp(Now - R.Last, 0.0, 0.1);
			R.Last = Now;
			const float T = (float)(Now - R.Start);
			if (R.TargetActor.IsValid()) { R.TargetDir = (R.TargetActor->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal(); }
			else if (R.Test == TEXT("enemy")) { Finish(R, TEXT("target gone")); All.RemoveAt(i); continue; }
			// Aim error in the ship's frame (yaw right +, pitch up +), from the reticle's world direction.
			const FQuat Q = Pawn->GetActorQuat();
			auto Angles = [&Q](const FVector& Dir) { const FVector L = Q.UnrotateVector(Dir); return FVector2D(FMath::RadiansToDegrees(FMath::Atan2(L.Y, L.X)), FMath::RadiansToDegrees(FMath::Atan2(L.Z, FVector2D(L.X, L.Y).Size()))); };
			const FVector2D AimA = Angles(SPC->GetAimWorldDirection()), TgtA = Angles(R.TargetDir);
			const FVector2D Err = TgtA - AimA;
			const float AimErr = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(SPC->GetAimWorldDirection(), R.TargetDir), -1.f, 1.f)));
			const float NoseErr = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Pawn->GetActorForwardVector(), R.TargetDir), -1.f, 1.f)));
			if (T < 0.05f) { R.FirstErr = AimErr; }
			if (R.NoseReach < 0.f && NoseErr < 3.f) { R.NoseReach = T; }
			if (R.Angle > 0.f)
			{
				// Turns: overshoot = how far the nose went past the target bearing (yaw).
				const float Turned = FMath::FindDeltaAngleDegrees(R.StartYaw, Pawn->GetActorRotation().Yaw) * FMath::Sign(R.Angle);
				const float TurnedAbs = Turned < -90.f ? Turned + 360.f : Turned;
				if (R.NoseReach >= 0.f) { R.MaxPast = FMath::Max(R.MaxPast, TurnedAbs - FMath::Abs(R.Angle)); }
			}
			else if (R.NoseReach >= 0.f || AimErr < 1.5f)
			{
				// Acquire: overshoot = the aim error after the reticle first reached the target.
				if (AimErr < 1.5f) { R.bCrossed = true; }
				if (R.bCrossed) { R.MaxPast = FMath::Max(R.MaxPast, AimErr); }
			}
			R.PeakStick = FMath::Max(R.PeakStick, (float)SPC->GetVirtualStick().Size());
			// Settled: the reticle within 1.5 deg of the target for 1 s.
			if (AimErr < 1.5f) { if (R.AimSettleStart < 0.f) { R.AimSettleStart = T; } }
			else { R.AimSettleStart = -1.f; }
			if (R.AimSettleStart >= 0.f && T - R.AimSettleStart >= 1.f) { R.Settled = R.AimSettleStart; Finish(R, TEXT("held 1 s")); All.RemoveAt(i); continue; }
			if (T > 10.f) { Finish(R, TEXT("timeout 10 s")); All.RemoveAt(i); continue; }
			// The hand: counts toward the target, proportional to the error, capped.
			FVector2D Want(Err.X * Gain, Err.Y * Gain);
			if (Want.Size() > MaxCountsPerSec) { Want = Want.GetSafeNormal() * MaxCountsPerSec; }
			const FVector2D Move = Want * Dt + R.Carry;
			const FVector2D Whole(FMath::RoundToFloat(Move.X), FMath::RoundToFloat(Move.Y));
			R.Carry = Move - Whole;
			R.Counts += FMath::Abs(Whole.X) + FMath::Abs(Whole.Y);
			APlayerController* Base = SPC; // InputKey is public on APlayerController (protected on ours)
			auto SendAxis = [Base](const FKey& Key, float V)
			{
				FInputKeyEventArgs A = FInputKeyEventArgs::CreateSimulated(Key, IE_Axis, V, 1);
				A.DeltaTime = Base->GetWorld() ? Base->GetWorld()->GetDeltaSeconds() : 1.f / 60.f;
				Base->InputKey(A);
			};
			if (Whole.X != 0.f) { SendAxis(EKeys::MouseX, Whole.X); }
			if (Whole.Y != 0.f) { SendAxis(EKeys::MouseY, SPC->IsMousePitchInverted() ? -Whole.Y : Whole.Y); }
		}
		return All.Num() > 0;
	}

	static void Start(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn || Args.Num() < 1)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Flight.MouseProbe turn90|turn180|acquire30|enemy [Gain=60] [MaxCountsPerSec=1500]"));
			return;
		}
		if (Args.IsValidIndex(1)) { Gain = FCString::Atof(*Args[1]); }
		if (Args.IsValidIndex(2)) { MaxCountsPerSec = FCString::Atof(*Args[2]); }
		FRun R;
		R.PC = PC;
		R.Test = Args[0].ToLower();
		R.Start = R.Last = FPlatformTime::Seconds();
		const FRotator Level(0.f, Pawn->GetActorRotation().Yaw, 0.f);
		R.StartYaw = Level.Yaw;
		if (R.Test == TEXT("turn90") || R.Test == TEXT("turn180"))
		{
			R.Angle = R.Test == TEXT("turn90") ? 90.f : 179.f;
			R.TargetDir = (Level + FRotator(0.f, R.Angle, 0.f)).Vector();
		}
		else if (R.Test == TEXT("acquire30"))
		{
			R.TargetDir = Pawn->GetActorQuat().RotateVector(FRotator(15.f, 26.f, 0.f).Vector()); // 30 deg off the nose
		}
		else if (R.Test == TEXT("enemy"))
		{
			float Best = TNumericLimits<float>::Max();
			for (TActorIterator<ATDEnemyShip> It(World); It; ++It)
			{
				const FVector To = It->GetActorLocation() - Pawn->GetActorLocation();
				const float Off = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(To.GetSafeNormal(), Pawn->GetActorForwardVector()), -1.f, 1.f)));
				if (Off > 15.f && Off < 60.f && To.Size() < 150000.f && To.Size() < Best) { Best = To.Size(); R.TargetActor = *It; }
			}
			if (!R.TargetActor.IsValid()) { UE_LOG(LogTriadDrift, Warning, TEXT("TDMOUSEPROBE enemy: none 15-60 deg off the nose within 1.5 km")); return; }
		}
		else
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Flight.MouseProbe: unknown test %s"), *Args[0]);
			return;
		}
		const bool bStart = Runs().Num() == 0;
		Runs().Reset();
		Runs().Add(R);
		if (bStart) { FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TDMouseProbe::Tick)); }
	}
}
static FAutoConsoleCommandWithWorldAndArgs GTDMouseProbeCmd(TEXT("TD.Flight.MouseProbe"),
	TEXT("Measure mouse flight through the real mouse path: TD.Flight.MouseProbe turn90|turn180|acquire30|enemy [Gain=60] [MaxCountsPerSec=1500] (logs TDMOUSEPROBE)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDMouseProbe::Start));

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

namespace TDInputStateDebug
{
	// One line of everything a pad playtest needs to check: device, flight input, ship states, powers, weapons, pause,
	// the settings page, and on foot the pilot's move / blade / block / dash state.
	static void State(const TArray<FString>& Args, UWorld* World)
	{
		ATDSpacePlayerController* PC = World ? Cast<ATDSpacePlayerController>(World->GetFirstPlayerController()) : nullptr;
		if (!PC) { UE_LOG(LogTriadDrift, Log, TEXT("TD.Input.State: no space controller")); return; }
		const FString Tag = Args.Num() > 0 ? Args[0] : TEXT("-");
		FString Line = FString::Printf(TEXT("[%s] pad %d paused %d settings %d hint '%s'"), *Tag, PC->IsUsingGamepad() ? 1 : 0,
			PC->IsGamePausedByPlayer() ? 1 : 0, UTDSettingsSubsystem::IsSettingsPageOpen(World) ? 1 : 0,
			PC->GetHints() ? *PC->GetHints()->GetCurrentText().ToString() : TEXT(""));
		if (const ATDShip* Ship = Cast<ATDShip>(PC->GetPawn()))
		{
			const FTDFlightInput& In = PC->GetCurrentFlightInput();
			Line += FString::Printf(TEXT(" | ship %s throttle %.2f aim %.2f,%.2f roll %.2f strafe %.2f,%.2f boost %d drift %d brake %d dodging %d cockpit %d"),
				*UEnum::GetDisplayValueAsText(Ship->GetShipId()).ToString(), In.Throttle, In.Aim.X, In.Aim.Y, In.Roll, In.Strafe.X, In.Strafe.Y,
				Ship->IsBoosting() ? 1 : 0, Ship->IsDrifting() ? 1 : 0, In.bBrake ? 1 : 0,
				Ship->GetShipMovement() && Ship->GetShipMovement()->IsDodging() ? 1 : 0, Ship->IsCockpitView() ? 1 : 0);
			if (const UTDWeaponComponent* W = Ship->FindComponentByClass<UTDWeaponComponent>())
			{
				Line += FString::Printf(TEXT(" | heat %.2f ammo %d"), W->GetHeat01(ETDWeaponSlot::Primary), W->GetAmmo(ETDWeaponSlot::Secondary));
			}
			TInlineComponentArray<UTDPowerComponent*> Powers(Ship);
			for (const UTDPowerComponent* P : Powers)
			{
				Line += FString::Printf(TEXT(" | %s active %d cd %.2f"), *P->GetPowerName().ToString(), P->IsPowerActive() ? 1 : 0, P->GetCooldown01());
			}
			if (const UTDHyperdriveComponent* H = Ship->GetHyperdrive())
			{
				Line += FString::Printf(TEXT(" | hyper %d"), (int32)H->GetState());
			}
		}
		else if (const ATDFirstPersonCharacter* Pilot = Cast<ATDFirstPersonCharacter>(PC->GetPawn()))
		{
			const UTDEnergyBladeComponent* Blade = Pilot->GetBlade();
			Line += FString::Printf(TEXT(" | foot vel %.0f sprint %d dash %d charges %d block %d attack %d combo %d interact %.2f jumps %d third %d"),
				(float)Pilot->GetVelocity().Size2D(), Pilot->IsSprinting() ? 1 : 0, Pilot->IsDashing() ? 1 : 0, Pilot->GetDashCharges(),
				Pilot->IsBlocking() ? 1 : 0, Blade && Blade->IsAttacking() ? 1 : 0, Blade ? Blade->GetComboIndex() : -1,
				Pilot->GetInteractProgress01(), Pilot->GetJumpsUsed(), Pilot->IsThirdPersonView() ? 1 : 0);
			if (const UTDSettingsSubsystem* S = UTDSettingsSubsystem::GetTDSettings(World))
			{
				Line += FString::Printf(TEXT(" mode %d"), (int32)S->GetOnFootCameraMode());
			}
		}
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Input.State: %s"), *Line);
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdState(TEXT("TD.Input.State"),
		TEXT("TD.Input.State [label]: one line of controller / ship / power / weapon / pilot state (pad playtests)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&State));
}


#else

class ULocalPlayer;
void TDActivateInputToolset(ULocalPlayer*) {}


#endif // !UE_BUILD_SHIPPING
