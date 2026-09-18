/* ============================================================
 * PipeLang grammar (Bison, C++ skeleton)
 * Produces AST nodes defined in include/pipelang/ast.hpp
 * ============================================================ */

%skeleton "lalr1.cc"
%require  "3.5"
%language "c++"
%defines
%define api.value.type variant
%define parse.error verbose
%define parse.trace

%code requires {
    #include <string>
    #include <vector>
    #include <memory>
    #include "pipelang/ast.hpp"
}

%code {
    #include <cstdio>
    #include <FlexLexer.h>
    #include "pipelang/driver.hpp"

    /* Bison expects a free function yylex; we forward to the C++ lexer
     * held by the Driver. */
    static yy::parser::symbol_type yylex(pipelang::Driver& drv);
}

%param { pipelang::Driver& drv }

/* -------------------- Tokens -------------------- */
%token <int>          INT_LIT
%token <double>       FLOAT_LIT
%token <std::string*> STRING_LIT IDENT

%token PIPELINE INPUT CLEAN FILTER WHERE TRANSFORM SORT GROUP BY
       AGGREGATE OUTPUT ASCENDING DESCENDING
%token GE LE EQ NE GT LT PLUS MINUS STAR SLASH
%token LBRACE RBRACE LPAREN RPAREN COMMA SEMI

/* -------------------- Non-terminal types -------------------- */
%type <pipelang::PipelinePtr>         pipeline
%type <std::vector<pipelang::StmtPtr>> stmt_list
%type <pipelang::StmtPtr>             stmt
%type <pipelang::ExprPtr>             expr

%left PLUS MINUS
%left STAR SLASH

%%

program
    : pipeline                          { drv.set_pipeline(std::move($1)); }
    ;

pipeline
    : PIPELINE IDENT LBRACE stmt_list RBRACE
        {
            $$ = std::make_unique<pipelang::Pipeline>(*$2, std::move($4));
            delete $2;
        }
    ;

stmt_list
    : /* empty */              { $$ = {}; }
    | stmt_list stmt           { $1.push_back(std::move($2)); $$ = std::move($1); }
    ;

stmt
    : INPUT IDENT
        { $$ = std::make_unique<pipelang::Stmt>(pipelang::Op::Input, *$2); delete $2; }
    | CLEAN IDENT
        { $$ = std::make_unique<pipelang::Stmt>(pipelang::Op::Clean, *$2); delete $2; }
    | FILTER IDENT WHERE expr
        { auto s = std::make_unique<pipelang::Stmt>(pipelang::Op::Filter, *$2);
          s->predicate = std::move($4); delete $2; $$ = std::move(s); }
    | TRANSFORM IDENT IDENT
        { auto s = std::make_unique<pipelang::Stmt>(pipelang::Op::Transform, *$2);
          s->argument = *$3; delete $2; delete $3; $$ = std::move(s); }
    | SORT IDENT BY IDENT
        { auto s = std::make_unique<pipelang::Stmt>(pipelang::Op::Sort, *$2);
          s->argument = *$4; delete $2; delete $4; $$ = std::move(s); }
    | GROUP IDENT BY IDENT
        { auto s = std::make_unique<pipelang::Stmt>(pipelang::Op::Group, *$2);
          s->argument = *$4; delete $2; delete $4; $$ = std::move(s); }
    | AGGREGATE IDENT IDENT
        { auto s = std::make_unique<pipelang::Stmt>(pipelang::Op::Aggregate, *$2);
          s->argument = *$3; delete $2; delete $3; $$ = std::move(s); }
    | OUTPUT IDENT
        { $$ = std::make_unique<pipelang::Stmt>(pipelang::Op::Output, *$2); delete $2; }
    ;

expr
    : IDENT                    { $$ = pipelang::Expr::ident(*$1); delete $1; }
    | INT_LIT                  { $$ = pipelang::Expr::intLit($1); }
    | FLOAT_LIT                { $$ = pipelang::Expr::floatLit($1); }
    | STRING_LIT               { $$ = pipelang::Expr::strLit(*$1); delete $1; }
    | expr GT  expr            { $$ = pipelang::Expr::bin(">",  std::move($1), std::move($3)); }
    | expr LT  expr            { $$ = pipelang::Expr::bin("<",  std::move($1), std::move($3)); }
    | expr GE  expr            { $$ = pipelang::Expr::bin(">=", std::move($1), std::move($3)); }
    | expr LE  expr            { $$ = pipelang::Expr::bin("<=", std::move($1), std::move($3)); }
    | expr EQ  expr            { $$ = pipelang::Expr::bin("==", std::move($1), std::move($3)); }
    | expr NE  expr            { $$ = pipelang::Expr::bin("!=", std::move($1), std::move($3)); }
    | expr PLUS  expr          { $$ = pipelang::Expr::bin("+", std::move($1), std::move($3)); }
    | expr MINUS expr          { $$ = pipelang::Expr::bin("-", std::move($1), std::move($3)); }
    | expr STAR  expr          { $$ = pipelang::Expr::bin("*", std::move($1), std::move($3)); }
    | expr SLASH expr          { $$ = pipelang::Expr::bin("/", std::move($1), std::move($3)); }
    | LPAREN expr RPAREN       { $$ = std::move($2); }
    ;

%%

void yy::parser::error(const std::string& msg) {
    std::fprintf(stderr, "Parse error: %s\n", msg.c_str());
}

/* Bridge from Bison's yylex to the C++ FlexLexer held by the Driver */
static yy::parser::symbol_type yylex(pipelang::Driver& drv) {
    /* Not used — we use %skeleton lalr1.cc with a symbol_type based lexer only
     * if we set api.token.constructor. In this scaffold we keep the classic
     * union-style value stack and rely on lexer.l returning token codes with
     * yylval set. See driver.cpp for the actual parse invocation. */
    return yy::parser::symbol_type{};
}
