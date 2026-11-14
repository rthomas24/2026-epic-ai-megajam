# Brief (R-042 item 3, Warden part)

From MESSAGEBOARD.md R-042, "Next in your queue": "the Warden's fire and summon clips, and crew trooper clip sets
(raw clips; the animation builder retargets them)". This r1 covers the Warden's fire (BoltFan) and summon only.

Delivery asked of Grok Bot: HY-Motion on fal (cap ~$3), retarget onto the Warden skeleton in Blender, clean up
(no foot sliding, weighty timing, anticipation -> release -> recovery, closed grips, root motion per brief,
30 fps), one FBX per clip, contact sheet + short MP4, SOURCE.md with prompts, settings, cost, notify frames and
usage terms, verified by reimport, one PR for Lead review.

Code timing (TDCrewCharacter.cpp): StartAttack plays the one-shot at telegraph start; BurstWindup 0.5 s
(BoltFan and Summon); ReleaseAttack -> DoBoltFan (7 bolts, 11 in phase 2, 60 deg arc, all at once) or DoSummon
(1 grunt, 2 in phase 2); RecoverLeft 0.5 s. Current fallbacks: MM_Rifle_Fire (fan), MM_ChargedAttack (summon, slam).
