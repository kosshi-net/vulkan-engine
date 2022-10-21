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

glslc src/glsl/widget.frag -o bin/widget.frag.spv
glslc src/glsl/widget.vert -o bin/widget.vert.spv

glslc src/glsl/softbody.frag -o bin/softbody.frag.spv
glslc src/glsl/softbody.vert -o bin/softbody.vert.spv
glslc src/glsl/softbody.shadow.vert -o bin/softbody.shadow.vert.spv

glslc src/glsl/wireframe.frag -o bin/wireframe.frag.spv
glslc src/glsl/wireframe.vert -o bin/wireframe.vert.spv

glslc src/glsl/sphere.frag -o bin/sphere.frag.spv
glslc src/glsl/sphere.vert -o bin/sphere.vert.spv
