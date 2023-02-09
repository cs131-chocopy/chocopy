#include "chocopy_semant.hpp"

#include <fmt/core.h>
#include <fmt/format.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <utility>

#include "FunctionDefType.hpp"
#include "SymbolType.hpp"
#include "ValueType.hpp"
#include "chocopy_parse.hpp"

using std::set;
namespace semantic {
void SymbolTableGenerator::visit(parser::Program &program) {
    for (const auto &decl : program.declarations) {
        auto id = decl->get_id();
        const auto &name = id->name;

        if (sym->declares(name)) {
            errors->emplace_back(new SemanticError(
                id->location,
                "Duplicate declaration of identifier in same scope: " + name));
            continue;
        }

        return_value = nullptr;
        decl->accept(*this);

        if (return_value == nullptr) continue;
        sym->put(name, return_value);
    }
}
void SymbolTableGenerator::visit(parser::ClassDef &class_def) {
    const auto &super_name = class_def.superClass->name;
    const auto super_class_type = global_sym->get<SymbolType>(super_name);
    if (super_class_type == nullptr) {
        errors->emplace_back(
            new SemanticError(class_def.superClass->location,
                              "Super-class not defined: " + super_name));
        return;
    }
    const auto super_class_def = dynamic_cast<ClassDefType *>(super_class_type);
    if (super_class_def == nullptr) {
        errors->emplace_back(
            new SemanticError(class_def.superClass->location,
                              "Super-class must be a class: " + super_name));
        return;
    }
    if (super_name == "int" || super_name == "str" || super_name == "bool") {
        errors->emplace_back(
            new SemanticError(class_def.superClass->location,
                              "Cannot extend special class: " + super_name));
        return;
    }
    const auto &super_scope = super_class_def->current_scope;
    const auto &class_name = class_def.name->name;
    const auto class_ = std::make_shared<ClassDefType>(super_name, class_name);
    hierachy_tree->add_class(class_name, super_name);
    assert(this->sym == global_sym);
    class_->current_scope.parent = global_sym;

    this->sym = &class_->current_scope;

    for (const auto &decl : class_def.declaration) {
        const auto id = decl->get_id();
        const auto &name = id->name;

        if (sym->declares(name)) {
            errors->emplace_back(new SemanticError(
                id->location,
                "Duplicate declaration of identifier in same scope: " + name));
            continue;
        }

        decl->accept(*this);

        if (return_value->is_value_type()) {
            if (super_scope.declares(name)) {
                errors->emplace_back(new SemanticError(
                    id->location, "Cannot re-define attribute: " + name));
            }
            sym->put(name, return_value);
        } else {
            const auto func =
                std::dynamic_pointer_cast<FunctionDefType>(return_value);
            assert(func);
            const auto params = func->params;
            if (params.size() < 1 ||
                std::dynamic_pointer_cast<ClassValueType>(params.front()) ==
                    nullptr ||
                params.front()->get_name() != class_name) {
                errors->emplace_back(
                    new SemanticError(id->location,
                                      "First parameter of the following method "
                                      "must be of the enclosing class: " +
                                          name));
            }
            func->is_method = true;
            if (name != "__init__" && super_scope.declares(name)) {
                auto super_func =
                    super_class_def->current_scope.get<FunctionDefType>(name);
                if (super_func) {
                    if (*super_func == *func) {
                        sym->put(name, func);
                    } else {
                        errors->emplace_back(
                            new SemanticError(id->location,
                                              "Method overridden with "
                                              "different type signature: " +
                                                  name));
                    }
                } else {
                    errors->emplace_back(new SemanticError(
                        id->location, "Cannot re-define attribute: " + name));
                }
            } else {
                sym->put(name, func);
            }
        }
    }

    for (const auto [name, type] : super_scope.tab) {
        if (sym->declares(name)) continue;
        sym->put(name, type);
        class_->inherit_members.emplace_back(name);
    }

    return_value = class_;
    this->sym = this->sym->parent;
}
void SymbolTableGenerator::visit(parser::FuncDef &func_def) {
    const auto id = func_def.get_id();
    const auto &name = id->name;

    const auto func = std::make_shared<FunctionDefType>();
    func->current_scope.parent = this->sym;
    this->sym = &func->current_scope;

    func->func_name = name;
    func->return_type = ValueType::annotate_to_val(func_def.returnType.get());

    for (const auto &param : func_def.params) {
        const auto id = param->identifier.get();
        const auto &name = id->name;

        const auto type = ValueType::annotate_to_val(param->type.get());
        func->params.emplace_back(type);

        if (sym->declares(name)) {
            errors->emplace_back(new SemanticError(
                id->location,
                "Duplicate declaration of identifier in same scope: " + name));
            continue;
        }

        sym->put(name, type);
    }

    for (const auto &decl : func_def.declarations) {
        const auto id = decl->get_id();
        const auto &name = id->name;

        if (sym->declares(name)) {
            errors->emplace_back(new SemanticError(
                id->location,
                "Duplicate declaration of identifier in same scope: " + name));
            continue;
        }

        return_value = nullptr;
        decl->accept(*this);
        if (return_value) sym->put(name, return_value);
    }

    return_value = func;
    this->sym = this->sym->parent;
}
void SymbolTableGenerator::visit(parser::VarDef &var_def) {
    return_value = ValueType::annotate_to_val(var_def.var->type.get());
}
void SymbolTableGenerator::visit(parser::NonlocalDecl &nonlocal_decl) {
    return_value =
        std::make_shared<NonlocalRefType>(nonlocal_decl.get_id()->name);
}
void SymbolTableGenerator::visit(parser::GlobalDecl &global_decl) {
    return_value = std::make_shared<GlobalRefType>(global_decl.get_id()->name);
}

void DeclarationAnalyzer::visit(parser::Program &program) {
    for (const auto &decl : program.declarations) {
        decl->accept(*this);
    }

    for (const auto &stmt : program.statements) {
        if (dynamic_cast<parser::ReturnStmt *>(stmt.get())) {
            errors->emplace_back(new SemanticError(
                stmt->location,
                "Return statement cannot appear at the top level"));
        }
    }
}
void DeclarationAnalyzer::visit(parser::VarDef &var_def) {
    const auto &id = var_def.var->identifier;
    const auto &name = id->name;

    assert(sym->get<ValueType>(name));

    if (getClass(name)) {
        errors->emplace_back(new SemanticError(
            id->location, "Cannot shadow class name: " + name));
    }
    checkValueType(sym->get<ValueType>(name), var_def.var->type);
}
void DeclarationAnalyzer::visit(parser::ClassDef &class_def) {
    const auto &class_name = class_def.get_id()->name;
    const auto class_ = getClass(class_name);
    assert(class_);
    current_class = class_;
    sym = &class_->current_scope;

    for (const auto &decl : class_def.declaration) {
        if (const auto var_def = dynamic_cast<parser::VarDef *>(decl.get())) {
            const auto &id = var_def->var->identifier;
            const auto &name = id->name;
            assert(sym->get<ValueType>(name));
            checkValueType(sym->get<ValueType>(name), var_def->var->type);
        } else {
            decl->accept(*this);
        }
    }

    sym = sym->parent;
    current_class = nullptr;
}
void DeclarationAnalyzer::visit(parser::FuncDef &func_def) {
    const auto id = func_def.get_id();
    const auto &name = id->name;

    const auto func_type = sym->declares<FunctionDefType>(name);
    assert(func_type);
    sym = &func_type->current_scope;

    for (int i = 0; i < func_type->params.size(); i++) {
        const auto &annoation = func_def.params.at(i)->type;
        const auto value_type =
            dynamic_cast<ValueType *>(func_type->params.at(i).get());
        assert(value_type);
        checkValueType(value_type, annoation);
    }

    for (const auto &decl : func_def.declarations) {
        decl->accept(*this);
    }

    if (name == "__init__" && current_class) {
        for (const auto &stmt : func_def.statements) {
            if (dynamic_cast<parser::ReturnStmt *>(stmt.get())) {
                errors->emplace_back(new SemanticError(
                    stmt->location,
                    "Return statements should not appear in method: __init__"));
            }
        }
    }

    this->sym = this->sym->parent;
}
void DeclarationAnalyzer::visit(parser::GlobalDecl &global_decl) {
    const auto id = global_decl.get_id();
    const auto &name = id->name;
    const auto type = sym->get<GlobalRefType>(name);
    assert(type);

    const auto real_type = globals->declares_shared<ValueType>(name);
    if (!real_type)
        errors->emplace_back(
            new SemanticError(id->location, "Not a global variable: " + name));

    sym->put(name, real_type);
}
void DeclarationAnalyzer::visit(parser::NonlocalDecl &nonlocal_decl) {
    const auto id = nonlocal_decl.get_id();
    const auto &name = id->name;
    const auto type = sym->get<NonlocalRefType>(name);
    assert(type);

    shared_ptr<ValueType> real_type;
    auto table = sym->parent;
    while (real_type == nullptr && table != globals) {
        real_type = table->declares_shared<ValueType>(name);
        table = table->parent;
    }
    if (!real_type)
        errors->emplace_back(new SemanticError(
            id->location, "Not a nonlocal variable: " + name));

    sym->put(name, real_type);
}

void TypeChecker::typeError(parser::Node *node, const string &message) {
    errors->emplace_back(
        std::make_unique<SemanticError>(node->location, message));
    if (!node->has_type_err()) {
        node->typeError = message;
    } else {
        node->typeError += "\t" + message;
    }
}
void TypeChecker::visit(parser::Program &program) {
    for (const auto &decl : program.declarations) {
        decl->accept(*this);
    }
    for (const auto &stmt : program.statements) {
        stmt->accept(*this);
    }
}
void TypeChecker::visit(parser::BinaryExpr &node) {
    node.left->accept(*this);
    node.right->accept(*this);
    auto left_type = node.left->inferredType;
    auto right_type = node.right->inferredType;
    if (left_type == nullptr || right_type == nullptr) return;
    if (node.operator_ == "+") {
        if (left_type->get_name() == "int" && right_type->get_name() == "int") {
            node.inferredType = int_value_type;
            return;
        }
        if (left_type->get_name() == "str" && right_type->get_name() == "str") {
            node.inferredType = str_value_type;
            return;
        }
        auto left_list_type = dynamic_pointer_cast<ListValueType>(left_type);
        auto right_list_type = dynamic_pointer_cast<ListValueType>(right_type);
        if (left_list_type != nullptr && right_list_type != nullptr) {
            auto elem_type = get_common_type(left_list_type->element_type,
                                             right_list_type->element_type);
            node.inferredType = std::make_shared<ListValueType>(
                static_pointer_cast<ValueType>(elem_type));
            return;
        }
        typeError(&node,
                  fmt::format("Cannot apply opreator + on types {} and {}",
                              left_type->get_name(), right_type->get_name()));
        if (left_type->get_name() == "int" || right_type->get_name() == "int") {
            node.inferredType = int_value_type;
        } else {
            node.inferredType = object_value_type;
        }
    } else if (node.operator_ == "-" || node.operator_ == "*" ||
               node.operator_ == "//" || node.operator_ == "%") {
        if (left_type->get_name() == "int" && right_type->get_name() == "int") {
            node.inferredType = int_value_type;
        } else {
            typeError(&node,
                      fmt::format("Cannot apply opreator {} on types {} and {}",
                                  node.operator_, left_type->get_name(),
                                  right_type->get_name()));
        }
    } else if (node.operator_ == "==" || node.operator_ == "!=") {
        if ((left_type->get_name() == "int" &&
             right_type->get_name() == "int") ||
            (left_type->get_name() == "str" &&
             right_type->get_name() == "str") ||
            (left_type->get_name() == "bool" &&
             right_type->get_name() == "bool")) {
            node.inferredType = bool_value_type;
        } else {
            typeError(&node,
                      fmt::format("Cannot apply opreator {} on types {} and {}",
                                  node.operator_, left_type->get_name(),
                                  right_type->get_name()));
        }
    } else if (node.operator_ == "<=" || node.operator_ == ">=" ||
               node.operator_ == "<" || node.operator_ == ">") {
        if (left_type->get_name() == "int" && right_type->get_name() == "int") {
            node.inferredType = bool_value_type;
        } else {
            typeError(&node,
                      fmt::format("Cannot apply opreator {} on types {} and {}",
                                  node.operator_, left_type->get_name(),
                                  right_type->get_name()));
        }
    } else if (node.operator_ == "and" || node.operator_ == "or") {
        if (left_type->get_name() == "bool" &&
            right_type->get_name() == "bool") {
            node.inferredType = bool_value_type;
        } else {
            typeError(&node,
                      fmt::format("Cannot apply opreator {} on types {} and {}",
                                  node.operator_, left_type->get_name(),
                                  right_type->get_name()));
        }
    } else if (node.operator_ == "is") {
        auto lcn = left_type->get_name(), rcn = right_type->get_name();
        if (lcn == "int" || lcn == "bool" || lcn == "str" || rcn == "int" ||
            rcn == "bool" || rcn == "str") {
            typeError(&node,
                      fmt::format("Cannot apply opreator {} on types {} and {}",
                                  node.operator_, left_type->get_name(),
                                  right_type->get_name()));
        }
        node.inferredType = bool_value_type;
    } else {
        assert(false);
    }
}
void TypeChecker::visit(parser::BoolLiteral &node) {
    node.inferredType = bool_value_type;
}
void TypeChecker::visit(parser::CallExpr &node) {
    auto func = sym->get_shared<FunctionDefType>(node.function->name);
    if (func && func->is_method) {
        func = global->get_shared<FunctionDefType>(node.function->name);
    }
    const auto class_ = global->get<ClassDefType>(node.function->name);
    if (func == nullptr && class_ == nullptr) {
        typeError(&node, "Not a function or class: " + node.function->name);
        return;
    }
    if (func) {
        if (!global->declares<FunctionDefType>(node.function->name) &&
            cur_lambda_params) {
            cur_lambda_params->emplace_back(node.function->name);
        }
        node.function->inferredType = func;
    }

    decltype(func->params)::const_iterator param;
    if (func != nullptr) {
        if (node.args.size() != func->params.size()) {
            typeError(&node,
                      fmt::format("Expected {} arguments; got {}",
                                  func->params.size(), node.args.size()));
            return;
        }
        param = func->params.cbegin();
    } else {
        const auto init_func =
            class_->current_scope.get<FunctionDefType>("__init__");
        if (node.args.size() != init_func->params.size() - 1) {
            typeError(&node, fmt::format("Expected {} arguments; got {}",
                                         init_func->params.size() - 1,
                                         node.args.size()));
            return;
        }
        param = init_func->params.cbegin();
        param++;
    }

    for (int i = 0; i < node.args.size(); i++) {
        const auto &arg = node.args.at(i);
        arg->accept(*this);
        const auto arg_type = arg->inferredType;
        const auto param_type = *(param++);
        if (!is_subtype(arg_type.get(), param_type.get())) {
            typeError(
                &node,
                fmt::format("Expected type `{}`; got type `{}` in parameter {}",
                            param_type->get_name(), arg_type->get_name(), i));
        }
    }

    if (func != nullptr) {
        node.inferredType = func->return_type;
    } else {
        node.inferredType =
            std::make_shared<ClassValueType>(class_->get_name());
    }
}
void TypeChecker::visit(parser::ExprStmt &node) { node.expr->accept(*this); }
void TypeChecker::visit(parser::ForStmt &node) {
    node.iterable->accept(*this);
    for (const auto &s : node.body) {
        s->accept(*this);
    }
    is_lvalue = true;
    node.identifier->accept(*this);
    is_lvalue = false;
    auto ident_type = sym->get<ValueType>(node.identifier->name);
    if (ident_type == nullptr) return;

    if (node.iterable->inferredType->get_name() == "str") {
    } else if (auto iter_list_type = dynamic_pointer_cast<ListValueType>(
                   node.iterable->inferredType)) {
        auto list_elem_type = iter_list_type->element_type;
        if (!is_subtype(list_elem_type.get(), ident_type)) {
            typeError(
                &node,
                fmt::format(
                    "The type of id {}:{} and iterator type {} do not match.",
                    node.identifier->name, ident_type->get_name(),
                    node.iterable->inferredType->get_name()));
        }
    } else {
        typeError(&node, fmt::format("not iterable type {} in for_stmt",
                                     node.iterable->inferredType->get_name()));
    }
}
void TypeChecker::visit(parser::ClassDef &node) {
    saved.push(sym);
    sym = &sym->get<ClassDefType>(node.name->name)->current_scope;
    for (const auto &s : node.declaration) {
        s->accept(*this);
    }
    sym = saved.top();
    saved.pop();
}
void TypeChecker::visit(parser::FuncDef &node) {
    saved.push(sym);
    saved_func.push(cur_func);
    cur_func = sym->get<FunctionDefType>(node.name->name);
    assert(cur_func);
    sym = &cur_func->current_scope;

    std::set<std::string> declared_local_vars;
    for (const auto &s : node.declarations) {
        s->accept(*this);
        if (dynamic_cast<parser::VarDef *>(s.get()) ||
            dynamic_cast<parser::FuncDef *>(s.get())) {
            declared_local_vars.insert(s->get_id()->name);
        }
        if (dynamic_cast<parser::NonlocalDecl *>(s.get())) {
            node.lambda_params.emplace_back(s->get_id()->name);
        }
        if (dynamic_cast<parser::FuncDef *>(s.get())) {
            for (auto &x : ((parser::FuncDef *)s.get())->lambda_params)
                node.lambda_params.emplace_back(x);
        }
    }

    bool have_return =
        is_subtype(none_value_type.get(), cur_func->return_type.get());
    cur_lambda_params = &node.lambda_params;
    for (const auto &s : node.statements) {
        s->accept(*this);
        if (s->is_return) {
            have_return = true;
        }
    }

    std::vector<std::string> new_lambda_params;
    std::copy_if(cur_lambda_params->begin(), cur_lambda_params->end(),
                 std::back_inserter(new_lambda_params),
                 [&declared_local_vars](const std::string &x) {
                     return declared_local_vars.find(x) ==
                            declared_local_vars.end();
                 });
    cur_lambda_params = nullptr;
    node.lambda_params = std::move(new_lambda_params);

    if (!have_return && node.returnType != nullptr) {
        typeError(&node, fmt::format("All paths in this function/method must "
                                     "have a return statement: {}",
                                     node.name->name));
    }

    sym = saved.top();
    saved.pop();
    cur_func = saved_func.top();
    saved_func.pop();
}
void TypeChecker::visit(parser::Ident &node) {
    auto type = sym->declares_shared<SymbolType>(node.name);
    if (type == nullptr) {
        type = sym->get_shared<SymbolType>(node.name);
        if (type) {
            if (is_lvalue) {
                typeError(&node,
                          fmt::format("Cannot assign to variable that is not "
                                      "explicitly declared in this scope: {}",
                                      node.name));
            } else {
                if (cur_lambda_params &&
                    global->get<SymbolType>(node.name) != type.get()) {
                    cur_lambda_params->emplace_back(node.name);
                }
                node.inferredType = type;
            }
        } else {
            typeError(&node, fmt::format("Not a variable: {}", node.name));
        }
    } else {
        if (type->is_value_type()) {
            node.inferredType = type;
        } else {
            typeError(&node, fmt::format("Not a variable: {}", node.name));
            node.inferredType = object_value_type;
        }
    }
}
void TypeChecker::visit(parser::IfStmt &node) {
    bool is_return_t = false, is_return_f = false;
    node.condition->accept(*this);
    for (const auto &s : node.thenBody) {
        s->accept(*this);
        if (s->is_return) {
            is_return_t = true;
        }
    }
    if (node.el == parser::IfStmt::cond::THEN_ELSE) {
        for (const auto &s : node.elseBody) {
            s->accept(*this);
            if (s->is_return) {
                is_return_f = true;
            }
        }
    }
    if (node.el == parser::IfStmt::cond::THEN_ELIF) {
        assert(node.elifBody);
        node.elifBody->accept(*this);
        if (node.elifBody->is_return) {
            is_return_f = true;
        }
    }
    if (is_return_t && is_return_f) {
        node.is_return = true;
    }
    if (node.condition->inferredType->get_name() != "bool") {
        typeError(&node, fmt::format("Condition expression must be bool"));
    }
}
void TypeChecker::visit(parser::IfExpr &node) {
    node.condition->accept(*this);
    if (node.condition->inferredType->get_name() != "bool") {
        typeError(&node, fmt::format("Condition expression must be bool"));
    }
    node.thenExpr->accept(*this);
    node.elseExpr->accept(*this);
    node.inferredType = get_common_type(node.thenExpr->inferredType,
                                        node.elseExpr->inferredType);
}
void TypeChecker::visit(parser::IndexExpr &node) {
    node.list->accept(*this);
    node.index->accept(*this);
    if (auto class_type =
            dynamic_pointer_cast<ClassValueType>(node.list->inferredType)) {
        if (class_type->get_name() == "str") {
            node.inferredType = str_value_type;
        } else {
            typeError(&node, fmt::format("Indexing on a non-string type {}",
                                         class_type->get_name()));
        }
    } else if (auto list_type = dynamic_pointer_cast<ListValueType>(
                   node.list->inferredType)) {
        node.inferredType = list_type->element_type;
    } else {
        throw("A Value type but neither ClassValueType or ListValueType?");
    }
    if (node.index->inferredType->get_name() != "int") {
        typeError(&node, fmt::format("Index is a non-int type {}",
                                     node.index->inferredType->get_name()));
    }
}
void TypeChecker::visit(parser::IntegerLiteral &node) {
    node.inferredType = int_value_type;
}
void TypeChecker::visit(parser::ListExpr &node) {
    if (node.elements.empty()) {
        node.inferredType = std::make_shared<ClassValueType>("<Empty>");
        return;
    }
    shared_ptr<ValueType> v;
    for (auto &e : node.elements) {
        e->accept(*this);
    }
    for (auto &e : node.elements) {
        if (auto elem_class_type =
                dynamic_pointer_cast<ClassValueType>(e->inferredType)) {
            if (v == nullptr) {
                v = elem_class_type;
            } else {
                if (v->is_list_type()) {
                    v = object_value_type;
                } else {
                    auto lca = get_common_type(v, elem_class_type);
                    assert(lca->is_value_type());
                    v = std::static_pointer_cast<ValueType>(lca);
                }
            }
        } else {
            auto elem_list_type =
                dynamic_pointer_cast<ListValueType>(e->inferredType);
            if (v == nullptr) {
                v = elem_list_type;
            } else {
                if (elem_list_type->neq(v.get())) {
                    v = object_value_type;
                }
            }
        }
    }
    node.inferredType = std::make_shared<ListValueType>(v);
}
void TypeChecker::visit(parser::MemberExpr &node) {
    node.object->accept(*this);
    if (auto class_ =
            dynamic_pointer_cast<ClassValueType>(node.object->inferredType)) {
        if (auto class_def = global->get<ClassDefType>(class_->class_name)) {
            const auto member_type =
                class_def->current_scope.declares_shared<SymbolType>(
                    node.member->name);
            if (dynamic_cast<FunctionDefType *>(member_type.get()) ||
                dynamic_cast<ValueType *>(member_type.get())) {
                if (member_type->is_func_type() && !node.is_function_call) {
                    typeError(
                        &node,
                        fmt::format("{}.{} is a function, which is not "
                                    "fisrt class citizen in Chocopy",
                                    class_->class_name, node.member->name));
                } else if (!member_type->is_func_type() &&
                           node.is_function_call) {
                    typeError(
                        &node,
                        fmt::format("{}.{} is not a function, but be called",
                                    class_->class_name, node.member->name));
                }
                node.inferredType = member_type;
            } else {
                typeError(&node,
                          fmt::format(
                              "There is no attribute named `{}` in class `{}`",
                              node.member->name, class_->class_name));
            }
        } else {
            typeError(&node, fmt::format("basic class {} do not have member",
                                         class_->class_name));
        }
    } else {
        typeError(&node, fmt::format("MemberExpr object is not a class"));
    }
}
void TypeChecker::visit(parser::MethodCallExpr &node) {
    node.method->is_function_call = true;
    node.method->accept(*this);

    const auto func =
        dynamic_pointer_cast<FunctionDefType>(node.method->inferredType);
    if (func == nullptr) {
        node.inferredType = none_value_type;
        return;
    }

    if (node.args.size() != func->params.size() - 1) {
        typeError(&node,
                  fmt::format("Expected {} arguments; got {}",
                              func->params.size() - 1, node.args.size()));
        return;
    }

    for (int i = 0; i < node.args.size(); i++) {
        const auto &arg = node.args.at(i);
        arg->accept(*this);
        const auto arg_type = arg->inferredType;
        const auto param_type = func->params.at(i + 1);
        if (!is_subtype(arg_type.get(), param_type.get())) {
            typeError(
                &node,
                fmt::format("Expected type `{}`; got type `{}` in parameter {}",
                            param_type->get_name(), arg_type->get_name(), i));
        }
    }

    node.inferredType = func->return_type;
}
void TypeChecker::visit(parser::NoneLiteral &node) {
    node.inferredType = none_value_type;
}
void TypeChecker::visit(parser::ReturnStmt &node) {
    auto &func_return_type = cur_func->return_type;
    shared_ptr<SymbolType> T;
    if (node.value == nullptr) {
        T = none_value_type;
    } else {
        node.value->accept(*this);
        T = node.value->inferredType;
    }
    if (!is_subtype(T.get(), func_return_type.get())) {
        typeError(&node,
                  fmt::format("Expected type `{}`; got type `{}`",
                              func_return_type->get_name(), T->get_name()));
    }
}
void TypeChecker::visit(parser::StringLiteral &node) {
    node.inferredType = str_value_type;
}
void TypeChecker::visit(parser::UnaryExpr &node) {
    node.operand->accept(*this);
    if (parser::UnaryExpr::hashcode(node.operator_) ==
        parser::UnaryExpr::operator_code::Minus) {
        auto t =
            dynamic_pointer_cast<ClassValueType>(node.operand->inferredType);
        if (t == nullptr || t->class_name != "int") {
            typeError(&node,
                      fmt::format("Cannot apply opreator {} on type {}",
                                  node.operator_,
                                  node.operand->inferredType->get_name()));
            node.inferredType = object_value_type;
            return;
        }
        node.inferredType = int_value_type;
    } else {
        auto t =
            dynamic_pointer_cast<ClassValueType>(node.operand->inferredType);
        if (t == nullptr || t->class_name != "bool") {
            typeError(&node,
                      fmt::format("Cannot apply opreator {} on type {}",
                                  node.operator_,
                                  node.operand->inferredType->get_name()));
            node.inferredType = object_value_type;
            return;
        }
        node.inferredType = bool_value_type;
    }
}
void TypeChecker::visit(parser::VarDef &node) {
    node.value->accept(*this);
    auto listType = dynamic_cast<parser::ListType *>(node.var->type.get());
    if (listType != nullptr) {
        if (node.value->inferredType->get_name() != "<None>") {
            typeError(
                &node,
                fmt::format(
                    "Cannot use literal value type {} to declare list value {}",
                    node.value->inferredType->get_name(),
                    listType->get_name()));
        }
    } else {
        auto classType =
            dynamic_cast<parser::ClassType *>(node.var->type.get());
        assert(classType != nullptr);
        auto valueType = ClassValueType(classType);
        if (!(classType->className == "object" ||
              classType->className == node.value->inferredType->get_name() ||
              (valueType.is_special_class() &&
               node.value->inferredType->get_name() == "<None>"))) {
            typeError(&node, fmt::format("Expected type `{}`; got type `{}`",
                                         valueType.get_name(),
                                         node.value->inferredType->get_name()));
        }
    }
}
void TypeChecker::visit(parser::WhileStmt &node) {
    node.condition->accept(*this);
    for (const auto &s : node.body) {
        s->accept(*this);
    }
    if (node.condition->inferredType->get_name() != "bool") {
        typeError(&node, fmt::format("Type of condition of if must be bool"));
    }
}
void TypeChecker::visit(parser::AssignStmt &node) {
    node.value->accept(*this);
    auto value_type = node.value->inferredType;
    bool is_error = false;
    if (node.targets.size() > 1 && value_type->get_name() == "[<None>]") {
        typeError(&node,
                  "Right-hand side of multiple assignment may not be [<None>]");
        is_error = true;
    }
    for (const auto &s : node.targets) {
        is_lvalue = true;
        s->accept(*this);
        is_lvalue = false;
        auto target_type = s->inferredType;
        if (is_error || target_type == nullptr) continue;
        if (auto index = dynamic_cast<parser::IndexExpr *>(s.get());
            index != nullptr &&
            index->list->inferredType->get_name() == "str") {
            typeError(&node, "Cannot assign to string index");
        } else if (!is_subtype(value_type.get(), target_type.get())) {
            typeError(&node, fmt::format("Expected type `{}`; got type `{}`",
                                         target_type->get_name(),
                                         value_type->get_name()));
            is_error = true;
        }
    }
}
shared_ptr<SymbolType> TypeChecker::get_common_type(shared_ptr<SymbolType> x,
                                                    shared_ptr<SymbolType> y) {
    if (is_subtype(x.get(), y.get())) return y;
    if (is_subtype(y.get(), x.get())) return x;
    if (x->is_list_type() || y->is_list_type()) return object_value_type;
    return std::make_shared<ClassValueType>(
        hierachy_tree->common_ancestor(x->get_name(), y->get_name()));
}
bool TypeChecker::is_subtype(SymbolType const *sub, SymbolType const *super) {
    assert(sub != nullptr);
    assert(super != nullptr);
    // rule 3
    // <Empty> <= [T]
    if (super->is_list_type() && sub->get_name() == "<Empty>") return true;

    if (sub->is_list_type() && super->is_list_type()) {
        // [T] <= [T]
        if (sub->eq(super)) return true;
        // rule 4
        // [<None>] <= [T]
        const auto sub_e =
            dynamic_cast<ListValueType const *>(sub)->element_type;
        const auto super_e =
            dynamic_cast<ListValueType const *>(super)->element_type;
        if (sub_e->get_name() == "<None>") {
            sub = sub_e.get();
            super = super_e.get();
        } else {
            return false;
        }
    }

    // rule 2
    // <None> <= T
    if (sub->get_name() == "<None>") {
        return super->get_name() != "int" && super->get_name() != "bool" &&
               super->get_name() != "str";
    }

    // rule 1
    if (super->get_name() == "object") return true;
    if (sub->is_list_type() || super->is_list_type()) return false;
    return hierachy_tree->is_superclass(sub->get_name(), super->get_name());
}
shared_ptr<ValueType> ValueType::annotate_to_val(
    parser::TypeAnnotation *annotation) {
    if (dynamic_cast<parser::ClassType *>(annotation)) {
        return std::make_shared<ClassValueType>(
            (parser::ClassType *)annotation);
    } else {
        if (annotation != nullptr && annotation->kind == "<None>")
            return std::make_shared<ClassValueType>("<None>");
        if (dynamic_cast<parser::ListType *>(annotation))
            return std::make_shared<ListValueType>(
                (parser::ListType *)annotation);
    }
    return nullptr;
}
}  // namespace semantic

#ifdef PA2
int main(int argc, char *argv[]) {
    std::unique_ptr<parser::Program> tree(parse(argv[1]));
    if (tree->errors->compiler_errors.size() == 0) {
        auto symboltableGenerator = semantic::SymbolTableGenerator(*tree);
        tree->accept(symboltableGenerator);
    }
    if (tree->errors->compiler_errors.size() == 0) {
        auto declarationAnalyzer = semantic::DeclarationAnalyzer(*tree);
        tree->accept(declarationAnalyzer);
    }
    if (tree->errors->compiler_errors.size() == 0) {
        auto typeChecker = semantic::TypeChecker(*tree);
        tree->accept(typeChecker);
    }

    auto j = tree->toJSON();
    std::cout << j.dump(2) << std::endl;
}
#endif
