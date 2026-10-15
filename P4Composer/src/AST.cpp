#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../include/AST.h"
#include "../include/ASTParser.h"
#include "../include/ASTApplyBlockOptimizer.h"
#include "../include/ASTSimplifier.h"
#include "../include/SymbolTable.h"
#include "../include/TofinoMerge.h"
#include "../include/V1Merge.h"
#include "../include/Log.h"
#include "../include/CFG.h"

#include "../include/IPLBase.hpp"

std::string AST::TAB_SPACES = "";

std::string AST::NextVirtTypeID(){
    static int x = 0;
    return "VirtType" + std::to_string(x++);
}

std::string AST::NodeTypeToString(NodeType nt){
    static const std::unordered_map<NodeType, std::string> nodeTypeToStringMap = {
        {NT_ABSTRACT_FUNCTION_PROTOTYPE, "NT_ABSTRACT_FUNCTION_PROTOTYPE"},
        {NT_ACTION_DECLARATION, "NT_ACTION_DECLARATION"},
        {NT_ACTION_LIST, "NT_ACTION_LIST"},
        {NT_ACTION_REF, "NT_ACTION_REF"},
        {NT_ANNOTATION, "NT_ANNOTATION"},
        {NT_ANNOTATION_BODY, "NT_ANNOTATION_BODY"},
        {NT_ANNOTATION_LIST, "NT_ANNOTATION_LIST"},
        {NT_ANNOTATION_TOKEN, "NT_ANNOTATION_TOKEN"},
        {NT_ANNOTATION_TOKEN_LIST, "NT_ANNOTATION_TOKEN_LIST"},
        {NT_ARGUMENT, "NT_ARGUMENT"},
        {NT_ARGUMENT_LIST, "NT_ARGUMENT_LIST"},
        {NT_ASSIGN_STATEMENT, "NT_ASSIGN_STATEMENT"},
        {NT_BASE_TYPE, "NT_BASE_TYPE"},
        {NT_CONDITIONAL_STATEMENT, "NT_CONDITIONAL_STATEMENT"},
        {NT_CONSTANT_DECLARATION, "NT_CONSTANT_DECLARATION"},
        {NT_CONSTRUCTOR_METHOD_PROTOTYPE, "NT_CONSTRUCTOR_METHOD_PROTOTYPE"},
        {NT_CONTROL_DECLARATION, "NT_CONTROL_DECLARATION"},
        {NT_CONTROL_LOCAL_DEC_LIST, "NT_CONTROL_LOCAL_DEC_LIST"},
        {NT_CONTROL_TYPE_DECLARATION, "NT_CONTROL_TYPE_DECLARATION"},
        {NT_DIRECT_APPLICATION, "NT_DIRECT_APPLICATION"},
        {NT_DIRECTION, "NT_DIRECTION"},
        {NT_EMPTY_STATEMENT, "NT_EMPTY_STATEMENT"},
        {NT_ENTRIES_LIST, "NT_ENTRIES_LIST"},
        {NT_ENTRY, "NT_ENTRY"},
        {NT_ENUM_DECLARATION, "NT_ENUM_DECLARATION"},
        {NT_ERROR_DECLARATION, "NT_ERROR_DECLARATION"},
        {NT_EXPRESSION, "NT_EXPRESSION"},
        {NT_EXPRESSION_LIST, "NT_EXPRESSION_LIST"},
        {NT_EXTERN_DECLARATION, "NT_EXTERN_DECLARATION"},
        {NT_FUNCTION_DECLARATION, "NT_FUNCTION_DECLARATION"},
        {NT_FUNCTION_PROTOTYPE, "NT_FUNCTION_PROTOTYPE"},
        {NT_HEADER_STACK_TYPE, "NT_HEADER_STACK_TYPE"},
        {NT_HEADER_TYPE_DECLARATION, "NT_HEADER_TYPE_DECLARATION"},
        {NT_HEADER_UNION_DECLARATION, "NT_HEADER_UNION_DECLARATION"},
        {NT_IDENTIFIER_LIST, "NT_IDENTIFIER_LIST"},
        {NT_INSTANTIATION, "NT_INSTANTIATION"},
        {NT_KEY_ELEMENT, "NT_KEY_ELEMENT"},
        {NT_KEY_ELEMENT_LIST, "NT_KEY_ELEMENT_LIST"},
        {NT_LITERAL, "NT_LITERAL"},
        {NT_MATCH_KIND_DECLARATION, "NT_MATCH_KIND_DECLARATION"},
        {NT_MASK, "NT_MASK"},
        {NT_METHOD_CALL_STATEMENT, "NT_METHOD_CALL_STATEMENT"},
        {NT_METHOD_PROTOTYPE, "NT_METHOD_PROTOTYPE"},
        {NT_METHOD_PROTOTYPE_LIST, "NT_METHOD_PROTOTYPE_LIST"},
        {NT_NAME_STATE_EXPRESSION, "NT_NAME_STATE_EXPRESSION"},
        {NT_NONE, "NT_NONE"},
        {NT_OBJECT_DECLARATION_LIST, "NT_OBJECT_DECLARATION_LIST"},
        {NT_PACKAGE_TYPE_DECLARATION, "NT_PACKAGE_TYPE_DECLARATION"},
        {NT_PARAMETER, "NT_PARAMETER"},
        {NT_PARAMETER_LIST, "NT_PARAMETER_LIST"},
        {NT_PARSER_DECLARATION, "NT_PARSER_DECLARATION"},
        {NT_PARSER_LOCAL_ELEMENT_LIST, "NT_PARSER_LOCAL_ELEMENT_LIST"},
        {NT_PARSER_STATE, "NT_PARSER_STATE"},
        {NT_PARSER_STATE_LIST, "NT_PARSER_STATE_LIST"},
        {NT_PARSER_STATEMENT_LIST, "NT_PARSER_STATEMENT_LIST"},
        {NT_PARSER_TYPE_DECLARATION, "NT_PARSER_TYPE_DECLARATION"},
        {NT_PROGRAM, "NT_PROGRAM"},
        {NT_RANGE, "NT_RANGE"},
        {NT_RETURN_STATEMENT, "NT_RETURN_STATEMENT"},
        {NT_SELECT_CASE, "NT_SELECT_CASE"},
        {NT_SELECT_CASE_LIST, "NT_SELECT_CASE_LIST"},
        {NT_SELECT_EXPRESSION, "NT_SELECT_EXPRESSION"},
        {NT_SIMPLE_EXPRESSION_LIST, "NT_SIMPLE_EXPRESSION_LIST"},
        {NT_SIMPLE_KEYSET_DEFAULT, "NT_SIMPLE_KEYSET_DEFAULT"},
        {NT_SIMPLE_KEYSET_DONTCARE, "NT_SIMPLE_KEYSET_DONTCARE"},
        {NT_SPECIFIED_IDENTIFIER, "NT_SPECIFIED_IDENTIFIER"},
        {NT_SPECIFIED_IDENTIFIER_LIST, "NT_SPECIFIED_IDENTIFIER_LIST"},
        {NT_STAT_OR_DEC_LIST, "NT_STAT_OR_DEC_LIST"},
        {NT_STRUCT_FIELD, "NT_STRUCT_FIELD"},
        {NT_STRUCT_FIELD_LIST, "NT_STRUCT_FIELD_LIST"},
        {NT_STRUCT_TYPE_DECLARATION, "NT_STRUCT_TYPE_DECLARATION"},
        {NT_SWITCH_CASE, "NT_SWITCH_CASE"},
        {NT_SWITCH_CASE_LIST, "NT_SWITCH_CASE_LIST"},
        {NT_SWITCH_STATEMENT, "NT_SWITCH_STATEMENT"},
        {NT_TABLE_DECLARATION, "NT_TABLE_DECLARATION"},
        {NT_TABLE_PROPERTY, "NT_TABLE_PROPERTY"},
        {NT_TABLE_PROPERTY_LIST, "NT_TABLE_PROPERTY_LIST"},
        {NT_TUPLE_KEYSET_EXPRESSION, "NT_TUPLE_KEYSET_EXPRESSION"},
        {NT_TUPLE_TYPE, "NT_TUPLE_TYPE"},
        {NT_TYPE_ARGUMENT_LIST, "NT_TYPE_ARGUMENT_LIST"},
        {NT_TYPE_NAME, "NT_TYPE_NAME"},
        {NT_TYPE_OR_VOID, "NT_TYPE_OR_VOID"},
        {NT_TYPE_PARAMETER, "NT_TYPE_PARAMETER"},
        {NT_TYPE_PARAMETER_LIST, "NT_TYPE_PARAMETER_LIST"},
        {NT_TYPEARG_DONTCARE, "NT_TYPEARG_DONTCARE"},
        {NT_TYPEARG_NONTYPENAME, "NT_TYPEARG_NONTYPENAME"},
        {NT_TYPEARG_VOID, "NT_TYPEARG_VOID"},
        {NT_TYPEDEF_DECLARATION, "NT_TYPEDEF_DECLARATION"},
        {NT_VALUE_SET_DECLARATION, "NT_VALUE_SET_DECLARATION"},
        {NT_VARIABLE_DECLARATION, "NT_VARIABLE_DECLARATION"}
    };

    auto it = nodeTypeToStringMap.find(nt);
    Assert(it != nodeTypeToStringMap.end());
    return it->second;
}

std::string AST::BaseTypeToString(AST *ast){
    Assert(ast != NULL && ast->nType == NT_BASE_TYPE);
    std::string result;

    if(ast->value == "bit"){
        result = result + "bit<";
        if(ast->getChildNT(NT_LITERAL) != NULL)
            result = result + ast->getChildNT(NT_LITERAL)->value;
        else if(ast->getChildNT(NT_EXPRESSION) != NULL)
            result = result + ast->getChildNT(NT_EXPRESSION)->value;
        result = result + ">";
    }
    else if(ast->value == "bool")
        result = "bool";

    return result;
}

int findPrefix(const std::string& str){
    int ulp = str.find("_");

    if(ulp == (int) std::string::npos || str.size() < 2 || str[0] != 'P')
        return -1;

    if((str[1] == 'S' && str[2] == '_') || IPLBase::intFromStr(str.substr(1, ulp - 1).c_str()) != -1)
        return ulp;

    if(str[1] != 's')
        return -1;
    
    std::string tS;
    int i = 2;
    while(i < ulp){
        if(str[i] == 's'){
            if(IPLBase::intFromStr(tS.c_str()) == -1)
                return -1;
            tS.clear();
        }
        else{
            tS.push_back(str[i]);
        }
        i++;
    }

    return ulp;
}

AST* AST::getRoot(){
    AST *aux = this;
    while(aux->parent != NULL)
      aux = aux->parent;
    return aux;
}

bool AST::MergeRange(AST *r1, AST *r2){
    if(r1->nType == NT_RANGE && r2->nType == NT_RANGE){
        const int r11 = IPLBase::intFromStr(r1->children[0]->str());
        const int r12 = IPLBase::intFromStr(r1->children[1]->str());
        const int r21 = IPLBase::intFromStr(r2->children[0]->str());
        const int r22 = IPLBase::intFromStr(r2->children[1]->str());

        if((r11 <= r22 && r21 - 1 <= r12) || (r21 <= r12 && r11 - 1 <= r22)){
            r1->children[0]->value = r11 < r21 ? r1->children[0]->value : r2->children[0]->value;
            r1->children[1]->value = r12 > r22 ? r1->children[1]->value : r2->children[1]->value;
            return true;
        }
    }

    return false;
}

void AST::WriteParserDOT(const std::string& fileName){
    ASTParser::WriteParserDOT(this, fileName);
}

std::vector<AST*> AST::MergeParsers(std::vector<AST*> programs, std::vector<AST*> parsers, const std::vector<AST*>& headersDec, const std::string& newParserName, const std::vector<std::set<int>>& virtualSwitchPorts, AST* IG_PORT_EXP) {
    return AST::MergeParsers(programs, parsers, headersDec, newParserName, virtualSwitchPorts, IG_PORT_EXP, false);
}

std::vector<AST*> AST::MergeParsers(std::vector<AST*> programs, std::vector<AST*> parsers, const std::vector<AST*>& headersDec, const std::string& newParserName, const std::vector<std::set<int>>& virtualSwitchPorts, AST* IG_PORT_EXP, const bool printErrors) {
    Assert(programs.size() >= 2);
    Assert(programs.size() == parsers.size() && programs.size() == virtualSwitchPorts.size());

    std::string LogM;
    LogM = "Merging Parsers [ ";
    for(int i = 0; i < (int) parsers.size(); i++)
        LogM = LogM + "\"" + parsers[i]->getChildNT(NT_PARSER_TYPE_DECLARATION)->value + "\" ";
    LogM = LogM + "] into \"" + newParserName + "\"";
    Log::MSG(LogM, LOG_LEVEL_SHORT);
    Log::Push();

    char buffer[256];
    std::vector<AST*> newNodes;
    
    //Determine continuous port allocations
    std::vector<std::vector<std::pair<int, int>>> continuousPorts;
    for(const std::set<int>& ports : virtualSwitchPorts)
        continuousPorts.push_back(AST::GetContinuous(ports));

    //Determine first and last ports used
    int FIRST_SWITCH_PORT = 100000;
    int LAST_SWITCH_PORT = 0;
    for(const std::set<int>& ports : virtualSwitchPorts){
        for(int p : ports){
            if(p < FIRST_SWITCH_PORT)
                FIRST_SWITCH_PORT = p;
            if(p > LAST_SWITCH_PORT)
                LAST_SWITCH_PORT = p;
        }
    }

    std::vector<AST*> TempLocalElementLists;
    std::vector<AST*> TempStateLists;
    
    for(AST* parser : parsers){
        TempLocalElementLists.push_back(NULL_CLONE(parser->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)));
        TempStateLists.push_back(NULL_CLONE(parser->getChildNT(NT_PARSER_STATE_LIST)));
    }
    
    AST *newParser = new AST(newParserName, NT_PARSER_DECLARATION);
    AST *ParserTypeDec = new AST(newParserName, NT_PARSER_TYPE_DECLARATION);
    newParser->addChild(ParserTypeDec);
    newParser->addChild(NULL); //optConstructorParameters
    for(int j = 0; j < 2; j++)
        ParserTypeDec->addChild(NULL_CLONE(parsers[0]->getChildNT(NT_PARSER_TYPE_DECLARATION)->children[j]));
    AST *ParserParameterList = new AST("", NT_PARAMETER_LIST);
    ParserTypeDec->addChild(ParserParameterList);

    //Check if the parsers have the same number of parameters
    int nParams = (int) parsers[0]->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children.size();
    for(int j = 1; j < (int) parsers.size(); j++){
        if((int) parsers[j]->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children.size() != nParams){
            for(AST *ast : TempLocalElementLists)
                NullDelete(ast);
            for(AST *ast : TempStateLists)
                NullDelete(ast);
            delete newParser;

            if(printErrors){
                fprintf(stderr, "Error: Cannot merge parsers with different number of parameters (");
                for(int k = 0; k < (int) parsers.size(); k++){
                    fprintf(stderr, "%s - %s", parsers[k]->str(), programs[k]->str());
                    if(k != (int) parsers.size() - 1)
                        fprintf(stderr, ", ");
                }
                fprintf(stderr, ")\n");
            }
            
            Log::Pop();
            return std::vector<AST*>();
        }
    }

    for(int j = 0; j < nParams; j++){
        std::vector<AST*> params;
        for(AST *parser : parsers)
            params.push_back(parser->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[j]);

        sprintf(buffer, "VirtParam%d", j + 1);
        AST *newParam = new AST(std::string(buffer), NT_PARAMETER);
        ParserParameterList->addChild(newParam);
        newParam->addChild(NULL); //TODO: Merge annotations?
        
        AST *direction = params[0]->getChildNT(NT_DIRECTION);
        bool allEqual = true;
        for(int pn = 1; pn < (int) parsers.size(); pn++)
            if(!AST::Equals(direction, params[pn]->getChildNT(NT_DIRECTION)))
                allEqual = false;
        if(allEqual)
            newParam->addChild(NULL_CLONE(direction));
        else
            newParam->addChild(new AST("inout", NT_DIRECTION));
        
        //Check if the types of the parameters are equal (if not, create a new merged type)
        allEqual = true;
        for(int pn = 1; pn < (int) parsers.size(); pn++)
            if(!AST::Equals(params[0]->children[2], params[pn]->children[2]))
                allEqual = false;

        if(!allEqual){
            std::vector<std::string> paramTypes;
            for(int pn = 0; pn < (int) parsers.size(); pn++)
                paramTypes.push_back(params[pn]->children[2]->value);

            bool hasNull = false;
            std::vector<AST*> hPs(parsers.size(), NULL);
            for(int pn = 0; pn < (int) parsers.size(); pn++){
                for(AST *c : headersDec)
                    if(c != NULL && c->value == paramTypes[pn])
                        hPs[pn] = c;
                if(hPs[pn] == NULL){
                    hasNull = true;

                    if(printErrors){
                        fprintf(stderr, "Error: Cannot find header / struct to merge (%s - %s, line %d)\n", paramTypes[pn].c_str(), programs[pn]->str(), params[pn]->children[2]->lineNumber);
                    }
                }
            }
            if(hasNull){
                for(AST *node : newNodes)
                    delete node;
                delete newParser;
                for(AST *TempLEL : TempLocalElementLists)
                    NullDelete(TempLEL);
                for(AST *TempSL : TempStateLists)
                    NullDelete(TempSL);
                Log::Pop();
                return std::vector<AST*>();
            } 

            AST *newHeaderType = new AST(AST::NextVirtTypeID(), hPs[0]->nType);
            newHeaderType->addChild(NULL); //Annotations
            newHeaderType->addChild(NULL); //TypeParameters
            
            bool canMerge = AST::CanMergeStructFieldListByNames(headersDec, paramTypes);
            if(!canMerge){
                delete newHeaderType;
                for(AST *node : newNodes)
                    delete node;
                delete newParser;
                for(AST *TempLEL : TempLocalElementLists)
                    NullDelete(TempLEL);
                for(AST *TempSL : TempStateLists)
                    NullDelete(TempSL);
                Log::Pop();
                return std::vector<AST*>();
            }
            newHeaderType->addChild(AST::MergeStructFieldListByNames(headersDec, paramTypes));

            for(int x = 0; x < (int) TempLocalElementLists.size(); x++){
                if(TempLocalElementLists[x] != NULL)
                    TempLocalElementLists[x]->SubstituteIdentifierWithType(params[x]->value, newParam->value, paramTypes[x]);
                if(TempStateLists[x] != NULL)
                    TempStateLists[x]->SubstituteIdentifierWithType(params[x]->value, newParam->value, paramTypes[x]);
            }

            for(int pn = 0; pn < (int) parsers.size(); pn++){
                std::vector<bool> matchedFieldsInNewHeader(newHeaderType->children[2] == NULL ? 1 : newHeaderType->children[2]->children.size(), false);
                if(hPs[pn]->children[2] != NULL){
                    for(AST *c : hPs[pn]->children[2]->children){
                        int i = 0;
                        while(i < (int) newHeaderType->children[2]->children.size() && (matchedFieldsInNewHeader[i] || !AST::Equals(newHeaderType->children[2]->children[i], c)))
                            i++;
                        Assert(i < (int) newHeaderType->children[2]->children.size());
                        matchedFieldsInNewHeader[i] = true;
                                
                        if(newHeaderType->children[2]->children[i]->value != c->value){
                            if(TempLocalElementLists[pn] != NULL){
                                TempLocalElementLists[pn]->SubstituteStructFieldIdentifier(c->value, newHeaderType->children[2]->children[i]->value, paramTypes[pn]);
                            }
                            if(TempStateLists[pn] != NULL){
                                TempStateLists[pn]->SubstituteStructFieldIdentifier(c->value, newHeaderType->children[2]->children[i]->value, paramTypes[pn]);
                            }
                        }
                    }
                }
            }

            newNodes.push_back(newHeaderType);

            newParam->addChild(new AST(newHeaderType->value, NT_TYPE_NAME));
        
            for(int x = 0; x < (int) TempLocalElementLists.size(); x++){
                size_t y = 0;
                while(y < headersDec.size() && headersDec[y]->value != paramTypes[x]){
                    y++;
                }
                Assert(y < headersDec.size());
                
                if(TempLocalElementLists[x] != NULL){
                    TempLocalElementLists[x]->RenameProgramTypes(hPs[x]->value, newHeaderType->value);
                    TempLocalElementLists[x]->ChangeIdsAfterMergeHeaderLists(headersDec[y], newHeaderType);
                }
                if(TempStateLists[x] != NULL){
                    TempStateLists[x]->RenameProgramTypes(hPs[x]->value, newHeaderType->value);
                    TempStateLists[x]->ChangeIdsAfterMergeHeaderLists(headersDec[y], newHeaderType);
                }
            }
        }
        else{
            newParam->addChild(params[0]->children[2]->Clone());
            for(int x = 0; x < (int) TempLocalElementLists.size(); x++){
                if(TempLocalElementLists[x] != NULL)
                    TempLocalElementLists[x]->SubstituteIdentifierWithType(params[x]->value, newParam->value, params[x]->children[2]->value);
                if(TempStateLists[x] != NULL)
                    TempStateLists[x]->SubstituteIdentifierWithType(params[x]->value, newParam->value, params[x]->children[2]->value);
            }
        }
    }

    //Merge local elements
    AST *LocalElementList = NULL;;
    for(int pn = 0; pn < (int) parsers.size(); pn++){
        if(TempLocalElementLists[pn] != NULL){
            if(LocalElementList == NULL){
                LocalElementList = TempLocalElementLists[pn]->Clone();
                for(AST *LocalElement : LocalElementList->children){
                    sprintf(buffer, "Ps%d_%s", pn + 1, LocalElement->str());
                    std::string newName = std::string(buffer);
                    LocalElementList->SubstituteIdentifierWithType(LocalElement->value, newName, LocalElement->programType);
                    TempStateLists[pn]->SubstituteIdentifierWithType(LocalElement->value, newName, LocalElement->programType);
                
                    LogM = "Adding local element \"" + LocalElement->value + "\" as \"" + newName + "\"";
                    Log::MSG(LogM, LOG_LEVEL_ALL);

                    LocalElement->value = newName;
                }
            }
            else{
                std::unordered_set<AST*> Matched;
                const size_t NumElementsToCompare = LocalElementList->children.size();
                for(AST *localElement : TempLocalElementLists[pn]->children){
                    bool matchedD;
                    std::string newName;
                    switch(localElement->nType){
                        case NT_VARIABLE_DECLARATION:
                        case NT_CONSTANT_DECLARATION:
                        case NT_INSTANTIATION:
                            matchedD = false;
                            for(size_t leIDX = 0; leIDX < NumElementsToCompare; leIDX++){
                                AST *LocalElement = LocalElementList->children.at(leIDX);
                                if(!matchedD && Matched.find(LocalElement) == Matched.end() && AST::Equals(localElement, LocalElement)){
                                    Matched.emplace(LocalElement);
                                    newName = LocalElement->value;
                                    sprintf(buffer, "s%d", pn+1);
                                    newName = newName.substr(0, newName.find("_")) + std::string(buffer) + newName.substr(newName.find("_"));
                                    LocalElementList->SubstituteIdentifierWithType(LocalElement->value, newName, LocalElement->programType);
                                    for(int pn2 = 0; pn2 < pn; pn2++)
                                        TempStateLists[pn2]->SubstituteIdentifierWithType(LocalElement->value, newName, LocalElement->programType);
                                    TempLocalElementLists[pn]->SubstituteIdentifierWithType(localElement->value, newName, LocalElement->programType);
                                    TempStateLists[pn]->SubstituteIdentifierWithType(localElement->value, newName, LocalElement->programType);
                                    LocalElement->value = newName;
                                    matchedD = true;
                                }
                            }
                            if(!matchedD){
                                AST *newElement = localElement->Clone();
                                sprintf(buffer, "Ps%d_%s", pn + 1, localElement->str());
                                newName = std::string(buffer);
                                TempLocalElementLists[pn]->SubstituteIdentifierWithType(localElement->value, newName, localElement->programType);
                                if(TempStateLists[pn] != NULL)
                                    TempStateLists[pn]->SubstituteIdentifierWithType(localElement->value, newName, localElement->programType);
                                newElement->value = newName;
                                LocalElementList->addChild(newElement);
                            }
                            break;
                        default:
                            LocalElementList->addChild(localElement->Clone());
                            break;
                    }
                }
            }
        }
    }
    newParser->addChild(LocalElementList);

    //Get the value set declarations
    std::vector<AST*> ValueSetDeclarations;
    if(LocalElementList != NULL){
        for(AST *localElement : LocalElementList->children){
            if(localElement->nType == NT_VALUE_SET_DECLARATION)
                ValueSetDeclarations.push_back(localElement);
        }
    }

    //Merge States
    std::vector<std::vector<int>> statePrograms;
    AST *StateList = new AST("", NT_PARSER_STATE_LIST);
    newParser->addChild(StateList);
    for(int pn = 0; pn < (int) parsers.size(); pn++){
        for(AST *state : TempStateLists[pn]->children){
            AST *cloneS = state->Clone();
            StateList->addChild(cloneS);
            statePrograms.push_back({pn});
            sprintf(buffer, "P%d_%s", pn + 1, cloneS->value.c_str());
            cloneS->value = std::string(buffer);

            AST* nameStateExp = cloneS->getChildNT(NT_NAME_STATE_EXPRESSION);
            if(nameStateExp != NULL){
                if(nameStateExp->value != "accept" && nameStateExp->value != "reject"){
                    sprintf(buffer, "P%d_%s", pn + 1, nameStateExp->str());
                    nameStateExp->value = std::string(buffer);
                }
            }
            else{
                AST* selectEXP = cloneS->getChildNT(NT_SELECT_EXPRESSION);
                Assert(selectEXP != NULL);
                for(AST *selectCase : selectEXP->children[1]->children){
                    if(selectCase->value != "accept" && selectCase->value != "reject"){
                        sprintf(buffer, "P%d_%s", pn + 1, selectCase->value.c_str());
                        selectCase->value = std::string(buffer);
                    }
                }
            }
        }
    }

    bool tryAgain = true;
    while(tryAgain){
        ASTParser::MergeParserEqualStates(StateList, statePrograms);
        tryAgain = ASTParser::MergeParserSemiEqualStates(StateList, statePrograms, continuousPorts, IG_PORT_EXP, ValueSetDeclarations);
    }

    //Check if we made all select statements the same somehow...
    for(AST *state : StateList->children){
        if(state->getChildNT(NT_SELECT_EXPRESSION) != NULL){
            std::string nextStateName = state->getChildNT(NT_SELECT_EXPRESSION)->children[1]->children[0]->value;
            bool allSame = true;
            for(int j = 1; j < (int) state->getChildNT(NT_SELECT_EXPRESSION)->children[1]->children.size(); j++){
                if(nextStateName != state->getChildNT(NT_SELECT_EXPRESSION)->children[1]->children[j]->value)
                    allSame = false;
            }
            if(allSame){
                delete state->children[2];
                state->children[2] = new AST(nextStateName, NT_NAME_STATE_EXPRESSION);
            }
        }
    }

    //Unify port Ranges
    if(IG_PORT_EXP != NULL){
        AST* PORT_EXPRESSION_AST = IG_PORT_EXP->Clone();
        for(AST *state : StateList->children){
            if(state->getChildNT(NT_SELECT_EXPRESSION) != NULL){
                AST* SCL = state->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_SELECT_CASE_LIST);
                AST* EL = state->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_EXPRESSION_LIST);
                if(AST::Equals(EL->children[0], PORT_EXPRESSION_AST) && EL->children.size() > 1){
                    tryAgain = true;
                    while(tryAgain){
                        std::set<AST*> SCsRemove;
                        tryAgain = false;
                        for(int j = 0; j < (int) SCL->children.size(); j++){
                            if(tryAgain)
                                continue;
                            
                            AST* SCJRange = SCL->children[j]->getChildNT(NT_TUPLE_KEYSET_EXPRESSION)->getChildNT(NT_SIMPLE_EXPRESSION_LIST)->children[0];
                            if(SCJRange->nType == NT_RANGE){
                                if(IPLBase::intFromStr(SCJRange->children[0]->str()) == FIRST_SWITCH_PORT && IPLBase::intFromStr(SCJRange->children[1]->str()) == LAST_SWITCH_PORT){
                                    delete SCJRange->children[0];
                                    delete SCJRange->children[1];
                                    SCJRange->children.clear();
                                    SCJRange->nType = NT_SIMPLE_KEYSET_DONTCARE;
                                    SCJRange->value = "_";
                                }
                            }

                            for(int k = j + 1; k < (int) SCL->children.size(); k++){
                                bool restEqual = SCL->children[j]->value == SCL->children[k]->value;
                                if(restEqual){
                                    for(int e = 1; e < (int) EL->children.size(); e++){
                                       if((k != j + 1 && SCL->children[j]->getChildNT(NT_TUPLE_KEYSET_EXPRESSION)->getChildNT(NT_SIMPLE_EXPRESSION_LIST)->children[e]->nType == NT_SIMPLE_KEYSET_DONTCARE) || !AST::Equals(SCL->children[j]->getChildNT(NT_TUPLE_KEYSET_EXPRESSION)->getChildNT(NT_SIMPLE_EXPRESSION_LIST)->children[e], SCL->children[k]->getChildNT(NT_TUPLE_KEYSET_EXPRESSION)->getChildNT(NT_SIMPLE_EXPRESSION_LIST)->children[e]))
                                            restEqual = false;
                                    }
                                    if(restEqual){
                                        if(AST::MergeRange(SCL->children[j]->getChildNT(NT_TUPLE_KEYSET_EXPRESSION)->getChildNT(NT_SIMPLE_EXPRESSION_LIST)->children[0], SCL->children[k]->getChildNT(NT_TUPLE_KEYSET_EXPRESSION)->getChildNT(NT_SIMPLE_EXPRESSION_LIST)->children[0])){
                                            tryAgain = true;
                                            SCsRemove.emplace(SCL->children[k]);
                                        }
                                    }
                                }
                            }
                        }
                        for(AST *child : SCsRemove){
                            SCL->RemoveChild(child);
                            delete child;
                        }
                    }
                    bool allDontCare = true;
                    for(int j = 0; j < (int) SCL->children.size(); j++){
                        if(SCL->children[j]->getChildNT(NT_TUPLE_KEYSET_EXPRESSION)->getChildNT(NT_SIMPLE_EXPRESSION_LIST)->children[0]->nType != NT_SIMPLE_KEYSET_DONTCARE)
                            allDontCare = false;
                    }
                    if(allDontCare){
                        if(EL->children.size() == 2){
                            delete EL->children[0];
                            EL->children.erase(EL->children.begin());
                            for(int j = 0; j < (int) SCL->children.size(); j++){
                                AST *newValue = SCL->children[j]->getChildNT(NT_TUPLE_KEYSET_EXPRESSION)->getChildNT(NT_SIMPLE_EXPRESSION_LIST)->children[1]->Clone();
                                delete SCL->children[j]->children[0];
                                SCL->children[j]->children[0] = newValue;
                                if(newValue->nType == NT_SIMPLE_KEYSET_DONTCARE){
                                    newValue->nType = NT_SIMPLE_KEYSET_DEFAULT;
                                    newValue->value = "default";
                                }
                            }
                        }
                        else{
                            //TODO
                        }
                    }
                }
            }
        }
        delete PORT_EXPRESSION_AST;
    }

    //Determine start state
    std::unordered_map<AST*, int> stateOrder;
    std::vector<AST*> stateExpandList;

    int hasEqualStartState = 0;
    for(AST *state : StateList->children){
        if(state->value.substr(state->value.find("_") + 1) == "start"){
            hasEqualStartState++;
            stateOrder.emplace(state, 1);
            stateExpandList.push_back(state);
        }
    }
    Assert(hasEqualStartState > 0);
    if(hasEqualStartState > 1){
        AST *startState = new AST("start", NT_PARSER_STATE);
        stateOrder.emplace(startState, 0);
        stateExpandList.push_back(startState);
        startState->addChild(NULL);
        startState->addChild(NULL);
        AST *newTransition = new AST("select", NT_SELECT_EXPRESSION);
        startState->addChild(newTransition);
        AST *newExpList = new AST("", NT_EXPRESSION_LIST);
        AST *newExpression = new AST(".", NT_EXPRESSION);
        newExpression->addChild(new AST("a", NT_EXPRESSION));
        newExpression->addChild(new AST("b", NT_NONE));
        if(IG_PORT_EXP != NULL){
            delete newExpression;
            newExpression = IG_PORT_EXP->Clone();
        }
        newExpList->addChild(newExpression);
        newTransition->addChild(newExpList);
        AST *newSelectCaseList = new AST("", NT_SELECT_CASE_LIST);
        newTransition->addChild(newSelectCaseList);

        for(int i = 0; i < (int) StateList->children.size(); i++){
            AST *state = StateList->children[i];
            int ulp = findPrefix(state->value);
            if(ulp != -1 && state->value.substr(ulp + 1) == "start"){
                for(int program : statePrograms[i]){
                    for(const std::pair<int, int>& portRange : continuousPorts[program]){
                        AST *newSCCase = new AST(state->value, NT_SELECT_CASE);
                        if(portRange.first == portRange.second){
                            sprintf(buffer, "%d", portRange.first);
                            newSCCase->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                        }
                        else{
                            AST *rangeAST = new AST("..", NT_RANGE);
                            sprintf(buffer, "%d", portRange.first);
                            rangeAST->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                            sprintf(buffer, "%d", portRange.second);
                            rangeAST->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                            newSCCase->addChild(rangeAST);
                        }
                        newSelectCaseList->addChild(newSCCase);
                    }
                }
            }
        }
        StateList->addChildStart(startState);
    }
    else{
        stateExpandList[0]->value = "start";
    }

    std::unordered_map<std::string, std::set<std::string>> stateTransitions;

    //Order states
    while(stateExpandList.size() > 0){
        AST *s = stateExpandList[stateExpandList.size() - 1];
        stateExpandList.pop_back();

        if(stateTransitions.find(s->value) == stateTransitions.end())
            stateTransitions.emplace(s->value, std::set<std::string>());

        if(s->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL){
            stateTransitions.find(s->value)->second.insert(s->getChildNT(NT_NAME_STATE_EXPRESSION)->value);
            for(AST* n : StateList->children){
                if(n->value == s->getChildNT(NT_NAME_STATE_EXPRESSION)->value){
                    auto it = stateOrder.find(n);
                    if(it == stateOrder.end()){
                        stateOrder.emplace(n, stateOrder.find(s)->second + 1);
                    }
                    else
                        it->second = (stateOrder.find(s)->second + 1 > it->second) ? stateOrder.find(s)->second + 1 : it->second;
                    stateExpandList.push_back(n);
                }
            }
        }
        else{
            for(AST *sc : s->getChildNT(NT_SELECT_EXPRESSION)->children[1]->children){
                if(stateTransitions.find(s->value)->second.find(sc->value) == stateTransitions.find(s->value)->second.end()){ 
                    stateTransitions.find(s->value)->second.insert(sc->value);
                    for(AST* n : StateList->children){
                        if(n->value == sc->value){
                            auto it = stateOrder.find(n);
                            if(it == stateOrder.end()){
                                stateOrder.emplace(n, stateOrder.find(s)->second + 1);
                            }
                            else
                                it->second = (stateOrder.find(s)->second + 1 > it->second) ? stateOrder.find(s)->second + 1 : it->second;
                            stateExpandList.push_back(n);
                        }
                    }
                }
            }
        }
    }

    for(int i = 1; i < (int) StateList->children.size(); i++){
        int j = i - 1;
        while(j >= 0 && stateOrder.find(StateList->children[j])->second > stateOrder.find(StateList->children[j+1])->second){
            AST *auxS = StateList->children[j];
            StateList->children[j] = StateList->children[j+1];
            StateList->children[j+1] = auxS;
            j--;
        }
    }

    newNodes.push_back(newParser);

    for(AST *TempLEL : TempLocalElementLists)
        NullDelete(TempLEL);
    for(AST *TempSL : TempStateLists)
        NullDelete(TempSL);
    Log::Pop();

    return newNodes;
}

