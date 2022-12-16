#ifndef COMMON_C
#define COMMON_C

#ifndef __EMSCRIPTEN__
#include <x86intrin.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include <stdbool.h>

#include <assert.h>
#include <time.h>

#include <math.h>

/*
  METAPROGRAMMING MARKERS
 */

/* mark these on structs? To allow for autogeneration of serialization code */
/* Ground rules for how this should work:

   SERIALIZE is equivalent to
   SERIALIZE_VERSIONS(ALL_VERSIONS) or SERIALIZE_VERSIONS(1 to CURRENT_LEVEL_AREA_VERSION)
   SERIALIZE_SAVE is equivalent to
   SERIALIZE_SAVE_VERSIONS(ALL_VERSIONS) or SERIALIZE_VERSIONS(2 to CURRENT_LEVEL_AREA_VERSION)

   SERIALIZE on struct def only considers it for serialization. Does not automatic serialize. Fields
   are never automatically serialized unless otherwise specified.

   These two serialization procedures are based on the save and level area versions respectively,
   they are slightly different.

   UNPACK_INTO(), for the few structures that require an unpack step since I for some reason decided
   there are some differences in the serialization process, I'm just going to parse a weird DSL
   to get these to unpack sanely.

   This is context dependent.

   This automates the unpack function generation, and if there are fields that don't exist or are
   the wrong type it will give me a compilation error so I can always notice it.

   It's not perfect, but it's better than nothing.

   the editable stuff, is obvious, need to do that later...

   NOTE: might not do this, but I might forget to remove the markup...
 */
#define SERIALIZE
#define SERIALIZE_SAVE
#define SERIALIZE_VERSIONS(...)
#define SERIALIZE_SAVE_VERSIONS(...)
#define UNPACK_INTO(...)
#define PACKED_AS(...)
#define VARIABLE_ARRAY(entity_savepoint_count)
#define EDITABLE
#define EDITABLE_BITFLAG(flag_enumeration)
#define EDITABLE_LIMITS(a, b)

/*
  END OF METAPROGRAMMING MARKERS
 */

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

#define Bytes(x)                    (uint64_t)(x)
#define Byte(x)                    (uint64_t)(x)
#define Kilobyte(x)                 (uint64_t)(x * 1024LL)
#define Megabyte(x)                 (uint64_t)(x * 1024LL * 1024LL)
#define Gigabyte(x)                 (uint64_t)(x * 1024LL * 1024LL * 1024LL)
#define Terabyte(x)                 (uint64_t)(x * 1024LL * 1024LL * 1024LL * 1024LL)

#ifndef RELEASE
#define _debugprintf(fmt, args...)   fprintf(stderr, "[%s:%d:%s()]: " fmt "\n", __FILE__, __LINE__, __func__, ##args)
#define _debugprintfhead()   fprintf(stderr, "[%s:%d:%s()]: " ,__FILE__, __LINE__, __func__)
#define _debugprintf1(fmt, args...)  fprintf(stderr,  fmt, ##args)
#define DEBUG_CALL(fn) fn; _debugprintf("calling %s in [%s:%d:%s()]", #fn, __FILE__, __LINE__, __func__)
#else
#define _debugprintf(fmt, args...)  
#define _debugprintfhead(fmt, args...)
#define _debugprintf1(fmt, args...)
#define DEBUG_CALL(_) _;
#endif

#define Array_For_Each(NAME, TYPE, ARR, COUNT) for (TYPE * NAME = ARR; NAME != (ARR + COUNT); NAME += 1)

#define unused(x) (void)(x)

#define Fixed_Array_Push(array, counter) &array[counter++]
#define Fixed_Array_Remove_And_Swap(array, where, counter) array[where] = array[--counter]

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

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

local char* memory_strings(size_t in_bytes);
local char* biggest_valid_memory_string(size_t in_bytes);
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
#define XorSwap(a, b)                           \
    do {                                        \
        a ^= b;                                 \
        b ^= a;                                 \
        a ^= b;                                 \
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

#include "interpolations.c"

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
    u64 index = 0, real_index = 0;
    for (index = 0; index < amount >> 3; ++index, real_index+=8) {
        ((u64*)destination)[index] = ((u64*)source)[index];
    }

    for (; real_index < amount; ++real_index) {
        ((u8*)destination)[real_index] = ((u8*)source)[real_index];
    }
}

