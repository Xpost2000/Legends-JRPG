#ifndef COMMON_C
#define COMMON_C

#ifndef __EMSCRIPTEN__
#include <x86intrin.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include <stdbool.h>

#include <assert.h>
#include <time.h>

#define assertion(x) assert(x)
#define unimplemented(x) assertion(false && x);
#define bad_case         default: { unimplemented ("no case"); } break
#define invalid_cases()  bad_case
#define array_count(x) (sizeof(x)/sizeof(x[0]))
#define local    static
#define internal static
#define safe_assignment(x) if(x) *x

#define BIT(x)             (1 << x)
#define BIT64(x) (uint64_t)(1LL << x)
#define Toggle_Bit(a, x) (a ^= x)
#define Set_Bit(a, x)    (a |= x)
#define Get_Bit(a, x)    (a & x)

#define Kilobyte(x)                 (uint64_t)(x * 1024LL)
#define Megabyte(x)                 (uint64_t)(x * 1024LL * 1024LL)
#define Gigabyte(x)                 (uint64_t)(x * 1024LL * 1024LL * 1024LL)
#define Terabyte(x)                 (uint64_t)(x * 1024LL * 1024LL * 1024LL * 1024LL)

#ifndef RELEASE
#define _debugprintf(fmt, args...)   fprintf(stderr, "[%s:%d:%s()]: " fmt "\n", __FILE__, __LINE__, __func__, ##args)
#define _debugprintf1(fmt, args...)  fprintf(stderr,  fmt, ##args)
/* #define _debugprintf(fmt, args...)   */
/* #define _debugprintf1(fmt, args...) */
#else
#define _debugprintf(fmt, args...)  
#define _debugprintf1(fmt, args...)
#endif

#define Array_For_Each(NAME, TYPE, ARR, COUNT) for (TYPE * NAME = ARR; NAME != (ARR + COUNT); NAME += 1)

#define unused(x) (void)(x)

#define min(a, b) ((a) < (b)) ? (a) : (b)
#define max(a, b) ((a) > (b)) ? (a) : (b)

typedef char* cstring;

typedef float  f32;
typedef double f64;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8; /* byte */

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

static inline f32 normalized_sinf(f32 t) {
    return (sinf(t)+1)/2.0;
}
static inline f32 normalized_cosf(f32 t) {
    return (cosf(t)+1)/2.0;
}

static inline f32 sq_f32(f32 x) {
    return x*x;
}

#define Minimum(a, b) ((a) < (b)) ? (a) : (b)
#define Maximum(a, b) ((a) > (b)) ? (a) : (b)

static inline f32 sign_f32(f32 x) {
    if      (x > 0)   return 1;
    else if (x == 0)  return 0;
    else if (x < 0)   return -1;

    return 1;
}

static inline u32 hash_bytes_fnv1a(u8* bytes, size_t length) {
    u32 offset_basis = 2166136261;
    u32 prime        = 16777619;

    u32 hash = offset_basis;

    for (unsigned index = 0; index < length; ++index) {
        hash ^= bytes[index];
        hash *= prime;
    }

    return hash;
}

#define Swap(a, b, type)                        \
    do {                                        \
        type tmp = a;                           \
        a = b;                                  \
        b = tmp;                                \
    } while (0)

#define Reverse_Array_Inplace(ARRAY, LENGTH, TYPE)                      \
    do {                                                                \
        for (s32 _index = 0; _index < LENGTH/2; ++_index) {             \
            Swap(ARRAY[_index], ARRAY[(LENGTH-1)-_index], TYPE);        \
        }                                                               \
    } while (0)

/* hopefully these are branchless, I can optimize on sunday? */
s32 clamp_s32(s32 x, s32 min, s32 max) {
#if 0
    if (x < min) x = min;
    if (x > max) x = max;
    return x;
#else
    s32 i = (x < min) ? min : x;
    return (i > max)  ? max : i;
#endif
}
f32 clamp_f32(f32 x, f32 min, f32 max) {
#if 0
    if (x < min) x = min;
    if (x > max) x = max;
    return x;
#else
    f32 i = (x < min) ? min : x;
    return (i > max)  ? max : i;
#endif
}