std::string AST::GetVirtInstanceP4InfoName(std::string virtInstanceName){
    size_t pos = virtInstanceName.find("VirtInstance___");
    while(pos == 0){
        virtInstanceName.erase(virtInstanceName.begin(), virtInstanceName.begin() + 15);
        pos = virtInstanceName.find("___");
        Assert(pos != std::string::npos);
        virtInstanceName[pos] = '.';
        virtInstanceName.erase(virtInstanceName.begin() + pos + 1, virtInstanceName.begin() + pos + 3);
        pos = virtInstanceName.find("VirtInstance___");
    }
    return virtInstanceName;
}

std::vector<AST*> AST::Linearize(AST *sList){
    Assert(sList != NULL && sList->nType == NT_STAT_OR_DEC_LIST);

    std::vector<AST*> result;
    for(AST* a : sList->children){
        if(a == NULL)
            continue;

        switch(a->nType){
            case NT_STAT_OR_DEC_LIST:
                {
                std::vector<AST*> tempR = AST::Linearize(a);
                for(AST* r : tempR)
                    result.push_back(r);
                }
                break;
            case NT_CONDITIONAL_STATEMENT:
                {
                AST *ifb = new AST("IF", NT_NONE);
                ifb->addChild(a->children[0]->Clone());
                result.push_back(ifb);
                if(a->children[1] != NULL){
                    if(a->children[1]->nType == NT_STAT_OR_DEC_LIST){
                        std::vector<AST*> tempR = AST::Linearize(a->children[1]);
                        for(AST* r : tempR)
                            result.push_back(r);
                    }
                    else
                        result.push_back(a->children[1]->Clone());
                }
                ifb = new AST("ELSE", NT_NONE);
                result.push_back(ifb);
                if(a->children[2] != NULL){
                     if(a->children[2]->nType == NT_STAT_OR_DEC_LIST){
                        std::vector<AST*> tempR = AST::Linearize(a->children[2]);
                        for(AST* r : tempR)
                            result.push_back(r);
                    }
                    else
                        result.push_back(a->children[2]->Clone());
                }
                ifb = new AST("DONE", NT_NONE);
                result.push_back(ifb);
                }
                break;
            default:
                result.push_back(a->Clone());
                break;
        }
    }
    return result;
}

void AST::RemoveEmptyIfElses(){
    for(AST *c : this->children)
        if(c != NULL)
            c->RemoveEmptyIfElses();

    int i = 0;
    while(i < (int) this->children.size()){
        if(this->children[i] != NULL && this->children[i]->nType == NT_CONDITIONAL_STATEMENT){
            if(this->children[i]->children[0]->value == "true"){
                AST *aux = this->children[i];
                this->children[i] = aux->children[1];
                aux->children[1] = NULL;
                delete aux;

                if(this->children[i] == NULL){
                    this->children.erase(this->children.begin() + i);
                    i--;
                }
            }
            else {
                if(this->children[i]->children[1] != NULL && this->children[i]->children[1]->children.size() == 0){
                    delete this->children[i]->children[1];
                    this->children[i]->children[1] = NULL;
                }

                //If the true statement block is empty, negate the condition and move the false to the true
                if(this->children[i]->children[1] != NULL && this->children[i]->children[1]->nType == NT_STAT_OR_DEC_LIST && this->children[i]->children[1]->children.size() == 0){
                    delete this->children[i]->children[1];
                    this->children[i]->children[1] = NULL;
                }
                if(this->children[i]->children[1] == NULL){
                    this->children[i]->children[1] = this->children[i]->children[2];
                    this->children[i]->children[2] = NULL;
                    AST *condition = this->children[i]->children[0];
                    this->children[i]->RemoveChild(condition);
                    AST *newCondition = new AST("!", NT_EXPRESSION);
                    newCondition->addChild(condition);
                    this->children[i]->addChildStart(newCondition);
                }

                if(this->children[i]->children[2] != NULL && this->children[i]->children[2]->children.size() == 0){
                    delete this->children[i]->children[2];
                    this->children[i]->children[2] = NULL;
                }
                if(this->children[i]->children[2] == NULL && (this->children[i]->children[1] == NULL || this->children[i]->children[1]->children.size() == 0)){
                    delete this->children[i];
                    this->children[i] = NULL;
                    if(this->nType == NT_STAT_OR_DEC_LIST){
                        this->children.erase(this->children.begin() + i);
                        i--;
                    }
                }
            }
        }
        i++;
    }
}

void AST::RemoveEmptySwitches(){
    if(this->nType == NT_STAT_OR_DEC_LIST){
        int i = 0;
        while(i < (int) this->children.size()){
            if(this->children[i] != NULL && this->children[i]->nType == NT_SWITCH_STATEMENT){
                bool removeSS = false;
                if(this->children[i]->getChildNT(NT_SWITCH_CASE_LIST) == NULL)
                    removeSS = true;
                else{
                    AST *SCL = this->children[i]->getChildNT(NT_SWITCH_CASE_LIST);
                    int j = 0;
                    while(j < (int) SCL->children.size()){
                        if(SCL->children[j]->children.size() > 1 && SCL->children[j]->children[1] != NULL && SCL->children[j]->children[1]->children.size() == 0){
                            delete SCL->children[j];
                            SCL->children.erase(SCL->children.begin() + j);
                            j--;
                        }
                        j++;
                    }
                    if(j == 0)
                        removeSS = true;
                    else if(j == 1){ //Substitute switch -> if
                        AST *newCS = new AST("IF", NT_CONDITIONAL_STATEMENT);
                        AST *condEXP = new AST("==", NT_EXPRESSION);
                        condEXP->addChild(this->children[i]->children[0]->Clone());
                        condEXP->addChild(SCL->children[0]->children[0]->Clone());
                        newCS->addChild(condEXP);
                        newCS->addChild(NULL_CLONE(SCL->children[0]->children[1]));
                        newCS->addChild(NULL); //else
                        delete this->children[i];
                        this->children[i] = newCS;
                    }
                }

                if(removeSS){
                    delete this->children[i];
                    this->children.erase(this->children.begin() + i);
                    i--;
                }
            }
            i++;
        }
    }

    for(AST *c : this->children)
        if(c != NULL)
            c->RemoveEmptySwitches();
}

void AST::_RemoveDependencyGraphDummies(AST *graph, std::set<AST*>& dummies){
    for(AST *c : graph->children)
        if(c != NULL)
            AST::_RemoveDependencyGraphDummies(c, dummies);

    const size_t cs = graph->children.size();
    std::vector<size_t> deleteIDXs;
    for(size_t i = 0; i < cs; i++){
        if(graph->children[i]->value == "Dummy"){
            deleteIDXs.push_back(i - deleteIDXs.size());
            dummies.emplace(graph->children[i]);
            for(AST *c : graph->children[i]->children){
                bool isChild = false;
                for(AST *c2 : graph->children)
                    if(c2 == c)
                        isChild = true;
                if(!isChild)
                    graph->addChild(c);
            }
        }
    }

    for(size_t i : deleteIDXs){
        graph->children.erase(graph->children.begin() + i);
    }
}

//TODO: Maybe dependencies can be more througly checked like tofino does
AST* AST::GetSharedTablesDependencyGraph(AST *applyBlock){
    AST *root = new AST("Root", NT_NONE);
    AST *currentParent = root;
    std::vector<AST*> hangingStack;

    std::vector<AST*> linear = AST::Linearize(applyBlock);

    for(AST *ast : linear){
        if(ast->nType == NT_METHOD_CALL_STATEMENT){
            if(ast->children[0]->children.size() > 0 && findPrefix(ast->children[0]->children[0]->value) > 4 && ast->children[0]->children[1]->value == "apply"){ //TODO: find prefix may fail (Ps1000_)
                AST *newNode = ast->children[0]->children[0]->Clone();
                currentParent->addChild(newNode);
                currentParent = newNode;
            }
        }
        else if(ast->value == "IF"){
            hangingStack.push_back(currentParent);
        }
        else if(ast->value == "ELSE"){
            if(currentParent != hangingStack[hangingStack.size() - 1]){
                AST *aux = currentParent;
                currentParent = hangingStack[hangingStack.size() - 1];
                hangingStack[hangingStack.size() - 1] = aux;
            }
        }
        else if(ast->value == "DONE"){
            AST *dummy = new AST("Dummy", NT_NONE);
            hangingStack[hangingStack.size() - 1]->addChild(dummy);
            if(currentParent != hangingStack[hangingStack.size() - 1]){
                currentParent->addChild(dummy);
            }
            hangingStack.pop_back();
            currentParent = dummy;
        }
    }

    for(AST *ast : linear)
        delete ast;

    std::set<AST*> dummies;
    AST::_RemoveDependencyGraphDummies(root, dummies);
    for(AST *dummy : dummies){
        dummy->children.clear();
        delete dummy;
    }

    return root;
}

std::vector<std::string> AST::GetDependencyGraphOrder(AST *graph){
    std::unordered_map<std::string, int> order;
    order.emplace(graph->value, 0);
    std::vector<AST*> stack;
    stack.push_back(graph);

    int nextOrder = 1;

    while(!stack.empty()){
        AST *current = stack[stack.size() - 1];
        stack.pop_back();
        int myOrder = order.find(current->value)->second;
        for(AST *c : current->children){     
            if(order.find(c->value) == order.end()){
                order.emplace(c->value, nextOrder);
                nextOrder++;
                stack.push_back(c);
            }
            else if(order.find(c->value)->second < myOrder + 1){
                order.find(c->value)->second = nextOrder;
                nextOrder++;
                stack.push_back(c);
            }
        }
    }

    std::vector<std::string> result;
    result.reserve(order.size());

    //Flip the map and fill the result
    std::map<int, std::string> flip;
    for(auto& p : order)
        flip.emplace(p.second, p.first);
    for(auto& p : flip)
        result.push_back(p.second);

    return result;
}

std::vector<std::string> AST::GetTableOrderFromLinearize(AST *applyBlock){
    std::vector<AST*> linear = AST::Linearize(applyBlock);
    std::vector<std::string> result;

    for(AST *ast : linear){
        if(ast->nType == NT_METHOD_CALL_STATEMENT){
            if(ast->children[0]->children.size() > 0 && findPrefix(ast->children[0]->children[0]->value) > 4 && ast->children[0]->children[1]->value == "apply"){ //TODO: find prefix may fail (Ps1000_)
                result.push_back(ast->children[0]->children[0]->value);
            }
        }
    }

    for(AST *ast : linear)
        delete ast;

    return result;
}

std::vector<AST*> AST::SplitApply(AST* apply, const std::vector<std::string>& sharedTNs, int *maxIfDepth){
    char buffer[BUFFER_SIZE];
    int ifcount = 0;
    
    std::vector<AST*> result;
    AST* tempSTL = new AST("", NT_STAT_OR_DEC_LIST);
    AST* rootSTL = tempSTL;
    result.push_back(tempSTL);
    
    int tCounter = 0;

    //TODO: Switch statements
    std::vector<bool> ifStack;
    std::vector<AST*> ifChain;
    std::vector<int> ifNumStack;
    std::vector<AST*> linear = AST::Linearize(apply);
    
    for(int x = 0; x < (int) linear.size(); x++){
        if(linear[x]->value == "IF"){
            ifcount++;
            if(ifcount > *maxIfDepth)
                *maxIfDepth = ifcount;
            ifStack.push_back(true);
            ifNumStack.push_back(ifcount);
            AST* ifS = new AST("IF", NT_CONDITIONAL_STATEMENT);
            ifS->addChild(linear[x]->children[0]->Clone());
            ifS->addChild(new AST("", NT_STAT_OR_DEC_LIST));
            ifS->addChild(new AST("", NT_STAT_OR_DEC_LIST));
            ifChain.push_back(ifS);
            tempSTL->addChild(ifS);
            tempSTL = ifS->children[1];
            tempSTL->addChild(new AST("=", NT_ASSIGN_STATEMENT));
            sprintf(buffer, "VirtIF_%d", ifcount);
            tempSTL->children[tempSTL->children.size() - 1]->addChild(new AST(std::string(buffer), NT_NONE));
            tempSTL->children[tempSTL->children.size() - 1]->addChild(new AST("true", NT_EXPRESSION));
        }
        else if(linear[x]->value == "ELSE"){
            ifStack.pop_back();
            ifStack.push_back(false);
            tempSTL = ifChain[ifChain.size() - 1]->children[2];
        }
        else if(linear[x]->value == "DONE"){
            ifStack.pop_back();
            ifChain.pop_back();
            ifNumStack.pop_back();
            if(ifChain.size() > 0)
                tempSTL = ifChain[ifChain.size() - 1]->children[1];
            else
                tempSTL = rootSTL;
        }
        else{
            if(linear[x]->nType == NT_METHOD_CALL_STATEMENT && linear[x]->children[0]->children.size() > 0){
                int newTCounter = tCounter;
                while(newTCounter < (int) sharedTNs.size() && linear[x]->children[0]->children[0]->value != sharedTNs[newTCounter])
                    newTCounter++;
                if(newTCounter < (int) sharedTNs.size()){
                    while(tCounter != newTCounter){
                        tCounter++;
                        result.push_back(NULL);
                    }

                    sprintf(buffer, "VirtApplyT%d", tCounter + 1);
                    tempSTL->addChild(new AST("=", NT_ASSIGN_STATEMENT));
                    tempSTL->children[tempSTL->children.size() - 1]->addChild(new AST(std::string(buffer), NT_NONE));
                    tempSTL->children[tempSTL->children.size() - 1]->addChild(new AST("true", NT_EXPRESSION));
                    tempSTL = new AST("", NT_STAT_OR_DEC_LIST);
                    rootSTL = tempSTL;
                    result.push_back(tempSTL);
                    tCounter++;
                    for(int i = 0; i < (int) ifStack.size(); i++){
                        AST *newIF = new AST("IF", NT_CONDITIONAL_STATEMENT);
                        ifChain[i] = newIF;
                        sprintf(buffer, "VirtIF_%d", ifNumStack[i]);
                        AST *condition; 
                        if(ifStack[i])
                            condition = new AST(std::string(buffer), NT_EXPRESSION);
                        else{
                            condition = new AST("!", NT_EXPRESSION);
                            condition->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                        }
                        newIF->addChild(condition);
                        newIF->addChild(new AST("", NT_STAT_OR_DEC_LIST));
                        newIF->addChild(new AST("", NT_STAT_OR_DEC_LIST));
                        tempSTL->addChild(newIF);
                        tempSTL = newIF->children[1];
                    }
                }
                else{
                    tempSTL->addChild(linear[x]->Clone());
                }
            }
            else{
                tempSTL->addChild(linear[x]->Clone());
            }
        }
    }

    while(result.size() < sharedTNs.size() + 1)
        result.push_back(NULL);

    for(AST* ast : linear)
        delete ast;
    
    return result;
}

std::string AST::RemoveMergePrefix(const std::string& str){
    const int prefix = findPrefix(str);
    if(prefix == -1)
        return str;

    return str.substr(prefix + 1);
}

bool AST::isP4Keyword(const std::string& word){
  const static std::unordered_set<std::string> P4Keywords {"select", "if", "typedef", "bool", "int", "error", "bit", "varbit", "const", "control", "apply", "parser", "type", "default", "_", "exit", "table", "mask", "range", "switch", "struct", "value_set"
                                                           "header", "header_union", "in", "out", "else", "match_kind", "key", "action", "actions", "state", "entries", "void", "pragma", "true", "false", "this", "return", "package", "enum", "transition", "tuple", "extern", "abstract"};

  return P4Keywords.find(word) != P4Keywords.end();
}

std::vector<AST*> AST::MergeControls(const std::vector<AST*>& programs, const std::vector<AST*>& controls, const std::vector<AST*>& headersDec, const std::string& newControlName, const std::vector<std::set<int>>& virtualSwitchPorts) {
    return AST::MergeControls(programs, controls, headersDec, newControlName, virtualSwitchPorts, false);
}

