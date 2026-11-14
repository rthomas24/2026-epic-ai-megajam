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
	if (!Layout || !bAdoptLayoutRocks)
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

	int32 Adopted = 0, Cover = 0;
	for (const FSource& Src : Sources)
	{
		UStaticMesh* Mesh = Src.Comp->GetStaticMesh();
		if (!Mesh) { continue; }
		const float MeshR = FMath::Max(1.f, Mesh->GetBounds().SphereRadius);
		for (int32 i = 0; i < Src.Comp->GetInstanceCount(); ++i)
		{
			FTransform T;
			Src.Comp->GetInstanceTransform(i, T, /*bWorldSpace*/ true);
			const FVector At = T.GetLocation();
			const float* R = Src.bRock ? ColliderRadius.Find(Key(At)) : nullptr;
			const float Radius = R ? *R : MeshR * T.GetMaximumAxisScale();	// the rock's own sphere, or the piece's bounds
			const float Size = Radius * 2.f;

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
			const int32* Obstacle = Src.bRock ? ObstacleAt.Find(Key(At)) : nullptr;
			PendingObstacle = Obstacle ? *Obstacle : (Src.bRock ? INDEX_NONE : -2);
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
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: adopted %d layout rocks and pieces (%d cover rocks at x%.0f health)"), Adopted, Cover, CoverHealthScale);
}

void ATDSpaceField::ShowLayoutRocks(bool bLayoutOwnsThem)
{
	// The field on = the layout's rocks hidden and not solid (the field's copies are); off = the layout exactly as today.
	for (const TWeakObjectPtr<UInstancedStaticMeshComponent>& C : AdoptedLayoutComps)
	{
		if (C.IsValid()) { C->SetVisibility(bLayoutOwnsThem); }
	}
	if (LayoutCollision && AdoptedLayoutComps.Num() > 0)
	{
		LayoutCollision->SetCollisionEnabled(bLayoutOwnsThem ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}
