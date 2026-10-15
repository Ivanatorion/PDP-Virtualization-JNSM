#include "../include/pcheaders.h"
#include "../proto/bfruntime.grpc.pb.h"
#include "../proto/p4runtime.grpc.pb.h"
#include "../include/IPLBase.hpp"
#include "../include/PropertyList.h"
#include "../include/P4InfoDesc.h"
#include "../include/RuntimeCFG.h"
#include "../include/Utils.h"

#define CFG_FILE_NUM_WORDS_PER_LINE 2
#define NUM_VIRT_TABLE_PROPERTIES 5

RuntimeCFG::RuntimeCFG(const std::string& clientCFGPath, const std::string& virtCFGPath, const std::string& virtP4InfoPath){
    if(clientCFGPath == "" || virtCFGPath == "" || virtP4InfoPath == ""){
        printf("Warning: No initial runtime configuration\n");
    }
    else {
        this->Update(clientCFGPath, virtCFGPath, virtP4InfoPath);
    }
}

RuntimeCFG::~RuntimeCFG(){
    for(auto &it : this->m_clientsP4Info)
        delete it.second.clientP4InfoDesc;
}

void RuntimeCFG::Update(const std::string& clientCFGPath, const std::string& virtCFGPath, const std::string& virtP4InfoPath){
    printf("Loading RuntimeCFG from \"%s\" and \"%s\"\n", clientCFGPath.c_str(), virtCFGPath.c_str());

    char buffer[BUFFER_SIZE];
    this->m_clientsTableNameTranslator.clear();
    this->m_clientsActionNameTranslator.clear();

    for(auto entry : this->m_clientsP4Info)
        delete entry.second.clientP4InfoDesc;
    this->m_clientsP4Info.clear();

    std::vector<std::string> clientCFGLines = IPLBase::readFileLines(clientCFGPath);
    std::vector<std::string> virtCFGLines = IPLBase::readFileLines(virtCFGPath);

    if(clientCFGLines.size() == 0){
        clientCFGLines = IPLBase::readFileLines(TEMP_DIR_NAME "/" + clientCFGPath);
        if(clientCFGLines.size() == 0){
            fprintf(stderr, "Error: Could not open file \"%s\"\n", clientCFGPath.c_str());
            exit(1);
        }
    }
    if(virtCFGLines.size() == 0){
        virtCFGLines = IPLBase::readFileLines(TEMP_DIR_NAME "/" + virtCFGPath);
        if(virtCFGLines.size() == 0){
            fprintf(stderr, "Error: Could not open file \"%s\"\n", virtCFGPath.c_str());
            exit(1);
        }
    }

    this->m_virtP4Info.LoadFromFile(virtP4InfoPath);

    int VCFGVTablesStartLine = 0;
    while(VCFGVTablesStartLine < (int) virtCFGLines.size() && virtCFGLines[VCFGVTablesStartLine] != "VTables:")
        VCFGVTablesStartLine++;
    if(VCFGVTablesStartLine == (int) virtCFGLines.size()){
        fprintf(stderr, "Error: Malformed file \"%s\"\n", virtCFGPath.c_str());
        exit(1);
    }
    int VCFGSharedTablesStartLine = 0;
    while(VCFGSharedTablesStartLine < (int) virtCFGLines.size() && virtCFGLines[VCFGSharedTablesStartLine] != "VirtShared Tables:")
        VCFGSharedTablesStartLine++;
    if(VCFGSharedTablesStartLine == (int) virtCFGLines.size() || VCFGSharedTablesStartLine < VCFGVTablesStartLine){
        fprintf(stderr, "Error: Malformed file \"%s\"\n", virtCFGPath.c_str());
        exit(1);
    }
    int VCFGActionsStartLine = 0;
    while(VCFGActionsStartLine < (int) virtCFGLines.size() && virtCFGLines[VCFGActionsStartLine] != "Actions:")
        VCFGActionsStartLine++;
    if(VCFGActionsStartLine == (int) virtCFGLines.size() || VCFGActionsStartLine < VCFGSharedTablesStartLine){
        fprintf(stderr, "Error: Malformed file \"%s\"\n", virtCFGPath.c_str());
        exit(1);
    }

    int ptr = VCFGVTablesStartLine + 1;
    while(ptr < (int) virtCFGLines.size() && ptr < VCFGSharedTablesStartLine){
        VirtTableData vtd = RuntimeCFG::getVirtTableDataFromLine(virtCFGLines[ptr]);
        this->m_virtTableDatas.emplace(vtd.name, vtd);
        ptr++;
    }

    for(int i = 0; i < (int) clientCFGLines.size(); i++){
        const std::string& line = clientCFGLines[i];

        std::vector<std::string> lineS = IPLBase::split(line);
        if(lineS.size() != CFG_FILE_NUM_WORDS_PER_LINE){
            fprintf(stderr, "Error: Malformed line in \"%s\" - %s\n", clientCFGPath.c_str(), line.c_str());
            exit(1);
        }

        const std::string ClientP4InfoFilePath = std::filesystem::exists(lineS[1]) ? lineS[1] : std::string(TEMP_DIR_NAME "/") + lineS[1];

        this->m_clientsP4Info.emplace(lineS[0], ClientData({i+1, new P4InfoDesc(ClientP4InfoFilePath)}));

        //Create table map for client
        std::map<std::string, std::string> tableNameTranslator;
        std::map<std::string, std::string> tableNameTranslatorReverse;
        sprintf(buffer, "%d:", i);
        int l = VCFGSharedTablesStartLine + 1;
        while(l < VCFGActionsStartLine && strcmp(buffer, virtCFGLines[l].c_str()))
            l++;
        if(l == VCFGActionsStartLine){
            fprintf(stderr, "Error: Malformed file \"%s\" (No VirtShared Tables list for program %d)\n", virtCFGPath.c_str(), i);
            exit(1);
        }
        l++;
        std::map<std::string, ProgramTableData> ptdMap;
        while(virtCFGLines[l][virtCFGLines[l].size() - 1] != ':'){
            ProgramTableData data = RuntimeCFG::getProgramTableDataFromLine(virtCFGLines[l]);
            tableNameTranslator.emplace(data.name, data.virtName);
            tableNameTranslatorReverse.emplace(data.virtName, data.name);
            ptdMap.emplace(data.name, data);
            l++;
        }
        this->m_clientsTableNameTranslator.emplace(lineS[0], tableNameTranslator);
        this->m_clientsTableNameTranslatorReverse.emplace(lineS[0], tableNameTranslatorReverse);
        this->m_programTableDatas.emplace(lineS[0], ptdMap);

        //Create action map for client
        std::map<std::string, std::string> actionNameTranslator;
        std::map<std::string, std::string> actionNameTranslatorReverse;
        l = VCFGActionsStartLine + 1;
        while(l < (int) virtCFGLines.size() && strcmp(buffer, virtCFGLines[l].c_str()))
            l++;
        if(l == (int) virtCFGLines.size()){
            fprintf(stderr, "Error: Malformed file \"%s\" (No Actions list for program %d)\n", virtCFGPath.c_str(), i);
            exit(1);
        }
        l++;
        while(l < (int) virtCFGLines.size() && virtCFGLines[l][virtCFGLines[l].size() - 1] != ':'){
            ProgramActionData data = RuntimeCFG::getProgramActionDataFromLine(virtCFGLines[l]);
            actionNameTranslator.emplace(data.name, data.virtName);
            actionNameTranslatorReverse.emplace(data.virtName, data.name);
            //TODO: Use rest of data
            l++;
        }
        this->m_clientsActionNameTranslator.emplace(lineS[0], actionNameTranslator);
        this->m_clientsActionNameTranslatorReverse.emplace(lineS[0], actionNameTranslatorReverse);
    }

    /*
    printf("Tables:\n");
    for(auto it : this->m_clientsTableNameTranslatorReverse){
        printf("%s:\n", it.first.c_str());
        for(auto it2 : it.second)
            printf("%s - %s\n", it2.first.c_str(), it2.second.c_str());
        printf("\n");
    }

    printf("Actions:\n");
    for(auto it : this->m_clientsActionNameTranslatorReverse){
        printf("%s:\n", it.first.c_str());
        for(auto it2 : it.second)
            printf("%s - %s\n", it2.first.c_str(), it2.second.c_str());
        printf("\n");
    }
    */
}

