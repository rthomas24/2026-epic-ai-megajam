# The AAA quality bar (what "done" means)

"AAA" here means **stylized, bright sci-fi with shipped-game polish**, not photorealism. A
screenshot should look *designed*: a strong silhouette, believable construction, controlled detail,
clean light-catching edges, rich but disciplined colour, and zero technical defects. If a gamer
would call it "programmer art", it fails, however clean the mesh is.

## Design principles, in the order to apply them

1. **Silhouette first.** It should be recognisable as a solid black shape at 64–128 px from the
   top, side and 3/4 views. The three squad ships must be distinguishable by silhouette alone:
   - **Rift:** crescent wings and the nose portal ring.
   - **Lancer:** needle fuselage and forward-swept wings.
   - **Bastion:** wide, heavy hull with twin missile pods and a dome emitter.

   Enemies must read as a different family from the squad.
2. **Shape language carries faction and story.**
   - **Squad (heroic, agile):** flowing lines running nose to tail, rounded-chamfer hard surface,
     one signature element per ship, a sense of motion even at rest.
   - **Null Armada (menace):** blade-like angular plates, forward-raked spikes, layered black armour
     with red seams, glowing white "eye" lights, heavier at the front.
   - **Verdance:** organic, rounded, flowing. **Cinder:** vertical, crystalline, sharp.
     **Dominion interior:** brutal ribbed industrial, giant lenses watching.
3. **Hierarchy and proportion.**
   - **Primary masses** (1–3) define the read; **secondary forms** (4–10) are the functional parts;
     **tertiary detail** (seams, vents, bolts, greebles, decals) comes last.
   - Contrast big, medium and small at roughly 1 : 0.4 : 0.1. Never make a row of equal-sized parts.
   - Put the focal point (cockpit, weapon, signature feature) about a third of the way along the
     main axis, not dead centre.
4. **Rest areas versus busy areas (~60/40).**
   - Large calm planes carry paint and livery. Detail clusters where function lives: engines,
     intakes, joints, hardpoints, cockpit rims.
   - Detail density scales with viewing distance: an enemy seen at 300 m needs bold shapes, not
     2 cm greebles.
5. **Construction logic.**
   - Every part visibly attaches through overlap (2–5 cm), pylons, flanges or struts.
   - Intakes face forward and exhausts face aft. The cockpit has sightlines. Landing gear has bays.
   - Panel seams follow the form: along curvature, around openings, at changes of material. Never
     place them at random.
6. **Edges and shading.**
   - Every hard edge gets a chamfer so it catches light. Bevel width is about 0.5–1% of the part
     size, with 2–3 segments, harden normals and weighted normals.
   - Curved surfaces have no faceting, and there are no smoothing artefacts, no pinching and no
     shading seams.
7. **Colour and materials.**
   - **60/30/10:** dominant paint, secondary colour, then accent (trim and emissive). Emissive
     covers no more than about 5% of the surface and leads the eye: engines, cockpit, weapon tips,
     faction lights.
   - **Roughness has range:** paint 0.35–0.5, bare metal 0.2–0.35, rubber or dark composite
     0.6–0.85. Metallic = 1 only on bare metal; painted panels stay around 0.3 metallic so colour
     stays vivid in Unreal.
   - **AAA cues:** livery stripes, unit numbers and hazard markings (decals or texture), subtle edge
     wear and cavity dirt from baked curvature and AO. No pure black or white surfaces.
8. **Scale cues.**
   - A 6–9 m fighter has a canopy about 1 m wide. Hatches are about 0.8 m, vent slats 5–10 cm
     apart, warning stripes 10–20 cm. Corridor doors are 2 × 3 m.
   - Known-size details are what make an asset read at the right scale in the game.
9. **Technical health.**
   - It meets the tri budget and has clean UVs. `asset_report.py` shows no errors and
     `assembly_check.py` passes.
   - Naming, pivot and collision follow the `blender-unreal-export` skill.

## Instant fails ("programmer art" tells)
- Built from untouched primitives of uniform sizes (box hull, cylinder engines, plank wings).
- Floating or barely touching parts, or symmetric everything with no focal point.
- Greebles sprinkled everywhere with no rest areas, or panel lines that ignore the form.
- One flat colour per part, every material at the same roughness, or a black-and-white palette.
- Glow everywhere, or emission so strong it renders white.
- Razor edges with no bevel, or blobby over-rounded bevels.
- Wrong scale cues (a giant cockpit, door-sized vents), or a silhouette that reads as a blob from
  the game camera.

## Review rubric: score 1–5 per category

| Category | 1: toy | 3: decent indie | 5: AAA |
|---|---|---|---|
| Silhouette | generic blob or boxes | readable, a bit generic | iconic; recognisable as a black shape; unique to its faction |
| Proportion and hierarchy | equal-sized parts, no focal point | clear primary form, flat secondary rhythm | strong big/medium/small rhythm with a clear focal point |
| Detail quality and distribution | none, or random noise | some purposeful detail, uneven | functional detail clustered with calm rest areas, density matched to camera distance |
| Construction believability | floating parts, no logic | attached, a few odd joints | every part mounted and functional; seams follow form |
| Edges and shading | faceted or razor edges, artefacts | bevels present, minor artefacts | crisp chamfers catch light everywhere, flawless smoothing |
| Materials and colour | flat single values | palette right, little variation | 60/30/10, roughness range, emissive leads the eye, livery or wear cues |
| Style fit | off-palette, wrong faction language | mostly on brief | unmistakably Triad Drift: bright, saturated, faction-true |
| Technical health | errors, no UVs, over budget | warnings only | clean report, assembly OK, on budget, export round trip passes |

**Ship thresholds:** at least 4 in every category for gameplay assets. Hero assets (player ships,
the Dominion) need at least 4 everywhere and an average of 4.5 or more. Final sign-off belongs to
the human.

## Reviewer prompt template (fresh subagent or fresh chat)

> You are an art director for a stylized, bright sci-fi game ("Triad Drift", Unreal Engine 5).
> Review ONE asset. You have: the brief (below), the concept or references (paths), the latest clay
> and material contact sheets (paths; cells in this order: front 3/4, front, right / top, back 3/4,
> low 3/4), and the numeric reports (below). Score each rubric category 1–5 using the anchors
> provided, citing what you see. Then list the THREE highest-impact fixes, each as a concrete
> modelling or material action with a location on the asset ("the wing roots read as
> planks: add a 15 cm root fairing that blends into the hull"). Don't praise and don't hedge. If a
> category can't be judged from the images, say which extra view you need.
> [brief] [rubric table] [report numbers] [image paths]

Run the fixes, re-render the same views, and give the next round to a new reviewer. Revert to the
last saved version if the scores drop.