std::vector<AST*> AST::MergeControls(const std::vector<AST*>& programs, const std::vector<AST*>& controls, const std::vector<AST*>& headersDec, const std::string& newControlName, const std::vector<std::set<int>>& virtualSwitchPorts, const bool printErrors) {
    Assert(programs.size() >= 2);
    Assert(programs.size() == controls.size() && programs.size() == virtualSwitchPorts.size());
    
    std::string LogM;
    LogM = "Merging Control Blocks [ ";
    for(int i = 0; i < (int) controls.size(); i++)
        LogM = LogM + "\"" + controls[i]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value + "\" ";
    LogM = LogM + "] into \"" + newControlName + "\"";
    Log::MSG(LogM, LOG_LEVEL_SHORT);
    Log::Push();
    
    char buffer[256];
    std::vector<AST*> newNodes;

    //Determine continuous port allocations
    std::vector<std::vector<std::pair<int, int>>> continuousPorts;
    for(const std::set<int>& ports : virtualSwitchPorts)
        continuousPorts.push_back(AST::GetContinuous(ports));
    
    std::vector<AST*> TempLocalElementLists;
    std::vector<AST*> TempApplyLists;
    for(AST* control : controls){
        TempLocalElementLists.push_back(NULL_CLONE(control->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)));
        TempApplyLists.push_back(NULL_CLONE(control->getChildNT(NT_STAT_OR_DEC_LIST)));
    }

    AST *newControl = new AST(newControlName, NT_CONTROL_DECLARATION);
    AST *ControlTypeDec = new AST(newControlName, NT_CONTROL_TYPE_DECLARATION);
    newControl->addChild(ControlTypeDec);
    newControl->addChild(NULL); //optConstructorParameters
    
    for(int j = 0; j < 2; j++)
        ControlTypeDec->addChild(NULL_CLONE(controls[0]->children[0]->children[j]));
    AST *ControlParameterList = new AST("", NT_PARAMETER_LIST);
    ControlTypeDec->addChild(ControlParameterList);

    //Check if the controls have the same number of parameters
    int nParams = (int) controls[0]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children.size();
    for(int j = 1; j < (int) controls.size(); j++){
        if((int) controls[j]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children.size() != nParams){
            for(AST *ast : TempLocalElementLists)
                NullDelete(ast);
            for(AST *ast : TempApplyLists)
                NullDelete(ast);
            delete newControl;

            if(printErrors){
                fprintf(stderr, "Error: Cannot merge controls with different number of parameters (");
                for(int k = 0; k < (int) controls.size(); k++){
                    fprintf(stderr, "%s - %s", controls[k]->str(), programs[k]->str());
                    if(k != (int) controls.size() - 1)
                        fprintf(stderr, ", ");
                }
                fprintf(stderr, ")\n");
            }
            Log::Pop();
            return std::vector<AST*>();
        }
    }
    
    for(int j = 0; j < nParams; j++){
        std::vector<AST*> params;
        for(AST *control : controls)
            params.push_back(control->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[j]);

        sprintf(buffer, "VirtParam%d", j + 1);
        AST *newParam = new AST(std::string(buffer), NT_PARAMETER);
        ControlParameterList->addChild(newParam);
        newParam->addChild(NULL); //TODO: Merge annotations?

        AST *direction = params[0]->getChildNT(NT_DIRECTION);
        bool allEqual = true;
        for(int pn = 1; pn < (int) controls.size(); pn++)
            if(!AST::Equals(direction, params[pn]->getChildNT(NT_DIRECTION)))
                allEqual = false;
        if(allEqual)
            newParam->addChild(NULL_CLONE(direction));
        else
            newParam->addChild(new AST("inout", NT_DIRECTION));
        
        allEqual = true;
        for(int pn = 1; pn < (int) controls.size(); pn++)
            if(!AST::Equals(params[0]->children[2], params[pn]->children[2]))
                allEqual = false;

        if(!allEqual){
            std::vector<std::string> paramTypes;
            for(int pn = 0; pn < (int) controls.size(); pn++)
                paramTypes.push_back(params[pn]->children[2]->value);

            bool hasNull = false;
            std::vector<AST*> hPs(controls.size(), NULL);
            for(int pn = 0; pn < (int) controls.size(); pn++){
                for(AST *c : headersDec)
                    if(c != NULL && c->value == paramTypes[pn])
                        hPs[pn] = c;
                if(hPs[pn] == NULL)
                    hasNull = true;
            }
            if(hasNull){
                for(AST *node : newNodes)
                    delete node;
                delete newControl;
                for(AST *TempLEL : TempLocalElementLists)
                    NullDelete(TempLEL);
                for(AST *TempAL : TempApplyLists)
                    NullDelete(TempAL);
                Log::Pop();
                return std::vector<AST*>();
            }
            

            AST *newHeaderType = new AST(AST::NextVirtTypeID(), hPs[0]->nType);
            newHeaderType->addChild(NULL); //Annotations
            newHeaderType->addChild(NULL); //TypeParameters
            
            newHeaderType->addChild(AST::MergeStructFieldListByNames(headersDec, paramTypes));

            for(int x = 0; x < (int) TempLocalElementLists.size(); x++){
                if(TempLocalElementLists[x] != NULL)
                    TempLocalElementLists[x]->SubstituteIdentifierWithType(params[x]->value, newParam->value, paramTypes[x]);
                
                if(TempApplyLists[x] != NULL){
                    TempApplyLists[x]->SubstituteIdentifierWithType(params[x]->value, newParam->value, paramTypes[x]);  
                }
            }

            for(int pn = 0; pn < (int) controls.size(); pn++){
                std::vector<bool> matchedFieldsInNewHeader(newHeaderType->children[2] == NULL ? 1 : newHeaderType->children[2]->children.size(), false);
                if(hPs[pn]->children[2] != NULL){
                    for(AST *c : hPs[pn]->children[2]->children){
                        int i = 0;
                        while(i < (int) newHeaderType->children[2]->children.size() && (matchedFieldsInNewHeader[i] || !AST::Equals(newHeaderType->children[2]->children[i], c)))
                            i++;
                        Assert(i < (int) newHeaderType->children[2]->children.size());
                        matchedFieldsInNewHeader[i] = true;
                        
                                
                        if(newHeaderType->children[2]->children[i]->value != c->value){
                            if(TempLocalElementLists[pn] != NULL){
                                TempLocalElementLists[pn]->SubstituteStructFieldIdentifier(c->value, newHeaderType->children[2]->children[i]->value, paramTypes[pn]);
                            }
                            if(TempApplyLists[pn] != NULL){
                                TempApplyLists[pn]->SubstituteStructFieldIdentifier(c->value, newHeaderType->children[2]->children[i]->value, paramTypes[pn]);
                            }
                        }
                    }
                }
            }

            newNodes.push_back(newHeaderType);

            newParam->addChild(new AST(newHeaderType->value, NT_TYPE_NAME));
        
            for(int x = 0; x < (int) TempLocalElementLists.size(); x++){
                size_t y = 0;
                while(y < headersDec.size() && headersDec[y]->value != paramTypes[x]){
                    y++;
                }
                Assert(y < headersDec.size());
                
                if(TempLocalElementLists[x] != NULL){
                    TempLocalElementLists[x]->RenameProgramTypes(hPs[x]->value, newHeaderType->value);
                    TempLocalElementLists[x]->ChangeIdsAfterMergeHeaderLists(headersDec[y], newHeaderType);
                }
                if(TempApplyLists[x] != NULL){
                    TempApplyLists[x]->RenameProgramTypes(hPs[x]->value, newHeaderType->value);
                    TempApplyLists[x]->ChangeIdsAfterMergeHeaderLists(headersDec[y], newHeaderType);
                }
            }
        }
        else{
            newParam->addChild(params[0]->children[2]->Clone());
            for(int x = 0; x < (int) TempLocalElementLists.size(); x++){
                if(TempLocalElementLists[x] != NULL)
                    TempLocalElementLists[x]->SubstituteIdentifierWithType(params[x]->value, newParam->value, params[x]->children[2]->value);
                if(TempApplyLists[x] != NULL)
                    TempApplyLists[x]->SubstituteIdentifierWithType(params[x]->value, newParam->value, params[x]->children[2]->value);
            }
        }
    }

    //Merge local elements

    std::unordered_map<AST*, int> elementsToProgramsMap; //To avoid unifying elements from the same program

    std::map<AST*, int> tablesToPrograms; //To pass to TablesMerge

    AST *LocalElementList = NULL;
    for(int cn = 0; cn < (int) controls.size(); cn++){
        if(TempLocalElementLists[cn] != NULL){
            if(LocalElementList == NULL){
                LocalElementList = TempLocalElementLists[cn]->Clone();
                for(AST *LocalElement : LocalElementList->children){
                    elementsToProgramsMap.emplace(std::make_pair(LocalElement, cn));

                    sprintf(buffer, "Ps%d_%s", cn + 1, LocalElement->str());
                    std::string newName = std::string(buffer);

                    if(LocalElement->nType == NT_TABLE_DECLARATION){
                        CFG::AddProgramTable(cn, LocalElement, newName, controls[cn]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value, newControlName);
                        tablesToPrograms.emplace(LocalElement, cn);
                    }
                    else if(LocalElement->nType == NT_ACTION_DECLARATION){
                        CFG::AddProgramAction(cn, LocalElement, LocalElement->value, newName, controls[cn]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value, newControlName);
                    }

                    std::string oldID = LocalElement->value;
                    LocalElementList->SubstituteIdentifierWithType(oldID, newName, LocalElement->programType);
                    if(TempApplyLists[cn] != NULL)
                        TempApplyLists[cn]->SubstituteIdentifierWithType(oldID, newName, LocalElement->programType);

                    LogM = "Added local element \"" + oldID + "\" from program \"" + programs[cn]->value + "\" as \"" + newName + "\""; 
                    Log::MSG(LogM, LOG_LEVEL_ALL);

                    LocalElement->value = newName;
                }
            }
            else{
                std::unordered_set<AST*> Matched;
                for(AST *localElement : TempLocalElementLists[cn]->children){
                    bool matchedD;
                    std::string newName;
                    switch(localElement->nType){
                        case NT_VARIABLE_DECLARATION:
                        case NT_INSTANTIATION:
                        case NT_CONSTANT_DECLARATION:
                        case NT_ACTION_DECLARATION:
                            matchedD = false;
                            for(AST *LocalElement : LocalElementList->children){
                                if(elementsToProgramsMap.find(LocalElement)->second != cn && !matchedD && Matched.find(LocalElement) == Matched.end() && AST::Equals(localElement, LocalElement)){
                                    elementsToProgramsMap.find(LocalElement)->second = cn;

                                    const std::string elementNameInProg = localElement->value;
                                    
                                    LogM = "Found equal element of \"" + elementNameInProg + "\" from \"" + programs[cn]->value + "\" (" + LocalElement->value + "). Merged into \"";

                                    Matched.emplace(LocalElement);
                                    newName = LocalElement->value;
                                    sprintf(buffer, "s%d", cn+1);
                                    newName = newName.substr(0, newName.find("_")) + std::string(buffer) + newName.substr(newName.find("_"));
                                    
                                    for(int cn2 = 0; cn2 <= cn; cn2++){
                                        if(TempLocalElementLists[cn2] != NULL){
                                            TempLocalElementLists[cn2]->SubstituteIdentifierWithType(LocalElement->value, newName, LocalElement->programType);  
                                        }
                                        if(TempApplyLists[cn2] != NULL){
                                            TempApplyLists[cn2]->SubstituteIdentifierWithType(LocalElement->value, newName, LocalElement->programType);
                                        }
                                    }

                                    TempLocalElementLists[cn]->SubstituteIdentifierWithType(elementNameInProg, newName, localElement->programType);
                                    if(TempApplyLists[cn] != NULL)
                                        TempApplyLists[cn]->SubstituteIdentifierWithType(elementNameInProg, newName, localElement->programType);

                                    if(localElement->nType == NT_ACTION_DECLARATION){
                                        CFG::ChangeVActionName(newControlName, LocalElement->value, newName);
                                        CFG::AddProgramAction(cn, localElement, elementNameInProg, newName, controls[cn]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value, newControlName);
                                    }

                                    LocalElementList->SubstituteIdentifierWithType(LocalElement->value, newName, LocalElement->programType);
                                    
                                    LocalElement->value = newName;
                                    matchedD = true;
                                
                                    LogM = LogM + newName + "\"";
                                    Log::MSG(LogM, LOG_LEVEL_ALL);
                                }
                            }
                            if(!matchedD){
                                AST *newElement = localElement->Clone();
                                sprintf(buffer, "Ps%d_%s", cn + 1, localElement->str());
                                newName = std::string(buffer);

                                std::string oldID = localElement->value;
                                TempLocalElementLists[cn]->SubstituteIdentifierWithType(oldID, newName, localElement->programType);
                                if(TempApplyLists[cn] != NULL)
                                    TempApplyLists[cn]->SubstituteIdentifierWithType(oldID, newName, localElement->programType);
                                newElement->value = newName;
                                LocalElementList->addChild(newElement);
                                elementsToProgramsMap.emplace(std::make_pair(newElement, cn));

                                if(localElement->nType == NT_ACTION_DECLARATION)
                                    CFG::AddProgramAction(cn, localElement, oldID, newName, controls[cn]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value, newControlName);

                                LogM = "Added local element \"" + oldID + "\" from program \"" + programs[cn]->value + "\" as \"" + newName + "\""; 
                                Log::MSG(LogM, LOG_LEVEL_ALL);
                            }
                            break;
                        case NT_TABLE_DECLARATION:
                            {
                            matchedD = false;
                            const int LELSize = (int) LocalElementList->children.size();
                            for(int lelp = 0; lelp < LELSize; lelp++){
                                AST *LocalElement = LocalElementList->children[lelp];
                                if(elementsToProgramsMap.find(LocalElement)->second != cn && !matchedD && Matched.find(LocalElement) == Matched.end()){
                                    AST* mergedTable = AST::TablesMerge(LocalElement, localElement, tablesToPrograms.find(LocalElement)->second + 1, cn + 1);
                                    if(mergedTable != NULL){
                                        elementsToProgramsMap.find(LocalElement)->second = cn;

                                        matchedD = true;
                                        Matched.emplace(LocalElement);
                                        newName = LocalElement->value;
                                        const std::string tableNameInProg = localElement->value;
                                        LogM = "Found compatible table of \"" + tableNameInProg + "\" from \"" + programs[cn]->value + "\" (" + LocalElement->value + "). Merged into \"";

                                        sprintf(buffer, "s%d", cn+1);
                                        newName = newName.substr(0, newName.find("_")) + std::string(buffer) + newName.substr(newName.find("_"));
                                        mergedTable->value = newName;
                                        LocalElementList->addChild(mergedTable);
                                        elementsToProgramsMap.emplace(std::make_pair(mergedTable, cn));

                                        CFG::ChangeVTableName(newControlName, LocalElement->value, mergedTable->value);
                                        CFG::AddProgramTable(cn, localElement, mergedTable->value, controls[cn]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value, newControlName);

                                        for(int cn2 = 0; cn2 < cn; cn2++){
                                            if(TempLocalElementLists[cn2] != NULL)
                                                TempLocalElementLists[cn2]->SubstituteIdentifierWithType(LocalElement->value, mergedTable->value, LocalElement->programType);
                                            if(TempApplyLists[cn2] != NULL)
                                                TempApplyLists[cn2]->SubstituteIdentifierWithType(LocalElement->value, mergedTable->value, LocalElement->programType);
                                        }
                                        if(TempLocalElementLists[cn] != NULL)
                                            TempLocalElementLists[cn]->SubstituteIdentifierWithType(tableNameInProg, mergedTable->value, localElement->programType);
                                        if(TempApplyLists[cn] != NULL)
                                            TempApplyLists[cn]->SubstituteIdentifierWithType(tableNameInProg, mergedTable->value, localElement->programType);

                                        LogM = LogM + newName + "\"";
                                        Log::MSG(LogM, LOG_LEVEL_ALL);
                                    }
                                }
                            }
                            if(!matchedD){
                                AST *newElement = localElement->Clone();
                                sprintf(buffer, "Ps%d_%s", cn + 1, localElement->str());
                                newName = std::string(buffer);
                                std::string oldID = localElement->value;
                                TempLocalElementLists[cn]->SubstituteIdentifierWithType(oldID, newName, localElement->programType);
                                if(TempApplyLists[cn] != NULL)
                                    TempApplyLists[cn]->SubstituteIdentifierWithType(oldID, newName, localElement->programType);
                                newElement->value = newName;
                                LocalElementList->addChild(newElement);
                                elementsToProgramsMap.emplace(std::make_pair(newElement, cn));

                                LogM = "Added local element \"" + oldID + "\" from program \"" + programs[cn]->value + "\" as \"" + newName + "\""; 
                                Log::MSG(LogM, LOG_LEVEL_ALL);

                                tablesToPrograms.emplace(newElement, cn);
                                CFG::AddProgramTable(cn, localElement, newName, controls[cn]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value, newControlName);
                            }
                            }
                            break;
                        default:
                            Assert(false);
                            break;
                    }
                }
                for(AST *table : Matched){
                    if(table->nType == NT_TABLE_DECLARATION){
                        LocalElementList->RemoveChild(table);
                        delete table;
                    }
                }
            }
        }
    }
    newControl->addChild(LocalElementList);

    //Apply
    if(newControl->getChildNT(NT_CONTROL_LOCAL_DEC_LIST) != NULL){
        AST* newApply = new AST("", NT_STAT_OR_DEC_LIST);
        newControl->addChild(newApply);

        AST* templateSwitchS = new AST("", NT_SWITCH_STATEMENT);
        templateSwitchS->addChild(new AST(TEMP_PROGRAM_CODE, NT_EXPRESSION));
        templateSwitchS->addChild(new AST("", NT_SWITCH_CASE_LIST));
        for(int p = 0; p < (int) programs.size(); p++){
            AST* newSC = new AST("", NT_SWITCH_CASE);
            templateSwitchS->getChildNT(NT_SWITCH_CASE_LIST)->addChild(newSC);
            sprintf(buffer, "%d", p + 1);
            newSC->addChild(new AST(std::string(buffer), NT_EXPRESSION));
            newSC->addChild(new AST("", NT_STAT_OR_DEC_LIST));
        }

        //Order shared tables
        //TODO: Delete dependencyGraph
        //AST *dependencyGraph = AST::GetSharedTablesDependencyGraph(TempApplyLists[0]);
        //std::vector<std::string> sharedTableNames = GetDependencyGraphOrder(dependencyGraph);
        
        std::vector<std::string> sharedTableNames = AST::GetTableOrderFromLinearize(TempApplyLists[0]);

        /* 
        //This was only looking at the local declarations
        std::vector<std::string> sharedTableNames;
        for(AST* le : newControl->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->children){
            if(le->nType == NT_TABLE_DECLARATION && findPrefix(le->value) > 4){
                sharedTableNames.push_back(le->value);
            }
        }
        */

        std::vector<std::vector<AST*>> splitedApply(programs.size());
        int maxIfDepth = 0;
        for(int p = 0; p < (int) programs.size(); p++){
            if(TempApplyLists[p] != NULL){
                splitedApply[p] = SplitApply(TempApplyLists[p], sharedTableNames, &maxIfDepth);
                Assert(splitedApply[p].size() == sharedTableNames.size() + 1);
            }
            else{
                splitedApply[p].resize(sharedTableNames.size() + 1, NULL);
            }
        }

        for(int i = 0; i < maxIfDepth; i++){
            sprintf(buffer, "VirtIF_%d", i + 1);
            AST *ifVarDec = new AST(std::string(buffer), NT_VARIABLE_DECLARATION);
            ifVarDec->addChild(NULL); //annotations
            ifVarDec->addChild(new AST("bool", NT_BASE_TYPE));
            ifVarDec->addChild(new AST("false", NT_EXPRESSION));
            newApply->addChild(ifVarDec);
        }
        for(int i = 0; i < (int) sharedTableNames.size(); i++){
            sprintf(buffer, "VirtApplyT%d", i + 1);
            AST *atVarDec = new AST(std::string(buffer), NT_VARIABLE_DECLARATION);
            atVarDec->addChild(NULL); //annotations
            atVarDec->addChild(new AST("bool", NT_BASE_TYPE));
            atVarDec->addChild(new AST("false", NT_EXPRESSION));
            newApply->addChild(atVarDec);
        }

        for(int i = 0; i < (int) sharedTableNames.size() + 1; i++){
            AST* newSS = templateSwitchS->Clone();
            size_t equalCount = 0;

            for(int p = 0; p < (int) programs.size(); p++){
                if(splitedApply[p][i] != NULL){
                    splitedApply[p][i]->RemoveEmptyIfElses();

                    for(int p2 = 0; p2 < p; p2++){
                        if(AST::Equals(splitedApply[p][i], splitedApply[p2][i])){
                            equalCount++;

                            AST *aux = newSS->getChildNT(NT_SWITCH_CASE_LIST)->children[p2];
                            for(int auxP = p2 + 1; auxP < p; auxP++){
                                newSS->getChildNT(NT_SWITCH_CASE_LIST)->children[auxP - 1] = newSS->getChildNT(NT_SWITCH_CASE_LIST)->children[auxP];
                            }
                            newSS->getChildNT(NT_SWITCH_CASE_LIST)->children[p-1] = aux;

                            delete newSS->getChildNT(NT_SWITCH_CASE_LIST)->children[p-1]->children[1];
                            newSS->getChildNT(NT_SWITCH_CASE_LIST)->children[p-1]->children.erase(newSS->getChildNT(NT_SWITCH_CASE_LIST)->children[p-1]->children.begin() + 1);
                        }
                    }

                    delete newSS->getChildNT(NT_SWITCH_CASE_LIST)->children[p]->children[1];
                    newSS->getChildNT(NT_SWITCH_CASE_LIST)->children[p]->children[1] = splitedApply[p][i];
                }
            }

            //All are equal
            if(equalCount == programs.size() - 1){
                for (AST *statement : newSS->getChildNT(NT_SWITCH_CASE_LIST)->children[programs.size() - 1]->children[1]->children){
                    newApply->addChild(statement->Clone());
                }
                delete newSS;
            }
            else {
                newApply->addChild(newSS);
            }
            if(i < (int) sharedTableNames.size()){
                sprintf(buffer, "VirtApplyT%d", i + 1);

                bool alwaysApplied = false;
                for(AST *statement : newApply->children){
                    if(statement->nType == NT_ASSIGN_STATEMENT && statement->children[0]->value == std::string(buffer) && statement->children[1]->value == "true"){
                        alwaysApplied = true;
                    }
                }

                AST *applyCall = new AST("", NT_METHOD_CALL_STATEMENT);
                applyCall->addChild(new AST(".", NT_NONE));
                applyCall->children[0]->addChild(new AST(sharedTableNames[i], NT_NONE));
                applyCall->children[0]->addChild(new AST("apply", NT_NONE));
                applyCall->addChild(NULL); //typeArgumentList
                applyCall->addChild(NULL); //arguments

                if(!alwaysApplied){
                    AST *tableApplyCondition = new AST("IF", NT_CONDITIONAL_STATEMENT);
                    tableApplyCondition->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                    tableApplyCondition->addChild(new AST("", NT_STAT_OR_DEC_LIST));
                    tableApplyCondition->addChild(NULL); //else
                    tableApplyCondition->children[1]->addChild(applyCall);
                    newApply->addChild(tableApplyCondition);
                }
                else{
                    size_t si = 0;
                    while(si < newApply->children.size()){
                        AST *statement = newApply->children[si];
                        if((statement->nType == NT_VARIABLE_DECLARATION && statement->value == std::string(buffer)) || (statement->nType == NT_ASSIGN_STATEMENT && statement->children[0]->value == std::string(buffer))){
                            delete newApply->children[si];
                            newApply->children.erase(newApply->children.begin() + si);
                            si--;
                        }
                        si++;
                    }
                    newApply->addChild(applyCall);
                }
            }
        }
        delete templateSwitchS;
    }
    else{
        std::set<int> VerifiedApplies;
        std::vector<std::vector<int>> EqualApplies;
        for(int i = 0; i < (int) programs.size(); i++){
            if(VerifiedApplies.find(i) == VerifiedApplies.end()){
                VerifiedApplies.emplace(i);
                EqualApplies.push_back({i});
                for(int j = i + 1; j < (int) programs.size(); j++){
                    if(AST::Equals(TempApplyLists[i], TempApplyLists[j])){
                        VerifiedApplies.emplace(j);
                        EqualApplies[EqualApplies.size() - 1].push_back(j);
                    }
                }
            }
        }

        if(EqualApplies.size() == 1){
            newControl->addChild(NULL_CLONE(TempApplyLists[0]));
        }
        else{
            AST *newApply = new AST("", NT_STAT_OR_DEC_LIST);
            AST *branchProgram = new AST("", NT_SWITCH_STATEMENT);
            branchProgram->addChild(new AST(TEMP_PROGRAM_CODE, NT_EXPRESSION));
            branchProgram->addChild(new AST("", NT_SWITCH_CASE_LIST));
            for(const std::vector<int>& ea : EqualApplies){
                for(int x = 0; x < (int) ea.size(); x++){
                    sprintf(buffer, "%d", ea[x] + 1);
                    AST *newSC = new AST("", NT_SWITCH_CASE);
                    newSC->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                    if(x == (int) ea.size() - 1)
                        newSC->addChild((TempApplyLists[ea[x]] == NULL ? new AST("", NT_STAT_OR_DEC_LIST) : TempApplyLists[ea[x]]->Clone()));
                    branchProgram->children[1]->addChild(newSC);
                }
            }
            newApply->addChild(branchProgram);
            newControl->addChild(newApply);
        }
    }

    //Remove unused VirtIFs
    //if(newControl->getChildNT(NT_STAT_OR_DEC_LIST) != NULL)
    //   ASTApplyBlockOptimizer::RemoveUnusedVariables(newControl->getChildNT(NT_STAT_OR_DEC_LIST));

    ASTApplyBlockOptimizer::Optimize(newControl->getChildNT(NT_STAT_OR_DEC_LIST));

    newNodes.push_back(newControl);
    for(AST *TempLEL : TempLocalElementLists)
        NullDelete(TempLEL);
    for(AST *TempAL : TempApplyLists)
        NullDelete(TempAL);
    Log::Pop();

    return newNodes;
}

void AST::UnifyErrorMKDeclarations(){
    std::set<std::string> errorIdentifiers;
    std::set<std::string> mkIdentifiers;
    
    int i = 0;
    while(i < (int) this->children.size()){
        AST *c = this->children[i];
        if(c->nType == NT_ERROR_DECLARATION || c->nType == NT_MATCH_KIND_DECLARATION){
            if(c->children[0] != NULL){
                for(AST *identifier : c->children[0]->children){
                    if(c->nType == NT_ERROR_DECLARATION){
                        if(errorIdentifiers.find(identifier->value) == errorIdentifiers.end())
                            errorIdentifiers.emplace(identifier->value);
                    }
                    else{
                        if(mkIdentifiers.find(identifier->value) == mkIdentifiers.end())
                            mkIdentifiers.emplace(identifier->value);
                    }
                }
            }
            delete this->children[i];
            this->children.erase(this->children.begin() + i);
            i--;
        }
        i++;
    }

    if(!errorIdentifiers.empty()){
        AST *errorDec = new AST("", NT_ERROR_DECLARATION);
        AST *errIdentifierList = new AST("", NT_IDENTIFIER_LIST);
        errorDec->addChild(errIdentifierList);
        for(const std::string& s : errorIdentifiers)
            errIdentifierList->addChild(new AST(s, NT_NONE));
        this->addChildStart(errorDec);
    }
    
    if(!mkIdentifiers.empty()){
        AST *mkDec = new AST("", NT_MATCH_KIND_DECLARATION);
        AST *mkIdentifierList = new AST("", NT_IDENTIFIER_LIST);
        mkDec->addChild(mkIdentifierList);

        for(const std::string& s : mkIdentifiers)
            mkIdentifierList->addChild(new AST(s, NT_NONE));
        this->addChildStart(mkDec);
    }
}

bool AST::TablesMergeable(AST *t1, AST *t2){
    if(t1 == NULL || t2 == NULL || t1->nType != NT_TABLE_DECLARATION || t2->nType != NT_TABLE_DECLARATION || t1->getChildNT(NT_TABLE_PROPERTY_LIST) == NULL || t2->getChildNT(NT_TABLE_PROPERTY_LIST) == NULL)
        return false;

    //Check is both tables are "VirtShared" compatible
    std::string vsharedName = "";
    if(t1->getChildNT(NT_ANNOTATION_LIST) != NULL){
        int i = 0;
        while(i < (int) t1->getChildNT(NT_ANNOTATION_LIST)->children.size() && t1->getChildNT(NT_ANNOTATION_LIST)->children[i]->value != VIRT_SHARED_ANNOTATION)
            i++;
        if(i < (int) t1->getChildNT(NT_ANNOTATION_LIST)->children.size()){
            AST *ATL = t1->getChildNT(NT_ANNOTATION_LIST)->children[i]->getChildNT(NT_ANNOTATION_TOKEN_LIST);
            if(ATL == NULL || ATL->children.size() < 0)
                return false;
            vsharedName = ATL->children[0]->value;
        }
    }
    if(t2->getChildNT(NT_ANNOTATION_LIST) != NULL){
        int i = 0;
        while(i < (int) t2->getChildNT(NT_ANNOTATION_LIST)->children.size() && t2->getChildNT(NT_ANNOTATION_LIST)->children[i]->value != VIRT_SHARED_ANNOTATION)
            i++;
        if(i < (int) t2->getChildNT(NT_ANNOTATION_LIST)->children.size()){
            AST *ATL = t2->getChildNT(NT_ANNOTATION_LIST)->children[i]->getChildNT(NT_ANNOTATION_TOKEN_LIST);
            if(ATL == NULL || ATL->children.size() < 0)
                return false;
            if(vsharedName != ATL->children[0]->value)
                return false;
        }
    }
    else if(vsharedName != ""){
        return false;
    }

    //Check if "const default_action" is the same
    std::string constDefaultAction1 = "";
    for(AST *c : t1->getChildNT(NT_TABLE_PROPERTY_LIST)->children)
        if(c != NULL && c->value == "const default_action")
            constDefaultAction1 = c->children[0]->value;
    if(constDefaultAction1 == "")
        return false;
    std::string constDefaultAction2 = "";
    for(AST *c : t2->getChildNT(NT_TABLE_PROPERTY_LIST)->children)
        if(c != NULL && c->value == "const default_action")
            constDefaultAction2 = c->children[0]->value;
    if(constDefaultAction1 != constDefaultAction2)
        return false;

    //Check if both tables define a size (Should always be the case...)
    bool hasSize = false;
    for(AST *c : t1->getChildNT(NT_TABLE_PROPERTY_LIST)->children)
        if(c != NULL && c->value == "size")
            hasSize = true;
    Assert(hasSize);
    hasSize = false;
    for(AST *c : t2->getChildNT(NT_TABLE_PROPERTY_LIST)->children)
        if(c != NULL && c->value == "size")
            hasSize = true;
    Assert(hasSize);

    //Check if keys are the same
    AST *keyProperty1 = t1->getChildNT(NT_TABLE_PROPERTY_LIST)->FindChildrenByValue("key", {NT_TABLE_PROPERTY});
    AST *keyProperty2 = t2->getChildNT(NT_TABLE_PROPERTY_LIST)->FindChildrenByValue("key", {NT_TABLE_PROPERTY});
    Assert(keyProperty1 != NULL && keyProperty2 != NULL);
    Assert(keyProperty1->children[0] != NULL && keyProperty2->children[0] != NULL);

    std::unordered_set<AST*> matchedT1Keys;
    for(AST *keyElement : keyProperty1->children[0]->children)
        if(keyElement->children[0]->value == TEMP_PROGRAM_CODE)
            matchedT1Keys.emplace(keyElement);
    for(AST *keyElement : keyProperty2->children[0]->children){
        if(keyElement->children[0]->value != TEMP_PROGRAM_CODE){
            bool matched = false;
            for(AST* t1KeyElement : keyProperty1->children[0]->children){
                if(!matched && matchedT1Keys.find(t1KeyElement) == matchedT1Keys.end() && AST::Equals(keyElement, t1KeyElement)){
                    matchedT1Keys.emplace(t1KeyElement);
                    matched = true;
                }
            }
            if(!matched)
                return false;
        }
    }
    for(AST *keyElement : keyProperty1->children[0]->children)
        if(matchedT1Keys.find(keyElement) == matchedT1Keys.end())
            return false;

    //Check if both tables have constant entries or none has constant entries
    bool t1ConstEntries = t1->getChildNT(NT_TABLE_PROPERTY_LIST)->FindChildrenByValue("const entries", {NT_TABLE_PROPERTY}) != NULL;
    bool t2ConstEntries = t2->getChildNT(NT_TABLE_PROPERTY_LIST)->FindChildrenByValue("const entries", {NT_TABLE_PROPERTY}) != NULL;
    if(t1ConstEntries != t2ConstEntries)
        return false;

    return true;
}

AST* AST::TablesMerge(AST *t1, AST *t2, int P1, int P2){
    if(!AST::TablesMergeable(t1, t2)){
        return NULL;
    }

    AST *newTable = t1->Clone();

    bool isVirtShared = false;
    AST* AnnotationList = newTable->getChildNT(NT_ANNOTATION_LIST);
    if(AnnotationList != NULL){
        int i = 0;
        while(i < (int) AnnotationList->children.size() && newTable->getChildNT(NT_ANNOTATION_LIST)->children[i]->value != VIRT_SHARED_ANNOTATION)
            i++;
        if(i < (int) AnnotationList->children.size()){
            isVirtShared = true;
        }
    }

    if(!isVirtShared){
        AST *keyProperty = newTable->getChildNT(NT_TABLE_PROPERTY_LIST)->FindChildrenByValue("key", {NT_TABLE_PROPERTY});
        Assert(keyProperty != NULL && keyProperty->children[0] != NULL);

        bool Table1HasProgramCode = false;
        if(keyProperty->children[0]->children[0]->children[0]->value != TEMP_PROGRAM_CODE){
            AST *newKP = new AST("exact", NT_KEY_ELEMENT);
            newKP->addChild(new AST(TEMP_PROGRAM_CODE, NT_EXPRESSION));
            newKP->addChild(NULL); //optAnnotations
            keyProperty->children[0]->addChildStart(newKP);
        }
        else{
            Table1HasProgramCode = true;
        }

        //Merge actions
        AST *actionProperty = newTable->getChildNT(NT_TABLE_PROPERTY_LIST)->FindChildrenByValue("actions", {NT_TABLE_PROPERTY});
        AST *actionPropertyT2 = t2->getChildNT(NT_TABLE_PROPERTY_LIST)->FindChildrenByValue("actions", {NT_TABLE_PROPERTY});
        Assert(actionProperty != NULL && actionPropertyT2 != NULL);
        Assert(actionProperty->getChildNT(NT_ACTION_LIST) != NULL && actionPropertyT2->getChildNT(NT_ACTION_LIST) != NULL);
        
        std::unordered_set<std::string> actionNames;
        for(AST *actionR : actionProperty->getChildNT(NT_ACTION_LIST)->children)
            if(actionR != NULL && actionR->nType == NT_ACTION_REF)
                actionNames.emplace(actionR->value);
        for(AST *actionR : actionPropertyT2->getChildNT(NT_ACTION_LIST)->children)
            if(actionR != NULL && actionR->nType == NT_ACTION_REF)
                if(actionNames.find(actionR->value) == actionNames.end())
                    actionProperty->getChildNT(NT_ACTION_LIST)->addChild(actionR->Clone());

        //Merge size
        AST *sizeProperty = newTable->getChildNT(NT_TABLE_PROPERTY_LIST)->FindChildrenByValue("size", {NT_TABLE_PROPERTY});
        AST *sizePropertyT2 = t2->getChildNT(NT_TABLE_PROPERTY_LIST)->FindChildrenByValue("size", {NT_TABLE_PROPERTY});
        Assert(sizeProperty->children[0]->children.size() == 0 && sizePropertyT2->children[0]->children.size() == 0);
        const int size1 = IPLBase::intFromStr(sizeProperty->children[0]->str());
        const int size2 = IPLBase::intFromStr(sizePropertyT2->children[0]->str());
        Assert(size1 > 0 && size2 > 0);

        char buffer[128];
        sprintf(buffer, "%d", size1 + size2);
        sizeProperty->children[0]->value = std::string(buffer);

        //Merge ConstantEntries
        AST *constEntries = newTable->getChildNT(NT_TABLE_PROPERTY_LIST)->FindChildrenByValue("const entries", {NT_TABLE_PROPERTY});
        AST *constEntriesT2 = t2->getChildNT(NT_TABLE_PROPERTY_LIST)->FindChildrenByValue("const entries", {NT_TABLE_PROPERTY});
        Assert((constEntries == NULL && constEntriesT2 == NULL) || (constEntries != NULL && constEntriesT2 != NULL));
        if(constEntriesT2 != NULL){
            if(!Table1HasProgramCode){
                sprintf(buffer, "%d", P1);
                for(AST *entry : constEntries->getChildNT(NT_ENTRIES_LIST)->children){
                    if(entry->children[0]->nType != NT_TUPLE_KEYSET_EXPRESSION){
                        AST *newEntryKey = new AST("", NT_TUPLE_KEYSET_EXPRESSION);
                        newEntryKey->addChild(new AST("", NT_SIMPLE_EXPRESSION_LIST));
                        newEntryKey->children[0]->addChild(new AST(buffer, NT_EXPRESSION));
                        newEntryKey->children[0]->addChild(entry->children[0]);
                        entry->children[0] = newEntryKey;
                    }
                    else{
                        AST *SEL = entry->children[0]->getChildNT(NT_SIMPLE_EXPRESSION_LIST);
                        Assert(SEL != NULL);
                        SEL->addChildStart(new AST(buffer, NT_EXPRESSION));
                    }
                }
            }
            sprintf(buffer, "%d", P2);
            for(AST *entry : constEntriesT2->getChildNT(NT_ENTRIES_LIST)->children){
                AST *clonedEntry = entry->Clone();
                if(clonedEntry->children[0]->nType != NT_TUPLE_KEYSET_EXPRESSION){
                    AST *newEntryKey = new AST("", NT_TUPLE_KEYSET_EXPRESSION);
                    newEntryKey->addChild(new AST("", NT_SIMPLE_EXPRESSION_LIST));
                    newEntryKey->children[0]->addChild(new AST(buffer, NT_EXPRESSION));
                    newEntryKey->children[0]->addChild(clonedEntry->children[0]);
                    clonedEntry->children[0] = newEntryKey;
                }
                else{
                    AST *SEL = clonedEntry->children[0]->getChildNT(NT_SIMPLE_EXPRESSION_LIST);
                    Assert(SEL != NULL);
                    SEL->addChildStart(new AST(buffer, NT_EXPRESSION));
                }
                constEntries->getChildNT(NT_ENTRIES_LIST)->addChild(clonedEntry);
            }
        }
    }

    return newTable;
}

bool AST::EqualActionDeclarations(AST* a1, AST* a2){
    if(!Equals(a1->children[0], a2->children[0]) || !Equals(a1->children[1], a2->children[1]))
        return false;
    
    AST *tempA2 = a2->Clone();

    if(a1->getChildNT(NT_PARAMETER_LIST) != NULL && a2->getChildNT(NT_STAT_OR_DEC_LIST) != NULL){
        for(int i = 0; i < (int) a1->getChildNT(NT_PARAMETER_LIST)->children.size(); i++){
            tempA2->getChildNT(NT_STAT_OR_DEC_LIST)->SubstituteValue(tempA2->getChildNT(NT_PARAMETER_LIST)->children[i]->value, a1->getChildNT(NT_PARAMETER_LIST)->children[i]->value);
            tempA2->getChildNT(NT_PARAMETER_LIST)->children[i]->value = a1->getChildNT(NT_PARAMETER_LIST)->children[i]->value;
        }
    }

    bool result = AST::Equals(a1->getChildNT(NT_STAT_OR_DEC_LIST), tempA2->getChildNT(NT_STAT_OR_DEC_LIST));
    delete tempA2;
    return result;
}

bool AST::Equals(AST *a1, AST *a2){
    const static std::unordered_set<std::string> HeaderEqualsBlacklist {"ingress_intrinsic_metadata_for_tm_t", "ingress_intrinsic_metadata_from_parser_t", 
                                                                        "egress_intrinsic_metadata_from_parser_t"};

    if(a1 == NULL || a2 == NULL)
        return (a1 == NULL && a2 == NULL);
    if(a1->nType != a2->nType || a1->children.size() != a2->children.size())
        return false;
    switch(a1->nType){
        case NT_STRUCT_TYPE_DECLARATION:
        case NT_HEADER_TYPE_DECLARATION:
            for(const std::string& headerName : HeaderEqualsBlacklist){
                if(a1->value.find(headerName) != std::string::npos && a2->value.find(headerName) == std::string::npos)
                    return false;
            }
            if(a1->getChildNT(NT_STRUCT_FIELD_LIST) == NULL || a2->getChildNT(NT_STRUCT_FIELD_LIST) == NULL)
                return false;
            for(int i = 0; i < (int) a1->children.size(); i++)
                if(!Equals(a1->children[i], a2->children[i]))
                    return false;
            return true;
            break;
        case NT_STRUCT_FIELD:
            for(int i = 0; i < (int) a1->children.size(); i++)
                if(!Equals(a1->children[i], a2->children[i]))
                    return false;
            if(hasVirtPreventMergeAnnotation(a1) || hasVirtPreventMergeAnnotation(a2))
                return a1->value == a2->value;
            return true;
            break;
        //case NT_EXTERN_DECLARATION:
        case NT_STRUCT_FIELD_LIST:
        case NT_METHOD_PROTOTYPE_LIST:
        case NT_METHOD_PROTOTYPE:
        case NT_PARAMETER_LIST:
        case NT_TYPE_PARAMETER_LIST:
        case NT_CONSTRUCTOR_METHOD_PROTOTYPE:
        case NT_PARSER_STATEMENT_LIST:
        case NT_METHOD_CALL_STATEMENT:
        case NT_PARSER_STATE:
        case NT_INSTANTIATION:
        case NT_VARIABLE_DECLARATION:
        case NT_PARAMETER:
        case NT_CONTROL_TYPE_DECLARATION:
            for(int i = 0; i < (int) a1->children.size(); i++)
                if(!Equals(a1->children[i], a2->children[i]))
                    return false;
            return true;
            break;
        case NT_ACTION_DECLARATION:
            return AST::EqualActionDeclarations(a1, a2);
            break;
        /*
        case NT_FUNCTION_PROTOTYPE:
        case NT_BASE_TYPE:
        case NT_PARAMETER:
        */
        case NT_TYPE_NAME:
        case NT_LITERAL:
        case NT_DIRECTION:
        case NT_TYPE_OR_VOID:
        case NT_TYPE_PARAMETER:
            return a1->value == a2->value;
            break;
        default:
            if(a1->value != a2->value)
                return false;
            for(int i = 0; i < (int) a1->children.size(); i++)
                if(!Equals(a1->children[i], a2->children[i]))
                    return false;
            return true;
            break;
        
    }
    return false;
}

