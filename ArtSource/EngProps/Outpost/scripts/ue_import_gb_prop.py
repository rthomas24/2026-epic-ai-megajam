# SPDX-License-Identifier: MIT
# Triad Drift - shared Unreal import for a Grok Bot single-mesh prop (the relay and the wreck use it).
# Not run directly: a small per-prop script sets CONFIG and calls run(CONFIG). Headless through Tools\td-python.ps1.
#
# CONFIG keys:
#   dest          /Game/... folder (everything is written there and nowhere else)
#   name          asset stem: SM_<name>, M_<name>, MI_<name>..., T_<name>_*, BP_<name>_*
#   fbx, manifest absolute paths of the exported FBX and its manifest
#   textures      {suffix: (file, "color" | "normal" | "masks")}; BC, N, ORM required, E optional
#   power_param   name of the scalar that scales the emissive (e.g. RelayPower); None if the prop has no emissive
#   instances     {MI name: {"scalars": {...}, "vectors": {...}}}; the first is assigned to the mesh
#   actors        {BP name: MI name}: an actor class with the mesh and that instance (for TD.Dev.SpawnClass and for sites)
#   report        absolute path of the JSON report
# Master material M_<name>: BaseColorTex x Tint, NormalTex (DirectX), ORMTex (R AO, G roughness x RoughnessScale,
# B metallic), and when there is an E map: emissive = EmissiveTex x GlowColor x GlowStrength x <power_param>.
import json
import os

import unreal

eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log("TDPY: " + msg)


def _set(obj, prop, value, notes):
    try:
        obj.set_editor_property(prop, value)
    except Exception as error:
        notes.append("could not set %s.%s (%s)" % (type(obj).__name__, prop, error))


def ensure_dir(path):
    if not eas.does_directory_exist(path):
        eas.make_directory(path)


def import_textures(cfg, notes):
    dest = cfg["dest"] + "/Textures"
    ensure_dir(dest)
    tasks = []
    for suffix, (path, _kind) in cfg["textures"].items():
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", path)
        t.set_editor_property("destination_path", dest)
        t.set_editor_property("destination_name", "T_%s_%s" % (cfg["name"], suffix))
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    out = {}
    for suffix, (_path, kind) in cfg["textures"].items():
        tex = eas.load_asset("%s/T_%s_%s" % (dest, cfg["name"], suffix))
        if tex is None:
            notes.append("texture not imported: " + suffix)
            continue
        if kind == "normal":
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP, notes)
            _set(tex, "srgb", False, notes)
            _set(tex, "flip_green_channel", False, notes)
        elif kind == "masks":
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_MASKS, notes)
            _set(tex, "srgb", False, notes)
        else:
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT, notes)
            _set(tex, "srgb", True, notes)
        eas.save_loaded_asset(tex)
        out[suffix] = tex
    return out


