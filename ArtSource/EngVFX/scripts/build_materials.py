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
        'T_TD_VFX_Grok_ShockwaveRing': 'Shockwave/r1/T_VFX_ShockwaveRing.png',
        # r2
        'T_TD_VFX_Grok_EngineExhaust_8x8': 'EngineExhaust/r1/T_VFX_EngineExhaust_8x8.png',
        'T_TD_VFX_Grok_PlasmaBolt': 'PlasmaBolt/r1/T_VFX_PlasmaBolt.png',
        'T_TD_VFX_Grok_ShieldHexRipple_8x8': 'ShieldHexRipple/r1/T_VFX_ShieldHexRipple_8x8.png',
        'T_TD_VFX_Grok_ElectricArc_8x8': 'ElectricArc/r1/T_VFX_ElectricArc_8x8.png',
        'T_TD_VFX_Grok_DebrisChunks_4x4': 'DebrisChunks/r1/T_VFX_DebrisChunks_4x4.png',
        # round 6: descent textures, in git under ArtSource/EngAI/Descent/<Kind>/r1/
        'T_TD_VFX_Grok_PlasmaSheath_8x8': '../Descent/PlasmaSheath/r1/T_Descent_PlasmaSheath_8x8.png',
        'T_TD_VFX_Grok_HeatStreak': '../Descent/HeatStreak/r1/T_Descent_HeatStreak.png',
        'T_TD_VFX_Grok_ShockCone': '../Descent/ShockCone/r1/T_Descent_ShockCone.png',
        'T_TD_VFX_Grok_CloudPunch_8x8': '../Descent/CloudPunch/r1/T_Descent_CloudPunch_8x8.png',
        'T_TD_VFX_Grok_CloudWisp_8x8': '../Descent/CloudWisp/r1/T_Descent_CloudWisp_8x8.png'}
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

# Flipbook materials: name -> (texture, columns, rows, frame position expression using Age / Rnd, loops[, uv expression]).
# The optional uv expression replaces the quad UV inside a frame (e.g. to wrap a side-view streak along a mesh).
# The material body gets FA and FB (the two neighbouring frames, RGBA) and blends them by frac({FP}).
FLIP = {'M_TD_VFX_FlipExplosion': ('T_TD_VFX_Grok_Explosion_8x8', 8, 8, 'Age*62.0', False),
        'M_TD_VFX_FlipSmoke': ('T_TD_VFX_Grok_Smoke_8x8', 8, 8, 'frac(Age*0.45 + Rnd)*64.0', True),
        'M_TD_VFX_FlipRing': ('T_TD_VFX_Grok_ShockwaveRing', 1, 1, '0.0', False),
        'M_TD_VFX_Sheath': ('T_TD_VFX_Grok_PlasmaSheath_8x8', 8, 8, 'frac(Age*3.0 + Rnd)*64.0', True,
                            'float2(0.5 + (abs(UV.x*2.0 - 1.0) - 0.5)*0.9, clamp((1.0 - UV.y)*0.92 + 0.04, 0.004, 0.996))'),
        'M_TD_VFX_HeatStreak': ('T_TD_VFX_Grok_HeatStreak', 1, 1, '0.0', False, 'float2(clamp(1.0 - UV.y, 0.01, 0.99), clamp(UV.x, 0.01, 0.99))'),
        'M_TD_VFX_FlipShockCone': ('T_TD_VFX_Grok_ShockCone', 1, 1, '0.0', False),
        'M_TD_VFX_CloudPunch': ('T_TD_VFX_Grok_CloudPunch_8x8', 8, 8, 'Age*62.0', False),
        'M_TD_VFX_CloudWisp': ('T_TD_VFX_Grok_CloudWisp_8x8', 8, 8, 'frac(Age*0.6 + Rnd)*64.0', True),
        'M_TD_VFX_FlipArc': ('T_TD_VFX_Grok_ElectricArc_8x8', 8, 8, 'frac(Age*1.5 + Rnd)*64.0', True),
        'M_TD_VFX_FlipShield': ('T_TD_VFX_Grok_ShieldHexRipple_8x8', 8, 8, 'Age*62.0', False),
        'M_TD_VFX_FlipDebris': ('T_TD_VFX_Grok_DebrisChunks_4x4', 4, 4, 'floor(Rnd*15.99)', False, 'clamp(UV, 0.07, 0.93)'),
        # the side-view plasma streak read along the bolt's length (V of the mesh): its head-to-tail brightness profile
        'M_TD_VFX_Bolt': ('T_TD_VFX_Grok_PlasmaBolt', 1, 1, '0.0', False, 'float2(clamp(UV.y, 0.01, 0.99), 0.5)'),
        # the exhaust flipbook wrapped round the flame body: nozzle -> tip along V, mirrored round the circumference
        'M_TD_VFX_Flame': ('T_TD_VFX_Grok_EngineExhaust_8x8', 8, 8, 'frac(Age*3.0 + Rnd)*64.0', True,
                           'float2(clamp(UV.y*0.9, 0.004, 0.996), 0.5 + (abs(UV.x*2.0 - 1.0) - 0.5)*0.45)')}

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

    # round 9r: a NEW asset name - rebuilding M_TD_VFX_IceBurst in place crashed the editor twice
    # (DeleteAllMaterialExpressions, Assertion failed: !IsRooted()); the old asset stays, unused.
    # round 9p (Lead: the break flare read like a lens flare): an ice burst - 9 rays of uneven length and width from a hash
    # of the ray index and the particle's random, a soft core; only the asteroid breaks use it.
    'M_TD_VFX_IceShatter': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float ang = atan2(p.y, p.x) + Rnd*6.283;
