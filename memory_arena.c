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
