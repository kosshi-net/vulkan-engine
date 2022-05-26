# Text API overview

## Handles vs Pointers
Functions generally return integer handles. The handles are named the same as 
the underlying structures, but without the `struct` keyword. 

Example: `TextEngine` is a handle to an interal `struct TextEngine *`. These 
are two distinct types. The user should not ever have to use the pointer types.

Do note handles are not type-checked compile-time, as they're all the same 
underlying type.

## API Objects
The objects are designed to separate handling and rendering logic.

### TextEngine 
- Manages core resources
- Manages all fonts and glyph cache
- There should not be a need to create multiple text engines, but it's possible

### TextBlock 
- Handles all the alignment, layout, styles, colors, etc text processing
- Stores processed blocks of text in a flexible intermediate format
- Converting utf8 text to a TextBlock *can* be expensive, intended to be used as a cache
- Changing layout is very cheap
- Is oblivious to the actual position on-screen, only understands lines and their maximum widths
- Depends on TextEngine

### TextRenderer
- Instance per TextEngine 
- Stores shared pipelines etc vulkan objects
- Renders TextGeometries
- Depends on TextEngine, TextGeometry

### TextGeometry
- Manages a fixed block of memory on device (allocate for worst case)
- Converts TextBlocks to actual geometry and uploads to device
- Intended use is to draw many textblocks at once, with a single draw call
- Able to combine several TextBlocks with limited layout functionality
- Decides the final origin of TextBlocks on-screen
- Can be set up to be static or dynamic (upload once / every frame)
- TODO: Allows for dynamic scissor and positioning of the combined geometry (without reupload)
- Depends on TextRenderer, TextBlock


