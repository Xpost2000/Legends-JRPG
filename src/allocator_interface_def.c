#ifndef ALLOCATOR_INTERFACE_DEF_C
#define ALLOCATOR_INTERFACE_DEF_C

/*
  Explicit allocators are good
*/

struct IAllocator;

#define Allocator_Allocate_Function(name)   void* name(struct IAllocator* allocator, u64 amount)
#define Allocator_Deallocate_Function(name) void name(struct IAllocator* allocator, void* ptr)
#define Allocator_Reallocate_Function(name) void* name(struct IAllocator* allocator, void* ptr, u64 amount)

typedef Allocator_Allocate_Function((*allocator_allocate));
typedef Allocator_Deallocate_Function((*allocator_deallocate));
typedef Allocator_Reallocate_Function((*allocator_reallocate));

typedef struct IAllocator {
    void*                userdata;
    allocator_allocate   alloc;
    allocator_deallocate free;
    allocator_reallocate realloc;
} IAllocator;

#endif
