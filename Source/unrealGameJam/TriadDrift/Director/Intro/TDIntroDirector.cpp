// Triad Drift - ENG-33 the cinematic intro. Owner: Teammate 3.

#include "TriadDrift/Director/Intro/TDIntroDirector.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Input/TDTutorialMusicSubsystem.h"

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
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
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
#include "UObject/StrongObjectPtr.h"
#include "Engine/Texture2D.h"
#include "Widgets/SLeafWidget.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/UI/TDUIHostSettings.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDPortalGate.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Director/TDCutPresentation.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"

#define LOCTEXT_NAMESPACE "TDIntro"

static TAutoConsoleVariable<int32> CVarTDIntro(
	TEXT("td.Intro"), 1,
	TEXT("ENG-33 cinematic intro on New Game: 1 = play (default), 0 = never."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTDIntroEclipseEV(
	TEXT("td.Intro.EclipseEV"), -3.5f,
	TEXT("Intro shot 2 (Verdance in eclipse): the exposure bias (stops; the galaxy's exposure is manual). Lower = darker."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTDIntroSunFromLight(
	TEXT("td.Intro.SunFromLight"), 0,
	TEXT("Intro eclipse: 0 = the sky's sun (yaw 35, pitch 18), 1 = the brightest directional light in the level."),
	ECVF_Default);

namespace TDIntro
{
	/** The intro poses squad ships on purpose: tell T1's silent-move watchdog before each pose (something else, e.g. a
	 *  retake teleport, may have cleared the flag the hold set). */
	static void AnnounceMove(ATDShip* Ship)
	{
		if (UTDShipMovementComponent* Move = Ship ? Ship->GetShipMovement() : nullptr)
		{
			Move->SetMoveAnnounced(true);
		}
	}
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

	// ---- T4's showcase hooks (World / Waves): the real ships, the real Dominion, the real cues. Each falls back to a
	// visual-only stand-in here if its hook has nothing to give (an unknown id, the Dominion mid-assault). ----
	static void Flash(UWorld* World, const FVector& Where, float Radius, const FLinearColor& Color, float Seconds)
	{
		if (UTDProjectileSubsystem* P = World ? UTDProjectileSubsystem::Get(World) : nullptr)
		{
			P->PlayPlaceholderFlash(Where, Radius, Color, Seconds, nullptr);
		}
	}

	static AActor* SpawnFighterStandIn(UWorld* World, const FTransform& At);

	static AActor* SpawnFighter(UWorld* World, const FTransform& At)
	{
		static int32 Variant = 0;
		const FName Id = (Variant++ % 3 == 2) ? TDEnemyIds::HookInterceptor : TDEnemyIds::ShardFighter;
		if (AActor* Ship = ATDWaveDirector::SpawnShowcaseShip(World, Id, At))
		{
			Ship->SetActorScale3D(At.GetScale3D());
			return Ship;
		}
		return SpawnFighterStandIn(World, At);
	}

	static AActor* SpawnFighterStandIn(UWorld* World, const FTransform& At)
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
		if (!Dart)
		{
			return nullptr;
		}
		UStaticMeshComponent* Mesh = Dart->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// The Null Armada's own fighters, exactly as the game shows them: mesh and materials from the enemy visuals assets
		// (DA_Enemy_Fighter_Visuals / DA_Enemy_Interceptor_Visuals, Presentation over engineering), so the intro never drifts
		// from the fleet and the meshes are cooked through the visuals. Every third ship is an interceptor.
		static int32 Variant = 0;
		const bool bInterceptor = (Variant++ % 3 == 2);
		if (const UTDEnemyVisuals* V = UTDStatics::LoadPresentation<UTDEnemyVisuals>(bInterceptor ? TEXT("DA_Enemy_Interceptor_Visuals") : TEXT("DA_Enemy_Fighter_Visuals")))
		{
			if (UStaticMesh* Ship = V->Mesh.LoadSynchronous())
			{
				Mesh->SetStaticMesh(Ship);
				for (int32 i = 0; i < V->MaterialOverrides.Num(); ++i)
				{
					if (UMaterialInterface* M = V->MaterialOverrides[i].LoadSynchronous())
					{
						Mesh->SetMaterial(i, M);
					}
				}
				return Dart;
			}
		}
		// No visuals asset: the Rodin fleet by path (Lead, Nov 13).
		if (UStaticMesh* Ship = LoadObject<UStaticMesh>(nullptr, bInterceptor
			? TEXT("/Game/TriadDrift/Gameplay/Ships/NullInterceptor/SM_NullInterceptor.SM_NullInterceptor")
			: TEXT("/Game/TriadDrift/Gameplay/Ships/NullFighter/SM_NullFighter.SM_NullFighter"), nullptr, LOAD_Quiet | LOAD_NoWarn))
		{
			Mesh->SetStaticMesh(Ship);
			return Dart;
		}
		UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
		if (!Cone)
		{
			return Dart;
		}
		Mesh->SetStaticMesh(Cone);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f)); // the cone's tip (+Z) along the actor's +X
		Mesh->SetWorldScale3D(FVector(3.5f, 3.5f, 9.f));
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Dart);
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.9f, 0.12f, 0.04f));
			Mesh->SetMaterial(0, MID);
		}
		return Dart;
	}

	static void WarpFlash(UWorld* World, const FVector& Where, float Size)
	{
		// Nick's EnemyWarpIn cue through T4's hook once it has visual art. Until then T4's placeholder marker is a
		// gameplay readability ring (big, long-lived, camera-facing) that would veil the Dominion in a 135 mm shot:
		// a brief red flash reads better on film.
		const UTDTravelFX* FX = UTDStatics::LoadPresentation<UTDTravelFX>(TEXT("DA_Travel_FX"));
		if (FX && FTDCombatPlaceholders::HasVisualArt(FX->EnemyWarpIn))
		{
			ATDWaveDirector::PlayShowcaseWarpIn(World, Where, Size);
			return;
		}
		Flash(World, Where, Size, FLinearColor(1.f, 0.12f, 0.05f), 0.3f);
	}

	static void GateFlare(AActor* Gate, const FVector& Where)
	{
		if (ATDPortalGate* Portal = Cast<ATDPortalGate>(Gate))
		{
			Portal->PlayTransitFlare(Where);
			return;
		}
		Flash(Gate ? Gate->GetWorld() : nullptr, Where, 2600.f, FLinearColor(0.5f, 0.9f, 1.f), 0.6f);
	}

	static bool ShowDominion(UWorld* World, bool bShow, const FTransform* Override)
	{
		ATDDominion* Dominion = World ? ATDDominion::Get(World) : nullptr;
		return Dominion && Dominion->SetShowcase(bShow, Override); // false: the stand-in from DA_Dominion_Visuals
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
				const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), FMath::RoundToInt(39.f * Scale));
				const float Y = Size.Y - FMath::Max(Bar * 0.5f, 70.f * Scale);
				// A soft dark backing so the line reads over bright space and planets.
				const FVector2D Pad(28.f * Scale, 10.f * Scale);
				if (!S.SubtitleSpeaker.IsEmpty())
				{
					// The opening's lines: "KATO  No engine readings..." in the speaker's colour, like the in-game radio.
					const FSlateFontInfo LineFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FMath::RoundToInt(36.f * Scale));
					const FSlateFontInfo TagFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::RoundToInt(26.f * Scale));
					const FVector2D TagS = Measure->Measure(S.SubtitleSpeaker, TagFont);
					const FVector2D LineS = Measure->Measure(S.Subtitle, LineFont);
					const float Gap = 18.f * Scale;
					const float W = TagS.X + Gap + LineS.X;
					const float H = FMath::Max(TagS.Y, LineS.Y);
					const float X0 = Size.X * 0.5f - W * 0.5f;
					Box(FVector2D(X0, Y - H * 0.5f) - Pad, FVector2D(W, H) + Pad * 2.f, FLinearColor(0.f, 0.f, 0.f, 0.45f * S.SubtitleAlpha), 2);
					FLinearColor TagC = S.SubtitleSpeakerColor;
					TagC.A = S.SubtitleAlpha;
					Text(S.SubtitleSpeaker, TagFont, FVector2D(X0 + TagS.X * 0.5f, Y), TagC, 3);
					Text(S.Subtitle, LineFont, FVector2D(X0 + TagS.X + Gap + LineS.X * 0.5f, Y), FLinearColor(0.97f, 0.96f, 0.92f, S.SubtitleAlpha), 3);
				}
				else
				{
					const FVector2D TS = Measure->Measure(S.Subtitle, Font);
					Box(FVector2D(Size.X * 0.5f, Y) - TS * 0.5f - Pad, TS + Pad * 2.f, FLinearColor(0.f, 0.f, 0.f, 0.45f * S.SubtitleAlpha), 2);
					Text(S.Subtitle, Font, FVector2D(Size.X * 0.5f, Y), FLinearColor(0.97f, 0.96f, 0.92f, S.SubtitleAlpha), 3);
				}
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
			// Pilot name card (issue #49, concept A "cinematic signature"): insignia, a thin accent, a large name, the ship
			// and role, the trait line; the HUD palette per ship. Measured and fitted: inside the title-safe frame, above the
			// subtitle band, clear of the skip prompt.
			if (S.NameCardAlpha > 0.f && !S.NameCard.IsEmpty())
			{
				const FTDIntroCardLayout L = UTDIntroDirector::LayoutNameCard(Size, S.NameCard, S.NameCardRole, S.NameCardTrait,
					[&Measure](const FText& T, const FSlateFontInfo& F) { return Measure->Measure(T, F); });
				FLinearColor C = UTDIntroDirector::CardColor(S.NameCardShip);
				C.A = S.NameCardAlpha;
				// A soft dark band behind the text block so it reads over bright nebulae (no hard box).
				Box(FVector2D(L.Insignia.Min.X - 18.f * Scale, L.Name.Min.Y - 10.f * Scale),
					FVector2D(FMath::Max3(L.Name.Max.X, L.Role.Max.X, L.Trait.Max.X) - L.Insignia.Min.X + 46.f * Scale, L.Trait.Max.Y - L.Name.Min.Y + 20.f * Scale),
					FLinearColor(0.f, 0.f, 0.f, 0.32f * S.NameCardAlpha), 2);
				DrawInsignia(Out, Layer + 3, Geo, L.Insignia, S.NameCardShip, C);
				Box(L.Bar.Min, L.Bar.GetSize(), C, 3);
				Text(S.NameCard, FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), L.NameFontSize), L.Name.GetCenter(), FLinearColor(1.f, 1.f, 1.f, S.NameCardAlpha), 3);
				if (!S.NameCardRole.IsEmpty())
				{
					FSlateFontInfo RoleFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), L.RoleFontSize);
					Text(S.NameCardRole, RoleFont, L.Role.GetCenter(), C, 3);
				}
				if (!S.NameCardTrait.IsEmpty())
				{
					Text(S.NameCardTrait, FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), L.TraitFontSize), L.Trait.GetCenter(),
						FLinearColor(0.86f, 0.88f, 0.92f, 0.9f * S.NameCardAlpha), 3);
				}
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

		/** Nick's insignia image when the UI style has one; else a shape in code: a crescent (Rift), a lance chevron
		 *  (Lancer), a hexagon (Bastion). */
		void DrawInsignia(FSlateWindowElementList& Out, int32 L, const FGeometry& Geo, const FBox2D& R, int32 Ship, const FLinearColor& C) const
		{
			const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
			const TSoftObjectPtr<UTexture2D>* Slot = !Style ? nullptr
				: (Ship == 1 ? &Style->InsigniaLancer : (Ship == 2 ? &Style->InsigniaBastion : &Style->InsigniaRift));
			if (Slot && !Slot->IsNull())
			{
				if (UTexture2D* Tex = Slot->LoadSynchronous())
				{
					FSlateBrush& B = Brushes.FindOrAdd(Ship);
					if (B.GetResourceObject() != Tex)
					{
						B.SetResourceObject(Tex);
						B.ImageSize = FVector2D(Tex->GetSizeX(), Tex->GetSizeY());
						Textures.FindOrAdd(Ship) = TStrongObjectPtr<UTexture2D>(Tex);
					}
					FSlateDrawElement::MakeBox(Out, L, Geo.ToPaintGeometry(R.GetSize(), FSlateLayoutTransform(R.Min)), &B, ESlateDrawEffect::None, C);
					return;
				}
			}
			const FVector2D Ctr = R.GetCenter();
			const float Rad = R.GetSize().Y * 0.46f;
			const float W = FMath::Max(1.5f, Rad * 0.14f);
			TArray<FVector2D> P;
			auto Line = [&](const TArray<FVector2D>& Pts)
			{
				FSlateDrawElement::MakeLines(Out, L, Geo.ToPaintGeometry(), Pts, ESlateDrawEffect::None, C, true, W);
			};
			if (Ship == 1)
			{
				// Lancer: a lance chevron pointing forward (up), with the shaft.
				Line({ Ctr + FVector2D(-Rad * 0.8f, Rad * 0.55f), Ctr + FVector2D(0.f, -Rad), Ctr + FVector2D(Rad * 0.8f, Rad * 0.55f) });
				Line({ Ctr + FVector2D(0.f, -Rad * 0.35f), Ctr + FVector2D(0.f, Rad) });
			}
			else if (Ship == 2)
			{
				// Bastion: a hexagon (a shield cell) with a smaller one inside.
				for (const float K : { 1.f, 0.55f })
				{
					P.Reset();
					for (int32 i = 0; i <= 6; ++i)
					{
						const float A = FMath::DegreesToRadians(60.f * i - 90.f);
						P.Add(Ctr + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rad * K);
					}
					Line(P);
				}
			}
			else
			{
				// Rift: a crescent (two arcs meeting at the horns).
				P.Reset();
				for (int32 i = 0; i <= 24; ++i)
				{
					const float A = FMath::DegreesToRadians(-140.f + 280.f * i / 24.f);
					P.Add(Ctr + FVector2D(-FMath::Cos(A), FMath::Sin(A)) * Rad);
				}
				for (int32 i = 24; i >= 0; --i)
				{
					const float A = FMath::DegreesToRadians(-140.f + 280.f * i / 24.f);
					P.Add(Ctr + FVector2D(-FMath::Cos(A) * 0.45f + Rad * 0.002f, FMath::Sin(A) * 0.78f) * Rad + FVector2D(-Rad * 0.18f, 0.f));
				}
				const FVector2D First = P[0]; // a copy: Add(P[0]) would reference storage the Add may reallocate (it crashed)
				P.Add(First);
				Line(P);
			}
		}

	private:
		TWeakObjectPtr<UTDIntroDirector> Model;
		mutable TMap<int32, FSlateBrush> Brushes;
		mutable TMap<int32, TStrongObjectPtr<UTexture2D>> Textures;
	};
}