std::vector<std::string> RuntimeCFG::GetClientNames() const{
    std::vector<std::string> result;

    for(int i = 0; i < (int) this->m_clientsP4Info.size(); i++){
        for(auto& it : this->m_clientsP4Info){
            if(it.second.clientProgID == i + 1){ //Client IDs start at 1
                result.push_back(it.first);
                break;
            }
        }
        Assert(i < (int) result.size());
    }

    return result;
}

p4::v1::TableEntry RuntimeCFG::TranslateTableEntryClientToVirt(const std::string& client, const p4::v1::TableEntry* tableEntry){
    char buffer[BUFFER_SIZE];
    
    p4::v1::TableEntry result = *tableEntry;
    
    auto ait = this->m_clientsP4Info.find(client);
    auto ait2 = this->m_clientsTableNameTranslator.find(client);
    auto ait3 = this->m_programTableDatas.find(client);
    auto ait4 = this->m_clientsActionNameTranslator.find(client);
    if(ait == this->m_clientsP4Info.end() || ait2 == this->m_clientsTableNameTranslator.end() || ait3 == this->m_programTableDatas.end() || ait4 == this->m_clientsActionNameTranslator.end())
        throw RUNTIME_EXCEPT_CLIENT_CFG_NOT_FOUND_MSG;
    ClientData clientData = ait->second;
    std::map<std::string, std::string>* clientTableTranslator = &ait2->second;
    //std::map<std::string, ProgramTableData>* clientProgramTableDatas = &ait3->second;
    std::map<std::string, std::string>* clientActionTranslator = &ait4->second;

    //If the client sets an idle timeout, check if the table supports it
    if(tableEntry->idle_timeout_ns() != 0){
        std::vector<Property>* idleTimeoutBehaviorProperty = clientData.clientP4InfoDesc->getTablePropertyList((int) tableEntry->table_id())->getProperty("idle_timeout_behavior");
        Assert(idleTimeoutBehaviorProperty == NULL || idleTimeoutBehaviorProperty->size() == 1);
        if(idleTimeoutBehaviorProperty == NULL || idleTimeoutBehaviorProperty->at(0).propString == "NO_TIMEOUT"){
            throw RUNTIME_EXCEPT_IDLE_TIMEOUT_NOT_SUPPORTED;
        }
    }
    
    std::string tableName = clientData.clientP4InfoDesc->getTableNameFromID((int) tableEntry->table_id());
    auto it = clientTableTranslator->find(tableName);
    if(it == clientTableTranslator->end())
        throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
    int newID = this->m_virtP4Info.getTableIDFromName(it->second);
    result.set_table_id(newID);

    //Translate Action
    if(tableEntry->has_action()){
        if(tableEntry->action().has_action()){
            std::string actionName = clientData.clientP4InfoDesc->getActionNameFromID(tableEntry->action().action().action_id());
            auto it2 = clientActionTranslator->find(actionName);
            if(it2 == clientActionTranslator->end())
                throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
            int newActionID = this->m_virtP4Info.getActionIDFromName(it2->second);
            result.mutable_action()->mutable_action()->set_action_id(newActionID);
        }
    }
    
    //If the table is hidden shared, add program id to the match key
    auto vttIT = this->m_virtTableDatas.find(it->second);
    Assert(vttIT != this->m_virtTableDatas.end());
    if(vttIT->second.type == VTT_HIDDEN_SHARED){
        for(int j = 0; j < (int) result.match().size(); j++){
            p4::v1::FieldMatch* fm = result.mutable_match(j);
            fm->set_field_id(fm->field_id() + 1);
        }
        p4::v1::FieldMatch* newM = result.add_match();
        newM->set_field_id(1);
        //sprintf(buffer, "\\%03o", clientData.clientProgID);
        buffer[0] = (char) clientData.clientProgID;
        buffer[1] = '\0';
        newM->mutable_exact()->set_value(std::string(buffer));
    }

    return result;
}

