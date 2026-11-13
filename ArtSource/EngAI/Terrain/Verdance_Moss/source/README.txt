PBRgen export

TEXTURES
  pbrgen_Green_Moss_Carpet_<MAP>_<size>.png
    6 maps: COL, SPEC, ROUGH, DISP, NRM, AO

THE MATERIAL FOR OTHER SOFTWARE
  pbrgen_Green_Moss_Carpet.mtlx
    MaterialX document, OpenPBR. The lossless one, and what the PBRgen
    viewport itself renders. Native in Houdini/Karma, Maya (LookdevX,
    Arnold) and RenderMan.
  pbrgen_Green_Moss_Carpet.usda
    A USD scene: model, material and lights in one file, bound to the
    .mtlx's OpenPBR material. For Houdini, Maya and other USD apps that
    read MaterialX; the simpler UsdPreviewSurface material is kept as its
    preview. Apps that read no MaterialX may show no material at all.

MODEL
  shaderball.obj
    The shader ball in quads: shape, base and logo. Opens in any 3D software.

LIGHTING
  The viewport's studio rig: a key and a fill, plus an ambient term.

A note on matching: OpenPBR is a different shading model from
UsdPreviewSurface, so that one is a close approximation rather than
exact. The .mtlx is the reference.
