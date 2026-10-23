%option noyywrap nodefault yylineno nounput noinput

%x BLOCK_COMMENT

%{
#include <string>

#include "parser.hpp"
#include "pipelang/driver.hpp"

/* Bison passes the token location (YYLTYPE) because %locations is enabled. */
#define YY_DECL int yylex(YYSTYPE* yylval, YYLTYPE* yylloc, pipelang::Driver& driver)

/* Give every token a source line before its rule action runs. yylineno is
   maintained by flex via %option yylineno. */
#define YY_USER_ACTION { \
    if (yylloc != nullptr) { \
        yylloc->first_line = yylineno; \
        yylloc->last_line = yylineno; \
    } \
}

/* Records the token for --dump-tokens and hands it to the parser. */
#define TOKEN(kind) do { \
    driver.note_token(#kind, std::string(yytext, yyleng), yylineno); \
    return kind; \
} while (0)

/* Value-carrying tokens also pass their text to the parser. */
#define VALUE_TOKEN(kind, value) do { \
    yylval->text = new std::string(value); \
    TOKEN(kind); \
} while (0)

static int comment_start_line = 0;
%}

WHITESPACE  [ \t\r]+
ID          [a-zA-Z_][a-zA-Z0-9_]*
INTEGER     [0-9]+
FLOAT       [0-9]+\.[0-9]+
STRING_BODY ([^\\\"\n]|\\.)*

%%

{WHITESPACE}                    ;
\n                              ;

"//"[^\n]*                       ;
"#"[^\n]*                        ;

"/*" {
    comment_start_line = yylineno;
    BEGIN(BLOCK_COMMENT);
}
<BLOCK_COMMENT>"*/"              BEGIN(INITIAL);
<BLOCK_COMMENT>[^*\n]+           ;
<BLOCK_COMMENT>"*"               ;
<BLOCK_COMMENT>\n                ;
<BLOCK_COMMENT><<EOF>> {
    driver.report(
        pipelang::DiagnosticKind::Lexical,
        comment_start_line,
        "unterminated block comment"
    );
    BEGIN(INITIAL);
    return 0;
}

"pipeline"                       TOKEN(PIPELINE);
"input"                          TOKEN(INPUT);
"clean"                          TOKEN(CLEAN);
"filter"                         TOKEN(FILTER);
"where"                          TOKEN(WHERE);
"transform"                      TOKEN(TRANSFORM);
"sort"                           TOKEN(SORT);
"group"                          TOKEN(GROUP);
"by"                             TOKEN(BY);
"aggregate"                      TOKEN(AGGREGATE);
"output"                         TOKEN(OUTPUT);
"ascending"                      TOKEN(ASCENDING);
"descending"                     TOKEN(DESCENDING);
"and"                            TOKEN(AND);
"or"                             TOKEN(OR);

">="                             TOKEN(GREATER_EQUAL);
"<="                             TOKEN(LESS_EQUAL);
"=="                             TOKEN(EQUAL_EQUAL);
"!="                             TOKEN(NOT_EQUAL);
">"                              TOKEN(GREATER);
"<"                              TOKEN(LESS);
"+"                              TOKEN(PLUS);
"-"                              TOKEN(MINUS);
"*"                              TOKEN(STAR);
"/"                              TOKEN(SLASH);

"{"                              TOKEN(LBRACE);
"}"                              TOKEN(RBRACE);
"("                              TOKEN(LPAREN);
")"                              TOKEN(RPAREN);
";"                              TOKEN(SEMICOLON);

{FLOAT}                          VALUE_TOKEN(FLOAT_LITERAL, yytext);
{INTEGER}                        VALUE_TOKEN(INTEGER_LITERAL, yytext);
{ID}                             VALUE_TOKEN(IDENTIFIER, yytext);

\"{STRING_BODY}\" {
    /* Strip the surrounding quotes; escape sequences are kept as written. */
    VALUE_TOKEN(
        STRING_LITERAL,
        std::string(yytext + 1, static_cast<std::size_t>(yyleng - 2))
    );
}

\"{STRING_BODY} {
    driver.report(
        pipelang::DiagnosticKind::Lexical,
        yylineno,
        "unterminated string literal"
    );
}

. {
    /* Report the character and keep scanning, so one stray character does
       not hide later errors. The driver fails the compilation afterwards. */
    driver.report(
        pipelang::DiagnosticKind::Lexical,
        yylineno,
        std::string("unexpected character '") + yytext[0] + "'"
    );
}

%%
