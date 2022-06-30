#include "string_def.c"

string string_from_cstring(cstring str) {
    return (string) {
        .length = cstring_length(str),
        .data   = str,
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

u64 cstring_length(const char* cstring) {
    char* cursor = (char*)cstring;

    while (*cursor) {
        cursor++;
    }

    return (cursor - cstring);
}

void cstring_copy(cstring source, cstring destination, u64 destination_length) {
    u64 source_length = cstring_length(source);

    for (u64 index = 0; index < destination_length && index < source_length; ++index) {
        destination[index] = source[index];
    }
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
    return (string) {
        .length = end - start,
        .data   = a.data + start
    };
}
