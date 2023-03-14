#include "chocopy_lightir.hpp"

#include <cassert>
#include <fstream>
#include <ranges>
#include <regex>
#include <string>
#include <utility>

#include "BasicBlock.hpp"
#include "Class.hpp"
#include "ClassDefType.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "FunctionDefType.hpp"
#include "GlobalVariable.hpp"
#include "Module.hpp"
#include "Type.hpp"
#include "Value.hpp"
#include "chocopy_parse.hpp"
#include "chocopy_semant.hpp"

namespace lightir {
#define CONST(num) ConstantInt::get(num, &*module)

int LightWalker::get_next_type_id() { return next_type_id++; }
int LightWalker::get_const_type_id() { return next_const_id++; }

string LightWalker::get_fully_qualified_name(semantic::FunctionDefType *func) {
    // TODO: FQN
    // check ChocoPy v2.2: RISC-V Implementation Guide 2.Naming conventions
    return "";
}

Type *LightWalker::semantic_type_to_llvm_type(semantic::SymbolType *type) {
    if (type->is_list_type()) {
        return PtrType::get(list_class);
    } else if (dynamic_cast<semantic::ClassValueType *>(type)) {
        if (type->get_name() == "int") {
            return i32_type;
        } else if (type->get_name() == "bool") {
            return i1_type;
        } else if (type->get_name() == "str") {
            return ptr_str_type;
        } else if (type->get_name() == "<None>") {
            return ptr_obj_type;
        } else {
            const auto class_ =
                dynamic_cast<Class *>(scope.find_in_global(type->get_name()));
            assert(class_);
            return PtrType::get(class_);
        }
    } else if (type->is_func_type()) {
        // only support global function
        const auto func_def_type =
            dynamic_cast<semantic::FunctionDefType *>(type);
        std::vector<Type *> arg_types;
        for (auto param : func_def_type->params) {
            arg_types.emplace_back(semantic_type_to_llvm_type(param.get()));
        }
        auto func_return_type =
            semantic_type_to_llvm_type(func_def_type->return_type.get());
        auto func_type = FunctionType::get(func_return_type, arg_types);
        return func_type;
    }
    assert(0);
}

LightWalker::LightWalker(parser::Program &program)
    : sym(&program.symbol_table) {
    module = std::make_unique<Module>("ChocoPy code");
    builder = std::make_unique<IRBuilder>(nullptr, module.get());

    void_type = Type::get_void_type(&*module);
    i32_type = Type::get_int32_type(&*module);
    i1_type = Type::get_int1_type(&*module);
    i8_type = IntegerType::get(8, &*module);
    ptr_i8_type = PtrType::get(i8_type);

    /** Get the class ready. */
    object_class = new Class(&*module, "object", 0, nullptr, true, true);
    ptr_obj_type = PtrType::get(object_class->get_type());
    null = ConstantNull::get(ptr_obj_type);

    auto object_init =
        Function::create(FunctionType::get(ptr_obj_type, {ptr_obj_type}),
                         "$object.__init__", &*module);
    object_class->add_method(object_init);
    ptr_ptr_obj_type = PtrType::get(ptr_obj_type);

    int_class = new Class(&*module, "int", 1, nullptr, true, true);
    ptr_int_type = PtrType::get(int_class->get_type());
    int_class->add_method(object_init);
    int_class->add_attribute(new AttrInfo(i32_type, "__int__"));

    bool_class = new Class(&*module, "bool", 2, nullptr, true, true);
    ptr_bool_type = PtrType::get(bool_class->get_type());
    bool_class->add_method(object_init);
    bool_class->add_attribute(new AttrInfo(i1_type, "__bool__"));

    Class *str_class = new Class(module.get(), "str", 3, object_class, true);
    str_class->add_attribute(new AttrInfo(i32_type, "__len__", 0));
    str_class->add_attribute(new AttrInfo(ptr_i8_type, "__str__"));
    str_class->add_method(object_init);
    ptr_str_type = PtrType::get(str_class->get_type());

    list_class = new Class(&*module, ".list", -1, nullptr, true);
    list_class->add_method(object_init);
    list_class->add_attribute(new AttrInfo(i32_type, "__len__", 0));
    list_class->add_attribute(new AttrInfo(ptr_ptr_obj_type, "__list__", 0));

    auto TyListClass = list_class->get_type();
    ptr_list_type = PtrType::get(TyListClass);

    /** Predefined functions. */
    // print Out Of Bound error and exit
    error_oob_fun = Function::create(FunctionType::get(void_type, {}),
                                     "error.OOB", module.get());

    // print None error and exit
    error_none_fun = Function::create(FunctionType::get(void_type, {}),
                                      "error.None", module.get());

    // print Div Zero error and exit
    error_div_fun = Function::create(FunctionType::get(void_type, {}),
                                     "error.Div", module.get());

    // param: number of elements, element, element, ... (variable args)
    // return: pointer to a list
    construct_list_fun = Function::create(
        FunctionType::get(ptr_list_type, {i32_type, i32_type}, true),
        "construct_list", module.get());

    // param: pointer to a list, pointer to a list
    // return: pointer to a new list
    concat_fun = Function::create(
        FunctionType::get(ptr_list_type, {ptr_list_type, ptr_list_type}),
        "concat_list", module.get());

    // param: char
    // return: pointer to a str object
    makestr_fun = Function::create(FunctionType::get(ptr_str_type, {i8_type}),
                                   "makestr", module.get());

    // param: pointer to an object
    len_fun = Function::create(FunctionType::get(i32_type, {ptr_obj_type}),
                               "$len", module.get());

    // param: pointer to an object
    print_fun = Function::create(FunctionType::get(void_type, {ptr_obj_type}),
                                 "print", module.get());

    // param: pointer to object
    // return: pointer to a new object with the same type
    alloc_fun =
        Function::create(FunctionType::get(ptr_obj_type, {ptr_obj_type}),
                         "alloc_object", module.get());

    // param: bool value
    // return: pointer to a bool object
    makebool_fun = Function::create(FunctionType::get(ptr_bool_type, {i1_type}),
                                    "makebool", module.get());

    // param: int value
    // return: pointer to a int object
    makeint_fun = Function::create(FunctionType::get(ptr_int_type, {i32_type}),
                                   "makeint", module.get());

    // return: pointer to a str object
    input_fun = Function::create(FunctionType::get(ptr_str_type, {}), "$input",
                                 module.get());

    // param: pointer to str object, pointer to str object
    // return: bool
    auto str_compare_type =
        FunctionType::get(i1_type, {ptr_str_type, ptr_str_type});
    streql_fun =
        Function::create(str_compare_type, "str_object_eq", module.get());
    strneql_fun =
        Function::create(str_compare_type, "str_object_neq", module.get());

    // param: pointer to str object, pointer to str object
    // return: pointer to a new str object
    strcat_fun = Function::create(
        FunctionType::get(ptr_str_type, {ptr_str_type, ptr_str_type}),
        "str_object_concat", module.get());

    scope.enter();
    scope.push_in_global("object", object_class);
    scope.push_in_global("int", int_class);
    scope.push_in_global("bool", bool_class);
    scope.push_in_global("str", str_class);
    scope.push_in_global(".list", list_class);
}

// Useless visitors
void LightWalker::visit(parser::Errors &) { assert(0); };
void LightWalker::visit(parser::Node &) { assert(0); };
void LightWalker::visit(parser::TypeAnnotation &) { assert(0); }
void LightWalker::visit(parser::TypedVar &) { assert(0); }
void LightWalker::visit(parser::PassStmt &) {}
void LightWalker::visit(parser::GlobalDecl &) {}
void LightWalker::visit(parser::NonlocalDecl &) {}

/**
 * Analyze PROGRAM, creating Info objects for all symbols.
 * Populate the global symbol table.
 */
void LightWalker::visit(parser::Program &node) {
    auto main_func_type = FunctionType::get(void_type, {});
    auto main_func = Function::create(main_func_type, "main", module.get());
    auto main_bb = BasicBlock::create(&*module, "", main_func);
    builder->set_insert_point(main_bb);
    scope.push_in_global("$main", main_func);

    // TODO: maybe you have to add/modifiy some code here, it is up to you
    for (const auto &decl : node.declarations) {
        decl->accept(*this);
    }
    for (const auto &stmt : node.statements) {
        stmt->accept(*this);
    }

    // no need to change the code below
    for (auto &func : this->module->get_functions()) {
        func->set_instr_name();
    }

    builder->create_asm(
        "li a0, 0 \\0A"
        "li a7, 93 #exit system call\\0A"
        "ecall");
    builder->create_void_ret();
}

void LightWalker::visit(parser::ExprStmt &node) { node.expr->accept(*this); }
void LightWalker::visit(parser::IntegerLiteral &node) {
    auto C = CONST(node.value);
    visitor_return_value = C;
}
void LightWalker::visit(parser::BoolLiteral &node) {
    auto C = CONST(node.bin_value);
    visitor_return_value = C;
}
void LightWalker::visit([[maybe_unused]] parser::NoneLiteral &node) {
    visitor_return_value = null;
}

void LightWalker::visit(parser::AssignStmt &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::BinaryExpr &node) {
    // TODO: Implement this, this is not complete
    Instruction *result;
    node.left->accept(*this);
    auto v1 = this->visitor_return_value;
    node.right->accept(*this);
    auto v2 = this->visitor_return_value;
    if (node.operator_ == "+") {
        if (node.left->inferredType->get_name() == "int") {
            result = builder->create_iadd(v1, v2);
        } else if (node.left->inferredType->get_name() == "str") {
            result = builder->create_call(strcat_fun, {v1, v2});
            result->set_type(ptr_str_type);
        } else {
            result = builder->create_call(concat_fun, {v1, v2});
        }
    } else {
        assert(false);
    }
    visitor_return_value = result;
}
void LightWalker::visit(parser::CallExpr &node) {
    // TODO: Implement this, this is not complete
    const auto &func_name = node.function->name;
    if (func_name == "print") {
        node.args.at(0)->accept(*this);
        auto v1 = this->visitor_return_value;
        if (node.args.at(0)->inferredType->get_name() == "int") {
            auto t = builder->create_call(makeint_fun, {v1});
            builder->create_call(print_fun, {t});
        } else if (node.args.at(0)->inferredType->get_name() == "bool") {
            v1->set_type(IntegerType::get(1, module.get()));
            auto t = builder->create_call(makebool_fun, {v1});
            builder->create_call(print_fun, {t});
        } else {
            builder->create_call(print_fun, {v1});
        }
    }
}
void LightWalker::visit(parser::ClassDef &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::ClassType &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::ForStmt &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::FuncDef &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::Ident &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::IfExpr &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::ListExpr &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::ListType &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::MemberExpr &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::IfStmt &node) {
    node.condition->accept(*this);
    auto cond = visitor_return_value;
    auto b = builder->get_insert_block();
    auto b_true = BasicBlock::create(module.get(), "", b->get_parent());
    auto b_end = BasicBlock::create(module.get(), "", b->get_parent());
    if (node.el == parser::IfStmt::cond::THEN) {
        builder->create_cond_br(cond, b_true, b_end);
        builder->set_insert_point(b_true);
        for (const auto &stmt : node.thenBody) {
            stmt->accept(*this);
        }
        builder->create_br(b_end);
    } else {
        auto b_false = BasicBlock::create(module.get(), "", b->get_parent());
        builder->create_cond_br(cond, b_true, b_false);
        builder->set_insert_point(b_true);
        for (const auto &stmt : node.thenBody) {
            stmt->accept(*this);
        }
        builder->create_br(b_end);
        builder->set_insert_point(b_false);
        if (node.el == parser::IfStmt::cond::THEN_ELSE) {
            for (const auto &stmt : node.elseBody) {
                stmt->accept(*this);
            }
        } else {
            node.elifBody->accept(*this);
        }
        builder->create_br(b_end);
    }
    builder->set_insert_point(b_end);
}
void LightWalker::visit(parser::MethodCallExpr &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::ReturnStmt &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::StringLiteral &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::UnaryExpr &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::VarDef &node) {
    // TODO: Implement this, this is not complete
    if (scope.in_global()) {
        if (node.var->type->get_name() == "int") {
            GlobalVariable::create(
                node.var->identifier->name, module.get(),
                IntegerType::get(32, module.get()), false,
                ConstantInt::get(node.value->int_value, module.get()));
        } else {
            assert(0);
        }
    } else {
        assert(0);
    }
}
void LightWalker::visit(parser::WhileStmt &node) {
    // TODO: Implement this
}
void LightWalker::visit(parser::IndexExpr &node) {
    // TODO: Implement this
}
}  // namespace lightir

