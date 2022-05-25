include_directories	(engine)

add_executable( engine
	main.c
	input.c
	freecam.c
	term.c
	teapot/teapot.c

	engine/engine.c
	engine/res.c
	engine/fileutil.c
	engine/ppm.c
	engine/log/log.c

	engine/event/event.c
	engine/win/win.c

	engine/gfx/gfx.c

	engine/gfx/vk_util.c
	engine/gfx/vk_instance.c
	engine/gfx/vk_device.c
	engine/gfx/vk_swapchain.c

	engine/text/text_engine.c
	engine/text/text_block.c
	engine/text/text_renderer.c
	engine/text/text_geometry.c

	engine/gfx/vma.cpp
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