s32 lerp_s32(s32 a, s32 b, f32 normalized_t) {
    return a * (1 - normalized_t) + (b * normalized_t);
}
f32 lerp_f32(f32 a, f32 b, f32 normalized_t) {
    return a * (1 - normalized_t) + (b * normalized_t);
}

f32 cubic_ease_in_f32(f32 a, f32 b, f32 normalized_t) {
    return (b - a) * (normalized_t * normalized_t * normalized_t) + a;
}

f32 cubic_ease_out_f32(f32 a, f32 b, f32 normalized_t) {
    normalized_t -= 1;
    return (b - a) * (normalized_t * normalized_t * normalized_t) + a;
}

f32 quadratic_ease_in_f32(f32 a, f32 b, f32 normalized_t) {
    return (b - a) * (normalized_t * normalized_t) + a;
}

f32 quadratic_ease_out_f32(f32 a, f32 b, f32 normalized_t) {
    return -(b - a) * ((normalized_t * normalized_t) - 2.0) + a;
}

f32 fractional_f32(f32 x) {
    return x - floor(x);
}
f32 whole_f32(f32 x) {
    return x - fractional_f32(x);
}
f32 step_f32(f32 x, f32 edge) {
    return (x < edge) ? 0 : 1;
}
s32 step_s32(s32 x, s32 edge) {
    return (x < edge) ? 0 : 1;
}

f32 pick_f32(f32 a, f32 b, s32 v) {
    return (v == 0) ? a : b;
}

s32 pick_s32(s32 a, s32 b, s32 v) {
    return (v == 0) ? a : b;
}

typedef struct s32_range {
    s32 min; s32 max;
    s32 value;
} s32_range;
typedef struct f32_range {
    f32 min; f32 max;
    f32 value;
} f32_range;
s32_range s32_range_add(s32_range range, s32 value) {
    s32 new_value = range.value;
    range.value = clamp_s32(new_value+value, range.min, range.max);
    return range;
}
s32_range s32_range_sub(s32_range range, s32 value) {
    s32 new_value = range.value;
    range.value = clamp_s32(new_value-value, range.min, range.max);
    return range;
}
s32_range s32_range_mul(s32_range range, s32 value) {
    s32 new_value = range.value;
    range.value = clamp_s32(new_value*value, range.min, range.max);
    return range;
}
s32_range s32_range_div(s32_range range, s32 value) {
    s32 new_value = range.value;
    range.value = clamp_s32(new_value/value, range.min, range.max);
    return range;
}
f32_range f32_range_add(f32_range range, f32 value) {
    f32 new_value = range.value;
    range.value = clamp_f32(new_value+value, range.min, range.max);
    return range;
}
f32_range f32_range_sub(f32_range range, f32 value) {
    f32 new_value = range.value;
    range.value = clamp_f32(new_value-value, range.min, range.max);
    return range;
}
f32_range f32_range_mul(f32_range range, f32 value) {
    f32 new_value = range.value;
    range.value = clamp_f32(new_value*value, range.min, range.max);
    return range;
}
f32_range f32_range_div(f32_range range, f32 value) {
    f32 new_value = range.value;
    range.value = clamp_f32(new_value/value, range.min, range.max);
    return range;
}

static size_t _globally_tracked_memory_allocation_counter = 0;
static size_t _globally_tracked_memory_allocation_peak    = 0;

struct tracked_memory_allocation_header {
    size_t amount;
};

/* use 0 for destination_size if I don't care. Ideally I should know both though. */

/* Minorly faster implementations without stosb. */
static inline void memory_copy64(void* source, void* destination, size_t amount) {
    for (u64 index = 0; index < amount >> 3; ++index) {
        ((u64*)destination)[index] = ((u64*)source)[index];
    }
}

static inline void memory_copy32(void* source, void* destination, size_t amount) {
    for (u64 index = 0; index < amount >> 2; ++index) {
        ((u32*)destination)[index] = ((u32*)source)[index];
    }
}

