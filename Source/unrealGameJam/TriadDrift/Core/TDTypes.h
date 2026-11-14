// Triad Drift - shared types (features.md §3.1). Lead-owned; append only, never rename or remove.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "TDTypes.generated.h"

class AActor;

UNREALGAMEJAM_API DECLARE_LOG_CATEGORY_EXTERN(LogTriadDrift, Log, All);

/**
 * Actor tags that carry a contract between lanes (and with presentation). The strings never change.
 */
namespace TDTags
{
	/**
	 * "This actor is where the player should go right now." World and flow actors add it while they
	 * are the objective and remove it when they stop being one. The squad manager returns the tagged,
	 * live actors from ITDHUDData::GetObjectiveMarkers(); the HUD draws direction markers from that.
	 */
	inline const FName ObjectiveMarker(TEXT("TD.Objective.Marker"));

	/** A map-placed sky: switches all placeholder lighting, exposure and stars off (ATDPlaceholderSky). */
	inline const FName PresentationSky(TEXT("TD.Presentation.Sky"));
}

/** Collision channels (Config/DefaultEngine.ini). Characters use the engine's Pawn channel. */
namespace TDCollision
{
	inline constexpr ECollisionChannel Projectile = ECC_GameTraceChannel1;
	inline constexpr ECollisionChannel Ship = ECC_GameTraceChannel2;
	inline constexpr ECollisionChannel Interactable = ECC_GameTraceChannel3;
	inline constexpr ECollisionChannel Portal = ECC_GameTraceChannel4;
}

UENUM(BlueprintType)
enum class ETDShipId : uint8
{
	Rift,
	Lancer,
	Bastion
};

UENUM(BlueprintType, meta = (ScriptName = "TDTeamId"))
enum class ETDTeam : uint8
{
	Player,
	Enemy,
	Neutral
};

UENUM(BlueprintType)
enum class ETDAbilityId : uint8
{
	None,
	GravityWell,
	Cloak,
	EMPPulse
};

UENUM(BlueprintType)
enum class ETDDestination : uint8
{
	MainMenu,
	Galaxy,
	Verdance,
	Cinder,
	DominionInterior,
	Victory
};

UENUM(BlueprintType)
enum class ETDDamageKind : uint8
{
	Energy,
	Kinetic,
	Explosive,
	Melee,
	Ram,
	Environmental
};

UENUM(BlueprintType)
enum class ETDGamePhase : uint8
{
	Intro,
	Waves1to2,
	Verdance,
	Waves3to4,
	Cinder,
	Wave5,
	DominionAssault,
	DominionInterior,
	Escape,
	Victory
};

UENUM(BlueprintType)
enum class ETDWeaponSlot : uint8
{
	Primary,
	Secondary
};

/** Director's Cut "money shot" moments (features.md C-03 #2). */
UENUM(BlueprintType)
enum class ETDHighlightKind : uint8
{
	RamKill,
	MissileReturn,
	MultiKill,
	GeneratorKill,
	ReactorKill,
	PerfectDeflect,
	SwitchStrikeKill,
	AbilityCombo,
	CarrierKill
};

UENUM(BlueprintType)
enum class ETDHyperdriveState : uint8
{
	Idle,
	Spooling,
	Jumping,
	DropOut
};

UENUM(BlueprintType)
enum class ETDPortalState : uint8
{
	Opening,
	Open,
	Transit,
	Closing,
	Closed
};

/** Pilot intent for one frame. Players and AI both fly a spacecraft only through this. */
USTRUCT(BlueprintType)
struct FTDFlightInput
{
	GENERATED_BODY()

	/** -1 (min speed) .. +1 (cruise). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Flight")
	float Throttle = 0.f;

	/** X = yaw, Y = pitch, each -1..1 of the maximum turn rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Flight")
	FVector2D Aim = FVector2D::ZeroVector;

	/** -1..1 of the maximum roll rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Flight")
	float Roll = 0.f;

	/** X = right, Y = up, each -1..1 of the strafe speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Flight")
	FVector2D Strafe = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Flight")
	bool bBoost = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Flight")
	bool bDrift = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Flight")
	bool bBrake = false;
};

USTRUCT(BlueprintType)
struct FTDRunStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Stats")
	int32 Kills = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Stats")
	int32 ShipSwitches = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Stats")
	int32 PortalsUsed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Stats")
	int32 ShotsDeflected = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Stats")
	int32 Deaths = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Stats")
	int32 MissilesReturned = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Stats")
	int32 HighlightMoments = 0;

	/** Asteroids and wreckage the squad broke in T6's space field (Ryan, Nov 13: shooting the field is rewarding). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Stats")
	int32 DebrisCleared = 0;
};

USTRUCT(BlueprintType)
struct FTDProgress
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	ETDGamePhase Phase = ETDGamePhase::Intro;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	int32 WavesCleared = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	TSet<FName> CompletedObjectives;

	/** Ability -> the ship that owns it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	TMap<ETDAbilityId, ETDShipId> UnlockedAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	bool bDominionRevealed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	bool bDominionDefensesDown = false;

	/** Boarding finale (ENG-29): set by UTDGameInstance::SetPhase(DominionInterior); docking travels in after it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	bool bDominionBoarded = false;

	/** The interior's reactor core is dead (ENG-30): T4 rebuilds the burning Dominion for the escape flight from this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	bool bReactorDestroyed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	FName ReturnPoint = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	ETDShipId LastShip = ETDShipId::Rift;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	float RunTimeSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	FTDRunStats Stats;

	/** Director's Cut: scene id -> number of takes used (1 = first try). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	TMap<FName, int32> Takes;

	/** Story transmissions already played this run (UTDStorySubsystem): kept across retakes, saved, never replayed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "TriadDrift|Progress")
	TSet<FName> StoryTransmissions;
};

/** One squad card on the HUD. */
USTRUCT(BlueprintType)
struct FTDShipHUD
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	ETDShipId Ship = ETDShipId::Rift;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	float Health01 = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	float Shield01 = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	float Boost01 = 1.f;

