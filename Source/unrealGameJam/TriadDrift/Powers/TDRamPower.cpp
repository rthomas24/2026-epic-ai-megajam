// Triad Drift - Powers (Teammate 2). Lancer: Afterburner Ram.

#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Combat/TDCombatPerf.h"
#include "TriadDrift/Combat/TDFieldDamageable.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDFlightTypes.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

namespace
{
	/**
	 * Distance from the segment From-To to Actor's colliding box in its own (oriented) frame, 0 inside. Two closest-point
	 * passes (both convex). The oriented box hugs a rotated carrier; its world AABB's corners reached past mounted parts.
	 */
	float TDRamDistanceSegmentToBounds(const AActor* Actor, const FVector& From, const FVector& To)
	{
		FBox Local = Actor->CalculateComponentsBoundingBoxInLocalSpace(false, false);
		if (!Local.IsValid || Local.GetExtent().IsNearlyZero())
		{
			// No colliding bounds (a hull-mounted part such as a turret): its visible box, so the part is reached before
			// the hull it sits on stops the dash.
			Local = Actor->CalculateComponentsBoundingBoxInLocalSpace(true, false);
		}
		if (!Local.IsValid || Local.GetExtent().IsNearlyZero())
		{
			// Nothing drawn either: a 50 cm ball, as TDRamBoundsRadius.
			return FMath::Max(0.f, FMath::PointDistToSegment(Actor->GetActorLocation(), From, To) - 50.f);
		}
		const FTransform& X = Actor->GetActorTransform();
		const FVector A = X.InverseTransformPosition(From);
		const FVector B = X.InverseTransformPosition(To);
		FVector OnSeg = FMath::ClosestPointOnSegment(Local.GetCenter(), A, B);
		for (int32 Pass = 0; Pass < 2; ++Pass)
		{
			OnSeg = FMath::ClosestPointOnSegment(Local.GetClosestPointTo(OnSeg), A, B);
		}
		return FVector::Dist(X.TransformPosition(OnSeg), X.TransformPosition(Local.GetClosestPointTo(OnSeg)));
	}

	/**
	 * Distance from the segment to the real collision of a big hull (its blocking primitives: the UCX hull or the
	 * blocking sphere), 0 touching. T4 (Rodin carrier): the art box sits well outside the hull surface where bays and
	 * turrets are now socket-mounted, so the box stopped the dash before the parts were in reach. Falls back to the box
	 * when no primitive answers (no physics body yet).
	 */
	float TDRamDistanceSegmentToHull(const AActor* Actor, const FVector& From, const FVector& To)
	{
		float Best = -1.f;
		TInlineComponentArray<UPrimitiveComponent*> Prims(Actor);
		for (const UPrimitiveComponent* Prim : Prims)
		{
			if (!Prim || !Prim->IsCollisionEnabled() || Prim->GetCollisionResponseToChannel(TDCollision::Ship) != ECR_Block)
			{
				continue;
			}
			// Alternate closest points between the segment and the primitive (convex enough for a hull's UCX pieces).
			FVector OnSeg = FMath::ClosestPointOnSegment(Prim->Bounds.Origin, From, To);
			FVector OnPrim;
			float D = -1.f;
			for (int32 Pass = 0; Pass < 3; ++Pass)
			{
				D = Prim->GetClosestPointOnCollision(OnSeg, OnPrim);
				if (D < 0.f)
				{
					break;
				}
				if (D == 0.f)
				{
					break; // the segment point is inside: touching
				}
				OnSeg = FMath::ClosestPointOnSegment(OnPrim, From, To);
			}
			if (D >= 0.f)
			{
				D = D == 0.f ? 0.f : FVector::Dist(OnSeg, OnPrim);
				Best = Best < 0.f ? D : FMath::Min(Best, D);
			}
		}
		return Best >= 0.f ? Best : TDRamDistanceSegmentToBounds(Actor, From, To);
	}

	float TDRamBoundsRadius(const AActor* Actor, FVector& OutCenter)
	{
		FVector Extent;
		Actor->GetActorBounds(true, OutCenter, Extent, false);
		if (Extent.IsNearlyZero())
		{
			OutCenter = Actor->GetActorLocation();
			return 50.f;
		}
		return Extent.GetMax();
	}
}

const FName UTDRamPower::RamOverrideKey(TEXT("Ram"));

