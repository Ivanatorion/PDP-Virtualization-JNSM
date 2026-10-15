#ifndef TOFINO_MERGE_H
#define TOFINO_MERGE_H

class TofinoMerge {
    public:
        static bool MergeTofino(const std::vector<AST*> programs, const std::vector<AST*>& headersDec, const std::vector<std::set<int>>& virtualSwitchPorts, const int cpuPort);

        //Fix known compiler errors due to mismatches with the P4 Language specification
        static void FixCompilerErrors(AST *ast);
    private:
        static bool RemoveSwitchInstantiation(AST *ast);
        static bool GetPipelines(AST *ast, std::vector<AST*> *Pipelines);
        static bool MergePipelines(const std::vector<AST*>& programs, const std::vector<AST*>& pipelines, const std::vector<AST*>& headersDec, const std::vector<std::set<int>>& virtualSwitchPorts, const int cpuPort);

        static AST* CreateLookaheadHeaderIngress(const bool withProgramID);
        static AST* CreateLookaheadHeaderEgress(const bool withProgramID);

        static bool RemoveIngressPortMetadataExtractions(const std::string& programName, AST *statelist, AST *state, const std::string packetInName, const std::string parameterName, bool intrinsicMDportMD);
        static bool RemoveEgressPortMetadataExtractions(const std::string& programName, AST *statelist, AST *state, const std::string packetInName, const std::string parameterName);

        static void RemoveSwitchCaseEmitsFromDeparser(AST *deparser, const size_t nPrograms);

        static void RemoveConditionalChecksum(AST *deparser);

        //Add statements to skip the bits starting at 128
        static void AddVirtPacketOutAdvanceToParser(const std::vector<AST*>& headersDec, AST *parser, const int cpuPort);
        static void _AddVirtPacketOutAdvaceToParserRec(int remainingBits, const std::vector<AST*>& headersDec, AST *parser, AST *state, std::unordered_set<AST*>& visitedStates, const int cpuPort);
};

#endif
