/*
  Basic metaprogram for legends.

  Will add features as I occasionally want to reduce work,
  also metaprogramming playground for myself.

  Would be curious to get metagen to autogenerate many of the wrapper script
  functions, since the vast majority of the script functions are trivially
  wrappable.
*/

#define RELEASE
#include "common.c"
#include "stretchy_buffer_extra.c"

/* don't care about anything else right now. */
struct game_script_function_entry {char name[255];};
static struct game_script_function_entry game_script_function_entry(char* name, int length) {
    struct game_script_function_entry result = {};
    cstring_copy(name, result.name, length);
    return result;
}

static struct game_script_function_entry* global_game_script_functions_list = 0;
static struct memory_arena                arena;

string format_temp_s(const char* fmt, ...) {
    return string_literal("unused. just hack around it");
}

string* string_sliced_into_lines(string original) {
    string* line_list = 0;

    s32 character_index = 0;

    while (character_index < original.length) {
        switch (original.data[character_index]) {
            case '\n': {
                character_index++;
            } break;
            default: {
                s32 line_start = character_index;
                while (original.data[character_index] != '\n' && character_index < original.length) {
                    character_index++;
                }
                s32 line_end = character_index;

                string current_line = string_slice(original, line_start, line_end);
                array_push(line_list, current_line);
            } break;
        }
    }

    return line_list;
}

/* we don't need to tokenize surprisingly :) */
static void parse_for_game_script_entries_and_append(string filestring) {
    string* lines = string_sliced_into_lines(filestring);

    for (unsigned line_index = 0; line_index < array_get_count(lines); ++line_index) {
        string current_line         = lines[line_index];
        string substring_to_compare = string_slice(current_line, 0, cstring_length("GAME_LISP_FUNCTION"));

        if (string_is_substring(substring_to_compare, string_literal("GAME_LISP_FUNCTION"))) {
            unsigned start_of_parens = 0;
            while (current_line.data[start_of_parens] != '(') start_of_parens++;
            unsigned end_of_parens   = start_of_parens;
            while (current_line.data[end_of_parens] != ')') end_of_parens++;

            /* string function_name = string_slice(current_line, start_of_parens+1, end_of_parens); */
            string function_name = string_slice(current_line, start_of_parens+1, end_of_parens);
            printf("(%d, %d)[%d]Found game lisp binding: %.*s\n", start_of_parens, end_of_parens, current_line.length, function_name.length, function_name.data);

            struct game_script_function_entry entry = game_script_function_entry(function_name.data, function_name.length);
            array_push(global_game_script_functions_list, entry);
        } else {
            continue;
        }
    }
    array_finish(lines);
}

static void generate_game_script_builtins_procedure_table(char* argv0, const char* outfile) {
    printf("Going to generate: %s\n", outfile);

    {
        /*
          Prune as many directories as possible and automatically generate the binding table.
          generally though all script procedures should be in game_script_procs.c... If they aren't that's
          okay too...
         */
        struct directory_listing listing = directory_listing_list_all_files_in(&arena, string_literal("./"));

        static string blacklisted_files[] = {
            string_literal("metagen.c"),
            string_literal("generated_game_script_proc_table.c"),
        };

        for (unsigned entry_index = 0; entry_index < listing.count; ++entry_index) {
            string filename = string_from_cstring(listing.files[entry_index].name);

            if (!listing.files[entry_index].is_directory) {
                if (!string_is_substring(filename,string_literal(".c")) &&
                    !string_is_substring(filename,string_literal(".h")) &&
                    !string_is_substring(filename,string_literal(".cc"))) {
                    continue;
                }

                bool cancel = false;
                for (unsigned blacklist_entry_index = 0; blacklist_entry_index < array_count(blacklisted_files); ++blacklist_entry_index) {
                    if (string_equal(filename, blacklisted_files[blacklist_entry_index])) {
                        cancel = true;
                        printf("Bad file: %.*s\n", filename.length, filename.data);
                        break;
                    }
                }

                if (!cancel) {
                    struct file_buffer input_file = read_entire_file(heap_allocator(), filename);
                    printf("parsing: %.*s\n", filename.length, filename.data);
                    parse_for_game_script_entries_and_append(file_buffer_as_string(&input_file));
                    file_buffer_free(&input_file);
                }
            }
        }
    }

#if 1
    FILE* output = fopen(outfile, "wb+");

    fprintf(output, "// This file was autogenerated by %s, don't manually touch!\n", argv0);
    fprintf(output, "local struct game_script_function_builtin script_function_table[] =");
    fprintf(output, "{\n");
    for (unsigned index = 0; index < array_get_count(global_game_script_functions_list); ++index) {
        struct game_script_function_entry* current_entry = global_game_script_functions_list+index;
        fprintf(output, "\tGAME_LISP_FUNCTION(%s),\n", current_entry->name);
    }
    fprintf(output, "};\n");

    printf("Okay! Wrote %s\n", outfile);
    fclose(output);
#endif
}

int main(int argc, char** argv) {
    arena = memory_arena_create_from_heap("LOL", Megabyte(16));
    {
        generate_game_script_builtins_procedure_table(argv[0], "generated_game_script_proc_table.c");
    }
    return 0;
}
