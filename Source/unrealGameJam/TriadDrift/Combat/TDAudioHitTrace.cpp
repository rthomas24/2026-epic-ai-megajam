// Triad Drift - audio hit trace (Teammate 2, board N-069). See the header.

#include "TriadDrift/Combat/TDAudioHitTrace.h"

#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "TriadDrift/Combat/TDCombatCueLog.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDAudioTrace, Log, All);

namespace TDAudioHitTraceState
{
	TArray<FString>& Recent()
	{
		static TArray<FString> Lines;
		return Lines;
	}
#if !UE_BUILD_SHIPPING
	static TAutoConsoleVariable<int32> CVarTraceHits(TEXT("TD.Audio.TraceHits"), 0,
		TEXT("1: log a timestamped line (utc, app, world and audio clocks, frame) for every projectile hull / shield hit, with the impact sound, ")
		TEXT("the firing ship, the victim and its shield before the hit (Development only; board N-069)."));
#endif
}

bool FTDAudioHitTrace::IsOn()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return TDAudioHitTraceState::CVarTraceHits.GetValueOnGameThread() != 0;
#endif
}

void FTDAudioHitTrace::SetOn(bool bOn)
{
#if !UE_BUILD_SHIPPING
	TDAudioHitTraceState::CVarTraceHits->Set(bOn ? 1 : 0, ECVF_SetByCode);
	TDAudioHitTraceState::Recent().Reset();
#endif
}

const TArray<FString>& FTDAudioHitTrace::GetRecent()
{
	return TDAudioHitTraceState::Recent();
}

void FTDAudioHitTrace::Trace(const UObject* WorldContext, const FTDFXCue* Cue, ETDCombatCueKind Kind, const AActor* Shooter, const AActor* Victim,
	const UTDHealthComponent* VictimHealth)
{
#if !UE_BUILD_SHIPPING
	if (!IsOn() || !Victim || !VictimHealth)
	{
		return;
	}
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	double AudioClock = -1.0;
	if (World)
	{
		if (FAudioDevice* Device = World->GetAudioDeviceRaw())
		{
			AudioClock = Device->GetAudioClock();
		}
	}
	const FString Sound = Cue && !Cue->Sound.IsNull() ? Cue->Sound.ToSoftObjectPath().ToString() : FString(TEXT("none"));
	const FString Line = FString::Printf(
		TEXT("TD audio hit: utc=%s app=%.3fs world=%.3fs audio=%.3fs frame=%llu cue=%s sound=%s shooter=%s victim=%s shield=%s %.0f/%.0f hull=%.0f/%.0f"),
		*FDateTime::UtcNow().ToIso8601(), FPlatformTime::Seconds() - GStartTime, World ? World->GetTimeSeconds() : -1.0, AudioClock,
		static_cast<uint64>(GFrameCounter), FTDCombatCueLog::KindName(Kind), *Sound, Shooter ? *Shooter->GetName() : TEXT("none"), *Victim->GetName(),
		VictimHealth->IsShieldUp() ? TEXT("up") : TEXT("down"), VictimHealth->GetShield(),
		VictimHealth->GetShield01() > 0.f ? VictimHealth->GetShield() / VictimHealth->GetShield01() : 0.f, VictimHealth->GetHull(),
		VictimHealth->GetHull01() > 0.f ? VictimHealth->GetHull() / VictimHealth->GetHull01() : 0.f);
	UE_LOG(LogTDAudioTrace, Display, TEXT("%s"), *Line);
	TArray<FString>& Recent = TDAudioHitTraceState::Recent();
	if (Recent.Num() >= 64)
	{
		Recent.RemoveAt(0);
	}
	Recent.Add(Line);
#endif
}

namespace TDAudioHitTraceImpl
{
	ATDEnemyShip* SpawnHeld(UWorld* World, const FVector& At, const FVector& Facing, const TCHAR* Name, float Shield)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Name = MakeUniqueObjectName(World->GetCurrentLevel(), ATDEnemyShip::StaticClass(), FName(Name));
		ATDEnemyShip* Enemy = World->SpawnActor<ATDEnemyShip>(ATDEnemyShip::StaticClass(), At, Facing.Rotation(), Params);
		if (!Enemy)
		{
			return nullptr;
		}
		Enemy->InitFromSpec(TDEnemyIds::ShardFighter);
		if (AController* C = Enemy->GetController())
		{
			C->UnPossess();
			C->Destroy();
		}
		if (UMovementComponent* Move = Enemy->FindComponentByClass<UMovementComponent>())
		{
			Move->StopMovementImmediately();
			Move->Deactivate();
		}
		if (UTDHealthComponent* H = UTDStatics::GetHealth(Enemy))
		{
			H->InitMaxValues(FTDAudioHitTrace::TargetPoints, Shield, true);
		}
		return Enemy;
	}
}

void FTDAudioHitTrace::SpawnTargets(UWorld* World, const FVector& At, const FVector& Forward, float Metres, AActor*& OutShieldUp, AActor*& OutShieldDown)
{
	OutShieldUp = OutShieldDown = nullptr;
	if (!World)
	{
		return;
	}
	const FVector F = Forward.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const FVector R = FVector::CrossProduct(FVector::UpVector, F).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
	const FVector Centre = At + F * FMath::Max(5.f, Metres) * 100.f;
	OutShieldUp = TDAudioHitTraceImpl::SpawnHeld(World, Centre - R * 600.f, -F, TEXT("TDAudioTarget_ShieldUp"), TargetPoints);
	OutShieldDown = TDAudioHitTraceImpl::SpawnHeld(World, Centre + R * 600.f, -F, TEXT("TDAudioTarget_ShieldDown"), 0.f);
	UE_LOG(LogTDAudioTrace, Display, TEXT("TD.Audio.SpawnTargets: %s (shield up, left) and %s (no shield, right), %.0f m ahead."),
		OutShieldUp ? *OutShieldUp->GetName() : TEXT("FAILED"), OutShieldDown ? *OutShieldDown->GetName() : TEXT("FAILED"), Metres);
}

#if !UE_BUILD_SHIPPING
namespace TDAudioHitTraceImpl
{
	static FAutoConsoleCommandWithWorldAndArgs CmdSpawnTargets(TEXT("TD.Audio.SpawnTargets"),
		TEXT("TD.Audio.SpawnTargets [metres=40]: two held, unkillable targets ahead of the flown ship: TDAudioTarget_ShieldUp (left; every hit ")
		TEXT("plays ImpactShield) and TDAudioTarget_ShieldDown (right; no shield, every hit plays ImpactHull). Pair with TD.Audio.TraceHits 1."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			if (!Pawn)
			{
				UE_LOG(LogTDAudioTrace, Warning, TEXT("TD.Audio.SpawnTargets: no flown ship."));
				return;
			}
			AActor* Up = nullptr;
			AActor* Down = nullptr;
			FTDAudioHitTrace::SpawnTargets(World, Pawn->GetActorLocation(), Pawn->GetActorForwardVector(), Args.Num() > 0 ? FCString::Atof(*Args[0]) : 40.f, Up, Down);
		}));
}
#endif
