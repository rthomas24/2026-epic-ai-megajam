# Triad Drift ENG-37 round 10f (Lead: v1 read as a solid pale ball with an outline and greyed the view from inside):
# M_TD_RegionHaze2 = additive (adds the region's colour, never darkens or greys), Density 0.09, a much softer falloff
# (nothing in the outer 40% of the radius as seen from outside). Was: M_TD_RegionHaze, a cheap local tint for the space regions on a scaled sphere.
#   python ArtSource/EngVFX/scripts/run_editor_py.py <port> ArtSource/EngVFX/scripts/build_regionhaze.py
# Translucent, unlit, two-sided. Opacity = Density x (path length through the sphere, approximated by the facing |N.V|: thick
# in the middle, nothing at the silhouette, so no hard edge) x a slow world-space noise (one 1K texture, two samples)
# x depth fade (soft where it meets geometry) x camera-distance fade (gone within NearFade of the camera, so a ship
# flying through the shell never sees a wall). Parameters: Color (vector), Density (0..1, default 0.25), NoiseScale
# (world cm per noise tile), NearFade (cm). Inside the sphere the far wall still reads as a tinted backdrop.
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MAT = '/Game/TriadDrift/Gameplay/VFX/Materials'
NAME = 'M_TD_RegionHaze2'
noise = unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Textures/T_TD_VFX_Noise')
path = MAT + '/' + NAME
out = []
if EAL.does_asset_exist(path):
    out.append('EXISTS: not rebuilt in place (round 9 lesson: deleting expressions of a loaded material can assert)')
    mat = unreal.load_asset(path)
else:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, MAT, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('used_with_niagara_mesh_particles', True)
    mat.set_editor_property('two_sided', True)
    mat.set_editor_property('used_with_static_lighting', False)

    def node(cls, x, y):
        return MEL.create_material_expression(mat, cls, x, y)

    def par(name, x, y, v):
        if isinstance(v, tuple):
            p = node(unreal.MaterialExpressionVectorParameter, x, y)
            p.set_editor_property('default_value', unreal.LinearColor(*v))
        else:
            p = node(unreal.MaterialExpressionScalarParameter, x, y)
            p.set_editor_property('default_value', v)
        p.set_editor_property('parameter_name', name)
        return p

    color = par('Color', -1600, 0, (0.55, 0.85, 1.0, 1.0))
    dens = par('Density', -1600, 120, 0.09)
    nscale = par('NoiseScale', -1600, 240, 200000.0)
    nearf = par('NearFade', -1600, 360, 6000.0)
    wp = node(unreal.MaterialExpressionWorldPosition, -1600, 480)
    t = node(unreal.MaterialExpressionTime, -1600, 600)
    # noise UVs: world XY and YZ over NoiseScale, slowly drifting (cheap: two samples, no 3D noise)
    uvc = node(unreal.MaterialExpressionCustom, -1300, 500)
    uvc.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    ins = []
    for n in ('WP', 'S', 'T'):
        ci = unreal.CustomInput(); ci.set_editor_property('input_name', n); ins.append(ci)
    uvc.set_editor_property('inputs', ins)
    uvc.set_editor_property('code', 'float3 q = WP/S; return float4(q.xy + T*float2(0.004, 0.002), q.yz*1.7 - T*float2(0.003, 0.005));')
    MEL.connect_material_expressions(wp, '', uvc, 'WP'); MEL.connect_material_expressions(nscale, '', uvc, 'S'); MEL.connect_material_expressions(t, '', uvc, 'T')
    m01 = node(unreal.MaterialExpressionComponentMask, -1100, 450)
    m23 = node(unreal.MaterialExpressionComponentMask, -1100, 600)
    for m, on in ((m01, ('r', 'g')), (m23, ('b', 'a'))):
        for ch in 'rgba':
            m.set_editor_property(ch, ch in on)
        MEL.connect_material_expressions(uvc, '', m, '')
    samples = []
    for i, m in enumerate((m01, m23)):
        s = node(unreal.MaterialExpressionTextureSample, -900, 450 + 160 * i)
        s.set_editor_property('texture', noise)
        s.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        MEL.connect_material_expressions(m, '', s, 'UVs')
        samples.append(s)
    df = node(unreal.MaterialExpressionDepthFade, -900, 800)
    df.set_editor_property('fade_distance_default', 3000.0)
    nw = node(unreal.MaterialExpressionVertexNormalWS, -900, 950)
    cv = node(unreal.MaterialExpressionCameraVectorWS, -900, 1050)
    cp = node(unreal.MaterialExpressionCameraPositionWS, -900, 1150)
    main = node(unreal.MaterialExpressionCustom, -500, 400)
    main.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    names = ['C', 'D', 'NF', 'A', 'B', 'DF', 'NW', 'CV', 'CP', 'WP']
    ins = []
    for n in names:
        ci = unreal.CustomInput(); ci.set_editor_property('input_name', n); ins.append(ci)
    main.set_editor_property('inputs', ins)
    main.set_editor_property('code', '''
float facing = abs(dot(normalize(NW), normalize(CV)));
float body = pow(smoothstep(0.4, 1.0, facing), 2.0);    // 10f: nothing in the outer part as seen from outside: no outline
float n = A.r*0.6 + B.g*0.4;
float nearf = saturate((length(WP - CP) - NF*0.25)/NF);  // nothing within a quarter of NearFade, full beyond 1.25x
float a = saturate(D*body*(0.55 + 0.9*n)*DF*nearf);
return float4(C.rgb*(0.85 + 0.3*n)*a, a);''')   # additive: colour x amount
    for n, src, o in (('C', color, ''), ('D', dens, ''), ('NF', nearf, ''), ('A', samples[0], 'RGBA'), ('B', samples[1], 'RGBA'),
                      ('DF', df, ''), ('NW', nw, ''), ('CV', cv, ''), ('CP', cp, ''), ('WP', wp, '')):
        MEL.connect_material_expressions(src, o, main, n)
    rgb = node(unreal.MaterialExpressionComponentMask, -200, 350)
    al = node(unreal.MaterialExpressionComponentMask, -200, 500)
    for ch in 'rgba':
        rgb.set_editor_property(ch, ch != 'a'); al.set_editor_property(ch, ch == 'a')
    MEL.connect_material_expressions(main, '', rgb, ''); MEL.connect_material_expressions(main, '', al, '')
    MEL.connect_material_property(rgb, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    pass   # additive: no opacity
    MEL.recompile_material(mat)
    out.append('saved' if EAL.save_loaded_asset(mat) else 'SAVE FAILED')
tx = noise
out.append('noise %dx%d' % (tx.blueprint_get_size_x(), tx.blueprint_get_size_y()))
try:
    st = MEL.get_statistics(mat)
    out.append('stats: ps=%s vs=%s samplers=%s' % (st.num_pixel_shader_instructions, st.num_vertex_shader_instructions, st.num_samplers))
except Exception as ex:
    out.append('stats unavailable: %s' % ex)
"\n".join(out)
