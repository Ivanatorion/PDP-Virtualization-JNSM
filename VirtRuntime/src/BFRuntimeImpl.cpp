#include "../include/pcheaders.h"
#include "../proto/p4runtime.grpc.pb.h"
#include "../proto/bfruntime.grpc.pb.h"
#include "../include/BFRTManager.h"
#include "../include/BFRTInfoDesc.h"
#include "../include/RuntimeCFGBFRT.h"
#include "../include/BFRuntimeImpl.h"
#include "../include/Utils.h"

#define SLEEP_MILLISECONDS(X) std::this_thread::sleep_for(std::chrono::milliseconds(X));

BFRuntimeImpl::BFRuntimeImpl(RuntimeCFGBFRT *rtcfg, BfRuntimeConnection s1connection){
    this->m_runtimeCFGBFRT = rtcfg;
    //this->m_stub = stub;
    //this->m_stream = stream;
    this->m_s1connection = s1connection;

    m_verbose = VERBOSE_ALL;

    this->ResetConnectionsAndKnownClients();

    std::filesystem::remove_all(TEMP_DIR_NAME);
    std::filesystem::create_directory(TEMP_DIR_NAME);
}

BFRuntimeImpl::~BFRuntimeImpl(){
    delete this->m_runtimeCFGBFRT;
    std::filesystem::remove_all(TEMP_DIR_NAME);
}

void BFRuntimeImpl::SendStreamResponseToClient(const int clientID, bfrt_proto::StreamMessageResponse* response){
    const std::string clientName = m_runtimeCFGBFRT->GetClientNames()[clientID - 1]; //Client IDs start at 1
    
    if(m_verbose >= VERBOSE_SIMPLE)
        printf("Sending stream response to client \"%s\"\n", clientName.c_str());
    auto openConnections = m_connections.find(clientName);
    if (openConnections == m_connections.end()){
        printf("- Client not found!\n");
        return;
    }

    ConnectionInfoBFRT* primary = NULL;
    for(auto& info : openConnections->second){
        if(info.second.isPrimary){
            primary = &info.second;
        }
    }

    if(primary == NULL){
        if(m_verbose >= VERBOSE_SIMPLE)
            printf("- No primary found for the client\n");
        return;
    }

    if(response->has_digest()){
        if(m_verbose >= VERBOSE_SIMPLE)
            printf("- Response is digest\n");

        primary->stream->Write(*(response));

        if(m_verbose >= VERBOSE_SIMPLE)
            printf("- Sent digest to %s...\n", primary->controllerID.c_str());
    }
}

grpc::Status BFRuntimeImpl::Write(grpc::ServerContext* context, const bfrt_proto::WriteRequest* request, bfrt_proto::WriteResponse* response) {
    const std::string client = std::string(context->auth_context().get()->GetPeerIdentity().begin()->begin(), context->auth_context().get()->GetPeerIdentity().begin()->end());
    const std::string controller = std::string(context->peer().c_str());

    if(m_verbose >= VERBOSE_SIMPLE){
        printf("Write from \"%s - %s\"\n", client.c_str(), controller.c_str());
        if(m_verbose >= VERBOSE_ALL)
        Utils::PrintWriteRequestBFRT(request, m_runtimeCFGBFRT->GetClientBFRTInfoDesc(client));
    }

    auto ClientMapIT = m_connections.find(client);
    if(ClientMapIT == this->m_connections.end()){
        if(m_verbose >= VERBOSE_SIMPLE)
        printf("- Client not found!\n");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Client not found \"" + client + "\".");
    }
    
    if(ClientMapIT->second.find(controller) == ClientMapIT->second.end()){
        if(m_verbose >= VERBOSE_SIMPLE)
        printf("- Controller did not started a stream channel yet!\n");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Stream channel not open.");
    }

    grpc::Status status = grpc::Status(grpc::StatusCode::OK, "");
    try{
        bfrt_proto::WriteRequest r = this->m_runtimeCFGBFRT->Translate(client, request);
        
        if(m_verbose >= VERBOSE_SIMPLE){
            printf("Translated:\n");
            Utils::PrintWriteRequestBFRT(&r, m_runtimeCFGBFRT->GetVirtRuntimeBFRTInfoDesc());
        }

        //printf("TEST: Returning before write...\n");
        //return grpc::Status::OK;

        bfrt_proto::WriteResponse result;
        grpc::ClientContext scontext;

        m_stubMutex.lock();
        status = this->m_s1connection.stub->Write(&scontext, r, &result);
        m_stubMutex.unlock();

        if(m_verbose >= VERBOSE_SIMPLE)
            printf("Status Code: %d\nMSG: %s\n", (int) status.error_code(), status.error_message().c_str());

        //if(status.error_code() == 0){
        //    this->m_runtimeCFGBFRT->IncreaseTableUsageForRequests(client, request);
        //}
    }
    catch (const char* err){
        if(!strcmp(err, RUNTIME_EXCEPT_CLIENT_CFG_NOT_FOUND_MSG)){
            fprintf(stderr, "Except: Client %s not found in RuntimeCFG! Ignoring request...\n", client.c_str());
            return grpc::Status(grpc::StatusCode::UNKNOWN, "Credentials not found, please contact the administrator.");
        }
        else if(!strcmp(err, RUNTIME_EXCEPT_TABLE_FULL)){
            fprintf(stderr, "Except: Resource exhausted on table insert!\n");
            return grpc::Status(grpc::StatusCode::RESOURCE_EXHAUSTED, "Table is full.");
        }
        else if(!strcmp(err, RUNTIME_EXCEPT_MALFORMED_REQUEST)){
            fprintf(stderr, "Except: Malformed write request!\n");
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid request.");
        }
        else if(!strcmp(err, RUNTIME_EXCEPT_IDLE_TIMEOUT_NOT_SUPPORTED)){
            fprintf(stderr, "Except: Write request with invalid idle timeout\n");
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "TableEntry idle_timeout_ns should be 0 for tables that do not support idle timeout.");
        }
        else{
            fprintf(stderr, "Unknown exception during request translation\n");
            return grpc::Status(grpc::StatusCode::UNKNOWN, "");
        }
    }

    if(status.error_code() == grpc::StatusCode::UNAVAILABLE)
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Manager not connected to the switch server. Please try again later or contact the system administrator.");
    return status;
}
      
