/*
 * Because CIF 2.0 is fundamentally an 8-bit binary format (always representing characters as their Unicode code point
 * values encoded according to UTF-8) it is essential that the scanner handle input bytes with their high bit set.  It
 * also must not attempt to perform its own case folding, as that is a locale and code page sensitive operation that
 * does not produce the same result on every system.
 */

%option 8bit
%option nocase-insensitive

/*
 * Disabling the default rule and enabling warnings helps the scanner generator (i.e. flex) alert about flaws in
 * the rule set.
 */
%option nodefault
%option warn

/*
 * The "full" table representation is expected to yield the fastest scanner for the specified rule set.
 */
%option full

/*
 * The remaining options represent implementation choices with no special justification.  They could readily be
 * altered to adapt the defined scanner to a different usage model.
 */
%option reentrant
%option noyywrap
%option bison-bridge
%option bison-locations

%{
#include "cif_parser.tab.h"

/*
 * The scanner leverages the parser's error reporting function for its own error reporting purposes; here is its
 * prototype.
 */
void yyerror(YYLTYPE *locp, FILE *input_file, void *scanner, const char *message);
%}

/*
 * The start conditions used in this scanner track progress through the input; though the current start condition
 * may be re-asserted (as in fact happens frequently), previous start conditions are never revisited.
 */

%s POSTBOM
%s BODY
%s EOFWS

/*
 * Bytes are expressed as octal codes to avoid sensitivity to the compiler's code page.  CIF 2.0 must always be encoded
 * in UTF-8, regardless of compiler code page or system default locale.  Note, however, that this scanner does not
 * validate multibyte UTF-8 code sequences or test for disallowed characters outside the Unicode Basic Latin set,
 * though it does flag bytes that never occur in UTF-8-encoded streams.
 */

/*
 * Flex's standard character class syntax is not useful here because octal and hexadecimal escapes are not recognized
 * within it.  Flex takes a lot longer to process rule sets involving long, multi-alternative patterns such as the
 * following, but the generated scanner is not adversely affected.
 */

/* C0 controls not used by CIF; these do not appear in well-formed CIF documents */
C0    \000|\001|\002|\003|\004|\005|\006|\007|\010|\013|\014|\016|\017|\020|\021|\022|\023|\024|\025|\026|\027|\030|\031|\032|\033|\034|\035|\036|\037

/* ASCII punctuation marks that are not significant to CIF, plus colon */
PUNC  \041|\045|\046|\050|\051|\052|\053|\054|\055|\056|\057|\072|\074|\075|\076|\077|\100|\134|\136|\140|\174|\176

/* ASCII digits */
DIGIT \060|\061|\062|\063|\064|\065|\066|\067|\070|\071

/* ASCII uppercase letters */
UPPER \101|\102|\103|\104|\105|\106|\107|\110|\111|\112|\113|\114|\115|\116|\117|\120|\121|\122|\123|\124|\125|\126|\127|\130|\131|\132

/* ASCII lowercase letters */
LOWER \141|\142|\143|\144|\145|\146|\147|\150|\151|\152|\153|\154|\155|\156|\157|\160|\161|\162|\163|\164|\165|\166|\167|\170|\171|\172

/* byte values having the high bit set */
CONT1 \200|\201|\202|\203|\204|\205|\206|\207|\210|\211|\212|\213|\214|\215|\216|\217|\220|\221|\222|\223|\224|\225|\226|\227|\230|\231|\232|\233|\234|\235|\236|\237
CONT2 \240|\241|\242|\243|\244|\245|\246|\247|\250|\251|\252|\253|\254|\255|\256|\257|\260|\261|\262|\263|\264|\265|\266|\267|\270|\271|\272|\273|\274|\275|\276|\277
LEAD2 \302|\303|\304|\305|\306|\307|\310|\311|\312|\313|\314|\315|\316|\317|\320|\321|\322|\323|\324|\325|\326|\327|\330|\331|\332|\333|\334|\335|\336|\337
LEAD3 \340|\341|\342|\343|\344|\345|\346|\347|\350|\351|\352|\353|\354|\355|\356|\357
LEAD4 \360|\361|\362|\363|\364
INVAL \300|\301|\365|\366|\367|\370|\371|\372|\373|\374|\375|\376|\377

