/*
 * parser.c
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

/**
 * It is important to understand that CIF 2.0 being UTF-8-only makes it a @i binary file type, albeit one that
 * can reliably pass for text on many modern systems.  Standard text parsing tools cannot portably be applied to
 * CIF 2.0 parsing, however, because imlicitly, those are defined in terms of abstract characters.  The encoded
 * forms vary with the code page assumed by the compiler, whereas CIF 2.0's actual encoding does not.
 */

/*
 * The CIF parser implemented herein is akin to a recursive descent parser -- to a predictive parser, in fact --
 * but using an explicit stack in place of most recursion.  It is driven from the scanner side instead of from the
 * more traditional parser side.
 *
 * FIXME: rewrite as a bona fide recursive-descent parser?  Is there an advantage to the manual stack?
 */

#include <stdio.h>
#include <assert.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* For UChar: */
#include <unicode/umachine.h>

#include "cif.h"
/* #include "cif_parser.h" */
#include "internal/utils.h"

#define BUF_SIZE_INITIAL (2 * CIF_LINE_LENGTH + 4)

#define NEXT_CHAR(s) ( /* TODO */ -1 )

/* character class codes */
#define GENERAL_CLASS 0
#define WS_CLASS      1
#define HASH_CLASS    2
#define EOL_CLASS     3
#define UNDERSC_CLASS 4
#define QUOTE_CLASS   5
#define SEMI_CLASS    6
#define OBRAK_CLASS   7
#define CBRAK_CLASS   8
#define OCURL_CLASS   9
#define CCURL_CLASS  10
#define COLON_CLASS  11
#define DOLLAR_CLASS 12
#define A_CLASS      13
#define B_CLASS      14
#define D_CLASS      15
#define E_CLASS      16
#define G_CLASS      17
#define L_CLASS      18
#define O_CLASS      19
#define P_CLASS      20
#define S_CLASS      21
#define T_CLASS      22
#define V_CLASS      23
#define LAST_CLASS   V_CLASS

/* character metaclass codes */
#define GENERAL_META  0
#define WS_META       1

/**
 * @brief a token handler result indicating that the handler accepts the token
 *
 * The handler is expected to remove its own context from the context stack if it is not prepared to accept further
 * tokens.
 */
#define RESULT_ACCEPTED  0

/**
 * @brief a token handler result indicating that the handler rejects the token
 *
 * The handler returning this code must remove its own context from the context stack to avoid receiving
 * any further tokens (if indeed that is its intent).
 */
#define RESULT_REJECTED -1

struct scan_state_s {
    UChar *buffer;
    size_t buffer_size;
    size_t buffer_limit;

    size_t token_offset;
    size_t token_end;
    UChar *next_char;

    size_t line;
    unsigned column;
};

enum context_type {
    CIF = 0, CONTAINER, LOOP, ITEM, LIST, TABLE
};

enum token_type {
    BLOCK_HEAD, FRAME_HEAD, FRAME_TERM, LOOPKW, NAME, OTABLE, CTABLE, OLIST, CLIST, KEY, VALUE, QVALUE, TVALUE,
    VALUE_COMPLETE, UNCERTAIN, END
};

struct parse_context_s;
struct parser_s;

/**
 * @brief handles the next token from the token stream in a manner appropriate to the current parser state
 *
 * @param[in,out] parser a pointer to the parser state, including the current context; normally altered by this function
 * @param[in] ttype the token type of the token
 * @param[in] token_value the token value as a Unicode string, not necessarily NUL-terminated. The caller retains
 *         ownership.
 * @param[in] token_length the number of characters (UChar units) in the token
 */
typedef int (*token_handler_t)(struct parser_s *parser, enum token_type ttype, UChar *token_value,
        int32_t token_length);
typedef void (*context_clean_t)(struct parse_context_s *context);

struct parse_context_s {
    struct parse_context_s *parent;
    enum context_type type;
    /*
     * Use of member 'data' varies by context type, but generally it represents the production being parsed:
     * CIF:       a copy of the handle on the CIF under construction
     * CONTAINER: a handle on the data block or save frame being parsed
     * LOOP:      a handle on the loop being constructed
     * ITEM:      the item's name, as a Unicode string; owned by the context
     * LIST:      the list value being parsed
     * TABLE:     the table value being parsed
     */
    void *data;
    /* A drop box for parsed value objects, especially those computed by child contexts */
    cif_value_t *value;
    /*
     * Use of member 'temp' varies by context type: 
     * CIF:       unused
     * CONTAINER: a handle on the container's scalar loop
     * LOOP:      ?
     * ITEM:      unused
     * LIST:      unused
     * TABLE:     current key
     */
    void *temp;
    /* a flag or counter useful in some contexts */
    int counter;
    token_handler_t handle_token;
    context_clean_t context_clean;
};

struct parser_s {
    int char_class[128];
    int meta_class[LAST_CLASS + 1];
    struct parse_context_s *context;
    struct scan_state_s scan_state;
};

/* static function headers */

/* scanner functions */
static int scan_ws(struct parser_s *parser);
static int scan_to_ws(struct parser_s *parser);
static int scan_to_eol(struct parser_s *parser);
static int scan_delim_string(struct parser_s *parser);
static int scan_text(struct parser_s *parser);

/* token handlers */
static int cif_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value, int32_t token_length);
static int container_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value,
        int32_t token_length);
static int loop_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value, int32_t token_length);
static int item_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value, int32_t token_length);
static int list_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value, int32_t token_length);
static int table_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value,
        int32_t token_length);
static int scalar_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value,
        int32_t token_length);

/* context cleaners */
static void default_context_clean(struct parse_context_s *context);
static void container_context_clean(struct parse_context_s *context);
static void loop_context_clean(struct parse_context_s *context);
static void item_context_clean(struct parse_context_s *context);
static void value_context_clean(struct parse_context_s *context);

/* other functions */
static int parse_text(struct parser_s *parser, UChar *text, int32_t text_length, cif_value_t **dest);

/* static */

/* the standard token handler for each context_type: */
static const token_handler_t token_handlers[6] = {
    cif_handle_token,
    container_handle_token,
    loop_handle_token,
    item_handle_token,
    list_handle_token,
    table_handle_token
};

/* the standard context cleaner for each context_type: */
static const context_clean_t context_cleaners[6] = {
    default_context_clean,
    container_context_clean,
    loop_context_clean,
    item_context_clean,
    value_context_clean,
    value_context_clean
};

