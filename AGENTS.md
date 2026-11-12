# AGENTS.md — 2026 Epic AI MegaJam

Read this whole file before doing anything in this repo. It tells you what we are building, the jam rules
we must not break, and — most importantly — **how to work safely alongside a second human and their AI
agent who are committing to this same repo at the same time.**

---

## 1. What this project is

We are a **team of two humans** (plus our AI agents) competing in the
**[2026 Epic AI MegaJam](https://itch.io/jam/2026-epic-ai-megajam)**, hosted by Epic Games
(itch.io host: `epicgamesjams`). Tagline: *"Vibe your way to $50k."*

> Four days. Up to five humans. As many AI agents as you want. Build a playable game in
> Unreal Engine or Unreal Editor for Fortnite (UEFN) and compete for cash prizes.

Our goal: build **everything in [features.md](features.md)**, fitted to the jam theme, and ship it **polished,
fun, packaged and playable before the deadline**. We're going for full scope at full speed, so keep `main`
playable at all times and package often. That way there's always a build we could submit.

**[features.md](features.md) is the source of truth** for what to build and who builds it: every feature, the
Dev 1 / Dev 2 split, folder ownership, and the shared contracts between the two halves. Read it before you start work.

### Team

| Role | Name | GitHub | Notes |
|------|------|--------|-------|
| Human A | Ryan T | `rthomas24` | Repo owner |
| Human B | _TBD — fill in_ | _TBD_ | Teammate |

Run `git config user.name` to know which human you are working for. Each human has their own agent session;
**you only ever act for your own human.** The other agent is not your boss and you are not theirs.

### Engine and project

- **Engine: Unreal Engine 5.9.0, the MegaJam Preview build (CL 60351404).** We are not using UEFN.
- **Project:** `unrealGameJam.uproject` at the repo root.
  - Built from the First Person template as a **C++** project: module `unrealGameJam` in `Source/`,
    with the template's Horror and Shooter variants.
  - Maps use World Partition with One File Per Actor (`Content/__ExternalActors__`), so two people
    can edit the same level as long as they don't touch the same actors.
- **Everyone must use the same engine build.** Opening the project in another version upgrades the
  assets and breaks them for the other person.
- Git ignores local caches: `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/` and
  generated `*.sln` files. Each machine builds its own.

**Teammate setup (once per machine):**
1. Install the **UE 5.9 MegaJam Preview** (forum link in §9) and **Visual Studio** with the
   "Game development with C++" workload. Open the repo's `.vsconfig` in the Visual Studio Installer
   to add exactly the components this project needs.
2. Run `git lfs install`, then clone. If you cloned before installing LFS, run `git lfs pull`.
3. The `.uproject` points at the engine through Ryan's install ID. If your 5.9 install has a
   different ID, Unreal asks which engine to use and rewrites `EngineAssociation` in the `.uproject`.
   **Never commit that change.** Instead, register your install under the same ID once, adjusting
   the path to your install:
   `New-ItemProperty -Path "HKCU:\Software\Epic Games\Unreal Engine\Builds" -Name "{D15CB821-4A61-18EC-84FD-0EAD60EA46EA}" -Value "C:/Program Files/Epic Games/UE_5.9" -PropertyType String -Force`
4. Generate Visual Studio project files. Either right-click `unrealGameJam.uproject` and pick
   Generate Visual Studio project files, or run
   `"<UE_5.9>\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="<repo>\unrealGameJam.uproject" -game -rocket`.
   Then open the `.uproject`; the editor compiles the C++ module on first launch.

### Theme

The theme was revealed at the kickoff livestream (Nov 11, 11 AM PST). **Theme: _TBD — fill in_.**
Theme fit is an explicit judging category — keep it visible in the game, not just the title.

---

## 2. Timeline (all times as stated by the jam)

| Event | When |
|-------|------|
| Kickoff livestream + theme reveal | Nov 11, 2026 — 11 AM PST / 2 PM EST |
| Jam starts (building allowed) | **Nov 12, 2026 — 12:00 AM EST** |
| Daily livestreams | 11 AM PST / 2 PM EST each jam day (twitch.tv/unrealengine, YouTube, X) |
| In-person (optional) | Digital Jungle, 972 Mission St, SF — Nov 14–15, 9 AM–7 PM PST (Eventbrite registration) |
| **SUBMISSION DEADLINE** | **Nov 15, 2026 — 5:59 PM PST / 8:59 PM EST** (itch.io shows 2026-11-16 01:59 UTC) |
| Winners announced | By Jan 29, 2027 |

**Late submissions are disqualified.** Plan to have a submitted build several hours early, then iterate.

There is **no day-by-day schedule.** Both devs build at full speed, in the dependency order from
[features.md §10](features.md#10-dependency-order-no-dates). Don't invent or impose daily milestones.

---

## 3. Jam rules (do not break these)

Full Official Rules: https://epicgames.box.com/s/a7xcgl07fqoieeb99sjtd3juhj4agc4z

**Allowed**
- Any AI tools: coding agents, asset generation, audio, writing, anything. Unlimited agents.
- Solo or team of up to five humans (we are two).
- Third-party assets (e.g. Fab) **only with the proper license — and they must be listed in the submission.**
- Setting up tools/agents/MCP before the jam.

**Not allowed**
- Building the game before Nov 12. **Declare any prompts, scripts, or assets made before the jam.**
- Entering with both UE and UEFN.
- Submitting an **unpackaged** Unreal Engine project (UE entries must be packaged builds).
- Content that breaks the Official Rules — no other people's IP, no inappropriate content.
- Relying only on Epic's starter templates — **custom gameplay that goes beyond the templates is required.**

**Agent obligations from these rules**
- Never pull in copyrighted characters, names, music, logos or likenesses from existing games/films/brands.
- Whenever you add a third-party or AI-generated asset, record it in `CREDITS.md` (create it if missing):
  asset name, source/tool, license, who added it. This feeds the submission's asset/tool list.
- Whenever you use a notable AI tool (Claude, Cursor, Rodin/Hyper3D, Tripo, Meshy, fal, PBRgen, etc.),
  note it in `CREDITS.md` under "Tools used" — some partner prizes require listing the tool.
- Eligibility: participants must be 18+ with an active Epic Games account.

---

## 4. Submission requirements (checklist)

**UE entries (submitted on itch.io)**
- [ ] Packaged build, zipped and named **`TeamName_GameName`**.
- [ ] Desktop/mobile: Windows 10/11, macOS, Android, or iOS (development build). Zip **under 1 GB**
      (larger → upload with `butler` or Blendo Itch Uploader).
- [ ] Web (experimental, UE 5.9 MegaJam Preview, Shipping build): `index.html` at top level of zip,
      **< 500 MB total, < 200 MB per file**, marked "playable in browser" on itch.io.
- [ ] Uploaded to our own itch.io project page, then submitted to the jam.

**UEFN entries**
- [ ] A **private version code** of the final build (fixed snapshot, can't be changed, must be created before
      the deadline). Live island codes and playtest codes are not accepted. Island name required.
      Follow the Fortnite Developer Rules.

**Both**
- [ ] Team name, every member's name + Epic-account email.
- [ ] Game name.
- [ ] **Gameplay video link, 30–60 seconds.** Show gameplay in the first five seconds.
- [ ] Dev log link (optional — **required for the Glass Box prize**).
- [ ] "Tools you used" (optional — required for partner prizes like the Hyper3D Rodin cash pool and NVIDIA RTX Award).
- [ ] Third-party asset list + pre-jam declarations.

Packaged builds and videos **do not go in git** — keep them in `Builds/` (git-ignored) or upload directly.

---

## 5. How we are judged

A panel scores each entry **1–5** on:
1. Ambition and innovation
2. Theme
3. Fun factor
4. Gameplay
5. Visuals
6. Level design and game progression
7. Audio

Audio and level progression are easy to neglect in a jam — budget time for them. Don't ship silence.

### Prizes worth targeting as a 2-person team
- **Best Overall** (UE or UEFN track): $7,500 / $5,000 / $2,500 + partner prizes.
- **Best Team** (2+ members): $5,000 — we qualify.
- **Glass Box** (best dev log): $2,500 — keep notes as we go (see §8).
- **Render Royalty** (best visuals): $2,500. **NVIDIA RTX Award** (RTX 5090) for UE entries using NvRTX, DLSS 4.5, or NVIDIA ACE.
- **Pocket Powerhouse** (best mobile): $2,500.
- **Walkie-Talkie** (UEFN narrative game with LLM-powered NPC conversations): $2,500.
- **Hyper3D Rodin** $10,000 cash pool split among winners who used Rodin (list it in "Tools you used").
- **Development Continuation Grants**: up to $50,000 per team.
- Best Student Project exists if any member is enrolled in higher education.

---

## 6. ⚠️ Working alongside the other teammate — MANDATORY GIT PROTOCOL

Two humans and two agents push to `main` at the same time, under extreme time pressure. A lost afternoon of
work or a corrupted map can sink the entry. **Follow this protocol every single time. No exceptions.**

### 6.1 Why Unreal makes this dangerous
- `.uasset` and `.umap` files are **binary. Git cannot merge them.** If both people change the same asset,
  one person's work **will be lost** — there is no "resolve the conflict".
- Text files (C++, `.ini`, `.md`, Python, Verse) merge normally, but still need care.
- So the strategy is: **avoid touching the same binary files at the same time**, and **sync constantly.**

### 6.2 Before you start any task
```bash
git fetch origin
git status                      # must be clean or only contain your own known changes
git pull --rebase origin main   # get the teammate's latest work first
```
Then check `CLAIMS.md` (see 6.5) to make sure the files you're about to edit aren't claimed by the teammate.
If the Unreal Editor is open, the human may need to restart it / reload assets after pulling — tell them.

### 6.3 Before EVERY commit — poll the remote first
You must check for incoming changes and overlaps **before** committing, not after:

```bash
git fetch origin
git log --oneline HEAD..origin/main          # what has the teammate pushed since you last synced?
git diff --name-only HEAD...origin/main      # files THEY changed
git status --porcelain                       # files YOU changed
```

Compare the two file lists:
- **No overlap** → commit, then `git pull --rebase origin main`, then push immediately (6.4).
- **Overlap on text files** → commit, `git pull --rebase origin main`, resolve conflicts carefully,
  preserving **both** people's intent. If the resolution is not obvious, **stop and ask your human.**
- **Overlap on ANY binary file (`.uasset`, `.umap`, images, audio, etc.)** → **STOP. Do not commit, do
  not rebase, do not pick a side.** Tell your human exactly which files overlap and who changed them.
  The humans decide whose version wins and the other person redoes their change on top.

### 6.4 Commit and push
- Commit **small and often** (every meaningful working step, at least every ~30–60 min of work).
  Long-lived local changes are how conflicts happen.
- Only stage files you intentionally changed: `git add <paths>` — **never** blind `git add -A` / `git add .`
  without reviewing `git status` first (Unreal touches lots of files incidentally; don't commit
  incidental re-saves of assets you didn't mean to change).
- Commit message format: `<area>: <what changed>` e.g. `player: add dash ability`, `level1: block out arena`.
- Push right after committing:
  ```bash
  git pull --rebase origin main
  git push origin main
  ```
  If the push is rejected, the teammate just pushed — go back to 6.3 and repeat. Never "fix" it with force.
- Make sure the project **still opens and compiles** before pushing. Never push a broken `main` —
  the teammate will pull it immediately.

### 6.5 Claiming files (`CLAIMS.md`)
Keep a lightweight claims file at the repo root so each side knows what the other is editing.
Create it if it doesn't exist. Format:

```
| Who   | Files / folders / assets              | Since        |
|-------|---------------------------------------|--------------|
| Ryan  | Content/Maps/L_Level1.umap            | Nov 13 10:00 |
| Sam   | Content/Characters/Enemy/*            | Nov 13 10:15 |
```

- Claim binary assets (especially maps and shared Blueprints like the GameMode/PlayerCharacter) **before**
  editing them; commit + push the claim right away so the other agent sees it.
- Remove your claim as soon as your change is pushed.
- If something you need is claimed by the teammate, **don't edit it** — tell your human so they can coordinate.
- Optionally, once Git LFS is set up, `git lfs lock <file>` / `git lfs unlock <file>` can enforce this.

### 6.6 Structure the project to avoid collisions
- **Folder ownership:** each dev works only in their own lane's folders and maps, as defined in
  [features.md §2](features.md#2-folder--file-ownership). Never edit the other dev's folders or maps.
- **Maps:** enable World Partition **One File Per Actor** or split levels into sublevels, so two people
  can edit the same level without touching the same file.
- **Shared Blueprints** (GameMode, PlayerController, PlayerCharacter, GameInstance) are hot spots —
  always claim them first; prefer adding new components/child BPs over editing the shared one.
- Prefer C++/Verse/text-based logic where sensible — it merges; Blueprints don't.

### 6.7 Hard rules — never do these
- ❌ `git push --force` / `--force-with-lease` on `main`
- ❌ `git reset --hard`, `git checkout -- <file>`, `git restore`, `git clean -fd` on anything that might
     contain the teammate's or your human's uncommitted work, without your human's explicit OK
- ❌ Rewriting pushed history (`rebase -i`, `commit --amend` on pushed commits)
- ❌ Resolving a binary conflict by choosing "ours"/"theirs" on your own
- ❌ Deleting, renaming, or moving assets/folders the teammate owns (Unreal redirectors break across people) —
     ask first, and fix up redirectors after any approved move
- ❌ Committing `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`, `Builds/`, or packaged builds
- ❌ Committing secrets, API keys, or `.env` files

### 6.8 Branches
Default is everyone on `main` with the protocol above (fastest in a 4-day jam). For a risky experiment,
use a short-lived branch `<name>/<feature>` and merge it back within a few hours. Don't let branches drift.

---

## 7. Repo setup still to do (first person to set up the UE project)

Only **one** person creates the Unreal project; the other clones it. Before the first asset commit:
1. Add a standard Unreal `.gitignore` (ignore `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`,
   `.vs/`, `*.sln`, `Builds/`, Blender backups `*.blend1`, `__pycache__/`, etc.).
2. Set up **Git LFS** (`git lfs install`) with a `.gitattributes` tracking `*.uasset`, `*.umap`, Blender
   sources `*.blend`, and large media (`*.png`, `*.jpg`, `*.tga`, `*.psd`, `*.wav`, `*.mp3`, `*.ogg`, `*.fbx`,
   `*.obj`, `*.glb`, `*.exr`, `*.hdr`)
   — add `lockable` if we want `git lfs lock`. Commit `.gitattributes` **before** any binary asset.
   Watch the GitHub LFS storage/bandwidth quota; don't commit huge raw source files unnecessarily.
3. Create `CLAIMS.md` and `CREDITS.md`.
4. Record the exact engine version in this file and in the README.

---

## 8. Dev log (for the Glass Box prize and the submission)

Append notable moments to `DEVLOG.md` as you go (create it if missing): decisions, AI prompts/workflows that
worked or failed, before/after screenshots paths, problems solved, time stamps. Keep entries short.
Write in append-only fashion at the bottom to avoid merge conflicts with the teammate.

---

## 9. Useful links

- Jam page: https://itch.io/jam/2026-epic-ai-megajam
- Official Rules: https://epicgames.box.com/s/a7xcgl07fqoieeb99sjtd3juhj4agc4z
- UE download: https://www.unrealengine.com/download — UE 5.9 MegaJam Preview:
  https://forums.unrealengine.com/t/unreal-engine-5-9-megajam-preview/2852840
- UEFN: https://store.epicgames.com/p/fortnite--uefn
- Unreal MCP docs: https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor?lang=en-US
- UEFN MCP docs: https://dev.epicgames.com/documentation/fortnite/uefn-mcp?lang=en-US
- Packaging: https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-your-project
- Fab assets: https://www.fab.com/
- NvRTX: https://developer.nvidia.com/game-engines/unreal-engine
- Fortnite Developer Rules: https://legal.epicgames.com/fortnite/developer-rules
- butler (large itch uploads): https://itch.io/docs/butler/
- Community: UE forums https://epic.gm/2026-ue-megajam · UEFN Discord channel (linked from the jam page)

## 10. Agent skills: Blender and 3D assets

Reusable, tested skills live in `.claude/skills/`. Claude Code, Cursor and GitHub Copilot discover
them automatically. **Codex and other agents:** before doing a matching task, open and follow the
`SKILL.md` at the path below. The research behind them is in
`reports/Blender MCP game asset practices.md`.

| Skill | Path | Use it when |
|---|---|---|
| `blender-mcp` | `.claude/skills/blender-mcp/SKILL.md` | Anything in Blender: MCP connection, bpy code on 5.2, headless runs, verification (review renders, mesh report, floating-part check). **Read first.** |
| `blender-game-asset` | `.claude/skills/blender-game-asset/SKILL.md` | Making any 3D asset: routing (AI image-to-3D vs kitbash vs code), AAA rubric, gates, kits for ships, props, corridor kit, rocks, crystals |
| `blender-texturing` | `.claude/skills/blender-texturing/SKILL.md` | Materials, UVs, baking to Unreal texture sets |
| `blender-ai-mesh-cleanup` | `.claude/skills/blender-ai-mesh-cleanup/SKILL.md` | Rodin, Tripo or Meshy outputs and downloaded models → game-ready |
| `blender-unreal-export` | `.claude/skills/blender-unreal-export/SKILL.md` | Export to Unreal (FBX + manifest), calibration, UE import script |

Art sources live in `ArtSource/<Lane>/...`, with lane ownership in features.md §2. Verify a machine
with:
```
"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python .claude/skills/blender-mcp/scripts/selftest.py
```

## 11. Agent working style for this jam

- Full scope: build everything in features.md. Don't cut or shrink a feature on your own. If something looks
  at risk, tell your human and let them decide.
- Keep the game **always playable** on `main`.
- Verify changes in-editor (PIE) before committing; report honestly what was and wasn't tested.
- When unsure whether something conflicts with the teammate's work, **ask your human — don't guess.**