UTDRamPower::UTDRamPower()
{
	Slot = ETDPowerInput::Signature1;
	PowerName = TEXT("AfterburnerRam");
	CooldownSeconds = 7.f;
	bCooldownStartsWhenPowerEnds = false;
	ActiveSeconds = 0.3f + 1.2f;
	bEndOnOwnerDeath = true;
}

float UTDRamPower::GetWindup01() const
{
	if (!IsPowerActive() || WindupSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(ActiveElapsed / WindupSeconds, 0.f, 1.f);
}

float UTDRamPower::GetBoostSpeed() const
{
	if (const ATDSpacecraft* Craft = Cast<ATDSpacecraft>(GetOwner()))
	{
		if (const UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			return Move->GetFlightTuning().BoostSpeed;
		}
	}
	return FallbackBoostSpeed;
}

float UTDRamPower::GetDashSpeed() const
{
	return GetBoostSpeed() * BoostSpeedMultiplier;
}

bool UTDRamPower::IsHyperdriveBusy() const
{
	if (const ATDShip* Ship = Cast<ATDShip>(GetOwner()))
	{
		if (const UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
		{
			return Hyper->GetState() != ETDHyperdriveState::Idle;
		}
	}
	return false;
}

bool UTDRamPower::CanActivate() const
{
	// Never mid-jump. Other overrides (atmosphere entry...) are fine: keyed overrides stack and ours is
	// removed alone at the end, restoring whatever was underneath.
	return Super::CanActivate() && !IsHyperdriveBusy();
}

void UTDRamPower::RefreshInvulnerability(float Seconds)
{
	// Timed and refreshed, never the untimed flag (TD.God and cinematics own that one).
	if (UTDHealthComponent* Health = UTDStatics::GetHealth(GetOwner()))
	{
		Health->SetInvulnerableFor(Seconds);
	}
}

bool UTDRamPower::OnActivate()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	ActiveSeconds = WindupSeconds + DashSeconds;
	HitThisDash.Reset();
	bSaidRamKill = false;
	bDashStarted = false;
	bStoppedByImpact = false;
	DashDistance = 0.f;
	PostDashGrace = 0.f;
	RefreshInvulnerability(0.2f);
	if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Owner))
	{
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			Move->OnFlightImpact.AddUniqueDynamic(this, &UTDRamPower::HandleFlightImpact);
		}
	}
	if (WindupSeconds <= 0.f)
	{
		BeginDash();
	}
	return true;
}

void UTDRamPower::SetShipCollisionIgnored(bool bIgnore)
{
	// Lancer smashes THROUGH fighters: other ships stop blocking its movement for the dash.
	if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(GetOwner() ? GetOwner()->GetRootComponent() : nullptr))
	{
		Prim->SetCollisionResponseToChannel(TDCollision::Ship, bIgnore ? ECR_Ignore : ECR_Block);
		bIgnoringShips = bIgnore;
	}
}

void UTDRamPower::BeginDash()
{
	AActor* Owner = GetOwner();
	bDashStarted = true;
	LastLocation = Owner->GetActorLocation();
	DashDirection = Owner->GetActorForwardVector();
	SetShipCollisionIgnored(true);
	PlayShipCue(1);
	if (!HasActiveArt() && !PlaceholderCone)
	{
		// Lancer's afterburner spear: a cyan cone ahead of the nose (engine cone axis is +Z: pitch -90 points it forward).
		PlaceholderCone = FTDPowerPresentation::AddShape(Owner, nullptr, FTDPowerPresentation::EShape::Cone, FVector(5.f, 5.f, 9.f),
			FRotator(-90.f, 0.f, 0.f), UTDStatics::ShipColor(ETDShipId::Lancer), 5.f);
		if (PlaceholderCone)
		{
			PlaceholderCone->SetRelativeLocation(FVector(700.f, 0.f, 0.f));
			// The chase camera sits behind Lancer, so the cone alone hides behind the hull: two long afterburner
			// streaks trailing off the wingtips read from behind (cylinder axis is +Z: pitch 90 lays it along X).
			for (const float Side : { -1.f, 1.f })
			{
				if (UStaticMeshComponent* Streak = FTDPowerPresentation::AddShape(Owner, PlaceholderCone, FTDPowerPresentation::EShape::Cylinder,
					FVector(0.35f, 0.35f, 14.f), FRotator(0.f, 0.f, 0.f), UTDStatics::ShipColor(ETDShipId::Lancer), 8.f))
				{
					Streak->ComponentTags.Remove(FTDPowerPresentation::PlaceholderTag); // part of the cone placeholder
					Streak->SetUsingAbsoluteScale(true);
					Streak->SetWorldScale3D(FVector(0.35f, 0.35f, 14.f));
					Streak->SetRelativeLocation(FVector(0.f, Side * 380.f / 5.f, -1400.f / 9.f)); // cone space: Z is backwards along the hull
				}
			}
		}
	}
	if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Owner))
	{
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			Move->SetSpeedOverrideFor(RamOverrideKey, GetDashSpeed(), SteeringScale, DashAcceleration);
			// The flight model's velocity grip eases speed changes in over ~0.2 s; the ram is a kick.
			Move->Velocity = Owner->GetActorForwardVector() * GetDashSpeed();
			bOwnsOverride = true;
		}
	}
}

