# Verdance documents: scope questions for Ryan

From Nicholas's four documents in `Story/` (Gameplay 02 Verdance Arrival, Cinematic 03 Beneath the Surface, Gameplay 03
Facility Defense, Cinematic 04 Verdance Departure). They are proposals; each one says so, and `Story/README.md` says
co-op and durations "do not establish supported multiplayer or change the campaign budget".

**About the hours:** they are my rough guesses from reading `features.md`, not measured against the team's speed and
not checked with T1-T5. Treat them as small / medium / large. Ask the owning teammate before deciding on a number.

## The questions

### 1. Length: Verdance alone would be as long as the whole game
- **Asks:** Gameplay 02 at 10-15 min, Cinematic 03 at 90-120 s, Gameplay 03 at 7-12 min, Cinematic 04 at 70-95 s.
  Verdance total: about 20-30 min. With the opening (2:20), the tutorial battle (6-10 min) and *Aftermath* (75-100 s),
  the game reaches 29-45 min before Cinder, waves 3-5 and the Dominion.
- **Today:** the whole game targets 20-30 min. Verdance is two short objectives on a 1 km course.
- **Cost:** none to decide; the cost is in every item below if the lengths are kept.
- **Recommend: fits in a reduced form.** Verdance as about 8-10 min in all: arrival and route 3-4 min, Cinematic 03
  40-60 s, defence and extraction 3-4 min, Cinematic 04 30-45 s. Both documents say their lengths are suggestions.

### 2. Co-op references
- **Asks:** nothing firm. The documents say "do not require co-op", "solo play must not depend on three humans",
  "in solo or co-op, enemy pressure must scale to actual players", and trigger Cinematic 03 "after all squad members
  are at the console (or when the local player interacts in solo mode)".
- **Today:** solo only, two AI wingmen in space, one player on foot.
- **Cost:** 0 h. **Recommend: fits as is**, read as solo. Say on the board that co-op is not in the game so the
  wording stops appearing in later documents.

### 3. The research-outpost defence (a new mode)
- **Asks:** defend a terminal while a progress meter fills; enemies try to disable the terminal; a midpoint
  checkpoint; a tougher unit or jammer interrupts the transfer; then extraction. 2-3 escalating enemy compositions.
- **Today:** Verdance objective 1 has "hold-interact 4 s while 2 waves drop in", then a carry back to the ship.
  Encounter volumes, wave spawns, attack tokens, checkpoints, mercy tiers all exist. No enemy attacks an object
  instead of the player, and there is no interruptible timed objective (from `features.md`; not checked in code).
- **Cost:** full version about 10-16 h (T5 mode and arena, T4 enemies that target the terminal, T2 meter), plus the
  set. Reduced version about 4-6 h (T5, with 1-2 h of T2 for the meter).
- **Recommend: fits in a reduced form.** Stretch the existing hold-interact into a 60-90 s transfer with a meter and
  three fixed waves; the transfer pauses while any enemy stands in a zone at the terminal (no new enemy AI); the
  third wave brings a Heavy as "the disruption"; then the existing carry-back as "extract". The full version with
  enemies that path to and damage the terminal does not fit before the freeze beside boarding and ENG-38.

### 4. The outpost itself (arena, interior, perimeter defences)
- **Asks:** an intact, lit research station with an interior room, a console, a perimeter with defences to "disable
  or bypass", a route with survey beacons and dig instruments, emergency lighting for the defence.
- **Today:** a wreck site and a relay on the course; engineering props; no building interior on Verdance.
- **Cost:** about 8-14 h (builders / Grok Bot for a small modular station, T5 to fit the course, environment builder
  for two light states). The largest single item, and it competes with the Dominion interior kit.
- **Recommend: fits in a reduced form.** One open-sided survey station (a roofed platform or a single room with wide
  doors) re-dressed from the wreck site, with the console and a dormant drone in it; "perimeter defences" = the
  existing trooper ambush and one gate. A full walk-in facility does not fit.

