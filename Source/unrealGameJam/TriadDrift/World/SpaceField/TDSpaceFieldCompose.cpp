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
	RegionCentres.Add(2, Zone);

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
		const float Size = Power(Rng, 400.f, MaxSize, 2.8f);
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
		if (Region == 1 || Region == 3 || Region == 4 || Region == 5 || Region == 6 || Region == 7)
		{
			// Around the region's centre: 5 km and 1 km out on the side away from the zone, 25 degrees up; inside = 150 m.
			const FVector* Found = Field->RegionCentres.Find(Region);
			const FVector C = Found ? *Found : (Region == 1 ? FVector(170000.f, 0.f, 0.f) : (Region == 4 ? FVector(300000.f, 150000.f, 20000.f) : FVector(600000.f, 0.f, 0.f)));
			FVector Out = C.GetSafeNormal2D();
			if (Out.IsNearlyZero()) { Out = FVector::ForwardVector; }
			if (Region == 7 || Region == 6) { Out = -Out; }	// seen from the zone side (the Moons: along the route to the Dominion)	// the Outer Dark is seen from the zone side, looking out (5 km beyond it is outside the play bounds)
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

	/** Range 3: the point of nearest approach on the routes the player flies, looking half along the route, half at the region. */
	static bool RouteView(const ATDSpaceField* Field, int32 Region, FVector& OutFrom, FVector& OutAt)
	{
		const FVector* C = Field->RegionCentres.Find(Region);
		if (!C) { return false; }
		float Best = TNumericLimits<float>::Max();
		for (const TArray<FVector>& Lane : Field->GetRouteLanes())
		{
			const FVector P = FMath::ClosestPointOnSegment(*C, Lane[0], Lane[1]);
			const float D = (float)FVector::Dist(P, *C);
			if (D < Best)
			{
				Best = D;
				const FVector Fwd = (Lane[1] - Lane[0]).GetSafeNormal();
				OutFrom = P - Fwd * 20000.f;	// a little before the nearest point
				OutAt = OutFrom + (Fwd + (*C - P).GetSafeNormal()).GetSafeNormal() * 100000.f;
			}
		}
		if (Best < 30000.f)
		{
			// The region sits on the route (the Shattered Moons): look at its nearest set piece instead of straight ahead.
			float Near = TNumericLimits<float>::Max();
			FVector Aim = OutAt;
			for (const FTDSpaceFieldPiece& S : Field->GetStructures())
			{
				const float D = (float)FVector::Dist(S.Location, OutFrom);
				if (FVector::Dist(S.Location, *C) < 200000.f && D > 30000.f && D < Near) { Near = D; Aim = S.Location; }
			}
			OutAt = Aim;
		}
		return Best < TNumericLimits<float>::Max();
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdView(TEXT("TD.SpaceField.ViewRegion"),
		TEXT("Captures: park the flown ship at a region's fixed vantage, facing it: TD.SpaceField.ViewRegion <region 1-7 of docs/eng/SPACE-DESIGN.md> <0 = 5 km | 1 = 1 km | 2 = inside | 3 = from the route>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDSpaceField* Field = ATDSpaceField::Get(World);
			ATDSpacecraft* Ship = World ? Cast<ATDSpacecraft>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
			FVector From, At;
			const int32 Region = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 2;
			const int32 Range = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
			if (!Field || !Ship || !(Range == 3 ? RouteView(Field, Region, From, At) : RegionView(Field, Region, Range, From, At))) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.ViewRegion: no field / ship / view for region %d"), Region); return; }
			const FRotator Facing = (At - From).Rotation();
			Ship->TeleportSpacecraft(From, Facing, /*bKeepSpeed*/ false);
			GEngine->Exec(World, *FString::Printf(TEXT("TD.Input.TurnToward %.2f %.2f 2"), Facing.Yaw, Facing.Pitch));
			UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.ViewRegion %d %d: from %s facing %s"), Region, Range, *From.ToCompactString(), *Facing.ToCompactString());
		}));

	static FAutoConsoleCommandWithWorldAndArgs CmdJourneyCheck(TEXT("TD.SpaceField.JourneyCheck"),
		TEXT("Log, for each of T4's journey keep-outs and routes, the rocks, set pieces and other obstacles that sit inside it (live)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (const ATDSpaceField* Field = ATDSpaceField::Get(World)) { Field->LogJourneyCheck(); }
		}));

	static FAutoConsoleCommandWithWorldAndArgs CmdJourneyStep(TEXT("TD.SpaceField.JourneyStep"),
		TEXT("Captures: park the flown ship on journey route <index> at fraction <t 0-1>, facing along it: TD.SpaceField.JourneyStep <index> <t>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const ATDSpaceField* Field = ATDSpaceField::Get(World);
			ATDSpacecraft* Ship = World ? Cast<ATDSpacecraft>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
			const int32 Index = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
			const float T = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.f;
			FVector From; FRotator Facing; FName Id;
			if (!Field || !Ship || !Field->JourneyPose(Index, T, From, Facing, Id)) { UE_LOG(LogTemp, Warning, TEXT("TD.SpaceField.JourneyStep: no field / ship / route %d"), Index); return; }
			Ship->TeleportSpacecraft(From, Facing, /*bKeepSpeed*/ false);
			GEngine->Exec(World, *FString::Printf(TEXT("TD.Input.TurnToward %.2f %.2f 2"), Facing.Yaw, Facing.Pitch));
			UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.JourneyStep %d (%s) %.2f: from %s facing %s"), Index, *Id.ToString(), T, *From.ToCompactString(), *Facing.ToCompactString());
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

bool ATDSpaceField::AddStructure(UStaticMesh* Mesh, const FTransform& T, int32 BeaconRegion)
{
	// The structures builder's set pieces (real scale, 1-2 convex hulls each): solid with their own collision, never
	// broken (Lead: nothing is passed through; they are landmarks), and a chain of avoidance spheres for the AI.
	if (!Mesh) { return false; }
	const FBox Box = Mesh->GetBoundingBox();
	const FVector Ext = Box.GetExtent() * T.GetScale3D();
	if (!IsPlacementAllowed(T.GetLocation(), Ext.Size() * 2.f, true)) { return false; }
	if (IsInDockingCone(T.GetLocation(), Ext.Size())) { return false; }
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
	if (BeaconRegion > 0)
	{
		BeaconPoints.Add({ T.TransformPosition(Box.GetCenter()) + FVector::UpVector * (Ext.Z + 500.f), BeaconRegion });	// above its top
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
	const FVector A = Zone + VDir * 320000.f + Side * Sign * 45000.f + FVector(0.f, 0.f, 12000.f);
	const FVector B = Zone + VDir * 380000.f + Side * Sign * 55000.f + FVector(0.f, 0.f, 20000.f);
	UStaticMesh* Hulls[] = { LoadSetPiece(TEXT("SM_SP_Research_A")), LoadSetPiece(TEXT("SM_SP_Research_B")) };
	RegionCentres.Add(4, (A + B) * 0.5f);
	int32 Placed = 0;
	for (int32 i = 0; i < GraveyardHulls; ++i)
	{
		const float T = GraveyardHulls > 1 ? float(i) / (GraveyardHulls - 1) : 0.5f;
		// Lying roughly along the line, each a little turned and rolled: a fleet that went quiet in formation. Up to eight
		// nearby spots per hull (a little along the line, a little further from the route) before giving up.
		const FRotator R = FRotator(Rng.FRandRange(-15.f, 15.f), (B - A).Rotation().Yaw + Rng.FRandRange(-30.f, 30.f), Rng.FRandRange(-60.f, 60.f));
		for (int32 Try = 0; Try < 8; ++Try)
		{
			const FVector P = FMath::Lerp(A, B, FMath::Clamp(T + Rng.FRandRange(-0.08f, 0.08f), -0.1f, 1.1f)) + Side * Sign * (Try * 5000.f) + FVector(Rng.GetUnitVector()) * 6000.f;
			if (AddStructure(Hulls[i % 2], FTransform(R, P)))
			{
				++Placed;
				ScatterDebris(P, 12000.f, 35, 1500.f, Rng);
				break;
			}
		}
	}
	// The fleet's debris along its line: hull fragments and trails, thicker near each hull (150-300 pieces).
	int32 Frags = 0;
	for (int32 Try = 0; Frags < GraveyardFragments && Try < GraveyardFragments * 4; ++Try)
	{
		const TArray<TObjectPtr<UStaticMesh>>& Pool = Families[Rng.FRand() < 0.75f ? 5 : 4];
		if (Pool.Num() == 0) { continue; }
		const FVector At = FMath::Lerp(A, B, Rng.FRandRange(-0.15f, 1.15f)) + FVector(Gauss(Rng) * 9000.f, Gauss(Rng) * 9000.f, Gauss(Rng) * 5000.f);
		Frags += AddPiece(Pool[Rng.RandRange(0, Pool.Num() - 1)], At, FRotator(Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f)),
			Power(Rng, 300.f, 3000.f, 2.2f), ETDSpaceFieldKind::Debris, false) ? 1 : 0;
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

// ------------------------------------------------------------------------------ regions 3, 5, 6 and the region set pieces

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDockingBay.h"

namespace TDSpaceFieldCompose
{
	static UStaticMesh* LoadKitPiece(const TCHAR* Name)
	{
		const FString Path = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Props/Kit/Space/%s.%s"), Name, Name);
		return LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
	}

	/** A point on a quadratic Bezier (A, control C, B). */
	static FVector Bezier(const FVector& A, const FVector& C, const FVector& B, float T)
	{
		const float U = 1.f - T;
		return A * (U * U) + C * (2.f * U * T) + B * (T * T);
	}
}

void ATDSpaceField::BuildIceDrift(FRandomStream& Rng)
{
	// Region 3, the Ice Drift: a pale stream of ice and teal crystal curving from the belt toward Verdance, on the side of
	// the Verdance bearing away from the Quiet Graveyard, clear of the route (the lane keep-outs) and of the convoy tube.
	using namespace TDSpaceFieldCompose;
	const FTransform Xf = Layout->GetActorTransform();
	const FTDGalaxyLayoutData& L = Layout->GetLayout();
	const FVector Zone = Xf.TransformPosition(L.CombatZoneCenter);
	FVector VLoc = Zone + FVector(450000.f, 260000.f, 0.f);
	float VAtmo = 72000.f;
	for (const FTDPlanetSpec& Pl : L.Planets) { if (Pl.Destination == ETDDestination::Verdance) { VLoc = Xf.TransformPosition(Pl.Location); VAtmo = Pl.AtmosphereRadius; } }
	const FVector VDir = (VLoc - Zone).GetSafeNormal();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, VDir).GetSafeNormal();
	const float GraveSign = RegionCentres.Contains(4) ? FMath::Sign((float)FVector::DotProduct(RegionCentres[4] - Zone, Side)) : 1.f;
	const float S = -GraveSign;	// the other side
	const FVector Head = Zone + VDir * (BeltOuter + 25000.f) + Side * S * 40000.f;
	const FVector Tail = VLoc - VDir * (VAtmo * 1.5f + 30000.f) + Side * S * 50000.f + FVector(0.f, 0.f, -15000.f);
	const FVector Ctrl = (Head + Tail) * 0.5f + Side * S * 40000.f;
	int32 Placed = 0;
	for (int32 Try = 0; Placed < IceDriftRocks && Try < IceDriftRocks * 6; ++Try)
	{
		const float T = Rng.FRand();
		const FVector On = Bezier(Head, Ctrl, Tail, T);
		const FVector Tangent = (Bezier(Head, Ctrl, Tail, FMath::Min(1.f, T + 0.01f)) - Bezier(Head, Ctrl, Tail, FMath::Max(0.f, T - 0.01f))).GetSafeNormal();
		const FVector N1 = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
		const FVector N2 = FVector::CrossProduct(N1, Tangent).GetSafeNormal();
		// Thin, ribbon-like cross-section (wider than tall), thinning toward the tail; big rocks at the head.
		const float W = 12000.f * (1.f - 0.5f * T), H = 4000.f * (1.f - 0.5f * T);
		const FVector At = On + N1 * Gauss(Rng) * W + N2 * Gauss(Rng) * H;
		const float Size = Power(Rng, 300.f, FMath::Lerp(7000.f, 1200.f, T), 3.f);
		const int32 Fam = Rng.FRand() < 0.8f ? 1 : 3;	// ice, a few teal crystal (the specials keep their own look)
		Placed += AddRock(At, Size, ETDSpaceFieldKind::Cluster, Rng, Fam == 3 ? 1 : Fam) ? 1 : 0;	// a stream, not the belt
	}
	RegionCentres.Add(3, Bezier(Head, Ctrl, Tail, 0.5f));
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: Ice Drift %d rocks (side %+.0f)"), Placed, S);
}

void ATDSpaceField::BuildSurveyScar(FRandomStream& Rng)
{
	// Region 5, the Survey Scar: on the Cinder approach, a mined-out field of metal and violet-crystal rock round the
	// structures builder's split-rock rig, two Null rigs on anchored host rocks, hollowed rocks, and a rubble stream trailing
	// toward Cinder (the yield hauled to the dig). The zone -> Cinder route stays clear (the lane keep-outs).
	using namespace TDSpaceFieldCompose;
	const FTransform Xf = Layout->GetActorTransform();
	const FTDGalaxyLayoutData& L = Layout->GetLayout();
	const FVector Zone = Xf.TransformPosition(L.CombatZoneCenter);
	FVector CLoc = Zone + FVector(-420000.f, 330000.f, 40000.f);
	float CAtmo = 66000.f;
	for (const FTDPlanetSpec& Pl : L.Planets) { if (Pl.Destination == ETDDestination::Cinder) { CLoc = Xf.TransformPosition(Pl.Location); CAtmo = Pl.AtmosphereRadius; } }
	const FVector CDir = (CLoc - Zone).GetSafeNormal();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, CDir).GetSafeNormal();
	const FVector Centre = Zone + CDir * 360000.f + Side * 50000.f + FVector(0.f, 0.f, 10000.f);
	RegionCentres.Add(5, Centre);
	// The centrepiece: the split-rock survey rig.
	// Scales from the structures builder (Lead, Nov 14): each piece subtends 3+ degrees at 1.5 km.
	AddStructure(LoadKitPiece(TEXT("SM_KS_ScarRig")), FTransform(FRotator(Rng.FRandRange(-10.f, 10.f), (Zone - Centre).Rotation().Yaw, 0.f), Centre, FVector(2.f)), 5);
	// Two Null rigs, each on an anchored host rock (the structures builder pairs SM_Asteroid_03 at scale 80 with the rig).
	UStaticMesh* Host = Families[0].Num() > 2 ? Families[0][2].Get() : (Families[0].Num() > 0 ? Families[0][0].Get() : nullptr);
	UStaticMesh* Rig = LoadSetPiece(TEXT("SM_SP_NullRig"));
	for (int32 r = 0; r < 2; ++r)
	{
		const FVector At = Centre + (FVector(Rng.GetUnitVector()) * FVector(1.f, 1.f, 0.4f)).GetSafeNormal() * Rng.FRandRange(25000.f, 40000.f);
		if (!Host || !AddStructure(Host, FTransform(FRotator::ZeroRotator, At, FVector(80.f)))) { continue; }
		const float Top = Host->GetBounds().BoxExtent.Z * 80.f;
		AddStructure(Rig, FTransform(FRotator::ZeroRotator, At + FVector(0.f, 0.f, Top * 0.85f)));
	}
	// Hollowed rocks round the field.
	UStaticMesh* Hollow = LoadSetPiece(TEXT("SM_SP_HollowRock"));
	TArray<FVector> Husks;
	for (int32 h = 0, Try = 0; h < 8 && Try < 40; ++Try)
	{
		const FVector At = Centre + (FVector(Rng.GetUnitVector()) * FVector(1.f, 1.f, 0.5f)).GetSafeNormal() * Rng.FRandRange(16000.f, 42000.f);
		if (AddStructure(Hollow, FTransform(FRotator(Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f), Rng.FRandRange(-180.f, 180.f)), At))) { Husks.Add(At); ++h; }
	}
	// Cable lines from the rig to each husk: thin truss segments every 12 m (the yield is hauled along them).
	UStaticMesh* Cable = Families.IsValidIndex(5) && Families[5].Num() > 0 ? Families[5][0].Get() : nullptr;
	for (const FVector& H : Husks)
	{
		const float Len = (float)FVector::Dist(Centre, H);
		const FRotator Along = (H - Centre).Rotation();
		for (float D = 6000.f; Cable && D < Len - 4000.f; D += 1200.f)
		{
			AddPiece(Cable, FMath::Lerp(Centre, H, D / Len), Along, 1100.f, ETDSpaceFieldKind::Debris, false);
		}
	}
	// The mined-out field: metal and violet crystal, an elongated blob along the approach.
	int32 Rocks = 0;
	for (int32 Try = 0; Rocks < ScarRocks && Try < ScarRocks * 5; ++Try)
	{
		const FVector At = Centre + CDir * Gauss(Rng) * 30000.f + Side * Gauss(Rng) * 18000.f + FVector::UpVector * Gauss(Rng) * 9000.f;
		const float Size = Power(Rng, 400.f, 8000.f, 3.f);
		Rocks += AddRock(At, Size, ETDSpaceFieldKind::Cluster, Rng, Rng.FRand() < 0.75f ? 2 : 0) ? 1 : 0;
	}
	// The rubble stream: from the scar toward Cinder, thinning, stopping short of the atmosphere.
	const FVector StreamEnd = CLoc - CDir * (CAtmo * 1.4f + 20000.f) + Side * 40000.f;
	int32 Rubble = 0;
	for (int32 Try = 0; Rubble < ScarRubble && Try < ScarRubble * 5; ++Try)
	{
		const float T = Rng.FRand();
		const FVector At = FMath::Lerp(Centre, StreamEnd, T) + FVector(Rng.GetUnitVector()) * 5000.f * (1.f - 0.6f * T);
		Rubble += AddRock(At, Power(Rng, 300.f, 2500.f, 3.f), ETDSpaceFieldKind::Debris, Rng, 2) ? 1 : 0;
	}
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: Survey Scar %d rocks, %d rubble"), Rocks, Rubble);
}

void ATDSpaceField::BuildShatteredMoons(FRandomStream& Rng)
{
	// Region 6, the Shattered Moons: broken moon shards lining the last stretch of the route to the Dominion, so the
	// player flies between them on the approach. T4's finer keep-outs instead of the 3.5 km sphere: MoonDominionClear from
	// the Dominion (its 720 m field, the generators on a 389 m ring, the turrets), 400 m + the shard from the bow axis out
	// to 5 km (the escape flight), 600 m from the approach stage start, and 300 m + the shard from the route line.
	// Fracture faces (+X) turned toward the route; gravel in each shard's lee.
	using namespace TDSpaceFieldCompose;
	const ATDDominion* Dom = ATDDominion::Get(this);
	const FTransform Xf = Layout->GetActorTransform();
	const FTDGalaxyLayoutData& L = Layout->GetLayout();
	const FVector Zone = Xf.TransformPosition(L.CombatZoneCenter);
	const FVector DLoc = Dom ? Dom->GetActorLocation() : Xf.TransformPosition(L.Dominion.GetLocation());
	const FVector Bow = Dom ? Dom->GetActorForwardVector() : (Zone - DLoc).GetSafeNormal();
	const FVector Stage = Dom ? Dom->GetStageStart(false).GetLocation() : DLoc + Bow * 200000.f;
	const FVector Route = (DLoc - Zone).GetSafeNormal();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Route).GetSafeNormal();
	UStaticMesh* Shards[] = { LoadKitPiece(TEXT("SM_KS_MoonShard_A")), LoadKitPiece(TEXT("SM_KS_MoonShard_B")) };
	TGuardValue<bool> MoonRules(bMoonPlacement, true);
	int32 Placed = 0;
	for (int32 Try = 0; Placed < MoonShards && Try < MoonShards * 60; ++Try)
	{
		UStaticMesh* M = Shards[Placed % 2];
		if (!M) { break; }
		const float Half = M->GetBounds().SphereRadius;
		// Along the route from 1.9 to 3.6 km short of the Dominion, alternating sides, 300 m + the shard to 1.1 km out.
		const float FromDom = Rng.FRandRange(MoonDominionClear + Half + 10000.f, 360000.f);
		const float Out = 30000.f + Half + Rng.FRandRange(0.f, 80000.f);
		const FVector At = DLoc - Route * FromDom + Side * ((Placed % 2) ? 1.f : -1.f) * Out + FVector(0.f, 0.f, Rng.FRandRange(-40000.f, 40000.f));
		const FVector OnBow = FMath::ClosestPointOnSegment(At, DLoc, DLoc + Bow * 500000.f);
		if (FVector::Dist(At, OnBow) < 40000.f + Half) { continue; }
		if (FVector::Dist(At, Stage) < 60000.f + Half) { continue; }
		const FVector OnRoute = FMath::ClosestPointOnSegment(At, Zone, DLoc);
		const FRotator Face = (OnRoute - At).Rotation();
		if (!AddStructure(M, FTransform(FRotator(Face.Pitch, Face.Yaw, Rng.FRandRange(-40.f, 40.f)), At), 6)) { continue; }
		++Placed;
		const FVector Lee = At + (At - OnRoute).GetSafeNormal() * (Half + 8000.f);
		for (int32 g = 0; g < 25; ++g)
		{
			AddRock(Lee + FVector(Rng.GetUnitVector()) * Rng.FRandRange(0.f, 15000.f), Power(Rng, 400.f, 3500.f, 2.5f), ETDSpaceFieldKind::Cluster, Rng, 0);
		}
	}
	RegionCentres.Add(6, DLoc - Route * 270000.f);
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: Shattered Moons %d shards along the Dominion route"), Placed);
}

