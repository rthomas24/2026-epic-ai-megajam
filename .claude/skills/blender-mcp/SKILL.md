---
name: blender-mcp
description: Drives Blender 5.2 from an agent through the official Blender Lab MCP server (tools such as execute_blender_code, get_objects_summary, render_viewport_to_path, search_api_docs) or headless blender.exe. Covers connecting and troubleshooting, writing bpy code that works on Blender 5.x, running this repo's tested helper scripts, and verifying work with multi-view review renders, mesh-health reports and floating-part checks. Use whenever a task touches Blender, .blend files, bpy, the Blender MCP tools or 3D models in this repo, including when Blender tools fail to connect, hang or time out. Read it before the other blender-* skills.
compatibility: Blender 5.2.x (tested on 5.2.2 LTS, Windows 11). Live mode needs the Blender Lab "MCP" add-on 1.0.3+ and the Blender MCP server (MCP key "Blender"); headless mode only needs a shell.
---

# Working in Blender as an agent (Blender Lab MCP)

The rules below come from measured failures: our own tests on 5.2.2 plus the research in
`reports/Blender MCP game asset practices.md`. Follow them even when a task looks simple.

## The rules that matter most

1. **Inspect before you change anything.** Never assume names, units or values. The user's .blend
   holds their work. Never call `read_factory_settings`, and never delete or overwrite objects you
   didn't create without asking. Build new assets in their own collection.
2. **One small step per `execute_blender_code` call.** Each call gets a fresh namespace, so import
   `bpy` (and `bmesh`, `math`, `mathutils`) every time. Finish with `result = {...}`, a JSON-serialisable
   **dict**. Capture object names right after creating them: Blender silently renames clashes to `.001`.
3. **Check `status` in every response.** Errors come back as normal tool results with
   `status: "error"` and a message. Read `stdout` as diagnostics, not as success.
4. **Never resend a mutation that timed out.** The socket timeout is about 300 s, and the code may
   have run anyway. Re-inspect first, and keep each call well under 2–3 minutes. Run long bakes and
   renders headless.
5. **Measured numbers decide, images confirm.** After every phase run `asset_report.py` and
   `assembly_check.py` (numbers) and `review_renders.py` (one contact-sheet image). A render that
   looks fine is not proof: floating parts and inside-out meshes hide in some views.
6. **Write Blender 5.2 API, not tutorial-era API.** The top failure of LLM-written Blender code is
   calling APIs that were removed. Read `references/blender-5-api-gotchas.md` before your first
   script. Look anything uncertain up with `search_api_docs` / `get_python_api_docs` (bundled docs
   are 5.1) or by introspection (`dir()`, `bl_rna`).
7. **Don't use the `*_for_cli` MCP tools on Windows.** They hang until a 120 s timeout because of an
   unfixed upstream bug. Use the headless shell run below instead.
8. **Checkpoint, and keep a way back.** Save versioned files (`bpy.ops.wm.save_as_mainfile`)
   before batch operations and after every accepted stage, so you can revert when a "fix" makes
   things worse. Research found unguarded self-critique loops often degrade the asset.

## Pick a mode

| Mode | Use it when | How |
|---|---|---|
| **Live (MCP)** | The human has Blender open with the add-on running; interactive building they can watch | `execute_blender_code`, inspection tools, screenshots |
| **Headless (shell)** | No MCP connection, batch jobs, bakes, exports, renders longer than ~1 min, a teammate without the MCP | `blender.exe --background --factory-startup <file.blend> --python <script.py> -- <args>` |

Never save a .blend headlessly while the human has the same file open with unsaved changes. Ask
them to save and close it first, or do the work live.

**Find Blender:** use `$env:BLENDER_PATH` if set. Otherwise take the newest
`C:\Program Files\Blender Foundation\Blender 5.*\blender.exe` (on macOS,
`/Applications/Blender.app/Contents/MacOS/Blender`). Then run `& $blender --version` to check you
have 5.2.x.

## Connect (live mode) and verify

First check: call `get_blendfile_summary_path_info`, or `execute_blender_code` with
`import bpy; result = {"version": bpy.app.version_string, "file": bpy.data.filepath}`.

If it fails with "Cannot connect to Blender at localhost:9876", the add-on isn't running. Walk the
human through `references/setup-and-troubleshooting.md`:
1. Install the add-on from the `lab.blender.org` extensions repository.
2. Turn on **Preferences > System > Allow Online Access**. The bridge silently never starts without it.
3. Start the server.

Until that's done, continue in headless mode instead of stopping. Tool names in Claude Code look
like `mcp__Blender__execute_blender_code`. Other agents show them as `Blender:execute_blender_code`
or similar; the plain tool names in this skill are what matter.

## Tool map (official server)

| Need | Tool | Notes |
|---|---|---|
| What file is open, paths | `get_blendfile_summary_path_info` | cheap connection check |
| List objects | `get_objects_summary` | huge on big scenes (1,670 objects ≈ 900 KB); prefer filtered `execute_blender_code` queries |
| One object's details | `get_object_detail_summary(name)` | modifiers, materials, dims, parent |
| Datablock usage / missing files / libraries | `get_blendfile_summary_*` | |
| API and manual lookup | `search_api_docs`, `get_python_api_docs`, `search_manual_docs` | work even when Blender isn't running (local RST); docs are 5.1 |
| Build / modify | `execute_blender_code(code)` | the workhorse; see rules 2–4 |
| See the viewport | `jump_to_view3d_object_by_name` → `get_screenshot_of_area_as_image("VIEW_3D", size_limit_in_bytes=500000)` | black images if Blender is minimized (Windows) |
| UI layout as JSON | `get_screenshot_of_window_as_json` | |
| F12 render of the scene camera | `render_viewport_to_path` | despite the name it's a full render; output lands in `bpy.app.tempdir/blender_mcp/<basename>`, so read the returned `filepath` |
| Thumbnail | `render_thumbnail_to_path` | **broken in the live UI** (renders full size; wrong EEVEE id). Use `review_renders.py` |

