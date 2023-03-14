#include "Constant.hpp"

#include <fmt/core.h>

#include <iostream>

#include "Module.hpp"
#include "Type.hpp"
#include "chocopy_parse.hpp"

using namespace std;
namespace lightir {

ConstantInt *ConstantInt::get(int val, Module *m) {
    return new ConstantInt(Type::get_int32_type(m), val);
}

ConstantInt *ConstantInt::get(bool val, Module *m) {
    return new ConstantInt(Type::get_int1_type(m), val ? 1 : 0);
}

string ConstantInt::print() {
    string const_ir;
    const_ir += fmt::format("{} ", this->get_type()->print());
    Type *ty = this->get_type();
    if (ty->is_integer_type() &&
        dynamic_cast<IntegerType *>(ty)->get_num_bits() == 1) {
        /** int 1 */
        const_ir += (this->get_value() == 0) ? "false" : "true";
    } else {
        /** int 32 */
        const_ir += fmt::format("{}", this->get_value());
    }
    return const_ir;
}

string ConstantNull::print() {
    return fmt::format("{} null", this->get_type()->print());
}

ConstantZero *ConstantZero::get(Type *ty, [[maybe_unused]] Module *m) {
    return new ConstantZero(ty);
}

std::string ConstantZero::print() { return "zeroinitializer"; }
ConstantStr *ConstantStr::get(const string &val, int id, Module *m) {
    return new ConstantStr(PtrType::get(IntegerType::get(8, m)), val, id);
}
string ConstantStr::print() {
    string const_ir;
    if (header_print_) {
        const_ir += "@const_" + std::to_string(id_) +
                    " = global %$str$prototype_type {\n" +
                    fmt::format(
                        "  i32 3,\n  i32 5,\n  %$str$dispatchTable_type* "
                        "@$str$dispatchTable,\n  i32 {},\n  "
                        "i8* @str.const_{}",
                        value_.size(), id_) +
                    "\n}";
        string s;
        for (char c : value_) {
            int a = c / 16, b = c % 16;
            s += fmt::format("\\{:x}{:x}", a, b);
        }
        const_ir += fmt::format(
            "\n@str.const_{} = private unnamed_addr global [{} x i8] "
            "c\"{}\\00\", align 1\n",
            id_, value_.size() + 1, s);
        header_print_ = false;
    } else {
        const_ir += fmt::format("@const_{}", id_);
    }
    return const_ir;
}

ConstantBoxInt *ConstantBoxInt::get(Class *int_class, int val, int id) {
    return new ConstantBoxInt(int_class, val, id);
}
string ConstantBoxInt::print() {
    string const_ir;
    if (header_print_) {
        const_ir += "@const_" + std::to_string(id_) +
                    " = global %$int$prototype_type {\n" +
                    fmt::format(
                        "  i32 1,\n  i32 4,\n  %$int$dispatchTable_type* "
                        "@$int$dispatchTable,\n  i32 {}\n}}",
                        value_);
        header_print_ = false;
    } else {
        const_ir += fmt::format("@const_{}", id_);
    }
    return const_ir;
}

ConstantBoxBool *ConstantBoxBool::get(Class *bool_class, bool val, int id) {
    return new ConstantBoxBool(bool_class, val, id);
}
string ConstantBoxBool::print() {
    string const_ir;
    if (header_print_) {
        const_ir += "@const_" + std::to_string(id_) +
                    " = global %$bool$prototype_type {\n" +
                    fmt::format(
                        "  i32 2,\n  i32 4,\n  %$bool$dispatchTable_type* "
                        "@$bool$dispatchTable,\n  i1 {}\n}}",
                        (int)value_);
        header_print_ = false;
    } else {
        const_ir += fmt::format("@const_{}", id_);
    }
    return const_ir;
}
}  // namespace lightir
