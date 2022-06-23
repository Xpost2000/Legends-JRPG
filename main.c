/* NOTE no alpha blending yet. */

#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include <stdbool.h>

#include <assert.h>

#include <SDL2/SDL.h>

#define assertion(x) assert(x)

#define BIT(x)             (x << 1)
#define BIT64(x) (uint64_t)(x << 1LL)

#define Kilobyte(x)         (uint64_t)(x * 1024LL)
#define Megabyte(x)         (uint64_t)(x * 1024LL * 1024LL)
#define Gigabyte(x)         (uint64_t)(x * 1024LL * 1024LL * 1024LL)
#define Terabyte(x)         (uint64_t)(x * 1024LL * 1024LL * 1024LL * 1024LL)

#define unused(x) (void)(x)

typedef char* cstring;

typedef float  f32;
typedef double f64;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8; /* byte */

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

#define Swap(a, b, type)                        \
    do {                                        \
        type tmp = a;                           \
        a = b;                                  \
        b = tmp;                                \
    } while (0)

s32 clamp_s32(s32 x, s32 min, s32 max) {
    if (x < min) x = min;
    if (x > max) x = max;
    return x;
}
f32 clamp_f32(f32 x, f32 min, f32 max) {
    if (x < min) x = min;
    if (x > max) x = max;
    return x;
}

static size_t _globally_tracked_memory_allocation_counter = 0;

struct tracked_memory_allocation_header {
    size_t amount;
};

/* use 0 for destination_size if I don't care. Ideally I should know both though. */
static inline void memory_copy(void* source, void* destination, size_t amount_from_source, size_t destination_size) {
    if (destination_size != 0) {
        if (amount_from_source > destination_size)
            amount_from_source = destination_size;
    }

    for (u64 index = 0; index < amount_from_source; ++index) {
        ((u8*)destination)[index] = ((u8*)source)[index];
    }
}

static inline void zero_memory(void* memory, size_t amount) {
    for (u64 index = 0; index < amount; ++index) {
        ((u8*)memory)[index] = 0;
    }
}

static inline void memory_set8(void* memory, size_t amount, u8 value) {
    u8* memory_u8 = memory;
    for (u64 index = 0; index < amount; ++index) {
        memory_u8[index] = value;
    }
}
static inline void memory_set16(void* memory, size_t amount, u16 value) {
    u16* memory_u16 = memory;
    for (u64 index = 0; index < amount/2; ++index) {
        memory_u16[index] = value;
    }
}
static inline void memory_set32(void* memory, size_t amount, u32 value) {
    u32* memory_u32 = memory;
    for (u64 index = 0; index < amount/4; ++index) {
        memory_u32[index] = value;
    }
}
static inline void memory_set64(void* memory, size_t amount, u64 value) {
    u64* memory_u64 = memory;
    for (u64 index = 0; index < amount/8; ++index) {
        memory_u64[index] = value;
    }
}

void* system_heap_memory_allocate(size_t amount) {
    amount += sizeof(struct tracked_memory_allocation_header);
    _globally_tracked_memory_allocation_counter += amount;

    void* memory = malloc(amount);

    zero_memory(memory, amount);
    ((struct tracked_memory_allocation_header*)(memory))->amount = amount;

    return memory + sizeof(struct tracked_memory_allocation_header);
}

void system_heap_memory_deallocate(void* pointer) {
    void* header = pointer - sizeof(struct tracked_memory_allocation_header);
    _globally_tracked_memory_allocation_counter -= ((struct tracked_memory_allocation_header*)(header))->amount;
    free(header);
}

bool system_heap_memory_leak_check(void) {
    return (_globally_tracked_memory_allocation_counter == 0);
}

size_t system_heap_currently_allocated_amount(void) {
    return _globally_tracked_memory_allocation_counter;
}

/* so I can track everything, and so I don't accidently misuse them. */
#define malloc(x) system_heap_memory_allocate(x)
#define free(x)   system_heap_memory_deallocate(x)

/* double ended stack memory arena, does not grow */
/* TODO: memory allocator interface for seamless usage. */
struct memory_arena {
    const cstring name;
    u8*           memory;
    u64           capacity;
    u64           used;
    u64           used_top;
};

#define MEMORY_ARENA_DEFAULT_NAME ("(no-name)")
struct memory_arena memory_arena_create_from_heap(cstring name, u64 capacity) {
    if (!name)
        name = MEMORY_ARENA_DEFAULT_NAME;