void UTDRamPower::OnTickActive(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner || IsHyperdriveBusy())
	{
		EndPower(true);
		return;
	}
	RefreshInvulnerability(0.2f);
	if (!bDashStarted)
	{
		if (ActiveElapsed < WindupSeconds)
		{
			return;
		}
		BeginDash();
	}

	// Owners without the flight model (odd actors) are driven straight along the nose.
	if (!Owner->IsA<ATDSpacecraft>())
	{
		Owner->SetActorLocation(Owner->GetActorLocation() + Owner->GetActorForwardVector() * GetDashSpeed() * DeltaTime);
	}

	const FVector Now = Owner->GetActorLocation();
	// A portal / gate moved us this frame: restart the sweep at the new spot (and keep dashing).
	if (FVector::DistSquared(Now, LastLocation) > FMath::Square(GetDashSpeed() * DeltaTime * 3.f + 500.f))
	{
		LastLocation = Now;
		DashDirection = Owner->GetActorForwardVector();
		return;
	}
	if (!Now.Equals(LastLocation))
	{
		DashDirection = (Now - LastLocation).GetSafeNormal();
	}
	DashDistance += FVector::Dist(Now, LastLocation);
	SweepHits(LastLocation, Now);
	LastLocation = Owner->GetActorLocation();
}

void UTDRamPower::SweepHits(const FVector& From, const FVector& To)
{
	AActor* Owner = GetOwner();
	UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	if (!Owner || !Registry)
	{
		return;
	}
	FVector OwnerCenter;
	const float Reach = TDRamBoundsRadius(Owner, OwnerCenter) + HitPadding;

	struct FTouch
	{
		TWeakObjectPtr<AActor> Actor;
		float Along = 0.f;
	};
	TArray<FTouch, TInlineAllocator<16>> Touched;
	{
		TArray<AActor*>& Hostiles = HostilesScratch;
		Hostiles.Reset();
		Registry->GetHostilesOf(GetOwnerTeam(), Hostiles, true);
		const FVector Seg = To - From;
		const double SegLenSq = FMath::Max(Seg.SizeSquared(), (double)KINDA_SMALL_NUMBER);
		for (AActor* Hostile : Hostiles)
		{
			if (!IsValid(Hostile) || HitThisDash.Contains(Hostile))
			{
				continue;
			}
			// Inside the old bounding sphere AND within reach of the hull's own oriented box: passing a long hull's short
			// side no longer "touches" its long axis (second review), and nothing is reached earlier than before.
			FVector Center;
			const float Radius = TDRamBoundsRadius(Hostile, Center);
			const float ToSurface = IsBig(Hostile) ? TDRamDistanceSegmentToHull(Hostile, From, To) : TDRamDistanceSegmentToBounds(Hostile, From, To);
			if (FMath::PointDistToSegment(Center, From, To) <= Reach + Radius && ToSurface <= Reach)
			{
				Touched.Add({ Hostile, (float)(FVector::DotProduct(Center - From, Seg) / SegLenSq) });
			}
		}
	}
	// In path order, so a big ship stops the dash before anything behind it.
	Touched.Sort([](const FTouch& A, const FTouch& B) { return A.Along < B.Along; });
	for (const FTouch& T : Touched)
	{
		if (!IsPowerActive())
		{
			return;
		}
		if (AActor* Victim = T.Actor.Get())
		{
			HitActor(Victim, DashDirection);
		}
	}
	// T6's destructible field: the dash smashes small and medium rocks along its path, looking a little ahead so they
	// break before the flight model touches them; a large one stops the dash where the field says.
	if (IsPowerActive())
	{
		TArray<AActor*> Fields;
		FTDFieldDamage::GetFields(GetWorld(), Fields);
		const FVector Ahead = To + DashDirection * (Reach + GetDashSpeed() * .05f);
		for (AActor* Field : Fields)
		{
			FVector Block;
			if (Cast<ITDFieldDamageable>(Field)->ReceiveRamSweep(From, Ahead, Reach, Owner, Block))
			{
				StopDash(Block, Field);
				return;
			}
		}
	}
}

