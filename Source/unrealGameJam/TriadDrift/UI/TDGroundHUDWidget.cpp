// Triad Drift - on-foot HUD (Teammate 2).

#include "TriadDrift/UI/TDGroundHUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/Planets/TDPlanetProps.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/UI/TDUIPaint.h"
#include "TriadDrift/UI/TDFlightHUDWidget.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"

void UTDGroundHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Sample(0.f);
}

void UTDGroundHUDWidget::NativeDestruct()
{
	UnbindPawn();
	Super::NativeDestruct();
}

void UTDGroundHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Sample(InDeltaTime);
}

// ------------------------------------------------------------------------------------------ data

void UTDGroundHUDWidget::BindPawn(APawn* Pawn)
{
	if (BoundPawn.Get() == Pawn)
	{
		return;
	}
	UnbindPawn();
	BoundPawn = Pawn;
	if (!Pawn)
	{
		return;
	}
	if (UTDHealthComponent* Health = UTDStatics::GetHealth(Pawn))
	{
		Health->OnDamaged.AddUniqueDynamic(this, &UTDGroundHUDWidget::HandleDamaged);
		BoundHealth = Health;
	}
	if (UTDEnergyBladeComponent* Blade = Pawn->FindComponentByClass<UTDEnergyBladeComponent>())
	{
		DeflectHandle = Blade->OnDeflect.AddUObject(this, &UTDGroundHUDWidget::HandleDeflect);
		BoundBlade = Blade;
	}
}

void UTDGroundHUDWidget::UnbindPawn()
{
	if (UTDHealthComponent* Health = BoundHealth.Get())
	{
		Health->OnDamaged.RemoveDynamic(this, &UTDGroundHUDWidget::HandleDamaged);
	}
	if (UTDEnergyBladeComponent* Blade = Cast<UTDEnergyBladeComponent>(BoundBlade.Get()))
	{
		Blade->OnDeflect.Remove(DeflectHandle);
	}
	DeflectHandle.Reset();
	BoundHealth.Reset();
	BoundBlade.Reset();
	BoundPawn.Reset();
}

void UTDGroundHUDWidget::HandleDamaged(UTDHealthComponent* Health, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage)
{
	const APawn* Pawn = BoundPawn.Get();
	if (!Pawn || ShieldDamage + HullDamage <= 0.f)
	{
		return;
	}
	FTDGroundHit Hit;
	if (const AActor* From = Info.Instigator.Get(); From && From != Pawn)
	{
		Hit.From = From->GetActorLocation();
	}
	else if (!Info.HitDirection.IsNearlyZero())
	{
		Hit.From = Pawn->GetActorLocation() - Info.HitDirection.GetSafeNormal() * 1000.f;
	}
	else
	{
		Hit.From = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 1000.f;
	}
	Hits.Insert(Hit, 0);
	Hits.SetNum(FMath::Min(Hits.Num(), 6));
	DamageFlash = 1.f;
}

void UTDGroundHUDWidget::HandleDeflect(AActor* Projectile, bool bPerfect)
{
	View.bPerfect = bPerfect;
	View.Deflect = bPerfect ? PerfectSeconds : DeflectSeconds;
}

