#include "Context.hpp"
#include "../ast/basic_node.hpp"
namespace Z{
        namespace { 
                void init(Context* ctx){
                        ctx->env = new std::unordered_map<std::wstring, Expression*>();                        
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

        void Context::MarkChilds(std::set<Collectable*>& marked) {
                if(marked.find(this)!=marked.end()){
                        return;
                }
                for(auto&x:*env){
                        if(marked.find((Collectable*)x.second)!=marked.end()){
                            continue;
                        }
                        marked.insert((Collectable*)x.second);
                        extern void __MarkChilds(decltype(marked),Expression*);
                                __MarkChilds(marked,x.second);
                }
                if(parent){
                        parent->MarkChilds(marked);
                }
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
        }
        /*
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
                        if(contains(immutable,key)){
                            return false;
                        }
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

        Context* Context::getRoot(){ return parent?parent->getRoot():this; }

        Expression*& Context::getVar(const std::wstring& key){
                if ( contains(*env,key) ) {
                        return env->find(key)->second;
                }
                if ( parent ) {
                        return parent->getVar(key);
                }
                return this->nil;
        }

        void Context::setImmutableState(const std::wstring& name){
                this->immutable.insert(name);
        }

        bool Context::setVar(const std::wstring& key, Expression* value){
                if ( contains(*env,key) ) {
                        if(contains(immutable,key)){
                            return false;
                        }
                        env->find(key)->second = value;
                        return true;
                }
                if(parent){
                        return parent->setVar(key,value);
                }
                return false;
        }

        void Context::createVar(const std::wstring& name){
                if(contains(immutable,name)){
                    return;
                }
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
                extern Expression* __alloc_native_function(native_fun_t);
                (*getRoot()->env)[key]= __alloc_native_function(func);
        }
}