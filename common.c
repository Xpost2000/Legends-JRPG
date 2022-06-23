#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include <stdbool.h>

#include <assert.h>
#include <time.h>

#define assertion(x) assert(x)

#define BIT(x)             (x << 1)
#define BIT64(x) (uint64_t)(x << 1LL)

#define Kilobyte(x)         (uint64_t)(x * 1024LL)
#define Megabyte(x)         (uint64_t)(x * 1024LL * 1024LL)
#define Gigabyte(x)         (uint64_t)(x * 1024LL * 1024LL * 1024LL)
#define Terabyte(x)         (uint64_t)(x * 1024LL * 1024LL * 1024LL * 1024LL)

#define unused(x) (void)(x)

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

#define Swap(a, b, type)                        \
    do {                                        \
        type tmp = a;                           \
        a = b;                                  \
        b = tmp;                                \
    } while (0)

s32 clamp_s32(s32 x, s32 min, s32 max) {
    if (x < min) x = min;
    if (x > max) x = max;
    return x;
}
f32 clamp_f32(f32 x, f32 min, f32 max) {
    if (x < min) x = min;
    if (x > max) x = max;
    return x;
}

f32 lerp_f32(f32 a, f32 b, f32 normalized_t) {
    return a * (1 - normalized_t) + (b * normalized_t);
}

static size_t _globally_tracked_memory_allocation_counter = 0;
static size_t _globally_tracked_memory_allocation_peak    = 0;

struct tracked_memory_allocation_header {
    size_t amount;
};

/* use 0 for destination_size if I don't care. Ideally I should know both though. */
static inline void memory_copy(void* source, void* destination, size_t amount_from_source, size_t destination_size) {
    if (destination_size != 0) {
        if (amount_from_source > destination_size)
            amount_from_source = destination_size;
    }

    for (u64 index = 0; index < amount_from_source; ++index) {
        ((u8*)destination)[index] = ((u8*)source)[index];
    }
}

static inline void zero_memory(void* memory, size_t amount) {
    for (u64 index = 0; index < amount; ++index) {
        ((u8*)memory)[index] = 0;
    }
}

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

void* system_heap_memory_allocate(size_t amount) {
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

    return system_heap_memory_allocate(new_amount);
}

void system_heap_memory_deallocate(void* pointer) {
    if (pointer) {
        void* header = pointer - sizeof(struct tracked_memory_allocation_header);
        _globally_tracked_memory_allocation_counter -= ((struct tracked_memory_allocation_header*)(header))->amount;
        free(header);
    }
}

bool system_heap_memory_leak_check(void) {
    return (_globally_tracked_memory_allocation_counter == 0);
}

size_t system_heap_currently_allocated_amount(void) {
    return _globally_tracked_memory_allocation_counter;
}

size_t system_heap_peak_allocated_amount(void) {
    return _globally_tracked_memory_allocation_peak;
}

struct file_buffer {
    u8* buffer;
    u64 length;
};

/* TODO should be using a memory arena, although this means we can't really have this here lol */
/* TODO We should also be using a length string... */
bool   file_exists(char* path) {
    FILE* f = fopen(path, "r");

    if (f) {
        fclose(f);
        return true;
    }

    return false;
}

size_t file_length(char* path) {
    size_t result = 0;
    FILE*  file   = fopen(path, "rb+");

    if (file) {
        fseek(file, 0, SEEK_END);
        result = ftell(file);
        fclose(file);
    }

    return result;
}

struct file_buffer read_entire_file(char* path) {
    size_t file_size   = file_length(path);
    u8*    file_buffer = system_heap_memory_allocate(file_size+1);

    FILE* file = fopen(path, "rb+");
    fread(file_buffer, 1, file_size, file);
    fclose(file);

    file_buffer[file_size] = 0;
    return (struct file_buffer) {
        .buffer = file_buffer,
        .length = file_size,
    };
}

void file_buffer_free(struct file_buffer* file) {
    system_heap_memory_deallocate(file->buffer);
}

struct rectangle_f32 {
    f32 x;
    f32 y;
    f32 w;
    f32 h;
};
#define rectangle_f32(X,Y,W,H) (struct rectangle_f32){.x=X,.y=Y,.w=W,.h=H}
#define RECTANGLE_F32_NULL rectangle_f32(0,0,0,0)

#include "prng.c"
