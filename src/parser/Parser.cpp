#include "Parser.hpp"
#include "../debug.hpp"
namespace Z{
        namespace {using p = Parser;}
        p::Parser(const std::wstring & code):tkn(code){_initTokenizer(); tkn.Next();}
        p::~Parser(){}
        void p::_initTokenizer(){
                #define tok(ty) Token(TokTy::None,L## #ty,0,0,ty)
                tkn.DefKw(L"$::",SubTokTy::Quasi,true);
                tkn.DefKw(L"expr",SubTokTy::ExprNode);
                tkn.DefKw(L"@::",SubTokTy:: ExprNode,true);
                tkn.DefKw(L"stmt",SubTokTy:: Quasi);
                defop(L"+",100);
                defop(L"-",100);
                defop(L"=",150);
                defop(L"==",150);
                defop(L"!=",150);
                defop(L">=",150);
                defop(L"<=",150);
                defop(L">",150);
                defop(L"<",150);
                defop(L"/",150);
                defop(L"*",150);
                defop(L"!",150);
                defop(L"#",150);
                defop(L"%",150);
                defop(L"^",150);
                defop(L"&",150);
                defop(L"&&",150);
                defop(L"|",150);
                defop(L"||",150);
                defop(L"~",150);
                tkn.DefKw(L",",SubTokTy::Comma,true);
                defop(L"[",150,SubTokTy::LSqParen,tok(SubTokTy::RSqParen));
                tkn.DefKw(L"]",SubTokTy::RSqParen);
                defop(L"{",150,SubTokTy::LBlock,tok(SubTokTy::RBlock));
                tkn.DefKw(L"}",SubTokTy::RBlock);
                #undef defop
                #undef def
                defop(L"(",150, SubTokTy::LParen,tok(SubTokTy::RParen));
                tkn.DefKw(L")", SubTokTy::RParen);
                #undef tok
                op_precedence[L"unary$-"] = 150;                
                op_precedence[L"unary$+"] = 150;
        }
        void p::defop(const std::wstring& str, int64_t prec, SubTokTy sty, Token follows){
                tkn.DefKw(str, sty,true);
                op_precedence[str] = prec;
                if(follows.sty!=SubTokTy::None){
                        follows_oper[sty] = follows;
                }
        }
        String* p::expectString(){
                DBG_TRACE();
                auto res = new String(tkn.Last());
                tkn.Next();
                return res;
        }
        Number* p::expectNumber(){
                DBG_TRACE();
                auto res = new Number(tkn.Last());
                tkn.Next();
                return res;
        }
        Expression* p::expectExpression(){
                DBG_TRACE();
                auto lhs = expectPrimary();
                if(!lhs){
                        return nullptr;
                }
                return expectBinary(0,lhs);
        }
        Expression* p::expectUnary(){
                DBG_TRACE();
                auto op = tkn.Last();
                int64_t prec;
                if((prec=op_prec(L"unary$"+op.str))<=0){
                        setError(L"Operator expected",op);
                        return nullptr;
                }
                tkn.Next();
                auto lhs = expectPrimary();
                if(!lhs){
                        return nullptr;
                }
                return expectBinary(prec,lhs);
        }
        Expression* p::expectBinary(int64_t prec, Expression* lhs){
                DBG_TRACE();
                while (true) {
                        DBG_TRACE("%s pos:%lu", "in loop", tkn.Last().line);
                        int64_t opprec = op_prec(tkn.Last().str);    
                        if (opprec < prec){
                                return lhs;
                        }
                        auto binOp = tkn.Last();
                        tkn.Next();
                        if(tkn.eof()){
                                lhs->FullRelease();
                                return nullptr;
                        }
                        Expression* rhs = expectPrimary();
                        if (!rhs){
                                lhs->FullRelease();
                                return nullptr;
                        }
                        int64_t nextprec = op_prec(tkn.Last().str);
                        if (opprec < nextprec) {
                                rhs = expectBinary(opprec, rhs);
                                if (!rhs){ 
                                        lhs->FullRelease();
                                        return nullptr;
                                }
                        }
                        lhs = new BinOp(binOp, lhs,rhs);
                }
        }
        Variable* p::expectVariable(){
                DBG_TRACE();
                auto res = new Variable(tkn.Last());
                tkn.Next();
                return res;
        }
        Expression* p::expectPrimary(){
                DBG_TRACE();
                auto tok = tkn.Last();
                auto ty = tok.ty;
                auto sty = tok.sty;
                switch(ty){
                        case TokTy::Number: return expectNumber();
                        case TokTy::String: return expectString();
                        case TokTy::Identifer: return expectVariable();
                        case TokTy::Operator: {
                                switch(sty){
                                        case SubTokTy::LParen: return expectParen();
                                        //case SubTokTy::LSqParen LBlock;
                                        case SubTokTy::Quasi:{
                                                tkn.Next();
                                                auto tmp = expectStatement();
                                                if(!tmp){
                                                        return nullptr;
                                                }
                                                return new AstNode(tmp);
                                        }
                                        case SubTokTy::ExprNode:{
                                                tkn.Next();
                                                auto tmp = expectExpression();
                                                if(!tmp){
                                                        return nullptr;
                                                }
                                                return new AstNodeExpr(tmp);
                                        }
                                        case SubTokTy::Oper: return expectUnary();
                                        default: setError(L"Primary expected[0]",tok); return nullptr;
                                }
                        }
                        default: setError(L"Primary expected[1]",tok); return nullptr;
                }
        }
        Expression* p::expectParen(){
                DBG_TRACE();
                tkn.Next(); // eat (
                auto res = expectExpression();
                if(tkn.Last().sty!=SubTokTy::RParen){
                        if(tkn.Last().sty == SubTokTy::Comma){
                                std::vector<Expression*> vec;
                                vec.push_back(res);
                                while(tkn.Last().sty == SubTokTy::Comma){
                                        res = expectExpression();
                                        if(!res){
                                                for(auto&x:vec)x->FullRelease();
                                                return nullptr;
                                        }
                                        vec.push_back(res);
                                }
                                if(tkn.Last().sty!=SubTokTy::RParen){
                                        setError(L"LParen expected[0]", tkn.Last());
                                        for(auto&x:vec)x->FullRelease();
                                        return nullptr;
                                }
                                if(tkn.Next().sty == SubTokTy::Arrow){
                                        tkn.Next();
                                        auto body = expectExpression();
                                        if(!body){
                                                for(auto&x:vec)x->FullRelease();
                                                return nullptr;
                                        }
                                        std::vector<Variable*> varvec;
                                        for(auto&x:vec){
                                                if(x->type()!=NodeTy::Variable){
                                                        for(auto&x:vec)x->FullRelease();
                                                        setError(L"Variable exected",tkn.Last());
                                                        return nullptr;
                                                }
                                                varvec.push_back(dynamic_cast<Variable*>(x));
                                        }
                                        return new Lambda(new Expr2Stmt(body),new VecHelper<Variable>(varvec));
                                }
                                return new Tuple(new VecHelper<Expression>(vec));
                        }
                        setError(L"LParen expected[1]", tkn.Last());
                        res->FullRelease();
                        return nullptr;
                }
                tkn.Next();
                return res;
        }
        int64_t p::op_prec(const std::wstring& str){
                if(op_precedence.find(str)==op_precedence.end()){
                        return -1;
                }
                auto prec = op_precedence[str];
                return prec?prec:-1;
        }
        bool p::is_op(const std::wstring& str){ return op_prec(str)!=-1; }

        Statement* p::expectStatement(){
                DBG_TRACE();
                return new Expr2Stmt(expectExpression());
        }
        bool p::isSuccess(){ return !failed; }
        std::wstring p::ErrorMsg(){ if(isSuccess())return L"Ok!"; else return errMsg; }

        void p::Parse(){delete expectStatement();}

        void p::setError(const std::wstring& msg, const Token& info){
                errMsg = msg + L"[found:\'" + info.str + L"\'[ty:" + std::to_wstring((int)info.ty) + L"|sty:" + std::to_wstring((int)info.sty) + L"]] on line:" + std::to_wstring(info.line) + L" pos:" + std::to_wstring(info.pos);
                failed = true; 
        }
}