%{
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "pipelang/ast.hpp"
#include "pipelang/driver.hpp"

void yyerror(pipelang::Driver& driver, const char* message);
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
int yylex(YYSTYPE* yylval, pipelang::Driver& driver);
}

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

%token PIPELINE INPUT CLEAN FILTER WHERE TRANSFORM SORT GROUP BY
%token AGGREGATE OUTPUT ASCENDING DESCENDING

%token LBRACE RBRACE LPAREN RPAREN SEMICOLON COMMA

%token GREATER LESS GREATER_EQUAL LESS_EQUAL
%token EQUAL_EQUAL NOT_EQUAL
%token PLUS MINUS STAR SLASH

%token INVALID

%token <text> IDENTIFIER INTEGER_LITERAL FLOAT_LITERAL STRING_LITERAL

%type <expr> expression primary
%type <stmt> statement
%type <statements> statements
%type <order> order

%left EQUAL_EQUAL NOT_EQUAL
%left GREATER LESS GREATER_EQUAL LESS_EQUAL
%left PLUS MINUS
%left STAR SLASH

%start program

%%

program
    : PIPELINE IDENTIFIER LBRACE statements RBRACE
      {
          driver.set_program(
              pipelang::make_pipeline(*$2, std::move(*$4))
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
              pipelang::make_input(*$2)
          );
          delete $2;
      }
    | CLEAN IDENTIFIER
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_clean(*$2)
          );
          delete $2;
      }
    | FILTER IDENTIFIER WHERE expression
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_filter(*$2, std::move(*$4))
          );
          delete $2;
          delete $4;
      }
    | TRANSFORM IDENTIFIER IDENTIFIER
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_transform(*$2, *$3)
          );
          delete $2;
          delete $3;
      }
    | SORT IDENTIFIER BY IDENTIFIER order
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_sort(*$2, *$4, $5)
          );
          delete $2;
          delete $4;
      }
    | GROUP IDENTIFIER BY IDENTIFIER
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_group(*$2, *$4)
          );
          delete $2;
          delete $4;
      }
    | AGGREGATE IDENTIFIER BY IDENTIFIER IDENTIFIER
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_aggregate(*$2, *$4, *$5)
          );
          delete $2;
          delete $4;
          delete $5;
      }
    | OUTPUT IDENTIFIER
      {
          $$ = new pipelang::StmtPtr(
              pipelang::make_output(*$2)
          );
          delete $2;
      }
    ;

order
    : /* empty */
      {
          $$ = pipelang::SortOrder::Ascending;
      }
    | ASCENDING
      {
          $$ = pipelang::SortOrder::Ascending;
      }
    | DESCENDING
      {
          $$ = pipelang::SortOrder::Descending;
      }
    ;

expression
    : primary
      {
          $$ = $1;
      }
    | expression GREATER expression
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_binary(
                  ">",
                  std::move(*$1),
                  std::move(*$3)
              )
          );
          delete $1;
          delete $3;
      }
    | expression LESS expression
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_binary(
                  "<",
                  std::move(*$1),
                  std::move(*$3)
              )
          );
          delete $1;
          delete $3;
      }
    | expression GREATER_EQUAL expression
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_binary(
                  ">=",
                  std::move(*$1),
                  std::move(*$3)
              )
          );
          delete $1;
          delete $3;
      }
    | expression LESS_EQUAL expression
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_binary(
                  "<=",
                  std::move(*$1),
                  std::move(*$3)
              )
          );
          delete $1;
          delete $3;
      }
    | expression EQUAL_EQUAL expression
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_binary(
                  "==",
                  std::move(*$1),
                  std::move(*$3)
              )
          );
          delete $1;
          delete $3;
      }
    | expression NOT_EQUAL expression
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_binary(
                  "!=",
                  std::move(*$1),
                  std::move(*$3)
              )
          );
          delete $1;
          delete $3;
      }
    | expression PLUS expression
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_binary(
                  "+",
                  std::move(*$1),
                  std::move(*$3)
              )
          );
          delete $1;
          delete $3;
      }
    | expression MINUS expression
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_binary(
                  "-",
                  std::move(*$1),
                  std::move(*$3)
              )
          );
          delete $1;
          delete $3;
      }
    | expression STAR expression
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_binary(
                  "*",
                  std::move(*$1),
                  std::move(*$3)
              )
          );
          delete $1;
          delete $3;
      }
    | expression SLASH expression
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_binary(
                  "/",
                  std::move(*$1),
                  std::move(*$3)
              )
          );
          delete $1;
          delete $3;
      }
    ;

primary
    : IDENTIFIER
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_identifier(*$1)
          );
          delete $1;
      }
    | INTEGER_LITERAL
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_integer(*$1)
          );
          delete $1;
      }
    | FLOAT_LITERAL
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_float(*$1)
          );
          delete $1;
      }
    | STRING_LITERAL
      {
          $$ = new pipelang::ExprPtr(
              pipelang::make_string(*$1)
          );
          delete $1;
      }
    | LPAREN expression RPAREN
      {
          $$ = $2;
      }
    ;

%%

void yyerror(pipelang::Driver& driver, const char* message) {
    driver.error(0, message);
}