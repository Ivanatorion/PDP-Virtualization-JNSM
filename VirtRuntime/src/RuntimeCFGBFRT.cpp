#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../proto/bfruntime.grpc.pb.h"
#include "../include/BFRTInfoDesc.h"
#include "../include/RuntimeCFGBFRT.h"

RuntimeCFGBFRT::RuntimeCFGBFRT(const std::string& clientCFGPath, const std::string& virtCFGPath, const std::string& virtBFRTInfoPath){
    if(clientCFGPath == "" || virtCFGPath == "" || virtBFRTInfoPath == ""){
        printf("Warning: No initial runtime configuration\n");
    }
    else {
        this->Update(clientCFGPath, virtCFGPath, virtBFRTInfoPath);
    }
}

RuntimeCFGBFRT::~RuntimeCFGBFRT(){
    
}

void RuntimeCFGBFRT::Update(const std::string& clientCFGPath, const std::string& virtCFGPath, const std::string& virtBFRTInfoPath){
    printf("Loading RuntimeCFG-BFRT from \"%s\" | \"%s\" | \"%s\"\n", clientCFGPath.c_str(), virtCFGPath.c_str(), virtBFRTInfoPath.c_str());

    m_clientNames.clear();
    m_clientsBFInfo.clear();
    m_virtTableDatas.clear();
    m_clientsTableTranslator.clear();
    m_clientsActionTranslator.clear();
    m_clientsRegisterTranslator.clear();
    m_clientsAuxFlagRegisterSizes.clear();
    m_clientsAuxFlagRegisterStart.clear();
    m_clientsPorts.clear();

    uint32_t clientID;

    //Load virt BFRT info
    m_virtBFInfo.LoadFromFile(virtBFRTInfoPath);

    //PrintBFInfo(m_virtBFInfo);
    std::vector<std::string> clientCFGLines = IPLBase::readFileLines(clientCFGPath);
    clientID = 1;

    std::vector<size_t> registerCurrentSize;
    for(const std::string& line : clientCFGLines){
        const std::vector<std::string> lineSplit = IPLBase::split(line, ' ');
        m_clientNames.push_back(lineSplit[0]);
        BFRTInfoDesc clientBFInfo(lineSplit[1]);
        m_clientsBFInfo.push_back(std::move(clientBFInfo));

        //Parse aux flag register sizes
        size_t endFlagRegisterSizeArray = 2;
        while(endFlagRegisterSizeArray < lineSplit.size() && lineSplit[endFlagRegisterSizeArray].back() != ']')
            endFlagRegisterSizeArray++;
        if(endFlagRegisterSizeArray == lineSplit.size()){
            fprintf(stderr, "Error: Malformed file \"%s\"\n", virtCFGPath.c_str());
            exit(1);
        }
        endFlagRegisterSizeArray++;

        std::string auxN;
        std::vector<size_t> registerSizes;
        std::vector<size_t> registerStarts;
        for(size_t s = 2; s < endFlagRegisterSizeArray; s++){
            auxN.clear();
            for(size_t i = 0; i < lineSplit[s].size(); i++){
                if(lineSplit[s][i] >= '0' && lineSplit[s][i] <= '9')
                    auxN.push_back(lineSplit[s][i]);
            }
            if(auxN.size() > 0){
                registerSizes.push_back((size_t) IPLBase::intFromStr(auxN.c_str()));
                int idx = s - 2;
                if(registerCurrentSize.size() <= (size_t) idx){
                    registerCurrentSize.push_back(0);
                }
                registerStarts.push_back(registerCurrentSize[idx]);
                registerCurrentSize[idx] = registerCurrentSize[idx] + registerSizes.back();
            }
        }

        m_clientsAuxFlagRegisterSizes.emplace(clientID, std::move(registerSizes));
        m_clientsAuxFlagRegisterStart.emplace(clientID, std::move(registerStarts));
        
        //Parse ports
        size_t startPortsArray = endFlagRegisterSizeArray;
        size_t endPortsArray = startPortsArray;
        while(endPortsArray < lineSplit.size() && lineSplit[endPortsArray].back() != ']')
            endPortsArray++;
        if(endPortsArray == lineSplit.size()){
            fprintf(stderr, "Error: Malformed file \"%s\"\n", virtCFGPath.c_str());
            exit(1);
        }
        endPortsArray++;
        std::vector<uint32_t> ports;
        for(size_t s = startPortsArray; s < endPortsArray; s++){
            auxN.clear();
            for(size_t i = 0; i < lineSplit[s].size(); i++){
                if(lineSplit[s][i] >= '0' && lineSplit[s][i] <= '9')
                    auxN.push_back(lineSplit[s][i]);
            }
            if(auxN.size() > 0){
                ports.push_back((uint32_t) IPLBase::intFromStr(auxN.c_str()));
            }
        }

        m_clientsPorts.emplace(clientID, std::move(ports));

        clientID++;
    }

    std::vector<std::string> virtCFGLines = IPLBase::readFileLines(virtCFGPath);

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
    int VCFGRegistersStartLine = 0;
    while(VCFGRegistersStartLine < (int) virtCFGLines.size() && virtCFGLines[VCFGRegistersStartLine] != "Registers:")
        VCFGRegistersStartLine++;
    if(VCFGRegistersStartLine == (int) virtCFGLines.size() || VCFGRegistersStartLine < VCFGActionsStartLine){
        fprintf(stderr, "Error: Malformed file \"%s\"\n", virtCFGPath.c_str());
        exit(1);
    }

    int i;

    i = VCFGVTablesStartLine + 1;
    
    while(i < VCFGSharedTablesStartLine){
        if(virtCFGLines[i].size() > 0){
            VirtTableDataBFRT vtd = RuntimeCFGBFRT::GetVirtTableDataFromLineBFRT(virtCFGLines[i]);
            m_virtTableDatas.emplace("VirtPipe." + vtd.name, vtd);
        }
        i++;
    }

    i = VCFGSharedTablesStartLine + 1;
    clientID = 1;
    while(i < VCFGActionsStartLine){
        m_clientsTableTranslator.emplace(clientID, std::unordered_map<uint64_t, uint64_t>{});
        m_clientsTableNameTranslator.emplace(clientID, std::unordered_map<std::string, std::string>{});
        m_clientsTableNameTranslatorReverse.emplace(clientID, std::unordered_map<std::string, std::string>{});

        while(virtCFGLines[i].back() != ':')
            i++;
        i++;

        auto it = m_clientsTableTranslator.find(clientID);
        auto itName = m_clientsTableNameTranslator.find(clientID);
        auto itNameReverse = m_clientsTableNameTranslatorReverse.find(clientID);
        size_t space = virtCFGLines[i].find(' ');
        while(space != std::string::npos){
            std::string tableName = "pipe." + virtCFGLines[i].substr(0, space);
            uint64_t tableID = m_clientsBFInfo[clientID-1].GetTableID(tableName);
            Assert(tableID != 0);

            std::vector<std::string> attrList = IPLBase::split(virtCFGLines[i].substr(space + 1));
            std::string virtTableName = "VirtPipe." + attrList[1].substr(0, attrList[1].size() - 1);
            uint64_t virtTableID = m_virtBFInfo.GetTableID(virtTableName);
            Assert(virtTableID != 0);
            it->second.emplace(tableID, virtTableID);
            itName->second.emplace(tableName, virtTableName);
            itNameReverse->second.emplace(virtTableName, tableName);

            i++;
            space = virtCFGLines[i].find(' ');
        }

        clientID++;
    }

    i = VCFGActionsStartLine + 1;
    clientID = 1;
    while(i < VCFGRegistersStartLine){
        m_clientsActionTranslator.emplace(clientID, std::unordered_map<uint64_t, uint64_t>{});
        m_clientsActionNameTranslator.emplace(clientID, std::unordered_map<std::string, std::string>{});
        m_clientsActionNameTranslatorReverse.emplace(clientID, std::unordered_map<std::string, std::string>{});

        while(virtCFGLines[i].back() != ':')
            i++;
        i++;

        auto it = m_clientsActionTranslator.find(clientID);
        auto itName = m_clientsActionNameTranslator.find(clientID);
        auto itNameReverse = m_clientsActionNameTranslatorReverse.find(clientID);
        size_t space = virtCFGLines[i].find(' ');
        while(space != std::string::npos){
            std::string actionName = virtCFGLines[i].substr(0, space);
            uint64_t actionID = m_clientsBFInfo[clientID-1].GetActionID(actionName);
            Assert(actionID != 0);

            std::string virtActionName = virtCFGLines[i].substr(virtCFGLines[i].find(':') + 2);
            uint64_t virtActionID = m_virtBFInfo.GetActionID(virtActionName);
            Assert(virtActionID != 0);
            it->second.emplace(actionID, virtActionID);
            itName->second.emplace(actionName, virtActionName);
            itNameReverse->second.emplace(virtActionName, actionName);

            i++;
            space = virtCFGLines[i].find(' ');
        }

        clientID++;
    }

    i = VCFGRegistersStartLine + 1;
    clientID = 1;
    while(i < (int) virtCFGLines.size()){
        m_clientsRegisterTranslator.emplace(clientID, std::unordered_map<uint64_t, uint64_t>{});
        m_clientsRegisterIDs.emplace(clientID, std::vector<uint64_t>{});

        while(virtCFGLines[i].back() != ':')
            i++;
        i++;

        auto it = m_clientsRegisterTranslator.find(clientID);
        auto itIDs = m_clientsRegisterIDs.find(clientID);
        size_t space = virtCFGLines[i].find(' ');
        while(space != std::string::npos){
            std::string tableName = "pipe." + virtCFGLines[i].substr(0, space);
            uint64_t tableID = m_clientsBFInfo[clientID-1].GetTableID(tableName);
            Assert(tableID != 0);

            std::string virtTableName = "VirtPipe." + virtCFGLines[i].substr(space + 1);
            uint64_t virtTableID = m_virtBFInfo.GetTableID(virtTableName);
            Assert(virtTableID != 0);
            it->second.emplace(tableID, virtTableID);
            itIDs->second.push_back(tableID);

            i++;
            space = i < (int) virtCFGLines.size() ? virtCFGLines[i].find(' ') : std::string::npos;
        }

        clientID++;
    }
}

