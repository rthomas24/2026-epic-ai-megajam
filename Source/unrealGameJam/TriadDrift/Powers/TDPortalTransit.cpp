// Triad Drift - Powers (Teammate 2). Shared portal transit.

#include "TriadDrift/Powers/TDPortalTransit.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDSpacecraft.h"

FVector UTDPortalTransit::MapLocation(const FVector& Location, const FTransform& Entry, const FTransform& Exit, float ExitForwardOffset)
{
	FVector Local = Entry.InverseTransformPositionNoScale(Location);
	Local.X = ExitForwardOffset;
	return Exit.TransformPositionNoScale(Local);
}

FVector UTDPortalTransit::MapDirection(const FVector& Direction, const FTransform& Entry, const FTransform& Exit)
{
	const FVector Local = Entry.InverseTransformVectorNoScale(Direction);
	return Exit.TransformVectorNoScale(Local);
}

bool UTDPortalTransit::CanTransit(const AActor* Actor, float CooldownSeconds)
{
	if (!IsValid(Actor))
	{
		return false;
	}
	if (const ATDProjectile* P = Cast<ATDProjectile>(Actor))
	{
		if (!P->IsActive())
		{
			return false;
		}
	}
	else if (!Actor->IsA<ATDSpacecraft>())
	{
		return false;
	}
	const UTDPowerSubsystem* Subsystem = UTDPowerSubsystem::Get(Actor);
	return !Subsystem || !Subsystem->IsInTransitCooldown(Actor, CooldownSeconds);
}

bool UTDPortalTransit::TransitActor(AActor* Actor, const FTransform& Entry, const FTransform& Exit, AActor* PortalInstigator,
	const FTDPortalTransitParams& Params)
{
	if (!CanTransit(Actor, Params.ReentryCooldownSeconds))
	{
		return false;
	}
	const FVector NewLocation = MapLocation(Actor->GetActorLocation(), Entry, Exit, Params.ExitForwardOffset);
	UTDEventBus* Bus = UTDEventBus::Get(Actor);

	if (ATDProjectile* P = Cast<ATDProjectile>(Actor))
	{
		const FVector NewDir = MapDirection(P->GetVelocity().GetSafeNormal(), Entry, Exit).GetSafeNormal();
		P->TeleportProjectile(NewLocation, NewDir);

		if (Params.bReturnEnemyProjectiles && UTDStatics::AreTeamsHostile(P->GetTeam(), ETDTeam::Player))
		{
			const bool bMissile = P->IsMissile();
			P->Deflect(PortalInstigator, NewDir, ETDTeam::Player, Params.ReturnSpeedMultiplier);
			if (bMissile)
			{
				if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(Actor))
				{
					Flow->AddStat(TEXT("MissilesReturned"), 1);
				}
				// Subject: the enemy it returns to (stable; the projectile is pooled). Director picks the importance.
				AActor* Subject = P->GetOriginalShooter() ? P->GetOriginalShooter() : P;
				if (UTDDirectorSubsystem* Director = UTDDirectorSubsystem::Get(Actor))
				{
					Director->ReportHighlight(ETDHighlightKind::MissileReturn, Subject, -1.f, PortalInstigator);
				}
				else if (Bus)
				{
					Bus->BroadcastHighlightMoment(ETDHighlightKind::MissileReturn, Subject, 0.55f);
				}
			}
		}
	}
	else if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Actor))
	{
		// Ships take the exit's facing (keeping roll upright to the exit's up).
		Craft->TeleportSpacecraft(NewLocation, Exit.GetRotation().Rotator(), Params.bKeepSpeed);
		if (UTDHealthComponent* Health = Craft->GetHealth())
		{
			Health->SetInvulnerableFor(Params.InvulnerableSeconds);
		}
		if (ATDEnemyShip* Enemy = Cast<ATDEnemyShip>(Craft))
		{
			Enemy->LoseTracking(Params.EnemyTrackingLostSeconds);
		}
		else if (Craft->IsPlayerControlled())
		{
			if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(Actor))
			{
				Flow->AddStat(TEXT("PortalsUsed"), 1);
			}
		}
	}
	else
	{
		return false;
	}

	if (UTDPowerSubsystem* Subsystem = UTDPowerSubsystem::Get(Actor))
	{
		Subsystem->MarkTransited(Actor);
	}
	return true;
}
