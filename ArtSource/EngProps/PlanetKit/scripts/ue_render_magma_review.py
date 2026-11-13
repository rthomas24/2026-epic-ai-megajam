# SPDX-License-Identifier: MIT
# Render the Cinder magma volumes BEFORE (gameplay's placeholder: the engine cube stretched over the volume, tinted
# with M_TD_PlaceholderGlow at (0.85, 0.12, 0.02) x 1.1, exactly as ATDMagmaVolume::BeginPlay does) and AFTER
# (SM_Cind_MagmaSurface fitted the way TDPropArt::Dress fits it), next to the Cinder ground (MI_Ground_Cinder on the
# 6 km slab ATDPlanetCourse builds, top at Z 0), at the volumes' real sizes and course positions:
#   field  centre (5600, 4800), 1600 x 7000 cm      lake  centre (8700, -5000), 3400 x 5000 cm
# from the arrival altitude (100 m, pitch -32) and eye height (1.7 m), under the Cinder light rig of
# ue_render_ground_review.py. Read-only for content: a throwaway level (deleted by run_magma_review.ps1).
#   powershell -File ArtSource\EngProps\PlanetKit\scripts\run_magma_review.ps1 -Tag r1
import os

import unreal as ue

TAG = os.environ.get("MG_TAG", "review")
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
OUT = os.path.join(REPO, "Saved", "MagmaSurface", TAG)
LEVEL_DIR = "/Game/TriadDrift/Gameplay/Props/Planet/Review_MagmaTmp"
eal = ue.EditorAssetLibrary
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
editor = ue.get_editor_subsystem(ue.UnrealEditorSubsystem)
problems = []

VOLUMES = {"field": ((5600.0, 4800.0), (1600.0, 7000.0)), "lake": ((8700.0, -5000.0), (3400.0, 5000.0))}
VIEWS = {   # name: (volume, location cm, pitch, yaw[, fov]); *_zoom: same vantage, 30 deg FOV (detail inspection)
    "field_alt100": ("field", (5600.0 - 16000.0, 4800.0, 10000.0), -32.0, 0.0),
    "field_alt100_zoom": ("field", (5600.0 - 16000.0, 4800.0, 10000.0), -32.0, 0.0, 30.0),
    "field_eye": ("field", (4500.0, 3300.0, 170.0), -10.0, 30.0),
    "field_eye_low": ("field", (4700.0, 6200.0, 170.0), -25.0, 10.0),
    "lake_alt100": ("lake", (8700.0 - 16000.0, -5000.0, 10000.0), -32.0, 0.0),
    "lake_eye": ("lake", (6700.0, -5800.0, 170.0), -10.0, 15.0),
}
PLACEHOLDER_TINT = (0.85, 0.12, 0.02)
PLACEHOLDER_BOOST = 1.1


def log(m):
    ue.log("TDPY: " + str(m))


def post(bias):
    pp = ue.PostProcessSettings()
    for k, v in (("auto_exposure_method", ue.AutoExposureMethod.AEM_MANUAL), ("auto_exposure_apply_physical_camera_exposure", False),
                 ("auto_exposure_bias", bias), ("bloom_intensity", 1.3), ("bloom_threshold", 1.5), ("motion_blur_amount", 0.0)):
        pp.set_editor_property("override_" + k, True)
        pp.set_editor_property(k, v)
    return pp


def spawn_mesh(mesh, loc, scale):
    a = actors.spawn_actor_from_class(ue.StaticMeshActor, ue.Vector(*loc))
    c = a.static_mesh_component
    c.set_mobility(ue.ComponentMobility.MOVABLE)
    c.set_static_mesh(mesh)
    a.set_actor_scale3d(ue.Vector(*scale))
    return a, c


