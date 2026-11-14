// Triad Drift - the space field adopts the galaxy layout's own rocks (Teammate 6; Lead's decision (a), Nov 14).
//
// Ryan: everything in space breaks. The galaxy layout (T4) places the combat belt, the flank field, the two planet
// approach fields, their small debris and the lane rocks; those were not destructible and soaked shots. The field now
// takes each of them over at load with the SAME mesh, transform and collision radius (so cover, lines of fire and the
// dogfight pocket are unchanged), reuses the layout's avoidance obstacle for each rock, and hides the layout's own
// components and switches their collision off. Nothing in TDGalaxyLayout changes: td.SpaceField 0 shows the layout's
// untouched components again, which is today's layout exactly.
//
// Cover: rocks of 20 m and up within 1 km of the combat zone take CoverHealthScale times their health, so a wave does
// not erase the cover; smaller rocks there break normally.

#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace TDSpaceFieldAdopt
{
	static FIntVector Key(const FVector& P) { return FIntVector(FMath::RoundToInt(P.X / 10.f), FMath::RoundToInt(P.Y / 10.f), FMath::RoundToInt(P.Z / 10.f)); }
}

void ATDSpaceField::AdoptLayoutRocks()
{
	using namespace TDSpaceFieldAdopt;
	if (!Layout || !bAdoptLayoutRocks || AdoptedLayoutComps.Num() > 0)
	{
		return;
	}
	UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this);
	const FVector Zone = Layout->GetActorTransform().TransformPosition(Layout->GetLayout().CombatZoneCenter);

	// The layout's collision spheres (hidden when its rocks carry their look) and its obstacles, by position.
	LayoutCollision = const_cast<UInstancedStaticMeshComponent*>(Layout->GetAsteroidCollision());
	TMap<FIntVector, float> ColliderRadius;
	if (LayoutCollision)
	{
		for (int32 i = 0; i < LayoutCollision->GetInstanceCount(); ++i)
		{
			FTransform T;
			LayoutCollision->GetInstanceTransform(i, T, /*bWorldSpace*/ true);
			ColliderRadius.Add(Key(T.GetLocation()), T.GetScale3D().X * 50.f);	// the engine sphere is 100 cm across
		}
	}
	TMap<FIntVector, int32> ObstacleAt;
	if (WorldSys)
	{
		const TArray<FSphere>& Obs = WorldSys->GetObstacles();
		for (int32 h = 0; h < Obs.Num(); ++h) { if (Obs[h].W >= 0.f) { ObstacleAt.Add(Key(Obs[h].Center), h); } }
	}

	// What to take over: the rock looks (they pair with a collision sphere at the same spot), the debris between them, and
	// the lane rocks (visual only in the layout; solid now, like every field piece).
	struct FSource { UInstancedStaticMeshComponent* Comp; bool bRock; };
	TArray<FSource> Sources;
	int32 RockLooks = 0;
	for (UInstancedStaticMeshComponent* C : Layout->GetAsteroidLooks()) { if (C) { Sources.Add({ C, true }); RockLooks += C->GetInstanceCount(); } }
	if (RockLooks == 0)
	{
		Sources.Reset();	// Nick's rock mesh is on the collision spheres themselves: leave the layout as it is
	}
	for (UInstancedStaticMeshComponent* C : Layout->GetDebrisLooks()) { if (C) { Sources.Add({ C, false }); } }
	for (UInstancedStaticMeshComponent* C : Layout->GetDressingComponents()) { if (C && C->GetName().StartsWith(TEXT("LaneRock"))) { Sources.Add({ C, false }); } }

	// T4's journey (Lead, Nov 14): a layout rock inside a route or keep-out, outside the pockets, moves sideways to just
	// outside it (its radius + 50 m), so rocks line the corridor edges (speed and scale) and none sits inside. td.SpaceField 0
	// keeps the layout's own positions.
	auto InPocket = [this](const FVector& P)
	{
		for (const FTDJourneyKeyPoint& Q : JourneyPoints) { if (Q.Kind == ETDJourneyPointKind::Pocket && Q.DistanceTo(P) < 0.f) { return true; } }
		return false;
	};
	auto ClearOfJourney = [this, &InPocket](FVector P, float Radius) -> FVector
	{
		for (int32 Pass = 0; Pass < 4; ++Pass)
		{
			if (InPocket(P)) { return P; }
			const FTDJourneyKeyPoint* Hit = nullptr;
			for (const FTDJourneyKeyPoint& K : JourneyPoints) { if (K.Kind != ETDJourneyPointKind::Pocket && K.DistanceTo(P) < Radius) { Hit = &K; break; } }
			if (!Hit) { return P; }
			const FVector C = Hit->Kind == ETDJourneyPointKind::Route
				? FMath::ClosestPointOnSegment(P, Hit->Location, Hit->Location + Hit->Direction * Hit->Length) : Hit->Location;
			FVector Out = P - C;
			if (Hit->Kind == ETDJourneyPointKind::Route) { Out -= Hit->Direction * FVector::DotProduct(Out, Hit->Direction); }
			if (Out.SizeSquared() < 1.f)
			{
				Out = FVector::CrossProduct(Hit->Kind == ETDJourneyPointKind::Route ? Hit->Direction : FVector::ForwardVector, FVector::UpVector);
				if (Out.SizeSquared() < 1e-4f) { Out = FVector::RightVector; }
			}
			P = C + Out.GetSafeNormal() * (Hit->Radius + Radius + 5000.f);
		}
		return P;
	};

	int32 Adopted = 0, Cover = 0, Moved = 0;
	for (const FSource& Src : Sources)
	{
		UStaticMesh* Mesh = Src.Comp->GetStaticMesh();
		if (!Mesh) { continue; }
		const float MeshR = FMath::Max(1.f, Mesh->GetBounds().SphereRadius);
		for (int32 i = 0; i < Src.Comp->GetInstanceCount(); ++i)
		{
			FTransform T;
			Src.Comp->GetInstanceTransform(i, T, /*bWorldSpace*/ true);
			const FVector Orig = T.GetLocation();
			const float* R = Src.bRock ? ColliderRadius.Find(Key(Orig)) : nullptr;
			const float Radius = R ? *R : MeshR * T.GetMaximumAxisScale();	// the rock's own sphere, or the piece's bounds
			const float Size = Radius * 2.f;
			const FVector At = ClearOfJourney(Orig, Radius);
			const bool bMoved = !At.Equals(Orig, 1.f);
			if (bMoved) { T.SetLocation(At); ++Moved; }

			UInstancedStaticMeshComponent* Visual = ComponentFor(Mesh, Size);
			const int32 Instance = Visual->AddInstance(T, /*bWorldSpace*/ true);
			FTDSpaceFieldPiece Piece;
			Piece.Location = At;
			Piece.Size = Size;
			Piece.bCollides = Src.bRock;
			Piece.Kind = Src.bRock ? ETDSpaceFieldKind::Belt : ETDSpaceFieldKind::Debris;
			Piece.bAdopted = true;
			Pieces.Add(Piece);
			// Exactly the layout's collision for its rocks (cover keeps its shape); its own obstacle, not a second one.
			PendingColliderRadius = R ? *R : Radius * 0.8f;
			const int32* Obstacle = Src.bRock ? ObstacleAt.Find(Key(Orig)) : nullptr;
			if (Obstacle && bMoved && WorldSys)
			{
				WorldSys->RemoveObstacle(*Obstacle);	// the old spot; RegisterPiece files a new one where the rock now is
				Obstacle = nullptr;
			}
			// Debris and lane rocks were visual only in the layout: solid now (every piece has a collision sphere), and those
			// of 10 m and up get an obstacle like every field piece, so the AI steers round them (smalls shatter on contact).
			PendingObstacle = Obstacle ? *Obstacle : INDEX_NONE;
			const int32 Index = RegisterPiece(Visual, Instance, Mesh, Size, false, INDEX_NONE, INDEX_NONE);
			PendingColliderRadius = -1.f;
			PendingObstacle = INDEX_NONE;
			if (Src.bRock && Size >= CoverMinSize && FVector::Dist(At, Zone) < CoverRadius)
			{
				States[Index].Health *= CoverHealthScale;
				++Cover;
			}
			++Adopted;
		}
		AdoptedLayoutComps.Add(Src.Comp);
	}
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: adopted %d layout rocks and pieces (%d cover rocks at x%.0f health; %d moved out of the journey's routes and keep-outs)"), Adopted, Cover, CoverHealthScale, Moved);
	AdoptedMovedOffJourney = Moved;
}