bool AST::EqualsParserDeclaration(AST *a1, AST *a2, AST *p1, AST *p2, const std::vector<AST*>& headersDec){

    //Parser statement lists must be equal
    if(!AST::Equals(p1->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST), p1->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)))
        return false;

    std::vector<AST*> newNodes = MergeParsers({a1, a2}, {p1, p2}, headersDec, "TempName", {std::set<int>({0}), std::set<int>({1})}, NULL);
    if(newNodes.size() == 0)
        return false;

    bool result = p1->children[3]->children.size() == newNodes[newNodes.size() - 1]->children[3]->children.size();

    for(AST *node : newNodes)
        delete node;

    return result;
}

bool AST::EqualsControlDeclaration(AST *a1, AST *a2, AST *c1, AST *c2, const std::vector<AST*>& headersDec){
    //TODO: Better?
    return AST::Equals(c1, c2);
}

bool AST::hasVirtPreventMergeAnnotation(AST *ast){
    if(ast->getChildNT(NT_ANNOTATION_LIST) == NULL)
        return false;
    for(AST *annotation : ast->getChildNT(NT_ANNOTATION_LIST)->children){
        if(annotation->value == VIRT_PREVENT_MERGE_ANNOTATION)
            return true;
    }
    return false;
}

bool AST::hasFieldWithVirtPreventMergeAnnotation(AST *SFL){
    if(SFL == NULL)
        return false;
    Assert(SFL->nType == NT_STRUCT_FIELD_LIST);
    for(AST *field : SFL->children){
        if(AST::hasVirtPreventMergeAnnotation(field))
            return true;
    }
    return false;
}

bool AST::MakeHeaderEqual(AST *h1, AST *h2){
    char buffer[32];

    std::vector<std::tuple<int, int, std::string, bool>> h1Fields;
    std::vector<std::tuple<int, int, std::string, bool>> h2Fields;

    if(h1->nType != NT_HEADER_TYPE_DECLARATION || h2->nType != NT_HEADER_TYPE_DECLARATION)
        return false;

    AST *SFL1 = h1->getChildNT(NT_STRUCT_FIELD_LIST);
    AST *SFL2 = h2->getChildNT(NT_STRUCT_FIELD_LIST);

    if(SFL1 == NULL || SFL2 == NULL)
        return false;

    int counter = 0;
    for(size_t i = 0; i < SFL1->children.size(); i++){
        bool unused = false;
        if(SFL1->children[i]->getChildNT(NT_ANNOTATION_LIST) != NULL){
            for(AST *a : SFL1->children[i]->getChildNT(NT_ANNOTATION_LIST)->children){
                if(a->value == "unused"){
                    unused = true;
                }
            }
        }

        //Assert(SFL1->children[i]->getChildNT(NT_BASE_TYPE) != NULL);
        if(SFL1->children[i]->getChildNT(NT_BASE_TYPE) == NULL)
            return false;

        int bitSize = IPLBase::intFromStr(SFL1->children[i]->getChildNT(NT_BASE_TYPE)->children[0]->str());
        h1Fields.push_back(std::make_tuple(counter, counter + bitSize, SFL1->children[i]->value, unused));
        counter += bitSize;
    }
    counter = 0;
    for(size_t i = 0; i < SFL2->children.size(); i++){
        bool unused = false;
        if(SFL2->children[i]->getChildNT(NT_ANNOTATION_LIST) != NULL){
            for(AST *a : SFL2->children[i]->getChildNT(NT_ANNOTATION_LIST)->children){
                if(a->value == "unused"){
                    unused = true;
                }
            }
        }

        Assert(SFL2->children[i]->getChildNT(NT_BASE_TYPE) != NULL);
        int bitSize = IPLBase::intFromStr(SFL2->children[i]->getChildNT(NT_BASE_TYPE)->children[0]->str());
        h2Fields.push_back(std::make_tuple(counter, counter + bitSize, SFL2->children[i]->value, unused));
        counter += bitSize;
    }

    if(std::get<1>(h1Fields[h1Fields.size() - 1]) != std::get<1>(h2Fields[h2Fields.size() - 1]))
        return false;

    for(size_t i = 0; i < h1Fields.size(); i++){
        if(!std::get<3>(h1Fields[i])){
            int j = 0;
            while(j < (int) h2Fields.size() && (std::get<0>(h2Fields[j]) > std::get<0>(h1Fields[i]) || std::get<1>(h2Fields[j]) < std::get<1>(h1Fields[i])))
                j++;
            if(j == (int) h2Fields.size()){
                return false;
            }
            if(std::get<0>(h2Fields[j]) != std::get<0>(h1Fields[i]) || std::get<1>(h2Fields[j]) != std::get<1>(h1Fields[i])){
                if(!std::get<3>(h2Fields[j]))
                    return false;
                else{
                    std::tuple<int, int, std::string, bool> pre = std::make_tuple(std::get<0>(h2Fields[j]), std::get<0>(h1Fields[i]), std::get<2>(h2Fields[j]) + "_pre", true);
                    std::tuple<int, int, std::string, bool> post = std::make_tuple(std::get<1>(h1Fields[i]), std::get<1>(h2Fields[j]), std::get<2>(h2Fields[j]) + "_post", true);
                    h2Fields.erase(h2Fields.begin() + j);
                    h2Fields.insert(h2Fields.begin() + j, pre);
                    h2Fields.insert(h2Fields.begin() + j + 1, h1Fields[i]);
                    h2Fields.insert(h2Fields.begin() + j + 2, post);
                }
            }
            else if(std::get<3>(h2Fields[j])){
                h2Fields[j] = h1Fields[i];
            }
        }
    }
    for(size_t i = 0; i < h2Fields.size(); i++){
        if(!std::get<3>(h2Fields[i])){
            int j = 0;
            while(j < (int) h1Fields.size() && (std::get<0>(h1Fields[j]) > std::get<0>(h2Fields[i]) || std::get<1>(h1Fields[j]) < std::get<1>(h2Fields[i])))
                j++;
            if(j == (int) h1Fields.size()){
                return false;
            }
            if(std::get<0>(h1Fields[j]) != std::get<0>(h2Fields[i]) || std::get<1>(h1Fields[j]) != std::get<1>(h2Fields[i])){
                if(!std::get<3>(h1Fields[j]))
                    return false;
                else{
                    std::tuple<int, int, std::string, bool> pre = std::make_tuple(std::get<0>(h1Fields[j]), std::get<0>(h2Fields[i]), std::get<2>(h1Fields[j]) + "_pre", true);
                    std::tuple<int, int, std::string, bool> post = std::make_tuple(std::get<1>(h2Fields[i]), std::get<1>(h1Fields[j]), std::get<2>(h1Fields[j]) + "_post", true);
                    h1Fields.erase(h1Fields.begin() + j);
                    h1Fields.insert(h1Fields.begin() + j, pre);
                    h1Fields.insert(h1Fields.begin() + j + 1, h2Fields[i]);
                    h1Fields.insert(h1Fields.begin() + j + 2, post);
                }
            }
            else if(std::get<3>(h1Fields[j])){
                h1Fields[j] = h2Fields[i];
            }
        }
    }

    AST *newSFL1 = new AST("", NT_STRUCT_FIELD_LIST);
    AST *newSFL2 = new AST("", NT_STRUCT_FIELD_LIST);

    for(size_t i = 0; i < h1Fields.size(); i++){
        if(std::get<0>(h1Fields[i]) != std::get<1>(h1Fields[i])){
            AST *newSF = new AST(std::get<2>(h1Fields[i]), NT_STRUCT_FIELD);
            if(std::get<3>(h1Fields[i])){
                AST *newA = new AST("", NT_ANNOTATION_LIST);
                newA->addChild(new AST("unused", NT_ANNOTATION));
                newA->children[0]->addChild(NULL);
                newSF->addChild(newA);
            }
            else{
                newSF->addChild(NULL);
            }
            newSF->addChild(new AST("bit", NT_BASE_TYPE));
            sprintf(buffer, "%d", std::get<1>(h1Fields[i]) - std::get<0>(h1Fields[i]));
            newSF->children[1]->addChild(new AST(buffer, NT_LITERAL));
            newSFL1->addChild(newSF);
        }
    }

    for(size_t i = 0; i < h2Fields.size(); i++){
        if(std::get<0>(h2Fields[i]) != std::get<1>(h2Fields[i])){
            AST *newSF = new AST(std::get<2>(h2Fields[i]), NT_STRUCT_FIELD);
            if(std::get<3>(h2Fields[i])){
                AST *newA = new AST("", NT_ANNOTATION_LIST);
                newA->addChild(new AST("unused", NT_ANNOTATION));
                newA->children[0]->addChild(NULL);
                newSF->addChild(newA);
            }
            else{
                newSF->addChild(NULL);
            }
            newSF->addChild(new AST("bit", NT_BASE_TYPE));
            sprintf(buffer, "%d", std::get<1>(h2Fields[i]) - std::get<0>(h2Fields[i]));
            newSF->children[1]->addChild(new AST(buffer, NT_LITERAL));
            newSFL2->addChild(newSF);
        }
    }

    delete h1->children[2];
    delete h2->children[2];

    h1->children[2] = newSFL1;
    h2->children[2] = newSFL2;

    return true;
}

std::vector<AST*> AST::MergeUnifyHeaders(const std::vector<AST*>& programs){
    char buffer[256];
    std::vector<AST*> newNodes;
    std::vector<std::vector<AST*>> pHeaders(programs.size());
    std::vector<std::set<AST*>> deleteHeaders(programs.size());

    Log::MSG("Unifying equal Structs and Headers", LOG_LEVEL_SHORT);
    Log::Push();

    for(int i = 0; i < (int) programs.size(); i++){
        for(AST *c : programs[i]->children){
            if(c != NULL && (c->nType == NT_HEADER_TYPE_DECLARATION || c->nType == NT_STRUCT_TYPE_DECLARATION))
                pHeaders[i].push_back(c);
        }
    }

    std::unordered_map<AST *, std::set<int>> nodesToPrograms;

    std::map<AST*, std::vector<std::pair<AST*, int>>> newNodesToHeaders;
    std::vector<AST*> headersToSearch {programs};

    for(int i = 0; i < (int) programs.size(); i++){
        for(int j = 0; j < (int) pHeaders[i].size(); j++){
            bool repeated = false;
            for(AST *n : newNodes){
                auto it = nodesToPrograms.find(n);
                if(repeated || (it != nodesToPrograms.end() && it->second.find(i) != it->second.end()))
                    continue;

                if(!hasFieldWithVirtPreventMergeAnnotation(pHeaders[i][j]->children[2]) && !hasFieldWithVirtPreventMergeAnnotation(n->children[2]) && (Equals(pHeaders[i][j], n) || MakeHeaderEqual(pHeaders[i][j], n))){
                    sprintf(buffer, "s%d", i+1);
                    std::string newName = n->value.substr(0, n->value.find("_")) + std::string(buffer) + n->value.substr(n->value.find("_"));
                    
                    Log::MSG("Headers \"" + n->value + "\" and \"" + pHeaders[i][j]->value + "\" are equal. Renaming to \"" + newName + "\"", LOG_LEVEL_ALL);
                    
                    n->value = newName;
                    if(n->getChildNT(NT_STRUCT_FIELD_LIST) != NULL)
                        for(AST *slf : n->getChildNT(NT_STRUCT_FIELD_LIST)->children)
                            slf->value = slf->value.substr(0, slf->value.find("_")) + std::string(buffer) + slf->value.substr(slf->value.find("_"));

                    newNodesToHeaders.find(n)->second.push_back(std::make_pair(pHeaders[i][j], i));
                    for(auto h : newNodesToHeaders.find(n)->second){
                        programs[h.second]->SubstituteHeaderType(h.first, n, headersToSearch);
                        for(AST* n2 : newNodes)
                            n2->SubstituteHeaderType(h.first, n, {});
                        
                        h.first->value = n->value;
                        if(h.first->getChildNT(NT_STRUCT_FIELD_LIST) != NULL){
                            delete h.first->children[2];
                            h.first->children[2] = n->getChildNT(NT_STRUCT_FIELD_LIST)->Clone();
                        }
                    }

                    deleteHeaders[i].insert(pHeaders[i][j]);
                    repeated = true;
                    nodesToPrograms.find(n)->second.insert(i);
                }
            }
            if(!repeated){
                newNodes.push_back(pHeaders[i][j]->Clone());
                nodesToPrograms.emplace(std::make_pair(newNodes[newNodes.size() - 1], std::set<int>{i}));
                headersToSearch.push_back(newNodes[newNodes.size() - 1]);
                newNodesToHeaders.emplace(std::make_pair(newNodes[newNodes.size() - 1], std::vector<std::pair<AST*, int>>{{pHeaders[i][j], i}}));
                deleteHeaders[i].insert(pHeaders[i][j]);
            }
        }
    }

    for(int i = 0; i < (int) programs.size(); i++){
        int cc = 0;
        while(cc < (int) programs[i]->children.size()){
            if(deleteHeaders[i].find(programs[i]->children[cc]) != deleteHeaders[i].end()){
                delete programs[i]->children[cc];
                programs[i]->children.erase(programs[i]->children.begin() + cc);
                cc--;
            }
            cc++;
        }
    }

    Log::Pop();

    return newNodes;
}

int AST::CalculateStructSize(AST *program, const std::string& structName){
    int result = 0;

    AST *structDec = program->FindChildrenByValue(structName, {NT_STRUCT_TYPE_DECLARATION, NT_HEADER_TYPE_DECLARATION});
    Assert(structDec != NULL);

    for(AST *sf : structDec->getChildNT(NT_STRUCT_FIELD_LIST)->children){
        AST *type = sf->children[1];
        if(type->nType == NT_BASE_TYPE){
            Assert(type->value == "bit");
            if(type->children.size() == 1)
                result = result + IPLBase::intFromStr(type->children[0]->value.c_str());
            else
                result = result + 1;
        }
        else if(type->nType == NT_TYPE_NAME){
            result = result + CalculateStructSize(program, type->value);
        }
        else{
            Assert(false);
        }
    }

    return result;
}

int AST::CalculateStructSize(const std::vector<AST*>& structs, const std::string& structName){
    AST *temp = new AST("", NT_PROGRAM);
    temp->children = structs;
    int result = CalculateStructSize(temp, structName);
    temp->children.clear();
    delete temp;
    return result;
}

void ASTChangeExternName(AST *a, const std::string oldN, const std::string newN){
    if(a->getProgramType() == oldN)
        a->setProgramType(newN);
    if(a->getNodeType() == NT_TYPE_NAME && a->getValue() == oldN)
        a->setValue(newN);
    for(AST *c : a->getChildren())
        if(c != NULL)
            ASTChangeExternName(c, oldN, newN);
}

std::vector<AST*> AST::MergeUnifyExterns(const std::vector<AST*>& programs){
    std::vector<AST*> newNodes;
    std::vector<std::vector<AST*>> externs(programs.size());

    for(int i = 0; i < (int) programs.size(); i++){
        for(AST *c : programs[i]->children)
            if(c != NULL && c->nType == NT_EXTERN_DECLARATION)
                externs[i].push_back(c);
    }

    std::vector<std::map<AST*, AST*>> deleteExterns(programs.size());

    for(int i = 0; i < (int) programs.size(); i++){
        for(int ei = 0; ei < (int) externs[i].size(); ei++){
            bool repeated = false;
            for(int ni = 0; ni < (int) newNodes.size(); ni++){
                AST *n = newNodes[ni];
                if(AST::Equals(externs[i][ei], n)){
                    repeated = true;
                    deleteExterns[i].emplace(std::make_pair(externs[i][ei], n));
                }
            }
            if(!repeated){
                std::string eName = (externs[i][ei]->children.size() == 3) ? externs[i][ei]->value : externs[i][ei]->children[1]->value;
                newNodes.push_back(externs[i][ei]->Clone());
                deleteExterns[i].emplace(std::make_pair(externs[i][ei], newNodes[newNodes.size() - 1]));
            }
        }
    }

    for(int i = 0; i < (int) deleteExterns.size(); i++){
        for(auto it : deleteExterns[i])
            ASTChangeExternName(programs[i], it.first->value, it.second->value);

        int cc = 0;
        while(cc < (int) programs[i]->children.size()){
            if(deleteExterns[i].find(programs[i]->children[cc]) != deleteExterns[i].end()){
                delete programs[i]->children[cc];
                programs[i]->children.erase(programs[i]->children.begin() + cc);
                cc--;
            }
            cc++;
        }
    }

    return newNodes;
}

AST* ASTfindHeaderInProgramsRec(const std::string& name, AST* ast){
    if((ast->getNodeType() == NT_HEADER_TYPE_DECLARATION || ast->getNodeType() == NT_STRUCT_TYPE_DECLARATION) && ast->getValue() == name)
        return ast;

    for(AST *c : ast->getChildren()){
        if(c != NULL){
            AST* header = ASTfindHeaderInProgramsRec(name, c);
            if(header != NULL)
                return header;
        }
    }

    return NULL;
}

AST* ASTfindHeaderInPrograms(const std::string& name, const std::vector<AST*>& programs){
    for(AST* p : programs){
        AST* header = ASTfindHeaderInProgramsRec(name, p);
        if(header != NULL)
            return header;
    }

    return NULL;
}

void AST::SubstituteHeaderType(AST *oldHeader, AST *newHeader, const std::vector<AST*>& programs){
    if(this->programType == oldHeader->value)
        this->programType = newHeader->value;

    if(this->nType == NT_TYPE_NAME && this->value == oldHeader->value){
        this->value = newHeader->value;
    }
    else if(this->nType == NT_EXPRESSION || this->nType == NT_NONE){
        if(this->value == "."){
            if(this->children[0]->value == "." && this->children.size() == 2){
                std::string prevType = this->children[0]->programType;
                this->children[0]->SubstituteHeaderType(oldHeader, newHeader, programs);
                if(this->children[0]->programType != prevType && this->parent->nType != NT_METHOD_CALL_STATEMENT){
                    std::string currentHeaderType = this->programType;
                    AST* currentLHSheader = ASTfindHeaderInPrograms(prevType, programs);
                    Assert(currentLHSheader != NULL);
                    AST* newLHSheader = ASTfindHeaderInPrograms(this->children[0]->programType, programs);
                    Assert(newLHSheader != NULL);

                    AST *currentSFL = currentLHSheader->getChildNT(NT_STRUCT_FIELD_LIST);
                    AST *newSFL = newLHSheader->getChildNT(NT_STRUCT_FIELD_LIST);
                    Assert(currentSFL != NULL);
                    Assert(newSFL != NULL);
                    
                    int bitSize = 0;
                    size_t posOld = 0;
                    while(posOld < currentSFL->children.size() && currentSFL->children[posOld]->value != this->children[1]->value){
                        bitSize = bitSize + AST::GetSizeOfFieldInBits(currentSFL->children[posOld]);
                        posOld++;
                    }
                    Assert(posOld < currentSFL->children.size());

                    size_t posNew = 0;
                    while(posNew < newSFL->children.size() && bitSize > 0){
                        bitSize = bitSize - AST::GetSizeOfFieldInBits(newSFL->children[posNew]);
                        posNew++;
                    }
                    Assert(posNew < newSFL->children.size());
                    Assert(bitSize == 0);
                    Assert(AST::GetSizeOfFieldInBits(currentSFL->children[posOld]) == AST::GetSizeOfFieldInBits(newSFL->children[posNew]));

                    this->children[1]->value = newSFL->children[posNew]->value;
                }
            }

            if(this->children[0]->programType == oldHeader->value && this->children.size() == 2){
                if(this->children[1] != NULL){
                    AST *currentSFL = oldHeader->getChildNT(NT_STRUCT_FIELD_LIST);
                    AST *newSFL = newHeader->getChildNT(NT_STRUCT_FIELD_LIST);
                    Assert(currentSFL != NULL);
                    Assert(newSFL != NULL);
                    
                    int bitSize = 0;
                    size_t posOld = 0;
                    while(posOld < currentSFL->children.size() && currentSFL->children[posOld]->value != this->children[1]->value){
                        bitSize = bitSize + AST::GetSizeOfFieldInBits(currentSFL->children[posOld]);
                        posOld++;
                    }
                    Assert(posOld < currentSFL->children.size());

                    size_t posNew = 0;
                    while(posNew < newSFL->children.size() && bitSize > 0){
                        bitSize = bitSize - AST::GetSizeOfFieldInBits(newSFL->children[posNew]);
                        posNew++;
                    }
                    Assert(posNew < newSFL->children.size());
                    Assert(bitSize == 0);
                    Assert(AST::GetSizeOfFieldInBits(currentSFL->children[posOld]) == AST::GetSizeOfFieldInBits(newSFL->children[posNew]));

                    this->children[1]->value = newSFL->children[posNew]->value;
                }
            }
        }
    }
    else if(this->nType == NT_ANNOTATION_TOKEN_LIST){
        for(size_t i = 1; i < this->children.size() - 1; i++){
            if(this->children[i]->value == "." && this->children[i-1]->programType == oldHeader->value){
                this->children[i-1]->programType = newHeader->value;

                AST *currentSFL = oldHeader->getChildNT(NT_STRUCT_FIELD_LIST);
                AST *newSFL = newHeader->getChildNT(NT_STRUCT_FIELD_LIST);
                Assert(currentSFL != NULL);
                Assert(newSFL != NULL);
                
                int bitSize = 0;
                size_t posOld = 0;
                while(posOld < currentSFL->children.size() && currentSFL->children[posOld]->value != this->children[i+1]->value){
                    bitSize = bitSize + AST::GetSizeOfFieldInBits(currentSFL->children[posOld]);
                    posOld++;
                }
                Assert(posOld < currentSFL->children.size());

                size_t posNew = 0;
                while(posNew < newSFL->children.size() && bitSize > 0){
                    bitSize = bitSize - AST::GetSizeOfFieldInBits(newSFL->children[posNew]);
                    posNew++;
                }
                Assert(posNew < newSFL->children.size());
                Assert(bitSize == 0);
                Assert(AST::GetSizeOfFieldInBits(currentSFL->children[posOld]) == AST::GetSizeOfFieldInBits(newSFL->children[posNew]));

                this->children[i+1]->value = newSFL->children[posNew]->value;
            }
        }
            
    }
    for(AST *c : this->children){
        if(c != NULL){
            c->parent = this;
            c->SubstituteHeaderType(oldHeader, newHeader, programs);
        }
    }
}

int AST::GetSizeOfFieldInBits(AST *field){
    Assert(field->nType == NT_STRUCT_FIELD);
    if(field->children[1]->nType == NT_BASE_TYPE){
        if(field->children[1]->children.size() == 1){
            return IPLBase::intFromStr(field->children[1]->children[0]->str());
        }
        else{
            return 1;
        }
    }
    else if(field->children[1]->nType == NT_TYPE_NAME){
        return 1; //TODO: This is a workaround, we should calculate the size of the type
    }
    Assert(false);
    return 0;
}

void AST::ChangeHeaderFieldName(const std::string headerName, const std::string oldName, const std::string newName, AST *parent){
    if(parent != NULL && parent->getNodeType() != NT_METHOD_CALL_STATEMENT && (this->getValue() == ".") && (this->getNodeType() == NT_NONE || this->getNodeType() == NT_EXPRESSION) && this->children.size() == 2){
        if(this->children[0]->getProgramType() == headerName && this->children[1]->getValue() == oldName){
            this->children[1]->setValue(newName);
        }
    }

    if(this->nType == NT_ANNOTATION_TOKEN_LIST){
        for(size_t i = 1; i < this->children.size() - 1; i++){
            if(this->children[i]->value == "." && this->children[i-1]->programType == headerName && this->children[i+1]->value == oldName){
                this->children[i+1]->value = newName;
            }
        }
    }

    for(AST *c : this->children)
        if(c != NULL)
            c->ChangeHeaderFieldName(headerName, oldName, newName, this);
}

void AST::RemoveHeaderFieldsPrefixRec(const std::string headerName, const std::string longestP, const AST* parent){
    if(parent != NULL && parent->getNodeType() != NT_METHOD_CALL_STATEMENT && (this->value == ".") && (this->nType == NT_NONE || this->nType == NT_EXPRESSION) && this->children.size() == 2){
        if(this->children[0]->programType == headerName){
            this->children[1]->setValue(this->children[1]->value.substr(longestP.size()));
        }
    }

    if(this->nType == NT_ANNOTATION_TOKEN_LIST){
        for(size_t i = 1; i < this->children.size() - 1; i++){
            if(this->children[i]->getValue() == "." && this->children[i-1]->getProgramType() == headerName){
                this->children[i+1]->setValue(this->children[i+1]->getValue().substr(longestP.size()));
            }
        }
    }

    for(AST *c : this->children)
        if(c != NULL)
            c->RemoveHeaderFieldsPrefixRec(headerName, longestP, this);
}

void AST::RemoveHeaderFieldsPrefix(AST *header){
    AST *sfl = header->getChildNT(NT_STRUCT_FIELD_LIST);

    if(sfl == NULL || sfl->children.size() == 0)
        return;

    std::string longestPrefix = sfl->children[0]->value;
    for(int i = 1; i < (int) sfl->children.size(); i++){
        AST *structField = sfl->children[i];
        int j = 0;
        while(j < (int) structField->value.size() && j < (int) longestPrefix.size() && longestPrefix[j] == structField->value[j])
            j++;
        if(j < (int) longestPrefix.size())
            longestPrefix = longestPrefix.substr(0, j);
    }

    while(longestPrefix.size() > 0 && longestPrefix[longestPrefix.size() - 1] != '_')
        longestPrefix.pop_back();

    if(longestPrefix.size() > 0){
        char buffer[BUFFER_SIZE];
        sprintf(buffer, "Removing \"%s\" prefix from header \"%s\"", longestPrefix.c_str(), header->str());
        Log::MSG(buffer, LOG_LEVEL_SHORT);
        Log::Push();
        
        for(AST *structField : sfl->children){
            sprintf(buffer, "%s -> %s", structField->str(), structField->value.substr(longestPrefix.size(), structField->value.size() - longestPrefix.size()).c_str());
            Log::MSG(buffer, LOG_LEVEL_ALL);

            structField->value = structField->value.substr(longestPrefix.size(), structField->value.size() - longestPrefix.size());
        }

        Log::Pop();

        this->RemoveHeaderFieldsPrefixRec(header->value, longestPrefix, NULL);
    }

    //Tries to remove other remaining prefixes
    std::unordered_map<std::string, int> conflicts;
    for(size_t i = 0; i < sfl->children.size(); i++){
        AST *structField = sfl->children[i];
        int ulp = findPrefix(structField->value);
        if(ulp != -1){
            size_t pos = structField->value.substr(ulp + 1).find(header->value + "_");
            if(pos != std::string::npos){
                const std::string newName = structField->value.substr(ulp + 1 + pos + header->value.size() + 1);
                auto it = conflicts.find(newName);
                if(it != conflicts.end())
                    it->second++;
                else
                    conflicts.emplace(std::make_pair(newName, 1));
            }
        }
    }
    for(size_t i = 0; i < sfl->children.size(); i++){
        AST *structField = sfl->children[i];
        int ulp = findPrefix(structField->value);
        if(ulp != -1){
            size_t pos = structField->value.substr(ulp + 1).find(header->value + "_");
            if(pos != std::string::npos){
                const std::string newName = structField->value.substr(ulp + 1 + pos + header->value.size() + 1);
                auto it = conflicts.find(newName);
                Assert(it != conflicts.end());
                if(it->second == 1 && !AST::isP4Keyword(newName)){
                    this->ChangeHeaderFieldName(header->value, structField->value, newName, NULL);
                    structField->value = newName;
                }
            }
        } 
    }
}

bool UnusedBlock(AST *ast, const std::string& blockName){
    if((ast->getNodeType() == NT_EXPRESSION || ast->getNodeType() == NT_TYPE_NAME) && ast->getValue() == blockName){
        return false;
    }
    for(AST *c : ast->getChildren()){
        if(c != NULL && !UnusedBlock(c, blockName))
            return false;
    }
    return true;
}

bool UnusedTypeName(AST *ast, const std::string& typeName){
    if(ast->getNodeType() == NT_TYPE_NAME && ast->getValue() == typeName)
        return false;
    for(AST *c : ast->getChildren()){
        if(c != NULL && !UnusedTypeName(c, typeName))
            return false;
    }
    return true;
}

void AST::RemoveHeaderPrefix(const std::string headerName){
    int ulp = findPrefix(headerName);
    Assert(ulp != -1);
    if(this->value == headerName)
        this->value = headerName.substr(ulp + 1);
    if(this->programType == headerName)
        this->programType = headerName.substr(ulp + 1);

    for(AST *c : this->children)
        if(c != NULL)
            c->RemoveHeaderPrefix(headerName);
}

void AST::RemoveControllerHeaderAnnotations(const int cpuPort){
    for(AST *c : this->children){
        //Skip virtual packet headers
        if(c != NULL && c->nType == NT_HEADER_TYPE_DECLARATION && c->value != VIRT_PACKET_IN_HEADER_TYPE && c->value != VIRT_PACKET_OUT_HEADER_TYPE){
            size_t ali = 0;
            while(ali < c->children.size()){
                if(c->children[ali] != NULL && c->children[ali]->nType == NT_ANNOTATION_LIST){
                    AST *AL = c->children[ali];
                    size_t i = 0;
                    while(i < AL->children.size()){
                        if(AL->children[i]->value == "controller_header"){
                            AST *ATL = AL->children[i]->getChildNT(NT_ANNOTATION_TOKEN_LIST);
                            if(ATL != NULL){
                                for(AST *at : ATL->children){
                                    if(cpuPort <= 0 && (at->value == "\"packet_in\"" || at->value == "\"packet_out\"")){
                                        fprintf(stderr, "Warning: Header \"%s\" has a %s annotation but no CPU PORT is defined.\n", c->value.c_str(), at->value.c_str());
                                    }
                                }
                            }

                            delete AL->children[i];
                            AL->children.erase(AL->children.begin() + i);
                            i--;
                        }
                        i++;
                    }
                    if(AL->children.size() == 0){
                        delete c->children[ali];
                        c->children[ali] = NULL;
                    }
                }
                ali++;
            }
        }
    }
}

