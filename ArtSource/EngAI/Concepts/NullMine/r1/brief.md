# Null plasma mine r1 — brief

Reference-only concept for the Null Maul bomber's plasma mine. Dark spiked core, red arming light, readable at 300 m.

## Visual

- Gloss-black faceted core `#0E0E12`, crimson seams `#C3122E`, sharp Null spikes.
- One dominant white-hot arming eye `#F4F7FF` with red `#FF1E3C` bloom — the long-range cue.
- Visual diameter **~2.8 m** from `CollisionRadius` 140 cm. Not a planet-scale object.

## Combat spec (`TDWeaponLibrary.cpp`)

| Field | Value |
|---|---|
| Damage | 60 |
| SplashRadius | 800 cm (8.0 m) |
| ProximityFuseRadius | 500 cm (5.0 m) |
| FuseArmDelay | 0.75 s |
| Speed / Lifetime | 300 / 20 s |
| CollisionRadius | 140 cm |
| PlaceholderScale | 1.6, 1.6, 1.6 |
| Drop pattern | trail of 4, every 5 s |

## Scale references

- Mine body 2.8 m.
- Null Maul bomber 11.80 m (1180.0 × 1034.6 × 215.6 cm).
- Rift (player ship) **5.00 m** — the art-direction “~7 m” is stale.
- 180 cm figure omitted (space munition; unreadable at 300 m).

## 300 m test

The cinematic keyframe is mood. The board also has a measured Python panel at 10 m, 50 m and 300 m (90° HFOV). At 300 m the body is a few tens of pixels on a 4K 16:9 frame; the readable cue is the red arming light, not the silhouette.

See `SOURCE.md` for prompts, request ids and spend ($0.90).
