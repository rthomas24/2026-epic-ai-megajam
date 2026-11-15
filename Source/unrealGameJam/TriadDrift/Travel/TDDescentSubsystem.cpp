// Triad Drift - planetary descent (ENG-38 stage 1). Owner: Teammate 1.

#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Perf/TDWarmupSubsystem.h"
#include "Camera/CameraComponent.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/World/TDCloudDeck.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "Components/AudioComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "TriadDrift/Flight/TDFlightFeedbackComponent.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
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
#include "ShaderCompiler.h"
#include "ShaderPipelineCache.h"

// ------------------------------------------------------------------------------------------------ cover widget

static int32 GTDCoverPaints = 0;

static TAutoConsoleVariable<int32> CVarTDDescentWarmUnderCover(TEXT("td.Descent.WarmUnderCover"), 1,
	TEXT("#89: 1 = on the way back to the galaxy the warm-up stream is finished under the white cover (the reveal waits, 4 s at most); 0 = as before."));
static TAutoConsoleVariable<int32> CVarTDDescentWarmShaders(TEXT("td.Descent.WarmShaders"), 1,
	TEXT("T6 (T1's checkerboard on a cold cache): 1 = the reveal also waits while shaders compile (Development: drawn with the grid material until done) or PSOs precompile (Shipping, first run), up to the 12 s hard limit; 0 = as before."));
/** When the reveal began waiting on shaders (-1 = not waiting) and the queue then, for the log line at the reveal. */
static double GTDShaderWaitStart = -1.0;
static int32 GTDShaderJobsAtStart = 0;
static uint32 GTDPsosAtStart = 0;
static TAutoConsoleVariable<int32> CVarTDDescentGlide(TEXT("td.Descent.Glide"), 1,
	TEXT("ENG-38 stage 2: 1 = the arrival sits over the cloud deck and a forced glide carries the ship down through it after the reveal; 0 = today's arrival under the ceiling."));

static TAutoConsoleVariable<int32> CVarTDDescentDirectorShot(TEXT("td.Descent.DirectorShot"), 1,
	TEXT("ENG-38: 1 = the atmosphere entry and climb out are filmed by the director camera (three-quarter front, under the white the cut back); 0 = the chase camera throughout."));

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
	// A retake (CUT) in the same map ends a glide and gives the squad back to the zone.
	if (UTDGameInstance* GI = Cast<UTDGameInstance>(GetGameInstance()))
	{
		GI->OnSceneRestart.AddWeakLambda(this, [this](FName, int32) { EndGlide(TEXT("retake")); });
	}
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
	EndBurnShot(); // the cover is full white here: the cut back to the chase camera is hidden
	EndGlide(TEXT("level change"));
	bArrivedOverDeck = false;
	// The pilot who goes on is the one flying now (docking with Lancer after landing Bastion parked Bastion in the hangar and
	// put Moss on foot): whenever a level change starts with the player in a ship, that ship's pilot is the last ship.
	if (UWorld* From = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(From, 0);
		const ATDShip* Flown = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
		if (Flown && Flown->IsPlayerFlown())
		{
			if (UTDGameInstance* GI = Cast<UTDGameInstance>(GetGameInstance())) { GI->SetLastShip(Flown->GetShipId()); }
		}
	}
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
		bArrivedOverDeck = false;
		bGlideEndPredicted = false;
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
const FName UTDDescentSubsystem::IntensityParam(TEXT("User.Intensity"));

