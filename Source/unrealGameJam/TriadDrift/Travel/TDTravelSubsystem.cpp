// Triad Drift - travel registry (ENG-15, ENG-20). Owner: Teammate 1.

#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Core/TDTypes.h"
#include "Engine/World.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Misc/PackageName.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flow/TDGameInstance.h"

UTDTravelSubsystem* UTDTravelSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDTravelSubsystem>() : nullptr;
}

UTDTravelFX* UTDTravelSubsystem::GetDefaultTravelFX(const UObject* WorldContextObject)
{
	UTDTravelSubsystem* Travel = Get(WorldContextObject);
	if (!Travel)
	{
		return nullptr;
	}
	if (!Travel->TravelFX && !Travel->bTravelFXLooked)
	{
		// Core's single presentation loader (quiet when missing, cooked path); held strongly per world.
		Travel->bTravelFXLooked = true;
		Travel->TravelFX = UTDStatics::LoadPresentation<UTDTravelFX>(TEXT("DA_Travel_FX"));
	}
	return Travel->TravelFX;
}

void UTDTravelSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Hyperdrive locks are owned by their owners (review MAJOR, Nov 12): a CUT retake must not clear them, or the
	// next take could jump away from the finale. Nothing to bind here.
}

void UTDTravelSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UTDTravelSubsystem::ClearAllHyperdriveLocks()
{
	HyperdriveLocks.Reset();
}

const UTDTravelSubsystem::FHyperdriveLock* UTDTravelSubsystem::ActiveHyperdriveLock() const
{
	for (int32 i = HyperdriveLocks.Num() - 1; i >= 0; --i)
	{
		if (!HyperdriveLocks[i].IsOrphaned())
		{
			return &HyperdriveLocks[i];
		}
	}
	return nullptr;
}

FText UTDTravelSubsystem::GetHyperdriveLockText() const
{
	const FHyperdriveLock* Lock = ActiveHyperdriveLock();
	return Lock ? Lock->Reason : FText::GetEmpty();
}

void UTDTravelSubsystem::RegisterGravityWell(FName Id, FVector Location, float Radius, AActor* Owner)
{
	FTDTravelGravityWell Well;
	Well.Id = Id;
	Well.Location = Location;
	Well.Radius = FMath::Max(0.f, Radius);
	Well.Owner = Owner;
	for (FTDTravelGravityWell& Existing : Wells)
	{
		if (Id != NAME_None && Existing.Id == Id)
		{
			Existing = Well;
			return;
		}
	}
	Wells.Add(Well);
}

void UTDTravelSubsystem::UnregisterGravityWell(FName Id)
{
	Wells.RemoveAll([Id](const FTDTravelGravityWell& W) { return W.Id == Id; });
}

bool UTDTravelSubsystem::FindWellContaining(const FVector& Location, FTDTravelGravityWell& OutWell) const
{
	float Best = TNumericLimits<float>::Max();
	bool bFound = false;
	for (const FTDTravelGravityWell& Well : Wells)
	{
		if (Well.Owner.IsStale())
		{
			continue; // owner destroyed
		}
		const float Dist = FVector::Dist(Well.GetLocation(), Location);
		if (Dist <= Well.Radius && Dist < Best)
		{
			Best = Dist;
			OutWell = Well;
			bFound = true;
		}
	}
	return bFound;
}

bool UTDTravelSubsystem::FindWellOnSegment(const FVector& From, const FVector& To, FTDTravelGravityWell& OutWell) const
{
	float BestAlong = TNumericLimits<float>::Max();
	bool bFound = false;
	for (const FTDTravelGravityWell& Well : Wells)
	{
		if (Well.Owner.IsStale())
		{
			continue;
		}
		const FVector P = Well.GetLocation();
		const FVector Closest = FMath::ClosestPointOnSegment(P, From, To);
		if (FVector::Dist(Closest, P) <= Well.Radius)
		{
			const float Along = FVector::Dist(From, Closest);
			if (Along < BestAlong)
			{
				BestAlong = Along;
				OutWell = Well;
				bFound = true;
			}
		}
	}
	return bFound;
}

void UTDTravelSubsystem::SetPlayBounds(FVector Center, float Radius)
{
	bHasPlayBounds = Radius > 0.f;
	PlayBoundsCenter = Center;
	PlayBoundsRadius = FMath::Max(0.f, Radius);
	++PlayBoundsVersion;
}

void UTDTravelSubsystem::ClearPlayBounds()
{
	bHasPlayBounds = false;
	++PlayBoundsVersion;
}

bool UTDTravelSubsystem::GetPlayBounds(FVector& OutCenter, float& OutRadius) const
{
	OutCenter = PlayBoundsCenter;
	OutRadius = PlayBoundsRadius;
	return bHasPlayBounds;
}

void UTDTravelSubsystem::AddHyperdriveLock(FName Owner, FText Reason, const UObject* OwnerObject)
{
	HyperdriveLocks.RemoveAll([Owner](const FHyperdriveLock& L) { return L.Owner == Owner || L.IsOrphaned(); });
	FHyperdriveLock& Lock = HyperdriveLocks.AddDefaulted_GetRef();
	Lock.Owner = Owner;
	Lock.Reason = Reason;
	Lock.OwnerObject = OwnerObject;
	Lock.bHasOwnerObject = OwnerObject != nullptr;
}

void UTDTravelSubsystem::RemoveHyperdriveLock(FName Owner)
{
	HyperdriveLocks.RemoveAll([Owner](const FHyperdriveLock& L) { return L.Owner == Owner || L.IsOrphaned(); });
}

