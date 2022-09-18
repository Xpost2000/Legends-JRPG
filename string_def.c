#ifndef STRING_DEF_C
#define STRING_DEF_C

typedef struct {
    s32   length;
    char* data;
} string;
#define string_literal(x) (string){ .length = sizeof(x)-1, .data = x }
#define string_null       (string){}

bool is_whitespace(char c);
bool is_alphabetic(char c);
bool is_alphabetic_lowercase(char c);
bool is_alphabetic_uppercase(char c);
bool is_numeric(char c);
bool is_numeric_with_decimal(char c);
bool is_valid_real_number(string str);
bool is_valid_integer(string str);

char character_lowercase(char c);
char character_uppercase(char c);

s32 string_to_s32(string s);
f32 string_to_f32(string s);

string string_from_cstring(cstring str);
string string_from_cstring_length_counted(cstring str, u64 length);
string string_slice(string a, s32 start, s32 end);
bool   string_equal(string a, string b);
bool   string_equal_case_insensitive(string a, string b);

bool cstring_equal(cstring a, cstring b);
u64  cstring_length(const char* cstring);
void cstring_copy(cstring source, cstring destination, u64 destination_length);

/* obviously this implies you build the string in linear fashion... Not string builder style I hope */
/* but I can offer a string builder... */
/* add snprintf with an arena... */
#include "memory_arena_def.c"

string string_concatenate(struct memory_arena* arena, string a, string b);
string string_clone(struct memory_arena* arena, string a);

#endif
