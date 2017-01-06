// lexer.cpp
// FireFly server lexical analyser module

#include "global.h"
#include "parse.h"
#include "grammar.h"
#include "string.h"
#include "array.h"

static int g_nLine = 0;
static const char* g_pLexSrc = NULL;
static const char* g_pLexPos = NULL;

static arrayp g_arTemporaries = NULL;

#define LEX_EOF '\0'

#define ADVANCE  (++g_pLexPos)
#define NEXTCHAR (g_pLexPos[0])
#define NEXTNEXT (g_pLexPos[1])

#define MATCH(ch,tok) if (NEXTCHAR == ch) { ADVANCE; return tok; }

#define ISALPHA(ch) ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
#define ISDIGIT(ch) (ch >= '0' && ch <= '9')
#define ISHEX(ch) ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))


struct keyword {
	const char* szName;
	int nToken;
};

static struct keyword keywords[] = {

    // Reserved words

    { "break", T_BREAK },
    { "case", T_CASE },
    { "catch", T_CATCH },
    { "continue", T_CONTINUE },
    { "default", T_DEFAULT },
    { "delete", T_DELETE },
    { "do", T_DO },
    { "else", T_ELSE },
    { "exit", T_EXIT },
    { "for", T_FOR },
    { "foreach", T_FOREACH },
    { "if", T_IF },
    { "in", T_IN },
    { "lock", T_LOCK },
    { "new", T_NEW },
    { "null", T_NULL },
    { "repeat", T_REPEAT },
    { "return", T_RETURN },
    { "switch", T_SWITCH },
    { "this", T_THIS },
    { "thread", T_THREAD },
    { "throw", T_THROW },
    { "try", T_TRY },
    { "until", T_UNTIL },
    { "while", T_WHILE },

    { 0, 0 }

};


int lex_line() {
	return g_nLine;
}

int lex_pos() {
	return g_pLexPos - g_pLexSrc;
}

void lex_init( const char* source )
{
	// Initialise lexer globals
	g_pLexPos = g_pLexSrc = source;
	g_nLine = 1;

	// Allocate a global array to hold all parser temporaries.
	// This is to make sure there can't be any memory leaks.
	g_arTemporaries = new array_t;
}

void lex_cleanup()
{
	// Release all temporaries. The compiler must call AddRef on
	// any of these that it wants to keep, before this happens.
	if( g_arTemporaries ) g_arTemporaries->Release();
	g_arTemporaries = NULL;
}

static stringp NewTempString()
{
	// Create the new string
	stringp str = new string_t(); // new

	// Add the string to the temporaries array
	g_arTemporaries->AppendItem(any_t(str)); // ref
	str->Release();

	return str;
}

static void LexSkipComment()
{
	// Skip to the end of the current line
	while (NEXTCHAR != '\n' && NEXTCHAR != LEX_EOF) ADVANCE;
}

static void LexSkipBlock()
{
    int depth = 1;
    while (NEXTCHAR != LEX_EOF) {
		if (NEXTCHAR == '\n') ++g_nLine;
        ADVANCE;
        // Check for nested comments.
        if (NEXTCHAR == '/' && NEXTNEXT == '*') {
			// Nested comment.
            ADVANCE;
			ADVANCE;
            depth++;
        } else if (NEXTCHAR == '*' && NEXTNEXT == '/') {
			// End of comment.
            ADVANCE;
            ADVANCE;
            if (--depth < 1) return;
        }
    }
}

static stringp LexIdent()
{
	stringp sResult = NewTempString();

    char szStuff[2];
    szStuff[1] = 0;

    while (ISALPHA(NEXTCHAR) || ISDIGIT(NEXTCHAR)) {
		szStuff[0] = NEXTCHAR;
        sResult->AppendCStr(szStuff);
        ADVANCE;
    }

    return sResult;
}

