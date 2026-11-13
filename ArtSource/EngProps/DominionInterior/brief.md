# DominionInterior brief: modular kit for the inside of the Dominion (boarding finale, ENG-29 / ENG-30)

**Purpose and camera.** The level is built in code (T5) from collision boxes on a 400 cm grid; each box is dressed
with one of these meshes BY NAME. Seen on foot in third person (camera about 250 cm up, 400 cm behind) at 2-30 m,
lit by a few lights placed in code, so the kit must read under flat light.

**Story (storyline.md sections 4-6).** The Dominion was built by the squad's own future. The interior is therefore
FAMILIAR MILITARY ARCHITECTURE under Null Armada armour: handrails, floor grating, cable trays, hazard chevrons,
stencilled bay marks (plain geometric bars and dots, never text or logos), bulkhead ribs every 400 cm, a door a
person fits through, consoles at standing height. One recurring, quiet motif: a three-chevron (Triad) panel,
embossed, scuffed and half-covered by a bolted armour plate, on door lintels and the core's base. Nothing says it.

**Shape language.** "Ribbed, armoured, human". Null Armada family from the fleet (ArtSource/EngShips/NullFleet,
Dominion): gloss black armour panels over dark gunmetal structure, thin red-orange emissive strips, cold white
work lights.

**Readability.** Floors mid-dark grey and matte (characters read against them); walls darker and glossier; red
strips at ankle height (20 cm) and head height (190 cm) give depth; only `SM_DI_Cover_Low` looks like cover.

**Palette.** Armour `#16171B` gloss, gunmetal `#33373D`, floor `#43454A` matte, crimson `#4A0A12`, hazard
`#BF541A` on black, stencil `#9EA19A`, strip red-orange (1.0, 0.14, 0.02) as the fleet's `MI_TD_DominionGlowRed`,
work light (0.9, 0.95, 1.0).

**Budget.** 300-2500 triangles a piece (core 6-10k). Two materials for the kit: one shared 2K atlas / trim sheet
(`T_DI_Atlas_BC/_N/_ORM`, 2.56 px/cm, every trim periodic over 400 cm) and one emissive master with a few instances
(red, white, screen, cell, core). The shield shell is the one translucent extra. No Nanite.

**Contract.** Units cm, +X forward, pivot at the bottom centre of the footprint on the walkable plane (Z = 0),
400 cm grid, exact sizes in README.md. Simple box collision inside the stated box on every mesh.

**Owner and folders.** Engineering (Ryan's side) under the take-over rule (AGENTS.md section 11, R-024); sources
`ArtSource/EngProps/DominionInterior/`, Unreal `/Game/TriadDrift/Gameplay/Props/DominionInterior/`. Nicholas keeps
art direction and may restyle or replace any of it. Original work, code-built (route C), no downloads.
