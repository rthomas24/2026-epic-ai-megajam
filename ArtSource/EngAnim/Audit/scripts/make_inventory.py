# Writes ArtSource/EngAnim/Audit/inventory.md from Saved/EngAnim/{inventory,metrics,retarget_report,derive_report}.json
# (made by ue_i00_inventory.py, ue_i10_metrics.py, ue_r10_retarget.py, ue_r20_derive.py). Prose sections are in this file.
import json, os
S = "Saved/EngAnim/"
inv = json.load(open(S + "inventory.json"))
ret = [r for r in json.load(open(S + "retarget_report.json")) if r.get("status") == "new"]
der = json.load(open(S + "derive_report.json"))
used_src = {}
for r in ret:
    used_src.setdefault(r["source"], []).append(r["name"])
OLD_CREW = {"MM_Idle", "MF_Unarmed_Walk_Fwd", "MF_Unarmed_Jog_Fwd", "MF_Rifle_Idle_ADS", "MF_Rifle_Walk_Fwd", "MF_Rifle_Jog_Fwd", "MM_Attack_01",
            "MM_ChargedAttack", "MM_Rifle_Fire", "MM_HitReact_Front_Lgt_01", "MM_Death_Front_01", "MM_Death_Back_01", "MM_Death_Left_01", "MM_Death_Right_01"}
OLD_PILOT = {"MM_Attack_01": "A_Pilot_Light1", "MM_Attack_02": "A_Pilot_Alt_LeftHook (mirrored: A_Pilot_Light2)", "MM_Attack_03": "A_Pilot_Alt_SpinKick",
             "MM_ChargedAttack": "A_Pilot_Heavy (release: A_Pilot_Light3)", "MM_Dash": "A_Pilot_Dash", "MM_Jump": "A_Pilot_JumpStart", "MM_Fall_Loop": "A_Pilot_FallLoop",
             "MM_Land": "A_Pilot_Land", "MM_HitReact_Front_Med_01": "A_Pilot_HitReact_Front", "MM_HitReact_Back_Med_01": "A_Pilot_HitReact_Back",
             "MM_Death_Front_01": "A_Pilot_Death_Front", "MM_Death_Back_01": "A_Pilot_Death_Back"}
