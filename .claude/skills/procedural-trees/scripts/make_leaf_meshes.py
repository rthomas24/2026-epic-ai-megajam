# Editor script (run through ue_exec.py): builds the foliage meshes the PVE Foliage Palette places.
# Original geometry, generated with Unreal's Geometry Script; no textures, no alpha masks (opaque
# two-sided triangles, which Nanite and plain LODs both handle cheaply).
#
#   python ue_exec.py --port 8017 make_leaf_meshes.py --args '{"folder": "/Game/TriadDrift/Gameplay/Vegetation/Foliage"}'
#
# PVE convention (measured on the engine's own sample twigs): the mesh starts at the origin and
# points along +Z. Sizes are centimetres at a nominal length; the Foliage Distributor scales them.
# UV0.x runs 0 at the leaf base to 1 at the tip (the materials use it for the glowing accents),
# UV0.y runs 0..1 across the blade with 0.5 on the midrib. Stems have UV0.x = 0.
import math
import random

FOLDER = ARGS.get("folder", "/Game/TriadDrift/Gameplay/Vegetation/Foliage")
MATERIAL = ARGS.get("material", "")  # optional default material for slot 0


class Builder:
    def __init__(self):
        self.vertices, self.normals, self.uvs, self.triangles = [], [], [], []

    def _add(self, position, normal, uv):
        self.vertices.append(unreal.Vector(position[0], position[1], position[2]))
        self.normals.append(unreal.Vector(normal[0], normal[1], normal[2]))
        self.uvs.append(unreal.Vector2D(uv[0], uv[1]))
        return len(self.vertices) - 1

    def blade(self, base, direction, side, length, width, fold=0.18, droop=0.12, u0=0.0, u1=1.0):
        """One leaf: base point, unit direction, unit side vector. A six-point outline (widest a third of the
        way along, tapering to the tip), 4 triangles, lifted at the edges and drooping at the tip."""
        up = cross(side, direction)

        def point(along, across, lift):
            position = add(base, scale(direction, length * along))
            position = add(position, scale(side, width * across))
            return add(position, scale(up, width * lift - length * droop * along * along))

        ids = [self._add(point(0.0, 0.0, 0.0), up, (u0, 0.5)),
               self._add(point(0.34, -0.5, fold), up, (u0 + (u1 - u0) * 0.34, 0.0)),
               self._add(point(0.34, 0.5, fold), up, (u0 + (u1 - u0) * 0.34, 1.0)),
               self._add(point(0.72, -0.3, fold * 0.6), up, (u0 + (u1 - u0) * 0.72, 0.2)),
               self._add(point(0.72, 0.3, fold * 0.6), up, (u0 + (u1 - u0) * 0.72, 0.8)),
               self._add(point(1.0, 0.0, 0.0), up, (u1, 0.5))]
        b, l1, r1, l2, r2, t = ids
        self.triangles += [unreal.IntVector(b, l1, r1), unreal.IntVector(l1, l2, r1), unreal.IntVector(r1, l2, r2), unreal.IntVector(l2, t, r2)]

    def stem(self, points, radius):
        """A three-sided tube through the points (two triangles per side per segment)."""
        rings = []
        for index, point in enumerate(points):
            nxt = points[min(index + 1, len(points) - 1)]
            prv = points[max(index - 1, 0)]
            axis = normalize(sub(nxt, prv))
            ref = (1.0, 0.0, 0.0) if abs(axis[0]) < 0.9 else (0.0, 1.0, 0.0)
            a = normalize(cross(axis, ref))
            b = cross(axis, a)
            taper = 1.0 - 0.6 * index / max(1, len(points) - 1)
            ring = []
            for k in range(3):
                angle = k * 2.0 * math.pi / 3.0
                offset = add(scale(a, math.cos(angle)), scale(b, math.sin(angle)))
                ring.append(self._add(add(point, scale(offset, radius * taper)), offset, (0.0, 0.5)))
            rings.append(ring)
        for index in range(len(rings) - 1):
            lower, upper = rings[index], rings[index + 1]
            for k in range(3):
                n = (k + 1) % 3
                self.triangles += [unreal.IntVector(lower[k], upper[k], lower[n]), unreal.IntVector(lower[n], upper[k], upper[n])]

    def save(self, name):
        buffers = unreal.GeometryScriptSimpleMeshBuffers()
        buffers.set_editor_property("vertices", self.vertices)
        buffers.set_editor_property("normals", self.normals)
        buffers.set_editor_property("uv0", self.uvs)
        buffers.set_editor_property("triangles", self.triangles)
        mesh = unreal.DynamicMesh()
        unreal.GeometryScript_MeshEdits.append_buffers_to_mesh(mesh, buffers, 0)
        path = "%s/%s" % (FOLDER, name)
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.EditorAssetLibrary.delete_asset(path)
        options = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
        options.set_editor_property("enable_recompute_normals", False)
        options.set_editor_property("enable_recompute_tangents", True)
        options.set_editor_property("enable_nanite", False)
        options.set_editor_property("enable_collision", False)
        asset, outcome = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(mesh, path, options)
        if asset is None:
            raise RuntimeError("could not create %s (%s)" % (path, outcome))
        material = unreal.load_asset(MATERIAL) if MATERIAL else None
        if material:
            asset.set_material(0, material)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        bounds = asset.get_bounds()
        return {"path": path, "triangles": len(self.triangles), "vertices": len(self.vertices),
                "size_cm": [round(bounds.box_extent.x * 2), round(bounds.box_extent.y * 2), round(bounds.box_extent.z * 2)]}


