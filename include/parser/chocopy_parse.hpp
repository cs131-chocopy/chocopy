//
// Created by yiwei yang on 2/15/21.
//
#ifndef CHOCOPY_COMPILER_PARSE_HPP
#define CHOCOPY_COMPILER_PARSE_HPP

#pragma once

#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

#include "SymbolTable.hpp"
#include "chocopy_ast.hpp"
#include "chocopy_logging.hpp"
#include "hierarchy_tree.hpp"

using namespace std;
using json = nlohmann::json;

namespace semantic {
class SymbolType;
class ClassValueType;
class ValueType;
}  // namespace semantic

namespace ast {
class Visitor;
}  // namespace ast

namespace parser {
class AssignStmt;
class BinaryExpr;
class BoolLiteral;
class CallExpr;
class ClassDef;
class ClassType;
class Decl;
class CompilerErr;
class Expr;
class ExprStmt;
class ForStmt;
class FuncDef;
class GlobalDecl;
class Ident;
class IfExpr;
class IndexExpr;
class IntegerLiteral;
class ListExpr;
class ListType;
class Literal;
class MemberExpr;
class MethodCallExpr;
class Node;
class NoneLiteral;
class NonlocalDecl;
class Program;
class ReturnStmt;
class Stmt;
class StringLiteral;
class TypeAnnotation;
class TypedVar;
class UnaryExpr;
class VarDef;
class WhileStmt;
class PassStmt;

struct LocationUnit {
    int line;
    int column;
    LocationUnit(int line = 0, int column = 0);
};
struct Location {
    LocationUnit first, last;
    Location();
    Location(LocationUnit first, LocationUnit last);
    Location(Location front, Location back);
};

/**
 * Root of the AST class hierarchy.  Every node has a left and right
 * location, indicating the start and end of the represented construct
 * in the source text.
 *
 * Every node can be marked with an error message, which serves two purposes:
 *   1. It indicates that an error message has been issued for this
 *      Node, allowing tne program to reduce cascades of error
 *      messages.
 *   2. It aids in debugging by making it convenient to see which
 *      Nodes have caused an error.
 */
class Node {
   public:
    string kind;
    string typeError;

    Location location;

    explicit Node(Location location) : location(location) {}
    Node(Location location, string kind)
        : location(location), kind(std::move(kind)) {}
    virtual ~Node() = default;

    virtual bool has_type_err() const { return !this->typeError.empty(); }
    virtual json toJSON() const;
    virtual void accept(ast::Visitor &visitor);
};

/**
 * Base of all AST nodes representing definitions or declarations.
 */
class Decl : public Node {
   public:
    /** A definition or declaration spanning source locations [LEFT..RIGHT]. */
    explicit Decl(Location location) : Node(location) {}
    Decl(Location location, string kind) : Node(location, std::move(kind)) {}

    virtual Ident *get_id() { return nullptr; };
};

/** Represents a single error.  Does not correspond to any Python source
 *  construct. */
class CompilerErr : public Node {
   public:
    string message;
    bool syntax = false;

    /** Represents an error with message MESSAGE.  Iff SYNTAX, it is a
     *  syntactic error.  The error applies to source text at [LEFT..RIGHT]. */
    CompilerErr(Location location, string message, bool syntax)
        : Node(location, "CompilerError"),
          message(std::move(message)),
          syntax(syntax){};

    CompilerErr(Location location, string message)
        : Node(location, "CompilerError"), message(std::move(message)) {}
    json toJSON() const override;
};

/** Collects the error messages in a Program.  There is exactly one per
 *  Program node. */
class Errors : public Node {
   public:
    /** The accumulated error messages in the order added. */
    vector<std::unique_ptr<CompilerErr>> compiler_errors;
    explicit Errors(Location location) : Node(location, "Errors") {}

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** An identifier with attached type annotation. */
class TypedVar : public Node {
   public:
    std::unique_ptr<Ident> identifier;
    std::unique_ptr<TypeAnnotation> type;

