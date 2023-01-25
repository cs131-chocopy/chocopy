//
// Created by yiwei yang on 2021/7/26.
//

#ifndef CHOCOPY_COMPILER_CLASSDEFTYPE_HPP
#define CHOCOPY_COMPILER_CLASSDEFTYPE_HPP

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
    bool is_special_class() const override {
        return class_name != "str" && class_name != "int" &&
               class_name != "bool";
    };
    virtual json toJSON() const override { abort(); }
    std::vector<string> inherit_members;
    string super_class;
    string class_name;
    SymbolTable current_scope;
};
}  // namespace semantic
#endif  // CHOCOPY_COMPILER_CLASSDEFTYPE_HPP
