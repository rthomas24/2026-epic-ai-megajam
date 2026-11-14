// Triad Drift - engineering space HUD (ENG-36, Teammate 2). Spec: docs/eng/hud-redesign.md. See docs/eng/ui-host.md.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/UI/TDHUDModel.h"
#include "TDFlightHUDWidget.generated.h"

/** ENG-36 colour tokens (spec section 5). Mirrored into UTDUIStyle once that has slots for them, so Nicholas can restyle. */
struct FTDFlightHUDTokens
{
	static FLinearColor Neutral() { return FLinearColor::FromSRGBColor(FColor(0xE8, 0xEE, 0xF7)); }
	static FLinearColor Dim() { return FLinearColor::FromSRGBColor(FColor(0x8A, 0x94, 0xA6)); }
	static FLinearColor Rift() { return FLinearColor::FromSRGBColor(FColor(0xFF, 0x4F, 0xD8)); }
	static FLinearColor Lancer() { return FLinearColor::FromSRGBColor(FColor(0x2F, 0xE6, 0xFF)); }
	static FLinearColor Bastion() { return FLinearColor::FromSRGBColor(FColor(0xFF, 0xB1, 0x3B)); }
	static FLinearColor Shield() { return FLinearColor::FromSRGBColor(FColor(0x9F, 0xD8, 0xFF)); }
	static FLinearColor Warning() { return FLinearColor::FromSRGBColor(FColor(0xFF, 0xE1, 0x4A)); }
	static FLinearColor Danger() { return FLinearColor::FromSRGBColor(FColor(0xFF, 0x4A, 0x3D)); }
	static FLinearColor Plate() { return FLinearColor::FromSRGBColor(FColor(0x0A, 0x0F, 0x1C)); }
	static FLinearColor Ship(ETDShipId Ship)
	{
		return Ship == ETDShipId::Rift ? Rift() : Ship == ETDShipId::Lancer ? Lancer() : Bastion();
	}
	/** Hull colour band: white above 50%, amber to 25%, red below. */
	static FLinearColor Hull(float Hull01)
	{
		return Hull01 > .5f ? Neutral() : Hull01 > .25f ? Bastion() : Danger();
	}
};

/**
 * Engineering's own space HUD (ENG-36): the essentials at the reticle, a compact squad strip, one objective line; the
 * rest contextual. Built in phases (spec section 7). P1, this file: reticle with hull and shield arcs, the signature
 * power pip, the squad strip and the objective line.
 *
 * Painted in one NativePaint in a 1920x1080 design space letterboxed into the viewport (so on ultrawide everything sits
 * in a centred 16:9 frame), like UTDGroundHUDWidget. Every frame it samples its data source (the squad manager, through
 * ITDHUDData) into FView, which is what it draws and what the tests read.
 *
 * Shown by UTDUIHostSubsystem when UTDUIHostSettings picks the engineering HUD (TD.UI.SpaceHUD). Nicholas's widget and
 * the dev HUD stay selectable. It exposes SetDataSource / RefreshData under the same names as his widget, so the host
 * drives both the same way.
 */
/** Game-thread cost of one part of the flight HUD since the last reset (TD.UI.Cost). */
struct FTDHUDCost
{
	uint64 Cycles = 0;
	uint64 Worst = 0;
	int32 Frames = 0;
	double AverageMs() const { return Frames > 0 ? FPlatformTime::ToMilliseconds64(Cycles) / Frames : 0.0; }
	double WorstMs() const { return FPlatformTime::ToMilliseconds64(Worst); }
};

struct FTDHUDCostScope
{
	FTDHUDCost& Cost;
	uint64 Start;
	explicit FTDHUDCostScope(FTDHUDCost& InCost) : Cost(InCost), Start(FPlatformTime::Cycles64()) {}
	~FTDHUDCostScope()
	{
		const uint64 D = FPlatformTime::Cycles64() - Start;
		Cost.Cycles += D;
		Cost.Worst = FMath::Max(Cost.Worst, D);
		++Cost.Frames;
	}
};

