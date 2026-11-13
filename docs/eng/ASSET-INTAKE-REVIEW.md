# Review of Nicholas's supplied models (asset management, ART-02)

Nov 13, 2026, asset builder on Ryan's side. A review only: nothing was imported, converted or copied into the game
tree. Ledger: `docs/eng/ASSETS.md`. Feature freeze: Nov 15, 12:00 EST.

## 1. What was reviewed and how
| Library | Ref | Contents |
|---|---|---|
| Intake 02 | `origin/nick/assets-meshy-intake02` @ `4587658` | 14 GLB sources, imported there as 91 Unreal packages under `Content/Pilot/{CrimsonTempest, SentinelArmor}` and `Content/Intake/*` |
| Weapons | `origin/nick/assets-weapons-intake` @ `55d0010` | 5 GLB sources, 27 packages under `Content/TriadDrift/Art/Weapons/MeshyIntake01/` |
| Intake 03 | `origin/nick/assets-meshy-intake03` @ `aa53ff9c` | 36 original sources (22 GLB, 14 ZIP), not imported. Azure Shadow is excluded by Nicholas's side and was not opened |

- **Read:** each branch's `SOURCE.md`, `README.md` / `IMPORT_PLAN.md` and `intake_manifest.json`.
- **Measured here:** all 41 GLB files (1.77 GB of LFS objects), by parsing the glTF JSON chunk: triangles from the
  index accessors, embedded image sizes, skins, animations, bounding box. The counts match the manifests wherever the
  manifests give one. Three of the 14 ZIP archives (73 MB) were opened and their FBX measured and rendered in headless
  Blender 5.2. **The other 11 ZIPs (1.2 GB) were not fetched** (LFS budget) and are marked "not measured".
- **Looked at:** a three-view thumbnail of every GLB (points sampled on the surface and coloured from the base-colour
  and emissive textures; not a lit render) and three EEVEE renders. Scratch files: `Saved/AssetReview/` (git-ignored).
  Enough to judge silhouette and palette; not enough to judge shading, small decals or mesh defects.
- **Not done:** no Unreal import, no decimation trial, no rig test. Costs below are estimates in builder-hours for
  one builder using `.claude/skills/blender-ai-mesh-cleanup` (weld, decimate, re-unwrap, re-bake) and
  `.claude/skills/blender-unreal-export` (scale, +X forward, pivot, sockets, collision, import script).

## 2. What is true of the whole library
1. **Provenance: tool by filename only; licence NOT RECORDED.** All three `SOURCE.md` files say the same: the files
   are "Meshy AI" by filename; generator version, prompts, account plan, creator identity and licence documents were
   not supplied and are unverified; no licence or ownership claim is made. Nicholas's ownership statement on the board
   covers Sentinel Armor only, and Intake 02's `SOURCE.md` says not to extend it to the batch. Before any of these
   ships: Nicholas states the Meshy plan each was made on, and `CREDITS.md` gets a line per asset.
2. **Possible pre-jam assets (inference from filenames).** 13 of the 14 ZIPs carry an eight-digit stamp that reads as
   a date before the jam began (for example `Aurora_Landing_Pad_20260725`, `Neon_Lumen_Assault_Rifle_20260329`,
   `Neon_Security_Gate_20261025`); the files inside the Aurora archive are dated Nov 10, two days before the start.
   The three excluded or held files show the same pattern (`20260422`, `20260514`, `20260909`). The GLBs carry a
   ten-digit stamp that reads as month, day and time on Nov 12 or 13. Intake 03's `SOURCE.md` says filename dates are
   not proof either way. The jam requires assets made before Nov 12 to be declared. **Decision here: every stamped
   ZIP is NOT NOW until Nicholas says when it was made.**
3. **Image-to-3D with an unknown input image.** Filenames such as `Gothic_Stone_Arch_Wal_..._image-to-3d-texture`,
   `make_it_3d_rote_to_`, `Create_a_massive_hard_`, `Un_personnage_masculi_` and `T_Pose_The_robot_` read as prompts
   or image-to-3D jobs. Where an image was the input, nobody on Ryan's side has seen it. If any input image came from
   an existing game, film or another artist, the result is not ours to ship. Ask Nicholas for the input of each pick.
