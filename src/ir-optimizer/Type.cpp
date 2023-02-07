#include "Type.hpp"

#include <utility>

#include "Module.hpp"
#include "chocopy_logging.hpp"

namespace lightir {

Type::Type(type tid, Module *m) : tid_(tid) { m_ = m; }

Module *Type::get_module() { return m_; }

Type *Type::get_void_type(Module *m) { return m->get_void_type(); }

Type *Type::get_label_type(Module *m) { return m->get_label_type(); }

Type *Type::get_class_type(Module *m, int id_) {
    return m->get_class_type(id_);
}

IntegerType *Type::get_int1_type(Module *m) { return m->get_int1_type(); }

IntegerType *Type::get_int32_type(Module *m) { return m->get_int32_type(); }

Type *Type::get_ptr_element_type() {
    if (!dynamic_cast<PtrType *>(this))
        return this;
    else if (this->is_ptr_type())
        return dynamic_cast<PtrType *>(this)->get_element_type();
    else
        return this;
}

int Type::get_size() {
    if (this->is_integer_type()) {
        auto bits = dynamic_cast<IntegerType *>(this)->get_num_bits();
        return bits > 0 ? bits : 1;
    }
    if (this->is_ptr_type()) return 32;

    return 0;
}
std::strong_ordering Type::operator<=>(Type rhs) {
    if (this->is_ptr_type()) {
        return this->get_ptr_element_type() <=> rhs.get_ptr_element_type();
    } else {
        return this->get_type_id() <=> rhs.get_type_id();
    }
}
string Type::print() { return {}; }

IntegerType::IntegerType(unsigned num_bits, Module *m)
    : Type(num_bits == 1 ? Type::type::BOOL : Type::type::INT, m),
      num_bits_(num_bits) {}

IntegerType *IntegerType::get(unsigned num_bits, Module *m) {
    return new IntegerType(num_bits, m);
}

unsigned IntegerType::get_num_bits() const { return num_bits_; }

FunctionType::FunctionType(Type *result, const std::vector<Type *> &params)
    : Type(Type::type::FUNC, result->get_module()) {
    result_ = result;
    for (auto p : params) {
        args_.push_back(p);
    }
}

FunctionType *FunctionType::get(Type *result,
                                const std::vector<Type *> &params) {
    return new FunctionType(result, params);
}

FunctionType *FunctionType::get(Type *result, const std::vector<Type *> &params,
                                bool is_variable_args) {
    auto tmp_func = new FunctionType(result, params);
    tmp_func->is_variable_args = is_variable_args;
    return tmp_func;
}

unsigned FunctionType::get_num_of_args() const { return args_.size(); }

Type *FunctionType::get_param_type(unsigned i) const { return args_[i]; }

Type *FunctionType::get_return_type() const { return result_; }

Type *FunctionType::get_arg_type(unsigned int i) const { return args_[i]; }

string FunctionType::print() {
    string type_ir;
    type_ir += dynamic_cast<FunctionType *>(this)->get_return_type()->print();
    type_ir += " (";
    for (int i = 0; i < dynamic_cast<FunctionType *>(this)->get_num_of_args();
         i++) {
        if (i) type_ir += ", ";
        type_ir +=
            dynamic_cast<FunctionType *>(this)->get_param_type(i)->print();
    }
    if (is_variable_args) {
        type_ir += ", ...";
    }
    type_ir += ")";
    return type_ir;
}

PtrType::PtrType(Type *contained)
    : Type(Type::type::LIST, contained->get_module()) {
    contained_ = contained;
}

PtrType *PtrType::get(Type *contained) {
    auto res = contained->get_module()->get_ptr_type(contained);
    return res;
}

string PtrType::print() {
    string type_ir;
    type_ir += "ptr";
    return type_ir;
}

LabelType *LabelType::get(string str_, Class *stored_, Module *m) {
    return new LabelType(std::move(str_), stored_, m);
}
LabelType::LabelType(string label_, Class *stored_, Module *m)
    : Type(Type::type::LABEL, m), label_(std::move(label_)), stored_(stored_) {}
string LabelType::get_label() const { return label_; }

}  // namespace lightir