float seg = ang/6.2831853*9.0; float id = floor(seg); float f = frac(seg) - 0.5;
float h = frac(sin(id*12.9898 + Rnd*78.233)*43758.5453);
float h2 = frac(sin(id*39.346 + Rnd*11.135)*24634.6345);
float len = lerp(0.18, 0.42, h);   // 9q: rays under half (Lead: still read as a lens-flare star)
float wid = lerp(0.06, 0.16, h2);
float ray = exp(-(f*f)/(wid*wid*0.08))*saturate(1.0 - r/len)*smoothstep(0.0, 0.08, r);
float core = exp(-r*r/0.05);
float3 cc = float3(0.80, 0.95, 1.0);   // pale cyan-white core
float3 c = PC.rgb*(ray*1.0 + pow(m, 2.0)*0.15) + cc*core*1.1;
return float4(c*Intensity*m, PC.a);''', {}),

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
float prof = saturate(dot(FA.rgb, float3(0.4,0.4,0.4))*FA.a*1.6);
float body = pow(f, 3.0)*(0.35 + n*0.6)*(0.25 + prof);
float core = pow(f, 7.0)*prof;
float rim = pow(1-f, 2.0)*0.25*prof;
float3 sat = PC.rgb*PC.rgb*1.4;
float3 c = sat*(body*0.9 + rim) + lerp(PC.rgb, float3(1,1,1), 0.8)*core*1.2*max(lum, 0.25);
return float4(c*0.42*Intensity, PC.a);''', {}),

    # Mesh flame (SM_TD_VFX_Flame, V runs nozzle -> tip): engine exhaust, motor flames, muzzle cones.
    'M_TD_VFX_Flame': ('additive', True, 1.0, 2.0, (0.0, -1.9), (0.0, -3.1), 0.09, '''
float f = saturate(1-Fres); float v = UV.y; float lum = dot(PC.rgb, float3(0.3,0.5,0.2));
float n = N.g*0.65 + N.r*0.35;
float fadeTip = pow(saturate(1-v), 1.25); float fadeIn = smoothstep(0.0, 0.05, v);
float ex = dot(lerp(FA, FB, frac({FP})).rgb, float3(0.35,0.35,0.30));
float body = pow(f, 1.15)*(0.40 + n*1.05)*fadeTip*(0.55 + 1.6*ex);
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

    # Grok Bot's hexagon shield ripple (8x8, expands once over the particle's life), tinted.
    'M_TD_VFX_FlipShield': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float4 t = lerp(FA, FB, frac({FP}));
float l = dot(t.rgb, float3(0.33,0.33,0.33))*t.a;
float3 c = PC.rgb*l*1.3 + float3(1,1,1)*pow(l, 3.0)*0.25;
return float4(c*(1-Age*Age)*smoothstep(1.0, 0.9, r)*Intensity, PC.a);''', {}),

    # Grok Bot's debris atlas (4x4, one random chunk per particle): painted metal whose hot edges cool, then it shrinks away.
    'M_TD_VFX_FlipDebris': ('translucent', False, 1, 1, (0, 0), (0, 0), 0, '''
float4 t = FA;
float heat = pow(saturate(1-Age*1.3), 2.0);
float hot = saturate((t.r - t.b)*2.5);
float3 c = t.rgb*(0.30 + 0.5*heat)*lerp(1.0, 0.3, hot*(1-heat)) + t.rgb*hot*heat*5.0;
float a = smoothstep(0.35, 0.6, t.a)*saturate((1-Age)*6.0)*smoothstep(0.02, 0.10, Age);
return float4(c*PC.rgb*Intensity, a*PC.a);''', {}),

    # Grok Bot's shockwave ring (still), additive.
    'M_TD_VFX_FlipRing': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float4 t = FA;
