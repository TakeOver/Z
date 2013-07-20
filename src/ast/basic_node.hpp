#pragma once
#include <cstdint>
#include <vector>
namespace Z{
        #define ret_ty void
        #define inp_ty void
        enum class NodeTy{
                Node = 0,
                Expr,
                Stmt,
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
                Oper,
                AstNodeExpr,
                AstNode,
                Expr2Stmt
        };
        class Statement {
        public:
                virtual ~Statement(){}
                virtual ret_ty emit(inp_ty) = 0;
                virtual NodeTy type() = 0; 
                virtual void FullRelease() { delete this; }
        };

        class Expression {
        public:
                virtual ~Expression(){}
                virtual ret_ty emit(inp_ty) = 0;
                virtual uint64_t reg() = 0;
                virtual NodeTy type() = 0; 
                virtual void FullRelease() { delete this; }
        };
        class Expr2Stmt : public virtual Statement {
                Expression* expr;
        public:
                ~Expr2Stmt() override { /*if(expr)expr->FullRelease();*/ }
                Expr2Stmt(Expression * expr):expr(expr){}
                virtual ret_ty emit(inp_ty) override {expr->emit();}
                virtual NodeTy type() override { return NodeTy::Expr2Stmt; }

        };
        template<typename K> class VecHelper: public virtual Expression{
                uint64_t _reg = 0;
                std::vector<K*> container;
        public:
                ~VecHelper() override { for(auto&x:container)delete x; }
                virtual uint64_t reg()override{ return _reg; }
                VecHelper(const std::vector<K*>& v):container(v){}
                virtual ret_ty emit(inp_ty) override {}
                virtual NodeTy type() override { return NodeTy::VecHelper; }
                std::vector<K*>& get(){return container;}
        };
}