4. **Not game-ready.** 33 of the 44 measured models are between 195,000 and 3,080,000 triangles. The base-colour
   atlases that were opened are thousands of tiny islands (generator UVs), so each kept model needs a re-unwrap and a
   re-bake after decimation, which the clean-up skill does. Every model is normalised to about 1.9 m on its longest
   side, whatever it is, so real size has to be set per asset. Normals are OpenGL convention (weapons `SOURCE.md`).
5. **Not in the faction palette.** No model is black armour with red-orange seams and white eyes, or white ceramic
   with magenta, cyan or amber. Each pick needs a tint or re-texture pass, which also moves it away from the raw
   generator output.
6. **Characters: only two are rigged.** Sentinel Armor (in the game) and Crimson Sentinel. The rest are static, and
   the crew already has three types on `SKEL_NullCrew` plus the Rodin drone and Warden, so characters are poor value.
7. **Weapons: the game needs one.** Enemies fire from a `Muzzle` socket and the pilot uses the blade; there is no
   weapon-mesh slot in `UTDEnemyVisuals`. One rifle for the trooper is the whole requirement (about 15,000 triangles
   or less). Twenty-one of the 55 assets are firearms.

## 3. Every asset
Fit: 1 = wrong for this game, 3 = usable with a re-texture, 5 = fits the story and look as it is shaped.
Textures are the embedded images (base colour, metal-rough, normal, emissive where present).

