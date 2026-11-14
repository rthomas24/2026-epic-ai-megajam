# Painted metal surfaces (#198-#200): evaluation, nothing imported

All three: Quixel Megascans via Fab, **Fab Standard License** (https://www.fab.com/eula), not AI-generated. Supplied by Nicholas
Tosti in the private release `asset-sources-nicholas-20261114`; archives hash-checked against the issues and kept in the git-ignored
`Builds/intake6/`; nothing in them was run. Previews (base colour or mask, 3x3 repeats) in `review/`.

| Issue | Set | Archive SHA-256 | What it is | Verdict |
|---|---|---|---|---|
| #198 | "Painted Metal Sheet" (industrial_metals_panel_metal_green_07, xeutbhl), https://www.fab.com/listings/0d9c3130-a796-4e2f-a444-706f69420663 | `fedb57768613b8e007159d0edd8fdf5509680d364ddf63fcd93a2506b1df83e3` | 4K tiling surface, full Megascans map set; flat green paint with rust specks and two scratch streaks that repeat clearly at 3x3 | **SKIP**: Earth-industrial green paint with an obvious repeat; our kit trim (bevels, panel lines, Null colours) is better for hut walls and Dominion panels |
| #199 | "Scratched Painted Metal" (slonbffc), https://www.fab.com/listings/838b7400-572e-45fd-b171-75658e8e82a6 | `afb1852ab8ae2eb73f2e4305a5081d9fca012c166a500ee1a4131eaac30f7dd4` | not a surface: a scratch / wear mask (gloss, mask, roughness at 1K-8K, no colour or normal) | **SKIP** for now: useful only through a master with a wear-mask input (`M_TD_PropTextured` has none); worth revisiting as a detail layer on the Dominion's black panels after the freeze |
| #200 | "Painted Metal Panel" (smlmmpg), https://www.fab.com/listings/e5fc65c4-1cfb-443e-8129-7b0d88a050ed | `2496752bc2f32b1b62dddf2a1c6039e1d12aff682a65983e5d742a084d0e9239` | not tiling: a single beige painted panel decal, 4096 x 2048 with an opacity map | **SKIP**: a one-off Earth panel decal, does not tile, does not fit the palette |

No cooked size added.
