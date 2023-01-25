#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "chocopy_parse.hpp"

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
class PassStmt;
class IfStmt;
class Errors;
class WhileStmt;

}  // namespace parser

std::unique_ptr<parser::Program> parse(const char *input_path);

namespace ast {
class Visitor {
   public:
    virtual void visit(parser::AssignStmt &) = 0;
    virtual void visit(parser::Program &) = 0;
    virtual void visit(parser::PassStmt &) = 0;
    virtual void visit(parser::BinaryExpr &) = 0;
    virtual void visit(parser::BoolLiteral &) = 0;
    virtual void visit(parser::CallExpr &) = 0;
    virtual void visit(parser::ClassDef &) = 0;
    virtual void visit(parser::ClassType &) = 0;
    virtual void visit(parser::ExprStmt &) = 0;
    virtual void visit(parser::ForStmt &) = 0;
    virtual void visit(parser::FuncDef &) = 0;
    virtual void visit(parser::GlobalDecl &) = 0;
    virtual void visit(parser::Ident &) = 0;
    virtual void visit(parser::IfExpr &) = 0;
    virtual void visit(parser::IndexExpr &) = 0;
    virtual void visit(parser::IntegerLiteral &) = 0;
    virtual void visit(parser::ListExpr &) = 0;
    virtual void visit(parser::ListType &) = 0;
    virtual void visit(parser::MemberExpr &) = 0;
    virtual void visit(parser::IfStmt &node) = 0;
    virtual void visit(parser::MethodCallExpr &) = 0;
    virtual void visit(parser::NoneLiteral &) = 0;
    virtual void visit(parser::NonlocalDecl &) = 0;
    virtual void visit(parser::ReturnStmt &) = 0;
    virtual void visit(parser::StringLiteral &) = 0;
    virtual void visit(parser::TypeAnnotation &) = 0;
    virtual void visit(parser::TypedVar &) = 0;
    virtual void visit(parser::UnaryExpr &) = 0;
    virtual void visit(parser::VarDef &) = 0;
    virtual void visit(parser::WhileStmt &) = 0;
    virtual void visit(parser::Errors &) = 0;
    virtual void visit(parser::Node &) = 0;
};

}  // namespace ast