void ATDSpaceField::ShowLayoutRocks(bool bLayoutOwnsThem)
{
	// The field on = the layout's rocks hidden and not solid (the field's copies are); off = the layout exactly as today.
	// Flip both hidden flags and rebuild the render state of the layout's look components. Used by tests and internally;
	// td.SpaceField itself is read at level load only (0 at load never adopts, so it never needs this path).
	int32 Shown = 0, Instances = 0;
	for (const TWeakObjectPtr<UInstancedStaticMeshComponent>& C : AdoptedLayoutComps)
	{
		if (!C.IsValid()) { continue; }
		C->SetHiddenInGame(!bLayoutOwnsThem);
		C->SetVisibility(bLayoutOwnsThem);
		C->MarkRenderStateDirty();
		Shown += C->IsVisible() && !C->bHiddenInGame ? 1 : 0;
		Instances += C->GetInstanceCount();
	}
	if (LayoutCollision && AdoptedLayoutComps.Num() > 0)
	{
		LayoutCollision->SetCollisionEnabled(bLayoutOwnsThem ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: layout rocks %s: %d of %d components shown, %d instances, layout collision %d"), bLayoutOwnsThem ? TEXT("restored") : TEXT("taken over"),
		Shown, AdoptedLayoutComps.Num(), Instances, LayoutCollision ? (int32)LayoutCollision->IsCollisionEnabled() : -1);
}
