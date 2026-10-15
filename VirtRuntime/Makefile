OPTIMIZATION_FLAG = -O0
CPP_STD = -std=c++17
ADDITIONAL_FLAGS = -Wall -D ENABLE_ASSERTS -g
#ADDITIONAL_FLAGS = -Wall
COMPILER = g++

PROTO_DEPS = proto/bfruntime.grpc.pb.cc proto/p4runtime.grpc.pb.cc proto/bfruntime.pb.cc proto/p4runtime.pb.cc proto/status.pb.cc proto/p4info.pb.cc proto/p4data.pb.cc proto/p4types.pb.cc
PROTO_OBJS = bin/proto/status.pb.o bin/proto/p4types.pb.o bin/proto/p4data.pb.o bin/proto/p4info.pb.o bin/proto/p4runtime.grpc.pb.o bin/proto/p4runtime.pb.o bin/proto/bfruntime.grpc.pb.o bin/proto/bfruntime.pb.o

INCLUDE_DIR = ~/.local/include
LIB_DIR = ~/.local/lib

LIBS = -lprotoc -lgrpc++ -labsl_flags -labsl_flags_parse -lgrpc++_reflection -lprotobuf -lgrpc -lcares -laddress_sorting -lre2 -lupb -labsl_statusor -lgpr -lz -ldl -lm -lrt -labsl_status -labsl_strerror -labsl_random_distributions -labsl_random_seed_sequences -labsl_random_internal_pool_urbg -labsl_random_internal_randen -labsl_random_internal_randen_hwaes -labsl_random_internal_randen_hwaes_impl -labsl_random_internal_randen_slow -labsl_random_internal_platform -labsl_random_internal_seed_material -labsl_random_seed_gen_exception -lssl -lcrypto -labsl_flags_usage -labsl_flags_usage_internal -labsl_flags -labsl_flags_internal -labsl_flags_marshalling -labsl_str_format_internal -labsl_flags_reflection -labsl_flags_config -labsl_flags_private_handle_accessor -labsl_flags_commandlineflag -labsl_flags_commandlineflag_internal -labsl_flags_program_name -labsl_hash -labsl_city -labsl_bad_variant_access -labsl_low_level_hash -labsl_cord -labsl_cordz_info -labsl_cord_internal -labsl_cordz_functions -labsl_cordz_handle -labsl_crc_cord_state -labsl_crc32c -labsl_crc_internal -labsl_crc_cpu_detect -labsl_raw_hash_set -labsl_bad_optional_access -labsl_hashtablez_sampler -labsl_exponential_biased -labsl_synchronization -labsl_graphcycles_internal -labsl_stacktrace -labsl_symbolize -labsl_malloc_internal -labsl_debugging_internal -labsl_demangle_internal -labsl_time -labsl_strings -labsl_strings_internal -labsl_base -labsl_spinlock_wait -lrt -labsl_throw_delegate -labsl_raw_logging_internal -labsl_log_severity -labsl_int128 -labsl_civil_time -labsl_time_zone

all: include/pcheaders.h.gch VP4RT

include/pcheaders.h.gch: include/pcheaders.h
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) include/pcheaders.h

