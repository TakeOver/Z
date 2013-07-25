#pragma once
#include <iostream>
#include "basic_node.hpp"
#include "../tokenizer/Token.hpp"
#include <fstream>
#include <cstring>
namespace Z{
        class Variable: public virtual Expression{
        public:
                Token name;
                ~Variable() override { }
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
        class BinOp: public virtual Expression{
        public:
                Token op;
                Expression * lhs, * rhs;
                ~BinOp() override { delete lhs; delete rhs; }
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
                                return add(lhs->eval(ctx), rhs->eval(ctx),ctx);
                        if(op.str == L"-")
                                return sub(lhs->eval(ctx), rhs->eval(ctx),ctx);
                        if(op.str == L"/")
                                return div(lhs->eval(ctx), rhs->eval(ctx),ctx);
                        if(op.str == L"*")
                                return mul(lhs->eval(ctx), rhs->eval(ctx),ctx);
                        if(op.str == L"%")
                                return mod(lhs->eval(ctx), rhs->eval(ctx),ctx);
                        if(op.str == L">")
                                return great(lhs->eval(ctx), rhs->eval(ctx),ctx);
                        if(op.str == L"<")
                                return less(lhs->eval(ctx), rhs->eval(ctx),ctx);
                        if(op.str == L">=")
                                return great_eq(lhs->eval(ctx), rhs->eval(ctx),ctx);
                        if(op.str == L"<=")
                                return less_eq(lhs->eval(ctx), rhs->eval(ctx),ctx);
                        if(op.str == L"==")
                                return eq(lhs->eval(ctx), rhs->eval(ctx),ctx);
                        if(op.str == L"!=")
                                return notb(eq(lhs->eval(ctx),rhs->eval(ctx),ctx),ctx);
                        if(op.str == L"." || op.str == L"["){
                                auto obj = lhs->eval(ctx);
                                auto key = rhs->eval(ctx);
                                if(key.type == ValType::String){
                                        return getKey(obj, *key.str, ctx);
                                }
                                if(key.type == ValType::Number){
                                        return getIdx(obj,static_cast<int64_t>(key.num),ctx);
                                }
                                return ctx->null;
                        }
                        if(op.str == L"and"){
                                auto lval = lhs->eval(ctx);
                                if(to_bool(lval)){
                                        return rhs->eval(ctx);
                                }
                                return lval;
                        }
                        if(op.str == L"or"){
                                auto lval = lhs->eval(ctx);
                                if(to_bool(lval)){
                                        return lval;
                                }
                                return rhs->eval(ctx);
                        }
                        if(op.str == L"="){
                                if(lhs->type() == NodeTy::BinOp){
                                        auto bo = dynamic_cast<BinOp*>(lhs);
                                        if(bo->op.str == L"[" || bo->op.str == L"."){
                                                auto _what = bo->lhs->eval(ctx);
                                                auto _key = bo->rhs->eval(ctx);
                                                Value val;
                                                if(_key.type == ValType::String){
                                                        setKey(_what, *_key.str,val=rhs->eval(ctx), ctx);
                                                }
                                                if(_key.type == ValType::Number){
                                                        setIdx(_what,static_cast<int64_t>(_key.num),val = rhs->eval(ctx),ctx);
                                                }
                                                return val;
                                        }
                                }
                                if(lhs->type()!=NodeTy::Variable){
                                        std::wcerr << L"Variable expected as LHS\n";
                                        return ctx->null;
                                }
                                Value val;
                                ctx->setVar(dynamic_cast<Variable*>(lhs)->name.str,val = rhs->eval(ctx));
                                return val;
                        }
                        return ctx->null;
                }
                virtual NodeTy type() override { return NodeTy::BinOp; }
        };
        class UnOp: public virtual Expression{
                Token op;
                Expression * lhs;
        public:
                ~UnOp() override { delete lhs; }
                UnOp(const Token& op, Expression * lhs):op(op),lhs(lhs){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << op.str << L"(";
                        lhs->emit();
                        std::wcerr << L")";
                }
                virtual Value eval(Context*ctx)override{
                        if(op.str == L"+")
                                return lhs->eval(ctx);
                        auto val = lhs->eval(ctx);
                        if(op.str == L"-" && val.type == ValType::Number)
                                return Value(-val.num);
                        if(op.str == L"!"){
                                return notb(val,ctx);
                        }
                        return Value();
                }
                virtual NodeTy type() override { return NodeTy::UnOp; }
        };
        class String: public virtual Expression{
                Token value;
        public:
                ~String() override { }
                String(const Token& value):value(value){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << L"\"" << value.str << L"\"";
                }
                virtual Value eval(Context*ctx)override{
                        return Value(&value.str);       
                }
                virtual NodeTy type() override { return NodeTy::String; }
        };
        class Boolean: public virtual Expression{
                bool value;
        public:
                ~Boolean() override { }
                Boolean(bool value):value(value){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << (value?L" true ":L" false ");
                }
                virtual Value eval(Context*ctx)override{
                        return Value(value);       
                }
                virtual NodeTy type() override { return NodeTy::Boolean; }
        };
        class Nil: public virtual Expression{
        public:
                ~Nil() override { }
                Nil(){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << (L"nil");
                }
                virtual Value eval(Context*ctx)override{
                        return Value();       
                }
                virtual NodeTy type() override { return NodeTy::Nil; }
        };
        class Ellipsis: public virtual Expression{
                Expression* what;
        public:
                ~Ellipsis() override { what->FullRelease(); }
                Ellipsis(Expression * what):what(what){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << (L"..."); what->emit();
                }
                virtual Value eval(Context*ctx)override{
                        auto ell = what->eval(ctx);
                        if(ell.type!=ValType::Array){
                                return Value(new std::vector<Value>());
                        }
                        return ell;
                }
                virtual NodeTy type() override { return NodeTy::Ellipsis; }
        };
        class For: public virtual Expression{
                Variable * var;
                Expression *from, *to,*body;

