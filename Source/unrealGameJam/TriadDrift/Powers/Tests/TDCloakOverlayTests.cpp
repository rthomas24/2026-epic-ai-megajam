// Cloak hull overlay (UTDAbilityFX::HullOverlay, Lead's decision after the Nov 14 play session: the cloak read as a flat
// grey saucer). Teammate 2. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Powers.Ability.CloakOverlay

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Powers/TDAbilityPowers.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDCloakOverlayTest
{
	UTDAbilityFX* MakeFX(UObject* Outer)
	{
		UTDAbilityFX* FX = NewObject<UTDAbilityFX>(Outer);
		FX->HullOverlay = UMaterial::GetDefaultMaterial(MD_Surface);
		return FX;
	}

	UStaticMeshComponent* AddHull(AActor* Owner, FName Name)
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner, Name);
		Mesh->SetupAttachment(Owner->GetRootComponent());
		Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Mesh->RegisterComponent();
		return Mesh;
	}

	float Cloak01(UTDCloakPower* Cloak)
	{
		float V = -1.f;
		if (UMaterialInstanceDynamic* MID = Cloak->GetHullOverlayMID())
		{
			MID->GetScalarParameterValue(FName(TEXT("Cloak")), V);
		}
		return V;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCloakOverlay, "TriadDrift.Powers.Ability.CloakOverlay.SetFadedAndCleared", TD_TEST_FLAGS)
bool FTDCloakOverlay::RunTest(const FString& Parameters)
{
	using namespace TDCloakOverlayTest;
	using namespace TDPowerTest;
	FTDTestWorld W;
	AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(100.f), 100.f, 100.f);
	UStaticMeshComponent* HullA = AddHull(Rift, TEXT("HullA"));
	UStaticMeshComponent* HullB = AddHull(Rift, TEXT("HullB"));
	UMaterialInterface* Previous = UMaterial::GetDefaultMaterial(MD_PostProcess);
	HullB->SetOverlayMaterial(Previous);
	UTDCloakPower* Cloak = AddPower<UTDCloakPower>(Rift);

	// Empty slot: the cloak still works, nothing is overlaid.
	Cloak->FX = NewObject<UTDAbilityFX>(Rift);
	TestTrue(TEXT("no overlay art: cloak on"), Cloak->TryActivate());
	TestEqual(TEXT("no overlay art: no mesh overlaid"), Cloak->GetOverlaidMeshCount(), 0);
	Cloak->ResetPower();

	Cloak->FX = MakeFX(Rift);
	TestTrue(TEXT("cloak on"), Cloak->TryActivate());
	TestEqual(TEXT("both hull meshes overlaid"), Cloak->GetOverlaidMeshCount(), 2);
	TestTrue(TEXT("overlay set on the hull"), HullA->GetOverlayMaterial() != nullptr && HullA->GetOverlayMaterial() == Cloak->GetHullOverlayMID());
	TestTrue(TEXT("starts at Cloak 0"), FMath::IsNearlyZero(Cloak01(Cloak)));
	W.Advance(.12f, 1.f / 60.f);
	TestTrue(*FString::Printf(TEXT("fading in (%.2f)"), Cloak01(Cloak)), Cloak01(Cloak) > .2f && Cloak01(Cloak) < .8f);
	W.Advance(.2f, 1.f / 60.f);
	TestTrue(TEXT("fully in after 0.25 s"), FMath::IsNearlyEqual(Cloak01(Cloak), 1.f));

	// Expiry (6 s): fades back over 0.25 s, then the overlay is gone and the old one restored.
	W.Advance(6.f, 1.f / 60.f);
	TestFalse(TEXT("cloak over"), Cloak->IsCloaked());
	W.Advance(.3f, 1.f / 60.f);
	TestTrue(TEXT("end: overlay removed"), HullA->GetOverlayMaterial() == nullptr && Cloak->GetOverlaidMeshCount() == 0);
	TestTrue(TEXT("end: the hull's own overlay restored"), HullB->GetOverlayMaterial() == Previous);

	// Death: cleared at once.
	Cloak->ResetCooldown();
	TestTrue(TEXT("cloak again"), Cloak->TryActivate());
	W.Advance(.3f, 1.f / 60.f);
	UTDStatics::GetHealth(Rift)->Kill();
	W.Tick(1.f / 60.f);
	TestTrue(TEXT("death: overlay cleared at once"), HullA->GetOverlayMaterial() == nullptr && HullB->GetOverlayMaterial() == Previous);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCloakOverlaySwitch, "TriadDrift.Powers.Ability.CloakOverlay.ShipSwitchKeepsThenClears", TD_TEST_FLAGS)
bool FTDCloakOverlaySwitch::RunTest(const FString& Parameters)
{
	using namespace TDCloakOverlayTest;
	FTDTestWorld W;
	APlayerController* PC = W.SpawnLocalPlayerController();
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>(FVector(0.f, 0.f, -60000.f));
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform(FVector(0.f, 0.f, -60000.f)));
	W.Advance(1.1f, 1.f / 60.f);
	Squad->RequestSwitchTo(ETDShipId::Rift);
	W.Advance(.2f, 1.f / 60.f);
	ATDShip* Rift = Squad->GetShip(ETDShipId::Rift);
	if (!TestTrue(TEXT("flying Rift"), Rift && PC->GetPawn() == Rift))
	{
		return false;
	}
	UTDCloakPower* Cloak = Rift->FindComponentByClass<UTDCloakPower>();
	if (!Cloak)
	{
		Cloak = NewObject<UTDCloakPower>(Rift, TEXT("CloakUnderTest"));
		Cloak->RegisterComponent();
	}
	Cloak->FX = MakeFX(Rift);
	Cloak->ResetPower();
	W.Tick(1.f / 60.f);
	TestTrue(TEXT("cloak on (player flown)"), Cloak->TryActivate());
	TestTrue(TEXT("Rift's hull overlaid"), Cloak->GetOverlaidMeshCount() > 0);
	W.Advance(1.2f, 1.f / 60.f); // past the switch cooldown
	Squad->RequestSwitchTo(ETDShipId::Lancer);
	W.Advance(.5f, 1.f / 60.f);
	TestTrue(TEXT("flying Lancer"), PC->GetPawn() == Squad->GetShip(ETDShipId::Lancer));
	// The cloak keeps running on the AI-flown Rift (review decision, CloakSurvivesASwitchWithEnemiesInRange): so does its look.
	TestTrue(TEXT("switch: Rift still cloaked"), Cloak->IsCloaked());
	TestTrue(TEXT("switch: the wingman's hull still shows the cloak"), Cloak->GetOverlaidMeshCount() > 0 && FMath::IsNearlyEqual(Cloak01(Cloak), 1.f));
	W.Advance(6.f, 1.f / 60.f);
	TestFalse(TEXT("cloak expired under AI"), Cloak->IsCloaked());
	TestEqual(TEXT("after the switch: overlay gone once the cloak ends"), Cloak->GetOverlaidMeshCount(), 0);
	TInlineComponentArray<UMeshComponent*> Meshes(Rift);
	bool bAnyLeft = false;
	for (UMeshComponent* M : Meshes)
	{
		bAnyLeft |= M && M->GetOverlayMaterial() && M->GetOverlayMaterial()->IsA<UMaterialInstanceDynamic>();
	}
	TestFalse(TEXT("after the switch: no cloak overlay left on any Rift mesh"), bAnyLeft);
	return true;
}

#endif
