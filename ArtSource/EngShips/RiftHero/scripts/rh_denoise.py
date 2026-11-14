# SPDX-License-Identifier: MIT
# Triad Drift - feature-preserving denoise for an AI-generated (Rodin) hull: bilateral normal filtering, then vertex
# positions refitted to the filtered normals (Zheng et al. 2011 "Bilateral normal filtering for mesh denoising";
# vertex update of Sun et al. 2007). Large panels lose their lumps, creases stay sharp, the silhouette, UVs and
# topology do not change (so every baked map still fits). Used by the ship builder's close-range pass, Nov 13.
#
#   import rh_denoise; stats = rh_denoise.denoise(obj, sigma_s=None, sigma_r=0.35, normal_iters=12, vertex_iters=20)
#
# sigma_s: spatial falloff in metres (default 1.6 x the mean edge length); sigma_r: normal-difference falloff (smaller
# keeps more creases); a vertex mask (vertex group name) limits the change to part of the mesh.
import bmesh
import numpy as np


def _arrays(ob):
    me = ob.data
    nv, nf = len(me.vertices), len(me.polygons)
    co = np.empty(nv * 3, np.float64)
    me.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3)
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])          # work on triangles; the mesh itself keeps its faces
    tris = np.array([[v.index for v in f.verts] for f in bm.faces], np.int64)
    edges = {}
    for i, f in enumerate(bm.faces):
        for e in f.edges:
            edges.setdefault(e.index, []).append(i)
    pairs = np.array([p[:2] for p in edges.values() if len(p) >= 2], np.int64)
    elen = np.array([e.calc_length() for e in bm.edges])
    bm.free()
    return co, tris, pairs, float(elen.mean()) if len(elen) else 0.01


def _face_geo(co, tris):
    a, b, c = co[tris[:, 0]], co[tris[:, 1]], co[tris[:, 2]]
    cr = np.cross(b - a, c - a)
    area2 = np.linalg.norm(cr, axis=1)
    n = cr / np.maximum(area2[:, None], 1e-12)
    return n, (a + b + c) / 3.0, area2 * 0.5


def denoise(ob, sigma_s=None, sigma_r=0.35, normal_iters=12, vertex_iters=20, mask_group=None, max_move=None):
    co0, tris, pairs, mean_edge = _arrays(ob)
    co = co0.copy()
    sigma_s = sigma_s or 1.6 * mean_edge
    max_move = max_move or 2.5 * mean_edge
    weight_v = np.ones(len(co))
    if mask_group and mask_group in ob.vertex_groups:
        g = ob.vertex_groups[mask_group].index
        weight_v[:] = 0.0
        for v in ob.data.vertices:
            for ge in v.groups:
                if ge.group == g:
                    weight_v[v.index] = ge.weight
    n, c, area = _face_geo(co, tris)
    i, j = pairs[:, 0], pairs[:, 1]
    for _ in range(normal_iters):
        d = np.linalg.norm(c[i] - c[j], axis=1)
        ws = np.exp(-(d * d) / (2 * sigma_s * sigma_s))
        dn = np.linalg.norm(n[i] - n[j], axis=1)
        wr = np.exp(-(dn * dn) / (2 * sigma_r * sigma_r))
        acc = n * area[:, None]                               # each face keeps itself
        w_ij = (ws * wr)[:, None]
        np.add.at(acc, i, n[j] * area[j, None] * w_ij)
        np.add.at(acc, j, n[i] * area[i, None] * w_ij)
        n = acc / np.maximum(np.linalg.norm(acc, axis=1)[:, None], 1e-12)
    # vertex refit: move each vertex so the faces around it agree with the filtered normals
    nf = np.repeat(np.arange(len(tris)), 3)
    vv = tris.reshape(-1)
    cnt = np.bincount(vv, minlength=len(co)).astype(np.float64)
    for _ in range(vertex_iters):
        _, c, _ = _face_geo(co, tris)
        proj = np.einsum("ij,ij->i", n[nf], c[nf] - co[vv])
        delta = np.zeros_like(co)
        np.add.at(delta, vv, n[nf] * proj[:, None])
        co += (delta / np.maximum(cnt[:, None], 1.0)) * weight_v[:, None]
        move = co - co0
        ml = np.linalg.norm(move, axis=1)
        over = ml > max_move
        co[over] = co0[over] + move[over] / ml[over, None] * max_move
    ob.data.vertices.foreach_set("co", co.reshape(-1).astype(np.float32))
    ob.data.update()
    ml = np.linalg.norm(co - co0, axis=1)
    return {"verts": len(co), "faces": len(tris), "mean_edge_mm": round(mean_edge * 1000, 1),
            "sigma_s_mm": round(sigma_s * 1000, 1), "moved_mean_mm": round(float(ml.mean()) * 1000, 2),
            "moved_p99_mm": round(float(np.percentile(ml, 99)) * 1000, 2), "moved_max_mm": round(float(ml.max()) * 1000, 2)}