        public:
                ~For() override { delete var; from->FullRelease(); to->FullRelease(); body->FullRelease(); }
                For(decltype(var)var,decltype(from)from,decltype(to)to,decltype(body) body):var(var),from(from),to(to),body(body){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << (L"for ") << var->getname() << L' ';
                        from->emit(); std::wcerr << L" to "; to->emit(); std::wcerr << L' ';
                        body->emit();
                }
                virtual Value eval(Context*ctx)override{
                        auto _ctx = new Context(ctx);
                        auto _from =  from->eval(ctx);
                        if(_from.type!=ValType::Number){
                                ctx->RaiseException(L"Number expected as for-range(from)");
                                return ctx->null;
                        }
                        _ctx->createVar(var->getname());
                        _ctx->setVar(var->getname(),_from);
                        auto _to = to->eval(ctx);
                        if(_to.type!=ValType::Number){
                                ctx->RaiseException(L"Number expected as for-range(to)");
                                return ctx->null;
                        }
                        auto step = 1.0;
                        if(_from.num > _to.num)step=-step;
                        Value res;
                        while(to_bool(less_eq(_ctx->getVar(var->getname()),_to,ctx))){
                                res=body->eval(_ctx);
                                auto _var = _ctx->getVar(var->getname());
                                if(_var.type!=ValType::Number){
                                        ctx->RaiseException(L"Variable type must be number in for-range");
                                        return ctx->null;
                                }
                                _ctx->setVar(var->getname(),Value(_var.num + step));
                        }
                        return res;


                }
                virtual NodeTy type() override { return NodeTy::For; }
        };
        class While: public virtual Expression{
                Expression* cond, *body;
        public:
                ~While() override { cond->FullRelease(); body->FullRelease(); }
                While(decltype(cond) cond,decltype(body) body):cond(cond),body(body){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << (L"while("); cond->emit(); std::wcerr << L")";
                        body->emit();
                }
                virtual Value eval(Context*ctx)override{
                        Value res;
                        auto _ctx = new Context(ctx);
                        while(to_bool(cond->eval(ctx))){
                                res = body->eval(_ctx);
                        }
                        _ctx->Release();
                        return res;
                }
                virtual NodeTy type() override { return NodeTy::While; }
        };
        class Array: public virtual Expression{
                VecHelper<Expression>* arr;
        public:
                ~Array() override { delete arr; }
                Array(decltype(arr) arr):arr(arr){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << (L"Array!( ");
                        for(auto&x:arr->get()){
                                x->emit();
                                std::wcerr << L' ';
                        }
                        std::wcerr << L')';
                }
                virtual Value eval(Context*ctx)override{
                        std::vector<Value>* array = new std::vector<Value>();
                        array->resize(arr->get().size());
                        uint it = 0;
                        for(auto&x:arr->get()){
                                (*array)[it] = x->eval(ctx);
                                ++it;
                        }
                        return Value(array);
                }
                virtual NodeTy type() override { return NodeTy::Array; }
        };
        class Hash: public virtual Expression{
                VecHelper<Expression>* arr;
                std::vector<Token> keys;
        public:
                ~Hash() override { delete arr; }
                Hash(decltype(arr) arr,decltype(keys)keys):arr(arr),keys(keys){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << (L"Hash!( ");
                        for(auto&x:arr->get()){
                                x->emit();
                                std::wcerr << L' ';
                        }
                        std::wcerr << L')';
                }
                virtual Value eval(Context*ctx)override{
                        std::unordered_map<std::wstring,Value>* hash = new std::unordered_map<std::wstring,Value>();
                        uint it = 0;
                        for(auto&x:arr->get()){
                                (*hash)[keys[it].str] = x->eval(ctx);
                                ++it;
                        }
                        return Value(hash);
                }
                virtual NodeTy type() override { return NodeTy::Hash; }
        };
        class Show: public virtual Expression{
                Expression * what;
                bool newline;
        public:
                ~Show() override { what->FullRelease(); }
                Show(Expression* what, bool newline = false):what(what),newline(newline){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << (L"show!( "); what->emit(); std::wcerr << L" )\n";
                }
                virtual Value eval(Context*ctx)override{
                        auto val = what->eval(ctx);
                        print(val);
                        if(newline){
                                std::wcout << L'\n';
                        }
                        return val;
                }
                virtual NodeTy type() override { return NodeTy::Show; }
        };
        class Export: public virtual Expression{
                Token variable;
        public:
                ~Export() override { }
                Export(const Token& variable):variable(variable){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << (L"export") << variable.str;
                }
                virtual Value eval(Context*ctx)override{
                        auto val = ctx->getVar(variable.str);
                        ctx = ctx->getRoot();
                        ctx->createVar(variable.str);
                        ctx->setVar(variable.str, val);
                        return val;
                }
                virtual NodeTy type() override { return NodeTy::Export; }
        };
        class Number: public virtual Expression{
                Token value;
        public:
                ~Number() override { }
                Number(const Token& value):value(value){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << value.str;
                }
                virtual Value eval(Context*ctx)override{
                        return Value(wcstod(value.str.c_str(),nullptr));
                }
                virtual NodeTy type() override { return NodeTy::Number; }
        };
        class Lambda: public virtual Expression{
                Expression* body;
                VecHelper<Variable>* args;
                bool is_ellipsis;
        public:
                ~Lambda() override { delete args; delete body; }
                Lambda(Expression * body, VecHelper<Variable> *args, bool is_ellipsis=false):body(body),args(args),
                        is_ellipsis(is_ellipsis){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << L"lambda!( ";
                        for(auto&x:args->get()){
                                x->emit();
                                std::wcerr << L" ";
                        }
                        if(is_ellipsis)std::wcerr << L"... ";
                        std::wcerr << L")->";
                        body->emit();
                }

