# Pilot: which way does the normal map's green channel point? A tangent-space normal map of a real surface is the
# gradient of a height field, so its "curl" is zero only when green is read with the right sign.
# Reads export/T_Pilot_N_source_opengl.png (the untouched glTF image). Prints the curl energy for both readings at several blur scales.
import bpy, os, numpy as np
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
img = bpy.data.images.load(os.path.join(ROOT, "export", "T_Pilot_N_source_opengl.png")); img.colorspace_settings.name = "Non-Color"
w, h = img.size
px = np.array(img.pixels[:], dtype=np.float64).reshape(h, w, 4)        # row 0 = bottom, so axis 0 is +v
nx = px[..., 0] * 2 - 1; ny = px[..., 1] * 2 - 1; nz = np.clip(px[..., 2] * 2 - 1, 0.2, 1)
print("NRM flat fraction (|xy| < 2/255):", round(float(((np.abs(nx) < 0.008) & (np.abs(ny) < 0.008)).mean()), 3),
      " std x,y:", round(float(nx.std()), 4), round(float(ny.std()), 4))
def pool(a, k):
    return a[:h // k * k, :w // k * k].reshape(h // k, k, w // k, k).mean((1, 3))
for k in (1, 2, 4, 8, 16):
    gx = pool(-nx / nz, k); gy = pool(-ny / nz, k)
    dgx_dv = np.diff(gx, axis=0)[:, :-1]; dgy_du = np.diff(gy, axis=1)[:-1, :]
    mask = (np.abs(dgx_dv) < 0.5) & (np.abs(dgy_du) < 0.5) & ((np.abs(dgx_dv) + np.abs(dgy_du)) > 0.01)
    a = dgx_dv[mask]; b = dgy_du[mask]
    curl_gl = float(((a - b) ** 2).mean()); curl_dx = float(((a + b) ** 2).mean())
    print("NRM pool %2d  samples %8d  corr %+.3f  curl energy if OpenGL %.5f  if DirectX %.5f  -> %s" % (
        k, a.size, float(np.corrcoef(a, b)[0, 1]), curl_gl, curl_dx, "OpenGL" if curl_gl < curl_dx else "DirectX"))
