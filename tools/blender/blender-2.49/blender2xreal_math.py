#!BPY


# math code for Blender 2 Xreal Suite,
# by Adrian 'otty' Fuhrmann,
# based on 'der ton's blender2md5 script

BASE_MATRIX = None # matrix_rotate_x(math.pi / 2.0)

import sys, os, os.path, struct, math, string

# HACK -- it seems that some Blender versions don't define sys.argv,
# which may crash Python if a warning occurs.
if not hasattr(sys, "argv"): sys.argv = ["???"]


# Math stuff
# some of this is not used in this version, but not thrown out yet.


def matrix2quaternion(m):
  s = math.sqrt(abs(m[0][0] + m[1][1] + m[2][2] + m[3][3]))
  if s == 0.0:
    x = abs(m[2][1] - m[1][2])
    y = abs(m[0][2] - m[2][0])
    z = abs(m[1][0] - m[0][1])
    if   (x >= y) and (x >= z): return 1.0, 0.0, 0.0, 0.0
    elif (y >= x) and (y >= z): return 0.0, 1.0, 0.0, 0.0
    else:                       return 0.0, 0.0, 1.0, 0.0
  return quaternion_normalize([
    -(m[2][1] - m[1][2]) / (2.0 * s),
    -(m[0][2] - m[2][0]) / (2.0 * s),
    -(m[1][0] - m[0][1]) / (2.0 * s),
    0.5 * s,
    ])

def quaternion_normalize(q):
  l = math.sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3])
  return q[0] / l, q[1] / l, q[2] / l, q[3] / l

def quaternion_multiply(q1, q2):
  r = [
    q2[3] * q1[0] + q2[0] * q1[3] + q2[1] * q1[2] - q2[2] * q1[1],
    q2[3] * q1[1] + q2[1] * q1[3] + q2[2] * q1[0] - q2[0] * q1[2],
    q2[3] * q1[2] + q2[2] * q1[3] + q2[0] * q1[1] - q2[1] * q1[0],
    q2[3] * q1[3] - q2[0] * q1[0] - q2[1] * q1[1] - q2[2] * q1[2],
    ]
  d = math.sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2] + r[3] * r[3])
  r[0] /= d
  r[1] /= d
  r[2] /= d
  r[3] /= d
  return r


def matrix_invert(m):
  det = (m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
       - m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
       + m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]))
  if det == 0.0: return None
  det = 1.0 / det
  r = [ [
      det * (m[1][1] * m[2][2] - m[2][1] * m[1][2]),
    - det * (m[0][1] * m[2][2] - m[2][1] * m[0][2]),
      det * (m[0][1] * m[1][2] - m[1][1] * m[0][2]),
      0.0,
    ], [
    - det * (m[1][0] * m[2][2] - m[2][0] * m[1][2]),
      det * (m[0][0] * m[2][2] - m[2][0] * m[0][2]),
    - det * (m[0][0] * m[1][2] - m[1][0] * m[0][2]),
      0.0
    ], [
      det * (m[1][0] * m[2][1] - m[2][0] * m[1][1]),
    - det * (m[0][0] * m[2][1] - m[2][0] * m[0][1]),
      det * (m[0][0] * m[1][1] - m[1][0] * m[0][1]),
      0.0,
    ] ]
  r.append([
    -(m[3][0] * r[0][0] + m[3][1] * r[1][0] + m[3][2] * r[2][0]),
    -(m[3][0] * r[0][1] + m[3][1] * r[1][1] + m[3][2] * r[2][1]),
    -(m[3][0] * r[0][2] + m[3][1] * r[1][2] + m[3][2] * r[2][2]),
    1.0,
    ])
  return r

def matrix_rotate_x(angle):
  cos = math.cos(angle)
  sin = math.sin(angle)
  return [
    [1.0,  0.0, 0.0, 0.0],
    [0.0,  cos, sin, 0.0],
    [0.0, -sin, cos, 0.0],
    [0.0,  0.0, 0.0, 1.0],
    ]

def matrix_rotate_y(angle):
  cos = math.cos(angle)
  sin = math.sin(angle)
  return [
    [cos, 0.0, -sin, 0.0],
    [0.0, 1.0,  0.0, 0.0],
    [sin, 0.0,  cos, 0.0],
    [0.0, 0.0,  0.0, 1.0],
    ]

def matrix_rotate_z(angle):
  cos = math.cos(angle)
  sin = math.sin(angle)
  return [
    [ cos, sin, 0.0, 0.0],
    [-sin, cos, 0.0, 0.0],
    [ 0.0, 0.0, 1.0, 0.0],
    [ 0.0, 0.0, 0.0, 1.0],
    ]

def matrix_rotate(axis, angle):
  vx  = axis[0]
  vy  = axis[1]
  vz  = axis[2]
  vx2 = vx * vx
  vy2 = vy * vy
  vz2 = vz * vz
  cos = math.cos(angle)
  sin = math.sin(angle)
  co1 = 1.0 - cos
  return [
    [vx2 * co1 + cos,          vx * vy * co1 + vz * sin, vz * vx * co1 - vy * sin, 0.0],
    [vx * vy * co1 - vz * sin, vy2 * co1 + cos,          vy * vz * co1 + vx * sin, 0.0],
    [vz * vx * co1 + vy * sin, vy * vz * co1 - vx * sin, vz2 * co1 + cos,          0.0],
    [0.0, 0.0, 0.0, 1.0],
    ]

  
def point_by_matrix(p, m):
  return [p[0] * m[0][0] + p[1] * m[1][0] + p[2] * m[2][0] + m[3][0],
          p[0] * m[0][1] + p[1] * m[1][1] + p[2] * m[2][1] + m[3][1],
          p[0] * m[0][2] + p[1] * m[1][2] + p[2] * m[2][2] + m[3][2]]

def vector_by_matrix(p, m):
  return [p[0] * m[0][0] + p[1] * m[1][0] + p[2] * m[2][0],
          p[0] * m[0][1] + p[1] * m[1][1] + p[2] * m[2][1],
          p[0] * m[0][2] + p[1] * m[1][2] + p[2] * m[2][2]]

def vector_length(v):
  return math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])

def vector_normalize(v):
  l = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])
  try:
    return v[0] / l, v[1] / l, v[2] / l
  except:
    return 1, 0, 0
  
def vector_dotproduct(v1, v2):
  return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]

def vector_crossproduct(v1, v2):
  return [
    v1[1] * v2[2] - v1[2] * v2[1],
    v1[2] * v2[0] - v1[0] * v2[2],
    v1[0] * v2[1] - v1[1] * v2[0],
    ]

def vector_angle(v1, v2):
  s = vector_length(v1) * vector_length(v2)
  f = vector_dotproduct(v1, v2) / s
  if f >=  1.0: return 0.0
  if f <= -1.0: return math.pi / 2.0
  return math.atan(-f / math.sqrt(1.0 - f * f)) + math.pi / 2.0

def vector_sum(v1, v2):
	return v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2]

# end of math stuff.