                virtual Value eval(Context*ctx)override{
                        ctx->AddRef();
                        return Value(new Function(ctx,args,body,is_ellipsis));       
                }
                virtual NodeTy type() override { return NodeTy::Lambda; }
        };
        class FCall: public virtual Expression{
                Expression* func;
                VecHelper<Expression>* args;
        public:
                ~FCall() override { delete args; delete func; }
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
                        Value fun;
                        std::vector<Value> _args;
                        BinOp *obj = dynamic_cast<decltype(obj)>(func);
                        if(obj && (obj->op.str == L"." || obj->op.str == L"[")){
                                auto    _obj = obj->lhs->eval(ctx), 
                                        _key = obj->rhs->eval(ctx);
                                if(_key.type==ValType::String){
                                        fun = getKey(_obj,*_key.str,ctx);
                                        _args.push_back(_obj);
                                }else if(_key.type == ValType::Number){
                                        fun = getIdx(_obj,(int64_t)_key.num,ctx);
                                }
                        }else{
                                fun = func->eval(ctx);
                        }
                        for(auto&x:args->get()){
                                if(dynamic_cast<Ellipsis*>(x)!=0){
                                        auto ell = x->eval(ctx);
                                        for(auto&y:*ell.arr){
                                                _args.push_back(y);
                                        }
                                }else{
                                        _args.push_back(x->eval(ctx));
                                }
                        }
                        if(fun.type == ValType::NativeFunction){
                                return fun.native(ctx,_args);
                        }
                        if(fun.type!=ValType::Function){
                                return ctx->null;
                        }       
                        Context* _ctx = new Context(fun.fun->ctx);
                        auto& vars = fun.fun->args->get();
                        bool ell = fun.fun->is_ellipsis;
                        for(uint j = 0; j<_args.size();++j){
                                if(j+1==vars.size() && ell){
                                        std::vector<Value>* rest = new std::vector<Value>();
                                        for(uint k = j; k<_args.size();++k){
                                                rest->push_back(_args[k]);
                                        }
                                        _ctx->createVar(vars.back()->getname());
                                        _ctx->setVar(vars.back()->getname(), Value(rest));
                                        break;
                                }
                                if(j>=vars.size()){
                                        break;
                                }
                                _ctx->createVar(vars[j]->getname());
                                _ctx->setVar(vars[j]->getname(),_args[j]);
                        }
                        auto res = fun.fun->body->eval(_ctx);
                        _ctx->Release();
                        return res;
                }
                virtual NodeTy type() override { return NodeTy::FCall; }
        };

        class Expr: public virtual Expression{
                Expression* expr;
        public:
                ~Expr() override { delete expr; }
                Expr(Expression* expr):expr(expr){}
                virtual ret_ty emit(inp_ty) override { expr->emit(); }
                virtual Value eval(Context*ctx)override{
                        return expr->eval(ctx);
                }
                virtual NodeTy type() override { return NodeTy::Expr; }
        };
        class Import: public virtual Expression{
                Token module;
        public:
                ~Import() override {}
                Import(const Token& module):module(module){}
                virtual ret_ty emit(inp_ty) override { std::wcerr << L"import " << module.str; }
                virtual Value eval(Context*ctx)override{
                        char * name = new char[module.str.length()*4+10];
                        wcstombs(name, module.str.c_str(), module.str.length()*4+1);
                        strcpy(name+strlen(name),".z\0");
                        std::wifstream in (name);
                        delete [] name;
                        if(!in){
                                ctx->RaiseException(Value(new std::wstring(L"Failed to load module")));
                                std::wcerr << L"Failed to load module " << module.str << L".z\n";
                                return ctx->null;
                        }
                        extern Expression* Parse(const std::wstring&);
                        std::wstring buf = L"{", // no global variables :)
                                        tmp;
                        while(!in.eof()){
                                std::getline(in,tmp);
                                buf+= tmp + L"\n";
                        }
                        buf += L"\nnil\n}"; // import return nil. +TODO add optional end of module.
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
                Expression* expr;
        public:
                ~AstNodeExpr() override {}
                AstNodeExpr(Expression* expr):expr(expr){}
                virtual ret_ty emit(inp_ty) override {
                        std::wcerr << L"expr!(";
                        expr->emit();
                        std::wcerr << L")";
                }
                virtual Value eval(Context*ctx)override{
                        ctx->AddRef();
                        return Value(expr,ctx);
                }
                virtual NodeTy type() override { return NodeTy::AstNodeExpr; }
                virtual void FullRelease() override { 
                        expr->FullRelease(); 
                        delete this; 
                }
        };
        class EvalExpr: public virtual Expression{
                Expression* expr;
        public:
                ~EvalExpr() override { }
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
                        return hom;
                }
                virtual NodeTy type() override { return NodeTy::EvalExpr; }
                virtual void FullRelease() override { 
                        expr->FullRelease(); 
                        delete this; 
                }
        };
        class Match: public virtual Expression{
                Expression* what;
                VecHelper<Expression> *cond;
                VecHelper<Expression> *res;
        public:
                ~Match() override { delete cond; delete res;what->FullRelease(); }
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
        class Cond: public virtual Expression{
                VecHelper<Expression> *cond;
                VecHelper<Expression> *res;
        public:
                ~Cond() override { delete cond; delete res; }
                Cond(decltype(cond) cond, decltype(res) res):cond(cond),res(res){}
                virtual ret_ty emit(inp_ty) override {     
                        std::wcerr << L"cond!" << L"{\n";
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
                        for(uint i=0;i<cond->get().size();++i){
                                auto pattern = cond->get()[i]->eval(ctx);
                                if(to_bool(pattern)){
                                        return res->get()[i]->eval(ctx);
                                }
                        }
                        return ctx->null;
                }
                virtual NodeTy type() override { return NodeTy::Cond; }
        };
        class Block: public virtual Expression{
                VecHelper<Expression>* block;
        public:
                ~Block() override { /*if(block)block->FullRelease();*/ }
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
                        Context *_ctx = new Context(ctx);
                        for(auto&x:block->get()){
                                last = x->eval(_ctx);
                        }
                        _ctx->Release();
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
                ~Let() override { value->FullRelease(); }
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
                Token name;
                Expression* value;
        public:
                ~Var() override {
                        if(value){
                                value->FullRelease();
                        }
                }
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