#include "allocator_interface_def.c"
Allocator_Allocate_Function(stub_allocator_allocate) { return NULL; }
Allocator_Reallocate_Function(stub_allocator_reallocate) { return ptr; }
Allocator_Deallocate_Function(stub_allocator_deallocate) {}

Allocator_Allocate_Function(heap_allocator_allocate) {
    return system_heap_memory_allocate(amount); 
}

Allocator_Reallocate_Function(heap_allocator_reallocate) {
    return system_heap_memory_reallocate(ptr, amount); 
}

Allocator_Deallocate_Function(heap_allocator_deallocate) {
    system_heap_memory_deallocate(ptr); 
}

IAllocator heap_allocator(void) {
    return (IAllocator) {
        .alloc   = heap_allocator_allocate,
        .free    = heap_allocator_deallocate,
        .realloc = heap_allocator_reallocate,
    };
}

Allocator_Allocate_Function(memory_arena_allocator_allocate) {
    struct memory_arena* arena = (struct memory_arena*) allocator->userdata;
    return memory_arena_push(arena, amount);
}

/* memory arenas aren't expected for realloc purposes sorry! */
IAllocator memory_arena_allocator(struct memory_arena* allocator) {
    return (IAllocator) {
        .userdata = allocator,
        .alloc    = memory_arena_allocator_allocate,
        .free     = stub_allocator_deallocate,
        .realloc  = stub_allocator_reallocate,
    };
}


