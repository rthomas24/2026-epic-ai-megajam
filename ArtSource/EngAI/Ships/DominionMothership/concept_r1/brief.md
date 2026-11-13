# Dominion Mothership - concept_r1 (brief)

Round 1 of the R-030 item 6 / GitHub issue #41 **pre-mesh concept gate**. Ryan's Lead asked for a concept sheet of the Dominion Mothership exterior **before any hull meshing**, because this ship replaces the finale's centrepiece. The same R-030 item also calls for **four separate blue-domed shield generators**: the standalone Shield Generator mesh is delivered first in its own package; this round is the **hull only**, with empty dorsal mounting sockets so Engineering can place four generator meshes in-engine.

**If approved, concept_r1 is the sign-off reference for the hull mesh pass (Rodin / Blender).** Please confirm the dorsal-socket layout and the target scale (notes below) before that pass starts.

## Spec
**Dominion Mothership** (finale enemy capital ship, issue #41). Elongated heavy dark-slate armoured dreadnought silhouette, a tiered central citadel spire, ventral structural spires, warm amber engine / hangar-bay slits, and vertical deep-red hull banners with a white stylized trident emblem. Dorsal deck: the intended brief is **four symmetrical circular mounting pads in a 2×2 port-and-starboard layout** (two forward, two mid-aft ahead of the bridge tower) — flat armoured docking rings with faint amber rim lights and **no blue energy domes baked into the hull**.

**v3 socket-layout note (please confirm):** the v3 pick keeps the empty pads (no domes), which is correct. The top orthographic view, however, still reads as **three large circular pads in a forward-to-mid centerline row** (one forward, two mid, closely related to superseded v2) rather than a clear 2×2 of four port/starboard sockets. The 3/4 hero shows the near-side forward pad clearly. Lead / Engineering: please confirm whether to mesh this three-pad layout, or to request a v4 with an unmistakable 2×2 of four sockets, before the hull mesh starts.

Target scale for this gate: **~200 m dreadnought class** (180 cm pilot = 0.9% of a 200 m hull). The ship is **seen from a ship only** (not walkable). Exact engine target scale is confirmable during the mesh pass. Two other figures exist in the record and should be reconciled then:
- Nicholas's modelling reference board labels the ship at ~4,200 m (13,780 ft) length / ~820 ft height.
- The current engineering stand-in hull (`ArtSource/EngShips/Dominion/brief.md`) uses a ~60,000 cm (~600 m) envelope.

Target (for the 3D step, after sign-off): FBX + GLB; Unreal cm, **+X forward**, **+Z up**; 2K PBR (`BC`, DirectX `N`, `ORM`, thresholded `E`). Four separate Shield Generator meshes are socketed onto the four dorsal pads in-engine; they are not part of this hull package.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands. The deep-red banners and white stylized trident are the Dominion faction markings from Nicholas's original #41 board; do not add any other names, logos, insignia or text.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder.
- Unreal: cm, +X forward, +Z up; 2K textures at most.

## What this round delivers
- `source/sheet-DominionMothership-v3_PICK.png`: the chosen 4K 16:9 turnaround (large 3/4 hero plus top, side, front and rear orthos). Untouched fal bytes; this is the intended hull-concept input once the socket layout is signed off.
- `source/` also keeps every alternate, untouched (renamed only; original fal file names in `SOURCE.md`): `sheet-DominionMothership-v1.png` (initial 4K sheet) and `sheet-DominionMothership-v2.png` (3 inline pads, superseded).
- `contact_sheet.png`: reference vs v3 pick (hero, orthos and a dorsal-socket callout), a 200 m / socket / 180 cm pilot scale bar, and the v1 / v2 / v3 thumbnails.
- `refs/Mothership_Modeling_Reference.png`: Nicholas's #41 modelling reference, untouched, as sent to the model.
