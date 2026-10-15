#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../proto/p4runtime.grpc.pb.h"
#include "../proto/bfruntime.grpc.pb.h"
#include "../include/BFRTManager.h"
#include "../include/Utils.h"

BFRTManager::BFRTManager(std::shared_ptr<bfrt_proto::BfRuntime::Stub> stub, std::shared_ptr<grpc::ClientReaderWriter<bfrt_proto::StreamMessageRequest, bfrt_proto::StreamMessageResponse>> stream) {
    m_stub = stub;
    m_stream = stream;
}

BFRTManager::~BFRTManager() {
}

void BFRTManager::LoadFromFile(const std::string& filePath) {
    FILE *fp = fopen(filePath.c_str(), "rb");
    if(!fp){
        fprintf(stderr, "Could not open file: %s\n", filePath.c_str());
        exit(1);
    }

    size_t fileSize;
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buffer = new char[fileSize + 1];
    fread(buffer, 1, fileSize, fp);
    buffer[fileSize] = '\0';
    fclose(fp);

    char *ptr = strstr(buffer, "tables");
    char *learn_filters_ptr = strstr(buffer, "learn_filters");

    ptr = strstr(ptr, "\"name\"");
    while(ptr != NULL && (learn_filters_ptr == NULL || ptr < learn_filters_ptr)){
        //Read Actions
        char *actionsPTR = strstr(ptr, "\"action_specs\"");
        if(actionsPTR != NULL && actionsPTR < strstr(ptr, "\"attributes\"")){
            char *startActions = strstr(actionsPTR, "[");
            char *endActions = Utils::findMatchingDelimiter(startActions);
            char *ptr2 = strstr(startActions, "\"id\"");
            while(ptr2 != NULL && ptr2 < endActions){
                ptr2 = strstr(ptr2, ":") + 2;
                char *endInfoPTR = strstr(ptr2, ",");
                std::string actionIDStr(ptr2, endInfoPTR - ptr2);
                ptr2 = strstr(ptr2, "\"name\"");
                ptr2 = strstr(ptr2, ":") + 3;
                endInfoPTR = strstr(ptr2, "\"");
                std::string actionNameStr(ptr2, endInfoPTR - ptr2);

                //Skip data
                ptr2 = strstr(ptr2, "\"data\"");
                ptr2 = strstr(ptr2, "[");
                ptr2 = Utils::findMatchingDelimiter(ptr2);

                BFActionInfo actionInfo;
                actionInfo.actionID = std::stoul(actionIDStr);
                actionInfo.actionName = actionNameStr;
                m_actionInfoList.push_back(actionInfo);

                ptr2 = strstr(ptr2, "\"id\"");
            }
        }

        ptr = strstr(ptr, ":") + 3;
        char* endPtr = strstr(ptr, "\"");
        std::string tableName(ptr, endPtr - ptr);
        ptr = strstr(endPtr, "id");
        ptr = strstr(ptr, ":") + 2;
        endPtr = strstr(ptr, ",");
        std::string tableIDStr(ptr, endPtr - ptr);
        ptr = strstr(endPtr, "\"table_type\"");
        ptr = strstr(ptr, ":") + 3;
        endPtr = strstr(ptr, "\"");
        std::string tableType(ptr, endPtr - ptr);
        ptr = strstr(endPtr, "size");
        ptr = strstr(ptr, ":") + 2;
        endPtr = strstr(ptr, ",");
        std::string tableSizeStr(ptr, endPtr - ptr);
        
        if(tableType == "MatchAction_Direct" || tableType == "PreNode" || tableType == "PreMgid"){
            BFTableInfo tInfo;
            tInfo.tableID = std::stoul(tableIDStr);
            tInfo.size = std::stoul(tableSizeStr);
            tInfo.tableName = tableName;
            m_tableInfoList.push_back(tInfo);
        }
        else if(tableType == "Register"){
            BFRegisterInfo rInfo;
            rInfo.registerID = std::stoul(tableIDStr);
            rInfo.size = std::stoul(tableSizeStr);
            rInfo.registerName = tableName;
            m_regInfoList.push_back(rInfo);
        }

        ptr = strstr(ptr, "\"attributes\"");
        ptr = strstr(ptr, "\"name\"");
    }

    delete[] buffer;

    size_t i = 0;
    while(i < m_actionInfoList.size()){
        size_t j = m_actionInfoList.size() - 1;
        while(j > i){
            if(m_actionInfoList[i].actionID == m_actionInfoList[j].actionID){
                m_actionInfoList.erase(m_actionInfoList.begin() + j);
            }
            j--;
        }
        i++;
    }
}

