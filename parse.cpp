// parse.cpp
// FireFly server parsing module

#include "global.h"
#include "parse.h"
#include "grammar.h"
#include "opcode.h"
#include "runtime.h"
#include "string.h"
#include "function.h"
#include "array.h"
#include "any_impl.h"

#define MAX_BREAKS 512
#define MAX_CONTINUES 512
#define MAX_CONTEXTS 256

#define CTX_LOOP		0
#define CTX_IF			1
#define CTX_SWITCH		2
#define CTX_INVOKE		3
#define CTX_LIST		4
#define CTX_CATCH		5
#define CTX_ATTRIB		6
#define CTX_MAPPING     7

#define PARSECODE_NORMAL	0	// value only, or name-value pair
#define PARSECODE_SELECT	1	// select object clause
#define PARSECODE_INHERIT	2	// inherit object clause

typedef struct reg_s {
    stringp sName;
    int32 nType;    // register type (args,local,temp,mapping;lvalue;invoke)
    int32 nInUse;   // reference count for this register
} reg_t;

typedef struct context_s {
    int32 nType;
    int32 nBreaks;
    int32 nContinues;
	uint32 nNextPass;
	uint32 nPatch1;
	uint32 nPatch2;
	any_t mValue;
} context_t;

typedef struct predef_s
{
	int nOp;
	const char* szName;
	int nMinArgs;
	int nMaxArgs;
	bool bReturn;
} predef_t;

static bool g_importing;
static objectp g_importObj;
static functionp g_funcResult;
static stringp g_sErrors;

static stringp g_sLexSource;
static stringp g_sParseName;

// function metadata.
static int32 g_nFuncLine;
static int32 g_nFuncPos;
static int32 g_nLocals;
static int32 g_nArgs;
static reg_t g_pLocals[MAX_LOCALS];

// contexts.
static int32 g_nBreaks;
static int32 g_nContinues;
static int32 g_nContexts;
static int32 g_pBreaks[MAX_BREAKS];
static int32 g_pContinues[MAX_CONTINUES];
static context_t g_pContext[MAX_CONTEXTS];

// code generator.
static int32 g_nEmitPos;
static int32 g_nLinePos;
static uint32* g_pCode;
static int32* g_pLines;
static arrayp g_arConstants;

predef_t g_pPredef[] = {
	{ PF_VERSION, "version", 0, 0, true },
	{ PF_SLEEP, "sleep", 1, 1, false },
	{ PF_SIZEOF, "sizeof", 1, 1, true },

	{ PF_SIZEOF, "length", 1, 1, true },
	{ PF_TRIM, "trim", 1, 2, true }, // trim( str, ?? )
	{ PF_LTRIM, "ltrim", 1, 2, true },
	{ PF_RTRIM, "rtrim", 1, 2, true },
	{ PF_LOWER, "lower_case", 1, 1, true },
	{ PF_UPPER, "upper_case", 1, 1, true },
	{ PF_CAPITALISE, "capitalise", 1, 1, true },
	{ PF_TO_STR, "to_string", 1, 1, true },
	{ PF_TO_INT, "to_int", 1, 1, true },
	{ PF_TO_FLOAT, "to_float", 1, 1, true },
	{ PF_SEARCH, "search", 2, 3, true }, // search( str, find [,start] )
	{ PF_RSEARCH, "rsearch", 2, 3, true }, // rsearch( str, find [,start] )
	{ PF_REPLACE, "replace", 3, 3, true }, // replace( str, find, replace )
	{ PF_REVERSE, "reverse", 1, 1, true },
	{ PF_SPLIT, "split", 1, 4, true }, // split( str, [delim(" ")], [maximum(0)], [collapse(0)] )
	{ PF_JOIN, "join", 1, 3, true }, // join( array, [delim(" ")] [final_delim(null)] )
	{ PF_SKIP, "skip", 1, 3, true }, // skip( str [,start(0) [,charset(whitespace)]] )
	{ PF_SKIPTO, "skipto", 1, 3, true }, // skipto( str [,start(0) [,charset(whitespace)]] )
	{ PF_CRYPT, "crypt", 1, 2, true }, // crypt( str, salt )

	{ PF_KEYS, "keys", 1, 1, true },
	{ PF_VALUES, "values", 1, 1, true },
	{ PF_EXISTS, "exists", 2, 2, true }, // exists( obj, key )
	{ PF_OWNER, "owner", 1, 1, true },
	{ PF_PARENT, "parent", 1, 1, true },
	{ PF_ACCESS, "access", 1, 1, true },
	{ PF_DEF_ACCESS, "default_access", 1, 1, true },
	{ PF_SET_OWNER, "set_owner", 2, 2, false },
	{ PF_SET_ACCESS, "set_access", 2, 2, false },
	{ PF_SET_DEF_ACCESS, "set_default_access", 2, 2, false },
	{ PF_PARSE, "parse", 1, 2, true }, // parse( str, start ) = [ value, err$, start% ]
	{ PF_TOKEN, "token", 1, 2, true },
	{ PF_ENCODE, "encode", 1, 1, true },
	{ PF_FUNCTION_NAME, "function_name", 1, 1, true },
	{ PF_FUNCTION_CONTEXT, "function_context", 1, 1, true },
	{ PF_REMOVE, "remove", 2, 2, false }, // remove( obj, key ) ; remove( arr, val )

	{ PF_LISTEN, "listen", 1, 1, true },
	{ PF_ACCEPT, "accept", 1, 1, true },
	{ PF_WRITE, "write", 2, 2, false },
	{ PF_WRITE_TEXT, "write_text", 2, 2, false },
	{ PF_READ, "read", 2, 2, true },
	{ PF_READLN, "read_line", 1, 1, true },
	{ PF_CLOSE, "close", 1, 1, false },
	{ PF_CONNECT, "connect", 1, 2, true },
	{ PF_PORT_OPEN, "is_open", 1, 1, true },

	{ PF_OBJECT_PATH, "object_path", 1, 1, true },
	{ PF_FIND_OBJECT, "find_object", 1, 2, true },
	{ PF_UNIQUE_NAME, "unique_name", 1, 2, true },
	{ PF_ADD_INHERIT, "add_inherit", 2, 2, false },
	{ PF_REMOVE_INHERIT, "remove_inherit", 2, 2, false },
	{ PF_INHERIT_LIST, "inherit_list", 1, 1, true },

	{ PF_KILL_THREAD, "kill_thread", 1, 1, false },
	{ PF_EXIT_VALUE, "exit_value", 1, 1, true },
	{ PF_THREAD_STOPPED, "is_stopped", 1, 1, true },
	{ PF_THREAD_BLOCKED, "is_blocked", 1, 1, true },
	{ PF_THREAD_LIST, "threads", 0, 0, true },
	{ PF_THREAD_STACK, "thread_stack", 1, 1, true },
	{ PF_THREAD_FUNCTION, "thread_function", 1, 1, true },
	{ PF_THREAD_STATE, "thread_state", 1, 1, true },
	{ PF_YIELD, "yield", 0, 0, false },

	{ PF_UNDEFINEDP, "undefinedp", 1, 1, true },
	{ PF_NULLP, "nullp", 1, 1, true },
	{ PF_INTEGERP, "integerp", 1, 1, true },
	{ PF_REALP, "realp", 1, 1, true },
	{ PF_NUMBERP, "numberp", 1, 1, true },
	{ PF_RANGEP, "rangep", 1, 1, true },
	{ PF_STRINGP, "stringp", 1, 1, true },
	{ PF_ARRAYP, "arrayp", 1, 1, true },
	{ PF_MAPPINGP, "mappingp", 1, 1, true },
	{ PF_OBJECTP, "objectp", 1, 1, true },
	{ PF_FUNCTIONP, "functionp", 1, 1, true },
	{ PF_PORTP, "portp", 1, 1, true },
	{ PF_THREADP, "threadp", 1, 1, true },
	{ PF_DELETEDP, "deletedp", 1, 1, true },
	{ PF_CLONEP, "clonep", 1, 1, true },
	{ PF_CHILDP, "childp", 1, 1, true },

	{ PF_SHUTDOWN, "shutdown", 0, 0, false },
	{ PF_RANDOM, "random", 1, 1, true },
	{ PF_CONSOLE, "console", 1, 1, false },

	{ PF_TIME, "time", 0, 0, true },
	{ PF_CTIME, "ctime", 0, 1, true },
	{ PF_STRFTIME, "strftime", 1, 2, true },
	{ PF_LOCALTIME, "localtime", 0, 1, true },
	{ PF_UTCTIME, "utctime", 0, 1, true },

	{ PF_MOVE_OBJECT, "move_object", 3, 3, false }, // move_object(obj, parent, name)
	{ PF_OBJECT_NAME, "object_name", 1, 1, true },  // object_name(obj)
	{ PF_APPEND, "append", 2, 2, false },           // append(array, item)
	{ PF_SLICE, "slice", 2, 3, true },              // slice(sring, start, [end])

	{ 0, NULL, 0, 0, false }
};

