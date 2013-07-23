#include "Parser.hpp"
#include "../debug.hpp"
namespace Z{
        namespace {using p = Parser;}
        p::Parser(const std::wstring & code):tkn(code){_initTokenizer(); tkn.Next();}
        p::~Parser(){}
        void p::_initTokenizer(){
                #define tok(ty) Token(TokTy::None,L## #ty,0,0,ty)
                tkn.DefKw(L"expr!",SubTokTy::ExprNode,true);
                tkn.DefKw(L"${",SubTokTy::Quasi,true);
                defop(L"+",100);
                defop(L"-",100);
                defop(L"=",80);
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
                defop(L"and",70);
                defop(L"or",60);
                defop(L"!",150);
                defop(L"||",150);
                defop(L"~",150);
                tkn.DefKw(L",",SubTokTy::Comma,true);
                tkn.DefKw(L";",SubTokTy::Semicolon,true);
                tkn.DefKw(L"eval",SubTokTy::Eval,true);
                tkn.DefKw(L"export",SubTokTy::Export,true);
                tkn.DefKw(L"match",SubTokTy::Match,true);
                tkn.DefKw(L"true",SubTokTy::True,true);
                tkn.DefKw(L"false",SubTokTy::False,true);
                tkn.DefKw(L"nil",SubTokTy::Nil,true);
                tkn.DefKw(L"import",SubTokTy::Import,true);
                tkn.DefKw(L"let",SubTokTy::Let,true);
                tkn.DefKw(L"var",SubTokTy::Var,true);
                tkn.DefKw(L"->",SubTokTy::Arrow,true);
                tkn.DefKw(L"=>",SubTokTy::Arrow2,true);
                defop(L"[",150,SubTokTy::LSqParen,tok(SubTokTy::RSqParen));
                tkn.DefKw(L"]",SubTokTy::RSqParen);
                tkn.DefKw(L"{",SubTokTy::LBlock,true);
                tkn.DefKw(L"}",SubTokTy::RBlock);
                defop(L"(",150, SubTokTy::LParen,tok(SubTokTy::RParen));
                tkn.DefKw(L")", SubTokTy::RParen);
                #undef tok
                op_precedence[L"unary$-"] = 150;                
                op_precedence[L"unary$+"] = 150;              
                op_precedence[L"unary$!"] = 150;
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
        Expression* p::expectImport(){
                auto module = tkn.Next();
                tkn.Next();
                if(module.ty!=TokTy::Identifer){
                        setError(L"Indetifer expected in import",module);
                        return nullptr;
                }
                return new Import(module);
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
                lhs = expectBinary(prec,lhs);
                if(!lhs){
                        return nullptr;
                }
                return new UnOp(op,lhs);
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
                        Expression* rhs;

                        if(follows_oper.find(binOp.sty)!=follows_oper.end()){
                                if(binOp.sty == SubTokTy::LParen){
                                        std::vector<Expression*> tmp;
                                        while(!tkn.eof() && tkn.Last().sty!=SubTokTy::RParen){
                                                auto expr = expectExpression();
                                                if(!expr){
                                                        for(auto&x:tmp){
                                                                x->FullRelease();
                                                        }
                                                        return nullptr;
                                                }
                                                tmp.push_back(expr);

                                                if(tkn.Last().sty == SubTokTy::Comma){
                                                        tkn.Next();
                                                }else{
                                                        break;
                                                }
                                        }
                                        rhs = new VecHelper<Expression>(tmp);
                                }else{
                                        rhs = expectExpression();
                                }
                                auto next = follows_oper[binOp.sty];
                                if(tkn.Last().sty!=next.sty){
                                        rhs->FullRelease();
                                        setError(L"Expected suboper in binary expr(" + next.str + L")",tkn.Last());
                                        return nullptr;
                                }
                                tkn.Next();
                        }else{
                                rhs = expectPrimary();
                                if (!rhs){
                                        lhs->FullRelease();
                                        return nullptr;
                                }
                        }
                        int64_t nextprec = op_prec(tkn.Last().str);
                        if (opprec < nextprec) {
                                rhs = expectBinary(opprec, rhs);
                                if (!rhs){ 
                                        lhs->FullRelease();
                                        return nullptr;
                                }
                        }
                        if(binOp.sty == SubTokTy::LParen){
                                lhs = new FCall(lhs,dynamic_cast<VecHelper<Expression>*>(rhs));
                        }else{
                                lhs = new BinOp(binOp, lhs,rhs);
                        }
                }
        }
        Expression* p::expectVariable(bool lambda_possible){
                DBG_TRACE();
                auto res = new Variable(tkn.Last());
                if(tkn.Next().sty == SubTokTy::Arrow && lambda_possible){
                        DBG_TRACE("%s","lambda a->a parsing");
                        tkn.Next();
                        auto body = expectExpression();
                        if(!body){
                                delete res;
                                return nullptr;
                        }
                        return new Lambda(body,new VecHelper<Variable>({res}));
                }
                return res;
        }
        Expression* p::expectBlock(){
                tkn.Next();
                std::vector<Expression*> block;
                while(tkn.Last().sty!=SubTokTy::RBlock && !tkn.eof()){
                        Expression* expr = expectExpression();
                        if(!expr){
                                for(auto&x:block)x->FullRelease();
                                return nullptr;
                        }
                        block.push_back(expr);
                        if(tkn.Last().sty == SubTokTy::Semicolon){
                                tkn.Next();
                        }
                }
                if(tkn.Last().sty!=SubTokTy::RBlock){
                        for(auto&x:block)x->FullRelease();
                        setError(L"} expected in block",tkn.Last());
                        return nullptr;
                }
                tkn.Next();
                return new Block(new VecHelper<Expression>(block));
        }
        Expression* p::expectPrimary(bool lambda_possible){
                DBG_TRACE();
                auto tok = tkn.Last();
                auto ty = tok.ty;
                auto sty = tok.sty;
                switch(ty){
                        case TokTy::Number: return expectNumber();
                        case TokTy::String: return expectString();
                        case TokTy::Identifer: return expectVariable(lambda_possible);
                        case TokTy::Operator: {
                                switch(sty){
                                        case SubTokTy::LParen: return expectParen();
                                        //case SubTokTy::LSqParen LBlock;
                                        case SubTokTy::Match: return expectMatch();
                                        case SubTokTy::Quasi:{
                                                tkn.Next();
                                                auto tmp = expectExpression();
                                                if(!tmp){
                                                        return nullptr;
                                                }
                                                if(tkn.Last().sty!=SubTokTy::RBlock){
                                                        tmp->FullRelease();
                                                        setError(L"} expected in ast expr",tkn.Last());
                                                        return nullptr;
                                                }
                                                tkn.Next();
                                                return new AstNodeExpr(tmp);
                                        }
                                        case SubTokTy::ExprNode:{
                                                tkn.Next();
                                                auto tmp = expectExpression();
                                                if(!tmp){
                                                        return nullptr;
                                                }
                                                return new AstNodeExpr(tmp);
                                        }
                                        case SubTokTy::True: {tkn.Next(); return new Boolean(true);}
                                        case SubTokTy::False: {tkn.Next(); return new Boolean(false);}
                                        case SubTokTy::Nil: {tkn.Next(); return new Nil();}
                                        case SubTokTy::Eval:{
                                                tkn.Next();
                                                auto expr = expectExpression();
                                                if(!expr){
                                                        return nullptr;
                                                }
                                                return new EvalExpr(expr);
                                        }
                                        case SubTokTy::Export: {
                                                auto var = tkn.Next();
                                                if(var.ty==TokTy::Identifer && !var.sty){
                                                        return new Export(var);
                                                }
                                                setError(L"Identifer expected as export argument",var);
                                                return nullptr;
                                        }
                                        case SubTokTy::LBlock: return expectBlock();
                                        case SubTokTy::Oper: return expectUnary();
                                        case SubTokTy::Let: return expectLet();
                                        case SubTokTy::Var: return expectVar();
                                        case SubTokTy::Import: return expectImport();
                                        default: setError(L"Primary expected[0]",tok); return nullptr;
                                }
                        }
                        default: setError(L"Primary expected[1]",tok); return nullptr;
                }
        }
        Expression* p::expectLet(){
                auto name = tkn.Next();
                if(name.ty!=TokTy::Identifer){
                        setError(L"Identifer expected in let",name);
                        return nullptr;
                }
                if(tkn.Next().str!=L"="){
                        setError(L"Assign expected in let", tkn.Last());
                        return nullptr;
                }
                tkn.Next();
                auto value = expectExpression();
                if(!value){
                        return nullptr;
                }
                return new Let(name,value);
        }
        Expression* p::expectVar(){
                auto name = tkn.Next();
                if(name.ty!=TokTy::Identifer){
                        setError(L"Identifer expected in var",name);
                        return nullptr;
                }
                if(tkn.Next().str!=L"="){
                        return new Var(name,nullptr);
                }
                tkn.Next();
                auto value = expectExpression();
                if(!value){
                        return nullptr;
                }
                return new Var(name,value);
        }
        Expression* p::expectParen(){
                DBG_TRACE();
                tkn.Next(); // eat (
                auto res = expectExpression();
                if(tkn.Last().sty!=SubTokTy::RParen){ // tuple or lambda arrow notation
                        DBG_TRACE("%s","try lambda or tuple");
                        if(tkn.Last().sty == SubTokTy::Comma){
                                std::vector<Expression*> vec;
                                vec.push_back(res);
                                while(tkn.Last().sty == SubTokTy::Comma){
                                        tkn.Next();
                                        res = expectExpression();
                                        if(!res){
                                                for(auto&x:vec)x->FullRelease();
                                                return nullptr;
                                        }
                                        vec.push_back(res);
                                }
                                if(tkn.Last().sty!=SubTokTy::RParen){
                                        setError(L"RParen expected[0]", tkn.Last());
                                        for(auto&x:vec)x->FullRelease();
                                        return nullptr;
                                }
                                if(tkn.Next().sty == SubTokTy::Arrow){ // (a,b...)->expr
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
                                        return new Lambda((body),new VecHelper<Variable>(varvec));
                                }
                                setError(L"Arrow expected", tkn.Last());
                                for(auto&x:vec)x->FullRelease();
                                return nullptr;
                        }
                        setError(L"RParen expected[1]", tkn.Last());
                        res->FullRelease();
                        return nullptr;
                }
                if(tkn.Next().sty == SubTokTy::Arrow){ // lambda arrow notation. (a)-> expr
                        if(res->type()!=NodeTy::Variable){
                                res->FullRelease();
                                setError(L"Variable expected in lambda",tkn.Last());
                                return nullptr;
                        }
                        std::vector<Variable*> varvec;
                        varvec.push_back(dynamic_cast<Variable*>(res));
                        tkn.Next();
                        auto body = expectExpression();
                        if(!body){
                                res->FullRelease();
                                return nullptr;
                        }
                        return new Lambda((body),new VecHelper<Variable>(varvec));
                }
                return res; // simple (expr)
        }
        int64_t p::op_prec(const std::wstring& str){
                if(op_precedence.find(str)==op_precedence.end()){
                        return -1;
                }
                auto prec = op_precedence[str];
                return prec?prec:-1;
        }
        bool p::is_op(const std::wstring& str){ return op_prec(str)!=-1; }