void BFRTManager::LoadFromMemory(const std::string& memory) {
    char* buffer = new char[memory.size() + 1];
    memcpy(buffer, memory.c_str(), memory.size());
    buffer[memory.size()] = '\0';

    char *ptr = strstr(buffer, "tables");
    char *learn_filters_ptr = strstr(buffer, "learn_filters");

    ptr = strstr(ptr, "\"name\"");
    while(ptr != NULL && (learn_filters_ptr == NULL || ptr < learn_filters_ptr)){
        //Read Actions
        char *actionsPTR = strstr(ptr, "\"action_specs\"");
        if(actionsPTR != NULL && actionsPTR < strstr(ptr, "\"attributes\"")){
            char *startActions = strstr(actionsPTR, "[");
            char *endActions = Utils::findMatchingDelimiter(startActions);
            char *ptr2 = strstr(startActions, "\"id\"");
            while(ptr2 != NULL && ptr2 < endActions){
                ptr2 = strstr(ptr2, ":") + 2;
                char *endInfoPTR = strstr(ptr2, ",");
                std::string actionIDStr(ptr2, endInfoPTR - ptr2);
                ptr2 = strstr(ptr2, "\"name\"");
                ptr2 = strstr(ptr2, ":") + 3;
                endInfoPTR = strstr(ptr2, "\"");
                std::string actionNameStr(ptr2, endInfoPTR - ptr2);

                //Skip data
                ptr2 = strstr(ptr2, "\"data\"");
                ptr2 = strstr(ptr2, "[");
                ptr2 = Utils::findMatchingDelimiter(ptr2);

                BFActionInfo actionInfo;
                actionInfo.actionID = std::stoul(actionIDStr);
                actionInfo.actionName = actionNameStr;
                m_actionInfoList.push_back(actionInfo);

                ptr2 = strstr(ptr2, "\"id\"");
            }
        }

        ptr = strstr(ptr, ":") + 3;
        char* endPtr = strstr(ptr, "\"");
        std::string tableName(ptr, endPtr - ptr);
        ptr = strstr(endPtr, "id");
        ptr = strstr(ptr, ":") + 2;
        endPtr = strstr(ptr, ",");
        std::string tableIDStr(ptr, endPtr - ptr);
        ptr = strstr(endPtr, "\"table_type\"");
        ptr = strstr(ptr, ":") + 3;
        endPtr = strstr(ptr, "\"");
        std::string tableType(ptr, endPtr - ptr);
        ptr = strstr(endPtr, "size");
        ptr = strstr(ptr, ":") + 2;
        endPtr = strstr(ptr, ",");
        std::string tableSizeStr(ptr, endPtr - ptr);
        
        if(tableType == "MatchAction_Direct" || tableType == "PreNode" || tableType == "PreMgid"){
            BFTableInfo tInfo;
            tInfo.tableID = std::stoul(tableIDStr);
            tInfo.size = std::stoul(tableSizeStr);
            tInfo.tableName = tableName;
            m_tableInfoList.push_back(tInfo);
        }
        else if(tableType == "Register"){
            BFRegisterInfo rInfo;
            rInfo.registerID = std::stoul(tableIDStr);
            rInfo.size = std::stoul(tableSizeStr);
            rInfo.registerName = tableName;
            m_regInfoList.push_back(rInfo);
        }

        ptr = strstr(ptr, "\"attributes\"");
        ptr = strstr(ptr, "\"name\"");
    }

    delete[] buffer;

    size_t i = 0;
    while(i < m_actionInfoList.size()){
        size_t j = m_actionInfoList.size() - 1;
        while(j > i){
            if(m_actionInfoList[i].actionID == m_actionInfoList[j].actionID){
                m_actionInfoList.erase(m_actionInfoList.begin() + j);
            }
            j--;
        }
        i++;
    }
}