const char *g_szOpcodes[] = {
    "OP_NULL",
    "OP_ASSIGN",
    "OP_ADD",
    "OP_SUB",
    "OP_MUL",
    "OP_DIV",
    "OP_MOD",
    "OP_AND",
    "OP_OR",
    "OP_XOR",
    "OP_SHR",
    "OP_SHL",
    "OP_POW",
    "OP_EQ",
    "OP_NEQ",
    "OP_GT",
    "OP_LT",
    "OP_GE",
    "OP_LE",
    "OP_IF",
    "OP_IFZ",
    "OP_GOTO",
    "OP_RETURN",
    "OP_EXIT",
    "OP_DELETE",
    "OP_INVOKE",
    "OP_THREAD",
    "OP_RANGE",
    "OP_RANGE_U",
    "OP_RANGE_D",
    "OP_ARRAY",
    "OP_MAPPING",
    "OP_INDEX",
    "OP_CATCH",
    "OP_THROW",
    "OP_LOCK",
    "OP_UNLOCK",
    "OP_FOREACH",
    "OP_THIS",
    "OP_ACCESS",
    "OP_LOOKUP",
    "OP_NOT",
    "OP_BITNOT",
    "OP_NEG",
    "OP_CONST",
    "OP_SETNULL",
    "OP_NEXTEACH",
    "OP_SWITCH",
    "OP_NEW_OBJECT",
    "OP_NEW_ARRAY",
    "OP_NEW_MAPPING",
    "OP_COPY",
    "PF_VERSION",
    "PF_SLEEP",
    "PF_SIZEOF",
    "PF_TRIM",
    "PF_LTRIM",
    "PF_RTRIM",
    "PF_LOWER",
    "PF_UPPER",
    "PF_CAPITALISE",
    "PF_TO_STR",
    "PF_TO_INT",
    "PF_TO_FLOAT",
    "PF_SEARCH",
    "PF_REVERSE",
    "PF_KEYS",
    "PF_VALUES",
    "PF_OWNER",
    "PF_ACCESS",
    "PF_DEF_ACCESS",
    "PF_SET_OWNER",
    "PF_SET_ACCESS",
    "PF_SET_DEF_ACCESS",
    "PF_PARSE",
    "PF_ENCODE",
    "PF_FUNCTION_NAME",
    "PF_LISTEN",
    "PF_ACCEPT",
    "PF_WRITE",
    "PF_READ",
    "PF_READLN",
    "PF_CLOSE",
    "PF_CONNECT",
    "PF_PORT_OPEN",
    "PF_ADD_INHERIT",
    "PF_REMOVE_INHERIT",
    "PF_INHERITS",
    "PF_INHERIT_LIST",
    "PF_KILL_THREAD",
    "PF_EXIT_VALUE",
    "PF_THREAD_STOPPED",
    "PF_THREAD_BLOCKED",
    "PF_UNDEFINEDP",
    "PF_NULLP",
    "PF_INTEGERP",
    "PF_REALP",
    "PF_NUMBERP",
    "PF_RANGEP",
    "PF_STRINGP",
    "PF_ARRAYP",
    "PF_MAPPINGP",
    "PF_OBJECTP",
    "PF_FUNCTIONP",
    "PF_PORTP",
    "PF_THREADP",
    "PF_COPY",
    "PF_DEEP_COPY",
    "PF_PARENT",
    "PF_REMOVE",
    "PF_SHUTDOWN",
    "PF_RANDOM",
    "PF_FUNCTION_CONTEXT",
	"PF_REPLACE",
	"PF_EXISTS",
	"PF_CONSOLE",
	"PF_WRITE_TEXT",
	"PF_SPLIT",
	"PF_JOIN",
	"PF_OBJECT_PATH",
	"PF_FIND_OBJECT",
	"PF_UNIQUE_NAME",
	"PF_TOKEN",
	"PF_SKIP",
	"PF_RSEARCH",
	"OP_SETROOT",
	"PF_CRYPT",
	"PF_DELETEDP",
	"PF_CLONEP",
	"PF_CHILDP",
	"PF_TIME",
	"PF_CTIME",
	"PF_STRFTIME",
	"PF_LOCALTIME",
	"PF_UTCTIME",
	"PF_THREAD_LIST",
	"PF_THREAD_STACK",
	"PF_THREAD_FUNCTION",
	"PF_THREAD_STATE",
	"PF_YIELD",
	"PF_MOVE_OBJECT",
	"PF_OBJECT_NAME",
	"PF_SKIPTO",
	"PF_APPEND",
	"PF_SLICE"
};

static void EmitOp( byte op, byte a, byte b, byte c );
static void EmitOpD( byte op, byte a, unsigned short d );
static void PopContext();
static void DebugDump();

#define EMITOP(OP, A, B, C) EmitOp(byte(OP), byte(A), byte(B), byte(C))
#define EMITOPD(OP, A, D) EmitOpD(byte(OP), byte(A), (unsigned short)(D))
#define EPATCHD(P, D) (((unsigned short*)&g_pCode[P])[1] = (unsigned short)(D))
#define EPATCHA(P, A) (((byte*)&g_pCode[P])[1] = byte(A))

#define THIS_CONTEXT (g_pContext[g_nContexts - 1])


// Public interface

static void AddError( const char *szMessage, int32 nLine )
{
	static char szError[256];
	sprintf( szError, "error at line %d: %s\n", (int)lex_line(), szMessage );

	if (!g_sErrors) g_sErrors = new string_t();
	g_sErrors->AppendCStr( szError );
}

static void AddWarning( const char *szMessage, int32 nLine )
{
	static char szError[256];
	sprintf( szError, "warning at line %d: %s\n", (int)lex_line(), szMessage );

	if (!g_sErrors) g_sErrors = new string_t();
	g_sErrors->AppendCStr( szError );
}

void yyerror( const char *yymessage )
{
	AddError( yymessage, lex_line() );
}


void yywarn( const char *yymessage )
{
	AddWarning( yymessage, lex_line() );
}

RESULT Parse( stringp sSource, bool importing, any_t& mResult )
{
	mResult.AssignNull();

	g_importing = importing;
	g_importObj = importing ? g_oDatabase.RootObject() : NULL;
	g_sLexSource = sSource; // borrow

	// Initialise the lexical analyser.
	lex_init( sSource->CString() );

	// Initialise globals that may be used for function parsing.
	g_sErrors = NULL;
	g_funcResult = NULL;
	g_sParseName = NULL;
	g_arConstants = NULL;
    g_pCode = NULL;
    g_pLines = NULL;
	g_nContexts = 0;

	// Attempt to parse the function or import script.
	if (yyparse() == 0 && !g_sErrors) {
		if (!importing && g_funcResult) {
			// No errors compiling a function.
			mResult.AssignRef(any_t(g_funcResult)); // ownership
			g_funcResult = NULL;
		}
	}

	// Release function result if we did not use it.
	if (g_funcResult) {
		g_funcResult->Release();
		g_funcResult = NULL;
	}

	// Return the error string if there were errors.
	if (g_sErrors) {
		mResult.AssignRef(any_t(g_sErrors)); // ownership
	}

	// Clean up globals that might be left over.
	if (g_arConstants) g_arConstants->Release();
    if (g_pCode) FREE(g_pCode);
    if (g_pLines) FREE(g_pLines);
	while (g_nContexts) {
		THIS_CONTEXT.nBreaks = 0;
		THIS_CONTEXT.nContinues = 0;
		PopContext();
	}

	// Clean up the lexical analyser.
	lex_cleanup();

	return E_SUCCESS;
}

