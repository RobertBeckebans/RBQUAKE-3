#!BPY
""" 
Name: 'Unreal Skeletal Mesh/Animation (.psk and .psa)' 
Blender: 245
Group: 'Export' 
Tooltip: 'Unreal Skeletal Mesh and Animation Export (*.psk, *.psa)' 
""" 
__author__ = "Darknet/Optimus_P-Fat/Active_Trash" 
__version__ = "0.0.13" 
__bpydoc__ = """\ 

-- Unreal Skeletal Mesh and Animation Export (.psk  and .psa) export script v0.0.1 --<br> 

- NOTES:
- This script Exports To Unreal's PSK and PSA file formats for Skeletal Meshes and Animations. <br>
- This script DOES NOT support vertex animation! These require completely different file formats. <br>

- v0.0.1
- Initial version

- v0.0.2
- This version adds support for more than one material index!

[ - Edit by: Darknet
- v0.0.3 - v0.0.12
- This will work on UT3 and it is a stable version that work with vehicle for testing. 
- Main Bone fix no dummy needed to be there.
- Just bone issues position, rotation, and offset for psk.
- The armature bone position, rotation, and the offset of the bone is fix. It was to deal with skeleton mesh export for psk.
- Animation is fix for position, offset, rotation bone support one rotation direction when armature build. 
- It will convert your mesh into triangular when exporting to psk file.
- Did not work with psa export yet.

- v0.0.13
- The animation will support different bone rotations when export the animation.
- ]

Credit to:
- export_cal3d.py (Position of the Bones Format)
- blender2md5.py (Animation Translation Format)

-Give Credit who work on this script.
""" 
# DANGER! This code is complete garbage!  Do not read!
# TODO: Throw some liscence junk in here: (maybe some GPL?)
# Liscence Junk: Use this script for whatever you feel like! 
import Blender, time, os, math, sys as osSys, operator
from Blender import sys, Window, Draw, Scene, Mesh, Material, Texture, Image, Mathutils, Armature

from cStringIO import StringIO
from struct import pack, calcsize
# REFERENCE MATERIAL JUST IN CASE:
# 
# U = x / sqrt(x^2 + y^2 + z^2)
# V = y / sqrt(x^2 + y^2 + z^2)
#
# Triangles specifed counter clockwise for front face
#
#defines for sizeofs
SIZE_FQUAT = 16
SIZE_FVECTOR = 12
SIZE_VJOINTPOS = 44
SIZE_ANIMINFOBINARY = 168
SIZE_VCHUNKHEADER = 32
SIZE_VMATERIAL = 88
SIZE_VBONE = 120
SIZE_FNAMEDBONEBINARY = 120
SIZE_VRAWBONEINFLUENCE = 12
SIZE_VQUATANIMKEY = 32
SIZE_VVERTEX = 16
SIZE_VPOINT = 12
SIZE_VTRIANGLE = 12
########################################################################
# Generic Object->Integer mapping
# the object must be usable as a dictionary key
class ObjMap:
	def __init__(self):
		self.dict = {}
		self.next = 0
	def get(self, obj):
		if (obj in self.dict):
			return self.dict[obj]
		else:
			id = self.next
			self.next = self.next + 1
			self.dict[obj] = id
			return id
			
	def items(self):
		getval = operator.itemgetter(0)
		getkey = operator.itemgetter(1)
		return map(getval, sorted(self.dict.items(), key=getkey))

########################################################################
# RG - UNREAL DATA STRUCTS - CONVERTED FROM C STRUCTS GIVEN ON UDN SITE 
# provided here: http://udn.epicgames.com/Two/BinaryFormatSpecifications.html
class FQuat:
	def __init__(self): 
		self.X = 0.0
		self.Y = 0.0
		self.Z = 0.0
		self.W = 1.0
		
	def dump(self):
		data = pack('ffff', self.X, self.Y, self.Z, self.W)
		return data
		
	def __cmp__(self, other):
		return cmp(self.X, other.X) \
			or cmp(self.Y, other.Y) \
			or cmp(self.Z, other.Z) \
			or cmp(self.W, other.W)
		
	def __hash__(self):
		return hash(self.X) ^ hash(self.Y) ^ hash(self.Z) ^ hash(self.W)
		
	def __str__(self):
		return "[%f,%f,%f,%f](FQuat)" % (self.X, self.Y, self.Z, self.W)
		
class FVector:
	def __init__(self, X=0.0, Y=0.0, Z=0.0):
		self.X = X
		self.Y = Y
		self.Z = Z
		
	def dump(self):
		data = pack('fff', self.X, self.Y, self.Z)
		return data
		
	def __cmp__(self, other):
		return cmp(self.X, other.X) \
			or cmp(self.Y, other.Y) \
			or cmp(self.Z, other.Z)
		
	def __hash__(self):
		return hash(self.X) ^ hash(self.Y) ^ hash(self.Z)
		
	def dot(self, other):
		return self.X * other.X + self.Y * other.Y + self.Z * other.Z
	
	def cross(self, other):
		return FVector(self.Y * other.Z - self.Z * other.Y,
				self.Z * other.X - self.X * other.Z,
				self.X * other.Y - self.Y * other.X)
				
	def sub(self, other):
		return FVector(self.X - other.X,
			self.Y - other.Y,
			self.Z - other.Z)

