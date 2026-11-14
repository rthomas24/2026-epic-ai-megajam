# Flight HUD concept: squad panel, radar, cluster (Nov 14)

Design document for the Lead. It builds on the ENG-36 HUD that is already on `main` (`UTDFlightHUDWidget`, spec
`docs/eng/hud-redesign.md`) and answers Ryan's request: the three ship cards should show health and power-ups, a
small radar in a corner should show nearby enemies and what is coming, and the HUD should feel more like a spacecraft
shooter's. **Stage: design only.** Nothing here is built, tested or played.

| File | What it is |
|---|---|
| `mockup.html` / `mockup.png` | **Concept A (recommended)**: squad panel bottom-left, radar bottom-right. Shown in combat as Rift: missile warning, a locked target, Overcharge active, a wingman under fire. |
| `mockup_b.html` / `mockup_b.png` | **Concept B**: squad stacked on the left, radar bottom-centre. Shown at the moment of a switch to Lancer (name card), with a wingman down and respawning. |

The mockups use the HUD's existing colour tokens (`FTDFlightHUDTokens`) and the existing enemy marker shapes, so
the build changes as little as possible. Both are 1920x1080 and use only inline SVG and CSS.

---

## 1. What exists today (read from the code and captures)

`UTDFlightHUDWidget` paints everything in one `NativePaint` in a 1920x1080 design frame, from an `FView` snapshot
sampled every tick from `ITDHUDData` (the squad manager). It already draws:

- **Reticle cluster:** hull arcs (r 44, thick) and shield arcs (r 53, thin), signature power pip (C+64,56), ability
  pip, second-power pip, ammo, heat arc, boost bar with dodge notch, hyperdrive bar, Overcharge timer ring, Switch
  Strike ring, hit-direction arcs, low-hull vignette, missile warning tag.
- **Squad strip** (bottom-left, 1.3x): three chips with keycap, ship glyph (crescent, dart, hexagon), thin shield and
  hull mini-bars, a power-ready dot, a "!" trouble badge under 35% hull, cross and respawn seconds when down. Only the
  flown chip shows a name.
- **Objective pill** top-centre with an "N LEFT" counter; it expands for 3 s on change; Dominion generator pips.
- **World markers:** target bracket with health bar and distance, lock diamond, lead ring, hostile type markers
  (fighter chevron, interceptor dart, bomber wedge, gunship diamond, carrier hexagon), edge chevrons for hostiles,
  missiles and wingmen, objective diamonds with distance, pickup markers, the approach marker, transponder.
- **Lanes:** one lower-lane message (prompt, hyperdrive block, hint card), subtitles at y 900, pickup collect toast,
  boundary warning, Director's Cut handling (CUT, slate, money shot, pause dimming).

What the captures show (`t2/.../play1110/swarm/03.png`, `t6/.../region3_route.png`): the HUD is clean and readable,
but the squad strip is small (bars are hairlines with no numbers), wingman power state is a single dot, there is
no way to see what is around or behind you except edge chevrons, and nothing says what is coming next. The thin
strokes without backing plates lose contrast over the bright cyan and magenta nebulae.

**So this concept keeps the reticle cluster, the world markers, the lanes and the Director's Cut behaviour as they
are, and changes three things:** the squad strip becomes a squad panel, a radar is added, and the whole HUD gets a
"cockpit" treatment (backing plates, angled corners, monospaced numerals, a soft glow on live elements).

---

## 2. Research summary

Conventions found in arcade and simulation space shooters, and why they exist. Games are named only as factual
references; nothing here copies any game's layout, art, icons, fonts or names.

