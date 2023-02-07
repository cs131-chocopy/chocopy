#pragma once

#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "Class.hpp"
#include "Function.hpp"
#include "GlobalVariable.hpp"
#include "Type.hpp"
#include "Value.hpp"
#include "chocopy_logging.hpp"

using std::list;
using std::map;
using std::string;
namespace lightir {
class BasicBlock;
class Function;
class Instruction : public User {
   public:
    enum OpID {
        // Terminator Instructions
        Ret,
        Br,
        // Standard unary operators
        Neg,
        Not,
        // Standard binary operators
        Add,
        Sub,
        Mul,
        Div,
        Rem,
        And,
        Or,
        // Memory operators
        Alloca,
        Load,
        Store,
        // Shift operators
        Shl,   // <<
        AShr,  // arithmetic >>
        LShr,  // logical >>
        // Other operators
        ICmp,
        PHI,
        Call,
        GEP,
        ZExt,      // zero extend
        InElem,    // insert element
        ExElem,    // extract element
        BitCast,   // bitcast
        PtrToInt,  // ptrtoint
        Trunc,     // truncate
        VExt,      // VExt SIMD
        ASM
    };
    /** create instruction, auto insert to bb
     * ty here is result type */
    Instruction(Type *ty, OpID id, unsigned num_ops, BasicBlock *parent);
    Instruction(Type *ty, OpID id, unsigned num_ops);
    inline const BasicBlock *get_parent() const { return parent_; }
    inline BasicBlock *get_parent() { return parent_; }
    void set_parent(BasicBlock *parent) { this->parent_ = parent; }
    /** Return the function this instruction belongs to. */
    const Function *get_function() const;
    Function *get_function() {
        return const_cast<Function *>(
            static_cast<const Instruction *>(this)->get_function());
    }

    Module *get_module();

    OpID get_instr_type() { return op_id_; }

    bool is_void() {
        return ((op_id_ == Ret) || (op_id_ == Br) || (op_id_ == Store) ||
                (op_id_ == Call && this->get_type()->is_void_type()));
    }

    bool is_phi() { return op_id_ == PHI; }
    bool is_store() { return op_id_ == Store; }
    bool is_alloca() { return op_id_ == Alloca; }
    bool is_ret() { return op_id_ == Ret; }
    bool is_load() { return op_id_ == Load; }
    bool is_br() { return op_id_ == Br; }

    bool is_add() { return op_id_ == Add; }
    bool is_sub() { return op_id_ == Sub; }
    bool is_rem() { return op_id_ == Rem; }
    bool is_mul() { return op_id_ == Mul; }
    bool is_div() { return op_id_ == Div; }
    bool is_and() { return op_id_ == And; }
    bool is_or() { return op_id_ == Or; }

    bool is_cmp() { return op_id_ == ICmp; }

    bool is_call() { return op_id_ == Call; }
    bool is_gep() { return op_id_ == GEP; }
    bool is_zext() { return op_id_ == ZExt; }

    bool is_shl() { return op_id_ == Shl; }
    bool is_ashr() { return op_id_ == AShr; }
    bool is_lshr() { return op_id_ == LShr; }

    bool is_vext() { return op_id_ == VExt; }

    bool is_binary() {
        return (is_add() || is_sub() || is_mul() || is_div() || is_rem() ||
                is_shl() || is_ashr() || is_lshr() || is_and() || is_or()) &&
               (get_num_operand() == 2);
    }

    bool is_void_ret() {
        return ((op_id_ == Ret) || (op_id_ == Br) || (op_id_ == Store) ||
                (op_id_ == Call && this->get_type()->is_void_type()));
    }

    bool isTerminator() { return is_br() || is_ret(); }

    string print_comment() { return comment_; };

    void set_comment(std::string_view comment) { comment_ = comment; };

   private:
    BasicBlock *parent_;
    OpID op_id_;
    unsigned num_ops_;
    string comment_;
};

class BinaryInst : public Instruction {
   public:
    BinaryInst(Type *ty, OpID id, Value *v1, Value *v2, BasicBlock *bb);

   public:
    /** create Add instruction, auto insert to bb */
    static BinaryInst *create_add(Value *v1, Value *v2, BasicBlock *bb,
                                  Module *m);

    /** create Sub instruction, auto insert to bb */
    static BinaryInst *create_sub(Value *v1, Value *v2, BasicBlock *bb,
                                  Module *m);

    /** create mul instruction, auto insert to bb */
    static BinaryInst *create_mul(Value *v1, Value *v2, BasicBlock *bb,
                                  Module *m);

    /** create Div instruction, auto insert to bb */
    static BinaryInst *create_sdiv(Value *v1, Value *v2, BasicBlock *bb,
                                   Module *m);