class VJointPos:
	def __init__(self):
		self.Orientation = FQuat()
		self.Position = FVector()
		self.Length = 0.0
		self.XSize = 0.0
		self.YSize = 0.0
		self.ZSize = 0.0
		
	def dump(self):
		data = self.Orientation.dump() + self.Position.dump() + pack('4f', self.Length, self.XSize, self.YSize, self.ZSize)
		return data
		
		
class AnimInfoBinary:
	def __init__(self):
		self.Name = "" # length=64
		self.Group = ""	# length=64
		self.TotalBones = 0
		self.RootInclude = 0
		self.KeyCompressionStyle = 0
		self.KeyQuotum = 0
		self.KeyPrediction = 0.0
		self.TrackTime = 0.0
		self.AnimRate = 0.0
		self.StartBone = 0
		self.FirstRawFrame = 0
		self.NumRawFrames = 0
		
	def dump(self):
		data = pack('64s64siiiifffiii', self.Name, self.Group, self.TotalBones, self.RootInclude, self.KeyCompressionStyle, self.KeyQuotum, self.KeyPrediction, self.TrackTime, self.AnimRate, self.StartBone, self.FirstRawFrame, self.NumRawFrames)
		return data

class VChunkHeader:
	def __init__(self, name, type_size):
		self.ChunkID = name # length=20
		self.TypeFlag = 1999801 # special value
		self.DataSize = type_size
		self.DataCount = 0
		
	def dump(self):
		data = pack('20siii', self.ChunkID, self.TypeFlag, self.DataSize, self.DataCount)
		return data
		
class VMaterial:
	def __init__(self):
		self.MaterialName = "" # length=64
		self.TextureIndex = 0
		self.PolyFlags = 0 # DWORD
		self.AuxMaterial = 0
		self.AuxFlags = 0 # DWORD
		self.LodBias = 0
		self.LodStyle = 0
		
	def dump(self):
		data = pack('64siLiLii', self.MaterialName, self.TextureIndex, self.PolyFlags, self.AuxMaterial, self.AuxFlags, self.LodBias, self.LodStyle)
		return data

class VBone:
	def __init__(self):
		self.Name = "" # length = 64
		self.Flags = 0 # DWORD
		self.NumChildren = 0
		self.ParentIndex = 0
		self.BonePos = VJointPos()
		
	def dump(self):
		data = pack('64sLii', self.Name, self.Flags, self.NumChildren, self.ParentIndex) + self.BonePos.dump()
		return data

#same as above - whatever - this is how Epic does it...		
class FNamedBoneBinary:
	def __init__(self):
		self.Name = "" # length = 64
		self.Flags = 0 # DWORD
		self.NumChildren = 0
		self.ParentIndex = 0
		self.BonePos = VJointPos()
		
		self.IsRealBone = 0  # this is set to 1 when the bone is actually a bone in the mesh and not a dummy
		
	def dump(self):
		data = pack('64sLii', self.Name, self.Flags, self.NumChildren, self.ParentIndex) + self.BonePos.dump()
		return data
	
class VRawBoneInfluence:
	def __init__(self):
		self.Weight = 0.0
		self.PointIndex = 0
		self.BoneIndex = 0
		
	def dump(self):
		data = pack('fii', self.Weight, self.PointIndex, self.BoneIndex)
		return data
		
class VQuatAnimKey:
	def __init__(self):
		self.Position = FVector()
		self.Orientation = FQuat()
		self.Time = 0.0
		
	def dump(self):
		data = self.Position.dump() + self.Orientation.dump() + pack('f', self.Time)
		return data
		
class VVertex:
	def __init__(self):
		self.PointIndex = 0 # WORD
		self.U = 0.0
		self.V = 0.0
		self.MatIndex = 0 #BYTE
		self.Reserved = 0 #BYTE
		
	def dump(self):
		data = pack('HHffBBH', self.PointIndex, 0, self.U, self.V, self.MatIndex, self.Reserved, 0)
		return data
		
	def __cmp__(self, other):
		return cmp(self.PointIndex, other.PointIndex) \
			or cmp(self.U, other.U) \
			or cmp(self.V, other.V) \
			or cmp(self.MatIndex, other.MatIndex) \
			or cmp(self.Reserved, other.Reserved)
			
	def __hash__(self):
		return hash(self.PointIndex) \
			^ hash(self.U) ^ hash(self.V) \
			^ hash(self.MatIndex) \
			^ hash(self.Reserved)
		
class VPoint:
	def __init__(self):
		self.Point = FVector()
		
	def dump(self):
		return self.Point.dump()
		
	def __cmp__(self, other):
		return cmp(self.Point, other.Point)
		
	def __hash__(self):
		return hash(self.Point)
		
class VTriangle:
	def __init__(self):
		self.WedgeIndex0 = 0 # WORD
		self.WedgeIndex1 = 0 # WORD
		self.WedgeIndex2 = 0 # WORD
		self.MatIndex = 0 # BYTE
		self.AuxMatIndex = 0 # BYTE
		self.SmoothingGroups = 0 # DWORD
		
	def dump(self):
		data = pack('HHHBBL', self.WedgeIndex0, self.WedgeIndex1, self.WedgeIndex2, self.MatIndex, self.AuxMatIndex, self.SmoothingGroups)
		return data

# END UNREAL DATA STRUCTS
########################################################################
#RG - helper class to handle the normal way the UT files are stored 
#as sections consisting of a header and then a list of data structures
class FileSection:
	def __init__(self, name, type_size):
		self.Header = VChunkHeader(name, type_size)
		self.Data = [] # list of datatypes
		
	def dump(self):
		data = self.Header.dump()
		for i in range(len(self.Data)):
			data = data + self.Data[i].dump()
		return data
		
	def UpdateHeader(self):
		self.Header.DataCount = len(self.Data)
		
