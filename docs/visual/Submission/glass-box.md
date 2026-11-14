# Building Triad Drift in four days

On November 12, our first Shipping build opened on a squadron of ships, a planet and an asteroid belt. There was no HUD. We had tested the HUD in Development. The fallback had been compiled out of Shipping, and the styled HUD was still off by default. A capture from outside the packaged game showed the failure immediately. The next build drew the ship cards, objectives and enemy markers.

We tried again from the main menu. An agent sent Enter, Tab and Esc to the running Windows build. Enter launched wave one, Tab cut from Lancer to Bastion, and Esc paused the game. Leaving the controls alone taught us something else: Rift could go down in the first thirty seconds. The team gave the opening wave a grace period. An early hyperdrive prompt also sat over the player’s ship. Seeing the prompt against a moving target told us where to move it. [Packaged gameplay capture](../../eng/evidence/shipping_keys_ship_2200.png)

## A material that looked right in the data

On Cinder and Verdance, crystal props and giant mushrooms appeared as black silhouettes. Their colour values looked right in the editor data. A rendered walk through both worlds gave engineering the place to look: the game log said the master material expected a mask texture but had received a colour texture as its default. With a project mask as the default, the amber and cyan crystals and glowing mushrooms appeared in the next captures. Our import checks gained a second step: render the prop in the game and read the material compiler log.

## How the two leads used AI

Ryan’s engineering team used Claude Code for gameplay and builds. Nicholas’s presentation team used OpenAI Codex for music, interface and story work. The teams shared a message board and claimed files before editing. For the enemy trooper, a builder fitted armour to the project mannequin in Blender. A separate reviewer compared the contact sheets to Ryan’s concept and pointed out that the slim limbs, boxy plates and dark face still read like the mannequin. Later versions changed the helmet, added visible eye panels and gave the grunt and heavy distinct shoulder widths. [Trooper review sheets](../../../ArtSource/EngCharacters/NullTrooper/review)

The music handoff exposed a different gap. Nicholas chose *Push The Attack* for the Dominion space assault. We preserved the complete 144.68-second track and its quiet ending. A second Unreal process reopened it and confirmed looping and the Music sound class. That check can establish what the file contains. Listening through the generator battle will establish whether the general combat track yields, where the loop returns, and whether it stops when the assault ends. [Audio verification record](../../../ArtSource/Audio/DominionAssault02/native_checkpoint_01.json)

Drafted November 13 from the [team development log](../../../DEVLOG.md). The full campaign playthrough and final submission build are still ahead of this account.
