// Triad Drift - the space field's region dressing (Teammate 6): the effects builder's range beacons on the region set
// pieces and a faint coloured haze per region, so each named place reads from the route. Both loaded soft and
// null-safe (the field works without them); td.SpaceField.Dressing 0 hides them at once (the look before them).

#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace TDSpaceFieldDressing
{
	static int32 GDressing = 0;
	static FAutoConsoleVariableRef CVarDressing(TEXT("td.SpaceField.Dressing"), GDressing,
		TEXT("Region beacons and haze (the effects builder's NS_TD_Region_Beacon and M_TD_RegionHaze): 0 off (default: the Lead has not accepted the assets yet), 1 on (built the first time it turns on). Live."));

	/** The effects builder's colours per region (docs/eng/SPACE-DESIGN.md numbers). */
	static FLinearColor BeaconColor(int32 Region)
	{
		switch (Region)
		{
		case 1: return FLinearColor(1.0f, 0.62f, 0.18f);	// Watchline: amber
		case 4: return FLinearColor(0.70f, 0.85f, 1.0f);	// Quiet Graveyard: cold blue-white
		case 5: return FLinearColor(1.0f, 0.10f, 0.04f);	// Survey Scar: Null red
		case 6: return FLinearColor(1.0f, 0.38f, 0.08f);	// Shattered Moons: ember
		default: return FLinearColor::White;
		}
	}
}

bool ATDSpaceField::IsDressingWanted()
{
	return TDSpaceFieldDressing::GDressing != 0;
}

void ATDSpaceField::BuildRegionDressing()
{
	using namespace TDSpaceFieldDressing;
	if (bDressingBuilt) { return; }
	bDressingBuilt = true;
	UNiagaraSystem* Beacon = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Region_Beacon.NS_TD_Region_Beacon"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	UMaterialInterface* Haze = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TriadDrift/Gameplay/VFX/Materials/M_TD_RegionHaze.M_TD_RegionHaze"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"), nullptr, LOAD_Quiet | LOAD_NoWarn);

	// Beacons: one per region set piece, at the top of its bounds.
	if (Beacon)
	{
		for (const FBeaconPoint& B : BeaconPoints)
		{
			UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Beacon, B.Location, FRotator::ZeroRotator, FVector(1.f),
				/*bAutoDestroy*/ false, /*bAutoActivate*/ true, ENCPoolMethod::None);
			if (NC)
			{
				NC->SetVariableLinearColor(TEXT("User.Color"), BeaconColor(B.Region));
				RegionBeacons.Add(NC);
			}
		}
	}
	// Haze: a scaled engine sphere per region with a dynamic instance of the haze material.
	struct FHaze { int32 Region; float Radius; FLinearColor Color; };
	static const FHaze Hazes[] = {
		{ 4, 110000.f, FLinearColor(0.70f, 0.85f, 1.0f) },	// Quiet Graveyard
		{ 5, 100000.f, FLinearColor(1.0f, 0.12f, 0.05f) },	// Survey Scar
		{ 3, 130000.f, FLinearColor(0.55f, 0.85f, 1.0f) },	// Ice Drift
		{ 7, 250000.f, FLinearColor(0.30f, 0.15f, 0.55f) } };	// Outer Dark
	if (Haze && Sphere)
	{
		for (const FHaze& H : Hazes)
		{
			const FVector* C = RegionCentres.Find(H.Region);
			if (!C) { continue; }
			UStaticMeshComponent* S = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("RegionHaze_%d"), H.Region));
			S->SetupAttachment(RootComponent);
			S->SetMobility(EComponentMobility::Movable);
			S->SetStaticMesh(Sphere);
			S->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			S->SetCastShadow(false);
			S->SetCanEverAffectNavigation(false);
			S->bAffectDistanceFieldLighting = false;
			S->SetWorldLocation(*C);
			S->SetWorldScale3D(FVector(H.Radius / 50.f));	// the engine sphere is 100 cm across
			S->RegisterComponent();
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Haze, S))
			{
				MID->SetVectorParameterValue(TEXT("Color"), H.Color);
				MID->SetScalarParameterValue(TEXT("Density"), HazeDensity);
				S->SetMaterial(0, MID);
			}
			RegionHazes.Add(S);
		}
	}
	bDressingShown = true;
	SetRegionDressingVisible(IsDressingWanted());
	UE_LOG(LogTemp, Log, TEXT("TDSpaceField: region dressing %d beacons (%s), %d haze volumes (%s)"), RegionBeacons.Num(), Beacon ? TEXT("loaded") : TEXT("missing"),
		RegionHazes.Num(), Haze ? TEXT("loaded") : TEXT("missing"));
}

void ATDSpaceField::EnsureRegionDressing()
{
	BuildRegionDressing();
}

void ATDSpaceField::SetRegionDressingVisible(bool bOn)
{
	bDressingShown = bOn;
	for (UNiagaraComponent* NC : RegionBeacons) { if (NC) { NC->SetVisibility(bOn); if (bOn) { NC->Activate(); } else { NC->Deactivate(); } } }
	for (UStaticMeshComponent* S : RegionHazes) { if (S) { S->SetVisibility(bOn); } }
}
