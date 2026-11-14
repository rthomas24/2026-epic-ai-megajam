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
//   TD.Director.CarrierKill       the player's ship destroys a real Hive Carrier parked ahead
//   TD.Director.ReactorKill       first run: destroys every live generator; once exposed: destroys the reactor
//   TD.Director.StrikeKill        power -> switch (Switch Strike) -> the new ship kills a fighter ahead
//   TD.Squad.Passive <0|1>        wingmen hold fire and formation (weapon checks without the wing stealing targets)
//   TD.Squad.KillActive           kill the ship the player flies (the tumble, then the auto-switch)
//   TD.Squad.Check <label>        log frame, active ship, possessed pawn, view target, slow-mo, pause (switch stress)
//   TD.Intro.Play                 play the ENG-33 cinematic intro now (no waves are started by it here)
//   TD.Intro.At <s> [hold]        jump to s seconds into the intro (replayed in 1/30 s steps); "hold" freezes it there
//   TD.Intro.Probe <u> <v>        log what is at a screen point (0..1) from the current view (captures triage)
//   TD.Intro.Run                  unfreeze;  TD.Intro.Skip  skip it (after the first second)
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
#include "Blueprint/UserWidget.h"
#include "TriadDrift/Director/TDDirectorCamera.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Flow/TDFrontEndGameModes.h"
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
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"
#include "TriadDrift/Director/Menu/TDMenuBackdrop.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"

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

	static void CarrierKill(const TArray<FString>&, UWorld* World)
	{
		ATDSquadManager* S = Squad(World);
		ATDShip* Player = S ? S->GetActiveShip() : nullptr;
		if (!Player)
		{
			return;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector At = Player->GetActorLocation() + Player->GetActorForwardVector() * 18000.f;
		ATDEnemyShip* Carrier = World->SpawnActor<ATDEnemyShip>(ATDEnemyShip::StaticClass(), At, (-Player->GetActorForwardVector()).Rotation(), Params);
		if (!Carrier)
		{
			return;
		}
		Carrier->InitFromSpec(TDEnemyIds::HiveCarrier);
		Carrier->SetFlightParked(true);
		UTDStatics::ApplyTDDamage(Carrier, 10000000.f, ETDDamageKind::Explosive, Player, Player);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.CarrierKill: carrier destroyed 180 m ahead"));
	}

	static void ReactorKill(const TArray<FString>& Args, UWorld* World)
	{
		ATDSquadManager* S = Squad(World);
		ATDShip* Player = S ? S->GetActiveShip() : nullptr;
		for (TActorIterator<ATDDominionReactor> It(World); It; ++It)
		{
			const UTDHealthComponent* H = UTDStatics::GetHealth(*It);
			if (H && H->IsAlive() && !H->IsInvulnerable())
			{
				UTDStatics::ApplyTDDamage(*It, 100000000.f, ETDDamageKind::Explosive, Player, Player);
				UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.ReactorKill: reactor phase destroyed (%s)"), H->IsAlive() ? TEXT("next phase opens shortly") : TEXT("the reactor is dead"));
				return;
			}
			const ATDDominion* Dom = ATDDominion::Get(World);
			if (H && H->IsAlive() && Dom && Dom->GetState() == ETDDominionState::ReactorExposed)
			{
				// Between phases (a short invulnerable beat): the generators are irrelevant now, so don't touch them.
				UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.ReactorKill: reactor between phases; run again in a second"));
				return;
			}
		}
		int32 Killed = 0;
		for (TActorIterator<ATDShieldGenerator> It(World); It; ++It)
		{
			const UTDHealthComponent* H = UTDStatics::GetHealth(*It);
			if (H && H->IsAlive())
			{
				UTDStatics::ApplyTDDamage(*It, 1000000.f, ETDDamageKind::Explosive, Player, Player);
				++Killed;
			}
		}
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.ReactorKill: reactor not exposed yet; destroyed %d generator(s), run again once exposed"), Killed);
	}

	static void StrikeKill(const TArray<FString>&, UWorld* World)
	{
		ATDSquadManager* S = Squad(World);
		UTDEventBus* Bus = UTDEventBus::Get(World);
		if (!S || !Bus || !S->GetActiveShip())
		{
			return;
		}
		Bus->BroadcastPowerUsed(S->GetActiveShipId(), TEXT("DebugPower"));
		if (!S->RequestSwitchNext() || !S->IsSwitchStrikeActive())
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Director.StrikeKill: switch refused (cooldown?); try again"));
			return;
		}
		ATDShip* Player = S->GetActiveShip();
		ATDEnemyShip* Victim = SpawnParkedFighter(World, Player->GetActorLocation() + Player->GetActorForwardVector() * 7000.f, -Player->GetActorForwardVector());
		if (Victim)
		{
			UTDStatics::ApplyTDDamage(Victim, 1000000.f, ETDDamageKind::Energy, Player, Player);
		}
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.StrikeKill: %s killed a fighter under Switch Strike"), *UEnum::GetDisplayValueAsText(S->GetActiveShipId()).ToString());
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

	static FAutoConsoleCommandWithWorldAndArgs CmdCarrier(TEXT("TD.Director.CarrierKill"), TEXT("TD.Director.CarrierKill: destroy a real carrier ahead"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CarrierKill));
	static FAutoConsoleCommandWithWorldAndArgs CmdReactor(TEXT("TD.Director.ReactorKill"), TEXT("TD.Director.ReactorKill: generators, then the reactor"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ReactorKill));
	static FAutoConsoleCommandWithWorldAndArgs CmdStrike(TEXT("TD.Director.StrikeKill"), TEXT("TD.Director.StrikeKill: a kill under Switch Strike"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StrikeKill));
	static FAutoConsoleCommandWithWorldAndArgs CmdMulti(TEXT("TD.Director.MultiKill"), TEXT("TD.Director.MultiKill: three kills in one beat"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MultiKill));
	static FAutoConsoleCommandWithWorldAndArgs CmdMissile(TEXT("TD.Director.MissileReturn"), TEXT("TD.Director.MissileReturn: Rift sends a missile back through its portals"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MissileReturn));
	static FAutoConsoleCommandWithWorldAndArgs CmdPassive(TEXT("TD.Squad.Passive"), TEXT("TD.Squad.Passive <0|1>: wingmen hold fire (1) or fight (0)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (ATDSquadManager* S = Squad(World))
			{
				const bool bPassive = Args.Num() == 0 || FCString::Atoi(*Args[0]) != 0;
				S->SetWingmenPassive(bPassive);
				UE_LOG(LogTriadDrift, Display, TEXT("TD.Squad.Passive: wingmen %s"), bPassive ? TEXT("passive (hold fire)") : TEXT("fighting"));
			}
		}));
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

	static void IntroPlay(const TArray<FString>& Args, UWorld* World)
	{
		UTDIntroDirector* Intro = UTDIntroDirector::Get(World);
		const bool bOk = Intro && Intro->Play(FSimpleDelegate::CreateLambda([]() { UE_LOG(LogTriadDrift, Display, TEXT("TD.Intro: finished")); }));
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Intro.Play: %s"), bOk ? TEXT("playing") : TEXT("nothing to play (td.Intro 0, no viewport, or already playing)"));
	}
	static void IntroAt(const TArray<FString>& Args, UWorld* World)
	{
		UTDIntroDirector* Intro = UTDIntroDirector::Get(World);
		if (Intro && !Intro->IsPlaying())
		{
			IntroPlay(Args, World);
		}
		if (Intro && Intro->IsPlaying() && Args.Num() > 0)
		{
			Intro->DebugSetTime(FCString::Atof(*Args[0]), Args.Num() > 1 && Args[1].Equals(TEXT("hold"), ESearchCase::IgnoreCase));
			const int32 Shot = Intro->GetShotIndex();
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Intro.At %.2f: shot %d (%s)%s"), Intro->GetElapsed(), Shot,
				Intro->GetShots().IsValidIndex(Shot) ? *Intro->GetShots()[Shot].Id.ToString() : TEXT("-"), Intro->IsDebugFrozen() ? TEXT(", held") : TEXT(""));
		}
	}
	static void IntroRun(const TArray<FString>& Args, UWorld* World)
	{
		if (UTDIntroDirector* Intro = UTDIntroDirector::Get(World); Intro && Intro->IsPlaying())
		{
			Intro->DebugSetTime(Intro->GetElapsed(), false);
		}
	}
	static void IntroSkip(const TArray<FString>& Args, UWorld* World)
	{
		UTDIntroDirector* Intro = UTDIntroDirector::Get(World);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Intro.Skip: %s"), Intro && Intro->Skip() ? TEXT("skipped") : TEXT("not playing or in its first second"));
	}
	static void IntroProbe(const TArray<FString>& Args, UWorld* World)
	{
		// What is at a point of the screen (u, v in 0..1 of the 16:9 frame), from the current view: name, class, mesh.
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (!PC || !PC->PlayerCameraManager || Args.Num() < 2)
		{
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Intro.Probe <u> <v>: no view"));
			return;
		}
		const FMinimalViewInfo POV = PC->PlayerCameraManager->GetCameraCacheView();
		const float U = FCString::Atof(*Args[0]) * 2.f - 1.f;
		const float V = 1.f - FCString::Atof(*Args[1]) * 2.f;
		const float TanH = FMath::Tan(FMath::DegreesToRadians(POV.FOV * 0.5f));
		const FVector Dir = POV.Rotation.RotateVector(FVector(1.f, U * TanH, V * TanH * 9.f / 16.f)).GetSafeNormal();
		FCollisionQueryParams Q(SCENE_QUERY_STAT(TDIntroProbe), true);
		TArray<FHitResult> Hits;
		// Visual-only dressing has no collision: test bounds of every primitive along the ray as well.
		World->LineTraceMultiByChannel(Hits, POV.Location, POV.Location + Dir * 2.0e6f, ECC_Visibility, Q);
		for (const FHitResult& H : Hits)
		{
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Intro.Probe hit %s.%s (%s) at %.0f m"), *GetNameSafe(H.GetActor()), *GetNameSafe(H.GetComponent()),
				H.GetComponent() ? *H.GetComponent()->GetClass()->GetName() : TEXT("-"), H.Distance / 100.f);
		}
		int32 Listed = 0;
		for (TObjectIterator<UPrimitiveComponent> It; It && Listed < 12; ++It)
		{
			if (It->GetWorld() != World || !It->IsVisible() || (It->GetOwner() && It->GetOwner()->IsHidden()) || It->Bounds.SphereRadius > 200000.f)
			{
				continue;
			}
			const FVector C = It->Bounds.Origin;
			const float Along = FVector::DotProduct(C - POV.Location, Dir);
			if (Along > 0.f && FMath::PointDistToLine(C, Dir, POV.Location) < It->Bounds.SphereRadius && Along < 500000.f)
			{
				++Listed;
				const UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(*It);
				UE_LOG(LogTriadDrift, Display, TEXT("TD.Intro.Probe on ray: %s.%s (%s, mesh %s, mat %s) %.0f m away, radius %.0f m"),
					*GetNameSafe(It->GetOwner()), *It->GetName(), *It->GetClass()->GetName(), SM ? *GetNameSafe(SM->GetStaticMesh()) : TEXT("-"),
					*GetNameSafe(It->GetMaterial(0)), Along / 100.f, It->Bounds.SphereRadius / 100.f);
			}
		}
	}
	static void MenuAt(const TArray<FString>& Args, UWorld* World)
	{
		for (TActorIterator<ATDMenuBackdrop> It(World); It; ++It)
		{
			It->DebugSetTime(Args.Num() ? FCString::Atof(*Args[0]) : 0.f);
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Menu.At: fly-by %d, camera %s"), It->IsFlybyActive(), It->GetCamera() ? *It->GetCamera()->GetActorLocation().ToCompactString() : TEXT("-"));
			for (const UStaticMeshComponent* C : It->GetSquadMeshes())
			{
				UE_LOG(LogTriadDrift, Display, TEXT("TD.Menu.At:   %s at %s, visible %d, mesh %s, bounds %.0f"), *C->GetName(), *C->GetComponentLocation().ToCompactString(),
					C->IsVisible(), *GetNameSafe(C->GetStaticMesh()), C->Bounds.SphereRadius);
			}
		}
	}
	// N-092 (Nick's Director card candidate, review only; production defaults untouched):
	//   TD.Director.Widget /Script/TriadDriftHUD.TDDirectorCardsWidget   use that renderer now (no argument: back to the style's)
	//   TD.Director.Claim 1 | 0 | -1                                     force the HUD's name-card claim true / false / real
	static FAutoConsoleCommandWithWorldAndArgs CmdDirectorWidget(TEXT("TD.Director.Widget"),
		TEXT("TD.Director.Widget [class path]: review a Director card renderer (no argument: back to DA_UI_Style's)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UTDDirectorSubsystem* D = UTDDirectorSubsystem::Get(World);
			if (!D) { return; }
			UClass* Class = Args.Num() > 0 ? LoadClass<UUserWidget>(nullptr, *Args[0]) : nullptr;
			D->SetDirectorWidgetForReview(Class);
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.Widget: %s"), Class ? *Class->GetPathName() : TEXT("the style's (or the fallback)"));
		}));
