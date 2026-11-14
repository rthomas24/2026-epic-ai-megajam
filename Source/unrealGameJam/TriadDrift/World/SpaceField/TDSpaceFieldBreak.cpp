// Triad Drift - the space field, destructible pieces (Teammate 6). Ryan, Nov 13: "all things in space ... should be
// destructible ... shoot it and it will explode ... the harder you hit it, the more it will do damage to your ship ...
// you shouldn't be able to go through them."
//
// Every piece has a hidden collision sphere (BlockAll: ships, shots and hyperdrive sweeps all stop on it) and a health.
// Small pieces break and vanish, mediums split into 2-3 smalls, larges and wrecks into 2-3 mediums. A broken piece's
// collision sphere goes to a far graveyard at once (a ship re-sweeping the same frame passes) and its visual instance
// is shrunk to nothing; both slots are reused by later fragments, so nothing grows over a run. Each break plays a cue
// (UTDSpaceFieldFX, or a placeholder burst) and broadcasts OnFieldObjectDestroyed on the Core bus (T2: score, drops).

#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/SpaceField/TDSpaceFieldFX.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace TDSpaceFieldBreak
{
	/** Where dead collision spheres wait (far below the play sphere; the collision component is hidden). */
	static const FVector Graveyard(0.f, 0.f, -9000000.f);
	/** Collision sphere radius as a share of a piece's bounding diameter (a little inside the silhouette). */
	static constexpr float ColliderShare = 0.4f;
}

