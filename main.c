#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include <stdbool.h>

#include <assert.h>

#include <SDL2/SDL.h>

#define assertion(x) assert(x)

#define Byte(x)             (x << 1)
#define Byte64(x) (uint64_t)(x << 1LL)

#define Kilobyte(x)         (x * 1024)
#define Megabyte(x)         (x * 1024 * 1024)
#define Gigabyte(x)         (x * 1024 * 1024 * 1024)
#define Terabyte(x)         (x * 1024 * 1024 * 1024 * 1024)

typedef char* cstring;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8; /* byte */

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

static size_t _globally_tracked_memory_allocation_counter = 0;

struct tracked_memory_allocation_header {
    size_t amount;
};

void* system_heap_memory_allocate(size_t amount) {
    amount += sizeof(struct tracked_memory_allocation_header);
    _globally_tracked_memory_allocation_counter += amount;

    void* memory = malloc(amount);
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
    arena->memory = 0;
    system_heap_memory_deallocate(arena->memory);
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

static SDL_Window* global_game_window  = NULL;
static bool        global_game_running = true;

int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_VIDEO);

    global_game_window = SDL_CreateWindow("RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);

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
    }

    SDL_Quit();
    assertion(system_heap_memory_leak_check());
    return 0;
}