UCLASS()
class UNREALGAMEJAM_API UTDFlightHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** One squad chip as drawn. */
	struct FChip
	{
		ETDShipId Ship = ETDShipId::Rift;
		float Hull = 1.f;
		float Shield = 1.f;
		float RespawnSeconds = 0.f;
		bool bActive = false;
		bool bAlive = true;
		/** Alive with hull under TroubleHull: amber pulsing outline and a "!" badge. */
		bool bTrouble = false;
		/** Its signature power is ready (the dot on the chip, so a wingman's ready power is visible before switching). */
		bool bPowerReady = false;
		/** Switch key / button shown on the keycap ("1", "2", "3"; empty on a pad). */
		FString Key;
		/** Drawn position and size in the design space (tests check the layout). */
		FVector2D Pos = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;
	};

	/** How a transmission line is drawn (the story's voices: squad clean, "Unknown" through static, "Older" as an echo). */
	enum class ETransmissionStyle : uint8 { Clean, Static, Echo };

	/** One subtitle / transmission line in the slot under the lower lane. */
	struct FSubtitle
	{
		FString Speaker;
		FString Text;
		ETransmissionStyle Style = ETransmissionStyle::Clean;
		float Age = 0.f;
		float Seconds = 0.f;
		/** The speaker tag's colour (T3's UTDStorySubsystem::SpeakerColor for story lines; the pilot's colour for barks). */
		FLinearColor TagColor = FLinearColor(.54f, .58f, .65f);
		/** A story transmission (from T3's subsystem), not a squad bark. */
		bool bStory = false;
		/** A story line whose transmission says bShowSubtitle false (T3, ENG-45): its text and tag are not drawn. */
		bool bStoryHidden = false;
	};

	/** P2: one hostile or missile as drawn: a bracket / tick on screen, an edge chevron off screen. */
	struct FMark
	{
		bool bOnScreen = false;
		/** Design-space position (on screen) or the chevron's spot on the safe frame (off screen). */
		FVector2D Pos = FVector2D::ZeroVector;
		/** Off screen: the chevron's angle in radians (screen space, 0 = right, +PI/2 = down). */
		float Angle = 0.f;
		bool bCurrent = false;
		bool bLocked = false;
		bool bMissile = false;
		bool bFocus = false;
		/** Current target: bracket size (48..160) and its health 0..1 (-1 = unknown). */
		float Size = 0.f;
		float Health = -1.f;
		/** Metres. */
		float Distance = 0.f;
		/** What it is (KindOf): its marker's outline. */
		uint8 Kind = 0;
		/** Its heading relative to the player: 0 crossing, 1 toward, 2 away; HeadingAngle = its nose on screen (radians). */
		uint8 Heading = 0;
		float HeadingAngle = 0.f;
		/** It is attacking the flown ship (its pilot's target): a brighter, heavier outline. */
		bool bThreat = false;
		/** Its projected radius on screen (design px): the marker sits just above the ship's outline. */
		float ScreenRadius = 0.f;
	};

	/**
	 * Enemy marker shapes (the ship builder's readability pass, via the Lead): at chase-camera range the ships are a few
	 * dozen pixels, so the marker carries the type. 1 fighter = small chevron, 2 interceptor = narrow dart, 3 bomber =
	 * wide wedge, 4 gunship = heavy diamond, 5 carrier = large bracketed hexagon, 0 anything else = the plain triangle.
	 */
	enum : uint8 { KindOther = 0, KindFighter = 1, KindInterceptor = 2, KindBomber = 3, KindGunship = 4, KindCarrier = 5 };
	static uint8 KindOf(const AActor* Actor);
	static const TCHAR* KindName(uint8 Kind);

	/** An objective waypoint (planet, gate, generator, landing pad): a white diamond on screen, an edge diamond off screen. */
	struct FWaypoint
	{
		bool bOnScreen = false;
		FVector2D Pos = FVector2D::ZeroVector;
		float Angle = 0.f;
		/** Metres. */
		float Distance = 0.f;
		/** 0..1 when the marker has health (a Dominion generator), else -1. */
		float Health = -1.f;
		/** On screen but inside the player-ship zone: drawn at DrawPos (just above it) with a tether to Pos. */
		bool bLifted = false;
		FVector2D DrawPos = FVector2D::ZeroVector;
	};

	/** Design-space zone the chase camera's own ship covers (under the reticle cluster). */
	static FBox2D ShipZone() { return FBox2D(FVector2D(790.f, 612.f), FVector2D(1130.f, 800.f)); }
	/** Where an on-screen waypoint at Pos is drawn: just above the ship zone when inside it (returns true), else Pos. */
	static bool LiftWaypoint(const FVector2D& Pos, FVector2D& OutDraw)
	{
		const bool bLift = ShipZone().IsInside(Pos);
		OutDraw = bLift ? FVector2D(Pos.X, ShipZone().Min.Y - 14.f) : Pos;
		return bLift;
	}

	/** P2: a wingman's arrow (off screen only, hollow chevron in its colour; pulses in trouble). */
	struct FWingArrow
	{
		ETDShipId Ship = ETDShipId::Rift;
		FVector2D Pos = FVector2D::ZeroVector;
		float Angle = 0.f;
		bool bTrouble = false;
	};

	/** P2: one hit-direction arc around the reticle. */
	struct FHitArc
	{
		float Angle = 0.f;
		float Opacity = 0.f;
		bool bShieldOnly = false;
	};

	/** What the one lower-lane message is (priority order). */
	enum class ELane : uint8 { None, Prompt, HyperdriveBlocked, Hint };

	/** Snapshot of what the widget draws. Updated every tick from the data source. */
	/** Issue #160, Concept A: one squad card (flown 320x100, wingman 232x76; the order is always 1-2-3). */
	struct FSquadCard
	{
		bool bValid = false;
		ETDShipId Ship = ETDShipId::Rift;
		bool bFlown = false;
		bool bAlive = true;
		float Hull = 1.f;
		float Shield = 1.f;
		/** The hull just lost, draining to Hull over 400 ms (the white ghost segment). */
		float GhostHull = 1.f;
		float Power01 = 1.f;
		bool bPowerActive = false;
		/** None = locked (dashed ring + padlock). */
		ETDAbilityId Ability = ETDAbilityId::None;
		float Ability01 = 1.f;
		bool bAbilityActive = false;
		bool bTrouble = false;
		float UnderFire01 = 0.f;
		/** 0 = wingman size, 1 = flown size (eased over 120 ms on a switch; the input is never blocked). */
		float Grow = 0.f;
		FVector2D Pos = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;
		/** Cached texts, rebuilt only when their number changes (no per-frame formatting). */
		FString HullText, ShieldText, DownText, Key;
		/** Ability cooldown in whole seconds ("7"), empty when ready; overcharge seconds ("OVERCHARGE 6.2"). */
		FString AbilitySecondsText, OverchargeText;
		/** Status flags (step 2): shown as tags above the card, in priority order. */
		bool bCloaked = false;
		bool bDomeUp = false;
		bool bAfterburn = false;
		bool bOvercharge = false;
	};

	struct FView
	{
		/** #160: the squad panel is on (off = the Minimal HUD setting: today's chips). */
		bool bSquadPanel = false;
		/** 0..1 while a director shot runs (eased): the squad panel and radar are out of the shot. */
		float ShotHide = 0.f;
		FSquadCard Cards[3];
		/** Cards' size factor in design units and the text factor (below 900 px: cards 85%, text at its 1080 size). */
		float PanelK = 1.f;
		float PanelFont = 1.f;
		/** ENG-45 / #155: UTDSettingsSubsystem::GetSubtitleScale. */
		float SubtitleScale = 1.f;
		/** Bottom of the subtitle slot (above the squad panel's flag row when the panel is on). */
		float SubtitleBottom = 964.f;
		/** Navigation pulse: 0..1 through it (0 = none), and the corridor's ends in the design frame. */
		float NavPulse = 0.f;
		FVector2D NavFrom = FVector2D::ZeroVector;
		FVector2D NavTo = FVector2D::ZeroVector;
		/** The line in the subtitle slot (a bark or a story transmission); empty Text = nothing shown. */
		FSubtitle Subtitle;
		// ---- Director's Cut moments
		/** The whole HUD's opacity: 0 during a CUT, 0.4 behind a slate, 0.3 paused, fading back in over 0.25 s. */
		float HUDOpacity = 1.f;
		/** 0..1 into a money shot (director slow-mo): everything but the reticle (40%) and the arcs hides. */
		float MoneyShot = 0.f;
		bool bInCut = false;
		bool bInSlate = false;
		bool bPaused = false;
		/** A focus hold from the host (T1's atmosphere entry): combat objectives (pill, waypoints) off screen. */
		bool bCombatHeld = false;
		/**
		 * The Dominion approach marker (storyline / alignment a13): 1 = pending (Cinder's field anchor is down, the field
		 * still closed: a dashed amber diamond), 2 = open (the 8 s after wave 5's clear reveals the Dominion), 0 = none.
		 */
		int32 Approach = 0;
		FWaypoint ApproachWp;
		/** Space pickups (UTDPickupSubsystem) within PickupMarkRange: Type is uint8 ETDPickupType. */
		struct FPickupMark
		{
			uint8 Type = 0;
			FLinearColor Color = FLinearColor::White;
			bool bOnScreen = false;
			FVector2D Pos = FVector2D::ZeroVector;
			float Angle = 0.f;
			float Distance = 0.f;
		};
		TArray<FPickupMark> Pickups;
		/** The collect toast ("+HULL", "SHIELD CELL", "POWER READY", "OVERCHARGE"): text, colour, 0..1 age. */
		FString PickupToast;
		FLinearColor PickupToastColor = FLinearColor::White;
		float PickupToast01 = 0.f;
		/** T6's field: pieces the squad broke this scene, seconds since the last (the DEBRIS tick), a field-hit tick 0..1. */
		int32 DebrisCount = 0;
		float DebrisAge = 1e9f;
		/** A broken asteroid / wreck topped up the boost meter this long ago (the "+BOOST" tick by the boost bar). */
		float BreakBoostAge = 1e9f;
		float FieldHit01 = 0.f;
		/** Overcharge time left 0..1 (the timer ring) and in seconds. */
		float Overcharge01 = 0.f;
		float OverchargeSeconds = 0.f;
		// ---- P2
		TArray<FMark> Marks;
		TArray<FWaypoint> Waypoints;
		bool bHasLead = false;
		FVector2D Lead = FVector2D::ZeroVector;
		TArray<FWingArrow> Wings;
		ETDMissileWarning Missile = ETDMissileWarning::None;
		TArray<FHitArc> Hits;
		/** Low-hull edge glow strength 0..1 (hull under 25%). */
		float Vignette = 0.f;
		/** Boost bar: shown while boosting or refilling, hidden 1.5 s after full. */
		bool bShowBoost = false;
		float Boost = 1.f;
		bool bDodgeReady = true;
		/** Hyperdrive charge bar while charging. */
		float Hyperdrive = 0.f;
		ELane Lane = ELane::None;
		FString LaneText;
		/**
		 * Onboarding card (T3, with T1's UTDHintComponent): the lane's hint drawn as a card with a highlighted key glyph,
		 * the action and a step counter, when the hint getters are in the build; plain LaneText otherwise.
		 */
		bool bHintCard = false;
		FString HintKey;
		FString HintAction;
		int32 HintStep = 0;
		int32 HintCount = 0;
		/** The player just did it: the tick shows, then the card fades (0..1 through the 0.5 s done beat). */
		bool bHintDone = false;
		float HintDone01 = 0.f;
		FString Boundary;
		float SwitchStrike = 0.f;
		/** 0..1 through the reward pulse when a Switch Strike window opens (0 when not pulsing). */
		float SwitchStrikePulse = 0.f;
		/** A focus-fire order: 1 when given, draining to 0. */
		float Focus = 0.f;
		/** Hit feel: 0..1 through the reticle hit tick / kill X (0 when idle), and the kill X at the victim's spot. */
		float HitMark = 0.f;
		float KillMark = 0.f;
		bool bKillAtTarget = false;
		FVector2D KillPos = FVector2D::ZeroVector;
		float KillAtTarget = 0.f;
		/** Metres, -1 when not in atmosphere. */
		float Altitude = -1.f;
		/** The flown ship's ability (E / Y), second power (Bastion's dome, hold Q) and secondary ammo. */
		bool bHasAbility = false;
		float Ability = 1.f;
		FString AbilityGlyph;
		bool bHasPower2 = false;
		float Power2 = 1.f;
		int32 Ammo = 0;
		/** The flown ship has a secondary-ammo weapon (review M5). */
		bool bUsesAmmo = false;
		float Heat = 0.f;
		/** A ship is under the player's control (review m9: none while the flown ship is down before the retake). */
		bool bHasFlown = false;
		// ---- P1
		bool bHasData = false;
		/** The flown ship (the reticle cluster's colour). */
		ETDShipId Flown = ETDShipId::Rift;
		float Hull = 1.f;
		float Shield = 1.f;
		/** Arc opacity: 1 while either value is below 100% or changed in the last ArcAwakeSeconds, else ArcRestOpacity. */
		float ArcOpacity = 1.f;
		/** Signature power: 0 just used, 1 ready. */
		float Power = 1.f;
		bool bPowerReady = true;
		/** 0..1 through the ready pulse (0 when not pulsing). */
		float PowerPulse = 0.f;
		/** The power key / button glyph ("Q", or "X" on a pad). */
		FString PowerGlyph;
		TArray<FChip> Chips;
		/** The objective line's text in caps, and its right-hand counter ("5 LEFT", empty between waves). */
		FString Objective;
		FString Counter;
		/** Second line of the expanded objective ("WAVE 3 OF 5"). */
		FString ObjectiveDetail;
		/**
		 * The Dominion assault (storyline.md Gameplay 06 step 3, a persistent count 0/4 to 4/4): one pip per shield
		 * generator under the objective line, filled when it is down; GeneratorFlash01 1..0 after each one falls.
		 */
		int32 GeneratorsTotal = 0;
		int32 GeneratorsDown = 0;
		float GeneratorFlash01 = 0.f;
		/**
		 * The transponder off Verdance (T4's ATDTransponderBeacon; storyline.md §18): a sensor marker on it labelled
		 * "UNKNOWN", that reads "VERIFIED" for a moment once, then resets. Nobody explains it.
		 */
		bool bTransponder = false;
		bool bTransponderOnScreen = false;
		FVector2D TransponderPos = FVector2D::ZeroVector;
		bool bTransponderVerified = false;
		float TransponderMetres = 0.f;
		/**
		 * The escape flight (T4's ATDDominion in EscapeFlight: GetEscapeTimeLeft, EscapeSeconds, GetEscapeExitRadius):
		 * the clock under the objective, and "CLEAR" where the way out is (straight away from the hull, at the radius).
		 */
		/**
		 * ENG-38, the descent (T1's readings): during the entry burn, the glide and the climb-out an altimeter tape
		 * (ITDHUDData::GetAltitude), a hull-temperature bar (UTDDescentSubsystem::GetEntryHeat01) and "ATMOSPHERE";
		 * "ORBIT" for OrbitCueSeconds after the climb-out's galaxy reveal. Hidden otherwise.
		 */
		/**
		 * ENG-29 docking (T4's ATDDockingBay): while its tractor pulls the flown ship "TRACTOR BEAM · CONTROLS LIMITED" with
		 * the pull 0..1; once it glides in (controls locked) "DOCKING · CONTROLS LOCKED". 0 = none, 1 = pulled, 2 = gliding.
		 */
		uint8 Docking = 0;
		/** T4's reactor hold has the flown ship: "REACTOR IN RANGE · HOLD FIRE ON THE CORE". */
		bool bReactorHold = false;
		/** For the radar header in the assault: the reactor is exposed, its phase (1..N) and N. */
		bool bReactorExposed = false;
		int32 ReactorPhase = 0;
		int32 ReactorPhases = 0;
		float TractorPull01 = 0.f;
		bool bDescent = false;
		float Heat01 = 0.f;
		float Descent01 = -1.f;
		float OrbitCue01 = 0.f;
		bool bEscapeFlight = false;
		float EscapeLeft = 0.f;
		float EscapeTotal = 0.f;
		bool bClearOnScreen = false;
		FVector2D ClearPos = FVector2D::ZeroVector;
		float ClearMetres = 0.f;
		/** 0 collapsed .. 1 expanded (for ObjectiveExpandSeconds after the text changes, eased over 0.3 s). */
		float ObjectiveExpand = 0.f;
		bool bUsingGamepad = false;
	};
	const FView& GetView() const { return View; }

	/** Same name and signature as Nick's UTDSpaceHUDWidget, so the host's reflection path drives both. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	void SetDataSource(UObject* InSource);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	void RefreshData();

	UObject* GetDataSource() const { return Source.Get(); }

	/**
	 * Screen coverage the spec's way (docs/eng/hud-redesign.md 3.3): the sum of the drawn elements' bounding boxes over the
	 * 1920x1080 design area, from the last paint. Budget 8%; the spec's steady combat is about 3.2%.
	 */
	float GetCoverage01() const { return LastCoverage01; }

	/** Re-reads the data source and advances the timers by DeltaSeconds (the tick calls this every frame). */
	void Sample(float DeltaSeconds);

	/** Tests: force the glyph device (-1 = ask the owning controller). */
	int32 ForceGamepad = -1;

	/** The player pressed the hyperdrive key: if it is blocked, the reason shows in the lower lane for 3 s (spec: only then). */
	UFUNCTION()
	void HandleHyperdriveInput(bool bHeld);

	/** The event bus's OnHitConfirmed (the player's hits): the reticle tick, or on a kill the X there and on the victim. */
	UFUNCTION()
	void HandleHitConfirmed(AActor* Target, bool bKill);

	// ---- Aftermath navigation pulse (storyline.md section 12): the HUD briefly marks the drones' corridor, then clears.
	/** A faint corridor From -> To (world) for Seconds; it must read as sensor noise (nobody comments on it). */
	void ShowNavigationPulse(const FVector& From, const FVector& To, float Seconds = 1.5f);
	static constexpr float NavPulseSeconds = 1.5f;
	/** The pulse's envelope now, 0..1 (what the corridor draws with; UTDUIHostSubsystem::GetNavigationPulse01). */
	float GetNavigationPulseEnvelope() const
	{
		const float T01 = NavLeft > 0.f && NavSeconds > 0.f ? 1.f - NavLeft / NavSeconds : 0.f;
		return T01 <= 0.f ? 0.f : FMath::Clamp(FMath::Min(T01 / .1f, (1.f - T01) / .35f), 0.f, 1.f);
	}

	// ---- subtitles and story transmissions (T3's transmission system calls UTDUIHostSubsystem::ShowTransmission)
	/** Shows Line in the subtitle slot for Seconds: speaker tag in the pilot's colour, style per voice. */
	void ShowTransmission(FName Speaker, const FText& Line, ETransmissionStyle Style, float Seconds);
	/** The event bus's OnBarkRequested: the squad's barks as subtitles (a readable line from the bark id). */
	UFUNCTION()
	void HandleBarkRequested(FName BarkId, FName Speaker);
	/** "Bark_SquadEngageHighLeft" -> "engage high left" (until barks carry their dialogs.md text). */
	static FString BarkText(FName BarkId);
	static constexpr float BarkSeconds = 2.5f;
	static constexpr float SubtitleY = 900.f;
	static constexpr float SubtitleMaxWidth = 880.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversSubtitles = true;

	// ---- Director's Cut moments (spec section 4; the theme is a judging category, the HUD gets out of the way)
	UFUNCTION()
	void HandleTakeStarted(FName SceneId, int32 TakeNumber);
	UFUNCTION()
	void HandleSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds);
	/** Tests: force the money-shot / paused state (-1 = read the feel subsystem / world). */
	int32 ForceMoneyShot = -1;
	/** Tests: >= 0 forces the subtitles setting (ENG-45: UTDSettingsSubsystem::AreSubtitlesOn); -1 reads it. */
	int32 ForceSubtitlesOn = -1;
	/** ENG-45: subtitles drawn at all (the setting; voices and timing are unchanged either way). */
	bool AreSubtitlesShown() const { return bSubtitlesOn; }
	int32 ForcePaused = -1;
	/**
	 * Set every tick by UTDUIHostSubsystem from its focus holds (PushHUDFocus): the whole HUD eases to at most FocusOpacity,
	 * and bFocusHideCombat takes the objective pill and waypoints off screen. 1 / false = no hold.
	 */
	float FocusOpacity = 1.f;
	bool bFocusHideCombat = false;

	/** Tests: -1 reads the run's progress; 1 / 2 force the approach marker pending / open (0 = none). */
	int32 ForceApproach = -1;
	/** The objective whose completion means Cinder's field anchor is down (T5's course). */
	static inline const FName AnchorObjectiveId = TEXT("Obj_Jammer");
	static constexpr float ApproachOpenSeconds = 8.f;
	/** Pickups get a marker within this many cm (800 m); the collect toast lasts PickupToastSeconds. */
	static constexpr float PickupMarkRange = 80000.f;
	static constexpr float PickupToastSeconds = 1.4f;
	/** Where the approach marker points when no ATDDominion is in the world (tests). */
	TOptional<FVector> ApproachTargetOverride;
	static constexpr float SlateSeconds = 1.5f;
	static constexpr float SlateOpacity = .4f;
	static constexpr float PauseOpacity = .3f;
	static constexpr float CutFadeInSeconds = .25f;
	static constexpr float MoneyShotEaseSeconds = .15f;

	static constexpr float HitMarkSeconds = .15f;
	static constexpr float KillMarkSeconds = .35f;
	static constexpr float KillAtTargetSeconds = .6f;

	// Surfaces this widget draws, so the dev HUD stops drawing them (UTDUIHostSubsystem reads these by name). Subtitles
	// and the pause line stay with the dev HUD / pause screen.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversDamageIndicators = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversSquadMarkers = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversInteractPrompt = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversHyperdriveBlockText = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversAltitude = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversBoundaryWarning = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversCombatStates = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversObjectiveMarkers = true;

	static constexpr float StrikePulseSeconds = .35f;

	static constexpr float BlockedSeconds = 3.f;
	static constexpr float BoostHideSeconds = 1.5f;
	static constexpr float HitSeconds = 1.2f;
	static constexpr float HitRadius = 150.f;
	static constexpr float BracketMin = 48.f;
	static constexpr float BracketMax = 160.f;

	static constexpr float TroubleHull = .35f;
	static constexpr float ArcAwakeSeconds = 3.f;
	static constexpr float ArcRestOpacity = .55f;
	static constexpr float PulseSeconds = .3f;
	static constexpr float ObjectiveExpandSeconds = 3.f;
	static constexpr float ObjectiveEaseSeconds = .3f;

	// Design-space layout (1920x1080, spec section 3.1; the strip at the Lead's 1.3x).
	static constexpr float CenterX = 960.f;
	static constexpr float CenterY = 540.f;
	static constexpr float SafeX = 96.f;
	static constexpr float SafeY = 54.f;
	static constexpr float HullRadius = 44.f;
	static constexpr float ShieldRadius = 53.f;
	static constexpr float ArcHalfDeg = 40.f;
	static constexpr float PipOffsetX = 64.f;
	static constexpr float PipOffsetY = 56.f;
	static constexpr float PipRadius = 17.f;
	static constexpr float StripScale = 1.3f;
	static constexpr float ChipHeight = 44.f * StripScale;
	static constexpr float ActiveChipWidth = 148.f * StripScale;
	static constexpr float WingChipWidth = 100.f * StripScale;
	static constexpr float ChipGap = 6.f * StripScale;
	/** #160 squad panel (Concept A), design px at 1080p. */
	static constexpr float FlownCardW = 320.f;
	static constexpr float FlownCardH = 100.f;
	static constexpr float WingCardW = 232.f;
	static constexpr float WingCardH = 76.f;
	static constexpr float CardGap = 12.f;
	static constexpr float CardGrowSeconds = .12f;
	static constexpr float ObjectiveWidth = 480.f;
	static constexpr float ObjectiveHeight = 28.f;
	static constexpr float ObjectiveExpandedWidth = 720.f;
	static constexpr float ObjectiveExpandedHeight = 72.f;

