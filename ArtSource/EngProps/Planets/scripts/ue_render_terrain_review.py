# SPDX-License-Identifier: MIT
# Review renders of the terrain materials on a test terrain that carries ATDPlanetTerrain's vertex channels (made by
# build_terrain_review_mesh.py, loaded into a ProceduralMeshComponent like the game's chunks), under the planet light
# rigs of ATDPlaceholderSky (sun, fill, sky light, atmosphere, height fog) with a fixed exposure.
#   TM_TAG  = before (MI_Ground_<planet> on the terrain) | after (MI_Terrain_<planet>) | any other tag = after
#   TM_FOG  = 0 to leave the height fog out;  TM_ONLY = Verdance,Cinder;  TM_SET = "Cinder.ValleyWidth=0.3;..."
#             (in-memory overrides, never saved)
# Read-only for content: a throwaway level under Props/Planets/Review (deleted by run_terrain_review.ps1).
# Captures: Saved/TerrainMat/<tag>/<Planet>_<view>_<tag>.png
import array
import json
import os
import random
import struct

import unreal as ue

TAG = os.environ.get("TM_TAG", "after")
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
DATA = os.path.join(REPO, "Saved", "TerrainMat")
OUT = os.path.join(DATA, TAG)
DEST = "/Game/TriadDrift/Gameplay/Props/Planets"
eal = ue.EditorAssetLibrary
mel = ue.MaterialEditingLibrary
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
editor = ue.get_editor_subsystem(ue.UnrealEditorSubsystem)
problems = []

RIGS = {
    "Verdance": dict(sun=(-9.0, 200.0), sun_i=4.0, sun_c=(1.0, 0.78, 0.62), fill_c=(0.35, 0.9, 0.85),
                     rayleigh=(0.0035, 0.02, 0.03), bias=float(os.environ.get("GT_BIAS_V", "1.0")),
                     fog_d=0.018, fog_c=(0.10, 0.32, 0.28), fog_dc=(0.35, 0.45, 0.35)),
    "Cinder": dict(sun=(-22.0, 130.0), sun_i=6.0, sun_c=(1.0, 0.55, 0.35), fill_c=(1.0, 0.4, 0.2),
                   rayleigh=(0.03, 0.011, 0.005), bias=float(os.environ.get("GT_BIAS_C", "-3.5")),
                   fog_d=0.022, fog_c=(0.42, 0.16, 0.05), fog_dc=(0.6, 0.3, 0.12)),
}


def log(m):
    ue.log("TDPY: " + str(m))


def post(bias):
    pp = ue.PostProcessSettings()
    for k, v in (("auto_exposure_method", ue.AutoExposureMethod.AEM_MANUAL), ("auto_exposure_apply_physical_camera_exposure", False),
                 ("auto_exposure_bias", bias), ("bloom_intensity", 1.3), ("bloom_threshold", 1.5), ("motion_blur_amount", 0.0)):
        pp.set_editor_property("override_" + k, True)
        pp.set_editor_property(k, v)
    return pp


def add_component(actor, cls):
    sds = ue.get_engine_subsystem(ue.SubobjectDataSubsystem)
    root = sds.k2_gather_subobject_data_for_instance(actor)[0]
    handle, fail = sds.add_new_subobject(ue.AddNewSubobjectParams(parent_handle=root, new_class=cls, blueprint_context=None))
    data = sds.k2_find_subobject_data_from_handle(handle)
    return ue.SubobjectDataBlueprintFunctionLibrary.get_object(data)


def load_sections(planet):
    out = []
    with open(os.path.join(DATA, "terrain_%s.bin" % planet), "rb") as fh:
        (count,) = struct.unpack("<I", fh.read(4))
        for _ in range(count):
            nv, nt = struct.unpack("<II", fh.read(8))
            arrs = []
            for width in (3, 3, 2, 4):
                a = array.array("f")
                a.frombytes(fh.read(4 * width * nv))
                arrs.append(a)
            tris = array.array("i")
            tris.frombytes(fh.read(4 * 3 * nt))
            out.append((nv, arrs, tris))
    return out