p4::v1::TableEntry RuntimeCFG::TranslateTableEntryVirtToClient(const p4::v1::TableEntry* tableEntry, int* resultClientID){
    p4::v1::TableEntry result = *tableEntry;
    *resultClientID = -1;

    std::string tableName = this->m_virtP4Info.getTableNameFromID((int) tableEntry->table_id());
    std::string clientName;

    //If the table is hidden shared, get the program id from the match key
    auto vttIT = this->m_virtTableDatas.find(tableName);
    Assert(vttIT != this->m_virtTableDatas.end());
    if(vttIT->second.type == VTT_HIDDEN_SHARED){
        int removeIDX = -1;
        for(int j = 0; j < (int) tableEntry->match().size(); j++){
            p4::v1::FieldMatch* fm = result.mutable_match(j);
            fm->set_field_id(fm->field_id() - 1);
            if(fm->field_id() == 0){
                removeIDX = j;
                *resultClientID = Utils::byteStringToInt(fm->exact().value());
            }
        }
        if(removeIDX != -1)
            result.mutable_match()->erase(result.mutable_match()->begin() + removeIDX);
    }
    else{
        Assert(vttIT->second.type == VTT_NOT_SHARED);
        Assert(vttIT->second.programIDs.size() == 1);
        *resultClientID = *vttIT->second.programIDs.begin() + 1; 
    }

    Assert(*resultClientID != -1);
    for(const auto& cd : this->m_clientsP4Info)
        if(cd.second.clientProgID == *resultClientID)
            clientName = cd.first;

    auto ait = this->m_clientsP4Info.find(clientName);
    auto ait2 = this->m_clientsTableNameTranslatorReverse.find(clientName);
    auto ait3 = this->m_programTableDatas.find(clientName);
    auto ait4 = this->m_clientsActionNameTranslatorReverse.find(clientName);
    if(ait == this->m_clientsP4Info.end() || ait2 == this->m_clientsTableNameTranslatorReverse.end() || ait3 == this->m_programTableDatas.end() || ait4 == this->m_clientsActionNameTranslatorReverse.end())
        throw RUNTIME_EXCEPT_CLIENT_CFG_NOT_FOUND_MSG;
    ClientData clientData = ait->second;
    std::map<std::string, std::string>* clientTableTranslatorReverse = &ait2->second;
    std::map<std::string, std::string>* clientActionTranslatorReverse = &ait4->second;
    
    auto it = clientTableTranslatorReverse->find(tableName);
    if(it == clientTableTranslatorReverse->end())
        throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
    
    int newID = clientData.clientP4InfoDesc->getTableIDFromName(it->second);
    result.set_table_id(newID);

    //Translate Action
    if(tableEntry->has_action()){
        if(tableEntry->action().has_action()){
            std::string actionName = this->m_virtP4Info.getActionNameFromID(tableEntry->action().action().action_id());
            auto it2 = clientActionTranslatorReverse->find(actionName);
            if(it2 == clientActionTranslatorReverse->end())
                throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
            
            int newActionID = clientData.clientP4InfoDesc->getActionIDFromName(it2->second);
            result.mutable_action()->mutable_action()->set_action_id(newActionID);
        }
    }
    
    return result;
}