## Run this repo's scripts

Scripts live in `<repo>/.claude/skills/<skill>/scripts/`. Each exposes `main(**kwargs)` returning a
dict, and also runs from a shell. Get the repo root with `git rev-parse --show-toplevel` and pass
**absolute** paths. Blender runs on the same machine, so it can read the files directly; don't
paste whole scripts into the tool.

**Live:**
```python
import runpy
m = runpy.run_path(r"C:\path\to\game-jam\.claude\skills\blender-mcp\scripts\asset_report.py")
result = m["main"](targets=["SM_Rock_A"], tri_budget=6000)
```
**Headless (PowerShell)** — the script prints one line starting with `__RESULT__` followed by JSON:
```powershell
& $blender --background --factory-startup "C:\...\Rock_A.blend" --python "C:\...\asset_report.py" -- --targets SM_Rock_A
```
In Git Bash, append `< /dev/null` so Blender doesn't inherit the agent's stdin.

**Self-test** (proves Blender and every blender-* script work on this machine, about 3 s; headless only):
```powershell
& $blender --background --factory-startup --python .claude/skills/blender-mcp/scripts/selftest.py -- --out-dir "$env:TEMP\blender_selftest"
```

## Verification kit (scripts/)

| Script | What it tells you | Typical call |
|---|---|---|
| `review_renders.py` | One contact-sheet PNG from fixed angles in a temporary scene; the user's scene is untouched. Modes: `clay` (form and silhouette), `material` (EEVEE, materials and glow), `wireframe` (topology). Orthographic views share one scale (`ortho_cm_per_px` returned); perspective views auto-crop. | `main(targets=[...], out_dir=..., label="rift_v03", mode="clay")` |
| `asset_report.py` | Evaluated (post-modifier) tri counts, n-gons, non-manifold / flipped / inside-out geometry, duplicate and loose verts, unapplied or negative scale, origin placement, size in cm, UVs, empty material slots, bad names, tri budget | `main(targets=[...], tri_budget=...)` |
| `assembly_check.py` | Floating parts: islands that don't touch the main body (with gap in metres) | `main(targets=["SM_Ship_Rift"])` |
| `selftest.py` | End-to-end smoke test of all blender-* scripts | headless only |

How to read a contact sheet: cells run left-to-right, top-to-bottom in `layout.order` (default
`front_3q, front, right / top, back_3q, low_3q`). "Front" means the camera is on -Y looking +Y,
Blender's Front view. Look at **3–4 images per turn at most**, since images are expensive in
context. Glow colours look paler than in Unreal because Blender's AgX view transform desaturates
bright emission, so judge emissive colour from the material values or the emissive map.

## Blender 5.x code essentials (full list in references/blender-5-api-gotchas.md)

- EEVEE's engine id is `'BLENDER_EEVEE'` (`'BLENDER_EEVEE_NEXT'` raises an error in 5.x).
  Principled BSDF sockets: `Base Color`, `Emission Color`, `Emission Strength`, `Coat Weight`,
  `Specular IOR Level`, `Transmission Weight`.
- No `use_auto_smooth`. Use `mesh.shade_smooth()` + `mesh.set_sharp_from_angle(angle=rad)` (data
  API) or `bpy.ops.object.shade_auto_smooth`.
- Operator context: `with bpy.context.temp_override(...)` (passing a context dict raises
  `ValueError`). Edit-mode operators (`mesh.select_all`, `uv.smart_project`) read the **real**
  view-layer active object, so set `view_layer.objects.active` and the selection for real, then
  restore them.
- `material.use_nodes = True` is deprecated (new materials already have node trees). Compositor:
  `scene.compositing_node_group`. Boolean solvers: `'EXACT' | 'MANIFOLD' | 'FLOAT'`.
- Geometry Nodes modifier inputs (5.2): `getattr(mod.properties.inputs, "Socket_2").value = v`
  (`mod["Socket_2"] = v` raises `TypeError`).
- Prefer the data API (`bpy.data`, `bmesh`, `mesh.transform`, `new_from_object`) over operators in
  scripts that must run both live and headless. View3D operators and `render.opengl` fail headless.

## Before you commit anything Blender made

Follow AGENTS.md §6: poll the remote first, stage only your own files, and claim shared .blend
files in CLAIMS.md. Binary art (`*.blend`, `*.fbx`, `*.glb`, `*.png`, `*.exr`) must go through Git
LFS. If `.gitattributes` doesn't track them yet, stop and tell the human (features.md C-01). Never
commit `*.blend1` backups, `__pycache__`, or temp renders outside `review/`.

## Related skills

- `blender-game-asset`: how to make assets that meet the AAA bar (routing, design, staged
  workflow, review rubric). **Start there for any "make/model/build a ..." request.**
- `blender-texturing`: materials, UVs and baking to Unreal texture sets.
- `blender-ai-mesh-cleanup`: AI-generated or downloaded meshes → game-ready.
- `blender-unreal-export`: validate, export FBX, import into UE5.