### 3.1 Intake 02 (14, imported on the branch)
| Asset | Kind | Tris | Textures | Rig / anims | Fit | Proposed use | Cost | Concerns | Decision |
|---|---|---|---|---|---|---|---|---|---|
| SentinelArmor | Character | 33,938 | 3 x 2K | 74 joints; 6 clips | 4: white armour with a cyan visor reads as Triad | Already the pilot (`Gameplay/Characters/Pilot`) | Done | Meshy plan not recorded; silhouette close to well-known sci-fi armour (`Pilot/SOURCE.md`); the branch's second copy under `Content/Pilot/` must not be merged | **USE** (in game) |
| CrimsonTempest | Ship | 11,842 | 4 x 2K, emissive | none | 3: angular fighter, but the hull is mostly saturated red, not black with seams | Third ship type in the lanes of machines | 1.5 h: darken the red panels to black, keep red as seams, pivot, scale, import. No decimation | None seen at thumbnail scale | **USE AFTER CLEAN-UP** |
| CrimsonSentinel | Character | 231,791 | 4K, 8K, 4K PNG | 28 joints (Mixamo route by filename); run, walk, spin jump, agree gesture, air squat | 4: dark grey plate with red seams, the closest character to the Null look | A fourth crew silhouette for the Dominion interior (an officer) | 5 h: decimate to 25k, re-bake 2K, re-skin onto `SKEL_NullCrew` by weight transfer from `SK_NullTrooper`, white eyes | Its own skeleton and clips are not usable as they are; the 8K map must not ship | **NOT NOW** (first reserve if a fourth crew type is wanted) |
| CyberneticSentinel | Character | 1,995,134 | 3 x 2K | none | 2: white and blue android; no role | None | 6 h+ | Two million triangles | **NOT NOW** |
| EmeraldShadowstar | Ship | 49,874 | 3 x 2K | none | 3: dark hull with glowing seams is the right idea, in green | Lane ship, seams shifted to red-orange | 2 h: decimate to 10k, hue-shift seams, re-bake | None seen | **NOT NOW** (alternate for Crimson Tempest) |
| GothicStoneArchWall | Prop | 9,763 | 4 x 2K PNG, emissive | none | 3: weathered stone arch with a glowing blue glyph; reads as ancient, but masonry, not machinery | The one ancient artefact that recurs on Verdance, Cinder and in the core chamber (storyline §6) | 1.5 h: scale to 4 m, pivot at the base, collision, import, glyph on a material parameter | Image-to-3D: input image unknown. Nicholas owns whether the Origin Core's builders look like this | **USE AFTER CLEAN-UP**, after Nicholas's call |
| NebulaShadowfighter | Ship | 7,741 | 3 x 2K | none | 4: dark gunmetal, conventional airframe with small lights: "human military engineering underneath" | Lane ship; the cheapest ship in the library | 1 h: cyan lights to red-orange and a white eye, pivot, scale, import | None seen | **USE AFTER CLEAN-UP** |
| NeonShadowEnforcer | Character | 19,428 | 4 x 2K, emissive | none | 3: black armour, green lights | Crew variant after a re-colour and rig | 4 h; arms-down pose rigs badly at the shoulders | - | **NOT NOW** |
| NeonVoidCruiser | Ship | 12,646 | 4 x 2K, emissive | none | 1: purple and cyan organic hull reads as alien; the story never shows aliens, and purple sits on Rift's magenta | None | - | - | **NOT NOW** |
| Ruinforged | Character | 359,815 | 4 x 2K | none | 2: white armour; no role (the pilots' faces and bodies are not shown beyond the player) | None | 5 h | - | **NOT NOW** |
| ShadowboundAssassin | Character | 824,072 | 4 x 2K | none | 1: hooded, cloaked dark figure; fantasy | None | - | **Originality:** a stock hooded-assassin figure, close to well-known game and film characters | **NOT NOW** |
| Spaceship | Ship | 697,703 | 3 x 2K | none | 4 for Triad's side: white hull, cyan ring engines | The missing patrol's wreck (storyline §11, "Patrol Seven") or the crashed research vessel at the Verdance site | 4 h: decimate to 12k and re-bake (2.5 h), cut into two or three scorched pieces (1.5 h) | Cyan is Lancer's colour: fine on a friendly wreck, not on a flying ship | **USE AFTER CLEAN-UP** |
| TPoseRobot | Character | 626,135 | 4 x 2K, emissive | none (T-pose) | 4: dark armoured figure with orange chest lights; T-pose is rig-friendly | Crew variant or a dormant figure in the interior | 5 h | Prompt-style filename; chest and hip lights need a close look for emblems | **NOT NOW** |
| MaleCharacter ("Un personnage masculin") | Character | 579,530 | 4K + 2K, emissive | none | 3: black armour with blue and red glow | Crew variant | 5 h | Prompt-style filename | **NOT NOW** |

### 3.2 Weapons (5, imported on the branch; each about 1.9 m long as supplied)
| Asset | Kind | Tris | Textures | Rig / anims | Fit | Proposed use | Cost | Concerns | Decision |
|---|---|---|---|---|---|---|---|---|---|
| NeonPulseRifle | Rifle | 195,919 | 4 x 2K, emissive | none | 2: white and cyan, Triad's palette on an enemy gun | None | 2.5 h | - | **NOT NOW** |
| RuststeelAssaultRifle | Rifle | 201,907 | 4K, 2K, 4K | none | 2: olive and rust, scoped | None | 2.5 h | - | **NOT NOW** |
| CrimsonRiftSniper | Rifle | 388,364 | 4K + 2K, emissive | none | 3: gunmetal with a red-orange core, but an ornate filigree stock | Alternate trooper rifle | 3 h | - | **NOT NOW** |
| PlasmaRifle | Rifle | 544,893 | 3 x 2K | none | 2: dark with cyan | None | 3 h | - | **NOT NOW** |
| UnlabelledRifle01 | Heavy rifle | 879,930 | 4K, 2K, 4K | none | 4: black body, glowing orange coil core, chunky enough to read at range | The heavy's cannon | 3 h: decimate to 10k, re-bake 2K, grip and muzzle | Prompt-style filename (`make_it_3d_rote_to_`): input image unknown | **USE AFTER CLEAN-UP** (low priority) |

### 3.3 Intake 03, GLB sources (22, not imported)
| Asset | Kind | Tris | Textures | Rig / anims | Fit | Proposed use | Cost | Concerns | Decision |
|---|---|---|---|---|---|---|---|---|---|
| PlasmaPistol03 | Pistol | 1,972,746 | 3 x 2K | none | 3: black, white and red glow | None: nobody carries a pistol | 3 h | Stencilled glyphs on the side need reading before any use | **NOT NOW** |
| ArmoredCombatTruck | Vehicle | 1,498,782 | 4K + 2K, emissive | none | 3: rusted six-wheel truck with a turret | Parked at the Cinder mining complex | 3 h: decimate to 12k, re-bake | A wheeled truck is a tone risk beside starfighters | **NOT NOW** (reserve dressing for Cinder) |
| AzureRailgun | Rifle | 1,879,168 | 3 x 2K | none | 2: white and blue | None | 3 h | - | **NOT NOW** |
| BluePowerArmor | Character | 1,000,008 | 4K + 2K, emissive | none | 1: bulky blue power armour | None | - | **Originality:** the proportions and pauldrons are close to well-known space-marine power armour | **NOT NOW** |
| ChinaFighter | Ship | 6,574 | 4 x 2K, emissive | none | 2: red and gold ornamental fighter | None | 1.5 h | The name refers to a real country; check for national markings | **NOT NOW** |
| **CoilgunAssaultRifle** | Rifle | **4,124** | 4K, 2K, 4K, 2K, emissive | none | 4: plain military rifle with exposed coils; grey, so it re-colours cleanly | **The trooper's rifle.** The only weapon already inside the 15k budget | 1.5 h | 1.0 m long as supplied (the others are 1.9 m) | **USE AFTER CLEAN-UP** |
| CircularBulkheadWall (`Create_a_massive_hard_`) | Set piece | 3,076,718 | 8K, 4K, 4K | none | 4: heavy blue-grey bulkhead frame with a round port: "familiar military architecture" | The Dominion's hangar or core-chamber door; also the dig door on Cinder that recognises the pilots | 3.5 h: voxel remesh and decimate to 10k, re-bake 2K, darken, collision | Three million triangles and an 8K map: run alone on a 16 GB machine. Prompt-style filename | **USE AFTER CLEAN-UP** |
| CrimsonSigilRailgun | Rifle | 500,025 | 4K + 2K, emissive | none | 3: black and red | None | 3 h | A glowing round sigil on the receiver reads as an emblem | **NOT NOW** |
| CrimsonSkullAssault | Rifle | 461,793 | 3 x 2K | none | 2: black and red with skulls | None | 3 h | Skull iconography says "villains"; the story says the future Triad is not one | **NOT NOW** |
| CrimsonSteamRevolver | Revolver | 415,012 | 4K + 2K, emissive | none | 1: steampunk | None | - | - | **NOT NOW** |
| **CrimsonVanguard** | Ship | 619,753 | 3 x 2K | none | **5**: near-black gunmetal, red accents, twin booms and cannons on a recognisably human airframe | A ship in the lanes of machines streaming to the planets; could also fly as a wave variant | 2.5 h: decimate to 10k, re-bake 2K, white eye, engine and muzzle sockets | None seen at thumbnail scale | **USE AFTER CLEAN-UP** |
| CyberneticSpecialForces | Character | 800,746 | 3 x 2K | none; posed holding a rifle | 2: modern special-forces soldier, baked pose | None | 6 h | Cannot be rigged cleanly from this pose | **NOT NOW** |
| FuturisticLandingPad | Prop | 552,544 | 3 x 2K | none | 3: square pad, amber lights | None: `SM_LandingPad` exists | 2.5 h | - | **NOT NOW** |
| FuturisticPistol | Pistol | 3,014,464 | 3 x 2K | none | 1 | None | - | Blocky sidearm of a familiar sci-fi shooter type | **NOT NOW** |
| HyperLumenRifleX | Rifle | 343,044 | 3 x 2K | none | 1: present-day carbine pattern with green cells | None | - | - | **NOT NOW** |
| IronWardenPistol | Pistol | 375,285 | 4 x 2K, emissive | none | 1 | None | - | - | **NOT NOW** |
| NeonFluxBlaster | Rifle | 291,444 | 3 x 2K | none | 2: sleek dark hull, cyan core; reads as alien | None | 2.5 h | - | **NOT NOW** |
| NeonSentinel | Character | 1,107,688 | 3 x 2K | none (near T-pose) | 3: dark armour, cyan lights | Crew variant | 5 h | Helmet and plate layout are close to familiar sci-fi troopers | **NOT NOW** |
| SciFiWeaponPack | Pistol (one model, despite the name) | 772,753 | 3 x 2K | none | 2 | None | - | - | **NOT NOW** |
| VanguardVX07RailS | Sniper rifle | 1,193,568 | 3 x 2K | none | 2: white and black with a bipod | None | 3 h | - | **NOT NOW** |
| WastelandPulseRifle | Rifle | 889,676 | 8K, 4K, 4K | none | 1: scrap-built | None | - | - | **NOT NOW** |
| UnnamedCyanRifle (`_1112153907`) | Rifle | 845,260 | 3 x 2K | none | 2: green and tan with cyan | None | 3 h | - | **NOT NOW** |

### 3.4 Intake 03, ZIP sources (14, not imported)
| Asset | Kind | Tris | Textures | Rig / anims | Fit | Proposed use | Cost | Concerns | Decision |
|---|---|---|---|---|---|---|---|---|---|
| NeonSecurityGate | Door frame | 364,878 (measured) | BC, N, M, R PNG | none | 4: grey military door frame, blue lights, hazard chevrons | A Dominion or dig-site doorway | 2.5 h | Filename date `20261025` (pre-jam, inference); small yellow glyph decals to read; `SM_DI_Door`, `SM_DI_Doorway` and `SM_Door` already exist | **NOT NOW** (first pick from the ZIPs if Nicholas clears the date) |
| AuroraLandingPad | Prop | 1,592 (measured) | 1 PNG (base colour only) | none | 3: square pad, cyan lights | None: `SM_LandingPad` exists | 1 h | Filename date `20260725`; files inside dated Nov 10, before the jam | **NOT NOW** |
| VerdantStarblade | Ship | 9,940 (measured) | BC, N, E, M, R PNG | none | 1: green hull with gold filigree, ornamental | None | - | Filename date `20260427` | **NOT NOW** |
| TitanSentinel | Character | not measured (264 MB) | not measured | Two FBX per the manifest: a Mixamo-animated file (60 fps) and a separate rest-rig mesh | Not judged | A Warden alternative; Grok Bot's Warden is already delivered | not estimated | Stamp `20261113` (inside the jam) | **NOT NOW** |
| ArmoredTechCase | Prop | not measured | not measured | - | Not judged | A case for the signal-analysis device | - | Filename date `20260707` | **NOT NOW** |
| BrassStormGatlingCannon | Weapon | not measured | not measured | - | Not judged | - | - | Filename date `20260511` | **NOT NOW** |
| IndustrialLandingPad | Prop | not measured | not measured | - | Not judged | Cinder platform | - | Filename date `20261007` | **NOT NOW** |
| NeonLumenAssaultRifle | Weapon | not measured | not measured | - | Not judged | - | - | Filename date `20260329` | **NOT NOW** |
| NeonSteelPlatform | Prop | not measured | not measured | - | Not judged | - | - | Filename date `20261025` | **NOT NOW** |
| ObsidianVanguard | Listed as prop / vehicle | not measured (141 MB) | not measured | - | Not judged | - | - | Filename date `20260607` | **NOT NOW** |
| OrbitalCargoModule | Prop | not measured (155 MB) | not measured | - | Not judged | Lane cargo; `SM_CargoPod_01..03` exist | - | Filename date `20260531` | **NOT NOW** |
| QuantumFreightPod | Prop | not measured | not measured | - | Not judged | Same | - | Filename date `20260727` | **NOT NOW** |
| RustyOrbitalCargoPod | Prop | not measured (140 MB) | not measured | - | Not judged | Same | - | Filename date `20260713` | **NOT NOW** |
| ModularFloorTile3d | Prop | not measured (133 MB) | not measured | - | Not judged | Interior floor; the `SM_DI_*` kit has floors | - | Filename date `20261024`; image-to-3D by filename | **NOT NOW** |

Excluded or held by Nicholas's side and not reviewed: Azure Shadow, the red-brick wall, Heraldic Castle Wall
(excluded); Earth from Space, Full Moon (held).

**Totals:** USE 1 (already in the game), USE AFTER CLEAN-UP 8, NOT NOW 46.

## 4. Shortlist: eight worth importing before the freeze, in order
All eight share these steps, so they are not repeated: fetch the source with
`git lfs fetch origin <ref> -I <path>` and `git cat-file blob <ref>:<path> | git lfs smudge > <scratch>`; copy it
untouched into `ArtSource/EngIntake/<Name>/source/` with a `SOURCE.md` (tool: Meshy AI by filename; supplied by
Nicholas; branch and commit; licence NOT RECORDED until he states the plan); run `cleanup_ai_mesh.py` headless with
`keep_source=True`; review renders in clay and material mode; `asset_report.py`; export with `blender-unreal-export`
(+X forward, cm, `UCX_` hull, `SOCKET_*`); import under `Content/TriadDrift/Gameplay/**`; one row in
`docs/eng/ASSETS.md` and one line in `CREDITS.md`. Announce each on the board with an `Asset:` subject first.
Total: about 18.5 builder-hours, so two builders can finish all eight on Nov 14; one builder should stop after pick 5.

1. **CoilgunAssaultRifle → the trooper's rifle** (`aa53ff9c`, `Meshy_AI_Coilgun_Assault_Rifle_1113061845_texture.glb`,
   4,124 tris). 1.5 h. Keep the mesh as it is; weld only. Scale to 85 cm long. Re-colour in the bake: body to the
   trooper's black (match `T_NullTrooper_BC`), coils and emissive to red-orange, one white pip at the muzzle. One 1K
   set (BC, N, ORM, E). Flip the normal map's green channel for DirectX. Pivot at the pistol grip, barrel along +X.
   There is no weapon slot in `UTDEnemyVisuals`, so the route with no code change is to join the rifle to
   `SK_NullTrooper` in Blender, weighted fully to `hand_r`, and place the existing `Muzzle` socket at the barrel tip;
   check `MF_Rifle_Idle_ADS` and `MM_Rifle_Fire` for the left hand. Done when the trooper is seen firing from the
   barrel in a running game at 10 m.
