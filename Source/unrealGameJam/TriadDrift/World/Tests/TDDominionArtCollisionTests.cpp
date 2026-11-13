// ENG-18 with the engineering Dominion kit (DA_Dominion_Visuals): the art hull's UCX collision is larger than the
// old placeholder box (bow to X 396 m, wings +-160 m, stern -334 m). Nothing the assault needs may end up inside it:
// generators, turrets and the reactor must stay hittable, launched fighters must not spawn inside the hull, and the
// trench run must stay open.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Dominion.Art

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace TDDominionArtTest
{
	/** The Dominion's own blocking primitives (hull, bridge, trench walls...), not its attached actors. */
	TArray<UPrimitiveComponent*> Blockers(ATDDominion* D)
	{
		TArray<UPrimitiveComponent*> Out;
		TInlineComponentArray<UPrimitiveComponent*> Prims(D);
		for (UPrimitiveComponent* P : Prims)
		{
			if (P && P->IsCollisionEnabled() && P->IsRegistered()) { Out.Add(P); }
		}
		return Out;
	}

	/** Name of the first Dominion primitive a sphere at Where overlaps, or empty. */
	FString Inside(const TArray<UPrimitiveComponent*>& Prims, const FVector& Where, float Radius)
	{
		for (UPrimitiveComponent* P : Prims)
		{
			if (P->OverlapComponent(Where, FQuat::Identity, FCollisionShape::MakeSphere(Radius)))
			{
				return P->GetName();
			}
		}
		return FString();
	}

	/** Samples a straight flight line every 10 m; returns "" when open, else the first block. */
	FString FirstBlock(const TArray<UPrimitiveComponent*>& Prims, const FVector& From, const FVector& To, float Radius)
	{
		const float Len = FVector::Dist(From, To);
		for (float S = 0.f; S <= Len; S += 1000.f)
		{
			const FString Hit = Inside(Prims, FMath::Lerp(From, To, Len > 0.f ? S / Len : 0.f), Radius);
			if (!Hit.IsEmpty()) { return FString::Printf(TEXT("%s at %.0f m"), *Hit, S / 100.f); }
		}
		return FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDominionArtCollision, "TriadDrift.World.Dominion.ArtCollisionTrapsNothing", TD_TEST_FLAGS)
bool FTDDominionArtCollision::RunTest(const FString&)
{
	using namespace TDDominionArtTest;
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	GI->Progress.bDominionRevealed = true;
	GI->Progress.Phase = ETDGamePhase::DominionAssault;
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
	if (!TestNotNull(TEXT("dominion"), D)) { return false; }
	W.Advance(D->WarpInSeconds + 0.5f);
	const UTDDominionVisuals* V = UTDStatics::LoadPresentation<UTDDominionVisuals>(TEXT("DA_Dominion_Visuals"));
	if (!V || V->HullMesh.IsNull())
	{
		AddInfo(TEXT("no Dominion art in this checkout: placeholder box only"));
	}
	const TArray<UPrimitiveComponent*> Prims = Blockers(D);
	TestTrue(TEXT("the Dominion has collision"), Prims.Num() > 0);

	const float ShipRadius = 300.f;	// a squad ship (220 cm) with a margin
	const FString Hangar = Inside(Prims, D->GetHangarLocation(), ShipRadius);
	TestTrue(*FString::Printf(TEXT("launched fighters appear outside the hull (inside %s)"), *Hangar), Hangar.IsEmpty());
	{
		// Launched fighters leave the hangar bow-first: the first 100 m are open.
		const FVector H = D->GetHangarLocation();
		const FString Exit = FirstBlock(Prims, H, H + D->GetActorForwardVector() * 10000.f, ShipRadius);
		TestTrue(*FString::Printf(TEXT("hangar exit open for 100 m (first block: %s)"), *Exit), Exit.IsEmpty());
	}

	TestEqual(TEXT("four generators built"), D->GetGenerators().Num(), 4);
	TestTrue(TEXT("turrets built"), D->GetTurrets().Num() > 0);
	TestNotNull(TEXT("reactor built"), D->GetReactor());
	TestTrue(TEXT("the art hull is in use"), !V || V->HullMesh.IsNull() || D->FindComponentByClass<UStaticMeshComponent>() != nullptr);
	for (ATDShieldGenerator* G : D->GetGenerators())
	{
		const FString In = Inside(Prims, G->GetActorLocation(), 200.f);
		TestTrue(*FString::Printf(TEXT("%s is not buried (inside %s)"), *G->GetName(), *In), In.IsEmpty());
		// An attack run comes in from outside: the 300 m line from open space straight to the generator is open.
		FVector Out = G->GetActorLocation() - D->GetActorLocation();
		Out.Z = 0.f;
		const FVector Far = G->GetActorLocation() + Out.GetSafeNormal() * 30000.f;
		const FString Run = FirstBlock(Prims, Far, G->GetActorLocation(), ShipRadius);
		TestTrue(*FString::Printf(TEXT("%s attack run open (first block: %s)"), *G->GetName(), *Run), Run.IsEmpty());
	}
	for (ATDEnemyTurret* T : D->GetTurrets())
	{
		const FString In = Inside(Prims, T->GetActorLocation(), 150.f);
		TestTrue(*FString::Printf(TEXT("%s at %s is not buried (inside %s)"), *T->GetName(),
			*D->GetActorTransform().InverseTransformPosition(T->GetActorLocation()).ToCompactString(), *In), In.IsEmpty());
	}
	if (ATDDominionReactor* R = D->GetReactor())
	{
		const FString In = Inside(Prims, R->GetActorLocation(), 200.f);
		TestTrue(*FString::Printf(TEXT("reactor at %s is not buried (inside %s)"),
			*D->GetActorTransform().InverseTransformPosition(R->GetActorLocation()).ToCompactString(), *In), In.IsEmpty());
		// The trench run: from the reactor, the line toward the stern down the middle of the trench stays open until
		// the bridge, which closes the trench's stern end (as the placeholder bridge always did).
		const FVector Start = R->GetActorLocation() - D->GetActorForwardVector() * 3000.f;
		int32 Blocked = 0;
		FString FirstHit;
		for (float S = 0.f; S <= 25000.f; S += 1000.f)
		{
			const FVector P = Start - D->GetActorForwardVector() * S;
			const FString Hit = Inside(Prims, P, ShipRadius);
			if (!Hit.IsEmpty())
			{
				++Blocked;
				if (FirstHit.IsEmpty()) { FirstHit = FString::Printf(TEXT("%s at %.0f m"), *Hit, S / 100.f); }
			}
		}
		TestEqual(*FString::Printf(TEXT("trench run open for 250 m behind the reactor (first block: %s)"), *FirstHit), Blocked, 0);
		// ...and past the reactor to the trench's bow end (the walls run to 0.8 x HullExtent.X).
		const FVector LocalR = D->GetActorTransform().InverseTransformPosition(R->GetActorLocation());
		const FVector Past = R->GetActorLocation() + D->GetActorForwardVector() * 3000.f;
		const FVector BowEnd = D->GetActorTransform().TransformPosition(FVector(D->HullExtent.X * 0.8f, 0.f, LocalR.Z));
		const FString Ahead = FirstBlock(Prims, Past, BowEnd, ShipRadius);
		TestTrue(*FString::Printf(TEXT("trench open from the reactor to the bow end (first block: %s)"), *Ahead), Ahead.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDominionStageStarts, "TriadDrift.World.Dominion.StageStartsInOpenSpace", TD_TEST_FLAGS)
bool FTDDominionStageStarts::RunTest(const FString&)
{
	using namespace TDDominionArtTest;
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	GI->Progress.bDominionRevealed = true;
	GI->Progress.Phase = ETDGamePhase::DominionAssault;
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f), FRotator(0.f, 37.f, 0.f));
	if (!TestNotNull(TEXT("dominion"), D)) { return false; }
	W.Advance(D->WarpInSeconds + 0.5f);
	const TArray<UPrimitiveComponent*> Prims = Blockers(D);
	const FBox Local = D->GetLocalHullBounds();
	TestTrue(*FString::Printf(TEXT("hull bounds measured (%s)"), *Local.GetSize().ToCompactString()), Local.IsValid && Local.GetSize().X > 10000.f);

	const FTransform Approach = D->GetStageStart(false);
	const FVector ALocal = D->GetActorTransform().InverseTransformPosition(Approach.GetLocation());
	TestFalse(TEXT("approach start is outside the hull's box"), Local.ExpandBy(10000.f).IsInside(ALocal));
	// At cruise toward the Dominion's centre, several seconds before anything solid.
	const FString Hit = FirstBlock(Prims, Approach.GetLocation(), Approach.GetLocation() + Approach.GetRotation().GetForwardVector() * 50000.f, 300.f);
	TestTrue(*FString::Printf(TEXT("the first 500 m ahead of the approach start are open (first block: %s)"), *Hit), Hit.IsEmpty());
	TestTrue(TEXT("approach faces the Dominion"), FVector::DotProduct(Approach.GetRotation().GetForwardVector(), (D->GetActorLocation() - Approach.GetLocation()).GetSafeNormal()) > 0.99f);

	const FTransform Trench = D->GetStageStart(true);
	const FVector TLocal = D->GetActorTransform().InverseTransformPosition(Trench.GetLocation());
	TestTrue(TEXT("trench start is off the real bow"), TLocal.X > Local.Max.X + 10000.f);
	TestTrue(*FString::Printf(TEXT("trench start is clear (inside %s)"), *Inside(Prims, Trench.GetLocation(), 600.f)), Inside(Prims, Trench.GetLocation(), 600.f).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDominionBreakupShows, "TriadDrift.World.Dominion.BreakupExplodesTheHullThenVictory", TD_TEST_FLAGS)
bool FTDDominionBreakupShows::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	GI->Progress.bDominionRevealed = true;
	GI->Progress.Phase = ETDGamePhase::DominionAssault;
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
	AActor* Killer = W.Spawn<AActor>(FVector(150000.f, 0.f, 0.f));
	W.Advance(D->ProgressPollSeconds + D->WarpInSeconds + 0.5f);
	for (ATDShieldGenerator* G : D->GetGenerators()) { UTDStatics::ApplyTDDamage(G, 1000000.f, ETDDamageKind::Explosive, Killer, Killer); W.Advance(0.2f); }
	W.Advance(1.f);
	// One phase per hit (each phase opens after a short invulnerable beat), as TD.Director.ReactorKill does.
	for (int32 Hit = 0; Hit < 10 && D->GetReactor() && D->GetState() != ETDDominionState::Breakup; ++Hit)
	{
		UTDStatics::ApplyTDDamage(D->GetReactor(), 100000000.f, ETDDamageKind::Explosive, Killer, Killer);
		W.Advance(1.5f);
	}
	if (!TestEqual(TEXT("breakup started"), (int32)D->GetState(), (int32)ETDDominionState::Breakup)) { return false; }
	const UTDDominionVisuals* V = UTDStatics::LoadPresentation<UTDDominionVisuals>(TEXT("DA_Dominion_Visuals"));
	const bool bArt = V && FTDCombatPlaceholders::HasVisualArt(V->BreakupSequence);
	W.Advance(D->BreakupSeconds - 1.f, 1.f / 30.f);
	if (!bArt)
	{
		TestTrue(*FString::Printf(TEXT("a chain of explosions walks the hull (%d)"), D->GetBreakupBoomCount()), D->GetBreakupBoomCount() >= 8);
		TestTrue(TEXT("final blast"), D->HasBreakupFinalBlast());
		TestTrue(TEXT("the hull is gone after the blast"), D->IsHidden());
	}
	W.Advance(2.f);
	TestTrue(TEXT("victory follows the breakup"), GI->Progress.Phase == ETDGamePhase::Victory);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDominionArtMaterials, "TriadDrift.World.Dominion.ArtHullKeepsItsAuthoredMaterials", TD_TEST_FLAGS)
bool FTDDominionArtMaterials::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
	if (!TestNotNull(TEXT("dominion"), D)) { return false; }
	W.Advance(0.5f);
	int32 Checked = 0;
	TInlineComponentArray<UStaticMeshComponent*> Meshes(D);
	for (UStaticMeshComponent* C : Meshes)
	{
		const UStaticMesh* M = C ? C->GetStaticMesh() : nullptr;
		if (!M || !M->GetName().StartsWith(TEXT("SM_Dominion_"))) { continue; }
		for (int32 Slot = 0; Slot < M->GetStaticMaterials().Num(); ++Slot)
		{
			++Checked;
			TestTrue(*FString::Printf(TEXT("%s slot %d draws the authored material"), *C->GetName(), Slot), C->GetMaterial(Slot) == M->GetMaterial(Slot));
		}
	}
	if (Checked == 0) { AddInfo(TEXT("no Dominion art in this checkout")); }
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDominionTurretArt, "TriadDrift.World.Dominion.TurretsWearTheirArt", TD_TEST_FLAGS)
bool FTDDominionTurretArt::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	GI->Progress.bDominionRevealed = true;
	GI->Progress.Phase = ETDGamePhase::DominionAssault;
	ATDDominion* D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
	if (!TestNotNull(TEXT("dominion"), D)) { return false; }
	W.Advance(D->WarpInSeconds + 0.5f);
	const UTDEnemyVisuals* TV = UTDStatics::LoadPresentation<UTDEnemyVisuals>(TEXT("DA_DominionTurret_Visuals"));
	UStaticMesh* Art = (TV && !TV->TurretMesh.IsNull()) ? TV->TurretMesh.LoadSynchronous() : nullptr;
	TestTrue(TEXT("turrets built"), D->GetTurrets().Num() > 0);
	for (ATDEnemyTurret* T : D->GetTurrets())
	{
		if (!T) { continue; }
		if (Art)
		{
			TestTrue(*FString::Printf(TEXT("%s draws the turret art (Visuals reaches it before Init)"), *T->GetName()), T->GetMesh() && T->GetMesh()->GetStaticMesh() == Art);
		}
	}
	if (!Art) { AddInfo(TEXT("no DA_DominionTurret_Visuals in this checkout: placeholder turrets")); }
	return true;
}

#endif