static const UChar unknown_value_token[2] = { 0x3f, 0 };

/* Note: numeric character values avoid any compiler codepage dependencies associated with ordinary char literals */
#define LF_CHAR       0x0A
#define CR_CHAR       0x0D
#define DECIMAL_CHAR  0x2E
#define SEMI_CHAR     0x3B
#define QUERY_CHAR    0x3F
#define BKSL_CHAR     0x5C

#define INIT_PARSER(p) do { \
    int i; \
    for (i = 0; i < 128; ) (p).char_class[i] = GENERAL_CLASS; \
    for (i = 0; i < 8; )   (p).meta_class[i] = GENERAL_META; \
    (p).char_class[0x09] =   WS_CLASS; \
    (p).char_class[0x20] =   WS_CLASS; \
    (p).char_class[CR_CHAR] =  EOL_CLASS; \
    (p).char_class[LF_CHAR] =  EOL_CLASS; \
    (p).char_class[0x95] =   UNDERSC_CLASS; \
    (p).char_class[0x22] =   QUOTE_CLASS; \
    (p).char_class[0x24] =   DOLLAR_CLASS; \
    (p).char_class[0x27] =   QUOTE_CLASS; \
    (p).char_class[0x3B] =   SEMI_CLASS; \
    (p).char_class[0x23] =   HASH_CLASS; \
    (p).char_class[0x5B] =   OBRAK_CLASS; \
    (p).char_class[0x5D] =   CBRAK_CLASS; \
    (p).char_class[0x7B] =   OCURL_CLASS; \
    (p).char_class[0x7D] =   CCURL_CLASS; \
    (p).char_class[0x3A] =   COLON_CLASS; \
    (p).char_class[0x41] =   A_CLASS; \
    (p).char_class[0x61] =   A_CLASS; \
    (p).char_class[0x42] =   B_CLASS; \
    (p).char_class[0x62] =   B_CLASS; \
    (p).char_class[0x44] =   D_CLASS; \
    (p).char_class[0x64] =   D_CLASS; \
    (p).char_class[0x45] =   E_CLASS; \
    (p).char_class[0x65] =   E_CLASS; \
    (p).char_class[0x47] =   G_CLASS; \
    (p).char_class[0x67] =   G_CLASS; \
    (p).char_class[0x4C] =   L_CLASS; \
    (p).char_class[0x6C] =   L_CLASS; \
    (p).char_class[0x4F] =   O_CLASS; \
    (p).char_class[0x6F] =   O_CLASS; \
    (p).char_class[0x50] =   P_CLASS; \
    (p).char_class[0x70] =   P_CLASS; \
    (p).char_class[0x53] =   S_CLASS; \
    (p).char_class[0x73] =   S_CLASS; \
    (p).char_class[0x54] =   T_CLASS; \
    (p).char_class[0x74] =   T_CLASS; \
    (p).char_class[0x56] =   V_CLASS; \
    (p).char_class[0x76] =   V_CLASS; \
    (p).meta_class[WS_CLASS] =  WS_META; \
    (p).meta_class[EOL_CLASS] = WS_META; \
    (p).context = (struct parse_context_s *) malloc(sizeof(struct parse_context_s)); \
} while (CIF_FALSE)

#define TOKEN_LENGTH(s) ((s).token_end - (s).token_offset)

/**
 * @brief Determines the class of the specified character based on the specified parser ruleset.
 *
 * @param[in] c the character whose class is requested.  Must be non-negative.  May be evaluated more than once.
 * @param[in] p a struct parser_s containing the rules defining the character's class.
 */
#define CLASS_OF(c, p)  (((c) < 128) ? (p).char_class[c] : GENERAL_CLASS)

#define PUSH_NEW_CONTEXT(t, d, p, ev) do { \
    struct parse_context_s *new_context = (struct parse_context_s *) malloc(sizeof(struct parse_context_s)); \
    if (new_context == NULL) { \
        ev = CIF_ERROR; \
    } else { \
        struct parser_s *_p = (p); \
        enum token_type _t = (t); \
        new_context->parent = _p->context; \
        new_context->type = _t; \
        new_context->data = (d); \
        new_context->value = NULL; \
        new_context->temp = NULL; \
        new_context->counter = 0; \
        new_context->handle_token = token_handlers[_t]; \
        new_context->context_clean = context_cleaners[_t]; \
        _p->context = new_context; \
    } \
} while (CIF_FALSE)

#define POP_CONTEXT(parser) do { \
    struct parser_s *_p = (parser); \
    struct parse_context_s *_c = _p->context; \
    if (_c != NULL) { \
        _p->context = _c->parent; \
        _c->context_clean(_c); \
        if (_c->value != NULL) cif_value_free(_c->value); \
        free(_c); \
    } \
} while (CIF_FALSE)

