# Enemy ships at chase distance

At **136 m**, the orange ship silhouette separates from the cyan nebula. The selected ship lies under several HUD elements, but two neighboring outlines are recognizable even away from their red markers. At **328 m**, the lock diamond, range and brackets remain easy to find. The small orange contact at the diamond’s right edge is difficult to resolve against the stars. I could not identify two distinct features resembling a white eye and a red strip on the selected ship.

| Lancer at 9.50 s, 136 m | Bastion at 12.75 s, 328 m |
|---|---|
| ![Front of a ship formation against a cyan nebula](target-136m.png) | ![Small target in the bracket against a dark purple sky](target-328m.png) |

Both images are 760 × 440 crops of actual 1920 × 1080 gameplay frames, at original pixel scale and with the captured exposure. The enemy pass and camera angle differ. T3 recorded this footage before the Rodin mesh pickup, so the images establish a baseline for the new fleet. [Original dogfight clip in T3’s release](https://github.com/rthomas24/2026-epic-ai-megajam/releases/tag/trailer-v2-footage)

For the integration-build comparison, capture the same enemy at 300 m, 600 m and 1 km in the chase camera, first against dark sky and then against a bright nebula. Pair each normal frame with a HUD-hidden frame at the same pose and exposure. Look for a forward white eye and a separate red/orange hull strip that still show the direction of travel. Preserve the dark hull and check bloom near the player before increasing brightness across the whole material. Nicholas can choose the desired brightness from that comparison. No present-build or 1 km appearance judgment follows from these two frames.
