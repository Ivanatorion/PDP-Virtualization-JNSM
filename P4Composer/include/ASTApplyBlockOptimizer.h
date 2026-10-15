#ifndef AST_APPLY_BLOCK_OPTIMIZER_H
#define AST_APPLY_BLOCK_OPTIMIZER_H

enum OptimizerBoolVariableValue {
    OPTIMIZER_BOOL_VARIABLE_TRUE,
    OPTIMIZER_BOOL_VARIABLE_FALSE,
    OPTIMIZER_BOOL_VARIABLE_UNKNOWN
};

typedef struct {
    AST* statement;
    std::vector<AST*> nextStatements;
} OptimizerStatementSuccessors;

class ASTApplyBlockOptimizer {
    public:
        ASTApplyBlockOptimizer();
        ~ASTApplyBlockOptimizer();

        static std::string OBVVToString(OptimizerBoolVariableValue obvv);

        static void Optimize(AST *applyBlock);
        static void RemoveUnusedVariables(AST *ast);

    private:

        static void GetSucessorGraphRec(AST *block, std::vector<OptimizerStatementSuccessors> *successors, std::vector<size_t>* hangingIDXs);
        static std::vector<OptimizerStatementSuccessors> GetSucessorGraph(AST *block);

        static void OptimizeBlock(AST *block, std::unordered_map<std::string, OptimizerBoolVariableValue>* boolVariables);
        static void ASTRemoveUnusedVar(AST *ast, const std::string& varName);
        static bool ASTCheckUnusedVar(AST *ast, const std::string& varName);
        static bool ASTCheckUnchangedVar(AST *ast, const std::string& varName);

        static bool SimplifyTableApplyIfs(AST *block, const std::set<std::string>& boolVariablesSetTrueOnce, const std::vector<OptimizerStatementSuccessors>& successorGraph, std::unordered_set<AST*> *toDelete);
        static bool SimplifyAdjacentIfs(AST *block);
        static bool SimplifySimilarSwitchCases(AST *block);
        static bool SimplifySubsequentIfsFromVars(AST *block);

        static void GetVariablesSetTrueOnceRec(AST *block, std::unordered_map<std::string, int>* boolVariables);
        static std::set<std::string> GetVariablesSetTrueOnce(AST *block);

        //Aux
        static void GenStatementSuccessorsDot(const std::vector<OptimizerStatementSuccessors>& successors, FILE *fp);
        static void GenStatementSuccessorsDot(const std::vector<OptimizerStatementSuccessors>& successors, const std::string& fileName);
};

#endif