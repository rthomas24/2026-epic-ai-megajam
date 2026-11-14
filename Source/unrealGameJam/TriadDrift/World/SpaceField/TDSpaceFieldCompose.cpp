// Triad Drift - the space field's composed regions (Teammate 6; docs/eng/SPACE-DESIGN.md, approved Nov 14).
// Ryan: space must read as a real region with belts and distinct areas, not "a bunch of rocks bundled in a pile".
// td.SpaceField.Compose 1 (default) builds the regions; 0 builds the Nov 13 composition (random tilted rings and
// cluster shells), kept and indexed in docs/eng/ART-HISTORY.md.
//
// Region 2, the Shepherd Belt: a true ring round the combat zone in ONE thin plane, rocks on circular orbits, larger
// rocks on the inner edge grading to gravel outside, thicker and thinner stretches along the arc, a few landmark rocks,
// clean gaps where the routes cross (900 m+ wide on the Verdance bearing: T4's tutorial escape crosses there), and a
// shepherd moonlet on each edge of the Verdance and Dominion gaps.

#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"

namespace TDSpaceFieldCompose
{
	static int32 GCompose = 1;
	static FAutoConsoleVariableRef CVarCompose(TEXT("td.SpaceField.Compose"), GCompose,
		TEXT("1 (default): the composed regions of docs/eng/SPACE-DESIGN.md. 0: the Nov 13 composition (random rings and cluster shells). Read at level load."));

	/** Size by the power law used everywhere (many small, few large). */
	static float Power(FRandomStream& Rng, float Min, float Max, float Exponent) { return FMath::Lerp(Min, Max, FMath::Pow(Rng.FRand(), Exponent)); }
	/** A standard normal sample (Box-Muller). */
	static float Gauss(FRandomStream& Rng) { const float U = FMath::Max(1e-6f, Rng.FRand()); return FMath::Sqrt(-2.f * FMath::Loge(U)) * FMath::Cos(2.f * PI * Rng.FRand()); }
}

bool ATDSpaceField::UseComposedRegions() const
{
	return TDSpaceFieldCompose::GCompose != 0;
}