    /** The AST for
     *       IDENTIFIER : TYPE.
     *  spanning source locations [LEFT..RIGHT].
     */
    TypedVar(Location location, Ident *identifier, TypeAnnotation *type)
        : Node(location, "TypedVar"), identifier(identifier), type(type) {}

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

class Stmt : public Node {
   public:
    bool is_return = false;
    explicit Stmt(Location location) : Node(location) {}
    Stmt(Location location, string kind) : Node(location, std::move(kind)) {}
};

/**
 * Base of all AST nodes representing expressions.
 *
 * There is nothing in this class, but there will be many AST
 * node types that have fields that are *any expression*. For those
 * cases, having a field of this type will encompass all types of
 * expressions such as binary expressions and literals that subclass
 * this class.
 */
class Expr : public Node {
   public:
    /** A Python expression spanning source locations [LEFT..RIGHT]. */
    explicit Expr(Location location) : Node(location){};
    Expr(Location location, string kind) : Node(location, std::move(kind)){};

    json toJSON() const override;
    /**
     * The type of the value that this expression evaluates to.
     *
     * This field is always <tt>null</tt> after the parsing stage,
     * but is populated by the type checker in the semantic analysis
     * stage.
     *
     * After typechecking this field may be <tt>null</tt> only for
     * expressions that cannot be assigned a type. In particular,
     * {@link NoneLiteral} expressions will not have a typed assigned
     * to them.
     */
    std::shared_ptr<semantic::SymbolType> inferredType{};
};

/** A simple identifier. */
class Ident : public Expr {
   public:
    /** An AST for the variable, method, or parameter named NAME, spanning
     *  source locations [LEFT..RIGHT]. */
    Ident(Location location, string name) : Expr(location, "Identifier") {
        this->name = std::move(name);
    }

    string name;

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/**
 * Base of all the literal nodes.
 *
 * There is nothing in this class, but it is useful to isolate
 * expressions that are constant literals.
 */
class Literal : public Expr {
   public:
    /** A literal spanning source locations [LEFT..RIGHT]. */
    explicit Literal(Location location) : Expr(location){};
    Literal(Location location, string kind) : Expr(location, std::move(kind)){};
    Literal(Location location, string kind, int value)
        : Expr(location, std::move(kind)), int_value(value), is_init(true){};
    Literal(Location location, string kind, string value)
        : Expr(location, std::move(kind)),
          value(std::move(value)),
          is_init(true){};

    string value;
    int int_value{};
    bool is_init = false;
};

/**
 * Base of all AST nodes representing type annotations (list or class
 * types.
 */
class TypeAnnotation : public Node {
   public:
    /** An annotation spanning source locations [LEFT..RIGHT]. */
    explicit TypeAnnotation(Location location) : Node(location){};
    TypeAnnotation(Location location, string kind)
        : Node(location, std::move(kind)){};

    string get_name();
};

/** Single and multiple assignments. DEPRECATED from semantic */
class AssignStmt : public Stmt {
   public:
    vector<std::unique_ptr<Expr>> targets;
    std::unique_ptr<Expr> value;

    /** AST for TARGETS[0] = TARGETS[1] = ... = VALUE spanning source locations
     *  [LEFT..RIGHT].
     */
    AssignStmt(Location location, Expr *target, Expr *value)
        : Stmt(location, "AssignStmt"), value(value) {
        this->targets.emplace_back(target);
    }

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** <operand> <operator> <operand>. */
class BinaryExpr : public Expr {
   public:
    enum class operator_code {
        Plus = 0,
        Minus,
        Star,
        Slash,
        Percent,
        EqEq,
        NotEq,
        LessThan,
        MoreThan,
        More,
        Less,
        And,
        Or,
        Is,
        Unknown
    };

    std::unique_ptr<Expr> left;
    string operator_;
    std::unique_ptr<Expr> right;

    /** An AST for expressions of the form LEFTEXPR OP RIGHTEXPR
     *  from text in range [LEFTLOC..RIGHTLOC]. */
    BinaryExpr(Location location, Expr *leftExpr, string op, Expr *rightExpr)
        : Expr(location, "BinaryExpr"), left(leftExpr), right(rightExpr) {
        this->operator_ = std::move(op);
    }

