// Triad Drift - energy blade (ENG-22). Owner: Teammate 5.

#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"

namespace TDBlade
{
	/** Distance from P to the segment AB. */
	static float PointSegmentDistance(const FVector& P, const FVector& A, const FVector& B)
	{
		return FMath::PointDistToSegment(P, A, B);
	}

	/** Rough radius of an actor for reach checks (bounds, or 40 cm for bound-less stand-ins). */
	static float ActorRadius(const AActor* Actor)
	{
		FVector Origin, Extent;
		Actor->GetActorBounds(true, Origin, Extent);
		const float R = FVector2D(Extent.X, Extent.Y).Size();
		return R > 1.f ? FMath::Min(R, 200.f) : 40.f;
	}

}

UTDEnergyBladeComponent::UTDEnergyBladeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	FTDBladeSwing S1; S1.Damage = 25.f; S1.Range = 180.f; S1.ArcDegrees = 100.f; S1.Duration = 0.28f;
	FTDBladeSwing S2 = S1;
	FTDBladeSwing S3 = S1; S3.Damage = 45.f; S3.Duration = 0.36f; S3.Knockback = 900.f;
	LightCombo = { S1, S2, S3 };
	Stamina = BlockStaminaSeconds;
}

void UTDEnergyBladeComponent::BeginPlay()
{
	Super::BeginPlay();
	Stamina = BlockStaminaSeconds;
	if (!BladeFX && FPackageName::DoesPackageExist(TEXT("/Game/TriadDrift/Presentation/DA_Blade_FX")))
	{
		BladeFX = LoadObject<UTDBladeFX>(nullptr, TEXT("/Game/TriadDrift/Presentation/DA_Blade_FX.DA_Blade_FX"));
	}
	OwnerHealth = UTDStatics::GetHealth(GetOwner());
	if (OwnerHealth)
	{
		OwnerHealth->DamageFilter.BindUObject(this, &UTDEnergyBladeComponent::FilterIncomingDamage);
	}
	PlayBladeCue(TEXT("Ignite"), BladeFX ? &BladeFX->Ignite : nullptr, true, FVector::ZeroVector);
}

void UTDEnergyBladeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwnerHealth && OwnerHealth->DamageFilter.IsBoundToObject(this))
	{
		OwnerHealth->DamageFilter.Unbind();
	}
	Super::EndPlay(EndPlayReason);
}

void UTDEnergyBladeComponent::PlayBladeCue(FName Name, const FTDFXCue* Cue, bool bAttached, const FVector& Location)
{
	OnCueRequested.Broadcast(Name);
	if (!Cue || Cue->IsEmpty() || !GetOwner())
	{
		return;
	}
	if (bAttached)
	{
		UTDStatics::PlayCueAttached(*Cue, GetOwner()->GetRootComponent());
	}
	else
	{
		UTDStatics::PlayCue(this, *Cue, Location, GetOwner()->GetActorRotation());
	}
}

void UTDEnergyBladeComponent::Retract()
{
	if (bRetracted)
	{
		return;
	}
	CancelAll();
	bRetracted = true;
	PlayBladeCue(TEXT("Retract"), BladeFX ? &BladeFX->Retract : nullptr, true, FVector::ZeroVector);
	OnRetractChanged.Broadcast(true);
}

void UTDEnergyBladeComponent::Ignite()
{
	if (!bRetracted)
	{
		return;
	}
	bRetracted = false;
	PlayBladeCue(TEXT("Ignite"), BladeFX ? &BladeFX->Ignite : nullptr, true, FVector::ZeroVector);
	OnRetractChanged.Broadcast(false);
}

ACharacter* UTDEnergyBladeComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

ETDTeam UTDEnergyBladeComponent::GetOwnerTeam() const
{
	return UTDStatics::GetTeam(GetOwner());
}