static void PushContext( int nType )
{
    if( g_nContexts >= MAX_CONTEXTS )
    {
        AddError( "maximum context nesting depth reached", lex_line() );
		return;
    }

	// Initialise the new context frame
	g_pContext[g_nContexts].nType = nType;
    g_pContext[g_nContexts].nBreaks = 0;
    g_pContext[g_nContexts].nContinues = 0;
    g_pContext[g_nContexts].nNextPass = g_nEmitPos;
	g_pContext[g_nContexts].nPatch1 = 0;
	g_pContext[g_nContexts].nPatch2 = 0;
    g_nContexts++;
}

static void PushBreak()
{
	// Push the current EmitPos onto the Break stack

	// Find enclosing loop/switch context
	int nContext = g_nContexts;
	while( nContext && g_pContext[nContext-1].nType != CTX_LOOP &&
		               g_pContext[nContext-1].nType != CTX_SWITCH ) nContext--;
	if( !nContext )
	{
        AddError( "break statement outside of any loop or switch", lex_line() );
		return;
	}

	if( g_nBreaks > MAX_BREAKS )
	{
        AddError( "too many break statements in current context", lex_line() );
		return;
    }

	g_pBreaks[g_nBreaks++] = g_nEmitPos;
	g_pContext[nContext-1].nBreaks++;
}

static void PushContinue()
{
	// Push the current EmitPos onto the Continue stack

	// Find enclosing loop context
	int nContext = g_nContexts;
	while( nContext && g_pContext[nContext-1].nType != CTX_LOOP ) nContext--;
	if( !nContext )
	{
        AddError( "continue statement outside of any loop", lex_line() );
		return;
	}

	if( g_nContinues > MAX_CONTINUES )
	{
        AddError( "too many continue statements in current context", lex_line() );
		return;
    }

	g_pContinues[g_nContinues++] = g_nEmitPos;
	g_pContext[nContext-1].nContinues++;
}

static void PopContext()
{
	g_nContexts--;

	int nExitLoop = g_nEmitPos;
	int nNextPass = g_pContext[g_nContexts].nNextPass;

	// Patch all break addresses for this context
	while( g_pContext[g_nContexts].nBreaks-- )
	{
		uint32 nPatch = g_pBreaks[--g_nBreaks];
		EPATCHD( nPatch, nExitLoop );
	}

	// Patch all continue addresses for this context
	while( g_pContext[g_nContexts].nContinues-- )
	{
		uint32 nPatch = g_pContinues[--g_nContinues];
		EPATCHD( nPatch, nNextPass );
	}

	// Clean up the info array
	g_pContext[g_nContexts].mValue.AssignNull();
}

static int AllocReg( stringp sName, int nType )
{
    int32 nCount;
	int nRet = INVALID_REGISTER;

	if( sName )
	{
		// Check for a duplicate name
		for( nCount = IDXL_ARGS; nCount < g_nLocals; nCount++ )
		{
			if( (g_pLocals[nCount].nType & RTYP_MASK) == (nType & RTYP_MASK) &&
				 g_pLocals[nCount].sName != NULL )
			{
				if( sName->Compare( g_pLocals[nCount].sName ) == 0 )
				{
					g_pLocals[nCount].nType = nType;
					g_pLocals[nCount].nInUse++;
					return nCount;
				}
			}
		}
	}

	// Look for a free register
	for( nCount = 0; nCount < g_nLocals; nCount++ )
	{
		if( (g_pLocals[nCount].nType & RTYP_MASK) == (nType & RTYP_MASK) &&
			 g_pLocals[nCount].nInUse == 0 )
		{
			nRet = nCount;
			break;
		}
	}

	if( nRet == INVALID_REGISTER )
	{
		if( g_nLocals >= MAX_LOCALS )
		{
			AddError( "too many local/intermediate variables", lex_line() );
			return 0;
		}
		else
		{
			nRet = g_nLocals++;
		}
	}

    g_pLocals[nRet].sName = sName;
    g_pLocals[nRet].nType = nType;
    g_pLocals[nRet].nInUse = 1;

//if( sName ) printf( "allocated register #%d name '%s' type %d\n", nRet, sName->CString(), nType );
//else printf( "allocated register #%d name NULL type %d\n", nRet, nType );

    return nRet;
}

static void UnlinkReg( int nReg )
{
	if( nReg != INVALID_REGISTER  && nReg > IDXL_THIS )
	{
		ASSERT( nReg >= 0 && nReg < g_nLocals );

		/* This is called for all registers but only affects temp registers
		if( ((g_pLocals[nReg].nType & RTYP_MASK) == RTYP_TEMP ||
			 (g_pLocals[nReg].nType & RTYP_MASK) == RTYP_MEMBER ||
			 ( bLocal && (g_pLocals[nReg].nType & RTYP_MASK) == RTYP_LOCAL )) &&
			g_pLocals[nReg].nInUse > 0 )
		*/

		if( (g_pLocals[nReg].nType & RTYP_MASK) == RTYP_TEMP &&
			g_pLocals[nReg].nInUse > 0 )
		{
			g_pLocals[nReg].nInUse--;
			if( g_pLocals[nReg].nInUse < 1 )
			{
				// Clear the name so it won't match anything later
				g_pLocals[nReg].sName = NULL;
//printf( "freed register #%d\n", nReg );
			}
			else
			{
//printf( "unlinked register #%d (now %d refs)\n", nReg, g_pLocals[nReg].nInUse );
			}
		}

    }
}

static void LinkReg( int nReg, bool bLocal = false )
{
    ASSERT( nReg >= 0 && nReg < g_nLocals );

    // This is called for all registers but only affects temp/member registers
    if( (g_pLocals[nReg].nType & RTYP_MASK) == RTYP_TEMP ||
		(g_pLocals[nReg].nType & RTYP_MASK) == RTYP_MEMBER ||
		( bLocal && (g_pLocals[nReg].nType & RTYP_MASK) == RTYP_LOCAL ) )
    {
        ASSERT( g_pLocals[nReg].nInUse > 0 );
        g_pLocals[nReg].nInUse++;
//printf( "linked register R#%d (now %d refs)\n", nReg, g_pLocals[nReg].nInUse );
    }
}


/* ---- Function Header Bits ---- */

void P_Function( stringp sName )
{
	// If we're not importing, we should only parse one function.
	if ( !g_importing && (g_funcResult || g_sErrors) ) {
		AddError("expecting end of function", lex_line());
		return;
	}

	g_sParseName = sName; // borrow

	// Remember the start of the function source code.
    g_nFuncLine = lex_line();
	g_nFuncPos = lex_pos();

	// Initialise all global state required for function parsing
	g_nArgs = 0;
    g_nLocals = 0;
    g_nBreaks = 0;
    g_nContinues = 0;
    g_nEmitPos = 0;
    g_nLinePos = 0;

	// Allocate a global array for constants
	g_arConstants = new array_t;
	if( !g_arConstants )
	{
		AddError( "out of memory", lex_line() );
	}

	// Reserve a register for "this"
	g_pLocals[IDXL_THIS].sName = NULL;
	g_pLocals[IDXL_THIS].nType = RTYP_RESERVED;
	g_pLocals[IDXL_THIS].nInUse = 1;
	g_nLocals++;
}

