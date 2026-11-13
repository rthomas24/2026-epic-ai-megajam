// Triad Drift - planetary descent (ENG-38 stage 1). Owner: Teammate 1.

#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/SLeafWidget.h"

// ------------------------------------------------------------------------------------------------ cover widget

static int32 GTDCoverPaints = 0;

/** A full-screen flat colour. Counts its paints, which is how the probe proves it is still on screen after a load. */
class STDDescentCover : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(STDDescentCover) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&) { SetVisibility(EVisibility::HitTestInvisible); }

	FLinearColor Color = FLinearColor::White;

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Cull, FSlateWindowElementList& Out,
		int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override
	{
		++GTDCoverPaints;
		if (Color.A > 0.f)
		{
			FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(), FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),
				ESlateDrawEffect::None, Color);
		}
		return Layer;
	}
};

// ------------------------------------------------------------------------------------------------ subsystem

void UTDDescentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PreLoadHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UTDDescentSubsystem::HandlePreLoadMap);
	PostLoadHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UTDDescentSubsystem::HandlePostLoadMap);
}

void UTDDescentSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadHandle);
	HideCover();
	StopRoar(0.f);
	Super::Deinitialize();
}

UTDDescentSubsystem* UTDDescentSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UTDDescentSubsystem>() : nullptr;
}

int32 UTDDescentSubsystem::GetCoverPaints() { return GTDCoverPaints; }

void UTDDescentSubsystem::BeginTiming(const TCHAR* What)
{
	TimingStart = LastMark = LastTickTime = FPlatformTime::Seconds();
	LongestFrame = 0.0;
	bWaitFirstFrame = bWaitTerrain = bWaitControl = false;
	PaintsAtPostLoad = PaintsAtFirstFrame = RoarAtFirstFrame = -1;
	ControlTime = FirstFrameTime = TerrainTime = PostLoadTime = OpenLevelTime = -1.0;
	LoadedWorld.Reset(); // wait for this travel's map, not the last one
	UE_LOG(LogTriadDrift, Log, TEXT("TD.Descent.Timing commit (%s) +0.000 s"), What);
}

void UTDDescentSubsystem::Mark(const TCHAR* Step)
{
	if (!IsTiming()) { return; }
	const double Now = FPlatformTime::Seconds();
	UE_LOG(LogTriadDrift, Log, TEXT("TD.Descent.Timing %s +%.3f s (step %.3f s)"), Step, Now - TimingStart, Now - LastMark);
	LastMark = Now;
}

void UTDDescentSubsystem::HandlePreLoadMap(const FString& MapName)
{
	if (!IsTiming()) { return; }
	OpenLevelTime = FPlatformTime::Seconds() - TimingStart;
	Mark(*FString::Printf(TEXT("OpenLevel %s (cover %s, roar %s)"), *FPackageName::GetShortName(MapName), IsCoverShown() ? TEXT("up") : TEXT("off"),
		IsRoarPlaying() ? TEXT("playing") : TEXT("silent")));
}

void UTDDescentSubsystem::HandlePostLoadMap(UWorld* World)
{
	if (!IsTiming() || !World || World->GetGameInstance() != GetGameInstance()) { return; }
	PostLoadTime = FPlatformTime::Seconds() - TimingStart;
	PaintsAtPostLoad = GTDCoverPaints;
	Mark(*FString::Printf(TEXT("post-load %s"), *FPackageName::GetShortName(World->GetOutermost()->GetName())));
	// The transition map is a stop on the way: wait for the destination (a map with a planet terrain or a ship).
	LoadedWorld = World;
	PostLoadFrame = GFrameCounter;
	bWaitFirstFrame = bWaitTerrain = bWaitControl = true;
	TerrainTime = ControlTime = FirstFrameTime = -1.0;
}

void UTDDescentSubsystem::Tick(float DeltaTime)
{
	if (!IsTiming()) { return; }
	const double Now = FPlatformTime::Seconds();
	LongestFrame = FMath::Max(LongestFrame, Now - LastTickTime);
	LastTickTime = Now;
	UWorld* World = LoadedWorld.Get();
	if (Now - TimingStart > 40.0)
	{
		EndTiming(TEXT("40 s without control"));
		return;
	}
	if (!World || !World->HasBegunPlay() || GFrameCounter <= PostLoadFrame) { return; } // the load's own frame is not the first

	if (bWaitFirstFrame)
	{
		bWaitFirstFrame = false;
		FirstFrameTime = Now - TimingStart;
		PaintsAtFirstFrame = GTDCoverPaints;
		RoarAtFirstFrame = IsRoarPlaying() ? 1 : 0;
		Mark(*FString::Printf(TEXT("first frame (cover %s, %d paints since post-load; roar %s)"), IsCoverShown() ? TEXT("up") : TEXT("off"),
			GTDCoverPaints - PaintsAtPostLoad, IsRoarPlaying() ? TEXT("playing") : TEXT("silent")));
	}
	if (bWaitTerrain)
	{
		for (TActorIterator<ATDPlanetTerrain> It(World); It; ++It)
		{
			if (It->GetChunkCount() > 0 && It->GetPendingCount() == 0)
			{
				bWaitTerrain = false;
				TerrainTime = Now - TimingStart;
				Mark(*FString::Printf(TEXT("terrain built (%d chunks, none pending)"), It->GetChunkCount()));
			}
			break;
		}
	}
	if (bWaitControl)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (PC && Cast<ATDShip>(PC->GetPawn()) && !PC->IsMoveInputIgnored())
		{
			bWaitControl = false;
			ControlTime = Now - TimingStart;
			Mark(*FString::Printf(TEXT("control (%s)"), *PC->GetPawn()->GetName()));
		}
	}
	// Done once both control and the terrain are there (a map with no terrain: control), plus 1 s for the probe's checks.
	const bool bHasTerrain = TActorIterator<ATDPlanetTerrain>(World) ? true : false;
	if (!bWaitControl && (!bWaitTerrain || !bHasTerrain))
	{
		const double Ready = FMath::Max(ControlTime, TerrainTime);
		if (!bProbe || Now - TimingStart >= Ready + 1.0)
		{
			EndTiming(TEXT("control and terrain"));
		}
	}
}

