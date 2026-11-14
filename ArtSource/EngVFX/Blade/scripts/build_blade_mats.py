# Triad Drift #163: the energy blade's new beam (mesh import + materials) and the VFX material of its swing sheet.
# Run inside a RENDERING editor (materials do not compile under -nullrhi):
#   python ArtSource/EngVFX/scripts/run_editor_py.py <port> ArtSource/EngVFX/Blade/scripts/build_blade_mats.py
# Creates NEW assets only (the first beam, M_TD_BladeBeam and SM_TD_BladeBeam, are kept as they are):
#   /Game/TriadDrift/Gameplay/Props/Blade/SM_TD_BladeBeam_V2      (from ArtSource/EngVFX/Blade/meshes/SM_TD_BladeBeam_V2)
#   /Game/TriadDrift/Gameplay/Props/Blade/M_TD_BladeBeamCore_V2   slot 0: white-hot core, saturated body, fresnel edge
#   /Game/TriadDrift/Gameplay/Props/Blade/M_TD_BladeBeamGlow_V2   slot 1: soft glow shell (no hard silhouette)
#   /Game/TriadDrift/Gameplay/VFX/Materials/M_TD_VFX_BladeSweep   the swing sheet ribbon (bright at the tip edge)
#   /Game/TriadDrift/Gameplay/VFX/Materials/M_TD_VFX_BladeEmber   a tiny hot ember with a soft tail
# Both beam materials: BladeColor (vector, set by the code to the pilot's squad colour), Intensity, and a slow energy
# noise travelling up the blade, a 28 + 38 Hz flicker, a hotter base at the emitter. Rebuilt in place when re-run.
import os
import traceback

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
BLADE = '/Game/TriadDrift/Gameplay/Props/Blade'
VMAT = '/Game/TriadDrift/Gameplay/VFX/Materials'
NOISE = '/Game/TriadDrift/Gameplay/VFX/Textures/T_TD_VFX_Noise'
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FBX = os.path.join(PROJ, 'ArtSource', 'EngVFX', 'Blade', 'meshes', 'SM_TD_BladeBeam_V2', 'SM_TD_BladeBeam_V2.fbx')
log = []
DO_MESH = globals().get('DO_MESH', True)
ONLY_MATS = globals().get('ONLY_MATS')


def _ci(n):
    ci = unreal.CustomInput()
    ci.set_editor_property('input_name', n)
    return ci


def material(folder, name, blend, two_sided, body, params, mesh=True, particle=False, pan=((3.0, 1.0), (-0.9, 0.0), (7.0, 2.0), (-2.1, 0.13), 0.08)):
    path = folder + '/' + name
    if EAL.does_asset_exist(path):
        mat = unreal.load_asset(path)
        MEL.delete_all_material_expressions(mat)
    else:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('blend_mode', {'additive': unreal.BlendMode.BLEND_ADDITIVE}.get(blend, unreal.BlendMode.BLEND_TRANSLUCENT))
    mat.set_editor_property('two_sided', two_sided)
    if particle:
        for flag in ('used_with_niagara_sprites', 'used_with_niagara_ribbons', 'used_with_niagara_mesh_particles'):
            mat.set_editor_property(flag, True)

    def node(cls, x, y):
        return MEL.create_material_expression(mat, cls, x, y)

    uv = node(unreal.MaterialExpressionTextureCoordinate, -1600, 0)
    tm = node(unreal.MaterialExpressionTime, -1600, 100)
    inputs = [('UV', uv, ''), ('T', tm, '')]
    yy = 300
    for pname, default in params.items():
        if isinstance(default, tuple):
            sp = node(unreal.MaterialExpressionVectorParameter, -1600, yy)
            sp.set_editor_property('default_value', unreal.LinearColor(*(list(default) + [1.0])[:4]))
        else:
            sp = node(unreal.MaterialExpressionScalarParameter, -1600, yy)
            sp.set_editor_property('default_value', float(default))
        sp.set_editor_property('parameter_name', pname)
        inputs.append((pname, sp, ''))
        yy += 100
    if mesh:
        nw = node(unreal.MaterialExpressionVertexNormalWS, -1600, yy)
        cv = node(unreal.MaterialExpressionCameraVectorWS, -1600, yy + 100)
        inputs += [('NW', nw, ''), ('CV', cv, '')]
    if particle:
        pc = node(unreal.MaterialExpressionParticleColor, -1600, yy + 200)
        pc4 = node(unreal.MaterialExpressionAppendVector, -1400, yy + 200)
        MEL.connect_material_expressions(pc, 'RGB', pc4, 'A')
        MEL.connect_material_expressions(pc, 'A', pc4, 'B')
        age = node(unreal.MaterialExpressionParticleRelativeTime, -1600, yy + 300)
        rnd = node(unreal.MaterialExpressionParticleRandom, -1600, yy + 400)
        inputs += [('PC', pc4, ''), ('Age', age, ''), ('Rnd', rnd, '')]
    # two noise taps: the second one distorted by the first (a slow travelling energy along U)
    noise = unreal.load_asset(NOISE)
    (ta, pa, tb, pb, dist) = pan
    cua = node(unreal.MaterialExpressionCustom, -1200, 800)
    cua.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    cua.set_editor_property('inputs', [_ci('UV'), _ci('T')])
    cua.set_editor_property('code', 'return UV*float2(%f,%f) + T*float2(%f,%f);' % (ta[0], ta[1], pa[0], pa[1]))
    MEL.connect_material_expressions(uv, '', cua, 'UV')
    MEL.connect_material_expressions(tm, '', cua, 'T')
    sa = node(unreal.MaterialExpressionTextureSample, -950, 800)
    sa.set_editor_property('texture', noise)
    sa.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    MEL.connect_material_expressions(cua, '', sa, 'UVs')
    cub = node(unreal.MaterialExpressionCustom, -700, 800)
    cub.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    cub.set_editor_property('inputs', [_ci('UV'), _ci('T'), _ci('NA')])
    cub.set_editor_property('code', 'return UV*float2(%f,%f) + T*float2(%f,%f) + (NA.rg-0.5)*%f;' % (tb[0], tb[1], pb[0], pb[1], dist))
    MEL.connect_material_expressions(uv, '', cub, 'UV')
    MEL.connect_material_expressions(tm, '', cub, 'T')
    MEL.connect_material_expressions(sa, 'RGB', cub, 'NA')
    sb = node(unreal.MaterialExpressionTextureSample, -450, 800)
    sb.set_editor_property('texture', noise)
    sb.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    MEL.connect_material_expressions(cub, '', sb, 'UVs')
    inputs += [('NA', sa, 'RGBA'), ('NB', sb, 'RGBA')]

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
    return mat