float3 c = t.rgb*t.a*lerp(float3(1,1,1), PC.rgb, 0.65)*1.6;
return float4(c*smoothstep(1.0, 0.9, r)*Intensity, PC.a);''', {}),
    # ------------------------------------------------------------------ round 5
    # Dark body under a tracer (SM_TD_VFX_Bolt, a little fatter than the bolt): the shot reads on a bright nebula too.
    'M_TD_VFX_BoltShadow': ('translucent', False, 1, 1, (0, 0), (0, 0), 0, '''
float f = saturate(1-Fres);
return float4(0.0, 0.0, 0.0, saturate(pow(f, 1.3)*Dark*PC.a));''', {'Dark': 0.55}),

    # Smoke whose eroding edge is lit orange by the fire inside while the puff is young (rim light), then cools to grey.
    'M_TD_VFX_SmokeRim': ('translucent', True, 0.5, 0.8, (0.015, 0.03), (-0.02, 0.05), 0.14, PRE + '''
float n = N.r*0.6 + N.a*0.4;
float body = m*1.3 + (n-0.5)*0.9;
float thr = lerp(0.10, 0.80, Age);
float a = smoothstep(thr, thr+0.40, body)*smoothstep(0, 0.32, m);
float edge = saturate(1.0 - (body-thr)*2.0);
float young = pow(saturate(1-Age*1.25), 1.5);
float shade = 0.30 + 0.70*N.r;
float3 c = PC.rgb*shade + float3(1.0,0.40,0.08)*pow(edge, 2.0)*young*2.4 + float3(1.0,0.75,0.4)*pow(n, 3.0)*young*young*1.5;
return float4(c*Intensity, saturate(a*PC.a));''', {}),

    # Distortion field shell (on a sphere mesh): bright shimmering rim, flowing interference bands, faint body.
    'M_TD_VFX_Field': ('additive', True, 3.0, 5.0, (0.01, 0.035), (-0.02, 0.05), 0.25, '''