class PSKFile:
	def __init__(self):
		self.GeneralHeader = VChunkHeader("ACTRHEAD", 0)
		self.Points = FileSection("PNTS0000", SIZE_VPOINT)		#VPoint
		self.Wedges = FileSection("VTXW0000", SIZE_VVERTEX)		#VVertex
		self.Faces = FileSection("FACE0000", SIZE_VTRIANGLE)		#VTriangle
		self.Materials = FileSection("MATT0000", SIZE_VMATERIAL)	#VMaterial
		self.Bones = FileSection("REFSKELT", SIZE_VBONE)		#VBone
		self.Influences = FileSection("RAWWEIGHTS", SIZE_VRAWBONEINFLUENCE)	#VRawBoneInfluence
		
		#RG - this mapping is not dumped, but is used internally to store the new point indices 
		# for vertex groups calculated during the mesh dump, so they can be used again
		# to dump bone influences during the armature dump
		#
		# the key in this dictionary is the VertexGroup/Bone Name, and the value
		# is a list of tuples containing the new point index and the weight, in that order
		#
		# Layout:
		# { groupname : [ (index, weight), ... ], ... }
		#
		# example: 
		# { 'MyVertexGroup' : [ (0, 1.0), (5, 1.0), (3, 0.5) ] , 'OtherGroup' : [(2, 1.0)] }
		
		self.VertexGroups = {} 
		
	def AddPoint(self, p):
		#print 'AddPoint'
		self.Points.Data.append(p)
		
	def AddWedge(self, w):
		#print 'AddWedge'
		self.Wedges.Data.append(w)
	
	def AddFace(self, f):
		#print 'AddFace'
		self.Faces.Data.append(f)
		
	def AddMaterial(self, m):
		#print 'AddMaterial'
		self.Materials.Data.append(m)
		
	def AddBone(self, b):
		#print 'AddBone [%s]: Position: (x=%f, y=%f, z=%f) Rotation=(%f,%f,%f,%f)'  % (b.Name, b.BonePos.Position.X, b.BonePos.Position.Y, b.BonePos.Position.Z, b.BonePos.Orientation.X,b.BonePos.Orientation.Y,b.BonePos.Orientation.Z,b.BonePos.Orientation.W)
		self.Bones.Data.append(b)
		
	def AddInfluence(self, i):
		#print 'AddInfluence'
		self.Influences.Data.append(i)
		
	def UpdateHeaders(self):
		self.Points.UpdateHeader()
		self.Wedges.UpdateHeader()
		self.Faces.UpdateHeader()
		self.Materials.UpdateHeader()
		self.Bones.UpdateHeader()
		self.Influences.UpdateHeader()
		
	def dump(self):
		self.UpdateHeaders()
		data = self.GeneralHeader.dump() + self.Points.dump() + self.Wedges.dump() + self.Faces.dump() + self.Materials.dump() + self.Bones.dump() + self.Influences.dump()
		return data
		
	def GetMatByIndex(self, mat_index):
		if mat_index >= 0 and len(self.Materials.Data) > mat_index:
			return self.Materials.Data[mat_index]
		else:
			m = VMaterial()
			m.MaterialName = "Mat%i" % mat_index
			self.AddMaterial(m)
			return m
		
	def PrintOut(self):
		print '--- PSK FILE EXPORTED ---'
		print 'point count: %i' % len(self.Points.Data)
		print 'wedge count: %i' % len(self.Wedges.Data)
		print 'face count: %i' % len(self.Faces.Data)
		print 'material count: %i' % len(self.Materials.Data)
		print 'bone count: %i' % len(self.Bones.Data)
		print 'inlfuence count: %i' % len(self.Influences.Data)
		print '-------------------------'
# PSA FILE NOTES FROM UDN:
#
#	The raw key array holds all the keys for all the bones in all the specified sequences, 
#	organized as follows:
#	For each AnimInfoBinary's sequence there are [Number of bones] times [Number of frames keys] 
#	in the VQuatAnimKeys, laid out as tracks of [numframes] keys for each bone in the order of 
#	the bones as defined in the array of FnamedBoneBinary in the PSA. 
#
#	Once the data from the PSK (now digested into native skeletal mesh) and PSA (digested into 
#	a native animation object containing one or more sequences) are associated together at runtime, 
#	bones are linked up by name. Any bone in a skeleton (from the PSK) that finds no partner in 
#	the animation sequence (from the PSA) will assume its reference pose stance ( as defined in 
#	the offsets & rotations that are in the VBones making up the reference skeleton from the PSK)