void UTDEnergyBladeComponent::GetView(FVector& OutLocation, FVector& OutForward) const
{
	const AActor* Owner = GetOwner();
	FRotator Rot = FRotator::ZeroRotator;
	OutLocation = FVector::ZeroVector;
	if (Owner)
	{
		Owner->GetActorEyesViewPoint(OutLocation, Rot);
	}
	OutForward = Rot.Vector();
}

bool UTDEnergyBladeComponent::IsAttacking() const
{
	switch (State)
	{
	case ETDBladeState::Light1:
	case ETDBladeState::Light2:
	case ETDBladeState::Light3:
	case ETDBladeState::HeavyLunge:
	case ETDBladeState::AirSlam:
	case ETDBladeState::DashStrike:
		return true;
	default:
		return false;
	}
}

bool UTDEnergyBladeComponent::IsInFrontArc(const FVector& WorldLocation) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	FVector Eye, Forward;
	GetView(Eye, Forward);
	const FVector Flat = FVector(Forward.X, Forward.Y, 0.f).GetSafeNormal();
	FVector To = WorldLocation - Owner->GetActorLocation();
	To.Z = 0.f;
	if (To.IsNearlyZero() || Flat.IsNearlyZero())
	{
		return true; // directly above/below: give the player the benefit of the doubt
	}
	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(BlockArcDegrees * 0.5f));
	return FVector::DotProduct(Flat, To.GetSafeNormal()) >= CosHalf;
}

// ------------------------------------------------------------------------------------------------
// Input
// ------------------------------------------------------------------------------------------------

void UTDEnergyBladeComponent::PressAttack()
{
	bAttackHeld = true;
	AttackHeldTime = 0.f;
	bHeavyFiredThisPress = false;
}

void UTDEnergyBladeComponent::ReleaseAttack()
{
	if (!bAttackHeld)
	{
		return;
	}
	bAttackHeld = false;
	if (bHeavyFiredThisPress)
	{
		return;
	}
	// Quick press: what it does depends on what the body is doing.
	if (const ATDFirstPersonCharacter* FPC = Cast<ATDFirstPersonCharacter>(GetOwner()))
	{
		if (FPC->IsDashing())
		{
			DashStrike(FPC->GetDashDirection(), FPC->GetDashRemainingDistance() + 150.f);
			return;
		}
	}
	const ACharacter* Char = GetOwnerCharacter();
	if (Char && Char->GetCharacterMovement() && Char->GetCharacterMovement()->IsFalling())
	{
		AirSlam();
		return;
	}
	LightAttack();
}

void UTDEnergyBladeComponent::SetBlockHeld(bool bHeld)
{
	bBlockHeld = bHeld;
	if (bHeld)
	{
		if (!bRetracted && !IsAttacking() && State != ETDBladeState::GuardBroken && Stamina > 0.f && State != ETDBladeState::Blocking)
		{
			State = ETDBladeState::Blocking;
			BlockTime = 0.f;
			ComboIndex = 0;
			ComboTimeLeft = 0.f;
			bLightQueued = false;
			RecentlyDeflected.Reset();
		}
	}
	else if (State == ETDBladeState::Blocking)
	{
		State = ETDBladeState::Idle;
		TimeSinceBlock = 0.f;
	}
}

// ------------------------------------------------------------------------------------------------
// Moves
// ------------------------------------------------------------------------------------------------

bool UTDEnergyBladeComponent::LightAttack()
{
	if (bRetracted || LightCombo.Num() == 0 || State == ETDBladeState::GuardBroken || State == ETDBladeState::Blocking)
	{
		return false;
	}
	const bool bInLightSwing = State == ETDBladeState::Light1 || State == ETDBladeState::Light2 || State == ETDBladeState::Light3;
	if (bInLightSwing && SwingTimeLeft > 0.f)
	{
		// Buffer the press: it fires the moment this swing ends (arcade-responsive combos).
		bLightQueued = ComboIndex < LightCombo.Num();
		return bLightQueued;
	}
	if (IsAttacking())
	{
		return false; // mid-lunge or slam
	}
	const int32 Index = (ComboTimeLeft > 0.f && ComboIndex < LightCombo.Num()) ? ComboIndex : 0;
	StartSwing(Index);
	return true;
}

