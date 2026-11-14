# SPDX-License-Identifier: MIT
# Triad Drift - import builder: bring Grok Bot's merged deliveries (ArtSource/EngAI/**) into the game. Headless:
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngImport\scripts\ue_import_engai.py
# TD_IMPORT_ONLY=Dominion,Generator,Anchor,Hauler,DrillRig,CrystalCache,OutpostDressing limits the run; TD_IMPORT_MI_ONLY=1 keeps meshes and
# textures and only rewrites the material instances and the data assets (tuning).
#
# Follows ArtSource/EngShips/Rodin/scripts/ue_import_rodin_ships.py (the squad / Null fleet import):
#   - FBX through Interchange with the existing pipeline Gameplay/Ships/Pipelines/PL_Ship_StaticMesh (UCX_ collision by
#     name, no lightmap UVs, no materials, Nanite), uniform scale 1.0 (every delivery has UnitScaleFactor 100);
#   - Grok Bot's T_<Asset>_BC|N|ORM|E at 2K (N is DirectX: no green flip; ORM linear Masks); ship maps never stream
#     (the ship builder's setting since issue #63); props stream as usual;
#   - one material instance per mesh: ships on the ship master M_TD_Ship, props on M_TD_PropTextured;
#   - sockets checked against the delivery's manifest (added when Interchange dropped one), names as delivered;
#   - every save checked: a package a running game of this checkout holds (Windows error 32) fails the run.
# Wiring (data only; the old meshes stay where they are, see docs/eng/ART-HISTORY.md):
#   Dominion  -> Characters/Dominion/DA_Dominion_Visuals: HullMesh = SM_DominionMothership; BridgeMesh and TrenchMesh
#                cleared (the r2 hull carries its own bridge and trench walls; leaving them would draw the old pieces
#                inside the new hull).
#   Generator -> DA_Dominion_Visuals: GeneratorMesh = SM_DominionShieldGenerator, GeneratorCoreMesh = ..Core.
#   Hauler    -> Characters/NullFleet/DA_Enemy_Hauler_Visuals (new UTDEnemyVisuals: Mesh + MaterialOverrides).
#   Anchor, DrillRig: meshes only (the course code and the dig-site class place them).
# TD_IMPORT_REVERT=Dominion,Generator puts the previous meshes back in DA_Dominion_Visuals.
import json
import math
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
AI = os.path.join(REPO, "ArtSource", "EngAI")
SHIPS = "/Game/TriadDrift/Gameplay/Ships"
PROPS = "/Game/TriadDrift/Gameplay/Props"
PIPELINE = SHIPS + "/Pipelines/PL_Ship_StaticMesh"
SHIP_MASTER = SHIPS + "/Materials/M_TD_Ship"
PROP_MASTER = "/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured"
DOM_DA = "/Game/TriadDrift/Gameplay/Characters/Dominion/DA_Dominion_Visuals"
HAULER_DA = "/Game/TriadDrift/Gameplay/Characters/NullFleet/DA_Enemy_Hauler_Visuals"
OLD_DOM = "/Game/TriadDrift/Gameplay/Props/Dominion/"
ENEMY_GLOW = (1.0, 0.55, 0.35)

