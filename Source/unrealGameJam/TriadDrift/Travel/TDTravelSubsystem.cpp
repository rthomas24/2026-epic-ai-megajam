// Triad Drift - travel registry (ENG-15, ENG-20). Owner: Teammate 1.

#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Travel/TDLandingPad.h"
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

UTDTravelFX* UTDTravelSubsystem::GetDefaultTravelFX()
{
	static TWeakObjectPtr<UTDTravelFX> Cached;
	static bool bLooked = false;
	if (!Cached.IsValid() && !bLooked)
	{
		bLooked = true;
		static const TCHAR* Package = TEXT("/Game/TriadDrift/Presentation/DA_Travel_FX");
		if (FPackageName::DoesPackageExist(Package))
		{
			Cached = Cast<UTDTravelFX>(StaticLoadObject(UTDTravelFX::StaticClass(), nullptr,
				TEXT("/Game/TriadDrift/Presentation/DA_Travel_FX.DA_Travel_FX"), nullptr, LOAD_NoWarn | LOAD_Quiet));
		}
	}
	return Cached.Get();
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

void UTDTravelSubsystem::AddHyperdriveLock(FName Owner, FText Reason)
{
	HyperdriveLocks.RemoveAll([Owner](const TPair<FName, FText>& L) { return L.Key == Owner; });
	HyperdriveLocks.Emplace(Owner, Reason);
}

void UTDTravelSubsystem::RemoveHyperdriveLock(FName Owner)
{
	HyperdriveLocks.RemoveAll([Owner](const TPair<FName, FText>& L) { return L.Key == Owner; });
}

bool UTDTravelSubsystem::IsHyperdriveLocked(FText& OutReason) const
{
	if (HyperdriveLocks.Num() == 0)
	{
		return false;
	}
	OutReason = HyperdriveLocks.Last().Value;
	return true;
}

bool UTDTravelSubsystem::CanRequestLevelTravel(FText* OutReason) const
{
	auto Fail = [OutReason](const TCHAR* Key, const TCHAR* Text)
	{
		if (OutReason) { *OutReason = FText::AsCultureInvariant(Text); }
		return false;
	};
	if (bLevelTravelRequested)
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
