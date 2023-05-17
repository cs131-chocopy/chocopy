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

/** Use the symbol table to generate the type id */
int LightWalker::get_next_type_id() { return next_type_id++; }
int LightWalker::get_const_type_id() { return next_const_id++; }

string LightWalker::get_fully_qualified_name(semantic::FunctionDefType *func,
                                             bool with_start_dollar = true) {
    const auto parent_sym = func->current_scope.parent;
    const auto grand_parent_sym = parent_sym->parent;
    if (grand_parent_sym) {
        for (auto &x : grand_parent_sym->tab) {
            auto parent_func =
                dynamic_cast<semantic::FunctionDefType *>(x.second.get());
            if (parent_func) {
                if (&parent_func->current_scope == parent_sym) {
                    return with_start_dollar
                               ? "$" +
                                     get_fully_qualified_name(parent_func,
                                                              false) +
                                     "." + func->get_name()
                               : get_fully_qualified_name(parent_func, false) +
                                     "." + func->get_name();
                }
            }
            auto parent_class =
                dynamic_cast<semantic::ClassDefType *>(x.second.get());
            if (parent_class) {
                if (&parent_class->current_scope == parent_sym) {
                    return "$$METHOD$$" + parent_class->get_name() + "." +
                           func->get_name();
                }
            }
        }
        assert(0 && "No parent function found");
    } else {
        return with_start_dollar ? "$" + func->get_name() : func->get_name();
    }
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

    for (const auto &decl : node.declarations) {
        if (auto node = dynamic_cast<parser::ClassDef *>(decl.get()); node) {
            Class *super_class = nullptr;
            for (const auto e : module->get_class()) {
                if (e->get_name() == node->superClass->name) {
                    super_class = e;
                    break;
                }
            }
            assert(super_class);

            auto class_ =
                new Class(module.get(), node->name->name, get_next_type_id(),
                          super_class, true, false, true);
            scope.push(node->name->name, class_);
        }
    }

    for (const auto &decl : node.declarations) {
        if (auto node = dynamic_cast<parser::FuncDef *>(decl.get()); node) {
            auto &func_name = node->get_id()->name;
            auto func_def_type =
                sym->declares<semantic::FunctionDefType>(func_name);
            assert(func_def_type);
            auto unique_func_name = get_fully_qualified_name(func_def_type);

            auto func_type = dynamic_cast<FunctionType *>(
                semantic_type_to_llvm_type(func_def_type));
            assert(func_type);

            auto func =
                Function::create(func_type, unique_func_name, module.get());
            scope.push(func_name, func);
        }
    }

    for (const auto &decl : node.declarations) {
        if (auto node = dynamic_cast<parser::VarDef *>(decl.get()); node) {
            decl->accept(*this);
        }
    }
    for (const auto &decl : node.declarations) {
        if (auto node = dynamic_cast<parser::VarDef *>(decl.get()); node) {
        } else {
            decl->accept(*this);
        }
    }
    for (const auto &stmt : node.statements) {
        stmt->accept(*this);
    }
    /** For Optimization Debug */
    for (auto &func : this->module->get_functions()) {
        func->set_instr_name();
    }

    builder->create_asm(
        "li a0, 0 \\0A"
        "li a7, 93 #exit system call\\0A"
        "ecall");
    builder->create_void_ret();
}

