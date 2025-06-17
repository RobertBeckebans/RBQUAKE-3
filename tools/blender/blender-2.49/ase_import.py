#!BPY 

""" 
Name: 'ASCII Scene (.ase) v0.12' 
Blender: 242 
Group: 'Import' 
Tooltip: 'ASCII Scene import (*.ase)' 
""" 
__author__ = "Goofos" 
__version__ = "0.12" 

# goofos at epruegel.de 
# 
# ***** BEGIN GPL LICENSE BLOCK ***** 
# 
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License 
# as published by the Free Software Foundation; either version 2 
# of the License, or (at your option) any later version. 
# 
# This program is distributed in the hope that it will be useful, 
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU General Public License for more details. 
# 
# You should have received a copy of the GNU General Public License 
# along with this program; if not, write to the Free Software Foundation, 
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 
# 
# ***** END GPL LICENCE BLOCK ***** 

import string, time, sys as osSys 
import Blender 
from Blender import Draw, Mesh, Window, Object, Scene 
#import meshtools 


def read_main(filename): 

   global counts 
   counts = {'verts': 0, 'tris': 0} 

   start = time.clock() 
   file = open(filename, "r") 

   print_boxed("----------------start-----------------") 
   print 'Import Patch: ', filename 

   editmode = Window.EditMode()    # are we in edit mode?  If so ... 
   if editmode: Window.EditMode(0) # leave edit mode before getting the mesh 

   lines= file.readlines() 
   read_file(file, lines) 

   Blender.Window.DrawProgressBar(1.0, '')  # clear progressbar 
   file.close() 
   print "----------------end-----------------" 
   end = time.clock() 
   seconds = " in %.2f %s" % (end-start, "seconds") 
   totals = "Verts: %i Tris: %i " % (counts['verts'], counts['tris']) 
   print_boxed(totals) 
   message = "Successfully imported " + Blender.sys.basename(filename) + seconds 
   #meshtools.print_boxed(message) 
   print_boxed(message) 


def print_boxed(text): #Copy/Paste from meshtools, only to remove the beep :) 
   lines = text.splitlines() 
   maxlinelen = max(map(len, lines)) 
   if osSys.platform[:3] == "win": 
      print chr(218)+chr(196) + chr(196)*maxlinelen + chr(196)+chr(191) 
      for line in lines: 
         print chr(179) + ' ' + line.ljust(maxlinelen) + ' ' + chr(179) 
      print chr(192)+chr(196) + chr(196)*maxlinelen + chr(196)+chr(217) 
   else: 
      print '+-' + '-'*maxlinelen + '-+' 
      for line in lines: print '| ' + line.ljust(maxlinelen) + ' |' 
      print '+-' + '-'*maxlinelen + '-+' 
   #print '\a\r', # beep when done 


class ase_obj: 

   def __init__(self): 
      self.name = 'Name' 
      self.objType = None 
      self.row0x = None 
      self.row0y = None 
      self.row0z = None 
      self.row1x = None 
      self.row1y = None 
      self.row1z = None 
      self.row2x = None 
      self.row2y = None 
      self.row2z = None 
      self.row3x = None 
      self.row3y = None 
      self.row3z = None 
      self.parent = None 
      self.obj = None 
      self.objName = 'Name' 

class ase_mesh: 

   def __init__(self): 
      self.name = '' 
      self.vCount = 0 
      self.fCount = 0 
      self.uvVCount = 0 
      self.uvFCount = 0 
      self.vcVCount = 0 
      self.vcFCount = 0 
      self.meVerts = [] 
      self.meFaces = [] 
      self.uvVerts = [] 
      self.uvFaces = [] 
      self.vcVerts = [] 
      self.vcFaces = [] 
      self.hasFUV = 0 
      self.hasVC = 0 

class mesh_face: 

   def __init__(self): 
      self.v1 = 0 
      self.v2 = 0 
      self.v3 = 0 
      self.mat = None 
        
class mesh_vert: 

   def __init__(self): 
      self.x = 0.0 
      self.y = 0.0 
      self.z = 0.0 

class mesh_uvVert: 

   def __init__(self): 
      self.index = 0 
      self.u = 0.0 
      self.v = 0.0 
      self.vec = Blender.Mathutils.Vector(self.u, self.v) 

class mesh_uvFace: 

   def __init__(self): 
      self.index = 0 
      self.uv1 = 0 
      self.uv2 = 0 
      self.uv3 = 0 
        
class mesh_vcVert: 

   def __init__(self): 
      self.index = 0 
      self.r = 0 
      self.g = 0 
      self.b = 0 
      self.a = 255 
        
