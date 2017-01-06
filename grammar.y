// grammar.y
// FireFly server language grammar


// Header C code

%{

#include "global.h"
#include "parse.h"
#include "runtime.h"
#include "any.h"
#include "string.h"

#define YYERROR_VERBOSE 1
#define YYDEBUG 1

%}

// YYSTYPE union

%union {
	int r;
	stringp s;
	float64 f;
	int32 n;
}

// Expect 4 shift-reduce conflicts:
//    93 - this [ T_DOT .. ]
//   158 - new .. [ T_LPAREN .. ]
//   230 - try .. [ catch .. ]
//   325 - if .. then .. [ else .. ]
%expect 4


// Literal tokens

%token T_UNKNOWN

%token <s> T_IDENT
%token <s> T_PATH
%token <s> T_STRING
%token <n> T_INTEGER
%token <f> T_FLOAT

// Reserved words

%token <s> T_BREAK
%token <s> T_CASE
%token <s> T_CATCH
%token <s> T_CONTINUE
%token <s> T_DEFAULT
%token <s> T_DELETE
%token <s> T_DO
%token <s> T_ELSE
%token <s> T_EXIT
%token <s> T_FOR
%token <s> T_FOREACH
%token <s> T_IF
%token <s> T_IN
%token <s> T_LOCK
%token <s> T_NEW
%token <s> T_NULL
%token <s> T_REPEAT
%token <s> T_RETURN
%token <s> T_SWITCH
%token <s> T_THIS
%token <s> T_THREAD
%token <s> T_THROW
%token <s> T_TRY
%token <s> T_UNTIL
%token <s> T_WHILE

// Operators

%token T_PLUS_A      // +=         
%token T_PLUS        // +          
%token T_MINUS_A     // -=         
%token T_ARROW       // ->
%token T_MINUS       // -          
%token T_STAR_A      // *=         
%token T_STAR        // *          
%token T_SLASH_A     // /=         
%token T_SLASH       // /          
%token T_PERCENT_A   // %=         
%token T_PERCENT     // %          
%token T_CARET_A     // ^=         
%token T_CARET       // ^          
%token T_AND         // &&         
%token T_AMPER_A     // &=         
%token T_AMPER       // &          
%token T_OR          // ||         
%token T_BAR_A       // |=         
%token T_BAR         // |          
%token T_HASH_A      // #=         
%token T_HASH        // #          
%token T_SHR_A       // >>=        
%token T_SHR         // >>         
%token T_GREATER_EQ  // >=         
%token T_GREATER     // >          
%token T_SHL_A       // <<=        
%token T_SHL         // <<         
%token T_LESS_EQ     // <=         
%token T_LESS        // <          
%token T_TILDE       // ~          
%token T_EQUAL       // ==         
%token T_RANGE       // ..         
%token T_DOT         // .          
%token T_ASSIGN      // =          
%token T_INHERIT     // ::         
%token T_COLON       // :          
%token T_NOT_EQ      // !=         
%token T_BANG        // !          
%token T_COMMA       // ,          
%token T_SEMI        // ;
%token T_LPAREN      // (          
%token T_RPAREN      // )          
%token T_LBRACE      // {          
%token T_RBRACE      // }          
%token T_LBRACK      // [          
%token T_RBRACK      // ]


// Rule types

%type <r> o_expr expr expr1 expr2 expr3 expr4 expr5 expr6
%type <r> expr7 expr8 expr9 expr10 expr11 expr12 expr13
%type <r> expr14 expr15 expr16 expr17 literal for expr1a
%type <s> strlit member attrib

%%

/* ---- grammar and actions ---- */

target  : defns                                 { YYACCEPT; }
;

defns   : defns defn
        | defn
;

defn    : T_LBRACK T_PATH T_RBRACK { P_SelectObject($2); } oinherit
        | attrib T_ASSIGN { P_BeginAttr($1); } declit osemi { P_EndAttr(); }
        | attrib T_LPAREN { P_Function($1); } fargs T_RPAREN block { P_EndFunction(); }