std::vector<BFTableEntry> BFRTManager::ReadTableEntries(const uint32_t tableID){
    std::vector<BFTableEntry> result;

    bfrt_proto::ReadRequest request;
    request.set_client_id(BFRT_CLIENT_ID);

    request.mutable_target()->set_device_id(0);
    request.mutable_target()->set_pipe_id(0xffff);

    bfrt_proto::Entity* entity = request.add_entities();
    entity->mutable_table_entry()->set_table_id(tableID);

    grpc::ClientContext cContext;
    bfrt_proto::ReadResponse response;
    std::unique_ptr<grpc::ClientReader<bfrt_proto::ReadResponse>> reader(
      m_stub->Read(&cContext, request));

    while(reader->Read(&response)){
        for(int i = 0; i < response.entities_size(); i++){
            const bfrt_proto::Entity& entity = response.entities(i);

            if(entity.has_table_entry()){
                BFTableEntry te;
                te.protoEntry.CopyFrom(entity.table_entry());
                
                const bfrt_proto::TableKey& key = entity.table_entry().key();
                for(int j = 0; j < key.fields_size(); j++){
                    const bfrt_proto::KeyField& field = key.fields(j);
                    te.keys.push_back(field.exact().value());
                }

                const bfrt_proto::TableData& data = entity.table_entry().data();
                te.action.actionID = data.action_id();
                for(const BFActionInfo& ai : m_actionInfoList)
                    if(ai.actionID == te.action.actionID)
                        te.action.actionName = ai.actionName;
                
                for(int j = 0; j < data.fields_size(); j++){
                    const bfrt_proto::DataField& field = data.fields(j);
                    te.actionParams.push_back(field.stream());
                }

                result.push_back(te);
            }
        }
    }

    return result;
}

std::vector<std::string> BFRTManager::ReadFromRegister(uint32_t regID, uint32_t index) {
    std::vector<std::string> values;

    grpc::ClientContext cContext;
    
    bfrt_proto::ReadRequest request;
    request.set_client_id(BFRT_CLIENT_ID);

    request.mutable_target()->set_device_id(0);
    request.mutable_target()->set_pipe_id(0xffff);
    
    bfrt_proto::Entity* entity = request.add_entities();
    bfrt_proto::TableEntry* tableEntry = entity->mutable_table_entry();
    tableEntry->set_table_id(regID);
    tableEntry->mutable_table_flags()->set_from_hw(true);

    bfrt_proto::TableKey* key = tableEntry->mutable_key();
    bfrt_proto::KeyField* fieldMatch = key->add_fields();
    fieldMatch->set_field_id(65557);
    fieldMatch->mutable_exact()->set_value(Utils::getUint32AsBytes(index));

    bfrt_proto::ReadResponse response;
    std::unique_ptr<grpc::ClientReader<bfrt_proto::ReadResponse>> reader(
      m_stub->Read(&cContext, request));

    while(reader->Read(&response)){
        //printf("ReadFromRegister Status: %d\nMSG: %s\n", response.error_code(), response.error_message().c_str());

        for(int i = 0; i < response.entities_size(); i++){
            const bfrt_proto::Entity& entity = response.entities(i);
            if(entity.has_table_entry()){
                const bfrt_proto::TableEntry& tableEntry = entity.table_entry();
                if(tableEntry.has_data()){
                    const bfrt_proto::TableData& data = tableEntry.data();
                    for(int j = 0; j < data.fields_size(); j++){
                        const bfrt_proto::DataField& field = data.fields(j);
                        if(field.field_id() == 1){ // value
                            values.push_back(field.stream());
                            if(regID == 2337768643)
                                printf("ReadFromRegister Index %u: Value: %d\n", index, (int) field.stream().c_str()[0]);
                        }
                    }
                }
            }
        }
    }

    grpc::Status status = reader->Finish();
    printf("ReadFromRegister Status: %d\nMSG: %s\n", status.error_code(), status.error_message().c_str());

    return values;
}