BFRTInfoDesc* RuntimeCFGBFRT::GetClientBFRTInfoDesc(const std::string& client){
    size_t clientIDX = 0;
    while(clientIDX < m_clientNames.size() && m_clientNames[clientIDX] != client)
        clientIDX++;
    if(clientIDX == m_clientNames.size())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;

    return &m_clientsBFInfo[clientIDX];
}

bfrt_proto::WriteRequest RuntimeCFGBFRT::Translate(const std::string& client, const bfrt_proto::WriteRequest* request){
    char buffer[BUFFER_SIZE];

    bfrt_proto::WriteRequest result = *request;

    size_t clientIDX = 0;
    while(clientIDX < m_clientNames.size() && m_clientNames[clientIDX] != client)
        clientIDX++;
    if(clientIDX == m_clientNames.size())
        throw RUNTIME_EXCEPT_CLIENT_NOT_FOUND_MSG;

    clientIDX++; //Client ID is always +1

    auto ait1 = this->m_clientsTableTranslator.find(clientIDX);
    auto ait2 = this->m_clientsActionTranslator.find(clientIDX);
    auto ait3 = this->m_clientsRegisterTranslator.find(clientIDX);
    
    std::unordered_map<uint64_t, uint64_t>* clientTableTranslator = &ait1->second;
    std::unordered_map<uint64_t, uint64_t>* clientActionTranslator = &ait2->second;
    std::unordered_map<uint64_t, uint64_t>* clientRegisterTranslator = &ait3->second;

    for(int i = 0; i < (int) result.mutable_updates()->size(); i++){
        bfrt_proto::Update* upd = result.mutable_updates(i);
        if(upd->mutable_entity()->has_table_entry()){
            bfrt_proto::TableEntry* tableEntry = upd->mutable_entity()->mutable_table_entry();
            
            //Translate Table / Register
            bool isRegister = false;
            auto it = clientTableTranslator->find(tableEntry->table_id());
            if(it == clientTableTranslator->end()){
                isRegister = true;
                it = clientRegisterTranslator->find(tableEntry->table_id());
                if(it == clientRegisterTranslator->end())
                    throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
            }
            uint32_t newID = static_cast<uint32_t>(it->second);
            tableEntry->set_table_id(newID);

            //Translate Action
            if(!isRegister){
                auto it2 = clientActionTranslator->find(tableEntry->data().action_id());
                if(it2 == clientActionTranslator->end())
                    throw RUNTIME_EXCEPT_MALFORMED_REQUEST;
                uint32_t newActionID = it2->second;
                tableEntry->mutable_data()->set_action_id(newActionID);
            
                //If the table is hidden shared, add program id to the match key
                auto vttIT = this->m_virtTableDatas.find(m_virtBFInfo.GetTableName(it->second));
                Assert(vttIT != this->m_virtTableDatas.end());
                if(vttIT->second.type == VTTBF_HIDDEN_SHARED){
                    for(int j = 0; j < (int) tableEntry->mutable_key()->fields_size(); j++){
                        bfrt_proto::KeyField* f = tableEntry->mutable_key()->mutable_fields(j);
                        f->set_field_id(f->field_id() + 1);
                    }
                    bfrt_proto::KeyField* newM = tableEntry->mutable_key()->add_fields();
                    newM->set_field_id(1);
                    //sprintf(buffer, "\\%03o", clientData.clientProgID);
                    buffer[0] = (char) clientIDX;
                    buffer[1] = '\0';
                    newM->mutable_exact()->set_value(std::string(buffer));
                }
            }
            else{
                //TODO: Need to check something?
            }
        }
    }

    result.set_client_id(BFRT_CLIENT_ID);
    return result;
}

