#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

using nlohmann::json;
using std::string;
using std::string_view;

namespace semantic {
class SymbolType {
   public:
    virtual ~SymbolType() = default;

    virtual constexpr bool is_value_type() const { return false; }
    virtual constexpr bool is_list_type() const { return false; }
    virtual constexpr bool is_func_type() const { return false; }

    virtual const string get_name() const = 0;
    virtual json toJSON() const = 0;
};
}  // namespace semantic