| # | Convention | Why it works | Source |
|---|---|---|---|
| 1 | **Circular scope centred on the player, forward up**, with stems from each blip down to the player's plane to show above or below (the long-standing "3D planar radar" of space sims). | A flat circle reads at a glance; the stem adds the third axis without a second display. | Pioneer and community descriptions of the Elite-style planar radar [summary]; Black Hole Incident devlog [summary] |
| 2 | **The radar simplifies as range grows:** small things (fighters, missiles) drop out at long range, big things (planets, fields) become a single icon, asteroid fields become one area, not many blips. | Keeps the scope readable instead of a dot storm. | Howling Moon Software, "Radar and HUD work" [read] |
| 3 | **Off-scope contacts stick to the rim** instead of vanishing, so direction is kept. | Players lose objectives that silently drop off the map. | bugnet.io, minimap edge clamping; Godot minimap recipe [summary] |
| 4 | **Corner elements cost eye travel**; eye-tracking shows players bouncing between minimap and ammo. Fast-changing data belongs near the centre; static or slow data can live in corners. | Foveal vision is on the reticle; every glance away is time not aiming. | Game Developer, "My personal crusade against mini-maps and other corner-based HUD elements" [read] |
| 5 | **Party / wingman frames:** one compact frame per member, shield shown as a separate (usually blue, thinner) bar over health, ability availability as a lit / unlit diamond or a greyed icon with a number. | Lets a leader judge who needs help and who can act without opening anything. | Warframe mission interface docs; Smite and Dissidia HUD references; UEFN squad view docs [summary] |
| 6 | **Target states as one shape language:** open bracket = targeted, solid / diamond = locked, a separate small ring ahead of the target = lead; incoming-lock warnings have a sound as well as a symbol. | One family of shapes is learned once; audio catches what the eye misses. | Vega Strike HUD manual; Space Engineers combat guide; itch.io player feedback on lock warnings [summary] |
| 7 | **Sci-fi "FUI" feel** comes from thin strokes rather than filled boxes, concentric rings and radial gauges, monospaced numerals, a restrained palette (one data colour, amber for caution, red for critical) and minimal decoration. Overuse of glow and thin low-contrast lines is the usual failure. | The look reads as "instrument", and the restraint keeps it legible. | FUI / sci-fi interface pattern write-ups (daisyUI trends, sci-fi design pattern guides) [summary] |
| 8 | **Never colour alone**: health bars, minimaps and objective markers are the usual offenders. Add shape, icon or text. | Colour-blind players (about 1 in 12 men) otherwise lose the information. | Game Accessibility Guidelines (basic); Xbox Accessibility Guideline 103 [summary] |
| 9 | **Contrast:** 4.5:1 for normal text, 3:1 for large text and graphics; a solid or translucent plate behind text and outlines are the recommended fixes over busy backgrounds. | Bright nebulae are exactly the busy background the guideline warns about. | Xbox Accessibility Guideline 102 [summary] |
| 10 | **Text size:** a readable default plus a scale option; a "28 px at 1080p" minimum is quoted for Xbox XAG 101 on secondary sites only (unverified). | The jam build is judged on PCs and laptops, often at 720p in a window. | Xbox / Game Accessibility Guidelines [summary] |

Sources:
- Howling Moon Software, Radar and HUD work: https://howlingmoonsoftware.com/radar-and-hud-work
- Game Developer, My personal crusade against mini-maps and other corner-based HUD elements: https://gamedeveloper.com/design/my-personal-crusade-against-mini-maps-and-other-corner-based-hud-elements-in-immersive-games-
- Pioneer Space Sim, Flight UI (bot-protected; used through search summary): https://wiki.pioneerspacesim.net/wiki/Flight_UI
- Black Hole Incident devlog #4 (spherical radar placement): https://introvertedgames.itch.io/black-hole-incident/devlog/466095/major-ui-improvements-devlog-4
- bugnet.io, Minimap icons not clamping to the edge: https://bugnet.io/blog/how-to-fix-minimap-icons-not-clamping-to-the-edge-for-offscreen-targets
- Godot Recipes, Minimap: https://kidscancode.org/godot_recipes/4.x/ui/minimap/index.html
- Warframe support, Mission Interface: https://support.warframe.com/hc/en-us/articles/38801911653517-Mission-Interface
- Epic, Making a custom squad view (UEFN): https://dev.epicgames.com/documentation/en-us/fortnite/making-a-custom-squad-view-in-unreal-editor-for-fortnite
- Vega Strike manual, HUD: https://wiki.vega-strike.org/Manual:HUD
- Space Engineers, Warfare combat guide: https://spaceengineersgame.com/new-players/warfare2-combat-guide
- daisyUI trends, FUI / HUD: https://trends.daisyui.com/trend/fui-hud/
- Xbox Accessibility Guideline 103: https://learn.microsoft.com/en-gb/gaming/accessibility/xbox-accessibility-guidelines/103
- Xbox Accessibility Guideline 102 (contrast): https://devdocs.xbox.com/gaming/accessibility/xbox-accessibility-guidelines/102
- Game Accessibility Guidelines, basic: https://gameaccessibilityguidelines.com/basic/

The ENG-36 spec's own sources (section 8 of `docs/eng/hud-redesign.md`) still apply and were not repeated.

---

## 3. Design goals

1. **The squad is the hero.** At a glance the player knows each ship's hull, shield and whether its power is ready,
   so switching is a decision, not a guess. (Protects the core mechanic.)
2. **See around you, and see what is coming.** A radar shows the fight in all directions, and the next wave's arrival
   point and timer are visible before it lands.
3. **The centre stays clean.** Nothing new goes inside the reticle cluster; fast-changing data stays near the
   centre, slow data goes to corners (finding 4).
4. **Cockpit feel without clutter:** thin glowing strokes, angled plates, monospaced numerals and one shape language;
   steady-combat coverage stays under 8% of the screen.
