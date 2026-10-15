#include <vector>
#include <stack>
#include <map>
#include <unordered_map>
#include <cmath>
#include <cstring>
#include <string>
#include <queue>
#include <chrono>
#include <future>
#include <unordered_set>
#include <set>
#include <cstdio>
#include <filesystem>
#include <tuple>

#define BUFFER_SIZE 256

const char C_RED[16] = "\033[0;31m";
const char C_YELLOW[16] = "\033[0;33m";
const char C_NONE[16] = "\033[0m";

enum P4Architecture {P4A_UNDEFINED = 0, P4A_V1MODEL, P4A_TNA, P4A_NVDA_DOCA};

class AST;
class ASTParser;
class ASTApplyBlockOptimizer;
class TypedefTable;
class SymbolTable;