void UTDEnergyBladeComponent::StartSwing(int32 Index)
{
	const FTDBladeSwing& Swing = LightCombo[Index];
	ComboIndex = Index + 1;
	State = static_cast<ETDBladeState>(static_cast<uint8>(ETDBladeState::Light1) + FMath::Min(Index, 2));
	SwingTimeLeft = Swing.Duration;
	ComboTimeLeft = 0.f;
	bLightQueued = false;
	OnSwing.Broadcast(State);
	PlayBladeCue(TEXT("SwingLight"), BladeFX ? &BladeFX->SwingLight : nullptr, true, FVector::ZeroVector);
	HitArc(Swing.Range, Swing.ArcDegrees, Swing.Damage, Swing.Knockback);
}

bool UTDEnergyBladeComponent::HeavyLunge()
{
	if (bRetracted || HeavyCooldownLeft > 0.f || State == ETDBladeState::GuardBroken || IsAttacking())
	{
		return false;
	}
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	FVector Eye, Forward;
	GetView(Eye, Forward);
	const FVector Dir = FVector(Forward.X, Forward.Y, 0.f).GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return false;
	}
	State = ETDBladeState::HeavyLunge;
	HeavyCooldownLeft = HeavyCooldown;
	SwingTimeLeft = HeavyLungeDuration;
	ComboIndex = 0;
	ComboTimeLeft = 0.f;
	OnSwing.Broadcast(State);
	PlayBladeCue(TEXT("SwingHeavy"), BladeFX ? &BladeFX->SwingHeavy : nullptr, true, FVector::ZeroVector);

	// Stab the first hostile along the lunge path.
	const FVector Start = Owner->GetActorLocation();
	const FVector End = Start + Dir * HeavyLungeDistance;
	AActor* Best = nullptr;
	float BestAlong = TNumericLimits<float>::Max();
	if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		TArray<AActor*> Hostiles;
		Registry->GetHostilesOf(GetOwnerTeam(), Hostiles);
		for (AActor* H : Hostiles)
		{
			if (!H || H == Owner)
			{
				continue;
			}
			const FVector P = H->GetActorLocation();
			if (TDBlade::PointSegmentDistance(P, Start, End) <= HeavyHitRadius + TDBlade::ActorRadius(H))
			{
				const float Along = FVector::DotProduct(P - Start, Dir);
				if (Along >= -50.f && Along < BestAlong)
				{
					BestAlong = Along;
					Best = H;
				}
			}
		}
	}
	float TravelDistance = HeavyLungeDistance;
	if (Best)
	{
		// Stop just short of the victim instead of ploughing through it.
		TravelDistance = FMath::Clamp(BestAlong - TDBlade::ActorRadius(Best) - 40.f, 0.f, HeavyLungeDistance);
		if (ITDStaggerable* Guarded = Cast<ITDStaggerable>(Best))
		{
			Guarded->BreakGuard(Owner);
		}
		HitActor(Best, HeavyDamage, Best->GetActorLocation(), Dir, 0.f);
	}
	if (ATDFirstPersonCharacter* FPC = Cast<ATDFirstPersonCharacter>(Owner))
	{
		FPC->StartForcedMove(Dir * (TravelDistance / FMath::Max(HeavyLungeDuration, 0.01f)), HeavyLungeDuration, 0.f);
	}
	return true;
}

bool UTDEnergyBladeComponent::AirSlam()
{
	ACharacter* Char = GetOwnerCharacter();
	if (bRetracted || !Char || !Char->GetCharacterMovement() || !Char->GetCharacterMovement()->IsFalling() || IsAttacking() || State == ETDBladeState::GuardBroken)
	{
		return false;
	}
	State = ETDBladeState::AirSlam;
	SwingTimeLeft = 5.f; // ends on landing
	OnSwing.Broadcast(State);
	Char->LaunchCharacter(FVector(0.f, 0.f, -AirSlamSpeed), true, true);
	return true;
}