5. **Readable over any nebula and for colour-blind players:** every element on a dark plate or with an outline, and
   every colour meaning carried by a shape or word as well.

---

## 4. Layout at 16:9 (Concept A, recommended)

All values in the 1920x1080 design frame the widget already uses (`SafeX` 96, `SafeY` 54); % of screen width x height.

```
 x: 0        96                 720            1200                 1824  1920
 y:0 +--------------------------------------------------------------------+
     |                                                                    |
  54 |                  [ OBJECTIVE PILL  480x30, top-centre ]            |
     |                  [ (expands to 720x72 for 3 s)        ]            |
     |  edge chevrons on the safe frame (hostile / missile / wingman / obj)|
     |                                                                    |
 444 |                       [ MISSILE tag 150x28 ]                       |
 478 |               [ RETICLE CLUSTER 224x140 around C ]  [PICKUP TOAST] |
     |               arcs, pips, overcharge ring, boost arc  214x44       |
 612 |               [ ship zone 340x188: kept clear ]                    |
     |                                                                    |
 700 |                                              [ WAVE HEADER 280x34 ]|
 746 |                                              [                    ]|
 858 |        [ SUBTITLE  max 880x44, y 858 ]       [   RADAR  d 280     ]|
 898 | [flags]                                      [   centre 1684,886  ]|
 926 | [ FLOWN CARD 320x100 ][ WING 232x76 ][ WING 232x76 ]               |
1026 | [  bottom-left, x 96..904                  ]  [                    ]|
1080 +--------------------------------------------------------------------+
```

| Region | Anchor | Position and size (px) | % of screen (w x h) |
|---|---|---|---|
| Objective pill | top-centre | 720,54 · 480x30 (expanded 720x72) | 25 x 2.8 |
| Missile warning | centre | 885,438 · 150x28 | 7.8 x 2.6 |
| Reticle cluster | centre (960,540) | 848..1041 x 478..613 | 10 x 12.5 |
| Pickup toast | centre-right of cluster | 1062,520 · 214x44, 1.4 s | 11 x 4 |
| Ship zone (clear) | centre | 790..1130 x 612..800 | 17.7 x 17.4 |
| Subtitle | bottom-centre | centred, y 858, max 880x44 | 46 x 4 |
| Squad panel | bottom-left (safe frame) | 96..904 x 898..1026 (with flag row) | 42 x 11.9 |
| Wave header | bottom-right, above radar | 1544..1824 x 700..734 | 14.6 x 3.1 |
| Radar | bottom-right (safe frame) | centre 1684,886, r 140 (box 280x280) | 14.6 x 25.9 |
| Edge chevrons | safe frame | as today | – |

Coverage by bounding boxes in steady combat: cluster 1.3%, squad panel 3.7%, radar + header 4.2%, objective 0.7% =
**about 9.9%** (the boxes overstate the radar circle, which is 79% of its box, and the ring cluster). Measured on the
real area of plates and strokes it is about 7%. That is above the ENG-36 3.2% figure, deliberately: Ryan asked for
more information. Two settings bring it back down (section 11).

**Why bottom-right for the radar:** the bottom-left is where players have already learned the squad; the bottom-centre
is where the chase-cam ship, boost bar, lower lane and subtitles live; the top corners are where edge chevrons and the
objective waypoint usually sit. Bottom-right keeps the radar one short glance from the squad panel (the two things
you read together before a switch) and leaves the centre column clear.

### Concept B (alternative)

Squad cards stacked vertically on the left (96..416 x 766..1026, order 1-2-3 top to bottom, the flown card wider),
radar bottom-centre (centre 960,926, r 100, just below the ship zone), the wave timeline folded into a two-line
objective pill, subtitles bottom-right (1300..1824 x 962..1026). Shorter eye travel to the radar (finding 4), but the
radar sits under the chase-cam ship and the boost / hyperdrive bars and the lower-lane prompts must move up into the
ship zone, and the vertical stack eats more of the left edge where hostile chevrons appear.

---

## 5. Squad panel

Three cards in fixed order 1, 2, 3 (Rift, Lancer, Bastion); the order never changes. Replaces the chips in
`LayoutChips` / the "strip" box.

### 5.1 Card anatomy

