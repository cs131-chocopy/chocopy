//
// Created by yiwei yang on 2021/7/26.
//

#ifndef CHOCOPY_COMPILER_FUNCTIONDEFTYPE_HPP
#define CHOCOPY_COMPILER_FUNCTIONDEFTYPE_HPP

#include <json.hpp>

#include "SymbolTable.hpp"
#include "SymbolType.hpp"
#include "ValueType.hpp"
using nlohmann::json;

namespace semantic {
class ValueType;
class ClassValueType;
class FunctionDefType : public SymbolType {
   public:
    FunctionDefType() = default;
    ~FunctionDefType() {
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

    string func_name;
    ValueType *return_type{};
    std::vector<SymbolType *> *params = new std::vector<SymbolType *>();
    SymbolTable *current_scope = new SymbolTable();

    bool operator==(const FunctionDefType &f2) const;
    bool is_func_type() const final { return true; }
    const string get_name() const final { return func_name; }
    string get_type() const override {
        auto func_type = string(typeid(this).name()).substr(14);
        func_type.erase(remove(func_type.begin(), func_type.end(), 'E'),
                        func_type.end());
        return func_type;
    }
    void set_name(string_view className) override;
    json toJSON() const;
};

}  // namespace semantic
#endif  // CHOCOPY_COMPILER_FUNCTIONDEFTYPE_HPP