def build_master(cfg, tex):
    folder = cfg["dest"] + "/Materials"
    ensure_dir(folder)
    name = "M_" + cfg["name"]
    path = folder + "/" + name
    mat = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    ok = [True]

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def link(a, out_name, b, in_name):
        ok[0] &= bool(mel.connect_material_expressions(a, out_name, b, in_name))

    def tex_param(pname, texture, sampler, y):
        n = node(unreal.MaterialExpressionTextureSampleParameter2D, -1200, y)
        n.set_editor_property("parameter_name", pname)
        n.set_editor_property("texture", texture)
        n.set_editor_property("sampler_type", sampler)
        return n

    def scalar(pname, default, x, y):
        n = node(unreal.MaterialExpressionScalarParameter, x, y)
        n.set_editor_property("parameter_name", pname)
        n.set_editor_property("default_value", default)
        return n

    def vector(pname, rgb, x, y):
        n = node(unreal.MaterialExpressionVectorParameter, x, y)
        n.set_editor_property("parameter_name", pname)
        n.set_editor_property("default_value", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        return n

    def mul(a, a_out, b, b_out, x, y):
        n = node(unreal.MaterialExpressionMultiply, x, y)
        link(a, a_out, n, "A"); link(b, b_out, n, "B")
        return n

    S = unreal.MaterialSamplerType
    P = unreal.MaterialProperty
    bc = tex_param("BaseColorTex", tex["BC"], S.SAMPLERTYPE_COLOR, -500)
    nrm = tex_param("NormalTex", tex["N"], S.SAMPLERTYPE_NORMAL, -200)
    orm = tex_param("ORMTex", tex["ORM"], S.SAMPLERTYPE_MASKS, 100)
    albedo = mul(bc, "RGB", vector("Tint", (1, 1, 1), -900, -650), "", -600, -550)
    rough = node(unreal.MaterialExpressionSaturate, -400, 100)
    link(mul(orm, "G", scalar("RoughnessScale", 1.0, -900, 250), "", -650, 150), "", rough, "")
    pairs = [(albedo, "", P.MP_BASE_COLOR), (rough, "", P.MP_ROUGHNESS), (orm, "B", P.MP_METALLIC), (orm, "R", P.MP_AMBIENT_OCCLUSION), (nrm, "RGB", P.MP_NORMAL)]
    if "E" in tex and cfg.get("power_param"):
        emi = tex_param("EmissiveTex", tex["E"], S.SAMPLERTYPE_COLOR, 450)
        g1 = mul(emi, "RGB", vector("GlowColor", (1, 1, 1), -900, 600), "", -650, 500)
        g2 = mul(g1, "", scalar("GlowStrength", 10.0, -650, 650), "", -450, 520)
        g3 = mul(g2, "", scalar(cfg["power_param"], 1.0, -450, 700), "", -250, 540)
        pairs.append((g3, "", P.MP_EMISSIVE_COLOR))
    for n, out_name, prop in pairs:
        ok[0] &= bool(mel.connect_material_property(n, out_name, prop))
    if not ok[0]:
        log("FAILED: could not wire " + path)
        return None
    try:
        mel.recompile_material(mat)
    except Exception:
        log("note: no renderer in this headless run; the master compiles on first load")
    eas.save_asset(path, only_if_is_dirty=False)
    return mat


def ensure_pipeline(cfg, notes):
    folder = cfg["dest"] + "/Pipelines"
    ensure_dir(folder)
    path = "%s/PL_%s_StaticMesh" % (folder, cfg["name"])
    if not eas.does_asset_exist(path):
        eas.duplicate_asset("/Interchange/Pipelines/DefaultAssetsPipeline", path)
    pipe = eas.load_asset(path)
    _set(pipe, "asset_type_sub_folders", False, notes)
    mesh = pipe.get_editor_property("mesh_pipeline")
    for k, v in {"import_static_meshes": True, "import_skeletal_meshes": False,
                 "collision": True, "import_collision_according_to_mesh_name": True, "one_convex_hull_per_ucx": True,
                 "generate_lightmap_u_vs": False, "build_nanite": False}.items():
        _set(mesh, k, v, notes)
    try:
        sp = pipe.get_editor_property("material_pipeline")
        _set(sp, "import_materials", False, notes)
        try:
            _set(sp.get_editor_property("texture_pipeline"), "import_textures", False, notes)
        except Exception:
            pass
    except Exception as error:
        notes.append("pipeline material_pipeline: %s" % error)
    eas.save_loaded_asset(pipe)
    return unreal.SoftObjectPath("%s.%s" % (path, path.rsplit("/", 1)[-1]))


def make_actor(cfg, bp_name, mesh, mi):
    path = "%s/%s" % (cfg["dest"], bp_name)
    if eas.does_asset_exist(path):
        eas.delete_asset(path)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.Actor)
    bp = tools.create_asset(bp_name, cfg["dest"], unreal.Blueprint, factory)
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    lib = unreal.SubobjectDataBlueprintFunctionLibrary
    handles = sds.k2_gather_subobject_data_for_blueprint(bp)
    params = unreal.AddNewSubobjectParams(parent_handle=handles[0], new_class=unreal.StaticMeshComponent, blueprint_context=bp)
    handle, fail = sds.add_new_subobject(params)
    if not lib.is_handle_valid(handle):
        log("FAILED: add component to %s (%s)" % (bp_name, fail))
        return False
    sds.rename_subobject(handle, unreal.Text("Mesh"))
    comp = lib.get_object_for_blueprint(lib.get_data(handle), bp)
    comp.set_static_mesh(mesh)
    comp.set_material(0, mi)
    comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eas.save_asset(path, only_if_is_dirty=False)
    return True


