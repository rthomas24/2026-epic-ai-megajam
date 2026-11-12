"""Additive UE 5.9 Hero benchmark. Never writes the baseline package."""
import hashlib
import importlib.util
import json
import sys
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue

HERE=Path(__file__).resolve().parent
DEST='/Game/TriadDrift/Environment/SpaceFoundation/HeroM02'
BASE='/Game/TriadDrift/Environment/SpaceFoundation'
ED=ue.EditorAssetLibrary
ML=ue.MaterialEditingLibrary
TOOLS=ue.AssetToolsHelpers.get_asset_tools()
spec=importlib.util.spec_from_file_location('td_hero_baseline_helpers',HERE.parent/'build_native.py')
helpers=importlib.util.module_from_spec(spec);spec.loader.exec_module(helpers)
helpers.DEST=DEST


def save(obj):
    assert obj.get_path_name().startswith(DEST+'/'),obj.get_path_name()
    if not ED.save_loaded_asset(obj,only_if_is_dirty=False):raise RuntimeError('Save failed')


def node(mat,cls,**properties):
    obj=ML.create_material_expression(mat,cls)
    for k,v in properties.items():obj.set_editor_property(k,v)
    return obj


def link(a,b,pin,output=''):
    if not ML.connect_material_expressions(a,output,b,pin):raise RuntimeError('Connection failed: '+pin)


def texture(name,filename,cube=False,max_size=8192):
    task=ue.AssetImportTask()
    for key,value in {'filename':str(HERE/'export'/filename),'destination_path':DEST,
                      'destination_name':name,'automated':True,'replace_existing':False,
                      'save':False,'factory':ue.TextureFactory()}.items():task.set_editor_property(key,value)
    TOOLS.import_asset_tasks([task])
    obj=ED.load_asset(DEST+'/'+name)
    if not obj:raise RuntimeError('Import failed: '+name)
    assert isinstance(obj,ue.TextureCube if cube else ue.Texture2D),obj.get_class().get_name()
    for key,value in {'srgb':not cube,'never_stream':True,
        'lod_group':ue.TextureGroup.TEXTUREGROUP_SKYBOX,
        'max_texture_size':max_size,'filter':ue.TextureFilter.TF_TRILINEAR,
        'compression_settings':ue.TextureCompressionSettings.TC_HDR_COMPRESSED if cube else ue.TextureCompressionSettings.TC_BC7}.items():
        obj.set_editor_property(key,value)
    if not cube:
        obj.set_editor_property('address_x',ue.TextureAddress.TA_WRAP)
        obj.set_editor_property('address_y',ue.TextureAddress.TA_CLAMP)
    save(obj);return obj