void ATDSpaceField::BuildShepherdBelt(FRandomStream& Rng)
{
	using namespace TDSpaceFieldCompose;
	const FTransform Xf = Layout->GetActorTransform();
	const FTDGalaxyLayoutData& L = Layout->GetLayout();
	const FVector Zone = Xf.TransformPosition(L.CombatZoneCenter);

	// The plane: tilted BeltTiltDeg about a seeded horizontal axis. Basis U, V in the plane, N its normal.
	const FQuat Tilt = FQuat(FVector::UpVector, FMath::DegreesToRadians(Rng.FRandRange(0.f, 360.f))) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(BeltTiltDeg));
	const FVector U = Tilt.RotateVector(FVector::ForwardVector);
	const FVector V = Tilt.RotateVector(FVector::RightVector);
	const FVector N = Tilt.RotateVector(FVector::UpVector);
	BeltNormal = N;
	BeltCenter = Zone;

	// Gaps: every route out of the zone (to the planets, the Dominion, the gate ends), as a bearing in the plane with a
	// half width. The Verdance bearing is wide (T4: the escape line can start anywhere in the pocket).
	struct FGap { FVector Dir; float HalfWidth; bool bShepherds; };
	TArray<FGap> Gaps;
	auto InPlane = [&](const FVector& WorldTarget) { FVector D = WorldTarget - Zone; D -= N * FVector::DotProduct(D, N); return D.GetSafeNormal(); };
	for (const FTDPlanetSpec& Pl : L.Planets)
	{
		const bool bVerdance = Pl.Destination == ETDDestination::Verdance;
		Gaps.Add({ InPlane(Xf.TransformPosition(Pl.Location)), bVerdance ? BeltVerdanceGapHalf : BeltGapHalf, true });
	}
	Gaps.Add({ InPlane(Xf.TransformPosition(L.Dominion.GetLocation())), BeltGapHalf, true });
	for (const FTDGatePairSpec& G : L.Gates)
	{
		for (const FTransform& E : { G.A, G.B })
		{
			const FVector P = Xf.TransformPosition(E.GetLocation());
			if (FVector::Dist(P, Zone) > BeltInner * 0.5f) { Gaps.Add({ InPlane(P), BeltGapHalf, false }); }
		}
	}
	auto InGap = [&](const FVector& Local, float Pad)	// Local = offset from the zone, in the plane
	{
		for (const FGap& G : Gaps)
		{
			const float Along = FVector::DotProduct(Local, G.Dir);
			if (Along <= 0.f) { continue; }
			if ((Local - G.Dir * Along).Size() < G.HalfWidth + Pad) { return true; }
		}
		return false;
	};

	// Shepherd moonlets: one on each edge of the gaps that have them, on the ring's middle.
	const float Mid = (BeltInner + BeltOuter) * 0.5f;
	for (const FGap& G : Gaps)
	{
		if (!G.bShepherds) { continue; }
		const FVector Side = FVector::CrossProduct(N, G.Dir).GetSafeNormal();
		for (const float S : { -1.f, 1.f })
		{
			const float Size = Rng.FRandRange(MoonletMinSize, MoonletMaxSize);
			const FVector Local = G.Dir * Mid + Side * S * (G.HalfWidth + Size * 0.5f + 8000.f);
			AddRock(Zone + Local, Size, ETDSpaceFieldKind::Belt, Rng, 0);
		}
	}

	// Landmarks: big rocks spread round the ring in the plane (so the belt is not uniform noise).
	for (int32 i = 0, Try = 0; i < BeltLandmarks && Try < BeltLandmarks * 6; ++Try)
	{
		const float A = (2.f * PI) * (i + Rng.FRandRange(0.f, 0.8f)) / BeltLandmarks;
		const float R = FMath::Lerp(BeltInner, Mid, Rng.FRand());
		const FVector Local = (U * FMath::Cos(A) + V * FMath::Sin(A)) * R + N * Gauss(Rng) * BeltThickness * 0.3f;
		const float Size = Rng.FRandRange(6000.f, 12000.f);
		if (InGap(Local, Size * 0.5f + 3000.f)) { continue; }
		i += AddRock(Zone + Local, Size, ETDSpaceFieldKind::Belt, Rng, Rng.FRand() < 0.6f ? 0 : 2) ? 1 : 0;
	}

	// The ring: orbit radius across the band, density modulated along the arc (three thick and three thin stretches plus
	// noise), size graded from the inner edge (to 90 m) to the outer edge (gravel to 15 m), thickness thinner outside.
	const float PhaseA = Rng.FRandRange(0.f, 2.f * PI), PhaseB = Rng.FRandRange(0.f, 2.f * PI);
	int32 Placed = 0;
	for (int32 Try = 0; Placed < ShepherdRocks && Try < ShepherdRocks * 6; ++Try)
	{
		const float A = Rng.FRandRange(0.f, 2.f * PI);
		const float Density = 0.55f + 0.3f * FMath::Sin(3.f * A + PhaseA) + 0.15f * FMath::Sin(7.f * A + PhaseB);
		if (Rng.FRand() > Density) { continue; }
		const float T = FMath::Clamp(0.5f + Gauss(Rng) * 0.22f, 0.f, 1.f);	// across the band: dense middle, soft edges
		const float R = FMath::Lerp(BeltInner, BeltOuter, T);
		const float Thick = BeltThickness * (1.f - 0.5f * T);
		const FVector Local = (U * FMath::Cos(A) + V * FMath::Sin(A)) * R + N * Gauss(Rng) * Thick;
		const float MaxSize = FMath::Lerp(9000.f, 1500.f, T);
		const float Size = Power(Rng, 400.f, MaxSize, 3.5f);
		if (InGap(Local, Size * 0.5f)) { continue; }
		// Families: rock and metal inside, ice toward the outer edge.
		const int32 Fam = Rng.FRand() < T ? 1 : (Rng.FRand() < 0.5f ? 0 : 2);
		Placed += AddRock(Zone + Local, Size, ETDSpaceFieldKind::Belt, Rng, Fam) ? 1 : 0;
	}
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: Shepherd Belt %d rocks, %d gaps, r %.1f-%.1f km, tilt %.0f deg"), Placed, Gaps.Num(), BeltInner / 100000.f,
		BeltOuter / 100000.f, BeltTiltDeg);
}

// ------------------------------------------------------------------------------------- region views (captures)

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TriadDrift/Flight/TDSpacecraft.h"

