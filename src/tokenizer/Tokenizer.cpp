#include "Tokenizer.hpp"
#include <iostream>
namespace Z{
        namespace {
                inline bool is_num0     (wchar_t c){ return (c>=L'0' && c<=L'9'); }
                inline bool is_num1     (wchar_t c){ return is_num0(c) || (c == '.'); }
                inline bool is_letter0  (wchar_t c){ return (c==L'_') || (c==L'`') || (c>=L'a' && c<=L'z') || (c>=L'A' && c<=L'Z') || (c>=L'а' && c<=L'я') || (c>=L'А' && c<=L'Я'); }
                inline bool is_letter1  (wchar_t c){ return is_letter0(c) || is_num0(c); }
                inline bool is_str      (wchar_t c){ return (c==L'\'') || (c==L'\"'); }
                inline bool is_ws       (wchar_t c){ return (c==L'\t') ||(c==L'\n') ||(c==L'\r') ||(c==L' ');}
                inline bool is_op       (wchar_t c){ 
                        return (c==L'@') ||(c==L'%') || (c==L'~') || (c==L'!') || (c==L'{') || (c==L'}')
                                || (c==L']')|| (c==L'[') || (c==L'(')|| (c==L')')
                                || (c==L'^') ||(c==L':') ||(c==L'&') ||(c==L'*') || (c==L':')
                                || (c==L'-') ||(c==L'+') ||  (c==L'=') || (c==L'$') || (c == L'#')
                                || (c==L'/') || (c==L'?') || (c==L'>') || (c==L'<') 
                                || (c==L'.') || (c==L'|') || (c==L';') || (c==L'\\') || (c==L',');
                }
                template<typename K, typename V> bool contains(const K &k, const V & v){ return k.find(v)!=k.end(); }
        }

        Tokenizer::Tokenizer(const std::wstring & code):code(code){}
        Tokenizer::~Tokenizer(){}
        bool Tokenizer::eof()const { return _eof(); }
        Token& Tokenizer::Last()const{
                if(cache_pos){
                        return cache[cache_pos-1];
                }
                return __tokNone;
        }
        void Tokenizer::DefKw(const std::wstring& str,SubTokTy ty, bool op)const{
                _defkw(str, ty,op);
        }
        Token& Tokenizer::Next() const {
                if(eof())return __tokNone;
                if(cache_pos<cache.size()){
                        return cache[cache_pos++];
                }
                auto tok = _next();
                if(failed){
                        return this->__tokNone;
                }
                if(contains(keywords,tok.str)){
                        tok.sty = keywords[tok.str];
                }
                if(contains(ops,tok.str)){
                        tok.ty = TokTy::Operator; 
                }
                cache.push_back(tok);
                ++cache_pos;
                return cache.back();
        }
        Token& Tokenizer::Look(uint64_t num) const {
                for (int i = 0; i < num; ++i){
                        Next();
                }
                auto& res = cache.back();
                cache_pos-=num;
                return res;
        }
        void Tokenizer::RetTokens(uint64_t num) const {
                if(num>cache_pos){/*error*/}
                else cache_pos -= num;
        }
        bool Tokenizer::_eof() const { return this->position >= this->code.size(); }

        void Tokenizer::_trim() const {
                if(_eof()){
                        return;
                }
                wchar_t c;
                while(is_ws(c=_curChar()) && c){
                        if(c==L'\n'){
                                lineno+=1;
                        }
                        position+=1;
                }
                if(position+1 < code.size()){
                        if(_curChar() ==L'/' && code[position+1] == '/'){
                                while(!_eof() && _curChar()!=L'\n'){
                                        position+=1;
                                }
                                lineno+=1;
                                _nextChar();
                                if(!_eof()){
                                        _trim();
                                }
                        }
                }
        }

        wchar_t Tokenizer::_nextChar() const {
                if(_eof() || position+1 == code.size()){
                        return 0;
                }
                return code[++position];
        }
        wchar_t Tokenizer::_curChar() const {
                if(_eof()){
                        return 0;
                }
                return code[position];
        }
        void Tokenizer::reset() const {
                this->errMsg = L"";
                this->failed = false;
                this->lineno = 0;
                this->position = 0;
                this->cache.clear();
                this->cache_pos = 0;
                this->code.clear();
        }
        void Tokenizer::setCode(const std::wstring& code) const {
                this->code = code;
        } 
        Token Tokenizer::_next() const {
                _trim();
                if(_eof()){
                        return Token(TokTy::None,L"",lineno,position);
                }
                wchar_t c = _curChar();
                auto l =lineno,p =position;
                if(is_num0(c)){
                        return Token(TokTy::Number,_num(),l,p);
                }
                if(is_letter0(c)){
                        return Token(TokTy::Identifer,_id(),l,p);
                }
                if(is_str(c)){
                        return Token(TokTy::String,_str(),l,p);
                }
                if(is_op(c)){
                        return Token(TokTy::Operator,_op(),l,p);
                }
                return Token(TokTy::None,L"$error",l,p);

        }
        std::wstring Tokenizer::_num() const {
                std::wstring res; res = _curChar();
                wchar_t c;
                while((c=_nextChar()) && (is_num1(c)))res+=c;
                if(res.back() == L'.'){
                        position--;
                }
                return res;
        }
        std::wstring Tokenizer::_id() const {
                std::wstring res; res = _curChar();
                wchar_t c;
                while((c=_nextChar()) && (is_letter1(c)))res+=c;
                if(c==L'!' || c==L'?'){
                        res+=c;
                        _nextChar();
                }
                return res;
        }
        std::wstring Tokenizer::_str() const {
                std::wstring res; 
                wchar_t end = _curChar(); 
                wchar_t c;
                while(!eof() && (c=_nextChar()) && (c!=end)){
                        if(c=='\\'){
                                switch(_nextChar()){
                                        case 'n': c = '\n';break;
                                        case '\\': c ='\\';break;
                                        case 'r': c = '\r';break;
                                        case 't': c = '\t';break;
                                        case 'a': c = '\a';break;
                                        case 'b': c = '\b';break;
                                        case '$': c = '$'; break;
                                        case 'f': c = '\f';break;
                                        default:           break;
                                }
                        }
                        res+=c;
                }
                if(c!=end){
                        setError(L"End of string expected, found:\""+(res)+L"\"");
                }
                _nextChar(); // eat \'
                return res;
        }
        void Tokenizer::setError(const std::wstring& msg) const {
                errMsg = msg + L" on line:" + std::to_wstring(lineno) + L" position:"+std::to_wstring(position);
                failed = true; 
        }
        std::wstring Tokenizer::_op() const {
                std::wstring tmp; tmp = _curChar();
                auto _pos = position;
                wchar_t c;
                while(is_op(c=_nextChar())){
                        tmp+=c;
                }
                for(uint i = tmp.length();i>0;--i){
                        const std::wstring& res = tmp.substr(0,i);
                        if(contains(ops,res)){
                                position = _pos +i;
                                return res;
                        }
                }
                position = _pos+1;
                return tmp.substr(0,1);

        }
        void Tokenizer::_defkw(const std::wstring& str,SubTokTy ty, bool op) const {
                if(op){
                        ops.insert(str);
                }
                keywords[str] = ty;
        }
}