void print_help(const string_view &exe_name) {
    std::cout << fmt::format(
                     "Usage: {} [ -h | --help ] [ -o <target-file> ] [ -emit ] "
                     "[ -run ] [ -assem ] <input-file>",
                     exe_name)
              << std::endl;
}

#ifdef PA3
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

    if (emit) {
        cout << IR;
    }

    if (assem || run) {
        auto generate_assem = fmt::format(
            "llc -opaque-pointers -O0 -verify-machineinstrs "
            "-mtriple=riscv32-unknown-elf "
            "-o {}.s {}.ll "
            R"(&& /usr/bin/sed -i 's/.*addrsig.*//g' )"
            "{}.s",
            target_path, target_path, target_path);
        int re_code = std::system(generate_assem.c_str());
        std::cerr << "llc return code: " << re_code << std::endl;
    }

    if (run) {
        auto generate_exec = fmt::format(
            "riscv64-elf-gcc -mabi=ilp32 -march=rv32imac -g "
            "-o {} {}.s "
            "-L./ -L./build -L../build -lchocopy_stdlib",
            target_path, target_path);
        int re_code_0 = std::system(generate_exec.c_str());
        std::cerr << "gcc return code: " << re_code_0 << std::endl;

        auto qemu_run = fmt::format("qemu-riscv32 {}", target_path);
        int re_code_1 = std::system(qemu_run.c_str());
        std::cerr << "qemu return code: " << re_code_1 << std::endl;
    }

    return 0;
}
#endif
