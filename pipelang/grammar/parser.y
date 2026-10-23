%{
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "pipelang/ast.hpp"
#include "pipelang/driver.hpp"

/* YYLTYPE is defined later in parser.hpp; a forward declaration is enough
   for the pointer parameters used here. */
struct YYLTYPE;

void yyerror(YYLTYPE* location, pipelang::Driver& driver, const char* message);

/* Semantic values are heap pointers (a %union cannot hold unique_ptr).
   These helpers take ownership of the operands and return a new node. */
static pipelang::ExprPtr* binary(
    const char* op,
    pipelang::ExprPtr* left,
    pipelang::ExprPtr* right
) {
    auto* node = new pipelang::ExprPtr(
        pipelang::make_binary(op, std::move(*left), std::move(*right))
    );
    delete left;
    delete right;
    return node;
}

static pipelang::ExprPtr* unary(const char* op, pipelang::ExprPtr* operand) {
    auto* node = new pipelang::ExprPtr(
        pipelang::make_unary(op, std::move(*operand))
    );
    delete operand;
    return node;
}

static pipelang::ExprPtr* leaf(
    pipelang::ExprPtr (*make)(std::string),
    std::string* text
) {
    auto* node = new pipelang::ExprPtr(make(std::move(*text)));
    delete text;
    return node;
}
%}

%code requires {
#include <memory>
#include <string>
#include <vector>

#include "pipelang/ast.hpp"

namespace pipelang {
class Driver;
}
}

%code provides {
/* With %locations and a pure parser, Bison passes the token location
   to the lexer and to yyerror. */
int yylex(YYSTYPE* yylval, YYLTYPE* yylloc, pipelang::Driver& driver);
}

%locations
%define parse.error detailed
%define api.pure full

%parse-param { pipelang::Driver& driver }
%lex-param   { pipelang::Driver& driver }

%union {
    std::string* text;
    pipelang::ExprPtr* expr;
    pipelang::StmtPtr* stmt;
    std::vector<pipelang::StmtPtr>* statements;
    pipelang::SortOrder order;
}

/* Frees semantic values that Bison discards while handling a syntax error. */
%destructor { delete $$; } <text> <expr> <stmt> <statements>

/* The quoted aliases are what syntax errors show, e.g. expecting "where". */
%token PIPELINE "pipeline" INPUT "input" CLEAN "clean" FILTER "filter"
%token WHERE "where" TRANSFORM "transform" SORT "sort" GROUP "group" BY "by"
%token AGGREGATE "aggregate" OUTPUT "output"
%token ASCENDING "ascending" DESCENDING "descending" AND "and" OR "or"

%token LBRACE "{" RBRACE "}" LPAREN "(" RPAREN ")" SEMICOLON ";"

%token GREATER ">" LESS "<" GREATER_EQUAL ">=" LESS_EQUAL "<="
%token EQUAL_EQUAL "==" NOT_EQUAL "!="
%token PLUS "+" MINUS "-" STAR "*" SLASH "/"

%token <text> IDENTIFIER "identifier"
%token <text> INTEGER_LITERAL "integer"
%token <text> FLOAT_LITERAL "float"
%token <text> STRING_LITERAL "string"

%type <expr> expression primary
%type <stmt> statement
%type <statements> statements
%type <order> order

/* Lowest to highest precedence. */
%left OR
%left AND
%left EQUAL_EQUAL NOT_EQUAL
%left GREATER LESS GREATER_EQUAL LESS_EQUAL
%left PLUS MINUS
%left STAR SLASH
%precedence UNARY_MINUS

%start program

%%

program
    : PIPELINE IDENTIFIER LBRACE statements RBRACE
      {
          driver.set_program(
              pipelang::make_pipeline(*$2, std::move(*$4), @1.first_line)
          );

          delete $2;
          delete $4;
      }
    ;

statements
    : /* empty */
      {
          $$ = new std::vector<pipelang::StmtPtr>();
      }
    | statements statement optional_separator
      {
          $1->push_back(std::move(*$2));
          delete $2;
          $$ = $1;
      }
    ;

