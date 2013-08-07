#pragma once
#include <cstdint>
#include <vector>
#include "../runtime/Collectable.hpp"
#include "../runtime/Context.hpp"
namespace Z{
        #define ret_ty void
        #define inp_ty void
        enum class NodeTy{
                Node = 0,
                Expr,
                FCall,
                Func,
                Match,
                Lambda,
                BinOp,
                UnOp,
                VecHelper,
                Variable,
                String,
                Number,
                AstNodeExpr,
                Block,
                Let,
                Var,
                NativeFunction,
                Import,
                Delete,
                Boolean,
                Nil,
                Function,
                Export,
                Cond,
                Array,
                Hash,
                While,
                For,
                Ellipsis,
                AstNode,
                ArrayAst,
                HashAst
        };
        class Expression: public virtual Collectable {
        public:
                virtual ~Expression(){}
                virtual ret_ty emit(inp_ty) = 0;
                virtual Expression* eval(Context*) = 0;
                virtual NodeTy type() = 0; 
                virtual void FullRelease() { delete this; }
                template<typename T> T* as(){ return dynamic_cast<T*>(this); }
        };
        template<typename K> class VecHelper: public virtual Expression{
                uint64_t _reg = 0;
                std::vector<K*> *container; // cannot mutate via ast modifers.
        public:
                ~VecHelper() override { 
                        delete container;
                }
                VecHelper(const std::vector<K*>& v):container(new std::vector<K*>(v)){}
                VecHelper(std::vector<K*>* v):container(v){}
                virtual ret_ty emit(inp_ty) override {}
                virtual Expression* eval(Context* ctx){ return ctx->nil; };
                virtual NodeTy type() override { return NodeTy::VecHelper; }
                void FullRelease() override {
                        for(auto&x:*container){
                                x->FullRelease();
                        }
                        delete this;
                }
                void MarkChilds(std::set<Collectable*> & marked) override {
                        marked.insert(this);
                        for(auto&x:*container){
                                if(contains(marked,x)){
                                        continue;
                                }
                                x->MarkChilds(marked);
                        }
                }
                std::vector<K*>& get(){ return *container; }
        };
}