void P_FunctionArg( stringp sName )
{
    int32 nCount;

    for( nCount = IDXL_ARGS; nCount < g_nLocals; nCount++ )
    {
        if( sName->Compare( g_pLocals[nCount].sName ) == 0 )
        {
            AddError( "duplicate argument name", lex_line() );
            return;
        }
	}

    if( g_nLocals >= MAX_LOCALS )
    {
        AddError( "function has too many arguments", lex_line() );
        return;
    }

    g_pLocals[g_nLocals].sName = sName;
    g_pLocals[g_nLocals].nType = RTYP_LOCAL | RTYP_LVALUE;
    g_pLocals[g_nLocals].nInUse = 1;

//printf( "allocated argument %d (name %s)\n", g_nLocals, sName->CString() );

    g_nLocals++;
	g_nArgs++;
}

void P_EndStmt()
{
    // Probably need this for optimising
}

void P_EndExpr( int nReg )
{
    UnlinkReg( nReg );
}

void P_EndFunction()
{
	if( !g_sErrors )
	{
		EMITOP(OP_RETURN, 0, 0, 0);

		//DebugDump();

		// Build the new function result
		functionp fnResult = new function_t;

		// Trim back to the closing brace.
		// int32 endPos = g_nPos;
		// while (g_pLexSrc[endPos] != '}' && endPos > g_nFuncPos) --endPos;

		// The lexer advances beyond '(' before the parser calls P_Function,
		// so we must go backwards until we find it.
		const char* src = g_sLexSource->CString();
		int32 start = g_nFuncPos;
		while (src[start] != '(' && src[start] != '\n' && start > 0) --start;

		// Likewise, the lexer advances beyond the end of the function to
		// the beginning of the next token.
		int32 end = lex_pos();
		if (end) --end; // before the next token.
		while (src[end] != '}' && end > start) --end;
		++end; // include '}'

		stringp sSource = g_sLexSource->SubString(start, end);
		sSource->Prepend(g_sParseName); // add function name.

		g_sParseName->AddRef(); // name is a lex temporary

		// Populate the function object
		fnResult->m_sName = g_sParseName; // new ref
		fnResult->m_sSource = sSource; // ownership
		fnResult->m_arConstants = g_arConstants; // ownership
		fnResult->m_nArgs = g_nArgs;
		fnResult->m_nLocals = g_nLocals;
		fnResult->m_nCodeSize = g_nEmitPos;
		fnResult->m_nLineSize = g_nLinePos;
		fnResult->m_pCode = g_pCode;
		fnResult->m_pLines = g_pLines;

		// Make sure these don't get released later.
		g_pCode = NULL;
		g_pLines = NULL;
		g_arConstants = NULL;

		if (g_importing) {
			if (g_importObj) {
				g_importObj->Assign( g_sParseName, any_t(fnResult) ); // new ref
			}
			fnResult->Release();
		} else {
			// Assign the new function to the parse value
			g_funcResult = fnResult; // ownership
		}
	}
}


/* ---- Flow of Control Keywords ---- */

void P_If( int nReg )
{
	PushContext( CTX_IF );

    // Emit if-goto for false condition
    THIS_CONTEXT.nPatch1 = g_nEmitPos;
    EMITOPD( OP_IFZ, nReg, 0 );

    UnlinkReg( nReg );
}

void P_Else()
{
    uint32 nIfGoto = THIS_CONTEXT.nPatch1;

    // Emit goto for exit
    THIS_CONTEXT.nPatch1 = g_nEmitPos;
    EMITOPD( OP_GOTO, 0, 0 );

	// Patch if-goto for else condition
    EPATCHD( nIfGoto, g_nEmitPos );
}

void P_EndIf()
{
    // Patch exit address
    EPATCHD( THIS_CONTEXT.nPatch1, g_nEmitPos );

	PopContext();
}

void P_Return( int nReg )
{
	EMITOP( OP_RETURN, nReg, 0, 0 );
}

void P_Exit( int nReg )
{
	EMITOP( OP_EXIT, nReg, 1, 0 );
}

void P_Delete( int nReg )
{
    EMITOP( OP_DELETE, nReg, 0, 0 );
}

void P_While()
{
	PushContext( CTX_LOOP );
}

void P_WhileExpr( int nReg )
{
    // Emit if-goto for false condition
    THIS_CONTEXT.nPatch1 = g_nEmitPos;
    EMITOPD( OP_IFZ, nReg, 0 );

    UnlinkReg( nReg );
}

void P_WhileEnd()
{
    // Emit goto for looping
    EMITOPD( OP_GOTO, 0, THIS_CONTEXT.nNextPass );

	// Patch the if-goto for loop exit
    EPATCHD( THIS_CONTEXT.nPatch1, g_nEmitPos );

	// Resolve all breaks/continues in the loop
	PopContext();
}

void P_Until()
{
	PushContext( CTX_LOOP );
}

void P_UntilExpr( int nReg )
{
    // Emit if-goto for true condition
    THIS_CONTEXT.nPatch1 = g_nEmitPos;
    EMITOPD( OP_IF, nReg, 0 );

    UnlinkReg( nReg );
}

void P_UntilEnd()
{
    // Emit goto for looping
    EMITOPD( OP_GOTO, 0, THIS_CONTEXT.nNextPass );

	// Patch the if-goto for loop exit
	EPATCHD( THIS_CONTEXT.nPatch1, g_nEmitPos );

	// Resolve all breaks/continues in the loop
	PopContext();
}

void P_Repeat()
{
	PushContext( CTX_LOOP );
}

void P_RepeatEnd()
{
    // Emit goto for looping
    EMITOPD( OP_GOTO, 0, THIS_CONTEXT.nNextPass );

	// Resolve all breaks/continues in the loop
	PopContext();
}

void P_Do()
{
	PushContext( CTX_LOOP );
}

void P_DoWhile( int nReg )
{
    // Emit if-goto for true condition
    EMITOPD( OP_IF, nReg, THIS_CONTEXT.nNextPass );

	// Resolve all breaks/continues in the loop
	PopContext();

    UnlinkReg( nReg );
}

void P_DoUntil( int nReg )
{
    // Emit if-goto for false condition
    EMITOPD( OP_IFZ, nReg, THIS_CONTEXT.nNextPass );

	// Resolve all breaks/continues in the loop
	PopContext();

    UnlinkReg( nReg );
}

void P_ForInit( int nReg )
{
	PushContext( CTX_LOOP );

	// Record address of conditional expression
	THIS_CONTEXT.nPatch1 = g_nEmitPos;

	UnlinkReg( nReg );
}

void P_ForCondition( int nReg )
{
	// Emit if-goto after conditional expression (goto loop body)
	THIS_CONTEXT.nPatch2 = g_nEmitPos;
	if( nReg != INVALID_REGISTER )
	{
		EMITOPD( OP_IF, nReg, 0 );

		// Emit goto for failed test (goto loop exit)
		EMITOPD( OP_GOTO, 0, 0 ); // nPatch2 + 1
	}
	else
	{
		// Always continue
		EMITOPD( OP_GOTO, 0, 0 );
	}

	// Modify the NextPass address (special case in for loops):
	// Address of increment expression
	THIS_CONTEXT.nNextPass = g_nEmitPos;

	UnlinkReg( nReg );
}

void P_ForInc( int nReg )
{
	// Emit goto for looping (goto conditional expression)
  EMITOPD( OP_GOTO, 0, THIS_CONTEXT.nPatch1 );

	// Patch the if-goto for loop body
	EPATCHD( THIS_CONTEXT.nPatch2, g_nEmitPos );

	UnlinkReg( nReg );
}

void P_EndFor()
{
  // Emit goto for looping (goto increment expression)
  EMITOPD( OP_GOTO, 0, THIS_CONTEXT.nNextPass );

	// Patch the goto for loop exit
	EPATCHD( THIS_CONTEXT.nPatch2 + 1, g_nEmitPos );

	// Resolve all breaks/continues in the loop
	PopContext();
}

void P_ForEach( stringp sName, int nTarget )
{
	PushContext( CTX_LOOP );

	int nCtrl = AllocReg( NULL, RTYP_TEMP );
	int nLocal = P_Local( sName );

	// Set up the foreach
	EMITOP( OP_FOREACH, nCtrl, nLocal, nTarget );
	EMITOPD( 0, 0, 0 );

	// Record the loop body address
	THIS_CONTEXT.nPatch1 = g_nEmitPos;

	// Save the control register for P_EndForEach
	THIS_CONTEXT.nPatch2 = nCtrl;
}

