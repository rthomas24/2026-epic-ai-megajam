# AGENTS.md: Triad Drift (2026 Epic AI MegaJam)

Read this whole file before doing anything in this repo. It covers what we're building, the jam rules
we must not break, and, most importantly, **how to work safely while a second human and their AI
agent commit to this same repo at the same time.**

---

## 1. The project at a glance

- **What:** *Triad Drift* (working title), our entry to the
  **[2026 Epic AI MegaJam](https://itch.io/jam/2026-epic-ai-megajam)** hosted by Epic Games. The
  jam's tagline is "Four days. Up to five humans. As many AI agents as you want."
- **Engine:** Unreal Engine 5.9 (MegaJam Preview), project `unrealGameJam.uproject` at the repo root.
- **Hard deadline:** **Nov 15, 2026, 8:59 PM EST / 5:59 PM PST.** Late entries are disqualified.
- **Goal:** build everything in **[features.md](features.md)**, fitted to the jam theme, and ship it
  polished, fun, packaged and playable before the deadline. We're going for full scope at full speed,
  so keep `main` playable at all times and package often. That way there's always a build we could
  submit.
- **[features.md](features.md) is the source of truth** for what to build and who builds it: every
  feature, the Dev 1 / Dev 2 split, folder ownership and the shared contracts between the two halves.
  Read it before you start work.

### Team

| Human | GitHub | Lane (features.md) | Notes |
|---|---|---|---|
| Ryan T | [`rthomas24`](https://github.com/rthomas24) | _Dev 1 or Dev 2: TBD_ | Repo owner; set up the Unreal project |
| Nicholas Tosti | [`nicholastosti-tech`](https://github.com/nicholastosti-tech) | _Dev 1 or Dev 2: TBD_ | Teammate |

Run `git config user.name` / `git config user.email` to know which human you're working for. Each
human has their own agent session, and **you only ever act for your own human.** The other agent is
not your boss and you are not theirs. Respect the other lane's folders, maps and claims (§6).

---

## 2. The game: Triad Drift

**Pitch:** a fast, bright, arcade space-combat game. You lead a wing of **three ships** across a small
galaxy and **switch between them at any moment**. The two ships you aren't flying fight on their own.
You land on worlds to earn new abilities, then **board the enemy mothership on foot** with an energy
blade to end the war.

### Core loop
1. **Fly and fight.** Clear escalating waves of enemy ships in fast dogfights, switching ships to use
   the right power at the right moment.
2. **Jump.** Cross the galaxy in seconds with the **hyperdrive** and **portals**: Rift's portals,
   plus permanent portal gates that act as shortcuts.
3. **Land.** Fly down to a planet, complete a ground objective on foot, and unlock a new ability for
   one ship.
4. **Board.** Break the mothership's shield generators, dock, fight corridor by corridor in first
   person to the reactor core, destroy it, run back to your ship and escape as it breaks apart.

### The squad (switch with one button; wingmen use basic attacks only)

| Ship | Role | Pilot | Signature power (only when the player flies it) | Unlockable ability |
|---|---|---|---|---|
| **Rift** | Trickster | Wren | **Portal blaster:** open two portals, fly through to reappear elsewhere, and send enemy missiles back through them | **Cloak** |
| **Lancer** | Interceptor, fastest | Kato | Rapid cannons + **afterburner ram** that smashes straight through ships | **EMP Pulse** |
| **Bastion** | Heavy | Moss | **Missile swarm** + **shield dome** that protects nearby wingmen | **Gravity Well** |

### World, enemies and finale
- **Galaxy:** one big space level with a central combat zone, two landable planets, portal gates, and
  the mothership's approach.
- **Verdance:** a bioluminescent jungle planet. Its objectives (recover a wreck's core; wake an ancient
  relay) unlock **Gravity Well** and **Cloak**.
- **Cinder:** a volcanic world with crystals. Its objectives (clear an enemy outpost; destroy a signal
  jammer) unlock **EMP Pulse** and **reveal the mothership**.
- **Enemy, the Null Armada:**
  - waves that grow in size and variety: swarming fighters, missile-firing interceptors, shielded
    gunships, mine-laying bombers, and carriers that launch more fighters
  - on foot: crew grunts, troopers with deflectable blaster bolts, shielded heavies, drones, and a
    Warden mini-boss
- **Finale, the Dominion mothership:**
  - break its 4 shield generators and dock through a tractor beam
  - fight through the interior with an **energy blade**: a melee combo, plus a block that deflects
    shots back at the shooter, with a perfect-timing window
  - destroy the reactor core in a 3-phase fight, then a 90-second escape run, an escape flight through
    the breaking ship, and victory
- **Player journey:** main menu → intro → waves 1–2 → Verdance → waves 3–4 → Cinder → wave 5 → Dominion
  assault → boarding → core → escape → victory. Aim for 20–30 minutes.

### Look and feel
- **Bright, saturated, colourful sci-fi:** in the spirit of No Man's Sky, with **original designs only**.
  Saturated nebulae, ringed planets, bold ship silhouettes with glowing engines, a sense of huge scale.
- **Combat:** arcade-fast and readable, with bright weapon trails and big, clear explosions.
- **Colour coding:** player projectiles in squad colours (Rift magenta, Lancer cyan, Bastion amber);
  enemies in red and orange with black armour and glowing white "eyes".
- **Sound:** engine roar, hyperdrive wind-up, and a soundtrack that swells as the waves escalate.
- Pilot, faction and planet names are working names. Rename freely, but update features.md when you do.

### Theme
The jam theme was revealed at the Nov 11 kickoff. **Theme: _TBD — fill in_.** Theme fit is a judging
category: once it's known, make it visible in gameplay, not just the title (features.md C-03).

### How the Unreal project maps to the game
- The project started from the **First Person C++ template**. Its first-person character, Enhanced
  Input setup and the **`Variant_Shooter`** code are the starting point for the on-foot parts (planets,
  Dominion interior). That covers weapons, projectiles, pickups, StateTree-driven NPC AI, and UI. The
  `Variant_Horror` content is template reference only.
- **Space flight, ship switching, wingmen, waves, powers, hyperdrive and portals are new systems** to
  build (the space lane in features.md).
- **Blueprint-first; C++ is allowed** (module `unrealGameJam` in `Source/`). Whoever changes C++ must
  make sure the project still compiles before pushing.
- **New content goes under `Content/TriadDrift/<Lane>/...`** (features.md §2). Leave the template
  folders as reference until replaced, and claim any template asset before editing it.
- Art sources (Blender files, AI-generated originals, exports) live in `ArtSource/<Lane>/...`.

---

## 3. Engine, project and setup

- **Engine: Unreal Engine 5.9.0, the MegaJam Preview build (CL 60351404).** Everyone must use **the
  same engine build**: opening the project in another version upgrades the assets and breaks them for
  the other person.
- **Project:** `unrealGameJam.uproject` at the repo root, a C++ project using World Partition with
  One File Per Actor (`Content/__ExternalActors__`). Two people can edit the same level as long as
  they don't touch the same actors.
- Git ignores local caches: `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`, and
  generated `*.sln` / `*.slnx`. Each machine builds its own.
- Binary files (`.uasset`, `.umap`, `.blend`, `.fbx`, images, audio) go through **Git LFS**
  (`.gitattributes`).

**One-time setup on each machine:**
1. Install the **UE 5.9 MegaJam Preview** (forum link in §9) and **Visual Studio** with the
   "Game development with C++" workload. Open the repo's `.vsconfig` in the Visual Studio Installer to
   add exactly the components this project needs.
2. Run `git lfs install`, then clone. If you cloned before installing LFS, run `git lfs pull`.
3. The `.uproject` points at the engine through Ryan's install ID. If your 5.9 install has a
   different ID, Unreal asks which engine to use and rewrites `EngineAssociation` in the `.uproject`.
   **Never commit that change.** Instead, register your install under the same ID once (adjust the
   path to your install):
   `New-ItemProperty -Path "HKCU:\Software\Epic Games\Unreal Engine\Builds" -Name "{D15CB821-4A61-18EC-84FD-0EAD60EA46EA}" -Value "C:/Program Files/Epic Games/UE_5.9" -PropertyType String -Force`
4. Generate the Visual Studio project files. Either right-click `unrealGameJam.uproject` and choose
   Generate Visual Studio project files, or run
   `"<UE_5.9>\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="<repo>\unrealGameJam.uproject" -game -rocket`.
   Then open the `.uproject`. The editor compiles the C++ module on first launch.

---

## 4. Timeline (all times as stated by the jam)

| Event | When |
|---|---|
| Jam started (building allowed) | Nov 12, 2026, 12:00 AM EST |
| Daily livestreams | 11 AM PST / 2 PM EST each jam day (twitch.tv/unrealengine, YouTube, X) |
| In-person (optional) | Digital Jungle, 972 Mission St, SF, Nov 14–15, 9 AM–7 PM PST (Eventbrite registration) |
| **SUBMISSION DEADLINE** | **Nov 15, 2026, 5:59 PM PST / 8:59 PM EST** (itch.io shows 2026-11-16 01:59 UTC) |
| Winners announced | By Jan 29, 2027 |

Plan to have a submitted build several hours early, then keep improving it. There is **no day-by-day
schedule**: both devs build at full speed in the dependency order from
[features.md §10](features.md#10-dependency-order-no-dates). Don't invent or impose daily milestones.

---

## 5. Jam rules, submission and judging

Full Official Rules: https://epicgames.box.com/s/a7xcgl07fqoieeb99sjtd3juhj4agc4z

### 5.1 Rules (don't break these)
- **Allowed:** any AI tools, unlimited agents, teams of up to five (we're two), and setting up
  tools, agents and MCP servers before the jam.
- **Third-party assets** (e.g. Fab) are allowed **only with the proper licence, and they must be
  listed in the submission.**
- **Not allowed:**
  - Building the game before Nov 12. **Declare any prompts, scripts or assets made before the jam.**
  - Submitting an **unpackaged** Unreal project. The entry must be a packaged build.
  - Other people's IP, or inappropriate content.
  - Relying only on Epic's starter templates. **Custom gameplay beyond the template is required.**
- **What this means for agents:**
  - Never use copyrighted characters, names, music, logos or likenesses from existing games, films
    or brands.
  - Log every third-party or AI-generated asset in `CREDITS.md` (create it if missing): name,
    source or tool, licence, and who added it.
  - Log notable AI tools (Claude, Cursor, Hyper3D Rodin, Tripo, Meshy, fal, PBRgen, ...) under
    "Tools used". Some partner prizes require it.
- Participants must be 18+ with an active Epic Games account.

### 5.2 Submission checklist (itch.io)
- [ ] Packaged Windows build (Windows 10/11), zipped and named **`TeamName_GameName`**, **under 1 GB**.
      Larger files go up with `butler` or the Blendo Itch Uploader.
- [ ] Optional web build (experimental; UE 5.9 MegaJam Preview, Shipping): `index.html` at the top
      level of the zip, **< 500 MB total, < 200 MB per file**, marked "playable in browser".
- [ ] Uploaded to our own itch.io project page, then submitted to the jam.
- [ ] Team name, every member's name and Epic-account email, and the game name.
- [ ] **Gameplay video link, 30–60 seconds**, with gameplay in the first five seconds.
- [ ] Dev log link (optional; **required for the Glass Box prize**).
- [ ] "Tools you used" (needed for partner prizes such as the Hyper3D Rodin cash pool and the NVIDIA
      RTX Award).
- [ ] Third-party asset list and pre-jam declarations.

Packaged builds and videos **don't go in git**. Keep them in `Builds/` (git-ignored) or upload them
directly.

### 5.3 How we're judged
A panel scores each entry 1–5 on **ambition and innovation, theme, fun factor, gameplay, visuals,
level design and game progression, and audio.** Audio and level progression are easy to neglect in a
jam, so budget time for them. Don't ship silence.

**Prizes worth targeting:**
- **Best Overall (Unreal Engine):** $7,500 / $5,000 / $2,500, plus partner prizes.
- **Best Team (2+ members):** $5,000. We qualify.
- **Glass Box (best dev log):** $2,500. Keep notes as we go (§8).
- **Render Royalty (best visuals):** $2,500.
- **NVIDIA RTX Award:** an RTX 5090 for entries using NvRTX, DLSS 4.5 or NVIDIA ACE.
- **Hyper3D Rodin:** a $10,000 cash pool shared by winners who used Rodin (list it in "Tools you
  used").
- **Development Continuation Grants:** up to $50,000 per team.

---

## 6. Working alongside the other teammate: MANDATORY GIT PROTOCOL

Two humans and two agents push to `main` at the same time, under extreme time pressure. A lost
afternoon of work or a corrupted map can sink the entry. **Follow this protocol every single time.**

### 6.1 Why Unreal makes this dangerous
- `.uasset` and `.umap` files are **binary, and git can't merge them.** If both people change the same
  asset, one person's work **will be lost**. There is no "resolve the conflict".
- Text files (C++, `.ini`, `.md`, Python) merge normally, but still need care.
- So the strategy is to **avoid touching the same binary files at the same time**, and **sync
  constantly.**

### 6.2 Before you start any task
```bash
git fetch origin
git status                      # must be clean or only contain your own known changes
git pull --rebase origin main   # get the teammate's latest work first
```
Then check `CLAIMS.md` (§6.5) to make sure the files you're about to edit aren't claimed by the
teammate. If the Unreal Editor is open, the human may need to restart it or reload assets after
pulling; tell them.

### 6.3 Before EVERY commit: poll the remote first
```bash
git fetch origin
git log --oneline HEAD..origin/main          # what has the teammate pushed since you last synced?
git diff --name-only HEAD...origin/main      # files THEY changed
git status --porcelain                       # files YOU changed
```
Compare the two file lists:
- **No overlap:** commit, then `git pull --rebase origin main`, then push immediately (§6.4).
- **Overlap on text files:** commit, then `git pull --rebase origin main`, and resolve conflicts
  carefully, keeping **both** people's intent. If the resolution isn't obvious, **stop and ask your
  human.**
- **Overlap on ANY binary file** (`.uasset`, `.umap`, `.blend`, images, audio): **STOP.** Don't commit,
  don't rebase, don't pick a side. Tell your human exactly which files overlap and who changed them.
  The humans decide whose version wins, and the other person redoes their change on top.

### 6.4 Commit and push
- Commit **small and often**: every meaningful working step, at least every ~30–60 minutes of work.
  Long-lived local changes are how conflicts happen.
- Stage only the files you intentionally changed: `git add <paths>`. **Never** run a blind
  `git add -A` / `git add .` without reviewing `git status` first. Unreal touches lots of files
  incidentally, so don't commit incidental re-saves of assets you didn't mean to change.
- Commit message format: `<area>: <what changed>`, e.g. `player: add dash ability`.
- Push right after committing:
  ```bash
  git pull --rebase origin main
  git push origin main
  ```
  If the push is rejected, the teammate just pushed: go back to §6.3 and repeat. Never "fix" it with
  force.
- Make sure the project **still opens and compiles** before pushing. Never push a broken `main`; the
  teammate will pull it immediately.

### 6.5 Claiming files (`CLAIMS.md`)
Keep a lightweight claims file at the repo root so each side knows what the other is editing. Create
it if it doesn't exist:

```
| Who      | Files / folders / assets                   | Since        |
|----------|--------------------------------------------|--------------|
| Ryan     | Content/TriadDrift/Space/Maps/L_Galaxy     | Nov 13 10:00 |
| Nicholas | Content/TriadDrift/Ground/Enemies/*        | Nov 13 10:15 |
```

- Claim binary assets **before** editing them, especially maps and shared Blueprints like the
  GameMode, PlayerController, PlayerCharacter and GameInstance. Commit and push the claim right away
  so the other agent sees it.
- Remove your claim as soon as your change is pushed.
- If something you need is claimed by the teammate, **don't edit it.** Tell your human so the two of
  them can coordinate.
- `git lfs lock <file>` / `git lfs unlock <file>` is available if you want a hard lock on a hot file.

### 6.6 Structure the project to avoid collisions
- **Folder ownership:** each dev works only in their own lane's folders and maps
  ([features.md §2](features.md#2-folder--file-ownership)). Never edit the other dev's folders or maps.
- **Maps:** World Partition One File Per Actor is on, so two people can edit the same level without
  touching the same file. You still need to claim the map's World Settings and shared actors.
- **Shared Blueprints** (GameMode, PlayerController, PlayerCharacter, GameInstance) are hot spots.
  Always claim them first, and prefer adding new components or child Blueprints over editing the
  shared one.
- Prefer C++ or other text-based logic where it makes sense: text merges, Blueprints don't.

### 6.7 Hard rules: never do these
- ❌ `git push --force` / `--force-with-lease` on `main`
- ❌ `git reset --hard`, `git checkout -- <file>`, `git restore` or `git clean -fd` on anything that
  might contain the teammate's or your human's uncommitted work, without your human's explicit OK
- ❌ Rewriting pushed history (`rebase -i`, `commit --amend` on pushed commits)
- ❌ Resolving a binary conflict by choosing "ours" or "theirs" on your own
- ❌ Deleting, renaming or moving assets or folders the teammate owns. Unreal redirectors break across
  people, so ask first, and fix up redirectors after any approved move.
- ❌ Committing `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`, `Builds/` or
  packaged builds
- ❌ Committing a changed `EngineAssociation` in `unrealGameJam.uproject` (see §3)
- ❌ Committing secrets, API keys or `.env` files

### 6.8 Branches
By default everyone works on `main` with the protocol above, which is fastest in a 4-day jam. For a
risky experiment, use a short-lived branch `<name>/<feature>` and merge it back within a few hours.
Don't let branches drift.

---

## 7. Repo status and remaining setup

**Done (Nov 12):**
- The Unreal project is at the repo root.
- `.gitignore` and Git LFS are set up. Files aren't marked `lockable`: we coordinate through
  `CLAIMS.md` instead.
- The Blender asset skills are in place (§10).

**Still to do** (features.md C-01, C-02 and C-03):
- the `Content/TriadDrift/...` folder tree
- collision channels and the maps
- the shared contracts (C-02)
- `CLAIMS.md`, `CREDITS.md` and `DEVLOG.md`
- filling in the theme and each human's lane above

---

## 8. Dev log (for the Glass Box prize and the submission)

Append notable moments to `DEVLOG.md` as you go (create it if missing): decisions, AI prompts and
workflows that worked or failed, paths to before/after screenshots, problems solved, and timestamps.
Keep entries short, and only ever add them at the bottom to avoid merge conflicts with the teammate.

---

## 9. Useful links

- Jam page: https://itch.io/jam/2026-epic-ai-megajam
- Official Rules: https://epicgames.box.com/s/a7xcgl07fqoieeb99sjtd3juhj4agc4z
- UE 5.9 MegaJam Preview: https://forums.unrealengine.com/t/unreal-engine-5-9-megajam-preview/2852840
- Unreal MCP in the editor: https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor?lang=en-US
- Packaging: https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-your-project
- Fab assets: https://www.fab.com/
- NvRTX: https://developer.nvidia.com/game-engines/unreal-engine
- butler (large itch uploads): https://itch.io/docs/butler/
- Community: UE forums https://epic.gm/2026-ue-megajam

---

## 10. Agent skills: Blender and 3D assets

Reusable, tested skills live in `.claude/skills/`. Claude Code, Cursor and GitHub Copilot discover
them automatically. **Codex and other agents:** before doing a matching task, open and follow the
`SKILL.md` at the path below. The research behind them is in
`reports/Blender MCP game asset practices.md`.

| Skill | Path | Use it when |
|---|---|---|
| `blender-mcp` | `.claude/skills/blender-mcp/SKILL.md` | Anything in Blender: MCP connection, bpy code on 5.2, headless runs, and verification (review renders, mesh report, floating-part check). **Read first.** |
| `blender-game-asset` | `.claude/skills/blender-game-asset/SKILL.md` | Making any 3D asset: routing (AI image-to-3D vs kitbash vs code), the AAA rubric, approval gates, and kits for ships, props, the corridor kit, rocks and crystals |
| `blender-texturing` | `.claude/skills/blender-texturing/SKILL.md` | Materials, UVs, and baking to Unreal texture sets |
| `blender-ai-mesh-cleanup` | `.claude/skills/blender-ai-mesh-cleanup/SKILL.md` | Rodin, Tripo or Meshy outputs and downloaded models → game-ready |
| `blender-unreal-export` | `.claude/skills/blender-unreal-export/SKILL.md` | Export to Unreal (FBX + manifest), calibration, and the UE import script |

Check that a machine's Blender works with the skills:
```
"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python .claude/skills/blender-mcp/scripts/selftest.py
```

---

## 11. Agent working style for this jam

- Full scope: build everything in features.md. Don't cut or shrink a feature on your own. If
  something looks at risk, tell your human and let them decide.
- Keep the game **always playable** on `main`.
- Verify changes in the editor (Play In Editor) before committing, and report honestly what was and
  wasn't tested.
- When you're unsure whether something conflicts with the teammate's work, **ask your human. Don't
  guess.**
