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
 * CIF 2.0 parsing, however, because implicitly, those are defined in terms of abstract characters.  The encoded
 * forms vary with the code page assumed by the compiler, whereas CIF 2.0's actual encoding does not.
 */

/*
 * The CIF parser implemented herein is a predictive recursive descent parser with full error recovery.
 */

#include <stdio.h>
#include <assert.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* For UChar: */
#include <unicode/umachine.h>

#include "cif.h"
#include "internal/utils.h"

/* plain macros */

#define SSIZE_T_MAX      (((size_t) -1) / 2)
#define BUF_SIZE_INITIAL (4 * (CIF_LINE_LENGTH + 2))
#define BUF_MIN_FILL          (CIF_LINE_LENGTH + 2)

/* specific character codes */
/* Note: numeric character codes avoid codepage dependencies associated with use of ordinary char literals */
#define LF_CHAR       0x0A
#define CR_CHAR       0x0D
#define DECIMAL_CHAR  0x2E
#define COLON_CHAR    0x3A
#define SEMI_CHAR     0x3B
#define QUERY_CHAR    0x3F
#define BKSL_CHAR     0x5C
#define EOF_CHAR      0xFFFF

/* character class codes */
/* code NO_CLASS must have value 0; the values of other codes may be changed as desired */
#define NO_CLASS      0
#define GENERAL_CLASS 1
#define WS_CLASS      2
#define EOL_CLASS     3
#define EOF_CLASS     4
#define HASH_CLASS    5
#define UNDERSC_CLASS 6
#define QUOTE_CLASS   7
#define SEMI_CLASS    9
#define OBRAK_CLASS  10
#define CBRAK_CLASS  11
#define OCURL_CLASS  12
#define CCURL_CLASS  13
#define COLON_CLASS  14
#define DOLLAR_CLASS 15
#define A_CLASS      16
#define B_CLASS      17
#define D_CLASS      18
#define E_CLASS      19
#define G_CLASS      20
#define L_CLASS      21
#define O_CLASS      22
#define P_CLASS      23
#define S_CLASS      24
#define T_CLASS      25
#define V_CLASS      26

/* identifies the numerically-last class code, but does not itself directly represent a class: */
#define LAST_CLASS   V_CLASS

/* character metaclass codes */
#define NO_META       0
#define GENERAL_META  1
#define WS_META       2
#define OPEN_META     3
#define CLOSE_META    4

/* data types */

enum token_type {
    BLOCK_HEAD, FRAME_HEAD, FRAME_TERM, LOOPKW, NAME, OTABLE, CTABLE, OLIST, CLIST, KV_SEP, VALUE, QVALUE, TVALUE, END
};

typedef ssize_t (*read_chars_t)(void *char_source, UChar *dest, size_t count);

struct scanner_s {
    UChar *buffer;                   /* A character buffer from which to scan characters */
    size_t buffer_size;              /* The total size of the buffer */
    size_t buffer_limit;             /* The size of the initial segment of the buffer containing valid characters */
    UChar *next_char;                /* A pointer into the buffer to the next character to be scanned */

    enum token_type ttype;           /* The grammatic category of the most recent token parsed */
    UChar *text_start;               /* The start position of the text from which the current token, if any, was parsed.
                                        This may differ from tvalue_start in some cases, such as for tokens representing
                                        delimited values. */
    UChar *tvalue_start;             /* The start position of the value of the current token, if any */
    size_t tvalue_length;            /* The length of the value of the current token, if any */

    size_t line;                     /* The current 1-based input line number */
    unsigned column;                 /* The number of characters so far scanned from the current line */

    unsigned int char_class[160];    /* Character class codes of the first 160 Unicode characters */
    unsigned int meta_class[LAST_CLASS + 1];  /* Character metaclass codes for all the character classes */

    /* character source */
    read_chars_t read_func;
    void *char_source;
    int at_eof;
};

/* static function headers */

/* grammar productions */
static int parse_cif(struct scanner_s *scanner, cif_t *cifp);
static int parse_container(struct scanner_s *scanner, cif_container_t *container, int is_block);
static int parse_item(struct scanner_s *scanner, cif_container_t *container, UChar *name);
static int parse_loop(struct scanner_s *scanner, cif_container_t *container);
static int parse_list(struct scanner_s *scanner, cif_value_t **listp);
static int parse_table(struct scanner_s *scanner, cif_value_t **tablep);
static int parse_value(struct scanner_s *scanner, cif_value_t **valuep);

/* scanner functions */
static int next_token(struct scanner_s *scanner);
static int scan_ws(struct scanner_s *scanner);
static int scan_to_ws(struct scanner_s *scanner);
static int scan_to_eol(struct scanner_s *scanner);
static int scan_unquoted(struct scanner_s *scanner);
static int scan_delim_string(struct scanner_s *scanner);
static int scan_text(struct scanner_s *scanner);
static int get_more_chars(struct scanner_s *scanner);

/* other functions */
static int decode_text(struct scanner_s *scanner, UChar *text, int32_t text_length, cif_value_t **dest);

/* function-like macros */

#define INIT_SCANNER(s) do { \
    int _i; \
    (s).line = 1; \
    (s).column = 0; \
    (s).ttype = END; \
    for (_i =   0; _i <  32;         _i += 1) (s).char_class[_i] = NO_CLASS; \
    for (_i =  32; _i < 127;         _i += 1) (s).char_class[_i] = GENERAL_CLASS; \
    for (_i = 128; _i < 160;         _i += 1) (s).char_class[_i] = NO_CLASS; \
    for (_i =   1; _i <= LAST_CLASS; _i += 1) (s).meta_class[_i] = GENERAL_META; \
    (s).char_class[0x09] =   WS_CLASS; \
    (s).char_class[0x20] =   WS_CLASS; \
    (s).char_class[CR_CHAR] =  EOL_CLASS; \
    (s).char_class[LF_CHAR] =  EOL_CLASS; \
    (s).char_class[0x23] =   HASH_CLASS; \
    (s).char_class[0x95] =   UNDERSC_CLASS; \
    (s).char_class[0x22] =   QUOTE_CLASS; \
    (s).char_class[0x27] =   QUOTE_CLASS; \
    (s).char_class[SEMI_CHAR] = SEMI_CLASS; \
    (s).char_class[0x5B] =   OBRAK_CLASS; \
    (s).char_class[0x5D] =   CBRAK_CLASS; \
    (s).char_class[0x7B] =   OCURL_CLASS; \
    (s).char_class[0x7D] =   CCURL_CLASS; \
    (s).char_class[COLON_CHAR] = GENERAL_CLASS; \
    (s).char_class[0x24] =   DOLLAR_CLASS; \
    (s).char_class[0x41] =   A_CLASS; \
    (s).char_class[0x61] =   A_CLASS; \
    (s).char_class[0x42] =   B_CLASS; \
    (s).char_class[0x62] =   B_CLASS; \
    (s).char_class[0x44] =   D_CLASS; \
    (s).char_class[0x64] =   D_CLASS; \
    (s).char_class[0x45] =   E_CLASS; \
    (s).char_class[0x65] =   E_CLASS; \
    (s).char_class[0x47] =   G_CLASS; \
    (s).char_class[0x67] =   G_CLASS; \
    (s).char_class[0x4C] =   L_CLASS; \
    (s).char_class[0x6C] =   L_CLASS; \
    (s).char_class[0x4F] =   O_CLASS; \
    (s).char_class[0x6F] =   O_CLASS; \
    (s).char_class[0x50] =   P_CLASS; \
    (s).char_class[0x70] =   P_CLASS; \
    (s).char_class[0x53] =   S_CLASS; \
    (s).char_class[0x73] =   S_CLASS; \
    (s).char_class[0x54] =   T_CLASS; \
    (s).char_class[0x74] =   T_CLASS; \
    (s).char_class[0x56] =   V_CLASS; \
    (s).char_class[0x76] =   V_CLASS; \
    (s).char_class[0x7F] =   NO_CLASS; \
    (s).meta_class[NO_CLASS] =  NO_META; \
    (s).meta_class[WS_CLASS] =  WS_META; \
    (s).meta_class[EOL_CLASS] = WS_META; \
    (s).meta_class[EOF_CLASS] = WS_META; \
    (s).meta_class[OBRAK_CLASS] = OPEN_META; \
    (s).meta_class[OCURL_CLASS] = OPEN_META; \
    (s).meta_class[CBRAK_CLASS] = CLOSE_META; \
    (s).meta_class[CCURL_CLASS] = CLOSE_META; \
} while (CIF_FALSE)

