# ENG-36 · HUD redesign: design spec

Build document for the engineer (T2). Research and design by the Lead's HUD sub-agent, Nov 13.
All sizes are pixels at **1920x1080**; scale everything by `viewport height / 1080`.
Stage: **design only**. Nothing here is built, tested or played yet.

Wireframes (SVG, to scale, in `docs/eng/hud-redesign/`):

| File | Shows |
|---|---|
| `space_combat.svg` | Steady combat: every always-on element, with sizes |
| `space_combat_alert.svg` | Wingman in trouble, one down, power on cooldown, weapon hot, target locked, missile, hit arcs, hyperdrive blocked |
| `on_foot.svg` | On foot, third person (first person uses the same HUD) |
| `dominion.svg` | Dominion assault: generator pips 2/4, generator markers, reactor bar |
| `current_vs_new.svg` | Old footprint beside the new one, with the coverage numbers |

**Coverage result:** current HUD **21.5%** by element boxes (its bottom band blocks 29.4% of the screen);
new HUD **3.2%** in combat, **2.7%** between waves, **4.7%** in the alert state. Budget is 8%.

---

## 1. Findings

How each source was used: **[read]** = the page was opened and read; **[summary]** = only a search-result summary
was available (the page refused automated access or was not opened), so treat it as a pointer, not a quotation.

| # | Principle for this game | Source |
|---|---|---|
| 1 | Ask of every element "is this essential to play?" Most HUD items persist from habit and can go. | Greg Wilson, "Off With Their HUDs!" (2006) [read, excerpt] |
| 2 | Players look at the centre, around their ship. Put vital state (hull, shield) there at reduced opacity; it must not hide enemies or shots. | Yang Pulse, "2D Space Shooter Design Lessons" (2017) [read] |
| 3 | Target state belongs on the target's own indicator, where the pilot already looks. The reticle stays simple but has distinct states. | gamedev.net, "UI/UX Artist: Target LOCKED!" and Evochron developer interview [summary] |
| 4 | A permanent HUD is not wrong: most players are content with overlays, and a permanent HUD can help players understand the scene. So keep a small always-on core; hide only the rest. | Game Developer, "Study: most players happy with traditional HUD overlays" (Jørgensen); Caroux and Isbister, IJHCS [summary] |
| 5 | Keep the count of simultaneous readouts low (3 to 4). | Critical write-ups of GDC 2013 "Crafting Destruction: The Evolution of the Dead Space User Interface" (Dino Ignacio) [summary] |
| 6 | Colour alone never carries meaning: add shape, icon or text. Greyed-out also needs a second signal (a lock, a cross). | Xbox Accessibility Guideline 103 [read]; Game Accessibility Guidelines, basic [read] |
| 7 | Hit direction is shown on screen as well as heard; scale the indicator with hit strength. | XAG 103 [read]; Jasper Stephenson, "A UX Analysis of First-Person Shooter Damage Indicators" [summary] |
| 8 | Contrast: 4.5:1 for normal text, 3:1 for large text and graphics; allow a solid plate behind text. | Xbox Accessibility Guideline 102 [summary] |
| 9 | Prompts say what to do now, one at a time, spaced out. A prompt shown once and lost is a trap: keep a way to see it again. | gamedev.net, "7 UX Lessons From The Trenches"; Roblox Creator Docs, "Onboarding techniques" [summary] |
| 10 | On a character-switch HUD, each character's chip carries its core state and whether its ability is usable; switch prompts mirror the buttons. | "Character Switching HUD, The Odessa Tree" (ArtStation case study) [summary] |
| 11 | PC has no platform safe area: set margins yourself, and check with `r.DebugSafeZone.Mode 1`. | Epic, "Setting up TV safe zone debugging" and "UMG Safe Zones" [summary] |
| 12 | On ultrawide, anchor edge elements to a centred 16:9 frame so glance distance does not grow. | bugnet.io, "How to debug UI scaling on ultrawide monitors" [summary, secondary] |

No figure was found in an official source for a minimum text size (a "28 px at 1080p" figure appears only on
secondary sites and is **unverified**). This spec uses 17 px for words and 14 px for numerals and tags; raise both
through `UTDUIStyle` if play shows they are too small.

URLs are listed in section 8.

---

## 2. Audit of the current HUD