p4::v1::WriteRequest RuntimeCFG::Translate(const std::string& client, const p4::v1::WriteRequest* request){
    char buffer[BUFFER_SIZE];

    p4::v1::WriteRequest result = *request;

    auto ait = this->m_clientsP4Info.find(client);
    auto ait2 = this->m_clientsTableNameTranslator.find(client);
    auto ait3 = this->m_programTableDatas.find(client);
    auto ait4 = this->m_clientsActionNameTranslator.find(client);
    if(ait == this->m_clientsP4Info.end() || ait2 == this->m_clientsTableNameTranslator.end() || ait3 == this->m_programTableDatas.end() || ait4 == this->m_clientsActionNameTranslator.end())
        throw RUNTIME_EXCEPT_CLIENT_CFG_NOT_FOUND_MSG;
    ClientData clientData = ait->second;
    std::map<std::string, std::string>* clientTableTranslator = &ait2->second;
    std::map<std::string, ProgramTableData>* clientProgramTableDatas = &ait3->second;
    std::map<std::string, std::string>* clientActionTranslator = &ait4->second;

    for(int i = 0; i < (int) result.mutable_updates()->size(); i++){
        p4::v1::Update* upd = result.mutable_updates(i);
        if(upd->mutable_entity()->has_table_entry()){
            p4::v1::TableEntry* tableEntry = upd->mutable_entity()->mutable_table_entry();
            
            //If the client sets an idle timeout, check if the table supports it
            if(tableEntry->idle_timeout_ns() != 0){
                std::vector<Property>* idleTimeoutBehaviorProperty = clientData.clientP4InfoDesc->getTablePropertyList((int) tableEntry->table_id())->getProperty("idle_timeout_behavior");
                Assert(idleTimeoutBehaviorProperty == NULL || idleTimeoutBehaviorProperty->size() == 1);
                if(idleTimeoutBehaviorProperty == NULL || idleTimeoutBehaviorProperty->at(0).propString == "NO_TIMEOUT"){
                    throw RUNTIME_EXCEPT_IDLE_TIMEOUT_NOT_SUPPORTED;
                }
            }
            
            std::string tableName = clientData.clientP4InfoDesc->getTableNameFromID((int) tableEntry->table_id());
            auto it = clientTableTranslator->find(tableName);
            if(it == clientTableTranslator->end())
                throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
            int newID = this->m_virtP4Info.getTableIDFromName(it->second);
            tableEntry->set_table_id(newID);

            //Translate Action
            if(tableEntry->has_action()){
                if(tableEntry->action().has_action()){
                    std::string actionName = clientData.clientP4InfoDesc->getActionNameFromID(tableEntry->action().action().action_id());
                    auto it2 = clientActionTranslator->find(actionName);
                    if(it2 == clientActionTranslator->end())
                        throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
                    int newActionID = this->m_virtP4Info.getActionIDFromName(it2->second);
                    tableEntry->mutable_action()->mutable_action()->set_action_id(newActionID);
                }
            }
            
            //If the table is hidden shared, add program id to the match key
            auto vttIT = this->m_virtTableDatas.find(it->second);
            Assert(vttIT != this->m_virtTableDatas.end());
            if(vttIT->second.type == VTT_HIDDEN_SHARED){
                for(int j = 0; j < (int) tableEntry->mutable_match()->size(); j++){
                    p4::v1::FieldMatch* fm = tableEntry->mutable_match(j);
                    fm->set_field_id(fm->field_id() + 1);
                }
                p4::v1::FieldMatch* newM = tableEntry->add_match();
                newM->set_field_id(1);
                //sprintf(buffer, "\\%03o", clientData.clientProgID);
                buffer[0] = (char) clientData.clientProgID;
                buffer[1] = '\0';
                newM->mutable_exact()->set_value(std::string(buffer));
            }

            auto ptdIT = clientProgramTableDatas->find(tableName);
            Assert(ptdIT != clientProgramTableDatas->end());
            if(upd->type() == p4::v1::Update_Type_INSERT && ptdIT->second.curProgSize >= ptdIT->second.maxProgSize)
                throw RUNTIME_EXCEPT_TABLE_FULL;
        }
    }

    result.mutable_election_id()->set_low(1);
    result.mutable_election_id()->set_high(0);
    return result;
}

