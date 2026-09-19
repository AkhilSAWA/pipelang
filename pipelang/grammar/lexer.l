/* ============================================================
 * PipeLang lexical analyzer (Flex, C output)
 * Emits Bison C++ variant symbol_types via make_XXX(...) helpers.
 * ============================================================ */

%option noyywrap nounput noinput
%option yylineno

%{
#include <string>
#include <cstdlib>
#include "parser.hpp"
#include "pipelang/driver.hpp"

/* Shorthand for the generated symbol_type constructors. */
using yy::parser;

/* Bison declared yylex inside namespace yy; match that here. */
#undef  YY_DECL
#define YY_DECL yy::parser::symbol_type yy::yylex(pipelang::Driver& drv)

/* Forward-declare so the generated lexer.cpp sees the same signature. */
namespace yy { parser::symbol_type yylex(pipelang::Driver& drv); }

/* Track column for nicer error messages (currently unused). */
static int g_column = 1;
#define YY_USER_ACTION g_column += yyleng;
%}

DIGIT   [0-9]
LETTER  [a-zA-Z_]
ID      {LETTER}({LETTER}|{DIGIT})*
INT     {DIGIT}+
FLOAT   {DIGIT}+"."{DIGIT}+
STRING  \"([^\"\\]|\\.)*\"

%%

"pipeline"      { return parser::make_PIPELINE(); }
"input"         { return parser::make_INPUT(); }
"clean"         { return parser::make_CLEAN(); }
"filter"        { return parser::make_FILTER(); }
"where"         { return parser::make_WHERE(); }
"transform"     { return parser::make_TRANSFORM(); }
"sort"          { return parser::make_SORT(); }
"group"         { return parser::make_GROUP(); }
"by"            { return parser::make_BY(); }
"aggregate"     { return parser::make_AGGREGATE(); }
"output"        { return parser::make_OUTPUT(); }
"ascending"     { return parser::make_ASCENDING(); }
"descending"    { return parser::make_DESCENDING(); }

">="            { return parser::make_GE(); }
"<="            { return parser::make_LE(); }
"=="            { return parser::make_EQ(); }
"!="            { return parser::make_NE(); }
">"             { return parser::make_GT(); }
"<"             { return parser::make_LT(); }
"+"             { return parser::make_PLUS(); }
"-"             { return parser::make_MINUS(); }
"*"             { return parser::make_STAR(); }
"/"             { return parser::make_SLASH(); }

"{"             { return parser::make_LBRACE(); }
"}"             { return parser::make_RBRACE(); }
"("             { return parser::make_LPAREN(); }
")"             { return parser::make_RPAREN(); }
","             { return parser::make_COMMA(); }
";"             { return parser::make_SEMI(); }

{FLOAT}         { return parser::make_FLOAT_LIT(std::atof(yytext)); }
{INT}           { return parser::make_INT_LIT(std::atoi(yytext)); }
{STRING}        { return parser::make_STRING_LIT(std::string(yytext+1, yyleng-2)); }
{ID}            { return parser::make_IDENT(std::string(yytext, yyleng)); }

"//".*          { /* line comment */ }
[ \t\r]+        { /* whitespace */ }
\n              { g_column = 1; }

.               { std::fprintf(stderr,
                    "Lexical error: unexpected '%s' at line %d\n",
                    yytext, yylineno);
                }

<<EOF>>         { return parser::make_END(); }

%%
