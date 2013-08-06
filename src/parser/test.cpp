#include "Parser.hpp"
#include <iostream>
#include "../debug.hpp"
using namespace Z;
namespace Z{
Expression* Parse(const std::wstring& s){
        DBG_TRACE();
        auto res = Parser(s).Parse();
        return res;
}
extern void print(Expression*what){
        DBG_TRACE();
        what->emit();
}
}
Expression* show(Context* ctx, const std::vector<Expression*>& args){
        DBG_TRACE();
        for(auto&x:args){
                auto y = x->eval(ctx);
                if(y->type()==NodeTy::String){
                        std::wcout << *y->as<String>()->value;
                }else{
                        y->emit();
                }
        }
        return ctx->nil;
}
Expression* eval(Context* ctx, const std::vector<Expression*>& args){
        DBG_TRACE();
        if(args.size()!=1){
                return ctx->nil;
        }
        auto expr = args.front()->eval(ctx);
        auto ast = expr->as<AstNode>();
        if(ast){
                return ast->eval(ast->_ctx);
        }
        return expr;
}
Expression* input(Z::Context* ctx, const std::vector<Expression*>& args){
        DBG_TRACE();
        for(auto&x:args){
                Z::print(x->eval(ctx));
        }
        std::wstring* str = new std::wstring();
        std::getline(std::wcin,*str);
        return new String(str);
}
Expression* len(Z::Context* ctx, const std::vector<Expression*>& args){
        DBG_TRACE();
        auto obj = args.front()->eval(ctx);
        switch(obj->type()){
                case NodeTy::String: return new Number(static_cast<double>(dynamic_cast<String*>(obj)->value->length()));
                case NodeTy::Array: return new Number(static_cast<double>(dynamic_cast<Array*>(obj)->value->size()));
                default: return new Number(0.0);
        }
}
Expression* append(Z::Context* ctx, const std::vector<Expression*>& args){
        DBG_TRACE();
        auto self = dynamic_cast<AstNode*>(args.front()->eval(ctx));
        if(!self){
            return ctx->nil;
        }
        if(self->expr->type()==NodeTy::HashAst){
                if(args.size()<3){
                        return ctx->nil;
                }
                auto key = args[1]->eval(ctx);
                auto value = args[2]->eval(ctx);
                auto hash = dynamic_cast<HashAst*>(self->expr);
                hash->keys->get().push_back(key);
                hash->arr->get().push_back(value);
        } else if(self->expr->type() == NodeTy::Block){
                if(args.size()<2){
                        return ctx->nil;
                }
                auto value = (args[1]->eval(ctx));
                dynamic_cast<Block*>(self->expr)->block->get().push_back(value);
        }
        return self;
}
Expression* set(Z::Context*ctx, const std::vector<Expression*>&args){
        DBG_TRACE();
        if(args.size()!=2){
                return ctx->nil;
        }
        auto var = args.front()->eval(ctx);
        auto what = args.back()->eval(ctx);
        if(var->type()!=NodeTy::AstNode){
                return ctx->nil;
        }
        if(dynamic_cast<AstNode*>(var)->expr->type()!=NodeTy::Variable){
                return ctx->nil;
        }
        dynamic_cast<AstNode*>(var)->_ctx->setVar(dynamic_cast<Variable*>(dynamic_cast<AstNode*>(var)->expr)->getname(),what);
        return what;
}
Expression* parse(Z::Context*ctx, const std::vector<Expression*>&args){
        DBG_TRACE();
        auto what = args.back();
        if(what->type()!=NodeTy::String){
                return ctx->nil;
        }
        Parser par (*dynamic_cast<String*>(what)->value);
        auto ast = par.Parse();
        if(!ast){
                return ctx->nil;
        }
        return new AstNode(ast,ctx);
}
bool to_bool(Expression* expr){
        if(expr->type()==NodeTy::Nil){
                return false;
        }
        if(expr->type()==NodeTy::Boolean){
                return expr->as<Boolean>()->value;
        }
        return true;
}
#define defop(name,op,T)\
        Expression* name(Z::Context* ctx, const std::vector<Expression*>&args){\
                if(args.size()!=2){\
                        return ctx->nil;\
                }\
                auto lhs = args.front()->eval(ctx)->as<T>(),\
                        rhs = args.back()->eval(ctx)->as<T>();\
                if(!lhs || !rhs){\
                        return ctx->nil;\
                }\
                return new T(lhs->value op rhs->value);\
        }
