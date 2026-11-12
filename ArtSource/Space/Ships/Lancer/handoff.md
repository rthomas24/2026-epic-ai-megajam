# Lancer integration handoff

This is the proposed visual interface, not a change to Ryan's gameplay API.
The Lead relays the attachment contract to the VFX Agent. No weapon effects are
authored here. Do not reference these filesystem sources from shipped content.

## Assets and assembly

| Source asset | Purpose | Pivot |
|---|---|---|
| SM_Ship_Lancer_Proxy | Needle hull, forward-swept wings, canopy, twin drives, dorsal overdrive inserts | Assembly origin |
| SM_Lancer_UniversalMount | Common mounting foot and cradle, instanced four times | Mechanical interface |
| SM_Lancer_MinigunBody | Breech/receiver and motor, instanced twice | Common mount |
| SM_Lancer_MinigunRotor | Six hollow barrels and collars, instanced twice | Axial spin pivot |
| SM_Lancer_RocketPod | Four actual open tube fronts and recessed loaded noses, instanced twice | Common mount |
| SM_Lancer_SpikeRocket | Independent projectile visual | Body centre |

All JSON coordinates below are cm in Unreal axes (+X forward, +Y right, +Z up).
Left means -Y when looking in the ship's forward direction. Mirrored placements
use positive unit scale and identical forward-facing weapon geometry.

| Attachment | Parent | Position | Direction |
|---|---|---|---|
| Mount_Primary_L / R | Hull | (30, -100 / +100, -42) | +X |
| Mount_Secondary_L / R | Hull | (-90, -220 / +220, -44) | +X |
| Muzzle_Primary_L / R | Primary_L / R | (174, 0, -15) | +X |
| Rotor_Pivot | Primary_L / R | (35, 0, -15) | +X spin axis |
| Muzzle_Secondary_L/R_01..04 | Secondary_L / R | (112, ±16, -9 or -39) | +X |
| Thruster_L / R | Hull | (-402, -111 / +111, -3) | -X outward |
| Ability_Overdrive | Hull | (-205, 0, 61) | +X |
| Ability_EMP | Hull | (0, 0, 0) | +X |
| Ram_Visual | Hull | (404, 0, 0) | +X |

Rocket tube order is y=-16,z=-9; y=-16,z=-39; y=+16,z=-9; y=+16,z=-39.
Each rotor also has Barrel_01..06 at local x=138, radius 12 cm. These are optional
individual muzzle points; keep a grouped primary muzzle for cheap jam effects.
Attachment names are unique per parent, not globally. Full transforms are in
`attachments.json`. Engine VFX must align their emission axis to the supplied
outward basis; do not assume every effect points along the ship's +X.

## Ryan's implementation needs

- Confirm whether `UTDShipVisuals` can describe an array of attached modular mesh
  components and socket names, or request a single assembled export fallback.
  The current project document names presentation hooks, not this exact schema.
- Bind weapon and boost presentation through the existing `UTDWeaponFX` /
  `UTDShipVisuals` contracts. This package does not create their instances.
- Instantiate/swap visual modules at the common mounts. Keep the rotor optional;
  its spin is cosmetic and has no effect on projectile trajectories or fire rate.
- Map weapon events to the grouped muzzle or individual tube origin. Damage,
  firing, heat, ammo, cooldowns, ram collision and ability state remain Ryan's work.
- Expose boost/overdrive state to presentation. Material controls proposed here:
  BaseColor, Metallic, Roughness, EmissiveColor, EmissiveStrength. Defaults are in
  the manifest. A visual artist must create/verify Unreal instances after approval.

## Remaining production gates

1. Human silhouette/design approval. Repository skill routes final hero hulls
   through concept/image-to-3D or licensed kitbash; this code hull stays a proxy.
2. Lead publishes binary claims and obtains production authorization. No merge
   into main is authorized by this source package.
3. Validate the Blender adapter on supported Blender 5.2. Only Blender 5.1 was
   installed during preflight; it was not launched. Unwrap/material/assembly QA,
   bevel/shading review and visual contact sheets are still required.
4. Finalize collision and weapon envelopes with Ryan. Ram_Visual is an effect
   locator, not a collision shape. Validate LODs, material slots and draw calls.
5. Calibrate FBX with the repository pipeline; export each reusable module with
   a single render mesh. Verify centimetres, forward axis, pivots and sockets in
   an Unreal 5.9 round trip. Interchange socket support must be checked explicitly.

Root credits/devlog are owned by Lead. Merge the local provenance entry there at
integration. The Lead owns the existing completion sound for this batch; this
chat does not emit a second sound.