int32 ATDSpaceField::AddCollider(const FVector& At, float Size)
{
	using namespace TDSpaceFieldBreak;
	if (!Collision)
	{
		return INDEX_NONE;
	}
	const FTransform T(FRotator::ZeroRotator, At, FVector(FMath::Max(Size * ColliderShare, 10.f) / 50.f));
	if (FreeColliders.Num() > 0)
	{
		const int32 Index = FreeColliders.Pop(EAllowShrinking::No);
		Collision->UpdateInstanceTransform(Index, T, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		return Index;
	}
	return Collision->AddInstance(T, /*bWorldSpace*/ true);
}

float ATDSpaceField::HealthFor(float Size, bool bWreck) const
{
	if (bWreck)
	{
		return WreckHealth;
	}
	switch (SizeClassFor(Size))
	{
	case 0: return SmallHealth;
	case 1: return MediumHealth;
	default: return LargeHealthBase + LargeHealthPerMetre * Size / 100.f;
	}
}

int32 ATDSpaceField::RegisterPiece(UInstancedStaticMeshComponent* Visual, int32 Instance, UStaticMesh* Mesh, float Size, bool bSpecial, int32 Tumbler, int32 Wreck)
{
	const int32 Index = Pieces.Num() - 1;
	FPieceState S;
	S.Visual = Visual;
	S.Instance = Instance;
	S.Tumbler = Tumbler;
	S.Wreck = Wreck;
	S.Mesh = Mesh;
	S.SizeClass = Wreck != INDEX_NONE ? 2 : SizeClassFor(Size);
	S.Health = HealthFor(Size, Wreck != INDEX_NONE);
	S.bSpecial = bSpecial;
	S.Collider = AddCollider(Pieces[Index].Location, Size);
	States.SetNum(Pieces.Num());
	States[Index] = S;
	if (S.Collider != INDEX_NONE)
	{
		if (ColliderToPiece.Num() <= S.Collider) { ColliderToPiece.SetNum(S.Collider + 1); }
		ColliderToPiece[S.Collider] = Index;
	}
	Grid.FindOrAdd(CellOf(Pieces[Index].Location)).Add(Index);
	return Index;
}

int32 ATDSpaceField::PieceForHit(const UPrimitiveComponent* Component, int32 Item) const
{
	if (!Component || Component != Collision || !ColliderToPiece.IsValidIndex(Item))
	{
		return INDEX_NONE;
	}
	const int32 Piece = ColliderToPiece[Item];
	return IsPieceAlive(Piece) && States[Piece].Collider == Item ? Piece : INDEX_NONE;
}

int32 ATDSpaceField::GetAliveCount() const
{
	int32 N = 0;
	for (const FPieceState& S : States) { N += S.bAlive ? 1 : 0; }
	return N;
}

bool ATDSpaceField::GetColliderTransform(int32 Piece, FTransform& Out) const
{
	return IsPieceAlive(Piece) && Collision && Collision->GetInstanceTransform(States[Piece].Collider, Out, /*bWorldSpace*/ true);
}

bool ATDSpaceField::DamagePiece(int32 Piece, float Damage, AActor* ByWhom)
{
	if (!bEnabled || !IsPieceAlive(Piece) || Damage <= 0.f)
	{
		return false;
	}
	FPieceState& S = States[Piece];
	S.Health -= Damage;
	if (S.Health > 0.f)
	{
		return false;
	}
	BreakPiece(Piece, ByWhom);
	return true;
}

void ATDSpaceField::BreakPiece(int32 Piece, AActor* ByWhom)
{
	using namespace TDSpaceFieldBreak;
	FPieceState& S = States[Piece];
	const FTDSpaceFieldPiece P = Pieces[Piece];
	S.bAlive = false;
	S.Health = 0.f;
	++Broken;

	// Collision first: the caller (a ship's swept move) may re-sweep this frame.
	if (Collision && S.Collider != INDEX_NONE)
	{
		Collision->UpdateInstanceTransform(S.Collider, FTransform(FRotator::ZeroRotator, Graveyard + FVector(S.Collider * 10.f, 0.f, 0.f), FVector(0.01f)),
			/*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		FreeColliders.Add(S.Collider);
	}
	// The look.
	if (S.Wreck != INDEX_NONE && WreckComponents.IsValidIndex(S.Wreck) && WreckComponents[S.Wreck])
	{
		WreckComponents[S.Wreck]->SetVisibility(false);
		WreckHomes[S.Wreck].SetScale3D(FVector(0.001f));	// the drift tick keeps writing this transform
	}
	else if (S.Tumbler != INDEX_NONE && Tumblers.IsValidIndex(S.Tumbler))
	{
		Tumblers[S.Tumbler].Home.SetScale3D(FVector(0.001f));	// the tumble tick writes it next frame
	}
	else if (S.Visual && S.Instance != INDEX_NONE)
	{
		FTransform T;
		S.Visual->GetInstanceTransform(S.Instance, T, true);
		T.SetScale3D(FVector(0.001f));
		S.Visual->UpdateInstanceTransform(S.Instance, T, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
		FreeVisuals.FindOrAdd(S.Visual).Add(S.Instance);
	}
	if (TArray<int32>* Cell = Grid.Find(CellOf(P.Location)))
	{
		Cell->RemoveSingleSwap(Piece);
	}

	// Split: a medium into smalls, a large or a wreck into mediums.
	if (S.SizeClass > 0)
	{
		const bool bWreck = S.Wreck != INDEX_NONE;
		const float Real = bWreck ? P.Size - 6000.f : P.Size;	// a wreck's piece size includes its drift loop
		const float Cap = (S.SizeClass == 1 ? MediumFromSize : LargeFromSize) * 0.95f;
		const int32 N = BreakRng.RandRange(FMath::Max(1, SplitMin), FMath::Max(SplitMin, SplitMax));
		for (int32 i = 0; i < N; ++i)
		{
			const float Size = FMath::Clamp(Real * BreakRng.FRandRange(0.3f, 0.5f), 150.f, Cap);
			UStaticMesh* Mesh = S.Mesh;
			if (bWreck && Families.IsValidIndex(5) && Families[5].Num() > 0)
			{
				Mesh = Families[5][BreakRng.RandRange(0, Families[5].Num() - 1)];
			}
			else if (S.bSpecial && Families.IsValidIndex(0) && Families[0].Num() > 0)
			{
				Mesh = Families[0][BreakRng.RandRange(0, Families[0].Num() - 1)];	// the crystal heart is gone
			}
			const FVector At = P.Location + FVector(BreakRng.GetUnitVector()) * Real * BreakRng.FRandRange(0.2f, 0.45f);
			SpawnFragment(Mesh, At, Size);
		}
	}

	PlayBreakCue(P.Location, P.Size, S.SizeClass, S.bSpecial);
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->BroadcastFieldObjectDestroyed(P.Location, S.SizeClass, S.bSpecial, ByWhom);
	}
}

void ATDSpaceField::SpawnFragment(UStaticMesh* Mesh, const FVector& At, float Size)
{
	if (!Mesh)
	{
		return;
	}
	const FTransform T(FRotator(BreakRng.FRandRange(-180.f, 180.f), BreakRng.FRandRange(-180.f, 180.f), BreakRng.FRandRange(-180.f, 180.f)), At,
		FVector(Size * NormFor(Mesh)));
	UInstancedStaticMeshComponent* Visual = ComponentFor(Mesh, Size);
	int32 Instance = INDEX_NONE;
	if (TArray<int32>* Free = FreeVisuals.Find(Visual); Free && Free->Num() > 0)
	{
		Instance = Free->Pop(EAllowShrinking::No);
		Visual->UpdateInstanceTransform(Instance, T, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
	}
	else
	{
		Instance = Visual->AddInstance(T, /*bWorldSpace*/ true);
	}
	FTDSpaceFieldPiece Piece;
	Piece.Location = At;
	Piece.Size = Size;
	Piece.bCollides = false;	// fragments collide (every piece does) but are not avoidance obstacles
	Piece.Kind = ETDSpaceFieldKind::Debris;
	Pieces.Add(Piece);
	RegisterPiece(Visual, Instance, Mesh, Size, false, INDEX_NONE, INDEX_NONE);
}

void ATDSpaceField::PlayBreakCue(const FVector& At, float Size, uint8 SizeClass, bool bSpecial)
{
	if (!bFXLoaded)
	{
		bFXLoaded = true;
		if (!FX)
		{
			FX = UTDStatics::LoadPresentation<UTDSpaceFieldFX>(TEXT("DA_SpaceField_FX"));
		}
	}
	if (FX)
	{
		const FTDFXCue& Cue = bSpecial ? FX->BreakSpecial : (SizeClass == 0 ? FX->BreakSmall : (SizeClass == 1 ? FX->BreakMedium : FX->BreakLarge));
		if (!Cue.VFX.IsNull())
		{
			UTDStatics::PlayCue(this, Cue, At, FRotator::ZeroRotator);
			return;
		}
		if (!Cue.IsEmpty())
		{
			UTDStatics::PlayCue(this, Cue, At, FRotator::ZeroRotator);	// sound or shake only: still show the burst below
		}
	}
	// Placeholder: T2's explosion read (core, shell, debris), dusty orange for rock, teal for a special.
	if (UTDProjectileSubsystem* Sub = UTDProjectileSubsystem::Get(this))
	{
		const FLinearColor Color = bSpecial ? FLinearColor(0.2f, 1.f, 0.9f) : FLinearColor(1.f, 0.6f, 0.3f);
		Sub->PlayPlaceholderExplosion(At, FMath::Clamp(Size * 0.6f, 300.f, 6000.f), Color, SizeClass == 0 ? 4 : 8);
	}
}

void ATDSpaceField::GatherNear(const FVector& Min, const FVector& Max, TArray<int32>& Out) const
{
	// Pieces are filed by their centre: widen by the largest piece (a 150 m rock's centre can sit a cell away).
	const FIntVector A = CellOf(Min - FVector(8000.f));
	const FIntVector B = CellOf(Max + FVector(8000.f));
	const int64 Cells = int64(B.X - A.X + 1) * int64(B.Y - A.Y + 1) * int64(B.Z - A.Z + 1);
	if (Cells > 4096)
	{
		for (int32 i = 0; i < States.Num(); ++i) { if (States[i].bAlive) { Out.Add(i); } }
		return;
	}
	for (int32 X = A.X; X <= B.X; ++X)
	{
		for (int32 Y = A.Y; Y <= B.Y; ++Y)
		{
			for (int32 Z = A.Z; Z <= B.Z; ++Z)
			{
				if (const TArray<int32>* Cell = Grid.Find(FIntVector(X, Y, Z))) { Out.Append(*Cell); }
			}
		}
	}
}

bool ATDSpaceField::RamSweep(const FVector& From, const FVector& To, float Radius, AActor* ByWhom, FVector& OutBlock)
{
	using namespace TDSpaceFieldBreak;
	if (!bEnabled)
	{
		return false;
	}
	TArray<int32> Near;
	GatherNear(From.ComponentMin(To) - FVector(Radius), From.ComponentMax(To) + FVector(Radius), Near);
	struct FTouch { int32 Piece; float Along; };
	TArray<FTouch> Touched;
	const FVector Seg = To - From;
	const float LenSq = FMath::Max((float)Seg.SizeSquared(), 1.f);
	for (const int32 I : Near)
	{
		if (!IsPieceAlive(I)) { continue; }
		const FVector C = Pieces[I].Location;
		const float R = Pieces[I].Size * ColliderShare;
		if (FMath::PointDistToSegment(C, From, To) <= Radius + R)
		{
			Touched.Add({ I, (float)(FVector::DotProduct(C - From, Seg) / LenSq) });
		}
	}
	Touched.Sort([](const FTouch& A, const FTouch& B) { return A.Along < B.Along; });
	for (const FTouch& T : Touched)
	{
		if (States[T.Piece].SizeClass >= 2)
		{
			// A large rock or a wreck stops the dash at its surface (it still takes the blow).
			const FVector C = Pieces[T.Piece].Location;
			const float R = Pieces[T.Piece].Size * ColliderShare;
			const FVector Dir = Seg.GetSafeNormal();
			const float Along = FMath::Max(0.f, (float)FVector::DotProduct(C - From, Dir) - R - Radius);
			OutBlock = From + Dir * Along;
			DamagePiece(T.Piece, MediumHealth, ByWhom);
			return true;
		}
		BreakPiece(T.Piece, ByWhom);
	}
	return false;
}

void ATDSpaceField::Splash(const FVector& Center, float Radius, float Damage, AActor* ByWhom)
{
	using namespace TDSpaceFieldBreak;
	if (!bEnabled || Radius <= 0.f || Damage <= 0.f)
	{
		return;
	}
	TArray<int32> Near;
	GatherNear(Center - FVector(Radius), Center + FVector(Radius), Near);
	for (const int32 I : Near)
	{
		if (!IsPieceAlive(I)) { continue; }
		const float D = FMath::Max(0.f, (float)FVector::Dist(Center, Pieces[I].Location) - Pieces[I].Size * ColliderShare);
		if (D <= Radius)
		{
			DamagePiece(I, Damage * FMath::Max(0.25f, 1.f - D / Radius), ByWhom);
		}
	}
}

bool ATDSpaceField::ShipImpact(const UPrimitiveComponent* Component, int32 Item, float NormalSpeed, float HeadOn01, AActor* Ship, bool bRamming)
{
	const int32 Piece = PieceForHit(Component, Item);
	if (Piece == INDEX_NONE)
	{
		return false;
	}
	// Lancer's ram smashes smalls and mediums and keeps going.
	if (bRamming && States[Piece].SizeClass <= 1)
	{
		BreakPiece(Piece, Ship);
		return true;
	}
	// Anything else takes damage by the square of the impact speed; the ship's own damage is flight's (T1). Only a small
	// piece that shatters lets the ship through unhurt; a medium or large one still bounces and hurts the ship (Ryan: "the
	// harder you hit it, the more it will do damage to your ship"), even if it breaks.
	const uint8 Class = States[Piece].SizeClass;
	const float MetresPerSecond = NormalSpeed / 100.f;
	const bool bBroke = DamagePiece(Piece, ShipImpactDamageScale * MetresPerSecond * MetresPerSecond * FMath::Clamp(HeadOn01, 0.1f, 1.f), Ship);
	return bBroke && Class == 0;
}
