#!BPY

"""
Name: 'ASCII Scene (.ase) v0.6.10'
Blender: 244
Group: 'Export'
Tooltip: 'ASCII Scene Export (*.ase)'
"""
__author__ = "Goofos"
__version__ = "0.6.10"
__url__ = ["http://www.doom3world.org","http://www.doom3world.org/phpbb2/viewtopic.php?f=50&t=9275&st=0&sk=t&sd=a"]
__bpydoc__ = """\
-- ASCII Scene Export (.ase) export script v0.6.10 for Blender 2.44 --<br>

Can export:<br>
-Mesh Objects<br>
-Materials and Textures (no Procedural but Image)<br>
   Note: Normalmaps will be exported as Bumpmaps (FixMe).
      Image path depends on how you have loaded it
      (absolute path's looks better :))
   Currently supported: Amb, Col, Csp, Hard, Alpha, Nor, Disp<br>
-Vertex Colors<br>
   Note: If the mesh has materials you must enable "Vcol Paint"
   in Material tab. Without Materials, make sure "VertCol"
   in Mesh tab is enabled. Seems like the ASE Format doesn't
   support multiple Vertex Color layers.<br>
-Face UV<br>
   Make sure "TexFace" in Mesh tab is enabled.
   Multi UV layers are now supported<br>
-Solid or Smooth Faces<br>
   ... smoothgroups currently only with a workaround. Solid
   faces will not have a smoothgroup, smooth faces will be by default in
   smoothgroup 1.<br>

-- Export Options Description --<br>
Apply Modifiers: Export the mesh with applied modifiers.
   Note: This uses the render settings of the modifiers.<br>
Materials: Export Materials if any.<br>
Face UV: Export TexFace UV if any. The current active UV Layer will be used
   as the first mapping channel.<br>
Vertex Colors: Export Vertex Colors if any (See note above). The
   current VC Layer will be used.<br>
Selection Only: Export only selected Objects or if nothing is selected
   all Objects.<br>
VertGr. as SmoothGr.: You can export SmoothGroups defined by
   VertexGroups. Simply create a VertGroup and name it "smooth." plus
   a group number, e.g. "smooth.2". Please note that you should not use
   more than 32 smoothgroups!
   Vertex Normals currently might not calculated right!!
   And there is a simple problem, if you add e.g. 3 faces of a cube
   to a smoothgroup, all 6 faces will be in the smoothgroup!! This is
   because the verts of the other 3 faces are in that group, too.
   You can see this if you select the vertexgroup.<br>
Center Objects: Move all objects to the World Grid Center.
"""
# goofos
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

import Blender, time, math, sys as osSys #os
from Blender import sys, Window, Draw, Scene, Mesh, Material, Texture, Image, Mathutils

#============================================
#           Write!
#============================================

def write(filename):
   start = time.clock()
   print_boxed('---------Start of Export------------')
   print 'Export Path: ' + filename

   global exp_list, Tab, idnt, imgTable, worldTable

   exp_list =[]
   Tab = "\t"
   idnt = 1
   matTable = {}
   worldTable = {'ambR': 0.0, 'ambG': 0.0, 'ambB': 0.0, 'horR': 0.0, 'horG': 0.0, 'horB': 0.0} #default
   total = {'Verts': 0, 'Tris': 0, 'Faces': 0}
   
   scn = Blender.Scene.GetCurrent()

   set_up(scn, exp_list, matTable, worldTable)
   if not exp_list:
      #if there is nothing to export, end here
      return

   file = open(filename, "w")
   write_header(file, filename, scn, worldTable)
   write_materials(file, exp_list, worldTable, matTable)
   write_mesh(file, scn, exp_list, matTable, total)
   file.close()
   
   Blender.Window.DrawProgressBar(0, "")    # clear progressbar
   end = time.clock()
   seconds = " in %.2f %s" % (end-start, "seconds")
   totals = "Verts: %i Tris: %i Faces: %i" % (total['Verts'], total['Tris'], total['Faces'])
   print_boxed(totals)
   name = filename.split('/')[-1].split('\\')[-1]
   message = "Successfully exported " + name + seconds
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

#============================================
#           Setup
#============================================

def set_up(scn, exp_list, matTable, worldTable):
   print "Setup"
   #Get selected Objects or if none selected all Objects from the current Scene
   if scn.objects.selected and guiTable['SELO'] == 1:
      objects = scn.objects.selected
   elif scn.objects:
      objects = scn.objects
   else:
      print "No Objects"
      return

   set_lists(exp_list, objects, matTable, worldTable)


def set_lists(exp_list, objects, matTable, worldTable):
   global mat_cnt
   mat_cnt = 0
   mat_index = 0
   #exp_list = [container1 = [ [mesh], [material_ref] ],...]

   for current_obj in objects:
      container = []
      if current_obj.getType() == 'Mesh':
         container.append(current_obj)

         mat_type = 0 #1=Material, 2=UV Images
         mat_ref = []
         mesh = current_obj.data
         mats_me = mesh.materials
         mats_ob = current_obj.getMaterials(0)
         #Find used Materials by Meshes or Objects
         if guiTable['MTL'] == 1 and mats_me or mats_ob: #Materials
            if mats_me:
               me_mats = mats_me
            elif mats_ob:
               me_mats = mats_ob
            mat_ref = -1

            for i,m in matTable.iteritems():
               for mat in me_mats:
                  if mat in m:
                     for amat in me_mats:
                        if amat not in m:
                           m.append(amat)
                     mat_ref = i
                     break

            if mat_ref < 0:
               matTable[mat_index] = me_mats
               mat_ref = mat_index
               mat_cnt+=1
               mat_index+=1
         container.append(mat_ref)
         exp_list.append(container)

   #If there is a world shader get some values
   world = Blender.World.GetCurrent()
   if world != None:
      worldAmb = world.getAmb()
      worldHor = world.getHor()

      worldTable['ambR'] = worldAmb[0]
      worldTable['ambG'] = worldAmb[1]
      worldTable['ambB'] = worldAmb[2]

      worldTable['horR'] = worldHor[0]
      worldTable['horG'] = worldHor[1]
      worldTable['horB'] = worldHor[2]



#============================================
#           Header/Scene
#============================================

