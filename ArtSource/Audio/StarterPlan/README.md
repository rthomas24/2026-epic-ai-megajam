# Triad Drift audio starter plan

**Source-only milestone, 2026-11-12.** No audio was downloaded, recorded, imported, or licensed
on the team's behalf. `audition.html` synthesizes original placeholder tones in the browser,
only after a button press. It exports nothing and contacts no service.

## Inventory and first pass

The reviewed `eb8b8b5` checkout contains 542 files under `Content/`. A case-insensitive filename
scan for Audio/Sound/Music/SFX and raw WAV/OGG/MP3 found **zero named candidates**. This does not
prove no embedded SoundWave exists: inspect Unreal's asset registry later, once approved.
No external personal libraries or engine assets were treated as acquired project resources.

1. Audition the three switch signatures: Rift = airy rising fifth; Lancer = quick high double;
   Bastion = low, weighty descending response. Keep these timbres distinct in combat.
2. Source engine loops, fire and hull/shield impacts first. Cut seamless engine loops with
   matching start/end amplitude; audition the loop at minimum and maximum proposed pitch.
3. Acquire three variations for repeated weapons/impacts; avoid immediate repeats. Trim silence,
   add short edge fades, and preserve source/license evidence before import.
4. Add hyperdrive, targeting and warning cues; then exploration/combat music stems at a common
   tempo and phrase length. Sidechain music under VO and protect warning intelligibility.

`cues.json` is an **authoring manifest**, not a promised Ryan API or imported Unreal data table.
All asset references are null, all bindings proposed. Fade/pitch/voice limits are starting tuning
values; peak ceilings are requested export ceilings, not measured loudness. No final music LUFS
target is asserted before the complete mix is heard. Proposed raw export format: 48 kHz / 24-bit
WAV, mono for spatial SFX, stereo for music/UI where appropriate. Unreal codec settings follow
later listening and platform checks. Never normalize every layer to the same peak.

## Short resource list — nothing acquired

| Candidate | Primary evidence checked | Use gate |
|---|---|---|
| Kenney Sci-fi Sounds | [Pack](https://kenney.nl/assets/sci-fi-sounds), [support](https://kenney.nl/support): primary search results state CC0, commercial use permitted, attribution optional. Direct fetch was blocked. | First small-pack candidate for engine/computer textures. Re-open pack page and retain included license before download/use; no file-level verification yet. Credit Kenney voluntarily. |
| Sonniss #GameAudioGDC | [Current license v2.0](https://sonniss.com/gdc-bundle-license/), effective 2026-08-27: commercial modification/synchronization allowed, no mandatory attribution. | Confirm selected file belongs to official bundle; retain published license dated at acquisition. Raw assets cannot be distributed as assets; own-project team sharing allowed. No AI training/development use. Do not commit originals into a public repo. |
| WeLoveIndies jam offer | [Jam page](https://www.weloveindies.com/en/epicmegajam), [terms](https://www.weloveindies.com/en/agb): rights tied to project; jam page says contact provider if project becomes commercial. | Conditional option for music, not cleared. Verify exact checkout tier, project evidence and promotional/video coverage; commercial plans require provider clearance. Do not infer free-jam rights cover monetization or a different project. |

Before any acquisition, record provider, pack/file, URL, license/version/date, project owner,
receipt or grant, permitted team access, redistribution limits, edits, and submission credit.
The manifest's `source_candidate` is a search suggestion only. It never grants a license.

## Integration handoff

Ryan's documented presentation classes and events are read-only references (features.md §3.4–3.5).
Keep SFX, Music, VO and UI routes separate. Engine pitch should follow smoothed throttle, with
equal-power idle/boost crossfade; fire/impact sounds need pooled concurrency and distance culling.
Before binding, ask Ryan to confirm finalized slot/property names, event timing, active versus
wingman engine policy, and authoritative time for hyperdrive transitions. No gameplay edit is
needed to review this source package. Native assets, in-engine mixing and final listening remain
unverified.