static inline void memory_copy32(void* source, void* destination, size_t amount) {
    u64 index = 0, real_index = 0;
    for (index = 0; index < amount >> 2; ++index, real_index+=4) {
        ((u32*)destination)[index] = ((u32*)source)[index];
    }

    for (; real_index < amount; ++real_index) {
        ((u8*)destination)[real_index] = ((u8*)source)[real_index];
    }
}

static inline void memory_copy16(void* source, void* destination, size_t amount) {
    u64 index = 0, real_index = 0;
    for (index = 0; index < amount >> 1; real_index+=2,++index) {
        ((u16*)destination)[index] = ((u16*)source)[index];
    }

    for (; real_index < amount; ++real_index) {
        ((u8*)destination)[real_index] = ((u8*)source)[real_index];
    }
}

static inline void memory_copy8(void* source, void* destination, size_t amount) {
    for (u64 index = 0; index < amount; ++index) {
        ((u8*)destination)[index] = ((u8*)source)[index];
    }
}

static inline void memory_copy(void* source, void* destination, size_t amount) {
#if 1
    if (amount & ~(63)) {
        memory_copy64(source, destination, amount);
#if 0
        _debugprintf("memory copy 64bit");
#endif
    } else if (amount & ~(31)) {
        memory_copy32(source, destination, amount);
#if 0
        _debugprintf("memory copy 32bit");
#endif
    } else if (amount & ~(15)) {
        memory_copy16(source, destination, amount);
#if 0
        _debugprintf("memory copy 16bit");
#endif
    } else {
        memory_copy8(source, destination, amount);
#if 0
        _debugprintf("memory copy default");
#endif
    }
#else
    memcpy(destination, source, amount);
#endif
}

static inline void zero_memory(void* memory, size_t amount) {
    for (u64 index = 0; index < amount; ++index) {
        ((u8*)memory)[index] = 0;
    }
}
#define zero_array(x) zero_memory(x, array_count(x))
#define zero_struct(x) zero_memory(&x, sizeof(x));

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

#define TEMPORARY_STORAGE_BUFFER_SIZE (2048)
#define TEMPORARY_STORAGE_BUFFER_COUNT (8)
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

#include "bigfilemaker/bigfile_def.c"

struct file_buffer {
    IAllocator allocator;
    u8* buffer;
    u64 length;
    u8  does_not_own_memory;
};

string file_buffer_slice(struct file_buffer* buffer, u64 start, u64 end) {
    char* cstring_buffer = (char*)buffer->buffer;
    return string_from_cstring_length_counted(cstring_buffer + start, (end - start));
}

string file_buffer_as_string(struct file_buffer* buffer) {
    return file_buffer_slice(buffer, 0, buffer->length);
}

bool OS_file_exists(string path) {
    FILE* f = fopen(path.data, "r");

    if (f) {
        fclose(f);
        return true;
    }

    return false;
}

size_t OS_file_length(string path) {
    size_t result = 0;
    FILE*  file   = fopen(path.data, "rb+");

    if (file) {
        fseek(file, 0, SEEK_END);
        result = ftell(file);
        fclose(file);
    }

    return result;
}

void OS_read_entire_file_into_buffer(string path, u8* buffer, size_t buffer_length) {
    FILE* file = fopen(path.data, "rb+");
    fread(buffer, 1, buffer_length, file);
    fclose(file);
}

struct file_buffer OS_read_entire_file(IAllocator allocator, string path) {
    _debugprintf("file buffer create!");
    size_t file_size   = OS_file_length(path);
    u8*    file_buffer = allocator.alloc(&allocator, file_size+1);
    OS_read_entire_file_into_buffer(path, file_buffer, file_size);
    file_buffer[file_size] = 0;
    return (struct file_buffer) {
        .allocator = allocator,
        .buffer = file_buffer,
        .length = file_size,
    };
}

