#pragma once

#include <iostream>
#include <list>
#include <string>
#include <utility>

using std::list;
using std::string;

namespace lightir {
class Type;
class Value;

struct Use {
    Value *val_;
    unsigned arg_no_;
    Use(Value *val, unsigned no) : val_(val), arg_no_(no) {}
};

class Value {
   public:
    Value(Type *ty, string name);
    Value(Type *ty) : Value(ty, ""){};
    ~Value() = default;

    Type *get_type() const { return type_; }

    void set_type(Type *type) { type_ = type; }

    list<Use> &get_use_list() { return use_list_; }

    void add_use(Value *val, unsigned arg_no = 0);

    bool set_name(string name) {
        name_ = std::move(name);
        return true;
    }

    virtual string get_name();

    void replace_all_use_with(Value *new_val);
    void remove_use(Value *val);

    virtual string print() { return ""; };

    Type *type_;
    string name_; /* The name field to put on */
    bool should_load = false;

   private:
    list<Use> use_list_; /* The list contains people who call the value */
};
}  // namespace lightir
