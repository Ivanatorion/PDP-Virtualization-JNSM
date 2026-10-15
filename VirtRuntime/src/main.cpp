#include "../include/pcheaders.h"
#include "../proto/p4runtime.grpc.pb.h"
#include "../proto/bfruntime.grpc.pb.h"
#include "../include/PropertyList.h"
#include "../include/P4InfoDesc.h"
#include "../include/BFRTInfoDesc.h"
#include "../include/RuntimeCFG.h"
#include "../include/RuntimeCFGBFRT.h"
#include "../include/BFRTManager.h"
#include "../include/BFRuntimeImpl.h"
#include "../include/P4RuntimeImpl.h"
#include "../include/IPLBase.hpp"
#include "../include/Utils.h"

#define PI_PROGID_METADATAID 1

#define SLEEP(X) std::this_thread::sleep_for(std::chrono::milliseconds(X));

typedef struct _certs{
    std::string key;
    std::string cert;
    std::string root;
} Certificates;

Certificates getCertificates(){
    const std::vector<std::string> Paths = {"certificates/VirtRuntime.key", "certificates/VirtRuntime.crt", "certificates/TestbedCA.crt"};
    std::string fileReads[3];

    for(int i = 0; i < 3; i++){
        FILE *fp = fopen(Paths[i].c_str(), "r");
        if(!fp){
            fprintf(stderr, "Error: Could not open file \"%s\"\n", Paths[i].c_str());
            exit(1);
        }
        while(!feof(fp)){
            char c = fgetc(fp);
            if(!feof(fp))
                fileReads[i].push_back(c);
        }
        fclose(fp);
    }

    return Certificates({fileReads[0], fileReads[1], fileReads[2]});
}

void parseArgs(int argc, char *argv[], bool *help, std::string* clientFilePath, std::string* virtMapFilePath, std::string* virtP4InfoPath, std::string* virtBFRTInfoFilePath, std::string* virtServerAdd, std::string* virtTargetAdd){
    int i = 1;
    while(i < argc){
        if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")){
            *help = true;
        }
        else if(!strcmp(argv[i], "--clientdata")){
            i++;
            if(i < argc)
                *clientFilePath = std::string(argv[i]);
        }
        else if(!strcmp(argv[i], "--virtmap")){
            i++;
            if(i < argc)
                *virtMapFilePath = std::string(argv[i]);
        }
        else if(!strcmp(argv[i], "--virtp4info")){
            i++;
            if(i < argc)
                *virtP4InfoPath = std::string(argv[i]);
        }
        else if(!strcmp(argv[i], "--virtbfrtinfo")){
            i++;
            if(i < argc)
                *virtBFRTInfoFilePath = std::string(argv[i]);
        }
        else if(!strcmp(argv[i], "--serveraddr")){
            i++;
            if(i < argc)
                *virtServerAdd = std::string(argv[i]);
        }
        else if(!strcmp(argv[i], "--targetaddr")){
            i++;
            if(i < argc)
                *virtTargetAdd = std::string(argv[i]);
        }
        else{
            fprintf(stderr, "Warning: Unrecognized option \"%s\"\n", argv[i]);
        }
        i++;
    }
}

void printHelp(char progName[]){
    printf("Usage: %s [OPTS] (--virtp4info Virt.p4info.txt | --virtbfrtinfo bf-rt.json) --virtmap VirtMap.cfg --clientdata ClientFile.cfg --serveraddr ip:port --targetaddr ip:port\n", progName);
    printf("\nOPTS:\n");
    printf("(--help | -h)          : Displays help message\n");
    printf("(--targetaddr ip:port) : Server addr of the P4runtime server of the target\n");
}