Measured on `Saved/TDShots/audit5/12_pool/mcp_01_36940.png` (1920x1080).

| Element | Box | % | Verdict | Reason |
|---|---|---|---|---|
| Title block "TRIAD / DRIFT, SQUADRON 03" | 263x52 | 0.66 | **Remove** | Not information. |
| Objective | 465x65 | 1.46 | **Shrink** | One 28 px line; expands only when it changes. |
| "WAVE 01 / 04 HOSTILES" | 190x56 | 0.51 | **Merge** | Becomes the counter at the end of the objective line. |
| Flight telemetry (speed number, boost, hyperdrive) | 228x193 | 2.12 | **Remove number; bars contextual** | Speed is felt, not read. Boost and hyperdrive show only in use. |
| Three squad cards | 1104x222 | 11.82 | **Shrink and move** | 360x44 strip. Percent numbers, role text, "WINGMAN", power names, padlocks and ammo for ships you are not flying all go. |
| "HYPERDRIVE LOCKED" permanent text | 304x18 | 0.26 | **Contextual** | Only when the player tries to jump. |
| Heat / ammo / signature block | 294x295 | 4.18 | **Shrink and move** | One 34 px pip beside the reticle. It repeats the active card ("PORTAL BLASTER READY" appears twice). |
| Key legend | 410x20 | 0.40 | **Contextual** | First-use hints only. |
| Reticle | 50x50 | 0.12 | **Keep, extend** | Gains the hull and shield arcs. |
| **Total** | | **21.53** | | The bottom band (y 705 to 1050) is 29.4% of the screen and sits where the ship is. |

Duplicated today: active ship hull and shield (card only, far from the reticle); signature power (card and block);
ammo (card and block); wave (objective and wave block); "you" (label, tag and border).

On foot (`tour/v_foot3`): vitals panel 215x138 at 720p and the objective plate are about 5.5% scaled to 1080p.
Verdict: shrink both; move block stamina to the reticle.

---

## 3. New layout: space combat

Anchors: **C** = screen centre (960, 540). **Safe frame** = 96 px left and right, 54 px top and bottom.

### 3.1 Always on

| Element | Anchor and size | Detail |
|---|---|---|
| **Reticle** | C, 32x32 | 2 px dot, four 8 px ticks starting 8 px out, 2 px white. |
| **Hull arcs** | C, radius 44, 5 px | Two mirrored 80° arcs (left and right, like brackets). Drain from the top down. White above 50%, amber to 25%, red below. Dim track at 16%. |
| **Shield arcs** | C, radius 53, 3 px | Same shape, outside the hull arcs, pale blue. Empty = track only. Shield is the thin outer layer, hull the thick inner one: shape, not colour, tells them apart. |
| **Signature power pip** | C + (64, 56), 34 px ring | Ready: solid ring in the flown ship's colour with the button glyph inside. Cooling: grey ring refilling clockwise, glyph at 45%. On ready: 300 ms scale pulse 1.0 to 1.25 to 1.0. |
| **Squad strip** | Bottom-left of safe frame, 360x44 at (96, 982) | Three chips in fixed order 1, 2, 3; 6 px gaps. Each: keycap (18 px) with the switch key, ship icon (20 px) in squad colour, shield mini-bar (2 px) over hull mini-bar (5 px). Active chip is 148 wide, 100% opacity, squad-colour outline and top bar, and shows the ship name (14 px). Wingmen are 100 wide at 72%. The order never changes. |
| **Objective line** | Top-centre of safe frame, 480x28 at (720, 54) | Diamond icon, objective in 17 px caps, counter right-aligned ("5 LEFT"). 70% opacity on a 45% dark pill. |

The cluster box is 224x116 (x 880 to 1104, y 500 to 616). Arcs stay at full opacity while either value is below
100% or changed in the last 3 s; otherwise they rest at 55%.

### 3.2 Contextual

