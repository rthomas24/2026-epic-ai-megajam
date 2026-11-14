# SPDX-License-Identifier: MIT
# Measure the squad hulls for landing gear: mesh bounds, sockets, and the visuals data asset's mesh transform.
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_ship_measure.py
import json
import os

import unreal

eas = unreal.EditorAssetLibrary
out = {}
for ship in ("Rift", "Lancer", "Bastion"):
    m = eas.load_asset("/Game/TriadDrift/Gameplay/Ships/%s/SM_%s" % (ship, ship))
    bb = m.get_bounding_box()
    socks = []
    try:
        for s in m.get_editor_property("sockets"):
            socks.append((str(s.socket_name), [s.relative_location.x, s.relative_location.y, s.relative_location.z]))
    except Exception as e:
        socks.append(str(e))
    da = eas.load_asset("/Game/TriadDrift/Gameplay/Characters/Squad/DA_%s_Visuals" % ship)
    props = {}
    if da:
        for name in ("mesh_scale", "mesh_offset", "mesh_rotation", "hull_scale", "hull_offset", "hull_rotation", "mesh_transform"):
            try:
                v = da.get_editor_property(name)
                props[name] = str(v)
            except Exception:
                pass
    out[ship] = {"min": [bb.min.x, bb.min.y, bb.min.z], "max": [bb.max.x, bb.max.y, bb.max.z], "sockets": socks, "da": props,
                 "tris": m.get_num_triangles(0)}
    unreal.log("TDPY: %s %s" % (ship, json.dumps(out[ship])))
path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "review", "ship_measure.json")
with open(path, "w", encoding="utf-8") as fh:
    json.dump(out, fh, indent=1)
