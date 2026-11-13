// Retake audit (Lead): a squad wipe in waves 1, 3 and 5 and in the Dominion assault, through the real
// game flow (T5's game instance and game mode, the "CUT!" delay, the next take). Before the wipe the take
// is made dirty the way play makes it dirty: a power used (Rift's portals open), guns fired and hot,
// projectiles in flight, a focus order, Switch Strike armed, wingmen hurt. After the retake every piece of
// state is checked and anything that survived the take is reported by name.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Squad.Retake

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EngineUtils.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Director/TDDirectorCamera.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flow/TDCheatManager.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDSpaceGameMode.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/TDWingmanAIController.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Waves/TDWaveDirector.h"

namespace TDRetakeAudit
{
	static void RunFlow(FTDTestWorld& W, float Seconds)
	{
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		for (int32 i = 0; i < FMath::CeilToInt(Seconds * 60.f); ++i)
		{
			W.Tick(1.f / 60.f);
			if (GI)
			{
				GI->TickFlow(1.f / 60.f);
			}
		}
	}

	static ATDWaveDirector* FindWaves(UWorld* World)
	{
		for (TActorIterator<ATDWaveDirector> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	/** One stage: dirty the take, wipe, retake, audit. Returns the list of leftovers. */
	static TArray<FString> Audit(FAutomationTestBase& T, const FString& Stage)
	{
		TArray<FString> Left;
		FTDTestWorld W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
		RunFlow(W, 0.5f);
		UWorld* World = W.GetWorld();
		if (!T.TestTrue(Stage + TEXT(": stage jump"), UTDCheatManager::DoStage(World, Stage)))
		{
			return Left;
		}
		RunFlow(W, 6.f);
		ATDSquadManager* Squad = ATDSquadManager::Get(World);
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		if (!T.TestNotNull(Stage + TEXT(": squad"), Squad) || !GI)
		{
			return Left;
		}
		ATDWaveDirector* Waves = FindWaves(World);
		const bool bWave = Waves && (Waves->GetState() == ETDWaveState::Active || Waves->GetState() == ETDWaveState::WarpingIn);
		const int32 WaveBefore = Waves ? Waves->GetCurrentWaveIndex() : -1;

		// ---- dirty the take
		UTDEventBus* Bus = UTDEventBus::Get(World);
		if (Squad->GetActiveShipId() != ETDShipId::Rift)
		{
			RunFlow(W, 0.5f);
			Squad->RequestSwitchTo(ETDShipId::Rift);
		}
		ATDShip* Rift = Squad->GetShip(ETDShipId::Rift);
		UTDPowerComponent::RoutePowerInput(Rift, ETDPowerInput::Signature1, true);  // portal A
		UTDPowerComponent::RoutePowerInput(Rift, ETDPowerInput::Signature1, false);
		RunFlow(W, 0.2f);
		Rift->SetActorRotation(Rift->GetActorRotation() + FRotator(0.f, 90.f, 0.f));
		UTDPowerComponent::RoutePowerInput(Rift, ETDPowerInput::Signature1, true);  // portal B
		UTDPowerComponent::RoutePowerInput(Rift, ETDPowerInput::Signature1, false);
		if (UTDWeaponComponent* Guns = Rift->GetWeapons())
		{
			Guns->SetTriggerHeld(ETDWeaponSlot::Primary, true);
		}
		RunFlow(W, 1.f);
		if (Bus)
		{
			Bus->BroadcastPowerUsed(ETDShipId::Rift, TEXT("PortalBlaster"));
		}
		RunFlow(W, 0.45f);
		Squad->RequestSwitchTo(ETDShipId::Lancer); // Switch Strike armed
		TArray<TWeakObjectPtr<AActor>> EnemiesBefore;
		for (TActorIterator<ATDEnemyShip> It(World); It; ++It)
		{
			if (UTDStatics::GetHealth(*It) && UTDStatics::GetHealth(*It)->IsAlive())
			{
				EnemiesBefore.Add(*It);
			}
		}
		if (EnemiesBefore.Num() > 0)
		{
			Squad->IssueFocusCommand(EnemiesBefore[0].Get());
		}
		UTDStatics::ApplyTDDamage(Squad->GetShip(ETDShipId::Bastion), 50.f, ETDDamageKind::Energy, nullptr, nullptr);
		RunFlow(W, 0.2f);
		int32 PortalsBefore = 0;
		for (TActorIterator<ATDPortal> It(World); It; ++It) { ++PortalsBefore; }
		const UTDProjectileSubsystem* Proj = UTDProjectileSubsystem::Get(Squad);
		T.AddInfo(FString::Printf(TEXT("%s before the wipe: %d portals, %d projectiles, %d enemies, strike %d, focus %d, wave %d"),
			*Stage, PortalsBefore, Proj ? Proj->GetLiveProjectiles().Num() : -1, EnemiesBefore.Num(), Squad->IsSwitchStrikeActive(), Squad->GetFocusTarget() != nullptr, WaveBefore + 1));

		// ---- wipe and retake
		for (const ETDShipId Id : UTDShipLibrary::AllShips())
		{
			if (UTDHealthComponent* H = UTDStatics::GetHealth(Squad->GetShip(Id)))
			{
				H->Kill();
			}
		}
		W.Tick(1.f / 60.f);
		if (!T.TestTrue(Stage + TEXT(": the flow took the wipe"), GI->IsRetakePending()))
		{
			return Left;
		}
		for (int32 i = 0; i < 60 * 8 && GI->IsRetakePending(); ++i)
		{
			RunFlow(W, 1.f / 60.f);
		}
		RunFlow(W, 2.f / 60.f);
		T.TestFalse(Stage + TEXT(": the retake ran"), GI->IsRetakePending());

		// ---- audit
		auto Note = [&](bool bBad, const FString& What) { if (bBad) { Left.Add(What); } };
		Note(Squad->GetLivingShips().Num() != 3, TEXT("not all three ships back"));
		Note(Squad->GetActiveShipId() != Squad->StartingShip, TEXT("not in the starting ship"));
		const ATDShip* Leader = Squad->GetActiveShip();
		for (const ETDShipId Id : UTDShipLibrary::AllShips())
		{
			ATDShip* Ship = Squad->GetShip(Id);
			const FString N = UEnum::GetDisplayValueAsText(Id).ToString();
			const UTDHealthComponent* H = UTDStatics::GetHealth(Ship);
			Note(!H || H->GetHull01() < 0.999f || H->GetShield01() < 0.999f, N + TEXT(": hull/shield not full"));
			Note(!Ship || Ship->IsHidden() || Ship->IsFlightParked(), N + TEXT(": hidden or parked"));
			if (UTDWeaponComponent* Guns = Ship ? Ship->GetWeapons() : nullptr)
			{
				Note(Guns->GetHeat01(ETDWeaponSlot::Primary) > 0.01f, N + TEXT(": primary still hot"));
			}
			if (Ship)
			{
				TArray<UTDPowerComponent*> Powers;
				Ship->GetComponents<UTDPowerComponent>(Powers);
				for (const UTDPowerComponent* P : Powers)
				{
					Note(P->IsPowerActive() || P->GetCooldown01() < 0.999f, N + TEXT(": a power still active or cooling (") + P->GetName() + TEXT(")"));
				}
				Note(Leader && FVector::Dist(Ship->GetActorLocation(), Leader->GetActorLocation()) > 8000.f, N + TEXT(": not with the squad"));
			}
			if (Id != Squad->GetActiveShipId())
			{
				const ATDWingmanAIController* AI = Ship ? Cast<ATDWingmanAIController>(Ship->GetController()) : nullptr;
				Note(!AI || AI->GetWingmanState() == ETDWingmanState::Downed, N + TEXT(": wingman AI not flying it"));
			}
		}
		int32 Portals = 0;
		for (TActorIterator<ATDPortal> It(World); It; ++It) { Portals += IsValid(*It) ? 1 : 0; }
		Note(Portals > 0, FString::Printf(TEXT("%d portal(s) from the last take"), Portals));
		int32 OldEnemies = 0;
		for (const TWeakObjectPtr<AActor>& E : EnemiesBefore)
		{
			OldEnemies += (E.IsValid() && UTDStatics::GetHealth(E.Get()) && UTDStatics::GetHealth(E.Get())->IsAlive()) ? 1 : 0;
		}
		Note(bWave && OldEnemies > 0, FString::Printf(TEXT("%d enemy ship(s) from the last take still alive"), OldEnemies));
		if (Proj)
		{
			int32 Squadshots = 0;
			for (const ATDProjectile* P : Proj->GetLiveProjectiles())
			{
				Squadshots += (IsValid(P) && P->IsActive()) ? 1 : 0;
			}
			Note(Squadshots > 0, FString::Printf(TEXT("%d projectile(s) still flying 2 frames after the retake"), Squadshots));
		}
		Note(Squad->IsSwitchStrikeActive(), TEXT("Switch Strike carried over"));
		Note(Squad->GetFocusTarget() != nullptr, TEXT("focus order carried over"));
		Note(Squad->GetDamageIndicators().Num() > 0, TEXT("damage indicators carried over"));
		if (const UTDDirectorSubsystem* D = UTDDirectorSubsystem::Get(World))
		{
			Note(D->GetCameraRig() && D->GetCameraRig()->IsActive(), TEXT("a director shot carried over"));
		}
		for (const FTDShipHUD& Card : Squad->GetSquadHUD())
		{
			Note(!Card.bAlive || Card.RespawnSeconds > 0.f, UEnum::GetDisplayValueAsText(Card.Ship).ToString() + TEXT(": HUD card not back"));
		}
		if (bWave && Waves)
		{
			Note(!(Waves->GetState() == ETDWaveState::Active || Waves->GetState() == ETDWaveState::WarpingIn), TEXT("wave director not running the scene again"));
			Note(Waves->GetCurrentWaveIndex() != WaveBefore, FString::Printf(TEXT("wave changed: %d -> %d"), WaveBefore + 1, Waves->GetCurrentWaveIndex() + 1));
		}
		T.AddInfo(Stage + TEXT(": ") + (Left.Num() == 0 ? FString(TEXT("clean")) : FString::Join(Left, TEXT("; "))));
		return Left;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSquadRetakeAudit, "TriadDrift.Squad.Retake.NothingSurvivesTheTake", TD_TEST_FLAGS)
bool FTDSquadRetakeAudit::RunTest(const FString& Parameters)
{
	for (const TCHAR* Stage : { TEXT("Waves1to2"), TEXT("Waves3to4"), TEXT("Wave5"), TEXT("DominionAssault") })
	{
		const TArray<FString> Left = TDRetakeAudit::Audit(*this, Stage);
		for (const FString& L : Left)
		{
			AddError(FString(Stage) + TEXT(": survives the retake: ") + L);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