protected:
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	bool IsUsingGamepad() const;
	void LayoutChips();
	void SampleCombat(const class ITDHUDData& Data, const FTDShipHUD* Flown, float Dt);
	/** Viewport pixels -> design space (the same letterbox the painter uses). */
	FVector2D ToDesign(const FVector2D& Pixels) const;
	/** A point on the safe frame along Dir from the centre (edge chevrons). */
	static FVector2D OnSafeFrame(const FVector2D& Dir);
	void BindController();

	FVector2D ViewportSize = FVector2D(1920, 1080);
	/** Screen pixels per local unit (UMG's DPI scale: at 1280x720 the widget is still about 1080 units tall). */
	float PixelScale = 1.f;
	/** Written by NativePaint (const): what the last frame drew. */
	mutable float LastCoverage01 = 0.f;
public:
	/** Where the last paint put the low-hull vignette, in widget-local units (tests: it spans the whole widget). */
	mutable FBox2D LastVignetteRect = FBox2D(ForceInit);
	/**
	 * Every element's design-space box from the last paint, by name ("reticle", "strip", "objective", "lane", "boost",
	 * "missile", "boundary", "waypoint", "mark", ...): the layout tests check each one lies in the safe frame.
	 */
	mutable TArray<TPair<FName, FBox2D>> LastBoxes;
