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
        };

        class Array: public virtual Expression{
        public:
                std::vector<Expression*>* value;
                ~Array() override { }
                Array(decltype(value) value):value(value){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"[";
                        for(int i= 0; i<((int)value->size())-1;++i){
                                (*value)[i]->emit();
                                std::wcout << L",";
                        }
                        if(value->size()){
                                value->back()->emit();
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
                        if(std::abs(key)>=value->size()){
                                return ctx->nil;
                        }
                        if(key<0){
                                key = value->size() - key;
                        }
                        return (*value)[key];
                }
                Expression* set(Context* ctx, int64_t key, Expression* _value){
                        if(key<0){
                                key = value->size() - key;
                        }
                        if(key<0){
                                return ctx->nil;
                        }
                        if((key)>=value->size()){
                                value->resize(std::abs(key));
                        }
                        return (*value)[key] = _value;
                }
                virtual NodeTy type() override { return NodeTy::Array; }
        };

        class Function: public virtual Expression{
        public:
                Context* _ctx;
                Expression* body;
                VecHelper<Variable>* args;
                bool is_ellipsis;
                ~Function() override { }
                Function(Context* _ctx,Expression * body, VecHelper<Variable> *args, bool is_ellipsis=false):_ctx(_ctx),body(body),args(args),
                        is_ellipsis(is_ellipsis){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"( ";
                        for(int i=0;i<((int)args->get().size())-1;++i){
                                args->get()[i]->emit();
                                std::wcout << L",";
                        }
                        if(args->get().size()){
                                args->get().back()->emit();
                        }
                        if(is_ellipsis)std::wcout << L"...";
                        std::wcout << L")->";
                        body->emit();
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
                        auto __ctx = new Context(_ctx);
                        auto &varargs = args->get();
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
                                __ctx->setVar(varargs.back()->getname(),new Array(rest));
                        }
                        return body->eval(__ctx);

                }
                virtual NodeTy type() override { return NodeTy::Function; }
        };

        class NativeFunction: public virtual Expression{
        public:
                native_fun_t func_ptr;
                ~NativeFunction() override { }
                NativeFunction(native_fun_t func_ptr):func_ptr(func_ptr){ }
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"NativeFunction!";
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return this;
                }
                virtual NodeTy type() override { return NodeTy::NativeFunction; }
                Expression* call(Context* ctx, const std::vector<Expression*>& args){
                        return func_ptr(ctx,args);
                }
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
                        return ctx->nil;

                }
                void FullRelease()override{ 
                        lhs->FullRelease(); 
                        rhs->FullRelease();
                        delete this; 
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
                        return ctx->nil;
                }
                void FullRelease()override{ 
                        lhs->FullRelease(); 
                        delete this; 
                }
                virtual NodeTy type() override { return NodeTy::UnOp; }
        };

        class String: public virtual Expression{
        public:
                std::wstring* value;
                ~String() override { }
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
                virtual NodeTy type() override { return NodeTy::String; }
        };

        class Hash: public virtual Expression{
        public:
                std::unordered_map<std::wstring, Expression*>* value;
                ~Hash() override { }
                Hash(decltype(value) value):value(value){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"#{";
                        uint i = 0;
                        for(auto&x:*value){
                                ++i;
                                std::wcout <<L"\"" <<  x.first << L"\"=";
                                x.second->emit();
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
                        auto iter = value->find(key);
                        if(iter!=value->end()){
                                return iter->second;
                        }
                        return ctx->nil;
                }
                Expression* set(Context* ctx, const std::wstring & key, Expression* value){
                        (*this->value)[key] = value;
                        return value;
                }
                virtual NodeTy type() override { return NodeTy::Hash; }
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
                virtual NodeTy type() override { return NodeTy::Boolean; }
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
                void FullRelease()override{ 
                        what->FullRelease(); 
                        delete this; 
                }
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
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        auto ell = what->eval(ctx);
                        if(ell->type()!=NodeTy::Array){
                                return new Array(new std::vector<Expression*>());
                        }
                        return ell;
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
                void FullRelease()override{ 
                        body->FullRelease(); 
                        cond->FullRelease();
                        delete this; 
                }
                virtual NodeTy type() override { return NodeTy::While; }
        };

        class ArrayAst: public virtual Expression{
                VecHelper<Expression>* arr;
        public:
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
                        return new Array(res);
                }
                void FullRelease()override{ 
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
                virtual NodeTy type() override { return NodeTy::Export; }
        };

        class Lambda: public virtual Expression{
                Expression* body;
                VecHelper<Variable>* args;
                bool is_ellipsis;
        public:
                ~Lambda() override { }
                Lambda(Expression * body, VecHelper<Variable> *args, bool is_ellipsis=false):body(body),args(args),
                        is_ellipsis(is_ellipsis){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcout << L"(";
                        Function(nullptr,body,args,is_ellipsis).emit();
                        std::wcout << L")";
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
                virtual NodeTy type() override { return NodeTy::Lambda; }
        };

        
        class AstNode: public virtual Expression{
        public:
                Expression* expr;
                Context* _ctx;
                ~AstNode() override { /*_ctx->release();*/ }
                AstNode(Expression* expr,Context * _ctx):expr(expr),_ctx(_ctx){}
                virtual ret_ty emit(inp_ty) override {
                        expr->emit();
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        return expr->eval(_ctx);
                }
                virtual NodeTy type() override { return NodeTy::AstNode; }
                virtual void FullRelease() override { 
                        expr->FullRelease(); 
                        delete this; 
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
        };

        class FCall: public virtual Expression{
                Expression* func;
                VecHelper<Expression>* args;
        public:
                ~FCall() override { }
                FCall(Expression * func, VecHelper<Expression> *args):func(func),args(args){}
                virtual ret_ty emit(inp_ty) override {
                        func->emit();
                        std::wcout << L'(' << L' ';
                        for(auto&x:args->get()){
                                x->emit();
                                std::wcout << L' ';
                        }
                        std::wcout << L')';
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
                                        for(auto&y:*dynamic_cast<Array*>(ell)->value){
                                                _args.push_back(y);
                                        }
                                }else{
                                        _args.push_back(x->eval(ctx));
                                }
                        }
                        if(fun->type() == NodeTy::NativeFunction){
                                return fun->as<NativeFunction>()->call(ctx,_args);
                        }
                        if(fun->type()==NodeTy::Function){
                                return fun->as<Function>()->call(ctx, _args,false);
                        }       
                        return ctx->nil;
                }
                virtual NodeTy type() override { return NodeTy::FCall; }
                void FullRelease()override{  
                        args->FullRelease(); 
                        func->FullRelease(); 
                        delete this; 
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
                        std::wcout << L"match("; what->emit(); std::wcout << L"){";
                        for(auto i1 = cond->get().begin(), 
                                 i2 = res->get().begin(), 
                                 e1 = cond->get().end(), 
                                 e2 = res->get().end();
                                 i1!=e1 && i2!=e2; 
                                 ++i1,++i2){
                                (*i1)->emit(); std::wcout << L"=>"; (*i2)->emit();
                                std::wcout << L'\t';
                        }
                        std::wcout << L"_ =>"; default_val->emit();
                        std::wcout << L"}";
                }
                virtual Expression* eval(Context*ctx)override{
                        DBG_TRACE();
                        auto pattern = what->eval(ctx);
                        auto equal = ctx->findBuiltinOp(L"binary@==");
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
                        std::wcout << L"cond" << L"{\t";
                        for(auto i1 = cond->get().begin(), 
                                 i2 = res->get().begin(), 
                                 e1 = cond->get().end(), 
                                 e2 = res->get().end();
                                 i1!=e1 && i2!=e2; 
                                 ++i1,++i2){
                                (*i1)->emit(); std::wcout << L"=>"; (*i2)->emit();
                                std::wcout << L'\t';
                        }
                        std::wcout << L"}";
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
                virtual NodeTy type() override { return NodeTy::Block; }
                virtual void FullRelease() override { 
                        block->FullRelease(); 
                        delete this; 
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
                        return val;
                }
                void FullRelease()override{ 
                        value->FullRelease(); 
                        delete this; 
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
                                default: return L"Expression";
                        }
                        #undef _case
                }
        }
}