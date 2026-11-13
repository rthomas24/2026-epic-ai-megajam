// Triad Drift - ENG-33 the cinematic intro. Owner: Teammate 3.

#include "TriadDrift/Director/Intro/TDIntroDirector.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InputComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rendering/DrawElements.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/UI/TDUIHostSettings.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDPortalGate.h"

#define LOCTEXT_NAMESPACE "TDIntro"

static TAutoConsoleVariable<int32> CVarTDIntro(
	TEXT("td.Intro"), 1,
	TEXT("ENG-33 cinematic intro on New Game: 1 = play (default), 0 = never."),
	ECVF_Default);

namespace TDIntro
{
	/** Super 35 width: the focal lengths in the shot list mean what they mean on a cinema camera. */
	static constexpr float SensorWidthMm = 24.89f;
	static constexpr float CinemaAspect = 2.39f;

	static float FovForFocal(float FocalMm)
	{
		return FMath::RadiansToDegrees(2.f * FMath::Atan(SensorWidthMm / (2.f * FMath::Max(FocalMm, 1.f))));
	}

	static float Smooth(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}

	/** In-out alpha for a span inside a shot: fades in over FadeIn after Start, out over FadeOut before End. */
	static float Span(float T, float Start, float End, float FadeIn, float FadeOut)
	{
		if (T < Start || T > End)
		{
			return 0.f;
		}
		const float In = FadeIn > 0.f ? FMath::Clamp((T - Start) / FadeIn, 0.f, 1.f) : 1.f;
		const float Out = FadeOut > 0.f ? FMath::Clamp((End - T) / FadeOut, 0.f, 1.f) : 1.f;
		return FMath::Min(In, Out);
	}

	// ---- T4's showcase hooks (World / Waves). Until they land on main: visual-only stand-ins here. ----
	// Switch TD_INTRO_T4_HOOKS to 1 once ATDDominion::SetShowcase, ATDWaveDirector::SpawnShowcaseShip /
	// PlayShowcaseWarpIn and ATDPortalGate::PlayTransitFlare are on main.
#ifndef TD_INTRO_T4_HOOKS
#define TD_INTRO_T4_HOOKS 0
#endif

	static void Flash(UWorld* World, const FVector& Where, float Radius, const FLinearColor& Color, float Seconds)
	{
		if (UTDProjectileSubsystem* P = World ? UTDProjectileSubsystem::Get(World) : nullptr)
		{
			P->PlayPlaceholderFlash(Where, Radius, Color, Seconds, nullptr);
		}
	}

	static AActor* SpawnFighter(UWorld* World, const FTransform& At)
	{
		if (!World)
		{
			return nullptr;
		}
		// Stand-in: a dark red dart (tip forward). Replaced by the real Null fighter through T4's hook.
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		AStaticMeshActor* Dart = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), At, Params);
		UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
		if (!Dart || !Cone)
		{
			return Dart;
		}
		UStaticMeshComponent* Mesh = Dart->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(Cone);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f)); // the cone's tip (+Z) along the actor's +X
		Mesh->SetWorldScale3D(FVector(3.5f, 3.5f, 9.f));
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Dart);
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.45f, 0.03f, 0.02f));
			Mesh->SetMaterial(0, MID);
		}
		return Dart;
	}

	static void WarpFlash(UWorld* World, const FVector& Where, float Size)
	{
		Flash(World, Where, Size, FLinearColor(1.f, 0.12f, 0.05f), 0.45f);
	}

	static void GateFlare(AActor* Gate, const FVector& Where)
	{
		Flash(Gate ? Gate->GetWorld() : nullptr, Where, 2600.f, FLinearColor(0.5f, 0.9f, 1.f), 0.6f);
	}

	static bool ShowDominion(UWorld* World, bool bShow, const FTransform* Override)
	{
		(void)World; (void)bShow; (void)Override;
	return false; // T4's ATDDominion::SetShowcase; until then shot 6 is the fleet against the stars
	}

	// ---- The overlay: bars, black, subtitles, title, name cards, the skip hint ----
	class STDIntroOverlay : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(STDIntroOverlay) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, UTDIntroDirector* InModel)
		{
			Model = InModel;
			SetVisibility(EVisibility::HitTestInvisible);
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Culling,
			FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override
		{
			const UTDIntroDirector* M = Model.Get();
			if (!M || !M->IsPlaying())
			{
				return Layer;
			}
			const FTDIntroOverlayState& S = M->GetOverlay();
			const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
			const FVector2D Size = Geo.GetLocalSize();
			const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			auto Box = [&](const FVector2D& Pos, const FVector2D& Extent, const FLinearColor& Color, int32 L)
			{
				FSlateDrawElement::MakeBox(Out, Layer + L, Geo.ToPaintGeometry(Extent, FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, Color);
			};
			auto Text = [&](const FText& T, const FSlateFontInfo& Font, const FVector2D& Center, const FLinearColor& Color, int32 L)
			{
				const FVector2D TS = Measure->Measure(T, Font);
				const FVector2D Pos = Center - TS * 0.5f;
				FSlateDrawElement::MakeText(Out, Layer + L, Geo.ToPaintGeometry(TS, FSlateLayoutTransform(Pos + FVector2D(2.f, 2.f))), T, Font,
					ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.7f));
				FSlateDrawElement::MakeText(Out, Layer + L + 1, Geo.ToPaintGeometry(TS, FSlateLayoutTransform(Pos)), T, Font, ESlateDrawEffect::None, Color);
				return TS;
			};

			// Black (fades between beats) under the bars.
			if (S.Black > 0.f)
			{
				Box(FVector2D::ZeroVector, Size, FLinearColor(0.f, 0.f, 0.f, S.Black), 0);
			}
			// 2.39:1 bars.
			const float BarFull = FMath::Max(0.f, (Size.Y - Size.X / CinemaAspect) * 0.5f);
			const float Bar = BarFull * S.Bars;
			if (Bar > 0.5f)
			{
				Box(FVector2D::ZeroVector, FVector2D(Size.X, Bar), FLinearColor::Black, 1);
				Box(FVector2D(0.f, Size.Y - Bar), FVector2D(Size.X, Bar), FLinearColor::Black, 1);
			}
			const float Scale = Size.Y / 1080.f;
			// Subtitle: centred in the lower bar (or just above the frame edge if the bar is thin).
			if (S.SubtitleAlpha > 0.f && !S.Subtitle.IsEmpty())
			{
				const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), FMath::RoundToInt(26.f * Scale));
				const float Y = Size.Y - FMath::Max(Bar * 0.5f, 60.f * Scale);
				Text(S.Subtitle, Font, FVector2D(Size.X * 0.5f, Y), FLinearColor(0.96f, 0.95f, 0.9f, S.SubtitleAlpha), 3);
			}
			// Title.
			if (S.TitleAlpha > 0.f)
			{
				FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::RoundToInt(110.f * Scale));
				Font.LetterSpacing = 400;
				const FVector2D TS = Text(LOCTEXT("Title", "TRIAD DRIFT"), Font, Size * 0.5f, FLinearColor(1.f, 1.f, 1.f, S.TitleAlpha), 5);
				// The squad's three colours under the title.
				const FLinearColor Colors[3] = { FLinearColor(1.f, 0.2f, 0.8f), FLinearColor(0.1f, 0.9f, 1.f), FLinearColor(1.f, 0.65f, 0.1f) };
				const float W = TS.X / 3.f;
				for (int32 i = 0; i < 3; ++i)
				{
					FLinearColor C = Colors[i];
					C.A = S.TitleAlpha;
					Box(FVector2D(Size.X * 0.5f - TS.X * 0.5f + W * i + 6.f, Size.Y * 0.5f + TS.Y * 0.55f), FVector2D(W - 12.f, 6.f * Scale), C, 5);
				}
			}
			// Pilot name card (C-03 #1 style): lower third, left of centre, a squad-colour bar beside it.
			if (S.NameCardAlpha > 0.f && !S.NameCard.IsEmpty())
			{
				const FSlateFontInfo Big = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::RoundToInt(44.f * Scale));
				const FSlateFontInfo Small = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FMath::RoundToInt(20.f * Scale));
				const FVector2D At(Size.X * 0.22f, Size.Y - BarFull - 120.f * Scale);
				const FVector2D NS = Measure->Measure(S.NameCard, Big);
				FLinearColor C = S.NameCardColor;
				C.A = S.NameCardAlpha;
				Box(At - FVector2D(NS.X * 0.5f + 22.f * Scale, NS.Y * 0.5f), FVector2D(7.f * Scale, NS.Y * 1.9f), C, 3);
				Text(S.NameCard, Big, At, FLinearColor(1.f, 1.f, 1.f, S.NameCardAlpha), 3);
				Text(S.NameCardRole, Small, At + FVector2D(0.f, NS.Y * 0.85f), FLinearColor(C.R, C.G, C.B, S.NameCardAlpha), 3);
			}
			// HOLD TO SKIP, bottom right, with its fill.
			if (S.SkipHintAlpha > 0.f)
			{
				const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::RoundToInt(16.f * Scale));
				const FText T = LOCTEXT("HoldToSkip", "HOLD TO SKIP");
				const FVector2D TS = Measure->Measure(T, Font);
				const FVector2D C(Size.X - 70.f * Scale - TS.X * 0.5f, Size.Y - FMath::Max(Bar * 0.5f, 40.f * Scale));
				Text(T, Font, C, FLinearColor(1.f, 1.f, 1.f, 0.85f * S.SkipHintAlpha), 6);
				const FVector2D BarPos(C.X - TS.X * 0.5f, C.Y + TS.Y * 0.6f);
				Box(BarPos, FVector2D(TS.X, 3.f * Scale), FLinearColor(1.f, 1.f, 1.f, 0.25f * S.SkipHintAlpha), 6);
				Box(BarPos, FVector2D(TS.X * FMath::Clamp(S.SkipProgress, 0.f, 1.f), 3.f * Scale), FLinearColor(1.f, 1.f, 1.f, S.SkipHintAlpha), 7);
			}
			return Layer + 8;
		}

	private:
		TWeakObjectPtr<UTDIntroDirector> Model;
	};
}

