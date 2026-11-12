#include "TriadDrift/Core/TDDebugHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"

static TAutoConsoleVariable<int32> CVarTDDebugHUD(
	TEXT("TD.DebugHUD"), 1,
	TEXT("1 = draw the engineering placeholder HUD when no real HUD is set, 0 = off."),
	ECVF_Default);

namespace
{
	const FLinearColor ColorRift(1.f, 0.17f, 0.84f);
	const FLinearColor ColorLancer(0.25f, 0.91f, 1.f);
	const FLinearColor ColorBastion(1.f, 0.7f, 0.12f);
	const FLinearColor ColorEnemy(1.f, 0.15f, 0.2f);
	const FLinearColor ColorDim(0.7f, 0.75f, 0.8f);

	FLinearColor ShipColor(ETDShipId Ship)
	{
		switch (Ship)
		{
		case ETDShipId::Rift: return ColorRift;
		case ETDShipId::Lancer: return ColorLancer;
		default: return ColorBastion;
		}
	}

	const TCHAR* ShipName(ETDShipId Ship)
	{
		switch (Ship)
		{
		case ETDShipId::Rift: return TEXT("RIFT");
		case ETDShipId::Lancer: return TEXT("LANCER");
		default: return TEXT("BASTION");
		}
	}

	FString Bar(float Value01, int32 Cells = 10)
	{
		const int32 Filled = FMath::Clamp(FMath::RoundToInt(Value01 * Cells), 0, Cells);
		FString Out;
		for (int32 Index = 0; Index < Cells; ++Index)
		{
			Out += Index < Filled ? TEXT("|") : TEXT(".");
		}
		return Out;
	}
}

// ------------------------------------------------------------------------------------------------ HUD

AActor* ATDDebugHUD::FindHUDDataActor()
{
	// The player's pawn wins when it serves HUD data itself (the first-person character). Checked
	// every call and never cached: the pawn changes on every ship switch and on landing.
	if (APawn* Pawn = GetOwningPawn())
	{
		if (Cast<ITDHUDData>(Pawn))
		{
			return Pawn;
		}
	}
	if (AActor* Cached = CachedHUDDataActor.Get())
	{
		return Cached;
	}

	// Otherwise any non-pawn actor that serves HUD data (the squad manager). Searched at most twice
	// a second; this function is called several times per frame.
	const UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() < NextSearchTime)
	{
		return nullptr;
	}
	NextSearchTime = World->GetTimeSeconds() + 0.5f;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (Cast<ITDHUDData>(*It) && !It->IsA<APawn>())
		{
			CachedHUDDataActor = *It;
			return *It;
		}
	}
	return nullptr;
}

void ATDDebugHUD::DrawLine2D(const FVector2D& A, const FVector2D& B, const FLinearColor& Color, float Thickness)
{
	DrawLine(A.X, A.Y, B.X, B.Y, Color, Thickness);
}

void ATDDebugHUD::DrawBracket(const FVector2D& C, float H, const FLinearColor& Color)
{
	const float L = H * 0.45f;
	for (int32 SX = -1; SX <= 1; SX += 2)
	{
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			const FVector2D Corner(C.X + SX * H, C.Y + SY * H);
			DrawLine2D(Corner, Corner - FVector2D(SX * L, 0.f), Color, 2.f);
			DrawLine2D(Corner, Corner - FVector2D(0.f, SY * L), Color, 2.f);
		}
	}
}

void ATDDebugHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas || CVarTDDebugHUD.GetValueOnGameThread() == 0)
	{
		return;
	}
	// Start below the tallest letterbox bar (12% of the height) so the cut presentation never covers these rows.
	float Y = FMath::Max(24.f, Canvas->SizeY * 0.12f);
	DrawSquad(Y);
	DrawWave();
	DrawTargets();
	DrawCrosshair();
	DrawFlight();
}

