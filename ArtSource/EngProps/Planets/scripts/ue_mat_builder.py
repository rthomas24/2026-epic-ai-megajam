# Small material-graph builder shared by the planet scripts (Unreal editor Python).
import unreal

mel = unreal.MaterialEditingLibrary


class MB:
    def __init__(self, mat, problems):
        self.m = mat
        self.y = 0
        self.problems = problems

    def e(self, cls, x=-800, **props):
        self.y += 120
        n = mel.create_material_expression(self.m, cls, x, self.y)
        for k, v in props.items():
            n.set_editor_property(k, v)
        return n

    def c(self, a, ao, b, bi):
        if not mel.connect_material_expressions(a, ao, b, bi):
            self.problems.append("%s: could not connect %s.%s -> %s.%s" % (self.m.get_name(), a.get_name(), ao, b.get_name(), bi))

    def out(self, a, ao, prop):
        if not mel.connect_material_property(a, ao, prop):
            self.problems.append("%s: could not connect %s.%s -> %s" % (self.m.get_name(), a.get_name(), ao, prop))

    def scalar(self, name, v):
        return self.e(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=float(v))

    def vector(self, name, rgb):
        return self.e(unreal.MaterialExpressionVectorParameter, parameter_name=name,
                      default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))

    def const(self, v):
        return self.e(unreal.MaterialExpressionConstant, r=float(v))

    def const2(self, a, b):
        return self.e(unreal.MaterialExpressionConstant2Vector, r=float(a), g=float(b))

    def const3(self, a, b, c):
        return self.e(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(a, b, c, 1.0))

    def tex(self, name, texture, sampler, uv=None, uv_out=""):
        n = self.e(unreal.MaterialExpressionTextureSampleParameter2D, parameter_name=name)
        n.set_editor_property("texture", texture)          # default texture type must match the sampler type
        n.set_editor_property("sampler_type", sampler)
        if uv is not None:
            self.c(uv, uv_out, n, "UVs")
        return n

    def op(self, cls, a, b, ao="", bo=""):
        n = self.e(cls)
        self.c(a, ao, n, "A")
        self.c(b, bo, n, "B")
        return n

    def mul(self, a, b, ao="", bo=""): return self.op(unreal.MaterialExpressionMultiply, a, b, ao, bo)
    def add(self, a, b, ao="", bo=""): return self.op(unreal.MaterialExpressionAdd, a, b, ao, bo)
    def sub(self, a, b, ao="", bo=""): return self.op(unreal.MaterialExpressionSubtract, a, b, ao, bo)
    def div(self, a, b, ao="", bo=""): return self.op(unreal.MaterialExpressionDivide, a, b, ao, bo)
    def dot(self, a, b, ao="", bo=""): return self.op(unreal.MaterialExpressionDotProduct, a, b, ao, bo)
    def append(self, a, b, ao="", bo=""): return self.op(unreal.MaterialExpressionAppendVector, a, b, ao, bo)

    def lerp(self, a, b, alpha, ao="", bo="", alo=""):
        n = self.e(unreal.MaterialExpressionLinearInterpolate)
        self.c(a, ao, n, "A")
        self.c(b, bo, n, "B")
        self.c(alpha, alo, n, "Alpha")
        return n

    def unary(self, cls, a, ao=""):
        n = self.e(cls)
        self.c(a, ao, n, "")
        return n

    def sat(self, a, ao=""): return self.unary(unreal.MaterialExpressionSaturate, a, ao)
    def frac(self, a, ao=""): return self.unary(unreal.MaterialExpressionFrac, a, ao)
    def norm(self, a, ao=""): return self.unary(unreal.MaterialExpressionNormalize, a, ao)
    def one_minus(self, a, ao=""): return self.unary(unreal.MaterialExpressionOneMinus, a, ao)

    def pow(self, a, b, ao=""):
        n = self.e(unreal.MaterialExpressionPower)
        self.c(a, ao, n, "Base")
        self.c(b, "", n, "Exponent")
        return n

    def mask(self, a, r=True, g=False, b=False, ao=""):
        n = self.e(unreal.MaterialExpressionComponentMask, r=r, g=g, b=b, a=False)
        self.c(a, ao, n, "")
        return n

    def tangent_to_world(self, a, ao=""):
        n = self.e(unreal.MaterialExpressionTransform)
        n.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_TANGENT)
        n.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
        self.c(a, ao, n, "")
        return n
