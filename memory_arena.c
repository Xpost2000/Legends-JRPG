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

s64  memory_arena_get_cursor(struct memory_arena* arena) {
    if (arena->alloc_region == MEMORY_ARENA_ALLOCATION_REGION_BOTTOM) {
        return arena->used;
    } else {
        return arena->used_top;
    }

    return 0;
}

void memory_arena_set_cursor(struct memory_arena* arena, s64 where) {
    if (arena->alloc_region == MEMORY_ARENA_ALLOCATION_REGION_BOTTOM) {
        arena->used = where;
    } else {
        arena->used_top = where;
    }
}

void memory_arena_finish(struct memory_arena* arena) {
#if 0
    if (arena->parent) {
        arena->memory = 0;
        return;
    }
#endif

    system_heap_memory_deallocate(arena->memory);
    arena->memory = 0;
}

void memory_arena_clear_top(struct memory_arena* arena) {
    zero_memory(arena->memory+arena->capacity - arena->used_top, arena->used_top);
    arena->used_top = 0;
}

void memory_arena_clear_bottom(struct memory_arena* arena) {
    zero_memory(arena->memory, arena->used);
    arena->used = 0;
}

void memory_arena_clear(struct memory_arena* arena) {
    memory_arena_clear_bottom(arena);
    memory_arena_clear_top(arena);
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
#if 0 /* This triggers a warning on GCC, that I'm pretty sure is actually bogus */
        struct temporary_memory* temporary_arena = (struct temporary_memory*) arena;
        assertion((temporary_arena->used+temporary_arena->used_top+temporary_arena->parent->used +temporary_arena->parent->used_top) <= temporary_arena->parent->capacity);
#endif
    }
}

void memory_arena_set_allocation_region_top(struct memory_arena* arena) {
    arena->alloc_region = MEMORY_ARENA_ALLOCATION_REGION_TOP;
}

void memory_arena_set_allocation_region_bottom(struct memory_arena* arena) {
    arena->alloc_region = MEMORY_ARENA_ALLOCATION_REGION_BOTTOM;
}

void* memory_arena_push_unaligned(struct memory_arena* arena, u64 amount) {
    if (arena->alloc_region == MEMORY_ARENA_ALLOCATION_REGION_BOTTOM) {
        void* base_pointer = arena->memory + arena->used;

        arena->used += amount;

        if (arena->used > arena->peak_bottom) {
            arena->peak_bottom = arena->used;
        }

        arena->flags |= MEMORY_ARENA_TOUCHED_BOTTOM;
        _memory_arena_usage_bounds_check(arena);

        return base_pointer;
    } else {
        void* end_of_memory = arena->memory + arena->capacity;
        arena->used_top += amount;
        void* base_pointer  = end_of_memory - arena->used_top;

        if (arena->used_top > arena->peak_top) {
            arena->peak_top = arena->used_top;
        }

        arena->flags |= MEMORY_ARENA_TOUCHED_TOP;
        _memory_arena_usage_bounds_check(arena);

        return base_pointer;
    }

    return 0;
}

struct memory_arena memory_arena_push_sub_arena(struct memory_arena* arena, u64 amount) {
    char* name = MEMORY_ARENA_DEFAULT_NAME;

    struct memory_arena sub_arena = {};
    /* sub_arena.parent   = arena; */
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

string memory_arena_push_string(struct memory_arena* arena, string to_copy) {
    string result = {};
    result.length = to_copy.length;
    result.data   = memory_arena_push(arena, to_copy.length+1);
    cstring_copy(to_copy.data, result.data, result.length);
    result.data[to_copy.length] = 0;
    return result;
}


void _memory_arena_peak_usages(struct memory_arena* arena) {
    _debugprintf("\n\t(%s)[%s cap!] has peak allocations at:\n\t%s (bottom)\n\t%s (top)\n\t%s (total)",
                 arena->name,
                 biggest_valid_memory_string(arena->capacity),
                 memory_strings(arena->peak_bottom),
                 memory_strings(arena->peak_top),
                 biggest_valid_memory_string(arena->peak_bottom + arena->peak_top));
}
