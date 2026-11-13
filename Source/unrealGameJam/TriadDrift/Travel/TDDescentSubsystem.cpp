// Triad Drift - planetary descent (ENG-38 stage 1). Owner: Teammate 1.

#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "Components/AudioComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
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

static TAutoConsoleVariable<int32> CVarTDDescentLegacy(TEXT("td.Descent.Legacy"), 0,
	TEXT("ENG-38: 1 = planet entry and exit use today's 0.6 s fade to black and the L_Transition loading screen; 0 = the white cover straight to the map (default)."));

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
	if (CoverPhase == ETDDescentCover::RampIn || CoverPhase == ETDDescentCover::Held)
	{
		SetCoverAlpha(1.f); // the last galaxy frame is full white
		CoverPhase = ETDDescentCover::Held;
		bDestLoaded = false;
	}
	if (!IsTiming()) { return; }
	OpenLevelTime = FPlatformTime::Seconds() - TimingStart;
	Mark(*FString::Printf(TEXT("OpenLevel %s (cover %s, roar %s)"), *FPackageName::GetShortName(MapName), IsCoverShown() ? TEXT("up") : TEXT("off"),
		IsRoarPlaying() ? TEXT("playing") : TEXT("silent")));
}

void UTDDescentSubsystem::HandlePostLoadMap(UWorld* World)
{
	if (World && World->GetGameInstance() == GetGameInstance() && CoverPhase == ETDDescentCover::Held)
	{
		DestWorld = World;
		DestPostLoadFrame = GFrameCounter;
		bDestLoaded = true;
		bDestFirstFrameSeen = false;
		bFirstViewBuilt = false;
		bArrivalSettled = false;
	}
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

const FName UTDDescentSubsystem::HeatGlowParam(TEXT("HeatGlow"));

void UTDDescentSubsystem::Tick(float DeltaTime)
{
	TickCover(FMath::Min(DeltaTime, 0.1f));
	TickHeat(FMath::Min(DeltaTime, 0.1f));
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

// ------------------------------------------------------------------------------------------------ covered route

bool UTDDescentSubsystem::IsLegacy() { return CVarTDDescentLegacy.GetValueOnGameThread() != 0; }

bool UTDDescentSubsystem::IsCoveredRoute(ETDDestination From, ETDDestination To)
{
	auto IsPlanet = [](ETDDestination D) { return D == ETDDestination::Verdance || D == ETDDestination::Cinder; };
	return !IsLegacy() && ((From == ETDDestination::Galaxy && IsPlanet(To)) || (IsPlanet(From) && To == ETDDestination::Galaxy));
}

float UTDDescentSubsystem::BeginCoveredTravel(ETDDestination To)
{
	CoverDestination = To;
	bDestLoaded = false;
	DestWorld.Reset();
	if (CoverAlpha >= 1.f)
	{
		CoverPhase = ETDDescentCover::Held;
		CoverFullAt = FPlatformTime::Seconds();
		return 0.f;
	}
	CoverPhase = ETDDescentCover::RampIn;
	CoverRampRate = 1.f / CoverRampSeconds;
	return (1.f - CoverAlpha) * CoverRampSeconds;
}

void UTDDescentSubsystem::SetEntryCover(float Alpha01)
{
	if (CoverPhase == ETDDescentCover::Off || CoverPhase == ETDDescentCover::RampIn)
	{
		if (Alpha01 > CoverAlpha)
		{
			SetCoverAlpha(Alpha01);
		}
	}
}

void UTDDescentSubsystem::AbortCover(const TCHAR* Why)
{
	if (CoverPhase == ETDDescentCover::Off && CoverAlpha <= 0.f && !Roar) { return; } // the entry ramp may be up with phase Off
	UE_LOG(LogTriadDrift, Log, TEXT("TD descent: cover dropped (%s)"), Why);
	CoverPhase = ETDDescentCover::Aborting;
	StopRoar(0.3f);
}

void UTDDescentSubsystem::SetCoverAlpha(float Alpha)
{
	CoverAlpha = FMath::Clamp(Alpha, 0.f, 1.f);
	if (CoverAlpha <= 0.f)
	{
		HideCover();
	}
	else
	{
		ShowCover(CoverAlpha);
	}
	if (CoverAlpha >= 1.f && CoverFullAt < 0.0)
	{
		CoverFullAt = FPlatformTime::Seconds();
	}
}

void UTDDescentSubsystem::TickCover(float Dt)
{
	const double Now = FPlatformTime::Seconds();
	switch (CoverPhase)
	{
	case ETDDescentCover::Off:
		return;
	case ETDDescentCover::RampIn:
		SetCoverAlpha(CoverAlpha + CoverRampRate * Dt);
		if (CoverAlpha >= 1.f) { CoverPhase = ETDDescentCover::Held; }
		break;
	case ETDDescentCover::Held:
	{
		UWorld* World = DestWorld.Get();
		bool bReveal = false;
		const TCHAR* Why = TEXT("");
		if (bDestLoaded && World && World->HasBegunPlay() && GFrameCounter > DestPostLoadFrame)
		{
			if (!bDestFirstFrameSeen) { bDestFirstFrameSeen = true; DestFirstFrameAt = Now; }
			// T4's first view around the arrival, once, under the cover (its cost lands on the white frame).
			ATDPlanetTerrain* Terrain = nullptr;
			for (TActorIterator<ATDPlanetTerrain> It(World); It; ++It) { Terrain = *It; break; }
			if (Terrain && !bFirstViewBuilt && ArrivalWorld.Get() == World && Terrain->GetChunkCount() > 0)
			{
				bFirstViewBuilt = true;
				const double Ms = Terrain->BuildFirstViewAt(ArrivalCourseLocal);
				Mark(*FString::Printf(TEXT("terrain first view at the arrival (%.0f ms; setup %.0f ms)"), Ms, Terrain->GetSetupMs()));
			}
			bool bTerrainReady = !Terrain || (Terrain->GetChunkCount() > 0 && Terrain->GetPendingCount() == 0);
			// The flight zone fits its ceiling to the terrain (highest ground + 400 m): wait for it, then keep the arrival
			// out of the ceiling's warning band (a high arrival over high ground showed "EDGE OF THE FLIGHT AREA").
			ATDAtmosphereFlightZone* Zone = nullptr;
			for (TActorIterator<ATDAtmosphereFlightZone> It(World); It; ++It) { Zone = *It; break; }
			if (Terrain && Zone && !Zone->IsFittedToTerrain()) { bTerrainReady = false; }
			if (bTerrainReady && !bArrivalSettled && ArrivalWorld.Get() == World && Zone)
			{
				bArrivalSettled = true;
				const float Top = Zone->GetZoneBox().Max.Z - CeilingClearanceCm;
				for (TActorIterator<ATDShip> It(World); It; ++It)
				{
					FVector L = It->GetActorLocation();
					if (L.Z <= Top) { continue; }
					const float Ground = Terrain ? Terrain->GetHeightAt(L) : L.Z - ArrivalAltitudeCm;
					const float NewZ = FMath::Max(Top, Ground + 30000.f);
					Mark(*FString::Printf(TEXT("arrival lowered %.0f m under the ceiling (%s)"), (L.Z - NewZ) / 100.f, *It->GetName()));
					L.Z = NewZ;
					It->TeleportSpacecraft(L, It->GetActorRotation(), /*bKeepSpeed*/ true);
				}
				FVector A = ArrivalWorldXf.GetLocation();
				if (A.Z > Top) { A.Z = FMath::Max(Top, (Terrain ? Terrain->GetHeightAt(A) : A.Z) + 30000.f); ArrivalWorldXf.SetLocation(A); } // retakes too
			}
			const double Held = Now - DestFirstFrameAt;
			if (bTerrainReady && Held >= RevealMinHoldSeconds) { bReveal = true; Why = Terrain ? TEXT("terrain built") : TEXT("no terrain"); }
			else if (Held >= RevealMaxHoldSeconds) { bReveal = true; Why = TEXT("4 s hold limit"); }
		}
		if (!bReveal && CoverFullAt >= 0.0 && Now - CoverFullAt > CoverHardLimitSeconds) { bReveal = true; Why = TEXT("12 s hard limit"); }
		if (bReveal)
		{
			Mark(*FString::Printf(TEXT("reveal (%s)"), Why));
			UE_LOG(LogTriadDrift, Log, TEXT("TD descent: reveal (%s)"), Why);
			CoverPhase = ETDDescentCover::Revealing;
			StopRoar(1.f);
		}
		break;
	}
	case ETDDescentCover::Revealing:
	case ETDDescentCover::Aborting:
		SetCoverAlpha(CoverAlpha - Dt / (CoverPhase == ETDDescentCover::Revealing ? RevealSeconds : 0.3f));
		if (CoverAlpha <= 0.f)
		{
			CoverPhase = ETDDescentCover::Off;
			CoverFullAt = -1.0;
			bDestLoaded = false;
		}
		break;
	}
}

void UTDDescentSubsystem::SetEntryHeat(float Heat01)
{
	Heat = FMath::Max(Heat, FMath::Clamp(Heat01, 0.f, 1.f));
	HeatFedLeft = 0.25f; // ticks of this subsystem, not wall time
}

void UTDDescentSubsystem::ClearHeat()
{
	Heat = 0.f;
	HeatFedLeft = 0.f;
	TickHeat(0.f); // apply now
}

int32 UTDDescentSubsystem::ApplyHeatToShip(ATDShip* Ship, float InHeat)
{
	int32 Set = 0;
	if (!Ship) { return 0; }
	TInlineComponentArray<UMeshComponent*> Meshes(Ship);
	for (UMeshComponent* Mesh : Meshes)
	{
		for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
		{
			UMaterialInterface* M = Mesh->GetMaterial(i);
			if (!M) { continue; }
			float Current = 0.f;
			if (!M->GetScalarParameterValue(FHashedMaterialParameterInfo(HeatGlowParam), Current)) { continue; } // no HeatGlow: leave it alone
			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(M);
			if (!MID)
			{
				if (InHeat <= 0.f) { continue; } // never heated: no instance needed
				MID = Mesh->CreateDynamicMaterialInstance(i, M);
			}
			if (MID) { MID->SetScalarParameterValue(HeatGlowParam, InHeat); ++Set; }
		}
	}
	return Set;
}

void UTDDescentSubsystem::TickHeat(float Dt)
{
	if (Heat <= 0.f && AppliedHeat <= 0.f) { return; }
	// Decay once the entry has stopped feeding it and the cover is not holding (after the reveal, or with no cover).
	const bool bFed = HeatFedLeft > 0.f;
	HeatFedLeft = FMath::Max(0.f, HeatFedLeft - Dt);
	const bool bHeld = CoverPhase == ETDDescentCover::RampIn || CoverPhase == ETDDescentCover::Held;
	if (!bFed && !bHeld && Heat > 0.f)
	{
		Heat = FMath::Max(0.f, Heat - Dt / HeatDecaySeconds);
	}
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World) { return; }
	// Every frame while hot: ships spawn (the planet squad) and the player can switch; values are cheap to set.
	for (TActorIterator<ATDShip> It(World); It; ++It)
	{
		ApplyHeatToShip(*It, Heat);
	}
	AppliedHeat = Heat;
}

void UTDDescentSubsystem::RecordEntry(ETDDestination Planet, const ATDPlanetAtmosphere* Atmosphere, const FVector& WorldPoint, const FVector& WorldVelocity)
{
	if (!Atmosphere) { ClearArrival(); return; }
	FVector GlobeVel = FVector::ZeroVector;
	const FTDGlobePoint P = FTDPlanetEntry::GlobeFromWorld(Atmosphere, WorldPoint, WorldVelocity, GlobeVel);
	RecordEntryGlobe(Planet, P, GlobeVel);
}

void UTDDescentSubsystem::RecordEntryGlobe(ETDDestination Planet, const FTDGlobePoint& P, const FVector& GlobeVelocity)
{
	bArrivalPending = true;
	ArrivalPlanet = Planet;
	ArrivalPoint = P;
	ArrivalGlobeVelocity = GlobeVelocity;
	ArrivalWorld.Reset();
	UE_LOG(LogTriadDrift, Log, TEXT("TD descent: entry recorded at lat %.1f lon %.1f (%.0f deg from the landing zone)"), P.LatDeg, P.LonDeg, P.AngleFromPadDeg());
}

bool UTDDescentSubsystem::GetArrival(UWorld* World, ETDDestination Planet, const FTransform& CourseToWorld, FTransform& OutWorld)
{
	if (World && ArrivalWorld.Get() == World)
	{
		OutWorld = ArrivalWorldXf; // a retake in the same visit: the same arrival
		return true;
	}
	if (!bArrivalPending || ArrivalPlanet != Planet || !World)
	{
		return false;
	}
	const FTransform Local = FTDPlanetEntry::ArrivalFromGlobe(Planet, ArrivalPoint, ArrivalGlobeVelocity, ArrivalAltitudeCm); // T4's ring: 6 km
	ArrivalCourseLocal = Local.GetLocation();
	OutWorld = Local * CourseToWorld;
	ArrivalWorldXf = OutWorld;
	ArrivalWorld = World;
	bArrivalPending = false; // consumed: a later visit without a new entry uses the fixed start
	UE_LOG(LogTriadDrift, Log, TEXT("TD descent: arrival at course-local (%.0f, %.0f, %.0f) m, yaw %.0f"),
		ArrivalCourseLocal.X / 100.f, ArrivalCourseLocal.Y / 100.f, ArrivalCourseLocal.Z / 100.f, Local.Rotator().Yaw);
	return true;
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
