#pragma once

#include <cassert>
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
#include "chocopy_logging.hpp"
#include "hierarchy_tree.hpp"

using std::shared_ptr;
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
    virtual void visit(parser::BinaryExpr &node __attribute__((unused))){};
    virtual void visit(parser::Node &node __attribute__((unused))){};
    virtual void visit(parser::Errors &node __attribute__((unused))){};
    virtual void visit(parser::PassStmt &node __attribute__((unused))){};
    virtual void visit(parser::BoolLiteral &node __attribute__((unused))){};
    virtual void visit(parser::CallExpr &node __attribute__((unused))){};
    virtual void visit(parser::ClassDef &node __attribute__((unused))){};
    virtual void visit(parser::ClassType &node __attribute__((unused))){};
    virtual void visit(parser::ExprStmt &node __attribute__((unused))){};
    virtual void visit(parser::ForStmt &node __attribute__((unused))){};
    virtual void visit(parser::FuncDef &node __attribute__((unused))){};
    virtual void visit(parser::GlobalDecl &node __attribute__((unused))){};
    virtual void visit(parser::Ident &node __attribute__((unused))){};
    virtual void visit(parser::IfExpr &node __attribute__((unused))){};
    virtual void visit(parser::IfStmt &node __attribute__((unused))){};
    virtual void visit(parser::IndexExpr &node __attribute__((unused))){};
    virtual void visit(parser::IntegerLiteral &node __attribute__((unused))){};
    virtual void visit(parser::ListExpr &node __attribute__((unused))){};
    virtual void visit(parser::ListType &node __attribute__((unused))){};
    virtual void visit(parser::MemberExpr &node __attribute__((unused))){};
    virtual void visit(parser::MethodCallExpr &node __attribute__((unused))){};
    virtual void visit(parser::NoneLiteral &node __attribute__((unused))){};
    virtual void visit(parser::NonlocalDecl &node __attribute__((unused))){};
    virtual void visit(parser::ReturnStmt &node __attribute__((unused))){};
    virtual void visit(parser::StringLiteral &node __attribute__((unused))){};
    virtual void visit(parser::TypedVar &node __attribute__((unused))){};
    virtual void visit(parser::UnaryExpr &node __attribute__((unused))){};
    virtual void visit(parser::VarDef &node __attribute__((unused))){};
    virtual void visit(parser::WhileStmt &node __attribute__((unused))){};
    virtual void visit(parser::TypeAnnotation &){};
    virtual void visit(parser::AssignStmt &node __attribute__((unused))){};
    virtual void visit(parser::Program &node __attribute__((unused))){};
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

    TypeChecker(parser::Program &program)
        : sym(&program.symbol_table),
          global(&program.symbol_table),
          hierachy_tree(&program.hierachy_tree),
          errors(&program.errors->compiler_errors) {}

    /** Inserts an error message in NODE if there isn"t one already.
     *  The message is constructed with MESSAGE and ARGS as for
     *  String.format. */
    void typeError(parser::Node *node, const string &message);

    /** The current symbol table (changes depending on the function
     *  being analyzed). */
    SymbolTable *sym;
    stack<SymbolTable *> saved;
    SymbolTable *const global;
    HierachyTree *const hierachy_tree;

    /** For the nested function declaration */
    FunctionDefType *cur_func{};
    std::vector<std::string> *cur_lambda_params;
    stack<FunctionDefType *> saved_func{};

    bool is_lvalue{false};

    /** Collector for errors. */
    vector<std::unique_ptr<parser::CompilerErr>> *errors;

    /** Should work on ClassType, ListType, <None>, ...
     * hierachy_tree->common_ancestor() only works on Class */
    shared_ptr<SymbolType> get_common_type(shared_ptr<SymbolType>,
                                           shared_ptr<SymbolType>);
    bool is_subtype(SymbolType const *, SymbolType const *);

    std::shared_ptr<ClassValueType>
        object_value_type = std::make_shared<ClassValueType>("object"),
        int_value_type = std::make_shared<ClassValueType>("int"),
        bool_value_type = std::make_shared<ClassValueType>("bool"),
        none_value_type = std::make_shared<ClassValueType>("<None>"),
        str_value_type = std::make_shared<ClassValueType>("str");
};

