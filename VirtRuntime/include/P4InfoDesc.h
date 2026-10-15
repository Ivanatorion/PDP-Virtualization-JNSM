#ifndef P4_INFO_DESC_H
#define P4_INFO_DESC_H

class P4InfoDesc {
    public:
        P4InfoDesc();
        P4InfoDesc(const std::string& filePath);

        void LoadFromFile(const std::string& filePath);
        void Print() const;

        PropertyList* getPropertyList();

        //Returns NULL if not found
        PropertyList* getTablePropertyList(const std::string& tableName);
        //Returns NULL if not found
        PropertyList* getTablePropertyList(const int id);

        //Returns empty string if not found
        std::string getTableNameFromID(const int id);
        //Returns -1 if not found
        int getTableIDFromName(const std::string& name);

        //Returns NULL if not found
        PropertyList* getActionPropertyList(const std::string& tableName);
        //Returns NULL if not found
        PropertyList* getActionPropertyList(const int id);
        //Returns empty string if not found
        std::string getActionNameFromID(const int id);
        //Returns -1 if not found
        int getActionIDFromName(const std::string& name);

        p4::config::v1::P4Info GetP4Info();

    private:
        PropertyList m_propertyList;
};

inline void P4InfoDesc::LoadFromFile(const std::string& filePath){
    this->m_propertyList.LoadFromFile(filePath);
}

inline void P4InfoDesc::Print() const{
    this->m_propertyList.Print();
}

inline PropertyList* P4InfoDesc::getPropertyList(){
    return &this->m_propertyList;
}

#endif