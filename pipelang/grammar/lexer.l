%option noyywrap nodefault yylineno nounput noinput

%{
#include <string>

#include "parser.hpp"
#include "pipelang/driver.hpp"

#define YY_DECL int yylex(YYSTYPE* yylval, pipelang::Driver& driver)

static std::string unquote(const char* text, int length) {
    return std::string(
        text + 1,
        static_cast<std::size_t>(length - 2)
    );
}
%}

WHITESPACE  [ \t\r]+
ID          [a-zA-Z_][a-zA-Z0-9_]*
INTEGER     [0-9]+
FLOAT       [0-9]+\.[0-9]+
STRING      \"([^\\\"]|\\.)*\"

%%

{WHITESPACE}                    ;
\n                              ;

"//"[^\n]*                       ;
"#"[^\n]*                        ;
"/*"([^*]|\*+[^*/])*\*+"/"       ;

"pipeline"                       return PIPELINE;
"input"                          return INPUT;
"clean"                          return CLEAN;
"filter"                         return FILTER;
"where"                          return WHERE;
"transform"                      return TRANSFORM;
"sort"                           return SORT;
"group"                          return GROUP;
"by"                             return BY;
"aggregate"                      return AGGREGATE;
"output"                         return OUTPUT;
"ascending"                      return ASCENDING;
"descending"                     return DESCENDING;

">="                             return GREATER_EQUAL;
"<="                             return LESS_EQUAL;
"=="                             return EQUAL_EQUAL;
"!="                             return NOT_EQUAL;
">"                              return GREATER;
"<"                              return LESS;
"+"                              return PLUS;
"-"                              return MINUS;
"*"                              return STAR;
"/"                              return SLASH;

{FLOAT} {
    yylval->text = new std::string(yytext, yyleng);
    driver.note_token("FLOAT", *yylval->text, yylineno);
    return FLOAT_LITERAL;
}

{INTEGER} {
    yylval->text = new std::string(yytext, yyleng);
    driver.note_token("INTEGER", *yylval->text, yylineno);
    return INTEGER_LITERAL;
}

{STRING} {
    yylval->text = new std::string(unquote(yytext, yyleng));
    driver.note_token("STRING", *yylval->text, yylineno);
    return STRING_LITERAL;
}

{ID} {
    yylval->text = new std::string(yytext, yyleng);
    driver.note_token("ID", *yylval->text, yylineno);
    return IDENTIFIER;
}

"{"                              return LBRACE;
"}"                              return RBRACE;
"("                              return LPAREN;
")"                              return RPAREN;
";"                              return SEMICOLON;
","                              return COMMA;

. {
    driver.error(
        yylineno,
        std::string("unexpected character '") + yytext[0] + "'"
    );
    return INVALID;
}

%%