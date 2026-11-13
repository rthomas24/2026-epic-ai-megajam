# Pilot (on-foot player body): source and pipeline

## Source

- File (read-only, owned by Nick's side): `ArtSource/Characters/Samples/SentinelArmor/Meshy_AI_Sentinel_Armor_Turnaround_20261113_animated_ue5_mannequin_All_animation.glb`
- SHA-256: `1adba4e4583dd0e8451814b720c17f6c05a6790634282c7a817f12a15775ae1b` (25,583,856 bytes; matches `sample_manifest.json` beside it)
- Supplied by Nicholas (message board N-021) as a **Meshy AI generated, auto-rigged sample**.
- **Licence terms and the Meshy tool plan must be confirmed by Nicholas before submission.** Until then this asset is
  not cleared to ship. When cleared it needs a `CREDITS.md` entry (Meshy, AI-generated mesh, textures, rig and animations).
- The sample's own README calls it a review-only sample; using it as the pilot is an engineering import for evaluation.
- Design note for whoever signs off: white and grey powered armour with a cyan visor. It is a generic design, but the
  silhouette is close to well-known sci-fi armour; Nicholas should judge it against the "original designs only" rule.

## What is in the source

- 1 skinned mesh, 33,938 triangles, 44,952 vertices, 1 material, three 2048 x 2048 textures.
- 74 bones, UE4-mannequin names, pelvis is the top bone (no `root`, no twist bones, no `spine_04/05`). Extra bones:
  `Bone_026` to `Bone_033` (two eye chains on the head, weighted), `headfront`, and 13 `*_end` leaf bones (not weighted).
- 170.0 cm tall, feet at 0, faces Blender -Y. Maximum 4 bone influences per vertex, no unweighted vertices.
- Six animations, keyed at 30 fps, all **in place** (the pelvis does not travel), so no in-place versions were needed.

| Asset | Source name | Length | Loops (first vs last pose) | Notes |
|---|---|---|---|---|
| `A_Pilot_Run` | Running | 0.633 s (19 frames) | yes, exact | pelvis bob 7 cm |
| `A_Pilot_Walk` | Walking | 1.033 s (31 frames) | yes, exact | pelvis bob 7 cm, 6 cm side sway |
| `A_Pilot_CasualWalk` | Casual_Walk | 4.167 s (125 frames) | **no** (6 cm / 10 degrees apart) | needs a blend at the loop point if used |
| `A_Pilot_Idle02` | Idle_02 | 1.867 s (56 frames) | yes, exact | guard stance, right fist raised; least motion (about 1 cm) |
| `A_Pilot_Idle3` | Idle_3 | 9.967 s (299 frames) | yes (0.07 cm / 0.07 degrees) | neutral stand, hands at the sides, slow weight shift. **Chosen idle** |
| `A_Pilot_Idle6` | Idle_6 | 7.367 s (221 frames) | yes (0.05 cm / 0.10 degrees) | relaxed stand on one leg, hands move most |

## Pipeline (Blender FBX route, all headless)

Unreal's direct glTF import was not used: the Blender route let the scale, root bone, loops, root motion and normal
convention be measured and fixed before import, and FBX skeletal import was already proven in this project.

1. `scripts/p00_inspect.py`: lists what the GLB contains (no output files).
2. `scripts/p10_build.py`: imports the GLB at 30 fps, measures mesh and animations, writes the textures, converts to
   centimetres, exports `export/SK_Pilot.fbx` and one `export/A_Pilot_*.fbx` per animation, writes
   `export/Pilot.manifest.json` (all measurements) and `work/Pilot_v01.blend` (metres, textures packed).
   - The armature object is named `root`, so Unreal gets a real `root` bone at the origin above `pelvis` (75 bones).
   - Exported in centimetres (unit scale 0.01, scale baked into the data). A metre-scale export gave a root bone with
     scale 100 in Unreal.
   - `T_Pilot_BC.png` and `T_Pilot_ORM.png` are the glTF images byte for byte. ORM: G roughness, B metallic, R is not
     occlusion and is left unconnected.
   - `T_Pilot_N.png` is the glTF normal map with **green inverted** (DirectX style, what Unreal expects). The untouched
     original is kept as `export/T_Pilot_N_source_opengl.png`.
3. `scripts/p12_normal_seam_test.py`: evidence that the source normal map is OpenGL style (decoded normals agree across
   UV seams 0.82 when read as OpenGL, 0.72 as DirectX). `scripts/p11_normal_convention.py` was a first test that proved
   inconclusive and is kept only for the record.
4. `scripts/p13_idle_motion.py`: how much each idle moves (basis for the idle choice).
5. `scripts/p20_review.py`: review sheets in `review/`.
6. `scripts/ue_import_pilot.py`: Unreal import, material, physics asset, data asset and verification numbers.
   `powershell -File Tools\td-python.ps1 -Script ArtSource\EngCharacters\Pilot\scripts\ue_import_pilot.py`
7. `scripts/p30_check_roundtrip.py`: reads back the FBX Unreal exported from `SK_Pilot` (`Saved/Pilot/`) to count it.

Blender: `"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python scripts/<script>`
(`p12`, `p13` and `p20` open `work/Pilot_v01.blend` instead of `--factory-startup`).

## Unreal assets (`/Game/TriadDrift/Gameplay/Characters/Pilot/`)

`SK_Pilot`, `SKEL_Pilot`, `PA_Pilot`, `A_Pilot_Run`, `A_Pilot_Walk`, `A_Pilot_CasualWalk`, `A_Pilot_Idle02`,
`A_Pilot_Idle3`, `A_Pilot_Idle6`, `T_Pilot_BC`, `T_Pilot_N`, `T_Pilot_ORM`, `M_Pilot`, `MI_Pilot`, `DA_Pilot_Visuals`.

- In Unreal: 68.7 x 34.9 x 170.0 cm, feet at Z = 0, faces **+Y** (mannequin convention), left hand on +X.
  `MeshYaw = -90` makes it face +X.
- `MeshScale = 1.04`: the eye bones are at 157.7 cm, so 1.04 puts the eyes at 164.0 cm and the helmet top at 176.8 cm
  (inside the 180 cm capsule).
- To rebuild `M_Pilot` delete its `.uasset` first: editing a material that has instances crashes the headless editor.

## Not verified

- Never seen rendered in Unreal (headless, no renderer): material look, normal map direction and skinning under
  animation were judged from the Blender renders and from numbers only.
- Interchange warned on import: "Imported skeleton has some invalid bind poses. Skeletal mesh skinning has been rebind
  using the time zero pose". The FBX is exported in the rest pose at time zero, and bounds and the round trip match, but
  a look in the editor is still needed.
- `PA_Pilot` is the editor's automatic physics asset, untuned.
