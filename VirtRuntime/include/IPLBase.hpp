#ifndef IPLBASE_HPP
#define IPLBASE_HPP

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>

//Useful Macros
#define MIN(a, b) ((a > b) ? b : a)
#define MAX(a, b) ((a > b) ? a : b)
#define STRINGFY_S(S) #S
#define STRINGFY(S) STRINGFY_S(S)
#define NullDelete(P) if(P != NULL) delete P

#if ENABLE_ASSERTS
#define Assert(S) if(!(S)){ printf("\033[0;31mIPLBase - Assertion failed (" STRINGFY(__FILE__) ": " STRINGFY(__LINE__) "): " STRINGFY(S) "\033[0m\n"); exit(1);}
#else
#define Assert(S)
#endif

namespace IPLBase{
//Useful Functions

inline bool isInt(const char *str){
    if(*str == '\0')
        return false;

    while(*str != '\0'){
        if(*str > '9' || *str < '0')
            return false;
        str++;
    }

    return true;
}

inline int intFromStr(const char *str){
    int result = 0;

    while(*str != '\0'){
        if(*str > '9' || *str < '0')
            return -1;
        result = result * 10 + (*str - '0');
        str++;
    }

    return result;
}

inline uint64_t intFromStr64(const char *str){
    uint64_t result = 0;

    while(*str != '\0'){
        if(*str > '9' || *str < '0')
            return 0;
        result = result * 10 + (*str - '0');
        str++;
    }

    return result;
}

inline std::vector<std::string> readFileLines(const std::string& filePath){
    std::vector<std::string> result;
    std::string aux;

    FILE *fp = fopen(filePath.c_str(), "r");
    if(!fp)
        return result;

    char c = fgetc(fp);
    while(!feof(fp)){
        if(c == '\n'){
            if(aux.size() > 0){
                result.push_back(aux);
                aux.clear();
            }
        }
        else{
            aux.push_back(c);
        }
        c = fgetc(fp);
    }
    if(aux.size() > 0)
        result.push_back(aux);

    fclose(fp);
    return result;
}

inline std::vector<std::string> split(const std::string& str, const char separator){
    std::vector<std::string> result;
    std::string aux;

    int i = 0;
    while(i < (int) str.size()){
        if(str[i] == separator){
            if(aux.size() > 0){
                result.push_back(aux);
                aux.clear();
            }
        }
        else{
            aux.push_back(str[i]);
        }
        i++;
    }
    if(aux.size() > 0)
        result.push_back(aux);

    return result;
}

inline std::vector<std::string> split(const std::string& str){
    return split(str, ' ');
}

inline std::string stripQuotes(const std::string& str){
    std::string result = str;
    while(result.size() > 0 && result.back() == '\"')
        result.pop_back();
    while(result.size() > 0 && result.front() == '\"')
        result.erase(result.begin());
    return result;
}

};

#endif