class PSAFile:
	def __init__(self):
		self.GeneralHeader = VChunkHeader("ANIMHEAD", 0)
		self.Bones = FileSection("BONENAMES", SIZE_FNAMEDBONEBINARY)	#FNamedBoneBinary
		self.Animations = FileSection("ANIMINFO", SIZE_ANIMINFOBINARY)	#AnimInfoBinary
		self.RawKeys = FileSection("ANIMKEYS", SIZE_VQUATANIMKEY)	#VQuatAnimKey
		
		# this will take the format of key=Bone Name, value = (BoneIndex, Bone Object)
		# THIS IS NOT DUMPED
		self.BoneLookup = {} 
		
	def dump(self):
		data = self.Generalheader.dump() + self.Bones.dump() + self.Animations.dump() + self.RawKeys.dump()
		return data
	
	def AddBone(self, b):
		#LOUD
		#print "AddBone: " + b.Name
		self.Bones.Data.append(b)
		
	def AddAnimation(self, a):
		#LOUD
		#print "AddAnimation: %s, TotalBones: %i, AnimRate: %f, NumRawFrames: %i, TrackTime: %f" % (a.Name, a.TotalBones, a.AnimRate, a.NumRawFrames, a.TrackTime)
		self.Animations.Data.append(a)
		
	def AddRawKey(self, k):
		#LOUD
		#print "AddRawKey [%i]: Time: %f, Quat: x=%f, y=%f, z=%f, w=%f, Position: x=%f, y=%f, z=%f" % (len(self.RawKeys.Data), k.Time, k.Orientation.X, k.Orientation.Y, k.Orientation.Z, k.Orientation.W, k.Position.X, k.Position.Y, k.Position.Z)
		self.RawKeys.Data.append(k)
		
	def UpdateHeaders(self):
		self.Bones.UpdateHeader()
		self.Animations.UpdateHeader()
		self.RawKeys.UpdateHeader()
		
	def GetBoneByIndex(self, bone_index):
		if bone_index >= 0 and len(self.Bones.Data) > bone_index:
			return self.Bones.Data[bone_index]
	
	def IsEmpty(self):
		return (len(self.Bones.Data) == 0 or len(self.Animations.Data) == 0)
	
	def StoreBone(self, b):
		self.BoneLookup[b.Name] = [-1, b]
					
	def UseBone(self, bone_name):
		if bone_name in self.BoneLookup:
			bone_data = self.BoneLookup[bone_name]
			
			if bone_data[0] == -1:
				bone_data[0] = len(self.Bones.Data)
				self.AddBone(bone_data[1])
				#self.Bones.Data.append(bone_data[1])
			
			return bone_data[0]
			
	def GetBoneByName(self, bone_name):
		if bone_name in self.BoneLookup:
			bone_data = self.BoneLookup[bone_name]
			return bone_data[1]
		
	def GetBoneIndex(self, bone_name):
		if bone_name in self.BoneLookup:
			bone_data = self.BoneLookup[bone_name]
			return bone_data[0]
		
	def dump(self):
		self.UpdateHeaders()
		data = self.GeneralHeader.dump() + self.Bones.dump() + self.Animations.dump() + self.RawKeys.dump()
		return data
		
	def PrintOut(self):
		print '--- PSA FILE EXPORTED ---'
		print 'bone count: %i' % len(self.Bones.Data)
		print 'animation count: %i' % len(self.Animations.Data)
		print 'rawkey count: %i' % len(self.RawKeys.Data)
		print '-------------------------'
		
####################################	
# helpers to create bone structs
def make_vbone(name, parent_index, child_count, orientation_quat, position_vect):
	bone = VBone()
	bone.Name = name
	bone.ParentIndex = parent_index
	bone.NumChildren = child_count
	bone.BonePos.Orientation = orientation_quat
	bone.BonePos.Position.X = position_vect.x
	bone.BonePos.Position.Y = position_vect.y
	bone.BonePos.Position.Z = position_vect.z
	
	#these values seem to be ignored?
	#bone.BonePos.Length = tail.length
	#bone.BonePos.XSize = tail.x
	#bone.BonePos.YSize = tail.y
	#bone.BonePos.ZSize = tail.z

	return bone

def make_namedbonebinary(name, parent_index, child_count, orientation_quat, position_vect, is_real):
	bone = FNamedBoneBinary()
	bone.Name = name
	bone.ParentIndex = parent_index
	bone.NumChildren = child_count
	bone.BonePos.Orientation = orientation_quat
	bone.BonePos.Position.X = position_vect.x
	bone.BonePos.Position.Y = position_vect.y
	bone.BonePos.Position.Z = position_vect.z
	bone.IsRealBone = is_real
	return bone	
	
##################################################
#RG - check to make sure face isnt a line
def is_1d_face(blender_face):
	return ((blender_face.v[0].co == blender_face.v[1].co) or \
	(blender_face.v[1].co == blender_face.v[2].co) or \
	(blender_face.v[2].co == blender_face.v[0].co))

##################################################
# http://en.wikibooks.org/wiki/Blender_3D:_Blending_Into_Python/Cookbook#Triangulate_NMesh
def triangulateNMesh(nm):
	import Blender
        '''
        Converts the meshes faces to tris, modifies the mesh in place.
        '''
        #============================================================================#
        # Returns a new face that has the same properties as the origional face      #
        # but with no verts                                                          #
        #============================================================================#
        def copyFace(face):
				#Blender.NMesh.Face()#Current Version of 2.45
				#NMesh.Face() #Out Date Script
                newFace = Blender.NMesh.Face()
                # Copy some generic properties
                newFace.mode = face.mode
                if face.image != None:
                    newFace.image = face.image
                newFace.flag = face.flag
                newFace.mat = face.mat
                newFace.smooth = face.smooth
                return newFace
        # 2 List comprehensions are a lot faster then 1 for loop.
        tris = [f for f in nm.faces if len(f) == 3]
        quads = [f for f in nm.faces if len(f) == 4]

        if quads: # Mesh may have no quads.
                has_uv = quads[0].uv 
                has_vcol = quads[0].col
                for quadFace in quads:
                        #print "4"
                        # Triangulate along the shortest edge
                        if (quadFace.v[0].co - quadFace.v[2].co).length < (quadFace.v[1].co - quadFace.v[3].co).length:
                                # Method 1
                                triA = 0,1,2
                                triB = 0,2,3
                        else:
                                # Method 2
                                triA = 0,1,3
                                triB = 1,2,3
                                
                        for tri1, tri2, tri3 in (triA, triB):
                                newFace = copyFace(quadFace)
                                newFace.v = [quadFace.v[tri1], quadFace.v[tri2], quadFace.v[tri3]]
                                if has_uv: newFace.uv = [quadFace.uv[tri1], quadFace.uv[tri2], quadFace.uv[tri3]]
                                if has_vcol: newFace.col = [quadFace.col[tri1], quadFace.col[tri2], quadFace.col[tri3]]
                                
                                nm.addEdge(quadFace.v[tri1], quadFace.v[tri3]) # Add an edge where the 2 tris are devided.
                                tris.append(newFace)
	nm.faces = tris # This will return the mesh into triangle with uv
	return nm
