"""One scheduled headless build / QA / hero review pass. No Unreal work."""
from pathlib import Path
import json
import runpy
import bpy

PROD=Path(__file__).resolve().parent
ROOT=PROD.parent
OUT=ROOT/"review/M02"
OUT.mkdir(parents=True,exist_ok=True)
env=runpy.run_path(r"C:\Users\nicho\.agents\skills\blender-core\scripts\env_report.py")["run"]()
(OUT/"environment.json").write_text(json.dumps(env,indent=2))
runpy.run_path(str(PROD/"build_m02.py"))["build"]()
result=runpy.run_path(str(PROD/"qa_m02.py"))["run"]()
if result["technical_pass"]:
    bpy.context.scene["review_stage"]="technically_verified_visual_approval_pending"
    bpy.data.orphans_purge(do_recursive=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(PROD/"Lancer_M02_v03.blend"))
# A passing candidate earns the full multi-angle review in the same bounded
# process; a failed candidate only renders the diagnostic hero, never finalizes.
runpy.run_path(str(OUT/"render_m02.py"))["run"](hero_only=not result["technical_pass"])
