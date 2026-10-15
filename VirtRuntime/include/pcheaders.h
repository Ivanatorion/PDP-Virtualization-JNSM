#include <vector>
#include <stack>
#include <map>
#include <unordered_map>
#include <cmath>
#include <cstring>
#include <string>
#include <queue>
#include <chrono>
#include <thread>
#include <mutex>
#include <future>
#include <unordered_set>
#include <set>
#include <cstdio>
#include <bitset>
#include <filesystem>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpc++/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#define BUFFER_SIZE 256
#define HOTSWAP_DELAY_MS 3000

#define BFRT_CLIENT_ID 0

const char C_RED[16] = "\033[0;31m";
const char C_YELLOW[16] = "\033[0;33m";
const char C_GREEN[16] = "\033[0;34m";
const char C_NONE[16] = "\033[0m";

#define VERBOSE_NONE 0
#define VERBOSE_SIMPLE 1
#define VERBOSE_ALL 2

#define TEMP_DIR_NAME ".tmpVirtRuntime"
#define TEMP_CLIENT_DATA_FILE_NAME TEMP_DIR_NAME "/ClientData.cfg"
#define TEMP_VIRT_MAP_FILE_NAME TEMP_DIR_NAME "/VirtMap.cfg"

#define TEMP_VIRT_P4INFO_FILE_NAME TEMP_DIR_NAME "/VirtP4Info.txt"

#define TEMP_VIRT_BFINFO_FILE_NAME TEMP_DIR_NAME "/virt-bf-rt.json"