    u8* memory = system_heap_memory_allocate(capacity);

    return (struct memory_arena) {
        .name     = name,
        .capacity = capacity,
        .memory   = memory,
        .used     = 0,
        .used_top = 0,
    };
}

void memory_arena_finish(struct memory_arena* arena) {
    system_heap_memory_deallocate(arena->memory);
    arena->memory = 0;
}

void* memory_arena_push_bottom_unaligned(struct memory_arena* arena, u64 amount) {
    void* base_pointer = arena->memory + arena->used;

    arena->used += amount;
    assertion((arena->used+arena->used_top) <= arena->capacity);

    return base_pointer;
}

void* memory_arena_push_top_unaligned(struct memory_arena* arena, u64 amount) {
    void* end_of_memory = arena->memory + arena->capacity;
    void* base_pointer  = end_of_memory - arena->used_top;

    arena->used_top += amount;
    assertion((arena->used+arena->used_top) <= arena->capacity);

    return base_pointer;
}

#define memory_arena_push(arena, amount) memory_arena_push_bottom_unaligned(arena, amount)

static SDL_Window*   global_game_window          = NULL;
/* While this is *software* rendered, we still blit using a hardware method since it's still faster than manual texture blits. */
static SDL_Renderer* global_game_sdl_renderer    = NULL;
static SDL_Texture*  global_game_texture_surface = NULL;
static bool          global_game_running         = true;

/* always an RGBA32 bit framebuffer */
struct software_framebuffer {
    u32 width;
    u32 height;
    u8* pixels;
};

/* we would like temporary arenas yes... */
struct software_framebuffer software_framebuffer_create(struct memory_arena* arena, u32 width, u32 height) {
    u8* pixels = memory_arena_push(arena, width * height * sizeof(u32));

    return (struct software_framebuffer) {
        .width  = width,
        .height = height,
        .pixels = pixels,
    };
}

struct rectangle_f32 {
    f32 x;
    f32 y;
    f32 w;
    f32 h;
};
#define rectangle_f32(X,Y,W,H) (struct rectangle_f32){.x=X,.y=Y,.w=W,.h=H}
#define RECTANGLE_F32_NULL rectangle_f32(0,0,0,0)
struct rectangle_f32 rectangle_f32_clamp(struct rectangle_f32 input, struct rectangle_f32 region) {
    input.x = clamp_f32(input.x, region.x, region.x + region.w);
    input.w = clamp_f32(input.w, 0, region.w);
    input.y = clamp_f32(input.y, region.y, region.y + region.h);
    input.h = clamp_f32(input.h, 0, region.h);

    return input;
}

union color32u8 {
    struct { u8 r, g, b, a; };
    u8  rgba[4];
    u32 rgba_packed;
};
/* for percentage and modulations */
union color32f32 {
    struct { f32 r, g, b, a; };
    f32  rgba[4];
};
#define color32u8(R,G,B,A)  (union color32u8){.r  = R,.g=G,.b=B,.a=A}
#define color32f32(R,G,B,A) (union color32f32){.r = R,.g=G,.b=B,.a=A}

void software_framebuffer_clear_buffer(struct software_framebuffer* framebuffer, union color32u8 rgba) {
    memory_set32(framebuffer->pixels, framebuffer->width * framebuffer->height * sizeof(u32), rgba.rgba_packed);
}

void software_framebuffer_draw_quad(struct software_framebuffer* framebuffer, struct rectangle_f32 destination, union color32u8 rgba) {
    struct rectangle_f32 draw_region = rectangle_f32_clamp(
        destination, rectangle_f32(0, 0, framebuffer->width, framebuffer->height)
    );
    s32 start_x = (s32)draw_region.x;
    s32 start_y = (s32)draw_region.y;
    s32 end_x   = (s32)(destination.x + draw_region.w);
    s32 end_y   = (s32)(destination.y + draw_region.h);


    u32* framebuffer_pixels_as_32 = (u32*)framebuffer->pixels;
    for (u32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (u32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
            u32 stride = framebuffer->width;
            framebuffer_pixels_as_32[y_cursor * stride + x_cursor] = rgba.rgba_packed;
        }
    }
}