# item -> meshes: (asset, source folder, Unreal folder, master, never stream, material instance values)
ITEMS = {
    "Dominion": [dict(
        asset="SM_DominionMothership", src="Ships/DominionMothership/mesh_r2", dest=SHIPS + "/DominionMothership",
        master="ship", never_stream=True, detail=("Detail_N", "Detail_ORM"),
        # Seen from 8 km down to the trench at 30-80 m. The atlas UV 0..1 spans 96 m (48 m per half), so tiling 12 =
        # the brief's 8 m detail tile; the detail fades out between 60 and 260 m (no shimmer at range).
        scalars={"EmissiveStrength": 8.0, "EyeBoost": 3.0, "WhiteSharpness": 4.0, "RoughnessScale": 1.0, "SelfLight": 0.04,
                 "DetailTiling": 12.0, "DetailStrength": 0.6, "DetailAO": 0.6, "DetailRough": 0.3,
                 "DetailFadeStart": 6000.0, "DetailFadeRange": 20000.0, "RoughnessMin": 0.3, "MetallicScale": 1.0,
                 "BaseFloor": 0.0, "HeatGlow": 0.0},
        vectors={"EmissiveTint": (1.0, 1.0, 1.0), "BaseTint": (1.0, 1.0, 1.0)})],
    "Generator": [
        dict(asset="SM_DominionShieldGenerator", src="Ships/DominionShieldGenerator/mesh_r1", dest=SHIPS + "/DominionShieldGenerator",
             master="ship", never_stream=True,
             scalars={"EmissiveStrength": 8.0, "EyeBoost": 3.0, "WhiteSharpness": 4.0, "RoughnessScale": 1.0, "SelfLight": 0.04,
                      "DetailStrength": 0.0, "RoughnessMin": 0.3, "HeatGlow": 0.0},
             vectors={"EmissiveTint": (1.0, 1.0, 1.0), "BaseTint": (1.0, 1.0, 1.0)}),
        dict(asset="SM_DominionShieldGeneratorCore", src="Ships/DominionShieldGenerator/mesh_r1", dest=SHIPS + "/DominionShieldGenerator",
             master="ship", never_stream=True,
             # the weak point: the cyan core must read at a glance from the chase camera at 100-400 m
             scalars={"EmissiveStrength": 14.0, "EyeBoost": 6.0, "WhiteSharpness": 4.0, "RoughnessScale": 1.0, "SelfLight": 0.1,
                      "DetailStrength": 0.0, "HeatGlow": 0.0},
             vectors={"EmissiveTint": (1.0, 1.0, 1.0), "BaseTint": (1.0, 1.0, 1.0)})],
    "Anchor": [
        # fbx: Grok Bot's mesh and sockets with the plinth hull refitted by anchor_collision.py (his UCX_00 was a 13.8 m
        # box that buried the charge sockets and stood up to 2.2 m outside the visible octagon)
        dict(asset="SM_CinderAnchorTower", src="Props/CinderFieldAnchor/mesh_r1", dest=PROPS + "/CinderAnchor",
             master="prop", never_stream=False, scalars={"EmissiveStrength": 6.0},
             fbx=os.path.join(REPO, "ArtSource", "EngImport", "CinderAnchor", "SM_CinderAnchorTower.fbx")),
        dict(asset="SM_CinderAnchorEmitter", src="Props/CinderFieldAnchor/mesh_r1", dest=PROPS + "/CinderAnchor",
             master="prop", never_stream=False, scalars={"EmissiveStrength": 10.0})],
    "Hauler": [dict(
        asset="SM_NullHauler", src="Ships/NullHauler/mesh_r1", dest=SHIPS + "/NullHauler", master="ship", never_stream=True,
        scalars={"EmissiveStrength": 9.0, "EyeBoost": 1.6, "WhiteSharpness": 4.0, "RoughnessScale": 1.0, "SelfLight": 0.0,
                 "DetailStrength": 0.0, "HeatGlow": 0.0},
        vectors={"EmissiveTint": ENEMY_GLOW, "BaseTint": (1.0, 1.0, 1.0)})],
    "DrillRig": [dict(
        asset="SM_CinderDrillRig", src="Props/CinderDigSite/mesh_r1", dest=PROPS + "/CinderDigSite", master="prop",
        never_stream=False, scalars={"EmissiveStrength": 3.0},
        # no UCX in the delivery: one box so the pilot can't walk through the rig beside the cabin
        add_box=True)],
    "CrystalCache": [
        dict(asset="SM_CrystalCacheCradle", src="Props/CrystalCache/mesh_r1", dest=PROPS + "/CrystalCache", master="prop",
             never_stream=False, scalars={"EmissiveStrength": 4.0}),
        # r2 (#80): the denser hero cluster, 14 cyan + 5 amber crystals, 15 UCX; r1 is mesh_r1 (see ART-HISTORY)
        dict(asset="SM_CrystalCacheCrystal", src="Props/CrystalCache/mesh_r2", dest=PROPS + "/CrystalCache", master="prop",
             never_stream=False, scalars={"EmissiveStrength": 8.0})],
    # #79 (#40): ten small set-dressing props on one shared 1K trim (T_OutpostProps_*, one MI_OutpostProps). Import only:
    # the environment builder places them around the outposts and pads.
    "OutpostDressing": [dict(asset="SM_Outpost_" + n, src="Props/OutpostSetDressing/mesh_r1", dest=PROPS + "/OutpostDressing",
                             master="prop", never_stream=False, tex_prefix="OutpostProps", mi_name="OutpostProps",
                             scalars={"EmissiveStrength": 4.0})
                        for n in ("CargoCrate", "SupplyCrate", "FieldTerminal", "DataKiosk", "CommAntenna", "CommsDish",
                                  "Floodlight", "BollardLight", "Generator", "PowerJunction")],
}
TEX_DONE = {}      # (dest, prefix) -> textures, so a shared trim is imported once per run

eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
SAVE_FAILURES = []


def log(msg):
    unreal.log("TDPY: " + msg)


def save_loaded(asset):
    ok = eas.save_loaded_asset(asset, False)
    if not ok:
        SAVE_FAILURES.append(asset.get_path_name() if asset else "None")
    return ok


def _set(obj, prop, value, notes):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception as error:
        notes.append("could not set %s.%s (%s)" % (type(obj).__name__, prop, error))
        return False


def ensure_dir(path):
    if not eas.does_directory_exist(path):
        eas.make_directory(path)


def import_textures(files, dest, notes):
    tasks = []
    for p in files.values():
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", p)
        t.set_editor_property("destination_path", dest)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    if tasks:
        tools.import_asset_tasks(tasks)
    out = {}
    tcs = unreal.TextureCompressionSettings
    for suffix, p in files.items():
        name = os.path.splitext(os.path.basename(p))[0]
        tex = eas.load_asset("%s/%s" % (dest, name))
        if tex is None:
            notes.append("texture not imported: " + name)
            continue
        if suffix in ("N", "Detail_N"):
            _set(tex, "compression_settings", tcs.TC_NORMALMAP, notes)
            _set(tex, "srgb", False, notes)
            _set(tex, "flip_green_channel", False, notes)      # Grok Bot's maps are already DirectX
        elif suffix in ("ORM", "Detail_ORM"):
            _set(tex, "compression_settings", tcs.TC_MASKS, notes)
            _set(tex, "srgb", False, notes)
        else:
            _set(tex, "compression_settings", tcs.TC_DEFAULT, notes)
            _set(tex, "srgb", True, notes)
        _set(tex, "max_texture_size", 2048, notes)
        out[suffix] = tex
    return out


def import_mesh(fbx, dest, notes):
    try:
        params = unreal.ImportAssetParameters()
        params.is_automated = True
        params.replace_existing = True
        params.override_pipelines.append(unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1])))
        mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
        mgr.import_asset(dest, unreal.InterchangeManager.create_source_data(fbx), params)
        return "interchange"
    except Exception as error:
        notes.append("Interchange import failed (%s)" % error)
        return "failed"


