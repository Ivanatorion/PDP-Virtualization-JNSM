%{
#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../include/AST.h"
#include "parser.tab.h"

#define YYSTYPE LexVal
#define RET_TOK(TOKEN) if(!_IS_LINE_COMMENT && !_IS_OTHER_COMMENT) {/* printf("%s - ", yytext); for(auto c : parserTypeIdentifiers){printf("%s ", c.c_str());} for(auto v : parserTypeIdentifierStack)for(auto c : v){printf("%s ", c.c_str());} printf("\n"); */ if(TOKEN == TOKEN_ERROR) {printf("Token Error: %s\n", yytext);} return TOKEN;};

int _LINE_NUMBER_UNIQUE_VAR_NAME = 1;
int _IS_LINE_COMMENT = 0;
int _IS_OTHER_COMMENT = 0;

extern std::vector<std::pair<std::string, int>> includeStack;

extern std::unordered_set<std::string> parserTypeIdentifiers;
extern std::vector<std::unordered_set<std::string>> parserTypeIdentifierStack;

extern void CSClear();

std::string _lex_top_file_name;
std::string _lex_file_name;

int get_line_number();
std::string get_file_name();
void reset();

std::string hexToDec(char *str){
    uint64_t num = 0;
    str = str + 2;
    while(*str != '\0'){
        num = num * 16 + ((*str >= '0' && *str <= '9') ? (*str - '0') : (toupper(*str) - 'A' + 10));
        str++;
    }

    if(num == 0)
        return "0";

    std::string ret;
    
    while(num > 0){
        ret.push_back(num % 10 + '0');
        num = num / 10;
    }
    for(int i = 0; i < (int) ret.size() / 2; i++){
        char a = ret[i];
        ret[i] = ret[ret.size() - 1 - i];
        ret[ret.size() - 1 - i] = a;
    }

    return ret;
}

std::string binToDec(char *str){
    //printf("\033[0;31mBin: %s - ", str);

    int num = 0;
    str = str + 2;
    while(*str != '\0'){
        num = num * 2 + ((*str >= '0' && *str <= '1') ? (*str - '0') : 0);
        str++;
    }

    if(num == 0){
    //    printf("0\033[0m\n");
        return "0";
    }

    std::string ret;
    
    while(num > 0){
        ret.push_back(num % 10 + '0');
        num = num / 10;
    }
    for(int i = 0; i < (int) ret.size() / 2; i++){
        char a = ret[i];
        ret[i] = ret[ret.size() - 1 - i];
        ret[ret.size() - 1 - i] = a;
    }

    //printf("%s\033[0m\n", ret.c_str());

    return ret;
}

void setYyVal(){
    if(_IS_LINE_COMMENT || _IS_OTHER_COMMENT)
        return;

    yylval.lexval = new std::string(yytext);
    
    if(strlen(yytext) > 2 && *yytext == '0' && *(yytext+1) == 'x')
        *(yylval.lexval) = hexToDec(yytext);
    else if(strlen(yytext) > 2 && *yytext == '0' && *(yytext+1) == 'b')
        *(yylval.lexval) = binToDec(yytext);
}

bool isTypeidentifier(const std::string& id){
    if(parserTypeIdentifiers.find(id) != parserTypeIdentifiers.end())
        return true;
    for(auto v : parserTypeIdentifierStack)
        if(v.find(id) != v.end())
            return true;
    return false;
}

void PushInclude(){
    std::string str = std::string(strstr(yytext, "\"") + 1);
    str.pop_back();
    includeStack.push_back(std::make_pair(str, _LINE_NUMBER_UNIQUE_VAR_NAME));
    _LINE_NUMBER_UNIQUE_VAR_NAME = 0;

    _lex_file_name = str;
}

void PopInclude(){
    _LINE_NUMBER_UNIQUE_VAR_NAME = includeStack[includeStack.size() - 1].second;
    includeStack.pop_back();

    if(includeStack.size() == 0){
        _lex_file_name = _lex_top_file_name;
    }
    else{
        _lex_file_name = includeStack[includeStack.size() - 1].first;
    }
}

%}

especChar [\[,\],\,\;\:\(\)\{\}\+\-\|\?\@\*\/\<\>\=\!\&\%\#\^\.\$\~\`]
digit [0-9]
litInt {digit}+
litString \"(\\.|[^"\\])*\"
cExp [eE][-+]?{litInt}?

identificador [a-zA-Z_][a-zA-Z0-9_]*

%%

"__PUSH_INCLUDE__ "{litString} {PushInclude();}
"__POP_INCLUDE__ "{litString} {PopInclude();}

