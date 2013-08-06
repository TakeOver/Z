#include "Context.hpp"
#include "../ast/basic_node.hpp"
namespace Z{
        namespace { 
                void init(Context* ctx){
                        ctx->env = new std::unordered_map<std::wstring, Expression*>();                        
                }
                template<typename K,typename V> bool contains(K&k,V&v){
                        return k.find(v)!=k.end();
                } 
        }
        Context::Context(Expression* nil){
                init(this);
                this->nil = nil;
                imported_modules = new std::unordered_map<std::wstring, Expression*>();
                builtin_ops =      new std::unordered_map<std::wstring, native_fun_t>();
                nil              = nullptr;
                parent           = nullptr;
        }
        Context::Context(Context * parent){
                init(this);
             //   parent->addRef();
                this->parent = parent;
                this->nil = parent->nil;
                this->imported_modules = parent->imported_modules;
                this->builtin_ops = parent->builtin_ops;
        }
        Context::~Context(){
            /*    if(parent){
                        parent->release();
                }*/
                //env is collectable.
        }/*
        void Context::release(){
                if ( -- refcnt ) {
                        if(parent){
                                parent->release();
                        }
                        delete this;
                }
        }*/
        bool Context::deleteVar(const std::wstring& key){
                if (contains(*env,key)) {
                        env->erase(key);
                        return true;
                }
                if(!parent){
                        return false;
                }
                return parent->deleteVar(key);
        }
        bool Context::isImported(const std::wstring& key){
                return contains(*imported_modules,key);
        }
        void Context::setModuleValue(const std::wstring& key,Expression* value){
                (*this->imported_modules)[key]=value;
        }
        Expression* Context::moduleValue(const std::wstring& key){
                if(contains(*imported_modules,key)){
                        return imported_modules->find(key)->second;
                }
                return nil;
        }
        Context* Context::getRoot(){ return parent; }
        Expression*& Context::getVar(const std::wstring& key){
                if ( contains(*env,key) ) {
                        return env->find(key)->second;
                }
                if ( parent ) {
                        return parent->getVar(key);
                }
                return this->nil;
        }
        bool Context::setVar(const std::wstring& key, Expression* value){
                if ( contains(*env,key) ) {
                        env->find(key)->second = value;
                        return true;
                }
                if(parent){
                        return parent->setVar(key,value);
                }
                return false;
        }
        void Context::createVar(const std::wstring& name){
                env->insert({name,nil});
        }
       // void Context::addRef(){ ++ refcnt; }
        native_fun_t Context::findBuiltinOp(const std::wstring& key){
                if ( contains(*builtin_ops,key) ) {
                        return builtin_ops->find(key)->second;
                }
                return nullptr;
        }
        void Context::defBuiltinOp(const std::wstring& key,native_fun_t func){
                (*builtin_ops)[key]=func;
        }
}