def manifest_sockets(folder, asset):
    """[(name, (x, y, z) cm)] from whichever manifest the delivery has."""
    out = []
    for name in (asset + ".manifest.json", "manifest.json"):
        p = os.path.join(folder, name)
        if not os.path.isfile(p):
            continue
        with open(p, encoding="utf-8") as fh:
            m = json.load(fh)
        if isinstance(m.get("ue_sockets"), list):
            out = [(s["name"], tuple(s["ue_cm"])) for s in m["ue_sockets"]]
        elif isinstance(m.get("sockets"), dict):                      # the Dominion hull: position and yaw
            out = [(k, tuple(v["ue_cm"]), v.get("ue_yaw_deg")) for k, v in m["sockets"].items()]
        elif asset == "SM_CinderAnchorTower" and "tower" in m:
            out = [(s["name"], tuple(s["pos_cm"])) for s in m["tower"]["sockets"]]
        elif asset == "SM_CrystalCacheCradle" and "cradle" in m:
            out = [(s["name"], tuple(s["pos_cm"])) for s in m["cradle"]["sockets"]]
        elif asset == "SM_DominionShieldGenerator" and "socket_core" in m:
            out = [("SOCKET_Core", tuple(m["socket_core"]["ue_cm"]))]
        if out:
            return out
    return out


