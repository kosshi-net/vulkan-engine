# Vulkan engine

This project aims to implement a small vulkan-based game engine for primarily 
personal use. 

## Current features
### General
- Able to load and draw .obj models
	- Dynamic descriptors used for rendering the same model with different transforms
	- UBOs used instead of push constants
- Extensive use of C's designated initialziers makes the code the prettiest Vulkan you'll ever see 😊
- Targets Vulkan 1.2 (so no dynamic render passes for now)

### Text rendering 
- Good enough unicode support
- Textured quads type 
- Glyph caching system (glyphs rendered with Freetype)
- Shaping (HarfBuzz)
- Supports fallback fonts (using a brute force approach)
- Supports bidirectional text (GNU Fribidi)
- Rudimentary support for line wrapping and basic layouting (centering, aligning)
- TODO: Color
- TODO: Italic and bold
- TODO: Vertical text
- TODO: Cursor

This is still a special-purpose text renderer, with certain limitations.
Caching system has to be manually tuned for usage patterns, fonts and font 
sizes. At runtime you need to choose from a limited set of font configurations.
It's also only suitable for 2D use, as SDF isn't suitable for runtime glyph
generation.

![Text rendering](images/screenshot.png?raw=true)

https://user-images.githubusercontent.com/19539479/154944471-8bf0f198-ee2f-4e73-84c0-0dd79d597658.mp4

## To fix (besides the obvious)
- Handle minUniformBufferOffsetAlignment properly
	- Temp. patched with \_\_attribute\_\_ aligned 256
- Buffers
	- Stop using depricated VMA usage flags 
	- Stop recreating the staging buffer
- Make sure atlas upload is synced properly 
	- Pipeline barriers?

## Building
### Linux 
```
./prepare.sh
./build.sh 
./run.sh
```

### Windows
Not tested but should be trivial with MinGW64.

## Dependencies
Clone the following repositories or files into lib/
- glfw
- cglm
- VulkanMemoryAllocator
- fast_obj
- Freetype
- stb/stb_image.h

Install the following using your package manager:
- HarfBuzz
- GNU Fribidi

# License
MIT
