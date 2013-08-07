#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
namespace Z{
        class Expression;
        class Context;
        typedef Expression* (*native_fun_t)(Context*,const std::vector<Expression*> & args);
        class Context
        {
                long refcnt = 1;
                Context * parent;
                std::unordered_map<std::wstring, Expression*> *imported_modules;
                std::unordered_map<std::wstring,native_fun_t> * builtin_ops;
                std::unordered_set<std::wstring> immutable;
        public:
                void setImmutableState(const std::wstring&);
                std::unordered_map<std::wstring, Expression*> *env;
                Expression* nil;
                Context(Context * parent);
                Context(Expression*);
                bool deleteVar(const std::wstring&);
                bool isImported(const std::wstring & );
                ~Context();
                void setModuleValue(const std::wstring&str,Expression* val);
                Expression* moduleValue(const std::wstring &);
                //void release();
                Context* getRoot();
                Expression*& getVar(const std::wstring & name);
                bool setVar(const std::wstring &name, Expression* val);
                void createVar(const std::wstring &name);
               // void addRef();
                void defBuiltinOp(const std::wstring&,native_fun_t);
                native_fun_t findBuiltinOp(const std::wstring&);
        };
}