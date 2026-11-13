# Intro beat sheet (ENG-33): for scoring the opening cinematic

Owner: Teammate 3. Picture: `Builds/intro/TriadDrift_Intro_0de516a.mp4` (1920x1080, 30 fps constant, H.264, silent,
one continuous take recorded from the running game at real-time pacing) and `TriadDrift_Intro_0de516a_720p.mp4`
(the same at 720p, ~3 Mbit/s). Frame 0 of the file is frame 0 of the intro: the first frame of black.

**Total: 49.00 s (1470 frames at 30 fps).** 48.00 s of shots, then a 1.00 s hand-off into gameplay.

Times are mm:ss.ff, where ff is frames at 30 fps. "Hit" means a picture event worth a musical accent.

| # | Shot | Start | Length | On screen | Mood / hit points |
|---|---|---|---|---|---|
| 1 | Nebula | 00:00.00 | 6.0 s | Fade up from black over 2.5 s into a deep nebula, a slow push. Subtitle: "Scene one. Deep space. A quiet frontier, waiting for its story." Last 0.5 s dips toward black. | Quiet, held breath, scale. The picture is fully up at 00:02.15. |
| 2 | Eclipse | 00:06.00 | 6.0 s | Verdance in silhouette against the sun, a slow lateral dolly. The sun creeps to the limb and **breaks over it in the last second**. Subtitle: "Verdance. Cinder. Two worlds still keeping their secrets." | Mystery that builds. **Hit at ~00:11.00 (eclipse reveal / sunburst)**. |
| 3 | Squad | 00:12.00 | 8.0 s | The squad passes close in a whip-pan follow, low angle. Pilot name cards: WREN 00:14.06-00:15.27, KATO 00:15.27-00:17.09, MOSS 00:17.09-00:19.12. Subtitle: "Our leads: a wing of three. Wren. Kato. Moss." | Lift and energy, the heroes' theme. **A whoosh per pass** (on each name card in). |
| 4 | Belt | 00:20.00 | 6.0 s | Threading the Verdance lane past drifting cargo pods (foreground parallax), the squad small ahead, ringed Verdance filling the left. Subtitle: "One lead. Any cockpit. Every take counts." | Momentum and confidence. |
| 5 | Gate | 00:26.00 | 6.0 s | The squad races down the gate approach along the beacon buoys and **through the portal ring**, which flares as the lead crosses. Tracking beside them, the ring framing Cinder. Ends on a quick dip to black. | Rising, the push into action. **Hit at ~00:31.08 (gate flare)**. |
| 6 | Threat, far | 00:32.00 | 5.5 s | Fade up from black (0.8 s): the Dominion's silhouette slides across the stars on a long lens. Red warp flashes as the fleet arrives (from ~00:32.27). Subtitle: "And out of the dark... the Null Armada." | Turn to menace: low, ominous. **Hit at 00:32.27 (the fleet warps in)**. |
| 7 | Threat, close | 00:37.15 | 4.5 s | **Hard cut** under the bow, the hull filling the sky. Fighters warp in and scream past the lens, one every 0.45 s from 00:37.20 (six flashes to ~00:39.27). Quick dip to black in the last 0.15 s. | Aggression, the loudest moment. **Hit at the cut, 00:37.15**; staccato accents on the warp-ins. |
| 8 | Title | 00:42.00 | 6.0 s | Black. **TRIAD DRIFT** title fades in (00:42.09-00:42.27), holds, fades out by 00:44.15. From 00:44.18 the picture fades up (1 s) behind the player's ship; hold on the ship. | Title stinger: **hit at 00:42.09**. Then a breath and resolve as the ship appears. |
| 9 | Hand-off | 00:48.00 | 1.0 s | The letterbox bars ease off and the camera settles into the chase camera. The slate "SCENE 1 · WAVE 1 · TAKE 1" comes up with the slate clap. Gameplay starts; wave 1's enemies arrive 3 s later (~00:52). | Out cleanly: ring out or button by **00:49.00**. |

## Delivering music that plays in game

- **One file for the whole intro**, starting at frame 0, **49.0 s long**. A natural ring-out that ends inside the last
  second is ideal. A tail up to ~2 s past 00:49.00 is fine, but it plays under the slate clap and the first seconds
  of wave 1.
- Assign it to **`DA_UI_Style.IntroMusic`**: Nick's `/Game/TriadDrift/Presentation/DA_UI_Style`, or until then the
  engineering copy. It replaces the engineering fallback with no code change. The asset carries its own sound class
  and volume, and it plays 2D.
- **Length is fixed:** the shots run on the game clock and don't depend on frame rate (8 shots, 48.0 s, then the 1.0 s
  hand-off). The capture above is the real game pacing: its cuts land on the frame at 6 s intervals. Under a severe
  frame-time hitch the picture can lag the music by a frame or two, nothing more.
- **Skip:** the player can hold any button to skip after the first second. The music then **fades out over 1.0 s**
  (eng/t3, after 0de516a) and the game hands off to wave 1. Watched to the end, the music is not stopped; it plays to
  the end of the file, which lines up with the hand-off.
- **Intro off or Continue:** a Continue never plays the intro (nor its music). New Game always does.
- **Re-timing risk today:** none planned. The art inside shots may still improve: the portal ring's membrane (shot 5,
  the white disc is the old placeholder), the generated hero ships (shot 3), the Dominion (shots 6-7). Timings, cuts
  and text stay. The one thing that would replace this cut is Nick authoring his own Level Sequence
  (`DA_UI_Style.IntroSequence`), which is not planned.
