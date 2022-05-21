#!/bin/sh

set -e

cp -v resources/vulkan_logo.png  build/bin/texture.png
cp -v resources/teapot.obj       build/bin/teapot.obj

cp -rv resources/fonts           build/bin/

cd build;
make -j12 || exit 

cp -rv ../src/glsl/ src/

glslc src/glsl/tri.frag -o bin/frag.spv 
glslc src/glsl/tri.vert -o bin/vert.spv

glslc src/glsl/text.frag -o bin/text.frag.spv
glslc src/glsl/text.vert -o bin/text.vert.spv