namespace TDIntroSkip
{
	static TAutoConsoleVariable<float> CVarSkipLock(TEXT("td.Intro.SkipLockSeconds"), -1.f,
		TEXT("Seconds before the intro can be skipped. -1: 10 s (N-060), or 1 s inside automation tests."));
}

namespace TDIntroLines
{
	static TAutoConsoleVariable<int32> CVarLines(TEXT("td.Intro.Lines"), 1,
		TEXT("1: the opening speaks the approved First Contact lines (O_*); 0: the old narrator captions (Intro_N*)."));
}

bool UTDIntroDirector::UseOpeningLines()
{
	return TDIntroLines::CVarLines.GetValueOnGameThread() != 0;
}

const TArray<UTDIntroDirector::FTimedLine>& UTDIntroDirector::OpeningLines()
{
	// Lead (Nov 14): the approved O_* lines in story order over the eight shots (Nebula 0-6, Eclipse 6-12, Squad 12-20,
	// Belt 20-26, Gate 26-32, ThreatFar 32-37.5, ThreatClose 37.5-42, Title 42-48). Words unchanged (Nicholas's).
	static const TArray<FTimedLine> Lines = {
		{ 0.8f, TEXT("O_PatrolSeven") },
		{ 3.6f, TEXT("O_Command_Orders") },
		{ 8.4f, TEXT("O_Wren_HowBad") },
		{ 13.2f, TEXT("O_Kato_StopSaying") },
		{ 16.0f, TEXT("O_Moss_Especially") },
		{ 21.0f, TEXT("O_Wren_BigShip") },
		{ 27.0f, TEXT("O_Kato_NoSignature") },
		{ 32.4f, TEXT("O_Moss_Message") },
		{ 34.2f, TEXT("O_Kato_Handshake") },
		{ 37.8f, TEXT("O_Wren_WhyNotFiring") },
		{ 40.4f, TEXT("O_Kato_Contacts") },
	};
	return Lines;
}

