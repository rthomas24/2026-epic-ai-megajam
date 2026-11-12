#include "TriadDrift/Core/TDStatics.h"

#include "Camera/CameraShakeBase.h"
#include "Components/AudioComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDDamageTypes.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"

// ---------------------------------------------------------------------------------------------- teams

ETDTeam UTDStatics::GetTeam(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return ETDTeam::Neutral;
	}
	if (const ITDTeam* TeamActor = Cast<ITDTeam>(Actor))
	{
		return TeamActor->GetTeam();
	}
	if (const UTDHealthComponent* Health = Actor->FindComponentByClass<UTDHealthComponent>())
	{
		return Health->DefaultTeam;
	}
	return ETDTeam::Neutral;
}

bool UTDStatics::AreTeamsHostile(ETDTeam A, ETDTeam B)
{
	return (A == ETDTeam::Player && B == ETDTeam::Enemy) || (A == ETDTeam::Enemy && B == ETDTeam::Player);
}

bool UTDStatics::AreHostile(const AActor* A, const AActor* B)
{
	return AreTeamsHostile(GetTeam(A), GetTeam(B));
}

UTDHealthComponent* UTDStatics::GetHealth(const AActor* Actor)
{
	return IsValid(Actor) ? Actor->FindComponentByClass<UTDHealthComponent>() : nullptr;
}

// --------------------------------------------------------------------------------------------- damage

float UTDStatics::ApplyTDDamage(AActor* Target, float Amount, ETDDamageKind Kind, AActor* Instigator, AActor* Causer,
	FVector HitLocation, FVector HitDirection, bool bIgnoreShield)
{
	if (!IsValid(Target) || Amount <= 0.f)
	{
		return 0.f;
	}

	// Friendly fire is off. The causer's team wins (a deflected projectile hurts its old team).
	ETDTeam SourceTeam = GetTeam(Causer);
	if (SourceTeam == ETDTeam::Neutral)
	{
		SourceTeam = GetTeam(Instigator);
	}
	if (SourceTeam != ETDTeam::Neutral && SourceTeam == GetTeam(Target))
	{
		return 0.f;
	}

	if (UTDHealthComponent* Health = GetHealth(Target))
	{
		FTDDamageInfo Info;
		Info.Amount = Amount;
		Info.Kind = Kind;
		Info.Instigator = Instigator;
		Info.Causer = Causer;
		Info.HitLocation = HitLocation.IsZero() ? Target->GetActorLocation() : HitLocation;
		Info.HitDirection = HitDirection;
		Info.bIgnoreShield = bIgnoreShield;
		return Health->ApplyDamage(Info);
	}

	// No health component: fall back to the engine path (destructible props, template actors).
	return UGameplayStatics::ApplyDamage(Target, Amount, nullptr, Causer ? Causer : Instigator, UTDDamageType::ClassFor(Kind));
}

int32 UTDStatics::ApplyTDRadialDamage(const UObject* WorldContextObject, FVector Origin, float Radius, float Amount,
	ETDDamageKind Kind, ETDTeam SourceTeam, AActor* Instigator, AActor* Causer, bool bFalloff, AActor* IgnoreActor)
{
	const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(WorldContextObject);
	if (!Registry || Radius <= 0.f || Amount <= 0.f)
	{
		return 0;
	}

	// Copy: applying damage can kill actors and mutate the registry.
	TArray<AActor*, TInlineAllocator<32>> Victims;
	for (const TWeakObjectPtr<AActor>& Entry : Registry->GetAll())
	{
		AActor* Actor = Entry.Get();
		if (!IsValid(Actor) || Actor == IgnoreActor)
		{
			continue;
		}
		const ETDTeam Team = GetTeam(Actor);
		const bool bHostile = (SourceTeam == ETDTeam::Neutral) ? true : AreTeamsHostile(SourceTeam, Team);
		if (bHostile)
		{
			Victims.Add(Actor);
		}
	}

	int32 HitCount = 0;
	for (AActor* Victim : Victims)
	{
		FVector BoundsOrigin, BoundsExtent;
		Victim->GetActorBounds(false, BoundsOrigin, BoundsExtent);
		if (BoundsExtent.IsNearlyZero())
		{
			// No primitive components (logic-only actors, tests): measure to the actor itself.
			BoundsOrigin = Victim->GetActorLocation();
		}
		const float SurfaceDistance = FMath::Max(0.f, static_cast<float>(FVector::Dist(Origin, BoundsOrigin)) - static_cast<float>(BoundsExtent.GetMin()));
		if (SurfaceDistance > Radius)
		{
			continue;
		}
		const float Scale = bFalloff ? FMath::Lerp(1.f, 0.25f, SurfaceDistance / Radius) : 1.f;
		const FVector Direction = (BoundsOrigin - Origin).GetSafeNormal();

		UTDHealthComponent* Health = GetHealth(Victim);
		if (!Health || Health->IsDead())
		{
			continue;
		}
		FTDDamageInfo Info;
		Info.Amount = Amount * Scale;
		Info.Kind = Kind;
		Info.Instigator = Instigator;
		Info.Causer = Causer;
		Info.HitLocation = BoundsOrigin;
		Info.HitDirection = Direction;
		if (Health->ApplyDamage(Info) > 0.f)
		{
			++HitCount;
		}
	}
	return HitCount;
}