void LightWalker::visit(parser::AssignStmt &node) {
    node.value->accept(*this);
    auto v = visitor_return_value;
    for (auto &i : node.targets) {
        get_lvalue = true;
        i->accept(*this);
        get_lvalue = false;
        auto address = visitor_return_value;
        if (node.value->inferredType->get_name() == "int") {
            if (i->inferredType->get_name() == "int") {
                builder->create_store(v, address);
            } else {
                v = builder->create_call(makeint_fun, {v});
                builder->create_store(v, address);
            }
        } else if (node.value->inferredType->get_name() == "bool") {
            if (i->inferredType->get_name() == "bool") {
                builder->create_store(v, address);
            } else {
                v = builder->create_call(makebool_fun, {v});
                builder->create_store(v, address);
            }
        } else if (node.value->inferredType->get_name() == "str") {
            if (i->inferredType->get_name() == "str") {
                builder->create_store(v, address);
            } else {
                builder->create_store(v, address);
            }
        } else {
            // class assignment
            builder->create_store(v, address);
        }
    }
}
void LightWalker::visit(parser::BinaryExpr &node) {
    node.left->accept(*this);
    auto v1 = this->visitor_return_value;
    Instruction *res;
    if (node.operator_ == "and" || node.operator_ == "or") {  // short circuit
        auto b = builder->get_insert_block();
        auto b_run = BasicBlock::create(module.get(), "", b->get_parent());
        auto b_norun = BasicBlock::create(module.get(), "", b->get_parent());
        auto b_end = BasicBlock::create(module.get(), "", b->get_parent());
        if (node.operator_ == "and") {
            builder->create_cond_br(v1, b_run, b_norun);
        } else {
            builder->create_cond_br(v1, b_norun, b_run);
        }
        builder->set_insert_point(b_run);
        node.right->accept(*this);
        auto v2 = this->visitor_return_value;
        auto b_run_end = builder->get_insert_block();
        Instruction *run_res = nullptr;
        if (node.operator_ == "and") {
            run_res = builder->create_iand(v1, v2);
            run_res->set_type(i1_type);
        } else {
            run_res = builder->create_ior(v1, v2);
            run_res->set_type(i1_type);
        }
        builder->create_br(b_end);
        builder->set_insert_point(b_norun);
        Value *norun_res = nullptr;
        if (node.operator_ == "and") {
            norun_res = CONST(false);
        } else {
            norun_res = CONST(true);
        }
        builder->create_br(b_end);
        builder->set_insert_point(b_end);
        auto phi = builder->create_phi(i1_type);
        phi->set_lval(run_res);
        phi->add_phi_pair_operand(run_res, b_run_end);
        phi->add_phi_pair_operand(norun_res, b_norun);
        b_end->add_instr_begin(phi);
        res = phi;
    } else {
        node.right->accept(*this);
        auto v2 = this->visitor_return_value;
        if (node.operator_ == "+") {
            if (node.left->inferredType->get_name() == "int") {
                res = builder->create_iadd(v1, v2);
            } else if (node.left->inferredType->get_name() == "str") {
                vector<Value *> params;
                params.push_back(v1);
                params.push_back(v2);
                res = builder->create_call(strcat_fun, params);
                res->set_type(ptr_str_type);
            } else {
                vector<Value *> params;
                params.push_back(v1);
                params.push_back(v2);
                res = builder->create_call(concat_fun, params);
            }
        } else if (node.operator_ == "-") {
            res = builder->create_isub(v1, v2);
        } else if (node.operator_ == "*") {
            res = builder->create_imul(v1, v2);
        } else if (node.operator_ == "//") {
            auto t1 = builder->create_icmp_eq(v2, CONST(0));
            auto b = builder->get_insert_block();
            auto b1 = BasicBlock::create(module.get(), "", b->get_parent());
            auto b2 = BasicBlock::create(module.get(), "", b->get_parent());
            /*b->add_succ_basic_block(b1);
            b->add_succ_basic_block(b2);
            b1->add_pre_basic_block(b);
            b2->add_pre_basic_block(b);
            // To make LLVM happy, b1 will jump to b2. However this won't
            happen. So b1 -> b2 is not nessary.*/
            auto t2 = builder->create_cond_br(t1, b1, b2);
            builder->set_insert_point(b1);
            builder->create_call(error_div_fun, {});
            builder->create_br(b2);
            builder->set_insert_point(b2);
            auto t3 = builder->create_isdiv(v1, v2);
            res = t3;
        } else if (node.operator_ == "%") {
            auto t1 = builder->create_icmp_eq(v2, CONST(0));
            auto b = builder->get_insert_block();
            auto b1 = BasicBlock::create(module.get(), "", b->get_parent());
            auto b2 = BasicBlock::create(module.get(), "", b->get_parent());
            /*b->add_succ_basic_block(b1);
            b->add_succ_basic_block(b2);
            b1->add_pre_basic_block(b);
            b2->add_pre_basic_block(b);
            // To make LLVM happy, b1 will jump to b2. However this won't
            happen. So b1 -> b2 is not nessary.*/
            auto t2 = builder->create_cond_br(t1, b1, b2);
            builder->set_insert_point(b1);
            builder->create_call(error_div_fun, {});
            builder->create_br(b2);
            builder->set_insert_point(b2);
            auto t3 = builder->create_irem(v1, v2);
            res = t3;
        } else if (node.operator_ == "<") {
            res = builder->create_icmp_lt(v1, v2);
        } else if (node.operator_ == "<=") {
            res = builder->create_icmp_le(v1, v2);
        } else if (node.operator_ == "==") {
            if (node.left->inferredType->get_name() == "bool") {
                auto v1_32 = builder->create_zext(
                    v1, IntegerType::get(32, module.get()));
                auto v2_32 = builder->create_zext(
                    v2, IntegerType::get(32, module.get()));
                res = builder->create_icmp_eq(v1_32, v2_32);
            } else if (node.left->inferredType->get_name() == "int") {
                res = builder->create_icmp_eq(v1, v2);
            } else {
                vector<Value *> params;
                params.push_back(v1);
                params.push_back(v2);
                res = builder->create_call(streql_fun, params);
            }
        } else if (node.operator_ == "!=") {
            if (node.left->inferredType->get_name() == "bool") {
                auto v1_32 = builder->create_zext(
                    v1, IntegerType::get(32, module.get()));
                auto v2_32 = builder->create_zext(
                    v2, IntegerType::get(32, module.get()));
                res = builder->create_icmp_ne(v1_32, v2_32);
            } else if (node.left->inferredType->get_name() == "int") {
                res = builder->create_icmp_ne(v1, v2);
            } else {
                vector<Value *> params;
                params.push_back(v1);
                params.push_back(v2);
                res = builder->create_call(strneql_fun, params);
            }
        } else if (node.operator_ == ">") {
            res = builder->create_icmp_gt(v1, v2);
        } else if (node.operator_ == ">=") {
            res = builder->create_icmp_ge(v1, v2);
        } else if (node.operator_ == "is") {
            get_lvalue = false;
            node.left->accept(*this);
            auto l = visitor_return_value;
            get_lvalue = false;
            node.right->accept(*this);
            auto r = visitor_return_value;
            res = builder->create_icmp_eq(l, r);
        } else {
            assert(false);
        }
    }
    visitor_return_value = res;
}
void LightWalker::visit(parser::BoolLiteral &node) {
    auto C = CONST(node.bin_value);
    visitor_return_value = C;
}
void LightWalker::visit(parser::CallExpr &node) {
    const auto &func_name = node.function->name;
    if (func_name == "print") {  // FIXME: problem here.
        node.args.at(0)->accept(*this);
        auto v1 = this->visitor_return_value;
        if (node.args.at(0)->inferredType->get_name() == "int") {
            auto t = builder->create_call(makeint_fun, {v1});
            builder->create_call(print_fun, {t});
        } else if (node.args.at(0)->inferredType->get_name() == "bool") {
            v1->set_type(IntegerType::get(1, module.get()));
            auto t1 = builder->create_call(makebool_fun, {v1});
            builder->create_call(print_fun, {t1});
        } else {
            builder->create_call(print_fun, {v1});
        }
    } else if (func_name == "input") {
        visitor_return_value =
            builder->create_call(input_fun, vector<Value *>());

    } else if (func_name == "len") {
        node.args.at(0)->accept(*this);
        auto v1 = this->visitor_return_value;
        auto arg_type = node.args.at(0)->inferredType;
        if (dynamic_cast<semantic::ListValueType *>(arg_type.get()) ==
                nullptr &&
            arg_type->get_name() != "str") {
            v1 = null;
        }
        auto v_len = builder->create_call(len_fun, {v1});
        visitor_return_value = v_len;
    } else if (func_name == "int") {
        visitor_return_value = CONST(0);
    } else if (func_name == "bool") {
        visitor_return_value = ConstantInt::get(false, module.get());
    } else {
        Function *func;
        std::vector<Value *> args;
        auto is_object_init = sym->get<semantic::ClassDefType>(func_name);
        if (is_object_init) {
            auto class_type =
                dynamic_cast<Class *>(scope.find_in_global(func_name));
            assert(class_type);
            auto pointer_class_type = PtrType::get(class_type);

            auto alloc_type = alloc_fun->get_args().front()->type_;

            auto op = builder->create_bitcast(class_type, alloc_type);
            auto alloc_call = builder->create_call(alloc_fun, {op});

            func = dynamic_cast<Function *>(class_type->get_method()->at(0));
            assert(func);
            visitor_return_value =
                builder->create_bitcast(alloc_call, pointer_class_type);
            args.emplace_back(builder->create_bitcast(
                alloc_call, func->get_function_type()->get_arg_type(0)));
        } else {
            func = dynamic_cast<Function *>(scope.find_in_global(func_name));
            if (func == nullptr) {
                // lambda function
                func = dynamic_cast<Function *>(scope.find(func_name));
                auto class_instance = scope.find(func_name + "$anon");
                assert(func);
                assert(class_instance);
                args.emplace_back(class_instance);
            }
        }
        for (const auto &arg : node.args) {
            visitor_return_value = nullptr;
            arg->accept(*this);
            assert(visitor_return_value);
            args.push_back(this->visitor_return_value);
        }
        auto v = builder->create_call(func, std::move(args));
        if (!is_object_init) visitor_return_value = v;
    }
}
void LightWalker::visit(parser::ClassDef &node) {
    const auto class_type = sym->get<semantic::ClassDefType>(node.name->name);
    const auto super_class_type =
        sym->get<semantic::ClassDefType>(node.superClass->name);
    assert(super_class_type);
    auto saved_sym = sym;
    sym = &class_type->current_scope;

    auto class_ = dynamic_cast<Class *>(scope.find_in_global(node.name->name));
    assert(class_);
    auto super_class = class_->super_class_info_;

    for (const auto attr : *super_class->get_attribute()) {
        class_->add_attribute(attr);
    }
    for (const auto method : *super_class->get_method()) {
        class_->add_method(method);
    }
    for (const auto &decl : node.declaration) {
        const auto &name = decl->get_id()->name;
        if (const auto var_def = dynamic_cast<parser::VarDef *>(decl.get());
            var_def) {
            if (!super_class_type->current_scope.declares(name)) {
                var_def->var->type->accept(*this);
                assert(visitor_return_type);
                const auto attr_type = visitor_return_type;

                if (attr_type->is_integer_type()) {
                    class_->add_attribute(
                        new AttrInfo(attr_type, name,
                                     static_cast<parser::IntegerLiteral *>(
                                         var_def->value.get())
                                         ->value));
                } else if (attr_type->is_bool_type()) {
                    class_->add_attribute(new AttrInfo(
                        attr_type, name,
                        static_cast<parser::BoolLiteral *>(var_def->value.get())
                            ->bin_value));
                } else if (attr_type == ptr_str_type) {
                    const auto &str_literal =
                        dynamic_cast<parser::StringLiteral *>(
                            var_def->value.get());
                    assert(str_literal);

                    int id = get_const_type_id();
                    auto C =
                        ConstantStr::get(str_literal->value, id, module.get());
                    auto t = GlobalVariable::create(
                        "const_" + std::to_string(id), module.get(), C);
                    t->set_type(ptr_str_type);

                    class_->add_attribute(new AttrInfo(attr_type, name, t));
                } else {
                    class_->add_attribute(new AttrInfo(attr_type, name));
                }
            }
        }
    }
    for (const auto &decl : node.declaration) {
        if (const auto func_def = dynamic_cast<parser::FuncDef *>(decl.get());
            func_def) {
            auto &func_name = func_def->get_id()->name;
            auto func_def_type =
                sym->declares<semantic::FunctionDefType>(func_name);
            assert(func_def_type);
            auto unique_func_name = get_fully_qualified_name(func_def_type);

            auto func_type = dynamic_cast<FunctionType *>(
                semantic_type_to_llvm_type(func_def_type));
            assert(func_type);

            auto func =
                Function::create(func_type, unique_func_name, module.get());
            scope.push(unique_func_name, func);
            class_->add_method(func);
        }
    }
    for (const auto &decl : node.declaration) {
        if (const auto func_def = dynamic_cast<parser::FuncDef *>(decl.get());
            func_def) {
            func_def->accept(*this);
        }
    }
    sym = saved_sym;
}
void LightWalker::visit(parser::ClassType &node) {
    if (node.className == "<None>") {
        // ! FIXME: actually I don't know what I should do here.
        visitor_return_type = PtrType::get(object_class);
        return;
    }
    const auto class_ =
        dynamic_cast<Class *>(scope.find_in_global(node.className));
    assert(class_);
    if (node.get_name() == "int") {
        visitor_return_type = IntegerType::get(32, module.get());
    } else if (node.get_name() == "bool") {
        visitor_return_type = IntegerType::get(1, module.get());
    } else if (node.get_name() == "str") {
        visitor_return_type = ptr_str_type;
    } else {
        visitor_return_type = PtrType::get(class_);
    }
}
void LightWalker::visit(parser::ExprStmt &node) { node.expr->accept(*this); }
void LightWalker::visit(parser::ForStmt &node) {
    node.iterable->accept(*this);
    auto list = visitor_return_value;

    auto cond_null = builder->create_icmp_eq(list, null);
    auto b_before_null = builder->get_insert_block();
    auto b_null_true =
        BasicBlock::create(module.get(), "", b_before_null->get_parent());
    auto b_null_false =
        BasicBlock::create(module.get(), "", b_before_null->get_parent());
    builder->create_cond_br(cond_null, b_null_true, b_null_false);
    builder->set_insert_point(b_null_true);
    builder->create_call(error_none_fun, vector<Value *>());
    builder->create_br(b_null_false);
    builder->set_insert_point(b_null_false);

    auto it = scope.find(node.identifier->name);
    // auto i = builder->create_alloca(IntegerType::get(32, module.get()));
    auto i_begin = CONST(0);
    // builder->create_store(CONST(0), i);
    auto v_len = builder->create_call(len_fun, {list});
    auto b = builder->get_insert_block();
    auto b_cond = BasicBlock::create(module.get(), "", b->get_parent());
    auto b_body = BasicBlock::create(module.get(), "", b->get_parent());
    auto b_end = BasicBlock::create(module.get(), "", b->get_parent());
    builder->create_br(b_cond);
    builder->set_insert_point(b_cond);
    // auto ii =builder->create_load(i);
    auto i = builder->create_phi(i32_type);
    i->set_lval(i_begin);
    i->add_phi_pair_operand(i_begin, b);
    b_cond->add_instr_begin(i);
    auto cond = builder->create_icmp_lt(i, v_len);
    builder->create_cond_br(cond, b_body, b_end);
    builder->set_insert_point(b_body);
    if (node.iterable->inferredType->get_name() == "str") {
        auto p_str = builder->create_gep(list, CONST(4));
        p_str->set_type(PtrType::get(ptr_i8_type));
        auto str = builder->create_load(p_str);
        auto p_char = builder->create_gep(str, i);
        auto i_char = builder->create_load(p_char);
        vector<Value *> makestr_para;
        makestr_para.push_back(i_char);
        auto t7 = builder->create_call(makestr_fun, makestr_para);

        builder->create_store(t7, it);

        for (const auto &s : node.body) {
            s->accept(*this);
        }
    } else {
        auto list_type = dynamic_cast<semantic::ListValueType *>(
            node.iterable->inferredType.get());
        if (list_type != nullptr) {
            auto element_type = list_type->element_type;
            auto p_actual_list = builder->create_gep(list, CONST(4));
            p_actual_list->set_type(PtrType::get(ptr_ptr_obj_type));
            auto actual_list = builder->create_load(p_actual_list);
            auto p_element = builder->create_gep(actual_list, i);
            Type *type = nullptr;
            if (element_type->get_name() == "int") {
                type = i32_type;
            } else if (element_type->get_name() == "bool") {
                type = i1_type;
            } else if (element_type->get_name() == "str") {
                type = ptr_str_type;
            } else {
                if (element_type->is_list_type()) {
                    type = PtrType::get(list_class);
                } else {
                    type = dynamic_cast<Type *>(
                        scope.find_in_global(element_type->get_name()));
                    assert(type != nullptr);
                    type = PtrType::get(type);
                }
            }
            auto t1 = builder->create_bitcast(p_element, PtrType::get(type));
            auto t2 = builder->create_load(t1);
            builder->create_store(t2, it);
            for (const auto &s : node.body) {
                s->accept(*this);
            }
        }
    }
    auto i_next = builder->create_iadd(i, CONST(1));
    i->add_phi_pair_operand(i_next, builder->get_insert_block());
    // builder->create_store(ni, i);
    builder->create_br(b_cond);
    builder->set_insert_point(b_end);
}
void LightWalker::visit(parser::FuncDef &node) {
    auto &func_name = node.get_id()->name;
    auto func_def_type = sym->declares<semantic::FunctionDefType>(func_name);
    assert(func_def_type);
    auto unique_func_name = get_fully_qualified_name(func_def_type);
    // std::cerr << "enter " << unique_func_name << std::endl;

    Function *func;
    Class *anon = nullptr;
    if (!scope.in_global()) {
        func = dynamic_cast<Function *>(scope.find(func_name));
        anon = dynamic_cast<Class *>(
            func->get_function_type()->get_arg_type(0)->get_ptr_element_type());
        assert(anon);
        assert(func);
    } else {
        auto is_method =
            unique_func_name.find("$$METHOD$$") != std::string::npos;
        func = dynamic_cast<Function *>(
            scope.find_in_global(is_method ? unique_func_name : func_name));
        assert(func);
    }
    auto &arg_types = func->get_function_type()->args_;

    auto saved_b = builder->get_insert_block();
    auto b = BasicBlock::create(module.get(), "", func);
    builder->set_insert_point(b);
    scope.enter();
    auto saved_sym = sym;
    sym = &func_def_type->current_scope;
    for (const auto &decl : node.declarations) {
        if (auto node = dynamic_cast<parser::FuncDef *>(decl.get()); node) {
            // lambda function
            auto &func_name = node->get_id()->name;
            auto func_def_type =
                sym->declares<semantic::FunctionDefType>(func_name);
            assert(func_def_type);
            auto unique_func_name = get_fully_qualified_name(func_def_type);

            auto func_type = dynamic_cast<FunctionType *>(
                semantic_type_to_llvm_type(func_def_type));
            assert(func_type);
            auto anon = new Class(module.get(), unique_func_name, true);
            func_type->args_.insert(func_type->args_.begin(),
                                    PtrType::get(anon));

            auto func =
                Function::create(func_type, unique_func_name, module.get());
            scope.push(func_name, func);

            auto class_instance = builder->create_alloca(anon);
            scope.push(func_name + "$anon", class_instance);
        }
    }

    if (anon) {
        builder->set_insert_point(saved_b);
        auto arg = new Value(arg_types[0], "arg0");
        auto class_instance = scope.find(func_name + "$anon");
        for (int i = 0; i < node.lambda_params.size(); i++) {
            auto &capture_name = node.lambda_params.at(i);
            auto capture_value = scope.find(capture_name);
            assert(capture_value);

            auto capture_type = capture_value->get_type();
            if (dynamic_cast<Function *>(capture_value)) {
                auto capture_func = (Function *)capture_value;
                auto capture_value = scope.find(capture_name + "$anon");
                assert(capture_value);
                capture_type = capture_value->get_type();
            }
            assert(capture_type);

            // std::cerr << unique_func_name << " add attr " << capture_name <<
            // " of type " << capture_type->print() << ' ' << std::endl;
            anon->add_attribute(new AttrInfo(capture_type, capture_name,
                                             ConstantNull::get(capture_type)));

            auto addr = builder->create_gep(class_instance, CONST(i));
            auto v = scope.find(capture_name);
            if (dynamic_cast<Function *>(v)) {
                v = scope.find(capture_name + "$anon");
            }
            builder->create_store(v, addr);
        }

        builder->set_insert_point(b);
        for (int i = 0; i < node.lambda_params.size(); i++) {
            auto &capture_name = node.lambda_params.at(i);
            auto t = builder->create_gep(arg, CONST(i));
            auto v = builder->create_load(t);
            if (v->get_type()->get_ptr_element_type()->is_class_anon()) {
                scope.push(capture_name + "$anon", v);
            } else {
                scope.push(capture_name, v);
            }
        }
    }

    for (int i = 0, arg_num = anon ? 1 : 0; i < node.params.size();
         i++, arg_num++) {
        auto arg_type = arg_types[arg_num];
        const auto &arg = node.params.at(i);
        auto alloca = builder->create_alloca(arg_type);
        builder->create_store(
            new Value(arg_type, "arg" + std::to_string(arg_num)), alloca);
        scope.push(arg->identifier->name, alloca);
    }

    for (const auto &decl : node.declarations) {
        if (auto node = dynamic_cast<parser::VarDef *>(decl.get()); node) {
            decl->accept(*this);
        }
    }
    for (const auto &decl : node.declarations) {
        if (auto node = dynamic_cast<parser::VarDef *>(decl.get()); node) {
        } else {
            decl->accept(*this);
        }
    }
    for (const auto &stmt : node.statements) {
        stmt->accept(*this);
    }

    if (node.returnType->get_name() != "int" &&
        node.returnType->get_name() != "bool" &&
        node.returnType->get_name() != "str") {
        builder->create_ret(
            new ConstantNull(func->get_function_type()->get_return_type()));
    }
    builder->set_insert_point(saved_b);
    scope.exit();
    sym = saved_sym;
}
void LightWalker::visit(parser::Ident &node) {
    // TODO: LightWalker for Ident
    // NOTE: List Or Class is not implemented.

    // Local var
    auto v = scope.find(node.name);
    if (v) {
        if (get_lvalue) {
            visitor_return_value = v;
        } else {
            auto t = builder->create_load(v);
            visitor_return_value = t;
        }
        return;
    }

    // Global var
    if (get_lvalue) {
        auto v = scope.find_in_global(node.name);
        visitor_return_value = v;
    } else {
        auto v = scope.find_in_global(node.name);
        auto t = builder->create_load(v);
        visitor_return_value = t;
    }
}
void LightWalker::visit(parser::IfExpr &node) {
    node.condition->accept(*this);
    auto cond = visitor_return_value;
    Instruction *res = nullptr;
    auto b = builder->get_insert_block();
    auto b_true = BasicBlock::create(module.get(), "", b->get_parent());
    auto b_false = BasicBlock::create(module.get(), "", b->get_parent());
    auto b_end = BasicBlock::create(module.get(), "", b->get_parent());
    builder->create_cond_br(cond, b_true, b_false);
    builder->set_insert_point(b_true);
    node.thenExpr->accept(*this);
    auto b_true_end = builder->get_insert_block();
    auto v_true = visitor_return_value;
    if (node.thenExpr->inferredType->get_name() == "int" &&
        node.inferredType->get_name() != "int") {
        v_true = builder->create_call(makeint_fun, {v_true});
    } else if (node.thenExpr->inferredType->get_name() == "bool" &&
               node.inferredType->get_name() != "bool") {
        v_true = builder->create_call(makebool_fun, {v_true});
    }
    builder->create_br(b_end);
    builder->set_insert_point(b_false);
    node.elseExpr->accept(*this);
    auto b_false_end = builder->get_insert_block();
    auto v_false = visitor_return_value;
    if (node.elseExpr->inferredType->get_name() == "int" &&
        node.inferredType->get_name() != "int") {
        v_false = builder->create_call(makeint_fun, {v_false});
    } else if (node.elseExpr->inferredType->get_name() == "bool" &&
               node.inferredType->get_name() != "bool") {
        v_false = builder->create_call(makebool_fun, {v_false});
    }
    builder->create_br(b_end);
    builder->set_insert_point(b_end);
    auto phi = builder->create_phi(v_true->get_type());
    phi->set_lval(v_true);
    phi->add_phi_pair_operand(v_true, b_true_end);
    phi->add_phi_pair_operand(v_false, b_false_end);
    b_end->add_instr_begin(phi);
    visitor_return_value = phi;
}
void LightWalker::visit(parser::IntegerLiteral &node) {
    auto C = CONST(node.value);
    visitor_return_value = C;
}
void LightWalker::visit(parser::ListExpr &node) {
    auto type =
        dynamic_cast<semantic::ListValueType *>(node.inferredType.get());
    vector<Value *> para;
    if (type == nullptr) {
        para.push_back(CONST(0));
        para.push_back(CONST(0));  // dummy arg, to make llvm happy
    } else {
        auto element_type = type->element_type;
        para.push_back(CONST(int(node.elements.size())));
        for (const auto &i : node.elements) {
            i->accept(*this);
            auto v = visitor_return_value;
            if (element_type->get_name() == "int") {
                para.push_back(v);
            } else if (element_type->get_name() == "bool") {
                auto v_32 =
                    builder->create_zext(v, IntegerType::get(32, module.get()));
                para.push_back(v_32);
            } else {
                if (i->inferredType->get_name() == "int") {
                    v = builder->create_call(makeint_fun, {v});
                } else if (i->inferredType->get_name() == "bool") {
                    v = builder->create_call(makebool_fun, {v});
                }
                v = builder->create_ptrtoint(v, module->get_int32_type());
                para.push_back(v);
            }
        }
    }
    Instruction *list = builder->create_call(construct_list_fun, para);
    list =
        builder->create_bitcast(list, PtrType::get(PtrType::get(list_class)));
    visitor_return_value = list;
}
void LightWalker::visit(parser::ListType &node) {
    visitor_return_type = PtrType::get(list_class);
}
void LightWalker::visit(parser::MemberExpr &node) {
    const auto param_get_lvalue = get_lvalue;
    get_lvalue = false, visitor_return_value = nullptr;
    node.object->accept(*this);
    auto obj = visitor_return_value;
    assert(obj);
    auto class_type = dynamic_cast<Class *>(
        scope.find_in_global(node.object->inferredType->get_name()));
    assert(class_type);

    auto is_none =
        builder->create_icmp_eq(obj, ConstantNull::get(obj->get_type()));
    auto b = builder->get_insert_block();
    auto b_true = BasicBlock::create(module.get(), "", b->get_parent());
    auto b_end = BasicBlock::create(module.get(), "", b->get_parent());
    builder->create_cond_br(is_none, b_true, b_end);
    builder->set_insert_point(b_true);
    builder->create_call(error_none_fun, {});
    builder->create_br(b_end);
    builder->set_insert_point(b_end);

    if (class_type->get_attr_offset(node.member->name) <
        class_type->get_attribute()->size()) {
        // member is a attribute
        auto v = builder->create_gep(
            obj, CONST(3 + class_type->get_attr_offset(node.member->name)));
        if (param_get_lvalue) {
            visitor_return_value = v;
        } else {
            auto t = builder->create_load(v);
            visitor_return_value = t;
        }
    } else {
        // member is a method
        auto dispatch_table_ptr = builder->create_gep(obj, CONST(2));
        auto dispatch_table = builder->create_load(dispatch_table_ptr);
        auto offset = class_type->get_method_offset(node.member->name);
        assert(offset < class_type->get_method()->size());
        auto func_ptr = builder->create_gep(dispatch_table, CONST(offset));
        auto func_type = class_type->get_method()->at(offset)->get_type();
        func_ptr->set_type(PtrType::get(PtrType::get(func_type)));
        auto func = builder->create_load(func_ptr);
        visitor_return_object = obj;
        visitor_return_value = func;
        visitor_return_type = func_type;
    }
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
    visitor_return_object = nullptr;
    visitor_return_value = nullptr;
    visitor_return_type = nullptr;
    node.method->accept(*this);
    assert(visitor_return_value);
    auto func = visitor_return_value;
    auto func_type = visitor_return_type;

    std::vector<Value *> args{visitor_return_object};
    for (const auto &arg : node.args) {
        visitor_return_value = nullptr;
        arg->accept(*this);
        assert(visitor_return_value);
        args.push_back(this->visitor_return_value);
    }
    visitor_return_value =
        builder->create_call(func, func_type, std::move(args));
}
void LightWalker::visit(parser::NoneLiteral &node) {
    visitor_return_value = null;
}
void LightWalker::visit(parser::ReturnStmt &node) {
    if (node.value == nullptr) {
        builder->create_ret(new ConstantNull(
            PtrType::get((Class *)scope.find_in_global("object"))));
        return;
    }
    visitor_return_value = nullptr;
    node.value->accept(*this);
    assert(visitor_return_value);
    builder->create_ret(visitor_return_value);
}
void LightWalker::visit(parser::StringLiteral &node) {
    int id = get_const_type_id();
    auto C = ConstantStr::get(node.value, id, module.get());
    auto t =
        GlobalVariable::create("const_" + std::to_string(id), module.get(), C);
    auto ptr_type = ptr_str_type;
    t->set_type(ptr_type);
    auto p =
        GlobalVariable::create("ptr_const_" + std::to_string(id), module.get(),
                               ptr_type, false, ConstantNull::get(ptr_type));
    // std::cerr<<p->get_type()->print()<<"\n";
    builder->create_store(t, p);
    auto p2 = builder->create_load(p);

    visitor_return_value = p2;
}
void LightWalker::visit(parser::UnaryExpr &node) {
    node.operand->accept(*this);
    auto v = visitor_return_value;
    Instruction *res = nullptr;
    if (node.operator_ == "-") {
        res = builder->create_ineg(v);
    } else if (node.operator_ == "not") {
        res = builder->create_not(v);
        res->set_type(i1_type);
    } else {
        assert(false);
    }
    visitor_return_value = res;
}
GlobalVariable *LightWalker::generate_init_object(parser::Literal *literal) {
    int const_id = get_const_type_id();
    string const_name = "const_" + std::to_string(const_id);
    if (auto s = dynamic_cast<parser::StringLiteral *>(literal); s) {
        auto g = GlobalVariable::create(
            const_name, module.get(),
            ConstantStr::get(s->value, const_id, module.get()));
        g->set_type(ptr_str_type);
        return g;
    } else if (auto i = dynamic_cast<parser::IntegerLiteral *>(literal); i) {
        auto g = GlobalVariable::create(
            const_name, module.get(),
            ConstantBoxInt::get(int_class, i->int_value, const_id));
        g->set_type(PtrType::get(int_class));
        return g;
    } else if (auto b = dynamic_cast<parser::BoolLiteral *>(literal); b) {
        auto g = GlobalVariable::create(
            const_name, module.get(),
            ConstantBoxBool::get(bool_class, b->bin_value, const_id));
        g->set_type(PtrType::get(bool_class));
        return g;
    } else {
        assert(0);
    }
}
void LightWalker::visit(parser::VarDef &node) {
    if (scope.in_global()) {
        GlobalVariable *t;
        if (node.var->type->get_name() == "int") {
            t = GlobalVariable::create(
                node.var->identifier->name, module.get(),
                IntegerType::get(32, module.get()), false,
                ConstantInt::get(node.value->int_value, module.get()));
        } else if (node.var->type->get_name() == "bool") {
            const auto b =
                dynamic_cast<parser::BoolLiteral *>(node.value.get());
            assert(b);
            t = GlobalVariable::create(
                node.var->identifier->name, module.get(),
                IntegerType::get(1, module.get()), false,
                ConstantInt::get(b->bin_value, module.get()));
        } else if (node.var->type->kind == "ClassType") {
            const auto &class_name = node.var->type->get_name();
            auto init_value_type_name = node.value->inferredType->get_name();
            const auto class_type =
                dynamic_cast<Class *>(scope.find_in_global(class_name));
            if (!class_type) {
                std::cerr << fmt::format("class {} not found", class_name)
                          << std::endl;
            }
            assert(class_type);
            const auto var_type = PtrType::get(class_type);
            const auto init_val = ConstantNull::get(var_type);
            t = GlobalVariable::create(node.var->identifier->name, module.get(),
                                       var_type, false, init_val);
            if (init_value_type_name == "<None>") {
                builder->create_store(null, t);
            } else if (init_value_type_name == "int" ||
                       init_value_type_name == "str" ||
                       init_value_type_name == "bool") {
                auto g = generate_init_object(node.value.get());
                builder->create_store(g, t);
                // 这里为什么用 store 而不是直接给 t
                // 一个初值呢？因为框架限制，ddl 到了没时间重构
            } else {
                assert(0);
            }
        } else if (dynamic_cast<parser::ListType *>(node.var->type.get())) {
            auto ptr_list_type = PtrType::get(list_class);
            t = GlobalVariable::create(node.var->identifier->name, module.get(),
                                       ptr_list_type, false,
                                       ConstantNull::get(ptr_list_type));
            builder->create_store(null, t);
        }
        scope.push_in_global(node.var->identifier->name, t);
    } else {
        Value *t;
        if (node.var->type->get_name() == "int") {
            t = builder->create_alloca(IntegerType::get(32, module.get()));
            builder->create_store(
                ConstantInt::get(node.value->int_value, module.get()), t);
        } else if (node.var->type->get_name() == "bool") {
            auto b = dynamic_cast<parser::BoolLiteral *>(node.value.get());
            assert(b);
            t = builder->create_alloca(IntegerType::get(1, module.get()));
            builder->create_store(ConstantInt::get(b->bin_value, module.get()),
                                  t);
        } else if (node.var->type->kind == "ClassType") {
            const auto &class_name = node.var->type->get_name();
            auto init_value_type_name = node.value->inferredType->get_name();
            const auto class_type =
                dynamic_cast<Class *>(scope.find_in_global(class_name));
            assert(class_type);
            const auto var_type = PtrType::get(class_type);
            t = builder->create_alloca(var_type);

            if (init_value_type_name == "<None>") {
                const auto init_val = ConstantNull::get(var_type);
                builder->create_store(init_val, t);
            } else if (init_value_type_name == "int" ||
                       init_value_type_name == "str" ||
                       init_value_type_name == "bool") {
                auto g = generate_init_object(node.value.get());
                builder->create_store(g, t);
            } else {
                assert(0);
            }
        } else if (auto list_type =
                       dynamic_cast<parser::ListType *>(node.var->type.get())) {
            auto ptr_list_type = PtrType::get(list_class);
            t = builder->create_alloca(ptr_list_type);
            builder->create_store(ConstantNull::get(ptr_list_type), t);
        } else {
            assert(0);
        }
        scope.push(node.var->identifier->name, t);
    }
}
void LightWalker::visit(parser::WhileStmt &node) {
    auto before = builder->get_insert_block();
    auto b_cond = BasicBlock::create(module.get(), "", before->get_parent());
    auto b_body = BasicBlock::create(module.get(), "", before->get_parent());
    auto b_end = BasicBlock::create(module.get(), "", before->get_parent());
    builder->create_br(b_cond);
    builder->set_insert_point(b_cond);
    node.condition->accept(*this);
    auto cond = visitor_return_value;
    builder->create_cond_br(cond, b_body, b_end);
    builder->set_insert_point(b_body);
    for (const auto &stmt : node.body) {
        stmt->accept(*this);
    }
    builder->create_br(b_cond);
    builder->set_insert_point(b_end);
}
void LightWalker::visit(parser::IndexExpr &node) {
    bool is_get_lvalue = get_lvalue;
    get_lvalue = false;
    node.list->accept(*this);
    auto list = visitor_return_value;
    node.index->accept(*this);
    auto idx = visitor_return_value;

    auto cond_null = builder->create_icmp_eq(list, null);
    auto b_before_null = builder->get_insert_block();
    auto b_null_true =
        BasicBlock::create(module.get(), "", b_before_null->get_parent());
    auto b_null_false =
        BasicBlock::create(module.get(), "", b_before_null->get_parent());
    builder->create_cond_br(cond_null, b_null_true, b_null_false);
    builder->set_insert_point(b_null_true);
    builder->create_call(error_none_fun, vector<Value *>());
    builder->create_br(b_null_false);
    builder->set_insert_point(b_null_false);

    auto v_len = builder->create_call(len_fun, {list});
    auto b = builder->get_insert_block();
    auto b_oob = BasicBlock::create(module.get(), "", b->get_parent());
    auto b_1 = BasicBlock::create(module.get(), "", b->get_parent());
    auto cond_1 = builder->create_icmp_ge(idx, v_len);
    auto cond_2 = builder->create_icmp_lt(idx, CONST(0));
    auto cond = builder->create_ior(cond_1, cond_2);
    cond->set_type(i1_type);
    builder->create_cond_br(cond, b_oob, b_1);
    builder->set_insert_point(b_oob);
    builder->create_call(error_oob_fun, vector<Value *>());
    builder->create_br(b_1);
    builder->set_insert_point(b_1);
    if (node.list->inferredType->get_name() == "str") {
        assert(!is_get_lvalue);
        auto p_str = builder->create_gep(list, CONST(4));
        p_str->set_type(PtrType::get(ptr_i8_type));
        auto str = builder->create_load(p_str);
        auto p_char = builder->create_gep(str, idx);
        auto i_char = builder->create_load(p_char);
        vector<Value *> makestr_para;
        makestr_para.push_back(i_char);
        auto t7 = builder->create_call(makestr_fun, makestr_para);
        visitor_return_value = t7;
    } else {
        Instruction *res = nullptr;
        if (node.inferredType->get_name() == "int") {
            auto t2 = builder->create_gep(list, CONST(4));
            t2->set_type(PtrType::get(ptr_ptr_obj_type));
            auto t3 = builder->create_load(t2);
            auto t5 = builder->create_gep(t3, idx);
            auto t6 = builder->create_bitcast(
                t5, PtrType::get(IntegerType::get(32, module.get())));
            res = t6;
        } else if (node.inferredType->get_name() == "bool") {
            auto t2 = builder->create_gep(list, CONST(4));
            t2->set_type(PtrType::get(ptr_ptr_obj_type));
            auto t3 = builder->create_load(t2);
            auto t5 = builder->create_gep(t3, idx);
            auto t6 = builder->create_bitcast(
                t5, PtrType::get(IntegerType::get(1, module.get())));
            res = t6;
        } else {
            auto t2 = builder->create_gep(list, CONST(4));
            t2->set_type(PtrType::get(ptr_ptr_obj_type));
            auto t3 = builder->create_load(t2);
            auto t5 = builder->create_gep(t3, idx);
            auto type_name = node.inferredType->get_name();
            Type *type = nullptr;
            if (type_name[0] == '[') {
                type = list_class->get_type();
            } else {
                type = dynamic_cast<Class *>(scope.find_in_global(type_name));
            }
            auto t6 =
                builder->create_bitcast(t5, PtrType::get(PtrType::get(type)));
            res = t6;
        }
        if (!is_get_lvalue) {
            res = builder->create_load(res);
        }
        get_lvalue = is_get_lvalue;
        visitor_return_value = res;
    }
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
        auto declarationAnalyzer = semantic::DeclarationAnalyzer(*tree);
        tree->accept(declarationAnalyzer);
    }
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

    if (assem || run) {
        auto generate_assem = fmt::format(
            "llc -opaque-pointers -O0 -verify-machineinstrs "
            "-mtriple=riscv32-unknown-elf "
            "-o {}.s {}.ll "
            R"(&& /usr/bin/sed -i 's/.*addrsig.*//g' )"
            "{}.s",
            target_path, target_path, target_path);
        int re_code = std::system(generate_assem.c_str());
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