UTDIntroDirector* UTDIntroDirector::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDIntroDirector>() : nullptr;
}

bool UTDIntroDirector::IsIntroPlaying(const UObject* WorldContext)
{
	const UTDIntroDirector* Intro = Get(WorldContext);
	return Intro && Intro->IsPlaying();
}

const TArray<FText>& UTDIntroDirector::NarrationLines()
{
	// The director framing the story (Director's Cut voice). Original lines.
	static const TArray<FText> Lines = {
		LOCTEXT("N0", "Scene one. Deep space. A quiet frontier, waiting for its story."),
		LOCTEXT("N1", "Verdance. Cinder. Two worlds still keeping their secrets."),
		LOCTEXT("N2", "Our leads: a wing of three. Wren. Kato. Moss."),
		LOCTEXT("N3", "One lead. Any cockpit. Every take counts."),
		LOCTEXT("N4", "And out of the dark... the Null Armada."),
	};
	return Lines;
}

float UTDIntroDirector::GetTotalDuration() const
{
	float Total = 0.f;
	for (const FTDIntroShot& S : Shots)
	{
		Total += S.Duration;
	}
	return Total;
}

APlayerController* UTDIntroDirector::GetPlayer() const
{
	return Player.Get();
}

FVector UTDIntroDirector::SunDirection() const
{
	// Toward the sun: the brightest directional light in the level.
	FVector Best = FVector(0.35f, -0.55f, 0.75f).GetSafeNormal();
	float BestIntensity = -1.f;
	if (const UWorld* World = GetWorld())
	{
		for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
		{
			if (It->GetWorld() == World && It->IsVisible() && It->Intensity > BestIntensity)
			{
				BestIntensity = It->Intensity;
				Best = -It->GetForwardVector();
			}
		}
	}
	return Best;
}

