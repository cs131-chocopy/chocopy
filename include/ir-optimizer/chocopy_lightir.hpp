//
// Created by yiwei yang on 2021/7/26.
//

#ifndef CHOCOPY_COMPILER_CHOCOPY_LIGHTIR_HPP
#define CHOCOPY_COMPILER_CHOCOPY_LIGHTIR_HPP

#include <memory>
#include <regex>

#include "BasicBlock.hpp"
#include "Class.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "SymbolType.hpp"
#include "Type.hpp"
#include "chocopy_ast.hpp"
#include "chocopy_optimization.hpp"
#include "chocopy_semant.hpp"

const std::regex to_class_replace("\\$(.+?)+__init__\\.");

void print_help(const string_view &exe_name);
void print_help_all(const string_view &exe_name);

namespace semantic {
class SymbolTable;
}

namespace lightir {

class ScopeAnalyzer {
   public:
    /** enter a new scope */
    void enter() { inner.emplace_back(); }

    /** exit a scope */
    void exit() { inner.pop_back(); }

    int get_depth() { return inner.size(); }

    bool in_global() { return inner.size() == 1; /** depth = 1 */ }

    /** push a name to scope
     * return true if successful
     * return false if this name already exits */
    bool push(const string &name, Value *val) {
        // std::cerr << "push " << name << " in scope " <<
        // &inner[inner.size()-1] << std::endl;
        auto result = inner[inner.size() - 1].insert({name, val});
        return result.second;
    }

    Value *find(const string &name) {
        Value *ret_val = nullptr;
        static_cast<void>(
            std::find_if(inner.rbegin(), inner.rend(), [&](const auto &item) {
                // std::cerr << "try to find " << name << " in scope " << &item
                // << std::endl;
                auto value = item.find(name);
                if (value != item.end()) {
                    // std::cerr << "find " << name << " in scope " << &item <<
                    // std::endl;
                    ret_val = value->second;
                    return true;
                }
                return false;
            }));
        return ret_val;
    }
    Value *find(const string &name, Type *ty) {
        Value *ret_val = nullptr;
        for (auto s = inner.rbegin(); s != inner.rend(); s++) {
            auto iter = s->find(name);
            if (iter != s->end()) {
                if (!dynamic_cast<Function *>(iter->second) ||
                    ((dynamic_cast<Function *>(iter->second))->get_args())
                            .front()
                            ->get_type()
                            ->get_type_id() == ty->get_type_id()) {
                    return iter->second;
                }
            }
        }
        return ret_val;
    }
    Value *find_in_global(const string &name) {
        Value *ret_val = nullptr;
        auto s = inner[0];
        auto iter = s.find(name);
        if (iter != s.end()) {
            return iter->second;
        }
        return ret_val;
    }
    Value *find_in_nonlocal(const string &nam) {
        Value *ret_val = nullptr;
        for (auto s = inner.rbegin(); s != inner.rend(); s++) {
            auto iter = s->find(nam);
            if (iter != s->end()) {
                return iter->second;
            }
        }
        return ret_val;
    }
    /** Get the class anon's offset */
    Value *find_in_nonlocal(const string &nam, IRBuilder *builder) {
        /** Get the start of class.anon */
        Class *to_find_class = nullptr;
        Value *store = nullptr;
        for (auto &&s : inner[inner.size() - 2]) {
            if (s.first.starts_with("$class.anon")) {
                to_find_class = dynamic_cast<Class *>(s.second);
            }
        }
        if (to_find_class == nullptr) {
            for (auto &&s : inner[inner.size() - 1]) {
                if (s.first.starts_with("$class.anon")) {
                    to_find_class = dynamic_cast<Class *>(s.second);
                }
            }
        }
        if (to_find_class != nullptr) {
            /** Create GEP */
            store = builder->create_gep(
                to_find_class,
                ConstantInt::get(to_find_class->get_attr_offset(nam),
                                 builder->get_module()));
        }
        return store;
    }
    void remove_in_global(const string &name) {
        inner[0].erase(inner[0].find(name));
    }
    bool push_in_global(const string &name, Value *val) {
        auto result = inner[0].insert({name, val});
        return result.second;
    }

