#!BPY

"""
Name: 'Quake3 (.md3)...'
Blender: 242
Group: 'Export'
Tooltip: 'Export to Quake3 file format. (.md3)'
"""
__author__ = "PhaethonH, Bob Holcomb, Damien McGinnes, Robert (Tr3B) Beckebans"
__url__ = ("http://xreal.sourceforge.net")
__version__ = "0.7 2006-11-12"

__bpydoc__ = """\
This script exports a Quake3 file (MD3).

Supported:<br>
    Surfaces, Materials and Tags.

Missing:<br>
    None.

Known issues:<br>
    None.

Notes:<br>
    TODO
"""

import sys, os, os.path, struct, string, math

import Blender
from Blender import *
from Blender.Draw import *
from Blender.BGL import *
from Blender.Window import *

import types

import textwrap

import logging 
reload(logging)

import md3
from md3 import *

import q_math
from q_math import *

import q_shared
from q_shared import *

# our own logger class. it works just the same as a normal logger except
# all info messages get show. 
class Logger(logging.Logger):
	def __init__(self, name,level = logging.NOTSET):
		logging.Logger.__init__(self, name, level)
		
		self.has_warnings = False
		self.has_errors = False
		self.has_critical = False
	
	def info(self, msg, *args, **kwargs):
		apply(self._log,(logging.INFO, msg, args), kwargs)
		
	def warning(self, msg, *args, **kwargs):
		logging.Logger.warning(self, msg, *args, **kwargs)
		self.has_warnings = True
	
	def error(self, msg, *args, **kwargs):
		logging.Logger.error(self, msg, *args, **kwargs)
		self.has_errors = True
		
	def critical(self, msg, *args, **kwargs):
		logging.Logger.critical(self, msg, *args, **kwargs)
		self.has_errors = True
		
# should be able to make this print to stdout in realtime and save MESSAGES
# as well. perhaps also have a log to file option
class LogHandler(logging.StreamHandler):
	def __init__(self):
		logging.StreamHandler.__init__(self, sys.stdout)
		
		if "md3_export_log" not in Blender.Text.Get():
			self.outtext = Blender.Text.New("md3_export_log")
		else:
			self.outtext = Blender.Text.Get('md3_export_log')
			self.outtext.clear()
			
		self.lastmsg = ''
		
	def emit(self, record):
		# print to stdout and  to a new blender text object
		msg = self.format(record)
		
		if msg == self.lastmsg:
			return
		
		self.lastmsg = msg
		self.outtext.write("%s\n" %msg)
		
		logging.StreamHandler.emit(self, record)

logging.setLoggerClass(Logger)
log = logging.getLogger('md3_export')

handler = LogHandler()
formatter = logging.Formatter('%(levelname)s %(message)s')
handler.setFormatter(formatter)

log.addHandler(handler)
# set this to minimum output level. eg. logging.DEBUG, logging.WARNING, logging.ERROR
# logging.CRITICAL. logging.INFO will make little difference as these always get 
# output'd
log.setLevel(logging.WARNING)


class BlenderGui:
	def __init__(self):
		text = """A log has been written to a blender text window. Change this window type to 
a text window and you will be able to select the file md3_export_log."""

		text = textwrap.wrap(text,40)
		text += ['']
		
		if log.has_critical:
			text += ['There were critical errors!!!!']
			
		elif log.has_errors:
			text += ['There were errors!']
			
		elif log.has_warnings:
			text += ['There were warnings']
			
		# add any more text before here
		text.reverse()
		
		self.msg = text
		
		Blender.Draw.Register(self.gui, self.event, self.button_event)
		
	def gui(self,):
		quitbutton = Blender.Draw.Button("Exit", 1, 0, 0, 100, 20, "Close Window")
		
		y = 35
		
		for line in self.msg:
			BGL.glRasterPos2i(10,y)
			Blender.Draw.Text(line)
			y+=15
			
	def event(self,evt, val):
		if evt == Blender.Draw.ESCKEY:
			Blender.Draw.Exit()
			return
	
	def button_event(self,evt):
		if evt == 1:
			Blender.Draw.Exit()
			return


def ApplyTransform(vert, matrix):
	return vert * matrix


def UpdateFrameBounds(v, f):
	for i in range(0, 3):
		f.mins[i] = min(v[i], f.mins[i])
	for i in range(0, 3):
		f.maxs[i] = max(v[i], f.maxs[i])


