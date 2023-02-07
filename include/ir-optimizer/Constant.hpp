#pragma once

#include <map>
#include <utility>

#include "User.hpp"
#include "Value.hpp"
#include "chocopy_parse.hpp"

using std::string;
using std::vector;
namespace lightir {
class Class;
class Module;
class PtrType;
class Constant : public User {
   public:
    explicit Constant(Type *ty, const string &name = "", unsigned num_ops = 0)
        : User(ty, name, num_ops) {}
    ~Constant() = default;
};

class ConstantStr : public Constant {
   private:
    string value_;
    int id_;
    bool header_print_ = true;

   public:
    static string get_value(ConstantStr *const_val) {
        return const_val->value_;
    }
    string get_name() override { return fmt::format("const_{}", id_); }
    string get_value() const { return value_; }
    int get_id() const { return id_; }
    static ConstantStr *get(const string &val, int id, Module *m);
    string print() override;
    ConstantStr(Type *ty, string val, int id)
        : Constant(ty, "", 0), id_(id), value_(std::move(val)) {}
};

class ConstantInt : public Constant {
   private:
    int value_;

   public:
    ConstantInt(Type *ty, int val) : Constant(ty, "", 0), value_(val) {}
    static int get_value(ConstantInt *const_val) { return const_val->value_; }
    int get_value() const { return value_; }
    static ConstantInt *get(int val, Module *m);
    static ConstantInt *get(bool val, Module *m);
    void set_value(int value) { value_ = value; }
    string print() override;
};

class ConstantBoxInt : public Constant {
   private:
    int value_;
    int id_;
    bool header_print_ = true;

   public:
    static int get_value(ConstantBoxInt *const_val) {
        return const_val->value_;
    }
    string get_name() override { return fmt::format("const_{}", id_); }
    int get_value() const { return value_; }
    int get_id() const { return id_; }
    static ConstantBoxInt *get(Class *int_class, int val, int id);
    string print() override;
    ConstantBoxInt(Type *ty, int val, int id)
        : Constant(ty, "", 0), id_(id), value_(val) {}
};

class ConstantBoxBool : public Constant {
   private:
    bool value_;
    int id_;
    bool header_print_ = true;

   public:
    static int get_value(ConstantBoxBool *const_val) {
        return const_val->value_;
    }
    string get_name() override { return fmt::format("const_{}", id_); }
    int get_value() const { return value_; }
    int get_id() const { return id_; }
    static ConstantBoxBool *get(Class *bool_class, bool val, int id);
    string print() override;
    ConstantBoxBool(Type *ty, bool val, int id)
        : Constant(ty, "", 0), id_(id), value_(val) {}
};

class ConstantNull : public Constant {
   public:
    ConstantNull(Type *ty) : Constant(ty, "", 0) {}
    static ConstantNull *get(Type *ty) { return new ConstantNull(ty); };

    string print() override;
};

class ConstantZero : public Constant {
   private:
    explicit ConstantZero(Type *ty) : Constant(ty, "", 0) {}

   public:
    static ConstantZero *get(Type *ty, Module *m);
    string print() override;
};
}  // namespace lightir
