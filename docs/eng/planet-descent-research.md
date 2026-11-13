# Planet descent: research and recommended design

Nov 13, 2026. Research report for the Lead. Read-only study of `lead/int`; nothing was built, launched or tested.

**How to read the marks.** Every claim carries one of:
- **[S]** sourced from the web, URL given. "(snippet)" means only a search summary was read, not the page.
- **[C]** read from our code or docs, path given (all paths are under `.wt/int/`).
- **[I]** my own inference or design judgement. Not verified.

Two things could not be done and are flagged where they matter: the two Hello Games GDC 2017 talks could not be
read in detail (GDC Vault and transcripts were blocked), and no timing was measured in our game (no build or
launch was allowed).

---

## 1. One-page summary

### 1.1 Recommendation

**Build architecture A+: keep the separate planet maps, hide the level load inside the brightest second of the
plasma phase, arrive where and how the player hit the atmosphere, and add an Elite-style forced "glide" on the
planet side that carries the ship from high altitude through a cloud deck down to free flight.** [I]

Why this one:
- The load today is a 0.6 s fade to black, then the `L_Transition` loading-screen map for at least 1 s, then the
  planet map [C: `Flow/TDGameInstance.cpp:253-258, 400-409`, `Flow/TDFlowSettings.h:38-42`]. Replacing black and
  a loading screen with a white-hot plasma frame that is already on screen costs a few hours and removes the
  only visible break. A frozen frame that is one flat colour cannot be told from a running one. [I]
- The arrival is hard-wired: 600 m behind the pad, 300 m up, nose on the pad, whatever the player did in space
  [C: `Travel/TDPlanetTravelRig.h:61`, `TDPlanetTravelRig.cpp:118-124`, `Ground/Planets/TDPlanetDirector.cpp:166`].
  Deriving it from the entry point is a pure-math change with one data hand-off.
- "The world comes into focus" needs altitude. Today the player appears 300 m above the pad. A glide from about
  3 km gives 15 to 20 seconds of cloud, haze and terrain resolving, and gives the terrain time to refine. [I]
- True continuity (B) and a real sphere (C) are 30+ and 100+ engineer-hours at high risk, against 45 hours with
  five engineers already loaded. Section 5.1 has the comparison.

**What to cut first, in this order** [I]: (1) ship airbrake / heat-shield animation; (2) Volumetric Cloud
component, falling back to a card cloud deck; (3) landing anywhere off the pad; (4) the polished take-off
reverse (keep a short mirrored version); (5) the baked globe texture. Never cut: the white cover replacing the
loading screen, and arrival from the entry point. Those two are the feature.

### 1.2 Beat sheet (target, after stage 2)

Altitudes in space are above the globe surface; on the planet they are above pad level (z = 0). All [I] except
the numbers marked [C].

| # | Beat | Where | Time | What the player sees and can do |
|---|---|---|---|---|
| 1 | Approach | Galaxy, 700 m out | free | Hyperdrive drops out at 1.8 x atmosphere radius (1,296 m from centre on Verdance) [C]. Globe shows the real continents, sea and volcano, and a beacon glint at the landing site |
| 2 | Orbit band | 696 m down to 120 m altitude | free | Full control. Circle the globe, pick a spot. The planet map preloads in the background |
| 3 | Interface | 228 m to 120 m | about 2 s at cruise | First wisps streak past, faint orange on the nose, low rumble, HUD "ATMOSPHERE". Still free to pull out |
| 4 | Commit and plasma | 120 m, the existing trigger [C] | 3.0 s | Ship committed, steering 35% [C], invulnerable [C]. Glow orange to pink to white, shake to the cap, FOV +6, roar, other sound muffled, hull-temperature bar climbs. Screen is fully white-pink from 2.4 s |
| 5 | Hidden load | white screen | target under 2.5 s, tolerated to 6 s | `OpenLevel` straight to the planet. A viewport overlay and the roar loop both survive the level change. No black, no loading screen |
| 6 | Burn-through | Planet, about 3,000 m | 1.5 to 2 s | White thins to streaks. Sonic boom, sound opens up. Ship is where the player aimed, on the heading they had, above a cloud deck with the hero peak poking through |
| 7 | Glide | 3,000 m to 2,050 m | about 8 s | Forced glide, 120 m/s, pitch held between 20 and 50 degrees down, yaw free. Altimeter spins down. Hull temperature falls |
| 8 | Cloud punch | 2,050 m to 1,900 m | about 2 s | Wisps, a one-second whiteout, buffeting, canopy droplets, then out of the cloud base |
| 9 | Focus | 1,900 m to 900 m | about 8 s | Terrain at full detail under the glide path, fog thins, trees and rocks fade in, the pad marker shows range |
| 10 | Hand-back | about 900 m above ground | 2 s | Airbrake thump, speed falls to the atmosphere cap (36 m/s cruise, 65 m/s boost [C]). Free flight |
| 11 | Land | pad | existing | Hold F within 400 m [C]. Stretch: hold F over any flat ground |
| 12 | Take-off | climb past about 1.2 km above ground [C] | 4 s | Reverse: forced climb through the cloud deck, sky darkens, white flash, load, emerge above the point of the globe you left from |

### 1.3 Staged plan

| Stage | On `main` by | Contents | Engineer-hours |
|---|---|---|---|
| **1** | tonight | White cover that survives the load; no black fade and no `L_Transition` for planet entry and exit; arrival point and heading from the entry; terrain's first view built around the arrival; roar loop persists; timing log; legacy switch | about 11 |
| **2** | tomorrow midday | Glide phase; cloud deck; plasma, streak and heat effects; sound layers; altimeter and hull-temperature HUD; globe texture baked from the terrain field with pad beacon; map preload during orbit; mirrored take-off | about 22 engineering + 12 builder |
| **Stretch** | only if stage 2 is played and solid | Land anywhere; Volumetric Cloud upgrade; airbrake animation; chunk data precomputed in orbit; bigger globes | 4 to 8 each |

### 1.4 Three biggest risks

1. **The load time is unmeasured.** If `OpenLevel` plus the terrain's synchronous first build takes over about
   6 s on the target machine, the white hold reads as a freeze. Stage 1's first task is to log it. [I]
2. **The overlay and the sound persisting across `OpenLevel`** rest on my knowledge of the engine, not on a
   page I could read. Verify in the first hour; the fallback (white camera fade on both sides) still beats black. [I]
3. **Scale.** The globe is a 600 m radius ball [C] and the ground is 16 km across [C]. The mapping is a
   deliberate cheat. It reads fine if the globe texture matches the ground; it reads as a trick if it does not. [I]

---

## 2. Research

### 2.1 No Man's Sky

**Not verified:** region sizes, LOD counts, the worker-thread and GPU split, and how the low-detail planet seen
from space is produced. These are in McKendrick's talk, which I could not open. Watch
https://www.youtube.com/watch?v=sCRzxEEcO2Y and https://www.youtube.com/watch?v=C9RyEiEzMiU before quoting numbers.