def write_header(file, filename, scn, worldTable):
   print "Write Header"

   context = scn.getRenderingContext()

   file.write("*3DSMAX_ASCIIEXPORT%s200\n" % (Tab)) 
   file.write("*COMMENT \"Exported from Blender %s - %s\"\n" % (Blender.Get('version'), time.asctime(time.localtime()))) 
   file.write("*SCENE {\n") 
   #file.write("%s*SCENE_FILENAME \"%s\"\n" % (Tab, os.path.basename(Blender.Get('filename'))))
   name = Blender.Get('filename').split('/')[-1].split('\\')[-1] #Blender 2.44
   file.write("%s*SCENE_FILENAME \"%s\"\n" % (Tab, name))

   file.write("%s*SCENE_FIRSTFRAME %d\n" % (Tab,context.startFrame()))
   file.write("%s*SCENE_LASTFRAME %d\n" % (Tab,context.endFrame()))
   file.write("%s*SCENE_FRAMESPEED %d\n" % (Tab,context.framesPerSec()))
   file.write("%s*SCENE_TICKSPERFRAME 160\n" % (Tab)) #Blender has no Ticks?

   file.write("%s*SCENE_BACKGROUND_STATIC %.4f %.4f %.4f\n" % (Tab, worldTable['horR'], worldTable['horG'], worldTable['horB']))
   file.write("%s*SCENE_AMBIENT_STATIC %.4f %.4f %.4f\n" % (Tab, worldTable['ambR'], worldTable['ambG'], worldTable['ambB']))
   file.write("}\n") 


#============================================
#           Materials
#============================================

def write_materials(file, exp_list, worldTable, matTable):
   print "Write Materials"

   file.write("*MATERIAL_LIST {\n") 
   file.write("%s*MATERIAL_COUNT %s\n" % (Tab, mat_cnt))

   for i,m in matTable.iteritems():
      if len(m) == 1: # single mat
         mat_class = 'Standard'

         mats = m
         material = mats[0]
         mat_name = material.name

         file.write("%s*MATERIAL %d {\n" % ((Tab), i))

         idnt = 2
         mat_para(file, idnt, material, mat_name, mat_class, worldTable)
         mat_dummy(file, idnt)
         mat_map(file, idnt, mat_name)

         file.write("%s}\n" % (Tab))

      elif len(m) > 1: # multiple mat
         mat_class = 'Multi/Sub-Object'

         mats = m
         material = mats[0]
         mat_name = 'Multi # ' + material.name
         submat_no = len(mats)

         idnt = 2
         file.write("%s*MATERIAL %d {\n" % ((Tab), i))

         mat_para(file, idnt, material, mat_name, mat_class, worldTable)

         file.write("%s*NUMSUBMTLS %d\n" % ((Tab*idnt), submat_no))

         for submat_cnt,current_mat in enumerate(mats):
            material = current_mat
            mat_class = 'Standard'
            mat_name = material.name

            idnt = 2
            file.write("%s*SUBMATERIAL %d {\n" % ((Tab*idnt), submat_cnt))
            submat_cnt += 1

            idnt = 3
            mat_para(file, idnt, material, mat_name, mat_class, worldTable)
            mat_dummy(file, idnt)
            mat_map(file, idnt, mat_name)

            idnt = 2
            file.write("%s}\n" % (Tab*idnt))

         file.write("%s}\n" % (Tab))


   file.write("}\n")


def mat_para(file, idnt, material, mat_name, mat_class, worldTable):

   mat_amb = material.getAmb()
   mat_dif = material.getRGBCol()
   mat_specCol = material.getSpecCol()
   mat_spec = material.getSpec()
   mat_hard = material.getHardness()
   mat_alpha = 1.0000-material.getAlpha()

   file.write("%s*MATERIAL_NAME \"%s\"\n" % ((Tab*idnt), mat_name))
   file.write("%s*MATERIAL_CLASS \"%s\"\n" % ((Tab*idnt), mat_class))
   file.write("%s*MATERIAL_AMBIENT %.4f   %.4f   %.4f\n" % ((Tab*idnt), (worldTable['ambR']*mat_amb), (worldTable['ambG']*mat_amb), (worldTable['ambB']*mat_amb))) #-Usefull?
   file.write("%s*MATERIAL_DIFFUSE %.4f   %.4f   %.4f\n" % ((Tab*idnt), mat_dif[0], mat_dif[1], mat_dif[2]))
   file.write("%s*MATERIAL_SPECULAR %.4f   %.4f   %.4f\n" % ((Tab*idnt), mat_specCol[0], mat_specCol[1], mat_specCol[2]))
   file.write("%s*MATERIAL_SHINE %.4f\n" % ((Tab*idnt), mat_spec))
   file.write("%s*MATERIAL_SHINESTRENGTH %.4f\n" % ((Tab*idnt), (mat_hard/511.))) #-511 or 512?
   file.write("%s*MATERIAL_TRANSPARENCY %.4f\n" % ((Tab*idnt), mat_alpha))
   file.write("%s*MATERIAL_WIRESIZE 1.0000\n" % (Tab*idnt))


def mat_dummy(file, idnt):

   file.write("%s*MATERIAL_SHADING Blinn\n" % (Tab*idnt))
   file.write("%s*MATERIAL_XP_FALLOFF 0.0000\n" % (Tab*idnt))
   file.write("%s*MATERIAL_SELFILLUM 0.0000\n" % (Tab*idnt))
   file.write("%s*MATERIAL_FALLOFF In\n" % (Tab*idnt))
   file.write("%s*MATERIAL_XP_TYPE Filter\n" % (Tab*idnt))


