#include "TriadDrift/Core/TDHealthComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDDamageTypes.h"
#include "TriadDrift/Core/TDEventBus.h"

UTDHealthComponent::UTDHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UTDHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Hull = MaxHull;
	Shield = MaxShield;
	bDead = false;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddUniqueDynamic(this, &UTDHealthComponent::HandleAnyDamage);
		if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
		{
			Registry->Register(Owner);
		}
	}
}

void UTDHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.RemoveDynamic(this, &UTDHealthComponent::HandleAnyDamage);
		if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
		{
			Registry->Unregister(Owner);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UTDHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDead || DeltaTime <= 0.f)
	{
		return;
	}

	TimeSinceDamage += DeltaTime;
	InvulnerableTimeRemaining = FMath::Max(0.f, InvulnerableTimeRemaining - DeltaTime);
	ShieldSuppressedRemaining = FMath::Max(0.f, ShieldSuppressedRemaining - DeltaTime);

	if (ShieldRegenFractionPerSecond > 0.f && Shield < MaxShield && ShieldSuppressedRemaining <= 0.f
		&& TimeSinceDamage >= ShieldRegenDelay)
	{
		Shield = FMath::Min(MaxShield, Shield + MaxShield * ShieldRegenFractionPerSecond * DeltaTime);
	}

	if (HullRegenPerSecond > 0.f && Hull < MaxHull && TimeSinceDamage >= HullRegenDelay)
	{
		Hull = FMath::Min(MaxHull, Hull + HullRegenPerSecond * DeltaTime);
	}
}

float UTDHealthComponent::ApplyDamage(const FTDDamageInfo& InInfo)
{
	if (bDead || InInfo.Amount <= 0.f || IsInvulnerable())
	{
		return 0.f;
	}

	FTDDamageInfo Info = InInfo;
	Info.Amount *= IncomingDamageMultiplier;

	if (DamageFilter.IsBound())
	{
		Info.Amount = DamageFilter.Execute(Info);
	}
	if (Info.Amount <= 0.f)
	{
		return 0.f;
	}

	float Remaining = Info.Amount;
	float ShieldDamage = 0.f;
	const bool bShieldWasUp = Shield > 0.f;

	if (!Info.bIgnoreShield && Shield > 0.f)
	{
		ShieldDamage = FMath::Min(Shield, Remaining);
		Shield -= ShieldDamage;
		Remaining -= ShieldDamage;
	}

	const float HullDamage = FMath::Min(Hull, Remaining);
	Hull -= HullDamage;

	TimeSinceDamage = 0.f;
	if (Info.Instigator)
	{
		LastAttacker = Info.Instigator;
	}

	OnDamaged.Broadcast(this, Info, ShieldDamage, HullDamage);
	if (bShieldWasUp && Shield <= 0.f)
	{
		OnShieldBroken.Broadcast(this, Info);
	}

	if (Hull <= 0.f)
	{
		Die(Info);
	}
	return ShieldDamage + HullDamage;
}

void UTDHealthComponent::Kill(AActor* Killer, ETDDamageKind Kind)
{
	if (bDead)
	{
		return;
	}
	FTDDamageInfo Info;
	Info.Amount = Hull + Shield;
	Info.Kind = Kind;
	Info.Instigator = Killer;
	Info.Causer = Killer;
	Info.bIgnoreShield = true;
	Shield = 0.f;
	Hull = 0.f;
	Die(Info);
}

void UTDHealthComponent::Die(const FTDDamageInfo& KillingBlow)
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	Hull = 0.f;

	OnDeath.Broadcast(this, KillingBlow);

	AActor* Owner = GetOwner();
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->BroadcastActorKilled(Owner, KillingBlow.Instigator, KillingBlow.Kind);
	}
	if (Owner && DestroyOwnerAfterDeathSeconds >= 0.f)
	{
		Owner->SetLifeSpan(FMath::Max(DestroyOwnerAfterDeathSeconds, 0.01f));
	}
}

void UTDHealthComponent::Revive(float HullFraction, float ShieldFraction)
{
	const bool bWasDead = bDead;
	bDead = false;
	Hull = FMath::Clamp(HullFraction, 0.01f, 1.f) * MaxHull;
	Shield = FMath::Clamp(ShieldFraction, 0.f, 1.f) * MaxShield;
	TimeSinceDamage = 1000.f;
	ShieldSuppressedRemaining = 0.f;
	if (bWasDead)
	{
		OnRevived.Broadcast(this);
	}
}

void UTDHealthComponent::RepairHull(float Amount)
{
	if (!bDead && Amount > 0.f)
	{
		Hull = FMath::Min(MaxHull, Hull + Amount);
	}
}

void UTDHealthComponent::RestoreShield(float Amount)
{
	if (!bDead && Amount > 0.f && ShieldSuppressedRemaining <= 0.f)
	{
		Shield = FMath::Min(MaxShield, Shield + Amount);
	}
}

void UTDHealthComponent::SuppressShield(float Seconds)
{
	if (bDead || Seconds <= 0.f)
	{
		return;
	}
	const bool bWasUp = Shield > 0.f;
	Shield = 0.f;
	ShieldSuppressedRemaining = FMath::Max(ShieldSuppressedRemaining, Seconds);
	if (bWasUp)
	{
		FTDDamageInfo Info;
		Info.Kind = ETDDamageKind::Energy;
		OnShieldBroken.Broadcast(this, Info);
	}
}

void UTDHealthComponent::InitMaxValues(float NewMaxHull, float NewMaxShield, bool bRefill)
{
	MaxHull = FMath::Max(1.f, NewMaxHull);
	MaxShield = FMath::Max(0.f, NewMaxShield);
	if (bRefill)
	{
		Hull = MaxHull;
		Shield = MaxShield;
	}
	else
	{
		Hull = FMath::Min(Hull, MaxHull);
		Shield = FMath::Min(Shield, MaxShield);
	}
}

void UTDHealthComponent::SetInvulnerableFor(float Seconds)
{
	InvulnerableTimeRemaining = FMath::Max(InvulnerableTimeRemaining, Seconds);
}

void UTDHealthComponent::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	// Damage that arrived through the engine's ApplyDamage path (not UTDStatics::ApplyTDDamage).
	FTDDamageInfo Info;
	Info.Amount = Damage;
	Info.Kind = UTDDamageType::KindOf(DamageType);
	Info.Instigator = (InstigatedBy && InstigatedBy->GetPawn()) ? static_cast<AActor*>(InstigatedBy->GetPawn()) : DamageCauser;
	Info.Causer = DamageCauser;
	if (DamagedActor)
	{
		Info.HitLocation = DamagedActor->GetActorLocation();
	}
	ApplyDamage(Info);
}
