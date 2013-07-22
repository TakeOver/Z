#pragma once
#include <string>
#include "Value.hpp"
#include <unordered_map>
namespace Z{
        class Context
        {
                long refcnt = 1;
                Context * parent;
                std::unordered_map<std::wstring, Value> env;
        public:
                Value null;
                Context(Context * parent = nullptr):parent(parent){}
                ~Context(){}
                void Release(){
                        if(--refcnt <= 0){
                                delete this;
                        }
                }
                Value& getVar(const std::wstring & name){
                        if(env.find(name)==env.end()){
                                if(!parent){
                                        return null;
                                }
                                return parent->getVar(name);
                        }
                        return env[name];
                }
                bool setVar(const std::wstring &name, const Value& val){
                        if(env.find(name)!=env.end()){
                                env[name]=val;
                                return true;
                        }
                        if(!parent){
                                return false;
                        }
                        if(!parent->setVar(name,val)){
                                return false;
                        }
                        return true;
                }
                void createVar(const std::wstring &name){
                        env[name]=null;
                }
        };
}