void UTDDescentSubsystem::Tick(float DeltaTime)
{
	TickCover(FMath::Min(DeltaTime, 0.1f));
	TickHeat(FMath::Min(DeltaTime, 0.1f));
	TickCloudDeck();
	TickGlide(FMath::Min(DeltaTime, 0.1f));
	TickExposureRamp();
	TickGlideWind(FMath::Min(DeltaTime, 0.1f));
	TickArrivalFloor();
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
	if (ClimbHoldSeconds > 0.f)
	{
		// The climb out drives the cover (SetEntryCover); the level change waits for the climb.
		const float Hold = ClimbHoldSeconds;
		ClimbHoldSeconds = 0.f;
		CoverPhase = ETDDescentCover::RampIn;
		CoverRampRate = 0.f;
		return Hold;
	}
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
	EndBurnShot();
	EndGlide(Why);
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
			if (bDepartPending && CoverDestination == ETDDestination::Galaxy) { PlaceDeparture(World); }
			// Every covered arrival on a planet settles (a recorded entry, or any other travel in: the cheat, a stage, a flow
			// travel with no entry point - T2's live finding: those glided from the spawn height).
			if (bTerrainReady && !bArrivalSettled && Zone && (ArrivalWorld.Get() == World || (IsGlideEnabled() && CoverDestination != ETDDestination::Galaxy)))
			{
				bArrivalSettled = true;
				float DeckZ = IsGlideEnabled() && CoverDestination != ETDDestination::Galaxy ? FindDeckZ(World) : 0.f;
				if (DeckZ > 0.f)
				{
					// The glide-lite: the arrival over the cloud tops, the squad kept in formation and released from the zone
					// (even above the normal ceiling) until the glide ends.
					APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
					ATDShip* Flown = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
					if (Flown)
					{
						const FVector From = Flown->GetActorLocation();
						const float Ground = Terrain ? Terrain->GetHeightAt(From) : 0.f;
						const FVector Fwd = FVector(Flown->GetActorForwardVector().X, Flown->GetActorForwardVector().Y, 0.f).GetSafeNormal();
						// The glide end, predicted from a provisional plan; T4's deck for that corridor; the plan again on it.
						auto PredictEnd = [&](float StartZ)
						{
							float Dur = 0.f, Speed = 0.f;
							PlanGlide(StartZ, Ground + GlideEndAboveGroundCm, Dur, Speed);
							FVector E = From + Fwd * Speed * Dur * 0.85f;
							E.Z = (Terrain ? Terrain->GetGroundZAt(E) : 0.f) + GlideEndAboveGroundCm;
							return E;
						};
						FVector End = PredictEnd(DeckZ + GlideAboveDeckCm);
						if (Terrain)
						{
							const float Corridor = Terrain->GetCloudDeckZFor(FVector(From.X, From.Y, DeckZ + GlideAboveDeckCm), End);
							if (Corridor > 0.f) { DeckZ = Corridor; End = PredictEnd(DeckZ + GlideAboveDeckCm); }
						}
						const float Want = FMath::Max(DeckZ + GlideAboveDeckCm, Ground + 30000.f);
						const float Shift = Want - From.Z;
						for (TActorIterator<ATDShip> It(World); It; ++It)
						{
							FVector L = It->GetActorLocation();
							L.Z += Shift;
							It->TeleportSpacecraft(L, It->GetActorRotation(), /*bKeepSpeed*/ true);
							if (UTDShipMovementComponent* M = It->GetShipMovement()) { M->SetBoundaryReleaseFor(GlideOverride, true); }
						}
						Mark(*FString::Printf(TEXT("arrival over the cloud deck (deck %.0f m, %.0f m above it, shift %.0f m)"), DeckZ / 100.f, (Want - DeckZ) / 100.f, Shift / 100.f));
						GlideEndPredicted = End;
						bGlideEndPredicted = Terrain != nullptr;
						// T4: the deck actor moves to the corridor's deck, and the scatter round the glide end is built first.
						if (Terrain) { Terrain->PrepareArrival(FVector(From.X, From.Y, Want), End); }
						if (ArrivalWorld.Get() == World) { FVector A = ArrivalWorldXf.GetLocation(); A.Z = Want; ArrivalWorldXf.SetLocation(A); } // retakes too
						bArrivedOverDeck = true;
					}
				}
				const float Top = DeckZ > 0.f ? Zone->GetZoneBox().Max.Z : Zone->GetZoneBox().Max.Z - CeilingClearanceCm;
				// Over the deck the squad is released and glides down together: no lowering (a wingman lowered 588 m in the
				// live run of Nov 14 broke the formation).
				for (TActorIterator<ATDShip> It(World); It; ++It)
				{
					if (bArrivedOverDeck) { break; }
					FVector L = It->GetActorLocation();
					if (L.Z <= Top) { continue; }
					const float Ground = Terrain ? Terrain->GetHeightAt(L) : L.Z - ArrivalAltitudeCm;
					const float NewZ = FMath::Max(Top, Ground + 30000.f);
					Mark(*FString::Printf(TEXT("arrival lowered %.0f m under the ceiling (%s)"), (L.Z - NewZ) / 100.f, *It->GetName()));
					L.Z = NewZ;
					It->TeleportSpacecraft(L, It->GetActorRotation(), /*bKeepSpeed*/ true);
				}
				FVector A = ArrivalWorldXf.GetLocation();
				if (!bArrivedOverDeck && A.Z > Top) { A.Z = FMath::Max(Top, (Terrain ? Terrain->GetHeightAt(A) : A.Z) + 30000.f); ArrivalWorldXf.SetLocation(A); } // retakes too
			}
			const double Held = Now - DestFirstFrameAt;
			if (bTerrainReady && bGlideEndPredicted && Terrain && !Terrain->IsArrivalReady(GlideEndPredicted)) { bTerrainReady = false; } // T4's scatter
			// #89 (T6, with the Lead's leave): back in the galaxy the warm-up stream ran in the open and the first enemy to warp
			// in forced flush after flush of it (76 of 664 frames over 16.6 ms in the first 10 s). Finish it under the white.
			if (bTerrainReady && CoverDestination == ETDDestination::Galaxy && CVarTDDescentWarmUnderCover.GetValueOnGameThread() != 0)
			{
				UTDWarmupSubsystem* Warm = UTDWarmupSubsystem::Get(World);
				if (Warm && Warm->IsLoading()) { Warm->HurryUnderCover(); bTerrainReady = Warm->IsLoading() ? false : bTerrainReady; }
				// T4's planet globes: bake under the cover too (cached for the run after the first, so this only waits once).
				if (ATDPlanetAtmosphere::AreGlobeBakesRunning(World)) { bTerrainReady = false; }
			}
			// T6, with the Lead's leave (T1's checkerboard on a cold cache): the destination's relief, mountain ring, scatter
			// kinds, hut kits and Fab props are loaded by now, but on a cold cache their shaders are still compiling
			// (Development: drawn with the engine's grid material until done) or their PSOs precompiling (Shipping, first run).
			// Keep the white up while that queue drains; the 12 s hard limit below still applies.
			bool bShadersPending = false;
			if (bTerrainReady && CVarTDDescentWarmShaders.GetValueOnGameThread() != 0)
			{
				const int32 Jobs = GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : 0;
				const uint32 Psos = FShaderPipelineCache::NumPrecompilesRemaining();
				if (Jobs > 0 || Psos > 0)
				{
					bShadersPending = true;
					bTerrainReady = false;
					if (GTDShaderWaitStart < 0.0)
					{
						GTDShaderWaitStart = Now; GTDShaderJobsAtStart = Jobs; GTDPsosAtStart = Psos;
						UE_LOG(LogTriadDrift, Log, TEXT("TD descent: holding the cover for %d shader jobs and %u PSO precompiles"), Jobs, Psos);
					}
				}
			}
			if (bTerrainReady && Held >= RevealMinHoldSeconds) { bReveal = true; Why = Terrain ? TEXT("terrain built") : TEXT("no terrain"); }
			else if (Held >= RevealMaxHoldSeconds && !bShadersPending) { bReveal = true; Why = TEXT("4 s hold limit"); }
		}
		if (!bReveal && CoverFullAt >= 0.0 && Now - CoverFullAt > CoverHardLimitSeconds) { bReveal = true; Why = TEXT("12 s hard limit"); }
		if (bReveal)
		{
			Mark(*FString::Printf(TEXT("reveal (%s)"), Why));
			if (GTDShaderWaitStart >= 0.0)
			{
				UE_LOG(LogTriadDrift, Log, TEXT("TD descent: shaders under the cover: waited %.2f s (shader jobs %d -> %d, PSO precompiles %u -> %u)"),
					Now - GTDShaderWaitStart, GTDShaderJobsAtStart, GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : 0,
					GTDPsosAtStart, FShaderPipelineCache::NumPrecompilesRemaining());
				GTDShaderWaitStart = -1.0;
			}
			UE_LOG(LogTriadDrift, Log, TEXT("TD descent: reveal (%s)"), Why);
			CoverPhase = ETDDescentCover::Revealing;
			StopRoar(1.f);
			bool bGlideBegan = false;
			if (IsGlideEnabled() && CoverDestination != ETDDestination::Galaxy && World && bArrivedOverDeck) // never from a spawn height
			{
				APlayerController* GlidePC = UGameplayStatics::GetPlayerController(World, 0);
				if (ATDShip* Flown = GlidePC ? Cast<ATDShip>(GlidePC->GetPawn()) : nullptr) { bGlideBegan = BeginGlide(Flown); }
			}
			// #139 (Nick's review; T6 with the Lead's leave): the over-deck placement released every ship from the zone
			// boundary for the glide. When no glide begins (td.Descent.Glide turned off under the cover, no flown ship,
			// a refused BeginGlide) nothing would ever undo it: give the squad back to the zone here.
			if (!bGlideBegan && World && !IsGliding())
			{
				for (TActorIterator<ATDShip> It(World); It; ++It)
				{
					if (UTDShipMovementComponent* M = It->GetShipMovement()) { M->SetBoundaryReleaseFor(GlideOverride, false); }
				}
				if (bArrivedOverDeck) { UE_LOG(LogTriadDrift, Log, TEXT("TD descent: no glide at the reveal: the boundary release is undone on every ship")); }
			}
			OnRevealed.Broadcast(CoverDestination);
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

void UTDDescentSubsystem::RecordSquad(UWorld* World)
{
	ATDSquadManager* Squad = nullptr;
	for (TActorIterator<ATDSquadManager> It(World); It; ++It) { Squad = *It; break; }
	if (!Squad) { ClearSquadCarry(); return; }
	FTDSquadCarry C;
	C.bValid = true;
	C.Active = Squad->GetActiveShipId();
	for (const FTDSquadSlot& Slot : Squad->GetSlots())
	{
		const UTDHealthComponent* H = Slot.Ship ? Slot.Ship->GetHealth() : nullptr;
		const bool bAlive = Slot.State == ETDSquadSlotState::Alive && H && !H->IsDead();
		C.Hull01.Add(Slot.ShipId, bAlive ? H->GetHull01() : 0.f); // downed / dying: 0 (the read side sets the floor)
		C.Shield01.Add(Slot.ShipId, bAlive ? H->GetShield01() : 0.f);
	}
	SquadCarry = C;
	SquadCarryWorld.Reset();
	// T5: FTDProgress::LastShip is what the spawners and a Continue read (saved); nothing wrote it until now.
	if (UTDGameInstance* GI = Cast<UTDGameInstance>(GetGameInstance())) { GI->SetLastShip(C.Active); }
	UE_LOG(LogTriadDrift, Log, TEXT("TD descent: squad carried (flown %s; hulls %s)"), *UEnum::GetValueAsString(C.Active),
		*FString::JoinBy(C.Hull01, TEXT(" "), [](const TPair<ETDShipId, float>& P) { return FString::Printf(TEXT("%s %.0f%%"), *UEnum::GetDisplayValueAsText(P.Key).ToString(), P.Value * 100.f); }));
}

bool UTDDescentSubsystem::ConsumeSquadCarry(UWorld* World, FTDSquadCarry& Out, bool& bOutFirstSpawnInWorld)
{
	bOutFirstSpawnInWorld = false;
	if (!SquadCarry.bValid || !World) { return false; }
	if (SquadCarryWorld.IsValid() && SquadCarryWorld.Get() != World)
	{
		ClearSquadCarry(); // a later map with no new record: as before
		return false;
	}
	bOutFirstSpawnInWorld = !SquadCarryWorld.IsValid();
	SquadCarryWorld = World;
	Out = SquadCarry;
	return true;
}

void UTDDescentSubsystem::TickCloudDeck()
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const ATDCloudDeck* Deck = nullptr;
	if (World) { for (TActorIterator<ATDCloudDeck> It(World); It; ++It) { Deck = *It; break; } }
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	ATDShip* Ship = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
	if (!Deck || !Ship)
	{
		if (UNiagaraComponent* W = CloudWisps.Get()) { W->Deactivate(); }
		CloudWisps.Reset();
		LastShipDeckSide = 0.f;
		return;
	}
	const UTDTravelFX* FX = UTDTravelSubsystem::GetDefaultTravelFX(World);
	const float Rel = Ship->GetActorLocation().Z - Deck->GetActorLocation().Z;
	const float Side = FMath::Sign(Rel);
	const bool bGlidePass = IsGliding() && GlideShip.Get() == Ship;
	// Through the deck, either way: one punch. During the glide only its sound (the in-cloud layer replaces the puffs).
	if (LastDeckShip.Get() == Ship && LastShipDeckSide != 0.f && Side != 0.f && Side != LastShipDeckSide && FX)
	{
		++CloudPunches;
		FTDFXCue Punch = FX->CloudPunch;
		if (bGlidePass) { Punch.VFX.Reset(); }
		UTDStatics::PlayCue(World, Punch, Ship->GetActorLocation(), Ship->GetActorRotation());
	}
	LastShipDeckSide = Side;
	LastDeckShip = Ship;
	// The in-cloud layer (NS_TD_Entry_CloudWisps, the effects builder's contract): attached to the flown ship; User.Intensity
	// 0 -> 1 -> 0 across the deck, peaking at its height, over about 1.5-2 s of the glide (the band follows the ship's
	// vertical speed); User.Color / User.Embers per planet; capped at 0.5 under reduced motion; deactivated outside the band.
	float Band = CloudDeckBand;
	if (bGlidePass)
	{
		const float Vz = FMath::Abs((float)Ship->GetVelocity().Z);
		Band = FMath::Clamp(Vz * 0.9f, 1500.f, GlideDeckPassBand * 2.f); // +-0.9 s of vertical travel
	}
	const bool bNear = FMath::Abs(Rel) < Band;
	if (bGlidePass && bNear && !bInGlideDeckPass) { ++GlideDeckPasses; } // entering the deck's band during the glide
	bInGlideDeckPass = bGlidePass && bNear;
	if (!bNear)
	{
		if (UNiagaraComponent* W = CloudWisps.Get()) { W->Deactivate(); }
		CloudWisps.Reset();
		CloudIntensity = 0.f;
		return;
	}
	if (!CloudWisps.IsValid() && FX)
	{
		UAudioComponent* NoAudio = nullptr;
		CloudWisps = UTDStatics::PlayCueAttachedLoop(FX->CloudWisps, Ship->GetRootComponent(), NAME_None, NoAudio);
		if (UNiagaraComponent* W = CloudWisps.Get())
		{
			const bool bCinder = Deck->GetPlanet() == ETDDestination::Cinder;
			W->SetVariableLinearColor(TEXT("User.Color"), bCinder ? FLinearColor(0.45f, 0.36f, 0.30f, 1.f) : FLinearColor(0.78f, 0.92f, 0.92f, 1.f));
			W->SetVariableFloat(TEXT("User.Embers"), bCinder ? 1.f : 0.f);
		}
	}
	CloudIntensity = FMath::Clamp(1.f - FMath::Abs(Rel) / Band, 0.f, 1.f);
	if (UTDSettingsSubsystem::IsReduceMotion(Ship)) { CloudIntensity = FMath::Min(CloudIntensity, 0.5f); }
	if (UNiagaraComponent* W = CloudWisps.Get()) { W->SetVariableFloat(TEXT("User.Intensity"), CloudIntensity); }
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

int32 UTDDescentSubsystem::SetShipMaterialScalar(ATDShip* Ship, FName Param, float Value)
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
			if (!M->GetScalarParameterValue(FHashedMaterialParameterInfo(Param), Current)) { continue; } // not exposed: leave it alone
			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(M);
			if (!MID)
			{
				if (Value == Current) { continue; } // nothing to change: no instance needed
				MID = Mesh->CreateDynamicMaterialInstance(i, M);
			}
			if (MID) { MID->SetScalarParameterValue(Param, Value); ++Set; }
		}
	}
	return Set;
}