private:
	float BlockedLeft = 0.f;
	float HintDoneAge = 0.f;
	float StrikePulseLeft = 0.f;
	float CutLeft = 0.f;
	float NavLeft = 0.f;
	/** Approach marker: the reveal seen last sample (an edge opens the 8 s "open" state), its countdown, first-sample guard. */
	/** Pickups: the collected counts last seen (a change = a toast) and the toast's clock. */
	int32 PickupSeen[5] = {}; // TDPickupTypeCount (static_assert in the .cpp)
	/** The quiet field-hit tick (FTDFieldDamage::OnPlayerFieldHit): its clock and the binding. */
	float FieldHitLeft = 0.f;
	/** Generator pips: the down count last seen (a rise flashes) and the flash clock. */
	int32 GeneratorsDownSeen = -1;
	/** The ORBIT cue's clock; the descent cover seen last sample (its end on the galaxy map is the climb-out reveal). */
	float OrbitCueLeft = 0.f;
	FTDHUDModel HUDModel;
	void UpdateSquadCards(float Dt);
	float GhostRate[3] = { 0.f, 0.f, 0.f };
	float LastCardHull[3] = { -1.f, -1.f, -1.f };
	int32 CardHullShown[3] = { -1, -1, -1 };
	int32 CardShieldShown[3] = { -1, -1, -1 };
	int32 CardDownShown[3] = { -2, -2, -2 };
	int32 CardAbilitySecShown[3] = { -1, -1, -1 };
	int32 CardOverShown[3] = { -1, -1, -1 };
	FTDWaveInfo FrameWave;
	/** "KATO DOWN · SWITCHED TO MOSS" after an auto-switch, for 3 s. */
	FString SwitchNotice;
	float SwitchNoticeLeft = 0.f;
	/** The player's Minimal HUD setting (read once a second). */
	bool bMinimalSetting = false;
	float MinimalReadLeft = 0.f;
	/** #161: T1's "PLANET CLOSED" warning (UTDAtmosphereEntryComponent), upper-cased once. */
	bool bPlanetClosed = false;
	FString PlanetClosedRaw, PlanetClosedLine;
	/** #154: the subtitle's wrap, cached per text / size / width (NativePaint is const). */
	mutable FString SubWrapText;
	mutable TArray<FString> SubWrapLines;
	mutable int32 SubWrapPx = 0;
	mutable float SubWrapMax = 0.f;
	mutable float SubWrapWidth = 0.f;
	/** This frame's target / objective markers from the source (read once, shared by the marks and the radar model). */
	TArray<FTDTargetMarker> FrameMarkers;
	TArray<AActor*> FrameObjectives;
	TWeakObjectPtr<AActor> DockingBay;
	float DockingScanLeft = 0.f;
	bool bSubtitlesOn = true;
	double OrbitRevealSeen = -1.0;
	bool bCoverSeen = false;
	FDelegateHandle RevealHandle;
	float GeneratorFlashLeft = 0.f;
	/** The transponder marker: the beacon (re-found every 2 s), and how long since it was first sighted. */
	TWeakObjectPtr<AActor> TransponderBeacon;
	float TransponderScanLeft = 0.f;
	float TransponderSeenFor = -1.f;