void ATDSpaceField::PlaceRegionCentrepieces(FRandomStream& Rng)
{
	// The structures builder's region pieces (Props/Kit/Space): the Watchline's broken picket ring and the Quiet
	// Graveyard's cruiser hulk in three sections. Solid and indestructible like every set piece.
	using namespace TDSpaceFieldCompose;
	const FVector Zone = Layout->GetActorTransform().TransformPosition(Layout->GetLayout().CombatZoneCenter);
	if (const FVector* Halo = RegionCentres.Find(1))
	{
		// Beside the first patrol wreck, a little further out, its arc facing the zone.
		const FVector Out = (*Halo - Zone).GetSafeNormal();
		const FVector At = *Halo + Out * 12000.f + FVector::CrossProduct(FVector::UpVector, Out).GetSafeNormal() * 15000.f;
		AddStructure(LoadKitPiece(TEXT("SM_KS_PicketRing")), FTransform(FRotator(Rng.FRandRange(-15.f, 15.f), (Zone - At).Rotation().Yaw, Rng.FRandRange(-20.f, 20.f)), At, FVector(2.f)), 1);
	}
	if (const FVector* Grave = RegionCentres.Find(4))
	{
		// The hulk in three sections about 200 m apart along a broken line, beyond the research hulls.
		const FVector Out = (*Grave - Zone).GetSafeNormal();
		const FVector Base = *Grave + Out * 30000.f + FVector(0.f, 0.f, 10000.f);
		const FVector Along = FVector::CrossProduct(FVector::UpVector, Out).GetSafeNormal();
		const TCHAR* Parts[] = { TEXT("SM_KS_Hulk_Fore"), TEXT("SM_KS_Hulk_Mid"), TEXT("SM_KS_Hulk_Aft") };
		for (int32 i = 0; i < 3; ++i)
		{
			const FVector At = Base + Along * (i - 1) * 30000.f + FVector(Rng.GetUnitVector()) * 3500.f;	// 1.5x: sections and spacing
			AddStructure(LoadKitPiece(Parts[i]), FTransform(FRotator(Rng.FRandRange(-12.f, 12.f), Along.Rotation().Yaw + Rng.FRandRange(-15.f, 15.f), Rng.FRandRange(-35.f, 35.f)), At, FVector(1.5f)), 4);
		}
	}
}

