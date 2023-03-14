#pragma once

#include <iostream>
#include <utility>
#include <vector>

#include "Constant.hpp"
#include "Value.hpp"
#include "chocopy_parse.hpp"
#include "chocopy_semant.hpp"

using namespace std;
namespace lightir {

class Module;
class IntegerType;
class FunctionType;
class PtrType;
class Class;
class ConstantArray;

class Type {
   public:
    enum type {
        CLASS_ANON = -7,
        LABEL,
        FUNC,
        VOID,
        VECTOR,
        LIST,
        OBJECT,
        INT,
        BOOL,
        STRING,
        CLASS
    };

    explicit Type(type tid, Module *m);
    explicit Type(type tid) : Type(tid, nullptr){};
    virtual ~Type() = default;

    constexpr int get_type_id() const { return get_underlying<type>(tid_); }

    constexpr bool is_class_anon() const { return tid_ == type::CLASS_ANON; }

    constexpr bool is_func_type() const { return tid_ == type::FUNC; }

    constexpr bool is_vector_type() const { return tid_ == type::VECTOR; }

    constexpr bool is_list_type() const { return tid_ == type::LIST; }

    constexpr bool is_ptr_type() const { return tid_ == type::LIST; };

    constexpr bool is_void_type() const {
        return tid_ == type::VOID; /** reserved type */
    }

    constexpr bool is_integer_type() const { return tid_ == type::INT; }

    constexpr bool is_bool_type() const {
        return tid_ == type::BOOL; /** same as int 1 */
    }

    constexpr bool is_value_type() const {
        return is_bool_type() || is_integer_type();
    }

    constexpr bool is_class_type() const { return tid_ >= type::CLASS; }

    static Type *get_void_type(Module *m);

    static Type *get_label_type(Module *m);

    static IntegerType *get_int1_type(Module *m);

    static IntegerType *get_int32_type(Module *m);

    Type *get_ptr_element_type();

    Module *get_module();

    virtual int get_size();

    virtual string print();

    std::strong_ordering operator<=>(Type rhs);
   private:
    const type tid_;
    Module *m_;
};

class IntegerType : public Type {
   public:
    explicit IntegerType(unsigned num_bits, Module *m);

    static IntegerType *get(unsigned num_bits, Module *m);

    unsigned get_num_bits() const;

    virtual string print() { return fmt::format("i{}", get_num_bits()); }

   private:
    unsigned num_bits_;
};

class FunctionType : public Type {
   public:
    FunctionType(Type *result, const vector<Type *> &params);

    static FunctionType *get(Type *result, const vector<Type *> &params);

    static FunctionType *get(Type *result, const vector<Type *> &params,
                             bool is_variable_args);

    unsigned get_num_of_args() const;

    Type *get_param_type(unsigned i) const;
    vector<Type *>::iterator param_begin() { return args_.begin(); }
    vector<Type *>::iterator param_end() { return args_.end(); }
    Type *get_return_type() const;
    Type *get_arg_type(unsigned i) const;

    virtual string print();
    bool is_variable_args = false;

   public:
    Type *result_;
    vector<Type *> args_;
};

class PtrType : public Type {
   public:
    explicit PtrType( Type *Contained);
    static PtrType *get(Type *contained);

    Type *get_element_type() const { return contained_; }

    virtual string print();

   private:
    Type *contained_;   // The element type of the array.
};

class LabelType : public Type {
   public:
    explicit LabelType(string str_, Class *stored_, Module *m);

    static LabelType *get(string str_, Class *stored_, Module *m);

    string get_label() const;
    Class *get_class() const { return stored_; };

    virtual string print() { return "%" + label_; }

   private:
    string label_;
    Class *stored_;
};

class VoidType : public Type {
   public:
    explicit VoidType(Module *m) : Type(Type::type::VOID, m){};

    static VoidType *get(Module *m) { return new VoidType(m); };

    virtual string print() { return "void"; }
};

}  // namespace lightir
