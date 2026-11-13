"""Five blockout review views, not final presentation renders. No asset remodeling."""
import bpy,json,os,math
from pathlib import Path
from mathutils import Vector
OWN=Path(__file__).resolve().parent.parent
def main():
    assert os.environ.get('TD_LANCER_BLOCKOUT_CLEARED')=='RENDER'
    assert Path(bpy.data.filepath).resolve()==(OWN/'source/armor/Blockout02/Lancer_Armor_Blockout_v02.blend').resolve()
    out=OWN/'review/Blockout02'
    assert not out.exists();out.mkdir(parents=True)
    scene=bpy.context.scene;scene.render.engine='BLENDER_WORKBENCH'
    scene.render.threads_mode='FIXED';scene.render.threads=2
    scene.render.resolution_x=720;scene.render.resolution_y=960;scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG'
    scene.display.shading.light='STUDIO';scene.display.shading.color_type='MATERIAL'
    scene.display.shading.show_shadows=True;scene.display.shading.show_cavity=True
    scene.display.shading.background_type='WORLD';scene.world.color=(.035,.045,.055)
    helper=bpy.data.collections.new('REVIEW_ONLY');scene.collection.children.link(helper)
    data=bpy.data.cameras.new('ReviewCamera');camera=bpy.data.objects.new('ReviewCamera',data);helper.objects.link(camera);scene.camera=camera
    data.type='ORTHO';data.ortho_scale=2.18;target=Vector((0,0,.91))
    views=[('front',(0,-4,.91)),('side',(4,0,.91)),('back',(0,4,.91)),('top',(0,-.001,5)),('beauty',(3,-4,2.3))]
    outputs=[]
    for name,pos in views:
        camera.location=pos;look=Vector((0,0,.9)) if name!='top' else Vector((0,0,0))
        camera.rotation_euler=(look-camera.location).to_track_quat('-Z','Y').to_euler()
        scene.render.filepath=str(out/('Lancer_Blockout02_'+name+'.png'))
        assert bpy.ops.render.render(write_still=True)=={'FINISHED'}
        outputs.append(dict(view=name,path=scene.render.filepath,bytes=Path(scene.render.filepath).stat().st_size))
    (out/'views.json').write_text(json.dumps(dict(stage='Studio-color PRIMARY FORM REVIEW; not final beauty/PBR approval',resolution=[720,960],views=outputs),indent=2)+'\n')
    print('LANCER_BLOCKOUT_RENDER_OK '+str(out))
if __name__=='__main__':main()