    static operator_code hashcode(std::string const &str) {
        if (str == "+") return operator_code::Plus;
        if (str == "-") return operator_code::Minus;
        if (str == "*") return operator_code::Star;
        if (str == "//") return operator_code::Slash;
        if (str == "%") return operator_code::Percent;
        if (str == "==") return operator_code::EqEq;
        if (str == "!=") return operator_code::NotEq;
        if (str == "<=") return operator_code::LessThan;
        if (str == ">=") return operator_code::MoreThan;
        if (str == ">") return operator_code::More;
        if (str == "<") return operator_code::Less;
        if (str == "and") return operator_code::And;
        if (str == "or") return operator_code::Or;
        if (str == "is") return operator_code::Is;
        return operator_code::Unknown;
    }

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** Literals True or False. */
class BoolLiteral : public Literal {
   public:
    /** True iff I represent True. */
    bool bin_value;

    /** An AST for the token True or False at [LEFT..RIGHT], depending on
     *  VALUE. */
    BoolLiteral(Location location, bool value)
        : Literal(location, "BooleanLiteral") {
        this->bin_value = value;
    }

    json toJSON() const override;

    void accept(ast::Visitor &visitor) override;
};

/** A function call. */
class CallExpr : public Expr {
   public:
    std::unique_ptr<Ident> function;
    vector<std::unique_ptr<Expr>> args;

    CallExpr(Location location, Ident *function,
             vector<std::unique_ptr<Expr>> *args)
        : Expr(location, "CallExpr"), function(function) {
        this->args = std::move(*args);
        delete args;
    }

    CallExpr(Location location, Ident *function)
        : Expr(location, "CallExpr"), function(function) {}

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** A class definition. */
class ClassDef : public Decl {
   public:
    std::unique_ptr<Ident> name;
    std::unique_ptr<Ident> superClass;
    vector<std::unique_ptr<Decl>> declaration;

    /** A class definition. */
    ClassDef(Location location, Ident *name, Ident *superClass,
             vector<std::unique_ptr<Decl>> *declaration)
        : Decl(location, "ClassDef"), name(name), superClass(superClass) {
        this->declaration = std::move(*declaration);
        delete declaration;
    }

    Ident *get_id() override { return this->name.get(); }
    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** A simple class type name. */
class ClassType : public TypeAnnotation {
   public:
    string className;
    /** An AST denoting a type named CLASSNAME0 at [LEFT..RIGHT]. */
    ClassType(Location location, string className)
        : TypeAnnotation(location, "ClassType") {
        this->className = std::move(className);
    }

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** A statement containing only an expression. */
class ExprStmt : public Stmt {
   public:
    std::unique_ptr<Expr> expr;
    ExprStmt(Location location, Expr *expr)
        : Stmt(location, "ExprStmt"), expr(expr) {}

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** For statements. */
class ForStmt : public Stmt {
   public:
    std::unique_ptr<Ident> identifier;
    std::unique_ptr<Expr> iterable;
    vector<std::unique_ptr<parser::Stmt>> body;

    /** The AST for
     *      for IDENTIFIER in ITERABLE:
     *          BODY
     *  spanning source locations [LEFT..RIGHT].
     */
    ForStmt(Location location, Ident *identifier, Expr *iterable,
            vector<std::unique_ptr<parser::Stmt>> *body)
        : Stmt(location, "ForStmt"),
          identifier(identifier),
          iterable(iterable) {
        this->body = std::move(*body);
        delete body;
    }

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** Def statements. */
class FuncDef : public Decl {
   public:
    std::unique_ptr<Ident> name;
    vector<std::unique_ptr<TypedVar>> params;
    std::unique_ptr<TypeAnnotation> returnType;
    vector<std::unique_ptr<Decl>> declarations;
    vector<std::unique_ptr<parser::Stmt>> statements;
    vector<string> lambda_params;

