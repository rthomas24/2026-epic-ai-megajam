import bpy,sys
pre = sys.argv[sys.argv.index("--")+1]
for o in bpy.data.objects:
    if o.type=='MESH' and o.name.startswith(pre):
        c=[v.co for v in o.data.vertices]
        print("BB %-22s x[%.3f %.3f] y[%.3f %.3f] z[%.3f %.3f] t%d" % (o.name,min(v.x for v in c),max(v.x for v in c),min(v.y for v in c),max(v.y for v in c),min(v.z for v in c),max(v.z for v in c), len(o.data.polygons)))