bool UTDIntroDirector::Play(FSimpleDelegate OnFinished)
{
	UWorld* World = GetWorld();
	if (bPlaying || !World || !World->IsGameWorld() || CVarTDIntro.GetValueOnGameThread() == 0)
	{
		return false;
	}
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		return false;
	}
	const bool bHasViewport = GEngine && GEngine->GameViewport && GEngine->GameViewport->GetWorld() == World;
	if (!bHasViewport && !bAllowWithoutViewport)
	{
		return false; // headless runs and dedicated tests: nothing to watch
	}

	Player = PC;
	OnFinishedDelegate = MoveTemp(OnFinished);
	bPlaying = true;
	bHandingOff = false;
	bSkipped = false;
	bSkipHeld = false;
	bSkipConsumed = false;
	SkipHeldFor = 0.f;
	Elapsed = 0.f;
	HandOffElapsed = 0.f;
	ShotIndex = -1;
	LastNarrationVoice = -1;
	bFleetSpawned = false;
	bGateFlared = false;
	bDominionShown = false;
	Overlay = FTDIntroOverlayState();

	// Input: one blocking component on top of the stack. It sees every key (for the hold-to-skip) and nothing
	// below it (the ship, the controller) sees any, for exactly as long as the intro plays.
	SkipInput = NewObject<UInputComponent>(PC, TEXT("TDIntroSkip"), RF_Transient);
	SkipInput->bBlockInput = true;
	SkipInput->Priority = 10000;
	SkipInput->BindKey(EKeys::AnyKey, IE_Pressed, this, &UTDIntroDirector::HandleSkipPressed);
	SkipInput->BindKey(EKeys::AnyKey, IE_Released, this, &UTDIntroDirector::HandleSkipReleased);
	PC->PushInputComponent(SkipInput);
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);

	if (AHUD* HUD = PC->GetHUD())
	{
		bHUDWasShown = HUD->bShowHUD;
		HUD->bShowHUD = false;
	}

	HoldSquad(true);
	BuildShots();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	Camera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, Params);
	if (Camera)
	{
		UCameraComponent* Cam = Camera->GetCameraComponent();
		Cam->bConstrainAspectRatio = false; // our own bars, so the hand-off can ease them out
		FPostProcessSettings& PP = Cam->PostProcessSettings;
		Cam->PostProcessBlendWeight = 1.f;
		PP.bOverride_MotionBlurAmount = true;
		PP.MotionBlurAmount = 0.6f;
		PP.bOverride_MotionBlurMax = true;
		PP.MotionBlurMax = 6.f;
		PP.bOverride_BloomIntensity = true;
		PP.BloomIntensity = 1.4f;
		PP.bOverride_LensFlareIntensity = true;
		PP.LensFlareIntensity = 1.6f;
		PP.bOverride_FilmGrainIntensity = true;
		PP.FilmGrainIntensity = 0.12f;
		PP.bOverride_VignetteIntensity = true;
		PP.VignetteIntensity = 0.45f;
		PP.bOverride_DepthOfFieldSensorWidth = true;
		PP.DepthOfFieldSensorWidth = TDIntro::SensorWidthMm;
		PP.bOverride_DepthOfFieldFstop = true;
		PP.bOverride_DepthOfFieldFocalDistance = true;
		// Exposure: adapt instantly at each cut (camera cut), then hold through the shot.
		PP.bOverride_AutoExposureSpeedUp = true;
		PP.AutoExposureSpeedUp = 0.25f;
		PP.bOverride_AutoExposureSpeedDown = true;
		PP.AutoExposureSpeedDown = 0.25f;
	}

	// Music (presentation slot; the asset carries its own class and volume).
	if (const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style")))
	{
		if (USoundBase* Sound = Style->IntroMusic.LoadSynchronous())
		{
			Music = UGameplayStatics::SpawnSound2D(this, Sound);
		}
	}

	EnsureWidget();
	// Presentation override (UTDUIStyle::IntroSequence): Nick's authored Level Sequence plays instead of the code
	// shots, with the same skip, input, HUD, squad and hand-off rules.
	if (const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style")))
	{
		if (ULevelSequence* Sequence = Cast<ULevelSequence>(Style->IntroSequence.LoadSynchronous()))
		{
			FMovieSceneSequencePlaybackSettings Settings;
			Settings.bDisableMovementInput = true;
			Settings.bDisableLookAtInput = true;
			Settings.bHideHud = true;
			ALevelSequenceActor* OutActor = nullptr;
			SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(World, Sequence, Settings, OutActor);
			SequenceActor = OutActor;
			if (SequencePlayer)
			{
				Overlay.Black = 0.f;
				SequencePlayer->Play();
				UE_LOG(LogTriadDrift, Log, TEXT("Intro: playing the presentation sequence %s"), *GetNameSafe(Sequence));
				return true;
			}
		}
	}
	if (Camera)
	{
		PC->SetViewTarget(Camera);
	}
	BeginShot(0);
	UE_LOG(LogTriadDrift, Log, TEXT("Intro: playing %d shots, %.1f s"), Shots.Num(), GetTotalDuration());
	return true;
}

void UTDIntroDirector::BuildShots()
{
	Shots.Reset();
	const ATDGalaxyLayout* Galaxy = ATDGalaxyLayout::Get(this);
	const FTDGalaxyLayoutData Layout = Galaxy ? Galaxy->GetLayout() : ATDGalaxyLayout::MakeDefaultLayout();
	const FVector Up = FVector::UpVector;
	const FVector S0 = Layout.PlayerStart.GetLocation();
	const FVector F0 = Layout.PlayerStart.GetRotation().GetForwardVector().GetSafeNormal2D();
	const FVector R0 = FVector::CrossProduct(Up, F0).GetSafeNormal();
	const FVector Sun = SunDirection();
	const float FormationGap = 600.f;
	const TArray<FVector> Tight = { FVector::ZeroVector, FVector(-FormationGap, -FormationGap, 120.f), FVector(-FormationGap * 1.2f, FormationGap, -80.f) };

	// 1. Black, then the nebula: locked 35 mm, a very slow push, looking away from the sun.
	{
		FTDIntroShot S;
		S.Id = TEXT("Nebula");
		S.Duration = 6.f;
		const FVector Away = (-Sun + Up * 0.25f + R0 * 0.3f).GetSafeNormal();
		S.CamFrom = S0 - F0 * 8000.f + Up * 1500.f;
		S.CamTo = S.CamFrom + Away * 2500.f;
		S.LookFrom = S.CamFrom + Away * 100000.f;
		S.LookTo = S.LookFrom;
		S.FocalFrom = S.FocalTo = 35.f;
		S.FStop = 11.f;
		S.FadeIn = 2.5f;
		S.FadeOut = 0.5f;
		S.Narration = 0;
		Shots.Add(S);
	}
	// 2. Verdance in eclipse: long lens from the planet's night side, a slow lateral dolly until the sun breaks
	// over the limb.
	{
		const FTDPlanetSpec* V = Layout.Planets.FindByPredicate([](const FTDPlanetSpec& P) { return P.Destination == ETDDestination::Verdance; });
		const FVector PV = V ? V->Location : FVector(450000.f, 260000.f, 0.f);
		const float R = V ? V->PlanetRadius : 60000.f;
		const float D = R * 13.f;
		const FVector Lat = FVector::CrossProduct(Sun, Up).GetSafeNormal();
		const FVector Base = PV - Sun * D;
		FTDIntroShot S;
		S.Id = TEXT("Eclipse");
		S.Duration = 6.f;
		S.CamFrom = Base + Lat * (R * 0.78f) + Up * (R * 0.18f);
		S.CamTo = Base + Lat * (R * 1.06f) + Up * (R * 0.2f);
		S.LookFrom = PV + Lat * (R * 0.35f);
		S.LookTo = PV + Lat * (R * 0.5f);
		S.FocalFrom = 85.f;
		S.FocalTo = 135.f;
		S.FStop = 16.f;
		S.FocusDistance = D;
		S.Narration = 1;
		Shots.Add(S);
	}
	// 3. The squad passes close: 24 mm low angle, whip-pan follow, shallow focus, a name card per pilot.
	{
		FTDIntroShot S;
		S.Id = TEXT("Squad");
		S.Duration = 8.f;
		S.CamFrom = S0 + R0 * 1300.f - Up * 260.f;
		S.CamTo = S.CamFrom + F0 * 600.f;
		S.bSquad = true;
		S.SquadFrom = S0 - F0 * 9000.f;
		S.SquadTo = S0 + F0 * 13400.f; // 2800 cm/s
		S.bTrackSquad = true;
		S.FocalFrom = S.FocalTo = 24.f;
		S.FStop = 2.f;
		S.Handheld = 6.f;
		S.Narration = 2;
		Shots.Add(S);
	}
	// 4. Threading an asteroid field: 50 mm drifting through, the squad small ahead, rocks in the foreground.
	{
		const FTDAsteroidFieldSpec* Field = Layout.AsteroidFields.Num() > 1 ? &Layout.AsteroidFields[1] : (Layout.AsteroidFields.Num() ? &Layout.AsteroidFields[0] : nullptr);
		const FVector C = Field ? Field->Center : S0;
		const float FR = Field ? Field->Radius : 20000.f;
		// The clearest line through the field (camera and squad both), with the most rocks close to it.
		const UInstancedStaticMeshComponent* Rocks = Galaxy ? Galaxy->GetAsteroidCollision() : nullptr;
		FVector BestDir = F0;
		float BestScore = -1.f;
		for (int32 i = 0; i < 16; ++i)
		{
			const FVector Dir = F0.RotateAngleAxis(i * 22.5f, Up);
			const FVector A = C - Dir * FR * 0.8f;
			const FVector B = C + Dir * FR * 0.9f;
			float Score = 0.f;
			bool bBlocked = false;
			if (Rocks)
			{
				for (int32 k = 0; k < Rocks->GetInstanceCount(); ++k)
				{
					FTransform T;
					Rocks->GetInstanceTransform(k, T, true);
					const float Radius = Rocks->GetStaticMesh() ? Rocks->GetStaticMesh()->GetBounds().SphereRadius * T.GetMaximumAxisScale() : 1000.f;
					const float Dist = FMath::PointDistToSegment(T.GetLocation(), A, B) - Radius;
					if (Dist < 900.f)
					{
						bBlocked = true;
						break;
					}
					if (Dist < 6000.f)
					{
						Score += 1.f;
					}
				}
			}
			if (!bBlocked && Score > BestScore)
			{
				BestScore = Score;
				BestDir = Dir;
			}
		}
		FTDIntroShot S;
		S.Id = TEXT("Belt");
		S.Duration = 6.f;
		S.CamFrom = C - BestDir * FR * 0.8f + Up * 300.f;
		S.CamTo = S.CamFrom + BestDir * 9000.f;
		S.bSquad = true;
		S.SquadFrom = S.CamFrom + BestDir * 9000.f;
		S.SquadTo = S.SquadFrom + BestDir * 24000.f;
		S.bTrackSquad = true;
		S.FocalFrom = S.FocalTo = 50.f;
		S.FStop = 5.6f;
		S.Handheld = 2.f;
		S.Narration = 3;
		Shots.Add(S);
	}
	// 5. Cinder far off, beyond a portal gate ring: 35 mm tracking with the squad as it flies through.
	{
		const FTDPlanetSpec* Cin = Layout.Planets.FindByPredicate([](const FTDPlanetSpec& P) { return P.Destination == ETDDestination::Cinder; });
		const FVector PC = Cin ? Cin->Location : FVector(-420000.f, 330000.f, 40000.f);
		// The zone gate nearest the line to Cinder.
		FVector Gate = S0 + (PC - S0).GetSafeNormal() * 20000.f;
		float BestDot = -2.f;
		for (const FTDGatePairSpec& G : Layout.Gates)
		{
			for (const FTransform* T : { &G.A, &G.B })
			{
				const FVector At = T->GetLocation();
				const float Dot = FVector::DotProduct((At - S0).GetSafeNormal(), (PC - S0).GetSafeNormal()) - FVector::Dist(At, S0) / 1.0e6f;
				if (Dot > BestDot && FVector::Dist(At, S0) < 150000.f)
				{
					BestDot = Dot;
					Gate = At;
				}
			}
		}
		const FVector Dir = (PC - Gate).GetSafeNormal();
		const FVector Side = FVector::CrossProduct(Up, Dir).GetSafeNormal();
		FTDIntroShot S;
		S.Id = TEXT("Gate");
		S.Duration = 6.f;
		S.bSquad = true;
		S.SquadFrom = Gate - Dir * 11000.f;
		S.SquadTo = Gate + Dir * 5000.f;
		S.CamFrom = S.SquadFrom - Dir * 2800.f + Side * 1100.f + Up * 420.f;
		S.CamTo = S.SquadTo - Dir * 2800.f + Side * 1100.f + Up * 420.f;
		S.LookFrom = S.SquadFrom + Dir * 6000.f;
		S.LookTo = S.SquadTo + Dir * 6000.f;
		S.FocalFrom = S.FocalTo = 35.f;
		S.FStop = 4.f;
		S.FocusDistance = 3200.f;
		S.Handheld = 3.f;
		Shots.Add(S);
	}
	// 6a. The threat from far away: 135 mm, the Dominion's silhouette sliding across the stars, red warp flashes.
	const FVector Dom = Layout.Dominion.GetLocation();
	const FVector DomFwd = Layout.Dominion.GetRotation().GetForwardVector();
	const FVector DomSide = FVector::CrossProduct(Up, DomFwd).GetSafeNormal();
	const float DomR = 60000.f;
	{
		FTDIntroShot S;
		S.Id = TEXT("ThreatFar");
		S.Duration = 5.5f;
		S.CamFrom = Dom + DomFwd * (DomR * 7.f) + DomSide * (DomR * 1.6f) + Up * (DomR * 0.25f);
		S.CamTo = S.CamFrom - DomFwd * (DomR * 0.35f);
		S.LookFrom = Dom + DomSide * (DomR * 0.9f);
		S.LookTo = Dom - DomSide * (DomR * 0.5f);
		S.FocalFrom = 135.f;
		S.FocalTo = 135.f;
		S.FStop = 16.f;
		S.FocusDistance = DomR * 7.f;
		S.FadeIn = 0.8f;
		S.FadeOut = 0.f;
		S.Narration = 4;
		Shots.Add(S);
	}
	// 6b. Hard cut: 24 mm close, fighters screaming past the lens out of the dark.
	{
		FTDIntroShot S;
		S.Id = TEXT("ThreatClose");
		S.Duration = 4.5f;
		S.CamFrom = Dom + DomFwd * (DomR * 2.6f) + DomSide * 1500.f;
		S.CamTo = S.CamFrom + DomFwd * 500.f;
		S.LookFrom = Dom + DomSide * 4000.f;
		S.LookTo = Dom - DomSide * 2000.f;
		S.FocalFrom = S.FocalTo = 24.f;
		S.FStop = 2.8f;
		S.FocusDistance = 2500.f;
		S.Handheld = 10.f;
		S.FadeIn = 0.f;
		S.FadeOut = 0.15f;
		Shots.Add(S);
	}
	// 7. Black, the title, then the camera eases into the chase camera behind the player's ship.
	{
		FTDIntroShot S;
		S.Id = TEXT("Title");
		S.Duration = 6.f;
		S.CamFrom = S0 - F0 * 2600.f + Up * 700.f + R0 * 300.f;
		S.CamTo = S0 - F0 * 1900.f + Up * 480.f;
		S.LookFrom = S0 + F0 * 3000.f;
		S.LookTo = S0 + F0 * 3000.f;
		S.FocalFrom = 35.f;
		S.FocalTo = 28.f;
		S.FStop = 4.f;
		S.FocusDistance = 2000.f;
		S.FadeIn = 0.f;
		S.FadeOut = 0.f;
		Shots.Add(S);
	}
}

void UTDIntroDirector::BeginShot(int32 Index)
{
	if (!Shots.IsValidIndex(Index))
	{
		return;
	}
	float Start = 0.f;
	for (int32 i = 0; i < Index; ++i)
	{
		Start += Shots[i].Duration;
	}
	ShotIndex = Index;
	ShotStart = Start;
	const FTDIntroShot& S = Shots[Index];
	UWorld* World = GetWorld();

	if (S.Id == TEXT("ThreatFar"))
	{
		bDominionShown = TDIntro::ShowDominion(World, true, nullptr);
	}
	if (S.Id == TEXT("Title"))
	{
		ClearShowcase();
		// Back where the run starts (black frame): the hand-off flies into the real start.
		for (const FHeldShip& H : Held)
		{
			if (ATDShip* Ship = H.Ship.Get())
			{
				Ship->SetActorTransform(H.Transform, false, nullptr, ETeleportType::TeleportPhysics);
				if (UTDShipMovementComponent* Move = Ship->GetShipMovement())
				{
					Move->Velocity = H.Velocity;
				}
			}
		}
	}
	// A cut: let the exposure adapt at once to the new shot.
	if (APlayerController* PC = GetPlayer())
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->SetGameCameraCutThisFrame();
		}
	}
	LastLook = (S.LookFrom - S.CamFrom).GetSafeNormal();
	UpdateShot(0.f);
}