    void remove(const string &name) {
        if (inner[inner.size() - 1].find(name) !=
            inner[inner.size() - 1].end()) {
            inner[inner.size() - 1].erase(inner[inner.size() - 1].find(name));
        }
    }

    string cat_nest_func(const string &name) { return ""; }

   private:
    vector<map<std::string, Value *>> inner;
};

class LightWalker : public ast::Visitor {
   public:
    explicit LightWalker(parser::Program &program);
    std::shared_ptr<Module> get_module() { return std::move(module); };

    /** Predefined classes. The list "class" is a fake class; we use it only
     *  to emit a prototype object for empty lists. */
    Class *object_class, *int_class, *bool_class, *str_class, *list_class;
    Union *union_put, *union_len, *union_conslist;

    void visit(parser::AssignStmt &) override final;
    void visit(parser::Program &) override final;
    void visit(parser::PassStmt &) override final;
    void visit(parser::BinaryExpr &) override final;
    void visit(parser::BoolLiteral &) override final;
    void visit(parser::CallExpr &) override final;
    void visit(parser::ClassDef &) override final;
    void visit(parser::ClassType &) override final;
    void visit(parser::ExprStmt &) override final;
    void visit(parser::ForStmt &) override final;
    void visit(parser::FuncDef &) override final;
    void visit(parser::GlobalDecl &) override final;
    void visit(parser::Ident &) override final;
    void visit(parser::IfExpr &) override final;
    void visit(parser::IntegerLiteral &) override final;
    void visit(parser::ListExpr &) override final;
    void visit(parser::ListType &) override final;
    void visit(parser::MemberExpr &) override final;
    void visit(parser::IfStmt &node) override final;
    void visit(parser::MethodCallExpr &) override final;
    void visit(parser::NoneLiteral &) override final;
    void visit(parser::NonlocalDecl &) override final;
    void visit(parser::ReturnStmt &) override final;
    void visit(parser::StringLiteral &) override final;
    void visit(parser::TypeAnnotation &) override final;
    void visit(parser::TypedVar &) override final;
    void visit(parser::UnaryExpr &) override final;
    void visit(parser::VarDef &) override final;
    void visit(parser::WhileStmt &) override final;
    void visit(parser::Errors &) override final;
    void visit(parser::Node &) override final;
    void visit(parser::IndexExpr &) override final;

    IRBuilder *builder;
    semantic::SymbolTable *sym;
    ScopeAnalyzer scope;
    unique_ptr<Module> module;
    int next_type_id = 0;
    int next_const_id = 9;
    int get_next_type_id();
    int get_const_type_id();
    string get_nested_func_name(semantic::FunctionDefType const *const, bool);
    GlobalVariable *generate_init_object(parser::Literal *literal);
    Type *semantic_type_to_llvm_type(semantic::SymbolType *type);
    bool func_found = false;
    vector<parser::ClassDef *> *class_stack = new vector<parser::ClassDef *>();
    vector<parser::FuncDef *> *func_stack = new vector<parser::FuncDef *>();
    vector<parser::ClassDef *> *class_finished =
        new vector<parser::ClassDef *>();
    vector<tuple<string, parser::ReturnStmt *>> *return_stack =
        new vector<tuple<string, parser::ReturnStmt *>>();

    Value *visitor_return_value = nullptr;
    Value *visitor_return_object = nullptr;
    Type *visitor_return_type = nullptr;
    bool get_lvalue = false;  // mark if the visitor is in lvalue context

    Function *nonlist_fun, *strcat_fun, *concat_fun, *noconv_fun, *streql_fun,
        *strneql_fun, *makebool_fun, *makeint_fun, *makestr_fun, *alloc_fun,
        *conslist_fun, *heap_init_fun;
    Type *i32_type, *i1_type, *vstr_type, *ptr_vstr_type;
    Value *invalid_value;
    Value *null;
};

}  // namespace lightir
#endif  // CHOCOPY_COMPILER_CHOCOPY_LIGHTIR_HPP
