#/bin/bash

MODE=$1
MAPFILE=$2

ARCH="x86"

if [ $# -ne 2 ]
then

echo "Incorrect arguments"
echo "use: blender2_xreal_buid [mode] [path_to_map_file]"
echo "		Modes:"
echo "			2 - simple build"
echo "			1- comlete build"
echo "			3 - use xmap instead of xmap2"

exit

fi


cd ../

if [ $1 == "2" ]
then 

./xmap2.$ARCH -v -meta $MAPFILE
./xmap2.$ARCH -vis $MAPFILE
./xmap2.$ARCH -v -light -fast -super 2 -filter -bounce 8  $MAPFILE

elif [ $1 == "1" ]
then

./xmap2.$ARCH -v -meta $MAPFILE
./xmap2.$ARCH -vis -saveprt -fast $MAPFILE
./xmap2.$ARCH -v -light -fast -super 2 -filter $MAPFILE


elif [ $1 == "3" ]
then

./xmap.$ARCH -map2bsp -v $MAPFILE
./xmap.$ARCH -vis $MAPFILE

else

echo "Incorrect build mode $1"

fi

exit