void P_EndForEach( stringp sName, int nTarget )
{
	int nLocal = P_Local( sName );
	int nCtrl = THIS_CONTEXT.nPatch2;

	// Modify the NextPass address (special case in foreach loops):
	// Address of next-element opcode
	THIS_CONTEXT.nNextPass = g_nEmitPos;

	// Emit op to advance the foreach (goto loop body)
	EMITOP( OP_NEXTEACH, nCtrl, nLocal, nTarget );
	EMITOPD( 0, 0, THIS_CONTEXT.nPatch1 );

	// Patch the foreach opcode for immediate loop exit
	int32 nPatch = THIS_CONTEXT.nPatch1 - 1;
	EPATCHD( nPatch, g_nEmitPos );

	// Resolve all breaks/continues in the loop
	PopContext();

	UnlinkReg( nTarget );
	UnlinkReg( nCtrl );
}

void P_Switch( int nReg )
{
	PushContext( CTX_SWITCH );

	// Switches are a little wierd. The switch opcode referrs to an
	// array in the constants list. Each element-pair in the array
	// specifies (constant-value, code-offset) for a case statement.
	// The first element is the code-offset for the default case.
	arrayp arInfo = new array_t; // new
	THIS_CONTEXT.mValue.AssignRef( any_t(arInfo) ); // ownership

	// Reserve the first element for the default case
	arInfo->AppendItem( any_t( (int32)0 ) );

	// Add the switch info to the constants list
	unsigned short nConst = g_arConstants->Size();
	g_arConstants->AppendItem( any_t( arInfo ) ); // ref

	// Emit the switch opcode using the array constant
	EMITOPD( OP_SWITCH, nReg, nConst );
}

void P_SwitchCaseInt( int32 nValue )
{
	// Make sure we're in a switch context
	if( !g_nContexts || THIS_CONTEXT.nType != CTX_SWITCH )
	{
        AddError( "case statement outside of switch", lex_line() );
		return;
	}

	// Add this case to the switch info
	((arrayp)THIS_CONTEXT.mValue)->AppendItem( any_t( nValue ) );
	((arrayp)THIS_CONTEXT.mValue)->AppendItem( any_t( g_nEmitPos ) );
}

void P_SwitchCaseFloat( float64 fValue )
{
	// Make sure we're in a switch context
	if( !g_nContexts || THIS_CONTEXT.nType != CTX_SWITCH )
	{
        AddError( "case statement outside of switch", lex_line() );
		return;
	}

	// Add this case to the switch info
	((arrayp)THIS_CONTEXT.mValue)->AppendItem(any_t(fValue));
	((arrayp)THIS_CONTEXT.mValue)->AppendItem(any_t(g_nEmitPos));
}

void P_SwitchCaseString( stringp sValue )
{
	// Make sure we're in a switch context
	if( !g_nContexts || THIS_CONTEXT.nType != CTX_SWITCH )
	{
        AddError( "case statement outside of switch", lex_line() );
		return;
	}

	// Add this case to the switch info
	((arrayp)THIS_CONTEXT.mValue)->AppendItem(any_t(sValue)); // ref
	((arrayp)THIS_CONTEXT.mValue)->AppendItem(any_t(g_nEmitPos));
}

void P_SwitchDefault()
{
	// Make sure we're in a switch context
	if( !g_nContexts || THIS_CONTEXT.nType != CTX_SWITCH )
	{
        AddError( "case statement outside of switch", lex_line() );
		return;
	}

	// Check if a default has already been found
	const any_t& mValue = ((arrayp)THIS_CONTEXT.mValue)->Lookup(0); // borrow
	if( (int32)mValue > 0 )
	{
        AddError( "more than one default clause in switch statement", lex_line() );
	}
	else
	{
		// Set the code offset for the default clause
		((arrayp)THIS_CONTEXT.mValue)->Assign( 0, any_t( g_nEmitPos ) );
	}
}

void P_EndSwitch()
{
	// Check if a default case was found
	const any_t& mValue = ((arrayp)THIS_CONTEXT.mValue)->Lookup(0); // borrow
	if( (int32)mValue == 0 )
	{
		// Set the code offset for the default clause
		((arrayp)THIS_CONTEXT.mValue)->Assign( 0, any_t( g_nEmitPos ) );
	}

	// Resolve all breaks in the switch
	PopContext();
}

void P_Break()
{
	PushBreak();
	EMITOP( OP_GOTO, 0, 0, 0 );
}

void P_Continue()
{
	PushContinue();
	EMITOP( OP_GOTO, 0, 0, 0 );
}

void P_Try()
{
	PushContext( CTX_CATCH );

	// Emit op to set up catch address
	EMITOPD( OP_CATCH, 0, 0 );
}

void P_Catch( stringp sName )
{
	// Emit goto for block exit
	THIS_CONTEXT.nPatch1 = g_nEmitPos;
	EMITOPD( OP_GOTO, 0, 0 );

	// Patch the catch op with address of catch block
	EPATCHD( THIS_CONTEXT.nNextPass, g_nEmitPos );

	// Allocate a register for the catch variable
	// Patch the catch op with the new register
	int nReg = P_Local( sName );
	EPATCHA( THIS_CONTEXT.nNextPass, nReg );
}

void P_EndCatch()
{
	if( THIS_CONTEXT.nPatch1 )
	{
		// Patch the exit address
		EPATCHD( THIS_CONTEXT.nPatch1, g_nEmitPos );
	}
	else
	{
		// Point the catch address at the exit address
		// Leave the A register set to zero (no catch variable)
		EPATCHD( THIS_CONTEXT.nNextPass, g_nEmitPos );
	}

	PopContext();

	// When the parse-tree version is built:
	// Find an enclosing try..catch context
	// Emit an OP_CATCH to restore the outer catch block

	// For the moment, this stacking is handled at runtime
	EMITOPD( OP_CATCH, 0, 0 );
}


/* ---- Dyadic Operators ---- */

int P_Dyadic( int nOp, int nR1, int nR2 )
{
    int nRet = AllocReg( NULL, RTYP_TEMP );

    EMITOP( nOp, nRet, nR1, nR2 );

    UnlinkReg( nR1 );
    UnlinkReg( nR2 );

    return nRet;
}

int P_Assign( int nR1, int nR2 )
{
    if( (g_pLocals[nR1].nType & RTYP_LVALUE) == 0 )
    {
        AddError( "LHS of assignment not an lvalue", lex_line() );
    }
	else
	{
		// Copy the result (R2) into the lvalue (R1)
		EMITOP( OP_ASSIGN, nR1, nR2, 0 );
		UnlinkReg( nR2 );
	}

	return nR1;
}

void P_Or( int nReg )
{
    int nRet = AllocReg( NULL, RTYP_TEMP );

	// Copy the lhs into the result register
	EMITOP( OP_COPY, nRet, nReg, 0 );
	UnlinkReg( nReg );

    // Emit if-goto for true condition
	PushContext( CTX_IF );
    EMITOPD( OP_IF, nRet, 0 );

	// Save the return register for P_EndOr
	THIS_CONTEXT.nPatch1 = nRet;
}

int P_EndOr( int nReg )
{
	int nRet = THIS_CONTEXT.nPatch1;

    // Copy the rhs-result into the result register
    EMITOP( OP_COPY, nRet, nReg, 0 );
    UnlinkReg( nReg );

	// Patch the if-goto with the exit address
    EPATCHD( THIS_CONTEXT.nNextPass, g_nEmitPos );
	PopContext();

    return nRet;
}

void P_And( int nReg )
{
    int nRet = AllocReg( NULL, RTYP_TEMP );

	// Copy the lhs into the result register
	EMITOP( OP_COPY, nRet, nReg, 0 );
	UnlinkReg( nReg );

    // Emit if-goto for false condition
	PushContext( CTX_IF );
    EMITOPD( OP_IFZ, nRet, 0 );

	// Save the return register for P_EndAnd
	THIS_CONTEXT.nPatch1 = nRet;
}

