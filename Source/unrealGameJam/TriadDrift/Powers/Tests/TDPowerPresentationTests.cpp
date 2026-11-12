// Triad Drift - Powers (Teammate 2). B2: data-asset presentation by convention + Shipping-safe placeholders.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Powers.Presentation

#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

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
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
	FTDTestWorld W;
	// Portal: disc + rim.
	AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDPortalBlasterPower* Portal = AddPower<UTDPortalBlasterPower>(Rift);
	Portal->HandleInputPressed();
	ATDPortal* P = Portal->GetPortals().Num() ? Portal->GetPortals()[0].Get() : nullptr;
	TestTrue(TEXT("portal has its placeholder rings (open centre, no solid disc)"), P && FTDPowerPresentation::CountPlaceholders(P) >= 2);

	// Dome: lattice shell.
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector(0.f, -30000.f, 0.f), FVector(100.f));
	UTDShieldDomePower* Dome = AddPower<UTDShieldDomePower>(Bastion);
	Dome->TryActivate();
	W.Tick(PowerFrameStep());
	TestTrue(TEXT("dome has its lattice"), Dome->GetDome() && FTDPowerPresentation::CountPlaceholders(Dome->GetDome()) == 1);

	// Gravity well: core, then the ring when the vortex opens.
	UTDGravityWellPower* Well = AddPower<UTDGravityWellPower>(Bastion);
	Well->TryActivate();
	W.Tick(PowerFrameStep());
	TestTrue(TEXT("well orb has a core"), Well->GetLastWell() && FTDPowerPresentation::CountPlaceholders(Well->GetLastWell()) == 1);
	W.Advance(1.f, PowerFrameStep());
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
	W.Advance(0.6f, PowerFrameStep());
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
	W.Advance(0.5f, PowerFrameStep());
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
	continue;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPresArtHidesPlaceholders, "TriadDrift.Powers.Presentation.ArtReplacesPlaceholders", TD_TEST_FLAGS)
bool FTDPresArtHidesPlaceholders::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
	FTDTestWorld W;
	const TSoftObjectPtr<UNiagaraSystem> SomeArt(FSoftObjectPath(TEXT("/Game/TriadDrift/VFX/NS_NotYetAuthored.NS_NotYetAuthored")));

	AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDPortalBlasterPower* Portal = AddPower<UTDPortalBlasterPower>(Rift);
	UTDTravelFX* Travel = NewObject<UTDTravelFX>();
	const TStrongObjectPtr<UTDTravelFX> TravelRooted(Travel); // a test-made asset: nothing else holds it until it's assigned
	Travel->PortalIdle.VFX = SomeArt;
	Portal->TravelFX = Travel;
	Portal->HandleInputPressed();
	ATDPortal* P = Portal->GetPortals().Num() ? Portal->GetPortals()[0].Get() : nullptr;
	TestTrue(TEXT("portal art assigned: no placeholder"), P && FTDPowerPresentation::CountPlaceholders(P) == 0);

	UTDCloakPower* Cloak = AddPower<UTDCloakPower>(Rift);
	UTDAbilityFX* CloakFX = NewObject<UTDAbilityFX>();
	const TStrongObjectPtr<UTDAbilityFX> CloakFXRooted(CloakFX); // a test-made asset: nothing else holds it until it's assigned
	CloakFX->Loop.VFX = SomeArt;
	Cloak->FX = CloakFX;
	Cloak->TryActivate();
	TestEqual(TEXT("cloak art assigned: no shimmer"), FTDPowerPresentation::CountPlaceholders(Rift), 0);

	// Weapons: a designer-set FX is never replaced by the path convention.
	UTDWeaponComponent* Weapons = NewObject<UTDWeaponComponent>(Rift, TEXT("Weapons"));
	Weapons->RegisterComponent();
	UTDWeaponFX* Mine = NewObject<UTDWeaponFX>();
	const TStrongObjectPtr<UTDWeaponFX> MineRooted(Mine); // a test-made asset: nothing else holds it until it's assigned
	Weapons->PrimaryFX = Mine;
	Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
	TestTrue(TEXT("designer FX kept"), Weapons->PrimaryFX == Mine);
	continue;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPresConvention, "TriadDrift.Powers.Presentation.PathConventionIsOptional", TD_TEST_FLAGS)
