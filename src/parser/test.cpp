#include "Parser.hpp"
#include <iostream>
using namespace Z;
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
        if(par.isSuccess()) ast->emit(),std::wcerr <<L'\n'  <<ast->eval(new Context()).num << L'\n';
}