def main():
    assert ue.SystemLibrary.get_engine_version().startswith('5.9.0-60351404')
    names=['T_HeroM02_Back','T_HeroM02_Front','T_HeroM02_Stars',
           'M_HeroM02_Nebula','MI_HeroM02_Benchmark','BP_GalaxySkybox_HeroM02']
    for name in names:
        if ED.does_asset_exist(DEST+'/'+name):raise RuntimeError('Refusing existing '+name)
    for file in ('T_HeroM02_Back.png','T_HeroM02_Front.png','T_HeroM02_Stars.hdr'):
        if not (HERE/'export'/file).is_file():raise RuntimeError('Missing authoring input '+file)
    ED.make_directory(DEST)
    back=texture(names[0],'T_HeroM02_Back.png',max_size=8192)
    front=texture(names[1],'T_HeroM02_Front.png',max_size=4096)
    stars=texture(names[2],'T_HeroM02_Stars.hdr',cube=True,max_size=1024)
    mat=TOOLS.create_asset(names[3],DEST,ue.Material,ue.MaterialFactoryNew())
    for key,value in {'shading_model':ue.MaterialShadingModel.MSM_UNLIT,'two_sided':True,'is_sky':True}.items():
        mat.set_editor_property(key,value)
    ray=node(mat,ue.MaterialExpressionCameraVectorWS)
    direction=node(mat,ue.MaterialExpressionCustom,code='return normalize(-Ray);',
                   output_type=ue.CustomMaterialOutputType.CMOT_FLOAT3,
                   inputs=[helpers.custom_input('Ray')],description='World-space sky direction')
    link(ray,direction,'Ray')
    uv=node(mat,ue.MaterialExpressionCustom,
        code='return float2(frac(atan2(D.y,D.x)*0.1591549430918953),acos(clamp(D.z,-1.0,1.0))*0.3183098861837907);',
        output_type=ue.CustomMaterialOutputType.CMOT_FLOAT2,
        inputs=[helpers.custom_input('D')],description='Baseline Hero orientation')
    link(direction,uv,'D')
    gradient=node(mat,ue.MaterialExpressionCustom,
        code='float2 x=ddx(UV),y=ddy(UV);x.x-=round(x.x);y.x-=round(y.x);return float4(x,y);',
        output_type=ue.CustomMaterialOutputType.CMOT_FLOAT4,inputs=[helpers.custom_input('UV')])
    link(uv,gradient,'UV')
    dx=node(mat,ue.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False)
    dy=node(mat,ue.MaterialExpressionComponentMask,r=False,g=False,b=True,a=True)
    link(gradient,dx,'Input');link(gradient,dy,'Input')
    samples=[]
    for parameter,tex in (('BackgroundGas',back),('ForegroundDust',front)):
        sample=node(mat,ue.MaterialExpressionTextureSampleParameter2D,
                    parameter_name=parameter,texture=tex,mip_value_mode=ue.TextureMipValueMode.TMVM_DERIVATIVE)
        link(uv,sample,'UVs');link(dx,sample,'DDX(UVs)');link(dy,sample,'DDY(UVs)');samples.append(sample)
    star_sample=node(mat,ue.MaterialExpressionTextureSampleParameterCube,
        parameter_name='AngularStars',texture=stars,sampler_type=ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    # UE's HDR long-lat importer uses Y-up and atan2(X,-Z). Rotate that basis
    # so source longitude/latitude match the world's Z-up gas and test-star poses.
    cube_basis=node(mat,ue.MaterialExpressionCustom,code='return float3(-D.y,D.z,D.x);',
                    output_type=ue.CustomMaterialOutputType.CMOT_FLOAT3,
                    inputs=[helpers.custom_input('D')],description='Z-up angular star cube basis')
    link(direction,cube_basis,'D');link(cube_basis,star_sample,'UVs')
    combine=node(mat,ue.MaterialExpressionCustom,
        code='float a=saturate(Front.a*DustStrength); float3 gas=Back.rgb*(1-a)+Front.rgb*a*ForegroundScatter; '
             'return max(0,(gas*NebulaIntensity*Tint+Stars*StarIntensity*Back.a*(1-a))*SkyIntensity);',
        output_type=ue.CustomMaterialOutputType.CMOT_FLOAT3,
        inputs=[helpers.custom_input(n) for n in ('Back','Front','Stars','DustStrength','ForegroundScatter','NebulaIntensity','Tint','StarIntensity','SkyIntensity')],
        description='Layered optical-depth extinction and restrained scattered light')
    link(samples[0],combine,'Back','RGBA');link(samples[1],combine,'Front','RGBA')
    link(star_sample,combine,'Stars','RGB')
    for name,value in {'DustStrength':1.0,'ForegroundScatter':1.0,'NebulaIntensity':1.0,
                       'StarIntensity':1.0,'SkyIntensity':1.0}.items():
        parameter=node(mat,ue.MaterialExpressionScalarParameter,parameter_name=name,
                       default_value=value,group='Hero M02')
        link(parameter,combine,name)
    tint=node(mat,ue.MaterialExpressionVectorParameter,parameter_name='NebulaTint',
              default_value=ue.LinearColor(1,1,1,1),group='Hero M02')
    link(tint,combine,'Tint','RGB')
    assert ML.connect_material_property(combine,'',ue.MaterialProperty.MP_EMISSIVE_COLOR)
    ML.layout_material_expressions(mat)
    errors=ML.recompile_material(mat)
    if errors:raise RuntimeError(str(errors))
    save(mat)
    instance=TOOLS.create_asset(names[4],DEST,ue.MaterialInstanceConstant,ue.MaterialInstanceConstantFactoryNew())
    ML.set_material_instance_parent(instance,mat);ML.update_material_instance(instance);save(instance)
    light=json.loads((HERE.parent/'review/build.json').read_text())['lighting_presets']['hero']
    bp=helpers.blueprint_rig('HeroM02',instance,light)
    defaults=ue.get_default_object(bp.generated_class())
    defaults.set_editor_property('tags',[ue.Name('TD.Presentation.Sky')])
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    save(bp)
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    ue.SystemLibrary.execute_console_command(world,'Editor.AsyncTextureCompilationFinishAll')
    ue.SystemLibrary.execute_console_command(world,'Editor.AsyncStaticMeshCompilationFinishAll')
    stats=ML.get_statistics(instance)
    report={'engine':ue.SystemLibrary.get_engine_version(),'assets':[DEST+'/'+n for n in names],
            'shader_errors':list(errors),'lighting':light,
            'material_statistics':{n:stats.get_editor_property(n) for n in ('num_pixel_shader_instructions','num_vertex_shader_instructions','num_samplers','num_pixel_texture_samples')},
            'sky_actor_tag':'TD.Presentation.Sky',
            'baseline_untouched':True,'manual_approval':'pending','integrated':False,'gameplay_validated':False}
    (HERE/'review').mkdir(exist_ok=True)
    (HERE/'review/build.json').write_text(json.dumps(report,indent=2)+'\n')
    ue.log('HERO_M02_BUILD_SUCCESS '+json.dumps(report))


if __name__=='__main__':main()