static stringp LexPath()
{
	stringp sResult = NewTempString();

	char szStuff[2];
	szStuff[0] = '$';
    szStuff[1] = 0;
	sResult->AppendCStr(szStuff);

	ADVANCE; // $

    while (ISALPHA(NEXTCHAR) || ISDIGIT(NEXTCHAR) || NEXTCHAR == '.' || NEXTCHAR == '-' || NEXTCHAR == '@') {
		szStuff[0] = NEXTCHAR;
        sResult->AppendCStr(szStuff);
        ADVANCE;
    }

    return sResult;
}

static stringp StringLit() {
	int i, sch;
	stringp str = NewTempString();

    char szStuff[2];
    szStuff[1] = 0;

	ADVANCE; // opening "

    while( NEXTCHAR != '"' && NEXTCHAR != LEX_EOF )
    {
        if( NEXTCHAR == '\\' )
        {
            ADVANCE;
            if( ISDIGIT(NEXTCHAR) )
            {
                int i = 0;
                while( ISDIGIT(NEXTCHAR) )
                {
                    i = i * 10 + (NEXTCHAR - 48);
                    ADVANCE;
                }
                sch = i % 256;
            }
            else
            {
                switch (NEXTCHAR) {
                    case '"': sch = '"';  break;
                    case 'n': sch = '\n'; break;
                    case 'r': sch = '\r'; break;
                    case 't': sch = '\t'; break;
                    case 'f': sch = '\f'; break;
                    case 'v': sch = '\v'; break;
                    case 'b': sch = '\b'; break;
                    case 'x':
                        ADVANCE;
                        i = 0;
                        if( ISDIGIT(NEXTCHAR) ) i = i * 16 + (NEXTCHAR - 48);
                        else if( NEXTCHAR >= 'a' ) i = i * 16 + (NEXTCHAR - 'a' + 10);
                        else i = i * 16 + (NEXTCHAR - 'A' + 10);
                        ADVANCE;
                        if( ISDIGIT(NEXTCHAR) ) i = i * 16 + (NEXTCHAR - 48);
                        else if( NEXTCHAR >= 'a' ) i = i * 16 + (NEXTCHAR - 'a' + 10);
                        else i = i * 16 + (NEXTCHAR - 'A' + 10);
                        sch = i % 256;
                        break;
                    default:
						sch = NEXTCHAR; break;
                }
                ADVANCE;
            }
        }
        else
        {
            sch = NEXTCHAR;
            ADVANCE;
        }
		szStuff[0] = sch;
		str->AppendCStr(szStuff);
    }

	if (NEXTCHAR == '"') ADVANCE; // closing "

	return str;
}