void UTDIntroDirector::DebugJumpToShot(int32 Index)
{
	if (!bPlaying || !Shots.IsValidIndex(Index))
	{
		return;
	}
	if (Index < 5)
	{
		ClearShowcase();
		bFleetSpawned = false;
	}
	bGateFlared = false;
	float Start = 0.f;
	for (int32 i = 0; i < Index; ++i)
	{
		Start += Shots[i].Duration;
	}
	Elapsed = FMath::Max(Start, SkipLockSeconds);
	BeginShot(Index);
}

void UTDIntroDirector::DebugSetTime(float T, bool bFreeze)
{
	if (!bPlaying || Shots.IsEmpty())
	{
		return;
	}
	T = FMath::Clamp(T, 0.f, GetTotalDuration() - 0.01f);
	int32 Index = 0;
	float Start = 0.f;
	while (Index + 1 < Shots.Num() && T >= Start + Shots[Index].Duration)
	{
		Start += Shots[Index].Duration;
		++Index;
	}
	ClearShowcase();
	bFleetSpawned = Index > 5; // the far wave belongs to shot 6a; 6b replays it from its own start
	bGateFlared = false;
	bHandingOff = false;
	Elapsed = Start;
	BeginShot(Index);
	if (Shots[Index].Id == TEXT("ThreatClose"))
	{
		bFleetSpawned = true;
		for (int32 k = 0; k < 10; ++k)
		{
			Showcase.Add(nullptr); // the far wave is out of frame by now: the close wave spawns after it
		}
	}
	const float Step = 1.f / 30.f;
	LastDt = Step;
	while (Elapsed + Step <= T)
	{
		Elapsed += Step;
		UpdateShot(Elapsed - ShotStart);
		UpdateOverlay(Elapsed - ShotStart);
	}
	Elapsed = T;
	UpdateShot(Elapsed - ShotStart);
	UpdateOverlay(Elapsed - ShotStart);
	bDebugFrozen = bFreeze;
}