bfrt_proto::ReadRequest RuntimeCFGBFRT::Translate(const std::string& client, const bfrt_proto::ReadRequest* request){
    bfrt_proto::ReadRequest result = *request;

    return result;
}

bfrt_proto::ReadResponse RuntimeCFGBFRT::Translate(const std::string& client, const bfrt_proto::ReadResponse* response){
    bfrt_proto::ReadResponse result = *response;

    return result;
}

std::vector<std::string> VirtTableDataAttrListSplitBF(const std::string& attrList){
    std::vector<std::string> result;
    std::string aux;
    int inList = 0;

    int i = 1;
    while(i < (int) attrList.size() - 1){
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

VirtTableDataBFRT RuntimeCFGBFRT::GetVirtTableDataFromLineBFRT(const std::string& line){
    VirtTableDataBFRT vtd;
    
    const int space = line.find(' ');
    vtd.name = line.substr(0, space);
    std::string attr = line.substr(space + 1);
    
    std::vector<std::string> attrList = VirtTableDataAttrListSplitBF(attr);

    if(attrList[0] == "NOT_SHARED")
        vtd.type = VTTBF_NOT_SHARED;
    else if(attrList[0] == "SHARED")
        vtd.type = VTTBF_SHARED;
    else if(attrList[0] == "HIDDEN_SHARED")
        vtd.type = VTTBF_HIDDEN_SHARED;
    else{
        fprintf(stderr, "Error: Malformed virt table data line \"%s\"\n", line.c_str());
        exit(1);
    }

    std::vector<std::string> programIDSListS = IPLBase::split(attrList[1]);
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
        vtd.programIDs.insert(pid);
    }
    vtd.totalSize = IPLBase::intFromStr(attrList[2].c_str());
    if(vtd.totalSize < 0){
        fprintf(stderr, "Error: Malformed CFG line - %s\n", line.c_str());
        exit(1);
    }

    return vtd;
}