float rim = pow(Fres, 2.6)*smoothstep(1.0, 0.88, Fres);   // fades at the very silhouette: hides the sphere mesh's polygon outline
float n = N.r*0.55 + N.g*0.45;
float bands = pow(abs(sin(UV.y*70.0 + n*9.0)), 6.0);
float cells = pow(n, 3.0);
float a = rim*(0.55 + 1.6*cells + 0.7*bands) + (cells*0.35 + bands*0.10)*(0.25 + Fres);
float3 c = PC.rgb*a + lerp(PC.rgb, float3(1,1,1), 0.8)*pow(rim, 3.0)*0.7;
return float4(c*Intensity, PC.a);''', {}),

    # Grok Bot's electric-arc flipbook (8x8 loop), tinted: EMP ring, field arcs, conduit breaks.
    'M_TD_VFX_FlipArc': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float4 t = lerp(FA, FB, frac({FP}));
float l = dot(t.rgb, float3(0.33,0.33,0.33))*t.a;
float3 c = PC.rgb*l*2.2 + float3(1,1,1)*pow(l, 2.5)*1.2;
return float4(c*smoothstep(1.0, 0.85, r)*Intensity, PC.a);''', {}),

    # Transmission / temporal glitch: rows of a quad torn sideways with a red / cyan fringe; a new pattern 14 times per life.
    'M_TD_VFX_Glitch': ('additive', False, 1, 1, (0, 0), (0, 0), 0, '''
float step_t = floor(Age*14.0) + Rnd*17.0;
float row = floor(UV.y*Rows);
float h = frac(sin(row*91.7 + step_t*37.3)*43758.5);
float h2 = frac(sin(row*13.1 + step_t*7.7)*24634.6);
float on = step(0.58, h);
float x = UV.x + (h2-0.5)*0.6;
float w = 0.10 + 0.30*h;
float br = smoothstep(0.5-w-0.02, 0.5-w, x-0.025)*smoothstep(0.5+w+0.02, 0.5+w, x-0.025);
float bg = smoothstep(0.5-w-0.02, 0.5-w, x)*smoothstep(0.5+w+0.02, 0.5+w, x);
float bb = smoothstep(0.5-w-0.02, 0.5-w, x+0.025)*smoothstep(0.5+w+0.02, 0.5+w, x+0.025);
float scan = 0.65 + 0.35*step(0.5, frac(UV.y*Rows*3.0));
float fy = smoothstep(0.0, 0.08, UV.y)*smoothstep(1.0, 0.92, UV.y);
float3 c = float3(br, bg, bb)*on*scan*fy*(0.25 + 0.75*h2);
return float4(c*PC.rgb*Intensity, PC.a);''', {'Rows': 26.0}),

    # Spiral arms turning once per particle life (looping particle): gravity well, portal face.
    'M_TD_VFX_Swirl': ('additive', True, 1.2, 2.0, (0.03, 0.02), (-0.02, 0.04), 0.15, PRE + '''
float ang = atan2(p.y, p.x);
float arms = pow(0.5 + 0.5*sin(ang*Arms + 7.0/(r+0.22) + Age*6.2831853), 3.0);
float a = arms*smoothstep(1.0, 0.55, r)*smoothstep(0.03, 0.30, r)*(0.45 + 1.1*N.g);
float core = pow(saturate(1-r*2.2), 2.0);
float3 c = PC.rgb*a + lerp(PC.rgb, float3(1,1,1), 0.7)*pow(a, 2.5)*0.8 + PC.rgb*core*Core;
return float4(c*Intensity, PC.a);''', {'Arms': 3.0, 'Core': 0.6}),

    # ------------------------------------------------------------------ round 6: planetary descent (Grok Bot's Descent textures)
    # Bow-shock sheath on SM_TD_VFX_Flame used apex FORWARD (the mesh's tip ahead of the nose, its base behind the hull):
    # the plasma-sheath flipbook wrapped round it, hot at the apex, fading to nothing at the open back. Walls seen at a
    # grazing angle are bright, faces turned to the camera dim: from the chase camera it is a tunnel of plasma round the
    # ship. (First version was wide end forward and rim-lit: from behind it read as a white target ring.)
    'M_TD_VFX_Sheath': ('additive', True, 1.0, 2.0, (0.0, 1.9), (0.0, 3.1), 0.09, """
float v = 1.0 - UV.y;
float4 t = lerp(FA, FB, frac({FP}));
float l = dot(t.rgb, float3(0.4,0.4,0.3));
float n = N.g*0.65 + N.r*0.35;
float fadeBack = pow(saturate(1-v*1.6), 1.6); float fadeIn = smoothstep(0.0, 0.05, v);   // gone before the mesh's bulge: no outline from behind
float wall = 0.12 + 0.88*pow(Fres, 0.8);
float body = wall*(0.30 + 1.3*l)*(0.45 + 1.0*n)*fadeBack;
float nose = pow(saturate(1-v*3.0), 2.0);
float3 c = t.rgb*PC.rgb*body*1.5 + PC.rgb*body*0.35 + lerp(PC.rgb, float3(1,1,1), 0.75)*nose*wall*1.4;
return float4(c*fadeIn*Intensity, PC.a);""", {}),

    # Heat streak sheet on a velocity-aligned sprite (the sheet's long axis along the sprite's).
    'M_TD_VFX_HeatStreak': ('additive', False, 1, 1, (0, 0), (0, 0), 0, '''
float4 t = FA;
float l = dot(t.rgb, float3(0.33,0.33,0.33));
float3 c = (t.rgb*PC.rgb*1.5 + PC.rgb*l*0.6)*t.a;
return float4(c*Intensity, PC.a);''', {}),

    # Front-on Mach ring (still), additive.
    'M_TD_VFX_FlipShockCone': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float4 t = FA;
