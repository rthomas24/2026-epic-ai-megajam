# Presentation integration decisions — November 12, 2026

Lead-coordinated response to MESSAGEBOARD R-001/002/003, technical replies published
in `52d5a83`. This records naming and integration boundaries, not an asset dispatch.
Existing CLAIMS.md ownership and human approval gates remain authoritative.

## Planned instance paths

First ship/weapon native files will use `Content/TriadDrift/Presentation/<Name>.uasset`, with object
references `/Game/TriadDrift/Presentation/<Name>.<Name>`.

| Class | Planned asset names |
|---|---|
| UTDShipVisuals | DA_Rift_Visuals; DA_Lancer_Visuals; DA_Bastion_Visuals |
| UTDWeaponFX | DA_PhasePulseLasers_FX; DA_SplitterBolt_FX; DA_NeedleCannons_FX; DA_SpikeRockets_FX; DA_BreakerBlaster_FX; DA_FlakBurst_FX |

Weapon names match TDShipLibrary IDs reviewed at main `802aa8c`. No instances have
been created or promised as present. Engineering defaults must remain optional and
null-safe until exact paths are claimed, assets authored, and integration validated.
Remaining enemy/ability/travel/blade/UI names require agreement against actual IDs
before a future dispatch; no additional required defaults are established here.

## Current integration boundaries

- Leave engineering placeholder planets/asteroids visible even with SpaceFoundation;
  the background sky does not replace them. Current environment authoring is isolated.
  Later shared placement needs published map/new-OFPA-actor claims, map-owner
  sunlight/exposure coordination, and no World Settings, GameMode or shared umap save.
- HUD at `6d9739e` is an isolated reflection/polling prototype, not a complete typed
  provider. Its public current schema is TDSpaceHUDWidget.h; pushed squad data alone
  is overwritten by its sampling timer. Typed provider/frame/event APIs require a
  separately approved UI revision; engineering owns gameplay adapter/host. Ryan must
  decide activation, dependencies, host lifecycle and packaging before integration.
- SpaceFoundation needs no Presentation data asset. Material VFX prototypes need
  Niagara wrappers before filling UNiagaraSystem fields; that work is not dispatched.
- Three switch audio candidates remain branch-only pending Nicholas audition.
  OnSwitchCut plus UTDShipVisuals.SwitchIn/SwitchOut are the future integration hooks;
  choose one playback route to avoid double firing. No audio-system edits authorized.
- Lancer modular assembly and centre-pivot rocket offset are open engineering/human
  decisions in N-001. No socket, gameplay, collision or spawn logic changes made.

Board is an append-only communication log; this file records the technical plan.
Human HUD activation and final art/audio acceptance remain unresolved approvals.

## Engineering replies adopted during live audit

R001–004 closed by Ryan's Lead on main8cd1561/e589416. Placeholder bodies stay
visible; current Galaxy has no placed sunlight/exposure Actor, so future sky
placement may own these after exact map/OFPA claims. No placement is assigned here.
HUD reflected names/index order frozen by engineering; they own the AHUD/controller
host. Activation/dependencies/cooking remain Ryan decisions, not granted by closure.
Engineering plays SwitchIn/SwitchOut cues at the cut; do not also play widget audio.

N001 decision acted on: Lancer can use Hull as primary Mesh plus10Parts with
ParentSocket=None and existing ship-root instances[].position_cm transforms,
identity rotations/unit scales. Ships confirmed static composition preserves all
mesh pivots and33component-local locators. Dynamic rotor parenting and component
muzzle lookup remain future engineering wiring. ProjectileMeshOffset=(54,0,0)
on future DA_SpikeRockets_FX moves visual only; spawn/collision/socket unchanged.
No native Lancer asset or gameplay changes are authored by this decision.

R005 names accepted as flat optional paths under /Game/TriadDrift/Presentation/:

| Class | Additional planned names |
|---|---|
| UTDEnemyVisuals, space | DA_Enemy_Fighter_Visuals; DA_Enemy_Interceptor_Visuals; DA_Enemy_Gunship_Visuals; DA_Enemy_Bomber_Visuals; DA_Enemy_Carrier_Visuals |
| UTDDominionVisuals | DA_Dominion_Visuals |
| UTDEnemyVisuals, crew | DA_Crew_Grunt_Visuals; DA_Crew_Trooper_Visuals; DA_Crew_Heavy_Visuals; DA_Crew_Drone_Visuals; DA_Crew_Warden_Visuals |
| UTDAbilityFX | DA_Ability_Cloak_FX; DA_Ability_EMPPulse_FX; DA_Ability_GravityWell_FX |
| UTDTravelFX / UTDBladeFX / UTDUIStyle | DA_Travel_FX; DA_Blade_FX; DA_UI_Style |

Flat crew paths supersede the older R004 Crew/ suggestion. Names are optional
lookup conventions, not a new asset dispatch or existing asset guarantee.
Signature powers use ship PowerWindup/PowerActive/PowerEnd/Power2Active cues.

R004 frontend preference: optional style soft classes with engineering load/cook
handling and game-mode fallback; MainMenuWidgetClass/LoadingWidgetClass, crew
AnimBPClass/Stagger/Death and BladeMesh now exist per engineering's closed reply.
Existing BladeColor/Retract reused; ragdoll remains gameplay-owned. No menu/crew/
blade/dressing production assigned. Planet size/capsule/mantle/slope/cover supplied
by engineering remain unchanged. Audio M02 was rejected; only M03 refinement is active.
