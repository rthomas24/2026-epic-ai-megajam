# Triad Drift ENG-37: builds the VFX texture + materials under /Game/TriadDrift/Gameplay/VFX.
# Run inside the editor:  python ArtSource/EngVFX/scripts/run_editor_py.py 8017 ArtSource/EngVFX/scripts/build_materials.py
# Idempotent: an existing material is rebuilt in place (its expressions are replaced), never deleted.
# Every material is unlit, reads Particle Color (rgb = tint, a = fade), Particle Relative Time (age 0..1) and
# Particle Random, and reaches exactly zero at the quad edge (no visible billboard squares).
import os
import traceback

ROOT = '/Game/TriadDrift/Gameplay/VFX'
MAT = ROOT + '/Materials'
TEX = ROOT + '/Textures'
SRC = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), 'ArtSource', 'EngVFX', 'textures', 'T_TD_VFX_Noise.png')
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
log = []


def _ci(n):
    ci = unreal.CustomInput()
    ci.set_editor_property('input_name', n)
    return ci

# ---------------------------------------------------------------- texture
if not EAL.does_asset_exist(TEX + '/T_TD_VFX_Noise'):
    task = unreal.AssetImportTask()
    task.filename = SRC
    task.destination_path = TEX
    task.destination_name = 'T_TD_VFX_Noise'
    task.automated = True
    task.replace_existing = False
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
noise = unreal.load_asset(TEX + '/T_TD_VFX_Noise')
noise.set_editor_property('srgb', False)
noise.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_BC7)
noise.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_EFFECTS)
EAL.save_loaded_asset(noise)

PRE = 'float2 p = UV*2-1; float r = length(p); float m = saturate(1-r); float lum = dot(PC.rgb, float3(0.3,0.5,0.2));\n'

