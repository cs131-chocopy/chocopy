%locations
%define api.location.type {::parser::Location}

%{
#define YYLLOC_DEFAULT(Cur, Rhs, N)
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <string>
#include <iostream>
#include <chocopy_parse.hpp>
#include <chocopy_ast.hpp>

using ::parser::Location;

/** external functions from lex */
extern void yyrestart(FILE*);
extern int yylex();
extern int yyparse();
extern FILE* yyin;
std::unique_ptr<::parser::Program> ROOT = std::make_unique<::parser::Program>(Location());

/** Error reporting */
void yyerror(const char *s);


/** Return a mutable list initially containing the single value ITEM. */
template<typename T>
std::vector<T *>* single(T *item) {
    std::vector<T *> *list=new std::vector<T *>();
    list->push_back(item);
    return list;
}

/* Aappend item to the end of LIST. Then returns LIST. */
template<typename T, typename X>
T* combine(T* list, X *item) {
    list->emplace_back(item);
    return list;
}
%}


%union {
  char *raw_str;
  int raw_int;
  ::parser::Program *PtrProgram;
  ::parser::Stmt *PtrStmt;
  ::parser::Decl *PtrDecl;
  ::parser::AssignStmt *PtrAssignStmt;
  ::parser::BinaryExpr *PtrBinaryExpr;
  ::parser::BoolLiteral *PtrBoolLiteral;
  ::parser::CallExpr *PtrCallExpr;
  ::parser::ClassDef *PtrClassDef;
  ::parser::ClassType *PtrClassType;
  ::parser::Expr *PtrExpr;
  ::parser::ExprStmt *PtrExprStmt;
  ::parser::ForStmt *PtrForStmt;
  ::parser::FuncDef *PtrFuncDef;
  ::parser::GlobalDecl *PtrGlobalDecl;
  ::parser::Ident *PtrIdent;
  ::parser::IfExpr *PtrIfExpr;
  ::parser::IndexExpr *PtrIndexExpr;
  ::parser::IntegerLiteral *PtrIntegerLiteral;
  ::parser::ListExpr *PtrListExpr;
  ::parser::ListType *PtrListType;
  ::parser::Literal *PtrLiteral;
  ::parser::MemberExpr *PtrMemberExpr;
  ::parser::MethodCallExpr *PtrMethodCallExpr;
  ::parser::Node *PtrNode;
  ::parser::NoneLiteral *PtrNoneLiteral;
  ::parser::NonlocalDecl *PtrNonlocalDecl;
  ::parser::ReturnStmt *PtrReturnStmt;
  ::parser::StringLiteral *PtrStringLiteral;
  ::parser::TypeAnnotation *PtrTypeAnnotation;
  ::parser::TypedVar *PtrTypedVar;
  ::parser::UnaryExpr *PtrUnaryExpr;
  ::parser::VarDef *PtrVarDef;
  ::parser::WhileStmt *PtrWhileStmt;
  ::parser::IfStmt *PtrIfStmt;
  std::vector<std::unique_ptr<::parser::Decl>> *PtrListDecl;
  std::vector<std::unique_ptr<::parser::Stmt>> *PtrListStmt;
  std::vector<std::unique_ptr<::parser::TypedVar>> *PtrListTypedVar;
  std::vector<std::unique_ptr<::parser::Expr>> *PtrExprList;
}


/* declare tokens */
%token <raw_int> TOKEN_INTEGER
%token <raw_str> TOKEN_IDENTIFIER
%token <raw_str> TOKEN_STRING
%token TOKEN_TRUE
%token TOKEN_FALSE
%token TOKEN_AND
%token TOKEN_BREAK
%token TOKEN_DEF
%token TOKEN_ELIF
%token TOKEN_ELSE
%token TOKEN_FOR
%token TOKEN_IF
%token TOKEN_NOT
%token TOKEN_OR
%token TOKEN_RETURN
%token TOKEN_WHILE
%token TOKEN_NONE
%token TOKEN_AS
%token TOKEN_CLASS
%token TOKEN_GLOBAL
%token TOKEN_IN
%token TOKEN_IS
%token TOKEN_NONLOCAL
%token TOKEN_PASS
%token TOKEN_plus
%token TOKEN_minus
%token TOKEN_star
%token TOKEN_slash
%token TOKEN_percent
%token TOKEN_less
%token TOKEN_greater
%token TOKEN_lessequal
%token TOKEN_greaterequal
%token TOKEN_equalequal
%token TOKEN_exclaimequal
%token TOKEN_equal
%token TOKEN_l_paren
%token TOKEN_r_paren
%token TOKEN_l_square
%token TOKEN_r_square
%token TOKEN_comma
%token TOKEN_colon
%token TOKEN_period
%token TOKEN_rarrow
%token TOKEN_INDENT
%token TOKEN_DEDENT
%token TOKEN_NEWLINE