How planets are represented:
- The pipeline is voxel generation, then polygonisation, texturing, population and simulation, running
  continuously in real time. [S (snippet): https://www.gamedeveloper.com/programming/video-how-continuous-world-generation-works-in-i-no-man-s-sky-i- ; https://gdcvault.com/play/1024265/]
- Voxel regions are organised as octrees on a cube and projected onto a sphere; voxel data sits in flat arrays;
  whole planets are textured triplanar. From McKendrick's earlier 2015 talk.
  [S: https://procedural-generation.isaackarth.com/2016/06/22/building-a-galaxy-procedural-generation-in-no.html]
- Each planet is a cube projected to a sphere. A flat map wrapped on a sphere was tried and rejected. Terrain
  is voxels with procedural textures; props and creatures are meshes; everything is deterministic from a seed
  and nothing is stored. [S: https://en.wikipedia.org/wiki/Development_of_No_Man%27s_Sky]
- Worlds Part I (2024) rewrote terrain generation to dual marching cubes for speed and moved trees, rocks and
  grass to a GPU system. [S: https://nomanssky.com/worlds-part-i-update/]

Seamlessness and pop-in:
- At launch, outlets reported pop-in when flying over land. [S (snippet): https://www.tweaktown.com/news/53355/mans-sky-suffers-fps-drops-texture-pop-ins-ps4/index.html]
- PC Gamer's review: landing "may technically be seamless, but watching half a mountain slowly form itself in
  front of my visor doesn't make it feel that way". [S (snippet): https://www.pcgamer.com/no-mans-sky-review/]
- NEXT (2018): "Improved terrain lods and visuals when flying in from space", "massively improved draw
  distance", fixes for terrain tiles slow to appear, and planet surfaces that "look more interesting and
  representative from orbit". [S: https://www.nomanssky.com/next-update/]
- Origins (2020): terrain rendering improved, fixes for grass and objects popping near the player.
  [S: https://www.nomanssky.com/origins-update/]
- 2024: an engine programmer flew a debug camera across three planets in about 50 s on PS5 with no loading
  screens [S: https://80.lv/articles/3-planets-in-under-a-minute-a-closer-look-at-the-power-of-no-man-s-sky-s-engine];
  pop-in is still visible in that clip [S (snippet): https://gamesradar.com/games/open-world/no-mans-sky-dev-proves-its-not-all-smoke-and-mirrors-in-space-games-by-zipping-across-3-planets-in-50-seconds-to-show-how-powerful-the-engine-really-is].

Atmospheric entry, clouds, orbit view:
- Atmospheres are deliberately "inverted in density" to make take-off and landing transitions more dramatic.
  [S: https://en.wikipedia.org/wiki/Development_of_No_Man%27s_Sky]
- Worlds Part II (2025): "The atmosphere entry effect has been overhauled". [S: https://nomanssky.com/worlds-part-ii-update/]
- The pulse engine cannot be used on or too close to a planet. [S (snippet): https://www.shacknews.com/article/96231/no-mans-sky-how-to-fix-your-ship-repair-launch-thrusters-refuel-pulse-engines]
- NEXT added dynamic volumetric clouds and better clouds seen from space [S: https://www.nomanssky.com/next-update/].
  Murray on the 2024 rewrite: "you see them from space, from planet, you can fly through them"
  [S: https://nomanssky.com/2024/07/no-mans-sky-worlds-part-i].
- Origins: "the clouds seen from space now match the current atmospheric conditions of the planet below".
  [S: https://www.nomanssky.com/origins-update/]
- The exact look of the entry effect (glow round the cockpit, shake) and its sound: not sourced. [I]

Landing:
- Take-off costs launch fuel except from landing pads [S (snippet): https://nomanssky.fandom.com/wiki/Launch_Thruster].
- One-button auto-land that picks a spot under the ship: common knowledge, not sourced. [I]

What to take from it [I]:
- Players forgive a short transition. They do not forgive terrain forming in front of them. Hide refinement
  behind cloud, haze and speed.
- Hello Games spent four years of patches on "the planet from orbit should look like the ground". It matters.
- The dense upper atmosphere is a design decision, not physics. Exaggerate it.

### 2.2 Other games

**Elite Dangerous** (the model for our glide):
- Sequence: supercruise, orbital cruise, glide, planetary flight. The altimeter marks the orbital-cruise and
  drop heights. Pitching up too far aborts the glide; it ends by itself near the surface.
  [S (snippet): https://community.elitedangerous.com/node/348]
- Players report a fixed 2,500 m/s glide ending at a few kilometres, entered at 24 to 35 km, with a usable
  pitch of about 5 to 60 degrees down. [S, player posts: https://steamcommunity.com/app/359320/discussions/0/3041605810453423687 ; https://forums.frontier.co.uk/threads/the-popular-guide-to-planetary-landings.216183/page-2]
- That the glide exists to cover streaming and the change of flight mode is my inference; one UE forum thread
  describes Elite as using instanced areas. [I; S, community: https://forums.unrealengine.com/t/landing-on-planets-in-ue-how-do-games-like-space-engineers-and-empyrion-do-this/629882]

**Star Citizen**: 64-bit positioning was a fundamental engine change [S: https://gamersnexus.net/gg/2622-star-citizen-sean-tracy-64bit-engine-tech-edge-blending];
object container streaming, in development from 2016, loads and unloads the world with no loading screens
[S: https://starcitizen.tools/Object_Container_Streaming]. Years of engine work. Not for us. [I]

**Outer Wilds**: island-sized planets tens of kilometres apart, with physics bent to suit
[S (snippet): https://pcgamer.com/how-to-build-a-black-hole]. Real spheres work when they are tiny and
hand-built. Our planets are flat 16 km heightfields, so this does not transfer. [I]

**Starfield**: Todd Howard said space and surface are separate "realities" and the segue is "really just not
that important to the player" [S: https://www.thegamer.com/starfield-wont-feature-seamless-space-planet-landing/];
it became one of the main launch criticisms [S (snippet): https://www.gamesradar.com/games/rpg/starfield-modder-finally-brings-seamless-planetary-landing-to-bethesdas-rpg-without-any-loading-screens-after-over-a-year-of-work/].
The lesson: the same load is judged very differently when the player never loses the view from the cockpit. [I]

**Kerbal Space Program**: re-entry effects are driven by velocity and air density
[S (snippet): https://forum.kerbalspaceprogram.com/topic/168883-aerodynamic-effects]. Drive our effect from
two numbers the same way: depth into the atmosphere and speed. [I]

**Disguised loads**: God of War Ragnarok's director said any point where the player is blocked both ways gives
time to load [S (snippet): https://exputer.com/news/games/loading-screens-god-of-war-ragnarok/]. Our committed
dive is that point: the player has already given up control for 2.5 s [C: `Travel/TDAtmosphereZones.h:23`].

**Ace Combat 7** (cloud punch-through): entering cloud causes vibration and icing and wets the canopy; cloud
edges were given stronger noise because soft edges read poorly; clouds cost 30 to 40% of GPU after
optimisation. [S: https://www.gamebusiness.jp/article/2019/11/06/16410.html ; https://www.skywardfm.com/post/the-form-and-function-of-clouds-and-weather-in-ace-combat-7]

**Everspace 2** keeps separate worlds for space and planetary locations with a brief visible load and says it
cannot hide it [S: https://www.everspace.game/en/?p=1369]. We can do better because our cover is already
full-screen white. [I]

### 2.3 Unreal Engine techniques

Sky Atmosphere:
- Epic says it supports moving "seamlessly from the planet's surface through the atmosphere to outer space".
  The smallest documented planet is 300 km radius; more than one atmosphere on screen is not supported; views
  from space can hitch when the renderer switches method.
  [S: https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-atmosphere-component-in-unreal-engine]
- So: do not put a Sky Atmosphere on our 600 m globes in the galaxy (two planets, tiny radius, both outside
  what is documented). Keep the globe's own atmosphere shell material. Use Sky Atmosphere on the planet map
  only, where the placeholder sky already has one [C: `Flow/TDPlaceholderSky.h:80`]. [I]
- Aerial Perspective View Distance Scale thickens haze with distance
  [S: https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-atmosphere-component-properties-in-unreal-engine].
  Animating it down during the descent is a cheap "comes into focus" control. [I]

Volumetric Cloud:
- Epic says it suits "flying, and ground to outer space transitions". `r.VolumetricRenderTarget.Mode 0`
  (quarter-resolution trace) is the mode for fast flight through clouds.
  [S: https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-cloud-component-in-unreal-engine]
- Layer Bottom Altitude and Layer Height are in km; with a Sky Atmosphere present the cloud uses the
  atmosphere's planet radius.
  [S: https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-cloud-component-properties-in-unreal-engine]
- Cost on our target machine is unknown. Ace Combat's 30 to 40% is the warning. [I]

Hiding a level load:
- Non-seamless travel blocks the game thread [S: https://dev.epicgames.com/documentation/en-us/unreal-engine/travelling-in-multiplayer-in-unreal-engine].
  We use `UGameplayStatics::OpenLevel` [C: `Flow/TDGameInstance.cpp:277`].
- Preload: `LoadPackageAsync` the map, hold a reference, then `OpenLevel` finds it loaded. Results are mixed:
  one user still froze 10 s. [S, community: https://forums.unrealengine.com/t/pre-load-level/117817 ; https://forums.unrealengine.com/t/async-load-map/99939]
- A World Partition map streamed in as a sub-level is reported not to load its partitioned content
  [S, community: https://forums.unrealengine.com/t/streaming-different-world-partitioned-levels/1152287].
  Our maps are World Partition [C: `AGENTS.md` section 3]. This rules out "stream the planet map into the galaxy". [I]
- Seamless travel needs a transition map and Epic's docs are silent on single-player use
  [S: https://dev.epicgames.com/documentation/en-us/unreal-engine/travelling-in-multiplayer-in-unreal-engine].
  Not worth the risk now. [I]
- PSO precaching: the first draw of a new material can hitch; hold a cover until
  `FShaderPipelineCache::NumPrecompilesRemaining()` is 0.
  [S: https://dev.epicgames.com/documentation/en-us/unreal-engine/pso-precaching-for-unreal-engine]
- From my knowledge of the engine, not confirmed on a page: a Slate widget added with
  `UGameViewportClient::AddViewportWidgetContent` stays up across `OpenLevel` (UMG widgets added to the
  viewport do not), and `UGameplayStatics::SpawnSound2D` / `CreateSound2D` take
  `bPersistAcrossLevelTransition`. **Verify both first.** [I]

Large World Coordinates: the default world limit is 88 million km and types are doubles
[S: https://dev.epicgames.com/documentation/en-us/unreal-engine/large-world-coordinates-in-unreal-engine-5],
but shadow precision problems are still reported beyond about 1 million units
[S, community: https://forums.unrealengine.com/t/cascaded-shadows-break-beyond-1-million-units/2500306].
Putting a 160 km terrain at an offset inside the galaxy world is possible but not free. [I]

Runtime terrain:
- Procedural mesh components are not in Nanite's supported list
  [S: https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine]
  and get no distance fields [S: https://dev.epicgames.com/documentation/en-us/unreal-engine/mesh-distance-fields-in-unreal-engine].
- `bUseAsyncCooking` cooks collision off the game thread
  [S: https://docs.unrealengine.com/4.27/en-US/API/Plugins/ProceduralMeshComponent/UProceduralMeshComponent/bUseAsyncCooking/index.html].
  We already use it for refreshes and turn it off for the first build [C: `World/TDPlanetTerrain.cpp:98, 119-121`].

Effects, camera, sound:
- Post-process materials can offset the scene-colour lookup, the basis of radial blur and heat distortion;
  each pass costs; give every parameter a neutral default.
  [S: https://dev.epicgames.com/documentation/en-us/unreal-engine/post-process-materials-in-unreal-engine]
- Camera shakes: Perlin-noise pattern for rumble, location, rotation and FOV channels, playable from gameplay.
  [S: https://dev.epicgames.com/documentation/en-us/unreal-engine/camera-shakes-in-unreal-engine]
- MetaSound inputs are set from gameplay through the audio component
  [S: https://dev.epicgames.com/documentation/en-us/unreal-engine/metasounds-reference-guide-in-unreal-engine];
  `SetSubmixEffectChainOverride` cross-fades a submix to a new effect chain, which is how to muffle and reopen
  the mix [S: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/AudioMixer/UAudioMixerBlueprintLibrary/SetSubmixEffectChainOverride].

Baking a texture from code:
- `UTexture2D::CreateTransient` makes a texture from pixel data
  [S: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UTexture2D/CreateTransient];
  an editor-only call saves a render target as an asset
  [S: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UKismetRenderingLibrary].
- Azimuthal equidistant projection keeps true distance along lines from the centre; sideways stretch is
  c / sin c, which is 1.57 at 90 degrees from the centre and unbounded at the far point.
  [S: https://mathworld.wolfram.com/AzimuthalEquidistantProjection.html]

### 2.4 The feel of entry

Real re-entry, as crews describe it:
- Shuttle: an "intense, orange-pink glow" around the windows, and at peak heating a white-hot plume flashing
  behind the orbiter. [S (snippet): https://www.smithsonianmag.com/blogs/air-space-museum/2024/01/12/space-shuttle-astronauts-tell-all/]
- Orion 2014: plasma starts as a gradual glow and becomes "brilliant magenta"; communications black out;
  thruster puffs are visible. [S: https://www.planetary.org/articles/20141219-ride-along-orion]
- Reported colour order on Orion: white, yellow, lavender, magenta at peak.
  [S (snippet): https://gizmodo.com/you-must-watch-the-acid-trip-of-a-light-show-orion-prod-1673372283]
- Apollo 11's Collins: "all the colors of the rainbow" with an orange and yellow core; a lunar return has a
  blackout of about six minutes and over 3 g. [S: https://spectator.com/article/how-artemis-ii-returned-to-earth/?edition=us]

What reads as "piercing the atmosphere" on screen [I, drawing on the sources above]:
1. **A colour progression, not one colour.** Faint orange, then orange-pink, then magenta-white at peak. Our
   squad colours are magenta, cyan and amber [C: `AGENTS.md` section 2], so tint the peak per ship.
2. **Flicker.** Crews describe strobing, not a steady glow. Pulse the sheath at 8 to 14 Hz with noise.
3. **A shock cone ahead and streaks behind.** The cone sits on the nose; streaks run back past the camera.
4. **Build, peak, release.** Shake and roar rise for 2.5 s, hold, then drop sharply at burn-through. The release
   is what makes the peak feel violent.
5. **Blackout.** Muffle everything except the roar; cut radio barks. Then open the mix with a boom.
6. **Two whiteouts with a view between them.** Plasma, then a clear high view of the cloud tops, then the cloud.
   The clear gap proves to the player that nothing was swapped.
7. **Numbers that move.** An altimeter running down and a hull-temperature bar going up then down.
8. **Detail resolving in order.** Sky and horizon, then mountains through haze, then ground colour, then
   rivers and lava, then trees, then the pad. Each with its own altitude.

---

## 3. What the game does today

### 3.1 Step by step, from pointing at the planet to wheels down

| # | What the player sees | Code path | Cut, fade or load |
|---|---|---|---|
| 1 | Planet marker revealed after waves 2 and 4; the atmosphere "opens" only in that planet's phase | `World/TDPlanetAtmosphere.cpp:223-248` | none |
| 2 | Hyperdrive drops out at the gravity well, 1.8 x the atmosphere radius | `TDPlanetAtmosphere.cpp:165`, `.h:93` | none |
| 3 | Free flight to the globe from any direction. The globe is a sphere mesh with surface, atmosphere shell and ring materials, spinning 0.3 deg/s | `TDPlanetAtmosphere.cpp:62-87, 294-298`, `World/TDGalaxyLayout.cpp:234-266` | none |
| 4 | Crossing the atmosphere radius anywhere on the sphere starts the entry (overlap, plus a per-tick distance check) | `TDPlanetAtmosphere.cpp:250-282, 327-336` | none |
| 5 | Entry: 2.5 s. Nose is snapped toward the planet if facing away. Speed held (at most 90 m/s, clamped so the dive ends 30 m above the surface), steering 35%, hyperdrive off, invulnerable. The `AtmosphericEntry` cue loops on the ship | `Travel/TDAtmosphereZones.cpp:83-156`, `.h:66-81` | none |
| 6 | Placeholder feedback when the cue has no art: shake up to 0.9 deg, FOV +4, orange glow bursts on the nose, on a bell curve | `Flight/TDFlightFeedbackComponent.cpp:410-422` | none |
| 7 | `RequestLevelTravel` then `TravelTo`: camera fades **to black** over 0.6 s | `TDAtmosphereZones.cpp:158-177`, `Flow/TDGameInstance.cpp:223-266`, `Flow/TDFlowSettings.h:38` | **fade to black** |
| 8 | `OpenLevel(L_Transition)`: the loading-screen map and widget, held at least 1.0 s | `TDGameInstance.cpp:400-409, 306-312`, `TDFlowSettings.cpp:15`, `Flow/TDFrontEndGameModes.h:126-137` | **blocking load + loading screen** |
| 9 | `OpenLevel(L_Planet_<name>)` | `TDGameInstance.cpp:281-285`, `TDFlowSettings.cpp:11-12` | **blocking load** |
| 10 | Ground game mode spawns the planet director, travel rig (pad at the map origin), course, placeholder sky and terrain. The terrain waits for the course (up to 10 s), then builds all 256 chunks in one synchronous pass with collision | `Ground/TDGroundGameMode.cpp:179-184, 267`, `TDPlanetDirector.cpp:120-168`, `World/TDPlanetTerrain.cpp:86-121, 335-338` | **hitch, not measured** |
| 11 | Squad spawns at the fixed descent start: 600 m behind the pad, 300 m up, aimed at a point 30 m above the pad (about 27 degrees down) | `TDPlanetTravelRig.h:61`, `.cpp:118-124`, `TDPlanetDirector.cpp:166` | hard cut in |
| 12 | Atmosphere flight: gravity 1,200, speed capped at 60% (36 m/s cruise, 64.8 m/s boost), no hyperdrive, soft walls and ceiling | `TDAtmosphereZones.cpp:326-352`, `Flight/TDFlightTypes.h:26-30`, `Flight/TDFlightSim.h:85` | none |
| 13 | Pad carries the objective marker. Hold F within 400 m for the approach assist, within 120 m to land; auto-land, wingmen beside you, hand-off to first person | `features.md:785-796`, `Travel/TDLandingComponent.*` | none |
| 14 | Leaving: once objectives are done, climb to about 1.2 km above the ground under the ship plus 30 m; `TravelTo(Galaxy, "<Planet>_Orbit")` with the same black fade and loading screen; arrive at a fixed orbit point 200 m outside the atmosphere, facing the combat zone | `TDAtmosphereZones.cpp:455-478`, `TDGalaxyLayout.cpp:268-272` | **fade, loading screen, load** |

So from commit to control on the planet the player gets: 2.5 s of effect, 0.6 s to black, a loading screen of at
least 1 s plus two blocking loads, then a hard cut to a fixed spot. [C, as cited]

`storyline.md` is not on `lead/int`; it exists only in `.wt/t5/storyline.md` and its section 5 is the campaign
beat table, which says nothing specific about the descent. `animations.md` section 5.3 lists landing gear,
settle and lift as "placeholder" and has no entry animation row. [C: `animations.md:96-101`]

### 3.2 Scale numbers

| Quantity | Value | Source [C] |
|---|---|---|
| Verdance globe radius / atmosphere trigger | 600 m / 720 m | `TDGalaxyLayout.cpp:237-238` |
| Cinder globe radius / atmosphere trigger | 550 m / 660 m | `TDGalaxyLayout.cpp:255-256` |
| Entry altitude above the globe | 120 m / 110 m | derived |
| Hyperdrive drop-out | 1.8 x atmosphere radius: 1,296 m / 1,188 m from centre | `TDPlanetAtmosphere.h:93` |
| Orbit return point | atmosphere radius + 200 m, toward the combat zone | `TDGalaxyLayout.cpp:271` |
| Galaxy play boundary | 9 km radius | `TDGalaxyLayout.h:95` |
| Space cruise / boost | 60 m/s / 108 m/s | `Flight/TDFlightTypes.h:26, 30` |
| Entry duration, speed, steering, clearance | 2.5 s, up to 90 m/s (min 15), 35%, 30 m | `TDAtmosphereZones.h:66-81` |
| Arrival point | pad + (-600 m, 0, +300 m), aimed at the pad | `TDPlanetTravelRig.h:61` |
| Atmosphere cruise / boost cap | 36 m/s / 64.8 m/s | 60% of the above; `features.md:782-784` |
| Terrain collision core | 16 x 16 km (half 8 km), 1 km chunks, 256 chunks | `World/TDTerrainField.h:39`, `TDPlanetTerrain.h:43` |
| Terrain far ring (no collision) | 80 x 80 km (half 40 km) | `TDTerrainField.h:40` |
| Chunk detail | 64 / 24 / 8 quads per side; switches at 1.8 km and 4.5 km | `TDPlanetTerrain.h:44-49` |
| Hero peak | 2,500 m, about 7 km from the course; a volcano on one planet | `TDTerrainField.cpp:43-46, 65-67` |
| Sea or lava sea | centred 5.8 km out, radius 3.6 km | `TDTerrainField.cpp:294`, `.h:109` |
| Flight zone after fitting | 17 x 17 km; ceiling 400 m above the highest ground in it | `TDAtmosphereZones.h:195-199`, `.cpp:354-391` |
| Exit height | about 1.2 km above the ground under the ship, plus 30 m | `TDAtmosphereZones.cpp:457-466`, `TDPlanetTravelRig.h:50` |
| Travel timeout | 30 s, then control is handed back | `TDFlowSettings.h:46` |

### 3.3 What is hard-wired

- **Arrival position and heading**: a constant offset from the pad. The entry point, direction and speed are
  thrown away. `ReturnPoint` carries only the name of the orbit point for the trip back.
  [C: `TDPlanetTravelRig.cpp:118-124`, `TDAtmosphereZones.cpp:167`]
- **Pad transform**: the map origin [C: `Ground/TDGroundGameMode.h:58`].
- **Terrain's first detailed view**: centred on the pad (`Pad = ZeroVector`) [C: `TDPlanetTerrain.cpp:110-117`].
  It also carves an approach cone under the fixed glide slope from -X [C: `TDTerrainField.h:31-35`].
- **Return to space**: one fixed orbit point per planet [C: `TDGalaxyLayout.cpp:268-272`].
- **The cover is black** and the route goes through `L_Transition` for every destination
  [C: `TDGameInstance.cpp:256, 405-408`].
- **The globe textures are art**, not derived from the terrain field: `T_Planet_Verdance_BC/_N/_ORM/_E/_Clouds`,
  the same five for Cinder, `M_TD_Planet`, `MI_Planet_Verdance`, `MI_Planet_Cinder`, `M_TD_PlanetAtmosphere`,
  `MI_PlanetAtmo_*`, `SM_Planet_Sphere`, `SM_Planet_Atmosphere`, `SM_Planet_Ring`, `M_TD_PlanetRing`
  [C: names in `Content/TriadDrift/Gameplay/Props/Planets/`].

What already works in our favour [C]:
- Entry can be triggered from any direction on the sphere (`TDPlanetAtmosphere.cpp:327-336`).
- The entry already exposes `GetEntry01()` for effects (`TDAtmosphereZones.h:58-60`) and already handles death,
  retake, failed and timed-out travel (`TDAtmosphereZones.cpp:219-257`).
- The terrain field is pure, const and thread-safe (`TDPlanetTerrain.h:99-100`), so it can be sampled from the
  galaxy world, an editor command or a worker thread.
- The terrain has `SetViewOverride`, `IsBuilt`, `GetPendingCount` and `GetHeightAt` (`TDPlanetTerrain.h:63-72`).
- The planet's content is spawned by code, not placed in the map (`TDPlanetDirector.cpp:120-168`).
- `OpenLevelOverride` lets tests exercise travel without loading maps (`TDGameInstance.h:218-219`).

---

## 4. Architectures compared

Hours are engineer-hours to "technically verified", excluding effects and art. All [I].

| | A+ Hidden load with glide (recommended) | B Terrain inside the galaxy world | C Real spherical planet |
|---|---|---|---|
| What it is | Separate maps. Load hidden in the plasma whiteout. Arrival from the entry. Forced glide on the planet side | Spawn terrain, course and sky into the galaxy world far from the play space; cross-fade globe to heightfield; no level change | Cube-sphere terrain with LOD, radial gravity, curved flight |
| Player perceives | No break. One white-hot second, then high over the world they aimed at | Truly continuous, in principle | The real thing |
| Engineering | 33 h over two stages | 30 to 45 h | 100+ h |
| Risk | Low to medium. Each piece is small and has a fallback | High. Two game modes merged; space sky and planet sky in one world; on-foot hand-off, checkpoints and about 30 travel and planet tests assume separate maps; Nick's tagged planet environment lives in the planet maps; precision at large offsets | Very high. Flight, landing, AI, course, on-foot and terrain all assume a flat world with +Z up |
| What breaks | Nothing that exists. A legacy switch restores today's flow | `main` playability while it lands | Everything on the planets |
| Verdict | **Build** | Reject for this jam | Reject |

Hybrids considered:
- **A without the glide** (stage 1 only): the minimum. It removes the black and the loading screen and fixes the
  arrival, but the player still appears low and the world does not "resolve". This is the fallback if stage 2 slips.
- **Elite-style glide on the space side instead**: makes the dive toward the globe longer. Not useful, because
  the globe is only 120 m below the trigger and its texture is soft close up. Put the long phase where the
  detail is: on the planet. [I]
- **Preload plus `OpenLevel`**: part of A+, stage 2. It shortens the white hold; it does not change the design.
- **Seamless travel**: would keep the frame loop alive during the load. Undocumented for single player and
  untested here. Not now.

---

## 5. The recommended design in detail

### 5.1 The mapping: a point on the globe to a place and heading on the ground

The globe is 600 m in radius and the ground is flat and 16 km wide, so the mapping is a projection, chosen so
that the landing site is undistorted and the far side of the globe is the far edge of the world. [I]

**Frame.** Give each planet a fixed "site pole": the point on the globe that stands for the landing pad. Point
it at the combat zone, so a normal approach arrives near the site. Stop the surface spin (set
`SpinDegPerSecond` to 0 for the surface; today it is 0.3 [C: `TDPlanetAtmosphere.h:41`]) so the pole stays put.
Let the cloud layer drift in the material instead.

**Forward map (space to ground), azimuthal equidistant:**
- `d` = unit vector from the planet centre to the ship at commit.
- `p` = unit vector to the site pole. `m` = a fixed reference direction at right angles to `p` (world up,
  flattened against `p`).
- `c = acos(d . p)`, from 0 at the site to pi at the far side.
- `az = atan2(d . (p x m), d . m)`.
- Ground range `r = Rmap * c / pi`, with **Rmap = 7.5 km**, which keeps every point inside the 8 km collision
  core and puts the hero peak (7 km out) and the sea (5.8 km out) on the globe.
- Aimed ground point, in course-local XY: `A = r * (cos az, sin az)`.

**Heading.** Take the ship's velocity, remove the part along `d`, and measure its angle `beta` from the
direction toward the pole along the surface. On the ground, "toward the pole" is "toward the pad". So the
ground heading is the direction from `A` to the pad, turned by `beta`. If the tangential speed is tiny (a
straight-down dive), head for the pad.

**Steepness.** The angle between the velocity and straight down sets the glide pitch: a vertical dive gives 45
degrees down, a grazing entry gives 25.

**Where the glide starts.** The player should end the glide over the point they aimed at, so start it one
glide-length back along the heading: `Start = A - heading * 1.8 km`, clamped to 7.5 km from the pad, at about
3,000 m above pad level and at least 150 m under the fitted zone ceiling.

**Finding the pad from anywhere.** The pad already carries the HUD objective marker while flying
[C: `features.md:794`]. Add range in km to that marker. From the worst case (far side of the globe, 7.5 km) it
is about 2 minutes at boost; from a normal approach it is under 30 s. On the globe, a pulsing beacon at the site
pole tells the player where to aim for a short trip. [I]

**The same map, inverted, bakes the globe texture** (section 5.4) and gives the take-off return point
(section 5.3). One function pair, `GlobeToGround` and `GroundToGlobe`, in one header, used by all three. [I]

### 5.2 Descent, beat by beat

**Beat 1-2, approach and orbit (galaxy).**
- Nothing changes in flight. Between the drop-out radius and the trigger the player has about 580 m of depth to
  fly around the globe. One low circuit at cruise is about 75 s (circumference at 720 m is 4.5 km).
- When the player enters the gravity well of an open planet, the flow starts `LoadPackageAsync` on that planet's
  map and holds the reference (stage 2).
- HUD: a thin ring cue, "ORBIT - DIVE TO ENTER", and the site beacon.

**Beat 3, interface (galaxy), 1.15 x to 1.0 x atmosphere radius.**
- A new 0..1 value, `AtmoDepth`, from distance. It drives: wisp streaks (camera-attached sprites), a faint
  orange nose glow, a low rumble, light shake (0.1 deg), HUD "ATMOSPHERE" with a hull-temperature bar at 10%.
- The player can still turn away; everything fades back out. Skimming the top of the atmosphere to make the
  ship glow is a toy in itself.

**Beat 4, commit and plasma (galaxy), 3.0 s.** Extend `EntrySeconds` from 2.5 to 3.0 [C: `TDAtmosphereZones.h:66`].

| t (s) | Sheath colour | Shake (deg) | FOV | Sound | Screen cover |
|---|---|---|---|---|---|
| 0.0 | orange, thin, nose only | 0.3 | +0 | roar starts; mix begins to muffle | 0% |
| 1.0 | orange-pink, wraps the hull, flicker starts | 0.8 | +3 | roar 60%; barks cut | 10% (edge glow) |
| 2.0 | magenta-white, shock cone, heavy streaks | 1.5 (the cap [C: `TDFlightFeedbackComponent.h:175`]) | +6 | roar 100%, all else low-passed | 50% |
| 2.4 | white | 1.5 | +6 | peak | 100% |
| 3.0 | white | - | - | roar holds | 100%: travel fires |

- Control: steering 35% as today. Ship switching locked, as today.
- The ship keeps moving toward the globe at the clamped speed (about 30 m/s), which is slow, so streaks and blur
  carry the sense of speed.
- Record the arrival (section 5.1) at commit, in planet-local terms, into the game instance.

**Beat 5, the hidden load.**
- At 3.0 s: `OpenLevel` on the planet map directly. No camera fade, no `L_Transition`. The white cover is a
  Slate overlay owned by the game instance on the game viewport, so it is still there on the first frame of the
  new world. The roar is a 2D loop flagged to persist across the level change.
- Budget: under 2.5 s is invisible. Up to 6 s is a long white hold with sound, acceptable once. Over that, do
  the stage 2 preload and the stretch precompute; never bring back the loading screen for this route.
- If travel fails or times out, the existing hand-back runs [C: `TDAtmosphereZones.cpp:245-256`]; the overlay
  must fade out on the same condition and has its own hard limit of 12 s.

**Beat 6, burn-through (planet), about 3,000 m.**
- The squad spawns at the arrival transform in glide state. The cover stays until all of these are true: at
  least 0.5 s has passed; the terrain is built and has no pending chunks within 2.5 km of the glide's end point;
  the flight zone has fitted to the terrain; PSO precompiles are done. Hard limit 4 s.
- Then the cover fades over 1.2 s through streaks. A sonic boom plays, the low-pass opens over 0.8 s, the roar
  drops to wind. Shake falls from 1.5 to 0.4 in half a second. Residual orange on the leading edges for 2 s.
- First view: sky above, a cloud deck below with the 2,500 m hero peak through it. This is the "nothing was
  swapped" proof.

**Beat 7-9, the glide (planet).** A new state owned by flight, like the entry override.
- Speed 120 m/s held. Pitch limited to 20 to 50 degrees down; yaw at 60% authority; no boost, no fire, no
  switch; invulnerable. Pulling up past the limit does nothing (do not abort; there is nowhere to abort to).
- Terrain: on spawn, call `SetViewOverride` with the glide's end point so LOD 0 builds there first; release it
  when the glide ends. The first synchronous build centres on the arrival point instead of the pad.
- What appears when (above pad level):

| Altitude | What resolves | How |
|---|---|---|
| 3,000 m | Sky, sun, cloud tops, hero peak, far ranges as silhouettes | Aerial-perspective scale high (x3), height fog dense |
| 2,050-1,900 m | Cloud deck: wisps, 1 s whiteout in the middle, buffet (shake 0.8), droplets on the canopy, wind peak | Cover driven by depth into the deck, so it works with any cloud technique |
| 1,900 m | Whole terrain in haze: sea or lava glow, valleys, the course basin | Far ring and LOD 2 are already there; fog starts thinning |
| 1,600 m | Mountain faces, ridgeline glow, rivers | Aerial-perspective scale eases to x1.5; LOD 1 within 4.5 km |
| 1,300 m | Ground material detail under the path | LOD 0 within 1.8 km of the end point, prebuilt |
| 1,100 m | Trees, rocks, crystals | Scatter instances fade in by distance (dither between cull start and end), none popping inside 600 m |
| 900 m above ground | Pad beacon, range read-out, landing prompt hints | Glide ends |

- Time of day stays as the planet styles set it (dusk on Verdance, ember haze on Cinder
  [C: `TDPlaceholderSky.h:21-22`]). Do not animate the sun.
- The glide ends at 900 m above the ground under the ship, or at 1,400 m above pad level, whichever comes
  first; the existing terrain pull-up stays active throughout [C: `features.md:1002`].

**Beat 10, hand-back.** Over 2 s the speed override blends from 120 m/s to the atmosphere cap, with an
airbrake thump, a short FOV settle (-4 then back) and the wind loop dropping. Then normal atmosphere flight.

**Beat 11, landing.** At the pad, unchanged. Landing anywhere is a stretch: hold F over ground with slope under
about 12 degrees within the collision core, and the landing component spawns a temporary player pad with two
side pads at the ground point and runs the normal sequence. It reuses the whole pad system; the check is one
`FTDTerrainField::Sample`. Not inside the course basin's paths. [I]

### 5.3 Take-off, the reverse

- Trigger as today: exit open and the ship above the exit height [C: `TDAtmosphereZones.cpp:467`].
- Instead of travelling at once: a 4 s forced climb. Pitch eases to 60 degrees up, speed to 150 m/s, the ship
  goes through the cloud deck for real (same whiteout by depth), the fog thins, the cover ramps to white from
  2.8 s with a lighter version of the plasma (heating on the way up is not physical, so keep it to streaks and
  a blue-white flash).
- `OpenLevel` on the galaxy at 4 s under the cover.
- Arrival in space: `GroundToGlobe` of the ship's XY at exit gives a direction on the globe; place the squad
  200 m outside the atmosphere along it, facing away from the planet, and fade the cover out over 1 s with the
  globe behind. Falls back to the named orbit point when there is no record.
- If time runs out, ship only: the white cover instead of black and the loading screen. That is stage 1.

### 5.4 The globe must match the ground

Bake the globe's textures from `FTDTerrainField`, offline, with a console command. [I]

- For each texel of a 2048 x 1024 equirectangular image: texel to direction on the mesh, `GroundToGlobe`
  inverted to a ground XY, `Field.Sample(X, Y)`, then a colour from height, slope and the broad mask using a
  small per-planet ramp that follows the terrain material (Verdance: lake, moss and soil, rock, glowing
  ridgelines; Cinder: lava sea, basalt, lava rivers, ash and crystal [C: `features.md:992-994`]).
- Outputs: base colour, a normal map from the height gradient, an emissive map (lava, ridgeline glow, crater),
  and roughness in the ORM (sea smooth, rock rough). Same names as today's textures so the material instances
  keep working. Keep the current art per `docs/eng/ART-HISTORY.md`.
- Put the site pole on the mesh's equator, not its UV pole, so the landing site has no pinch. Beyond 93% of the
  way to the far side, blend to a cloud or haze cap to hide the collapse of the projection.
- Add an emissive beacon dot and ring at the site pole.
- Why offline: about 2 million field samples with slope is seconds of CPU even in parallel, which is too slow
  for map start, and a transient texture would need the same work on every launch. Baked PNGs are cooked like
  any texture. The bake is deterministic, so a test can re-sample 200 random texels against the field.
- Needs a 15-minute calibration: which way `SM_Planet_Sphere`'s UVs run. Bake a labelled test image first.

### 5.5 Parts list

Engineering hours to technically verified. "Flight" is the flight and travel teammate (T1), "Terrain" is T4,
"Flow" is whoever owns `Flow/` today (or the Lead), builders are sub-agents. All estimates [I].

**Stage 1 (tonight)**

| # | Part | Owner | Hours | Depends on | Acceptance check |
|---|---|---|---|---|---|
| 1.1 | Timing log: `OpenLevel` start, post-load, terrain built, first frame, cover off. One log line each | Flow | 0.5 | - | Numbers for both planets in the dev log, editor and packaged |
| 1.2 | Persistent cover: Slate overlay on the game viewport, colour and alpha set from the game instance, survives `OpenLevel`, 12 s hard limit | Flow | 2.5 | - | Cover at 100% white is on screen on the last frame of the galaxy and the first of the planet; never stuck after a failed travel |
| 1.3 | Travel option "covered": no camera fade, no `L_Transition`, used for galaxy to planet and back. `td.Descent.Legacy 1` restores today's route | Flow | 1.5 | 1.2 | Flow tests pass in both modes; menu and victory routes unchanged |
| 1.4 | Arrival record in the game instance (planet, pole angle, azimuth, heading offset, steepness; cleared after use; not saved to a checkpoint) | Flow + Flight | 1 | - | Unit test: record set at commit, consumed once on the planet |
| 1.5 | `GlobeToGround` / `GroundToGlobe` in one header, with tests (pole maps to the pad; far side maps to 7.5 km; round trip within 1 m) | Flight | 1.5 | - | Tests pass |
| 1.6 | Entry writes the record; rig and planet director use it for the spawn transform, falling back to today's descent start | Flight | 2 | 1.4, 1.5 | Enter from four sides of the globe: four different arrival points and headings; existing planet tests pass |
| 1.7 | Terrain: first detailed build centred on a given point; approach-cone carve only when the default arrival is used | Terrain | 1 | 1.4 | No LOD pop under the ship on arrival 5 km from the pad |
| 1.8 | Roar as a persistent 2D loop from the existing fallback sound set | Flight or sound builder | 1 | 1.2 | Roar is continuous across the load in a packaged build |

**Stage 2 (tomorrow midday)**

| # | Part | Owner | Hours | Depends on | Acceptance check |
|---|---|---|---|---|---|
| 2.1 | Glide state: speed, pitch limits, yaw authority, invulnerable, hand-back blend; reveal gate (terrain, zone, PSO) | Flight | 5 | stage 1 | Hands-off glide ends in level flight over the aimed point; never above the ceiling or into a slope, on both planets from 8 entry points |
| 2.2 | `AtmoDepth` interface phase in space and `Descent01` on the planet, exposed for effects, HUD and sound | Flight | 1.5 | - | Values logged monotonic through a full descent |
| 2.3 | Camera: shake curve, FOV curve, cap respected, user shake scale respected | Flight | 1.5 | 2.2 | Peaks match the table in 5.2; shake slider at 0 gives none |
| 2.4 | Cloud deck actor: Volumetric Cloud layer 1.9 to 2.05 km with a 2-hour timebox and a GPU gate; fallback is two panning translucent sheets plus the depth-driven whiteout | Terrain or a builder, with Nick's side told first (sky is ART-13) | 3 | - | From above: a deck with peaks through it. Inside: white. Below: a ceiling. Under 3 ms GPU at 1080p or the fallback ships |
| 2.5 | Fog and aerial-perspective ramp by altitude on the placeholder sky; no effect when Nick's tagged sky is present unless he opts in | Terrain | 1.5 | 2.2 | Captures at 3,000 / 1,900 / 1,300 / 900 m show haze thinning |
| 2.6 | Scatter fade-in by distance, and scatter ready around the glide end point before the reveal | Terrain | 2 | 1.7 | No instance appears within 600 m of the camera during a glide |
| 2.7 | Globe bake command and the `GroundToGlobe` texture, site beacon, surface spin off, pole toward the combat zone | Terrain | 4 | 1.5 | Side by side: globe capture and a top-down capture of the terrain show the same sea, peak and basin; sample test passes |
| 2.8 | Map preload on entering the gravity well; reference held; dropped on leaving | Flow | 1.5 | 1.1 | White hold shorter by the logged amount; no hitch on entering the well |
| 2.9 | Take-off: forced climb, covered travel, return point from `GroundToGlobe` | Flight | 2.5 | 2.1 | Leave from two different places, arrive above two different parts of the globe |
| 2.10 | HUD: altimeter tape (m above ground), hull-temperature bar, "ATMOSPHERE" and "ORBIT" cues, range on the pad marker | HUD teammate (ENG-36) | 3 | 2.2 | All four readable at speed; hidden outside the descent |
| 2.11 | Tests (section 6.3) | each owner | in the rows above | | |

**Effects (builders, about 8 h)**

| Effect | Type | Driven by | Acceptance |
|---|---|---|---|
| Plasma sheath | Niagara, sprites spawned on the hull mesh surface, additive, colour curve orange to magenta-white, 8-14 Hz flicker | `Entry01` | Wraps each of the three ships; tinted per ship at peak |
| Shock cone | Niagara mesh renderer, a cone on the nose with a panning erosion texture | `Entry01` | Reads at chase-camera distance, does not hide the ship |
| Plasma streaks and wisps | Niagara ribbons from wingtips and nose, plus camera-attached sprite streaks | `AtmoDepth`, `Entry01`, glide speed | Visible sense of speed at 30 m/s real speed |
| Heat haze | Post-process material, scene-colour offset by a panning normal, masked to screen edges | `Entry01` | Neutral at 0; no shimmer with TAA |
| Radial speed blur and edge glow | Post-process material | `Entry01`, glide | Off at 0 |
| Bloom and exposure push | Post-process settings on the ship camera | `Entry01` | Ends at the white cover with no visible step |
| Cloud wisps and canopy droplets | Camera-attached sprites; a screen-space droplet normal | depth into the deck | Seen for about 2 s, then clears in 1 s |
| Hull heat glow | Emissive mask parameter on the ship materials, leading edges | hull temperature | Fades over 2 s after burn-through |
| Airbrake puff | Niagara burst | hand-back | One puff, with the thump |

All go through the existing `AtmosphericEntry` cue and new cue slots in the travel presentation asset
[C: `Core/TDPresentation.h:466`], as fallbacks in engineering folders; Nick's assets at the Presentation names
win [C: `AGENTS.md` section 11].

**Sound (sound builder, about 3 h; `ue-game-audio` skill)**

| Sound | Notes |
|---|---|
| Entry roar loop | Layered: low rumble, mid tearing, high hiss. One 0..1 intensity input. Persistent across the load |
| Mix muffle | Low-pass on the main submix except the roar, in over 1 s, out over 0.8 s at burn-through |
| Sonic boom | One-shot at burn-through |
| High-altitude wind loop | Pitch and level from glide speed |
| Cloud buffet | Short loop inside the deck |
| Airbrake thump and servo | At hand-back |
| Hull tick and creak | Sparse one-shots while cooling |
| Climb-out whoosh | Take-off |

Acceptance: levels measured with the skill's report script; nothing "presentation approved" until Nicholas has
heard it in the running game [C: `AGENTS.md` section 11].

**Lighting and atmosphere (presentation side, Nicholas's call)**
- Whether the planet sky carries a Volumetric Cloud component, its colour, and the fog and aerial-perspective
  curve. Engineering ships fallbacks on the placeholder sky only. Post on `MESSAGEBOARD.md` before starting 2.4
  and 2.5.

**Assets to request from the AI generator (Grok Bot), in priority order**

| # | Asset | Spec | Used by |
|---|---|---|---|
| 1 | Plasma sheath flipbook | 8 x 8 frames, 2048, additive, white on black, turbulent flame sheet, loops | sheath, streaks |
| 2 | Erosion / flow noise set | 3 tiling 1024 greyscale textures: streaky, cellular, wispy | shock cone, haze, wisps |
| 3 | Cloud wisp flipbook | 8 x 8, 2048, soft white wisps with alpha, loops | cloud punch, interface wisps |
| 4 | Cloud deck top texture | Tiling 2048 height and alpha, cumulus field seen from above | fallback deck, globe cloud layer |
| 5 | Canopy droplet normal map | Tiling 1024, streaked droplets | canopy droplets |
| 6 | Ship heat-glow masks | One greyscale mask per ship in its UV layout: leading edges, nose, intakes | hull glow. Needs the final ship UVs |
| 7 | Globe detail overlay | Tiling 2048 colour and normal noise per planet, to break up the baked globe close up | globe material |
| 8 | Airbrake or heat-shield panels | Small animated panel set per ship, only once the final ship meshes are in | stretch |

Log each in `CREDITS.md` [C: `AGENTS.md` section 5.1]. The globe colour maps themselves are baked from our field,
not generated, because they must match the ground.

---

## 6. Performance, failure cases and testing

### 6.1 Budgets

- The project target is 60 fps at 1080p on the mid-spec machine with no hitch over 50 ms
  [C: `features.md:220, 887`]. The terrain rule is no generation hitch above 4 ms a frame [C: `features.md:1003`].
- Proposed budget for the descent [I]: 60 fps through beats 3, 4 and 7 to 10; effects under 2.5 ms GPU at peak
  (the screen is mostly white then, so overdraw is the cost to watch); cloud deck under 3 ms GPU or use the
  fallback; glide speed of 120 m/s against a terrain apply budget of 1.5 ms a frame and at most one LOD 0 upload
  a frame [C: `TDPlanetTerrain.h:51, 150`]. That last pairing is the one to measure: LOD 0 reaches 1.8 km, the
  glide covers about 100 m/s over the ground, so the view override on the end point must do the work.
- The one allowed hitch is the load itself, and only under the full white cover.

### 6.2 The load hitch

What is inside it [C for the facts, I for the judgement]:
1. Tearing down the galaxy world and garbage collection.
2. Loading the planet map package. The maps hold little, because the content is spawned by code.
3. Game mode start: director, rig, course, sky.
4. The terrain: it waits for the course, then generates 256 chunks in parallel and applies them all with
   synchronous collision cooking in one frame [`TDPlanetTerrain.cpp:106-121`]. I expect this to be the largest
   single piece. It is not measured.
5. First-frame shader and PSO work for the terrain and course materials.

How to shrink it, cheapest first [I]:
- Measure (1.1). Do nothing else until the numbers are in.
- Preload the map package in orbit (2.8).
- Build only the chunks within about 3 km of the glide path with collision in the first pass; give the rest
  LOD 2 without collision and let the existing async refresh cook them. Collision 6 km away is not needed in the
  first second.
- Stretch: generate the chunk arrays for the arrival area on worker threads in the galaxy world during orbit
  and keep them in the game instance as plain arrays, so the planet only applies them.
- Hold the cover until PSO precompiles finish, so the cost lands under the white.

What if it is slow anyway: the white holds with the roar, up to 6 s. A flat white frame does not look frozen.
The only tell is the sound, which is why the roar must persist. [I]

### 6.3 Failure cases

| Case | Handling |
|---|---|
| Travel refused, fails or times out (30 s) | Existing hand-back [C: `TDAtmosphereZones.cpp:245-256`]; cover fades on the same signal; arrival record cleared |
| Death or "CUT!" during the commit | Existing abort [C: `TDAtmosphereZones.cpp:219-232`]; cover and roar must stop in `RestoreFlight` |
| "CUT!" retake during the glide or after | The scene restarts at the default descent start, not the recorded arrival, so a bad arrival can never loop |
| Continue from a save on the planet | No arrival record: default descent start or the on-foot checkpoint, as today [C: `TDPlanetDirector.cpp:170-176`] |
| Arrival point lands over a peak | Arrival height is fixed near the ceiling, which is above all ground [C: `TDAtmosphereZones.h:186-190`]; the glide end rule uses height above ground; the pull-up stays on |
| Arrival outside the collision core | Clamped to 7.5 km by the map |
| Entry at the exact far side of the globe | Azimuth is arbitrary there; any value is valid |
| Nick's tagged sky or environment present | It replaces the placeholder sky and the terrain [C: `TDPlaceholderSky.h:30-33`, `TDPlanetTerrain.h:16-17`]. The cover, glide and effects do not depend on either; fog ramps and the cloud deck switch off unless he opts in; with his environment the arrival falls back to the default descent start unless a height query exists |
| Cover stuck | 12 s hard limit, and a console command to clear it |
| Wingmen | Spawn in formation at the arrival transform as today [C: `TDPlanetDirector.cpp:166`]; they follow the glide with the catch-up allowance |
| Legacy switch | `td.Descent.Legacy 1` gives exactly today's behaviour, for the packaged build if anything is wrong on the day |

### 6.4 What tests can check, and what only a person can judge

Automation (headless, with `OpenLevelOverride`):
- Map round trip and the fixed points of the projection.
- The record is written at commit, consumed once, never saved.
- Covered travel calls `OpenLevel` once, on the planet map, with no transition map; the legacy switch restores
  the old sequence.
- Arrival transforms for a grid of entry points: inside the core, under the ceiling, above the ground, heading
  as predicted.
- A simulated hands-off glide from each: ends in level flight, never below ground clearance, hands control
  back, takes 12 to 25 s.
- `Entry01`, `AtmoDepth` and `Descent01` are monotonic and reach their ends.
- Cover alpha is 1 at travel and 0 within the limit after arrival; cleared on every failure path.
- Globe bake: sampled texels agree with the field.
- Every existing travel, flow and planet test still passes.

Agent playtest through the engine's MCP server: captures at each beat altitude on both planets, the timing log,
frame times through the descent, in the packaged build.

Only a person can judge [I]:
- Whether the white hold reads as plasma or as a freeze.
- Whether the roar is continuous across the load on real speakers.
- Whether the globe and the ground feel like the same place.
- Whether the forced glide feels like flying or like a cutscene. If it feels like a cutscene, raise the yaw
  authority and widen the pitch range before anything else.
- Whether 20 seconds is too long on the second planet. A shorter glide for Cinder (start at 2,400 m) is one number.
- Shake and flash comfort.

State the stage honestly in the ledger: after stage 1 and 2 land with tests and captures this is "technically
verified", not "gameplay validated", until Ryan has flown it [C: `AAA.md` stages via `AGENTS.md` section 11].

---

## 7. Open questions for the Lead

1. Who owns `Flow/` changes tonight (1.2 to 1.4)? They touch the game instance, a shared class.
2. May the surface spin be turned off and the site pole pointed at the combat zone? It changes the globe's look
   slightly and is needed for a stable map.
3. The cloud deck and fog ramp sit on Nick's lighting ground (ART-13). Post before starting.
4. Rmap: 7.5 km shows the whole world on the globe but allows a 2-minute flight to the pad from the far side.
   4.5 km halves that and leaves the hero peak off the globe. I recommend 7.5 km with the site beacon.
5. `features.md` section 7 steps 3, 4 and 10 and ENG-20 need updating if this is adopted ("arrival and take-off
   stay as they are" in ENG-34 no longer holds [C: `features.md:996`]).