public:
	/** Seconds after the first sighting when the label reads VERIFIED (twice, briefly), then UNKNOWN for good. */
	static constexpr float TransponderVerifiedAt = 1.4f;
	static constexpr float TransponderVerifiedFor = .45f;
	/** Tests: >= 0 sets the seconds since the sighting (no beacon needed with ForceTransponderAt). */
	float ForceTransponderAge = -1.f;
	TOptional<FVector> ForceTransponderAt;
	/** Tests: >= 0 forces the generator count (total, down) without a Dominion. */
	int32 ForceGeneratorsTotal = -1;
	int32 ForceGeneratorsDown = 0;
	/** Tests: >= 0 forces the descent readings (heat 0..1, glide 0..1 or -1); bForceOrbitCue starts the ORBIT cue. */
	float ForceHeat01 = -1.f;
	/** Issue #160: the squad panel and radar data (layout-free; drawn once the concept is picked). */
	const FTDHUDModel& GetHUDModel() const { return HUDModel; }
	/** TD.UI.Cost: what Sample and NativePaint cost on the game thread (NativePaint is const: mutable). */
	FTDHUDCost SampleCost;
	mutable FTDHUDCost PaintCost;
	/** Tests: >= 0 forces the docking state (0 none, 1 pulled, 2 gliding) and ForceTractorPull01 its pull. */
	int32 ForceDocking = -1;
	float ForceTractorPull01 = 0.f;
	float ForceDescent01 = -2.f;
	void StartOrbitCue() { OrbitCueLeft = OrbitCueSeconds; }
	static constexpr float OrbitCueSeconds = 3.f;
	/** Tests: >= 0 forces an escape flight (seconds left of total) clear of a hull at ForceEscapeCentre, radius cm. */
	float ForceEscapeLeft = -1.f;
	float ForceEscapeTotal = 60.f;
	FVector ForceEscapeCentre = FVector::ZeroVector;
	float ForceEscapeRadius = 100000.f;
