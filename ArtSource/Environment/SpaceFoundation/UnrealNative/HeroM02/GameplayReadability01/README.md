# Hero gameplay readability 01 — source preparation

Status: source prepared; **not executed in Unreal**. Lead published the bounded task
at main `55e6046`. Hero checkpoint `5fde8c3`, all 22 existing evidence/source/package
files, and all Combat/Verdance assets remain frozen. No look tuning is permitted.

This is a controlled test on the Lead's compiled `day1-integration` candidate with
the merged HUD from PR8. It is not persistent map integration, final art approval,
Shipping/cook validation or full-journey acceptance.

## Required host inputs

Lead mounts the exact six runtime packages in `mounts.json` at their existing
`/Game/TriadDrift/Environment/SpaceFoundation/HeroM02/` paths. The review map is
excluded. The script never copies, imports, builds, saves or mounts an asset.
Original M01 Hero material and dependencies, actual `L_Galaxy`, current compiled
game module, merged HUD plugin and saved HUD WBP must already exist in the host.
LFS pointer files are invalid inputs.

Use `launch.ps1 -HostProject <absolute day1-integration/unrealGameJam.uproject>
-LeadGrant <fresh Lead slot token>` for a read-only preflight. **Omitting `-Execute`
never launches Unreal.** Actual execution additionally requires `-Execute`, a free
serialized slot, at least 4 GiB available RAM and no competing Unreal, Blender,
shader compiler or C++ compiler. Source readiness is not a slot grant.

## Bounded execution

One hidden-start editor on private port 8024 loads actual `L_Galaxy`. A startup
Python module registers one bounded editor-only probe through Epic's native
ToolsetRegistry; no Python console endpoint is exposed. All MCP calls are serial.
Background CLI is the default; the native PIE-window exception is necessary here
to validate actual gameplay and Slate/UMG viewport readability, not for asset work.
The script discovers the registered toolset and records the actual schemas.
`-UserDir` redirects Unreal's user/saved/intermediate state into this unique owned
run directory. The probe verifies that resolved ProjectSavedDir is inside it before
placing anything; automatic gameplay save writes cannot affect existing human saves.

The probe disables autosave in memory and records/restores the prior settings.
It starts normal in-process floating-window PIE through `EditorAppToolset`, with a
possessed player, then spawns its own world-owned runtime rig at identity directly
in that actual PIE world. No editor-world actor is placed. After more than 0.5
seconds of unpaused game time, it verifies exactly the frozen tagged rig,
placeholder lights/stars have yielded, and the saved HUD WBP is visible. Other
map directional lights/unbound post-process components are recorded, never removed.

Native spawn uses `GameplayStatics::BeginDeferredActorSpawnFromClass` and
`FinishSpawningActor`. Their `BlueprintInternalUseOnly` metadata excludes normal
Python attributes (`PyGenUtil.cpp:2154`). The installed engine's documented
`Object.call_method` bridge (`PyWrapperObject.cpp:1218–1289`) resolves these reflected
UFUNCTIONs by name and builds temporary Python parameter glue. This passes the
actual PIE world, identity transform, AlwaysSpawn, no owner and MultiplyWithRoot.
Both the world and loaded material object are checked directly after spawning.
The generic reflection bridge is used only for these two fixed native calls;
the bounded probe exposes no arbitrary function-call or Python endpoint.

The existing-material statistics call waits for SM6 shader resources before PIE;
it does not reauthor/save a material. Command-line worker limits target one shader
compiler on this 16-thread machine and retain stock memory-pressure settings.

M01 and M02 are **material-only A/B variants under the same Hero rig lighting**:
8 lux, 6500 K sun at pitch -25/yaw -35; manual exposure bias 0 with physical camera
exposure disabled; bloom 0.2, vignette/motion blur 0. The rig has no skylight
component: the older `sky_light_intensity` metadata does not create one. All M02
scalar gains stay 1 and tint stays white. No material-instance or CDO edits occur.

The first two 1920×1080 PNGs capture the same paused game snapshot with M01 and M02.
The native viewport capture includes UMG; any capture warning, wrong resolution,
PNG CRC/tail defect or camera/time/exposure mismatch fails instead of being repaired.
Check the actual image before continuing. In the unique output directory, write
`initial_gate.json` with `readable: true`, a named `reviewer` and concrete `notes`
only after inspecting both images. A false/missing decision stops extended testing
and closes the editor after at most 90 seconds. This is an operator inspection gate,
not an additional user approval request.

After that gate only: eight sparse native frame-timing samples per material (about
2 seconds per variant after 1 second of settling), a short boost/primary-fire pass,
all three ship switches/signature-power inputs, and one further matched turned-view
pair. Input uses the existing real `TD.Input` route. It does not unlock abilities,
skip stages, change difficulty, inject target actors, use god mode or load/save a
human progression file. A gameplay action is only verified if the
resulting image/log supports it. A selected target is not proof of target lock.