    /** create mod instruction, auto insert to bb */
    static BinaryInst *create_rem(Value *v1, Value *v2, BasicBlock *bb,
                                  Module *m);

    /** create and instruction, auto insert to bb */
    static BinaryInst *create_and(Value *v1, Value *v2, BasicBlock *bb,
                                  Module *m);

    /** create or instruction, auto insert to bb */
    static BinaryInst *create_or(Value *v1, Value *v2, BasicBlock *bb,
                                 Module *m);

    string print() override;

   private:
    void assert_valid();
};

class UnaryInst : public Instruction {
   private:
    UnaryInst(Type *ty, OpID id, Value *v, BasicBlock *bb);

    UnaryInst(Type *ty, OpID id, BasicBlock *bb) : Instruction(ty, id, 1, bb) {}

   public:
    /** create neg instruction, auto insert to bb */
    static UnaryInst *create_neg(Value *v, BasicBlock *bb, Module *m);

    /** create not instruction, auto insert to bb */
    static UnaryInst *create_not(Value *v, BasicBlock *bb, Module *m);

    string print() override;
};

class CmpInst : public Instruction {
   public:
    enum CmpOp {
        EQ, /* == */
        NE, /* != */
        GT, /* >  */
        GE, /* >= */
        LT, /* <  */
        LE  /* <= */
    };

   private:
    CmpInst(Type *ty, CmpOp op, Value *lhs, Value *rhs, BasicBlock *bb);

   public:
    static CmpInst *create_cmp(CmpOp op, Value *lhs, Value *rhs, BasicBlock *bb,
                               Module *m);

    CmpOp get_cmp_op() { return cmp_op_; }

    string print() override;

   private:
    CmpOp cmp_op_;

    void assert_valid();
};

class CallInst : public Instruction {
   protected:
    CallInst(Function *func, vector<Value *> args, BasicBlock *bb);
    CallInst(Value *real_func, FunctionType *func, vector<Value *> args,
             BasicBlock *bb);

   public:
    static CallInst *create(Function *func, vector<Value *> args,
                            BasicBlock *bb);
    static CallInst *create(Value *real_func, FunctionType *func,
                            std::vector<Value *> args, BasicBlock *bb);
    FunctionType *get_function_type() const;

    string print() override;

   private:
    FunctionType *func_type_;
};

class BranchInst : public Instruction {
   private:
    BranchInst(Value *cond, BasicBlock *if_true, BasicBlock *if_false,
               BasicBlock *bb);
    BranchInst(BasicBlock *if_true, BasicBlock *bb);

   public:
    static BranchInst *create_cond_br(Value *cond, BasicBlock *if_true,
                                      BasicBlock *if_false, BasicBlock *bb);
    static BranchInst *create_br(BasicBlock *if_true, BasicBlock *bb);

    bool is_cond_br() const;
    bool is_cmp_br() const;

    string print() override;
};

class ReturnInst : public Instruction {
   private:
    ReturnInst(Value *val, BasicBlock *bb);
    explicit ReturnInst(BasicBlock *bb);

   public:
    static ReturnInst *create_ret(Value *val, BasicBlock *bb);
    static ReturnInst *create_void_ret(BasicBlock *bb);
    bool is_void_ret() const;

    string print() override;
};

class StoreInst : public Instruction {
   private:
    StoreInst(Value *val, Value *ptr, BasicBlock *bb);

   public:
    static StoreInst *create_store(Value *val, Value *ptr, BasicBlock *bb);

    Value *get_rval() { return this->get_operand(0); }
    Value *get_lval() { return this->get_operand(1); }

    string print() override;
};

class LoadInst : public Instruction {
   private:
    LoadInst(Type *ty, Value *ptr, BasicBlock *bb);

   public:
    static LoadInst *create_load(Type *ty, Value *ptr, BasicBlock *bb);
    Value *get_lval() { return this->get_operand(0); }

    Type *get_load_type() const;

    string print() override;
};

class AllocaInst : public Instruction {
   private:
    AllocaInst(Type *ty, BasicBlock *bb);

   public:
    static AllocaInst *create_alloca(Type *ty, BasicBlock *bb);

    Type *get_alloca_type() const;

    string print() override;

   private:
    Type *alloca_ty_;
};

class ZextInst : public Instruction {
   private:
    ZextInst(OpID op, Value *val, Type *ty, BasicBlock *bb);

   public:
    static ZextInst *create_zext(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const;

    string print() override;

   private:
    Type *dest_ty_;
};

class InsertElementInst : public Instruction {
   private:
    InsertElementInst(OpID op, Value *val, Type *ty, BasicBlock *bb);

   public:
    static InsertElementInst *create_insert_element(Value *val, Type *ty,
                                                    BasicBlock *bb);

    Type *get_dest_type() const;

