#include "TriadDrift/Core/TDDebugHUD.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "UObject/ObjectKey.h"

static TAutoConsoleVariable<int32> CVarTDDebugHUD(
	TEXT("TD.DebugHUD"), 1,
	TEXT("1 = draw the engineering placeholder HUD when no real HUD is set, 0 = off."),
	ECVF_Default);

static TSet<TObjectKey<UWorld>> GTDRealHUDWorlds;

void ATDDebugHUD::SetRealHUDActive(const UWorld* World, bool bActive)
{
	if (!World)
	{
		return;
	}
	if (bActive)
	{
		GTDRealHUDWorlds.Add(World);
	}
	else
	{
		GTDRealHUDWorlds.Remove(World);
	}
}

bool ATDDebugHUD::IsRealHUDActive(const UWorld* World)
{
	return World && GTDRealHUDWorlds.Contains(World);
}

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

	const TCHAR* PilotName(ETDShipId Ship)
	{
		switch (Ship)
		{
		case ETDShipId::Rift: return TEXT("WREN");
		case ETDShipId::Lancer: return TEXT("KATO");
		default: return TEXT("MOSS");
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
	// A cinematic (the intro) hides every HUD: the real widget is hidden on purpose then, so this fallback must not
	// read that as "no real HUD" and draw its full readout over the letterboxed shot.
	if (const UTDUIHostSubsystem* UIHost = UTDUIHostSubsystem::Get(this); UIHost && UIHost->IsCinematicHidden())
	{
		return;
	}
	// A real HUD widget is up: draw only what it does not show yet (R-017), so no gameplay information is
	// lost while presentation catches up. Each surface switches off the moment the widget claims it.
	const bool bSupplement = IsRealHUDActive(GetWorld());
	auto Covered = [this, bSupplement](ETDHUDSurface Surface)
	{
		return bSupplement && UTDUIHostSubsystem::IsSurfaceCoveredByNativeHUD(this, Surface);
	};
	if (bSupplement)
	{
		if (!Covered(ETDHUDSurface::SquadMarkers))
		{
			DrawWingmen();
		}
		if (!Covered(ETDHUDSurface::DamageIndicators))
		{
			DrawDamageIndicators();
		}
		// Nick's widget already draws the interact prompt (N-018), so that one is never duplicated here.
		DrawSupplementText(false, !Covered(ETDHUDSurface::HyperdriveBlockText),
			!Covered(ETDHUDSurface::Altitude));
	}
	else
	{
		DrawFull();
	}

	if (const UWorld* World = GetWorld(); World && World->GetRealTimeSeconds() < BarkUntil && !BarkLine.IsEmpty()
		&& !Covered(ETDHUDSurface::Subtitles))
	{
		float W = 0.f, H = 0.f;
		GetTextSize(BarkLine, W, H, nullptr, 1.2f);
		DrawText(BarkLine, FLinearColor(0.85f, 0.95f, 1.f), (Canvas->SizeX - W) * 0.5f, Canvas->SizeY * 0.86f, nullptr, 1.2f);
	}

	if (const UWorld* World = GetWorld(); World && World->IsPaused() && !Covered(ETDHUDSurface::Pause))
	{
		const FString Paused(TEXT("PAUSED - ESC TO RESUME   ·   O / Y  SETTINGS"));
		float W = 0.f, H = 0.f;
		GetTextSize(Paused, W, H, nullptr, 2.2f);
		DrawText(Paused, FLinearColor::White, (Canvas->SizeX - W) * 0.5f, Canvas->SizeY * 0.42f, nullptr, 2.2f);
	}
}

// The three text read-outs a real HUD may not have yet, placed centrally, above the ship cards
// of Nick's HUD (which start at about 77% of the height and draw over this canvas).
void ATDDebugHUD::DrawSupplementText(bool bPrompt, bool bHyperdriveBlock, bool bAltitude)
{
	const ITDHUDData* Data = Cast<ITDHUDData>(FindHUDDataActor());
	if (!Data || Data->GetSquadHUD().Num() == 0)
	{
		return;
	}
	auto Centred = [this](const FString& Text, float Height01, float Scale, const FLinearColor& Colour)
	{
		float W = 0.f, H = 0.f;
		GetTextSize(Text, W, H, nullptr, Scale);
		DrawText(Text, Colour, (Canvas->SizeX - W) * 0.5f, Canvas->SizeY * Height01, nullptr, Scale);
	};
	if (bPrompt)
	{
		const FString Prompt = Data->GetInteractPrompt().ToString();
		if (!Prompt.IsEmpty())
		{
			Centred(Prompt, 0.665f, 1.5f, FLinearColor::White);
		}
	}
	// Short-lived combat states no HUD shows yet: the Switch Strike window and a focus order.
	{
		FString State;
		const float Strike = Data->GetSwitchStrike01();
		if (Strike > 0.f)
		{
			State = FString::Printf(TEXT("SWITCH STRIKE +25%% %s"), *Bar(Strike, 8));
		}
		const float Focus = Data->GetFocusRemaining01();
		if (Focus > 0.f)
		{
			State += FString::Printf(TEXT("%sFOCUS FIRE %s"), State.IsEmpty() ? TEXT("") : TEXT("     "), *Bar(Focus, 8));
		}
		if (!State.IsEmpty())
		{
			Centred(State, 0.17f, 1.3f, FLinearColor(1.f, 0.85f, 0.3f));
		}
	}
	FString Line;
	if (bHyperdriveBlock)
	{
		Line = Data->GetHyperdriveBlockText().ToString();
	}
	if (bAltitude && Data->GetAltitude() >= 0.f)
	{
		Line += FString::Printf(TEXT("%sALT %.0f m"), Line.IsEmpty() ? TEXT("") : TEXT("   "), Data->GetAltitude());
	}
	if (!Line.IsEmpty())
	{
		Centred(Line, 0.705f, 1.2f, ColorDim);
	}
}

void ATDDebugHUD::DrawFull()
{
	// Start below the tallest letterbox bar (12% of the height) so the cut presentation never covers these rows.
	float Y = FMath::Max(24.f, Canvas->SizeY * 0.12f);
	DrawSquad(Y);
	DrawWave();
	DrawTargets();
	DrawWingmen();
	DrawDamageIndicators();
	DrawObjective();
	DrawCrosshair();
	DrawFlight();
	DrawSupplementText(false, false, false); // the Switch Strike / focus line only
}

void ATDDebugHUD::BeginPlay()
{
	Super::BeginPlay();
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnHitConfirmed.AddUniqueDynamic(this, &ATDDebugHUD::HandleHitConfirmed);
		Bus->OnBarkRequested.AddUniqueDynamic(this, &ATDDebugHUD::HandleBarkRequested);
	}
}

void ATDDebugHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The bus lives on the game instance and outlives this HUD.
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnHitConfirmed.RemoveAll(this);
		Bus->OnBarkRequested.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

// A bark id such as "Bark_SquadEngageHighLeft" becomes "KATO: engage high left": readable enough to
// judge when and how often the wing speaks, before any voice exists.
void ATDDebugHUD::HandleBarkRequested(FName BarkId, FName Speaker)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FString Text = BarkId.ToString();
	Text.RemoveFromStart(TEXT("Bark_"));
	Text.RemoveFromStart(TEXT("Squad"));
	FString Spaced;
	for (int32 i = 0; i < Text.Len(); ++i)
	{
		if (i > 0 && FChar::IsUpper(Text[i]) && !FChar::IsUpper(Text[i - 1]))
		{
			Spaced += TEXT(" ");
		}
		Spaced.AppendChar(FChar::ToLower(Text[i]));
	}
	if (Spaced.RemoveFromStart(TEXT("engage ")))
	{
		Spaced = TEXT("engaging, ") + Spaced;
	}
	BarkLine = FString::Printf(TEXT("%s: %s"), *Speaker.ToString().ToUpper(), *Spaced);
	BarkUntil = World->GetRealTimeSeconds() + 2.5;
}

void ATDDebugHUD::HandleHitConfirmed(AActor* Target, bool bKill)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// A kill marker is never shortened by a later plain hit.
	const double Until = World->GetRealTimeSeconds() + (bKill ? 0.35 : 0.12);
	if (bKill || World->GetRealTimeSeconds() >= HitMarkerUntil || !bLastHitWasKill)
	{
		bLastHitWasKill = bKill;
	}
	HitMarkerUntil = FMath::Max(HitMarkerUntil, Until);
}