%type <PtrProgram> program
%type <PtrStmt> stmt simple_stmt
%type <PtrIfStmt> elif_list
%type <PtrListDecl> top_level_decl class_body func_decls
%type <PtrListStmt> stmt_list block
%type <PtrListTypedVar> typed_var_list
%type <PtrClassDef> class_def
%type <PtrGlobalDecl> global_decl
%type <PtrTypedVar> typed_var
%type <PtrTypeAnnotation> type func_return_type
%type <PtrExpr> expr expr_no_if cexpr target
%type <PtrExprList> expr_list
%type <PtrIndexExpr> index_expr
%type <PtrMemberExpr> member_expr
%type <PtrLiteral> literal
%type <PtrIdent> identifier
%type <PtrVarDef> var_def
%type <PtrFuncDef> func_def
%type <PtrNonlocalDecl> nonlocal_decl
%type <PtrAssignStmt> assign_stmt


%left TOKEN_OR
%left TOKEN_AND
%left TOKEN_NOT
%nonassoc TOKEN_equalequal TOKEN_exclaimequal TOKEN_greater TOKEN_greaterequal TOKEN_less TOKEN_lessequal TOKEN_IS
%left TOKEN_plus TOKEN_minus
%left TOKEN_star TOKEN_slash TOKEN_percent
%left UMINUS
%right TOKEN_period TOKEN_comma TOKEN_l_square TOKEN_r_square

%start program

%%
/* The grammar rules Your Code Here */

program :
    top_level_decl stmt_list {
        @$ = {$1->size() ? $1->front()->location : $2->front()->location, $2->back()->location};
        ROOT = std::move(std::make_unique<::parser::Program>(@$, $1, $2));
    }
    | top_level_decl {
        if ($1->size() > 0) {
            @$ = {$1->front()->location, $1->back()->location};
        }
        ROOT = std::move(std::make_unique<::parser::Program>(@$, $1));
    }
top_level_decl : { $$ = new std::vector<std::unique_ptr<::parser::Decl>>(); }
    | top_level_decl var_def { $$ = combine($1, $2); }
    | top_level_decl func_def { $$ = combine($1, $2); }
    | top_level_decl class_def { $$ = combine($1, $2); }

stmt_list :
    stmt {
        $$ = new std::vector<std::unique_ptr<::parser::Stmt>>();
        $$->emplace_back($1);
        @$ = @1;
    }
    | stmt_list stmt { $$ = combine($1, $2); @$ = {@1, @2}; }

class_def :
    TOKEN_CLASS identifier TOKEN_l_paren identifier TOKEN_r_paren TOKEN_colon TOKEN_NEWLINE TOKEN_INDENT class_body TOKEN_DEDENT {
        $$ = new parser::ClassDef(@$ = {@1, @9}, $2, $4, $9);
    }
    | TOKEN_CLASS identifier TOKEN_l_paren identifier TOKEN_r_paren TOKEN_colon TOKEN_NEWLINE TOKEN_INDENT TOKEN_PASS TOKEN_NEWLINE TOKEN_DEDENT {
        $$ = new parser::ClassDef(@$ = {@1, @9}, $2, $4, new std::vector<std::unique_ptr<::parser::Decl>>());
    }
class_body :
    var_def {
        $$ = new std::vector<std::unique_ptr<::parser::Decl>>();
        $$->emplace_back($1);
        @$ = @1;
    }
    | func_def {
        $$ = new std::vector<std::unique_ptr<::parser::Decl>>();
        $$->emplace_back($1);
        @$ = @1;
    }
    | class_body var_def { $$ = combine($$, (parser::Decl *)$2); @$ = {@1, @2}; }
    | class_body func_def { $$ = combine($$, (parser::Decl *)$2); @$ = {@1, @2}; }

func_def : TOKEN_DEF identifier TOKEN_l_paren typed_var_list TOKEN_r_paren func_return_type TOKEN_colon TOKEN_NEWLINE TOKEN_INDENT func_decls stmt_list TOKEN_DEDENT {
    $$ = new parser::FuncDef(@$ = {@1, @11}, $2, $4, $6, $10, $11);
}
typed_var_list:  { $$ = new std::vector<std::unique_ptr<::parser::TypedVar>>(); }
    | typed_var {
        $$ = new std::vector<std::unique_ptr<::parser::TypedVar>>();
        $$->emplace_back($1);
    }
    | typed_var_list TOKEN_comma typed_var { $$ = combine($1, $3); }
