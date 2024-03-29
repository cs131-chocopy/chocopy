#pragma once

#include <vector>

#include "Value.hpp"

using std::string;
using std::vector;
namespace lightir {
class User : public Value {
   public:
    explicit User(Type *ty, const string &name = "", unsigned num_ops_ = 0);
    ~User() = default;

    vector<Value *> &get_operands();

    /* The operand starts from 0 */
    Value *get_operand(unsigned i) const;
    void set_operand(unsigned i, Value *v);
    void add_operand(Value *v);

    unsigned get_num_operand() const;

    void remove_use_of_ops();
    void remove_operands(int index1, int index2);

   private:
    vector<Value *> operands_;
    unsigned num_ops_;
};
}  // namespace lightir