#ifdef EXPERIMENTAL_VFS
#define MAX_MOUNTABLE_BIGFILES (32)
local bigfile_blob_t global_mounted_bigfiles[MAX_MOUNTABLE_BIGFILES] = {};
local s32            global_mounted_bigfile_count                    = 0;

/* NOTE: we cannot unmount any archives */

local bool VFS__find_last_mounted_file(string path, struct bigfile_file_data* out_result) {
    if (global_mounted_bigfile_count <= 0) {
        out_result->data   = NULL;
        out_result->length = 0;
        return false;
    }

    for (s32 mounted_bigfile_index = global_mounted_bigfile_count-1; mounted_bigfile_index >= 0; --mounted_bigfile_index) {
        out_result->data   = NULL;
        out_result->length = 0;
        *out_result        = bigfile_get_raw_file_data_by_name(global_mounted_bigfiles[mounted_bigfile_index], path);

        if (out_result->data) {
            _debugprintf("BUFFER CONTENTS");
            _debugprintf("%.*s", (s32)out_result->length, out_result->data);
            _debugprintf("END BUFFER CONTENTS");
            return true;
        }
    }

    return false;
}

size_t VFS_file_length(string path) {
    struct bigfile_file_data found_mounted_file = {};
    if (VFS__find_last_mounted_file(path, &found_mounted_file)) {
        return found_mounted_file.length;
    }

    return 0;
}

bool VFS_file_exists(string path) {
    struct bigfile_file_data found_mounted_file = {};
    return VFS__find_last_mounted_file(path, &found_mounted_file);
}

void VFS_read_entire_file_into_buffer(string path, u8* buffer, size_t buffer_length) {
    {
        struct bigfile_file_data found_mounted_file = {};
        if (VFS__find_last_mounted_file(path, &found_mounted_file)) {
            memory_copy(found_mounted_file.data, buffer, buffer_length);
        }
    }
}

struct file_buffer VFS_read_entire_file(IAllocator allocator, string path) {
    struct bigfile_file_data found_mounted_file = {};
    struct file_buffer as_filebuffer = {};

    if (VFS__find_last_mounted_file(path, &found_mounted_file)) {
        as_filebuffer.buffer              = found_mounted_file.data;
        as_filebuffer.length              = found_mounted_file.length;
        as_filebuffer.does_not_own_memory = true;
    }

    return as_filebuffer;
}
#endif

/* NOTE: these wrappers should have a way to specify preference */
bool file_exists(string path) {
#ifdef EXPERIMENTAL_VFS
    if (VFS_configuration_prefer_local) {
        return OS_file_exists(path) || VFS_file_exists(path);
    } else {
        return VFS_file_exists(path) || OS_file_exists(path);
    }
#else
    return OS_file_exists(path);
#endif
}

size_t file_length(string path) {
#ifdef EXPERIMENTAL_VFS
    if (VFS_configuration_prefer_local) {
        if (OS_file_exists(path)) {
            return OS_file_length(path);
        }

        return VFS_file_length(path);
    } else {
        if (VFS_file_exists(path)) {
            return VFS_file_length(path);
        }

        return OS_file_length(path);
    }
#else
    return OS_file_length(path);
#endif
}

void read_entire_file_into_buffer(string path, u8* buffer, size_t buffer_length) {
#ifdef EXPERIMENTAL_VFS
    if (VFS_configuration_prefer_local) {
        if (OS_file_exists(path)) {
            OS_read_entire_file_into_buffer(path, buffer, buffer_length);
            return;
        }
        VFS_read_entire_file_into_buffer(path, buffer, buffer_length);
    } else {
        if (VFS_file_exists(path)) {
            VFS_read_entire_file_into_buffer(path, buffer, buffer_length);
            return;
        }
        OS_read_entire_file_into_buffer(path, buffer, buffer_length);
    }
#else
    OS_read_entire_file_into_buffer(path, buffer, buffer_length);
#endif
}