| Part | Flown card (320x100) | Wingman card (232x76) |
|---|---|---|
| Plate | `Plate` #0A0F1C at 82-88%, top-right corner cut 14 px, 1.5 px border in the ship colour, 3 px top bar in ship colour with a 6 px glow | Same plate, 1 px border Neutral at 22%, 2 px top bar in ship colour at 70% |
| Keycap | 24 px rounded square with "1/2/3" (pad: the switch button glyph) | 20 px |
| Ship glyph | 22 px crescent / dart / hexagon in ship colour (existing glyphs) | 18 px |
| Names | Pilot 19 px bold caps ("WREN"); ship and role 12 px dim ("RIFT · TRICKSTER") | Pilot 14 px bold caps |
| Shield bar | "SHD" 11 px tag; 180x6, `Shield` #9FD8FF, 10 segments (2 px gaps); number 14 px mono | 170x4, number 12 px |
| Hull bar | "HUL" tag; 180x12, Neutral above 50%, Bastion amber to 25%, Danger below (existing `Hull()` band), 10 segments; number 14 px mono in the bar colour | 170x10, number 13 px |
| Damage ghost | A white segment shows the hull just lost and drains over 400 ms (ENG-36 motion rule) | Same |
| Signature power | 32 px ring at the right: **ready** = solid ring in the ship colour, 18% fill, glow, glyph "Q" at 100%; **cooling** = grey track with a ship-colour arc refilling clockwise, glyph at 50%; the power's short name under it (PORTAL, RAM, SWARM), 10 px | 16 px dot: ready = filled ship colour with glow; cooling = arc refilling |
| Unlocked ability | 24 px ring under the power: **locked** = dashed dim ring with a small padlock (shown on cards only, never at the reticle); **ready** = white ring with "E" and glow; **cooling** = refilling arc | 14 px: dashed + padlock / white ring with a dot / arc |
| Status flags | A row of tags above the card (see 5.2) | Same, max 2 |

Numbers are shown on every card (Ryan asked to see health). They use tabular monospaced numerals so they do not jitter.

### 5.2 Status flags

Tags 20 px high above the card, icon + word, left to right in priority order; at most 3 on the flown card, 2 on a
wingman. Each has a shape so colour is never the only signal.

| Flag | Shown when | Look |
|---|---|---|
| UNDER FIRE | The ship took damage in the last 1.5 s | Red plate, warning triangle, "UNDER FIRE"; the card border turns amber and pulses at 1 Hz if hull < 35% (the existing trouble rule, which keeps its "!" badge) |
| DOWN | `bAlive` false | Card goes dark red, glyph crossed, bars replaced by "DOWN · BACK IN 0:06" and a respawn ring |
| CLOAKED | Rift's Cloak active | Magenta outline tag, eye-with-slash icon; the whole card at 60% opacity |
| DOME UP | Bastion's shield dome active | Amber tag with a half-circle icon; wingmen inside the dome show a small dome icon on their own card |
| OVERCHARGE 6.2 | Overcharge pickup active (flown ship) | Gold-white tag with a turned square, seconds counting down and a thin draining underline |
| AFTERBURN / BOOST | Afterburner ram / boost in use | Ship-colour tag with a double chevron |
| SWITCH STRIKE | The Switch Strike window is open (`GetSwitchStrike01` > 0) | On the card you should switch to: ship-colour tag with a lightning-style zig-zag, draining |

### 5.3 Behaviour on switch (Director's Cut)

On `OnShipSwitched`, in the same frame: the old flown card shrinks to wingman size and the new one grows (120 ms
ease-out, positions slide; the order never changes); the reticle cluster recolours instantly. The Director's Cut name
card ("CUT TO · CAM 2" / "KATO · LANCER", 360x50 at y 758, ship-colour bar on its left, 0.9 s) plays as today, and
the new card's top bar flashes with the same wipe so the eye links the two. Switching stays instant; no input is
blocked.

---

## 6. Power-ups and pickups

- **Announce:** on collect, the existing toast (`PickupToast`) becomes a 214x44 plate right of the reticle cluster:
  coloured left bar, the pickup's shape icon (plus, disc, double point, turned cube, arrowhead, matching
  `ETDPickupType` and its `ColorOf`), the word in 16 px caps and one line of what it did ("+25 · 34 → 59",
  "Damage x1.5 · 8 s"). It shows for 1.4 s, then slides 40 px toward the squad panel and fades: the timed ones land
  as a status flag on the flown card.
- **Active, timed (Overcharge):** the gold dashed ring around the reticle that already exists (r 62, draining), plus
  the OVERCHARGE flag with seconds on the flown card. **Shield** on wingmen ("half on the wingmen") also flashes
  their shield bars white once.
- **Power ready (pickup):** the power ring on the flown card and the power pip at the reticle play the existing
  ready pulse (1.0 → 1.25 → 1.0 in 300 ms).
- **Boost:** the boost meter moves from a separate bar into a 40° arc under the reticle (r 70), with the dodge notch
  at its right end; it shows while boosting or refilling and hides 1.5 s after full (unchanged rule). "+BOOST" ticks
  beside it when a broken asteroid tops it up (existing `BreakBoostAge`).
- **Pickups in the world** keep their markers; within radar range they also show on the radar as small coloured
  icons of the same shapes.

---

## 7. Radar

### 7.1 Placement and frame

