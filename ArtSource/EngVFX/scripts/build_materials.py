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

# ---------------------------------------------------------------- Grok Bot flipbooks (accepted AI-generated textures)
# Sources are in git under ArtSource/EngAI/VFX/<Kind>/r1/ (prompts, seeds, licence in each SOURCE.md); this worktree
# imports from the Lead's copy in Saved/GrokVFX. Imported as NEW assets T_TD_VFX_Grok_*; nothing existing is replaced.
GROK = {'T_TD_VFX_Grok_Explosion_8x8': 'Explosion/r1/T_VFX_Explosion_8x8.png',
        'T_TD_VFX_Grok_Smoke_8x8': 'Smoke/r1/T_VFX_Smoke_8x8.png',
        'T_TD_VFX_Grok_Fire_8x8': 'Fire/r1/T_VFX_Fire_8x8.png',
        'T_TD_VFX_Grok_ShockwaveRing': 'Shockwave/r1/T_VFX_ShockwaveRing.png'}
_proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
for _tn, _rel in GROK.items():
    if EAL.does_asset_exist(TEX + '/' + _tn):
        continue
    _src = next((c for c in (os.path.join(_proj, 'ArtSource', 'EngAI', 'VFX', _rel), os.path.join(_proj, 'Saved', 'GrokVFX', _rel)) if os.path.exists(c)), None)
    if not _src:
        log.append('GROK SOURCE MISSING ' + _rel)
        continue
    task = unreal.AssetImportTask()
    task.filename = _src
    task.destination_path = TEX
    task.destination_name = _tn
    task.automated = True
    task.replace_existing = False
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    _t = unreal.load_asset(TEX + '/' + _tn)
    if _t:
        _t.set_editor_property('srgb', True)
        _t.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_BC7)
        _t.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_EFFECTS)
        EAL.save_loaded_asset(_t)
        log.append(_tn + ' imported')

