<p align="center">
  <img src="docs/media/cover.jpg" alt="Triad Drift cover art: the Triad Drift logo over three ships" width="540">
</p>

<h1 align="center">Triad Drift</h1>

<p align="center">
  <b>Lead a wing of three ships, cut between them like a director, land on alien worlds and board the mothership.</b>
</p>

<p align="center">
  <a href="https://github.com/rthomas24/2026-epic-ai-megajam/releases/download/v1.0-jam/TeamTriad_TriadDrift.zip"><b>Download for Windows (884 MB)</b></a>
  &nbsp;·&nbsp;
  <a href="https://rthomas24.itch.io/triad-drift">Play on itch.io</a>
  &nbsp;·&nbsp;
  <a href="https://www.youtube.com/watch?v=b0bGf0opTZs">Watch the trailer</a>
  &nbsp;·&nbsp;
  <a href="DEVLOG.md">Read the dev log</a>
</p>

<p align="center">
  Made in four days for the <b>2026 Epic AI MegaJam</b> · Theme: <i>Director's Cut</i> · Unreal Engine 5.9
</p>

---

## Trailer

<p align="center">
  <a href="https://www.youtube.com/watch?v=b0bGf0opTZs">
    <img src="docs/media/trailer-thumb.jpg" alt="Watch the Triad Drift gameplay trailer on YouTube" width="720">
  </a>
</p>

## About the game

You lead a wing of three ships across a small galaxy and can switch between them at any moment. The two you are not flying keep fighting on their own.

The run goes like this:

1. **Hold the line.** Clear the Null Armada's waves in open space.
2. **Jump.** Hyperdrive across the galaxy to new worlds.
3. **Land.** Set down on **Verdance**, a bioluminescent jungle, and **Cinder**, a world of lava and crystal, and earn a new ability on each.
4. **Break the shields.** Destroy the Dominion mothership's shield generators.
5. **Board.** Go inside the Dominion on foot with an energy blade.
6. **Escape.** Get out before it comes apart.

<p align="center">
  <img src="docs/media/space-combat.jpg" alt="Space combat: Kato's Lancer near a ringed planet, with the squad HUD" width="100%">
</p>

## Director's Cut

We read the theme literally: you are the director of a three-ship squad.

- **Switching ships is a camera cut.** Every switch cuts to the new ship and shows its pilot's name card.
- **Director shots.** Big moments drop into slow motion.
- **Scenes and takes, not lives.** When you die, the game calls **"CUT!"** and restarts the scene as the next take.