func_return_type: { $$ = new parser::ClassType(@$ = yylloc, "<None>"); }
    | TOKEN_rarrow type { $$ = $2; @$ = {@1, @2}; }
func_decls: { $$ = new std::vector<std::unique_ptr<::parser::Decl>>(); }
    | func_decls global_decl { $$ = combine($1, $2); }
    | func_decls nonlocal_decl { $$ = combine($1, $2); }
    | func_decls var_def { $$ = combine($1, $2); }
    | func_decls func_def { $$ = combine($1, $2); }

typed_var : identifier TOKEN_colon type { $$ = new parser::TypedVar(@$ = {@1, @3}, $1, $3); }
type :
    TOKEN_IDENTIFIER { $$ = new parser::ClassType(@$ = yylloc, string($1)); free($1); }
    | TOKEN_STRING { $$ = new parser::ClassType(@$ = yylloc, string($1)); delete[] $1; }
    | TOKEN_l_square type TOKEN_r_square { $$ = new parser::ListType(@$ = {@1, @3}, $2); }

global_decl : TOKEN_GLOBAL identifier TOKEN_NEWLINE { $$ = new parser::GlobalDecl(@$ = {@1, @2}, $2); }
nonlocal_decl : TOKEN_NONLOCAL identifier TOKEN_NEWLINE { $$ = new parser::NonlocalDecl(@$ = {@1, @2}, $2); }
var_def : typed_var TOKEN_equal literal TOKEN_NEWLINE { $$ = new parser::VarDef(@$ = {@1, @3}, $1, $3); }

stmt :
    simple_stmt TOKEN_NEWLINE { $$ = $1; @$ = @1; }
    | TOKEN_WHILE expr TOKEN_colon block {
        $$ = new parser::WhileStmt(@$ = {@1, @4}, $2, $4);
    }
    | TOKEN_FOR identifier TOKEN_IN expr TOKEN_colon block { $$ = new parser::ForStmt(@$ = {@1, @6}, $2, $4, $6); }
    | TOKEN_IF expr TOKEN_colon block { $$ = new parser::IfStmt(@$ = {@1, @4}, $2, $4); }
    | TOKEN_IF expr TOKEN_colon block elif_list { $$ = new parser::IfStmt(@$ = {@1, @5}, $2, $4, $5); }
    | TOKEN_IF expr TOKEN_colon block TOKEN_ELSE TOKEN_colon block { $$ = new parser::IfStmt(@$ = {@1, @7}, $2, $4, $7); }
    | assign_stmt { std::reverse($1->targets.begin(), $1->targets.end()); $$ = $1; @$ = @1; }
elif_list :
    TOKEN_ELIF expr TOKEN_colon block elif_list { $$ = new parser::IfStmt(@$ = {@1, @5}, $2, $4, $5); }
    | TOKEN_ELIF expr TOKEN_colon block TOKEN_ELSE TOKEN_colon block { $$ = new parser::IfStmt(@$ = {@1, @7}, $2, $4, $7); }
    | TOKEN_ELIF expr TOKEN_colon block { $$ = new parser::IfStmt(@$ = {@1, @4}, $2, $4); }

simple_stmt :
    TOKEN_PASS { $$ = new parser::PassStmt(@$ = yylloc); }
    | expr { $$ = new parser::ExprStmt(@$ = @1, $1); }
    | TOKEN_RETURN { $$ = new parser::ReturnStmt(@$ = yylloc); }
    | TOKEN_RETURN expr { $$ = new parser::ReturnStmt(@1 /* TODO: fix */, $2); }

block : TOKEN_NEWLINE TOKEN_INDENT stmt_list TOKEN_DEDENT  { $$ = $3; @$ = @3; }

literal :
    TOKEN_INTEGER { $$ = new parser::IntegerLiteral(@$ = yylloc, $1); }
    | TOKEN_TRUE { $$ = new parser::BoolLiteral(@$ = yylloc, true); }
    | TOKEN_FALSE { $$ = new parser::BoolLiteral(@$ = yylloc, false); }
    | TOKEN_NONE { $$ = new parser::NoneLiteral(@$ = yylloc); }
    | TOKEN_STRING { $$ = new parser::StringLiteral(@$ = yylloc, string($1)); delete[] $1; }

expr :
    expr_no_if { $$ = $1; @$ = @1; }
    | expr_no_if TOKEN_IF expr TOKEN_ELSE expr {
        $$ = new parser::IfExpr(@$ = {@1, @5}, $3, $1, $5);
    }

