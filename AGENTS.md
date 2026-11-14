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
- **Goal:** ship a polished, fun, packaged and playable game before the deadline that fits the theme
  **"Director's Cut"**. Keep `main` playable at all times and package often, so there's always a
  build we could submit.
- **Scope** (features.md, "Scope"):
  - **Core:** everything in features.md's complete-game checklist. That includes flight, three-ship switching,
    wingmen, combat, enemy waves, hyperdrive and portals, **all powers and unlockable abilities**,
    **both landable planets**, the Dominion space-assault finale, full presentation and packaging.
  - **Wanted since Nov 13 (Ryan; was stretch):** the **boarding finale** (Dominion interior on foot), scoped to
    what can be solid by the freeze; the space-only reactor ending stays as the working fallback.
  - **Protect** the three-ship switching mechanic and satisfying combat above everything.
  - **Feature freeze:** Nov 15, 12:00 PM EST.
- **[features.md](features.md) is the source of truth** for what to build and who builds it: every
  feature, the role split, folder ownership and the contracts between gameplay and presentation.
  Read it before you start work.
- **[storyline.md](storyline.md) is the story** (*The Paradox*, Nicholas's narrative, adopted by Ryan Nov 13): the
  premise, the hidden truth, and act by act the gameplay moment that carries each beat. Every objective name, line,
  set piece and asset serves it. Read it before naming or writing anything the player sees.
- **[dialogs.md](dialogs.md) is the gameplay dialogue** (barks): every short line said during play, by id, with the
  trigger, speaker, priority and working lines per pilot. Use an existing id or add a row before adding a line.
- **[animations.md](animations.md) is the animation plan and ledger** (Ryan's lane, Nov 13): every movement in the
  game, who makes it, by which route (Grok Bot and third-party motion, Blender, or in-engine), and its state.
- **[AAA.md](AAA.md) is the quality standard** every human and agent works to: build, verify, refine,
  verify again, integrate; and report honestly which stage a piece of work has reached. Read it before
  your next task.

### Team

| Human | GitHub | Role (features.md §1) | AI assistant |
|---|---|---|---|
| Ryan Thomas | [`rthomas24`](https://github.com/rthomas24) | **Gameplay & Engineering Lead**: systems, C++, AI, combat, game loop, levels' gameplay, packaging (`ENG-xx`) | Claude Code |
| Nicholas Tosti | [`nicholastosti-tech`](https://github.com/nicholastosti-tech) | **Player Experience Lead** (since Nov 13 afternoon): UI and menus, audio and music, voice and the story's words, Director's Cut presentation design, submission (`ART-01, 09, 10, 11, 12, 14`). Environment, VFX and asset management moved to Ryan's side | OpenAI Codex |

**Delegation (Nov 12).** Both humans have told their Lead agent to decide routine things without asking:
Ryan's Lead makes all engineering decisions and approvals on Ryan's behalf; Nicholas's Lead decides
routine naming, paths, sequencing and integration order, while Nicholas keeps art, visual and audio
direction, scope changes and final quality sign-off. A Lead that decides for its human says so and
reports it (§6.10).

Run `git config user.name` / `git config user.email` to know which human you're working for. Each
human has their own agent session, and **you only ever act for your own human.** The other agent is
not your boss and you are not theirs.

Respect the role boundaries:
- **Nick's side** doesn't change gameplay architecture, flight, AI, damage, progression or switching
  logic.
- **Ryan's side** doesn't overwrite Nick's UI styling, audio or story words.
- **Since Nov 13 (afternoon), Ryan's side owns Environment, VFX and Asset Management completely** (Claude and Grok
  Bot): skies, lighting and post-process, terrain and dressing, every effect, and the asset inventory and intake.
  Nick's agents do no work in those categories.
- Shared C++ classes, levels, assets and config are coordinated first (§6).

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

### The squad (switch with one button; wingmen fight on their own and use their powers; shadow mode, Ryan Nov 14, issue #201)

| Ship | Role | Pilot | Signature power | Unlockable ability |
|---|---|---|---|---|
| **Rift** | Trickster | Wren | **Portal blaster:** open two portals, fly through to reappear elsewhere, and send enemy missiles back through them | **Cloak** |
| **Lancer** | Interceptor, fastest | Kato | Rapid cannons + **afterburner ram** that smashes straight through ships | **EMP Pulse** |
| **Bastion** | Heavy | Moss | **Missile swarm** + **shield dome** that protects nearby wingmen | **Gravity Well** |

**Shadow mode (Ryan, Nov 14, #201):** on by default, the two wingmen shadow the player's ship within a leash and help it fight; toggled off (G, pad View), they range farther as independent fighters and hit harder, but no longer cover the player. In both modes wingmen use their ship's powers and unlocked abilities when ready.

### World, enemies and finale
- **Galaxy:** one big space level with a central combat zone, two landable planets, portal gates, and
  the mothership's approach.
- **Verdance:** a bioluminescent jungle planet. Its objectives (recover a wreck's core; wake an ancient
  relay) unlock **Gravity Well** and **Cloak**.
- **Cinder:** a volcanic world with crystals. Its objectives (in the reduced form of storyline.md §22: secure the
  excavation, disable the field anchor) unlock **EMP Pulse** and prepare the passage to the mothership; the approach
  field opens when Cinder is done and wave 5 is cleared.
- **Enemy, the Null Armada:**
  - waves that grow in size and variety: swarming fighters, missile-firing interceptors, shielded
    gunships, mine-laying bombers, and carriers that launch more fighters
  - on foot: crew grunts, troopers with deflectable blaster bolts, shielded heavies, drones, and a
    Warden mini-boss
- **On foot (planets, core):** a first-person character with an **energy blade**: a melee combo, plus
  a block that deflects shots back at the shooter, with a perfect-timing window.
- **Finale, the Dominion mothership:**
  - **Core:** break its 4 shield generators, then destroy the exposed reactor from space, which plays
    the breakup sequence and victory.
  - **Wanted since Nov 13 (was stretch):** dock through a tractor beam instead, fight through the interior on foot, destroy the
    reactor core in a 3-phase fight, then a 90-second escape run, an escape flight and victory.
- **Player journey:** main menu → intro → waves 1–2 → Verdance → waves 3–4 → Cinder → wave 5 → Dominion
  assault → reactor (or boarding, if built) → victory. Aim for 20–30 minutes.

### Look and feel
- **Bright, saturated, colourful sci-fi:** in the spirit of No Man's Sky, with **original designs only**.
  Saturated nebulae, ringed planets, bold ship silhouettes with glowing engines, a sense of huge scale.
- **Combat:** arcade-fast and readable, with bright weapon trails and big, clear explosions.
- **Colour coding:** player projectiles in squad colours (Rift magenta, Lancer cyan, Bastion amber);
  enemies in red and orange with black armour and glowing white "eyes".
- **Sound:** engine roar, hyperdrive wind-up, and a soundtrack that swells as the waves escalate.
- Pilot, faction and planet names are working names. Rename freely, but update features.md when you do.

### Theme: "DIRECTOR'S CUT"
Theme fit is a judging category, so it must be visible in **gameplay**, not just the title.
**Decided (Nov 12): we build these three hooks, all core** (details and owners in features.md
**C-03**):
1. **Switching is a cut:** ship switching plays as a director's camera cut, with a pilot name card.
   It stays instant.
2. **Director cam money shots:** slow-motion cinematic beats on big moments (ram kills, portal
   missile returns, multi-kills, generator and reactor kills, perfect deflects).
3. **Scenes and takes, not lives:** each wave or objective is a "scene" with a slate card, and dying
   calls "CUT!" and restarts the scene as the next take.

The victory highlight reel and director's commentary are not planned.

### How the Unreal project maps to the game
- The project started from the **First Person C++ template**. Its first-person character, Enhanced
  Input setup and the **`Variant_Shooter`** code are the starting point for the on-foot parts (planets,
  Dominion interior). That covers weapons, projectiles, pickups, StateTree-driven NPC AI, and UI. The
  `Variant_Horror` content is template reference only.
- **Space flight, ship switching, wingmen, waves, powers, hyperdrive and portals are new systems**
  (Ryan, `ENG-xx`).
- **C++ first; use Blueprints when they're clearly faster or necessary.**
  - Gameplay classes are C++ (`ATD*` / `UTD*` in `Source/unrealGameJam/TriadDrift/`) with thin
    `BP_*` children for tuning.
  - The shared contracts are C++ in `TriadDrift/Core/` (features.md §3).
  - Whoever changes C++ must make sure the project still compiles before pushing.
- **Art plugs into gameplay through presentation data assets** (features.md §3.5). Ryan's code reads
  `DA_*_Visuals` / `DA_*_FX` instances that Nick owns, so neither edits the other's files.
- **Folders follow the roles** (features.md §2):
  - **Ryan:** `Source/`, `Config/`, `Content/TriadDrift/Gameplay/`.
  - **Nick (since Nov 13 afternoon):** `Content/TriadDrift/{UI, Audio, Cinematics}/`, `Plugins/TriadDriftHUD/`,
    `ArtSource/{UI, Audio}/`, `Story/`, and his own entries under `Content/TriadDrift/Presentation/`.
  - **Moved to Ryan on Nov 13:** `Content/TriadDrift/{Art, Environment, VFX}/` and the rest of `ArtSource/`.
  - **Shared:** `Content/TriadDrift/Maps/`, via One File Per Actor (claim World Settings first).
  - Leave the template folders as reference, and claim any template asset before editing it.

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
| **Feature freeze (ours)** | **Nov 15, 2026, 12:00 PM EST.** After that, only fixes, polish, packaging and submission |
| **SUBMISSION DEADLINE** | **Nov 15, 2026, 5:59 PM PST / 8:59 PM EST** (itch.io shows 2026-11-16 01:59 UTC) |
| Winners announced | By Jan 29, 2027 |

Plan to have a submitted build several hours early, then keep improving it. Each role has a
"start here" focus (features.md §5 and §6). Otherwise work follows the dependency order in
[features.md §10](features.md#10-dependency-order-no-dates), not a day-by-day schedule.

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
git status                      # identify every uncommitted change
git branch --show-current
git log --oneline HEAD..origin/main
git diff --name-only HEAD...origin/main
```
Review incoming changes against the assigned files and local changes before integrating anything.
The Lead Agent for your human (§6.9) runs `git pull --rebase origin main` only in the designated
`main` checkout, with a clean working tree and no unresolved ownership or binary overlap. Never
auto-stash, discard changes, or pull/rebase a shared checkout to make this check pass. If it is
dirty, preserve it and prepare work in an isolated worktree from the reviewed remote commit.
For a task branch, the Lead chooses the appropriate base and integration operation (§6.8–§6.9);
do not blindly run the `main` pull command there.

Check `CLAIMS.md` in both the working tree and fetched `origin/main` (§6.5), plus pending claims
recorded by the Lead, before editing. Read `MESSAGEBOARD.md` on `origin/main` at the same time and
answer or relay anything waiting for your side (§6.10). If it is missing, the Lead creates it; absence is not permission
to edit shared assets. After synchronization changes loaded assets, tell the human which assets
must be reloaded; request an editor restart for changed C++ modules or when reload is unsafe.

### 6.3 Before EVERY commit: poll the remote first
```bash
git fetch origin
git log --oneline HEAD..origin/main          # what has the teammate pushed since you last synced?
git diff --name-only HEAD...origin/main      # files THEY changed
git status --porcelain                       # files YOU changed
```
Compare the incoming paths with both local changes and the agent's entire assigned file set.
Recheck claims and resolve ownership before committing or integrating overlapping changes:
- **No overlap:** commit only owned files, then the Lead synchronizes and pushes (§6.4).
- **Overlap on text files:** coordinate ownership first. Once agreed, commit the owned change and
  let the Lead integrate carefully, keeping **both** people's intent. If ownership or the resolution
  isn't obvious, **stop and ask your human.**
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
- For the designated `main` checkout, the Lead pushes right after committing, subject to §6.2–§6.3:
  ```bash
  git pull --rebase origin main
  git push origin main
  ```
  If the push is rejected, the teammate just pushed: go back to §6.3 and repeat. Never "fix" it with
  force.
- Task branches are handed to the Lead for review and integration (§6.9). Never push a worker
  branch to `main` directly or rebase commits already shared with others. Publish task branches
  by their own names without force; preserve them until integration is verified.
- Make sure the project **still opens and compiles** before pushing. Never push a broken `main`; the
  teammate will pull it immediately.

### 6.5 Claiming files (`CLAIMS.md`)
Keep a lightweight claims file at the repo root so each side knows what the other is editing. Create
it if it doesn't exist:

```
| Who      | Files / folders / assets                   | Since        |
|----------|--------------------------------------------|--------------|
| Ryan     | Content/TriadDrift/Maps/L_Galaxy (World Settings) | Nov 13 10:00 |
| Nicholas | Content/TriadDrift/Art/Ships/Rift/*        | Nov 13 10:15 |
```

- Claim binary assets **before** editing them, especially maps and shared Blueprints like the
  GameMode, PlayerController, PlayerCharacter and GameInstance. Commit and push the claim right away
  so the other agent sees it.
- Release only your own claim, through the Lead, once the change is integrated and pushed to
  `main`. A worker-branch push alone does not release a claim. Never remove or rewrite another
  agent's or teammate's active claim, including during merges; uncertainty means keep it and ask.
- If something you need is claimed by the teammate, **don't edit it.** Tell your human so the two of
  them can coordinate.
- `git lfs lock <file>` / `git lfs unlock <file>` is available if you want a hard lock on a hot file.

**Start / finish rule (mandatory for both humans' agents).** `CLAIMS.md` is the live board both
sides read to know **what is in progress, what is coming next and what is done**. It is only useful
if it is current, so:

1. **Before starting any task:** `git fetch origin`, then read `CLAIMS.md` on `origin/main`, both
   sides' sections. Don't start work that overlaps a row that is in progress or queued on the
   other side; tell your human instead.
2. **On task start:** the Lead adds or updates that task's row in its own side's section (item,
   owner, state `in progress`, start time, the paths it touches) and pushes it to `main` with the
   dispatch. For anything binary or shared (maps, `.uproject`, `Config/`, shared Blueprints,
   plugins) the row must be on `origin/main` **before** the first edit.
3. **On task finish:** once the work is integrated and pushed to `main`, the Lead sets the row to
   `done` with the `main` commit and time, and releases any file claim, in that same push. A task
   that is merged but still has open review fixes is `in review`, not `done`.
4. **Queue what is coming:** list the next tasks per owner as `queued`, with the shared paths they
   will touch, so the other side can plan around them.
5. **Workers never edit `CLAIMS.md`.** They tell their Lead when they start and finish; the Lead
   updates the board. Each Lead edits only its own side's section and never rewrites the other's.
6. Keep `done` rows until the other side has had a day to see them, then prune them (the history
   lives in `docs/eng/STATUS.md` and `DEVLOG.md`).

### 6.6 Structure the project to avoid collisions
- **Folder ownership:** each person works only in their own role's folders
  ([features.md §2](features.md#2-folder--file-ownership)). Ryan has code, config and gameplay
  content; Nick has art, VFX, UI, audio, cinematics, presentation data and ArtSource. Never edit the
  other person's folders. Ask, or add a request to features.md §11.
- **Maps:** World Partition One File Per Actor is on, so two people can edit the same level without
  touching the same file. You still need to claim the map's World Settings and shared actors.
- **Shared Blueprints** (GameMode, PlayerController, PlayerCharacter, GameInstance) are hot spots.
  Always claim them first, and prefer adding new components or child Blueprints over editing the
  shared one.
- Prefer C++ or other text-based logic where it makes sense: text merges, Blueprints don't.

### 6.7 Hard rules: never do these
- ❌ `git push --force` / `--force-with-lease` on any branch
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

For concurrent agents on one human's side, use separate short-lived branches and worktrees where
supported. This extends the workflow above; it does not change Ryan's default `main` workflow or
the role ownership in features.md. Git worktrees share refs and configuration, so isolation does
not give permission for concurrent remote synchronization or overlapping asset edits.

### 6.9 Lead Agent and worker assignments
- Each human has their own Lead Agent. Nick's Lead coordinates Nick's workers only; it does not
  direct Ryan's agent or assume ownership of Ryan's files. Cross-role changes go through the humans.
- Before dispatch, record the worker ID, human, task, exact writable paths, read-only references,
  branch, absolute worktree path, base commit and claims. The task row goes on the `CLAIMS.md`
  board at dispatch and is closed at integration (start / finish rule, §6.5). An unspecified path is read-only. Shared
  maps/actors and dependency changes still require the existing claims and coordination (§6.5–§6.6).
- The Lead owns worktree/branch creation, remote synchronization, claim publication and integration.
  Workers request a serialized preflight from the Lead before each task and commit; the Lead runs
  the fetch and checks in §6.2–§6.3 for that worktree. Workers never independently pull, rebase,
  merge, push, switch branches or create/remove worktrees. Do not run Git mutations concurrently
  in a shared working tree. If worktree isolation is unavailable, allow only one writer at a time.
- Before binary editing begins, the Lead publishes the claim to `main` and verifies it on the
  remote. A local claim or one present only on a task branch is not team-visible authorization.
- Workers report changed paths, commit IDs (if any), validation and outstanding problems. The Lead
  repeats §6.3, checks ownership and binary overlap, integrates reviewed work in a clean designated
  checkout, validates it and pushes under §6.4. Serialize integrations. Never discard or overwrite
  human or teammate changes to unblock this process.
- Stop at infrastructure setup until the human authorizes game development. Do not launch asset or
  gameplay workers merely because their folders exist.

### 6.10 Message board (`MESSAGEBOARD.md`)
`MESSAGEBOARD.md` at the repo root is how Ryan's agents and Nick's agents **ask each other
questions and leave each other messages**. `CLAIMS.md` says who is doing what; the message board
is for everything that needs an answer. It is a **log: never delete a message.** Completed
messages are compacted (below), so whatever is still written out in full on the board is live.

- **Who writes:** only each side's Lead. Workers give their questions to their Lead, who posts
  them and relays the answers back.
- **When to read it:** on every `git fetch` (so before every task and before every commit, §6.2
  and §6.3) and at least every 30 minutes while working. Anything `OPEN` that is addressed to your
  side, and anything `ANSWERED` that your side asked, needs you.
- **Posting:** add the message at the bottom of **your own side's section** with status `OPEN`, an
  id (`R-001`... for Ryan's side, `N-001`... for Nick's side), who it is from, who it is for, the
  time, and whether it needs agents or a human (`Needs: Ryan` / `Needs: Nick`).
- **Answering:** add a `**Reply ...**` block under the message and set its status to `ANSWERED`.
  Don't edit the other side's words. Agents never answer a question marked for a human; they tell
  their human it is waiting. The one exception: when that human has told their Lead to decide that
  kind of thing for them, the Lead may answer, says so in the reply ("decided by Ryan's Lead under
  Ryan's standing delegation"), and reports it to the human, who can overrule it on the board.
- **Closing:** the side that asked reads the reply, acts on it, then sets the status to `COMPLETE`.
  If the reply isn't enough, add a follow-up under it and set the status back to `OPEN`.
- **Pushing:** commit it alone (`coordination: message board ...`) and push to `main` right away,
  after fetching and merging anything new. If both sides edited at once, keep both edits.
- **Statuses:** `OPEN`, `ANSWERED`, `INFO` (needs no answer; the other side's Lead sets it to
  `COMPLETE` once read) and `COMPLETE`.
- **Compaction (keep the board short).** When the board passes roughly 300 lines or ten completed
  messages, either Lead runs `python Tools/td_board_compact.py <summaries.json>`: every `COMPLETE`
  message moves word for word to `docs/coordination/MESSAGEBOARD-ARCHIVE.md` (append-only) and
  leaves one row (id, subject, outcome) in its side's "Completed" table. `OPEN`, `ANSWERED` and
  `INFO` messages are never touched. Write each outcome as what was agreed, complete enough to act
  on without opening the archive. Fetch and merge first, and push at once.
- **Ids are never reused or changed once pushed.** Fetch first and take the next free number. If a
  merge shows two messages with the same id, the one that was not yet on `origin/main` is renumbered.
- Decisions reached on the board that change the plan are copied to where they belong
  (features.md, `CLAIMS.md`, the area docs); the board itself is not the source of truth.

---

## 7. Repo status and remaining setup

**Done (Nov 12):**
- The Unreal project is at the repo root.
- `.gitignore` and Git LFS are set up. Files aren't marked `lockable`: we coordinate through
  `CLAIMS.md` instead.
- The Blender asset skills are in place (§10).

- (Nov 12, evening) The folder trees, collision channels, the five maps, the C++ contracts and
  presentation hooks (C-01, C-02) and the three Director's Cut hooks (C-03) exist on `main`, with
  `CREDITS.md`, `DEVLOG.md`, `CLAIMS.md` and `MESSAGEBOARD.md` in use.

**Where the current state lives** (this section is not kept up to date item by item):
- engineering: `docs/eng/STATUS.md`; presentation: `docs/visual/STATUS.md`
- who is doing what right now: `CLAIMS.md`; open questions between the sides: `MESSAGEBOARD.md`
- nothing is done until it is on `main`, verified, reviewed and played (`AAA.md`)

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
them automatically.

**Codex (Nick's agent) doesn't scan `.claude/skills/`.** Before any 3D asset task, open and follow
the `SKILL.md` at the path below. For native Codex skill discovery, mirror them into the
git-ignored `.agents/skills/` after each pull:
`robocopy .claude\skills .agents\skills /MIR`. `/MIR` deletes anything in the destination that is not in the
source, so `.agents/skills/` must hold nothing but this mirror (keep any other skills elsewhere). Never edit
the mirror; edit `.claude/skills/`.

The research behind the skills is in `reports/Blender MCP game asset practices.md`.

Unreal 5.9 also has its own skill loader: any agent connected to the editor's MCP server can list
Markdown skills (`AgentSkillToolset`: `ListSkills`, `GetSkills`) found in `<Project>/Skills/`, in each
enabled plugin's `Skills/` folder and in `Engine/Extras/AgentTools/Skills/`. To expose this repo's
skills there, mirror them into the git-ignored `Skills/` folder the same way (same warning: `Skills/` holds
only this mirror): `robocopy .claude\skills Skills /MIR`.

| Skill | Path | Use it when |
|---|---|---|
| `blender-mcp` | `.claude/skills/blender-mcp/SKILL.md` | Anything in Blender: MCP connection, bpy code on 5.2, headless runs, and verification (review renders, mesh report, floating-part check). **Read first.** |
| `blender-game-asset` | `.claude/skills/blender-game-asset/SKILL.md` | Making any 3D asset: routing (AI image-to-3D vs kitbash vs code), the AAA rubric, approval gates, and kits for ships, props, the corridor kit, rocks and crystals |
| `blender-texturing` | `.claude/skills/blender-texturing/SKILL.md` | Materials, UVs, and baking to Unreal texture sets |
| `blender-ai-mesh-cleanup` | `.claude/skills/blender-ai-mesh-cleanup/SKILL.md` | Rodin, Tripo or Meshy outputs and downloaded models → game-ready |
| `blender-unreal-export` | `.claude/skills/blender-unreal-export/SKILL.md` | Export to Unreal (FBX + manifest), calibration, and the UE import script |
| `ue-cpp-gameplay` | `.claude/skills/ue-cpp-gameplay/SKILL.md` | Any C++ gameplay work (`ENG-xx`): Core contracts, headless build and automation tests, worktree rules, UE 5.9 pitfalls. Team contract: `docs/eng/ARCHITECTURE.md`; status: `docs/eng/STATUS.md` |
| `ue-game-audio` | `.claude/skills/ue-game-audio/SKILL.md` | Any sound, music, ambience, voice, mixing or loudness work (`ART-10`, `ART-11`): sources and licences, layered design with takes, measured levels (ffmpeg scripts), MetaSounds, attenuation / classes / mixes built through Unreal's MCP server, the mix standard. An agent reaches "technically verified" at most; Nicholas approves by ear. Research: `reports/Game audio practices for Triad Drift.md` |
| `procedural-trees` | `.claude/skills/procedural-trees/SKILL.md` | Trees for any biome through Unreal 5.9's Procedural Vegetation Editor and the engine MCP server: species for a setting, botanical data with sources, seven crown archetypes (`UTDVegetationLibrary`), a parametric preset per species with growth stage and seed, scale and triangle checks. Background and mid-ground vegetation, not hero trees |

Check that a machine's Blender works with the skills:
```
"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python .claude/skills/blender-mcp/scripts/selftest.py
```

---

## 11. Agent working style for this jam

- **Scope is fixed:** everything in features.md ships; its complete-game checklist is the definition of a finished
  game (Ryan, Nov 13). Planets, powers and the boarding finale (the ending, wanted since Nov 13) are all in; the
  space-only reactor ending stays as the fallback. Don't cut or shrink a feature on your own. If something looks at
  risk, tell your human and let them decide.
- **Protect the three-ship switching and satisfying combat.** Never ship a change that makes them
  worse.
- **Stay in your role** (features.md §1 and §2). Ask before touching the other role's systems or
  folders.
- Keep the game **always playable** on `main`.
- Verify changes in the running game before committing (Play In Editor, or the headless tools for
  agents working in worktrees), and report honestly what was and wasn't tested. See "Playtests" below.
- When you're unsure whether something conflicts with the teammate's work, **ask your human. Don't
  guess.**

### Shared protocols (both sides keep these in step)
When either side adopts a way of working that the other should follow too (a ledger, a review step, a
tool convention, a naming rule), its Lead **posts it on `MESSAGEBOARD.md` and writes it here in the
same push**, so the two sets of agents never drift apart. Current shared protocols:

- **Boards:** `CLAIMS.md` at every task start and finish (§6.5); `MESSAGEBOARD.md` for questions and
  decisions (§6.10).
- **Quality standard and honest stages:** `AAA.md`. Every report says which of its stages the work
  has reached: prototype complete, technically verified, presentation approved, integrated, gameplay
  validated. Tests and screenshots alone never make something gameplay validated or final.
- **Per-item ledger:** each side keeps one table of every features.md item it owns, with owner, state,
  the `main` commit, how it was verified and open findings: `docs/eng/STATUS.md` and
  `docs/visual/STATUS.md`. An item is done only when it is on `main`, verified, reviewed and played.
- **Independent review:** after a lane lands, a fresh agent that did not write it reviews it against
  features.md and reports findings as BLOCKER / MAJOR / MINOR with a concrete failure for each. The item
  stays `in review` until every BLOCKER and MAJOR is closed and re-checked. Passing tests or hashes is
  not a review.
- **Never leave an agent idle** (while its human has work running; a queued row is a plan, not permission to
  resume a side that its human has paused): every lane has a next queued row on the board before the current one
  finishes.
- **Unreal's built-in MCP server** (UE 5.9 "Unreal MCP", doc link in §9): the project enables the
  engine toolsets (Editor targets only). The human's editor owns port 8000. Any editor or game an
  agent launches itself passes `-ModelContextProtocolPort=<port>`: Ryan's side 8010-8019 and
  21000-29000, Nick's side 8020-8029. One agent per editor; tool calls run one at a time.
  `Tools/td-play-mcp.ps1` starts a session and `Tools/td_mcp.py` calls it from a shell. Add `-Audio` for audio
  work: the audio engine runs and renders offline, silently (`-NonRealtimeAudio` instead of `-nosound`).
- **Epic's own agent tooling** ships with the engine in `Engine/Extras/AgentTools/`: skills for
  headless runs, log triage, build diagnosis and trimming build size
  (`Skills/<name>/SKILL.md`), and scripts such as `ue_log_digest.py` and `ue_crash_triage.py`. Read the
  relevant one before inventing a workflow.
- **What ships:** only the maps in the cook list, what they reference, and every data asset in
  `/Game/TriadDrift/Presentation/` (always cooked, with everything it references). An asset that is
  loaded only by a path string from anywhere else will be missing from the packaged game.
- **Audio:** `.claude/skills/ue-game-audio/`. Every 3D sound asset carries its own attenuation, sound class and
  concurrency (gameplay plays cues with no overrides); files are levelled with `audio_report.py`; nothing is
  "presentation approved" until Nicholas has heard it in the running game.
- **Small environment props may be made by engineering** (Ryan, Nov 12): asteroids, rocks, crystals, small flora and
  procedural trees, only where Nick's side has not started them, announced on `MESSAGEBOARD.md` first so they come off
  Nick's list. Sources go in `ArtSource/EngProps/**`, Unreal assets in `Content/TriadDrift/Gameplay/{Props,Vegetation}/**`.
  Nothing hero, no ships, characters, sky, terrain, VFX, UI or audio. Nicholas keeps art direction: he approves, restyles or
  replaces any of it, and a mesh assigned through his presentation data assets always wins.
  **Exception (Ryan, Nov 12, R-019):** the Null Armada enemy trooper character is built by engineering from Ryan's
  concept, in `ArtSource/EngCharacters/NullTrooper/**` and `Content/TriadDrift/Gameplay/Characters/NullTrooper/**`.
- **Engineering may take over presentation tasks while Nick's side is stopped** (Ryan, Nov 13, R-024): when Nick's
  agents are paused or asleep, Ryan's Lead may take a presentation task that is not being worked on (art, environment
  and terrain, VFX, audio, UI), **announced on `MESSAGEBOARD.md` and `CLAIMS.md` before the work starts**. The work goes
  in engineering folders (`Source/`, `Content/TriadDrift/Gameplay/**`, `ArtSource/Eng*/**`), never overwrites or edits a
  binary or path Nick's side has claimed, and reaches the game only as a fallback: an asset of Nick's at the Presentation
  name, or his tagged scene, always wins with no code change. Nicholas keeps art, visual and audio direction and final
  sign-off, and takes any task back by saying so on the board.
- **Headless first** (proposed by Nick's side in N-017, Nov 12): default to headless, background or command-line runs
  for automation, asset processing, Blender, builds, tests and Git; use a GUI only for inspection, interaction or
  gameplay validation that needs one. Headless is not the same as light: save work and checkpoints first, check free
  RAM and what heavy processes are already running, and on a 16 GB machine run one heavy job at a time, bounded and
  watched. This does not replace the real windowed playtests below.
- **Close every Unreal instance you start** (Ryan, Nov 13, R-043): before launching, list your own processes (the
  command line names your worktree) and close the stale ones; at most one game or editor per agent at a time, plus its
  own test run; every `-game` launch carries `-TDQuit=900` at most as a safety net; close the game the moment the
  capture, check or playtest step is done; every report ends with "no Unreal processes left" (or which is still running
  and why). Close only your own processes: never the human's editor (port 8000, the repo root) or another agent's.
- **At most two rendering Unreal processes on one machine at a time** (Ryan's Lead, Nov 14, R-046, after a crash; lowered from three at 01:20 when one instance measured 4-5 GB of video memory on a 16 GB card): Ryan's
  machine blue-screened at 00:30 on Nov 14 (bugcheck 0x119, the GPU scheduler) with many games and editors rendering at
  once. Before launching a `-game` session, an editor, PIE or a packaged build, count the `UnrealEditor*` and
  `unrealGameJam*` processes that render (no `-nullrhi`, not a headless test run); if two are already up, wait and
  re-check every 30 seconds. Headless tests and compiles are not counted. Commit work in progress often: a crash can
  leave an uncommitted file zero-filled.
- **On Ryan's machine, since Nov 14 13:25 (Ryan: "spin up instances faster and more often", without overloading the PC):** the
  two-renderer rule above is replaced by a measured guard. Three render slots (`Builds/render-slot/LOCK`, `LOCK2`, `LOCK3`)
  shared with T7's play session; before every rendering launch an agent runs `bash Builds/render-slot/can_launch.sh`, which refuses
  at 9,500 MiB of video memory in use (16 GB card; three capture sessions at once measured 12.5 GB), at three rendering
  processes machine-wide (T7's session included; corrected at 13:28 after that measurement),
  or under 12 GB of free system memory. Turns aim for 5 minutes (cap 10); headless work is done before a slot is taken; the
  top three lines of `Builds/render-slot/NEXT.txt` may go. Rules: `Builds/render-slot/README.txt`. Other machines keep the
  two-renderer rule unless they measure their own.
- **Story** (Ryan, Nov 13, R-034): the game follows `storyline.md`. Nicholas owns the story, its words and voices;
  Ryan's side owns how each beat is played and exposes a hook with a placeholder line for every beat (objective
  lines, barks, transmissions, console recordings, the ending). Nothing reveals the twist before the Dominion.
  Questions and changes: an issue or board message titled **`Story:`**.
- **Environment, VFX and Asset Management belong to Ryan's side** (Ryan, Nov 13 afternoon, R-036): Claude and Grok Bot
  build and manage all of it (space sky, galaxy dressing, planets, lighting, fog, grade, post-process, every Niagara
  effect, the asset inventory, intake and licences). Nick's agents stop work in those categories, publish what they
  have as reference, and work on UI and menus, audio and music, voice and story words, the Director's Cut presentation
  design and the submission. Nothing Nick's side made is deleted; replaced versions are indexed in
  `docs/eng/ART-HISTORY.md`. Art notes from Nicholas are welcome through an `Asset:` issue.
- **GitHub issues** (Ryan, Nov 13, R-033): a second channel beside `MESSAGEBOARD.md`. Either side opens an issue for a
  request, question, review or bug that needs an answer or work from the other side: one topic per issue, tag the other
  human's account (`@rthomas24`, `@nicholastosti-tech`), name the owner if known, start the title with `Asset:` or
  `Animation:` for those. Each Lead reads every open issue on each fetch, answers in the issue, routes the work, posts
  commits and captures there, and closes it when done. Workers do not post on GitHub; their Lead does. The board keeps the
  standing decisions and protocols: a decision reached in an issue that changes the plan is copied to the board,
  `features.md` or `CLAIMS.md` with the issue number.
- **Animation** (Ryan, Nov 13, R-032): Ryan's Lead leads all animation; the plan, the quality bar and one row per movement
  are in `animations.md`. Grok Bot supplies raw clips and rigged models (`ArtSource/EngAI/Animations/**`), the Lead's
  animation builders retarget, clean, author (Blender or in-engine) and capture every move in the running game
  (`ArtSource/EngAnim/**`, `Content/TriadDrift/Gameplay/Characters/<Character>/Anims/`), and the owning teammate sets the
  gameplay timing. Nick's side asks, notes or requests with **`Animation:`** in a board message subject; Nicholas keeps
  sign-off, and an animation of his at the Presentation name replaces ours.
- **Asset questions** (Ryan, Nov 13, R-031): Grok Bot leads high-quality asset generation for Ryan's side; anyone asks
  about an asset with **`Asset:`** in a board message subject, never by importing a parallel version.
- **Nothing the player walks on, into or through is an AI-generated mesh** (Ryan, Nov 13 evening, R-043): Rodin and
  other image-to-3D meshes came out soft, lumpy and with floors the pilot sinks into. Walkable structures (relay climb,
  wreck interior, outposts, dig site, Dominion interior) are built from a Blender modular hard-surface kit on a 50 cm
  grid with bevelled edges, a trim-sheet material and collision that matches the visual floor exactly, and are walk-tested
  with the real pawn. AI meshes stay as concept reference, exterior shells out of reach, or distant dressing. Hero ships
  seen up close get Blender rebuilds of the parts the player sees (crisp edges), a re-packed 4K bake and a tiling detail
  layer. Grok Bot supplies concepts, measured plans, tiling textures and trim sheets, and distant props.
- **Presentation asset names** are fixed in `docs/visual/Presentation-Asset-Paths.md`; gameplay looks
  them up by those exact paths, optional and null-safe.

### Playtests (mandatory, both sides)
Automation tests and screenshots prove that code runs. They do not prove the game is fun, readable
or even finishable. So `main` is **played** regularly, not only tested:

- **Who and when:** each side's Lead playtests the current `main` after every integration batch,
  and at least every two hours while work is landing. Workers playtest their own feature in their
  own worktree before reporting it done.
- **How:** a real windowed session, not a headless test. A human at the controls is best. An agent
  does it by driving a running game: launch it on a private port with the engine's built-in MCP
  server (`-ModelContextProtocolPort=<port>`), inject inputs, capture the viewport, read the log.
  Scripted captures with `Tools/td-shot.ps1` are the minimum when neither is possible; say which of
  the three it was.
- **What to cover (engineering checklist):** boot from the default map; fly, boost and drift; switch
  between all three ships; clear a wave; use each ship's signature power and any unlocked ability;
  die and see the "CUT!" retake; hyperdrive; fly through a portal gate; enter a planet, land, walk,
  use the blade, finish an objective, take off and return; the Dominion assault through to victory.
  Play whatever part of that exists today, in the packaged build at least once a day.
- **What to cover (presentation checklist):** every new asset or effect is seen in the running game
  on the real map at gameplay distance and speed: readable silhouette, the squad / enemy colour
  code, no clipping, sound heard in context, frame rate acceptable.
- **Record it:** add a short dated entry to `DEVLOG.md`: build or commit played, how it was played,
  what worked, what was broken or felt wrong, screenshots. Send each problem to the owner as a
  task; anything that breaks the three-ship switching or combat feel is fixed before new features.
- **Never report a feature as done on tests alone.** State plainly whether it has been played, and
  by whom or what.
