# Packaging reusable agent skills in a git repo for Claude Code, Codex, Cursor, Copilot and Gemini/Antigravity (as of 2026-11-12)

All pages were fetched on 2026-11-12. Most vendor doc pages carry no "last updated" date. Where a page or release has one, it is given inline. Claude Code facts are current through v2.1.322, published to npm on 2026-11-10.

## 1. What do Anthropic's current docs say about Claude Code project skills (location, frontmatter, naming and limits, size, bundled files, invocation, authoring best practices)?

### Takeaway
A Claude Code project skill is a folder at `.claude/skills/<skill-name>/SKILL.md` that you commit to the repo. Claude Code accepts the six Agent Skills spec fields plus about 15 of its own (for example `when_to_use`, `disable-model-invocation`, `user-invocable`, `allowed-tools`, `context: fork`, `paths`, `shell`, `effort`, `hooks`). Every field is optional in Claude Code, and the `name` defaults to the folder name. The `SKILL.md` should stay under 500 lines, with detail moved into files linked one level deep. Claude loads a skill automatically when its description matches the task, or when you type `/skill-name`. Anthropic's best-practices guide calls for:
- short skills
- third-person descriptions that say what the skill does and when to use it ("Use when…")
- progressive disclosure
- scripts for deterministic steps
- evaluations written before the docs
- fully qualified MCP tool names

### Cited Findings

#### Location and discovery
- **Locations.** Project skills live at `.claude/skills/<skill-name>/SKILL.md` and are shared by committing them. Other locations:
  - personal: `~/.claude/skills/<skill-name>/SKILL.md`
  - enterprise: inside the managed-settings directory
  - plugins: `<plugin>/skills/<skill-name>/SKILL.md`, invoked as `/plugin-name:skill-name`
  - `--add-dir` directories
  - skills synced from the claude.ai account
  — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Precedence.** Enterprise wins over personal, and personal wins over project. A skill wins over a `.claude/commands/` file of the same name. Plugin skills are namespaced, so they never collide. Legacy `.claude/commands/*.md` files still work and take the same frontmatter, except `name` and `paths`. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Directory walk.** Project skills load from the start directory up to the repo root. Skills in a nested `<subdir>/.claude/skills/` load only the first time Claude reads or edits a file in that subdirectory. Since v2.1.257, `/add-dir` loads them sooner. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
  - On a name clash, the nested skill appears as `<dir>:<name>`. — [Claude Code CHANGELOG](https://github.com/anthropics/claude-code/blob/main/CHANGELOG.md)
- **Worktrees.** In a linked git worktree, Claude Code searches parent directories only up to the worktree root. From v2.1.277, if the worktree has no `.claude/skills`, Claude Code loads the main checkout's project skills instead. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Symlinks.** A `<skill-name>` entry in the enterprise, personal or project location can be a symlink to a directory elsewhere. Claude Code reads `SKILL.md` from the target, and loads the skill only once even when several locations point to the same target. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Skill folder as a plugin.** A skill folder that contains `.claude-plugin/plugin.json` loads as a plugin named `<name>@skills-dir`, with its own agents, hooks and MCP servers. In a project's `.claude/skills/`, this requires accepting the workspace trust dialog. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Live reload.** Live change detection covers only the text of `SKILL.md`. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)

