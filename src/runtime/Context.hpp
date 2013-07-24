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
        public:
                Value null;
                Context(Context * parent);
                Context();
                void SetTry(){
                        _is_try = true;
                }
                Value RaiseException(const Value& excep);
                ~Context();
                void Release();
                Context* getRoot();
                Value& getVar(const std::wstring & name);
                bool setVar(const std::wstring &name, const Value& val);
                void createVar(const std::wstring &name);
                void AddRef();
        };
}