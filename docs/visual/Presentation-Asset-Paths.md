# Presentation asset lookup contracts

These are stable lookup names and integration boundaries, not a task assignment or a statement that every asset exists. Current owner/path authorization is in [CLAIMS.md](../../CLAIMS.md); acceptance is in [STATUS.md](STATUS.md). The previous chronological plan is [archived](archive/2026-11-13-cdea6df/Presentation-Asset-Paths.md).

## Lookup names

Preferred project path: `Content/TriadDrift/Presentation/<Name>.uasset`, object reference `/Game/TriadDrift/Presentation/<Name>.<Name>`. Use the exact names below, not `DA_Ship_*_Visuals`. Optional empty slots preserve engineering fallbacks.

| Class | Exact names |
|---|---|
| UTDShipVisuals | DA_Rift_Visuals; DA_Lancer_Visuals; DA_Bastion_Visuals |
| UTDWeaponFX | DA_PhasePulseLasers_FX; DA_SplitterBolt_FX; DA_NeedleCannons_FX; DA_SpikeRockets_FX; DA_BreakerBlaster_FX; DA_FlakBurst_FX |
| UTDEnemyVisuals, space | DA_Enemy_Fighter_Visuals; DA_Enemy_Interceptor_Visuals; DA_Enemy_Gunship_Visuals; DA_Enemy_Bomber_Visuals; DA_Enemy_Carrier_Visuals |
| UTDDominionVisuals | DA_Dominion_Visuals |
| UTDEnemyVisuals, crew | DA_Crew_Grunt_Visuals; DA_Crew_Trooper_Visuals; DA_Crew_Heavy_Visuals; DA_Crew_Drone_Visuals; DA_Crew_Warden_Visuals |
| UTDPilotVisuals | DA_Pilot_Visuals |
| UTDAbilityFX | DA_Ability_Cloak_FX; DA_Ability_EMPPulse_FX; DA_Ability_GravityWell_FX |
| UTDTravelFX | DA_Travel_FX |
| UTDBladeFX | DA_Blade_FX |
| UTDUIStyle | DA_UI_Style |

The player uses **UTDPilotVisuals / DA_Pilot_Visuals**, not the earlier proposed DA_Player_Visuals or the crew class. Current Core documents a fallback lookup under `/Game/TriadDrift/Gameplay/Characters/Pilot`; engineering also supplies crew looks in Gameplay/Characters. Those published engineering assets do not transfer ownership of Nick's Presentation folder. Confirm current source lookup/fallback and cook behavior before adding an overriding instance.

## Native/runtime contracts

The current field definitions are [Core/TDPresentation.h](../../Source/unrealGameJam/TriadDrift/Core/TDPresentation.h); runtime consumers and [features.md section 3](../../features.md#3-shared-contracts) define behavior. Additive API changes are coordinated through Ryan's Lead under AGENTS; do not rename/remove established interfaces unilaterally.

- **Ships:** Mesh or modular Parts; preserve agreed component-local frame names, units, axes and transforms. The legacy Lancer contract supports Hull plus ten Parts. The agreed `ProjectileMeshOffset=(54,0,0)` is a visual-only SpikeRocket offset, not a spawn/collision change. A new design still needs its own verified manifest/approval.
- **Crew:** SkeletalMesh, AnimBPClass, Stagger and Death are published hooks. **Pilot:** its own optional mesh/animation contract; do not assume every imported skeleton is Manny-compatible.
- **HUD:** engineering owns the host, data producers, default activation and fallback coverage. UI consumes the agreed public provider; each newly covered surface needs a coordinated coverage flag so fallback information is not duplicated or lost.
- **Front end:** MainMenuWidgetClass, LoadingWidgetClass, PauseWidgetClass, VictoryWidgetClass and SettingsWidgetClass are separate style slots. MainMenuWidgetClass must not replace victory statistics. Preserve existing code-built fallbacks and verify the actual return-to-menu route when assigning a menu.
- **Music:** MenuMusic is an optional soft SoundBase reference played by the menu game mode. The selected track/class lives under Audio/MenuTheme01; looping/style assignment readiness is tracked separately from PR9's asset availability. Preserve other style fields during serialized edits. Current music class is `SC_MenuTheme_Music`; the Combat01 SFX class is `SC_Combat01_SFX` under its claimed Mix root. Coordinate volume hookups without renaming approved packages.
- **Intro:** IntroSequence, IntroMusic and IntroNarrationVoice are optional UTDUIStyle fields; their existence does not dispatch new cinematics or voice work.
- **Sky:** SpaceFoundation does not require a Presentation DA. A real sky actor tagged `TD.Presentation.Sky` yields the engineering placeholder sky/lighting. Claim persistent placement/map actors before saving; a temporary dev-hook test is not persistent integration.
- **VFX/audio:** Niagara slots require UNiagaraSystem assets; material prototypes cannot fill them. Each 3D sound carries its own attenuation/class/concurrency. SwitchIn/SwitchOut are played by gameplay; do not also fire the same switch cue from UI.

## Ownership and acceptance

Nick's presentation instances and assets, engineering fallback assets, shared maps and runtime Source/Config remain distinct ownership lanes. An absent instance does not grant write permission. Publish exact claims before edits and serialize shared binary/style field handoffs.

`/Game/TriadDrift/Presentation` is always cooked with its references. Other packages require the appropriate references/cook rules; loading a path in an editor is not proof it will ship. Validate the target package and gameplay behavior before claiming integration or completion. Current production readiness is recorded once in STATUS, not duplicated here.