def fill_mesh(pmc, planet):
    for index, (nv, (pos, nrm, uv1, col), tris) in enumerate(load_sections(planet)):
        verts = [ue.Vector(pos[i * 3], pos[i * 3 + 1], pos[i * 3 + 2]) for i in range(nv)]
        normals = [ue.Vector(nrm[i * 3], nrm[i * 3 + 1], nrm[i * 3 + 2]) for i in range(nv)]
        uv0s = [ue.Vector2D(pos[i * 3] / 1000.0, pos[i * 3 + 1] / 1000.0) for i in range(nv)]
        uv1s = [ue.Vector2D(uv1[i * 2], uv1[i * 2 + 1]) for i in range(nv)]
        cols = [ue.LinearColor(col[i * 4], col[i * 4 + 1], col[i * 4 + 2], col[i * 4 + 3]) for i in range(nv)]
        # tangent along world X projected on the surface (what ProceduralMesh users normally supply)
        tans = [ue.ProcMeshTangent(ue.Vector(1.0, 0.0, 0.0), False)] * nv
        pmc.create_mesh_section_linear_color(index, verts, list(tris), normals, uv0s, uv1s, [], [], cols, tans, False, False)
        log("%s section %d: %d verts %d tris" % (planet, index, nv, len(tris) // 3))


def main():
    os.makedirs(OUT, exist_ok=True)
    levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    if not levels.new_level(DEST + "/Review/L_TerrainReview_%d" % random.randint(0, 999999)):
        log("FAILED: could not create an isolated level")
        return
    world = editor.get_editor_world()
    for cmd in ("r.TextureStreaming 0", "Editor.AsyncTextureCompilationFinishAll", "Editor.AsyncAssetCompilationFinishAll"):
        ue.SystemLibrary.execute_console_command(world, cmd)
    before = TAG.startswith("before")
    mats = {}
    for g in RIGS:
        mats[g] = eal.load_asset("%s/MI_%s_%s" % (DEST, "Ground" if before else "Terrain", g))
        if mats[g] is None:
            log("FAILED: missing material for " + g)
            return
        st = mel.get_statistics(mats[g])
        log("stats %s: pixel %s vertex %s samplers %s" % (mats[g].get_name(), st.get_editor_property("num_pixel_shader_instructions"),
                                                         st.get_editor_property("num_vertex_shader_instructions"), st.get_editor_property("num_samplers")))
    for item in filter(None, os.environ.get("TM_SET", "").split(";")):
        key, val = item.split("=")
        g, pname = key.split(".")
        mel.set_material_instance_scalar_parameter_value(mats[g], pname, float(val))
        mel.update_material_instance(mats[g])
        log("override (not saved) %s.%s = %s" % (g, pname, val))

    sun = actors.spawn_actor_from_class(ue.DirectionalLight, ue.Vector(0, 0, 0))
    sl = sun.light_component
    sl.set_mobility(ue.ComponentMobility.MOVABLE)
    sl.set_editor_property("atmosphere_sun_light", True)
    if os.environ.get("TM_SHADOW"):          # cm; the game's sun keeps the engine default (200 m of cascades)
        sl.set_editor_property("dynamic_shadow_distance_movable_light", float(os.environ["TM_SHADOW"]))
    fill = actors.spawn_actor_from_class(ue.DirectionalLight, ue.Vector(0, 0, 0))
    fl = fill.light_component
    fl.set_mobility(ue.ComponentMobility.MOVABLE)
    fl.set_cast_shadows(False)
    fl.set_intensity(1.2)
    fl.set_editor_property("atmosphere_sun_light", False)
    fl.set_editor_property("forward_shading_priority", 0)
    fill.set_actor_rotation(ue.Rotator(pitch=-25.0, yaw=210.0, roll=0.0), False)
    atmo = actors.spawn_actor_from_class(ue.SkyAtmosphere, ue.Vector(0, 0, 0))
    ac = atmo.get_component_by_class(ue.SkyAtmosphereComponent)
    sky = actors.spawn_actor_from_class(ue.SkyLight, ue.Vector(0, 0, 0))
    sc = sky.get_component_by_class(ue.SkyLightComponent)
    sc.set_mobility(ue.ComponentMobility.MOVABLE)
    sc.set_editor_property("real_time_capture", False)
    sc.set_editor_property("lower_hemisphere_is_black", False)
    sc.set_intensity(1.0)
    fog = None
    if os.environ.get("TM_FOG", "1") == "1":
        fog_actor = actors.spawn_actor_from_class(ue.ExponentialHeightFog, ue.Vector(0, 0, 0))
        fog = fog_actor.get_component_by_class(ue.ExponentialHeightFogComponent)
        fog.set_fog_height_falloff(0.12)
        fog.set_start_distance(6000.0)
        fog.set_fog_max_opacity(1.0)

    holder = actors.spawn_actor_from_class(ue.StaticMeshActor, ue.Vector(0, 0, 0))
    holder.static_mesh_component.set_mobility(ue.ComponentMobility.MOVABLE)
    pmc = add_component(holder, ue.ProceduralMeshComponent)
    if pmc is None:
        log("FAILED: could not add a ProceduralMeshComponent")
        return
    # the course slab (as ATDPlanetCourse): top at Z 0, the planet's ground instance, inside the basin
    slab = actors.spawn_actor_from_class(ue.StaticMeshActor, ue.Vector(0, 0, -100))
    slab.static_mesh_component.set_static_mesh(eal.load_asset("/Engine/BasicShapes/Cube"))

    cap_actor = actors.spawn_actor_from_class(ue.SceneCapture2D, ue.Vector(0, 0, 0))
    cap = cap_actor.get_component_by_class(ue.SceneCaptureComponent2D)
    cap.set_editor_property("capture_every_frame", False)
    cap.set_editor_property("capture_on_movement", False)
    cap.set_editor_property("capture_source", ue.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    cap.set_editor_property("fov_angle", 90.0)
    cap.set_editor_property("post_process_blend_weight", 1.0)
    rt = ue.RenderingLibrary.create_render_target2d(world, 1600, 900, ue.TextureRenderTargetFormat.RTF_RGBA8)
    rt.set_editor_property("target_gamma", 2.2)
    cap.set_editor_property("texture_target", rt)
    files = []
    for g in [x for x in RIGS if x in os.environ.get("TM_ONLY", "Verdance,Cinder").split(",")]:
        r = RIGS[g]
        with open(os.path.join(DATA, "terrain_%s.json" % g)) as fh:
            info = json.load(fh)
        pmc.clear_all_mesh_sections()
        fill_mesh(pmc, g)
        for i in range(pmc.get_num_sections()):
            pmc.set_material(i, mats[g])
        half = info["basin"] - 5000.0                    # the slab stops 50 m inside the basin edge (terrain is flat there)
        slab.set_actor_scale3d(ue.Vector(half / 50.0, half / 50.0, 2.0))
        slab.static_mesh_component.set_material(0, eal.load_asset("%s/MI_Ground_%s" % (DEST, g)))
        sun.set_actor_rotation(ue.Rotator(pitch=r["sun"][0], yaw=r["sun"][1], roll=0.0), False)
        sl.set_intensity(r["sun_i"])
        sl.set_light_color(ue.LinearColor(*r["sun_c"], 1.0))
        fl.set_light_color(ue.LinearColor(*r["fill_c"], 1.0))
        ac.set_rayleigh_scattering(ue.LinearColor(*r["rayleigh"], 1.0))
        if fog:
            fog.set_fog_density(r["fog_d"])
            fog.set_fog_inscattering_color(ue.LinearColor(*r["fog_c"], 1.0))
            fog.set_directional_inscattering_color(ue.LinearColor(*r["fog_dc"], 1.0))
        sc.recapture_sky()
        cap.capture_scene()
        ue.SystemLibrary.execute_console_command(world, "Editor.AsyncAssetCompilationFinishAll")
        cap.set_editor_property("post_process_settings", post(r["bias"]))
        for vname, v in info["views"].items():
            loc, look = ue.Vector(*v["loc"]), ue.Vector(*v["look"])
            cap_actor.set_actor_location(loc, False, False)
            cap_actor.set_actor_rotation(ue.MathLibrary.find_look_at_rotation(loc, look), False)
            for cmd in ("Editor.AsyncTextureCompilationFinishAll", "Editor.AsyncAssetCompilationFinishAll"):
                ue.SystemLibrary.execute_console_command(world, cmd)
            for _ in range(6):
                cap.capture_scene()
            name = "%s_%s_%s.png" % (g, vname, TAG)
            ue.RenderingLibrary.export_render_target(world, rt, OUT, name)
            path = os.path.join(OUT, name)
            if not os.path.isfile(path) or os.path.getsize(path) < 1000:
                problems.append("missing capture " + path)
            log("captured " + path)
            files.append(path)
    for pr in problems:
        log("PROBLEM: " + pr)
    log("FAILED (%d problems)" % len(problems) if problems else "OK %d captures in %s" % (len(files), OUT))


main()