void UTDIntroDirector::PlaceSquad(const FVector& Centre, const FRotator& Facing, float Time)
{
	const FTDIntroShot& S = Shots[ShotIndex];
	const FVector Velocity = (S.SquadTo - S.SquadFrom) / FMath::Max(S.Duration, 0.1f);
	// Squad shot: a staggered line so each pilot passes the lens in turn; elsewhere a tight wedge.
	const bool bLine = S.Id == TEXT("Squad");
	const FVector Line[3] = { FVector::ZeroVector, FVector(-4000.f, -520.f, 220.f), FVector(-8000.f, 380.f, -140.f) };
	const FVector Wedge[3] = { FVector::ZeroVector, FVector(-650.f, -700.f, 140.f), FVector(-800.f, 720.f, -90.f) };
	int32 i = 0;
	for (const FHeldShip& H : Held)
	{
		ATDShip* Ship = H.Ship.Get();
		if (!Ship || H.bHidden)
		{
			++i;
			continue;
		}
		const FVector Offset = Facing.RotateVector(bLine ? Line[i % 3] : Wedge[i % 3]);
		// A little life: each ship breathes on its own slow curve.
		const FVector Drift(0.f, FMath::Sin(Time * 0.9f + i * 2.1f) * 40.f, FMath::Sin(Time * 1.3f + i) * 30.f);
		const FRotator Bank(0.f, 0.f, FMath::Sin(Time * 0.8f + i * 1.7f) * 6.f);
		Ship->SetActorLocationAndRotation(Centre + Offset + Facing.RotateVector(Drift), (Facing.Quaternion() * Bank.Quaternion()), false, nullptr, ETeleportType::TeleportPhysics);
		if (UTDShipMovementComponent* Move = Ship->GetShipMovement())
		{
			Move->Velocity = Velocity; // engines and trails read the speed
		}
		++i;
	}
}