grpc::Status BFRuntimeImpl::Read(grpc::ServerContext* context, const bfrt_proto::ReadRequest* request, grpc::ServerWriter< bfrt_proto::ReadResponse>* writer) {
    return grpc::Status::OK;
}

grpc::Status BFRuntimeImpl::SetForwardingPipelineConfig(grpc::ServerContext* context, const bfrt_proto::SetForwardingPipelineConfigRequest* request, bfrt_proto::SetForwardingPipelineConfigResponse* response) {
    const std::string client = std::string(context->auth_context().get()->GetPeerIdentity().begin()->begin(), context->auth_context().get()->GetPeerIdentity().begin()->end());
    const std::string controller = std::string(context->peer().c_str());

    printf("SetForwardingPipelineConfig from %s (%s)\n", controller.c_str(), client.c_str());

    if(client == "admin"){
        return this->ProcessSetForwardingPipelineConfigFromAdmin(context, request, response);
    }

    auto it = this->m_connections.find(client);
    if(it == this->m_connections.end()){
        if(m_verbose >= VERBOSE_SIMPLE)
        printf("- Client not found!\n");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Client not found \"" + client + "\".");
    }

    auto ClientMapIT = m_connections.find(client);
    if(ClientMapIT->second.find(controller) == ClientMapIT->second.end()){
        if(m_verbose >= VERBOSE_SIMPLE)
        printf("- Controller did not started a stream channel yet!\n");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Stream channel not open.");
    }

    if(request->action() != bfrt_proto::SetForwardingPipelineConfigRequest_Action_BIND){
        if(m_verbose >= VERBOSE_SIMPLE)
            printf("- Unsupported action!\n");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Action must be BIND.");
    }
    else if(request->config_size() != 1){
        if(m_verbose >= VERBOSE_SIMPLE)
            printf("- Only one config supported!\n");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Only one config supported.");
    }

    bfrt_proto::ForwardingPipelineConfig config = request->config(0);
    if(config.bfruntime_info() != m_runtimeCFGBFRT->GetClientBFRTInfoDesc(client)->GetBFRTInfoSerializedString()){
        if(m_verbose >= VERBOSE_SIMPLE)
            printf("- BFRT Info does not match!\n");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "BFRT Info does not match.");
    }

    return grpc::Status::OK;
}

