
// +------------------------------------------------------------------------------+
// |   Some common defines and stuff, used on both, host-side AND on device-side  |
// +------------------------------------------------------------------------------+

// Just a helper to configure scene setup with multiple object instances:
#define NUM_MODELS_PER_DIM			7

// Maximum number of struct object_data entries in the respective buffer:
#define MAX_OBJECTS					100

// Maximum number of subdivision steps that divides patches into smaller parts:
// (8 steps are enough to subdivide 8192x8192 down into 32x32)
#define MAX_PATCH_SUBDIV_STEPS		12

// Maximum number of LOD patches per "ping/pong" buffer:
#define MAX_LOD_PATCHES				1000000

// Maximum number of indirect dispatch calls for the "pixel fill" compute pass (2nd pass):
// NOTE: This  vvv  is also the stride for the start indices of the different rendering variants.
#define MAX_INDIRECT_DISPATCHES		1000000

// The vertex buffers offer  space for 67M vertices each (=> 800MB for positions data):
#define MAX_VERTICES				 67108864
// The index  buffer  offers space for 67M vertices (=> 500MB for index data):
#define MAX_INDICES					134217728

// Whether statistics are enabled at all
#define STATS_ENABLED               0

// Enable/disable local rasterization into shared memory in px fill shaders:
#define SHARED_MEM_LOCAL_RASTER     0

// Enable supersampling, i.e., accumulating of samples and storing their average in the framebuffer
#define SUPERSAMPLING               0

// Enable pass3_px_fill_local_fb.comp instead of pass3_px_fill.comp
#define PX_FILL_LOCAL_FB                1
// Tile size for which we construct a local framebuffer:
#define PX_FILL_LOCAL_FB_TILE_SIZE_X    16
#define PX_FILL_LOCAL_FB_TILE_SIZE_Y    16

// Set factor by how much LOCAL_FB_X is larger than PX_FILL_LOCAL_FB_TILE_SIZE_X:
#define TILE_FACTOR_X                    2
// Set factor by how much LOCAL_FB_Y is larger than PX_FILL_LOCAL_FB_TILE_SIZE_Y:
#define TILE_FACTOR_Y                    2

// Local framebuffer's size (must be exactly PX_FILL_LOCAL_FB_TILE_SIZE_X*TILE_FACTOR_X, or PX_FILL_LOCAL_FB_TILE_SIZE_Y*TILE_FACTOR_Y, respectively):
#define LOCAL_FB_X                      32
#define LOCAL_FB_Y                      32

// Extends struct px_fill_data by one element, fills it in pass2x, and reads it in pass3_px_fill_local_fb:
#define WRITE_MAX_COORDS_IN_PASS2       1

// If enabled, adds a new buffer and a new compute pass which goes over all patches and assigns them to screen tiles.
// Only makes sense in conjunction with PX_FILL_LOCAL_FB enabled.
#define SEPARATE_PATCH_TILE_ASSIGNMENT_PASS    1
#define MAX_PATCHES_PER_TILE                   3000
// 20736 is enough for all the buckets of a 4k resolution
#define TILE_PATCHES_BUFFER_ELEMENTS           MAX_PATCHES_PER_TILE * 20736

// Draws points and lines, visualizing how pass2x evaluates the split decisions
#define DRAW_PATCH_EVAL_DEBUG_VIS 0

// Data size of the big brain dataset to be loaded (note that only even numbers will be properly shaded (sorry!), i.e., e.g.: 10x10, 100x100, or the maximum of 140x140)
#define SH_BRAIN_DATA_SIZE_X      140
#define SH_BRAIN_DATA_SIZE_Y      140
#define SH_BRAIN_ELEMENT_SCALE      0.5
#define SH_BRAIN_ELEMENT_OFFSET    15.0
