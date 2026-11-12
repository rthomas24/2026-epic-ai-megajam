"""Render the current M02 source; studio/camera math reused read-only from M01.

Run only in a Lead-scheduled Blender process. Writes only review/M02/.
"""
from pathlib import Path
import json
import math
import runpy
import sys
import time

import bpy

OUT=Path(__file__).resolve().parent
ROOT=OUT.parents[1]
REPO=ROOT.parents[3]
BASE=runpy.run_path(str(ROOT/"review_lancer.py"))
BASE["sheet"].__globals__["OUT"]=OUT


def run(hero_only=False):
    started=time.perf_counter()
    obs=[o for o in bpy.context.scene.objects if o.type=="MESH" and o.get("instance_name")]
    projectile=bpy.data.objects.new("Review_M02_Projectile",bpy.data.meshes["SM_Lancer_M02_Projectile"])
    bpy.context.scene.collection.objects.link(projectile)
    projectile.hide_render=True
    camera=BASE["studio"]()
    sc=bpy.context.scene
    sc.render.resolution_x,sc.render.resolution_y=1800,1200
    # Neutral studio exposure; coat and metal carry highlights, not bloom.
    sc.view_settings.exposure=-.25
    camera.data.lens=78
    specs=[("01_hero_front_3q","Front three-quarter",35,24,True,obs),
           ("02_front","Front",0,0,False,obs),("03_rear","Rear",180,0,False,obs),
           ("04_side","Starboard",90,0,False,obs),("05_top","Top",0,90,False,obs),
           ("06_rear_3q","Rear three-quarter",215,22,True,obs),
           ("07_underside","Underside and mounts",35,-28,True,obs),
           ("08_weapon_detail","Port weapon modules",35,-10,True,[o for o in obs if o.get("instance_name") in ["Primary_L","Rotor_L","Secondary_L","MountPrimary_L","MountSecondary_L"]]),
           ("09_rocket_visual","Spike rocket visual",35,18,True,[projectile])]
    records=[];paths=[]
    for slug,title,az,el,perspective,targets in specs:
        if hero_only and slug!="01_hero_front_3q":
            continue
        for o in obs+[projectile]:
            o.hide_render=o not in targets
        BASE["camera_frame"](camera,targets,az,el,perspective,margin=1.16)
        path=OUT/(slug+".png")
        sc.render.filepath=str(path)
        print("M02_RENDER "+slug,flush=True)
        bpy.ops.render.render(write_still=True)
        record=BASE["image_check"](path);record.update(title=title)
        records.append(record);paths.append(path)
    if not hero_only:
        BASE["sheet"](paths,"Lancer_M02_overview.png")
        for o in obs:
            o.hide_render=False
        projectile.hide_render=True
        diagnostic=runpy.run_path(str(REPO/".claude/skills/blender-mcp/scripts/review_renders.py"))["main"]
        result=diagnostic(targets=[o.name for o in obs],out_dir=str(OUT),label="Lancer_M02",mode="clay",
                          views=["front_3q","front","right","top","back_3q","low_3q"],resolution=700,samples=16,keep_views=False)
        (OUT/"clay_manifest.json").write_text(json.dumps(result,indent=2))
    report=dict(blender=bpy.app.version_string,engine=sc.render.engine,color_view=sc.view_settings.view_transform,
                images=records,elapsed_seconds=time.perf_counter()-started,render_threads=2,source_revision="M02",
                geometry_modified_for_render=False,final_visual_approval=False,unreal_imported=False)
    (OUT/("render_hero_manifest.json" if hero_only else "render_manifest.json")).write_text(json.dumps(report,indent=2))


if __name__=="__main__":
    run(hero_only="--hero-only" in sys.argv)
