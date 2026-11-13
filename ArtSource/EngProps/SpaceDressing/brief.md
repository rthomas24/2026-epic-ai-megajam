# SpaceDressing kit: brief

**Owner:** engineering (Ryan's side), small environment props under the Nov 12 rule (AGENTS.md section 11).
Nicholas keeps art direction and may restyle or replace any of it.
**Route:** C (code-built in Blender 5.2, headless): `scripts/build_space_dressing.py`.
**Camera:** fast ships at 50 m to 3 km, often against black space and a bright nebula. Detail is sized for
50 m+; the smallest modelled detail is about 3 to 5 cm on the 4 to 15 m props and 25 cm+ on the large pieces.

## Look
Bright, saturated sci-fi dressing that must never read as the player's squad (magenta / cyan / amber) or as
the Null Armada (red-orange on black armour). Neutral palette: warm ivory, gunmetal, gold trim and foil, faded
teal and violet paint, teal and warm-white glow. Shape language: functional, weathered, "built by someone".

| Family | Size | Shape words | Signature |
|---|---|---|---|
| Ice asteroids | 20-80 m | faceted, fractured, glacial | polygonal fracture network with blue crevices, frost sparkle, faint glow in deep cracks |
| Metal asteroids | 15-60 m | dense, hammered, veined | thumbprint dents, raised gold veins, rust in the creases |
| Crystal asteroids | 20-50 m | broken, crystalline | big violet or teal crystal clusters erupting from pits, glowing roots and fissures |
| Derelict satellites | 6-15 m | delicate, abandoned | dish, solar wings (one bent and torn), gold foil, faded paint, blinking light, a puncture |
| Cargo pods | 4-10 m | sturdy, utilitarian | gold/teal hazard stripes, reinforced corners, latches |
| Beacon buoy | 5 m | slender, nautical | armoured halo ring with a teal glow strip, warm-white light tip |
| Station truss | 20-60 m | industrial, torn | lattice girders, torn kinked ends, spilled cables, hull cladding |
| Debris clusters | 10-30 m | tangled | bent torn plates, kinked I-beams, pipes, joined into one drifting mass |
| Gate ring | 120 m inner diameter | ornate, monumental | 8 segmented armour sections, 8 teal emitter nodes, crown fins, gold rails |

## Budgets and materials
Asteroids 3k-15k triangles, everything else 2k-25k; Nanite on. One material slot per asset: one baked set
(BC, N DirectX, ORM, E where it glows) at 2048 (1024 for the small pods, buoy and the two smallest rocks).
Emission strength 2 in Blender (never above 3), scaled in Unreal by the material instance.
