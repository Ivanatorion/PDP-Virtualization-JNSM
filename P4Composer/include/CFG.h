#ifndef CFG_H
#define CFG_H

#define VIRT_SHARED_ANNOTATION "VirtShared"

enum VTableType {VTT_SHARED, VTT_HIDDEN_SHARED, VTT_NOT_SHARED};

typedef struct _match_field{
    std::string fieldName;
    std::string fieldType;
} TableMatchField;

typedef struct _vtable_data{
    std::string controlBlockName;
    std::string name;
    VTableType vtype;
    std::set<int> programs;
    int totalSize;
    bool supportTimeout;

    std::vector<TableMatchField> matchFieldList;

    uint32_t p4infoID;
} VirtTableData;

typedef struct _shared_vtable_data{
    std::string alias;
    int vtdIDX;
    AST* tableCopy;
    int owner;
    bool nonOwnerCanWrite;
} SharedTableData;

typedef struct _program_table_data{
    std::string controlBlockName;
    std::string programName;
    std::string annotatedName;

    std::string vTableControlBL;
    std::string vTableName;

    std::vector<std::string> actionList;

    bool owner;
    bool canWrite;
    int programSize;

    uint32_t p4infoID;
    bool p4infoIDisAnnotated;
} ProgramTableData;

typedef struct _action_param_data {
    std::string name;
    uint32_t bitwidth;
} ProgramActionParamData;

typedef struct _program_action_data {
    std::string controlBlockName; //empty for global
    std::string programName;
    std::string originalProgramName; //To generate p4info
    std::string annotatedName;

    std::vector<ProgramActionParamData> paramsData;

    std::string vActionControlBL; //empty for global
    std::string vActionName;

    uint32_t p4infoID;
    bool p4infoIDisAnnotated;
} ProgramActionData;

class CFG{
    public:
        CFG(const CFG&) = delete;
        void operator=(const CFG&) = delete;

        static CFG& Get();

        static void SetOutputDir(const std::string& outputDir);

        static void SetProgramNames(const std::vector<std::string>& programNames);
        static void SetProgramASTs(const std::vector<AST*> programASTs);
        
        static void SetArchitecture(const P4Architecture arch);
        static void SetMetadataProgIDName(const std::string& metadataProgIDFullName);

        static void AddProgramTable(int program, AST *table, const std::string& vTableName, const std::string& programControlBL, const std::string& virtualControlBL);
        static void ChangeVTableName(const std::string& virtControlName, const std::string& oldName, const std::string& newName);
    
        static void AddProgramAction(int program, AST *action, const std::string& originalProgramName, const std::string& vActionName, const std::string& programControlBL, const std::string& virtualControlBL);
        static void ChangeVActionName(const std::string& virtControlName, const std::string& oldName, const std::string& newName);
    
        static void GenerateP4Infos(AST *merged);
    private:
        CFG();
        ~CFG();

        std::string m_outputDir;

        std::vector<std::string> m_programNames;
        std::vector<AST*> m_programASTs;

        P4Architecture m_architecture;
        std::string m_metadataProgIDFullName;

        std::map<std::string, std::vector<std::string>> m_virtControlsToProgControls;

        std::vector<SharedTableData> m_sharedTableDatas;
        std::vector<VirtTableData> m_vTableDatas;
        std::vector<std::vector<ProgramTableData>> m_programTableDatas;
        std::vector<std::vector<ProgramActionData>> m_programActionDatas;

        void GenerateMergedP4Info(AST *merged) const;
        void GenerateProgramP4Info(const int progN) const;

        static ProgramActionData GetPad(AST *ActionD, const std::string& controlBlockName, const int progN);
        static std::set<uint32_t> GetDeclaredIDs(AST *ast);
        static void AddVirtTableMatchFields(AST *merged);
        static uint32_t GetRandomUniqueID();
};

#endif