#!BPY


# mapfile code for Blender 2 Xreal Suite,
# by Adrian 'otty' Fuhrmann

import math


from blender2xreal_tools import *

def Blend2X_Header(file):

	file.write("Version 3 \n")



def Blend2X_WorldMesh(file, numEntitys, meshfile):
	file.write("// entity %i \n" % (numEntitys + 1))
	file.write("{\n")
	file.write("\"classname\" \"func_static\"\n" )
	file.write("\"name\" \"func_static_1\"\n" )
	file.write("\"origin\" \"")
	file.write("0 ")
	file.write("0 ")
	file.write("0" )
	file.write("\"\n")		
	file.write("\"model\" \"%s\"\n" % meshfile)
	file.write("}\n")

def Blend2X_Lights(file, n, light, lData):		
		file.write("// entity %i \n" % ( n + 1))
		file.write("{\n")
		
		file.write("\"classname\" \"light\"\n")
		file.write("\"name\" \"%s\"\n" % light.getName())
		
		file.write("\"origin\" \"")
		file.write("%i " % light.LocX)
		file.write("%i " % light.LocY)
		file.write("%i" % light.LocZ)
		file.write("\"\n")
		
		file.write("\"light_radius\" \"")
		file.write("%i " % lData.dist)
		file.write("%i " % lData.dist)
		file.write("%i" % lData.dist)
		file.write("\"\n")

		file.write("\"_color\" \"")
		file.write("%f " % lData.R)
		file.write("%f " % lData.G)
		file.write("%f" % lData.B)
		file.write("\"\n")
		
		flags = lData.getMode()
		
		if flags & lData.Modes["NoSpecular"]:
			file.write ("\"nospecular\" \"1\"\n")
		else:
			file.write ("\"nospecular\" \"0\"\n")
       	
		if flags & lData.Modes["NoDiffuse"]:
			file.write ("\"nodiffuse\" \"1\"\n")
		else:
			file.write ("\"nodiffuse\" \"0\"\n")
   
		if flags & lData.Modes["Shadows"]:
			file.write ("\"noshadows\" \"1\"\n")
		else:
			file.write ("\"noshadows\" \"0\"\n")
   
		
		file.write("}\n")
		
			


def Blend2X_SkyBox( file, size, hullShader):
	# skybox :
	
	brushDefTex = [ 0.03125 , 0 , 0 , 0 , 0.03125 , 0]
	brushDef = []
	
	#formated like this for readability....
	bDef0 = [[ 0, 0 , 1,-1 * size],[ 0, 1 , 0,-1 * size],[ 1, 0 , 0,-1 * size],[ 0,-1 , 0,-1 * size],[-1, 0 , 0,-1 * size],[ 0, 0 ,-1, 1 * size - 8]]
	bDef1 = [[ 0, 0 , 1,-1 * size],[ 0, 1 , 0,-1 * size],[ 1, 0 , 0,-1 * size],[ 0, 0 ,-1,-1 * size],[-1, 0 , 0,-1 * size],[ 0, -1, 0, 1 * size - 8]]
	bDef2 = [[ 0, 0 , 1,-1 * size],[ 0, 1 , 0,-1 * size],[ 1, 0 , 0,-1 * size],[ 0, 0 ,-1,-1 * size],[ 0,-1 , 0,-1 * size],[-1, 0 , 0, 1 * size - 8]]
	bDef3 = [[ 0, 1 , 0,-1 * size],[ 1, 0 , 0,-1 * size],[ 0, 0 , -1,-1 * size],[ 0,-1 , 0,-1 * size],[-1, 0 , 0,-1 * size],[ 0, 0 ,1, 1 * size - 8]]
	bDef4 = [[ 0, 0 , 1,-1 * size],[ 1, 0 , 0,-1 * size],[ 0, 0 , -1,-1 * size],[ 0,-1 , 0,-1 * size],[-1, 0 , 0,-1 * size],[ 0, 1 ,0, 1 * size - 8]]
	bDef5 = [[ 0, 0 , 1,-1 * size],[ 0, 1 , 0,-1 * size],[ 0, 0 , -1,-1 * size],[ 0,-1 , 0,-1 * size],[-1, 0 , 0,-1 * size],[ 1, 0 , 0, 1 * size - 8]]

		   
	brushDef.append(bDef0)		   
	brushDef.append(bDef1)		   
	brushDef.append(bDef2)		   
	brushDef.append(bDef3)		   
	brushDef.append(bDef4)		   
	brushDef.append(bDef5)	
	
	for i in range( 0, 6):
		file.write("// brush %i\n" % i)
		file.write("{\n")
		file.write("brushDef3\n")
		file.write("{\n")		

		for n in range (0, 6): 
			file.write("( ")
			for k in range ( 0, 4):				
				file.write("%i " % brushDef[i][n][k])
			
			file.write(") ")					
			file.write("( ( ")
			for k in range (0, 3):
				file.write("%f " % brushDefTex[k])			
			file.write(" ) ( ")
			for k in range (0, 3):
				file.write("%f " % brushDefTex[k+3])			
			file.write(" ) ) ")
			file.write( "\"%s\"" % hullShader)
			file.write("\n")					
		file.write("}\n")
		file.write("}\n")		
			
					
	

				