#include "Parser.hpp"
namespace Z{
        namespace {using p = Parser;}
        void p::_initTokenizer(){
                #define def(q,w) tkn.DefKw(L## #q,SubTokTy:: w)
                #define defop(q,w,...) defop(L## #q,w,##__VA_ARGS__)
                #define tok(ty) Token(TokTy::None,L## #ty,0,0,ty)
                def($::,Quasi);
                def(expr,ExprNode);
                def(@::, ExprNode);
                def(stmt, Quasi);
                defop(+,150);
                defop(-,150);
                defop(=,150);
                defop(==,150);
                defop(!=,150);
                defop(>=,150);
                defop(<=,150);
                defop(>,150);
                defop(<,150);
                defop(/,150);
                defop(*,150);
                defop(!,150);
                defop(#,150);
                defop(%,150);
                defop(^,150);
                defop(&,150);
                defop(&&,150);
                defop(|,150);
                defop(||,150);
                defop(~,150);
                defop([,150,SubTokTy::LSqParen,tok(SubTokTy::RSqParen));
                def(],RSqParen);
                defop({,150,SubTokTy::LBlock,tok(SubTokTy::RBlock));
                def(},RBlock);
                defop(L"(",150, SubTokTy::LParen,tok(SubTokTy::RParen));
                def(L")", RParen);
                #undef def
                #undef tok
                #undef defop
        }
        void p::defop(const std::wstring& str, int64_t prec, SubTokTy sty, Token follows){
                tkn.DefKw(str, sty,true);
                op_precedence[str] = prec;
                if(follows.sty!=SubTokTy::None){
                        follows_oper[sty] = follows;
                }
        }
        String* p::expectString(){
                auto res = new String(tkn.Last());
                tkn.Next();
                return res;
        }
        Number* p::expectNumber(){
                auto res = new Number(tkn.Last());
                tkn.Next();
                return res;
        }
        Expression* p::expectExpression(){
                auto lhs = expectParen();
                if(!lhs){
                        return nullptr;
                }
                return expectBinary(0,lhs);
        }
        Expression* p::expectUnary(){
                auto op = tkn.Last();
                int64_t prec;
                if((prec=op_prec(op.str))<=0){
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
                while (true) {
                        int64_t opprec = op_prec(tkn.Last().str);    
                        if (opprec < prec){
                                return lhs;
                        }
                        auto binOp = tkn.Last();
                        tkn.Next();
                        Expression* rhs = expectPrimary();
                        if (!rhs){
                                lhs->FullRelease();
                                return nullptr;
                        }
                        int64_t nextprec = op_prec(tkn.Last().str);
                        if (opprec < nextprec) {
                                rhs = expectBinary(opprec+1, rhs);
                                if (!rhs){ 
                                        lhs->FullRelease();
                                        return nullptr;
                                }
                        }
                        lhs = new BinOp(binOp, lhs,rhs);
                }
        }
        Variable* p::expectVariable(){
                auto res = new Variable(tkn.Last());
                tkn.Next();
                return res;
        }
        Expression* p::expectPrimary(){
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
                                                auto tmp = expectStatement();
                                                if(!tmp){
                                                        return nullptr;
                                                }
                                                return new AstNode(tmp);
                                        }
                                        case SubTokTy::ExprNode:{
                                                auto tmp = expectExpression();
                                                if(!tmp){
                                                        return nullptr;
                                                }
                                                return new AstNodeExpr(tmp);
                                        }
                                        default: setError(L"Primary expected",tok); return nullptr;
                                }
                        }
                        default: setError(L"Primary expected",tok); return nullptr;
                }
        }
        Expression* p::expectParen(){
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
                                        setError(L"LParen expected", tkn.Last());
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
                        setError(L"LParen expected", tkn.Last());
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
                return op_precedence[str];
        }
        bool p::is_op(const std::wstring& str){ return op_prec(str)!=-1; }
}