# Flipbook materials: name -> (texture, columns, rows, frame position expression using Age / Rnd, loops).
# The material body gets FA and FB (the two neighbouring frames, RGBA) and blends them by frac({FP}).
FLIP = {'M_TD_VFX_FlipExplosion': ('T_TD_VFX_Grok_Explosion_8x8', 8, 8, 'Age*62.0', False),
        'M_TD_VFX_FlipSmoke': ('T_TD_VFX_Grok_Smoke_8x8', 8, 8, 'frac(Age*0.45 + Rnd)*64.0', True),
        'M_TD_VFX_FlipRing': ('T_TD_VFX_Grok_ShockwaveRing', 1, 1, '0.0', False)}

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
    # ------------------------------------------------------------------ round 3
    # Mesh bolt (SM_TD_VFX_Bolt, V runs tail -> head). Fres = 0 facing the camera, 1 at the silhouette, so the bolt has
    # a white-hot centre and a coloured rim from every direction, including straight from behind.
    'M_TD_VFX_Bolt': ('additive', True, 1.0, 2.0, (0.0, 1.6), (0.0, 2.7), 0.06, '''
float f = saturate(1-Fres); float lum = dot(PC.rgb, float3(0.3,0.5,0.2));
float n = N.g*0.6 + N.r*0.4;
float body = pow(f, 1.4); float core = pow(f, 4.5);
float head = lerp(0.45, 1.0, smoothstep(0.15, 0.85, UV.y));
float3 c = PC.rgb*body*(0.55 + n*0.9) + lerp(PC.rgb, float3(1,1,1), 0.9)*core*2.4*max(lum, 0.25);
return float4(c*head*Intensity, PC.a);''', {}),

    # Mesh flame (SM_TD_VFX_Flame, V runs nozzle -> tip): engine exhaust, motor flames, muzzle cones.
    'M_TD_VFX_Flame': ('additive', True, 1.0, 2.0, (0.0, -1.9), (0.0, -3.1), 0.09, '''
float f = saturate(1-Fres); float v = UV.y; float lum = dot(PC.rgb, float3(0.3,0.5,0.2));
float n = N.g*0.65 + N.r*0.35;
float fadeTip = pow(saturate(1-v), 1.25); float fadeIn = smoothstep(0.0, 0.05, v);
float body = pow(f, 1.15)*(0.40 + n*1.05)*fadeTip;
float core = pow(f, 3.2)*pow(saturate(1-v*1.2), 1.8);
float dia = Diamonds*pow(abs(sin(v*3.14159*5.5 - 0.4)), 5.0)*pow(saturate(1-v), 1.2)*pow(f, 1.6);
float3 hot = lerp(PC.rgb, float3(1,1,1), 0.9);
float3 c = PC.rgb*body*1.25 + hot*(core*2.6 + dia*3.2)*max(lum, 0.25);
return float4(c*fadeIn*Intensity, PC.a);''', {'Diamonds': 0.0}),

    # Explosion fire: black-body ramp over the particle's life, white-hot -> yellow -> orange -> deep red, eroding to nothing.
    'M_TD_VFX_Fireball': ('additive', True, 0.55, 0.9, (0.02, 0.07), (-0.04, 0.11), 0.18, PRE + '''
float n = N.r*0.55 + N.g*0.45;
float body = m*1.4 + (n-0.5)*1.0*(0.5+Age);
float thr = lerp(0.15, 1.05, pow(Age, 1.1));
float a = smoothstep(thr, thr+0.22, body)*smoothstep(0, 0.25, m);
float heat = saturate((body-thr)*1.5)*(1-Age*0.8);
heat = heat*heat*(1.0-0.35*Age) + (1-Age)*(1-Age)*0.35*a;
float3 ramp = lerp(float3(0.35,0.02,0.0), float3(1.0,0.28,0.03), saturate(heat*2.5));
ramp = lerp(ramp, float3(1.0,0.72,0.25), saturate(heat*2.5-1.0));
ramp = lerp(ramp, float3(1.0,0.97,0.85), saturate(heat*3.0-2.0));
return float4(ramp*PC.rgb*a*(0.6 + heat*3.2)*Intensity, PC.a);''', {}),

    # Smoke that starts with glowing embers inside it and cools to a dark drifting cloud.
    'M_TD_VFX_SmokeHot': ('translucent', True, 0.5, 0.8, (0.015, 0.03), (-0.02, 0.05), 0.14, PRE + '''
float n = N.r*0.6 + N.a*0.4;
float body = m*1.3 + (n-0.5)*0.9;
float thr = lerp(0.10, 0.80, Age);
float a = smoothstep(thr, thr+0.40, body)*smoothstep(0, 0.32, m);
float shade = 0.35 + 0.65*N.r;
float ember = pow(saturate(1-Age*2.2), 2.0)*pow(n, 2.0)*3.0;
return float4((PC.rgb*shade + float3(1.0,0.35,0.06)*ember)*Intensity, saturate(a*PC.a));''', {}),

    # A thin, crisp shockwave ring with a faint filled wake.
    'M_TD_VFX_Shock': ('additive', True, 1.3, 2.1, (0.02, 0.03), (-0.03, 0.02), 0.08, PRE + '''
float ring = smoothstep(0.86, 0.945, r)*smoothstep(1.0, 0.955, r);
float wake = pow(smoothstep(0.35, 0.93, r), 5.0)*smoothstep(1.0, 0.95, r)*0.10;
float3 c = lerp(PC.rgb, float3(1,1,1), 0.45)*ring*(0.45 + 1.1*N.g) + PC.rgb*wake;
return float4(c*Intensity, PC.a);''', {}),

    # Debris chunk (mesh, opaque): near-black metal with edges and cracks that glow orange and cool to red, then dark.
    'M_TD_VFX_Debris': ('opaque', True, 1.0, 2.0, (0, 0), (0, 0), 0.0, '''
float heat = pow(saturate(1-Age*1.25), 2.0);
float3 base = float3(0.02,0.018,0.02)*(0.5 + N.r);
float3 glow = lerp(float3(0.8,0.06,0.0), float3(1.0,0.55,0.12), heat)*(pow(Fres, 1.3)*2.5 + pow(N.b, 3.0)*2.0)*heat*4.0;
return float4(base + glow*PC.rgb*Intensity, 1);''', {}),

    # Shield hit: a ring front spreading over a hexagon lattice (use on SM_TD_VFX_Disc lying on the hit plane, or a sprite).
    'M_TD_VFX_Hex': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float2 q = abs(p)*HexScale; float2 s = float2(1.0, 1.7320508);
