# Pilot blade combat animations - r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (12:46 EST queue change, item 2
with the energy blade) and R-032 (`animations.md`, route A). This is the first
delivery of the on-foot pilot blade set. Builders retarget to `SKEL_Pilot`, cut to
gameplay lengths and wire the montages; T5 owns hit / deflect timing. This drop is
source plus processed FBX only. Do not merge without the Lead's OK.

## Spec (from the Lead)
- FBX, standard humanoid, 30 fps, one clip per file, T- or A-pose reference included.
- In place except the heavy lunge and the dash, which keep root motion.
- Clips: light 1, light 2, light 3 (a combo that flows, each 0.35-0.50 s to the hit),
  heavy lunge (0.6 s wind-up), air slam, block hold (loop) + deflect flick,
  perfect-deflect flourish, dash, hit react front / back, death x2, idle with the
  blade lit.
- One-handed (right hand), weighty and readable from a third-person chase camera.
- Folder: `ArtSource/EngAI/Animations/PilotBlade/`.

## What this round delivers
- 14 processed clips `A_PilotBlade_<Clip>.fbx` plus `A_PilotBlade_RefPose.fbx` (T-pose),
  all on the same 52-bone SMPL-H skeleton, 30 fps, mannequin mesh included for preview.
- `source/`: the picked raw fal FBX, untouched bytes, one file per clip.
- `previews/`: per-clip strips, a follow-cam strip for the travelling clips, and
  `contact_sheet.jpg`.
- `SOURCE.md`: tool, prompts, seeds, licence, root motion, frame ranges, hit frames,
  SMPL-H → `SKEL_Pilot` bone map, and an honest quality note per clip.

The skeleton is SMPL-H (Pelvis, Spine1-3, L_/R_ limbs and fingers), not UE5 Manny
names. Builders retarget to `SKEL_Pilot`. There is no weapon prop in the clip; the
right hand is a closed sword grip.

## Quality flags (please read)
- Light 1's measured hit is early (0.23 s vs the 0.35-0.50 s target). The slash itself
  is the strongest of the set.
- Light 1 / 2 / 3 are a usable combo but the start/end poses were not authored as a
  continuous take, so they will need a short blend.
- Air slam is the weak clip: a very high jump (~6 m) with overhead arms and a wide
  landing, not a tight downward sword smash. Flagged for a later round if time allows.
- In-place bakes zero horizontal pelvis travel and keep vertical motion, so any
  original foot step will skate (worst on Light 3, Flourish, the deaths and Air slam).
