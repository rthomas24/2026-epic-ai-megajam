// Triad Drift - Combat (Teammate 2). Dev tool: freeze and screenshot the moment an enemy dies.

#include "TriadDrift/Combat/TDDevDeathCapture.h"

#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"
#include "UObject/StrongObjectPtr.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDDevDeathCapture, Log, All);

namespace TDDevDeathCaptureImpl
{
	TStrongObjectPtr<UTDDevDeathCapture>& Active()
	{
		static TStrongObjectPtr<UTDDevDeathCapture> Ptr;
		return Ptr;
	}
}

UTDDevDeathCapture* UTDDevDeathCapture::Arm(UWorld* InWorld, const FString& ClassFilter, int32 Count, float InHoldSeconds, bool bRequestScreenshots,
	float Dilation)
{
	Disarm();
	UTDEventBus* EventBus = InWorld ? UTDEventBus::Get(InWorld) : nullptr;
	if (!EventBus)
	{
		UE_LOG(LogTDDevDeathCapture, Warning, TEXT("TD.Dev.CaptureOnDeath: no event bus in this world."));
		return nullptr;
	}
	UTDDevDeathCapture* Capture = NewObject<UTDDevDeathCapture>();
	Capture->Filter = ClassFilter;
	Capture->Remaining = FMath::Max(Count, 1);
	Capture->HoldSeconds = FMath::Clamp(InHoldSeconds, 0.f, 30.f);
	Capture->bScreenshots = bRequestScreenshots;
	Capture->HoldDilation = FMath::Clamp(Dilation, 0.001f, 1.f);
	Capture->World = InWorld;
	Capture->Bus = EventBus;
	EventBus->OnActorKilled.AddUniqueDynamic(Capture, &UTDDevDeathCapture::HandleActorKilled);
	TDDevDeathCaptureImpl::Active().Reset(Capture);
	UE_LOG(LogTDDevDeathCapture, Display, TEXT("TD.Dev.CaptureOnDeath: armed for %d death(s) of '%s', hold %.1f s."), Capture->Remaining, *ClassFilter, Capture->HoldSeconds);
	return Capture;
}

void UTDDevDeathCapture::Disarm()
{
	if (UTDDevDeathCapture* Capture = TDDevDeathCaptureImpl::Active().Get())
	{
		Capture->EndHold();
		Capture->Unbind();
	}
	TDDevDeathCaptureImpl::Active().Reset();
}

UTDDevDeathCapture* UTDDevDeathCapture::GetActive()
{
	return TDDevDeathCaptureImpl::Active().Get();
}

void UTDDevDeathCapture::Unbind()
{
	if (UTDEventBus* EventBus = Bus.Get())
	{
		EventBus->OnActorKilled.RemoveDynamic(this, &UTDDevDeathCapture::HandleActorKilled);
	}
	Bus.Reset();
}

bool UTDDevDeathCapture::Matches(const AActor* Victim) const
{
	if (!Victim)
	{
		return false;
	}
	return Filter.Equals(TEXT("any"), ESearchCase::IgnoreCase) || Victim->GetClass()->GetName().Contains(Filter, ESearchCase::IgnoreCase)
		|| Victim->GetName().Contains(Filter, ESearchCase::IgnoreCase) || DescribeVictim(Victim).Contains(Filter, ESearchCase::IgnoreCase);
}

FString UTDDevDeathCapture::DescribeVictim(const AActor* Victim)
{
	if (const ATDEnemyShip* Ship = Cast<ATDEnemyShip>(Victim))
	{
		return Ship->GetEnemyId().ToString();
	}
	return Victim ? Victim->GetClass()->GetName() : FString();
}

