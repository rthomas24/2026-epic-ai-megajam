import json, os, sys
r = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "export", "Pilot.manifest.json")))
print(json.dumps(r["mesh"])); print(r["rest_bone_heads_m"]); print(r["textures"])
for k, v in r["animations"].items(): print(k, json.dumps(v))