grpc::Status BFRuntimeImpl::ProcessSetForwardingPipelineConfigFromAdmin(grpc::ServerContext* context, const bfrt_proto::SetForwardingPipelineConfigRequest* request, bfrt_proto::SetForwardingPipelineConfigResponse* response) {
    if(request->config_size() != 1){
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid number of configs (!= 1)");
    }

    const bfrt_proto::ForwardingPipelineConfig *config = &request->config().at(0);

    if(config->profiles_size() != 1){
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid number of profiles (!= 1)");
    }

    const bfrt_proto::ForwardingPipelineConfig_Profile *profile = &config->profiles().at(0);

    if(config->p4_name() == "HOTSWAP"){
        //TODO: We assume the admin already deployed S2

        std::string AuxAddr;
        std::string S2Addr;

        //VirtP4Info
        FILE *fNewVirtBFInfo = fopen(TEMP_VIRT_BFINFO_FILE_NAME, "w");
        fprintf(fNewVirtBFInfo, "%s", config->bfruntime_info().c_str());
        fclose(fNewVirtBFInfo);

        const int RequestDataSize = (int) profile->binary().size();
        const char *data = profile->binary().c_str();

        int NumClientToMigrateNames = 0;
        std::vector<std::string> ClientToMigrateNames;
        std::vector<uint32_t> PortsInAux;
        std::vector<uint32_t> PortsInAuxForS2;
        int ClientDataFileSize = 0, VirtMapFileSize = 0, NumAditionalFiles = 0;
        std::string newRegSelectSwitchValue;

        printf("RequestDataSize: %d\n", RequestDataSize);
        try {
            int readData = 0;
            
            //PortsInAux
            if(readData + 4 > RequestDataSize)
                throw "Malformed configuration.";
            const int NumPortsInAux = Utils::Get4BytesAsInt(data + readData);
            readData = readData + 4;
            printf("NumPortsInAux: %d\n", NumPortsInAux);
            for(int i = 0; i < NumPortsInAux; i++){
                if(readData + 4 > RequestDataSize)
                    throw "Malformed configuration.";
                const uint32_t port = Utils::Get4BytesAsInt(data + readData);
                readData = readData + 4;
                PortsInAux.push_back(port);
                printf("Port %d: %u\n", i + 1, port);
            }

            for(int i = 0; i < NumPortsInAux; i++){
                if(readData + 4 > RequestDataSize)
                    throw "Malformed configuration.";
                const uint32_t port = Utils::Get4BytesAsInt(data + readData);
                readData = readData + 4;
                PortsInAuxForS2.push_back(port);
                printf("Port S2 %d: %u\n", i + 1, port);
            }

            //New RegSelectSwitch Value
            if(readData + 1 > RequestDataSize)
                throw "Malformed configuration.";
            newRegSelectSwitchValue = std::string(data + readData, 1);
            readData = readData + 1;
            printf("New RegSelectSwitch Value: %d\n", (int) newRegSelectSwitchValue.c_str()[0]);

            //Aux IP
            if(readData + 4 > RequestDataSize)
                throw "Malformed configuration.";

            const int AuxIPStringSize = Utils::Get4BytesAsInt(data + readData);
            readData = readData + 4;
            printf("AuxIPStringSize: %d\n", AuxIPStringSize);
            if(readData + AuxIPStringSize > RequestDataSize)
                throw "Malformed configuration.";
            char *AuxIPString = (char *) malloc(AuxIPStringSize + 1);
            memcpy(AuxIPString, data + readData, AuxIPStringSize);
            AuxIPString[AuxIPStringSize] = '\0';
            readData = readData + AuxIPStringSize;
            AuxAddr = std::string(AuxIPString);
            free(AuxIPString);
            printf("AuxAddr: %s\n", AuxAddr.c_str());

            //S2 IP
            if(readData + 4 > RequestDataSize)
                throw "Malformed configuration.";

            const int S2IPStringSize = Utils::Get4BytesAsInt(data + readData);
            readData = readData + 4;
            printf("S2IPStringSize: %d\n", S2IPStringSize);
            if(readData + S2IPStringSize > RequestDataSize)
                throw "Malformed configuration.";
            char *S2IPString = (char *) malloc(S2IPStringSize + 1);
            memcpy(S2IPString, data + readData, S2IPStringSize);
            S2IPString[S2IPStringSize] = '\0';
            readData = readData + S2IPStringSize;
            S2Addr = std::string(S2IPString);
            free(S2IPString);
            printf("S2Addr: %s\n", S2Addr.c_str());

            //Clients to be updated
            if(readData + 4 > RequestDataSize)
                throw "Malformed configuration.";

            NumClientToMigrateNames = Utils::Get4BytesAsInt(data + readData);
            readData = readData + 4;
            printf("NumClientToMigrateNames: %d\n", NumClientToMigrateNames);
            for(int i = 0; i < NumClientToMigrateNames; i++){
                if(readData + 4 > RequestDataSize)
                    throw "Malformed configuration.";
                const int ClientNameSize = Utils::Get4BytesAsInt(data + readData);
                readData = readData + 4;
                if(readData + ClientNameSize > RequestDataSize)
                    throw "Malformed configuration.";

                printf("%d ClientNameSize: %d\n", i + 1, ClientNameSize);

                char *clientName = (char *) malloc(ClientNameSize + 1);
                memcpy(clientName, data + readData, ClientNameSize);
                clientName[ClientNameSize] = '\0';
                readData = readData + ClientNameSize;

                printf("%d ClientName: %s\n", i + 1, clientName);
                ClientToMigrateNames.push_back(std::string(clientName));
                free(clientName);
            }

            if(readData + 4 > RequestDataSize)
                throw "Malformed configuration.";
        
            ClientDataFileSize = Utils::Get4BytesAsInt(data + readData);
            readData = readData + 4;

            printf("ClientDataFileSize: %d\n", ClientDataFileSize);

            if(readData + ClientDataFileSize > RequestDataSize)
                throw "Malformed configuration.";
        
            FILE *fp = fopen(TEMP_CLIENT_DATA_FILE_NAME, "wb");
            fwrite(data + readData, sizeof(char), ClientDataFileSize, fp);
            fclose(fp);

            readData = readData + ClientDataFileSize;

            if(readData + 4 > RequestDataSize)
                throw "Malformed configuration.";

            VirtMapFileSize = Utils::Get4BytesAsInt(data + readData);
            readData = readData + 4;

            printf("VirtMapFileSize: %d\n", VirtMapFileSize);

            if(readData + VirtMapFileSize > RequestDataSize)
                throw "Malformed configuration.";
        
            fp = fopen(TEMP_VIRT_MAP_FILE_NAME, "wb");
            fwrite(data + readData, sizeof(char), VirtMapFileSize, fp);
            fclose(fp);

            readData = readData + VirtMapFileSize;

            if(readData + 4 > RequestDataSize)
                throw "Malformed configuration.";

            NumAditionalFiles = Utils::Get4BytesAsInt(data + readData);
            readData = readData + 4;

            printf("NumAditionalFiles: %d\n", NumAditionalFiles);

            for(int i = 0; i < NumAditionalFiles; i++){
                if(readData + 4 > RequestDataSize)
                    throw "Malformed configuration.";
                const int AdditionaFileNameSize = Utils::Get4BytesAsInt(data + readData);
                readData = readData + 4;
                if(readData + AdditionaFileNameSize > RequestDataSize)
                    throw "Malformed configuration.";

                printf("%d AdditionalFileNameSize: %d\n", i + 1, AdditionaFileNameSize);

                char *fileName = (char *) malloc(AdditionaFileNameSize + 1);
                memcpy(fileName, data + readData, AdditionaFileNameSize);
                fileName[AdditionaFileNameSize] = '\0';
                readData = readData + AdditionaFileNameSize;

                printf("%d FileName: %s\n", i + 1, fileName);

                if(readData + 4 > RequestDataSize)
                    throw "Malformed configuration.";
                const int AdditionalFileSize = Utils::Get4BytesAsInt(data + readData);
                readData = readData + 4;
                if(readData + AdditionalFileSize > RequestDataSize)
                    throw "Malformed configuration.";

                printf("%d AdditionalFileSize: %d\n", i + 1, AdditionalFileSize);

                fp = fopen((std::string(TEMP_DIR_NAME) + "/" + std::string(fileName)).c_str(), "wb");
                free(fileName);
                if(!fp)
                    throw "Malformed configuration.";

                fwrite(data + readData, sizeof(char), AdditionalFileSize, fp);
                fclose(fp);
                readData = readData + AdditionalFileSize;
            }

            printf("ReadData: %d\n", readData);
        }
        catch (const char* err){
            fprintf(stderr, "Exception during HOTSWAP config processing: %s\n", err);
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Malformed configuration data.");
        }

        RuntimeCFGBFRT *newCFG = new RuntimeCFGBFRT(TEMP_CLIENT_DATA_FILE_NAME, TEMP_VIRT_MAP_FILE_NAME, TEMP_VIRT_BFINFO_FILE_NAME);

        //TODO: Block clients controllers during swap

        try{
            this->Hotswap(AuxAddr, S2Addr, ClientToMigrateNames, PortsInAux, PortsInAuxForS2, newCFG, newRegSelectSwitchValue);
        }
        catch (const char* err){
            fprintf(stderr, "Exception during HOTSWAP config processing: %s\n", err);
            return grpc::Status(grpc::StatusCode::ABORTED, "Error: " + std::string(err));
        }
    }
    else{
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Unrecognized p4_name from admin");
    }
    
    return grpc::Status::OK;
}
       