void AST::AddHeaderPaddings(){
    for(AST *c : this->children){
        if(c->nType == NT_HEADER_TYPE_DECLARATION || c->nType == NT_STRUCT_TYPE_DECLARATION){
            AST *sfl = c->getChildNT(NT_STRUCT_FIELD_LIST);
            if(sfl != NULL){
                int bitcount = 0;
                bool allFieldsAreBits = true;
                for(AST *sf : sfl->children){
                    if(sf->getChildNT(NT_BASE_TYPE) != NULL && sf->getChildNT(NT_BASE_TYPE)->value == "bit"){
                        int bitsize = -1;
                        if(sf->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL) != NULL)
                            bitsize = IPLBase::intFromStr(sf->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL)->str());
                        else if(sf->getChildNT(NT_BASE_TYPE)->getChildNT(NT_EXPRESSION) != NULL)
                            bitsize = IPLBase::intFromStr(sf->getChildNT(NT_BASE_TYPE)->getChildNT(NT_EXPRESSION)->str());
                        Assert(bitsize > 0);
                        bitcount = bitcount + bitsize;
                    }
                    else{
                        allFieldsAreBits = false;
                    }
                }
                if(allFieldsAreBits){
                    int padding = (8 - (bitcount % 8)) % 8;
                    if(padding != 0){
                        char buffer[128];
                        sprintf(buffer, "Adding %d bits of padding to \"%s\"", padding, c->str());
                        Log::MSG(buffer, LOG_LEVEL_SHORT);
                        AST *paddingAST = new AST("_virt_padding", NT_STRUCT_FIELD);
                        paddingAST->addChild(new AST("", NT_ANNOTATION_LIST));
                        paddingAST->children[0]->addChild(new AST("padding", NT_ANNOTATION));
                        paddingAST->children[0]->children[0]->addChild(NULL);
                        paddingAST->addChild(new AST("bit", NT_BASE_TYPE));
                        sprintf(buffer, "%d", padding);
                        paddingAST->children[1]->addChild(new AST(std::string(buffer), NT_LITERAL));
                        sfl->addChild(paddingAST);
                    }
                }
            }
        }
    }
}

AST* AST::FindChildrenByValue(const LexVal& value, const std::unordered_set<NodeType>& validTypes){
    AST* child = NULL;
    for(AST *c : this->children)
        if(c != NULL && c->value == value && (validTypes.empty() || validTypes.find(c->nType) != validTypes.end()))
            child = c;
    return child;
}

AST* AST::FindDescendantByValue(const std::string& value, const std::unordered_set<NodeType>& validTypes){
    for(AST *c : this->children)
        if(c != NULL && c->value == value && (validTypes.empty() || validTypes.find(c->nType) != validTypes.end()))
            return c;
    
    for(AST *c : this->children){
        if(c != NULL){
            AST *descendant = c->FindDescendantByValue(value, validTypes);
            if(descendant != NULL)
                return descendant;
        }
    }

    return NULL;
}

void AST::SubstituteValue(const std::string& oldValue, const std::string& newValue){
    if(this->value == oldValue)
        this->value = newValue;
    if(this->programType == oldValue)
        this->programType == newValue;
    for(AST *c : this->children)
        if(c != NULL)
            c->SubstituteValue(oldValue, newValue);
}

void AST::RemoveMergePrefixes(){
    switch(this->nType){
        case NT_PARSER_DECLARATION:
            if(this->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST) != NULL){
                for(AST* LocalElement : this->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->children){
                    int ulp = findPrefix(LocalElement->value);
                    if(ulp != -1){
                        const std::string candidateName = LocalElement->value.substr(ulp + 1);
                        bool noConflicts = true;
                        for(AST* LocalElement2 : this->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->children){
                            if(LocalElement2 != LocalElement && findPrefix(LocalElement2->value) != -1 && candidateName == LocalElement2->value.substr(findPrefix(LocalElement2->value) + 1)){
                                noConflicts = false;
                            }
                        }
                        if(noConflicts){
                            for(AST *LocalElement2 : this->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->children)
                                LocalElement2->SubstituteIdentifierWithType(LocalElement->value, candidateName, LocalElement->programType);
                            Assert(this->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST) != NULL);
                            for(AST *State : this->getChildNT(NT_PARSER_STATE_LIST)->children)
                                State->SubstituteIdentifierWithType(LocalElement->value, candidateName, LocalElement->programType);
                            
                            Log::MSG("Renamed \"" + LocalElement->value + "\" to \"" + candidateName + "\" in \"" + this->getChildNT(NT_PARSER_TYPE_DECLARATION)->value + "\"", LOG_LEVEL_ALL);
                            LocalElement->value = candidateName;
                        }
                    }
                }
            }

            for(AST* state : this->getChildNT(NT_PARSER_STATE_LIST)->children){
                int ulp = findPrefix(state->value);
                if(ulp != -1){
                    const std::string candidateName = state->value.substr(ulp + 1);
                    bool noConflicts = true;
                    for(AST *state2 : this->getChildNT(NT_PARSER_STATE_LIST)->children){
                        if(state2 != state && findPrefix(state2->value) != -1 && candidateName == state2->value.substr(findPrefix(state2->value) + 1)){
                            noConflicts = false;
                        }
                    }
                    if(noConflicts){
                        for(AST *state2 : this->getChildNT(NT_PARSER_STATE_LIST)->children){
                            if(state2->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL && state2->getChildNT(NT_NAME_STATE_EXPRESSION)->value == state->value){
                                state2->getChildNT(NT_NAME_STATE_EXPRESSION)->value = candidateName;
                            }
                            else if(state2->getChildNT(NT_SELECT_EXPRESSION) != NULL) {
                                for(AST *selectCase : state2->getChildNT(NT_SELECT_EXPRESSION)->children[1]->children){
                                    if(selectCase->value == state->value)
                                        selectCase->value = candidateName;
                                }
                            }
                        }
                        Log::MSG("Renamed \"" + state->value + "\" to \"" + candidateName + "\" in \"" + this->getChildNT(NT_PARSER_TYPE_DECLARATION)->value + "\"", LOG_LEVEL_ALL);
                        state->value = candidateName;
                    }
                }
            }
            break;
        case NT_CONTROL_DECLARATION:
            if(this->getChildNT(NT_CONTROL_LOCAL_DEC_LIST) != NULL){
                for(AST* LocalElement : this->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->children){
                    int ulp = findPrefix(LocalElement->value);
                    if(ulp != -1){
                        const std::string candidateName = LocalElement->value.substr(ulp + 1);
                        bool noConflicts = true;
                        for(AST* LocalElement2 : this->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->children){
                            if(LocalElement2 != LocalElement && findPrefix(LocalElement2->value) != -1 && candidateName == LocalElement2->value.substr(findPrefix(LocalElement2->value) + 1)){
                                noConflicts = false;
                            }
                        }
                        if(noConflicts){
                            Log::MSG("Renamed \"" + LocalElement->value + "\" to \"" + candidateName + "\" in \"" + this->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value + "\"", LOG_LEVEL_ALL);
                            CFG::ChangeVTableName(this->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value, LocalElement->value, candidateName);
                            CFG::ChangeVActionName(this->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value, LocalElement->value, candidateName);
                            
                            const std::string oldID = LocalElement->value;
                            for(AST *LocalElement2 : this->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->children)
                                LocalElement2->SubstituteIdentifierWithType(oldID, candidateName, LocalElement->programType);
                            if(this->getChildNT(NT_STAT_OR_DEC_LIST) != NULL)
                                for(AST *SOD : this->getChildNT(NT_STAT_OR_DEC_LIST)->children)
                                    SOD->SubstituteIdentifierWithType(oldID, candidateName, LocalElement->programType);

                            LocalElement->value = candidateName;
                        }
                    }
                }
            }
            break;
        default:
            for(AST *c : this->children){
                if(c != NULL){
                    if(c->nType == NT_HEADER_TYPE_DECLARATION || c->nType == NT_STRUCT_TYPE_DECLARATION){
                        int ulp = findPrefix(c->value);
                        if(ulp != -1){
                            bool noConflicts = true;
                            for(AST *c2 : this->children){
                                if(c2 != NULL && c2 != c && (c2->nType == NT_HEADER_TYPE_DECLARATION || c2->nType == NT_STRUCT_TYPE_DECLARATION) && findPrefix(c2->value) != -1 && c2->value.substr(findPrefix(c2->value)) == c->value.substr(ulp)){
                                    noConflicts = false;
                                }
                            }
                            if(noConflicts){
                                this->RemoveHeaderPrefix(c->value);
                            }
                        }

                        this->RemoveHeaderFieldsPrefix(c);
                    }
                    else if(c->nType == NT_PARSER_DECLARATION){
                        int ulpPN = findPrefix(c->getChildNT(NT_PARSER_TYPE_DECLARATION)->value);
                        if(ulpPN != -1){
                            bool noConflicts = true;
                            for(AST *c2 : this->children){
                                if(c2 != NULL && c2 != c && c2->nType == NT_PARSER_DECLARATION && findPrefix(c2->getChildNT(NT_PARSER_TYPE_DECLARATION)->value) != -1 && c2->getChildNT(NT_PARSER_TYPE_DECLARATION)->value.substr(findPrefix(c2->getChildNT(NT_PARSER_TYPE_DECLARATION)->value)) == c->getChildNT(NT_PARSER_TYPE_DECLARATION)->value.substr(ulpPN)){
                                    noConflicts = false;
                                }
                            }
                            if(noConflicts){
                                std::string newName = c->getChildNT(NT_PARSER_TYPE_DECLARATION)->value.substr(ulpPN + 1);

                                Log::MSG("Renamed \"" + c->getChildNT(NT_PARSER_TYPE_DECLARATION)->value + "\" to \"" + newName + "\"", LOG_LEVEL_SHORT);
                                this->SubstituteTypeName(c->getChildNT(NT_PARSER_TYPE_DECLARATION)->value, newName);
                                c->getChildNT(NT_PARSER_TYPE_DECLARATION)->value = newName;
                            }
                        }
                    }
                    else if(c->nType == NT_CONTROL_DECLARATION){
                        int ulpPN = findPrefix(c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value);
                        if(ulpPN != -1){
                            bool noConflicts = true;
                            for(AST *c2 : this->children){
                                if(c2 != NULL && c2 != c && c2->nType == NT_CONTROL_DECLARATION && findPrefix(c2->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value) != -1 && c2->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value.substr(findPrefix(c2->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value)) == c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value.substr(ulpPN)){
                                    noConflicts = false;
                                }
                            }
                            if(noConflicts){
                                std::string newName = c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value.substr(ulpPN + 1);

                                Log::MSG("Renamed \"" + c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value + "\" to \"" + newName + "\"", LOG_LEVEL_SHORT);
                                this->SubstituteTypeName(c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value, newName);
                                c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value = newName;
                            }
                        }
                    }
                    c->RemoveMergePrefixes();
                }
            }
            break;
    }
}

void AST::FixUnusedStructFieldsNames(){
    char buffer[32];

    Assert(this->nType == NT_PROGRAM);

    for(AST *c : this->children){
        if(c->nType == NT_STRUCT_TYPE_DECLARATION || c->nType == NT_HEADER_TYPE_DECLARATION){
            AST *SFL = c->getChildNT(NT_STRUCT_FIELD_LIST);
            if(SFL != NULL){
                int unusedFields = 1;
                for(AST *SF : SFL->children){
                    Assert(SF->nType == NT_STRUCT_FIELD);
                    AST *AL = SF->getChildNT(NT_ANNOTATION_LIST);
                    if(AL != NULL){
                        bool isUnused = false;
                        for(AST *AT : AL->children){
                            if(AT->value == "unused"){
                                isUnused = true;
                            }
                        }
                        if(isUnused){
                            sprintf(buffer, "_unused%d", unusedFields);
                            SF->value = buffer;
                            unusedFields++;
                        }
                    }
                }
            }
        }
    }
}

void AST::RemoveUnusedBlocks(){
    std::unordered_set<AST*> childsToDelete;

    for(AST *child : this->children)
        if(child != NULL && (child->nType == NT_PARSER_DECLARATION || child->nType == NT_CONTROL_DECLARATION) && UnusedBlock(this, child->children[0]->value))
            childsToDelete.emplace(child);

    int i = 0;
    while(i < (int) this->children.size()){
        if(childsToDelete.find(this->children[i]) != childsToDelete.end()){
            Log::MSG("Remove unused block \"" + this->children[i]->children[0]->value + "\"", LOG_LEVEL_SHORT);
            delete this->children[i];
            this->children.erase(this->children.begin() + i);
            i--;
        }
        i++;
    }

    if(!childsToDelete.empty())
        this->RemoveUnusedBlocks();
}


void AST::RemoveUnusedHeaders(){
    std::unordered_set<AST*> childsToDelete;

    for(AST *child : this->children)
        if(child != NULL && (child->nType == NT_HEADER_TYPE_DECLARATION || child->nType == NT_STRUCT_TYPE_DECLARATION) && UnusedTypeName(this, child->value)){
            bool hasControllerHeaderAnnotation = false;
            if(child->getChildNT(NT_ANNOTATION_LIST) != NULL){
                for(AST *annotation : child->getChildNT(NT_ANNOTATION_LIST)->children){
                    if(annotation->value == "controller_header"){
                        hasControllerHeaderAnnotation = true;
                    }
                }
            }

            if(!hasControllerHeaderAnnotation)
                childsToDelete.emplace(child);
        }

    int i = 0;
    while(i < (int) this->children.size()){
        if(childsToDelete.find(this->children[i]) != childsToDelete.end()){
            Log::MSG("Remove unused header/struct \"" + this->children[i]->value + "\"", LOG_LEVEL_SHORT);
            delete this->children[i];
            this->children.erase(this->children.begin() + i);
            i--;
        }
        i++;
    }

    if(!childsToDelete.empty())
        this->RemoveUnusedHeaders();
}

void AST::RemoveDuplicateHeaders(){
    std::vector<AST*> structHeaderDeclarations;
    std::set<int> childsToDelete;

    for(AST *child : this->children)
        if(child != NULL && (child->nType == NT_HEADER_TYPE_DECLARATION || child->nType == NT_STRUCT_TYPE_DECLARATION))
            structHeaderDeclarations.push_back(child);

    for(int i = 0; i < (int) structHeaderDeclarations.size(); i++){
        if(childsToDelete.find(i) == childsToDelete.end()){
            for(int j = i + 1; j < (int) structHeaderDeclarations.size(); j++){
                if(AST::Equals(structHeaderDeclarations[i], structHeaderDeclarations[j])){
                    this->SubstituteHeaderType(structHeaderDeclarations[j], structHeaderDeclarations[i], {this});
                    childsToDelete.emplace(j);
                }
            }
        }
    }

    int dc = 0;
    int i = 0;
    while(i < (int) this->children.size()){
        if(childsToDelete.find(i + dc) != childsToDelete.end()){
            delete this->children[i];
            this->children.erase(this->children.begin() + i);
            dc++;
            i--;
        }
        i++;
    }

    //Change VirtTypes numeration
    std::set<int> virtTypesNumbers;
    std::set<int> virtTypesNumbersEmpty;
    for(AST *child : this->children){
        if(child != NULL && (child->nType == NT_HEADER_TYPE_DECLARATION || child->nType == NT_STRUCT_TYPE_DECLARATION) && (child->value.size() > strlen(VIRT_TYPE_ID) && child->value.substr(0, strlen(VIRT_TYPE_ID)) == VIRT_TYPE_ID)){
            if(child->getChildNT(NT_STRUCT_FIELD_LIST) != NULL)
                virtTypesNumbers.emplace(IPLBase::intFromStr(child->value.c_str() + strlen(VIRT_TYPE_ID)));
            else
                virtTypesNumbersEmpty.emplace(IPLBase::intFromStr(child->value.c_str() + strlen(VIRT_TYPE_ID)));
        }
    }

    for(int n : virtTypesNumbersEmpty){
        char buffer1[128];
        char buffer2[128];
        sprintf(buffer1, "%s%d", VIRT_TYPE_ID, n);
        sprintf(buffer2, "VirtTypeEmpty");
        this->SubstituteValue(buffer1, buffer2);
        this->RenameProgramTypes(buffer1, buffer2);
    }

    int vtN = 1;
    for(int n : virtTypesNumbers){
        char buffer1[128];
        char buffer2[128];
        sprintf(buffer1, "%s%d", VIRT_TYPE_ID, n);
        sprintf(buffer2, "%s%d", VIRT_TYPE_ID, vtN);
        this->SubstituteValue(buffer1, buffer2);
        this->RenameProgramTypes(buffer1, buffer2);
        vtN++;
    }

    //Remove Duplicate VirtTypeEmpty
    i = 0;
    bool removeHeader = false;
    while(i < (int) this->children.size()){
        if(this->children[i]->value == "VirtTypeEmpty"){
            if(removeHeader){
                delete this->children[i];
                this->children.erase(this->children.begin() + i);
                i--;
            }
            else{
                removeHeader = true;
            }
        }
        i++;
    }
}

void AST::RemoveSimpleParserStates(){
    for(AST *c : this->children){
        if(c != NULL && c->nType == NT_PARSER_DECLARATION){
            c->RemoveSimpleParserStates();
        }
    }

    if(this->nType == NT_PARSER_DECLARATION){
        AST *stateList = this->getChildNT(NT_PARSER_STATE_LIST);
        if(stateList != NULL){
            size_t i = 0;
            while(i < stateList->children.size() - 1){
                AST *state = stateList->children[i];
                Assert(state != NULL && state->nType == NT_PARSER_STATE);        
                if(state->getChildNT(NT_PARSER_STATEMENT_LIST) == NULL && state->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL){   
                    const std::string name = state->getChildNT(NT_NAME_STATE_EXPRESSION)->value;
                    if(state->value == "start") {
                        std::string msg = "Changing state \"" + name + "\" from parser \"" + this->getChildNT(NT_PARSER_TYPE_DECLARATION)->value + "\" to starting state (start)";
                        Log::MSG(msg, LOG_LEVEL_ALL);
                        stateList->SubstituteValue(name, state->value);
                    }
                    else {
                        std::string msg = "Removing state \"" + state->value + "\" from parser \"" + this->getChildNT(NT_PARSER_TYPE_DECLARATION)->value + "\" as it has no statements and a single transition";
                        Log::MSG(msg, LOG_LEVEL_ALL);
                        stateList->SubstituteValue(state->value, name);
                    }
                    delete state;
                    stateList->children.erase(stateList->children.begin() + i);
                    i--;
                }
                i++;
            }
        }
    }
}

void AST::CheckParsersStates() {
    Assert(this->nType == NT_PROGRAM);
    for(AST *c : this->children)
        if(c != NULL)
            c->CheckParserStates(this->value);
}

void AST::CheckParserStates(const std::string& programName) {
    if(this->nType != NT_PARSER_DECLARATION)
        return;
    ASTParser::CheckParsersStates(this, programName);
}

bool AST::CanMergeStructFieldListByNames(const std::vector<AST*>& headersDec, const std::vector<std::string>& names){
    for(int i = 0; i < (int) names.size(); i++){
        AST* header = NULL;
        for(int j = 0; j < (int) headersDec.size(); j++)
            if(headersDec[j]->value == names[i])
                header = headersDec[j];

        Assert(header != NULL);

        if(header->nType != NT_STRUCT_TYPE_DECLARATION && header->nType != NT_HEADER_TYPE_DECLARATION)
            return false;
    }
    return true;
}

AST* AST::MergeStructFieldListByNames(const std::vector<AST*>& headersDec, const std::vector<std::string>& names){
    bool allNull = true;
    std::vector<AST*> SFLs;
    for(int i = 0; i < (int) names.size(); i++){
        AST* header = NULL;
        for(int j = 0; j < (int) headersDec.size(); j++)
            if(headersDec[j]->value == names[i])
                header = headersDec[j];

        Assert(header != NULL);

        SFLs.push_back(header->getChildNT(NT_STRUCT_FIELD_LIST));
        if(header->getChildNT(NT_STRUCT_FIELD_LIST) != NULL)
            allNull = false;
    }

    if(allNull)
        return NULL;

    std::unordered_map<AST*, std::set<int>> fieldsToPrograms;
    int first = 0;
    while(SFLs[first] == NULL)
        first++;
    AST* result = SFLs[first]->Clone();
    for(AST *sf : result->children){
        fieldsToPrograms.emplace(std::make_pair(sf, std::set<int> {first}));
    }
    for(int i = first + 1; i < (int) SFLs.size(); i++){
        if(SFLs[i] != NULL){
            const int lastSF = (int) result->children.size();
            std::unordered_set<AST*> matched;
            for(int j = 0; j < (int) SFLs[i]->children.size(); j++){
                int x = 0;
                while(x < lastSF && (!Equals(result->children[x], SFLs[i]->children[j]) || matched.find(result->children[x]) != matched.end() || AST::hasVirtPreventMergeAnnotation(result->children[x]))){
                    x++;
                }
                if(x < lastSF){
                    matched.emplace(result->children[x]);
                    fieldsToPrograms.find(result->children[x])->second.insert(i);
                
                    while(x < (int) result->children.size() - 1 && fieldsToPrograms.find(result->children[x + 1])->second.size() == 1 && fieldsToPrograms.find(result->children[x + 1])->second.find(i) != fieldsToPrograms.find(result->children[x + 1])->second.end()){
                        AST *aux = result->children[x];
                        result->children[x] = result->children[x + 1];
                        result->children[x + 1] = aux;
                        x++;
                    }
                }
                else {
                    result->addChild(SFLs[i]->children[j]->Clone());
                    fieldsToPrograms.emplace(std::make_pair(result->children[result->children.size() - 1], std::set<int> {i}));
                }
            }
        }
    }

    return result;
}

void ASTRenameTypeDeclarationsGetRec(AST *a, std::unordered_set<std::string>& s){
    const static std::unordered_set<NodeType> DecTypes = {NT_PARSER_TYPE_DECLARATION, NT_PACKAGE_TYPE_DECLARATION, NT_CONTROL_TYPE_DECLARATION, NT_HEADER_TYPE_DECLARATION, NT_STRUCT_TYPE_DECLARATION};
    for(AST *c : a->getChildren()){
        if(c != NULL){
            if(DecTypes.find(c->getNodeType()) != DecTypes.end())
                if(s.find(c->getValue()) == s.end())
                    s.emplace(c->getValue());
            ASTRenameTypeDeclarationsGetRec(c, s);
        }
    }
}

void ASTRenameTypeDeclarationsRec(AST *a, const std::string& oldName, const std::string& newName){
    for(AST *c : a->getChildren()){
        if(c != NULL){
            if(c->getValue() == oldName && (c->getNodeType() == NT_EXPRESSION || c->getNodeType() == NT_TYPE_NAME || c->getNodeType() == NT_PARSER_TYPE_DECLARATION || c->getNodeType() == NT_PACKAGE_TYPE_DECLARATION || c->getNodeType() == NT_CONTROL_TYPE_DECLARATION || c->getNodeType() == NT_HEADER_TYPE_DECLARATION || c->getNodeType() == NT_STRUCT_TYPE_DECLARATION))
                c->setValue(newName);
            if(c->getProgramType() == oldName)
                c->setProgramType(newName);
            ASTRenameTypeDeclarationsRec(c, oldName, newName);
        }
    }
}

void AST::RenameTypeDeclarations(const std::vector<AST *>& programs){
    char buffer[128];
    std::vector<std::unordered_set<std::string>> PTypeNames(programs.size());

    Log::MSG("Renaming type declarations in programs", LOG_LEVEL_SHORT);
    Log::Push();

    for(int i = 0; i < (int) programs.size(); i++){
        ASTRenameTypeDeclarationsGetRec(programs[i], PTypeNames[i]);
        for(const std::string& t1 : PTypeNames[i]){
            sprintf(buffer, "Ps%d_%s", i+1, t1.c_str());
            ASTRenameTypeDeclarationsRec(programs[i], t1, std::string(buffer));

            Log::MSG("Renamed \"" + t1 + "\" to \"" + std::string(buffer) + "\" in \"" + programs[i]->value + "\"", LOG_LEVEL_ALL);
        }
    }

    Log::Pop();
}

void AST::AddGlobalActionsToCFG(const std::vector<AST *>& programs){
    int i = 1;
    for(AST* prog : programs){
        for(AST *c : prog->children){
            if(c->nType == NT_ACTION_DECLARATION){
                CFG::Get().AddProgramAction(i - 1, c, c->value, c->value, "", "");
            }
        }
        i++;
    }
}

AST* AST::MergeAST(const std::vector<AST*> programs, const std::vector<std::set<int>>& virtualSwitchPorts, const P4Architecture architecture, const int cpuPort){
    Assert(architecture != P4A_UNDEFINED);

    std::string msg;

    AST *root = new AST("merged.p4", NT_PROGRAM);

    RenameTypeDeclarations(programs);
    AddGlobalActionsToCFG(programs);

    Log::MSG("Infering types...", LOG_LEVEL_SHORT);
    Log::Push();
    for(AST* p: programs)
        p->InferTypes();
    Log::Pop();

    std::vector<AST*> newExterns = MergeUnifyExterns(programs);
    for(AST * n : newExterns)
        root->addChild(n);

    std::vector<AST*> newHeaders = MergeUnifyHeaders(programs);
    //Add VirtPacketInHeader and VirtPacketOutHeader
    if(cpuPort > 0){
        AST *VirtPacketInHeader = new AST(VIRT_PACKET_IN_HEADER_TYPE, NT_HEADER_TYPE_DECLARATION);
        VirtPacketInHeader->addChild(new AST("", NT_ANNOTATION_LIST));
        VirtPacketInHeader->children[0]->addChild(new AST("controller_header", NT_ANNOTATION));
        VirtPacketInHeader->children[0]->children[0]->addChild(new AST("", NT_ANNOTATION_TOKEN_LIST));
        VirtPacketInHeader->children[0]->children[0]->children[0]->addChild(new AST("\"packet_in\"", NT_ANNOTATION_TOKEN));
        VirtPacketInHeader->addChild(NULL); //optTypeParameters
        VirtPacketInHeader->addChild(new AST("", NT_STRUCT_FIELD_LIST));
        VirtPacketInHeader->children[2]->addChild(new AST(VIRT_PACKET_IN_HEADER_PROG_ID_FIELD_NAME, NT_STRUCT_FIELD));
        VirtPacketInHeader->children[2]->children[0]->addChild(NULL); //annotations
        VirtPacketInHeader->children[2]->children[0]->addChild(new AST("bit", NT_BASE_TYPE));
        VirtPacketInHeader->children[2]->children[0]->children[1]->addChild(new AST("8", NT_LITERAL));
        newHeaders.push_back(VirtPacketInHeader);

        AST *VirtPacketOutHeader = new AST(VIRT_PACKET_OUT_HEADER_TYPE, NT_HEADER_TYPE_DECLARATION);
        VirtPacketOutHeader->addChild(new AST("", NT_ANNOTATION_LIST));
        VirtPacketOutHeader->children[0]->addChild(new AST("controller_header", NT_ANNOTATION));
        VirtPacketOutHeader->children[0]->children[0]->addChild(new AST("", NT_ANNOTATION_TOKEN_LIST));
        VirtPacketOutHeader->children[0]->children[0]->children[0]->addChild(new AST("\"packet_out\"", NT_ANNOTATION_TOKEN));
        VirtPacketOutHeader->addChild(NULL); //optTypeParameters
        VirtPacketOutHeader->addChild(new AST("", NT_STRUCT_FIELD_LIST));
        VirtPacketOutHeader->children[2]->addChild(new AST(VIRT_PACKET_OUT_HEADER_PROG_ID_FIELD_NAME, NT_STRUCT_FIELD));
        VirtPacketOutHeader->children[2]->children[0]->addChild(NULL); //annotations
        VirtPacketOutHeader->children[2]->children[0]->addChild(new AST("bit", NT_BASE_TYPE));
        VirtPacketOutHeader->children[2]->children[0]->children[1]->addChild(new AST("8", NT_LITERAL));
        newHeaders.push_back(VirtPacketOutHeader);
    }
    for(AST * n : newHeaders)
        root->addChild(n);

    std::unordered_set<AST*> nodesToRemove;

    //Identify Equal Parsers
    std::vector<AST*> newParsers;
    std::vector<std::vector<std::pair<int, AST*>>> equalParsers;

    Log::MSG("Unifying equal Parsers", LOG_LEVEL_SHORT);
    Log::Push();
    for(int p1 = 0; p1 < (int) programs.size(); p1++){
        for(int c1 = 0; c1 < (int) programs[p1]->children.size(); c1++){
            if(programs[p1]->children[c1]->nType == NT_PARSER_DECLARATION){
                for(int p2 = p1 + 1; p2 < (int) programs.size(); p2++){
                    for(int c2 = 0; c2 < (int) programs[p2]->children.size(); c2++){
                        if(programs[p2]->children[c2]->nType == NT_PARSER_DECLARATION && nodesToRemove.find(programs[p2]->children[c2]) == nodesToRemove.end()){
                            Log::EnablePush(false);
                            if(AST::EqualsParserDeclaration(programs[p1], programs[p2], programs[p1]->children[c1], programs[p2]->children[c2], newHeaders)){
                                Log::EnablePop();
                                msg = "Parsers are equal: \"" + programs[p1]->children[c1]->getChildNT(NT_PARSER_TYPE_DECLARATION)->value  + "\"";
                                msg = msg + " and \"" + programs[p2]->children[c2]->getChildNT(NT_PARSER_TYPE_DECLARATION)->value + "\" are equal";
                                Log::MSG(msg, LOG_LEVEL_SHORT);
                                nodesToRemove.emplace(programs[p2]->children[c2]);
                                if(equalParsers.size() > 0 && equalParsers[equalParsers.size() - 1][0].second == programs[p1]->children[c1]){
                                    equalParsers[equalParsers.size() - 1].push_back(std::make_pair(p2, programs[p2]->children[c2]));
                                }
                                else{
                                    nodesToRemove.emplace(programs[p1]->children[c1]);
                                    equalParsers.push_back({std::make_pair(p1, programs[p1]->children[c1]), std::make_pair(p2, programs[p2]->children[c2])});
                                }
                            }
                            else{
                                Log::EnablePop();
                            }
                        }
                    }
                }
            }
        }
    }
    for(const std::vector<std::pair<int, AST*>>& eqs : equalParsers){
        msg = "Unified parsers: ";

        AST* newParser = eqs[0].second->Clone();
        newParsers.push_back(newParser);
        std::string* pName = &newParser->getChildNT(NT_PARSER_TYPE_DECLARATION)->value;
        for(int i = 1; i < (int) eqs.size(); i++)
            *pName = pName->substr(0, pName->find("_")) + eqs[i].second->getChildNT(NT_PARSER_TYPE_DECLARATION)->value.substr(1, eqs[i].second->getChildNT(NT_PARSER_TYPE_DECLARATION)->value.find("_")) + pName->substr(pName->find("_") + 1);

        for(const std::pair<int, AST*>& pd : eqs){
            msg = msg + "\"" + pd.second->getChildNT(NT_PARSER_TYPE_DECLARATION)->value + "\" ";
            programs[pd.first]->SubstituteTypeName(pd.second->getChildNT(NT_PARSER_TYPE_DECLARATION)->value, *pName);
            pd.second->getChildNT(NT_PARSER_TYPE_DECLARATION)->value = *pName;
        }

        msg = msg + "into \"" + *pName + "\"";
        Log::MSG(msg, LOG_LEVEL_SHORT);
    }
    Log::Pop();

    //Identify Equal Controls
    std::vector<AST*> newControls;
    std::vector<std::vector<std::pair<int, AST*>>> equalControls;
    Log::MSG("Unifying equal Controls", LOG_LEVEL_SHORT);
    Log::Push();
    for(int p1 = 0; p1 < (int) programs.size(); p1++){
        for(int c1 = 0; c1 < (int) programs[p1]->children.size(); c1++){
            if(programs[p1]->children[c1]->nType == NT_CONTROL_DECLARATION){
                for(int p2 = p1 + 1; p2 < (int) programs.size(); p2++){
                    for(int c2 = 0; c2 < (int) programs[p2]->children.size(); c2++){
                        if(programs[p2]->children[c2]->nType == NT_CONTROL_DECLARATION && nodesToRemove.find(programs[p2]->children[c2]) == nodesToRemove.end()){
                            Log::EnablePush(false);
                            if(AST::EqualsControlDeclaration(programs[p1], programs[p2], programs[p1]->children[c1], programs[p2]->children[c2], newHeaders)){
                                Log::EnablePop();
                                msg = "Controls are equal: \"" + programs[p1]->children[c1]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value  + "\"";
                                msg = msg + " and \"" + programs[p2]->children[c2]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value + "\" are equal";
                                Log::MSG(msg, LOG_LEVEL_SHORT);
                                nodesToRemove.emplace(programs[p2]->children[c2]);
                                if(equalControls.size() > 0 && equalControls[equalControls.size() - 1][0].second == programs[p1]->children[c1]){
                                    equalControls[equalControls.size() - 1].push_back(std::make_pair(p2, programs[p2]->children[c2]));
                                }
                                else{
                                    nodesToRemove.emplace(programs[p1]->children[c1]);
                                    equalControls.push_back({std::make_pair(p1, programs[p1]->children[c1]), std::make_pair(p2, programs[p2]->children[c2])});
                                }
                            }
                            else{
                                Log::EnablePop();
                            }
                        }
                    }
                }
            }
        }
    }
    for(const std::vector<std::pair<int, AST*>>& eqs : equalControls){
        msg = "Unified controls: ";

        AST* newControl = eqs[0].second->Clone();
        newControls.push_back(newControl);
        std::string* pName = &newControl->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value;
        for(int i = 1; i < (int) eqs.size(); i++)
            *pName = pName->substr(0, pName->find("_")) + eqs[i].second->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value.substr(1, eqs[i].second->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value.find("_")) + pName->substr(pName->find("_") + 1);

        for(const std::pair<int, AST*>& pd : eqs){
            msg = msg + "\"" + pd.second->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value + "\" ";
            programs[pd.first]->SubstituteTypeName(pd.second->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value, *pName);
            pd.second->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value = *pName;
        }

        msg = msg + "into \"" + *pName + "\"";
        Log::MSG(msg, LOG_LEVEL_SHORT);
    }
    Log::Pop();

    for(AST* np : newParsers)
        programs[programs.size() - 1]->addChild(np);
    for(AST* nc : newControls)
        programs[programs.size() - 1]->addChild(nc);

    //Architecture specific merge
    bool result = false;
    switch(architecture){
        case P4A_V1MODEL:
            result = V1Merge::MergeV1(programs, newHeaders, virtualSwitchPorts, cpuPort);
            break;
        case P4A_TNA:
            try {
                result = TofinoMerge::MergeTofino(programs, newHeaders, virtualSwitchPorts, cpuPort);
            }
            catch (ExceptionWrongEmitOrder& eweo){
                delete root;
                throw eweo;
            }
            break;
        default:
            break;
    }
    if(!result){
        fprintf(stderr, "Error merging programs\n");
        exit(1);
    }

    for(AST* p : programs){
        for(AST *c : p->children){
            if(nodesToRemove.find(c) == nodesToRemove.end())
                root->addChild(c->Clone());
        }
    }
    
    root->RemoveEmptySwitches();
    root->RemoveEmptyIfElses();

    root->UnifyErrorMKDeclarations();

    //Sort headers and structs to start
    for(int i = 1; i < (int) root->children.size(); i++){
        if(root->children[i]->nType == NT_HEADER_TYPE_DECLARATION || root->children[i]->nType == NT_STRUCT_TYPE_DECLARATION){
            int j = i;
            while(j > 0 && (root->children[j - 1]->nType != NT_HEADER_TYPE_DECLARATION && root->children[j - 1]->nType != NT_STRUCT_TYPE_DECLARATION)){
                AST *a = root->children[j];
                root->children[j] = root->children[j - 1];
                root->children[j - 1] = a;
                j--;
            }
        }
    }

    root->RemoveControllerHeaderAnnotations(cpuPort);

    root->RemoveDuplicateHeaders();

    Log::MSG("Removing unused blocks...", LOG_LEVEL_SHORT);
    Log::Push();
    root->RemoveUnusedBlocks();
    Log::Pop();

    Log::MSG("Removing unused headers...", LOG_LEVEL_SHORT);
    Log::Push();
    root->RemoveUnusedHeaders();
    Log::Pop();

    Log::MSG("Removing prefixes...", LOG_LEVEL_SHORT);
    Log::Push();
    root->RemoveMergePrefixes();
    Log::Pop();

    root->FixUnusedStructFieldsNames();

    Log::MSG("Adding Padding to headers...", LOG_LEVEL_SHORT);
    Log::Push();
    root->AddHeaderPaddings();
    Log::Pop();

    switch(architecture){
        case P4A_V1MODEL:
            V1Merge::RenameParams(root);
            break;
        case P4A_TNA:
            TofinoMerge::FixCompilerErrors(root);
            break;
        default:
            break;
    }

    //Add Hotswap capability

    ASTSimplifier::RemoveVirtPreventMergeAnnotations(root);

    ASTSimplifier::CheckUnusedHeaders(root);

    return root;
}

