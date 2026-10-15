#ifndef RUNTIME_CFG_H
#define RUNTIME_CFG_H

#define RUNTIME_EXCEPT_CLIENT_CFG_NOT_FOUND_MSG "NOT_FOUND"
#define RUNTIME_EXCEPT_TABLE_FULL "TABLE_RESOURCE_EXHAUSTED"
#define RUNTIME_EXCEPT_IDLE_TIMEOUT_NOT_SUPPORTED "IDLE_TIMEOUT_NOT_SUPPORTED"
#define RUNTIME_EXCEPT_MALFORMED_REQUEST "MALFORMED_REQUEST"

enum VirtTableType {VTT_NOT_SHARED = 0, VTT_HIDDEN_SHARED = 1, VTT_SHARED = 2};

typedef struct _virt_table_data{
    std::string name;
    VirtTableType type;
    std::set<int> programIDs;
    int totalSize;
} VirtTableData;

typedef struct _program_table_data{
    std::string name;
    std::string virtName;
    bool isSharedOwner;
    bool canWrite;

    int curProgSize;
    int maxProgSize;
} ProgramTableData;

typedef struct _program_action_data{
    std::string name;
    std::string virtName;
} ProgramActionData;

typedef struct _client_data{
    int clientProgID;
    P4InfoDesc* clientP4InfoDesc;
} ClientData;

class RuntimeCFG{
    public:
        RuntimeCFG(const std::string& clientCFGPath, const std::string& virtCFGPath, const std::string& virtP4InfoPath);
        ~RuntimeCFG();
        
        void Update(const std::string& clientCFGPath, const std::string& virtCFGPath, const std::string& virtP4InfoPath);

        std::vector<std::string> GetClientNames() const;

        int GetClientID(const std::string& client);
        P4InfoDesc* GetClientP4InfoDesc(const std::string& client);

        p4::v1::TableEntry TranslateTableEntryClientToVirt(const std::string& client, const p4::v1::TableEntry* tableEntry);
        p4::v1::TableEntry TranslateTableEntryVirtToClient(const p4::v1::TableEntry* tableEntry, int* resultClientID);


        p4::v1::WriteRequest Translate(const std::string& client, const p4::v1::WriteRequest* request);
        p4::v1::ReadRequest Translate(const std::string& client, const p4::v1::ReadRequest* request);

        p4::v1::ReadResponse Translate(const std::string& client, const p4::v1::ReadResponse* response);

        p4::config::v1::P4Info GetMergedP4Info();

        VirtTableData GetVirtTableDataForTable(const std::string& tableName);
        VirtTableData GetVirtTableDataForTable(const int tableID);
    
        void IncreaseTableUsageForRequests(const std::string& client, const p4::v1::WriteRequest* request);
    
        static std::string VirtTableTypeToString(const VirtTableType vtt);
    private:
        std::map<std::string, std::map<std::string, std::string>> m_clientsTableNameTranslator;
        std::map<std::string, std::map<std::string, std::string>> m_clientsActionNameTranslator;
        
        std::map<std::string, std::map<std::string, std::string>> m_clientsTableNameTranslatorReverse;
        std::map<std::string, std::map<std::string, std::string>> m_clientsActionNameTranslatorReverse;
        
        std::map<std::string, ClientData> m_clientsP4Info;
        std::map<std::string, VirtTableData> m_virtTableDatas;
        std::map<std::string, std::map<std::string, ProgramTableData>> m_programTableDatas;
        P4InfoDesc m_virtP4Info;

        static VirtTableData getVirtTableDataFromLine(const std::string& line);
        static ProgramTableData getProgramTableDataFromLine(const std::string& line);
        static ProgramActionData getProgramActionDataFromLine(const std::string& line);
};

inline P4InfoDesc* RuntimeCFG::GetClientP4InfoDesc(const std::string& client){
    auto it = this->m_clientsP4Info.find(client);
    if(it == this->m_clientsP4Info.end())
        return NULL;
    return it->second.clientP4InfoDesc;
}

inline int RuntimeCFG::GetClientID(const std::string& client){
    return this->m_clientsP4Info.find(client)->second.clientProgID;
}

inline p4::config::v1::P4Info RuntimeCFG::GetMergedP4Info(){
    return this->m_virtP4Info.GetP4Info();
}

inline std::string RuntimeCFG::VirtTableTypeToString(const VirtTableType vtt){
    std::string result;
    switch(vtt){
        case VTT_NOT_SHARED:
            return "VTT_NOT_SHARED";
            break;
        case VTT_HIDDEN_SHARED:
            return "VTT_HIDDEN_SHARED";
            break;
        case VTT_SHARED:
            return "VTT_SHARED";
            break;
        default:
            break;
    }
    return result;
}

#endif