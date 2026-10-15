#include "../include/pcheaders.h"
#include "../proto/bfruntime.grpc.pb.h"
#include "../proto/p4runtime.grpc.pb.h"
#include "../include/PropertyList.h"
#include "../include/P4InfoDesc.h"
#include "../include/RuntimeCFG.h"
#include "../include/P4RuntimeImpl.h"
#include "../include/Utils.h"

#define SLEEP_MILLISECONDS(X) std::this_thread::sleep_for(std::chrono::milliseconds(X));

P4RuntimeImpl::P4RuntimeImpl(RuntimeCFG *rtcfg, std::shared_ptr<p4::v1::P4Runtime::Stub> stub, std::shared_ptr<grpc::ClientReaderWriter<p4::v1::StreamMessageRequest, p4::v1::StreamMessageResponse>> stream, uint8_t verbose){
  this->m_runtimeCFG = rtcfg;
  this->m_stub = stub;
  this->m_stream = stream;
  this->m_verbose = verbose;

  this->ResetConnectionsAndKnownClients();

  std::filesystem::remove_all(TEMP_DIR_NAME);
  std::filesystem::create_directory(TEMP_DIR_NAME);
}

void P4RuntimeImpl::SendStreamResponseToClient(const int clientID, p4::v1::StreamMessageResponse* response){
  const std::string clientName = m_runtimeCFG->GetClientNames()[clientID - 1]; //Client IDs start at 1
  
  if(m_verbose >= VERBOSE_SIMPLE)
    printf("Sending stream response to client \"%s\"\n", clientName.c_str());
  auto openConnections = m_connections.find(clientName);
  if (openConnections == m_connections.end()){
    printf("- Client not found!\n");
    return;
  }

  ConnectionInfo* primary = NULL;
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

  if(response->has_packet()){
    if(m_verbose >= VERBOSE_SIMPLE)
      printf("- Response is packet-in\n");

    std::string payload = response->packet().payload();

    std::string strBinMetadatas = "";
    for (int i = 0; i < primary->PacketInMetadataTotalBitwidth/8; i++)
      if (i < (int) payload.size())
        strBinMetadatas.append(std::bitset<8>(payload[i]).to_string());

    size_t totalExtracted = 0;
    for(int i = 0; i < (int) primary->PacketInMetadatas.size(); i++){
      std::string extractedBits = strBinMetadatas.substr(totalExtracted, primary->PacketInMetadatas[i].bitwidth);
      totalExtracted = totalExtracted + primary->PacketInMetadatas[i].bitwidth;
      std::string data = Utils::binaryToBytes(extractedBits);

      p4::v1::PacketMetadata* newMetadata = response->mutable_packet()->add_metadata();
      newMetadata->set_metadata_id(primary->PacketInMetadatas[i].id);
      newMetadata->set_value(data);
    }
    response->mutable_packet()->set_payload(payload.substr(primary->PacketInMetadataTotalBitwidth/8));

    primary->stream->Write(*(response));

    if(m_verbose >= VERBOSE_SIMPLE)
      printf("- Sent packet-in to %s...\n", primary->controllerID.c_str());
  }
  else if(response->has_idle_timeout_notification()){
    if(m_verbose >= VERBOSE_SIMPLE)
      printf("- Response is idle_timeout_notification\n");

    primary->stream->Write(*response);

    if(m_verbose >= VERBOSE_SIMPLE)
      printf("- Sent response to contoller %s\n", primary->controllerID.c_str());
  }
}