    /** The AST for
     *     def NAME(PARAMS) -> RETURNTYPE:
     *         DECLARATIONS
     *         STATEMENTS
     *  spanning source locations [LEFT..RIGHT].
     */
    FuncDef(Location location, Ident *name,
            vector<std::unique_ptr<TypedVar>> *params,
            TypeAnnotation *returnType,
            vector<std::unique_ptr<Decl>> *declarations,
            vector<std::unique_ptr<parser::Stmt>> *statements)
        : Decl(location, "FuncDef"), name(name), returnType(returnType) {
        this->params = std::move(*params);
        delete params;
        this->declarations = std::move(*declarations);
        delete declarations;
        this->statements = std::move(*statements);
        delete statements;
    }
    Ident *get_id() override { return this->name.get(); }
    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

class GlobalDecl : public Decl {
   public:
    std::unique_ptr<Ident> variable;
    GlobalDecl(Location location, Ident *variable)
        : Decl(location, "GlobalDecl"), variable(variable) {}
    json toJSON() const override;

    Ident *get_id() override { return this->variable.get(); }
    void accept(ast::Visitor &visitor) override;
};

/** Conditional statement. */
class IfStmt : public Stmt {
   public:
    enum cond { THEN_ELSE = 0, THEN_ELIF, THEN };
    /** Test condition. */
    std::unique_ptr<Expr> condition;
    /** "True" branch. */
    vector<std::unique_ptr<parser::Stmt>> thenBody;
    /** "False" branch. */
    vector<std::unique_ptr<parser::Stmt>> elseBody;
    std::unique_ptr<IfStmt> elifBody;
    /** Bool manifest else or elif or int */
    char el = cond::THEN;
    /** The AST for
     *      if CONDITION:
     *          THENBODY
     *      else:
     *          ELSEBODY
     *  spanning source locations [LEFT..RIGHT].
     */
    IfStmt(Location location, Expr *condition,
           vector<std::unique_ptr<parser::Stmt>> *thenBody,
           vector<std::unique_ptr<parser::Stmt>> *elseBody)
        : Stmt(location, "IfStmt"), el(cond::THEN_ELSE), condition(condition) {
        this->thenBody = std::move(*thenBody);
        delete thenBody;
        this->elseBody = std::move(*elseBody);
        delete elseBody;
    }

    /** elseBody can be IfStmt to support polymorphism */
    IfStmt(Location location, Expr *condition,
           vector<std::unique_ptr<parser::Stmt>> *thenBody, IfStmt *elifBody)
        : Stmt(location, "IfStmt"),
          el(cond::THEN_ELIF),
          condition(condition),
          elifBody(elifBody) {
        this->thenBody = std::move(*thenBody);
        delete thenBody;
    }

    IfStmt(Location location, Expr *condition,
           vector<std::unique_ptr<parser::Stmt>> *thenBody)
        : Stmt(location, "IfStmt"), condition(condition) {
        this->thenBody = std::move(*thenBody);
        delete thenBody;
    }

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** Conditional expressions. */
class IfExpr : public Expr {
   public:
    /** Boolean condition. */
    std::unique_ptr<Expr> condition;
    /** True branch. */
    std::unique_ptr<Expr> thenExpr;
    /** False branch. */
    std::unique_ptr<Expr> elseExpr;

    /** The AST for
     *     THENEXPR if CONDITION else ELSEEXPR
     *  spanning source locations [LEFT..RIGHT].
     */
    IfExpr(Location location, Expr *condition, Expr *thenExpr, Expr *elseExpr)
        : Expr(location, "IfExpr"),
          condition(condition),
          thenExpr(thenExpr),
          elseExpr(elseExpr) {}

    json toJSON() const override;

    void accept(ast::Visitor &visitor) override;
};

/** List-indexing expression. */
class IndexExpr : public Expr {
   public:
    /** Indexed list. */
    std::unique_ptr<Expr> list;
    /** Expression for index value. */
    std::unique_ptr<Expr> index;

    /** The AST for
     *      LIST[INDEX].
     *  spanning source locations [LEFT..RIGHT].
     */
    IndexExpr(Location location, Expr *list, Expr *index)
        : Expr(location, "IndexExpr"), list(list), index(index) {}

