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
#include "NiagaraComponent.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "NiagaraSystem.h"
#include "TriadDrift/Combat/TDFieldDamageable.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "TriadDrift/Perf/TDWarmupSubsystem.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Powers/TDRamPower.h"

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
	const float Radius = PendingColliderRadius > 0.f ? PendingColliderRadius : FMath::Max(Size * ColliderShare, 10.f);
	const FTransform T(FRotator::ZeroRotator, At, FVector(Radius / 50.f));
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
	default: return FMath::Min(LargeHealthBase + LargeHealthPerMetre * Size / 100.f, LargeHealthMax);	// 14-20 bolts
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
	// Wingmen and enemies steer round everything 10 m and up (FindAvoidance, gridded): smalls only shatter on a ship.
	if (PendingObstacle >= 0)
	{
		States[Index].Obstacle = PendingObstacle;	// an adopted layout rock: its own obstacle, removed when it breaks
	}
	else if (S.SizeClass >= 1 && PendingObstacle != -2)
	{
		if (UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this))
		{
			States[Index].Obstacle = WorldSys->AddObstacle(Pieces[Index].Location, Size * 0.5f + (Wreck != INDEX_NONE ? 3000.f : 0.f));
			++ObstaclesAdded;
		}
	}
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

static int32 GTDLogBreaks = 0;
static FAutoConsoleVariableRef CVarTDLogBreaks(TEXT("td.SpaceField.LogBreaks"), GTDLogBreaks,
	TEXT("1: log every field break (piece, class, size, special, who broke it). Live captures and playtests."));

void ATDSpaceField::BreakPiece(int32 Piece, AActor* ByWhom)
{
	if (GTDLogBreaks != 0 && Pieces.IsValidIndex(Piece))
	{
		const FTDSpaceFieldPiece& P = Pieces[Piece];
		UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.Break: piece %d %s %.0f m%s by %s"), Piece,
			P.Size < 1000.f ? TEXT("small") : P.Size < 4000.f ? TEXT("medium") : TEXT("large"), P.Size / 100.f,
			States.IsValidIndex(Piece) && States[Piece].bSpecial ? TEXT(" special") : TEXT(""), *GetNameSafe(ByWhom));
	}
	// Timed at the outermost break only (a ram can break several in one call; each is counted inside).
	const double Start = FPlatformTime::Seconds();
	++PerfBreakDepth;
	BreakPieceInner(Piece, ByWhom);
	--PerfBreakDepth;
	const double Ms = (FPlatformTime::Seconds() - Start) * 1000.0;
	if (PerfBreakDepth == 0)
	{
		PerfFrameBreakMs += Ms;
	}
	PerfBreakMsSum += Ms;
	PerfBreakMsMax = FMath::Max(PerfBreakMsMax, Ms);
	++PerfBreaks;
}