        bool p::isSuccess(){ return !failed; }
        std::wstring p::ErrorMsg(){ if(isSuccess())return L"Ok!"; else return errMsg; }

        Expression* p::Parse(){return expectExpression();}

        void p::setError(const std::wstring& msg, const Token& info){
                errMsg = msg + L"[found:\'" + info.str + L"\'[ty:" + std::to_wstring((int)info.ty) + L"|sty:" + std::to_wstring((int)info.sty) + L"]] on line:" + std::to_wstring(info.line) + L" pos:" + std::to_wstring(info.pos);
                failed = true; 
        }
        Match* p::expectMatch(){
                tkn.Next();
                auto expr = expectExpression();
                if(!expr){
                        return nullptr;
                }
                if(tkn.Last().sty!=SubTokTy::LBlock){
                        expr->FullRelease();
                        setError(L"{ expected in match expr",tkn.Last());
                        return nullptr;
                }
                tkn.Next();
                std::vector<Expression*> cond;
                std::vector<Expression*> res;
                while(tkn.Last().sty!=SubTokTy::RBlock && !tkn.eof()){
                        auto _cond = expectExpression();
                        if(!_cond){
                                for(auto&x:cond)x->FullRelease();
                                for(auto&x:res)x->FullRelease();
                                expr->FullRelease();
                                return nullptr;
                        }
                        if(tkn.Last().sty!=SubTokTy::Arrow2){
                                for(auto&x:cond)x->FullRelease();
                                for(auto&x:res)x->FullRelease();
                                expr->FullRelease();
                                return nullptr;                                
                        }
                        tkn.Next();
                        auto _res = expectExpression();
                        if(!_res){
                                for(auto&x:cond)x->FullRelease();
                                for(auto&x:res)x->FullRelease();
                                expr->FullRelease();
                                _cond->FullRelease();
                                return nullptr;
                        }
                        cond.push_back(_cond);
                        res.push_back(_res);
                }
                if(tkn.Last().sty!=SubTokTy::RBlock){
                                for(auto&x:cond)x->FullRelease();
                                for(auto&x:res)x->FullRelease();
                                expr->FullRelease();
                                setError(L"} expected in match expr",tkn.Last());
                                return nullptr;                        
                }
                tkn.Next();
                return new Match((expr), new VecHelper<Expression>(cond), new VecHelper<Expression>(res));


        }
}