namespace TDSpaceFieldCompose
{
	static FAutoConsoleCommandWithWorldAndArgs CmdRouteReport(TEXT("TD.SpaceField.RouteReport"),
		TEXT("Log, for each region, its nearest approach from every route the player flies (zone to each planet, the Dominion and each gate end), and how large its biggest set piece looks from there"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const ATDSpaceField* Field = ATDSpaceField::Get(World);
			if (!Field) { return; }
			Field->LogRouteReport();
		}));
}

void ATDSpaceField::LogRouteReport() const
{
	static const TCHAR* Names[] = { TEXT(""), TEXT("Watchline halo"), TEXT("Shepherd Belt"), TEXT("Ice Drift"), TEXT("Quiet Graveyard"), TEXT("Survey Scar"), TEXT("Shattered Moons"), TEXT("Outer Dark") };
	static const TCHAR* Routes[] = { TEXT("zone->Verdance"), TEXT("zone->Cinder"), TEXT("zone->Dominion") };
	for (int32 R = 1; R <= 7; ++R)
	{
		const FVector* C = RegionCentres.Find(R);
		if (!C) { UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.RouteReport: %d %s not built"), R, Names[R]); continue; }
		// The region's biggest set piece within 1.2 km of its centre (its silhouette), else 100 m.
		float Big = 10000.f;
		for (const FTDSpaceFieldPiece& S : Structures) { if (FVector::Dist(S.Location, *C) < 120000.f) { Big = FMath::Max(Big, S.Size); } }
		FString Line;
		float Nearest = TNumericLimits<float>::Max();
		for (int32 L = 0; L < BuoyLanes && L < Lanes.Num(); ++L)
		{
			const FVector P = FMath::ClosestPointOnSegment(*C, Lanes[L][0], Lanes[L][1]);
			const float D = (float)FVector::Dist(P, *C);
			Nearest = FMath::Min(Nearest, D);
			if (L < 3) { Line += FString::Printf(TEXT(" | %s %.1f km (%.1f deg)"), Routes[L], D / 100000.f, FMath::RadiansToDegrees(2.f * FMath::Atan(Big * 0.5f / FMath::Max(D, 1.f)))); }
		}
		UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.RouteReport: %d %s, centre %.1f km from the zone, biggest piece %.0f m, nearest route %.1f km%s"), R, Names[R],
			FVector::Dist(*C, BeltCenter) / 100000.f, Big / 100.f, Nearest / 100000.f, *Line);
	}
}

