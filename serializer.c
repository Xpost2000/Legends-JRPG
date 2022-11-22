#include "serializer_def.c"

void serializer_set_endianess(struct binary_serializer* serializer, enum endianess expected_endianess) {
    serializer->expected_endianess = expected_endianess;
}

struct binary_serializer open_write_file_serializer(string filename) {
    struct binary_serializer result = {};
    result.mode = BINARY_SERIALIZER_WRITE;
    result.type = BINARY_SERIALIZER_FILE;

    _debugprintf("requested to open: %s\n", filename.data);
    result.file_handle = fopen(filename.data, "wb+");
    return result;
}

struct binary_serializer open_read_file_serializer(string filename) {
    struct binary_serializer result = {};
    result.mode = BINARY_SERIALIZER_READ;
    result.type = BINARY_SERIALIZER_FILE;

    _debugprintf("requested to open: %s\n", filename.data);
    result.file_handle = fopen(filename.data, "rb+");
    return result;
}

struct binary_serializer open_write_memory_serializer(void) {
    struct binary_serializer result = {};
    result.mode = BINARY_SERIALIZER_WRITE;
    result.type = BINARY_SERIALIZER_MEMORY;

    return result;
}

struct binary_serializer open_read_memory_serializer(void* buffer, size_t buffer_size) {
    struct binary_serializer result = {};
    result.mode = BINARY_SERIALIZER_READ;
    result.type = BINARY_SERIALIZER_MEMORY;

    result.memory_buffer.size   = buffer_size;
    result.memory_buffer.buffer = buffer;

    return result;
}

void serializer_finish(struct binary_serializer* serializer) {
    switch (serializer->type) {
        case BINARY_SERIALIZER_FILE: {
            if (serializer->file_handle) {
                fclose(serializer->file_handle);
                serializer->file_handle = NULL;
            }
        } break;
        case BINARY_SERIALIZER_MEMORY: {
            if (serializer->mode == BINARY_SERIALIZER_WRITE) {
                /*free all memory*/ {
                    struct binary_serializer_memory_node* current_node = serializer->memory_nodes.head;

                    while (current_node) {
                        struct binary_serializer_memory_node* next = current_node->next;
                        system_heap_memory_deallocate(current_node);
                        current_node = next;
                    }
                }
            }
        } break;
            invalid_cases();
    }
}

/* does not use an arena, but maybe it should? */
void* serializer_flatten_memory(struct binary_serializer* serializer, size_t* size) {
    assertion(serializer->type == BINARY_SERIALIZER_MEMORY &&
              serializer->mode == BINARY_SERIALIZER_WRITE &&
              "Incompatible serializer type");
    size_t buffer_size = 0;
    /*count buffer size*/ {
        struct binary_serializer_memory_node* current_node = serializer->memory_nodes.head;

        while (current_node) {
            struct binary_serializer_memory_node* next = current_node->next;
            buffer_size += current_node->size;
            current_node = next;
        }
    }

    uint8_t* buffer = system_heap_memory_allocate(buffer_size);
    /*copy all data*/{
        struct binary_serializer_memory_node* current_node = serializer->memory_nodes.head;

        size_t written = 0;
        while (current_node) {
            struct binary_serializer_memory_node* next = current_node->next;
            memcpy(buffer + written, current_node->buffer, current_node->size);
            written += current_node->size;
            current_node = next;
        }
    }

    safe_assignment(size) = buffer_size;
    return buffer;
}

/* simple singly linked list */
void serializer_push_memory_node(struct binary_serializer* serializer, void* bytes, size_t size) {
    struct binary_serializer_memory_node* new_node = system_heap_memory_allocate(size + sizeof(*new_node));
    new_node->size = size;
    memcpy(new_node->buffer, bytes, size);

    if (!serializer->memory_nodes.head) {
        serializer->memory_nodes.head = new_node;
    }

    if (serializer->memory_nodes.tail) {
        serializer->memory_nodes.tail->next = new_node;
    }

    serializer->memory_nodes.tail = new_node;
}

void serialize_bytes(struct binary_serializer* serializer, void* bytes, size_t size) {
    switch (serializer->type) {
        case BINARY_SERIALIZER_FILE: {
            assert(serializer->file_handle && "File handle not opened on file serializer?");

            if (serializer->mode == BINARY_SERIALIZER_READ) {
                fread(bytes, size, 1, serializer->file_handle);
            } else {
                fwrite(bytes, size, 1, serializer->file_handle);
            }
        } break;
        case BINARY_SERIALIZER_MEMORY: {
            if (serializer->mode == BINARY_SERIALIZER_READ) {
                _debugprintf("already read: (%llu) : (%p) (sz to read %llu)", serializer->memory_buffer.already_read, serializer->memory_buffer.buffer, size);
                memcpy(bytes, serializer->memory_buffer.buffer + serializer->memory_buffer.already_read, size);
                serializer->memory_buffer.already_read += size;
            } else {
                serializer_push_memory_node(serializer, bytes, size);
            }
        } break;
            invalid_cases();
    }
}