def UpdateFrameRadius(f):
	f.radius = RadiusFromBounds(f.mins, f.maxs)


def ProcessSurface(scene, blenderObject, md3, pathName, modelName):
	# because md3 doesnt suppoort faceUVs like blender, we need to duplicate
	# any vertex that has multiple uv coords

	vertDict = {}
	indexDict = {} # maps a vertex index to the revised index after duplicating to account for uv
	vertList = [] # list of vertices ordered by revised index
	numVerts = 0
	uvList = [] # list of tex coords ordered by revised index
	faceList = [] # list of faces (they index into vertList)
	numFaces = 0

	scene.makeCurrent()
	Blender.Set("curframe", 1)
	Blender.Window.Redraw()

	# get the object (not just name) and the Mesh, not NMesh
	mesh = blenderObject.getData(False, True)
	matrix = blenderObject.getMatrix('worldspace')

	surf = md3Surface()
	surf.numFrames = md3.numFrames
	surf.name = blenderObject.getName()
	surf.ident = MD3_IDENT
	
	# create shader for surface
	surf.shaders.append(md3Shader())
	surf.numShaders += 1
	surf.shaders[0].index = 0
	
	log.info("Materials: %s", mesh.materials)
	# :P
	shaderpath=Blender.Draw.PupStrInput("shader path for "+blenderObject.name+":", "", MAX_QPATH )
	if 	shaderpath == "" :
		if not mesh.materials:
			surf.shaders[0].name = pathName + blenderObject.name
		else:
			surf.shaders[0].name = pathName + mesh.materials[0].name
	else:
		if not mesh.materials:
			surf.shaders[0].name = shaderpath + blenderObject.name
		else:
			surf.shaders[0].name = shaderpath + mesh.materials[0].name

	# process each face in the mesh
	for face in mesh.faces:
		
		tris_in_this_face = []  #to handle quads and up...
		
		# this makes a list of indices for each tri in this face. a quad will be [[0,1,1],[0,2,3]]
		for vi in range(1, len(face.v)-1):
			tris_in_this_face.append([0, vi, vi + 1])
		
		# loop across each tri in the face, then each vertex in the tri
		for this_tri in tris_in_this_face:
			numFaces += 1
			tri = md3Triangle()
			tri_ind = 0
			for i in this_tri:
				# get the vertex index, coords and uv coords
				index = face.v[i].index
				v = face.v[i].co
				if mesh.faceUV == True:
					uv = (face.uv[i][0], face.uv[i][1])
				elif mesh.vertexUV:
					uv = (face.v[i].uvco[0], face.v[i].uvco[1])
				else:
					uv = (0.0, 0.0) # handle case with no tex coords	

				
				if vertDict.has_key((index, uv)):
					# if we've seen this exact vertex before, simply add it
					# to the tris list of vertex indices
					tri.indexes[tri_ind] = vertDict[(index, uv)]
				else:
					# havent seen this tri before 
					# (or its uv coord is different, so we need to duplicate it)
					
					vertDict[(index, uv)] = numVerts
					
					# put the uv coord into the list
					# (uv coord are directly related to each vertex)
					tex = md3TexCoord()
					tex.u = uv[0]
					tex.v = uv[1]
					uvList.append(tex)

					tri.indexes[tri_ind] = numVerts

					# now because we have created a new index, 
					# we need a way to link it to the index that
					# blender returns for NMVert.index
					if indexDict.has_key(index):
						# already there - each of the entries against 
						# this key represents  the same vertex with a
						# different uv value
						ilist = indexDict[index]
						ilist.append(numVerts)
						indexDict[index] = ilist
					else:
						# this is a new one
						indexDict[index] = [numVerts]

					numVerts += 1
				tri_ind +=1
			faceList.append(tri)

	# we're done with faces and uv coords
	for t in uvList:
		surf.uv.append(t)

	for f in faceList:
		surf.triangles.append(f)

	surf.numTriangles = len(faceList)
	surf.numVerts = numVerts

	# now vertices are stored as frames -
	# all vertices for frame 1, all vertices for frame 2...., all vertices for frame n
	# so we need to iterate across blender's frames, and copy out each vertex
	for	frameNum in range(1, md3.numFrames + 1):
		Blender.Set("curframe", frameNum)
		Blender.Window.Redraw()

		m = NMesh.GetRawFromObject(blenderObject.name)

		vlist = [0] * numVerts
		for vertex in m.verts:
			try:
				vindices = indexDict[vertex.index]
			except:
				log.warning("Found a vertex in %s that is not part of a face", blenderObject.name)
				continue

			vTx = ApplyTransform(vertex.co, matrix)
			nTx = ApplyTransform(vertex.no, matrix)
			UpdateFrameBounds(vTx, md3.frames[frameNum - 1])
			vert = md3Vert()
			#vert.xyz = vertex.co[0:3]
			#vert.normal = vert.Encode(vertex.no[0:3])
			vert.xyz = vTx[0:3]
			vert.normal = vert.Encode(nTx[0:3])
			for ind in vindices:  # apply the position to all the duplicated vertices
				vlist[ind] = vert

		UpdateFrameRadius(md3.frames[frameNum - 1])

		for vl in vlist:
			surf.verts.append(vl)

	surf.Dump(log)
	md3.surfaces.append(surf)
	md3.numSurfaces += 1


