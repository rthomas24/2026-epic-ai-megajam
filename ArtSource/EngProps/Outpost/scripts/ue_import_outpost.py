# SPDX-License-Identifier: MIT
# Triad Drift - import the survey outpost (Grok Bot's research outpost and console, made walkable by op_build.py).
# Headless, from the repo root:
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Outpost\scripts\ue_import_outpost.py
#
# Writes ONLY under /Game/TriadDrift/Gameplay/Props/Outpost:
#   Textures/T_Outpost_Cabin_BC, _N (2K, Grok Bot's maps as delivered), _ORM, _M, _E (1K, ArtSource/EngProps/Outpost/textures)
#   Textures/T_Outpost_Console_BC (2K, as delivered: the screens are about 130 px wide in it), _N, _ORM, _E, _M (1K)
#   Materials/M_Outpost                 the one master (parameters below)
#   MI_Outpost_Cabin_Verdance, MI_Outpost_Cabin_Cinder, MI_Outpost_Console_Verdance, MI_Outpost_Console_Cinder
#   Pipelines/PL_Outpost_StaticMesh     Interchange pipeline (UCX collision by name, no Nanite, no lightmap UVs)
#   SM_Outpost_Cabin, SM_Outpost_Console   (slot 0 = the Verdance instance)
# Reads Grok Bot's folder ArtSource/EngAI/Props/VerdanceOutpost/mesh_r1 (or $OUTPOST_SRC); modifies nothing outside
# its own folder. Idempotent. Prints TDPY: OK only if both meshes verify (bounds, triangles, hulls, slot).
#
# M_Outpost parameters:
#   BaseColorTex, NormalTex (DirectX), ORMTex (R AO, G roughness, B metallic), EmissiveTex, MaskTex (R accent paint,
#   G light panels)
#   Tint (colour, x base colour), Desaturate (0..1), PanelTint (colour, x base colour where MaskTex.G)
#   AccentColor, AccentAmount (0 = paint as delivered, 1 = repaint MaskTex.R with AccentColor)
#   SootAmount, SootColor, SootBase, SootHeight (cm above the pivot: full soot below SootBase, none above Base + Height)
#   RoughnessScale, MetallicScale
#   ScreenColor, ScreenPower (emissive = EmissiveTex x ScreenColor x ScreenPower; the game brightens ScreenPower)
import glob
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
SRC = os.environ.get("OUTPOST_SRC") or os.path.join(REPO, "ArtSource", "EngAI", "Props", "VerdanceOutpost", "mesh_r1")
DEST = "/Game/TriadDrift/Gameplay/Props/Outpost"
TEX_DEST = DEST + "/Textures"
MASTER = DEST + "/Materials/M_Outpost"
PIPELINE = DEST + "/Pipelines/PL_Outpost_StaticMesh"