2. **CrimsonVanguard → a Null Armada ship in the lanes** (`aa53ff9c`, `Meshy_AI_Crimson_Vanguard_1113062628_texture.glb`,
   619,753 tris). 2.5 h. `target_tris=10000`, `sharp_angle_deg=35`, `bake_size=2048`. Length 14 m, nose +X, pivot
   at the centre. In the bake: push the hull to near-black, keep the red as seams, add a white eye bar at the
   cockpit. Sockets `Engine_0`, `Engine_1` at the two nozzles, `Muzzle_0`, `Muzzle_1` at the cannons. Import to
   `Gameplay/Characters/NullFleet/SM_Null_Vanguard` with an `MI` on the fleet master. First use: the background
   stream toward Verdance (storyline §12, §13), which needs no `DA`. Check it at 300 m beside `SM_Null_Fighter`: it
   must read as the same fleet.
3. **NebulaShadowfighter → the second lane ship** (`4587658`, `Meshy_AI_Nebula_Shadowfighter_1113031822_texture.glb`,
   7,741 tris). 1 h. No decimation. Length 11 m. Re-bake at 1K with the cyan lights turned red-orange and a white
   eye. Two engine sockets. Same folder and material as pick 2. Two silhouettes plus the fighter make the lanes read
   as a fleet and not a row of copies.
