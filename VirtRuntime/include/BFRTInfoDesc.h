#ifndef BFRT_INFO_DESC_H
#define BFRT_INFO_DESC_H

enum TableType {TT_REGISTER = 0, TT_OTHER = 1};

typedef struct {
    std::string name;
    uint64_t id;
    uint32_t size;
    TableType type;
} BFRTTable;

typedef struct {
    std::string name;
    uint64_t id;
} BFRTAction;

typedef struct {
    std::vector<BFRTTable> tables;
    std::vector<BFRTAction> actions;
} BFInfo;

class BFRTInfoDesc {
    public:
        BFRTInfoDesc();
        BFRTInfoDesc(const std::string& filePath);

        void LoadFromFile(const std::string& filePath);
        void Print() const;

        std::string GetBFRTInfoSerializedString() const;

        uint64_t GetTableID(const std::string& tableName) const;
        uint64_t GetActionID(const std::string& actionName) const;
        std::string GetTableName(const uint64_t tableID) const;
        std::string GetActionName(const uint64_t actionID) const;

        std::vector<BFRTTable> GetTables() const;

    private:
        BFInfo m_info;

        std::string m_bfrtInfoSerializedString;
};

inline uint64_t BFRTInfoDesc::GetTableID(const std::string& tableName) const {
    size_t i = 0;
    while(i < m_info.tables.size() && m_info.tables[i].name != tableName)
        i++;
    return i < m_info.tables.size() ? m_info.tables[i].id : 0;
}

inline uint64_t BFRTInfoDesc::GetActionID(const std::string& actionName) const {
    size_t i = 0;
    while(i < m_info.actions.size() && m_info.actions[i].name != actionName)
        i++;
    return i < m_info.actions.size() ? m_info.actions[i].id : 0;
}

inline std::string BFRTInfoDesc::GetTableName(const uint64_t tableID) const {
    size_t i = 0;
    while(i < m_info.tables.size() && m_info.tables[i].id != tableID)
        i++;
    return i < m_info.tables.size() ? m_info.tables[i].name : "";
}

inline std::string BFRTInfoDesc::GetActionName(const uint64_t actionID) const {
    size_t i = 0;
    while(i < m_info.actions.size() && m_info.actions[i].id != actionID)
        i++;
    return i < m_info.actions.size() ? m_info.actions[i].name : "";
}

inline std::string BFRTInfoDesc::GetBFRTInfoSerializedString() const {
    return m_bfrtInfoSerializedString;
}

inline std::vector<BFRTTable> BFRTInfoDesc::GetTables() const {
    return m_info.tables;
}

#endif