static inline void memory_copy16(void* source, void* destination, size_t amount) {
    for (u64 index = 0; index < amount >> 1; ++index) {
        ((u16*)destination)[index] = ((u16*)source)[index];
    }
}

static inline void memory_copy8(void* source, void* destination, size_t amount) {
    for (u64 index = 0; index < amount; ++index) {
        ((u8*)destination)[index] = ((u8*)source)[index];
    }
}

static inline void memory_copy(void* source, void* destination, size_t amount) {
    if (amount & ~(63)) {
        memory_copy64(source, destination, amount);
        /* _debugprintf("memory copy 64bit"); */
    } else if (amount & ~(31)) {
        memory_copy32(source, destination, amount);
        /* _debugprintf("memory copy 32bit"); */
    } else if (amount & ~(15)) {
        memory_copy16(source, destination, amount);
        /* _debugprintf("memory copy 16bit"); */
    } else {
        memory_copy8(source, destination, amount);
        /* _debugprintf("memory copy default"); */
    }
}

static inline void zero_memory(void* memory, size_t amount) {
    for (u64 index = 0; index < amount; ++index) {
        ((u8*)memory)[index] = 0;
    }
}
#define zero_array(x) zero_memory(x, array_count(x))

static inline void memory_set8(void* memory, size_t amount, u8 value) {
    u8* memory_u8 = memory;
    for (u64 index = 0; index < amount; ++index) {
        memory_u8[index] = value;
    }
}
static inline void memory_set16(void* memory, size_t amount, u16 value) {
    u16* memory_u16 = memory;
    for (u64 index = 0; index < amount/2; ++index) {
        memory_u16[index] = value;
    }
}
static inline void memory_set32(void* memory, size_t amount, u32 value) {
    u32* memory_u32 = memory;
    for (u64 index = 0; index < amount/4; ++index) {
        memory_u32[index] = value;
    }
}
static inline void memory_set64(void* memory, size_t amount, u64 value) {
    u64* memory_u64 = memory;
    for (u64 index = 0; index < amount/8; ++index) {
        memory_u64[index] = value;
    }
}

void* system_heap_memory_allocate(size_t amount, s32 line_number, const char* filename, const char* function) {
    /* if (line_number && filename && function) _debugprintf("heap allocation! : [%s:%d:%s()]", filename, line_number, function); */
    amount += sizeof(struct tracked_memory_allocation_header);
    _globally_tracked_memory_allocation_counter += amount;

    if (_globally_tracked_memory_allocation_counter > _globally_tracked_memory_allocation_peak)
        _globally_tracked_memory_allocation_peak = _globally_tracked_memory_allocation_counter;

    void* memory = malloc(amount);

    zero_memory(memory, amount);
    ((struct tracked_memory_allocation_header*)(memory))->amount = amount;

    return memory + sizeof(struct tracked_memory_allocation_header);
}


void* system_heap_memory_reallocate(void* original_pointer, size_t new_amount) {
    if (original_pointer) {
        void* header = original_pointer - sizeof(struct tracked_memory_allocation_header);
        _globally_tracked_memory_allocation_counter -= ((struct tracked_memory_allocation_header*)(header))->amount;

        header = realloc(header, new_amount);
        zero_memory(header, new_amount);
        ((struct tracked_memory_allocation_header*)(header))->amount = new_amount;

        _globally_tracked_memory_allocation_counter += ((struct tracked_memory_allocation_header*)(header))->amount;

        return header + sizeof(struct tracked_memory_allocation_header);
    }

    return system_heap_memory_allocate(new_amount, 0, 0, 0);
}


void system_heap_memory_deallocate(void* pointer, s32 line_number, const char* filename, const char* function) {
    /* if (line_number && filename && function) _debugprintf("heap deallocation! : [%s:%d:%s()]", filename, line_number, function); */
    if (pointer) {
        void* header = pointer - sizeof(struct tracked_memory_allocation_header);
        _globally_tracked_memory_allocation_counter -= ((struct tracked_memory_allocation_header*)(header))->amount;
        free(header);
    }
}

