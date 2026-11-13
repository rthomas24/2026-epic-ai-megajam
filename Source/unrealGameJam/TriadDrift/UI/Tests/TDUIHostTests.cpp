// Automation tests for the space HUD host (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.UI

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/PlayerController.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/UnrealType.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Core/TDDebugHUD.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Powers/TDAbilityComponent.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/UI/TDUIHostSettings.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"

namespace TDUIHostTests
{
	/**
	 * Forces Nick's HUD on (1) or the dev HUD (0) for the scope of a test and restores both switches. These tests are
	 * about Nick's widget, so they pin TD.UI.SpaceHUD too: with the engineering HUD as the default (ENG-36), "Nick's off"
	 * alone would show the engineering HUD instead of none.
	 */
	struct FScopedNativeHUD
	{
		IConsoleVariable* Var = nullptr;
		IConsoleVariable* Choice = nullptr;
		int32 Previous = -1;
		int32 PreviousChoice = -1;
		explicit FScopedNativeHUD(int32 Value)
		{
			Var = IConsoleManager::Get().FindConsoleVariable(TEXT("TD.UI.NativeHUD"));
			Choice = IConsoleManager::Get().FindConsoleVariable(TEXT("TD.UI.SpaceHUD"));
			Previous = Var ? Var->GetInt() : -1;
			PreviousChoice = Choice ? Choice->GetInt() : -1;
			Set(Value);
		}
		void Set(int32 Value) const
		{
			if (Var)
			{
				Var->Set(Value, ECVF_SetByCode);
			}
			if (Choice)
			{
				Choice->Set(Value > 0 ? 1 : 0, ECVF_SetByCode); // 1 = Nick's, 0 = dev
			}
		}
		~FScopedNativeHUD()
		{
			if (Var)
			{
				Var->Set(Previous, ECVF_SetByCode);
			}
			if (Choice)
			{
				Choice->Set(PreviousChoice, ECVF_SetByCode);
			}
		}
	};

	/** A local player flying a spawned squad, with the host evaluated by hand (no viewport). */
	struct FHostFixture
	{
		FTDTestWorld W;
		APlayerController* PC = nullptr;
		ATDSquadManager* Squad = nullptr;
		UTDUIHostSubsystem* Host = nullptr;

		FHostFixture()
		{
			PC = W.SpawnLocalPlayerController();
			Squad = W.Spawn<ATDSquadManager>();
			if (Squad)
			{
				Squad->SetSquadPlayerController(PC);
				Squad->SpawnSquad(FTransform::Identity);
			}
			Host = UTDUIHostSubsystem::Get(W.GetWorld());
			if (Host)
			{
				Host->bAddToViewport = false;
			}
			W.Tick();
		}

		/** Evaluate the host, sample the widget, decode its snapshot. */
		bool Sample(TArray<FTDHostShipView>& Out) const
		{
			if (!Host)
			{
				return false;
			}
			Host->Evaluate();
			UUserWidget* HUD = Host->GetSpaceHUD();
			return HUD && UTDUIHostSubsystem::CallRefreshData(HUD) && UTDUIHostSubsystem::ReadSnapshot(HUD, Out) && Out.Num() == 3;
		}
	};

	// ---- frozen-name helpers: each checks one reflected name exactly as the plugin uses it