# name: (blend, uses_noise, tileA, tileB, panA(x,y), panB(x,y), distort, hlsl body, extra scalar params {name: default})
SPECS = {
    'M_TD_VFX_Glow': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float g = m*m*(3-2*m); g = pow(g, Falloff);
float core = pow(m, 9);
float3 c = PC.rgb*(g*0.50 + core*1.0) + lerp(PC.rgb, float3(1,1,1), 0.7)*core*core*0.7*max(lum, 0.25);
return float4(c*Intensity, PC.a);''', {'Falloff': 2.2}),

    'M_TD_VFX_Fire': ('additive', True, 0.55, 0.9, (0.03, 0.11), (-0.05, 0.19), 0.16, PRE + '''
float n = N.r*0.55 + N.g*0.45;
float body = m*1.35 + (n-0.5)*0.95*(0.45+Age);
float thr = lerp(0.2, 0.98, pow(Age, 1.25));
float a = smoothstep(thr, thr+0.24, body)*smoothstep(0, 0.28, m);
float heat = saturate((body-thr)*1.7)*(1-Age*0.65);
float3 c = lerp(PC.rgb*float3(0.55,0.3,0.3), PC.rgb, saturate(heat*2));
c = lerp(c, lerp(PC.rgb, float3(1,1,1), 0.3), saturate(heat*2-1));
return float4(c*a*Intensity*(0.5+heat*0.6), PC.a);''', {}),

    'M_TD_VFX_Smoke': ('translucent', True, 0.5, 0.8, (0.02, 0.05), (-0.03, 0.08), 0.14, PRE + '''
float n = N.r*0.6 + N.a*0.4;
float body = m*1.3 + (n-0.5)*0.9;
float thr = lerp(0.12, 0.85, Age);
float a = smoothstep(thr, thr+0.38, body)*smoothstep(0, 0.32, m);
float shade = 0.45 + 0.55*N.r;
return float4(PC.rgb*shade*Intensity, saturate(a*PC.a));''', {}),

    'M_TD_VFX_Spark': ('additive', False, 1, 1, (0, 0), (0, 0), 0, '''
float x = abs(UV.x*2-1); float y = abs(UV.y*2-1); float lum = dot(PC.rgb, float3(0.3,0.5,0.2));
float w = pow(saturate(1-x), 2.5); float l = pow(saturate(1-y), 1.3);
float3 c = PC.rgb*w*l*1.3 + lerp(PC.rgb, float3(1,1,1), 0.85)*pow(w, 5)*l*1.6*max(lum, 0.25);
return float4(c*Intensity, PC.a);''', {}),

    'M_TD_VFX_Ribbon': ('additive', True, 1.0, 1.6, (-1.7, 0.0), (-2.6, 0.13), 0.08, '''
float v = abs(UV.y*2-1); float e = saturate(1-v); float lum = dot(PC.rgb, float3(0.3,0.5,0.2));
float prof = pow(e, 2.2); float core = pow(e, 7);
float wob = lerp(1.0, 0.25 + N.g*1.5, Wisp);
float3 c = PC.rgb*prof*wob*1.1 + lerp(PC.rgb, float3(1,1,1), 0.85)*core*1.1*max(lum, 0.25)*lerp(1.0, 0.6+N.r*0.8, Wisp);
return float4(c*Intensity, PC.a);''', {'Wisp': 0.6}),

    'M_TD_VFX_Ring': ('additive', True, 1.3, 2.1, (0.02, 0.03), (-0.03, 0.02), 0.1, PRE + '''
float ring = pow(smoothstep(0.45, 0.88, r), 3.0)*smoothstep(1.0, 0.86, r);
float br = 0.10 + 2.0*pow(N.g, 1.6);
float3 c = PC.rgb*ring*br + lerp(PC.rgb, float3(1,1,1), 0.8)*pow(ring, 4)*0.7*max(lum, 0.25);
return float4(c*Intensity, PC.a);''', {}),

    'M_TD_VFX_Flare': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float ang = atan2(p.y, p.x);
float spikes = pow(abs(cos(ang*2 + Rnd*6.283)), 26) + 0.55*pow(abs(cos(ang*3 + Rnd*12.0)), 44);
float s = spikes*pow(m, 1.4); float core = pow(m, 5);
float3 c = PC.rgb*(s*1.2 + core*1.4 + pow(m, 2.5)*0.3) + lerp(PC.rgb, float3(1,1,1), 0.85)*pow(m, 10)*2.0*max(lum, 0.25);
return float4(c*Intensity, PC.a);''', {}),

    'M_TD_VFX_Shield': ('additive', True, 1.6, 2.4, (0.01, 0.02), (-0.02, 0.03), 0.05, PRE + '''
float w = sin(r*24 - Age*17)*0.5 + 0.5;
float front = smoothstep(Age*1.05 + 0.08, Age*1.05 - 0.12, r);
float a = pow(w, 3)*m*m*(0.25 + N.b*1.6)*(1-Age)*front + pow(m, 7)*(1-Age)*(1-Age)*0.6;
float3 c = PC.rgb*a*1.3 + lerp(PC.rgb, float3(1,1,1), 0.8)*pow(saturate(a), 3)*0.6;
return float4(c*Intensity, PC.a);''', {}),

    # A long teardrop flame for engine jets and motor flames. Sprite is stretched along V (velocity aligned);
    # V = 0 is the nozzle end when FlipV = 0.
    'M_TD_VFX_Jet': ('additive', True, 0.45, 0.7, (0.0, -1.3), (0.05, -2.1), 0.10, '''
float y = lerp(UV.y, 1-UV.y, FlipV); float x = abs(UV.x*2-1); float lum = dot(PC.rgb, float3(0.3,0.5,0.2));
float n = N.g*0.65 + N.r*0.35;
float width = (0.30 + 0.70*smoothstep(0.0, 0.30, y))*lerp(0.80, 0.10, pow(y, 0.7))*(0.85 + n*0.30);
float e = saturate(1 - x/width); e = e*e*(3-2*e);
float fadeL = smoothstep(0.0, 0.25, y)*pow(saturate(1-y), 1.6);
float body = e*(0.35 + n*1.1);
float a = smoothstep(0.04, 0.95, body)*fadeL;
float core = pow(e, 4)*fadeL*(0.6 + n*0.6);
float3 c = PC.rgb*a*1.0 + lerp(PC.rgb, float3(1,1,1), 0.85)*core*core*1.5*max(lum, 0.25);
return float4(c*Intensity, PC.a);''', {'FlipV': 0.0}),
}