    json toJSON() const override;

    void accept(ast::Visitor &visitor) override;
};

/** Integer numerals. */
class IntegerLiteral : public Literal {
   public:
    /** Value denoted. */
    int value;

    /** The AST for the literal VALUE, spanning source
     *  locations [LEFT..RIGHT]. */
    IntegerLiteral(Location location, int value)
        : Literal(location, "IntegerLiteral", value) {
        this->value = value;
    }

    json toJSON() const override;

    void accept(ast::Visitor &visitor) override;
};

/** List displays. */
class ListExpr : public Expr {
   public:
    /** List of element expressions. */
    vector<std::unique_ptr<Expr>> elements;

    /** The AST for
     *      [ ELEMENTS ].
     *  spanning source locations [LEFT..RIGHT].
     */
    ListExpr(Location location, vector<std::unique_ptr<Expr>> *elements)
        : Expr(location, "ListExpr") {
        this->elements = std::move(*elements);
        delete elements;
    }
    explicit ListExpr(Location location) : Expr(location, "ListExpr") {}

    json toJSON() const override;

    void accept(ast::Visitor &visitor) override;
};

/** Type denotation for a list type. */
class ListType : public TypeAnnotation {
   public:
    std::unique_ptr<TypeAnnotation> elementType;
    /** The AST for the type annotation
     *       [ ELEMENTTYPE ].
     *  spanning source locations [LEFT..RIGHT].
     */
    ListType(Location location, TypeAnnotation *element)
        : TypeAnnotation(location, "ListType"), elementType(element) {}

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** Attribute accessor. */
class MemberExpr : public Expr {
   public:
    std::unique_ptr<Expr> object;
    std::unique_ptr<Ident> member;
    /** The AST for
     *     OBJECT.MEMBER.
     *  spanning source locations [LEFT..RIGHT].
     */
    MemberExpr(Location location, Expr *object, Ident *member)
        : Expr(location, "MemberExpr"), object(object), member(member) {}

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;

    Ident *get_id() { return static_cast<Ident *>(object.get()); }

    bool is_function_call = false;
};

/** Method calls. */
class MethodCallExpr : public Expr {
   public:
    /** Expression for the bound method to be called. */
    std::unique_ptr<MemberExpr> method;
    /** Actual parameters. */
    vector<std::unique_ptr<Expr>> args;
    /** The AST for
     *      METHOD(ARGS).
     *  spanning source locations [LEFT..RIGHT].
     */
    MethodCallExpr(Location location, MemberExpr *method,
                   vector<std::unique_ptr<Expr>> *args)
        : Expr(location, "MethodCallExpr"), method(method) {
        this->args = std::move(*args);
        delete args;
    }
    /** The initializer for unintialised args*/
    MethodCallExpr(Location location, MemberExpr *method)
        : Expr(location, "MethodCallExpr"), method(method) {}

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** The expression 'None'. */
class NoneLiteral : public Literal {
   public:
    /** The AST for None, spanning source locations [LEFT..RIGHT]. */
    explicit NoneLiteral(Location location)
        : Literal(location, "NoneLiteral") {}

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** Nonlocal declaration. */
class NonlocalDecl : public Decl {
   public:
    std::unique_ptr<Ident> variable;
    /** The AST for
     *      nonlocal VARIABLE
     *  spanning source locations [LEFT..RIGHT].
     */
    NonlocalDecl(Location location, Ident *variable)
        : Decl(location, "NonLocalDecl"), variable(variable) {}

    Ident *get_id() override { return this->variable.get(); }
    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

class Program : public Node {
   public:
    vector<std::unique_ptr<parser::Stmt>> statements;
    std::unique_ptr<Errors> errors{new Errors({})};
    vector<std::unique_ptr<Decl>> declarations;

    // For semantic analysis
    semantic::SymbolTable symbol_table;
    semantic::HierachyTree hierachy_tree;