expr_no_if :
    cexpr { $$ = $1; @$ = @1; }
    | TOKEN_NOT expr_no_if { $$ = new parser::UnaryExpr(@$ = {@1, @2}, std::string("not"), $2); }
    | expr_no_if TOKEN_AND expr_no_if { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string("and"), $3); }
    | expr_no_if TOKEN_OR expr_no_if { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string("or"), $3); }

expr_list : { $$ = new std::vector<std::unique_ptr<::parser::Expr>>(); }
    | expr {
        $$ = new std::vector<std::unique_ptr<::parser::Expr>>();
        $$->emplace_back($1);
    }
    | expr_list TOKEN_comma expr {
        $$ = combine($1, $3);
    }

cexpr :
    identifier { $$ = $1; @$ = @1; }
    | literal { $$ = $1; @$ = @1; }
    | TOKEN_l_square expr_list TOKEN_r_square { $$ = new parser::ListExpr(@$ = {@1, @3}, $2); }
    | TOKEN_l_paren expr TOKEN_r_paren { $$ = $2; $$->location = @$ = @1; }
    | member_expr { $$ = $1; @$ = @1; }
    | index_expr { $$ = $1; @$ = @1; }
    | member_expr TOKEN_l_paren expr_list TOKEN_r_paren { $$ = new parser::MethodCallExpr(@$ = {@1, @4}, $1, $3); }
    | identifier TOKEN_l_paren expr_list TOKEN_r_paren { $$ = new parser::CallExpr(@$ = {@1, @4}, $1, $3); }
    | TOKEN_minus cexpr %prec UMINUS { $$ = new parser::UnaryExpr(@$ = {@1, @2}, string("-"), $2); }
    | cexpr TOKEN_plus cexpr { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string("+"), $3); }
    | cexpr TOKEN_minus cexpr { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string("-"), $3); }
    | cexpr TOKEN_star cexpr { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string("*"), $3); }
    | cexpr TOKEN_slash cexpr { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string("//"), $3); }
    | cexpr TOKEN_percent cexpr { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string("%"), $3); }
    | cexpr TOKEN_equalequal cexpr { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string("=="), $3); }
    | cexpr TOKEN_exclaimequal cexpr { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string("!="), $3); }
    | cexpr TOKEN_lessequal cexpr { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string("<="), $3); }
    | cexpr TOKEN_greaterequal cexpr { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string(">="), $3); }
    | cexpr TOKEN_greater cexpr { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string(">"), $3); }
    | cexpr TOKEN_less cexpr { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string("<"), $3); }
    | cexpr TOKEN_IS cexpr { $$ = new parser::BinaryExpr(@$ = {@1, @3}, $1, string("is"), $3); }

member_expr : cexpr TOKEN_period identifier {
    $$ = new parser::MemberExpr(@$ = {@1, @3}, $1, $3);
}
index_expr : cexpr TOKEN_l_square expr TOKEN_r_square {
    $$ = new parser::IndexExpr(@$ = {@1, @4}, $1, $3);
}
target :
    identifier { $$ = $1; @$ = @1; }
    | member_expr { $$ = $1; @$ = @1; }
    | index_expr { $$ = $1; @$ = @1; }
assign_stmt :
    target TOKEN_equal expr TOKEN_NEWLINE {
        $$ = new parser::AssignStmt(@$ = {@1, @3}, $1, $3);
    }
    | target TOKEN_equal assign_stmt {
        $$ = $3;
        $$->location = @$ = {@1, @3};
        $3->targets.emplace_back($1);
    }

identifier : TOKEN_IDENTIFIER {
    $$ = new parser::Ident(@$ = yylloc, string($1));
    free($1);
}
%%

/** The error reporting function. */
void yyerror(const char *s) {
    /** TO STUDENTS: This is just an example.
     * You can customize it as you like. */
    string info("Parser error near token");
    auto test = std::make_unique<::parser::CompilerErr>(Location(), info, true);
    ROOT->errors->compiler_errors.emplace_back(std::move(test));
}

std::unique_ptr<::parser::Program> parse(const char* input_path) {
    if (input_path != NULL) {
        if (!(yyin = fopen(input_path, "r"))) {
            std::cerr << fmt::format("[ERR] Open input file {} failed.", input_path) << std::endl;
            exit(EXIT_FAILURE);
        }
    } else {
        yyin = stdin;
    }
    /** Uncomment to see the middle process of bison*/
    /* yydebug = 1; */
    yyrestart(yyin);
    yyparse();
    return std::move(ROOT);
}
