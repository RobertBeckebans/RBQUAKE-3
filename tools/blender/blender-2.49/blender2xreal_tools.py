#!BPY


import sys, os.path, struct, math, string
import struct, os, cStringIO, time
import subprocess
import Blender
from Blender import *
from Blender.Mathutils import *

import BPyMesh
import BPyObject

scene = []
cameras = []
objects = []

numLights=0
numEntitys=0
numCameras=0

entityData = []
lightData = []
worldData = []
brushData = []
entityBBoxData = []
worldSpawnData = []
cameraData = []

logData = []
warnings = 0

compile_list = []

def log(text):
	global logData
	logData.append(text)
	print("BlendXmap: %s" % text)
	#Blender.Redraw(1)
def warning(text):
	global warnings
	warnings = warnings + 1
	log("WARNING: %s" % text)
def alert( message ):
	block = []
	block.append(message)
	Blender.Draw.PupBlock("Message:", block)
	print("Messagebox: %s" % message)
	#Blender.Redraw(1)

import blender2xreal_math
import blender2xreal_map
import blender2xreal_ase
import blender2xreal_tools

from blender2xreal_math import *
from blender2xreal_map import *
from blender2xreal_ase import *
from blender2xreal_tools import *





def Blend2X_GroupExists(groupname):
	groups = Blender.Group.Get()	
	for group in groups:
		if group.name == groupname:
				return 1			
	return 0

def Blend2X_GroupForEntity(ob):
	groups = Blender.Group.Get()	
	for group in groups:
		for gob in group.objects:
			if gob == ob:
				return group			
	return "<invalid>"

def Blend2X_GroupCheck ( object, group ):
	grp = Blender.Group.Get(group)

	for ob in list(grp.objects):
		if object == ob :
			return 1
	return 0

	


def isConcave(mesh):
    #check every edge for > 180 degrees...
    concaveFlag = 0
    for firstEdge in mesh.edges:
        for secondEdge in mesh.edges:
            if (firstEdge.v1 <> secondEdge.v1) and (firstEdge.v2 <> secondEdge.v2):
                match = 0
                if (firstEdge.v1 == secondEdge.v1):
                    match = 1
                    Vec1 = Vector(firstEdge.v1) - Vector(firstEdge.v2)
                    Vec2 = Vector(secondEdge.v1) - Vector(secondEdge.v2)
                if (firstEdge.v2 == secondEdge.v1):
                    match = 1
                    Vec1 = Vector(firstEdge.v2) - Vector(firstEdge.v1)
                    Vec2 = Vector(secondEdge.v1) - Vector(secondEdge.v2)
                if (firstEdge.v1 == secondEdge.v2):
                    match = 1
                    Vec1 = Vector(firstEdge.v1) - Vector(firstEdge.v2)
                    Vec2 = Vector(secondEdge.v2) - Vector(secondEdge.v1)
                if (firstEdge.v2 == secondEdge.v2):
                    match = 1
                    Vec1 = Vector(firstEdge.v2) - Vector(firstEdge.v1)
                    Vec2 = Vector(secondEdge.v2) - Vector(secondEdge.v1)
                if match == 1:
                    Vec1.normalize
                    Vec2.normalize
                    if AngleBetweenVecs(Vec1, Vec2) > 180:
                        concaveFlag = concaveFlag + 1
                        print("Large angle: %d\n" %AngleBetweenVecs(Vec1, Vec2))
    return concaveFlag