# Shared: facing (1 facing the camera, 0 at the silhouette), the travelling energy, the flicker, the hot base, and the
# squad colour pushed to full saturation (Lead, round 2: saturated at the edge, never pastel).
# Round 2: TRANSLUCENT, not additive, so the colour holds against a bright sky as well as in the dark.
COMMON = '''
float facing = saturate(abs(dot(normalize(NW), normalize(CV))));
float u = saturate(UV.x);
float n = NA.r*0.55 + NB.g*0.45;
float fl = 0.95 + 0.035*sin(T*175.93) + 0.025*sin(T*238.76 + 1.3);
float basek = 1.0 + BaseHeat*exp(-u*24.0);
float3 col = pow(max(BladeColor.rgb, 0.0001), SatPow);
'''

# The wedge's V runs round its section: |sin(2 pi V)| is 0 at mid-depth on both faces and 1 at the edge and the spine.
# Near-white only in the inner third of the depth; saturated colour out to the edge; a fresnel rim in colour.
CORE = COMMON + '''
float d = abs(sin(UV.y*6.2831853));
float coreMask = (1.0 - smoothstep(CoreW, CoreW + 0.28, d))*(0.75 + 0.25*facing);
float energy = 0.80 + 0.40*n;
float3 body = col*BodyK*energy*(1.0 + EdgeK*pow(1.0 - facing, 2.0));
float3 white = float3(0.97, 0.98, 1.0)*WhiteK*(0.94 + 0.12*n);
float3 c = lerp(body, white, coreMask);
return float4(c*fl*basek*Intensity, 1.0);
'''

GLOW = COMMON + '''
float g = pow(facing, GlowFalloff);
float streak = 0.70 + 0.60*n;
float a = saturate(GlowA*g*streak*(1.0 + 0.6*(basek - 1.0)));
return float4(col*GlowK*fl*Intensity, a);
'''