std::vector<std::vector<std::string>> BFRTManager::ReadFromRegister(uint32_t regID, std::shared_ptr<bfrt_proto::BfRuntime::Stub> stub) {
    std::vector<std::vector<std::string>> values;

    grpc::ClientContext cContext;
    
    bfrt_proto::ReadRequest request;
    request.set_client_id(BFRT_CLIENT_ID);

    request.mutable_target()->set_device_id(0);
    request.mutable_target()->set_pipe_id(0xffff);
    
    bfrt_proto::Entity* entity = request.add_entities();
    bfrt_proto::TableEntry* tableEntry = entity->mutable_table_entry();
    tableEntry->set_table_id(regID);
    tableEntry->mutable_table_flags()->set_from_hw(true);

    bfrt_proto::ReadResponse response;
    std::unique_ptr<grpc::ClientReader<bfrt_proto::ReadResponse>> reader(
      stub->Read(&cContext, request));

    while(reader->Read(&response)){
        //printf("ReadFromRegister Status: %d\nMSG: %s\n", response.error_code(), response.error_message().c_str());

        for(int i = 0; i < response.entities_size(); i++){
            const bfrt_proto::Entity& entity = response.entities(i);
            if(entity.has_table_entry()){
                const bfrt_proto::TableEntry& tableEntry = entity.table_entry();
                if(tableEntry.has_data()){
                    const bfrt_proto::TableData& data = tableEntry.data();
                    values.push_back(std::vector<std::string>());
                    std::vector<std::string>& entryValues = values.back();
                    for(int j = 0; j < data.fields_size(); j++){
                        const bfrt_proto::DataField& field = data.fields(j);
                        if(field.field_id() == 1){ // value
                            entryValues.push_back(field.stream());
                        }
                    }
                }
            }
        }
    }

    grpc::Status status = reader->Finish();
    printf("ReadFromRegister Status: %d\nMSG: %s\n", status.error_code(), status.error_message().c_str());

    return values;
}

std::vector<std::vector<std::string>> BFRTManager::ReadFromRegister(uint32_t regID) {
    return BFRTManager::ReadFromRegister(regID, m_stub);
}

void BFRTManager::WriteToRegister(uint32_t regID, uint32_t index, const std::string& value) {
    if(value.size() > 8)
        return;

    grpc::ClientContext cContext;
    
    bfrt_proto::WriteRequest request;
    request.set_client_id(BFRT_CLIENT_ID);

    request.mutable_target()->set_device_id(0);
    request.mutable_target()->set_pipe_id(0xffff);
    
    bfrt_proto::Update* update = request.add_updates();
    update->set_type(bfrt_proto::Update_Type_MODIFY);

    bfrt_proto::Entity* entity = update->mutable_entity();
    bfrt_proto::TableEntry* tableEntry = entity->mutable_table_entry();
    tableEntry->set_table_id(regID);

    bfrt_proto::TableKey* key = tableEntry->mutable_key();
    bfrt_proto::KeyField* fieldMatch = key->add_fields();
    fieldMatch->set_field_id(65557);
    fieldMatch->mutable_exact()->set_value(Utils::getUint32AsBytes(index));

    bfrt_proto::TableData *data = tableEntry->mutable_data();

    data->set_action_id(0);
    bfrt_proto::DataField* dataField = data->add_fields();
    dataField->set_field_id(1);
    dataField->set_stream(value);
    
    bfrt_proto::WriteResponse response;
    grpc::Status status = m_stub->Write(&cContext, request, &response);

    printf("WriteToRegister Status: %d\nMSG: %s\n", status.error_code(), status.error_message().c_str());
}

