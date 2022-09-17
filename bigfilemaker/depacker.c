#include <stdarg.h>
#define RELEASE
#include "../common.c"
#include "bigfile_def.c"

#include "bigfile_unpacker.c"

int main(int argc, char** argv) {
    if (argc > 2) {
        bigfile_blob_t bf = bigfile_load_blob(heap_allocator(), string_from_cstring(argv[1]));

        for (s32 i = 2; i < argc; ++i) {
            string remaining_file = string_from_cstring(argv[i]);
            struct bigfile_file_data contents = bigfile_get_raw_file_data_by_name(bf, remaining_file);

            if (contents.data) {
                printf("%.*s found!\n", remaining_file.length, remaining_file.data);
                printf("=========== BEGIN CONTENTS\n");
                for (s32 j = 0; j < contents.length; ++j) {
                    putc(contents.data[j], stdout);
                }
                printf("=========== END CONTENTS\n");
            }
        }
    } else {
        printf("depacker-test needs a bigfile archive and a file");
    }
    return 0;
}
