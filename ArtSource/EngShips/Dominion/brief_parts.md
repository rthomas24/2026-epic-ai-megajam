# Brief: Dominion shield generator, generator core and reactor (finale objectives)

Engineering-made (route C, code-built, Blender 5.2 headless), same pipeline and materials as the Dominion exterior
kit (`brief.md`). Original designs. Nicholas keeps art direction.

## Purpose and camera
- **Shield generator** (x4): free-floating Null Armada pylon at radius 388 m around the Dominion, attacked from
  100-1000 m. It must read as "enemy machine with a glowing weak point" as a 15 px speck at 1 km (the cyan cell
  carries it) and as a menacing armoured spire at 100 m.
- **Generator core**: the visible weak point, a cyan-white energy cell in the top of the pylon (shield colour).
  Hidden when the generator dies; the pylon stays (scaled to 40 % height about its centre) as a stump.
- **Reactor**: the final target, exposed in the dorsal trench, attacked at 50-300 m, mostly from above and along
  the trench. Hot amber-white core inside armoured petals and rings, open at the top so the glow reads from above.
  Three damage phases (cue-driven, no mesh change).

## Code contract (do not change the code; the meshes fit it)
All Unreal cm, +X forward, +Z up, mesh origin = actor origin, scale 1.

| Mesh | Placeholder it replaces | Envelope to match | Collision |
|---|---|---|---|
| `SM_Dominion_Generator` (component `Pylon`, ROOT, BlockAllDynamic, damage target) | engine Cylinder x (12, 12, 20): 1200 dia x 2000, centred on origin (Z -1000..1000) | body r <= 600, Z -1000..1000; crown prongs up to ~Z 1700 around the core | UCX: the 12 x 20 m body hull + one hull around the core cradle (Z 700..1700) so shots at the visible weak point hit |
| `SM_Dominion_GeneratorCore` (component `Core`, NoCollision, attached at relative (0, 0, 60) cm; with art its scale becomes 1) | engine Sphere x (0.9, 0.9, 0.5) under the scaled pylon: 1080 x 1080 x 1000 ellipsoid centred at actor Z 1200 | geometry centred at mesh-local Z +1140 (= actor 1200), <= 1080 x 1080 x 1000 | none |
| `SM_Dominion_Reactor` (ROOT, BlockAllDynamic, damage target) | engine Sphere x 25: 2500 dia centred on origin; actor at Dominion (16500, 0, 6500), trench floor at local Z -1500, trench walls' inner faces at Y +/-2300 | main body r <= 1250 about origin; a pedestal down to the trench floor (Z -1500); |Y| <= 1400 | UCX: convex hull of the 25 m sphere + pedestal hull |

## Design
- **Generator: "the lantern spike".** Hexagonal spire, heavier at the top: a downward keel spike (Z -1000), an
  armoured skirt of six forward-raked plates at mid-height, three blade vanes sweeping up and out, an upper shaft
  that flares into a cradle of three claw prongs holding the cell. Focal point: the cell at two-thirds height.
  White eye slits on the shaft between the vanes; one thin red seam ring at the skirt. Palette graphite / near-black
  trim / red + white accents (emissive <= 5 % excluding the cell).
- **Core: "the cell".** Elongated faceted capsule of cyan-white light (0.3, 0.8, 1.0), held by two dark containment
  bands and three vertical ribs (dark trim) so it reads as a machine part, not a ball.
- **Reactor: "the caged sun".** Amber-white core sphere (1.0, 0.7, 0.15), r ~8.5 m, inside six curved armour petals
  with glowing gaps between them, a heavy equatorial clamp band with six clamp blocks, an open crown (oculus) with
  six raked chevron teeth, sitting in a flared hexagonal pedestal cup with four buttresses to the trench floor.
  Focal point: the oculus glow seen from above; gaps make a striped glow from the side.

## Materials
Reuse the kit's material instances (no new textures, zero texture memory): `MI_TD_DominionPanel`,
`MI_TD_DominionTrim`, `MI_TD_DominionGlowRed`, `MI_TD_DominionGlowWhite`, plus two new instances of
`M_TD_DominionGlow`: `MI_TD_DominionGlowCyan` (generator cell) and `MI_TD_DominionGlowReactor` (reactor core).
Slots named `M_Dominion_<Role>` and mapped by name on import.

## Budget
Generator 5-40k tris, core 1-10k, reactor 10-60k; Nanite on all three.
