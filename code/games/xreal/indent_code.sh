#!/bin/sh

#for i in `find -iname "*.h"`; do dos2unix $i; done
#for i in `find -iname "*.c"`; do dos2unix $i; done

for i in `find -iname "*.h"`; do indent $i; done
for i in `find -iname "*.c"`; do indent $i; done