### 5. Consoles, archive transfer, data package, extraction
- **Asks:** activate the survey console (triggers the cutscene); a transfer with visible progress; collect a
  "verified data package"; reach an extraction point; pursuit breaks; optional "emergency reboot" interaction.
- **Today:** tap / hold interactables, a carried item, the board point marker and take-off all exist.
- **Cost:** about 2-3 h (T5 renames and hooks, T2 meter shared with item 3). Reboot interaction: skip (the document
  calls it optional).
- **Recommend: fits as is**, as renames of what exists (device → data package, ship → extraction).

### 6. Orbital approach: convoys that ignore you, a "scan", a landing interception
- **Asks:** fly an approach corridor, scan one convoy of drones carrying equipment, clear a small interception force
  over the landing path.
- **Today:** hyperdrive drop-out in Verdance orbit, then the descent. No scan action. Wave 2 fighters and
  interceptors exist; `storyline.md` §12 already suggests wave 2 could become "the approach to Verdance".
- **Cost:** about 4-6 h (T4 a non-hostile convoy lane and wave 2 moved to Verdance orbit; T2 a "hold lock to scan"
  prompt). The same non-hostile lane is wanted for the tutorial battle (§13), so it is built once.
- **Recommend: fits in a reduced form.** Wave 2 becomes the interception at Verdance; the convoy is background
  traffic with one Kato line; "scan" = hold the existing lock on a convoy ship for 2 s, or cut the scan.

### 7. Cutscenes 03 and 04
- **Asks:** two in-engine cutscenes (90-120 s and 70-95 s): a room track, a geological hologram of a buried
  structure, a drone that wakes, glows blue and pulses three times, comms answering by themselves, alarms; then
  take-off shots, a two-pattern overlay, a Dominion cut with a marker flash.
- **Today:** the director's cameras and the transmission system; the opening and *Aftermath* are queued to be built
  once, after the art is in (§11 gate). No pilot bodies to show on foot.
- **Cost:** about 6-10 h for both at reduced length (T3, with T2 for two 2D graphics and T4 / effects for the drone
  light). Full length with a 3D hologram: about double.
- **Recommend: fits in a reduced form.** Voice-only over the room and a monitor graphic (the document allows both);
  40-60 s and 30-45 s; built in the same single pass as the opening and *Aftermath*, so they wait for the same gate.
  Risk: that pass is already large (2:20 + 1:30); four cutscenes may not all land. Order of value: Cinematic 03's
  drone beat first (it pays off in the finale), Cinematic 04 last (three transmissions over the take-off can carry it).

### 8. New enemy types
- **Asks:** "drones" on the ground, "a tougher machine or jamming unit", "limited ground patrols". No named new type.
- **Today:** grunt, trooper, heavy, sentry drone, Warden; the sentry drone and Warden have no finished mesh.
- **Cost:** 0 h if existing types are used; a jamming unit as a new enemy about 4-6 h plus a model.
- **Recommend: fits as is** using trooper, sentry drone and Heavy. No new enemy type before the freeze.

### 9. Small clue beats (each is cheap, each needs an owner)
- **Asks:** a transponder marker that reads "verified" then "unknown"; one drone that hesitates half a second at the
  terminal; the three-pulse rhythm; the navigation marker flash on departure.
- **Today:** the HUD navigation pulse of *Aftermath* is planned (T2); the blue "holds fire" light is planned (T4).
- **Cost:** about 1 h each (T2 two, T4 two). **Recommend: fits as is.** They are the best value in the four documents.

### 10. Cinder shielding
- **Asks:** Command orders the squad to "secure any shielding material necessary for the approach"; the next
  document may make "secure environmental protection" an objective. The document itself says it is undecided and not
  to be treated as implemented.
