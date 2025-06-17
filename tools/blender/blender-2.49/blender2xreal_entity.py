#!BPY
"""
Name: 'View/Modify Entity Data'
Blender: 248
Group: 'Add'
"""

#blendXmap Entity Data Scipt
#allows user to set further data for objects threatend as entitys


import blender2xreal_tools
from blender2xreal_tools import *
      
global keys, values, info, probs, ob, delete

ob = 0		
keys = []
values = []
probs = 0


info = []
delete = []
	
		
def  Blend2X_NewEntry(a,b):
	key = Blender.Draw.Create("")
	value = Blender.Draw.Create("")
  
	block = []  
	block.append(("Key: ", key, 0, 30, ""))
	block.append(("Value: ", value, 0, 30, ""))
	
	if ( Blender.Draw.PupBlock("New Entity Data:", block)  == 1):
		if ( key.val != ""):
			if ( value.val != ""):
				ob.addProperty(key.val, value.val,  'STRING')
			else:
				alert("Invalid value!")
		else:
			alert("Invalid key!")
		

	
		
def Blend2X_EntityInspectorDraw():
	global keys, values, info, probs, ob,delete
	
	props = ob.getAllProperties()
		
	width = 250
	height = 80 + ( 20 * len(props))

	xmouse, ymouse = Blender.Window.GetMouseCoords()
 
	xbase = xmouse - (width / 2)
	ybase = ymouse - (height / 2)
	
	i = 0
	

	Blender.Draw.Label(""  , xbase , ybase,width, height)
	ybase = ybase + height
	Blender.Draw.Label("Entity Inspector: %s" % ob.name , xbase , ybase,width, 20)
	ybase = ybase - 5
	Blender.Draw.Label("__________________________________" , xbase , ybase,width, 20)


	ybase = ybase - 40

	
	for prop in props:
		
		keys.append(prop.name)
		values.append(prop.data)
		
		info.append(Blender.Draw.Create(values[i]))	
		delete.append(Blender.Draw.Create(0))	
		
		info[i] = Draw.String(("%s: " % keys[i]), 0, xbase+10, ybase, 200, 18, info[i].val, 40)
		delete[i] = Draw.Toggle("delete", 0, xbase+220, ybase, 100, 20 ,delete[i].val)
	
		ybase = ybase - 22
		i = i + 1	
	
	ybase = ybase - 22
	
	Draw.PushButton("add", 1, xbase+200, ybase, 120, 20, "", Blend2X_NewEntry)
	
	
	
		
def Blend2X_EntityInspector():
	global keys, values, info, probs, ob
	
	objects = Object.GetSelected()
	
	if len(objects) <= 0:
			alert("no object selected..")
			return
	ob = objects[0]
	obType = ob.getType()
	
	if obType == "Lamp":
		alert ("TODO: Light")
			
	elif obType == "Mesh":

		group = Blend2X_GroupForEntity(ob)
	
	 	if group.name == "brushdata":
			alert ("its a brush, not an entity")
			return
	 	if group.name == "meshdata":
			alert ("its a mesh, not an entity")
			return
					
		if group.name == "<invalid>":
			alert ("unknown entity type")
			return
		else:
			log ("found entity in %s " % group.name)
			
		
		Blender.Draw.UIBlock(Blend2X_EntityInspectorDraw)
	
		for i in range(0, len(info)):
			#print("value changed from '%s' to '%s'" % (values[i], info[i].val))
			ob.removeProperty(keys[i])
			s = "%s" % info[i].val
			ob.addProperty(keys[i], s,  'STRING')
			
			
		
		for i in range(0, len(info)):
			if delete[i].val == 1:	
				ob.removeProperty(keys[i])
			
	else:		
		alert ("Unknown objecttype: %s" % obType)	
			
Blend2X_EntityInspector()