void UTDEnergyBladeComponent::NotifyOwnerLanded()
{
	if (State != ETDBladeState::AirSlam)
	{
		return;
	}
	AActor* Owner = GetOwner();
	const FVector Center = Owner->GetActorLocation();
	PlayBladeCue(TEXT("AirSlam"), BladeFX ? &BladeFX->AirSlam : nullptr, false, Center);
	if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		TArray<AActor*> Hostiles;
		Registry->GetHostilesOf(GetOwnerTeam(), Hostiles, true);
		const TArray<AActor*> Victims = Hostiles; // damage can unregister actors
		for (AActor* H : Victims)
		{
			if (H && FVector::Dist(H->GetActorLocation(), Center) <= AirSlamRadius + TDBlade::ActorRadius(H))
			{
				const FVector Dir = (H->GetActorLocation() - Center).GetSafeNormal2D();
				HitActor(H, AirSlamDamage, H->GetActorLocation(), Dir, 600.f);
			}
		}
	}
	State = ETDBladeState::Idle;
	SwingTimeLeft = 0.f;
}

bool UTDEnergyBladeComponent::DashStrike(FVector Direction, float Distance)
{
	AActor* Owner = GetOwner();
	const FVector Dir = Direction.GetSafeNormal2D();
	if (bRetracted || !Owner || Dir.IsNearlyZero() || State == ETDBladeState::GuardBroken || IsAttacking())
	{
		return false;
	}
	State = ETDBladeState::DashStrike;
	SwingTimeLeft = 0.2f;
	OnSwing.Broadcast(State);
	PlayBladeCue(TEXT("SwingLight"), BladeFX ? &BladeFX->SwingLight : nullptr, true, FVector::ZeroVector);
	const FVector Start = Owner->GetActorLocation();
	const FVector End = Start + Dir * FMath::Max(Distance, 0.f);
	if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		TArray<AActor*> Hostiles;
		Registry->GetHostilesOf(GetOwnerTeam(), Hostiles);
		const TArray<AActor*> Victims = Hostiles;
		for (AActor* H : Victims)
		{
			if (H && H != Owner && TDBlade::PointSegmentDistance(H->GetActorLocation(), Start, End) <= DashStrikeRadius + TDBlade::ActorRadius(H))
			{
				HitActor(H, DashStrikeDamage, H->GetActorLocation(), Dir, 0.f);
			}
		}
	}
	return true;
}

int32 UTDEnergyBladeComponent::HitArc(float Range, float ArcDegrees, float Damage, float Knockback)
{
	AActor* Owner = GetOwner();
	UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	if (!Owner || !Registry)
	{
		return 0;
	}
	FVector Eye, Forward;
	GetView(Eye, Forward);
	const FVector Flat = FVector(Forward.X, Forward.Y, 0.f).GetSafeNormal();
	const FVector Origin = Owner->GetActorLocation();
	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(ArcDegrees * 0.5f));

	TArray<AActor*> Hostiles;
	Registry->GetHostilesOf(GetOwnerTeam(), Hostiles);
	const TArray<AActor*> Candidates = Hostiles;
	int32 Hits = 0;
	for (AActor* H : Candidates)
	{
		if (!H || H == Owner)
		{
			continue;
		}
		FVector To = H->GetActorLocation() - Origin;
		const float Vertical = FMath::Abs(To.Z);
		To.Z = 0.f;
		const float Dist = To.Size() - TDBlade::ActorRadius(H);
		if (Dist > Range || Vertical > Range)
		{
			continue;
		}
		// Point-blank targets always count; otherwise they must be inside the arc.
		const bool bInArc = To.Size() < 30.f || FVector::DotProduct(Flat, To.GetSafeNormal()) >= CosHalf;
		if (bInArc && HitActor(H, Damage, H->GetActorLocation(), To.GetSafeNormal(), Knockback))
		{
			++Hits;
		}
	}
	return Hits;
}

