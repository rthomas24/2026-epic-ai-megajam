# Native AirSlam authoring setup

Assigned roots published by Lead792d1f90. UE5.9 CL60351404, private engine-only
host Saved/H00/AirSlamAuthoring.uproject, Nick port8024. No current game module
or shared runtime assets are loaded into this small authoring host.

prepare_host.py writes the task-owned host descriptor/private renderer config
and mounts ONLY Content/TriadDrift/VFX/CombatReadability01/AirSlam01. Existing
descriptor/config differences or a wrong mount stop without overwriting them.
No native launch occurs during preparation. Source integrity is recorded in
Saved/native_pins.json. This is integrity evidence, not an approval requirement.

run_native.py is dry unless --execute is supplied with the current manifest
SHA and a unique scheduler --slot. The scheduler serializes jobs on this16GB
machine; it is not a visual approval gate. Fresh4.25GiB/no competing heavy job,
one shader worker, matching5.9 engine, unique UserDir and owned-PID cleanup are
enforced. Stop after freeRAM<1.75GiB for10s or an outer360s(probe)/480s(material)
bound. No automatic retry; partial outputs and evidence are preserved.

Stages:
- probe: read installed RadialBurst summary/topology/data into JSON; no package save.
- materials: probe, create/compile the two owned materials, validate statistics,
  then save only M_AirSlam_Ring and M_AirSlam_Dust. Ring is unlit/additive;
  dust is translucent/lit with roughness1 and no emissive input.
- verify-materials: fresh load/statistics, no package writes.

Unknown topology/async tool results stop instead of spinning on the editor
thread. A materials-only success is explicitly NOT a finished Niagara system.
The next system-authoring pass uses the actual template topology rather than
assuming emitter input names, facing modes or vector struct representations.

Every run checks its actual process result0/ok/errors0/dialogs0, log digest0,
protected host/projectSource/Config/Content/Binaries unchanged, and exact
per-stage output set. The read-only resource/hash helper is imported from the
frozen NativeDraft02 without modifying its file; the task uses its own port
and own PID identity guard. No muzzle effects or other frozen checkpoints run.

Default game-floor contact, surface orientation and data-asset binding stay
with engineering under N040. No Source/config/map/DA writes are made here.
Once the complete system is technically verified, publish and notify the team
under Nick's standing authorization; no further permission wait is needed.
Actual game capture must follow binding in a matching runtime. Hero remains
paused and Combat/Verdance unchanged. No GPU timing or game acceptance yet.

First native attempt materials_1794592270815482300 exited125/no_progress
during cold engine material/shader startup, before Python ran. It created no
native output and changed no protected input; minimumfree3.460GiB. One shader
worker was active. The source progress limit is now300s to accommodate this
startup, inside the unchanged420s engine/480s outer material-stage bounds.
This is not a successful material compile; failed evidence and cache remain.
