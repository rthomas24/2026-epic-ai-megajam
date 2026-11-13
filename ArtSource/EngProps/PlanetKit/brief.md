# PlanetKit brief (engineering-made on-foot set pieces for Verdance and Cinder)

**Purpose and camera.** Replace the engine basic shapes (cubes, cylinders, cones) that gameplay uses for the planet
set pieces (features.md ENG-25 Verdance, ENG-26 Cinder). Seen on foot from a 1.8 m eye height at 1-30 m, under dim
light (Cinder: dim warm volcanic light; Verdance: dusk with cyan bioluminescence).

**Footprints are fixed.** Gameplay keeps its own collision boxes and has tuned jump / mantle / dash routes on them, so
every mesh fills its box exactly (pivot bottom centre unless stated), walkable tops are flat at the exact height, and
sides that gameplay collides with stay within +-2 cm of the box. No collision on the meshes.

**Shape language (aaa-quality-bar.md).**
- Null Armada (outpost wall, doorway frame, door, crates, signal jammer): blade-like angular plates, layered matte
  black armour, dark crimson secondary plates, red-orange emissive seams, white "eye" lights, forward-raked spikes.
- Cinder rock (choke rocks, stepping stone, collapse slab, debris): vertical, crystalline, sharp: black basalt columns,
  ash on top faces, cyan / amber crystal veins, magma glow only at the lava line.
- Verdance relay (platforms, plateau, pillar, node stub, trailhead): ancient carved violet-grey stone, masonry
  courses, a recessed glyph band with faint cyan glyphs, medallions as each side's focal point, teal moss with glowing
  spores, glyph rings on the walkable tops.
- Verdance wreck (hull, engine, wing, fin): an original ivory / teal ship, torn and half-buried, overgrown with
  moss grown on the skin and hanging vines with glowing pods.

**Palette.** Null: armour `#1B1B21`, crimson `#4A0A12`, gunmetal `#262A31`, seam `#FF4A1E`, eye `#F4F7FF`. Cinder:
basalt `#1A1718` / ash `#4A4446`, crystal cyan `#2EE6FF`, amber `#FFB347`, magma `#FF5A1F`. Verdance: stone
`#3A3D55` / `#787A94` with violet tint `#6A5A9E`, moss `#1E6E5E`, glow `#3CFFE0`, violet glow `#B07CFF`; wreck
ivory `#D9D2C1`, livery teal `#19C3B1`. Neutral interactables: gunmetal `#3A3F48`, ivory `#D8D2C4`, glow `#3FE9FF`.

**Budgets.** Big pieces up to 40k triangles (Nanite on), small pieces 2k-10k. One material slot per mesh, baked
BC / N / ORM / E sets (2048 big, 1024 small) on instances of one master `M_TD_PlanetKit`; stone pieces also carry a
second UV channel for a tiling detail normal so the large faces do not look stretched or blurry.

**Owner and folders.** Engineering (Ryan's side), as small environment props under the Nov 12 rule (AGENTS.md
section 11); sources `ArtSource/EngProps/PlanetKit/`, Unreal `/Game/TriadDrift/Gameplay/Props/Planet/`. Nicholas keeps
art direction and may restyle or replace any of it.
