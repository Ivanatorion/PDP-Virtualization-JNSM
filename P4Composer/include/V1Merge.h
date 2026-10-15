#ifndef V1_MERGE_H
#define V1_MERGE_H

#define V1MODEL_VIRT_HEADER_TYPE "VirtHeaders"

class V1Merge {
    public:
        static bool MergeV1(const std::vector<AST*> programs, const std::vector<AST*>& headersDec, const std::vector<std::set<int>>& virtualSwitchPorts, const int cpuPort);
        static void RenameParams(AST *program);
    private:
        static bool GetPipeline(AST *ast, std::vector<AST*> *Pipeline);
        static bool MergePipelines(const std::vector<AST*>& programs, const std::vector<std::vector<AST*>>& pipelines, const std::vector<AST*>& headersDec, const std::vector<std::set<int>>& virtualSwitchPorts, const int cpuPort);

        static void OrderUniqueEmitStatements(std::vector<AST*>& uniqueStatements, const std::vector<std::vector<AST*>>& programEmitStatements);
};

#endif
