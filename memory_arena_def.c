#ifndef MEMORY_ARENA_DEF_C
#define MEMORY_ARENA_DEF_C

/* double ended stack memory arena, does not grow */
/* TODO: memory allocator interface for seamless usage. */
#define MEMORY_ARENA_DEFAULT_NAME ("(no-name)")

enum memory_arena_flags {
    /* used for smarter temporary memory */
    MEMORY_ARENA_TOUCHED_BOTTOM = BIT(1),
    MEMORY_ARENA_TOUCHED_TOP    = BIT(2),
};

#define Memory_Arena_Base                       \
    cstring              name;                  \
    u8*                  memory;                \
    u64                  capacity;              \
    u64                  used;                  \
    u64                  used_top;              \
    u8                   flags;                 \
    struct memory_arena* next;           

struct memory_arena {
    Memory_Arena_Base;
};

struct temporary_memory {
    Memory_Arena_Base;
    struct memory_arena* parent;
    u64                  parent_top_marker;
    u64                  parent_bottom_marker;
};

struct memory_arena     memory_arena_create_from_heap(cstring name, u64 capacity);
void                    memory_arena_finish(struct memory_arena* arena);
void                    memory_arena_clear_top(struct memory_arena* arena);
void                    memory_arena_clear_bottom(struct memory_arena* arena);
void*                   memory_arena_push_top_unaligned(struct memory_arena* arena, u64 amount);
void*                   memory_arena_push_bottom_unaligned(struct memory_arena* arena, u64 amount);
struct temporary_memory memory_arena_begin_temporary_memory(cstring name, struct memory_arena* arena);
void                    memory_arena_end_temporary_memory(struct temporary_memory* temporary_arena);
#define                 memory_arena_push(arena, amount) memory_arena_push_bottom_unaligned(arena, amount)

#endif
