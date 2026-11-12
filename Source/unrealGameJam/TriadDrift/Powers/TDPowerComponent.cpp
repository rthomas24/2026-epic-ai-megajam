// Triad Drift - Powers (Teammate 2). Power base.

#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"

UTDPowerComponent::UTDPowerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTDPowerComponent::BeginPlay()
{
	Super::BeginPlay();
	// Nick's data assets by convention, if nobody assigned one.
	if (!FX && GetAbilityId() != ETDAbilityId::None)
	{
		FX = FTDPowerPresentation::LoadAbilityFX(GetAbilityId());
	}
	ETDShipId Ship;
	if (!ShipVisuals && GetOwnerShipId(Ship))
	{
		ShipVisuals = FTDPowerPresentation::LoadShipVisuals(Ship);
	}
	if (UTDPowerSubsystem* Subsystem = UTDPowerSubsystem::Get(this))
	{
		ResetHandle = Subsystem->OnPowersReset.AddUObject(this, &UTDPowerComponent::HandlePowersReset);
	}
}

void UTDPowerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDPowerSubsystem* Subsystem = UTDPowerSubsystem::Get(this))
	{
		Subsystem->OnPowersReset.Remove(ResetHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UTDPowerComponent::HandlePowersReset()
{
	EndPower(true);
	CooldownRemaining = 0.f;
}

bool UTDPowerComponent::CanActivate() const
{
	if (bPowerActive || CooldownRemaining > 0.f)
	{
		return false;
	}
	if (bRequirePlayerControl && !IsOwnerPlayerControlled())
	{
		return false;
	}
	const UTDHealthComponent* Health = UTDStatics::GetHealth(GetOwner());
	return !Health || Health->IsAlive();
}

bool UTDPowerComponent::TryActivate()
{
	if (!CanActivate() || !OnActivate())
	{
		return false;
	}
	bPowerActive = ActiveSeconds > 0.f || bEndsManually;
	ActiveElapsed = 0.f;
	// Sustained powers with an end-started cooldown get it in EndPower.
	CooldownRemaining = (bCooldownStartsWhenPowerEnds && bPowerActive) ? 0.f : CooldownSeconds;

	ETDShipId Ship;
	if (GetOwnerShipId(Ship))
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(this))
		{
			Bus->BroadcastPowerUsed(Ship, PowerName);
		}
	}
	PlayActivationCues();
	OnPowerActivated.Broadcast(this);

	if (!bPowerActive)
	{
		// Instant power: ends the moment it starts.
		OnEnd(false);
		OnPowerEnded.Broadcast(this, false);
	}
	return true;
}

void UTDPowerComponent::EndPower(bool bInterrupted)
{
	if (!bPowerActive)
	{
		return;
	}
	bPowerActive = false;
	if (bCooldownStartsWhenPowerEnds)
	{
		CooldownRemaining = CooldownSeconds;
	}
	OnEnd(bInterrupted);
	if (FX)
	{
		if (const AActor* Owner = GetOwner())
		{
			FTDPowerPresentation::RequestCue(this, TEXT("Ability.End"), FX->End, Owner->GetActorLocation(), Owner->GetActorRotation());
		}
	}
	PlayShipCue(2);
	OnPowerEnded.Broadcast(this, bInterrupted);
}

void UTDPowerComponent::PlayActivationCues()
{
	if (FX)
	{
		if (const AActor* Owner = GetOwner())
		{
			FTDPowerPresentation::RequestCue(this, TEXT("Ability.Activate"), FX->Activate, Owner->GetActorLocation(), Owner->GetActorRotation());
		}
	}
	PlayShipCue(1);
}

void UTDPowerComponent::PlayShipCue(int32 Which)
{
	const AActor* Owner = GetOwner();
	if (!ShipVisuals || !Owner || Slot == ETDPowerInput::Ability)
	{
		return;
	}
	const bool bSecond = Slot == ETDPowerInput::Signature2;
	const FTDFXCue& Cue = Which == 0 ? ShipVisuals->PowerWindup
		: (Which == 2 ? ShipVisuals->PowerEnd : (bSecond ? ShipVisuals->Power2Active : ShipVisuals->PowerActive));
	const FName Moment = Which == 0 ? FName(TEXT("Ship.PowerWindup")) : (Which == 2 ? FName(TEXT("Ship.PowerEnd"))
		: (bSecond ? FName(TEXT("Ship.Power2Active")) : FName(TEXT("Ship.PowerActive"))));
	FTDPowerPresentation::RequestCueAttached(Moment, Cue, Which == 1 ? GetActiveCueRoot() : Owner->GetRootComponent());
}