def mat_map(file, idnt, mat_name):

   mapTable = {0:'*MAP_AMBIENT',1:'*MAP_DIFFUSE',2:'*MAP_SPECULAR',3:'*MAP_SHINE',4:'*MAP_SHINESTRENGTH',5:'*MAP_SELFILLUM',6:'*MAP_OPACITY',7:'*MAP_FILTERCOLOR',8:'*MAP_BUMP',9:'*MAP_REFLECT',10:'*MAP_REFRACT',11:'*MAP_REFRACT'}
   tex_list = [[],[],[],[],[],[],[],[],[],[],[],[]]

   mat = Material.Get(mat_name)
   MTexes = mat.getTextures()

   for current_MTex in MTexes:
      if current_MTex is not None:
         # MAP_SUBNO 0 = *MAP_AMBIENT
         if current_MTex.mapto & Texture.MapTo.AMB:
            map_getTex(current_MTex, 0, (current_MTex.dvar*current_MTex.varfac), tex_list)
         # MAP_SUBNO 1 = *MAP_DIFFUSE = COL = 1
         elif current_MTex.mapto & Texture.MapTo.COL:
            map_getTex(current_MTex, 1, current_MTex.colfac, tex_list)
         # MAP_SUBNO 2 = *MAP_SPECULAR (Color)= CSP or SPEC? = 4
         elif current_MTex.mapto & Texture.MapTo.CSP:
            map_getTex(current_MTex, 2, current_MTex.colfac, tex_list)
         # MAP_SUBNO 3 = *MAP_SHINE (Spec Level) = SPEC or CSP? = 32
         elif current_MTex.mapto & Texture.MapTo.SPEC:
            map_getTex(current_MTex, 3, (current_MTex.dvar*current_MTex.varfac), tex_list)
         # MAP_SUBNO 4 = *MAP_SHINESTRENGTH (Gloss) = HARD = 256
         elif current_MTex.mapto & Texture.MapTo.HARD:
            map_getTex(current_MTex, 4, (current_MTex.dvar*current_MTex.varfac), tex_list)
         # MAP_SUBNO 5 = *MAP_SELFILLUM
         # MAP_SUBNO 6 = *MAP_OPACITY = ALPHA = 128
         elif current_MTex.mapto & Texture.MapTo.ALPHA:
            map_getTex(current_MTex, 6, (current_MTex.dvar*current_MTex.varfac), tex_list)
         # MAP_SUBNO 7 = *MAP_FILTERCOLOR
         # MAP_SUBNO 8 = *MAP_BUMP = NOR = 2
         elif current_MTex.mapto & Texture.MapTo.NOR:
            map_getTex(current_MTex, 8, (current_MTex.norfac/25), tex_list)
         # MAP_SUBNO 9 = *MAP_REFLECT
         elif current_MTex.mapto & Texture.MapTo.REF:
            map_getTex(current_MTex, 9, (current_MTex.norfac/25), tex_list)
         # MAP_SUBNO 10 = *MAP_REFRACT (refraction)
         # MAP_SUBNO 11 = *MAP_REFRACT (displacement)
         elif current_MTex.mapto & Texture.MapTo.DISP:
            map_getTex(current_MTex, 11, (current_MTex.norfac/25), tex_list)

   # Write maps
   for current_LI in tex_list:
      subNo = tex_list.index(current_LI)
      for current_MTex in current_LI:
         tex = current_MTex[0].tex
         if tex.type == Texture.Types.IMAGE:
            map_image(file, idnt, current_MTex, subNo, tex, mapTable[subNo])


def map_getTex(MTex, map_subNo, map_amount, texes):
   # container = [[[MTex], [map_amount]], ...]
   container = []
   container.append(MTex)
   container.append(map_amount)
   texes[map_subNo].append(container)

         
def map_image(file, idnt, MTexCon, subNo, tex, mapType):

   img = tex.getImage()
   #path = sys.expandpath(img.getFilename()).replace('/', '\\')
   path = img.filename #or img.getFilename()
   tex_class = 'Bitmap'
   tex_mapType = 'Screen'
   tex_filter = 'Pyramidal'

   file.write("%s%s {\n" % ((Tab*idnt), mapType))

   idnt += 1
   file.write("%s*MAP_NAME \"%s\"\n" % ((Tab*idnt), tex.getName()))
   file.write("%s*MAP_CLASS \"%s\"\n" % ((Tab*idnt), tex_class))
   file.write("%s*MAP_SUBNO %s\n" % ((Tab*idnt), subNo))
   file.write("%s*MAP_AMOUNT %.4f\n" % ((Tab*idnt), MTexCon[1]))
   file.write("%s*BITMAP \"%s\"\n" % ((Tab*idnt), path))
   file.write("%s*MAP_TYPE %s\n" % ((Tab*idnt), tex_mapType))

   # hope this part is right!
   u_tiling = tex.repeat[0]*tex.crop[2]
   v_tiling = tex.repeat[1]*tex.crop[3]
   file.write("%s*UVW_U_OFFSET %.4f\n" % ((Tab*idnt), tex.crop[0]))
   file.write("%s*UVW_V_OFFSET %.4f\n" % ((Tab*idnt), tex.crop[1]))
   file.write("%s*UVW_U_TILING %.4f\n" % ((Tab*idnt), u_tiling))
   file.write("%s*UVW_V_TILING %.4f\n" % ((Tab*idnt), v_tiling))

   map_uvw(file, idnt) #hardcoded

   file.write("%s*BITMAP_FILTER %s\n" % ((Tab*idnt), tex_filter))

   idnt -= 1
   file.write("%s}\n" % (Tab*idnt))


def mat_uv(file, idnt, uv_image, uv_name, mat_class, worldTable):
   fake_val0 = '0.0000'
   fake_val1 = '0.1000'
   fake_val2 = '0.5882'
   fake_val3 = '0.9000'
   fake_val4 = '1.0000'

   file.write("%s*MATERIAL_NAME \"%s\"\n" % ((Tab*idnt), uv_name))
   file.write("%s*MATERIAL_CLASS \"%s\"\n" % ((Tab*idnt), mat_class))
   file.write("%s*MATERIAL_AMBIENT %.4f   %.4f   %.4f\n" % ((Tab*idnt), worldTable['ambR'], worldTable['ambG'], worldTable['ambB'])) #------------Usefull?
   file.write("%s*MATERIAL_DIFFUSE %s   %s   %s\n" % ((Tab*idnt), fake_val2, fake_val2, fake_val2))
   file.write("%s*MATERIAL_SPECULAR %s   %s   %s\n" % ((Tab*idnt), fake_val3, fake_val3, fake_val3))
   file.write("%s*MATERIAL_SHINE %s\n" % ((Tab*idnt), fake_val1))
   file.write("%s*MATERIAL_SHINESTRENGTH %s\n" % ((Tab*idnt), fake_val0))
   file.write("%s*MATERIAL_TRANSPARENCY %s\n" % ((Tab*idnt), fake_val0))
   file.write("%s*MATERIAL_WIRESIZE %s\n" % ((Tab*idnt), fake_val4))