4. **Spaceship → the patrol's wreck** (`4587658`, `Meshy_AI_Spaceship_1113031758_texture.glb`, 697,703 tris). 4 h.
   `target_tris=12000`, 2K bake, length 18 m. Then cut it into three pieces (nose, mid hull, one engine ring) with
   capped, scorched breaks; kill the cyan glow on two of them and leave one engine ring flickering. Collision per
   piece. Import to `Gameplay/Props/Space/SM_Wreck_Patrol_{Nose,Hull,Engine}`. Use: drifting near the Dominion in
   the opening (the missing patrol), and, laid on the ground, the research vessel at the Verdance site beside the
   existing `SM_Verd_Wreck_*`. White hull with Triad's insignia shape is exactly the clue storyline §6 wants.
5. **CircularBulkheadWall → the Dominion's great door** (`aa53ff9c`, `Meshy_AI_Create_a_massive_hard_1113062957_texture.glb`,
   3,076,718 tris, 139 MB). 3.5 h, alone on the machine. `voxel_size` about 0.012 of its size, `target_tris=10000`,
   2K bake from the 8K source. Real size 8 m wide by 7 m tall, pivot at the floor centre. Darken to the interior
   kit's panel colour (`T_DI_Atlas_*`), add a red-orange seam around the port. Make the round port a separate
   two-leaf iris or a plain disc so it can open. Import to `Gameplay/Props/DominionInterior/SM_DI_GreatDoor` (+
   `_Port`). Use: the hangar mouth where the player docks and the core chamber entrance; the same frame, smaller,
   as the one door at the Cinder dig that recognises the pilots.
