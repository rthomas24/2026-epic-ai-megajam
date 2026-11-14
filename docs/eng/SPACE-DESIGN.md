# Space design: the galaxy as a place (ENG-39, space half)

Teammate 6, Nov 14, 01:00 EST. Ryan: space must not be "a bunch of rocks bundled in a pile"; it should read as a real
region with belts and distinct areas, each with its own theme and its own piece of the story. T4 reviews (layout,
keep-outs, wave rings, lanes); the Lead decides. Built through the space field (`World/SpaceField`); every piece stays
destructible; nothing changes inside T4's combat pocket.

## The map (top down, north = +Y, 1 character ≈ 500 m; the flyable sphere is 9 km round the zone)

```
                 · · · · · · · · · OUTER DARK (comets, lone giants) · · · · · · · · ·
        CINDER (-4.2, 3.3)                                          VERDANCE (4.5, 2.6)
           ( C )   ◇◇ SURVEY SCAR ◇◇                 ICE DRIFT ~~~~  ( V )
                 ◇◇ rigs · rubble stream ◇           ~~ teal ice ~~   ⊙ ring
                    ◇◇                              ~~~~       ▫ ▫ QUIET GRAVEYARD
                         ═ ═ ═ ═ SHEPHERD BELT (r 2.0-2.6 km) ═ ═ ═ ═   ▫ transponder
                      ═ ●                                        ● ═     (beacon)
                    ═        ·  · patrol wreckage halo ·  ·          ═
                    ═      ·    ┌─────────────────────┐     ·        ═
                    ═     ·     │ THE WATCHLINE        │     ·        ═
                    ═      ·    │ T4's pocket, ruins   │    ·         ═
                    ═       ·   └─────────────────────┘   ·         ═
                      ═ ●     ·  ·  ·  ·  ·  ·  ·  ·  ·        ● ═     ● = shepherd moonlets
                         ═ ═ ═ ═ ═ ═ ═  gap = Dominion lane ═ ═ ═      (gap edges)
               ▲▲                                              ▲▲
          ▲▲ SHATTERED MOONS: the rim of the Dominion's cleared kill-zone ▲▲
               ▲▲          (nothing inside 3.5 km)            ▲▲
                                   [ DOMINION (0, -5.2) ]
```

## The regions, in the order the player meets them

