# Environment & VFX responsibility handoff

Read-only reference, 2026-11-12. This records completed work and boundaries; it does
not launch development, relax claims, grant a heavy-tool slot or authorize a main merge.
Lead Coordinator coordinates Nick's agents and integration, rather than producing
their assets. The dedicated Environment & VFX Agent owns future work in this lane.

Verified authorization update: Nicholas approved the branch-only native environment
exception in the new chat's user message `01a1d0ce-e918-7920-8372-0ccb794f01e6`.
It covers ONLY `ArtSource/Environment/SpaceFoundation/UnrealNative/**` and
`Content/TriadDrift/Environment/SpaceFoundation/**` on `nick/environment-vfx`.
The general gate below still applies to other lanes, including future VFX binaries.
Heavy-tool scheduling and main-merge approval remain separate gates.

## Preserved checkpoint

- Canonical source review: `nick/visual-assets`, checkpoint `e2d4139` before this handoff.
- Existing source PR: https://github.com/rthomas24/2026-epic-ai-megajam/pull/2
- Pending claims PR: https://github.com/rthomas24/2026-epic-ai-megajam/pull/1
- Environment source commit: `4795dd9`; VFX source commit: `e5f261d`, preview fix `a9d3ecb`.
- Legacy `nick/environment` and `nick/vfx` worktrees are clean, completed and frozen.
  Do not resume those workers, rewrite their commits, move assets or regenerate copies.
- Open [the source review entry point](../../ArtSource/START_HERE.html).

## Existing deliverables: reuse read-only

| Directory | Completed sources | Validation / remaining work |
|---|---|---|
| `ArtSource/Environment/SpaceFoundation/` | `generate_sky.py`, `build_unreal.py`, `presets.json`, `preview.html`, `README.txt`, `credits.json` | Deterministic sky authoring; seam/pole/range checks and source syntax passed. Two textures, one unlit sky master and three instances are planned, not generated. Browser composition reviewed; no native shader/save/reload/performance validation. |
| `ArtSource/VFX/StarterKit/` | `build_unreal.py`, `preview.html`, `README.md`, `credits.json`, four `shaders/*.ush` files | Engine trail, muzzle flash, hull impact, shield impact. Source/mock controls reviewed. Four masters plus twelve squad-color instances are planned, not generated. No Niagara systems or native shader compilation yet. |

Original procedural work; no downloaded textures, models, audio or fonts bundled.
Per-kit credits and root `CREDITS.md`/`DEVLOG.md` retain provenance. Browser previews
are approximations, not Unreal screenshots. Sky textures are LDR art, not calibrated HDR.
Builders refuse existing asset names. Preserve that protection and the original source files.

## Exclusive ownership and exclusions

The first native environment milestone can own only:

- `ArtSource/Environment/SpaceFoundation/UnrealNative/**` for new adapters/reports/review outputs.
- `Content/TriadDrift/Environment/SpaceFoundation/**` for approved native output.

Existing SpaceFoundation files outside `UnrealNative/` stay read-only. Old broad source
claims must be reconciled by Lead as a frozen-source/native-output split before editing;
they must not be silently deleted or treated as active competing writers.

The future VFX lane, when that milestone is authorized, is:

- `ArtSource/VFX/StarterKit/UnrealNative/**` and `Content/TriadDrift/VFX/StarterKit/**`.
- Existing VFX source/shaders outside `UnrealNative/` remain read-only inputs. Request an
  explicit source-file assignment before changing them; do not fork/recreate the kit.

Read-only other owners:

- UI/UX: `Plugins/TriadDriftHUD/**`, `ArtSource/UI/TriadHUD/Unreal/**`,
  `Content/TriadDrift/UI/TriadHUD/**`; original browser HUD is a frozen reference.
- Ships & Weapons: `ArtSource/Space/Ships/Lancer/**` and its attachment manifest;
  no ship/weapon geometry or loader edits by Environment & VFX.
- Audio/music: existing team responsibility, **not transferred**. `ArtSource/Audio/StarterPlan/**`
  remains frozen and no new audio owner or acquisition is assigned here.
- Ryan: gameplay `Source/`, `Config/`, gameplay content and frozen interfaces. Shared
  maps, project files and persistent actors require separate approval. No `L_Galaxy` edits.
- Lead: coordination handoff, integration reviews and root provenance aggregation.

## Safest workspace arrangement

Use a new `nick/environment-vfx` branch in
`C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/environment-vfx`, if still available.
Lead should provision it from the reviewed visual-assets checkpoint including this
handoff, then integrate reviewed `origin/main` into that **new clean branch**, checking
overlap first. This preserves source history without cherry-picking duplicate kits.
Do not work in Lead's or the old source workers' worktrees. Do not auto-pull/rebase an
active worktree. Any workspace creation/integration is coordinated through Lead.

Provisioned by Lead during handoff: the recommended worktree/branch now exists at
`e0868df`, preserving this source history and merging reviewed `origin/main` at
`571d85f` into the new branch only. No conflicts or source-kit changes occurred.
This is workspace setup, not a new development dispatch or a merge into main.

Until PR #2 is integrated into main, the new branch will inherit its source changes.
At end-of-day review integrate PR #2 once, then only the Environment/VFX follow-up
delta; do not separately merge equivalent legacy worker commits. Nicholas must
explicitly approve any main merge after Ryan's current work is checked.

## Gates and safe next milestone

1. Confirm exact ownership and latest local/remote claims. Current main does not
   contain the visual claims. Shared AGENTS.md §6.9 requires main-published claims
   before binary editing; alternatively obtain Nicholas's explicit narrow exception.
   UI and Lancer permissions are specific to those agents and are not transferable.
   The verified environment-only exception above resolves this gate for the first
   native sky milestone, but does not publish or release any other teammate claim.
2. Remain read-only until workspace/ownership is confirmed. No heavy window is
   granted here. HUD currently owns the sequential compile/test/capture slot; a
   Lancer diagnostic rerender is queued afterward. Recheck the live queue with Lead.
3. First authorized native milestone: reuse the existing sky generator and builder
   for two sky textures, one master and Hero/Combat/Verdance instances. Keep any new
   adapter/output in the assigned `UnrealNative/` lane; original files stay untouched.
   No shared map or configuration edits are needed for material-preview validation.
4. Verify target UE 5.9 CL60351404, compile shaders, save/reload, seams/poles, exposure,
   target readability and resource cost. Capture review evidence only when authorized.
5. After sky acceptance, recommend the highest-value combat VFX conversion. Existing
   `FTDFXCue.VFX` and tracer slots in Ryan's `TDPresentation.h` accept `UNiagaraSystem`;
   material prototypes alone cannot be assigned. Niagara wrappers can be authored
   independently. Confirm effect lifetime/time source, attachment conventions and
   final event wiring with Ryan before gameplay integration, without altering his API.

Lightweight safe work: read existing sources, compare parameter/contract names,
inspect provenance and prepare validation expectations. No new feature production
or binary authorization is implied by this handoff. Latest review found no incoming
Ryan changes in these visual paths; repeat that check before actual editing.