/* various forms of whitespace. It is not necessary for these paterns to match the two-byte sequence \015\012 */
BL    \040|\011
EOL   \012|\015
WS    {BL}|{EOL}

/* punctuation marks having significance in CIF */
DQ    \042
HASH  \043
DOLAR \044
SQ    \047
COLON \072
SEMI  \073
OBRAK \133
CBRAK \135
UNDER \137
OCURL \173
CCURL \175

/* particular ASCII letters having (case-insensitive) significance in certain CIF contexts */
A     \101|\141
B     \102|\142
D     \104|\144
E     \105|\145
G     \107|\147
L     \114|\154
O     \117|\157
P     \120|\160
S     \123|\163
T     \124|\164
V     \126|\166

/* broader character categories built on those above */
  /* PLAIN bytes can start bare data values and have no special significance in CIF */
PLAIN {PUNC}|{DIGIT}|{UPPER}|{LOWER}|{CONT1}|{CONT2}|{LEAD2}|{LEAD3}|{LEAD4}
  /* VALCH bytes can appear in bare data values, after the first byte */
VALCH {PLAIN}|{HASH}|{UNDER}|{SQ}|{DQ}|{DOLAR}|{SEMI}
  /* NOTSQ bytes can appear inside a data value delimited by the {SQ} byte */
NOTSQ {PLAIN}|{HASH}|{UNDER}|{DQ}|{DOLAR}|{SEMI}|{OBRAK}|{CBRAK}|{OCURL}|{CCURL}|{BL}
  /* NOTDQ bytes can appear inside a data value delimited by the {DQ} byte */
NOTDQ {PLAIN}|{HASH}|{UNDER}|{SQ}|{DOLAR}|{SEMI}|{OBRAK}|{CBRAK}|{OCURL}|{CCURL}|{BL}
  /* NSEMI bytes can appear immediately following an {EOL} in a text field, without delimiting the field */
NSEMI {PLAIN}|{HASH}|{UNDER}|{SQ}|{DQ}|{DOLAR}|{OBRAK}|{CBRAK}|{OCURL}|{CCURL}|{BL}
  /* NONWS bytes can be any allowed byte that does not constitute whitespace */
NONWS {VALCH}|{OBRAK}|{CBRAK}|{OCURL}|{CCURL}
  /* NEOL bytes can be any allowed byte that does not terminate CIF lines */
NEOL  {NONWS}|{BL}


/*
 * Useful larger-scale definitions
 */

/* A UTF-8-encoded BOM == 0xefbbbf */
BOM   \357\273\277

/* patterns related to text fields */
TSTART {EOL}{SEMI}{NEOL}*
TCONT {EOL}({NSEMI}{NEOL}*)?
TEND  {EOL}{SEMI}

/* UTF-8 encoding of "#\#CIF_2.0": */
V2    {HASH}\134{HASH}\103\111\106{UNDER}\062\056\060


%{
#define RETURN(x) do { BEGIN(BODY); return (x); } while(0)
#define ERROR(x)  do { \
    BEGIN(BODY); \
    yyerror(NULL, yyget_in(yyscanner), yyscanner, "CIF syntax error"); \
    return -1; \
} while(0)
%}

%%

    /* file identification */