;

oinherit : /* empty */
         | T_ARROW T_PATH                       { P_Inherit($2); }
;

osemi	: /* empty */
		| T_SEMI
;

fargs   : /* empty */
        | fargl
;

fargl   : T_IDENT                               { P_FunctionArg($1); }
        | fargl T_COMMA T_IDENT                 { P_FunctionArg($3); }
;

block   : T_LBRACE o_block
;

o_block : T_RBRACE
        | compound T_RBRACE
;

compound: stmt                                  { P_EndStmt(); }
        | compound stmt                         { P_EndStmt(); }
;

stmt    : if
        | while
        | for									{ /* ignore value */ }
		| switch
        | break T_SEMI
        | return T_SEMI
        | try
        | delete T_SEMI
        | block
        | expr T_SEMI                           { P_EndExpr($1); }
;

if      : T_IF T_LPAREN expr T_RPAREN { P_If($3); } stmt o_else
;

o_else  : /* empty */                           { P_EndIf(); }
        | T_ELSE { P_Else(); } stmt             { P_EndIf(); }
;

while   : T_WHILE { P_While(); } T_LPAREN expr T_RPAREN
            { P_WhileExpr($4); } o_while
        | T_UNTIL { P_Until(); } T_LPAREN expr T_RPAREN
            { P_UntilExpr($4); } o_until
        | T_REPEAT { P_Repeat(); } stmt         { P_RepeatEnd(); }
        | T_DO { P_Do(); } stmt o_do
;

o_while : T_SEMI                                { P_WhileEnd(); }
        | stmt                                  { P_WhileEnd(); }
;

o_until : T_SEMI                                { P_UntilEnd(); }
        | stmt                                  { P_UntilEnd(); }
;

o_do    : T_WHILE T_LPAREN expr T_RPAREN T_SEMI { P_DoWhile($3); }
        | T_UNTIL T_LPAREN expr T_RPAREN T_SEMI { P_DoUntil($3); }
;

for     : T_FOR T_LPAREN o_expr { P_ForInit($3); } T_SEMI
                         o_expr { P_ForCondition($6); } T_SEMI
						 o_expr { P_ForInc($9); } T_RPAREN o_for { /* do nothing */ }
        | T_FOREACH T_LPAREN T_IDENT T_IN expr T_RPAREN
			{ P_ForEach($3, $5); } o_forea { P_EndForEach($3, $5); }
;

o_for	: T_SEMI								{ P_EndFor(); }
		| stmt									{ P_EndFor(); }
;

o_forea	: T_SEMI
		| stmt
;

switch	: T_SWITCH T_LPAREN expr T_RPAREN		{ P_Switch($3); }
			block								{ P_EndSwitch(); }
		| T_CASE caselit T_COLON				{ /* do nothing */ }
		| T_DEFAULT { P_SwitchDefault(); } T_COLON { /* do nothing */ }
;

break   : T_BREAK								{ P_Break(); }
        | T_CONTINUE							{ P_Continue(); }
;

return  : T_RETURN o_expr                       { P_Return($2); }
        | T_EXIT o_expr                         { P_Exit($2); }
;

try     : T_TRY { P_Try(); } stmt o_catch		{ P_EndCatch(); }
;

o_catch	: /* empty */
		| T_CATCH T_LPAREN T_IDENT T_RPAREN { P_Catch($3); } stmt
;

delete  : T_DELETE expr                         { P_Delete($2); }
;

o_expr  : /* empty */                           { $$ = INVALID_REGISTER; }
        | expr
;

expr    : expr1
		| expr T_RANGE expr1					{ $$ = P_Range($1, $3); }
		| expr T_RANGE 							{ $$ = P_Range($1, INVALID_REGISTER); }
		| T_RANGE expr1							{ $$ = P_Range(INVALID_REGISTER, $2); }
