#ifndef IPLBASE_HPP
#define IPLBASE_HPP

#include <cstdlib>
#include <cstdio>

//Useful Macros
#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define STRINGFY_S(S) #S
#define STRINGFY(S) STRINGFY_S(S)
#define NullDelete(P) if(P != NULL) delete P

#if ENABLE_ASSERTS
#define Assert(S) if(!(S)){ printf("\033[0;31mIPLBase - Assertion failed (" STRINGFY(__FILE__) ": " STRINGFY(__LINE__) "): " STRINGFY(S) "\033[0m\n"); abort();}
#define Warn(S) printf("\033[0;33mIPLBase - Warning (" STRINGFY(__FILE__) ": " STRINGFY(__LINE__) "): " STRINGFY(S) "\033[0m\n")
#else
#define Assert(S)
#define Warn(S)
#endif

namespace IPLBase{
//Useful Functions
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
            return -1;
        result = result * 10 + (*str - '0');
        str++;
    }

    return result;
}

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

inline std::string fold(const std::vector<std::string>& str, const char separator){
    if(str.size() == 0)
        return "";
    
    std::string result = str[0];

    for(int i = 1; i < (int) str.size(); i++)
        result = result + separator + str[i];

    return result;
}

inline std::string fold(const std::vector<std::string>& str){
    return fold(str, ' ');
}

};

#endif