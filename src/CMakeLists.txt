add_executable( engine
	main.c
	array.c
	res.c
	fileutil.c


	gfx/gfx.c
	gfx/vk_util.c
	gfx/vk_instance.c
	gfx/vk_device.c
	gfx/vk_swapchain.c

	gfx/vma.cpp
)

target_compile_options( engine PUBLIC
	#"-O2"
	#"-mavx2"
	#"-ffast-math"
	#"-march=native"
	#"-ftree-vectorize"
	"-Wformat-truncation=0"
	"-Wall"
	"-static"
	"-ggdb"

	"${OpenMP_CXX_FLAGS}"
)

target_link_libraries( engine
	"${OpenMP_CXX_FLAGS}"	
	OpenMP::OpenMP_CXX
	vulkan
	glfw
	freetype
)

