#!/bin/sh
rm build -rfv;
mkdir build;
cd build;
cmake ..;
cp compile_commands.json ..
cd ..;