void ATDSpaceField::BreakPieceInner(int32 Piece, AActor* ByWhom)
{
	using namespace TDSpaceFieldBreak;
	const double T0 = FPlatformTime::Seconds();
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
		S.Visual->UpdateInstanceTransform(S.Instance, T, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		DirtyVisuals.Add(S.Visual);	// one render-state update per component per frame (Tick), not per break
		FreeVisuals.FindOrAdd(S.Visual).Add(S.Instance);
	}
	if (TArray<int32>* Cell = Grid.Find(CellOf(P.Location)))
	{
		Cell->RemoveSingleSwap(Piece);
	}
	if (S.Obstacle != INDEX_NONE)
	{
		if (UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this)) { WorldSys->RemoveObstacle(S.Obstacle); }
		S.Obstacle = INDEX_NONE;
	}

	const double T1 = FPlatformTime::Seconds();
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

	const double T2 = FPlatformTime::Seconds();
	PlayBreakCue(P.Location, P.Size, S.SizeClass, S.bSpecial);
	const double T3 = FPlatformTime::Seconds();
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->BroadcastFieldObjectDestroyed(P.Location, S.SizeClass, S.bSpecial, ByWhom);	// T2's drops, stat, boost; the HUD's notice
	}
	const double T4 = FPlatformTime::Seconds();
	// Zero-lag budget (3 ms a break): any slow break says which phase took the time.
	if ((T4 - T0) * 1000.0 > 3.0)
	{
		UE_LOG(LogTemp, Warning, TEXT("TDSpaceField: slow break %.2f ms (class %d special %d): collision+look %.2f, fragments %.2f, effect+sound %.2f, bus listeners %.2f"),
			(T4 - T0) * 1000.0, S.SizeClass, S.bSpecial, (T1 - T0) * 1000.0, (T2 - T1) * 1000.0, (T3 - T2) * 1000.0, (T4 - T3) * 1000.0);
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
	UInstancedStaticMeshComponent* Visual = FragmentComponentFor(Mesh);
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
	// The effects builder's breaks (eng/vfx 7a7c0023) are the defaults; a DA_SpaceField_FX cue with a VFX wins.
	static const TCHAR* DefaultVFX[] = {
		TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Asteroid_Break_S.NS_TD_Asteroid_Break_S"),
		TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Asteroid_Break_M.NS_TD_Asteroid_Break_M"),
		TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Asteroid_Break_L.NS_TD_Asteroid_Break_L"),
		TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Asteroid_Break_Crystal.NS_TD_Asteroid_Break_Crystal") };
	static const float DefaultScale[] = { 1.f, 2.2f, 4.5f, 2.2f };	// the systems' authored User.Scale per class
	const int32 Slot = bSpecial ? 3 : FMath::Min<int32>(SizeClass, 2);
	FTDFXCue Cue;
	if (FX)
	{
		Cue = bSpecial ? FX->BreakSpecial : (SizeClass == 0 ? FX->BreakSmall : (SizeClass == 1 ? FX->BreakMedium : FX->BreakLarge));
	}
	const bool bAuthored = !Cue.VFX.IsNull();
	if (!bAuthored)
	{
		UNiagaraSystem* Default = DefaultBreakVFX.IsValidIndex(Slot) ? DefaultBreakVFX[Slot].Get() : nullptr;
		Cue.VFX = Default ? TSoftObjectPtr<UNiagaraSystem>(Default) : TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(DefaultVFX[Slot]));
	}
	// Effect and sound timed apart (the zero-lag budget: which one costs on a first play).
	FTDFXCue VfxOnly = Cue; VfxOnly.Sound.Reset(); VfxOnly.Shake.Reset();
	FTDFXCue Rest = Cue; Rest.VFX.Reset();
	const double C0 = FPlatformTime::Seconds();
	UNiagaraComponent* Played = UTDStatics::PlayCue(this, VfxOnly, At, FRotator::ZeroRotator);
	const double C1 = FPlatformTime::Seconds();
	if (!Rest.IsEmpty()) { UTDStatics::PlayCue(this, Rest, At, FRotator::ZeroRotator); }
	const double C2 = FPlatformTime::Seconds();
	if ((C2 - C0) * 1000.0 > 2.0)
	{
		UE_LOG(LogTemp, Warning, TEXT("TDSpaceField: slow cue %.2f ms: effect %s %.2f ms, sound %s %.2f ms"), (C2 - C0) * 1000.0, *GetNameSafe(VfxOnly.VFX.Get()),
			(C1 - C0) * 1000.0, Cue.Sound.IsNull() ? TEXT("none") : *Cue.Sound.ToSoftObjectPath().GetAssetName(), (C2 - C1) * 1000.0);
	}
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: break class %d special %d size %.0f m at %s: %s %s, sound %s (%s)"), SizeClass, bSpecial, Size / 100.f, *At.ToCompactString(),
		*GetNameSafe(Cue.VFX.Get()), Played ? TEXT("playing") : TEXT("NOT PLAYED (placeholder burst)"), Cue.Sound.IsNull() ? TEXT("none") : *Cue.Sound.ToSoftObjectPath().GetAssetName(),
		bAuthored ? TEXT("DA_SpaceField_FX") : TEXT("defaults"));
	if (UNiagaraComponent* NC = Played)
	{
		if (!bAuthored)
		{
			// Bigger pieces of a class read bigger: the authored scale times the piece's size against the class middle.
			static const float ClassMiddle[] = { 500.f, 2500.f, 8000.f, 2500.f };
			NC->SetVariableFloat(TEXT("User.Scale"), DefaultScale[Slot] * FMath::Clamp(Size / ClassMiddle[Slot], 0.6f, 2.f));
		}
		return;
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

// --------------------------------------------------------------------------------- ITDFieldDamageable (T2)

void ATDSpaceField::ReceiveProjectileHit(const FHitResult& Hit, float Damage, AActor* InInstigator, ETDDamageKind Kind)
{
	const int32 Piece = PieceForHit(Hit.GetComponent(), Hit.Item);
	if (Piece != INDEX_NONE)
	{
		DamagePiece(Piece, Damage, InInstigator);
	}
}

bool ATDSpaceField::ReceiveRamSweep(const FVector& From, const FVector& To, float Radius, AActor* InInstigator, FVector& OutBlock)
{
	return RamSweep(From, To, Radius, InInstigator, OutBlock);
}

void ATDSpaceField::ReceiveSplash(const FVector& Center, float Radius, float Damage, AActor* InInstigator)
{
	Splash(Center, Radius, Damage, InInstigator);
}

// ------------------------------------------------------------------------------- ITDShipImpactReceiver (T1)

bool ATDSpaceField::ReceiveShipImpact(UPrimitiveComponent* HitComponent, int32 Item, const FVector& ImpactPoint, float NormalSpeed, float HeadOn01, ATDShip* Ship)
{
	const UTDRamPower* Ram = Ship ? Ship->FindComponentByClass<UTDRamPower>() : nullptr;
	return ShipImpact(HitComponent, Item, NormalSpeed, HeadOn01, Ship, Ram && Ram->IsPowerActive());
}

// ------------------------------------------------------------------------------------------- dev commands

namespace TDSpaceFieldDev
{
	static FAutoConsoleCommandWithWorldAndArgs CmdAim(TEXT("TD.SpaceField.Aim"),
		TEXT("Turn the flown ship (real input) toward the nearest live field piece: TD.SpaceField.Aim <class 0 small | 1 medium | 2 large | 3 special> [MinMetres=0]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDSpaceField* Field = ATDSpaceField::Get(World);
			APawn* Pawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
			if (!Field || !Pawn) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.Aim: no field or no pawn")); return; }
			const int32 Want = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
			const float MinCm = Args.Num() > 1 ? FCString::Atof(*Args[1]) * 100.f : 0.f;
			const FVector From = Pawn->GetActorLocation();
			int32 Best = INDEX_NONE;
			float BestD = TNumericLimits<float>::Max();
			for (int32 i = 0; i < Field->GetPieces().Num(); ++i)
			{
				if (!Field->IsPieceAlive(i) || Field->GetPieces()[i].Kind == ETDSpaceFieldKind::Wreck) { continue; }
				const bool bMatch = Want == 3 ? Field->IsPieceSpecial(i) : (Field->GetPieceSizeClass(i) == Want && !Field->IsPieceSpecial(i));
				const float D = (float)FVector::Dist(From, Field->GetPieces()[i].Location);
				if (bMatch && D >= MinCm && D < BestD) { Best = i; BestD = D; }
			}
			if (Best == INDEX_NONE) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.Aim: none of class %d"), Want); return; }
			const FRotator R = (Field->GetPieces()[Best].Location - From).Rotation();
			UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.Aim: piece %d class %d size %.0f m at %.0f m, yaw %.1f pitch %.1f"), Best, Field->GetPieceSizeClass(Best),
				Field->GetPieces()[Best].Size / 100.f, BestD / 100.f, R.Yaw, R.Pitch);
			GEngine->Exec(World, *FString::Printf(TEXT("TD.Input.TurnToward %.2f %.2f 2"), R.Yaw, R.Pitch));
		}));

	static FAutoConsoleCommandWithWorldAndArgs CmdStats(TEXT("TD.SpaceField.Stats"), TEXT("Log the field's pieces: alive, broken, specials, and the flown ship's distance to the nearest."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDSpaceField* Field = ATDSpaceField::Get(World);
			if (!Field) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.Stats: no field")); return; }
			UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.Stats: %d pieces, %d alive, %d broken"), Field->GetPieces().Num(), Field->GetAliveCount(), Field->GetBrokenCount());
		}));
}

