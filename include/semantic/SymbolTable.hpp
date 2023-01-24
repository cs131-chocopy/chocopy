//
// Created by yiwei yang on 2021/7/26.
//

#ifndef CHOCOPY_COMPILER_SYMBOLTABLE_HPP
#define CHOCOPY_COMPILER_SYMBOLTABLE_HPP

#include <map>
#include <string>
#include <vector>

#include "SymbolType.hpp"

using std::map;
using std::string;

namespace semantic {
/** A block-structured symbol table a mapping identifiers to information
 *  about them of type T in a given declarative region. */
class SymbolTable {
   public:
    /** A table representing a region nested in that represented by
     *  PARENT0. */
    explicit SymbolTable(SymbolTable *parent) : parent(parent) {}
    SymbolTable() : parent(nullptr) {}
    ~SymbolTable() {
        // ! life is short, just let it leak
        // for (const auto &symbol : *tab) {
        //     delete symbol.second;
        // }
    }
    map<string, SymbolType *> tab;

    bool is_nonlocal(const string &name) const {
        return this->parent != nullptr &&
               this->parent->is_nonlocal_helper(name);
    }
    bool is_nonlocal_helper(const string &name) const {
        return this->parent != nullptr &&
               (tab.contains(name) || this->parent->is_nonlocal_helper(name));
    }

    /** Returns the mapping in this scope or in the parent scope using a
     * recursive traversal */
    template <typename T>
    T get(const string &name) {
        if (tab.count(name) > 0)
            return dynamic_cast<T>(tab.at(name));
        else if (parent != nullptr)
            return parent->get<T>(name);
        else
            return nullptr;
    }

    template <typename T = SymbolType *>
    T declares(const string &name) const {
        if (tab.count(name) > 0) return dynamic_cast<T>(tab.at(name));
        return nullptr;
    }

    /** Adds a new mapping in the current scope, possibly shadowing mappings in
     * the parent scope. */
    SymbolTable *put(const string &name, SymbolType *value) {
        tab[name] = value;
        return this;
    }

    SymbolTable *parent;
};
}  // namespace semantic
#endif  // CHOCOPY_COMPILER_SYMBOLTABLE_HPP
