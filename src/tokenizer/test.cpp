#include "Tokenizer.hpp"
#include <iostream>
using namespace Z;
int main(){
    Tokenizer tkn (L"abc 1234 1345.6 + - $:: $: $::: abc\n qag ");
    Token tok;
    tkn.DefKw(L"$::", SubTokTy::Quasi,true);
    while((tok=tkn.Next()).ty!=TokTy::None){
	std::wcout << tok.str << '\n';
    }
}
