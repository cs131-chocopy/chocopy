#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "chocopy_logging.hpp"

using std::string;
using std::string_view;
using std::vector;
namespace cgen {
class RiscVBackEnd;
extern vector<string> reg_name;
class InstGen {
   public:
    static const int max_reg_id = 31;
    class Reg;

    class Value {
       public:
        virtual constexpr bool is_reg() const = 0;
        virtual constexpr bool is_constant() const = 0;
        virtual constexpr bool has_shift() const = 0;
        virtual constexpr string get_name() const = 0;
    };

    class Reg : public Value {
        int id;

       public:
        explicit Reg(int id) : id(id) {
            if (id < 0 || id > max_reg_id) {
                LOG(ERROR) << "Invalid Reg ID!";
                abort();
            }
        }
        Reg(const string &name) {
            id = std::distance(
                reg_name.cbegin(),
                std::find_if(reg_name.cbegin(), reg_name.cend(),
                             [&name](const string &s) { return s == name; }));
        }
        bool is_reg() const override { return true; }
        bool is_constant() const override { return false; }
        bool has_shift() const override { return false; }
        int getID() const { return this->id; }
        string get_name() const override { return reg_name[id]; }
        constexpr bool operator<(const Reg &rhs) const {
            return this->id < rhs.id;
        }
        constexpr bool operator==(const Reg &rhs) const {
            return this->id == rhs.id;
        }
        constexpr bool operator!=(const Reg &rhs) const {
            return this->id != rhs.id;
        }
    };

    class Addr {
        Reg reg;
        int offset;
        string str;

       public:
        explicit Addr(Reg reg, int offset)
            : reg(std::move(reg)), offset(offset) {}

        Addr(string str) : str(std::move(str)), reg(Reg(0)), offset(0) {}
        Addr(string_view str)
            : str(str.begin(), str.end()), reg(Reg(0)), offset(0) {}
        Addr(const char *str) : str(str), reg(Reg(0)), offset(0) {}
        Reg getReg() const { return this->reg; }
        void setReg(Reg reg) { this->reg = reg; }
        int getOffset() const { return this->offset; }
        string get_name() const;
    };

    class Constant : public Value {
        int value;

       public:
        explicit Constant(int value) : value(value) {}
        bool is_reg() const override { return false; }
        bool is_constant() const override { return true; }
        bool has_shift() const override { return false; }
        int getValue() const { return this->value; }
        string get_name() const override {
            return fmt::format("#{}", this->value);
        }
    };

    class Label {
        string label;
        int offset;

       public:
        explicit Label(string label, int offset)
            : label(std::move(label)), offset(offset) {}
        explicit Label(string label) : label(std::move(label)), offset(0) {}
        string get_name() const { return fmt::format("{}+{}", label, offset); }
    };

    static string set_value(const Reg &target, const Constant &source);
    static string get_address(const Reg &target, const Constant &source) {
        return nullptr;
    };

    static const int imm_8_max = 255;
    static const int imm_16_max = 65535;
};

}  // namespace cgen
