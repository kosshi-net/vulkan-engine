cp texture.png build/bin/
cd build;
make -j12 || exit 

cp -rv ../src/glsl/ src/

glslc src/glsl/tri.frag -o bin/frag.spv || exit
glslc src/glsl/tri.vert -o bin/vert.spv || exit 

#cd bin
#ls
#./engine $@;
