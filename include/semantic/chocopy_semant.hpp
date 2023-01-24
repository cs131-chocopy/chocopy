//
// Created by yiwei yang on 2021/7/26.
//

#ifndef CHOCOPY_COMPILER_CHOCOPY_SEMANT_HPP
#define CHOCOPY_COMPILER_CHOCOPY_SEMANT_HPP

#include <cassert>
#include <chocopy_logging.hpp>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <string>

#include "ClassDefType.hpp"
#include "FunctionDefType.hpp"
#include "SymbolTable.hpp"
#include "SymbolType.hpp"
#include "ValueType.hpp"
#include "hierarchy_tree.hpp"

using std::stack;
using std::string;

namespace parser {
class AssignStmt;
class BinaryExpr;
class BoolLiteral;
class CallExpr;
class ClassDef;
class ClassType;
class Decl;
class CompilerErr;
class Expr;
class ExprStmt;
class ForStmt;
class FuncDef;
class GlobalDecl;
class Ident;
class IfExpr;
class IndexExpr;
class IntegerLiteral;
class ListExpr;
class ListType;
class Literal;
class MemberExpr;
class MethodCallExpr;
class Node;
class NoneLiteral;
class NonlocalDecl;
class Program;
class ReturnStmt;
class Stmt;
class StringLiteral;
class TypeAnnotation;
class TypedVar;
class UnaryExpr;
class VarDef;
class PassStmt;
class IfStmt;
class Errors;
class WhileStmt;

}  // namespace parser

using std::string;

namespace ast {
class Visitor;
class ASTAnalyzer : public Visitor {
   public:
    virtual void visit(parser::BinaryExpr &node){};
    virtual void visit(parser::Node &node){};
    virtual void visit(parser::Errors &node){};
    virtual void visit(parser::PassStmt &node){};
    virtual void visit(parser::BoolLiteral &node){};
    virtual void visit(parser::CallExpr &node){};
    virtual void visit(parser::ClassDef &node){};
    virtual void visit(parser::ClassType &node){};
    virtual void visit(parser::ExprStmt &node){};
    virtual void visit(parser::ForStmt &node){};
    virtual void visit(parser::FuncDef &node){};
    virtual void visit(parser::GlobalDecl &node){};
    virtual void visit(parser::Ident &node){};
    virtual void visit(parser::IfExpr &node){};
    virtual void visit(parser::IfStmt &node){};
    virtual void visit(parser::IndexExpr &node){};
    virtual void visit(parser::IntegerLiteral &node){};
    virtual void visit(parser::ListExpr &node){};
    virtual void visit(parser::ListType &node){};
    virtual void visit(parser::MemberExpr &node){};
    virtual void visit(parser::MethodCallExpr &node){};
    virtual void visit(parser::NoneLiteral &node){};
    virtual void visit(parser::NonlocalDecl &node){};
    virtual void visit(parser::ReturnStmt &node){};
    virtual void visit(parser::StringLiteral &node){};
    virtual void visit(parser::TypedVar &node){};
    virtual void visit(parser::UnaryExpr &node){};
    virtual void visit(parser::VarDef &node){};
    virtual void visit(parser::WhileStmt &node){};
    virtual void visit(parser::TypeAnnotation &){};
    virtual void visit(parser::AssignStmt &node){};
    virtual void visit(parser::Program &node){};
};

}  // namespace ast

namespace semantic {

class SemanticError : public parser::CompilerErr {
   public:
    SemanticError(parser::Location location, const string &message)
        : CompilerErr(location, message, false) {}

    string message;
};

/** Analyzer that performs ChocoPy type checks on all nodes.  Applied after
 *  collecting declarations. */
class TypeChecker : public ast::ASTAnalyzer {
   public:
    void visit(parser::BinaryExpr &node) override;
    void visit(parser::BoolLiteral &node) override;
    void visit(parser::CallExpr &node) override;
    void visit(parser::ClassDef &node) override;
    void visit(parser::ExprStmt &node) override;
    void visit(parser::ForStmt &node) override;
    void visit(parser::FuncDef &node) override;
    void visit(parser::Ident &node) override;
    void visit(parser::IfStmt &node) override;
    void visit(parser::IfExpr &node) override;
    void visit(parser::IndexExpr &node) override;
    void visit(parser::IntegerLiteral &node) override;
    void visit(parser::ListExpr &node) override;
    void visit(parser::MemberExpr &node) override;
    void visit(parser::MethodCallExpr &node) override;
    void visit(parser::NoneLiteral &node) override;
    void visit(parser::ReturnStmt &node) override;
    void visit(parser::StringLiteral &node) override;
    void visit(parser::UnaryExpr &node) override;
    void visit(parser::VarDef &node) override;
    void visit(parser::WhileStmt &node) override;
    void visit(parser::Program &node) override;
    void visit(parser::AssignStmt &node) override;

    /** type checker attributes and their chocopy typing judgement analogues:
     * O : symbolTable
     * M : classes
     * C : currentClass
     * R : expReturnType */
    TypeChecker(parser::Program &program)
        : errors(&program.errors->compiler_errors),
          global(&program.symbol_table),
          sym(&program.symbol_table),
          hierachy_tree(&program.hierachy_tree) {}
    /** Inserts an error message in NODE if there isn"t one already.
     *  The message is constructed with MESSAGE and ARGS as for
     *  String.format. */
    void typeError(parser::Node *node, const string &message);