def check_sockets(mesh, wanted, notes):
    """Each delivered socket must exist where the manifest says (within 2 cm). Interchange may keep or strip the
    SOCKET_ prefix; the name it gave is kept (gameplay is told the exact names)."""
    report = []
    for entry in wanted:
        name, pos = entry[0], entry[1]
        want_yaw = entry[2] if len(entry) > 2 else None
        bare = name[7:] if name.startswith("SOCKET_") else name
        sock = mesh.find_socket(name) or mesh.find_socket(bare)
        if sock is None:
            sock = unreal.StaticMeshSocket(mesh)
            sock.set_editor_property("socket_name", name)
            sock.set_editor_property("relative_location", unreal.Vector(*pos))
            mesh.add_socket(sock)
            notes.append("socket %s added from the manifest" % name)
        loc = sock.get_editor_property("relative_location")
        err = ((loc.x - pos[0]) ** 2 + (loc.y - pos[1]) ** 2 + (loc.z - pos[2]) ** 2) ** 0.5
        if err > 2.0:
            # Kept as imported: the socket travels with the geometry in the FBX. Seen Nov 13 on the anchor tower: its
            # manifest lists the charge ports in Blender's Y (mirrored), the FBX puts ports and sockets at -Y together.
            notes.append("socket %s at (%.0f,%.0f,%.0f) differs from the manifest (%.0f,%.0f,%.0f): kept as imported" % (
                sock.get_editor_property("socket_name"), loc.x, loc.y, loc.z, pos[0], pos[1], pos[2]))
        # Upright: the FBX empties come through Interchange with the Blender-to-Unreal axis turn left in their rotation
        # (found by T4 on the r2 hull: +X right, but Z along the ring tangent, a 90 degree roll). Every delivered
        # socket is specified as a position and a heading with Z up, so keep the heading (the manifest's yaw when it
        # gives one, else where the imported +X points in the horizontal plane) and set pitch and roll to zero.
        rot = sock.get_editor_property("relative_rotation")
        fwd = unreal.MathLibrary.get_forward_vector(rot)
        up = unreal.MathLibrary.get_up_vector(rot)
        if want_yaw is not None:
            yaw = ((float(want_yaw) + 180.0) % 360.0) - 180.0
        elif abs(fwd.z) < 0.9:
            yaw = math.degrees(math.atan2(fwd.y, fwd.x))
        else:
            yaw = None
            notes.append("socket %s: +X is vertical, rotation left as imported" % name)
        if yaw is not None:
            if up.z < 0.99985 or abs((((rot.yaw - yaw) + 180.0) % 360.0) - 180.0) > 0.5:
                notes.append("socket %s: rotation (p %.0f y %.0f r %.0f, up %.2f %.2f %.2f) set upright, yaw %.1f" % (
                    sock.get_editor_property("socket_name"), rot.pitch, rot.yaw, rot.roll, up.x, up.y, up.z, yaw))
            sock.set_editor_property("relative_rotation", unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
            rot = sock.get_editor_property("relative_rotation")
        up = unreal.MathLibrary.get_up_vector(rot)
        tilt = math.degrees(math.acos(max(-1.0, min(1.0, up.z))))
        if tilt > 1.0:
            notes.append("socket %s: up is %.1f degrees off the mesh's up" % (name, tilt))
        report.append("%s(%.0f,%.0f,%.0f yaw %.0f up-tilt %.1f)" % (sock.get_editor_property("socket_name"), loc.x, loc.y, loc.z, rot.yaw, tilt))
    return report


def build_mi(cfg, tex, notes):
    dest = cfg["dest"]
    short = cfg.get("mi_name", cfg["asset"][3:])
    master = eas.load_asset(SHIP_MASTER if cfg["master"] == "ship" else PROP_MASTER)
    if master is None:
        notes.append("master missing")
        return None
    mi_path = "%s/MI_%s" % (dest, short)
    mi = eas.load_asset(mi_path) if eas.does_asset_exist(mi_path) else tools.create_asset(
        "MI_" + short, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, master)
    params = [("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N"), ("EmissiveTex", "E")]
    if cfg["master"] == "ship":
        params += [("DetailNormalTex", "Detail_N"), ("DetailRMTex", "Detail_ORM")]
    for pname, suffix in params:
        if suffix in tex:
            mel.set_material_instance_texture_parameter_value(mi, pname, tex[suffix])
    for pname, value in cfg.get("scalars", {}).items():
        if pname == "EmissiveStrength" and "E" not in tex:
            value = 0.0
        mel.set_material_instance_scalar_parameter_value(mi, pname, value)
    for pname, rgb in cfg.get("vectors", {}).items():
        mel.set_material_instance_vector_parameter_value(mi, pname, unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    save_loaded(mi)
    return mi


def import_one(cfg, mi_only):
    notes = []
    folder = os.path.join(AI, *cfg["src"].split("/"))
    asset, dest, short = cfg["asset"], cfg["dest"], cfg["asset"][3:]
    fbx = cfg.get("fbx") or os.path.join(folder, asset + ".fbx")
    if not os.path.isfile(fbx):
        log("FAILED %s: no FBX at %s" % (asset, fbx))
        return None
    ensure_dir(dest)
    files = {}
    prefix = cfg.get("tex_prefix", short)
    for suffix in ("BC", "N", "ORM", "E") + tuple(cfg.get("detail", ())):
        p = os.path.join(folder, "T_%s_%s.png" % (prefix, suffix))
        if os.path.isfile(p):
            files[suffix] = p
    if (dest, prefix) in TEX_DONE:
        tex = TEX_DONE[(dest, prefix)]
    elif mi_only:
        tex = {}
        for suffix, p in files.items():
            t = eas.load_asset("%s/%s" % (dest, os.path.splitext(os.path.basename(p))[0]))
            if t is not None:
                tex[suffix] = t
    else:
        tex = import_textures(files, dest, notes)
    TEX_DONE[(dest, prefix)] = tex
    for suffix, t in tex.items():
        if cfg.get("never_stream"):
            _set(t, "never_stream", True, notes)
            _set(t, "lod_bias", 0, notes)
        save_loaded(t)
    how = "kept" if mi_only else import_mesh(fbx, dest, notes)
    mesh = eas.load_asset("%s/%s" % (dest, asset))
    if mesh is None:
        log("FAILED %s: mesh not found after import (%s) %s" % (asset, how, "; ".join(notes)))
        return None
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    try:
        sme.set_generate_lightmap_uv(mesh, False)
    except Exception as error:
        notes.append("lightmap toggle: %s" % error)
    try:
        ns = mesh.get_editor_property("nanite_settings")
        ns.enabled = True
        mesh.set_editor_property("nanite_settings", ns)
    except Exception as error:
        notes.append("nanite: %s" % error)
    hulls = sme.get_convex_collision_count(mesh)
    boxes = sme.get_simple_collision_count(mesh) if hasattr(sme, "get_simple_collision_count") else -1
    if cfg.get("add_box") and hulls == 0 and boxes <= 0:
        try:
            sme.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.BOX)
            notes.append("box collision added")
        except Exception as error:
            notes.append("box collision: %s" % error)
        boxes = sme.get_simple_collision_count(mesh) if hasattr(sme, "get_simple_collision_count") else -1
    sockets = check_sockets(mesh, manifest_sockets(folder, asset), notes)
    mi = build_mi(cfg, tex, notes)
    slots = []
    for i, sm in enumerate(mesh.static_materials):
        if mi is not None:
            mesh.set_material(i, mi)
        slots.append(str(sm.get_editor_property("material_slot_name")))
    save_loaded(mesh)
    tris = -1
    try:
        tris = int(mesh.get_num_triangles(0))
    except Exception:
        pass
    b = mesh.get_bounding_box()
    row = {"asset": "%s/%s" % (dest, asset), "importer": how, "tris": tris,
           "bounds_min_cm": [round(b.min.x, 1), round(b.min.y, 1), round(b.min.z, 1)],
           "bounds_max_cm": [round(b.max.x, 1), round(b.max.y, 1), round(b.max.z, 1)],
           "convex_hulls": hulls, "simple_shapes": boxes, "slots": slots, "mi": mi.get_path_name() if mi else None,
           "textures": {k: v.get_path_name() for k, v in tex.items()}, "sockets": sockets, "notes": notes}
    log("%s: tris %d, bounds %s..%s cm, hulls %d, shapes %s, slots %s, sockets %s%s" % (
        asset, tris, row["bounds_min_cm"], row["bounds_max_cm"], hulls, boxes, slots, sockets,
        ("  NOTES: " + "; ".join(notes)) if notes else ""))
    return mesh, mi, row


def soft_name(obj):
    return str(obj.get_path_name()) if obj else "None"


def wire_dominion(fields, rows, revert):
    da = eas.load_asset(DOM_DA)
    if da is None:
        log("FAILED: %s missing" % DOM_DA)
        return False
    for name in ("SM_Dominion_Hull", "SM_Dominion_Bridge", "SM_Dominion_TrenchWall", "SM_Dominion_Generator",
                 "SM_Dominion_GeneratorCore", "SM_Dominion_ShieldEmitter", "SM_Dominion_Reactor"):
        eas.load_asset(OLD_DOM + name)      # resolve the soft pointers so the log names what was there
    before = {f: soft_name(da.get_editor_property(f)) for f in ("hull_mesh", "generator_mesh", "generator_core_mesh", "bridge_mesh",
                                                              "trench_mesh", "shield_emitter_mesh", "reactor_mesh")}
    notes = []
    for prop, value in fields.items():
        _set(da, prop, value, notes)
    save_loaded(da)
    after = {f: soft_name(da.get_editor_property(f)) for f in before}
    log("DA_Dominion_Visuals%s: before %s" % (" (REVERT)" if revert else "", json.dumps(before)))
    log("DA_Dominion_Visuals: after %s %s" % (json.dumps(after), notes))
    rows.append({"asset": DOM_DA, "before": before, "after": after, "notes": notes})
    return not notes


def wire_hauler(mesh, mi, rows):
    notes = []
    if eas.does_asset_exist(HAULER_DA):
        da = eas.load_asset(HAULER_DA)
    else:
        folder, name = HAULER_DA.rsplit("/", 1)
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.TDEnemyVisuals)
        da = tools.create_asset(name, folder, unreal.TDEnemyVisuals, factory)
    _set(da, "mesh", mesh, notes)
    _set(da, "material_overrides", [mi] if mi else [], notes)
    save_loaded(da)
    log("DA_Enemy_Hauler_Visuals: Mesh %s, MaterialOverrides %s %s" % (soft_name(da.get_editor_property("mesh")),
                                                                      [soft_name(m) for m in da.get_editor_property("material_overrides")], notes))
    rows.append({"asset": HAULER_DA, "mesh": soft_name(mesh), "notes": notes})
    return not notes