void serialize_format(struct binary_serializer* serializer, char* format_string, ...) {
    va_list variadic_arguments;
    va_start(variadic_arguments, format_string);
    {
        switch (serializer->type) {
            case BINARY_SERIALIZER_FILE: {
                FILE* file_handle = serializer->file_handle;

                if (serializer->mode == BINARY_SERIALIZER_READ) {
                    vfscanf(file_handle, format_string, variadic_arguments);
                } else {
                    vfprintf(file_handle, format_string, variadic_arguments);
                }
            } break;
            case BINARY_SERIALIZER_MEMORY: {
                assertion(!"TODO: Has not given good consideration to how this should be implemented.");
            } break;
        }
    }
    va_end(variadic_arguments);
}

#define Serialize_Object_Into_File(type)                                \
    case BINARY_SERIALIZER_FILE: {                                      \
        assert(serializer->file_handle && "File handle not opened on file serializer?"); \
        if (serializer->mode == BINARY_SERIALIZER_READ) {               \
            fread(&obj_, sizeof(type), 1, serializer->file_handle);       \
        } else {                                                        \
            fwrite(&obj_, sizeof(type), 1, serializer->file_handle);      \
        }                                                               \
    } break
#define Serialize_Object_Into_Memory_Buffer(type)                       \
    case BINARY_SERIALIZER_MEMORY: {                                    \
        if (serializer->mode == BINARY_SERIALIZER_READ) {               \
            memcpy(&obj_, serializer->memory_buffer.buffer + serializer->memory_buffer.already_read, sizeof(type)); \
            serializer->memory_buffer.already_read += sizeof(type);     \
        } else {                                                        \
            serializer_push_memory_node(serializer, &obj_, sizeof(type)); \
        }                                                               \
    } break

/* Pretty sure there's no special casing so this is okay. C "templates" to the rescue! */
/* endian aware for these primitive types */
#define Define_Serializer_Function(Typename, Type)                      \
    void serialize_##Typename(struct binary_serializer* serializer, Type* obj) { \
        Type obj_ = *obj;                                               \
        if (sizeof(Type) > 1) {                                         \
            assertion(serializer->expected_endianess != 0 && "Serializer does not have a set endian target!"); \
            if (system_get_endian() != serializer->expected_endianess) { \
                obj_ = byteswap_##Type(obj_);                           \
            }                                                           \
        }                                                               \
        switch (serializer->type) {                                     \
            Serialize_Object_Into_File(Type);                           \
            Serialize_Object_Into_Memory_Buffer(Type);                  \
            invalid_cases();                                            \
        }                                                               \
    }

Define_Serializer_Function(u64, u64);
Define_Serializer_Function(s64, s64);
Define_Serializer_Function(u32, u32);
Define_Serializer_Function(s32, s32);
Define_Serializer_Function(u16, u16);
Define_Serializer_Function(s16, s16);
Define_Serializer_Function(u8 , u8);
Define_Serializer_Function(s8 , s8);
Define_Serializer_Function(f32, f32);
Define_Serializer_Function(f64, f64);
/*
  helpful macros

  NOTE: Not so helpful anymore once we introduced endian-awareness,
  but :/

  I mean they're okay for strings. Which are always safe to write.
*/

#define Serialize_Fixed_Array_And_Allocate_From_Arena(serializer, arena, type, counter, array) \
    do {                                                                \
        serialize_##type(serializer, &counter);                         \
        _debugprintf("allocating: %d %s objects", counter, #array);      \
        if (serializer->mode == BINARY_SERIALIZER_READ)                 \
            array = memory_arena_push(arena, counter * sizeof(*array)); \
        serialize_bytes(serializer, array, counter * sizeof(*array));   \
    } while (0)
#define Serialize_Fixed_Array(serializer, type, counter, array) \
    do {                                                                \
        serialize_##type(serializer, &counter);                         \
        _debugprintf("allocating: %d (%s) objects", counter, #array);      \
        serialize_bytes(serializer, array, counter * sizeof(*array));   \
    } while (0)
#define Serialize_Structure(serializer, structure) \
    do {                                                                \
        serialize_bytes(serializer, &structure, sizeof(structure));     \
    } while (0)


#undef Serialize_Object_Into_File