std::unique_ptr<grpc::Server> CreateP4RTServer(const std::string serverAddr, Certificates certificates, P4RuntimeImpl* service){ 
    grpc::ServerBuilder builder;

	grpc::SslServerCredentialsOptions::PemKeyCertPair keycert =
	{
		certificates.key,
		certificates.cert
	};

	grpc::SslServerCredentialsOptions sslOps;
	sslOps.pem_root_certs = certificates.root;
	sslOps.pem_key_cert_pairs.push_back ( keycert );
    sslOps.client_certificate_request = GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
    sslOps.force_client_auth = true;

	builder.AddListeningPort(serverAddr, grpc::SslServerCredentials(sslOps));
	builder.RegisterService(service);

	std::unique_ptr <grpc::Server> server ( builder.BuildAndStart() );
	std::cout << "Server listening on " << serverAddr << std::endl;

	return server;
}

std::unique_ptr<grpc::Server> CreateBFRTServer(const std::string serverAddr, Certificates certificates, BFRuntimeImpl* service){ 
    grpc::ServerBuilder builder;

	grpc::SslServerCredentialsOptions::PemKeyCertPair keycert =
	{
		certificates.key,
		certificates.cert
	};

	grpc::SslServerCredentialsOptions sslOps;
	sslOps.pem_root_certs = certificates.root;
	sslOps.pem_key_cert_pairs.push_back ( keycert );
    sslOps.client_certificate_request = GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
    sslOps.force_client_auth = true;

	builder.AddListeningPort(serverAddr, grpc::SslServerCredentials(sslOps));
	builder.RegisterService(service);

	std::unique_ptr <grpc::Server> server ( builder.BuildAndStart() );
	std::cout << "Server listening on " << serverAddr << std::endl;

	return server;
}

void ProcessStreamChannelWithRunningSwitchP4RT(P4RuntimeImpl* P4RTServer, RuntimeCFG *runtimeCFG, std::shared_ptr<grpc::ClientReaderWriter<p4::v1::StreamMessageRequest, p4::v1::StreamMessageResponse>> stream, const int verboseLevel){
    p4::v1::StreamMessageResponse streamMessageResponse;
    
    while(stream->Read(&streamMessageResponse)){
        if(streamMessageResponse.has_packet()){
            if(verboseLevel >= VERBOSE_SIMPLE)
                printf("Received Packet-In\n");
            int program = 0;
            for(auto m: streamMessageResponse.packet().metadata()){
                if(m.metadata_id() == PI_PROGID_METADATAID){
                    Assert(m.value().size() == 1);
                    program = (int) m.value()[0];
                }
            }
            if(program > 0){
                streamMessageResponse.mutable_packet()->mutable_metadata()->Clear();
                P4RTServer->SendStreamResponseToClient(program, &streamMessageResponse);
            }
        }
        else if(streamMessageResponse.has_idle_timeout_notification()){
            if(verboseLevel >= VERBOSE_SIMPLE)
                printf("Received %d idle_timeout_notification(s)\n", streamMessageResponse.idle_timeout_notification().table_entry().size());
            for(int i = 0; i < (int) streamMessageResponse.mutable_idle_timeout_notification()->mutable_table_entry()->size(); i++){
                p4::v1::StreamMessageResponse partialResponse = streamMessageResponse;

                p4::v1::TableEntry* entry = partialResponse.mutable_idle_timeout_notification()->mutable_table_entry(i);
                
                int j = 0;
                while(j < partialResponse.mutable_idle_timeout_notification()->mutable_table_entry()->size()){
                    if(partialResponse.mutable_idle_timeout_notification()->mutable_table_entry(j) != entry){
                        partialResponse.mutable_idle_timeout_notification()->mutable_table_entry()->erase(partialResponse.mutable_idle_timeout_notification()->mutable_table_entry()->begin() + j);
                        j--;
                    }
                    j++;
                }
                
                VirtTableData vtd = runtimeCFG->GetVirtTableDataForTable(entry->table_id());
                int program = -1;
                if(verboseLevel >= VERBOSE_ALL)
                    Utils::PrintTableEntry(*entry);
                *entry = runtimeCFG->TranslateTableEntryVirtToClient(entry, &program);
                if(verboseLevel >= VERBOSE_ALL)
                    Utils::PrintTableEntry(*entry);
                P4RTServer->SendStreamResponseToClient(program, &partialResponse);
            }
        }
        else{
            printf("Other Response!\n");
        }
    }

    printf("Stream channel with target closed\n");
}

