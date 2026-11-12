# Asset routing: which method makes which asset

Agents are strong at precise, modular and procedural geometry: kits, props, rocks, crystals and
parametric parts. They are weak at organic forms, characters, foliage, judging proportion and
"design taste". Route accordingly. Every route still passes the gates and rubric in
`aaa-quality-bar.md`.

## Route table for Triad Drift (from features.md)

| Asset | Route | Notes |
|---|---|---|
| Rift, Lancer, Bastion (hero player ships) | **A** (Rodin) | Code-built versions are proxies/blockouts only. Keep the parts the AI is bad at as code: emissive nozzles, portal ring, sockets, collision |
| Enemy Shard Fighter, Hook Interceptor | **A** or **C** | Many on screen: 5k–20k tris, bold shapes |
| Wall Gunship, Maul Bomber, Hive Carrier | **A** + code-built bays, turrets and doors | Animated parts (bay doors, turrets) as separate meshes |
| Dominion exterior | **A** hero pieces + **C** modular hull panels and greeble tiles, Nanite | Kitbash at scale; shield generators and turrets as separate actors |
| Dominion interior corridor kit | **C** (grid-modular) + trim sheet | 400 cm grid; see `hard-surface.md` |
| Consoles, crates, doors, pickups, wreck core, charges, jammer pieces | **C** | |
| Energy blade (hilt + blade) | **C** | Blade is a separate emissive mesh (not Nanite) |
| Asteroids, rocks, crystals | **C** (`procedural_rocks.py`) | Nanite for big ones |
| Verdance flora (giant mushrooms, fern trees), creature critters | **A** or **B** (Fab, Poly Haven) + GN scatter in Blender or UE foliage | Organic: don't hand-code |
| Crew enemies (humanoids), first-person arms | **B**: rigged marketplace or Mixamo characters, or Rodin T-pose + auto-rig | Never code-model people |

## Route A: concept → AI image-to-3D → cleanup → polish
1. **Concept image** (human makes it, or agent writes the prompt). Prompt template:
   > "Concept art of a [role] spaceship for a bright stylized sci-fi game, [3 shape-language words],
   > [signature feature], [palette], three-quarter front view, single object centred, full object in
   > frame, plain light-grey background, no text, no logos, clean hard-surface design, studio lighting"

   Keep the background plain, show the whole object and use no motion blur. Image-to-3D works far
   better on clean product shots. Make a side view too if the tool supports multi-view input.
2. **Generate** (the human runs it in the tool's web app unless they explicitly ask the agent to
   drive a browser).
   - **Hyper3D Rodin Gen-2** (first choice: team has Business credits, redeem by **Nov 16**, they
     expire **Nov 20**; Rodin use enters the **$10k prize pool**, so list "Hyper3D Rodin" in "Tools
     you used"):
     - Image-to-3D with material **PBR**. Never "Shaded", which bakes lighting into the texture.
     - Mesh mode **Quad** for clean hard surface (presets 50k / 18k / 8k / 4k), or **Raw** when you
       will re-bake anyway.
     - Hero ships: 18k–50k.
     - Export **GLB**.
   - **Tripo**:
     - model v3.1, `pbr` on, texture quality detailed. Keep `delight` on (needs texture v3.5).
     - `quad` on for hard surface (10k–50k).
     - Set `export_orientation` deliberately: a wrong orientation still reports success.
     - Free-tier rights are contradictory, so use a paid plan for anything shipped.
   - **Meshy**:
     - Meshy 7.1, `topology` quad, `target_polycount` about 30k.
     - `enable_pbr` and `remove_lighting` on.
     - `save_pre_remeshed_model` gives the dense original as a bake source.
     - The free plan is CC BY 4.0: attribution required.
   - **Hunyuan3D 2.1** (self-hosted): **not usable by EU / UK / South Korea residents** under its
     licence. Output is unit-less (normalised); rescale it.
3. **Save the original** to `ArtSource/.../<Asset>/source/` with `SOURCE.md` recording tool, plan,
   date, input image and prompt, and licence. Add a CREDITS.md line.
4. **Clean up** with the `blender-ai-mesh-cleanup` skill: scale to the brief's size, weld, decimate
   or remesh to budget, UV, and re-bake the source look.
5. **Polish** (code is great at this): exact mirror for symmetric hulls; replace mushy nozzles,
   intakes and canopies with crisp `hardsurface.py` parts; add emissive inserts, sockets and
   collision. Then texture and export.
6. **Gate:** blockout-equivalent silhouette check on the AI mesh before polishing. Regenerate
   rather than polish a bad generation.

## Route B: licensed libraries and kitbash
- Fab (check each listing's licence; the Fab Standard License allows use in games), Poly Haven
  (CC0), Poly Pizza (CC0 / CC-BY per model), Kenney (CC0).
- Sketchfab only with an explicit CC-BY or CC0 licence. Never NC or ND, never "editorial", and never
  anything resembling a franchise's ships or characters (jam rules forbid others' IP).
- Record every asset in CREDITS.md (name, author, URL, licence). Clean it up and re-style it to
  Triad Drift's palette before use.

## Route C: code-built
Only for the asset types the table assigns to C. Still needs a brief, a concept or written design,
and the gates. Use the kits (`hardsurface.py`, `procedural_rocks.py`). If two review rounds fail to
lift the score, re-route to A or B instead of iterating forever.

## Jam rules that apply to every route
- Third-party assets must be licensed and **listed in the submission**.
- AI tools go in "Tools you used" (needed for partner prizes).
- Anything made before Nov 12 must be declared.
- No other people's IP.