Bottom-right, centre (1684, 886), radius 140 (concept A). Forward is up; the scope rotates with the flown ship's yaw
(heading-up), and above / below is relative to the ship's own up vector. Plate: radial fill #0E1830 → #070B16 at
78-86%, 1.5 px Neutral rim at 35%; cardinal ticks on the rim; a faint 70° wedge (6%) shows the camera's forward view,
so the player can match blips to the screen.

### 7.2 Range

Two inner rings at 1/3 and 2/3. Default range **1.5 km** (the waves fight inside about 1 km; `PickupMarkRange` is
800 m), shown in a 12 px mono label on the bottom of the rim. Auto-zoom option: 1.0 km in a fight with 5+ hostiles
within 600 m, 3 km in quiet travel; the label changes and the rings ease over 300 ms. Mapping is linear inside 2/3 of
the radius and compressed (log) in the outer third, so close fights get the space.

### 7.3 What is drawn

| Contact | Glyph (all original simple shapes) | Colour |
|---|---|---|
| You | Filled dart, 16 px, at the centre, with a glow | Flown ship colour |
| Wingmen | Hollow dart 12 px, rotated to their heading | Their squad colour; a 1 Hz amber ring when in trouble; a cross in their colour where they went down |
| Fighter | Small chevron 10 px | Danger #FF4A3D |
| Interceptor | Narrow dart 7x12 | Enemy orange #FF5A36 |
| Bomber | Wide wedge 16x9 | Enemy orange |
| Gunship | Diamond 14 px | Enemy orange |
| Carrier | Hexagon 16 px with a dot | Enemy orange |
| Other hostile | Small triangle | Danger |
| Current target | The hostile's glyph inside a 12 px ring | Warning yellow ring |
| Incoming missile | 4 px white dot with red outline and a short trail, flashing at 2.5 Hz | White / Danger |
| Objective | Hollow diamond with a dot | Neutral |
| Portal gate | Ellipse with an inner ellipse | Violet #B48CFF (new token) |
| Planet / landing beacon | Ring with a centre dot | Mint #7CFFB0 (new token) |
| Pickups | Their own shapes, 8 px | `ATDPickup::ColorOf` |
| Asteroid field | Stippled area, no blips | Neutral at 30-45% |

These are the same shapes as the existing on-screen hostile markers (`KindOf`: fighter chevron, interceptor dart,
bomber wedge, gunship diamond, carrier hexagon), so the player learns one set.

**Above / below:** each contact has a stem from its blip to a 1.6 px "foot" dot on your plane; the stem's length is the
height difference (capped at 18 px). Above you: blip at full brightness, stem hangs below it. Below you: blip at 55%,
stem rises above it. Level (within 50 m): no stem. Shape (stem direction) plus brightness, never colour alone.

**Off-scope contacts** are pinned to the rim at their bearing (radius r-8), drawn hollow: the carrier you have not met,
the objective, the planet beacon. Fighters and missiles beyond range are **not** pinned (finding 2), so the rim does not
fill with noise; the edge chevrons on screen still cover nearby off-screen threats.

### 7.4 What is coming up

- **Wave header** (280x34 plate above the radar): "WAVE 3 / 5", five pips (cleared = white, current = red, to come =
  hollow), and when known a red tag "+4 0:04" (enemies arriving and seconds).
- **Arrival on the rim:** when a warp-in starts (`ATDWaveDirector::OnWarpInStarted(Location, EnemyId)`, 1.5 s of
  telegraph) a red 5 px arc lights on the rim at that bearing, 30° wide, with an inward arrow, pulsing at 2 Hz until
  the enemies exist. In a breather (`FTDWaveInfo::bInBreather`, `BreatherRemaining`) the header shows "NEXT WAVE 0:12"
  and, if the director can tell us the next spawn points ahead of time, the arc shows early in a dashed style.
- **Carrier launches** show as a short red arc at the carrier's bearing.

### 7.5 Limits

At most **48 blips** (closest first; missiles and the current target always included), 8 pinned rim contacts, 1 field
texture per field. Blips that would overlap within 4 px merge into the larger one with a "x3" 10 px mono count.

---

## 8. Flight cluster around the reticle

Unchanged from ENG-36 except where noted. Centre stays clear: nothing new inside r 40.

