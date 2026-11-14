# Fab surface sets: ForestNordicMoss, ConcreteRubble, StoneTileFloor

Texture sets only, with one material instance each on `M_TD_PropTextured`; not assigned anywhere (the environment builder
places them). Unreal: `/Game/TriadDrift/Gameplay/Props/Fab/Textures/<Set>/` (`T_Fab_<Set>_BC`, `_N` DirectX, `_ORM`
occlusion / roughness / metallic, `MI_Fab_<Set>`), 4096, max texture size 4096. Prepared by `../scripts/prep_fab_textures.py`
(Blender 5.2, report `prep_textures_report.json`), imported by `../scripts/ue_import_fab_multi.py`. Tiled previews:
`<Set>/review/T_Fab_<Set>_BC_tiled3x3.png`. All supplied by Nicholas Tosti in the private release
`asset-sources-nicholas-20261114`; originals kept in the git-ignored `Builds/intake3/`. Evaluations: `../intake3/<Pack>/SOURCE.md`.

| Set | Source | Licence | AI | Changes |
|---|---|---|---|---|
| ForestNordicMoss | "Forest Nordic Rock Mossy Rough 05" by **Quixel Megascans** (surface xiembdz), https://www.fab.com/listings/94a346ac-968d-4140-95e6-afa22c644e63 (#189), archive SHA-256 `82118353d9f4ef38bdfee2fa771e8889afa40a03670e1f3d4291e3100455f1e1` | **Fab Standard License** (https://www.fab.com/eula); tier acquired not recorded | not AI-generated | moss shifted from green toward Verdance teal (seller colour kept as `export/T_Fab_ForestNordicMoss_BC_orig.jpg`); normal flipped OpenGL to DirectX; ORM packed from AO and roughness, metallic 0 |
| ConcreteRubble | "Concrete (Rubble) Texture & Material Vol. 08" by **ehsan vaezi**, https://www.fab.com/listings/d6f41d19-01c4-4185-84fd-bc1a6d160154 (#190), archive SHA-256 `9578070716525c7f397bfa103a1dba623f6d3a1a6f2e9d7967affb32955ad6a8` | **CC BY 4.0** (https://creativecommons.org/licenses/by/4.0/): attribution required | not AI-generated | darkened and desaturated toward Cinder ash (seller colour kept as `export/T_Fab_ConcreteRubble_BC_orig.jpg`); normal flipped OpenGL to DirectX; ORM packed; TGA maps read out of the Unity package as data |
| StoneTileFloor | "Stone Tile Floor 1" by **Yi-X**, https://www.fab.com/listings/8bf741b0-c44f-4269-9a94-a7b5821cded2 (#186), archive SHA-256 `b109026e95e299d7f9562d58467b9e5b0009b93201b4450da5f75ed7b529fb52` | **Fab Standard License**; tier acquired not recorded | not AI-generated (NoAI label) | colours as supplied; ORMH split to ORM (height dropped); normal already DirectX. Use: Verdance relay plaza only |