int P_EndAnd( int nReg )
{
	int nRet = THIS_CONTEXT.nPatch1;

    // Copy the rhs-result into the lhs-result register
    EMITOP( OP_COPY, nRet, nReg, 0 );
    UnlinkReg( nReg );

	// Patch the if-goto with the exit address
    EPATCHD( THIS_CONTEXT.nNextPass, g_nEmitPos );
	PopContext();

    return nRet;
}


/* ---- Monadic Operators ---- */

int P_Monadic( int nOp, int nR1 )
{
    int nRet = AllocReg( NULL, RTYP_TEMP );

    EMITOP( nOp, nRet, nR1, 0 );

    UnlinkReg( nR1 );

    return nRet;
}


/* ---- Misc operations ---- */

int P_Local( stringp sName )
{
	return AllocReg( sName, RTYP_LOCAL | RTYP_LVALUE );
}

int P_Access( int nReg, stringp sName )
{
    int nRet = AllocReg( NULL, RTYP_TEMP | RTYP_LVALUE );

	// Add the constant to the array
	unsigned short nConst = g_arConstants->Size();
	g_arConstants->AppendItem(any_t(sName)); // ref

	// Emit an op to copy the constant into a register
	EMITOPD( OP_CONST, nRet, nConst );
	EMITOP( OP_ACCESS, nRet, nReg, nRet );

	return nRet;
}

int P_Lookup( stringp sName )
{
    int nRet = AllocReg( NULL, RTYP_TEMP | RTYP_LVALUE );

	// Add the constant to the array
	unsigned short nConst = g_arConstants->Size();
	g_arConstants->AppendItem(any_t(sName)); // ref

	// Emit an op to copy the constant into a register
	EMITOPD( OP_LOOKUP, nRet, nConst );

	return nRet;
}

int P_Index( int nR1, int nR2 )
{
    int nRet = AllocReg( NULL, RTYP_TEMP | RTYP_LVALUE );

    EMITOP( OP_INDEX, nRet, nR1, nR2 );

    return nRet;
}

void P_Invoke()
{
	PushContext( CTX_LIST );

	// Create an array to hold list item registers
	arrayp arInfo = new array_t;
	THIS_CONTEXT.mValue.AssignRef( any_t(arInfo) ); // ownership
}

int P_EndInvoke( int nReg, bool bThread )
{
    int nRet, nArgs, nArgReg;
    int32 nCount, nPos;
    byte *pArgs;
	bool bFail = false;

	nRet = AllocReg( NULL, RTYP_TEMP );

	// Validate the argument list
	arrayp arInfo = ((arrayp)THIS_CONTEXT.mValue);
	ASSERT( arInfo != NULL );
	nArgs = arInfo->Size();
	if( nArgs > MAX_LOCALS )
	{
        AddError( "too many arguments in function call", lex_line() );
		bFail = true;
	}

	// Check for a predefined function
	int nPredef = -1;
	if( g_pLocals[nReg].sName )
	{
		for( int i = 0; g_pPredef[i].szName; i++ )
		{
			if( g_pLocals[nReg].sName->CompareCStr( g_pPredef[i].szName ) == 0 )
			{
				// Found a matching predefined function
				nPredef = i;

				// Unlink the local reserved for this predef name
				// if( (g_pLocals[nReg].nType & MASK) == RTYP_LOCAL ) UnlinkReg( nReg, true );

				// Check the argument count
				if( nArgs > g_pPredef[nPredef].nMaxArgs )
				{
					AddError( "too many arguments to function", lex_line() );
					bFail = true;
				}
				else if( nArgs < g_pPredef[nPredef].nMinArgs )
				{
					AddError( "too few arguments to function", lex_line() );
					bFail = true;
				}

				// Pad the argument list to maximum length
				while( nArgs < g_pPredef[nPredef].nMaxArgs )
				{
					arInfo->AppendItem(any_t(int32(INVALID_REGISTER)));
					nArgs++;
				}

				// What happen if the function does not return a value?
				// Should we return an 'invalid register' result or a fixed 'null' register?
				// For the moment, the PF_* opcodes all assign null as the return.

				break;
			}
		}
	}

	if( !bFail )
	{
		// Emit the opcode and arguments
		if( nPredef > -1 )
		{
			EMITOP( g_pPredef[nPredef].nOp, nRet, 0, 0 );
			nPos = 2; // starting byte for arguments
		}
		else
		{
			if( bThread ) EMITOP( OP_THREAD, nReg, nArgs, nRet );
			else EMITOP( OP_INVOKE, nReg, nArgs, nRet );
			if( nArgs > 0 ) EMITOPD(0, 0, 0); // reserve space
			nPos = 0;
		}

//if( bThread ) printf( "thread %d with %d args (ret %d)\n", (int)nReg, (int)nArgs, (int)nRet );
//else printf( "invoke %d with %d args (ret %d)\n", (int)nReg, (int)nArgs, (int)nRet );

		if( nArgs > 0 )
		{
			pArgs = ((byte*)&g_pCode[g_nEmitPos - 1]);
			for( nCount = 0; nCount < nArgs; nCount++ )
			{
				if( nPos > 3 )
				{
					EMITOPD(0, 0, 0); // reserve more space, may move g_pCode
					pArgs = (byte*)&g_pCode[g_nEmitPos - 1];
					nPos = 0;
				}
				const any_t& mValue = arInfo->Lookup(nCount); // borrow
				nArgReg = int32(mValue);
//printf( "argument is %d\n", (int)nArgReg );
				pArgs[nPos++] = (byte)nArgReg;
			}
		}
	}

	// Unlink all list item registers
	for( nCount = 0; nCount < nArgs; nCount++ )
	{
		const any_t& mValue = arInfo->Lookup( nCount ); // borrow
		UnlinkReg( (int32)mValue );
	}

	PopContext();

    return nRet;
}

void P_ListItem( int nReg )
{
	if( !g_nContexts || THIS_CONTEXT.nType != CTX_LIST )
	{
		AddError( "list item outside of list", lex_line() );
		UnlinkReg( nReg );
		return;
	}

	// Add this register to the register list
	((arrayp)THIS_CONTEXT.mValue)->AppendItem(any_t((int32)nReg));
}

void P_MapItem( int nR1, int nR2 )
{
  // NOT USED

	// Make sure we're in a map context
	if( !g_nContexts || THIS_CONTEXT.nType != CTX_MAPPING )
	{
    AddError( "mapping item outside of mapping", lex_line() );
    UnlinkReg( nR1 );
    UnlinkReg( nR2 );
		return;
	}

	// Add this register pair to the register list
	((arrayp)THIS_CONTEXT.mValue)->AppendItem(any_t((int32)nR1, (int32)nR2));
}

void P_BuildList()
{
	PushContext( CTX_LIST );

	// Create an array to hold list item registers
	arrayp arInfo = new array_t;
	THIS_CONTEXT.mValue.AssignRef( any_t(arInfo) ); // ownership
}