def add(a, b): return (a[0] + b[0], a[1] + b[1], a[2] + b[2])
def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def scale(a, s): return (a[0] * s, a[1] * s, a[2] * s)
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def normalize(a):
    length = math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]) or 1.0
    return (a[0] / length, a[1] / length, a[2] / length)


def outward(azimuth_deg, elevation_deg):
    """Unit vector: azimuth around +Z, elevation above the horizontal plane."""
    az, el = math.radians(azimuth_deg), math.radians(elevation_deg)
    return (math.cos(el) * math.cos(az), math.cos(el) * math.sin(az), math.sin(el))


def side_of(direction):
    """Horizontal unit vector at right angles to the direction, so blades lie flat to the sky."""
    flat = normalize((-direction[1], direction[0], 0.0))
    return flat if abs(direction[2]) < 0.99 else (1.0, 0.0, 0.0)


def broad_twig(rng):
    """A 60 cm twig with fourteen simple leaves in a loose spiral: beech, mahogany, cacao-like foliage."""
    b = Builder()
    spine = [(0, 0, 0), (1.5, 0.5, 20), (-1.0, 1.5, 40), (0.5, -0.5, 58)]
    b.stem(spine, 0.9)
    count = 13
    for index in range(count):
        t = 0.12 + 0.8 * index / (count - 1)
        height = t * 58.0
        azimuth = index * 137.5 + rng.uniform(-12, 12)
        direction = outward(azimuth, rng.uniform(5, 30))
        b.blade((0, 0, height), direction, side_of(direction), rng.uniform(17, 23), rng.uniform(9, 12), droop=rng.uniform(0.1, 0.3))
    direction = outward(rng.uniform(0, 360), 70)
    b.blade(spine[-1], direction, side_of(direction), 22, 10)
    return b


def palmate_spray(rng):
    """Two long-stalked hand-shaped leaves (seven leaflets each): kapok, cecropia, schefflera-like foliage."""
    b = Builder()
    for leaf in range(2):
        azimuth = leaf * 160.0 + rng.uniform(-20, 20)
        stalk_dir = outward(azimuth, 55 - leaf * 15)
        hub = scale(stalk_dir, 38.0 + leaf * 8.0)
        b.stem([(0, 0, 0), scale(stalk_dir, 20.0), hub], 0.7)
        # leaflets fan out around the stalk's end, drooping a little like an umbrella
        a = normalize(cross(stalk_dir, (0.0, 0.0, 1.0)))
        c = cross(stalk_dir, a)
        leaflets = 7
        for k in range(leaflets):
            angle = math.radians(k * 360.0 / leaflets + rng.uniform(-8, 8))
            radial = add(scale(a, math.cos(angle)), scale(c, math.sin(angle)))
            direction = normalize(add(radial, scale(stalk_dir, 0.15)))
            side = normalize(cross(stalk_dir, direction))
            b.blade(hub, direction, side, rng.uniform(20, 26), rng.uniform(7, 9), fold=0.12, droop=0.25)
    return b


def frond(rng, fern):
    """A 100 cm arching frond. Palm: leaflets of even length. Fern: long at the base, short at the tip (a triangle)."""
    b = Builder()
    segments = 7
    spine = []
    for index in range(segments + 1):
        t = index / segments
        # rises, then arches over towards +X and droops at the tip
        spine.append((100.0 * (0.55 * t * t + 0.12 * t), 0.0, 100.0 * (0.95 * t - 0.42 * t * t)))
    b.stem(spine, 1.6)
    pairs = 13
    for index in range(pairs):
        t = 0.14 + 0.84 * index / (pairs - 1)
        position = point_on(spine, t)
        tangent = normalize(sub(point_on(spine, min(1.0, t + 0.02)), point_on(spine, max(0.0, t - 0.02))))
        if fern:
            length = 34.0 * (1.0 - 0.85 * t) + 5.0
            width = 9.0 * (1.0 - 0.6 * t) + 2.0
        else:
            length = 36.0 * math.sin(math.pi * (0.12 + 0.8 * t)) + 6.0
            width = 5.5
        for sign in (-1.0, 1.0):
            direction = normalize(add(add(scale((0.0, sign, 0.0), 1.0), scale(tangent, 0.45)), (0.0, 0.0, -0.25)))
            side = normalize(cross(direction, (0.0, 0.0, 1.0)))
            b.blade(position, direction, scale(side, sign), length * rng.uniform(0.92, 1.08), width, fold=0.1, droop=0.2, u0=0.25 + 0.5 * t, u1=1.0)
    direction = normalize(sub(spine[-1], spine[-2]))
    b.blade(spine[-1], direction, (0.0, 1.0, 0.0), 14.0 if fern else 22.0, 5.0, u0=0.7)
    return b


def point_on(points, t):
    scaled = t * (len(points) - 1)
    index = min(int(scaled), len(points) - 2)
    f = scaled - index
    return add(scale(points[index], 1.0 - f), scale(points[index + 1], f))


if not unreal.EditorAssetLibrary.does_directory_exist(FOLDER):
    unreal.EditorAssetLibrary.make_directory(FOLDER)
rng = random.Random(1870)
results = [
    broad_twig(rng).save("SM_TDVeg_Leaf_Broad"),
    palmate_spray(rng).save("SM_TDVeg_Leaf_Palmate"),
    frond(rng, False).save("SM_TDVeg_Leaf_Frond"),
    frond(rng, True).save("SM_TDVeg_Leaf_FernFrond"),
]
results