struct file_buffer read_entire_file(IAllocator allocator, string path) {
#ifdef EXPERIMENTAL_VFS
    _debugprintf("read_entire_file start");
    /* this will be compiled out anyways, I need a slightly more elegant way perhaps, but this is good enough */
    if (VFS_configuration_prefer_local) {
        if (OS_file_exists(path)) {
            _debugprintf("Read \"%.*s\" from real file system", path.length, path.data);
            return OS_read_entire_file(allocator, path);
        }

        _debugprintf("Read \"%.*s\" from VFS", path.length, path.data);
        return VFS_read_entire_file(allocator, path);
    } else {
        if (VFS_file_exists(path)) {
            _debugprintf("Read \"%.*s\" from VFS", path.length, path.data);
            return VFS_read_entire_file(allocator, path);
        }

        _debugprintf("Read \"%.*s\" from real file system", path.length, path.data);
        return OS_read_entire_file(allocator, path);
    }
#else
    return OS_read_entire_file(allocator, path);
#endif
}

local void mount_bigfile_archive(struct memory_arena* arena, string path) {
#ifdef EXPERIMENTAL_VFS
    if (file_exists(path)) {
        s32 current_archive                      = global_mounted_bigfile_count;
        global_mounted_bigfiles[current_archive] = bigfile_load_blob(memory_arena_allocator(arena), path);

        if (global_mounted_bigfiles[current_archive]) {
            global_mounted_bigfile_count++;
            _debugprintf("\"%.*s\" bigfile archive was mounted!", path.length, path.data);
            _debugprintf("\tVERSION: %d", bigfile_get_version(global_mounted_bigfiles[current_archive]));
            _debugprintf("\tRECORDCOUNT: %d", bigfile_get_record_count(global_mounted_bigfiles[current_archive]));
        } else {
            _debugprintf("Failure to mount \"%.*s\"", path.length, path.data);
        }

        assertion(global_mounted_bigfile_count < MAX_MOUNTABLE_BIGFILES && "Too many mounted bigfile archives!");
    }
#endif
}

/* there is no VFS variation because bigfiles are read only */
void write_entire_file(string path, u8* buffer, size_t buffer_length) {
    FILE* file = fopen(path.data, "wb+");
    fwrite(buffer, 1, buffer_length, file);
    fclose(file);
}

void write_string_into_entire_file(string path, string contents) {
    write_entire_file(path, (u8*)contents.data, contents.length);
}

void file_buffer_free(struct file_buffer* file) {
    if (file->does_not_own_memory) {
        return;
    } else if (file->buffer) {
        file->allocator.free(&file->allocator, file->buffer);
    }
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
    char                   basename[260];
    s32                    count;
    struct directory_file* files;
};

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef __EMSCRIPTEN__
#include <windows.h>
#endif

bool path_exists(string location) {
#ifndef __EMSCRIPTEN__
    char tmp_copy[260] = {};
    for (s32 i = 0; i < location.length; ++i) {
        tmp_copy[i] = location.data[i];
    }
    if (location.data[location.length-1] == '/' || location.data[location.length-1] == '\\') {
        tmp_copy[location.length-1] = 0;
    }

    WIN32_FIND_DATA find_data = {};
    HANDLE handle = FindFirstFile(tmp_copy, &find_data);


    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool is_path_directory(string location) {
#ifndef __EMSCRIPTEN__
    char tmp_copy[260] = {};
    for (s32 i = 0; i < location.length; ++i) {
        tmp_copy[i] = location.data[i];
    }
    if (location.data[location.length-1] == '/' || location.data[location.length-1] == '\\') {
        tmp_copy[location.length-1] = 0;
    }

    WIN32_FIND_DATA find_data = {};
    HANDLE handle = FindFirstFile(tmp_copy, &find_data);

    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return true;
    }

    return false;
#else
    return false;
#endif
}