TEXTURES = {    # asset name: (file, kind)
    "T_Outpost_Cabin_BC": (os.path.join(SRC, "T_VerdanceOutpost_BC.png"), "color"),
    "T_Outpost_Cabin_N": (os.path.join(SRC, "T_VerdanceOutpost_N.png"), "normal"),
    "T_Outpost_Cabin_ORM": (os.path.join(KIT, "textures", "T_Outpost_Cabin_ORM.png"), "masks"),
    "T_Outpost_Cabin_E": (os.path.join(KIT, "textures", "T_Outpost_Cabin_E.png"), "color"),
    "T_Outpost_Cabin_M": (os.path.join(KIT, "textures", "T_Outpost_Cabin_M.png"), "masks"),
    "T_Outpost_Console_BC": (os.path.join(SRC, "T_ResearchConsole_BC.png"), "color"),
    "T_Outpost_Console_N": (os.path.join(KIT, "textures", "T_Outpost_Console_N.png"), "normal"),
    "T_Outpost_Console_ORM": (os.path.join(KIT, "textures", "T_Outpost_Console_ORM.png"), "masks"),
    "T_Outpost_Console_E": (os.path.join(KIT, "textures", "T_Outpost_Console_E.png"), "color"),
    "T_Outpost_Console_M": (os.path.join(KIT, "textures", "T_Outpost_Console_M.png"), "masks"),
}
AMBER = (1.0, 0.42, 0.04)
VARIANTS = {    # instance: (texture set, scalars, vectors)
    "MI_Outpost_Cabin_Verdance": ("Cabin", {"AccentAmount": 0.0, "Desaturate": 0.0, "SootAmount": 0.0, "RoughnessScale": 1.0, "ScreenPower": 12.0},
                                  {"Tint": (1, 1, 1), "PanelTint": (1.15, 1.15, 1.15), "AccentColor": AMBER, "ScreenColor": (0.45, 1.0, 1.0)}),
    "MI_Outpost_Cabin_Cinder": ("Cabin", {"AccentAmount": 1.0, "Desaturate": 0.3, "SootAmount": 0.75, "SootBase": 120.0, "SootHeight": 380.0,
                                          "RoughnessScale": 1.2, "ScreenPower": 12.0},
                                {"Tint": (0.86, 0.78, 0.70), "PanelTint": (0.60, 0.54, 0.48), "AccentColor": AMBER, "SootColor": (0.20, 0.17, 0.15),
                                 "ScreenColor": (1.0, 0.5, 0.12)}),
    "MI_Outpost_Console_Verdance": ("Console", {"AccentAmount": 0.0, "Desaturate": 0.0, "SootAmount": 0.0, "RoughnessScale": 1.0, "ScreenPower": 1.6},
                                    {"Tint": (1, 1, 1), "PanelTint": (1, 1, 1), "AccentColor": AMBER, "ScreenColor": (1, 1, 1)}),
    "MI_Outpost_Console_Cinder": ("Console", {"AccentAmount": 1.0, "Desaturate": 0.3, "SootAmount": 0.5, "SootBase": 20.0, "SootHeight": 120.0,
                                              "RoughnessScale": 1.2, "ScreenPower": 1.6},
                                  {"Tint": (0.86, 0.78, 0.70), "PanelTint": (0.60, 0.54, 0.48), "AccentColor": AMBER, "SootColor": (0.20, 0.17, 0.15),
                                   "ScreenColor": (1, 1, 1)}),
}
MESHES = {"SM_Outpost_Cabin": "MI_Outpost_Cabin_Verdance", "SM_Outpost_Console": "MI_Outpost_Console_Verdance"}

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


def import_textures(notes):
    out = {}
    tasks = []
    for name, (path, _kind) in TEXTURES.items():
        if not os.path.isfile(path):
            notes.append("texture file missing: " + path)
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", path)
        t.set_editor_property("destination_path", TEX_DEST)
        t.set_editor_property("destination_name", name)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    for name, (_path, kind) in TEXTURES.items():
        tex = eas.load_asset("%s/%s" % (TEX_DEST, name))
        if tex is None:
            notes.append("texture not imported: " + name)
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
        out[name] = tex
    return out