namespace TDSpaceFieldDev
{
	static FAutoConsoleCommandWithWorldAndArgs CmdStage(TEXT("TD.SpaceField.Stage"),
		TEXT("Captures: put the flown ship <Metres> from the nearest live piece of a class with a clear line, facing it, at rest: TD.SpaceField.Stage <class 0|1|2|3 special> [Metres=120]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDSpaceField* Field = ATDSpaceField::Get(World);
			ATDSpacecraft* Ship = World ? Cast<ATDSpacecraft>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
			if (!Field || !Ship) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.Stage: no field or no flown ship")); return; }
			const int32 Want = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
			const float Standoff = (Args.Num() > 1 ? FCString::Atof(*Args[1]) : 120.f) * 100.f;
			const TArray<FTDSpaceFieldPiece>& P = Field->GetPieces();
			const FVector From = Ship->GetActorLocation();
			int32 Best = INDEX_NONE;
			float BestD = TNumericLimits<float>::Max();
			FVector BestAt = FVector::ZeroVector;
			for (int32 i = 0; i < P.Num(); ++i)
			{
				if (!Field->IsPieceAlive(i) || P[i].Kind == ETDSpaceFieldKind::Wreck) { continue; }
				const bool bMatch = Want == 3 ? Field->IsPieceSpecial(i) : (Field->GetPieceSizeClass(i) == Want && !Field->IsPieceSpecial(i));
				const float D = (float)FVector::Dist(From, P[i].Location);
				if (!bMatch || D >= BestD) { continue; }
				// Stand off toward the current ship position, with nothing else on the line.
				const FVector Dir = (From - P[i].Location).GetSafeNormal();
				const FVector At = P[i].Location + Dir * (P[i].Size * 0.4f + Standoff);
				bool bClear = true;
				for (int32 j = 0; j < P.Num() && bClear; ++j)
				{
					bClear = j == i || !Field->IsPieceAlive(j) || FMath::PointDistToSegment(P[j].Location, At, P[i].Location) > P[j].Size * 0.4f + 800.f;
				}
				if (bClear) { Best = i; BestD = D; BestAt = At; }
			}
			if (Best == INDEX_NONE) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.Stage: no clear piece of class %d"), Want); return; }
			const FRotator Facing = (P[Best].Location - BestAt).Rotation();
			Ship->TeleportSpacecraft(BestAt, Facing, /*bKeepSpeed*/ false);
			// The flight stick is a virtual joystick that survives a teleport: hold the new bearing through the real input
			// (it centres the stick once on bearing), or the ship swings back to its old heading.
			GEngine->Exec(World, *FString::Printf(TEXT("TD.Input.TurnToward %.2f %.2f 3"), Facing.Yaw, Facing.Pitch));
			UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.Stage: piece %d class %d size %.0f m special %d, ship %.0f m from its surface"), Best, Field->GetPieceSizeClass(Best),
				P[Best].Size / 100.f, Field->IsPieceSpecial(Best), Standoff / 100.f);
		}));
}