AActor* UTDDevDeathCapture::KillCentral(UWorld* InWorld, const FString& IdFilter)
{
	APlayerController* PC = InWorld ? InWorld->GetFirstPlayerController() : nullptr;
	UTDCombatRegistry* Registry = InWorld ? UTDCombatRegistry::Get(InWorld) : nullptr;
	if (!PC || !Registry)
	{
		return nullptr;
	}
	int32 SizeX = 0, SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);
	const FVector2D Centre(SizeX * 0.5f, SizeY * 0.5f);
	TArray<AActor*> Hostiles;
	Registry->GetHostilesOf(ETDTeam::Player, Hostiles, true);
	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (AActor* Hostile : Hostiles)
	{
		const UTDHealthComponent* Health = UTDStatics::GetHealth(Hostile);
		if (!IsValid(Hostile) || !Health || !Health->IsAlive())
		{
			continue;
		}
		if (!IdFilter.IsEmpty() && !DescribeVictim(Hostile).Contains(IdFilter, ESearchCase::IgnoreCase)
			&& !Hostile->GetClass()->GetName().Contains(IdFilter, ESearchCase::IgnoreCase))
		{
			continue;
		}
		FVector2D Screen;
		if (!PC->ProjectWorldLocationToScreen(Hostile->GetActorLocation(), Screen, true) || Screen.X < 0 || Screen.Y < 0 || Screen.X > SizeX || Screen.Y > SizeY)
		{
			continue;
		}
		const float Score = FVector2D::Distance(Screen, Centre);
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Hostile;
		}
	}
	if (!Best)
	{
		UE_LOG(LogTDDevDeathCapture, Display, TEXT("TD.Dev.KillCentral: no hostile on screen%s."), IdFilter.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" matching '%s'"), *IdFilter));
		return nullptr;
	}
	APawn* Pawn = PC->GetPawn();
	const FVector Dir = Pawn ? (Best->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal() : FVector::ForwardVector;
	UE_LOG(LogTDDevDeathCapture, Display, TEXT("TD.Dev.KillCentral: %s (%s) at %.0f m, %.0f px from centre."), *Best->GetName(), *DescribeVictim(Best),
		Pawn ? FVector::Dist(Pawn->GetActorLocation(), Best->GetActorLocation()) * 0.01f : -1.f, BestScore);
	UTDStatics::ApplyTDDamage(Best, 1000000.f, ETDDamageKind::Energy, Pawn, Pawn, Best->GetActorLocation() - Dir * 100.f, Dir, true);
	return Best;
}

void UTDDevDeathCapture::HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind)
{
	UWorld* W = World.Get();
	if (!W || Remaining <= 0 || bHolding || !Matches(Victim))
	{
		return;
	}
	--Remaining;
	++Caught;
	const APlayerController* PC = W->GetFirstPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const float Metres = Pawn ? FVector::Dist(Pawn->GetActorLocation(), Victim->GetActorLocation()) * 0.01f : -1.f;
	UE_LOG(LogTDDevDeathCapture, Display, TEXT("TD.Dev.CaptureOnDeath: caught %s (%s) at %.0f m, killer %s; holding %.1f s (%d left)."),
		*Victim->GetName(), *DescribeVictim(Victim), Metres, Killer ? *Killer->GetName() : TEXT("none"), HoldSeconds, Remaining);

	if (bScreenshots && W->GetGameViewport())
	{
		FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("TDDeath_%s_%02d"), *DescribeVictim(Victim), Caught), false, false);
	}
	if (HoldSeconds > 0.f)
	{
		bHolding = true;
		AWorldSettings* Settings = W->GetWorldSettings();
		DilationBefore = Settings ? Settings->TimeDilation : 1.f;
		UGameplayStatics::SetGlobalTimeDilation(W, HoldDilation);
		TWeakObjectPtr<UTDDevDeathCapture> WeakThis(this);
		HoldTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
		{
			if (UTDDevDeathCapture* Self = WeakThis.Get())
			{
				Self->EndHold();
			}
			return false; // one shot
		}), HoldSeconds);
	}
	if (Remaining <= 0 && !bHolding)
	{
		Unbind();
	}
}

void UTDDevDeathCapture::EndHold()
{
	if (!bHolding)
	{
		return;
	}
	bHolding = false;
	if (HoldTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HoldTicker);
		HoldTicker.Reset();
	}
	if (UWorld* W = World.Get())
	{
		UGameplayStatics::SetGlobalTimeDilation(W, DilationBefore);
	}
	if (Remaining <= 0)
	{
		Unbind();
		UE_LOG(LogTDDevDeathCapture, Display, TEXT("TD.Dev.CaptureOnDeath: done (%d caught)."), Caught);
	}
}

#if !UE_BUILD_SHIPPING
namespace TDDevDeathCaptureImpl
{
	static FAutoConsoleCommandWithWorldAndArgs CmdCaptureOnDeath(TEXT("TD.Dev.CaptureOnDeath"),
		TEXT("TD.Dev.CaptureOnDeath <class|enemy id|any|off> [count=1] [hold seconds=3] [dilation=0.2]: slow the world and screenshot the next deaths."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() == 0 || Args[0].Equals(TEXT("off"), ESearchCase::IgnoreCase))
			{
				UTDDevDeathCapture::Disarm();
				UE_LOG(LogTDDevDeathCapture, Display, TEXT("TD.Dev.CaptureOnDeath: off."));
				return;
			}
			const int32 Count = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1;
			const float Hold = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 3.f;
			const float Dilation = Args.Num() > 3 ? FCString::Atof(*Args[3]) : UTDDevDeathCapture::DefaultHoldDilation;
			UTDDevDeathCapture::Arm(World, Args[0], Count, Hold, true, Dilation);
		}));

	static FAutoConsoleCommandWithWorldAndArgs CmdKillCentral(TEXT("TD.Dev.KillCentral"),
		TEXT("TD.Dev.KillCentral [enemy id or class]: kill the live hostile closest to the screen centre through the normal damage path."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UTDDevDeathCapture::KillCentral(World, Args.Num() > 0 ? Args[0] : FString());
		}));
}
#endif