// ----------------------------------------------------------------------------------------------- flow

ITDGameFlow* UTDStatics::GetGameFlow(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
	{
		return nullptr;
	}
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return Cast<ITDGameFlow>(GameInstance);
}

// --------------------------------------------------------------------------------------- presentation

namespace
{
	void PlayCueShake(const UObject* WorldContextObject, const FTDFXCue& Cue, const FVector& Location)
	{
		if (Cue.Shake.IsNull() || !GEngine)
		{
			return;
		}
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
		UClass* ShakeClass = Cue.Shake.LoadSynchronous();
		if (!World || !ShakeClass)
		{
			return;
		}
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (!PC || !PC->IsLocalController())
			{
				continue;
			}
			if (Cue.ShakeRadius > 0.f)
			{
				FVector ViewLocation;
				FRotator ViewRotation;
				PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
				if (FVector::DistSquared(ViewLocation, Location) > FMath::Square(Cue.ShakeRadius))
				{
					continue;
				}
			}
			PC->ClientStartCameraShake(ShakeClass, Cue.ShakeScale);
		}
	}
}

UNiagaraComponent* UTDStatics::PlayCue(const UObject* WorldContextObject, const FTDFXCue& Cue, FVector Location, FRotator Rotation)
{
	if (!WorldContextObject || Cue.IsEmpty())
	{
		return nullptr;
	}

	UNiagaraComponent* Component = nullptr;
	if (!Cue.VFX.IsNull())
	{
		if (UNiagaraSystem* System = Cue.VFX.LoadSynchronous())
		{
			Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContextObject, System, Location, Rotation, Cue.VFXScale,
				true, true, ENCPoolMethod::AutoRelease, true);
		}
	}
	if (!Cue.Sound.IsNull())
	{
		if (USoundBase* Sound = Cue.Sound.LoadSynchronous())
		{
			UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Sound, Location, Rotation);
		}
	}
	PlayCueShake(WorldContextObject, Cue, Location);
	return Component;
}

UNiagaraComponent* UTDStatics::PlayCueAttached(const FTDFXCue& Cue, USceneComponent* AttachTo, FName Socket)
{
	UAudioComponent* Audio = nullptr;
	return PlayCueAttachedLoop(Cue, AttachTo, Socket, Audio);
}

UNiagaraComponent* UTDStatics::PlayCueAttachedLoop(const FTDFXCue& Cue, USceneComponent* AttachTo, FName Socket,
	UAudioComponent*& OutAudio)
{
	OutAudio = nullptr;
	if (!IsValid(AttachTo) || Cue.IsEmpty())
	{
		return nullptr;
	}

	UNiagaraComponent* Component = nullptr;
	if (!Cue.VFX.IsNull())
	{
		if (UNiagaraSystem* System = Cue.VFX.LoadSynchronous())
		{
			Component = UNiagaraFunctionLibrary::SpawnSystemAttached(System, AttachTo, Socket, FVector::ZeroVector,
				FRotator::ZeroRotator, Cue.VFXScale, EAttachLocation::SnapToTarget, true, ENCPoolMethod::None, true);
		}
	}
	if (!Cue.Sound.IsNull())
	{
		if (USoundBase* Sound = Cue.Sound.LoadSynchronous())
		{
			OutAudio = UGameplayStatics::SpawnSoundAttached(Sound, AttachTo, Socket);
		}
	}
	PlayCueShake(AttachTo, Cue, AttachTo->GetComponentLocation());
	return Component;
}