def map_uv(file, idnt, uv_image, uv_name):
   map_type = '*MAP_DIFFUSE'
   map_subNo = '1'
   tex_class = 'Bitmap'
   tex_mapType = 'Screen'
   tex_filter = 'Pyramidal'

   fake_val0 = '0.0000'
   fake_val1 = '0.1000'
   fake_val2 = '0.5882'
   fake_val3 = '0.9000'
   fake_val4 = '1.0000'

   #replace "/" with "\" in image path
   uv_filename = uv_image.getFilename().replace('/', '\\')

   file.write("%s%s {\n" % ((Tab*idnt), map_type))

   idnt += 1
   file.write("%s*MAP_NAME \"%s\"\n" % ((Tab*idnt), uv_name))
   file.write("%s*MAP_CLASS \"%s\"\n" % ((Tab*idnt), tex_class))
   file.write("%s*MAP_SUBNO %s\n" % ((Tab*idnt), map_subNo))
   file.write("%s*MAP_AMOUNT %s\n" % ((Tab*idnt), fake_val4))
   file.write("%s*BITMAP \"%s\"\n" % ((Tab*idnt), uv_filename))
   file.write("%s*MAP_TYPE %s\n" % ((Tab*idnt), tex_mapType))
   file.write("%s*UVW_U_OFFSET %s\n" % ((Tab*idnt), fake_val0))
   file.write("%s*UVW_V_OFFSET %s\n" % ((Tab*idnt), fake_val0))
   file.write("%s*UVW_U_TILING %s\n" % ((Tab*idnt), fake_val4))
   file.write("%s*UVW_V_TILING %s\n" % ((Tab*idnt), fake_val4))

   map_uvw(file, idnt) #hardcoded

   file.write("%s*BITMAP_FILTER %s\n" % ((Tab*idnt), tex_filter))

   idnt -= 1
   file.write("%s}\n" % (Tab*idnt))


def map_uvw(file, idnt):

   fake_val0 = '0.0000'
   fake_val1 = '1.0000'

   file.write("%s*UVW_ANGLE %s\n" % ((Tab*idnt), fake_val0))
   file.write("%s*UVW_BLUR %s\n" % ((Tab*idnt), fake_val1))
   file.write("%s*UVW_BLUR_OFFSET %s\n" % ((Tab*idnt), fake_val0))
   file.write("%s*UVW_NOUSE_AMT %s\n" % ((Tab*idnt), fake_val1))
   file.write("%s*UVW_NOISE_SIZE %s\n" % ((Tab*idnt), fake_val1))
   file.write("%s*UVW_NOISE_LEVEL 1\n" % (Tab*idnt))
   file.write("%s*UVW_NOISE_PHASE %s\n" % ((Tab*idnt), fake_val0))


#============================================
#           Mesh
#============================================


def write_mesh(file, scn, exp_list, matTable, total):
   print "Write Geometric"

   for current_container in exp_list:

      TransTable = {'SizeX': 1, 'SizeY': 1, 'SizeZ': 1}
      nameMe = {'objName': 'obj', 'meName': 'me'}
      sGroups = {}
      hasTable = {'hasMat': 0, 'hasSG': 0, 'hasUV': 0, 'hasVC': 0, 'matRef': 0}
      count = {'face': 0, 'vert': 0, 'UVs': 0, 'cVert': 0}

      obj = current_container[0]
      #mat_ref = current_container[1]
      data = obj.getData(0,1)
      nameMe['objName'] = obj.name
      nameMe['meName'] = data.name

      mats_me = [mat for mat in data.materials if mat] #fix for 2.44, get rid of NoneType Objects in me.materials
      mats_ob = obj.getMaterials(0)
      materials = False

      if mats_me:
         materials = mats_me
      elif mats_ob:
         materials = mats_ob

      if guiTable['MTL'] and materials:
         hasTable['hasMat'] = 1
         hasTable['matRef'] = current_container[1]

      if obj.getParent():
         nameMe['parent'] = obj.getParent().name
      
      me = Mesh.New()      # Create a new mesh

      if guiTable['MOD']:   # Use modified mesh
         me.getFromObject(obj.name, 0) # Get the object's mesh data, cage 0 = apply mod
      else:
         me.getFromObject(obj.name, 1)

      me.transform(obj.matrix)   # ASE stores transformed mesh data
      if guiTable['RECENTER']:   # Recentre Objects to 0,0,0 feature
         rec_matrix = Mathutils.TranslationMatrix(obj.matrix.translationPart().negate())
         me.transform(rec_matrix)

      tempObj = Blender.Object.New('Mesh', 'ASE_export_temp_obj')
      tempObj.setMatrix(obj.matrix)
      tempObj.link(me)

      if guiTable['VG2SG']:
         VGNames = data.getVertGroupNames()
         for vg in VGNames:
            me.addVertGroup(vg)
            gverts = data.getVertsFromGroup(vg, 1)
            gverts_copy = []
            for gv in gverts:
               gverts_copy.append(gv[0])
            me.assignVertsToGroup(vg, gverts_copy, 1, 1)

      obj = tempObj
      faces = me.faces
      verts = me.verts

      count['vert'] = len(verts)
      total['Verts'] += count['vert']

      if count['vert'] == 0:
         print 'Error: ' + nameMe['meName'] + 'has 0 Verts'
         continue

      vGroups = me.getVertGroupNames()
      if guiTable['VG2SG'] and len(vGroups) > 0:
         for current_VG in vGroups:
            if current_VG.lower().count("smooth."):
               hasTable['hasSG'] = 1
               smooth_num = int(current_VG.lower().replace("smooth.", ""))
               gverts = me.getVertsFromGroup(current_VG)
               for vi in gverts:
                  if not sGroups.has_key(vi):
                     sGroups[vi] = [smooth_num]
                  else:
                     sGroups[vi].append(smooth_num)

      if guiTable['UV']:
         if me.faceUV == True or me.faceUV == 1:
            hasTable['hasUV'] = 1

      if guiTable['VC']:
         if me.vertexColors:
            hasTable['hasVC'] = 1
         elif hasTable['hasMat']: # Blender material
            for current_mat in materials:
               if current_mat.getMode() & Material.Modes['VCOL_PAINT']:
                  hasTable['hasVC'] = 1
                  break

      for current_face in faces:
         if len(current_face.verts) is 3:
            count['face'] += 1
            total['Tris'] += 1
            total['Faces'] += 1
         elif len(current_face.verts) is 4:
            count['face'] += 2
            total['Tris'] += 2
            total['Faces'] += 1

      #Open Geomobject
      file.write("*GEOMOBJECT {\n")
      file.write("%s*NODE_NAME \"%s\"\n" % (Tab, nameMe['objName']))

      if nameMe.has_key('parent'):
         file.write("%s*NODE_PARENT \"%s\"\n" % (Tab, nameMe['parent']))

      idnt = 1
      mesh_matrix(file, idnt, obj, nameMe, TransTable)

      #Open Mesh
      file.write("%s*MESH {\n" % (Tab))

      idnt = 2 
      file.write("%s*TIMEVALUE 0\n" % (Tab*idnt))
      file.write("%s*MESH_NUMVERTEX %i\n" % ((Tab*idnt), count['vert']))
      file.write("%s*MESH_NUMFACES %i\n" % ((Tab*idnt), count['face']))

      idnt = 2
      mesh_vertexList(file, idnt, verts, count)
      idnt = 2
      mesh_faceList(file, idnt, me, materials, sGroups, faces, matTable, hasTable, count)


      if hasTable['hasUV'] == 1:
         UVTable = {}

         active_map_channel = me.activeUVLayer
         map_channels = me.getUVLayerNames()

         idnt = 2
         mesh_tVertList(file, idnt, faces, UVTable, count)
         #idnt = 2
         mesh_tFaceList(file, idnt, faces, UVTable, count)
         UVTable = {}
         
         if len(map_channels) > 1:
            chan_index = 2
            for map_chan in map_channels:
               if map_chan != active_map_channel:
                  me.activeUVLayer = map_chan

                  idnt = 2
                  file.write("%s*MESH_MAPPINGCHANNEL %i {\n" % ((Tab*idnt), chan_index))
                  idnt = 3
                  mesh_tVertList(file, idnt, faces, UVTable, count)
                  mesh_tFaceList(file, idnt, faces, UVTable, count)
                  UVTable = {}
                  chan_index += 1
                  idnt = 2
                  file.write("%s}\n" % (Tab*idnt))

         me.activeUVLayer = active_map_channel

      else:
      # dirty fix
         file.write("%s*MESH_NUMTVERTEX %i\n" % ((Tab*idnt), count['UVs']))

      if hasTable['hasVC'] == 1:
         cVertTable = {}

         idnt = 2
         mesh_cVertList(file, idnt, faces, cVertTable, count)
         #idnt = 2
         mesh_cFaceList(file, idnt, faces, cVertTable, count)
      else:
      # dirty fix
         file.write("%s*MESH_NUMCVERTEX %i\n" % ((Tab*idnt), count['cVert']))


      idnt = 2
      mesh_normals(file, idnt, faces, verts, count)

      # Close *MESH
      idnt = 1
      file.write("%s}\n" % (Tab*idnt))

      idnt = 1
      mesh_footer(file, idnt, hasTable)

      # Close *GEOMOBJECT
      file.write("}\n")
      
      #free some memory
      me.materials = [None]
      me.faces.delete(1,[(f.index) for f in me.faces])
      me.verts.delete(me.verts)
      obj.fakeUser = False
      me.fakeUser = False
      scn.objects.unlink(obj)