void BFRTManager::WriteToRegister(uint32_t regID, const std::vector<uint32_t>& indexes, const std::vector<std::string>& values) {
    if(indexes.size() != values.size())
        return;

     grpc::ClientContext cContext;
    
    bfrt_proto::WriteRequest request;
    request.set_client_id(BFRT_CLIENT_ID);

    request.mutable_target()->set_device_id(0);
    request.mutable_target()->set_pipe_id(0xffff);
    
    for(size_t i = 0; i < indexes.size(); i++){
        bfrt_proto::Update* update = request.add_updates();
        update->set_type(bfrt_proto::Update_Type_MODIFY);

        bfrt_proto::Entity* entity = update->mutable_entity();
        bfrt_proto::TableEntry* tableEntry = entity->mutable_table_entry();
        tableEntry->set_table_id(regID);

        bfrt_proto::TableKey* key = tableEntry->mutable_key();
        bfrt_proto::KeyField* fieldMatch = key->add_fields();
        fieldMatch->set_field_id(65557);
        fieldMatch->mutable_exact()->set_value(Utils::getUint32AsBytes(indexes[i]));

        bfrt_proto::TableData *data = tableEntry->mutable_data();

        data->set_action_id(0);
        bfrt_proto::DataField* dataField = data->add_fields();
        dataField->set_field_id(1);
        dataField->set_stream(values[i]);
    }
    
    bfrt_proto::WriteResponse response;
    grpc::Status status = m_stub->Write(&cContext, request, &response);

    printf("WriteToRegisters Status: %d\nMSG: %s\n", status.error_code(), status.error_message().c_str());
}

void BFRTManager::ErasePreNodeTableEntries() {
    const uint32_t tableID = GetTableID("$pre.node");
    if(tableID == 0){
        fprintf(stderr, "ErasePreNodeTableEntries: Could not find $pre.node table ID\n");
        throw "Could not find $pre.node table ID";
    }

    std::vector<BFTableEntry> entries = this->ReadTableEntries(tableID);

    if(entries.size() == 0){
        printf("No $pre.node table entries to erase\n");
        return;
    }

    grpc::ClientContext cContext;
    
    bfrt_proto::WriteRequest request;
    request.set_client_id(BFRT_CLIENT_ID);

    request.mutable_target()->set_device_id(0);
    request.mutable_target()->set_pipe_id(0xffff);
    
    for(const BFTableEntry& te : entries){
        if(te.keys.size() == 1){
            bfrt_proto::Update* update = request.add_updates();
            update->set_type(bfrt_proto::Update_Type_DELETE);

            bfrt_proto::Entity* entity = update->mutable_entity();
            bfrt_proto::TableEntry* tableEntry = entity->mutable_table_entry();
            tableEntry->set_table_id(tableID);

            bfrt_proto::TableKey* key = tableEntry->mutable_key();
            bfrt_proto::KeyField* fieldMatch = key->add_fields();
            fieldMatch->set_field_id(1);
            fieldMatch->mutable_exact()->set_value(te.keys.at(0));
        }
    }

    bfrt_proto::WriteResponse response;
    grpc::Status status = m_stub->Write(&cContext, request, &response);

    printf("ErasePreNodeTableEntries Status: %d\nMSG: %s\n", status.error_code(), status.error_message().c_str());

    if(status.error_code() != grpc::StatusCode::OK){
        throw "Error erasing $pre.node table entries";
    }
}

