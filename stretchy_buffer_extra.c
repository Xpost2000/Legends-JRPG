#ifndef STRETCHYBUFFER_C
#define STRETCHYBUFFER_C

/*
  This is not very explicit which is not usually my style but I figured this would hurt me less...

  Ideally this would allow you to parameterize allocations

  NOTE: DO NOT USE THIS IN THE MAIN ENGINE CODE / GAME CODE.
  THIS IS ONLY FOR TOOLS USAGE! SINCE DYNAMIC ALLOCATION IS BASICALLY A NO DURING
  GAME RUNTIME, AND I'D LIKE TO KEEP IT THAT WAY!
*/

typedef struct {
    s32 count;
    s32 capacity;
    s32 object_size;
} array_header;

#define ARRAY_INITIAL_CAP (16)

/* scary macro black magic */
s32 array_get_count(void* ar) {
    array_header* h = (u8*)ar - sizeof(*h);
    return h->count;
}

void array_finish(void* ar) {
    free((u8*)ar - sizeof(array_header));
}

#define array_push(AR, V) do {                                          \
        if (!AR) {                                                      \
            AR = malloc(sizeof(array_header) + sizeof(V) * ARRAY_INITIAL_CAP); \
            u8* AR_ = AR;                                               \
            {                                                           \
                array_header* R = AR_;                                  \
                R->count        = 0;                                    \
                R->capacity     = ARRAY_INITIAL_CAP;                    \
                R->object_size  = sizeof(*AR);                          \
            }                                                           \
            AR_ += sizeof(array_header);                                \
            AR = AR_;                                                   \
        }                                                               \
        {                                                               \
            array_header* R = ((u8*)AR - sizeof(array_header));         \
            R->count++;                                                 \
            if (R->count >= R->capacity) {                              \
                R->capacity *= 2;                                       \
                u8* new_r = realloc((u8*)AR-sizeof(array_header), R->capacity*R->object_size + sizeof(array_header)); \
                R = ((u8*)new_r);                                       \
                AR = new_r + sizeof(array_header);                      \
            }                                                           \
            AR[R->count-1] = V;                                         \
        }                                                               \
    } while (0);         

#endif