const TArray<FName>& UTDIntroDirector::HandOffLines()
{
	// At the hand-off, as radio: "Evade now!" leads straight into the first lesson (ENG-40: EVASIVE MANEUVERS).
	static const TArray<FName> Lines = { TEXT("O_Command_Disengage"), TEXT("O_Unknown_Dont"), TEXT("O_Wren_Break"), TEXT("O_Command_Evade") };
	return Lines;
}

void UTDIntroDirector::FireHandOffLines()
{
	if (bHandOffLinesFired || !UseOpeningLines())
	{
		return;
	}
	bHandOffLinesFired = true;
	for (const FName& Id : HandOffLines())
	{
		UTDStorySubsystem::Fire(this, Id); // the story queue plays them once the intro lets go (shared "Dont" sound included)
	}
}

float UTDIntroDirector::GetSkipLockSeconds()
{
	const float V = TDIntroSkip::CVarSkipLock.GetValueOnGameThread();
	return V >= 0.f ? V : (GIsAutomationTesting ? 1.f : 10.f);
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

FLinearColor UTDIntroDirector::CardColor(int32 Ship)
{
	switch (Ship)
	{
	case 1: return FLinearColor::FromSRGBColor(FColor(0x79, 0xE5, 0xFF));
	case 2: return FLinearColor::FromSRGBColor(FColor(0xFF, 0xCE, 0x77));
	default: return FLinearColor::FromSRGBColor(FColor(0xF1, 0x8C, 0xFA));
	}
}

int32 UTDIntroDirector::SquadSegment(float T, float ShotDuration)
{
	return FMath::Clamp(FMath::FloorToInt(T / FMath::Max(ShotDuration / 3.f, 0.01f)), 0, 2);
}

FTDIntroCardLayout UTDIntroDirector::LayoutNameCard(const FVector2D& Size, const FText& Name, const FText& Role, const FText& Trait,
	TFunctionRef<FVector2D(const FText&, const FSlateFontInfo&)> Measure)
{
	FTDIntroCardLayout L;
	const float BarFull = FMath::Max(0.f, (Size.Y - Size.X / TDIntro::CinemaAspect) * 0.5f);
	L.Picture = FBox2D(FVector2D(0.f, BarFull), FVector2D(Size.X, Size.Y - BarFull));
	const float Scale = Size.Y / 1080.f; // the overlay's own scale (the subtitle and the skip prompt use it too)

	// The narration line: centred in the lower bar, or just above the frame edge without bars (as OnPaint draws it).
	{
		const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), FMath::RoundToInt(39.f * Scale));
		const float LineH = Measure(FText::FromString(TEXT("Ag")), Font).Y;
		const float Y = Size.Y - FMath::Max(BarFull * 0.5f, 70.f * Scale);
		const float Half = LineH * 0.5f + 10.f * Scale;
		L.SubtitleBand = FBox2D(FVector2D(0.f, Y - Half), FVector2D(Size.X, Y + Half));
	}
	// HOLD TO SKIP, bottom right (same placement as OnPaint).
	{
		const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::RoundToInt(16.f * Scale));
		const FVector2D TS = Measure(LOCTEXT("HoldToSkip", "HOLD TO SKIP"), Font);
		const FVector2D C(Size.X - 70.f * Scale - TS.X * 0.5f, Size.Y - FMath::Max(BarFull * 0.5f, 40.f * Scale));
		L.Skip = FBox2D(C - TS * 0.5f, C + FVector2D(TS.X * 0.5f, TS.Y * 0.6f + 3.f * Scale));
	}
	// Concept A, "cinematic signature": the insignia, a thin accent, then the type hierarchy (a large name, the ship
	// and role in small caps, the trait line under it), left-aligned inside the title-safe 16:9 frame (centred on
	// ultrawide). Too wide for the space to the frame's middle: every line shrinks together.
	const float Frame169 = FMath::Min(Size.X, Size.Y * 16.f / 9.f);
	const float Left = (Size.X - Frame169) * 0.5f + Frame169 * 0.06f;
	int32 NameSize = FMath::Max(8, FMath::RoundToInt(56.f * Scale));
	int32 RoleSize = FMath::Max(8, FMath::RoundToInt(19.f * Scale));
	int32 TraitSize = FMath::Max(8, FMath::RoundToInt(16.f * Scale));
	auto MeasureAll = [&](FVector2D& NS, FVector2D& RS, FVector2D& TS)
	{
		NS = Measure(Name, FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), NameSize));
		RS = Role.IsEmpty() ? FVector2D::ZeroVector : Measure(Role, FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), RoleSize));
		TS = Trait.IsEmpty() ? FVector2D::ZeroVector : Measure(Trait, FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), TraitSize));
	};
	FVector2D NS, RS, TS;
	MeasureAll(NS, RS, TS);
	float Ins = NS.Y * 1.05f;                       // the insignia: as tall as the name line
	float TextX = Left + Ins + 30.f * Scale;        // insignia, 12 px, accent, 16 px, text
	const float MaxW = FMath::Max(40.f, Size.X * 0.5f - TextX);
	const float Widest = FMath::Max3(NS.X, RS.X, TS.X);
	if (Widest > MaxW)
	{
		const float K = MaxW / Widest;
		NameSize = FMath::Max(8, FMath::FloorToInt(NameSize * K));
		RoleSize = FMath::Max(8, FMath::FloorToInt(RoleSize * K));
		TraitSize = FMath::Max(8, FMath::FloorToInt(TraitSize * K));
		MeasureAll(NS, RS, TS);
		Ins = NS.Y * 1.05f;
		TextX = Left + Ins + 30.f * Scale;
	}
	// Stacked upward from just above the subtitle band (and inside the picture): trait, role, name.
	const float Bottom = FMath::Min(L.SubtitleBand.Min.Y, L.Picture.Max.Y) - 18.f * Scale;
	L.Trait = FBox2D(FVector2D(TextX, Bottom - TS.Y), FVector2D(TextX + TS.X, Bottom));
	L.Role = FBox2D(FVector2D(TextX, L.Trait.Min.Y - RS.Y), FVector2D(TextX + RS.X, L.Trait.Min.Y));
	L.Name = FBox2D(FVector2D(TextX, L.Role.Min.Y - NS.Y), FVector2D(TextX + NS.X, L.Role.Min.Y));
	const float AccentX = Left + Ins + 12.f * Scale;
	L.Bar = FBox2D(FVector2D(AccentX, L.Name.Min.Y + NS.Y * 0.12f), FVector2D(AccentX + FMath::Max(1.f, 2.f * Scale), Bottom));
	const float InsTop = L.Name.Min.Y + (NS.Y - Ins) * 0.5f;
	L.Insignia = FBox2D(FVector2D(Left, InsTop), FVector2D(Left + Ins, InsTop + Ins));
	L.NameFontSize = NameSize;
	L.RoleFontSize = RoleSize;
	L.TraitFontSize = TraitSize;
	return L;
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
	// The art direction's sun (the sky sphere's sun: yaw 35, pitch 18), unless the level carries its own light.
	if (CVarTDIntroSunFromLight.GetValueOnGameThread() == 0)
	{
		return FRotator(18.f, 35.f, 0.f).Vector();
	}
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
	if (!bHasViewport && !bAllowWithoutViewport && !bAllowWithoutViewportAlways)
	{
		return false; // headless runs and dedicated tests: nothing to watch
	}

	Player = PC;
	OnFinishedDelegate = MoveTemp(OnFinished);
	// Issue #49: no engine debug text over the film (startup messages, map warnings); restored at Finish.
	if (GEngine && !bScreenMessagesHidden)
	{
		bScreenMessagesHidden = true;
		bWasScreenMessages = GEngine->bEnableOnScreenDebugMessagesDisplay;
		bWasMapWarnings = GEngine->bSuppressMapWarnings;
		GEngine->bEnableOnScreenDebugMessagesDisplay = false;
		GEngine->bSuppressMapWarnings = true;
	}
	bPlaying = true;
	bHandingOff = false;
	bSkipped = false;
	bSkipHeld = false;
	bSkipConsumed = false;
	SkipHeldFor = 0.f;
	Elapsed = 0.f;
	HandOffElapsed = 0.f;
	LastOpeningLine = -1;
	bHandOffLinesFired = false;
	ShotIndex = -1;
	LastNarrationVoice = -1;
	bFleetSpawned = false;
	bGateFlared = false;
	bDominionShown = false;
	bSequenceMode = false;
	bSequenceStarted = false;
	Overlay = FTDIntroOverlayState();

	// Input: one blocking component on top of the stack. It sees every key (for the hold-to-skip) and nothing
	// below it (the ship, the controller) sees any, for exactly as long as the intro plays.
	SkipInput = NewObject<UInputComponent>(PC, TEXT("TDIntroSkip"), RF_Transient);
	SkipInput->bBlockInput = true;
	SkipInput->Priority = 10000;
	{
		FInputKeyBinding Pressed(FInputChord(EKeys::AnyKey), IE_Pressed);
		Pressed.KeyDelegate.BindDelegate(this, &UTDIntroDirector::HandleSkipPressed);
		SkipInput->KeyBindings.Add(Pressed);
		FInputKeyBinding Released(FInputChord(EKeys::AnyKey), IE_Released);
		Released.KeyDelegate.BindDelegate(this, &UTDIntroDirector::HandleSkipReleased);
		SkipInput->KeyBindings.Add(Released);
	}
	SkipKeysHeld.Reset();
	PC->PushInputComponent(SkipInput);
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);

	// T2: every HUD (Nick's space and ground HUDs, the fallback HUD's draw) hides for the intro and comes back by
	// itself after; the pause screen cannot open meanwhile.
	UTDUIHostSubsystem::SetCinematicHiddenIn(this, true);
	// T1: no pause while it plays (registered once for every world; the blocker asks this world's intro).
	static bool bPauseBlockerRegistered = false;
	if (!bPauseBlockerRegistered)
	{
		bPauseBlockerRegistered = true;
		ATDSpacePlayerController::RegisterPauseBlocker(TEXT("Intro"), [](const UObject* Context, FText& OutReason)
		{
			OutReason = LOCTEXT("NoPause", "Intro");
			return UTDIntroDirector::IsIntroPlaying(Context);
		});
	}

	HoldSquad(true);
	BuildShots();
	// The narration's words come from the story table (Intro_N0..N4, storyline.md: the premise); these are fallbacks.
	Narration.Reset();
	for (int32 i = 0; i < NarrationLines().Num(); ++i)
	{
		Narration.Add(UTDStorySubsystem::GetText(FName(*FString::Printf(TEXT("Intro_N%d"), i)), NarrationLines()[i]));
	}

	// Music (presentation slot; the asset carries its own class and volume). T1, issue #24: not while the tutorial
	// music (prelude / loop) owns a New Game's opening; IntroMusic stays for runs without it.
	bIntroMusicSuppressed = !UTDTutorialMusicSubsystem::ShouldIntroPlayIntroMusic(this);
	if (const UTDUIStyle* Style = bIntroMusicSuppressed ? nullptr : UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style")))
	{
		if (USoundBase* Sound = Style->IntroMusic.LoadSynchronous())
		{
			Music = UGameplayStatics::SpawnSound2D(this, Sound);
		}
	}

	EnsureWidget();
	// Presentation override (UTDUIStyle::IntroSequence): Nick's authored Level Sequence plays instead of the code
	// shots, with the same skip, input, HUD, squad and hand-off rules. It owns its framing: no bars of ours.
	ULevelSequence* Sequence = SequenceOverrideForTests.Get();
	if (!Sequence)
	{
		if (const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style")))
		{
			Sequence = Cast<ULevelSequence>(Style->IntroSequence.LoadSynchronous());
		}
	}
	if (Sequence)
	{
		FMovieSceneSequencePlaybackSettings Settings;
		Settings.bDisableMovementInput = true;
		Settings.bDisableLookAtInput = true;
		Settings.bHideHud = true;
		ALevelSequenceActor* OutActor = nullptr;
		SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(World, Sequence, Settings, OutActor);
		SequenceActor = OutActor;
		if (SequencePlayer && SequencePlayer->GetDuration().AsSeconds() < 0.1)
		{
			// Nothing to play (an empty or unfinished sequence): say so and play the code shots.
			UE_LOG(LogTriadDrift, Warning, TEXT("Intro: the presentation sequence did not start (%s has no length); playing the code shots"), *GetNameSafe(Sequence));
			SequencePlayer = nullptr;
			if (SequenceActor)
			{
				SequenceActor->Destroy();
				SequenceActor = nullptr;
			}
		}
		else if (SequencePlayer)
		{
			bSequenceMode = true;
			bSequenceStarted = false;
			Overlay.Black = 0.f;
			Overlay.Bars = 0.f;
			SequencePlayer->Play();
			UE_LOG(LogTriadDrift, Log, TEXT("Intro: playing the presentation sequence %s"), *GetNameSafe(Sequence));
			return true;
		}
		else
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("Intro: the presentation sequence did not start (%s could not be played); playing the code shots"), *GetNameSafe(Sequence));
		}
	}
	StartCodeShots();
	return true;
}