| Element | Where | Change |
|---|---|---|
| Hull / shield arcs | r 44 / 53 | Keep. |
| Signature power pip | C+(64, 56) | Keep; adds the power's 10 px name under it for the first three uses (existing hint rule). |
| Ability pip | C+(-64, 56) | Keep. |
| Heat arc | r 30, bottom 60° | Keep (amber ≥ 60%, red + "HOT" ≥ 90%). |
| **Boost arc** | r 70, bottom 40° (936..984 x 600..610) | **New position**: the boost bar moves from y 792 under the ship to an arc in the cluster, so speed / boost is read where the eye is. Dodge notch at its right end. |
| Overcharge ring | r 62, gold dashed, draining | Keep. |
| Speed | none | Still no speed number (speed is felt; ENG-36 audit). Optional: 6 thin speed ticks on the boost arc that light with `GetSpeed01()`. |
| Hyperdrive | bar 240x6 at y 806 while charging | Keep; in concept A the subtitle sits above it at y 858. "HYPERDRIVE READY" 14 px under the objective pill when the jump is available out of combat. |
| Lock / missile warning | tag at y 444, steady (Tracking) or 2.5 Hz (Imminent) | Keep; add a 4 px red tick on the radar rim at the missile's bearing. |
| Hit-direction arcs | r 150 | Keep. |

---

## 9. Objective and comms

- **Objective pill** stays top-centre (720, 54), 480x30, now with a 14 px angled right end and the counter in mono
  numerals. Concept B adds a second line with the wave timeline; concept A keeps that above the radar.
- **Subtitles / transmissions** move from y 900 to y 858 in concept A (so they clear the 100 px flown card), max 880
  wide on a 62% plate; speaker tag in the pilot's colour. In B they go bottom-right.
- **Lower lane** (prompts, hyperdrive block, hint card) stays centred at y 826..906; in A it shares the column with the
  subtitle: when both are up, the prompt wins and the subtitle moves up 48 px.

---

## 10. Colour and type

| Token | Value | Use |
|---|---|---|
| Neutral | #E8EEF7 | Text, reticle, hull above 50% |
| Dim | #8A94A6 | Tracks, tags, disabled |
| Rift / Lancer / Bastion | #FF4FD8 / #2FE6FF / #FFB13B | Only that ship's own elements |
| Shield | #9FD8FF | Shield arcs and bars (always the thinner one) |
| Warning | #FFE14A | Lock, target ring; always with a diamond or triangle |
| Danger / Enemy | #FF4A3D / #FF5A36 | Hostiles, missiles, critical hull, under-fire |
| Overcharge | #FFE7A0 (new) | Overcharge ring, tag, toast |
| Gate | #B48CFF (new) | Portal gates on the radar |
| Beacon | #7CFFB0 (new) | Planet / landing beacons on the radar |
| Plate | #0A0F1C at 62-88% | Every plate |

- **Type:** Roboto (ships with the engine) for words, caps with +1.5 px tracking; a monospaced face for every number
  (Roboto Mono is Apache 2.0; the engine also ships DroidSansMono). Tabular numerals so values do not jitter.
- **Minimum sizes at 1920x1080:** 11 px for bar tags (SHD / HUL) only, 12 px for mono numbers, 14 px for names and
  labels. Scaled by `height / 1080`, so at **1280x720** these become 7.3 / 8 / 9.3 px, which is too small: **the HUD
  scale must floor at 1.0 of the 1080 sizes below 900 px height** (i.e. text no smaller than 12 px on screen at 720p)
  and the cards and radar shrink to 85% instead. A HUD scale setting 0.8-1.3 (already a P3 item in ENG-36) covers the
  rest.
- **Colour-blind check:** ship identity = glyph + keycap number + colour; shield vs hull = thin vs thick and the
  SHD / HUL tag; enemy vs friend on the radar = filled vs hollow and shape; above vs below = stem direction and
  brightness; ready vs cooling = full ring + bright glyph vs arc + dim glyph; locked ability = dashed ring + padlock.
  Run the greyscale capture check from the ENG-36 acceptance list on both concepts.
- **Over bright nebulae:** every text and bar sits on a plate at ≥ 62%; strokes on the world (brackets, markers) keep
  the existing 1 px dark outline. Target ≥ 4.5:1 for text on plates (Neutral on #0A0F1C at 85% over white is about
  13:1), ≥ 3:1 for bars and glyphs.
- **Glow:** only on live / ready things (flown card top bar, ready power, you on the radar, warp-in arc). Never on text.

---

## 11. States