void UTDRamPower::HitActor(AActor* Victim, const FVector& Direction)
{
	if (!IsValid(Victim))
	{
		return;
	}
	AActor* Owner = GetOwner();
	HitThisDash.Add(Victim);

	if (ATDEnemyShip* Enemy = Cast<ATDEnemyShip>(Victim))
	{
		if (Enemy->HasFrontalShield())
		{
			Enemy->BreakFrontalShield();
		}
	}
	if (ATDCarrierBay* Bay = Cast<ATDCarrierBay>(Victim))
	{
		Bay->BreakDoors();
	}

	const UTDHealthComponent* Health = UTDStatics::GetHealth(Victim);
	const bool bWasAlive = !Health || Health->IsAlive();
	// Switch Strike's +25% applies to powers too (second review minor).
	const ATDSquadManager* Squad = ATDSquadManager::Get(this);
	const float Strike = Squad ? Squad->GetDamageMultiplier(Owner) : 1.f;
	UTDStatics::ApplyTDDamage(Victim, RamDamage * Strike, ETDDamageKind::Ram, Owner, Owner, Victim->GetActorLocation(), Direction, false);
	// RamKill money shot: UTDDirectorSubsystem derives it from the Ram damage kind on OnActorKilled.
	if (bWasAlive && Health && Health->IsDead())
	{
		if (UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(this); Feel && !WasStartedByAI()) // #201: no hit-stop for a wingman's ram kill
		{
			Feel->RequestHitStop();
		}
		// dialogs.md Bark_Ram_Kill: "Through and through." once per dash (a dash through three fighters says it once).
		if (!bSaidRamKill)
		{
			bSaidRamKill = true;
			FTDCombatBarks::SayAsPilot(Owner, TDBarkIdsT2::RamKill, ETDBarkPriority::High);
		}
	}

	if (IsBig(Victim))
	{
		// Stop at first contact, not wherever the frame ended (we may be inside the hull by now).
		FVector OwnerCenter;
		const float OwnerRadius = TDRamBoundsRadius(Owner, OwnerCenter);
		const FVector Here = Owner->GetActorLocation();
		// The hull's own parts at the point of impact (a bay or turret on its socket, flush with the hull) take the ram
		// too: at a fine frame step the hull is reached a frame before the part beside it, and the dash ends there.
		{
			TArray<AActor*> Mounted;
			Victim->GetAttachedActors(Mounted, true, true);
			const float PartReach = OwnerRadius + HitPadding;
			for (AActor* Part : Mounted)
			{
				if (IsValid(Part) && !HitThisDash.Contains(Part) && !IsBig(Part) && UTDStatics::GetHealth(Part)
					&& UTDStatics::AreTeamsHostile(GetOwnerTeam(), UTDStatics::GetTeam(Part))
					&& TDRamDistanceSegmentToBounds(Part, Here, Here + Direction * MountedPartGrace) <= PartReach)
				{
					HitActor(Part, Direction);
				}
			}
		}
		const float Back = FMath::Max(0.f, OwnerRadius - TDRamDistanceSegmentToHull(Victim, Here, Here)); // to the hull's real collision
		if (Back > 0.f)
		{
			Owner->SetActorLocation(Owner->GetActorLocation() - Direction * Back);
		}
		StopDash(Owner->GetActorLocation() + Direction * 200.f, Victim);
	}
}

