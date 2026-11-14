# Triad Drift - tiny vector / quaternion maths with Unreal's conventions (FQuat (x, y, z, w), FTransform composition
# Child * Parent), pure Python so the grip solver runs anywhere. Animation builder, Nov 13.
import math


def add(a, b): return (a[0] + b[0], a[1] + b[1], a[2] + b[2])
def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def mul(a, s): return (a[0] * s, a[1] * s, a[2] * s)
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def length(a): return math.sqrt(dot(a, a))


def norm(a):
    n = length(a)
    return mul(a, 1.0 / n) if n > 1e-9 else (0.0, 0.0, 0.0)


def qmul(a, b):
    """Hamilton product a*b (applies b first, then a), same as FQuat::operator*."""
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def qinv(q): return (-q[0], -q[1], -q[2], q[3])


def qnorm(q):
    n = math.sqrt(sum(c * c for c in q))
    return tuple(c / n for c in q)


def qaxis(axis, deg):
    axis = norm(axis)
    h = math.radians(deg) * 0.5
    s = math.sin(h)
    return (axis[0] * s, axis[1] * s, axis[2] * s, math.cos(h))


def qrot(q, v):
    """Rotate v by q (FQuat::RotateVector)."""
    qv = (q[0], q[1], q[2])
    t = mul(cross(qv, v), 2.0)
    return add(add(v, mul(t, q[3])), cross(qv, t))


def qangle(a, b):
    """Angle in degrees between two rotations."""
    d = abs(sum(x * y for x, y in zip(qnorm(a), qnorm(b))))
    return math.degrees(2.0 * math.acos(min(1.0, d)))


def qfrom_basis(x, y, z):
    """Quaternion from orthonormal basis vectors (columns X, Y, Z), as FQuat(FMatrix)."""
    m00, m01, m02 = x[0], y[0], z[0]
    m10, m11, m12 = x[1], y[1], z[1]
    m20, m21, m22 = x[2], y[2], z[2]
    tr = m00 + m11 + m22
    if tr > 0:
        s = math.sqrt(tr + 1.0) * 2
        w = 0.25 * s
        qx = (m21 - m12) / s
        qy = (m02 - m20) / s
        qz = (m10 - m01) / s
    elif m00 > m11 and m00 > m22:
        s = math.sqrt(1.0 + m00 - m11 - m22) * 2
        w = (m21 - m12) / s
        qx = 0.25 * s
        qy = (m01 + m10) / s
        qz = (m02 + m20) / s
    elif m11 > m22:
        s = math.sqrt(1.0 + m11 - m00 - m22) * 2
        w = (m02 - m20) / s
        qx = (m01 + m10) / s
        qy = 0.25 * s
        qz = (m12 + m21) / s
    else:
        s = math.sqrt(1.0 + m22 - m00 - m11) * 2
        w = (m10 - m01) / s
        qx = (m02 + m20) / s
        qy = (m12 + m21) / s
        qz = 0.25 * s
    return qnorm((qx, qy, qz, w))


def to_rotator(q):
    """FQuat::Rotator() -> (pitch, yaw, roll) in degrees."""
    x, y, z, w = q
    singularity = z * x - w * y
    yaw_y = 2.0 * (w * z + x * y)
    yaw_x = 1.0 - 2.0 * (y * y + z * z)
    thresh = 0.4999995
    yaw = math.degrees(math.atan2(yaw_y, yaw_x))
    if singularity < -thresh:
        pitch = -90.0
        roll = -yaw - 2.0 * math.degrees(math.atan2(x, w))
    elif singularity > thresh:
        pitch = 90.0
        roll = yaw - 2.0 * math.degrees(math.atan2(x, w))
    else:
        pitch = math.degrees(math.asin(2.0 * singularity))
        roll = math.degrees(math.atan2(-2.0 * (w * x + y * z), 1.0 - 2.0 * (x * x + y * y)))
    return (pitch, yaw, roll)


def dist_to_line(p, origin, direction):
    d = norm(direction)
    v = sub(p, origin)
    return length(sub(v, mul(d, dot(v, d))))