int yylex()
{
    for (;;) {
		switch(NEXTCHAR) {
			case '\0':
				return -1; // end of input for yacc.
            case '\n':
				ADVANCE;
				++g_nLine;
                continue;
            case ' ':
            case '\t':
            case '\f':
            case '\v':
            case '\r':
				ADVANCE;
                continue;
            case '+':
				ADVANCE;
                MATCH('=', T_PLUS_A);		// +=
                return T_PLUS;              // +
            case '-':
				ADVANCE;
                MATCH('=', T_MINUS_A);      // -=
                MATCH('>', T_ARROW);        // ->
                return T_MINUS;             // -
            case '*':
				ADVANCE;
                MATCH('=', T_STAR_A);       // *=
                return T_STAR;              // *
            case '/':
				ADVANCE;
                if( NEXTCHAR == '/' ) { LexSkipComment(); continue; }
                if( NEXTCHAR == '*' ) { LexSkipBlock(); continue; }
                MATCH('=', T_SLASH_A);      // /=
                return T_SLASH;             // /
            case '%':
				ADVANCE;
                MATCH('=', T_PERCENT_A);    // %=
                return T_PERCENT;           // %
            case '^':
				ADVANCE;
                MATCH('=', T_CARET_A);      // ^=
                return T_CARET;             // ^
            case '&':
				ADVANCE;
                MATCH('&', T_AND);          // &&
                MATCH('=', T_AMPER_A);      // &=
                return T_AMPER;             // &
            case '|':
				ADVANCE;
                MATCH('|', T_OR);           // ||
                MATCH('=', T_BAR_A);        // |=
                return T_BAR;               // |
            case '#':
				ADVANCE;
                MATCH('=', T_HASH_A);       // #=
                return T_HASH;              // #
            case '>':
				ADVANCE;
                if (NEXTCHAR == '>') {
                    ADVANCE;
                    MATCH('=', T_SHR_A);    // >>=
                    return T_SHR;           // >>
                }
                MATCH('=', T_GREATER_EQ);   // >=
                return T_GREATER;           // >
            case '<':
				ADVANCE;
                if (NEXTCHAR == '<') {
                    ADVANCE;
                    MATCH('=', T_SHL_A);    // <<=
                    return T_SHL;           // <<
                }
                MATCH('=', T_LESS_EQ);      // <=
                return T_LESS;              // <
            case '~':
				ADVANCE;
                return T_TILDE;             // ~
            case '=':
				ADVANCE;
                MATCH('=', T_EQUAL);        // ==
                return T_ASSIGN;            // =
            case '.':
				if (NEXTNEXT == '.') {
					ADVANCE;
					ADVANCE;
					return T_RANGE;
				}
                if (ISDIGIT(NEXTNEXT)) {
					double number = 0.0;
					int len = 0;
					if (sscanf( g_pLexPos, "%lf%n", &number, &len ) == 1) {
						g_pLexPos += len;
					}
					yylval.f = float64(number);
					return T_FLOAT;
                }
				ADVANCE;
                return T_DOT;
            case ':':
				ADVANCE;
                MATCH(':', T_INHERIT);      // ::
                return T_COLON;             // :
            case '$':
                yylval.s = LexPath();
                return T_PATH;
            case '!':
				ADVANCE;
                MATCH('=', T_NOT_EQ);       // !=
                return T_BANG;              // !
            case ',':
				ADVANCE;
                return T_COMMA;             // ,
            case ';':
				ADVANCE;
                return T_SEMI;              // ;
            case '(':
				ADVANCE;
                return T_LPAREN;            // (
            case ')':
				ADVANCE;
                return T_RPAREN;            // )
            case '{':
				ADVANCE;
                return T_LBRACE;            // {
            case '}':
				ADVANCE;
                return T_RBRACE;            // }
            case '[':
				ADVANCE;
                return T_LBRACK;            // [
            case ']':
				ADVANCE;
                return T_RBRACK;            // ]
            case '"':
                yylval.s = StringLit();
                return T_STRING;
            default:
                // Check for a number literal
                if (ISDIGIT(NEXTCHAR)) {
					double number = 0.0;
					int len = 0;
					if (sscanf( g_pLexPos, "%lf%n", &number, &len ) == 1) {
						if (len && g_pLexPos[len-1] == '.') --len; // exclude final dot.
						g_pLexPos += len;
					}
					int32 intVal = int32(number);
					if (intVal == number) {
						yylval.n = intVal;
						return T_INTEGER;
					} else {
						yylval.f = float64(number);
						return T_FLOAT;
					}
                }
                // Check for an identifier or keyword
                if (ISALPHA(NEXTCHAR)) {
					int i;
                    yylval.s = LexIdent();
                    // Check keyword list for match
                    for( i = 0; keywords[i].szName; i++ )
					{
                        if( yylval.s->CompareCStr( keywords[i].szName ) == 0 )
                        {
//printf( "lexer: keyword -- %s\n", keywords[i].szName );
                            // leave the string attached // yylval.s = NULL;
                            return keywords[i].nToken;
                        }
                    }
                    return T_IDENT;
                }

				// Always consume one character.
				ADVANCE;
                return T_UNKNOWN;
        }
    }
}
