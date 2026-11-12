# Triad Drift — cinematic switch candidates M02

**Agent:** Audio & Music Agent  
**Role:** Cinematic sound design and music  
**Task:** Three ship-switch WAV candidates for Nicholas's listening review

Created November 12, 2026. Review candidates only; quality approval and Unreal
integration are pending. The previous five browser oscillator sketches were
rejected as retro. They and the 32-cue plan are preserved outside this directory.
Current feature references are ART-10, ART-09/12 and C-03 in `features.md`; the
older S-23/G-14 references in the brief now map to ART-10.

## Audition

Open `audition/index.html` in a browser or open these actual WAVs in a player:

| File | Length | Intended identity |
|---|---:|---|
| `audition/SW_TD_Rift_Switch_M02.wav` | 1.62 s | Pressure implosion, textured dimensional movement, spatial wake |
| `audition/SW_TD_Lancer_Switch_M02.wav` | 1.28 s | Quick armature contacts, turbine surge, compact activation |
| `audition/SW_TD_Bastion_Switch_M02.wav` | 1.78 s | Heavy clamps, armored plate resonance, deep pressure weight |

48 kHz, 24-bit signed little-endian PCM WAV, stereo, no loops. Files total
approximately 1.35 MB. These are complete composite SFX, not musical cues.
Playback page uses only these WAVs with native audio controls; it synthesizes
nothing, requests no external service and prevents simultaneous manual playback.

## Sound design

All audio is original procedural synthesis, authored for this project. There
are no field recordings, downloaded library samples, pretrained audio outputs,
external impulse responses, copyrighted melodies or sound-alike references.

- Damped, inharmonic shell modes suggest metal structures and physical mass.
- Broad stochastic pressure excitation supplies attacks and substantial bodies.
- Irregular windowed micro-contacts suggest rails, clamps and plate vibration.
- Filtered turbulent air and brief reverse pressure add motion and release.
- Restrained independently timed L/R reflections supply spatial depth.
- Bass below approximately 145 Hz is centered; side movement lives above it.
- Gentle parallel body saturation, 28 Hz low cut, upper-frequency rolloff,
  edge fades and final offset removal prepare a clean export.
- Common short-SFX loudness is selected without crushing the highest transient.
  Deterministic TPDF dither is applied once at PCM24 conversion.

There is no chiptune pulse train or note-sequence activation melody. Modal
synthesis does contain sinusoidal components, as physical resonant models do;
the delivered composite is layered with noise, friction, pressure and reflections.
These techniques aim for physical cinematic character; listening decides whether
they meet the requested realism. No AAA-quality acceptance is asserted.

## Verification and limits

`verification.json` records rendering measurements and hashes.
`export-qa.json` independently decodes the delivered PCM bytes, checks their
headers, clipping, onset, edges, DC offset, channel balance, spectral energy
and mono correlation. Three WAVs only. Both scripts must complete successfully.

The K-weighted loudness reference uses 400 ms blocks, 100 ms hops and absolute/
relative gates. These short transient files are not long programs; the number
is a matching aid and does not guarantee perceived equality. Peak interpolation
uses a guarded 4x FFT estimate, not a certified BS.1770 true-peak meter.
Headroom applies to individual files; repeated overlapping playback can exceed it.

No auditory QA is claimed: headphone/speaker listening, repeated-switch fatigue,
small-speaker translation and combat-mix masking require Nicholas's review.
No Unreal import, MetaSound, asset registry, PIE, attenuation, concurrency,
packaging or device playback test was performed in this milestone.

## Reproduce

Requires Python 3.11+ and NumPy. Production used the standalone Python 3.13.9
interpreter bundled with installed Blender 5.1, NumPy 2.3.4. Blender itself was
not launched; no bpy calls, heavy application, network or paid tool was used.

```powershell
$audioPython = 'C:/Program Files/Blender Foundation/Blender 5.1/5.1/python/bin/python.exe'
& $audioPython ./render_switches.py
& $audioPython ./verify_exports.py
```

Run from this directory, or pass absolute script paths. Scripts resolve output
relative to their own location. They replace only this milestone's generated
WAVs and JSON reports. Do not rerun once the package is frozen for Lead review
without coordinating a new revision. Fixed seed 261112; exact byte parity
depends on the Python/NumPy/platform floating-point implementation.

## Ownership and integration handoff

Lead dispatch: main `24d5dddf7407ce0680dc02cde00a41b0664959ae`.
Chat `01a1d0e9-aaa8-7f61-85c7-86b6fd3f1db7`, branch `nick/audio-music`, worktree
`C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-music`.
Only `ArtSource/Audio/StarterPlan/SwitchM02/**` is writable. Prior `/root/audio`
worker stopped; its `nick/audio` worktree remains untouched. Direct messaging
to/from Lead was tested. Lead owns remote synchronization, claims, branch
publication and integration; request serialized precommit review.

`credits.json` and `CREDITS-HANDOFF.md` supply provenance for Lead's root credits
and devlog update. This worker does not edit shared root documents or claims.

Confirmed current presentation hook, from Lead/R-003 and read-only main
`802aa8c`: `ATDSquadManager::OnSwitchCut(FTDSwitchCut)` carries pilot/card,
colors, camera transforms and timing. `UTDShipVisuals.SwitchIn` / `SwitchOut`
are the existing presentation slots. After approval, propose the destination
ship's `SwitchIn.Sound` only, with engineering confirming a single playback
path so the slot and event listener do not double-trigger the same WAV.
Proposed future instance names are `/Game/TriadDrift/Presentation/DA_Rift_Visuals`,
`DA_Lancer_Visuals` and `DA_Bastion_Visuals`; these are names only, no native
asset or new write claim was created. Fire immediately at the camera cut; there
is no preroll requirement. These stereo cues are proposed as local cockpit/UI
presentation, without world attenuation. Do not delay possession, input or
camera changes to wait for the 1.28–1.78 s tails. Avoid playing both an old-ship
switch-out and destination composite together without a listening pass.

For rapid 0.4 s gameplay switching, propose a shared voice cap of two and a
35–60 ms fade of the oldest tail when replaced; verify in combat. Start at unity
pitch and tune cue gain by ear under engines, weapons and VO. These are handoff
suggestions, not a promised Ryan API or implemented audio system.

Ryan dependency after approval: confirm presentation slot/event ownership and
runtime routing/concurrency, via Lead; engineer-owned code is untouched.
Lead should coordinate the existing UI agent's cut/name-card timing with these
immediate attacks. No UI files are changed.

Next milestone: revise these three from listening notes if necessary, then
seek authorization for engine/weapon/impact production and repeat variation.
Broader SFX/music production waits for first-milestone review. The longer-term
brief includes engines/powers/combat/UI and reusable hybrid score stems; no
soundtrack work has started. No main audio integration before listening approval.
