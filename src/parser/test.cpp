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
