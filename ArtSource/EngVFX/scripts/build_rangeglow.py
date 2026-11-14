# Triad Drift ENG-37 round 10e: M_TD_VFX_RangeGlow, a camera-facing glow of CONSTANT ANGULAR SIZE, computed in the material
# (Engine.Owner.LODDistance read 0 for these systems, so a size grown in Niagara never grew).
#   python ArtSource/EngVFX/scripts/run_editor_py.py <port> ArtSource/EngVFX/scripts/build_rangeglow.py   (needs a renderer)
# The sprite is authored LARGE (its full size is what the glow may reach at the far end); the material draws a soft disc of
# radius Angle x PixelDepth (world cm), so it covers the same angle at any distance until it fills the sprite, then
# shrinks with distance like any object. A short horizontal streak (Streak 0..1) and a white pin-point centre.
# Additive, unlit; parameters Angle (radians of radius, default 0.006), Streak (default 0), Intensity.
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MAT = '/Game/TriadDrift/Gameplay/VFX/Materials'
NAME = 'M_TD_VFX_RangeGlow'
path = MAT + '/' + NAME
out = []
if EAL.does_asset_exist(path):
    out.append('EXISTS: not rebuilt in place')
else:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, MAT, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('two_sided', True)
    for flag in ('used_with_niagara_sprites', 'used_with_niagara_ribbons', 'used_with_niagara_mesh_particles'):
        mat.set_editor_property(flag, True)

    def node(cls, x, y):
        return MEL.create_material_expression(mat, cls, x, y)

    def par(name, x, y, v):
        p = node(unreal.MaterialExpressionScalarParameter, x, y)
        p.set_editor_property('parameter_name', name)
        p.set_editor_property('default_value', v)
        return p

    uv = node(unreal.MaterialExpressionTextureCoordinate, -1200, 0)
    pc = node(unreal.MaterialExpressionParticleColor, -1200, 100)
    ps = node(unreal.MaterialExpressionParticleSize, -1200, 200)
    pd = node(unreal.MaterialExpressionPixelDepth, -1200, 300)
    ang = par('Angle', -1200, 400, 0.006)
    stk = par('Streak', -1200, 500, 0.0)
    inten = par('Intensity', -1200, 600, 1.0)
    main = node(unreal.MaterialExpressionCustom, -600, 200)
    main.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ins = []
    for n in ('UV', 'PC', 'PS', 'PD', 'Ang', 'Stk', 'I'):
        ci = unreal.CustomInput(); ci.set_editor_property('input_name', n); ins.append(ci)
    main.set_editor_property('inputs', ins)
    main.set_editor_property('code', '''
float2 p = UV*2.0 - 1.0;
float frac_r = saturate(Ang*PD/max(0.5*PS.x, 1.0));        // the disc radius as a fraction of the sprite half-size
float r = length(p)/max(frac_r, 0.001);
float glow = exp(-r*r*3.0) + 0.35*exp(-r*1.6);
float pin = exp(-r*r*40.0);
float streak = Stk*exp(-abs(p.y)/max(frac_r*0.12, 0.002))*saturate(1.0 - abs(p.x)/max(frac_r*4.0, 0.004));
float3 c = PC.rgb*(glow + streak*0.8) + lerp(PC.rgb, float3(1,1,1), 0.8)*pin*1.2;
return c*PC.a*I*step(abs(p.x), 1.0);''')
    for n, src, o in (('UV', uv, ''), ('PC', pc, ''), ('PS', ps, ''), ('PD', pd, ''), ('Ang', ang, ''), ('Stk', stk, ''), ('I', inten, '')):
        MEL.connect_material_expressions(src, o, main, n)
    MEL.connect_material_property(main, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    out.append('saved' if EAL.save_loaded_asset(mat) else 'SAVE FAILED')
    # instances: the region beacon (0.8 degree disc + short streak) and the Null plume beacon (about 1.1 degrees)
    for mi_name, params in (('MI_TD_VFX_RangeGlow_Beacon', {'Angle': 0.007, 'Streak': 1.0}), ('MI_TD_VFX_RangeGlow_Plume', {'Angle': 0.0095, 'Streak': 0.0})):
        mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(mi_name, MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, mat)
        for k, v in params.items():
            MEL.set_material_instance_scalar_parameter_value(mi, k, v)
        out.append('%s %s' % (mi_name, 'saved' if EAL.save_loaded_asset(mi) else 'SAVE FAILED'))
"\n".join(out)
