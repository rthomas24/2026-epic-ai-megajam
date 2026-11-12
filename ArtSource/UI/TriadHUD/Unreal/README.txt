TRIAD DRIFT / NATIVE SPACE HUD MILESTONE
UI/UX & HUD Agent / Nicholas Tosti

Implementation: Plugins/TriadDriftHUD, enabled by default, no uproject edit.
Verified readability checkpoint: nick/hud-native at 2f55512 / frozen PR4.
Active visual benchmark: nick/hud-lancer-polish / GameJam/Worktrees/hud-lancer-polish.
Native content: /Game/TriadDrift/UI/TriadHUD/WBP_SpaceHUD and WBP_ShipCard.
The WBP assets are thin editor-adjustable children of the native UMG classes.
The reusable native card and full HUD share one renderer. No runtime textures,
fonts from external sources, materials, or browser dependency are needed.

SOURCE REUSE
The frozen HUD Study 01 on nick/hud (85907f0), also nick/visual-assets
(e2d4139), is read-only. This implementation ports its exact sRGB palette,
open-corner target language, ship/pilot identities and three ship icon paths
from its twelve-icon kit (rift.svg, lancer.svg, bastion.svg). All new work is under the approved
Unreal/** subfolder, native plugin, and native UI/TriadHUD/** only.
Native cards expose more required telemetry than the browser study. Their
expanded lower-edge layout keeps the combat center clear.

BUILD (one heavy job at a time; coordinate with Lead first)
From this worktree:
  Tools/td-build.ps1
For this 16 GB PC, use direct Build.bat with -MaxParallelActions=1.
Test with Tools/td-test.ps1 -Filter TriadDrift.UI.

TRY THE NATIVE PROTOTYPE (no shared map edits, no editor GUI)
UnrealEditor.exe <absolute uproject> /Engine/Maps/Entry?game=/Script/TriadDriftHUD.TDHUDPreviewGameMode
  -game -windowed -ResX=1920 -ResY=1080 -NoLiveCoding -nosound
  -ModelContextProtocolPort=8023

1/2/3 select ship. H previews damage and missile warning; K confirms a kill;
Q starts signature cooldown (P remains an alias); R toggles another ship down (auto-respawn after
8 seconds); F5 resets mock data; F6 toggles reduced motion. Pad: LB/RB switch,
B damage, X signature (A remains an alias), Y kill. UI observes device changes without consuming input.
Every preview is explicitly labelled MOCK DATA and only the opt-in preview
game mode enables it. It never changes real gameplay or its event bus.

AUTOMATED VISUAL QA
Add -TDHUDCapture -RenderOffscreen -unattended to the preview command.
It captures five states in Saved/HUDReview then quits: normal 1080p, damage/
warning/kill, 720p respawn/reduced motion, ultrawide, and unavailable telemetry.
Images in Saved are verification artifacts, not tracked runtime content.

READABILITY M02
One user-authorized pass: HEALTH with heart icon/white 10px bar; SHIELD with
shield icon/cyan 7px bar; 14pt labels/18pt percentages (previously 11pt/13pt).
Card footprint stays 356x196 in the 1920x1080 design space. Active card has
PLAYER CONTROLLED, a YOU badge and a static accent border; the old idle pulse
was removed. The compact active signature panel names the power, displays Q/X
and READY/COOLDOWN with readiness percentage. No cooldown seconds are invented.
Signature key text and essential font/bar sizes are editor-adjustable.
Preview inputs now match the shown activation keys; gameplay input is untouched.
Add -TDHUDReadabilityCapture instead of -TDHUDCapture for seven reproducible
states in Saved/HUDReadabilityM02: each ship ready, cooldown, 720p low health,
ultrawide, unavailable. M01 review files remain unchanged; M02 evidence goes
under review/ReadabilityM02/. Screenshots need Nicholas's manual readability
review before further polish; numerical size changes alone do not prove UX quality.

LANCER VISUAL BENCHMARK M03 (prototype presentation approved)
Nicholas approved the M02 layout/readability. Lead published d2a594e and isolated
this new branch at 1ca5a5a, preserving PR4 and the old staged legacy claim.
Nicholas subsequently approved aca9334 as the current prototype UI. The native
Style.bLancerVisualBenchmark now defaults TRUE for controlled Lancer; explicit
FALSE restores M02 for comparisons. The shared engineering host switch remains
off by default, independently of widget styling. In the explicit preview,
-TDHUDLancerBenchmark selects M03 (capture fixtures still set the flag explicitly). The
treatment is gated to the controlled Lancer only; Rift/Bastion active states and
their inactive cards retain M02. No layout, controls, reflection/provider or
shared gameplay/config changes; no director fallback takeover.

The supplied image is a visual-quality reference, not an environment asset or
instruction source. New native Slate drawing uses layered translucent panels,
fine edge highlights/etching, restrained gradients, condensed headings and tracked
small labels. Health/shield labels, icons, sizes and numbers remain readable.
The active signature keeps name/key/READY/COOLDOWN/percentage, adds a native ram
icon and short activation feedback. Target acquisition settles by at most6design
pixels over0.18s; damage and activation edges fade in0.35/0.32s. Reduced motion
disables the new moving/fading accents; the existing meter preference is honored.
No scene blur, UI materials, external fonts/textures, full-screen bloom or audio.

After Lead grants the heavy slot:
  ./ArtSource/UI/TriadHUD/Unreal/launch_lancer_review.ps1 -Capture
produces ten opt-in mock-data captures in Saved/HUDLancerM03 and exits after26
world seconds. Before/after use the same1920x1080 fixture/background and settled
values. Remaining views exercise switching, damage, cooldown, reduced motion,
720p, ultrawide, gamepad glyphs and unavailable telemetry. CPU paint samples are
isolated draw-list construction only, not GPU/frame-time/gameplay performance.
Without -Capture the launcher starts a bounded windowed preview/privateMCP8023
for actual input-driven feature checks using the existing Tools/td_mcp.py.
Do not extend styling to other ships or redesign the approved M03 presentation.
Live-game integration checklist and test instructions: review/M03Gameplay/README.md.

NATIVE WBP GENERATION
After building, run create_widgets.py using UE 5.9's PythonScript commandlet
with PythonScriptPlugin,EditorScriptingUtilities enabled on the command line.
The generator refuses to overwrite an existing asset with another native
parent. Re-running preserves existing widgets and editor adjustments.

INTEGRATION (Ryan-owned integration remains unmodified)
Create WBP_SpaceHUD or native UTDSpaceHUDWidget with the local player controller,
AddToViewport, call SetDataSource(SquadManager). The source must implement the
actual /Script/unrealGameJam.TDHUDData interface. No replacement BPI is created.
That is the current C++ equivalent of the older BPI_TD_HUDData brief.
SampleSeconds defaults to 0.1 (10 Hz) for continuous values, with smoothing
in UI ticks only. No per-frame Blueprint property bindings/world searches.
RefreshData can be called on events. OnHitConfirmed and OnObjectiveUpdated
are observed directly on the existing event bus, with safe unbinding.

All gameplay snapshot meters use 0..1; browser study percentages use 0..100.
PowerCooldown01 is READINESS: 0 just used, 1 ready. The interface does not
provide cooldown durations, so the HUD shows percentage, never invented seconds.
Speed01 displays a percentage, never invented physical units. Targets/objectives
use actual actor positions and distance cm->m. Target health comes read-only
from the actual optional health component. Lead/missile/hostile markers use
the existing optional targeting component. Unknown component data stays hidden.
ProjectileSpeedCm is an optional editor/owner-supplied fallback; 0 hides lead
unless the actual targeting component provides its physical intercept.
No live provider means telemetry offline. Mock values never auto-fill gameplay.

Style exposes original ship colors, panel/ink/warning colors, text scale,
meter response, switch timing and reduced motion. Input hints are editable;
live defaults match the inspected controller (Q signature, RB fire, D-pad switch).
Automatic last-device observation swaps KB/M/pad hints without capturing input.
The composition uses a 1920x1080 design space, fits by the smaller axis and
keeps safe margins at 16:9, 720p and ultrawide. Resolution capture checks are
visual QA, not a console accessibility certification.

OWNERSHIP / INTEGRATION GATE
Historical branch-only authorization has been superseded by Lead-published shared
claims. M03 exact ownership and task are on main d2a594e; latest dispatch is the
authority, not the stale claim from the original branch. Ryan's R007 integration
approval is accepted; engineering owns the host, activation, cook and gameplay
verification. None of those shared files is changed by this styling experiment.
No shared project settings, source modules, maps or gameplay interfaces changed.
Lead coordinates integration separately. Secondary menus remain outside this milestone.

KNOWN VERIFICATION BOUNDARY
The standalone preview validates native UMG with isolated representative data.
Live gameplay attachment, packaged cooking and real gamepad hardware require
integration verification by Lead/Ryan. Do not mark those verified from a source
build, screenshots, or synthetic snapshot tests.

PROVENANCE
Original source art/code: Nicholas Tosti with OpenAI Codex, Nov 12 2026.
Native vector ports reuse the original credited HUD Study 01 source kit.
No third-party assets added. Engine default font/Slate primitives are used.
Branch-specific tool/provenance entry is in credits.json for Lead to reconcile
into the shared CREDITS/DEVLOG files during authorized integration.