void ProcessStreamChannelWithRunningSwitchBFRT(BFRuntimeImpl* BFRTServer, RuntimeCFGBFRT *runtimeCFGBFRT, std::shared_ptr<grpc::ClientReaderWriter<bfrt_proto::StreamMessageRequest, bfrt_proto::StreamMessageResponse>> stream, const int verboseLevel){
    bfrt_proto::StreamMessageResponse streamMessageResponse;
    
    while(stream->Read(&streamMessageResponse)){
        if(streamMessageResponse.has_digest()){
            printf("- Digest (%u)\n", (int) streamMessageResponse.digest().digest_id());
            
            for(bfrt_proto::TableData& td : *streamMessageResponse.mutable_digest()->mutable_data()){
                
                const bfrt_proto::DataField& dfProgramID = td.fields(0);
                Assert(dfProgramID.stream().size() == 1);
                const int programID = (int) dfProgramID.stream()[0];

                if(verboseLevel >= VERBOSE_ALL)
                    printf("-- Program ID: %d\n", programID);

                td.mutable_fields()->erase(td.mutable_fields()->begin()); //Remove Program ID field

                for(int i = 1; i < td.fields_size(); i++){
                    //Decrement field IDs by 1 to account for removed Program ID field
                    td.mutable_fields(i - 1)->set_field_id(td.fields(i).field_id() - 1);
                    if(verboseLevel >= VERBOSE_ALL){
                        printf("-- Field ID: %d\n", (int) td.fields(i).field_id() - 1);
                        printf("-- Value: ");
                        for(unsigned char c : td.fields(i).stream()){
                            printf("%02X ", c);
                        }
                    }
                }

                BFRTServer->SendStreamResponseToClient(programID, &streamMessageResponse);

            }
        }
        else{
            printf("Other Response!\n");
        }
    }

    printf("Stream channel with target closed\n");
}

std::string BuildTofinoP4DeviceConfigString(const std::string& programName, const std::string& tofinoBinPath, const std::string& contextJSONPath){
    FILE *tofinoBin = fopen(tofinoBinPath.c_str(), "rb");
    if(!tofinoBin){
        fprintf(stderr, "Cannot open file: %s\n", tofinoBinPath.c_str());
        exit(1);
    }
    FILE *contextJSON = fopen(contextJSONPath.c_str(), "rb");
    if(!contextJSON){
        fclose(tofinoBin);
        fprintf(stderr, "Cannot open file: %s\n", contextJSONPath.c_str());
        exit(1);
    }

    std::string tofinoBinSTR;
    while(!feof(tofinoBin)){
        char c = fgetc(tofinoBin);
        if(!feof(tofinoBin))
            tofinoBinSTR.push_back(c);
    }
    std::string contextJSONSTR;
    while(!feof(contextJSON)){
        char c = fgetc(contextJSON);
        if(!feof(contextJSON))
            contextJSONSTR.push_back(c);
    }

    fclose(contextJSON);
    fclose(tofinoBin);

    const size_t ByteCount = 4 + programName.size() + 4 + tofinoBinSTR.size() + 4 + contextJSONSTR.size();

    char *buffer = new char[ByteCount];
    size_t ptr = 0;
    Utils::GetIntAs4Bytes(buffer, programName.size());
    ptr = ptr + 4;
    for(size_t x = 0; x < programName.size(); x++)
        buffer[ptr + x] = programName[x];
    ptr = ptr + programName.size();
    Utils::GetIntAs4Bytes(buffer + ptr, tofinoBinSTR.size());
    ptr = ptr + 4;
    for(size_t x = 0; x < tofinoBinSTR.size(); x++)
        buffer[ptr + x] = tofinoBinSTR[x];
    ptr = ptr + tofinoBinSTR.size();
    Utils::GetIntAs4Bytes(buffer + ptr, contextJSONSTR.size());
    ptr = ptr + 4;
    for(size_t x = 0; x < contextJSONSTR.size(); x++)
        buffer[ptr + x] = contextJSONSTR[x];
    std::string result(buffer, ByteCount);
    delete[] buffer;

    return result;
}

