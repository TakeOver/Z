#include "Context.hpp"
namespace Z{
        namespace { using ctx = Context; }
                ctx::Context(Context * parent):parent(parent){
                        parent->AddRef();
                        this->imported_modules = parent->imported_modules;
                }
                ctx::Context():parent(nullptr){
                        this->imported_modules = new std::unordered_map<std::wstring, Value>();
                }

                Value ctx::RaiseException(const Value& excep){ // TODO IMPLEMENTATION
                        if(_is_try){
                                return Value();
                        }
                        std::wcerr << (*(excep.str)) << L'\n';
                        exit(0);
                }

                ctx::~Context(){
                        if(!parent){
                                delete this->imported_modules;
                        }
                }
                bool ctx::deleteVar(const std::wstring& name){
                        if(env.find(name)!=env.end()){
                                env.erase(name);
                                return true;
                        }
                        if(!parent){
                                return false;
                        }
                        return parent->deleteVar(name);
                }
                void ctx::setModuleValue(const std::wstring&str,Value val){
                        this->imported_modules->insert({str,val});
                }
                void ctx::SetTry(){
                        _is_try = true;
                }
                bool ctx::is_imported(const std::wstring& name ){
                        return this->imported_modules->find(name)!=this->imported_modules->end();
                }
                Value ctx::module_value(const std::wstring & name){
                        return is_imported(name)?this->imported_modules->find(name)->second:null;
                }
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