void UTDRamPower::HandleFlightImpact(const FTDFlightImpact& Impact)
{
	if (!IsDashing())
	{
		return;
	}
	// Ships are ignored during the dash, so anything the flight model still hits is world geometry.
	AActor* Other = Impact.OtherActor.Get();
	if (Other && Other->IsA<ATDSpacecraft>())
	{
		// A big hostile hull still blocks (a carrier's socket-fitted collision): crashing into it IS the ram, which also
		// takes its mounted parts at the impact. At 30 fps one dash step (12 m) reached the hull beside a turret before
		// the sweep reached either, and the dash slid off along the hull with nothing hit.
		if (!HitThisDash.Contains(Other) && IsBig(Other) && UTDStatics::AreTeamsHostile(GetOwnerTeam(), UTDStatics::GetTeam(Other)))
		{
			HitActor(Other, DashDirection);
		}
		return;
	}
	// The flight model touched T6's field before the sweep broke it: the field decides (a small or medium rock breaks and
	// the dash goes on; a large one stops it).
	if (Other && Other->Implements<UTDFieldDamageable>())
	{
		FVector OwnerCenter;
		const float Reach = TDRamBoundsRadius(GetOwner(), OwnerCenter) + HitPadding;
		FVector Block;
		if (!Cast<ITDFieldDamageable>(Other)->ReceiveRamSweep(Impact.ImpactPoint - DashDirection * Reach, Impact.ImpactPoint + DashDirection * Reach,
			Reach, GetOwner(), Block))
		{
			return;
		}
		StopDash(Block, Other);
		return;
	}
	// A hostile part the flight model hits first (carrier bay, turret, shield generator: BlockAllDynamic) takes the
	// full ram before the dash stops: otherwise the shockwave spares it as the stopper and it gets nothing (review).
	if (Other && !HitThisDash.Contains(Other) && UTDStatics::GetHealth(Other)
		&& UTDStatics::AreTeamsHostile(GetOwnerTeam(), UTDStatics::GetTeam(Other)))
	{
		HitActor(Other, DashDirection);
		if (!IsDashing())
		{
			return; // HitActor already stopped the dash
		}
	}
	StopDash(Impact.ImpactPoint, Other);
}

void UTDRamPower::StopDash(const FVector& Where, AActor* Stopper)
{
	bStoppedByImpact = true;
	AActor* Owner = GetOwner();
	// The shockwave spares whatever stopped us: it already took the full 300.
	UTDStatics::ApplyTDRadialDamage(this, Where, ShockwaveRadius, ShockwaveDamage * (ATDSquadManager::Get(this) ? ATDSquadManager::Get(this)->GetDamageMultiplier(Owner) : 1.f), ETDDamageKind::Explosive, GetOwnerTeam(), Owner, Owner, false, Stopper);
	EndPower(false);
}

bool UTDRamPower::IsBig(const AActor* Actor) const
{
	if (const ATDEnemyShip* Enemy = Cast<ATDEnemyShip>(Actor))
	{
		return static_cast<uint8>(Enemy->GetSize()) >= static_cast<uint8>(StopOnSize);
	}
	return false; // bays, turrets and unknown actors are smashed through
}

void UTDRamPower::OnEnd(bool bInterrupted)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	if (bIgnoringShips)
	{
		SetShipCollisionIgnored(false);
	}
	if (PlaceholderCone)
	{
		FTDPowerPresentation::DestroyPlaceholder(PlaceholderCone); // the streaks are its children: they go too
		PlaceholderCone = nullptr;
	}
	if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Owner))
	{
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			Move->OnFlightImpact.RemoveAll(this);
			if (bOwnsOverride)
			{
				Move->ClearSpeedOverrideFor(RamOverrideKey); // only ours: anything underneath is restored
			}
		}
	}
	bOwnsOverride = false;
	bDashStarted = false;
	// Stay safe while the speed bleeds off (head-on hits at 45 000 cm/s are lethal) - unless the player switched away:
	// the AI-flown ship gets no grace.
	if (bEndingForControlLoss)
	{
		PostDashGrace = 0.f;
		if (UTDHealthComponent* Health = UTDStatics::GetHealth(GetOwner()))
		{
			Health->ClearTimedInvulnerability();
		}
		return;
	}
	PostDashGrace = MaxPostDashInvulnerableSeconds;
	RefreshInvulnerability(0.2f);
}

void UTDRamPower::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	TD_COMBAT_PERF(Powers);
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsPowerActive() && PostDashGrace > 0.f)
	{
		PostDashGrace -= DeltaTime;
		const AActor* Owner = GetOwner();
		const UTDHealthComponent* Health = UTDStatics::GetHealth(Owner);
		if (!Owner || (Health && Health->IsDead()) || Owner->GetVelocity().Size() <= GetBoostSpeed() * 1.02f)
		{
			PostDashGrace = 0.f;
			return;
		}
		RefreshInvulnerability(0.1f);
	}
}