p4::v1::ReadRequest RuntimeCFG::Translate(const std::string& client, const p4::v1::ReadRequest* request){
    char buffer[BUFFER_SIZE];

    p4::v1::ReadRequest result = *request;

    auto ait = this->m_clientsP4Info.find(client);
    auto ait2 = this->m_clientsTableNameTranslator.find(client);
    auto ait3 = this->m_programTableDatas.find(client);
    auto ait4 = this->m_clientsActionNameTranslator.find(client);
    if(ait == this->m_clientsP4Info.end() || ait2 == this->m_clientsTableNameTranslator.end() || ait3 == this->m_programTableDatas.end() || ait4 == this->m_clientsActionNameTranslator.end())
        throw RUNTIME_EXCEPT_CLIENT_CFG_NOT_FOUND_MSG;
    ClientData clientData = ait->second;
    std::map<std::string, std::string>* clientTableTranslator = &ait2->second;
    std::map<std::string, std::string>* clientActionTranslator = &ait4->second;

    for(int i = 0; i < (int) result.entities().size(); i++){
        p4::v1::Entity* ett = result.mutable_entities(i);
        if(ett->has_table_entry()){
            p4::v1::TableEntry* tableEntry = ett->mutable_table_entry();
            if(tableEntry->table_id() != 0){
                std::string tableName = clientData.clientP4InfoDesc->getTableNameFromID((int) tableEntry->table_id());
                auto it = clientTableTranslator->find(tableName);
                if(it == clientTableTranslator->end())
                    throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
                int newID = this->m_virtP4Info.getTableIDFromName(it->second);
                tableEntry->set_table_id(newID);
            }

            //Translate Action
            if(tableEntry->has_action()){
                if(tableEntry->action().has_action() && tableEntry->action().action().action_id() != 0){
                    std::string actionName = clientData.clientP4InfoDesc->getActionNameFromID(tableEntry->action().action().action_id());
                    auto it2 = clientActionTranslator->find(actionName);
                    if(it2 == clientActionTranslator->end())
                        throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
                    int newActionID = this->m_virtP4Info.getActionIDFromName(it2->second);
                    tableEntry->mutable_action()->mutable_action()->set_action_id(newActionID);
                }
            }
            
            //If the table is hidden shared, add program id to the match key
            if(tableEntry->table_id() != 0){
                std::string tableName = this->m_virtP4Info.getTableNameFromID((int) tableEntry->table_id());
                auto vttIT = this->m_virtTableDatas.find(tableName);
                Assert(vttIT != this->m_virtTableDatas.end());
                if(vttIT->second.type == VTT_HIDDEN_SHARED){
                    for(int j = 0; j < (int) tableEntry->mutable_match()->size(); j++){
                        p4::v1::FieldMatch* fm = tableEntry->mutable_match(j);
                        fm->set_field_id(fm->field_id() + 1);
                    }
                    p4::v1::FieldMatch* newM = tableEntry->add_match();
                    newM->set_field_id(1);
                    //sprintf(buffer, "\\%03o", clientData.clientProgID);
                    buffer[0] = (char) clientData.clientProgID;
                    buffer[1] = '\0';
                    newM->mutable_exact()->set_value(std::string(buffer));
                }
            }
        }
    }

    return result;
}

