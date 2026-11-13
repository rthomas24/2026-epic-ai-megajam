# Energy blade - concept_r1 (brief)

R-030 queue item 2 (Ryan's request, originally R-027 #12), moved up with the pilot's blade combat
animations on 2026-11-13. This is the concept-gate round: three original design directions on one
sheet, in the squad material language. **3D (Rodin) waits on the Lead's pick of a direction.**

## Spec
The pilot's one-handed energy blade: **the physical hilt, guard and emitter only** — no baked
glowing blade volume. Must not read as a lightsaber. Squad language: white ceramic-composite over
gunmetal, thin cyan (`#3FE9FF`) strips and small emissive pips. One-handed, 30-45 cm long, silhouette
readable from a third-person chase camera. After approval, Rodin of the pick: <= 8k tris, 2K PBR,
pivot at the grip centre, +X along the blade, to `ArtSource/EngAI/Weapons/EnergyBlade/`. The energy
edge, trails and ignite/retract stay effects (and a later mechanical unfold, route B in
`animations.md`, if the folding direction is chosen).

## The three directions (left to right on the pick sheet)
1. **Open-frame fork emitter with a deflector guard.** Ribbed gunmetal grip, a swept white-ceramic
   deflector plate with a cyan edge, and a split wishbone emitter. The gap between the tines stays
   open (a faint cyan ghost is allowed on the concept only; it is not geometry for 3D).
2. **Folding single-edged sabre.** White ceramic grip with a dark wrap and a folding knuckle spine.
   Shown unfolded (a slim single-edged emitter rail locked out, with a cyan lit-state strip along
   the spine) and folded (rail stowed against the grip). The cyan strip on the concept is a lit
   indication, not a cylindrical blade volume.
3. **Compact survival blade with a glowing fuel cell.** Shorter, thicker utility hilt; a visible
   cyan fuel cell in the pommel; a protective emitter cage and a short wedge-shaped emitter.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or
  text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder.
- Unreal: cm, +X forward, +Z up; 2K textures at most.

## What this round delivers
- `source/sheet-EnergyBlade-nb21-s2202_PICK.png`: the chosen three-direction sheet (hero views of all
  three, plus folded and unfolded states of the sabre). The intended design-choice image.
- `source/sheet-EnergyBlade-turnaround-nb21-s3303_PICK.png`: extra views of those same three designs
  (fork front / side / back, sabre unfolded + folded, survival front and end-on).
- `source/` also keeps the seed-1101 three-direction alternate and a hero-pass of the pick designs
  (the hero pass is very close to the pick sheet).
- `contact_sheet.png`: the two picks stacked (three-direction sheet over the turnaround).
- No `refs/`: this round started from text, with no external image references. The turnaround and
  hero passes used the pick sheet as Image 1.