- **Today:** Cinder has magma, geysers and crystals; no heat or shielding system; the crystals' story job is to
  stabilise the passage through the distortion field (§5 act 3).
- **Cost:** a real heat / shielding mechanic about 6-10 h (T5, T1). As words only: 0 h.
- **Recommend: does not fit before the freeze as a mechanic.** Either drop the clause from Command's line or let the
  existing crystals be the "shielding material" in words (Nicholas's choice).

### 11. "Visible minor battle damage" carried into the departure
- **Asks:** "if possible". **Today:** no persistent damage look on the player ships. **Cost:** 3-5 h (effects
  builder). **Recommend: does not fit**; the document marks it optional.

### 12. Ability rewards are missing from the documents
- **Asks:** nothing; the four documents never mention Gravity Well, Cloak, the signal-analysis device or the relay.
- **Today:** objective 1 gives Gravity Well, objective 2 (the relay, platforming with drones) gives Cloak; both are
  core and built.
- **Cost:** 0 h if kept. **Recommend:** keep both objectives and both rewards. Objective 1 takes the new fiction
  (outpost, console, transfer, package → Gravity Well); objective 2 stays the relay → Cloak. Nicholas says what the
  relay means in the new telling (see contradiction 1).

## Contradictions with `storyline.md` §1-13 and the twist rule

1. **Act 2's content changed.** §5 act 2 and ENG-25: a wrecked / abandoned research site, a **signal-analysis
   device**, recordings with **fragmented human voices**, the relay. The documents: an intact, silent outpost, a
   **geological survey archive**, a **buried artificial structure**, a drone's three-pulse acknowledgment. No voices
   anywhere. `A2_DeviceFound`, `A2_Voices`, `A2_TrailToCinder` and the clue row "transmission fragments ... Verdance
   relay" (§6) have no place in the new version unless the relay objective keeps them. Needs Nicholas's answer.
2. **Act 3's premise changed.** §5 act 3: a **mining complex**, **crystals that stabilise a passage through the
   field**. Cinematic 04: a **survey station** holding "the missing section", and shielding against heat. What opens
   the distortion field is no longer stated.
3. **Waves 3-4 are skipped.** §5 and `features.md` put waves 3-4 between Verdance and Cinder; Cinematic 04 jumps
   straight to Cinder.
4. **"Wrecked" against "maintained".** ENG-25 says "wrecked research site"; Gameplay 02 says the station "appears
   maintained ... no obvious battlefield". §12 ("a research outpost has gone dark") agrees with the new documents.
5. **A forbidden word.** §8: before act 4 nobody says "future", "time" or "us". Kato's "Data secured ... **Time** to
   go." would fail the story test (it rejects " time" in any pre-act-4 line). Wren's "tell **us** anything" is the
   ordinary pronoun and passes the test (which only rejects "it's us"), though it breaks §8 read literally.
6. **Names.** The documents call the machines "Dominion drones / Dominion machines"; §8 has the squad call the forces
   the **Null Armada**. Speaker is "COMMAND" (the system has both Command and Control; act 1-3 placeholders use
   Control). "Triad enters, weapons lowered": on foot the player has an energy blade, and only one body.
7. **Twist rule: no violation found.** Nothing names the future, time travel or the squad's identity. The strongest
   hints are the transponder flashing "verified", Triad's comms answering the drone's three pulses, the drone that
   pauses at the terminal and the marker flash on departure. All are deniable as scanning or tracking, and the
   documents forbid future faces, a human insignia or a decrypted message. Taken together with the opening's blue
   drone and handshake, that is four recognition beats on one planet; if playtesters guess the twist early, drop the
   "verified" label first (it is the only one that is text).
8. **Consistent, worth noting:** the "Don't-" fragment is held back in all four documents (§11, §12 rule); the blue
   optic matches the opening's red-to-blue drone; the marker pulse repeats *Aftermath*'s clue; "The Watcher" as a
   beat title reuses the name of act 1.
