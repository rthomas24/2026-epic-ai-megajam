// Triad Drift - cover points for crew (ENG-23). Owner: Teammate 5.

#include "TriadDrift/Ground/Crew/TDCrewCover.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace TDCrewCoverImpl
{
	/** Eye height used for all cover sight checks (a crouched trooper's head vs the player's chest). */
	static constexpr float SightHeight = 70.f;
}

UTDCrewCoverSubsystem* UTDCrewCoverSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UTDCrewCoverSubsystem>() : nullptr;
}

void UTDCrewCoverSubsystem::RegisterCoverActor(AActor* Actor, float StandOff)
{
	if (!Actor)
	{
		return;
	}
	UnregisterCoverActor(Actor);
	FVector Origin, Extent;
	Actor->GetActorBounds(true, Origin, Extent);
	const float GroundZ = Origin.Z - Extent.Z;
	const FVector Faces[4] = { FVector::ForwardVector, -FVector::ForwardVector, FVector::RightVector, -FVector::RightVector };
	for (const FVector& N : Faces)
	{
		FTDCoverPoint P;
		const float Out = FMath::Abs(N.X) > 0.5f ? Extent.X : Extent.Y;
		const float Along = FMath::Abs(N.X) > 0.5f ? Extent.Y : Extent.X;
		P.Normal = N;
		P.Tangent = FVector::CrossProduct(FVector::UpVector, N);
		P.Location = FVector(Origin.X, Origin.Y, GroundZ) + N * (Out + StandOff);
		P.PeekReach = Along + StandOff;
		P.Owner = Actor;
		Points.Add(P);
	}
}

void UTDCrewCoverSubsystem::UnregisterCoverActor(AActor* Actor)
{
	Points.RemoveAll([Actor](const FTDCoverPoint& P) { return !P.Owner.IsValid() || P.Owner.Get() == Actor; });
}

bool UTDCrewCoverSubsystem::Blocked(const FVector& From, const FVector& To, const AActor* IgnoreA, const AActor* IgnoreB) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDCoverLOS), false);
	if (IgnoreA) { Params.AddIgnoredActor(IgnoreA); }
	if (IgnoreB) { Params.AddIgnoredActor(IgnoreB); }
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params);
}

int32 UTDCrewCoverSubsystem::FindCover(const AActor* Seeker, const AActor* Threat, float MaxTravel, float MinThreatDistance, float MaxThreatDistance) const
{
	if (!Seeker || !Threat)
	{
		return INDEX_NONE;
	}
	const FVector Me = Seeker->GetActorLocation();
	const FVector Them = Threat->GetActorLocation();
	int32 Best = INDEX_NONE;
	float BestScore = TNumericLimits<float>::Max();
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const FTDCoverPoint& P = Points[i];
		if (!P.IsValid() || (P.ClaimedBy.IsValid() && P.ClaimedBy.Get() != Seeker))
		{
			continue;
		}
		const float Travel = FVector::Dist2D(Me, P.Location);
		const float ToThreat = FVector::Dist2D(P.Location, Them);
		if (Travel > MaxTravel || ToThreat < MinThreatDistance || ToThreat > MaxThreatDistance)
		{
			continue;
		}
		if (!IsProtected(i, Threat, Seeker))
		{
			continue;
		}
		// Prefer close points that are near the middle of the preferred band.
		const float Score = Travel + FMath::Abs(ToThreat - 0.5f * (MinThreatDistance + MaxThreatDistance)) * 0.5f;
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = i;
		}
	}
	return Best;
}

bool UTDCrewCoverSubsystem::Claim(int32 Index, AActor* By)
{
	if (!Points.IsValidIndex(Index) || !Points[Index].IsValid())
	{
		return false;
	}
	FTDCoverPoint& P = Points[Index];
	if (P.ClaimedBy.IsValid() && P.ClaimedBy.Get() != By)
	{
		return false;
	}
	Release(By);
	P.ClaimedBy = By;
	return true;
}

void UTDCrewCoverSubsystem::Release(AActor* By)
{
	for (FTDCoverPoint& P : Points)
	{
		if (P.ClaimedBy.Get() == By)
		{
			P.ClaimedBy = nullptr;
		}
	}
}

bool UTDCrewCoverSubsystem::IsProtected(int32 Index, const AActor* Threat, const AActor* Ignore) const
{
	const FTDCoverPoint* P = GetPoint(Index);
	if (!P || !P->IsValid() || !Threat)
	{
		return false;
	}
	const FVector Eye = P->Location + FVector(0.f, 0.f, TDCrewCoverImpl::SightHeight);
	return Blocked(Threat->GetActorLocation(), Eye, Threat, Ignore);
}

bool UTDCrewCoverSubsystem::FindPeekLocation(int32 Index, const AActor* Threat, const AActor* Ignore, FVector& OutLocation) const
{
	const FTDCoverPoint* P = GetPoint(Index);
	if (!P || !P->IsValid() || !Threat)
	{
		return false;
	}
	for (const float Side : { 1.f, -1.f })
	{
		const FVector Peek = P->Location + P->Tangent * Side * P->PeekReach;
		if (!Blocked(Peek + FVector(0.f, 0.f, TDCrewCoverImpl::SightHeight), Threat->GetActorLocation(), Threat, Ignore))
		{
			OutLocation = Peek;
			return true;
		}
	}
	return false;
}
