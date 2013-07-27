#pragma once
#include <string>
#include <iostream>
#include "Value.hpp"
#include <unordered_map>
namespace Z{
        class Context
        {
                long refcnt = 1;
                Context * parent;
                std::unordered_map<std::wstring, Value> env;
                bool _is_try = false;
                std::unordered_map<std::wstring, Value> *imported_modules;
        public:
                Value null;
                Context(Context * parent);
                Context();
                decltype(env)& getEnv();
                void SetTry();
                bool deleteVar(const std::wstring&);
                bool is_imported(const std::wstring & );
                Value RaiseException(const Value& excep);
                ~Context();
                void setModuleValue(const std::wstring&str,Value val);
                Value module_value(const std::wstring &);
                void Release();
                Context* getRoot();
                Value& getVar(const std::wstring & name);
                bool setVar(const std::wstring &name, const Value& val);
                void createVar(const std::wstring &name);
                void AddRef();
        };
}