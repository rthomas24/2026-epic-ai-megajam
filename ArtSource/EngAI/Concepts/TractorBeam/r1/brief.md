# Tractor-beam cone r1 — brief

Reference-only concept for the Dominion docking tractor (ENG-29 / ENG-37). Seen from behind the ship, looking toward the bay. A wide volumetric cyan/white cone, not a dotted laser.

## Constants (`TDDockingBay.h`)

| Field | Value |
|---|---|
| ConeHalfAngle | 35° |
| ConeDepth | 300 000 cm (3.00 km) |
| MouthDistance | 30 000 cm (300 m) |
| MouthRadius | 6 000 cm (60 m) lock |
| Radius at 300 m (cone) | 300 · tan 35° = 210 m |
| Radius at 3 km | 3 000 · tan 35° = 2 101 m |
| Emitter | `SM_DominionBay_TractorEmitter`, 900 × 186 cm, cyan GlowColor, hull (31200, 0, 1400) |
| Cue | attached at the bay mouth, +X out toward the ship |
| User.Length | bay-to-ship distance (cm) |
| User.Intensity | 0..1 |
| Rift length | 5.00 m (approved mesh) |
| Formula | r(D) = D · tan(35°) ≈ D · 0.700 |

## Frames on the board

1. Key: camera aft of the Rift, engines toward us, cone wrapping the hull and tightening onto the 9 m emitter.
2. Establishing volume (mood — the ship may sit outside the cone in this painting).
3. Side silhouette (mood — half-angle in the painting is approximate).
4. Measured Python diagram: near field 0–400 m (emitter and ship to scale) and a full 3 km inset.

180 cm figure omitted (kilometre-scale cone). See `SOURCE.md` for prompts, request ids and spend ($0.90).
