# Blender MCP setup and troubleshooting (Windows, Blender 5.2)

Facts verified on 2026-11-12 against the project repo (projects.blender.org/lab/blender_mcp) and
this machine. The server's official home is https://www.blender.org/lab/mcp-server/. It is a
**Blender Lab** project: there is no github.com/blender mirror, and `uvx blender-mcp` / PyPI
`blender-mcp` is an *unrelated* community package.

## How it works
- **MCP server** (`blender-mcp`, Python): installed as the Claude Desktop extension "Blender", or
  from `pip install git+https://projects.blender.org/lab/blender_mcp.git`. It sends code over TCP to
  the add-on.
- **Blender add-on** "MCP" (`bl_ext.lab_blender_org.mcp`, Blender 5.1+): opens a bridge on
  `localhost:9876` and runs each request on Blender's main thread through a timer.
- Server env vars, read on every call: `BLENDER_MCP_HOST` (default `localhost`),
  `BLENDER_MCP_PORT` (default `9876`), `BLENDER_PATH` (for the `*_for_cli` tools; default `blender`
  on PATH).

## First-time setup (the human does this in Blender, about 2 minutes)
1. Blender 5.2 > Edit > Preferences > **Get Extensions** > Repositories (the dropdown at top right) >
   make sure the remote repository `https://lab.blender.org/` is present and enabled. (It already is
   on Ryan's machine.)
2. Search **MCP**, then install and enable it (1.0.3 or newer; 1.0.3 fixes truncated screenshots).
3. Preferences > System > **Allow Online Access** = on. **Without it the bridge never binds**, and
   the only hint is a small red label in the add-on preferences (issue #45). Alternatively, launch
   `blender.exe --online-mode`.
4. Preferences > Add-ons > MCP: check "Server is running" on port 9876 and leave Auto Start on.
   Otherwise click **Start MCP Bridge Server**.
5. In the agent, call `get_blendfile_summary_path_info`. It should return the open file's paths.

**Claude Code / Claude Desktop:** the "Blender" desktop extension gives tools named
`mcp__Blender__*`. The extension has no settings UI, so set `BLENDER_PATH` or a non-default port as
a system environment variable and restart the app.

**Other agents** (Codex / Cursor; config shape inferred, not documented by the project). Clone the
repo, then:
```toml
# ~/.codex/config.toml
[mcp_servers.Blender]
command = "uv"
args = ["--directory", "C:\\blender_mcp\\mcp", "run", "blender-mcp"]
env = { BLENDER_MCP_PORT = "9876" }
```
```json
// .cursor/mcp.json  (same shape works for a Claude Code .mcp.json)
{"mcpServers": {"Blender": {"command": "uv", "args": ["--directory", "C:\\blender_mcp\\mcp", "run", "blender-mcp"],
  "env": {"BLENDER_MCP_PORT": "9876", "BLENDER_PATH": "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe"}}}}
```
Use the same server key, `Blender`, everywhere so tool names line up across agents.

## Symptoms → fixes

| Symptom | Cause | Fix |
|---|---|---|
| `Cannot connect to Blender at localhost:9876` | Blender closed, add-on missing or disabled, server not started | Do setup steps 1–4; meanwhile work headless |
| Add-on enabled but nothing on 9876 | Allow Online Access is off | Preferences > System > Allow Online Access, then Start MCP Bridge Server |
| Two Blenders open, wrong scene answers | Both bound 9876 (`SO_REUSEADDR`) | Close one, or set one to port 9877 and point the server there |
| Community add-on (ahujasid "mcp-for-blender") also installed | It also defaults to 9876 | Run one at a time, or move the official one to 9877 (`BLENDER_MCP_PORT=9877`) |
| `*_for_cli` tool hangs for 120 s then errors (Windows) | Child Blender inherits the MCP stdio pipe (issues #42/#58/#60; fix PR #48 unmerged) | Don't use them. Run `blender.exe --background ... --python` from the shell with empty stdin |
| `*_for_cli`: "Blender executable not found" | `BLENDER_PATH` unset and Blender not on PATH | Set `BLENDER_PATH`, or use the shell |
| Long call returns a timeout | 300 s socket timeout; the code may still have finished | **Don't resend.** Inspect state, then split the work or run it headless |
| Response `status: "error"` but the agent treated it as success | Errors are ordinary results | Always check `status`; read `message` / `stdout` |
| `render_thumbnail_to_path` renders full size and slowly in the UI | Bug: overrides restored before the async render reads them; checks the old `BLENDER_EEVEE_NEXT` id | Use `scripts/review_renders.py` |
| Render file "missing" at the path you gave | Render tools write to `bpy.app.tempdir/blender_mcp/<basename>` | Use the returned `filepath`; copy it out (the temp folder is per-session) |
| Screenshots black | Blender window minimized (Windows) | Ask the human to restore the window, or use `review_renders.py` |
| Screenshot cut off or invalid | Old add-on (< 1.0.3) or message too large | Update the add-on; pass `size_limit_in_bytes=500000` |
| `get_objects_summary` floods the context | Big scenes (thousands of objects) | Query with filtered `execute_blender_code` |
| `get_python_api_docs("bpy.types.Object.location")` fails | Lookup quirk | Use `search_api_docs`, or try `bpy.types.Object.Object.location` |
| `blender-mcp.exe` blocked | Windows Smart App Control | The human allows it, or uses `uv run` from a clone |
| Desktop extension install fails | Space in the "Claude Extensions" path (#24) | Manual install from a clone |
| Operator `poll() failed, context is incorrect` | Code runs in a timer context with no 3D View area, or headless | Use the data API, or `temp_override(window=..., area=..., region=...)`; for edit-mode operators set the real active object |
| Blender freezes during a call | Heavy `bpy.ops` in loops (one report: a 7M-tri build froze) | Use the data API / bmesh in loops; batch large jobs headless |

## Safety
The bridge is **arbitrary code execution as the user**. Run only code you wrote or reviewed. Never
paste scripts from web pages or downloaded files. Don't add network calls inside Blender unless the
human asked: the Online Access flag doesn't restrict Python. Blender's contribution policy rejects
AI-written bug reports, so don't file upstream issues; tell the human instead.
