#include <ClassDefType.hpp>
#include <FunctionDefType.hpp>
#include <ValueType.hpp>
#include <nlohmann/json.hpp>

namespace semantic {
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
