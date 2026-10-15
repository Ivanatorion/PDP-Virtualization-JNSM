#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../include/AST.h"
#include "../include/SymbolTable.h"

SymbolTable::SymbolTable(){
    this->m_parent = NULL;
}

SymbolTable::~SymbolTable(){
    for(auto it : m_table){
        NullDelete(it.second.subtable);
        NullDelete(it.second.decTypeEnum);
    }
}

int checkCommonExpressions(const std::string& expression){
    if(expression.find("isValid()") == expression.size() - 9)
        return 1;
    if(expression.find('[') != std::string::npos && expression.back() == ']'){
        const int n1 = IPLBase::intFromStr(expression.substr(expression.find('[') + 1, expression.find(':') - expression.find('[') - 1).c_str());
        const int n2 = IPLBase::intFromStr(expression.substr(expression.find(':') + 1, expression.size() - 1 - expression.find(':') - 1).c_str());
        Assert(n1 >= 0 && n2 >= 0);
        return n1 - n2 + 1;
    }

    return -1;
}

int SymbolTable::getBitwidthFromExpression(const std::string& expression, const std::string& controlBlock) const{
    Assert(expression.size() > 0);
    
    int commomExp = checkCommonExpressions(expression);
    if(commomExp != -1)
        return commomExp;
    
    if(controlBlock != ""){
        auto it = m_table.find(controlBlock);
        Assert(it != m_table.end());
        Assert(it->second.subtable != NULL);
        return it->second.subtable->getBitwidthFromExpression(expression, "");
    }
    int result = -1;

    if(expression.find(".") != std::string::npos){
        bool isHeaderStack = false;
        std::string firstPart = expression.substr(0, expression.find("."));
        if(firstPart.find('[') != std::string::npos && firstPart.back() == ']'){
            firstPart = firstPart.substr(0, firstPart.find('['));
            isHeaderStack = true;
        }

        SymbolTable const *current = this;
        SymbolTableEntry const *entry = NULL;
        while(entry == NULL && current != NULL){
            if(current->m_table.find(firstPart) != current->m_table.end()){
                entry = &current->m_table.find(firstPart)->second;
            }
            current = current->m_parent;
        }
        Assert(entry != NULL);
        std::string instType = entry->instUserType;
        Assert(instType != "");
        if(isHeaderStack){
            Assert(instType.find(HEADER_STACK_PROGRAM_TYPE_SUFFIX) == instType.size() - strlen(HEADER_STACK_PROGRAM_TYPE_SUFFIX));
            instType = instType.substr(0, instType.find(HEADER_STACK_PROGRAM_TYPE_SUFFIX));
        }

        current = this;
        entry = NULL;
        while(entry == NULL && current != NULL){
            if(current->m_table.find(instType) != current->m_table.end()){
                entry = &current->m_table.find(instType)->second;
            }
            current = current->m_parent;
        }
        Assert(entry != NULL && entry->subtable != NULL);
        result = entry->subtable->getBitwidthFromExpression(expression.substr(expression.find(".") + 1), "");
    }
    else{
        SymbolTable const *current = this;
        SymbolTableEntry const *entry = NULL;
        while(entry == NULL && current != NULL){
            if(current->m_table.find(expression) != current->m_table.end()){
                entry = &current->m_table.find(expression)->second;
            }
            current = current->m_parent;
        }
        Assert(entry != NULL);
        if(entry->type == STET_INST && !entry->instIsUserType)
            result = entry->bitwidth;
    }

    return result;
}