def main():
    only = [s.strip() for s in os.environ.get("TD_IMPORT_ONLY", "").split(",") if s.strip()]
    revert = [s.strip() for s in os.environ.get("TD_IMPORT_REVERT", "").split(",") if s.strip()]
    mi_only = os.environ.get("TD_IMPORT_MI_ONLY", "") == "1"
    if not eas.does_asset_exist(PIPELINE):
        log("FAILED: pipeline %s missing (run ue_import_rodin_ships.py once)" % PIPELINE)
        return
    rows, bad = [], 0
    if os.environ.get("TD_IMPORT_SOCKETS_ONLY", "") == "1":
        # Re-check and correct the sockets of meshes that are already imported: nothing is re-imported, no material,
        # texture or data asset is touched, only the static meshes with sockets are saved.
        for item, meshes in ITEMS.items():
            if only and item not in only:
                continue
            for cfg in meshes:
                wanted = manifest_sockets(os.path.join(AI, *cfg["src"].split("/")), cfg["asset"])
                mesh = eas.load_asset("%s/%s" % (cfg["dest"], cfg["asset"]))
                if not wanted or mesh is None:
                    continue
                notes = []
                report = check_sockets(mesh, wanted, notes)
                save_loaded(mesh)
                log("%s sockets: %s%s" % (cfg["asset"], report, ("  NOTES: " + "; ".join(notes)) if notes else ""))
                bad += sum(1 for n in notes if "degrees off" in n)
        log("OK" if not bad and not SAVE_FAILURES else "FAILED: sockets")
        return
    if revert:
        fields = {}
        if "Dominion" in revert:
            fields.update({"hull_mesh": eas.load_asset(OLD_DOM + "SM_Dominion_Hull"),
                           "bridge_mesh": eas.load_asset(OLD_DOM + "SM_Dominion_Bridge"),
                           "trench_mesh": eas.load_asset(OLD_DOM + "SM_Dominion_TrenchWall")})
        if "Generator" in revert:
            fields.update({"generator_mesh": eas.load_asset(OLD_DOM + "SM_Dominion_Generator"),
                           "generator_core_mesh": eas.load_asset(OLD_DOM + "SM_Dominion_GeneratorCore")})
        ok = wire_dominion(fields, rows, True)
        log("OK" if ok and not SAVE_FAILURES else "FAILED: revert")
        return
    got = {}
    for item, meshes in ITEMS.items():
        if only and item not in only:
            continue
        for cfg in meshes:
            r = import_one(cfg, mi_only)
            if r is None:
                bad += 1
                continue
            got[cfg["asset"]] = r
            rows.append(r[2])
    dom = {}
    if "SM_DominionMothership" in got:
        dom.update({"hull_mesh": got["SM_DominionMothership"][0], "bridge_mesh": None, "trench_mesh": None})
    if "SM_DominionShieldGenerator" in got and "SM_DominionShieldGeneratorCore" in got:
        dom.update({"generator_mesh": got["SM_DominionShieldGenerator"][0],
                    "generator_core_mesh": got["SM_DominionShieldGeneratorCore"][0]})
    if dom and not wire_dominion(dom, rows, False):
        bad += 1
    if "SM_NullHauler" in got and not wire_hauler(got["SM_NullHauler"][0], got["SM_NullHauler"][1], rows):
        bad += 1
    out = os.path.join(REPO, "Saved", "EngImport", "import_report.json")
    try:
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, "w", encoding="utf-8") as fh:
            json.dump(rows, fh, indent=2, default=str)
    except Exception as error:
        log("note: could not write report (%s)" % error)
    if SAVE_FAILURES:
        log("FAILED: %d asset(s) could not be saved (is a game of this checkout running?): %s" % (len(SAVE_FAILURES), SAVE_FAILURES))
        return
    log("OK" if not bad else "FAILED: %d item(s), see the lines above" % bad)


main()