int32 UTDDescentSubsystem::GetBurnFXCount() const
{
	int32 N = 0;
	for (const auto& P : BurnFX) { if (P.Value.IsValid() && P.Value->IsActive()) { ++N; } }
	return N;
}

void UTDDescentSubsystem::StopBurnFX()
{
	for (auto& P : BurnFX) { if (UNiagaraComponent* FX = P.Value.Get()) { FX->Deactivate(); } }
	BurnFX.Reset();
}

void UTDDescentSubsystem::TickHeat(float Dt)
{
	// The camera's burn feel and the HUD hold follow the same curve (T2's hold: dim to 35 %, objectives and slates away).
	UTDFlightFeedbackComponent::DescentIntensity = Heat;
	const bool bHold = Heat > 0.f || CoverPhase != ETDDescentCover::Off;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWorld* W = GI->GetWorld())
		{
			if (bHold) { UTDUIHostSubsystem::PushHUDFocus(W, TEXT("Entry"), 0.35f, true); bHUDHeld = true; }
			else if (bHUDHeld) { UTDUIHostSubsystem::PopHUDFocus(W, TEXT("Entry")); bHUDHeld = false; }
		}
	}
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
	const UTDTravelFX* FX = UTDTravelSubsystem::GetDefaultTravelFX(World);
	for (TActorIterator<ATDShip> It(World); It; ++It)
	{
		ATDShip* Ship = *It;
		ApplyHeatToShip(Ship, Heat);
		// The burn on every squad ship (the flown ship and the wingmen), on both sides of the load: User.Intensity = heat.
		TWeakObjectPtr<UNiagaraComponent>& Slot = BurnFX.FindOrAdd(Ship);
		if (Heat > 0.f && !Slot.IsValid() && FX)
		{
			FTDFXCue Cue = bBurnExit ? FX->AtmosphericExit : FX->AtmosphericEntry;
			Cue.Sound.Reset(); // sound: the roar (covered route) or the entry component's own (legacy)
			UAudioComponent* NoAudio = nullptr;
			Slot = UTDStatics::PlayCueAttachedLoop(Cue, Ship->GetRootComponent(), NAME_None, NoAudio);
		}
		if (UNiagaraComponent* NC = Slot.Get())
		{
			NC->SetVariableFloat(IntensityParam, Heat);
		}
	}
	if (Heat <= 0.f) { StopBurnFX(); }
	AppliedHeat = Heat;
}

