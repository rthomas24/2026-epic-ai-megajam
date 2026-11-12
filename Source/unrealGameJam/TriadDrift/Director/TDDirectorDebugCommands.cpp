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
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Squad.Frame: wingOn=%d%s | enemiesOn=%d/%d nearest=%s"), WingOn, *Wing, EnemiesOn, Enemies, *Enemy);
	}

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
