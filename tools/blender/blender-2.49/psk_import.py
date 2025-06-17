#!BPY

"""
Name: 'Unreal Skeletal Mesh (.psk)'
Blender: 248a
Group: 'Import'
Tooltip: 'Unreal Skeletal Mesh Import (*.psk)' 
"""

__author__ = "Robert (Tr3B) Beckebans"
__url__ = ("http://xreal.sourceforge.net")
__version__ = "0.9 2009-03-18"
__bpydoc__ = """\ 

-- Unreal Skeletal Mesh (.psk) import script --<br>

- NOTES:
- This script imports from Unreal's PSK file format for Skeletal Meshes. <br>

- v0.9
- Initial version


- LICENSE:
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
""" 







import string
from string import *
from struct import *

import Blender, time, os, math, sys as osSys, operator, struct
from Blender import sys, Window, Draw, Scene, Object, NMesh, Mesh, Material, Texture, Image, Mathutils, Armature
from Blender.Mathutils import *


"""
       from matrix and quaternion faq
       x = w1x2 + x1w2 + y1z2 - z1y2
       y = w1y2 + y1w2 + z1x2 - x1z2
       z = w1z2 + z1w2 + x1y2 - y1x2

       w = w1w2 - x1x2 - y1y2 - z1z2
"""
def QuatMultiply1(qa, qb):

    qc = Quaternion()

    qc.x = qa.w * qb.x + qa.x * qb.w + qa.y * qb.z - qa.z * qb.y;
    qc.y = qa.w * qb.y + qa.y * qb.w + qa.z * qb.x - qa.x * qb.z;
    qc.z = qa.w * qb.z + qa.z * qb.w + qa.x * qb.y - qa.y * qb.x;
    qc.w = qa.w * qb.w - qa.x * qb.x - qa.y * qb.y - qa.z * qb.z;
    
    return qc



def asciiz(s):
    n = 0
    while(ord(s[n]) != 0):
        n = n + 1
    return s[0:n]



class axChunkHeader:
    binaryFormat = "20s3i"
    
    def __init__(self):
        self.chunkID = [] # length=20
        self.typeFlags = 0 # 1999801 special value
        self.dataSize = 0
        self.dataCount = 0
    
    def Load(self, file):
        data = []
        try:
            tmpData = file.read(struct.calcsize(self.binaryFormat))
            data = struct.unpack(self.binaryFormat, tmpData)
        except:
            print("Exception while reading axChunkHeader")
            raise
        
        self.chunkID = str(data[0])
        self.typeFlags = data[1]
        self.dataSize = data[2]
        self.dataCount = data[3]
    
    #def Save(self):
    #    data = pack('20siii', self.ChunkID, self.TypeFlag, self.DataSize, self.DataCount)
    #    return data
    
    def Dump(self):
        print("axChunkHeader:")
        print("chunkID:", self.chunkID)
        print("typeFlags:", self.typeFlags)
        print("dataSize:", self.dataSize)
        print("dataCount:", self.dataCount)


class axPoint:
    binaryFormat = "3f"

    def __init__(self):
        self.x = 0.0
        self.y = 0.0
        self.z = 0.0
        

    def Load(self, file):
        try:
            tmpData = file.read(struct.calcsize(self.binaryFormat))
            data = struct.unpack(self.binaryFormat, tmpData)
        except:
            print("Exception while reading axPoint")
            raise
        
        self.x = data[0]
        self.y = data[1]
        self.z = data[2]

    def Dump(self):
        print("axPoint:")
        print("X:", self.x)
        print("Y:", self.y)
        print("Z:", self.z)


class axVertex:
    binaryFormat = "hhffbbh"
    
    def __init__(self):
        self.pointIndex = 0
        self.unknownA = 0
        self.u = 0.0
        self.v = 0.0
        self.materialIndex = 0
        self.reserved = 0
        self.unknownB = 0

    def Load(self, file):
        try:
            tmpData = file.read(struct.calcsize(self.binaryFormat))
            data = struct.unpack(self.binaryFormat, tmpData)
        except:
            print("Exception while reading axVertex")
            raise
        
        self.pointIndex = data[0]
        self.unknownA = data[1]
        self.u = data[2]
        self.v = data[3]
        self.materialIndex = data[4]
        self.reserved = data[5]
        self.unknownB = data[6]

    def Dump(self):
        print("axVertex:")
        print("pointIndex:", self.pointIndex)
        print("UV:", self.u, self.v)
        print("materialIndex:", self.materialIndex)
        