const FName UTDDescentSubsystem::GlideOverride(TEXT("DescentGlide"));

bool UTDDescentSubsystem::IsGlideEnabled() { return CVarTDDescentGlide.GetValueOnGameThread() != 0; }

float UTDDescentSubsystem::FindDeckZ(UWorld* World)
{
	if (!World) { return 0.f; }
	// The built deck actor (T4 moves it to the corridor's deck at PrepareArrival); else T4's zone-wide deck.
	for (TActorIterator<ATDCloudDeck> It(World); It; ++It) { if (It->IsBuilt()) { return It->GetActorLocation().Z; } }
	for (TActorIterator<ATDPlanetTerrain> It(World); It; ++It) { const float Z = It->GetCloudDeckZ(); if (Z > 0.f) { return Z; } break; }
	return 0.f;
}

float UTDDescentSubsystem::GetDescent01(const UObject* WorldContext)
{
	const UTDDescentSubsystem* D = Get(WorldContext);
	return D && D->IsGliding() ? D->GlideProgress : -1.f;
}

void UTDDescentSubsystem::PlanGlide(float StartZ, float EndZ, float& OutSeconds, float& OutSpeed)
{
	const float Drop = FMath::Max(StartZ - EndZ, 0.f);
	OutSeconds = FMath::Clamp(Drop / 8500.f, GlideSeconds, GlideMaxSeconds); // ~9-10 s for T4's 1.1-1.6 km decks (the Lead)
	OutSpeed = FMath::Clamp(Drop / (OutSeconds * 0.7f) * 1.1f, GlideSpeed, GlideMaxSpeed);
}