namespace TDSpaceFieldCompose
{
	/** Fixed vantage points per region, so before / after captures share the view. Region 2 = the Shepherd Belt. */
	static bool RegionView(const ATDSpaceField* Field, int32 Region, int32 Range, FVector& OutFrom, FVector& OutAt)
	{
		if (Region == 1 || Region == 4 || Region == 7)
		{
			// Around the region's centre: 5 km and 1 km out on the side away from the zone, 25 degrees up; inside = 150 m.
			const FVector* Found = Field->RegionCentres.Find(Region);
			const FVector C = Found ? *Found : (Region == 1 ? FVector(170000.f, 0.f, 0.f) : (Region == 4 ? FVector(300000.f, 150000.f, 20000.f) : FVector(600000.f, 0.f, 0.f)));
			FVector Out = C.GetSafeNormal2D();
			if (Out.IsNearlyZero()) { Out = FVector::ForwardVector; }
			if (Region == 7) { Out = -Out; }	// the Outer Dark is seen from the zone side, looking out (5 km beyond it is outside the play bounds)
			const FVector Dir = (Out * FMath::Cos(FMath::DegreesToRadians(25.f)) + FVector::UpVector * FMath::Sin(FMath::DegreesToRadians(25.f))).GetSafeNormal();
			const float D = Range == 0 ? 500000.f : (Range == 1 ? 100000.f : 15000.f);
			OutAt = C;
			OutFrom = C + Dir * D;
			return true;
		}
		if (Region == 2)
		{
			// World axes, not the belt's seeded tilt (8 degrees), so Compose 0 and 1 captures share the exact view.
			const FVector C = FVector::ZeroVector;
			const FVector N = FVector::UpVector;
			const FVector U = FVector::CrossProduct(N, FVector::RightVector).GetSafeNormal();	// a fixed in-plane direction
			const FVector V = FVector::CrossProduct(N, U).GetSafeNormal();
			const float Mid = (Field->BeltInner + Field->BeltOuter) * 0.5f;
			switch (Range)
			{
			case 0:	// 5 km out, 30 degrees above the plane, looking at the zone: the whole ring
				OutAt = C;
				OutFrom = C + (U * FMath::Cos(FMath::DegreesToRadians(30.f)) + N * FMath::Sin(FMath::DegreesToRadians(30.f))) * 500000.f;
				return true;
			case 1:	// 1 km outside the ring, just above the plane, looking at its middle radius
				OutAt = C + U * Mid;
				OutFrom = C + U * (Field->BeltOuter + 100000.f) + N * 15000.f;
				return true;
			default:	// inside the ring, on the middle radius, looking along it
				OutFrom = C + U * Mid + N * 2000.f;
				OutAt = OutFrom + V * 100000.f;
				return true;
			}
		}
		return false;
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdView(TEXT("TD.SpaceField.ViewRegion"),
		TEXT("Captures: park the flown ship at a region's fixed vantage, facing it: TD.SpaceField.ViewRegion <region (1 Watchline halo, 2 Shepherd Belt, 4 Quiet Graveyard)> <0 = 5 km | 1 = 1 km | 2 = inside>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDSpaceField* Field = ATDSpaceField::Get(World);
			ATDSpacecraft* Ship = World ? Cast<ATDSpacecraft>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
			FVector From, At;
			const int32 Region = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 2;
			const int32 Range = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
			if (!Field || !Ship || !RegionView(Field, Region, Range, From, At)) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.ViewRegion: no field / ship / view for region %d"), Region); return; }
			const FRotator Facing = (At - From).Rotation();
			Ship->TeleportSpacecraft(From, Facing, /*bKeepSpeed*/ false);
			GEngine->Exec(World, *FString::Printf(TEXT("TD.Input.TurnToward %.2f %.2f 2"), Facing.Yaw, Facing.Pitch));
			UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.ViewRegion %d %d: from %s facing %s"), Region, Range, *From.ToCompactString(), *Facing.ToCompactString());
		}));
}

// ------------------------------------------------------------------------------ structures (indestructible set pieces)

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "TriadDrift/World/TDWorldSubsystem.h"

namespace TDSpaceFieldCompose
{
	static UStaticMesh* LoadSetPiece(const TCHAR* Name)
	{
		const FString Path = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Props/Space/SetPieces/%s.%s"), Name, Name);
		return LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
	}
}

