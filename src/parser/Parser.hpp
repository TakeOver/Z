#pragma once
#include "../tokenizer/Tokenizer.hpp"
#include "../ast/lang_nodes.hpp"
#include <map>
namespace Z{
        class Parser
        {
        public:
                Parser();
                ~Parser();
                
        private:
                mutable Tokenizer tkn;
                
                Expression*     expectExpression(); // +
                Number*         expectNumber(); // +
                Variable*       expectVariable(); // +
 //               Lambda*         expectLambda();
                String*         expectString(); // +
                Expression*     expectParen(); // +
                Expression*     expectUnary();  // +
                Expression*     expectBinary(int64_t prec, Expression*lhs); //+
                Expression*     expectPrimary(); // +
                Statement*      expectStatement();
                void            _initTokenizer();

                mutable bool failed = false;
                mutable std::wstring errMsg;
                void setError(const std::wstring& msg,const Token& tok);

                mutable std::unordered_map<std::wstring, uint64_t> op_precedence;

                bool is_op(const std::wstring&);
                int64_t op_prec(const std::wstring&);

                mutable std::map<SubTokTy, Token> follows_oper; // ( -> ) | [ -> ] and etc.

                void defop(const std::wstring& str, int64_t prec, SubTokTy sty = SubTokTy::Oper, Token follows = Token(TokTy::None,L""));
        };
}