bool UTDEnergyBladeComponent::HitActor(AActor* Victim, float Damage, const FVector& HitLocation, const FVector& HitDir, float Knockback)
{
	AActor* Owner = GetOwner();
	UTDHealthComponent* Health = UTDStatics::GetHealth(Victim);
	if (!Victim || !Health || !Health->IsAlive())
	{
		return false;
	}
	float Amount = Damage;
	if (bCritReady)
	{
		Amount *= CritMultiplier;
		bCritReady = false;
	}
	const float Dealt = UTDStatics::ApplyTDDamage(Victim, Amount, ETDDamageKind::Melee, Owner, Owner, HitLocation, HitDir);
	if (Dealt <= 0.f)
	{
		return false;
	}
	const bool bKill = Health->IsDead();

	if (Knockback > 0.f && !bKill)
	{
		if (ACharacter* VictimChar = Cast<ACharacter>(Victim))
		{
			VictimChar->LaunchCharacter(HitDir.GetSafeNormal2D() * Knockback + FVector(0.f, 0.f, 250.f), true, true);
		}
	}
	if (UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(this))
	{
		Feel->RequestHitStop(bKill ? HitStopOnKill : HitStopOnHit);
	}
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->BroadcastHitConfirmed(Victim, bKill);
	}
	PlayBladeCue(TEXT("Hit"), BladeFX ? &BladeFX->Hit : nullptr, false, HitLocation);
	OnBladeHit.Broadcast(Victim, Dealt, bKill);
	return true;
}

// ------------------------------------------------------------------------------------------------
// Block, deflect, parry
// ------------------------------------------------------------------------------------------------

bool UTDEnergyBladeComponent::TryDeflect(AActor* ProjectileActor)
{
	if (!IsBlocking() || !ProjectileActor)
	{
		return false;
	}
	ITDDeflectable* Deflectable = Cast<ITDDeflectable>(ProjectileActor);
	if (!Deflectable)
	{
		return false;
	}
	for (const TWeakObjectPtr<AActor>& Done : RecentlyDeflected)
	{
		if (Done.Get() == ProjectileActor)
		{
			return false;
		}
	}
	AActor* Owner = GetOwner();
	const bool bPerfect = IsInPerfectWindow();

	FVector Eye, Forward;
	GetView(Eye, Forward);
	FVector Dir = Forward;
	ATDProjectile* Proj = Cast<ATDProjectile>(ProjectileActor);
	if (bPerfect && Proj)
	{
		// Auto-aim back at whoever fired it.
		if (const AActor* Shooter = Proj->GetOriginalShooter())
		{
			Dir = (Shooter->GetActorLocation() - ProjectileActor->GetActorLocation()).GetSafeNormal();
		}
	}
	Deflectable->Deflect(Owner, Dir, GetOwnerTeam(), DeflectSpeedMultiplier);
	if (Proj)
	{
		const float Mult = bPerfect ? PerfectDeflectDamageMultiplier : NormalDeflectDamageMultiplier;
		Proj->SetDamageMultiplier(Proj->GetDamageMultiplier() * Mult);
		if (!bPerfect)
		{
			Proj->ClearHoming(); // Deflect re-homes on the shooter; a normal deflect flies along the crosshair
		}
	}
	RecentlyDeflected.Add(ProjectileActor);
	Stamina = FMath::Max(0.f, Stamina - BlockHitStaminaCost);

	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		Flow->AddStat(TEXT("ShotsDeflected"), 1);
	}
	if (bPerfect)
	{
		// Director's Cut money shot (C-03 #2): the director owns the slow-mo, camera, event and stat.
		bool bDirected = false;
		if (UTDDirectorSubsystem* Director = UTDDirectorSubsystem::Get(this))
		{
			bDirected = Director->ReportHighlight(ETDHighlightKind::PerfectDeflect, Owner, PerfectDeflectImportance, Proj ? Proj->GetOriginalShooter() : nullptr);
		}
		// Refused (rate limit, no director): still give the deflect its 0.15 s beat, except mid-CUT or travel.
		const UTDGameInstance* GI = UTDGameInstance::Get(this);
		const bool bFlowBusy = GI && (GI->IsRetakePending() || GI->IsTravelPending());
		if (!bDirected && !bFlowBusy)
		{
			if (UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(this))
			{
				Feel->RequestSlowMo(PerfectDeflectSlowMoSeconds, PerfectDeflectSlowMoDilation);
			}
		}
	}
	if (bPerfect)
	{
		PlayBladeCue(TEXT("PerfectDeflect"), BladeFX ? &BladeFX->PerfectDeflect : nullptr, false, ProjectileActor->GetActorLocation());
	}
	else
	{
		PlayBladeCue(TEXT("Deflect"), BladeFX ? &BladeFX->Deflect : nullptr, false, ProjectileActor->GetActorLocation());
	}
	OnDeflect.Broadcast(ProjectileActor, bPerfect);
	if (Stamina <= 0.f)
	{
		BreakGuard();
	}
	return true;
}

