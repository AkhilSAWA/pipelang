/* ============================================================
 * PipeLang lexical analyzer
 * Flex specification — generates tokens consumed by parser.y
 * ============================================================ */

%option noyywrap nounput noinput
%option yylineno
%option c++ prefix="PipeLang"

%{
#include <string>
#include <cstdlib>
#include "parser.hpp"

/* Bison-generated token types live in yy::parser::token */
using token = yy::parser::token;

/* Track column for nicer error messages */
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

"pipeline"      { return token::PIPELINE; }
"input"         { return token::INPUT; }
"clean"         { return token::CLEAN; }
"filter"        { return token::FILTER; }
"where"         { return token::WHERE; }
"transform"     { return token::TRANSFORM; }
"sort"          { return token::SORT; }
"group"         { return token::GROUP; }
"by"            { return token::BY; }
"aggregate"     { return token::AGGREGATE; }
"output"        { return token::OUTPUT; }
"ascending"     { return token::ASCENDING; }
"descending"    { return token::DESCENDING; }

">="            { return token::GE; }
"<="            { return token::LE; }
"=="            { return token::EQ; }
"!="            { return token::NE; }
">"             { return token::GT; }
"<"             { return token::LT; }
"+"             { return token::PLUS; }
"-"             { return token::MINUS; }
"*"             { return token::STAR; }
"/"             { return token::SLASH; }

"{"             { return token::LBRACE; }
"}"             { return token::RBRACE; }
"("             { return token::LPAREN; }
")"             { return token::RPAREN; }
","             { return token::COMMA; }
";"             { return token::SEMI; }

{FLOAT}         { yylval->fval = std::atof(yytext); return token::FLOAT_LIT; }
{INT}           { yylval->ival = std::atoi(yytext); return token::INT_LIT;   }
{STRING}        { yylval->sval = new std::string(yytext+1, yyleng-2);
                  return token::STRING_LIT; }
{ID}            { yylval->sval = new std::string(yytext, yyleng);
                  return token::IDENT; }

"//".*          { /* line comment */ }
[ \t\r]+        { /* whitespace */ }
\n              { g_column = 1; }

.               { std::fprintf(stderr,
                    "Lexical error: unexpected '%s' at line %d\n",
                    yytext, yylineno);
                }

%%
