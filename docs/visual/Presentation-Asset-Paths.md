# Presentation integration decisions — November 12, 2026

Lead-coordinated response to MESSAGEBOARD R-001/002/003, technical replies published
in `52d5a83`. This records naming and integration boundaries, not an asset dispatch.
Existing CLAIMS.md ownership and human approval gates remain authoritative.

## Planned instance paths

Native files will use `Content/TriadDrift/Presentation/<Name>.uasset`, with object
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