class mesh_vcFace: 

   def __init__(self): 
      self.index = 0 
      self.c1 = 0 
      self.c2 = 0 
      self.c3 = 0 


def read_file(file, lines): 

   objects = [] 
   objIdx = 0 
   objCheck = -1 #needed to skip helper objects 
   PBidx = 0.0 
   lineCount = float(len(lines)) 

   print 'Read file' 
   Blender.Window.DrawProgressBar(0.0, "Read File...") 

   for line in lines: 
      words = string.split(line) 

      if (PBidx % 10000) == 0.0: 
                   Blender.Window.DrawProgressBar(PBidx / lineCount, "Read File...") 

      if not words: 
         continue 
      elif words[0] == '*GEOMOBJECT': 
         objCheck = 0 
         newObj = ase_obj() 
         objects.append(newObj) 
         obj = objects[objIdx] 
         objIdx += 1 
      elif words[0] == '*NODE_NAME' and objCheck != -1: 
         if objCheck == 0: 
            obj.name = words[1] 
            objCheck = 1 
         elif objCheck == 1: 
            obj.objName = words[1] 
      elif words[0] == '*TM_ROW0' and objCheck != -1: 
         obj.row0x = float(words[1]) 
         obj.row0y = float(words[2]) 
         obj.row0z = float(words[3]) 
      elif words[0] == '*TM_ROW1' and objCheck != -1: 
         obj.row1x = float(words[1]) 
         obj.row1y = float(words[2]) 
         obj.row1z = float(words[3]) 
      elif words[0] == '*TM_ROW2' and objCheck != -1: 
         obj.row2x = float(words[1]) 
         obj.row2y = float(words[2]) 
         obj.row2z = float(words[3]) 
      elif words[0] == '*TM_ROW3' and objCheck != -1: 
         obj.row3x = float(words[1]) 
         obj.row3y = float(words[2]) 
         obj.row3z = float(words[3]) 
         objCheck = -1 
      elif words[0] == '*MESH': 
         obj.objType = 'Mesh' 
         obj.obj = ase_mesh() 
         me = obj.obj 
      elif words[0] == '*MESH_NUMVERTEX': 
         me.vCount = int(words[1]) 
      elif words[0] == '*MESH_NUMFACES': 
         me.fCount = int(words[1]) 
      elif words[0] == '*MESH_VERTEX': 
         #v = mesh_vert() 
         v = [float(words[2]),float(words[3]),float(words[4])] 
         #v.x = float(words[2]) 
         #v.y = float(words[3]) 
         #v.z = float(words[4]) 
         me.meVerts.append(v) 
      elif words[0] == '*MESH_FACE': 
         #f = mesh_face() 
         f = [int(words[3]),int(words[5]),int(words[7])] 
         #f.v1 = int(words[3]) 
         #f.v2 = int(words[5]) 
         #f.v3 = int(words[7]) 
         me.meFaces.append(f) 
      elif words[0] == '*MESH_NUMTVERTEX': 
         me.uvVCount = int(words[1]) 
         if me.uvVCount > 0: 
            me.hasFUV = 1 
      elif words[0] == '*MESH_TVERT': 
         uv = mesh_uvVert() 
         uv.index = int(words[1]) 
         uv.u = float(words[2]) 
         uv.v = float(words[3]) 
         me.uvVerts.append(uv) 
      elif words[0] == '*MESH_NUMTVFACES': 
         me.uvFCount = int(words[1]) 
      elif words[0] == '*MESH_TFACE': 
         fUv = mesh_uvFace() 
         fUv.index = int(words[1]) 
         fUv.uv1 = int(words[2]) 
         fUv.uv2 = int(words[3]) 
         fUv.uv3 = int(words[4]) 
         me.uvFaces.append(fUv) 
      elif words[0] == '*MESH_NUMCVERTEX': 
         me.vcVCount = int(words[1]) 
         if me.uvVCount > 0: 
            me.hasVC = 1 
      elif words[0] == '*MESH_VERTCOL': 
         c = mesh_vcVert() 
         c.index = int(words[1]) 
         c.r = round(float(words[2])*256) 
         c.g = round(float(words[3])*256) 
         c.b = round(float(words[4])*256) 
         me.vcVerts.append(c) 
      elif words[0] == '*MESH_CFACE': 
         fc = mesh_vcFace() 
         fc.index = int(words[1]) 
         fc.c1 = int(words[2]) 
         fc.c2 = int(words[3]) 
         fc.c3 = int(words[4]) 
         me.vcFaces.append(fc) 

      PBidx += 1.0 

   spawn_main(objects) 

   Blender.Redraw() 