void BFRTManager::ErasePreMGIDTableEntries() {
    const uint32_t tableID = GetTableID("$pre.mgid");
    if(tableID == 0){
        fprintf(stderr, "ErasePreMGIDTableEntries: Could not find $pre.mgid table ID\n");
        throw "Could not find $pre.mgid table ID";
    }

    std::vector<BFTableEntry> entries = this->ReadTableEntries(tableID);

    if(entries.size() == 0){
        printf("No $pre.mgid table entries to erase\n");
        return;
    }

    grpc::ClientContext cContext;
    
    bfrt_proto::WriteRequest request;
    request.set_client_id(BFRT_CLIENT_ID);

    request.mutable_target()->set_device_id(0);
    request.mutable_target()->set_pipe_id(0xffff);
    
    for(const BFTableEntry& te : entries){
        if(te.keys.size() == 1){
            bfrt_proto::Update* update = request.add_updates();
            update->set_type(bfrt_proto::Update_Type_DELETE);

            bfrt_proto::Entity* entity = update->mutable_entity();
            bfrt_proto::TableEntry* tableEntry = entity->mutable_table_entry();
            tableEntry->set_table_id(tableID);

            bfrt_proto::TableKey* key = tableEntry->mutable_key();
            bfrt_proto::KeyField* fieldMatch = key->add_fields();
            fieldMatch->set_field_id(1);
            fieldMatch->mutable_exact()->set_value(te.keys.at(0));
        }
    }

    bfrt_proto::WriteResponse response;
    grpc::Status status = m_stub->Write(&cContext, request, &response);

    printf("ErasePreMGIDTableEntries Status: %d\nMSG: %s\n", status.error_code(), status.error_message().c_str());

    if(status.error_code() != grpc::StatusCode::OK){
        throw "Error erasing $pre.mgid table entries";
    }
}

bool BFRTManager::WriteToPreNode(uint32_t mcNodeID, uint16_t mcRID, const std::vector<uint8_t>& mcLAGID, const std::vector<uint32_t>& portList) {
    const uint32_t tableID = GetTableID("$pre.node");
    if(tableID == 0){
        fprintf(stderr, "WriteToPreNode: Could not find $pre.node table ID\n");
        throw "Could not find $pre.node table ID";
    }

    grpc::ClientContext cContext;
    
    bfrt_proto::WriteRequest request;
    request.set_client_id(BFRT_CLIENT_ID);

    request.mutable_target()->set_device_id(0);
    request.mutable_target()->set_pipe_id(0xffff);
    
    bfrt_proto::Update* update = request.add_updates();
    update->set_type(bfrt_proto::Update_Type_INSERT);

    bfrt_proto::Entity* entity = update->mutable_entity();
    bfrt_proto::TableEntry* tableEntry = entity->mutable_table_entry();
    tableEntry->set_table_id(tableID);

    bfrt_proto::TableKey* key = tableEntry->mutable_key();
    bfrt_proto::KeyField* fieldMatch = key->add_fields();
    fieldMatch->set_field_id(1); // mc_node_id
    fieldMatch->mutable_exact()->set_value(Utils::getUint32AsBytes(mcNodeID));

    bfrt_proto::TableData *data = tableEntry->mutable_data();

    data->set_action_id(0);
    
    bfrt_proto::DataField* dataField = data->add_fields();
    dataField->set_field_id(1); // mc_rid
    dataField->set_stream(Utils::getUint16AsBytes(mcRID));

    dataField = data->add_fields();
    dataField->set_field_id(2); // lag_vector
    bfrt_proto::DataField_IntArray* intArray = dataField->mutable_int_arr_val();
    for(uint8_t lagID : mcLAGID){
        intArray->add_val((uint32_t) lagID);
    }

    dataField = data->add_fields();
    dataField->set_field_id(3); // port_vector
    intArray = dataField->mutable_int_arr_val();
    for(uint32_t portN : portList){
        intArray->add_val(portN);
    }

    bfrt_proto::WriteResponse response;
    grpc::Status status = m_stub->Write(&cContext, request, &response);

    printf("WriteToPreNode Status: %d\nMSG: %s\n", status.error_code(), status.error_message().c_str());

    return status.error_code() == grpc::StatusCode::OK;
}