new_names = set(r["name"] for r in ret) | set(r["name"] for r in der) | set(r["name"] + "_Add" for r in ret if r.get("additive_copy"))
USE = {  # how code uses the clips that existed before today
    "A_Pilot_Idle3": "DA_Pilot_Visuals.Idle (speed < 60)", "A_Pilot_Walk": "DA_Pilot_Visuals.Walk (60-520 cm/s, rate speed/300 clamped 0.6-1.5)",
    "A_Pilot_Run": "DA_Pilot_Visuals.Run (>= 520 cm/s, rate speed/650 clamped 0.6-1.5)", "A_Pilot_Idle02": "unused", "A_Pilot_Idle6": "unused", "A_Pilot_CasualWalk": "unused",
    "A_Pilot_FallLoop": "loop while falling (rate 1)", "A_Pilot_JumpStart": "one-shot x1.4 on MANTLE only (a jump plays nothing)", "A_Pilot_Land": "unused (no code plays it)",
    "A_Pilot_Dash": "one-shot x1.8 on dash, x0.8 on slide", "A_Pilot_Light1": "one-shot x1.6 on Light1 (upper body only when moving > 150 cm/s)",
    "A_Pilot_Light2": "one-shot x1.6 on Light2", "A_Pilot_Light3": "one-shot x1.5 on Light3; x1.2 on AirSlam", "A_Pilot_Heavy": "one-shot x1.4 on HeavyLunge and DashStrike",
    "A_Pilot_Block": "upper-body loop (spine_01 down) while Blocking, 0.12 s fade", "A_Pilot_HitReact_Front": "asked for on damage and GuardBroken, but REFUSED by PlayOneShot (additive): never plays",
    "A_Pilot_HitReact_Back": "asked for on damage from behind, REFUSED (additive): never plays", "A_Pilot_HitReact_Front_Add": "unused (needs an additive layer in code)",
    "A_Pilot_HitReact_Back_Add": "unused (needs an additive layer in code)", "A_Pilot_Death_Front": "unused (death is a ragdoll)", "A_Pilot_Death_Back": "unused (death is a ragdoll)",
    "A_Pilot_Alt_LeftHook": "unused", "A_Pilot_Alt_SpinKick": "unused",
    "MM_Idle": "grunt idle loop (speed < 40)", "MF_Unarmed_Walk_Fwd": "grunt loop 40-300 cm/s, rate speed/200", "MF_Unarmed_Jog_Fwd": "grunt loop >= 300, rate speed/450",
    "MF_Rifle_Idle_ADS": "trooper / heavy / warden idle loop", "MF_Rifle_Walk_Fwd": "trooper / heavy loop 40-300, rate speed/200", "MF_Rifle_Jog_Fwd": "trooper / heavy loop >= 300, rate speed/450",
    "MM_Attack_01": "one-shot at the START of a melee wind-up (grunt 0.6 s, heavy 0.7 s)", "MM_ChargedAttack": "one-shot at the start of ChargeSlam / Summon (warden)",
    "MM_Rifle_Fire": "one-shot at the start of a burst / shotgun / fan wind-up (once per attack, not per shot)", "MM_HitReact_Front_Lgt_01": "one-shot on hull damage (not grunt, not while attacking)",
    "MM_Death_Front_01": "PlayAnimation on death, by hit direction", "MM_Death_Back_01": "same", "MM_Death_Left_01": "same", "MM_Death_Right_01": "same",
}
LEDGER = [  # (path fragment, ledger row it could fill)
    ("Rifle/Walk", "5.2 trooper walk (8-way strafe set)"), ("Rifle/Jog", "5.2 trooper run (8-way strafe set)"), ("Rifle/Jump", "5.2 trooper (jump / drop; not in the ledger)"),
    ("Rifle/AIM", "5.2 trooper aim (aim offset poses up / centre / down)"), ("Rifle/HitReact", "5.2 hit react variety (all crew), 5.1 pilot hit react variety"),
    ("MM_Rifle_Reload", "5.2 trooper reload"), ("MM_Rifle_Equip", "5.2 trooper (weapon raise on alert); 5.1 pilot interact candidate"), ("MM_Rifle_DryFire", "5.2 trooper (jam / empty)"),
    ("MM_Rifle_Fire", "5.2 trooper fire"), ("MF_Rifle_Idle_ADS", "5.2 trooper idle / aim"),
    ("Pistol/Walk", "5.2 heavy slow walk (two hands forward = shield presented)"), ("Pistol/Jog", "5.2 heavy (not needed at 330 cm/s)"), ("Pistol/Jump", "none"), ("Pistol/Aim", "5.2 heavy aim offset"),
    ("MF_Pistol_Idle_ADS", "5.2 heavy idle, shield up"), ("MM_Pistol_Equip", "5.2 heavy shield up; 5.1 pilot blade draw / pickup candidate"), ("MM_Pistol_Reload", "5.2 heavy shotgun reload; 5.1 pilot console interact candidate"),
    ("MM_Pistol_Fire", "5.2 heavy shotgun fire (additive kick)"), ("MM_Pistol_DryFire", "none"),
    ("Unarmed/Walk", "5.1 pilot strafe / backpedal (walk); 5.2 grunt walk strafes"), ("Unarmed/Jog", "5.1 pilot strafe / backpedal (run); 5.2 grunt charge strafes"),
    ("Unarmed/Attack", "5.2 grunt melee variety; 5.1 pilot (already used)"), ("MM_Dash", "5.2 grunt charge lunge; 5.1 pilot dash (already used)"), ("MM_WallJump", "5.1 pilot double jump / mantle candidate"),
    ("Unarmed/Jump", "5.1 pilot jump (already used); 5.2 grunt"), ("Death/", "5.1 / 5.2 death variety"), ("Unarmed/MM_Idle", "5.2 grunt idle (already used)"),
]


def ledger(path):
    for frag, row in LEDGER:
        if frag in path:
            return row
    return ""


def seqs(prefix):
    return [r for r in inv if r["class"] == "AnimSequence" and r["path"].startswith(prefix)]


