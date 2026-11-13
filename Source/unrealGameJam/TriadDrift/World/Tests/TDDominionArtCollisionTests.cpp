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

#endif