void UTDGroundHUDWidget::Sample(float DeltaSeconds)
{
	APlayerController* PC = GetOwningPlayer();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	BindPawn(Pawn);
	const ITDHUDData* Data = Cast<ITDHUDData>(Pawn);
	if (Data)
	{
		View.Health = Data->GetHealth01();
		View.Block = Data->GetBlockStamina01();
		View.Dash = Data->GetDashCharges();
		View.Objective = Data->GetObjectiveText().ToString();
		View.Prompt = Data->GetInteractPrompt().ToString();
	}
	if (!ForceObjective.IsEmpty())
	{
		View.Objective = ForceObjective;
	}
	if (const ATDFirstPersonCharacter* Character = Cast<ATDFirstPersonCharacter>(Pawn))
	{
		View.DashMax = FMath::Max(1, Character->Tuning.DashCharges);
	}
	View.DashMax = FMath::Max(View.DashMax, View.Dash);

	// Meters ease like the space HUD's (MeterResponse 9).
	const float Alpha = DeltaSeconds > 0.f ? 1.f - FMath::Exp(-9.f * DeltaSeconds) : 1.f;
	SmoothHealth = FMath::Lerp(SmoothHealth, View.Health, Alpha);
	SmoothBlock = FMath::Lerp(SmoothBlock, View.Block, Alpha);
	DamageFlash = FMath::Max(0.f, DamageFlash - DeltaSeconds * 3.f);
	View.Deflect = FMath::Max(0.f, View.Deflect - DeltaSeconds);
	for (FTDGroundHit& Hit : Hits)
	{
		Hit.Age += DeltaSeconds;
	}
	Hits.RemoveAll([](const FTDGroundHit& Hit) { return Hit.Age >= HitSeconds; });
	View.Hits = Hits.Num();

	// Nearby live crew get hostile brackets.
	Hostiles.Reset();
	if (const UWorld* World = GetWorld(); World && Pawn)
	{
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			APawn* Other = *It;
			if (Other == Pawn || Other->IsA<ATDSpacecraft>() || !UTDStatics::AreTeamsHostile(ETDTeam::Player, UTDStatics::GetTeam(Other)))
			{
				continue;
			}
			const UTDHealthComponent* Health = UTDStatics::GetHealth(Other);
			if (!Health || !Health->IsAlive() || FVector::DistSquared(Other->GetActorLocation(), Pawn->GetActorLocation()) > FMath::Square(BracketRangeCm))
			{
				continue;
			}
			Hostiles.Add(Other);
			if (Hostiles.Num() >= 12)
			{
				break;
			}
		}
	}
	View.Hostiles = Hostiles.Num();

	// Verdance: the survey archive transfer (T5). Progress never goes down; the fill eases toward it.
	Time += DeltaSeconds;
	float Progress = 0.f;
	bool bPaused = false;
	FText Unused;
	const bool bWasTransfer = View.bTransfer;
	if (ForceTransfer01 >= 0.f)
	{
		View.bTransfer = true;
		Progress = ForceTransfer01;
		bPaused = bForceTransferPaused;
	}
	else
	{
		View.bTransfer = ATDArchiveTransfer::GetArchiveTransfer(this, Progress, bPaused, Unused);
	}
	View.Transfer01 = FMath::Clamp(Progress, 0.f, 1.f);
	View.bTransferPaused = View.bTransfer && bPaused;
	SmoothTransfer = !bWasTransfer ? View.Transfer01 : FMath::Lerp(SmoothTransfer, View.Transfer01, Alpha);

	// The Warden's bar (T5). Mercy tiers start it lower; phase 2 from half hull.
	const bool bWasBoss = View.bBoss;
	float BossHull = 0.f;
	int32 Phase = 1;
	FText BossName;
	if (ForceBoss01 >= 0.f)
	{
		View.bBoss = true;
		BossHull = ForceBoss01;
		Phase = ForceBoss01 <= BossPhase2At ? 2 : 1;
		BossName = FText::FromString(TEXT("WARDEN"));
	}
	else
	{
		View.bBoss = ATDCrewCharacter::GetActiveBoss(this, BossHull, Phase, BossName);
	}
	View.Boss01 = FMath::Clamp(BossHull, 0.f, 1.f);
	View.BossPhase = Phase;
	View.BossName = BossName.ToString().ToUpper();
	if (!bWasBoss)
	{
		SmoothBoss = GhostBoss = LastBoss = View.Boss01;
		GhostHold = 0.f;
	}
	SmoothBoss = FMath::Lerp(SmoothBoss, View.Boss01, Alpha);
	if (View.Boss01 >= GhostBoss)
	{
		GhostBoss = View.Boss01; // healed or untouched: no ghost
		GhostHold = 0.f;
	}
	else
	{
		if (View.Boss01 < LastBoss - .001f)
		{
			GhostHold = .6f; // a new hit: hold the ghost where it is a moment longer
		}
		GhostHold -= DeltaSeconds;
		if (GhostHold <= 0.f)
		{
			GhostBoss = FMath::Max(View.Boss01, GhostBoss - DeltaSeconds * .5f);
		}
	}
	LastBoss = View.Boss01;

	// Counted steps and the escape run (T5's props; read, never driven). Re-found twice a second.
	PropScanLeft -= DeltaSeconds;
	if (PropScanLeft <= 0.f)
	{
		PropScanLeft = .5f;
		StepCounter = nullptr;
		EscapeRun = nullptr;
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ATDStepCounter> It(World); It; ++It)
			{
				if (It->Required > 1 && It->IsLinkedStepCurrent()) { StepCounter = *It; break; }
			}
			for (TActorIterator<ATDCollapseRun> It(World); It; ++It)
			{
				if (It->IsRunning()) { EscapeRun = *It; break; }
			}
		}
	}
	{
		int32 Count = 0, Required = 0;
		FString Noun;
		if (ForceStepRequired >= 0)
		{
			Required = ForceStepRequired;
			Count = FMath::Clamp(ForceStepCount, 0, Required);
			Noun = ForceStepNoun;
		}
		else if (const ATDStepCounter* Counter = Cast<ATDStepCounter>(StepCounter.Get()); Counter && Counter->IsLinkedStepCurrent())
		{
			Required = Counter->Required;
			Count = FMath::Clamp(Counter->GetCount(), 0, Required);
			FString Base = Counter->BaseText.ToString().ToUpper();
			int32 Space = INDEX_NONE;
			Base.FindLastChar(TEXT(' '), Space);
			Noun = Space == INDEX_NONE ? Base : Base.Mid(Space + 1);
		}
		if (Required > 0 && StepCountSeen >= 0 && Count > StepCountSeen)
		{
			StepFlashLeft = 1.2f;
		}
		StepCountSeen = Required > 0 ? Count : -1;
		StepFlashLeft = FMath::Max(0.f, StepFlashLeft - DeltaSeconds);
		View.StepCount = Count;
		View.StepRequired = Required;
		View.StepNoun = Noun;
		View.StepFlash01 = StepFlashLeft / 1.2f;
	}
	if (ForceEscapeLeft >= 0.f)
	{
		View.bEscape = true;
		View.EscapeLeft = ForceEscapeLeft;
		View.EscapeTotal = FMath::Max(ForceEscapeTotal, 1.f);
	}
	else if (const ATDCollapseRun* Run = Cast<ATDCollapseRun>(EscapeRun.Get()); Run && Run->IsRunning())
	{
		View.bEscape = true;
		View.EscapeLeft = FMath::Max(0.f, Run->GetTimeLeft());
		View.EscapeTotal = FMath::Max(Run->Seconds, 1.f);
	}
	else
	{
		View.bEscape = false;
	}
	// The way out while the clock runs: the pawn's first objective marker (T5 points it at the exit).
	// The objective waypoint (ENG-36, the new style draws it natively): the first live marker the objective supplies, which
	// is T5's route point while the straight line would cross magma (GetActiveWaypoints), "EXIT" during the escape. Re-read
	// every frame: a broken conduit drops out at once.
	View.bExitMarker = false;
	if (PC && Pawn)
	{
		TOptional<FVector> At = ForceExitAt;
		if (!At.IsSet() && ForceMarkerActor.IsValid())
		{
			At = ForceMarkerActor->GetActorLocation();
		}
		if (!At.IsSet() && Data)
		{
			for (AActor* Marker : Data->GetObjectiveMarkers())
			{
				if (IsValid(Marker)) { At = Marker->GetActorLocation(); break; }
			}
		}
		if (At.IsSet())
		{
			View.bExitMarker = true;
			View.ExitMetres = FVector::Dist(Pawn->GetActorLocation(), At.GetValue()) * .01f;
			// Off screen (or behind): the direction from the view, on the frame edge.
			FVector ViewLoc;
			FRotator ViewRot;
			PC->GetPlayerViewPoint(ViewLoc, ViewRot);
			const FVector Local = ViewRot.UnrotateVector(At.GetValue() - ViewLoc);
			ExitDirection = FVector2D(Local.Y, -Local.Z).GetSafeNormal();
			if (ExitDirection.IsNearlyZero()) { ExitDirection = FVector2D(0.f, -1.f); }
			View.bExitOnScreen = Local.X > 0.f;
			ExitWorld = At.GetValue();
		}
	}
	// ENG-26: the prepared-but-blocked approach after Cinder's anchor (wording a placeholder for Nicholas).
	if (ForceApproachPending >= 0)
	{
		View.bApproachPending = ForceApproachPending > 0;
	}
	else
	{
		bool bRevealed = false;
		if (const ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
		{
			bRevealed = Flow->GetProgress().bDominionRevealed;
		}
		View.bApproachPending = ATDPlanetCourse::IsFieldAnchorDisabled(this) && !bRevealed;
	}
	// The sensor marker flicker (host): "UNKNOWN", "VERIFIED" for a moment once, then "UNKNOWN".
	View.bSensor = false;
	if (const UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(this))
	{
		FVector SensorAt;
		float SensorAge = 0.f;
		if (Host->GetSensorFlicker(SensorAt, SensorAge))
		{
			View.bSensor = true;
			const float Since = SensorAge - UTDUIHostSubsystem::SensorVerifiedAt;
			View.bSensorVerified = (Since >= 0.f && Since < .45f) || (Since >= .55f && Since < .67f);
			SensorWorld = SensorAt;
		}
	}
}