p4::v1::ReadResponse RuntimeCFG::Translate(const std::string& client, const p4::v1::ReadResponse* response){
    char buffer[BUFFER_SIZE];

    p4::v1::ReadResponse result = *response;

    auto ait = this->m_clientsP4Info.find(client);
    auto ait2 = this->m_clientsTableNameTranslatorReverse.find(client);
    auto ait3 = this->m_programTableDatas.find(client);
    auto ait4 = this->m_clientsActionNameTranslatorReverse.find(client);
    if(ait == this->m_clientsP4Info.end() || ait2 == this->m_clientsTableNameTranslatorReverse.end() || ait3 == this->m_programTableDatas.end() || ait4 == this->m_clientsActionNameTranslatorReverse.end())
        throw RUNTIME_EXCEPT_CLIENT_CFG_NOT_FOUND_MSG;
    ClientData clientData = ait->second;
    std::map<std::string, std::string>* clientTableTranslatorReverse = &ait2->second;
    std::map<std::string, std::string>* clientActionTranslatorReverse = &ait4->second;

    for(int i = 0; i < (int) result.mutable_entities()->size(); i++){
        p4::v1::Entity* ett = result.mutable_entities(i);
        
        //If we set it to true, the entity is removed after "translation"
        bool removeEntity = false;
        
        if(ett->has_table_entry()){
            p4::v1::TableEntry* tableEntry = ett->mutable_table_entry();
            std::string tableName = this->m_virtP4Info.getTableNameFromID((int) tableEntry->table_id());
            auto it = clientTableTranslatorReverse->find(tableName);
            if(it == clientTableTranslatorReverse->end())
                removeEntity = true;
            else{
                int newID = clientData.clientP4InfoDesc->getTableIDFromName(it->second);
                tableEntry->set_table_id(newID);
            }

            //Translate Action
            if(tableEntry->has_action()){
                if(tableEntry->action().has_action()){
                    std::string actionName = this->m_virtP4Info.getActionNameFromID(tableEntry->action().action().action_id());
                    auto it2 = clientActionTranslatorReverse->find(actionName);
                    if(it2 == clientActionTranslatorReverse->end())
                        removeEntity = true;
                    else{
                        int newActionID = clientData.clientP4InfoDesc->getActionIDFromName(it2->second);
                        tableEntry->mutable_action()->mutable_action()->set_action_id(newActionID);
                    }
                }
            }
            
            //If the table is hidden shared, remove program id from the match key
            if(!removeEntity){
                auto vttIT = this->m_virtTableDatas.find(it->first);
                Assert(vttIT != this->m_virtTableDatas.end());
                if(vttIT->second.type == VTT_HIDDEN_SHARED){
                    int removeIDX = -1;
                    for(int j = 0; j < (int) tableEntry->mutable_match()->size(); j++){
                        p4::v1::FieldMatch* fm = tableEntry->mutable_match(j);
                        fm->set_field_id(fm->field_id() - 1);
                        if(fm->field_id() == 0){
                            removeIDX = j;
                            buffer[0] = (char) clientData.clientProgID;
                            buffer[1] = '\0';
                            if(fm->has_exact() && fm->exact().value().c_str() != std::string(buffer))
                                removeEntity = true;
                        }
                    }
                    if(removeIDX != -1)
                        tableEntry->mutable_match()->erase(tableEntry->mutable_match()->begin() + removeIDX);
                }
            }
        }
        if(removeEntity){
            result.mutable_entities()->erase(result.mutable_entities()->begin() + i);
            i--;
        }
    }

    return result;
}