PIE is then stopped and restarted once. The probe requires a different UWorld and
spawns its own rig again there; it requires the original M02 material on that actor,
with HUD present, takeover completed and no duplicate sky.
Before stopping PIE, the probe releases Python actor references and stores only
the old world description/native address string, so it does not retain an old
UWorld through garbage collection. It confirms no game world after StopPIE before
checking the new world's description; an unchanged native representation fails
conservatively rather than being treated as proof of recreation.
This verifies world recreation in the same process; the previous isolated Capture04
remains the separate six-package fresh-process reload evidence. World Partition
does not serialize RF_Transient editor actors into its PIE copy (`Level.cpp:721`);
no such duplication or persistent-placement workaround is used here.

## Outputs and preservation

Each execution creates a new ignored directory under
`HeroM02/Saved/GameplayReadability01/<UTC timestamp>/`. It contains launch arguments,
actual tool schemas, PNGs and native capture metadata, before/after view snapshots,
raw timings and small-sample summaries, the initial inspection decision, engine
log/result/markers/digest, process exit and host input hashes. Existing benchmark
captures and `compare.html` are never overwritten. Expected maximum: nine PNGs.

Cleanup releases input and ends PIE, which destroys its own runtime actor,
restores its in-memory settings, then requests editor exit. No save API or save
command is called. After actual process exit, compare host Source, Config, maps,
external actors/objects, environment packages, project descriptor and Git status;
recheck all 22 frozen checkpoint hashes. Differences are reported and never reset.
Only this exact child process tree may be terminated at the outer cleanup deadline.

Sparse samples include live AI/wave variation and RPC effects. They are approximate
observations, not a causal FPS improvement, benchmark or total VRAM-residency claim.
Zero/unsupported GPU timing is omitted from summaries. Warnings stay in the native
digest; no clean-run claim is inferred from screenshots. A technical pass requires
actual process exit 0, native result 0/reason ok, zero digest errors/no suppressed
dialogs, complete sequence, successful cleanup and unchanged protected inputs.

Fresh independent source review and the first native capability checks remain
pending. No engine run is authorized by this README.

## Acceptance handoff for Lead and the Galaxy owner

Source readiness: the two reported source blockers are corrected. Material checks
compare loaded objects. The test spawns directly in each live PIE world through
the installed reflected-call API; it does not depend on World Partition copying a
transient editor actor. Those fixes have syntax/source evidence, not native-run
evidence. The revised three files still require fresh review before a slot grant.

The six required packages are `BP_GalaxySkybox_HeroM02`, `MI_HeroM02_Benchmark`,
`M_HeroM02_Nebula`, `T_HeroM02_Back`, `T_HeroM02_Front` and `T_HeroM02_Stars`.
Their exact relative paths, sizes and frozen SHA256 values are in `mounts.json`.
Lead mounts them into the selected compiled host; the review map is excluded.

| Gate | Evidence required from the actual run | Acceptance limit |
| --- | --- | --- |
| Runtime takeover | One identity rig of the frozen generated class tagged `TD.Presentation.Sky`; more than 0.5 seconds of game time after spawn; no placeholder sky or stars | Additional presentation rigs or lingering fallback fail. Other map lights/PP are disclosed; do not remove them. |
| Matched A/B | M01 and M02 on the same live mesh, same paused world time, player camera transform/FOV, lighting and exposure; two full 1920×1080 UI-inclusive PNGs | Any mismatch, capture warning, missing UI, wrong dimensions or PNG defect fails. No repair or retuning. |
| HUD and enemy clarity | Saved HUD WBP with the squad provider; readable health/shield/ability text, target indicators and enemy/ship silhouettes against both dark space and bright gas | A HUD-only frame does not prove enemy readability. Missing/on-screen-unproven enemies, target lock or missile warnings remain explicitly unverified. |
| Combat in motion | Existing real input drives boost, primary fire, three ship switches and signature actions; captures/logs show the resulting behavior | Record which actions and squad/enemy colors are actually visible. Inputs alone do not prove successful powers, lock, hits or combat readability. Stop if the new sky obscures critical cues. |
| Runtime cost | Eight raw Frame/Game/Render/RHI/GPU timing observations per material after settling, with sample counts, medians and nearest-rank p95/max | Report M02 minus M01 only for comparable available counters; with eight samples p95 is the maximum. Zero GPU timestamps mean GPU cost unverified. Live-AI differences and RPC overhead preclude causal FPS claims. |
| World restart | StopPIE confirms no active game world; a new world description and newly spawned M02 rig pass takeover/HUD checks | Same-process world recreation only. No fresh-process, Shipping or complete-journey claim. |
| Preservation | Actual process exit/native result/digest plus before/after input hashes and Git status, with all frozen 22 paths unchanged | Any protected difference, error, suppressed dialog or forced termination prevents technical pass. Never save a map or reset a changed file. |

Initial image inspection decides only whether this bounded run continues. Final
gameplay-readability acceptance requires reviewing its actual enemy/combat frames
and measured cost, with missing coverage listed. The script's `technical_pass`
does not grant that visual acceptance. Current fleet models/effects are whichever
are already integrated in the Lead candidate; this does not validate future art.

After a passing run, send Lead one concise handoff with the exact host commit,
run-directory path, matched-frame paths, observed/missing combat coverage, timing
summary and preservation/result status. Lead can relay it to the T4 World and T5
map owners under N017. Persistent placement/cook ownership remains their separate
coordination question; no map-save or Main-promotion request is bundled into this
test. Stop there with Combat/Verdance unchanged pending Nicholas's decision.