grpc::Status BFRuntimeImpl::GetForwardingPipelineConfig(grpc::ServerContext* context, const bfrt_proto::GetForwardingPipelineConfigRequest* request, bfrt_proto::GetForwardingPipelineConfigResponse* response) {
    const std::string client = std::string(context->auth_context().get()->GetPeerIdentity().begin()->begin(), context->auth_context().get()->GetPeerIdentity().begin()->end());
    const std::string controller = std::string(context->peer().c_str());

    printf("GetForwardingPipelineConfig from %s (%s)\n", controller.c_str(), client.c_str());

    auto it = this->m_connections.find(client);
    if(it == this->m_connections.end()){
        if(m_verbose >= VERBOSE_SIMPLE)
        printf("- Client not found!\n");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Client not found \"" + client + "\".");
    }

    bfrt_proto::ForwardingPipelineConfig *config = response->add_config();
    config->set_p4_name("Program");
    config->set_bfruntime_info(m_runtimeCFGBFRT->GetClientBFRTInfoDesc(client)->GetBFRTInfoSerializedString());

    return grpc::Status::OK;
}
      
grpc::Status BFRuntimeImpl::StreamChannel(grpc::ServerContext* context, grpc::ServerReaderWriter< bfrt_proto::StreamMessageResponse, bfrt_proto::StreamMessageRequest>* stream) {
    const std::string client = std::string(context->auth_context().get()->GetPeerIdentity().begin()->begin(), context->auth_context().get()->GetPeerIdentity().begin()->end());
    const std::string controller = std::string(context->peer().c_str());

    if(m_verbose >= VERBOSE_SIMPLE)
        printf("StreamChannel from client %s - controller: %s\n", client.c_str(), controller.c_str());
    
    auto it = this->m_connections.find(client);
    if(it == this->m_connections.end()){
        if(m_verbose >= VERBOSE_SIMPLE)
        printf("- Client not found!\n");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Client not found \"" + client + "\".");
    }

    if(it->second.find(controller) != it->second.end()){
        if(m_verbose >= VERBOSE_SIMPLE)
        printf("- Already open! Returning error...\n");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Stream channel already open for controller \"" + controller + "\".");
    }

    ConnectionInfoBFRT connectionInfo;
    connectionInfo.controllerID = controller;
    connectionInfo.stream = stream;
    connectionInfo.streamContext = context;
    connectionInfo.bfrtInfoDesc = m_runtimeCFGBFRT->GetClientBFRTInfoDesc(client);
    connectionInfo.isPrimary = (it->second.size() == 0); //First connection is primary
    it->second.emplace(std::make_pair(controller, connectionInfo));

    //Handle the stream messages from the controller
    bfrt_proto::StreamMessageRequest request;
    while(stream->Read(&request)){
        if(m_verbose >= VERBOSE_ALL){
            printf("Received StreamMessageRequest from client %s - controller %s:\n", client.c_str(), controller.c_str());
        }

        if(request.has_subscribe()){
            if(m_verbose >= VERBOSE_SIMPLE)
                printf("- Message is subscribe\n");

            bfrt_proto::StreamMessageResponse response;
            response.mutable_subscribe()->CopyFrom(request.subscribe());
            response.mutable_subscribe()->mutable_status()->set_code(0);
            response.mutable_subscribe()->mutable_status()->set_message("Subscription successful.");
            stream->Write(response);
        }
        else if(request.has_digest_ack()){
            if(m_verbose >= VERBOSE_SIMPLE)
                printf("- Message is digest_ack\n");
        }
    }

    //Remove connection on exit
    it->second.erase(controller);
    if(m_verbose >= VERBOSE_SIMPLE)
        printf("Stream channel closed for client %s - controller %s\n", client.c_str(), controller.c_str());
    
    //If primary was closed, assign a new primary
    if(connectionInfo.isPrimary && it->second.size() > 0){
        it->second.begin()->second.isPrimary = true;
        if(m_verbose >= VERBOSE_SIMPLE)
            printf("- New primary for client %s is controller %s\n", client.c_str(), it->second.begin()->first.c_str());
    }
    
    return grpc::Status::OK;
}


void BFRuntimeImpl::ResetConnectionsAndKnownClients(){
    for(auto client : m_connections){
        for(auto controller : client.second){
            controller.second.streamContext->TryCancel();
        }
    }

    m_connections.clear();
    //m_clientElectionIDsSet.clear();

    for(const std::string& name : m_runtimeCFGBFRT->GetClientNames()){
        m_connections.emplace(std::make_pair(name, std::map<std::string, ConnectionInfoBFRT>{}));
    }
}

inline bool AllFalse(const std::vector<std::string>& pipelineValues){
    bool result = true;
    for(const std::string& byte : pipelineValues){
        uint8_t val = static_cast<uint8_t>(byte[0]);
        if(val == 1)
            result = false;
    }
    return result;
}

