# Building Triad Drift in four days

On November 12, our first Shipping build opened on a squadron of ships, a planet and an asteroid belt. There was no HUD. We had tested the HUD in Development. The fallback had been compiled out of Shipping, and the styled HUD was still off by default. A capture from outside the packaged game showed the failure immediately. The next build drew the ship cards, objectives and enemy markers.

We tried again from the main menu. An agent sent Enter, Tab and Esc to the running Windows build. Enter launched wave one, Tab cut from Lancer to Bastion, and Esc paused the game. Leaving the controls alone taught us something else: Rift could go down in the first thirty seconds. The team gave the opening wave a grace period. An early hyperdrive prompt also sat over the player’s ship. Seeing the prompt against a moving target told us where to move it. [Packaged gameplay capture](../../eng/evidence/shipping_keys_ship_2200.png)

## A material that looked right in the data

On Cinder and Verdance, crystal props and giant mushrooms appeared as black silhouettes. Their colour values looked right in the editor data. A rendered walk through both worlds gave engineering the place to look: the game log said the master material expected a mask texture but had received a colour texture as its default. With a project mask as the default, the amber and cyan crystals and glowing mushrooms appeared in the next captures. Our import checks gained a second step: render the prop in the game and read the material compiler log.

## How the two leads used AI

Ryan’s engineering team used Claude Code for gameplay and builds. Nicholas’s presentation team used OpenAI Codex for music, interface and story work. The teams shared a message board and claimed files before editing. For the enemy trooper, a builder fitted armour to the project mannequin in Blender. A separate reviewer compared the contact sheets to Ryan’s concept and pointed out that the slim limbs, boxy plates and dark face still read like the mannequin. Later versions changed the helmet, added visible eye panels and gave the grunt and heavy distinct shoulder widths. [Trooper review sheets](../../../ArtSource/EngCharacters/NullTrooper/review)

The music handoff exposed a different gap. Nicholas chose *Push The Attack* for the Dominion space assault. We preserved the complete 144.68-second track and its quiet ending. A second Unreal process reopened it and confirmed looping and the Music sound class. That check can establish what the file contains. Listening through the generator battle will establish whether the general combat track yields, where the loop returns, and whether it stops when the assault ends. [Audio verification record](../../../ArtSource/Audio/DominionAssault02/native_checkpoint_01.json)

## The click between repeats

Nicholas heard a small click when the music restarted. We recorded the Verdance theme across its repeat and compared the game recording with the source. The old restart left a 32-millisecond gap. The source also ended at a different sample value from where it began.

The repair kept the whole track. We brought its two edges smoothly to zero over four milliseconds and let Unreal's audio renderer loop the Wave. A second editor reopened the asset. Then we recorded more than six minutes on the Verdance map, including a repeat during an ending fade. Both joins aligned without the old gap. The arrival and departure were triggered for the test; the natural campaign handoff still needs its own check. [Hear the two recorded joins](MusicBuild01/music-loop-check.md).

## When focus existed but could not be seen

The menu passed its input tests: closing Settings returned focus to the right button, and pressing Enter reopened it with the changed volume intact. The screenshots showed a different result. Every button had the same blue outline, even the disabled Continue action. The player could not see which action was selected.