# Actual object parsing functions
def parse_meshes(blender_meshes, psk_file):
	import Blender
	#nme = Blender.NMesh.GetRaw()
	print "----- parsing meshes -----"
	#print 'blender_meshes length: %i' % (len(blender_meshes))
	
	for current_obj in blender_meshes: 
		current_mesh = current_obj.getData()
		print "Triangulate NMesh..."
		current_mesh = triangulateNMesh(current_mesh) #Conver mesh
		print "Triangulate NMesh Done!"
		object_mat = current_obj.mat 
	
		points = ObjMap()
		wedges = ObjMap()
			
		discarded_face_count = 0
		
		print ' -- Dumping Mesh Faces -- '
		for current_face in current_mesh.faces:
			#print ' -- Dumping UVs -- '
			#print current_face.uv
			
			if len(current_face.v) != 3:
				raise RuntimeError("Non-triangular face (%i)" % len(current_face.v))
				#todo: add two fake faces made of triangles?
			
			#RG - apparently blender sometimes has problems when you do quad to triangle 
			#	conversion, and ends up creating faces that have only TWO points -
			# 	one of the points is simply in the vertex list for the face twice. 
			#	This is bad, since we can't get a real face normal for a LINE, we need 
			#	a plane for this. So, before we add the face to the list of real faces, 
			#	ensure that the face is actually a plane, and not a line. If it is not 
			#	planar, just discard it and notify the user in the console after we're
			#	done dumping the rest of the faces
			
			if not is_1d_face(current_face):
			
				wedge_list = []
				vect_list = []
				
				#get or create the current material
				m = psk_file.GetMatByIndex(current_face.mat)
				#print current_face.mat
				#print 'material: %i' % (current_face.mat)
				
				for i in range(3):
					vert = current_face.v[i]
					
					if len(current_face.uv) != 3:
						#print "WARNING: Current face is missing UV coordinates - writing 0,0..."
						uv = [0.0, 0.0]
					else:
						uv = list(current_face.uv[i])
						
					#flip V coordinate because UEd requires it and DOESN'T flip it on its own like it
					#does with the mesh Y coordinates.
					#this is otherwise known as MAGIC-2
					uv[1] = 1.0 - uv[1]
					
					#print "Vertex UV: ", uv, " UVCO STUFF:", vert.uvco.x, vert.uvco.y
					
					# RE - Append untransformed vector (for normal calc below)
					# TODO: convert to Blender.Mathutils
					vect_list.append(FVector(vert.co.x, vert.co.y, vert.co.z))
					
					# Transform position for export
					vpos = vert.co * object_mat
					
					# Create the point
					p = VPoint()
					p.Point.X = vpos.x
					p.Point.Y = vpos.y
					p.Point.Z = vpos.z
					
					# Create the wedge
					w = VVertex()
					w.MatIndex = current_face.mat
					w.PointIndex = points.get(p) # get index from map
					
					w.U = uv[0]
					w.V = uv[1]
					
					wedge_index = wedges.get(w)
					wedge_list.append(wedge_index)
					
					#print results
					#print 'result PointIndex=%i, U=%f, V=%f, wedge_index=%i' % (
					#	w.PointIndex,
					#	w.U,
					#	w.V,
					#	wedge_index)
				
				# Determine face vertex order
				# get normal from blender
				no = current_face.no
				
				# TODO: convert to Blender.Mathutils
				# convert to FVector
				norm = FVector(no[0], no[1], no[2])
				
				# Calculate the normal of the face in blender order
				tnorm = vect_list[1].sub(vect_list[0]).cross(vect_list[2].sub(vect_list[1]))
				
				# RE - dot the normal from blender order against the blender normal
				# this gives the product of the two vectors' lengths along the blender normal axis
				# all that matters is the sign
				dot = norm.dot(tnorm)

				# print results
				#print 'face norm: (%f,%f,%f), tnorm=(%f,%f,%f), dot=%f' % (
				#	norm.X, norm.Y, norm.Z,
				#	tnorm.X, tnorm.Y, tnorm.Z,
				#	dot)

				tri = VTriangle()
				# RE - magic: if the dot product above > 0, order the vertices 2, 1, 0
				#        if the dot product above < 0, order the vertices 0, 1, 2
				#        if the dot product is 0, then blender's normal is coplanar with the face
				#        and we cannot deduce which side of the face is the outside of the mesh
				if (dot > 0):
					(tri.WedgeIndex2, tri.WedgeIndex1, tri.WedgeIndex0) = wedge_list
				elif (dot < 0):
					(tri.WedgeIndex0, tri.WedgeIndex1, tri.WedgeIndex2) = wedge_list
				else:
					raise RuntimeError("normal vector coplanar with face! points:", current_face.v[0].co, current_face.v[1].co, current_face.v[2].co)
				
				tri.MatIndex = current_face.mat
				psk_file.AddFace(tri)
				
			else:
				discarded_face_count = discarded_face_count + 1
				
		for point in points.items():
			psk_file.AddPoint(point)
			
		for wedge in wedges.items():
			psk_file.AddWedge(wedge)
	
		#RG - if we happend upon any non-planar faces above that we've discarded, 
		#	just let the user know we discarded them here in case they want 
		#	to investigate
	
		if discarded_face_count > 0: 
			print "INFO: Discarded %i non-planar faces." % (discarded_face_count)
		
		#RG - walk through the vertex groups and find the indexes into the PSK points array 
		#for them, then store that index and the weight as a tuple in a new list of 
		#verts for the group that we can look up later by bone name, since Blender matches
		#verts to bones for influences by having the VertexGroup named the same thing as
		#the bone
		vertex_groups = current_mesh.getVertGroupNames()
		for group in vertex_groups:
			verts = current_mesh.getVertsFromGroup(group, 1)
			
			vert_list = []
			
			for vert_data in verts:
				vert_index = vert_data[0]
				vert_weight = vert_data[1]
				
				vert = current_mesh.verts[vert_index]
				
				vpos = vert.co * object_mat
				
				p = VPoint()
				p.Point.X = vpos.x
				p.Point.Y = vpos.y
				p.Point.Z = vpos.z
				
				point_index = points.get(p)
				
				v_item = (point_index, vert_weight)
				vert_list.append(v_item)
				
				#print 'VertexGroup: %s, vert index=%i, point_index=%i' % (group, vert_index, point_index)
			
			psk_file.VertexGroups[group] = vert_list
	