bool UTDTravelSubsystem::IsHyperdriveLocked(FText& OutReason) const
{
	const FHyperdriveLock* Lock = ActiveHyperdriveLock();
	if (!Lock)
	{
		return false;
	}
	OutReason = Lock->Reason;
	return true;
}

bool UTDTravelSubsystem::CanRequestLevelTravel(FText* OutReason) const
{
	auto Fail = [OutReason](const TCHAR* Key, const TCHAR* Text)
	{
		if (OutReason) { *OutReason = FText::AsCultureInvariant(Text); }
		return false;
	};
	if (HasRequestedLevelTravel())
	{
		return Fail(TEXT("Requested"), TEXT("Travel already requested from this level"));
	}
	const UWorld* World = GetWorld();
	if (const UTDGameInstance* GI = World ? Cast<UTDGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (GI->IsTravelPending())
		{
			return Fail(TEXT("Pending"), TEXT("Travel already pending"));
		}
		if (GI->IsRetakePending())
		{
			return Fail(TEXT("Retake"), TEXT("Retake pending"));
		}
	}
	return true;
}

bool UTDTravelSubsystem::HasRequestedLevelTravel() const
{
	if (!bLevelTravelRequested)
	{
		return false;
	}
	// Still in this world after a request: it is under way while the flow says so; a voided or timed-out travel
	// (UTDGameInstance::FinishTravel(false)) frees it for another attempt.
	const UWorld* World = GetWorld();
	const UTDGameInstance* GI = World ? Cast<UTDGameInstance>(World->GetGameInstance()) : nullptr;
	return !GI || GI->IsTravelPending();
}

bool UTDTravelSubsystem::RequestLevelTravel(ETDDestination Destination, FName ReturnPoint)
{
	if (!CanRequestLevelTravel())
	{
		return false;
	}
	ITDGameFlow* Flow = UTDStatics::GetGameFlow(this);
	if (!Flow)
	{
		return false;
	}
	bLevelTravelRequested = Flow->TravelTo(Destination, ReturnPoint);
	return bLevelTravelRequested;
}

void UTDTravelSubsystem::RegisterPad(ATDLandingPad* Pad)
{
	if (Pad)
	{
		Pads.AddUnique(Pad);
	}
}

void UTDTravelSubsystem::UnregisterPad(ATDLandingPad* Pad)
{
	Pads.Remove(Pad);
}

void UTDTravelSubsystem::UpdatePadMarker(const UObject* Requester, ATDLandingPad* Wanted, bool bIsPlayerShip)
{
	if (!bIsPlayerShip)
	{
		if (MarkOwner.Get() != Requester)
		{
			return; // not ours to clear
		}
		Wanted = nullptr;
		MarkOwner.Reset();
	}
	else
	{
		MarkOwner = Requester;
	}
	ATDLandingPad* Old = MarkedPad.Get();
	if (Old == Wanted)
	{
		return;
	}
	if (Old)
	{
		Old->Tags.Remove(TDTags::ObjectiveMarker);
	}
	MarkedPad = Wanted;
	if (Wanted)
	{
		Wanted->Tags.AddUnique(TDTags::ObjectiveMarker);
	}
}

ATDLandingPad* UTDTravelSubsystem::FindPlayerPad(const FVector& Location, float MaxDistance) const
{
	ATDLandingPad* Best = nullptr;
	float BestDist = MaxDistance;
	for (const TWeakObjectPtr<ATDLandingPad>& Weak : Pads)
	{
		ATDLandingPad* Pad = Weak.Get();
		if (!Pad || Pad->IsWingmanPad() || Pad->IsOccupied() || !Pad->IsPadEnabled())
		{
			continue;
		}
		const float Dist = FVector::Dist(Pad->GetBeaconLocation(), Location);
		if (Dist <= BestDist)
		{
			BestDist = Dist;
			Best = Pad;
		}
	}
	return Best;
}

void UTDTravelSubsystem::FindWingmanPads(const ATDLandingPad* PlayerPad, int32 Count, TArray<ATDLandingPad*>& OutPads) const
{
	OutPads.Reset();
	if (!PlayerPad || Count <= 0)
	{
		return;
	}
	// Explicit side pads first.
	for (const TObjectPtr<ATDLandingPad>& Side : PlayerPad->GetSidePads())
	{
		if (Side && !Side->IsOccupied() && OutPads.Num() < Count)
		{
			OutPads.Add(Side);
		}
	}
	if (OutPads.Num() >= Count)
	{
		return;
	}
	// Then the nearest free wingman pads.
	TArray<TPair<float, ATDLandingPad*>> Candidates;
	for (const TWeakObjectPtr<ATDLandingPad>& Weak : Pads)
	{
		ATDLandingPad* Pad = Weak.Get();
		if (Pad && Pad != PlayerPad && Pad->IsWingmanPad() && !Pad->IsOccupied() && !OutPads.Contains(Pad))
		{
			const float Dist = FVector::Dist(Pad->GetActorLocation(), PlayerPad->GetActorLocation());
			if (Dist <= PlayerPad->GetWingmanPadSearchRadius())
			{
				Candidates.Emplace(Dist, Pad);
			}
		}
	}
	Candidates.Sort([](const TPair<float, ATDLandingPad*>& A, const TPair<float, ATDLandingPad*>& B) { return A.Key < B.Key; });
	for (const TPair<float, ATDLandingPad*>& C : Candidates)
	{
		if (OutPads.Num() >= Count)
		{
			break;
		}
		OutPads.Add(C.Value);
	}
}
