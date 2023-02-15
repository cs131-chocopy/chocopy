%locations
%define api.location.type {::parser::Location}

%{
#define YYLLOC_DEFAULT(Cur, Rhs, N)
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <string>
#include <iostream>

#include "chocopy_parse.hpp"
#include "chocopy_ast.hpp"

using ::parser::Location;

/* external functions from lex */
extern void yyrestart(FILE*);
extern int yylex();
extern int yyparse();
extern FILE* yyin;

/* the program, the root of AST
* all information is stored in ROOT
* The AST (a tree) structure is held by (smart) pointers. 
*/
std::unique_ptr<::parser::Program> ROOT = std::make_unique<::parser::Program>(Location());

/* error reporting */
void yyerror(const char *s);

/* append item to the end of LIST. Then returns LIST. */
/* this is a handy helper function. */
template<typename T, typename X>
T* combine(T* list, X *item) {
    list->emplace_back(item);
    return list;
}
%}

/* all possible types of semantic value */
/* check https://www.gnu.org/software/bison/manual/html_node/Union-Decl.html */
%union {
  char *raw_str;
  int raw_int;
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


/* declare tokens and their type */
/* check https://www.gnu.org/software/bison/manual/html_node/Token-Decl.html */

/*
* You can delete, add, and modify the below at your convenience.
*/

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

%type <PtrStmt> stmt simple_stmt
%type <PtrIfStmt> elif_list
%type <PtrListDecl> top_level_decl class_body func_decls
%type <PtrListStmt> stmt_list block
%type <PtrListTypedVar> typed_var_list
%type <PtrClassDef> class_def
%type <PtrGlobalDecl> global_decl
%type <PtrTypedVar> typed_var
%type <PtrTypeAnnotation> type func_return_type
%type <PtrExpr> expr cexpr target
%type <PtrExprList> expr_list
%type <PtrIndexExpr> index_expr
%type <PtrMemberExpr> member_expr
%type <PtrLiteral> literal
%type <PtrIdent> identifier
%type <PtrVarDef> var_def
%type <PtrFuncDef> func_def
%type <PtrNonlocalDecl> nonlocal_decl
%type <PtrAssignStmt> assign_stmt

/* the destructor is called when error happends */
/* you don't need to modify the code */
/* check https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html */
%destructor { } <raw_int>
%destructor { free($$); } <raw_str>
%destructor { delete $$; } <*>

/* you may define associativity and precedence here */
/* check https://www.gnu.org/software/bison/manual/html_node/Precedence-Decl.html */
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
/* write grammar rule here! */

/* The start symbol of your grammar */
/* The rule is not complete(nor sound). You can rewrite and add some rules. */
/* 喜报：我们不再对 JSON 中 "Locaton" 项计分，因此你可以选择不计算每个 symbol 的 Location。程序也不会输出关于 Location 的信息。
 * 你可以添加 __PARSER_PRINT_LOCATION 的宏定义以恢复 Location 的输出。
 * 你只需用 Location() 新建一个空 Location 传入构造函数。 
 */
program : top_level_decl stmt_list {
        ROOT = std::move(std::make_unique<::parser::Program>(Location(), $1, $2));
    }

/* Entry point to declerations */
top_level_decl : { $$ = new std::vector<std::unique_ptr<::parser::Decl>>(); }

stmt_list : stmt {
        $$ = new std::vector<std::unique_ptr<::parser::Stmt>>();
        $$->emplace_back($1);
    }

stmt : simple_stmt TOKEN_NEWLINE { $$ = $1; }

simple_stmt : expr { $$ = new parser::ExprStmt(Location(), $1); } 

expr: cexpr { $$ = $1; }

literal : TOKEN_INTEGER { $$ = new parser::IntegerLiteral(Location(), $1); }

cexpr :
    | literal { $$ = $1; }
    | cexpr TOKEN_plus cexpr { $$ = new parser::BinaryExpr(Location(), $1, string("+"), $3); }
%%

/** The error reporting function. */
void yyerror(const char *s) {
    /* This is just an example.
     * You can customize it as you like. */
    string info("Parsing error");
    auto error = std::make_unique<::parser::CompilerErr>(Location(), info, true);
    ROOT->errors->compiler_errors.emplace_back(std::move(error));
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
    /* uncomment to see the middle process of Bison */
    /* yydebug = 1; */
    yyrestart(yyin);
    yyparse();
    return std::move(ROOT);
}