defop(add,+,Number);
defop(sub,-,Number);
defop(div,/,Number);
defop(mul,*,Number);
#undef defop
#define defop(name,op)\
        Expression* name(Z::Context* ctx, const std::vector<Expression*>&args){\
                if(args.size()!=2){\
                        return ctx->nil;\
                }\
                auto lhs = args.front()->eval(ctx),\
                        rhs = args.back()->eval(ctx);\
                if(lhs->type()!=rhs->type()){\
                        return new Boolean(int(lhs->type()) op int(rhs->type()));\
                }\
                if(lhs->type() == NodeTy::Number){\
                        return new Boolean(lhs->as<Number>()->value op rhs->as<Number>()->value);\
                }\
                if(lhs->type() == NodeTy::Boolean){\
                        return new Boolean(lhs->as<Boolean>()->value op rhs->as<Boolean>()->value);\
                }\
                if(lhs->type() == NodeTy::String){\
                        return new Boolean(*lhs->as<String>()->value op *rhs->as<String>()->value);    \
                }\
                return new Boolean(lhs op rhs);\
        }
defop(less,<);
defop(less_eq,<=);
defop(great,>);
defop(great_eq,>=);
defop(equal,==);
defop(nonequal,!=);
#undef defop
Expression* _and(Z::Context* ctx, const std::vector<Expression*>&args){
        if(args.size()!=2){
                return ctx->nil;
        }
        auto lhs = args.front()->eval(ctx),
                rhs = args.back()->eval(ctx);
        if(!::to_bool(lhs)){
                return lhs;
        }
        return rhs;
}
Expression* _or(Z::Context* ctx, const std::vector<Expression*>&args){
        if(args.size()!=2){
                return ctx->nil;
        }
        auto lhs = args.front()->eval(ctx),
                rhs = args.back()->eval(ctx);
        if(!::to_bool(lhs)){
                return rhs;
        }
        return lhs;
}
Expression* _not(Z::Context* ctx, const std::vector<Expression*>&args){
        if(args.size()!=1){
                return ctx->nil;
        }
        auto lhs = args.front()->eval(ctx);
        return new Boolean(!::to_bool(lhs));
}
Expression* _index(Z::Context* ctx, const std::vector<Expression*>&args){
        if(args.size()!=2){
                return ctx->nil;
        }
        auto obj = args.front()->eval(ctx),
                key = args.back()->eval(ctx);
        if(key->type()==Z::NodeTy::String){
                if(obj->type()!=Z::NodeTy::Hash){
                        obj = ctx->getVar(typeof_str(obj->type()));
                        if(obj->type()!=Z::NodeTy::Hash){
                                return ctx->nil;
                        }
                }
                return obj->as<Hash>()->get(ctx,*key->as<String>()->value);
        }
        if(key->type()!=Z::NodeTy::Number || obj->type()!=Z::NodeTy::Array){
                return ctx->nil;
        }
        return obj->as<Array>()->get(ctx,std::floor(key->as<Number>()->value));

}
Expression* _assign(Z::Context* ctx, const std::vector<Expression*>&args){
        if(args.size()!=2){
                return ctx->nil;
        }
        auto lvalue = args.front(); // no eval!
        auto rvalue = args.back()->eval(ctx);
        switch(lvalue->type()){
                case NodeTy::Variable:{
                        ctx->setVar(lvalue->as<Variable>()->getname(), rvalue);
                        return rvalue;
                }
                case NodeTy::BinOp:{
                        auto bin = lvalue->as<BinOp>();
                        if(bin->op == L"binary@." || bin->op == L"binary@["){
                                auto lhs = bin->lhs->eval(ctx), 
                                        rhs = bin->rhs->eval(ctx);
                                auto hash = lhs->as<Hash>();
                                auto keystr = rhs->as<String>();
                                if(hash && keystr){
                                        hash->set(ctx,*keystr->value,rvalue);
                                        return rvalue;
                                }
                                auto array = lhs->as<Array>();
                                auto index = rhs->as<Number>();
                                if(array && index){
                                        array->set(ctx,std::floor(index->value),rvalue);
                                        return rvalue;
                                }
                                return rvalue;
                        }
                        lvalue->eval(ctx); // like imperative langs;
                        return ctx->nil;                        
                }
                default: {
                        lvalue->eval(ctx);
                        return rvalue;
                }
        }

}
int main(){
        Context* ctx = new Context(new Nil());
        ctx->defBuiltinOp(L"binary@+",add);
        ctx->defBuiltinOp(L"binary@-",sub);
        ctx->defBuiltinOp(L"binary@/",div);
        ctx->defBuiltinOp(L"binary@*",mul);
        ctx->defBuiltinOp(L"binary@<",less);
        ctx->defBuiltinOp(L"binary@<=",less_eq);
        ctx->defBuiltinOp(L"binary@>",great);
        ctx->defBuiltinOp(L"binary@>=",great_eq);
        ctx->defBuiltinOp(L"binary@==",equal);
        ctx->defBuiltinOp(L"binary@!=",nonequal);
        ctx->defBuiltinOp(L"binary@and",_and);
        ctx->defBuiltinOp(L"binary@or",_or);
        ctx->defBuiltinOp(L"unary@not",_not);
        ctx->defBuiltinOp(L"binary@&&",_and);
        ctx->defBuiltinOp(L"binary@||",_or);
        ctx->defBuiltinOp(L"unary@!",_not);
        ctx->defBuiltinOp(L"binary@[",_index);
        ctx->defBuiltinOp(L"binary@.",_index);
        ctx->defBuiltinOp(L"binary@=",_assign);
        ctx->createVar(L"input");
        ctx->setVar(L"input",new NativeFunction(input)); 
        ctx->createVar(L"show!");
        ctx->setVar(L"show!",new NativeFunction(show)); 
        ctx->createVar(L"eval!");
        ctx->setVar(L"eval!",new NativeFunction(eval)); 
        ctx->createVar(L"set!");
        ctx->setVar(L"set!",new NativeFunction(set));
        ctx->createVar(L"parse!");
        ctx->setVar(L"parse!",new NativeFunction(parse));
        ctx->createVar(L"Native");
        ctx->setVar(L"Native",new Hash(new std::unordered_map<std::wstring, Expression*>({
        {L"ast",new Hash(new std::unordered_map<std::wstring, Expression*>({
                        {L"append",new NativeFunction(append)}}))
        },      {L"str",new Hash(new std::unordered_map<std::wstring,Expression*>({
                        {L"len",new NativeFunction(len)}}))}})));
        Parser par(L"import object; import extra;");
        std::vector<Z::Expression*> image; 
        image.push_back(par.Parse());
        image.front()->eval(ctx);
        std::wcout << L">>";
        while(true){
                std::wstring buf, tmp;
                while(true){
                        std::getline(std::wcin,tmp);
                        if(tmp == L";;"){
                                break;
                        }
                        buf+=tmp + L'\n';
                }
                tmp = L"";
                par.reset().setCode(buf);
                auto expr = par.Parse();
                if(!expr){
                        std::wcout << "[Error!]:" << par.ErrorMsg() << L'\n';
                }else{
                        image.push_back(expr);
                        std::wcout << L">>>>\t";
                        Z::print(expr->eval(ctx));
                        std::wcout << L"\n>>";
                }
        }

}
