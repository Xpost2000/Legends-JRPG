/*
  A lisp style form reader, and
  a lexer that is useful for reading lisp but can be used to read some
  other formats. With some modifications anyways.
  
  Lexer implies comments to
  be

  #
*/

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
    string_literal("(t)"),
    string_literal("(nil)"),
    string_literal("(number)"),
    string_literal("(colon)"),
    string_literal("(comma)"),
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

static void _debug_print_token(struct lexer_token token) {
    string type_string = token_type_strings[token.type];
    _debugprintf("type: (%.*s) value : \"%.*s\"", type_string.length, type_string.data, token.str.length, token.str.data);
}


bool lexer_done(struct lexer* lexer) {
    if (lexer->current_read_index <= lexer->buffer.length) {
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
            list_end_index = lexer->current_read_index;

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

bool lexer_token_is_null(struct lexer_token token) {
    return token.type == TOKEN_TYPE_NONE;
}

bool lexer_token_is_symbol_matching(struct lexer_token token, string symbol_value) {
    if (token.type == TOKEN_TYPE_SYMBOL) {
        return string_equal(token.str, symbol_value);
    }

    return false;
}

struct lexer_token lexer_try_to_eat_identifier_symbol_or_number(struct lexer* lexer) {
    s32 identifier_start_index = lexer->current_read_index-1;
    s32 identifier_end_index   = identifier_start_index;

    struct lexer_token result = NULL_TOKEN;
    
    bool found_end_of_token = false;
    while (!lexer_done(lexer) && !found_end_of_token) {
        char eaten = lexer_eat_next_character(lexer);

        /* _debugprintf("eating: \'%c(%d)\'", eaten, eaten); */
        if (is_whitespace(eaten) || (eaten == 0) || lexer_done(lexer)) {
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
            if (is_valid_integer(result.str)) {
                result.type = TOKEN_TYPE_NUMBER;
            } else {
                if (is_valid_real_number(result.str)) {
                    result.type    = TOKEN_TYPE_NUMBER;
                    result.is_real = true;
                }
            }
        }
    }

    if (result.str.length == 0) {
        _debugprintf("empty token");
        return NULL_TOKEN;
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
            case ',': {
                return (struct lexer_token) {
                    .type = TOKEN_TYPE_COMMA,
                    .str  = string_literal(",")
                };
            } break;
            case ':': {
                return (struct lexer_token) {
                    .type = TOKEN_TYPE_COLON,
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

struct lexer_token lexer_peek_token(struct lexer* lexer) {
    s32 current_read_index = lexer->current_read_index;
    struct lexer_token peeked = lexer_next_token(lexer);
    lexer->current_read_index = current_read_index;
    return peeked;
}
struct lisp_form;
enum lisp_form_type {
    LISP_FORM_NIL,
    LISP_FORM_T,
    LISP_FORM_STRING,
    LISP_FORM_SYMBOL,
    LISP_FORM_NUMBER,
    LISP_FORM_QUOTE,
    LISP_FORM_LIST,
    LISP_FORM_COUNT,
};
static string lisp_form_type_strings[] = {
    string_literal("(nil)"),
    string_literal("(t)"),
    string_literal("(string)"),
    string_literal("(symbol)"),
    string_literal("(number)"),
    string_literal("(quote)"),
    string_literal("(list)"),
    string_literal("(count)"),
};
struct lisp_list {
    /* this is a simpler API to build than a real linked list... */
    s32               count;
    struct lisp_form* forms;
};
struct lisp_form {
    s32 type;
    s8  is_real;
    s8  quoted;
    union {
        struct lisp_list  list;
        string            string;
        f32               real;
        s32               integer;
    };
};

#define LISP_t   (struct lisp_form) { .type = LISP_FORM_T, .string = string_literal("t") }
#define LISP_nil (struct lisp_form) { .type = LISP_FORM_NIL, .string = string_literal("nil") }

struct lisp_form lisp_form_real(f32 value) {
    struct lisp_form result = {};
    result.type    = LISP_FORM_NUMBER;
    result.is_real = true;
    result.real    = value;
    return result;
}

struct lisp_form lisp_form_integer(s32 value) {
    struct lisp_form result = {};
    result.type    = LISP_FORM_NUMBER;
    result.integer = value;
    return result;
}

bool lisp_form_get_f32(struct lisp_form form, f32* value) {
    if (form.type == LISP_FORM_NUMBER) {
        if (form.is_real) {
            *value = form.real;
        } else {
            *value = (f32)form.integer;
        }

        return true;
    }

    return false;
}

bool lisp_form_get_s32(struct lisp_form form, s32* value) {
    if (form.type == LISP_FORM_NUMBER) {
        if (form.is_real) {
            *value = (s32)form.real;
        } else {
            *value = form.integer;
        }

        return true;
    }

    return false;
}

bool lisp_form_symbol_matching(struct lisp_form form, string symbol_value) {
    if (form.type == LISP_FORM_SYMBOL) {
        if (string_equal_case_insensitive(form.string, symbol_value)) {
            return true;
        }
    }

    return false;
}

static void _debug_print_out_lisp_code(struct lisp_form* code) {
    switch (code->type) {
        case LISP_FORM_LIST: {
            _debugprintf1("( ");

            struct lisp_list* list_contents = &code->list;
            for (unsigned index = 0; index < list_contents->count; ++index) {
                _debug_print_out_lisp_code(&list_contents->forms[index]);
                _debugprintf1(" ");
            }

            _debugprintf1(" )");
        } break;
        case LISP_FORM_NUMBER: {
            if (code->is_real) {
                _debugprintf1("(real)%f", code->real);
            } else {
                _debugprintf1("(int)%d", code->integer);
            }
        } break;
        case LISP_FORM_STRING: {
            _debugprintf1("\"%.*s\"", code->string.length, code->string.data);
        } break;
        case LISP_FORM_T:
        case LISP_FORM_NIL:
        case LISP_FORM_SYMBOL: {
            char* flavor_text = "";
            if (code->type == LISP_FORM_SYMBOL) {
                if (code->quoted) flavor_text = "[quoted]";
            }
            _debugprintf1("%.*s(%s)", code->string.length, code->string.data, flavor_text);
        } break;
    }
}

/* you can use this to try and read a top level I guess... (reads multiple forms I guess) */
struct lisp_form lisp_read_form(struct memory_arena* arena, string code);
struct lisp_form lisp_read_list(struct memory_arena* arena, string code);

struct lisp_form* lisp_list_get_child(struct lisp_form* form, s32 index) {
    if (form->type == LISP_FORM_LIST) {
        if (index >= 0 && index < form->list.count) {
            return form->list.forms + index;
        }
    }

    return NULL;
}

struct lisp_form lisp_read_list(struct memory_arena* arena, string code) {
    struct lexer lexer_state = {.buffer = code};
    struct lisp_form result = {};

    {
        struct lexer_token first_token = lexer_peek_token(&lexer_state);

        if (lexer_token_is_null(first_token)) {
            result.type   = LISP_FORM_NIL;
            result.string = string_literal("nil");
        } else {
            result.type = LISP_FORM_LIST;
            struct lisp_list* list_contents = &result.list;

            s32 child_count = 0;
            s32 current_read_index = lexer_state.current_read_index;

            /* precount children to preallocate nodes. */
            {
                while (!lexer_done(&lexer_state)) {
                    struct lexer_token next_token = lexer_next_token(&lexer_state);
                    if (!lexer_token_is_null(next_token)) {
                        if (next_token.type != TOKEN_TYPE_SINGLE_QUOTE) {
                            child_count++;
                        }
                    }
                }
                lexer_state.current_read_index = current_read_index;
            }
            _debugprintf("counted : %d children", child_count);

            list_contents->count = child_count;
            list_contents->forms = memory_arena_push(arena, sizeof(*list_contents->forms) * list_contents->count);

            /* grr, I have to special case this a bit... Oh well */
            bool previous_was_quote  = false;
            s32  current_child_index = 0;

            while (!lexer_done(&lexer_state)) {
                struct lisp_form* current_form = list_contents->forms + current_child_index;
                struct lexer_token token = lexer_next_token(&lexer_state);

                if (token.type == TOKEN_TYPE_SINGLE_QUOTE) {
                    previous_was_quote = true;
                } else {
                    _debug_print_token(token);
                    (*current_form) = lisp_read_form(arena, token.str); 

                    if (previous_was_quote) {
                        current_form->quoted = true;
                        previous_was_quote   = false;
                    }

                    current_child_index++;
                }
            }
        }
    }

    return result;
}

/* reads a singular form. */
struct lisp_form lisp_read_form(struct memory_arena* arena, string code) {
    struct lexer lexer_state = {.buffer = code,};
    struct lexer_token first_token = lexer_next_token(&lexer_state);

    struct lisp_form result = {};

    if (first_token.type == TOKEN_TYPE_LIST) {
        string inner_list_string = string_slice(first_token.str, 1, first_token.str.length-1);
        _debugprintf("Inner code \"%.*s\"", inner_list_string.length, inner_list_string.data);
        result                   = lisp_read_list(arena, inner_list_string);
    } else {
        /* read form */
        switch (first_token.type) {
            case TOKEN_TYPE_SYMBOL:
            case TOKEN_TYPE_COLON:
            case TOKEN_TYPE_COMMA: {
                result.type       = LISP_FORM_SYMBOL;
                /* TODO intern these into a table! */
                result.string     = string_clone(arena, first_token.str);
            } break;
            case TOKEN_TYPE_SINGLE_QUOTE: {
                /* result.type       = LISP_FORM_QUOTE; */
                /* result.string     = string_literal("\'"); */
                /* string s = string_slice(code, lexer_state.current_read_index, code.length - lexer_state.current_read_index); */
                /* _debugprintf("%.*s", s.length, s.data); */
                struct lisp_form next_form = lisp_read_form(arena, string_slice(code, lexer_state.current_read_index, (code.length+1) - lexer_state.current_read_index));
                next_form.quoted = true;
                /* _debugprintf("test"); */
                /* _debug_print_out_lisp_code(&next_form); */
                /* _debugprintf("test end"); */
                return next_form;
            } break;
            case TOKEN_TYPE_T: {
                result.type       = LISP_FORM_T;
                result.string     = string_literal("T");
            } break;
            case TOKEN_TYPE_NIL: {
                result.type       = LISP_FORM_NIL;
                result.string     = string_literal("nil");
            } break;
            case TOKEN_TYPE_STRING: {
                result.type       = LISP_FORM_STRING;
                result.string     = string_clone(arena, first_token.str);
            } break;
            case TOKEN_TYPE_NUMBER: {
                result.type       = LISP_FORM_NUMBER;
                result.is_real    = first_token.is_real;

                if (result.is_real) {
                    result.real = string_to_f32(first_token.str);
                } else {
                    result.integer = string_to_s32(first_token.str);
                }
            } break;
        }
    }

    return result;
}