void UTDEnergyBladeComponent::ScanForProjectiles(float DeltaTime)
{
	UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(this);
	AActor* Owner = GetOwner();
	if (!Projectiles || !Owner)
	{
		return;
	}
	const ETDTeam MyTeam = GetOwnerTeam();
	const FVector Center = Owner->GetActorLocation();

	// Collect first: deflecting mutates projectile state.
	TArray<ATDProjectile*, TInlineAllocator<16>> Catch;
	for (const TObjectPtr<ATDProjectile>& P : Projectiles->GetLiveProjectiles())
	{
		if (!P || !P->IsActive() || !UTDStatics::AreTeamsHostile(P->GetTeam(), MyTeam))
		{
			continue;
		}
		const FVector Loc = P->GetActorLocation();
		const FVector Vel = P->GetVelocity();
		const FVector ToMe = Center - Loc;
		const float Dist = ToMe.Size();
		// Coming at us, and close enough to arrive within about a frame and a half.
		const float Reach = DeflectRadius + Vel.Size() * DeltaTime * 1.5f;
		if (Dist <= Reach && FVector::DotProduct(Vel, ToMe) > 0.f && IsInFrontArc(Loc))
		{
			Catch.Add(P);
		}
	}
	for (ATDProjectile* P : Catch)
	{
		if (!IsBlocking())
		{
			break; // guard broke mid-volley
		}
		TryDeflect(P);
	}
}

float UTDEnergyBladeComponent::FilterIncomingDamage(const FTDDamageInfo& Info)
{
	if (!IsBlocking() || Info.Kind == ETDDamageKind::Environmental)
	{
		return Info.Amount;
	}
	// Where did it come from?
	FVector From = Info.HitLocation;
	if (!Info.HitDirection.IsNearlyZero() && GetOwner())
	{
		From = GetOwner()->GetActorLocation() - Info.HitDirection.GetSafeNormal() * 100.f;
	}
	else if (Info.Instigator)
	{
		From = Info.Instigator->GetActorLocation();
	}
	if (!IsInFrontArc(From))
	{
		return Info.Amount; // flanked
	}

	if (Info.Kind == ETDDamageKind::Melee && IsInPerfectWindow())
	{
		// Parry: attacker reels, our next hit crits.
		if (ITDStaggerable* Attacker = Cast<ITDStaggerable>(Info.Instigator))
		{
			Attacker->ApplyStagger(ParryStaggerSeconds, GetOwner());
		}
		bCritReady = true;
		OnParry.Broadcast(Info.Instigator);
		PlayBladeCue(TEXT("PerfectDeflect"), BladeFX ? &BladeFX->PerfectDeflect : nullptr, false, Info.HitLocation);
		return 0.f;
	}

	Stamina = FMath::Max(0.f, Stamina - BlockHitStaminaCost);
	PlayBladeCue(TEXT("Block"), BladeFX ? &BladeFX->Block : nullptr, false, Info.HitLocation);
	if (Stamina <= 0.f)
	{
		BreakGuard();
	}
	return 0.f;
}