L = []
A = L.append
A("# Animation inventory (audit, Nov 13)\n")
A("Made by the animation builder on `lead/int` (1050b21 + today's new clips). Numbers come from the headless scripts in `ArtSource/EngAnim/Audit/scripts/`")
A("(`ue_i00_inventory.py`, `ue_i10_metrics.py`, `ue_i20_visuals.py`) and from reading `Source/unrealGameJam/TriadDrift/Ground/**`. All clips are 30 fps.\n")
ours = seqs("/Game/TriadDrift/")
before = [r for r in ours if r["path"].split("/")[-1] not in new_names]
tmpl = seqs("/Game/Characters/")
A("## Headline numbers\n")
A("- Before today: **%d animation sequences** of ours (%d pilot, %d crew), 3 retargeters, no animation blueprint, no blend space, no montage, no notify on any clip." % (
    len(before), sum(1 for r in before if "/Pilot/" in r["path"]), sum(1 for r in before if "/Crew/" in r["path"])))
A("- Template content that can be retargeted: **%d sequences** on `SK_Mannequin` (`/Game/Characters/Mannequins/Anims/**`): unarmed 8-way walk and jog, rifle 8-way walk and jog, pistol 8-way walk and jog, 4 unarmed attacks, dash, jumps, 6 deaths, 8 hit reacts, reloads, equips, aim-offset poses. %d of them were already in use; **%d more were retargeted today**." % (
    len(tmpl), len(set(list(OLD_CREW) + list(OLD_PILOT))), len(used_src)))
A("- `Content/Variant_Shooter`, `Variant_Horror`, `FirstPerson` hold **no sequences of their own**: only animation blueprints (`ABP_FP_Pistol`, `ABP_FP_Weapon`, `ABP_TP_Pistol`, `ABP_TP_Rifle`, `ABP_FP_Copy`) and one montage (`FP_Rifle_Shoot_Montage`) that play the mannequin clips above. `Content/Weapons` has three weapon meshes with skeletons and no clips.")
A("- Engine content: nothing useful for a humanoid was found referenced by the project (the engine ships only tutorial and test sequences; not checked plugin by plugin).")
A("- Nicholas's Sentinel sample GLB: **6 clips, all 6 already imported** (see section 3).")
A("- Added today: **%d retargeted clips** (+ %d additive copies) and **%d derived clips** (re-timed or full-pose copies). Listed in section 5.\n" % (
    len(ret), sum(1 for r in ret if r.get("additive_copy")), len(der)))

A("## 1. What existed before today\n")
A("| Clip | Folder | Length (s) | Frames | Additive | Root travel left in the clip (cm) | How the code uses it |")
A("|---|---|---|---|---|---|---|")
for r in sorted(before, key=lambda r: r["path"]):
    n = r["path"].split("/")[-1]
    A("| `%s` | `%s` | %.3f | %d | %s | %s | %s |" % (n, "/".join(r["path"].split("/")[5:-1]), r["len"], r["frames"], r["additive"].replace("AAT_", "").replace("NONE", "no"),
                                                 ("%.0f" % r["root_travel_max"]) if r["root_travel_max"] else "0", USE.get(n, "")))
A("\nRetargeters and rigs: `Pilot/Anims/RTG_Manny_to_Pilot` (`IK_Pilot_SrcManny` -> `IK_Pilot`), `Crew/RTG_Manny_to_NullCrew` (`Crew/Anims/IK_Crew_Manny` -> `Crew/IK_NullCrew`), `Crew/Anims/RTG_Crew_Manny_Identity` (no longer needed).")
A("Skeletons: `SKEL_Pilot` (75 bones, Meshy rig with mannequin names, no twist bones), `SKEL_NullCrew` (shared by `SK_NullTrooper`, `SK_NullGrunt`, `SK_NullHeavy`; the drone is a static mesh).")
A("Visuals assets: `DA_Pilot_Visuals` has Idle = `A_Pilot_Idle3`, Walk = `A_Pilot_Walk`, Run = `A_Pilot_Run`, no AnimBPClass. `DA_Crew_Trooper/Grunt/Heavy_Visuals` have a mesh and no AnimBPClass; `UTDEnemyVisuals` has **no clip slots at all**.\n")

A(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "inventory_code.md")).read())