VirtTableData RuntimeCFG::GetVirtTableDataForTable(const std::string& tableName){
    //TODO: This can fail
    return this->m_virtTableDatas.find(tableName)->second;
}

VirtTableData RuntimeCFG::GetVirtTableDataForTable(const int tableID){
    return this->GetVirtTableDataForTable(this->m_virtP4Info.getTableNameFromID(tableID));
}

void RuntimeCFG::IncreaseTableUsageForRequests(const std::string& client, const p4::v1::WriteRequest* request){
    auto ait = this->m_clientsP4Info.find(client);
    auto ait2 = this->m_clientsTableNameTranslator.find(client);
    auto ait3 = this->m_programTableDatas.find(client);
    auto ait4 = this->m_clientsActionNameTranslator.find(client);
    
    Assert(ait != this->m_clientsP4Info.end());
    Assert(ait2 != this->m_clientsTableNameTranslator.end());
    Assert(ait3 != this->m_programTableDatas.end());
    Assert(ait4 != this->m_clientsActionNameTranslator.end());

    ClientData clientData = ait->second;
    
    std::map<std::string, ProgramTableData>* clientProgramTableDatas = &ait3->second;
    p4::v1::WriteRequest result = *request;

    for(int i = 0; i < (int) request->updates().size(); i++){
        const p4::v1::Update upd = request->updates(i);
        if(upd.entity().has_table_entry()){
            const p4::v1::TableEntry tableEntry = upd.entity().table_entry();
            std::string tableName = clientData.clientP4InfoDesc->getTableNameFromID((int) tableEntry.table_id());

            auto ptdIT = clientProgramTableDatas->find(tableName);
            Assert(ptdIT != clientProgramTableDatas->end());

            if(upd.type() == p4::v1::Update_Type_INSERT){
                ptdIT->second.curProgSize++;
            }
        }
    }
}