The theme also shaped how we made the game. Two people acted as directors for a crew of AI agents, cutting between them the way the player cuts between ships. More on that [below](#how-it-was-made).

## The squad

| Ship | Pilot | Weapon and signature power | Ability unlocked on a planet |
|---|---|---|---|
| **Rift** | Wren | Portal blaster | Cloak |
| **Lancer** | Kato | Rapid cannons and an afterburner ram | EMP Pulse |
| **Bastion** | Moss | Missile swarm and a shield dome | Gravity Well |

## Download and play

1. Download **[TeamTriad_TriadDrift.zip](https://github.com/rthomas24/2026-epic-ai-megajam/releases/download/v1.0-jam/TeamTriad_TriadDrift.zip)** (884 MB) from the [v1.0-jam release](https://github.com/rthomas24/2026-epic-ai-megajam/releases/tag/v1.0-jam), or from [itch.io](https://rthomas24.itch.io/triad-drift).
2. Unzip it anywhere.
3. Run `unrealGameJam.exe`.

**Requirements:** Windows 10 or 11, 64-bit. If the game won't start because a runtime is missing, run `Engine\Extras\Redist\en-us\vc_redist.x64.exe` from the unzipped folder first.

| | |
|---|---|
| Build | Shipping, Win64 |
| Source commit | [`54cfd78a`](https://github.com/rthomas24/2026-epic-ai-megajam/commit/54cfd78afad12d6e42b30e7c01163e27f01c77ad) |
| Zip SHA256 | `D2B7D5450ECCFC67FD3CB44A1E8B67B06B0E62A3EF31B556B648B3463B2EDCBA` |

## Controls

Keyboard and mouse. A controller is mapped for every action, but we haven't fully play-tested it.

**In flight**

| Action | Key |
|---|---|
| Aim | Mouse |
| Throttle | W / S |
| Roll | A / D |
| Boost | Left Shift |
| Drift | C |
| Brake | Space |
| Fire | Left / right mouse button |
| Signature power | Q (tap or hold) |
| Unlocked ability | E |
| Switch ship | 1 / 2 / 3 or Tab |
| Shadow mode | G |
| Hyperdrive | H |
| Squad command / land | F (hold F to land) |
| Target | T |
| Camera | V |
| Pause | Esc |

**On foot**

| Action | Key |
|---|---|
| Move | WASD |
| Look | Mouse |
| Jump | Space |
| Sprint | Shift |
| Energy blade | Left mouse button |
| Block / deflect | Right mouse button |
| Interact | E |

## Screenshots

<table>
  <tr>
    <td><img src="docs/media/keyart-verdance.jpg" alt="The pilot with a cyan energy blade in Verdance's bioluminescent forest"></td>
    <td><img src="docs/media/keyart-cinder.jpg" alt="The pilot with a magenta energy blade on Cinder, among lava and crystals"></td>
  </tr>
  <tr>
    <td><img src="docs/media/wave1.jpg" alt="Wave 1 against the Null Armada, with the squad HUD"></td>
    <td><img src="docs/media/flight.jpg" alt="Flying past a ringed planet through an asteroid field"></td>
  </tr>
  <tr>
    <td><img src="docs/media/flight-95s.jpg" alt="Combat in a magenta nebula with target lock and radar"></td>
    <td><img src="docs/media/main-menu.jpg" alt="The Triad Drift main menu"></td>
  </tr>
  <tr>
    <td><img src="docs/media/dominion-hangar.jpg" alt="On foot in the Dominion's hangar with the energy blade"></td>
    <td><img src="docs/media/reactor-room.jpg" alt="The Dominion reactor room"></td>
  </tr>
</table>

<sub>The Verdance and Cinder images and the space combat shot are key art from the game; the rest are captures from the packaged Shipping build and in-engine checks.</sub>

## How it was made

Triad Drift was built during the jam, November 12–15, 2026, by two people, each directing a team of AI agents. The agents wrote the code, built the levels and assets, and reviewed each other's work. The humans set direction, played the builds and made the calls.

| Person | Role | Agent team |
|---|---|---|
| **Ryan Thomas** | Gameplay and engineering | **Claude Code** (Anthropic): a lead agent plus builder sub-agents for gameplay C++, tools, environments, effects, animation, imports and packaging. **Grok Bot** for concept art, textures and asset generation. |
| **Nicholas Tosti** | Player experience: UI, audio, story, presentation | **OpenAI Codex**: a lead agent plus workers for UI and menus, audio and music hookups, story and dialogue presentation, reviews and playtests. |

### How the agents worked together

Everything went through this repository. The coordination files are in the repo:

- [`AGENTS.md`](AGENTS.md): the shared rules every agent works under.
- [`MESSAGEBOARD.md`](MESSAGEBOARD.md): requests and answers between the two teams' lead agents.
- [`CLAIMS.md`](CLAIMS.md): who is working on which files, so agents don't collide.
- [`DEVLOG.md`](DEVLOG.md): a running record of decisions, failures and fixes, written as the work happened.

Work landed through branches and pull requests, and builds were packaged and play-checked along the way. The [releases page](https://github.com/rthomas24/2026-epic-ai-megajam/releases) keeps the intermediate playable builds and evidence packs as well as the final one.

### Art and asset pipeline

- **Hyper3D Rodin** (image-to-3D) for the squad ships, Null fleet, the Warden, the relay, the wreck, the outpost and other set pieces. Concept images came first, then Rodin meshes.
- **Blender 5.2, scripted and run headless.** Ships, props, rocks, crystals, and a modular walkable kit on a 50 cm grid. Everything the player walks on was rebuilt from that kit, with bevels and weighted normals.
- **Unreal Engine 5.9 (MegaJam Preview)**, including its built-in MCP server for agent-driven editor work, Niagara and the Procedural Vegetation Editor.
- **AI voice generation** for pilot and command lines, plus AI-generated music and textures. Every tool and asset is listed in [CREDITS.md](CREDITS.md).
- **Fab** marketplace assets for some rocks, plants, cliffs and decals, also credited in [CREDITS.md](CREDITS.md).

### What existed before the jam

Only tooling: agent configurations and instructions, MCP server configs, agent skills and prompt templates, and general-purpose Blender helper scripts. No game code, levels, art, audio or story existed before November 12.

### AI disclosure

This game was made with generative AI. Code, graphics, sound, voice and text were all produced or assisted by AI tools, directed and reviewed by the two of us. [CREDITS.md](CREDITS.md) marks which assets are AI-generated and with which tool.

## Building from source

1. Install **Unreal Engine 5.9** (the MegaJam Preview build) and Visual Studio with the C++ game development workload (see [`.vsconfig`](.vsconfig)).
2. Clone this repository. The project uses Git LFS for binary assets, so run `git lfs pull` after cloning.
3. Open `unrealGameJam.uproject` and let it build the C++ modules.
4. To package a Windows build like the release, follow [docs/coordination/PACKAGING-HOWTO.md](docs/coordination/PACKAGING-HOWTO.md).

## Team

- **Ryan Thomas**: gameplay and engineering
- **Nicholas Tosti**: player experience (UI, audio, story, presentation)

Full asset, tool and licence credits: [CREDITS.md](CREDITS.md).
