#pragma once
#include <iostream>
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
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << L"(";
                        lhs->emit();
                        std::wcerr <<op.str;
                        rhs->emit();
                        std::wcerr << L")";
                }
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
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << op.str << L"(";
                        lhs->emit();
                        std::wcerr << L")";
                }
                virtual NodeTy type() override { return NodeTy::UnOp; }
        };
        class Variable: public virtual Expression{
                uint64_t _reg = 0;//since I want SSA - bytecode would be like - getvar %reg %name
                Token name;
        public:
                ~Variable() override { }
                virtual uint64_t reg()override{ return _reg; }
                Variable(const Token& name):name(name){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << name.str;
                }
                virtual NodeTy type() override { return NodeTy::Variable; }
        };
        class String: public virtual Expression{
                uint64_t _reg = 0;
                Token value;
        public:
                ~String() override { }
                virtual uint64_t reg()override{ return _reg; }
                String(const Token& value):value(value){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << L"\"" << value.str << L"\"";
                }
                virtual NodeTy type() override { return NodeTy::String; }
        };
        class Number: public virtual Expression{
                uint64_t _reg = 0;
                Token value;
        public:
                ~Number() override { }
                virtual uint64_t reg()override{ return _reg; }
                Number(const Token& value):value(value){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << value.str;
                }
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
                Expression* body;
                VecHelper<Variable>* args;
        public:
                ~Lambda() override { delete args; delete body; }
                virtual uint64_t reg()override{ return _reg; }
                Lambda(Expression * body, VecHelper<Variable> *args):body(body),args(args){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << L"lambda!( ";
                        for(auto&x:args->get()){
                                x->emit();
                                std::wcerr << L" ";
                        }
                        std::wcerr << L")->";
                        body->emit();
                }
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

        class Expr: public virtual Expression{
                uint64_t _reg = 0;
                Expression* expr;
        public:
                ~Expr() override { delete expr; }
                virtual uint64_t reg()override{ return _reg; }
                Expr(Expression* expr):expr(expr){}
                virtual ret_ty emit(inp_ty) override { expr->emit(); }
                virtual NodeTy type() override { return NodeTy::Expr; }
        };
        class AstNodeExpr: public virtual Expression{
                uint64_t _reg = 0;
                Expression* expr;
        public:
                ~AstNodeExpr() override {/* if(expr)expr->FullRelease(); */}
                virtual uint64_t reg()override{ return _reg; }
                AstNodeExpr(Expression* expr):expr(expr){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << L"expr!(";
                        expr->emit();
                        std::wcerr << L")";
                }
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
                ~AstNode() override { /*if(stmt)stmt->FullRelease();*/ }
                virtual uint64_t reg()override{ return _reg; }
                AstNode(Statement* stmt):stmt(stmt){}
                virtual ret_ty emit(inp_ty) override {                        
                        std::wcerr << L"stmt!(";
                        stmt->emit();
                        std::wcerr << L")";
                }
                virtual NodeTy type() override { return NodeTy::AstNode; }
                virtual void FullRelease() override { 
                        stmt->FullRelease(); 
                        delete this; 
                }
        };
        class Match: public virtual Expression{
                uint64_t _reg = 0;
                Expression* what;
                VecHelper<Expression> *cond;
                VecHelper<Expression> *res;
        public:
                ~Match() override { delete cond; delete res;what->FullRelease(); }
                virtual uint64_t reg()override{ return _reg; }
                Match(Expression*what, decltype(cond) cond, decltype(res) res):what(what),cond(cond),res(res){}
                virtual ret_ty emit(inp_ty) override {     
                        std::wcerr << L"match!("; what->emit(); std::wcerr << L"){\n";
                        for(auto i1 = cond->get().begin(), 
                                 i2 = res->get().begin(), 
                                 e1 = cond->get().end(), 
                                 e2 = res->get().end();
                                 i1!=e1 && i2!=e2; 
                                 ++i1,++i2){
                                std::wcerr << L"\t|"; (*i1)->emit(); std::wcerr << L"->"; (*i2)->emit();
                                std::wcerr << L'\n';
                        }
                        std::wcerr << L"}";
                }
                virtual NodeTy type() override { return NodeTy::Match; }
        };
}