def build_master(tex):
    """One master for the cabin and the console. Rebuilt on every run (expressions deleted first)."""
    ensure_dir(DEST + "/Materials")
    folder, name = MASTER.rsplit("/", 1)
    mat = eas.load_asset(MASTER) if eas.does_asset_exist(MASTER) else tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    ok = [True]

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def link(a, out_name, b, in_name):
        ok[0] &= bool(mel.connect_material_expressions(a, out_name, b, in_name))

    def tex_param(pname, texture, sampler, y):
        n = node(unreal.MaterialExpressionTextureSampleParameter2D, -1700, y)
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

    def op(cls, a, a_out, b, b_out, x, y, names=("A", "B")):
        n = node(cls, x, y)
        link(a, a_out, n, names[0])
        if b is not None:
            link(b, b_out, n, names[1])
        return n

    def lerp(a, a_out, b, b_out, alpha, alpha_out, x, y):
        n = node(unreal.MaterialExpressionLinearInterpolate, x, y)
        link(a, a_out, n, "A"); link(b, b_out, n, "B"); link(alpha, alpha_out, n, "Alpha")
        return n

    def const(v, x, y):
        n = node(unreal.MaterialExpressionConstant, x, y)
        n.set_editor_property("r", v)
        return n

    M, A, S = unreal.MaterialExpressionMultiply, unreal.MaterialExpressionAdd, unreal.MaterialExpressionSubtract
    black = unreal.load_asset("/Engine/EngineResources/Black")
    bc = tex_param("BaseColorTex", tex["T_Outpost_Cabin_BC"], unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -600)
    nrm = tex_param("NormalTex", tex["T_Outpost_Cabin_N"], unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -300)
    orm = tex_param("ORMTex", tex["T_Outpost_Cabin_ORM"], unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 0)
    msk = tex_param("MaskTex", tex["T_Outpost_Cabin_M"], unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 300)
    emi = tex_param("EmissiveTex", black, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, 600)

    # luminance and tint
    lw = node(unreal.MaterialExpressionConstant3Vector, -1400, -700)
    lw.set_editor_property("constant", unreal.LinearColor(0.3, 0.59, 0.11, 1.0))
    lum = op(unreal.MaterialExpressionDotProduct, bc, "RGB", lw, "", -1200, -650)
    desat = lerp(bc, "RGB", lum, "", scalar("Desaturate", 0.0, -1200, -500), "", -1000, -600)
    tinted = op(M, desat, "", vector("Tint", (1, 1, 1), -1000, -800), "", -800, -650)
    paneled = op(M, tinted, "", vector("PanelTint", (1, 1, 1), -800, -850), "", -600, -750)
    base = lerp(tinted, "", paneled, "", msk, "G", -400, -650)
    # accent repaint
    lum_s = node(unreal.MaterialExpressionSaturate, -800, -350)
    link(op(A, op(M, lum, "", const(1.5, -1200, -350), "", -1050, -380), "", const(0.3, -1050, -300), "", -920, -350), "", lum_s, "")
    acc = op(M, vector("AccentColor", AMBER, -800, -250), "", lum_s, "", -600, -300)
    acc_a = op(M, msk, "R", scalar("AccentAmount", 0.0, -800, -100), "", -600, -150)
    base2 = lerp(base, "", acc, "", acc_a, "", -200, -500)
    # soot gradient: height above the pivot
    wp = node(unreal.MaterialExpressionWorldPosition, -1400, 900)
    op_ = node(unreal.MaterialExpressionObjectPositionWS, -1400, 1000)
    rel = op(S, wp, "", op_, "", -1200, 950)
    relz = node(unreal.MaterialExpressionComponentMask, -1050, 950)
    for ch, v in (("r", False), ("g", False), ("b", True), ("a", False)):
        relz.set_editor_property(ch, v)
    link(rel, "", relz, "")
    grad = op(unreal.MaterialExpressionDivide, op(S, relz, "", scalar("SootBase", 100.0, -1050, 1050), "", -900, 980), "",
              scalar("SootHeight", 300.0, -900, 1100), "", -750, 1000)
    grad_s = node(unreal.MaterialExpressionSaturate, -600, 1000)
    link(grad, "", grad_s, "")
    inv = node(unreal.MaterialExpressionOneMinus, -450, 1000)
    link(grad_s, "", inv, "")
    soot = op(M, inv, "", scalar("SootAmount", 0.0, -450, 1100), "", -300, 1030)
    sooty = op(M, base2, "", vector("SootColor", (0.2, 0.17, 0.15), -200, -300), "", 0, -400)
    albedo = lerp(base2, "", sooty, "", soot, "", 200, -500)
    # roughness, metallic
    rough = node(unreal.MaterialExpressionSaturate, 200, 100)
    link(op(A, op(M, orm, "G", scalar("RoughnessScale", 1.0, -1200, 80), "", -900, 50), "", op(M, soot, "", const(0.25, -300, 200), "", -100, 150), "", 50, 100), "", rough, "")
    metal = op(M, orm, "B", scalar("MetallicScale", 1.0, -1200, 180), "", -900, 180)
    # screens
    glow = op(M, op(M, emi, "RGB", vector("ScreenColor", (1, 1, 1), -1200, 700), "", -900, 650), "", scalar("ScreenPower", 0.0, -900, 800), "", -700, 700)

    P = unreal.MaterialProperty
    for n, out_name, prop in ((albedo, "", P.MP_BASE_COLOR), (rough, "", P.MP_ROUGHNESS), (metal, "", P.MP_METALLIC), (orm, "R", P.MP_AMBIENT_OCCLUSION),
                              (nrm, "RGB", P.MP_NORMAL), (glow, "", P.MP_EMISSIVE_COLOR)):
        ok[0] &= bool(mel.connect_material_property(n, out_name, prop))
    if not ok[0]:
        log("FAILED: could not wire " + MASTER)
        return None
    try:
        mel.recompile_material(mat)
    except Exception:
        log("note: no renderer in this headless run; M_Outpost compiles on first load in an editor or game")
    eas.save_asset(MASTER, only_if_is_dirty=False)
    return mat


def instance(name, parent):
    path = "%s/%s" % (DEST, name)
    mi = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(
        name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    return mi


def ensure_pipeline(notes):
    ensure_dir(DEST + "/Pipelines")
    if not eas.does_asset_exist(PIPELINE):
        eas.duplicate_asset("/Interchange/Pipelines/DefaultAssetsPipeline", PIPELINE)
    pipe = eas.load_asset(PIPELINE)
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
    return unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1]))