def mesh_matrix(file, idnt, obj, nameMe, TransTable):

   #i should check why i have to get and invert the matrix
   #exactly in that sequence.

   row = obj.getMatrix('localspace').invert()
   #row = obj.getInverseMatrix()

   if guiTable['RECENTER']:
      location = 0.0,0.0,0.0
      row[3][0] = row[3][1] = row[3][2] = 0.0
   else:
      location = obj.getLocation()

   quat = row.invert().toQuat()
   #quat = obj.getMatrix('localspace').toQuat()
   rota = quat.axis
   #angle = quat.angle * (math.pi/180) #Blender: degrees -> ASE: radians
   angle = math.radians(quat.angle)

   Blender.Window.DrawProgressBar(0.0, "Writing Transform Node")

   file.write("%s*NODE_TM {\n" % (Tab*idnt))

   idnt += 1
   file.write("%s*NODE_NAME \"%s\"\n" % ((Tab*idnt), nameMe['meName']))
   # Inherit from what?..
   file.write("%s*INHERIT_POS 0 0 0\n" % (Tab*idnt))
   file.write("%s*INHERIT_ROT 0 0 0\n" % (Tab*idnt)) 
   file.write("%s*INHERIT_SCL 0 0 0\n" % (Tab*idnt)) 

   file.write("%s*TM_ROW0 %.4f %.4f %.4f\n" % ((Tab*idnt), row[0][0], row[0][1], row[0][2]))
   file.write("%s*TM_ROW1 %.4f %.4f %.4f\n" % ((Tab*idnt), row[1][0], row[1][1], row[1][2]))
   file.write("%s*TM_ROW2 %.4f %.4f %.4f\n" % ((Tab*idnt), row[2][0], row[2][1], row[2][2]))
   file.write("%s*TM_ROW3 %.4f %.4f %.4f\n" % ((Tab*idnt), row[3][0], row[3][1], row[3][2]))

   file.write("%s*TM_POS %.4f %.4f %.4f\n" % ((Tab*idnt), location[0], location[1], location[2]))

   file.write("%s*TM_ROTAXIS %.4f %.4f %.4f\n" % ((Tab*idnt), rota.x, rota.y, rota.z))
   file.write("%s*TM_ROTANGLE %.4f\n" % ((Tab*idnt), angle))

   file.write("%s*TM_SCALE %.4f %.4f %.4f\n" % ((Tab*idnt), TransTable['SizeX'], TransTable['SizeY'], TransTable['SizeZ']))
   #file.write("%s*TM_SCALEAXIS 0.0000 0.0000 0.0000\n" % (Tab*idnt))
   # Looks more logic, because blender use the rotaxis for rot and scale:
   file.write("%s*TM_SCALEAXIS %.4f %.4f %.4f\n" % ((Tab*idnt), rota.x, rota.y, rota.z))
   file.write("%s*TM_SCALEAXISANG %.4f\n" % ((Tab*idnt), angle))

   idnt -= 1
   file.write("%s}\n" % (Tab*idnt)) 


def mesh_vertexList(file, idnt, verts, count):

   file.write("%s*MESH_VERTEX_LIST {\n" % (Tab*idnt)) 

   idnt += 1 

   Blender.Window.DrawProgressBar(0.0, "Writing vertices")

   for current_vert in verts:

      vIndex = current_vert.index

      if (vIndex % 1000) == 0: 
                   Blender.Window.DrawProgressBar((vIndex+1.0) / count['vert'], "Writing vertices")

      file.write("%s*MESH_VERTEX %d\t%.4f\t%.4f\t%.4f\n" % ((Tab*idnt), vIndex, current_vert.co[0], current_vert.co[1], current_vert.co[2]))

   idnt -= 1 
   file.write("%s}\n" % (Tab*idnt)) 