| State | HUD |
|---|---|
| **Combat** | Everything in the mockup. |
| **Quiet travel** (no hostile within 2 km for 5 s, or hyperdrive) | Squad panel collapses to the ENG-36 chip size and 70% opacity; radar zooms to 3 km and dims to 70%, header shows "NEXT WAVE 0:12" or the objective distance; cluster arcs rest at 55% (existing rule). Returns in 120 ms on the first hostile within range or damage. In the hyperdrive jump: radar and squad hidden (existing rule). |
| **Planet approach / atmosphere** | Radar switches to a ground mode: the landing pad and objective markers only, range 1 km, no space contacts; altitude readout and descent heat bar as today. Squad panel at chip size. |
| **On foot** | Flight HUD hidden; `UTDGroundHUDWidget` as today. |
| **Docking** (tractor) | Squad panel and radar hidden; "TRACTOR BEAM · CONTROLS LIMITED" lane as today. |
| **Low health** (flown hull < 25%) | Flown card hull bar red with the 1 Hz breathe, the existing vignette; radar unchanged. |
| **Ship down** | That card's DOWN state; its wreck marked with a cross on the radar; if the flown ship is down before the retake, the cluster hides (existing `bHasFlown`). |
| **Director's Cut moments** | Unchanged: CUT hides all, slate dims to 40%, money shot hides everything but the reticle and arcs; the radar and squad panel follow the same opacity. |
| **Reduced motion** (setting) | No pulses (trouble, warp-in, missile flash become steady with a static icon), no card slide on switch (instant swap), no glow animation. |
| **Minimal HUD** (setting) | Squad panel at ENG-36 chip size without numbers, radar off (edge chevrons stay), wave header folded into the objective pill. This is the ENG-36 HUD of today, so it is free. |

---

## 12. Build plan for engineering

All in `Source/unrealGameJam/TriadDrift/UI/TDFlightHUDWidget.{h,cpp}` (engineering's widget; `Plugins/TriadDriftHUD`
is not touched), painted in the same `NativePaint`, every new value in `FView` so the tests can read it.

### 12.1 Reuse and extend

| Piece | Today | Change |
|---|---|---|
| `FChip` / `LayoutChips` | chip data + layout | Add `Power01`, `bHasAbility`, `Ability01`, `bAbilityLocked`, `UnderFireAge`, `bCloaked`, `bDomeUp`, flag list; new card sizes and layout; numbers. |
| `FView::PickupToast*`, `Overcharge01/Seconds` | toast + ring | New toast plate and the fold-into-flag animation; OVERCHARGE flag on the flown card. |
| `FView::bShowBoost`, `Boost`, `bDodgeReady` | bar at y 792 | Draw as the r 70 arc in the cluster. |
| `KindOf` / marker shapes | screen markers | Reuse the same shape painters at radar scale. |
| `FView::Pickups` (`FPickupMark`) | pickup screen marks | Add radar-space position. |
| `FTDFlightHUDTokens` | colours | Add Overcharge, Gate, Beacon; mirror into `UTDUIStyle` as ENG-36 planned. |
| `LastBoxes` / coverage tests | layout checks | Add "squad", "radar", "waveheader" boxes; raise the steady-combat budget if the Lead accepts concept A's ~7-10%. |

### 12.2 New data needed