bool ATDSpaceField::AddStructure(UStaticMesh* Mesh, const FTransform& T)
{
	// The structures builder's set pieces (real scale, 1-2 convex hulls each): solid with their own collision, never
	// broken (Lead: nothing is passed through; they are landmarks), and a chain of avoidance spheres for the AI.
	if (!Mesh) { return false; }
	const FBox Box = Mesh->GetBoundingBox();
	const FVector Ext = Box.GetExtent() * T.GetScale3D();
	if (!IsPlacementAllowed(T.GetLocation(), Ext.Size() * 2.f, true)) { return false; }
	const FString Key = FString::Printf(TEXT("Struct_%s"), *GetNameSafe(Mesh));
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& C = Components.FindOrAdd(Key);
	if (!C)
	{
		C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, *Key);
		C->SetupAttachment(RootComponent);
		C->SetStaticMesh(Mesh);
		C->SetMobility(EComponentMobility::Static);
		C->SetCollisionProfileName(TEXT("BlockAll"));
		C->SetCanEverAffectNavigation(false);
		C->SetCastShadow(true);
		C->SetCullDistances(0, 900000);
		C->RegisterComponent();
	}
	C->AddInstance(T, /*bWorldSpace*/ true);
	if (UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this))
	{
		// Spheres along the longest local axis, each as wide as the second-longest extent.
		const int32 Axis = Ext.X >= Ext.Y && Ext.X >= Ext.Z ? 0 : (Ext.Y >= Ext.Z ? 1 : 2);
		const float Long = Ext[Axis];
		float Mid = 0.f;
		for (int32 a = 0; a < 3; ++a) { if (a != Axis) { Mid = FMath::Max(Mid, (float)Ext[a]); } }
		const int32 N = FMath::Clamp(FMath::CeilToInt(Long / FMath::Max(Mid, 100.f)), 1, 8);
		FVector Dir = FVector::ZeroVector; Dir[Axis] = 1.f;
		const FVector Centre = T.TransformPosition(Box.GetCenter());
		const FVector WDir = T.TransformVectorNoScale(Dir);
		for (int32 i = 0; i < N; ++i)
		{
			const float Along = N == 1 ? 0.f : FMath::Lerp(-Long + Mid, Long - Mid, float(i) / (N - 1));
			WorldSys->AddObstacle(Centre + WDir * Along, Mid * 1.1f);
			++ObstaclesAdded;
		}
	}
	FTDSpaceFieldPiece P;
	P.Location = T.GetLocation();
	P.Size = Ext.Size() * 2.f;
	P.bCollides = true;
	P.Kind = ETDSpaceFieldKind::Wreck;
	Structures.Add(P);
	return true;
}

void ATDSpaceField::ScatterDebris(const FVector& Centre, float Radius, int32 Count, float MaxSize, FRandomStream& Rng)
{
	// Spilled wreckage and rubble round a structure: small, destructible, solid.
	for (int32 n = 0, Try = 0; n < Count && Try < Count * 4; ++Try)
	{
		const bool bWreckage = Rng.FRand() < 0.6f;
		const TArray<TObjectPtr<UStaticMesh>>& Pool = Families[bWreckage ? 5 : 4];
		if (Pool.Num() == 0) { continue; }
		const FVector At = Centre + FVector(Rng.GetUnitVector()) * Radius * FMath::Sqrt(Rng.FRand());
		const float Size = TDSpaceFieldCompose::Power(Rng, 150.f, MaxSize, 2.5f);
		n += AddPiece(Pool[Rng.RandRange(0, Pool.Num() - 1)], At, FRotator(Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f)),
			Size, ETDSpaceFieldKind::Debris, false) ? 1 : 0;
	}
}