int cif_parse_internal(FILE *source, cif_t *dest) {
    FAILURE_HANDLING;
    struct parser_s parser;

    INIT_PARSER(parser);
    if (parser.context != NULL) {
        parser.context->type = CIF;
        parser.context->data = dest;
        parser.scan_state.buffer = (UChar *) malloc(BUF_SIZE_INITIAL * sizeof(UChar));
        if (parser.scan_state.buffer != NULL) {
            /* TODO: really ought to use read(2) where that is available */
            parser.scan_state.buffer_size = BUF_SIZE_INITIAL;
            parser.scan_state.buffer_limit = /* XXX: fread(parser.scan_state.buffer, 1, buf_size, source) */ 0;
            parser.scan_state.token_offset = 0;
            parser.scan_state.token_end = 0;
            parser.scan_state.next_char = parser.scan_state.buffer;
            parser.scan_state.line = 1;
            parser.scan_state.column = 0;

            /* TODO: handle BOM, if any */
            /* TODO: handle CIF version comment, if any */

            do {
                int32_t c = NEXT_CHAR(parser.scan_state);  /* consumes the next char but does not advance the token start position */
                int scan_result;
                enum token_type ttype;
                ssize_t token_length;

                if (c <= 0) {
                    /* TODO: handle EOF */
                    break;
                }
                switch (CLASS_OF(c, parser)) {
                    case WS_CLASS:
                        scan_result = scan_ws(&parser);
                        /* FIXME: report to listener, if any */
                        continue;
                    case HASH_CLASS:
                        scan_result = scan_to_eol(&parser);
                        /* FIXME: report to listener, if any */
                        continue;
                    case EOL_CLASS:
                        /* TODO: parse newline */
                        continue;
                    case UNDERSC_CLASS:
                        ttype = NAME;
                        scan_result = scan_to_ws(&parser);
                        parser.scan_state.token_offset += 1;
                        break;
                    case QUOTE_CLASS:
                        ttype = QVALUE;
                        scan_result = scan_delim_string(&parser);
                        parser.scan_state.token_offset += 1;
                        break;
                    case OBRAK_CLASS:
                        ttype = OLIST;
                        parser.scan_state.token_end = parser.scan_state.token_offset;
                        break;
                    case CBRAK_CLASS:
                        ttype = CLIST;
                        parser.scan_state.token_end = parser.scan_state.token_offset;
                        break;
                    case OCURL_CLASS:
                        ttype = OTABLE;
                        parser.scan_state.token_end = parser.scan_state.token_offset;
                        break;
                    case CCURL_CLASS:
                        ttype = CTABLE;
                        parser.scan_state.token_end = parser.scan_state.token_offset;
                        break;
                    case SEMI_CLASS:
                        if (parser.scan_state.column == 1) {  /* semicolon was in column 1 */
                            scan_result = scan_text(&parser);
                            parser.scan_state.token_offset += 1;
                        } else {
                            scan_result = scan_to_ws(&parser);
                        }
                        ttype = TVALUE;
                        break;
                    /* TODO: handle COLON_CLASS when necessary */
                    case DOLLAR_CLASS:
                        /* TODO: error frame reference */
                        /* recover by accepting it as a ws-delimited string */
                    default:
                        scan_result = scan_to_ws(&parser);
                        ttype = UNCERTAIN;
                        break;
                }

                if (scan_result == CIF_OK) {
                    UChar *token = parser.scan_state.buffer + parser.scan_state.token_offset;
                    int result;

                    token_length = TOKEN_LENGTH(parser.scan_state);
                    if (ttype == UNCERTAIN) {
                        ttype = VALUE; /* this is the most likely token type */

                        /* messy, case-insensitive, codepage-independent detection of reserved words */
                        if ((token_length > 4) && (CLASS_OF(*(token + 4), parser) == UNDERSC_CLASS)) {
                            if (             (CLASS_OF(*token, parser) == D_CLASS)
                                    && (CLASS_OF(*(token + 1), parser) == A_CLASS)
                                    && (CLASS_OF(*(token + 2), parser) == T_CLASS)
                                    && (CLASS_OF(*(token + 3), parser) == A_CLASS)) {
                                if (token_length == 5) {
                                    /* TODO: error missing block code / reserved word */
                                    /* recover by dropping it */
                                    goto reset_token;
                                } else {
                                    ttype = BLOCK_HEAD;
                                    parser.scan_state.token_offset += 5;
                                    token_length -= 5;
                                }
                            } else if (      (CLASS_OF(*token, parser) == S_CLASS)
                                    && (CLASS_OF(*(token + 1), parser) == A_CLASS)
                                    && (CLASS_OF(*(token + 2), parser) == V_CLASS)
                                    && (CLASS_OF(*(token + 3), parser) == E_CLASS)) {
                                ttype = ((token_length == 5) ? FRAME_TERM : FRAME_HEAD);
                                parser.scan_state.token_offset += 5;
                                token_length -= 5;
                            } else if ((token_length == 5)
                                    && (CLASS_OF(*(token + 2), parser) == O_CLASS)
                                    && (CLASS_OF(*(token + 3), parser) == P_CLASS)) {
                                if (             (CLASS_OF(*token, parser) == L_CLASS)
                                        && (CLASS_OF(*(token + 1), parser) == O_CLASS)) {
                                    ttype = LOOPKW;
                                    parser.scan_state.token_offset += 5;
                                    token_length -= 5;
                                } else if (      (CLASS_OF(*token, parser) == S_CLASS)
                                        && (CLASS_OF(*(token + 1), parser) == T_CLASS)) {
                                    /* TODO: error 'stop' reserved word */
                                    /* recover by dropping it */
                                    goto reset_token;
                                } 
                            }
                        } else if ((token_length == 7) && (CLASS_OF(*(token + 6), parser) == UNDERSC_CLASS)
                                &&       (CLASS_OF(*token, parser) == G_CLASS)
                                && (CLASS_OF(*(token + 1), parser) == L_CLASS)
                                && (CLASS_OF(*(token + 2), parser) == O_CLASS)
                                && (CLASS_OF(*(token + 3), parser) == B_CLASS)
                                && (CLASS_OF(*(token + 4), parser) == A_CLASS)
                                && (CLASS_OF(*(token + 5), parser) == L_CLASS)) {
                            /* TODO: error 'global' reserved word */
                            /* recover by ignoring it */
                            goto reset_token;
                        } else {
                            /* TODO: validate contents do not contain square brackets or curly braces */
                            /* recover by accepting the value */
                        }
                    }

                    /* dispatch the token to current-context token handler */
                    do {
                        assert (parser.context != NULL);
                        result = parser.context->handle_token(&parser, ttype, token, token_length);
                    } while (result == RESULT_REJECTED);
                    if (result != RESULT_ACCEPTED) {
                        /* the token was neither accepted nor rejected; the result must be an error code */
                        SET_RESULT(result);
                        break;
                    }

                    reset_token:
                    /* FIXME: verify this: */
                    parser.scan_state.token_offset = parser.scan_state.next_char - parser.scan_state.buffer;
                } else {
                    /* TODO: error handling */
                    break;
                }

            } while (CIF_TRUE);

            free(parser.scan_state.buffer);
        }

        /* Clean up any remaining parse contexts */
        while (parser.context != NULL) {
            POP_CONTEXT(&parser);
        }
    }

    FAILURE_TERMINUS;
}

/* TODO: macro GET_MORE_CHARS(state); must account for EOF / ferror */
#define GET_MORE_CHARS(state) 0

#define TOKEN_SIZE(state) ((state).next_char - ((state).buffer + (state).token_offset))

static int scan_ws(struct parser_s *parser) {
    do {
        UChar *top = parser->scan_state.buffer + parser->scan_state.buffer_limit;

        while (parser->scan_state.next_char < top) {
            UChar c = *(parser->scan_state.next_char++);

            if (CLASS_OF(c, *parser) != WS_CLASS) {
                goto end_of_token;
            }
        }
    } while (GET_MORE_CHARS(state) > 0);

    end_of_token:
    parser->scan_state.token_end = (parser->scan_state.next_char - parser->scan_state.buffer);
    return CIF_OK;
}

