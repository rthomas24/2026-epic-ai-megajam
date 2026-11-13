# Null Armada Trooper: brief

Owner: engineering (Ryan's side), taken over from Nick's list at Ryan's direct request on Nov 12, 2026.
Concept: `refs/enemy1.png` (four-view turnaround supplied by Ryan; crops in `refs/concept_*.png`). The concept is
the approved design: match it as closely as possible.

## Purpose and camera
- The on-foot enemy of both planets and the Dominion interior: the Trooper first, then re-used with small changes
  for the Grunt and the Heavy (features.md ENG-23).
- Seen in first person from 2 to 30 m, mostly 5 to 15 m, often several at once, at dusk on Verdance and in
  firelight on Cinder. It must read as hostile at a glance and its wind-up must be readable.

## Size, budget, materials
- Height 188 cm in the A-pose of the concept, on the UE5 mannequin (Manny) skeleton so it uses the project's
  animations with no retargeting. Capsule in gameplay: radius 34, half-height 90 cm.
- Budget: 35 000 to 60 000 triangles for LOD0 (one hero enemy type on screen up to about 8 at once), one skeletal
  mesh, at most 3 material slots (armour, undersuit, emissive may share one set).
- Textures: one 2048 set (BC, N, ORM, E), `T_NullTrooper_*`.
- Material plan (60 / 30 / 10): 60% dark gunmetal armour plates with worn edges and fine scratches (rough 0.45
  to 0.6, metallic), 30% matte black segmented undersuit (rough 0.8, non-metal), 10% accents: red-orange emissive
  and thin red pinstripe lines. Emissive at most 5% of the surface.

## Shape language
Armoured, insectile, predatory.

## Signature features (from the concept, must all be present)
- Helmet: smooth domed crown with a central ridge, angular faceplate tapering to a narrow jaw, **two swept-back
  antenna fins** rising behind the ear discs, **round ear discs with a glowing ring**, and a **cluster of three
  small red-orange eye lights on each side** of the faceplate (the white-eyed read of the placeholders becomes
  this red-orange cluster; see "Open question").
- Chest: layered pectoral plates, a **hexagonal chest core** with a red-orange glow, and **ribbed hoses** running
  from the core to the sides of the ribcage.
- Shoulders: large layered pauldrons, each with **two or three small round emissive studs**.
- Forearms: heavy bracers with a **long red-orange emissive panel** on the outer face (hex-grid texture).
- Back: a **backpack unit with a tall vertical emissive slot**, shoulder-blade plates.
- Waist: utility belt with square pouches all round and a central buckle plate.
- Legs: thigh plates with thin red pinstripes following the plate edges, **angular knee guards**, shin armour.
- Boots: heavy, strapped, thick tread soles, a small emissive bar at the heel.

## Palette
- Armour: gunmetal `#2B2D30` to `#3A3C40`, edge wear up to `#6A6C70`.
- Undersuit: near-black `#101113`.
- Emissive and pinstripes: Null Armada red-orange `#FF4A1E` (features.md colour code: enemies red and orange with
  black armour).

## Functional parts, sockets, collision
- Skeleton: UE5 `SK_Mannequin` (Manny). Skinning by weight transfer from Manny, rigid plates weighted to one bone
  where possible so armour does not bend.
- Sockets (on bones, added in Unreal): `Muzzle` on `hand_r` (blaster bolts start here; the gameplay tell glows
  here), `HeadGlow` on `head`.
- Collision: the gameplay capsule; a physics asset from the mannequin for ragdoll on death.

## Delivery
- Source: `ArtSource/EngCharacters/NullTrooper/` (versioned `.blend`, `review/`, `export/`).
- Unreal: `Content/TriadDrift/Gameplay/Characters/NullTrooper/` (`SK_NullTrooper`, `MI_NullTrooper_*`,
  `T_NullTrooper_*`), assigned to crew through `DA_Crew_Trooper_Visuals`.

## Route
Characters are route A or B in the asset skill ("never code-model people from nothing"). Two tracks:
1. **Now:** fitted armour built on the exported Manny body (the allowed mannequin route), so the character is on
   the right skeleton, at the right scale, animated and in the game.
2. Hyper3D Rodin image-to-3D was offered and **declined by Ryan (Nov 12)**: track 1 is the character that ships.

## Open question
The colour code in features.md says enemies have glowing **white** eyes; the concept has red-orange eye clusters.
The concept wins for this character unless Ryan says otherwise; the wind-up tell stays red-orange.

## References
`refs/enemy1.png` only. Original design; no existing game's or film's character is to be copied.
