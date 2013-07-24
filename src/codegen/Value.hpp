#pragma once
#include <iostream>
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
        inline bool to_bool(const Value& val){
                if(!val.type){
                        return false;
                }
                if(val.type!=ValType::Boolean || val.boolv)
                        return true;
                return false;
        }
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
                if(to_bool(lhs)){
                        return rhs;
                }
                return Value(false);
        }
        inline Value orb(const Value& lhs, const Value&rhs, Context * ctx){
                if(to_bool(lhs)){
                        return lhs;
                }
                return rhs;
        }
        inline Value eq(const Value& lhs, const Value&rhs, Context * ctx){
                if(lhs.type!=rhs.type){
                        return Value(false);
                }
                if(lhs.boolv == rhs.boolv){
                        return Value(true);
                }
                return Value(false);
        }
        inline Value notb(const Value& lhs, Context * ctx){
                return Value(!to_bool(lhs));
        }

        inline Value less(const Value& lhs, const Value&rhs, Context * ctx){
                if(lhs.type!=rhs.type){
                        return Value(lhs.type < rhs.type);
                }
                if(lhs.type == ValType::String){
                        return Value(*lhs.str < *rhs.str);
                }
                if(lhs.type == ValType::Number){
                        return Value(lhs.num < rhs.num);
                }
                if(lhs.type == ValType::Boolean){
                        return Value(lhs.boolv < rhs.boolv);
                }
                std::wcerr << L"Attempt to compare uncompareable types\n";
                return Value(false);
        }
        inline Value great(const Value& lhs, const Value&rhs, Context * ctx){
                return less(rhs,lhs,ctx);
        }

        inline Value less_eq(const Value& lhs, const Value&rhs, Context * ctx){
                return Value(eq(lhs,rhs,ctx).boolv||less(lhs,rhs,ctx).boolv);
        }

        inline Value great_eq(const Value& lhs, const Value&rhs, Context * ctx){
                return less_eq(rhs,lhs,ctx);
        }
}