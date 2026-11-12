"""Read-only geometry/attachment QA for the versioned Lancer M02 Blender scene."""
from pathlib import Path
import hashlib
import json
import runpy
import sys
import time

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

PROD=Path(__file__).resolve().parent
ROOT=PROD.parent
REPO=ROOT.parents[3]
OUT=ROOT/"review/M02"
OUT.mkdir(parents=True,exist_ok=True)


def write(name,data):
    (OUT/name).write_text(json.dumps(data,indent=2)+"\n")


def co(value):
    x,y,z=value
    return Vector((y/100,-x/100,z/100))


def run():
    started=time.perf_counter()
    obs=[o for o in bpy.context.scene.objects if o.type=="MESH" and o.get("instance_name")]
    names=[o.name for o in obs]
    a=runpy.run_path(str(REPO/".claude/skills/blender-mcp/scripts/asset_report.py"))["main"](targets=names,tri_budget=60000)
    write("asset_report.json",a)
    qa=runpy.run_path(r"C:\Users\nicho\.agents\skills\blender-asset-qa\scripts\qa_check.py")["run"]
    q=qa(objects=names,profile="game",eps=.001,tol=.002)
    write("geometry_qa.json",q)
    contacts=runpy.run_path(str(REPO/".claude/skills/blender-mcp/scripts/assembly_check.py"))["main"](targets=names,tolerance=.002)
    write("assembly_contacts.json",contacts)
    manifest=json.loads((ROOT/"attachments.json").read_text())
    checks=[]
    for spec in manifest["sockets"]:
        ob=bpy.data.objects["SOCKET_"+spec["parent"]+"_"+spec["name"]]
        forward=co([100*v for v in spec["forward"]]).normalized()
        up=co([100*v for v in spec["up"]]).normalized()
        m=ob.matrix_world.to_3x3()
        err=(ob.location-co(spec["position_cm"])).length
        fd=(m@Vector((0,-1,0))).normalized().dot(forward)
        ud=(m@Vector((0,0,1))).normalized().dot(up)
        checks.append(dict(name=spec["name"],parent=spec["parent"],position_error_m=err,
                           forward_dot=fd,up_dot=ud,determinant=m.determinant(),
                           pass_check=err<1e-6 and fd>.99999 and ud>.99999 and abs(m.determinant()-1)<1e-6))
    vertices=[];faces=[]
    for ob in obs:
        offset=len(vertices)
        vertices.extend(ob.matrix_world@v.co for v in ob.data.vertices)
        faces.extend(tuple(offset+i for i in p.vertices) for p in ob.data.polygons)
    bvh=BVHTree.FromPolygons(vertices,faces,all_triangles=True)
    if q["zfighting"]:
        # Pinpoint local collision positions rather than guessing from a render.
        owner=[]
        for ob in obs:
            owner.extend([ob.name]*len(ob.data.polygons))
        normals=[(vertices[t[1]]-vertices[t[0]]).cross(vertices[t[2]]-vertices[t[0]]).normalized() for t in faces]
        membership=[set(f) for f in faces]
        inside=qa.__globals__["_point_in_tri"]
        pairs=set();samples=[]
        for i,t in enumerate(faces):
            a,b,c=[vertices[k] for k in t];n=normals[i];centre=(a+b+c)/3
            for p in [centre,centre.lerp(a,.6),centre.lerp(b,.6),centre.lerp(c,.6)]:
                for _,_,j,_ in bvh.find_nearest_range(p,.001):
                    if j is None or i==j or membership[i]&membership[j] or n.dot(normals[j])<.999:
                        continue
                    a2,b2,c2=[vertices[k] for k in faces[j]]
                    projection=p-normals[j]*normals[j].dot(p-a2)
                    key=tuple(sorted([i,j]))
                    if key not in pairs and inside(projection,a2,b2,c2,normals[j]):
                        pairs.add(key)
                        samples.append(dict(objects=[owner[i],owner[j]],position_cm=[-p.y*100,p.x*100,p.z*100],normal=list(n)))
        write("overlap_locations.json",samples)
    else:
        # Clear earlier failure evidence from the current report directory.
        write("overlap_locations.json",[])
    rays=[]
    for spec in manifest["sockets"]:
        if not spec["name"].startswith(("Muzzle_","Thruster_")):
            continue
        socket=bpy.data.objects["SOCKET_"+spec["parent"]+"_"+spec["name"]]
        d=co([100*v for v in spec["forward"]]).normalized()
        point,normal,face,distance=bvh.ray_cast(socket.matrix_world.translation+d*.0001,d,100)
        rays.append(dict(name=spec["name"],parent=spec["parent"],clear=point is None,
                         hit_distance_cm=None if point is None else distance*100))
    points=[o.matrix_world@v.co for o in obs for v in o.data.vertices]
    low=[min(p[k] for p in points) for k in range(3)];high=[max(p[k] for p in points) for k in range(3)]
    build=json.loads((OUT/"build_manifest.json").read_text())
    hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(ROOT.glob("*.mesh.json"))+[ROOT/"attachments.json"]}
    report=dict(blender=bpy.app.version_string,assembly_triangles=sum(len(o.data.polygons) for o in obs),
                assembly_instances=len(obs),materials=len({s.material.name for o in obs for s in o.material_slots}),
                dimensions_cm=[(high[1]-low[1])*100,(high[0]-low[0])*100,(high[2]-low[2])*100],
                geometry_qa_pass=q["pass"],geometry_errors=q["errors"],geometry_warnings=q["warnings"],
                contact_pass=contacts["ok"],floating_islands=contacts["floating"],
                attachment_checks=checks,attachment_pass=all(c["pass_check"] for c in checks),
                ray_clearance=rays,ray_clearance_pass=all(r["clear"] for r in rays),
                original_sources_unchanged=hashes==build["m01_hashes_before"],
                unit_positive_scale=all(tuple(o.scale)==(1.,1.,1.) for o in obs),
                all_meshes_have_panel_tint=all(o.data.color_attributes.get("PanelTint") for o in obs),
                final_visual_approval=False,unreal_imported=False,elapsed_seconds=time.perf_counter()-started)
    report["technical_pass"]=all(report[k] for k in ["geometry_qa_pass","contact_pass","attachment_pass","ray_clearance_pass","original_sources_unchanged","unit_positive_scale","all_meshes_have_panel_tint"]) and report["assembly_triangles"]<=60000
    write("validation_summary.json",report)
    print("M02_QA "+json.dumps({k:report[k] for k in ["assembly_triangles","technical_pass","geometry_errors","contact_pass","attachment_pass","ray_clearance_pass"]}),flush=True)
    return report


if __name__=="__main__":
    run()