static int scan_to_ws(struct parser_s *parser) {
    do {
        UChar *top = parser->scan_state.buffer + parser->scan_state.buffer_limit;

        while (parser->scan_state.next_char < top) {
            UChar c = *(parser->scan_state.next_char++);

            if (parser->meta_class[CLASS_OF(c, *parser)] == WS_META) {
                goto end_of_token;
            }
        }
    } while (GET_MORE_CHARS(state) > 0);

    end_of_token:
    parser->scan_state.token_end = (parser->scan_state.next_char - parser->scan_state.buffer);
    return CIF_OK;
}

static int scan_to_eol(struct parser_s *parser) {
    do {
        UChar *top = parser->scan_state.buffer + parser->scan_state.buffer_limit;

        while (parser->scan_state.next_char < top) {
            UChar c = *(parser->scan_state.next_char++);

            if (CLASS_OF(c, *parser) == EOL_CLASS) {
                goto end_of_token;
            } else {
                parser->scan_state.next_char += 1;
            }
        }
    } while (GET_MORE_CHARS(state) > 0);

    end_of_token:
    parser->scan_state.token_end = (parser->scan_state.next_char - parser->scan_state.buffer);
    return CIF_OK;
}

static int scan_delim_string(struct parser_s *parser) {
    UChar delim = *(parser->scan_state.buffer + parser->scan_state.token_offset);
    int backup = 0;

    do {
        UChar *top = parser->scan_state.buffer + parser->scan_state.buffer_limit;

        while (parser->scan_state.next_char < top) {
            UChar c = *(parser->scan_state.next_char++);

            if (c == delim) {
                backup = 1;
                goto end_of_token;
            } else if (CLASS_OF(c, *parser) == EOL_CLASS) {
                /* handle error: unterminated quoted string */
                /* recover by assuming a trailing close-quote */
                parser->scan_state.next_char -= 1;
                goto end_of_token;
            }
        }
    } while (GET_MORE_CHARS(state) > 0);

    end_of_token:
    parser->scan_state.token_end = (parser->scan_state.next_char - parser->scan_state.buffer) - backup;
    return CIF_OK;
}

static int scan_text(struct parser_s *parser) {
    int backup = 0;

    do {
        UChar *top = parser->scan_state.buffer + parser->scan_state.buffer_limit;

        while (parser->scan_state.next_char < top) {
            UChar c = *(parser->scan_state.next_char++);
            int sol = CIF_FALSE;

            if ((CLASS_OF(c, *parser) == SEMI_CLASS) && sol) {
                backup = ((*(parser->scan_state.next_char - 1) == LF_CHAR)
                        && (*(parser->scan_state.next_char - 2) == CR_CHAR)) ? 3 : 2;
                goto end_of_token;
            } else if (parser->char_class[c] == EOL_CLASS) {
                /* the next character is at the start of a line */
                sol = CIF_TRUE;
                parser->scan_state.line += 1;
            } else {
                sol = CIF_FALSE;
            }
        }
    } while (GET_MORE_CHARS(state) > 0);

    /* TODO: handle unterminated block at EOF */
    end_of_token:
    parser->scan_state.token_end = (parser->scan_state.next_char - parser->scan_state.buffer) - backup;
    return CIF_OK;
}

/* a context cleaning function that does nothing */
static void default_context_clean(struct parse_context_s *context) {
    /* empty */
}

static void container_context_clean(struct parse_context_s *context) {
    cif_loop_free((cif_loop_t *) context->temp);            /* ignore any error */
    cif_container_free((cif_container_t *) context->data);  /* ignore any error */
}

static void loop_context_clean(struct parse_context_s *context) {
    /* TODO */
}

static void item_context_clean(struct parse_context_s *context) {
    free(context->data);
}

static void value_context_clean(struct parse_context_s *context) {
    cif_value_free((cif_value_t *) context->data);  /* ignore any error */
    if ((context->type == TABLE) && (context->temp != NULL)) {
        context->value = NULL;  /* any item value recorded belongs to the table value that was just freed */
    }
}

static int cif_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value, int32_t token_length) {
    cif_t *cif = (cif_t *) parser->context->data;
    cif_block_t *block = NULL;
    int result;
    UChar saved;

    switch (ttype) {
        case BLOCK_HEAD:
            /* insert a string terminator into the input buffer, after the current token */
            saved = *(token_value + token_length);
            *(token_value + token_length) = 0;

            result = cif_create_block(cif, token_value, &block);
            switch (result) {
                case CIF_INVALID_BLOCKCODE:
                    /* TODO: report syntax error */
                    /* recover by using the block code anyway */
                    if ((result = (cif_create_block_internal(cif, token_value, 1, &block))) != CIF_DUP_BLOCKCODE) {
                        break;
                    }
                    /* else result == CIF_DUP_BLOCKCODE, so fall through */
                case CIF_DUP_BLOCKCODE:
                    /* TODO: report data error */
                    /* recover by using the existing block */
                    result = cif_get_block(cif, token_value, &block);
                    break;
                /* default: do nothing */
            }

            /* restore the next character in the input buffer */
            *(token_value + token_length) = saved;

            if (result == CIF_OK) {
                result = RESULT_ACCEPTED;
                /* TODO: call block start handler, if any */
                PUSH_NEW_CONTEXT(CONTAINER, block, parser, result);
            }

            break;
        case END:
            /* do nothing */
            result = RESULT_ACCEPTED; /* FIXME: is anything else required at EOF? */
            break;
        case KEY:
        case VALUE_COMPLETE:
            /* should not happen */
            result = CIF_INTERNAL_ERROR;
            break;
        default:
            /* TODO: error missing data block header */
            /* recover by creating and installing an anonymous block context to eat up the content */
            if ((result = cif_create_block_internal(cif, &cif_uchar_nul, 1, &block)) == CIF_DUP_BLOCKCODE) {
                result = cif_get_block(cif, &cif_uchar_nul, &block);
            }

            if (result == CIF_OK) {
                PUSH_NEW_CONTEXT(CONTAINER, block, parser, result);
                if (result == CIF_OK) {
                    /* forward the token to the synthetic context */
                    result = parser->context->handle_token(parser, ttype, token_value, token_length);
                }
            }
            break;
    }

    /* this handler never pops the context */

    return result;
}

