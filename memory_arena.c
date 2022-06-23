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

struct temporary_memory memory_arena_begin_temporary_memory(cstring name, struct memory_arena* arena, u64 amount) {
    struct temporary_memory result = { .name = name };
    
    result.parent = arena;
    result.parent_bottom_marker = arena->used;
    result.parent_top_marker = arena->used_top;
    result.memory = memory_arena_push(arena, amount);

    return result;
}

void memory_arena_end_temporary_memory(struct temporary_memory* temporary_arena) {
    temporary_arena->parent->used     = temporary_arena->parent_bottom_marker;
    temporary_arena->parent->used_top = temporary_arena->parent_top_marker;
    temporary_arena->memory           = 0;
}
