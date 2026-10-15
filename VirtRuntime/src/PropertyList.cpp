#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../include/PropertyList.h"

#define TAB_SPACE_INC "    "
std::string PropertyList::_prop_tab_spaces = "";
#define TAB_SPACES PropertyList::_prop_tab_spaces

void PropertyList::IncreaseTabSpaces(){
    TAB_SPACES = TAB_SPACES + TAB_SPACE_INC;
}

void PropertyList::DecreaseTabSpaces(){
    if(TAB_SPACES.length() >= strlen(TAB_SPACE_INC))
        TAB_SPACES = TAB_SPACES.substr(strlen(TAB_SPACE_INC));
}

PropertyList::PropertyList(){
    
}

PropertyList::~PropertyList(){
    for(auto it : this->m_list)
        for(Property& p : it.second)
            if(p.propType == RT_LIST)
                delete p.propList;
}

PropertyList::PropertyList(PropertyList &p){
    this->m_list = p.m_list;
    for(auto &it : this->m_list){
        for(Property &p : it.second){
            if(p.propType == RT_LIST){
                PropertyList *newPL = new PropertyList();
                *newPL = *(p.propList);
            }
        }
    }
}

void PropertyList::Print() const{
    for(auto entry : this->m_list){
        for(const Property& p : entry.second){
            printf("%s%s", TAB_SPACES.c_str(), entry.first.c_str());
            switch(p.propType){
                case RT_LIST:
                    printf(" {\n");
                    PropertyList::IncreaseTabSpaces();
                    p.propList->Print();
                    PropertyList::DecreaseTabSpaces();
                    printf("%s}\n", TAB_SPACES.c_str());
                    break;
                case RT_STRING:
                    printf(": %s\n", p.propString.c_str());
                    break;
            }
        }
    }
}

std::unordered_map<std::string, std::vector<Property>> PropertyList::GetList(FILE *fp){
    std::unordered_map<std::string, std::vector<Property>> list;
    std::string auxKey;
    Property tempP;

    char c = fgetc(fp);
    while(c == '\n' || c == ' ')
        c = fgetc(fp);
    while(!feof(fp) && c != '}'){
        auxKey.clear();
        while(c != ' '){
            auxKey.push_back(c);
            c = fgetc(fp);
        }
        while(c == ' ')
            c = fgetc(fp);
        if(c == '{'){
            tempP.propType = RT_LIST;
            tempP.propString.clear();
            tempP.propList = new PropertyList;
            tempP.propList->m_list = GetList(fp);
        } else {
            auxKey.pop_back();
            tempP.propString.clear();
            while(c != ' ' && c != '\n'){
                tempP.propString.push_back(c);
                c = fgetc(fp);
            }
            tempP.propType = RT_STRING;
            tempP.propList = NULL;
        }
        auto it = list.find(auxKey);
        if(it == list.end())
            list.emplace(auxKey, std::vector<Property>{tempP});
        else
            it->second.push_back(tempP);

        c = fgetc(fp);
        while(c == '\n' || c == ' ')
            c = fgetc(fp);
    }

    return list;
}

void PropertyList::LoadFromFile(const std::string& filePath){
    FILE *fp = fopen(filePath.c_str(), "r");
    if(!fp){
        fprintf(stderr, "Could not open file: %s\n", filePath.c_str());
        exit(1);
    }

    this->m_list = PropertyList::GetList(fp);
    
    fclose(fp);
}
