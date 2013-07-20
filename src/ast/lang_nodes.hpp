#pragma once
#include "basic_node.hpp"
#include "../tokenizer/Token.hpp"
namespace Z{
        class BinOp: public virtual Expression{
                uint64_t _reg = 0;
                Token op;
                Expression * lhs, * rhs;
        public:
                ~BinOp() override { delete lhs; delete rhs; }
                virtual uint64_t reg()override{ return _reg; }
                BinOp(const Token& op, Expression * lhs, Expression * rhs):op(op),lhs(lhs),rhs(rhs){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::BinOp; }
        };
        class UnOp: public virtual Expression{
                uint64_t _reg = 0;
                Token op;
                Expression * lhs;
        public:
                ~UnOp() override { delete lhs; }
                virtual uint64_t reg()override{ return _reg; }
                UnOp(const Token& op, Expression * lhs):op(op),lhs(lhs){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::UnOp; }
        };
        class Variable: public virtual Expression{
                uint64_t _reg = 0;//since I want SSA - bytecode would be like - getvar %reg %name
                Token name;
        public:
                ~Variable() override { }
                virtual uint64_t reg()override{ return _reg; }
                Variable(const Token& name):name(name){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::Variable; }
        };
        class String: public virtual Expression{
                uint64_t _reg = 0;
                Token value;
        public:
                ~String() override { }
                virtual uint64_t reg()override{ return _reg; }
                String(const Token& value):value(value){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::String; }
        };
        class Number: public virtual Expression{
                uint64_t _reg = 0;
                Token value;
        public:
                ~Number() override { }
                virtual uint64_t reg()override{ return _reg; }
                Number(const Token& value):value(value){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::Number; }
        };
        class Oper: public virtual Expression{
                uint64_t _reg = 0;
                Token value;
        public:
                ~Oper() override { }
                virtual uint64_t reg()override{ return _reg; }
                Oper(const Token& value):value(value){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::Oper; }
        };
        class Lambda: public virtual Expression{
                uint64_t _reg = 0;
                Statement* body;
                VecHelper<Variable>* args;
        public:
                ~Lambda() override { delete args; delete body; }
                virtual uint64_t reg()override{ return _reg; }
                Lambda(Statement * body, VecHelper<Variable> *args):body(body),args(args){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::Lambda; }
        };
        class FCall: public virtual Expression{
                uint64_t _reg = 0;
                Expression* func;
                VecHelper<Expression>* args;
        public:
                ~FCall() override { delete args; delete func; }
                virtual uint64_t reg()override{ return _reg; }
                FCall(Expression * func, VecHelper<Expression> *args):func(func),args(args){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::FCall; }
        };
        class Tuple: public virtual Expression{
                uint64_t _reg = 0;
                VecHelper<Expression>* args;
        public:
                ~Tuple() override { delete args; }
                virtual uint64_t reg()override{ return _reg; }
                Tuple(VecHelper<Expression> *args):args(args){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::Tuple; }
        };

        class Expr: public virtual Expression{
                uint64_t _reg = 0;
                Expression* expr;
        public:
                ~Expr() override { delete expr; }
                virtual uint64_t reg()override{ return _reg; }
                Expr(Expression* expr):expr(expr){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::Expr; }
        };
        class AstNodeExpr: public virtual Expression{
                uint64_t _reg = 0;
                Expression* expr;
        public:
                ~AstNodeExpr() override { delete expr; }
                virtual uint64_t reg()override{ return _reg; }
                AstNodeExpr(Expression* expr):expr(expr){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::AstNodeExpr; }
                virtual void FullRelease() override { 
                        expr->FullRelease(); 
                        delete this; 
                }
        };
        class AstNode: public virtual Expression{
                uint64_t _reg = 0;
                Statement* stmt;
        public:
                ~AstNode() override { delete stmt; }
                virtual uint64_t reg()override{ return _reg; }
                AstNode(Statement* stmt):stmt(stmt){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::AstNode; }
                virtual void FullRelease() override { 
                        stmt->FullRelease(); 
                        delete this; 
                }
        };
}