def mesh_faceList(file, idnt, me, materials, sGroups, faces, matTable, hasTable, count):

   file.write("%s*MESH_FACE_LIST {\n" % (Tab*idnt)) 
   idnt += 1
   faceNo = 0

   Blender.Window.DrawProgressBar(0.0, "Writing faces")
   if hasTable['hasMat'] and matTable:
      mats = matTable[hasTable['matRef']]

   fgon_eds = [(ed.key) for ed in me.edges if ed.flag & Mesh.EdgeFlags.FGON]
   for current_face in faces:

      face_verts = current_face.verts
      smooth = '*MESH_SMOOTHING'
      matID = '*MESH_MTLID 0'

      if (faceNo % 500) == 0:
         Blender.Window.DrawProgressBar((faceNo+1.0) / count['face'], "Writing faces")

      if hasTable['hasMat']: # Blender mats
         #print current_face.mat
         mtlid = mats.index(materials[current_face.mat])
         matID = '*MESH_MTLID %i' % (mtlid)

      if len(face_verts) is 3:
         vert0 = face_verts[0].index
         vert1 = face_verts[1].index
         vert2 = face_verts[2].index

         #Find hidden (fgon) edges
         edge_keys = current_face.edge_keys
         eds_fgon = [1,1,1]
         for i,ed_key in enumerate(edge_keys):
            if ed_key in fgon_eds:
               eds_fgon[i] = 0

         #Find Smoothgroups for this face:
         if guiTable['VG2SG'] and hasTable['hasSG'] and current_face.smooth:
            if sGroups.has_key(vert0) and sGroups.has_key(vert1) and sGroups.has_key(vert2):
               sg = []
               gis = [sGroups[vert0],sGroups[vert1],sGroups[vert2]]
               for gil in gis:
                  for gi in gil:
                     sg.append(gi)
               sg = set(sg)
               for gi in sg:
                  smooth += ' %s,' % gi
               smooth = smooth[:-1]

         elif current_face.smooth:
            smooth += ' 1'

         file.write("%s*MESH_FACE %i:    A: %i B: %i C: %i AB:    %i BC:    %i CA:    %i\t %s \t%s\n" % ((Tab*idnt), faceNo, vert0, vert1, vert2, eds_fgon[0], eds_fgon[1], eds_fgon[2], smooth, matID))
         faceNo+=1

      elif len(face_verts) is 4:
         vert0 = face_verts[0].index
         vert1 = face_verts[1].index
         vert2 = face_verts[2].index
         vert3 = face_verts[3].index

         #Find hidden (fgon) edges
         edge_keys = current_face.edge_keys
         eds_fgon = [1,1,1,1]
         for i,ed_key in enumerate(edge_keys):
            if ed_key in fgon_eds:
               eds_fgon[i] = 0

         #Find Smooth for this face:
         if guiTable['VG2SG'] and hasTable['hasSG'] and current_face.smooth:
            if sGroups.has_key(vert0) and sGroups.has_key(vert1) and sGroups.has_key(vert2) and sGroups.has_key(vert3):
               ## I hate VG2SG ;> not sure which way is correct
               #sg0,sg1,sg2,sg3 = sGroups[vert0],sGroups[vert1],sGroups[vert2],sGroups[vert3]
               #if sg0 == sg1 == sg2 == sg3:
               #   sg = sg0
               #else:
               #   lens = [len(sg0),len(sg1),len(sg2),len(sg3)]
               #   lens_sort = lens
               #   lens_sort.sort()
               #   lowest = lens_sort[0]
               #   for l,s in zip(lens,[sg0,sg1,sg2,sg2]):
               #      if l == lowest:
               #         sg = s
               #         break

               sg = []
               gis = [sGroups[vert0],sGroups[vert1],sGroups[vert2],sGroups[vert3]]
               for gil in gis:
                  for gi in gil:
                     sg.append(gi)
               sg = set(sg)
               for gi in sg:
                  smooth += ' %s,' % gi
               smooth = smooth[:-1]
            else:
               smooth += ' 1'

         elif current_face.smooth:
            smooth += ' 1'

         file.write("%s*MESH_FACE %i:    A: %i B: %i C: %i AB:    %i BC:    %i CA:    0\t %s \t%s\n" % ((Tab*idnt), faceNo, vert0, vert1, vert2, eds_fgon[0], eds_fgon[1], smooth, matID))
         faceNo+=1
         file.write("%s*MESH_FACE %i:    A: %i B: %i C: %i AB:    %i BC:    %i CA:    0\t %s \t%s\n" % ((Tab*idnt), faceNo, vert0, vert2, vert3, eds_fgon[1], eds_fgon[2], smooth, matID))
         faceNo+=1

   idnt -= 1
   file.write("%s}\n" % (Tab*idnt))

def mesh_tVertList(file, idnt, faces, UVTable, count):

   Blender.Window.DrawProgressBar(0.0, "Setup UV index")

   for current_face in faces:
      faceuv = current_face.uv
      for current_uv in faceuv:
         uv = (current_uv.x, current_uv.y)
         if not UVTable.has_key(uv):
            UVTable[uv] = 0
            count['UVs'] += 1

   #count['UVs'] = len(UVTable)
   file.write("%s*MESH_NUMTVERTEX %d\n" % ((Tab*idnt), count['UVs']))
   file.write("%s*MESH_TVERTLIST {\n" % (Tab*idnt))

   idnt += 1
   Blender.Window.DrawProgressBar(0.0, "Writing UV index")

   for index,current_UV in enumerate(UVTable.iterkeys()):
      if (index % 1000) == 0:
         Blender.Window.DrawProgressBar((index+1.0) / count['face'], "Writing UV index")

      file.write("%s*MESH_TVERT %i\t%.4f\t%.4f\t0.0000\n" % ((Tab*idnt), index, current_UV[0], current_UV[1]))
      UVTable[current_UV] = index

   idnt -= 1
   file.write("%s}\n" % (Tab*idnt))


