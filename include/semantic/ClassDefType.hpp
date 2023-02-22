#pragma once

#include <iostream>
#include <utility>

#include "SymbolTable.hpp"
#include "ValueType.hpp"

namespace semantic {
class ClassDefType : public SymbolType {
   public:
    ClassDefType(string parent, string self)
        : super_class(std::move(parent)), class_name(std::move(self)){};

    const string get_name() const override { return class_name; }
    virtual json toJSON() const override { abort(); }
    string super_class;
    string class_name;
    SymbolTable current_scope;
};
}  // namespace semantic
