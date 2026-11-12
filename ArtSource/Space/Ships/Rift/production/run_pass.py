"""One bounded Lead-scheduled build/QA/review process. No native or gameplay work."""
from pathlib import Path
import json, runpy
import bpy
P=Path(__file__).resolve().parent;OUT=P.parent/'review/M01';OUT.mkdir(parents=True,exist_ok=True)
env=runpy.run_path(r'C:/Users/nicho/.agents/skills/blender-core/scripts/env_report.py')['run']()
(OUT/'environment.json').write_text(json.dumps(env,indent=2))
runpy.run_path(str(P/'build_rift.py'))['build']()
qa=runpy.run_path(str(P/'qa_rift.py'))['run']()
if qa['technical_pass']:
    bpy.context.scene['stage']='technically_verified_refined_foundation_manual_review_pending'
    bpy.data.orphans_purge(do_recursive=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(P/'Rift_M01_v01.blend'))
runpy.run_path(str(OUT/'render_rift.py'))['run'](full=qa['technical_pass'])
