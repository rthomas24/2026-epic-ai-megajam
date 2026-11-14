G=/tmp/wt-enemy-attacks/ArtSource/EngCharacters
/home/box/.local/bin/blender -b --factory-startup --python /tmp/wd_atk/py/verify_atk.py -- "$(python3 -c "
import json
G='$G'
print(json.dumps({'out':'/tmp/wd_atk/final/verify.json','clips':[
 {'fbx':'/tmp/wd_atk/final/A_NullGrunt_Charge.fbx','ref':G+'/Crew/export/NullGrunt/SK_NullGrunt.fbx','kind':'run','inplace_speed':303.5294},
 {'fbx':'/tmp/wd_atk/final/A_NullGrunt_Charge_RootMotion.fbx','ref':G+'/Crew/export/NullGrunt/SK_NullGrunt.fbx','kind':'run','inplace_speed':0.0},
 {'fbx':'/tmp/wd_atk/final/A_NullHeavy_ShieldBash.fbx','ref':G+'/Crew/export/NullHeavy/SK_NullHeavy.fbx','kind':'bash','track':['lowerarm_l','hand_l']},
 {'fbx':'/tmp/wd_atk/final/A_Warden_GroundSlam.fbx','ref':G+'/Warden/export/SK_Warden.fbx','kind':'slam','track':['hand_l','hand_r'],'contact_tol':2.5}]}))")" 2>&1 | grep -E "^VER|Error|Traceback" -A2
