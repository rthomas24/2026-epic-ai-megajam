# Cinder field anchor tower — mesh_r1 brief

GitHub issue #52. Distant Cinder landmark and charge-planting objective (R-042 / R-043).
The player plants three charges at the base ports, then runs clear. This is **not a walkable interior**.

## Parts

- `SM_CinderAnchorTower` — 35.78 m shaft and octagonal plinth, origin at the ground-base centre, +Z up, +X forward (a charge port faces +X).
- `SM_CinderAnchorEmitter` — 5.97 m top emitter, seated in `SOCKET_TopEmitter` with 1 cm radial clearance. Assembled height **40.00 m**.

## Sockets (use these, not the current C++ 520 cm ring)

- `SOCKET_TopEmitter` at (0, 0, 3402.8 cm) — floor of the top well.
- `SOCKET_Charge_0..2` on the plinth faces, radius **587–601 cm**, z ≈ 259–275 cm, on the −X side, headings close to the C++ angles (`π + (i-1)*0.9`).
- C++ currently plants charges at radius **520 cm**, z = 60 cm (`TDPlanetCourse.cpp`). The plinth radius is ≈ **752 cm**, so that ring is buried in the concrete. Keep the sockets; the builders should spawn the interactables on `SOCKET_Charge_0..2`.

## Collision

Three `UCX_` boxes on the tower (plinth, shaft, top collar) and two on the emitter (base plug, crown). Simple, slightly conservative.

## Materials

`T_<Asset>_{BC,N,ORM,E}.png` at 2K, for `M_TD_PropTextured`. N is DirectX (green flipped). ORM is R=baked AO, G=roughness, B=metallic. E is thresholded (ports / plasma).

## Source

Raw Hyper3D Rodin Gen-2.5 packs in `source/` (untouched). Editable `.blend` next to the FBX: `SRC_*` hidden, `WeightedNormal` unapplied, `UCX_` + `SOCKET_` in the file. See `SOURCE.md`.
