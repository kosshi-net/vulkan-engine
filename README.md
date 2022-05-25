# Vulkan engine

This aims to create a low-level game engine framework, for primarily personal 
use. Apart from a simple demo, no actual game will be implemented here. 

![Screenshot](images/screenshot.png?raw=true)

## Current features
### Demo
- Able to load and draw .obj models
- Freecam controls

### Engine
- Unicode text renderer
- Modular renderer
- Logging system
- Misc debugging tools
- TODO: UI toolkit
- TODO: In-game shell (with maybe LUA integration?)
- TODO: Memory managment / better general purpose allocators
- TODO: Config system
- TODO: Multithreading (fibers?)
- TODO: Audio

### Platform support 
- Linux
- TODO: Windows 

### Vulkan
- Dynamic pipelines for fast resize
- Dynamic descriptors used for rendering the same model with different transforms
- UBOs used instead of push constants
- Using VMA
- Targets Vulkan 1.2 (so no dynamic render passes for now)
- Extensive use of C's designated initialziers makes the code the prettiest Vulkan you'll ever see 😊

### Text rendering 
- Decent unicode support
- Textured quads type 
- Glyph caching system (glyphs rendered with Freetype)
- Shaping (HarfBuzz)
- Supports fallback fonts (using a brute force approach)
- Supports bidirectional text (GNU Fribidi)
- Rudimentary support for line wrapping and basic layouting (centering, aligning, indenting)
	- TODO: Line justification
- Text styling: color, italic and bold 
	- TODO: Underline/strikethrough
	- TODO: Borders
- TODO: Vertical text
- TODO: Cursor

### To fix (besides the obvious)
- Handle minUniformBufferOffsetAlignment properly
	- Temp. patched with \_\_attribute\_\_ aligned 256
- Buffers
	- Stop using depricated VMA usage flags 
	- Stop recreating the staging buffer
- Make sure atlas upload is synced properly 
	- Pipeline barriers?

This is still a special-purpose text renderer, with certain limitations.
Caching system has to be manually tuned for usage patterns, fonts and font 
sizes. At runtime you need to choose from a limited set of font configurations.
It's also only good for 2D use, as SDF isn't suitable for runtime glyph 
generation. Only monochrome glyphs are supported as well.

## Building
### Linux 
```
./prepare.sh
./build.sh 
./run.sh
```

### Windows
TODO! Should be trivial with MinGW64.

## Dependencies
Clone the following repositories or files into lib/
- glfw
- cglm
- VulkanMemoryAllocator
- fast\_obj
- Freetype
- stb/stb\_image.h

Install the following using your package manager:
- HarfBuzz
- GNU Fribidi