float2 a2 = fmod(q, s) - s*0.5; float2 b2 = fmod(q + s*0.5, s) - s*0.5;
float2 g = abs(dot(a2,a2) < dot(b2,b2) ? a2 : b2);
float d = max(dot(g, s*0.5), g.x);
float edge = smoothstep(0.40, 0.49, d);
float front = 0.08 + Age*1.0;
float band = smoothstep(front-0.38, front, r)*smoothstep(front+0.05, front, r);
float fill = smoothstep(front, front-0.6, r)*0.22*(1-Age);
float fadeR = smoothstep(1.0, 0.82, r);
float a = (edge*(band*1.7 + fill) + band*0.22 + pow(band, 6.0)*0.9)*fadeR*(1-Age*Age);
float3 c = PC.rgb*a*1.5 + lerp(PC.rgb, float3(1,1,1), 0.8)*pow(saturate(a), 2.5)*1.3 + PC.rgb*pow(m, 6.0)*(1-Age)*(1-Age)*1.6;
return float4(c*Intensity, PC.a);''', {'HexScale': 11.0}),

    # Grok Bot's explosion flipbook (8x8, one shot over the particle's life), additive, soft-masked at the quad edge.
    'M_TD_VFX_FlipExplosion': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float4 t = lerp(FA, FB, frac({FP}));
float l = dot(t.rgb, float3(0.3,0.5,0.2));
float edge = smoothstep(1.0, 0.80, r);
float3 c = t.rgb*t.a*(1.0 + 2.2*l*l);
return float4(c*PC.rgb*edge*Intensity, PC.a);''', {}),

    # Grok Bot's smoke flipbook (8x8 loop): dark billowing smoke, lit from inside by embers while young.
    'M_TD_VFX_FlipSmoke': ('translucent', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float4 t = lerp(FA, FB, frac({FP}));
float l = dot(t.rgb, float3(0.3,0.5,0.2));
float edge = smoothstep(1.0, 0.55, r);
float a = saturate(t.a*1.6)*edge*smoothstep(0.0, 0.12, Age)*pow(saturate(1-Age), 1.2);
float ember = pow(saturate(1-Age*2.6), 2.0);
float3 c = PC.rgb*(0.25 + 2.2*l) + float3(1.0,0.38,0.07)*ember*(0.3 + 2.0*l);
return float4(c*Intensity, saturate(a*PC.a));''', {}),

    # Grok Bot's shockwave ring (still), additive.
    'M_TD_VFX_FlipRing': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float4 t = FA;
float3 c = t.rgb*t.a*lerp(float3(1,1,1), PC.rgb, 0.65)*1.6;
return float4(c*smoothstep(1.0, 0.9, r)*Intensity, PC.a);''', {}),
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
    mat.set_editor_property('blend_mode', {'additive': unreal.BlendMode.BLEND_ADDITIVE, 'opaque': unreal.BlendMode.BLEND_OPAQUE}.get(blend, unreal.BlendMode.BLEND_TRANSLUCENT))
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

    if name in FLIP:
        tname, cols, rows, fp, loops = FLIP[name]
        body = body.replace('{FP}', fp)
        ftex = unreal.load_asset(TEX + '/' + tname)
        nfr = cols*rows
        for which, key in ((0, 'FA'), (1, 'FB')):
            cu = node(unreal.MaterialExpressionCustom, -1100, 1300 + which*200)
            cu.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT2)
            cu.set_editor_property('inputs', [_ci(n) for n in ('UV', 'Age', 'Rnd')])
            nxt = ('fmod(f0 + 1.0, %d.0)' % nfr) if loops else ('min(f0 + 1.0, %d.0)' % (nfr - 1))
            cu.set_editor_property('code', 'float fp = %s; float f0 = min(floor(fp), %d.0); float f = %s; float2 cell = float2(fmod(f, %d.0), floor(f/%d.0)); return (cell + clamp(UV, 0.004, 0.996))/float2(%d.0, %d.0);'
                                   % (fp, nfr - 1, nxt if which else 'f0', cols, cols, cols, rows))
            for n, src in (('UV', uv), ('Age', age), ('Rnd', rnd)):
                MEL.connect_material_expressions(src, '', cu, n)
            smp = node(unreal.MaterialExpressionTextureSample, -850, 1300 + which*200)
            smp.set_editor_property('texture', ftex)
            MEL.connect_material_expressions(cu, '', smp, 'UVs')
            inputs.append((key, smp, 'RGBA'))

    if 'Fres' in body:   # Fres: 0 where the surface faces the camera, 1 at the silhouette (mesh particles).
        # Computed from the absolute facing so it also holds for back faces and for meshes mirrored by a negative scale.
        nw = node(unreal.MaterialExpressionVertexNormalWS, -1500, 1100)
        cv = node(unreal.MaterialExpressionCameraVectorWS, -1500, 1200)
        inputs += [('NW', nw, ''), ('CV', cv, '')]
        body = 'float Fres = 1.0 - abs(dot(normalize(NW), normalize(CV)));\n' + body.strip()

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
    if blend != 'opaque':
        MEL.connect_material_property(al, '', unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    log.append(name + ' ok exprs=' + str(MEL.get_num_material_expressions(mat)))


ONLY_MATS = globals().get('ONLY_MATS')   # optional list of names, set by a wrapper line: rebuild just these
for _name, _spec in SPECS.items():
    if ONLY_MATS and _name not in ONLY_MATS:
        continue
    try:
        build(_name, _spec)
    except Exception as ex:
        import_tb = traceback.format_exc()
        log.append(_name + ' FAILED ' + import_tb[-600:])
    await asyncio.sleep(0)
"\n".join(log)

# ---------------------------------------------------------------- material instances (variants)
MICS = {'M_TD_VFX_Ribbon_Clean': ('M_TD_VFX_Ribbon', {'Wisp': 0.12}),
        'M_TD_VFX_Jet_Rev': ('M_TD_VFX_Jet', {'FlipV': 1.0}),
        'M_TD_VFX_Ribbon_Trail': ('M_TD_VFX_Ribbon', {'Wisp': 0.0}),
        'M_TD_VFX_Flame_Boost': ('M_TD_VFX_Flame', {'Diamonds': 1.0})}
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
