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
#define                 memory_arena_push_top(arena, amount) memory_arena_push_top_unaligned(arena, amount)
void*                   memory_arena_push_bottom_unaligned(struct memory_arena* arena, u64 amount);
struct memory_arena     memory_arena_push_sub_arena(struct memory_arena* arena, u64 amount);
#define                 memory_arena_push_bottom(arena, amount) memory_arena_push_bottom_unaligned(arena, amount)
struct temporary_memory memory_arena_begin_temporary_memory(cstring name, struct memory_arena* arena);
void                    memory_arena_end_temporary_memory(struct temporary_memory* temporary_arena);
string                  memory_arena_push_string_bottom(struct memory_arena* arena, string to_copy);
string                  memory_arena_push_string_top(struct memory_arena* arena, string to_copy);
#define                 memory_arena_push_string(arena, string) memory_arena_push_string_bottom(arena, string)
#define                 memory_arena_push(arena, amount) memory_arena_push_bottom_unaligned(arena, amount)

#endif
