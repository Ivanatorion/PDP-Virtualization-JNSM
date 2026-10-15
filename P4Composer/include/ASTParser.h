#ifndef AST_PARSER_H
#define AST_PARSER_H

class ASTParser {
    public:
        static void CheckParsersStates(AST *parser, const std::string& programName);
        static void WriteParserDOT(AST *parser, const std::string& fileName);
        static void MergeParserEqualStates(AST *stateList, std::vector<std::vector<int>>& statePrograms);
        static bool MergeParserSemiEqualStates(AST *stateList, std::vector<std::vector<int>>& statePrograms, const std::vector<std::vector<std::pair<int, int>>>& continuousPorts, AST* IG_PORT_EXP, std::vector<AST*> valueSetDeclarations);
        
    private:
        static void OrderParserStates(AST *StateList, const std::string& parserName, const std::string& programName);
        static bool BothOrNoneStart(AST* state, AST* state2);
};

#endif