;

expr1	: expr1a
        | expr1 T_OR { P_Or($1); } expr1a		{ $$ = P_EndOr($4); }
;

expr1a  : expr2
        | expr1a T_AND { P_And($1); } expr2		{ $$ = P_EndAnd($4); }
;

expr2   : expr3
        | T_BANG expr3                          { $$ = P_Not($2); }
;

expr3   : expr4
        | expr3 T_EQUAL expr4                   { $$ = P_Equal($1, $3); }
        | expr3 T_NOT_EQ expr4                  { $$ = P_NotEq($1, $3); }
        | expr3 T_GREATER expr4                 { $$ = P_Greater($1, $3); }
        | expr3 T_LESS expr4                    { $$ = P_Less($1, $3); }
        | expr3 T_GREATER_EQ expr4              { $$ = P_GreaterEq($1, $3); }
        | expr3 T_LESS_EQ expr4                 { $$ = P_LessEq($1, $3); }
;

expr4   : expr5
        | expr5 T_ASSIGN expr4                  { $$ = P_Assign($1, $3); }
        | expr5 T_PLUS_A expr4                  { $$ = P_Plus($1, $3); $$ = P_Assign($1, $$); }
        | expr5 T_MINUS_A expr4                 { $$ = P_Minus($1, $3); $$ = P_Assign($1, $$); }
        | expr5 T_STAR_A expr4                  { $$ = P_Star($1, $3); $$ = P_Assign($1, $$); }
        | expr5 T_SLASH_A expr4                 { $$ = P_Slash($1, $3); $$ = P_Assign($1, $$); }
        | expr5 T_PERCENT_A expr4               { $$ = P_Percent($1, $3); $$ = P_Assign($1, $$); }
        | expr5 T_CARET_A expr4                 { $$ = P_Caret($1, $3); $$ = P_Assign($1, $$); }
        | expr5 T_AMPER_A expr4                 { $$ = P_Amper($1, $3); $$ = P_Assign($1, $$); }
        | expr5 T_BAR_A expr4                   { $$ = P_Bar($1, $3); $$ = P_Assign($1, $$); }
        | expr5 T_HASH_A expr4                  { $$ = P_Hash($1, $3); $$ = P_Assign($1, $$); }
        | expr5 T_SHR_A expr4                   { $$ = P_Shr($1, $3); $$ = P_Assign($1, $$); }
        | expr5 T_SHL_A expr4                   { $$ = P_Shl($1, $3); $$ = P_Assign($1, $$); }
;

expr5   : expr6
		| expr5 T_BAR expr6                     { $$ = P_Bar($1, $3); }
;

expr6   : expr7
        | expr6 T_AMPER expr7                   { $$ = P_Amper($1, $3); }
;

expr7   : expr8
		| expr7 T_HASH expr8                    { $$ = P_Hash($1, $3);}
;

expr8   : expr9
        | expr8 T_SHR expr9                     { $$ = P_Shr($1, $3);}
        | expr8 T_SHL expr9                     { $$ = P_Shl($1, $3);}
;

expr9   : expr10
        | expr9 T_PLUS expr10                   { $$ = P_Plus($1, $3); }
        | expr9 T_MINUS expr10                  { $$ = P_Minus($1, $3); }
;

expr10  : expr11
        | expr10 T_STAR expr11                  { $$ = P_Star($1, $3); }
        | expr10 T_SLASH expr11                 { $$ = P_Slash($1, $3); }
        | expr10 T_PERCENT expr11               { $$ = P_Percent($1, $3); }
;

expr11  : expr12
		| expr11 T_CARET expr12                 { $$ = P_Caret($1, $3);}
;

expr12  : expr13
        | T_TILDE expr13                        { $$ = P_UTilde($2); }
        | T_MINUS expr13                        { $$ = P_UMinus($2); }