void BFRuntimeImpl::Hotswap(const std::string& auxAddr, const std::string& S2Addr, const std::vector<std::string>& clientsToMigrate, const std::vector<uint32_t>& PortsInAux, const std::vector<uint32_t>& PortsInAuxForS2, RuntimeCFGBFRT *newCFG, const std::string& newRegSelectSwitchValue){
    const auto StartTime = std::chrono::steady_clock::now();

    BfRuntimeConnection auxConnection;
    BfRuntimeConnection s2Connection;

    grpc::ChannelArguments cargs;
    const int max_message_size = 100 * 1024 * 1024; // 100 MB
    cargs.SetMaxReceiveMessageSize(max_message_size);
    cargs.SetMaxSendMessageSize(max_message_size);
    
    auxConnection.channel = grpc::CreateCustomChannel(auxAddr, grpc::InsecureChannelCredentials(), cargs);
    printf("Waiting for connection to Aux (%s)...\n", auxAddr.c_str());
    
    auxConnection.channel->WaitForConnected(gpr_time_from_seconds(10, gpr_clock_type::GPR_TIMESPAN));
    if(auxConnection.channel->GetState(false) != GRPC_CHANNEL_READY){
        fprintf(stderr, "Could not stabilish connection to %s\n", auxAddr.c_str());
        throw "Could not stabilish connection to aux";
    }

    s2Connection.channel = grpc::CreateCustomChannel(S2Addr, grpc::InsecureChannelCredentials(), cargs);
    printf("Waiting for connection to S2 (%s)...\n", S2Addr.c_str());
    
    s2Connection.channel->WaitForConnected(gpr_time_from_seconds(10, gpr_clock_type::GPR_TIMESPAN));
    if(s2Connection.channel->GetState(false) != GRPC_CHANNEL_READY){
        fprintf(stderr, "Could not stabilish connection to %s\n", S2Addr.c_str());
        throw "Could not stabilish connection to S2";
    }

    printf("Connected!\n");

    auxConnection.stub = bfrt_proto::BfRuntime::NewStub(auxConnection.channel);
    auxConnection.streamContext = std::make_shared<grpc::ClientContext>(); 
    auxConnection.stream = auxConnection.stub->StreamChannel(auxConnection.streamContext.get());
    auxConnection.bfrtManager = std::make_shared<BFRTManager>(auxConnection.stub, auxConnection.stream);

    s2Connection.stub = bfrt_proto::BfRuntime::NewStub(s2Connection.channel);
    s2Connection.streamContext = std::make_shared<grpc::ClientContext>(); 
    s2Connection.stream = s2Connection.stub->StreamChannel(s2Connection.streamContext.get());
    s2Connection.bfrtManager = std::make_shared<BFRTManager>(s2Connection.stub, s2Connection.stream);

    bfrt_proto::StreamMessageResponse response;

    auxConnection.bfrtManager->SendSubscribe();
    auxConnection.stream->Read(&response);
    if(response.has_subscribe()){
        printf("Subscribed to %s successfully.\n", auxAddr.c_str());
        printf("-- Code: %d\n-- Message: %s\n", response.subscribe().status().code(), response.subscribe().status().message().c_str()); 
    
        if(response.subscribe().status().code() != 0){
            fprintf(stderr, "Subscription to %s failed.\n", auxAddr.c_str());
            exit(1);
        }

        bfrt_proto::GetForwardingPipelineConfigResponse rResponse = auxConnection.bfrtManager->GetFwdPipeline();
                
        //Binds to the program running on the switch, as required by the bfruntime specification
        auxConnection.bfrtManager->Bind(rResponse.config(0));

        SaveNonP4Config(rResponse.non_p4_config().bfruntime_info(), TEMP_DIR_NAME "/non_p4_config-" + auxAddr + ".txt");
        SaveBfRuntimeInfo(rResponse.config(0).bfruntime_info(), TEMP_DIR_NAME "/bfruntime_info-" + auxAddr + ".txt");
        auxConnection.bfrtManager->LoadFromFile(TEMP_DIR_NAME "/bfruntime_info-" + auxAddr + ".txt");
        auxConnection.bfrtManager->LoadFromFile(TEMP_DIR_NAME "/non_p4_config-" + auxAddr + ".txt");
    }
    else{
        fprintf(stderr, "Could not subscribe to %s\n", auxAddr.c_str());
        exit(1);
    }

    s2Connection.bfrtManager->SendSubscribe();
    s2Connection.stream->Read(&response);
    if(response.has_subscribe()){
        printf("Subscribed to %s successfully.\n", S2Addr.c_str());
        printf("-- Code: %d\n-- Message: %s\n", response.subscribe().status().code(), response.subscribe().status().message().c_str()); 
    
        if(response.subscribe().status().code() != 0){
            fprintf(stderr, "Subscription to %s failed.\n", S2Addr.c_str());
            exit(1);
        }

        bfrt_proto::GetForwardingPipelineConfigResponse rResponse = s2Connection.bfrtManager->GetFwdPipeline();
                
        //Binds to the program running on the switch, as required by the bfruntime specification
        s2Connection.bfrtManager->Bind(rResponse.config(0));

        SaveNonP4Config(rResponse.non_p4_config().bfruntime_info(), TEMP_DIR_NAME "/non_p4_config-" + S2Addr + ".txt");
        SaveBfRuntimeInfo(rResponse.config(0).bfruntime_info(), TEMP_DIR_NAME "/bfruntime_info-" + S2Addr + ".txt");
        s2Connection.bfrtManager->LoadFromFile(TEMP_DIR_NAME "/bfruntime_info-" + S2Addr + ".txt");
        s2Connection.bfrtManager->LoadFromFile(TEMP_DIR_NAME "/non_p4_config-" + S2Addr + ".txt");
    }
    else{
        fprintf(stderr, "Could not subscribe to %s\n", S2Addr.c_str());
        exit(1);
    }

    //Setup ArrayS
    BFRTManager *AuxManager = auxConnection.bfrtManager.get();
    
    std::vector<BFRegisterInfo> AuxFlagRegisters = AuxManager->GetRegisterInfoList();
    size_t idx = 0;
    while(idx < AuxFlagRegisters.size()){
        if(AuxFlagRegisters[idx].registerName.find("regFlagsModified") == std::string::npos){
            AuxFlagRegisters.erase(AuxFlagRegisters.begin() + idx);
            idx--;
        }
        idx++;
    }

    std::vector<std::vector<bool>> ArrayS(AuxFlagRegisters.size());
    for(size_t i = 0; i < AuxFlagRegisters.size(); i++){
        ArrayS[i].resize(AuxFlagRegisters[i].size, true);
    }

    //Sets all the flags to true for the clients not migrating
    const std::vector<std::string> ClientNames = m_runtimeCFGBFRT->GetClientNames();
    
    if(m_verbose >= VERBOSE_SIMPLE){
        printf("Clients: ");
        for(const std::string& name : ClientNames)
            printf("%s ", name.c_str());
        printf("\nClients to migrate: ");
        for(const std::string& name : clientsToMigrate)
            printf("%s ", name.c_str());
        printf("\n");
    }
    
    for(const std::string& name : ClientNames){
        if(std::find(clientsToMigrate.begin(), clientsToMigrate.end(), name) != clientsToMigrate.end()){
            if(m_verbose >= VERBOSE_SIMPLE)
                printf("- Client %s will be migrated. Setting its flags to false...\n", name.c_str());
            
            for(size_t i = 0; i < ArrayS.size(); i++){
                const uint32_t start = m_runtimeCFGBFRT->GetStartOfFlagRegisterForClient(name, i);
                const uint32_t size = m_runtimeCFGBFRT->GetSizeOfFlagRegisterForClient(name, i);

                printf("-- %lu (%u - %u)\n", i, start, size);

                for(uint32_t j = 0; j < size; j++){
                    ArrayS[i][j + start] = false;
                }
            }
        }
    }

    //Setting Multicast groups on Aux for S2 ports
    printf("Hotswap: Setting Aux Multicast Table Entries...\n");
    auxConnection.bfrtManager->ErasePreNodeTableEntries();
    auxConnection.bfrtManager->ErasePreMGIDTableEntries();
    for(uint32_t i = 0; i < (uint32_t) PortsInAux.size(); i++){
        //uint32_t mcNodeID = PortsInAux[i];
        uint32_t mcNodeID = i + 1;
        uint32_t mcRID = i + 1;
        const std::vector<uint32_t> portList = {PortsInAux[i], PortsInAuxForS2[i]};
        HotswapSetAuxPreNode(&auxConnection, mcNodeID, mcRID, portList);
    }

    for(uint32_t i = 0; i < (uint32_t) PortsInAux.size(); i++){
        uint32_t mgID = PortsInAux[i];
        uint32_t mcNodeID = i + 1;
        HotswapSetAuxPreMGID(&auxConnection, mgID, std::vector<uint32_t>{mcNodeID});
    }

    printf("Hotswap: Clearing Aux Flag Registers...\n");
    HotswapClearAuxFlagRegisters(&auxConnection, AuxFlagRegisters);

    HotswapCopyTables(clientsToMigrate, newCFG, s2Connection);

    HotswapSetMigration(true, clientsToMigrate);

    std::vector<uint32_t> indexesToCopy;
    bool allDone = false;
    while(!allDone){
        printf("Waiting on delay (%dms)...\n", HOTSWAP_DELAY_MS);
        allDone = true;
        SLEEP_MILLISECONDS(HOTSWAP_DELAY_MS);

        for(size_t i = 0; i < AuxFlagRegisters.size(); i++){
            indexesToCopy.clear();
            std::vector<std::vector<std::string>> regValues = AuxManager->ReadFromRegister(AuxFlagRegisters[i].registerID);
            for(size_t j = 0; j < regValues.size(); j++){
                if(ArrayS[i][j])
                    continue;
                
                bool MValue = !AllFalse(regValues[j]);
                if(MValue){
                    printf("Register %s index %lu modified\n", AuxFlagRegisters[i].registerName.c_str(), j);
                    ArrayS[i][j] = true;
                }
                else{
                    indexesToCopy.push_back(j);
                }
            }

            //TranslateIndexesToCopy from S1 to S2
            std::vector<std::vector<uint32_t>> indexesPerClient(m_runtimeCFGBFRT->GetClientNames().size());
            
            for(size_t k = 0; k < indexesToCopy.size(); k++){
                std::pair<uint32_t, size_t> pair = m_runtimeCFGBFRT->GetClientAndIndexOfFlagRegister(i, indexesToCopy[k]);
                uint32_t clientID = pair.first;
                size_t indexInClient = pair.second;

                indexesPerClient[clientID - 1].push_back(indexInClient);
            }

            for(uint32_t c = 0; c < (uint32_t) indexesPerClient.size(); c++){
                if(indexesPerClient[c].size() == 0)
                    continue;
                
                uint32_t id = c + 1;; //Client IDs start at 1
                uint64_t regIDS1 = m_runtimeCFGBFRT->GetClientRegisterID(id, i);
                uint64_t regIDS2 = newCFG->GetClientRegisterID(id, i);
                
                //Get the mapped register IDs
                regIDS1 = m_runtimeCFGBFRT->GetMappedRegisterID(id, regIDS1);
                regIDS2 = newCFG->GetMappedRegisterID(id, regIDS2);

                //Print client id and regIDs
                printf("Copying register %s from S1 (RegID: %lu) to S2 (RegID: %lu) for client %s (ID: %u) for %lu indexes\n", 
                    AuxFlagRegisters[i].registerName.c_str(), regIDS1, regIDS2, m_runtimeCFGBFRT->GetClientName(id).c_str(), id, indexesPerClient[c].size());
            
                HotswapCopyRegisters(&s2Connection, static_cast<uint32_t>(regIDS1), static_cast<uint32_t>(regIDS2), indexesPerClient[c]);
            }
            
            std::vector<std::vector<std::string>> newRegValues = AuxManager->ReadFromRegister(AuxFlagRegisters[i].registerID);
            for(size_t l = 0; l < indexesToCopy.size(); l++){
                bool MValue = !AllFalse(newRegValues[indexesToCopy[l]]);
                if(MValue){
                    allDone = false;
                    AuxManager->WriteToRegister(AuxFlagRegisters[i].registerID, indexesToCopy[l], std::string("\x00", 1));
                }
                else{
                    ArrayS[i][indexesToCopy[l]] = true;
                }
            }
        }
    }

    printf("All flags set to true on Aux!\n");

    printf("Writing to regSelectSwitch (value %d) to switch ingress to S2...\n", (int) newRegSelectSwitchValue.c_str()[0]);
    AuxManager->WriteToRegister("pipe.SwitchIngress.regSelectSwitch", 0, newRegSelectSwitchValue);

    //Change m_s1connection to point to S2
    printf("Updating internal connection to point to S2...\n");
    this->m_s1connection.channel = s2Connection.channel;
    this->m_s1connection.stub = s2Connection.stub;
    this->m_s1connection.streamContext = s2Connection.streamContext;
    this->m_s1connection.stream = s2Connection.stream;
    this->m_s1connection.bfrtManager = s2Connection.bfrtManager;

    printf("Updating internal CFG to the new one...\n");
    delete this->m_runtimeCFGBFRT;
    this->m_runtimeCFGBFRT = newCFG;

    const auto EndTime =  std::chrono::steady_clock::now();
    const auto Duration = std::chrono::duration_cast<std::chrono::seconds>(EndTime - StartTime).count();
    printf("Hotswap completed in %ld seconds.\n", Duration);
}

