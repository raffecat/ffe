// parse.h
// FireFly server parsing module

#ifndef FIREFLY_PARSE_H
#define FIREFLY_PARSE_H

#include "opcode.h"
#include "any.h"

// Public interface to parser

RESULT Parse( stringp sSource, bool importing, any_t& mResult );

// Interface to YACC grammar

int yyparse();

// Interface to lexcical analyser

void lex_init( const char* source );
void lex_cleanup();
int lex_line();
int lex_pos();

// YACC interface to lexcical analyser

int yylex();
void yyerror( const char *yymessage );
void yywarn( const char *yymessage );

// YACC interface to parse module

void P_Function( stringp sName );
void P_FunctionArg( stringp sName );
void P_EndFunction();
void P_EndStmt();
void P_EndExpr( int nReg );

void P_If( int nReg );
void P_Else();
void P_EndIf();
void P_While();
void P_WhileExpr( int nReg );
void P_WhileEnd();
void P_Until();
void P_UntilExpr( int nReg );
void P_UntilEnd();
void P_Repeat();
void P_RepeatEnd();
void P_Do();
void P_DoWhile( int nReg );
void P_DoUntil( int nReg );
void P_ForInit( int nReg );
void P_ForCondition( int nReg );
void P_ForInc( int nReg );
void P_EndFor();
void P_ForEach( stringp sName, int nTarget );
void P_EndForEach( stringp sName, int nTarget );
void P_Switch( int nReg );
void P_SwitchCaseInt( int32 nValue );
void P_SwitchCaseFloat( float64 fValue );
void P_SwitchCaseString( stringp sValue );
void P_SwitchDefault();
void P_EndSwitch();

void P_Return( int nReg );
void P_Exit( int nReg );

void P_Break();
void P_Continue();

void P_Delete( int nReg );

void P_Try();
void P_Catch( stringp sName );
void P_EndCatch();

// Dyadic operators

int P_Dyadic( int nOp, int nR1, int nR2 );
int P_Assign( int nR1, int nR2 );

void P_Or( int nReg );
void P_And( int nReg );
int P_EndOr( int nReg );
int P_EndAnd( int nReg );

#define P_Range(R1, R2)			P_Dyadic( OP_RANGE, R1, R2 )
#define P_Equal(R1, R2)			P_Dyadic( OP_EQ, R1, R2 )
#define P_NotEq(R1, R2)			P_Dyadic( OP_NEQ, R1, R2 )
#define P_Greater(R1, R2)		P_Dyadic( OP_GT, R1, R2 )
#define P_Less(R1, R2)			P_Dyadic( OP_LT, R1, R2 )
#define P_GreaterEq(R1, R2)		P_Dyadic( OP_GE, R1, R2 )
#define P_LessEq(R1, R2)		P_Dyadic( OP_LE, R1, R2 )
#define P_Plus(R1, R2)			P_Dyadic( OP_ADD, R1, R2 )
#define P_Minus(R1, R2)			P_Dyadic( OP_SUB, R1, R2 )
#define P_Star(R1, R2)			P_Dyadic( OP_MUL, R1, R2 )
#define P_Slash(R1, R2)			P_Dyadic( OP_DIV, R1, R2 )
#define P_Percent(R1, R2)		P_Dyadic( OP_MOD, R1, R2 )
#define P_Caret(R1, R2)			P_Dyadic( OP_POW, R1, R2 )
#define P_Amper(R1, R2)			P_Dyadic( OP_AND, R1, R2 )
#define P_Bar(R1, R2)			P_Dyadic( OP_OR, R1, R2 )
#define P_Hash(R1, R2)			P_Dyadic( OP_XOR, R1, R2 )
#define P_Shl(R1, R2)			P_Dyadic( OP_SHL, R1, R2 )
#define P_Shr(R1, R2)			P_Dyadic( OP_SHR, R1, R2 )

// Monadic operators

int P_Monadic( int nOp, int nR1 );

#define P_Not(R)		P_Monadic( OP_NOT, R )
#define P_UTilde(R)		P_Monadic( OP_BITNOT, R )
#define P_UMinus(R)		P_Monadic( OP_NEG, R )

// Misc operations

int P_Local( stringp sName );
int P_Access( int nReg, stringp sName );
int P_Lookup( stringp sName );
int P_Index( int nR1, int nR2  );
void P_Invoke();
int P_EndInvoke( int nReg, bool bThread );
int P_New( stringp sClass, bool bHasArgs = false );
int P_EndNew();

int P_AddIntConst( int32 nValue );
int P_AddFloatConst( float64 fValue );
int P_AddStringConst( stringp sValue );
int P_AddThisConst();
int P_AddPathConst( stringp path );
int P_AddNullConst();

void P_BuildList();
void P_ListItem( int nReg );
void P_MapItem( int nR1, int nR2 );
int P_EndListMap();

// Import operations

void P_SelectObject( stringp sPath );
void P_Inherit( stringp sPath );
void P_BeginAttr( stringp sName );
void P_AttrInt( int32 nValue, int32 minus );
void P_AttrFloat( float64 fValue, float64 minus );
void P_AttrStr( stringp sValue );
void P_AttrNull();
void P_AttrList();
void P_AttrMap();
void P_EndAttrListMap();
void P_AttrRef( stringp sPath );
void P_EndAttr();

void P_ConstListItem( int nValue );
void P_ConstMapItem( int nKey, int nValue );

#endif // FIREFLY_PARSE_H