void UTDEnergyBladeComponent::BreakGuard()
{
	State = ETDBladeState::GuardBroken;
	GuardBrokenLeft = GuardBreakSeconds;
	Stamina = 0.f;
	TimeSinceBlock = 0.f;
	PlayBladeCue(TEXT("GuardBreak"), BladeFX ? &BladeFX->GuardBreak : nullptr, false, GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector);
	OnGuardBroken.Broadcast();
}

void UTDEnergyBladeComponent::CancelAll()
{
	State = ETDBladeState::Idle;
	SwingTimeLeft = 0.f;
	ComboIndex = 0;
	ComboTimeLeft = 0.f;
	bLightQueued = false;
	bAttackHeld = false;
	bBlockHeld = false;
}

// ------------------------------------------------------------------------------------------------
// Tick
// ------------------------------------------------------------------------------------------------

void UTDEnergyBladeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (DeltaTime <= 0.f)
	{
		return;
	}

	HeavyCooldownLeft = FMath::Max(0.f, HeavyCooldownLeft - DeltaTime);

	// Hold-to-heavy.
	if (bAttackHeld && !bHeavyFiredThisPress)
	{
		AttackHeldTime += DeltaTime;
		if (AttackHeldTime >= HeavyHoldSeconds)
		{
			bHeavyFiredThisPress = HeavyLunge() || true; // a held press never turns into a light swing
		}
	}

	// Current move.
	if (IsAttacking() && State != ETDBladeState::AirSlam)
	{
		SwingTimeLeft -= DeltaTime;
		if (SwingTimeLeft <= 0.f)
		{
			const bool bWasLight = State == ETDBladeState::Light1 || State == ETDBladeState::Light2 || State == ETDBladeState::Light3;
			State = ETDBladeState::Idle;
			SwingTimeLeft = 0.f;
			if (bWasLight)
			{
				ComboTimeLeft = ComboWindow;
				if (bLightQueued)
				{
					bLightQueued = false;
					StartSwing(ComboIndex < LightCombo.Num() ? ComboIndex : 0);
				}
			}
		}
	}
	else if (ComboTimeLeft > 0.f)
	{
		ComboTimeLeft -= DeltaTime;
		if (ComboTimeLeft <= 0.f)
		{
			ComboTimeLeft = 0.f;
			ComboIndex = 0;
		}
	}

	// Guard.
	if (State == ETDBladeState::GuardBroken)
	{
		GuardBrokenLeft -= DeltaTime;
		if (GuardBrokenLeft <= 0.f)
		{
			State = ETDBladeState::Idle;
			TimeSinceBlock = 0.f;
		}
	}
	else if (State == ETDBladeState::Idle && bBlockHeld)
	{
		SetBlockHeld(true); // re-raise after an attack or guard break ends while still holding
	}

	if (State == ETDBladeState::Blocking)
	{
		BlockTime += DeltaTime;
		Stamina -= DeltaTime;
		if (Stamina <= 0.f)
		{
			BreakGuard();
		}
		else
		{
			ScanForProjectiles(DeltaTime);
		}
	}
	else if (State != ETDBladeState::GuardBroken)
	{
		TimeSinceBlock += DeltaTime;
		if (TimeSinceBlock >= StaminaRegenDelay)
		{
			Stamina = FMath::Min(BlockStaminaSeconds, Stamina + StaminaRegenRate * DeltaTime);
		}
	}
}
