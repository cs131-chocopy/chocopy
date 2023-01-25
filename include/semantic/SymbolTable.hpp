#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "SymbolType.hpp"

using std::map;
using std::shared_ptr;
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
    map<string, shared_ptr<SymbolType>> tab;

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
    T *get(const string &name) {
        if (tab.count(name) > 0)
            return dynamic_cast<T *>(tab.at(name).get());
        else if (parent != nullptr)
            return parent->get<T>(name);
        else
            return nullptr;
    }

    template <typename T>
    shared_ptr<T> get_shared(const string &name) {
        if (tab.count(name) > 0)
            return std::dynamic_pointer_cast<T>(tab.at(name));
        else if (parent != nullptr)
            return parent->get_shared<T>(name);
        else
            return nullptr;
    }

    template <typename T = SymbolType>
    T *declares(const string &name) const {
        if (tab.count(name) > 0) return dynamic_cast<T *>(tab.at(name).get());
        return nullptr;
    }

    template <typename T = SymbolType>
    shared_ptr<T> declares_shared(const string &name) const {
        if (tab.count(name) > 0)
            return std::dynamic_pointer_cast<T>(tab.at(name));
        return nullptr;
    }

    /** Adds a new mapping in the current scope, possibly shadowing mappings in
     * the parent scope. */
    void put(const string &name, shared_ptr<SymbolType> value) {
        tab[name] = value;
    }

    SymbolTable *parent;
};
}  // namespace semantic
