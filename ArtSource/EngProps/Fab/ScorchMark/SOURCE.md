# Scorch Mark decals (Quixel Megascans "Scorch Mark")

Deferred decals for blast and burn marks: `MI_Fab_ScorchMark_Cinder` (pale ash with a dim ember in the hottest core: Cinder's
ground is near-black basalt, so a dark char does not read there) and `MI_Fab_ScorchMark_Verdance` (cool near-black char for
crash sites and battle damage round outposts), on a new master `M_TD_FabDecal`, with `T_Fab_ScorchMark_Mask`, all in
`/Game/TriadDrift/Gameplay/Props/Fab/ScorchMark/`. Not placed anywhere; the environment builder places them (decal actor,
projected down, about 4-8 m across).

## Source and licence

- "Scorch Mark" by **Quixel Megascans** (wlbkbdhc), https://www.fab.com/listings/414e0206-95fb-420a-96ef-e885fe65d707 (issue #202).
  Supplied by Nicholas Tosti (release `asset-sources-nicholas-20261114`, archive `QuixelScorchMark-Nicholas-20261114.zip`,
  SHA-256 `1128f44b9a803bfd346eac59b396ad465008c1c3917598ab10ad3d30c50b728a`; original kept in the git-ignored `Builds/intake7/`).
- **Licence: Fab Standard License** (https://www.fab.com/eula); tier acquired not recorded. **Not AI-generated**.

## What was done (Ryan's side, Claude Code, Nov 14 2026)

- The pack holds only a brush mask (1K-8K). `../scripts/prep_fab_scorch.py` made `T_Fab_ScorchMark_Mask` at 1024 from the 2K brush
  and previewed both looks composited top-down on our own ground textures (`T_Fab_TundraVolcanic_BC` for Cinder,
  `T_Fab_ForestNordicMoss_BC` for Verdance): `review/Fab_ScorchMark_on_ground.png` (left plain, right with the decal). A first pass
  (hard contrast, dark char on Cinder, strong ember) read as a flat splat and an orange flood; the looks were softened.
- `../scripts/ue_import_fab_decal.py` (headless) built `M_TD_FabDecal`: deferred decal, translucent; opacity =
  saturate((mask - Bias) x Contrast) x Opacity, base colour CharColor, roughness CharRoughness, emissive EmberColor x EmberStrength x mask^EmberPower.
  The instance values are in `prep_report.json` (`looks`); `EmberStrength` is the knob if the Cinder ember reads too warm or too weak.
- Mask texture: grayscale (linear), clamped, max 1024. Estimated cooked cost about 1.5 MB (mask 1.3 MB with mips, plus the material).
- Not yet seen in the engine: the decal shaders compile on first load in a rendering editor (the import ran with no renderer).
