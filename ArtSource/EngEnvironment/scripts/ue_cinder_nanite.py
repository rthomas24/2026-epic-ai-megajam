# SPDX-License-Identifier: MIT
# T6's Cinder profile (Nov 14): crystals, basalt columns, small basalt rocks and charred trees were 15.5k of the 18.6k
# instances at the pad and none were Nanite. Turn Nanite on for those meshes (engineering's props), headless.
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngEnvironment\scripts\ue_cinder_nanite.py
import unreal

eal = unreal.EditorAssetLibrary
P = "/Game/TriadDrift/Gameplay/Props/"
names = ["Cinder/SM_Cind_Crystal_0%d" % i for i in range(1, 5)] + ["Cinder/SM_Cind_BasaltColumn_01", "Cinder/SM_Cind_BasaltColumn_02",
         "Cinder/SM_Cind_Basalt_03", "Scatter/SM_Scatter_Cind_CharTree_01", "Scatter/SM_Scatter_Cind_CharTree_02"]
done, bad = [], []
for n in names:
    m = eal.load_asset(P + n)
    if not isinstance(m, unreal.StaticMesh):
        bad.append(n)
        continue
    ns = m.get_editor_property("nanite_settings")
    if not ns.get_editor_property("enabled"):
        ns.set_editor_property("enabled", True)
        m.set_editor_property("nanite_settings", ns)
        try:
            unreal.StaticMeshEditorSubsystem().build_static_mesh(m)  # noqa
        except Exception:
            pass
        eal.save_asset(P + n, only_if_is_dirty=False)
    done.append(n)
unreal.log("TDPY: nanite on %s; missing %s" % (done, bad))