void SymbolTable::addEntry(SymbolTable *root, const std::string& key, AST *ast){
    if(key == "")
        return;

    SymbolTableEntry newEntry;
    newEntry.key = key;
    newEntry.subtable = NULL;
    newEntry.decTypeEnum = NULL;
    newEntry.bitwidth = 0;
    switch(ast->getNodeType()){
        case NT_HEADER_TYPE_DECLARATION:
            newEntry.type = STET_DEC;
            newEntry.dectype = STEDT_HEADER;
            newEntry.subtable = new SymbolTable;
            newEntry.subtable->m_parent = this;
            if(ast->getChildNT(NT_STRUCT_FIELD_LIST) != NULL){
                for(AST *c : ast->getChildNT(NT_STRUCT_FIELD_LIST)->getChildren()){
                    newEntry.subtable->addEntry(this, c->getValue(), c);
                }
            }
            this->m_table.emplace(key, newEntry);
            break;
        case NT_STRUCT_TYPE_DECLARATION:
            newEntry.type = STET_DEC;
            newEntry.dectype = STEDT_STRUCT;
            newEntry.subtable = new SymbolTable;
            newEntry.subtable->m_parent = this;
            if(ast->getChildNT(NT_STRUCT_FIELD_LIST) != NULL){
                for(AST *c : ast->getChildNT(NT_STRUCT_FIELD_LIST)->getChildren()){
                    newEntry.subtable->addEntry(this, c->getValue(), c);
                }
            }
            this->m_table.emplace(key, newEntry);
            break;
        case NT_PARSER_DECLARATION:
            newEntry.type = STET_DEC;
            newEntry.dectype = STEDT_PARSER;
            newEntry.subtable = new SymbolTable;
            newEntry.subtable->m_parent = this;
            if(ast->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST) != NULL){
                for(AST* parameter : ast->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->getChildren())
                    newEntry.subtable->addEntry(this, parameter->getValue(), parameter);
            }
            this->m_table.emplace(key, newEntry);

            break;
        case NT_CONTROL_DECLARATION:
            newEntry.type = STET_DEC;
            newEntry.dectype = STEDT_CONTROL;
            newEntry.subtable = new SymbolTable;
            newEntry.subtable->m_parent = this;
            if(ast->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST) != NULL){
                for(AST* parameter : ast->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->getChildren())
                    newEntry.subtable->addEntry(this, parameter->getValue(), parameter);  
            }
            if(ast->getChildNT(NT_CONTROL_LOCAL_DEC_LIST) != NULL){
                for(AST* localD : ast->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->getChildren()){
                    newEntry.subtable->addEntry(this, localD->getValue(), localD);
                }
            }
            this->m_table.emplace(key, newEntry);

            break;
        case NT_ENUM_DECLARATION:
            {
            newEntry.type = STET_DEC;
            newEntry.dectype = STEDT_ENUM;
            newEntry.decTypeEnum = new DecTypeEnum;
            
            AST *typeRef = ast->getChildren()[1];
            if(typeRef != NULL){
                if(typeRef->getNodeType() == NT_BASE_TYPE && typeRef->getValue() == "bit"){
                    if(typeRef->getChildNT(NT_LITERAL) != NULL)
                        newEntry.bitwidth = IPLBase::intFromStr(typeRef->getChildNT(NT_LITERAL)->str());
                    else if(typeRef->getChildNT(NT_EXPRESSION) != NULL)
                        newEntry.bitwidth = IPLBase::intFromStr(typeRef->getChildNT(NT_EXPRESSION)->str());
                    else
                        Assert(false);
                }
            }
            else{
                newEntry.decTypeEnum->typeBitwidth = 0;
            }
            
            auto result = this->m_table.emplace(key, newEntry);
            if(!result.second)
                delete newEntry.decTypeEnum;
            }

            break;
        case NT_VARIABLE_DECLARATION:
        case NT_STRUCT_FIELD:
        case NT_PARAMETER:
        case NT_INSTANTIATION:
            newEntry.type = STET_INST;
            if(ast->getChildNT(NT_BASE_TYPE) != NULL){
                newEntry.instIsUserType = false;
                if(ast->getChildNT(NT_BASE_TYPE)->getValue() == "bit"){
                    if(ast->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL) != NULL)
                        newEntry.bitwidth = IPLBase::intFromStr(ast->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL)->str());
                    else if(ast->getChildNT(NT_BASE_TYPE)->getChildNT(NT_EXPRESSION) != NULL)
                        newEntry.bitwidth = IPLBase::intFromStr(ast->getChildNT(NT_BASE_TYPE)->getChildNT(NT_EXPRESSION)->str());
                }
                else if(ast->getChildNT(NT_BASE_TYPE)->getValue() == "bool"){
                    newEntry.bitwidth = 1;
                }
                if(newEntry.bitwidth <= 0){
                    /*
                    ast->writeCode(stdout);
                    ast->GenDot(stdout);
                    Assert(false);
                    */
                }
            }
            else if(ast->getChildNT(NT_TYPE_NAME) != NULL){
                newEntry.instIsUserType = true;
                newEntry.instUserType = ast->getChildNT(NT_TYPE_NAME)->getValue();
            }
            else if(ast->getChildNT(NT_HEADER_STACK_TYPE) != NULL){
                Assert(ast->getChildNT(NT_HEADER_STACK_TYPE)->getChildNT(NT_TYPE_NAME) != NULL)
                newEntry.instIsUserType = true;
                newEntry.instUserType = ast->getChildNT(NT_HEADER_STACK_TYPE)->getChildNT(NT_TYPE_NAME)->getValue() + HEADER_STACK_PROGRAM_TYPE_SUFFIX;
            }
            this->m_table.emplace(key, newEntry);
            break;
        case NT_EXTERN_DECLARATION:
            newEntry.type = STET_DEC;
            newEntry.dectype = STEDT_EXTERN;
            newEntry.bitwidth = NO_BITWIDTH;
            this->m_table.emplace(key, newEntry);
            break;
        case NT_ACTION_DECLARATION:
            {
            newEntry.type = STET_DEC;
            newEntry.dectype = STEDT_ACTION;
            newEntry.subtable = new SymbolTable;
            newEntry.subtable->m_parent = this;
            if(ast->getChildNT(NT_PARAMETER_LIST) != NULL){
                for(AST* parameter : ast->getChildNT(NT_PARAMETER_LIST)->getChildren()){
                    newEntry.subtable->addEntry(this, parameter->getValue(), parameter);
                }
            }

            auto result = this->m_table.emplace(key, newEntry);
            if(!result.second)
                delete newEntry.subtable;
            }
            break;
        case NT_TABLE_DECLARATION:
            newEntry.type = STET_DEC;
            newEntry.dectype = STEDT_TABLE;
            this->m_table.emplace(key, newEntry);
            break;
        case NT_PARSER_TYPE_DECLARATION:
            //TODO
            break;
        case NT_CONTROL_TYPE_DECLARATION:
            //TODO
            break;
        case NT_PACKAGE_TYPE_DECLARATION:
            //TODO
            break;
        default:
            Assert(false);
            break;
    }
}

