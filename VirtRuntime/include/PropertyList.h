#ifndef PROPERTY_LIST_H
#define PROPERTY_LIST_H

class PropertyList;

enum RawType {RT_LIST, RT_STRING};

typedef struct _property {
    RawType propType;
    std::string propString;
    PropertyList* propList;
} Property;

class PropertyList {
    public:
        PropertyList();
        ~PropertyList();
        PropertyList(PropertyList &p);

        void Print() const;
        void LoadFromFile(const std::string& filePath);

        //Returns NULL if not found
        std::vector<Property>* getProperty(const std::string& key);

    private:
        std::unordered_map<std::string, std::vector<Property>> m_list;

        static std::unordered_map<std::string, std::vector<Property>> GetList(FILE *fp);

        static std::string _prop_tab_spaces;
        static void IncreaseTabSpaces();
        static void DecreaseTabSpaces();
};

inline std::vector<Property>* PropertyList::getProperty(const std::string& key){
    auto it = this->m_list.find(key);
    if(it == this->m_list.end())
        return NULL;
    return &it->second;
}

#endif