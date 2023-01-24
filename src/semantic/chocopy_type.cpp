#include <ClassDefType.hpp>
#include <FunctionDefType.hpp>
#include <ValueType.hpp>
#include <nlohmann/json.hpp>

namespace semantic {
ClassDefType::~ClassDefType() {
    for (const auto &e : inherit_members) {
        current_scope->tab->erase(e);
    }
    delete this->current_scope;
}

ListValueType::ListValueType(ValueType *element) : element_type(element) {}

FunctionDefType::~FunctionDefType() {
    // ! Will cause memory leak if params have duplicated names
    // if (params) {
    //     for (auto param : *params) {
    //         delete param;
    //     }
    //     delete params;
    // }
    delete return_type;
    delete current_scope;
}
bool FunctionDefType::operator==(const FunctionDefType &f2) const {
    std::vector<SymbolType *> *a = this->params;
    std::vector<SymbolType *> *b = f2.params;
    if (a->size() == b->size()) {
        for (int i = 1; i < a->size(); i++) {
            if (a->at(i)->get_name() != b->at(i)->get_name()) return false;
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
    for (auto &parameter : *this->params) {
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
