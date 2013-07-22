#pragma once
#include <iostream>
#include "basic_node.hpp"
#include "../tokenizer/Token.hpp"
#include <fstream>
#include <cstring>
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
                virtual Value eval(Context*ctx)override{
                        if(op.str == L"+")
                                return Value(lhs->eval(ctx).num + rhs->eval(ctx).num);
                        if(op.str == L"-")
                                return Value(lhs->eval(ctx).num - rhs->eval(ctx).num);
                        if(op.str == L"/")
                                return Value(lhs->eval(ctx).num / rhs->eval(ctx).num);
                        if(op.str == L"*")
                                return Value(lhs->eval(ctx).num * rhs->eval(ctx).num);
                        return ctx->null;
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
                virtual Value eval(Context*ctx)override{
                        if(op.str == L"+")
                                return lhs->eval(ctx);
                        return Value(-lhs->eval(ctx).num);
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
                virtual Value eval(Context*ctx)override{
                        return ctx->getVar(name.str);
                }
                virtual NodeTy type() override { return NodeTy::Variable; }
                std::wstring getname(){ return name.str; }
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
                virtual Value eval(Context*ctx)override{
                        return Value(&value.str);       
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
                virtual Value eval(Context*ctx)override{
                        return Value(wcstold(value.str.c_str(),nullptr));
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

                virtual Value eval(Context*ctx)override{
                        return Value(new Function(ctx,args,body));       
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
                virtual ret_ty emit(inp_ty) override {
                        func->emit();
                        std::wcerr << L'(' << L' ';
                        for(auto&x:args->get()){
                                x->emit();
                                std::wcerr << L' ';
                        }
                        std::wcerr << L')';
                }

                virtual Value eval(Context*ctx)override{
                        auto fun = func->eval(ctx);
                        if(fun.type!=ValType::Function){
                                return ctx->null;
                        }       
                        Context* _ctx = new Context(fun.fun->ctx);
                        auto& as = args->get();
                        auto& vs = fun.fun->args->get();
                        uint it = 0;
                        for(auto&x:vs){
                                _ctx->createVar(x->getname());
                                if(it<as.size()){
                                        _ctx->setVar(x->getname(),as[it]->eval(ctx));
                                }
                                ++it;
                        }
                        return fun.fun->body->eval(_ctx);
                }
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
                virtual Value eval(Context*ctx)override{
                        return expr->eval(ctx);
                }
                virtual NodeTy type() override { return NodeTy::Expr; }
        };
        class Import: public virtual Expression{
                uint64_t _reg = 0;
                Token module;
        public:
                ~Import() override {}
                virtual uint64_t reg()override{ return _reg; }
                Import(const Token& module):module(module){}
                virtual ret_ty emit(inp_ty) override { std::wcerr << L"import " << module.str; }
                virtual Value eval(Context*ctx)override{
                        char * name = new char[module.str.length()*4+10];
                        wcstombs(name, module.str.c_str(), module.str.length()*4+1);
                        strcpy(name+strlen(name),".z\0");
                        std::wifstream in (name);
                        delete [] name;
                        if(!in){
                                std::wcerr << L"Failed to load module " << module.str << L".z\n";
                                return ctx->null;
                        }
                        extern Expression* Parse(const std::wstring&);
                        std::wstring buf,tmp;
                        while(!in.eof()){
                                std::getline(in,tmp);
                                buf+= tmp + L"\n";
                        }
                        in.close();
                        auto expr = Parse(std::wstring(buf));
                        if(!expr){
                                return ctx->null;
                        }
                        return expr->eval(ctx);

                }
                virtual NodeTy type() override { return NodeTy::Import; }
        };
        class AstNodeExpr: public virtual Expression{
                uint64_t _reg = 0;
                Expression* expr;
        public:
                ~AstNodeExpr() override {}
                virtual uint64_t reg()override{ return _reg; }
                AstNodeExpr(Expression* expr):expr(expr){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << L"expr!(";
                        expr->emit();
                        std::wcerr << L")";
                }
                virtual Value eval(Context*ctx)override{
                        return Value(expr,ctx);
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
                ~AstNode() override { }
                virtual uint64_t reg()override{ return _reg; }
                AstNode(Statement* stmt):stmt(stmt){}
                virtual ret_ty emit(inp_ty) override {                        
                        std::wcerr << L"stmt!(";
                        stmt->emit();
                        std::wcerr << L")";
                }
                virtual Value eval(Context*ctx)override{
                        return Value(stmt,ctx);
                }
                virtual NodeTy type() override { return NodeTy::AstNode; }
                virtual void FullRelease() override { 
                        stmt->FullRelease(); 
                        delete this; 
                }
        };
        class EvalExpr: public virtual Expression{
                uint64_t _reg = 0;
                Expression* expr;
        public:
                ~EvalExpr() override { }
                virtual uint64_t reg()override{ return _reg; }
                EvalExpr(Expression* expr):expr(expr){}
                virtual ret_ty emit(inp_ty) override {                        
                        std::wcerr << L"eval!(";
                        expr->emit();
                        std::wcerr << L")";
                }
                virtual Value eval(Context*ctx)override{
                        auto hom = expr->eval(ctx);
                        if(hom.type==ValType::Expression){
                                return hom.expr->eval(hom.ctx);
                        }
                        if(hom.type==ValType::Statement){
                                return hom.stmt->eval(hom.ctx);
                        }
                        return hom;
                }
                virtual NodeTy type() override { return NodeTy::EvalExpr; }
                virtual void FullRelease() override { 
                        expr->FullRelease(); 
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
                virtual Value eval(Context*ctx)override{
                        auto pattern = what->eval(ctx);
                        std::wcerr << L"match to " << pattern.num << L'\n';
                        for(uint i=0;i<cond->get().size();++i){
                                auto pattern2 = cond->get()[i]->eval(ctx);
                                if(pattern2.num == pattern.num){
                                        return res->get()[i]->eval(ctx);
                                }
                        }
                        return ctx->null;
                }
                virtual NodeTy type() override { return NodeTy::Match; }
        };
        class Block: public virtual Expression{
                uint64_t _reg = 0;
                VecHelper<Expression>* block;
        public:
                ~Block() override { /*if(block)block->FullRelease();*/ }
                virtual uint64_t reg()override{ return _reg; }
                Block(VecHelper<Expression>* block):block(block){}
                virtual ret_ty emit(inp_ty) override {                        
                        std::wcerr << L"{\n";
                        for(auto&x:block->get()){
                                std::wcerr << L'\t';
                                x->emit();
                                std::wcerr << L'\n';
                        }
                        std::wcerr << L"}";
                }
                virtual Value eval(Context*ctx)override{
                        Value last;
                        for(auto&x:block->get()){
                                last = x->eval(ctx);
                        }
                        return last;
                }
                virtual NodeTy type() override { return NodeTy::Block; }
                virtual void FullRelease() override { 
                        block->FullRelease(); 
                        delete this; 
                }
        };
        class Let: public virtual Expression{
                uint64_t _reg = 0;
                Token name;
                Expression* value;
        public:
                ~Let() override { value->FullRelease(); }
                virtual uint64_t reg()override{ return _reg; }
                Let(const Token& name, Expression* value):name(name),value(value){}
                virtual ret_ty emit(inp_ty) override { 
                        std::wcerr << L"let "<< name.str << L" = "; value->emit();
                }
                virtual Value eval(Context*ctx)override{
                        ctx->createVar(name.str);
                        Value val;
                        ctx->setVar(name.str,val = value->eval(ctx));
                        return val;
                }
                virtual NodeTy type() override { return NodeTy::Let; }
        };
        class Var: public virtual Expression{
                uint64_t _reg = 0;
                Token name;
                Expression* value;
        public:
                ~Var() override { value->FullRelease(); }
                virtual uint64_t reg()override{ return _reg; }
                Var(const Token& name, Expression* value):name(name),value(value){}
                virtual ret_ty emit(inp_ty) override { 
                        std::wcerr << L"var "<< name.str << L" = "; if(value)value->emit();else std::wcerr << L"nil";
                }
                virtual Value eval(Context*ctx)override{
                        ctx->createVar(name.str);
                        if(!value){
                                return ctx->null;
                        }
                        Value val;
                        ctx->setVar(name.str,val = value->eval(ctx));
                        return val;
                }
                virtual NodeTy type() override { return NodeTy::Var; }
        };
}