UI changed the outline drawing. Engineering captured the packaged game immediately after Escape and a simulated controller B press, before any further navigation. At 720p, 1080p and ultrawide, only Settings was outlined. It stayed selected, cleared behind the reopened Settings page, and skipped disabled Continue. We checked those images alongside the passing tests before the implementation reached main. The production menu assignment and physical-device playtest remain separate steps. [Packaged review images](https://github.com/rthomas24/2026-epic-ai-megajam/releases/tag/i71-focus-662b8c04-dev).

## A package, a recording, and two different kinds of evidence

On November 14, engineering released Shipping build `6fe3e90f`, a 938,981,901-byte ZIP. The full engineering suite had passed 870 tests. An agent had traversed the campaign in the engineering build, using labelled debug moves for some stages. The package itself had a shorter launch check. We kept those results separate: neither established a complete human playthrough of the packaged game.

Nicholas asked to see and hear the current game. We verified the released ZIP, extracted it to an isolated folder and recorded the opening and early combat. Our first window-capture method kept returning the menu image while the game had already moved on. That take was rejected. The next take captured the foreground game area with audio from the same game process. It produced 200 seconds of actual play, with no replacement soundtrack. We retained the recorder's 70-millisecond packet-timestamp gap in the report rather than treating the video as a precise music-loop test. [Watch the recording and read its limits](LatestGameplayDemo01/README.md).

Story's review then found a gap that the build tests had not addressed: the opening still selected five legacy narrator captions. Updating the approved dialogue table would not fix the scene if the scene continued to select a different track. We sent the actual timestamps to engineering alongside the existing script-correction assignment. The captured build remains useful evidence of what played, including what still needs fixing.

## Keeping a supplied track intact until its scene is ready

Nicholas selected separate tracks for Cinder exploration and combat, and the original Cathedral Pulse for the core chamber. We preserved the supplied originals, decoded complete stereo masters and imported the SoundWaves. Separate Unreal processes reopened them and exported their samples for comparison. The two Cinder tracks retained their full 150.0- and 148.8-second durations; the core track retained 69.56 seconds. The check did not choose a new loop, entry offset or final mix.

The next dependency was the game's event contract: which scene owns the score, when dialogue yields it, and when the reactor fight gives way to escape. Engineering agreed to include the new music folders in its next normal cook and supply the actual phase events. Those agreements are recorded as pending work, not as music already heard in the campaign. [Cinder verification](../../../ArtSource/Audio/CinderMusic01/native-checks) · [Core verification](../../../ArtSource/Audio/DominionCore01/native-checks).

## November 15: the saved game was the test

A Cinder route that looked ready still failed when we resumed Nicholas's original saved game. The pilot reached the safety slab below the visible relief and could die in magma. Changing the spawn height did not establish that the visible floor had working collision: high-origin physics traces missed it too.

Engineering separated the terrain's collision into independently cooked tiles while preserving the geometry and the protected ground offset. The same six-point collision regression failed before the change and passed after it. An offscreen run from the original save then crossed the river and bridge to the conduit stage without a player death. We kept the claim narrow: that crossing did not prove the rest of the planet, boarding or the packaged campaign. [The recorded crossing result](https://github.com/rthomas24/2026-epic-ai-megajam/issues/194#issuecomment-6897705499).

The later Session58 replay continued that original save through the Warden, EMP recovery and field-anchor charges, then returned to the pad and boarded the ship. It used real movement and combat inputs with heading assistance and pauses, and included normal retakes and a return-bridge lava death after a missed alignment. Followers were disabled after repeated hazard knockdowns. The session exited normally with no logged errors. This establishes an assisted route to boarding in the private Development integration; it is not an uninterrupted human run, orbit departure or final-package acceptance. [Full route result and assistance disclosure](https://github.com/rthomas24/2026-epic-ai-megajam/issues/194#issuecomment-6898493473).

The soundtrack exposed two different state problems. A saved on-foot Cinder visit does not repeat the normal planet-descent reveal event. Music that waited only for that event could stay silent. We used the existing saved-pad checkpoint to restore the intended planet score, without pretending that a new descent had occurred.

Nicholas also asked for a selected Cinder track to play through and loop cleanly. The native session showed exploration and combat cues replacing one another as encounters changed. Enabling an asset's loop flag could not prevent those state-driven replacements. A separate candidate now retains the selected cue through routine encounter and transmission changes, while preserving takeoff and critical-scene handoffs. Its Editor compilation passed as Build43, followed by all twelve focused music-fallback regressions, including the repeated-event continuity test. Those headless tests had no audio-position callbacks: audio-on playback, a clean natural repeat, human listening and final-package presence remain separate gates. Offline audio recordings use a different clock from the session's wall timestamps, so elapsed log time alone is not a measured audible loop duration. [Cinder continuity issue](https://github.com/rthomas24/2026-epic-ai-megajam/issues/212).

For the escape, we preserved Nicholas's complete 127.6-second Ritual of the Final Dance rather than cutting it to fit a guessed scene length. The supplied source and its decoded 48 kHz stereo master have recorded fingerprints and measurements. That proves which file was prepared; it does not approve the final in-game mix or prove its presence in the submission package.

The same distinction shaped smaller presentation fixes: a scan overlay should close without stopping dialogue, a controller landing prompt should follow the active device, and removing one isolated opening voice should preserve its subtitle, timing and the shared finale recording. Focused tests help protect those contracts. The current campaign and final package still have to demonstrate the result.

Updated November 15 from the existing published record, pinned source and scoped verification receipts. The older 200-second review recording is not the final 30–60-second submission trailer. Final package-matched credits, media, listening/controller checks and human submission approval remain open. This account records work and its limits; it does not certify unresolved rights or final presentation quality.
