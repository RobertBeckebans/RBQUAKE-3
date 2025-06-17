import sys, struct, string, math

def ANGLE2SHORT(x):
		return int((x * 65536 / 360) & 65535)

def SHORT2ANGLE(x):
		return x * (360.0 / 65536.0)

def DEG2RAD(a):
	return (a * math.pi) / 180.0

def RAD2DEG(a):
	return (a * 180.0) / math.pi

def DotProduct(x, y):
	return x[0] * y[0] + x[1] * y[1] + x[2] * y[2]

def CrossProduct(a,b):
	return [a[1]*b[2] - a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]

def VectorLength(v):
	return math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])

def VectorSubtract(a, b):
	return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]

def VectorAdd(a, b):
	return [a[0] + b[0], a[1] + b[1], a[2] + b[2]]

def VectorCopy(v):
	return [v[0], v[1], v[2]]

def VectorInverse(v):
	return [-v[0], -v[1], -v[2]]

#define VectorCopy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define	VectorScale(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))
#define	VectorMA(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))

def RadiusFromBounds(mins, maxs):
	corner = [0, 0, 0]
	a = 0
	b = 0
	
	for i in range(0, 3):
		a = abs(mins[i])
		b = abs(maxs[i])
		if a > b:
			corner[i] = a
		else:
			corner[i] = b

	return VectorLength(corner)


# NOTE: Tr3B - matrix is in column-major order
def MatrixIdentity():
	return [[1.0, 0.0, 0.0, 0.0],
			[0.0, 1.0, 0.0, 0.0],
			[0.0, 0.0, 1.0, 0.0],
			[0.0, 0.0, 0.0, 1.0]]

def MatrixFromAngles(pitch, yaw, roll):
	sp = math.sin(DEG2RAD(pitch))
	cp = math.cos(DEG2RAD(pitch))

	sy = math.sin(DEG2RAD(yaw))
	cy = math.cos(DEG2RAD(yaw))

	sr = math.sin(DEG2RAD(roll))
	cr = math.cos(DEG2RAD(roll))
	
#	return [[cp * cy, (sr * sp * cy + cr * -sy), (cr * sp * cy + -sr * -sy), 0.0],
#			[cp * sy, (sr * sp * sy + cr * cy),  (cr * sp * sy + -sr * cy),  0.0],
#			[-sp,      sr * cp,                   cr * cp,                   0.0],
#			[0.0,      0.0,                       0.0,                       1.0]]
	
	return [[cp * cy,                     cp * sy,                 -sp,       0.0],
			[(sr * sp * cy + cr * -sy),  (sr * sp * sy + cr * cy),  sr * cp,  0.0],
			[(cr * sp * cy + -sr * -sy), (cr * sp * sy + -sr * cy), cr * cp,  0.0],
			[0.0,                         0.0,                      0.0,      1.0]]
			
def MatrixTransformPoint(m, p):
	return [m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2] + m[3][0],
			m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2] + m[3][1],
			m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2] + m[3][2]]


def MatrixTransformNormal(m, p):
	return [m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2],
			m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2],
			m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2]]
			
def MatrixMultiply(b, a):
	return [[
			a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0],
			a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1],
			a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2],
			0.0,
			],[
			a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0],
			a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1],
			a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2],
			0.0,
			],[
			a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0],
			a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1],
			a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2],
			0.0,
			],[
			a[3][0] * b[0][0] + a[3][1] * b[1][0] + a[3][2] * b[2][0] + b[3][0],
			a[3][0] * b[0][1] + a[3][1] * b[1][1] + a[3][2] * b[2][1] + b[3][1],
			a[3][0] * b[0][2] + a[3][1] * b[1][2] + a[3][2] * b[2][2] + b[3][2],
			1.0,
			]]
			
def MatrixSetupTransform(forward, left, up, origin):
	return [[forward[0], forward[1], forward[2], origin[0]],
			[left[0],    left[1],   left[2],     origin[1]],
			[up[0],      up[1],     up[2],       origin[2]],
			[0.0,        0.0,       0.0,         1.0]]