typedef struct {
    int verboseLevel;
    std::string clientFilePath;
    std::string virtMapFilePath;
    std::string virtP4InfoFilePath;
    std::string serverAddr;
    std::string switchServerAddr;
    Certificates certificates;
} P4RTData;

void ProcessP4RT(P4RTData p4rtData){
    RuntimeCFG runtimeCFG(p4rtData.clientFilePath, p4rtData.virtMapFilePath, p4rtData.virtP4InfoFilePath);

    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(p4rtData.switchServerAddr, grpc::InsecureChannelCredentials());
    printf("Waiting for connection...\n");
    
    channel->WaitForConnected(gpr_time_from_seconds(10, gpr_clock_type::GPR_TIMESPAN));
    if(channel->GetState(false) != GRPC_CHANNEL_READY){
        fprintf(stderr, "Could not stabilish connection to %s\n", p4rtData.switchServerAddr.c_str());
        exit(1);
    }
    
    std::shared_ptr<p4::v1::P4Runtime::Stub> stub = p4::v1::P4Runtime::NewStub(channel);
    grpc::ClientContext cContext;
    std::shared_ptr<grpc::ClientReaderWriter<p4::v1::StreamMessageRequest, p4::v1::StreamMessageResponse>> stream(stub->StreamChannel(&cContext));

    //Tofino
    /*    
    printf("Setting up Tofino Switch...\n");
    printf("Setting Forwarding Pipeline...\n");
    p4::v1::SetForwardingPipelineConfigRequest setPipeRequest;
    setPipeRequest.set_action(p4::v1::SetForwardingPipelineConfigRequest_Action::SetForwardingPipelineConfigRequest_Action_VERIFY_AND_COMMIT);
    setPipeRequest.set_device_id(0);
    *(setPipeRequest.mutable_config()->mutable_p4info()) = runtimeCFG.GetMergedP4Info();
    *(setPipeRequest.mutable_config()->mutable_p4_device_config()) = BuildTofinoP4DeviceConfigString("VirtMerged", "NOMSExp/Exp1/tofino.bin", "NOMSExp/Exp1/context.json");
    grpc::ClientContext cContext2;
    p4::v1::SetForwardingPipelineConfigResponse setPipeResponse;
    grpc::Status status = stub->SetForwardingPipelineConfig(&cContext2, setPipeRequest, &setPipeResponse);
    printf("Status Code: %d\nMSG: %s\n", (int) status.error_code(), status.error_message().c_str());
    if(status.error_code() != 0){
        return 1;
    }
    */

    p4::v1::StreamMessageRequest smr;
    p4::v1::StreamMessageResponse smresponse;
    smr.mutable_arbitration()->mutable_election_id()->set_low(0x1);
    smr.mutable_arbitration()->set_device_id(0);
    stream->Write(smr);
    stream->Read(&smresponse);
    printf("Status Code: %d\nMSG: %s\n", (int) smresponse.arbitration().status().code(), smresponse.arbitration().status().message().c_str());

    P4RuntimeImpl service(&runtimeCFG, stub, stream, p4rtData.verboseLevel);

    //std::thread serverThread(RunServerThread, serverAddr, certificates, &service);
    std::unique_ptr<grpc::Server> server = CreateP4RTServer(p4rtData.serverAddr, p4rtData.certificates, &service);
    std::thread streamResponses(ProcessStreamChannelWithRunningSwitchP4RT, &service, &runtimeCFG, stream, p4rtData.verboseLevel);

    streamResponses.join();
    server->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(1));
}