#### Frontmatter reference (Claude Code)
- **General rules.** All fields are optional; `description` is recommended. Field names must match exactly, and unknown fields are silently ignored. Booleans accept `true/false`, `yes/no`, `on/off` and `1/0` in any case. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Fields:**
  - `name`: defaults to the directory name.
  - `description`: falls back to the first non-empty line of the body.
  - `when_to_use`: appended to `description`. The two together are truncated at 1,536 characters in the skill listing.
  - `argument-hint`, `arguments`
  - `disable-model-invocation`: Claude cannot auto-load the skill, and it cannot be preloaded into subagents.
  - `user-invocable: false`: hidden from the `/` menu.
  - `allowed-tools`: tools allowed without a prompt during the turn that invokes the skill only. The grant clears on the next message.
  - `disallowed-tools`
  - `model`: applies to the current turn only.
  - `effort`: low, medium, high, xhigh or max.
  - `context: fork` with `agent` and `background`: runs the skill in a subagent.
  - `hooks`
  - `paths`: globs that limit auto-activation to matching files.
  - `shell`: `bash`, the default, or `powershell`.
  - `metadata`: Claude Code ignores its contents.
  - `license` and `compatibility`: accepted but not acted on; `compatibility` is capped at 500 characters.

  — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Outside Claude Code** (claude.ai uploads, the Skills API, `package_skill.py` from anthropics/skills), only the six spec keys are allowed: `name`, `description`, `license`, `compatibility`, `metadata`, `allowed-tools`. Any other key is a hard error, for example `Unexpected key(s) in SKILL.md frontmatter: argument-hint. Allowed properties are: allowed-tools, compatibility, description, license, metadata, name`. Claude Code-only body features such as `!` command injection do not work in claude.ai or the API. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Undocumented keys.** The changelog says the keys `display-name`, `default-enabled` and `fallback` now accept kebab-case, snake_case and camelCase. None of the three appears in the frontmatter reference, so treat them as undocumented. — [Claude Code CHANGELOG](https://github.com/anthropics/claude-code/blob/main/CHANGELOG.md)

#### Naming rules and character limits
- **Claude Code refuses a skill name** that contains any of these:
  - zero-width or control characters
  - private-use or unassigned characters
  - direction-changing characters
  - non-plain blanks
  - `<` or `>`

  — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Length.** Since a recent release, a skill's `name` longer than 256 characters is ignored. — [Claude Code CHANGELOG](https://github.com/anthropics/claude-code/blob/main/CHANGELOG.md)
- **Reserved names.** The folder name `synced` is reserved. Skill folders in the `anthropic-skills` or `claude-ai` namespace no longer load. — [Claude Code: Skills](https://code.claude.com/docs/en/skills); [Claude Code CHANGELOG](https://github.com/anthropics/claude-code/blob/main/CHANGELOG.md)
- **Platform-wide limits** (Anthropic best-practices page):
  - `name`: at most 64 characters; lowercase letters, numbers and hyphens only; no XML tags; must not contain the reserved words `anthropic` or `claude`.
  - `description`: non-empty, at most 1,024 characters, no XML tags.

  — [Anthropic: Skill authoring best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)
- **Listing cap.** In the Claude Code listing, `description` plus `when_to_use` is capped at 1,536 characters per skill; the cap is configurable with `skillListingMaxDescChars`. `<` and `>` in a description display as `&lt;` and `&gt;`. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)

#### Invocation
- **Automatic and manual.** Claude loads a skill when its description matches the task, and you can always type `/skill-name`. Up to six skills can be stacked in the terminal; in `-p` or Agent SDK runs, only the first one loads. Writing `/name` in the middle of a sentence grants permission but does not run the skill. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Who can invoke:**

  | Frontmatter | User invokes | Claude invokes | Description in context |
  |---|---|---|---|
  | (default) | yes | yes | yes |
  | `disable-model-invocation: true` | yes | no | no |
  | `user-invocable: false` | no | yes | yes |

  — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Substitutions:** `$ARGUMENTS`, `$ARGUMENTS[N]` and `$N`, `$name`, `${CLAUDE_SKILL_DIR}`, `${CLAUDE_PROJECT_DIR}`, `${CLAUDE_SESSION_ID}`, `${CLAUDE_EFFORT}`. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Dynamic context.** `` !`cmd` `` and ` ```! ` blocks run before Claude sees the skill, and their output replaces them. A non-zero exit aborts the whole invocation. Admins can turn this off with the `disableSkillShellExecution` policy. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Forked skills.** `context: fork` runs the skill in a subagent that works in the background by default; `background: false` makes the turn wait for it. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)

#### Size and context budgets
- **Body size.** Keep `SKILL.md` under 500 lines and move detailed reference material into separate files. A skill body stays in context once loaded, so every line is a recurring token cost. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **After auto-compaction**, Claude Code re-attaches the first 5,000 tokens of each invoked skill. The combined budget is 25,000 tokens, filled from the most recently invoked skill. The docs therefore advise putting the most important instructions near the top of `SKILL.md`. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Listing budget.** The skill listing gets about 1% of the model's context window. When it overflows, the descriptions of the least-used skills are dropped first. Controls:
  - `skillListingBudgetFraction`
  - `SLASH_COMMAND_TOOL_CHAR_BUDGET`
  - `skillOverrides` set to `"name-only"`
  - `/doctor` estimates the listing's context cost.

  — [Claude Code: Skills](https://code.claude.com/docs/en/skills)

#### Bundled files and scripts
- **Layout.** The documented layout is `SKILL.md` (the overview), with `reference.md` and `examples.md` loaded when needed, and `scripts/helper.py`, which is executed rather than loaded. Reference every supporting file from `SKILL.md` so Claude knows when to load it. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Script paths.** Use `${CLAUDE_SKILL_DIR}` so script paths resolve from personal, project or plugin installs. Pairing it with a matching rule such as `allowed-tools: Bash(${CLAUDE_SKILL_DIR}/scripts/render.sh *)` lets the bundled script run without a permission prompt. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)

#### Validation and evaluation tooling
- **Validation.** `claude plugin validate .claude/skills` reports `SKILL.md` files whose frontmatter does not parse (v2.1.233+). Malformed YAML still loads the body with empty metadata, so `/name` works but automatic matching does not. `--debug` shows the parse error. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Unused skills.** `/skill-doctor` (v2.1.252+) finds unused skills. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Evals.** The skill-creator skill runs isolated per-test subagents, grading, with/without-skill benchmarks, blind A/B comparisons of skill versions, and description tuning against should-trigger and should-not-trigger prompts. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)

#### Windows-specific behavior
- **Shell.** `shell: powershell` runs `!` blocks through the PowerShell tool. With the default `shell: bash`, a skill fails on Windows when Git Bash is missing; the error says the skill requires bash but Git Bash was not found. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **CRLF bug.** Multi-line `!` blocks used to fail on Windows when the file had CRLF line endings. The fix is in a v2.1.29x release (between v2.1.289 and v2.1.300, roughly late September to mid-October 2026). — [Claude Code CHANGELOG](https://github.com/anthropics/claude-code/blob/main/CHANGELOG.md)

#### Anthropic "Skill authoring best practices" (platform.claude.com)
- **Be concise.** The guide calls the context window a shared, public resource and tells authors to assume Claude is already smart: keep only what Claude does not already know. — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)
- **Degrees of freedom.** Match specificity to how fragile the task is:
  - high freedom: plain text guidance
  - medium freedom: pseudocode or parameterized scripts
  - low freedom: exact scripts, for example "Run exactly this script … Do not modify the command"

  — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)
- **Test models.** Test with every model you plan to use (Haiku, Sonnet, Opus). — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)
- **Naming.** Prefer the gerund form (`processing-pdfs`). Noun phrases (`pdf-processing`) and action forms (`process-pdfs`) are acceptable. Avoid `helper`, `utils`, `tools`, overly generic names, and the reserved words. — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)
- **Descriptions.**
  - Always write in third person, because the description is injected into the system prompt. "Processes Excel files…" is good; "I can help…" and "You can use this…" are not.
  - Include both what the skill does and when to use it, with key terms.
  - Example: "Extract text and tables from PDF files, fill forms, merge documents. Use when working with PDF files or when the user mentions PDFs, forms, or document extraction."

  — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)
- **Progressive disclosure.**
  - Keep the `SKILL.md` body under 500 lines.
  - Patterns: a high-level guide with references, organization by domain (`reference/finance.md`, …), or conditional details.
  - Keep references one level deep from `SKILL.md`, because Claude may only `head -100` files that are nested deeper.
  - Give reference files over 100 lines a table of contents.

  — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)
- **Workflows.** Give checklists that Claude can copy and tick off, and validation loops (validator, fix, repeat). Avoid time-sensitive text (put it in an "Old patterns" section). Use consistent terminology, templates, input/output examples, and conditional workflow branches. — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)
- **Evaluation-first.** Build at least three evals before writing extensive docs and measure a no-skill baseline. Iterate with "Claude A" writing the skill and "Claude B" testing it, and watch how Claude navigates the files. — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)
- **Anti-patterns.** Windows-style backslash paths (always use forward slashes), and offering too many options instead of a default with an escape hatch. — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)
- **Scripts.**
  - "Solve, don't defer": handle errors inside the script, and avoid unexplained "voodoo constants".
  - Pre-made utility scripts are more reliable, save tokens, and run without their contents being loaded; only their output costs tokens.
  - Say explicitly whether Claude should run a script or read it as reference.
  - Use plan-validate-execute with a validated intermediate file for batch or destructive operations.
  - List required packages; don't assume tools are installed.

  — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)
- **MCP tools.** Always use fully qualified tool names in the form `ServerName:tool_name`, for example "Use the BigQuery:bigquery_schema tool". Without the server prefix, Claude may fail to find the tool when several MCP servers are connected. — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)

### Inferences
- **For skills shared across agents and products:**
  - Keep the frontmatter to the six spec keys.
  - Always set `name` equal to the folder name, at most 64 characters, lowercase with hyphens, and without "claude" or "anthropic".
  - Keep `description` at or under 1,024 characters. Claude Code allows 1,536, but the spec and other tools cap at 1,024.
  - Put the trigger text in `description`, not in `when_to_use`, which only Claude Code reads.
  - Add Claude Code-only keys (`disable-model-invocation`, `paths`, `context`, `shell`) only where other agents can safely ignore them. Claude Code and other tools ignore unknown keys locally; the hard errors come only from claude.ai and API uploads.
- **Order the Blender skills by importance.** Compaction keeps only the first 5,000 tokens of each invoked skill, so put the critical safety rules and gotchas in the first screen of each `SKILL.md` (for example "inspect the scene before mutating", "never delete without confirmation", and the export settings).
- **Line endings.** Given the CRLF bug history, a `.gitattributes` that forces `eol=lf` for `SKILL.md`, `*.md` and `*.py` under the skills directories is cheap insurance on Windows. This is a recommendation, not something Anthropic documents.

### Gaps
- The docs don't give a version or date for when each frontmatter field (for example `effort`, `paths`, `shell`, `arguments`, `background`) was added. Only scattered changelog entries exist.
- I did not re-fetch the Oct 2025 anthropic.com engineering post on Agent Skills; these notes rely on the current docs instead.
- `display-name`, `default-enabled` and `fallback` appear in the changelog but not in the frontmatter reference, so their purpose is unconfirmed.

## 2. What is the Agent Skills open standard as of late 2026, who has adopted it, which in-repo directories does each agent read, and does Claude Code read a cross-agent directory?

### Takeaway
The agentskills.io spec is still small:
- a folder with `SKILL.md`
- required `name` (at most 64 characters, lowercase and hyphens, must match the folder) and `description` (at most 1,024 characters)
- optional `license`, `compatibility` (at most 500 characters), `metadata` and an experimental `allowed-tools`
- optional `scripts/`, `references/` and `assets/` folders

On 2026-10-22 Anthropic donated the standard to the Linux Foundation's Agentic AI Foundation (AAIF), citing more than 70 integrating platforms. `.agents/skills/` is the de facto cross-client directory: Codex, Cursor, Copilot, Gemini CLI and Antigravity all read it. **Claude Code does not.** Its v2.1.277 AGENTS.md fallback (published 2026-09-18) explicitly excludes anything under `.agents/`, and a feature request to load `.agents/skills` was still open on 2026-11-12.

### Cited Findings

#### The spec (agentskills.io/specification; no version number or date on the page)
- **Layout.** A skill is a directory with a required `SKILL.md` (YAML frontmatter plus Markdown) and optional `scripts/`, `references/` and `assets/`. Any other files are allowed. — [Agent Skills spec](https://agentskills.io/specification)
- **Required fields:**
  - `name`: 1–64 characters; lowercase `a-z`, `0-9` and `-` only; no leading, trailing or consecutive hyphens; must match the parent directory name.
  - `description`: 1–1,024 characters; says what the skill does and when to use it; includes keywords.

  — [Agent Skills spec](https://agentskills.io/specification)
- **Optional fields:**
  - `license`: short form, or the name of a bundled license file.
  - `compatibility`: 1–500 characters, for environment requirements such as "Requires git, docker, jq…". Most skills don't need it.
  - `metadata`: a string-to-string map; use reasonably unique keys.
  - `allowed-tools`: space-separated, experimental; support varies by agent.

  — [Agent Skills spec](https://agentskills.io/specification)
- **Progressive disclosure.** About 100 tokens of metadata load at startup. The instructions should stay under 5,000 tokens and are loaded on activation. Resources load only when needed. `SKILL.md` should stay under 500 lines. — [Agent Skills spec](https://agentskills.io/specification)
- **File references.** Use relative paths from the skill root (for example `references/REFERENCE.md`, `scripts/extract.py`), kept one level deep. Validate a skill with `skills-ref validate ./my-skill`. — [Agent Skills spec](https://agentskills.io/specification)

#### Governance and adoption
- **Origin.** Anthropic originally developed the format and released it as an open standard that is open to contributions. — [agentskills.io overview](https://agentskills.io/home)
- **Donation to AAIF.** On 2026-10-22 Anthropic donated the standard, the Agent Skills name and logo, the GitHub organization and the agentskills.io domain to AAIF under the Linux Foundation.
  - The project will publish versioned releases and use a formal proposal process.
  - Engineers from Anthropic, Google, OpenAI and other companies maintain it as individuals; David Soria Parra and Mahesh Murag are the Lead Maintainers.
  - Anthropic says more than 70 agent platforms have integrated Agent Skills, including ChatGPT, Cursor, GitHub Copilot and VS Code.

  — [Anthropic: Donating Agent Skills to the AAIF](https://www.anthropic.com/news/donating-agent-skills-to-the-agentic-ai-foundation)
- **Original release date.** The open-standard release is dated 2025-12-18 by secondary coverage only; I did not fetch an Anthropic primary source for that date. — [Unite.AI (secondary)](https://www.unite.ai/anthropic-opens-agent-skills-standard-continuing-its-pattern-of-building-industry-infrastructure)
- **Showcase.** The agentskills.io client showcase page renders client-side, and no adopter list could be extracted from it. — [Client Showcase](https://agentskills.io/clients.md)

#### Cross-client directory convention (agentskills.io implementation guide)
- **Where to scan.** Clients should scan both their own directory (`<project>/.<client>/skills/`, `~/.<client>/skills/`) and the `.agents/skills/` convention at project and user scope. The guide describes `.agents/skills/` as a widely adopted cross-client convention, but notes that the spec itself does not mandate locations. Some implementations also scan `.claude/skills/` for compatibility, and some scan ancestor directories up to the git root. — [How to add skills support](https://agentskills.io/client-implementation/adding-skills-support)
- **Collisions.** Project-level skills override user-level skills. For collisions within the same scope, the client can keep the first or the last found, but should log a warning. — [How to add skills support](https://agentskills.io/client-implementation/adding-skills-support)
- **Trust.** Clients should consider loading project skills only from trusted folders. — [How to add skills support](https://agentskills.io/client-implementation/adding-skills-support)
- **Lenient validation.** A name that doesn't match the folder, or is longer than 64 characters, produces a warning and the skill still loads. A missing description, or YAML that cannot be parsed at all, skips the skill. Unquoted values containing a colon (for example `description: Use this skill when: …`) are the most common cross-client YAML break. — [How to add skills support](https://agentskills.io/client-implementation/adding-skills-support)
- **Catalog and context.** Each skill costs about 50–100 tokens in the catalog. Clients should protect skill content from compaction and allowlist skill directories for file reads. — [How to add skills support](https://agentskills.io/client-implementation/adding-skills-support)

#### Exact directories each agent reads (current docs, fetched 2026-11-12)

| Agent | In-repo (project) skill dirs | User-level dirs | Reads `.claude/skills`? | Reads `.agents/skills`? |
|---|---|---|---|---|
| Claude Code | `.claude/skills/` (start dir up to repo root; nested `<subdir>/.claude/skills/` on demand) | `~/.claude/skills/` | Yes (native) | **No** |
| OpenAI Codex | `$CWD/.agents/skills`, each parent up to `$REPO_ROOT/.agents/skills` | `$HOME/.agents/skills`; source code also loads `$CODEX_HOME/skills` (default `~/.codex/skills`); admin `/etc/codex/skills`; bundled system skills | Not documented | Yes (native) |
| Cursor (2.4+, released Jan 22, 2026) | `.agents/skills/`, `.cursor/skills/` (anywhere in the repo); compatibility: `.claude/skills/`, `.codex/skills/` | `~/.agents/skills/`, `~/.cursor/skills/`; compatibility: `~/.claude/skills/`, `~/.codex/skills/` | Yes | Yes |
| GitHub Copilot (CLI, app, cloud agent, code review, VS Code / Visual Studio / JetBrains agent mode) | `.github/skills`, `.claude/skills`, `.agents/skills` | `~/.copilot/skills`, `~/.agents/skills` | Yes | Yes |
| Gemini CLI (replaced by Antigravity CLI on June 18, 2026) | `.gemini/skills/` or the `.agents/skills/` alias (the alias wins) | `~/.gemini/skills/` or `~/.agents/skills/` | Not mentioned | Yes |
| Google Antigravity (app, CLI `agy`, IDE) | `.agents/skills/` and every parent up to the repo root; legacy `.agent/skills` | `~/.gemini/config/skills/`; CLI only: `~/.gemini/antigravity-cli/skills/`, `~/.gemini/skills/`; IDE legacy `~/.gemini/antigravity/skills/` | Not mentioned | Yes |

Sources for the table:
- Claude Code: [Claude Code: Skills](https://code.claude.com/docs/en/skills); [Claude Code: Memory / AGENTS.md](https://code.claude.com/docs/en/memory)
- Codex: [Codex: Build skills](https://learn.chatgpt.com/docs/build-skills) (developers.openai.com/codex/skills now 308-redirects here); [codex capability_locations.rs](https://github.com/openai/codex/blob/main/codex-rs/exec-server/src/discoverV2/capability_locations.rs)
- Cursor: [Cursor: Skills](https://cursor.com/docs/context/skills); [Cursor 2.4 changelog](https://cursor.com/changelog/2-4)
- Copilot: [GitHub: About agent skills](https://docs.github.com/en/copilot/concepts/agents/about-agent-skills); [GitHub: Create skills](https://docs.github.com/en/copilot/how-tos/use-copilot-agents/coding-agent/create-skills)
- Gemini CLI: [Gemini CLI: Skills](https://geminicli.com/docs/cli/skills/) (page last updated Apr 30, 2026; banner says it was replaced by Antigravity CLI on June 18, 2026)
- Antigravity: [Antigravity: Agent skills](https://antigravity.google/docs/skills)

- **Codex's old directory.** Codex's earlier repo location, `.codex/skills`, was described in Codex PR #10317 as "will remain but will be deprecated soon", and `~/.codex/skills` was kept for backwards compatibility in PR #10437. I saw this only in search snippets of a third-party mirror and did not fetch it directly. The current Codex docs list no `.codex/skills` location. — [Mirror of openai/codex commit for #10317 (secondary)](https://forge.lthn.ai/core/core-agent-ide/commit/39a6a84097ec4bdf180f42e5690f71159faa5670); [Mirror of #10437 (secondary)](https://forge.lthn.ai/core/core-agent-ide/commit/e24058b7a872054e3e76f9301d64eccccb75811d); [Codex: Build skills](https://learn.chatgpt.com/docs/build-skills)
- **Codex details:**
  - It supports symlinked skill folders and follows the symlink target.
  - `SKILL.md` must include `name` and `description`.
  - Explicit invocation is `$skill` or `/skills` in the CLI and IDE (`@` in ChatGPT); Codex can also pick skills implicitly.
  - An optional `agents/openai.yaml` can carry `interface`, `policy.allow_implicit_invocation` (default true) and `dependencies.tools` (MCP).
  - `[[skills.config]]` in `~/.codex/config.toml` enables or disables skills by `name` or `path`.

  — [Codex: Build skills](https://learn.chatgpt.com/docs/build-skills)
- **Cursor details.** Frontmatter: `name` (required, must match the folder), `description` (required), `paths`, `disable-model-invocation`, `icon`, `color` and `metadata`. The page does not list `license` or `compatibility`. Skills can be invoked automatically, with `/`, or as a Custom Mode (Alt+Enter on Windows). `/migrate-to-skills` converts dynamic rules and commands into `.cursor/skills/`. — [Cursor: Skills](https://cursor.com/docs/context/skills)
- **Copilot details.**
  - Frontmatter: `name` (required, lowercase with hyphens), `description` (required), `license` (optional), `allowed-tools` (optional).
  - When a skill is invoked, Copilot discovers all files in the skill directory.
  - `gh skill` (GitHub CLI 2.90.0+, public preview) installs and pins skills.
  - GitHub warns that skills are not verified by GitHub and may contain prompt injections.

  — [GitHub: Create skills](https://docs.github.com/en/copilot/how-tos/use-copilot-agents/coding-agent/create-skills)
- **Gemini CLI details.** Gemini activates a skill through an `activate_skill` tool call followed by a user consent prompt. Commands include `/skills list|link|enable|disable|reload` and `gemini skills install <git-url> --scope workspace`. — [Gemini CLI: Skills](https://geminicli.com/docs/cli/skills/)
- **Antigravity details.** `description` is required. `name` is optional and defaults to the folder name. In the CLI, every skill becomes a slash command. — [Antigravity: Agent skills](https://antigravity.google/docs/skills)

#### Does Claude Code read a cross-agent directory?
- **AGENTS.md support (v2.1.277).** v2.1.277 added AGENTS.md support: in a project with no CLAUDE.md, Claude Code reads AGENTS.md instead; this is configurable under "Project instructions" in `/config`. — [Claude Code CHANGELOG](https://github.com/anthropics/claude-code/blob/main/CHANGELOG.md)
  - npm published 2.1.277 at 2026-09-18T16:22Z, which confirms the "Sept 2026" date in the question. — [npm registry: @anthropic-ai/claude-code](https://registry.npmjs.org/@anthropic-ai/claude-code)
- **What it does not read.** The memory docs list as **not read**: `AGENTS.local.md`, `AGENTS.override.md`, or anything under a `.agents/` directory. AGENTS.md loads only when no `CLAUDE.md`, `.claude/CLAUDE.md` or `CLAUDE.local.md` exists in the working directory or above it. Alternatively, `CLAUDE.md` can import it with `@AGENTS.md`. — [Claude Code: Memory / AGENTS.md](https://code.claude.com/docs/en/memory)
- **No `.agents/skills` location.** The skills docs list only `.claude/skills` locations. A grep of the changelog through v2.1.322 (2026-11-10) finds no `.agents/skills` support. — [Claude Code: Skills](https://code.claude.com/docs/en/skills); [Claude Code CHANGELOG](https://github.com/anthropics/claude-code/blob/main/CHANGELOG.md)
- **Open requests.** Both of these issues were open on 2026-11-12:
  - #107421 "[Feature Request] Load skills from ~/.agents/skills/ and <project>/.agents/skills/", opened 2026-11-05
  - #31005 "Support for AGENTS.md and .agents/skills/…", opened 2026-03-05

  — [anthropics/claude-code#107421](https://github.com/anthropics/claude-code/issues/107421); [anthropics/claude-code#31005](https://github.com/anthropics/claude-code/issues/31005)

#### Writing descriptions that trigger correctly (agentskills.io)
- **Phrasing.**
  - Use imperative phrasing ("Use this skill when…").
  - Describe the user's intent, not the implementation.
  - Lean toward listing more contexts, including ones where the user doesn't name the domain ("even if they don't explicitly mention…").
  - Keep it to a few sentences, under 1,024 characters.

  — [Optimizing skill descriptions](https://agentskills.io/skill-creation/optimizing-descriptions)
- **Trigger evals.**
  - About 20 queries: 8–10 that should trigger and 8–10 that should not.
  - The most valuable negatives are near-misses that share keywords.
  - Run each query 3 times; the pass threshold is a trigger rate of 0.5.
  - Split 60/40 into train and validation sets, and choose the description by its validation pass rate.
  - About five iterations is usually enough.
  - Agents often skip skills for simple one-step tasks they can do alone.

  — [Optimizing skill descriptions](https://agentskills.io/skill-creation/optimizing-descriptions)
- **Content guidance.**
  - Start from real expertise or project artifacts, not generic LLM output.
  - Add only what the agent lacks.
  - Scope each skill as a coherent unit, like a function.
  - Give defaults rather than menus, and procedures rather than single answers.
  - A "Gotchas" section is often the highest-value content; keep it in `SKILL.md`.
  - Tell the agent *when* to load each reference file (for example "Read `references/api-errors.md` if the API returns non-200").
  - Bundle a script once you see the agent rewriting the same logic on every run.

  — [Best practices for skill creators](https://agentskills.io/skill-creation/best-practices)

#### Catalog budgets by client
- **Claude Code:** about 1% of the context window for the listing, with up to 1,536 characters per entry. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
- **Codex:** at most 2% of the context window (8,000 characters if the window size is unknown). `skills.max_context_tokens` can raise this, capped at 10,000 tokens. Codex may shorten descriptions when many skills are installed. — [Codex: Build skills](https://learn.chatgpt.com/docs/build-skills); [codex config.schema.json](https://github.com/openai/codex/blob/main/codex-rs/core/config.schema.json)

### Inferences
- **No single directory works for all five agents** natively as of 2026-11-12:
  - `.claude/skills/`: Claude Code, Cursor, Copilot
  - `.agents/skills/`: Codex, Cursor, Copilot, Gemini CLI, Antigravity
- **Descriptions.** Anthropic's "third person" rule and agentskills.io's "imperative" advice can both be met with one pattern: start with a third-person capability statement, then add "Use when …" triggers. Both docs' own examples follow this shape.
- **Quote descriptions.** Put description strings in quotes, or use a `>` block scalar, if they contain colons. That avoids the most common cross-client YAML parse failure.

### Gaps
- No primary-source list of the "70+" adopters was retrievable, because the showcase page renders client-side.
- I could not find an official Codex changelog entry dating the `.codex/skills` → `.agents/skills` move; I have only PR text seen through a mirror and search snippets.
- I did not verify whether Codex (Nov 2026) still loads a repo-level `.codex/skills`. It appears only in test files in the source search, and the docs omit it.
- The Copilot docs give no field length limits, and Cursor's docs do not say how it handles `license` or `compatibility`.

## 3. On Windows, what is the most robust way to make one set of skills discoverable by both Claude Code and Codex/Cursor in the same repo?

### Takeaway
Keep the canonical skills in `.claude/skills/<name>/`. Claude Code, Cursor and Copilot all read that directory natively, so the Claude Code + Cursor pairing needs no links at all. Only Codex needs a bridge, because its docs list only `.agents/skills` for repos and it has no setting to add skill folders.

For Codex, the most robust bridge on Windows is a real-file copy (mirror) of each skill into `.agents/skills/`, generated by a script. OpenAI's own Claude-to-Codex migrator does exactly this. Add a short "Skills" index in AGENTS.md as a fallback pointer.

Committed git symlinks are the most fragile option on Windows. Without Developer Mode (or the symlink privilege) and `core.symlinks=true` at clone time, git checks them out as small text files and the agent silently finds no skills.

### Cited Findings

#### Who reads what (coverage)
- **Claude Code** reads `.claude/skills/` and does not read anything under `.agents/`. — [Claude Code: Skills](https://code.claude.com/docs/en/skills); [Claude Code: Memory](https://code.claude.com/docs/en/memory)
- **Codex** documents only `.agents/skills` (from the CWD up to the repo root) for repos. — [Codex: Build skills](https://learn.chatgpt.com/docs/build-skills)
  - Its `skills` config supports only `bundled.enabled`, `config[]` (enable or disable by `name` or absolute `path`), `include_instructions` and `max_context_tokens`. There is no option for extra skill roots. — [codex config.schema.json](https://github.com/openai/codex/blob/main/codex-rs/core/config.schema.json)
- **Cursor** reads `.agents/skills/`, `.cursor/skills/`, and, for compatibility, `.claude/skills/` and `.codex/skills/`. — [Cursor: Skills](https://cursor.com/docs/context/skills)
- **Copilot** reads `.github/skills`, `.claude/skills` and `.agents/skills`. — [GitHub: About agent skills](https://docs.github.com/en/copilot/concepts/agents/about-agent-skills)

#### Symlink support in the agents
- **Claude Code** supports a symlinked `<skill-name>` entry and loads a target once even when several locations point to it. — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
  - Separately, for `.claude/rules/`, a symlink whose target is outside the working directory is treated like an external import that needs one-time approval. Network-path (UNC) symlink targets don't load. The docs don't say whether the same applies to skills. — [Claude Code: Memory](https://code.claude.com/docs/en/memory)
- **Codex** supports symlinked skill folders and follows the target when scanning. — [Codex: Build skills](https://learn.chatgpt.com/docs/build-skills)
- **Duplicates.** In Codex, skills that share a `name` are not merged, and both can appear in selectors. — [Codex: Build skills](https://learn.chatgpt.com/docs/build-skills)
  - The cross-client guide says a same-scope collision should keep the first or last skill found and log a warning. — [How to add skills support](https://agentskills.io/client-implementation/adding-skills-support)

#### Git and Windows symlink mechanics
- **Git (`core.symlinks`).** When it is false, symbolic links are checked out as small plain files that contain the link text. The default is true, but `git clone` and `git init` probe the filesystem and set it to false if appropriate. — [git Documentation/config/core.adoc](https://github.com/git/git/blob/master/Documentation/config/core.adoc)
- **Git for Windows:**
  - Symlink support is disabled by default, though existing symlinks can still be read. Enable it with `core.symlinks=true`, or `git clone -c core.symlinks=true <URL>`.
  - It turns on automatically only for a non-admin user with Developer Mode on a recent Windows 10 build.
  - Creating symlinks requires `SeCreateSymbolicLinkPrivilege`, which only Administrators have by default. Developer Mode lifts the UAC restriction from Windows 10 1703 on.
  - Symlinks work only on NTFS and ReFS (including Dev Drives), and Windows symlinks are typed as file or directory.
  - Git Bash `ln -s` creates *copies* by default; use `mklink` (`/d` for directories).
  - Directory junctions (`mklink /j`) can be created by non-admins.
  - Many programs do not understand symlinks.

  — [Git for Windows: Symbolic Links](https://gitforwindows.org/symbolic-links)

#### What official migration tooling does
- **OpenAI's `migrate-to-codex` skill:**
  - Converts `.claude/skills/<name>/SKILL.md` into `.agents/skills/<name>/SKILL.md`, copying `scripts/`, `references/` and `assets/` when they are real files under the source skill.
  - Keeps Claude `allowed-tools` only as prompt guidance; it is not a Codex permission boundary.
  - Marks `disable-model-invocation`, `model` and `effort` as having no direct equivalent.
  - Makes `AGENTS.md` a symlink to CLAUDE.md when the content looks provider-neutral, and writes a copy when it has Claude-only semantics.
  - Maps `.mcp.json` `mcpServers` to `.codex/config.toml` `[mcp_servers.<name>]`.

  — [openai/skills migrate-to-codex differences.md](https://github.com/openai/skills/blob/main/skills/.curated/migrate-to-codex/references/differences.md)

#### Installer-style alternatives (third-party or preview)
- **`npx skills`** (Vercel Labs; third-party) installs into `.agents/skills/<name>` for Claude Code, Codex, Cursor and 75 more agents. It records hashes in `skills-lock.json` and treats a symlink as the file it points to when hashing. — [vercel-labs/skills README](https://github.com/vercel-labs/skills)
- **`gh skill`** (GitHub CLI 2.90.0+, public preview) can search, preview, install, update and publish skills from GitHub repos, with `--pin`. — [GitHub: Create skills](https://docs.github.com/en/copilot/how-tos/use-copilot-agents/coding-agent/create-skills)

#### AGENTS.md as a pointer
- **Claude Code** reads AGENTS.md only when no CLAUDE.md exists (v2.1.277+). A CLAUDE.md can import it with `@AGENTS.md`. — [Claude Code: Memory](https://code.claude.com/docs/en/memory)
- **Codex** reads AGENTS.md as its project instruction file; OpenAI's migrator targets it. — [differences.md](https://github.com/openai/skills/blob/main/skills/.curated/migrate-to-codex/references/differences.md)

### Inferences
Options ranked for a 2-developer Windows 11 team with Claude Code plus Codex or Cursor:

1. **Recommended: canonical `.claude/skills/` plus a generated, gitignored `.agents/skills/` mirror (copies), plus an AGENTS.md skills index.**
   - Claude Code, Cursor and Copilot work immediately after cloning, with no links and no Developer Mode.
   - The Codex user runs a small sync script, for example `python tools/sync_skills.py`, that copies each `.claude/skills/<name>/` to `.agents/skills/<name>/`. Optionally wire it into a git `post-checkout`/`post-merge` hook.
   - Gitignoring the mirror means Cursor and Copilot users never see the same skill listed twice. Both read `.claude/skills` *and* `.agents/skills`, and neither documents removing duplicates.
   - This matches what OpenAI's own migrator does (real-file copies).
2. **Alternative: commit the mirror** (for Codex users who won't run a script).
   - Add a CI check or pre-commit hook that fails if the mirror differs from `.claude/skills/`.
   - Cost: Cursor and Copilot will list each skill twice, and Codex shows duplicates in selectors only if it also sees another copy.
3. **Per-developer junction**, `mklink /J .agents\skills .claude\skills`, created by a setup script and gitignored.
   - Needs no admin rights and updates live.
   - Codex documents following *symlinks*, not junctions, so test it first.
   - Gitignore it so git never records it. Git for Windows may treat reparse points like symlinks; this is unverified.
4. **Committed symlinks** (`.agents/skills` → `../.claude/skills`, or per-skill links). Use these only if both developers enable Developer Mode and clone with `-c core.symlinks=true`. Otherwise checkout produces plain text files and Codex silently finds no skills. Git Bash `ln -s` also won't create a real link by default.
5. **AGENTS.md-only pointer** ("Skills live in `.claude/skills/<name>/SKILL.md`; read the matching SKILL.md before doing X").
   - Works in any AGENTS.md reader with no file plumbing.
   - Loses native catalog listing, implicit triggering and `$skill`/`/skill` invocation.
   - Best kept as a fallback layered on option 1 or 2.
6. **Reverse layout** (canonical `.agents/skills/`). This would suit Codex, Cursor, Copilot and Antigravity, but Claude Code explicitly doesn't read `.agents/`. Claude Code would then need per-skill symlinks or copies, which moves the Windows problem to the Claude Code user. Revisit if anthropics/claude-code#107421 ships.

Other recommendations:
- **Portability rules for the shared skills:**
  - Use the six spec keys only, with `name` matching the folder.
  - Use relative forward-slash paths from the skill root.
  - Don't put `${CLAUDE_SKILL_DIR}`, `!` injection or `$ARGUMENTS` in the critical path. Codex's migrator flags them for manual review, and other agents ignore them.
- **Line endings.** Add `.gitattributes` entries with `eol=lf` for `.claude/skills/**`, because of the past Windows CRLF bug.

### Gaps
- The docs don't say whether Cursor, Copilot or Antigravity follow symlinks or junctions, or how they handle two skills with the same name in different directories.
- Claude Code documents symlinks for per-skill entries; I found no statement about making the whole `.claude/skills` directory a symlink.
- I found no Codex documentation on NTFS junctions.
- I found no Anthropic-provided cross-agent sync tool. `npx skills` is third-party, and I did not test its Windows link-or-copy behavior. Nothing was installed, per the constraints.
- The Git for Windows page discusses Developer Mode for Windows 10 1703+; it has no Windows 11-specific notes.

## 4. How should a skill that drives an MCP server (Blender Lab MCP, `execute_blender_code`) reference tools, degrade when the MCP isn't connected, and handle bundled Python scripts?

### Takeaway
Refer to MCP tools by their **server-independent tool name** plus the server identity, and give the fully qualified Claude Code form (`mcp__Blender__execute_blender_code`) as a concrete example. Clients build fully qualified names from the server key that each user configures:
- Claude Code: `mcp__<server>__<tool>`
- Codex: sanitized, historically `mcp__` prefixed
- Anthropic's general guide: `ServerName:tool_name`

Official example skills degrade gracefully in three ways:
- **OpenAI's `openai-docs`:** an explicit fallback ladder (MCP, then official web, then bundled references), and it tells the user which fallback it used.
- **OpenAI's `linear`:** a "Step 0" that detects the MCP isn't connected and sets it up, plus an `agents/openai.yaml` MCP dependency that makes Codex offer to install it.
- **Anthropic's knowledge-work plugins:** tool-agnostic categories; every skill still works with nothing connected.

For Blender, keep one script per operation in `scripts/` that works two ways: run inside the live session through `execute_blender_code` (with a short loader, not by pasting large code), or headless through the Blender CLI as the no-MCP fallback.

### Cited Findings

#### How each client names MCP tools
- **Claude Code** names MCP tools `mcp__<server>__<tool>`, and any character outside `A-Z a-z 0-9 _ -` becomes `_`.
  - Plugin-bundled servers are named `mcp__plugin_<plugin>_<server>__<tool>`.
  - The full name is required in permission rules, in a skill's `allowed-tools`, in subagent `tools` and in hook matchers.
  - Tools whose full name is longer than 128 characters are left out.

  — [Claude Code: MCP](https://code.claude.com/docs/en/mcp)
- **Claude Code permission rules.** `mcp__puppeteer` and `mcp__puppeteer__*` match every tool from that server; `mcp__puppeteer__puppeteer_navigate` matches one tool. An allow glob must start with a literal `mcp__<server>__`. — [Claude Code: Permissions](https://code.claude.com/docs/en/permissions)
- **Anthropic's best-practices guide** uses `ServerName:tool_name` (for example `BigQuery:bigquery_schema`). That is a different spelling from Claude Code's callable `mcp__BigQuery__bigquery_schema`. — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices); [Claude Code: MCP](https://code.claude.com/docs/en/mcp)
- **Codex** normalizes model-visible MCP tool names so they are sanitized, unique and at most 128 bytes, hashing when necessary. It adds the historical `mcp__` namespace prefix when `prefix_mcp_tool_names` is true. — [codex-rs/codex-mcp/src/tools.rs](https://github.com/openai/codex/blob/main/codex-rs/codex-mcp/src/tools.rs)
  - OpenAI's own skill refers to Codex MCP tools as `mcp__openaiDeveloperDocs__search_openai_docs`, `mcp__openaiDeveloperDocs__fetch_openai_doc` and `mcp__openaiDeveloperDocs__get_openapi_spec`. — [openai/skills openai-docs SKILL.md](https://github.com/openai/skills/blob/main/skills/.curated/openai-docs/SKILL.md)

#### Official example skills that drive MCP servers
- **OpenAI `openai-docs`:**
  - Defines "Docs MCP" as specific fully qualified tool names.
  - Uses official-domain web search only when those tools are unavailable or unhelpful.
  - If direct fetch and MCP/search both fail, uses the bundled `references/` files and tells the user it is using a fallback.

  — [openai-docs SKILL.md](https://github.com/openai/skills/blob/main/skills/.curated/openai-docs/SKILL.md)
- **OpenAI `linear`:**
  - Has a "Prerequisites" section (the MCP must be connected) and a mandatory "Step 0".
  - Step 0: if any MCP call fails because the Linear MCP is not connected, pause and set it up (`codex mcp add linear --url https://mcp.linear.app/mcp`, then `codex mcp login linear`), then tell the user to restart.
  - Includes a Windows/WSL note.

  — [openai/skills linear SKILL.md](https://github.com/openai/skills/blob/main/skills/.curated/linear/SKILL.md)
  - Its `agents/openai.yaml` declares `dependencies.tools: [{type: "mcp", value: "linear", description, transport: "streamable_http", url}]`. — [linear agents/openai.yaml](https://github.com/openai/skills/blob/main/skills/.curated/linear/agents/openai.yaml)
- **Codex runtime behavior.** When skills the user mentions declare MCP dependencies that are not installed, Codex asks whether to install them ("The following MCP servers are required by the selected skills but are not installed yet: … Install them now?"). It respects managed policy and doesn't ask again in that session once declined. — [codex mcp_skill_dependencies.rs](https://github.com/openai/codex/blob/main/codex-rs/core/src/mcp_skill_dependencies.rs); [Codex: Build skills](https://learn.chatgpt.com/docs/build-skills)
- **Anthropic knowledge-work-plugins:**
  - Plugins are tool-agnostic. Skills refer to categories (`~~CRM`, or plain words like "the CRM") rather than products.
  - `.mcp.json` preconfigures specific servers, but any MCP server in the category works.
  - Every skill also works with nothing connected, by having the user upload an export or paste notes.

  — [anthropics/knowledge-work-plugins sales/CONNECTORS.md](https://github.com/anthropics/knowledge-work-plugins/blob/main/sales/CONNECTORS.md)
- **anthropics/skills** has no skill that drives a third-party MCP server. A code search for "mcp" there found only `mcp-builder` (for building servers), `claude-api` docs and the README. — [anthropics/skills](https://github.com/anthropics/skills)

#### What the agent sees when an MCP server is missing or slow (Claude Code)
- **Tool search** is the default. Tool definitions are deferred, and only tool *names* and server instructions load at session start. Server instructions help Claude know when to search for a server's tools, much as skills do. — [Claude Code: MCP](https://code.claude.com/docs/en/mcp)
- **Failed connections.** With tool search on, Claude Code tells Claude which server failed and the connection error, including inside `ToolSearch` results that find no match. Without tool search, failed connections are not reported to Claude. In `claude -p` or SDK runs, Claude is told when a server's tools are unavailable pending authentication. — [Claude Code: MCP](https://code.claude.com/docs/en/mcp)

#### Scripts: execute vs read
- **Anthropic.** Utility scripts run through bash without their contents entering context; only the output costs tokens. Instructions must say whether Claude should run a script or read it as reference. — [Best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices)
- **agentskills.io.** Reference scripts with relative paths from the skill root; commands run from the skill directory. Scripts should:
  - never prompt interactively
  - document themselves through `--help`
  - give helpful error messages
  - write structured output (JSON) to stdout and diagnostics to stderr
  - be idempotent, offer `--dry-run`, and use meaningful exit codes
  - keep output size predictable, because harnesses truncate at roughly 10–30K characters
  - declare dependencies inline with PEP 723 and run via `uv run`

  — [Using scripts in skills](https://agentskills.io/skill-creation/using-scripts)
- **Spec.** `compatibility` (1–500 characters) is the place to state environment requirements such as required packages or products. — [Agent Skills spec](https://agentskills.io/specification)

#### Permissions and bundling MCP config
- **`allowed-tools` differs by agent:**
  - It is experimental in the spec, and support varies. — [Agent Skills spec](https://agentskills.io/specification)
  - In Claude Code it applies to the invoking turn only, and it is ignored under `allowManagedPermissionRulesOnly` (v2.1.282). — [Claude Code: Skills](https://code.claude.com/docs/en/skills)
  - In Codex it is not a permission boundary. — [differences.md](https://github.com/openai/skills/blob/main/skills/.curated/migrate-to-codex/references/differences.md)
  - GitHub warns that pre-approving shell or bash removes confirmation and can let prompt injections run arbitrary commands. — [GitHub: Create skills](https://docs.github.com/en/copilot/how-tos/use-copilot-agents/coding-agent/create-skills)
- **Sharing MCP config per agent:**
  - Claude Code: a project `.mcp.json` (servers load behind an approval prompt), or a skill folder that is also a plugin with its own MCP servers (requires workspace trust). — [Claude Code CHANGELOG](https://github.com/anthropics/claude-code/blob/main/CHANGELOG.md); [Claude Code: Skills](https://code.claude.com/docs/en/skills)
  - Codex: `.codex/config.toml` `[mcp_servers.<name>]`. — [differences.md](https://github.com/openai/skills/blob/main/skills/.curated/migrate-to-codex/references/differences.md)

### Inferences
Recommended pattern for the Blender asset skills:

- **Tool references.** In each `SKILL.md`, add a short "Tools" block. It should:
  - name the server ("the Blender Lab MCP server, configured as `Blender`") and the bare tool names (`execute_blender_code`, `get_objects_summary`, `get_object_detail_summary`, `render_viewport_to_path`, `get_screenshot_of_window_as_image`, `search_api_docs`)
  - say that in Claude Code these appear as `mcp__Blender__<tool>`, and in other agents as some prefixed form ending in the tool name
  - tell the agent to match on the tool-name suffix

  This satisfies Anthropic's "fully qualified" advice without hard-coding one client's format. Codex already uses the same `mcp__server__tool` shape in OpenAI's own skill.
- **Use the same server key everywhere.** Use `Blender` in `.mcp.json`, `.codex/config.toml` and Cursor's MCP config. A teammate who registers `blender` gets `mcp__blender__…`, and Claude Code `allowed-tools` entries naming `mcp__Blender__…` would stop matching.
- **Preflight and a fallback ladder**, modeled on `openai-docs` and `linear`:
  1. Call a cheap read-only tool (for example `get_objects_summary`) to confirm Blender is live.
  2. If no Blender tool exists, or the call returns a connection error, tell the user to start Blender with the MCP add-on. Then offer a headless fallback: run the same script through the Blender CLI in background mode.
  3. If Blender isn't available at all, output the script and manual steps.
  4. Always say which path ran.
- **Single-source scripts.** Write `scripts/<operation>.py` so the same code supports both paths:
  - **MCP path:** send a 2–3 line loader through `execute_blender_code`, for example `import runpy; result = runpy.run_path("<abs>/scripts/make_crate.py", init_globals={...})["main"](...)`. Avoid pasting the whole file: pasted code costs tokens twice (read, then tool input), while a loader keeps context small.
  - **CLI path:** an `if __name__ == "__main__":` block parses args after `--` and prints JSON to stdout.
  - Return JSON-serializable dicts.
  - Pass **absolute** paths into Blender, because Blender's working directory is not the skill directory.
  - Keep forward slashes in `SKILL.md`.
- **Mutate with code, verify with dedicated tools.** Use `execute_blender_code` for changes and the dedicated read-only tools for checks (summaries, viewport renders, screenshots). This follows the server's own guidance, observed in-session (see Gaps), that `execute_blender_code` is a last resort.
- **Claude Code-only frontmatter.** Optionally pre-approve read-only Blender tools in `allowed-tools`, for example `mcp__Blender__get_objects_summary mcp__Blender__render_viewport_to_path`. Do **not** pre-approve `execute_blender_code`, because it runs arbitrary Python unsandboxed.
- **Codex extra.** Add `agents/openai.yaml` with a `dependencies.tools` MCP entry so Codex offers to install the Blender MCP. Whether it supports a stdio, add-on-launched server is unverified (see Gaps).
- **Layout.** Keep MCP setup, per-agent config snippets and the tool catalog in `references/blender-mcp.md`, loaded only on setup errors. Keep the gotchas at the top of `SKILL.md`.

Skeleton (illustrative, not taken from any official source):
```markdown
---
name: blender-lowpoly-prop
description: Builds game-ready low-poly props in Blender (modeling, UVs, materials, .glb export) through the Blender MCP server, or headless Blender when the MCP is not connected. Use when the user wants a 3D prop, game asset, or .glb/.fbx model made or fixed, even if they don't mention Blender.
compatibility: Needs Blender with the Blender Lab MCP add-on running, or a Blender executable on PATH for headless fallback.
---
# Low-poly prop
## Gotchas (read first)
- Inspect the scene before changing it; never delete objects without asking.
## Tools
Blender MCP server (key `Blender`): `get_objects_summary`, `execute_blender_code`, `render_viewport_to_path`.
Claude Code names: `mcp__Blender__<tool>`; other agents: any prefixed name ending in the tool name.
## Step 0: check Blender
Call `get_objects_summary`. If no Blender tool exists or it errors, read references/blender-mcp.md and use the headless path.
## Steps
1. Build: run scripts/build_prop.py (MCP loader or headless CLI; see references/blender-mcp.md)
2. Verify: render_viewport_to_path, then inspect the image
3. Export: scripts/export_glb.py; report which path (MCP or headless) ran
```

### Gaps
- **Blender Lab docs blocked.** The official pages (blender.org/lab/mcp-server and projects.blender.org/lab/blender_mcp) and docs.blender.org returned HTTP 403 to automated fetching. I did not work around it. As a result, these are **not verified from public docs**:
  - the official tool list
  - what the `*_for_cli` tool variants do
  - how `execute_blender_code` returns values (for example a `result` variable vs. print capture)
  - the recommended server key
  - the exact Blender CLI flags (`-b`, `-P`, `--`)
- **Observed only in this session (Claude Code, server key `Blender`), not publicly verified:**
  - Deferred tool names include `mcp__Blender__execute_blender_code`, `mcp__Blender__execute_blender_code_for_cli`, `mcp__Blender__get_objects_summary`, `mcp__Blender__get_object_detail_summary`, `mcp__Blender__render_viewport_to_path`, `mcp__Blender__render_thumbnail_to_path`, `mcp__Blender__get_screenshot_of_window_as_image`, `mcp__Blender__get_screenshot_of_area_as_image`, `mcp__Blender__get_python_api_docs`, `mcp__Blender__search_api_docs`, `mcp__Blender__search_manual_docs`, `mcp__Blender__jump_to_*` and `mcp__Blender__get_blendfile_summary_*` (each with a `_for_cli` twin).
  - The server's instructions say:
    - `execute_blender_code` is a last resort when other tools can do the job
    - code should return structured data (dicts or lists), not print output
    - inspect the scene first and never assume missing values
    - don't destructively modify objects without confirmation
    - the Blender Python API and the manual are bundled as RST files under `data/api/` and `data/manual/`

  A secondary listing also mentions `execute_blender_code_for_cli` and `get_blendfile_summary_*` tools. — [mcpservers.org Blender MCP listing (secondary, not fetched)](https://mcpservers.org/servers/blender-mcp)
- **Other unknowns:**
  - Cursor's, Copilot's and Antigravity's MCP tool-naming conventions were not found in their docs.
  - Whether Codex `dependencies.tools` supports stdio or local servers (the documented example is `streamable_http` with a `url`).
  - Claude Code's behavior when `allowed-tools` names an MCP tool from a server that isn't connected is undocumented. Presumably harmless, but not verified.
