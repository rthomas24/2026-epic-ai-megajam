# Render MI_Ground_Verdance and MI_Ground_Cinder on a 6 km ground slab (the same slab ATDPlanetCourse builds) from the
# arrival altitude (100 m, shallow pitch, like the in-game captures) and from eye height (1.7 m), under the planet
# light rigs of ATDPlaceholderSky (Verdance / Cinder styles) with a fixed exposure, so before/after are comparable.
# Read-only for content: a throwaway level under Props/Planets/Review (deleted by the runner), nothing saved.
# Needs a renderer (materials compile): run with ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1.
#   $env:GT_TAG = "before"; powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngProps\Planets\scripts\ue_render_ground_review.py
import os

import unreal as ue

TAG = os.environ.get("GT_TAG", "review")
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
OUT = os.path.join(REPO, "Saved", "GroundTiling", TAG)
DEST = "/Game/TriadDrift/Gameplay/Props/Planets"
eal = ue.EditorAssetLibrary
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
editor = ue.get_editor_subsystem(ue.UnrealEditorSubsystem)
problems = []

# ATDPlaceholderSky planet styles: sun rotation / intensity / colour, fill colour, rayleigh, exposure bias
RIGS = {
    "Verdance": dict(sun=(-9.0, 200.0), sun_i=4.0, sun_c=(1.0, 0.78, 0.62), fill_c=(0.35, 0.9, 0.85),
                     rayleigh=(0.0035, 0.02, 0.03), bias=float(os.environ.get("GT_BIAS_V", "0.0"))),
    "Cinder": dict(sun=(-22.0, 130.0), sun_i=6.0, sun_c=(1.0, 0.55, 0.35), fill_c=(1.0, 0.4, 0.2),
                   rayleigh=(0.03, 0.011, 0.005), bias=float(os.environ.get("GT_BIAS_C", "0.0"))),
}
VIEWS = {   # name: (location cm, pitch, yaw)
    "alt100": ((0.0, 0.0, 10000.0), -32.0, 20.0),
    "alt100_far": ((0.0, 0.0, 10000.0), -12.0, 20.0),
    "eye": ((0.0, 0.0, 170.0), -12.0, 20.0),
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


def main():
    os.makedirs(OUT, exist_ok=True)
    world = editor.get_editor_world()
    for cmd in ("r.TextureStreaming 0", "Editor.AsyncTextureCompilationFinishAll"):
        ue.SystemLibrary.execute_console_command(world, cmd)
    levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    import random
    if not levels.new_level(DEST + "/Review/L_GroundTilingReview_%d" % random.randint(0, 999999)):
        log("FAILED: could not create an isolated level")
        return
    world = editor.get_editor_world()
    # Full-resolution textures come from the editor's -NoTextureStreaming flag (run_ground_review.ps1). Do not flip
    # never_stream here: it rebuilds the textures asynchronously and captures then randomly see the white placeholder.
    for cmd in ("Editor.AsyncTextureCompilationFinishAll", "Editor.AsyncAssetCompilationFinishAll"):
        ue.SystemLibrary.execute_console_command(world, cmd)
    # optional in-memory overrides for experiments (never saved): GT_SET="Verdance.EmissiveStrength=0;Cinder.FarScale=4"
    for item in filter(None, os.environ.get("GT_SET", "").split(";")):
        key, val = item.split("=")
        g, pname = key.split(".")
        if g not in RIGS:
            continue
        mi = eal.load_asset("%s/MI_Ground_%s" % (DEST, g))
        ue.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi, pname, float(val))
        ue.MaterialEditingLibrary.update_material_instance(mi)
        log("override (not saved) %s.%s = %s" % (g, pname, val))
    for g in ("Verdance", "Cinder"):
        m = eal.load_asset("%s/MI_Ground_%s" % (DEST, g))
        st = ue.MaterialEditingLibrary.get_statistics(m)
        log("stats MI_Ground_%s: pixel %s vertex %s samplers %s" % (g, st.get_editor_property("num_pixel_shader_instructions"),
                                                                   st.get_editor_property("num_vertex_shader_instructions"),
                                                                   st.get_editor_property("num_samplers")))
    ground = actors.spawn_actor_from_class(ue.StaticMeshActor, ue.Vector(0, 0, -100))
    gc = ground.static_mesh_component
    gc.set_static_mesh(eal.load_asset("/Engine/BasicShapes/Cube"))
    ground.set_actor_scale3d(ue.Vector(6000.0, 6000.0, 2.0))
    sun = actors.spawn_actor_from_class(ue.DirectionalLight, ue.Vector(0, 0, 0))
    sl = sun.light_component
    sl.set_mobility(ue.ComponentMobility.MOVABLE)
    sl.set_editor_property("atmosphere_sun_light", True)
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
    sc.set_editor_property("real_time_capture", os.environ.get("GT_RTSKY", "0") == "1")
    sc.set_editor_property("lower_hemisphere_is_black", False)
    sc.set_intensity(1.0)

    cap_actor = actors.spawn_actor_from_class(ue.SceneCapture2D, ue.Vector(0, 0, 0))
    cap = cap_actor.get_component_by_class(ue.SceneCaptureComponent2D)
    cap.set_editor_property("capture_every_frame", False)
    cap.set_editor_property("capture_on_movement", False)
    cap.set_editor_property("capture_source", getattr(ue.SceneCaptureSource, os.environ.get("GT_SOURCE", "SCS_FINAL_COLOR_LDR")))
    cap.set_editor_property("fov_angle", 90.0)
    cap.set_editor_property("post_process_blend_weight", 1.0)
    rt = ue.RenderingLibrary.create_render_target2d(world, 1600, 900, ue.TextureRenderTargetFormat.RTF_RGBA8)
    rt.set_editor_property("target_gamma", 2.2)
    cap.set_editor_property("texture_target", rt)
    files = []
    order = [g for g in RIGS if g in os.environ.get("GT_ONLY", "Verdance,Cinder").split(",")]
    if os.environ.get("GT_REVERSE", "0") == "1":
        order.reverse()
    for g in order:
        r = RIGS[g]
        gc.set_material(0, eal.load_asset("%s/MI_Ground_%s" % (DEST, g)))
        sun.set_actor_rotation(ue.Rotator(pitch=r["sun"][0], yaw=r["sun"][1], roll=0.0), False)
        sl.set_intensity(r["sun_i"])
        sl.set_light_color(ue.LinearColor(*r["sun_c"], 1.0))
        fl.set_light_color(ue.LinearColor(*r["fill_c"], 1.0))
        ac.set_rayleigh_scattering(ue.LinearColor(*r["rayleigh"], 1.0))
        sc.recapture_sky()
        cap.capture_scene()                 # requests the shader maps / textures; FinishAll then blocks until they exist
        ue.SystemLibrary.execute_console_command(world, "Editor.AsyncAssetCompilationFinishAll")
        cap.set_editor_property("post_process_settings", post(r["bias"]))
        for vname, (loc, pitch, yaw) in VIEWS.items():
            cap_actor.set_actor_location(ue.Vector(*loc), False, False)
            cap_actor.set_actor_rotation(ue.Rotator(pitch=pitch, yaw=yaw, roll=0.0), False)
            for cmd in ("Editor.AsyncTextureCompilationFinishAll", "Editor.AsyncAssetCompilationFinishAll"):
                ue.SystemLibrary.execute_console_command(world, cmd)
            for _ in range(6):              # let the sky light / atmosphere LUTs settle
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
