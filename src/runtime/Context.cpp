#include "Context.hpp"
namespace Z{
        namespace { using ctx = Context; }
                ctx::Context(Context * parent):parent(parent){
                        parent->AddRef();
                }
                ctx::Context():parent(nullptr){}

                Value ctx::RaiseException(const Value& excep){ // TODO IMPLEMENTATION
                        if(_is_try){
                                return Value();
                        }
                        std::wcerr << (*(excep.str)) << L'\n';
                        exit(0);
                }

                ctx::~Context(){}

                void ctx::Release(){
                        if(--refcnt <= 0){
                                if(parent){
                                        parent->Release();
                                }
                                delete this;
                        }
                }
                decltype(ctx::env)& ctx::getEnv(){ return env; }
                Context* ctx::getRoot(){
                        if(!parent){
                                return this;
                        }
                        return parent->getRoot();
                }
                Value& ctx::getVar(const std::wstring & name){
                        if(env.find(name)==env.end()){
                                if(!parent){
                                        return null;
                                }
                                return parent->getVar(name);
                        }
                        return env[name];
                }
                bool ctx::setVar(const std::wstring &name, const Value& val){
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
                void ctx::createVar(const std::wstring &name){
                        env[name]=null;
                }
                void ctx::AddRef(){ ++ refcnt; }
}