namespace TDSpaceFieldDev
{
	static FAutoConsoleCommandWithWorldAndArgs CmdShootAhead(TEXT("TD.SpaceField.ShootAhead"),
		TEXT("Captures: if a live field piece is under the crosshair (6 deg cone, 30-600 m), hold fire for <Seconds> (real input) and log it: TD.SpaceField.ShootAhead [Seconds=1.5]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDSpaceField* Field = ATDSpaceField::Get(World);
			APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
			if (!Field || !PC || !PC->PlayerCameraManager) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.ShootAhead: no field or camera")); return; }
			const float Secs = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1.5f;
			const FVector Eye = PC->PlayerCameraManager->GetCameraLocation();
			const FVector Fwd = PC->PlayerCameraManager->GetCameraRotation().Vector();
			const float CosCone = FMath::Cos(FMath::DegreesToRadians(6.f));
			int32 Best = INDEX_NONE;
			float BestD = 60000.f;
			for (int32 i = 0; i < Field->GetPieces().Num(); ++i)
			{
				if (!Field->IsPieceAlive(i)) { continue; }
				const FVector To = Field->GetPieces()[i].Location - Eye;
				const float D = (float)To.Size();
				if (D > 3000.f && D < BestD && FVector::DotProduct(To / D, Fwd) > CosCone) { Best = i; BestD = D; }
			}
			if (Best == INDEX_NONE) { UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.ShootAhead: nothing under the crosshair")); return; }
			UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.ShootAhead: piece %d class %d special %d size %.0f m at %.0f m: firing %.1f s"), Best, Field->GetPieceSizeClass(Best),
				Field->IsPieceSpecial(Best), Field->GetPieces()[Best].Size / 100.f, BestD / 100.f, Secs);
			GEngine->Exec(World, *FString::Printf(TEXT("TD.Input.Key LeftMouseButton %.2f"), Secs));
		}));
}