int32 ATDSpaceField::LogJourneyCheck() const
{
	if (!Layout) { return 0; }
	TArray<FTDJourneyKeyPoint> Journey;
	Layout->GetJourneyKeyPoints(Journey);
	const UTDWorldSubsystem* WS = UTDWorldSubsystem::Get(this);
	int32 Total = 0, RouteIdx = 0;
	for (const FTDJourneyKeyPoint& K : Journey)
	{
		if (K.Kind == ETDJourneyPointKind::Pocket) { continue; }
		int32 Field = 0, Adopted = 0, Sets = 0, Other = 0;
		FString First;
		for (const FTDSpaceFieldPiece& P : Pieces)
		{
			if (K.DistanceTo(P.Location) < P.Size * 0.5f)
			{
				++(P.bAdopted ? Adopted : Field);
				if (First.IsEmpty()) { First = FString::Printf(TEXT("%s rock %.0f m at %s"), P.bAdopted ? TEXT("adopted") : TEXT("field"), P.Size / 100.f, *P.Location.ToCompactString()); }
			}
		}
		for (const FTDSpaceFieldPiece& S : Structures)
		{
			if (K.DistanceTo(S.Location) < S.Size * 0.5f) { ++Sets; if (First.IsEmpty()) { First = FString::Printf(TEXT("set piece %.0f m at %s"), S.Size / 100.f, *S.Location.ToCompactString()); } }
		}
		if (WS)
		{
			// Every obstacle sphere in the galaxy (the field's own are counted above too; this catches ruins, wrecks, the layout's).
			for (const FSphere& O : WS->GetObstacles())
			{
				if (O.W >= 0.f && K.DistanceTo(O.Center) < O.W) { ++Other; if (First.IsEmpty()) { First = FString::Printf(TEXT("obstacle r %.0f m at %s"), O.W / 100.f, *O.Center.ToCompactString()); } }
			}
		}
		const bool bRoute = K.Kind == ETDJourneyPointKind::Route;
		UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.JourneyCheck: %s%s %s (r %.0f m%s): field %d, adopted %d, set pieces %d, obstacles %d%s%s"),
			bRoute ? *FString::Printf(TEXT("route %d "), RouteIdx) : TEXT(""), bRoute ? TEXT("") : TEXT("keep-out"), *K.Id.ToString(), K.Radius / 100.f,
			bRoute ? *FString::Printf(TEXT(", %.1f km"), K.Length / 100000.f) : TEXT(""), Field, Adopted, Sets, Other, First.IsEmpty() ? TEXT("") : TEXT(" | first: "), *First);
		Total += Field + Adopted + Sets;
		RouteIdx += bRoute ? 1 : 0;
	}
	UE_LOG(LogTemp, Display, TEXT("TD.SpaceField.JourneyCheck: %d field rocks and set pieces inside the journey"), Total);
	return Total;
}

