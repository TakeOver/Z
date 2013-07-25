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
int main(){
        std::wstring str =L"{\n",tmp, at_end = L"nil";
        while(!std::cin.eof()){
                std::getline(std::wcin,tmp);
                if(tmp == L"run!")break;
                str+=tmp + L'\n';
        }
        str+=L"\n" + at_end + L"\n}";
        Parser par (str);
        auto ast = par.Parse();
        std::wcout << par.isSuccess() << L' ' << par.ErrorMsg() << std::endl;
        if(par.isSuccess()){
                Context* ctx = new Context();
                ctx->createVar(L"input");
                ctx->setVar(L"input",Value(input)); 
                ctx->createVar(L"set!");
                ctx->setVar(L"set!",Value(set));
                ctx->createVar(L"parse!");
                ctx->setVar(L"parse!",Value(parse));
                Z::print(&ctx->getEnv());
                std::wcerr << L'\n';
                ast->emit();
                std::wcerr <<L'\n';
                ast->eval(ctx);
                std::wcout << std::endl;
                ctx->Release();
                ast->FullRelease();
        }
}