typedef struct {
    int verboseLevel;
    std::string clientFilePath;
    std::string virtMapFilePath;
    std::string virtBFRTInfoFilePath;
    std::string serverAddr;
    Certificates certificates;

    //TODO: Hotswap switches addrs
    std::string switchServerAddr;
} BFRTData;

bfrt_proto::GetForwardingPipelineConfigResponse GetFwdPipeline(std::shared_ptr<bfrt_proto::BfRuntime::Stub> stub){
    grpc::ClientContext cContext;
    
    bfrt_proto::GetForwardingPipelineConfigRequest request;
    request.set_device_id(0);
    request.set_client_id(BFRT_CLIENT_ID);
    bfrt_proto::GetForwardingPipelineConfigResponse response;
    grpc::Status status = stub->GetForwardingPipelineConfig(&cContext, request, &response);
    
    printf("GetForwardingPipelineConfig Status: %d\nMSG: %s\n", status.error_code(), status.error_message().c_str());
    return response;
}

void Bind(std::shared_ptr<bfrt_proto::BfRuntime::Stub> stub, bfrt_proto::ForwardingPipelineConfig fpc){
    grpc::ClientContext cContext;
    
    bfrt_proto::SetForwardingPipelineConfigRequest request;
    request.set_device_id(0);
    request.set_client_id(BFRT_CLIENT_ID);
    bfrt_proto::ForwardingPipelineConfig* ptr = request.mutable_config()->Add();
    ptr->CopyFrom(fpc);
    bfrt_proto::SetForwardingPipelineConfigResponse response;
    grpc::Status status = stub->SetForwardingPipelineConfig(&cContext, request, &response);
  
    printf("Bind Status: %d\nMSG: %s\n", status.error_code(), status.error_message().c_str());
}

void SendSubscribe(std::shared_ptr<grpc::ClientReaderWriter<bfrt_proto::StreamMessageRequest, bfrt_proto::StreamMessageResponse>> stream){
    bfrt_proto::StreamMessageRequest request;
    request.set_client_id(BFRT_CLIENT_ID);
    request.mutable_subscribe()->set_device_id(0);
    request.mutable_subscribe()->mutable_notifications()->set_enable_idletimeout_notifications(true);
    request.mutable_subscribe()->mutable_notifications()->set_enable_learn_notifications(true);
    stream->Write(request);
}