static int container_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value,
        int32_t token_length) {
    cif_container_t *container = (cif_container_t *) parser->context->data;
    int result;

    switch (ttype) {
        case BLOCK_HEAD:
            if (cif_container_assert_block(container) != CIF_OK) {
                /* TODO: error: unterminated save frame */
                /* recover by continuing */
            }
            /* reject the token */
            result = RESULT_REJECTED;
            break;
        case FRAME_HEAD:
            if (cif_container_assert_block(container) != CIF_OK) {
                /* TODO: error: unterminated save frame */
                /* recover by rejecting the token */
                result = RESULT_REJECTED;
            } else {
                cif_container_t *frame = NULL;
                UChar saved;

                /* insert a string terminator into the input buffer, after the current token */
                saved = *(token_value + token_length);
                *(token_value + token_length) = 0;

                result = cif_block_create_frame(container, token_value, &frame);
                switch (result) {
                    case CIF_INVALID_FRAMECODE:
                        /* TODO: report syntax error */
                        /* recover by using the frame code anyway */
                        if ((result = (cif_block_create_frame_internal(container, token_value, 1, &frame))) != CIF_DUP_FRAMECODE) {
                            break;
                        }
                        /* else result == CIF_DUP_BLOCKCODE, so fall through */
                    case CIF_DUP_FRAMECODE:
                        /* TODO: report data error */
                        /* recover by using the existing frame */
                        result = cif_block_get_frame(container, token_value, &frame);
                        break;
                    /* default: do nothing */
                }

                /* restore the next character in the input buffer */
                *(token_value + token_length) = saved;

                if (result == CIF_OK) {
                    result = RESULT_ACCEPTED;
                    /* TODO: call frame start handler, if any */
                    PUSH_NEW_CONTEXT(CONTAINER, frame, parser, result);
                }
            }
            break;
        case FRAME_TERM:
            if (cif_container_assert_block(container) == CIF_OK) {
                /* TODO: error unexpected frame terminator */
                /* recover by dropping the token */
            } else {
                /* close the context */
                POP_CONTEXT(parser);
            }
            result = RESULT_ACCEPTED;
            break;
        case LOOPKW:
            result = RESULT_ACCEPTED;
            PUSH_NEW_CONTEXT(LOOP, NULL, parser, result);
            break;
        case NAME:
            result = CIF_OK;
            PUSH_NEW_CONTEXT(ITEM, NULL, parser, result);
            if (result == CIF_OK) {
                /* parser->context is now an item context */
                parser->context->data = malloc((token_length + 1) * sizeof(UChar));
                if (parser->context->data == NULL) {
                    result = CIF_ERROR;
                } else {
                    /* copy the data name to a separate Unicode string in the item context */
                    u_strncpy((UChar *) parser->context->data, token_value, token_length);
                    ((UChar *)parser->context->data)[token_length] = 0;

                    /* check for dupes */
                    result = cif_container_get_item_loop(container, (UChar *) parser->context->data, NULL);
                    if (result == CIF_OK) {
                        /* TODO: error duplicate name */
                        /* recover by rejecting the item (but still parsing the associated value) */
                        free(parser->context->data);
                        parser->context->data = NULL;
                        result = RESULT_ACCEPTED;
                    } else if (result == CIF_NOSUCH_ITEM) {
                        result = RESULT_ACCEPTED;
                    }
                }
            }
            break;
        case TVALUE:
        case QVALUE:
        case VALUE:
        case OLIST: /* opening delimiter of a list value */
        case OTABLE: /* opening delimiter of a table value */
            /* TODO: error unexpected value */
            /* create and install an item context (with no data name) to handle the value; the value is discarded */
            PUSH_NEW_CONTEXT(ITEM, NULL, parser, result);
            if (result == CIF_OK) {
                /* forward the token to the item context */
                result = parser->context->handle_token(parser, ttype, token_value, token_length);
                /* the item context will pop itself after handling the value, no matter what the result */

                /* An item context should never reject a value */
                if (result == RESULT_REJECTED) {
                    result = CIF_INTERNAL_ERROR;
                }
            }
            break;
        case CTABLE:
        case CLIST:
            /* TODO: error unexpected closing delimiter */
            /* recover by dropping it */
            result = RESULT_ACCEPTED;
            break;
        case END:
            if (cif_container_assert_block(container) != CIF_OK) {
                /* TODO: error unterminated save frame */
                /* recover by rejecting the token (as would happen anyway) */
            }
            /* no special action, just reject the token */
            result = RESULT_REJECTED;
            break;
        default: /* including at least KEY, UNCERTAIN, and VALUE_COMPLETE */
            /* should not happen */
            result = CIF_INTERNAL_ERROR;
            break;
    }

    if (result != RESULT_ACCEPTED) {
        POP_CONTEXT(parser);
    }

    return result;
}

static int item_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value,
        int32_t token_length) {
    cif_value_t *value = NULL;
    int result;

    switch (ttype) {
        case OLIST: /* opening delimiter of a list value */
            /* create and install list context to handle the list; an error will be reported after the it is parsed */
            if ((result = cif_value_create(CIF_LIST_KIND, &value)) == CIF_OK) {
                result = RESULT_ACCEPTED;
                parser->context->value = value;
                PUSH_NEW_CONTEXT(LIST, value, parser, result);
            }
            break;
        case OTABLE: /* opening delimiter of a table value */
            /* create and install table context to handle the table; an error will be reported after the it is parsed */
            if ((result = cif_value_create(CIF_TABLE_KIND, &value)) == CIF_OK) {
                result = RESULT_ACCEPTED;
                parser->context->value = value;
                PUSH_NEW_CONTEXT(TABLE, value, parser, result);
            }
            break;
        case TVALUE:
        case QVALUE:
        case VALUE:
            if ((result = scalar_handle_token(parser, ttype, token_value, token_length)) != CIF_OK) {
                break;
            }
            /* fall through */
        case VALUE_COMPLETE:
            /* The parsed value object should be available in the context */
            assert(parser->context->value != NULL);

            /* the context datum is the target item name, or null if the value is to be discarded */
            if (parser->context->data != NULL) {
                /* a non-null data name implies that it is safe and appropriate to add the item as a scalar */
                assert(parser->context->parent != NULL);
                assert(parser->context->parent->type == CONTAINER);
                if ((result = cif_container_set_value((cif_container_t *) parser->context->parent->data,
                        (UChar *) parser->context->data,
                        parser->context->value)) != CIF_OK) {
                    break;
                }
            }

            /* this context will accept no more tokens, so pop it */
            POP_CONTEXT(parser);
            result = RESULT_ACCEPTED;
            break;
        case KEY:
        case UNCERTAIN:
            result = CIF_INTERNAL_ERROR;
            break;
        default:
            /* TODO: error missing value */
            /* recover by inserting a synthetic unknown value */
            assert(parser->context->value == NULL);
            cif_value_create(CIF_UNK_KIND, &parser->context->value);
            if ((result = item_handle_token(parser, VALUE_COMPLETE, NULL, 0)) == RESULT_ACCEPTED) {
                /* return directly to avoid popping an extra context */
                return RESULT_REJECTED;
            } else {
                /* return directly to avoid popping an extra context */
                return result;
            }
    }

    if (result != RESULT_ACCEPTED) {
        POP_CONTEXT(parser);
    }

    return result;
}

