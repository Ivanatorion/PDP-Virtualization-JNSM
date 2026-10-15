#ifndef P4RUNTIME_IMPL_H
#define P4RUNTIME_IMPL_H

typedef struct _controller_header_metadata{
    int id;
    std::string name;
    int bitwidth;
} ControllerHeaderMetadata;

typedef struct _connection_info {
    std::string controllerID; //Usually IP/Port
    
    grpc::ServerReaderWriter< p4::v1::StreamMessageResponse, p4::v1::StreamMessageRequest>* stream;
    grpc::ServerContext* streamContext;

    //Packet-in information
    int PacketInMetadataTotalBitwidth;
    std::vector<ControllerHeaderMetadata> PacketInMetadatas;

    //ElectionID information
    bool electionIDSet; //True if the controller already sent its electionID
    uint64_t electionID;
    bool isPrimary;

    //P4Info information
    P4InfoDesc *p4InfoDesc;
} ConnectionInfo;

class P4RuntimeImpl final : public p4::v1::P4Runtime::Service {
    public:
        P4RuntimeImpl(RuntimeCFG *rtcfg, std::shared_ptr<p4::v1::P4Runtime::Stub> stub, std::shared_ptr<grpc::ClientReaderWriter<p4::v1::StreamMessageRequest, p4::v1::StreamMessageResponse>> stream, uint8_t verbose);

        // Update one or more P4 entities on the target.
        grpc::Status Write(grpc::ServerContext* context, const p4::v1::WriteRequest* request, p4::v1::WriteResponse* response) override;
        // Read one or more P4 entities from the target.
        grpc::Status Read(grpc::ServerContext* context, const p4::v1::ReadRequest* request, grpc::ServerWriter< p4::v1::ReadResponse>* writer) override;
        // Sets the P4 forwarding-pipeline config.
        grpc::Status SetForwardingPipelineConfig(grpc::ServerContext* context, const p4::v1::SetForwardingPipelineConfigRequest* request, p4::v1::SetForwardingPipelineConfigResponse* response) override;
        // Gets the current P4 forwarding-pipeline config.
        grpc::Status GetForwardingPipelineConfig(grpc::ServerContext* context, const p4::v1::GetForwardingPipelineConfigRequest* request, p4::v1::GetForwardingPipelineConfigResponse* response) override;
        // Represents the bidirectional stream between the controller and the
        // switch (initiated by the controller), and is managed for the following
        // purposes:
        // - connection initiation through client arbitration
        // - indicating switch session liveness: the session is live when switch
        //   sends a positive client arbitration update to the controller, and is
        //   considered dead when either the stream breaks or the switch sends a
        //   negative update for client arbitration
        // - the controller sending/receiving packets to/from the switch
        // - streaming of notifications from the switch
        grpc::Status StreamChannel(grpc::ServerContext* context, grpc::ServerReaderWriter< p4::v1::StreamMessageResponse, p4::v1::StreamMessageRequest>* stream) override;
        grpc::Status Capabilities(grpc::ServerContext* context, const p4::v1::CapabilitiesRequest* request, p4::v1::CapabilitiesResponse* response) override;

        //Puts the packet-in in the correct connectioninfo
        void SendStreamResponseToClient(const int clientID, p4::v1::StreamMessageResponse* response);

    private:
        RuntimeCFG *m_runtimeCFG;

        std::shared_ptr<p4::v1::P4Runtime::Stub> m_stub;

        std::shared_ptr<grpc::ClientReaderWriter<p4::v1::StreamMessageRequest, p4::v1::StreamMessageResponse>> m_stream;

        uint8_t m_verbose;

        //For each Client  -- //For each controller -- info
        std::map<std::string, std::map<std::string, ConnectionInfo>> m_connections;

        //Set of all active election IDs for each client
        std::map<std::string, std::set<uint64_t>> m_clientElectionIDsSet;

        void ResetConnectionsAndKnownClients();
};

#endif