	/** A zero-input UFUNCTION on Class whose return property is of type ReturnType. */
	template <typename PropertyType>
	const PropertyType* FindGetter(FAutomationTestBase& Test, const UClass* Class, const TCHAR* Name)
	{
		const UFunction* Function = Class ? Class->FindFunctionByName(Name) : nullptr;
		Test.TestNotNull(FString::Printf(TEXT("%s::%s exists"), Class ? *Class->GetName() : TEXT("?"), Name), Function);
		if (!Function)
		{
			return nullptr;
		}
		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			Test.TestFalse(FString::Printf(TEXT("%s takes no inputs (plugin refuses getters with inputs)"), Name),
				It->HasAnyPropertyFlags(CPF_Parm) && !It->HasAnyPropertyFlags(CPF_ReturnParm));
		}
		const PropertyType* Return = CastField<PropertyType>(Function->GetReturnProperty());
		Test.TestNotNull(FString::Printf(TEXT("%s returns a %s"), Name, *PropertyType::StaticClass()->GetName()), Return);
		return Return;
	}

	void ExpectMember(FAutomationTestBase& Test, const UStruct* Type, const TCHAR* Name, const FFieldClass* Kind)
	{
		const FProperty* P = Type ? FindFProperty<FProperty>(Type, Name) : nullptr;
		Test.TestTrue(FString::Printf(TEXT("%s.%s is a %s"), Type ? *Type->GetName() : TEXT("?"), Name, *Kind->GetName()),
			P && P->GetClass()->IsChildOf(Kind));
	}

	void ExpectNumber(FAutomationTestBase& Test, const UStruct* Type, const TCHAR* Name)
	{
		ExpectMember(Test, Type, Name, FNumericProperty::StaticClass());
	}
}

using namespace TDUIHostTests;

// ------------------------------------------------------------------------------------------ frozen names

