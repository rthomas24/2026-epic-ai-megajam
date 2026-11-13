# Ship quality: making the squad ships smooth and AAA (Ryan, Nov 13 evening)

**Ryan's finding, playing the build:** "All of the ships honestly don't look that great. They look not smooth. I
feel like these don't look as good as they would through Blender ... make these assets much smoother and look a lot
better from a AAA perspective." Also: Rift's ring is an empty hoop with no portal effect in it.

**Lead: Ryan's Lead.** Owners: the ship polish builder (Blender), the ship builder (Unreal import and materials),
the effects builder (portal and engine effects), Grok Bot (regenerated sources). Rift is the pilot ship: it is
finished and shown to Ryan before Lancer and Bastion are started.

## Why the ships look rough today
The three squad ships are raw image-to-3D output (Hyper3D Rodin Gen-2.5) imported with light clean-up only:
- **Surface noise.** AI meshes are triangle soup: small bumps, wobbling edges and uneven panel gaps where a modelled
  ship has flat planes and clean curves. Normal maps baked from that noise make it worse at close range.
- **Shading.** The import kept Rodin's normals; there are no weighted normals, no bevelled hard edges and no
  smoothing groups, so large surfaces shimmer and edges look chewed.
- **Asymmetry and floating bits.** The meshes are not mirror-symmetric (Lancer's guns differ left and right) and
  carry small detached islands (35 on Rift).
- **Missing or wrong functional parts.** Rift's portal disc was cut out; Bastion has two nozzles where the design
  has four pods; nozzle interiors are dull discs.
- **Texture resolution at walking distance.** 2K maps for a 5 m ship hold up from the chase camera but not at 3-10 m,
  where the player now stands beside the landed ship on planets.

## The options
| # | Option | What it does | Cost | Verdict |
|---|---|---|---|---|
| A | **Blender rebuild over the Rodin mesh** (retopology by quad remesh and shrinkwrap, exact mirror, subdivision with creased hard edges, weighted normals, bevels; the functional parts rebuilt cleanly with the hard-surface kit: nozzles, intakes, guns, cockpit, Rift's ring; then re-bake the Rodin colour onto the clean mesh and add modelled panel lines) | Keeps the design Ryan approved, removes the noise, gives clean highlights and silhouettes | About 5-7 h per ship | **Chosen.** Rift first |
| B | Regenerate in Rodin at higher settings (quad output, more faces, 4K textures, symmetric input views) | Better source for A; does not by itself give clean hard-surface shading | Grok Bot, about 1 h per ship | **In parallel**, as input to A |
| C | Engine-side only (full-resolution Nanite, texture streaming fixes, a detail-normal and panel-line overlay, better lighting on the ship) | Helps close range and blotchiness; cannot fix lumpy geometry | 2-3 h for all three | **Doing now** (ship builder) as the stopgap |
| D | Model the ships from nothing in Blender code | Clean, but earlier code-built ships were judged programmer art | 8+ h per ship | Rejected |
| E | Licensed ship models (Fab) | Fast and clean, but not our design, and must be licensed and listed | Unknown | Only if A fails on Rift |

## What "done" means for a ship (the bar)
- Clean silhouette from the chase camera at 6-30 m and from 3 m on foot; no surface wobble in a moving highlight.
- Exact left-right symmetry where the design is symmetric; no floating islands; no open geometry from any angle.
- Hard edges bevelled, large panels flat or smoothly curved, panel lines modelled or in a crisp normal map.
- Functional parts read at a glance: engines with glowing interiors, weapons, cockpit glass, Rift's ring with a
  living portal, Lancer's ram prow, Bastion's missile pods and dome emitter.
- Squad colour code kept (Rift magenta, Lancer cyan, Bastion amber) at a 60 / 30 / 10 split with white hull.
- Budget: 40-60k triangles (Nanite), 2K texture sets (4K source kept), same size, pivot, sockets and collision as
  today so gameplay does not change.
- Verified by clay and material turnarounds in Blender, a fresh-eyes review against the rubric in
  `.claude/skills/blender-game-asset/references/aaa-quality-bar.md`, and captures in the running game from the chase
  camera, on foot beside the landed ship, and as a wingman. Ryan approves Rift before the other two start.
- The old meshes stay in the repo (`docs/eng/ART-HISTORY.md`).

## Order
1. Now: option C on all three and Rift's portal membrane (ship builder).
2. Now: option A on **Rift** (ship polish builder), with Grok Bot's Rift r2 used if it arrives in time.
3. After Ryan's look at Rift: Lancer, then Bastion; then the enemy fleet only if time remains (the fleet reads well
   at combat distance today).

## Result of the Rift pilot (Nov 13, about 17:00 EST): option A failed its gate; the route is now a hybrid
The clean Blender rebuild of Rift was stopped at the secondary-forms gate. Side by side with the Rodin original it
read as a toy ("a plastic shuttle with pink stickers", the fresh-eyes reviewer): the rebuild won on edges, shading
and technical health (4 against 2, 4 against 1) and lost on proportion, detail and style (2, 1 and 2 against 4, 3
and 3). Sheets: `ArtSource/EngShips/RiftHero/review/03_rebuild_v03_BEFORE_AFTER_{clay,material}.png`. Option A is
cancelled for all three ships.

**What the pilot showed.** Rendered in Blender with its own 2K textures, the Rodin original looks good: crisp panel
lines, clean colour. In the game the same ship looks blotchy at 8 m. So most of Ryan's finding is **our Unreal
import, material and lighting**, plus three real geometry faults: the empty, ragged ring, the engine block with a
hollow centre nozzle, and 11,122 disconnected islands.

**The route now (hybrid):**
1. Ship builder: make the in-game ship match the Blender render (texture streaming and settings, normal and ORM
   handling, the SelfLight and BaseTint values that flatten contrast, lighting on parked ships, exposure), for all
   three squad ships; the animated portal in Rift's ring.
2. Ship polish builder: keep the Rodin fuselage, canopy, wings and guns (welded and cleaned, same UVs); replace the
   ring, the engines and back plate, and the fins if it helps, with designed hard-surface parts; clean the kept
   textures (flat clean white and magenta, calmer roughness, softened normals on the wings). Exported for import.
3. Lancer and Bastion get the same treatment only after Ryan has seen Rift.