bool ATDSpaceField::JourneyPose(int32 Index, float T, FVector& OutFrom, FRotator& OutFacing, FName& OutId) const
{
	if (!Layout) { return false; }
	TArray<FTDJourneyKeyPoint> Journey;
	Layout->GetJourneyKeyPoints(Journey);
	int32 N = 0;
	for (const FTDJourneyKeyPoint& K : Journey)
	{
		if (K.Kind != ETDJourneyPointKind::Route) { continue; }
		if (N++ != Index) { continue; }
		OutFrom = K.Location + K.Direction * K.Length * FMath::Clamp(T, 0.f, 1.f);
		OutFacing = K.Direction.Rotation();
		OutId = K.Id;
		return true;
	}
	return false;
}

bool ATDSpaceField::IsInDockingCone(const FVector& P, float Radius) const
{
	// T4: the tractor cone (the bay's +X out of the mouth, ConeHalfAngleDeg out to ConeDepth) and the near-field capture
	// (up to 80 degrees within 400 m). No set piece in it, so a pilot lining up from the side never meets a shard.
	const ATDDominion* Dom = ATDDominion::Get(this);
	const ATDDockingBay* Bay = Dom ? Dom->GetDockingBay() : nullptr;
	if (!Bay) { return false; }
	const FVector Mouth = Bay->GetActorLocation();
	const FVector Fwd = Bay->GetActorForwardVector();
	const FVector V = P - Mouth;
	const float Along = (float)FVector::DotProduct(V, Fwd);
	const float Lateral = (float)(V - Fwd * Along).Size();
	if (V.Size() < 40000.f + Radius && Along > -Radius && Lateral <= FMath::Tan(FMath::DegreesToRadians(80.f)) * FMath::Max(Along, 0.f) + Radius) { return true; }
	if (Along < -Radius || Along > Bay->ConeDepth + Radius) { return false; }
	return Lateral <= FMath::Tan(FMath::DegreesToRadians(Bay->ConeHalfAngleDeg)) * FMath::Max(Along, 0.f) + Radius;
}
