#pragma once
#include <string>
namespace Z{
        enum class ValType{
                Null = 0,
                Number,
                String,
                Function,
                Expression,
                Statement
        };
        class Expression;
        class Variable;
        class Statement;
        class Context;
        template<typename> class VecHelper;
        struct Function{
                Context* ctx;
                VecHelper<Variable> *args;
                Expression *body;
                Function(Context* ctx, VecHelper<Variable> * args, Expression*body):ctx(ctx),args(args),body(body){}
        };
        struct Value{
                ValType type;
                union{
                        double num;
                        Function* fun;
                        std::wstring* str;
                        struct {
                                union{
                                        Expression* expr;
                                        Statement* stmt;
                                };
                                Context * ctx;
                        };
                };
                Value():type(ValType::Null),expr(nullptr),ctx(nullptr){}
                Value(double num):type(ValType::Number),num(num){ctx=nullptr;}
                Value(Function* fun):type(ValType::Function),fun(fun){ctx=nullptr;}
                Value(Expression* expr,Context * parent):type(ValType::Expression),expr(expr),ctx(parent){}
                Value(Statement* stmt,Context * parent):type(ValType::Statement),stmt(stmt),ctx(parent){}
                Value(std::wstring* str):type(ValType::String),str(str){ctx=nullptr;}
        };
}