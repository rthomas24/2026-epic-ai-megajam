# Triad Drift art direction: palettes, scale, budgets

The tone is bright, saturated, colourful sci-fi: huge scale, bold silhouettes, glowing engines and
readable arcade combat. It should be *in the spirit of* expansive, colourful space games, with
original designs only. Colours below are sRGB hex. Convert them with `hardsurface.hex_color("#...")`
before use in Blender, which needs linear values.

## Palettes

| Group | Dominant (60%) | Secondary (30%) | Trim | Emissive accent |
|---|---|---|---|---|
| **Rift** (Trickster, pilot Wren) | violet `#6B2BD9` | deep plum `#2A0F4F` | gunmetal `#22262C` | magenta `#FF2BD6` |
| **Lancer** (Interceptor, Kato) | ice white `#E8F4FF` | cyan `#12C8FF` | gunmetal `#22262C` | cyan `#3FE9FF` |
| **Bastion** (Heavy, Moss) | amber gold `#F2A01E` | dark gunmetal `#2B2F36` | steel `#8A939E` | amber `#FFB21E` |
| **Null Armada** (enemy) | gloss black `#0E0E12` | dark crimson `#4A0A12` | crimson seams `#C3122E` | white "eye" `#F4F7FF`, red `#FF1E3C` |
| **Verdance** (jungle world) | teal `#19C3B1` | violet `#7B3FE4` | deep blue-green `#0F3B3A` | bio-glow `#3CFFE0` |
| **Cinder** (volcanic world) | basalt `#1A1718` | ash grey `#4A4446` | cyan crystal `#2EE6FF` | amber crystal `#FFB347`, magma `#FF5A1F` |
| **Dominion interior** | gloss black `#0B0B0E` | gunmetal `#1E2128` | red strip `#FF1E3C` | white lens `#F4F7FF` |

**Readability rules:**
- Player projectiles use squad colours; enemy projectiles use red or orange.
- Nothing important should be lost against a dark space background, so hulls stay mid-to-high
  value with emissive rims.
- Blender's AgX view transform desaturates bright glows in review renders, and Unreal's bloom will
  read stronger. Judge glow colour from the material values.

## Real-world sizes (1 Blender m = 100 Unreal units)

| Thing | Size |
|---|---|
| Player fighters | Rift ~7 m long, Lancer ~8 m long, Bastion ~6 m long × 9 m wide |
| Cockpit canopy (fighters) | ~1 m wide, 1.8–2.2 m long |
| Enemy fighters / interceptors | 5–7 m |
| Gunship / bomber | 20–30 m |
| Hive Carrier | 100–150 m |
| Dominion | 600–1000 m (≥ 50× Bastion), built from modular Nanite pieces |
| First-person character | 180 cm eye-height basis; corridor modules 400 × 400 cm, doors 200 × 300 cm |
| Asteroids | combat-zone rocks 3–40 m; field fillers 0.5–3 m |
| Crystal clusters (Cinder) | 1–4 m tall |

## Budgets (UE5 desktop, jam)

| Asset | Tris (LOD0) | Textures / materials | Notes |
|---|---|---|---|
| Hero player ship | 20k–60k | one 2K set (BC/N/ORM/E), or flat MIs + 2K normal | Opaque → Nanite OK; canopy opaque glossy (no translucency) |
| Enemy fighter | 5k–20k | 1K–2K or flat MIs | Many on screen |
| Gunship / bomber | 15k–40k | 2K | |
| Carrier / Dominion pieces | Nanite 50k–200k per piece | trim sheet + decals | |
| Corridor kit piece | 1k–8k | shared 2K trim sheet | Texel density ~512 px/m |
| Prop | 0.5k–5k | 512–1K, or flat MIs | |
| Asteroid (near / hero) | Nanite 20k–80k, or 2k–6k + normal map | 1K–2K | |
| Crystal cluster | < 2k | flat MI + emissive | Faceted (flat shaded) |
| Energy blade | < 1.5k | flat emissive MI | Separate mesh; not Nanite if translucent |

**Texel density:** hero ships 256–512 px/m (a 2K set on a 7 m ship ≈ 290 px/m, which is fine for
stylized surfaces), props 256–512 px/m. Large surfaces get detail from tiling or trim materials in
Unreal.

**Camera distances to design for:** chase camera 6–30 m from player ships; enemies 20–500 m;
first-person interiors 1–10 m.
