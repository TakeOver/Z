#pragma once
#include <string>
namespace Z{
        enum class ValType{
                Null = 0,
                Number,
                String,
                Boolean,
                Function,
                Expression
        };
        class Expression;
        class Variable;
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
                        uint64_t boolv;
                        struct {
                                Expression* expr;
                                Context * ctx;
                        };
                };
                Value():type(ValType::Null),expr(nullptr),ctx(nullptr){}
                Value(double num):type(ValType::Number),num(num){ctx=nullptr;}
                Value(uint64_t boolv):type(ValType::Boolean),boolv(boolv){ctx=nullptr;}
                Value(bool boolv):type(ValType::Boolean),boolv(boolv){ctx=nullptr;}
                Value(Function* fun):type(ValType::Function),fun(fun){ctx=nullptr;}
                Value(Expression* expr,Context * parent):type(ValType::Expression),expr(expr),ctx(parent){}
                Value(std::wstring* str):type(ValType::String),str(str){ctx=nullptr;}
        };
        inline Value add(const Value& lhs, const Value&rhs, Context * ctx){
                if(!lhs.type || !rhs.type){
                        return Value();
                }
                if(lhs.type != rhs.type){
                        return Value();
                }
                if(lhs.type == ValType::String){
                        return Value(new std::wstring(*lhs.str + *rhs.str));
                }
                if(lhs.type == ValType::Number){
                        return Value(lhs.num + rhs.num);
                }
                if(lhs.type == ValType::Boolean){
                        return Value(lhs.boolv || rhs.boolv);
                }
                return Value();
        }        
        inline Value sub(const Value& lhs, const Value&rhs, Context * ctx){
                if(lhs.type != rhs.type){
                        return Value();
                }
                if(lhs.type == ValType::Number){
                        return Value(lhs.num - rhs.num);
                }
                return Value();
        }        
        inline Value mul(const Value& lhs, const Value&rhs, Context * ctx){
                if(lhs.type != rhs.type){
                        return Value();
                }
                if(lhs.type == ValType::Number){
                        return Value(lhs.num * rhs.num);
                }
                if(lhs.type == ValType::Boolean){
                        return Value(lhs.boolv && rhs.boolv);
                }
                return Value();
        }        
        inline Value div(const Value& lhs, const Value&rhs, Context * ctx){
                if(lhs.type != rhs.type){
                        return Value();
                }
                if(lhs.type == ValType::Number){
                        return Value(lhs.num / rhs.num);
                }
                return Value();
        }  
        inline Value mod(const Value& lhs, const Value&rhs, Context * ctx){
                if(lhs.type != rhs.type){
                        return Value();
                }
                if(lhs.type == ValType::Number){
                        return Value(static_cast<double>(static_cast<int64_t>(lhs.num) % static_cast<int64_t>(rhs.num)));
                }
                return Value();
        }  
        inline Value andb(const Value& lhs, const Value&rhs, Context * ctx){
                if(!lhs.type || !rhs.type )
                        return Value(false);
                if(lhs.type == ValType::Boolean && !lhs.boolv){
                        return Value(false);
                }
                //else lhs == true
                return rhs;
        }
        inline Value orb(const Value& lhs, const Value&rhs, Context * ctx){
                if(!lhs.type){
                        return rhs;
                }
                if(lhs.type==ValType::Boolean && !lhs.boolv){
                        return rhs;
                }
                return lhs;
        }
}