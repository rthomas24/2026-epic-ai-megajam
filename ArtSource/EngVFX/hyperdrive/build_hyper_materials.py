# Triad Drift: hyperdrive look (Ryan, Nov 13; T7). The rift material: a jagged energy tear with a white-hot rim,
# crawling arcs and a dark depth streaked by rays pulled toward the centre. Original design. Same shader style as the
# effects builder's M_TD_VFX_PortalVortex (round 8, eng/vfx c9153bfb): animated on engine time, PC = Niagara colour,
# PC.a = how open the rift is. UV (0.5, 0.5) is the centre; the mesh's scale gives the tear its shape (taller than wide).
# Run inside the editor:  python ArtSource/EngVFX/scripts/run_editor_py.py <port> ArtSource/EngVFX/hyperdrive/build_hyper_materials.py
# Idempotent: rebuilds the material in place.
import traceback

MAT = '/Game/TriadDrift/Gameplay/VFX/Hyperdrive/Materials'
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
log = []


def _ci(n):
    ci = unreal.CustomInput()
    ci.set_editor_property('input_name', n)
    return ci


SPECS = {
    'M_TD_VFX_HyperRift': ('translucent', """
float2 p = UV*2-1; float ang = atan2(p.y, p.x); float t = Tm;
float jag = 0.055*sin(ang*7.0 + t*7.0) + 0.035*sin(ang*13.0 - t*11.0) + 0.020*sin(ang*29.0 + t*23.0);
float r = length(p)*(1.0 + jag);
float open = saturate(PC.a);
float3 col = PC.rgb*Tint.rgb;
float3 hot = lerp(col, float3(1,1,1), 0.75);
float inside = 1.0 - smoothstep(0.84, 0.88, r);
float depth = pow(saturate(r/0.86), 2.0);
float rays = pow(0.5 + 0.5*sin(ang*22.0 + 2.2/(r + 0.15) - t*9.0), 6.0);
float rays2 = pow(0.5 + 0.5*sin(ang*9.0 - 1.4/(r + 0.25) + t*5.0), 10.0);
float3 c = col*0.015;
c += col*(rays*1.3 + rays2*0.8)*depth*inside;
float rim = exp(-pow((r - 0.87)/0.022, 2.0));
float flick = 0.75 + 0.5*frac(sin(floor(t*30.0)*12.9898 + floor(ang*4.0))*43758.5453);
c += hot*rim*3.2*flick;
float halo = exp(-max(r - 0.87, 0.0)*10.0)*(1.0 - inside);
c += col*halo*0.8;
float arcs = pow(saturate(0.5 + 0.5*sin(ang*5.0 - t*6.0)), 24.0)*exp(-pow((r - 0.94)/0.05, 2.0));
arcs += pow(saturate(0.5 + 0.5*sin(ang*3.0 + t*4.3 + 1.7)), 30.0)*exp(-pow((r - 0.91)/0.04, 2.0));
c += hot*arcs*2.4;
float mask = 1.0 - smoothstep(0.96, 1.0, length(p));
float a = saturate(inside*0.9 + rim + halo*0.35 + arcs)*mask*open;
return float4(c*Intensity*mask*open, a);""", {'Tint': (1.0, 1.0, 1.0)}),
}


def build(name, spec):
    blend, body, extra = spec
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
    pc4 = node(unreal.MaterialExpressionAppendVector, -1300, 200)
    MEL.connect_material_expressions(pc, 'RGB', pc4, 'A')
    MEL.connect_material_expressions(pc, 'A', pc4, 'B')
    inten = node(unreal.MaterialExpressionScalarParameter, -1500, 600)
    inten.set_editor_property('parameter_name', 'Intensity')
    inten.set_editor_property('default_value', 1.0)
    inputs = [('UV', uv), ('PC', pc4), ('Intensity', inten), ('Tm', node(unreal.MaterialExpressionTime, -1500, 400))]
    yy = 700
    for pname, default in extra.items():
        if isinstance(default, tuple):
            sp = node(unreal.MaterialExpressionVectorParameter, -1500, yy)
            sp.set_editor_property('parameter_name', pname)
            sp.set_editor_property('default_value', unreal.LinearColor(*(list(default) + [1.0])[:4]))
        else:
            sp = node(unreal.MaterialExpressionScalarParameter, -1500, yy)
            sp.set_editor_property('parameter_name', pname)
            sp.set_editor_property('default_value', default)
        inputs.append((pname, sp))
        yy += 100
    main = node(unreal.MaterialExpressionCustom, -100, 200)
    main.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    main.set_editor_property('inputs', [_ci(n) for n, _ in inputs])
    main.set_editor_property('code', body.strip())
    main.set_editor_property('description', name)
    for n, src in inputs:
        MEL.connect_material_expressions(src, '', main, n)
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
    ok = EAL.save_loaded_asset(mat)
    log.append('%s %s exprs=%d' % (name, 'saved' if ok else 'SAVE FAILED', MEL.get_num_material_expressions(mat)))


for _name, _spec in SPECS.items():
    try:
        build(_name, _spec)
    except Exception:
        log.append(_name + ' FAILED ' + traceback.format_exc()[-600:])
"\n".join(log)
