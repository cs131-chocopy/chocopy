#include "GlobalVariable.hpp"

#include <utility>

#include "Constant.hpp"
#include "IRprinter.hpp"
namespace lightir {

GlobalVariable::GlobalVariable(const string &name, Module *m, Type *ty,
                               bool is_const, Constant *init)
    : User(ty, name, init != nullptr), init_val_(init), is_const_(is_const) {
    m->add_global_variable(this);
    if (init) {
        this->set_operand(0, init);
    }
}

GlobalVariable *GlobalVariable::create(const string &name, Module *m, Type *ty,
                                       bool is_const,
                                       Constant *init = nullptr) {
    return new GlobalVariable(name, m, ty, is_const, init);
}
GlobalVariable *GlobalVariable::create(const string &name, Module *m,
                                       ConstantStr *init) {
    return new GlobalVariable(name, m, init->get_type(), true, init);
}
GlobalVariable *GlobalVariable::create(const string &name, Module *m,
                                       ConstantBoxInt *init) {
    return new GlobalVariable(name, m, init->get_type(), true, init);
}
GlobalVariable *GlobalVariable::create(const string &name, Module *m,
                                       ConstantBoxBool *init) {
    return new GlobalVariable(name, m, init->get_type(), true, init);
}

string GlobalVariable::print() {
    string global_ir;
    if (init_val_ == nullptr) {
        global_ir += fmt::format("@{} = external global {}", this->name_,
                                 this->type_->print());
        return global_ir;
    }
    if (dynamic_cast<ConstantStr *>(this->init_val_) ||
        dynamic_cast<ConstantBoxInt *>(this->init_val_) ||
        dynamic_cast<ConstantBoxBool *>(this->init_val_)) {
        global_ir += this->init_val_->print();
        return global_ir;
    }
    if (!is_print_head_) {
        global_ir = fmt::format("{} = {}{}", print_as_op(this, false),
                                (this->is_const_ ? "constant " : "global "),
                                this->get_init()->print());
        is_print_head_ = true;
    } else {
        global_ir = fmt::format("ptr {}", print_as_op(this, false));
    }
    return global_ir;
}
}  // namespace lightir
