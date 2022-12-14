#ifndef MEMORY_ARENA_DEF_C
#define MEMORY_ARENA_DEF_C

/*
  Hmmm... a better consideration for the API would
  actually be to make top allocation a flag, instead of a separate
  function call so that way you can inject allocation choices, instead
  of "hard-coding" them...
  
  TODO: but this requires a fair amount of changes so we'll wait on that... (this weekend?)
  
  Now fortunately, we're not likely to run out of allocated memory since I don't have long test sessions,
  and nor do I expect my player testers to play for more than an hour maybe, so this is okay. Obviously this
  needs to be fixed for releasing in any way shape or form though.
*/

/* double ended stack memory arena, does not grow */
/* TODO: memory allocator interface for seamless usage. */
#define MEMORY_ARENA_DEFAULT_NAME ("(no-name)")

enum memory_arena_flags {
    /* used for smarter temporary memory */
    MEMORY_ARENA_TOUCHED_BOTTOM = BIT(1),
    MEMORY_ARENA_TOUCHED_TOP    = BIT(2),
};

enum memory_arena_allocation_region {
    MEMORY_ARENA_ALLOCATION_REGION_BOTTOM,
    MEMORY_ARENA_ALLOCATION_REGION_TOP,
};

#define Memory_Arena_Base                       \
    cstring              name;                  \
    u8*                  memory;                \
    u64                  capacity;              \
    u64                  used;                  \
    u64                  used_top;              \
    u8                   flags;                 \
    s32                  peak_top;              \
    s32                  peak_bottom;           \
    struct memory_arena* next;                  \
    u8                   alloc_region;          \

struct memory_arena {
    Memory_Arena_Base;
};

struct temporary_memory {
    Memory_Arena_Base;
    struct memory_arena* parent;
    u64                  parent_top_marker;
    u64                  parent_bottom_marker;
};

void                    memory_arena_set_allocation_region_top(struct memory_arena* arena);
void                    memory_arena_set_allocation_region_bottom(struct memory_arena* arena);
s64                     memory_arena_get_cursor(struct memory_arena* arena);
void                    memory_arena_set_cursor(struct memory_arena* arena, s64 where);

struct memory_arena     memory_arena_create_from_heap(cstring name, u64 capacity);
void                    memory_arena_finish(struct memory_arena* arena);
void                    memory_arena_clear_top(struct memory_arena* arena);
void                    memory_arena_clear_bottom(struct memory_arena* arena);
void                    memory_arena_clear(struct memory_arena* arena);
void*                   memory_arena_push_unaligned(struct memory_arena* arena, u64 amount);
struct memory_arena     memory_arena_push_sub_arena(struct memory_arena* arena, u64 amount);
struct temporary_memory memory_arena_begin_temporary_memory(cstring name, struct memory_arena* arena);
void                    memory_arena_end_temporary_memory(struct temporary_memory* temporary_arena);
string                  memory_arena_push_string(struct memory_arena* arena, string to_copy);
void                    _memory_arena_peak_usages(struct memory_arena* arena);
#define                 memory_arena_push(arena, amount) memory_arena_push_unaligned(arena, amount)

#endif