private:
	FDelegateHandle FieldHitHandle;
	void HandleFieldHit(AActor* Field, const FVector& Where) { FieldHitLeft = FieldHitSeconds; }
	static constexpr float FieldHitSeconds = .12f;
	static constexpr float DebrisShowSeconds = 2.5f;
	float PickupToastLeft = 0.f;
	bool bApproachWasRevealed = false;
	bool bApproachSeeded = false;
	float ApproachOpenLeft = 0.f;
	float NavSeconds = 0.f;
	FVector NavWorldFrom = FVector::ZeroVector;
	FVector NavWorldTo = FVector::ZeroVector;
	float SlateLeft = 0.f;
	void SampleDirector(float Dt);
	/** T3's story transmissions (UTDStorySubsystem): the current line drives the subtitle slot. */
	void SampleStory();
	TWeakObjectPtr<class UTDStorySubsystem> BoundStory;
	float HitLeft = 0.f;
	float KillLeft = 0.f;
	float KillAtLeft = 0.f;
	FVector KillWorld = FVector::ZeroVector;
	TWeakObjectPtr<class UTDEventBus> BoundBus;
	void BindBus();
	/** Reads T3's hint getters (UTDHintComponent::GetHintKey / Action / Step / State); false when no hint is up. */
	bool ReadHintCard();
	FString LastHintKeyRaw;
	int32 LastWaveNumber = -1;
	/** Ships seen with secondary ammo this session (review M5: a ship with none shows no ammo number). */
	TSet<ETDShipId> ShipsWithAmmo;
	FString LastHintActionRaw;
	float BoostFullFor = 1e6f;
	TWeakObjectPtr<class ATDSpacePlayerController> BoundPC;

	FView View;
	TWeakObjectPtr<UObject> Source;
	float LastHull = -1.f;
	float LastShield = -1.f;
	float ArcAwake = 0.f;
	float PulseLeft = 0.f;
	bool bWasPowerReady = true;
	ETDShipId LastFlown = ETDShipId::Rift;
	FString LastObjective;
	/** Seconds since the objective text last changed (huge = never). */
	float ObjectiveAge = 1e6f;
	float Time = 0.f;
};
