// Triad Drift - presentation hooks (features.md §3.5). Lead-owned class definitions; Nick owns the
// asset instances. Every slot is optional: an empty slot must do nothing.
// Play a cue with UTDStatics::PlayCue / PlayCueAttached.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDPresentation.generated.h"

class UNiagaraSystem;
class USoundBase;
class UCameraShakeBase;
class UStaticMesh;
class USkeletalMesh;
class UMaterialInterface;
class UTexture2D;
class UAnimInstance;
class UAnimSequenceBase;
class UUserWidget;

/** One presentation moment: an effect, a sound and a camera shake, each optional. */
USTRUCT(BlueprintType)
struct FTDFXCue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|FX")
	TSoftObjectPtr<UNiagaraSystem> VFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|FX")
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|FX")
	TSoftClassPtr<UCameraShakeBase> Shake;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|FX", meta = (ClampMin = "0"))
	float ShakeScale = 1.f;

	/** World-space radius within which the local player's camera shakes; 0 = always. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|FX", meta = (ClampMin = "0"))
	float ShakeRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|FX")
	FVector VFXScale = FVector::OneVector;

	bool IsEmpty() const { return VFX.IsNull() && Sound.IsNull() && Shake.IsNull(); }
};

/**
 * One extra mesh of a modular ship (wing, pod, engine block): attached to the hull, either at a
 * socket of the primary mesh or at a transform relative to the ship root. Purely visual.
 */
USTRUCT(BlueprintType)
struct FTDShipVisualPart
{
	GENERATED_BODY()

	/** Name for tooling and for gameplay to find the part (for example "Pod_L", "Engine_R"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FName PartName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** Socket on the ship's primary mesh to attach to; None attaches to the ship root. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FName ParentSocket;

	/** Offset from the socket (or from the ship root when no socket is given). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FTransform RelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	TArray<TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDShipVisuals : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TArray<TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;

	/**
	 * Optional modular assembly: extra meshes attached to the hull. A ship can be one assembled
	 * Mesh, a Mesh plus Parts, or Parts only (then the first part should be the fuselage).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TArray<FTDShipVisualPart> Parts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FLinearColor SquadColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TSoftObjectPtr<UTexture2D> HUDIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue EngineTrail;

	/** Dodge: a short side-thruster burst, attached at the engine (player ship only). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue Dodge;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue BoostStart;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue BoostLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue DriftLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue DriftRelease;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	TSoftObjectPtr<USoundBase> EngineLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue CollisionScrape;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Switch")
	FTDFXCue SwitchIn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Switch")
	FTDFXCue SwitchOut;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
	FTDFXCue PowerWindup;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
	FTDFXCue PowerActive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
	FTDFXCue PowerEnd;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
	FTDFXCue Power2Active;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue ShieldHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue ShieldBreak;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue HullHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue LowHullLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue Explosion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue RespawnWarpIn;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDWeaponFX : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire")
	FTDFXCue Muzzle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire")
	FTDFXCue ChargeLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire")
	FTDFXCue Overheat;

	/** Attached to the projectile for its lifetime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TSoftObjectPtr<UNiagaraSystem> Tracer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TSoftObjectPtr<UStaticMesh> ProjectileMesh;

	/**
	 * Visual-only offset of ProjectileMesh from the projectile's collision centre, in the
	 * projectile's local space (X forward). For centre-pivoted meshes whose tail would clip the
	 * launcher at spawn. Never moves collision or gameplay.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	FVector ProjectileMeshOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FTDFXCue ImpactHull;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FTDFXCue ImpactShield;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FTDFXCue ImpactWorld;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FTDFXCue Detonation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FTDFXCue Deflected;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDEnemyVisuals : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TArray<TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue EngineTrail;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue WarpIn;

	/** Plays for the telegraph duration before a big attack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Telegraph;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue ShieldHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue HullHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Stunned;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Explosion;

	// ---- per-attack wind-ups; an empty slot falls back to Telegraph
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph")
	FTDFXCue TelegraphMissile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph")
	FTDFXCue TelegraphHeavyBolt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph")
	FTDFXCue TelegraphSalvo;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph")
	FTDFXCue TelegraphMineTrail;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph")
	FTDFXCue TurretTelegraph;

	// ---- carrier
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carrier")
	FTDFXCue BayLaunchTelegraph;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carrier")
	FTDFXCue BayLaunch;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carrier")
	FTDFXCue BayDestroyed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carrier")
	FTDFXCue TurretDestroyed;

	// ---- gunship frontal shield (hits use ShieldHit)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frontal Shield")
	FTDFXCue FrontalShieldBreak;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frontal Shield")
	FTDFXCue FrontalShieldRestore;

	// ---- damage states
	/** Attached loop once the hull is below 50%. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue DamagedSmoke;

	/** Attached, plays when the death tumble starts; Explosion plays at the end. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue DeathTumble;

	// ---- on-foot crew (all optional)
	/** Animation Blueprint class for SkeletalMesh. Empty: gameplay falls back to the template mannequin set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crew")
	TSoftClassPtr<UAnimInstance> AnimBPClass;

	/** Yaw of SkeletalMesh inside the capsule. UE5 mannequin-skeleton meshes face +Y, so -90 makes them face forward. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crew")
	float SkeletalMeshYaw = -90.f;

	/** Played when the enemy is staggered (parry, guard break, heavy hit). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crew")
	FTDFXCue Stagger;

	/** Played when an on-foot enemy dies. Gameplay owns the ragdoll; this is the presentation cue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crew")
	FTDFXCue Death;
};

/** The Dominion mothership (features.md ENG-18). */
UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDDominionVisuals : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> HullMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> GeneratorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> ReactorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arrival")
	FTDFXCue WarpIn;