void UTDIntroDirector::StartCodeShots()
{
	UWorld* World = GetWorld();
	APlayerController* PC = GetPlayer();
	if (!World || !PC)
	{
		return;
	}
	bSequenceMode = false;
	Overlay.Bars = 1.f;
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

	if (Camera)
	{
		PC->SetViewTarget(Camera);
	}
	BeginShot(0);
	UE_LOG(LogTriadDrift, Log, TEXT("Intro: playing %d shots, %.1f s, sun toward %s"), Shots.Num(), GetTotalDuration(), *SunDirection().ToCompactString());
}

void UTDIntroDirector::SetSequenceOverrideForTests(ULevelSequence* Sequence)
{
	SequenceOverrideForTests = Sequence;
}

void UTDIntroDirector::BuildShots()
{
	Shots.Reset();
	const ATDGalaxyLayout* Galaxy = ATDGalaxyLayout::Get(this);
	const FTDGalaxyLayoutData Layout = Galaxy ? Galaxy->GetLayout() : ATDGalaxyLayout::MakeDefaultLayout();
	const FVector Up = FVector::UpVector;
	// Anchor on the player's ship where the run really starts (the layout's start when there is no squad yet).
	FTransform Start = Layout.PlayerStart;
	if (const APlayerController* PC = GetPlayer())
	{
		for (const FHeldShip& H : Held)
		{
			if (H.Ship.IsValid() && H.Ship.Get() == PC->GetPawn())
			{
				Start = H.Transform;
			}
		}
	}
	const FVector S0 = Start.GetLocation();
	FVector F0 = Start.GetRotation().GetForwardVector().GetSafeNormal2D();
	if (F0.IsNearlyZero())
	{
		F0 = FVector::ForwardVector;
	}
	const FVector R0 = FVector::CrossProduct(Up, F0).GetSafeNormal();
	const FVector Sun = SunDirection();
	const float FormationGap = 600.f;
	const TArray<FVector> Tight = { FVector::ZeroVector, FVector(-FormationGap, -FormationGap, 120.f), FVector(-FormationGap * 1.2f, FormationGap, -80.f) };

	// 1. Black, then the nebula: locked 35 mm, a very slow push, looking away from the sun.
	{
		FTDIntroShot S;
		S.Id = TEXT("Nebula");
		S.Duration = 6.f;
		// High above the combat plane, looking out and up: nothing but the sky (no ruins or rocks in frame).
		const FVector Away = (-Sun + Up * 0.45f + R0 * 0.3f).GetSafeNormal();
		S.CamFrom = S0 - F0 * 8000.f + Up * 30000.f;
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
		// The sun creeps up to the limb and stays just behind it: a rim of light, never a flare.
		// Silhouetted Verdance; the sun slides out from behind it and breaks over the limb in the last second.
		S.CamFrom = Base + Lat * (R * 0.80f) + Up * (R * 0.12f);
		S.CamTo = Base + Lat * (R * 1.03f) + Up * (R * 0.12f);
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
		// Above the combat plane and its asteroid field: nothing between the lens and the ships.
		const FVector P3 = S0 + Up * 26000.f;
		S.CamFrom = P3 + R0 * 1300.f - Up * 260.f;
		S.CamTo = S.CamFrom + F0 * 600.f;
		S.bSquad = true;
		S.SquadFrom = P3 - F0 * 9000.f;
		S.SquadTo = P3 + F0 * 13400.f; // 2800 cm/s
		S.bTrackSquad = true;
		S.FocalFrom = S.FocalTo = 35.f; // steady frames: a 35 mm hero lens, no handheld (issue #49)
		S.FStop = 2.8f;
		S.Handheld = 0.f;
		S.Narration = 2;
		Shots.Add(S);
	}
	// 4. Threading the Verdance lane: 50 mm drifting along T4's lane-rock tube past the cargo pods spilled a third of the
	// way out (foreground parallax), the squad small ahead on the lane.
	{
		const FTDPlanetSpec* V = Layout.Planets.FindByPredicate([](const FTDPlanetSpec& P) { return P.Destination == ETDDestination::Verdance; });
		const FVector LaneEnd = V ? V->Location : FVector(450000.f, 260000.f, 0.f);
		const FVector LaneDir = (LaneEnd - Layout.CombatZoneCenter).GetSafeNormal();
		const FVector LaneSide = FVector::CrossProduct(LaneDir, Up).GetSafeNormal(); // the side the layout spills the pods
		const FVector Cargo = FMath::Lerp(Layout.CombatZoneCenter, LaneEnd, 0.33f) + LaneSide * 6000.f;
		const FVector OnLane = Cargo - LaneSide * 6000.f;
		FTDIntroShot S;
		S.Id = TEXT("Belt");
		S.Duration = 6.f;
		S.Anchor = Cargo;
		S.CamFrom = OnLane - LaneDir * 14000.f + LaneSide * 3200.f + Up * 700.f;
		S.CamTo = OnLane - LaneDir * 2000.f + LaneSide * 3400.f + Up * 900.f; // the pods slide past just off the lens
		S.bSquad = true;
		S.SquadFrom = OnLane - LaneDir * 2000.f;
		S.SquadTo = OnLane + LaneDir * 22000.f;
		S.bTrackSquad = true;
		S.FocalFrom = S.FocalTo = 50.f;
		S.FStop = 5.6f; // wide: no handheld
		S.Narration = 3;
		Shots.Add(S);
	}
	// 5. Cinder far off, down a gate approach: the squad races along T4's beacon buoys on the gate's own axis and through
	// the ring; 35 mm tracking beside them, the ring framing Cinder.
	{
		const FTDPlanetSpec* Cin = Layout.Planets.FindByPredicate([](const FTDPlanetSpec& P) { return P.Destination == ETDDestination::Cinder; });
		const FVector PC = Cin ? Cin->Location : FVector(-420000.f, 330000.f, 40000.f);
		// The zone gate nearest the line to Cinder.
		FTransform GateT(S0 + (PC - S0).GetSafeNormal() * 20000.f);
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
					GateT = *T;
				}
			}
		}
		const FVector Gate = GateT.GetLocation();
		FVector Axis = GateT.GetRotation().GetForwardVector();
		if (FVector::DotProduct(Axis, PC - Gate) < 0.f)
		{
			Axis = -Axis; // fly the approach toward Cinder
		}
		const FVector Side = FVector::CrossProduct(Up, Axis).GetSafeNormal();
		FTDIntroShot S;
		S.Id = TEXT("Gate");
		S.Duration = 6.f;
		S.Anchor = Gate;
		S.bSquad = true;
		S.SquadFrom = Gate - Axis * 40000.f; // past the buoy pairs at 90, 60 and 30 m out
		S.SquadTo = Gate + Axis * 6000.f;
		S.CamFrom = S.SquadFrom - Axis * 3500.f + Side * 2200.f + Up * 700.f;
		S.CamTo = Gate - Axis * 8000.f + Side * 11000.f + Up * 2000.f; // settles beside the ring, outside it, the whole ring in frame
		S.LookFrom = S.SquadFrom + Axis * 8000.f;
		S.LookTo = Gate + Axis * 9000.f;
		S.FocalFrom = S.FocalTo = 35.f;
		S.FStop = 4.f;
		S.FocusDistance = 4000.f; // wide: no handheld
		Shots.Add(S);
	}
	// 6a. The threat from far away: 135 mm, the Dominion's silhouette sliding across the stars, red warp flashes.
	const FVector Dom = Layout.Dominion.GetLocation();
	const FVector DomFwd = Layout.Dominion.GetRotation().GetForwardVector();
	const FVector DomSide = FVector::CrossProduct(Up, DomFwd).GetSafeNormal();
	const float DomR = DominionRadius();
	// 3/4, low, off the bow: the angle that makes a capital ship loom (the side reads as a slab).
	const FVector Quarter = (DomFwd * 0.75f + DomSide * 0.55f - Up * 0.3f).GetSafeNormal();
	{
		FTDIntroShot S;
		S.Id = TEXT("ThreatFar");
		S.Duration = 5.5f;
		// 135 mm: the whole hull (about 2 radii) across about 60% of the frame.
		const float Dist = DomR * 10.f; // the whole hull in frame, stern inside the edge
		S.CamFrom = Dom + Quarter * Dist;
		S.CamTo = S.CamFrom - Quarter * (Dist * 0.04f) - DomSide * (DomR * 0.5f);
		S.LookFrom = Dom + DomFwd * (DomR * 0.18f);
		S.LookTo = Dom - DomFwd * (DomR * 0.08f);
		S.FocalFrom = 135.f;
		S.FocalTo = 135.f;
		S.FStop = 16.f;
		S.FocusDistance = Dist;
		S.FadeIn = 0.8f;
		S.FadeOut = 0.f;
		S.Narration = 4;
		Shots.Add(S);
	}
	// 6b. Hard cut: 24 mm under the bow, the hull filling the sky, fighters screaming past the lens.
	{
		const FVector Low = (DomFwd * 0.85f + DomSide * 0.25f - Up * 0.45f).GetSafeNormal();
		FTDIntroShot S;
		S.Id = TEXT("ThreatClose");
		S.Duration = 4.5f;
		S.CamFrom = Dom + Low * (DomR * 1.9f);
		S.CamTo = S.CamFrom + DomFwd * 600.f;
		S.LookFrom = Dom + DomFwd * (DomR * 0.6f) + Up * (DomR * 0.1f);
		S.LookTo = Dom + DomFwd * (DomR * 0.4f) + Up * (DomR * 0.15f);
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
		S.CamFrom = S0 - F0 * 2600.f + Up * 420.f + R0 * 300.f;
		S.CamTo = S0 - F0 * 1900.f + Up * 260.f;
		S.LookFrom = S0 + F0 * 2500.f - Up * 150.f;
		S.LookTo = S0 + F0 * 2500.f;
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

	if (S.Id == TEXT("ThreatFar") || S.Id == TEXT("ThreatClose"))
	{
		bDominionShown = bDominionShown || TDIntro::ShowDominion(World, true, nullptr);
		if (!bDominionShown && !DominionStandIn)
		{
			const ATDGalaxyLayout* Galaxy = ATDGalaxyLayout::Get(this);
			SpawnDominionStandIn(Galaxy ? Galaxy->GetLayout().Dominion : ATDGalaxyLayout::MakeDefaultLayout().Dominion);
		}
	}
	RestoreGateSurface();
	RestoreShotHidden();
	if (S.Id == TEXT("Eclipse"))
	{
		// An eclipse needs the night side dark: the sky's fill light (lighting the shadow side for gameplay) is off
		// for this shot, and so are the gates near the planet (a pink ring floating beside it at 135 mm).
		if (const UWorld* W = GetWorld())
		{
			for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
			{
				if (It->GetWorld() == W && It->IsVisible() && !It->IsUsedAsAtmosphereSunLight())
				{
					It->SetVisibility(false);
					ShotHiddenLights.Add(*It);
				}
			}
		}
		if (const ATDGalaxyLayout* Galaxy = ATDGalaxyLayout::Get(this))
		{
			const FTDPlanetSpec* V = Galaxy->GetLayout().Planets.FindByPredicate([](const FTDPlanetSpec& P) { return P.Destination == ETDDestination::Verdance; });
			for (ATDPortalGate* G : Galaxy->GetGateActors())
			{
				if (G && V && !G->IsHidden() && FVector::Dist(G->GetActorLocation(), V->Location) < V->PlanetRadius * 4.f)
				{
					G->SetActorHiddenInGame(true);
					ShotHiddenActors.Add(G);
				}
			}
		}
	}
	if (S.Id == TEXT("Gate"))
	{
		// The ring frames Cinder: hide the gate's membrane for the shot (put back at the next cut).
		const FVector GateAt = S.Anchor;
		if (const ATDGalaxyLayout* Galaxy = ATDGalaxyLayout::Get(this))
		{
			ATDPortalGate* Nearest = nullptr;
			for (ATDPortalGate* G : Galaxy->GetGateActors())
			{
				if (G && (!Nearest || FVector::DistSquared(G->GetActorLocation(), GateAt) < FVector::DistSquared(Nearest->GetActorLocation(), GateAt)))
				{
					Nearest = G;
				}
			}
			TArray<UPrimitiveComponent*> Prims;
			if (Nearest)
			{
				Nearest->GetComponents(Prims);
			}
			for (UPrimitiveComponent* P : Prims)
			{
				if (P && P->GetFName() == TEXT("Surface") && P->IsVisible())
				{
					P->SetVisibility(false);
					HiddenSurface = P;
				}
			}
		}
	}
	if (Camera)
	{
		// The last beat eases into the chase camera: lift the exposure so the ship reads, not a black frame.
		FPostProcessSettings& PP = Camera->GetCameraComponent()->PostProcessSettings;
		PP.bOverride_AutoExposureBias = true;
		// The galaxy's exposure is manual (the sky's post-process), so the bias is the exposure. The eclipse is shot
		// well under it so the night side reads as a silhouette, with the lens flare down (its ghosts sat on the disc).
		const bool bEclipse = S.Id == TEXT("Eclipse");
		PP.LensFlareIntensity = bEclipse ? 0.f : 1.6f;
		// N-071: the three hero frames are locked to their ship; motion blur only smeared the hull's edges and lights (the
		// ship drifts and banks a little inside the frame). Off there, on everywhere else.
		PP.MotionBlurAmount = S.Id == TEXT("Squad") ? 0.f : 0.6f;
		PP.AutoExposureBias = S.Id == TEXT("Title") ? 1.5f : (S.Id.ToString().StartsWith(TEXT("Threat")) ? 0.7f : (bEclipse ? EclipseExposureEV100() : 0.f));
	}
	if (S.Id == TEXT("Title"))
	{
		ClearShowcase();
		// Back where the run starts (black frame): the hand-off flies into the real start.
		for (const FHeldShip& H : Held)
		{
			if (ATDShip* Ship = H.Ship.Get())
			{
				TDIntro::AnnounceMove(Ship);
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
	Elapsed = FMath::Max(Start, GetSkipLockSeconds());
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
		bFleetSpawned = true; // the far wave is out of frame by now
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
		TDIntro::AnnounceMove(Ship); // a pose, not a silent move (T1's watchdog; T7 saw three in BeginShot)
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
				TDIntro::AnnounceMove(Ship); // a pose, not a silent move (T1's watchdog; T7 saw three in BeginShot)
				Ship->SetActorLocation(H.Transform.GetLocation() - FVector(0.f, 0.f, 2.0e6f), false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}

	// The camera move.
	FVector Cam = FMath::Lerp(S.CamFrom, S.CamTo, S.bTrackSquad ? A : E);
	FVector Look = FMath::Lerp(S.LookFrom, S.LookTo, E);
	float Focus = S.FocusDistance > 0.f ? S.FocusDistance : 100000.f;
	if (S.Id == TEXT("Squad") && Held.Num())
	{
		// Issue #49 (Nicholas): one steady shot per ship, cut between them; no back-and-forth swings. Each frame is
		// locked to its ship (a three-quarter hero angle, the ship large and right of frame, room for the card at the
		// left) with a slow push-in; the lens holds its pose while the ship drifts and banks a little inside it.
		const int32 K = SquadSegment(T, S.Duration);
		const float Seg = S.Duration / 3.f;
		const float U = FMath::Clamp((T - K * Seg) / Seg, 0.f, 1.f);
		const ATDShip* Ship = Held.IsValidIndex(K) ? Held[K].Ship.Get() : nullptr;
		const FVector At = Ship ? Ship->GetActorLocation() : SquadCentre;
		// The hull's own size (the largest mesh on the ship; trails, rings of light and FX bounds are ignored), so the ship
		// fills about 45 % of the frame width at 35 mm whatever its class (issue #49 / N-050: large, whole and sharp).
		float R = 0.f;
		if (Ship)
		{
			TInlineComponentArray<UStaticMeshComponent*> Meshes(const_cast<ATDShip*>(Ship));
			for (const UStaticMeshComponent* M : Meshes)
			{
				if (M && M->GetStaticMesh() && M->IsVisible() && M->Bounds.SphereRadius < 6000.f)
				{
					R = FMath::Max(R, M->Bounds.SphereRadius);
				}
			}
		}
		R = R > 0.f ? FMath::Clamp(R, 200.f, 4000.f) : 900.f;
		const FRotator Facing = (S.SquadTo - S.SquadFrom).Rotation();
		const FVector Fwd = Facing.Vector();
		const FVector Right = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);
		const FVector UpV = FRotationMatrix(Facing).GetUnitAxis(EAxis::Z);
		// Wren from ahead and below, Kato in profile, Moss low and heroic.
		const FVector Local[3] = { FVector(2.1f, -1.5f, -0.35f), FVector(1.0f, -2.2f, 0.2f), FVector(2.2f, -1.35f, -0.5f) };
		const float Push = FMath::Lerp(1.f, 0.9f, TDIntro::Smooth(U));
		// 2R across 45 % of a 35 mm frame (39 deg): distance = 2R / (0.45 * 2 tan 19.5 deg), about 6.3 R.
		const float Dist = 6.3f * R * Push;
		Cam = At + (Fwd * Local[K].X + Right * Local[K].Y + UpV * Local[K].Z).GetSafeNormal() * Dist;
		const FVector Dir = (At - Cam).GetSafeNormal();
		const FVector CamRight = FVector::CrossProduct(UpV, Dir).GetSafeNormal();
		Look = At - CamRight * 0.45f * R - UpV * 0.12f * R; // the ship sits right of frame (about 60 %), a little high
		Focus = FVector::Dist(Cam, At);
		LastLook = (Look - Cam).GetSafeNormal();
	}
	else if (S.bTrackSquad && Held.Num())
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
		const FVector Gate = S.Anchor;
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
		if (S.Id == TEXT("ThreatClose") && CloseSpawned < 6 && T > 0.15f + CloseSpawned * 0.45f)
		{
			// The close wave: one at a time out of a warp flash ahead of the lens, screaming past it.
			const FVector Fwd = (S.LookFrom - S.CamFrom).GetSafeNormal();
			const FVector Side = FVector::CrossProduct(FVector::UpVector, Fwd).GetSafeNormal();
			const int32 k = CloseSpawned++;
			const FVector At = Camera->GetActorLocation() + Fwd * 9000.f + Side * ((k % 2 ? 1.f : -1.f) * (650.f + (k % 3) * 300.f)) + FVector::UpVector * ((k % 3) - 1) * 320.f;
			TDIntro::WarpFlash(World, At, 1100.f);
			if (AActor* F = TDIntro::SpawnFighter(World, FTransform((-Fwd).Rotation(), At)))
			{
				F->Tags.Add(TEXT("TDIntroClose"));
				Showcase.Add(F);
			}
		}
		// A red-orange key on the fleet, from the lens side, so the ships read against black.
		if (S.Id == TEXT("ThreatClose"))
		{
			const FVector Fwd = (S.LookFrom - S.CamFrom).GetSafeNormal();
			SetKeyLight(Camera->GetActorLocation() + Fwd * 3500.f + FVector::UpVector * 1800.f, 26000.f, 60.f);
		}
		else
		{
			FVector Centroid = FVector::ZeroVector;
			int32 N = 0;
			for (AActor* F : Showcase)
			{
				if (F)
				{
					Centroid += F->GetActorLocation();
					++N;
				}
			}
			if (N)
			{
				Centroid /= N;
				SetKeyLight(Centroid + (Camera->GetActorLocation() - Centroid).GetSafeNormal() * 15000.f + FVector::UpVector * 6000.f, 90000.f, 80.f);
			}
		}
		// Fly the fleet: each fighter keeps on along its nose.
		const float Dt = LastDt;
		for (int32 k = 0; k < Showcase.Num(); ++k)
		{
			if (AActor* F = Showcase[k])
			{
				const float Speed = F->Tags.Contains(TEXT("TDIntroClose")) ? 4200.f : 6000.f;
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
	// Well in front of the Dominion, a third of the way to the lens: big enough to read at 135 mm.
	const FVector Base = FMath::Lerp(S.LookFrom, S.LookTo, 0.35f) + Toward * (FVector::Dist(S.CamFrom, S.LookFrom) * 0.6f);
	for (int32 k = 0; k < 10; ++k)
	{
		const FVector At = Base + Side * ((k - 4.5f) * 2600.f) + FVector::UpVector * FMath::Sin(k * 1.7f) * 2200.f + Toward * (k % 3) * 2500.f;
		TDIntro::WarpFlash(World, At, 5000.f);
		if (AActor* F = TDIntro::SpawnFighter(World, FTransform(Toward.Rotation(), At, FVector(3.5f))))
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
	CloseSpawned = 0;
	if (DominionStandIn)
	{
		DominionStandIn->Destroy();
		DominionStandIn = nullptr;
	}
	if (KeyLight)
	{
		KeyLight->Destroy();
		KeyLight = nullptr;
	}
	RestoreGateSurface();
	if (bDominionShown)
	{
		TDIntro::ShowDominion(GetWorld(), false, nullptr);
		bDominionShown = false;
	}
}

float UTDIntroDirector::DominionRadius()
{
	const UTDDominionVisuals* V = UTDStatics::LoadPresentation<UTDDominionVisuals>(TEXT("DA_Dominion_Visuals"));
	if (const UStaticMesh* Hull = V ? V->HullMesh.LoadSynchronous() : nullptr)
	{
		return FMath::Max(5000.f, Hull->GetBounds().SphereRadius);
	}
	return 31000.f; // the placeholder hull: 600 x 160 x 100 m
}

AActor* UTDIntroDirector::SpawnDominionVisual(UWorld* World, const FTransform& At)
{
	const UTDDominionVisuals* V = UTDStatics::LoadPresentation<UTDDominionVisuals>(TEXT("DA_Dominion_Visuals"));
	UStaticMesh* Hull = V ? V->HullMesh.LoadSynchronous() : nullptr;
	if (!World || !Hull)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AActor* Ship = World->SpawnActor<AActor>(AActor::StaticClass(), At, Params);
	if (!Ship)
	{
		return nullptr;
	}
	USceneComponent* Root = NewObject<USceneComponent>(Ship, TEXT("Root"));
	Ship->SetRootComponent(Root);
	Root->RegisterComponent();
	Ship->SetActorTransform(At);
	auto Piece = [Ship, Root](const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale)
	{
		if (!Mesh)
		{
			return;
		}
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Ship, Name);
		C->SetupAttachment(Root);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetRelativeLocation(Location);
		C->SetRelativeScale3D(Scale);
		C->RegisterComponent();
	};
	// The same layout as ATDDominion's art path (hull at the pivot, the bridge and trench walls where its
	// placeholders sit for the default hull extent).
	const FVector E(30000.f, 8000.f, 5000.f);
	Piece(TEXT("Hull"), Hull, FVector::ZeroVector, FVector::OneVector);
	Piece(TEXT("Bridge"), V->BridgeMesh.LoadSynchronous(), FVector(-E.X * 0.65f, 0.f, E.Z * 1.6f), FVector::OneVector);
	UStaticMesh* Trench = V->TrenchMesh.LoadSynchronous();
	Piece(TEXT("TrenchL"), Trench, FVector(E.X * 0.1f, -2500.f, E.Z + 1500.f), FVector::OneVector);
	Piece(TEXT("TrenchR"), Trench, FVector(E.X * 0.1f, 2500.f, E.Z + 1500.f), FVector(1.f, -1.f, 1.f));
	return Ship;
}

void UTDIntroDirector::SpawnDominionStandIn(const FTransform& At)
{
	DominionStandIn = SpawnDominionVisual(GetWorld(), At);
}

void UTDIntroDirector::SetKeyLight(const FVector& At, float Radius, float Intensity)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (!KeyLight)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		APointLight* Light = World->SpawnActor<APointLight>(APointLight::StaticClass(), FTransform(At), Params);
		if (!Light)
		{
			return;
		}
		UPointLightComponent* L = Light->PointLightComponent;
		L->SetMobility(EComponentMobility::Movable);
		L->bUseInverseSquaredFalloff = false;
		L->SetLightFalloffExponent(2.f);
		L->SetLightColor(FLinearColor(1.f, 0.38f, 0.14f));
		L->SetCastShadows(false);
		KeyLight = Light;
	}
	KeyLight->SetActorLocation(At);
	if (UPointLightComponent* L = Cast<APointLight>(KeyLight)->PointLightComponent)
	{
		L->SetAttenuationRadius(Radius);
		L->SetIntensity(Intensity);
	}
}

float UTDIntroDirector::EclipseExposureEV100()
{
	return CVarTDIntroEclipseEV.GetValueOnGameThread();
}

void UTDIntroDirector::RestoreShotHidden()
{
	for (const TWeakObjectPtr<UDirectionalLightComponent>& L : ShotHiddenLights)
	{
		if (UDirectionalLightComponent* C = L.Get())
		{
			C->SetVisibility(true);
		}
	}
	ShotHiddenLights.Reset();
	for (const TWeakObjectPtr<AActor>& A : ShotHiddenActors)
	{
		if (AActor* Actor = A.Get())
		{
			Actor->SetActorHiddenInGame(false);
		}
	}
	ShotHiddenActors.Reset();
}

void UTDIntroDirector::RestoreGateSurface()
{
	if (UPrimitiveComponent* P = HiddenSurface.Get())
	{
		P->SetVisibility(true);
	}
	HiddenSurface.Reset();
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

	// The opening's lines (td.Intro.Lines 1): the O_* line whose window holds the intro clock.
	Overlay.SubtitleSpeaker = FText::GetEmpty();
	Overlay.SubtitleLineId = NAME_None;
	if (UseOpeningLines())
	{
		const TArray<FTimedLine>& Lines = OpeningLines();
		Overlay.SubtitleAlpha = 0.f;
		for (int32 i = 0; i < Lines.Num(); ++i)
		{
			FTDTransmission E;
			if (!UTDStorySubsystem::FindEntry(FName(Lines[i].Id), E))
			{
				continue;
			}
			const int32 Words = E.Text.ToString().Len() / 6 + 1;
			const float Next = Lines.IsValidIndex(i + 1) ? Lines[i + 1].Start : Lines[i].Start + 4.5f;
			const float End = FMath::Min(Lines[i].Start + FMath::Clamp(1.2f + 0.36f * Words, 1.8f, 4.6f), Next - 0.25f);
			if (Elapsed >= Lines[i].Start && Elapsed < End)
			{
				Overlay.SubtitleLineId = E.Id;
				Overlay.Subtitle = E.Text;
				Overlay.SubtitleSpeaker = UTDStorySubsystem::SpeakerTag(E.Speaker);
				Overlay.SubtitleSpeakerColor = UTDStorySubsystem::SpeakerColor(E.Speaker);
				const float Fade = 0.25f;
				Overlay.SubtitleAlpha = UTDStorySubsystem::AreSubtitlesShown(this)
					? FMath::Clamp(FMath::Min((Elapsed - Lines[i].Start) / Fade, (End - Elapsed) / Fade), 0.f, 1.f) : 0.f;
				if (LastOpeningLine != i)
				{
					LastOpeningLine = i;
					if (USoundBase* Voice = E.Sound.LoadSynchronous()) // Nicholas's voice when it exists (DA_Story_Transmissions); none of ours
					{
						UGameplayStatics::PlaySound2D(this, Voice);
					}
				}
				break;
			}
		}
	}
	// Narration.
	else if (S.Narration >= 0 && Narration.IsValidIndex(S.Narration))
	{
		Overlay.Subtitle = Narration[S.Narration];
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
		// One card per steady frame (issue #49): in after the cut settles, out before the next cut.
		const float Seg = S.Duration / 3.f;
		const float Windows[3][2] = { { 0.35f, Seg - 0.2f }, { Seg + 0.35f, 2.f * Seg - 0.2f }, { 2.f * Seg + 0.35f, S.Duration - 0.2f } };
		const ATDSquadManager* Squad = ATDSquadManager::Get(this);
		for (int32 i = 0; i < 3; ++i)
		{
			const float Alpha = TDIntro::Span(T, Windows[i][0], Windows[i][1], 0.2f, 0.25f);
			if (Alpha > 0.f)
			{
				const ETDShipId Id = static_cast<ETDShipId>(i);
				const FTDShipSpec Spec = Squad ? Squad->GetSpec(Id) : UTDShipLibrary::GetShipSpec(Id);
				Overlay.NameCard = FText::Format(LOCTEXT("PilotCard", "{0}"), Spec.PilotName);
				const FText Trait = UTDStorySubsystem::PilotTrait(Id);
				Overlay.NameCardRole = FText::FromString(FText::Format(LOCTEXT("PilotRole", "{0}  \u00b7  {1}"), Spec.ShipName, Spec.RoleName).ToString().ToUpper());
				// #49 re-check: the role line already says "INTERCEPTOR"; the trait line opened with it too. Drop the repeat here
				// only (the switch card has no role line above its trait).
				FString TraitText = Trait.ToString();
				const FString Role = Spec.RoleName.ToString().ToUpper();
				if (!Role.IsEmpty() && TraitText.ToUpper().StartsWith(Role))
				{
					TraitText = TraitText.Mid(Role.Len()).TrimStart();
					TraitText.RemoveFromStart(TEXT("·"));
					TraitText = TraitText.TrimStart();
				}
				Overlay.NameCardTrait = TraitText.IsEmpty() ? Trait : FText::FromString(TraitText);
				Overlay.NameCardShip = i;
				Overlay.NameCardColor = UTDIntroDirector::CardColor(i);
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

bool UTDIntroDirector::IsSkipKey(const FKey& Key)
{
	// Real buttons only: AnyKey also reports analog axes and their virtual "stick pushed" buttons (a resting thumb
	// drifting across a dead zone must neither skip nor cancel a skip), and the mouse wheel.
	if (!Key.IsValid() || Key.IsAnalog() || Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D())
	{
		return false;
	}
	const FString Name = Key.GetFName().ToString();
	return !Name.Contains(TEXT("Stick_")) && !Name.Contains(TEXT("MouseScroll")) && !Name.Contains(TEXT("MouseWheel"));
}

void UTDIntroDirector::HandleSkipPressed(FKey Key)
{
	if (!IsSkipKey(Key))
	{
		return;
	}
	if (SkipKeysHeld.IsEmpty())
	{
		SkipHeldFor = 0.f;
	}
	SkipKeysHeld.Add(Key);
	bSkipHeld = true;
}

void UTDIntroDirector::HandleSkipReleased(FKey Key)
{
	SkipKeysHeld.Remove(Key);
	if (SkipKeysHeld.IsEmpty())
	{
		bSkipHeld = false; // only when every held button is up: letting go of one never cancels another
		SkipHeldFor = 0.f;
	}
}

void UTDIntroDirector::UpdateSkip(float RealDelta)
{
	if (bSkipHeld)
	{
		Overlay.SkipHintAlpha = FMath::Min(1.f, Overlay.SkipHintAlpha + RealDelta * 6.f);
		if (Elapsed >= GetSkipLockSeconds())
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
	if (!bPlaying || Elapsed < GetSkipLockSeconds())
	{
		return false;
	}
	bSkipped = true;
	UE_LOG(LogTriadDrift, Log, TEXT("Intro: skipped at %.1f s"), Elapsed);
	// The score is cut to the picture: a skip fades it out over a second (watched, it plays to its own end, which
	// lines up with the hand-off: docs/eng/intro-beats.md).
	if (Music)
	{
		Music->FadeOut(1.f, 0.f);
		Music = nullptr;
	}
	Finish();
	return true;
}

void UTDIntroDirector::BeginHandOff()
{
	bHandingOff = true;
	HandOffElapsed = 0.f;
	// Then the slate (C-03): scene one's card comes up as the lens settles behind the ship, not seconds later
	// with the wave. The real first take's slate does not repeat it.
	if (UTDDirectorSubsystem* Director = UTDDirectorSubsystem::Get(this))
	{
		if (UTDCutPresentation* Cut = Director->GetCutPresentation())
		{
			Cut->PreviewFirstSlate(UTDGameInstance::WaveSceneId(1));
		}
	}
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
	// The intro runs on wall time (the music plays on the audio clock and lands its downbeat on control): the world's
	// undilated, unclamped frame time, so a hitch skips intro frames instead of stretching the intro. DeltaTime is
	// dilated and clamped by the world (MaxUndilatedFrameTime), so it is only the fallback.
	const float Dilation = (World && World->GetWorldSettings()) ? FMath::Max(World->GetWorldSettings()->GetEffectiveTimeDilation(), 0.01f) : 1.f;
	const float RealDt = FMath::Max(0.f, World ? World->DeltaRealTimeSeconds : DeltaTime / Dilation);
	// Smoothing (camera look, handheld, skip hold) keeps a bounded step: a 2 s frame must not overshoot a blend.
	const float Dt = FMath::Min(RealDt, 0.1f);
	LastDt = Dt;
	UpdateSkip(Dt);
	if (!bPlaying)
	{
		return; // skipped
	}
	if (bHandingOff)
	{
		HandOffElapsed += RealDt;
		Overlay.Black = 0.f;
		Overlay.SubtitleAlpha = 0.f;
		Overlay.Bars = bSequenceMode ? 0.f : 1.f - TDIntro::Smooth(HandOffElapsed / HandOffSeconds);
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
	Elapsed += RealDt;
	if (bSequenceMode)
	{
		// Presentation's own Level Sequence: it owns the camera cuts; the hand-off follows its last frame.
		Overlay.Black = 0.f;
		const bool bRunning = SequencePlayer && SequencePlayer->IsPlaying();
		bSequenceStarted = bSequenceStarted || bRunning;
		if (!bSequenceStarted && Elapsed > 0.5f)
		{
			// Never started (empty, broken, unloadable bindings): say so, and play the code shots instead.
			UE_LOG(LogTriadDrift, Warning, TEXT("Intro: the presentation sequence did not start; playing the code shots"));
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
			Elapsed = 0.f;
			StartCodeShots();
			return;
		}
		if (bSequenceStarted && !bRunning)
		{
			BeginHandOff();
		}
		return;
	}
	// A long frame may cross several shots: each is closed at its own last frame and the next begun, in order, so no
	// shot's events are lost or fired out of order; past the last shot the hand-off keeps the surplus time.
	while (Shots.IsValidIndex(ShotIndex) && Elapsed >= ShotStart + Shots[ShotIndex].Duration)
	{
		if (RealDt > 0.1f)
		{
			UpdateShot(Shots[ShotIndex].Duration);
		}
		if (ShotIndex + 1 >= Shots.Num())
		{
			const float Surplus = Elapsed - (ShotStart + Shots[ShotIndex].Duration);
			BeginHandOff();
			HandOffElapsed = FMath::Max(0.f, Surplus);
			Elapsed = GetTotalDuration();
			Overlay.Black = 0.f;
			Overlay.SubtitleAlpha = 0.f;
			Overlay.Bars = bSequenceMode ? 0.f : 1.f - TDIntro::Smooth(HandOffElapsed / HandOffSeconds);
			if (bPlaying && HandOffElapsed >= HandOffSeconds)
			{
				Finish(); // once: Finish ends play, and a later Tick returns at !bPlaying
			}
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
				Move->SetMoveAnnounced(true); // the intro poses the ship on purpose: not a silent move (T1's watchdog)
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
		TDIntro::AnnounceMove(Ship);
		Ship->SetActorTransform(H.Transform, false, nullptr, ETeleportType::TeleportPhysics);
		Ship->SetActorEnableCollision(H.bCollision);
		if (UTDShipMovementComponent* Move = Ship->GetShipMovement())
		{
			Move->SetComponentTickEnabled(H.bMovementTick);
			Move->Velocity = H.Velocity;
			Move->SetMoveAnnounced(false); // back where it was; the watchdog resumes from here
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
	FireHandOffLines(); // watched or skipped, the radio picks up as control returns
	ClearShowcase();
	RestoreShotHidden();
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
	}
	SkipInput = nullptr;
	UTDUIHostSubsystem::SetCinematicHiddenIn(this, false);
	if (Camera)
	{
		Camera->Destroy();
		Camera = nullptr;
	}
	RemoveWidget();
	bPlaying = false;
	bHandingOff = false;
	if (GEngine && bScreenMessagesHidden)
	{
		GEngine->bEnableOnScreenDebugMessagesDisplay = bWasScreenMessages;
		GEngine->bSuppressMapWarnings = bWasMapWarnings;
	}
	bScreenMessagesHidden = false;
	Overlay = FTDIntroOverlayState();
	Overlay.Black = 0.f;
	Overlay.Bars = 0.f;
	UE_LOG(LogTriadDrift, Log, TEXT("Intro: finished (%s) at %.1f s"), bSkipped ? TEXT("skipped") : TEXT("watched"), Elapsed);
	FSimpleDelegate Done = MoveTemp(OnFinishedDelegate);
	OnFinishedDelegate.Unbind();
	Done.ExecuteIfBound();
	OnIntroFinished.Broadcast(/*bPlayed*/ true, bSkipped); // N-039: the control frame
}

void UTDIntroDirector::NotifyRunStartedWithoutIntro()
{
	if (bPlaying)
	{
		return;
	}
	OnIntroFinished.Broadcast(/*bPlayed*/ false, /*bSkipped*/ false);
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
