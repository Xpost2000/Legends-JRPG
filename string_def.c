#ifndef STRING_DEF_C
#define STRING_DEF_C

typedef struct {
#if 0
    IAllocator allocator; /* for growable strings, if I ever need that */
#endif

    u64   length;
    u64   capacity; /* capacity = 0, means literal */
    char* data;
} string;
#define string_literal(x) (string){ .length = sizeof(x)-1, .capacity = 0, .data = x }

string string_from_cstring(cstring str);
string string_from_cstring_length_counted(cstring str, u64 length);
string string_slice(string a, s32 start, s32 end);
bool   string_equal(string a, string b);

bool cstring_equal(cstring a, cstring b);
u64  cstring_length(const char* cstring);
void cstring_copy(cstring source, cstring destination, u64 destination_length);

/* obviously this implies you build the string in linear fashion... Not string builder style I hope */
/* but I can offer a string builder... */
/* add snprintf with an arena... */
#include "memory_arena_def.c"

string string_concatenate(struct memory_arena* arena, string a, string b);

#endif
