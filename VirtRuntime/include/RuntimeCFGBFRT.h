#ifndef RUNTIME_CFG_BFRT
#define RUNTIME_CFG_BFRT

#define RUNTIME_EXCEPT_CLIENT_CFG_NOT_FOUND_MSG "NOT_FOUND"
#define RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG "CLIENT_NOT_FOUND"
#define RUNTIME_EXCEPT_TABLE_FULL "TABLE_RESOURCE_EXHAUSTED"
#define RUNTIME_EXCEPT_IDLE_TIMEOUT_NOT_SUPPORTED "IDLE_TIMEOUT_NOT_SUPPORTED"
#define RUNTIME_EXCEPT_MALFORMED_REQUEST "MALFORMED_REQUEST"
#define RUNTIME_EXCEPT_NOT_IMPLEMENTED "NOT_IMPLEMENTED"
#define RUNTIME_EXCEPT_INTERNAL_ERROR "INTERNAL_ERROR"

enum VirtTableTypeBFRT {VTTBF_NOT_SHARED = 0, VTTBF_HIDDEN_SHARED = 1, VTTBF_SHARED = 2};

typedef struct _virt_table_data_bfrt{
    std::string name;
    VirtTableTypeBFRT type;
    std::set<int> programIDs;
    int totalSize;
} VirtTableDataBFRT;

class RuntimeCFGBFRT {
    public:
        RuntimeCFGBFRT(const std::string& clientCFGPath, const std::string& virtCFGPath, const std::string& virtBFRTInfoPath);
        ~RuntimeCFGBFRT();

        void Update(const std::string& clientCFGPath, const std::string& virtCFGPath, const std::string& virtBFRTInfoPath);
        
        //uint64_t GetTableID(const BFInfo& info, const std::string& tableName);
        //uint64_t GetActionID(const BFInfo& info, const std::string& actionName);

        std::vector<std::string> GetClientNames() const;
        std::string GetClientName(const uint32_t clientID) const;

        BFRTInfoDesc* GetClientBFRTInfoDesc(const std::string& client);
        BFRTInfoDesc* GetVirtRuntimeBFRTInfoDesc();

        bfrt_proto::WriteRequest Translate(const std::string& client, const bfrt_proto::WriteRequest* request);
        bfrt_proto::ReadRequest Translate(const std::string& client, const bfrt_proto::ReadRequest* request);

        bfrt_proto::ReadResponse Translate(const std::string& client, const bfrt_proto::ReadResponse* response);

        uint32_t GetClientID(const std::string& clientName) const;

        uint64_t GetClientRegisterID(const uint32_t clientID, const uint32_t registerN) const;

        uint32_t GetStartOfFlagRegisterForClient(const std::string& clientName, const uint32_t registerN) const;
        uint32_t GetSizeOfFlagRegisterForClient(const std::string& clientName, const uint32_t registerN) const;

        uint64_t GetMappedTableID(uint32_t clientID, uint64_t clientTableID) const;
        uint64_t GetMappedRegisterID(uint32_t clientID, uint64_t clientRegisterID) const;
        std::string GetMappedTableName(uint32_t clientID, const std::string& clientTableName) const;
        std::string GetMappedTableNameReverse(uint32_t clientID, const std::string& virtTableName) const;
        std::string GetMappedActionName(uint32_t clientID, const std::string& clientActionName) const;
        std::string GetMappedActionNameReverse(uint32_t clientID, const std::string& virtActionName) const;

        VirtTableDataBFRT GetVirtTableData(const std::string& tableName) const;

        std::pair<uint32_t, size_t> GetClientAndIndexOfFlagRegister(const int flagRegN, const size_t flagRegIndex) const;

    private:
        BFRTInfoDesc m_virtBFInfo;

        std::vector<std::string> m_clientNames;
        std::vector<BFRTInfoDesc> m_clientsBFInfo;

        std::unordered_map<std::string, VirtTableDataBFRT> m_virtTableDatas;

        //Client ID to Table ID mapping
        std::unordered_map<uint32_t, std::unordered_map<uint64_t, uint64_t>> m_clientsTableTranslator; 

        //Client ID to Action ID mapping
        std::unordered_map<uint32_t, std::unordered_map<uint64_t, uint64_t>> m_clientsActionTranslator; 

        //Client ID to Register ID mapping
        std::unordered_map<uint32_t, std::unordered_map<uint64_t, uint64_t>> m_clientsRegisterTranslator; 

        std::unordered_map<uint32_t, std::vector<uint64_t>> m_clientsRegisterIDs;

        //Client ID to Register sizes
        std::unordered_map<uint32_t, std::vector<size_t>> m_clientsAuxFlagRegisterSizes;

        //Client ID to Register start
        std::unordered_map<uint32_t, std::vector<size_t>> m_clientsAuxFlagRegisterStart;

        //Client ID to Ports
        std::unordered_map<uint32_t, std::vector<uint32_t>> m_clientsPorts;

        //Client ID to Table Name mapping
        std::unordered_map<uint32_t, std::unordered_map<std::string, std::string>> m_clientsTableNameTranslator;
        std::unordered_map<uint32_t, std::unordered_map<std::string, std::string>> m_clientsTableNameTranslatorReverse; 

        //Client ID to Action Name mapping
        std::unordered_map<uint32_t, std::unordered_map<std::string, std::string>> m_clientsActionNameTranslator; 
        std::unordered_map<uint32_t, std::unordered_map<std::string, std::string>> m_clientsActionNameTranslatorReverse;

        static VirtTableDataBFRT GetVirtTableDataFromLineBFRT(const std::string& line);
};

