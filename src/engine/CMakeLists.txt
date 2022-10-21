add_executable( engine
	engine.c
	res/res.c
	util/file.c
	util/ppm.c
	log/log.c
	event/event.c
	win/win.c
	handle/handle.c
	gfx/gfx.c
	gfx/vk_util.c
	gfx/vk_instance.c
	gfx/vk_device.c
	gfx/vk_swapchain.c
	text/text_engine.c
	text/text_block.c
	text/text_renderer.c
	text/text_geometry.c
	widget/widget_renderer.c

	sphere/icosphere.c
	sphere/icosahedron.c
	sphere/sphere_renderer.c

	#mem/mem.c
	mem/sysmem.c

	gfx/vma.cpp
)


target_link_libraries( engine
	game
	"-fopenmp"
	OpenMP::OpenMP_CXX
	${OpenMP_CXX_FLAGS}
	${FRIBIDI_LIBRARIES}
	vulkan
	glfw
	freetype
	fast_obj_lib
	harfbuzz

)

