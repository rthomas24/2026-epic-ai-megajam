# Render Verdance and Cinder (surface + atmosphere + ring) lit by a known sun from three camera angles: front-lit
# (sun behind the camera), side-lit (terminator through the middle) and back-lit (thin crescent). The lights and
# post-process copy ATDPlaceholderSky's Space style (sun 8 lux, violet fill 1.2, sky light 1.0, manual exposure 0,
# bloom 0.8 / threshold 1.5) and the nebula sky sphere is placed around the planet, so the lit "before" material is
# judged under the real in-game light rig. Read-only for content: a throwaway level, nothing saved.
# Needs a renderer (materials compile): run with ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1.
#   $env:PL_REVIEW_TAG = "before"; powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngProps\Planets\scripts\ue_render_planet_review.py
import math
import os

import unreal as ue

TAG = os.environ.get("PL_REVIEW_TAG", "review")
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
OUT = os.path.join(REPO, "Saved", "PlanetNight", TAG)
DEST = "/Game/TriadDrift/Gameplay/Props/Planets"
SUN = ue.Vector(0.779, 0.546, 0.309)            # ATDSkySphere baked sun (direction TOWARDS the sun)
SUN_YAW, SUN_PITCH = 35.0, 18.0
DIST = 640.0                                    # cm; planet radius 100 cm at scale 1
eal = ue.EditorAssetLibrary
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
editor = ue.get_editor_subsystem(ue.UnrealEditorSubsystem)
problems = []


def log(m):
    ue.log("TDPY: " + str(m))


def norm(v):
    l = math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z)
    return ue.Vector(v.x / l, v.y / l, v.z / l)


def cross(a, b):
    return ue.Vector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)


def rot_about(v, axis, deg):
    """Rodrigues rotation of v about unit axis."""
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    k = axis
    kv = cross(k, v)
    kd = k.x * v.x + k.y * v.y + k.z * v.z
    return ue.Vector(v.x * c + kv.x * s + k.x * kd * (1 - c), v.y * c + kv.y * s + k.y * kd * (1 - c),
                     v.z * c + kv.z * s + k.z * kd * (1 - c))


def mesh_actor(mesh, mat, scale=1.0, rot=None):
    a = actors.spawn_actor_from_class(ue.StaticMeshActor, ue.Vector(0, 0, 0))
    c = a.static_mesh_component
    c.set_mobility(ue.ComponentMobility.MOVABLE)
    c.set_static_mesh(eal.load_asset(mesh))
    if mat:
        c.set_material(0, eal.load_asset(mat))
    a.set_actor_scale3d(ue.Vector(scale, scale, scale))
    if rot:
        a.set_actor_rotation(rot, False)
    return a


def light_rig():
    sun = actors.spawn_actor_from_class(ue.DirectionalLight, ue.Vector(0, 0, 0))
    sun.set_actor_rotation(ue.Rotator(pitch=-SUN_PITCH, yaw=SUN_YAW + 180.0, roll=0.0), False)
    lc = sun.light_component
    lc.set_mobility(ue.ComponentMobility.MOVABLE)
    lc.set_intensity(8.0)
    lc.set_light_color(ue.LinearColor(1.0, 0.93, 0.82, 1.0))
    lc.set_editor_property("atmosphere_sun_light", True)
    fwd = sun.get_actor_forward_vector()
    log("sun forward %s (expect -%s)" % (fwd, SUN))
    fill = actors.spawn_actor_from_class(ue.DirectionalLight, ue.Vector(0, 0, 0))
    fill.set_actor_rotation(ue.Rotator(pitch=20.0, yaw=SUN_YAW, roll=0.0), False)
    fc = fill.light_component
    fc.set_mobility(ue.ComponentMobility.MOVABLE)
    fc.set_intensity(1.2)
    fc.set_light_color(ue.LinearColor(0.5, 0.4, 1.0, 1.0))
    fc.set_cast_shadows(False)
    fc.set_editor_property("atmosphere_sun_light", False)
    fc.set_editor_property("forward_shading_priority", 0)
    sky = actors.spawn_actor_from_class(ue.SkyLight, ue.Vector(0, 0, 0))
    sc = sky.get_component_by_class(ue.SkyLightComponent)
    sc.set_mobility(ue.ComponentMobility.MOVABLE)
    sc.set_intensity(1.0)
    sc.set_light_color(ue.LinearColor(0.6, 0.7, 1.0, 1.0))
    sc.set_editor_property("lower_hemisphere_is_black", False)
    sc.recapture_sky()
    return sun, fill, sky


def post():
    pp = ue.PostProcessSettings()
    for k, v in (("auto_exposure_method", ue.AutoExposureMethod.AEM_MANUAL), ("auto_exposure_apply_physical_camera_exposure", False),
                 ("auto_exposure_bias", 0.0), ("bloom_intensity", 0.8), ("bloom_threshold", 1.5), ("motion_blur_amount", 0.0)):
        pp.set_editor_property("override_" + k, True)
        pp.set_editor_property(k, v)
    return pp


def compile_all(paths):
    for p in paths:
        m = eal.load_asset(p)
        if m is None:
            problems.append("missing " + p)
            continue
        st = ue.MaterialEditingLibrary.get_statistics(m)
        n = st.get_editor_property("num_pixel_shader_instructions")
        log("compiled %s: %s pixel instructions" % (p.split("/")[-1], n))
        if not n:
            problems.append("no pixel shader for " + p)