#define MAKE_COLON_SPECIAL(s) do { \
    (s)->char_class[COLON_CHAR] = COLON_CLASS; \
} while (CIF_FALSE)

#define MAKE_COLON_NORMAL(s) do { \
    (s)->char_class[COLON_CHAR] = GENERAL_CLASS; \
} while (CIF_FALSE)

/**
 * @brief Determines the class of the specified character based on the specified scanner ruleset.
 *
 * @param[in] c the character whose class is requested.  Must be non-negative.  May be evaluated more than once.
 * @param[in] s a struct scanner_s containing the rules defining the character's class.
 */
/*
 * There are additional code points that perhaps should be mapped to NO_CLASS: BMP not-a-character code points
 * 0xFDD0 - 0xFDEF, and per-plane not-a-character code points 0x??FFFE and 0x??FFFF except 0xFFFF (which is co-opted
 * as an EOF marker instead).  Also surrogate code units when not part of a surrogate pair.  At this time, however,
 * there appears little benefit to this macro making those comparatively costly distinctions, especially when it
 * cannot do so at all in the case of unpaired surrogates.
 */
#define CLASS_OF(c, s)  (((c) < 160) ? (s)->char_class[c] : ((c == EOF_CHAR) ? EOF_CLASS : GENERAL_CLASS))

#define GET_MORE_CHARS(s, ev) do { \
    /* FIXME: consider dropping this macro in favor of calling get_more_chars() directly */ \
    ev = get_more_chars(s); \
} while (CIF_FALSE)

/*
 * Ensures that the buffer of the specified scanner has at least one character available to read; the available
 * character can be an EOF marker
 */
#define ENSURE_CHARS(s, ev) do { \
    struct scanner_s *_s_ec = (s); \
    if (_s_ec->next_char >= _s_ec->buffer + _s_ec->buffer_limit) { \
        GET_MORE_CHARS(_s_ec, ev); \
        if (ev != CIF_OK) break; \
    } else { \
        ev = CIF_OK; \
    } \
} while (CIF_FALSE)

/*
 * Inputs the next available character from the scanner source into the given character without changing the relative
 * token start position or recorded token length.  The absolute token start position may change, however. 
 */
#define NEXT_CHAR(s, c, ev) do { \
    struct scanner_s *_s_nc = (s); \
    ENSURE_CHARS(_s_nc, ev); \
    c = *(_s_nc->next_char++); \
} while (CIF_FALSE)

/*
 * Returns the character that will be returned (again) by the next invocation of NEXT_CHAR().  May cause additional
 * characters to be read into the buffer, but does not otherwise alter the scan state.
 */
#define PEEK_CHAR(s, c, ev) do { \
    struct scanner_s *_s_pc = (s); \
    ENSURE_CHARS(_s_pc, ev); \
    c = *(_s_pc->next_char); \
} while (CIF_FALSE)

/*
 * Consumes the current token, if any, from the scanner.  When next asked for a token, the scanner will provide
 * the subsequent one.
 */
#define CONSUME_TOKEN(s) do { \
    struct scanner_s *_s_ct = (s); \
    _s_ct->text_start = _s_ct->next_char; \
    _s_ct->tvalue_start = _s_ct->next_char; \
    _s_ct->tvalue_length = 0; \
} while (CIF_FALSE)

/*
 * Rejects the current token, if any, from the scanner, so that the same text is scanned again the next time a
 * token is requested.  This is useful only if the scanner is afterward modified so that the rejected token text
 * is scanned differently the next time.
 * s: the scanner
 * t: the type of the token immediately preceding the rejected one
 */
#define REJECT_TOKEN(s, t) do { \
    struct scanner_s *_s_rt = (s); \
    _s_rt->next_char = _s_rt->text_start; \
    _s_rt->tvalue_start = _s_rt->next_char; \
    _s_rt->tvalue_length = 0; \
    _s_rt->ttype = (t); \
} while (CIF_FALSE)

/*
 * Pushes back all but the specified number of initial characters of the current token to be scanned again as part of
 * the next token.  The result is undefined if more characters are pushed back than the current token contains.  The
 * length of the token value is adjusted so that the value extends to the new end of the token.
 * s: the scanner in which to push back characters
 * n: the number of characters of the whole token text to keep (not just of the token value, if they differ)
 */
#define PUSH_BACK(s, n) do { \
    struct scanner_s *_s_pb = (s); \
    int32_t _n = (n); \
    _s_pb->next_char = _s_pb->text_start + _n; \
    _s_pb->tvalue_length = _s_pb->next_char - _s_pb->tvalue_start; \
} while (CIF_FALSE)

/* function implementations */

int cif_parse_internal(FILE *source, cif_t *dest) {
    struct scanner_s scanner;
    int result;

    scanner.buffer = (UChar *) malloc(BUF_SIZE_INITIAL * sizeof(UChar));
    if (scanner.buffer == NULL) {
        result = CIF_ERROR;
    } else {
        INIT_SCANNER(scanner);
        scanner.buffer_size = BUF_SIZE_INITIAL;
        scanner.buffer_limit = 0;
        scanner.next_char = scanner.buffer;
        scanner.text_start = scanner.buffer;
        scanner.tvalue_start = scanner.buffer;
        scanner.tvalue_length = 0;

        /* TODO: set up character source */
        /* TODO: handle BOM, if any (?) */
        /* TODO: handle CIF version comment, if any */

        result = parse_cif(&scanner, dest);
        
        free(scanner.buffer);
    }

    /* TODO: other cleanup ? */

    return result;
}

/*
 * Parse a while CIF via the provided scanner into the provided CIF object.  On success, all characters available from
 * the scanner will have been consumed.  The provided CIF object does not need to be empty, but semantic errors will
 * occur if it contains data blocks with block codes matching those of blocks read from the input.
 *
 * If the provided CIF handle is NULL then a syntax-only check is performed, but nothing is stored.  In that mode,
 * semantic constraints such as uniqueness of block codes, frame codes, and data names are not checked.
 */
