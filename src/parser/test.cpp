#include "Parser.hpp"
#include <iostream>
using namespace Z;
int main(){
        std::wstring str;
        std::getline(std::wcin,str);
    Parser par (str);
    auto ast = par.Parse();
    std::wcout << par.isSuccess() << L' ' << par.ErrorMsg() << std::endl;
    if(par.isSuccess()) ast->emit();
}
