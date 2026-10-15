#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

enum PreProcessorVerbosity {PPV_NONE = 0, PPV_SIMPLE = 1, PPV_ALL = 2};

typedef struct _define_macro{
    std::string name;
    std::vector<std::string> argNames;
    std::string body;

    size_t startline;
    size_t endline; //undef
} PPDefineMacro;

class PreProcessor {
    public:
        PreProcessor(const std::string& filePath, const std::vector<std::string>& p4includePaths, const std::vector<std::string>& defines, PreProcessorVerbosity verbosity);

        void Process();
        void ProcessStripComments();
        void Print(FILE *out) const;
        std::string PrintString() const;

        void SetVerbosity(PreProcessorVerbosity ppv);

        P4Architecture getArch() const;

        std::vector<std::string> getCommonIncludeList() const;
    
    private:
        P4Architecture m_p4architecture;
        std::string m_filePath;
        std::vector<std::string> m_fileLines;
        std::vector<std::string> m_p4includePaths;
        std::map<std::string, PPDefineMacro> m_defineMacros;
        std::vector<std::string> m_commonIncludeList;        

        int m_verboseLevel;

        static std::string Stringfy(const std::string &str);
        static size_t FindDefineArgInStr(const std::string& arg, const std::string& str, size_t start);

        void ProcessInclude(const int line, const std::string& includeFile, std::vector<std::string>* currentFilePathStack);
        void ProcessDefine(const int line, const std::vector<std::string>& stringSplit);

        void ReplaceDefine(const int startLine, const int nLines, const PPDefineMacro& ppdm);
        void ReplaceDefine(const int startLine, const PPDefineMacro& ppdm);

        bool EvalIfStringSplit(const std::vector<std::string>& stringSplit);

        static void PrintDefineMacro(const PPDefineMacro& ppdm);
        static std::vector<std::string> readFileLines(const std::string& filePath);

        static int RemoveCommonIncludeDeclarationsREC(AST *program, const std::unordered_map<NodeType,std::unordered_map<std::string, int>>& DeleteMap);

        static void RemoveCommomErrorMKDeclarations(AST *program, const std::vector<std::string>& includes);
};

inline P4Architecture PreProcessor::getArch() const { return this->m_p4architecture; }
inline std::vector<std::string> PreProcessor::getCommonIncludeList() const { return this->m_commonIncludeList; }

#endif