bool UTDDescentSubsystem::BeginGlide(ATDShip* Ship, float GroundZFallback)
{
	UTDShipMovementComponent* Move = Ship ? Ship->GetShipMovement() : nullptr;
	if (!Move || IsGliding()) { return false; }
	GlideShip = Ship;
	GlideTime = 0.f;
	bGlideLevelling = false;
	GlideLevelTime = 0.f;
	GlideGroundFallback = GroundZFallback;
	GlideDeckZ = FindDeckZ(Ship->GetWorld());
	GlideStartZ = Ship->GetActorLocation().Z;
	GlideEndZ = GlideStartZ;
	GlideProgress = 0.f;
	{
		float Ground = GroundZFallback;
		for (TActorIterator<ATDPlanetTerrain> It(Ship->GetWorld()); It; ++It) { Ground = It->GetGroundZAt(Ship->GetActorLocation()); break; }
		PlanGlide(GlideStartZ, Ground + GlideEndAboveGroundCm, GlideDur, GlideSpeedNow);
	}
	Move->SetSpeedOverrideFor(GlideOverride, GlideSpeedNow, GlideSteering);
	Move->SetForwardSpeedImmediate(GlideSpeedNow);
	if (UTDHealthComponent* Health = Ship->GetHealth()) { Health->SetInvulnerable(true); }
	Mark(TEXT("glide begins"));
	UE_LOG(LogTriadDrift, Log, TEXT("TD descent: glide begins at %.0f m (deck at %.0f m): %.1f s at %.0f m/s"), GlideStartZ / 100.f, GlideDeckZ / 100.f, GlideDur, GlideSpeedNow / 100.f);
	return true;
}