void ATDSpaceField::BuildWatchlineHalo(FRandomStream& Rng)
{
	// Region 1, the Watchline halo: the lost patrol. Three hulls broken in four pieces each (the structures builder
	// grouping, Space_PatrolWreck.manifest.json), 1.5-1.85 km from the zone, and a thin shell of their debris from 1.25 km.
	using namespace TDSpaceFieldCompose;
	struct FPart { const TCHAR* Mesh; FVector Off; FRotator Rot; };
	static const FPart Parts[] = {
		{ TEXT("SM_SP_Patrol_Mid"), FVector(0.f, 0.f, 0.f), FRotator(0.f, 0.f, 18.f) },
		{ TEXT("SM_SP_Patrol_Bow"), FVector(2100.f, 450.f, 350.f), FRotator(-9.f, 12.f, -25.f) },
		{ TEXT("SM_SP_Patrol_Aft"), FVector(-2300.f, -500.f, -420.f), FRotator(6.f, -14.f, 40.f) },
		{ TEXT("SM_SP_Patrol_Wing"), FVector(400.f, 1500.f, 900.f), FRotator(20.f, 35.f, 55.f) } };
	const FVector Zone = Layout->GetActorTransform().TransformPosition(Layout->GetLayout().CombatZoneCenter);
	int32 Wrecks = 0;
	for (int32 Try = 0; Wrecks < HaloWrecks && Try < HaloWrecks * 12; ++Try)
	{
		FVector Dir = FVector(Rng.GetUnitVector()); Dir.Z *= 0.35f; Dir.Normalize();
		const FVector C = Zone + Dir * Rng.FRandRange(150000.f, 185000.f);
		if (!IsPlacementAllowed(C, 12000.f, true)) { continue; }
		const FTransform G(FRotator(Rng.FRandRange(-25.f, 25.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-40.f, 40.f)), C);
		int32 Added = 0;
		for (const FPart& Pt : Parts)
		{
			// The pieces drift a little further apart than the manifest (1.5x) so each reads alone from 1 km.
			Added += AddStructure(LoadSetPiece(Pt.Mesh), FTransform(Pt.Rot, Pt.Off * 1.5f) * G) ? 1 : 0;
		}
		if (Added > 0) { ++Wrecks; ScatterDebris(C, 9000.f, 40, 900.f, Rng); if (!RegionCentres.Contains(1)) { RegionCentres.Add(1, C); } }
	}
	// The halo of their debris: small, sparse, in a shell round the pocket.
	int32 Bits = 0;
	for (int32 Try = 0; Bits < HaloDebris && Try < HaloDebris * 4; ++Try)
	{
		FVector Dir = FVector(Rng.GetUnitVector()); Dir.Z *= 0.5f; Dir.Normalize();
		const TArray<TObjectPtr<UStaticMesh>>& Pool = Families[Rng.FRand() < 0.5f ? 5 : 4];
		if (Pool.Num() == 0) { continue; }
		Bits += AddPiece(Pool[Rng.RandRange(0, Pool.Num() - 1)], Zone + Dir * Rng.FRandRange(125000.f, 185000.f),
			FRotator(Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f)), Power(Rng, 150.f, 1200.f, 3.f), ETDSpaceFieldKind::Debris, false) ? 1 : 0;
	}
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: Watchline halo %d patrol wrecks, %d debris"), Wrecks, Bits);
}

