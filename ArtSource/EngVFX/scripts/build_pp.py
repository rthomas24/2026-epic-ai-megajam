# Round 9r (ENG-38): M_TD_PP_DescentWhite, a post-process material for the planet-entry cover. Scalar Weight 0..1:
# 0 = a no-op (the scene colour passes through), 1 = the frame lifted towards the white cover (exposure x4, a soft bloom of
# the brights, mixed to near-white), so the cover has no visible step. T1 drives Weight from Descent01 / the cover alpha.
# Run in the editor: python ArtSource/EngVFX/scripts/run_editor_py.py <port> ArtSource/EngVFX/scripts/build_pp.py
PATH = '/Game/TriadDrift/Gameplay/VFX/Materials'
NAME = 'M_TD_PP_DescentWhite'
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
if EAL.does_asset_exist(PATH + '/' + NAME):
    mat = unreal.load_asset(PATH + '/' + NAME); MEL.delete_all_material_expressions(mat)
else:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)
mat.set_editor_property('blendable_location', unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
st = MEL.create_material_expression(mat, unreal.MaterialExpressionSceneTexture, -900, 0)
st.set_editor_property('scene_texture_id', unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
w = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 300)
w.set_editor_property('parameter_name', 'Weight'); w.set_editor_property('default_value', 0.0)
cu = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 100)
cu.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ci = []
for n in ('C', 'W'):
    x = unreal.CustomInput(); x.set_editor_property('input_name', n); ci.append(x)
cu.set_editor_property('inputs', ci)
cu.set_editor_property('code', '''float w = saturate(W);
float3 lifted = 1.0 - exp(-C.rgb*(1.0 + 3.0*w));            // exposure lift
float lum = dot(lifted, float3(0.3, 0.59, 0.11));
float3 bloom = lifted + smoothstep(0.5, 1.0, lum)*w*0.6;     // the brights bloom out
float3 white = lerp(bloom, float3(1, 1, 1), smoothstep(0.55, 1.0, w)*0.92);
return lerp(C.rgb, white, w);''')
MEL.connect_material_expressions(st, 'Color', cu, 'C')
MEL.connect_material_expressions(w, '', cu, 'W')
MEL.connect_material_property(cu, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
'%s saved=%s exprs=%d' % (NAME, EAL.save_loaded_asset(mat), MEL.get_num_material_expressions(mat))
