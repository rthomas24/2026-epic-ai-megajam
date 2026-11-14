#!/bin/sh
A=${1:-'{}'}
/home/box/.local/bin/blender -b --factory-startup --python /tmp/wd_atk/py/grunt.py -- "$(python3 -c "
import json,sys
a={'src':'/tmp/wd_atk/raw/gruntD_s4801.fbx','tgt':'/tmp/wt-enemy-attacks/ArtSource/EngCharacters/Crew/export/NullGrunt/SK_NullGrunt.fbx',
 'out':'/tmp/wd_atk/final/A_NullGrunt_Charge.fbx','out_rm':'/tmp/wd_atk/final/A_NullGrunt_Charge_RootMotion.fbx','blend':'/tmp/wd_atk/final/A_NullGrunt_Charge.blend','name':'A_NullGrunt_Charge',
 'a':36,'cyc':24,'N':16,'td_l':0,'td_r':8,'stance':8.5,'span':86,'lift':12,'roll':20,'twist':15,'lean':0,'fist':70}
a.update(json.loads(sys.argv[1])); print(json.dumps(a))" "$A")" > /tmp/wd_atk/final/A_NullGrunt_Charge.log 2>&1
grep -E "RT_OK|RM_OK|Error|Traceback" -A3 /tmp/wd_atk/final/A_NullGrunt_Charge.log | head -20