def run(cfg):
    notes = []
    ensure_dir(cfg["dest"])
    tex = import_textures(cfg, notes)
    if len(tex) != len(cfg["textures"]):
        log("FAILED: textures %s %s" % (sorted(tex), notes))
        return
    master = build_master(cfg, tex)
    if master is None:
        return
    mis = {}
    for mi_name, values in cfg["instances"].items():
        path = "%s/%s" % (cfg["dest"], mi_name)
        mi = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(
            mi_name, cfg["dest"], unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, master)
        for k, v in values.get("scalars", {}).items():
            mel.set_material_instance_scalar_parameter_value(mi, k, v)
        for k, v in values.get("vectors", {}).items():
            mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
        eas.save_loaded_asset(mi)
        mis[mi_name] = mi
    pipe = ensure_pipeline(cfg, notes)
    with open(cfg["manifest"], encoding="utf-8") as fh:
        man = json.load(fh)
    asset = man["asset"]
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    params.override_pipelines.append(pipe)
    unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(
        cfg["dest"], unreal.InterchangeManager.create_source_data(cfg["fbx"]), params)
    mesh = eas.load_asset("%s/%s" % (cfg["dest"], asset))
    if mesh is None:
        log("FAILED %s: not found after import" % asset)
        return
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    try:
        sme.set_generate_lightmap_uv(mesh, False)
        ns = mesh.get_editor_property("nanite_settings")
        ns.enabled = False
        mesh.set_editor_property("nanite_settings", ns)
    except Exception as error:
        notes.append("mesh settings: %s" % error)
    first = mis[next(iter(cfg["instances"]))]
    for i in range(len(mesh.static_materials)):
        mesh.set_material(i, first)
    eas.save_loaded_asset(mesh)
    bb = mesh.get_bounding_box()
    mn = [round(bb.min.x, 1), round(bb.min.y, 1), round(bb.min.z, 1)]
    mx = [round(bb.max.x, 1), round(bb.max.y, 1), round(bb.max.z, 1)]
    size = [round(mx[i] - mn[i], 1) for i in range(3)]
    tris = int(mesh.get_num_triangles(0))
    hulls = int(sme.get_convex_collision_count(mesh))
    ok = all(abs(size[i] - man["size_cm"][i]) < 1.0 for i in range(3)) and tris == man["tris"] and hulls == len(man.get("collision", []))
    for bp_name, mi_name in cfg.get("actors", {}).items():
        ok = make_actor(cfg, bp_name, mesh, mis[mi_name]) and ok
    content = os.path.join(unreal.Paths.project_content_dir(), cfg["dest"].replace("/Game/", ""))
    total = 0
    for base, _dirs, files in os.walk(content):
        total += sum(os.path.getsize(os.path.join(base, f)) for f in files if f.endswith(".uasset"))
    row = {"asset": asset, "ok": ok, "min_cm": mn, "max_cm": mx, "size_cm": size, "tris": tris, "hulls": hulls,
           "instances": list(mis), "actors": list(cfg.get("actors", {})), "uasset_bytes": total, "notes": notes}
    with open(cfg["report"], "w", encoding="utf-8") as fh:
        json.dump(row, fh, indent=1)
    log(json.dumps(row))
    log("OK" if ok else "FAILED: see the line above")