namespace TDSpaceFieldDev
{
	/** TD.SpaceField.FrameWatch: every frame's time while on (the field ticks each frame), the worst ten kept. */
	struct FFrameWatch
	{
		bool bOn = false;
		int32 Frames = 0, Over16 = 0, Over33 = 0;
		double Sum = 0.0, StartTime = 0.0;
		TArray<TPair<float, double>> Worst;	// ms, seconds since start
	};
	static FFrameWatch GFrameWatch;

	static FAutoConsoleCommandWithWorldAndArgs CmdFrameWatch(TEXT("TD.SpaceField.FrameWatch"),
		TEXT("Live: TD.SpaceField.FrameWatch 1 starts (resets) recording every frame's time; 0 stops and logs frames, mean, frames over 16.6 and 33.3 ms and the ten worst (with seconds since start)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const bool bStart = Args.Num() == 0 || Args[0] != TEXT("0");
			if (bStart)
			{
				GFrameWatch = FFrameWatch();
				GFrameWatch.bOn = true;
				GFrameWatch.StartTime = FPlatformTime::Seconds();
				UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.FrameWatch: started"));
				return;
			}
			GFrameWatch.bOn = false;
			FString W;
			for (const TPair<float, double>& P : GFrameWatch.Worst) { W += FString::Printf(TEXT(" %.1f ms @%.1fs"), P.Key, P.Value); }
			UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.FrameWatch: %d frames, mean %.2f ms, over 16.6 ms %d, over 33.3 ms %d, worst:%s"), GFrameWatch.Frames,
				GFrameWatch.Frames ? GFrameWatch.Sum / GFrameWatch.Frames : 0.0, GFrameWatch.Over16, GFrameWatch.Over33, *W);
		}));

	static FAutoConsoleCommandWithWorldAndArgs CmdTestShot(TEXT("TD.SpaceField.TestShot"),
		TEXT("Captures: put the flown ship 80 m from the nearest lone live piece of a class, facing it, and in the same frame launch <Shots> real player laser bolts from its nose at it: TD.SpaceField.TestShot <class 0|1|2|3 special> [Shots=10]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDSpaceField* Field = ATDSpaceField::Get(World);
			ATDSpacecraft* Ship = World ? Cast<ATDSpacecraft>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
			if (!Field || !Ship) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.TestShot: no field or no flown ship")); return; }
			const int32 Want = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
			const int32 Shots = Args.Num() > 1 ? FMath::Clamp(FCString::Atoi(*Args[1]), 1, 60) : 10;
			const TArray<FTDSpaceFieldPiece>& P = Field->GetPieces();
			const FVector From = Ship->GetActorLocation();
			int32 Best = INDEX_NONE;
			float BestD = TNumericLimits<float>::Max();
			for (int32 i = 0; i < P.Num(); ++i)
			{
				if (!Field->IsPieceAlive(i) || P[i].Kind == ETDSpaceFieldKind::Wreck) { continue; }
				const bool bMatch = Want == 3 ? Field->IsPieceSpecial(i) : (Field->GetPieceSizeClass(i) == Want && !Field->IsPieceSpecial(i));
				const float D = (float)FVector::Dist(From, P[i].Location);
				if (!bMatch || D >= BestD) { continue; }
				bool bLone = true;	// nothing else (field or layout rock) within 80 m of the line
				const FVector Stand = P[i].Location + (From - P[i].Location).GetSafeNormal() * (P[i].Size * 0.4f + 8000.f);
				for (int32 j = 0; j < P.Num() && bLone; ++j)
				{
					bLone = j == i || !Field->IsPieceAlive(j) || FMath::PointDistToSegment(P[j].Location, Stand, P[i].Location) > P[j].Size * 0.4f + 1500.f;
				}
				if (bLone && UTDWorldSubsystem::Get(World) && !UTDWorldSubsystem::Get(World)->IsInsideObstacle(Stand, 3000.f)) { Best = i; BestD = D; }
			}
			if (Best == INDEX_NONE) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.TestShot: no lone piece of class %d"), Want); return; }
			const FVector Target = P[Best].Location;
			const FVector Stand = Target + (From - Target).GetSafeNormal() * (P[Best].Size * 0.4f + 8000.f);
			const FRotator Facing = (Target - Stand).Rotation();
			Ship->TeleportSpacecraft(Stand, Facing, /*bKeepSpeed*/ false);
			const FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::PhasePulseLasers).Projectile;
			const FVector Nose = Stand + Facing.Vector() * 600.f;
			for (int32 s = 0; s < Shots; ++s)
			{
				// Spread along the line so each bolt arrives on its own frame (not one stacked hit).
				ATDProjectile::Launch(World, Spec, FTransform(Facing, Nose - Facing.Vector() * (s * 400.f)), Ship, ETDTeam::Player, nullptr, FVector::ZeroVector, 1.f, true);
			}
			UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.TestShot: piece %d class %d special %d size %.0f m health %.0f, %d bolts of %.0f from 80 m"), Best,
				Field->GetPieceSizeClass(Best), Field->IsPieceSpecial(Best), P[Best].Size / 100.f, Field->GetPieceHealth(Best), Shots, Spec.Damage);
		}));
}