// Where fire is coming from: a red arc segment around the crosshair for each recent hit, on the side
// the hit came from (up on screen = ahead, down = behind), fading over about 1.5 s.
void ATDDebugHUD::DrawDamageIndicators()
{
	const ITDHUDData* Data = Cast<ITDHUDData>(FindHUDDataActor());
	const APlayerController* PC = GetOwningPlayerController();
	if (!Data || !PC || !PC->PlayerCameraManager)
	{
		return;
	}
	const FRotator View = PC->PlayerCameraManager->GetCameraRotation();
	const FVector Forward = View.Vector();
	const FVector Right = FRotationMatrix(View).GetUnitAxis(EAxis::Y);
	const FVector Up = FRotationMatrix(View).GetUnitAxis(EAxis::Z);
	const FVector2D C(Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f);
	const float Radius = FMath::Min(Canvas->SizeX, Canvas->SizeY) * 0.16f;

	for (const FTDDamageIndicator& Hit : Data->GetDamageIndicators())
	{
		const float Life = 1.f - FMath::Clamp(Hit.AgeSeconds / 1.5f, 0.f, 1.f);
		if (Life <= 0.f)
		{
			continue;
		}
		// Flatten the direction onto the screen: sideways and up / down keep their sense, and a hit
		// from straight ahead or straight behind lands at the top or bottom of the ring.
		const float X = FVector::DotProduct(Hit.WorldDirection, Right);
		const float Y = FVector::DotProduct(Hit.WorldDirection, Up);
		const float Z = FVector::DotProduct(Hit.WorldDirection, Forward);
		FVector2D Dir(X, -Y);
		if (Dir.SizeSquared() < 0.01f)
		{
			Dir = FVector2D(0.f, Z >= 0.f ? -1.f : 1.f);
		}
		Dir.Normalize();
		const FVector2D Side(-Dir.Y, Dir.X);
		const FLinearColor Color = Hit.bShieldOnly ? FLinearColor(1.f, 0.95f, 0.8f, Life) : FLinearColor(1.f, 0.15f, 0.1f, Life); // shield hits near-white: never a squad colour
		const float Half = 26.f + 34.f * FMath::Clamp(Hit.Strength01 * 4.f, 0.f, 1.f);
		const FVector2D Mid = C + Dir * Radius;
		// A shallow chevron pointing outward, wider for a harder hit.
		DrawLine2D(Mid - Side * Half - Dir * 10.f, Mid, Color, 7.f);
		DrawLine2D(Mid + Side * Half - Dir * 10.f, Mid, Color, 7.f);
	}
}

// The wing: each AI-flown squad ship in its own colour. In view: the pilot's name beside it. Out of
// view: an edge arrow with the name, so the player always knows where the wing is. Brighter while
// that wingman is engaging a target.
void ATDDebugHUD::DrawWingmen()
{
	const ITDHUDData* Data = Cast<ITDHUDData>(FindHUDDataActor());
	const APawn* Pawn = GetOwningPawn();
	if (!Data || !Pawn)
	{
		return;
	}
	const FVector2D ScreenCenter(Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f);
	for (const FTDSquadMarker& Marker : Data->GetSquadMarkers())
	{
		const AActor* Actor = Marker.Actor.Get();
		if (Marker.bPlayerControlled || !Marker.bAlive || !IsValid(Actor) || Actor == Pawn)
		{
			continue;
		}
		FLinearColor Color = ShipColor(Marker.Ship);
		Color.A = Marker.bEngaging ? 1.f : 0.6f;
		const FString Label = Marker.bEngaging ? FString::Printf(TEXT("%s !"), PilotName(Marker.Ship)) : FString(PilotName(Marker.Ship));

		const FVector Screen = Project(Actor->GetActorLocation());
		const bool bOnScreen = Screen.Z > 0.f && Screen.X >= 0.f && Screen.X <= Canvas->SizeX && Screen.Y >= 0.f && Screen.Y <= Canvas->SizeY;
		if (bOnScreen)
		{
			DrawText(Label, Color, Screen.X + 14.f, Screen.Y + 8.f, nullptr, 0.9f);
			continue;
		}
		FVector2D Dir(Screen.X - ScreenCenter.X, Screen.Y - ScreenCenter.Y);
		if (Screen.Z <= 0.f)
		{
			Dir *= -1.f;   // behind the camera the projection is mirrored
		}
		if (!Dir.Normalize())
		{
			Dir = FVector2D(0.f, 1.f);
		}
		// Inside the ring the hostile arrows use (0.45), so the two never sit on top of each other.
		const float Radius = FMath::Min(Canvas->SizeX, Canvas->SizeY) * 0.34f;
		const FVector2D Tip = ScreenCenter + Dir * Radius;
		const FVector2D Side(-Dir.Y, Dir.X);
		const float Thickness = Marker.bEngaging ? 3.f : 2.f;
		DrawLine2D(Tip, Tip - Dir * 16.f + Side * 8.f, Color, Thickness);
		DrawLine2D(Tip, Tip - Dir * 16.f - Side * 8.f, Color, Thickness);
		float W = 0.f, H = 0.f;
		GetTextSize(Label, W, H, nullptr, 0.85f);
		DrawText(Label, Color, Tip.X - Dir.X * 30.f - W * 0.5f, Tip.Y - Dir.Y * 30.f - H * 0.5f, nullptr, 0.85f);
	}
}

