# A_Fab_* clips (Fab "Free Animations Pack 2")

Four raw clips on the project's UE5 mannequin skeleton (`/Game/Characters/Mannequins/Meshes/SK_Mannequin`), not retargeted:
the animation builder retargets them to the pilot and crew.

| Clip | Length | Intended use (Lead's pick) |
|---|---|---|
| `A_Fab_Hack` | 4.70 s | pilot uses a console or terminal |
| `A_Fab_FightingIdle` | 1.07 s (loop) | crew alert / ready stance |
| `A_Fab_TakeDown` | 3.33 s | a finisher or crew grab |
| `A_Fab_TakesOutGun` | 3.03 s | trooper draws a weapon |

All in `/Game/TriadDrift/Gameplay/Characters/Shared/Anims/Fab/`; preview mesh the template `SKM_Manny_Simple`.

## Source and licence

- "Free Animations Pack 2" by **SoerGame**, https://www.fab.com/listings/223581b5-2619-4632-9762-0097a108e7a3 (issue #185).
  Supplied by Nicholas Tosti (release `asset-sources-nicholas-20261114`, archive SHA-256
  `fd04aa97e9a8be8b650a890d1e78d1778ca4375fcc94a9a7be68540b82aecdec`, original kept in the git-ignored `Builds/intake2/FreeAnims2/`).
- **Licence: Fab Standard License** (https://www.fab.com/eula); tier acquired not recorded. **Not AI-generated** (NoAI label).
  No standalone redistribution. The pack's demo mannequin was not imported (only its skeleton, merged into ours).
- The other eight clips of the pack were not taken.

## How (Ryan's side, Claude Code, Nov 14 2026)

The four `AS_*` packages and the pack's `SK_Mannequin` were copied unchanged into a staging folder
`Content/FreeAnimationsPack2/` (never committed); `../scripts/ue_import_fab_anims.py` (headless editor, -nullrhi) consolidated
that skeleton into the template one, moved and renamed the clips, set the preview mesh, and deleted the staging folder. The
template skeleton file itself was not changed. Sheet: `review/Fab_FreeAnims2_clips.png` (the pack's own editor thumbnails of
the four clips). Stage: technically verified (skeleton and length checked); not played.