def dress_transform(center, size, art):
    """World location / scale of the art component exactly as ATDMagmaVolume::SetSize + TDPropArt::Dress(bFit)."""
    shape_scale = (size[0] / 100.0, size[1] / 100.0, 0.1)      # Surface: unit cube scaled, at relative Z -5
    shape_loc = (center[0], center[1], -5.0)
    bb = art.get_bounding_box()
    amin, amax = (bb.min.x, bb.min.y, bb.min.z), (bb.max.x, bb.max.y, bb.max.z)
    asize = [max(amax[i] - amin[i], 1.0) for i in range(3)]
    acenter = [(amax[i] + amin[i]) * 0.5 for i in range(3)]
    fit = [100.0 / asize[i] for i in range(3)]                  # cube box is 100 cm, centred
    rel = [0.0 - acenter[i] * fit[i] for i in range(3)]          # no turn: the art is square
    loc = tuple(shape_loc[i] + rel[i] * shape_scale[i] for i in range(3))
    scale = tuple(fit[i] * shape_scale[i] for i in range(3))
    return loc, scale


def main():
    os.makedirs(OUT, exist_ok=True)
    world = editor.get_editor_world()
    levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    import random
    if not levels.new_level(LEVEL_DIR + "/L_MagmaReview_%d" % random.randint(0, 999999)):
        log("FAILED: could not create an isolated level")
        return
    world = editor.get_editor_world()
    for cmd in ("r.TextureStreaming 0", "Editor.AsyncTextureCompilationFinishAll", "Editor.AsyncAssetCompilationFinishAll"):
        ue.SystemLibrary.execute_console_command(world, cmd)
    cube = eal.load_asset("/Engine/BasicShapes/Cube")
    art = eal.load_asset("/Game/TriadDrift/Gameplay/Props/Planet/SM_Cind_MagmaSurface")
    mi = eal.load_asset("/Game/TriadDrift/Gameplay/Props/Planet/MI_Cind_MagmaSurface")
    glow = eal.load_asset("/Game/TriadDrift/Gameplay/Materials/M_TD_PlaceholderGlow")
    if art is None or mi is None or glow is None:
        log("FAILED: missing art %s mi %s glow %s" % (art, mi, glow))
        return
    st = ue.MaterialEditingLibrary.get_statistics(mi)
    log("stats MI_Cind_MagmaSurface: pixel %s vertex %s samplers %s" % (
        st.get_editor_property("num_pixel_shader_instructions"), st.get_editor_property("num_vertex_shader_instructions"),
        st.get_editor_property("num_samplers")))

    ground, gc = spawn_mesh(cube, (0.0, 0.0, -100.0), (6000.0, 6000.0, 2.0))
    gc.set_material(0, eal.load_asset("/Game/TriadDrift/Gameplay/Props/Planets/MI_Ground_Cinder"))
    before, after = [], []
    for vname, (center, size) in VOLUMES.items():
        a, c = spawn_mesh(cube, (center[0], center[1], -5.0), (size[0] / 100.0, size[1] / 100.0, 0.1))
        c.set_cast_shadow(False)
        mid = c.create_dynamic_material_instance(0, glow)
        col = ue.LinearColor(*(x * PLACEHOLDER_BOOST for x in PLACEHOLDER_TINT), 1.0)
        mid.set_vector_parameter_value("Color", col)
        mid.set_vector_parameter_value("EmissiveColor", col)
        before.append(a)
        loc, scale = dress_transform(center, size, art)
        b, bc = spawn_mesh(art, loc, scale)
        bc.set_material(0, mi)
        after.append(b)
        log("%s: art at %s scale %s (top Z %.2f)" % (vname, [round(v, 2) for v in loc], [round(v, 3) for v in scale],
                                                     loc[2] + 10.0 * scale[2]))

    # Cinder rig (ATDPlaceholderSky style, as ue_render_ground_review.py)
    sun = actors.spawn_actor_from_class(ue.DirectionalLight, ue.Vector(0, 0, 0))
    sl = sun.light_component
    sl.set_mobility(ue.ComponentMobility.MOVABLE)
    sl.set_editor_property("atmosphere_sun_light", True)
    sun.set_actor_rotation(ue.Rotator(pitch=-22.0, yaw=130.0, roll=0.0), False)
    sl.set_intensity(6.0)
    sl.set_light_color(ue.LinearColor(1.0, 0.55, 0.35, 1.0))
    fill = actors.spawn_actor_from_class(ue.DirectionalLight, ue.Vector(0, 0, 0))
    fl = fill.light_component
    fl.set_mobility(ue.ComponentMobility.MOVABLE)
    fl.set_cast_shadows(False)
    fl.set_intensity(1.2)
    fl.set_editor_property("atmosphere_sun_light", False)
    fl.set_editor_property("forward_shading_priority", 0)
    fl.set_light_color(ue.LinearColor(1.0, 0.4, 0.2, 1.0))
    fill.set_actor_rotation(ue.Rotator(pitch=-25.0, yaw=210.0, roll=0.0), False)
    atmo = actors.spawn_actor_from_class(ue.SkyAtmosphere, ue.Vector(0, 0, 0))
    atmo.get_component_by_class(ue.SkyAtmosphereComponent).set_rayleigh_scattering(ue.LinearColor(0.03, 0.011, 0.005, 1.0))
    sky = actors.spawn_actor_from_class(ue.SkyLight, ue.Vector(0, 0, 0))
    sc = sky.get_component_by_class(ue.SkyLightComponent)
    sc.set_mobility(ue.ComponentMobility.MOVABLE)
    sc.set_editor_property("lower_hemisphere_is_black", False)
    sc.set_intensity(1.0)

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
    sc.recapture_sky()
    cap.capture_scene()
    ue.SystemLibrary.execute_console_command(world, "Editor.AsyncAssetCompilationFinishAll")
    cap.set_editor_property("post_process_settings", post(float(os.environ.get("MG_BIAS", "-3.5"))))

    # in-memory parameter overrides for tuning (never saved): MG_SET="EmissiveStrength=5;ChannelMax=0.4"
    mel = ue.MaterialEditingLibrary
    for item in filter(None, os.environ.get("MG_SET", "").split(";")):
        k, v = item.split("=")
        mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
        mel.update_material_instance(mi)
        log("override (not saved) %s = %s" % (k, v))

    def shoot(name):
        for _ in range(6):
            cap.capture_scene()
        ue.RenderingLibrary.export_render_target(world, rt, OUT, name)
        path = os.path.join(OUT, name)
        if not os.path.isfile(path) or os.path.getsize(path) < 1000:
            problems.append("missing capture " + path)
        log("captured " + path)

    modes = [m for m in ("before", "after") if m in os.environ.get("MG_MODES", "before,after").split(",")]
    for mode in modes:
        for a in before:
            a.set_actor_hidden_in_game(mode != "before")
            a.set_is_temporarily_hidden_in_editor(mode != "before")
        for a in after:
            a.set_actor_hidden_in_game(mode != "after")
            a.set_is_temporarily_hidden_in_editor(mode != "after")
        for vname, view in VIEWS.items():
            _vol, loc, pitch, yaw = view[:4]
            cap.set_editor_property("fov_angle", view[4] if len(view) > 4 else 90.0)
            cap_actor.set_actor_location(ue.Vector(*loc), False, False)
            cap_actor.set_actor_rotation(ue.Rotator(pitch=pitch, yaw=yaw, roll=0.0), False)
            for cmd in ("Editor.AsyncTextureCompilationFinishAll", "Editor.AsyncAssetCompilationFinishAll"):
                ue.SystemLibrary.execute_console_command(world, cmd)
            shoot("%s_%s_%s.png" % (mode, vname, TAG))
        if mode == "after":       # the flow: the same eye view 20 s later (FlowTimeOffset, in memory only)
            mel.set_material_instance_scalar_parameter_value(mi, "FlowTimeOffset", 20.0)
            mel.update_material_instance(mi)
            loc, pitch, yaw = VIEWS["field_eye_low"][1:4]
            cap.set_editor_property("fov_angle", 90.0)
            cap_actor.set_actor_location(ue.Vector(*loc), False, False)
            cap_actor.set_actor_rotation(ue.Rotator(pitch=pitch, yaw=yaw, roll=0.0), False)
            shoot("after_field_eye_low_t20_%s.png" % TAG)
    for pr in problems:
        log("PROBLEM: " + pr)
    log("FAILED (%d problems)" % len(problems) if problems else "OK captures in %s" % OUT)


main()