int P_EndListMap()
{
    int nRet, nArgs;
    int32 nCount;
    byte *pArgs;
    int nColl = 0;

    nRet = AllocReg( NULL, RTYP_TEMP );

	arrayp arInfo = ((arrayp)THIS_CONTEXT.mValue);
	ASSERT( arInfo != NULL );
	nArgs = arInfo->Size();
	if( nArgs > MAX_LOCALS )
	{
        AddError( "too many arguments in list construct", lex_line() );
	}
	else
	{
		if( THIS_CONTEXT.nType == CTX_MAPPING )
		{
      // NOT USED
      //printf( "mapping %d with %d args\n", (int)nRet, (int)nArgs );
			// EMITOP( OP_MAPPING, nRet, nArgs, 0 );
			EMITOP( OP_ARRAY, nRet, nArgs, 0 );
		}
		else
		{
      //printf( "array %d with %d args\n", (int)nRet, (int)nArgs );
			EMITOP( OP_ARRAY, nRet, nArgs, 0 );
		}

		if( nArgs > 0 )
		{
			EMITOPD(0, 0, 0); // May move g_pCode
			pArgs = (byte*)&g_pCode[g_nEmitPos - 1];
			for( nCount = 0; nCount < nArgs; nCount++ )
			{
				if( nColl > 3 )
				{
					// Reserve more code space (may move g_pCode)
					EMITOPD(0, 0, 0);
					pArgs = (byte*)&g_pCode[g_nEmitPos - 1];
					nColl = 0;
				}
				const any_t& mValue = arInfo->Lookup( nCount ); // borrow
				if( mValue.IsRange() )
				{
					const range_t& rnValue = mValue.range();
//printf( "arguments are %d : %d\n", (int)mValue.Lower(), (int)mValue.Upper() );
					pArgs[nColl++] = (byte)rnValue.nLower;
					pArgs[nColl++] = (byte)rnValue.nUpper;
				}
				else
				{
//printf( "argument is %d\n", (int)(int32)mValue );
					pArgs[nColl++] = (byte)(int32)mValue;
				}
			}
		}
	}

	// Unlink all list item registers
	for( nCount = 0; nCount < nArgs; nCount++ )
	{
		const any_t&  mValue = arInfo->Lookup( nCount ); // borrow
		if( mValue.IsRange() )
		{
			const range_t& rnValue = mValue.range();
			UnlinkReg( rnValue.nLower );
			UnlinkReg( rnValue.nUpper );
		}
		else
		{
			UnlinkReg( (int32)mValue );
		}
	}

	PopContext();

    return nRet;
}

int P_New( stringp sClass, bool bHasArgs )
{
    int nRet = AllocReg( NULL, RTYP_TEMP );

	PushContext( CTX_LIST );
	THIS_CONTEXT.nPatch2 = nRet;

	// Create an array to hold list item registers
	arrayp arInfo = new array_t;
	THIS_CONTEXT.mValue.AssignRef( any_t(arInfo) ); // ownership

    if( sClass->CompareCStr( "object" ) == 0 )
	{
		THIS_CONTEXT.nNextPass = OP_NEW_OBJECT;
	}
	else if( sClass->CompareCStr( "array" ) == 0 )
	{
		THIS_CONTEXT.nNextPass = OP_NEW_ARRAY;
	}
	else if( sClass->CompareCStr( "mapping" ) == 0 )
	{
		THIS_CONTEXT.nNextPass = OP_NEW_MAPPING;
	}
	else
	{
        AddError( "expecting 'object', 'array' or 'mapping' following 'new'", lex_line() );
	}

    return nRet;
}

int P_EndNew()
{
	int nOp = THIS_CONTEXT.nNextPass; // nPatch1 is stomped by the list functions
	int nRet = THIS_CONTEXT.nPatch2;
	int nB = INVALID_REGISTER, nC = INVALID_REGISTER;
	any_t mValue;

	// Validate the argument list
	arrayp arInfo = ((arrayp)THIS_CONTEXT.mValue);
	ASSERT( arInfo != NULL );
	int nArgs = arInfo->Size();
	bool bFail = false;

	if( nOp == OP_NEW_OBJECT )
	{
		if( nArgs < 1 )
		{
			AddError( "must supply argument(s) in 'new object' clause", lex_line() );
			bFail = true;
		}
		else if( nArgs > 2 )
		{
			AddError( "too many arguments in 'new object' clause", lex_line() );
			bFail = true;
		}
	}
	else if( nOp == OP_NEW_ARRAY )
	{
		if( nArgs > 1 )
		{
			AddError( "too many arguments in 'new array' clause", lex_line() );
			bFail = true;
		}
	}
	else if( nOp == OP_NEW_MAPPING )
	{
		if( nArgs > 0 )
		{
			AddError( "too many arguments in 'new mapping' clause", lex_line() );
			bFail = true;
		}
	}
	else
	{
        AddError( "internal error in new operator", lex_line() );
		bFail = true;
	}

	if( !bFail )
	{
		if( nArgs > 0 )
		{
			// Extract the first argument
			const any_t&  mValue = arInfo->Lookup(0); // borrow
			nB = int32(mValue);
		}

		if( nArgs > 1 )
		{
			// Extract the second argument
			const any_t&  mValue = arInfo->Lookup(1); // borrow
			nC = int32(mValue);
		}

		// Emit the appropriate 'new' opcode
		EMITOP( nOp, nRet, nB, nC );
	}

	// Unlink all list item registers
	for( int nCount = 0; nCount < nArgs; nCount++ )
	{
		const any_t&  mValue = arInfo->Lookup(nCount); // borrow
		UnlinkReg( (int32)mValue );
	}

	PopContext();

	return nRet;
}

int P_AddIntConst( int32 nValue )
{
    int nRet = AllocReg( NULL, RTYP_TEMP );

	// Add the constant to the array
	unsigned short nConst = (unsigned short)g_arConstants->Size();
	any_t mConst = nValue;
	g_arConstants->AppendItem( mConst );
	mConst.Release();

	// Emit an op to copy the constant into a register
	EMITOPD( OP_CONST, nRet, nConst );

	return nRet;
}

int P_AddFloatConst( float64 fValue )
{
    int nRet = AllocReg( NULL, RTYP_TEMP );

	// Add the constant to the array
	unsigned short nConst = (unsigned short)g_arConstants->Size();
	any_t mConst = fValue;
	g_arConstants->AppendItem( mConst );
	mConst.Release();

	// Emit an op to copy the constant into a register
	EMITOPD( OP_CONST, nRet, nConst );

	return nRet;
}

int P_AddStringConst( stringp sValue )
{
    int nRet = AllocReg( NULL, RTYP_TEMP );

	// Add the constant to the array
	unsigned short nConst = (unsigned short)g_arConstants->Size();
	g_arConstants->AppendItem( any_t( sValue ) ); // ref

	// Emit an op to copy the constant into a register
	EMITOPD( OP_CONST, nRet, nConst );

	return nRet;
}

int P_AddThisConst()
{
    // "this" is always in register zero
    return IDXL_THIS;
}

int P_AddPathConst( stringp path )
{
	if (path->Length() < 3) {
		// Compile as an OP_SETROOT to resolve '$'.
		int nRet = AllocReg(NULL, RTYP_TEMP);
		EMITOP(OP_SETROOT, nRet, 0, 0);
		return nRet;
	} else {
		// Compile as a series of OP_ACCESS after OP_LOOKUP.
		int nRet = AllocReg(NULL, RTYP_TEMP | RTYP_LVALUE);
		int nTemp = AllocReg(NULL, RTYP_TEMP);
		char* szPath = const_cast<char*>(path->CString()) + 1; // skip '$'
		if (*szPath == '.') ++szPath; // skip '.'
		EMITOP(OP_SETROOT, nRet, 0, 0);
		while (szPath) {
			// Find the next '.' in the path.
			stringp sName;
			char* szEnd = strchr(szPath, '.');
			if (szEnd) {
				sName = new string_t(szPath, szEnd - szPath);
				if (*szEnd == '.') szEnd++;
			} else {
				sName = new string_t(szPath);
			}
			// Add the name to the constants vector.
			unsigned short nConst = g_arConstants->Size();
			g_arConstants->AppendItem(any_t(sName)); // new ref
			sName->Release();
			// Emit an op to look up the next field.
			EMITOPD(OP_CONST, nTemp, nConst);
			EMITOP(OP_ACCESS, nRet, nRet, nTemp);
			szPath = szEnd;
		}
		UnlinkReg(nTemp);
		return nRet;
	}
}

int P_AddNullConst()
{
    int nRet = AllocReg( NULL, RTYP_TEMP );

	EMITOP( OP_SETNULL, nRet, 0, 0 );

	return nRet;
}


/* ---- Import operations ---- */

void P_SelectObject( stringp sPath )
{
	if (!g_importing) {
		AddError("expecting a function definition", lex_line());
		return;
	}
	// Find or create the object at the specified path.
	objectp oNew = NULL;
	RESULT nResult = g_oDatabase.FindCreateObject( sPath, NULL, NULL, true, oNew ); // borrow
	if (ISERROR(nResult)) {
		AddError( "cannot define object", lex_line() );
		g_importObj = NULL;
	} else {
		g_importObj = oNew;
	}
}