#define system_heap_memory_allocate(AMOUNT) system_heap_memory_allocate((AMOUNT), __LINE__, __FILE__, __func__)
#define system_heap_memory_deallocate(PTR)  system_heap_memory_deallocate((PTR),  __LINE__, __FILE__, __func__)

bool system_heap_memory_leak_check(void) {
    return (_globally_tracked_memory_allocation_counter == 0);
}

size_t system_heap_currently_allocated_amount(void) {
    return _globally_tracked_memory_allocation_counter;
}

size_t system_heap_peak_allocated_amount(void) {
    return _globally_tracked_memory_allocation_peak;
}

struct rectangle_f32 {
    f32 x; f32 y; f32 w; f32 h;
};
struct rectangle_s32 {
    s32 x; s32 y; s32 w; s32 h;
};
struct rectangle_s16 {
    s16 x; s16 y; s16 w; s16 h;
};

#define rectangle_f32(X,Y,W,H) (struct rectangle_f32){.x=X,.y=Y,.w=W,.h=H}
#define rectangle_s32(X,Y,W,H) (struct rectangle_s32){.x=X,.y=Y,.w=W,.h=H}
#define rectangle_s16(X,Y,W,H) (struct rectangle_s32){.x=X,.y=Y,.w=W,.h=H}

#define RECTANGLE_F32_NULL rectangle_f32(0,0,0,0)
#define RECTANGLE_S32_NULL rectangle_s32(0,0,0,0)
#define RECTANGLE_S16_NULL rectangle_s16(0,0,0,0)

struct rectangle_f32 rectangle_f32_centered(struct rectangle_f32 center_region, f32 width, f32 height) {
    return rectangle_f32(
        center_region.x + (center_region.w/2) - (width/2),
        center_region.y + (center_region.h/2) - (height/2),
        width, height
    );
}

struct rectangle_f32 rectangle_f32_scale(struct rectangle_f32 a, f32 k) {
    a.x *= k;
    a.y *= k;
    a.w *= k;
    a.h *= k;
    return a;
}
bool rectangle_f32_intersect(struct rectangle_f32 a, struct rectangle_f32 b) {
    if (a.x < b.x + b.w && a.x + a.w > b.x &&
        a.y < b.y + b.h && a.y + a.h > b.y) {
        return true;
    }

    return false;
}

#define FRAMETIME_SAMPLE_MAX (32)
struct {
    u16 index;
    u16 length;
    f32 data[FRAMETIME_SAMPLE_MAX];
} global_frametime_sample_array = {};

void add_frametime_sample(f32 data) {
    global_frametime_sample_array.data[global_frametime_sample_array.index++] = data;
    if (global_frametime_sample_array.length <  FRAMETIME_SAMPLE_MAX) global_frametime_sample_array.length++;
    if (global_frametime_sample_array.index  >= FRAMETIME_SAMPLE_MAX) global_frametime_sample_array.index = 0;
}

f32 get_average_frametime(void) {
    f32 sum = 0.0;

    for (unsigned index = 0; index < global_frametime_sample_array.length; ++index) {
        sum += global_frametime_sample_array.data[index];
    }

    return sum / global_frametime_sample_array.length;
}

#define TEMPORARY_STORAGE_BUFFER_SIZE (4096)
#define TEMPORARY_STORAGE_BUFFER_COUNT (4)
static const char* cstr_yesno[]     = {"no", "yes"};
static const char* cstr_truefalse[] = {"false", "true"};

char* format_temp(const char* fmt, ...) {
    local int current_buffer = 0;
    local char temporary_text_buffer[TEMPORARY_STORAGE_BUFFER_COUNT][TEMPORARY_STORAGE_BUFFER_SIZE] = {};

    char* target_buffer = temporary_text_buffer[current_buffer++];
    zero_memory(target_buffer, TEMPORARY_STORAGE_BUFFER_SIZE);
    {
        va_list args;
        va_start(args, fmt);
        int written = vsnprintf(target_buffer, TEMPORARY_STORAGE_BUFFER_SIZE-1, fmt, args);
        va_end(args);
    }

    if (current_buffer >= TEMPORARY_STORAGE_BUFFER_COUNT) {
        current_buffer = 0;
    }

    return target_buffer;
}


