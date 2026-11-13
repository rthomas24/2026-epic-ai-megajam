set -e
cd /tmp/rwork
blender -b --python cut.py -- -178 102 -114 2>&1 | grep -E "islands|cut:" 
blender -b --python assemble.py 2>&1 | grep -E "new tris"
blender -b --python cover.py 2>&1 | grep -E "outside"
blender -b --python build1.py 2>&1 | grep -E "removed|filled|boundary|tris"
python3 pack.py | tail -1
blender -b --python build2.py 2>&1 | grep -E "tris"
