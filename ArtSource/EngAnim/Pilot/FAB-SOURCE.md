# Fab animation clips: evaluation record (animation builder, Nov 14)

Source: "Free Animations Pack 2" by SoerGame on Fab (Standard licence), imported on main 99d489e7 under
`Content/TriadDrift/Gameplay/Characters/Shared/Anims/Fab/` (credited in CREDITS.md). Retargeted with
`ArtSource/EngAnim/Pilot/scripts/ue_fab_retarget.py` (pilot: RTG_Manny_to_Pilot; crew: RTG_Manny_to_NullCrew).

| Clip | Evaluated for | Decision | Why |
|---|---|---|---|
| `A_Fab_FightingIdle` (1.07 s) | the Grunt's standing loop | **used**: `A_Crew_Grunt_Idle_Fight` (T5's slot) | fists-up stance matching the end of the Grunt's MeleeCombo; today's `MM_Idle` is a relaxed template idle |
| `A_Fab_Hack` (4.7 s) | the pilot's console interaction | not used (retargeted copy removed) | a kneeling hack (pelvis 36 cm, hands 18-25 cm off the floor for the whole clip, no kneel-in or stand-up); our consoles are at chest height and `A_Pilot_Interact_Console` reaches them |
| `A_Fab_TakesOutGun` (3.03 s) | the Trooper's first sight of the pilot | not used (retargeted copy removed) | a one-handed pistol draw that holsters again; the Trooper carries a two-handed rifle |
| `A_Fab_TakeDown` (3.33 s) | (finisher) | not used, not retargeted | no slot for a takedown in the game |