def mesh_tFaceList(file, idnt, faces, UVTable, count):

   tfaceNo = 0

   Blender.Window.DrawProgressBar(0.0, "Writing Face UV")

   file.write("%s*MESH_NUMTVFACES %i\n" % ((Tab*idnt), count['face']))
   file.write("%s*MESH_TFACELIST {\n" % (Tab*idnt))

   idnt += 1

   for current_face in faces:

      faceUV = current_face.uv

      if (tfaceNo % 1000) == 0:
         Blender.Window.DrawProgressBar((tfaceNo+1.0) / count['face'], "Writing Face UV")

      if len(faceUV) is 3: #tri
         UV0 = UVTable[(faceUV[0].x, faceUV[0].y)]
         UV1 = UVTable[(faceUV[1].x, faceUV[1].y)]
         UV2 = UVTable[(faceUV[2].x, faceUV[2].y)]
         file.write("%s*MESH_TFACE %i\t%i\t%i\t%d\n" % ((Tab*idnt), tfaceNo, UV0, UV1, UV2))
         tfaceNo+=1

      elif len(faceUV) is 4: #quad
         UV0 = UVTable[(faceUV[0].x, faceUV[0].y)]
         UV1 = UVTable[(faceUV[1].x, faceUV[1].y)]
         UV2 = UVTable[(faceUV[2].x, faceUV[2].y)]
         UV3 = UVTable[(faceUV[3].x, faceUV[3].y)]
         file.write("%s*MESH_TFACE %i\t%i\t%i\t%i\n" % ((Tab*idnt), tfaceNo, UV0, UV1, UV2))
         tfaceNo+=1
         file.write("%s*MESH_TFACE %i\t%i\t%i\t%i\n" % ((Tab*idnt), tfaceNo, UV0, UV2, UV3))
         tfaceNo+=1

   idnt -= 1
   file.write("%s}\n" % (Tab*idnt))


def mesh_cVertList(file, idnt, faces, cVertTable, count):

   Blender.Window.DrawProgressBar(0.0, "Setup VCol index")

   for current_face in faces:
      facecol = current_face.col
      for current_col in facecol:
         col = (current_col.r, current_col.g, current_col.b)
         if not cVertTable.has_key(col):
            cVertTable[col] = 0
            count['cVert'] += 1

   file.write("%s*MESH_NUMCVERTEX %i\n" % ((Tab*idnt), count['cVert']))
   file.write("%s*MESH_CVERTLIST {\n" % (Tab*idnt))

   idnt += 1

   Blender.Window.DrawProgressBar(0.0, "Writing VCol index")

   for index,current_cvert in enumerate(cVertTable.iterkeys()):
      if (index % 1000) == 0:
         Blender.Window.DrawProgressBar((index+1.0) / count['face'], "Writing VCol index")

      file.write("%s*MESH_VERTCOL %i\t%.4f\t%.4f\t%.4f\n" % ((Tab*idnt), index, (current_cvert[0]/256.), (current_cvert[1]/256.), (current_cvert[2]/256.)))
      cVertTable[current_cvert] = index

   idnt -= 1
   file.write("%s}\n" % (Tab*idnt))


def mesh_cFaceList(file, idnt, faces, cVertTable, count):

   cFaceNo = 0

   Blender.Window.DrawProgressBar(0.0, "Writing Face Colors")

   file.write("%s*MESH_NUMCFACES %i\n" % ((Tab*idnt), count['face']))
   file.write("%s*MESH_CFACELIST {\n" % (Tab*idnt))

   idnt += 1
   for current_face in faces:

      if (cFaceNo % 500) == 0:
         Blender.Window.DrawProgressBar((cFaceNo+1.0) / count['face'], "Writing Face Colors")

      if len(current_face.verts) is 3: #tri
         color0 = cVertTable[(current_face.col[0].r, current_face.col[0].g, current_face.col[0].b)]
         color1 = cVertTable[(current_face.col[1].r, current_face.col[1].g, current_face.col[1].b)]
         color2 = cVertTable[(current_face.col[2].r, current_face.col[2].g, current_face.col[2].b)]

         file.write("%s*MESH_CFACE %i\t%i\t%i\t%i\n" % ((Tab*idnt), cFaceNo, color0, color1, color2))
         cFaceNo+= 1

      elif len(current_face.verts) is 4: #quad
         color0 = cVertTable[(current_face.col[0].r, current_face.col[0].g, current_face.col[0].b)]
         color1 = cVertTable[(current_face.col[1].r, current_face.col[1].g, current_face.col[1].b)]
         color2 = cVertTable[(current_face.col[2].r, current_face.col[2].g, current_face.col[2].b)]
         color3 = cVertTable[(current_face.col[3].r, current_face.col[3].g, current_face.col[3].b)]

         file.write("%s*MESH_CFACE %i\t%i\t%i\t%i\n" % ((Tab*idnt), cFaceNo, color0, color1, color2))
         cFaceNo+= 1
         file.write("%s*MESH_CFACE %i\t%i\t%i\t%i\n" % ((Tab*idnt), cFaceNo, color0, color2, color3))
         cFaceNo+= 1

   idnt -= 1
   file.write("%s}\n" % (Tab*idnt))


