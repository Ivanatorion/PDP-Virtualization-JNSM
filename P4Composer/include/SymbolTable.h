#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#define NO_BITWIDTH -1

class SymbolTable;

enum SymbolTableEntryType {STET_DEC, STET_INST};
enum SymbolTableEntryDecType {STEDT_EXTERN, STEDT_ENUM, STEDT_HEADER, STEDT_STRUCT, STEDT_PARSER, STEDT_CONTROL, STEDT_ACTION, STEDT_TABLE};

typedef struct _enum_dec_type{
    std::string type;
    int typeBitwidth;
    //TODO: std::vector<std::string> values;
} DecTypeEnum;

typedef struct _symbol_table_entry{
    std::string key;
    SymbolTableEntryType type;
    SymbolTable* subtable;
    int bitwidth;
    
    //For declarations
    SymbolTableEntryDecType dectype;
    DecTypeEnum* decTypeEnum;
    
    //For instantiations
    bool instIsUserType;
    std::string instUserType;
    
} SymbolTableEntry;

class SymbolTable{
    public:
        SymbolTable();
        ~SymbolTable();

        //Returns -1 if not found
        int getBitwidthFromExpression(const std::string& expression, const std::string& controlBlock) const;

        void addEntry(const std::string& key, AST *ast);
        void addEntry(AST *ast);

        void Print() const;

    private:
        SymbolTable *m_parent;
        std::unordered_map<std::string, SymbolTableEntry> m_table;

        void Print(const int depth) const;
        void addEntry(SymbolTable *root, const std::string& key, AST *ast);
};

#endif