void UTDDescentSubsystem::EndTiming(const TCHAR* Why)
{
	if (bProbe)
	{
		const int32 After = PaintsAtFirstFrame >= 0 ? GTDCoverPaints - PaintsAtFirstFrame : -1;
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Descent.Probe RESULT (%s): cover survived OpenLevel %s (%d paints on the planet up to 1 s after control and terrain); ")
			TEXT("roar playing on the first frame %s, at control %s | OpenLevel +%.2f, post-load +%.2f, first frame +%.2f, terrain +%.2f, control +%.2f s | longest frame %.0f ms"),
			Why, After > 0 ? TEXT("YES") : TEXT("NO"), After, RoarAtFirstFrame == 1 ? TEXT("YES") : TEXT("NO"), IsRoarPlaying() ? TEXT("YES") : TEXT("NO"),
			OpenLevelTime, PostLoadTime, FirstFrameTime, TerrainTime, ControlTime, LongestFrame * 1000.0);
		bProbe = false;
		HideCover();
		StopRoar(0.5f);
	}
	else
	{
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Descent.Timing end (%s): first frame +%.2f, terrain +%.2f, control +%.2f s, longest frame %.0f ms"),
			Why, FirstFrameTime, TerrainTime, ControlTime, LongestFrame * 1000.0);
	}
	TimingStart = -1.0;
}

void UTDDescentSubsystem::ShowCover(float Alpha, FLinearColor Color)
{
	UGameViewportClient* Viewport = GEngine ? GEngine->GameViewport : nullptr;
	if (!Viewport) { return; }
	if (!Cover.IsValid())
	{
		Cover = SNew(STDDescentCover);
		Viewport->AddViewportWidgetContent(Cover.ToSharedRef(), 10000);
	}
	Cover->Color = FLinearColor(Color.R, Color.G, Color.B, FMath::Clamp(Alpha, 0.f, 1.f));
}

void UTDDescentSubsystem::HideCover()
{
	if (Cover.IsValid())
	{
		if (UGameViewportClient* Viewport = GEngine ? GEngine->GameViewport : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Cover.ToSharedRef());
		}
		Cover.Reset();
	}
}

void UTDDescentSubsystem::StartRoar(USoundBase* Sound)
{
	StopRoar(0.f);
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!Sound || !World) { return; }
	Roar = UGameplayStatics::CreateSound2D(World, Sound, 1.f, 1.f, 0.f, nullptr, /*bPersistAcrossLevelTransition*/ true, /*bAutoDestroy*/ false);
	if (Roar) { Roar->Play(); }
}

void UTDDescentSubsystem::StopRoar(float FadeSeconds)
{
	if (Roar)
	{
		if (FadeSeconds > 0.f) { Roar->FadeOut(FadeSeconds, 0.f); }
		else { Roar->Stop(); Roar->DestroyComponent(); }
		Roar = nullptr;
	}
}

bool UTDDescentSubsystem::IsRoarPlaying() const { return Roar && Roar->IsPlaying(); }

#if !UE_BUILD_SHIPPING
namespace TDDescentDebug
{
	// ENG-38 stage 1 step 1: TD.Descent.Probe [Verdance|Cinder] [sound path]. White cover up, persistent roar on, then the
	// game's own planet travel (today's route); logs every step and whether the cover and the roar made it to the planet.
	static FAutoConsoleCommandWithWorldAndArgs GProbe(TEXT("TD.Descent.Probe"),
		TEXT("Planet travel with a white Slate cover and a persistent roar up; logs the timing and whether both survive OpenLevel."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UTDGameInstance* GI = UTDGameInstance::Get(World);
			UTDDescentSubsystem* D = UTDDescentSubsystem::Get(World);
			if (!GI || !D) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.Descent.Probe: no Triad Drift game instance")); return; }
			const bool bCinder = Args.Num() > 0 && Args[0].Equals(TEXT("Cinder"), ESearchCase::IgnoreCase);
			const FString SoundPath = Args.Num() > 1 ? Args[1] : TEXT("/Game/TriadDrift/Gameplay/Audio/Engines/SW_Boost_Loop_01.SW_Boost_Loop_01");
			D->BeginTiming(TEXT("probe"));
			D->ArmProbe();
			D->ShowCover(Args.Num() > 2 ? FCString::Atof(*Args[2]) : 1.f);
			D->StartRoar(LoadObject<USoundBase>(nullptr, *SoundPath));
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Descent.Probe: cover up, roar %s (%s)"), D->IsRoarPlaying() ? TEXT("playing") : TEXT("NOT playing"), *SoundPath);
			const bool bOk = GI->TravelTo(bCinder ? ETDDestination::Cinder : ETDDestination::Verdance, bCinder ? FName(TEXT("Cinder_Orbit")) : FName(TEXT("Verdance_Orbit")));
			D->Mark(bOk ? TEXT("TravelTo") : TEXT("TravelTo REFUSED"));
		}));
}
#endif
