/* lex and tokenize some text I guess */
/* basically we run some "load-time" lisp to construct a tree that fits the limited set of features
   that are supported in-engine. Hopefully pretty easy. */

/* this is a lispable token */
enum token_type {
    TOKEN_TYPE_NONE,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_LIST,
    TOKEN_TYPE_T,
    TOKEN_TYPE_NIL,
    TOKEN_TYPE_NUMBER,
    /* few special "punctuation tokens", although not really needed to separate per say... */
    TOKEN_TYPE_COLON,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_SINGLE_QUOTE,
    TOKEN_TYPE_COUNT
};
static string token_type_strings[] = {
    string_literal("(none)"),
    string_literal("(symbol)"),
    string_literal("(string)"),
    string_literal("(list)"),
    string_literal("(T)"),
    string_literal("(NIL)"),
    string_literal("(number)"),
    string_literal("(colon)"),
    string_literal("(single-quote)"),
    string_literal("(count)"),
};
struct lexer_token {
    u32    type;
    u8     is_real;
    string str;
};
#define NULL_TOKEN (struct lexer_token){}
struct lexer {
    string              buffer;
    s32                 current_read_index;
};

bool lexer_done(struct lexer* lexer) {
    if (lexer->current_read_index < lexer->buffer.length) {
        return false;
    }

    return true;
}

char lexer_eat_next_character(struct lexer* lexer) {
    if (!lexer_done(lexer)) {
        char result = (char)(lexer->buffer.data[lexer->current_read_index++]);
        return result;
    }

    return '\0';
}

char lexer_peek_next_character(struct lexer* lexer) {
    if (!lexer_done(lexer)) {
        char result = (char)(lexer->buffer.data[lexer->current_read_index]);
        return result;
    }

    return '\0';
}

string lexer_slice_from_file_buffer(struct lexer* lexer, s32 start, s32 end) {
    return string_slice(lexer->buffer, start, end);
}

/* 
   NOTE: this is supposed to return a giant lisp form that we can just read.
*/
struct lexer_token lexer_try_to_eat_list(struct lexer* lexer) {
    s32 list_start_index = lexer->current_read_index-1; /* have to include parenthesis */
    s32 list_end_index   = list_start_index;

    s32 balancer = 1;
    while (!lexer_done(lexer)) {
        char eaten = lexer_eat_next_character(lexer);

        switch (eaten) {
            case '(': {
                balancer++;
            } break;
            case ')': {
                balancer--;
            } break;
        }

        if (balancer == 0) {
            list_end_index = lexer->current_read_index-1;

            return (struct lexer_token) {
                .type = TOKEN_TYPE_LIST,
                .str  = string_slice(lexer->buffer, list_start_index, list_end_index),
            };
        }
    }

    return NULL_TOKEN;
}

struct lexer_token lexer_try_to_eat_string(struct lexer* lexer) {
    /* we've already eaten the double quote by this point */
    s32 string_start_index = lexer->current_read_index;
    s32 string_end_index   = string_start_index;

    /* allow string escaping */
    while (!lexer_done(lexer)) {
        char eaten = lexer_eat_next_character(lexer);

        if (eaten == '\\') {
            lexer_eat_next_character(lexer);
        } else {
            if (eaten == '\"') {
                string_end_index = lexer->current_read_index-1; 

                return (struct lexer_token) {
                    .type = TOKEN_TYPE_STRING,
                    .str  = string_slice(lexer->buffer, string_start_index, string_end_index),
                };
            }
        }
    }

    return NULL_TOKEN;
}

struct lexer_token lexer_try_to_eat_identifier_symbol_or_number(struct lexer* lexer) {
    s32 identifier_start_index = lexer->current_read_index-1;
    s32 identifier_end_index   = identifier_start_index;

    struct lexer_token result = NULL_TOKEN;
    
    bool found_end_of_token = false;
    while (!lexer_done(lexer) && !found_end_of_token) {
        char eaten = lexer_eat_next_character(lexer);

        if (is_whitespace(eaten)) {
            identifier_end_index = lexer->current_read_index-1;
            found_end_of_token   = true;
        }
    }

    result.str  = string_slice(lexer->buffer, identifier_start_index, identifier_end_index);
    result.type = TOKEN_TYPE_SYMBOL;

    {
        if (string_equal_case_insensitive(result.str, string_literal("t"))) {
            result.type = TOKEN_TYPE_T;
        } else if (string_equal_case_insensitive(result.str, string_literal("nil"))) {
            result.type = TOKEN_TYPE_NIL;
        } else {
            if (is_valid_real_number(result.str)) {
                result.type    = TOKEN_TYPE_NUMBER;
                result.is_real = true;
            } else if (is_valid_integer(result.str)) {
                result.type = TOKEN_TYPE_NUMBER;
            }
        }
    }

    return result;
}
/* 
   NOTE: 
   all the strings here are only valid as long as the filebuffer the lexer is working on is in memory! 
*/
struct lexer_token lexer_next_token(struct lexer* lexer) {
    while (!lexer_done(lexer)) {
        char first = lexer_eat_next_character(lexer);

        if (is_whitespace(first)) {
            continue;
        }

        switch (first) {
            case '#': {
                while (!lexer_done(lexer)) {
                    char eaten = lexer_eat_next_character(lexer);

                    if (eaten == '\n')
                        break;
                }
            } break;
            case ':': {
                return (struct lexer_token) {
                    .type = TOKEN_TYPE_SINGLE_QUOTE,
                    .str  = string_literal(":")
                };
            } break;
            case '\'': {
                return (struct lexer_token) {
                    .type = TOKEN_TYPE_SINGLE_QUOTE,
                    .str  = string_literal("\'")
                };
            } break;
            case '\"': {
                return lexer_try_to_eat_string(lexer);
            } break;
            case '(': {
                return lexer_try_to_eat_list(lexer);
            } break;
            default: {
                return lexer_try_to_eat_identifier_symbol_or_number(lexer);
            } break;
        }
    }

    return NULL_TOKEN;
}

void game_finish_conversation(struct game_state* state) {
    state->is_conversation_active = false;
    file_buffer_free(&state->conversation_file_buffer);
}

void game_open_conversation_file(struct game_state* state, string filename) {
    _debugprintf("opening file: %.*s", filename.length, filename.data);
    /* 
       NOTE:
       However we don't have stack capacity on the arena. It's just double ended linear for now.
       this can technically be marked on the arena as a stack item. 
       
       NOTE:
       keep this in memory until the conversation is done.
    */
    state->conversation_file_buffer = read_entire_file(heap_allocator(), filename);
    /* struct lexer lexer_state = {.buffer = file_buffer_as_string(&state->conversation_file_buffer),}; */
    struct lexer lexer_state = {.buffer = string_literal("hi hi hi this"),};

    while (!lexer_done(&lexer_state)) {
        struct lexer_token token       = lexer_next_token(&lexer_state);
        string             type_string = token_type_strings[token.type];

        _debugprintf("type: (%.*s) value : \"%.*s\"", type_string.length, type_string.data, token.str.length, token.str.data);
    }
    
}