void ATDDebugHUD::DrawSquad(float& Y)
{
	const ITDHUDData* Data = Cast<ITDHUDData>(FindHUDDataActor());
	if (!Data)
	{
		DrawText(TEXT("TRIAD DRIFT dev HUD (no HUD data source yet)"), ColorDim, 24.f, Y, nullptr, 1.1f);
		Y += 22.f;
		return;
	}
	const TArray<FTDShipHUD> Cards = Data->GetSquadHUD();
	for (const FTDShipHUD& Card : Cards)
	{
		const FLinearColor Color = Card.bAlive ? ShipColor(Card.Ship) : FLinearColor(0.35f, 0.35f, 0.35f);
		FString Line = FString::Printf(TEXT("%s %-8s"), Card.bPlayerControlled ? TEXT(">>") : TEXT("  "), ShipName(Card.Ship));
		if (Card.bAlive)
		{
			Line += FString::Printf(TEXT(" HULL %s  SHLD %s  PWR %s"), *Bar(Card.Health01), *Bar(Card.Shield01), *Bar(Card.PowerCooldown01, 5));
			if (Card.bPlayerControlled)
			{
				Line += FString::Printf(TEXT("  BOOST %s  HEAT %s  AMMO %d"), *Bar(Card.Boost01, 6), *Bar(Card.Heat01, 6), Card.SecondaryAmmo);
			}
		}
		else
		{
			Line += FString::Printf(TEXT(" DOWN - back in %.0f s"), Card.RespawnSeconds);
		}
		DrawText(Line, Color, 24.f, Y, nullptr, Card.bPlayerControlled ? 1.25f : 1.05f);
		Y += Card.bPlayerControlled ? 24.f : 20.f;
	}

	// Ground half (first-person character).
	if (Cards.Num() == 0)
	{
		const FString Line = FString::Printf(TEXT("HEALTH %s  BLOCK %s  DASH %d   %s"), *Bar(Data->GetHealth01()),
			*Bar(Data->GetBlockStamina01(), 6), Data->GetDashCharges(), *Data->GetInteractPrompt().ToString());
		DrawText(Line, FLinearColor::White, 24.f, Y, nullptr, 1.2f);
		Y += 24.f;
	}
}

void ATDDebugHUD::DrawWave()
{
	const ITDHUDData* Data = Cast<ITDHUDData>(FindHUDDataActor());
	if (!Data)
	{
		return;
	}
	const FTDWaveInfo Wave = Data->GetWaveInfo();
	FString Text;
	if (Wave.bInBreather)
	{
		Text = FString::Printf(TEXT("WAVE %d CLEARED - next in %.0f s"), Wave.WaveNumber, Wave.BreatherRemaining);
	}
	else if (Wave.bWaveActive)
	{
		Text = FString::Printf(TEXT("WAVE %d / %d   enemies %d / %d"), Wave.WaveNumber, Wave.WaveCount, Wave.EnemiesAlive, Wave.EnemiesTotal);
	}
	if (!Text.IsEmpty())
	{
		float W = 0.f, H = 0.f;
		GetTextSize(Text, W, H, nullptr, 1.4f);
		DrawText(Text, FLinearColor::White, Canvas->SizeX - W - 24.f, FMath::Max(22.f, Canvas->SizeY * 0.12f), nullptr, 1.4f);
	}
	if (Data->IsOutOfBounds())
	{
		const FString Warn(TEXT("LEAVING THE COMBAT ZONE - TURN BACK"));
		float W = 0.f, H = 0.f;
		GetTextSize(Warn, W, H, nullptr, 1.6f);
		DrawText(Warn, ColorEnemy, (Canvas->SizeX - W) * 0.5f, Canvas->SizeY * 0.22f, nullptr, 1.6f);
	}
}