double ATDSpaceField::BreakChain(const FVector& Where, int32 Count, AActor* ByWhom)
{
	TArray<int32> Order;
	for (int32 i = 0; i < Pieces.Num(); ++i) { if (IsPieceAlive(i) && Pieces[i].Kind != ETDSpaceFieldKind::Wreck) { Order.Add(i); } }
	Order.Sort([this, &Where](int32 A, int32 B) { return FVector::DistSquared(Pieces[A].Location, Where) < FVector::DistSquared(Pieces[B].Location, Where); });
	const double Start = FPlatformTime::Seconds();
	for (int32 k = 0; k < FMath::Min(Count, Order.Num()); ++k) { DamagePiece(Order[k], 1.0e7f, ByWhom); }
	return (FPlatformTime::Seconds() - Start) * 1000.0;
}

namespace TDSpaceFieldDev
{
	static FAutoConsoleCommandWithWorldAndArgs CmdPerf(TEXT("TD.SpaceField.Perf"), TEXT("Log the field's tick and break costs: TD.SpaceField.Perf [reset]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (ATDSpaceField* Field = ATDSpaceField::Get(World)) { Field->PerfReport(Args.Num() > 0 && Args[0].Equals(TEXT("reset"), ESearchCase::IgnoreCase)); }
		}));

	static FAutoConsoleCommandWithWorldAndArgs CmdChain(TEXT("TD.SpaceField.BreakChain"),
		TEXT("Perf: break the N live pieces nearest the flown ship in one frame and log the cost: TD.SpaceField.BreakChain [N=5]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDSpaceField* Field = ATDSpaceField::Get(World);
			APawn* Pawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
			if (!Field || !Pawn) { return; }
			const int32 N = Args.Num() > 0 ? FMath::Max(1, FCString::Atoi(*Args[0])) : 5;
			const double Ms = Field->BreakChain(Pawn->GetActorLocation(), N, Pawn);
			UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.BreakChain: %d breaks in one frame took %.3f ms"), N, Ms);
		}));
}