void UTDDescentSubsystem::EndGlide(const TCHAR* Why)
{
	ATDShip* Ship = GlideShip.Get();
	// The zone takes the squad back (released since the arrival over the deck), whether or not a glide ran.
	if (UWorld* W = Ship ? Ship->GetWorld() : (GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr))
	{
		for (TActorIterator<ATDShip> It(W); It; ++It) { if (UTDShipMovementComponent* M = It->GetShipMovement()) { M->SetBoundaryReleaseFor(GlideOverride, false); } }
	}
	if (Ship && GlideTime >= 0.f)
	{
		if (UTDShipMovementComponent* Move = Ship->GetShipMovement())
		{
			Move->ClearSpeedOverrideFor(GlideOverride);
			Move->ClearPitchHold();
		}
		if (UTDHealthComponent* Health = Ship->GetHealth()) { Health->SetInvulnerable(false); }
		UE_LOG(LogTriadDrift, Log, TEXT("TD descent: glide ends (%s) at %.0f m"), Why, Ship->GetActorLocation().Z / 100.f);
	}
	GlideShip.Reset();
	bGlideEndPredicted = false;
	GlideTime = -1.f;
	GlideProgress = -1.f;
	bGlideLevelling = false;
}

void UTDDescentSubsystem::TickGlide(float Dt)
{
	if (GlideTime < 0.f) { return; }
	ATDShip* Ship = GlideShip.Get();
	UTDShipMovementComponent* Move = Ship ? Ship->GetShipMovement() : nullptr;
	if (!Move || !Ship->IsPlayerFlown() || (Ship->GetHealth() && Ship->GetHealth()->IsDead()))
	{
		EndGlide(TEXT("the ship changed"));
		return;
	}
	GlideTime += Dt;
	const FVector L = Ship->GetActorLocation();
	float Ground = GlideGroundFallback;
	for (TActorIterator<ATDPlanetTerrain> It(Ship->GetWorld()); It; ++It) { Ground = It->GetHeightAt(L); break; }
	GlideEndZ = Ground + GlideEndAboveGroundCm; // re-sampled under the ship as it flies
	GlideProgress = GlideStartZ > GlideEndZ ? FMath::Clamp((GlideStartZ - L.Z) / (GlideStartZ - GlideEndZ), 0.f, 1.f) : 1.f;
	if (!bGlideLevelling)
	{
		const float Left = FMath::Max(GlideDur - GlideTime, 0.5f);
		const float Rate = (L.Z - GlideEndZ) / Left; // cm/s down wanted
		const float Pitch = -FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Rate / GlideSpeedNow, 0.f, 0.7f)));
		Move->SetPitchHold(Pitch, 40.f);
		if (GlideTime >= GlideDur || L.Z <= GlideEndZ + 2000.f)
		{
			bGlideLevelling = true; // level off, then hand back
			GlideLevelTime = 0.f;
			Mark(TEXT("glide levels off"));
		}
	}
	else
	{
		GlideLevelTime += Dt;
		Move->SetPitchHold(0.f, 60.f);
		// The speed eases from the glide's to cruise over the level-off (no drop when the override goes).
		const float A = FMath::Clamp(GlideLevelTime / GlideHandBackSeconds, 0.f, 1.f);
		Move->SetSpeedOverrideFor(GlideOverride, FMath::Lerp(GlideSpeedNow, Move->GetFlightTuning().CruiseSpeed, A * A * (3.f - 2.f * A)), GlideSteering);
		if (GlideLevelTime >= GlideHandBackSeconds) { EndGlide(TEXT("handed back")); }
	}
}

void UTDDescentSubsystem::RecordDeparture(ETDDestination Planet, const FTransform& CourseLocalDeparture)
{
	DepartPlanet = Planet;
	DepartPoint = FTDPlanetEntry::GlobeFromDeparture(CourseLocalDeparture, DepartGlobeDir);
	bDepartPending = true;
	UE_LOG(LogTriadDrift, Log, TEXT("TD descent: departure recorded at lat %.1f lon %.1f"), DepartPoint.LatDeg, DepartPoint.LonDeg);
}