enum software_framebuffer_draw_image_ex_flags {
    SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_HORIZONTALLY = BIT(0),
    SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_VERTICALLY   = BIT(1),
};

/* (I do recognize the intent of the engine is to eventually support paletted rendering but that comes later.) */
/* the image is expected to be rgba32, and must be converted to as such before rendering. */
struct image_buffer {
    union {
        u8* pixels;
        u32* pixels_u32;
    };
    u32 width;
    u32 height;
};
void software_framebuffer_draw_image_ex(struct software_framebuffer* framebuffer, struct image_buffer image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags) {
    f32 scale_ratio_w = (f32)image.width  / destination.w;
    f32 scale_ratio_h = (f32)image.height / destination.h;

    struct rectangle_f32 draw_region = rectangle_f32_clamp(
        destination, rectangle_f32(0, 0, framebuffer->width, framebuffer->height)
    );

    s32 start_x = (s32)draw_region.x;
    s32 start_y = (s32)draw_region.y;
    s32 end_x   = (s32)(destination.x + draw_region.w);
    s32 end_y   = (s32)(destination.y + draw_region.h);

    u32* framebuffer_pixels_as_32 = (u32*)framebuffer->pixels;
    for (u32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (u32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
            u32 stride       = framebuffer->width;
            u32 image_stride = image.width;

            s32 image_sample_x = floor((x_cursor * scale_ratio_w) + src.x);
            if (flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_HORIZONTALLY)
                image_sample_x = floor((src.x + src.w) - (x_cursor * scale_ratio_w));

            s32 image_sample_y = floor((y_cursor * scale_ratio_h) + src.y);
            if (flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_VERTICALLY)
                image_sample_y = floor((src.y + src.h) - (x_cursor * scale_ratio_h));

            framebuffer_pixels_as_32[y_cursor * stride + x_cursor] = image.pixels_u32[image_sample_y * image_stride + image_sample_x];
        }
    }

}

static struct software_framebuffer global_default_framebuffer;

int main(int argc, char** argv) {
    struct memory_arena game_arena = memory_arena_create_from_heap("Game Memory", Megabyte(256));

    SDL_Init(SDL_INIT_VIDEO);

    const u32 SCREEN_WIDTH  = 640;
    const u32 SCREEN_HEIGHT = 480;

    global_game_window         = SDL_CreateWindow("RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    global_game_sdl_renderer    = SDL_CreateRenderer(global_game_window, -1, SDL_RENDERER_ACCELERATED);
    global_default_framebuffer  = software_framebuffer_create(&game_arena, SCREEN_WIDTH, SCREEN_HEIGHT);
    global_game_texture_surface = SDL_CreateTexture(global_game_sdl_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, global_default_framebuffer.width, global_default_framebuffer.height);

    while (global_game_running) {
        {
            SDL_Event current_event;
            while (SDL_PollEvent(&current_event)) {
                switch (current_event.type) {
                    case SDL_QUIT: {
                        global_game_running = false;
                    } break;
                        
                    case SDL_KEYUP:
                    case SDL_KEYDOWN: {
                        bool is_keydown = (current_event.type == SDL_KEYDOWN);
                    } break;

                    case SDL_MOUSEWHEEL:
                    case SDL_MOUSEMOTION:
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEBUTTONUP: {
                        
                    } break;

                    default: {
                        
                    } break;
                }
            }
        }

        {
            software_framebuffer_clear_buffer(&global_default_framebuffer, color32u8(0, 255, 0, 255));
            software_framebuffer_draw_quad(&global_default_framebuffer, rectangle_f32(-50, 450, 100, 100), color32u8(255, 0, 0, 255));
        }

        {
            void* locked_pixel_region;
            u32   _pitch; unused(_pitch);
            SDL_LockTexture(global_game_texture_surface, 0, &locked_pixel_region, &_pitch);
            memory_copy(global_default_framebuffer.pixels, locked_pixel_region, global_default_framebuffer.width * global_default_framebuffer.height * sizeof(u32), 0);
            SDL_UnlockTexture(global_game_texture_surface);
        }

        SDL_RenderCopy(global_game_sdl_renderer, global_game_texture_surface, 0, 0);
        SDL_RenderPresent(global_game_sdl_renderer);
    }

    memory_arena_finish(&game_arena);
    SDL_Quit();
    assertion(system_heap_memory_leak_check());
    return 0;
}
