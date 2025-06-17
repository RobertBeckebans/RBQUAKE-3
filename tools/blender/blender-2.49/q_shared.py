import sys, struct, string, math
from types import *

import os
from os import path

GAMEDIR = 'D:/Games/XreaL_testing/base/'
#GAMEDIR = '/opt/XreaL/base/'
MAX_QPATH = 64

def asciiz(s):
	n = 0
	while(ord(s[n]) != 0):
		n = n + 1
	return s[0:n]

# strips the slashes from the back of a string
def StripPath(path):
	for c in range(len(path), 0, -1):
		if path[c-1] == "/" or path[c-1] == "\\":
			path = path[c:]
			break
	return path
	
# strips the model from path
def StripModel(path):
	for c in range(len(path), 0, -1):
		if path[c-1] == "/" or path[c-1] == "\\":
			path = path[:c]
			break
	return path

# strips file type extension
def StripExtension(name):
	n = 0
	best = len(name)
	while(n != -1):
		n = name.find('.',n+1)
		if(n != -1):
			best = n
	name = name[0:best]
	return name
	
# strips gamedir
def StripGamePath(name):
	gamepath = GAMEDIR.replace( '\\', '/' )
	namepath = name.replace( '\\', '/' )
	if namepath[0:len(gamepath)] == gamepath:
		namepath= namepath[len(gamepath):len(namepath)]
	return namepath
