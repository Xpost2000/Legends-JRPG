#define MAX_START_LOCATION_TABLE_SIZE_FOR_SUBSTRING (8192)
#include "string_def.c"

string string_from_cstring(cstring str) {
    return (string) {
        .length = cstring_length(str),
        .data   = str,
    };
}

string string_from_cstring_length_counted(cstring str, u64 length) {
    return (string) {
        .length = length,
        .data   = str
    };
}

bool string_equal(string a, string b) {
    if (a.length == b.length) {
        for (unsigned index = 0; index < a.length; ++index) {
            if (a.data[index] != b.data[index])
                return false;
        }

        return true;
    }

    return false;
}

bool string_equal_case_insensitive(string a, string b) {
    if (a.length == b.length) {
        for (unsigned index = 0; index < a.length; ++index) {
            if (character_lowercase(a.data[index]) != character_lowercase(b.data[index]))
                return false;
        }

        return true;
    }

    return false;
}

u64 cstring_length(const char* cstring) {
    char* cursor = (char*)cstring;

    while (*cursor) {
        cursor++;
    }

    return (cursor - cstring);
}

void cstring_copy(cstring source, cstring destination, u64 destination_length) {
    u64 source_length = cstring_length(source);

    s32 written = 0;
    for (u64 index = 0; index < destination_length && index < source_length; ++index, ++written) {
        destination[index] = source[index];
    }
    destination[written] = 0;
}

bool cstring_equal(cstring a, cstring b) {
    u64 a_length = cstring_length(a);
    u64 b_length = cstring_length(b);

    if (a_length == b_length) {
        for (unsigned index = 0; index < a_length; ++index) {
            if (a[index] != b[index])
                return false;
        }

        return true;
    }
    
    return false;
}

string string_slice(string a, s32 start, s32 end) {
    if (end == -1) {
        end = start + a.length;   
    }

    return (string) {
        .length = end - start,
        .data   = a.data + start
    };
}

string string_concatenate(struct memory_arena* arena, string a, string b) {
    s32 string_length = a.length + b.length;
    _debugprintf("string a: \"%.*s\"\n", a.length, a.data);
    _debugprintf("string b: \"%.*s\"\n", b.length, b.data);
    string new_string = (string) {
        .data   = memory_arena_push(arena, string_length+1),
        .length = string_length
    };

    s32 write_index = 0;

    for (s32 index = 0; index < a.length; ++index) {
        new_string.data[write_index++] = a.data[index];
    }

    for (s32 index = 0; index < b.length; ++index) {
        new_string.data[write_index++] = b.data[index];
    }

    /* cstring friendly... */
    new_string.data[write_index++] = 0;
    return new_string;
}

string string_clone(struct memory_arena* arena, string a) {
    string result = {};
    result.data    = memory_arena_push(arena, a.length);
    result.length = a.length;
    for (unsigned index = 0; index < a.length; ++index) {
        result.data[index] = a.data[index];
    }
    return result;
}

bool is_whitespace(char c) {
    if ((c == ' ') ||
        (c == '\t') ||
        (c == '\r') ||
        (c == '\n')) {
        return true;
    }

    return false;
}

bool is_alphabetic_lowercase(char c) {
    if (c >= 'a' && c <= 'z') {
        return true;
    }

    return false;
}

bool is_alphabetic_uppercase(char c) {
    if (c >= 'A' && c <= 'Z') {
        return true;
    }

    return false;
}

bool is_alphabetic(char c) {
    return is_alphabetic_lowercase(c) || is_alphabetic_uppercase(c);
}

bool is_numeric(char c) {
    if (c == '-') return true;
    if (c >= '0' && c <= '9') {
        return true;
    }
    return false;
}

bool is_numeric_with_decimal(char c) {
    return is_numeric(c) || c == '.' || c == '-';
}

bool is_valid_real_number(string str) {
    bool found_one_decimal = false;

    for (unsigned index = 0; index < str.length; ++index) {
        if (str.data[index] == '.') {
            if (!found_one_decimal) {
                found_one_decimal = true;
            } else {
                return false;
            }
        } else if (!is_numeric(str.data[index])) {
            return false;
        }
    }

    return true;
}

bool is_valid_integer(string str) {
    for (unsigned index = 0; index < str.length; ++index) {
        if (!is_numeric(str.data[index])) {
            return false;
        }
    }

    return true;
}

char character_lowercase(char c) {
    if (is_alphabetic_uppercase(c)) {
        return (c + 32);
    }

    return c;
}

char character_uppercase(char c) {
    if (is_alphabetic_lowercase(c)) {
        return (c - 32);
    }

    return c;
}

s32 string_to_s32(string s) {
    char* temporary = format_temp("%.*s", s.length, s.data);
    return atoi(temporary);
}

f32 string_to_f32(string s) {
    char* temporary = format_temp("%.*s", s.length, s.data);
    return atof(temporary);
}

struct string_array string_split(struct memory_arena* arena, string input_string, char separator) {
    struct string_array result = {};

    result.count = 0;
    result.strings = memory_arena_push(arena, 0);

    s32 character_index = 0;
    while (character_index < input_string.length) {
        s32 start_of_substring = character_index;
        s32 end_of_substring   = start_of_substring;

        while ((character_index < input_string.length) && input_string.data[character_index] != '+') {
            character_index++;
        }
        end_of_substring = character_index;
        character_index += 1;

        string substr = string_slice(input_string, start_of_substring, end_of_substring);

        memory_arena_push(arena, sizeof(string));
        result.strings[result.count++] = substr;
    }

    return result;
}

bool string_is_substring(string a, string substring) {
    if (a.length < substring.length) {
        return false;
    }

    /* while this isn't obviously stupid, it's also not really that great. Just build small prefix search list */
    /* this is only really possible on small strings, since I don't ask for an arena. (Ideally you shouldn't need one) */
    s32 possible_starting_locations[MAX_START_LOCATION_TABLE_SIZE_FOR_SUBSTRING] = {};
    s32 possible_starting_location_count = 0;

    for (unsigned index = 0; index < a.length; ++index) {
        if (a.data[index] == substring.data[0]) {
            if (index+1 < a.length && substring.length > 2) {
                if (a.data[index+1] == substring.data[1]) {
                    possible_starting_locations[possible_starting_location_count++] = index;
                }
            } else {
                possible_starting_locations[possible_starting_location_count++] = index;
            }
        }
    }

    for (unsigned starting_location_index = 0; starting_location_index < possible_starting_location_count; ++starting_location_index) {
        for (unsigned i = possible_starting_locations[starting_location_index]; i < a.length; ++i) {
            bool success = true;

            for (unsigned j = 0; j < substring.length; ++j) {
                if (i+j >= a.length) {
                    success = false;
                    break;
                }
                if (a.data[i+j] != substring.data[j]) {
                    success = false;
                    break;
                }
            }

            if (success) {
                return true;
            }
        }
    }

    return false;
}