void UTDIntroDirector::UpdateShot(float T)
{
	if (!Shots.IsValidIndex(ShotIndex) || !Camera)
	{
		return;
	}
	const FTDIntroShot& S = Shots[ShotIndex];
	const float A = FMath::Clamp(T / FMath::Max(S.Duration, 0.01f), 0.f, 1.f);
	const float E = TDIntro::Smooth(A);
	UWorld* World = GetWorld();

	// The squad.
	FVector SquadCentre = FVector::ZeroVector;
	if (S.bSquad)
	{
		SquadCentre = FMath::Lerp(S.SquadFrom, S.SquadTo, A); // constant speed: ships do not ease
		PlaceSquad(SquadCentre, (S.SquadTo - S.SquadFrom).Rotation(), Elapsed);
	}
	else if (S.Id != TEXT("Title"))
	{
		// Out of every other shot: park the squad far below the plane, invisible to the lens.
		for (const FHeldShip& H : Held)
		{
			if (ATDShip* Ship = H.Ship.Get())
			{
				Ship->SetActorLocation(H.Transform.GetLocation() - FVector(0.f, 0.f, 2.0e6f), false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}

	// The camera move.
	FVector Cam = FMath::Lerp(S.CamFrom, S.CamTo, S.bTrackSquad ? A : E);
	FVector Look = FMath::Lerp(S.LookFrom, S.LookTo, E);
	float Focus = S.FocusDistance > 0.f ? S.FocusDistance : 100000.f;
	if (S.bTrackSquad && Held.Num())
	{
		// Follow the pilot whose card is up (whip-pan between them), focus on that ship.
		int32 Target = 0;
		if (S.Id == TEXT("Squad"))
		{
			Target = T < 3.9f ? 0 : (T < 5.3f ? 1 : 2);
		}
		const ATDShip* Ship = Held.IsValidIndex(Target) ? Held[Target].Ship.Get() : nullptr;
		const FVector At = Ship ? Ship->GetActorLocation() : SquadCentre;
		const FVector Want = (At - Cam).GetSafeNormal();
		// A whip: a fast critically-damped follow, so the pan snaps to the next ship but never jitters.
		const float Rate = S.Id == TEXT("Squad") ? 7.f : 3.f;
		LastLook = T <= 0.f ? Want : (LastLook + (Want - LastLook) * FMath::Clamp(Rate * LastDt, 0.f, 1.f)).GetSafeNormal();
		Look = Cam + LastLook * 10000.f;
		Focus = FVector::Dist(Cam, At);
	}
	// Handheld noise, close shots only.
	FRotator Shake = FRotator::ZeroRotator;
	if (S.Handheld > 0.f)
	{
		const float N = Elapsed;
		Cam += FVector(FMath::PerlinNoise1D(N * 0.6f + 3.1f), FMath::PerlinNoise1D(N * 0.7f + 7.7f), FMath::PerlinNoise1D(N * 0.5f + 1.3f)) * S.Handheld;
		Shake = FRotator(FMath::PerlinNoise1D(N * 0.9f + 11.f) * 0.25f * S.Handheld / 6.f, FMath::PerlinNoise1D(N * 0.8f + 5.f) * 0.25f * S.Handheld / 6.f, 0.f);
	}
	const FRotator Rot = (Look - Cam).Rotation() + Shake;
	Camera->SetActorLocationAndRotation(Cam, Rot);
	ApplyLens(FMath::Lerp(S.FocalFrom, S.FocalTo, E), S.FStop, Focus);

	// Beats inside shots.
	if (S.Id == TEXT("Gate") && !bGateFlared && Held.Num() && Held[0].Ship.IsValid())
	{
		const FVector Dir = (S.SquadTo - S.SquadFrom).GetSafeNormal();
		const FVector Gate = S.SquadFrom + Dir * 11000.f;
		if (FVector::DotProduct(Held[0].Ship->GetActorLocation() - Gate, Dir) > 0.f)
		{
			bGateFlared = true;
			AActor* GateActor = nullptr;
			if (const ATDGalaxyLayout* Galaxy = ATDGalaxyLayout::Get(this))
			{
				float Best = 1.0e12f;
				for (ATDPortalGate* G : Galaxy->GetGateActors())
				{
					if (G && FVector::DistSquared(G->GetActorLocation(), Gate) < Best)
					{
						Best = FVector::DistSquared(G->GetActorLocation(), Gate);
						GateActor = G;
					}
				}
			}
			TDIntro::GateFlare(GateActor, Gate);
		}
	}
	if ((S.Id == TEXT("ThreatFar") || S.Id == TEXT("ThreatClose")) && World)
	{
		if (!bFleetSpawned && S.Id == TEXT("ThreatFar") && T > 0.9f)
		{
			bFleetSpawned = true;
			SpawnShowcase();
		}
		if (S.Id == TEXT("ThreatClose") && Showcase.Num() < 16 && T > 0.15f && Showcase.Num() >= 10)
		{
			// The close wave: out of the dark ahead of the lens, past it.
			const FVector Fwd = (S.LookFrom - S.CamFrom).GetSafeNormal();
			const FVector Side = FVector::CrossProduct(FVector::UpVector, Fwd).GetSafeNormal();
			for (int32 k = 0; k < 6; ++k)
			{
				const FVector At = S.CamFrom + Fwd * (7000.f + k * 1800.f) + Side * ((k % 2 ? 1.f : -1.f) * (500.f + k * 260.f)) + FVector::UpVector * ((k % 3) - 1) * 380.f;
				TDIntro::WarpFlash(World, At, 1400.f);
				if (AActor* F = TDIntro::SpawnFighter(World, FTransform((-Fwd).Rotation(), At)))
				{
					Showcase.Add(F);
				}
			}
		}
		// Fly the fleet: each fighter keeps on along its nose.
		const float Dt = LastDt;
		for (int32 k = 0; k < Showcase.Num(); ++k)
		{
			if (AActor* F = Showcase[k])
			{
				const float Speed = k < 10 ? 9000.f : 8500.f;
				F->AddActorWorldOffset(F->GetActorForwardVector() * Speed * Dt);
			}
		}
	}
}

void UTDIntroDirector::SpawnShowcase()
{
	UWorld* World = GetWorld();
	if (!World || !Shots.IsValidIndex(ShotIndex))
	{
		return;
	}
	// The far wave: warp flashes in front of the Dominion, fighters pouring toward the lens.
	const FTDIntroShot& S = Shots[ShotIndex];
	const FVector Toward = (S.CamFrom - S.LookFrom).GetSafeNormal();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Toward).GetSafeNormal();
	const FVector Base = FMath::Lerp(S.LookFrom, S.LookTo, 0.35f) + Toward * 70000.f;
	for (int32 k = 0; k < 10; ++k)
	{
		const FVector At = Base + Side * ((k - 4.5f) * 4200.f) + FVector::UpVector * FMath::Sin(k * 1.7f) * 3500.f + Toward * (k % 3) * 3000.f;
		TDIntro::WarpFlash(World, At, 5000.f);
		if (AActor* F = TDIntro::SpawnFighter(World, FTransform(Toward.Rotation(), At, FVector(3.f))))
		{
			Showcase.Add(F);
		}
	}
}

void UTDIntroDirector::ClearShowcase()
{
	for (AActor* A : Showcase)
	{
		if (A)
		{
			A->Destroy();
		}
	}
	Showcase.Reset();
	if (bDominionShown)
	{
		TDIntro::ShowDominion(GetWorld(), false, nullptr);
		bDominionShown = false;
	}
}

void UTDIntroDirector::ApplyLens(float FocalMm, float FStop, float FocusDistance)
{
	if (!Camera)
	{
		return;
	}
	UCameraComponent* Cam = Camera->GetCameraComponent();
	Cam->SetFieldOfView(TDIntro::FovForFocal(FocalMm));
	Cam->PostProcessSettings.DepthOfFieldFstop = FStop;
	Cam->PostProcessSettings.DepthOfFieldFocalDistance = FMath::Max(FocusDistance, 50.f);
}

void UTDIntroDirector::UpdateOverlay(float T)
{
	if (!Shots.IsValidIndex(ShotIndex))
	{
		return;
	}
	const FTDIntroShot& S = Shots[ShotIndex];
	// Black at the edges of a beat.
	float Black = 0.f;
	if (S.FadeIn > 0.f && T < S.FadeIn)
	{
		Black = 1.f - TDIntro::Smooth(T / S.FadeIn);
	}
	if (S.FadeOut > 0.f && T > S.Duration - S.FadeOut)
	{
		Black = FMath::Max(Black, TDIntro::Smooth((T - (S.Duration - S.FadeOut)) / S.FadeOut));
	}
	Overlay.Bars = 1.f;
	Overlay.TitleAlpha = 0.f;
	if (S.Id == TEXT("Title"))
	{
		// 0-2.6 s black with the title; 2.6-3.6 fade up behind the ship; hold; then the hand-off.
		Black = T < 2.6f ? 1.f : 1.f - TDIntro::Smooth((T - 2.6f) / 1.f);
		Overlay.TitleAlpha = TDIntro::Span(T, 0.3f, 2.5f, 0.6f, 0.5f);
	}
	Overlay.Black = Black;

	// Narration.
	if (S.Narration >= 0 && NarrationLines().IsValidIndex(S.Narration))
	{
		Overlay.Subtitle = NarrationLines()[S.Narration];
		const float Start = S.Id == TEXT("Nebula") ? 0.8f : 0.4f;
		Overlay.SubtitleAlpha = TDIntro::Span(T, Start, S.Duration - 0.35f, 0.5f, 0.45f);
		if (LastNarrationVoice != S.Narration && T >= Start)
		{
			LastNarrationVoice = S.Narration;
			PlayNarrationVoice(S.Narration);
		}
	}
	else
	{
		Overlay.SubtitleAlpha = 0.f;
	}

	// Name cards as each pilot passes (C-03 #1).
	Overlay.NameCardAlpha = 0.f;
	if (S.Id == TEXT("Squad"))
	{
		const float Windows[3][2] = { { 2.2f, 3.9f }, { 3.9f, 5.3f }, { 5.3f, 7.4f } };
		const ATDSquadManager* Squad = ATDSquadManager::Get(this);
		for (int32 i = 0; i < 3; ++i)
		{
			const float Alpha = TDIntro::Span(T, Windows[i][0], Windows[i][1], 0.2f, 0.25f);
			if (Alpha > 0.f)
			{
				const ETDShipId Id = static_cast<ETDShipId>(i);
				const FTDShipSpec Spec = Squad ? Squad->GetSpec(Id) : UTDShipLibrary::GetShipSpec(Id);
				Overlay.NameCard = FText::Format(LOCTEXT("PilotCard", "{0}"), Spec.PilotName);
				Overlay.NameCardRole = FText::Format(LOCTEXT("PilotRole", "{0}  ·  {1}"), Spec.ShipName, Spec.RoleName);
				Overlay.NameCardColor = UTDShipLibrary::GetSquadColor(Spec);
				Overlay.NameCardAlpha = Alpha;
			}
		}
	}
}

void UTDIntroDirector::PlayNarrationVoice(int32 Line)
{
	const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	if (Style && Style->IntroNarrationVoice.IsValidIndex(Line))
	{
		if (USoundBase* Voice = Style->IntroNarrationVoice[Line].LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Voice);
		}
	}
}

void UTDIntroDirector::HandleSkipPressed()
{
	bSkipHeld = true;
	SkipHeldFor = 0.f;
}

void UTDIntroDirector::HandleSkipReleased()
{
	bSkipHeld = false;
	SkipHeldFor = 0.f;
}

void UTDIntroDirector::UpdateSkip(float RealDelta)
{
	if (bSkipHeld)
	{
		Overlay.SkipHintAlpha = FMath::Min(1.f, Overlay.SkipHintAlpha + RealDelta * 6.f);
		if (Elapsed >= SkipLockSeconds)
		{
			SkipHeldFor += RealDelta;
		}
		Overlay.SkipProgress = SkipHeldFor / SkipHoldSeconds;
		if (SkipHeldFor >= SkipHoldSeconds)
		{
			Skip();
		}
	}
	else
	{
		Overlay.SkipProgress = 0.f;
		Overlay.SkipHintAlpha = FMath::Max(0.f, Overlay.SkipHintAlpha - RealDelta * 0.8f);
	}
}

bool UTDIntroDirector::Skip()
{
	if (!bPlaying || Elapsed < SkipLockSeconds)
	{
		return false;
	}
	bSkipped = true;
	UE_LOG(LogTriadDrift, Log, TEXT("Intro: skipped at %.1f s"), Elapsed);
	Finish();
	return true;
}

void UTDIntroDirector::BeginHandOff()
{
	bHandingOff = true;
	HandOffElapsed = 0.f;
	if (APlayerController* PC = GetPlayer())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			PC->SetViewTargetWithBlend(Pawn, HandOffSeconds, VTBlend_EaseInOut, 2.f);
		}
	}
}

