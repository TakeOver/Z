#include "Parser.hpp"
#include "../debug.hpp"
namespace Z{
        Parser::Parser(const std::wstring & code):tkn(code){_initTokenizer(); tkn.Next();}
        Parser::~Parser(){}
        void Parser::_initTokenizer(){
                #define tok(ty) Token(TokTy::None,L## #ty,0,0,ty)
                tkn.DefKw(L"${",SubTokTy::Quasi,true);
                tkn.DefKw(L"$",SubTokTy::Dollar,true);
                tkn.DefKw(L"#{",SubTokTy::HashKey,true);
                defop(L"+",120);
                defop(L"-",120);
                defop(L"=",20);
                defop(L"==",100);
                defop(L"!=",100);
                defop(L">=",100);
                defop(L"<=",100);
                defop(L">",100);
                defop(L"<",100);
                defop(L"/",130);
                defop(L"..",10);
                defop(L"*",130);
                defop(L"!",150);
                defop(L"#",150);
                defop(L"%",130);
                defop(L"^",70);
                defop(L"&&",50);
                defop(L".",160);
                defop(L"and",50);
                defop(L"or",40);
                defop(L"||",30);
                defop(L"~",150);
                is_right_assoc.insert(L"=");
                tkn.DefKw(L",",SubTokTy::Comma,true);
                tkn.DefKw(L";",SubTokTy::Semicolon,true);
                tkn.DefKw(L"class",SubTokTy::Class,true);
                tkn.DefKw(L"...",SubTokTy::Ellipsis,true);
                tkn.DefKw(L"delete",SubTokTy::Delete,true);
                tkn.DefKw(L"export",SubTokTy::Export,true);
                tkn.DefKw(L"macro",SubTokTy::Macro,true);
                tkn.DefKw(L"match",SubTokTy::Match,true);
                tkn.DefKw(L"as",SubTokTy::As,true);
                tkn.DefKw(L"if",SubTokTy::If,true);
                tkn.DefKw(L"else",SubTokTy::Else,true);
                tkn.DefKw(L"then",SubTokTy::Then,true);
                tkn.DefKw(L"_",SubTokTy::Placeholder,true);
                tkn.DefKw(L"for",SubTokTy::For,true);
                tkn.DefKw(L"while",SubTokTy::While,true);
                tkn.DefKw(L"cond",SubTokTy::Cond,true);
                tkn.DefKw(L"to",SubTokTy::To,true);
                tkn.DefKw(L"true",SubTokTy::True,true);
                tkn.DefKw(L"false",SubTokTy::False,true);
                tkn.DefKw(L"nil",SubTokTy::Nil,true);
                tkn.DefKw(L"import",SubTokTy::Import,true);
                tkn.DefKw(L"extends",SubTokTy::Extends,true);
                tkn.DefKw(L"let",SubTokTy::Let,true);
                tkn.DefKw(L"var",SubTokTy::Var,true);
                tkn.DefKw(L"->",SubTokTy::Arrow,true);
                tkn.DefKw(L"=>",SubTokTy::Arrow2,true);
                defop(L"[",160,SubTokTy::LSqParen,tok(SubTokTy::RSqParen));
                tkn.DefKw(L"]",SubTokTy::RSqParen);
                tkn.DefKw(L"{",SubTokTy::LBlock,true);
                tkn.DefKw(L"}",SubTokTy::RBlock);
                defop(L"(",160, SubTokTy::LParen,tok(SubTokTy::RParen));
                tkn.DefKw(L")", SubTokTy::RParen);
                #undef tok
                op_precedence[L"unary$-"] = 150;                
                op_precedence[L"unary$+"] = 150;              
                op_precedence[L"unary$!"] = 150;
        }
        Parser& Parser::reset(){
                failed = false;
                errMsg = L"";
                tkn.reset();
                return *this;
        }
        void Parser::setCode(const std::wstring& code){
                tkn.setCode(code);
                tkn.Next();
        }
        void Parser::defop(const std::wstring& str, int64_t prec, SubTokTy sty, Token follows){
                tkn.DefKw(str, sty,true);
                op_precedence[str] = prec;
                if(follows.sty!=SubTokTy::None){
                        follows_oper[sty] = follows;
                }
        }
        String* Parser::expectString(){
                DBG_TRACE();
                auto res = new String(new std::wstring(tkn.Last().str));
                tkn.Next();
                return res;
        }
        Number* Parser::expectNumber(){
                DBG_TRACE();
                auto res = new Number(std::wcstod(tkn.Last().str.c_str(),nullptr));
                tkn.Next();
                return res;
        }
        Expression* Parser::expectImport(){
                auto module = tkn.Next();
                tkn.Next();
                if(module.ty!=TokTy::Identifer){
                        setError(L"Identifer expected in import as module name",module);
                        return nullptr;
                }
                if( tkn.Last().sty == SubTokTy::As ){
                        if(tkn.Next().ty !=TokTy::Identifer){
                                setError(L"Identifer expected in import as 'as' argument",tkn.Last());
                                return nullptr;
                        }
                        auto var = tkn.Last();
                        tkn.Next();
                        return new Let(var,new Import(module,true));
                }
                return new Import(module);
        }
        Expression* Parser::expectExpression(){
                DBG_TRACE();
                auto lhs = expectPrimary();
                if(!lhs){
                        return nullptr;
                }
                return expectBinary(0,lhs);
        }
        Expression* Parser::expectUnary(){
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
                return new UnOp(op.str,lhs);
        }
        Expression* Parser::expectBinary(int64_t prec, Expression* lhs){
                DBG_TRACE();
                if(is_right_assoc.find(tkn.Last().str)!=is_right_assoc.end()){
                        auto op = tkn.Last();
                        tkn.Next();
                        auto rhs = expectExpression();
                        if(!rhs){
                                lhs->FullRelease();
                                return nullptr;
                        }
                        return new BinOp(op,lhs,rhs);
                }
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
                                setError(L"Unexpected eof in binary expression",tkn.Last());
                                return nullptr;
                        }
                        Expression* rhs;
                        if(binOp.str == L"."){
                                rhs = expectVariable(false);
                                if(!rhs){
                                        lhs->FullRelease();
                                        return nullptr;
                                }
                        }else if(follows_oper.find(binOp.sty)!=follows_oper.end()){
                                if(binOp.sty == SubTokTy::LParen){
                                        std::vector<Expression*> tmp;
                                        while(!tkn.eof() && tkn.Last().sty!=SubTokTy::RParen){
                                                bool is_ell = (tkn.Last().sty == SubTokTy::Ellipsis)?(tkn.Next(),true):false;
                                                auto expr = expectExpression();
                                                if(!expr){
                                                        for(auto&x:tmp){
                                                                x->FullRelease();
                                                        }
                                                        return nullptr;
                                                }
                                                tmp.push_back(is_ell?new Ellipsis(expr):expr);

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
                                rhs = expectBinary(opprec+1, rhs);
                                if (!rhs){ 
                                        lhs->FullRelease();
                                        return nullptr;
                                }
                        }
                        if(binOp.str == L"."){
                                String * str = new String(new std::wstring(dynamic_cast<Variable*>(rhs)->getname()));
                                delete rhs;
                                lhs = new BinOp(binOp, lhs, str);
                        }else if(binOp.sty == SubTokTy::LParen){
                                lhs = new FCall(lhs,dynamic_cast<VecHelper<Expression>*>(rhs));
                        }else{
                                lhs = new BinOp(binOp, lhs,rhs);
                        }
                }
        }
        Expression* Parser::expectVariable(bool lambda_possible){
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
                        return new Lambda(body,new VecHelper<Expression>({res}));
                }
                return res;
        }
        Expression* Parser::expectArray(){
                tkn.Next();
                std::vector<Expression*> vec;
                while(!tkn.eof() && tkn.Last().sty!=SubTokTy::RSqParen){
                        auto expr = expectExpression();
                        if(!expr){
                                expr->FullRelease();
                                return nullptr;
                        }
                        vec.push_back(expr);
                        if(tkn.Last().sty!=SubTokTy::Comma){
                                break;
                        }
                        tkn.Next();
                }
                if(tkn.Last().sty!=SubTokTy::RSqParen){
                        for(auto&x:vec){
                                x->FullRelease();
                        }
                        setError(L"] expected at end of array",tkn.Last());
                        return nullptr;
                }
                tkn.Next();
                return new ArrayAst(new VecHelper<Expression>(vec));
        }
        Expression* Parser::expectHash(){
                tkn.Next();
                std::vector<Expression*> keys;
                std::vector<Expression*> vals;
                while(!tkn.eof() && tkn.Last().sty!=SubTokTy::RBlock){
                        auto key = tkn.Last();
                        if(key.ty != TokTy::String && key.ty!=TokTy::Identifer){
                                for(auto&x:vals){
                                        x->FullRelease();
                                }
                                setError(L"Key(identifer or string) expected in hash",key);
                                return nullptr;       
                        }
                        tkn.Next();
                        keys.push_back(new String(new std::wstring(key.str)));
                        if(tkn.Last().str!= L"="){
                                for(auto&x:vals){
                                        x->FullRelease();
                                }
                                setError(L"Assign expected in hash",tkn.Last());
                                return nullptr;
                        }
                        tkn.Next();
                        auto expr = expectExpression();
                        if(!expr){
                                for(auto&x:vals){
                                        x->FullRelease();
                                }
                                return nullptr;
                        }
                        vals.push_back(expr);
                        if(tkn.Last().sty!=SubTokTy::Comma){
                                break;
                        }
                        tkn.Next();
                }
                if(tkn.Last().sty!=SubTokTy::RBlock){
                        for(auto&x:vals){
                                x->FullRelease();
                        }
                        setError(L"} expected at end of hash",tkn.Last());
                        return nullptr;
                }
                tkn.Next();
                return new HashAst(new VecHelper<Expression>(vals),new VecHelper<Expression>(keys));
        }
        Expression* Parser::expectBlock(){
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
        Expression* Parser::expectMacro(){
                std::vector<Expression*> args;
                Expression* body = nullptr;
                bool is_ellipsis = false;
                #define fail do{for(auto&x:args)delete x; if(body)body->FullRelease();}while(0) 
                #define fail_with(msg,tk) do{fail;setError(L##msg,tk); return nullptr;}while(0)
                if(tkn.Next().sty!=SubTokTy::LParen){
                        fail_with("LParen expected in macro definition", tkn.Last());
                }
                tkn.Next();
                while(!tkn.eof() && tkn.Last().sty!=SubTokTy::RParen){
                        auto var = expectVariable(false)->as<Variable>();
                        if(!var){
                                fail;
                                return nullptr;
                        }
                        args.push_back(var);
                        if(tkn.Last().sty==SubTokTy::Ellipsis){
                                is_ellipsis = true;
                                tkn.Next();
                                break;
                        }
                        if(tkn.Last().sty!=SubTokTy::Comma){
                                break;
                        }
                        tkn.Next();
                }
                if(tkn.Last().sty!=SubTokTy::RParen){
                        fail_with("RParen expected at end of macro args definition", tkn.Last());
                }
                if(tkn.Next().sty!=SubTokTy::Arrow){
                        fail_with("Arrow expected at macro definition", tkn.Last());
                }
                tkn.Next();
                body = expectExpression();
                if(!body){
                        fail;
                        return nullptr;
                }
                return new MacroAst(body, new VecHelper<Expression>(args),is_ellipsis);
                #undef fail
                #undef fail_with
        }
        Expression* Parser::expectFor(){
                tkn.Next();
                auto var = expectVariable(false);
                if(!var){
                        return nullptr;
                }
                if(tkn.Last().str!=L"="){
                        delete var;
                        setError(L"= expected in for-loop",tkn.Last());
                        return nullptr;
                }
                tkn.Next();
                auto from = expectExpression();
                if(!from){
                        delete var;
                        return nullptr;
                }
                if(tkn.Last().sty!=SubTokTy::To && tkn.Last().sty!=SubTokTy::Arrow2){
                        from->FullRelease();
                        delete var;
                        setError(L"=> or to expected as range in for-loop",tkn.Last());
                        return nullptr;
                }
                tkn.Next();
                auto to = expectExpression();
                if(!to){
                        delete var;
                        from->FullRelease();
                        return nullptr;
                }
                auto body = expectExpression();
                if(!body){
                        delete var;
                        from->FullRelease();
                        to->FullRelease();
                        return nullptr;
                }
                return new For(dynamic_cast<Variable*>(var),from,to,body);
        }
        Expression* Parser::expectPrimary(bool lambda_possible){
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
                                        case SubTokTy::Match: return expectMatch();
                                        case SubTokTy::Cond: return expectCond();
                                        case SubTokTy::HashKey: return expectHash();
                                        case SubTokTy::LSqParen: return expectArray();
                                        case SubTokTy::Macro: return expectMacro();
                                        case SubTokTy::Dollar: {
                                                auto tok = tkn.Next();
                                                if ( is_op(tok.str) ) {
                                                        tok.str = L"binary@" + tok.str;
                                                        tkn.Next();
                                                        return new Variable(tok);
                                                }

                                                setError(L"Not implemented($ - Dollar)",tok);
                                                return nullptr;
                                        }
                                        case SubTokTy::While:{
                                                tkn.Next();
                                                auto cond = expectExpression();
                                                if(!cond){
                                                        return nullptr;
                                                }
                                                auto body = expectExpression();
                                                if(!body){
                                                        cond->FullRelease();
                                                }
                                                return new While(cond,body);
                                        }
                                        case SubTokTy::Class: {
                                                if(tkn.Next().ty!=TokTy::Identifer){
                                                        setError(L"Identifer expected in class sugar syntax",tkn.Last());
                                                }
                                                auto name = tkn.Last();
                                                Variable* base = new Variable(Token(TokTy::Identifer,L"Object"));
                                                if(tkn.Next().sty == SubTokTy::Extends){
                                                        if(tkn.Next().ty!=TokTy::Identifer){
                                                                delete base;
                                                                setError(L"Identifer expected as inherit arguments in class sugar syntax",tkn.Last());
                                                                return nullptr;
                                                        }
                                                        base->name = tkn.Last();
                                                        tkn.Next();
                                                }
                                                auto body = expectHash();
                                                if(!body){
                                                        return nullptr;
                                                }
                                                return new Let(name,
                                                                new FCall(new BinOp(Token(TokTy::Operator,L".",0,0), 
                                                                          new Variable(Token(TokTy::Identifer,L"Class",0,0)),
                                                                          new String(new std::wstring(L"new"))),
                                                                new VecHelper<Expression>({base,new AstNodeExpr(body)})));
                                        }
                                        case SubTokTy::Delete: {
                                                tkn.Next();
                                                auto expr = expectExpression();
                                                if(!expr){
                                                        return nullptr;
                                                }
                                                return new Delete(expr);
                                        }
                                        case SubTokTy::For: return expectFor();
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
                                        case SubTokTy::True: {tkn.Next(); return new Boolean(true);}
                                        case SubTokTy::False: {tkn.Next(); return new Boolean(false);}
                                        case SubTokTy::Nil: {tkn.Next(); return new Nil();}
                                        case SubTokTy::Export: {
                                                auto var = tkn.Next();                                                        
                                                tkn.Next();
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
                                        case SubTokTy::If: {
                                                tkn.Next();
                                                auto cond = expectExpression();
                                                if(!cond){
                                                        return nullptr;
                                                }
                                                if(tkn.Last().sty == SubTokTy::Then){/*Optional, for now*/
                                                        tkn.Next();
                                                }
                                                auto iftrue = expectExpression();
                                                if(!iftrue){
                                                        cond->FullRelease();
                                                        return nullptr;
                                                }
                                                Expression* iffalse;
                                                if(tkn.Last().sty != SubTokTy::Else){/*optional, for now*/
                                                        iffalse = new Nil();
                                                }else{
                                                        tkn.Next();
                                                        iffalse = expectExpression();
                                                        if(!iffalse){
                                                                cond->FullRelease();
                                                                iftrue->FullRelease();
                                                                return nullptr;
                                                        }
                                                }
                                                return new Cond(new VecHelper<Expression>({cond,new Boolean(true)}),
                                                                new VecHelper<Expression>({iftrue,iffalse}));
                                        }
                                        case SubTokTy::Import: return expectImport();
                                        default: setError(L"Primary expected[0]",tok); return nullptr;
                                }
                        }
                        default: setError(L"Primary expected[1]",tok); return nullptr;
                }
        }
        Expression* Parser::expectLet(){
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
        Expression* Parser::expectVar(){
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
        Expression* Parser::expectParen(){
                DBG_TRACE();
                if(tkn.Next().sty == SubTokTy::RParen){
                        //lambda without arguments
                        if(tkn.Next().sty!=SubTokTy::Arrow){
                                setError(L"Arrow expected after lambda arg. list(no args)",tkn.Last());
                                return nullptr;
                        }
                        tkn.Next();
                        auto body = expectExpression();
                        if(!body){
                                return nullptr;
                        }
                        return new Lambda(body,new VecHelper<Expression>(new std::vector<Expression*>()));

                }
                bool is_ell = false;
                auto res = expectExpression();
                if(!res){
                        return nullptr;
                }
                if(tkn.Last().sty!=SubTokTy::RParen){ // tuple or lambda arrow notation
                        DBG_TRACE("%s","try lambda or tuple");
                        if(tkn.Last().sty == SubTokTy::Comma){
                                std::vector<Expression*> vec;
                                vec.push_back(res);
                                while(!tkn.eof() && tkn.Last().sty == SubTokTy::Comma){
                                        tkn.Next();
                                        res = expectExpression();
                                        if(!res){
                                                for(auto&x:vec){
                                                        x->FullRelease();
                                                }
                                                return nullptr;
                                        }
                                        vec.push_back(res);
                                        if(tkn.Last().sty == SubTokTy::Ellipsis){
                                                tkn.Next();
                                                is_ell = true;
                                                break;
                                        }
                                }
                                if(tkn.Last().sty!=SubTokTy::RParen){
                                        setError(L"RParen expected[0]", tkn.Last());
                                        for(auto&x:vec){
                                                x->FullRelease();
                                        }
                                        return nullptr;
                                }
                                if(tkn.Next().sty != SubTokTy::Arrow && is_ell){
                                        for(auto&x:vec){
                                                x->FullRelease();
                                        }
                                        setError(L"Arrow expected after lambda arg list(expected because of Ellipsis)",tkn.Last());
                                        return nullptr;
                                }
                                if(tkn.Last().sty == SubTokTy::Arrow){ // (a,b...)->expr
                                        tkn.Next();
                                        auto body = expectExpression();
                                        if(!body){
                                                for(auto&x:vec){
                                                        x->FullRelease();
                                                }
                                                return nullptr;
                                        }
                                        std::vector<Expression*> varvec;
                                        for(auto&x:vec){
                                                if(x->type()!=NodeTy::Variable){
                                                        for(auto&x:vec){
                                                                x->FullRelease();
                                                        }
                                                        setError(L"Variable exected",tkn.Last());
                                                        return nullptr;
                                                }
                                                varvec.push_back(dynamic_cast<Variable*>(x));
                                        }
                                        return new Lambda((body),new VecHelper<Expression>(varvec),is_ell);
                                }
                                setError(L"Arrow expected", tkn.Last());
                                for(auto&x:vec)x->FullRelease();
                                return nullptr;
                        }else if(tkn.Last().sty==SubTokTy::Ellipsis && res->type()==NodeTy::Variable){
                                if(tkn.Next().sty != SubTokTy::RParen){
                                        res->FullRelease();
                                        setError(L"Rparen expected[2]",tkn.Last());
                                        return nullptr;
                                }
                                if(tkn.Next().sty != SubTokTy::Arrow){
                                        res->FullRelease();
                                        setError(L"Arrow expected",tkn.Last());
                                        return nullptr;
                                }
                                tkn.Next();
                                auto body = expectExpression();
                                if(!body){
                                        res->FullRelease();
                                        return nullptr;
                                }
                                return new Lambda(body,new VecHelper<Expression>({res}),true);
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
                        std::vector<Expression*> varvec;
                        varvec.push_back(res);
                        tkn.Next();
                        auto body = expectExpression();
                        if(!body){
                                res->FullRelease();
                                return nullptr;
                        }
                        return new Lambda((body),new VecHelper<Expression>(varvec));
                }
                return res; // simple (expr)
        }
        int64_t Parser::op_prec(const std::wstring& str){
                if(op_precedence.find(str)==op_precedence.end()){
                        return -1;
                }
                auto prec = op_precedence[str];
                return prec?prec:-1;
        }
        bool Parser::is_op(const std::wstring& str){ return op_prec(str)!=-1; }

        bool Parser::isSuccess(){ return !failed; }
        std::wstring Parser::ErrorMsg(){ if(isSuccess())return L"Ok!"; else return errMsg; }

        Expression* Parser::Parse() { return expectExpression(); }

        void Parser::setError(const std::wstring& msg, const Token& info){
                errMsg = msg + L"[found:\'" + info.str + L"\'[ty:" + std::to_wstring((int)info.ty) + L"|sty:" + std::to_wstring((int)info.sty) + L"]] on line:" + std::to_wstring(info.line) + L" pos:" + std::to_wstring(info.pos);
                failed = true; 
        }
        Match* Parser::expectMatch(){
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
                Expression* deflt = new Nil();
                while(tkn.Last().sty!=SubTokTy::RBlock && !tkn.eof()){
                        bool is_deflt = false;
                        Expression* _cond;
                        if(tkn.Last().sty == SubTokTy::Placeholder){
                                is_deflt = true;
                                tkn.Next();
                        } else {
                                _cond = expectExpression();
                                if(!_cond){
                                        for(auto&x:cond)x->FullRelease();
                                        for(auto&x:res)x->FullRelease();
                                        expr->FullRelease();
                                        deflt->FullRelease();
                                        return nullptr;
                                }
                        }
                        if(tkn.Last().sty!=SubTokTy::Arrow2){
                                for(auto&x:cond)x->FullRelease();
                                for(auto&x:res)x->FullRelease();
                                expr->FullRelease();
                                deflt->FullRelease();
                                setError(L"=> Expected in match expression",tkn.Last());
                                return nullptr;                                
                        }
                        tkn.Next();
                        auto _res = expectExpression();
                        if(!_res){
                                for(auto&x:cond)x->FullRelease();
                                for(auto&x:res)x->FullRelease();
                                expr->FullRelease();
                                deflt->FullRelease();
                                if(!is_deflt){
                                        _cond->FullRelease();
                                }
                                return nullptr;
                        }
                        if(is_deflt){
                                deflt->FullRelease();
                                deflt = _res;
                        }else{
                                cond.push_back(_cond);
                                res.push_back(_res);
                        }
                        if(tkn.Last().sty == SubTokTy::Semicolon){
                                tkn.Next();
                        }
                }
                if(tkn.Last().sty!=SubTokTy::RBlock){
                                for(auto&x:cond)x->FullRelease();
                                for(auto&x:res)x->FullRelease();
                                expr->FullRelease();
                                deflt->FullRelease();
                                setError(L"} expected in match expr",tkn.Last());
                                return nullptr;                        
                }
                tkn.Next();
                return new Match((expr), new VecHelper<Expression>(cond), new VecHelper<Expression>(res),deflt);


        }
        Cond* Parser::expectCond(){
                tkn.Next();
                if(tkn.Last().sty!=SubTokTy::LBlock){
                        setError(L"{ expected in cond expr",tkn.Last());
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
                                return nullptr;
                        }
                        if(tkn.Last().sty!=SubTokTy::Arrow2){
                                for(auto&x:cond)x->FullRelease();
                                for(auto&x:res)x->FullRelease();
                                return nullptr;                                
                        }
                        tkn.Next();
                        auto _res = expectExpression();
                        if(!_res){
                                for(auto&x:cond)x->FullRelease();
                                for(auto&x:res)x->FullRelease();
                                _cond->FullRelease();
                                return nullptr;
                        }
                        cond.push_back(_cond);
                        res.push_back(_res);
                        if(tkn.Last().sty == SubTokTy::Semicolon){
                                tkn.Next();
                        }
                }
                if(tkn.Last().sty!=SubTokTy::RBlock){
                                for(auto&x:cond)x->FullRelease();
                                for(auto&x:res)x->FullRelease();
                                setError(L"} expected in cond expr",tkn.Last());
                                return nullptr;                        
                }
                tkn.Next();
                return new Cond(new VecHelper<Expression>(cond), new VecHelper<Expression>(res));


        }
}