void BFRuntimeImpl::HotswapSetAuxPreNode(BfRuntimeConnection *AuxConnection, const uint32_t mcNodeID, const uint16_t mcRID, const std::vector<uint32_t>& portList){
    BFRTManager *AuxManager = AuxConnection->bfrtManager.get();
    bool result = AuxManager->WriteToPreNode(mcNodeID, mcRID, std::vector<uint8_t>{}, portList);
    if(!result){
        throw "Error setting PreNode on Aux during Hotswap.";
    }
}

void BFRuntimeImpl::HotswapSetAuxPreMGID(BfRuntimeConnection *AuxConnection, const uint16_t mgID, const std::vector<uint32_t>& mcNodeID){
    BFRTManager *AuxManager = AuxConnection->bfrtManager.get();
    bool result = AuxManager->WriteToPreMGID(mgID, mcNodeID, std::vector<bool>{false}, std::vector<uint16_t>{0});
    if(!result){
        throw "Error setting PreMGID on Aux during Hotswap.";
    }
}

void BFRuntimeImpl::HotswapClearAuxFlagRegisters(BfRuntimeConnection *AuxConnection, const std::vector<BFRegisterInfo>& AuxFlagRegisters){
    BFRTManager *AuxManager = AuxConnection->bfrtManager.get();

    for(size_t i = 0; i < AuxFlagRegisters.size(); i++){
        std::vector<uint32_t> indexes(AuxFlagRegisters[i].size);
        for(size_t j = 0; j < AuxFlagRegisters[i].size; j++){
            indexes[j] = j;
        }
        AuxManager->WriteToRegister(AuxFlagRegisters[i].registerID, indexes, std::vector<std::string>(AuxFlagRegisters[i].size, std::string("\x00", 1)));
    }
}

