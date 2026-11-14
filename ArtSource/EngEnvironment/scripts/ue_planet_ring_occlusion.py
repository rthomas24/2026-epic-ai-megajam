# SPDX-License-Identifier: MIT
# Issue #145 (B1-5, Nicholas): a blurry teal patch where the ring's far arc meets the planet's right limb. The far arc is
# behind the planet, but between the opaque surface (1.0 R) and the additive atmosphere shell (1.025 R) nothing hides it,
# so it shows through the bright teal rim as a smudge. M_TD_PlanetRing is rebuilt with the same look (unlit,
# translucent, two-sided, RingTex x RingTint x Brightness, alpha x Opacity) plus an occlusion term: a ring pixel farther
# from the camera than the planet's closest approach along its view ray, and whose ray passes within the shell radius,
# is hidden (soft over 1 % of R). The planet radius comes from the ring's own bounds (outer radius 2.3 R, build_planets.py).
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_planet_ring_occlusion.py
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(REPO, "ArtSource", "EngProps", "Planets", "scripts"))
import ue_mat_builder  # noqa: E402

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty
ST = unreal.MaterialSamplerType
PATH = "/Game/TriadDrift/Gameplay/Props/Planets/M_TD_PlanetRing"
problems = []
mat = eal.load_asset(PATH)
tex = eal.load_asset("/Game/TriadDrift/Gameplay/Props/Planets/T_PlanetRing_Verdance")
mel.delete_all_material_expressions(mat)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)
mat.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_AFTER_DOF)
b = ue_mat_builder.MB(mat, problems)
rt = b.tex("RingTex", tex, ST.SAMPLERTYPE_COLOR)
col = b.mul(b.mul(rt, b.vector("RingTint", (1, 1, 1)), "RGB"), b.scalar("Brightness", 1.2))
b.out(col, "", MP.MP_EMISSIVE_COLOR)
# occlusion by the planet and its shell
P = b.e(unreal.MaterialExpressionWorldPosition)
E = b.e(unreal.MaterialExpressionCameraPositionWS)
C = b.e(unreal.MaterialExpressionObjectPositionWS)
radius = b.div(b.e(unreal.MaterialExpressionObjectRadius), b.scalar("RingOuterOverPlanet", 2.3))
shell = b.mul(radius, b.scalar("ShellRadius", 1.03))
pe = b.sub(P, E)
dist_p = b.e(unreal.MaterialExpressionDistance)
b.c(P, "", dist_p, "A")
b.c(E, "", dist_p, "B")
v = b.norm(pe)
ce = b.sub(C, E)
t = b.dot(ce, v)
closest = b.add(E, b.mul(v, t))
dist_c = b.e(unreal.MaterialExpressionDistance)
b.c(closest, "", dist_c, "A")
b.c(C, "", dist_c, "B")
soft = b.mul(radius, b.const(0.01))
outside = b.sat(b.div(b.sub(dist_c, shell), soft))        # 1 when the ray misses the shell
in_front = b.sat(b.div(b.sub(t, dist_p), soft))           # 1 when the ring pixel is nearer than the planet
visible = b.sat(b.add(outside, in_front))
b.out(b.mul(b.mul(rt, b.scalar("Opacity", 0.85), "A"), visible), "", MP.MP_OPACITY)
mel.recompile_material(mat)
mel.layout_material_expressions(mat)
eal.save_asset(PATH, only_if_is_dirty=False)
for p in problems:
    unreal.log("TDPY: PROBLEM " + p)
unreal.log("TDPY: " + ("FAILED %d" % len(problems) if problems else "OK"))
