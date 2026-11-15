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

	/**
	 * Size of this hull's boost plume (T7, Nov 14): while boosting the EngineTrail effects get "Boost" = this instead of 1,
	 * so a small hull keeps its plume in proportion (Lancer about 0.6). 1 = as authored; 0 = no boost growth.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0", ClampMax = "2"))
	float ExhaustBoostScale = 1.f;

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

	/** Carrier launch bay (Carrier visuals; real size, scale 1, +X = outward). Replaces the orange box. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> BayMesh;

	/** Point-defence / Dominion turret (real size, scale 1, +Z = up). Replaces the cylinder. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> TurretMesh;

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

	/** Optional structural pieces; each replaces its basic-shape stand-in when set (authored at real size, scale 1,
	 *  pivot where the stand-in sits: bridge on top of the hull, each trench wall at its own centre). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> BridgeMesh;

	/** One trench wall, authored as the LEFT wall; the right wall uses it mirrored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> TrenchMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> ShieldEmitterMesh;

	/** The generator's glowing weak point (the pylon is GeneratorMesh). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> GeneratorCoreMesh;

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

	/**
	 * One explosion of the breakup chain on a hull section (runs when BreakupSequence has no VFX): a burst every
	 * 0.45 s down to 0.12 s, walking out from the reactor. Author it for a 50 m radius; gameplay multiplies VFXScale
	 * by each burst's size (25-70 m radius, so about 0.5x to 1.7x). No VFX = the engineering placeholder burst.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finale")
	FTDFXCue HullSectionExplosion;

	/**
	 * The last flash of the breakup chain that swallows the ship (the hull hides under it). Author it for a 500 m
	 * radius; gameplay scales VFXScale to the real hull. No VFX = the engineering placeholder burst.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finale")
	FTDFXCue BreakupFinalBlast;

	/**
	 * Story (Act 1): the distortion field that hides the Dominion, far off and unreachable, until Cinder's field anchor
	 * falls. Attached loop on the field shell, authored for a 1 km radius; gameplay scales VFXScale to the real
	 * radius. When it has VFX the engineering stand-in (a lattice of lights) hides.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field")
	FTDFXCue DistortionFieldIdle;

	/** Optional material for a sphere shell at the field radius (a translucent shimmer); used with or without the cue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field")
	TSoftObjectPtr<UMaterialInterface> DistortionFieldMaterial;

	/** A ship touching the field (it pushes the ship back), at the contact point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field")
	FTDFXCue DistortionFieldContact;

	/** The field collapsing when the passage opens (the Dominion's reveal), authored for a 1 km radius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field")
	FTDFXCue DistortionFieldCollapse;

	/** Stretch: the docking bay opening (ENG-29). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finale")
	FTDFXCue DockingBayOpen;

	/**
	 * ENG-37: the tractor beam. A loop attached at the bay mouth, +X out of the bay toward the ship, while the tractor
	 * holds the flown ship. Gameplay sets User.Length (cm, bay to ship) and User.Intensity (0..1, the pull) each frame.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finale")
	FTDFXCue TractorBeam;

	/**
	 * ART-18: the Aftermath light row (NS_TD_Dominion_LightRow: 12 lights about 400 m along +Y, facing +X). Attached
	 * to the Dominion at the hull point nearest the escape line (or the player start on a tutorial skip), +X along
	 * the outward hull normal, from OnAftermathBegins until the hyperdrive jump (then deactivated: it fades in 1 s).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finale")
	FTDFXCue AftermathLightRow;
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

	/**
	 * Optional overlay material laid over every hull mesh of the ship while the ability runs (Cloak: the hull shimmer).
	 * Gameplay drives its scalar "Cloak" 0 -> 1 over 0.25 s on activate and 1 -> 0 on end, then removes it. Empty: none.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	TSoftObjectPtr<UMaterialInterface> HullOverlay;
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

	/** The jump's speed streaks (NS_TD_Hyperdrive_Streaks): an attached loop on the ship for the whole jump, beside the
	 *  tunnel, stopped at the drop-out. User.JumpSpeed (cm/s) and User.TunnelSeconds as on the tunnel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hyperdrive")
	FTDFXCue HyperdriveStreaks;

	/** ENG-38 exposure ramp (M_TD_PP_DescentWhite, the effects builder): a post-process material on the flown ship's camera
	 *  whose scalar "Weight" follows the white cover, entry and exit, so the frame brightens into the white and out of it.
	 *  Not applied (no cost) while the cover is off. Halved under reduced motion. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent")
	TSoftObjectPtr<class UMaterialInterface> DescentWhitePP;

	/** ENG-38 glide wind (SW_Travel_GlideWind_01, the effects builder): a 2D loop through the post-reveal glide, in as the
	 *  roar fades, out over the level-off. Only the Sound is used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent")
	FTDFXCue GlideWind;

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

	/** ENG-38 (T1): the entry roar, a 2D loop that persists across the planet load under the white cover. Only Sound is
	 *  used. Empty: AtmosphericEntry's sound stands in. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue DescentRoar;

	// ENG-38 entry / exit set (the effects builder's systems; every one has a User.Intensity 0..1 float that T1's
	// descent curve drives each frame; ship-local, +X = nose). All optional.
	/** Attached loop on each squad ship's root during the climb out (the mirror of AtmosphericEntry). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue AtmosphericExit;
	/** One shot at the ship as it leaves the atmosphere (ship rotation). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue ExitBreakaway;
	/** One shot at the ship when it crosses the cloud deck, either direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue CloudPunch;
	/** Attached loop while under the cloud deck, planet side. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue CloudWisps;

	/** Planet ambience (the effects builder's; ATDPlaceholderSky spawns it attached to the camera and drives User.Fade).
	 *  No gameplay code plays these. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue PlanetAmbientVerdance;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue PlanetAmbientCinder;

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

	/** A trail on the blade for every swing, light or heavy, attached to the blade (the animation builder, Nov 14). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue SwingTrail;

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
	 * The ability-unlock reward screen (board N-070 / N-072), mounted by UTDUIHostSubsystem for each award pending in
	 * UTDAbilityRewardSubsystem. Nick's UTDAbilityRewardWidget contract, called by name: PresentReward(Display) -> bool,
	 * FocusDismissButton(), IsRewardActive(), OnRewardDismissed(FName AbilityId). Empty: /Script/TriadDriftHUD.TDAbilityRewardWidget
	 * when that class exists; otherwise no reward screen (the flight hint still comes).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Front End")
	TSoftClassPtr<UUserWidget> AbilityRewardWidgetClass;

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
	 * The survey scan graphics (optional, issue #62; Nicholas's 1920x1080 art). ScanVerdance: the buried-structure scan
	 * on the Verdance terminal (Cinematic 03, "Beneath the Surface"); ScanCinder: the Verdance / Cinder match on the Cinder
	 * terminal (Cinematic 05, "The Pattern"). When set, engineering's scan monitor shows the image instead of its
	 * code-drawn placeholder, and the terminal's in-world screen shows the same texture
	 * (UTDUIHostSubsystem::ShowScanOnScreen). Empty = the placeholder.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Scans")
	TSoftObjectPtr<UTexture2D> ScanVerdance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Scans")
	TSoftObjectPtr<UTexture2D> ScanCinder;

	/**
	 * Combat music (optional; N-029): played by engineering's music director while a fight is on (space waves,
	 * the Dominion assault, active on-foot encounters), faded in and out; the sound's own class and volume apply.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music")
	TSoftObjectPtr<USoundBase> CombatMusic;

	/**
	 * The Dominion assault's own score (Nick's Audio, ART-19; board N-048), optional. Plays from wave 5's clear (the run
	 * enters ETDGamePhase::DominionAssault) through the four shield generators and the exposed reactor, with CombatMusic
	 * held meanwhile; fades out (3 s) on docking (DominionInterior), on the reactor's end (Victory) or any travel. The
	 * asset carries its own looping, class and volume. Empty: CombatMusic plays in the assault as before.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music")
	TSoftObjectPtr<USoundBase> AssaultMusic;

	/**
	 * Quiet flight in the galaxy (N-063), optional: plays when no other score does (out of the tutorial, not in a fight,
	 * not in the Dominion assault, no travel, not the menu) after IdleMusicDelaySeconds of quiet, fading in over 4 s and
	 * out over 2 s the moment a fight (combat music) or any other score starts. The asset's own class, looping and the
	 * dialogue duck of its sound class apply.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music")
	TSoftObjectPtr<USoundBase> IdleSpaceMusic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music", meta = (ClampMin = "0", Units = "s"))
	float IdleMusicDelaySeconds = 6.f;

	/**
	 * Combat feedback sounds (optional): played as 2D UI sounds by the HUD host when a squad shot lands
	 * (OnHitConfirmed) and when it kills. The sound's own class, volume and concurrency apply; silent when unset.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Feedback")
	FTDFXCue HitConfirm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Feedback")
	FTDFXCue KillConfirm;

	/**
	 * Interface sounds (optional, 2D): a selection moves, a value is confirmed or changed, a page closes, the game
	 * pauses and resumes. Played by the menus, the Settings page and the pause toggle; silent when unset.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI Sounds")
	FTDFXCue UIMove;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI Sounds")
	FTDFXCue UIConfirm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI Sounds")
	FTDFXCue UIBack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI Sounds")
	FTDFXCue PauseIn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI Sounds")
	FTDFXCue PauseOut;

	/**
	 * Director's Cut and progress stings (optional, 2D): the scene slate, the "CUT!" call, slow motion in and out
	 * for a money shot, an objective completed. Silent when unset.
	 */
	/**
	 * N-038: Nick's widget for the Director's Cut cards (switch cut, pilot name card, scene / take slate, "CUT!",
	 * money-shot letterbox). When set, the director keeps all timing and state and only its fallback drawing yields;
	 * the widget reads UTDDirectorSubsystem::GetDirectorCards (BlueprintPure, an FTDDirectorCards struct) and binds
	 * OnDirectorCardsChanged. Created by the director for the local player, hit-test invisible, gone with the world.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director")
	TSoftClassPtr<UUserWidget> DirectorWidgetClass;

	/** Issue #49 (concept A): the pilot insignia on the opening's name cards, white on transparent (tinted by the
	 *  ship's colour). Empty: the code draws a crescent, a lance chevron and a hexagon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director")
	TSoftObjectPtr<UTexture2D> InsigniaRift;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director")
	TSoftObjectPtr<UTexture2D> InsigniaLancer;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director")
	TSoftObjectPtr<UTexture2D> InsigniaBastion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director Sounds")
	FTDFXCue SlateClap;

	/** Cinematic 03: the dormant drone's three pulses as it wakes blue (the motif; Nicholas's sound). Played at the drone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director Sounds")
	FTDFXCue ThreePulse;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director Sounds")
	FTDFXCue CutSting;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director Sounds")
	FTDFXCue SlowMoIn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director Sounds")
	FTDFXCue SlowMoOut;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director Sounds")
	FTDFXCue ObjectiveComplete;

	/** Ambience beds (optional, looping, 2D): one per place, cross-faded by the music director's owner. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ambience")
	TSoftObjectPtr<USoundBase> AmbienceSpace;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ambience")
	TSoftObjectPtr<USoundBase> AmbienceVerdance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ambience")
	TSoftObjectPtr<USoundBase> AmbienceCinder;

	/**
	 * The intro (ENG-33), optional. When set, this Level Sequence plays on New Game instead of the code-driven
	 * shots, with the same skip and hand-off rules.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro")
	TSoftObjectPtr<UObject> IntroSequence;

	/**
	 * Tutorial music (N-039, issue #24; optional). On New Game the prelude (non-looping) starts so it ends on the frame
	 * the intro returns control (cut on a skip or with no intro); the loop (looping) starts at 0:00 on that frame and
	 * plays across the opening fights and gaps (the combat gate and intensity gain don't touch it; a retake doesn't
	 * restart it) until wave TutorialEndsAfterWave is cleared, then fades over TutorialFadeOutSeconds on a voice that
	 * survives level travel. CombatMusic takes over from the next wave. A Continue never plays it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Music")
	TSoftObjectPtr<USoundBase> TutorialPrelude;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Music")
	TSoftObjectPtr<USoundBase> TutorialLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Music", meta = (ClampMin = "0"))
	float TutorialFadeOutSeconds = 20.f;

	/**
	 * Issue #24 (contract agreed with Audio): ONE continuous source (prelude + loop, 95 s) in a MetaSound Source whose
	 * Wave Player loops source TutorialMusicDownbeatSeconds .. end (0:38 for 57 s) and whose Float input "StartTime"
	 * (seconds into the source) feeds the Wave Player's Start Time. When set, it replaces TutorialPrelude / TutorialLoop:
	 * a watched intro enters so the downbeat lands on the control frame; a skip (or no intro) enters at the downbeat
	 * with TutorialMusicSkipFadeInSeconds. The asset carries its own gain and the Music sound class.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Music")
	TSoftObjectPtr<USoundBase> TutorialMusic;

	/** Source time (s) of the downbeat that must coincide with control. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Music", meta = (ClampMin = "0", Units = "s"))
	float TutorialMusicDownbeatSeconds = 38.f;

	/** Fade-in (s) when the music enters at the downbeat on a skipped intro (Nicholas: 150 ms). Watched: none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Music", meta = (ClampMin = "0", Units = "s"))
	float TutorialMusicSkipFadeInSeconds = 0.15f;

	/**
	 * #146: an optional bed under the watched intro's opening, the part before the tutorial score comes in (the score is
	 * timed so its downbeat lands on control, so the first seconds of the intro have no music of their own). Starts with
	 * the watched intro's clock; fades out over IntroOpeningBedFadeSeconds when the tutorial score starts, and over
	 * 0.3 s when the intro is skipped, ends or is cut short. Empty = silence (as today). The asset carries its own class
	 * and volume. Not used when IntroMusic plays (a run without the tutorial score).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro")
	TSoftObjectPtr<USoundBase> IntroOpeningBed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro", meta = (ClampMin = "0", Units = "s"))
	float IntroOpeningBedFadeSeconds = 2.f;

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

/**
 * On-foot sounds (ENG-35, review M11): the pilot's steps, dash, jump, landing and hurt, the crew rifle shot and the drone
 * hum, as cue slots. Looked up as DA_Ground_FX (Presentation, else the engineering copy under Gameplay/Characters/FX);
 * an empty slot falls back to engineering's sound in code, so nothing goes silent. Owner of the class: Teammate 5.
 */
UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDGroundFX : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** A step on soft ground (Verdance). Played at the feet every stride. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pilot") FTDFXCue StepSoil;
	/** A step on rock (Cinder). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pilot") FTDFXCue StepRock;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pilot") FTDFXCue Dash;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pilot") FTDFXCue Jump;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pilot") FTDFXCue Land;
	/** The player's own hurt: its sound plays in 2D (one per burst of hits). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pilot") FTDFXCue Hurt;
	/** A crew bolt leaving the muzzle (one per volley). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crew") FTDFXCue CrewRifleShot;
	/** The drone's hover loop, attached, faded out on death. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crew") FTDFXCue DroneHum;
};