void ATDDebugHUD::DrawTargets()
{
	const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	const APawn* Pawn = GetOwningPawn();
	if (!Registry || !Pawn)
	{
		return;
	}
	const ITDHUDData* Data = Cast<ITDHUDData>(FindHUDDataActor());
	const AActor* Current = Data ? Data->GetCurrentTarget() : nullptr;
	const FVector Eye = Pawn->GetActorLocation();
	const ETDTeam MyTeam = UTDStatics::GetTeam(Pawn);
	const FVector2D ScreenCenter(Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f);

	for (const TWeakObjectPtr<AActor>& Entry : Registry->GetAll())
	{
		const AActor* Actor = Entry.Get();
		if (!IsValid(Actor) || Actor == Pawn)
		{
			continue;
		}
		const UTDHealthComponent* Health = Actor->FindComponentByClass<UTDHealthComponent>();
		if (!Health || Health->IsDead())
		{
			continue;
		}
		const ETDTeam Team = UTDStatics::GetTeam(Actor);
		const bool bHostile = UTDStatics::AreTeamsHostile(MyTeam, Team);
		const bool bFriendly = Team == MyTeam;
		if (!bHostile && !bFriendly)
		{
			continue;
		}

		const FVector Screen = Project(Actor->GetActorLocation());
		const bool bOnScreen = Screen.Z > 0.f && Screen.X >= 0.f && Screen.X <= Canvas->SizeX && Screen.Y >= 0.f && Screen.Y <= Canvas->SizeY;
		const float DistanceM = static_cast<float>(FVector::Dist(Eye, Actor->GetActorLocation())) / 100.f;
		const FLinearColor Color = bHostile ? ColorEnemy : FLinearColor(0.3f, 1.f, 0.5f);

		if (bOnScreen)
		{
			const bool bIsCurrent = Actor == Current;
			const float Half = bIsCurrent ? 26.f : (bHostile ? 16.f : 10.f);
			DrawBracket(FVector2D(Screen.X, Screen.Y), Half, bIsCurrent ? FLinearColor::White : Color);
			if (bHostile)
			{
				DrawText(FString::Printf(TEXT("%.0fm  %s"), DistanceM, *Bar(Health->GetHull01(), 5)), Color,
					Screen.X + Half + 4.f, Screen.Y - 8.f, nullptr, 0.9f);
			}
		}
		else if (bHostile)
		{
			// Edge arrow pointing toward the off-screen enemy.
			FVector2D Dir(Screen.X - ScreenCenter.X, Screen.Y - ScreenCenter.Y);
			if (Screen.Z <= 0.f)
			{
				Dir *= -1.f;
			}
			if (!Dir.Normalize())
			{
				Dir = FVector2D(0.f, 1.f);
			}
			const float Radius = FMath::Min(Canvas->SizeX, Canvas->SizeY) * 0.45f;
			const FVector2D Tip = ScreenCenter + Dir * Radius;
			const FVector2D Side(-Dir.Y, Dir.X);
			DrawLine2D(Tip, Tip - Dir * 18.f + Side * 9.f, Color, 2.f);
			DrawLine2D(Tip, Tip - Dir * 18.f - Side * 9.f, Color, 2.f);
		}
	}
}

void ATDDebugHUD::DrawCrosshair()
{
	const FVector2D C(Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f);
	const FLinearColor Color(1.f, 1.f, 1.f, 0.85f);
	DrawLine2D(C + FVector2D(-14.f, 0.f), C + FVector2D(-5.f, 0.f), Color, 2.f);
	DrawLine2D(C + FVector2D(5.f, 0.f), C + FVector2D(14.f, 0.f), Color, 2.f);
	DrawLine2D(C + FVector2D(0.f, -14.f), C + FVector2D(0.f, -5.f), Color, 2.f);
	DrawLine2D(C + FVector2D(0.f, 5.f), C + FVector2D(0.f, 14.f), Color, 2.f);
}

void ATDDebugHUD::DrawFlight()
{
	const APawn* Pawn = GetOwningPawn();
	const ITDHUDData* Data = Cast<ITDHUDData>(FindHUDDataActor());
	if (!Pawn)
	{
		return;
	}
	FString Line = FString::Printf(TEXT("SPEED %5.0f m/s"), static_cast<float>(Pawn->GetVelocity().Size()) / 100.f);
	if (Data)
	{
		const float Charge = Data->GetHyperdriveCharge01();
		if (Charge > 0.f)
		{
			Line += FString::Printf(TEXT("   HYPERDRIVE %s"), *Bar(Charge, 8));
		}
		const FText Block = Data->GetHyperdriveBlockText();
		if (!Block.IsEmpty())
		{
			Line += FString::Printf(TEXT("   %s"), *Block.ToString());
		}
		const float Altitude = Data->GetAltitude();
		if (Altitude >= 0.f)
		{
			Line += FString::Printf(TEXT("   ALT %4.0f m"), Altitude);
		}
	}
	DrawText(Line, ColorDim, 24.f, Canvas->SizeY - 44.f, nullptr, 1.2f);
}

// ------------------------------------------------------------------------------------------ installer

bool UTDDebugHUDSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
#endif
}

void UTDDebugHUDSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TimeUntilCheck -= DeltaTime;
	if (TimeUntilCheck > 0.f)
	{
		return;
	}
	TimeUntilCheck = 1.f;

	UWorld* World = GetWorld();
	if (!World || CVarTDDebugHUD.GetValueOnGameThread() == 0)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->IsLocalController())
		{
			continue;
		}
		const AHUD* Current = PC->GetHUD();
		// Only replace the stock HUD (or none). A real HUD class chosen by a game mode always wins.
		if (!Current || Current->GetClass() == AHUD::StaticClass())
		{
			PC->ClientSetHUD(ATDDebugHUD::StaticClass());
		}
	}
}

TStatId UTDDebugHUDSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDDebugHUDSubsystem, STATGROUP_Tickables);
}
