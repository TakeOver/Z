#include "Parser.hpp"
#include <iostream>
using namespace Z;
int main(){
        std::wstring str;
        std::getline(std::wcin,str);
    Parser par (str);
    par.Parse();
    std::wcout << par.isSuccess() << L' ' << par.ErrorMsg() << std::endl;
}
