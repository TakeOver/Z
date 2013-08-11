#pragma once
#include <iostream>
#include "basic_node.hpp"
#include "../tokenizer/Token.hpp"
#include <fstream>
#include <cmath>
#include <cstring>
#include "../debug.hpp"
namespace Z{
                        
        namespace { 
                bool to_bool(Expression*);
                std::wstring typeof_str(NodeTy); 
                Expression* __newContextExpr(Context*);
        }

        class Number: public virtual Expression{
        public:
                double value;
                ~Number() override { }
                Number(double value):value(value){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << value;
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return this;
                }
                virtual NodeTy type() override { return NodeTy::Number; }
                void MarkChilds(std::set<Collectable*>&) override {}
        };


        class Variable: public virtual Expression{
        public:
                Token name;

                ~Variable() override { }

                Variable(const Token& name):name(name){}

                virtual ret_ty emit(inp_ty) override {
                        std::wcout << name.str;
                }

                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return ctx->getVar(name.str);
                }

                virtual NodeTy type() override { return NodeTy::Variable; }

                std::wstring getname(){ return name.str; }

                void MarkChilds(std::set<Collectable*>&marked) override {
                        marked.insert(this);
                }
        };

        class Array: public virtual Expression{
        public:
                VecHelper<Expression>* value;
                ~Array() override { }
                Array(decltype(value) value):value(value){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"[";
                        auto arr = &value->get();
                        for(int i= 0; i<((int)arr->size())-1;++i){
                                (*arr)[i]->emit();
                                std::wcout << L",";
                        }
                        if(arr->size()){
                                arr->back()->emit();
                        }
                        std::wcout << L"]";
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return this;       
                }
                virtual void FullRelease() override { 
                        delete value; 
                        delete this; 
                }
                Expression* get(Context* ctx, int64_t key){
                        auto arr = &value->get();
                        if(std::abs(key)>=arr->size()){
                                return ctx->nil;
                        }
                        if(key<0){
                                key = arr->size() - key;
                        }
                        return (*arr)[key];
                }

                Expression* toArray(Context* ctx) override { return this; }
                
                Expression* set(Context* ctx, int64_t key, Expression* _value){
                        auto arr = &value->get();
                        if(key<0){
                                key = arr->size() - key;
                        }
                        if(key<0){
                                return ctx->nil;
                        }
                        if((key)>=arr->size()){
                                arr->resize(std::abs(key)+1);
                        }
                        return (*arr)[key] = _value;
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        value->MarkChilds(marked);
                }
                virtual NodeTy type() override { return NodeTy::Array; }
        };

        class Boolean: public virtual Expression{
        public:
                bool value;
                ~Boolean() override { }
                Boolean(bool value):value(value){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << (value?L"true":L"false");
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return this;       
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                }
                virtual NodeTy type() override { return NodeTy::Boolean; }
        };

        class Function: public virtual Expression{
        public:
                Context* _ctx;
                Expression* body;
                VecHelper<Expression>* args;
                bool is_ellipsis;
                ~Function() override { }
                Function(Context* _ctx,Expression * body, VecHelper<Expression> *args, bool is_ellipsis = false):_ctx(_ctx),body(body),args(args),
                        is_ellipsis(is_ellipsis){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"((";
                        for(int i=0;i<((int)args->get().size())-1;++i){
                                args->get()[i]->emit();
                                std::wcout << L",";
                        }
                        if(args->get().size()){
                                args->get().back()->emit();
                        }
                        if(is_ellipsis)std::wcout << L"...";
                        std::wcout << L") -> ";
                        body->emit();
                        std::wcout << L")";
                }
                Expression* toArray(Context* ctx) override { 
                        return new Array(
                                new VecHelper<Expression>(
                                        {__newContextExpr(_ctx),
                                        new Array(args),body,
                                        new Boolean(is_ellipsis)})); }

                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        _ctx->MarkChilds(marked);
                        body->MarkChilds(marked);
                        args->MarkChilds(marked);
                }

                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return this;       
                }
                Expression* call(Context* ctx,std::vector<Expression*> valargs, bool eval_args = true){
                        if(eval_args){
                                for(auto&x:(valargs)){
                                        x = x->eval(ctx);
                                }
                        }
                        std::vector<Variable*> varargs;
                        auto __ctx = new Context(_ctx);
                        for(auto&x:args->get()){
                                if(x->type()!=NodeTy::Variable){
                                        std::wcout << L"Variable expected in function in args[runtime error]\n";
                                        return ctx->nil;
                                }
                                varargs.push_back(x->as<Variable>());
                        }
                        uint it = 0, lim = args->get().size() - is_ellipsis;
                        for(;it<lim;++it){
                                __ctx->createVar(varargs[it]->getname());
                                if(it<valargs.size()){
                                        __ctx->setVar(varargs[it]->getname(),valargs[it]);
                                }else{
                                        __ctx->setVar(varargs[it]->getname(),ctx->nil);
                                }
                        }
                        if(is_ellipsis){
                                std::vector<Expression*> *rest = new std::vector<Expression*>();
                                for(;it<valargs.size();++it){
                                        rest->push_back(valargs[it]);
                                }
                                __ctx->createVar(varargs[args->get().size()-1]->getname());
                                __ctx->setVar(varargs[args->get().size()-1]->getname(),new Array(new VecHelper<Expression>(rest)));
                        }
                        return body->eval(__ctx);

                }
                virtual NodeTy type() override { return NodeTy::Function; }
        };

        class Macro: public virtual Expression{
        public:
                Context* _ctx;
                Expression* body;
                VecHelper<Expression>* args;
                bool is_ellipsis;
                ~Macro() override { }
                Macro(Context* _ctx,Expression * body, VecHelper<Expression> *args, bool is_ellipsis=false):_ctx(_ctx),body(body),args(args),
                        is_ellipsis(is_ellipsis){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"(macro(";
                        for(int i=0;i<((int)args->get().size())-1;++i){
                                args->get()[i]->emit();
                                std::wcout << L",";
                        }
                        if(args->get().size()){
                                args->get().back()->emit();
                        }
                        if(is_ellipsis)std::wcout << L"...";
                        std::wcout << L") -> ";
                        body->emit();
                        std::wcout << L")";
                }

                Expression* toArray(Context* ctx) override { 
                        return new Array(
                                new VecHelper<Expression>(
                                        {__newContextExpr(_ctx),
                                        new Array(args),body,
                                        new Boolean(is_ellipsis)})); }


                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        _ctx->MarkChilds(marked);
                        body->MarkChilds(marked);
                        args->MarkChilds(marked);
                }

                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return this;       
                }
                Expression* call(Context* ctx,std::vector<Expression*> valargs){
                        auto __ctx = new Context(_ctx);
                        std::vector<Variable*> varargs;
                        for(auto&x:args->get()){
                                if(x->type()!=NodeTy::Variable){
                                        std::wcout << L"Variable expected in macro in args[runtime error]\n";
                                        return ctx->nil;
                                }
                                varargs.push_back(x->as<Variable>());
                        }
                        uint it = 0, lim = varargs.size() - is_ellipsis;
                        for(;it<lim;++it){
                                __ctx->createVar(varargs[it]->getname());
                                if(it<valargs.size()){
                                        __ctx->setVar(varargs[it]->getname(),valargs[it]);
                                }else{
                                        __ctx->setVar(varargs[it]->getname(),ctx->nil);
                                }
                        }
                        if(is_ellipsis){
                                std::vector<Expression*> *rest = new std::vector<Expression*>();
                                for(;it<valargs.size();++it){
                                        rest->push_back(valargs[it]);
                                }
                                __ctx->createVar(varargs.back()->getname());
                                __ctx->setVar(varargs.back()->getname(),new Array(new VecHelper<Expression>(rest)));
                        }
                        return body->eval(__ctx);

                }
                virtual NodeTy type() override { return NodeTy::Macro; }
        };

        class NativeFunction: public virtual Expression {
        public:
                native_fun_t func_ptr;
                ~NativeFunction() override { }
                NativeFunction(native_fun_t func_ptr):func_ptr(func_ptr){ }
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"Native.Functions[\"p@" << (intptr_t) func_ptr << L"\"]";
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return this;
                }
                virtual NodeTy type() override { return NodeTy::NativeFunction; }
                Expression* call(Context* ctx, const std::vector<Expression*>& args){
                        return func_ptr(ctx,args);
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                }
        };

        class String: public virtual Expression{
        public:
                std::wstring* value;
                ~String() override { 
                        delete value;
                }
                String(std::wstring* value):value(value){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"\"" << *value << L"\"";
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return this;       
                }
                virtual void FullRelease() override { 
                        delete value; 
                        delete this; 
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                }
                virtual NodeTy type() override { return NodeTy::String; }
        };

        class Hash: public virtual Expression{
        public:
                std::unordered_map<std::wstring, Expression*>* value;
                bool owned;
                ~Hash() override { 
                        if(owned){
                                delete value;     
                        }
                }
                Hash(decltype(value) value, bool owned = true):value(value),owned(owned){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"#{";
                        uint i = 0;
                        for(auto&x:*value){
                                ++i;
                                std::wcout <<L"\"" <<  x.first << L"\"=";
                                if(x.second==this){
                                        std::wcout << L" @self ";
                                }else{
                                        x.second->emit();
                                }
                                if(i != value->size()){
                                        std::wcout << L",";
                                }
                        }
                        std::wcout << L"}";
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return this;       
                }
                virtual void FullRelease() override { 
                        delete value; 
                        delete this; 
                }
                Expression* get(Context* ctx, const std::wstring & key){
                        // first priority to user declared getters/setters. ((s|g)et:<key>)
                        auto iter = value->find(L"get:"+key);
                        if(iter==value->end()){
                                // if no getter/setter found -> try to found <key>
                                iter = value->find(key);
                                if(iter!=value->end()){
                                        return iter->second;
                                }
                                // else try to found $system_getter, all objects have this.(base object)
                                iter = value->find(L"$system_getter");
                                if(iter==value->end()){
                                        return ctx->nil;
                                }
                                //$system_getter found;
                                auto getter = iter->second;
                                if(getter->type()==NodeTy::Function){
                                        return getter->as<Function>()->call(ctx,{this,new String(new std::wstring(key))});
                                }
                                if(getter->type()==NodeTy::NativeFunction){
                                        return getter->as<NativeFunction>()->call(ctx,{this,new String(new std::wstring(key))});
                                }
                                if(getter->type()==NodeTy::Macro){
                                        return getter->as<Macro>()->call(ctx,{this,new String(new std::wstring(key))});
                                }
                                return ctx->nil;
                        }
                        // get:<key> found
                        auto getter = iter->second;
                        if(getter->type()==NodeTy::Function){
                                return getter->as<Function>()->call(ctx,{this});
                        }
                        if(getter->type()==NodeTy::NativeFunction){
                                return getter->as<NativeFunction>()->call(ctx,{this});
                        }
                        if(getter->type()==NodeTy::Macro){
                                return getter->as<Macro>()->call(ctx,{this});
                        }
                        return ctx->nil;
                }
                Expression* toHash(Context* ctx) override { return this; }
                Expression* set(Context* ctx, const std::wstring & key, Expression* value){
                        auto iter = this->value->find(L"set:"+key);
                        if(iter==this->value->end()){
                                return (*this->value)[key] = value;
                        }
                        auto getter = iter->second;
                        if(getter->type()==NodeTy::Function){
                                return getter->as<Function>()->call(ctx,{this,value});
                        }
                        if(getter->type()==NodeTy::NativeFunction){
                                return getter->as<NativeFunction>()->call(ctx,{this,value});
                        }
                        if(getter->type()==NodeTy::Macro){
                                return getter->as<Macro>()->call(ctx,{this,value});
                        }
                        return value;
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        for(auto&x:*value){
                                if(contains(marked,x.second)){
                                        continue;
                                }
                                x.second->MarkChilds(marked);
                        }
                }
                virtual NodeTy type() override { return NodeTy::Hash; }
        };
        class ContextExpr: public virtual Expression{
        public:
                Context* value;
                ~ContextExpr() override { }
                ContextExpr(Context* value):value(value){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"@context";
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return this;
                }
                virtual NodeTy type() override { return NodeTy::ContextExpr; }
                void FullRelease() override {
                        delete value;
                        delete this;
                }             

                Expression* toHash(Context* ctx) override { return new Hash(value->env); }
                void MarkChilds(std::set<Collectable*>&) override {}
        };


        class BinOp: public virtual Expression{
        public:
                std::wstring op;
                Expression * lhs, * rhs;
                ~BinOp() override { }
                BinOp(const Token& op, Expression * lhs, Expression * rhs):op(L"binary@" + op.str),lhs(lhs),rhs(rhs){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"(";
                        lhs->emit();
                        for(int i = 7;i<op.length();++i){
                                std::wcout << (wchar_t)op[i];
                        }
                        rhs->emit();
                        if(op == L"binary@["){
                                std::wcout << L"]";
                        }
                        std::wcout << L")";
                }
                Expression* toArray(Context* ctx) override { 
                        return new Array(
                                new VecHelper<Expression>(
                                        {new String(new std::wstring(op)),lhs,rhs})); }

                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        auto builtin = ctx->findBuiltinOp(op);
                        DBG_TRACE("builtin:%li",(intptr_t)builtin);
                        if(builtin){
                                return builtin(ctx,{lhs,rhs});
                        }
                        auto fun = ctx->getVar(op);
                        if(fun->type()==NodeTy::NativeFunction){
                                return dynamic_cast<NativeFunction*>(fun)->call(ctx,{lhs,rhs});
                        }
                        if(fun->type() == NodeTy::Function){
                                return fun->as<Function>()->call(ctx,{lhs->eval(ctx),rhs->eval(ctx)},false);
                        }
                        if(fun->type() == NodeTy::Macro){
                                return fun->as<Macro>()->call(ctx,{lhs,rhs});
                        }

                        return ctx->nil;

                }
                void FullRelease()override{ 
                        lhs->FullRelease(); 
                        rhs->FullRelease();
                        delete this; 
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        lhs->MarkChilds(marked);
                        rhs->MarkChilds(marked);
                }
                virtual NodeTy type() override { return NodeTy::BinOp; }
        };

        class UnOp: public virtual Expression{
                std::wstring op;
                Expression * lhs;
        public:
                ~UnOp() override { }
                UnOp(const std::wstring& op, Expression * lhs):op(L"unary@"+op),lhs(lhs){}
                virtual ret_ty emit(inp_ty) override {                        
                        std::wcout << L"(";
                        for(int i =6;i<op.length();++i){
                                std::wcout << (wchar_t)op[i];
                        }
                        lhs->emit();
                        std::wcout << L")";
                }
                Expression* toArray(Context* ctx) override { 
                        return new Array(
                                new VecHelper<Expression>(
                                        {new String(new std::wstring(op)),lhs})); }

                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        auto builtin = ctx->findBuiltinOp(op);
                        if(builtin){
                                return builtin(ctx,{lhs});
                        }
                        auto fun = ctx->getVar(op);
                        if(fun->type()==NodeTy::NativeFunction){
                                return fun->as<NativeFunction>()->call(ctx,{lhs});
                        }
                        if(fun->type()==NodeTy::Function){
                                return fun->as<Function>()->call(ctx,{lhs});
                        }
                        if(fun->type() == NodeTy::Macro){
                                return fun->as<Macro>()->call(ctx,{lhs});
                        }
                        return ctx->nil;
                }
                void FullRelease()override{ 
                        lhs->FullRelease(); 
                        delete this; 
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        lhs->MarkChilds(marked);
                }
                virtual NodeTy type() override { return NodeTy::UnOp; }
        };

        class Nil: public virtual Expression{
        public:
                ~Nil() override { }
                Nil(){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << (L"nil");
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return this;       
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                }
                virtual NodeTy type() override { return NodeTy::Nil; }
        };

        class Delete: public virtual Expression{
                Expression* what;
        public:
                ~Delete() override { }
                Delete(Expression*what):what(what){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << (L"delete"); 
                        what->emit();
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        switch(what->type()){
                                case NodeTy::BinOp: {
                                        auto bop = dynamic_cast<BinOp*>(what);
                                        if(bop->op == L"binary@." || bop->op == L"binary@["){
                                                auto obj = bop->lhs->eval(ctx), 
                                                        key = bop->rhs->eval(ctx);
                                                if(obj->type()!=NodeTy::Hash){
                                                        goto otherwise;
                                                }
                                                if(key->type()!=NodeTy::String){
                                                        return obj;
                                                }
                                                dynamic_cast<Hash*>(obj)->value->erase(*dynamic_cast<String*>(key)->value);
                                                return obj;
                                        }
                                        goto otherwise;
                                }
                                case NodeTy::Variable: {
                                        auto var = dynamic_cast<Variable*>(what);
                                        ctx->deleteVar(var->getname());
                                        return ctx->nil;
                                }
                                default: goto otherwise;
                        }
                        otherwise:
                        return ctx->nil;
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        what->MarkChilds(marked);
                }
                void FullRelease()override{ 
                        what->FullRelease(); 
                        delete this; 
                }
                Expression* toArray(Context* ctx) override { 
                        return new Array(
                                new VecHelper<Expression>(
                                        {what})); }

                virtual NodeTy type() override { return NodeTy::Delete; }
        };

        class Ellipsis: public virtual Expression{
                Expression* what;
        public:
                ~Ellipsis() override { }
                Ellipsis(Expression * what):what(what){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << (L"..."); what->emit();
                }
                Expression* toArray(Context* ctx) override { 
                        return new Array(
                                new VecHelper<Expression>(
                                        {what})); }

                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        auto ell = what->eval(ctx);
                        if(ell->type()!=NodeTy::Array){
                                return new Array(new VecHelper<Expression>(new std::vector<Expression*>()));
                        }
                        return ell;
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        what->MarkChilds(marked);
                }
                void FullRelease()override{ 
                        what->FullRelease(); 
                        delete this; 
                }
                virtual NodeTy type() override { return NodeTy::Ellipsis; }
        };

        class For: public virtual Expression{
                Variable * var;
                Expression *from, *to,*body;

        public:
                ~For() override { }
                For(decltype(var)var,decltype(from)from,decltype(to)to,decltype(body) body):var(var),from(from),to(to),body(body){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << (L"for ") << var->getname() << L'=';
                        from->emit(); std::wcout << L" to "; to->emit(); std::wcout << L' ';
                        body->emit();
                }
                Expression* toArray(Context* ctx) override { 
                        return new Array(
                                new VecHelper<Expression>(
                                        {var,from,to,body})); }

                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        Context* _ctx = new Context(ctx);
                        Number* _from =  dynamic_cast<Number*>(from->eval(ctx));
                        if(!_from){
                                return ctx->nil;
                        }
                        _ctx->createVar(var->getname());
                        _ctx->setVar(var->getname(),_from);
                        Number* _to = dynamic_cast<Number*>(to->eval(ctx));
                        if(!_to){
                                return ctx->nil;
                        }
                        auto step = 1.0;
                        if(_from->value > _to->value){
                                step=-step;
                        }
                        Expression* res;
                        auto less_eq = ctx->findBuiltinOp(L"binary@<=");
                        if(!less_eq){
                                return ctx->nil;
                        }
                        while(to_bool(less_eq(ctx,{_ctx->getVar(var->getname()),_to}))){
                                res = body->eval(_ctx);
                                auto _var = dynamic_cast<Number*>(_ctx->getVar(var->getname()));
                                if(!_var){
                                        return ctx->nil;
                                }
                                _ctx->setVar(var->getname(),new Number(_var->value + step));
                        }
                        return res;


                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        from->MarkChilds(marked);
                        to->MarkChilds(marked);
                        body->MarkChilds(marked);
                }
                void FullRelease()override{ 
                        body->FullRelease(); 
                        from->FullRelease(); 
                        to->FullRelease(); 
                        delete this; 
                        delete var; 
                }
                virtual NodeTy type() override { return NodeTy::For; }
        };

        class While: public virtual Expression{
                Expression *cond, 
                           *body;
        public:
                ~While() override { }
                While(decltype(cond) cond,decltype(body) body):cond(cond),body(body){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << (L"while("); cond->emit(); std::wcout << L")";
                        body->emit();
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        Expression* res;
                        auto _ctx = new Context(ctx);
                        while(to_bool(cond->eval(ctx))){
                                res = body->eval(_ctx);
                        }
                        /*_ctx->release();*/
                        return res;
                }
                Expression* toArray(Context* ctx) override { 
                        return new Array(
                                new VecHelper<Expression>(
                                        {cond,body})); }

                void FullRelease()override{ 
                        body->FullRelease(); 
                        cond->FullRelease();
                        delete this; 
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        cond->MarkChilds(marked);
                        body->MarkChilds(marked);
                }
                virtual NodeTy type() override { return NodeTy::While; }
        };

        class ArrayAst: public virtual Expression{
        public:
                VecHelper<Expression>* arr;
                ~ArrayAst() override { }
                ArrayAst(decltype(arr) arr):arr(arr){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << (L"[");
                        uint i = 0;
                        for(auto&x:arr->get()){
                                ++i;
                                x->emit();
                                if(i != arr->get().size()){
                                        std::wcout << L',';
                                }
                        }
                        std::wcout << L']';
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        std::vector<Expression*>* res = new std::vector<Expression*>();
                        res->resize(arr->get().size());
                        uint it = 0;
                        for(auto&x:arr->get()){
                                (*res)[it] = x->eval(ctx);
                                ++it;
                        }
                        return new Array(new VecHelper<Expression>(res));
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        arr->MarkChilds(marked);
                }
                Expression* toArray(Context* ctx) override { 
                        return new Array(arr); }

                void FullRelease() override { 
                        arr->FullRelease(); 
                        delete this; 
                }
                virtual NodeTy type() override { return NodeTy::ArrayAst; }
        };

        class HashAst: public virtual Expression{
        public:
                VecHelper<Expression>* arr;
                VecHelper<Expression>* keys;
                ~HashAst() override { }
                HashAst(decltype(arr) arr,decltype(keys)keys):arr(arr),keys(keys){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << (L"#{");
                        uint i = 0;
                        for(auto&x:arr->get()){
                                keys->get()[i]->emit();
                                ++i;
                                std::wcout << L'=';
                                x->emit();
                                if(i != arr->get().size()){
                                        std::wcout << L',';
                                }
                        }
                        std::wcout << L"}";
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        std::unordered_map<std::wstring,Expression*>* res = new std::unordered_map<std::wstring,Expression*>();
                        uint it = 0;
                        for(auto&x:arr->get()){
                                auto keyval = dynamic_cast<String*>(keys->get()[it]->eval(ctx));
                                if(keyval){
                                        (*res)[*keyval->value] = x->eval(ctx);
                                }
                                ++it;
                        }
                        return new Hash(res);
                }
                void FullRelease()override{ 
                        arr->FullRelease(); 
                        keys->FullRelease();
                        delete this; 
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        arr->MarkChilds(marked);
                        keys->MarkChilds(marked);
                }
                Expression* toArray(Context* ctx) override { 
                        return new Array(
                                new VecHelper<Expression>(
                                        {new Array(keys), new Array(arr)})); }

                virtual NodeTy type() override { return NodeTy::HashAst; }
        };

        class Export: public virtual Expression{
                Token variable;
        public:
                ~Export() override { }
                Export(const Token& variable):variable(variable){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << (L"export ") << variable.str;
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        auto val = ctx->getVar(variable.str);
                        ctx = ctx->getRoot();
                        ctx->createVar(variable.str);
                        ctx->setVar(variable.str, val);
                        return val;
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                }
                virtual NodeTy type() override { return NodeTy::Export; }
        };

        class Lambda: public virtual Expression{
                Expression* body;
                VecHelper<Expression>* args;
                bool is_ellipsis;
        public:
                ~Lambda() override { }
                Lambda(Expression * body, VecHelper<Expression> *args, bool is_ellipsis=false):body(body),args(args),
                        is_ellipsis(is_ellipsis){}
                virtual ret_ty emit(inp_ty) override {
                        Function(nullptr,body,args,is_ellipsis).emit();
                }

                void FullRelease()override{ 
                        args->FullRelease(); 
                        body->FullRelease();
                        delete this; 
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        //ctx->addRef();
                        return new Function(ctx,body,args,is_ellipsis);       
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        body->MarkChilds(marked);
                        args->MarkChilds(marked);
                }
                Expression* toArray(Context* ctx) override { 
                        return new Array(
                                new VecHelper<Expression>(
                                        {new Array(args),body})); }

                virtual NodeTy type() override { return NodeTy::Lambda; }
        };
        class MacroAst: public virtual Expression{
                Expression* body;
                VecHelper<Expression>* args;
                bool is_ellipsis;
        public:
                ~MacroAst() override { }
                MacroAst(Expression * body, VecHelper<Expression> *args, bool is_ellipsis=false):body(body),args(args),
                        is_ellipsis(is_ellipsis){}
                virtual ret_ty emit(inp_ty) override {
                        Function(nullptr,body,args,is_ellipsis).emit();
                }

                void FullRelease()override{ 
                        args->FullRelease(); 
                        body->FullRelease();
                        delete this; 
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        //ctx->addRef();
                        return new Macro(ctx,body,args,is_ellipsis);       
                }
                Expression* toArray(Context* ctx) override { 
                        return new Array(
                                new VecHelper<Expression>(
                                        {new Array(args),body})); }

                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        body->MarkChilds(marked);
                        args->MarkChilds(marked);
                }
                virtual NodeTy type() override { return NodeTy::MacroAst; }
        };

        
        class AstNode: public virtual Expression{
        public:
                Expression* expr;
                Context* _ctx;
                ~AstNode() override { /*_ctx->release();*/ }
                AstNode(Expression* expr,Context * _ctx):expr(expr),_ctx(_ctx){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"${";
                        expr->emit();
                        std::wcout << L"}";
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return this;
                }
                virtual NodeTy type() override { return NodeTy::AstNode; }
                virtual void FullRelease() override { 
                        expr->FullRelease(); 
                        delete this; 
                }
                Expression* toArray(Context* ctx) override { 
                        /*return new Array(
                                new VecHelper<Expression>(
                                        {expr,new ContextExpr(_ctx)}));*/
                        return expr->toArray(ctx);
                }

                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        _ctx->MarkChilds(marked);
                        expr->MarkChilds(marked);
                }
        };

        class AstNodeExpr: public virtual Expression{
                Expression* expr;
        public:
                ~AstNodeExpr() override { }
                AstNodeExpr(Expression* expr):expr(expr){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"${";
                        expr->emit();
                        std::wcout << L"}";
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        //ctx->addRef();
                        return new AstNode(expr,ctx);
                }
                virtual NodeTy type() override { return NodeTy::AstNodeExpr; }
                virtual void FullRelease() override { 
                        expr->FullRelease(); 
                        delete this; 
                }
                Expression* toArray(Context* ctx) override { 
                        return new Array(
                                new VecHelper<Expression>(
                                        {expr})); }

                void MarkChilds(std::set<Collectable*>& marked) override {
                        expr->MarkChilds(marked);
                        marked.insert(this);
                }
        };
        class FCall: public virtual Expression{
                Expression* func;
                VecHelper<Expression>* args;
        public:
                ~FCall() override { }
                FCall(Expression * func, VecHelper<Expression> *args):func(func),args(args){}
                virtual ret_ty emit(inp_ty) override {
                        func->emit();
                        std::wcout << L'(';
                        for(uint i = 0; i< args->get().size(); ++ i ){
                                args->get()[i]->emit();
                                if(i+1!=args->get().size()){
                                        std::wcout << L',';
                                }
                        }
                        std::wcout << L')';
                }

                Expression* toArray(Context* ctx) override { 
                        return new Array(new VecHelper<Expression>(new std::vector<Expression*>({new Array(args),func})));
                }

                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        Expression* fun;
                        std::vector<Expression*> _args;
                        BinOp *obj = func->as<BinOp>();
                        if(obj && (obj->op == L"binary@.")){
                                DBG_TRACE("object found");
                                auto    _obj = obj->lhs->eval(ctx), 
                                        _key = obj->rhs->eval(ctx), __obj = _obj;
                                if(_obj->type()!=NodeTy::Hash){
                                        __obj = ctx->getVar(typeof_str(_obj->type()));
                                        if(__obj->type()!=NodeTy::Hash){
                                                return ctx->nil;
                                        }
                                }
                                fun = __obj->as<Hash>()->get(ctx,*_key->as<String>()->value);
                                _args.push_back(_obj);
                        }else{
                                fun = func->eval(ctx);
                        }
                        for(auto&x:args->get()){
                                if(dynamic_cast<Ellipsis*>(x)!=nullptr){
                                        auto ell = x->eval(ctx);
                                        for(auto&y:dynamic_cast<Array*>(ell)->value->get()){
                                                _args.push_back(y);
                                        }
                                }else{
                                        if ( fun->type() == NodeTy::Macro ) {
                                                _args.push_back( new AstNode(x,ctx) );
                                        }else if ( fun->type() == NodeTy::NativeFunction ) {
                                                _args.push_back( x );
                                        } else {
                                                _args.push_back( x->eval( ctx ) );
                                        }
                                }
                        }
                        if(fun->type() == NodeTy::NativeFunction){
                                return fun->as<NativeFunction>()->call(ctx,_args);
                        }
                        if(fun->type()==NodeTy::Function){
                                return fun->as<Function>()->call(ctx, _args,false);
                        }       
                        if(fun->type()==NodeTy::Macro){
                                return fun->as<Macro>()->call(ctx, _args);
                        }       
                        return ctx->nil;
                }
                virtual NodeTy type() override { return NodeTy::FCall; }
                void FullRelease()override{  
                        args->FullRelease(); 
                        func->FullRelease(); 
                        delete this; 
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        func->MarkChilds(marked);
                        args->MarkChilds(marked);
                }
        };

        class Import: public virtual Expression{
                Token module;
                bool ret_mod;
        public:
                ~Import() override {}
                Import(const Token& module,bool ret_mod = false):module(module),ret_mod(ret_mod){}
                virtual ret_ty emit(inp_ty) override { std::wcout << L"import " << module.str; }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        if(ctx->isImported(module.str) && !ret_mod){
                                return ctx->moduleValue(module.str);
                        }
                        char * name = new char[module.str.length()*4+10];
                        wcstombs(name, module.str.c_str(), module.str.length()*4+1);
                        strcpy(name+strlen(name),".z\0");
                        std::wifstream in (name);
                        delete [] name;
                        if(!in){
                                std::wcout << L"Failed to load module " << module.str << L".z\n";
                                ctx->setModuleValue(module.str, ctx->nil);
                                return ctx->nil;
                        }
                        extern Expression* Parse(const std::wstring&);
                        std::wstring buf = L"{\n", // no global variables :)
                                        tmp;
                        while(!in.eof()){
                                std::getline(in,tmp);
                                buf+= tmp + L"\n";
                        }
                        buf += L"\nnil\n}"; // import return nil. +TODO add optional end of module.
                        in.close();
                        auto expr = Parse(std::wstring(buf));
                        if(!expr){
                                ctx->setModuleValue(module.str, ctx->nil);
                                return ctx->nil;
                        }
                        Context * _ctx = ret_mod?(new Context(ctx->nil)):(new Context(ctx));
                        auto res = expr->eval(_ctx);
                        ctx->setModuleValue(module.str,res);
                        if(!ret_mod){
                                /*_ctx->release();*/
                                return res;
                        }
                        auto mod = new Hash(_ctx->env);
                        /*_ctx->release();*/
                        return mod;

                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                }

                virtual NodeTy type() override { return NodeTy::Import; }
        };

        class Match: public virtual Expression{
                Expression* what;
                VecHelper<Expression> *cond;
                VecHelper<Expression> *res;
                Expression * default_val;
        public:
                ~Match() override { }
                Match(Expression*what, decltype(cond) cond, decltype(res) res, Expression* default_val):what(what),cond(cond),
                res(res),default_val(default_val){}
                virtual ret_ty emit(inp_ty) override {     
                        std::wcout << L"match("; what->emit(); std::wcout << L"){\n";
                        for(auto i1 = cond->get().begin(), 
                                 i2 = res->get().begin(), 
                                 e1 = cond->get().end(), 
                                 e2 = res->get().end();
                                        i1!=e1 && i2!=e2; 
                                                ++i1,++i2){
                                std::wcout << L'\t';
                                (*i1)->emit(); std::wcout << L"=>"; (*i2)->emit();
                                std::wcout << L";\n";
                        }
                        std::wcout << L"_ =>"; default_val->emit();
                        std::wcout << L"\n}";
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        auto pattern = what->eval(ctx);
                        auto equal = ctx->findBuiltinOp(L"binary@==");
                        DBG_TRACE("builtin(match|equal):%li",(intptr_t)equal);
                        if(!equal){
                                return ctx->nil;
                        }
                        for(uint i=0;i<cond->get().size();++i){
                                auto pattern2 = cond->get()[i]->eval(ctx);
                                if(equal(ctx, {pattern,pattern2})){
                                        return res->get()[i]->eval(ctx);
                                }
                        }
                        return default_val->eval(ctx);
                }
                Expression* toArray(Context* ctx) override  {
                        return new Array(
                                new VecHelper<Expression>(
                                        new  std::vector<Expression*>({
                                                what,
                                                new Array(cond),
                                                new Array(res),
                                                default_val})));
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        what->MarkChilds(marked);
                        default_val->MarkChilds(marked);
                        cond->MarkChilds(marked);
                        res->MarkChilds(marked);
                }
                void FullRelease()override{ 
                        what->FullRelease();
                        cond->FullRelease();
                        res->FullRelease();
                        default_val->FullRelease(); 
                        delete this; 
                }
                virtual NodeTy type() override { return NodeTy::Match; }
        };

        class Cond: public virtual Expression{
                VecHelper<Expression> *cond;
                VecHelper<Expression> *res;
        public:
                ~Cond() override { }
                Cond(decltype(cond) cond, decltype(res) res):cond(cond),res(res){}
                virtual ret_ty emit(inp_ty) override {     
                        std::wcout << L"cond" << L"{\n";
                        for(auto i1 = cond->get().begin(), 
                                 i2 = res->get().begin(), 
                                 e1 = cond->get().end(), 
                                 e2 = res->get().end();
                                        i1!=e1 && i2!=e2; 
                                                ++i1,++i2){
                                std::wcout << L'\t';
                                (*i1)->emit(); std::wcout << L"=>"; (*i2)->emit();
                                std::wcout << L";\n";
                        }
                        std::wcout << L"}";
                }
                Expression* toArray(Context* ctx) override  {
                        return new Array(
                                new VecHelper<Expression>(
                                        new  std::vector<Expression*>({
                                                new Array(cond),
                                                new Array(res)})));
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        for(uint i=0;i<cond->get().size();++i){
                                auto pattern = cond->get()[i]->eval(ctx);
                                if(to_bool(pattern)){
                                        return res->get()[i]->eval(ctx);
                                }
                        }
                        return ctx->nil;
                }
                void FullRelease()override{ 
                        res->FullRelease();
                        cond->FullRelease(); 
                        delete this; 
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        cond->MarkChilds(marked);
                        res->MarkChilds(marked);
                }
                virtual NodeTy type() override { return NodeTy::Cond; }
        };

        class Block: public virtual Expression{
        public:
                VecHelper<Expression>* block;
                ~Block() override { }
                Block(VecHelper<Expression>* block):block(block){}
                virtual ret_ty emit(inp_ty) override {                        
                        std::wcout << L"{\n";
                        for(auto&x:block->get()){
                                std::wcout << L'\t';
                                x->emit();
                                std::wcout << L";\n";
                        }
                        std::wcout << L"}";
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        Expression* last;
                        Context *_ctx = new Context(ctx);
                        for(auto&x:block->get()){
                                last = x->eval(_ctx);
                        }
                        /*_ctx->release();*/
                        return last;
                }
                Expression* toArray(Context* ctx) override { return new Array(block); }
                virtual NodeTy type() override { return NodeTy::Block; }
                virtual void FullRelease() override { 
                        block->FullRelease(); 
                        delete this; 
                }
                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        block->MarkChilds(marked);
                }
        };

        class Let: public virtual Expression{
                Token name;
                Expression* value;
        public:
                ~Let() override { }
                Let(const Token& name, Expression* value):name(name),value(value){}
                virtual ret_ty emit(inp_ty) override { 
                        std::wcout << L"let "<< name.str << L" = "; 
                        value->emit();
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        ctx->createVar(name.str);
                        Expression* val;
                        ctx->setVar(name.str,val = value->eval(ctx));
                        ctx->setImmutableState(name.str);
                        return val;
                }
                void FullRelease()override{ 
                        value->FullRelease(); 
                        delete this; 
                }
                
                Expression* toArray(Context* ctx) override { return new Array(new VecHelper<Expression>({value})); }

                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        value->MarkChilds(marked);
                }
                virtual NodeTy type() override { return NodeTy::Let; }
        };

        class Var: public virtual Expression{
                Token name;
                Expression* value;
        public:
                ~Var() override { }
                Var(const Token& name, Expression* value):name(name),value(value){}
                virtual ret_ty emit(inp_ty) override { 
                        std::wcout << L"var "<< name.str << L" = "; 
                        if(value){
                                value->emit();
                        } else {
                                std::wcout << L"nil";
                        }
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        ctx->createVar(name.str);
                        if(!value){
                                return ctx->nil;
                        }
                        Expression* val;
                        ctx->setVar(name.str,val = value->eval(ctx));
                        return val;
                }
                void FullRelease()override{ 
                        if(value){
                                value->FullRelease();
                        } 
                        delete this; 
                }
                Expression* toArray(Context* ctx) override { return new Array(new VecHelper<Expression>({value})); }

                void MarkChilds(std::set<Collectable*>& marked) override {
                        marked.insert(this);
                        if(value){
                                value->MarkChilds(marked);
                        }
                }
                virtual NodeTy type() override { return NodeTy::Var; }
        };

        namespace {
                bool to_bool(Expression* expr){
                        if(expr->type()==NodeTy::Nil){
                                return false;
                        }
                        if(expr->type()==NodeTy::Boolean){
                                return expr->as<Boolean>()->value;
                        }
                        return true;
                }
                std::wstring typeof_str(NodeTy ty){
                        #define _case(x) case NodeTy:: x : return L## #x
                        switch(ty){
                                _case(Number);
                                _case(String);
                                _case(Boolean);
                                _case(Nil);
                                _case(Hash);
                                _case(Array);
                                _case(Function);
                                _case(Macro);
                                _case(NativeFunction);
                                case NodeTy::AstNode: return L"Expression";
                                default: return L"UndefinedExpression";
                        }
                        #undef _case
                }
                Expression* __newContextExpr(Context*ctx){
                        return new ContextExpr(ctx);
                }
        }
}