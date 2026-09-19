/* ============================================================
 * PipeLang grammar (Bison C++ variant skeleton)
 * ============================================================ */

%skeleton "lalr1.cc"
%require  "3.5"
%language "c++"
%defines
%define api.value.type variant
%define api.token.constructor
%define parse.error verbose

/* ---------------------------------------------------------------
 * Anything the GENERATED parser.hpp needs to compile must go here.
 * pipelang::Driver appears in the parser's constructor signature.
 * --------------------------------------------------------------- */
%code requires {
    #include <string>
    #include <vector>
    #include <memory>
    #include "pipelang/ast.hpp"
    namespace pipelang { class Driver; }
}

/* Anything only the generated parser.cpp needs. */
%code {
    #include "pipelang/driver.hpp"

    /* Free-function yylex declaration; definition lives in the lexer. */
    namespace yy {
        parser::symbol_type yylex(pipelang::Driver& drv);
    }
}

%param { pipelang::Driver& drv }

/* -------------------- Tokens -------------------- */
%token                 END 0    "end of file"
%token <int>           INT_LIT
%token <double>        FLOAT_LIT
%token <std::string>   STRING_LIT
%token <std::string>   IDENT

%token PIPELINE INPUT CLEAN FILTER WHERE TRANSFORM SORT GROUP BY
       AGGREGATE OUTPUT ASCENDING DESCENDING
%token GE LE EQ NE GT LT PLUS MINUS STAR SLASH
%token LBRACE RBRACE LPAREN RPAREN COMMA SEMI

/* -------------------- Non-terminal types -------------------- */
%type <pipelang::PipelinePtr>          pipeline
%type <std::vector<pipelang::StmtPtr>> stmt_list
%type <pipelang::StmtPtr>              stmt
%type <pipelang::ExprPtr>              expr

%left EQ NE
%left LT GT LE GE
%left PLUS MINUS
%left STAR SLASH

%%

program
    : pipeline                          { drv.set_pipeline(std::move($1)); }
    ;

pipeline
    : PIPELINE IDENT LBRACE stmt_list RBRACE
        {
            $$ = std::make_unique<pipelang::Pipeline>($2, std::move($4));
        }
    ;

stmt_list
    : /* empty */
        { $$ = std::vector<pipelang::StmtPtr>{}; }
    | stmt_list stmt
        {
            $1.push_back(std::move($2));
            $$ = std::move($1);
        }
    ;

stmt
    : INPUT IDENT
        { $$ = std::make_unique<pipelang::Stmt>(pipelang::Op::Input, $2); }
    | CLEAN IDENT
        { $$ = std::make_unique<pipelang::Stmt>(pipelang::Op::Clean, $2); }
    | FILTER IDENT WHERE expr
        {
            auto s = std::make_unique<pipelang::Stmt>(pipelang::Op::Filter, $2);
            s->predicate = std::move($4);
            $$ = std::move(s);
        }
    | TRANSFORM IDENT IDENT
        {
            auto s = std::make_unique<pipelang::Stmt>(pipelang::Op::Transform, $2);
            s->argument = $3;
            $$ = std::move(s);
        }
    | SORT IDENT BY IDENT
        {
            auto s = std::make_unique<pipelang::Stmt>(pipelang::Op::Sort, $2);
            s->argument = $4;
            $$ = std::move(s);
        }
    | SORT IDENT BY IDENT ASCENDING
        {
            auto s = std::make_unique<pipelang::Stmt>(pipelang::Op::Sort, $2);
            s->argument = $4;
            $$ = std::move(s);
        }
    | SORT IDENT BY IDENT DESCENDING
        {
            auto s = std::make_unique<pipelang::Stmt>(pipelang::Op::Sort, $2);
            s->argument = $4;
            $$ = std::move(s);
        }
    | GROUP IDENT BY IDENT
        {
            auto s = std::make_unique<pipelang::Stmt>(pipelang::Op::Group, $2);
            s->argument = $4;
            $$ = std::move(s);
        }
    | AGGREGATE IDENT IDENT
        {
            auto s = std::make_unique<pipelang::Stmt>(pipelang::Op::Aggregate, $2);
            s->argument = $3;
            $$ = std::move(s);
        }
    | OUTPUT IDENT
        { $$ = std::make_unique<pipelang::Stmt>(pipelang::Op::Output, $2); }
    ;

expr
    : IDENT                    { $$ = pipelang::Expr::ident($1); }
    | INT_LIT                  { $$ = pipelang::Expr::intLit($1); }
    | FLOAT_LIT                { $$ = pipelang::Expr::floatLit($1); }
    | STRING_LIT               { $$ = pipelang::Expr::strLit($1); }
    | expr GT  expr            { $$ = pipelang::Expr::bin(">",  std::move($1), std::move($3)); }
    | expr LT  expr            { $$ = pipelang::Expr::bin("<",  std::move($1), std::move($3)); }
    | expr GE  expr            { $$ = pipelang::Expr::bin(">=", std::move($1), std::move($3)); }
    | expr LE  expr            { $$ = pipelang::Expr::bin("<=", std::move($1), std::move($3)); }
    | expr EQ  expr            { $$ = pipelang::Expr::bin("==", std::move($1), std::move($3)); }
    | expr NE  expr            { $$ = pipelang::Expr::bin("!=", std::move($1), std::move($3)); }
    | expr PLUS  expr          { $$ = pipelang::Expr::bin("+",  std::move($1), std::move($3)); }
    | expr MINUS expr          { $$ = pipelang::Expr::bin("-",  std::move($1), std::move($3)); }
    | expr STAR  expr          { $$ = pipelang::Expr::bin("*",  std::move($1), std::move($3)); }
    | expr SLASH expr          { $$ = pipelang::Expr::bin("/",  std::move($1), std::move($3)); }
    | LPAREN expr RPAREN       { $$ = std::move($2); }
    ;

%%

void yy::parser::error(const std::string& msg) {
    std::fprintf(stderr, "Parse error: %s\n", msg.c_str());
}
