#pragma once
#include <cstdint>
#include <string>
namespace Z{
        enum class TokTy {
                None = (short)0,
                Operator,
                String,
                Number,
                Identifer
        };
        enum class SubTokTy {
                None = (short)0,
                Semicolon,
                Arrow,
                Arrow2,
                LParen,
                RParen,
                LBlock,
                RBlock,
                RSqParen,
                LSqParen,
                Dot,
                As,
                Ellipsis,
                Class,
                EscapeStr,
                To,
                Use,
                Placeholder,
                Import,
                Var,
                Let,
                Def,
                Native,
                Module,
                QuMark,
                Dollar,
                ExprNode,
                Quasi,
                Oper,
                Comma,
                Match,
                Eval,
                True,
                False,
                Nil,
                Export,
                Cond,
                Show,
                Showln,
                If,
                Then,
                Else,
                HashKey,
                For,
                While
        };
        struct Token{
                mutable TokTy           ty = TokTy::None;
                mutable std::wstring      str;
                mutable uint64_t          line,
                                        pos;
                mutable SubTokTy        sty;
                
                Token(TokTy ty = TokTy::None, const std::wstring& str = L"", uint64_t line = 0, uint64_t pos = 0, SubTokTy sty = SubTokTy::None):ty(ty),str(str),line(line),pos(pos),sty(sty){}
                
                inline bool isNum()  const{ return ty == TokTy::Number; }
                inline bool isId()   const{ return ty == TokTy::Identifer; }
                inline bool isNone() const{ return ty == TokTy::None; }
                inline bool isOp()   const{ return ty == TokTy::Operator; }
                inline bool isStr()  const{ return ty == TokTy::String; }

                inline bool issub()  const{ return sty!=SubTokTy::None; }
        };
}