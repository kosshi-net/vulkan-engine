cmake_minimum_required (VERSION 3.9)

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


	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

### CLI OPTIONS ###
	option(DEBUG "Debug mode" TRUE)

	if (DEBUG)
		set (CMAKE_BUILD_TYPE Debug) 
		set (CMAKE_ENABLE_EXPORTS TRUE)
		add_compile_options(
			-Wall 
			-Wextra 
			-Wpedantic
			-Wno-unused-parameter
			-Wstrict-prototypes
			-Wold-style-definition

			-Wwrite-strings
			-Wcast-align
			-Wshadow
		)
	else()
		set (CMAKE_BUILD_TYPE Release)
	endif()

### LIBRARIES ### 
	set( EXTERNAL lib )

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
	add_compile_options(${OpenMP_CXX_FLAGS})

	# HARFBUZZ
	find_package(harfbuzz REQUIRED)

	# VULKAN
	find_package(Vulkan REQUIRED)

	# FRIBIDI 
	include(FindPkgConfig)
	pkg_search_module (FRIBIDI REQUIRED fribidi)
	include_directories	(${FRIBIDI_INCLUDE_DIRS})	

	include_directories	(include)	
	include_directories	(src)

### BUILD ###

add_subdirectory(src)