static int parse_cif(struct scanner_s *scanner, cif_t *cif) {
    int result;

    do {
        cif_block_t *block = NULL;
        int32_t token_length;
        UChar *token_value;

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }
        token_value = scanner->tvalue_start;
        token_length = scanner->tvalue_length;

        switch (scanner->ttype) {
            case BLOCK_HEAD:
                if (cif != NULL) {
                    /* insert a string terminator into the input buffer, after the current token */
                    UChar saved = *(token_value + token_length);
                    *(token_value + token_length) = 0;
    
                    result = cif_create_block(cif, token_value, &block);
                    switch (result) {
                        case CIF_INVALID_BLOCKCODE:
                            /* TODO: report syntax error */
                            /* recover by using the block code anyway */
                            if ((result = (cif_create_block_internal(cif, token_value, 1, &block)))
                                    != CIF_DUP_BLOCKCODE) {
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
                } /* else syntax check only; no actual block will be created */

                CONSUME_TOKEN(scanner);
    
                break;
            case END:
                /* it's more useful to leave the token than to consume it */
                goto cif_end;
            case KV_SEP:
                /* should not happen because we did not use MAKE_COLON_SPECIAL */
                result = CIF_INTERNAL_ERROR;
                goto cif_end;
            default:
                /* TODO: error missing data block header */
                /* recover by creating and installing an anonymous block context to eat up the content */
                if (cif != NULL) {
                    if ((result = cif_create_block_internal(cif, &cif_uchar_nul, 1, &block)) == CIF_DUP_BLOCKCODE) {
                        /*
                         * or fall back to using an anonymous context that already exists; this should happen only if
                         * the user provided a non-empty initial CIF
                         */
                        result = cif_get_block(cif, &cif_uchar_nul, &block);
                    }
                }
    
                break;
        }

        if (result != CIF_OK) {
            break;
        }

        result = parse_container(scanner, block, CIF_TRUE);
        if (block != NULL) {
            cif_block_free(block); /* ignore any error */
        }
    } while (result == CIF_OK);

    cif_end:
    return result;
}

static int parse_container(struct scanner_s *scanner, cif_container_t *container, int is_block) {
    int result;

    do {
        int32_t token_length;
        UChar *token_value;
        UChar *name;

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }
        token_value = scanner->tvalue_start;
        token_length = scanner->tvalue_length;

        switch (scanner->ttype) {
            case BLOCK_HEAD:
                if (!is_block) {
                    /* TODO: error: unterminated save frame */
                    /* recover by continuing */
                }
                /* reject the token */
                result = CIF_OK;
                goto container_end;
            case FRAME_HEAD:
                if (!is_block) {
                    /* TODO: error: unterminated save frame */
                    /* recover by rejecting the token */
                    result = CIF_OK;
                    goto container_end;
                } else {
                    cif_container_t *frame = NULL;
                    UChar saved;
    
                    /* insert a string terminator into the input buffer, after the current token */
                    saved = *(token_value + token_length);
                    *(token_value + token_length) = 0;
   
                    if (container == NULL) {
                        result = CIF_OK;
                    } else { 
                        result = cif_block_create_frame(container, token_value, &frame);
                        switch (result) {
                            case CIF_INVALID_FRAMECODE:
                                /* TODO: report syntax error */
                                /* recover by using the frame code anyway */
                                if ((result = (cif_block_create_frame_internal(container, token_value, 1, &frame)))
                                        != CIF_DUP_FRAMECODE) {
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
                    }
    
                    /* restore the next character in the input buffer */
                    *(token_value + token_length) = saved;
                    CONSUME_TOKEN(scanner);
    
                    if (result == CIF_OK) {
                        result = parse_container(scanner, frame, CIF_FALSE);
                    }
                }
                break;
            case FRAME_TERM:
                CONSUME_TOKEN(scanner);
                result = CIF_OK;
                if (is_block) {
                    /* TODO: error unexpected frame terminator */
                    /* recover by dropping the token */
                } else {
                    /* close the context */
                    goto container_end;
                }
                break;
            case LOOPKW:
                CONSUME_TOKEN(scanner);
                result = parse_loop(scanner, container);
                break;
            case NAME:
                name = (UChar *) malloc((token_length + 1) * sizeof(UChar));
                if (name == NULL) {
                    result = CIF_ERROR;
                    goto container_end;
                } else {
                    /* copy the data name to a separate Unicode string in the item context */
                    u_strncpy(name, token_value, token_length);
                    name[token_length] = 0;
                    CONSUME_TOKEN(scanner);
    
                    /* check for dupes */
                    result = ((container == NULL) ? CIF_NOSUCH_ITEM
                            : cif_container_get_item_loop(container, name, NULL));
                    if (result == CIF_OK) {
                        /* TODO: error duplicate name */
                        /* recover by rejecting the item (but still parsing the associated value) */
                        result = parse_item(scanner, container, NULL);
                    } else if (result == CIF_NOSUCH_ITEM) {
                        result = parse_item(scanner, container, name);
                    }

                    free(name);
                }
                break;
            case TVALUE:
            case QVALUE:
            case VALUE:
            case OLIST:  /* opening delimiter of a list value */
            case OTABLE: /* opening delimiter of a table value */
                /* TODO: error unexpected value */
                /* recover by consuming and discarding the value */
                result = parse_item(scanner, container, NULL);
                break;
            case CTABLE:
            case CLIST:
                /* TODO: error unexpected closing delimiter */
                /* recover by dropping it */
                CONSUME_TOKEN(scanner);
                result = CIF_OK;
                break;
            case END:
                if (!is_block) {
                    /* TODO: error unterminated save frame */
                    /* recover by rejecting the token (as would happen anyway) */
                }
                /* no special action, just reject the token */
                result = CIF_OK;
                goto container_end;
            default: /* including at least KV_SEP */
                /* should not happen */
                result = CIF_INTERNAL_ERROR;
                break;
        }

    } while (result == CIF_OK);

    container_end:
    /* no cleanup needed here */
    return result;
}

static int parse_item(struct scanner_s *scanner, cif_container_t *container, UChar *name) {
    int result = next_token(scanner);

    if (result == CIF_OK) {
        cif_value_t *value = NULL;
    
        switch (scanner->ttype) {
            case OLIST:  /* opening delimiter of a list value */
            case OTABLE: /* opening delimiter of a table value */
            case TVALUE:
            case QVALUE:
            case VALUE:
                /* parse the value */
                result = parse_value(scanner, &value);
                break;
            case KV_SEP:
                /* should not happen because MAKE_COLON_SPECIAL was not used */
                result = CIF_INTERNAL_ERROR;
                break;
            default:
                /* TODO: error missing value */
                /* recover by inserting a synthetic unknown value */
                result = cif_value_create(CIF_UNK_KIND, &value);
                /* do not consume the token */
                break;
        }
    
        if (result == CIF_OK) {
            if ((name != NULL) && (container != NULL)) {
                /* _copy_ the value into the CIF */
                result = cif_container_set_value(container, name, value);
            }
            cif_value_free(value); /* ignore any error */
        }
    }

    return result;
}

static int parse_loop(struct scanner_s *scanner, cif_container_t *container) {
    string_element_t *first_name = NULL;          /* the first data name in the header */
    string_element_t **next_namep = &first_name;  /* a pointer to the pointer to the next data name in the header */
    int name_count = 0;
    int result;

    /* parse the header */
    while (((result = next_token(scanner)) == CIF_OK) && (scanner->ttype == NAME)) {
        int32_t token_length = scanner->tvalue_length;
        UChar *token_value = scanner->tvalue_start;

        *next_namep = (string_element_t *) malloc(sizeof(string_element_t));
        if (*next_namep == NULL) {
            result = CIF_ERROR;
            goto loop_end;
        } else {
            (*next_namep)->next = NULL;
            (*next_namep)->string = (UChar *) malloc((token_length + 1) * sizeof(UChar));
            if ((*next_namep)->string == NULL) {
                result = CIF_ERROR;
                goto loop_end;
            } else {
                u_strncpy((*next_namep)->string, token_value, token_length);
                (*next_namep)->string[token_length] = 0;
                next_namep = &((*next_namep)->next);
                name_count += 1;
            }
        }

        CONSUME_TOKEN(scanner);
    }

    if (result == CIF_OK) {
        /* name_count is the number of data names parsed for the header */
        if (name_count == 0) {
            /* TODO: error empty loop header */
            /* recover by ignoring it */
        } else {
            /* an array of data names for creating the loop; elements belong to the linked list, not this array */
            UChar **names = (UChar **) malloc((name_count + 1) * sizeof(UChar *));

            if (names == NULL) {
                result = CIF_ERROR;
                goto loop_end;
            } else {
                cif_loop_t *loop = NULL;
                string_element_t *next_name;
                cif_packet_t *packet;
                int name_index = 0;
                
                /* validate the header and create the loop */

                name_index = 0;
                for(next_name = first_name; next_name != NULL; next_name = next_name->next) {
                    switch (result = ((container == NULL) ? CIF_NOSUCH_ITEM
                            : cif_container_get_item_loop(container, next_name->string, NULL))) {
                        case CIF_OK:
                            /* TODO: error duplicate item */
                            /* recover by ignoring the name and associated values in the loop to come */
                            /* a place-holder is retained in the list so that packet values are counted and assigned
                               correctly */
                            free(next_name->string);
                            next_name->string = NULL;
                            break;
                        case CIF_NOSUCH_ITEM:
                            /* the expected case */
                            names[name_index++] = next_name->string;
                            break;
                        default:
                            goto loop_body_end;
                    }
                }
                names[name_index] = NULL;

                if (container != NULL) {
                    /* create the loop */
                    switch (result = cif_container_create_loop(container, NULL, names, &loop)) {
                        case CIF_OK:         /* expected */
                        case CIF_NULL_LOOP:  /* tolerable */
                            break;
                        case CIF_INVALID_ITEMNAME:
                        case CIF_DUP_ITEMNAME:
                            /* these should not happen because the names were already validated */
                            result = CIF_INTERNAL_ERROR;
                            /* fall through */
                        default:
                            goto loop_body_end;
                    }
                }

                /* read packets */
                if ((result = cif_packet_create(&packet, names)) == CIF_OK) {
                    int have_packets = CIF_FALSE;

                    next_name = first_name;
                    while ((result = next_token(scanner)) == CIF_OK) {
                        UChar *name;
                        cif_value_t *value = NULL;

                        switch (scanner->ttype) {
                            case OLIST:  /* opening delimiter of a list value */
                            case OTABLE: /* opening delimiter of a table value */
                            case TVALUE:
                            case QVALUE:
                            case VALUE:
                                name = next_name->string;
                                next_name = next_name->next;

                                if (name != NULL) {
                                    if (((result = cif_packet_set_item(packet, name, NULL)) != CIF_OK)
                                            || ((result = cif_packet_get_item(packet, name, &value)) != CIF_OK)) {
                                        goto packets_end;
                                    }
                                }

                                /* parse the value */
                                result = parse_value(scanner, &value);
                                if (name == NULL) {
                                    /* discard the value -- the associated name was a duplicate or failed validation */
                                    cif_value_free(value);  /* ignore any error */
                                } /* else the value is already in the packet and belongs to it */

                                if (result != CIF_OK) { /* this is the parse_value() result code */
                                    goto packets_end;
                                } else if (next_name == NULL) {  /* that was the last value in the packet */
                                    if ((loop != NULL) && ((result = cif_loop_add_packet(loop, packet)) != CIF_OK)) {
                                        goto packets_end;
                                    }
                                    have_packets = CIF_TRUE;
                                    next_name = first_name;
                                }

                                break;
                            case CLIST:
                            case CTABLE:
                                /* TODO: error unexpected list/table delimiter */
                                /* recover by dropping it */
                                CONSUME_TOKEN(scanner);
                                break;
                            case KV_SEP: /* FIXME: should KV_SEP really be caught here? */
                                result = CIF_INTERNAL_ERROR;
                                goto packets_end;
                            default:
                                if (next_name != first_name) {
                                    /* TODO: error partial packet */
                                    /* recover by synthesizing unknown values to fill the packet, and saving it */
                                    if (loop != NULL) {
                                        for (; next_name != NULL; next_name = next_name->next) {
                                            if ((next_name->string != NULL)
                                                    && (result = cif_packet_set_item(packet, next_name->string, NULL))
                                                            != CIF_OK) {
                                                goto packets_end;
                                            }
                                        }
                                        if ((result = cif_loop_add_packet(loop, packet)) == CIF_OK) {
                                            goto packets_end;
                                        }
                                    }
                                } else if (!have_packets) {
                                    /* TODO: error no packets */
                                    /* recover by doing nothing for now */
                                    /*
                                     * JCB note: a loop without packets is not valid in the data model, but the data
                                     * names need to be retained until the container has been fully parsed to allow
                                     * duplicates to be detected correctly.  Probably some kind of pruning is needed
                                     * after the container is fully parsed.
                                     */
                                }
                                /* the token is not examined in any way */
                                goto packets_end;
                        } /* end switch */
                    } /* end while */

                    packets_end:
                    cif_packet_free(packet);
                }

                loop_body_end:
                free(names);
                /* elements of names belong to the linked list; they are freed later */
            } /* end ifelse (names) */
        } /* end ifelse (name_count) */
    } /* end if (header result) */

    loop_end:

    /* clean up the header name linked list */
    while (first_name != NULL) {
        string_element_t *next = first_name->next;

        if (first_name->string != NULL) {
            free(first_name->string);
        }
        free(first_name);
        first_name = next;
    }

    return result;
}

static int parse_list(struct scanner_s *scanner, cif_value_t **listp) {
    cif_value_t *list = NULL;
    size_t next_index = 0;
    int result;

    if (*listp == NULL) {
        /* create a new value object */
        result = cif_value_create(CIF_LIST_KIND, &list);
    } else {
        /* use the existing value object */
        list = *listp;
        result = cif_value_init(list, CIF_LIST_KIND);
    }

    while (result == CIF_OK) {
        cif_value_t *element = NULL;

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }

        switch (scanner->ttype) {
            case OLIST:  /* opening delimiter of a list value */
            case OTABLE: /* opening delimiter of a table value */
            case TVALUE:
            case QVALUE:
            case VALUE:
                /* Insert a dummy value, then update it with the parse result.  This minimizes copying. */
                result = cif_value_insert_element_at(list, next_index, NULL);
                switch ((result == CIF_OK) ? (result = cif_value_get_element_at(list, next_index++, &element))
                        : result) {
                    case CIF_OK:
                        /* element now points to a list element (not a copy). Parse the value into it. */
                        result = parse_value(scanner, &element);
                        break;
                    case CIF_ARGUMENT_ERROR:
                    case CIF_INVALID_INDEX:
                        /* should not happen */
                        result = CIF_INTERNAL_ERROR;
                        break;
                }
                break;
            case KV_SEP:
                /* should not happen because MAKE_COLON_SPECIAL was not used */
                result = CIF_INTERNAL_ERROR;
                break;
            case CLIST:
                /* accept the token and end the list */
                CONSUME_TOKEN(scanner);
                result = CIF_OK;
                goto list_end;
            default:
                /* TODO: error unterminated list */
                /* recover by synthetically closing the list */
                /* do not consume the token */
                result = CIF_OK;
                goto list_end;
        }
    } /* while */

    list_end:

    if (result == CIF_OK) {
        *listp = list;
    } else if (list != *listp) {
        cif_value_free(list);
    }

    return result;
}

static int parse_table(struct scanner_s *scanner, cif_value_t **tablep) {
    cif_value_t *table;
    int result;

    assert (tablep != NULL);
    if (*tablep == NULL) {
        result = cif_value_create(CIF_TABLE_KIND, &table);
    } else {
        table = *tablep;
        result = cif_value_init(table, CIF_TABLE_KIND);
    }

    while (result == CIF_OK) {
        /* parse key / separator / value triples */
        UChar *key = NULL;
        cif_value_t *value = NULL;
        int unquoted_key = CIF_FALSE;

        /* scan the next key, if any */

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }
        switch (scanner->ttype) {
            case VALUE:
                if (*(scanner->tvalue_start) == COLON_CHAR) {
                    /* TODO error missing key */
                    /* recover by re-scanning the token (later) as a separator, and dropping the subsequent value */
                    REJECT_TOKEN(scanner, QVALUE);
                    break;
                } else {
                    size_t index;
                    unquoted_key = CIF_TRUE;  /* flag that this MAY be an unquoted key */

                    /*
                     * an unquoted string is not valid here, but defer reporting the error until we know whether it is
                     * a stray value or an unquoted key.
                     */
                    for (index = 1; index < scanner->tvalue_length; index += 1) {
                        if (scanner->tvalue_start[index] == COLON_CHAR) {
                            /* push back the first colon and all characters following it */
                            PUSH_BACK(scanner, index);
                        }
                    }
                    /* even if no colon was present in the token value it might still be an unquoted key */
                }
                /* fall through */
            case QVALUE:  /* this and CTABLE are the only valid token types here */
                /* copy the key to a NUL-terminated Unicode string */
                key = (UChar *) malloc((scanner->tvalue_length + 1) * sizeof(UChar));
                if (key != NULL) {
                    u_strncpy(key, scanner->tvalue_start, scanner->tvalue_length);
                    key[scanner->tvalue_length] = 0;
                    CONSUME_TOKEN(scanner);    
                    break;
                }
                result = CIF_ERROR;
                goto table_end;
            case TVALUE:
                /* TODO error: disallowed key type */
                /* recover by attempting to use the text value as a key */
                /* extract the text value as a NUL-terminated Unicode string */
                if ((result = decode_text(scanner, scanner->tvalue_start, scanner->tvalue_length, &value)) == CIF_OK) {
                    CONSUME_TOKEN(scanner);
                    result = cif_value_get_text(value, &key);
                    cif_value_free(value); /* ignore any error */
                    if (result == CIF_OK) {
                        break;
                    }
                }
                goto table_end;
            case CTABLE:
                CONSUME_TOKEN(scanner);
                result = CIF_OK;
                goto table_end;
            case OLIST:  /* opening delimiter of a list value */
            case OTABLE: /* opening delimiter of a table value */
                /* TODO: disallowed key type */
                /* recover by accepting and dropping the value, and consuming the subsequent KV_SEP, if any */
                if ((result = parse_value(scanner, &value)) == CIF_OK) {
                    cif_value_free(value);  /* ignore any error */
                    continue; 
                }
                goto table_end;
            case KV_SEP:
                /* TODO: error missing key */
                /* recover by consuming the following value */
                /* do not consume the token -- it will be scanned again, next */
                break;
            default:
                /* TODO: error unterminated table */
                /* recover by ending the table; the token is left for the calling context to handle */
                result = CIF_OK;
                goto table_end;
        }

        /* scan the key/value separator */

        /* ensure that the special significance of the colon is activated */
        MAKE_COLON_SPECIAL(scanner);

        if ((result = next_token(scanner)) != CIF_OK) {
            /* Clean up the key and the value object as necessary */
            if (key == NULL) {
                cif_value_free(value); /* ignore any error */
            } else if (key != NULL) {
                /* the previous token cannot be accepted as a key */
                cif_value_remove_item_by_key(table, key, NULL);  /* frees the value after removing it.
                                                                    ignore any error. */
                free(key);
            }
            break;
        } else {
            if (scanner->ttype == KV_SEP) {
                /* expected */
                CONSUME_TOKEN(scanner);

                if (unquoted_key) {
                    /* TODO error: unquoted table key */
                    /* recover by accepting the key (already parsed) */
                }

                /* validate the key and prepare a value object */
                if (key != NULL) {
                    /* check for a duplicate key */
                    result = cif_value_get_item_by_key(table, key, NULL);
                    if (result == CIF_NOSUCH_ITEM) {
                        /* the expected result */

                        /* enroll the key in the table with an unknown value */
                        if ((result = cif_value_set_item_by_key(table, key, NULL)) == CIF_OK) {
                            /* Record a pointer to the item value.  This provides a reference, not a copy. */
                            result = cif_value_get_item_by_key(table, key, &value);
                        }
                        /* FIXME: handle CIF_INVALID_INDEX (invalid key) */
                    } else if (result == CIF_OK) {
                        /* TODO: error duplicate key */
                        /* recover by discarding the key, then preparing to parse and discard the incoming value */
                        free(key);
                        key = NULL;
                        result = cif_value_create(CIF_UNK_KIND, &value);
                    } /* else nothing */
                }
            } else { /* expected a key/value separator, but got something else */
                /* Clean up the key and the value object as necessary */
                if (key == NULL) {
                    cif_value_free(value); /* ignore any error */
                } else if (key != NULL) {
                    /* the previous token cannot be accepted as a key */
                    cif_value_remove_item_by_key(table, key, NULL);  /* frees the value after removing it.
                                                                        ignore any error. */
                    free(key);
                }

                /* determine what kind of error to report and how to proceed */
                switch (scanner->ttype) {
                    case TVALUE:
                    case QVALUE:
                    case VALUE:
                    case OLIST:  /* opening delimiter of a list value */
                    case OTABLE: /* opening delimiter of a table value */
                        /* TODO error extra value (preceding token) */
                        /* recover by dropping the previous token and continuing */
                        continue;
                    case CTABLE:
                        /* TODO: error extra value at end of table (preceding token) */
                        /* recover by dropping the previous token and ending the table */
                        CONSUME_TOKEN(scanner);
                        result = CIF_OK;
                        goto table_end;
                    default:
                        /* TODO: error unterminated table */
                        /* recover by closing the table and letting the calling context handle the token */
                        result = CIF_OK;
                        goto table_end;
                }
                /* not reached */
                assert(CIF_FALSE);
            } /* end if scanner->ttype */
        }

        /* scan the value */

        if (result == CIF_OK) {
            assert(value != NULL);

            if ((result = next_token(scanner)) == CIF_OK) {
                switch (scanner->ttype) {
                    case OLIST:  /* opening delimiter of a list value */
                    case OTABLE: /* opening delimiter of a table value */
                    case TVALUE:
                    case QVALUE:
                    case VALUE:
                        /* parse the incoming value into the existing value object */
                        result = parse_value(scanner, &value);
                        break;
                    default:
                        /* TODO: error missing value */
                        /* recover by using an unknown value (already set) */
                        /* do not consume the token yet */
                        break;
                }
            }

            /* free the value if and only if it does not belong to the table */
            if (key == NULL) {
                cif_value_free(value); /* ignore any error */
            }
        }

        if (key != NULL) {
            free(key);
        }
    } /* while */

    table_end:
    return result;
}

