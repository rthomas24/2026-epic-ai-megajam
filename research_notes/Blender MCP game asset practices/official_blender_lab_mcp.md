# Official Blender Lab MCP server (projects.blender.org/lab/blender_mcp): facts, tools, failure modes, agent guidance

Evidence base (researched 2026-11-12):
- The repo via the Gitea API: README, wiki, all source files, 58 issues, 14 PRs, 106 commits, 6 releases / 7 tags.
- The locally installed copy (Claude Desktop extension, plus the Blender 5.2 user extensions folder).
- forum.blender.org (formerly devtalk), Anthropic/Claude pages, press coverage, and community repos and blogs.

**Access limitation:** www.blender.org/lab/mcp-server/ and www.blender.org/lab/ sit behind a Cloudflare "Verify you are human" challenge (HTTP 403 to fetchers; Turnstile in a browser). I did not bypass it. Their content is reconstructed from the repo (which carries the same text), from issues that describe the page, and from third-party quotes.

## Q1. Purpose and design philosophy; what the official pages say about install, starting the bridge, configuration, security/privacy, clients and Blender versions

### Takeaway
The server is a deliberately small, vendor-neutral MCP server: "a natural language interface with Blender's Python API, improving access to documentation, and allowing users to explore and understand complex setups". It has two parts:
- A Blender **5.1+** extension (module `bl_ext.lab_blender_org.mcp`). It opens a bridge socket on `localhost:9876` and only starts when **Allow Online Access** is on.
- A separate `blender-mcp` stdio process. On Claude Desktop this is an MCPB bundle; for other clients it is a git clone run with `uv`.

It runs LLM-written Python with no real sandbox, and the Lab's own warning recommends a VM or a machine without sensitive data.

### Cited Findings

