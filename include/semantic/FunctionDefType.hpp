//
// Created by yiwei yang on 2021/7/26.
//

#ifndef CHOCOPY_COMPILER_FUNCTIONDEFTYPE_HPP
#define CHOCOPY_COMPILER_FUNCTIONDEFTYPE_HPP

#include <memory>
#include <nlohmann/json.hpp>

#include "SymbolTable.hpp"
#include "SymbolType.hpp"
#include "ValueType.hpp"
using nlohmann::json;
using std::shared_ptr;

namespace semantic {
class ValueType;
class ClassValueType;
class FunctionDefType : public SymbolType {
   public:
    string func_name;
    shared_ptr<ValueType> return_type;
    std::vector<shared_ptr<SymbolType>> params;
    SymbolTable current_scope;

    bool operator==(const FunctionDefType &f2) const;
    bool is_func_type() const final { return true; }
    const string get_name() const final { return func_name; }
    json toJSON() const override;
};

}  // namespace semantic
#endif  // CHOCOPY_COMPILER_FUNCTIONDEFTYPE_HPP
