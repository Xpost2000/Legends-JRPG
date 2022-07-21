#include "memory_arena_def.c"

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

#if 0
struct memory_arena memory_arena_create_from_heap_growable(cstring name, u64 capacity) {
    struct memory_arena arena = memory_arena_create_from_heap(name, capacity);
    arena.flags |= MEMORY_ARENA_GROWABLE;
    return arena;
}
#endif

void memory_arena_finish(struct memory_arena* arena) {
    system_heap_memory_deallocate(arena->memory);
    arena->memory = 0;
}

void memory_arena_clear_top(struct memory_arena* arena) {
    arena->used_top = 0;
}

void memory_arena_clear_bottom(struct memory_arena* arena) {
    arena->used = 0;
}

void _memory_arena_usage_bounds_check(struct memory_arena* arena) {
    /* arenas with capacity 0 are temporary arenas. */
    if (arena->capacity != 0) {
        if ((arena->used+arena->used_top) >= arena->capacity) {
#if 0
            if (arena->flags & MEMORY_ARENA_GROWABLE) {
                /* arena */
            } else {
                assertion((arena->used+arena->used_top) <= arena->capacity);
            }
#else
            assertion((arena->used+arena->used_top) <= arena->capacity);
#endif
        }
    } else {
        struct temporary_memory* temporary_arena = (struct temporary_memory*) arena;
        assertion((temporary_arena->used+temporary_arena->used_top+temporary_arena->parent->used +temporary_arena->parent->used_top) <= temporary_arena->parent->capacity);
    }
}

void* memory_arena_push_bottom_unaligned(struct memory_arena* arena, u64 amount) {
    void* base_pointer = arena->memory + arena->used;

    arena->used += amount;
    arena->flags |= MEMORY_ARENA_TOUCHED_BOTTOM;
    _memory_arena_usage_bounds_check(arena);

    return base_pointer;
}

void* memory_arena_push_top_unaligned(struct memory_arena* arena, u64 amount) {
    void* end_of_memory = arena->memory + arena->capacity;
    arena->used_top += amount;
    void* base_pointer  = end_of_memory - arena->used_top;

    arena->flags |= MEMORY_ARENA_TOUCHED_TOP;
    _memory_arena_usage_bounds_check(arena);

    return base_pointer;
}

struct memory_arena memory_arena_push_sub_arena(struct memory_arena* arena, u64 amount) {
    char* name = MEMORY_ARENA_DEFAULT_NAME;

    struct memory_arena sub_arena = {};
    /* sub_arena.parent = arena; */
    sub_arena.memory   = memory_arena_push(arena, amount);
    sub_arena.capacity = amount;
    sub_arena.name     = name;
    return sub_arena;
}

struct temporary_memory memory_arena_begin_temporary_memory(cstring name, struct memory_arena* arena) {
    struct temporary_memory result = { .name = name };
    
    result.parent               = arena;
    result.parent_bottom_marker = arena->used;
    result.parent_top_marker    = arena->used_top;
    result.memory               = memory_arena_push(arena, 0);

    return result;
}

void memory_arena_end_temporary_memory(struct temporary_memory* temporary_arena) {
    if (temporary_arena->flags & MEMORY_ARENA_TOUCHED_BOTTOM)
        temporary_arena->parent->used     = temporary_arena->parent_bottom_marker;
    if (temporary_arena->flags & MEMORY_ARENA_TOUCHED_TOP)
        temporary_arena->parent->used_top = temporary_arena->parent_top_marker;

    temporary_arena->memory = 0;
}

string memory_arena_push_string_bottom(struct memory_arena* arena, string to_copy) {
    string result = {};
    result.length = to_copy.length;
    result.data   = memory_arena_push_bottom_unaligned(arena, to_copy.length);
    cstring_copy(to_copy.data, result.data, result.length);
    return result;
}

string memory_arena_push_string_top(struct memory_arena* arena, string to_copy) {
    string result = {};
    result.length = to_copy.length;
    result.data   = memory_arena_push_top_unaligned(arena, to_copy.length);
    cstring_copy(to_copy.data, result.data, result.length);
    return result;
}
