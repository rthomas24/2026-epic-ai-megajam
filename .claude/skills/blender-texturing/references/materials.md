# Material recipes that read as AAA (Blender 5.2 → Unreal)

The Principled BSDF socket names in 5.x are `Base Color`, `Metallic`, `Roughness`, `Normal`,
`Emission Color`, `Emission Strength`, `Coat Weight` and `Specular IOR Level`. New materials already
have a node tree, so don't set `use_nodes`. Convert palette hex colours with
`hardsurface.hex_color()` (sRGB → linear).

## Value ranges that look right in Unreal

| Surface | Base colour | Metallic | Roughness | Notes |
|---|---|---|---|---|
| Painted hull (squad colours) | palette dominant | 0.2–0.35 | 0.35–0.5 | Low metallic keeps saturation; Coat 0.2–0.4 for a glossy livery |
| Secondary paint | palette secondary | 0.2–0.35 | 0.4–0.55 | Different roughness from the dominant, so they separate |
| Bare metal trim | `#8A939E` to `#22262C` | 1.0 | 0.2–0.35 | Engines, joints, hardpoints |
| Dark composite / rubber | `#1A1C20` | 0 | 0.6–0.85 | Intakes, seals, grips |
| Canopy (opaque stylized) | `#05202A` | 0 | 0.03–0.08 | Coat 1.0; avoid translucency (cost, no Nanite) |
| Emissive | dark base (≈ `#060606`) | 0 | 0.5 | Emission colour = palette accent, strength 1.5–3; ≤ 5% of the surface |
| Armada armour | `#0E0E12` | 0.4 | 0.25–0.35 | Glossy black reads menacing; crimson seams as emissive at strength 0.5–1 |
| Rock | two-tone noise (`make_rock_material`) | 0 | 0.75–0.9 | Fine-noise bump for normal detail |
| Crystal | saturated mid value | 0 | 0.05–0.2 | Flat shading; emission 0.3–1.5 |

**Never** use pure black (`#000`) or pure white (`#FFF`) albedo, a single roughness for the whole
asset, or metallic 1 on painted panels.

## Cheap "AAA" breakup (procedural, then bake)
- **Cavity dirt:** an Ambient Occlusion node (or a baked AO) → Color Ramp → multiply into base
  colour at 10–25%. Crevices darken and form reads better.
- **Edge wear:** Geometry `Pointiness` (Cycles only) or a Bevel-node-vs-normal difference →
  Color Ramp (narrow) → mix a lighter, more metallic, rougher "chipped" value at the convex edges.
  Keep it subtle (≤ 15% coverage) for a stylized look.
- **Livery:** stripes and numbers as separate geometry inset into material slots, or as Unreal decals
  (preferred for numbers). Hazard stripes are a Wave Texture (bands) masked to a region.
- **Roughness variation:** a large-scale noise at ±0.05–0.1 on roughness avoids the "plastic"
  look. Bake it into the ORM green channel.

Everything procedural here must go through `bake_texture_set.py` before Unreal. Flat values don't.

## Unreal side (for the import script / human)
- **Master `M_TD_Flat`:** vector params `BaseColor`, `EmissiveColor`; scalars `Metallic`,
  `Roughness`, `EmissiveStrength`. One MI per Blender material slot (the manifest's
  `material_params`).
- **Master `M_TD_Textured`:**
  - Texture params `BaseColorTex` (sRGB), `ORMTex` (Masks, sRGB off; R → AO, G → roughness,
    B → metallic), `NormalTex` (Normalmap compression, already DirectX), `EmissiveTex` (sRGB).
  - Scalar `EmissiveStrength` (× emissive).
  - One MI per baked asset.
- Emissive above 1.0 drives bloom. Tick **Emissive Light Source** on small glowing props if Lumen
  should pick them up.