VP4RT: $(PROTO_OBJS) bin/main.o bin/Utils.o bin/RuntimeCFG.o bin/RuntimeCFGBFRT.o bin/P4RuntimeImpl.o bin/BFRuntimeImpl.o bin/BFRTManager.o bin/P4InfoDesc.o bin/BFRTInfoDesc.o bin/PropertyList.o
	@echo "Linking VP4RT..."
	@$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -o VP4RT bin/*.o bin/proto/*.o -L $(LIB_DIR) $(LIBS) $(ADDITIONAL_FLAGS) 

bin/main.o: src/main.cpp include/pcheaders.h include/IPLBase.hpp include/Utils.h include/RuntimeCFG.h include/RuntimeCFGBFRT.h include/PropertyList.h include/P4InfoDesc.h include/BFRTInfoDesc.h include/BFRTManager.h include/P4RuntimeImpl.h $(PROTO_DEPS)
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/main.o -c src/main.cpp $(ADDITIONAL_FLAGS)

bin/Utils.o: src/Utils.cpp include/Utils.h include/pcheaders.h include/BFRTInfoDesc.h $(PROTO_DEPS)
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/Utils.o -c src/Utils.cpp $(ADDITIONAL_FLAGS)

bin/RuntimeCFG.o: src/RuntimeCFG.cpp include/RuntimeCFG.h include/IPLBase.hpp include/pcheaders.h include/PropertyList.h include/P4InfoDesc.h include/Utils.h $(PROTO_DEPS)
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/RuntimeCFG.o -c src/RuntimeCFG.cpp $(ADDITIONAL_FLAGS)

bin/RuntimeCFGBFRT.o: src/RuntimeCFGBFRT.cpp include/RuntimeCFGBFRT.h include/BFRTInfoDesc.h include/IPLBase.hpp include/pcheaders.h $(PROTO_DEPS)
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/RuntimeCFGBFRT.o -c src/RuntimeCFGBFRT.cpp $(ADDITIONAL_FLAGS)

bin/PropertyList.o: src/PropertyList.cpp include/pcheaders.h include/IPLBase.hpp include/PropertyList.h
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/PropertyList.o -c src/PropertyList.cpp $(ADDITIONAL_FLAGS)

bin/P4InfoDesc.o: src/P4InfoDesc.cpp include/pcheaders.h include/IPLBase.hpp include/P4InfoDesc.h include/PropertyList.h $(PROTO_DEPS)
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/P4InfoDesc.o -c src/P4InfoDesc.cpp $(ADDITIONAL_FLAGS)

bin/BFRTInfoDesc.o: src/BFRTInfoDesc.cpp include/pcheaders.h include/IPLBase.hpp include/BFRTInfoDesc.h $(PROTO_DEPS)
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/BFRTInfoDesc.o -c src/BFRTInfoDesc.cpp $(ADDITIONAL_FLAGS)

bin/BFRTManager.o: src/BFRTManager.cpp include/pcheaders.h include/IPLBase.hpp include/BFRTManager.h $(PROTO_DEPS)
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/BFRTManager.o -c src/BFRTManager.cpp $(ADDITIONAL_FLAGS)

bin/P4RuntimeImpl.o: src/P4RuntimeImpl.cpp include/P4RuntimeImpl.h include/pcheaders.h include/Utils.h include/PropertyList.h include/P4InfoDesc.h include/RuntimeCFG.h $(PROTO_DEPS)
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/P4RuntimeImpl.o -c src/P4RuntimeImpl.cpp $(ADDITIONAL_FLAGS)

bin/BFRuntimeImpl.o: src/BFRuntimeImpl.cpp include/BFRuntimeImpl.h include/BFRTInfoDesc.h include/BFRTManager.h include/pcheaders.h include/RuntimeCFGBFRT.h $(PROTO_DEPS)
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/BFRuntimeImpl.o -c src/BFRuntimeImpl.cpp $(ADDITIONAL_FLAGS)

proto/status.pb.cc: proto/status.proto
	protoc -I proto --cpp_out=proto proto/status.proto

proto/p4types.pb.cc: proto/p4types.proto
	protoc -I proto --cpp_out=proto proto/p4types.proto

proto/p4data.pb.cc: proto/p4data.proto
	protoc -I proto --cpp_out=proto proto/p4data.proto

proto/p4info.pb.cc: proto/p4info.proto proto/p4types.proto proto/p4types.pb.cc
	protoc -I proto --cpp_out=proto proto/p4info.proto

proto/p4runtime.grpc.pb.cc: proto/p4runtime.proto proto/status.proto proto/p4info.proto proto/p4data.proto proto/p4types.proto
	protoc -I proto --grpc_out=proto --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` proto/p4runtime.proto

proto/p4runtime.pb.cc: proto/p4runtime.proto proto/status.proto proto/p4info.proto proto/p4data.proto proto/p4types.proto 
	protoc -I proto --cpp_out=proto proto/p4runtime.proto

proto/bfruntime.grpc.pb.cc: proto/bfruntime.proto proto/status.proto
	protoc -I proto --grpc_out=proto --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` proto/bfruntime.proto

proto/bfruntime.pb.cc: proto/bfruntime.proto proto/status.proto 
	protoc -I proto --cpp_out=proto proto/bfruntime.proto

bin/proto/status.pb.o: proto/status.pb.cc
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/proto/status.pb.o -c proto/status.pb.cc

bin/proto/p4types.pb.o: proto/p4types.pb.cc
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/proto/p4types.pb.o -c proto/p4types.pb.cc

bin/proto/p4data.pb.o: proto/p4data.pb.cc
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/proto/p4data.pb.o -c proto/p4data.pb.cc

bin/proto/p4info.pb.o: proto/p4info.pb.cc
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/proto/p4info.pb.o -c proto/p4info.pb.cc

bin/proto/p4runtime.grpc.pb.o: proto/p4runtime.grpc.pb.cc proto/p4runtime.pb.cc proto/status.pb.cc proto/p4info.pb.cc proto/p4types.pb.cc proto/p4data.pb.cc
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/proto/p4runtime.grpc.pb.o -c proto/p4runtime.grpc.pb.cc

bin/proto/p4runtime.pb.o: proto/p4runtime.pb.cc proto/status.pb.cc proto/p4info.pb.cc proto/p4types.pb.cc proto/p4data.pb.cc
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/proto/p4runtime.pb.o -c proto/p4runtime.pb.cc

bin/proto/bfruntime.grpc.pb.o: proto/bfruntime.grpc.pb.cc proto/bfruntime.pb.cc proto/status.pb.cc
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/proto/bfruntime.grpc.pb.o -c proto/bfruntime.grpc.pb.cc

bin/proto/bfruntime.pb.o: proto/bfruntime.pb.cc proto/status.pb.cc
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o bin/proto/bfruntime.pb.o -c proto/bfruntime.pb.cc

#CLI

TestCLI: $(PROTO_OBJS) CLI/bin/main.o CLI/bin/GUI.o CLI/bin/Utils.o
	@echo "Linking TestCLI..."
	@$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o TestCLI CLI/bin/*.o bin/proto/*.o -lncursesw -L $(LIB_DIR) $(LIBS) $(ADDITIONAL_FLAGS)

CLI/bin/main.o: $(PROTO_DEPS) include/Utils.h include/IPLBase.hpp include/pcheaders.h CLI/src/main.cpp CLI/include/GUI.h
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o CLI/bin/main.o -c CLI/src/main.cpp $(ADDITIONAL_FLAGS)

CLI/bin/GUI.o: $(PROTO_DEPS) CLI/include/Utils.h include/IPLBase.hpp include/pcheaders.h CLI/src/GUI.cpp CLI/include/GUI.h
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o CLI/bin/GUI.o -c CLI/src/GUI.cpp $(ADDITIONAL_FLAGS)

CLI/bin/Utils.o: $(PROTO_DEPS) CLI/include/Utils.h include/IPLBase.hpp include/pcheaders.h CLI/src/Utils.cpp
	$(COMPILER) $(CPP_STD) $(OPTIMIZATION_FLAG) -I $(INCLUDE_DIR) -o CLI/bin/Utils.o -c CLI/src/Utils.cpp $(ADDITIONAL_FLAGS)

clean:
	rm -f VP4RT TestCLI
	rm -f bin/*.o bin/proto/*.o
	rm -f CLI/bin/*.o
	rm -f proto/*.cc proto/*.h
	rm -f include/pcheaders.h.gch