def spawn_main(objects): 

   PBidx = 0.0 
   objCount = float(len(objects)) 

   print 'Import Objects' 
   Blender.Window.DrawProgressBar(0.0, "Importing Objects...") 

   for obj in objects: 

      Blender.Window.DrawProgressBar(PBidx / objCount, "Importing Objects...") 

      if obj.objType == 'Mesh': 
         spawn_mesh(obj) 

      PBidx += 1.0 


def spawn_mesh(obj): 

   objMe = obj.obj 
   #normal_flag = 1 

   row0 = obj.row0x, obj.row0y, obj.row0z 
   row1 = obj.row1x, obj.row1y, obj.row1z 
   row2 = obj.row2x, obj.row2y, obj.row2z 
   row3 = obj.row3x, obj.row3y, obj.row3z 

   newMatrix = Blender.Mathutils.Matrix(row0, row1, row2, row3) 
   newMatrix.resize4x4() 

   newObj = Blender.Object.New(obj.objType, obj.name) 
   newObj.setMatrix(newMatrix) 
   Blender.Scene.getCurrent().link(newObj) 


   newMesh = Blender.Mesh.New(obj.objName) 
   newMesh.getFromObject(newObj.name) 


   # Verts 
   newMesh.verts.extend(objMe.meVerts) 

   # Faces 
   newMesh.faces.extend(objMe.meFaces) 

   #VertCol 
   if guiTable['VC'] == 1 and objMe.hasVC == 1: 
      newMesh.vertexColors = 1 
      for c in objMe.vcFaces: 

         FCol0 = newMesh.faces[c.index].col[0] 
         FCol1 = newMesh.faces[c.index].col[1] 
         FCol2 = newMesh.faces[c.index].col[2] 

         FCol0.r = int(objMe.vcVerts[c.c1].r) 
         FCol0.g = int(objMe.vcVerts[c.c1].g) 
         FCol0.b = int(objMe.vcVerts[c.c1].b) 

         FCol1.r = int(objMe.vcVerts[c.c2].r) 
         FCol1.g = int(objMe.vcVerts[c.c2].g) 
         FCol1.b = int(objMe.vcVerts[c.c2].b) 

         FCol2.r = int(objMe.vcVerts[c.c3].r) 
         FCol2.g = int(objMe.vcVerts[c.c3].g) 
         FCol2.b = int(objMe.vcVerts[c.c3].b) 

   # UV 
   if guiTable['UV'] == 1 and objMe.hasFUV == 1: 
      newMesh.faceUV = 1 
      for f in objMe.uvFaces: 
         uv1 = Blender.Mathutils.Vector(float(objMe.uvVerts[f.uv1].u), float(objMe.uvVerts[f.uv1].v)) 
         uv2 = Blender.Mathutils.Vector(float(objMe.uvVerts[f.uv2].u), float(objMe.uvVerts[f.uv2].v)) 
         uv3 = Blender.Mathutils.Vector(float(objMe.uvVerts[f.uv3].u), float(objMe.uvVerts[f.uv3].v)) 
         newMesh.faces[f.index].uv = [uv1, uv2, uv3] 

   newMesh.transform((newObj.getMatrix('worldspace').invert()), 1) 
   newObj.link(newMesh) 

   counts['verts'] += objMe.vCount 
   counts['tris'] += objMe.fCount 
   print 'Imported Mesh-Object: ', obj.name 



def read_ui(filename): 

   global guiTable, IMPORT_VC, IMPORT_UV 
   guiTable = {'VC': 1, 'UV': 1} 

   for s in Window.GetScreenInfo(): 
      Window.QHandle(s['id']) 

   IMPORT_VC = Draw.Create(guiTable['VC']) 
   IMPORT_UV = Draw.Create(guiTable['UV']) 

   # Get USER Options 
   pup_block = [('Import Options'),('Vertex Color', IMPORT_VC, 'Import Vertex Colors if exist'),('UV', IMPORT_UV, 'Import UV if exist'),] 

   if not Draw.PupBlock('Import...', pup_block): 
      return 

   Window.WaitCursor(1) 

   guiTable['VC'] = IMPORT_VC.val 
   guiTable['UV'] = IMPORT_UV.val 

   read_main(filename) 

   Window.WaitCursor(0) 


if __name__ == '__main__': 
   Window.FileSelector(read_ui, 'Import ASCII Scene', ('.ase'))