static int loop_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value,
        int32_t token_length) {
    cif_value_t *value = NULL;
    int result;

    switch (ttype) {
        case BLOCK_HEAD:
        case FRAME_HEAD:
        case FRAME_TERM:
        case LOOPKW:
        case NAME:
            /* TODO: end-of-loop handling */
            /* call handler, if any */
            /* close this context, rejecting the token */
            result = RESULT_REJECTED;
            break;
        case OLIST: /* opening delimiter of a list value */
            /* TODO: error if empty header; recover by dropping this context */
            /* create and install list context to handle the list; an error will be reported after the it is parsed */
            if ((result = cif_value_create(CIF_LIST_KIND, &value)) == CIF_OK) {
                result = RESULT_ACCEPTED;
                parser->context->value = value;
                PUSH_NEW_CONTEXT(LIST, value, parser, result);
            }
            break;
        case OTABLE: /* opening delimiter of a table value */
            /* TODO: error if empty header; recover by dropping this context */
            /* create and install table context to handle the table; an error will be reported after the it is parsed */
            if ((result = cif_value_create(CIF_TABLE_KIND, &value)) == CIF_OK) {
                result = RESULT_ACCEPTED;
                parser->context->value = value;
                PUSH_NEW_CONTEXT(TABLE, value, parser, result);
            }
            break;
        case TVALUE:
        case QVALUE:
        case VALUE:
            /* TODO: error if empty header; recover by dropping this context */
            /* else add the value to the current packet, in header order */
            break;
        case VALUE_COMPLETE:
            /* TODO */
            break;
        case CTABLE:
        case CLIST:
            /* TODO: error unexpected closing delimiter */
            /* recover by dropping it */
            result = RESULT_ACCEPTED;
            break;
        case END:
            /* TODO: end-of-loop handling */
            result = RESULT_REJECTED;
            break;
        default: /* including KEY and UNCERTAIN */
            result = CIF_INTERNAL_ERROR;
            break;
    }

    if (result != RESULT_ACCEPTED) {
        POP_CONTEXT(parser);
    }

    return result;
}

static int list_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value,
        int32_t token_length) {
    cif_value_t *list = (cif_value_t *) parser->context->data;
    cif_value_t *element = NULL;
    int result;

    switch (ttype) {
        case OLIST: /* opening delimiter of a list value */
            /* create and install list context to handle the list; an error will be reported after the it is parsed */
            if ((result = cif_value_create(CIF_LIST_KIND, &(parser->context->value))) == CIF_OK) {
                result = RESULT_ACCEPTED;
                PUSH_NEW_CONTEXT(LIST, parser->context->value, parser, result);
            }
            break;
        case OTABLE: /* opening delimiter of a table value */
            /* create and install table context to handle the table; an error will be reported after the it is parsed */
            if ((result = cif_value_create(CIF_TABLE_KIND, &(parser->context->value))) == CIF_OK) {
                result = RESULT_ACCEPTED;
                PUSH_NEW_CONTEXT(TABLE, parser->context->value, parser, result);
            }
            break;
        case TVALUE:
        case QVALUE:
        case VALUE:
            if ((result = scalar_handle_token(parser, ttype, token_value, token_length)) != CIF_OK) {
                break;
            }
            /* fall through */
        case VALUE_COMPLETE:
            /* pick up the element to add from the context */
            element = parser->context->value;
            result = ((element == NULL) ? CIF_INTERNAL_ERROR : RESULT_ACCEPTED);
            break;
        case KEY:
        case UNCERTAIN:
            /* should not happen */
            result = CIF_INTERNAL_ERROR;
            break;
        default:
            /* TODO: error unterminated list */
            /* recover by synthetically closing the list and rejecting the token */
            /* fall through */
        case CLIST:
            /* Pop this list context, as it is terminated by the offered CLIST token */
            POP_CONTEXT(parser);

            /* Notify the parent context (now top on the parser's stack) that the list is fully parsed */
            result = parser->context->handle_token(parser, VALUE_COMPLETE, NULL, 0);

            /*
             * Return directly to avoid double-popping.  The parent is expected to not reject the synthetic
             * "value complete" token, but we do reject the token presented to this context unless it is CLIST
             * (accepted), or we need to report an error.
             */
            return ((result == RESULT_ACCEPTED) ? ((ttype == CLIST) ? RESULT_ACCEPTED : RESULT_REJECTED) : result);
    }

    if (element != NULL) {
        /* a new element has been offered to the context's list */
        size_t size = 0;
        int list_result = cif_value_get_element_count(list, &size);

        if (list_result == CIF_ARGUMENT_ERROR) {
            /* should not happen because 'list' should refer to a value of CIF_LIST_KIND */
            result = CIF_INTERNAL_ERROR;
        } else if (list_result != CIF_OK) {
            result = list_result;
        } else {
            /* add the offered element to the list */
            /* FIXME: better in this context to assign values into the list than to copy them in */
            switch ((list_result = cif_value_insert_element_at(list, size, element))) {
                case CIF_ARGUMENT_ERROR:
                case CIF_INVALID_INDEX:
                    /* should not happen */
                    result = CIF_INTERNAL_ERROR;
                    break;
                case CIF_OK:
                    /* do nothing */
                    /* Note: the value has been cloned into a list element, not assigned directly to the list */
                    break;
                default:
                    result = list_result;
                    break;
            }
        }

        /* clean up the offered element no matter what */
        cif_value_free(element);  /* ignore any error */
        parser->context->value = NULL;
    }

    if (result != RESULT_ACCEPTED) {
        POP_CONTEXT(parser);
    }

    return result;
}

