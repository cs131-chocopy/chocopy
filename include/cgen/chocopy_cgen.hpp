#pragma once

#include <vector>
#include <algorithm>
#include <iostream>
#include <queue>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "GlobalVariable.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "RiscVBackEnd.hpp"
#include "Type.hpp"
#include "User.hpp"
#include "Value.hpp"

using namespace lightir;
using std::string;
using std::string_view;

namespace cgen {
class InstGen;
class RiscVBackEnd;

const string attribute = "rv32i2p0_m2p0_a2p0_c2p0";
class CodeGen {
   private:
    shared_ptr<Module> module;
    unique_ptr<RiscVBackEnd> backend = make_unique<RiscVBackEnd>();

    map<std::string, int> GOT;
    BasicBlock *current_basic_block;
    Function *current_function;

   public:
    explicit CodeGen(shared_ptr<Module> module);
    [[nodiscard]] string generateModuleCode();

    [[nodiscard]] string generateFunctionCode(Function *func);

    [[nodiscard]] string generateBasicBlockCode(BasicBlock *bb);
    [[nodiscard]] string generateBasicBlockPostCode(BasicBlock *bb);

    [[nodiscard]] string generateInstructionCode(Instruction *inst);
    [[nodiscard]] string generateFunctionCall(Instruction *inst,
                                              const string &call_inst,
                                              vector<Value *> ops);

    [[nodiscard]] string generateGlobalVarsCode();
    [[nodiscard]] string generateInitializerCode(Constant *init);
    [[nodiscard]] pair<int, bool> getConstIntVal(Value *val);

    [[nodiscard]] string stackToReg(InstGen::Addr addr, InstGen::Reg reg);
    [[nodiscard]] string regToStack(InstGen::Reg reg, InstGen::Addr addr);

    string comment(const string &s);
    string comment(const string &t, const string &s);
};

}  // namespace cgen