class axTriangle:
    binaryFormat = "3hbbi"
    
    def __init__(self):
        self.indexes = [0, 0, 0]
        self.materialIndex = 0
        self.materialIndex2 = 1
        self.smoothingGroups = 0

    def Load(self, file):
        try:
            tmpData = file.read(struct.calcsize(self.binaryFormat))
            data = struct.unpack(self.binaryFormat, tmpData)
        except:
            print("Exception while reading axTriangle")
            raise
        
        self.indexes[0] = data[0]
        self.indexes[1] = data[2] # reverse
        self.indexes[2] = data[1] # reverse
        self.materialIndex = data[3]
        self.materialIndex2 = data[4]
        self.smoothingGroups = data[5]

    def Dump(self):
        print("axTriangle:")
        print("indices:", self.indexes[0], self.indexes[1], self.indexes[2])
        print("materialIndex:", self.materialIndex)
        print("materialIndex2:", self.materialIndex2)



class axReferenceBone:
    binaryFormat = "64siii4f3fffff"
    
    def __init__(self):
        self.name = ""
        self.flags = 0
        self.numChildren = 1
        self.parentIndex = 0
        self.quat = Quaternion()
        self.position = Vector()
        self.length = 0
        self.xSize = 0
        self.ySize = 0
        self.zSize = 0

    def Load(self, file):
        try:
            tmpData = file.read(struct.calcsize(self.binaryFormat))
            data = struct.unpack(self.binaryFormat, tmpData)
        except:
            print("Exception while reading axReferenceBone")
            raise
        
        self.name = asciiz(data[0])
        self.flags = data[1]
        self.numChildren = data[2]
        self.parentIndex = data[3]
        self.quat.x = data[4]
        self.quat.y = data[5]
        self.quat.z = data[6]
        self.quat.w = data[7]
        self.position.x = data[8]
        self.position.y = data[9]
        self.position.z = data[10]
        self.length = self.position.length
        self.xSize = data[12]
        self.ySize = data[13]
        self.zSize = data[14]

    def Dump(self):
        print("axReferenceBone:")
        print("name:", self.name)
        print("flags:", self.flags)
        print("numChildren:", self.numChildren)
        print("parentIndex:", self.parentIndex)
        print("quat:", self.quat[0], self.quat[1], self.quat[2], self.quat[3])
        print("position:", self.position[0], self.position[1], self.position[2])
        print("length:", self.length)
        print("xSize:", self.xSize)
        print("ySize:", self.ySize)
        print("zSize:", self.zSize)



class axBoneWeight:
    binaryFormat = "fii"

    def __init__(self):
        self.weight = 0.0
        self.pointIndex = 0
        self.boneIndex = 0
        

    def Load(self, file):
        try:
            tmpData = file.read(struct.calcsize(self.binaryFormat))
            data = struct.unpack(self.binaryFormat, tmpData)
        except:
            print("Exception while reading axBoneWeight")
            raise
        
        self.weight = data[0]
        self.pointIndex = data[1]
        self.boneIndex = data[2]

    def Dump(self):
        print("axBoneWeight:")
        print("weight:", self.weight)
        print("pointIndex:", self.pointIndex)
        print("boneIndex:", self.boneIndex)