void ATDSpaceField::BuildQuietGraveyard(FRandomStream& Rng)
{
	// Region 4, the Quiet Graveyard: the research expedition hulls in a loose line beside the Verdance transponder,
	// 3.2-3.8 km from the zone on the Verdance side, 700 m+ off the route; dark and still; their debris; a few buoys.
	using namespace TDSpaceFieldCompose;
	const FTransform Xf = Layout->GetActorTransform();
	const FTDGalaxyLayoutData& L = Layout->GetLayout();
	const FVector Zone = Xf.TransformPosition(L.CombatZoneCenter);
	FVector VDir = FVector::ForwardVector;
	for (const FTDPlanetSpec& Pl : L.Planets) { if (Pl.Destination == ETDDestination::Verdance) { VDir = (Xf.TransformPosition(Pl.Location) - Zone).GetSafeNormal(); } }
	const FVector Side = FVector::CrossProduct(FVector::UpVector, VDir).GetSafeNormal();
	// The side of the bearing farther from the convoy lane (hulls are hittable: the convoy tube keeps them 1.2 km away).
	float Sign = 1.f, BestClear = -1.f;
	for (const float Sg : { 1.f, -1.f })
	{
		float Clear = TNumericLimits<float>::Max();
		for (const float Along : { 320000.f, 350000.f, 380000.f })
		{
			const FVector P = Zone + VDir * Along + Side * Sg * 80000.f;
			for (const TArray<FVector>& Lane : ConvoyLanes) { Clear = FMath::Min(Clear, (float)FVector::Dist(P, FMath::ClosestPointOnSegment(P, Lane[0], Lane[1]))); }
		}
		if (Clear > BestClear) { BestClear = Clear; Sign = Sg; }
	}
	const FVector A = Zone + VDir * 320000.f + Side * Sign * 70000.f + FVector(0.f, 0.f, 15000.f);
	const FVector B = Zone + VDir * 380000.f + Side * Sign * 90000.f + FVector(0.f, 0.f, 30000.f);
	UStaticMesh* Hulls[] = { LoadSetPiece(TEXT("SM_SP_Research_A")), LoadSetPiece(TEXT("SM_SP_Research_B")) };
	RegionCentres.Add(4, (A + B) * 0.5f);
	int32 Placed = 0;
	for (int32 i = 0; i < GraveyardHulls; ++i)
	{
		const float T = GraveyardHulls > 1 ? float(i) / (GraveyardHulls - 1) : 0.5f;
		const FVector P = FMath::Lerp(A, B, T) + FVector(Rng.GetUnitVector()) * 9000.f;
		// Lying roughly along the line, each a little turned and rolled: a fleet that went quiet in formation.
		const FRotator R = FRotator(Rng.FRandRange(-15.f, 15.f), (B - A).Rotation().Yaw + Rng.FRandRange(-30.f, 30.f), Rng.FRandRange(-60.f, 60.f));
		if (AddStructure(Hulls[i % 2], FTransform(R, P)))
		{
			++Placed;
			ScatterDebris(P, 12000.f, 35, 1500.f, Rng);
		}
	}
	if (BuoyMesh)
	{
		for (int32 b = 0; b < 3; ++b)
		{
			const FVector P = FMath::Lerp(A, B, (b + 0.5f) / 3.f) + FVector(0.f, 0.f, -12000.f);
			AddPiece(BuoyMesh, P, (B - A).Rotation(), 900.f, ETDSpaceFieldKind::Buoy, false);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: Quiet Graveyard %d hulls (side %+.0f, %.0f m from the convoy lane)"), Placed, Sign, BestClear / 100.f);
}

void ATDSpaceField::BuildOuterDark(FRandomStream& Rng)
{
	// Region 7, the Outer Dark: the edge of the system. Lone big rocks (20-120 m), each with a little gravel, from
	// OuterDarkInner to the play bounds in every direction; density varies smoothly so it is not a uniform sprinkle.
	using namespace TDSpaceFieldCompose;
	const FTransform Xf = Layout->GetActorTransform();
	const FVector Zone = Xf.TransformPosition(Layout->GetLayout().CombatZoneCenter);
	const float Outer = Layout->GetLayout().BoundaryRadius * 0.94f;
	const FVector P1(Rng.FRandRange(0.f, 6.f), Rng.FRandRange(0.f, 6.f), Rng.FRandRange(0.f, 6.f));
	int32 Anchors = 0, Gravel = 0;
	for (int32 Try = 0; Anchors < OuterDarkRocks && Try < OuterDarkRocks * 8; ++Try)
	{
		const FVector Dir = FVector(Rng.GetUnitVector());
		// A smooth density over the sphere of directions: thicker and thinner skies, never empty.
		const float Density = 0.45f + 0.35f * FMath::Sin(Dir.X * 3.1f + P1.X) * FMath::Cos(Dir.Y * 2.7f + P1.Y) + 0.2f * FMath::Sin(Dir.Z * 4.3f + P1.Z);
		if (Rng.FRand() > FMath::Clamp(Density, 0.15f, 1.f)) { continue; }
		const float R = FMath::Lerp(OuterDarkInner, Outer, FMath::Sqrt(Rng.FRand()));
		const FVector At = Zone + Dir * R;
		const float Size = Power(Rng, 2000.f, 12000.f, 2.2f);
		const int32 Fam = Rng.FRand() < 0.35f ? 1 : (Rng.FRand() < 0.5f ? 0 : 2);
		if (!AddRock(At, Size, ETDSpaceFieldKind::Cluster, Rng, Fam)) { continue; }
		++Anchors;
		for (int32 g = 0, n = Rng.RandRange(3, 6); g < n; ++g)
		{
			Gravel += AddRock(At + FVector(Rng.GetUnitVector()) * Rng.FRandRange(Size * 0.6f, Size * 0.6f + 8000.f), Power(Rng, 300.f, 1500.f, 2.f), ETDSpaceFieldKind::Cluster, Rng, Fam) ? 1 : 0;
		}
	}
	RegionCentres.Add(7, Zone + FVector(OuterDarkInner, 0.f, 0.f));
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: Outer Dark %d lone rocks, %d gravel, %.1f-%.1f km"), Anchors, Gravel, OuterDarkInner / 100000.f, Outer / 100000.f);
}