#if WITH_DEV_AUTOMATION_TESTS
	static FAutoConsoleCommandWithWorldAndArgs CmdDirectorClaim(TEXT("TD.Director.Claim"),
		TEXT("TD.Director.Claim 1|0|-1: force the hosted HUD's name-card claim (review); -1 reads the real HUD"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
		{
			const int32 V = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : -1;
			if (V < 0) { UTDCutPresentation::NameCardClaimOverrideForTests.Reset(); }
			else { UTDCutPresentation::NameCardClaimOverrideForTests = V != 0; }
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Director.Claim: %s"), V < 0 ? TEXT("the real HUD") : (V ? TEXT("claimed") : TEXT("not claimed")));
		}));
#endif
	static FAutoConsoleCommandWithWorldAndArgs CmdMenuDifficulty(TEXT("TD.Menu.Difficulty"), TEXT("TD.Menu.Difficulty [Story|Standard|Ace|Back]: press NEW GAME (no argument: the difficulty page opens), or one of its choices"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDMenuGameMode* M = World ? World->GetAuthGameMode<ATDMenuGameMode>() : nullptr;
			if (!M) { return; }
			if (Args.Num() == 0) { M->ChooseDifficulty(false); return; }
			if (Args[0].Equals(TEXT("Back"), ESearchCase::IgnoreCase)) { M->CancelDifficultyChoice(); return; }
			M->StartWithDifficulty(Args[0].Equals(TEXT("Story"), ESearchCase::IgnoreCase) ? 0 : (Args[0].Equals(TEXT("Ace"), ESearchCase::IgnoreCase) ? 2 : 1));
		}));
	static FAutoConsoleCommandWithWorldAndArgs CmdMenuReturn(TEXT("TD.Menu.Return"), TEXT("TD.Menu.Return: press RETURN TO MENU on the victory screen (captures, tests)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (ATDMenuGameMode* M = World ? World->GetAuthGameMode<ATDMenuGameMode>() : nullptr)
			{
				M->ReturnToMenu();
			}
		}));
	static FAutoConsoleCommandWithWorldAndArgs CmdMenuAt(TEXT("TD.Menu.At"), TEXT("TD.Menu.At <seconds>: jump the menu backdrop's loop (captures)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MenuAt));
	static FAutoConsoleCommandWithWorldAndArgs CmdIntroProbe(TEXT("TD.Intro.Probe"), TEXT("TD.Intro.Probe <u> <v>: log what is at that screen point"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&IntroProbe));
	static FAutoConsoleCommandWithWorldAndArgs CmdIntroPlay(TEXT("TD.Intro.Play"), TEXT("TD.Intro.Play: play the cinematic intro now"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&IntroPlay));
	static FAutoConsoleCommandWithWorldAndArgs CmdIntroAt(TEXT("TD.Intro.At"), TEXT("TD.Intro.At <seconds> [hold]: jump into the intro (captures)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&IntroAt));
	static FAutoConsoleCommandWithWorldAndArgs CmdIntroRun(TEXT("TD.Intro.Run"), TEXT("TD.Intro.Run: unfreeze the intro"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&IntroRun));
	static FAutoConsoleCommandWithWorldAndArgs CmdIntroSkip(TEXT("TD.Intro.Skip"), TEXT("TD.Intro.Skip: skip the intro"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&IntroSkip));
}

#endif // !UE_BUILD_SHIPPING
