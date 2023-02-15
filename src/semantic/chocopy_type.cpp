#include <nlohmann/json.hpp>

#include "ClassDefType.hpp"
#include "FunctionDefType.hpp"
#include "SymbolType.hpp"
#include "ValueType.hpp"

namespace semantic {
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
ListValueType::ListValueType(parser::ListType *typeAnnotation)
    : element_type(
          ValueType::annotate_to_val(typeAnnotation->elementType.get())) {}

ClassValueType::ClassValueType(parser::ClassType *classTypeAnnotation)
    : class_name(classTypeAnnotation->className) {}

const string ValueType::get_name() const {
    return ((ClassValueType *)this)->class_name;
}

bool SymbolType::eq(const SymbolType *Value) const {
    if (this->is_list_type()) {
        if (dynamic_cast<ListValueType const *>(Value))
            return ((ListValueType const *)this)
                ->element_type->eq(
                    ((ListValueType const *)Value)->element_type.get());
        else
            return false;
    } else if (this->is_value_type()) {
        if (dynamic_cast<ClassValueType const *>(Value))
            return ((ClassValueType const *)this)->get_name() ==
                   ((ClassValueType const *)Value)->get_name();
        else
            return false;
    }
    return false;
}
bool SymbolType::neq(const SymbolType *Value) const { return !eq(Value); }

bool FunctionDefType::operator==(const FunctionDefType &f2) const {
    auto &a = this->params;
    auto &b = f2.params;
    if (a.size() == b.size()) {
        for (int i = 1; i < a.size(); i++) {
            if (a.at(i)->get_name() != b.at(i)->get_name()) return false;
        }
        if (this->return_type->get_name() != f2.return_type->get_name())
            return false;
        return true;
    } else
        return false;
}

json FunctionDefType::toJSON() const {
    json d;
    d["kind"] = "FuncType";
    d["parameters"] = json::array();
    for (auto &parameter : this->params) {
        d["parameters"].emplace_back(parameter->toJSON());
    }

    d["returnType"] = return_type->toJSON();
    return d;
}
json ClassValueType::toJSON() const {
    return {{"kind", "ClassValueType"}, {"className", get_name()}};
}
json ListValueType::toJSON() const {
    return {{"kind", "ListValueType"}, {"elementType", element_type->toJSON()}};
}
}  // namespace semantic
