//
// Created by yiwei yang on 2021/7/26.
//

#ifndef CHOCOPY_COMPILER_FUNCTIONDEFTYPE_HPP
#define CHOCOPY_COMPILER_FUNCTIONDEFTYPE_HPP

#include <nlohmann/json.hpp>

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
    ~FunctionDefType();

    string func_name;
    ValueType *return_type{};
    std::vector<SymbolType *> *params = new std::vector<SymbolType *>();
    SymbolTable *current_scope = new SymbolTable();

    bool operator==(const FunctionDefType &f2) const;
    bool is_func_type() const final { return true; }
    const string get_name() const final { return func_name; }
    json toJSON() const override;
};

}  // namespace semantic
#endif  // CHOCOPY_COMPILER_FUNCTIONDEFTYPE_HPP
