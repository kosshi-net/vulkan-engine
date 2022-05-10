include_directories	(engine)

add_executable( engine
	main.c

	engine/engine.c
	engine/res.c
	engine/fileutil.c
	engine/ppm.c

	engine/event/event.c
	engine/win/win.c

	engine/gfx/gfx.c
	engine/gfx/vk_util.c
	engine/gfx/vk_instance.c
	engine/gfx/vk_device.c
	engine/gfx/vk_swapchain.c
	engine/gfx/text/vk_text.c
	engine/gfx/text/text.c
	engine/gfx/vma.cpp
)

target_compile_options( engine PUBLIC
	"-O3"
	#"-mavx2"
	#"-ffast-math"
	#"-march=native"
	#"-ftree-vectorize"
	#"-O3"
	"-Wformat-truncation=0"
	"-Wall"
	"-static"
	"-ggdb"

	"${OpenMP_CXX_FLAGS}"
)

target_link_libraries( engine
	"${OpenMP_CXX_FLAGS}"	
	OpenMP::OpenMP_CXX
	${FRIBIDI_LIBRARIES}
	vulkan
	glfw
	freetype
	fast_obj_lib
	harfbuzz

)

