#include "chocopy_cgen.hpp"

#include <fmt/core.h>

#include <cassert>
#include <ranges>
#include <regex>
#include <string>
#include <utility>

#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "GlobalVariable.hpp"
#include "InstGen.hpp"
#include "Module.hpp"
#include "RiscVBackEnd.hpp"
#include "Type.hpp"
#include "Value.hpp"
#include "chocopy_lightir.hpp"

using namespace lightir;

namespace cgen {
string CodeGen::generateModuleCode() {
    string asm_code;
    asm_code += ".data\n";
    for (auto &classInfo : this->module->get_class()) {
        asm_code += backend->emit_prototype(*classInfo);
    }
    asm_code += generateGlobalVarsCode();

    asm_code += ".text\n";
    for (auto func : this->module->get_functions()) {
        assert(dynamic_cast<Function *>(func));
        if (func->get_basic_blocks().size()) {
            asm_code += CodeGen::generateFunctionCode(func);
        }
    }
    return asm_code;
}

string CodeGen::generateFunctionCode(Function *func) {
    string asm_code;
    asm_code +=
        fmt::format(".globl {}\n{}:\n", func->get_name(), func->get_name());
    current_function = func;
    // TODO: Generate function code
    return asm_code;
}

CodeGen::CodeGen(shared_ptr<Module> module)
    : module(std::move(module)) {}

string CodeGen::generateBasicBlockCode(BasicBlock *bb) {
    std::string asm_code;
    current_basic_block = bb;
    for (auto &inst : bb->get_instructions()) {
        asm_code += CodeGen::generateInstructionCode(inst);
    }
    asm_code += generateBasicBlockPostCode(bb);
    return asm_code;
}
string CodeGen::generateBasicBlockPostCode(BasicBlock *bb) {
    std::string asm_code;
    // TODO: Generate phi code
    return asm_code;
}
string CodeGen::generateInstructionCode(Instruction *inst) {
    using Reg = InstGen::Reg;
    using Addr = InstGen::Addr;
    std::string asm_code;
    auto &ops = inst->get_operands();
    // TODO: Generate instruction code
    switch (inst->get_instr_type()) {
        case lightir::Instruction::Ret:
        case lightir::Instruction::Br:
        case lightir::Instruction::Neg:
        case lightir::Instruction::Not:
        case lightir::Instruction::Add:
        case lightir::Instruction::Sub:
        case lightir::Instruction::Mul:
        case lightir::Instruction::Div:
        case lightir::Instruction::Rem:
        case lightir::Instruction::And:
        case lightir::Instruction::Or:
        case lightir::Instruction::Alloca:
        case lightir::Instruction::Load:
        case lightir::Instruction::Store:
        case lightir::Instruction::ICmp:
        case lightir::Instruction::PHI:
        case lightir::Instruction::Call:
        case lightir::Instruction::GEP:
        case lightir::Instruction::ZExt:
        case lightir::Instruction::BitCast:
        case lightir::Instruction::ASM:
        case lightir::Instruction::InElem:
        case lightir::Instruction::ExElem:
        case lightir::Instruction::Trunc:
        case lightir::Instruction::VExt:
        case lightir::Instruction::Shl:
        case lightir::Instruction::AShr:
        case lightir::Instruction::LShr:
            break;
    }
    return asm_code;
}
string CodeGen::generateFunctionCall(Instruction *inst, const string &call_inst,
                                     vector<Value *> ops) {
    string asm_code;
    // TODO: Generate function call code
    return asm_code;
}
string CodeGen::generateGlobalVarsCode() {
    GOT.clear();
    string asm_code;
    // TODO: Generate Global Variables
    return asm_code;
}
string CodeGen::generateInitializerCode(Constant *init) {
    string asm_code;
    // TODO: Generate Constant
    return asm_code;
}

string CodeGen::stackToReg(InstGen::Addr addr, InstGen::Reg reg) {
    if (-2048 <= addr.getOffset() && addr.getOffset() < 2048) {
        return backend->emit_lw(reg, addr.getReg(), addr.getOffset());
    } else {
        string asm_code;
        const auto t0 = InstGen::Reg("t0");
        asm_code += backend->emit_li(t0, addr.getOffset());
        asm_code += backend->emit_add(t0, addr.getReg(), t0);
        asm_code += backend->emit_lw(reg, t0, 0);
        return asm_code;
    }
}
string CodeGen::regToStack(InstGen::Reg reg, InstGen::Addr addr) {
    if (-2048 <= addr.getOffset() && addr.getOffset() < 2048) {
        return backend->emit_sw(reg, addr.getReg(), addr.getOffset());
    } else {
        string asm_code;
        const auto t0 = InstGen::Reg("t0");
        asm_code += backend->emit_li(t0, addr.getOffset());
        asm_code += backend->emit_add(t0, addr.getReg(), t0);
        asm_code += backend->emit_sw(reg, t0, 0);
        return asm_code;
    }
}

pair<int, bool> CodeGen::getConstIntVal(Value *val) {
    auto const_val = dynamic_cast<ConstantInt *>(val);
    auto inst_val = dynamic_cast<Instruction *>(val);
    if (const_val) {
        return std::make_pair(const_val->get_value(), true);
    } else if (inst_val) {
        auto op_list = inst_val->get_operands();
        if (dynamic_cast<BinaryInst *>(val)) {
            auto val_0 = CodeGen::getConstIntVal(op_list.at(0));
            auto val_1 = CodeGen::getConstIntVal(op_list.at(1));
            if (val_0.second && val_1.second) {
                int ret = 0;
                bool flag = true;
                switch (inst_val->get_instr_type()) {
                    case Instruction::Add:
                        ret = val_0.first + val_1.first;
                        break;
                    case Instruction::Sub:
                        ret = val_0.first - val_1.first;
                        break;
                    case Instruction::And:
                        ret = val_0.first & val_1.first;
                        break;
                    case Instruction::Or:
                        ret = val_0.first | val_1.first;
                        break;
                    case Instruction::Mul:
                        ret = val_0.first * val_1.first;
                        break;
                    case Instruction::Div:
                        ret = val_0.first / val_1.first;
                        break;
                    case Instruction::Rem:
                        ret = val_0.first % val_1.first;
                        break;
                    default:
                        flag = false;
                        break;
                }
                return std::make_pair(ret, flag);
            } else {
                return std::make_pair(0, false);
            }
        }
        if (dynamic_cast<UnaryInst *>(val)) {
            auto val_0 = CodeGen::getConstIntVal(op_list.at(0));
            if (val_0.second) {
                int ret = 0;
                bool flag = true;
                switch (inst_val->get_instr_type()) {
                    case Instruction::Not:
                        ret = !val_0.first;
                        break;
                    case Instruction::Neg:
                        ret = -val_0.first;
                        break;
                    default:
                        flag = false;
                        break;
                }
                return std::make_pair(ret, flag);
            } else {
                return std::make_pair(0, false);
            }
        }
    } else if (dynamic_cast<ConstantNull *>(val)) {
        return std::make_pair(0, true);
    }
    LOG(ERROR) << "Function getConstIntVal exception!";
    exit(EXIT_FAILURE);
}
string CodeGen::comment(const string &s) {
    std::string asm_code;
    asm_code += fmt::format("# {}\n", s);
    return asm_code;
}
string CodeGen::comment(const string &t, const string &s) {
    std::string asm_code;
    asm_code += fmt::format("{:<42}{:<42}\n", t, fmt::format("# {}", s));
    return asm_code;
}
}  // namespace cgen

