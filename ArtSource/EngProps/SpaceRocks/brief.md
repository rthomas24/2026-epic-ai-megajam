# Brief: SpaceRocks kit (engineering-made head-start props)

Requested by the engineering lead (MESSAGEBOARD.md R-015) as a head start for the asteroid fields of
the space level. The art director reviews these afterwards and may restyle or replace them.

- **Purpose and camera:** scattered as scaled instances in the asteroid fields of the galaxy level;
  seen from a chase camera against black space under one strong sun, from far away down to very
  close, at instance scales up to several hundred metres. Strong medium forms; no tiny noisy detail.
- **Sizing contract:** every mesh fits a **100 cm diameter bounding sphere with the pivot at the
  centre of that sphere**, so it replaces the engine's unit sphere with the same scale values.
- **Pieces:** four asteroids `SM_Asteroid_01..04` and three debris shards `SM_SpaceDebris_01..03`.
- **Budgets:** asteroids 1,500-3,500 triangles, one 1024 texture set each; debris 200-600 triangles,
  one 512 set each. One material slot per mesh. Maps: base colour, normal (DirectX), packed ORM,
  and a separate emissive map where crystals are used (so the glow can be turned off).
- **Shape language:** broken, faceted, scalloped (flake scars), never a noise-displaced sphere.
  - `01` chunky and faceted: an angular block with a second block fused on and stepped ledges.
  - `02` elongated: a long tapering shard, one blunt broken end, a stepped notch.
  - `03` cratered and rounder: one giant crater with a central peak, several large and medium ones.
  - `04` fractured: one mass split by a deep, wide zig-zag cleft with a side crack.
  - debris: a flat slab shard, a long splinter, a blocky wedge chunk.
- **Palette:** mid-value stone (slate, rust, dust, basalt families), cooler and darker in cavities,
  lighter worn fracture edges, a few light mineral veins and patches. Emissive crystal inclusions on
  two asteroids only: cyan `#2EE6FF` on `02`, violet `#A24DFF` on `04`, well under 5% of the surface.
- **Collision:** one convex `UCX_` hull per asteroid (gameplay treats them as spheres anyway); none
  for debris. No sockets. No LODs.
- **Route:** C (code-built, procedural), per `.claude/skills/blender-game-asset/SKILL.md`.
- **References:** none copied; original shapes generated from seeded code in `build_spacerocks.py`.
- **Owner and folder:** engineering (Ryan's side), `ArtSource/EngProps/SpaceRocks/` (claimed in
  CLAIMS.md). Not imported into Unreal by this task.
