# Mechanic Base parts (Fab "Modular Scifi Mechanic Base")

22 static meshes (no Blueprints, no floors, walls or ceilings), with the materials and textures they need, in
`/Game/TriadDrift/Gameplay/Props/Fab/MechanicBase/` under the pack's own sub-paths (`Mesh/SM/...`, `Material/...`, `Texture/...`).
Dressing for the Dominion interior and outposts; door frames are frames only (our kit stays the walkable surface, R-043).

| Group | Meshes (triangles) |
|---|---|
| Consoles and machines | SM_COMPUTER_01 (4,756), SM_COMPUTER_02 (9,281), SM_ELEVATOR_CONSOLE_01 (1,156), SM_PANEL_01 (178), SM_ELECTRIC_BOX_01_CLOSE (2,674), SM_ELECTRIC_BOX_01_OPEN (4,756), SM_POWER_GENERATOR_01 (16,813), SM_SCANNER_01 (4,772), SM_CONNECTION_DEVICE_01 (1,632), SM_WALL_INSTALLATION_02 (1,020) |
| Lamps | SM_LAMP_01 (1,578), SM_LAMP_02 (2,160), SM_LAMP_03 (772), SM_LAMP_04 (428), SM_LIGHTBRACKET_01 (895), SM_LIGHTBRACKET_02 (92) |
| Door frames | SM_DOOR_FRAME_01_OUTSIDE (666; 4.0 x 3.5 m), SM_DOOR_FRAME_01_INSIDE (1,801), SM_DOOR_FRAME_03 (1,260; 7 x 4.5 m), SM_FRAME_04 (1,458), SM_FRAME_05 (7,108), SM_DOORFRAME_01_UP (992) |

## Source and licence

- "Modular Scifi Mechanic Base" by **Jay Zheng**, https://www.fab.com/listings/5782c63b-9238-4012-aa4d-53703529dd49 (issue #182).
  Supplied by Nicholas Tosti (release `asset-sources-nicholas-20261114`, archive SHA-256
  `8f42a5826a5f7e6bdd672172b1b8294fe41bc6074ab331d8d210fa9a1c1f35db`; original kept in the git-ignored `Builds/intake2/MechanicBase/`).
- **Licence: Fab Standard License** (https://www.fab.com/eula); tier acquired not recorded; publisher attribution optional,
  project credit given. **Not AI-generated** (NoAI label). No standalone redistribution.

## How (Ryan's side, Claude Code, Nov 14 2026)

The pack's Content folder was copied unchanged into a staging folder (`Content/Modular_Scifi_Mechanic_Base`, never committed,
deleted afterwards). `../scripts/ue_import_mechbase.py` (headless editor, -nullrhi) moved the 22 picked meshes
(`../scripts/ue_mechbase_pick.py`) and their whole dependency closure (61 packages: meshes, 14 material instances, the
pack's masters and material function, textures) in one batch, then:

- **Null recolour through the instances only** (base / second colour, decal colour, emission colour). Every recoloured instance
  keeps a copy of the seller's version beside it as `<name>_orig`. Grey-green metals to black armour; the orange and light-grey
  metals (`MI_Metal_06`, `MI_Metal_10`) to crimson accents; info decals dark red; lights (`MI_EMISION*`, cyan, amber and warm
  white before) to Null red-orange. The previous values are in the import log (`RECOLOURED`).
- Textures capped at 2048 (five trim textures are 8K at source; the source data is unchanged, the cook uses 2K).
- Collision as supplied (the picked meshes carry their own simple collision; not walk-tested).

Sheets: `review/Fab_MechanicBase_before.png` (the pack's editor thumbnails of the 22 meshes, before recolour);
the after look is `review/Fab_MechanicBase_after_ingame.png` when captured. The probe of the original materials:
`../scripts/ue_mechbase_probe.py`. Not placed anywhere.
