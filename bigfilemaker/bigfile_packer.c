/*
  Big File archive

  a simple directory packer, for release versions of the game.

  It's a simpler than zip file format, that's supposed to be pretty
  brainless to implement.

  Might have compression but for now basically just concatenates a bunch of
  files together

  Just meant to reduce files to distribute.

  This is it's own separate program.

  Windows only right now

  VERISON 1: Very similar to the fallout 1 file structure,
  where all entries are just stored as array entries (fullname and all). Linearly
  search through all of them to unpack.

  The interface for the file format will remain identical fyi. We just don't have
  any accelerating datastructures for anything.

  Most files are loaded once (at startup), which is why this is okay
  for me.

  If I need to, we'll switch to a hashmap...

  (it'd technically be faster to just store the hashes for bigfile names, so that way we save lots of space.)
*/

#include <stdarg.h>
#define RELEASE
#include "../common.c"
#include "../serializer_def.c"
#include "../serializer.c"

struct memory_arena archive_arena = {};
struct memory_arena scratch_arena = {};

#include "bigfile_def.c"

/* out has enough memory to hold results */
void produce_normalized_path(string in, char* out) {
    s32 character_index = 0;

    if (in.data[character_index] == '.' && in.data[character_index+1] == '/' || in.data[character_index+1] == '\\') {
        out[character_index] = '.';
        out[character_index+1] = '/';

        character_index += 1;
        character_index += 1;
    }

    for (; character_index < in.length; ++character_index) {
        if (in.data[character_index] == '\\') {
            out[character_index] = '/';
        } else {
            out[character_index] = in.data[character_index];
        }
    }
}

void read_and_add_directory_entries_recursively(struct directory_listing* listing, struct bigfile_archive* archive) {
    for (s32 directory_entry_index = 2; directory_entry_index < listing->count; ++directory_entry_index) {
        struct directory_file* file_entry = listing->files + directory_entry_index;
        printf("recursively reading: \"%s\" (%d)\n", file_entry->name, file_entry->is_directory);

        if (file_entry->is_directory) {
            printf("Trying to read subdirectory: \"%s\"\n", file_entry->name);
            string basepath = string_from_cstring(listing->basename);
            string fullpath = string_concatenate(&scratch_arena, string_concatenate(&scratch_arena, basepath, string_literal("/")), string_from_cstring(file_entry->name));
            char fixed_path[BIGFILE_RECORD_MAX_PATH] = {};
            produce_normalized_path(fullpath, fixed_path);

            struct directory_listing subdirectory_listing = directory_listing_list_all_files_in(&scratch_arena, fullpath);
            read_and_add_directory_entries_recursively(&subdirectory_listing, archive);
        } else {
            struct bigfile_record* record = archive->records + archive->record_count;
            archive->record_count++;
            memory_arena_push(&archive_arena, sizeof(*record));

            string basepath = string_from_cstring(listing->basename);
            string fullpath = string_concatenate(&scratch_arena, string_concatenate(&scratch_arena, basepath, string_literal("/")), string_from_cstring(file_entry->name));
            char fixed_path[BIGFILE_RECORD_MAX_PATH] = {};
            produce_normalized_path(fullpath, fixed_path);
            {cstring_copy(fixed_path, record->name, BIGFILE_RECORD_MAX_PATH);}

            record->compression_type  = BIGFILE_COMPRESSION_NONE;
            record->type              = BIGFILE_RECORD_FILE;
            record->uncompressed_size = file_entry->filesize;
            record->compressed_size   = file_entry->filesize;
            record->data_offset       = -1;
        }
    }
    printf("done reading one recursive subdir\n");
}

