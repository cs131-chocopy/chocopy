#include "chocopy_parse.hpp"

#include <memory>

#include "FunctionDefType.hpp"
#include "ValueType.hpp"

namespace parser {

LocationUnit::LocationUnit(int line, int colume) : line(line), column(colume) {}
Location::Location() {}
Location::Location(LocationUnit first, LocationUnit last)
    : first(first), last(last) {}
Location::Location(Location front, Location back)
    : first(front.first), last(back.last) {}

StringLiteral::StringLiteral(Location location, const string &value)
    : Literal(location, "StringLiteral", value) {
    if (value == "\"\"") {  // deal with null string
        this->value = "";
        ((Literal *)this)->value = "";
    } else
        this->value = value;
}

json Node::toJSON() const {
    json d;

    d["kind"] = kind;
#ifdef __PARSER_PRINT_LOCATION
    d["location"] = {location.first.line, location.first.column,
                     location.last.line, location.last.column};
#endif

    if (this->has_type_err()) d["errorMsg"] = typeError;
    return d;
}

json CompilerErr::toJSON() const {
    json d = Node::toJSON();
    d["message"] = message;
    if (this->syntax) d["syntax"] = true;
    return d;
}

json Errors::toJSON() const {
    json d = Node::toJSON();
    d["errors"] = json::array();
    for (auto &error : this->compiler_errors)
        d["errors"].emplace_back(error->toJSON());
    return d;
}

json TypedVar::toJSON() const {
    json d = Node::toJSON();
    d["identifier"] = identifier->toJSON();
    d["type"] = type->toJSON();
    return d;
}

json Expr::toJSON() const {
    json d = Node::toJSON();
    if (inferredType) d["inferredType"] = inferredType->toJSON();
    return d;
}

json Ident::toJSON() const {
    auto d = Expr::toJSON();
    d["name"] = name;
    return d;
}

json AssignStmt::toJSON() const {
    json d = Stmt::toJSON();
    d["targets"] = json::array();
    for (auto &target : this->targets)
        d["targets"].emplace_back(target->toJSON());
    d["value"] = value->toJSON();
    return d;
}

json BinaryExpr::toJSON() const {
    json d = Expr::toJSON();

    if (this->left != nullptr) d["left"] = left->toJSON();
    if (this->right != nullptr) d["right"] = right->toJSON();
    d["operator"] = operator_;
    return d;
}

json CallExpr::toJSON() const {
    json d = Expr::toJSON();
    d["function"] = function->toJSON();
    d["args"] = json::array();
    for (auto &arg : args) {
        d["args"].emplace_back(arg->toJSON());
    }
    return d;
}

json ClassDef::toJSON() const {
    json d = Decl::toJSON();
    d["name"] = name->toJSON();
    d["superClass"] = superClass->toJSON();
    d["declarations"] = json::array();
    for (auto &i : this->declaration) {
        d["declarations"].emplace_back(i->toJSON());
    }

    return d;
}

json ClassType::toJSON() const {
    json d = TypeAnnotation::toJSON();
    d["className"] = className;
    return d;
}

json ExprStmt::toJSON() const {
    json d = Stmt::toJSON();
    if (!this->expr->kind.empty()) {
        d["expr"] = expr->toJSON();
    }
    return d;
}

json ForStmt::toJSON() const {
    json d = Stmt::toJSON();
    d["identifier"] = identifier->toJSON();
    d["iterable"] = iterable->toJSON();
    d["body"] = json::array();
    for (auto &i : this->body) d["body"].emplace_back(i->toJSON());
    return d;
}

json FuncDef::toJSON() const {
    json d = Decl::toJSON();
    d["name"] = name->toJSON();
    d["params"] = json::array();
    for (auto &param : this->params) {
        d["params"].emplace_back(param->toJSON());
    }

    if (this->returnType != nullptr)
        d["returnType"] = returnType->toJSON();
    else {
        d["returnType"] = {
            {"kind", "ClassType"},
#ifdef __PARSER_PRINT_LOCATION
            {"location", {0, 0, 0, 0}},
#endif
            {"className", "<None>"},
        };
    }
    d["declarations"] = json::array();
    for (auto &declaration : this->declarations) {
        d["declarations"].emplace_back(declaration->toJSON());
    }
    d["statements"] = json::array();
    for (auto &statement : this->statements) {
        if (statement->kind != "PassStmt")
            d["statements"].emplace_back(statement->toJSON());
    }

    return d;
}

json GlobalDecl::toJSON() const {
    json d = Decl::toJSON();
    d["variable"] = variable->toJSON();
    return d;
}

json IfStmt::toJSON() const {
    json d = Stmt::toJSON();

    d["condition"] = condition->toJSON();

    d["thenBody"] = json::array();
    for (auto &statement : this->thenBody)
        if (statement->kind != "PassStmt")
            d["thenBody"].emplace_back(statement->toJSON());

    d["elseBody"] = json::array();
    if (this->el == cond::THEN_ELSE) {
        for (auto &statement : this->elseBody)
            if (statement->kind != "PassStmt")
                d["elseBody"].emplace_back(statement->toJSON());
    } else if (this->el == cond::THEN_ELIF) {
        d["elseBody"].emplace_back(elifBody->toJSON());
    }

    return d;
}

json IfExpr::toJSON() const {
    json d = Expr::toJSON();
    d["condition"] = condition->toJSON();
    d["thenExpr"] = thenExpr->toJSON();
    d["elseExpr"] = elseExpr->toJSON();
    return d;
}

json IndexExpr::toJSON() const {
    json d = Expr::toJSON();
    d["list"] = list->toJSON();
    d["index"] = index->toJSON();
    return d;
}

json ListExpr::toJSON() const {
    json d = Expr::toJSON();
    d["elements"] = json::array();
    for (auto &element : this->elements) {
        d["elements"].emplace_back(element->toJSON());
    }
    return d;
}

json ListType::toJSON() const {
    json d = TypeAnnotation::toJSON();
    d["elementType"] = elementType->toJSON();
    return d;
}

json MemberExpr::toJSON() const {
    json d = Expr::toJSON();
    d["object"] = object->toJSON();
    d["member"] = member->toJSON();
    return d;
}

json MethodCallExpr::toJSON() const {
    json d = Expr::toJSON();
    d["method"] = method->toJSON();
    d["args"] = json::array();
    for (auto &arg : this->args) d["args"].emplace_back(arg->toJSON());
    return d;
}

json NonlocalDecl::toJSON() const {
    json d = Decl::toJSON();
    d["variable"] = variable->toJSON();
    return d;
}

json Program::toJSON() const {
    json d = Node::toJSON();
    d["declarations"] = json::array();
    for (auto &declaration : this->declarations) {
        d["declarations"].emplace_back(declaration->toJSON());
    }
    d["statements"] = json::array();
    for (auto &statement : this->statements) {
        d["statements"].emplace_back(statement->toJSON());
    }
    d["errors"] = errors->toJSON();
    return d;
}

json ReturnStmt::toJSON() const {
    json d = Stmt::toJSON();

    if (this->value != nullptr) {
        d["value"] = value->toJSON();
    }

    return d;
}

json UnaryExpr::toJSON() const {
    json d = Expr::toJSON();
    d["operator"] = operator_;
    d["operand"] = operand->toJSON();
    return d;
}

json VarDef::toJSON() const {
    json d = Decl::toJSON();
    d["var"] = var->toJSON();
    d["value"] = value->toJSON();
    return d;
}

json WhileStmt::toJSON() const {
    json d = Stmt::toJSON();
    d["condition"] = condition->toJSON();
    d["body"] = json::array();
    for (auto &&i : this->body) {
        if (i->kind != "PassStmt") d["body"].emplace_back(i->toJSON());
    }
    return d;
}

json StringLiteral::toJSON() const {
    json d = Expr::toJSON();
    d["value"] = value;
    return d;
}

json IntegerLiteral::toJSON() const {
    json d = Expr::toJSON();
    d["value"] = int_value;
    return d;
}

json BoolLiteral::toJSON() const {
    json d = Expr::toJSON();
    d["value"] = bin_value;
    return d;
}
json NoneLiteral::toJSON() const { return Expr::toJSON(); }

string TypeAnnotation::get_name() {
    auto value_type = semantic::ValueType::annotate_to_val(this);
    if (auto class_type =
            dynamic_cast<semantic::ClassValueType *>(value_type.get())) {
        return class_type->class_name;
    } else if (auto list_type =
                   dynamic_cast<semantic::ListValueType *>(value_type.get())) {
        return list_type->get_name();
    }
    abort();
}
}  // namespace parser

#ifdef PA1
int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    auto tree = parse(argv[1]);

    auto j = tree->toJSON();
    std::cout << j.dump(2) << std::endl;
}
#endif