6. **GothicStoneArchWall → the recurring ancient artefact** (`4587658`,
   `Meshy_AI_Gothic_Stone_Arch_Wal_1113030746_image-to-3d-texture.glb`, 9,763 tris). 1.5 h, **only after Nicholas
   agrees the look and names the input image.** No decimation. 4 m tall, pivot at the base, one convex hull. Put the
   glyph's glow on a material parameter so it can be dark, blue, or pulsing. Import to
   `Gameplay/Props/Planet/SM_Ancient_Arch`. Place one on Verdance beside the relay, one in the Cinder dig, one in the
   core chamber.
7. **CrimsonTempest → a third lane ship** (`4587658`, `Meshy_AI_Crimson_Tempest_1113032024_texture.glb`, 11,842 tris).
   1.5 h. No decimation. Re-bake at 1K with the red panels pushed to black and the red kept as edge seams. Do this
   one only if picks 2 and 3 are in and the lanes still look thin.
8. **UnlabelledRifle01 → the heavy's cannon** (`55d0010`, `Meshy_AI_make_it_3d_rote_to__1113032038_texture.glb`,
   879,930 tris). 3 h. `target_tris=10000`, 1K bake, 110 cm long, joined to `SK_NullHeavy` at `hand_r` the same way
   as pick 1. Ask Nicholas for the input image first.