#define NO_FLAGS (0)

#include "prng.c"
#include "v2.c"
#include "string.c"

static const string yesno[] = {string_literal("no"), string_literal("yes")};
static const string truefalse[] = {string_literal("false"), string_literal("true")};

#include "memory_arena.c"
#include "allocators.c"

struct file_buffer {
    IAllocator allocator;
    u8* buffer;
    u64 length;
};

string file_buffer_slice(struct file_buffer* buffer, u64 start, u64 end) {
    char* cstring_buffer = (char*)buffer->buffer;
    return string_from_cstring_length_counted(cstring_buffer + start, (end - start));
}

string file_buffer_as_string(struct file_buffer* buffer) {
    return file_buffer_slice(buffer, 0, buffer->length);
}

bool   file_exists(string path) {
    FILE* f = fopen(path.data, "r");

    if (f) {
        fclose(f);
        return true;
    }

    return false;
}

size_t file_length(string path) {
    size_t result = 0;
    FILE*  file   = fopen(path.data, "rb+");

    if (file) {
        fseek(file, 0, SEEK_END);
        result = ftell(file);
        fclose(file);
    }

    return result;
}

void read_entire_file_into_buffer(string path, u8* buffer, size_t buffer_length) {
    FILE* file = fopen(path.data, "rb+");
    fread(buffer, 1, buffer_length, file);
    fclose(file);
}

struct file_buffer read_entire_file(IAllocator allocator, string path) {
    _debugprintf("file buffer create!");
    size_t file_size   = file_length(path);
    u8*    file_buffer = allocator.alloc(&allocator, file_size+1);
    read_entire_file_into_buffer(path, file_buffer, file_size);
    file_buffer[file_size] = 0;
    return (struct file_buffer) {
        .allocator = allocator,
        .buffer = file_buffer,
        .length = file_size,
    };
}

void file_buffer_free(struct file_buffer* file) {
    if (file->buffer)
        file->allocator.free(&file->allocator, file->buffer);
}

/* if the order is confusing, most significant bit is first. */
static inline u16 packu16(u8 b0, u8 b1) {
    return ((u32)(b0) << 4) | ((u32)(b1));
}

static inline u32 packu32(u8 b0, u8 b1, u8 b2, u8 b3) {
    return ((u32)(b0) << 24) |
        ((u32)(b1) << 16)    |
        ((u32)(b2) << 8)     |
        ((u32)b3);
}

/* NOTE win32 code for now */
struct directory_file {
    char    name[260];
    size_t  filesize;
    bool    is_directory;
    /* could add more info like timing stuff */
};
struct directory_listing {
    s32                    count;
    struct directory_file* files;
};

#define WIN32_LEAN_AND_MEAN
#ifndef __EMSCRIPTEN__
#include <windows.h>
#endif

struct directory_listing directory_listing_list_all_files_in(struct memory_arena* arena, string location) {
    struct directory_listing result = {};

#ifndef __EMSCRIPTEN__
    WIN32_FIND_DATA find_data = {};
    HANDLE handle = FindFirstFile(string_concatenate(arena, location, string_literal("/*")).data, &find_data);

    if (handle == INVALID_HANDLE_VALUE) {
        _debugprintf("no files found in directory (\"%s\")", location.data);
        return result;
    }

    result.files = memory_arena_push(arena, sizeof(*result.files));
    _debugprintf("allocating files");

    do {
        struct directory_file* current_file = &result.files[result.count++];

        current_file->is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        cstring_copy(find_data.cFileName, current_file->name, array_count(current_file->name));
        current_file->filesize = (find_data.nFileSizeHigh * (MAXDWORD+1)) + find_data.nFileSizeLow;

        _debugprintf("read file \"%s\"", find_data.cFileName);
        memory_arena_push(arena, sizeof(*result.files));
    } while (FindNextFile(handle, &find_data));
#endif

    return result;
}

static u64 read_timestamp_counter(void) {
    return __rdtsc();
}

#endif