optional_separator
    : /* empty */
    | SEMICOLON
    ;

statement
    : INPUT IDENTIFIER
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_input(*$2, @$.first_line)
          );
          delete $2;
      }
    | CLEAN IDENTIFIER
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_clean(*$2, @$.first_line)
          );
          delete $2;
      }
    | FILTER IDENTIFIER WHERE expression
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_filter(*$2, std::move(*$4), @$.first_line)
          );
          delete $2;
          delete $4;
      }
    | TRANSFORM IDENTIFIER IDENTIFIER
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_transform(*$2, *$3, @$.first_line)
          );
          delete $2;
          delete $3;
      }
    | SORT IDENTIFIER BY IDENTIFIER order
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_sort(*$2, *$4, $5, @$.first_line)
          );
          delete $2;
          delete $4;
      }
    | GROUP IDENTIFIER BY IDENTIFIER
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_group(*$2, *$4, @$.first_line)
          );
          delete $2;
          delete $4;
      }
    | AGGREGATE IDENTIFIER BY IDENTIFIER IDENTIFIER
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_aggregate(*$2, *$4, *$5, @$.first_line)
          );
          delete $2;
          delete $4;
          delete $5;
      }
    | OUTPUT IDENTIFIER
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_output(*$2, @$.first_line)
          );
          delete $2;
      }
    ;

order
    : /* empty */  { $$ = pipelang::SortOrder::Ascending; }
    | ASCENDING    { $$ = pipelang::SortOrder::Ascending; }
    | DESCENDING   { $$ = pipelang::SortOrder::Descending; }
    ;

expression
    : primary                                     { $$ = $1; }
    | expression OR expression                    { $$ = binary("or", $1, $3); }
    | expression AND expression                   { $$ = binary("and", $1, $3); }
    | expression EQUAL_EQUAL expression           { $$ = binary("==", $1, $3); }
    | expression NOT_EQUAL expression             { $$ = binary("!=", $1, $3); }
    | expression GREATER expression               { $$ = binary(">", $1, $3); }
    | expression LESS expression                  { $$ = binary("<", $1, $3); }
    | expression GREATER_EQUAL expression         { $$ = binary(">=", $1, $3); }
    | expression LESS_EQUAL expression            { $$ = binary("<=", $1, $3); }
    | expression PLUS expression                  { $$ = binary("+", $1, $3); }
    | expression MINUS expression                 { $$ = binary("-", $1, $3); }
    | expression STAR expression                  { $$ = binary("*", $1, $3); }
    | expression SLASH expression                 { $$ = binary("/", $1, $3); }
    | MINUS expression %prec UNARY_MINUS          { $$ = unary("-", $2); }
    ;

primary
    : IDENTIFIER                { $$ = leaf(pipelang::make_identifier, $1); }
    | INTEGER_LITERAL           { $$ = leaf(pipelang::make_integer, $1); }
    | FLOAT_LITERAL             { $$ = leaf(pipelang::make_float, $1); }
    | STRING_LITERAL            { $$ = leaf(pipelang::make_string, $1); }
    | LPAREN expression RPAREN  { $$ = $2; }
    ;

%%

void yyerror(YYLTYPE* location, pipelang::Driver& driver, const char* message) {
    /* A syntax error after a lexical error (e.g. an unterminated string) is
       almost always a consequence of it, so report only the lexical error. */
    for (const pipelang::Diagnostic& diagnostic : driver.diagnostics()) {
        if (diagnostic.kind == pipelang::DiagnosticKind::Lexical) {
            return;
        }
    }

    const int line = location != nullptr ? location->first_line : 0;

    /* Bison's messages start with "syntax error"; the diagnostic kind
       already says that, so keep only the detail. */
    const char* prefix = "syntax error, ";
    std::string detail = message;
    if (detail.rfind(prefix, 0) == 0) {
        detail.erase(0, std::strlen(prefix));
    }

    driver.report(pipelang::DiagnosticKind::Syntax, line, detail);
}