def make_fquat(bquat):
	quat = FQuat()
	
	#flip handedness for UT = set x,y,z to negative (rotate in other direction)
	quat.X = -bquat.x
	quat.Y = -bquat.y
	quat.Z = -bquat.z

	quat.W = bquat.w
	return quat
	
def make_fquat_default(bquat):
	quat = FQuat()
	
	quat.X = bquat.x
	quat.Y = bquat.y
	quat.Z = bquat.z
	
	quat.W = bquat.w
	return quat
# =================================================================================================
# TODO: remove this 1am hack
nbone = 0
def parse_bone(blender_bone, psk_file, psa_file, parent_id, is_root_bone, parent_mat,parent_root):
	global nbone 	# look it's evil!

	#print '-------------------- Dumping Bone ---------------------- '
	print "Blender Bone:",blender_bone.name
	#print blender_bone.parent
	#If bone does not have parent that mean it the main bone
	if not blender_bone.hasParent():
		parent_root = blender_bone
	#print "---------------PARENT ROOT BONE: ",parent_root
	
	if blender_bone.hasChildren():
		child_count = len(blender_bone.children)
	else:
		child_count = 0
		
	#child of parent
	child_parent = blender_bone.parent
	
	if child_parent != None:
		#if parent_root.name == child_parent.name:
		#	#This one deal rotation for bone for the whole bone that will inherit off from parent to child.
		#	quat_root = blender_bone.matrix['BONESPACE']#* parent_mat.rotationPart()
		#	quat = make_fquat(quat_root.toQuat())
		#else:
		#	quat_root = blender_bone.matrix['BONESPACE']
		#	quat = make_fquat(quat_root.toQuat())
		
		quat_root = blender_bone.matrix['BONESPACE']
		quat = make_fquat(quat_root.toQuat())
		
		quat_parent = child_parent.matrix['BONESPACE'].toQuat().inverse()
		parent_head = child_parent.head['BONESPACE']* quat_parent
		parent_tail = child_parent.tail['BONESPACE']* quat_parent
		
		set_position = (parent_tail - parent_head) + blender_bone.head['BONESPACE']
	else:
		# ROOT BONE
		#This for root 
		set_position = blender_bone.head['BONESPACE']* parent_mat #ARMATURE OBJECT Locction
		rot_mat = blender_bone.matrix['BONESPACE']* parent_mat.rotationPart() #ARMATURE OBJECT Rotation
		quat = make_fquat_default(rot_mat.toQuat())
		
	#print "[[======= FINAL POSITION:",set_position
	final_parent_id = parent_id
	
	#RG/RE -
	#if we are not seperated by a small distance, create a dummy bone for the displacement
	#this is only needed for root bones, since UT assumes a connected skeleton, and from here
	#down the chain we just use "tail" as an endpoint
	#if(head.length > 0.001 and is_root_bone == 1):
	if(0):	
		pb = make_vbone("dummy_" + blender_bone.name, parent_id, 1, FQuat(), tail)
		psk_file.AddBone(pb)
		pbb = make_namedbonebinary("dummy_" + blender_bone.name, parent_id, 1, FQuat(), tail, 0)
		psa_file.StoreBone(pbb)
		final_parent_id = nbone
		nbone = nbone + 1
		#tail = tail-head
		
	my_id = nbone
	
	pb = make_vbone(blender_bone.name, final_parent_id, child_count,quat,set_position)
	psk_file.AddBone(pb)
	pbb = make_namedbonebinary(blender_bone.name, final_parent_id, child_count,quat,set_position, 1)
	psa_file.StoreBone(pbb)

	nbone = nbone + 1
	
	#RG - dump influences for this bone - use the data we collected in the mesh dump phase
	# to map our bones to vertex groups
	if blender_bone.name in psk_file.VertexGroups:
		vertex_list = psk_file.VertexGroups[blender_bone.name]
		for vertex_data in vertex_list:
			point_index = vertex_data[0]
			vertex_weight = vertex_data[1]
			
			influence = VRawBoneInfluence()
			influence.Weight = vertex_weight
			influence.BoneIndex = my_id
			influence.PointIndex = point_index
			
			#print 'Adding Bone Influence for [%s] = Point Index=%i, Weight=%f' % (blender_bone.name, point_index, vertex_weight)
			
			psk_file.AddInfluence(influence)
	
	#blender_bone.matrix['BONESPACE']
	#recursively dump child bones
	mainparent = parent_mat
	if blender_bone.hasChildren():
		for current_child_bone in blender_bone.children:
			parse_bone(current_child_bone, psk_file, psa_file, my_id, 0, mainparent,parent_root)
			#parse_bone(current_child_bone, psk_file, psa_file, my_id, 0, parent_mat)
	
