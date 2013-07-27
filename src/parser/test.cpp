#include "Parser.hpp"
#include <iostream>
using namespace Z;
namespace Z{Expression* Parse(const std::wstring& s){
        return Parser(s).Parse();
}}
Value input(Z::Context* ctx, const std::vector<Value>& args){
        for(auto&x:args){
                Z::print(x);
        }
        std::wstring* str = new std::wstring();
        std::getline(std::wcin,*str);
        return Value(str);
}
Value len(Z::Context* ctx, const std::vector<Value>& args){
        auto obj = args.front();
        switch(obj.type){
                case ValType::String: return Value(static_cast<double>(obj.str->length()));
                case ValType::Array: return Value(static_cast<double>(obj.arr->size()));
                default: return Value(0.0);
        }
}
Expression* to_ast(Value val){
        switch(val.type){
                case ValType::Expression: return val.expr;
                case ValType::Number: return new Number(val.num);
                case ValType::Function: return new Lambda(val.fun->body,val.fun->args);
                case ValType::String: return new String(val.str);
                case ValType::Null: return new Nil();
                case ValType::Boolean: return new Boolean((bool)val.boolv);
                default: {
                        std::wcerr << L"Not implemented convertion to ast\n";
                        exit(0);
                }
        }
}
Value append(Z::Context* ctx, const std::vector<Value>& args){
        auto self = args.front();
        if(self.expr->type()==NodeTy::Hash){
                if(args.size()<3){
                        return ctx->null;
                }
                auto key = to_ast(args[1]);
                auto value = to_ast(args[2]);
                auto hash = dynamic_cast<Hash*>(self.expr);
                hash->keys->get().push_back(key);
                hash->arr->get().push_back(value);
        } else if(self.expr->type() == NodeTy::Block){
                if(args.size()<2){
                        return ctx->null;
                }
                auto value = to_ast(args[1]);
                dynamic_cast<Block*>(self.expr)->block->get().push_back(value);
        }
        return self;
}
Value set(Z::Context*ctx, const std::vector<Value>&args){
        if(args.size()!=2){
                return ctx->null;
        }
        auto var = args.front();
        auto what = args.back();
        if(var.type!=ValType::Expression){
                return ctx->null;
        }
        if(var.expr->type()!=NodeTy::Variable){
                return ctx->null;
        }
        var.ctx->setVar(dynamic_cast<Variable*>(var.expr)->getname(),what);
        return what;
}
Value parse(Z::Context*ctx, const std::vector<Value>&args){
        auto what = args.back();
        if(what.type!=ValType::String){
                return ctx->null;
        }
        Parser par (*what.str);
        auto ast = par.Parse();
        if(!ast){
                return ctx->null;
        }
        return Value(ast,ctx);
}
namespace Z{Value fcall(Value fun,const std::vector<Value> & _args,Context *ctx){
        if(fun.type==ValType::NativeFunction){
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
}}
int main(){
        std::wstring str =L"{\n import core; \n",tmp, at_end = L"nil";
        while(!std::cin.eof()){
                std::getline(std::wcin,tmp);
                if(tmp == L"run!")break;
                str+=tmp + L'\n';
        }
        str+=L"\n" + at_end + L"\n}";
        Parser par (str);
        auto ast = par.Parse();
        std::wcout << par.isSuccess() << L' ' << par.ErrorMsg() << std::endl;
        if(par.isSuccess() && ast){
                Context* ctx = new Context();
                ctx->createVar(L"input");
                ctx->setVar(L"input",Value(input)); 
                ctx->createVar(L"set!");
                ctx->setVar(L"set!",Value(set));
                ctx->createVar(L"parse!");
                ctx->setVar(L"parse!",Value(parse));
                ctx->createVar(L"Native");
                ctx->setVar(L"Native",Value(new std::unordered_map<std::wstring, Value>({
                        {L"ast",Value(new std::unordered_map<std::wstring, Value>({
                                {L"append",Value(append)}}))
                },      {L"str",Value(new std::unordered_map<std::wstring,Value>({{L"len",Value(len)}}))}})));
                Z::print(&ctx->getEnv());
                std::wcerr << L'\n';
                ast->emit();
                std::wcerr <<L'\n';
                ast->eval(ctx);
                std::wcout << std::endl;
                ctx->Release();
             //   ast->FullRelease();
        }
}