inline std::vector<std::string> RuntimeCFGBFRT::GetClientNames() const{
    return m_clientNames;
}

inline std::string RuntimeCFGBFRT::GetClientName(const uint32_t clientID) const {
    if(clientID == 0 || clientID > m_clientNames.size())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    return m_clientNames[clientID - 1];
}

inline BFRTInfoDesc* RuntimeCFGBFRT::GetVirtRuntimeBFRTInfoDesc(){
    return &m_virtBFInfo;
}

inline uint32_t RuntimeCFGBFRT::GetClientID(const std::string& clientName) const {
    size_t i = 0;
    while(i < m_clientNames.size()){
        if(m_clientNames[i] == clientName)
            return i + 1;
        i++;
    }
    return 0;
}

inline uint64_t RuntimeCFGBFRT::GetClientRegisterID(const uint32_t clientID, const uint32_t registerN) const {
    auto it = m_clientsRegisterIDs.find(clientID);
    if(it == m_clientsRegisterIDs.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    if(registerN >= it->second.size())
        throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
    return it->second[registerN];
}

inline uint32_t RuntimeCFGBFRT::GetStartOfFlagRegisterForClient(const std::string& clientName, const uint32_t registerN) const {
    uint32_t clientID = GetClientID(clientName);
    auto it = m_clientsAuxFlagRegisterStart.find(clientID);
    if(it == m_clientsAuxFlagRegisterStart.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    if(registerN >= it->second.size())
        throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
    return it->second[registerN];
}

inline uint32_t RuntimeCFGBFRT::GetSizeOfFlagRegisterForClient(const std::string& clientName, const uint32_t registerN) const {
    uint32_t clientID = GetClientID(clientName);
    auto it = m_clientsAuxFlagRegisterSizes.find(clientID);
    if(it == m_clientsAuxFlagRegisterSizes.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    if(registerN >= it->second.size())
        throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
    return it->second[registerN];
}

inline uint64_t RuntimeCFGBFRT::GetMappedTableID(uint32_t clientID, uint64_t clientTableID) const {
    auto it = m_clientsTableTranslator.find(clientID);
    if(it == m_clientsTableTranslator.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    auto it2 = it->second.find(clientTableID);
    if(it2 == it->second.end()) {
        //printf("GetMappedTableID: clientID %u, clientTableID %lu not found!\n", clientID, clientTableID);

        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    }
    return it2->second;
}

inline uint64_t RuntimeCFGBFRT::GetMappedRegisterID(uint32_t clientID, uint64_t clientRegisterID) const {
    auto it = m_clientsRegisterTranslator.find(clientID);
    if(it == m_clientsRegisterTranslator.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    auto it2 = it->second.find(clientRegisterID);
    if(it2 == it->second.end()) {
        //printf("GetMappedRegisterID: clientID %u, clientRegisterID %lu not found!\n", clientID, clientRegisterID);

        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    }
    return it2->second;
}

inline std::string RuntimeCFGBFRT::GetMappedTableName(uint32_t clientID, const std::string& clientTableName) const {
    auto it = m_clientsTableNameTranslator.find(clientID);
    if(it == m_clientsTableNameTranslator.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    auto it2 = it->second.find(clientTableName);
    if(it2 == it->second.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    return it2->second;
}

inline std::string RuntimeCFGBFRT::GetMappedTableNameReverse(uint32_t clientID, const std::string& virtTableName) const {
    auto it = m_clientsTableNameTranslatorReverse.find(clientID);
    if(it == m_clientsTableNameTranslatorReverse.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    auto it2 = it->second.find(virtTableName);
    if(it2 == it->second.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    return it2->second;
}

inline std::string RuntimeCFGBFRT::GetMappedActionName(uint32_t clientID, const std::string& clientActionName) const {
    auto it = m_clientsActionNameTranslator.find(clientID);
    if(it == m_clientsActionNameTranslator.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    auto it2 = it->second.find(clientActionName);
    if(it2 == it->second.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    return it2->second;
}

inline std::string RuntimeCFGBFRT::GetMappedActionNameReverse(uint32_t clientID, const std::string& virtActionName) const {
    auto it = m_clientsActionNameTranslatorReverse.find(clientID);
    if(it == m_clientsActionNameTranslatorReverse.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    auto it2 = it->second.find(virtActionName);
    if(it2 == it->second.end())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;
    return it2->second;
}

inline VirtTableDataBFRT RuntimeCFGBFRT::GetVirtTableData(const std::string& tableName) const {
    auto it = m_virtTableDatas.find(tableName);
    if(it == m_virtTableDatas.end()){
        /*
        printf("GetVirtTableData: Table %s not found in virt table datas!\n", tableName.c_str());
        
        for(auto const& x : m_virtTableDatas) {
            printf(" - Available table: %s\n", x.first.c_str());
        }
        */
        
        throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
    }
    return it->second;
}

inline std::pair<uint32_t, size_t> RuntimeCFGBFRT::GetClientAndIndexOfFlagRegister(const int flagRegN, const size_t flagRegIndex) const {
    for(const auto& clientEntry : m_clientsAuxFlagRegisterSizes){
        uint32_t clientID = clientEntry.first;
        size_t start = m_clientsAuxFlagRegisterStart.find(clientID)->second[flagRegN];
        size_t regSize = clientEntry.second[flagRegN];
        if(flagRegIndex >= start && flagRegIndex < regSize + start){
            return std::make_pair(clientID, flagRegIndex - start);
        }
    }
    throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
}


#endif