// ------------------------------------------------------------------------------------------ paint

namespace TDGroundStyle
{
	const FSlateBrush* Rounded(float RadiusPx)
	{
		static TMap<int32, TSharedPtr<FSlateRoundedBoxBrush>> Cache;
		const int32 R = FMath::Clamp(FMath::RoundToInt(RadiusPx), 1, 255);
		TSharedPtr<FSlateRoundedBoxBrush>& Brush = Cache.FindOrAdd(R);
		if (!Brush.IsValid())
		{
			Brush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor::White, float(R));
		}
		return Brush.Get();
	}

	/**
	 * ENG-36 (the 03:00 audit): the on-foot HUD in the flight HUD's language. Every panel is the flight HUD's dark rounded
	 * plate with a short accent edge on top (emphasis: a brighter edge and a thin outline), not the older space HUD's glass.
	 */
	struct FPaint : FTDUIPainter
	{
		using FTDUIPainter::FTDUIPainter;
		void Plate(float X, float Y, float W, float H, float Radius, FLinearColor C) const
		{
			FSlateDrawElement::MakeBox(List, Layer, Geo({ X, Y }, { W, H }), Rounded(Radius * Scale), ESlateDrawEffect::None, Faded(C));
		}
		void Glass(float X, float Y, float W, float H, FLinearColor C, bool bEmphasis = false) const
		{
			Plate(X, Y, W, H, 12.f, FTDFlightHUDTokens::Plate().CopyWithNewOpacity(.82f));
			Line({ X + 14.f, Y + 1.5f }, { X + FMath::Min(W - 14.f, 14.f + FMath::Max(40.f, W * .28f)), Y + 1.5f }, C.CopyWithNewOpacity(bEmphasis ? .95f : .7f), 2.f);
			if (bEmphasis)
			{
				Frame(X + 2.f, Y + 2.f, W - 4.f, H - 4.f, C, .22f);
			}
		}
	};
}