struct directory_listing directory_listing_list_all_files_in(struct memory_arena* arena, string location) {
    struct directory_listing result = {};
    cstring_copy(location.data, result.basename, 260);

#ifndef __EMSCRIPTEN__
    if (!is_path_directory(location)) {
        return result;
    }

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
#endif

s32 length_of_longest_string(string* strings, s32 count) {
    s32 length_of_longest = 0;
    for (s32 string_index = 0; string_index < count; ++string_index) {
        if (strings[string_index].length > length_of_longest) {
            length_of_longest = strings[string_index].length;
        }
    }
    return length_of_longest;
}

s32 length_of_longest_cstring(char** strings, s32 count) {
    s32 length_of_longest = 0;
    for (s32 string_index = 0; string_index < count; ++string_index) {
        if (strings[string_index]) {
            string s = string_from_cstring(strings[string_index]);
            if (s.length > length_of_longest) {
                length_of_longest = s.length;
            }
        }
    }
    return length_of_longest;
}

static u64 read_timestamp_counter(void) {
#ifndef __EMSCRIPTEN__
    return __rdtsc();
#else
    return 0;
#endif
}

/* more secure than format_temp, uses scratch buffer as backing memory. */
string format_temp_s(const char* fmt, ...);

struct calendar_time {
    s32 year;
    s32 day;
    s32 day_of_the_week;
    s32 month;

    s32 hours;
    s32 minutes;
    s32 seconds;
};

static char* day_strings[] = {
    "SUN",
    "MON",
    "TUE",
    "WED",
    "THU",
    "FRI",
    "SAT",
};

static char* month_strings[] = {
    "JAN",
    "FEB",
    "MAR",
    "APR",
    "MAY",
    "JUN",
    "JUL",
    "AUG",
    "SEP",
    "OCT",
    "NOV",
    "DEC",
};

u64 system_get_current_time(void) {
    return time(0);
}

struct calendar_time calendar_time_from(s64 timestamp) {
    time_t     current_time = timestamp; 
    struct tm* time_info    = localtime(&current_time);

    _debugprintf("%lld\n", timestamp);

    if (time_info) {
        return (struct calendar_time) {
            .year            = time_info->tm_year+1900,
            .month           = time_info->tm_mon,
            .hours           = time_info->tm_hour,
            .minutes         = time_info->tm_min,
            .seconds         = time_info->tm_sec,
            .day             = time_info->tm_mday,
            .day_of_the_week = time_info->tm_wday,
        };
    } else {
        return (struct calendar_time){};
    }
}

struct calendar_time current_calendar_time(void) {
    return calendar_time_from(system_get_current_time());
}

local char* memory_strings(size_t in_bytes) {
    return format_temp("%llu(B) (%llu)(KB) (%llu)(MB) (%llu)(GB)",
                       in_bytes,
                       in_bytes / Kilobyte(1),
                       in_bytes / Megabyte(1),
                       in_bytes / Gigabyte(1));
}
local char* biggest_valid_memory_string(size_t in_bytes) {
    size_t bytes     = in_bytes/Byte(1);
    size_t kilobytes = in_bytes/Kilobyte(1);
    size_t megabytes = in_bytes/Megabyte(1);
    size_t gigabytes = in_bytes/Gigabyte(1);

    if (gigabytes) {
        return format_temp("%llu (GB)", gigabytes);
    } else if (megabytes) {
        return format_temp("%llu (MB)", megabytes);
    } else if (kilobytes) {
        return format_temp("%llu (KB)", kilobytes);
    }

    return format_temp("%llu (B)", bytes);
}

local void OS_create_directory(string location) {
#ifdef _WIN32 
    string s = format_temp_s("%.*s", location.length, location.data);
    CreateDirectory(s.data, NULL);
#endif
}


enum endianess {
    ENDIANESS_LITTLE = 1,
    ENDIANESS_BIG    = 2,
};

local string endian_strings[] = {
    [ENDIANESS_LITTLE] = string_literal("(little-endian)"),
    [ENDIANESS_BIG] = string_literal("(big-endian)"),
};

#define ByteUnion(BITS, TYPE)                   \
    union {                                     \
        TYPE as_ ## TYPE;                           \
        u8   as_bytes[BITS/8];               \
    }

enum endianess system_get_endian(void) {
    ByteUnion(32, u32) x;
    x.as_u32 = 1;

    if (x.as_bytes[0]) {
        return ENDIANESS_LITTLE;
    }