float3 c = t.rgb*t.a*lerp(float3(1,1,1), PC.rgb, 0.5)*1.8;
return float4(c*smoothstep(1.0, 0.9, r)*Intensity, PC.a);''', {}),

    # Flying through a cloud wall (8x8 one shot over the particle's life), translucent white.
    'M_TD_VFX_CloudPunch': ('translucent', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float4 t = lerp(FA, FB, frac({FP}));
float l = dot(t.rgb, float3(0.33,0.33,0.33));
float a = saturate(t.a*1.6)*smoothstep(1.0, 0.75, r);
return float4(PC.rgb*(0.70 + 0.55*l)*Intensity, saturate(a*PC.a));''', {}),

    # Vapour wisps (8x8 loop), translucent white, fading in and out over the particle's life.
    'M_TD_VFX_CloudWisp': ('translucent', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float4 t = lerp(FA, FB, frac({FP}));
float l = dot(t.rgb, float3(0.33,0.33,0.33));
float a = saturate(t.a*1.5)*smoothstep(1.0, 0.7, r)*smoothstep(0.0, 0.15, Age)*saturate((1-Age)*3.0);
return float4(PC.rgb*(0.75 + 0.5*l)*Intensity, saturate(a*PC.a));''', {}),

    # Lead (cloud pass v2): the soft, nearly uniform veil of being INSIDE a cloud. A huge, low-contrast round blob with a
    # gentle slow noise (no curls), translucent, fading in and out over the particle's life; overlapping sprites make a veil.
    'M_TD_VFX_CloudVeil': ('translucent', True, 0.35, 0.6, (0.01, 0.02), (-0.015, 0.01), 0.0, PRE + '''
float n = N.r*0.5 + N.g*0.5;
float g = m*m*(3-2*m);
float a = pow(g, 0.8)*(0.85 + 0.30*(n-0.5))*smoothstep(0.0, 0.25, Age)*saturate((1-Age)*2.5);
return float4(PC.rgb*(0.92 + 0.10*n)*Intensity, saturate(a*PC.a));''', {}),

    # 10b: the approach field's barrier lattice, readable FACE-ON (DomeHex shows it only near the rim): hex edges at 35%
    # across the whole face, a slow bright band sweeping pole to pole, the rim as DomeHex. Additive, for the 1 km shell.
    'M_TD_VFX_FieldHex': ('additive', False, 1, 1, (0, 0), (0, 0), 0, """
float f = saturate(Fres);
float2 q = UV*float2(64.0, 32.0); float2 s = float2(1.0, 1.7320508);
float2 a2 = fmod(q, s) - s*0.5; float2 b2 = fmod(q + s*0.5, s) - s*0.5;
float2 g = abs(dot(a2, a2) < dot(b2, b2) ? a2 : b2);
float d = max(dot(g, s*0.5), g.x);
float edge = smoothstep(0.40, 0.48, d);
float sweep = pow(saturate(1.0 - abs(frac(UV.y*1.0 - Tm*0.05) - 0.5)*6.0), 2.0);
float rim = pow(f, 5.0)*smoothstep(1.0, 0.975, f)*1.4;
float a = edge*(0.35 + 0.65*pow(f, 2.0) + 1.2*sweep) + rim*1.4;
float3 c = PC.rgb*a + lerp(PC.rgb, float3(1,1,1), 0.6)*(rim*0.6 + edge*sweep*0.5);
return float4(c*Intensity, PC.a);""", {}),

    # 10c (Lead: the wall reads but is too heavy): a new asset (rebuilding a loaded material in place can assert): thinner
    # hex lines (edge band 0.45-0.48 instead of 0.40-0.48) and about 40% less light, so the nebula and ships behind read.
    'M_TD_VFX_FieldHex2': ('additive', False, 1, 1, (0, 0), (0, 0), 0, """
float f = saturate(Fres);
float2 q = UV*float2(64.0, 32.0); float2 s = float2(1.0, 1.7320508);
float2 a2 = fmod(q, s) - s*0.5; float2 b2 = fmod(q + s*0.5, s) - s*0.5;
float2 g = abs(dot(a2, a2) < dot(b2, b2) ? a2 : b2);
float d = max(dot(g, s*0.5), g.x);
float edge = smoothstep(0.45, 0.48, d)*smoothstep(0.50, 0.485, d);
float sweep = pow(saturate(1.0 - abs(frac(UV.y*1.0 - Tm*0.05) - 0.5)*6.0), 2.0);
float rim = pow(f, 5.0)*smoothstep(1.0, 0.975, f)*1.4;
float a = (edge*(0.35 + 0.65*pow(f, 2.0) + 1.2*sweep) + rim*1.4)*0.6;
float3 c = PC.rgb*a + lerp(PC.rgb, float3(1,1,1), 0.6)*(rim*0.6 + edge*sweep*0.5)*0.6;
return float4(c*Intensity, PC.a);""", {}),

    # 10f (T2's play): the cloaked Rift. An OVERLAY material for the ship's meshes (UMeshComponent::SetOverlayMaterial while
    # cloaked): the hull darkens to a deep translucent magenta, its silhouette glows magenta, and shimmer bands crawl
    # over it like refraction. Translucent; Tint is a vector parameter (Rift magenta), Cloak 0..1 fades it in and out.
    'M_TD_VFX_CloakOverlay': ('translucent', False, 1, 1, (0, 0), (0, 0), 0, """
float f = saturate(Fres);
float rim = pow(f, 2.2);
float bands = pow(0.5 + 0.5*sin(Tm*7.0 + (UV.x*37.0 + UV.y*23.0)), 6.0);
float a = saturate((0.50 + 0.45*rim + 0.15*bands)*Cloak);
float3 c = Tint.rgb*(0.06 + rim*2.6 + bands*0.9*(0.3 + rim)) + float3(1,1,1)*pow(rim, 6.0)*0.6;
return float4(c*Intensity, a);""", {'Tint': (1.0, 0.10, 0.85), 'Cloak': 1.0}),

    # round 10: a set piece's range beacon (NS_TD_Region_Beacon): soft round core, a short thin horizontal streak, a white
    # pin-point centre. Camera-facing sprite, additive.
    'M_TD_VFX_Beacon': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + '''
float core = pow(m, 2.6);
float streak = pow(saturate(1.0 - abs(p.x)), 1.5)*exp(-abs(p.y)*22.0);
float pin = pow(m, 12.0);
float3 c = PC.rgb*(core*0.9 + streak*0.8) + lerp(PC.rgb, float3(1,1,1), 0.75)*pin*1.4;
return float4(c*Intensity, PC.a);''', {}),

    # Blade swing: a crescent swept left to right across the top half of a camera-facing sprite over the particle's life.
    'M_TD_VFX_Slash': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + """
float ang = atan2(p.x, -p.y);
float lead = lerp(-1.35, 1.35, saturate(Age*1.7));
float behind = lead - ang;
float arc = smoothstep(0.0, 0.07, behind)*saturate(1.0 - behind/1.6)*step(abs(ang), 1.45);
float band = smoothstep(0.60, 0.88, r)*smoothstep(1.0, 0.94, r);
float core = smoothstep(0.87, 0.92, r)*smoothstep(0.985, 0.945, r);
float a = arc*(band*0.6 + core*1.7)*(1.0 - Age*Age);
float3 c = PC.rgb*a*1.4 + lerp(PC.rgb, float3(1,1,1), 0.8)*pow(a, 2.0)*0.9;
return float4(c*Intensity, PC.a);""", {}),

    # round 7: the bow shock's plasma streamers (SM_TD_VFX_BowArc: soft tubes, V = 0 at the shock .. 1 at the tail).
    # White-blue at the shock, the particle colour in the wake, deep orange at the tail; turbulence flows back along it.
    'M_TD_VFX_BowArc': ('additive', True, 1.0, 2.0, (0.0, 2.2), (0.13, 3.4), 0.10, """
float f = saturate(1-Fres); float v = 1.0 - UV.y;   // Unreal flips V on import: the apex is at UV.y = 1
float n = N.g*0.6 + N.r*0.4;
float soft = pow(f, 1.5);
float fadeTail = pow(saturate(1-v), 1.3); float fadeIn = smoothstep(0.0, 0.03, v);
float turb = 0.40 + 1.25*n;
float3 hot = float3(0.70, 0.88, 1.0)*1.25;
float3 ramp = lerp(hot, PC.rgb*1.2, saturate(v*3.0));
ramp = lerp(ramp, PC.rgb*float3(1.0, 0.42, 0.20), saturate(v*1.7 - 0.55));
float core = pow(f, 5.0)*saturate(1 - v*1.4);
float3 c = ramp*soft*turb*fadeTail*0.62 + float3(1,1,1)*core*0.22;
return float4(c*fadeIn*Intensity, PC.a);""", {}),

    # round 8: the hull-repair pickup's icon: a crisp plus with a white-hot rim and a soft halo (camera-facing sprite).
    'M_TD_VFX_Plus': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + """
float2 q = abs(p);
float d = min(max(q.x - 0.17, q.y - 0.66), max(q.y - 0.17, q.x - 0.66));   // < 0 inside the plus
float body = 1.0 - smoothstep(-0.015, 0.015, d);
float rim = (1.0 - smoothstep(0.0, 0.045, abs(d + 0.035)))*body;
float inner = saturate(-d/0.17);
float halo = pow(saturate(1.0 - max(d, 0.0)/0.34), 3.0)*m;
float3 c = PC.rgb*(body*(0.55 + 0.45*inner) + halo*0.55) + lerp(PC.rgb, float3(1,1,1), 0.75)*rim*0.9;
return float4(c*Intensity, PC.a);""", {}),

    # round 8: a soft dark disc drawn under a pickup (translucent, sorted first), so its colour holds over the bright
    # magenta nebula instead of adding up to white.
    # round 8: Rift's portal fill as an animated vortex with depth (Lead, after Ryan saw the ring up close: "a static,
    # cloudy purple texture"). Works on a particle (the disc mesh of NS_TD_Rift_RingIdle) AND straight on a static mesh:
    # it animates on engine time (Tm), its colour is PC x Tint (PC is white on a static mesh) and its strength is
    # PortalPower x PC.a. UV (0.5, 0.5) = centre, a UV radius of 0.5 = the rim. Translucent, nearly opaque, so it hides
    # whatever fill is under it. Spiral arms that twist tighter toward the centre and turn, a glow that falls off into a
    # dark centre (the depth), a white-hot rim, and sparks spiralling into the centre - all in the shader: no particles.
    'M_TD_VFX_PortalVortex': ('translucent', False, 1, 1, (0, 0), (0, 0), 0, """
float2 p = UV*2-1; float r = length(p); float ang = atan2(p.y, p.x);
float pw = saturate(PortalPower*PC.a);
float spd = 0.8;   // not scaled by power: a power change would jump the phase
float t = Tm*spd;
float tw = ang + 1.9/(r + 0.18) - t*3.2;
float arms = pow(0.5 + 0.5*sin(tw*3.0), 2.2);
float arms2 = pow(0.5 + 0.5*sin(tw*5.0 + 1.7 - t*1.3), 4.0);
float depth = smoothstep(0.02, 0.95, r);
float inner = pow(depth, 2.4);   // the first capture read flat: a deeper fall-off into the dark centre
float3 col = PC.rgb*Tint.rgb;
float3 hot = lerp(col, float3(1,1,1), 0.65);
float3 c = col*0.05;
c += col*(arms*0.9 + arms2*0.35)*inner*(0.35 + 1.3*pw);
c += hot*pow(arms, 3.0)*inner*inner*0.6*pw;
float rim = exp(-pow((r - 0.93)/0.035, 2.0));
float rimN = 0.75 + 0.25*sin(ang*7.0 + Tm*4.0);
c += hot*rim*(1.2 + 1.6*pw)*rimN;
float sp = 0.0;
for (int i = 0; i < 7; i++) {
    float h = frac(sin(i*12.9898)*43758.5453);
    float ph = frac(Tm*(0.45 + 0.35*h)*spd*1.6 + h);
    float rr = 0.9*(1.0 - ph);
    float aa = h*6.2831853 + ph*3.4;
    float2 q = p - rr*float2(cos(aa), sin(aa));
    float2 tang = float2(-sin(aa), cos(aa));
    float along = dot(q, tang); float across = dot(q, float2(cos(aa), sin(aa)));
    sp += exp(-(along*along)/0.006 - (across*across)/0.0006)*ph*(1.0 - ph)*4.0;
}
c += hot*sp*(0.2 + 1.8*pw);
c += hot*exp(-r*r/0.004)*0.25*pw;
float mask = 1.0 - smoothstep(0.985, 1.0, r);
float a = mask*saturate(0.92 + rim);
return float4(c*Intensity*mask, a);""", {'PortalPower': 1.0, 'Tint': (1.0, 1.0, 1.0)}),

    # round 9: Bastion's shield dome (T7's playtest: from inside, the placeholder filled the view). A hex lattice that lives
    # on the fresnel: almost clear where the shell faces the camera (the middle of the view from inside), the hexes and a
    # thin hot rim where the shell turns away (its silhouette from outside; where it crosses the screen from inside), a
    # slow shimmer travelling over the cells. On the engine sphere (lat-long UVs), additive, both sides.
    'M_TD_VFX_DomeHex': ('additive', False, 1, 1, (0, 0), (0, 0), 0, """
float f = saturate(Fres);
float2 q = UV*float2(64.0, 32.0); float2 s = float2(1.0, 1.7320508);
float2 a2 = fmod(q, s) - s*0.5; float2 b2 = fmod(q + s*0.5, s) - s*0.5;
float2 g = abs(dot(a2, a2) < dot(b2, b2) ? a2 : b2);
float d = max(dot(g, s*0.5), g.x);
float edge = smoothstep(0.40, 0.48, d);
float shim = 0.55 + 0.45*sin(Tm*1.7 - UV.y*31.0 + UV.x*13.0);
float body = pow(f, 3.6);   // the first build (2.2) drew the lattice over the whole view from inside
float rim = pow(f, 5.0)*smoothstep(1.0, 0.975, f)*1.4;   // 9e: brighter and closer to the silhouette, so the line where the dome crosses the screen reads (Lead)
float a = edge*(0.02 + 0.98*body)*shim + rim*1.6 + body*0.05;
float3 c = PC.rgb*a + lerp(PC.rgb, float3(1,1,1), 0.7)*rim*0.8;
return float4(c*Intensity, PC.a);""", {}),

    # round 9 (T7's colour-blind check: under deuteranopia the enemy's red-orange bolts and Bastion's amber shells match):
    # the enemy bolt reads by SHAPE - a hollow diamond, a hot rim round a dark core, a soft halo. Camera-facing sprite.
    'M_TD_VFX_Diamond': ('additive', False, 1, 1, (0, 0), (0, 0), 0, PRE + """
float2 q = abs(p);
float d = (q.x + q.y)*0.92 - 0.62;            // < 0 inside the diamond
float rim = exp(-pow(d/0.075, 2.0));
float inside = 1.0 - smoothstep(-0.02, 0.02, d);
float halo = exp(-max(d, 0.0)*7.0)*m;
float3 c = PC.rgb*(rim*1.8 + inside*0.10 + halo*0.30) + lerp(PC.rgb, float3(1,1,1), 0.35)*pow(rim, 4.0)*0.3;   // keep the rim enemy red-orange (Lead)
return float4(c*Intensity, PC.a);""", {}),

    'M_TD_VFX_Backdrop': ('translucent', False, 1, 1, (0, 0), (0, 0), 0, PRE + """
return float4(0.0, 0.0, 0.0, saturate(pow(m, 1.4)*Dark*PC.a));""", {'Dark': 0.75}),
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
        if isinstance(default, tuple):   # round 8: a vector parameter (r, g, b[, a])
            sp = node(unreal.MaterialExpressionVectorParameter, -1500, yy)
            sp.set_editor_property('parameter_name', pname)
            sp.set_editor_property('default_value', unreal.LinearColor(*(list(default) + [1.0])[:4]))
        else:
            sp = node(unreal.MaterialExpressionScalarParameter, -1500, yy)
            sp.set_editor_property('parameter_name', pname)
            sp.set_editor_property('default_value', default)
        inputs.append((pname, sp, ''))
        yy += 100
    if 'Tm' in body:   # round 8: engine time (animates on a static mesh too, where particle age is 0)
        inputs.append(('Tm', node(unreal.MaterialExpressionTime, -1500, yy), ''))

    if name in FLIP:
        tname, cols, rows, fp, loops = FLIP[name][:5]
        uvx = FLIP[name][5] if len(FLIP[name]) > 5 else 'clamp(UV, 0.004, 0.996)'
        body = body.replace('{FP}', fp)
        ftex = unreal.load_asset(TEX + '/' + tname)
        nfr = cols*rows
        for which, key in ((0, 'FA'), (1, 'FB')):
            cu = node(unreal.MaterialExpressionCustom, -1100, 1300 + which*200)
            cu.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT2)
            cu.set_editor_property('inputs', [_ci(n) for n in ('UV', 'Age', 'Rnd')])
            nxt = ('fmod(f0 + 1.0, %d.0)' % nfr) if loops else ('min(f0 + 1.0, %d.0)' % (nfr - 1))
            cu.set_editor_property('code', 'float fp = %s; float f0 = min(floor(fp), %d.0); float f = %s; float2 cell = float2(fmod(f, %d.0), floor(f/%d.0)); return (cell + %s)/float2(%d.0, %d.0);'
                                   % (fp, nfr - 1, nxt if which else 'f0', cols, cols, uvx, cols, rows))
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
        'M_TD_VFX_Flame_Boost': ('M_TD_VFX_Flame', {'Diamonds': 1.0}),
        # round 8: the vortex for the Rift's portal disc as a plain mesh material (for the ship builder to assign to the
        # disc's slot; T1's SetShipMaterialScalar drives PortalPower). Rift magenta.
        'MI_TD_VFX_RiftPortalVortex': ('M_TD_VFX_PortalVortex', {'Tint': (1.0, 0.10, 0.85), 'PortalPower': 0.5}),
        # 10c: review only (NS_TD_Debug_HazeSphere): the haze at Density 0.6 to judge its shape; T6 sets its own Density
        'MI_TD_RegionHaze_Review': ('M_TD_RegionHaze', {'Density': 0.6})}
mic_log = []
for _n, (_parent, _params) in MICS.items():
    if ONLY_MATS and _n not in ONLY_MATS:   # round 8: no incidental re-saves of the instances
        continue
    _p = MAT + '/' + _n
    if EAL.does_asset_exist(_p):
        _mi = unreal.load_asset(_p)
    else:
        _mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(_n, MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(_mi, unreal.load_asset(MAT + '/' + _parent))
    for _k, _v in _params.items():
        if isinstance(_v, tuple):
            MEL.set_material_instance_vector_parameter_value(_mi, _k, unreal.LinearColor(*(list(_v) + [1.0])[:4]))
        else:
            MEL.set_material_instance_scalar_parameter_value(_mi, _k, _v)
    MEL.update_material_instance(_mi)
    EAL.save_loaded_asset(_mi)
    mic_log.append(_n + ' ok')
"\n".join(log + mic_log)
