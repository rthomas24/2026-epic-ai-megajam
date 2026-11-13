# Pilot anims step 0: list source sequences and probe the IK retarget Python API. Writes nothing.
import unreal


def log(m):
    unreal.log("TDPY: " + str(m))


SRC = [
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01",
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02",
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03",
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack",
    "/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Dash",
    "/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump",
    "/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Fall_Loop",
    "/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Land",
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Med_01",
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01",
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Back_Med_01",
    "/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01",
    "/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_02",
    "/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01",
    "/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle",
]


def main():
    for p in SRC:
        a = unreal.EditorAssetLibrary.load_asset(p)
        if a is None:
            log("MISSING " + p)
            continue
        skel = a.get_editor_property("skeleton")
        try:
            n = a.get_editor_property("number_of_sampled_keys")
        except Exception:
            n = None
        try:
            fr = unreal.AnimationLibrary.get_frame_rate(a) if hasattr(unreal.AnimationLibrary, "get_frame_rate") else None
        except Exception as e:
            fr = "err %s" % e
        try:
            nf = unreal.AnimationLibrary.get_num_frames(a)
        except Exception as e:
            nf = "err %s" % e
        rm = a.get_editor_property("enable_root_motion")
        log("SRC %s len=%.3f frames=%s keys=%s rate=%s skel=%s rootmotion=%s" % (
            p.split("/")[-1], a.get_play_length(), nf, n, fr, skel.get_name() if skel else None, rm))
    for name in ["IKRigController", "IKRetargeterController", "IKRetargetBatchOperation", "IKRigDefinition",
                 "IKRetargeter", "IKRigDefinitionFactory", "IKRetargetFactory", "RetargetChainSettings"]:
        c = getattr(unreal, name, None)
        log("API %s %s" % (name, "present" if c else "absent"))
        if c:
            meths = [m for m in dir(c) if not m.startswith("_")]
            log("API %s methods: %s" % (name, ", ".join(meths)))
    sk = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Mannequins/Meshes/SK_Mannequin")
    log("SK_Mannequin class %s" % (sk.get_class().get_name() if sk else None))
    pm = unreal.EditorAssetLibrary.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/SK_Pilot")
    log("SK_Pilot class %s" % (pm.get_class().get_name() if pm else None))
    log("OK")


main()