void BFRuntimeImpl::HotswapCopyTables(const std::vector<std::string>& clientsToMigrate, RuntimeCFGBFRT *newCFG, BfRuntimeConnection s2connection){
    printf("HotswapCopyTables:\n");
    
    //Assumes that the clients to migrate did not change their bfinfos
    std::set<uint64_t> sharedTableIDsOld;
    std::set<uint64_t> sharedTableIDsNew;
    BFRTInfoDesc* bfrtInfoDescNewMergedOld = m_runtimeCFGBFRT->GetVirtRuntimeBFRTInfoDesc();
    std::vector<BFRTTable> tablesOld = bfrtInfoDescNewMergedOld->GetTables();
    for(BFRTTable& table : tablesOld){
        if(table.type == TT_OTHER){
            try{
                if(m_runtimeCFGBFRT->GetVirtTableData(table.name).type == VTTBF_HIDDEN_SHARED)
                    sharedTableIDsOld.insert(table.id);
            }
            catch(const char* err){
                //Table not found on CFG, ignore
            }
        }
    }
    BFRTInfoDesc* bfrtInfoDescNewMergedNew = newCFG->GetVirtRuntimeBFRTInfoDesc();
    std::vector<BFRTTable> tablesNew = bfrtInfoDescNewMergedNew->GetTables();
    for(BFRTTable& table : tablesNew){
        if(table.type == TT_OTHER){
            try{
                if(newCFG->GetVirtTableData(table.name).type == VTTBF_HIDDEN_SHARED)
                    sharedTableIDsNew.insert(table.id);
            }
            catch(const char* err){
                //Table not found on CFG, ignore
            }
        }
    }

    for(const std::string& clientName : clientsToMigrate){
        uint32_t OldClientID = m_runtimeCFGBFRT->GetClientID(clientName);
        uint32_t NewClientID = newCFG->GetClientID(clientName);

        printf("Client: %s (%d -> %d)\n", clientName.c_str(), OldClientID, NewClientID);

        BFRTInfoDesc* bfrtInfoClient = newCFG->GetClientBFRTInfoDesc(clientName);
        std::vector<BFRTTable> clientTables = bfrtInfoClient->GetTables();
        for(BFRTTable& table : clientTables){
            if(table.type == TT_OTHER){
                uint64_t S1VirtTableID, S2VirtTableID;
                try {
                    S1VirtTableID = m_runtimeCFGBFRT->GetMappedTableID(OldClientID, table.id);
                    S2VirtTableID = newCFG->GetMappedTableID(NewClientID, table.id);
                }
                catch(const char* err){
                    //Table not found, ignore
                    continue;
                }
                
                bool shouldRemoveProgramID = sharedTableIDsOld.find(S1VirtTableID) != sharedTableIDsOld.end() &&
                                            sharedTableIDsNew.find(S2VirtTableID) == sharedTableIDsNew.end();

                bool shouldAddProgramID = sharedTableIDsOld.find(S1VirtTableID) == sharedTableIDsOld.end() &&
                                        sharedTableIDsNew.find(S2VirtTableID) != sharedTableIDsNew.end();

                printf("- Table: %s (OldVirtID: %lu, NewVirtID: %lu) ShouldRemoveProgramID: %d ShouldAddProgramID: %d\n", table.name.c_str(), S1VirtTableID, S2VirtTableID, shouldRemoveProgramID, shouldAddProgramID);

                std::vector<BFTableEntry> tableEntries = m_s1connection.bfrtManager->ReadTableEntries(S1VirtTableID);

                grpc::ClientContext cContext;
    
                bfrt_proto::WriteRequest request;
                request.set_client_id(BFRT_CLIENT_ID);

                request.mutable_target()->set_device_id(0);
                request.mutable_target()->set_pipe_id(0xffff);
                
                for(const BFTableEntry& entry : tableEntries){
                    bfrt_proto::Update* update = request.add_updates();
                    update->set_type(bfrt_proto::Update_Type_INSERT);

                    bfrt_proto::TableEntry* tableEntry = update->mutable_entity()->mutable_table_entry();
                    tableEntry->CopyFrom(entry.protoEntry);

                    tableEntry->set_table_id(S2VirtTableID);

                    //Modify action id
                    const uint32_t oldVirtActionID = tableEntry->data().action_id();
                    const std::string oldVirtActionName = m_runtimeCFGBFRT->GetVirtRuntimeBFRTInfoDesc()->GetActionName(oldVirtActionID);
                    const std::string oldClientActionName = m_runtimeCFGBFRT->GetMappedActionNameReverse(OldClientID, oldVirtActionName);
                    const std::string newVirtActionName = newCFG->GetMappedActionName(NewClientID, oldClientActionName);
                    const uint32_t newVirtActionID = newCFG->GetVirtRuntimeBFRTInfoDesc()->GetActionID(newVirtActionName);

                    printf("-- Entry ActionID: %u (%s) -> %u (%s)\n", oldVirtActionID, oldVirtActionName.c_str(), newVirtActionID, newVirtActionName.c_str());

                    tableEntry->mutable_data()->set_action_id(newVirtActionID);
                
                    if(shouldRemoveProgramID){
                        //TODO: Remove program ID from entries
                        throw RUNTIME_EXCEPT_NOT_IMPLEMENTED;
                    }
                    else if(shouldAddProgramID){
                        //TODO: Add program ID to entries
                        throw RUNTIME_EXCEPT_NOT_IMPLEMENTED;
                    }
                }

                bfrt_proto::WriteResponse response;
                grpc::Status status = s2connection.stub->Write(&cContext, request, &response);

                printf("Write Status: %d\nMSG: %s\n", status.error_code(), status.error_message().c_str());
            }
        }
    }
}