static int table_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value,
        int32_t token_length) {
    UChar *key;
    cif_value_t *table;
    int result;

    switch (ttype) {
        case KEY:
            if (parser->context->temp != NULL) {
                /* TODO: error missing value */
                /* recover by assuming an unknown value (already set), and accepting the new key */
                parser->context->temp = NULL;
                parser->context->value = NULL;
            }

            /* check for key duplication and copy the key into this context */
            key = (UChar *) malloc((token_length + 1) * sizeof(UChar));
            if (key == NULL) {
                result = CIF_ERROR;
            } else {
                u_strncpy(key, token_value, token_length);
                key[token_length] = 0;

                /* check for a duplicate key */
                table = (cif_value_t *) parser->context->data;
                result = cif_value_get_item_by_key(table, key, NULL);
                if (result == CIF_NOSUCH_ITEM) {
                    /* the expected result */

                    /* record the key, and enroll it in the table with an unknown value */
                    parser->context->temp = key;
                    if (((result = cif_value_set_item_by_key(table, key, NULL)) == CIF_OK)
                            /* Record a pointer to the item value in the context.  The result is not a copy. */
                            && ((result = cif_value_get_item_by_key(table, key, &parser->context->value)) == CIF_OK)) {
                        result = RESULT_ACCEPTED;
                    }
                } else if (result == CIF_OK) {
                    /* TODO: error duplicate key */
                    /* recover by parsing and discarding the incoming value */
                    assert(parser->context->temp == NULL);
                    if ((result = cif_value_create(CIF_UNK_KIND, &parser->context->value)) == CIF_OK) {
                        result = RESULT_ACCEPTED;  /* The context owns the value */
                    }
                } /* else drop through */
                /* TODO: handle CIF_INVALID_INDEX (invalid key) */
            }

            break;
        case OLIST: /* opening delimiter of a list value */
            /* missing key will be signaled, if needed, after the list is parsed (?) */
            /* create and install list context to handle the list */
            if ((result = ((parser->context->value == NULL)
                    ? cif_value_create(CIF_LIST_KIND, &(parser->context->value))
                    : cif_value_init(parser->context->value, CIF_LIST_KIND))) == CIF_OK) {
                result = RESULT_ACCEPTED;
                PUSH_NEW_CONTEXT(LIST, parser->context->value, parser, result);
            }
            break;
        case OTABLE: /* opening delimiter of a table value */
            /* missing key will be signaled, if needed, after the list is parsed (?) */
            /* create and install table context to handle the table; an error will be reported after the it is parsed */
            if ((result = ((parser->context->value == NULL)
                    ? cif_value_create(CIF_TABLE_KIND, &(parser->context->value))
                    : cif_value_init(parser->context->value, CIF_TABLE_KIND))) == CIF_OK) {
                result = RESULT_ACCEPTED;
                PUSH_NEW_CONTEXT(TABLE, parser->context->value, parser, result);
            }
            break;
        case QVALUE:
        case TVALUE:
        case VALUE:
            /* TODO: make sure the scalar handler modifies this context's existing value instead of making a new one */
            if ((result = scalar_handle_token(parser, ttype, token_value, token_length)) != CIF_OK) {
                break;
            }
            /* fall through */
        case VALUE_COMPLETE:
            assert(parser->context->value != NULL);
            if (parser->context->temp == NULL) {
                /* TODO: error missing key */
                /* recovery by accepting and dropping the value is implicit, except for cleaning up the value */
                cif_value_free(parser->context->value);
            } else {
                /* consume the key recorded in the current context */
                free(parser->context->temp);
                parser->context->temp = NULL;
            }

            parser->context->value = NULL;
            result = RESULT_ACCEPTED;
            break;
        case UNCERTAIN:
            result = CIF_INTERNAL_ERROR;
            break;
        default:
            /* TODO: error unterminated table */
            /* recover by ending the table; the token will be rejected */
            /* fall through */
        case CTABLE:
            /* the end of this context, one way or another */
            if (parser->context->temp != NULL) {
                /* TODO: error missing value */
                /* recover by assuming an unknown value (already set) */
            }

            /* Notify the parent context that the table is fully parsed (as much as it can be) */
            POP_CONTEXT(parser);  /* remove this table context to expose its parent */
            result = parser->context->handle_token(parser, VALUE_COMPLETE, NULL, 0);

            /*
             * Return directly to avoid double-popping.  The parent is expected to not reject the synthetic
             * "value complete" token, but we do reject the token presented to this context unless it is CTABLE
             * (accepted), or we need to report an error.
             */
            return ((result == RESULT_REJECTED)
                    ? CIF_INTERNAL_ERROR
                    : ((ttype == CTABLE) ? result : RESULT_REJECTED));
    }

    if (result != RESULT_ACCEPTED) {
        POP_CONTEXT(parser);
    }

    return result;
}

/*
 * Handles a scalar value token by parsing it into a value object recorded in the current context.
 * Though structured as a token handler, it operates as a helper function for other handlers.  As such, it returns
 * CIF_OK when successful, and a standard CIF error code on failure.  In particular, it returns CIF_INTERNAL_ERROR if
 * the specified token type is anything other than TVALUE, QVALUE, or VALUE.
 *
 * Does not emit parse errors.
 */