A("## 3. Nicholas's Sentinel Armor sample GLB (read only)\n")
A("`ArtSource/Characters/Samples/SentinelArmor/Meshy_AI_Sentinel_Armor_Turnaround_20261113_animated_ue5_mannequin_All_animation.glb`: 1 skin, 74 joints, 6 animations, all in place. Every one was imported on Nov 12 (`ArtSource/EngCharacters/Pilot/SOURCE.md`). **Nothing is left to import from it.**\n")
A("| GLB clip | Length (s) | Keys | Imported as | Used |")
A("|---|---|---|---|---|")
for a, b, c, d, e in (("Running", 0.633, 20, "A_Pilot_Run", "Run slot"), ("Walking", 1.033, 32, "A_Pilot_Walk", "Walk slot"), ("Casual_Walk", 4.167, 126, "A_Pilot_CasualWalk", "no (6 cm pop at the loop point)"),
                      ("Idle_02", 1.867, 57, "A_Pilot_Idle02", "no (guard stance; a blade-ready idle candidate)"), ("Idle_3", 9.967, 300, "A_Pilot_Idle3", "Idle slot"), ("Idle_6", 7.367, 222, "A_Pilot_Idle6", "no (idle variant candidate)")):
    A("| %s | %.3f | %d | `%s` | %s |" % (a, b, c, d, e))

A("\n## 4. Template content available for retargeting (`/Game/Characters/Mannequins/Anims/**`, skeleton `SK_Mannequin`)\n")
A("Root travel is what the clip moves the root bone (root-motion clips); our players are in place, so it is removed on retarget. \"Now\" = the asset of ours made from it (bold = made today).\n")
A("| Template clip | Length (s) | Frames | Additive | Root travel (cm) | Ledger row it can fill | Now |")
A("|---|---|---|---|---|---|---|")
for r in sorted(tmpl, key=lambda r: r["path"]):
    n = r["path"].split("/")[-1]
    now = []
    if n in OLD_CREW:
        now.append("`Crew/Anims/%s`" % n)
    if n in OLD_PILOT:
        now.append("`%s`" % OLD_PILOT[n])
    now += ["**`%s`**" % x for x in used_src.get(r["path"], [])]
    A("| `%s` | %.3f | %d | %s | %s | %s | %s |" % (r["path"].replace("/Game/Characters/Mannequins/Anims/", ""), r["len"], r["frames"],
        r["additive"].replace("AAT_", "").replace("NONE", "no"), ("%.0f" % r["root_travel_max"]) if r["root_travel_max"] else "0", ledger(r["path"]), ", ".join(now) or "not used"))
A("\nOdd sources (as shipped by Epic, flagged so nobody trusts them blindly): `Rifle/Jog/MF_Rifle_Jog_Left` is 14 frames (0.467 s, half a cycle; its partner `_Right` is 32 frames); `Rifle/Jog/MF_Rifle_Jog_Fwd_Right` travels twice as far as `_Fwd_Left` in the same time; `Unarmed/Jog/MF_Unarmed_Jog_Left` / `_Right` and the pistol and rifle `Walk_Left` / `_Right` pairs have different lengths (a blend space must sync them by markers or normalised time).")
A("Not retargeted: the pistol jog and jump sets, `MM_Pistol_DryFire`, `MM_Pistol_Fire` (pure additive kick), `Rifle/Jump/MM_Rifle_Jump_Fall_Loop` / `_Start_Loop` / `_RecoveryAdditive`, the single-frame pistol aim-offset poses.\n")

A("## 5. Added today (all new files; nothing existing was changed)\n")
A("Retargeted with the existing retargeters by `ArtSource/EngAnim/Retarget/scripts/ue_r10_retarget.py` (root translation zeroed and root motion off except deaths; ACL codec). `_Add` = an additive copy (local space, against its own first frame) saved beside the full-pose clip.\n")
A("| New clip | Skeleton | From | Length (s) | Frames | Root travel removed (cm) | Additive copy |")
A("|---|---|---|---|---|---|---|")
for r in sorted(ret, key=lambda r: r["name"]):
    A("| `%s` | %s | `%s` | %.3f | %d | %s | %s |" % (r["name"], r["skel"], r["source"].replace("/Game/Characters/Mannequins/Anims/", ""), r["len"], r["frames"],
                                               "%.0f" % r["root_travel_removed"] if r["mode"] != "keep" else "kept (death)", "`%s_Add`" % r["name"] if r.get("additive_copy") else ""))
A("\nDerived by `ue_r20_derive.py` (copies; the originals are untouched):\n")
A("| New clip | From | Length (s) | Frames | What changed |")
A("|---|---|---|---|---|")
for r in der:
    A("| `%s` | `%s` | %.3f | %d | %s |" % (r["name"], r["source"].split("/")[-1], r["len"], r["frames"], r["why"]))
A("")
open("ArtSource/EngAnim/Audit/inventory.md", "w", encoding="utf-8").write("\n".join(L) + "\n")
print("WROTE inventory.md", len(L), "lines")