void BFRuntimeImpl::HotswapSetMigration(bool migration, const std::vector<std::string>& clientsToMigrate){
    BFRTManager *S1Manager = m_s1connection.bfrtManager.get();
    
    const std::string tableName = "VirtPipe.VirtIngress.SetSwapActiveTable";
    const uint32_t tableID = S1Manager->GetTableID(tableName);

    if(tableID == 0){
        fprintf(stderr, "Could not find table: %s\n", tableName.c_str());
        throw RUNTIME_EXCEPT_INTERNAL_ERROR;
    }

    bfrt_proto::WriteRequest request;
    request.set_client_id(BFRT_CLIENT_ID);

    request.mutable_target()->set_device_id(0);
    request.mutable_target()->set_pipe_id(0xffff);
    
    for(const std::string& clientName : clientsToMigrate){
        char buffer[256];
        
        const uint32_t clientID = m_runtimeCFGBFRT->GetClientID(clientName);
        std::string actionName = "VirtIngress.SetFromSwitch";
        if(migration){
            snprintf(buffer, sizeof(buffer), "VirtIngress.SetSwapActiveAction_%u", clientID);
            actionName = std::string(buffer);
        }
    
        const uint32_t actionID = S1Manager->GetActionID(actionName);

        if(actionID == 0){
            fprintf(stderr, "Could not find action: %s\n", actionName.c_str());
            throw RUNTIME_EXCEPT_INTERNAL_ERROR;
        }

        bfrt_proto::Update* update = request.add_updates();
        update->set_type(bfrt_proto::Update_Type_INSERT);

        bfrt_proto::TableEntry *tableEntry = update->mutable_entity()->mutable_table_entry();
        tableEntry->set_table_id(tableID);
        bfrt_proto::TableKey* key = tableEntry->mutable_key();
        bfrt_proto::KeyField* fieldMatch = key->add_fields();
        fieldMatch->set_field_id(1);
        buffer[0] = static_cast<char>(clientID);
        fieldMatch->mutable_exact()->set_value(std::string(buffer, 1));
        tableEntry->mutable_data()->set_action_id(actionID);
    }

    grpc::ClientContext cContext;
    bfrt_proto::WriteResponse response;
    grpc::Status status = m_s1connection.stub->Write(&cContext, request, &response);

    if(!status.ok()){
        //Try to modify existing entries
        for(int i = 0; i < request.updates_size(); i++){
            request.mutable_updates(i)->set_type(bfrt_proto::Update_Type_MODIFY);
        }

        grpc::ClientContext cContext2;
        status = m_s1connection.stub->Write(&cContext2, request, &response);

        if(!status.ok()){
            fprintf(stderr, "Error setting entry to table %s\n", tableName.c_str());
            fprintf(stderr, "-- Code: %d\n-- Message: %s\n", status.error_code(), status.error_message().c_str());
            throw RUNTIME_EXCEPT_INTERNAL_ERROR;
        }
    }

    printf("Set Hotswap Mode: %s\n", migration ? "true" : "false");
}

void BFRuntimeImpl::HotswapCopyRegisters(BfRuntimeConnection *S2Connection, const uint32_t regIDS1, const uint32_t regIDS2, const std::vector<uint32_t>& indexes){
    BFRTManager *S1Manager = m_s1connection.bfrtManager.get();
    BFRTManager *S2Manager = S2Connection->bfrtManager.get();

    std::vector<std::vector<std::string>> valueS1 = S1Manager->ReadFromRegister(regIDS1);
    
    std::vector<std::string> values;
    for(uint32_t idx : indexes){
        values.push_back(valueS1[idx][0]);
    }

    S2Manager->WriteToRegister(regIDS2, indexes, values);
}