void AST::Print(){
    printf("%p (%s) \'%s\': ", this, AST::NodeTypeToString(this->nType).c_str(), this->str());
    for(AST* c : children)
        printf("%p ", c);
    printf("\n");
    for(AST* c : children)
        if(c != NULL)
            c->Print();
}

void ASTGenDotGetADDrs(AST* ast, std::vector<AST*> &addrs){
    addrs.push_back(ast);
    for (AST* c : ast->getChildren()){
        bool inVector = false;
        for(AST* a : addrs){
            if(a == c){
                inVector = true;
                break;
            }
        }
        if(c != NULL && !inVector){
            ASTGenDotGetADDrs(c, addrs);
        }
    }
}

void ASTGenDotEdges(AST* ast, FILE *fp, const std::unordered_map<AST*, int>& addrOrder, std::set<AST*> &visited){
    if(visited.find(ast) != visited.end())
        return;

    visited.emplace(ast);
    
    for(AST* c : ast->getChildren()){
        if(c != NULL)
            fprintf(fp, "NODE_%d -> NODE_%d\n", addrOrder.find(ast)->second, addrOrder.find(c)->second);
    }
    for(AST *c : ast->getChildren()){
        if(c != NULL)
            ASTGenDotEdges(c, fp, addrOrder, visited);
    }
}

void ASTGenDotEdges(AST* ast, FILE *fp, const std::unordered_map<AST*, int>& addrOrder){
    std::set<AST*> visited;
    ASTGenDotEdges(ast, fp, addrOrder, visited);
}

void AST::GenDot(FILE *fp) {
    std::vector<AST*> addrs;
    std::unordered_map<AST*, int> addrOrder;
    ASTGenDotGetADDrs(this, addrs);
    for(int i = 0; i < (int) addrs.size(); i++)
        addrOrder.emplace(addrs[i], i);

    fprintf(fp, "digraph AST {\n");

    for(AST* a : addrs){
        char buffer[1024];
        sprintf(buffer, "%s", AST::NodeTypeToString(a->nType).c_str());
        if(a->value != ""){
            strcat(buffer, "\\lV: ");
            strcat(buffer, a->value.c_str());
        }
        if(a->programType != ""){
            strcat(buffer, "\\lT: ");
            strcat(buffer, a->programType.c_str());
        }
        strcat(buffer, "\\l");
        char *pos = strstr(buffer, "\"");
        if(pos == NULL)
            pos = strstr(buffer, "<");
        if(pos == NULL)
            pos = strstr(buffer, ">");
        while(pos != NULL){
            buffer[strlen(buffer) + 1] = '\0';
            for(int i = strlen(buffer); i > (int) (pos - buffer); i--){
                buffer[i] = buffer[i-1];
            }
            *pos = '\\';
            char *ppos = pos + 2;
            pos = strstr(ppos, "\"");
            if(pos == NULL)
                pos = strstr(ppos, "<");
            if(pos == NULL)
                pos = strstr(ppos, ">");
        }
        
        fprintf(fp, "NODE_%d [ shape=record, style=\"filled\", fillcolor=cornsilk, label=\"%s\" ];\n", addrOrder.find(a)->second, buffer);
    }

    ASTGenDotEdges(this, fp, addrOrder);

    fprintf(fp, "}\n");
}

void AST::GenDot(const std::string& filePath){
    FILE *fp = fopen(filePath.c_str(), "w");
    if(fp == NULL){
        fprintf(stderr, "Error opening file %s\n", filePath.c_str());
        exit(1);
    }
    this->GenDot(fp);
    fclose(fp);
}

bool AST::CheckLegal(bool printErrors, const std::string programName){
    char buffer[BUFFER_SIZE];

    Assert(!printErrors || programName != "");
    
    bool result = true;

    if(this->nType == NT_PARSER_DECLARATION){
        AST *ParserTypeDeclaration = this->getChildNT(NT_PARSER_TYPE_DECLARATION);
        Assert(ParserTypeDeclaration != NULL)
        const std::string pName = ParserTypeDeclaration->value;
        const int typeDecLine = ParserTypeDeclaration->lineNumber;

        bool hasStartState = false;
        int redefinesAccept = 0;
        int redefinesReject = 0;
        std::unordered_set<std::string> stateNames;

        AST *stateList = this->getChildNT(NT_PARSER_STATE_LIST);
        for(AST *state : stateList->children){
            if(state->value == "start")
                hasStartState = true;
            else if(state->value == "accept")
                redefinesAccept = state->lineNumber;
            else if(state->value == "reject")
                redefinesReject = state->lineNumber;
            
            if(stateNames.find(state->value) != stateNames.end()){
                if(printErrors)
                    fprintf(stderr, "Error: Multiple definitions of state \"%s\" in parser \"%s\" (Line %d - %s)\n", state->value.c_str(), pName.c_str(), typeDecLine, programName.c_str());
                result = false;
            }
            else{
                stateNames.insert(state->value);
            }
        }

        if(!hasStartState){
            if(printErrors)
                fprintf(stderr, "Error: Parser \"%s\" has no start state (Line %d - %s)\n", pName.c_str(), typeDecLine, programName.c_str());
            result = false;
        }
        if(redefinesAccept > 0){
            if(printErrors)
                fprintf(stderr, "Error: Parser \"%s\" redefines accept state (Line %d - %s)\n", pName.c_str(), redefinesAccept, programName.c_str());
            result = false;
        }
        if(redefinesReject > 0){
            if(printErrors)
                fprintf(stderr, "Error: Parser \"%s\" redefines reject state (Line %d - %s)\n", pName.c_str(), redefinesReject, programName.c_str());
            result = false;
        }
    }
    else if(this->nType == NT_TABLE_DECLARATION){
        AST* propertyKey = NULL;
        AST* propertySize = NULL;
        AST* propertyDefaultAction = NULL;
        AST* propertyActions = NULL;
        AST* propertyConstEntries = NULL;
        bool hasNoAction = false;
        int keyQuant = 0;
        int constEntriesQuant = 0;

        AST *pList = this->getChildNT(NT_TABLE_PROPERTY_LIST);
        Assert(pList != NULL);
        for(int i = 0; i < (int) pList->children.size(); i++){
            AST *property = pList->children[i];
            if(property->value == "key"){
                propertyKey = property;
                AST *keyElementList = property->getChildNT(NT_KEY_ELEMENT_LIST);
                Assert(keyElementList != NULL);
                keyQuant = (int) keyElementList->children.size();
            }
            else if(property->value == "size"){
                propertySize = property;
            }
            else if(property->value == "actions"){
                propertyActions = property;
                AST *actionList = property->getChildNT(NT_ACTION_LIST);
                if(actionList == NULL){
                    actionList = new AST("", NT_ACTION_LIST);
                    property->addChild(actionList);
                }
                for(AST *actionRef : actionList->children)
                    if(actionRef != NULL && actionRef->value == "NoAction")
                        hasNoAction = true;
                if(!hasNoAction){
                    actionList->addChild(NULL);
                    actionList->addChild(new AST("NoAction", NT_ACTION_REF));
                    actionList->children.back()->addChild(NULL);
                }
            }
            else if(property->value == "const entries"){
                propertyConstEntries = property;
                AST *constEntryList = property->getChildNT(NT_ENTRIES_LIST);
                Assert(constEntryList != NULL);
                constEntriesQuant = (int) constEntryList->children.size();
            }
            else if(strstr(property->value.c_str(), "default_action") != NULL){
                propertyDefaultAction = property;
            }
        }
        if(!propertyActions){
            result = false;
            if(printErrors){
                fprintf(stderr, "Error: Table \"%s\" has no actions (Line %d - %s)\n", this->str(), this->lineNumber, programName.c_str());
            }
        }
        if(!propertyDefaultAction && propertyActions){
            AST *defaultActionP = new AST("default_action", NT_TABLE_PROPERTY);
            //defaultActionP->addChild(NULL); //TODO: OptAnnotations
            defaultActionP->addChild(new AST("NoAction", NT_EXPRESSION));
            pList->addChild(defaultActionP);
            if(printErrors)
                fprintf(stderr, "Warning: Table \"%s\" has no default action. Setting to NoAction... (Line %d - %s)\n", this->str(), this->lineNumber, programName.c_str());  
        }
        if(!propertyKey && propertyConstEntries){
            result = false;
            if(printErrors)
                fprintf(stderr, "Error: Table \"%s\" has const entries without a key property (Line %d - %s)\n", this->str(), this->lineNumber, programName.c_str());
        }
        if(!propertySize){
            if(propertyConstEntries){
                sprintf(buffer, "%d", constEntriesQuant);
                if(printErrors)
                    fprintf(stderr, "Warning: Table \"%s\" does not declare a size. Setting to the ammount of constant entries - %d (Line %d - %s)\n", this->str(), constEntriesQuant, this->lineNumber, programName.c_str());
            }
            else{
                sprintf(buffer, "%d", DEFAULT_TABLE_SIZE);
                if(printErrors)
                    fprintf(stderr, "Warning: Table \"%s\" does not declare a size. Setting to default value - %d (Line %d - %s)\n", this->str(), DEFAULT_TABLE_SIZE, this->lineNumber, programName.c_str());
            }

            AST *sizeP = new AST("size", NT_TABLE_PROPERTY);
            sizeP->addChild(new AST(buffer, NT_EXPRESSION));
            pList->addChild(sizeP);
        }
        if(propertyConstEntries && propertyKey){
            AST *constEntryList = propertyConstEntries->getChildNT(NT_ENTRIES_LIST);
            for(AST *entry : constEntryList->children){
                if((entry->children[0]->nType != NT_TUPLE_KEYSET_EXPRESSION && keyQuant != 1) || (entry->children[0]->nType == NT_TUPLE_KEYSET_EXPRESSION && entry->children[0]->getChildNT(NT_SIMPLE_EXPRESSION_LIST) != NULL && ((int) entry->children[0]->getChildNT(NT_SIMPLE_EXPRESSION_LIST)->children.size()) != keyQuant)){
                    result = false;
                    if(printErrors){
                        fprintf(stderr, "Error: Constant entry \"");
                        entry->writeCode(stderr);
                        fprintf(stderr, "\" not compatible with the key for table \"%s\" (Line %d - %s)\n", this->str(), entry->lineNumber, programName.c_str());
                    }
                }
            }
        }
    }

    for(AST *c : this->children){
        if(c != NULL){
            bool cResult = c->CheckLegal(printErrors, programName);
            if(!cResult)
                result = false;
        }
    }

    return result;
}

bool AST::SimplifyConstExpressions(){
    for(AST *c : this->children)
        if(c != NULL)
            c->SimplifyConstExpressions();

    bool simplified = false;    
    if(this->nType == NT_EXPRESSION){
        if(this->children.size() == 2 && IPLBase::isInt(this->children[0]->str()) && IPLBase::isInt(this->children[1]->str())){
            const uint64_t v1 = IPLBase::intFromStr64(this->children[0]->str());
            const uint64_t v2 = IPLBase::intFromStr64(this->children[1]->str());

            if(this->value == "%"){
                if(v2 == 0){
                    fprintf(stderr, "Error: Division by zero in expression \"");
                    this->writeCode(stderr);
                    fprintf(stderr, "\"\n");
                    exit(1);
                }
                this->value = std::to_string(v1 % v2);
                simplified = true;
            }
            else if(this->value == "+"){
                this->value = std::to_string(v1 + v2);
                simplified = true;
            }
            else if(this->value == "-"){
                this->value = std::to_string(v1 - v2);
                simplified = true;
            }
            else if(this->value == "*"){
                this->value = std::to_string(v1 * v2);
                simplified = true;
            }
            else if(this->value == "/"){
                if(v2 == 0){
                    fprintf(stderr, "Error: Division by zero in expression \"");
                    this->writeCode(stderr);
                    fprintf(stderr, "\"\n");
                    exit(1);
                }
                this->value = std::to_string(v1 / v2);
                simplified = true;
            }
            else if(this->value == ">>"){
                this->value = std::to_string(v1 >> v2);
                simplified = true;
            }
            else if(this->value == "<<"){
                this->value = std::to_string(v1 << v2);
                simplified = true;
            }
        }
    }

    if(simplified){
        for(AST *c : this->children)
            NullDelete(c);
        this->children.clear();
    }
    
    return simplified;
}

void AST::RemoveErrorAndMKDeclarations(){
    size_t i = 0;
    while(i < this->children.size()){
        if(this->children[i]->nType == NT_ERROR_DECLARATION || this->children[i]->nType == NT_MATCH_KIND_DECLARATION){
            delete this->children[i];
            this->children.erase(this->children.begin() + i);
            i--;
        }
        i++;
    }
}

void AST::RemoveEmptyLists(){
    static const std::unordered_set<NodeType> ListTypes = {NT_STAT_OR_DEC_LIST, NT_PARSER_LOCAL_ELEMENT_LIST, NT_CONTROL_LOCAL_DEC_LIST};
    
    size_t i = 0;
    while(i < this->children.size()){
        AST *child = this->children[i];
        if(child != NULL){
            child->RemoveEmptyLists();
            if(ListTypes.find(child->nType) != ListTypes.end() && child->children.size() == 0){
                delete child;
                this->children[i] = NULL;
            }
        }
        i++;
    }
}

void AST::RemoveConstantDeclarations(){
    size_t i = 0;
    while(i < this->children.size()){
        AST *child = this->children[i];
        if(child != NULL){
            child->RemoveConstantDeclarations();
            if(child->nType == NT_CONSTANT_DECLARATION){
                delete child;
                this->children.erase(this->children.begin() + i);
                i--;
            }
        }
        i++;
    }
}

void AST::ChangeIdsAfterMergeHeaderListsRec(const std::string& oldsf, const std::string& newsf, const std::string& type){
    for(AST *c : this->children)
        if(c != NULL)
            c->ChangeIdsAfterMergeHeaderListsRec(oldsf, newsf, type);

    if(this->value == "." && this->nType != NT_ANNOTATION_TOKEN){
        if(this->children[0]->programType == type && this->children[1]->value == oldsf){
            if(this->children[1]->value != newsf){
                std::string logMSG = "Changed \"" + this->children[1]->value + "\" to \"" + newsf + "\"";
                Log::MSG(logMSG, LOG_LEVEL_ALL);
                this->children[1]->setValue(newsf);
            }
        }
    }
}

void AST::ChangeIdsAfterMergeHeaderLists(AST *oldHeader, AST *newHeader){
    if(oldHeader->getChildNT(NT_STRUCT_FIELD_LIST) == NULL)
        return;

    Assert(newHeader->getChildNT(NT_STRUCT_FIELD_LIST) != NULL);
    Assert(newHeader->getChildNT(NT_STRUCT_FIELD_LIST)->children.size() > 0);
    
    std::vector<bool> matchedFieldsInNewHeader(newHeader->getChildNT(NT_STRUCT_FIELD_LIST)->children.size(), false);  
    for(AST *sf : oldHeader->getChildNT(NT_STRUCT_FIELD_LIST)->children){
        size_t x = 0;
        AST *newHSF = NULL;
        while(x < newHeader->getChildNT(NT_STRUCT_FIELD_LIST)->children.size() && newHSF == NULL){
            if(!matchedFieldsInNewHeader[x] && AST::Equals(newHeader->getChildNT(NT_STRUCT_FIELD_LIST)->children[x]->children[1], sf->children[1]) && AST::Equals(newHeader->getChildNT(NT_STRUCT_FIELD_LIST)->children[x]->children[0], sf->children[0]))
                if(!hasVirtPreventMergeAnnotation(newHeader->getChildNT(NT_STRUCT_FIELD_LIST)->children[x]) || sf->value == newHeader->getChildNT(NT_STRUCT_FIELD_LIST)->children[x]->value) 
                    newHSF = newHeader->getChildNT(NT_STRUCT_FIELD_LIST)->children[x];
            x++;
        }
        Assert(newHSF != NULL);
        matchedFieldsInNewHeader[x - 1] = true;
    
        if(sf->value != newHSF->value)
            this->ChangeIdsAfterMergeHeaderListsRec(sf->value, newHSF->value, newHeader->value);
    }
}

void AST::RenameProgramTypes(const std::string& oldPT, const std::string& newPT){
    if(this->programType == oldPT)
        this->programType = newPT;
    for(AST *c : this->children)
        if(c != NULL)
            c->RenameProgramTypes(oldPT, newPT);
}

void AST::InferTypes(){
    std::map<std::string, std::string> nameToTypes;
    std::map<std::string, std::map<std::string, std::string>> nameToList;
    
    Log::MSG("Infering types on \"" + this->value + "\"", LOG_LEVEL_SHORT);
    Log::Push();
    for(AST *c : this->children)
        if(c != NULL)
            c->InferTypesRec(nameToTypes, nameToList, this->value);
    Log::Pop();
}

void AST::InferTypesAnnotationTokenList(std::map<std::string, std::string>& nameToTypes, std::map<std::string, std::map<std::string, std::string>>& nameToList, const std::string& programName){
    char buffer[BUFFER_SIZE];
    
    Assert(this->nType == NT_ANNOTATION_TOKEN_LIST);
    
    size_t i = 0;
    while(i < this->children.size()){
        AST *token = this->children[i];
        if(token->programType == ""){
            auto it = nameToTypes.find(token->value);
            if(it != nameToTypes.end()){
                sprintf(buffer, "Line %d: \"%s\" is \"%s\"", token->lineNumber, token->str(), AST::RemoveMergePrefix(it->second).c_str());
                Log::MSG(std::string(buffer), LOG_LEVEL_ALL);
                token->programType = it->second;
            }
        }

        if(token->programType != ""){
            if(i + 2 < this->children.size() && this->children[i+1]->value == "."){
                AST *nextToken = this->children[i+2];
                if(nextToken->programType == ""){
                    auto it = nameToList.find(token->programType);
                    if(it != nameToList.end()){
                        auto it2 = it->second.find(nextToken->value);
                        if(it2 != it->second.end()){
                            sprintf(buffer, "Line %d: \"%s\" is \"%s\"", nextToken->lineNumber, nextToken->str(), AST::RemoveMergePrefix(it2->second).c_str());
                            Log::MSG(std::string(buffer), LOG_LEVEL_ALL);
                            nextToken->programType = it2->second;
                        }
                    }
                }
                if(token->programType != "" && nameToList.find(token->programType) != nameToList.end() && this->children.size() > i + 2){
                    this->children[i+2]->value = token->programType + "_" + this->children[i+2]->value;
                }
                i += 1;
            }
        }
        i++;
    }
}

void AST::InferTypesRec(std::map<std::string, std::string>& nameToTypes, std::map<std::string, std::map<std::string, std::string>>& nameToList, const std::string& programName){
    char buffer[BUFFER_SIZE];

    switch(this->nType){
        case NT_ANNOTATION_TOKEN_LIST:
            this->InferTypesAnnotationTokenList(nameToTypes, nameToList, programName);
            break;
        case NT_EXPRESSION:
        case NT_NONE:
            {
            auto it = nameToTypes.find(this->value);
            if(it != nameToTypes.end()){
                sprintf(buffer, "Line %d: \"%s\" is \"%s\"", this->lineNumber, this->str(), AST::RemoveMergePrefix(it->second).c_str());
                Log::MSG(std::string(buffer), LOG_LEVEL_ALL);
                this->programType = it->second;
            }
            else if(this->value == "."){
                this->children[0]->InferTypesRec(nameToTypes, nameToList, programName);
                if(this->children[1] != NULL && this->children[1]->nType == NT_NONE){
                    auto it = nameToList.find(this->children[0]->programType);
                    if(it != nameToList.end()){
                        auto it2 = it->second.find(this->children[1]->value);
                        if(it2 != it->second.end()){
                            sprintf(buffer, "Line %d: \"%s\" is \"%s\"", this->lineNumber, this->getValueExpression().c_str(), AST::RemoveMergePrefix(it2->second).c_str());
                            Log::MSG(std::string(buffer), LOG_LEVEL_ALL);
                            this->programType = it2->second;
                            this->children[1]->programType = it2->second;
                        }
                    }
                    if(this->children[0]->programType != "" && nameToList.find(this->children[0]->programType) != nameToList.end() && this->children.size() == 2){
                        this->children[1]->value = this->children[0]->programType + "_" + this->children[1]->value;
                    }
                }
                if(this->children.size() > 2){
                    for(int i = 2; i < (int) this->children.size(); i++){
                        if(this->children[i] != NULL)
                            this->children[i]->InferTypesRec(nameToTypes, nameToList, programName);
                    }
                    if(this->children[1]->value == "lookahead"){
                        if(this->children[2]->nType == NT_TYPE_ARGUMENT_LIST && this->children[2]->children.size() == 1){
                            this->programType = this->children[2]->children[0]->value;
                        }
                    }
                }
            }
            else if(this->value == "[]"){
                this->children[0]->InferTypesRec(nameToTypes, nameToList, programName);
                if((this->children[0]->programType.find(HEADER_STACK_PROGRAM_TYPE_SUFFIX) == this->children[0]->programType.size() - strlen(HEADER_STACK_PROGRAM_TYPE_SUFFIX)) && this->children[1] != NULL && (this->children[1]->nType == NT_LITERAL || this->children[1]->nType == NT_EXPRESSION) && IPLBase::isInt(this->children[1]->str()))
                    this->programType = this->children[0]->programType.substr(0, this->children[0]->programType.find(HEADER_STACK_PROGRAM_TYPE_SUFFIX));
            }
            else{
                for(AST *c : this->children)
                    if(c != NULL)
                        c->InferTypesRec(nameToTypes, nameToList, programName);
            }
            }
            break;
        case NT_INSTANTIATION:
            this->programType = this->getChildNT(NT_TYPE_NAME)->value;
            nameToTypes.emplace(std::make_pair(this->value, this->getChildNT(NT_TYPE_NAME)->value));
            break;
        case NT_FUNCTION_DECLARATION:
        case NT_PARSER_DECLARATION:
        case NT_CONTROL_DECLARATION:
            {
            //Variables shadowed by this scope
            std::vector<std::pair<std::string, std::string>> shadowedTypes;

            //Variables added for this scope only
            std::vector<std::pair<std::string, std::string>> newTypes;

            sprintf(buffer, "Block \"%s\"", AST::RemoveMergePrefix(this->children[0]->value).c_str());
            Log::MSG(std::string(buffer), LOG_LEVEL_ALL);
            Log::Push();

            if(this->children[0]->children[2] != NULL){
                for(AST *parameter : this->children[0]->children[2]->children){
                    if(parameter->children[2]->nType == NT_TYPE_NAME){
                        auto it = nameToTypes.find(parameter->value);
                        if(it != nameToTypes.end()){
                            shadowedTypes.push_back(std::make_pair(it->first, it->second));
                            nameToTypes.erase(it);
                        }
                        nameToTypes.emplace(parameter->value, parameter->children[2]->value);
                        newTypes.push_back(std::make_pair(parameter->value, parameter->children[2]->value));
                    }
                }
            }

            for(AST *c : this->children)
                if(c != NULL)
                    c->InferTypesRec(nameToTypes, nameToList, programName);

            Log::Pop();

            for(std::pair<std::string, std::string> p : newTypes){
                nameToTypes.erase(nameToTypes.find(p.first));    
            }
            for(std::pair<std::string, std::string> p : shadowedTypes)
                nameToTypes.emplace(p);
            }
            break;
        case NT_HEADER_TYPE_DECLARATION:
        case NT_STRUCT_TYPE_DECLARATION:
            {
            Log::MSG("Changing fields of struct / header \"" + this->value + "\"", LOG_LEVEL_ALL);
            Log::Push();
            std::map<std::string, std::string> newMap;
            if(this->children[2] != NULL){
                for(int i = 0; i < (int) this->children[2]->children.size(); i++){
                    AST *type = this->children[2]->children[i]->children[1];
                    if(type != NULL){
                        if(type->nType == NT_TYPE_NAME)
                            newMap.emplace(this->children[2]->children[i]->value, type->value);
                        else if(type->nType == NT_BASE_TYPE){
                            newMap.emplace(this->children[2]->children[i]->value, AST::BaseTypeToString(type));
                        }
                        else if(type->nType == NT_HEADER_STACK_TYPE){
                            Assert(type->getChildNT(NT_TYPE_NAME) != NULL);
                            newMap.emplace(this->children[2]->children[i]->value, type->getChildNT(NT_TYPE_NAME)->value + HEADER_STACK_PROGRAM_TYPE_SUFFIX);
                        }

                        const std::string newValue = this->value + "_" + this->children[2]->children[i]->value;
                        sprintf(buffer, "%s -> %s", this->children[2]->children[i]->value.c_str(), newValue.c_str());
                        Log::MSG(std::string(buffer), LOG_LEVEL_ALL);
                        this->children[2]->children[i]->value = newValue;
                    }
                }
            }
            Log::Pop();
            nameToList.emplace(this->value, newMap);
            }
            break;
        case NT_METHOD_CALL_STATEMENT:
            for(AST *c : this->children){
                if(c != NULL){
                    if(c->value != ".")
                        c->InferTypesRec(nameToTypes, nameToList, programName);
                    else 
                        c->children[0]->InferTypesRec(nameToTypes, nameToList, programName);
                }
            }
            break;
        default:
            for(AST *c : this->children)
                if(c != NULL)
                    c->InferTypesRec(nameToTypes, nameToList, programName);
            break;
    }
}

