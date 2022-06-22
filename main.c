/* NOTE no alpha blending yet. */

#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include <stdbool.h>

#include <assert.h>

#include <SDL2/SDL.h>

#define assertion(x) assert(x)

#define Byte(x)             (x << 1)
#define Byte64(x) (uint64_t)(x << 1LL)

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

struct rectangle {
    f32 x;
    f32 y;
    f32 w;
    f32 h;
};
#define rectangle(X,Y,W,H) (struct rectangle){.x=X,.y=Y,.w=W,.h=H}

union color32u8 {
    struct { u8 r, g, b, a; };
    u8  rgba[4];
    u32 rgba_packed;
};
#define color32u8(R,G,B,A) (union color32u8){.r=R,.g=G,.b=B,.a=A}

void software_framebuffer_clear_buffer(struct software_framebuffer* framebuffer, union color32u8 rgba) {
    memory_set32(framebuffer->pixels, framebuffer->width * framebuffer->height * sizeof(u32), rgba.rgba_packed);
}

void software_framebuffer_draw_quad(struct software_framebuffer* framebuffer, struct rectangle destination, union color32u8 rgba) {
    s32 start_x = (s32)destination.x;
    s32 start_y = (s32)destination.y;
    s32 end_x   = (s32)(destination.x + destination.w);
    s32 end_y   = (s32)(destination.y + destination.h);

    if (start_x > end_x) Swap(start_x, end_x, s32);
    if (start_y > end_y) Swap(start_y, end_y, s32);

    start_x = clamp_s32(start_x, 0, framebuffer->width);
    start_y = clamp_s32(start_y, 0, framebuffer->height);
    end_x   = clamp_s32(end_x, 0, framebuffer->width);
    end_y   = clamp_s32(end_y, 0, framebuffer->height);

    u32* framebuffer_pixels_as_32 = framebuffer->pixels;
    for (u32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (u32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
            u32 stride = framebuffer->width;
            framebuffer_pixels_as_32[y_cursor * stride + x_cursor] = rgba.rgba_packed;
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
            software_framebuffer_draw_quad(&global_default_framebuffer, rectangle(100, 100, 100, 100), color32u8(255, 0, 0, 255));
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