void ATDDebugHUD::DrawSquad(float& Y)
{
	const ITDHUDData* Data = Cast<ITDHUDData>(FindHUDDataActor());
	if (!Data)
	{
		// Nothing to show (menu, loading, victory screen): draw nothing. A note here ended up behind the menu
		// in the packaged game.
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
				Line += FString::Printf(TEXT("  BOOST %s  HEAT %s  AMMO %d  DODGE %s"), *Bar(Card.Boost01, 6), *Bar(Card.Heat01, 6), Card.SecondaryAmmo, *Bar(Card.Dodge01, 3));
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
	// The data source's own text wins (it also covers the warning band before the edge); the plain
	// out-of-bounds flag is the fallback for a source that does not provide one.
	FString BoundaryText = Data->GetBoundaryWarningText().ToString();
	if (BoundaryText.IsEmpty() && Data->IsOutOfBounds())
	{
		BoundaryText = TEXT("LEAVING THE COMBAT ZONE - TURN BACK");
	}
	if (!BoundaryText.IsEmpty())
	{
		const FString Warn = BoundaryText;
		float W = 0.f, H = 0.f;
		GetTextSize(Warn, W, H, nullptr, 1.6f);
		DrawText(Warn, ColorEnemy, (Canvas->SizeX - W) * 0.5f, Canvas->SizeY * 0.22f, nullptr, 1.6f);
	}

	// Flying: the interact prompt (landing: "Hold F to land" / "SLOW DOWN TO LAND"), centred under the
	// reticle. On foot it is part of the health row instead.
	if (Data->GetSquadHUD().Num() > 0)
	{
		const FString Prompt = Data->GetInteractPrompt().ToString();
		if (!Prompt.IsEmpty())
		{
			float W = 0.f, H = 0.f;
			GetTextSize(Prompt, W, H, nullptr, 1.5f);
			// Low enough to clear the player's own ship in the chase view.
			DrawText(Prompt, FLinearColor::White, (Canvas->SizeX - W) * 0.5f, Canvas->SizeY * 0.80f, nullptr, 1.5f);
		}
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

// The objective line (top centre) and where to go: a diamond with the distance on each objective
// marker in view, an edge arrow toward each one out of view. Works in space and on foot.
void ATDDebugHUD::DrawObjective()
{
	const ITDHUDData* Data = Cast<ITDHUDData>(FindHUDDataActor());
	const APawn* Pawn = GetOwningPawn();
	if (!Data || !Pawn)
	{
		return;
	}
	const FLinearColor ColorObjective(1.f, 0.78f, 0.2f);

	const FString Objective = Data->GetObjectiveText().ToString();
	if (!Objective.IsEmpty())
	{
		float W = 0.f, H = 0.f;
		GetTextSize(Objective, W, H, nullptr, 1.3f);
		// Below the three squad rows, so the two never overlap.
		DrawText(Objective, ColorObjective, (Canvas->SizeX - W) * 0.5f, FMath::Max(22.f, Canvas->SizeY * 0.12f) + 76.f, nullptr, 1.3f);
	}

	const FVector Eye = Pawn->GetActorLocation();
	const FVector2D ScreenCenter(Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f);
	for (const AActor* Marker : Data->GetObjectiveMarkers())
	{
		if (!IsValid(Marker) || Marker == Pawn)
		{
			continue;
		}
		const FVector Screen = Project(Marker->GetActorLocation());
		const bool bOnScreen = Screen.Z > 0.f && Screen.X >= 0.f && Screen.X <= Canvas->SizeX && Screen.Y >= 0.f && Screen.Y <= Canvas->SizeY;
		const float DistanceM = static_cast<float>(FVector::Dist(Eye, Marker->GetActorLocation())) / 100.f;
		if (bOnScreen)
		{
			const FVector2D C(Screen.X, Screen.Y);
			const float R = 11.f;
			DrawLine2D(C + FVector2D(0.f, -R), C + FVector2D(R, 0.f), ColorObjective, 2.f);
			DrawLine2D(C + FVector2D(R, 0.f), C + FVector2D(0.f, R), ColorObjective, 2.f);
			DrawLine2D(C + FVector2D(0.f, R), C + FVector2D(-R, 0.f), ColorObjective, 2.f);
			DrawLine2D(C + FVector2D(-R, 0.f), C + FVector2D(0.f, -R), ColorObjective, 2.f);
			DrawText(FString::Printf(TEXT("%.0f m"), DistanceM), ColorObjective, C.X + R + 5.f, C.Y - 8.f, nullptr, 1.f);
		}
		else
		{
			// Behind the camera the projection is mirrored: flip it so the arrow points the way to turn.
			FVector2D Dir(Screen.X - ScreenCenter.X, Screen.Y - ScreenCenter.Y);
			if (Screen.Z <= 0.f)
			{
				Dir *= -1.f;
			}
			if (!Dir.Normalize())
			{
				Dir = FVector2D(0.f, 1.f);
			}
			const float Radius = FMath::Min(Canvas->SizeX, Canvas->SizeY) * 0.40f;
			const FVector2D Tip = ScreenCenter + Dir * Radius;
			const FVector2D Side(-Dir.Y, Dir.X);
			DrawLine2D(Tip, Tip - Dir * 22.f + Side * 11.f, ColorObjective, 3.f);
			DrawLine2D(Tip, Tip - Dir * 22.f - Side * 11.f, ColorObjective, 3.f);
			DrawText(FString::Printf(TEXT("%.0f m"), DistanceM), ColorObjective, Tip.X - Dir.X * 40.f - 14.f, Tip.Y - Dir.Y * 40.f - 8.f, nullptr, 0.9f);
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

	// Hit marker: four short diagonals around the centre; red and thicker for a kill.
	const UWorld* World = GetWorld();
	if (World && World->GetRealTimeSeconds() < HitMarkerUntil)
	{
		const FLinearColor Mark = bLastHitWasKill ? FLinearColor(1.f, 0.2f, 0.2f) : FLinearColor::White;
		const float Thick = bLastHitWasKill ? 3.f : 2.f;
		const float Gap = 8.f;
		const float Len = 10.f;
		for (const FVector2D& D : { FVector2D(1.f, 1.f), FVector2D(1.f, -1.f), FVector2D(-1.f, 1.f), FVector2D(-1.f, -1.f) })
		{
			const FVector2D N = D.GetSafeNormal();
			DrawLine2D(C + N * Gap, C + N * (Gap + Len), Mark, Thick);
		}
	}
}

void ATDDebugHUD::DrawFlight()
{
	const APawn* Pawn = GetOwningPawn();
	const ITDHUDData* Data = Cast<ITDHUDData>(FindHUDDataActor());
	// Flight read-outs only while flying: on foot the health row is the whole HUD.
	if (!Pawn || !Data || Data->GetSquadHUD().Num() == 0)
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
	// Also in Shipping: until Nick's HUD is on by default this is the only HUD a player gets (seen Nov 12:
	// the Shipping package had no HUD at all). It hides itself wherever a real HUD widget is up.
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
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