# The swing sheet: a ribbon whose width runs from the hilt side to the tip side. A hot crescent: saturated colour,
# strongest at the tip edge with a near-white line on it, gone towards the hilt; particle alpha (tip speed at the
# moment it was laid down, faded over its 0.2 s life) fades it. Translucent so it stays saturated against the sky.
SWEEP = '''
float v = FlipV > 0.5 ? 1.0 - UV.y : UV.y;
float n = NA.r*0.55 + NB.g*0.45;
float tipEdge = pow(saturate(v), 2.0);
float outer = 1.0 - smoothstep(0.92, 1.0, v);
float inner = smoothstep(0.05, 0.45, v);
float streak = 0.6 + 0.8*n;
float3 col = pow(max(PC.rgb, 0.0001), 1.8);
float hot = pow(saturate(v), 18.0)*outer;
float3 c = lerp(col*Bright, float3(0.97, 0.98, 1.0)*Bright*0.9, hot*0.8);
float a = saturate((tipEdge*streak*inner*outer*OpacK + hot*0.6)*PC.a);
return float4(c*Intensity, a);
'''

EMBER = '''
float2 p = UV*2.0 - 1.0; float r = length(p); float m = saturate(1.0 - r);
float hot = pow(m, 6.0);
float3 col = max(PC.rgb, 0.0);
float3 c = col*m*m*0.9 + lerp(col, float3(1,1,1), 0.75)*hot*1.4;
return float4(c*Intensity, PC.a);
'''

BEAM_PARAMS_CORE = {'BladeColor': (1.0, 0.17, 0.84), 'Intensity': 1.0, 'SatPow': 1.8, 'CoreW': 0.16, 'BodyK': 1.5, 'WhiteK': 2.2, 'EdgeK': 1.2, 'BaseHeat': 1.0}
BEAM_PARAMS_GLOW = {'BladeColor': (1.0, 0.17, 0.84), 'Intensity': 1.0, 'SatPow': 1.8, 'GlowK': 2.2, 'GlowA': 0.7, 'GlowFalloff': 1.8, 'BaseHeat': 1.0}

mats = {}
for _nm, _args in (('M_TD_BladeBeamCore_V2', (BLADE, 'translucent', False, CORE, BEAM_PARAMS_CORE)),
                   ('M_TD_BladeBeamGlow_V2', (BLADE, 'translucent', False, GLOW, BEAM_PARAMS_GLOW)),
                   ('M_TD_VFX_BladeSweep', (VMAT, 'translucent', True, SWEEP, {'Intensity': 1.0, 'FlipV': 0.0, 'Bright': 2.4, 'OpacK': 1.1})),
                   ('M_TD_VFX_BladeEmber', (VMAT, 'additive', True, EMBER, {'Intensity': 1.0}))):
    if ONLY_MATS and _nm not in ONLY_MATS:
        continue
    try:
        part = _nm.startswith('M_TD_VFX_')
        mats[_nm] = material(_args[0], _nm, *_args[1:4], params=_args[4], mesh=not part, particle=part,
                             pan=((1.0, 4.0), (-1.6, 0.0), (2.0, 9.0), (-2.7, 0.0), 0.06) if part else ((3.0, 1.0), (-0.9, 0.0), (7.0, 2.0), (-2.1, 0.13), 0.08))
    except Exception:
        log.append(_nm + ' FAILED ' + traceback.format_exc()[-700:])

if DO_MESH:
    try:
        before = set(EAL.list_assets(BLADE, False, False))
        t = unreal.AssetImportTask()
        t.filename = FBX
        t.destination_path = BLADE
        t.destination_name = 'SM_TD_BladeBeam_V2'
        t.automated = True
        t.replace_existing = True
        t.save = False
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
        sm = unreal.load_asset(BLADE + '/SM_TD_BladeBeam_V2')
        ns = sm.get_editor_property('nanite_settings')
        ns.enabled = False
        sm.set_editor_property('nanite_settings', ns)
        core = unreal.load_asset(BLADE + '/M_TD_BladeBeamCore_V2')
        glow = unreal.load_asset(BLADE + '/M_TD_BladeBeamGlow_V2')
        slots = sm.get_editor_property('static_materials')
        names = [str(s.get_editor_property('material_slot_name')) for s in slots]
        for i, s in enumerate(slots):
            sn = names[i].lower()
            sm.set_material(i, glow if 'glow' in sn else core)
        b = sm.get_bounding_box()
        sm.modify()
        EAL.save_loaded_asset(sm)
        log.append('SM_TD_BladeBeam_V2 slots %s min %s max %s tris %d' % (names, b.min, b.max, sm.get_num_triangles(0)))
        # the importer makes materials of its own (named after the Blender slots): delete only what this import created
        for p in EAL.list_assets(BLADE, False, False):
            if p not in before and not p.split('.')[0].endswith('SM_TD_BladeBeam_V2'):
                log.append('import extra %s deleted=%s' % (p, EAL.delete_asset(p)))
    except Exception:
        log.append('MESH FAILED ' + traceback.format_exc()[-900:])
"\n".join(log)