<INITIAL>{BOM}{VALCH}*            yyless(3); BEGIN(POSTBOM); /* consume a leading byte-order mark, if present */
<INITIAL,POSTBOM>{V2}({BL}{NEOL}*)?  RETURN(VCOMMENT); /* CIF 2.0 version comment */

    /* structural elements */
{D}{A}{T}{A}{UNDER}{NONWS}+       RETURN(BLOCK_HEADER);
{S}{A}{V}{E}{UNDER}{NONWS}+       RETURN(FRAME_HEADER);
{S}{A}{V}{E}{UNDER}               RETURN(FRAME_TERMINATOR);
{L}{O}{O}{P}{UNDER}               RETURN(LOOP);
{UNDER}{NONWS}+                   RETURN(DATANAME); /* data name */

    /* list delimiters */
{OBRAK}                           RETURN(LIST_OPEN);
{CBRAK}                           RETURN(LIST_CLOSE);

    /* table delimiters and keys */
{OCURL}                           RETURN(TABLE_OPEN);
{CCURL}                           RETURN(TABLE_CLOSE);
{SQ}{NOTSQ}*{SQ}{COLON}           |
{DQ}{NOTDQ}*{DQ}{COLON}           RETURN(KEY); /* table key */

    /* quoted values (including malformed ones) */
{SQ}{NOTSQ}*{SQ}                  |
{DQ}{NOTDQ}*{DQ}                  RETURN(SIMPLE_VALUE); /* in-line quoted value */
{SQ}{NOTSQ}*                      ERROR(SIMPLE_VALUE);  /* error: unterminated sq-string */
{DQ}{NOTDQ}*                      ERROR(SIMPLE_VALUE);  /* error: unterminated dq-string */
{TSTART}{TCONT}*{TEND}            RETURN(TEXT_FIELD);
{TSTART}{TCONT}*                  ERROR(TEXT_FIELD);    /* error: unterminated text field */
    /* The following qualified text field rules are not strictly necessary, as a text field cannot be valid in the
     * specified start conditions, but correctly matching text fields here leaves room for more effective error
     * recovery strategies to be defined than otherwise would be possible. */
<INITIAL,POSTBOM>{SEMI}{NEOL}*{TCONT}*{TEND}  RETURN(TEXT_FIELD);
<INITIAL,POSTBOM>{SEMI}{NEOL}*{TCONT}*        ERROR(TEXT_FIELD);  /* error: unterminated text field */

    /* bare values and reserved words */
{UNDER}                           ERROR(SIMPLE_VALUE);  /* error: invalid bare value */
{DOLAR}{NONWS}*                   ERROR(SIMPLE_VALUE);  /* error: frame reference (reserved) */
{D}{A}{T}{A}{UNDER}               ERROR(SIMPLE_VALUE);  /* error: reserved word data_ */
{G}{L}{O}{B}{A}{L}{UNDER}         ERROR(SIMPLE_VALUE);  /* error: reserved word global_ */
{S}{T}{O}{P}{UNDER}               ERROR(SIMPLE_VALUE);  /* error: reserved word stop_ */
({C0}|\177|{INVAL})+              ERROR(SIMPLE_VALUE);  /* error: disallowed characters */
{G}{L}{O}{B}{A}{L}{UNDER}{VALCH}+ |
{L}{O}{O}{P}{UNDER}{VALCH}+       |
{S}{T}{O}{P}{UNDER}{VALCH}+       |
    /* The following rule will not fire for text field delimiters because the appropriate whole-text-field rule above
     * appears first and will match at least as many bytes. */
({PLAIN}|{SEMI}){VALCH}*          RETURN(SIMPLE_VALUE); /* bare value */

    /* whitespace and whitespace-related malformations */
    /* whitespace is significant in CIF, so this scanner cannot simply consume it */
<EOFWS>{BL}+({HASH}{NEOL}*)?      return WS;  /* return without changing the start state */
{BL}+({HASH}{NEOL}*)?             RETURN(WS);
{EOL}({HASH}{NEOL}*)?             RETURN(WS);
<INITIAL,POSTBOM>{HASH}{NEOL}*    RETURN(WS); /* leading ordinary comment */
{HASH}{NEOL}*                     ERROR(WS);  /* syntax error: missing whitespace */
    /* insert synthetic whitespace at EOF, except for completely empty CIFs: */
<BODY><<EOF>>                     BEGIN(EOFWS); yy_scan_string("\040", yyscanner);

%%