static int scalar_handle_token(struct parser_s *parser, enum token_type ttype, UChar *token_value,
        int32_t token_length) {
    int check_numeric = CIF_FALSE;
    cif_value_t *value = NULL;
    int result;

    switch (ttype) {
        case TVALUE:
            /* parse text block contents into a value */
            return parse_text(parser, token_value, token_length, &(parser->context->value));
        case VALUE:
            /* special cases for unquoted question mark (?) and period (.) */
            if (token_length == 1) {
                if (*token_value == QUERY_CHAR) {
                    return (parser->context->value != NULL) ? cif_value_init(parser->context->value, CIF_UNK_KIND)
                            : cif_value_create(CIF_UNK_KIND, &(parser->context->value));
                }  else if (*token_value == DECIMAL_CHAR) {
                    return (parser->context->value != NULL) ? cif_value_init(parser->context->value, CIF_NA_KIND)
                            : cif_value_create(CIF_NA_KIND, &(parser->context->value));
                } /* else an ordinary character value */
            }

            check_numeric = CIF_TRUE;
            /* fall through */
        case QVALUE:
            /* build a value object, or modify the provided one */
            if ((result = ((parser->context->value != NULL)
                    ? ((value = parser->context->value), cif_value_init(value, CIF_UNK_KIND))
                    : cif_value_create(CIF_UNK_KIND, &value))) == CIF_OK) {
                UChar temp = token_value[token_length];

                token_value[token_length] = 0;
                result = (check_numeric ? cif_value_parse_numb(value, token_value) : CIF_INVALID_NUMBER);
                if (result == CIF_INVALID_NUMBER) {
                    result = cif_value_copy_char(value, token_value);
                }
                if (result == CIF_OK) {
                    parser->context->value = value;
                } else if (value != parser->context->value) {
                    cif_value_free(value); /* ignore any error */
                }
                token_value[token_length] = temp;

                return result;
            }
            break;
        default:
            return CIF_INTERNAL_ERROR;
    }

    /* never pops the context */
}

/*
 * Parses the contents of a text block, un-prefixing and unfolding lines if appropriate
 *
 * If dest points to a non-NULL value handle then the associated value object is modified
 * (on success) to be of kind CIF_CHAR_KIND, holding the parsed text.  Otherwise, a new value
 * object is created and its handle recorded where dest points.
 */
static int parse_text(struct parser_s *parser, UChar *text, int32_t text_length, cif_value_t **dest) {
    FAILURE_HANDLING;
    cif_value_t *value = *dest;
    int result = (value == NULL) ? cif_value_create(CIF_UNK_KIND, &value) : CIF_OK;

    if (result != CIF_OK) {
        /* failed to create a value object */
        return result;
    } else {
        if (text_length <= 0) {
            /* update the value object with an empty char value */
            if ((result = cif_value_init(value, CIF_CHAR_KIND)) != CIF_OK) {
                FAIL(early, CIF_ERROR);
            } else {
                *dest = value;
                return CIF_OK;
            }
        } else {
            UChar *buffer = (UChar *) malloc((text_length + 1) * sizeof(UChar));

            if (buffer != NULL) {
                /* analyze the text and copy its logical contents into the buffer */
                UChar *buf_pos = buffer;
                UChar *in_pos = text;
                UChar *in_limit = text + text_length; /* input boundary pointer */
                int32_t prefix_length;
                int folded;

                if (*text == SEMI_CHAR) { /* text beginning with a semicolon is neither prefixed nor folded */
                    prefix_length = 0;
                    folded = CIF_FALSE;
                } else {
                    /* check for text prefixing and/or line-folding */
                    int backslash_count = 0;
                    int nonws = CIF_FALSE;  /* whether there is any non-whitespace between the last backslash and EOL */

                    /* scan the first line of the text for a prefix and/or line-folding signature */
                    prefix_length = -1;
                    for (; in_pos < in_limit; in_pos += 1) {
                        UChar c = *(in_pos++);

                        /* prospectively copy the input character to the buffer */
                        *(buf_pos++) = c;

                        /* update statistics */
                        if (c == BKSL_CHAR) {
                            prefix_length = (in_pos - text) - 1; /* the number of characters preceding this one */
                            backslash_count += 1;                /* the total number of backslashes on this line */
                            nonws = CIF_FALSE;                   /* reset the nonws flag */
                        } else {
                            switch(CLASS_OF(c, *parser)) {
                                case EOL_CLASS:
                                    goto end_of_line;
                                case WS_CLASS:
                                    /* nothing */
                                    break;
                                default:
                                    nonws = CIF_TRUE;
                                    break;
                            }
                        }
                    }

                    end_of_line:
                    /* analyze the results of the scan of the first line */
                    prefix_length = prefix_length + 1 - backslash_count;
                    if (!nonws && ((backslash_count == 1)
                            || ((backslash_count == 2) && (prefix_length > 0) && (text[prefix_length] == BKSL_CHAR)))) {
                        /* prefixed or folded or both */
                        folded = ((prefix_length == 0) || (backslash_count == 2));

                        /* discard the text already copied to the buffer */
                        buf_pos = buffer;
                    } else {
                        /* neither prefixed nor folded */
                        prefix_length = 0;
                        folded = CIF_FALSE;
                    }
                }

                if (!folded && (prefix_length == 0)) {
                    /* copy the rest of the text verbatim */
                    u_strncpy(buf_pos, in_pos, text_length - (in_pos - text));
                    buffer[text_length] = 0;
                } else {
                    /* process the text line-by-line, confirming and consuming prefixes and unfolding as appropriate */
                    while (in_pos < in_limit) { /* each line */
                        UChar *buf_temp = NULL;

                        /* consume the prefix, if any */
                        if (u_strncmp(text, in_pos, prefix_length) == 0) {
                            in_pos += prefix_length;
                        } else {
                            /* TODO: error missing prefix */
                            /* recover by copying the un-prefixed text (no special action required) */
                        }

                        /* copy from input to buffer, up to and including EOL, accounting for folding if applicable */
                        while (in_pos < in_limit) { /* each UChar unit */
                            UChar c = *(in_pos++);

                            /* copy the character unconditionally */
                            *(buf_pos++) = c;

                            /* handle line folding where appropriate */
                            if ((c == BKSL_CHAR) && folded) {
                                buf_temp = buf_pos - 1;
                            } else {
                                int class = CLASS_OF(c, *parser);

                                if (class == EOL_CLASS) {
                                    /* if appropriate, rewind the output buffer to achieve line folding */
                                    if (buf_temp != NULL) buf_pos = buf_temp;
                                    break;
                                } else if (class != WS_CLASS) {
                                    buf_temp = NULL;
                                }
                            }
                        }
                    }

                    /* add a string terminator */
                    *buf_pos = 0;
                }

                /* assign the buffer to the value object */
                result = cif_value_init_char(value, buffer);
                if (result == CIF_OK) {
                    /* The value is successfully parsed / initialized; assign it to the output variable */
                    *dest = value;
                    return CIF_OK;
                } else {
                    SET_RESULT(result);
                }

                /* clean up the buffer */
                free(buffer);
            } /* else failed to allocate the text buffer */
        }

        FAILURE_HANDLER(early):
        /* clean up the value */
        if (value != *dest) cif_value_free(value); /* ignore any error */
    }

    FAILURE_TERMINUS;
}