	/** Dodge readiness of the player ship: 0 right after a dodge, 1 when it can dodge again (1 for wingmen). */
	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	float Dodge01 = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	float Heat01 = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	int32 SecondaryAmmo = 0;

	/** 0 = just used, 1 = ready. */
	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	float PowerCooldown01 = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	float Power2Cooldown01 = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	float AbilityCooldown01 = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	ETDAbilityId Ability = ETDAbilityId::None;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	bool bPlayerControlled = false;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	bool bAlive = true;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|HUD")
	float RespawnSeconds = 0.f;
};

USTRUCT(BlueprintType)
struct FTDWaveInfo
{
	GENERATED_BODY()

	/** 1-based wave number ("WAVE 3"); 0 before the first wave. All wave events use this numbering. */
	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Waves")
	int32 WaveNumber = 0;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Waves")
	int32 WaveCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Waves")
	float Intensity01 = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Waves")
	int32 EnemiesAlive = 0;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Waves")
	int32 EnemiesTotal = 0;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Waves")
	bool bWaveActive = false;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Waves")
	bool bInBreather = false;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Waves")
	float BreatherRemaining = 0.f;

	/** Director's Cut scene id for this wave, e.g. "Wave3". */
	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Waves")
	FName SceneId = NAME_None;

	/** This wave's own objective line while it is active (wave 5: "BREAK THE DOMINION GUARD"); empty = the generic line. */
	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Waves")
	FText ObjectiveText;
};

/** Everything a damage receiver needs to know about one hit. */
USTRUCT(BlueprintType)
struct FTDDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Damage")
	float Amount = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Damage")
	ETDDamageKind Kind = ETDDamageKind::Energy;

	/** The actor responsible (ship, character). May be null for environmental damage. */
	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Damage")
	TObjectPtr<AActor> Instigator = nullptr;

	/** The thing that physically hit (projectile, blade, the ship itself for a ram). */
	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Damage")
	TObjectPtr<AActor> Causer = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Damage")
	FVector HitLocation = FVector::ZeroVector;

	/** Direction the hit was travelling (world space, normalised when known). */
	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Damage")
	FVector HitDirection = FVector::ZeroVector;

	/** Skip the shield and damage the hull directly. */
	UPROPERTY(BlueprintReadWrite, Category = "TriadDrift|Damage")
	bool bIgnoreShield = false;
};

// ---- ENG-06 HUD data (appended by T2 with the Lead's one-off permission) ----

UENUM(BlueprintType)
enum class ETDMissileWarning : uint8
{
	None,
	/** A hostile missile is homing on the owner. */
	Tracking,
	/** ...and will hit within ImminentSeconds. */
	Imminent,
};

/** One hostile for the HUD: on-screen bracket or off-screen edge arrow. */
/** "Who is shooting me": one recent hit on the player's ship or character (ITDHUDData::GetDamageIndicators). */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDDamageIndicator
{
	GENERATED_BODY()

	/** World-space unit direction from the player toward where the hit came from. */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FVector WorldDirection = FVector::ForwardVector;

	/** Real seconds since the hit. Producers drop entries older than about 1.5 s. */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	float AgeSeconds = 0.f;

	/** 0..1: how hard the hit was relative to the ship's maximum hull plus shield (clamped). */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	float Strength01 = 0.f;

	/** The shield absorbed all of it. */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	bool bShieldOnly = false;
};

/** Where one squad ship is, for a HUD that shows the player their wing (ITDHUDData::GetSquadMarkers). */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDSquadMarker
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Squad")
	ETDShipId Ship = ETDShipId::Rift;

	UPROPERTY(BlueprintReadOnly, Category = "Squad")
	TWeakObjectPtr<AActor> Actor;

	/** The ship the player is flying (a HUD normally skips this one). */
	UPROPERTY(BlueprintReadOnly, Category = "Squad")
	bool bPlayerControlled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Squad")
	bool bAlive = true;

	/** This AI wingman has a live target right now. */
	UPROPERTY(BlueprintReadOnly, Category = "Squad")
	bool bEngaging = false;
};

USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDTargetMarker
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	float Distance = 0.f;

	/** In front of the view and inside the viewport. */
	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	bool bOnScreen = false;

	/** Normalised screen position (0..1) when on screen. */
	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	FVector2D ScreenPosition01 = FVector2D::ZeroVector;

	/** Unit 2D direction from screen centre toward the target (x right, y down): place the edge arrow along it. */
	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	FVector2D ScreenDirection = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	bool bIsCurrentTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	bool bLocked = false;

	/** This marker is an incoming missile, not a ship. */
	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	bool bIncomingMissile = false;
};