def build(name, spec):
    blend, uses_noise, tile_a, tile_b, pan_a, pan_b, distort, body, extra = spec
    path = MAT + '/' + name
    if EAL.does_asset_exist(path):
        mat = unreal.load_asset(path)
        MEL.delete_all_material_expressions(mat)
    else:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MAT, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE if blend == 'additive' else unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('two_sided', True)
    for flag in ('used_with_niagara_sprites', 'used_with_niagara_ribbons', 'used_with_niagara_mesh_particles'):
        mat.set_editor_property(flag, True)

    def node(cls, x, y):
        return MEL.create_material_expression(mat, cls, x, y)

    uv = node(unreal.MaterialExpressionTextureCoordinate, -1500, 0)
    pc = node(unreal.MaterialExpressionParticleColor, -1500, 200)
    age = node(unreal.MaterialExpressionParticleRelativeTime, -1500, 400)
    rnd = node(unreal.MaterialExpressionParticleRandom, -1500, 500)
    pc4 = node(unreal.MaterialExpressionAppendVector, -1300, 200)
    MEL.connect_material_expressions(pc, 'RGB', pc4, 'A')
    MEL.connect_material_expressions(pc, 'A', pc4, 'B')
    inten = node(unreal.MaterialExpressionScalarParameter, -1500, 600)
    inten.set_editor_property('parameter_name', 'Intensity')
    inten.set_editor_property('default_value', 1.0)

    inputs = [('UV', uv, ''), ('PC', pc4, ''), ('Age', age, ''), ('Rnd', rnd, ''), ('Intensity', inten, '')]
    yy = 700
    for pname, default in extra.items():
        sp = node(unreal.MaterialExpressionScalarParameter, -1500, yy)
        sp.set_editor_property('parameter_name', pname)
        sp.set_editor_property('default_value', default)
        inputs.append((pname, sp, ''))
        yy += 100

    if uses_noise:
        t = node(unreal.MaterialExpressionTime, -1500, 900)
        cua = node(unreal.MaterialExpressionCustom, -1100, 800)
        cua.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT2)
        cua.set_editor_property('inputs', [_ci(n) for n in ('UV', 'Rnd', 'T')])
        cua.set_editor_property('code', 'return UV*%f + float2(Rnd*7.31, Rnd*3.17) + T*float2(%f,%f);' % (tile_a, pan_a[0], pan_a[1]))
        for n, src in (('UV', uv), ('Rnd', rnd), ('T', t)):
            MEL.connect_material_expressions(src, '', cua, n)
        sa = node(unreal.MaterialExpressionTextureSample, -850, 800)
        sa.set_editor_property('texture', noise)
        sa.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        MEL.connect_material_expressions(cua, '', sa, 'UVs')
        cub = node(unreal.MaterialExpressionCustom, -600, 800)
        cub.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT2)
        cub.set_editor_property('inputs', [_ci(n) for n in ('UV', 'Rnd', 'T', 'NA')])
        cub.set_editor_property('code', 'return UV*%f + float2(Rnd*2.71, Rnd*5.13) + T*float2(%f,%f) + (NA.rg-0.5)*%f;' % (tile_b, pan_b[0], pan_b[1], distort))
        for n, src, out in (('UV', uv, ''), ('Rnd', rnd, ''), ('T', t, ''), ('NA', sa, 'RGB')):
            MEL.connect_material_expressions(src, out, cub, n)
        sb = node(unreal.MaterialExpressionTextureSample, -350, 800)
        sb.set_editor_property('texture', noise)
        sb.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        MEL.connect_material_expressions(cub, '', sb, 'UVs')
        inputs.append(('N', sb, 'RGBA'))

    main = node(unreal.MaterialExpressionCustom, -100, 200)
    main.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    main.set_editor_property('inputs', [_ci(n) for n, _, _ in inputs])
    main.set_editor_property('code', body.strip())
    main.set_editor_property('description', name)
    for n, src, out in inputs:
        MEL.connect_material_expressions(src, out, main, n)
    rgb = node(unreal.MaterialExpressionComponentMask, 200, 150)
    for ch, on in (('r', True), ('g', True), ('b', True), ('a', False)):
        rgb.set_editor_property(ch, on)
    al = node(unreal.MaterialExpressionComponentMask, 200, 300)
    for ch, on in (('r', False), ('g', False), ('b', False), ('a', True)):
        al.set_editor_property(ch, on)
    MEL.connect_material_expressions(main, '', rgb, '')
    MEL.connect_material_expressions(main, '', al, '')
    MEL.connect_material_property(rgb, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(al, '', unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    log.append(name + ' ok exprs=' + str(MEL.get_num_material_expressions(mat)))


for _name, _spec in SPECS.items():
    try:
        build(_name, _spec)
    except Exception as ex:
        import_tb = traceback.format_exc()
        log.append(_name + ' FAILED ' + import_tb[-600:])
    await asyncio.sleep(0)
"\n".join(log)

# ---------------------------------------------------------------- material instances (variants)
MICS = {'M_TD_VFX_Ribbon_Clean': ('M_TD_VFX_Ribbon', {'Wisp': 0.12}),
        'M_TD_VFX_Jet_Rev': ('M_TD_VFX_Jet', {'FlipV': 1.0})}
mic_log = []
for _n, (_parent, _params) in MICS.items():
    _p = MAT + '/' + _n
    if EAL.does_asset_exist(_p):
        _mi = unreal.load_asset(_p)
    else:
        _mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(_n, MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(_mi, unreal.load_asset(MAT + '/' + _parent))
    for _k, _v in _params.items():
        MEL.set_material_instance_scalar_parameter_value(_mi, _k, _v)
    MEL.update_material_instance(_mi)
    EAL.save_loaded_asset(_mi)
    mic_log.append(_n + ' ok')
"\n".join(log + mic_log)
