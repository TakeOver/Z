#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "Token.hpp"
namespace Z{
        namespace { using HashMap = std::unordered_map<std::wstring,SubTokTy> ;}
        class Tokenizer
        {
        public:
                Tokenizer(const std::wstring &);
                ~Tokenizer();
                Token& Next() const;
                Token& Look(uint64_t num = 1) const;
                Token& Last() const;
                void   RetTokens(uint64_t num = 1) const;
                void DefKw(const std::wstring&,SubTokTy ty, bool op = false) const;
        private:
                const std::wstring code;
                mutable std::vector<Token> cache;
                mutable uint64_t cache_pos = 0;

                mutable HashMap keywords;
                mutable std::unordered_set<std::wstring> ops;
                mutable uint64_t max_op_length = 1;

                mutable uint64_t lineno   = 0;
                mutable uint64_t position = 0; 
                
                Token           _next() const;
                std::wstring    _num()  const;
                std::wstring    _str()  const;
                std::wstring    _op()   const;
                std::wstring    _id()   const;
                bool            _eof()  const;

                wchar_t _nextChar() const;
                wchar_t _curChar() const;

                void _trim() const;

                void _defkw(const std::wstring&,SubTokTy ty, bool op = false) const;

                mutable Token __tokNone = Token(TokTy::None,L"",0,0);
        };
}