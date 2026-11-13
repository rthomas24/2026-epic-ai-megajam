---
name: blender-game-asset
description: Production workflow for AAA-quality, game-ready 3D assets for Triad Drift (Unreal Engine 5), made in Blender through the Blender MCP or headless. Covers spaceships, enemy ships, the Dominion mothership, modular sci-fi corridor kits, props, weapons, the energy blade, asteroids, rocks, crystals and alien flora. Routes each asset to the right method (concept + AI image-to-3D, licensed kitbash, or code-built), enforces human approval gates, and runs staged builds with measured checks and fresh-eyes review against an explicit AAA rubric. Use whenever someone asks to model, make, build, generate, design, block out, improve or "make better" any 3D model, asset, prop, ship or environment piece, even casually ("make me a cool ship", "we need rocks").
compatibility: Blender 5.2.x. Read the blender-mcp skill first (connection, code rules, verification scripts). Uses scripts/hardsurface.py and scripts/procedural_rocks.py.
---

# Making AAA-quality game assets in Blender

**The bar is AAA-worthy design, not "it renders".** A code-built fighter made from lofts, boxes and
greebles was judged "terrible" by the team lead. It was clean, manifold and well shaded, and still
read as programmer art. Research agrees: agents writing Blender code reliably produce "simplistic,
floating primitives" unless the *design* comes from somewhere stronger and the work passes hard
gates. This skill fixes that with three things:
1. **Route** the asset to a method that can reach the bar.
2. **Gates**: human approval at concept, blockout and final.
3. **Fresh-eyes review** against a written rubric, with measured checks at every stage.

Read `references/aaa-quality-bar.md` before your first asset. It defines "done".

## Approved Triad Drift routes and current references (November 12)

Nicholas explicitly approved Lancer M03 source production from his blue reference and approved A02 v03 structure at19:07EST. Continue that existing authored foundation for bounded materials/detail improvements; the route defaults below do not revoke it or require a new acquisition. White-player armor is original fitted gear on the existing owned Manny reference, not newly generated anatomy. Preserve the base; rig/export readiness remains unverified.

The fresh Lead handoff and CLAIMS.md identify exact source/reference paths and scoped grants. Blue Lancer, yellow Bastion and green Rift redesign references do not change the runtime Rift-magenta/Lancer-cyan/Bastion-amber contract. Bastion/Rift remain secondary and separately gated. Current-cycle Material04 was explicitly held by Nicholas for material correction; no export/import promotion follows a technical pass. Human final appearance, mesh QA, resource scheduling and Unreal validation remain mandatory.

## Step 0: Route the asset before touching Blender

| Asset type | Route | Why |
|---|---|---|
| **Hero ships** (Rift, Lancer, Bastion), the **Dominion** exterior, **creatures, characters, organic hero flora** | **A**: concept image → AI image-to-3D (Hyper3D Rodin Gen-2 first; Tripo or Meshy as alternatives) → `blender-ai-mesh-cleanup` → hand polish → texture → export. Or **B**: licensed Fab/kitbash parts | Design quality and speed. Code-only hero ships don't reach the bar. Rodin use qualifies for the jam's $10k Rodin prize pool |
| Enemy fighters, gunships, bombers, carrier | **A**, or **C** with a strong concept | Mid-detail; many on screen |
| Modular corridor kit, doors, consoles, crates, props, weapons, energy blade, debris | **C**: code-built with `scripts/hardsurface.py` + trim sheets | Agent strength: precise, modular, grid-based |
| Asteroids, rocks, crystal clusters | **C**: `scripts/procedural_rocks.py` (+ Nanite) | Procedural strength |
| Humanoid crew, first-person arms | **B**: Fab or Mixamo rigged assets (or Rodin T-pose + auto-rig) | Never code-model people from nothing. Armour and gear built in code **on an existing rigged mannequin** (UE5 Manny) is a different, allowed route when Nicholas has approved it |

Details, generator settings and licences: `references/asset-routing.md`. Code-built (route C)
assets still need a concept or a precise written design, and still have to pass the rubric.

## Step 1: Write the brief (5 minutes; saves hours)
Write it into the asset folder as `brief.md`. It covers:
- **Purpose and camera:** where it's seen and how close; for example chase cam 6–30 m.
- **Size in cm, tri budget, material plan:** take these from `references/art-direction.md`.
- **Shape language:** three words, such as "swept, predatory, crescent".
- **Signature feature, palette and functional parts list:** for example engines, intakes, cockpit, hardpoints.
- **Sockets and collision needs, owner (features.md §2) and folder.**
- **References:** concept image or 3+ reference images, used for inspiration only. No copying of
  existing games' or films' designs.

## Step 2: Concept gate (human)
Get a concept image from the human. Or write them a ready-to-paste image prompt (template in
`references/asset-routing.md`) for an image tool such as fal, ChatGPT or Midjourney. Show the brief
plus concept and **get an explicit OK before building**. If no image is possible, proceed with the
written design and treat the blockout gate as the design approval.

## Step 3: Build in gated stages
Each stage follows the same loop:
1. Build in small `execute_blender_code` steps (live) or one script (headless), in the asset's own
   collection, at real scale.
2. Check the numbers: `asset_report.py` (budget, geometry health, scale) and `assembly_check.py`
   (no floating parts).
3. Render `review_renders.py` (clay for form stages, material for the material stage), and look at
   it against the concept.
4. Get a fresh review (see below). Fix the top issues, or revert to the last saved version if the
   fix made it worse.