**Purpose / philosophy**
- README quote: "A lightweight MCP (Model Context Protocol) server for Blender. It offers a natural language interface with Blender's Python API, improving access to documentation, and allowing users to explore and understand complex setups." Also: "The project is deliberately small, maintainable, and does no more than necessary." Data flow: `MCP Client ⇐ MCP/stdio ⇒ blender-mcp ⇐ TCP socket ⇒ Blender Add-on`. — [readme.md](https://projects.blender.org/lab/blender_mcp/src/branch/main/readme.md)
- The MCPB README says the server "Allows LLM assistants to interact with a running Blender instance – inspecting scenes, executing Python code, rendering, and navigating the interface", and that it "contains the complete API and user manual as resources". — [mcp/README.md](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/README.md)
- Origin (Dalai Felinto, Head of Product, 2026-01-30): "To launch the Blender Lab initiative, the development team will explore natural language input in Blender. The experiment aims to develop an **MCP server** for Blender, with communication handled via the existing Python API." I read this through a third-party reader mirror, because blender.org blocks fetchers. — [Projects to Look Forward to in 2026](https://www.blender.org/development/projects-to-look-forward-to-in-2026/)
- The Lab org describes itself as "The innovation space within the Blender project, where designers and developers can work together on challenging or future-facing projects, to keep Blender relevant in the years to come." — [Gitea API repo/org record](https://projects.blender.org/api/v1/repos/lab/blender_mcp)
- Authors:
  - Campbell Barton (ideasman42) wrote almost all of the code, from the initial commit on 2026-02-19.
  - Dalai Felinto did packaging (MCPB), docs and triage.
  - Pablo Vazquez rewrote the README. — [commit history](https://projects.blender.org/lab/blender_mcp/commits/branch/main)
- Vendor-neutral stance:
  - dfelinto: "The MCP project, while it also works with Claude Desktop, follows an open standard and is LLM-chat agnostic, not tied to a specific vendor." — [#22](https://projects.blender.org/lab/blender_mcp/issues/22)
  - dfelinto rejected an installer script: "for the Lab project we are not interested on adding any platform specific solution. At least not any of the proprietary platform solutions". — [PR #50](https://projects.blender.org/lab/blender_mcp/pulls/50)
  - dfelinto, again: "there is no interest from the project at the moment to tailor towards specific proprietary platforms". — [#51](https://projects.blender.org/lab/blender_mcp/issues/51)
- Client-specific settings (Claude Code permissions etc.) "don't really fit within this project documentation. We are sticking to cover only what is part of the MCP standards." — dfelinto in [#23](https://projects.blender.org/lab/blender_mcp/issues/23)

**Versions and requirements**
- Add-on manifest:
  - `id = "mcp"`, `name = "MCP"`, `maintainer = "Blender Lab"`, `blender_version_min = "5.1.0"`.
  - `[permissions] network = "Runs a local TCP socket server for MCP client communication"`.
  - SPDX GPL-3.0-or-later. — [blender_manifest.toml](https://projects.blender.org/lab/blender_mcp/src/branch/main/addon/blender_mcp_addon/blender_manifest.toml)
- Why 5.1: the minimum Blender version was raised because the operator filtering in the weak sandbox (`bpy.ops._op_create_function`) "depends on recent changes to the API". — [commit d610dcdc](https://projects.blender.org/lab/blender_mcp/commit/d610dcdc5c7b9d68e4530671ad5892fe9cb713aa)
- Version conflict: Anthropic's tutorial says "Blender 4.2 or later" — [Claude Academy tutorial](https://academy.claude.com/tutorials/using-the-blender-connector-in-claude). The manifest says 5.1.0, and a user's install on 4.5.7 LTS failed — [YaroTech, 2026-05-01](https://note.com/yaoyoroztech/n/n2c1f2ff3cabe). MeltFlex advises trusting the manifest — [MeltFlex, 2026-10-13](https://www.meltflexai.com/blog/connect-claude-to-blender).
- Server package:
  - `name = "blender-mcp"`; dependencies `docutils`, `mcp[cli]>=1.2.0,<2`, `pyyaml`; `requires-python >=3.10`.
  - Entry point `blender-mcp = "blmcp:main"`. — [mcp/pyproject.toml](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/pyproject.toml)
  - `python -m blmcp` also works. — [__main__.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/__main__.py)
- MCPB manifest:
  - `manifest_version "0.4"`, `"server": {"type": "uv", "mcp_config": {"command": "uv", "args": ["run", "blender-mcp"]}}`, `privacy_policies: https://www.blender.org/privacy-policy/`.
  - Its long_description points add-on install to `https://lab.blender.org/mcp-server/#addon`.
  - It defines **no `user_config`/env** entries. — [mcp/manifest.json](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/manifest.json)

**Installing the Blender add-on**
- Official text: "Install the Blender Lab Extensions repository: `https://lab.blender.org/`" then "Find the MCP add-on, install and enable it." — [mcp/README.md](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/README.md)
- The blender.org page installs by drag-and-drop links of the form `https://projects.blender.org/lab/blender_mcp/releases/download/v1.0.0/mcp-1.0.0.zip?repository=https%3A%2F%2Flab.blender.org%2F&blender_version_min=5.1.0`. A bug report found the drag link and the download link pointed at different versions; it was fixed. — [#29](https://projects.blender.org/lab/blender_mcp/issues/29)
- The link must be dragged **twice**: the first drag adds the "lab" repository (a permission prompt), the second installs the add-on.
  - [Claude Academy](https://academy.claude.com/tutorials/using-the-blender-connector-in-claude)
  - [#51](https://projects.blender.org/lab/blender_mcp/issues/51) ("Drag&Drop ... twice due to permissions")
  - [MeltFlex](https://www.meltflexai.com/blog/connect-claude-to-blender) ("The second drag is the step people miss")
- On this machine, the Lab repository index (cached 2026-10-03 at `%APPDATA%\Blender Foundation\Blender\5.2\extensions\lab_blender_org\.blender_ext\index.json`) offers `mcp` **1.0.3**, archive `https://projects.blender.org/lab/blender_mcp/releases/download/v1.0.3/mcp-1.0.3.zip`, `blender_version_min 5.1.0`. No installed `mcp` add-on folder exists under the Blender 5.2 user extensions directory. — local files
- Developer CLI install:
  - Build: `blender -c extension build --source-dir ./addon/blender_mcp_addon`
  - Install: `blender -c extension install-file <zip> --repo=user_default` — [readme_local_llm.rst](https://projects.blender.org/lab/blender_mcp/src/branch/main/readme_local_llm.rst)
- lab.blender.org's TLS certificate expired once (Blender 5.1.2 / Windows 11 showed `CERTIFICATE_VERIFY_FAILED` on "Refresh Remote"). Reported 2026-05-30, closed 2026-09-08. — [#37](https://projects.blender.org/lab/blender_mcp/issues/37)
- A user fixed "can't connect" by uninstalling an old "Legacy" add-on and reinstalling the official zip. The maintainer removed an attached server build "for safety reasons", saying people should use the official releases. — [#40](https://projects.blender.org/lab/blender_mcp/issues/40)

**Starting the bridge; add-on preferences**
- Online access gate:
  - The bridge refuses to start unless `bpy.app.online_access` is true, with the error "Online access must be enabled in the system preferences".
  - In background mode it prints "Use --online-mode to enable online access from the command line".
  - Source comment: "we could consider `localhost` to be acceptable, this is a grey area". — [addon __init__.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/addon/blender_mcp_addon/__init__.py); [commit 3b5b0bd1](https://projects.blender.org/lab/blender_mcp/commit/3b5b0bd1b910d4cab12fb7be9e10836191ec78e6)
- The "server shows on but never binds" symptom comes from online access being off. The failure is easy to miss: a small red label in the add-on preferences. Fix: Preferences > System > Allow Online Access, then "Start MCP Bridge Server" (or restart Blender). Still open. — [#45](https://projects.blender.org/lab/blender_mcp/issues/45)
- `--online-mode`: "Allow internet access, overriding the preference." — bundled manual `advanced/command_line/arguments.rst` (in [data/manual](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/data))
- Preferences (Edit > Preferences > Add-ons > MCP) — [addon __init__.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/addon/blender_mcp_addon/__init__.py):

  | Setting | Default | Range / notes |
  |---|---|---|
  | Host | `localhost` | |
  | Port | 9876 | 1024–65535 |
  | Auto Start | True | ignored in background mode |
  | Auto Start Delay | 1.0 s | |
  | Timer Interval | 0.25 s | 0.05–5 |
  | Timer Interval Idle | 1.0 s | |
  | Idle Delay | 5.0 s | |
  | Log | off | prints every request/response to the terminal |

  Operators: "Start MCP Bridge Server" (`bpy.ops.blmcp.server_start`) and "Stop MCP Server" (`bpy.ops.blmcp.server_stop`). Status labels: "Server is running" / "Server is stopped".
- The Anthropic tutorial says to find a "BlenderMCP tab" and click "start MCP server". This is outdated: the settings live in the add-on's preferences. The maintainer referred the reporter to Anthropic, since that page is Anthropic's. — [#22](https://projects.blender.org/lab/blender_mcp/issues/22)
- Background bridge: started via `blender --background file.blend --command blender_mcp` (accepts `--host`, `--port`). It serves requests synchronously and rejects deferred responses. — [cli.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/addon/blender_mcp_addon/cli.py)
- `-c/--command` "Run a command which consumes all remaining arguments", so it must come last. — bundled manual `arguments.rst`

**Installing / configuring the MCP server**
- Claude Desktop: the directory listing "Blender" by Blender Lab is "Anthropic verified", version **1.0.1**, added April 2026, URL `https://claude.ai/directory/ant.dir.gh.blender.blender-mcp`, with the description "Natural language interface with the Python API and documentation". — [claude.com/connectors/blender](https://claude.com/connectors/blender)
- Per Claude Academy, the connector requires Claude Desktop (browser claude.ai is not supported) and Blender on the same machine. Changes land in the open session and are written when you save. Claude Code works through normal MCP config. — [Claude Academy](https://academy.claude.com/tutorials/using-the-blender-connector-in-claude)
- dfelinto: "only Claude Desktop and Windows seems to support" `.mcpb`. He tested llama.cpp, AI Playground and Claude Desktop (with mcpb and the regular `uv run` workflow). — [#20](https://projects.blender.org/lab/blender_mcp/issues/20)
- Generic stdio setup (wiki "Setup", last edited 2026-07-23):
  - Windows: `cd c:\` then `git clone https://projects.blender.org/lab/blender_mcp.git`
  - Config (verbatim): `{"mcpServers": {"blender": {"command": "uv", "args": ["--directory", "C:\\blender_mcp\\mcp", "run", "blender-mcp"]}}}`
  - macOS/Linux use `$HOME/blender_mcp/mcp`.
  - Wiki note: "Some LLM clients may not support local variables ($HOME)". — [wiki/Setup](https://projects.blender.org/lab/blender_mcp/wiki/Setup)
- Claude Code: `$HOME` is not expanded in `.mcp.json`. A working POSIX variant is `{"command": "sh", "args": ["-c", "cd \"$HOME/blender_mcp/mcp\" && uv run blender-mcp"]}`. — [#23 comment](https://projects.blender.org/lab/blender_mcp/issues/23)
- pip install: `pip install git+https://projects.blender.org/lab/blender_mcp.git#subdirectory=mcp`. The `#subdirectory=mcp` part was fixed via [PR #25](https://projects.blender.org/lab/blender_mcp/pulls/25). — [mcp/README.md](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/README.md)
- No-clone alternative used by a community skill: `uvx --from "git+https://projects.blender.org/lab/blender_mcp.git#subdirectory=mcp" blender-mcp`, registered under the MCP name `blender`. — [blender-kiln reference](https://github.com/elithril/blender-kiln/blob/main/plugin/references/blender-mcp.md)
- **Name collision:**
  - `uvx blender-mcp` pulls the unrelated PyPI package, which is incompatible with the Lab add-on; dfelinto: "There is no plan on changing the project name." — [#34](https://projects.blender.org/lab/blender_mcp/issues/34)
  - PyPI `blender-mcp` 2.0.0 is now a shim that installs ahujasid's `mcp-for-blender`. — [blender-kiln reference](https://github.com/elithril/blender-kiln/blob/main/plugin/references/blender-mcp.md)
  - The official server is not published to any package registry (open question). — [#71](https://projects.blender.org/lab/blender_mcp/issues/71)
- HTTP transport, for the llama.cpp web UI:
  - Command: `blender-mcp --transport http --port 9191` (`--host` defaults to 127.0.0.1; `--port` defaults to 8000).
  - It serves streamable HTTP at `/` with `stateless_http=True`, CORS `allow_origins=["*"]`, and `enable_dns_rebinding_protection=False`. — [blmcp/__init__.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/__init__.py); [readme_local_llm.rst](https://projects.blender.org/lab/blender_mcp/src/branch/main/readme_local_llm.rst); [wiki/Llama.cpp](https://projects.blender.org/lab/blender_mcp/wiki/Llama.cpp)
  - The developer comment calls it "not full HTTP support" and says it "could be disabled if it no longer serves its purpose". — [blmcp/__init__.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/__init__.py)
- Environment variables:
  - `BLENDER_MCP_HOST` (default `localhost`) and `BLENDER_MCP_PORT` (default 9876) are read by the server on each call. — [connection.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools_helpers/connection.py)
  - `BLENDER_PATH` (default `blender` on PATH) is used only by the `_for_cli` tools. — [blender_cli.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools_helpers/blender_cli.py)
- Clients tested or mentioned:
  - llama.cpp ≥ 8218 — [readme_local_llm.rst](https://projects.blender.org/lab/blender_mcp/src/branch/main/readme_local_llm.rst)
  - LM Studio + nemotron-nano — [#2](https://projects.blender.org/lab/blender_mcp/issues/2)
  - "Claude Code, Cursor, etc." — [readme.rst@v0.3.0](https://projects.blender.org/lab/blender_mcp/src/tag/v0.3.0/readme.rst)
  - Jan, VS Code, AI Playground, Claude Code CLI — [#23](https://projects.blender.org/lab/blender_mcp/issues/23)
  - Codex (issue reports from Codex trajectories) — [#54](https://projects.blender.org/lab/blender_mcp/issues/54)
  - Suggested local models: Gemma 4 26B-A4B and Qwen3.6-35B-A3B GGUFs (tested on an M3 MacBook). — [wiki/Llama.cpp](https://projects.blender.org/lab/blender_mcp/wiki/Llama.cpp)

**Our local install (verified on disk)**
- Claude Desktop extension `C:\Users\ryant\AppData\Roaming\Claude\Claude Extensions\ant.dir.gh.blender.blender-mcp\`:
  - manifest version "1.0.1", installed 2026-10-03.
  - `uv.lock` pins `mcp 1.27.0`; `pyproject.toml` has `mcp[cli]>=1.2.0` with no `<2` cap.
  - The screenshot limit is still `(1_048_576 * 3) // 4`, i.e. before the 2 KiB JSON-envelope headroom fix.
  - `blender_cli.py` passes no `stdin` to the Blender subprocess.
  - The thumbnail tool checks `BLENDER_EEVEE_NEXT`.
  - Bundled API docs title: "Blender 5.1 Python API Documentation".
  - `BLENDER_PATH` is unset, and Blender is not on PATH. — local files
- Conclusion: the directory's "1.0.1" is **v1.0.0-era server code**, not repo tag v1.0.1. Upstream tag v1.0.1 already has the `<2` pin — [pyproject@v1.0.1](https://projects.blender.org/lab/blender_mcp/src/tag/v1.0.1/mcp/pyproject.toml) — and the 2 KiB headroom fix — [commit 4309a396](https://projects.blender.org/lab/blender_mcp/commit/4309a39646e644261624bfcd2bca669b343b7621). Others report the same Claude Desktop extension "1.0.1" alongside add-on 1.0.0/1.0.3:
  - [#31 comment](https://projects.blender.org/lab/blender_mcp/issues/31)
  - [#70](https://projects.blender.org/lab/blender_mcp/issues/70)
  - [#72](https://projects.blender.org/lab/blender_mcp/issues/72)

**Security / privacy**
- Lab warning, verbatim from the repo: "The MCP server will execute LLM generated code in Blender without any guards in place to protect your data from removal or being sent to a remote location. To keep your data safe it is recommended to use a virtual machine, or a system without access to sensitive information." — [readme.rst@v0.3.0](https://projects.blender.org/lab/blender_mcp/src/tag/v0.3.0/readme.rst)
- The same text is quoted from blender.org by third parties: [StraySpark (vendor)](https://www.strayspark.studio/blog/official-blender-mcp-server-comparison-2026); [YaroTech](https://note.com/yaoyoroztech/n/n2c1f2ff3cabe)
- Weak sandbox — [weak_sandbox.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/addon/blender_mcp_addon/weak_sandbox.py):
  - Source: "Note that this isn't really a sandbox, more guidance that some things should not be done ... If the LLM (or its user) is motivated these can be worked around."
  - It blocks only `sys.exit()`, `bpy.ops.wm.quit_blender`, `wm.read_factory_settings`, `wm.read_factory_userpref` and `wm.read_userpref`. For resets it suggests `bpy.ops.wm.read_homefile(use_empty=True, use_factory_startup=True)`.
- Tool safety annotations were added ([#13](https://projects.blender.org/lab/blender_mcp/issues/13), [commit 49f21ce1](https://projects.blender.org/lab/blender_mcp/commit/49f21ce11b117864c13d76d9d61a6828046b71d0)). Claude Desktop then groups read-only tools apart from write/delete tools, which need approval. — [YaroTech](https://note.com/yaoyoroztech/n/n2c1f2ff3cabe)
- The official server lists no telemetry; the community ahujasid server has telemetry on by default (opt-out). — [YaroTech](https://note.com/yaoyoroztech/n/n2c1f2ff3cabe)
- Campbell Barton (2026-05-01) on sandboxing: "it's not something Blender can/should realistically solve". He tested the `jai` and `bubblejail` Linux sandboxes and needed extra measures to block `git push` from inside. — [forum thread 45131](https://forum.blender.org/t/blender-mcp-server-after-claude-mcp-security-for-blender-scripting-3d-agent-notes/45131)

### Inferences
- **Bring-up checklist for our machine** (Windows 11, Blender 5.2.2, Claude Desktop extension, Claude Code). It follows from the findings above:
  1. In Blender 5.2: Preferences > Get Extensions, add the remote repository `https://lab.blender.org/` (or drag the install link twice), then install and enable **MCP 1.0.3**. It currently looks uninstalled on disk.
  2. Preferences > System > **Allow Online Access** (or launch `blender.exe --online-mode`).
  3. Preferences > Add-ons > MCP: confirm "Server is running" on 9876 and leave Auto Start on.
  4. In Claude Code inside the desktop app, the tools appear as `mcp__Blender__*` (observed in this session's tool list). First call: `get_blendfile_summary_path_info` or `get_objects_summary` as a connection check.
- Updating the **add-on** to 1.0.3 matters more than updating the server. The screenshot-truncation fix ([commit 1e8d5976](https://projects.blender.org/lab/blender_mcp/commit/1e8d59762d97c199a9816396a665bae1a338f14f)) lives in the add-on. Server-side, the older Claude Desktop build differs mainly by:
  - The missing 2 KiB headroom. Mitigate by passing `size_limit_in_bytes` (e.g. 500000) to the screenshot tools.
  - A lockfile-pinned MCP SDK, which is fine.
- Because the MCPB manifest defines no `env`/`user_config`, the Claude Desktop extension cannot be told `BLENDER_PATH` or a non-default port from its UI. A teammate who needs them should register the server manually with an `env` block.
- Example Claude Code project `.mcp.json` (Windows, git clone at `C:\blender_mcp`; my addition: `env`):
  ```json
  {"mcpServers":{"blender":{"command":"uv","args":["--directory","C:\\blender_mcp\\mcp","run","blender-mcp"],
    "env":{"BLENDER_MCP_PORT":"9876","BLENDER_PATH":"C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe"}}}}
  ```
- Codex/Cursor equivalents (unverified against those clients' docs):
  - Cursor `.cursor/mcp.json` uses the same `mcpServers` JSON shape.
  - Codex `~/.codex/config.toml`:
    ```toml
    [mcp_servers.blender]
    command = "uv"
    args = ["--directory", "C:\\blender_mcp\\mcp", "run", "blender-mcp"]
    env = { BLENDER_MCP_PORT = "9876" }
    ```
  - Pin with `git checkout v1.0.3`, or use main for the 5.2 docs (see Q5).
- Never configure `uvx blender-mcp` (wrong package). Do not run the community add-on at the same time on 9876.
- Treat the bridge as **arbitrary code execution as the user**. For the jam this is acceptable on a dev box, but agents should not paste untrusted scripts. Agents should also never add network calls in `execute_blender_code` unless asked, because the online-access flag does not limit Python.

### Gaps
- The blender.org MCP page's full text (FAQ, any extra example prompts, "test files") could not be read directly: Cloudflare challenge. I found no official Windows-specific setup guidance and no official Codex/Cursor configs; the project explicitly declines to write them.
- Unverified: whether Claude Desktop passes system environment variables (e.g. a user-level `BLENDER_PATH`) through to MCPB extension processes.
- No official statement on supported Blender versions above 5.1 beyond `blender_version_min`. Users report v1.0.3 working on 5.2.2/5.2.3 LTS — e.g. [#58](https://projects.blender.org/lab/blender_mcp/issues/58), [#64](https://projects.blender.org/lab/blender_mcp/issues/64).

## Q2. What each tool is for, and the documented/recommended workflows (inspection, docs lookup, code execution, visual verification, background/CLI)

### Takeaway
There are 26 tools: 20 interactive, plus 6 `*_for_cli` twins that open a .blend in a separate `blender --background` process.
- **All creation and editing goes through `execute_blender_code`.** There are no modeling, material or asset tools. The server's own prompt still calls it "a last resort" behind the dedicated tools.
- The intended loop is: inspect with the summary tools → look up the API in the bundled docs → run small Python chunks that return a dict `result` → verify by numbers and by screenshot or render.
- Community benchmarks show agents under-use the screenshot tool unless told to capture after every phase.

### Cited Findings

**Tool catalogue** — names, signatures, annotations and docstrings from the source ([tools dir](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools); [readme_tools.rst](https://projects.blender.org/lab/blender_mcp/src/branch/main/readme_tools.rst)). Behaviour notes come from each `*_toolcode.py`.

| Tool (params) | Annotation | What it does / returns | Notes from source |
|---|---|---|---|
| `execute_blender_code(code)` | destructive | Runs Python in the live Blender; "assign a JSON-serialisable dict to a variable named `result`" | Non-JSON values are `repr()`'d (strict_json=False). Deferred completion via `check_is_finished` works only in the interactive add-on |
| `execute_blender_code_for_cli(blend_file, code)` | destructive | Runs code in `blender --background <blend_file> --python-expr` | If the GUI has that same file open and dirty, saves a temporary `<name>_mcp_0001.blend` copy, uses it, then deletes it. 120 s timeout. No weak sandbox. `result` must be a dict |
| `get_blendfile_summary_datablocks()` (+`_for_cli(blend_file)`) | read-only | Data-block counts, active workspace, render engine | |
| `get_blendfile_summary_missing_files()` (+cli) | read-only | Missing external files (images, libraries, fonts, sounds, clips, caches, sequences) | |
| `get_blendfile_summary_of_linked_libraries()` (+cli) | read-only | Tree of direct/indirect linked libraries | |
| `get_blendfile_summary_path_info()` (+cli) | read-only | "Simple/fast" path, save status, age, backups | |
| `get_blendfile_summary_usage_guess()` (+cli) | read-only | Guesses use-cases, scored 0–100 with certainty | |
| `get_objects_summary()` | read-only | Scene name, active workspace/object, mode, camera; recursive **view-layer** collection tree with per-object name/type/parent/data_name/selected/visible/hide flags (+instance_collection) | Can be huge (see Q3) |
| `get_object_detail_summary(name)` | read-only | Type, location, `rotation` (=`rotation_euler`), scale, **dimensions**, parent/children, modifiers (name/type/show_viewport/show_render), constraints, material slots, visibility, data name, collections | No vertex/face counts. Rotation is Euler even for quaternion objects |
| `get_python_api_docs(identifier)` | read-only | Bundled API RST for a dotted id; `*` lists top-level modules; `X.*` lists children | Kinds: exact/namespace/definition/partial/suggestions/missing. Files >32 KB return a dot-point summary |
| `search_api_docs(query, max_results=20, context=0, index=None)` | read-only | Ranked full-text paragraph hits (path, text, breadcrumb, index, score) | Whitespace tokens, all must match; stop-words dropped; no regex; `index` widens one hit to its section |
| `search_manual_docs(...)` | read-only | Same, over the bundled user manual | |
| `get_screenshot_of_area_as_image(area_ui_type, size_limit_in_bytes=0)` | read-only | PNG of the largest area of that `ui_type` (e.g. `VIEW_3D`, `ShaderNodeTree`, `GeometryNodeTree`, `OUTLINER`, `PROPERTIES`, `IMAGE_EDITOR`, `UV`…) | Default cap `((1_048_576-2048)*3)//4` ≈ 784 KB. HiDPI downscale, then divisor binary search. Error in background mode |
| `get_screenshot_of_window_as_image(size_limit_in_bytes=0)` | read-only | Whole-window PNG | Same limits |
| `get_screenshot_of_window_as_json()` | read-only | Window layout, areas, active object, selection as JSON | Cheap "where am I" check |
| `jump_to_tab_by_name(name)` | destructive | Switches workspace | |
| `jump_to_tab_by_space_type(space_type, allow_edits=False)` | destructive | Switches to a workspace whose largest area is that type | `allow_edits=True` duplicates the current workspace if none exists |
| `jump_to_view3d_object_by_name(name, allow_edits=False)` | destructive | Frames an object (`view3d.view_selected`) in the first 3D view | **Side effects:** forces OBJECT mode, deselects all, selects and activates the target, leaves camera view. `allow_edits` unhides and un-excludes collections |
| `jump_to_view3d_object_data_by_name(name, allow_edits=False)` | destructive | Same, by data-block name | |
| `render_thumbnail_to_path(output_path)` | destructive | Meant to render ≤320 px, 16 samples, simplify subdiv 1 | Writes to `bpy.app.tempdir/blender_mcp/<basename>`. Broken in GUI mode (Q3) |
| `render_viewport_to_path(output_path)` | read-only (sic) | **Full F12 render** with current settings and scene camera | Not an OpenGL viewport capture. Same forced output folder. Deferred job in GUI, blocking in background |

Sources:
- Tool signatures and docstrings: [execute_blender_code.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/execute_blender_code.py), [get_python_api_docs.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/get_python_api_docs.py), [rst_doc_search.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools_helpers/rst_doc_search.py)
- Screenshots: [get_screenshot_of_area_as_image_toolcode.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/get_screenshot_of_area_as_image_toolcode.py), [_template_image_downscale_to_size_limit.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/_template_image_downscale_to_size_limit.py)
- Navigation: [jump_to_view3d_object_by_name_toolcode.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/jump_to_view3d_object_by_name_toolcode.py), [jump_to_tab_by_space_type_toolcode.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/jump_to_tab_by_space_type_toolcode.py)
- Summaries: [get_object_detail_summary_toolcode.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/get_object_detail_summary_toolcode.py), [get_objects_summary_toolcode.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/get_objects_summary_toolcode.py)
- Renders: [render_viewport_to_path_toolcode.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/render_viewport_to_path_toolcode.py), [render_thumbnail_to_path_toolcode.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/render_thumbnail_to_path_toolcode.py)
- CLI path: [blender_cli.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools_helpers/blender_cli.py)

**Execution semantics** (what an agent must know)
- Request and response format — [connection.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools_helpers/connection.py); [mcp_to_blender_server.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/addon/blender_mcp_addon/mcp_to_blender_server.py):
  - Each call opens a new TCP connection and sends `{"type":"execute","code":...,"strict_json":bool}` terminated by `\0`.
  - Success reply: `{"status":"ok","result":{...},"stdout":...,"stderr":...}`.
  - Failure reply: `{"status":"error","message":"<traceback>"}`, delivered as a *successful* MCP tool result.
- Each call gets a fresh namespace `{"result": {}}`:
  - `bpy` is **not pre-imported**: `NameError: name 'bpy' is not defined`. — [#55](https://projects.blender.org/lab/blender_mcp/issues/55)
  - Python variables **do not persist** between calls. — [#53](https://projects.blender.org/lab/blender_mcp/issues/53)
  - `bpy.data` does persist, so look objects up by stable names (Blender Studio style prefixes `GEO-`, `MAT-`, `LGT-`). — [cc-blender-skill mcp-backends.md](https://github.com/RobLe3/cc-blender-skill/blob/main/plugin/skills/text-to-blender/references/mcp-backends.md)
- `result` must be a dict. The error text is "The `result` variable must be a dict, not list. Wrap your return value: `result = {"key": value}`". This contradicts the prompt line "Return structured data (dicts, lists)". — [#64](https://projects.blender.org/lab/blender_mcp/issues/64); [commit dc987e76](https://projects.blender.org/lab/blender_mcp/commit/dc987e763de0cf09f6eb3ad8062f78045224db3e) (the dict requirement comes from "LM Studio, Claude")
- stdout and stderr are teed and returned, so `print()` debugging works. — [capture_output.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/addon/blender_mcp_addon/capture_output.py); [commit f2d6adda via log](https://projects.blender.org/lab/blender_mcp/commits/branch/main)
- Code runs on Blender's main thread inside a `bpy.app.timers` callback, which polls every 0.25 s by default. — [execute_interactive.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/addon/blender_mcp_addon/execute_interactive.py); [addon __init__.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/addon/blender_mcp_addon/__init__.py)
- Deferred jobs — [deferred_tool.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/addon/blender_mcp_addon/deferred_tool.py); [_template_deferred_tool_check_for_file_output.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/_template_deferred_tool_check_for_file_output.py); [commit 813a46fc](https://projects.blender.org/lab/blender_mcp/commit/813a46fce1d51d17b7289336a187e3d9a8bae245):
  - If the code leaves a callable named `check_is_finished` in its namespace, the add-on keeps the connection open and calls it each tick. It returns `None` while running and a dict when done.
  - The add-on gives up after `_DEFERRED_TIMEOUT = 3600` s.
  - In background mode this is rejected with "Deferred responses via `check_is_finished` are only supported by the interactive addon server...".
  - The pattern exists for `bpy.ops.render.render('INVOKE_DEFAULT', ...)` plus `bpy.app.is_job_running('RENDER')`.
- Limits:
  - Request ≤ 10 MiB (`_MAX_REQUEST_BYTES`).
  - The client must deliver a complete request within 10 s (`_CLIENT_TIMEOUT`).
  - Response send timeout 30 s.
  - The server-side socket timeout is **300 s** (`_TIMEOUT = 300.0`).
  - The MCP protocol message limit used for images is 1 MiB. — [mcp_to_blender_server.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/addon/blender_mcp_addon/mcp_to_blender_server.py); [connection.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools_helpers/connection.py); [commit 1ec94618](https://projects.blender.org/lab/blender_mcp/commit/1ec94618c4e50a0e168b6c5620915e9063903b3d)
- `_for_cli` run path — [blender_cli.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools_helpers/blender_cli.py):
  - Command: `[BLENDER_PATH or "blender", "--background", blend_file, "--python-expr", wrapper]`.
  - No `--factory-startup`, no `--online-mode`, no stdin redirect.
  - The result comes back by parsing a `__BLMCP_RESULT__` line on stdout.

**Server instructions the LLM receives** — [prompts.yml](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/data/prompts.yml), summarised; wording verbatim where quoted
- "Respect existing structure and naming conventions. NEVER assume missing values - inspect the scene first. Do not destructively modify objects without confirmation."
- Docs: bundled RST at `data/api/` and `data/manual/`, which the prompt says to "Search and read ... directly with standard text tools (grep, file read, etc.)".
  - On this machine that is `C:\Users\ryant\AppData\Roaming\Claude\Claude Extensions\ant.dir.gh.blender.blender-mcp\blmcp\data\{api,manual}` (local).
  - The search tools were added because some clients lack file tools; ideasman42 notes agents handle only "10-50 files ... sometimes 100 max" as MCP resources. — [#17](https://projects.blender.org/lab/blender_mcp/issues/17)
- "The `execute_blender_code` tool is a last resort, if there are other tools that provide the functionality you need, use those instead." This was added because "Claude keeps failing to take screenshots even though we have a screenshot tool". — [commit 82f5c69a](https://projects.blender.org/lab/blender_mcp/commit/82f5c69a9e18113e38eb9be6acb7a0cf27bea317)
- Operators vs data API: prefer `bpy.ops` for standard actions; use `bpy.data` for precise or batch work.
- Mode, active object and selection: verify or set the mode first; set active and selection explicitly and re-set them between operator calls.
- Update the depsgraph before reading computed values.
- In edit mode use bmesh and flush the changes back ("forgetting this silently loses all edits").
- Link new objects to a collection; "capture references immediately after creation, never look up by assumed name" because of `.001` suffixes.
- Check user counts on shared data and make it single-user first.
- Check the three visibility states.
- Units default to meters.
- Prefer modifiers (non-destructive).
- Check rotation mode ("writing to the wrong property is silently ignored").
- Apply scale before booleans and physics.
- Use the world matrix for world-space reads.
- Explore progressively; "Don't dump entire scenes".

**Documented examples** (official)
- MCPB README examples — [mcp/README.md](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/README.md):
  1. Pebble Scattering demo: "fix the name of all the data-blocks to remove typos", e.g. `LGT-Lights`→`LGT-lights`, `Compositing Nodetree`→`Compositing Node Tree`.
  2. "Which objects are using the following material: pebbles" → 7 `GEO-pebble*`.
  3. Classroom demo: polygon-count outliers relative to on-screen size, which "consider the final amount of polygons after the object modifiers are applied ... getting the object from the dependency graph". `coat 1` shows 37k vs 74k depending on the VIEWPORT vs RENDER context.
- Earlier prompt set, "Tested With 5.1, Claude Opus 4.6 (with "High Effort" thinking enabled). All of these prompts have been tested to work" — [readme_prompt_examples.rst@v0.3.0](https://projects.blender.org/lab/blender_mcp/src/tag/v0.3.0/readme_prompt_examples.rst):
  - Video-editor text animation rendered to `.mpg`.
  - Geometry Nodes "Hello World" letters that jiggle and then explode.
  - Adding a jumping lamp to `luxo.blend`.
  - Adding coloured text to `text-regression.blend`.
  - A Grease Pencil hand-drawn title card (Build and Noise modifiers).
  - "Armature Rig, IK & Walk Cycle via NLA" (stick biped, IK, weight paint, 24-frame walk, NLA ×5, render).
  - A "Modifier-Driven Spiral Staircase" (Array ×30 + Curve modifier on a helix, render `staircase.png`).
  - These were later removed from the repo ("covered by the Wiki page") — [commit 98dfe83d](https://projects.blender.org/lab/blender_mcp/commit/98dfe83db6e860ee4295ed1e3a9fa77b937acff1)
- Claude Academy prompts — [tutorial](https://academy.claude.com/tutorials/using-the-blender-connector-in-claude):
  - "Look at the open scene and rename the data blocks so each name matches what it contains."
  - "Walk through the Geometry Nodes modifier on the active object."
  - "List everything in this file that uses the "Glass_Tinted" material."
  - Plus "find the heaviest objects" (polygons vs on-screen size).
- Anthropic: "analyze and debug entire Blender scenes", "build custom scripts to batch-apply changes to objects in a scene", and the connector "lets Claude add new tools directly to Blender's interface". — [Anthropic, Claude for Creative Work, 2026-04-28](https://www.anthropic.com/news/claude-for-creative-work)

**Community-verified workflow rules for this server**
- blender-kiln's tool map for the Lab server — [kiln reference](https://github.com/elithril/blender-kiln/blob/main/plugin/references/blender-mcp.md):
  - Scene → `get_objects_summary`.
  - Dimensions → `get_object_detail_summary`.
  - Look → `get_screenshot_of_area_as_image(area_ui_type="VIEW_3D")` or `render_viewport_to_path`.
  - Framing → `jump_to_view3d_object_by_name`.
  - APIs → `search_api_docs` / `get_python_api_docs` / `search_manual_docs`, or live `dir()` / `bl_rna.properties`.
  - Export → `bpy.ops.export_scene.gltf` inside `execute_blender_code`.
  - There is no add-on-status tool; "`get_objects_summary` answering is the connection check".
  - Measured behaviour: sessions "took **one screenshot per session** on 4 of 5 briefs, and framed once in five". kiln's rule: "At the end of every phase that changed geometry or materials: `jump_to_view3d_object_by_name(name)`, then `get_screenshot_of_area_as_image(area_ui_type="VIEW_3D")`. After TEXTURING, a second angle too. A single-asset run therefore takes **at least four** captures."
- cc-blender-skill (supports the official server first) — [mcp-backends.md](https://github.com/RobLe3/cc-blender-skill/blob/main/plugin/skills/text-to-blender/references/mcp-backends.md):
  - "Check the outer status and any inner `result.status`; `isError=false` alone does not establish success."
  - "Read **the returned `result.filepath`**, not the requested absolute path."
  - "Never switch backends or replay a mutation after a timeout: inspect the selected scene first because the operation may already have completed."
  - "Prefer a final render over a screenshot when judging materials and lighting."
  - "The official object summary includes dimensions but not mesh vertex/face counts ... use evaluated meshes when modifiers affect the expected result."
  - Blender 5.x API notes:
    - EEVEE's id is `BLENDER_EEVEE`.
    - The compositor uses `scene.compositing_node_group`.
    - Glare options are input sockets.
    - Actions use layered F-curves.
    - FFmpeg needs `media_type='VIDEO'`.
- MeltFlex tests (Blender 5.2.2 + Opus 5.5) recommend:
  - State units, and give exact dimensions.
  - Ask Claude to report final sizes and positions, then spot-check, because Claude reports only what you ask for (paraphrase of the article's conclusion). — [MeltFlex](https://www.meltflexai.com/blog/connect-claude-to-blender)
- 3D-Agent's author says "fewer, general-purpose tools" beat 50+ narrow tools. "Vision-in-the-loop matters": screenshot after each action. Scene-summary context "dramatically improves output quality". — [#4](https://projects.blender.org/lab/blender_mcp/issues/4)

### Inferences
- **Canonical agent loop for Triad Drift assets on this server.** It follows from prompts.yml plus the kiln and cc-blender-skill rules:
  1. Inspect with `get_blendfile_summary_path_info`, `get_objects_summary` (small scenes only) and `get_object_detail_summary`.
  2. Look up any uncertain API with `search_api_docs` / `get_python_api_docs`, or live introspection, because the bundled docs are 5.1 and we run 5.2.
  3. Build in small `execute_blender_code` chunks, each starting with `import bpy` (and `bmesh`, `math`, `mathutils`) and ending with `result = {...}`.
  4. Verify numerically: evaluated tri counts, dimensions, material slots.
  5. Frame and screenshot after each phase.
  6. Save (`bpy.ops.wm.save_mainfile()`) at checkpoints.
  7. Export FBX/glTF for UE5 via Python.
- Snippet: evaluated triangle count per mesh (modifiers applied, like the Classroom example).
  ```python
  import bpy
  dg = bpy.context.evaluated_depsgraph_get()
  tris = {}
  for ob in bpy.context.scene.objects:
      if ob.type == 'MESH':
          ev = ob.evaluated_get(dg); me = ev.to_mesh()
          me.calc_loop_triangles(); tris[ob.name] = len(me.loop_triangles)
          ev.to_mesh_clear()
  result = {"tris": tris, "total": sum(tris.values())}
  ```
- Snippet: a reliable preview render in GUI mode. It uses the documented `check_is_finished` hook and replaces the broken `render_thumbnail_to_path`. The overrides are restored only after the job finishes, which is the fix proposed in [#70](https://projects.blender.org/lab/blender_mcp/issues/70) and [PR #66](https://projects.blender.org/lab/blender_mcp/pulls/66).
  ```python
  import bpy, os
  scn = bpy.context.scene; rd = scn.render
  out = os.path.join(bpy.app.tempdir, "blender_mcp", "shipA_preview.png")
  saved = [(rd, a, getattr(rd, a)) for a in ("filepath","resolution_x","resolution_y","resolution_percentage")]
  if rd.engine == 'CYCLES': saved.append((scn.cycles, "samples", scn.cycles.samples)); scn.cycles.samples = 16
  elif rd.engine == 'BLENDER_EEVEE': saved.append((scn.eevee, "taa_render_samples", scn.eevee.taa_render_samples)); scn.eevee.taa_render_samples = 16
  rd.filepath = out; rd.resolution_x, rd.resolution_y, rd.resolution_percentage = 640, 360, 100
  bpy.ops.render.render('INVOKE_DEFAULT', write_still=True)
  def check_is_finished():
      if bpy.app.is_job_running('RENDER'):
          return None
      for o, a, v in saved: setattr(o, a, v)
      return {"status": "ok" if os.path.exists(out) else "error", "filepath": out}
  ```
- Then copy the render out of the session temp folder:
  ```python
  import bpy, os, shutil
  src = os.path.join(bpy.app.tempdir, "blender_mcp", "shipA_preview.png")
  dst = r"C:\path\to\game-jam\renders\shipA_preview.png"
  os.makedirs(os.path.dirname(dst), exist_ok=True); shutil.copy2(src, dst)
  result = {"copied": dst}
  ```
  Unverified: I believe Blender deletes its per-session temp folder on exit, but the bundled docs only say `tempdir` is "the temp directory used by blender (read-only)". Copy anything you need to keep.
- Code runs in a timer context with no `context.area` (the screenshot and jump tools always build `temp_override(window=..., area=..., region=...)` themselves). So any operator whose poll needs a 3D View should be wrapped the same way, or replaced with data-API/bmesh calls.
- Keep each call well under ~2–3 minutes (see the Q3 timeouts). Never re-send a mutation that timed out; re-inspect first.
- Avoid running two agents against one bridge at once. Requests run one by one on the main thread, but the separate calls of two agents would interleave on the same scene.

### Gaps
- No official "agent playbook" beyond prompts.yml, the docstrings and the three README examples. The blender.org page may hold more example prompts and test files that I could not read.
- No official guidance on game-asset export (FBX/glTF for Unreal) through this server. It is plain `bpy` in `execute_blender_code`.

## Q3. Known limitations, bugs, open issues and PRs (timeouts, deferred jobs, render output paths, size limits, background mode, Windows)

### Takeaway
On our exact setup (Windows 11, Blender 5.2.x, Claude Desktop extension "1.0.1", Claude Code over stdio):
- **All six `*_for_cli` tools hang until their 120 s timeout.** The fix is still open.
- **`render_thumbnail_to_path` renders at full resolution in GUI mode.**
- Renders always land in `bpy.app.tempdir/blender_mcp/`.
- Screenshots are capped near 1 MiB, and only add-on ≥1.0.3 sends them reliably.
- Large-scene summaries can flood the context.
- The bundled docs are 5.1.
- Windows-specific problems: Smart App Control blocking, Claude Desktop install failure on paths containing a space, and `SO_REUSEADDR` letting two bridges share one port.

### Cited Findings

**Windows `_for_cli` hang (open)**
- Every `*_for_cli` call over stdio fails with "Blender CLI timed out after 120s". Cause: `subprocess.run()` passes no `stdin`, so the background Blender inherits the MCP stdio pipe. Fix: `stdin=subprocess.DEVNULL`. Reports:
  - [#42](https://projects.blender.org/lab/blender_mcp/issues/42): Win 11, v1.0.0, Claude Code.
  - A later comment on #42: Win 10, Blender 5.2.3, v1.0.3, Claude Code CLI. With the fix, "timeout → 2.1 s".
  - [#58](https://projects.blender.org/lab/blender_mcp/issues/58): Win 11 26200, Blender 5.2.2 Steam, v1.0.3, mcp 1.30.0.
  - [#60](https://projects.blender.org/lab/blender_mcp/issues/60): v1.0.3, 5.2.2.
- The proposed explanation in #58: "On Windows, other I/O on a synchronous pipe handle queues behind a pending read".
- One-line fix in [PR #48](https://projects.blender.org/lab/blender_mcp/pulls/48), still open. A tester reported 120 s timeout → 2.8 s on Win 11 / 5.2.2. dfelinto called it "very straightforward" (Jul 24) but has not merged it.
- `_for_cli` also hangs with user add-ons enabled (open, 2026-11-09, same setup as ours: Win 11, Blender 5.2.3, Claude Desktop extension 1.0.1). Plain `blender -b file --python-expr "print('OK')"` "never exits". With `--factory-startup` it finishes in ~1.8 s. With `--factory-startup --addons bl_ext.lab_blender_org.mcp` while the GUI was serving on 9876, it "hung and broke the connection to the interactive instance until restart". The tool cannot pass `--factory-startup`. — [#72](https://projects.blender.org/lab/blender_mcp/issues/72)
- Microsoft Store Blender: `_for_cli` returns "No result marker in Blender output" even though the code ran, because `blender-launcher.exe` drops stdout, and `blender.exe` inside WindowsApps gives `WinError 5`. "Retrying can repeat exports or other file writes." — [#69](https://projects.blender.org/lab/blender_mcp/issues/69)
- CLI errors only return `str(ex)` with no traceback. Fix in [PR #68](https://projects.blender.org/lab/blender_mcp/pulls/68), still open. — [blender_cli.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools_helpers/blender_cli.py)

**Renders (open)**
- `render_thumbnail_to_path` in GUI mode — [#46](https://projects.blender.org/lab/blender_mcp/issues/46), [#61](https://projects.blender.org/lab/blender_mcp/issues/61), [#70](https://projects.blender.org/lab/blender_mcp/issues/70), fix in [PR #66](https://projects.blender.org/lab/blender_mcp/pulls/66) (open):
  - It renders at full resolution and samples while still returning `status: "ok"`, because the overrides are restored as soon as `bpy.ops.render.render('INVOKE_DEFAULT', write_still=True)` returns, before the job reads them.
  - Measured: 1920×1080 output that is byte-identical to `render_viewport_to_path`; 15 of 18 PNGs had the wrong size.
  - On a 1280×1600, 192-sample Cycles scene the "thumbnail" took 305 s. Claude Desktop "gave up about 3 minutes in (185 s ...) and marked the server unavailable"; "The add-on's one-hour deferred timeout doesn't help once the client has given up".
  - The blocking path (background / `_for_cli`) is not affected.
  - Secondary bug: the code checks `rd.engine == "BLENDER_EEVEE_NEXT"`, but in 5.2 the enum is only `['BLENDER_EEVEE']`, so EEVEE sample overrides never apply.
- Render tools force output into `os.path.join(bpy.app.tempdir, "blender_mcp", os.path.basename(output_path))` and discard the directory you pass. Users hit this as an undocumented surprise ([#49 comment](https://projects.blender.org/lab/blender_mcp/issues/49)). — [render_viewport_to_path_toolcode.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/render_viewport_to_path_toolcode.py)
- Rendering temporarily changes the user's render output settings until the render finishes. "Saving while rendering will then save using the temporary settings", and a failed restore callback "could also leave invalid temporary settings". A proper fix "requires changes to Blender's render operator" (open TODO). — [#12](https://projects.blender.org/lab/blender_mcp/issues/12)
- `render_viewport_to_path` is annotated `readOnlyHint=True` even though it runs a full render and writes a file. — [render_viewport_to_path.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/render_viewport_to_path.py)
- A macOS user also saw `render_viewport_to_path` time out without writing a file. — [#31 comment](https://projects.blender.org/lab/blender_mcp/issues/31)

**Screenshots and message size**
- History of "Unterminated string starting at: line 1 column 61 (char 60)" on HiDPI/Retina:
  - First an MCP 1 MB limit problem: [#9](https://projects.blender.org/lab/blender_mcp/issues/9), fixed by downscaling in [commit 1ec94618](https://projects.blender.org/lab/blender_mcp/commit/1ec94618c4e50a0e168b6c5620915e9063903b3d).
  - Then non-blocking `sendall()` truncation: [#21](https://projects.blender.org/lab/blender_mcp/issues/21), [#31](https://projects.blender.org/lab/blender_mcp/issues/31), [#35](https://projects.blender.org/lab/blender_mcp/issues/35), [#49](https://projects.blender.org/lab/blender_mcp/issues/49); PRs [#38](https://projects.blender.org/lab/blender_mcp/pulls/38), [#41](https://projects.blender.org/lab/blender_mcp/pulls/41).
- Fixed in two parts:
  - Server: 2 KiB JSON-envelope headroom — [commit 4309a396](https://projects.blender.org/lab/blender_mcp/commit/4309a39646e644261624bfcd2bca669b343b7621), 2026-08-06.
  - Add-on: queued partial writes — [commit 1e8d5976](https://projects.blender.org/lab/blender_mcp/commit/1e8d59762d97c199a9816396a665bae1a338f14f), 2026-09-10. Shipped in **v1.0.3** ("Fix the long standing screenshot capture tool"). — [releases](https://projects.blender.org/lab/blender_mcp/releases)
- One user noticed the failure in Claude Code but not in Cowork. — [PR #38 comment](https://projects.blender.org/lab/blender_mcp/pulls/38)
- Screenshots are PNGs downscaled to fit ~700 KB. WEBP is a TODO that "depends on changes in Blender 5.2" (open). — [#11](https://projects.blender.org/lab/blender_mcp/issues/11)
- Screenshot and jump tools return errors in background mode ("Screenshots are not available in background mode"). — [get_screenshot_of_area_as_image_toolcode.py](https://projects.blender.org/lab/blender_mcp/src/branch/main/mcp/blmcp/tools/get_screenshot_of_area_as_image_toolcode.py); [commit 2ef9f9fd](https://projects.blender.org/lab/blender_mcp/commit/2ef9f9fdb4aba5175ea990afe2e676dd81b69f7b)

**Context, semantics and docs**
- `get_objects_summary` on a scene with 1,670 objects produced ~904 KB of response and added ~15.5k–15.7k input tokens. A better description was invited as a PR (open). — [#54](https://projects.blender.org/lab/blender_mcp/issues/54)
- Unclear namespace semantics (bpy not imported; no persistence) are tracked as open issues. — [#55](https://projects.blender.org/lab/blender_mcp/issues/55), [#64](https://projects.blender.org/lab/blender_mcp/issues/64)
- The maintainer closed a long AI-written report on doc mismatches, citing the [AI Contribution Policy](https://developer.blender.org/docs/handbook/contributing/ai_contributions/). — [#53](https://projects.blender.org/lab/blender_mcp/issues/53)
- `get_python_api_docs("bpy.types.Object.location")` returns `kind: "partial", found: false`. Workaround: `bpy.types.Object.Object.location` returns the definition (open). — [#62](https://projects.blender.org/lab/blender_mcp/issues/62)
- Every released bundle (≤ v1.0.3) ships the **Blender 5.1** API and manual. 5.2 docs exist only on unreleased `main`. — [commit ff54e4d8](https://projects.blender.org/lab/blender_mcp/commit/ff54e4d8f6b09502f2f466189cca0e52b4a91643); [compare v1.0.3...main](https://projects.blender.org/lab/blender_mcp/compare/v1.0.3...main)
- A "Live API Lookup" tool reading `bl_rna` from the running Blender was proposed and closed (2026-04-24) without one being added. — [#16](https://projects.blender.org/lab/blender_mcp/issues/16)

**Connection and timeouts**
- "Client timed out" on complex scenes. A patch raising `_CLIENT_TIMEOUT` 10→300 s, the countdown 2→200 and the request limit 10→50 MB was never sent as a PR. Closed for inactivity with "related fixes on 1.0.3 ... may have fixed this". — [#28](https://projects.blender.org/lab/blender_mcp/issues/28)
- No compatibility check between add-on and server versions (TODO, open). — [#10](https://projects.blender.org/lab/blender_mcp/issues/10)
- Windows `SO_REUSEADDR`: a second Blender can bind the port already in use with no error, and clients keep reaching the first one. Suggested fix: `SO_EXCLUSIVEADDRUSE` (open). — [#63](https://projects.blender.org/lab/blender_mcp/issues/63)
- The community server also defaults to 9876, so running both causes port conflicts. — [MeltFlex](https://www.meltflexai.com/blog/connect-claude-to-blender)
- Dual setup that works: community server on 9876, official on 9877 with `BLENDER_MCP_HOST=127.0.0.1`, `BLENDER_MCP_PORT=9877`. — [cc-blender-skill](https://github.com/RobLe3/cc-blender-skill/blob/main/plugin/skills/text-to-blender/references/mcp-backends.md)
- A local Qwen 3.6 model looped forever through `execute_blender_code`; a watchdog was requested (open). — [#33](https://projects.blender.org/lab/blender_mcp/issues/33)

**Install problems**
- MCP SDK 2.0 renamed FastMCP to MCPServer, causing `ModuleNotFoundError: No module named 'mcp.server.fastmcp'`. — [#44](https://projects.blender.org/lab/blender_mcp/issues/44)
  - Fixed by pinning `<2` — [commit 49507605](https://projects.blender.org/lab/blender_mcp/commit/495076055d3f4f8edba0e6c64084f00c6edcc826), released as v1.0.2.
  - Migration PRs [#43](https://projects.blender.org/lab/blender_mcp/pulls/43) and [#47](https://projects.blender.org/lab/blender_mcp/pulls/47) are still open.
- Windows Claude Desktop install fails with `uv.exe ... setuptools.build_meta.build_editable ... error in 'egg_base' option: '.' does not exist` (open, 13 comments). — [#24](https://projects.blender.org/lab/blender_mcp/issues/24)
  - One commenter isolated it to the **space in `Claude Extensions`** (it becomes `%20`), on a non-MSIX Windows 11 Home 26200 install.
  - Workaround from that comment: copy the folder to a path without spaces, run `uv sync` once, and register manually: `"blender": {"command":"uv","args":["run","--directory","C:\\Users\\<user>\\.blender-mcp","blender-mcp"]}`.
  - Others report silent install hangs.
  - dfelinto could not reproduce it and pointed at Claude.
- Windows **Smart App Control** blocks the unsigned uv-generated `.venv\Scripts\blender-mcp.exe` ("os error 4551", Code Integrity event 3077), which shows in Claude Desktop as "Server disconnected" (open). — [#56](https://projects.blender.org/lab/blender_mcp/issues/56), [#65](https://projects.blender.org/lab/blender_mcp/issues/65)
- macOS `.mcpb` install fails: "Operation not permitted" on `~/.cache/uv`. — [#57](https://projects.blender.org/lab/blender_mcp/issues/57)

### Inferences
- **Do not let agents use `*_for_cli` tools on our Windows setup** until PR #48 ships. They will burn 120 s per call. Missing `BLENDER_PATH` would fail first anyway, since Blender is not on PATH. For background/batch work use the shell directly (Claude Code Bash/PowerShell):
  - `"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup <file.blend> --python <script.py>`
  - Stdin explicitly empty (`< /dev/null` in Git Bash).
  - Write results to a JSON file instead of relying on stdout.
  - This avoids #42, #72 and #69 together.
- Alternative headless route: start a second, background bridge.
  - Command: `blender.exe --background --online-mode <file.blend> --command blender_mcp --port 9877`. `--command` must be last.
  - Point a second MCP server entry at it with `BLENDER_MCP_PORT=9877`.
  - In background mode, renders run blocking, so the thumbnail overrides do apply. Screenshots and jump tools are unavailable.
  - Use a distinct port because of #63.
- Treat `render_thumbnail_to_path` as broken in the GUI. Use the custom deferred render snippet from Q2, or set low resolution and samples explicitly before `render_viewport_to_path`. Always read the returned `filepath`.
- Keep any single tool call under ~150 s. The observed Claude Desktop limit was ~185 s and the server-side socket limit is 300 s. Heavy bakes or Cycles renders should run headless from the shell.
- Pass `size_limit_in_bytes` around 400–600 KB on screenshot calls while the Claude Desktop extension is the v1.0.0-era build. Upgrade the add-on to 1.0.3 for the send fix.
- If Smart App Control blocks the launcher, an untested route that avoids the unsigned `blender-mcp.exe` shim is to launch the server as `uv --directory <path>\mcp run python -m blmcp`. The `blmcp/__main__.py` entry exists.

### Gaps
- No maintainer timeline for merging PR #48 (stdin), PR #66 (thumbnail) or PR #68, or for the SDK 2 decision. The last commit on main is 2026-09-29.
- Unknown whether Claude Code's own MCP tool timeout is shorter or longer than 300 s for this server. No official number found.
- No root cause for the #72 add-on hang. No report of `jump_*` tools misbehaving with a multi-window layout.

## Q4. Blender's published positions and guidance on AI-assisted creation with this MCP, and community reports of building/editing assets with it

### Takeaway
Blender's stated intent is **assistive, not generative**:
- The roadmap framed the work as "natural language input" through the Python API.
- The repo and Anthropic's copy stress understanding, documentation, debugging and batch scripting.
- Brecht Van Lommel explicitly advised against focusing on generative builds ("like building an Eiffel Tower") in favour of "making Blender easier to learn and use".

Even so, the developers' own early tested prompts included scene building (rigs, walk cycles, GN animation, modifier staircases), and community skill authors run full procedural asset pipelines on it with results comparable to the community server. The community reaction to the Anthropic tie-in was polarized.

### Cited Findings

**Blender and Anthropic positions**
- Brecht Van Lommel (2026-02-08), replying to a multi-agent "Eiffel Tower" demo: "For Blender MCP, I would suggest to not focus on generative use cases like building an Eiffel Tower, and instead on making Blender easier to learn and use." His examples:
  - Explaining errors and suggesting fixes.
  - Analyzing complex scenes and automating tedious tasks.
  - Suggesting data-block names "and apply if approved".
  - Diagnosing armature deformation.
  - Memory optimisation, shading artifacts, export settings, asset checklists.
  - — [forum.blender.org thread 44260](https://forum.blender.org/t/built-a-multi-agent-system-on-top-of-blenders-python-api-and-sharing-architecture-learnings-for-the-mcp-discussion/44260) (redirect from devtalk.blender.org)
- Dalai Felinto (2026-01-30): Lab launch = "explore natural language input in Blender ... an MCP server for Blender, with communication handled via the existing Python API." — [blender.org](https://www.blender.org/development/projects-to-look-forward-to-in-2026/)
- Anthropic (2026-04-28): the connector "offers a natural-language interface to its Python API" for users to "analyze and debug entire Blender scenes" and "build custom scripts to batch-apply changes to objects in a scene", and it "lets Claude add new tools directly to Blender's interface". A May 1 update says Blender chose to receive Anthropic's support as a one-time donation rather than through the Development Fund. — [Anthropic](https://www.anthropic.com/news/claude-for-creative-work)
- CG Channel (2026-04-29):
  - Anthropic first joined as Corporate Patron (€240,000/yr), with funds "dedicated towards Blender core development", particularly the Python API.
  - The Lab built the extension.
  - Francesco Siddi quote: "In these uncertain and divisive times, we appreciate Anthropic offering support to the Blender project".
  - A later page note says the funding was downgraded to a one-off donation. — [CG Channel](https://www.cgchannel.com/?p=175735)
- Francesco Siddi (Blender CEO): the support is "towards helping Blender itself as a software, this is not an AI takeover." — [80.lv](https://80.lv/articles/blender-ceo-on-anthropic-funding-this-is-not-ai-takeover)
- Campbell Barton (2026-05-01) on sandboxing: "not something Blender can/should realistically solve". — [forum thread 45131](https://forum.blender.org/t/blender-mcp-server-after-claude-mcp-security-for-blender-scripting-3d-agent-notes/45131)
  - Brecht closed that thread on 2026-09-08, noting it "was somewhat appropriate on this forum when it related to the Blender MCP experiment", and pointed to new AI contribution guidelines that require human-written posts.
  - A question there about whether the MCP Lab would be cancelled after the May 1 policy update went unanswered.
- Reports and PRs must be human-written. dfelinto: "the value of those analysis is for them to be read by a human, and then written and presented accordingly". — [PR #38](https://projects.blender.org/lab/blender_mcp/pulls/38); [#53](https://projects.blender.org/lab/blender_mcp/issues/53); [AI Contribution Policy](https://developer.blender.org/docs/handbook/contributing/ai_contributions/)
- The developers' own tested prompt list (Claude Opus 4.6, Blender 5.1) includes generative scene building: an armature rig with IK, weight painting and an NLA walk cycle; a modifier-driven spiral staircase; Geometry Nodes text animation; Grease Pencil title cards. — [readme_prompt_examples.rst@v0.3.0](https://projects.blender.org/lab/blender_mcp/src/tag/v0.3.0/readme_prompt_examples.rst)
- No asset-library or generation integrations: the official server has no Poly Haven, Sketchfab or Hyper3D tools.
  - [YaroTech](https://note.com/yaoyoroztech/n/n2c1f2ff3cabe)
  - [blender-kiln reference](https://github.com/elithril/blender-kiln/blob/main/plugin/references/blender-mcp.md): "none for marketplaces or generation".
- StraySpark (a vendor selling a competing server) claims "Blender Foundation has stated no generative AI functionality is planned for Blender". Not verified against a primary source. — [StraySpark](https://www.strayspark.studio/blog/official-blender-mcp-server-comparison-2026)

**Community reports of building and editing with it**
- **blender-kiln** (Claude Code skill; Blender 5.2.2, Opus 5.5, Oct 2026; Apple M4 Pro) — [README](https://github.com/elithril/blender-kiln):
  - "The same 20 briefs on the official Lab MCP — 20 of 20, $84.97" (API-equivalent).
  - Wall times vs the ahujasid server:

    | Brief | ahujasid | official |
    |---|---|---|
    | Scripted lantern prop | 3.5 min | 3.6 min |
    | PBR texturing (crate) | 6.3 min | 7.0 min |
    | Rigged villager | 7.1 min | 6.5 min |
    | Batch of three props | 7.8 min | 8.6 min |
    | Photo rebuilds | 15.3–23.0 min | 16.7–19.1 min |
    | Quadruped walk | 12.9 min | 21.6 min |

  - Repairing a TRELLIS GLB (191,520 tris, 13,732 non-manifold edges) to 9,000 tris with 0 non-manifold; "On the official Lab MCP: the same 0 and 0, 0.93 MB".
  - An earlier kiln version "shipped the same 7 GLBs on it, for 10% less cost". — [kiln reference](https://github.com/elithril/blender-kiln/blob/main/plugin/references/blender-mcp.md)
- **cc-blender-skill v1.4.0**: "official-first dual MCP support", with its 35 core checks passing on Blender 5.1.1 through both MCP routes. — [README](https://github.com/RobLe3/cc-blender-skill)
- **MeltFlex**, tested 2026-10-13 on macOS with Blender 5.2.2 and Opus 5.5 — [MeltFlex](https://www.meltflexai.com/blog/connect-claude-to-blender):
  - An architect massing brief took 3 min 23 s, 13 turns, $0.69. Dimensions were checked correct, but an unmentioned slab overhang slipped through.
  - An interior brief took 10 min 53 s, 13 turns, $0.68. Exact sizes, but "rounded blocks", no glass and a flat sky.
  - Reported no hiccups once online access was enabled (paraphrase).
- **YaroTech** (2026-05-01): the official server is "inspection and analysis first"; install difficulty rated 2/5 vs 4/5 for the community server. Recommends the official server for 5.1+ inspection and automation, and the community one for asset generation. "The two are more of a complementary relationship than competitors." — [YaroTech](https://note.com/yaoyoroztech/n/n2c1f2ff3cabe)
- **Codex trajectories** (issue author lanius) hit `NameError` on `bpy`, non-persistent variables, and context bloat from `get_objects_summary`. — [#54](https://projects.blender.org/lab/blender_mcp/issues/54), [#55](https://projects.blender.org/lab/blender_mcp/issues/55)
- **Forks extending it:**
  - Rich-Siomporas/blender-agent adds a skills registry ("animation, boolean modelings, lighting ... skills"), a "welcome" tool and streamable HTTP for Claude Code, Codex and OpenCode. — [#39](https://projects.blender.org/lab/blender_mcp/issues/39); [blender-agent](https://projects.blender.org/Rich-Siomporas/blender-agent)
  - gaetan-deturche/blender-mcp-45 is a Blender 4.5 LTS fork with Windows fixes ("CLI tools no longer inherit the MCP stdio pipe"). — [GitHub](https://github.com/gaetan-deturche/blender-mcp-45)
- **BlenderArtists reaction** (2026-04-28):
  - Mostly ethics and funding objections; one staff member cancelled his Development Fund donation.
  - sozap: "I've seen a lot of BS MCP for blender that do nothing interesting nor useful", while prompting an AI to rename objects or diagnose a scene "looks interesting", and it is "better that it's done by BF". — [BlenderArtists](https://blenderartists.org/t/from-blender-mcp-to-3d-agent-anthropic-partners-with-blender-claude-ai-connector-now-official/1639106)
- **Blender Conference 2026**: the "AI-Enabled Tools & Workflows SIG" thread (Oct 12–16) does not mention the Lab MCP. Recording: https://www.youtube.com/watch?v=fwpdDKbwA08. — [BlenderArtists BCON thread](https://blenderartists.org/t/bcon2026-ai-enabled-tools-workflows-sig/1654900)

### Inferences
- For Triad Drift this server is a capable "hands in Blender" for **scripted procedural hard-surface modeling**: primitives, bmesh, modifiers (Bevel, Mirror, Array, Solidify, Boolean), Geometry Nodes, and emissive materials for glowing engines. Community benches show it completing full asset briefs at roughly the community server's cost and speed.
- What it does **not** provide: AI mesh generation or CC0 asset search. Asteroids, crystals and flora should be procedural (noise displacement, GN scattering). Humanoid crew may need external sources, which other notes should cover.
- Skills should encode the Lab's assistive norms:
  - Confirm before destructive scene changes, file resets (`read_homefile`) or deletes.
  - Never guess values: inspect first.
  - Report what was changed.
  - This matches both the server instructions and Brecht's "apply if approved" model.
- Do not auto-file upstream issues or PRs from agents: Blender's AI contribution policy rejects AI-written reports.

### Gaps
- I found no Reddit threads or video write-ups specifically about the official Lab server (search results were dominated by the community server).
- No Blender Conference 2026 talk on the Lab MCP was found.
- No official Blender statement formally limiting the MCP to non-generative use, beyond Brecht's suggestion and the README framing.
- No Q2/Q3 2026 Blender Lab activity report mentioning MCP was found. The Q1 report's gamedev.net repost returned 403.

## Q5. GitHub mirror, what changed between released versions, and roadmap

### Takeaway
There is **no official GitHub mirror**. The canonical home is projects.blender.org (Gitea), which has ~38 forks; only community forks exist on GitHub.
- v1.0.0 (2026-04-27) was the real launch.
- v1.0.1 to v1.0.3 (Sept 8–11) only pinned the MCP SDK below 2 and fixed large-response truncation (screenshots).
- `main` is two commits ahead, with unreleased 5.2 docs and a LICENSE file.
- There is no formal roadmap: open TODOs plus a stack of unmerged community fix PRs.

### Cited Findings

**Mirror and repo state**
- The GitHub `blender` org lists 21 repos (blender, cycles, addons mirrors, kitsu, …) and no MCP repo. `gh repo view blender/blender-mcp` returns "Could not resolve to a Repository". — gh CLI, [github.com/blender](https://github.com/blender)
- Community GitHub copies: `gaetan-deturche/blender-mcp-45`, `abdulovia/blender-mcp-lab`. — gh search; [blender-mcp-45](https://github.com/gaetan-deturche/blender-mcp-45)
- Gitea repo `lab/blender_mcp` — [Gitea API](https://projects.blender.org/api/v1/repos/lab/blender_mcp):
  - Created 2026-02-21; 37 stars; 38 forks.
  - 27 open issues and 5 open PRs at research time; wiki enabled; licence GPL-3.0.
- A LICENSE file was added only on 2026-09-29, after a request. — [#59](https://projects.blender.org/lab/blender_mcp/issues/59); [commit dbbf836a](https://projects.blender.org/lab/blender_mcp/commit/dbbf836ad4b1025f14a2b3b504c43903f39e0b04)
- Layout: `addon/blender_mcp_addon/` (extension), `mcp/` (server + MCPB), `chat_client/` (llama.cpp text client), `tests/` (headless Blender + LLM integration tests), `_misc/` (doc-update scripts). — [repo tree](https://projects.blender.org/lab/blender_mcp/src/branch/main)

**Release history** — [releases](https://projects.blender.org/lab/blender_mcp/releases); [tags](https://projects.blender.org/lab/blender_mcp/tags)

| Version | Date | Notes |
|---|---|---|
| Initial commit (Campbell Barton) | 2026-02-19 | [8f3096a3](https://projects.blender.org/lab/blender_mcp/commit/8f3096a3c4886a8435259f85fa240afe1ca7287b) |
| Feature work | 2026-03-03 → 03-24 | Online-access gate; `result` must be a dict; weak sandbox; capture of stdout/stderr; `get_objects_summary` (renamed from `get_scene_summary_collections` because "The LLM seems to want to access 'objects'"); HTTP transport for llama.cpp; min Blender 5.1; area vs window screenshot split; 1 MB screenshot downscale; deferred responses for renders; background-mode guards |
| v0.1.0 | 2026-03-24 | Pre-release "Basic MCP functionality" (`mcp-0.1.0.zip`) |
| 26.04.10 | 2026-04-10 | Pre-release "Initial MCPB Package" (`blender-0.2.0.mcpb`). The 2026-03-31 manifest change points docs to https://www.blender.org/lab/mcp-server/ ([230d5d4a](https://projects.blender.org/lab/blender_mcp/commit/230d5d4a291483fbeb64069a4306b95a9f6fe041)) |
| v0.3.0 | 2026-04-14 | Pre-release "Updated MCPB". Around it: safety annotations ([49f21ce1](https://projects.blender.org/lab/blender_mcp/commit/49f21ce11b117864c13d76d9d61a6828046b71d0)); server moved to `./mcp/` ([e86f189b](https://projects.blender.org/lab/blender_mcp/commit/e86f189b7c955f8957e862d10e01bda590410fe8)); MCPB files ([fba78b37](https://projects.blender.org/lab/blender_mcp/commit/fba78b37e3dd5f40cd40fdd70048dd403052971e)); `search_api_docs`/`search_manual_docs` ([19ac12dc](https://projects.blender.org/lab/blender_mcp/commit/19ac12dc8e2c5bfdf9b725fbad778ecb99e2ed2a)); `get_python_api_docs` (2026-04-23, [788463ee](https://projects.blender.org/lab/blender_mcp/commit/788463ee254089e56e758967fc6d237a7a44b07d)) |
| **v1.0.0** | 2026-04-27 | "Initial release" (`blender-1.0.0.mcpb`, `mcp-1.0.0.zip`). Anthropic connector launch 2026-04-28 |
| v1.0.1 tag | 2026-09-08 | Manifest bump; same day as "Lock MCP version to <2" ([49507605](https://projects.blender.org/lab/blender_mcp/commit/495076055d3f4f8edba0e6c64084f00c6edcc826)). **Not the same as** Claude's directory "1.0.1" (v1.0.0-era code; see Q1) |
| v1.0.2 | 2026-09-08 | "MCP SDK Fix: Make sure the system uses the MCP SDK <2." (version sync) |
| **v1.0.3** | 2026-09-11 | "Screenshot Fix: Fix the long standing screenshot capture tool." = [1e8d5976](https://projects.blender.org/lab/blender_mcp/commit/1e8d59762d97c199a9816396a665bae1a338f14f) + version bump; add-on 1.0.3; `blender-1.0.3.mcpb`. The 2 KiB envelope headroom fix ([4309a396](https://projects.blender.org/lab/blender_mcp/commit/4309a39646e644261624bfcd2bca669b343b7621), 2026-08-06) predates it |
| main (unreleased) | 2026-09-11 / 09-29 | "Update 5.2: manual & API" ([ff54e4d8](https://projects.blender.org/lab/blender_mcp/commit/ff54e4d8f6b09502f2f466189cca0e52b4a91643)) and LICENSE. Main's manifest files still say 1.0.2 / add-on 1.0.0, because the 1.0.3 bump commit `2cea8d56` exists only on the tag. See [compare v1.0.3...main](https://projects.blender.org/lab/blender_mcp/compare/v1.0.3...main) |

Sources for the March feature-work row: [commit log](https://projects.blender.org/lab/blender_mcp/commits/branch/main); [813a46fc](https://projects.blender.org/lab/blender_mcp/commit/813a46fce1d51d17b7289336a187e3d9a8bae245); [1ec94618](https://projects.blender.org/lab/blender_mcp/commit/1ec94618c4e50a0e168b6c5620915e9063903b3d); [3a04c21a](https://projects.blender.org/lab/blender_mcp/commit/3a04c21a05a6604d3493b76ba5ef617bf23f86f4)

**Roadmap signals (open work)**
- TODOs from ideasman42:
  - Add-on/server compatibility check — [#10](https://projects.blender.org/lab/blender_mcp/issues/10)
  - WEBP screenshots (needs Blender 5.2) — [#11](https://projects.blender.org/lab/blender_mcp/issues/11)
  - Render-to-temp without touching user settings (needs a Blender render operator change) — [#12](https://projects.blender.org/lab/blender_mcp/issues/12)
- MCP SDK 2 migration is undecided ("I will leave for @ideasman42 to decide"). — [PR #43](https://projects.blender.org/lab/blender_mcp/pulls/43), [PR #47](https://projects.blender.org/lab/blender_mcp/pulls/47)
- Pending fix PRs:
  - stdin fix — [PR #48](https://projects.blender.org/lab/blender_mcp/pulls/48)
  - Thumbnail render fix + `BLENDER_EEVEE` id — [PR #66](https://projects.blender.org/lab/blender_mcp/pulls/66)
  - CLI traceback — [PR #68](https://projects.blender.org/lab/blender_mcp/pulls/68)
- Invited but unsent: `get_objects_summary` description PR. — [#54](https://projects.blender.org/lab/blender_mcp/issues/54)
- Scope stance: no proprietary-platform installers ([PR #50](https://projects.blender.org/lab/blender_mcp/pulls/50), [#51](https://projects.blender.org/lab/blender_mcp/issues/51)); no Docker image planned ([#18](https://projects.blender.org/lab/blender_mcp/issues/18)); no rename ([#34](https://projects.blender.org/lab/blender_mcp/issues/34)).

### Inferences
- Practical version policy for the jam:
  - Keep the Claude Desktop extension: its server code is functionally fine apart from the screenshot headroom.
  - Update the Blender add-on to 1.0.3 from the Lab repo.
  - Teammates who self-host should clone and check out tag `v1.0.3`, or `main` for 5.2 docs, then `uv run blender-mcp`.
  - Locally patching `stdin=subprocess.DEVNULL` in `blmcp/tools_helpers/blender_cli.py` is the documented fix if `_for_cli` is wanted. That modifies third-party code, so it needs the user's OK.
- With the last commit on 2026-09-29 and several ready PRs unmerged, expect no upstream fixes before the Nov 15 deadline. Build skills around the workarounds above.

### Gaps
- No public roadmap document or Lab status post for Q2–Q4 2026 found.
- No changelog beyond release one-liners.
- How Anthropic builds its directory package (why "1.0.1" contains v1.0.0-era code) is undocumented.