void UTDIntroDirector::Tick(float DeltaTime)
{
	if (!bPlaying)
	{
		return;
	}
	UWorld* World = GetWorld();
	const float Dilation = (World && World->GetWorldSettings()) ? FMath::Max(World->GetWorldSettings()->GetEffectiveTimeDilation(), 0.01f) : 1.f;
	const float Dt = FMath::Min(DeltaTime / Dilation, 0.1f);
	LastDt = Dt;
	UpdateSkip(Dt);
	if (!bPlaying)
	{
		return; // skipped
	}
	if (bHandingOff)
	{
		HandOffElapsed += Dt;
		Overlay.Black = 0.f;
		Overlay.SubtitleAlpha = 0.f;
		Overlay.Bars = 1.f - TDIntro::Smooth(HandOffElapsed / HandOffSeconds);
		if (HandOffElapsed >= HandOffSeconds)
		{
			Finish();
		}
		return;
	}
	if (bDebugFrozen)
	{
		LastDt = 0.f;
		UpdateShot(Elapsed - ShotStart); // hold the frame (captures)
		return;
	}
	Elapsed += Dt;
	if (SequencePlayer)
	{
		// Presentation's own Level Sequence: it owns the camera cuts; the hand-off follows its last frame.
		Overlay.Black = 0.f;
		if (!SequencePlayer->IsPlaying() && Elapsed > 0.2f)
		{
			BeginHandOff();
		}
		return;
	}
	while (Shots.IsValidIndex(ShotIndex) && Elapsed >= ShotStart + Shots[ShotIndex].Duration)
	{
		if (ShotIndex + 1 >= Shots.Num())
		{
			BeginHandOff();
			return;
		}
		BeginShot(ShotIndex + 1);
	}
	UpdateShot(Elapsed - ShotStart);
	UpdateOverlay(Elapsed - ShotStart);
}