void P_Inherit( stringp sPath )
{
	if (g_importObj) {
		// Find or create the inherited object.
		// Create it if necessary so forward references will work.
		objectp oInherit = NULL;
		g_oDatabase.FindCreateObject( sPath, NULL, NULL, true, oInherit ); // borrow
		if (oInherit) {
			RESULT err = g_importObj->AddInherit(oInherit);
			if (ISERROR(err)) AddError( "cannot inherit object", lex_line() );
		} else {
			AddError( "cannot inherit object", lex_line() );
		}
	}
}

void P_BeginAttr( stringp sName )
{
	if (!g_importing) {
		AddError("expecting a function definition", lex_line());
		return;
	}
	// Save the attribute name.
	g_sParseName = sName; // borrow
	PushContext( CTX_ATTRIB );
}

void P_EndAttr()
{
	if (g_nContexts && THIS_CONTEXT.nType == CTX_ATTRIB) {
		if (g_importObj && g_sParseName) {
			g_importObj->Assign(g_sParseName, THIS_CONTEXT.mValue); // new ref.
		}
		PopContext();
	}
}

static void AddConstToContext( const any_t& mValue )
{
	// Make sure we're in a list context
	if( !g_nContexts || (THIS_CONTEXT.nType != CTX_LIST &&
                       THIS_CONTEXT.nType != CTX_MAPPING &&
                       THIS_CONTEXT.nType != CTX_ATTRIB) )
	{
        AddError( "attribute value outside of any context", lex_line() );
        return;
	}

	if( THIS_CONTEXT.nType == CTX_LIST || THIS_CONTEXT.nType == CTX_MAPPING )
	{
		// Append the constant to the list
		((arrayp)THIS_CONTEXT.mValue)->AppendItem(mValue); // new ref
	}
	else
	{
		// Assign the constant to the context
		THIS_CONTEXT.mValue.Assign( mValue ); // new ref
	}
}

void P_AttrInt( int32 nValue, int32 minus )
{
	// Assign the value into the current context
	AddConstToContext( any_t(nValue * minus) ); // new ref
}

void P_AttrFloat( float64 fValue, float64 minus )
{
	// Assign the value into the current context
	AddConstToContext( any_t(fValue * minus) ); // new ref
}

void P_AttrStr( stringp sValue )
{
	// Assign the value into the current context
	AddConstToContext( any_t(sValue) ); // new ref
}

void P_AttrNull()
{
	any_t mResult; // null
	AddConstToContext( mResult ); // new ref
}

void P_AttrList()
{
  PushContext( CTX_LIST );

  // Create an array to hold list item constants
  arrayp arInfo = new array_t;
  THIS_CONTEXT.mValue.AssignRef( any_t(arInfo) ); // ownership
}

void P_AttrMap()
{
  PushContext( CTX_MAPPING );

  // Create an array to hold list item constants
  arrayp arInfo = new array_t;
  THIS_CONTEXT.mValue.AssignRef( any_t(arInfo) ); // ownership
}

void P_EndAttrListMap()
{
	any_t mResult;
	arrayp arInfo = ((arrayp)THIS_CONTEXT.mValue);
	if (arInfo)
	{
		if (THIS_CONTEXT.nType == CTX_MAPPING)
		{
			// Build a mapping from pairs of values in the array
			mappingp mpResult = new mapping_t;
			mResult.AssignRef( any_t(mpResult) ); // ownership

			int nArgs = arInfo->Size();
			for( int nCount = 0; nCount < nArgs; nCount += 2 )
			{
				const any_t& mKey = arInfo->Lookup( nCount ); // borrow
				const any_t& mValue = arInfo->Lookup( nCount + 1 ); // borrow
				mpResult->Assign( mKey, mValue ); // new refs
			}
		}
		else
		{
			// Keep the array
			mResult.Assign( any_t(arInfo) ); // new ref
		}
	}

	PopContext(); // release arInfo

	// Assign the result into the current context
	AddConstToContext( mResult ); // new ref
	mResult.Release();
}

void P_AttrRef( stringp sPath )
{
	// Link to an object during import (or create it for forward-references)
	objectp oValue = NULL;
	g_oDatabase.FindCreateObject( sPath, NULL, NULL, true, oValue ); // borrow
	if (oValue) {
		AddConstToContext( any_t(oValue) ); // new ref
	} else {
		AddError( "cannot resolve object reference", lex_line() );
	}
}


/* ---- Opcode Emit ---- */

static void EmitOp( byte op, byte a, byte b, byte c )
{
    byte *yNew;

	ASSERT( op > 0 && op < NUM_OPCODES );

    if( g_nEmitPos == 0 )
    {
        g_pCode = (uint32*)MALLOC(4);
        if( g_pCode == NULL )
        {
            AddError("failed to extend code chunk", lex_line());
            return;
        }
        g_pLines = (int32*)MALLOC(4);
        if( g_pLines == NULL )
        {
            AddError("failed to extend code chunk", lex_line());
            return;
        }
    }
    else
    {
        yNew = (byte*)REALLOC(g_pCode, (g_nEmitPos + 1) * 4);
        if( yNew == NULL )
        {
            AddError( "failed to extend code chunk", lex_line() );
            return;
        }
        g_pCode = (uint32*)yNew;
        yNew = (byte*)REALLOC(g_pLines, (g_nLinePos + 1) * 4);
        if( yNew == NULL )
        {
            AddError( "failed to extend line-info chunk", lex_line() );
            return;
        }
        g_pLines = (int32*)yNew;
    }

	byte* pOut = (byte*)&g_pCode[g_nEmitPos++];
	*pOut++ = op;
	*pOut++ = a;
	*pOut++ = b;
	*pOut++ = c;
    g_pLines[g_nLinePos++] = 1 + lex_line() - g_nFuncLine;
}

static void EmitOpD( byte op, byte a, unsigned short d )
{
    byte *yNew;

	ASSERT( op < NUM_OPCODES );

    if( g_nEmitPos == 0 )
    {
        g_pCode = (uint32*)MALLOC(4);
        if( g_pCode == NULL )
        {
            AddError("failed to extend code chunk", lex_line());
            return;
        }
        g_pLines = (int32*)MALLOC(4);
        if( g_pLines == NULL )
        {
            AddError("failed to extend code chunk", lex_line());
            return;
        }
    }
    else
    {
        yNew = (byte*)REALLOC(g_pCode, (g_nEmitPos + 1) * 4);
        if( yNew == NULL )
        {
            AddError("failed to extend code chunk", lex_line());
            return;
        }
        g_pCode = (uint32*)yNew;
        yNew = (byte*)REALLOC(g_pLines, (g_nLinePos + 1) * 4);
        if( yNew == NULL )
        {
            AddError("failed to extend line-info chunk", lex_line());
            return;
        }
        g_pLines = (int32*)yNew;
    }

	byte* pOut = (byte*)&g_pCode[g_nEmitPos++];
	*pOut++ = op;
	*pOut++ = a;
	*((unsigned short*)pOut) = d;

    g_pLines[g_nLinePos++] = 1 + lex_line() - g_nFuncLine;
}

static void DebugDump()
{
    int32 nCount;
    int op, a, b, c, bc;

    for( nCount = 0; nCount < g_nEmitPos; nCount++ )
    {
		byte* pIn = (byte*)&g_pCode[nCount];
        op = pIn[0];
        a = pIn[1];
        b = pIn[2];
        c = pIn[3];

		unsigned short* pInShort = (unsigned short*)&g_pCode[nCount];
		bc = pInShort[1];

        if( op < NUM_OPCODES )
        {
            printf( "[%4d] %s (%d) %d %d %d (%d)\n", (int)nCount, g_szOpcodes[op], op, a, b, c, bc );
        }
        else
        {
            printf( "[%4d] unknown (%d) %d %d %d (%d)\n", (int)nCount, op, a, b, c, bc );
        }
    }
}