def main():
    os.makedirs(OUT, exist_ok=True)
    world = editor.get_editor_world()
    for cmd in ("r.TextureStreaming 0", "Editor.AsyncTextureCompilationFinishAll", "Editor.AsyncStaticMeshCompilationFinishAll"):
        ue.SystemLibrary.execute_console_command(world, cmd)
    levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    # Throwaway level (Unreal refuses /Temp here): delete Content/TriadDrift/Gameplay/Props/Planets/Review afterwards.
    if not levels.new_level(DEST + "/Review/L_PlanetNightReview"):
        log("FAILED: could not create an isolated level")
        return
    world = editor.get_editor_world()
    # Full-resolution textures without waiting for streaming ticks (in memory only; nothing is saved).
    for p in ("Verdance", "Cinder"):
        for s_ in ("BC", "N", "ORM", "E", "Clouds"):
            t = eal.load_asset("%s/T_Planet_%s_%s" % (DEST, p, s_))
            if t:
                t.set_editor_property("never_stream", True)
    mats = ["%s/%s" % (DEST, n) for n in ("MI_Planet_Verdance", "MI_Planet_Cinder", "MI_PlanetAtmo_Verdance", "MI_PlanetAtmo_Cinder",
                                          "MI_PlanetRing_Verdance")] + ["/Game/TriadDrift/Gameplay/Props/Sky/MI_TD_NebulaSky"]
    compile_all(mats)
    if os.environ.get("PL_REVIEW_NOSKY", "0") != "1":
        sky_sphere = mesh_actor("/Game/TriadDrift/Gameplay/Props/Sky/SM_TD_SkySphere", "/Game/TriadDrift/Gameplay/Props/Sky/MI_TD_NebulaSky", 50000.0)
        sky_sphere.static_mesh_component.set_cast_shadow(False)
    light_rig()
    for cmd in ("Editor.AsyncTextureCompilationFinishAll", "Editor.AsyncStaticMeshCompilationFinishAll"):
        ue.SystemLibrary.execute_console_command(world, cmd)

    cap_actor = actors.spawn_actor_from_class(ue.SceneCapture2D, ue.Vector(0, 0, 0))
    cap = cap_actor.get_component_by_class(ue.SceneCaptureComponent2D)
    cap.set_editor_property("capture_every_frame", False)
    cap.set_editor_property("capture_on_movement", False)
    cap.set_editor_property("capture_source", ue.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    cap.set_editor_property("fov_angle", 48.0)
    cap.set_editor_property("post_process_settings", post())
    cap.set_editor_property("post_process_blend_weight", 1.0)
    rt = ue.RenderingLibrary.create_render_target2d(world, 1024, 1024, ue.TextureRenderTargetFormat.RTF_RGBA8)
    rt.set_editor_property("target_gamma", 2.2)
    cap.set_editor_property("texture_target", rt)

    s = norm(SUN)
    up = ue.Vector(0, 0, 1)
    side_axis = norm(cross(up, s))                 # horizontal, perpendicular to the sun
    views = {"1_front": rot_about(s, side_axis, -12.0),          # sun just over the camera's shoulder
             "2_side": norm(cross(s, up)),                        # 90 deg from the sun: terminator down the middle
             "3_back": rot_about(s, norm(cross(s, up)), 155.0)}   # 155 deg from the sun: crescent
    planets = {
        "Verdance": dict(surface="MI_Planet_Verdance", atmo="MI_PlanetAtmo_Verdance", ring=True),
        "Cinder": dict(surface="MI_Planet_Cinder", atmo="MI_PlanetAtmo_Cinder", ring=False),
    }
    files = []
    for pname, p in planets.items():
        spawned = [mesh_actor(DEST + "/SM_Planet_Sphere", "%s/%s" % (DEST, p["surface"])),
                   mesh_actor(DEST + "/SM_Planet_Atmosphere", "%s/%s" % (DEST, p["atmo"]))]
        if p["ring"]:
            spawned.append(mesh_actor(DEST + "/SM_Planet_Ring", DEST + "/MI_PlanetRing_Verdance", 1.0, ue.Rotator(pitch=0, yaw=0, roll=18.0)))
        for a in spawned:
            o, ext = a.get_actor_bounds(False)
            c = a.static_mesh_component
            log("%s mesh %s mat %s bounds origin %s extent %s visible %s" % (
                a.get_actor_label(), c.static_mesh.get_name() if c.static_mesh else None,
                c.get_material(0).get_name() if c.get_material(0) else None, o, ext, c.is_visible()))
        for vname, d in views.items():
            dn = norm(d)
            loc = ue.Vector(dn.x * DIST, dn.y * DIST, dn.z * DIST)
            cap_actor.set_actor_location(loc, False, False)
            cap_actor.set_actor_rotation(ue.MathLibrary.find_look_at_rotation(loc, ue.Vector(0, 0, 0)), False)
            ue.SystemLibrary.execute_console_command(world, "Editor.AsyncTextureCompilationFinishAll")
            cap.capture_scene()
            name = "%s_%s_%s.png" % (pname, vname, TAG)
            ue.RenderingLibrary.export_render_target(world, rt, OUT, name)
            path = os.path.join(OUT, name)
            if not os.path.isfile(path) or os.path.getsize(path) < 1000:
                problems.append("missing capture " + path)
            cosang = dn.x * s.x + dn.y * s.y + dn.z * s.z
            log("captured %s (camera-sun angle %.0f deg)" % (path, math.degrees(math.acos(max(-1, min(1, cosang))))))
            files.append(path)
        for a in spawned:
            actors.destroy_actor(a)
    for pr in problems:
        log("PROBLEM: " + pr)
    log("FAILED (%d problems)" % len(problems) if problems else "OK %d captures in %s" % (len(files), OUT))


main()