namespace TDSpaceFieldDev
{
	static FAutoConsoleCommandWithWorldAndArgs CmdStageBreak(TEXT("TD.SpaceField.StageBreak"),
		TEXT("Effects review: park the flown ship <Metres> from a piece of the class (clear space preferred), facing it, then break it after <Delay> s and log the system and sound: TD.SpaceField.StageBreak <class 0|1|2|3 special> [Metres=150] [Delay=1.5]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDSpaceField* Field = ATDSpaceField::Get(World);
			ATDSpacecraft* Ship = World ? Cast<ATDSpacecraft>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
			if (!Field || !Ship) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.StageBreak: no field or no flown ship")); return; }
			const int32 Want = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 2;
			const float Standoff = (Args.Num() > 1 ? FCString::Atof(*Args[1]) : 150.f) * 100.f;
			const float Delay = Args.Num() > 2 ? FMath::Max(0.1f, FCString::Atof(*Args[2])) : 1.5f;
			const TArray<FTDSpaceFieldPiece>& P = Field->GetPieces();
			const FVector From = Ship->GetActorLocation();
			// The nearest match with the most clear space around it within 2 km of the best candidates (any match as a fallback).
			int32 Best = INDEX_NONE;
			float BestScore = -TNumericLimits<float>::Max();
			for (int32 i = 0; i < P.Num(); ++i)
			{
				if (!Field->IsPieceAlive(i) || P[i].Kind == ETDSpaceFieldKind::Wreck) { continue; }
				const bool bMatch = Want == 3 ? Field->IsPieceSpecial(i) : (Field->GetPieceSizeClass(i) == Want && !Field->IsPieceSpecial(i));
				if (!bMatch) { continue; }
				float Clear = 20000.f;	// up to 200 m of clearance counts
				for (int32 j = 0; j < P.Num(); ++j)
				{
					if (j == i || !Field->IsPieceAlive(j)) { continue; }
					const float D = (float)FVector::Dist(P[i].Location, P[j].Location) - P[j].Size * 0.5f - P[i].Size * 0.5f;
					if (D < Clear) { Clear = D; if (Clear <= 0.f) { break; } }
				}
				const float Score = Clear * 10.f - (float)FVector::Dist(From, P[i].Location) * 0.01f;
				if (Score > BestScore) { BestScore = Score; Best = i; }
			}
			if (Best == INDEX_NONE) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.StageBreak: no live piece of class %d"), Want); return; }
			const FVector Target = P[Best].Location;
			const FVector Stand = Target + (From - Target).GetSafeNormal() * (P[Best].Size * 0.5f + Standoff);
			const FRotator Facing = (Target - Stand).Rotation();
			Ship->TeleportSpacecraft(Stand, Facing, /*bKeepSpeed*/ false);
			GEngine->Exec(World, *FString::Printf(TEXT("TD.Input.TurnToward %.2f %.2f %.1f"), Facing.Yaw, Facing.Pitch, Delay + 0.5f));
			UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.StageBreak: piece %d class %d special %d size %.0f m, ship %.0f m from its surface, breaking in %.1f s"), Best,
				Field->GetPieceSizeClass(Best), Field->IsPieceSpecial(Best), P[Best].Size / 100.f, Standoff / 100.f, Delay);
			TWeakObjectPtr<ATDSpaceField> WeakField(Field);
			TWeakObjectPtr<ATDSpacecraft> WeakShip(Ship);
			FTimerHandle Handle;
			World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakField, WeakShip, Best]()
			{
				if (ATDSpaceField* F = WeakField.Get()) { F->DamagePiece(Best, 1.0e7f, WeakShip.Get()); }
			}), Delay, false);
		}));
}

