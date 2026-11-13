# Probe: exact socket transforms on the current crew meshes (from the original FBX SOCKET_ empties). Saves nothing.
import unreal
for a in ("NullTrooper", "NullGrunt", "NullHeavy"):
    m = unreal.EditorAssetLibrary.load_asset("/Game/TriadDrift/Gameplay/Characters/%s/SK_%s" % (a, a))
    for i in range(m.num_sockets()):
        s = m.get_socket_by_index(i)
        l = s.get_editor_property("relative_location"); r = s.get_editor_property("relative_rotation"); sc = s.get_editor_property("relative_scale")
        unreal.log("TDPY: %s %s@%s loc=(%.4f,%.4f,%.4f) pitch=%.4f yaw=%.4f roll=%.4f scale=(%.3f,%.3f,%.3f)" % (
            a, s.get_editor_property("socket_name"), s.get_editor_property("bone_name"), l.x, l.y, l.z, r.pitch, r.yaw, r.roll, sc.x, sc.y, sc.z))
unreal.log("TDPY: OK")