void SymbolTable::addEntry(const std::string& key, AST *ast){
    this->addEntry(NULL, key, ast);    
}

void SymbolTable::addEntry(AST *ast){
    std::string key = ast->getValue();
    if(ast->getNodeType() == NT_CONTROL_DECLARATION)
        key = ast->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getValue();
    if(ast->getNodeType() == NT_PARSER_DECLARATION)
        key = ast->getChildNT(NT_PARSER_TYPE_DECLARATION)->getValue();
    this->addEntry(key, ast);
}

void SymbolTable::Print() const{
    this->Print(0);
}

void SymbolTable::Print(const int depth) const{
    std::string pad;
    for(int i = 0; i < depth; i++)
        pad = pad + "  ";

    for(auto it : m_table){
        SymbolTableEntry entry = it.second;
        printf("%s%s:\n", pad.c_str(), it.first.c_str());
        switch(entry.type){
            case STET_DEC:
                printf("%sType: Declaration ", pad.c_str());
                switch(entry.dectype){
                    case STEDT_ENUM:
                        printf("(Enum)\n");
                        break;
                    case STEDT_EXTERN:
                        printf("(Extern)\n");
                        break;
                    case STEDT_HEADER:
                        printf("(Header)\n");
                        break;
                    case STEDT_STRUCT:
                        printf("(Struct)\n");
                        break;
                    case STEDT_PARSER:
                        printf("(Parser)\n");
                        break;
                    case STEDT_CONTROL:
                        printf("(Control)\n");
                        break;
                    case STEDT_ACTION:
                        printf("(Action)\n");
                        break;
                    case STEDT_TABLE:
                        printf("(Table)\n");
                        break;
                }
                break;
            case STET_INST:
                printf("%sType: Instantiation\n", pad.c_str());
                if(entry.instIsUserType){
                    printf("%s- UserType: %s\n", pad.c_str(), entry.instUserType.c_str());
                }
                else{
                    printf("%s- Bitwidth: %d\n", pad.c_str(), entry.bitwidth);
                }
                break;
        }
        if(entry.subtable != NULL){
            printf("%sSubtable: {\n", pad.c_str());
            entry.subtable->Print(depth + 1);
            printf("%s}", pad.c_str());
        }
        printf("\n");
    }
}