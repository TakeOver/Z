#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
namespace Z{
        enum class ValType{
                Null = 0,
                Number,
                String,
                Boolean,
                Function,
                Expression,
                Array,
                Hash,
                NativeFunction
        };
        class Expression;
        class Variable;
        class Context;
        template<typename> class VecHelper;
        struct Value;
        typedef Value (*native_func_t)(Context*,const std::vector<Value>&);
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
                        native_func_t native;
                        std::vector<Value> *arr;
                        std::unordered_map<std::wstring,Value> *hash;

                };
                Value():type(ValType::Null),expr(nullptr),ctx(nullptr){}
                Value(const Value &val) = default;
                Value(double num):type(ValType::Number),num(num){ctx=nullptr;}
                Value(native_func_t native):type(ValType::NativeFunction),native(native){ctx=nullptr;}
                Value(uint64_t boolv):type(ValType::Boolean),boolv(boolv){ctx=nullptr;}
                Value(bool boolv):type(ValType::Boolean),boolv(boolv){ctx=nullptr;}
                Value(Function* fun):type(ValType::Function),fun(fun){ctx=nullptr;}
                Value(Expression* expr,Context * parent):type(ValType::Expression),expr(expr),ctx(parent){}
                Value(std::wstring* str):type(ValType::String),str(str){ctx=nullptr;}
                Value(std::vector<Value>* arr):type(ValType::Array),arr(arr){ctx=nullptr;}
                Value(std::unordered_map<std::wstring,Value>* hash):type(ValType::Hash),hash(hash){ctx=nullptr;}
        };
        inline void print(const Value& val, std::wostream & out = std::wcout){
                if(val.type == ValType::Number){
                        out << val.num;
                } else if(val.type == ValType::Boolean){
                        out << (val.boolv?L"true":L"false");
                } else if(val.type == ValType::String){
                        out << *val.str;
                } else if(val.type == ValType::Null){
                        out << L"nil";
                } else if(val.type == ValType::Function){
                        out << L"<function>";
                } else if(val.type == ValType::Expression){
                        out << L"<expression>";
                } else if(val.type == ValType::Array){
                        out << L"[";
                                auto i = 0u;
                                while(i < val.arr->size()){
                                        print((*val.arr)[i]);
                                        if(i+1!=val.arr->size()){
                                                out << L',';
                                        }
                                        ++i;
                                }
                                out << L']';
                } else if(val.type == ValType::Hash){
                        auto i = 1u;
                        out << L"#{";
                        for(auto&x:*val.hash){
                                out << L"\"" << x.first << L"\"=";
                                print(x.second);
                                if(i != (*val.hash).size()){
                                        out << L',';
                                }
                                ++i;
                        }
                        out << L"}";
                } else if(val.type == ValType::NativeFunction){
                        out << L"<native_function>";
                } else {
                        out << L"<unimplemented>";
                }
        }
        inline Value getIdx(const Value& val, int64_t idx, Context * ctx){
                if(val.type!=ValType::Array){
                        return Value();
                }
                if(std::abs(idx)>=val.arr->size()){
                        return Value();
                }
                if(idx<0){
                        idx = val.arr->size() + idx;
                }
                return (*val.arr)[idx];
        }
        inline void setIdx(const Value& val, int64_t idx, const Value& what, Context * ctx){
                if(val.type!=ValType::Array){
                        return;
                }
                if(std::abs(idx)>=val.arr->size()){
                        val.arr->resize(std::abs(idx)+1);
                }
                if(idx<0){
                        idx = val.arr->size() + idx;
                }
                (*val.arr)[idx] = what;
        }
        inline Value getKey(const Value& val, const std::wstring &key, Context * ctx){
                if(val.type!=ValType::Hash){
                        return Value();
                }
                return (*val.hash)[key];
        }
        inline void setKey(const Value& val, const std::wstring& key, const Value& what, Context * ctx){
                if(val.type!=ValType::Hash){
                        return;
                }
                (*val.hash)[key] = what;
        }
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