/*
 * Parses a value of any of the supported types.  The next token must represent a value, or the start of one.
 */
static int parse_value(struct scanner_s *scanner, cif_value_t **valuep) {
    int result = next_token(scanner);

    if (result == CIF_OK) {
        cif_value_t *value = *valuep;

        /* build a value object or modify the provided one, as appropriate */
        if ((value != NULL) || ((result = cif_value_create(CIF_UNK_KIND, &value)) == CIF_OK)) {
            UChar *token_value = scanner->tvalue_start;
            size_t token_length = scanner->tvalue_length;

            switch (scanner->ttype) {
                case OLIST: /* opening delimiter of a list value */
                    CONSUME_TOKEN(scanner);
                    result = parse_list(scanner, &value);
                    break;
                case OTABLE: /* opening delimiter of a table value */
                    CONSUME_TOKEN(scanner);
                    result = parse_table(scanner, &value);
                    break;
                case TVALUE:
                    /* parse text block contents into a value */
                    result = decode_text(scanner, token_value, token_length, &value);
                    CONSUME_TOKEN(scanner);  /* consume the token _after_ parsing its value */
                    break;
                case QVALUE:
                    /* overwrite the closing delimiter with a string terminator */
                    token_value[token_length - 1] = 0;
                    /* advance token start past the opening delimiter */
                    token_value += 1;
                    /* copy the input string into the value object */
                    result = cif_value_copy_char(value, token_value);
                    CONSUME_TOKEN(scanner);  /* consume the token _after_ parsing its value */
                   break;
                case VALUE:
                    /* special cases for unquoted question mark (?) and period (.) */
                    if (token_length == 1) {
                        if (*token_value == QUERY_CHAR) {
                            result = cif_value_init(value, CIF_UNK_KIND);
                            goto value_end;
                        }  else if (*token_value == DECIMAL_CHAR) {
                            result = cif_value_init(value, CIF_NA_KIND);
                            goto value_end;
                        } /* else an ordinary value */
                    }
        
                    if ((result = cif_value_parse_numb(value, token_value)) == CIF_INVALID_NUMBER) {
                        /* failed to parse the value as a number; record it as a string */
                        int32_t temp_length = token_length + 1;
                        UChar *temp = (UChar *) malloc((temp_length) * sizeof(UChar));

                        if (temp == NULL) {
                            result = CIF_ERROR;
                        } else {
                            /* copy the token value into a separate buffer, with terminator */
                            *temp = 0;
                            u_strncat(temp, token_value, temp_length);  /* ensures NUL termination */
                            /* assign the terminated string into the value object */
                            if ((result = cif_value_init_char(value, temp)) != CIF_OK) {
                                /* assignment failed; free the string */
                                free(temp);
                            }
                        }
                    }

                    value_end:
                    CONSUME_TOKEN(scanner);  /* consume the token _after_ parsing its value */
                    break;
                default:
                    /* This parse function should be called only when the incoming token is or starts a value */
                    result = CIF_INTERNAL_ERROR;
                    break;
            }

            if (result == CIF_OK) {
                *valuep = value;
            } else if (*valuep != value) {
                cif_value_free(value);  /* ignore any error */
            }
        }
    }

    return result;
}