5. Save `<Asset>_vNN.blend`.

| Stage | What exists at the end | Gate |
|---|---|---|
| **1. Blockout** | Primary masses only; correct dimensions; reads from front, side, top and the game camera | **Human approves the silhouette** (clay sheet). Nothing else proceeds until then |
| **2. Secondary forms** | Engines, wings, intakes, cockpit, weapons, plates, joints. Every part attached, with 2–5 cm overlap or a visible mount | Rubric ≥ 3 on silhouette, proportion and construction |
| **3. Tertiary detail** | Panel grooves following the form, inset panels and vents, clustered greebles near functional areas, chamfers on every hard edge | Rubric ≥ 4 on detail and shading |
| **4. Materials** | 60/30/10 colour split, roughness variety, metal only where bare, emissive ≤ 5% of surface leading the eye | Rubric ≥ 4 on everything; **human approves final** |
| **5. Game-ready** | Hand-off to `blender-texturing` (bake, or flat material instances) and `blender-unreal-export` | Numbers clean, FBX round trip passes |

**Route A assets** start at stage 2 after cleanup. The AI mesh replaces the blockout, but it still
passes the silhouette gate. Then polish:
- fix proportions with lattice or scale edits
- add the crisp parts AI is bad at: emissive inserts, nozzles, intakes, decals
- give the Squad's symmetric hulls an exact mirror
- add sockets and collision

## Fresh-eyes review (every stage after blockout)
Self-critique by the agent that built the asset barely helps, and can make things worse. Use a
reviewer who hasn't seen the build history.
- **In Claude Code:** spawn a subagent. Give it only the brief, the concept/references, the latest
  clay and material contact sheets, and the numeric reports. Ask it to score the rubric (1–5 per
  category) and list the **three highest-impact fixes**. The prompt template is in
  `references/aaa-quality-bar.md`.
- **Elsewhere:** do the same review in a fresh conversation, or ask the human.
- **Stop rules:** a prop gets 1–3 rounds, a hero asset 4–8. If two rounds pass without the score
  improving, stop and escalate to the human: change route or change the design. Don't grind.
- Keep every scored contact sheet in `review/` so the dev log (the Glass Box prize) can show the
  progression.

## Code-built assets: use the kits, not raw primitives
`scripts/hardsurface.py` gives designed starting forms and the standard finishing stack. Recipes and
the API are in `references/hard-surface.md`:
- **Forms:** `loft` (hulls from superellipse sections, with `groove` panel lines), `wing`, `lathe`
  (nacelles, nozzles, barrels).
- **Surface:** `plates` (layered armour with seams), `inset_panels` (vents, glow strips),
  `greebles(region=...)` (clustered, never sprinkled).
- **Finishing:** `add_bevel` + `add_weighted_normal`, `add_mirror`, `boolean` (`CUT_` cutters).
- **Utilities:** `join`, `set_origin`, `apply_transform`, plus `paint` / `glow` / `glass` /
  `hex_color` for materials.

`scripts/procedural_rocks.py`: `make_asteroid`, `make_crystal_cluster`, `make_rock_material`. See
`references/environment.md` (flora is a route A/B job).

Lessons from our own tests, so you don't repeat them:
- **Shards from decimation:** choose density at generation time; collapse-decimating noisy surfaces
  leaves shard-like triangles.
- **Torn displacement:** make displacement profiles continuous, or the surface tears.
- **Washed-out glow:** emission above about 3 turns saturated colours white.
- **Flat normal maps:** drive bump from a separate fine noise, or the baked normal map is flat.
- **Bevel and inset collapse:** add an ogive section at sharp nose cones, and skip tiny faces when
  insetting; both otherwise collapse into duplicate vertices.
- **Empty material slots:** give boolean cutters the target's material first.

## Files, names, ownership

```
ArtSource/<Category>/<Asset>/       e.g. ArtSource/Ships/Rift/, ArtSource/Environment/Asteroids/
  brief.md                  the brief (step 1)
  refs/                     concept + reference images
  source/                   AI or downloaded originals + SOURCE.md (tool/site, licence, date, prompt)
  <Asset>_v01.blend ...     versioned working files; latest = highest number
  review/                   scored contact sheets (keep them; small PNGs)
  export/                   SM_<Asset>.fbx, T_<Asset>_*.png, SM_<Asset>.manifest.json
```
- **Ownership (features.md §2):** `ArtSource/` and the Unreal art folders (`Content/TriadDrift/Art`,
  `Environment`, `VFX`, `Presentation`) belong to **Nick**, the Art, Visuals & Player Experience lead.
  Ryan's agent claims a file in `CLAIMS.md` before touching anything there, e.g. to add a collision
  proxy or socket. Final art reaches gameplay through Nick's presentation data assets (features.md
  §3.5), never by editing Ryan's Blueprints or C++.
- **Names:** objects `SM_<Asset>` (root), parts `SM_<Asset>_<Part>`, materials `M_<Asset>_<Role>`,
  helpers `CUT_*`, `SRC_*`, `UCX_*`, `SOCKET_*`. No spaces, no `.001`.
- Log every AI tool and downloaded asset in `CREDITS.md`, with tool or site, licence and what it was
  used for. The jam requires it, and Rodin, Meshy and Tripo usage feeds partner prizes.
- Add a one-line DEVLOG.md entry per finished asset: what worked, what didn't, and a review sheet
  path.

Shared or claimed `.blend` files follow AGENTS.md §6.5 (`CLAIMS.md`). Binary files need Git LFS;
see the blender-mcp skill.
