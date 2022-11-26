/* NOTE: Should provide a streamed version */
/* as the current version of the API copies the entire archive into memory which is kind of bad */
/* we only need to load the header at any given moment, but the game engine copies all relevant data from the archive so yeah. */

#ifndef BIGFILE_DEF_C
#define BIGFILE_DEF_C

#define BIGFILE_RECORD_MAX_PATH         (64) /* big enough for me. But pretty wasteful */
#define BIGFILE_ARCHIVE_CURRENT_VERSION (1)

enum bigfile_record_type {
    BIGFILE_RECORD_FILE,
    BIGFILE_RECORD_DIRECTORY,
};
enum bigfile_compression_type {
    BIGFILE_COMPRESSION_NONE,
    BIGFILE_COMPRESSION_COUNT,
};

/*
  for directories, I'm going to be a bit scared of how to handle nesting structures...

  Pointers upon pointers.
*/
struct bigfile_record {
    char name[BIGFILE_RECORD_MAX_PATH];
    u8 type;
    u8 compression_type;
    s64 uncompressed_size;
    s64 compressed_size; /* this is the "real byte" size as in the archive. */
    s64 data_offset; /* offset to start reading file data */
};

/* code structure, only for the packer. The code API is just pure binary. */
struct bigfile_archive {
    s32 version;
    s32 record_count;
    s64 file_data_size;
    struct bigfile_record* records;
    u8*                    file_data;
};

/* NOTE big files are manipulated at runtime only through binary blogs, the structs are mostly for the
   packer which does have a structured handling method.

   the bigfile records are the only structs you can use at runtime
 */

/* for unpacker.c */
typedef u8* bigfile_blob_t;
/* no deallocate right now */
bigfile_blob_t         bigfile_load_blob(IAllocator allocator, string path);
s32                    bigfile_get_version(bigfile_blob_t blob);
s32                    bigfile_get_record_count(bigfile_blob_t blob);
s64                    bigfile_get_file_data_size(bigfile_blob_t blob);
struct bigfile_record* bigfile_get_records(bigfile_blob_t blob);
u8*                    bigfile_get_file_data_bytes(bigfile_blob_t blob);

struct bigfile_file_data {
    s64 length;
    u8* data;
};
/* does not handle decompression if that's a thing (which it isn't right now.) */
struct bigfile_record*   bigfile_get_record_by_name(bigfile_blob_t blob, string file);
struct bigfile_file_data bigfile_get_raw_file_data_by_name(bigfile_blob_t blob, string file);

#endif
