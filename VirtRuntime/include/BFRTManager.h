#ifndef BF_RUNTIME_MANAGER_H
#define BF_RUNTIME_MANAGER_H

enum KeyType {KT_EXACT, KT_LPM};

typedef struct {
    uint32_t tableID;
    uint32_t size;
    std::string tableName;
} BFTableInfo;

typedef struct {
    uint32_t registerID;
    uint32_t size;
    std::string registerName;
} BFRegisterInfo;

typedef struct {
    uint32_t actionID;
    std::string actionName;
} BFActionInfo;

typedef struct {
    std::vector<std::string> keys; //TODO: Only works with exact?
    std::vector<std::string> actionParams;
    BFActionInfo action;

    bfrt_proto::TableEntry protoEntry;
} BFTableEntry;

class BFRTManager {
    public:
        BFRTManager(std::shared_ptr<bfrt_proto::BfRuntime::Stub> stub, std::shared_ptr<grpc::ClientReaderWriter<bfrt_proto::StreamMessageRequest, bfrt_proto::StreamMessageResponse>> stream);
        ~BFRTManager();

        void LoadFromFile(const std::string& filePath);
        void LoadFromMemory(const std::string& memory);

        void WriteToRegister(uint32_t regID, uint32_t index, const std::string& value);
        void WriteToRegister(const std::string& regName, uint32_t index, const std::string& value);

        void WriteToRegister(uint32_t regID, const std::vector<uint32_t>& indexes, const std::vector<std::string>& values);
        void WriteToRegister(const std::string& regName, const std::vector<uint32_t>& indexes, const std::vector<std::string>& values);

        //TODO: Implement
        //Vector has size 4 (one value per pipeline)
        std::vector<std::string> ReadFromRegister(uint32_t regID, uint32_t index);
        std::vector<std::string> ReadFromRegister(const std::string& regName, uint32_t index);

        static std::vector<std::vector<std::string>> ReadFromRegister(uint32_t regID, std::shared_ptr<bfrt_proto::BfRuntime::Stub> stub);
        std::vector<std::vector<std::string>> ReadFromRegister(uint32_t regID);
        std::vector<std::vector<std::string>> ReadFromRegister(const std::string& regName);

        void ErasePreNodeTableEntries();
        void ErasePreMGIDTableEntries();
        bool WriteToPreNode(uint32_t mcNodeID, uint16_t mcRID, const std::vector<uint8_t>& mcLAGID, const std::vector<uint32_t>& portList);
        bool WriteToPreMGID(uint16_t mgID, const std::vector<uint32_t>& mcNodeID, const std::vector<bool>& mcNodeL1XIDValid, const std::vector<uint16_t>& mcNodeL1XID);

        std::vector<BFTableEntry> ReadTableEntries(const uint32_t tableID);
        std::vector<BFTableEntry> ReadTableEntries(const std::string& tableName);

        uint32_t GetRegisterID(const std::string& regName) const;
        uint32_t GetTableID(const std::string& tableName) const;
        uint32_t GetActionID(const std::string& actionName) const;

        uint32_t GetRegisterSize(uint32_t regID) const;

        std::vector<BFTableInfo> GetTableInfoList() const;
        std::vector<BFRegisterInfo> GetRegisterInfoList() const;

        void SendSubscribe();
        bfrt_proto::GetForwardingPipelineConfigResponse GetFwdPipeline();
        void Bind(bfrt_proto::ForwardingPipelineConfig fpc);

    private:
        std::vector<BFTableInfo> m_tableInfoList;
        std::vector<BFRegisterInfo> m_regInfoList;
        std::vector<BFActionInfo> m_actionInfoList;

        std::shared_ptr<bfrt_proto::BfRuntime::Stub> m_stub;
        std::shared_ptr<grpc::ClientReaderWriter<bfrt_proto::StreamMessageRequest, bfrt_proto::StreamMessageResponse>> m_stream;
};

inline std::vector<BFTableInfo> BFRTManager::GetTableInfoList() const {
    return m_tableInfoList;
}

inline std::vector<BFRegisterInfo> BFRTManager::GetRegisterInfoList() const {
    return m_regInfoList;
}

inline uint32_t BFRTManager::GetRegisterID(const std::string& regName) const {
    for(const BFRegisterInfo& rInfo : m_regInfoList){
        if(rInfo.registerName == regName)
            return rInfo.registerID;
    }
    return 0;
}

inline uint32_t BFRTManager::GetTableID(const std::string& tableName) const {
    for(const BFTableInfo& tInfo : m_tableInfoList){
        if(tInfo.tableName == tableName)
            return tInfo.tableID;
    }
    return 0;
}

inline uint32_t BFRTManager::GetActionID(const std::string& actionName) const {
    for(const BFActionInfo& aInfo : m_actionInfoList){
        if(aInfo.actionName == actionName)
            return aInfo.actionID;
    }
    return 0;
}

inline uint32_t BFRTManager::GetRegisterSize(uint32_t regID) const {
    for(const BFRegisterInfo& rInfo : m_regInfoList){
        if(rInfo.registerID == regID)
            return rInfo.size;
    }
    return 0;
}

inline std::vector<BFTableEntry> BFRTManager::ReadTableEntries(const std::string& tableName){
    const uint32_t id = this->GetTableID(tableName);
    if(id == 0)
        return {};
    
    return this->ReadTableEntries(id); 
}

inline void BFRTManager::WriteToRegister(const std::string& regName, uint32_t index, const std::string& value) {
    uint32_t regID = this->GetRegisterID(regName);
    if(regID != 0)
        this->WriteToRegister(regID, index, value);
}

inline void BFRTManager::WriteToRegister(const std::string& regName, const std::vector<uint32_t>& indexes, const std::vector<std::string>& values) {
    uint32_t regID = this->GetRegisterID(regName);
    if(regID != 0)
        this->WriteToRegister(regID, indexes, values);
}

inline std::vector<std::string> BFRTManager::ReadFromRegister(const std::string& regName, uint32_t index) {
    uint32_t regID = this->GetRegisterID(regName);
    if(regID != 0)
        return this->ReadFromRegister(regID, index);
    return std::vector<std::string>();
}

inline std::vector<std::vector<std::string>> BFRTManager::ReadFromRegister(const std::string& regName) {
    uint32_t regID = this->GetRegisterID(regName);
    if(regID != 0)
        return this->ReadFromRegister(regID);
    return std::vector<std::vector<std::string>>();
}

#endif