void UTDIntroDirector::HoldSquad(bool bHold)
{
	if (bHold)
	{
		Held.Reset();
		ATDSquadManager* Squad = ATDSquadManager::Get(this);
		if (!Squad)
		{
			return;
		}
		for (ETDShipId Id : UTDShipLibrary::AllShips())
		{
			ATDShip* Ship = Squad->GetShip(Id);
			FHeldShip& H = Held.AddDefaulted_GetRef();
			H.Ship = Ship;
			if (!Ship)
			{
				continue;
			}
			H.Transform = Ship->GetActorTransform();
			H.bCollision = Ship->GetActorEnableCollision();
			H.bHidden = Ship->IsHidden();
			if (UTDShipMovementComponent* Move = Ship->GetShipMovement())
			{
				H.Velocity = Move->Velocity;
				H.bMovementTick = Move->IsComponentTickEnabled();
				Move->SetComponentTickEnabled(false);
			}
			if (AController* C = Ship->GetController(); C && !C->IsA<APlayerController>())
			{
				H.bControllerTick = C->IsActorTickEnabled();
				C->SetActorTickEnabled(false); // wingman AI waits for the first wave
			}
			Ship->SetActorEnableCollision(false); // no gates, triggers or hits while it poses
		}
		return;
	}
	for (const FHeldShip& H : Held)
	{
		ATDShip* Ship = H.Ship.Get();
		if (!Ship)
		{
			continue;
		}
		Ship->SetActorTransform(H.Transform, false, nullptr, ETeleportType::TeleportPhysics);
		Ship->SetActorEnableCollision(H.bCollision);
		if (UTDShipMovementComponent* Move = Ship->GetShipMovement())
		{
			Move->SetComponentTickEnabled(H.bMovementTick);
			Move->Velocity = H.Velocity;
		}
		if (AController* C = Ship->GetController(); C && !C->IsA<APlayerController>())
		{
			C->SetActorTickEnabled(H.bControllerTick);
		}
	}
	Held.Reset();
}

void UTDIntroDirector::Finish()
{
	if (!bPlaying)
	{
		return;
	}
	ClearShowcase();
	if (SequencePlayer)
	{
		SequencePlayer->Stop();
		SequencePlayer = nullptr;
	}
	if (SequenceActor)
	{
		SequenceActor->Destroy();
		SequenceActor = nullptr;
	}
	HoldSquad(false);
	APlayerController* PC = GetPlayer();
	if (PC)
	{
		if (SkipInput)
		{
			PC->PopInputComponent(SkipInput);
		}
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		if (APawn* Pawn = PC->GetPawn())
		{
			if (!bHandingOff || PC->GetViewTarget() != Pawn)
			{
				PC->SetViewTarget(Pawn); // a skip cuts straight to the chase camera
			}
		}
		if (AHUD* HUD = PC->GetHUD())
		{
			HUD->bShowHUD = bHUDWasShown;
		}
	}
	SkipInput = nullptr;
	if (Camera)
	{
		Camera->Destroy();
		Camera = nullptr;
	}
	RemoveWidget();
	bPlaying = false;
	bHandingOff = false;
	Overlay = FTDIntroOverlayState();
	Overlay.Black = 0.f;
	Overlay.Bars = 0.f;
	UE_LOG(LogTriadDrift, Log, TEXT("Intro: finished (%s) at %.1f s"), bSkipped ? TEXT("skipped") : TEXT("watched"), Elapsed);
	FSimpleDelegate Done = MoveTemp(OnFinishedDelegate);
	OnFinishedDelegate.Unbind();
	Done.ExecuteIfBound();
}

void UTDIntroDirector::EnsureWidget()
{
	UWorld* World = GetWorld();
	if (Widget.IsValid() || !World || !GEngine || !GEngine->GameViewport || GEngine->GameViewport->GetWorld() != World)
	{
		return;
	}
	Widget = SNew(TDIntro::STDIntroOverlay, this);
	// Over the HUD and the cut presentation; under nothing in a shipped build.
	GEngine->GameViewport->AddViewportWidgetContent(Widget.ToSharedRef(), UTDUIHostSettings::Get()->ZOrder + 50);
}

void UTDIntroDirector::RemoveWidget()
{
	if (Widget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Widget.ToSharedRef());
	}
	Widget.Reset();
}

void UTDIntroDirector::Deinitialize()
{
	if (bPlaying)
	{
		OnFinishedDelegate.Unbind(); // the world is going away: nothing to start
		Finish();
	}
	Super::Deinitialize();
}

#undef LOCTEXT_NAMESPACE