int32 UTDGroundHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Culling, FSlateWindowElementList& List,
	int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Base = Super::NativePaint(Args, G, Culling, List, LayerId, Style, bParentEnabled);
	const TDGroundStyle::FPaint P(G, List, Base + 1);
	if (P.Scale <= 0.f)
	{
		return Base;
	}
	const FLinearColor Accent = FTDUIPalette::Accent();
	const FLinearColor Ink = FTDUIPalette::Ink();
	const FLinearColor Dim = FTDUIPalette::Muted();
	const FLinearColor Hostile = FTDUIPalette::Hostile();
	const FVector2D Center(960, 540);
	APlayerController* PC = GetOwningPlayer();

	// Taking damage: a brief red edge, never over the centre of the frame.
	if (DamageFlash > 0.f)
	{
		const float A = .22f * DamageFlash;
		P.Gradient(0, 0, 1920, 90, Hostile.CopyWithNewOpacity(A), Hostile.CopyWithNewOpacity(0), false);
		P.Gradient(0, 990, 1920, 90, Hostile.CopyWithNewOpacity(0), Hostile.CopyWithNewOpacity(A), false);
		P.Gradient(0, 0, 120, 1080, Hostile.CopyWithNewOpacity(A), Hostile.CopyWithNewOpacity(0));
		P.Gradient(1800, 0, 120, 1080, Hostile.CopyWithNewOpacity(0), Hostile.CopyWithNewOpacity(A));
	}

	// Objective panel, top-centre (the space HUD's). It grows to fit its line (506 to 1000 px, centred) and wraps to two
	// lines beyond that (T7's playtest: "HEAD TO A SIGNAL · THE RESEARCH SITE OR THE RELAY" ran past a fixed 506 px panel,
	// and anything over 62 characters was cut).
	LastBoxes.Reset();
	float PanelBottom = 145.f;
	if (!View.Objective.IsEmpty())
	{
		const FString Line = View.Objective.ToUpper();
		auto Width = [&P](const FString& S) { const float M = P.MeasureTitle(S, 18); return M > 0.f ? M : S.Len() * 10.5f; };
		FString Line1 = Line, Line2;
		float TextW = Width(Line);
		if (TextW + 56.f > ObjectiveMaxWidth)
		{
			// Split at the space nearest the middle.
			int32 Split = INDEX_NONE;
			for (int32 D = 0; D < Line.Len() / 2 && Split == INDEX_NONE; ++D)
			{
				for (const int32 I : { Line.Len() / 2 + D, Line.Len() / 2 - D })
				{
					if (Line.IsValidIndex(I) && Line[I] == TEXT(' '))
					{
						Split = I;
						break;
					}
				}
			}
			if (Split != INDEX_NONE)
			{
				Line1 = Line.Left(Split);
				Line2 = Line.Mid(Split + 1);
				TextW = FMath::Max(Width(Line1), Width(Line2));
			}
		}
		// The flight HUD's objective pill: a white diamond, the line in caps, centred at the top.
		const float W = FMath::Clamp(TextW + 56.f, ObjectiveMinWidth, ObjectiveMaxWidth);
		const float H = Line2.IsEmpty() ? ObjectiveHeight : ObjectiveHeight2;
		const float X = 960.f - W * .5f, Y = 54.f;
		P.Plate(X, Y, W, H, 14.f, FTDFlightHUDTokens::Plate().CopyWithNewOpacity(.75f));
		const FVector2D Dia(X + 20.f, Y + 20.f);
		P.Line({ Dia.X, Dia.Y - 6.f }, { Dia.X + 6.f, Dia.Y }, Ink, 2.f);
		P.Line({ Dia.X + 6.f, Dia.Y }, { Dia.X, Dia.Y + 6.f }, Ink, 2.f);
		P.Line({ Dia.X, Dia.Y + 6.f }, { Dia.X - 6.f, Dia.Y }, Ink, 2.f);
		P.Line({ Dia.X - 6.f, Dia.Y }, { Dia.X, Dia.Y - 6.f }, Ink, 2.f);
		P.Title(X + 36.f, Y + 8.f, Line1, 18, Ink);
		if (!Line2.IsEmpty())
		{
			P.Title(X + 36.f, Y + 32.f, Line2, 18, Ink);
		}
		LastBoxes.Emplace(TEXT("objective"), FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H)));
		PanelBottom = Y + H;
	}

	// Archive transfer meter, under the objective panel: "ARCHIVE TRANSFER 63%", a 360 x 10 bar with the checkpoint tick
	// at 50%; paused, the bar and its line pulse amber with the reason (a crewman at the terminal).
	const float BelowPanel = PanelBottom + 11.f;
	if (View.bTransfer)
	{
		const float X = 760.f, Y = BelowPanel, W = 400.f, H = 62.f;
		const FLinearColor Amber = FTDUIPalette::Bastion();
		const float Pulse = .55f + .45f * FMath::Abs(FMath::Sin(Time * PI * 1.6f));
		const FLinearColor C = View.bTransferPaused ? Amber.CopyWithNewOpacity(Pulse) : Accent;
		P.Glass(X, Y, W, H, View.bTransferPaused ? Amber : Accent);
		LastBoxes.Emplace(TEXT("transfer"), FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H)));
		const int32 Pct = FMath::FloorToInt(View.Transfer01 * 100.f + KINDA_SMALL_NUMBER);
		P.Label(X + 20, Y + 10, TEXT("ARCHIVE TRANSFER"), 11, View.bTransferPaused ? Amber : Accent, 150);
		const FString PctText = FString::Printf(TEXT("%d%%"), Pct);
		const float PctW = P.MeasureText(PctText, 16, true);
		P.Text(X + W - 20 - (PctW > 0 ? PctW : 40.f), Y + 6, PctText, 16, Ink, true);
		P.Meter(X + 20, Y + 32, 360, SmoothTransfer, C, 10);
		P.Line({ X + 20 + 180, Y + 28 }, { X + 20 + 180, Y + 46 }, Ink.CopyWithNewOpacity(View.Transfer01 >= .5f ? .9f : .4f), 1.5f);
		// Paused: the plate and bar pulse amber only; the objective panel above carries the reason (the Lead, Nov 13).
	}

	// The Warden's bar, under the objective panel (and under the transfer meter if both are up): name left, phase right,
	// a 600 px hull bar in the hostile colour with a tick at the phase-2 threshold and a pale ghost of recent damage.
	if (View.bBoss)
	{
		const float W = 640.f, H = 58.f, X = 960.f - W * .5f;
		const float Y = View.bTransfer ? BelowPanel + 62.f + 10.f : BelowPanel;
		const bool bPhase2 = View.BossPhase >= 2;
		const FLinearColor Edge = bPhase2 ? Hostile : Hostile.CopyWithNewOpacity(.7f);
		P.Glass(X, Y, W, H, Edge, bPhase2);
		LastBoxes.Emplace(TEXT("boss"), FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H)));
		P.Label(X + 20.f, Y + 9.f, View.BossName.IsEmpty() ? TEXT("WARDEN") : View.BossName, 13, Hostile, 160);
		const FString PhaseText = bPhase2 ? TEXT("PHASE 2") : TEXT("PHASE 1");
		const float PhaseW = P.MeasureText(PhaseText, 12, true);
		P.Text(X + W - 20.f - (PhaseW > 0 ? PhaseW : 60.f), Y + 8.f, PhaseText, 12, bPhase2 ? Hostile : Dim, true);
		// The bar on layers above the glass: same-layer boxes were batched under the glass's gradients, so a full-hull
		// Warden showed an empty bar live (the test only read the values).
		FTDUIPainter Bar = P;
		Bar.Layer = P.Layer + 2;
		const float BarX = X + 20.f, BarY = Y + 34.f, BarW = W - 40.f;
		Bar.Box(BarX, BarY, BarW, 10.f, FLinearColor(.05f, .06f, .08f, .95f));
		Bar.Layer = P.Layer + 3;
		Bar.Box(BarX, BarY, BarW * FMath::Clamp(GhostBoss, 0.f, 1.f), 10.f, FLinearColor(1.f, .85f, .8f, .55f)); // recent damage
		Bar.Layer = P.Layer + 4;
		Bar.Box(BarX, BarY, BarW * FMath::Clamp(SmoothBoss, 0.f, 1.f), 10.f, Hostile);
		const float TickX = BarX + BarW * BossPhase2At;
		Bar.Line({ TickX, BarY - 4.f }, { TickX, BarY + 14.f }, Ink.CopyWithNewOpacity(bPhase2 ? .35f : .85f), 2.f);
	}

	// Below the panel's stack (transfer meter, Warden bar): the counted step's pips, then the escape clock.
	float Stack = BelowPanel + (View.bTransfer ? 72.f : 0.f) + (View.bBoss ? 68.f : 0.f);
	if (View.StepRequired > 0)
	{
		const float Pip = 14.f, Gap = 10.f;
		const float RowW = View.StepRequired * Pip + (View.StepRequired - 1) * Gap;
		const FString Label = View.StepFlash01 > 0.f && !View.StepNoun.IsEmpty()
			? FString::Printf(TEXT("%s  %d / %d"), *View.StepNoun, View.StepCount, View.StepRequired)
			: (View.StepNoun.IsEmpty() ? FString::Printf(TEXT("%d / %d"), View.StepCount, View.StepRequired)
				: FString::Printf(TEXT("%s  %d / %d"), *View.StepNoun, View.StepCount, View.StepRequired));
		const float LabelW = FMath::Max(P.MeasureText(Label, 13, true), Label.Len() * 8.f);
		const float W = RowW + 22.f + LabelW + 40.f, H = 34.f, X = 960.f - W * .5f, Y = Stack;
		P.Glass(X, Y, W, H, Accent);
		FTDUIPainter Pips = P;
		Pips.Layer = P.Layer + 2;
		for (int32 I = 0; I < View.StepRequired; ++I)
		{
			const FVector2D PC2(X + 20.f + I * (Pip + Gap) + Pip * .5f, Y + H * .5f);
			const bool bDone = I < View.StepCount;
			const bool bNewest = bDone && I == View.StepCount - 1 && View.StepFlash01 > 0.f;
			const FLinearColor Col = bNewest ? FMath::Lerp(Accent, Ink, View.StepFlash01) : bDone ? Accent : Dim.CopyWithNewOpacity(.7f);
			if (bDone)
			{
				Pips.Box(PC2.X - 4.f, PC2.Y - 4.f, 8.f, 8.f, Col);
			}
			Pips.Line({ PC2.X, PC2.Y - 7.f }, { PC2.X + 6.f, PC2.Y - 3.5f }, Col, 2.f);
			Pips.Line({ PC2.X + 6.f, PC2.Y - 3.5f }, { PC2.X + 6.f, PC2.Y + 3.5f }, Col, 2.f);
			Pips.Line({ PC2.X + 6.f, PC2.Y + 3.5f }, { PC2.X, PC2.Y + 7.f }, Col, 2.f);
			Pips.Line({ PC2.X, PC2.Y + 7.f }, { PC2.X - 6.f, PC2.Y + 3.5f }, Col, 2.f);
			Pips.Line({ PC2.X - 6.f, PC2.Y + 3.5f }, { PC2.X - 6.f, PC2.Y - 3.5f }, Col, 2.f);
			Pips.Line({ PC2.X - 6.f, PC2.Y - 3.5f }, { PC2.X, PC2.Y - 7.f }, Col, 2.f);
		}
		Pips.Text(X + 20.f + RowW + 22.f, Y + 8.f, Label, 13, View.StepFlash01 > 0.f ? Ink : Ink.CopyWithNewOpacity(.85f), true);
		LastBoxes.Emplace(TEXT("steps"), FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H)));
		Stack += H + 10.f;
	}
	if (View.bEscape)
	{
		const float W = 300.f, H = 70.f, X = 960.f - W * .5f, Y = Stack;
		const bool bLate = View.EscapeLeft <= 20.f;
		const float Pulse = View.EscapeLeft <= 10.f ? .6f + .4f * FMath::Abs(FMath::Sin(Time * PI * 2.f)) : 1.f;
		const FLinearColor Col = (bLate ? Hostile : FTDUIPalette::Bastion()).CopyWithNewOpacity(Pulse);
		P.Glass(X, Y, W, H, Col, bLate);
		const int32 Secs = FMath::CeilToInt(View.EscapeLeft - KINDA_SMALL_NUMBER);
		P.Label(X + 20.f, Y + 10.f, TEXT("ESCAPE"), 12, Col, 160);
		FTDUIPainter Clock = P;
		Clock.Layer = P.Layer + 2;
		Clock.Title(X + 20.f, Y + 24.f, FString::Printf(TEXT("%d:%02d"), Secs / 60, Secs % 60), 30, Ink);
		Clock.Box(X + 130.f, Y + 40.f, (W - 150.f) * FMath::Clamp(View.EscapeLeft / View.EscapeTotal, 0.f, 1.f), 8.f, Col);
		LastBoxes.Emplace(TEXT("escape"), FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H)));
		Stack += H + 10.f;
	}
	// The approach after the anchor: a dashed amber diamond and the line, under the stack.
	if (View.bApproachPending)
	{
		const FString Line = TEXT("APPROACH PREPARED  ·  PASSAGE STILL BLOCKED");
		const float TW = FMath::Max(P.MeasureText(Line, 14, true), Line.Len() * 8.f);
		const float W = TW + 64.f, H = 34.f, X = 960.f - W * .5f, Y = Stack;
		const FLinearColor Amber = FTDUIPalette::Bastion();
		P.Glass(X, Y, W, H, Amber);
		FTDUIPainter Top = P;
		Top.Layer = P.Layer + 2;
		const FVector2D D(X + 22.f, Y + H * .5f);
		for (int32 S = 0; S < 4; ++S) // dashed: half of each edge
		{
			const FVector2D A = D + FVector2D(FMath::Cos(S * HALF_PI), FMath::Sin(S * HALF_PI)) * 8.f;
			const FVector2D B = D + FVector2D(FMath::Cos((S + 1) * HALF_PI), FMath::Sin((S + 1) * HALF_PI)) * 8.f;
			Top.Line(A, FMath::Lerp(A, B, .5f), Amber, 2.f);
		}
		Top.Text(X + 42.f, Y + 8.f, Line, 14, Ink, true);
		LastBoxes.Emplace(TEXT("approach_pending"), FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H)));
		Stack += H + 10.f;
	}
	// The way out: an amber diamond with "EXIT" and the range on screen; off screen, a chevron on the frame edge.
	if (View.bExitMarker)
	{
		FVector2D Pos = Center;
		bool bOn = false;
		FVector2D UI;
		if (PC && UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, ExitWorld, UI, true))
		{
			Pos = P.ToDesign(UI);
			bOn = View.bExitOnScreen && Pos.X >= 60.f && Pos.X <= 1860.f && Pos.Y >= 60.f && Pos.Y <= 1020.f;
		}
		const FLinearColor Amber = View.bEscape ? FTDUIPalette::Bastion() : Ink;
		if (!bOn)
		{
			const FVector2D D = ExitDirection;
			const float T = FMath::Min(D.X != 0.f ? 860.f / FMath::Abs(D.X) : 1e6f, D.Y != 0.f ? 440.f / FMath::Abs(D.Y) : 1e6f);
			Pos = Center + D * T;
			const FVector2D Side(-D.Y, D.X);
			P.Line(Pos + D * 14.f, Pos - D * 6.f + Side * 12.f, Amber, 3.f);
			P.Line(Pos + D * 14.f, Pos - D * 6.f - Side * 12.f, Amber, 3.f);
		}
		else
		{
			P.Line({ Pos.X, Pos.Y - 12.f }, { Pos.X + 12.f, Pos.Y }, Amber, 2.5f);
			P.Line({ Pos.X + 12.f, Pos.Y }, { Pos.X, Pos.Y + 12.f }, Amber, 2.5f);
			P.Line({ Pos.X, Pos.Y + 12.f }, { Pos.X - 12.f, Pos.Y }, Amber, 2.5f);
			P.Line({ Pos.X - 12.f, Pos.Y }, { Pos.X, Pos.Y - 12.f }, Amber, 2.5f);
		}
		const FVector2D LabelAt = bOn ? Pos + FVector2D(18.f, -16.f) : Pos - ExitDirection * 40.f + FVector2D(-20.f, -10.f);
		if (View.bEscape)
		{
			P.Label(LabelAt.X, LabelAt.Y, TEXT("EXIT"), 12, Amber, 150);
		}
		P.Text(LabelAt.X, LabelAt.Y + 15.f, FString::Printf(TEXT("%.0f m"), View.ExitMetres), 12, Ink.CopyWithNewOpacity(.85f));
		LastBoxes.Emplace(TEXT("exit"), FBox2D(Pos - FVector2D(14.f, 14.f), Pos + FVector2D(14.f, 14.f)));
	}
	// The sensor marker: thin brackets, "SENSOR / UNKNOWN", for a moment "VERIFIED" in the shield blue.
	if (View.bSensor)
	{
		FVector2D UI;
		if (PC && UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, SensorWorld, UI, true))
		{
			const FVector2D Pos = P.ToDesign(UI);
			if (Pos.X >= 40.f && Pos.X <= 1880.f && Pos.Y >= 40.f && Pos.Y <= 1040.f)
			{
				const FLinearColor Blue = FLinearColor::FromSRGBColor(FColor(0x9F, 0xD8, 0xFF));
				const float Stutter = View.bSensorVerified ? .75f + .25f * FMath::Abs(FMath::Sin(Time * 47.f)) : 1.f;
				const FLinearColor Col = (View.bSensorVerified ? Blue : Dim).CopyWithNewOpacity(.9f * Stutter);
				P.Brackets(Pos, 16.f, Col, 6.f);
				P.Label(Pos.X + 24.f, Pos.Y - 16.f, TEXT("TRANSPONDER"), 11, Dim, 120);
				P.Text(Pos.X + 24.f, Pos.Y - 1.f, View.bSensorVerified ? TEXT("VERIFIED") : TEXT("UNKNOWN"), 13, Col, true);
				LastBoxes.Emplace(View.bSensorVerified ? TEXT("sensor_verified") : TEXT("sensor"), FBox2D(Pos - FVector2D(16.f, 16.f), Pos + FVector2D(110.f, 30.f)));
			}
		}
	}

	// Reticle.
	P.Reticle(Center, Accent);

	// Hostile brackets on nearby crew, sized by distance, with range.
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	for (const TWeakObjectPtr<AActor>& Weak : Hostiles)
	{
		const AActor* Other = Weak.Get();
		FVector2D UI;
		if (!Other || !Pawn || !UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Other->GetActorLocation(), UI, true))
		{
			continue;
		}
		const FVector2D Pos = P.ToDesign(UI);
		if (Pos.X < 60 || Pos.X > 1860 || Pos.Y < 60 || Pos.Y > 1020)
		{
			continue;
		}
		const float Dist = FVector::Dist(Other->GetActorLocation(), Pawn->GetActorLocation());
		const float R = FMath::Clamp(60000.f / FMath::Max(Dist, 1.f), 14.f, 46.f);
		P.Brackets(Pos, R, Hostile.CopyWithNewOpacity(.75f), FMath::Min(12.f, R * .45f));
		P.Text(Pos.X + R + 6, Pos.Y - R - 2, FString::Printf(TEXT("%.0f m"), Dist * .01f), 11, Hostile.CopyWithNewOpacity(.85f));
	}

	// Damage direction chevrons around the reticle.
	if (PC && Hits.Num() > 0)
	{
		FVector Eye;
		FRotator Rot;
		PC->GetPlayerViewPoint(Eye, Rot);
		for (const FTDGroundHit& Hit : Hits)
		{
			const FVector Local = FRotator(0.f, Rot.Yaw, 0.f).UnrotateVector(Hit.From - Eye);
			const float Angle = FMath::Atan2(Local.Y, Local.X); // 0 = ahead, +right
			const FVector2D Dir(FMath::Sin(Angle), -FMath::Cos(Angle));
			const FVector2D Side(-Dir.Y, Dir.X);
			// Live at 1080p the first size (18 x 32) read but was easy to miss mid-fight: 1.4x, thicker.
			const FVector2D Tip = Center + Dir * 165.f;
			const float A = FMath::Clamp(1.f - Hit.Age / HitSeconds, 0.f, 1.f);
			const FLinearColor C = Hostile.CopyWithNewOpacity(.95f * A);
			P.Line(Tip, Tip - Dir * 26 + Side * 23, C, 4);
			P.Line(Tip, Tip - Dir * 26 - Side * 23, C, 4);
			P.Line(Tip - Dir * 13, Tip - Dir * 38 + Side * 17, C.CopyWithNewOpacity(.5f * A), 2);
			P.Line(Tip - Dir * 13, Tip - Dir * 38 - Side * 17, C.CopyWithNewOpacity(.5f * A), 2);
		}
	}

	// Deflect: a ring bursting from the reticle; the perfect one with its title (a money shot).
	if (View.Deflect > 0.f)
	{
		const float Life = View.bPerfect ? PerfectSeconds : DeflectSeconds;
		const float A = FMath::Clamp(View.Deflect / Life, 0.f, 1.f);
		const float Grow = 1.f - A;
		P.Arc(Center, 34 + 90 * Grow, 1, (View.bPerfect ? Ink : Accent).CopyWithNewOpacity(.8f * A), View.bPerfect ? 3.f : 1.5f);
		if (View.bPerfect)
		{
			P.Arc(Center, 30 + 50 * Grow, 1, Accent.CopyWithNewOpacity(.7f * A), 1.5f);
			P.CenteredTitle(960, 610, TEXT("PERFECT DEFLECT"), 30, Ink.CopyWithNewOpacity(A));
			P.Line({ 860, 652 }, { 1060, 652 }, Accent.CopyWithNewOpacity(.6f * A), 1.5f);
		}
		else
		{
			P.CenteredText(960, 612, TEXT("DEFLECTED"), 14, Dim.CopyWithNewOpacity(A), true);
		}
	}

	// Interact prompt on a plate under the reticle.
	if (!View.Prompt.IsEmpty())
	{
		const FString Prompt = View.Prompt.ToUpper();
		const float TextW = P.MeasureText(Prompt, 17, true);
		const float W = FMath::Clamp((TextW > 0 ? TextW : 22.f * Prompt.Len() * .5f) + 56.f, 220.f, 900.f);
		P.Plate(960 - W * .5f, 680, W, 40, 12.f, FTDFlightHUDTokens::Plate().CopyWithNewOpacity(.82f));
		P.CenteredText(960, 688, Prompt, 17, Ink, true);
		LastBoxes.Emplace(TEXT("prompt"), FBox2D(FVector2D(960 - W * .5f, 680), FVector2D(960 + W * .5f, 720)));
	}

	// Vitals: a slim chip bottom-left, like the flight HUD's squad chip: health (number and bar), block under it, dash pips.
	const bool bLow = View.Health <= .25f;
	{
		const float X = 96.f, Y = 950.f, W = 270.f, H = 76.f;
		P.Glass(X, Y, W, H, bLow ? Hostile : Accent, bLow);
		FTDUIPainter Bars = P;
		Bars.Layer = P.Layer + 2;
		const FString HealthPct = FString::Printf(TEXT("%d"), FMath::RoundToInt(FMath::Clamp(View.Health, 0.f, 1.f) * 100.f));
		Bars.HealthIcon(X + 22.f, Y + 22.f, bLow ? Hostile : Ink);
		Bars.Text(X + 40.f, Y + 11.f, HealthPct, 18, bLow ? Hostile : Ink, true);
		Bars.Box(X + 92.f, Y + 18.f, W - 112.f, 8.f, FLinearColor(.05f, .06f, .08f, .95f));
		Bars.Box(X + 92.f, Y + 18.f, (W - 112.f) * FMath::Clamp(SmoothHealth, 0.f, 1.f), 8.f, bLow ? Hostile : Accent);
		Bars.ShieldIcon(X + 22.f, Y + 46.f, Ink.CopyWithNewOpacity(.8f));
		Bars.Box(X + 40.f, Y + 44.f, W - 150.f, 5.f, FLinearColor(.05f, .06f, .08f, .95f));
		Bars.Box(X + 40.f, Y + 44.f, (W - 150.f) * FMath::Clamp(SmoothBlock, 0.f, 1.f), 5.f, View.Block < .2f ? Dim : Ink.CopyWithNewOpacity(.9f));
		for (int32 I = 0; I < FMath::Min(View.DashMax, 6); ++I)
		{
			const float DX = X + W - 98.f + I * 26.f;
			const bool bReady = I < View.Dash;
			Bars.Box(DX, Y + 42.f, 20.f, 9.f, bReady ? Accent : FLinearColor(.05f, .06f, .08f, .95f));
			if (!bReady)
			{
				Bars.Frame(DX, Y + 42.f, 20.f, 9.f, Accent, .3f);
			}
		}
		LastBoxes.Emplace(TEXT("vitals"), FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H)));
	}
	if (bLow)
	{
		P.CenteredText(960, 714, TEXT("LOW HEALTH  /  BLOCK AND FALL BACK"), 17, Hostile, true);
	}
	return Base + 3;
}
