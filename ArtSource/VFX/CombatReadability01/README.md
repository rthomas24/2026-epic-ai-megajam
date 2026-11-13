# Compact muzzle burst — first combat VFX family

Stage: **source implemented; native assets, compile/reload and gameplay unverified**.
Owned paths were published by Lead in claims `e320f9c`. Primary Hero gameplay lighting
verification runs first; this preparation uses no native slot.

One texture-free six-ray flash replaces the StarterKit material's world-time
handshake with Niagara `ParticleRelativeTime`. It fades over 0.06 seconds, with a
small warm core, restrained emission and soft outer cutoff. One CPU sprite per
trigger, 36 cm diameter; no dynamic light, collision, ribbon, volumetrics or tick
Blueprint. Material instances expose `Color` and `Intensity`. Existing approved
magenta/cyan/amber variants are retained; future Rift redesign colors are not adopted.

`build_muzzle.py` creates only seven assets under
`/Game/TriadDrift/VFX/CombatReadability01/`: `M_MuzzleBurst`, three
`MI_MuzzleBurst_{Rift,Lancer,Bastion}` and three
`NS_MuzzleBurst_{Rift,Lancer,Bastion}`. It reuses Epic's installed RadialBurst
system scaffold and SimpleSpriteBurst emitter, replaces the emitter/render material,
removes extra particle-spawn/update shaping, limits count/size and overrides lifetime.
It refuses an unverified one-shot lifecycle, missing/noneditable template inputs,
non-sprite renderer, existing output or output mount outside this lane. A failure
is inspected before any retry; no template, shared asset, data asset or map is saved.

The Niagara editor toolset's installed reflected API and typed JSON converters are
used directly through ToolsetRegistry. The `struct.refPath`/`value` literal format
comes from NiagaraToolsetJsonConverters.cpp; topology/input names come from
NiagaraExternalSystemEditorUtilities.h. These are source references, not evidence
that this authoring recipe has run successfully. Actual template topology and
synchronous tool availability remain native capability gates.

## Authoring command for a later separate grant

Lead must first mount the owned writable output directory
`Content/TriadDrift/VFX/CombatReadability01` from this worktree into the compiled
day1-integration host at the same content path. The script verifies the resolved
directory matches this worktree. Do not mount the whole Content tree. No engine
or template assets are copied into Git. Use the existing engine headless-run/log
triage workflow with a fresh >=4 GiB/no-competing-heavy-process check.

Native command specification, **not executed**: pinned UE5.9
`UnrealEditor-Cmd.exe <day1-integration/unrealGameJam.uproject> -AgentMode
-run=pythonscript -script=<this folder/build_muzzle.py>
-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities,AllToolsets
-AllowCommandletRendering -d3d12 -sm6 -nosound -NoSourceControl -NoLiveCoding
-ModelContextProtocolPort=8025 -ExitTimeout=600 -NoProgressTimeout=180
-UserDir=<this folder/Saved/unique-run/RuntimeUser>
-abslog=<unique-run/Author.log> -ResultFile=<unique-run/Author.result.json>`.
Retain the reviewed one-worker CLI overrides and actual-process outer bound from
the existing launcher; no shared configuration edits. Lead schedules and owns this
separate authoring window. A pending async tool stops the recipe rather than
spinning the editor thread or claiming success. Do not launch this alongside Hero.

After authoring, require native system compile state/stack diagnostics, fresh
read-only reload of all seven assets, finite one-shot particle bounds/lifetime
inspection, and a representative native capture before any runtime binding.

## Serialized runtime handoff

Audio currently owns **Sound fields only** in `DA_NeedleCannons_FX`. This lane
does not load, create or save that shared data asset. After Audio releases it,
Lead can assign only:

| Field | Proposed value |
| --- | --- |
| `Muzzle.VFX` | `/Game/TriadDrift/VFX/CombatReadability01/NS_MuzzleBurst_Rift` |
| `Muzzle.VFXScale` | `(1,1,1)`, subject to the hull-size gate below |

Preserve every Sound, shake, impact, detonation and gameplay field. Current
`TDWeaponComponent` fires the existing cue and suppresses its placeholder when
authored visual art exists. `TDStatics::PlayCue` spawns at the muzzle world location;
it does not forward WeaponFX.Color or attach this cue to the moving ship. Hence
the palette is baked into each material instance and high-speed separation must
be checked in actual gameplay; no gameplay/attachment change is made here.

The current placeholder cap is **HullRadius / 3.6**, not /3. This authored effect
does not inherit that C++ placeholder cap automatically. Its visible radius is at
most 18 cm at scale1, so require actual firing hull radius >=64.8 cm, otherwise
reduce all Cue scale axes to at most HullRadius/64.8 before binding. Verify actual
sprite bounds rather than accepting the configured dimension as measured evidence.

Acceptance: short, readable flashes at normal/boost speed; enemy silhouettes and
targeting stay visible; squad colors are distinct; no looping/lingering particles;
no double placeholder flash; no shader/stack errors; acceptable native frame cost
with all three ships firing. Compare to the current placeholder at the same view.
Missing context remains unverified. No Main promotion or visual approval is implied.

Provenance: shader structure derives from the team's original read-only
`ArtSource/VFX/StarterKit/shaders/MuzzleFlash.ush`, adapted by Codex in this cycle.
Epic Niagara templates supply lifecycle scaffolding under the project's Unreal
Engine license. No external image textures or third-party media were copied.
Lead records the original shader/tool/template use in CREDITS before integration.
