// Triad Drift - Powers (Teammate 2). B2: data-asset presentation by convention + Shipping-safe placeholders.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Powers.Presentation

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EngineUtils.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Powers/TDAbilityPowers.h"
#include "TriadDrift/Powers/TDBastionPowers.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"

namespace TDPowerTest
{

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPresPlaceholders, "TriadDrift.Powers.Presentation.PlaceholdersWithoutArt", TD_TEST_FLAGS)
bool FTDPresPlaceholders::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	// Portal: disc + rim.
	AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDPortalBlasterPower* Portal = AddPower<UTDPortalBlasterPower>(Rift);
	Portal->HandleInputPressed();
	ATDPortal* P = Portal->GetPortals().Num() ? Portal->GetPortals()[0].Get() : nullptr;
	TestTrue(TEXT("portal has its placeholder disc and rim"), P && FTDPowerPresentation::CountPlaceholders(P) >= 2);

	// Dome: lattice shell.
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector(0.f, -30000.f, 0.f), FVector(100.f));
	UTDShieldDomePower* Dome = AddPower<UTDShieldDomePower>(Bastion);
	Dome->TryActivate();
	W.Tick();
	TestTrue(TEXT("dome has its lattice"), Dome->GetDome() && FTDPowerPresentation::CountPlaceholders(Dome->GetDome()) == 1);

	// Gravity well: core, then the ring when the vortex opens.
	UTDGravityWellPower* Well = AddPower<UTDGravityWellPower>(Bastion);
	Well->TryActivate();
	W.Tick();
	TestTrue(TEXT("well orb has a core"), Well->GetLastWell() && FTDPowerPresentation::CountPlaceholders(Well->GetLastWell()) == 1);
	W.Advance(1.f);
	TestTrue(TEXT("vortex adds its ring"), Well->GetLastWell() && FTDPowerPresentation::CountPlaceholders(Well->GetLastWell()) == 2);

	// EMP: a short ring flash actor.
	AActor* Lancer = SpawnBody(W, ETDTeam::Player, FVector(0.f, 30000.f, 0.f), FVector(100.f));
	UTDEMPPower* Emp = AddPower<UTDEMPPower>(Lancer);
	Emp->TryActivate();
	int32 Flashes = 0;
	for (TActorIterator<ATDPlaceholderFlash> It(W.GetWorld()); It; ++It)
	{
		++Flashes;
	}
	TestEqual(TEXT("EMP shows a pulse flash"), Flashes, 1);
	W.Advance(0.6f);
	Flashes = 0;
	for (TActorIterator<ATDPlaceholderFlash> It(W.GetWorld()); It; ++It)
	{
		++Flashes;
	}
	TestEqual(TEXT("the flash cleans itself up"), Flashes, 0);

	// Ram: cone during the dash only.
	ATDSpacecraft* Ship = W.Spawn<ATDSpacecraft>(FVector(60000.f, 0.f, 0.f), FRotator::ZeroRotator);
	Ship->SetTeam(ETDTeam::Player);
	UTDRamPower* Ram = AddPower<UTDRamPower>(Ship);
	Ram->TryActivate();
	W.Advance(0.5f);
	TestEqual(TEXT("ram cone while dashing"), FTDPowerPresentation::CountPlaceholders(Ship), 1);
	AdvanceUntil(W, [&] { return !Ram->IsPowerActive(); }, 2.f);
	TestEqual(TEXT("cone removed after the dash"), FTDPowerPresentation::CountPlaceholders(Ship), 0);

	// Cloak: shimmer shell while cloaked.
	UTDCloakPower* Cloak = AddPower<UTDCloakPower>(Rift);
	const int32 Before = FTDPowerPresentation::CountPlaceholders(Rift);
	Cloak->TryActivate();
	TestEqual(TEXT("cloak shimmer"), FTDPowerPresentation::CountPlaceholders(Rift), Before + 1);
	Cloak->EndPower(true);
	TestEqual(TEXT("shimmer gone"), FTDPowerPresentation::CountPlaceholders(Rift), Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPresArtHidesPlaceholders, "TriadDrift.Powers.Presentation.ArtReplacesPlaceholders", TD_TEST_FLAGS)
bool FTDPresArtHidesPlaceholders::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	const TSoftObjectPtr<UNiagaraSystem> SomeArt(FSoftObjectPath(TEXT("/Game/TriadDrift/VFX/NS_NotYetAuthored.NS_NotYetAuthored")));

	AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDPortalBlasterPower* Portal = AddPower<UTDPortalBlasterPower>(Rift);
	UTDTravelFX* Travel = NewObject<UTDTravelFX>();
	Travel->PortalIdle.VFX = SomeArt;
	Portal->TravelFX = Travel;
	Portal->HandleInputPressed();
	ATDPortal* P = Portal->GetPortals().Num() ? Portal->GetPortals()[0].Get() : nullptr;
	TestTrue(TEXT("portal art assigned: no placeholder"), P && FTDPowerPresentation::CountPlaceholders(P) == 0);

	UTDCloakPower* Cloak = AddPower<UTDCloakPower>(Rift);
	UTDAbilityFX* CloakFX = NewObject<UTDAbilityFX>();
	CloakFX->Loop.VFX = SomeArt;
	Cloak->FX = CloakFX;
	Cloak->TryActivate();
	TestEqual(TEXT("cloak art assigned: no shimmer"), FTDPowerPresentation::CountPlaceholders(Rift), 0);

	// Weapons: a designer-set FX is never replaced by the path convention.
	UTDWeaponComponent* Weapons = NewObject<UTDWeaponComponent>(Rift, TEXT("Weapons"));
	Weapons->RegisterComponent();
	UTDWeaponFX* Mine = NewObject<UTDWeaponFX>();
	Weapons->PrimaryFX = Mine;
	Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
	TestTrue(TEXT("designer FX kept"), Weapons->PrimaryFX == Mine);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPresConvention, "TriadDrift.Powers.Presentation.PathConventionIsOptional", TD_TEST_FLAGS)
bool FTDPresConvention::RunTest(const FString& Parameters)
{
	// None of Nick's assets exist yet: every lookup is a quiet null, twice (the miss is cached).
	for (int32 i = 0; i < 2; ++i)
	{
		for (const ETDShipId Ship : { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion })
		{
			FTDPowerPresentation::LoadShipVisuals(Ship);
		}
		FTDPowerPresentation::LoadAbilityFX(ETDAbilityId::Cloak);
		FTDPowerPresentation::LoadTravelFX();
		FTDPowerPresentation::LoadWeaponFX(TDWeaponIds::SpikeRockets);
	}
	TestNull(TEXT("missing ability FX is null"), FTDPowerPresentation::LoadAbilityFX(ETDAbilityId::None));
	TestNull(TEXT("no weapon id, no FX"), FTDPowerPresentation::LoadWeaponFX(NAME_None));

#if WITH_EDITOR
	for (UClass* C : { UTDPowerComponent::StaticClass(), UTDRamPower::StaticClass(), UTDPortalBlasterPower::StaticClass(),
		UTDMissileSwarmPower::StaticClass(), UTDShieldDomePower::StaticClass(), UTDGravityWellPower::StaticClass(),
		UTDCloakPower::StaticClass(), UTDEMPPower::StaticClass() })
	{
		TestTrue(*FString::Printf(TEXT("%s is Blueprintable"), *C->GetName()), C->GetBoolMetaData(TEXT("IsBlueprintBase")));
	}
#endif
	return true;
}

} // namespace TDPowerTest

#endif // WITH_DEV_AUTOMATION_TESTS