| # | Region | Where | Silhouette from 5 km | Its piece of the story (no twist) | Density | Fights |
|---|---|---|---|---|---|---|
| 1 | **The Watchline** | The combat zone (T4's pocket and ruin ring), unchanged inside 1.2 km; a halo of patrol wreckage 1.2-1.8 km | A glinting shell of debris around the station ruins | Triad's own patrol line was hit at First Contact: hull plates and cargo of a lost patrol drift round the ruined relay | Pocket as today; halo light (pieces 1-20 m, a few 40 m hulls) | Waves 1-5, the tutorial |
| 2 | **The Shepherd Belt** | A true ring round the zone, radius 2.0-2.6 km, one thin plane (±60 m, tilted 8°), cut by natural gaps where the lanes cross | A bright band across the sky from anywhere in the system | The system's old belt: the reason this was a patrol station. Calm, ordered, Keplerian | Dense band: larger rocks on the inner edge grading to gravel outside; 4 shepherd moonlets (250-400 m) mark the gap edges | Tutorial withdrawal corridor crosses it through the Verdance gap; skirmishes on the edge |
| 3 | **The Ice Drift** | A curved stream of ice from the belt toward Verdance (3-4.5 km), around the convoy lane | A pale blue river pointing at the green world | Convoys stream through it toward Verdance: the machines are searching, not invading | Medium, ice and teal crystal only, long and thin | The Verdance approach |
| 4 | **The Quiet Graveyard** | Beside the transponder off Verdance's orbit, 3.2-3.8 km out, clear of the tutorial plane at 2.5 km | A line of dark hulls lit by one blinking beacon | The research expedition that went quiet: the place the signal device was taken from. Still, cold, no rocks | Sparse: 5-6 old hulls (80-200 m) in a loose line, their spilled debris, buoys | None: the calm before the descent |
| 5 | **The Survey Scar** | The Cinder approach (Cinder side, 3-4.5 km), with a rubble stream trailing toward the planet | Fractured metal and violet crystal rocks, rig lights | Null survey rigs have mined these rocks hollow and haul the yield to Cinder: the search follows the same pattern as Verdance | Medium-high cluster with cut channels (the transports' route kept clear) | Gameplay 04's interception |
| 6 | **The Shattered Moons** | The rim of the Dominion's cleared zone: huge fragments 3.6-4.6 km from the Dominion on its far and side arcs (never the side facing the Watchline) | Moon-sized broken slabs framing the Dominion like a crater rim | Whatever stood near the Dominion was broken and swept aside; nothing is allowed close. Felt, not explained | Few, very large (150-400 m) with gravel in their lee; empty inside 3.5 km | Wave 5 guard line, the assault, the escape flight out |
| 7 | **The Outer Dark** | Beyond 6.5 km in every direction | Lone giants and comet trails against the nebula | The edge of the system: nothing lives here | Sparse lone rocks and ice | None |

## How it is built (the field's generators, replacing the pile-like clumps)

- **Belt**: Keplerian arc. Rocks placed by angle and orbit radius in one plane (Gaussian ±60 m), size gradation by
  radius, density modulated along the arc so it has thicker and thinner stretches; gaps where a lane, gate or return
  point would be crossed (T4's keep-outs, already in `IsPlacementAllowed`); shepherd moonlets at gap edges.
- **Stream**: rocks along a curved spline (start, two controls, end), thin cross-section, size and spacing gradation
  from head to tail; used for the Ice Drift and the Survey Scar's rubble trail.
- **Halo**: a thin shell of wreckage round a centre (the Watchline's patrol wreckage).
- **Graveyard**: a loose line of wrecks with their own debris and buoys.
- **Rim**: few huge fragments on a partial shell, gravel in each one's lee.
- **Outer Dark**: Poisson-disc lone rocks beyond 6.5 km.
- The 30-per-shell random clusters and the two random tilted rings go away; piece counts stay within today's (~5,400
  plus the 731 adopted layout rocks), so the perf budget holds. Every piece stays destructible and solid; the AI's
  obstacles follow the same rules. The Watchline's pocket, lines of fire, wave spawn rings and route lanes stay T4's.

## Set pieces needed (for the structures builder and Grok Bot; distant shells may be AI-made, nothing walkable)

| Region | Piece | Size | Count / variants | Notes |
|---|---|---|---|---|
| Watchline | Patrol hull wreck (Triad-style, broken in two) | 40-60 m | 2 | Triad's colours, burnt; shows the squad lost people here |
| Shepherd Belt | Shepherd moonlet (cratered, rounder than rocks) | 250-400 m | 2 | Reused 4x; one material, no new 2K texture |
| Graveyard | Research-expedition hull (long, old, civilian) | 80-200 m | 2-3 | Distant shells: AI-made acceptable; dark, a few lit windows |
| Graveyard | Transponder beacon's surroundings: a small dish platform | 15-25 m | 1 | Near the existing transponder |
| Survey Scar | Null survey rig (anchored to a rock: legs, drill, lights) | 30-60 m | 2 | Null red / black with white "eyes" |
| Survey Scar | Hollowed rock (a rock with a cut face and a bore hole) | 40-120 m | 2 | Or a material variant on the metal rocks |
| Shattered Moons | Moon fragment (big slab, one flat fracture face) | 150-400 m | 3 | The rim's silhouette depends on these |
| Outer Dark | Comet (ice head + tail effect) | 300 m head | 1 | Already in the plan (system D) |

Until they exist the regions use today's meshes (the kit's rocks scaled, the station truss and satellites for hulls),
so the layout and the captures can be judged tonight.

## Checks

Fixed-camera captures of each region from 5 km, 1 km and inside it, before and after, plus one flythrough of the
journey (start → belt → Ice Drift → Graveyard → Verdance, Verdance → Survey Scar → Cinder, Cinder → Shattered Moons →
Dominion). Frame time inside the densest stretch of the belt with a wave running; the field under 1.0 ms game thread.