    return ENDIANESS_BIG;
}

/* NOTE: I am horrified by how I use the preprocessor sometimes, */
#define Define_ByteSwap_Procedures(BITCOUNT)                            \
    inline local u ## BITCOUNT byteswap_u##BITCOUNT(u##BITCOUNT input) { \
        ByteUnion(BITCOUNT, u##BITCOUNT) x;                             \
        x.as_u##BITCOUNT = input;                                       \
        for (unsigned byte_index = 0; byte_index < sizeof(u##BITCOUNT)/2; ++byte_index) { \
            XorSwap(x.as_bytes[byte_index], x.as_bytes[(sizeof(u##BITCOUNT)-1) - byte_index]); \
        }                                                               \
        return x.as_u##BITCOUNT;                                        \
    }                                                                   \
    inline local s ## BITCOUNT byteswap_s##BITCOUNT(s##BITCOUNT input) { \
        ByteUnion(BITCOUNT, s##BITCOUNT) x;                             \
        x.as_s##BITCOUNT = input;                                       \
        for (unsigned byte_index = 0; byte_index < sizeof(s##BITCOUNT)/2; ++byte_index) { \
            XorSwap(x.as_bytes[byte_index], x.as_bytes[(sizeof(u##BITCOUNT)-1) - byte_index]); \
        }                                                               \
        return x.as_s##BITCOUNT;                                        \
    }                                                                   \
    inline local void inplace_byteswap_u##BITCOUNT(u##BITCOUNT *input) { \
        assertion(input && "cannot byteswap nullptr?");                 \
        u##BITCOUNT copy = *input;                                      \
        copy = byteswap_u##BITCOUNT(copy);                              \
        *input = copy;                                                  \
    }                                                                   \
    inline local void inplace_byteswap_s##BITCOUNT(s##BITCOUNT *input) { \
        assertion(input && "cannot byteswap nullptr?");                 \
        s##BITCOUNT copy = *input;                                      \
        copy = byteswap_s##BITCOUNT(copy);                              \
        *input = copy;                                                  \
    }

Define_ByteSwap_Procedures(64);
Define_ByteSwap_Procedures(32);
Define_ByteSwap_Procedures(16);
inline local u8 byteswap_u8(u8 input) {
    return input;
}
inline local s8 byteswap_s8(s8 input) {
    return input;
}
inline local void inplace_byteswap_s8(s8* input) {
    return;
}
inline local void inplace_byteswap_u8(u8* input) {
    return;
}
inline local f32 byteswap_f32(f32 input) {
    ByteUnion(32, f32) x;
    x.as_f32 = input;
    for (unsigned byte_index = 0; byte_index < sizeof(f32)/2; ++byte_index) {
        XorSwap(x.as_bytes[byte_index], x.as_bytes[(sizeof(f32)-1) - byte_index]);
    }
    return x.as_f32;
}
inline local f64 byteswap_f64(f64 input) {
    ByteUnion(64, f64) x;
    x.as_f64 = input;
    for (unsigned byte_index = 0; byte_index < sizeof(f64)/2; ++byte_index) {
        XorSwap(x.as_bytes[byte_index], x.as_bytes[(sizeof(f64)-1) - byte_index]);
    }
    return x.as_f64;
}
inline local void inplace_byteswap_f32(f32 *input) {
    assertion(input && "cannot byteswap nullptr?");
    f32 copy = *input;
    copy = byteswap_f32(copy);
    *input = copy;
}
inline local void inplace_byteswap_f64(f64 *input) {
    assertion(input && "cannot byteswap nullptr?");
    f64 copy = *input;
    copy = byteswap_f32(copy);
    *input = copy;
}

local void _debug_print_bitstring(u8* bytes, unsigned length) {
    unsigned bits = length * 8;
    _debugprintfhead();
    /* reverse print, higher addresses come first, lower addresses come last. To give obvious endian representation */
    for (s32 bit_index = bits-1; bit_index >= 0; --bit_index) {
        unsigned real_index   = (bit_index / 8);
        u8       current_byte = bytes[real_index];
        _debugprintf1("%d", (current_byte & BIT(bit_index % 8)) > 0);
        if (((bit_index) % 8) == 0) {
            _debugprintf1(" ");   
        }
    }
    _debugprintf1("\n");
}

#undef Define_ByteSwap_Procedures

#endif