"typedef" { setYyVal(); RET_TOK(TYPEDEF); }
"header" { setYyVal(); RET_TOK(HEADER); }
"struct" { setYyVal(); RET_TOK(STRUCT); }
"package" { setYyVal(); RET_TOK(PACKAGE); }
"int" { setYyVal(); RET_TOK(INT); }
"bool" { setYyVal(); RET_TOK(BOOL); }
"error" { setYyVal(); RET_TOK(ERROR); }
"string" { setYyVal(); RET_TOK(STRING); }
"bit" { setYyVal(); RET_TOK(BIT); }
"varbit" { setYyVal(); RET_TOK(VARBIT); }
"control" { setYyVal(); RET_TOK(CONTROL); }
"parser" { setYyVal(); RET_TOK(PARSER); }
"apply" { setYyVal(); RET_TOK(APPLY); }
"if" { setYyVal(); RET_TOK(IF); }
"else" { setYyVal(); RET_TOK(ELSE); }
"in" { setYyVal(); RET_TOK(IN); }
"out" { setYyVal(); RET_TOK(OUT); }
"inout" { setYyVal(); RET_TOK(INOUT); }
"return" { setYyVal(); RET_TOK(RETURN); }
"key" { setYyVal(); RET_TOK(KEY); }
"const" { setYyVal(); RET_TOK(CONST); }
"action" { setYyVal(); RET_TOK(ACTION); }
"actions" { setYyVal(); RET_TOK(ACTIONS); }
"switch" { setYyVal(); RET_TOK(SWITCH); }
"select" { setYyVal(); RET_TOK(SELECT); }
"state" { setYyVal(); RET_TOK(STATE); }
"entries" { setYyVal(); RET_TOK(ENTRIES); }
"type" { setYyVal(); RET_TOK(TYPE); }
"table" { setYyVal(); RET_TOK(TABLE); }
"value_set" { setYyVal(); RET_TOK(VALUESET); }
"transition" { setYyVal(); RET_TOK(TRANSITION); }
"tuple" { setYyVal(); RET_TOK(TUPLE); }
"enum" { setYyVal(); RET_TOK(ENUM); }
"void" { setYyVal(); RET_TOK(VOID); }
"default" { setYyVal(); RET_TOK(DEFAULT); }
"&&&" { setYyVal(); RET_TOK(MASK); }
"match_kind" { setYyVal(); RET_TOK(MATCH_KIND); }
".." { setYyVal(); RET_TOK(RANGE); }
"extern" { setYyVal(); RET_TOK(EXTERN); }
"abstract" { setYyVal(); RET_TOK(ABSTRACT); }
"_" { setYyVal(); RET_TOK(DONTCARE); }

"<=" { setYyVal(); RET_TOK( TK_OC_LE ); }
">=" { setYyVal(); RET_TOK( TK_OC_GE ); }
"==" { setYyVal(); RET_TOK( TK_OC_EQ ); }
"!=" { setYyVal(); RET_TOK( TK_OC_NE ); }
"&&" { setYyVal(); RET_TOK( TK_OC_AND ); }
"||" { setYyVal(); RET_TOK( TK_OC_OR ); }
"++" { setYyVal(); RET_TOK( TK_OC_PLUSPLUS );}
"|+|" { setYyVal(); RET_TOK( TK_OC_SAT_ADD );}
"|-|" { setYyVal(); RET_TOK( TK_OC_SAT_SUB );}

"false" { setYyVal(); RET_TOK( FALSE ); }
"true" { setYyVal(); RET_TOK( TRUE ); }
{litInt} { setYyVal(); RET_TOK( INTEGER ); }
"0x"[a-fA-F0-9_]* { setYyVal(); RET_TOK( INTEGER ); }
"0b"[a-fA-F0-9_]* { setYyVal(); RET_TOK( INTEGER ); }

\'.*\' { RET_TOK( TOKEN_ERROR ); }
{litString} { setYyVal(); RET_TOK(STRING_LITERAL); }

{litInt}w{litInt} { yytext = strstr(yytext, "w") + 1; setYyVal(); RET_TOK( INTEGER ); }
{litInt}w"0x"[a-fA-F0-9_]* { yytext = strstr(yytext, "w") + 1; setYyVal(); RET_TOK( INTEGER ); }

{litInt}{identificador} { RET_TOK(TOKEN_ERROR); }

{especChar} { setYyVal(); if(!strcmp(yytext, "%")) { RET_TOK(MODULUS); } else { RET_TOK(*yytext); }}

{identificador} { setYyVal(); 
    if(!_IS_LINE_COMMENT && !_IS_OTHER_COMMENT){
        if (isTypeidentifier(yytext)){
            RET_TOK(TYPE_IDENTIFIER);
        }
        else{ 
            RET_TOK(IDENTIFIER);
        }
    }
}

\n {_LINE_NUMBER_UNIQUE_VAR_NAME++; _IS_LINE_COMMENT = 0;}

"/*" {_IS_OTHER_COMMENT = 1;}
"*/" {_IS_OTHER_COMMENT = 0;}
"//" {_IS_LINE_COMMENT = 1;}

" " { }
\t { }

. { RET_TOK( TOKEN_ERROR ); }

%%

int get_line_number(){
  return _LINE_NUMBER_UNIQUE_VAR_NAME;
}

std::string get_file_name(){
    return _lex_file_name;
}

void reset(){
    _lex_top_file_name = "";
    _lex_file_name = "";

    _LINE_NUMBER_UNIQUE_VAR_NAME = 1;
    _IS_LINE_COMMENT = 0;
    _IS_OTHER_COMMENT = 0;
    parserTypeIdentifiers.clear();
    parserTypeIdentifierStack.clear();
    CSClear();
}

void set_top_file_name(std::string str){
    _lex_top_file_name = str;
    _lex_file_name = str;
}