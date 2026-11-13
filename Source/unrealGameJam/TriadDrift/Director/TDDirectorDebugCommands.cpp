// Triad Drift - Director / Squad debug commands (Teammate 3). Dev builds only.
// They drive the REAL gameplay paths (squad switch, Lancer's real ram, a real Dominion generator,
// a real squad wipe through the game flow) so a live playtest can trigger every Director's Cut
// moment without input injection:
//   TD.Squad.Switch <Rift|Lancer|Bastion|Next|Prev>
//   TD.Squad.Wipe                 kill all three ships -> OnSquadWiped -> flow "CUT!" -> retake + slate
//   TD.Director.RamKill           switch to Lancer, park a real Shard Fighter ahead, ram it
//   TD.Director.RevealDominion    reveal the Dominion through the game flow (it warps in)
//   TD.Director.GeneratorKill     the player's ship destroys the first live shield generator
//   TD.Director.Status            log the camera rig / presentation state
//   TD.Director.MultiKill         the player's ship kills three live enemies in one beat (spawns fighters if needed)
//   TD.Director.MissileReturn     Rift opens both portals; an enemy missile goes through and back at its shooter
//   TD.Squad.KillActive           kill the ship the player flies (the tumble, then the auto-switch)
//   TD.Squad.Check <label>        log frame, active ship, possessed pawn, view target, slow-mo, pause (switch stress)
//   TD.Squad.Frame                log where the wingmen and the nearest enemy are on screen (-1..1),
//                                 for a scripted live pilot and for measuring "wingmen in frame"

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Director/TDCutPresentation.h"
#include "TriadDrift/Director/TDDirectorCamera.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TimerManager.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/TDWingmanAIController.h"
#include "TriadDrift/World/TDShieldGenerator.h"