	/** Attached loop while the main shield is up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield")
	FTDFXCue ShieldIdle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield")
	FTDFXCue ShieldHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield")
	FTDFXCue ShieldSectionDown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield")
	FTDFXCue ShieldCollapse;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generators")
	FTDFXCue GeneratorHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generators")
	FTDFXCue GeneratorExplosion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactor")
	FTDFXCue ReactorExposed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactor")
	FTDFXCue ReactorHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactor")
	FTDFXCue ReactorPhaseChange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactor")
	FTDFXCue ReactorDestroyed;

	/** Plays when the breakup begins; Nick's Level Sequence can also listen for the same event. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finale")
	FTDFXCue BreakupSequence;

	/** Stretch: the docking bay opening (ENG-29). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finale")
	FTDFXCue DockingBayOpen;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDAbilityFX : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Activate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Loop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue End;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue AffectedTarget;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDTravelFX : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hyperdrive")
	FTDFXCue HyperdriveSpool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hyperdrive")
	FTDFXCue HyperdriveTunnel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hyperdrive")
	FTDFXCue HyperdriveDropOut;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FTDFXCue PortalOpen;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FTDFXCue PortalIdle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FTDFXCue PortalTransit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FTDFXCue PortalClose;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue AtmosphericEntry;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue LandingDust;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue TakeOff;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warp")
	FTDFXCue EnemyWarpIn;

	/** Played when the player presses the hyperdrive key while it is locked (wave active, finale) or blocked. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hyperdrive")
	FTDFXCue HyperdriveRefused;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDBladeFX : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FLinearColor BladeColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Ignite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Retract;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue SwingLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue SwingHeavy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Hit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Block;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Deflect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue PerfectDeflect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue GuardBreak;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue AirSlam;

	/** First-person blade mesh. Empty = engineering placeholder. Colour comes from BladeColor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blade")
	TSoftObjectPtr<UStaticMesh> BladeMesh;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDUIStyle : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors")
	TMap<ETDShipId, FLinearColor> SquadColors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors")
	FLinearColor EnemyColor = FLinearColor(1.f, 0.12f, 0.24f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icons")
	TMap<ETDAbilityId, TSoftObjectPtr<UTexture2D>> AbilityIcons;

	// ---- front end (optional). The menu / transition game modes use these when their own
	// widget class is not set, so the UI is assigned from this asset with no Blueprint or config edit.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Front End")
	TSoftClassPtr<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Front End")
	TSoftClassPtr<UUserWidget> LoadingWidgetClass;

	/**
	 * Pause menu (optional). Shown while the player has paused the game: open it on the player
	 * controller's OnPauseChanged(true) and close it on false. Without one the dev HUD draws a plain
	 * "PAUSED" line.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Front End")
	TSoftClassPtr<UUserWidget> PauseWidgetClass;

	/**
	 * Shown on the menu map after a won run INSTEAD of MainMenuWidgetClass (optional). Empty = the code-built
	 * victory screen (run time, kills, ships lost, cuts, takes per scene, return to menu). A main-menu widget is
	 * therefore never asked to show the victory statistics (N-023).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Front End")
	TSoftClassPtr<UUserWidget> VictoryWidgetClass;

	/**
	 * The Settings page (optional): opened from the main menu and the pause menu. Empty = the code-built page.
	 * A styled page calls the same UTDSettingsSubsystem functions (ENG-32).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Front End")
	TSoftClassPtr<UUserWidget> SettingsWidgetClass;

	/** The on-foot HUD (optional). Empty = engineering's styled native on-foot HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Front End")
	TSoftClassPtr<UUserWidget> GroundHUDWidgetClass;

	/**
	 * The intro (ENG-33), optional. When set, this Level Sequence plays on New Game instead of the code-driven
	 * shots, with the same skip and hand-off rules.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro")
	TSoftObjectPtr<UObject> IntroSequence;

	/** Music for the intro, swelling into wave 1 (optional; silence when empty). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro")
	TSoftObjectPtr<USoundBase> IntroMusic;

	/** One optional voice line per narration line of the intro, in order (silence for a missing entry). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro")
	TArray<TSoftObjectPtr<USoundBase>> IntroNarrationVoice;

	/**
	 * Music for the main menu (optional). The menu game mode plays it 2D and fades it out when the game
	 * travels to the first map. Empty = a silent menu. The sound's own class and volume are used as authored.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Front End")
	TSoftObjectPtr<USoundBase> MenuMusic;
};

/**
 * The on-foot player's body (optional). Looked up as DA_Pilot_Visuals in /Game/TriadDrift/Presentation, then in
 * /Game/TriadDrift/Gameplay/Characters/Pilot. Empty = no body is drawn (first person, blade only).
 * The body is drawn for shadows and third-person moments; in first person the owner sees it only below the
 * neck (the head is hidden from the owner by gameplay). It may use any skeleton: gameplay plays the three
 * loops below by speed when no AnimBPClass is given.
 */
UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDPilotVisuals : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TArray<TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;

	/** Optional animation blueprint. When empty the three loops below are played by speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TSoftClassPtr<UAnimInstance> AnimBPClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TSoftObjectPtr<UAnimSequenceBase> Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TSoftObjectPtr<UAnimSequenceBase> Walk;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TSoftObjectPtr<UAnimSequenceBase> Run;

	/** Yaw applied to the mesh so it faces the capsule's forward (+X). Mannequin-style meshes face +Y: -90. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	float MeshYaw = -90.f;

	/** Uniform scale. The gameplay capsule is 180 cm tall (half-height 90); the eye is at about 164 cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	float MeshScale = 1.f;

	/** Bone hidden from the owner in first person so the camera does not sit inside the head. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	FName HeadBone = TEXT("head");
};
