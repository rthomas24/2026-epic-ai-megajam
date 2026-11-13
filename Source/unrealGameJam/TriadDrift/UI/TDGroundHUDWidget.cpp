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
#include "TriadDrift/UI/TDUIPaint.h"

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
}

// ------------------------------------------------------------------------------------------ paint

int32 UTDGroundHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Culling, FSlateWindowElementList& List,
	int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Base = Super::NativePaint(Args, G, Culling, List, LayerId, Style, bParentEnabled);
	const FTDUIPainter P(G, List, Base + 1);
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

	// Objective panel, top-centre (the space HUD's).
	if (!View.Objective.IsEmpty())
	{
		P.Glass(707, 53, 506, 92, Accent);
		for (int32 I = 0; I < 11; ++I)
		{
			P.Line({ 721.f + I * 46, 136 }, { 729.f + I * 44, 143 }, Accent.CopyWithNewOpacity(.09f), .6f);
		}
		P.Label(727, 66, TEXT("OBJECTIVE"), 11, Accent, 150);
		P.Title(727, 90, View.Objective.ToUpper().Left(62), 20, Ink);
		P.Line({ 727, 128 }, { 1193, 128 }, Accent.CopyWithNewOpacity(.2f));
	}

	// Archive transfer meter, under the objective panel: "ARCHIVE TRANSFER 63%", a 360 x 10 bar with the checkpoint tick
	// at 50%; paused, the bar and its line pulse amber with the reason (a crewman at the terminal).
	LastBoxes.Reset();
	if (View.bTransfer)
	{
		const float X = 760.f, Y = 156.f, W = 400.f, H = 62.f;
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
		P.Glass(960 - W * .5f, 676, W, 50, Accent, true);
		P.CenteredText(960, 688, Prompt, 17, Ink, true);
	}

	// Vitals panel, bottom-left (the space HUD's telemetry panel position and style).
	const bool bLow = View.Health <= .25f;
	P.Glass(66, 812, 320, 206, bLow ? Hostile : Accent);
	P.Label(87, 826, TEXT("PILOT  /  VITALS"), 11, bLow ? Hostile : Accent, 90);
	P.Line({ 87, 850 }, { 365, 850 }, Accent.CopyWithNewOpacity(.15f));

	P.HealthIcon(97, 874, bLow ? Hostile : Accent);
	P.Text(116, 862, TEXT("HEALTH"), 14, Ink);
	const FString HealthPct = FString::Printf(TEXT("%d"), FMath::RoundToInt(FMath::Clamp(View.Health, 0.f, 1.f) * 100.f));
	const float PctW = P.MeasureText(HealthPct, 20, true);
	P.Text(365 - (PctW > 0 ? PctW : 30.f), 856, HealthPct, 20, bLow ? Hostile : Ink, true);
	P.Meter(87, 890, 278, SmoothHealth, bLow ? Hostile : Accent, 10);

	P.ShieldIcon(97, 928, Ink.CopyWithNewOpacity(.85f));
	P.Text(116, 916, TEXT("BLOCK"), 14, Ink);
	P.Meter(87, 942, 278, SmoothBlock, View.Block < .2f ? Dim : Ink.CopyWithNewOpacity(.9f), 7);

	P.Text(87, 968, TEXT("DASH"), 14, Ink);
	for (int32 I = 0; I < FMath::Min(View.DashMax, 6); ++I)
	{
		const float X = 160 + I * 38.f;
		if (I < View.Dash)
		{
			P.Gradient(X, 973, 30, 10, Accent * .55f, Accent, false);
			P.Line({ X, 973.5f }, { X + 30, 973.5f }, Ink.CopyWithNewOpacity(.7f), .7f);
		}
		else
		{
			P.Box(X, 973, 30, 10, FLinearColor(.05f, .10f, .14f, .95f));
			P.Frame(X, 973, 30, 10, Accent, .25f);
		}
	}
	if (bLow)
	{
		P.CenteredText(960, 714, TEXT("LOW HEALTH  /  BLOCK AND FALL BACK"), 17, Hostile, true);
	}
	return Base + 3;
}
