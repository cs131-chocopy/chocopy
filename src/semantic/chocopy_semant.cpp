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
    // TODO: Implement this
}
void SymbolTableGenerator::visit(parser::FuncDef &func_def) {
    // TODO: Implement this
}
void SymbolTableGenerator::visit(parser::VarDef &var_def) {
    return_value = ValueType::annotate_to_val(var_def.var->type.get());
}
void SymbolTableGenerator::visit(parser::NonlocalDecl &nonlocal_decl) {
    // TODO: Implement this
}
void SymbolTableGenerator::visit(parser::GlobalDecl &global_decl) {
    // TODO: Implement this
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
    // TODO: Implement this, this is not complete
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
        typeError(&node,
                  fmt::format("Cannot apply opreator + on types {} and {}",
                              left_type->get_name(), right_type->get_name()));
    }
}
void TypeChecker::visit(parser::BoolLiteral &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::CallExpr &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::ExprStmt &node) { node.expr->accept(*this); }
void TypeChecker::visit(parser::ForStmt &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::ClassDef &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::FuncDef &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::Ident &node) {
    // TODO: Implement this, this is not complete
    auto type = sym->declares_shared<SymbolType>(node.name);
    node.inferredType = type;
}
void TypeChecker::visit(parser::IfStmt &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::IfExpr &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::IndexExpr &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::IntegerLiteral &node) {
    node.inferredType = int_value_type;
}
void TypeChecker::visit(parser::ListExpr &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::MemberExpr &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::MethodCallExpr &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::NoneLiteral &node) {
    node.inferredType = none_value_type;
}
void TypeChecker::visit(parser::ReturnStmt &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::StringLiteral &node) {
    node.inferredType = str_value_type;
}
void TypeChecker::visit(parser::UnaryExpr &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::VarDef &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::WhileStmt &node) {
    // TODO: Implement this
}
void TypeChecker::visit(parser::AssignStmt &node) {
    // TODO: Implement this
}
}  // namespace semantic

#ifdef PA2
int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    std::unique_ptr<parser::Program> tree(parse(argv[1]));
    if (tree->errors->compiler_errors.size() == 0) {
        auto symboltableGenerator = semantic::SymbolTableGenerator(*tree);
        tree->accept(symboltableGenerator);
    }
    if (tree->errors->compiler_errors.size() == 0) {
        auto typeChecker = semantic::TypeChecker(*tree);
        tree->accept(typeChecker);
    }

    auto j = tree->toJSON();
    std::cout << j.dump(2) << std::endl;
}
#endif
