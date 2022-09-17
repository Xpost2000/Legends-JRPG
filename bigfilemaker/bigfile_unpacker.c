#include "bigfile_def.c"

bigfile_blob_t bigfile_load_blob(IAllocator allocator, string path) {
    struct file_buffer buffer = read_entire_file(allocator, path);
    return buffer.buffer;
}

s32 bigfile_get_version(bigfile_blob_t blob) {
    s32* address_of_version = (s32*)(blob + 0);
    return (*address_of_version);
}

s32 bigfile_get_record_count(bigfile_blob_t blob) {
    s32* address_of_record_count = (s32*)(blob + sizeof(s32));
    return (*address_of_record_count);
}

s64 bigfile_get_file_data_size(bigfile_blob_t blob) {
    s64* address_of_file_data_size = (s64*)(blob + sizeof(s32) * 2);
    return (*address_of_file_data_size);
}

struct bigfile_record* bigfile_get_records(bigfile_blob_t blob) {
    struct bigfile_record* address_of_records = (struct bigfile_record*)(blob + sizeof(s32)*2 + sizeof(s64));
    return address_of_records;
}

u8* bigfile_get_file_data_bytes(bigfile_blob_t blob) {
    s32                             record_count        = bigfile_get_record_count(blob);
    struct bigfile_record*          address_of_records  = bigfile_get_records(blob);
    u8*                             byte_location       = (u8*)address_of_records;
    byte_location                                      += record_count * sizeof(*address_of_records);

    return byte_location;
}

struct bigfile_record* bigfile_get_record_by_name(bigfile_blob_t blob, string file) {
    s32                    record_count        = bigfile_get_record_count(blob);
    struct bigfile_record* all_records         = bigfile_get_records(blob);

    for (s32 record_index = 0; record_index < record_count; ++record_index) {
        string record_name = string_from_cstring(all_records[record_index].name);

        if (string_equal(record_name, file)) {
            return &all_records[record_index];
        }
    }

    return NULL;
}

struct bigfile_file_data bigfile_get_raw_file_data_by_name(bigfile_blob_t blob, string file) {
    u8*                    all_file_data_bytes = bigfile_get_file_data_bytes(blob);

    struct bigfile_file_data result = {};
    struct bigfile_record* record = bigfile_get_record_by_name(blob, file);

    if (record) {
        u8* actual_location = all_file_data_bytes + record->data_offset;
        result.data         = actual_location;
        result.length       = record->compressed_size;
    }

    return result;
}
