#!/bin/sh
rm build -rfv;
mkdir build;
cd build;
cmake $@ ..;
cp compile_commands.json ..
cd ..;

if [ -z "$@" ] 
then
	echo
	echo "Building DEBUG by default."
	echo "Pass -DDEBUG=false to build release."
	echo
fi