std::vector<std::string> VirtTableDataAttrListSplit(const std::string& attrList){
    std::vector<std::string> result;
    std::string aux;
    int inList = 0;

    int i = 0;
    while(i < (int) attrList.size()){
        if(attrList[i] == ',' && inList == 0){
            if(aux.size() > 0){
                result.push_back(aux);
                aux.clear();
            }
        }
        else{
            if(attrList[i] != ' ' || inList > 0)
                aux.push_back(attrList[i]);
            if(attrList[i] == '[')
                inList++;
            else if(attrList[i] == ']')
                inList--;
        }
        i++;
    }
    if(aux.size() > 0)
        result.push_back(aux);

    return result;
}

VirtTableData RuntimeCFG::getVirtTableDataFromLine(const std::string& line){
    VirtTableData result;

    int nameEnd = line.find(" ");
    if(nameEnd == (int) std::string::npos || nameEnd >= (int) line.size() - 1){
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }
    result.name = line.substr(0, nameEnd);

    std::string attrList = line.substr(nameEnd + 1);
    if(attrList.size() < 2 || attrList[0] != '[' || attrList[attrList.size() - 1] != ']'){
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }
    attrList.pop_back();
    attrList.erase(attrList.begin());

    std::vector<std::string> attrListS = VirtTableDataAttrListSplit(attrList);
    if(attrListS[0] == "HIDDEN_SHARED"){
        result.type = VTT_HIDDEN_SHARED;
    }
    else if(attrListS[0] == "NOT_SHARED"){
        result.type = VTT_NOT_SHARED;
    }
    else if(attrListS[0] == "SHARED"){
        result.type = VTT_SHARED;
    }
    else{
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }
    std::vector<std::string> programIDSListS = IPLBase::split(attrListS[1]);
    if(programIDSListS.size() == 0){
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }
    for(int i = 0; i < (int) programIDSListS.size(); i++){
        while(programIDSListS[i].size() > 0 && (programIDSListS[i][0] == '[' || programIDSListS[i][0] == ','))
            programIDSListS[i].erase(programIDSListS[i].begin());
        while(programIDSListS[i].size() > 0 && (programIDSListS[i][programIDSListS[i].size() - 1] == ']' || programIDSListS[i][programIDSListS[i].size() - 1] == ','))
            programIDSListS[i].pop_back();
        int pid = IPLBase::intFromStr(programIDSListS[i].c_str());
        if(pid < 0){
            fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
            exit(1);
        }
        result.programIDs.insert(pid);
    }
    result.totalSize = IPLBase::intFromStr(attrListS[2].c_str());
    if(result.totalSize < 0){
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }

    return result;
}

ProgramTableData RuntimeCFG::getProgramTableDataFromLine(const std::string& line){
    ProgramTableData result;

    int nameEnd = line.find(" ");
    if(nameEnd == (int) std::string::npos || nameEnd >= (int) line.size() - 1){
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }
    result.name = line.substr(0, nameEnd);

    std::string attrList = line.substr(nameEnd + 1);
    if(attrList.size() < 2 || attrList[0] != '[' || attrList[attrList.size() - 1] != ']'){
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }
    attrList.pop_back();
    attrList.erase(attrList.begin());

    std::vector<std::string> attrListSplit = IPLBase::split(attrList);
    if(attrListSplit.size() != NUM_VIRT_TABLE_PROPERTIES){
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }
    for(int i = 0; i < (int) attrListSplit.size(); i++)
        if(attrListSplit[i][attrListSplit[i].size() - 1] == ',')
            attrListSplit[i].pop_back();

    result.virtName = attrListSplit[1];
    if(attrListSplit[2] == "true")
        result.isSharedOwner = true;
    else if(attrListSplit[2] == "false")
        result.isSharedOwner = false;
    else {
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }
    if(attrListSplit[3] == "true")
        result.canWrite = true;
    else if(attrListSplit[3] == "false")
        result.canWrite = false;
    else {
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }
    result.curProgSize = 0;
    result.maxProgSize = IPLBase::intFromStr(attrListSplit[4].c_str());
    if(result.maxProgSize <= 0){
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }

    return result;
}

ProgramActionData RuntimeCFG::getProgramActionDataFromLine(const std::string& line){
    ProgramActionData result;

    std::vector<std::string> lineS = IPLBase::split(line);
    if(lineS.size() != 3){
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }

    result.name = lineS[0];
    result.virtName = lineS[lineS.size() - 1];

    return result;
}