    Program(Location location, vector<std::unique_ptr<Decl>> *declarations,
            vector<std::unique_ptr<parser::Stmt>> *statements)
        : Node(location, "Program") {
        this->declarations = std::move(*declarations);
        delete declarations;
        this->statements = std::move(*statements);
        delete statements;
    };

    Program(Location location, vector<std::unique_ptr<Decl>> *declarations)
        : Node(location, "Program") {
        this->declarations = std::move(*declarations);
        delete declarations;
    };

    explicit Program(Location location) : Node(location, "Program"){};

    void add_error(vector<std::unique_ptr<CompilerErr>> *errs) {
        for (auto &err : *errs) {
            this->errors->compiler_errors.emplace_back(std::move(err));
        }
        errs->clear();
    }

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** Return from function. */
class ReturnStmt : public Stmt {
   public:
    /** Returned value. */
    std::unique_ptr<Expr> value;
    /** The AST for
     *     return VALUE
     *  spanning source locations [LEFT..RIGHT].
     */
    ReturnStmt(Location location, Expr *value)
        : Stmt(location, "ReturnStmt"), value(value) {
        is_return = true;
    }
    explicit ReturnStmt(Location location) : Stmt(location, "ReturnStmt") {
        is_return = true;
    }

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** String constants. */
class StringLiteral : public Literal {
   public:
    /** Contents of the literal, not including quotation marks. */
    string value;
    /** The AST for a string literal containing VALUE, spanning source
     *  locations [LEFT..RIGHT]. */
    StringLiteral(Location location, const string &value);
    json toJSON() const override;

    void accept(ast::Visitor &visitor) override;
};

/** An expression applying a unary operator. */
class UnaryExpr : public Expr {
   public:
    enum class operator_code { Minus = 0, Not, Unknown };

    string operator_;
    std::unique_ptr<Expr> operand;

    /** The AST for
     *      OPERATOR OPERAND
     *  spanning source locations [LEFT..RIGHT].
     */
    UnaryExpr(Location location, string operator_, Expr *operand)
        : Expr(location, "UnaryExpr"),
          operand(operand),
          operator_(std::move(operator_)) {}

    static operator_code hashcode(std::string const &str) {
        if (str == "-" || str == "MINUS:-") return operator_code::Minus;
        if (str == "not" || str == "NOT:not") return operator_code::Not;
        return operator_code::Unknown;
    }

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** A declaration of a variable (i.e., with type annotation). */
class VarDef : public Decl {
   public:
    /** The variable and its assigned type. */
    std::unique_ptr<TypedVar> var;
    /** The initial value assigned. */
    std::unique_ptr<Literal> value;

    /** The AST for
     *      VAR = VALUE
     *  where VAR has a type annotation, and spanning source
     *  locations [LEFT..RIGHT]. */
    VarDef(Location location, TypedVar *var, Literal *value)
        : Decl(location, "VarDef"), var(var), value(value) {}

    Ident *get_id() override { return var->identifier.get(); }
    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** Indefinite repetition construct. */
class WhileStmt : public Stmt {
   public:
    /** Test for whether to continue. */
    std::unique_ptr<Expr> condition;
    /** Loop body. */
    vector<std::unique_ptr<parser::Stmt>> body;

    /** The AST for
     *      while CONDITION:
     *          BODY
     *  spanning source locations [LEFT..RIGHT].
     */
    WhileStmt(Location location, Expr *condition,
              vector<std::unique_ptr<parser::Stmt>> *body)
        : Stmt(location, "WhileStmt"), condition(condition) {
        this->body = std::move(*body);
        delete body;
    }

    json toJSON() const override;
    void accept(ast::Visitor &visitor) override;
};

/** Indefinite repetition construct. */
class PassStmt : public Stmt, public Decl {
   public:
    /** The AST for
     *      PASS
     *  spanning source locations [LEFT..RIGHT].
     */
    explicit PassStmt(Location location)
        : Stmt(location, "PassStmt"), Decl(location, "PassStmt") {}
    void accept(ast::Visitor &visitor) override;
};
}  // namespace parser

#endif  // CHOCOPY_COMPILER_PARSE_HPP
