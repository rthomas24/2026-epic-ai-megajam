"""Read existing Lancer mesh sources, validate in Blender and render review images.

Authorized review only. Does not regenerate mesh JSON, save .blend, export FBX,
import Unreal assets or change the silhouette. Execute headlessly with two CPU
threads and one Blender process. Files are confined to this asset's review/.

blender -b --factory-startup -t 2 --python-exit-code 1 -P review_lancer.py -- --stage validate
blender -b --factory-startup -t 2 --python-exit-code 1 -P review_lancer.py -- --stage render
"""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import runpy
import sys
import time

import bpy
import bmesh
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
OUT = HERE / "review"
OUT.mkdir(exist_ok=True)
T0 = time.perf_counter()


def write(name, data):
    (OUT / name).write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def fingerprint():
    paths = sorted(HERE.glob("*.mesh.json")) + [HERE / "attachments.json"]
    return {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}


def load():
    # Remove only the factory objects in this explicitly background-only process.
    if not bpy.app.background:
        raise RuntimeError("Run in a new background process; never replace a live user scene")
    for obj in list(bpy.context.scene.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    sc = bpy.context.scene
    sc.unit_settings.system = "METRIC"
    sc.unit_settings.scale_length = 1.0
    sc.unit_settings.length_unit = "METERS"
    result = runpy.run_path(str(HERE / "build_in_blender.py"))["build"](review_authorized=True)
    coll = bpy.data.collections[result["collection"]]
    obs = [o for o in coll.objects if o.type == "MESH"]
    # The sixth source (projectile visual) is not mounted on the assembled ship.
    mesh = bpy.data.meshes["M01_SM_Lancer_SpikeRocket"]
    rocket = bpy.data.objects.new("SM_Lancer_M01_ProjectileSource", mesh)
    coll.objects.link(rocket)
    rocket.hide_render = True
    # Project/UV the otherwise uninstanced sixth source for canonical source QA.
    hs = runpy.run_path(str(REPO / ".claude/skills/blender-game-asset/scripts/hardsurface.py"))
    hs["smart_uv"](rocket)
    hs["smooth_by_angle"](rocket, angle_deg=35)
    bpy.context.view_layer.update()
    return result, coll, obs, rocket


def xyz_to_blender(value):
    x, y, z = value
    return Vector((y / 100, -x / 100, z / 100))


def xyz_from_blender(value):
    x, y, z = value
    return [-y * 100, x * 100, z * 100]


def box(obs):
    bpy.context.view_layer.update()
    pts = [o.matrix_world @ Vector(c) for o in obs for c in o.bound_box]
    lo = Vector(tuple(min(p[k] for p in pts) for k in range(3)))
    hi = Vector(tuple(max(p[k] for p in pts) for k in range(3)))
    return lo, hi


def preserve_geometry_check():
    checks = []
    for p in sorted(HERE.glob("*.mesh.json")):
        data = json.loads(p.read_text(encoding="utf-8"))
        mesh = bpy.data.meshes["M01_" + data["name"]]
        expected = [xyz_to_blender(v) for part in data["parts"] for v in part["vertices"]]
        max_error = max((v.co - q).length for v, q in zip(mesh.vertices, expected))
        expected_faces = []
        base = 0
        for part in data["parts"]:
            expected_faces.extend(tuple(base + i for i in face) for face in part["triangles"])
            base += len(part["vertices"])
        faces_equal = [tuple(p.vertices) for p in mesh.polygons] == expected_faces
        checks.append({"source": p.name, "vertices": len(mesh.vertices), "triangles": len(mesh.polygons),
                       "max_coordinate_error_m": max_error,
                       "source_vertex_count_matches": len(expected) == len(mesh.vertices),
                       "triangle_indices_unchanged": faces_equal,
                       "pass": max_error < 1e-6 and len(expected) == len(mesh.vertices) and faces_equal})
    return checks


def attachment_check(obs):
    data = json.loads((HERE / "attachments.json").read_text(encoding="utf-8"))
    checks = []
    for spec in data["sockets"]:
        name = "SOCKET_" + spec["parent"] + "_" + spec["name"]
        obj = bpy.data.objects[name]
        parent = bpy.data.objects["SM_Lancer_M01_" + spec["parent"]]
        local_err = (obj.location - xyz_to_blender(spec["position_cm"])).length
        actual_forward = (obj.matrix_world.to_3x3() @ Vector((0, -1, 0))).normalized()
        expect_forward = xyz_to_blender([100 * v for v in spec["forward"]]).normalized()
        dot = actual_forward.dot(expect_forward)
        checks.append({"name": spec["name"], "parent": spec["parent"],
                       "world_position_cm": [round(v, 5) for v in xyz_from_blender(obj.matrix_world.translation)],
                       "position_error_cm": local_err * 100, "forward_alignment_dot": dot,
                       "unit_scale": list(obj.scale),
                       "pass": obj.parent == parent and local_err < 1e-6 and dot > .99999 and tuple(obj.scale) == (1., 1., 1.)})
    # Ray tests from each grouped muzzle/tube origin; check the ENTIRE assembly.
    verts, triangles = [], []
    for ob in obs:
        base = len(verts)
        verts.extend(ob.matrix_world @ v.co for v in ob.data.vertices)
        triangles.extend(tuple(base + i for i in p.vertices) for p in ob.data.polygons)
    tree = BVHTree.FromPolygons(verts, triangles, all_triangles=True)
    rays = []
    for spec in data["sockets"]:
        if not (spec["name"].startswith("Muzzle_") or spec["name"].startswith("Thruster_")):
            continue
        ob = bpy.data.objects["SOCKET_" + spec["parent"] + "_" + spec["name"]]
        origin = ob.matrix_world.translation.copy()
        direction = xyz_to_blender([100 * v for v in spec["forward"]]).normalized()
        hit, normal, face, distance = tree.ray_cast(origin + direction * .0001, direction, 100)
        rays.append({"socket": spec["name"], "parent": spec["parent"],
                     "ray_distance_test_m": 100, "unobstructed": hit is None,
                     "first_hit_distance_cm": None if hit is None else round(distance * 100, 4)})
    return {"sockets": checks, "ray_clearance": rays,
            "pass": all(c["pass"] for c in checks) and all(c["unobstructed"] for c in rays)}


def validate(result, coll, obs, rocket, hashes):
    asset_report = runpy.run_path(str(REPO / ".claude/skills/blender-mcp/scripts/asset_report.py"))["main"]
    assembly = runpy.run_path(str(REPO / ".claude/skills/blender-mcp/scripts/assembly_check.py"))["main"]
    qa_path = Path(r"C:\Users\nicho\.agents\skills\blender-asset-qa\scripts\qa_check.py")
    qa = runpy.run_path(str(qa_path))["run"]
    assembly_names = [o.name for o in obs]
    report = asset_report(targets=assembly_names + [rocket.name], tri_budget=20000)
    write("asset_report.json", report)
    print("REVIEW: asset report written", flush=True)
    geometry_qa = qa(objects=assembly_names, profile="game", eps=.001, tol=.002)
    write("geometry_qa.json", geometry_qa)
    print("REVIEW: assembly geometry QA written", flush=True)
    contact = assembly(targets=assembly_names, tolerance=.002)
    write("assembly_contacts.json", contact)
    canonical = []
    seen = set()
    for ob in obs + [rocket]:
        if ob.data.name in seen:
            continue
        seen.add(ob.data.name)
        per = qa(objects=[ob.name], profile="game", eps=.001, tol=.002)
        canonical.append({"source_mesh": ob.data.name, "object": ob.name,
                          "result": per})
    write("canonical_sources_qa.json", canonical)
    preserved = preserve_geometry_check()
    attach = attachment_check(obs)
    lo, hi = box(obs)
    dimensions_ue = [round(v, 4) for v in [100*(hi.y-lo.y),100*(hi.x-lo.x),100*(hi.z-lo.z)]]
    topology_errors = [i for i in report["issues"] if i["severity"] == "error"]
    summary = {"purpose": "silhouette_approval_review_not_production_certification",
               "blender_version": bpy.app.version_string, "background": bpy.app.background,
               "source_geometry_preserved": all(x["pass"] for x in preserved),
               "source_hashes_before": hashes, "source_hashes_after": fingerprint(),
               "source_hashes_unchanged": hashes == fingerprint(),
               "canonical_source_count": len(canonical), "assembly_mesh_instances": len(obs),
               "assembly_triangles": sum(len(o.data.polygons) for o in obs),
               "assembly_dimensions_cm_XYZ": dimensions_ue,
               "assembly_hull_pivot_cm": xyz_from_blender(bpy.data.objects["SM_Lancer_M01_Hull"].location),
               "unit_scale_all_instances": all(tuple(o.scale)==(1.,1.,1.) for o in obs),
               "rotor_pivots_cm": {o.name:xyz_from_blender(o.location) for o in obs if "Rotor" in o.name},
               "topology_asset_report_errors": topology_errors,
               "geometry_qa_pass": geometry_qa["pass"],
               "geometry_qa_errors": geometry_qa["errors"],
               "geometry_qa_warnings": geometry_qa["warnings"],
               "assembly_contact_pass": contact["ok"], "assembly_floating_count": len(contact["floating"]),
               "geometry_source_comparison": preserved,
               "attachments": attach,
               "elapsed_seconds": round(time.perf_counter()-T0,3),
               "production_approved": False, "silhouette_approved": False,
               "unreal_import_performed": False}
    write("validation_summary.json", summary)
    print("REVIEW_SUMMARY " + json.dumps({k:summary[k] for k in ["blender_version","assembly_triangles","assembly_dimensions_cm_XYZ","source_geometry_preserved","geometry_qa_pass","assembly_floating_count"]}), flush=True)


def look(ob, point):
    ob.rotation_euler = (Vector(point)-ob.location).to_track_quat("-Z", "Y").to_euler()


def area(name, location, energy, size, color, target=(0,0,0)):
    data = bpy.data.lights.new(name, "AREA")
    data.energy, data.shape, data.size, data.color = energy, "DISK", size, color
    obj = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(obj)
    obj.location = location
    look(obj, target)
    return obj


def studio():
    sc = bpy.context.scene
    # Dynamic engine enum reports are incomplete; assigning is the supported probe.
    sc.render.engine = "BLENDER_EEVEE"
    if hasattr(sc, "eevee"):
        if hasattr(sc.eevee, "taa_render_samples"):
            sc.eevee.taa_render_samples = 64
        if hasattr(sc.eevee, "use_raytracing"):
            sc.eevee.use_raytracing = True
    sc.render.resolution_x, sc.render.resolution_y = 1600, 1100
    sc.render.resolution_percentage = 100
    sc.render.image_settings.file_format = "PNG"
    sc.render.image_settings.color_mode = "RGB"
    sc.render.image_settings.color_depth = "8"
    sc.render.film_transparent = False
    sc.render.threads_mode, sc.render.threads = "FIXED", 2
    for view in ["Khronos PBR Neutral", "AgX"]:
        try:
            sc.view_settings.view_transform = view
            break
        except TypeError:
            continue
    sc.view_settings.exposure = 0
    sc.world = bpy.data.worlds.new("Review_NeutralWorld")
    sc.world.use_nodes = True
    bg = next(n for n in sc.world.node_tree.nodes if n.type == "BACKGROUND")
    bg.inputs["Color"].default_value = (.075,.105,.145,1)
    bg.inputs["Strength"].default_value = .6
    area("Review_Key", (1,-5,8), 2200, 7, (1,.96,.89))
    area("Review_Fill", (-6,-2,3), 1400, 6, (.75,.87,1))
    area("Review_Rim", (3,6,6), 2600, 6, (.53,.82,1))
    area("Review_LowFill", (-1,0,-6), 900, 5, (.65,.78,1))
    cam = bpy.data.cameras.new("Review_Camera")
    camera = bpy.data.objects.new("Review_Camera", cam)
    sc.collection.objects.link(camera)
    cam.clip_start, cam.clip_end = .01, 200
    sc.camera = camera
    return camera


def camera_frame(camera, targets, az, el, perspective=False, margin=1.18):
    lo,hi=box(targets)
    centre=(lo+hi)*.5
    a,e=math.radians(az),math.radians(el)
    direction=Vector((math.sin(a)*math.cos(e),-math.cos(a)*math.cos(e),math.sin(e)))
    radius=(hi-lo).length*.5
    camera.location=centre+direction*max(radius*3.5,4)
    look(camera,centre)
    camera.data.type="PERSP" if perspective else "ORTHO"
    camera.data.lens=72
    bpy.context.view_layer.update()
    # Fit in camera space, so orthographic front/top/side images are never cropped.
    inv=camera.matrix_world.inverted()
    points=[inv@(ob.matrix_world@v.co) for ob in targets for v in ob.data.vertices]
    width=max(p.x for p in points)-min(p.x for p in points)
    height=max(p.y for p in points)-min(p.y for p in points)
    sc=bpy.context.scene
    aspect=sc.render.resolution_x/sc.render.resolution_y
    if not perspective:
        camera.data.ortho_scale=max(width,height*aspect)*margin
    else:
        dist=max(radius*3.5,4)
        tan_x=math.tan(camera.data.angle_x*.5)
        tan_y=tan_x/aspect
        # Tight perspective fit using each actual vertex, with depth included.
        # Avoid the bounding-sphere framing that leaves the slender ship tiny.
        fit=max(max(abs(p.x)*margin/tan_x,abs(p.y)*margin/tan_y)+p.z+dist for p in points)
        camera.location=centre+direction*max(fit,.5)
        look(camera,centre)


def image_check(path):
    im=bpy.data.images.load(str(path),check_existing=False)
    values=np.empty(len(im.pixels),dtype=np.float32)
    im.pixels.foreach_get(values)
    values=values.reshape((im.size[1],im.size[0],4))
    rgb=values[:,:,:3]
    record={"file":path.name,"width":im.size[0],"height":im.size[1],
            "bytes":path.stat().st_size,"mean_rgb":rgb.mean(axis=(0,1)).tolist(),
            "pixel_std":float(rgb.std()),"nonblack_fraction":float((rgb.max(axis=2)>.025).mean())}
    bpy.data.images.remove(im)
    return record


def sheet(paths, filename):
    # Only resamples actual renders; no generated changes to the depicted asset.
    cols=3;tw,th=800,550;gap=18;rows=math.ceil(len(paths)/cols)
    pixels=np.empty((rows*th+(rows+1)*gap,cols*tw+(cols+1)*gap,4),np.float32)
    pixels[:]=(.018,.03,.045,1)
    for i,path in enumerate(paths):
        im=bpy.data.images.load(str(path),check_existing=False)
        im.scale(tw,th)
        cell=np.empty(tw*th*4,np.float32)
        im.pixels.foreach_get(cell)
        row,col=divmod(i,cols)
        x=gap+col*(tw+gap); y=pixels.shape[0]-gap-(row+1)*th-row*gap
        pixels[y:y+th,x:x+tw]=cell.reshape((th,tw,4))
        bpy.data.images.remove(im)
    im=bpy.data.images.new("Review_Overview",width=pixels.shape[1],height=pixels.shape[0],alpha=False)
    im.pixels.foreach_set(pixels.ravel())
    im.filepath_raw=str(OUT/filename); im.file_format="PNG";im.save()
    bpy.data.images.remove(im)


def render(obs, rocket, hashes, only=None):
    camera=studio()
    shots=[]
    # Correct authoring conventions: front=-Y, aft=+Y, right=+X.
    specs=[("01_hero_front_3q","Front three-quarter",35,24,True,obs),
           ("02_front","Front / orthographic",0,0,False,obs),
           ("03_rear","Rear / orthographic",180,0,False,obs),
           ("04_side","Starboard / orthographic",90,0,False,obs),
           ("05_top","Top / orthographic",0,90,False,obs),
           ("06_rear_3q","Rear three-quarter",215,22,True,obs),
           ("07_mounts_underside","Weapon mount inspection",35,-28,True,obs),
           ("08_weapon_detail","Port minigun and rocket pod",35,-12,True,[o for o in obs if o.name.endswith(("Primary_L","Rotor_L","Secondary_L","MountPrimary_L","MountSecondary_L"))]),
           ("09_rocket_visual","Separate spike rocket source",35,18,True,[rocket])]
    records=[]
    for slug,title,az,el,perspective,targets in specs:
        if only and slug!=only:
            continue
        for ob in obs+[rocket]:
            ob.hide_render=(ob not in targets)
        camera_frame(camera,targets,az,el,perspective)
        dest=OUT/(slug+".png")
        bpy.context.scene.render.filepath=str(dest)
        print("REVIEW_RENDER_START "+slug,flush=True)
        bpy.ops.render.render(write_still=True)
        record=image_check(dest); record.update(title=title,azimuth_deg=az,elevation_deg=el,perspective=perspective)
        records.append(record);shots.append(dest)
        print("REVIEW_RENDER_DONE "+slug,flush=True)
    if not only:
        sheet(shots,"Lancer_overview.png")
        # Mandatory diagnostic views use the repository's reviewed Workbench kit.
        for ob in obs:
            ob.hide_render=False
        rocket.hide_render=True
        reviews=runpy.run_path(str(REPO/".claude/skills/blender-mcp/scripts/review_renders.py"))["main"]
        diagnostic=[]
        for mode in ["clay"]:
            diagnostic.append(reviews(targets=[o.name for o in obs],out_dir=str(OUT),label="Lancer_diagnostic",mode=mode,
                                      views=["front_3q","front","right","top","back_3q","low_3q"],
                                      resolution=700,samples=16,keep_views=False))
        write("diagnostic_render_manifest.json",diagnostic)
    manifest={"blender_version":bpy.app.version_string,"engine":bpy.context.scene.render.engine,
              "color_view":bpy.context.scene.view_settings.view_transform,"cpu_thread_limit":2,
              "source_hashes_unchanged":hashes==fingerprint(),"geometry_modified_for_render":False,
              "images":records,"elapsed_seconds":round(time.perf_counter()-T0,3),
              "silhouette_approval":"pending human review","unreal_import_performed":False}
    write("render_manifest"+("_"+only if only else "")+".json",manifest)
    if not only:
        render_wire(obs,rocket,hashes,camera)


def render_wire(obs,rocket,hashes,camera=None):
    """Actual mesh edges as thin independent curves: no wire-modifier miter spikes.

    Curves are temporary diagnostic display helpers, not source asset geometry.
    They have no joins/miters and cannot create the long acute-corner spikes seen
    with Blender's Wireframe modifier on this pre-existing triangulation.
    """
    if camera is None:
        camera=studio()
    sc=bpy.context.scene
    sc.render.engine="BLENDER_WORKBENCH"
    sh=sc.display.shading
    sh.light="STUDIO"
    sh.color_type="OBJECT"
    sh.show_shadows=True
    sh.show_cavity=True
    sh.cavity_type="BOTH"
    sh.show_object_outline=True
    sh.background_type="WORLD"
    sc.world.color=(.045,.055,.065)
    for ob in obs:
        ob.hide_render=False
        ob.color=(.64,.68,.72,1)
    rocket.hide_render=True
    curve=bpy.data.curves.new("Review_ActualEdges","CURVE")
    curve.dimensions="3D"
    curve.resolution_u=1
    curve.bevel_depth=.0025
    curve.bevel_resolution=0
    edge_count=0
    for ob in obs:
        for edge in ob.data.edges:
            spline=curve.splines.new("POLY")
            spline.points.add(1)
            for target,index in zip(spline.points,edge.vertices):
                p=ob.matrix_world@ob.data.vertices[index].co
                target.co=(*p,1)
            edge_count+=1
    overlay=bpy.data.objects.new("Review_EdgeOverlay",curve)
    sc.collection.objects.link(overlay)
    overlay.color=(.008,.012,.016,1)
    sc.render.resolution_x,sc.render.resolution_y=1600,1100
    paths=[]
    views=[("front_3q",35,22,True),("front",0,0,False),("right",90,0,False),
           ("top",0,90,False),("back_3q",215,22,True),("low_3q",325,-18,True)]
    for label,az,el,perspective in views:
        camera_frame(camera,obs,az,el,perspective)
        dest=OUT/("wire_"+label+".png")
        sc.render.filepath=str(dest)
        bpy.ops.render.render(write_still=True)
        paths.append(dest)
    sheet(paths,"Lancer_diagnostic_wireframe_sheet.png")
    write("wire_overlay_manifest.json",{"engine":sc.render.engine,"overlay":"temporary independent edge curves, no geometry miter",
          "edge_count":edge_count,"wire_radius_m":.0025,"source_hashes_unchanged":hashes==fingerprint(),
          "source_geometry_comparison":preserve_geometry_check(),"images":[image_check(p) for p in paths],
          "layout":[v[0] for v in views],"overlay_excluded_from_asset_counts":True})


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage",choices=["validate","render","wire"],required=True)
    parser.add_argument("--only")
    args=parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    hashes=fingerprint()
    result,coll,obs,rocket=load()
    if args.stage=="validate":
        validate(result,coll,obs,rocket,hashes)
    elif args.stage=="render":
        render(obs,rocket,hashes,args.only)
    else:
        render_wire(obs,rocket,hashes)
    if hashes!=fingerprint():
        raise RuntimeError("Source mesh/interface files changed unexpectedly")
