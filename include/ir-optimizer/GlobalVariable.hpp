#pragma once

#include "Constant.hpp"
#include "Module.hpp"
#include "User.hpp"

using std::string;

namespace lightir {
/** Code-generation related information about a global variable. */
class GlobalVariable : public User {
   public:
    bool is_const_ = true;
    bool is_print_head_ = false;
    bool is_init = false;

    Constant *init_val_;
    static GlobalVariable *create(const string &name, Module *m, Type *ty,
                                  bool is_const, Constant *init);
    static GlobalVariable *create(const string &name, Module *m,
                                  ConstantStr *init);
    static GlobalVariable *create(const string &name, Module *m,
                                  ConstantBoxInt *init);
    static GlobalVariable *create(const string &name, Module *m,
                                  ConstantBoxBool *init);
    GlobalVariable(const string &name, Module *m, Type *ty, bool is_const,
                   Constant *init);

    Constant *get_init() const { return init_val_; }
    string print() override;
};
}  // namespace lightir