bool UTDDescentSubsystem::PlaceDeparture(UWorld* World)
{
	if (!bDepartPending || !World) { return false; }
	const ATDPlanetAtmosphere* Atmo = nullptr;
	for (TActorIterator<ATDPlanetAtmosphere> It(World); It; ++It) { if (It->GetDestination() == DepartPlanet) { Atmo = *It; break; } }
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	ATDShip* Flown = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
	if (!Atmo || !Flown) { return false; } // the galaxy's squad isn't up yet: next frame
	FVector WorldDir = FVector::ForwardVector;
	const FVector OnShell = FTDPlanetEntry::WorldFromGlobe(Atmo, DepartPoint, DepartGlobeDir, WorldDir);
	const FVector Centre = Atmo->GetActorLocation();
	const FVector Out = (OnShell - Centre).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const FVector At = Centre + Out * Atmo->GetAtmosphereRadius() * DepartureRadiusScale;
	// Heading: the climb's world direction, never back into the globe.
	FVector Head = WorldDir.GetSafeNormal(UE_SMALL_NUMBER, Out);
	if (FVector::DotProduct(Head, Out) < 0.2f) { Head = (Head + Out).GetSafeNormal(UE_SMALL_NUMBER, Out); }
	const FVector Shift = At - Flown->GetActorLocation();
	for (TActorIterator<ATDShip> It(World); It; ++It)
	{
		if (It->GetTeam() != Flown->GetTeam()) { continue; } // the squad only
		It->TeleportSpacecraft(It->GetActorLocation() + Shift, Head.Rotation(), /*bKeepSpeed*/ true);
	}
	bDepartPending = false;
	Mark(*FString::Printf(TEXT("departure placed at lat %.1f lon %.1f, %.0f m from the planet"), DepartPoint.LatDeg, DepartPoint.LonDeg, (At - Centre).Size() / 100.f));
	return true;
}

void UTDDescentSubsystem::TickArrivalFloor()
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World || FloorCheckedWorld.Get() == World || !World->HasBegunPlay()) { return; }
	ATDPlanetTerrain* Terrain = nullptr;
	for (TActorIterator<ATDPlanetTerrain> It(World); It; ++It) { Terrain = *It; break; }
	if (!Terrain) { FloorCheckedWorld = World; return; } // not a planet
	if (CoverPhase != ETDDescentCover::Off) { return; } // a covered arrival settles itself (over the deck); look after the reveal
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	ATDShip* Flown = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
	if (!Flown) { return; } // on foot or not up yet: nothing to lift (checked again next frame)
	FloorCheckedWorld = World;
	if (IsGliding() || bArrivedOverDeck) { return; }
	const UTDLandingComponent* Landing = Flown->GetLanding();
	if (Landing && Landing->IsBusy()) { return; } // parked or landing: the pad holds it
	const float Ground = Terrain->GetHeightAt(Flown->GetActorLocation());
	const float Under = Ground + ArrivalFloorCm - Flown->GetActorLocation().Z;
	if (Under <= 0.f) { return; }
	for (TActorIterator<ATDShip> It(World); It; ++It)
	{
		const UTDLandingComponent* L = It->GetLanding();
		if (L && L->IsBusy()) { continue; }
		It->TeleportSpacecraft(It->GetActorLocation() + FVector(0.f, 0.f, Under), It->GetActorRotation(), /*bKeepSpeed*/ true);
	}
	++ArrivalFloorLifts;
	UE_LOG(LogTriadDrift, Log, TEXT("TD descent: arrival lifted %.0f m to %.0f m above the ground (uncovered arrival below the floor)"), Under / 100.f, ArrivalFloorCm / 100.f);
}

void UTDDescentSubsystem::TickGlideWind(float Dt)
{
	// In over 1 s as the roar fades at the reveal (the roar's own 1 s fade), out over the level-off: the two hand over.
	const float Want = IsGliding() ? (bGlideLevelling ? 1.f - FMath::Clamp(GlideLevelTime / GlideHandBackSeconds, 0.f, 1.f) : FMath::Clamp(GlideTime / 1.f, 0.f, 1.f)) : 0.f;
	GlideWindVolume = Want;
	if (Want <= 0.f)
	{
		if (GlideWind) { GlideWind->FadeOut(0.3f, 0.f); GlideWind = nullptr; }
		return;
	}
	if (!GlideWind)
	{
		UWorld* World = GlideShip.IsValid() ? GlideShip->GetWorld() : nullptr;
		const UTDTravelFX* FX = UTDTravelSubsystem::GetDefaultTravelFX(World);
		USoundBase* Sound = FX && !FX->GlideWind.Sound.IsNull() ? FX->GlideWind.Sound.LoadSynchronous() : nullptr;
		if (!Sound)
		{
			// The effects builder's loop while DA_Travel_FX leaves the slot empty; null-safe.
			static const FSoftObjectPath Default(TEXT("/Game/TriadDrift/Gameplay/Audio/Travel/SW_Travel_GlideWind_01.SW_Travel_GlideWind_01"));
			Sound = Cast<USoundBase>(Default.TryLoad());
		}
		if (!Sound || !World) { return; }
		GlideWind = UGameplayStatics::CreateSound2D(World, Sound, 1.f, 1.f, 0.f, nullptr, false, false);
		if (GlideWind) { GlideWind->Play(); }
	}
	if (GlideWind) { GlideWind->SetVolumeMultiplier(FMath::Max(Want, 0.001f)); }
}