int main(int argc, char** argv) {
    archive_arena = memory_arena_create_from_heap("ARENA", Megabyte(128));
    scratch_arena = memory_arena_create_from_heap("SCRATCH", Megabyte(16));

    if (argc > 2) {
        string archive_name = string_from_cstring(argv[1]);
        {
            struct bigfile_archive packed_archive = {};
            packed_archive.version      = BIGFILE_ARCHIVE_CURRENT_VERSION;
            packed_archive.record_count = 0;
            packed_archive.records      = memory_arena_push(&archive_arena, 0);

            for (s32 argument_index = 2; argument_index < argc; ++argument_index) {
                string argument = string_from_cstring(argv[argument_index]);

                printf("Looking at : \"%.*s\"\n", argument.length, argument.data);

                if (path_exists(argument)) {
                    if (is_path_directory(argument)) {
                        struct directory_listing listing = directory_listing_list_all_files_in(&scratch_arena, argument);
                        read_and_add_directory_entries_recursively(&listing, &packed_archive);
                        printf("\t\"%.*s\" is a directory\n", argument.length, argument.data);
                    } else {
                        struct bigfile_record* record = packed_archive.records + packed_archive.record_count;
                        packed_archive.record_count++;
                        memory_arena_push(&archive_arena, sizeof(*packed_archive.records));

                        char fixed_path[BIGFILE_RECORD_MAX_PATH] = {};
                        produce_normalized_path(argument, fixed_path);
                        {cstring_copy(fixed_path, record->name, BIGFILE_RECORD_MAX_PATH);}

                        record->compression_type  = BIGFILE_COMPRESSION_NONE;
                        record->type              = BIGFILE_RECORD_FILE;
                        record->uncompressed_size = file_length(string_from_cstring(fixed_path));
                        record->compressed_size   = file_length(string_from_cstring(fixed_path));
                        record->data_offset       = -1;

                        printf("\t\"%.*s\" is a file\n", argument.length, argument.data);
                    }
                } else {
                    printf("\t\"%.*s\" does not exist.", argument.length, argument.data);
                }
            }

            printf("Preallocation of packs, created %d entries.\n", packed_archive.record_count);
            printf("Beginning file loading\n");

            s64 beginning_offset_of_file_chunks = sizeof(s32) + sizeof(s32) + sizeof(*packed_archive.records) * packed_archive.record_count;

            u8* start_pointer_to_file_chunks_in_arena = memory_arena_push(&archive_arena, 0);
            packed_archive.file_data = start_pointer_to_file_chunks_in_arena;

            for (s32 record_index = 0; record_index < packed_archive.record_count; ++record_index) {
                struct bigfile_record* record = packed_archive.records + record_index;
                printf("Packing \"%s\"\n", record->name);

                #if BIGFILE_ARCHIVE_CURRENT_VERSION == 1
                assertion(record->type == BIGFILE_RECORD_FILE && "Version 1 should only have file entries!");
                #endif

                struct file_buffer file_data = read_entire_file(memory_arena_allocator(&archive_arena), string_from_cstring(record->name));

                /* read_entire_file adds a null byte */
                s64 occupied_file_data_size = file_data.length+1;

                packed_archive.file_data_size += occupied_file_data_size;
                {record->data_offset = beginning_offset_of_file_chunks;}
                beginning_offset_of_file_chunks += occupied_file_data_size;
                printf("\t%d bytes read from file\n", occupied_file_data_size);
            }

            /* okay now everything should be ready to directly serialize */

            {
                struct binary_serializer writer = open_write_file_serializer(archive_name);

                serialize_s32(&writer, &packed_archive.version);
                serialize_s32(&writer, &packed_archive.record_count);
                serialize_s64(&writer, &packed_archive.file_data_size);
                serialize_bytes(&writer, packed_archive.records, packed_archive.record_count * sizeof(*packed_archive.records));
                serialize_bytes(&writer, start_pointer_to_file_chunks_in_arena, packed_archive.file_data_size);

                serializer_finish(&writer);
            }
        }
    } else {
        printf("bigfile packer needs file names and directories.\n");
    }

    return 0;
}