Not on the list on purpose: every character (the crew is covered, and a re-rig costs five hours each), every other
firearm, the landing pads and cargo pods (engineering versions are in the game), and every
date-stamped ZIP.

## 5. What the library does not cover (for Grok Bot's queue, R-030)
- **Triad's three ships and the Null Armada's six**: nothing here replaces the approved concepts (the Rodin ships,
  fighter, interceptor and gunship are delivered on `grok/assets-*`; bomber, carrier and turret are still to come).
- **A space drone** for the opening (the one that scans Rift and holds fire) and the drones that stream in lanes.
- **The Dominion**: hangars that open, the shield generator and reactor, the row of exterior lights.
- **The Warden** (delivered by Grok Bot, to be imported) and its animations; per-type crew animation sets.
- **The pilot's energy blade** (concept r1 delivered) and the blade animations (HY-Motion r1 delivered).
- **Story props:** the signal-analysis device, the stabilising crystals as a hand-sized pick-up, the relay, the
  Origin Core and its chamber, consoles and doors that recognise the pilots, the future command deck.
- **Damage variants** that show Triad's insignia shape under black plating (ships and troopers).
- **Set dressing in the right style:** the Verdance research site (lab modules, antennae, survey gear), the Cinder
  mining complex (drill rig, conveyors, ore haulers, floodlights), interior props for the Dominion (bunks, lockers,
  weapon racks, screens with Triad's layout).
- **Bioluminescent Verdance flora and Cinder rock** beyond what engineering built; hero trees.
- **Everything that is not a mesh:** sky and cloud textures, effect flipbooks, terrain sets, key art, the itch
  banner, menu still and trailer end card.
- **Anything in the faction palette.** No supplied model is black with red-orange seams and white eyes, or white
  ceramic with a squad colour; that look still has to come from Grok Bot's concepts or a re-texture pass.