void AST::AddPreventMergeAnnotationBasedOnMergedEmit(int emitNum, const std::string& deparserName){
    Assert(this->nType == NT_PROGRAM);

    AST *deparser = NULL;
    for(AST *c : this->children)
        if(c->nType == NT_CONTROL_DECLARATION && c->value == deparserName)
            deparser = c;
    
    Assert(deparser != NULL);

    AST *parameterList = deparser->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST);
    std::string PacketOutParamName;
    for(AST *parameter : parameterList->children)
        if(parameter->getChildNT(NT_TYPE_NAME) != NULL && parameter->getChildNT(NT_TYPE_NAME)->value == "packet_out")
            PacketOutParamName = parameter->value;

    Assert(PacketOutParamName != "");

    std::vector<AST*> emits;
    AST *SODL = deparser->getChildNT(NT_STAT_OR_DEC_LIST);
    for(AST *c : SODL->children){
        if(c->nType == NT_METHOD_CALL_STATEMENT){
            if(c->children[0]->value == "." && c->children[0]->children[0]->value == PacketOutParamName && c->children[0]->children[1]->value == "emit"){
                emits.push_back(c);
            }
        }
    }

    Assert(emitNum < (int) emits.size());

    bool done = false;
    while(emitNum >= 0 && !done){
        AST *emit = emits[emitNum];

        std::string headerName;
        std::string fieldName;
        AST *ArgumentList = emit->getChildNT(NT_ARGUMENT_LIST);
        if(ArgumentList != NULL){
            Assert(ArgumentList->children.size() == 1);
            Assert(ArgumentList->children[0]->children[0]->value == ".");
            headerName = ArgumentList->children[0]->children[0]->children[0]->programType;
            fieldName = ArgumentList->children[0]->children[0]->children[1]->value;
        }
        Assert(headerName != "" && fieldName != "");

        AST *header = NULL;
        for(AST *c : this->children)
            if((c->nType == NT_HEADER_TYPE_DECLARATION || c->nType == NT_STRUCT_TYPE_DECLARATION) && c->value == headerName)
                header = c;
        Assert(header != NULL);

        AST *SFL = header->getChildNT(NT_STRUCT_FIELD_LIST);
        Assert(SFL != NULL);
        for(size_t i = 0; i < SFL->children.size(); i++){
            AST *SF = SFL->children[i];
            if(SF->value == fieldName){
                if(hasVirtPreventMergeAnnotation(SF) == false){
                    if(SF->children[0] == NULL)
                        SF->children[0] = new AST("", NT_ANNOTATION_LIST);

                    AST *annotation = new AST(VIRT_PREVENT_MERGE_ANNOTATION, NT_ANNOTATION);
                    annotation->addChild(NULL);
                    SF->children[0]->addChild(annotation);
                    done = true;
                }
            }
        }

        emitNum--;
    }
    Assert(emitNum >= 0 || done);
}

void AST::SubstituteIdentifierWithType(std::string oldID, const std::string& newID, const std::string& type, bool skipMemberAccesses){
    if((this->nType == NT_NONE || this->nType == NT_EXPRESSION || this->nType == NT_ACTION_REF || this->nType == NT_ACTION_DECLARATION || this->nType == NT_TABLE_DECLARATION || this->nType == NT_VARIABLE_DECLARATION || this->nType == NT_INSTANTIATION || this->nType == NT_ANNOTATION_TOKEN) && this->value == oldID && this->programType == type){
        this->value = newID;
    }
    if(skipMemberAccesses && this->value == "."){
        Assert(this->children[0] != NULL);
        this->children[0]->SubstituteIdentifierWithType(oldID, newID, type, skipMemberAccesses);
    }
    else{
        for(AST *c : this->children)
            if(c != NULL)
                c->SubstituteIdentifierWithType(oldID, newID, type, skipMemberAccesses);
    }
}

void AST::SubstituteIdentifierWithType(std::string oldID, const std::string& newID, const std::string& type){
    this->SubstituteIdentifierWithType(oldID, newID, type, false);
}

void AST::SubstituteStructFieldIdentifier(std::string oldID, const std::string& newID, const std::string& structTypeName){
    if(this->nType == NT_EXPRESSION && this->value == "." && this->children[0]->programType == structTypeName && this->children[1]->value == oldID){
        this->children[1]->value = newID;
    }
    else if(this->nType == NT_ANNOTATION_TOKEN_LIST){
        for(size_t i = 1; i < this->children.size() - 1; i++){
            if(this->children[i]->value == "." && this->children[i-1]->programType == structTypeName && this->children[i+1]->value == oldID){
                this->children[i+1]->value = newID;
            }
        }
    }
    for(AST *c : this->children)
        if(c != NULL)
            c->SubstituteStructFieldIdentifier(oldID, newID, structTypeName);
}

bool ASTCompatibleParemeterLists(AST *pList, AST *aList){

    if(pList == NULL)
        return aList == NULL;

    if(aList != NULL && aList->getChildren().size() > pList->getChildren().size())
        return false;
    
    size_t firstDefaultValue = 0;
    while(firstDefaultValue < pList->getChildren().size() && pList->getChildren()[firstDefaultValue]->getChildren().size() < 4)
        firstDefaultValue++;
    
    if(aList == NULL)
        return firstDefaultValue == 0;

    return aList->getChildren().size() >= firstDefaultValue;
}

std::vector<AST*> ASTFillArgumentsList(AST *pList, AST *aList){
    std::vector<AST*> result;

    size_t pi = 0;
    if(aList != NULL){
        pi = aList->getChildren().size();
        for(size_t i = 0; i < aList->getChildren().size(); i++){
            result.push_back(aList->getChildren()[i]->getChildren()[0]);
        }
    }

    if(pList != NULL){
        while(pi < pList->getChildren().size()){
            Assert(pList->getChildren()[pi]->getChildren().size() == 4);
            result.push_back(pList->getChildren()[pi]->getChildren()[3]);
            pi++;
        }
    }

    return result;
}

void AST::SubstituteParameter(const std::string& paramName, AST *newAST){
    for(size_t i = 0; i < this->children.size(); i++){
        if(this->children[i] != NULL){
            this->children[i]->SubstituteParameter(paramName, newAST);
            if(this->children[i]->getValue() == paramName && (this->value != "." || i == 0)){ //Only substitute the first element of a expression
                delete this->children[i];
                this->children[i] = newAST->Clone();
            }
        }
    }
}

AST* AST::CreateParserInstanceApplyStateList(const std::string& prefix, const std::string& suffix, const std::string& acceptState, AST *argumentList){
    Assert(this->getChildNT(NT_PARSER_STATE_LIST) != NULL);
    AST *myParameterList = this->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST);
    Assert((myParameterList == NULL && argumentList == NULL) || myParameterList->children.size() == argumentList->children.size());

    AST* result = this->getChildNT(NT_PARSER_STATE_LIST)->Clone();
    
    if(myParameterList != NULL){
        for(size_t i = 0; i < myParameterList->children.size(); i++){
            result->SubstituteParameter(myParameterList->children[i]->value, argumentList->children[i]->children[0]);
        }
    }

    for(AST *state : result->children){
        state->value = prefix + state->value + suffix;
        if(state->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL && state->getChildNT(NT_NAME_STATE_EXPRESSION)->value != "accept" && state->getChildNT(NT_NAME_STATE_EXPRESSION)->value != "reject")
            state->getChildNT(NT_NAME_STATE_EXPRESSION)->value = prefix + state->getChildNT(NT_NAME_STATE_EXPRESSION)->value + suffix;
        if(state->getChildNT(NT_SELECT_EXPRESSION) != NULL && state->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_SELECT_CASE_LIST) != NULL){
            for(AST *sc : state->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_SELECT_CASE_LIST)->children){
                if(sc->value != "accept" && sc->value != "reject"){
                    sc->value = prefix + sc->value + suffix;
                }
            }
        }
    }

    result->SubstituteValue("accept", acceptState);
    return result;
}

AST* AST::CreateParserInstance(AST *instantiation, const std::string& prefix){
    AST *root = instantiation->getRoot();
    AST *parserDeclaration = NULL;
    int ci = 0;
    while(ci < (int) root->children.size()){
        if(root->children[ci] != NULL && root->children[ci]->nType == NT_PARSER_DECLARATION && root->children[ci]->getChildNT(NT_PARSER_TYPE_DECLARATION)->value == instantiation->getChildNT(NT_TYPE_NAME)->value)
            parserDeclaration = root->children[ci];
        ci++;
    }

    if(parserDeclaration == NULL)
        return NULL;
    
    if(!ASTCompatibleParemeterLists(parserDeclaration->getChildNT(NT_PARAMETER_LIST), instantiation->getChildNT(NT_ARGUMENT_LIST))){
        fprintf(stderr, "Error: Wrong arguments in instantiation constructor of instance \"%s\" (\"%s\" - line %d)\n", instantiation->str(), root->str(), instantiation->lineNumber);
        exit(1);
    }
    
    AST *instance = parserDeclaration->Clone();

    if(instance->getChildNT(NT_PARAMETER_LIST) != NULL){
        for(int i = 0; i < (int) instance->getChildNT(NT_PARAMETER_LIST)->children.size(); i++){
            if(instance->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST) != NULL)
                instance->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->SubstituteParameter(instance->getChildNT(NT_PARAMETER_LIST)->children[i]->value, instantiation->getChildNT(NT_ARGUMENT_LIST)->children[i]->children[0]);
            if(instance->getChildNT(NT_PARSER_STATE_LIST) != NULL)
                instance->getChildNT(NT_PARSER_STATE_LIST)->SubstituteParameter(instance->getChildNT(NT_PARAMETER_LIST)->children[i]->value, instantiation->getChildNT(NT_ARGUMENT_LIST)->children[i]->children[0]);
        }
    }
    instance->RemoveInstantiationsInParser(root);
    
    if(instance->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST) != NULL){
        for(AST *le : instance->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->children){
            std::string oldV = le->value;
            instance->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->SubstituteIdentifierWithType(oldV, prefix + oldV, le->programType);
            if(instance->getChildNT(NT_PARSER_STATE_LIST) != NULL)
                instance->getChildNT(NT_PARSER_STATE_LIST)->SubstituteIdentifierWithType(oldV, prefix + oldV, le->programType);
        }
    }
    
    return instance;
}

AST* AST::CreateControlInstance(AST *root, AST *instantiation, const std::string& prefix){
    AST *controlDeclaration = NULL;
    int ci = 0;
    while(ci < (int) root->children.size()){
        if(root->children[ci] != NULL && root->children[ci]->nType == NT_CONTROL_DECLARATION && root->children[ci]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value == instantiation->getChildNT(NT_TYPE_NAME)->value)
            controlDeclaration = root->children[ci];
        ci++;
    }
    if(controlDeclaration != NULL){
        if(!ASTCompatibleParemeterLists(controlDeclaration->getChildNT(NT_PARAMETER_LIST), instantiation->getChildNT(NT_ARGUMENT_LIST))){
            fprintf(stderr, "Error: Wrong arguments in instantiation constructor of instance \"%s\" (\"%s\" - line %d)\n", instantiation->str(), root->str(), instantiation->lineNumber);
            exit(1);
        }
        
        AST *instance = controlDeclaration->Clone();
        
        std::vector<AST*> totalArguments = ASTFillArgumentsList(controlDeclaration->getChildNT(NT_PARAMETER_LIST), instantiation->getChildNT(NT_ARGUMENT_LIST));

        if(instance->getChildNT(NT_PARAMETER_LIST) != NULL){
            for(int i = 0; i < (int) instance->getChildNT(NT_PARAMETER_LIST)->children.size(); i++){
                if(instance->getChildNT(NT_CONTROL_LOCAL_DEC_LIST) != NULL)
                    instance->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->SubstituteParameter(instance->getChildNT(NT_PARAMETER_LIST)->children[i]->value, totalArguments[i]);
                if(instance->getChildNT(NT_STAT_OR_DEC_LIST) != NULL)
                    instance->getChildNT(NT_STAT_OR_DEC_LIST)->SubstituteParameter(instance->getChildNT(NT_PARAMETER_LIST)->children[i]->value, totalArguments[i]);
            }
        }
        instance->RemoveInstantiationsInControl(root);

        if(instance->getChildNT(NT_CONTROL_LOCAL_DEC_LIST) != NULL){
            for(AST *le : instance->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->children){
                std::string oldV = le->value;
                instance->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->SubstituteIdentifierWithType(oldV, prefix + oldV, le->programType, true);
                if(instance->getChildNT(NT_STAT_OR_DEC_LIST) != NULL){
                    instance->getChildNT(NT_STAT_OR_DEC_LIST)->SubstituteIdentifierWithType(oldV, prefix + oldV, le->programType, true);
                }
            }
        }

        return instance;
    }

    return NULL;
}

void AST::RemoveInstantiationsInParser(AST *root){
    char buffer[BUFFER_SIZE];

    std::unordered_map<std::string, AST*> instanceMap;

    if(this->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST) != NULL){
        size_t lei = 0;
        while(lei < this->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->children.size()){
            AST *instantiation = this->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->children[lei];
            if(instantiation != NULL && instantiation->nType == NT_INSTANTIATION && instantiation->getChildNT(NT_TYPE_NAME) != NULL){
                sprintf(buffer, "VirtInstance___%s___", instantiation->str());
                AST *instance = AST::CreateParserInstance(instantiation, std::string(buffer));
                if(instance != NULL){
                    instanceMap.emplace(std::make_pair(instantiation->value, instance));
                    delete instantiation;
                    this->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->children.erase(this->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->children.begin() + lei);
                    lei--;
                }
            }
            lei++;
        }
    }

    int instanceN = 0;
    AST *stateList = this->getChildNT(NT_PARSER_STATE_LIST);
    if(stateList != NULL){
        size_t sp = 0;
        while(sp < stateList->children.size()){
            AST *state = stateList->children[sp];
            if(state->getChildNT(NT_PARSER_STATEMENT_LIST) != NULL){
                size_t si = 0;
                while(si < state->getChildNT(NT_PARSER_STATEMENT_LIST)->children.size()){
                    AST *statement = state->getChildNT(NT_PARSER_STATEMENT_LIST)->children[si];
                    if(statement->nType == NT_METHOD_CALL_STATEMENT && statement->children[0]->value == "." && statement->children[0]->children.size() == 2){
                        auto it = instanceMap.find(statement->children[0]->children[0]->value);
                        if(it != instanceMap.end()){
                            if((statement->children[2] == NULL && it->second->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST) != NULL) || statement->children[2]->children.size() != it->second->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children.size()){
                                fprintf(stderr, "Error: Incorrect arguments in apply method call (\"%s\" - Line %d)\n", root->str(), statement->lineNumber);
                                exit(1);
                            }

                            sprintf(buffer, "Expanding parser apply statement in line %d", statement->lineNumber);
                            Log::MSG(std::string(buffer), LOG_LEVEL_SHORT);
                            
                            sprintf(buffer, "_%d", instanceN);
                            instanceN++;
                            const std::string nextPartName = state->value + "_vsplit";
                            AST *newStates = it->second->CreateParserInstanceApplyStateList(it->first + "_", std::string(buffer), nextPartName, statement->children[2]);
                            
                            //Splits this state in two
                            AST *state2 = state->Clone();

                            delete state->children[state->children.size() - 1];
                            state->children[state->children.size() - 1] = new AST(it->first + "_start" + std::string(buffer), NT_NAME_STATE_EXPRESSION);

                            for(size_t sii = si; sii < state->getChildNT(NT_PARSER_STATEMENT_LIST)->children.size(); sii++)
                                delete state->getChildNT(NT_PARSER_STATEMENT_LIST)->children[sii];
                            for(size_t sii = 0; sii < si + 1; sii++)
                                delete state2->getChildNT(NT_PARSER_STATEMENT_LIST)->children[sii];
                            
                            state->getChildNT(NT_PARSER_STATEMENT_LIST)->children.erase(state->getChildNT(NT_PARSER_STATEMENT_LIST)->children.begin() + si, state->getChildNT(NT_PARSER_STATEMENT_LIST)->children.end());
                            state2->getChildNT(NT_PARSER_STATEMENT_LIST)->children.erase(state2->getChildNT(NT_PARSER_STATEMENT_LIST)->children.begin(), state2->getChildNT(NT_PARSER_STATEMENT_LIST)->children.begin() + si + 1);
                            state2->value = nextPartName;
                            newStates->addChild(state2);

                            Log::Push();
                            for(AST *newS : newStates->children)
                                Log::MSG("Created state \"" + newS->value + "\"", LOG_LEVEL_SHORT);
                            Log::Pop();

                            //Adds new states to the list
                            for(size_t x = 0; x < newStates->children.size(); x++)
                                stateList->children.push_back(NULL);
                            size_t sp2 = stateList->children.size() - 1 - newStates->children.size();
                            while(sp2 > sp){
                                stateList->children[sp2 + newStates->children.size()] = stateList->children[sp2];
                                sp2--;
                            }
                            for(size_t x = 0; x < newStates->children.size(); x++)
                                stateList->children[sp + 1 + x] = newStates->children[x]->Clone();
                            
                            si = si + newStates->children.size();

                            delete newStates;
                        }
                    }
                    si++;
                }
            }
            sp++;
        }

        sp = 0;
        while(sp < stateList->children.size()){
            AST *state = stateList->children[sp];
            size_t x = 0;
            while(x < state->children.size()){
                if(state->children[x] != NULL && state->children[x]->nType == NT_PARSER_STATEMENT_LIST && state->children[x]->children.size() == 0){
                    delete state->children[x];
                    state->children[x] = NULL;
                    x--;
                }
                x++;
            }
            sp++;
        }
    }

    for(auto instance : instanceMap){
        if(instance.second->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST) != NULL){
            for(AST *le : instance.second->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->children){
                this->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->addChild(le->Clone());
            }
        }
        delete instance.second;
    }
}

void AST::RemoveInstantiationsInControlRec(AST *root, const std::unordered_map<std::string, AST*>& instanceMap, AST *applyStatements){    
    size_t i = 0;
    while(i < applyStatements->children.size()){
        AST *c = applyStatements->children[i];
        if(c != NULL){
            RemoveInstantiationsInControlRec(root, instanceMap, c);
            if(c->getNodeType() == NT_METHOD_CALL_STATEMENT){
                if(c->children[0]->value == "." && c->children[0]->children.size() == 2 && c->children[0]->children[1]->getValue() == "apply"){
                    auto it = instanceMap.find(c->children[0]->children[0]->getValue());
                    if(it != instanceMap.end()){
                        if((c->children[2] == NULL && it->second->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST) != NULL) || c->children[2]->children.size() != it->second->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children.size()){
                            fprintf(stderr, "Error: Incorrect arguments in apply method call (\"%s\" - line \"%d\")\n", root->str(), c->lineNumber);
                            exit(1);
                        }
                        
                        AST *newApply = it->second->CreateControlInstanceApplyStatements(c->children[2]);
                        
                        delete applyStatements->children[i];
                        applyStatements->children[i] = newApply;
                     }
                }
            }
        }
        i++;
    }
}

AST* AST::CreateControlInstanceApplyStatements(AST *argumentList){
    AST *result = NULL_CLONE(this->getChildNT(NT_STAT_OR_DEC_LIST));
    AST *myParameterList = this->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST);
    if(result == NULL)
        return result;

    if(myParameterList != NULL){
        for(size_t i = 0; i < myParameterList->children.size(); i++){
            if(this->getChildNT(NT_CONTROL_LOCAL_DEC_LIST) != NULL)
               this->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->SubstituteParameter(myParameterList->children[i]->value, argumentList->children[i]->children[0]);
            result->SubstituteParameter(myParameterList->children[i]->value, argumentList->children[i]->children[0]);
        }
    }

    return result;
}

void AST::RemoveInstantiationsInControl(AST *root){
    char buffer[BUFFER_SIZE];

    std::unordered_map<std::string, AST*> instanceMap;

    //Create instances
    if(this->getChildNT(NT_CONTROL_LOCAL_DEC_LIST) != NULL){
        size_t lei = 0;
        while(lei < this->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->children.size()){
            AST *instantiation = this->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->children[lei];
            if(instantiation != NULL && instantiation->nType == NT_INSTANTIATION && instantiation->getChildNT(NT_TYPE_NAME) != NULL){
                sprintf(buffer, "VirtInstance___%s___", instantiation->str());
                AST *instance = AST::CreateControlInstance(root, instantiation, std::string(buffer));
                if(instance != NULL){
                    instanceMap.emplace(std::make_pair(instantiation->value, instance));
                    delete instantiation;
                    this->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->children.erase(this->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->children.begin() + lei);
                    lei--;
                }
            }
            lei++;
        }
    }

    //Remove instances apply calls
    AST *applyStatements = this->getChildNT(NT_STAT_OR_DEC_LIST);
    if(applyStatements != NULL){
        AST::RemoveInstantiationsInControlRec(root, instanceMap, applyStatements);
    }

    //Adds local elements of instances to this control block
    for(auto instance : instanceMap){
        if(instance.second->getChildNT(NT_CONTROL_LOCAL_DEC_LIST) != NULL)
            for(AST *le : instance.second->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->children)
                this->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->addChild(le->Clone());
        delete instance.second;
    }
}

void AST::ExpandEmitsInControlBlock(const std::unordered_map<std::string, AST*>& structs){
    Assert(this->nType == NT_CONTROL_DECLARATION);

    AST *applyBlock = this->getChildNT(NT_STAT_OR_DEC_LIST);
    if(applyBlock != NULL){
        size_t i = 0;
        while(i < applyBlock->children.size()){
            AST *statement = applyBlock->children[i];
            if(statement->nType == NT_METHOD_CALL_STATEMENT){
                if(statement->children[0]->value == "." && statement->children[0]->children[0]->programType == "packet_out" && statement->children[0]->children[1]->value == "emit"){
                    AST *param = statement->getChildNT(NT_ARGUMENT_LIST)->getChildNT(NT_ARGUMENT)->children[0];
                    auto it = structs.find(param->programType);
                    if(it != structs.end()){
                        for(AST *sf : it->second->getChildNT(NT_STRUCT_FIELD_LIST)->children){
                            AST *newEmit = statement->Clone();
                            
                            AST *argumentExpression = new AST(".", NT_EXPRESSION);
                            argumentExpression->addChild(new AST(param->value, NT_EXPRESSION));
                            argumentExpression->addChild(new AST(sf->value, NT_NONE));
                            argumentExpression->children[0]->programType = param->programType;
                            argumentExpression->children[1]->programType = sf->children[1]->nType == NT_BASE_TYPE ? AST::BaseTypeToString(sf->children[1]) : sf->children[1]->value;
                            argumentExpression->programType = argumentExpression->children[1]->programType;

                            AST *argument = new AST("", NT_ARGUMENT);
                            argument->addChild(argumentExpression);
                            
                            delete newEmit->getChildNT(NT_ARGUMENT_LIST)->children[0];
                            newEmit->getChildNT(NT_ARGUMENT_LIST)->children[0] = argument;
                            argument->parent = newEmit->getChildNT(NT_ARGUMENT_LIST);
                        
                            applyBlock->children.insert(applyBlock->children.begin() + i, newEmit);
                            newEmit->parent = applyBlock;
                            i++;
                        }

                        delete applyBlock->children[i];
                        applyBlock->children.erase(applyBlock->children.begin() + i);
                        i--;
                    }
                }
            }
            i++;
        }
    }
}

void AST::ExpandEmits(){
    std::unordered_map<std::string, AST*> structs;

    for(AST *child : this->children)
        if(child->nType == NT_STRUCT_TYPE_DECLARATION)
            structs.emplace(child->value, child);
    
    for(AST *child : this->children){
        if(child->nType == NT_CONTROL_DECLARATION){
            bool hasPacketOut = false;
            AST *parameterList = child->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST);
            Assert(parameterList != NULL);

            for(AST *parameter : parameterList->children){
                AST *typeName = parameter->getChildNT(NT_TYPE_NAME);
                if(typeName != NULL && typeName->value == "packet_out")
                    hasPacketOut = true;
            }

            if(hasPacketOut)
                child->ExpandEmitsInControlBlock(structs);
        }
    }
}

void AST::RemoveDeclarationsFromIncludesByAnotation(const std::vector<std::string>& includes){  
    size_t i = 0;
    while(i < this->children.size()){
        AST *declaration = this->children[i];
        if(declaration->nType == NT_PARSER_DECLARATION || declaration->nType == NT_CONTROL_DECLARATION)
            declaration = declaration->children[0];
        AST *annotationList = declaration->getChildNT(NT_ANNOTATION_LIST);
        if(annotationList != NULL){
            size_t j = 0;
            bool found = false;
            while(j < annotationList->children.size()){
                AST *annotation = annotationList->children[j];
                if(annotation->value == "VirtIncludedDeclaration"){
                    Assert(annotation->children[0] != NULL && annotation->children[0]->children.size() == 1);
                    std::string headerName = IPLBase::split(annotation->children[0]->children[0]->value, '/').back();
                    headerName.pop_back(); // '\"'

                    for(size_t k = 0; k < includes.size(); k++)
                        if(includes[k] == headerName)
                            found = true;
                    if(found){
                        delete this->children[i];
                        this->children.erase(this->children.begin() + i);
                        i--;
                        break;
                    }
                    else{
                        delete annotation;
                        annotationList->children.erase(annotationList->children.begin() + j);
                        j--;
                    }
                }
                j++;
            }
            if(!found && annotationList->children.size() == 0){
                Assert(annotationList == declaration->children[0]);
                delete declaration->children[0];
                declaration->children[0] = NULL;
            }
        }
        i++;
    }
}

void AST::RemoveSwitchStatements(){
    size_t i = 0;
    while(i < this->children.size()){
        if(this->children[i] != NULL){
            if(this->children[i]->nType == NT_SWITCH_STATEMENT){
                AST* SCL = this->children[i]->getChildNT(NT_SWITCH_CASE_LIST);
                
                AST *newStatement = NULL;
                
                if(SCL != NULL && SCL->children.size() > 0){
                    AST *condition = new AST("==", NT_EXPRESSION);
                    condition->addChild(this->children[i]->children[0]->Clone());

                    AST *currentIF = NULL;
                    std::vector<AST*> conditionsInOr;
                    for(size_t ci = 0; ci < SCL->children.size(); ci++){
                        conditionsInOr.push_back(SCL->children[ci]->children[0]->Clone());
                        if(SCL->children[ci]->children.size() > 1){
                            AST *conditionsWithOr = condition->Clone();
                            conditionsWithOr->addChild(conditionsInOr[0]);

                            for(size_t cio = 1; cio < conditionsInOr.size(); cio++){
                                AST *newCWO = new AST("||", NT_EXPRESSION);
                                newCWO->addChild(conditionsWithOr);
                                newCWO->addChild(condition->Clone());
                                newCWO->children[1]->addChild(conditionsInOr[cio]);
                                conditionsWithOr = newCWO;
                            }

                            AST *newStatement2 = new AST("IF", NT_CONDITIONAL_STATEMENT);
                            newStatement2->addChild(conditionsWithOr);
                            newStatement2->addChild(SCL->children[ci]->children.size() > 1 ? NULL_CLONE(SCL->children[ci]->children[1]) : NULL);
                            newStatement2->addChild(NULL);
                            if(newStatement == NULL){
                                newStatement = newStatement2;
                            }
                            else{
                                currentIF->children[2] = newStatement2;
                            }
                            currentIF = newStatement2;
                            conditionsInOr.clear();
                        }
                    }

                    delete condition;
                }

                delete this->children[i];
                if(newStatement != NULL){
                    this->children[i] = newStatement;
                }
                else{
                    this->children.erase(this->children.begin() + i);
                    i--;
                }
            }
            else{
                this->children[i]->RemoveSwitchStatements();
            }
        }
        i++;
    }
}

void AST::RemoveInstantiations(){
    for(AST *c : this->children){
        if(c != NULL){
            if(c->nType == NT_PARSER_DECLARATION)
                c->RemoveInstantiationsInParser(this);
            else if(c->nType == NT_CONTROL_DECLARATION)
                c->RemoveInstantiationsInControl(this);
        }
    }
    this->RemoveUnusedBlocks();
    this->RemoveUnusedHeaders();
}

void AST::SubstituteTypedef(){
    std::unordered_map<std::string, AST*> table;
    this->SubstituteTypedefRec(&table);
    this->RemoveTypedefRec(&table);
}

void AST::SubstituteTypedefRec(std::unordered_map<std::string, AST*> *table){
    if(this->nType == NT_TYPEDEF_DECLARATION){
       AST* entry;
       entry = this->children[1];

       table->emplace(this->value, entry);
    }
    else if(this->nType == NT_TYPE_NAME){
        auto it = table->find(this->value);
        AST* entry = it == table->end() ? NULL : it->second;
        if(entry != NULL){
            this->nType = entry->nType;
            this->value = entry->value;
            for(AST *c : this->children)
                delete c;
            this->children.resize(entry->children.size(), NULL);
            for(int i = 0; i < (int) entry->children.size(); i++){
                this->children[i] = entry->children[i]->Clone();
            }    
        }
    }

    for(AST *c : this->children){
        if(c != NULL){
            c->SubstituteTypedefRec(table);
        }
    }
}

void AST::RemoveTypedefRec(std::unordered_map<std::string, AST*> *table){
    int i = 0;
    while(i < (int) this->children.size()){
        AST *c = this->children[i];
        if(c != NULL){
            if(c->nType == NT_TYPEDEF_DECLARATION){
                delete c;
                this->children.erase(this->children.begin() + i);
                i--;
            }
            else{
                c->RemoveTypedefRec(table);
            }
        }
        i++;
    }
}