| Element | Where | Shows when |
|---|---|---|
| Ability pip (28 px) | C + (−64, 56) | The flown ship's ability is unlocked. Never a padlock in flight. |
| Second power pip (24 px) | Right of the signature pip, +40 | The flown ship has a second power (Bastion's dome). |
| Ammo count (15 px numerals) | Right of the pips | The flown ship's weapon uses ammo. |
| Heat arc | C, radius 30, bottom 60° | Heat ≥ 60%: amber. ≥ 90%: red plus "HOT". |
| Switch Strike ring | C, radius 62, 2 px, ship colour | `GetSwitchStrike01 > 0`; the ring drains with it. |
| **Target bracket** | On the current target | Four corner ticks (min 48, max 160 px), 4 px health bar above, distance (14 px) below. Lock: amber diamond inside plus "LOCK". Other locked targets: 20 px amber diamond. Focus order: double chevron above the bracket. |
| Hostile tick | Above each on-screen hostile | 14 px hollow red triangle, point down. |
| Lead indicator | World point | 14 px white ring, only with a current target. |
| Edge arrows | On the safe frame | Hostile: filled red chevron. Missile: filled red chevron, pulsing. Wingman: hollow chevron in squad colour, and it pulses when that wingman is in trouble. Objective: white diamond with distance. |
| Missile warning | Centred, 150x28 at y 444 | `Tracking`: steady. `Imminent`: pulses at 2.5 Hz (kept under 3 Hz). |
| Hit direction arcs | C, radius 150, 50° wide, 8 px | One per `FTDDamageIndicator`. Red for hull, pale blue for shield-only. Opacity = 0.4 + 0.6·strength, fading over 1.2 s. |
| Low-hull vignette | Screen edge | Hull under 25%: 10% red edge glow, slow 1 Hz breathe. |
| Boost bar | Centred, 160x6 at y 792 (under the ship) | While boosting or refilling; hides 1.5 s after full. A 2 px notch at its right end is the dodge: lit = ready. |
| Hyperdrive bar | Centred, 240x6 at y 806 | While charging (`GetHyperdriveCharge01 > 0`). |
| **Lower lane** (one message at a time) | Centred, up to 420x36, y 826 to 906 | Priority: interact or landing prompt, then hyperdrive block text (3 s, on a jump attempt), then a first-use hint. |
| Boundary warning | Centred, 520x28 at y 132 | `GetBoundaryWarningText` non-empty. Amber, triangle icon. |
| Objective expanded | 720x72 at (600, 54) | For 3 s when the text changes: 28 px title, 16 px second line. Then collapses in 300 ms. |
| Wingman trouble | On its chip | Hull under 35%: amber outline pulsing at 1 Hz plus a "!" triangle badge. Down: icon crossed out, respawn seconds in place of the bars. |
| Subtitles | Centred, max 880x60 at y 900 | On a bark. 22 px on a 60% dark plate. |

### 3.3 Coverage (bounding boxes over 2,073,600 px)

| State | Elements | Area | % |
|---|---|---|---|
| Between waves | cluster 224x116, strip 360x44, objective 480x28, objective marker | 56,800 | **2.74** |
| **Combat, steady** | the above without the marker, plus target bracket 72x104, 8 hostile ticks, 4 edge arrows, lead, 2 wing arrows | 66,892 | **3.23** |
| Alert (`space_combat_alert.svg`) | combat plus boost bar, lock tags, missile tag and chevron, 2 hit arcs, lower-lane message | 96,512 | **4.65** |
| Transient peak | alert plus expanded objective, boundary warning, switch name card (360x56) | 169,632 | 8.18 for about 3 s |
| Peak with subtitles | plus 880x60 | 222,432 | 10.73 |

Steady flight is 2.7 to 3.2%, under half the budget. Boxes overstate the arcs and brackets, which are mostly empty.
The peaks are short and need several rare events at once; the lower-lane rule keeps them from stacking further.

---

## 4. Other states

| State | Changes from space combat |
|---|---|
| **Travel / hyperdrive** | Charging: hyperdrive bar fills. In the jump: hide the squad strip, pips, ticks and arrows; keep the reticle at 50%, hull and shield arcs, and the objective line. Blocked: the block text in the lower lane for 3 s. Portal gates use the objective diamond. |
| **Atmosphere flight** | Altitude sits left of the reticle at C + (−110, 5): "ALT 222 m", 15 px, only while `GetAltitude() ≥ 0`. Under 60 m the text turns amber. Landing pad = objective diamond with distance. Landing prompt in the lower lane (glyph plus "LAND"). No hostile UI unless enemies exist. |
| **On foot, first and third person** | Reticle: 3 px dot in an 8 px ring. Vitals at the squad strip's anchor, 260x40: health 200x8 with a heart, block 140x4 with a shield, two dash pips 18x7 (filled = ready). Vitals rest at 40% after 4 s at full health. Block stamina also appears as an arc (radius 28, bottom 80°) under the reticle while blocking or refilling. Perfect deflect: the reticle ring bursts to 40 px in 150 ms plus "PERFECT" (17 px) for 0.55 s; a normal deflect is a 0.3 s ring flash. Hostiles: triangle above the head; a 48x4 health bar only once damaged or targeted. Interact: lower lane, 300x40, glyph in a ring that fills over `GetHoldTime()`. Objective line and diamond marker as in space. Hit arcs as in space. Coverage **1.24%** steady, **2.23%** in a fight with a prompt. |
| **Dominion assault** | Objective line widens to 560: "BREAK THE SHIELD GENERATORS", four diamond pips (filled red = alive, hollow with a slash = destroyed) and "2 / 4". Live generators are objective markers: a ringed diamond (40 px) with a health bar and distance; the name only on the current target. At `ReactorExposed` the pips are replaced by a reactor bar, 480x26 at (720, 92), and the objective reads "DESTROY THE REACTOR". `Breakup`: hide everything except the objective line ("GET CLEAR OF THE DOMINION") and the arcs. Coverage **3.28%**, or 3.65% with the reactor bar. |
| **Switch cut (C-03 #1)** | Name card "WREN · RIFT", 360x56 at the lower third (centred, y 760), ship colour bar on its left, 0.9 s. The active chip changes in the same frame; the arcs and pips recolour instantly. Never blocks input. |
| **Slate (C-03 #3)** | "SCENE 3 · CINDER · TAKE 1", centred card 560x120, 1.5 s. HUD stays on at 40% behind it. |
| **Money shot (C-03 #2)** | Letterbox bars, 10% of the height top and bottom. Everything hides except the reticle (40%) and the hull and shield arcs. Returns over 150 ms when time dilation ends. The 0.15 s perfect-deflect flash does not letterbox. |
| **"CUT!" and retake** | `OnSceneCut`: all HUD off; "CUT!" 120 px centred for the delay; then the slate with the next take number; HUD fades in over 250 ms as control returns. Hints do not replay on a retake. |
| **Cinematic** | `SetCinematicHidden(true)` removes everything, as today. |
| **Pause** | HUD dims to 30% under a 60% black scrim; the existing pause screen on top. The pause screen lists the controls (the way back to any hint that was missed). |

---

## 5. Visual language

**Type.** Roboto (ships with the engine; no new licence). Caps with letter spacing for words.

| Use | Size | Weight |
|---|---|---|
| "CUT!" | 120 | Black |
| Slate, name card, expanded objective title | 28 | Bold |
| Subtitles | 22 | Medium |
| Objective line, prompts, warnings | 17 | Bold, +1 tracking |
| Ammo, glyphs in pips | 15 | Bold |
| Distance, chip name, tags (smallest allowed) | 14 | Semibold |

**Line and shape.** Strokes 2 px (3 for rings, 5 for the hull). Corner radius 6 on chips and plates, 14 on the
objective pill. No bevels, no panel borders except on the active chip. Glow: none on text; one soft 6 px outer glow
at 35% on a pip at the moment it becomes ready and on the active chip's top bar. Plates: `#0A0F1C` at 45 to 70%.
Every text and icon gets a 1 px dark outline or sits on a plate, so it reads over a bright nebula.

**Colour tokens** (put them in `UTDUIStyle` so Nicholas can restyle):

| Token | Value | Use |
|---|---|---|
| Neutral | `#E8EEF7` | Text, reticle, hull, neutral icons |
| Dim | `#8A94A6` | Tracks, disabled |
| Rift | `#FF4FD8` | Rift accent only |
| Lancer | `#2FE6FF` | Lancer accent only |
| Bastion | `#FFB13B` | Bastion accent only |
| Shield | `#9FD8FF` | Shield arcs and bars (always the thin outer line) |
| Warning | `#FFE14A` | Always with a triangle or lock icon; yellow, so it is not Bastion's amber |
| Danger / enemy | `#FF4A3D` / `#FF5A36` | Hull critical, missile, hostile marks |
| Plate | `#0A0F1C` | Backing |

Only the flown ship's colour appears in the reticle cluster, so the centre of the screen tells you which ship you fly.

**Icons** (original, simple shapes): Rift = crescent; Lancer = dart; Bastion = hexagon; hostile = hollow triangle,
point down; objective = diamond; generator = diamond in a ring; lock = diamond outline; warning = triangle with "!";
down = cross; health = heart; block = shield outline; blocked hyperdrive = padlock; keycap = rounded square;
pad button = circle.

**Motion.** Show: 120 ms ease-out. Hide: 250 ms ease-in. Objective expand and collapse: 300 ms. Trouble pulse: 1 Hz
sine, opacity 0.5 to 1. Imminent pulse: 2.5 Hz. Bars ease to their value over 150 ms; a hull loss leaves a white
"ghost" segment that drains over 400 ms. Nothing flashes faster than 3 Hz.

**Accessibility and screens.** Every colour meaning has a shape or word beside it (table above). Minimum text 14 px
at 1080p, scaled with height. Safe margin 5%. At 1440p everything scales 1.333. On ultrawide the cluster, objective
line, lanes and cards stay centred; the squad strip and edge arrows anchor to a centred 16:9 frame. Check with
`r.DebugSafeZone.Mode 1` and at 2560x1080 and 3440x1440. A HUD scale setting (0.8 to 1.3) is a P3 option.

**Glyphs.** Keyboard: rounded-square keycap with the key letter. Pad: circle with the button letter, "LB"/"RB" in a
pill. Switch on `ATDSpacePlayerController::IsUsingGamepad()`; text only, no vendor artwork.

---

## 6. Beginner path: the first five minutes

| When | What the player sees | How they learn it |
|---|---|---|
| 0:00 slate | "SCENE 1 · TAKE 1", then the objective line expands: "CLEAR THE NULL ARMADA". | A plain sentence says what to do. |
| 0:05 | Reticle with two full brackets around it. Hint: "[mouse] AIM   [LMB] FIRE". | Hint goes after the first hit confirmed. |
| First enemy | A red triangle; when aimed at, corner brackets with a health bar and distance. | The thing with the red bracket is the thing to shoot; the bar drops as they hit it. |
| First hit taken | The thin outer arc shortens and a red arc flashes on the side the shot came from. | Outer = shield goes first; then the thick arc, which changes colour. A one-time label "SHIELD" / "HULL" (14 px) sits beside the arcs for 4 s on the first damage. |
| 0:30 | The pip beside the reticle is solid magenta with "Q". Hint: "[Q] PORTAL BLASTER". | Press it: the ring empties and refills. Hint goes after the first use; the label under the pip stays for the first three uses of each ship's power. |
| 1:00 | Hint: "[1] [2] [3] SWITCH SHIP"; the strip's keycaps pulse once. | Switching plays the cut and the name card; the reticle cluster changes colour to match the new chip. Hint goes after the first switch. |
| Wingman hurt | A chip pulses amber with "!"; an edge chevron in that ship's colour pulses. | The chip shows the key to press to take that ship over. |
| Enemy off screen | Red chevrons on the screen edge. | Turn toward them. |
| Weapon hot | An amber arc appears under the reticle, then "HOT". | It only exists when it matters. |
| Wave cleared | Counter reaches "0 LEFT"; the line expands with the next objective and a diamond marker with distance appears. Hint: "[key] HYPERDRIVE". | Follow the diamond. |
| Jump attempt in combat | "HYPERDRIVE LOCKED - ENEMIES IN RANGE" with a padlock, 3 s. | Says why, at the moment of asking. |
| Death | "CUT!", then "TAKE 2". | Dying is a retake, not a game over. |

Hints come one at a time, at least 8 s apart, at most three showings each, never during a money shot, and are stored
so they do not return. The pause screen always lists the controls.

---

## 7. Implementation plan

Build it as engineering's own widget in `Source/unrealGameJam/TriadDrift/UI/`, painted the way
`UTDGroundHUDWidget` is (one `NativePaint`, a testable `FView` snapshot). `Plugins/TriadDriftHUD` is not touched.
`UTDUIHostSubsystem` shows it by default; `UTDUIHostSettings::bUseNativeSpaceHUD` (or `TD.UI.NativeHUD 1`) keeps
Nicholas's widget selectable. The new widget sets every `bCovers<Surface>` property to true so the dev HUD stops
drawing those surfaces.

| Pri | Module | Data binding |
|---|---|---|
| P1 | Reticle + hull and shield arcs | `GetSquadHUD()`: the entry with `bPlayerControlled` → `Health01`, `Shield01`, `Ship` (colour) |
| P1 | Signature power pip | Same entry: `PowerCooldown01` (1 = ready); glyph from `IsUsingGamepad()` |
| P1 | Squad strip | `GetSquadHUD()` all three: `Ship`, `Health01`, `Shield01`, `bPlayerControlled`, `bAlive`, `RespawnSeconds`; trouble = `bAlive && Health01 < 0.35`; `OnShipSwitched` for the instant change |
| P1 | Objective line | `GetObjectiveText()`, `OnObjectiveUpdated` (expand); counter from `GetWaveInfo()`: `EnemiesAlive` while `bWaveActive` |
| P2 | Ability, second power, ammo, heat | Entry: `Ability != None` and `AbilityCooldown01`; `Power2Cooldown01`; `SecondaryAmmo`; `Heat01` |
| P2 | Target bracket, ticks, edge arrows | `GetTargetMarkers()` (`bOnScreen`, `ScreenPosition01`, `ScreenDirection`, `bIsCurrentTarget`, `bLocked`, `bIncomingMissile`, `Distance`); `GetCurrentTarget()` with its `UTDHealthComponent` for the bar; `GetLockedTargets()`; `GetFocusTarget()`, `GetFocusRemaining01()` |
| P2 | Lead indicator | `GetLeadIndicator()` |
| P2 | Wing arrows | `GetSquadMarkers()` (`Actor`, `bAlive`, `bEngaging`, skip `bPlayerControlled`) |
| P2 | Missile warning | `GetMissileWarning()` |
| P2 | Hit arcs, vignette | `GetDamageIndicators()` (`WorldDirection`, `AgeSeconds`, `Strength01`, `bShieldOnly`); hull from the entry |
| P2 | Boost, dodge, hyperdrive bars | Entry `Boost01`, `Dodge01`; `GetHyperdriveCharge01()`, `OnHyperdriveState` |
| P2 | Lower lane | `GetInteractPrompt()`; `GetHyperdriveBlockText()` (show 3 s when it becomes non-empty and on a jump press while blocked: **needs the jump input forwarded to the widget**); hints from `OnTipRequested` and the existing `UTDHintComponent` |
| P2 | Boundary, Switch Strike, altitude | `GetBoundaryWarningText()`, `IsOutOfBounds()`; `GetSwitchStrike01()`; `GetAltitude()` |
| P3 | On-foot restyle | `UTDGroundHUDWidget`'s existing view: `GetHealth01()`, `GetBlockStamina01()`, `GetDashCharges()`, `GetInteractPrompt()`, `GetObjectiveText()`, its deflect and damage handlers |
| P3 | Dominion | Not on `ITDHUDData`: find `ATDDominion` once; `GetState()`, `GetGeneratorsRemaining()`, `GetGenerators()`, `GetReactor()` and their health components. Markers also arrive through `GetObjectiveMarkers()` |
| P3 | Director moments | `OnShipSwitched` (card), `OnTakeStarted` (slate), `OnSceneCut` ("CUT!"), `OnHighlightMoment` (letterbox and hide), `SetCinematicHidden` |
| P3 | Pause dim, subtitles, HUD scale, ultrawide frame | `OnPauseChanged`; `OnBarkRequested`; settings |

**Open questions for the engineer** (not verified in code by this spec): which of `Power`, `Power2` and
`SecondaryAmmo` each ship really uses, and the key bound to each; whether the existing fallback name card, slate and
"CUT!" should stay where they are drawn today and only be restyled.

**Acceptance checklist**

- [ ] Coverage measured from a 1920x1080 capture: ≤ 8% in steady combat (design value 3.2%); centre and lower middle clear.
- [ ] Before and after captures: combat, alert, travel, atmosphere, on foot (first and third person), Dominion (shields up and reactor exposed), cut card, slate, "CUT!", money shot, pause.
- [ ] One automation test per binding in the table (value in → `FView` out), including wingman trouble, down and respawn, lock, missile states, heat thresholds.
- [ ] Switching updates the strip and the cluster colour in the same frame; no input is lost.
- [ ] Keyboard and pad glyphs swap live.
- [ ] 1440p, 2560x1080 and 3440x1440 captures: nothing outside the safe frame, nothing overlapping.
- [ ] Greyscale capture: ships, shield vs hull, hostile vs objective, ready vs cooling are still distinguishable.
- [ ] A fresh reviewer plays the first five minutes and names every element without help.
- [ ] `TD.UI.NativeHUD 1` still shows Nicholas's HUD; with it on, this HUD is fully off.
- [ ] Played in a windowed session, recorded in `DEVLOG.md`; a note posted on `MESSAGEBOARD.md` for Nick's side.

---

## 8. Sources

- Greg Wilson, "Off With Their HUDs!: Rethinking the Heads-Up Display in Console Game Design": https://www.gamedeveloper.com/pc/feature-off-with-their-huds-rethinking-the-heads-up-display-
- Yang Pulse, "2D Space Shooter Design Lessons": https://gamedeveloper.com/design/2d-space-shooter-design-lessons
- "UI/UX Artist: Target LOCKED!" (gamedev.net blog): https://gamedev.net/blogs/entry/2268272-uiux-artist-target-locked
- "Study: most players happy with traditional HUD overlays" (Game Developer): https://gamedeveloper.com/design/study-most-players-happy-with-traditional-hud-overlays
- Caroux and Isbister, IJHCS preprint: https://hal.archives-ouvertes.fr/hal-01455547/file/Caroux&Isbister-IJHCS-InPress-PrePrint.pdf
- GDC Vault, "Crafting Destruction: The Evolution of the Dead Space User Interface": https://gdcvault.com/play/1017723/contactUs
- Xbox Accessibility Guideline 103, additional channels for visual and audio cues: https://learn.microsoft.com/en-gb/gaming/accessibility/xbox-accessibility-guidelines/103
- Xbox Accessibility Guideline 102, contrast: https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/102
- Game Accessibility Guidelines, basic: https://gameaccessibilityguidelines.com/basic/
- Jasper Stephenson, "A UX Analysis of First-Person Shooter Damage Indicators": https://medium.com/@jasper.stephenson/a-ux-analysis-of-first-person-shooter-damage-indicators-59ac9d41caf8
- "7 UX Lessons From The Trenches" (gamedev.net): https://gamedev.net/tutorials/game-design/ux-for-games/7-ux-lessons-from-the-trenches-r4969
- Roblox Creator Docs, "Onboarding techniques": https://create.roblox.com/docs/production/game-design/onboarding-techniques
- "Character Switching HUD, The Odessa Tree" (ArtStation): https://tomgia.artstation.com/projects/PXBvd8
- Epic, "Setting up TV safe zone debugging in Unreal Engine": https://dev.epicgames.com/documentation/unreal-engine/setting-up-tv-safe-zone-debugging-in-unreal-engine
- Epic, "UMG Safe Zones in Unreal Engine": https://dev.epicgames.com/documentation/en-us/unreal-engine/umg-safe-zones-in-unreal-engine
- bugnet.io, "How to debug UI scaling on ultrawide monitors": https://bugnet.io/blog/how-to-debug-ui-scaling-on-ultrawide-monitors
- Game UI Database (reference catalogue of shipped HUDs; it refused automated access, so it was not consulted): https://www.gameuidatabase.com/

The layout, icons and wording here are original; the games above are references for principles only.

## Lead's review notes (Nov 13, binding for the build)
- Approved as the build spec. PNG renders of every wireframe sit beside the SVGs in `docs/eng/hud-redesign/`.
- **Squad strip, one change:** switching ships is the game's core mechanic, so the strip is 1.3x the drawn size
  (about 470 x 58): each chip shows the ship icon, its switch key, hull and shield mini-bars, and the signature
  power's ready state as a small dot, so a player can see at a glance that a wingman's power is ready before
  switching. The coverage stays under 4% in steady combat.
- The active chip shows the pilot / ship name; wingman chips show names only during the first minutes (hint rule).
- Build order stays P1 (reticle cluster, squad strip, objective line, power pip), then P2, then P3; capture each
  state against its wireframe before moving on.