UInstancedStaticMeshComponent* ATDSpaceField::FragmentComponentFor(UStaticMesh* Mesh)
{
	// Fragments of broken rocks: a plain (non-hierarchical) instanced component per mesh, so adding or recycling one never
	// rebuilds a cluster tree. Few instances, near the camera: no shadows, 4 km cull.
	TObjectPtr<UInstancedStaticMeshComponent>& C = FragmentComponents.FindOrAdd(Mesh);
	if (!C)
	{
		C = NewObject<UInstancedStaticMeshComponent>(this, *FString::Printf(TEXT("Frag_%s"), *GetNameSafe(Mesh)));
		C->SetupAttachment(RootComponent);
		C->SetStaticMesh(Mesh);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		C->SetCastShadow(false);
		C->bAffectDistanceFieldLighting = false;
		C->SetCullDistances(0, 400000);
		C->RegisterComponent();
	}
	return C;
}

void ATDSpaceField::PreloadBreakCues()
{
	// Load every break effect and sound now, so no break ever loads one synchronously in its frame (the first breaks of
	// each kind cost 10-20 ms before this).
	if (!FX)
	{
		FX = UTDStatics::LoadPresentation<UTDSpaceFieldFX>(TEXT("DA_SpaceField_FX"));
	}
	bFXLoaded = true;
	if (FX)
	{
		for (const FTDFXCue* Cue : { &FX->BreakSmall, &FX->BreakMedium, &FX->BreakLarge, &FX->BreakSpecial })
		{
			if (!Cue->VFX.IsNull()) { PreloadedCueAssets.Add(Cue->VFX.LoadSynchronous()); }
			if (!Cue->Sound.IsNull()) { PreloadedCueAssets.Add(Cue->Sound.LoadSynchronous()); }
		}
	}
}

namespace TDSpaceFieldWarm
{
	// T7's warm-up (#89) streams DA_SpaceField_FX and everything it names; the field's own default break systems are
	// hard references on the class defaults (loaded) but would otherwise first activate on the first break.
	static struct FRegister
	{
		FRegister()
		{
			UTDWarmupSubsystem::RegisterProvider(TEXT("SpaceField"), [](const UWorld& World, TArray<FSoftObjectPath>& Out)
			{
				Out.Add(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Asteroid_Break_S.NS_TD_Asteroid_Break_S")));
				Out.Add(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Asteroid_Break_M.NS_TD_Asteroid_Break_M")));
				Out.Add(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Asteroid_Break_L.NS_TD_Asteroid_Break_L")));
				Out.Add(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Asteroid_Break_Crystal.NS_TD_Asteroid_Break_Crystal")));
			});
		}
	} GRegister;
}

void ATDSpaceField::NoteFrame(float DeltaSeconds)
{
	using namespace TDSpaceFieldDev;
	if (!GFrameWatch.bOn || GFrameNoted == GFrameCounter) { return; }
	GFrameNoted = GFrameCounter;
	const float Ms = DeltaSeconds * 1000.f;
	++GFrameWatch.Frames;
	GFrameWatch.Sum += Ms;
	GFrameWatch.Over16 += Ms > 16.67f ? 1 : 0;
	GFrameWatch.Over33 += Ms > 33.33f ? 1 : 0;
	GFrameWatch.Worst.Add({ Ms, FPlatformTime::Seconds() - GFrameWatch.StartTime });
	GFrameWatch.Worst.Sort([](const TPair<float, double>& A, const TPair<float, double>& B) { return A.Key > B.Key; });
	if (GFrameWatch.Worst.Num() > 10) { GFrameWatch.Worst.SetNum(10); }
}