void AST::writeCode(FILE *fp) const{
    static const std::unordered_set<std::string> BinaryOps({".", ":", "-", "+", "*", "/", "%", "&", "^", "|", "==", "!=", "&&", "||", "<", ">", "<=", ">=", "<<", ">>", "++", "|+|", "|-|"});
    static const std::unordered_set<std::string> BinaryOpsSpace({     "-", "+", "*", "/", "%", "&", "^", "|", "==", "!=", "&&", "||", "<", ">", "<=", ">=", "<<", ">>", "++", "|+|", "|-|"});

    switch(this->nType){
        case NT_PROGRAM:
            for(AST *c : this->children){
                if(c != NULL){
                    c->writeCode(fp);
                    if(c->nType == NT_PARSER_TYPE_DECLARATION || c->nType == NT_CONTROL_TYPE_DECLARATION){
                        fprintf(fp, ";\n\n");
                    }
                }
            }
            break;
        case NT_EXTERN_DECLARATION:
            if(this->children[0] != NULL){
                this->children[0]->writeCode(fp);
                fprintf(fp, "\n");
            }
            fprintf(fp, "%sextern ", TAB_SPACES.c_str());
            if(this->children.size() == 3){
                fprintf(fp, "%s", this->value.c_str());
                if(this->children[1] != NULL)
                    this->children[1]->writeCode(fp);
                fprintf(fp, " {\n");
                AST::IncreaseTabSpaces();
                if(this->children[2] != NULL)
                    this->children[2]->writeCode(fp);
                AST::DecreaseTabSpaces();
                fprintf(fp, "}\n\n");
            }
            else{
                if(this->children[1] != NULL)
                    this->children[1]->writeCode(fp);
                fprintf(fp, ";\n\n");
            }
            break;
        case NT_METHOD_PROTOTYPE_LIST:
            for(int i = 0; i < (int) this->children.size(); i++){
                this->children[i]->writeCode(fp);
                fprintf(fp, ";\n");
            }
            break;
        case NT_METHOD_PROTOTYPE:
            fprintf(fp, "%s", TAB_SPACES.c_str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            break;
        case NT_ABSTRACT_FUNCTION_PROTOTYPE:
            fprintf(fp, "abstract ");
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, " %s", this->str());
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, "(");
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            fprintf(fp, ")");
            break;
        case NT_TYPEARG_DONTCARE:
            fprintf(fp, "_");
            break;
        case NT_FUNCTION_PROTOTYPE:
            fprintf(fp, "%s", TAB_SPACES.c_str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, " %s", this->str());
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, "(");
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            fprintf(fp, ")");
            break;
        case NT_CONSTRUCTOR_METHOD_PROTOTYPE:
            fprintf(fp, "%s(", this->str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, ")");
            break;
        case NT_TUPLE_TYPE:
            fprintf(fp, "tuple");
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            break;
        case NT_ACTION_DECLARATION:
            if(this->children[0] != NULL){
                fprintf(fp, "%s", TAB_SPACES.c_str());
                this->children[0]->writeCode(fp);
                fprintf(fp, "\n");
            }
            fprintf(fp, "%s", TAB_SPACES.c_str());
            fprintf(fp, "action %s (", this->str());
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, ") {\n");
            AST::IncreaseTabSpaces();
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "%s", TAB_SPACES.c_str());
            fprintf(fp, "}\n\n");
            break;
        case NT_ENUM_DECLARATION:
            if(this->children[0] != NULL){
                this->children[0]->writeCode(fp);
                fprintf(fp, "\n");
            }
            fprintf(fp, "%senum ", TAB_SPACES.c_str());
            if(this->children[1] != NULL){
                this->children[1]->writeCode(fp);
                fprintf(fp, " ");
            }
            fprintf(fp, "%s {\n", this->str());
            AST::IncreaseTabSpaces();
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "}\n\n");
            break;
        case NT_TABLE_DECLARATION:
            fprintf(fp, "%s", TAB_SPACES.c_str());
            if(this->getChildNT(NT_ANNOTATION_LIST) != NULL){
                this->getChildNT(NT_ANNOTATION_LIST)->writeCode(fp);
                fprintf(fp, "\n%s", TAB_SPACES.c_str());
            }
            fprintf(fp, "table %s {\n", this->str());
            AST::IncreaseTabSpaces();
            if(this->getChildNT(NT_TABLE_PROPERTY_LIST) != NULL)
                this->getChildNT(NT_TABLE_PROPERTY_LIST)->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "%s", TAB_SPACES.c_str());
            fprintf(fp, "}\n\n");
            break;
        /*
        case NT_TABLE_PROPERTY_LIST:
            break;
        */
        case NT_TABLE_PROPERTY:
            if(this->children[0] != NULL){
                switch(this->children[0]->nType){
                    case NT_KEY_ELEMENT_LIST:
                    case NT_ACTION_LIST:
                    case NT_ENTRIES_LIST:
                        this->children[0]->writeCode(fp);
                        break;
                    default:
                        fprintf(fp, "%s", TAB_SPACES.c_str());
                        fprintf(fp, "%s = ", this->str());
                        this->children[0]->writeCode(fp);
                        fprintf(fp, ";\n");
                        break;
                }
            }
            break;
        case NT_HEADER_STACK_TYPE:
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, "[");
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, "]");
            break;
        case NT_KEY_ELEMENT_LIST:
            fprintf(fp, "%s", TAB_SPACES.c_str());
            fprintf(fp, "key = {\n");
            AST::IncreaseTabSpaces();
            for(AST *c : this->children)
                if(c != NULL)
                    c->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "%s", TAB_SPACES.c_str());
            fprintf(fp, "}\n\n");
            break;
        case NT_KEY_ELEMENT:
            fprintf(fp, "%s", TAB_SPACES.c_str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, ": %s", this->str());
            if(this->children[1] != NULL){
                fprintf(fp, " ");
                this->children[1]->writeCode(fp);
            }
            fprintf(fp, ";\n");
            break;
        /*
        case NT_OBJECT_DECLARATION_LIST:
            for(AST *c : this->children)
                if(c != NULL)
                    c->writeCode(fp);
            break;
        */
        case NT_ENTRIES_LIST:
            fprintf(fp, "%sconst entries = {\n", TAB_SPACES.c_str());
            if(this->children.size() > 0){
                AST::IncreaseTabSpaces();
                for(AST *c : this->children){
                    if(c != NULL){
                        c->writeCode(fp);
                        fprintf(fp, "\n");
                    }
                }
                AST::DecreaseTabSpaces();
            }
            fprintf(fp, "%s}\n", TAB_SPACES.c_str());
            break;
        case NT_ENTRY:
            fprintf(fp, "%s", TAB_SPACES.c_str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, ": ");
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            if(this->children[2] != NULL){
                fprintf(fp, " ");
                this->children[2]->writeCode(fp);
            }
            fprintf(fp, ";");
            break;
        case NT_ACTION_LIST:
            {
            fprintf(fp, "%s", TAB_SPACES.c_str());
            fprintf(fp, "actions = {\n");
            AST::IncreaseTabSpaces();
            bool nullAL = true;
            for(AST *c : this->children){
                if(c != NULL){
                    if(c->nType == NT_ANNOTATION_LIST){
                        fprintf(fp, "%s", TAB_SPACES.c_str());
                        c->writeCode(fp);
                        nullAL = false;
                    }
                    else{
                        if(nullAL)
                            fprintf(fp, "%s", TAB_SPACES.c_str());
                        c->writeCode(fp);
                        fprintf(fp, ";\n");
                        nullAL = true;
                    }
                }
            }
            AST::DecreaseTabSpaces();
            fprintf(fp, "%s", TAB_SPACES.c_str());
            fprintf(fp, "}\n\n");
            }
            break;
        case NT_ACTION_REF:
            fprintf(fp, "%s", this->str());
            if(this->children.size() > 0 && this->children[0] != NULL){
                fprintf(fp, "(");
                this->children[0]->writeCode(fp);
                fprintf(fp, ")");
            }
            break;
        case NT_CONSTANT_DECLARATION:
            if(this->children[0] != NULL){
                fprintf(fp, "%s", TAB_SPACES.c_str());
                this->children[0]->writeCode(fp);
                fprintf(fp, "\n");
            }
            fprintf(fp, "%sconst ", TAB_SPACES.c_str());
            this->children[1]->writeCode(fp);
            fprintf(fp, " %s = ", this->str());
            this->children[2]->writeCode(fp);
            fprintf(fp, ";\n\n");
            break;
        case NT_VARIABLE_DECLARATION:
            if(this->children[0] != NULL){
                fprintf(fp, "%s", TAB_SPACES.c_str());
                this->children[0]->writeCode(fp);
                fprintf(fp, "\n");
            }
            fprintf(fp, "%s", TAB_SPACES.c_str());
            this->children[1]->writeCode(fp);
            fprintf(fp, " %s", this->str());
            if(this->children[2] != NULL){
                fprintf(fp, " = ");
                this->children[2]->writeCode(fp);
            }
            fprintf(fp, ";\n\n");
            break;
        case NT_VALUE_SET_DECLARATION:
            if(this->children[0] != NULL){
                fprintf(fp, "%s", TAB_SPACES.c_str());
                this->children[0]->writeCode(fp);
                fprintf(fp, "\n");
            }
            fprintf(fp, "%s", TAB_SPACES.c_str());
            fprintf(fp, "value_set < ");

            Assert(this->children[1] != NULL);
            this->children[1]->writeCode(fp);
            fprintf(fp, " >(");
            Assert(this->children[2] != NULL);
            this->children[2]->writeCode(fp);
            fprintf(fp, ") %s;\n\n", this->str());
            break;
        case NT_BASE_TYPE:
            fprintf(fp, "%s", this->str());
            if(this->children.size() == 1){
                if(this->children[0]->nType == NT_LITERAL || this->children[0]->nType == NT_EXPRESSION){
                    fprintf(fp, "<");
                    this->children[0]->writeCode(fp);
                    fprintf(fp, ">");
                }
            }
            break;
        case NT_EXPRESSION:
            if(this->children.size() == 0){
                fprintf(fp, "%s", this->str());
            }
            else if(this->children.size() == 1){
                const static std::set<std::string> UnaryOps {"!", "-", "+", "~"};
                if(UnaryOps.find(this->value) != UnaryOps.end()){
                    fprintf(fp, "%s", this->str());
                    if(this->children[0] != NULL){
                        fprintf(fp, "(");
                        this->children[0]->writeCode(fp);
                        fprintf(fp, ")");
                    }
                }
                else{
                    fprintf(fp, "%s(", this->str());
                    if(this->children[0] != NULL)
                        this->children[0]->writeCode(fp);
                    fprintf(fp, ")");
                }
            }
            else if(this->children.size() == 2){
                if(BinaryOps.find(this->value) != BinaryOps.end()){
                    if(this->children[0] != NULL){
                        if(this->value != "." && this->children[0]->children.size() > 0)
                            fprintf(fp, "(");
                        this->children[0]->writeCode(fp);
                        if(this->value != "." && this->children[0]->children.size() > 0)
                            fprintf(fp, ")");
                    }
                    if(BinaryOpsSpace.find(this->value) != BinaryOpsSpace.end())
                        fprintf(fp, " ");
                    fprintf(fp, "%s", this->str());
                    if(BinaryOpsSpace.find(this->value) != BinaryOpsSpace.end())
                        fprintf(fp, " ");
                    if(this->children[1] != NULL){
                        if(this->value != "." && this->children[1]->children.size() > 0)
                            fprintf(fp, "(");
                        this->children[1]->writeCode(fp);
                        if(this->value != "." && this->children[1]->children.size() > 0)
                            fprintf(fp, ")");
                    }
                }
                else if(this->value == "?"){
                    if(this->children[0] != NULL)
                        this->children[0]->writeCode(fp);
                    fprintf(fp, " ? ");
                    if(this->children[1] != NULL)
                        this->children[1]->writeCode(fp);
                }
                else if(this->value == "[]"){
                    if(this->children[0] != NULL)
                        this->children[0]->writeCode(fp);
                    fprintf(fp, "[");
                    this->children[1]->writeCode(fp);
                    fprintf(fp, "]");
                }
                else if(this->value == "TYPE_CAST"){
                    fprintf(fp, "(");
                    this->children[0]->writeCode(fp);
                    fprintf(fp, ") ");
                    this->children[1]->writeCode(fp);
                }
                else if(this->children[0]->nType == NT_TYPE_ARGUMENT_LIST){
                    fprintf(fp, "%s", this->str());
                    this->children[0]->writeCode(fp);
                    fprintf(fp, "(");
                    if(this->children[1] != NULL)
                        this->children[1]->writeCode(fp);
                    fprintf(fp, ")");
                }
            }
            else if(this->children.size() == 3 && this->value == "."){
                if(this->children[0] != NULL)
                    this->children[0]->writeCode(fp);
                fprintf(fp, ".");
                if(this->children[1] != NULL)
                    this->children[1]->writeCode(fp);
                fprintf(fp, "(");
                if(this->children[2] != NULL)
                    this->children[2]->writeCode(fp);
                fprintf(fp, ")");
            }
            else if(this->children.size() == 4 && this->value == "."){
                if(this->children[0] != NULL)
                    this->children[0]->writeCode(fp);
                fprintf(fp, ".");
                if(this->children[1] != NULL)
                    this->children[1]->writeCode(fp);
                if(this->children[2] != NULL)
                    this->children[2]->writeCode(fp);
                fprintf(fp, "(");
                if(this->children[3] != NULL)
                    this->children[3]->writeCode(fp);
                fprintf(fp, ")");
            }
            break;
        case NT_LITERAL:
            fprintf(fp, "%s", this->str());
            break;
        case NT_HEADER_TYPE_DECLARATION:
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, "header %s ", this->str());
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, "{\n");
            AST::IncreaseTabSpaces();
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "}\n\n");
            break;
        case NT_HEADER_UNION_DECLARATION:
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, "header union %s ", this->str());
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, "{\n");
            AST::IncreaseTabSpaces();
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "}\n\n");
            break;
        case NT_ERROR_DECLARATION:
            fprintf(fp, "error {\n");
            AST::IncreaseTabSpaces();
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "}\n\n");
            break;
        case NT_STRUCT_TYPE_DECLARATION:
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, "struct %s ", this->str());
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, "{\n");
            AST::IncreaseTabSpaces();
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "}\n\n");
            break;
        /*
        case NT_STRUCT_FIELD_LIST:
            break;
        */
        case NT_STRUCT_FIELD:
            if(this->children[0] != NULL){
                fprintf(fp, "%s", TAB_SPACES.c_str());
                this->children[0]->writeCode(fp);
            }
            fprintf(fp, "%s", TAB_SPACES.c_str());
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, " %s;\n", this->str());
            break;
        case NT_CONTROL_DECLARATION:
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            if(this->children[1] != NULL){
                fprintf(fp, "(");
                this->children[1]->writeCode(fp);
                fprintf(fp, ")");
            }
            fprintf(fp, " {\n\n");
            AST::IncreaseTabSpaces();
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            fprintf(fp, "%sapply {\n", TAB_SPACES.c_str());
            AST::IncreaseTabSpaces();
            if(this->children[3] != NULL)
                this->children[3]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "%s}\n\n", TAB_SPACES.c_str());
            AST::DecreaseTabSpaces();
            fprintf(fp, "}\n\n");
            break;
        case NT_CONTROL_TYPE_DECLARATION:
            if(this->children[0] != NULL){
                this->children[0]->writeCode(fp);
                fprintf(fp, "\n");
            }
            fprintf(fp, "control %s", this->str());
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, "(");
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            fprintf(fp, ")");
            break;
        /*
        case NT_ANNOTATION_LIST:
            break;
        */
        /*
        case NT_ANNOTATION_TOKEN_LIST:
            break;
        */
        case NT_ANNOTATION:
            fprintf(fp, "@%s", this->str());
            if(this->children[0] != NULL){
                fprintf(fp, "(");
                this->children[0]->writeCode(fp);
                fprintf(fp, ")");
            }
            fprintf(fp, " ");
            break;
        /*
        case NT_CONTROL_LOCAL_DEC_LIST:
            break;
        */
        /*
        case NT_STAT_OR_DEC_LIST:
            break;
        */
        case NT_FUNCTION_DECLARATION:
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, " {\n");
            AST::IncreaseTabSpaces();
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "%s}\n", TAB_SPACES.c_str());
            break;
        case NT_RETURN_STATEMENT:
            fprintf(fp, "%sreturn ", TAB_SPACES.c_str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, ";\n");
            break;
        case NT_CONDITIONAL_STATEMENT:
            fprintf(fp, "%sif (", TAB_SPACES.c_str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, ") {\n");
            AST::IncreaseTabSpaces();
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "%s}\n", TAB_SPACES.c_str());
            if(this->children[2] != NULL){
                fprintf(fp, "%selse {\n", TAB_SPACES.c_str());
                AST::IncreaseTabSpaces();
                this->children[2]->writeCode(fp);
                AST::DecreaseTabSpaces();
                fprintf(fp, "%s}\n", TAB_SPACES.c_str());
            }
            break;
        case NT_SWITCH_STATEMENT:
            fprintf(fp, "%sswitch (", TAB_SPACES.c_str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, ") {\n");
            AST::IncreaseTabSpaces();
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "%s}\n", TAB_SPACES.c_str());
            break;
        case NT_SWITCH_CASE:
            fprintf(fp, "%s", TAB_SPACES.c_str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, ":");
            if(this->children.size() == 2){
                fprintf(fp, " {\n");
                if(this->children[1] != NULL){
                    AST::IncreaseTabSpaces();
                    this->children[1]->writeCode(fp);
                    AST::DecreaseTabSpaces();
                }
                fprintf(fp, "%s}", TAB_SPACES.c_str());
            }
            fprintf(fp, "\n");
            break;
        case NT_ASSIGN_STATEMENT:
            fprintf(fp, "%s", TAB_SPACES.c_str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, " = ");
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, ";\n");
            break;
        case NT_MATCH_KIND_DECLARATION:
            fprintf(fp, "%smatch_kind {\n", TAB_SPACES.c_str());
            AST::IncreaseTabSpaces();
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "%s}\n\n", TAB_SPACES.c_str());
            break;
        case NT_PARSER_DECLARATION:
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            if(this->children[1] != NULL){
                fprintf(fp, "(");
                this->children[1]->writeCode(fp);
                fprintf(fp, ")");
            }
            fprintf(fp, " {\n\n");
            AST::IncreaseTabSpaces();
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            if(this->children[3] != NULL)
                this->children[3]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "}\n\n");
            break;
        case NT_PARSER_TYPE_DECLARATION:
            if(this->children[0] != NULL){
                this->children[0]->writeCode(fp);
                fprintf(fp, "\n");
            }
            fprintf(fp, "parser %s", this->str());
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, "(");
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            fprintf(fp, ")");
            break;
        case NT_PACKAGE_TYPE_DECLARATION:
            if(this->children[0] != NULL){
                this->children[0]->writeCode(fp);
                fprintf(fp, "\n");
            }
            fprintf(fp, "package %s", this->str());
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, "(");
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            fprintf(fp, ");\n\n");
            break;
        /*
        case NT_PARSER_LOCAL_ELEMENT_LIST:
            break;
        */
        /*
        case NT_PARSER_STATE_LIST:
            break;
        */
        case NT_PARSER_STATE:
            if(this->children[0] != NULL){
                fprintf(fp, "%s", TAB_SPACES.c_str());
                this->children[0]->writeCode(fp);
                fprintf(fp, "\n");
            }
            fprintf(fp, "%s", TAB_SPACES.c_str());
            fprintf(fp, "state %s {\n", this->str());
            AST::IncreaseTabSpaces();
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, "%s", TAB_SPACES.c_str());
            fprintf(fp, "transition ");
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "%s", TAB_SPACES.c_str());
            fprintf(fp, "}\n\n");
            break;
        case NT_NAME_STATE_EXPRESSION:
            fprintf(fp, "%s;\n", this->str());
            break;
        case NT_SELECT_EXPRESSION:
            fprintf(fp, "%s(", this->str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, ") {\n");
            AST::IncreaseTabSpaces();
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            AST::DecreaseTabSpaces();
            fprintf(fp, "%s}\n", TAB_SPACES.c_str());
            break;
        case NT_TUPLE_KEYSET_EXPRESSION:
            fprintf(fp, "(");
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, ")");
            break;
        case NT_SIMPLE_EXPRESSION_LIST:
            for(int i = 0; i < (int) this->children.size(); i++){
                this->children[i]->writeCode(fp);
                if(i != (int) this->children.size() - 1)
                    fprintf(fp, ", ");
            }
            break;
        /*
        case NT_SELECT_CASE_LIST:
            break;
        */
        case NT_SELECT_CASE:
            fprintf(fp, "%s", TAB_SPACES.c_str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, ": %s;\n", this->str());
            break;
        case NT_MASK:
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, " &&& ");
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            break;
        case NT_RANGE:
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            fprintf(fp, " .. ");
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            break;
        case NT_SIMPLE_KEYSET_DEFAULT:
            fprintf(fp, "default");
            break;
        case NT_SIMPLE_KEYSET_DONTCARE:
            fprintf(fp, "_");
            break;
        /*
        case NT_PARSER_STATEMENT_LIST:
            break;
        */
        case NT_SPECIFIED_IDENTIFIER_LIST:
            for(int i = 0; i < (int) this->children.size(); i++){
                fprintf(fp, "%s", TAB_SPACES.c_str());
                this->children[i]->writeCode(fp);
                if(i != (int) this->children.size() - 1)
                    fprintf(fp, ",");
                fprintf(fp, "\n");
            }
            break;
        case NT_SPECIFIED_IDENTIFIER:
            fprintf(fp, "%s", this->str());
            if(this->children[0] != NULL){
                fprintf(fp, " = ");
                this->children[0]->writeCode(fp);
            }
            break;
        case NT_METHOD_CALL_STATEMENT:
            fprintf(fp, "%s", TAB_SPACES.c_str());
            if(this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, "(");
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            fprintf(fp, ");\n");
            break;
        case NT_NONE:
            if(this->value == "." && this->children.size() == 2){
                if(this->children[0] != NULL)
                    this->children[0]->writeCode(fp);
                fprintf(fp, ".");
                if(this->children[1] != NULL)
                    this->children[1]->writeCode(fp);
            }
            else if(this->value == "[]" && this->children.size() == 2){
                if(this->children[0] != NULL)
                    this->children[0]->writeCode(fp);
                fprintf(fp, "[");
                if(this->children[1] != NULL)
                    this->children[1]->writeCode(fp);
                fprintf(fp, "]");
            }
            else{
                fprintf(fp, "%s", this->str());
            }
            break;
        case NT_ARGUMENT_LIST:
            for(int i = 0; i < (int) this->children.size(); i++){
                this->children[i]->writeCode(fp);
                if(i != (int) this->children.size() - 1)
                    fprintf(fp, ", ");
            }
            break;
        case NT_IDENTIFIER_LIST:
            for(int i = 0; i < (int) this->children.size(); i++){
                fprintf(fp, "%s", TAB_SPACES.c_str());
                this->children[i]->writeCode(fp);
                if(i != (int) this->children.size() - 1)
                    fprintf(fp, ",");
                fprintf(fp, "\n");
            }
            break;
        case NT_ARGUMENT:
            fprintf(fp, "%s", this->str());
            if(this->children[0] != NULL){
                if(this->value != "")
                    fprintf(fp, " = ");

                if(this->children[0]->nType == NT_EXPRESSION_LIST)
                    fprintf(fp, "{");
                this->children[0]->writeCode(fp);
                if(this->children[0]->nType == NT_EXPRESSION_LIST)
                    fprintf(fp, "}");
            }
            break;
        case NT_INSTANTIATION:
            if(this->children[0] != NULL){
                fprintf(fp, "%s", TAB_SPACES.c_str());
                this->children[0]->writeCode(fp);
                fprintf(fp, "\n");
            }
            fprintf(fp, "%s", TAB_SPACES.c_str());
            if(this->children[1] != NULL)
                this->children[1]->writeCode(fp);
            fprintf(fp, "(");
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            fprintf(fp, ") %s", this->str());
            
            if(this->children.size() == 4){
                fprintf(fp, " {\n");
                AST::IncreaseTabSpaces();
                if(this->children[3] != NULL)
                    this->children[3]->writeCode(fp);
                AST::DecreaseTabSpaces();
                fprintf(fp, "%s}\n", TAB_SPACES.c_str());
            }
            else{
                fprintf(fp, ";\n");
            }
            
            break;
        case NT_PARAMETER_LIST:
            for(int i = 0; i < (int) this->children.size(); i++){
                this->children[i]->writeCode(fp);
                if(i != (int) this->children.size() - 1)
                    fprintf(fp, ", ");
            }
            break;
        case NT_EXPRESSION_LIST:
            for(int i = 0; i < (int) this->children.size(); i++){
                this->children[i]->writeCode(fp);
                if(i != (int) this->children.size() - 1)
                    fprintf(fp, ", ");
            }
            break;
        case NT_PARAMETER:
            if(this->children[0] != NULL){
                this->children[0]->writeCode(fp);
            }
            if(this->children[1] != NULL){
                this->children[1]->writeCode(fp);
                fprintf(fp, " ");
            }
            if(this->children[2] != NULL)
                this->children[2]->writeCode(fp);
            fprintf(fp, " %s", this->str());
            if(this->children.size() == 4){
                fprintf(fp, " = ");
                if(this->children[3] != NULL)
                    this->children[3]->writeCode(fp);
            }
            break;
        case NT_TYPE_PARAMETER:
        case NT_TYPEARG_NONTYPENAME:
        case NT_DIRECTION:
        case NT_ANNOTATION_TOKEN:
        case NT_TYPE_OR_VOID:
            fprintf(fp, "%s", this->str());
            break;
        case NT_TYPE_NAME:
            fprintf(fp, "%s", this->str());
            if(this->children.size() > 0 && this->children[0] != NULL)
                this->children[0]->writeCode(fp);
            break;
        case NT_TYPE_PARAMETER_LIST:
            fprintf(fp, "<");
            for(int i = 0; i < (int) this->children.size(); i++){
                this->children[i]->writeCode(fp);
                if(i != (int) this->children.size() - 1)
                    fprintf(fp, ", ");
            }
            fprintf(fp, ">");
            break;
        case NT_TYPE_ARGUMENT_LIST:
            fprintf(fp, "<");
            for(int i = 0; i < (int) this->children.size(); i++){
                this->children[i]->writeCode(fp);
                if(i != (int) this->children.size() - 1)
                    fprintf(fp, ", ");
            }
            fprintf(fp, ">");
            break;
        default:
            for(AST *c : this->children)
                if(c != NULL)
                    c->writeCode(fp);
            break;
    }
}

void AST::writeCode(const std::string& fileName) const {
    FILE *fp = fopen(fileName.c_str(), "w");
    if(fp == NULL){
        fprintf(stderr, "Error opening file %s\n", fileName.c_str());
        exit(1);
    }
    this->writeCode(fp);
    fclose(fp);
}

std::vector<std::pair<AST*, AST*>> AST::GetHeadersWithRemovedUnusedFields(){
    char buffer[BUFFER_SIZE];
    
    Assert(this->nType == NT_PROGRAM);
    
    std::vector<std::pair<AST*, AST*>> result;
    
    for(AST *c : this->children){
        if(c->nType == NT_HEADER_TYPE_DECLARATION){
            AST *SFL = c->getChildNT(NT_STRUCT_FIELD_LIST);
            if(SFL != NULL){
                std::vector<bool> fieldUsed(SFL->children.size(), true);
                for(size_t i = 0; i < SFL->children.size(); i++)
                    fieldUsed[i] = this->isFieldUsed(SFL->children[i]->value, c->value);
            
                AST *newHeader = new AST(c->value, NT_HEADER_TYPE_DECLARATION);
                newHeader->addChild(NULL_CLONE(c->children[0]));
                newHeader->addChild(NULL_CLONE(c->children[1]));
                newHeader->addChild(new AST("", NT_STRUCT_FIELD_LIST));
                int bitAcc = 0;
                int unusedCounter = 1;
                for(size_t i = 0; i < SFL->children.size(); i++){
                    AST *SF = SFL->children[i];
                    if(fieldUsed[i]){
                        if(bitAcc > 0){
                            sprintf(buffer, "_unused%d", unusedCounter);
                            AST *unusedSF = new AST(buffer, NT_STRUCT_FIELD);
                            unusedSF->addChild(new AST("", NT_ANNOTATION_LIST));
                            unusedSF->children[0]->addChild(new AST("unused", NT_ANNOTATION));
                            unusedSF->children[0]->children[0]->addChild(NULL);
                            unusedSF->addChild(new AST("bit", NT_BASE_TYPE));
                            sprintf(buffer, "%d", bitAcc);
                            unusedSF->children[1]->addChild(new AST(buffer, NT_LITERAL));
                            newHeader->getChildNT(NT_STRUCT_FIELD_LIST)->addChild(unusedSF);
                            
                            bitAcc = 0;
                            unusedCounter++;
                        }
                        newHeader->getChildNT(NT_STRUCT_FIELD_LIST)->addChild(SF->Clone());
                    }
                    else{
                        Assert(SF->getChildNT(NT_BASE_TYPE) != NULL);
                        Assert(SF->getChildNT(NT_BASE_TYPE)->value == "bit");
                        Assert(SF->getChildNT(NT_BASE_TYPE)->children.size() == 1);

                        bitAcc = bitAcc + IPLBase::intFromStr(SF->getChildNT(NT_BASE_TYPE)->children[0]->str());
                    }
                }
                if(bitAcc > 0){
                    sprintf(buffer, "_unused%d", unusedCounter);
                    AST *unusedSF = new AST(buffer, NT_STRUCT_FIELD);
                    unusedSF->addChild(new AST("", NT_ANNOTATION_LIST));
                    unusedSF->children[0]->addChild(new AST("unused", NT_ANNOTATION));
                    unusedSF->children[0]->children[0]->addChild(NULL);
                    unusedSF->addChild(new AST("bit", NT_BASE_TYPE));
                    sprintf(buffer, "%d", bitAcc);
                    unusedSF->children[1]->addChild(new AST(buffer, NT_LITERAL));
                    newHeader->getChildNT(NT_STRUCT_FIELD_LIST)->addChild(unusedSF);
                    
                    bitAcc = 0;
                }

                result.push_back(std::make_pair(c, newHeader));
            }
        }
    }

    return result;
}

bool AST::isFieldUsed(const std::string& fieldName, const std::string& headerType){
    if(this->value == "." && this->children.size() == 2 && this->children[0]->programType == headerType && this->children[1]->value == fieldName)
        return true;

    for(AST *c : this->children)
        if(c != NULL)
            if(c->isFieldUsed(fieldName, headerType))
                return true;

    return false;
}

void AST::SubstituteTypeName(const std::string& oldName, const std::string& newName){
    if(this->value == oldName && (this->nType == NT_TYPE_NAME || this->nType == NT_EXPRESSION))
        this->value = newName;
    if(this->programType == oldName)
        this->programType = newName;
    for(AST *c : this->children)
        if(c != NULL)
            c->SubstituteTypeName(oldName, newName);
}

void AST::IncreaseTabSpaces(){
    for(int i = 0; i < 4; i++)
        AST::TAB_SPACES.push_back(' ');
}

void AST::DecreaseTabSpaces(){
    for(int i = 0; i < 4; i++)
        AST::TAB_SPACES.pop_back();
}

void AST::RemoveEqualSwitchCases(AST *switchStatement){
    std::vector<std::vector<AST*>> sameCasesExpression;
    std::vector<AST*> sameCasesStatement;

    std::map<size_t, size_t> caseToUniqueList;

    AST *caseList = switchStatement->getChildNT(NT_SWITCH_CASE_LIST);
    Assert(caseList != NULL);

    for(size_t i = 0; i < caseList->children.size(); i++){
        bool unique = true;
        size_t j = i - 1;

        if(caseList->children[i]->children.size() == 1){ //Fall through
            size_t i2 = i + 1;
            while(i2 < caseList->children.size() && caseList->children[i2]->children.size() == 1)
                i2++;
            Assert(i2 < caseList->children.size() - 1);
            caseList->children[i]->children.push_back(caseList->children[i2]->children[1]->Clone());
        }

        while(unique && j < i){
            if(AST::Equals(caseList->children[i]->children[1], caseList->children[j]->children[1])){
                unique = false;
                size_t uniqueListIdx = caseToUniqueList.find(j)->second;
                caseToUniqueList.emplace(i, uniqueListIdx);
                sameCasesExpression[uniqueListIdx].push_back(caseList->children[i]->children[0]->Clone());
            }
            j--;
        }

        if(unique){
            caseToUniqueList.emplace(i, sameCasesExpression.size());
            sameCasesExpression.push_back(std::vector<AST*> {caseList->children[i]->children[0]->Clone()});
            sameCasesStatement.push_back(NULL_CLONE(caseList->children[i]->children[1]));
        }
    }

    if(sameCasesStatement.size() > 1){
        for(AST *scase : caseList->children)
            delete scase;
        caseList->children.clear();

        for(size_t i = 0; i < sameCasesExpression.size(); i++){
            for(size_t j = 0; j < sameCasesExpression[i].size(); j++){
                AST *newCase = new AST("", NT_SWITCH_CASE);
                newCase->addChild(sameCasesExpression[i][j]);
                caseList->addChild(newCase);
            }
            caseList->children[caseList->children.size() - 1]->addChild(sameCasesStatement[i]);
        }
    }
    else{
        Assert(sameCasesStatement.size() == 1);
        //Transform into a simple block statement

        for(std::vector<AST*>& v : sameCasesExpression)
            for(AST *c : v)
                delete c;

        for(AST *c : switchStatement->children)
            delete c;
        switchStatement->children.clear();
        switchStatement->nType = NT_STAT_OR_DEC_LIST;

        for(AST *c : sameCasesStatement[0]->children)
            switchStatement->addChild(c->Clone());
        delete sameCasesStatement[0];
    }
}

bool AST::isConditionVirtProgramIDCheck(AST *condition){
    if(condition == NULL)
        return false;

    if(condition->value == "||" && condition->children.size() == 2){
        return AST::isConditionVirtProgramIDCheck(condition->children[0]) && AST::isConditionVirtProgramIDCheck(condition->children[1]);
    }

    if(condition->value == "==" && condition->children.size() == 2){
        return condition->children[0]->value == TEMP_PROGRAM_CODE;
    }

    return false;
}