cmake_minimum_required (VERSION 3.12)

project ("vulkantest")

set( VERSION_MAJOR 0 )
set( VERSION_MINOR 1 )

### PLATFORM DETECTION ###
	if(APPLE)
		message( FATAL_ERROR "APPLE is not supported" )
	endif()
	if(WIN32)
		if(NOT MINGW)
			message( FATAL_ERROR "Only MinGW is supported on Windows! Use cmake .. -G \"MinGW Makefiles\"")
		endif()
	endif()



### CONFIGURATION ###
	
	# Disable building useless GLFW stuff
	set(GLFW_BUILD_DOCS 	OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS 	OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES	OFF CACHE BOOL "" FORCE)
	
	# Export stuff for linters
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

	# 3rd party library location
	set( EXTERNAL lib )

	# DEBUG
	set( DEBUG_GFX_TEXT_WRITE_PPM 0 )


	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

	find_package(Vulkan REQUIRED FATAL_ERROR)

### CLI OPTIONS ###
	option( DEBUG "Compile in debugging mode" )
	
	if(DEBUG)
		set( CMAKE_BUILD_TYPE Debug ) 
		set( CMAKE_BUILD_TYPE RelWithDebInfo )
		set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}
			-fno-omit-frame-pointer 
			-fsanitize=address 
			-fsanitize=undefined
			-fno-sanitize-recover"
		)
		set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} 
			-fno-omit-frame-pointer 
			-fsanitize=address")
	endif(DEBUG)

### LIBRARIES ### 


	add_subdirectory	(${EXTERNAL}/fast_obj)
	include_directories (${EXTERNAL}/fast_obj)

	# GLFW
	add_subdirectory	(${EXTERNAL}/glfw)
	include_directories	(${EXTERNAL}/glfw/include) # windows wants this

	# Freetype
	add_subdirectory	(${EXTERNAL}/freetype)
	include_directories	(${EXTERNAL}/freetype/include)

	# CGLM (header-only)
	include_directories	(${EXTERNAL}/cglm/include)

	# VulkanMemoryAllocator
	include_directories	(${EXTERNAL}/VulkanMemoryAllocator/include)

	# stb/stb_image.h (header-only)
	# Initalized in gfx
	include_directories	(${EXTERNAL}/stb)

	# OPENMP
	find_package(OpenMP REQUIRED)
	
	# HARFBUZZ
	find_package(harfbuzz REQUIRED)

	# FRIBIDI 
	include(FindPkgConfig)
	pkg_search_module (FRIBIDI REQUIRED fribidi)
	include_directories	(${FRIBIDI_INCLUDE_DIRS})	

	include_directories	(include)	
	include_directories	(src)


### BUILD ###

add_subdirectory(src)