namespace TDDirectorDebug
{
	static ATDSquadManager* Squad(UWorld* World)
	{
		ATDSquadManager* S = ATDSquadManager::Get(World);
		if (!S)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD debug: no squad manager in this world"));
		}
		return S;
	}

	static void Switch(const TArray<FString>& Args, UWorld* World)
	{
		ATDSquadManager* S = Squad(World);
		if (!S || Args.Num() == 0)
		{
			return;
		}
		const FString& A = Args[0];
		bool bOk = false;
		if (A.Equals(TEXT("Next"), ESearchCase::IgnoreCase)) { bOk = S->RequestSwitchNext(); }
		else if (A.Equals(TEXT("Prev"), ESearchCase::IgnoreCase)) { bOk = S->RequestSwitchPrev(); }
		else if (A.Equals(TEXT("Rift"), ESearchCase::IgnoreCase)) { bOk = S->RequestSwitchTo(ETDShipId::Rift); }
		else if (A.Equals(TEXT("Lancer"), ESearchCase::IgnoreCase)) { bOk = S->RequestSwitchTo(ETDShipId::Lancer); }
		else if (A.Equals(TEXT("Bastion"), ESearchCase::IgnoreCase)) { bOk = S->RequestSwitchTo(ETDShipId::Bastion); }
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Squad.Switch %s -> %s (active %s)"), *A, bOk ? TEXT("ok") : TEXT("refused"),
			*UEnum::GetValueAsString(S->GetActiveShipId()));
	}

	static void Wipe(const TArray<FString>&, UWorld* World)
	{
		ATDSquadManager* S = Squad(World);
		if (!S)
		{
			return;
		}
		for (const ETDShipId Id : UTDShipLibrary::AllShips())
		{
			if (UTDHealthComponent* H = UTDStatics::GetHealth(S->GetShip(Id)))
			{
				H->Kill();
			}
		}
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Squad.Wipe: all three ships killed"));
	}

	static void RamKill(const TArray<FString>&, UWorld* World)
	{
		ATDSquadManager* S = Squad(World);
		if (!S)
		{
			return;
		}
		if (S->GetActiveShipId() != ETDShipId::Lancer)
		{
			S->RequestSwitchTo(ETDShipId::Lancer);
		}
		ATDShip* Lancer = S->GetShip(ETDShipId::Lancer);
		if (!Lancer || S->GetActiveShipId() != ETDShipId::Lancer)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Director.RamKill: couldn't take Lancer (switch cooldown?); try again"));
			return;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector At = Lancer->GetActorLocation() + Lancer->GetActorForwardVector() * 9000.f;
		ATDEnemyShip* Fighter = World->SpawnActor<ATDEnemyShip>(ATDEnemyShip::StaticClass(), At, (-Lancer->GetActorForwardVector()).Rotation(), Params);
		if (!Fighter)
		{
			return;
		}
		Fighter->InitFromSpec(TDEnemyIds::ShardFighter);
		Fighter->SetFlightParked(true); // a sitting duck so the ram line is exact
		UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true);
		UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.RamKill: Lancer ramming a fighter 9000 cm ahead"));
	}

	static ATDEnemyShip* SpawnParkedFighter(UWorld* World, const FVector& At, const FVector& Facing)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATDEnemyShip* E = World->SpawnActor<ATDEnemyShip>(ATDEnemyShip::StaticClass(), At, Facing.Rotation(), Params);
		if (E)
		{
			E->InitFromSpec(TDEnemyIds::ShardFighter);
			E->SetFlightParked(true);
		}
		return E;
	}

	static void MultiKill(const TArray<FString>&, UWorld* World)
	{
		ATDSquadManager* S = Squad(World);
		ATDShip* Player = S ? S->GetActiveShip() : nullptr;
		if (!Player)
		{
			return;
		}
		// Three enemies in front of the player, so the shot has something to frame.
		TArray<ATDEnemyShip*> Victims;
		for (int32 i = 0; i < 3; ++i)
		{
			const FVector At = Player->GetActorLocation() + Player->GetActorForwardVector() * (6000.f + 1500.f * i) + Player->GetActorRightVector() * (i - 1) * 1500.f;
			if (ATDEnemyShip* E = SpawnParkedFighter(World, At, -Player->GetActorForwardVector()))
			{
				Victims.Add(E);
			}
		}
		for (ATDEnemyShip* E : Victims)
		{
			UTDStatics::ApplyTDDamage(E, 1000000.f, ETDDamageKind::Energy, Player, Player);
		}
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.MultiKill: the player's ship killed %d fighters"), Victims.Num());
	}

	static void MissileReturn(const TArray<FString>&, UWorld* World)
	{
		ATDSquadManager* S = Squad(World);
		if (!S)
		{
			return;
		}
		if (S->GetActiveShipId() != ETDShipId::Rift)
		{
			S->RequestSwitchTo(ETDShipId::Rift);
		}
		ATDShip* Rift = S->GetShip(ETDShipId::Rift);
		UTDPortalBlasterPower* Portal = Rift ? Cast<UTDPortalBlasterPower>(UTDPowerComponent::FindPower(Rift, ETDPowerInput::Signature1)) : nullptr;
		if (!Portal || S->GetActiveShipId() != ETDShipId::Rift)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Director.MissileReturn: couldn't take Rift (switch cooldown?); try again"));
			return;
		}
		// The power test's layout: portal A ahead, portal B to the side, a missile from ahead at Rift.
		Rift->SetFlightParked(true);
		const FRotator Rot = Rift->GetActorRotation();
		Portal->HandleInputPressed();
		Rift->SetActorRotation(Rot + FRotator(0.f, 90.f, 0.f));
		Portal->HandleInputPressed();
		Rift->SetActorRotation(Rot);
		const FVector F = Rot.Vector();
		ATDEnemyShip* Shooter = SpawnParkedFighter(World, Rift->GetActorLocation() + F * 20000.f, -F);
		ATDProjectile::Spawn(World, UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile,
			FTransform((-F).Rotation(), Rift->GetActorLocation() + F * 15000.f), Shooter, ETDTeam::Enemy, Rift);
		FTimerHandle H;
		TWeakObjectPtr<ATDShip> Weak(Rift);
		World->GetTimerManager().SetTimer(H, [Weak] { if (Weak.IsValid()) { Weak->SetFlightParked(false); } }, 4.f, false);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.MissileReturn: portals open, missile inbound"));
	}

	static void RevealDominion(const TArray<FString>&, UWorld* World)
	{
		if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(World))
		{
			Flow->SetDominionState(true, false);
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.RevealDominion: revealed (it warps in)"));
		}
	}

	static void GeneratorKill(const TArray<FString>&, UWorld* World)
	{
		ATDSquadManager* S = Squad(World);
		ATDShip* Player = S ? S->GetActiveShip() : nullptr;
		for (TActorIterator<ATDShieldGenerator> It(World); It; ++It)
		{
			const UTDHealthComponent* H = UTDStatics::GetHealth(*It);
			if (H && H->IsAlive())
			{
				UTDStatics::ApplyTDDamage(*It, 1000000.f, ETDDamageKind::Explosive, Player, Player);
				UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.GeneratorKill: %s destroyed"), *It->GetName());
				return;
			}
		}
		UE_LOG(LogTriadDrift, Warning, TEXT("TD.Director.GeneratorKill: no live generator (TD.Director.RevealDominion first)"));
	}

	static void Status(const TArray<FString>&, UWorld* World)
	{
		UTDDirectorSubsystem* D = UTDDirectorSubsystem::Get(World);
		if (!D)
		{
			return;
		}
		const UTDDirectorCameraRig* Rig = D->GetCameraRig();
		const UTDCutPresentation* P = D->GetCutPresentation();
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.Status: mode=%d shots=%d camera=%s presentation widget=%d empty=%d"),
			static_cast<int32>(UTDDirectorSettings::GetEffectiveMode()), D->GetShotCount(),
			Rig ? *UEnum::GetValueAsString(Rig->GetPhase()) : TEXT("-"), P && P->HasViewportWidget(), P ? P->GetState().IsEmpty() : 1);
	}

	/** Normalised screen position (-1..1, +Y down) of P from the player's view, false if off screen. */
	static bool ScreenPos(APlayerController* PC, const FVector& P, FVector2D& Out)
	{
		int32 W = 0, H = 0;
		PC->GetViewportSize(W, H);
		FVector2D Px;
		if (W <= 0 || H <= 0 || !PC->ProjectWorldLocationToScreen(P, Px, false))
		{
			Out = FVector2D(9.f, 9.f);
			return false;
		}
		Out = FVector2D(Px.X / W * 2.f - 1.f, Px.Y / H * 2.f - 1.f);
		return FMath::Abs(Out.X) <= 1.f && FMath::Abs(Out.Y) <= 1.f;
	}

	static void Frame(const TArray<FString>&, UWorld* World)
	{
		ATDSquadManager* S = Squad(World);
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		ATDShip* Player = S ? S->GetActiveShip() : nullptr;
		if (!S || !PC || !Player)
		{
			return;
		}
		FString Wing;
		int32 WingOn = 0;
		for (ATDShip* Mate : S->GetLivingShips())
		{
			if (Mate == Player)
			{
				continue;
			}
			FVector2D P;
			const bool bOn = ScreenPos(PC, Mate->GetActorLocation(), P);
			WingOn += bOn ? 1 : 0;
			// Why it is where it is: its state, and whether its target is itself on screen.
			FString Why = TEXT("-");
			if (const ATDWingmanAIController* AI = Cast<ATDWingmanAIController>(Mate->GetController()))
			{
				FVector2D TP;
				const AActor* T = AI->GetCurrentTarget();
				Why = UEnum::GetDisplayValueAsText(AI->GetWingmanState()).ToString()
					+ (T ? (ScreenPos(PC, T->GetActorLocation(), TP) ? TEXT("/tgtON") : TEXT("/tgtOff")) : TEXT(""))
					+ (AI->IsDefendingPlayer() ? TEXT("/def") : TEXT(""));
			}
			Wing += FString::Printf(TEXT(" %s=%s(%.2f,%.2f)[%s]"), *UEnum::GetDisplayValueAsText(Mate->GetShipId()).ToString(), bOn ? TEXT("ON") : TEXT("off"), P.X, P.Y, *Why);
		}
		const ATDEnemyShip* Nearest = nullptr;
		float Best = BIG_NUMBER;
		int32 EnemiesOn = 0, Enemies = 0;
		for (TActorIterator<ATDEnemyShip> It(World); It; ++It)
		{
			const UTDHealthComponent* H = UTDStatics::GetHealth(*It);
			if (H && H->IsDead())
			{
				continue;
			}
			++Enemies;
			FVector2D P;
			EnemiesOn += ScreenPos(PC, It->GetActorLocation(), P) ? 1 : 0;
			// "Nearest" the way a pilot picks: close AND ahead of the nose (a target behind costs 3x).
			const FVector To = It->GetActorLocation() - Player->GetActorLocation();
			const float Ahead = FVector::DotProduct(To.GetSafeNormal(), Player->GetActorForwardVector());
			const float D = To.Size() * (2.f - Ahead) * 0.5f;
			if (D < Best)
			{
				Best = D;
				Nearest = *It;
			}
		}
		FString Enemy = TEXT("none");
		if (Nearest)
		{
			// Direction to it in the player's frame (for steering even when it is behind the camera).
			const FVector L = Player->GetActorTransform().InverseTransformVectorNoScale(Nearest->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal();
			FVector2D P;
			const bool bOn = ScreenPos(PC, Nearest->GetActorLocation(), P);
			Enemy = FString::Printf(TEXT("%s(%.2f,%.2f) score=%.0fm local=(%.2f,%.2f,%.2f)"), bOn ? TEXT("ON") : TEXT("off"), P.X, P.Y, Best / 100.f, L.X, L.Y, L.Z);
		}
		const FRotator R = Player->GetActorRotation();
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Squad.Frame: wingOn=%d%s | enemiesOn=%d/%d nearest=%s | player=%s rot=(P %.1f Y %.1f R %.1f)"), WingOn, *Wing, EnemiesOn, Enemies, *Enemy,
			*UEnum::GetDisplayValueAsText(Player->GetShipId()).ToString(), R.Pitch, R.Yaw, R.Roll);
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdMulti(TEXT("TD.Director.MultiKill"), TEXT("TD.Director.MultiKill: three kills in one beat"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MultiKill));
	static FAutoConsoleCommandWithWorldAndArgs CmdMissile(TEXT("TD.Director.MissileReturn"), TEXT("TD.Director.MissileReturn: Rift sends a missile back through its portals"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MissileReturn));
	static FAutoConsoleCommandWithWorldAndArgs CmdKillActive(TEXT("TD.Squad.KillActive"), TEXT("TD.Squad.KillActive: kill the player's ship"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			ATDSquadManager* S = Squad(World);
			ATDShip* Ship = S ? S->GetActiveShip() : nullptr;
			if (UTDHealthComponent* H = UTDStatics::GetHealth(Ship))
			{
				H->Kill();
				UE_LOG(LogTriadDrift, Display, TEXT("TD.Squad.KillActive: %s killed (frame %llu)"), *UEnum::GetDisplayValueAsText(S->GetActiveShipId()).ToString(), GFrameCounter);
			}
		}));
	static FAutoConsoleCommandWithWorldAndArgs CmdCheck(TEXT("TD.Squad.Check"), TEXT("TD.Squad.Check <label>: log the switch-relevant state"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDSquadManager* S = Squad(World);
			APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			if (!S || !PC)
			{
				return;
			}
			const ATDShip* Active = S->GetActiveShip();
			const AActor* View = PC->GetViewTarget();
			const UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(World);
			UE_LOG(LogTriadDrift, Display, TEXT("TDCHECK %s frame=%llu active=%s alive=%d pawnIsActive=%d view=%s slowmo=%d paused=%d dilation=%.2f"),
				Args.Num() > 0 ? *Args[0] : TEXT("-"), GFrameCounter, *UEnum::GetDisplayValueAsText(S->GetActiveShipId()).ToString(),
				S->IsShipAlive(S->GetActiveShipId()), PC->GetPawn() == Active,
				!View ? TEXT("none") : (View == Active ? TEXT("active-ship") : (View->IsA<ATDDirectorCameraActor>() ? TEXT("director-cam") : *View->GetName())),
				Feel && Feel->IsSlowMoActive(), World->IsPaused(), World->GetWorldSettings() ? World->GetWorldSettings()->GetEffectiveTimeDilation() : 1.f);
		}));
	static FAutoConsoleCommandWithWorldAndArgs CmdFrame(TEXT("TD.Squad.Frame"), TEXT("TD.Squad.Frame: log wingmen / nearest enemy screen positions"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Frame));
	static FAutoConsoleCommandWithWorldAndArgs CmdSwitch(TEXT("TD.Squad.Switch"), TEXT("TD.Squad.Switch <Rift|Lancer|Bastion|Next|Prev>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Switch));
	static FAutoConsoleCommandWithWorldAndArgs CmdWipe(TEXT("TD.Squad.Wipe"), TEXT("TD.Squad.Wipe: kill all three squad ships (CUT! + retake)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Wipe));
	static FAutoConsoleCommandWithWorldAndArgs CmdRam(TEXT("TD.Director.RamKill"), TEXT("TD.Director.RamKill: Lancer rams a real fighter"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RamKill));
	static FAutoConsoleCommandWithWorldAndArgs CmdReveal(TEXT("TD.Director.RevealDominion"), TEXT("TD.Director.RevealDominion: reveal the Dominion"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RevealDominion));
	static FAutoConsoleCommandWithWorldAndArgs CmdGen(TEXT("TD.Director.GeneratorKill"), TEXT("TD.Director.GeneratorKill: destroy a live shield generator"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GeneratorKill));
	static FAutoConsoleCommandWithWorldAndArgs CmdStatus(TEXT("TD.Director.Status"), TEXT("TD.Director.Status: log director state"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Status));
}

#endif // !UE_BUILD_SHIPPING