    /** The current symbol table (changes depending on the function
     *  being analyzed). */
    SymbolTable *sym;
    stack<SymbolTable *> saved{};
    SymbolTable *const global;
    HierachyTree *const hierachy_tree;

    /** For the nested function declaration */
    FunctionDefType *curr_func = nullptr;
    std::vector<std::string> *curr_lambda_params;
    stack<FunctionDefType *> saved_func{};

    bool is_lvalue{false};

    /** Collector for errors. */
    vector<std::unique_ptr<parser::CompilerErr>> *errors;

    // The function can check both ClassType and ListType
    SymbolType *get_common_type(SymbolType *const, SymbolType *const);
    bool is_subtype(SymbolType const *, SymbolType const *);
    bool is_subtype(const string &, SymbolType const *);
};

class SymbolTableGenerator : public ast::ASTAnalyzer {
   public:
    SymbolTableGenerator(parser::Program &program)
        : errors(&program.errors->compiler_errors),
          globals(&program.symbol_table),
          sym(&program.symbol_table),
          hierachy_tree(&program.hierachy_tree) {
        auto *foo = new ClassDefType("", "object");
        auto *init = new FunctionDefType();
        init->func_name = "__init__";
        init->return_type = new ClassValueType("<None>");
        init->params.emplace_back(new ClassValueType("object"));
        foo->current_scope->tab.insert({"__init__", init});
        sym->tab.insert({"object", foo});

        foo = new ClassDefType("object", "str");
        init = new FunctionDefType();
        init->func_name = "__init__";
        init->return_type = new ClassValueType("<None>");
        init->params.emplace_back(new ClassValueType("str"));
        foo->current_scope->tab.insert({"__init__", init});
        sym->tab.insert({"str", foo});

        foo = new ClassDefType("object", "int");
        init = new FunctionDefType();
        init->func_name = "__init__";
        init->return_type = new ClassValueType("<None>");
        init->params.emplace_back(new ClassValueType("int"));
        foo->current_scope->tab.insert({"__init__", init});
        sym->tab.insert({"int", foo});

        foo = new ClassDefType("object", "bool");
        init = new FunctionDefType();
        init->func_name = "__init__";
        init->return_type = new ClassValueType("<None>");
        init->params.emplace_back(new ClassValueType("bool"));
        foo->current_scope->tab.insert({"__init__", init});
        sym->tab.insert({"bool", foo});

        auto bar = new FunctionDefType();
        bar->func_name = "len";
        bar->return_type = new ClassValueType("int");
        bar->params.emplace_back(new ClassValueType("object"));
        sym->tab.insert({"len", bar});

        bar = new FunctionDefType();
        bar->func_name = "print";
        bar->return_type = new ClassValueType("<None>");
        bar->params.emplace_back(new ClassValueType("object"));
        sym->tab.insert({"print", bar});

        bar = new FunctionDefType();
        bar->func_name = "input";
        bar->return_type = new ClassValueType("str");
        sym->tab.insert({"input", bar});
    }
    void visit(parser::Program &) override;
    void visit(parser::ClassDef &) override;
    void visit(parser::VarDef &) override;
    void visit(parser::FuncDef &) override;
    void visit(parser::NonlocalDecl &) override;
    void visit(parser::GlobalDecl &) override;

   private:
    SymbolType *ret = nullptr;
    SymbolTable *const globals;
    SymbolTable *sym;
    HierachyTree *const hierachy_tree;
    vector<std::unique_ptr<parser::CompilerErr>> *const errors;
};

/**
 * Analyzes declarations to create a top-level symbol table
 */
class DeclarationAnalyzer : public ast::ASTAnalyzer {
   public:
    void visit(parser::ClassDef &node) override;
    void visit(parser::FuncDef &node) override;
    void visit(parser::GlobalDecl &node) override;
    void visit(parser::NonlocalDecl &node) override;
    void visit(parser::VarDef &varDef) override;
    void visit(parser::Program &program) override;

    explicit DeclarationAnalyzer(parser::Program &program)
        : errors(&program.errors->compiler_errors),
          globals(&program.symbol_table),
          sym(&program.symbol_table) {}

    /** Collector for errors. */
    vector<std::unique_ptr<parser::CompilerErr>> *errors;

   private:
    ClassDefType *current_class = nullptr;
    SymbolTable *const globals;
    SymbolTable *sym;
    ClassDefType *getClass(const string &name) {
        return globals->declares<ClassDefType *>(name);
    }
    void checkValueType(
        ValueType *type,
        const std::unique_ptr<parser::TypeAnnotation> &annoation) {
        while (dynamic_cast<ListValueType *>(type)) {
            type = ((ListValueType *)type)->element_type;
        }
        assert(dynamic_cast<ClassValueType *>(type));
        const auto &class_name = ((ClassValueType *)type)->class_name;

        if (getClass(class_name) == nullptr) {
            errors->emplace_back(new SemanticError(
                annoation->location,
                "Invalid type annotation; there is no class named: " +
                    class_name));
        }
    }
};

}  // namespace semantic
#endif  // CHOCOPY_COMPILER_CHOCOPY_SEMANT_HPP