bool FTDPresConvention::RunTest(const FString& Parameters)
{
	for (const float TDFrameStep : kPowerFrameSteps)
	{
	FScopedPowerFrameStep TDStepScope(TDFrameStep);
	AddInfo(FString::Printf(TEXT("frame step %.0f fps"), 1.f / TDFrameStep));
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
	continue;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPresCueRequests, "TriadDrift.Powers.Presentation.EveryMomentRequestsItsCue", TD_TEST_FLAGS)
bool FTDPresCueRequests::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	FTDPowerPresentation::ResetCueRequests();
	auto N = [](const TCHAR* Moment) { return FTDPowerPresentation::GetCueRequests(FName(Moment)); };
	UTDShipVisuals* Ship = NewObject<UTDShipVisuals>();
	const TStrongObjectPtr<UTDShipVisuals> ShipRooted(Ship); // a test-made asset: nothing else holds it until it's assigned
	UTDAbilityFX* Ability = NewObject<UTDAbilityFX>();
	const TStrongObjectPtr<UTDAbilityFX> AbilityRooted(Ability); // a test-made asset: nothing else holds it until it's assigned
	UTDTravelFX* Travel = NewObject<UTDTravelFX>();
	const TStrongObjectPtr<UTDTravelFX> TravelRooted(Travel); // a test-made asset: nothing else holds it until it's assigned

	// Ram: wind-up, then active at the dash, then end.
	ATDSpacecraft* Lancer = W.Spawn<ATDSpacecraft>(FVector::ZeroVector, FRotator::ZeroRotator);
	Lancer->SetTeam(ETDTeam::Player);
	UTDRamPower* Ram = AddPower<UTDRamPower>(Lancer);
	Ram->ShipVisuals = Ship;
	Ram->TryActivate();
	TestEqual(TEXT("ram: PowerWindup"), N(TEXT("Ship.PowerWindup")), 1);
	TestEqual(TEXT("ram: no PowerActive before the dash"), N(TEXT("Ship.PowerActive")), 0);
	AdvanceUntil(W, [&] { return !Ram->IsPowerActive(); }, 2.f);
	TestEqual(TEXT("ram: PowerActive at the dash"), N(TEXT("Ship.PowerActive")), 1);
	TestEqual(TEXT("ram: PowerEnd"), N(TEXT("Ship.PowerEnd")), 1);

	// Dome (Signature2): Power2Active; its shell loop and end.
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector(0.f, -30000.f, 0.f), FVector(100.f));
	UTDShieldDomePower* Dome = AddPower<UTDShieldDomePower>(Bastion);
	Dome->ShipVisuals = Ship;
	Dome->FX = Ability;
	Dome->TryActivate();
	// Repro probe: a full garbage collection mid-power (the suite crashed here once, GC dependent).
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	TestEqual(TEXT("dome: Power2Active"), N(TEXT("Ship.Power2Active")), 1);
	TestTrue(TEXT("dome: shell Loop"), N(TEXT("Ability.Loop")) >= 1);
	W.Advance(6.5f);
	TestTrue(TEXT("dome: End on collapse"), N(TEXT("Ability.End")) >= 1);

	// Portals: open, idle, transit, close.
	AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector(0.f, 30000.f, 0.f), FVector(50.f));
	UTDPortalBlasterPower* Portal = AddPower<UTDPortalBlasterPower>(Rift);
	Portal->TravelFX = Travel;
	Portal->HandleInputPressed();
	Rift->SetActorRotation(FRotator(0.f, 90.f, 0.f));
	Portal->HandleInputPressed();
	TestEqual(TEXT("portal: PortalOpen x2"), N(TEXT("Travel.PortalOpen")), 2);
	TestEqual(TEXT("portal: PortalIdle x2"), N(TEXT("Travel.PortalIdle")), 2);
	ATDPortal* A = Portal->GetPortals()[0].Get();
	AActor* Hook = SpawnBody(W, ETDTeam::Enemy, A->GetActorLocation() + FVector(9000.f, 0.f, 0.f), FVector(100.f));
	LaunchMissile(W, A->GetActorLocation() + FVector(1500.f, 0.f, 0.f), -FVector::ForwardVector, Hook, Rift);
	W.Advance(0.5f);
	TestTrue(TEXT("portal: PortalTransit"), N(TEXT("Travel.PortalTransit")) >= 1);
	Portal->EndPower(true);
	TestEqual(TEXT("portal: PortalClose x2"), N(TEXT("Travel.PortalClose")), 2);

	// EMP: activate + one AffectedTarget per enemy hit.
	AActor* Lancer2 = SpawnBody(W, ETDTeam::Player, FVector(50000.f, 0.f, 0.f), FVector(100.f));
	SpawnEnemyShip(W, TDEnemyIds::ShardFighter, FVector(52000.f, 0.f, 0.f));
	SpawnEnemyShip(W, TDEnemyIds::ShardFighter, FVector(50000.f, 3000.f, 0.f));
	UTDEMPPower* Emp = AddPower<UTDEMPPower>(Lancer2);
	Emp->FX = Ability;
	const int32 Activates = N(TEXT("Ability.Activate"));
	const int32 Affected = N(TEXT("Ability.AffectedTarget"));
	Emp->TryActivate();
	TestEqual(TEXT("EMP: Activate"), N(TEXT("Ability.Activate")), Activates + 1);
	TestEqual(TEXT("EMP: AffectedTarget per enemy"), N(TEXT("Ability.AffectedTarget")), Affected + 2);

	// Cloak: activate, loop, end.
	UTDCloakPower* Cloak = AddPower<UTDCloakPower>(Rift);
	Cloak->FX = Ability;
	const int32 Loops = N(TEXT("Ability.Loop"));
	const int32 Ends = N(TEXT("Ability.End"));
	Cloak->TryActivate();
	Cloak->EndPower(true);
	TestEqual(TEXT("cloak: Loop"), N(TEXT("Ability.Loop")), Loops + 1);
	TestEqual(TEXT("cloak: End"), N(TEXT("Ability.End")), Ends + 1);
	return true;
}

} // namespace TDPowerTest

#endif // WITH_DEV_AUTOMATION_TESTS