USceneComponent* UTDPowerComponent::GetActiveCueRoot() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetRootComponent() : nullptr;
}

bool UTDPowerComponent::HasActiveArt() const
{
	if (FX && (FTDPowerPresentation::HasArt(FX->Activate) || FTDPowerPresentation::HasArt(FX->Loop)))
	{
		return true;
	}
	if (ShipVisuals && Slot != ETDPowerInput::Ability)
	{
		return FTDPowerPresentation::HasArt(Slot == ETDPowerInput::Signature2 ? ShipVisuals->Power2Active : ShipVisuals->PowerActive);
	}
	return false;
}

float UTDPowerComponent::GetCooldown01() const
{
	if (bPowerActive && bCooldownStartsWhenPowerEnds)
	{
		return 0.f;
	}
	return CooldownSeconds > 0.f ? 1.f - FMath::Clamp(CooldownRemaining / CooldownSeconds, 0.f, 1.f) : 1.f;
}

void UTDPowerComponent::HandleInputPressed()
{
	TryActivate();
}

void UTDPowerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const bool bCooldownFrozen = bPowerActive && bCooldownStartsWhenPowerEnds;
	if (!bCooldownFrozen && CooldownRemaining > 0.f)
	{
		CooldownRemaining = FMath::Max(0.f, CooldownRemaining - DeltaTime);
	}
	if (bPowerActive && bEndOnOwnerDeath)
	{
		const UTDHealthComponent* Health = UTDStatics::GetHealth(GetOwner());
		if (Health && Health->IsDead())
		{
			EndPower(true);
		}
	}
	// Switch away mid-power (before OnTickActive, so a ram wind-up never turns into a dash under AI).
	const bool bPlayerNow = IsOwnerPlayerControlled();
	if (bPowerActive && bWasPlayerControlled && !bPlayerNow)
	{
		OnLostPlayerControl();
	}
	bWasPlayerControlled = bPlayerNow;
	if (bPowerActive)
	{
		ActiveElapsed += DeltaTime;
		OnTickActive(DeltaTime);
		if (bPowerActive && ActiveSeconds > 0.f && ActiveElapsed >= ActiveSeconds)
		{
			EndPower(false);
		}
	}
}

UTDPowerComponent* UTDPowerComponent::FindPower(const AActor* Ship, ETDPowerInput InSlot)
{
	if (!Ship)
	{
		return nullptr;
	}
	TInlineComponentArray<UTDPowerComponent*> Powers(Ship);
	for (UTDPowerComponent* Power : Powers)
	{
		if (Power && Power->Slot == InSlot)
		{
			return Power;
		}
	}
	return nullptr;
}

bool UTDPowerComponent::RoutePowerInput(AActor* Ship, ETDPowerInput InSlot, bool bPressed)
{
	UTDPowerComponent* Power = FindPower(Ship, InSlot);
	if (!Power)
	{
		return false;
	}
	bPressed ? Power->HandleInputPressed() : Power->HandleInputReleased();
	return true;
}

bool UTDPowerComponent::IsOwnerPlayerControlled() const
{
	if (const ATDShip* Ship = Cast<ATDShip>(GetOwner()))
	{
		if (Ship->IsPlayerFlown())
		{
			return true;
		}
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn && Pawn->IsPlayerControlled();
}

bool UTDPowerComponent::GetOwnerShipId(ETDShipId& OutShip) const
{
	if (const ATDShip* Ship = Cast<ATDShip>(GetOwner()))
	{
		OutShip = Ship->GetShipId();
		return true;
	}
	return false;
}

ETDTeam UTDPowerComponent::GetOwnerTeam() const
{
	return UTDStatics::GetTeam(GetOwner());
}

void UTDPowerComponent::Highlight(ETDHighlightKind Kind, AActor* Subject, float Importance01) const
{
	// Only through the director (slow-mo, rate limits, camera hints, CUT / travel gating).
	if (UTDDirectorSubsystem* Director = UTDDirectorSubsystem::Get(this))
	{
		Director->ReportHighlight(Kind, Subject, Importance01, GetOwner());
	}
}

void UTDPowerComponent::AddStat(FName StatName, int32 Amount) const
{
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		Flow->AddStat(StatName, Amount);
	}
}
