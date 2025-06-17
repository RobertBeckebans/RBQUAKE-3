#!BPY

"""
Name: 'Unreal Skeletal Animation (.psa)'
Blender: 248a
Group: 'Import'
Tooltip: 'Unreal Skeletal Animation Import (*.psa)' 
"""

__author__ = "Robert (Tr3B) Beckebans"
__url__ = ("http://xreal.sourceforge.net")
__version__ = "0.1 2009-03-29"
__bpydoc__ = """\ 

-- Unreal Skeletal Animation (.psa) import script --<br>

- NOTES:
- This script imports from Unreal's PSA file format for Skeletal Animations. <br>

- v0.1
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


"""
typedef struct
{
    char            name[64];
    char            group[64];

    int                numBones;        // same as numChannels
    int                rootInclude;

    int                keyCompressionStyle;
    int                keyQuotum;
    float            keyReduction;

    float            trackTime;

    float            frameRate;

    int                startBoneIndex;

    int                firstRawFrame;
    int                numRawFrames;
} axAnimationInfo_t;
"""

class axAnimationInfo:
    binaryFormat = "64s64siiiifffiii"
    
    def __init__(self):
        self.name = ""
        self.group = ""
        self.numBones = 0
        self.rootInclude = 0
        self.keyCompressionStyle = 0
        self.keyQuotum = 0
        self.keyReduction = 0
        self.trackTime = 0.0
        self.frameRate = 0.0
        self.startBoneIndex = 0
        self.firstRawFrame = 0
        self.numRawFrames = 0

    def Load(self, file):
        try:
            tmpData = file.read(struct.calcsize(self.binaryFormat))
            data = struct.unpack(self.binaryFormat, tmpData)
        except:
            print("Exception while reading axReferenceBone")
            raise
        
        self.name = asciiz(data[0])
        self.group = asciiz(data[1])
        self.numBones = data[2]
        self.rootInclude = data[3]
        self.keyCompressionStyle = data[4]
        self.keyQuotum = data[5]
        self.keyReduction = data[6]
        self.trackTime = data[7]
        self.frameRate = data[8]
        self.startBoneIndex = data[9]
        self.firstRawFrame = data[10]
        self.numRawFrames = data[11]

    def Dump(self):
        print("axAnimationInfo:")
        print("name:", self.name)
        print("group:", self.group)
        print("numBones:", self.numBones)
        print("rootInclude:", self.rootInclude)
        print("keyCompressionStyle:", self.keyCompressionStyle)
        print("keyQuotum:", self.keyQuotum)
        print("keyReduction:", self.keyReduction)
        print("trackTime:", self.trackTime)
        print("frameRate:", self.frameRate)
        print("startBoneIndex:", self.startBoneIndex)
        print("firstRawFrame:", self.firstRawFrame)
        print("numRawFrames:", self.numRawFrames)



def ImportPSA(infile):
    print "Importing file: ", infile
    psaFile = file(infile,'rb')
    
    # read general header
    header = axChunkHeader()
    header.Load(psaFile)
    header.Dump()
    
    # read the BONENAMES header
    header.Load(psaFile)
    header.Dump()
    
    axReferenceBones = []
    for i in range(0, header.dataCount):
        axReferenceBones.append(axReferenceBone())
        axReferenceBones[i].Load(psaFile)
        axReferenceBones[i].Dump()
        
        quat = axReferenceBones[i].quat
        
        if i == 0:
            axReferenceBones[i].parentIndex = -1
            quat.y = -quat.y
            #quat.inverse()
        else:
            quat.inverse()
        
    
    # read the BONENAMES header
    header.Load(psaFile)
    header.Dump()
    
    anims = []
    for i in range(0, header.dataCount):
        anims.append(axAnimationInfo())
        anims[i].Load(psaFile)
        anims[i].Dump()
    
    scene = Scene.GetCurrent()
    
    # create an armature skeleton
#    armData = Armature.Armature("PSK") 
#    armData.drawAxes = True 
#    
#    armObject = Object.New('Armature', "ReferenceBones")
#    armObject.link(armData)
#    
#    
#    scene.objects.link(armObject)
#    
#    armData.makeEditable()
#    
#    editBones = []
#    for i in range(0, header.dataCount):
#        refBone = axReferenceBones[i]
#        
#        refBone.name = refBone.name.replace( ' ', '_')
#        
#        print("processing bone ", refBone.name)
#        
#        #if refBone.position.length == 0:
#        #refBone.Dump()
#        
#        editBone = Armature.Editbone()
#        editBone.name = refBone.name
#        #editBone.length = refBone.position.length
#        
#        if refBone.parentIndex >= 0:
#            refParent = axReferenceBones[refBone.parentIndex]
#            parentName = refParent.name
#            #print type(parentName)
#            print("looking for parent bone", parentName)
#            #parent = armData.bones[parentName]
#            #parent.
#            #
#            
#            editBone.head = refParent.position.copy()
#            
#            editParent = editBones[refBone.parentIndex]
#            #editParent = armData.bones[editBones[refBone.parentIndex].name]
#            #editParent = armData.bones[parentName]
#            editBone.parent = editParent
#            
#            #editBone.tail = refBone.position
#            #editBone.matrix = refBone.quat.toMatrix()
#            #m = Matrix(QuatToMatrix(refParent.quat))
#            #rotatedPos = m * refBone.position.copy()
#            rotatedPos = refParent.quat * refBone.position.copy()
#            editBone.tail = refParent.position + rotatedPos
#            refBone.position = refParent.position + rotatedPos
#            #editBone.tail = refBone.position = refParent.position + refBone.position
#            
#            q1 = refParent.quat.copy()
#            q2 = refBone.quat.copy()
#            refBone.quat = QuatMultiply1(q1, q2)
#            
#            #editBone.matrix = refBone.quat.toMatrix()
#            #matrix = Matrix(refParent.quat.toMatrix() * refBone.quat.toMatrix())
#            #m1 = refParent.quat.copy().toMatrix()
#            #m2 = refBone.quat.toMatrix()
#            
#            
#            #refBone.quat = matrix.toQuat()
#            
#            #editBone.options = [Armature.HINGE]
#            #editBone.options = [Armature.HINGE, Armature.CONNECTED]
#            editBone.options = [Armature.CONNECTED]
#            
#           
#            
#        else:
#            editBone.head = Vector(0, 0, 0)
#            editBone.tail = refBone.position.copy()
#            #editBone.tail = refBone.quat.toMatrix() * refBone.position
#            #editBone.options = [Armature.HINGE]
#            
#            #editBone.matrix = refBone.quat.toMatrix()
#        
#        editBones.append(editBone)
#        armData.bones[editBone.name] = editBone
#    
#    # only update after adding all edit bones or it will crash Blender !!!
#    armData.update()
        
    print("done processing reference bones")
    
    #for editBone in editBones:
        #armData.makeEditable()
        #armData.bones[editBone.name] = editBone
    
    #armData.update()
    
    #armObject.makeDisplayList()
    scene.update();
    Window.RedrawAll()
    
    
    psaFile.close()
    
    Window.RedrawAll()
    
    print "PSA2Blender completed"

##End of def ImportPSK#########################


def fs_callback(filename):
    t = sys.time() 
    import time
    import datetime
    print "====== IMPORTING UNREAL SKELETAL ANIMATION FORMAT========\r\n"
        
    ImportPSA(filename)
        
    print 'Import PSA Script finished in %.2f seconds' % (sys.time()-t) 
    t = datetime.datetime.now()
    EpochSeconds = time.mktime(t.timetuple())
    print datetime.datetime.fromtimestamp(EpochSeconds)
    #textstring = 'Import Complete!'
    #Blender.Draw.PupStrInput("Name:", "untitled", 25)
    #Draw.PupMenu(textstring)

if __name__ == '__main__': 
    Window.FileSelector(fs_callback, 'Import PSA File', sys.makename(ext='.psa'))