def main():
    notes = []
    ensure_dir(DEST)
    ensure_dir(TEX_DEST)
    tex = import_textures(notes)
    if len(tex) != len(TEXTURES):
        log("FAILED: textures %s %s" % (sorted(tex), notes))
        return
    master = build_master(tex)
    if master is None:
        return
    mis = {}
    for name, (tset, scalars, vectors) in VARIANTS.items():
        mi = instance(name, master)
        for pname, suffix in (("BaseColorTex", "BC"), ("NormalTex", "N"), ("ORMTex", "ORM"), ("MaskTex", "M")):
            mel.set_material_instance_texture_parameter_value(mi, pname, tex["T_Outpost_%s_%s" % (tset, suffix)])
        mel.set_material_instance_texture_parameter_value(mi, "EmissiveTex", tex["T_Outpost_%s_E" % tset])
        for k, v in scalars.items():
            mel.set_material_instance_scalar_parameter_value(mi, k, v)
        for k, v in vectors.items():
            mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
        eas.save_loaded_asset(mi)
        mis[name] = mi
    pipe = ensure_pipeline(notes)
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    rows, bad = [], 0
    for asset, mi_name in MESHES.items():
        with open(os.path.join(KIT, "export", asset + ".manifest.json"), encoding="utf-8") as fh:
            man = json.load(fh)
        fbx = os.path.join(KIT, "export", asset + ".fbx")
        n = []
        params = unreal.ImportAssetParameters()
        params.is_automated = True
        params.replace_existing = True
        params.override_pipelines.append(pipe)
        mgr.import_asset(DEST, unreal.InterchangeManager.create_source_data(fbx), params)
        mesh = eas.load_asset("%s/%s" % (DEST, asset))
        if mesh is None:
            log("FAILED %s: not found after import" % asset)
            bad += 1
            continue
        try:
            sme.set_generate_lightmap_uv(mesh, False)
        except Exception as error:
            n.append("lightmap: %s" % error)
        try:
            ns = mesh.get_editor_property("nanite_settings")
            ns.enabled = False
            mesh.set_editor_property("nanite_settings", ns)
        except Exception as error:
            n.append("nanite: %s" % error)
        for i in range(len(mesh.static_materials)):
            mesh.set_material(i, mis[mi_name])
        eas.save_loaded_asset(mesh)
        bb = mesh.get_bounding_box()
        mn = [round(bb.min.x, 1), round(bb.min.y, 1), round(bb.min.z, 1)]
        mx = [round(bb.max.x, 1), round(bb.max.y, 1), round(bb.max.z, 1)]
        try:
            tris = int(mesh.get_num_triangles(0))
        except Exception:
            tris = -1
        try:
            hulls = int(sme.get_convex_collision_count(mesh))
        except Exception as error:
            hulls = -1
            n.append("hull count: %s" % error)
        sockets = []
        try:
            s = mesh.find_socket("Interact")
            if s:
                loc = s.get_editor_property("relative_location")
                sockets.append(["Interact", round(loc.x, 1), round(loc.y, 1), round(loc.z, 1)])
        except Exception as error:
            n.append("socket: %s" % error)
        size = [round(mx[i] - mn[i], 1) for i in range(3)]
        ok = all(abs(size[i] - man["size_cm"][i]) < 1.0 for i in range(3)) and tris == man["tris"] \
            and hulls == len(man.get("collision", [])) and len(mesh.static_materials) == 1
        if not ok:
            bad += 1
        row = {"asset": asset, "ok": ok, "min_cm": mn, "max_cm": mx, "size_cm": size, "tris": tris, "hulls": hulls,
               "slots": [str(sm.material_slot_name) for sm in mesh.static_materials], "material": mi_name, "sockets": sockets, "notes": n}
        rows.append(row)
        log(json.dumps(row))
    # size on disk
    content = os.path.join(REPO, "Content", "TriadDrift", "Gameplay", "Props", "Outpost")
    total = sum(os.path.getsize(p) for p in glob.glob(os.path.join(content, "**", "*.uasset"), recursive=True))
    out = os.path.join(KIT, "review", "ue_import_report.json")
    with open(out, "w", encoding="utf-8") as fh:
        json.dump({"meshes": rows, "uasset_bytes": total, "notes": notes}, fh, indent=1)
    log("report: %s; uasset MB %.2f; notes %s" % (out, total / 1e6, notes))
    log("imported %d mesh(es), %d failed" % (len(rows), bad))
    if rows and not bad:
        log("OK")
    else:
        log("FAILED: see the lines above")


main()
