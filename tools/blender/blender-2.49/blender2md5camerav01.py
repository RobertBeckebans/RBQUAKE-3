#!BPY
"""
Name: 'Doom3 MD5 camera'
Blender: 241
Group: 'Export'
Tip: 'Export md5camera'
"""

# blender2md5camera Version 0.1
# by Adrian 'otty' Fuhrmann
# based on 'der ton's blender2md5 script

BASE_MATRIX = None # matrix_rotate_x(math.pi / 2.0)


#########################################################################################
# Code starts here.

import sys, os, os.path, struct, math, string
# if you get an error here, it might be
# because you don't have Python installed.
# go to www.python.org and get python 2.4 (not necessarily the latest!)

import Blender

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

# end of math stuff.
















class MD5CameraV10:
  def __init__(self, framerate):
    self.commandline = ""
    self.framerate = framerate
    self.cuts = []
    self.frames = []
  def to_md5camera(self):
    buf = "MD5Version 10\n"
    buf = buf + "commandline \"%s\"\n\n" % (self.commandline)
    buf = buf + "numFrames %i\n" % (len(self.frames))
    buf = buf + "frameRate %i\n" % (self.framerate)
    buf = buf + "numCuts %i\n\n" % (len(self.cuts))
    buf = buf + "cuts {\n"
    for c in self.cuts:
      buf = buf + "\t%i\n" % (c)
    buf = buf + "}\n\n"
    
    buf = buf + "camera {\n"
    for f in self.frames:
      buf = buf + "\t( %f %f %f ) ( %f %f %f ) %f\n" % (f)
    buf = buf + "}\n\n"
    return buf

def export_camera(ranges, framerate):
  cams = Blender.Camera.Get()
  scene = Blender.Scene.getCurrent()
  context = scene.getRenderingContext()
  if len(cams)==0: return None
  if len(ranges) != len(cams): return None

  themd5cam = MD5CameraV10(framerate)
  for camIndex in range(len(cams)):
    camobj = Blender.Object.Get(cams[len(cams)-camIndex-1].name)
  
    #generate the animation
    rangestart, rangeend = 1,ranges[camIndex]
    for i in range(rangestart, rangeend+1):
      context.currentFrame(i)    
      scene.makeCurrent()
      Blender.Redraw() # apparently this has to be done to update the object's matrix. Thanks theeth for pointing that out
      loc = camobj.getLocation()
      m1 = camobj.getMatrix('worldspace')
    
      # this is because blender cams look down their negative z-axis and "up" is y
      # doom3 cameras look down their x axis, "up" is z
      m2 = [[-m1[2][0], -m1[2][1], -m1[2][2], 0.0], [-m1[0][0], -m1[0][1], -m1[0][2], 0.0], [m1[1][0], m1[1][1], m1[1][2], 0.0], [0,0,0,1]]
      qx, qy, qz, qw = matrix2quaternion(m2)
      if qw>0:
        qx,qy,qz = -qx,-qy,-qz
      fov = 2 * math.atan(16/cams[camIndex].getLens())*180/math.pi

      themd5cam.frames.append((loc[0]*scale, loc[1]*scale, loc[2]*scale, qx, qy, qz, fov))

    if camIndex != len(cams) - 1:
      themd5cam.cuts.append(len(themd5cam.frames) + 1)

  try:
    file = open(md5camanim_filename.val, 'w')
  except IOError, (errno, strerror):
    errmsg = "IOError #%s: %s" % (errno, strerror)
  buffer = themd5cam.to_md5camera()
  file.write(buffer)
  file.close()
  print "saved md5animation to " + md5camanim_filename.val




print "\nAvailable actions:"
print Blender.Armature.NLA.GetActions().keys()

draw_busy_screen = 0
EVENT_NOEVENT = 1
EVENT_EXPORT = 2
EVENT_QUIT = 3
EVENT_MESHFILENAME = 4
EVENT_ANIMFILENAME = 5
EVENT_MESHFILENAME_STRINGBUTTON = 6
EVENT_ANIMFILENAME_STRINGBUTTON = 7
EVENT_CAMEXPORT = 8
EVENT_CAM_ANIMFILENAME = 10
md5mesh_filename = Blender.Draw.Create("")
md5anim_filename = Blender.Draw.Create("")
md5camanim_filename = Blender.Draw.Create("")
if len(Blender.Armature.NLA.GetActions().keys())>0:
    firstaction_name = Blender.Armature.NLA.GetActions().keys()[0]
else:
    firstaction_name = ""
export_animation = Blender.Draw.Create(firstaction_name)

## this is not used in this version, but might come back or might be useful for people who modify this script
## currently, the first action that is found will get exported
# This is the name of the animation that should get exported
#export_animation = Blender.Draw.Create("Action")


# this is a scale factor for md5 exporting. scaling with BASE_MATRIX won't work correctly
# setting this to 1 (no scaling) might help if your animation looks distorted
scale_slider = Blender.Draw.Create(1.0)
scale = 1.0

scene = Blender.Scene.getCurrent()
context = scene.getRenderingContext()

framerate_slider = Blender.Draw.Create(context.framesPerSec())

framespersecond = context.framesPerSec()

sliders = [0] * 10

for i in range ( 0, 10 ):
  sliders[i] = Blender.Draw.Create(context.endFrame())