def mesh_normals(file, idnt, faces, verts, count):
   # To export quads it is needed to calculate all face and vertex normals new!
   vec_null = Blender.Mathutils.Vector(0.0, 0.0, 0.0)
   v_normals = dict([(v.index, vec_null) for v in verts])
   f_normals = dict([(f.index, vec_null) for f in faces])
   f_normals_quad = {}

   file.write("%s*MESH_NORMALS {\n" % (Tab*idnt))

   Blender.Window.DrawProgressBar(0.0, "Setup Normals")

   #-- Calculate new face and vertex normals

   for i,f in enumerate(faces):
      f_dic = f_normals[i]
      f_vec = f_dic[0]

      f_verts = f.verts

      if len(f_verts) is 3: #tri
         v0,v1,v2 = f_verts[:]
         v0_i,v1_i,v2_i = f_verts[0].index, f_verts[1].index, f_verts[2].index
         f_no = Blender.Mathutils.TriangleNormal(v0.co, v1.co, v2.co)
         f_normals[f.index] = f_no
         if f.smooth:
            v_normals[v0_i] = v_normals[v0_i] + f_no
            v_normals[v1_i] = v_normals[v1_i] + f_no
            v_normals[v2_i] = v_normals[v2_i] + f_no

      if len(f_verts) is 4: #quad
         v0,v1,v2,v3 = f_verts[:]
         v0_i,v1_i,v2_i,v3_i = f_verts[0].index, f_verts[1].index, f_verts[2].index,f_verts[3].index
         f_no0 = Blender.Mathutils.TriangleNormal(v0.co, v1.co, v2.co)
         f_no1 = Blender.Mathutils.TriangleNormal(v2.co, v3.co, v0.co)
         f_normals[f.index] = f_no0
         f_normals_quad[f.index] = f_no1
         if f.smooth:
            v_normals[v0_i] = v_normals[v0_i] + f_no0
            v_normals[v1_i] = v_normals[v1_i] + f_no0
            v_normals[v2_i] = v_normals[v2_i] + f_no0
            
            v_normals[v0_i] = v_normals[v2_i] + f_no1
            v_normals[v2_i] = v_normals[v3_i] + f_no1
            v_normals[v3_i] = v_normals[v0_i] + f_no1


   #-- Normalize vectors
   #for i,vec in v_normals.iteritems():
   for vec in v_normals.itervalues():
      vec.normalize()

   #-- Finally write normals
   normNo = 0
   idnt += 2

   Blender.Window.DrawProgressBar(0.0, "Writing Normals")

   for f in faces:

      if (normNo % 500) == 0:
         Blender.Window.DrawProgressBar((normNo+1.0) / count['face'], "Writing Normals")

      f_verts = f.verts
      smooth = f.smooth

      if len(f_verts) is 3: #tri
         v0_i = f_verts[0].index
         v1_i = f_verts[1].index
         v2_i = f_verts[2].index

         idnt -= 1
         f_no = f_normals[f.index]
         file.write("%s*MESH_FACENORMAL %i\t%.4f\t%.4f\t%.4f\n" % ((Tab*idnt), normNo, f_no.x, f_no.y, f_no.z))
         normNo += 1

         idnt += 1
         mesh_vertNorm(file, idnt, v0_i, v1_i, v2_i, v_normals, smooth, f_no)

      #elif len(f_verts) is 4: #quad
      if len(f_verts) is 4: #quad
         v0_i = f_verts[0].index
         v1_i = f_verts[1].index
         v2_i = f_verts[2].index
         v3_i = f_verts[3].index

         idnt -= 1
         f_no = f_normals[f.index]
         file.write("%s*MESH_FACENORMAL %i\t%.4f\t%.4f\t%.4f\n" % ((Tab*idnt), normNo, f_no0.x, f_no0.y, f_no0.z))
         normNo += 1

         idnt += 1
         mesh_vertNorm(file, idnt, v0_i, v1_i, v2_i, v_normals, smooth, f_no0)

         idnt -= 1
         f_no = f_normals_quad[f.index]
         file.write("%s*MESH_FACENORMAL %i\t%.4f\t%.4f\t%.4f\n" % ((Tab*idnt), normNo, f_no1.x, f_no1.y, f_no1.z))
         normNo += 1

         idnt += 1
         mesh_vertNorm(file, idnt, v0_i, v2_i, v3_i, v_normals, smooth, f_no1)


   idnt -= 2
   file.write("%s}\n" % (Tab*idnt))
   
def mesh_vertNorm(file, idnt, v0_i, v1_i, v2_i, v_normals, smooth, f_no):
   if smooth:
      v_no0 = v_normals[v0_i]
      v_no1 = v_normals[v1_i]
      v_no2 = v_normals[v2_i]
   else: #If solid use the face normal
      v_no0 = v_no1 = v_no2 = f_no

   file.write("%s*MESH_VERTEXNORMAL %i\t%.4f\t%.4f\t%.4f\n" % ((Tab*idnt), v0_i, v_no0.x, v_no0.y, v_no0.z))
   file.write("%s*MESH_VERTEXNORMAL %i\t%.4f\t%.4f\t%.4f\n" % ((Tab*idnt), v1_i, v_no1.x, v_no1.y, v_no1.z))
   file.write("%s*MESH_VERTEXNORMAL %i\t%.4f\t%.4f\t%.4f\n" % ((Tab*idnt), v2_i, v_no2.x, v_no2.y, v_no2.z))


def mesh_footer(file, idnt, hasTable):

   file.write("%s*PROP_MOTIONBLUR 0\n" % (Tab*idnt))
   file.write("%s*PROP_CASTSHADOW 1\n" % (Tab*idnt))
   file.write("%s*PROP_RECVSHADOW 1\n" % (Tab*idnt))

   if hasTable['hasMat'] != 0:
      file.write("%s*MATERIAL_REF %i\n" % ((Tab*idnt), hasTable['matRef']))

   #-------------------------End?----------------------


def write_ui(filename):

   global guiTable, EXPORT_MOD, EXPORT_MTL, EXPORT_UV, EXPORT_VC, EXPORT_SELO, EXPORT_UVI, EXPORT_VG2SG
   guiTable = {'MOD': 1, 'MTL': 1, 'UV': 1, 'VC': 1, 'SELO': 1, 'UVI': 0, 'VG2SG': 1, 'RECENTER':0}

   EXPORT_MOD = Draw.Create(guiTable['MOD'])
   EXPORT_MTL = Draw.Create(guiTable['MTL'])
   EXPORT_UV = Draw.Create(guiTable['UV'])
   EXPORT_VC = Draw.Create(guiTable['VC'])
   EXPORT_SELO = Draw.Create(guiTable['SELO'])
   EXPORT_VG2SG = Draw.Create(guiTable['VG2SG'])
   EXPORT_REC = Draw.Create(guiTable['RECENTER'])

   # Get USER Options
   pup_block = [('Mesh Options...'),('Apply Modifiers', EXPORT_MOD, 'Use modified mesh data from each object.'),('Materials', EXPORT_MTL, 'Export Materials.'),('Face UV', EXPORT_UV, 'Export texface UV coords.'),('Vertex Colors', EXPORT_VC, 'Export vertex colors'),('Context...'),('Selection Only', EXPORT_SELO, 'Only export objects in visible selection, else export all mesh object.'),('Bonus...'),('VertGr. as SmoothGr.', EXPORT_VG2SG, 'Make SmoothGroups by VertGroups. See doc.'), ('Center Objects', EXPORT_REC, 'Center ALL objects to World-Grid-Origin-Center-Point-(0,0,0). ;)')]

   if not Draw.PupBlock('Export...', pup_block):
      return

   Window.WaitCursor(1)

   guiTable['MOD'] = EXPORT_MOD.val
   guiTable['MTL'] = EXPORT_MTL.val
   guiTable['UV'] = EXPORT_UV.val
   guiTable['VC'] = EXPORT_VC.val
   guiTable['SELO'] = EXPORT_SELO.val
   guiTable['VG2SG'] = EXPORT_VG2SG.val
   guiTable['RECENTER'] = EXPORT_REC.val

   if not filename.lower().endswith('.ase'):
      filename += '.ase'

   write(filename)

   Window.WaitCursor(0)


if __name__ == '__main__':
   Window.FileSelector(write_ui, 'Export ASCII Scene', sys.makename(ext='.ase'))