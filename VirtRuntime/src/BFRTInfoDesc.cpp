#include "../include/pcheaders.h"
#include "../proto/bfruntime.grpc.pb.h"
#include "../include/IPLBase.hpp"
#include "../include/BFRTInfoDesc.h"

BFRTInfoDesc::BFRTInfoDesc(){

}

BFRTInfoDesc::BFRTInfoDesc(const std::string& filePath){
    this->LoadFromFile(filePath);
}

void BFRTInfoDesc::Print() const {
    printf("Tables:\n");
    for(const BFRTTable& table : m_info.tables){
        printf("Name: %s\n", table.name.c_str());
        printf("ID: %lu\n", table.id);
        printf("Size: %u\n", table.size);
        printf("Type: %s\n", table.type == TT_REGISTER ? "Register" : "Other");
        printf("\n");
    }

    printf("Actions:\n");
    for(const BFRTAction& action : m_info.actions){
        printf("Name: %s\n", action.name.c_str());
        printf("ID: %lu\n", action.id);
        printf("\n");
    }
}

std::vector<std::string> SplitKeyValue(const std::string& line){
    std::vector<std::string> result(2);

    size_t j = 0;
    while(j < line.size() && line[j] != '"')
        j++;
    Assert(j < line.size());
    result[0] = "\"";
    j++;
    while(j < line.size() && line[j] != '"'){
        result[0].push_back(line[j]);
        j++;
    }
    result[0].push_back(line[j]);
    j++;
    while(j < line.size() && line[j] != ':')
        j++;
    Assert(j < line.size());
    j++;
    while(j < line.size() && line[j] == ' ')
        j++;
    Assert(j < line.size());
    std::string value;
    while(j < line.size()){
        result[1].push_back(line[j]);
        j++;
    }

    if(result[1].back() == ',')
        result[1].pop_back();
    
    return result;
}

void LoadBFRTAction(BFInfo *info, const std::vector<std::string>& lines, size_t& i){
    BFRTAction action;

    while(i < lines.size() && !(lines[i].find("}") == lines[i].size() - 1 || lines[i].find("},") == lines[i].size() - 2)){
        std::vector<std::string> kv = SplitKeyValue(lines[i]);

        if(kv[0] == "\"name\""){
            action.name = kv[1];
        }
        else if(kv[0] == "\"id\""){
            action.id = IPLBase::intFromStr64(kv[1].c_str());
            Assert(action.id != 0);
        }
        else if(kv[1] == "["){
            i++;
            while(i < lines.size() && !(lines[i].find(" ]") == lines[i].size() - 2 || lines[i].find(" ],") == lines[i].size() - 3))
                i++;
        }
        i++;
    }

    i++;

    info->actions.push_back(action);
}

void LoadBFRTTable(BFInfo *info, const std::vector<std::string>& lines, size_t& i){
    BFRTTable table;

    while(i < lines.size() && !(lines[i].find("}") == lines[i].size() - 1 || lines[i].find("},") == lines[i].size() - 2)){
        std::vector<std::string> kv = SplitKeyValue(lines[i]);

        if(kv[0] == "\"name\""){
            table.name = kv[1];
        }
        else if(kv[0] == "\"id\""){
            table.id = IPLBase::intFromStr64(kv[1].c_str());
            Assert(table.id != 0);
        }
        else if(kv[0] == "\"table_type\""){
            if(kv[1] == "\"Register\"")
                table.type = TT_REGISTER;
            else
                table.type = TT_OTHER;
        }
        else if(kv[0] == "\"size\""){
            table.size = IPLBase::intFromStr64(kv[1].c_str());
        }
        else if(kv[0] == "\"action_specs\""){
            i++;
            while(lines[i].back() == '{'){
                i++;
                LoadBFRTAction(info, lines, i);
            }
        }
        else if(kv[1] == "["){
            i++;
            int c = 1;
            while(i < lines.size() && c > 0){
                i++;
                if(lines[i].find(" ]") == lines[i].size() - 2 || lines[i].find(" ],") == lines[i].size() - 3)
                    c--;
                if(lines[i].find("[") == lines[i].size() - 1)
                    c++;
            }
            while(i < lines.size() && !(lines[i].find(" ]") == lines[i].size() - 2 || lines[i].find(" ],") == lines[i].size() - 3))
                i++;
        }
        i++;
    }

    info->tables.push_back(table);
}

void BFRTInfoDesc::LoadFromFile(const std::string& filePath){
    std::string path = filePath;
    if(!std::filesystem::exists(path)) {
        path = TEMP_DIR_NAME "/" + filePath;
        if(!std::filesystem::exists(path)) {
            fprintf(stderr, "Error: Could not find BFRT info file: %s\n", filePath.c_str());
            exit(1);
        }
    }

    std::vector<std::string> lines = IPLBase::readFileLines(path);
    if(lines.size() == 0){
        fprintf(stderr, "Error: Could not open file \"%s\"\n", path.c_str());
        exit(1);
    }

    size_t i = 0;
    while(i < lines.size() && lines[i].find("\"tables\" :") == std::string::npos){
        i++;
    }
    i++;

    bool done = (i == lines.size());
    while(!done){
        if(lines[i].find("{") == lines[i].size() - 1){
            i++;
            LoadBFRTTable(&m_info, lines, i);
        }
        else if(lines[i].find("]") == lines[i].size() - 1 || lines[i].find("],") == lines.size() - 2){
            done = true;
        }

        i++;
        if(i == lines[i].size())
            done = true;
    }

    //Remove quotes from table names
    for(size_t i = 0; i < m_info.tables.size(); i++){
        Assert(m_info.tables[i].name[0] == '"' && m_info.tables[i].name[m_info.tables[i].name.size() - 1] == '"');
        m_info.tables[i].name = m_info.tables[i].name.substr(1, m_info.tables[i].name.size() - 2);
    }

    //Remove quotes from action names
    for(size_t i = 0; i < m_info.actions.size(); i++){
        Assert(m_info.actions[i].name[0] == '"' && m_info.actions[i].name[m_info.actions[i].name.size() - 1] == '"');
        m_info.actions[i].name = m_info.actions[i].name.substr(1, m_info.actions[i].name.size() - 2);
    }

    //Serialize BFRTInfo to string from file
    m_bfrtInfoSerializedString.clear();
    for(const std::string& line : lines){
        m_bfrtInfoSerializedString += line + "\n";
    }
}