**Update, 11:16 Nov 14:** while this concept was being written, `FTDHUDModel`
(`Source/unrealGameJam/TriadDrift/UI/TDHUDModel.{h,cpp}`, issue #160, Teammate 2) landed on the integration branch
(`lead/int-al`) and is owned by `UTDFlightHUDWidget` (`GetHUDModel()`). It already provides, allocation-free:
`FTDSquadStatus Squad[3]` (hull, shield, down + respawn, power / second power / ability cooldowns **and active
flags**, boost, squad-wide overcharge, `UnderFire01` for every ship, trouble) and `Contacts` (max 48, in the flown
ship's frame in metres with `Up` for above / below, `bBeyond` for rim pinning, `bThreat`, types Enemy / Missile /
Wingman / Objective / Pickup / Gate / WarpIn / Incoming, and incoming groups with `Seconds` and `Count` from T4's
`GetPendingGroups`). `RadarRange` defaults to 1500 m, matching this concept. **Most of the table below is therefore
already covered; what remains is drawing it.** Still missing from the model: cloaked / dome-up as named flags (derive
from `bAbilityActive` on Rift and `bPower2Active` on Bastion), planet / landing beacons and asteroid fields as contact
types, and the wave timeline (read `GetWaveInfo()` directly).

| Need | Source in the code | Notes |
|---|---|---|
| Per-ship power / ability / respawn | `FTDShipHUD` already has `PowerCooldown01`, `AbilityCooldown01`, `Ability`, `bAlive`, `RespawnSeconds` | Nothing new. Locked = `Ability == None`; which ability a ship *will* unlock is a static map (Rift→Cloak, Lancer→EMP, Bastion→Gravity Well). |
| "Under fire" per ship | not exposed | Bind each ship's `UTDHealthComponent` damage event (or add `LastDamageTime` to `FTDShipHUD`). Small. |
| Cloaked / dome up | `UTDAbilityPowers::IsCloaked()`, `UTDShieldDome::IsDomeActive()` | Read through the squad's ship actors, or add two bools to `FTDShipHUD` (cleaner). |
| Radar contacts in world space | `GetTargetMarkers()` (`FTDTargetMarker`) is screen-oriented; `GetSquadMarkers()` has `Actor` | Need the hostile actors' world positions and kinds: either an `Actor` field on `FTDTargetMarker` (check if present) or a new `ITDHUDData::GetRadarContacts(TArray<FTDRadarContact>&, float RangeCm)` filled by the squad manager from the wave director's alive list. Missiles: from the existing missile-warning source. |
| Objectives, gates, planets | `GetObjectiveMarkers()`; portal gate and planet actors | Gates / planets: cache the actors once per level (by class or tag), re-scan every 2 s like the transponder. |
| Asteroid fields | T6's field actors | Centre and radius per field, cached once. |
| Wave timeline | `GetWaveInfo()` (`WaveNumber`, `WaveCount`, `EnemiesAlive`, `bInBreather`, `BreatherRemaining`) | Nothing new. |
| Warp-in arcs | `ATDWaveDirector::OnWarpInStarted(Location, EnemyId)` | Bind like the existing bus handlers; keep the last 6 for 2 s. |
| Next-wave direction before warp-in | not exposed | Optional: wave director exposes its next spawn points during the breather. Skip if it costs more than 30 min. |

### 12.3 Order and rough hours

| # | Step | Hours |
|---|---|---|
| 1 | Squad panel: card layout, bars with numbers and segments, power and ability rings, DOWN state, switch resize animation; tests for each state | 3 |
| 2 | Status flags (under fire, cloak, dome, overcharge, afterburn, switch strike) incl. the two new data reads | 1.5 |
| 3 | Radar: frame, rings, heading-up transform, you / wingmen / hostile glyphs, stems, rim pinning, 48-blip cap; tests on transform and pinning | 4 |
| 4 | Radar extras: objectives, gates, planets, pickups, field textures, missile flash, warp-in arcs, wave header | 2 |
| 5 | Cluster: boost arc, pickup toast plate and fold animation | 1 |
| 6 | States: quiet travel collapse, planet ground mode, minimal HUD and reduced motion settings | 1.5 |
| 7 | Captures at 1080p, 720p and greyscale; coverage numbers; windowed playtest | 1 |
| | **Total** | **about 14 h** of one engineer; steps 1-2 alone (4.5 h) already answer Ryan's first ask |

With `FTDHUDModel` already in place (12.2), steps 2-4 lose most of their data work: about **10-11 h** in total,
about 7 h for steps 1, 2, 3 and 5.

With the feature freeze at Nov 15 12:00 EST, the realistic jam scope is steps 1, 2, 3 and 5 (about 9.5 h, or two
teammates in parallel: one on the squad panel + cluster, one on the radar, which touch different paint functions).

### 12.4 Performance budget

- **Under 0.5 ms** game thread for sample + paint together at 1080p (measure with `stat slate` and an Insights trace
  in a wave-5 fight).
- **No per-frame allocations:** `FView` arrays `Reset()` (not `Empty()`) and are reserved once (48 radar blips, 8 rim,
  8 flags, 6 warp arcs); text uses cached `FText` / `FString` per state change, not per frame formatting.
- Actor scans (gates, planets, fields) every 2 s, never per frame; hostiles come from the existing lists.
- Radar paint: one `MakeLines` call per glyph type batch where possible; one rounded box for the plate. Glow is a
  second, wider, low-alpha stroke, not a material.

### 12.5 Not in scope for the jam

A true 3D sphere radar or a rotating holographic scope; a target picture-in-picture camera; per-enemy names or
health on the radar; a clickable / zoomable map; player-configurable colours; voice callouts for radar contacts;
custom icon art (the shapes above are drawn in code); a separate radar for the on-foot game.

---

## 13. Open questions for Nick's side (UI art direction is Nicholas's)

1. **Style tokens:** may engineering add the three new colours (Overcharge, Gate, Beacon) and the plate opacity to
   `UTDUIStyle` / `DA_UI_Style`, or does Nick want to pick them?
2. **Icon set:** are the code-drawn shapes (ship glyphs, enemy kinds, pickup shapes, flags) acceptable for the jam,
   or does Nick have an icon sheet that should replace them through the Presentation paths?
3. **Fonts:** is a monospaced face for numerals OK, and which one (Roboto Mono under Apache 2.0, or the engine's
   DroidSansMono)? Any display font Nick wants for names and the name card?
4. **Which HUD draws it:** engineering's `UTDFlightHUDWidget` (today's default) or Nick's `Plugins/TriadDriftHUD`
   widget? This concept assumes engineering's, with Nick's still selectable (`TD.UI.NativeHUD 1`).
5. **Concept A or B,** and is the higher screen coverage (about 7% real, 10% by boxes, against ENG-36's 3.2%) acceptable
   in exchange for the information Ryan asked for?
6. **Name card:** keep the "CUT TO · CAM n" line on the switch card, or does the Director's Cut presentation design
   want different words?