bool BFRTManager::WriteToPreMGID(uint16_t mgID, const std::vector<uint32_t>& mcNodeID, const std::vector<bool>& mcNodeL1XIDValid, const std::vector<uint16_t>& mcNodeL1XID) {
    const uint32_t tableID = GetTableID("$pre.mgid");
    if(tableID == 0)
        return false;

    grpc::ClientContext cContext;
    
    bfrt_proto::WriteRequest request;
    request.set_client_id(BFRT_CLIENT_ID);

    request.mutable_target()->set_device_id(0);
    request.mutable_target()->set_pipe_id(0xffff);
    
    bfrt_proto::Update* update = request.add_updates();
    update->set_type(bfrt_proto::Update_Type_INSERT);

    bfrt_proto::Entity* entity = update->mutable_entity();
    bfrt_proto::TableEntry* tableEntry = entity->mutable_table_entry();
    tableEntry->set_table_id(tableID);

    bfrt_proto::TableKey* key = tableEntry->mutable_key();
    bfrt_proto::KeyField* fieldMatch = key->add_fields();
    fieldMatch->set_field_id(1); // mgid
    fieldMatch->mutable_exact()->set_value(Utils::getUint16AsBytes(mgID));

    bfrt_proto::TableData *data = tableEntry->mutable_data();

    data->set_action_id(0);
    
    bfrt_proto::DataField* dataField = data->add_fields();
    dataField->set_field_id(1); // mc_node_id
    bfrt_proto::DataField_IntArray* intArray = dataField->mutable_int_arr_val();
    for(uint32_t nodeID : mcNodeID){
        intArray->add_val(nodeID);
    }

    dataField = data->add_fields();
    dataField->set_field_id(2); // mcNodeL1XIDValid
    bfrt_proto::DataField_BoolArray* boolArray = dataField->mutable_bool_arr_val();
    for(bool valid : mcNodeL1XIDValid){
        boolArray->add_val(valid);
    }

    dataField = data->add_fields();
    dataField->set_field_id(3); // mcNodeL1XID
    intArray = dataField->mutable_int_arr_val();
    for(uint32_t value : mcNodeL1XID){
        intArray->add_val(value);
    }

    bfrt_proto::WriteResponse response;
    grpc::Status status = m_stub->Write(&cContext, request, &response);

    printf("WriteToPreMGID Status: %d\nMSG: %s\n", status.error_code(), status.error_message().c_str());

    return status.error_code() == grpc::StatusCode::OK;
}

bfrt_proto::GetForwardingPipelineConfigResponse BFRTManager::GetFwdPipeline(){
    grpc::ClientContext cContext;
    
    bfrt_proto::GetForwardingPipelineConfigRequest request;
    request.set_device_id(0);
    request.set_client_id(BFRT_CLIENT_ID);
    bfrt_proto::GetForwardingPipelineConfigResponse response;
    grpc::Status status = m_stub->GetForwardingPipelineConfig(&cContext, request, &response);
    return response;
}

void BFRTManager::SendSubscribe(){
    bfrt_proto::StreamMessageRequest request;
    request.set_client_id(BFRT_CLIENT_ID);
    request.mutable_subscribe()->set_device_id(0);
    request.mutable_subscribe()->mutable_notifications()->set_enable_idletimeout_notifications(true);
    request.mutable_subscribe()->mutable_notifications()->set_enable_learn_notifications(true);
    m_stream->Write(request);
}

void BFRTManager::Bind(bfrt_proto::ForwardingPipelineConfig fpc){
    grpc::ClientContext cContext;
    
    bfrt_proto::SetForwardingPipelineConfigRequest request;
    request.set_device_id(0);
    request.set_client_id(BFRT_CLIENT_ID);
    bfrt_proto::ForwardingPipelineConfig* ptr = request.mutable_config()->Add();
    ptr->CopyFrom(fpc);
    bfrt_proto::SetForwardingPipelineConfigResponse response;
    grpc::Status status = m_stub->SetForwardingPipelineConfig(&cContext, request, &response);
  
    printf("Bind Status: %d\nMSG: %s\n", status.error_code(), status.error_message().c_str());
}