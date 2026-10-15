#ifndef AST_SIMPLIFIER_H
#define AST_SIMPLIFIER_H

class ASTSimplifier {
    public:
        static void RemoveVirtPreventMergeAnnotations(AST *ast);
        static void CheckUnusedHeaders(AST *ast);

    private:
        
};

#endif