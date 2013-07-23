#include "Parser.hpp"
#include <iostream>
using namespace Z;
namespace Z{Expression* Parse(const std::wstring& s){
        return Parser(s).Parse();
}}
int main(){
        std::wstring str,tmp;
        while(!std::cin.eof()){
                std::getline(std::wcin,tmp);
                if(tmp == L"run!")break;
                str+=tmp + L'\n';
        }
        Parser par (str);
        auto ast = par.Parse();
        std::wcout << par.isSuccess() << L' ' << par.ErrorMsg() << std::endl;
        if(par.isSuccess()){
                Context* ctx = new Context();
                ast->emit();
                std::wcerr <<L'\n';
                Value val = ast->eval(ctx);
                if(val.type == ValType::Number){
                        std::wcout << val.num;
                }
                if(val.type == ValType::Boolean){
                        std::wcout << (val.boolv?L"true":L"false");
                }
                if(val.type == ValType::String){
                        std::wcout << *val.str;
                }
                if(val.type == ValType::Null){
                        std::wcout << L"nil";
                }
                if(val.type == ValType::Function){
                        std::wcout << L"<function>";
                }
                if(val.type == ValType::Expression){
                        std::wcout << L"<expression>";
                }
                std::wcout << std::endl;
                ctx->Release();
                ast->FullRelease();
        }
}
