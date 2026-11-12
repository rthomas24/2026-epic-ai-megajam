TRIAD DRIFT / NATIVE SPACE HUD MILESTONE
UI/UX & HUD Agent / Nicholas Tosti

Implementation: Plugins/TriadDriftHUD, enabled by default, no uproject edit.
Dedicated branch/worktree: nick/hud-native / GameJam/Worktrees/hud-native.
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
For this 16 GB PC, prefer direct Build.bat with -MaxParallelActions=2.
Test with Tools/td-test.ps1 -Filter TriadDrift.UI.

TRY THE NATIVE PROTOTYPE (no shared map edits, no editor GUI)
UnrealEditor.exe <absolute uproject> /Engine/Maps/Entry?game=/Script/TriadDriftHUD.TDHUDPreviewGameMode
  -game -windowed -ResX=1920 -ResY=1080 -NoLiveCoding -nosound

1/2/3 select ship. H previews damage and missile warning; K confirms a kill;
P starts signature cooldown; R toggles another ship down (auto-respawn after
8 seconds); F5 resets mock data; F6 toggles reduced motion. Pad: LB/RB switch,
X damage, A power, Y kill. UI observes device changes without consuming input.
Every preview is explicitly labelled MOCK DATA and only the opt-in preview
game mode enables it. It never changes real gameplay or its event bus.

AUTOMATED VISUAL QA
Add -TDHUDCapture -RenderOffscreen -unattended to the preview command.
It captures five states in Saved/HUDReview then quits: normal 1080p, damage/
warning/kill, 720p respawn/reduced motion, ultrawide, and unavailable telemetry.
Images in Saved are verification artifacts, not tracked runtime content.

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
Nicholas explicitly authorized the narrow split and branch-only native assets
Nov 12, overriding the older main-claim-publication requirement for this task.
The claim is internal and NOT Ryan acknowledgment. Lead has been notified.
No shared project settings, source modules, maps or gameplay interfaces changed.
No main merge is authorized. Secondary menus remain outside this milestone.

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
