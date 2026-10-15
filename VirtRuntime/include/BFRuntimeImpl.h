#ifndef BFRUNTIME_IMPL_H
#define BFRUNTIME_IMPL_H

typedef struct _connection_info_bfrt {
    std::string controllerID; //Usually IP/Port
    
    grpc::ServerReaderWriter< bfrt_proto::StreamMessageResponse, bfrt_proto::StreamMessageRequest>* stream;
    grpc::ServerContext* streamContext;

    bool isPrimary;

    //P4Info information
    BFRTInfoDesc *bfrtInfoDesc;
} ConnectionInfoBFRT;

typedef struct {
    std::shared_ptr<grpc::Channel> channel;
    std::shared_ptr<bfrt_proto::BfRuntime::Stub> stub;
    std::shared_ptr<grpc::ClientContext> streamContext;
    std::shared_ptr<grpc::ClientReaderWriter<bfrt_proto::StreamMessageRequest, bfrt_proto::StreamMessageResponse>> stream;

    std::shared_ptr<BFRTManager> bfrtManager;
} BfRuntimeConnection;

class BFRuntimeImpl final : public bfrt_proto::BfRuntime::Service {
    public:
        //BFRuntimeImpl(RuntimeCFGBFRT *rtcfg, std::shared_ptr<bfrt_proto::BfRuntime::Stub> stub, std::shared_ptr<grpc::ClientReaderWriter<bfrt_proto::StreamMessageRequest, bfrt_proto::StreamMessageResponse>> stream);
        BFRuntimeImpl(RuntimeCFGBFRT *rtcfg, BfRuntimeConnection s1connection);
        ~BFRuntimeImpl();

        grpc::Status Write(grpc::ServerContext* context, const bfrt_proto::WriteRequest* request, bfrt_proto::WriteResponse* response) override;
      
        grpc::Status Read(grpc::ServerContext* context, const bfrt_proto::ReadRequest* request, grpc::ServerWriter< bfrt_proto::ReadResponse>* writer) override;
       
        grpc::Status SetForwardingPipelineConfig(grpc::ServerContext* context, const bfrt_proto::SetForwardingPipelineConfigRequest* request, bfrt_proto::SetForwardingPipelineConfigResponse* response) override;
       
        grpc::Status GetForwardingPipelineConfig(grpc::ServerContext* context, const bfrt_proto::GetForwardingPipelineConfigRequest* request, bfrt_proto::GetForwardingPipelineConfigResponse* response) override;
      
        grpc::Status StreamChannel(grpc::ServerContext* context, grpc::ServerReaderWriter< bfrt_proto::StreamMessageResponse, bfrt_proto::StreamMessageRequest>* stream) override;

        //Puts the digest in the correct connectioninfo
        void SendStreamResponseToClient(const int clientID, bfrt_proto::StreamMessageResponse* response);

        static void SaveNonP4Config(const std::string& npc, const std::string& fileName);
        static void SaveBfRuntimeInfo(const std::string& bfrtInfo, const std::string& fileName);


    private:
        uint8_t m_verbose;

        RuntimeCFGBFRT *m_runtimeCFGBFRT;

        std::mutex m_stubMutex;
        
        //std::shared_ptr<bfrt_proto::BfRuntime::Stub> m_stub;
        //std::shared_ptr<grpc::ClientReaderWriter<bfrt_proto::StreamMessageRequest, bfrt_proto::StreamMessageResponse>> m_stream;
        BfRuntimeConnection m_s1connection;

        //For each Client  -- //For each controller -- info
        std::map<std::string, std::map<std::string, ConnectionInfoBFRT>> m_connections;

        void ResetConnectionsAndKnownClients();
        grpc::Status ProcessSetForwardingPipelineConfigFromAdmin(grpc::ServerContext* context, const bfrt_proto::SetForwardingPipelineConfigRequest* request, bfrt_proto::SetForwardingPipelineConfigResponse* response);

        void Hotswap(const std::string& auxAddr, const std::string& S2Addr, const std::vector<std::string>& clientsToMigrate, const std::vector<uint32_t>& PortsInAux, const std::vector<uint32_t>& PortsInAuxForS2, RuntimeCFGBFRT *newCFG, const std::string& newRegSelectSwitchValue);
        void HotswapSetAuxPreNode(BfRuntimeConnection *AuxConnection, const uint32_t mcNodeID, const uint16_t mcRID, const std::vector<uint32_t>& portList);
        void HotswapSetAuxPreMGID(BfRuntimeConnection *AuxConnection, const uint16_t mgID, const std::vector<uint32_t>& mcNodeID);
        void HotswapClearAuxFlagRegisters(BfRuntimeConnection *AuxConnection, const std::vector<BFRegisterInfo>& AuxFlagRegisters);
        void HotswapCopyTables(const std::vector<std::string>& clientsToMigrate, RuntimeCFGBFRT *newCFG, BfRuntimeConnection s2connection);
        void HotswapSetMigration(bool migration, const std::vector<std::string>& clientsToMigrate);
        void HotswapCopyRegisters(BfRuntimeConnection *S2Connection, const uint32_t regIDS1, const uint32_t regIDS2, const std::vector<uint32_t>& indexes);

};

inline void BFRuntimeImpl::SaveNonP4Config(const std::string& npc, const std::string& fileName){
    FILE* f = fopen(fileName.c_str(), "wb");
    if(f){
        fwrite(npc.data(), 1, npc.size(), f);
        fclose(f);
    }
}

inline void BFRuntimeImpl::SaveBfRuntimeInfo(const std::string& bfrtInfo, const std::string& fileName){
    FILE* f = fopen(fileName.c_str(), "wb");
    if(f){
        fwrite(bfrtInfo.data(), 1, bfrtInfo.size(), f);
        fclose(f);
    }
}

#endif