class SymbolTableGenerator : public ast::ASTAnalyzer {
   public:
    SymbolTableGenerator(parser::Program &program)
        : global_sym(&program.symbol_table),
          sym(&program.symbol_table),
          hierachy_tree(&program.hierachy_tree),
          errors(&program.errors->compiler_errors) {
        auto object_value_type = std::make_shared<ClassValueType>("object");
        auto none_value_type = std::make_shared<ClassValueType>("<None>");

        auto object_class = std::make_shared<ClassDefType>("", "object");
        auto object_init = std::make_shared<FunctionDefType>();
        object_init->func_name = "__init__";
        object_init->return_type = none_value_type;
        object_init->params.emplace_back(object_value_type);
        object_class->current_scope.put("__init__", object_init);
        sym->tab.insert({"object", object_class});

        auto str_class = std::make_shared<ClassDefType>("object", "str");
        auto str_init = std::make_shared<FunctionDefType>();
        str_init->func_name = "__init__";
        str_init->return_type = none_value_type;
        str_init->params.emplace_back(std::make_shared<ClassValueType>("str"));
        str_class->current_scope.put("__init__", str_init);
        sym->tab.insert({"str", str_class});

        auto int_class = std::make_shared<ClassDefType>("object", "int");
        auto int_init = std::make_shared<FunctionDefType>();
        int_init->func_name = "__init__";
        int_init->return_type = none_value_type;
        int_init->params.emplace_back(std::make_shared<ClassValueType>("int"));
        int_class->current_scope.put("__init__", int_init);
        sym->tab.insert({"int", int_class});

        auto bool_class = std::make_shared<ClassDefType>("object", "bool");
        auto bool_init = std::make_shared<FunctionDefType>();
        bool_init->func_name = "__init__";
        bool_init->return_type = none_value_type;
        bool_init->params.emplace_back(
            std::make_shared<ClassValueType>("bool"));
        bool_class->current_scope.put("__init__", bool_init);
        sym->tab.insert({"bool", bool_class});

        auto len_func = std::make_shared<FunctionDefType>();
        len_func->func_name = "len";
        len_func->return_type = std::make_shared<ClassValueType>("int");
        len_func->params.emplace_back(object_value_type);
        sym->tab.insert({"len", len_func});

        auto print_func = std::make_shared<FunctionDefType>();
        print_func->func_name = "print";
        print_func->return_type = none_value_type;
        print_func->params.emplace_back(object_value_type);
        sym->tab.insert({"print", print_func});

        auto input_func = std::make_shared<FunctionDefType>();
        input_func->func_name = "input";
        input_func->return_type = std::make_shared<ClassValueType>("str");
        sym->tab.insert({"input", input_func});
    }
    void visit(parser::Program &) override;
    void visit(parser::ClassDef &) override;
    void visit(parser::VarDef &) override;
    void visit(parser::FuncDef &) override;
    void visit(parser::NonlocalDecl &) override;
    void visit(parser::GlobalDecl &) override;

   private:
    shared_ptr<SymbolType> return_value = nullptr;
    SymbolTable *const global_sym;  // Global symbol table
    SymbolTable *sym;               // Current symbol table
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
    ClassDefType *current_class;
    SymbolTable *const globals;
    SymbolTable *sym;
    ClassDefType *getClass(const string &name) {
        return globals->declares<ClassDefType>(name);
    }
    void checkValueType(
        ValueType *type,
        const std::unique_ptr<parser::TypeAnnotation> &annoation) {
        while (type->is_list_type()) {
            type = static_cast<ListValueType *>(type)->element_type.get();
        }
        assert(dynamic_cast<ClassValueType *>(type));
        const auto &class_name =
            static_cast<ClassValueType *>(type)->class_name;

        if (getClass(class_name) == nullptr) {
            errors->emplace_back(new SemanticError(
                annoation->location,
                "Invalid type annotation; there is no class named: " +
                    class_name));
        }
    }
};

}  // namespace semantic
