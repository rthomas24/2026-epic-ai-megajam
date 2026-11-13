"""One Lead-scheduled M03 structural or refined validation/review process."""
from pathlib import Path
import json,runpy,sys,os
import bpy
P=Path(__file__).resolve().parent;ROOT=P.parents[1];OUT=ROOT/('review/M03/StageA02' if '--a02' in sys.argv else 'review/M03/Review01');os.environ['LANCER_M03_REVIEW_DIR']=str(OUT);OUT.mkdir(parents=True,exist_ok=True)
structural='--structural' in sys.argv
env=runpy.run_path(r'C:/Users/nicho/.agents/skills/blender-core/scripts/env_report.py')['run']()
(OUT/'environment.json').write_text(json.dumps(env,indent=2))
runpy.run_path(str(P/'build_m03.py'))['build'](refined=not structural)
(OUT/'source_record.json').write_text(json.dumps(dict(source_file=bpy.data.filepath,stage='Structural A02' if '--a02' in sys.argv else 'M03'),indent=2))
qa=runpy.run_path(str(P/'qa_m03.py'))['run']()
if qa['technical_pass'] and not structural:
    bpy.context.scene['stage']='Lancer_M03_Production_Review01_visual_approval_pending'
    bpy.data.orphans_purge(do_recursive=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(P/'Lancer_M03_v01.blend'))
runpy.run_path(str(ROOT/'review/M03/Review01/render_m03.py'))['run'](full=qa['technical_pass'],structural=structural)
if not qa['technical_pass']:
    raise RuntimeError('Lancer M03 technical gate failed; source and diagnostic renders retained')