def ImportPSK(infile):
    print "Importing file: ", infile
    pskFile = file(infile,'rb')
    
    #
    mesh = Mesh.New('mesh01')
    
    # read general header
    header = axChunkHeader()
    header.Load(pskFile)
    header.Dump()
    
    # read the PNTS0000 header
    header.Load(pskFile)
    header.Dump()
    
    axPoints = []
    for i in range(0, header.dataCount):
        point = axPoint()
        point.Load(pskFile)
        
        axPoints.append(point)
            
        #mesh.verts.extend([[point.x, point.y, point.z]])
    
    
    # read the VTXW0000 header
    header.Load(pskFile)
    header.Dump()
    
    xyzList = []
    uvList = []
    axVerts = []
    for i in range(0, header.dataCount):
        vert = axVertex()
        vert.Load(pskFile)
        #vert.Dump()
        
        axVerts.append(vert)
        
        xyzList.append(Vector([axPoints[vert.pointIndex].x, axPoints[vert.pointIndex].y, axPoints[vert.pointIndex].z]))
        uvList.append(Vector([vert.u, vert.v]))
        
    mesh.verts.extend(xyzList)
    
    # read the FACE0000 header
    header.Load(pskFile)
    header.Dump()
    
    axTriangles = []
    for i in range(0, header.dataCount):
        tri = axTriangle()
        tri.Load(pskFile)
        #tri.Dump()
        
        axTriangles.append(tri)
    
    SGlist = []
    for i in range(0, header.dataCount):
        tri = axTriangles[i]
        
        mesh.faces.extend(tri.indexes)
        
        uvData = []
        uvData.append(uvList[tri.indexes[0]])
        uvData.append(uvList[tri.indexes[1]])
        uvData.append(uvList[tri.indexes[2]])
        mesh.faces[i].uv = uvData
        
        # collect a list of the smoothing groups
        if SGlist.count(tri.smoothingGroups) == 0:
            SGlist.append(tri.smoothingGroups)
        
        # assign a material index to the face
        #mesh.faces[-1].materialIndex = SGlist.index(indata[5])
        mesh.faces[i].mat = tri.materialIndex
        
    mesh.update()
    meshObject = Object.New('Mesh', 'PSKMesh')
    meshObject.link(mesh)
    
    scene = Scene.GetCurrent() 
    scene.link(meshObject)
    
    # read the MATT0000 header
    header.Load(pskFile)
    header.Dump()
    
    for i in range(0, header.dataCount):
        data = unpack('64s6i', pskFile.read(88))
        matName = asciiz(data[0])
        print("creating material", matName)
        
        if matName == "":
            matName = "no_texture"
        
        try:
            mat = Material.Get(matName)
        except:
            #print("creating new material:", matName)
            mat = Material.New(matName)
        
            # create new texture
            texture = Texture.New(matName)
            texture.setType('Image')
        
            # texture to material
            mat.setTexture(0, texture, Texture.TexCo.UV, Texture.MapTo.COL)
        
    
    # read the REFSKELT header
    header.Load(pskFile)
    header.Dump()
    
    axReferenceBones = []
    for i in range(0, header.dataCount):
        axReferenceBones.append(axReferenceBone())
        axReferenceBones[i].Load(pskFile)
        #axReferenceBones[i].Dump()
        
        quat = axReferenceBones[i].quat
        
        if i == 0:
            axReferenceBones[i].parentIndex = -1
            quat.y = -quat.y
            #quat.inverse()
        else:
            quat.inverse()
        
    
    # create an armature skeleton
    armData = Armature.Armature("PSK") 
    armData.drawAxes = True 
    
    armObject = Object.New('Armature', "ReferenceBones")
    armObject.link(armData)
    
    scene = Scene.GetCurrent() 
    scene.objects.link(armObject)
    
    armData.makeEditable()
    
    editBones = []
    for i in range(0, header.dataCount):
        refBone = axReferenceBones[i]
        
        refBone.name = refBone.name.replace( ' ', '_')
        
        print("processing bone ", refBone.name)
        
        #if refBone.position.length == 0:
        #refBone.Dump()
        
        editBone = Armature.Editbone()
        editBone.name = refBone.name
        #editBone.length = refBone.position.length
        
        if refBone.parentIndex >= 0:
            refParent = axReferenceBones[refBone.parentIndex]
            parentName = refParent.name
            #print type(parentName)
            print("looking for parent bone", parentName)
            #parent = armData.bones[parentName]
            #parent.
            #
            
            editBone.head = refParent.position.copy()
            
            editParent = editBones[refBone.parentIndex]
            #editParent = armData.bones[editBones[refBone.parentIndex].name]
            #editParent = armData.bones[parentName]
            editBone.parent = editParent
            
            #editBone.tail = refBone.position
            #editBone.matrix = refBone.quat.toMatrix()
            #m = Matrix(QuatToMatrix(refParent.quat))
            #rotatedPos = m * refBone.position.copy()
            rotatedPos = refParent.quat * refBone.position.copy()
            editBone.tail = refParent.position + rotatedPos
            refBone.position = refParent.position + rotatedPos
            #editBone.tail = refBone.position = refParent.position + refBone.position
            
            q1 = refParent.quat.copy()
            q2 = refBone.quat.copy()
            refBone.quat = QuatMultiply1(q1, q2)
            
            #editBone.matrix = refBone.quat.toMatrix()
            #matrix = Matrix(refParent.quat.toMatrix() * refBone.quat.toMatrix())
            #m1 = refParent.quat.copy().toMatrix()
            #m2 = refBone.quat.toMatrix()
            
            
            #refBone.quat = matrix.toQuat()
            
            #editBone.options = [Armature.HINGE]
            #editBone.options = [Armature.HINGE, Armature.CONNECTED]
            editBone.options = [Armature.CONNECTED]
            
           
            
        else:
            editBone.head = Vector(0, 0, 0)
            editBone.tail = refBone.position.copy()
            #editBone.tail = refBone.quat.toMatrix() * refBone.position
            #editBone.options = [Armature.HINGE]
            
            #editBone.matrix = refBone.quat.toMatrix()
        
        editBones.append(editBone)
        armData.bones[editBone.name] = editBone
    
    # only update after adding all edit bones or it will crash Blender !!!
    armData.update()
        
    print("done processing reference bones")
    
    #for editBone in editBones:
        #armData.makeEditable()
        #armData.bones[editBone.name] = editBone
    
    #armData.update()
    
    armObject.makeDisplayList()
    scene.update();
    Window.RedrawAll()
    
    
    # read the RAWWEIGHTS header
    header.Load(pskFile)
    header.Dump()
    
    axBoneWeights = []
    for i in range(0, header.dataCount):
        axBoneWeights.append(axBoneWeight())
        axBoneWeights[i].Load(pskFile)
        
        #if i < 10:
        #   axBoneWeights[i].Dump()
    
    
    # calculate the vertex groups
    vertGroupCreated = []
    for i in range(0, len(axReferenceBones)):
        vertGroupCreated.append(0)
    
    
    for i in range(0, len(axReferenceBones)):
        refBone = axReferenceBones[i]
            
        for boneWeight in axBoneWeights:
            
            if boneWeight.boneIndex == i:
                    
                # create a vertex group for this bone if not done yet
                if vertGroupCreated[i] == 0:
                    print('creating vertex group:', refBone.name)
                    mesh.addVertGroup(refBone.name)
                    vertGroupCreated[i] = 1
            
                for j in range(0, len(axVerts)):
                    axVert = axVerts[j]
                    
                    #vertList.append(boneWeight.pointIndex)
                    if boneWeight.pointIndex == axVert.pointIndex:
                        mesh.assignVertsToGroup(refBone.name, [j], boneWeight.weight, Mesh.AssignModes.ADD)
                
        #mesh.assignVertsToGroup(refBone.name, vertList, )
                
    armObject.makeParentDeform([meshObject], 0, 0)
    
    pskFile.close()
    
    Window.RedrawAll()
    
    print "PSK2Blender completed"

##End of def ImportPSK#########################


def fs_callback(filename):
    t = sys.time() 
    import time
    import datetime
    print "====== IMPORTING UNREAL SKELETAL MESH FORMAT========\r\n"
        
    ImportPSK(filename)
        
    print 'Import PSK Script finished in %.2f seconds' % (sys.time()-t) 
    t = datetime.datetime.now()
    EpochSeconds = time.mktime(t.timetuple())
    print datetime.datetime.fromtimestamp(EpochSeconds)
    #textstring = 'Import Complete!'
    #Blender.Draw.PupStrInput("Name:", "untitled", 25)
    #Draw.PupMenu(textstring)

if __name__ == '__main__': 
    Window.FileSelector(fs_callback, 'Import PSK File', sys.makename(ext='.psk'))