def Export(fileName):
	if(fileName.find('.md3', -4) <= 0):
		fileName += '.md3'
		
	log.info("Starting ...")
	
	log.info("Exporting MD3 format to: %s", fileName)
	
	pathName = StripGamePath(StripModel(fileName))
	log.info("Shader path name: %s", pathName)
	
	modelName = StripExtension(StripPath(fileName))
	log.info("Model name: %s", modelName)
	
	md3 = md3Object()
	md3.ident = MD3_IDENT
	md3.version = MD3_VERSION

	tagList = []
	
	# get the scene
	scene = Blender.Scene.getCurrent()
	context = scene.getRenderingContext()

	scene.makeCurrent()
	md3.numFrames = Blender.Get("curframe")
	Blender.Set("curframe", 1)

	# create a bunch of blank frames, they'll be filled in by 'ProcessSurface'
	for i in range(1, md3.numFrames + 1):
		frame = md3Frame()
		frame.name = "frame_" + str(i)
		md3.frames.append(frame)

	# export all selected objects
	objlist = Blender.Object.GetSelected()

	# process each object for the export
	for obj in objlist:
		# check if it's a mesh object
		if obj.getType() == "Mesh":
			log.info("Processing surface: %s", obj.name)
			if len(md3.surfaces) == MD3_MAX_SURFACES:
				log.warning("Hit md3 limit (%i) for number of surfaces, skipping ...", MD3_MAX_SURFACES, obj.getName())
			else:
				ProcessSurface(scene, obj, md3, pathName, modelName)
		elif obj.getType() == "Empty":   # for tags, we just put em in a list so we can process them all together
			if obj.name[0:4] == "tag_":
				log.info("Processing tag: %s", obj.name)
				tagList.append(obj)
				md3.numTags += 1
		else:
			log.info("Skipping object: %s", obj.name)

	
	# work out the transforms for the tags for each frame of the export
	for i in range(1, md3.numFrames + 1):
		
		# needed to update IPO's value, but probably not the best way for that...
		scene.makeCurrent()
		Blender.Set("curframe", i)
		Blender.Window.Redraw()
		for tag in tagList:
			t = md3Tag()
			matrix = tag.getMatrix('worldspace')
			t.origin[0] = matrix[3][0]
			t.origin[1] = matrix[3][1]
			t.origin[2] = matrix[3][2]
				
			t.axis[0] = matrix[0][0]
			t.axis[1] = matrix[0][1]
			t.axis[2] = matrix[0][2]
				
			t.axis[3] = matrix[1][0]
			t.axis[4] = matrix[1][1]
			t.axis[5] = matrix[1][2]
				
			t.axis[6] = matrix[2][0]
			t.axis[7] = matrix[2][1]
			t.axis[8] = matrix[2][2]
			t.name = tag.name
			#t.Dump(log)
			md3.tags.append(t)

	# export!
	file = open(fileName, "wb")
	md3.Save(file)
	file.close()
	md3.Dump(log)

def FileSelectorCallback(fileName):
	Export(fileName)
	
	BlenderGui()

Blender.Window.FileSelector(FileSelectorCallback, "Export Quake3 MD3")