grpc::Status P4RuntimeImpl::Write(grpc::ServerContext* context, const p4::v1::WriteRequest* request, p4::v1::WriteResponse* response) {
  const std::string client = std::string(context->auth_context().get()->GetPeerIdentity().begin()->begin(), context->auth_context().get()->GetPeerIdentity().begin()->end());
  const std::string controller = std::string(context->peer().c_str());

  if(m_verbose >= VERBOSE_SIMPLE){
    printf("Write from \"%s - %s\"\n", client.c_str(), controller.c_str());
    if(m_verbose >= VERBOSE_ALL)
      Utils::PrintWriteRequest(request);
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

  ConnectionInfo* connectionInfo = &ClientMapIT->second.find(controller)->second;

  if(!connectionInfo->isPrimary){
    if(m_verbose >= VERBOSE_SIMPLE)
      printf("- Controller is not primary!\n");
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Controller is not primary.");
  }

  grpc::Status status = grpc::Status(grpc::StatusCode::OK, "");
  try{
    p4::v1::WriteRequest r = this->m_runtimeCFG->Translate(client, request);
    
    if(m_verbose >= VERBOSE_ALL){
      printf("Translated:\n");
      Utils::PrintWriteRequest(&r);
    }

    p4::v1::WriteResponse result;
    grpc::ClientContext scontext;
    status = this->m_stub->Write(&scontext, r, &result);

    if(m_verbose >= VERBOSE_SIMPLE)
      printf("Status Code: %d\nMSG: %s\n", (int) status.error_code(), status.error_message().c_str());

    if(status.error_code() == 0){
      this->m_runtimeCFG->IncreaseTableUsageForRequests(client, request);
    }
  }
  catch (const char* err){
    if(!strcmp(err, RUNTIME_EXCEPT_CLIENT_CFG_NOT_FOUND_MSG)){
      fprintf(stderr, "Exept: Client %s not found in RuntimeCFG! Ignoring request...\n", client.c_str());
      return grpc::Status(grpc::StatusCode::UNKNOWN, "Credentials not found, please contact the administrator.");
    }
    else if(!strcmp(err, RUNTIME_EXCEPT_TABLE_FULL)){
      fprintf(stderr, "Exept: Resource exhausted on table insert!\n");
      return grpc::Status(grpc::StatusCode::RESOURCE_EXHAUSTED, "Table is full.");
    }
    else if(!strcmp(err, RUNTIME_EXCEPT_MALFORMED_REQUEST)){
      fprintf(stderr, "Exept: Malformed write request!\n");
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid request.");
    }
    else if(!strcmp(err, RUNTIME_EXCEPT_IDLE_TIMEOUT_NOT_SUPPORTED)){
      fprintf(stderr, "Exept: Write request with invalid idle timeout\n");
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

grpc::Status P4RuntimeImpl::Read(grpc::ServerContext* context, const p4::v1::ReadRequest* request, grpc::ServerWriter< p4::v1::ReadResponse>* writer) {
  const std::string client = std::string(context->auth_context().get()->GetPeerIdentity().begin()->begin(), context->auth_context().get()->GetPeerIdentity().begin()->end());
  const std::string controller = std::string(context->peer().c_str());

  if(m_verbose >= VERBOSE_SIMPLE){
    printf("Read from: \"%s - %s\"\n", client.c_str(), controller.c_str());
    if(m_verbose >= VERBOSE_ALL)
      Utils::PrintReadRequest(request);
  }
  
  grpc::Status status = grpc::Status(grpc::StatusCode::OK, "");

  try{
    p4::v1::ReadRequest r = this->m_runtimeCFG->Translate(client, request);
    
    if(m_verbose >= VERBOSE_ALL){
      printf("Translated:\n");
      Utils::PrintReadRequest(&r);
    }

    grpc::ClientContext scontext;
    std::unique_ptr<grpc::ClientReader<p4::v1::ReadResponse>> reader(m_stub->Read(&scontext, r));

    p4::v1::ReadResponse response;
    int i = 0;
    while (reader->Read(&response)) {
      i++;
      
      if(m_verbose >= VERBOSE_ALL){
        printf("Response %d:\n", i);
        Utils::PrintReadResponse(&response);
      }

      p4::v1::ReadResponse translated = this->m_runtimeCFG->Translate(client, &response);
      writer->Write(translated);
    }

    status = reader->Finish();
    printf("Status Code: %d\nMSG: %s\n", (int) status.error_code(), status.error_message().c_str());
  }
  catch (const char* err){
    if(!strcmp(err, RUNTIME_EXCEPT_CLIENT_CFG_NOT_FOUND_MSG)){
      fprintf(stderr, "Client %s not found in RuntimeCFG! Ignoring request...\n", client.c_str());
      return grpc::Status(grpc::StatusCode::UNKNOWN, "Credentials not found, please contact the administrator.");
    }
    else if(!strcmp(err, RUNTIME_EXCEPT_TABLE_FULL)){
      fprintf(stderr, "Resource exhausted on table insert!\n");
      return grpc::Status(grpc::StatusCode::RESOURCE_EXHAUSTED, "Table is full.");
    }
    else if(!strcmp(err, RUNTIME_EXCEPT_MALFORMED_REQUEST)){
      fprintf(stderr, "Malformed read request!\n");
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid request.");
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

grpc::Status P4RuntimeImpl::SetForwardingPipelineConfig(grpc::ServerContext* context, const p4::v1::SetForwardingPipelineConfigRequest* request, p4::v1::SetForwardingPipelineConfigResponse* response) {
  //The SetForwardinPipelineConfig is used by the administration interface to update the dataplane

  const std::string client = std::string(context->auth_context().get()->GetPeerIdentity().begin()->begin(), context->auth_context().get()->GetPeerIdentity().begin()->end());
  if(client != "admin")
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Cannot set the forwarding pipeline directly.");

  /*
  Size of ClientData file - 4 bytes
  ClientData file...
  Size of virtmapfile - 4 bytes
  virtmapfile...
  Number of additional files - 4 bytes
  Size of additional file 1
  Additional file 1...
  ...
  Target binary size - 4 bytes
  Rest of target binary
  */

  printf("SetForwardingPipelineConfig from %s\n", client.c_str());

  //VirtP4Info
  FILE *fVirtP4Info = fopen(TEMP_VIRT_P4INFO_FILE_NAME, "w");
  Utils::SetOutputStream(fVirtP4Info);
  Utils::PrintP4Info(request->config().p4info());
  Utils::SetOutputStream(stdout);
  fclose(fVirtP4Info);

  const int RequestDataSize = (int) request->config().p4_device_config().size();
  const char *data = request->config().p4_device_config().c_str();

  int ClientDataFileSize = 0, VirtMapFileSize = 0, NumAditionalFiles = 0, TargetBinarySize = 0;

  printf("RequestDataSize: %d\n", RequestDataSize);
  try {
    int readData = 0;
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

    if(readData + 4 > RequestDataSize)
      throw "Malformed configuration.";
    TargetBinarySize = Utils::Get4BytesAsInt(data + readData);
    readData = readData + 4;
    if(readData + TargetBinarySize != RequestDataSize)
      throw "Malformed configuration.";
    const std::string targetBinary = std::string(data + readData, TargetBinarySize);
    readData = readData + TargetBinarySize;
    printf("TargetBinarySize: %d\n", TargetBinarySize);

    printf("ReadData: %d\n", readData);

    p4::v1::SetForwardingPipelineConfigRequest targetRequest;
    targetRequest.set_action(p4::v1::SetForwardingPipelineConfigRequest_Action::SetForwardingPipelineConfigRequest_Action_VERIFY_AND_COMMIT);
    //targetRequest.set_device_id(0);
    targetRequest.mutable_election_id()->set_low(1);
    *(targetRequest.mutable_config()->mutable_p4info()) = request->config().p4info();
    *(targetRequest.mutable_config()->mutable_p4_device_config()) = targetBinary;
    grpc::ClientContext targetRequestContext;
    p4::v1::SetForwardingPipelineConfigResponse targetResponse;
    grpc::Status status = m_stub->SetForwardingPipelineConfig(&targetRequestContext, targetRequest, &targetResponse);
    printf("Status Code: %d\nMSG: %s\n", (int) status.error_code(), status.error_message().c_str());
    if(status.error_code() != 0)
        throw "Target Error.";

    this->m_runtimeCFG->Update(TEMP_CLIENT_DATA_FILE_NAME, TEMP_VIRT_MAP_FILE_NAME, TEMP_VIRT_P4INFO_FILE_NAME);
    this->ResetConnectionsAndKnownClients();
  }
  catch (const char *err){
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, grpc::string(err));
  }

  
  grpc::Status status = grpc::Status(grpc::StatusCode::OK, "");
  return status;
}

grpc::Status P4RuntimeImpl::GetForwardingPipelineConfig(grpc::ServerContext* context, const p4::v1::GetForwardingPipelineConfigRequest* request, p4::v1::GetForwardingPipelineConfigResponse* response) {
  const std::string client = std::string(context->auth_context().get()->GetPeerIdentity().begin()->begin(), context->auth_context().get()->GetPeerIdentity().begin()->end());
  const std::string controller = std::string(context->peer().c_str());

  printf("GetForwardingPipelineConfig from %s (%s)\n", controller.c_str(), client.c_str());

  P4InfoDesc* p4infoDesc = m_runtimeCFG->GetClientP4InfoDesc(client);

  if(p4infoDesc == NULL)
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Client data not found");

  *response->mutable_config()->mutable_p4info() = p4infoDesc->GetP4Info();
  
  grpc::Status status = grpc::Status(grpc::StatusCode::OK, "");
  
  if(status.error_code() == grpc::StatusCode::UNAVAILABLE)
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Manager not connected to the switch server. Please try again later or contact the system administrator.");
  return status;
}

grpc::Status P4RuntimeImpl::StreamChannel(grpc::ServerContext* context, grpc::ServerReaderWriter< p4::v1::StreamMessageResponse, p4::v1::StreamMessageRequest>* stream) {
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

  std::mutex lockTTS;
  std::vector<p4::v1::StreamMessageResponse*> packetInsToSend;
  it->second.emplace(std::make_pair(controller, ConnectionInfo{controller, stream, context, 0, {}, false, 0, false, m_runtimeCFG->GetClientP4InfoDesc(client)}));
  ConnectionInfo *connectionInfo = &(it->second.find(controller)->second);

  grpc::Status status = grpc::Status(grpc::StatusCode::OK, "");
 
  //Get the packet-in information from the "controller_packet_metadata" in the p4info file
  connectionInfo->PacketInMetadataTotalBitwidth = 0;
  std::vector<Property>* controllerMetadatasProperty = connectionInfo->p4InfoDesc->getPropertyList()->getProperty("controller_packet_metadata");
  if(controllerMetadatasProperty != NULL){
    for(auto p : *(connectionInfo->p4InfoDesc->getPropertyList()->getProperty("controller_packet_metadata"))){
      PropertyList *packetInfo = p.propList;
      std::string preambleName = packetInfo->getProperty("preamble")->at(0).propList->getProperty("name")->at(0).propString;
      
      if(preambleName != "\"packet_in\"")
        continue;

      std::vector<Property>* packetMetadataProperty = packetInfo->getProperty("metadata");
      if(packetMetadataProperty != NULL){
        for(int i = 0; i < (int) packetMetadataProperty->size(); i++){
          ControllerHeaderMetadata packetInMetadata;
          
          packetInMetadata.bitwidth = std::stoi(packetMetadataProperty->at(i).propList->getProperty("bitwidth")->data()->propString);
          packetInMetadata.id = std::stoi(packetMetadataProperty->at(i).propList->getProperty("id")->data()->propString);
          packetInMetadata.name = packetMetadataProperty->at(i).propList->getProperty("name")->data()->propString;
          
          connectionInfo->PacketInMetadatas.push_back(packetInMetadata);
          connectionInfo->PacketInMetadataTotalBitwidth += packetInMetadata.bitwidth;
        }
      }
    }
  }

  //Get information about the packet-out metadatas for this controller
  int PacketOutMetadataTotalBitwidth = 0;
  std::vector<ControllerHeaderMetadata> PacketOutMetadatas;
  std::vector<Property>* ControllerHeaderMetadataProperty = (connectionInfo->p4InfoDesc->getPropertyList()->getProperty("controller_packet_metadata"));
  if(ControllerHeaderMetadataProperty != NULL){
    for(auto p : *ControllerHeaderMetadataProperty){
      PropertyList *packetInfo = p.propList;
      std::string preambleName = packetInfo->getProperty("preamble")->at(0).propList->getProperty("name")->at(0).propString;
      
      if(preambleName != "\"packet_out\"")
        continue;

      std::vector<Property>* packetMetadata = packetInfo->getProperty("metadata");

      if(packetMetadata != NULL){
        for(int i = 0; i < (int) packetMetadata->size(); i++){
          ControllerHeaderMetadata packetOutMetadata;
          
          packetOutMetadata.bitwidth = std::stoi(packetMetadata->at(i).propList->getProperty("bitwidth")->data()->propString);
          packetOutMetadata.id = std::stoi(packetMetadata->at(i).propList->getProperty("id")->data()->propString);
          packetOutMetadata.name = packetMetadata->at(i).propList->getProperty("name")->data()->propString;
          
          PacketOutMetadatas.push_back(packetOutMetadata);
          PacketOutMetadataTotalBitwidth += packetOutMetadata.bitwidth;
        }
      }
    }
  }

  p4::v1::StreamMessageRequest request;
  while(stream->Read(&request)){
    if(m_verbose >= VERBOSE_SIMPLE)
      printf("StreamMessageRequest from %s (%s)\n", controller.c_str(), client.c_str());
    if(request.has_arbitration()){
      if(m_verbose >= VERBOSE_SIMPLE)
        printf("- Request is MasterArbitrationUpdate\n");

      p4::v1::StreamMessageResponse smr;
      *smr.mutable_arbitration() = request.arbitration();

      if (!request.arbitration().has_election_id()){
        if(m_verbose >= VERBOSE_SIMPLE)
          printf("- ERROR: Election ID is unset\n");
        smr.mutable_arbitration()->mutable_status()->set_code(grpc::StatusCode::INVALID_ARGUMENT);
        smr.mutable_arbitration()->mutable_status()->set_message("ElectionID is unset.");
        stream->Write(smr);
      }
      else if(connectionInfo->electionIDSet){
        if(m_verbose >= VERBOSE_SIMPLE)
          printf("- ERROR: Election ID already set for %s (%ld)\n", controller.c_str(), connectionInfo->electionID);
        smr.mutable_arbitration()->mutable_status()->set_code(grpc::StatusCode::INVALID_ARGUMENT);
        smr.mutable_arbitration()->mutable_status()->set_message("ElectionID is already set.");
        stream->Write(smr);
      }
      else if(m_clientElectionIDsSet.find(client)->second.find(request.arbitration().election_id().low()) != m_clientElectionIDsSet.find(client)->second.end()){
        if(m_verbose >= VERBOSE_SIMPLE)
          printf("- ERROR: Another controller already has election ID %ld\n", request.arbitration().election_id().low());
        smr.mutable_arbitration()->mutable_status()->set_code(grpc::StatusCode::ALREADY_EXISTS);
        smr.mutable_arbitration()->mutable_status()->set_message("ElectionID is repeated.");
        stream->Write(smr);
      }
      else{
        ConnectionInfo* maxElectionIDConnection = NULL;
        uint64_t maxElectionID = 0;

        for (auto& connectionPair : m_connections.find(client)->second) {
          ConnectionInfo& connectionInfo2 = connectionPair.second;
          if (connectionInfo2.electionIDSet) {
            if (connectionInfo2.electionID > maxElectionID) {
              maxElectionID = connectionInfo2.electionID;
              maxElectionIDConnection = &connectionInfo2;
            }
          }
        }

        connectionInfo->electionIDSet = true;
        connectionInfo->electionID = request.arbitration().election_id().low();
        m_clientElectionIDsSet.find(client)->second.insert(connectionInfo->electionID);

        if(maxElectionID < connectionInfo->electionID){
          if(m_verbose >= VERBOSE_SIMPLE)
            printf("- Controller \"%s\" is the new primary for client \"%s\"\n", controller.c_str(), client.c_str());

          if(maxElectionIDConnection != NULL)
            maxElectionIDConnection->isPrimary = false;
          connectionInfo->isPrimary = true;
          smr.mutable_arbitration()->mutable_status()->set_code(grpc::StatusCode::OK);
          smr.mutable_arbitration()->mutable_status()->set_message("Is primary.");
          stream->Write(smr);

          //Send updates to the other controllers
          smr.mutable_arbitration()->mutable_status()->set_code(grpc::StatusCode::UNKNOWN);
          smr.mutable_arbitration()->mutable_status()->set_message("Not primary.");
          
          for(auto& connectionPair : m_connections.find(client)->second)
            if(connectionPair.first != controller)
              connectionPair.second.stream->Write(smr);
        }
        else{
          if(m_verbose >= VERBOSE_SIMPLE)
            printf("- Controller is not primary\n");

          smr.mutable_arbitration()->mutable_status()->set_code(grpc::StatusCode::UNKNOWN);
          smr.mutable_arbitration()->mutable_status()->set_message("Not primary.");
          stream->Write(smr);
        }
      }
    }
    else if(request.has_packet()){ //Packet Out
      const size_t packetMDsize = request.packet().metadata().size();
      
      if(m_verbose >= VERBOSE_SIMPLE)
        printf("- Request is a packet-out\n");
    
      if(!connectionInfo->isPrimary){
        if(m_verbose >= VERBOSE_SIMPLE)
          printf("- ERROR: Controller is not primary\n");
        
        p4::v1::StreamMessageResponse smr;
        smr.mutable_error()->set_canonical_code(grpc::StatusCode::PERMISSION_DENIED);
        smr.mutable_error()->set_message("Not primary.");
        *smr.mutable_error()->mutable_packet_out()->mutable_packet_out() = request.packet();
        stream->Write(smr);
      }
      else if(packetMDsize != PacketOutMetadatas.size()){
        if(m_verbose >= VERBOSE_SIMPLE)
          printf("- ERROR: Packet-out with wrong number of metadatas (%d should be %d)\n", (int) packetMDsize, (int) PacketOutMetadatas.size());
        
        p4::v1::StreamMessageResponse smr;
        smr.mutable_error()->set_canonical_code(grpc::StatusCode::INVALID_ARGUMENT);
        smr.mutable_error()->set_message("Invalid packet-out metadatas.");
        *smr.mutable_error()->mutable_packet_out()->mutable_packet_out() = request.packet();
        stream->Write(smr);
      }
      else{
        p4::v1::StreamMessageResponse smr;

        std::string packetOutHeaderString;
        std::set<int> metadataIDs;
        bool packetOk = true;
        
        for(p4::v1::PacketMetadata packetMD : request.packet().metadata()){
          if(!packetOk)
            continue;
          
          if(metadataIDs.find(packetMD.metadata_id()) != metadataIDs.end()){
            if(m_verbose >= VERBOSE_SIMPLE)
              printf("- ERROR: Repeated packet-out metadata id %d\n", packetMD.metadata_id());
        
            smr.mutable_error()->set_canonical_code(grpc::StatusCode::INVALID_ARGUMENT);
            smr.mutable_error()->set_message("Repeated packet-out metadata id.");
            *smr.mutable_error()->mutable_packet_out()->mutable_packet_out() = request.packet();
            stream->Write(smr);
            packetOk = false;
          }
          else{
            metadataIDs.insert(packetMD.metadata_id());

            int i = 0;
            while(i < (int) PacketOutMetadatas.size() && PacketOutMetadatas[i].id != (int) packetMD.metadata_id()){
              i++;
            }

            if(i == (int) PacketOutMetadatas.size()){
              if(m_verbose >= VERBOSE_SIMPLE)
                printf("- ERROR: Could not find packet-out metadata with id %d\n", packetMD.metadata_id());
        
              smr.mutable_error()->set_canonical_code(grpc::StatusCode::INVALID_ARGUMENT);
              smr.mutable_error()->set_message("Invalid packet-out metadata id.");
              *smr.mutable_error()->mutable_packet_out()->mutable_packet_out() = request.packet();
              stream->Write(smr);
              packetOk = false;
            }
            else{
              std::string strBin = "";
              for(int padding = (int) packetMD.value().size() * 8; padding < PacketOutMetadatas[i].bitwidth; padding++)
                strBin.push_back('0');
              for(int i = 0; i < (int) packetMD.value().size() * 8; i++)
                strBin.push_back((((uint8_t) packetMD.value()[i / 8]) >> (7 - (i % 8))) % 2 == 1 ? '1' : '0');

              packetOutHeaderString = packetOutHeaderString + strBin.substr(strBin.size() - PacketOutMetadatas[i].bitwidth);
            }
          }
        }

        if(packetOk){
          char *headerBytes = new char[packetOutHeaderString.size() / 8];
          memset(headerBytes, 0, packetOutHeaderString.size() / 8);
          for(int i = 0; i < (int) packetOutHeaderString.size() / 8; i++){
            for(int j = 0; j < 8; j++)
              if(packetOutHeaderString[i * 8 + j] == '1')
                headerBytes[i] = headerBytes[i] | ((uint8_t) (1 << (7 - j)));
          }
          request.mutable_packet()->set_payload(std::string(headerBytes, packetOutHeaderString.size() / 8) + request.packet().payload());
          delete[] headerBytes;

          request.mutable_packet()->mutable_metadata()->Clear();
          auto *metadata = request.mutable_packet()->add_metadata();
          
          char clientIDChar = (char) m_runtimeCFG->GetClientID(client);

          metadata->set_metadata_id(1);
          metadata->set_value(std::string(&clientIDChar, 1));

          /*
          printf("ProgramID: %02x\n", metadata->value()[0] >= 0 ? metadata->value()[0] : -metadata->value()[0]);
          printf("PO Payload: ");
          for(int c : request.packet().payload())
            printf("%02x ", c >= 0 ? c : 256 + c);
          printf("\n");
          */

          m_stream->Write(request);
        }
      }
    }
  }

  //Stream channel is closed, cleanup...

  //Changes the primary if this was the primary
  if(connectionInfo->isPrimary && it->second.size() > 1){
    connectionInfo->isPrimary = false;

    ConnectionInfo* maxElectionIDConnection = NULL;
    uint64_t maxElectionID = 0;

    
    for (auto& connectionPair : m_connections.find(client)->second) {
      ConnectionInfo& connectionInfo2 = connectionPair.second;
      if (connectionPair.second.electionIDSet && connectionPair.first != controller) {
        if (connectionInfo2.electionID > maxElectionID) {
          maxElectionID = connectionInfo2.electionID;
          maxElectionIDConnection = &connectionInfo2;
        }
      }
    }

    if(maxElectionIDConnection != NULL){
      maxElectionIDConnection->isPrimary = true;
      p4::v1::StreamMessageResponse smr;
      smr.mutable_arbitration()->mutable_status()->set_code(grpc::StatusCode::OK);
      smr.mutable_arbitration()->mutable_status()->set_message("Is primary.");
      maxElectionIDConnection->stream->Write(smr);

      smr.mutable_arbitration()->mutable_status()->set_code(grpc::StatusCode::UNKNOWN);
      smr.mutable_arbitration()->mutable_status()->set_message("Not primary.");
      for(auto& serverPair : m_connections) {
        for(auto& connectionPair : serverPair.second) {
          if(connectionPair.first != controller && !connectionPair.second.isPrimary){
            connectionPair.second.stream->Write(smr);
          }
        }
      }
    }
  }

  if(connectionInfo->electionIDSet)
    m_clientElectionIDsSet.find(client)->second.erase(connectionInfo->electionID);

  it->second.erase(controller);

  if(m_verbose >= VERBOSE_SIMPLE)
    printf("Stream channel closed for client %s - controller %s\n", client.c_str(), controller.c_str());

  if(status.error_code() == grpc::StatusCode::UNAVAILABLE)
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Manager not connected to the switch server. Please try again later or contact the system administrator.");
  return status;
}

grpc::Status P4RuntimeImpl::Capabilities(grpc::ServerContext* context, const p4::v1::CapabilitiesRequest* request, p4::v1::CapabilitiesResponse* response) {
  grpc::Status status = grpc::Status(grpc::StatusCode::OK, "");

  if(status.error_code() == grpc::StatusCode::UNAVAILABLE)
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Manager not connected to the switch server. Please try again later or contact the system administrator.");
  return status;
}

void P4RuntimeImpl::ResetConnectionsAndKnownClients(){
  for(auto client : m_connections){
    for(auto controller : client.second){
      controller.second.streamContext->TryCancel();
    }
  }

  m_connections.clear();
  m_clientElectionIDsSet.clear();

  for(const std::string& name : m_runtimeCFG->GetClientNames()){
    m_connections.emplace(std::make_pair(name, std::map<std::string, ConnectionInfo>{}));
    m_clientElectionIDsSet.emplace(std::make_pair(name, std::set<uint64_t>{}));
  }
}