#ifdef PA4
int main(int argc, char *argv[]) {
    string target_path;
    string input_path;

    bool emit = false;
    bool run = false;
    bool assem = false;

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == "-h"s || argv[i] == "--help"s) {
            print_help(argv[0]);
            return 0;
        } else if (argv[i] == "-o"s) {
            if (target_path.empty() && i + 1 < argc) {
                target_path = argv[i + 1];
                i += 1;
            } else {
                print_help(argv[0]);
                return 0;
            }
        } else if (argv[i] == "-emit"s) {
            emit = true;
        } else if (argv[i] == "-assem"s) {
            assem = true;
        } else if (argv[i] == "-run"s) {
            run = true;
        } else {
            if (input_path.empty()) {
                input_path = argv[i];
                if (target_path.empty())
                    target_path = replace_all(input_path, ".py", "");
            } else {
                print_help(argv[0]);
                return 0;
            }
        }
    }

    std::unique_ptr<parser::Program> tree(parse(input_path.c_str()));
    if (tree->errors->compiler_errors.size() != 0) {
        cout << "Syntax Error" << endl;
        return 0;
    }

    auto symboltableGenerator = semantic::SymbolTableGenerator(*tree);
    tree->accept(symboltableGenerator);
    if (tree->errors->compiler_errors.size() == 0) {
        auto typeChecker = semantic::TypeChecker(*tree);
        tree->accept(typeChecker);
    }

    if (tree->errors->compiler_errors.size() != 0) {
        cout << "Type Error" << endl;
        return 0;
    }

    std::shared_ptr<lightir::Module> m;
    auto LightWalker = lightir::LightWalker(*tree);
    tree->accept(LightWalker);
    m = LightWalker.get_module();
    m->source_file_name_ = input_path;

    string IR = fmt::format(
        "; ModuleID = \"{}\"\n"
        "source_filename = \"{}\"\n"
        "{}",
        m->module_name_, m->source_file_name_, m->print());

    std::ofstream output_stream(target_path + ".ll");
    output_stream << IR;
    output_stream.flush();

    if (emit) {
        cout << IR;
    }

    cgen::CodeGen code_generator(m);
    string asm_code = code_generator.generateModuleCode();

    std::ofstream output_stream1(target_path + ".s");
    output_stream1 << asm_code;
    output_stream1.flush();

    if (assem) {
        cout << asm_code;
    }

    if (run) {
        auto generate_exec = fmt::format(
            "riscv64-elf-gcc -mabi=ilp32 -march=rv32imac -g "
            "-o {} {}.s "
            "-L./ -L./build -L../build -lchocopy_stdlib",
            target_path, target_path);
        int re_code_0 = std::system(generate_exec.c_str());

        auto qemu_run = fmt::format("qemu-riscv32 {}", target_path);
        int re_code_1 = std::system(qemu_run.c_str());
    }
    return 0;
}
#endif