/**
 * Every name Plugins/TriadDriftHUD looks up by reflection (TDSpaceHUDWidget.cpp), and every name the
 * host looks up on the plugin. A rename on either side fails here instead of silently blanking the HUD.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIHostFrozenNames, "TriadDrift.UI.HUDHost.FrozenNames", TD_TEST_FLAGS)
bool FTDUIHostFrozenNames::RunTest(const FString& Parameters)
{
	// --- what the plugin reads from us (class paths are FindObject strings in the plugin)
	UClass* Contract = FindObject<UClass>(nullptr, TEXT("/Script/unrealGameJam.TDHUDData"));
	TestTrue(TEXT("/Script/unrealGameJam.TDHUDData is the ITDHUDData interface"), Contract == UTDHUDData::StaticClass());
	TestTrue(TEXT("the squad manager implements it"), ATDSquadManager::StaticClass()->ImplementsInterface(Contract));

	// The plugin calls these on the data source object (the squad manager), so resolve them there.
	const UClass* Source = ATDSquadManager::StaticClass();
	if (const FArrayProperty* Squad = FindGetter<FArrayProperty>(*this, Source, TEXT("GetSquadHUD")))
	{
		const FStructProperty* Item = CastField<FStructProperty>(Squad->Inner);
		TestNotNull(TEXT("GetSquadHUD returns an array of structs"), Item);
		const UStruct* Card = Item ? Item->Struct : nullptr;
		ExpectMember(*this, Card, TEXT("Ship"), FEnumProperty::StaticClass());
		for (const TCHAR* Name : { TEXT("Health01"), TEXT("Shield01"), TEXT("Boost01"), TEXT("Heat01"), TEXT("PowerCooldown01"),
			TEXT("Power2Cooldown01"), TEXT("AbilityCooldown01"), TEXT("SecondaryAmmo"), TEXT("RespawnSeconds") })
		{
			ExpectNumber(*this, Card, Name);
		}
		ExpectMember(*this, Card, TEXT("bPlayerControlled"), FBoolProperty::StaticClass());
		ExpectMember(*this, Card, TEXT("bAlive"), FBoolProperty::StaticClass());
		ExpectMember(*this, Card, TEXT("Ability"), FEnumProperty::StaticClass());
		const FEnumProperty* Ship = Card ? FindFProperty<FEnumProperty>(Card, TEXT("Ship")) : nullptr;
		TestTrue(TEXT("Ship indices are Rift=0, Lancer=1, Bastion=2"), Ship && Ship->GetEnum() == StaticEnum<ETDShipId>()
			&& int32(ETDShipId::Rift) == 0 && int32(ETDShipId::Lancer) == 1 && int32(ETDShipId::Bastion) == 2);
		TestTrue(TEXT("Ability None is 0 (the plugin's 'locked')"), int32(ETDAbilityId::None) == 0);
	}
	FindGetter<FNumericProperty>(*this, Source, TEXT("GetSpeed01"));
	FindGetter<FNumericProperty>(*this, Source, TEXT("GetHyperdriveCharge01"));
	FindGetter<FBoolProperty>(*this, Source, TEXT("IsOutOfBounds"));
	FindGetter<FTextProperty>(*this, Source, TEXT("GetObjectiveText"));
	{
		// Hit-direction indicators ("who is shooting me"): an array of structs Nick's widget may read by reflection.
		const FArrayProperty* Damage = FindGetter<FArrayProperty>(*this, Source, TEXT("GetDamageIndicators"));
		TestTrue(TEXT("GetDamageIndicators returns an array of structs"), Damage && CastField<FStructProperty>(Damage->Inner));
	}
	if (const FStructProperty* Wave = FindGetter<FStructProperty>(*this, Source, TEXT("GetWaveInfo")))
	{
		ExpectNumber(*this, Wave->Struct, TEXT("WaveNumber"));
		ExpectNumber(*this, Wave->Struct, TEXT("EnemiesAlive"));
	}
	FindGetter<FObjectPropertyBase>(*this, Source, TEXT("GetCurrentTarget"));
	for (const TCHAR* Name : { TEXT("GetLockedTargets"), TEXT("GetObjectiveMarkers") })
	{
		const FArrayProperty* Array = FindGetter<FArrayProperty>(*this, Source, Name);
		TestTrue(FString::Printf(TEXT("%s is an array of objects"), Name), Array && CastField<FObjectPropertyBase>(Array->Inner));
	}

	UClass* Health = FindObject<UClass>(nullptr, TEXT("/Script/unrealGameJam.TDHealthComponent"));
	TestTrue(TEXT("/Script/unrealGameJam.TDHealthComponent"), Health == UTDHealthComponent::StaticClass());
	FindGetter<FNumericProperty>(*this, Health, TEXT("GetHull01"));
	FindGetter<FNumericProperty>(*this, Health, TEXT("GetShield01"));

	UClass* Targeting = FindObject<UClass>(nullptr, TEXT("/Script/unrealGameJam.TDTargetingComponent"));
	TestTrue(TEXT("/Script/unrealGameJam.TDTargetingComponent"), Targeting == UTDTargetingComponent::StaticClass());
	FindGetter<FNumericProperty>(*this, Targeting, TEXT("GetIncomingMissileCount"));
	if (const FArrayProperty* Markers = FindGetter<FArrayProperty>(*this, Targeting, TEXT("K2_GetMarkers")))
	{
		const FStructProperty* Item = CastField<FStructProperty>(Markers->Inner);
		ExpectMember(*this, Item ? Item->Struct : nullptr, TEXT("Actor"), FWeakObjectProperty::StaticClass());
	}
	{
		const UFunction* Lead = Targeting ? Targeting->FindFunctionByName(TEXT("GetLeadIndicator")) : nullptr;
		const FStructProperty* Out = Lead ? FindFProperty<FStructProperty>(Lead, TEXT("OutWorldLocation")) : nullptr;
		TestTrue(TEXT("GetLeadIndicator(FVector& OutWorldLocation) -> bool"),
			Out && Out->Struct == TBaseStructure<FVector>::Get() && CastField<FBoolProperty>(Lead->GetReturnProperty()));
	}

	UClass* Bus = FindObject<UClass>(nullptr, TEXT("/Script/unrealGameJam.TDEventBus"));
	TestTrue(TEXT("/Script/unrealGameJam.TDEventBus is a game-instance subsystem"),
		Bus == UTDEventBus::StaticClass() && Bus->IsChildOf(UGameInstanceSubsystem::StaticClass()));

	// --- what we read from the plugin (and the plugin's own handlers, for delegate compatibility)
	UClass* Widget = UTDUIHostSubsystem::FindNativeWidgetClass();
	TestNotNull(TEXT("/Script/TriadDriftHUD.TDSpaceHUDWidget is loaded (plugin enabled)"), Widget);
	if (!Widget)
	{
		return true;
	}
	const TPair<const TCHAR*, const TCHAR*> Delegates[] = { { TEXT("OnHitConfirmed"), TEXT("HandleHit") }, { TEXT("OnObjectiveUpdated"), TEXT("HandleObjective") } };
	for (const TPair<const TCHAR*, const TCHAR*>& D : Delegates)
	{
		const FMulticastDelegateProperty* P = Bus ? FindFProperty<FMulticastDelegateProperty>(Bus, D.Key) : nullptr;
		const UFunction* Handler = Widget->FindFunctionByName(D.Value);
		TestTrue(FString::Printf(TEXT("UTDEventBus::%s fits the widget's %s"), D.Key, D.Value),
			P && Handler && P->SignatureFunction && P->SignatureFunction->IsSignatureCompatibleWith(Handler));
	}
	const UFunction* SetSource = Widget->FindFunctionByName(TEXT("SetDataSource"));
	TestTrue(TEXT("SetDataSource(UObject*)"), SetSource && SetSource->NumParms == 1 && CastField<FObjectPropertyBase>(SetSource->PropertyLink));
	const UFunction* Refresh = Widget->FindFunctionByName(TEXT("RefreshData"));
	TestTrue(TEXT("RefreshData()"), Refresh && Refresh->NumParms == 0);
	const FArrayProperty* Ships = FindFProperty<FArrayProperty>(Widget, TEXT("Ships"));
	const FStructProperty* UIShip = Ships ? CastField<FStructProperty>(Ships->Inner) : nullptr;
	TestNotNull(TEXT("widget snapshot 'Ships' is an array of FTDUIShip"), UIShip);
	if (UIShip)
	{
		for (const TCHAR* Name : { TEXT("Hull"), TEXT("Shield"), TEXT("Power"), TEXT("Power2"), TEXT("Ability"), TEXT("RespawnSeconds") })
		{
			ExpectNumber(*this, UIShip->Struct, Name);
		}
		for (const TCHAR* Name : { TEXT("bActive"), TEXT("bAlive"), TEXT("bUnlocked"), TEXT("bAvailable") })
		{
			ExpectMember(*this, UIShip->Struct, Name, FBoolProperty::StaticClass());
		}
	}
	UClass* Configured = UTDUIHostSettings::Get()->SpaceHUDClass.TryLoadClass<UUserWidget>();
	TestTrue(TEXT("the configured WBP_SpaceHUD loads and is a UTDSpaceHUDWidget"), Configured && Configured->IsChildOf(Widget));
	TestTrue(TEXT("the host resolves the configured class"), UTDUIHostSubsystem::ResolveWidgetClass() == Configured);
	return true;
}

// ------------------------------------------------------------------------------------------ show / hide rules

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIHostShowRules, "TriadDrift.UI.HUDHost.ShowHideRules", TD_TEST_FLAGS)
bool FTDUIHostShowRules::RunTest(const FString& Parameters)
{
	FScopedNativeHUD Switch(0);
	FHostFixture F;
	if (!TestNotNull(TEXT("host subsystem"), F.Host) || !TestNotNull(TEXT("squad"), F.Squad))
	{
		return false;
	}
	TestTrue(TEXT("player flies a ship"), Cast<ATDShip>(F.PC->GetPawn()) != nullptr);

	F.Host->Evaluate();
	TestFalse(TEXT("switch off: no widget"), F.Host->IsSpaceHUDShown());

	Switch.Set(1);
	F.Host->Evaluate();
	UUserWidget* First = F.Host->GetSpaceHUD();
	TestNotNull(TEXT("switch on: widget created"), First);
	TestTrue(TEXT("data source is the squad manager"), F.Host->GetDataSource() == F.Squad);
	TestTrue(TEXT("owned by the local player"), First && First->GetOwningPlayer() == F.PC);
	TestTrue(TEXT("dev HUD steps aside while it is up"), ATDDebugHUD::IsRealHUDActive(F.W.GetWorld()));

	// Kept across ship switches: same widget, same source.
	for (const ETDShipId Id : { ETDShipId::Lancer, ETDShipId::Bastion, ETDShipId::Rift })
	{
		F.W.Advance(1.f);
		TestTrue(TEXT("switch accepted"), F.Squad->RequestSwitchTo(Id));
		F.W.Tick();
		F.Host->Evaluate();
		TestTrue(FString::Printf(TEXT("same widget after switching to %d"), int32(Id)), F.Host->GetSpaceHUD() == First);
	}
	TArray<FTDHostShipView> Cards;
	TestTrue(TEXT("snapshot decoded"), F.Sample(Cards));
	TestTrue(TEXT("Rift is the active card"), Cards.Num() == 3 && Cards[0].bActive && !Cards[1].bActive && !Cards[2].bActive);

	// On foot: the pawn isn't a ship.
	APawn* OnFoot = F.W.Spawn<APawn>(FVector(0.f, 0.f, 5000.f));
	F.PC->Possess(OnFoot);
	F.Host->Evaluate();
	TestFalse(TEXT("on foot: removed"), F.Host->IsSpaceHUDShown());
	TestFalse(TEXT("on foot: dev HUD back"), ATDDebugHUD::IsRealHUDActive(F.W.GetWorld()));

	// Back in a ship: re-created.
	F.PC->Possess(F.Squad->GetShip(ETDShipId::Rift));
	F.Host->Evaluate();
	TestTrue(TEXT("back in a ship: shown again"), F.Host->IsSpaceHUDShown());

	Switch.Set(0);
	F.Host->Evaluate();
	TestFalse(TEXT("switch turned off at runtime: removed"), F.Host->IsSpaceHUDShown());
	TestFalse(TEXT("switch off: dev HUD back"), ATDDebugHUD::IsRealHUDActive(F.W.GetWorld()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIHostNoSquad, "TriadDrift.UI.HUDHost.NoWidgetWithoutSquad", TD_TEST_FLAGS)
bool FTDUIHostNoSquad::RunTest(const FString& Parameters)
{
	// A menu / transition map: a local player, a ship even, but no squad manager.
	FScopedNativeHUD Switch(1);
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>();
	W.SpawnLocalPlayerController(Ship);
	UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("host subsystem"), Host))
	{
		return false;
	}
	Host->bAddToViewport = false;
	W.Advance(0.5f);
	Host->Evaluate();
	TestFalse(TEXT("no squad manager: no widget"), Host->IsSpaceHUDShown());
	return true;
}

// ------------------------------------------------------------------------------------------ N-004

/**
 * Nick's N-004 contract, read back from the widget's own decoded snapshot: fresh run all locked ->
 * unlock Rift's Cloak -> ready -> activate -> charging -> ready -> Rift dies and respawns -> still
 * unlocked. Runs at 30 / 60 / 120 fps.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIHostN004, "TriadDrift.UI.HUDHost.N004AbilitySequence", TD_TEST_FLAGS)
bool FTDUIHostN004::RunTest(const FString& Parameters)
{
	const float Steps[] = { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f };
	for (const float Step : Steps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FScopedNativeHUD Switch(1);
		FHostFixture F;
		TArray<FTDHostShipView> Cards;
		if (!TestTrue(TEXT("widget snapshot") + Fps, F.Sample(Cards)))
		{
			return false;
		}
		for (int32 I = 0; I < 3; ++I)
		{
			TestFalse(FString::Printf(TEXT("fresh run: ship %d ability locked"), I) + Fps, Cards[I].bUnlocked);
		}

		UTDEventBus* Bus = UTDEventBus::Get(F.W.GetWorld());
		if (!TestNotNull(TEXT("event bus") + Fps, Bus))
		{
			return false;
		}
		Bus->BroadcastAbilityUnlocked(ETDAbilityId::Cloak, ETDShipId::Rift);
		F.W.Tick(Step);
		F.Sample(Cards);
		TestTrue(TEXT("Rift unlocked") + Fps, Cards[0].bUnlocked);
		TestTrue(TEXT("Rift ability ready") + Fps, Cards[0].Ability >= 0.999f);
		TestFalse(TEXT("Lancer still locked") + Fps, Cards[1].bUnlocked);
		TestFalse(TEXT("Bastion still locked") + Fps, Cards[2].bUnlocked);

		ATDShip* Rift = F.Squad->GetShip(ETDShipId::Rift);
		TestTrue(TEXT("ability activates") + Fps, UTDPowerComponent::RoutePowerInput(Rift, ETDPowerInput::Ability, true));
		UTDPowerComponent::RoutePowerInput(Rift, ETDPowerInput::Ability, false);
		// Cooldown readiness drops once the power is spent (Cloak ends on its own or on firing).
		bool bCharging = false;
		for (float T = 0.f; T < 30.f && !bCharging; T += Step)
		{
			F.W.Tick(Step);
			F.Sample(Cards);
			bCharging = Cards[0].bUnlocked && Cards[0].Ability < 0.999f;
		}
		TestTrue(TEXT("charging after use") + Fps, bCharging);
		bool bReady = false;
		for (float T = 0.f; T < 60.f && !bReady; T += Step)
		{
			F.W.Tick(Step);
			F.Sample(Cards);
			bReady = Cards[0].Ability >= 0.999f;
		}
		TestTrue(TEXT("ready again") + Fps, bReady && Cards[0].bUnlocked);

		// Respawn keeps it.
		if (UTDHealthComponent* H = UTDStatics::GetHealth(Rift))
		{
			H->Kill();
		}
		F.W.Tick(Step);
		bool bBack = false;
		for (float T = 0.f; T < 20.f && !bBack; T += Step)
		{
			F.W.Tick(Step);
			bBack = F.Squad->IsShipAlive(ETDShipId::Rift);
		}
		TestTrue(TEXT("Rift respawned") + Fps, bBack);
		F.W.Tick(Step);
		F.Sample(Cards);
		TestTrue(TEXT("after respawn: still unlocked") + Fps, Cards[0].bUnlocked && Cards[0].bAlive);
		TestTrue(TEXT("after respawn: ready") + Fps, Cards[0].Ability >= 0.999f);
	}
	return true;
}

/** Supplement mode: which surfaces the native HUD covers. None today; a widget claims one with a bCovers<Surface> bool. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDUIHostCoverage, "TriadDrift.UI.HUDHost.SurfaceCoverage", TD_TEST_FLAGS)
bool FTDUIHostCoverage::RunTest(const FString& Parameters)
{
	FScopedNativeHUD Switch(0);
	FHostFixture F;
	F.Host->Evaluate();
	TestFalse(TEXT("native HUD off: nothing covered"), UTDUIHostSubsystem::IsSurfaceCoveredByNativeHUD(F.W.GetWorld(), ETDHUDSurface::DamageIndicators));
	Switch.Set(1);
	F.Host->Evaluate();
	TestTrue(TEXT("native HUD up"), F.Host->IsSpaceHUDShown());
	const UEnum* Enum = StaticEnum<ETDHUDSurface>();
	for (int32 I = 0; I < Enum->NumEnums() - 1; ++I)
	{
		const ETDHUDSurface Surface = static_cast<ETDHUDSurface>(Enum->GetValueByIndex(I));
		const FName Prop = UTDUIHostSubsystem::GetCoverPropertyName(Surface);
		const bool bClaimed = FindFProperty<FBoolProperty>(F.Host->GetSpaceHUD()->GetClass(), Prop) != nullptr;
		AddInfo(FString::Printf(TEXT("%s: widget claims it = %d"), *Prop.ToString(), bClaimed ? 1 : 0));
		if (!bClaimed)
		{
			TestFalse(FString::Printf(TEXT("%s not covered until the widget claims it"), *Prop.ToString()),
				UTDUIHostSubsystem::IsSurfaceCoveredByNativeHUD(F.W.GetWorld(), Surface));
		}
	}
	TestEqual(TEXT("property naming"), UTDUIHostSubsystem::GetCoverPropertyName(ETDHUDSurface::DamageIndicators), FName(TEXT("bCoversDamageIndicators")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