UMaterialInstanceDynamic* UTDStatics::TintPlaceholder(UMeshComponent* Mesh, FLinearColor Color, float EmissiveBoost)
{
	if (!IsValid(Mesh))
	{
		return nullptr;
	}

	// LoadObject is a cheap find after the first call; weak pointers survive a level change safely.
	static TWeakObjectPtr<UMaterialInterface> LitBase;
	static TWeakObjectPtr<UMaterialInterface> GlowBase;
	if (!LitBase.IsValid())
	{
		LitBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	if (EmissiveBoost > 0.f && !GlowBase.IsValid())
	{
		GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	}

	// The glow material may be missing from a cooked build; the lit one is always cooked.
	const bool bGlow = EmissiveBoost > 0.f && GlowBase.IsValid();
	UMaterialInterface* Base = bGlow ? GlowBase.Get() : LitBase.Get();
	if (!Base)
	{
		return nullptr;
	}

	const FLinearColor Final = bGlow ? Color * EmissiveBoost : Color;
	UMaterialInstanceDynamic* First = nullptr;
	const int32 SlotCount = FMath::Max(1, Mesh->GetNumMaterials());
	for (int32 Slot = 0; Slot < SlotCount; ++Slot)
	{
		UMaterialInstanceDynamic* Instance = Mesh->CreateDynamicMaterialInstance(Slot, Base);
		if (!Instance)
		{
			continue;
		}
		Instance->SetVectorParameterValue(TEXT("Color"), Final);
		if (bGlow)
		{
			Instance->SetVectorParameterValue(TEXT("EmissiveColor"), Final);
		}
		First = First ? First : Instance;
	}
	return First;
}

FLinearColor UTDStatics::ShipColor(ETDShipId Ship)
{
	switch (Ship)
	{
	case ETDShipId::Rift: return FLinearColor(1.f, 0.17f, 0.84f);
	case ETDShipId::Lancer: return FLinearColor(0.25f, 0.91f, 1.f);
	default: return FLinearColor(1.f, 0.7f, 0.12f);
	}
}

FLinearColor UTDStatics::TeamColor(ETDTeam Team)
{
	switch (Team)
	{
	case ETDTeam::Player: return FLinearColor(0.55f, 1.f, 0.7f);
	case ETDTeam::Enemy: return FLinearColor(1.f, 0.16f, 0.08f);
	default: return FLinearColor(0.5f, 0.5f, 0.5f);
	}
}

bool UTDStatics::ShouldDrawPlaceholders()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return true;
#endif
}

// ----------------------------------------------------------------------------------------------- feel

UTDFeelSubsystem* UTDFeelSubsystem::Get(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
	{
		return nullptr;
	}
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UTDFeelSubsystem>() : nullptr;
}

void UTDFeelSubsystem::RequestHitStop(float RealSeconds, float Dilation)
{
	const UWorld* World = GetWorld();
	if (!World || RealSeconds <= 0.f)
	{
		return;
	}
	const double Now = World->GetRealTimeSeconds();
	HitStopEnd = FMath::Max(HitStopEnd, Now + RealSeconds);
	HitStopDilation = FMath::Clamp(Dilation, 0.001f, 1.f);
	Apply();
}

void UTDFeelSubsystem::RequestSlowMo(float RealSeconds, float Dilation)
{
	const UWorld* World = GetWorld();
	if (!World || RealSeconds <= 0.f)
	{
		return;
	}
	const double Now = World->GetRealTimeSeconds();
	const float Clamped = FMath::Clamp(Dilation, 0.001f, 1.f);
	if (Now < SlowMoEnd)
	{
		// Overlapping requests: keep the slowest and the latest end.
		SlowMoDilation = FMath::Min(SlowMoDilation, Clamped);
		SlowMoEnd = FMath::Max(SlowMoEnd, Now + RealSeconds);
	}
	else
	{
		SlowMoDilation = Clamped;
		SlowMoEnd = Now + RealSeconds;
	}
	Apply();
}

void UTDFeelSubsystem::CancelAll()
{
	HitStopEnd = 0.0;
	SlowMoEnd = 0.0;
	Apply();
}

bool UTDFeelSubsystem::IsSlowMoActive() const
{
	const UWorld* World = GetWorld();
	return World && World->GetRealTimeSeconds() < SlowMoEnd;
}

void UTDFeelSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Apply();
}

TStatId UTDFeelSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDFeelSubsystem, STATGROUP_Tickables);
}

void UTDFeelSubsystem::Apply()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetRealTimeSeconds();
	float Desired = 1.f;
	if (Now < HitStopEnd)
	{
		Desired = FMath::Min(Desired, HitStopDilation);
	}
	if (Now < SlowMoEnd)
	{
		Desired = FMath::Min(Desired, SlowMoDilation);
	}
	if (!FMath::IsNearlyEqual(Desired, CurrentDilation, KINDA_SMALL_NUMBER))
	{
		CurrentDilation = Desired;
		if (AWorldSettings* Settings = World->GetWorldSettings())
		{
			Settings->SetTimeDilation(Desired);
		}
	}
}