######################################################
# Callbacks for Window functions
######################################################
def md5meshname_callback(filename):
  global md5mesh_filename
  md5mesh_filename.val=filename

def md5animname_callback(filename):
  global md5anim_filename
  md5anim_filename.val=filename

def md5camanimname_callback(filename):
  global md5camanim_filename
  md5camanim_filename.val=filename
  


######################################################
# GUI Functions
######################################################
def handle_event(evt, val):
  if evt == Blender.Draw.ESCKEY:
    Blender.Draw.Exit()
    return



def handle_button_event(evt):
  global EVENT_NOEVENT, EVENT_CAMEXPORT, EVENT_EXPORT, EVENT_QUIT, EVENT_MESHFILENAME, EVENT_ANIMFILENAME, EVENT_MESHFILENAME_STRINGBUTTON, EVENT_ANIMFILENAME_STRINGBUTTON
  global EVENT_CAM_MESHFILENAME, EVENT_CAM_ANIMFILENAME
  global draw_busy_screen, md5mesh_filename, md5anim_filename, scale_slider, scale
  global export_animation
  global framerate_slider, endframe_slider
  global md5camanim_filename
  global sliders
  global frame_sliders

  if evt == EVENT_QUIT:
    Blender.Draw.Exit()
  if evt == EVENT_MESHFILENAME:
    Blender.Window.FileSelector(md5meshname_callback, "Select md5mesh file...")
    Blender.Draw.Redraw(1)
  if evt == EVENT_ANIMFILENAME:
    Blender.Window.FileSelector(md5animname_callback, "Select md5anim file...")
    Blender.Draw.Redraw(1)
  if evt == EVENT_CAM_ANIMFILENAME:
    Blender.Window.FileSelector(md5camanimname_callback, "Select md5anim file...")
    Blender.Draw.Redraw(1)
  if evt == EVENT_CAMEXPORT:
    if md5camanim_filename.val == "": return
    scale = scale_slider.val
    draw_busy_screen = 1
    Blender.Draw.Draw()


    print frame_sliders
    export_camera(frame_sliders, framerate_slider.val)

    draw_busy_screen = 0
    Blender.Draw.Redraw(1)
  #wow, I just saw Dane's and HCL's C64 demo "Cycle", released today (2004-02-29) at Floppy2004, got voted first place. Simply AMAZING. I'll quickly finish this GUI crap and look into their plasma code...



def show_gui():
  global EVENT_NOEVENT, EVENT_CAMEXPORT, EVENT_EXPORT, EVENT_QUIT, EVENT_MESHFILENAME, EVENT_ANIMFILENAME, EVENT_MESHFILENAME_STRINGBUTTON, EVENT_ANIMFILENAME_STRINGBUTTON
  global draw_busy_screen, md5mesh_filename, md5anim_filename, scale_slider
  global export_animation
  global EVENT_CAM_MESHFILENAME, EVENT_CAM_ANIMFILENAME
  global md5camanim_filename
  global framerate_slider
  global frame_sliders
  global sliders

  button_width = 240
  browsebutton_width = 60
  button_height = 25
 


  if draw_busy_screen == 1:
    Blender.BGL.glClearColor(0.3,0.3,0.3,1.0)
    Blender.BGL.glClear(Blender.BGL.GL_COLOR_BUFFER_BIT)
    Blender.BGL.glColor3f(1,1,1)
    Blender.BGL.glRasterPos2i(20,25)
    Blender.Draw.Text("Please wait while exporting...")
    return
  Blender.BGL.glClearColor(0.6,0.6,0.6,1.0)
  Blender.BGL.glClear(Blender.BGL.GL_COLOR_BUFFER_BIT)
  #Blender.Draw.Button("Export!", EVENT_EXPORT, 20, 2*button_height, button_width, button_height, "Start the MD5-export")
  Blender.Draw.Button("Quit", EVENT_QUIT, 20, button_height, button_width, button_height, "Quit this script")
  Blender.Draw.Button("Export Camera", EVENT_CAMEXPORT, 20, 2*(button_height+5), button_width, button_height, "Start the Camera-export")

  Blender.Draw.Button("Browse...", EVENT_CAM_ANIMFILENAME, 21+button_width-browsebutton_width, 3*(button_height+5), browsebutton_width, button_height, "Specify MD5Camera-file")

  frame_sliders = [1,1]
  
  for i in range(0, len(Blender.Camera.Get())):
    sliders[i] = Blender.Draw.Slider("Cam %i frames:" % (i), EVENT_NOEVENT, 20, 6*(button_height+5) + i * 24, 240, 20, sliders[i].val, 1, 500, 0, "The last frame of animation to export")
    frame_sliders[i] = sliders[i].val




  md5camanim_filename = Blender.Draw.String("MD5Camera file:", EVENT_NOEVENT, 20, 3*(button_height+5), button_width-browsebutton_width, button_height, md5camanim_filename.val, 255, "MD5Camera-File to generate")

  framerate_slider = Blender.Draw.Slider("Framerate:", EVENT_NOEVENT, 20, 4*(button_height+5), button_width, button_height, framerate_slider.val, 1, 24, 0, "Framerate of camera to export")



Blender.Draw.Register (show_gui, handle_event, handle_button_event)