def parse_armature(blender_armature, psk_file, psa_file):
	print "----- parsing armature -----"
	#print 'blender_armature length: %i' % (len(blender_armature))
	
	#magic 0 sized root bone for UT - this is where all armature dummy bones will attach
	#dont increment nbone here because we initialize it to 1 (hackity hackity hack)

	#count top level bones first. screw efficiency again - ohnoz it will take dayz to runz0r!
	child_count = 0
	for current_obj in blender_armature: 
		current_armature = current_obj.getData()
		bones = [x for x in current_armature.bones.values() if not x.hasParent()]
		child_count += len(bones)

	for current_obj in blender_armature: 
		print 'Current Armature Name: ' + current_obj.name
		current_armature = current_obj.getData()
		#armature_id = make_armature_bone(current_obj, psk_file, psa_file)
		
		#we dont want children here - only the top level bones of the armature itself
		#we will recursively dump the child bones as we dump these bones
		bones = [x for x in current_armature.bones.values() if not x.hasParent()]
		
		for current_bone in bones:
			parse_bone(current_bone, psk_file, psa_file, 0, 0, current_obj.mat,None)
			
# get blender objects by type		
def get_blender_objects(objects, type):
	return [x for x in objects if x.getType() == type]
			
#strips current extension (if any) from filename and replaces it with extension passed in
def make_filename_ext(filename, extension):

	new_filename = ''
	extension_index = filename.rfind('.')
	
	if extension_index == -1:
		new_filename = filename + extension
	else:
		new_filename = filename[0:extension_index] + extension
		
	return new_filename

# returns the quaternion Grassman product a*b
# this is the same as the rotation a(b(x)) 
# (ie. the same as B*A if A and B are matrices representing 
# the rotations described by quaternions a and b)
def grassman(a, b):	
	return Blender.Mathutils.Quaternion(
		a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z,
		a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
		a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
		a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w)
		
def parse_animation(blender_scene, psa_file):
	print "----- parsing animation -----"
	blender_context = blender_scene.getRenderingContext()
	
	anim_rate = blender_context.framesPerSec()
	#print 'Scene: %s Start Frame: %i, End Frame: %i' % (blender_scene.getName(), blender_context.startFrame(), blender_context.endFrame())
	#print "Frames Per Sec: %i" % anim_rate
	
	export_objects = blender_scene.objects
	blender_armatures = get_blender_objects(export_objects, 'Armature')
	
	cur_frame_index = 0
	
	for act in Armature.NLA.GetActions().values():
		
		action_name = act.getName()
		action_keyframes = act.getFrameNumbers()
		start_frame = min(action_keyframes)
		end_frame = max(action_keyframes)
		scene_frames = xrange(start_frame, end_frame+1) 
		#scene_frames = action_keyframes
		
		frame_count = len(scene_frames)
		
		anim = AnimInfoBinary()
		anim.Name = action_name
		anim.Group = "" #wtf is group?
		anim.NumRawFrames = frame_count
		anim.AnimRate = anim_rate
		anim.FirstRawFrame = cur_frame_index
		count_previous_keys = len(psa_file.RawKeys.Data)
		
		#print "------------ Action: %s, frame keys:" % (action_name) , action_keys
		print "-- Action: %s" % action_name;
		
		unique_bone_indexes = {}
		
		for obj in blender_armatures:
			
			current_armature = obj.getData()
			act.setActive(obj)
			
			# bone lookup table
			bones_lookup =  {}
			for bone in current_armature.bones.values():
				bones_lookup[bone.name] = bone
				
			frame_count = len(scene_frames)
			#print "Frame Count: %i" % frame_count
			
			pose_data = obj.getPose()
			
			#these must be ordered in the order the bones will show up in the PSA file!
			ordered_bones = {}
			ordered_bones = sorted([(psa_file.UseBone(x.name), x) for x in pose_data.bones.values()], key=operator.itemgetter(0))
			
			#############################
			# ORDERED FRAME, BONE
			#for frame in scene_frames:
			
			for i in range(frame_count):
				frame = scene_frames[i]
				
				#LOUD
				#print "==== outputting frame %i ===" % frame
				
				if frame_count > i+1:
					next_frame = scene_frames[i+1]
					#print "This Frame: %i, Next Frame: %i" % (frame, next_frame)
				else:
					next_frame = -1
					#print "This Frame: %i, Next Frame: NONE" % frame
					
				Blender.Set('curframe', frame)
				
				cur_frame_index = cur_frame_index + 1
				for bone_data in ordered_bones:
					bone_index = bone_data[0]
					pose_bone = bone_data[1]
					blender_bone = bones_lookup[pose_bone.name]
					
					#just need the total unique bones used, later for this AnimInfoBinary
					unique_bone_indexes[bone_index] = bone_index
					#LOUD
					#print "-------------------", pose_bone.name
					head = pose_bone.head
					posebonemat = Blender.Mathutils.Matrix(pose_bone.poseMatrix)
					
					parent_pose = pose_bone.parent
					if parent_pose:
						parentposemat = Blender.Mathutils.Matrix(parent_pose.poseMatrix)
						posebonemat = posebonemat*parentposemat.invert()
					else:
						posebonemat = posebonemat*obj.getMatrix('worldspace')
						
					head = posebonemat.translationPart()
					quat = posebonemat.toQuat().normalize()
					#rot = [rot.w,rot.x,rot.y,rot.z]
					#quat = rot
					
					# no parent?  apply armature transform
					if not blender_bone.hasParent():
						#print "hasParent:",blender_bone.name
						parent_mat = obj.mat
						head = head * parent_mat
						#tail = tail * parent_mat
						quat = grassman(parent_mat.toQuat(), quat)
					
					vkey = VQuatAnimKey()
					vkey.Position.X = head.x
					vkey.Position.Y = head.y
					vkey.Position.Z = head.z
					
					#This reverse it direction of the quat from root main and parent
					if not blender_bone.hasParent():
						vkey.Orientation = make_fquat_default(quat)
					else:
						vkey.Orientation = make_fquat(quat)
					
					#vkey.Orientation = make_fquat(quat)
					#time frm now till next frame = diff / framesPerSec
					if next_frame >= 0:
						diff = next_frame - frame
					else:
						diff = 1.0
					
					#print "Diff = ", diff
					vkey.Time = float(diff)/float(blender_context.framesPerSec())
					
					psa_file.AddRawKey(vkey)
					
			#done looping frames
			
		#done looping armatures
		#continue adding animInfoBinary counts here
		
		anim.TotalBones = len(unique_bone_indexes)
		anim.TrackTime = float(frame_count) / anim.AnimRate
		psa_file.AddAnimation(anim)