    string print() override;

   private:
    Type *dest_ty_;
};

class ExtractElementInst : public Instruction {
   private:
    ExtractElementInst(OpID op, Value *val, Type *ty, BasicBlock *bb);

   public:
    static ExtractElementInst *create_extract_element(Value *val, Type *ty,
                                                      BasicBlock *bb);

    Type *get_dest_type() const;

    string print() override;

   private:
    Type *dest_ty_;
};

class BitCastInst : public Instruction {
   private:
    BitCastInst(OpID op, Value *val, Type *ty, BasicBlock *bb);

   public:
    static BitCastInst *create_bitcast(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const;

    string print() override;

   private:
    Type *dest_ty_;
};

class PtrToIntInst : public Instruction {
   private:
    PtrToIntInst(OpID op, Value *val, Type *ty, BasicBlock *bb);

   public:
    static PtrToIntInst *create_ptrtoint(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const;

    string print() override;

   private:
    Type *dest_ty_;
};

class TruncInst : public Instruction {
   private:
    TruncInst(OpID op, Value *val, Type *ty, BasicBlock *bb);

   public:
    static TruncInst *create_trunc(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const;

    string print() override;

   private:
    Type *dest_ty_;
};

class GetElementPtrInst : public Instruction {
   public:
    GetElementPtrInst(Value *ptr, Value *idx, BasicBlock *bb);
    GetElementPtrInst(Value *ptr, Value *idx);
    GetElementPtrInst(Type *ty, unsigned num_ops, BasicBlock *bb, Type *elem_ty)
        : Instruction(ty, Instruction::GEP, num_ops, bb),
          element_ty_(elem_ty) {}

    static Type *get_element_type(Value *ptr, Value *idx);
    static GetElementPtrInst *create_gep(Value *ptr, Value *idx,
                                         BasicBlock *bb);
    static GetElementPtrInst *create_gep(Value *ptr, Value *idx);
    Type *get_element_type() const;
    Value *get_idx() const;

    string print() override;

   private:
    Value *idx{};
    Type *element_ty_;
};

class PhiInst : public Instruction {
   private:
    PhiInst(vector<Value *> vals, vector<BasicBlock *> val_bbs, Type *ty,
            BasicBlock *bb);
    PhiInst(Type *ty, OpID op, unsigned num_ops, BasicBlock *bb)
        : Instruction(ty, op, num_ops, bb) {}
    Value *l_val_{};

   public:
    static PhiInst *create_phi(Type *ty, BasicBlock *bb);
    Value *get_lval() { return l_val_; }
    void set_lval(Value *l_val) { l_val_ = l_val; }
    void add_phi_pair_operand(Value *val, Value *pre_bb) {
        this->add_operand(val);
        this->add_operand(pre_bb);
    }
    string print() override;
};

class AsmInst : public Instruction {
    /** tail call void asm sideeffect " addi    s0, sp, 16\0Alui     a0, 2",
     * ""() */
   private:
    AsmInst(Module *m, string asm_str, BasicBlock *bb);
    string asm_str_;

   public:
    string print() override;
    string get_asm() { return asm_str_; };
    static AsmInst *create_asm(Module *m, const string &asm_str,
                               BasicBlock *bb);
};

class Function;
class GlobalVariable;
class Class;

class Module {
   public:
    explicit Module(string name);
    virtual ~Module();

    Type *get_void_type();
    Type *get_label_type();
    Type *get_class_type(int id_);
    IntegerType *get_int1_type();
    IntegerType *get_int32_type();
    PtrType *get_ptr_type(Type *contained);

    void add_function(Function *f);
    list<Function *> get_functions();
    void add_global_variable(GlobalVariable *g);
    list<GlobalVariable *> get_global_variable();
    void add_class(Class *c);
    list<Class *> get_class();
    string get_instr_op_name(Instruction::OpID instr) {
        return instr_id2string_[instr];
    }
    void set_print_name();
    void add_class_type(Type *);
    virtual string print();
    string module_name_;      /* Human-readable identifier for the module */
    string source_file_name_; /* Original source file name for module, for test
                                 and debug */
    int vectorize_num = 4;
    int thread_num = 4;
    bool is_declaration_ = false;

   private:
    list<GlobalVariable *>
        global_list_;                /* The Global Variables in the module */
    list<Function *> function_list_; /* The Functions in the module */
    list<Class *> class_list_;       /* The Functions in the module */
    map<Instruction::OpID, string>
        instr_id2string_; /* Instruction from opid to string */
    IntegerType *int1_ty_;
    IntegerType *int32_ty_;
    Type *label_ty_;
    map<int, Type *> obj_ty_;
    Type *void_ty_;
    map<pair<Type *, int>, PtrType *> ptr_map_;
};
}  // namespace lightir