/*
 * Decodes the contents of a text block by un-prefixing and unfolding lines as appropriate, and records the
 * result in a CIF value object.
 *
 * If dest points to a non-NULL value handle then the associated value object is modified
 * (on success) to be of kind CIF_CHAR_KIND, holding the parsed text.  Otherwise, a new value
 * object is created and its handle recorded where dest points.
 */
static int decode_text(struct scanner_s *scanner, UChar *text, int32_t text_length, cif_value_t **dest) {
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
                            switch(CLASS_OF(c, scanner)) {
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
                            /* recover by copying the un-prefixed text (no special action required for that) */
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
                                int class = CLASS_OF(c, scanner);

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

/*
 * Ensures that the provided scanner has the next available token identified and classified.
 *
 * The MAKE_COLON_SPECIAL macro activates recognition of KV_SEP tokens during (only) the next execution of this function
 */
static int next_token(struct scanner_s *scanner) {
    int result = CIF_OK;
    int last_ttype = scanner->ttype;

    /* any token may follow these without intervening whitespace: */
    int after_ws = ((last_ttype == END) || (last_ttype == OLIST) || (last_ttype == OTABLE) || (last_ttype == KV_SEP));

    while (scanner->text_start >= scanner->next_char) { /* else there is already a token waiting */
        enum token_type ttype;
        UChar c;
        int clazz;

        scanner->tvalue_start = scanner->text_start;
        scanner->tvalue_length = 0;
        NEXT_CHAR(scanner, c, result);
        if (result != CIF_OK) {
            break;
        }

        /* Require whitespace separation between most tokens, but not between keys and values (where it is forbidden) */
        clazz = CLASS_OF(c, scanner);
        if (!after_ws && (clazz != COLON_CLASS)) {
            switch (scanner->meta_class[clazz]) {
                case WS_META:
                case CLOSE_META:
                    /* do nothing: whitespace is never required before whitespace or closing brackets / braces */
                    break;
                default:
                    /* TODO: error missing whitespace */
                    /* recover by assuming the missing whitespace */
                    break;
            }
        }

        switch (clazz) {
            case WS_CLASS:
            case EOL_CLASS:
                result = scan_ws(scanner);
                /* FIXME: report to listener, if any */
                if (result == CIF_OK) {
                    CONSUME_TOKEN(scanner);
                    after_ws = CIF_TRUE;
                    continue;  /* cycle the LOOP */
                } else {
                    break;     /* break from the SWITCH */
                }
            case HASH_CLASS:
                result = scan_to_eol(scanner);
                /* FIXME: report to listener, if any */
                if (result == CIF_OK) {
                    CONSUME_TOKEN(scanner);
                    continue;  /* cycle the LOOP */
                } else {
                    break;     /* break from the SWITCH */
                }
            case EOF_CLASS:
                ttype = END;
                result = CIF_OK;
                break;
            case UNDERSC_CLASS:
                ttype = NAME;
                result = scan_to_ws(scanner);
                break;
            case OBRAK_CLASS:
                ttype = OLIST;
                scanner->tvalue_length = 1;
                break;
            case CBRAK_CLASS:
                ttype = CLIST;
                scanner->tvalue_length = 1;
                break;
            case OCURL_CLASS:
                ttype = OTABLE;
                scanner->tvalue_length = 1;
                break;
            case CCURL_CLASS:
                ttype = CTABLE;
                scanner->tvalue_length = 1;
                break;
            case QUOTE_CLASS:
                result = scan_delim_string(scanner);

                if (result == CIF_OK) {
                    /* determine whether this is a value or a table key */

                    /* peek ahead at the next character to see whether this looks like a table key */
                    PEEK_CHAR(scanner, c, result);

                    if (result == CIF_OK) {
                        if (CLASS_OF(c, scanner) == COLON_CLASS) {
                            scanner->next_char += 1;
                            ttype = KV_SEP;
                        } else {
                            ttype = QVALUE;
                        }

                        /* either way, the logical token value starts after the opening quote */
                        scanner->tvalue_start += 1;
                    }
                }
                break;
            case SEMI_CLASS:
                if (scanner->column == 1) {  /* semicolon was in column 1 */
                    result = scan_text(scanner);
                    scanner->tvalue_start += 1;
                    ttype = TVALUE;
                } else {
                    result = scan_unquoted(scanner);
                    ttype = VALUE;
                }
                break;
            case COLON_CLASS:
                /* note: whether any character has class COLON_CLASS depends on context */
                ttype = KV_SEP;
                scanner->tvalue_length = 1;
                MAKE_COLON_NORMAL(scanner);
                break;
            case DOLLAR_CLASS:
                /* TODO: error frame reference */
                /* recover by accepting it as a ws-delimited string */
                /* fall through */
            default:
                result = scan_unquoted(scanner);
                ttype = VALUE;
                break;
        }

        if (result == CIF_OK) {    
            if (ttype == VALUE) {

                /*
                 * Case-insensitive and locale- and codepage-independent detection of reserved words.
                 * It's a bit verbose, but straightforward.
                 */

                UChar *token = scanner->tvalue_start;
                size_t token_length = scanner->tvalue_length;

                if ((token_length > 4) && (CLASS_OF(token[4], scanner) == UNDERSC_CLASS)) {
                    /* check for data_*, save_*, loop_, and stop_ */
                    if (         (CLASS_OF(*token, scanner) == D_CLASS)
                            && (CLASS_OF(token[1], scanner) == A_CLASS)
                            && (CLASS_OF(token[2], scanner) == T_CLASS)
                            && (CLASS_OF(token[3], scanner) == A_CLASS)) {
                        if (token_length == 5) {
                            /* TODO: error missing block code / reserved word */
                            /* recover by dropping it */
                            CONSUME_TOKEN(scanner);
                        } else {
                            ttype = BLOCK_HEAD;
                            scanner->tvalue_start += 5;
                            scanner->tvalue_length -= 5;
                        }
                    } else if (  (CLASS_OF(*token, scanner) == S_CLASS)
                            && (CLASS_OF(token[1], scanner) == A_CLASS)
                            && (CLASS_OF(token[2], scanner) == V_CLASS)
                            && (CLASS_OF(token[3], scanner) == E_CLASS)) {
                        ttype = ((token_length == 5) ? FRAME_TERM : FRAME_HEAD);
                        scanner->tvalue_start += 5;
                        scanner->tvalue_length -= 5;
                    } else if ((token_length == 5)
                            && (CLASS_OF(token[2], scanner) == O_CLASS)
                            && (CLASS_OF(token[3], scanner) == P_CLASS)) {
                        if (         (CLASS_OF(*token, scanner) == L_CLASS)
                                && (CLASS_OF(token[1], scanner) == O_CLASS)) {
                            ttype = LOOPKW;
                            scanner->tvalue_start += 5;
                            scanner->tvalue_length -= 5;
                        } else if (  (CLASS_OF(*token, scanner) == S_CLASS)
                                && (CLASS_OF(token[1], scanner) == T_CLASS)) {
                            /* TODO: error 'stop' reserved word */
                            /* recover by dropping it */
                            CONSUME_TOKEN(scanner);
                        } 
                    }
                } else if ((token_length == 7) && (CLASS_OF(token[6], scanner) == UNDERSC_CLASS)
                        &&   (CLASS_OF(*token, scanner) == G_CLASS)
                        && (CLASS_OF(token[1], scanner) == L_CLASS)
                        && (CLASS_OF(token[2], scanner) == O_CLASS)
                        && (CLASS_OF(token[3], scanner) == B_CLASS)
                        && (CLASS_OF(token[4], scanner) == A_CLASS)
                        && (CLASS_OF(token[5], scanner) == L_CLASS)) {
                    /* TODO: error 'global' reserved word */
                    /* recover by ignoring it */
                    CONSUME_TOKEN(scanner);
                }
            } /* endif ttype == VALUE */
        } /* endif result == CIF_OK */
    } /* end while */

    MAKE_COLON_NORMAL(scanner);
    return result;
}

static int scan_ws(struct scanner_s *scanner) {
    int sol = 0;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        for (; scanner->next_char < top; scanner->next_char += 1) {
            UChar c = *(scanner->next_char);

            switch (CLASS_OF(c, scanner)) {
                case WS_CLASS:
                    /* increment the column number; c is assumed to not be a surrogate code value */
                    scanner->column += 1;
                    sol = 0;
                    break;
                case EOL_CLASS:
                    if (scanner->column > CIF_LINE_LENGTH) {
                        /* TODO: error long line */
                        /* recover by accepting it as-is */
                    }

                    /* the next character is at the start of a line; sol encodes data about the preceding terminators */
                    sol = ((sol << 2) + ((c == CR_CHAR) ? 2 : 1)) & 0xf;
                    /*
                     * sol code 0x9 == 1001b indicates that the last two characters were CR and LF.  In that case
                     * the line number was incremented for the CR, and should not be incremented again for the LF.
                     */
                    scanner->line += ((sol == 0x9) ? 0 : 1);
                    scanner->column = 0;
                    break;
                default:
                    goto end_of_token;
            }
        }

        GET_MORE_CHARS(scanner, result);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    scanner->tvalue_length = (scanner->next_char - scanner->tvalue_start);
    return CIF_OK;
}

static int scan_to_ws(struct scanner_s *scanner) {
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        for (; scanner->next_char < top; scanner->next_char += 1) {
            UChar c = *(scanner->next_char);
            int surrogate_mask;

            if (scanner->meta_class[CLASS_OF(c, scanner)] == WS_META) {
                goto end_of_token;
            }

            /* increment the column number, provided that c is not the trailing surrogate of a surrogate pair */
            surrogate_mask = c & 0xfc00;
            scanner->column += ((lead_surrogate && (surrogate_mask == 0xdc00)) ? 0 : 1);
            lead_surrogate = (surrogate_mask == 0xd800);
        }

        GET_MORE_CHARS(scanner, result);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    scanner->tvalue_length = (scanner->next_char - scanner->tvalue_start);
    return CIF_OK;
}

static int scan_to_eol(struct scanner_s *scanner) {
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        for (; scanner->next_char < top; scanner->next_char += 1) {
            UChar c = *(scanner->next_char);
            int surrogate_mask;

            switch (CLASS_OF(c, scanner)) {
                case EOL_CLASS:
                case EOF_CLASS:
                    goto end_of_token;
            }

            /* increment the column number, provided that c is not the trailing surrogate of a surrogate pair */
            surrogate_mask = c & 0xfc00;
            scanner->column += ((lead_surrogate && (surrogate_mask == 0xdc00)) ? 0 : 1);
            lead_surrogate = (surrogate_mask == 0xd800);
        }

        GET_MORE_CHARS(scanner, result);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    scanner->tvalue_length = (scanner->next_char - scanner->tvalue_start);
    return CIF_OK;
}

static int scan_unquoted(struct scanner_s *scanner) {
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        for (; scanner->next_char < top; scanner->next_char += 1) {
            UChar c = *(scanner->next_char);
            int surrogate_mask;

            switch (scanner->meta_class[CLASS_OF(c, scanner)]) {
                case OPEN_META:
                    /* TODO: error missing whitespace between values */
                    /* fall through */
                case CLOSE_META:
                case WS_META:
                    goto end_of_token;
            }

            /* increment the column number, provided that c is not the trailing surrogate of a surrogate pair */
            surrogate_mask = c & 0xfc00;
            scanner->column += ((lead_surrogate && (surrogate_mask == 0xdc00)) ? 0 : 1);
            lead_surrogate = (surrogate_mask == 0xd800);
        }

        GET_MORE_CHARS(scanner, result);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    scanner->tvalue_length = (scanner->next_char - scanner->tvalue_start);
    return CIF_OK;
}

/*
 * Scans a string delimited by matching characters, the opening one being at the current token start position.  The
 * scan will stop with a recoverable error if it reaches the end of the initial line.  The token start position is
 * advanced to immediately past the opening delimiter, and the resulting token length accounts for only the content,
 * not the delimiters.
 */
static int scan_delim_string(struct scanner_s *scanner) {
    UChar delim = *(scanner->text_start);
    int delim_size = 0;
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        while (scanner->next_char < top) {
            UChar c = *(scanner->next_char++);
            int surrogate_mask;

            /* increment the column number, provided that c is not the trailing surrogate of a surrogate pair */
            surrogate_mask = c & 0xfc00;
            scanner->column += ((lead_surrogate && (surrogate_mask == 0xdc00)) ? 0 : 1);
            lead_surrogate = (surrogate_mask == 0xd800);

            if (c == delim) {
                delim_size = 1;
                goto end_of_token;
            } else {
                switch (CLASS_OF(c, scanner)) {
                    case EOL_CLASS:
                    case EOF_CLASS:
                        /* handle error: unterminated quoted string */
                        /* recover by assuming a trailing close-quote */
                        goto end_of_token;
                }
            }
        }

        GET_MORE_CHARS(scanner, result);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    scanner->tvalue_start = scanner->text_start + 1;
    scanner->tvalue_length = (scanner->next_char - scanner->tvalue_start) - delim_size;
    return CIF_OK;
}

/*
 * Scans a text block.  Sets the token parameters to mark the block contents, exclusive of delimiters, assuming the
 * initial token start position to be at the leading semicolon.
 */
static int scan_text(struct scanner_s *scanner) {
    int delim_size = 0;
    int sol = 0;
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        while (scanner->next_char < top) {
            UChar c = *(scanner->next_char++);
            int surrogate_mask;

            /* increment the column number, provided that c is not the trailing surrogate of a surrogate pair */
            surrogate_mask = c & 0xfc00;
            scanner->column += ((lead_surrogate && (surrogate_mask == 0xdc00)) ? 0 : 1);
            lead_surrogate = (surrogate_mask == 0xd800);

            switch (CLASS_OF(c, scanner)) {
                case SEMI_CLASS:
                    if (sol != 0) {
                        delim_size = ((*(scanner->next_char - 2) == LF_CHAR)
                                && (*(scanner->next_char - 3) == CR_CHAR)) ? 3 : 2;
                        goto end_of_token;
                    }
                    break;
                case EOL_CLASS:
                    if (scanner->column > CIF_LINE_LENGTH) {
                        /* TODO: error long line */
                        /* recover by accepting it as-is */
                    }

                    /* the next character is at the start of a line */
                    sol = ((sol << 2) + ((c == CR_CHAR) ? 2 : 1)) & 0xf;
                    /*
                     * sol code 0x9 == 1001b indicates that the last two characters were CR and LF.  In that case
                     * the line number was incremented for the CR, and should not be incremented again for the LF.
                     */
                    scanner->line += ((sol == 0x9) ? 0 : 1);
                    scanner->column = 0;
                    break;
                case EOF_CLASS:
                    /* TODO: error unterminated text block */
                    /* recover by reporting out the whole tail as the token */
                    goto end_of_token;
                default:
                    sol = 0;
                    break;
            }
        }

        GET_MORE_CHARS(scanner, result);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    scanner->tvalue_start =  scanner->text_start + 1;
    scanner->tvalue_length = (scanner->next_char - scanner->tvalue_start) - delim_size;
    return CIF_OK;
}

/*
 * Transfers characters from the provided scanner's character source into its working character buffer, provided that
 * any are available.  May move unconsumed data within the buffer (adjusting the rest of the scanner's internal state
 * appropriately) and/or may increase the size of the buffer.  Will insert an EOF_CHAR into the buffer if called when
 * there are no more characters are available.  Returns CIF_OK if anny characters are transferred (including an EOF
 * marker), otherwise CIF_ERROR.
 */
static int get_more_chars(struct scanner_s *scanner) {
    size_t chars_read = scanner->next_char - scanner->buffer;
    size_t chars_consumed = scanner->text_start - scanner->buffer;
    ssize_t nread;

    assert(chars_read < SSIZE_T_MAX);
    assert(chars_consumed < chars_read);
    if (chars_consumed >= scanner->buffer_limit) {
        /* the buffer is empty; reset it to the beginning */
        assert(scanner->next_char == scanner->text_start);
        scanner->text_start = scanner->buffer;
        scanner->tvalue_start = scanner->buffer;
        scanner->next_char = scanner->buffer;
        scanner->buffer_limit = 0;
    } else if (scanner->buffer_size < (scanner->buffer_limit + BUF_MIN_FILL)) {
        /* need to make room at the top of the buffer */
        size_t current_chars = chars_read - chars_consumed;
        size_t tvalue_diff = scanner->tvalue_start - scanner->text_start;

        if (current_chars * 2 <= scanner->buffer_size) {
            /* move the current data to the front of the buffer */
            memmove(scanner->buffer, scanner->text_start, current_chars * sizeof(UChar));
        } else {
            /* extend the buffer */
            size_t new_size = scanner->buffer_size * 2;
            UChar *new_buffer = (UChar *) malloc(new_size * sizeof(UChar));

            if (new_buffer == NULL) {
                return CIF_ERROR;
            } else {
                /* preserve only the needed data */
                memcpy(new_buffer, scanner->text_start, current_chars * sizeof(UChar));

                /* assign the expanded buffer to the scanner */
                free(scanner->buffer);
                scanner->buffer = new_buffer;
                scanner->buffer_size = new_size;
            }
        }

        /* either way, update scanner state to reflect the changes */
        scanner->text_start = scanner->buffer;
        scanner->tvalue_start = scanner->buffer + tvalue_diff;
        scanner->next_char = scanner->buffer + current_chars;
        scanner->buffer_limit = current_chars;
    }

    /* once EOF has been detected, don't attempt to read from the character source any more */
    nread = scanner->at_eof ? 0 : scanner->read_func(scanner->char_source, scanner->buffer + scanner->buffer_limit,
                scanner->buffer_size - scanner->buffer_limit);

    if (nread < 0) {
        return CIF_ERROR;
    } else if (nread == 0) {
        scanner->buffer[scanner->buffer_limit] = EOF_CHAR;
        scanner->buffer_limit += 1;
        scanner->at_eof = CIF_TRUE;
        return CIF_OK;
    } else {
        scanner->buffer_limit += nread;
        return CIF_OK;
    }
}