void UTDDescentSubsystem::TickExposureRamp()
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	ATDShip* Ship = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
	UCameraComponent* Cam = Ship ? Ship->FindComponentByClass<UCameraComponent>() : nullptr;
	float Want = FMath::Clamp(CoverAlpha, 0.f, 1.f);
	if (Ship && UTDSettingsSubsystem::IsReduceMotion(Ship)) { Want *= 0.5f; } // a gentler flash
	// Off: take the blendable away (no cost).
	if (Want <= 0.f || !Cam)
	{
		if (UCameraComponent* Old = ExposureCamera.Get()) { if (ExposureMID) { Old->PostProcessSettings.RemoveBlendable(ExposureMID); } }
		ExposureCamera.Reset();
		ExposureWeight = 0.f;
		return;
	}
	const UTDTravelFX* FX = UTDTravelSubsystem::GetDefaultTravelFX(World);
	if (!ExposureMID)
	{
		UMaterialInterface* Mat = FX && !FX->DescentWhitePP.IsNull() ? FX->DescentWhitePP.LoadSynchronous() : nullptr;
		if (!Mat)
		{
			// The effects builder's material (eng/vfx 71e34a27) while DA_Travel_FX leaves the slot empty; null-safe.
			static const FSoftObjectPath Default(TEXT("/Game/TriadDrift/Gameplay/VFX/Materials/M_TD_PP_DescentWhite.M_TD_PP_DescentWhite"));
			Mat = Cast<UMaterialInterface>(Default.TryLoad());
		}
		if (!Mat) { ExposureWeight = Want; return; } // no material yet: the weight is still reported (tests, the log)
		ExposureMID = UMaterialInstanceDynamic::Create(Mat, this);
	}
	if (ExposureCamera.Get() != Cam)
	{
		if (UCameraComponent* Old = ExposureCamera.Get()) { Old->PostProcessSettings.RemoveBlendable(ExposureMID); }
		Cam->PostProcessSettings.AddBlendable(ExposureMID, 1.f);
		ExposureCamera = Cam;
	}
	ExposureMID->SetScalarParameterValue(TEXT("Weight"), Want);
	ExposureWeight = Want;
}

bool UTDDescentSubsystem::IsDirectorShotEnabled() { return CVarTDDescentDirectorShot.GetValueOnGameThread() != 0; }

FTransform UTDDescentSubsystem::BurnShotCamera(const ATDShip* Ship, bool bExit, float Alpha, bool bReducedMotion)
{
	// Scaled to the hull (the squad ships are being rebuilt; bounds, not a fixed size): 25 m ahead, 18 m to the side,
	// 6 m below for a ship ~14 m long. The climb out mirrors it (the other side, above, the sky behind).
	const FBox Hull = UTDLandingComponent::VisibleHullBoxLocal(Ship);
	const float Scale = Hull.IsValid ? FMath::Clamp((float)Hull.GetExtent().Size() / 700.f, 0.6f, 3.f) : 1.f;
	const FVector Base = bExit ? FVector(2500.f, -1800.f, 600.f) : FVector(2500.f, 1800.f, -600.f);
	const float Push = bReducedMotion ? 1.f : FMath::Lerp(1.f, 0.85f, FMath::Clamp(Alpha, 0.f, 1.f)); // slow push-in
	const FVector Loc = Base * Scale * Push;
	const FVector Look = (Hull.IsValid ? Hull.GetCenter() : FVector::ZeroVector) - Loc;
	return FTransform(Look.Rotation(), Loc);
}

bool UTDDescentSubsystem::BeginBurnShot(ATDShip* Ship, bool bExit, float Seconds)
{
	if (!Ship || !IsDirectorShotEnabled()) { return false; }
	const bool bReduced = UTDSettingsSubsystem::IsReduceMotion(Ship);
	TWeakObjectPtr<ATDShip> Weak = Ship;
	const bool bOk = UTDDirectorSubsystem::RequestAtmosphereShot(Ship, Ship, bExit, Seconds,
		[Weak, bExit, bReduced](float Alpha) { return BurnShotCamera(Weak.Get(), bExit, Alpha, bReduced); });
	bBurnShot = bOk;
	if (bOk) { Mark(bExit ? TEXT("director shot (climb out)") : TEXT("director shot (entry)")); }
	return bOk;
}

void UTDDescentSubsystem::EndBurnShot()
{
	if (!bBurnShot) { return; }
	bBurnShot = false;
	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		if (UTDDirectorSubsystem::IsAtmosphereShotActive(World)) { UTDDirectorSubsystem::EndAtmosphereShot(World, true); }
	}
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

USoundBase* UTDDescentSubsystem::ResolveRoar(const UTDTravelFX* FX, bool bExit)
{
	if (!FX) { return nullptr; }
	if (bExit && !FX->AtmosphericExit.Sound.IsNull()) { return FX->AtmosphericExit.Sound.LoadSynchronous(); }
	if (!FX->DescentRoar.Sound.IsNull()) { return FX->DescentRoar.Sound.LoadSynchronous(); }
	return FX->AtmosphericEntry.Sound.IsNull() ? nullptr : FX->AtmosphericEntry.Sound.LoadSynchronous();
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