def fs_callback(filename):
	t = sys.time() 
	import time
	import datetime
	print "======EXPORTING TO UNREAL SKELETAL MESH FORMATS========\r\n"
	
	psk = PSKFile()
	psa = PSAFile()
	
	#sanity check - this should already have the extension, but just in case, we'll give it one if it doesn't
	psk_filename = make_filename_ext(filename, '.psk')
	
	#make the psa filename
	psa_filename = make_filename_ext(filename, '.psa')
	
	#print 'PSK File: ' +  psk_filename
	#print 'PSA File: ' +  psa_filename
	
	blender_meshes = []
	blender_armature = []
	
	current_scene = Blender.Scene.GetCurrent()
	current_scene.makeCurrent()
	
	cur_frame = Blender.Get('curframe') #store current frame before we start walking them during animation parse
	
	objects = current_scene.getChildren()
	
	blender_meshes = get_blender_objects(objects, 'Mesh')
	blender_armature = get_blender_objects(objects, 'Armature')
	
	try:
	
		#######################
		# STEP 1: MESH DUMP
		# we build the vertexes, wedges, and faces in here, as well as a vertexgroup lookup table
		# for the armature parse
		parse_meshes(blender_meshes, psk)
		
	except:
		Blender.Set('curframe', cur_frame) #set frame back to original frame
		print "Exception during Mesh Parse"
		raise
	
	try:
	
		#######################
		# STEP 2: ARMATURE DUMP
		# IMPORTANT: do this AFTER parsing meshes - we need to use the vertex group data from 
		# the mesh parse in here to generate bone influences
		parse_armature(blender_armature, psk, psa) 
	
	except:
		Blender.Set('curframe', cur_frame) #set frame back to original frame
		print "Exception during Armature Parse"
		raise

	try:
		#######################
		# STEP 3: ANIMATION DUMP
		# IMPORTANT: do AFTER parsing bones - we need to do bone lookups in here during animation frames
		parse_animation(current_scene, psa) 
	except:
		Blender.Set('curframe', cur_frame) #set frame back to original frame
		print "Exception during Animation Parse"
		raise

	# reset current frame
	
	Blender.Set('curframe', cur_frame) #set frame back to original frame
	
  	##########################
  	# FILE WRITE
	
	#RG - dump psk file
	psk.PrintOut()
	file = open(psk_filename, "wb") 
	file.write(psk.dump())
	file.close() 
	print 'Successfully Exported File: ' + psk_filename

	#RG - dump psa file
	if not psa.IsEmpty():
		psa.PrintOut()
		file = open(psa_filename, "wb") 
		file.write(psa.dump())
		file.close() 
		print 'Successfully Exported File: ' + psa_filename
	else:
		print 'No Animations to Export'
	print 'My Export PSK/PSA Script finished in %.2f seconds' % (sys.time()-t) 
	t = datetime.datetime.now()
	EpochSeconds = time.mktime(t.timetuple())
	print datetime.datetime.fromtimestamp(EpochSeconds)
	textstring = 'Export Complete!'
	#Blender.Draw.PupStrInput("Name:", "untitled", 25)
	Draw.PupMenu(textstring)

if __name__ == '__main__': 
	Window.FileSelector(fs_callback, 'Export PSK/PSA File', sys.makename(ext='.psk'))
	#fs_callback('c:\\ChainBenderSideTurret.psk')
	