void ProcessBFRT(BFRTData bfrtData){
    //Do not delete, BFRuntimeImpl deletes it in the destructor
    RuntimeCFGBFRT *runtimeCFG = new RuntimeCFGBFRT(bfrtData.clientFilePath, bfrtData.virtMapFilePath, bfrtData.virtBFRTInfoFilePath);

    BfRuntimeConnection bfrtConnection;

    grpc::ChannelArguments cargs;
    int max_message_size = 100 * 1024 * 1024; // 100 MB
    cargs.SetMaxReceiveMessageSize(max_message_size);
    cargs.SetMaxSendMessageSize(max_message_size);
    bfrtConnection.channel = grpc::CreateCustomChannel(
        bfrtData.switchServerAddr, grpc::InsecureChannelCredentials(), cargs);
    
    printf("Waiting for connection...\n");
    
    bfrtConnection.channel->WaitForConnected(gpr_time_from_seconds(10, gpr_clock_type::GPR_TIMESPAN));
    if(bfrtConnection.channel->GetState(false) != GRPC_CHANNEL_READY){
        fprintf(stderr, "Could not stabilish connection to %s\n", bfrtData.switchServerAddr.c_str());
        exit(1);
    }

    bfrtConnection.streamContext = std::make_shared<grpc::ClientContext>(); 
    bfrtConnection.stub = bfrt_proto::BfRuntime::NewStub(bfrtConnection.channel);
    bfrtConnection.stream = bfrtConnection.stub->StreamChannel(bfrtConnection.streamContext.get());
    bfrtConnection.bfrtManager = std::make_shared<BFRTManager>(bfrtConnection.stub, bfrtConnection.stream);

    SendSubscribe(bfrtConnection.stream);

    bfrt_proto::StreamMessageResponse response;
    bfrtConnection.stream->Read(&response);
    if(!response.has_subscribe()){
        fprintf(stderr, "Could not subscribe to %s\n", bfrtData.switchServerAddr.c_str());
        exit(1);
    }

    printf("Subscribed to %s successfully.\n", bfrtData.switchServerAddr.c_str());
    printf("-- Code: %d\n-- Message: %s\n", response.subscribe().status().code(), response.subscribe().status().message().c_str()); 

    if(response.subscribe().status().code() != 0){
        fprintf(stderr, "Subscription to %s failed.\n", bfrtData.switchServerAddr.c_str());
        exit(1);
    }

    bfrt_proto::GetForwardingPipelineConfigResponse rResponse = GetFwdPipeline(bfrtConnection.stub);
            
    //Binds to the program running on the switch, as required by the bfruntime specification
    Bind(bfrtConnection.stub, rResponse.config(0));

    bfrtConnection.bfrtManager->LoadFromMemory(rResponse.non_p4_config().bfruntime_info());
    bfrtConnection.bfrtManager->LoadFromMemory(rResponse.config(0).bfruntime_info());

    //BFRuntimeImpl service(&runtimeCFG, bfrtConnection.stub, bfrtConnection.stream);
    BFRuntimeImpl service(runtimeCFG, bfrtConnection);

    //std::thread serverThread(RunServerThread, serverAddr, certificates, &service);
    std::unique_ptr<grpc::Server> server = CreateBFRTServer(bfrtData.serverAddr, bfrtData.certificates, &service);
    std::thread streamResponses(ProcessStreamChannelWithRunningSwitchBFRT, &service, runtimeCFG, bfrtConnection.stream, bfrtData.verboseLevel);

    streamResponses.join();
    server->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(1));

}

int main(int argc, char *argv[]){
    //const std::string serverAddr = "localhost:9559";
    std::string serverAddr = "";

    //std::string switchServerAddr = "143.54.51.26:9559";
    std::string switchServerAddr = "";
    
    std::string clientFilePath = "";
    std::string virtMapFilePath = "";
    std::string virtP4InfoFilePath = "";
    std::string virtBFRTInfoFilePath = "";

    bool help = false;
    int verboseLevel = VERBOSE_ALL;

    parseArgs(argc, argv, &help, &clientFilePath, &virtMapFilePath, &virtP4InfoFilePath, &virtBFRTInfoFilePath, &serverAddr, &switchServerAddr);

    if(help || clientFilePath == "" || virtMapFilePath == "" || (virtP4InfoFilePath == "" && virtBFRTInfoFilePath == "" ) || switchServerAddr == ""){
        printHelp(argv[0]);
        return 0;
    }

    Certificates certificates = getCertificates();
    
    if(virtP4InfoFilePath != ""){
        P4RTData p4rtData;
        p4rtData.certificates = certificates;
        p4rtData.clientFilePath = clientFilePath;
        p4rtData.serverAddr = serverAddr;
        p4rtData.switchServerAddr = switchServerAddr;
        p4rtData.verboseLevel = verboseLevel;
        p4rtData.virtMapFilePath = virtMapFilePath;
        p4rtData.virtP4InfoFilePath = virtP4InfoFilePath;
        ProcessP4RT(p4rtData);
    }
    else{
        BFRTData bfrtData;
        bfrtData.certificates = certificates;
        bfrtData.clientFilePath = clientFilePath;
        bfrtData.serverAddr = serverAddr;
        bfrtData.switchServerAddr = switchServerAddr;
        bfrtData.verboseLevel = verboseLevel;
        bfrtData.virtMapFilePath = virtMapFilePath;
        bfrtData.virtBFRTInfoFilePath = virtBFRTInfoFilePath;
        ProcessBFRT(bfrtData);
    }

    return 0;
}
