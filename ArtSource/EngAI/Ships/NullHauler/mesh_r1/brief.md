# SM_NullHauler — mesh_r1 brief

40 m Null Armada cargo / sensor hauler for `ATDArmadaConvoy` (Verdance / Cinder approach
lanes). Issue #51, R-030. Image-to-3D from Nicholas's four-view board, packaged under
`ArtSource/EngAI/Ships/NullHauler/mesh_r1/`.

- Length locked to **4000.0 cm** along +X (uniform scale; natural beam 19.40 m, height 8.76 m).
- Pivot at the bounding-box centre. +Z up, +X nose, −X quad thrusters.
- 26,497 tris (under the 30k budget). 2K PBR: BC, DirectX N, ORM, authored E.
- No guns, no turrets, no scanner ring on the mesh — the convoy's orange ring stays a C++
  placeholder (`H.Ring`).
- Sockets: `SOCKET_Thruster_0..3`, `SOCKET_SensorDome_0` (fore), `SOCKET_SensorDome_1` (aft).
- See `SOURCE.md` for licence, provenance, sockets and import notes.