;

expr13  : expr14
;

expr14  : expr15
        | expr14 T_LPAREN						{ P_Invoke(); }
		    invargs T_RPAREN					{ $$ = P_EndInvoke($1, false); }
        | T_THREAD expr15 T_LPAREN				{ P_Invoke(); }
		    invargs T_RPAREN					{ $$ = P_EndInvoke($2, true); }
        | expr14 T_DOT member                   { $$ = P_Access($1, $3); }
        | expr14 T_LBRACK expr T_RBRACK         { $$ = P_Index($1, $3); }
;

expr15  : expr16
;

expr16  : expr17
;

expr17  : literal
        | T_THIS T_DOT member                   { $$ = P_Lookup($3); }
        | T_IDENT                               { $$ = P_Local($1); }
        | T_NEW member							{ P_New($2); $$ = P_EndNew(); }
        | T_NEW member T_LPAREN					{ P_New($2); }
		    invargl T_RPAREN					{ $$ = P_EndNew(); }
        | T_LPAREN expr T_RPAREN                { $$ = $2; }
;

invargs : /* empty */
        | invargl
;

invargl : expr                                  { P_ListItem($1); }
        | invargl T_COMMA expr                  { P_ListItem($3); }
;

literal : T_INTEGER                             { $$ = P_AddIntConst($1); }
        | T_FLOAT                               { $$ = P_AddFloatConst($1); }
        | strlit                                { $$ = P_AddStringConst($1); }
        | T_THIS                                { $$ = P_AddThisConst(); }
        | T_PATH                                { $$ = P_AddPathConst($1); }
        | T_NULL                                { $$ = P_AddNullConst(); }
        | T_LBRACK                              { P_BuildList(); }
          invargs T_RBRACK                      { $$ = P_EndListMap(); }
;

caselit : T_INTEGER                             { P_SwitchCaseInt($1); }
        | T_FLOAT                               { P_SwitchCaseFloat($1); }
        | strlit                                { P_SwitchCaseString($1); }
;

strlit	: T_STRING								{ $$ = $1; }
		| strlit T_STRING						{ $$ = $1; $$->Append($2); }
;

member	: T_IDENT
		| T_BREAK
		| T_CASE
		| T_CATCH
		| T_CONTINUE
		| T_DEFAULT
		| T_DELETE
		| T_DO
		| T_ELSE
		| T_EXIT
		| T_FOR
		| T_FOREACH
		| T_IF
		| T_IN
		| T_LOCK
		| T_NEW
		| T_NULL
		| T_REPEAT
		| T_RETURN
		| T_SWITCH
		| T_THIS
		| T_THREAD
		| T_THROW
		| T_TRY
		| T_UNTIL
		| T_WHILE
;

attrib  : attrib T_MINUS member                 { $$ = $1; $$->AppendCStr("-"); $$->Append($3); }
        | member                                { $$ = $1; }
;

declit	: T_INTEGER                             { P_AttrInt($1, 1); }
        | T_MINUS T_INTEGER                     { P_AttrInt($2, -1); }
        | T_FLOAT                               { P_AttrFloat($1, 1); }
        | T_MINUS T_FLOAT                       { P_AttrFloat($2, -1); }
        | strlit                                { P_AttrStr($1); }
        | T_NULL                                { P_AttrNull(); }
        | T_LBRACK                              { P_AttrList(); } arrlit T_RBRACK { P_EndAttrListMap(); }
        | T_LBRACE                              { P_AttrMap(); }  maplit T_RBRACE { P_EndAttrListMap(); }
        | T_PATH                                { P_AttrRef($1); }
;

arrlit : /* empty */
        | arrlitl
;

arrlitl : declit
        | arrlitl T_COMMA declit
;

maplit : /* empty */
        | maplitl
;

maplitl : declit T_COLON declit
        | maplitl T_COMMA declit T_COLON declit
;

%%
