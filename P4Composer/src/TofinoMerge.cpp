#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../include/AST.h"
#include "../include/TofinoMerge.h"
#include "../include/Log.h"
#include "../include/CFG.h"

#define PROGRAM_ID_TYPE_BITS_STR "8"
#define METADATA_NAME "VirtMetadata"
#define METADATA_PROG_ID_FIELD_NAME "VirtProgramID"

#define INGRESS_LOOKAHEAD_HEADER_TYPE "VirtIngressLookaheadHeader"
#define INGRESS_LOOKAHEAD_VAR_NAME "VirtLookahead"

#define EGRESS_LOOKAHEAD_HEADER_TYPE "VirtEgressLookaheadHeader"
#define EGRESS_LOOKAHEAD_VAR_NAME "VirtLookahead"

#define INGRESS_METADATA_TYPE_NAME "VirtIngressMetadataType"
#define EGRESS_METADATA_TYPE_NAME "VirtEgressMetadataType"
#define SINGLE_PIPELINE_NAME "VirtPipe"

#define PI_PARAM_TYPE "packet_in"
#define PI_PARAM_NAME "pkt"
#define PO_PARAM_TYPE "packet_out"
#define PO_PARAM_NAME "pkt"
#define IG_INTRINSIC_MD_PARAM_TYPE "ingress_intrinsic_metadata_t"
#define IG_INTRINSIC_MD_PARAM_NAME "ig_intr_md"
#define IG_INTRINSIC_MD_PARSER_PARAM_TYPE "ingress_intrinsic_metadata_from_parser_t"
#define IG_INTRINSIC_MD_PARSER_PARAM_NAME "ig_intr_prsr_md"
#define IG_INTRINSIC_MD_DEPARSER_PARAM_TYPE "ingress_intrinsic_metadata_for_deparser_t"
#define IG_INTRINSIC_MD_DEPARSER_PARAM_NAME "ig_intr_dprsr_md"
#define IG_INTRINSIC_MD_TM_PARAM_TYPE "ingress_intrinsic_metadata_for_tm_t"
#define IG_INTRINSIC_MD_TM_PARAM_NAME "ig_intr_tm_md"
#define EG_INTRINSIC_MD_PARAM_TYPE "egress_intrinsic_metadata_t"
#define EG_INTRINSIC_MD_PARAM_NAME "eg_intr_md"
#define EG_INTRINSIC_MD_PARSER_PARAM_TYPE "egress_intrinsic_metadata_from_parser_t"
#define EG_INTRINSIC_MD_PARSER_PARAM_NAME "eg_intr_from_prsr"
#define EG_INTRINSIC_MD_DEPARSER_PARAM_TYPE "egress_intrinsic_metadata_for_deparser_t"
#define EG_INTRINSIC_MD_DEPARSER_PARAM_NAME "eg_intr_md_for_dprsr"
#define EG_INTRINSIC_MD_OP_PARAM_TYPE "egress_intrinsic_metadata_for_output_port_t"
#define EG_INTRINSIC_MD_OP_PARAM_NAME "eg_intr_md_for_oport"

void RenameVirtMetadataVariables(AST *ast, const std::string& metadataTypeName, const std::string& newName){
    if(ast->getProgramType() == metadataTypeName)
        ast->setValue(newName);
    for(AST *c : ast->getChildren())
        if(c != NULL)
            RenameVirtMetadataVariables(c, metadataTypeName, newName);
}

void RemoveTempProgramCode(AST *ast, const std::string& metadataTypeName, const std::string& newName){
    if(ast->getNodeType() == NT_EXPRESSION && ast->getValue() == TEMP_PROGRAM_CODE){
        ast->setValue(".");
        ast->addChild(new AST(newName, NT_EXPRESSION));
        ast->addChild(new AST(METADATA_PROG_ID_FIELD_NAME, NT_NONE));
        ast->getChildren()[1]->setProgramType(metadataTypeName);
    }
    for(AST *c : ast->getChildren())
        if(c != NULL)
            RemoveTempProgramCode(c, metadataTypeName, newName);
}

bool TofinoMerge::MergeTofino(const std::vector<AST*> programs, const std::vector<AST*>& headersDec, const std::vector<std::set<int>>& virtualSwitchPorts, const int cpuPort){
    CFG::SetMetadataProgIDName(std::string(METADATA_NAME "." METADATA_PROG_ID_FIELD_NAME));
    
    AST* LastProg = programs[programs.size() - 1];

    std::string msg = "Merging Tofino Programs:";
    for(AST *program : programs)
        msg = msg + " \"" + program->value + "\"";
    Log::MSG(msg, LOG_LEVEL_SHORT);
    Log::Push();

    std::vector<std::vector<AST*>> Pipelines(programs.size());

    int nPipes = -1;
    for(int i = 0; i < (int) programs.size(); i++){
        if(!GetPipelines(programs[i], &Pipelines[i])){
            Log::Pop();
            return false;
        }
        if(nPipes == -1)
            nPipes = (int) Pipelines[i].size();
        else if(nPipes != (int) Pipelines[i].size()){
            fprintf(stderr, "TofinoMerge: Different number of Switch Pipelines\n");
            Log::Pop();
            return false;
        }
        
        TofinoMerge::RemoveSwitchInstantiation(programs[i]);
    }
    
    for(int i = 0; i < (int) Pipelines[0].size(); i++){
        std::vector<AST*> pipelinesToMerge;
        for(int j = 0; j < (int) programs.size(); j++)
            pipelinesToMerge.push_back(Pipelines[j][i]);
        bool result = TofinoMerge::MergePipelines(programs, pipelinesToMerge, headersDec, virtualSwitchPorts, cpuPort);
        if(!result){
            Log::Pop();
            return false;
        }
    }

    //Construct new Switch instantiation
    Assert(Pipelines[0].size() == 1);
    AST* newSwitch = new AST("main", NT_INSTANTIATION);
    newSwitch->addChild(NULL); //annotations
    newSwitch->addChild(new AST("Switch", NT_TYPE_NAME));
    newSwitch->addChild(new AST("", NT_ARGUMENT_LIST));
    newSwitch->getChildNT(NT_ARGUMENT_LIST)->addChild(new AST("", NT_ARGUMENT));
    newSwitch->getChildNT(NT_ARGUMENT_LIST)->children[0]->addChild(new AST(SINGLE_PIPELINE_NAME, NT_EXPRESSION));
    newSwitch->getChildNT(NT_ARGUMENT_LIST)->children[0]->addChild(NULL);
    LastProg->addChild(newSwitch);

    const std::unordered_map<std::string, int> VirtPipelineStructNames = {{"VirtIngressParser", 2}, {"VirtEgressParser", 2}, {"VirtIngress", 1}, {"VirtEgress", 1}, {"VirtIngressDeparser", 2}, {"VirtEgressDeparser", 2}};
    const std::unordered_map<std::string, int> VirtIngressPipelineStructNames = {{"VirtIngressParser", 2}, {"VirtIngress", 1}, {"VirtIngressDeparser", 2}};
    const std::unordered_map<std::string, int> VirtEgressPipelineStructNames = {{"VirtEgressParser", 2}, {"VirtEgress", 1}, {"VirtEgressDeparser", 2}};
    for(int i = 0; i < (int) LastProg->children.size(); i++){
        bool isIngresStruct = VirtIngressPipelineStructNames.find(LastProg->children[i]->value) != VirtIngressPipelineStructNames.end();
        bool isEgresStruct = VirtEgressPipelineStructNames.find(LastProg->children[i]->value) != VirtEgressPipelineStructNames.end();
        if(isIngresStruct || isEgresStruct){
            const NodeType nTypeDec = (LastProg->children[i]->nType == NT_PARSER_DECLARATION) ? NT_PARSER_TYPE_DECLARATION : NT_CONTROL_TYPE_DECLARATION;
            const std::string structMetadataTypeName = isIngresStruct ? INGRESS_METADATA_TYPE_NAME : EGRESS_METADATA_TYPE_NAME;
            const std::string vMetadataHeaderName = LastProg->children[i]->getChildNT(nTypeDec)->getChildNT(NT_PARAMETER_LIST)->children[VirtPipelineStructNames.find(LastProg->children[i]->getChildNT(nTypeDec)->value)->second]->getChildNT(NT_TYPE_NAME)->value;
            LastProg->children[i]->getChildNT(nTypeDec)->getChildNT(NT_PARAMETER_LIST)->children[VirtPipelineStructNames.find(LastProg->children[i]->getChildNT(nTypeDec)->value)->second]->value = METADATA_NAME;
            
            AST *newField = new AST(METADATA_PROG_ID_FIELD_NAME, NT_STRUCT_FIELD);
            newField->addChild(NULL); //optAnnotations
            newField->addChild(new AST("bit", NT_BASE_TYPE));
            newField->children[1]->addChild(new AST("8", NT_LITERAL));
            
            int pp = 0;
            while(pp < (int) LastProg->children.size() && ((LastProg->children[pp]->nType != NT_HEADER_TYPE_DECLARATION && LastProg->children[pp]->nType != NT_STRUCT_TYPE_DECLARATION) || LastProg->children[pp]->value != vMetadataHeaderName))
                pp++;
            if(pp == (int) LastProg->children.size()){
                pp = 0;
                while(pp < (int) headersDec.size() && ((headersDec[pp]->nType != NT_HEADER_TYPE_DECLARATION && headersDec[pp]->nType != NT_STRUCT_TYPE_DECLARATION) || headersDec[pp]->value != vMetadataHeaderName))
                    pp++;
                Assert(pp < (int) headersDec.size());
                if(headersDec[pp]->getChildNT(NT_STRUCT_FIELD_LIST) != NULL){
                    bool alreadyHas = false;
                    for(int sf = 0; sf < (int) headersDec[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->children.size(); sf++){
                        if(headersDec[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->children[sf]->value == METADATA_PROG_ID_FIELD_NAME)
                            alreadyHas = true;
                    }
                    if(!alreadyHas){
                        headersDec[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->addChild(newField->Clone());
                    }
                }
                else{
                    AST *newSFL = new AST("", NT_STRUCT_FIELD_LIST);
                    newSFL->addChild(newField->Clone());
                    headersDec[pp]->children[headersDec[pp]->children.size() - 1] = newSFL;
                }
                LastProg->SubstituteTypeName(headersDec[pp]->value, structMetadataTypeName);
                headersDec[pp]->value = structMetadataTypeName;
            }
            else{
                if(LastProg->children[pp]->getChildNT(NT_STRUCT_FIELD_LIST) == NULL)
                    LastProg->children[pp]->children[2] = new AST("", NT_STRUCT_FIELD_LIST);
                
                bool alreadyHas = false;
                for(int sf = 0; sf < (int) LastProg->children[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->children.size(); sf++){
                    if(LastProg->children[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->children[sf]->value == METADATA_PROG_ID_FIELD_NAME)
                        alreadyHas = true;
                }
                if(!alreadyHas){
                    LastProg->RemoveHeaderFieldsPrefix(LastProg->children[pp]);
                    LastProg->children[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->addChild(newField->Clone());
                }

                LastProg->SubstituteTypeName(LastProg->children[pp]->value, structMetadataTypeName);
                LastProg->children[pp]->value = structMetadataTypeName;
            }

            delete newField;
            RenameVirtMetadataVariables(LastProg->children[i], structMetadataTypeName, METADATA_NAME);
            RemoveTempProgramCode(LastProg->children[i], structMetadataTypeName, METADATA_NAME);
        }
    }

    Log::Pop();
    return true;
}

void TofinoMerge::FixCompilerErrors(AST *ast){
    //Check for actions that do not use  attached objects in tables
    if(ast->getNodeType() == NT_TABLE_DECLARATION){
        AST *countersProperty = NULL;
        AST *metersProperty = NULL;
        AST *actionsProperty = NULL;
        AST *defaultActionProperty = NULL;

        AST *controlBlock = ast->parent;
        while(controlBlock != NULL && controlBlock->nType != NT_CONTROL_DECLARATION)
            controlBlock = controlBlock->parent;
        Assert(controlBlock != NULL);

        for(AST *property : ast->getChildNT(NT_TABLE_PROPERTY_LIST)->children){
            if(property->value == "counters")
                countersProperty = property;
            else if(property->value == "meters")
                metersProperty = property;
            else if(property->value == "actions")
                actionsProperty = property;
            else if(property->value == "default_action")
                defaultActionProperty = property;
        }

        Assert(actionsProperty != NULL);

        if(countersProperty != NULL || metersProperty != NULL){
            if(defaultActionProperty != NULL){
                const std::string actionName = defaultActionProperty->children[0]->value;
                if(actionName == "NoAction"){
                    defaultActionProperty->parent->RemoveChild(defaultActionProperty);
                    delete defaultActionProperty;
                    AST *actionList = actionsProperty->getChildNT(NT_ACTION_LIST);
                    for(size_t i = 1; i < actionList->children.size(); i = i + 2){
                        if(actionList->children[i]->value == actionName){
                            delete actionList->children[i - 1];
                            delete actionList->children[i];
                            actionList->children.erase(actionList->children.begin() + i - 1, actionList->children.begin() + i + 1);
                            break;
                        }
                    }
                }
            }
        }
    }

    for(AST *c : ast->getChildren())
        if(c != NULL)
            TofinoMerge::FixCompilerErrors(c);
}

bool TofinoMerge::RemoveIngressPortMetadataExtractions(const std::string& programName, AST *statelist, AST *state, const std::string packetInName, const std::string parameterName, bool intrinsicMDportMD){
    return true; //TODO: Is this necessary anymore?
    
    AST *statementList = state->getChildNT(NT_PARSER_STATEMENT_LIST);

    if(statementList != NULL){
        int i = 0;
        while(i < (int) statementList->children.size()){
            AST *statement = statementList->children[i];
            if(statement->nType == NT_METHOD_CALL_STATEMENT){
                if(statement->children[0]->children[1]->value == "extract"){
                    if(intrinsicMDportMD){
                        AST *argList = statement->getChildNT(NT_ARGUMENT_LIST);
                        if(argList != NULL && argList->children.size() == 1 && argList->children[0]->children[0]->value == parameterName){
                            delete statement;
                            statementList->children.erase(statementList->children.begin() + i);
                            i--;

                            intrinsicMDportMD = false;
                        }
                        else{
                            fprintf(stderr, "Error: Ingress parser first packet_in extract must be of \"%s\" in program \"%s\"\n", parameterName.c_str(), programName.c_str());
                            exit(1);
                        }
                    }
                    else{
                        fprintf(stderr, "Error: Ingress parser must skip (advance) the TNA Port Metadata in program \"%s\"\n", programName.c_str());
                        exit(1);
                    }
                }
                else if(statement->children[0]->children[1]->value == "advance"){
                    delete statement;
                    statementList->children.erase(statementList->children.begin() + i);
                    i--;

                    if(statementList->children.size() == 0){
                        delete statementList;
                        state->children[1] = NULL;
                    }

                    return true;
                }
            }

            i++;
        }
        if(statementList->children.size() == 0){
            delete statementList;
            state->children[1] = NULL;
        }
    }
    if(state->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL){
        AST *nextState = statelist->FindStateInList(state->getChildNT(NT_NAME_STATE_EXPRESSION)->value);
        if(nextState == NULL){
            fprintf(stderr, "Error: State not found in ingress parser: %s (\"%s\")\n", state->getChildNT(NT_NAME_STATE_EXPRESSION)->value.c_str(), programName.c_str());
            exit(1);
        }
        return TofinoMerge::RemoveIngressPortMetadataExtractions(programName, statelist, nextState, packetInName, parameterName, intrinsicMDportMD);
    }
    else{
        AST *SCL = state->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_SELECT_CASE_LIST);
        for(AST *sc : SCL->children){
            if(sc->value != "accept" && sc->value != "reject"){
                AST *nextState = statelist->FindStateInList(sc->value);
                if(nextState == NULL){
                    fprintf(stderr, "Error: State not found in ingress parser: %s (\"%s\")\n", state->getChildNT(NT_NAME_STATE_EXPRESSION)->value.c_str(), programName.c_str());
                    exit(1);
                }
                bool result = TofinoMerge::RemoveIngressPortMetadataExtractions(programName, statelist, nextState, packetInName, parameterName, intrinsicMDportMD);
                if(!result){
                    fprintf(stderr, "Error: Program \"%s\" must extract the ingress intrinsic metadata in the ingress parser and skip (advance) the Port Metadata field\n", programName.c_str());
                    exit(1);
                }
            }
        }
        return true;
    }

    return false;
}

bool TofinoMerge::RemoveEgressPortMetadataExtractions(const std::string& programName, AST *statelist, AST *state, const std::string packetInName, const std::string parameterName){
    return true; //TODO: Is this necessary anymore?
    
    AST *statementList = state->getChildNT(NT_PARSER_STATEMENT_LIST);

    //Check for the first extract statement of the packet_in
    if(statementList != NULL){
        size_t i = 0;
        while(i < statementList->children.size()){
            AST *statement = statementList->children[i];
            if(statement->nType == NT_METHOD_CALL_STATEMENT){
                if(statement->children[0]->children[1]->value == "extract"){
                    AST *argList = statement->getChildNT(NT_ARGUMENT_LIST);
                    if(argList != NULL && argList->children.size() == 1 && argList->children[0]->children[0]->value == parameterName){
                        delete statement;
                        statementList->children.erase(statementList->children.begin() + i);
                        i--;

                        if(statementList->children.size() == 0){
                            delete statementList;
                            state->children[1] = NULL;
                        }

                        return true;
                    }
                    else{
                        fprintf(stderr, "Error: Egress parser first packet_in extract must be of \"%s\" in program \"%s\"\n", parameterName.c_str(), programName.c_str());
                        exit(1);
                    }
                }
            }
            i++;
        }
        
        if(statementList->children.size() == 0){
            delete statementList;
            state->children[1] = NULL;
        }
    }

    if(state->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL){
        const std::string nextStateName = state->getChildNT(NT_NAME_STATE_EXPRESSION)->value;
        if(nextStateName == "accept" || nextStateName == "reject")
            return true;
        AST *nextState = statelist->FindStateInList(nextStateName);
        if(nextState == NULL){
            fprintf(stderr, "Error: State not found in egress parser: %s (\"%s\")\n", nextStateName.c_str(), programName.c_str());
            exit(1);
        }
        return TofinoMerge::RemoveEgressPortMetadataExtractions(programName, statelist, nextState, packetInName, parameterName);
    }
    else{
        AST *SCL = state->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_SELECT_CASE_LIST);
        for(AST *sc : SCL->children){
            if(sc->value != "accept" && sc->value != "reject"){
                AST *nextState = statelist->FindStateInList(sc->value);
                if(nextState == NULL){
                    fprintf(stderr, "Error: State not found in egress parser: %s (\"%s\")\n", state->getChildNT(NT_NAME_STATE_EXPRESSION)->value.c_str(), programName.c_str());
                    exit(1);
                }
                bool result = TofinoMerge::RemoveEgressPortMetadataExtractions(programName, statelist, nextState, packetInName, parameterName);
                if(!result){
                    fprintf(stderr, "Error: Program \"%s\" must extract the egress intrinsic metadata in the egress parser\n", programName.c_str());
                    exit(1);
                }
            }
        }
        return true;
    }

    return false;
}

bool TofinoMerge::MergePipelines(const std::vector<AST*>& programs, const std::vector<AST*>& pipelines, const std::vector<AST*>& headersDec, const std::vector<std::set<int>>& virtualSwitchPorts, const int cpuPort){
    const static std::unordered_map<std::string, std::string> ParamNames = {
        {PI_PARAM_TYPE, PI_PARAM_NAME},
        {PO_PARAM_TYPE, PO_PARAM_NAME},
        {IG_INTRINSIC_MD_PARAM_TYPE, IG_INTRINSIC_MD_PARAM_NAME},
        {IG_INTRINSIC_MD_PARSER_PARAM_TYPE, IG_INTRINSIC_MD_PARSER_PARAM_NAME},
        {IG_INTRINSIC_MD_DEPARSER_PARAM_TYPE, IG_INTRINSIC_MD_DEPARSER_PARAM_NAME},
        {IG_INTRINSIC_MD_TM_PARAM_TYPE, IG_INTRINSIC_MD_TM_PARAM_NAME},
        {EG_INTRINSIC_MD_PARAM_TYPE, EG_INTRINSIC_MD_PARAM_NAME},
        {EG_INTRINSIC_MD_PARSER_PARAM_TYPE, EG_INTRINSIC_MD_PARSER_PARAM_NAME},
        {EG_INTRINSIC_MD_DEPARSER_PARAM_TYPE, EG_INTRINSIC_MD_DEPARSER_PARAM_NAME},
        {EG_INTRINSIC_MD_OP_PARAM_TYPE, EG_INTRINSIC_MD_OP_PARAM_NAME},
    };

    std::vector<AST*> nodesToAdd;

    //Create Headers for metadata lookahead
    AST *ingressMetadataHeader = CreateLookaheadHeaderIngress(cpuPort > 0);
    AST *egressMetadataHeader = CreateLookaheadHeaderEgress(cpuPort > 0);
    nodesToAdd.push_back(ingressMetadataHeader);
    nodesToAdd.push_back(egressMetadataHeader);

    //Parsers
    const std::vector<int> parserParams = {0, 3};
    const std::vector<std::string> parserNames = {"VirtIngressParser", "VirtEgressParser"};
    int pCounter = 0;
    for(int parserParam : parserParams){
        std::vector<AST*> elements;
        std::vector<std::string> elementNames;
        for(AST* pipeline : pipelines)
            elementNames.push_back(pipeline->getChildNT(NT_ARGUMENT_LIST)->children[parserParam]->children[0]->value);

        for(int p = 0; p < (int) programs.size(); p++){
            int i = 0;
            while(i < (int) programs[p]->children.size() && (programs[p]->children[i]->nType != NT_PARSER_DECLARATION || programs[p]->children[i]->children[0]->value != elementNames[p])){
                i++;
            }
            if(i == (int) programs[p]->children.size()){
                fprintf(stderr, "TofinoMerge: Could not find \"%s\" parser declaration in program \"%s\"\n", elementNames[p].c_str(), programs[p]->value.c_str());
                return false;
            }

            AST *parser = programs[p]->children[i];

            if(cpuPort > 0){
                for(AST *a : headersDec){
                    if(a->value == parser->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[1]->getChildNT(NT_TYPE_NAME)->value){
                        bool hasPacketInHeader = false;
                        
                        if(a->getChildNT(NT_STRUCT_FIELD_LIST) == NULL)
                            a->children[2] = new AST("", NT_STRUCT_FIELD_LIST);

                        for(AST *SF : a->getChildNT(NT_STRUCT_FIELD_LIST)->children)
                            if(SF->getChildNT(NT_TYPE_NAME) != NULL && SF->getChildNT(NT_TYPE_NAME)->value == VIRT_PACKET_IN_HEADER_TYPE)
                                hasPacketInHeader = true;
                        
                        if(!hasPacketInHeader){
                            AST *SF = new AST(std::string(VIRT_PACKET_IN_HEADER_NAME), NT_STRUCT_FIELD);
                            SF->addChild(NULL); //optAnnotations
                            SF->addChild(new AST(VIRT_PACKET_IN_HEADER_TYPE, NT_TYPE_NAME));
                            a->getChildNT(NT_STRUCT_FIELD_LIST)->addChild(SF);
                        }
                    }
                }
            }

            //Add statement at the start to set program id
            for(AST *state : parser->getChildNT(NT_PARSER_STATE_LIST)->children){
                if(state->value == "start"){
                    //Removes the extraction / advance of intrinsic metadata
                    if(parserParam == 0)
                        TofinoMerge::RemoveIngressPortMetadataExtractions(programs[p]->value, programs[p]->children[i]->getChildNT(NT_PARSER_STATE_LIST), state, programs[p]->children[i]->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[0]->value, programs[p]->children[i]->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[3]->value, true);
                    else
                        TofinoMerge::RemoveEgressPortMetadataExtractions(programs[p]->value, programs[p]->children[i]->getChildNT(NT_PARSER_STATE_LIST), state, programs[p]->children[i]->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[0]->value, programs[p]->children[i]->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[3]->value);

                    char buffer[128];
                    AST *setIDStatement = new AST("=", NT_ASSIGN_STATEMENT);
                    AST *leftSide = new AST(".", NT_NONE);
                    leftSide->addChild(new AST(METADATA_NAME, NT_NONE));
                    leftSide->addChild(new AST(METADATA_PROG_ID_FIELD_NAME, NT_NONE));
                    setIDStatement->addChild(leftSide);
                    sprintf(buffer, "%d", p + 1);
                    setIDStatement->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                    
                    AST *sl = state->getChildNT(NT_PARSER_STATEMENT_LIST);
                    if(sl == NULL){
                        sl = new AST("", NT_PARSER_STATEMENT_LIST);
                        state->children[1] = sl;
                    }
                    sl->addChildStart(setIDStatement);
                }
            }

            parser->RemoveSimpleParserStates();

            elements.push_back(parser);
        }

        AST *newExpression = new AST(".", NT_EXPRESSION);
        AST *newExpressionL = new AST(parserParam == 0 ? IG_INTRINSIC_MD_PARAM_NAME : EG_INTRINSIC_MD_PARAM_NAME, NT_EXPRESSION);
        AST *newExpressionR = new AST(parserParam == 0 ? "ingress_port" : "egress_port", NT_NONE);
        newExpression->addChild(newExpressionL);
        newExpression->addChild(newExpressionR);

        std::vector<AST*> newNodes = AST::MergeParsers(programs, elements, headersDec, parserNames[pCounter], virtualSwitchPorts, newExpression);

        delete newExpression;
        if(newNodes.size() == 0)
            return false;

        AST *parser = newNodes[newNodes.size() - 1];

        //Change the names of the parameters
        for(AST *param : parser->children[0]->children[2]->children){
            auto it = ParamNames.find(AST::RemoveMergePrefix(param->children[2]->value));
            if(it != ParamNames.end()){
                std::string val = param->value;
                if(parser->children[2] != NULL)
                    parser->children[2]->SubstituteIdentifierWithType(val, it->second, param->children[2]->value);
                if(parser->children[3] != NULL)
                    parser->children[3]->SubstituteIdentifierWithType(val, it->second, param->children[2]->value);
                param->value = it->second;
            }
        }

        AST *startState = NULL;
        for(AST *state : parser->getChildNT(NT_PARSER_STATE_LIST)->children)
            if(state->value == "start")
                startState = state;
        Assert(startState != NULL);
        Assert(startState->getChildNT(NT_PARSER_STATEMENT_LIST) == NULL);

        const std::string packetParamName = parser->children[0]->children[2]->children[0]->value;

        AST *transitionSelectExpression = new AST(".", NT_EXPRESSION);
        
        if(parserParam == 0){ //Ingress
            //Add VirtLookahead variable
            AST *tempVarDec = new AST(INGRESS_LOOKAHEAD_VAR_NAME, NT_VARIABLE_DECLARATION);
            tempVarDec->addChild(NULL); //optAnnotations
            tempVarDec->addChild(new AST(INGRESS_LOOKAHEAD_HEADER_TYPE, NT_TYPE_NAME));
            tempVarDec->addChild(NULL); //optInitializer
            if(parser->children[2] == NULL)
                parser->children[2] = new AST("", NT_PARSER_STATEMENT_LIST);
            parser->children[2]->addChild(tempVarDec);

            AST *lookaheadExpression = new AST("", NT_EXPRESSION);
            lookaheadExpression->addChild(new AST(".", NT_EXPRESSION));
            lookaheadExpression->children[0]->addChild(new AST(packetParamName, NT_NONE));
            AST *exp = new AST("lookahead", NT_EXPRESSION);
            exp->addChild(new AST("", NT_TYPE_ARGUMENT_LIST));
            exp->children[0]->addChild(new AST(INGRESS_LOOKAHEAD_HEADER_TYPE, NT_TYPE_NAME));
            exp->addChild(new AST("", NT_ARGUMENT_LIST));
            lookaheadExpression->children[0]->addChild(exp);

            AST *lookaheadAssignExpression = new AST("=", NT_ASSIGN_STATEMENT);
            lookaheadAssignExpression->addChild(new AST(INGRESS_LOOKAHEAD_VAR_NAME, NT_EXPRESSION));
            lookaheadAssignExpression->addChild(lookaheadExpression);

            if(startState->getChildNT(NT_PARSER_STATEMENT_LIST) == NULL){
                startState->children[1] = new AST("", NT_PARSER_STATEMENT_LIST);
            }
            startState->children[1]->addChild(lookaheadAssignExpression);

            transitionSelectExpression->addChild(new AST(INGRESS_LOOKAHEAD_VAR_NAME, NT_NONE));
            transitionSelectExpression->addChild(new AST("ingress_port", NT_NONE));
        }
        else{ //Egress
            //Add VirtLookahead variable
            AST *tempVarDec = new AST(EGRESS_LOOKAHEAD_VAR_NAME, NT_VARIABLE_DECLARATION);
            tempVarDec->addChild(NULL); //optAnnotations
            tempVarDec->addChild(new AST(EGRESS_LOOKAHEAD_HEADER_TYPE, NT_TYPE_NAME));
            tempVarDec->addChild(NULL); //optInitializer
            if(parser->children[2] == NULL)
                parser->children[2] = new AST("", NT_PARSER_STATEMENT_LIST);
            parser->children[2]->addChild(tempVarDec);

            AST *lookaheadExpression = new AST("", NT_EXPRESSION);
            lookaheadExpression->addChild(new AST(".", NT_EXPRESSION));
            lookaheadExpression->children[0]->addChild(new AST(packetParamName, NT_NONE));
            AST *exp = new AST("lookahead", NT_EXPRESSION);
            exp->addChild(new AST("", NT_TYPE_ARGUMENT_LIST));
            exp->children[0]->addChild(new AST(EGRESS_LOOKAHEAD_HEADER_TYPE, NT_TYPE_NAME));
            exp->addChild(new AST("", NT_ARGUMENT_LIST));
            lookaheadExpression->children[0]->addChild(exp);

            AST *lookaheadAssignExpression = new AST("=", NT_ASSIGN_STATEMENT);
            lookaheadAssignExpression->addChild(new AST(EGRESS_LOOKAHEAD_VAR_NAME, NT_EXPRESSION));
            lookaheadAssignExpression->addChild(lookaheadExpression);

            if(startState->getChildNT(NT_PARSER_STATEMENT_LIST) == NULL){
                startState->children[1] = new AST("", NT_PARSER_STATEMENT_LIST);
            }
            startState->children[1]->addChild(lookaheadAssignExpression);

            transitionSelectExpression->addChild(new AST(EGRESS_LOOKAHEAD_VAR_NAME, NT_NONE));
            transitionSelectExpression->addChild(new AST("egress_port", NT_NONE));
        }

        AST *selectExp = startState->getChildNT(NT_SELECT_EXPRESSION);
        Assert(selectExp != NULL);
        AST *expressionList = selectExp->getChildNT(NT_EXPRESSION_LIST);
        Assert(expressionList != NULL && expressionList->children.size() == 1);
        delete expressionList->children[0];
        expressionList->children[0] = transitionSelectExpression;

        //Add packet-out handling in ingress parser
        if(cpuPort > 0 && parserParam == 0){
            for(size_t s = 0; s < parser->getChildNT(NT_PARSER_STATE_LIST)->children.size(); s++){
                AST *state = parser->getChildNT(NT_PARSER_STATE_LIST)->children[s];
                if(state->value == "start"){
                    char buffer[128];

                    AST *transition = state->getChildNT(NT_SELECT_EXPRESSION);
                    Assert(transition != NULL);
                    AST *newSelectCase = new AST("VirtParsePacketOut", NT_SELECT_CASE);
                    sprintf(buffer, "%d", cpuPort);
                    newSelectCase->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                    transition->getChildNT(NT_SELECT_CASE_LIST)->addChild(newSelectCase);
                
                    AST *newState = new AST("VirtParsePacketOut", NT_PARSER_STATE);
                    newState->addChild(NULL); //optAnnotations
                    newState->addChild(new AST("", NT_PARSER_STATEMENT_LIST));
                    newState->addChild(new AST("select", NT_SELECT_EXPRESSION));
                
                    parser->getChildNT(NT_PARSER_STATE_LIST)->children.insert(parser->getChildNT(NT_PARSER_STATE_LIST)->children.begin() + s + 1, newState);
                    s++;

                    newState->getChildNT(NT_SELECT_EXPRESSION)->addChild(new AST("", NT_EXPRESSION_LIST));
                    AST *transitionKey = new AST(".", NT_EXPRESSION);
                    transitionKey->addChild(new AST(INGRESS_LOOKAHEAD_VAR_NAME, NT_EXPRESSION));
                    transitionKey->children[0]->setProgramType(VIRT_PACKET_OUT_HEADER_TYPE);
                    transitionKey->addChild(new AST(VIRT_PACKET_OUT_HEADER_PROG_ID_FIELD_NAME, NT_NONE));
                    newState->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_EXPRESSION_LIST)->addChild(transitionKey);

                    newState->getChildNT(NT_SELECT_EXPRESSION)->addChild(new AST("", NT_SELECT_CASE_LIST));
                    for(int p = 0; p < (int) programs.size(); p++){
                        sprintf(buffer, "P%d_start", p + 1);
                        AST *sc = new AST(std::string(buffer), NT_SELECT_CASE);
                        sprintf(buffer, "%d", p + 1);
                        sc->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                        newState->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_SELECT_CASE_LIST)->addChild(sc);
                    }
                }
            }

            //TODO: Uncomment this after adding packet out metadata to all packets
            TofinoMerge::AddVirtPacketOutAdvanceToParser(headersDec, parser, cpuPort);
        }

        pCounter++;

        for(AST* n: newNodes)
            nodesToAdd.push_back(n);
    }

    //Controls
    const std::vector<int> controlParams = {1, 2, 4, 5};
    const std::vector<std::string> controlNames = {"VirtIngress", "VirtIngressDeparser", "VirtEgress", "VirtEgressDeparser"};
    int controlPipeN = 0;
    for(int controlP : controlParams){
        std::vector<AST*> elements;
        std::vector<std::string> elementNames;
        for(AST* pipeline : pipelines)
            elementNames.push_back(pipeline->getChildNT(NT_ARGUMENT_LIST)->children[controlP]->children[0]->value);

        for(int p = 0; p < (int) programs.size(); p++){
            int i = 0;
            while(i < (int) programs[p]->children.size() && (programs[p]->children[i]->nType != NT_CONTROL_DECLARATION || programs[p]->children[i]->children[0]->value != elementNames[p]))
                i++;
            if(i == (int) programs[p]->children.size()){
                fprintf(stderr, "TofinoMerge: Could not find \"%s\" Control declaration\n", elementNames[p].c_str());
                return false;
            }
            elements.push_back(programs[p]->children[i]);
        }

        std::vector<AST*> newNodes = AST::MergeControls(programs, elements, headersDec, controlNames[controlPipeN], virtualSwitchPorts);
        if(newNodes.size() == 0)
            return false;

        AST* mergedControl = newNodes[newNodes.size() - 1];
        for(AST *param : mergedControl->children[0]->children[2]->children){
            std::string key = AST::RemoveMergePrefix(param->children[2]->value);
            
            auto it = ParamNames.find(key);
            if(it != ParamNames.end()){
                if(mergedControl->getChildNT(NT_CONTROL_LOCAL_DEC_LIST) != NULL){
                    mergedControl->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->SubstituteIdentifierWithType(param->value, it->second, param->children[2]->value);
                }
                if(mergedControl->getChildNT(NT_STAT_OR_DEC_LIST) != NULL)
                    mergedControl->getChildNT(NT_STAT_OR_DEC_LIST)->SubstituteIdentifierWithType(param->value, it->second, param->children[2]->value);
                param->value = it->second;
            }
        }

        //Add egress port check
        if(controlPipeN == 0){
            if(mergedControl->children[2] == NULL)
                mergedControl->children[2] = new AST("", NT_CONTROL_LOCAL_DEC_LIST);
            if(mergedControl->children[3] == NULL)
                mergedControl->children[3] = new AST("", NT_STAT_OR_DEC_LIST);
            AST *controlLDs = mergedControl->children[2];

            AST *actionOK= new AST("VirtOutPortOK", NT_ACTION_DECLARATION);
            controlLDs->addChild(actionOK);
            actionOK->addChild(new AST("", NT_ANNOTATION_LIST));
            actionOK->children[0]->addChild(new AST("hidden", NT_ANNOTATION));
            actionOK->children[0]->children[0]->addChild(NULL);
            actionOK->addChild(NULL); //parameterList
            actionOK->addChild(NULL);

            AST *actionSendToCPU;
            if(cpuPort > 0){
                actionSendToCPU = new AST("VirtSendToCPU", NT_ACTION_DECLARATION);
                controlLDs->addChild(actionSendToCPU);
                actionSendToCPU->addChild(new AST("", NT_ANNOTATION_LIST));
                actionSendToCPU->children[0]->addChild(new AST("hidden", NT_ANNOTATION));
                actionSendToCPU->children[0]->children[0]->addChild(NULL);
                actionSendToCPU->addChild(new AST("", NT_PARAMETER_LIST));
                actionSendToCPU->children[1]->addChild(new AST("pID", NT_PARAMETER));
                actionSendToCPU->children[1]->children[0]->addChild(NULL); //annotations
                actionSendToCPU->children[1]->children[0]->addChild(NULL); //direction
                actionSendToCPU->children[1]->children[0]->addChild(new AST("bit", NT_BASE_TYPE));
                actionSendToCPU->children[1]->children[0]->children[2]->addChild(new AST("8", NT_LITERAL));
                AST *actionSendToCPUBlock = new AST("", NT_STAT_OR_DEC_LIST);
                actionSendToCPU->addChild(actionSendToCPUBlock);
                const std::string headersParamName = mergedControl->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[0]->value;
                actionSendToCPUBlock->addChild(new AST("", NT_METHOD_CALL_STATEMENT));
                actionSendToCPUBlock->children[0]->addChild(new AST(".", NT_NONE));
                actionSendToCPUBlock->children[0]->children[0]->addChild(new AST(".", NT_EXPRESSION));
                actionSendToCPUBlock->children[0]->children[0]->children[0]->addChild(new AST(headersParamName, NT_EXPRESSION));
                actionSendToCPUBlock->children[0]->children[0]->children[0]->addChild(new AST(std::string(VIRT_PACKET_IN_HEADER_NAME), NT_NONE));
                actionSendToCPUBlock->children[0]->children[0]->addChild(new AST("setValid", NT_NONE));
                actionSendToCPUBlock->children[0]->addChild(NULL);
                actionSendToCPUBlock->children[0]->addChild(NULL);
                actionSendToCPUBlock->addChild(new AST("==", NT_ASSIGN_STATEMENT));
                actionSendToCPUBlock->children[1]->addChild(new AST(".", NT_NONE));
                actionSendToCPUBlock->children[1]->children[0]->addChild(new AST(".", NT_NONE));
                actionSendToCPUBlock->children[1]->children[0]->children[0]->addChild(new AST(headersParamName, NT_NONE));
                actionSendToCPUBlock->children[1]->children[0]->children[0]->addChild(new AST(std::string(VIRT_PACKET_IN_HEADER_NAME), NT_NONE));
                actionSendToCPUBlock->children[1]->children[0]->addChild(new AST(std::string(VIRT_PACKET_IN_HEADER_PROG_ID_FIELD_NAME), NT_NONE));
                actionSendToCPUBlock->children[1]->addChild(new AST("pID", NT_EXPRESSION));
            }

            AST *actionDropUnmappedPort = new AST("VirtOutPortWrong", NT_ACTION_DECLARATION);
            controlLDs->addChild(actionDropUnmappedPort);
            actionDropUnmappedPort->addChild(new AST("", NT_ANNOTATION_LIST));
            actionDropUnmappedPort->children[0]->addChild(new AST("hidden", NT_ANNOTATION));
            actionDropUnmappedPort->children[0]->children[0]->addChild(NULL);
            actionDropUnmappedPort->addChild(NULL); //parameterList
            actionDropUnmappedPort->addChild(new AST("", NT_STAT_OR_DEC_LIST));
            AST *assignStatement = new AST("=", NT_ASSIGN_STATEMENT);
            assignStatement->addChild(new AST(".", NT_NONE));
            assignStatement->children[0]->addChild(new AST(IG_INTRINSIC_MD_DEPARSER_PARAM_NAME, NT_NONE));
            assignStatement->children[0]->addChild(new AST("drop_ctl", NT_NONE));
            assignStatement->addChild(new AST("1", NT_EXPRESSION));
            actionDropUnmappedPort->children[2]->addChild(assignStatement);

            AST *tableProgramID = new AST("VirtTableCheckProgramID", NT_TABLE_DECLARATION);
            controlLDs->addChild(tableProgramID);
            tableProgramID->addChild(new AST("", NT_ANNOTATION_LIST));
            tableProgramID->children[0]->addChild(new AST("hidden", NT_ANNOTATION));
            tableProgramID->children[0]->children[0]->addChild(NULL);
            AST *tablePL = new AST("", NT_TABLE_PROPERTY_LIST);
            tableProgramID->addChild(tablePL);
            tablePL->addChild(new AST("key", NT_TABLE_PROPERTY));
            AST *key = new AST("exact", NT_KEY_ELEMENT);
            key->addChild(new AST(".", NT_EXPRESSION));
            key->children[0]->addChild(new AST(IG_INTRINSIC_MD_TM_PARAM_NAME, NT_EXPRESSION));
            key->children[0]->addChild(new AST("ucast_egress_port", NT_NONE));
            key->addChild(NULL); //optAnnotations
            AST *key2 = new AST("exact", NT_KEY_ELEMENT);
            key2->addChild(new AST(".", NT_EXPRESSION));
            key2->children[0]->addChild(new AST(METADATA_NAME, NT_EXPRESSION));
            key2->children[0]->addChild(new AST(METADATA_PROG_ID_FIELD_NAME, NT_NONE));
            key2->addChild(NULL); //optAnnotations
            tablePL->children[0]->addChild(new AST("", NT_KEY_ELEMENT_LIST));
            tablePL->children[0]->children[0]->addChild(key);
            tablePL->children[0]->children[0]->addChildStart(key2);
            tablePL->addChild(new AST("actions", NT_TABLE_PROPERTY));
            tablePL->children[1]->addChild(new AST("", NT_ACTION_LIST));
            tablePL->children[1]->children[0]->addChild(new AST(actionOK->value, NT_ACTION_REF));
            if(cpuPort > 0)
                tablePL->children[1]->children[0]->addChild(new AST(actionSendToCPU->value, NT_ACTION_REF));
            tablePL->children[1]->children[0]->addChild(new AST(actionDropUnmappedPort->value, NT_ACTION_REF));
            tablePL->children[1]->children[0]->children[0]->addChild(NULL);
            tablePL->children[1]->children[0]->children[1]->addChild(NULL);
            if(cpuPort > 0)
                tablePL->children[1]->children[0]->children[2]->addChild(NULL);

            tablePL->addChild(new AST("const default_action", NT_TABLE_PROPERTY));
            tablePL->children[2]->addChild(new AST(actionDropUnmappedPort->value, NT_EXPRESSION));

            char buffer[128];
            tablePL->addChild(new AST("const entries", NT_TABLE_PROPERTY));
            tablePL->children[3]->addChild(new AST("", NT_ENTRIES_LIST));
            int tSize = 0;
            for(int vsN = 0; vsN < (int) virtualSwitchPorts.size(); vsN++){
                tSize = tSize + ((int) virtualSwitchPorts[vsN].size());
                for(int port : virtualSwitchPorts[vsN]){
                    AST *entry = new AST("", NT_ENTRY);
                    tablePL->children[3]->children[0]->addChild(entry);
                    entry->addChild(new AST("", NT_TUPLE_KEYSET_EXPRESSION));
                    entry->children[0]->addChild(new AST("", NT_SIMPLE_EXPRESSION_LIST));
                    sprintf(buffer, "%d", vsN + 1);
                    entry->children[0]->children[0]->addChild(new AST(buffer, NT_EXPRESSION));
                    sprintf(buffer, "%d", port);
                    entry->children[0]->children[0]->addChild(new AST(buffer, NT_EXPRESSION));
                    entry->addChild(new AST(actionOK->value, NT_ACTION_REF));
                    entry->children[1]->addChild(NULL);
                    entry->addChild(NULL); //optAnnotations
                }

                //CPU-Port
                if(cpuPort > 0){
                    tSize++;
                    AST *entry = new AST("", NT_ENTRY);
                    tablePL->children[3]->children[0]->addChild(entry);
                    entry->addChild(new AST("", NT_TUPLE_KEYSET_EXPRESSION));
                    entry->children[0]->addChild(new AST("", NT_SIMPLE_EXPRESSION_LIST));
                    sprintf(buffer, "%d", vsN + 1);
                    entry->children[0]->children[0]->addChild(new AST(buffer, NT_EXPRESSION));
                    sprintf(buffer, "%s", "CPU_PORT");
                    entry->children[0]->children[0]->addChild(new AST(buffer, NT_EXPRESSION));
                    entry->addChild(new AST(actionSendToCPU->value, NT_ACTION_REF));
                    sprintf(buffer, "%d", vsN + 1);
                    entry->children[1]->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                    entry->addChild(NULL); //optAnnotations
                }
            }

            sprintf(buffer, "%d", tSize);
            tablePL->addChild(new AST("size", NT_TABLE_PROPERTY));
            tablePL->children[4]->addChild(new AST(buffer, NT_LITERAL));

            AST* tableApplyStatement = new AST("", NT_METHOD_CALL_STATEMENT);
            mergedControl->children[3]->addChild(tableApplyStatement);
            tableApplyStatement->addChild(new AST(".", NT_NONE));
            tableApplyStatement->children[0]->addChild(new AST("VirtTableCheckProgramID", NT_NONE));
            tableApplyStatement->children[0]->addChild(new AST("apply", NT_NONE));
            tableApplyStatement->addChild(NULL);
            tableApplyStatement->addChild(NULL);

            mergedControl->RemoveSwitchStatements();
        }
        else if(controlPipeN == 1){
            try {
                TofinoMerge::RemoveSwitchCaseEmitsFromDeparser(mergedControl, programs.size());
            }
            catch (ExceptionWrongEmitOrder eweo){
                for(size_t pi = 0; pi < newNodes.size(); pi++)
                    delete newNodes[pi];
                for(size_t pi = 0; pi < nodesToAdd.size(); pi++)
                    delete nodesToAdd[pi];

                eweo.deparserName = AST::RemoveMergePrefix(elementNames[eweo.program]);
                throw eweo;
            }

            //Add virtPacketIn Header emit
            if(cpuPort > 0){
                const std::string packetOutParamName = mergedControl->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[0]->value;
                const std::string headersParamName = mergedControl->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[1]->value;
                AST *virtPacketInEmit = new AST("", NT_METHOD_CALL_STATEMENT);
                virtPacketInEmit->addChild(new AST(".", NT_NONE));
                virtPacketInEmit->children[0]->addChild(new AST(packetOutParamName, NT_NONE));
                virtPacketInEmit->children[0]->children[0]->programType = mergedControl->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[0]->getChildNT(NT_TYPE_NAME)->value;
                virtPacketInEmit->children[0]->addChild(new AST("emit", NT_NONE));
                virtPacketInEmit->addChild(NULL);
                virtPacketInEmit->addChild(new AST("", NT_ARGUMENT_LIST));
                virtPacketInEmit->children[2]->addChild(new AST("", NT_ARGUMENT));
                virtPacketInEmit->children[2]->children[0]->addChild(new AST(".", NT_EXPRESSION));
                virtPacketInEmit->children[2]->children[0]->children[0]->addChild(new AST(headersParamName, NT_NONE));
                virtPacketInEmit->children[2]->children[0]->children[0]->addChild(new AST(std::string(VIRT_PACKET_IN_HEADER_NAME), NT_NONE));
                Assert(mergedControl->getChildNT(NT_STAT_OR_DEC_LIST) != NULL);
                mergedControl->getChildNT(NT_STAT_OR_DEC_LIST)->addChildStart(virtPacketInEmit);
            }

            mergedControl->RemoveEmptySwitches();
            mergedControl->RemoveSwitchStatements();
            TofinoMerge::RemoveConditionalChecksum(mergedControl);
        }
        else if(controlPipeN == 3){
            try {
                TofinoMerge::RemoveSwitchCaseEmitsFromDeparser(mergedControl, programs.size());
            }
            catch (ExceptionWrongEmitOrder eweo){
                for(size_t pi = 0; pi < newNodes.size(); pi++)
                    delete newNodes[pi];
                for(size_t pi = 0; pi < nodesToAdd.size(); pi++)
                    delete nodesToAdd[pi];
                
                eweo.deparserName = AST::RemoveMergePrefix(elementNames[eweo.program]);
                throw eweo;
            }
            mergedControl->RemoveEmptySwitches();
            mergedControl->RemoveSwitchStatements();
            TofinoMerge::RemoveConditionalChecksum(mergedControl);
        }

        controlPipeN++;

        for(AST* n: newNodes)
            nodesToAdd.push_back(n);
    }

    //Construct new pipeline
    AST *newPipe = pipelines[pipelines.size() - 1]->Clone();
    newPipe->value = SINGLE_PIPELINE_NAME;
    newPipe->getChildNT(NT_TYPE_NAME)->value = "Pipeline";
    for(int i = 0; i < (int) parserParams.size(); i++)
        newPipe->getChildNT(NT_ARGUMENT_LIST)->children[parserParams[i]]->children[0]->value = parserNames[i];
    for(int i = 0; i < (int) controlParams.size(); i++)
        newPipe->getChildNT(NT_ARGUMENT_LIST)->children[controlParams[i]]->children[0]->value = controlNames[i];
    nodesToAdd.push_back(newPipe);

    for(int i = 0; i < (int) programs.size(); i++){
        int cc = 0;
        while(cc < (int) programs[i]->children.size()){
            if(programs[i]->children[cc] == pipelines[i]){
                programs[i]->children.erase(programs[i]->children.begin() + cc);
                cc--;
            }
            cc++;
        }
        delete pipelines[i];
    }

    for(AST* n : nodesToAdd)
        programs[programs.size() - 1]->addChild(n);

    return true;
}

AST* TofinoMerge::CreateLookaheadHeaderIngress(const bool withProgramID){
    AST *ingressMetadataHeader = new AST(INGRESS_LOOKAHEAD_HEADER_TYPE, NT_HEADER_TYPE_DECLARATION);
    ingressMetadataHeader->addChild(NULL); //optAnnotations
    ingressMetadataHeader->addChild(NULL); //optTypeParameters

    AST *sfl = new AST("", NT_STRUCT_FIELD_LIST);
    ingressMetadataHeader->addChild(sfl);

    AST *sf;
    AST *optAnnotations;

    //Ingress Intrinsic Metadata
    sf = new AST("resubmit_flag", NT_STRUCT_FIELD);
    sf->addChild(NULL); //optAnnotations
    sf->addChild(new AST("bit", NT_BASE_TYPE));
    sf->getChildNT(NT_BASE_TYPE)->addChild(new AST("1", NT_LITERAL));
    sfl->addChild(sf);

    sf = new AST("_pad1", NT_STRUCT_FIELD);
    optAnnotations = new AST("", NT_ANNOTATION_LIST);
    optAnnotations->addChild(new AST("padding", NT_ANNOTATION));
    optAnnotations->children[0]->addChild(NULL); //body
    sf->addChild(optAnnotations);
    sf->addChild(new AST("bit", NT_BASE_TYPE));
    sf->getChildNT(NT_BASE_TYPE)->addChild(new AST("1", NT_LITERAL));
    sfl->addChild(sf);

    sf = new AST("packet_version", NT_STRUCT_FIELD);
    sf->addChild(NULL); //optAnnotations
    sf->addChild(new AST("bit", NT_BASE_TYPE));
    sf->getChildNT(NT_BASE_TYPE)->addChild(new AST("2", NT_LITERAL));
    sfl->addChild(sf);

    sf = new AST("_pad2", NT_STRUCT_FIELD);
    optAnnotations = new AST("", NT_ANNOTATION_LIST);
    optAnnotations->addChild(new AST("padding", NT_ANNOTATION));
    optAnnotations->children[0]->addChild(NULL); //body
    sf->addChild(optAnnotations);
    sf->addChild(new AST("bit", NT_BASE_TYPE));
    sf->getChildNT(NT_BASE_TYPE)->addChild(new AST("3", NT_LITERAL));
    sfl->addChild(sf);

    sf = new AST("ingress_port", NT_STRUCT_FIELD);
    sf->addChild(NULL); //optAnnotations
    sf->addChild(new AST("bit", NT_BASE_TYPE));
    sf->getChildNT(NT_BASE_TYPE)->addChild(new AST("9", NT_LITERAL));
    sfl->addChild(sf);

    sf = new AST("ingress_mac_tstamp", NT_STRUCT_FIELD);
    sf->addChild(NULL); //optAnnotations
    sf->addChild(new AST("bit", NT_BASE_TYPE));
    sf->getChildNT(NT_BASE_TYPE)->addChild(new AST("48", NT_LITERAL));
    sfl->addChild(sf);

    //Ingress Port Metadata
    sf = new AST("PortMetadata", NT_STRUCT_FIELD);
    sf->addChild(NULL); //optAnnotations
    sf->addChild(new AST("bit", NT_BASE_TYPE));
    sf->getChildNT(NT_BASE_TYPE)->addChild(new AST("64", NT_LITERAL));
    sfl->addChild(sf);

    //Program ID
    if(withProgramID){
        sf = new AST("programID", NT_STRUCT_FIELD);
        sf->addChild(NULL); //optAnnotations
        sf->addChild(new AST("bit", NT_BASE_TYPE));
        sf->getChildNT(NT_BASE_TYPE)->addChild(new AST("8", NT_LITERAL));
        sfl->addChild(sf);
    }

    return ingressMetadataHeader;
}

AST* TofinoMerge::CreateLookaheadHeaderEgress(const bool withProgramID){
    AST *egressMetadataHeader = new AST(EGRESS_LOOKAHEAD_HEADER_TYPE, NT_HEADER_TYPE_DECLARATION);
    egressMetadataHeader->addChild(NULL); //optAnnotations
    egressMetadataHeader->addChild(NULL); //optTypeParameters

    AST *sfl = new AST("", NT_STRUCT_FIELD_LIST);
    egressMetadataHeader->addChild(sfl);

    AST *sf;
    AST *optAnnotations;

    //Egress Intrinsic Metadata
    sf = new AST("_pad0", NT_STRUCT_FIELD);
    optAnnotations = new AST("", NT_ANNOTATION_LIST);
    optAnnotations->addChild(new AST("padding", NT_ANNOTATION));
    optAnnotations->children[0]->addChild(NULL); //body
    sf->addChild(optAnnotations);
    sf->addChild(new AST("bit", NT_BASE_TYPE));
    sf->getChildNT(NT_BASE_TYPE)->addChild(new AST("7", NT_LITERAL));
    sfl->addChild(sf);

    sf = new AST("egress_port", NT_STRUCT_FIELD);
    sf->addChild(NULL); //optAnnotations
    sf->addChild(new AST("bit", NT_BASE_TYPE));
    sf->getChildNT(NT_BASE_TYPE)->addChild(new AST("9", NT_LITERAL));
    sfl->addChild(sf);

    sf = new AST("_pad1", NT_STRUCT_FIELD);
    optAnnotations = new AST("", NT_ANNOTATION_LIST);
    optAnnotations->addChild(new AST("padding", NT_ANNOTATION));
    optAnnotations->children[0]->addChild(NULL); //body
    sf->addChild(optAnnotations);
    sf->addChild(new AST("bit", NT_BASE_TYPE));
    sf->getChildNT(NT_BASE_TYPE)->addChild(new AST("200", NT_LITERAL));
    sfl->addChild(sf);

    //Program ID
    if(withProgramID){
        sf = new AST("programID", NT_STRUCT_FIELD);
        sf->addChild(NULL); //optAnnotations
        sf->addChild(new AST("bit", NT_BASE_TYPE));
        sf->getChildNT(NT_BASE_TYPE)->addChild(new AST("8", NT_LITERAL));
        sfl->addChild(sf);
    }

    return egressMetadataHeader;
}

bool TofinoMerge::RemoveSwitchInstantiation(AST *ast){
    int i = 0;
    while(i < (int) ast->children.size() && (ast->children[i]->nType != NT_INSTANTIATION || ast->children[i]->value != "main"))
        i++;
    if(i == (int) ast->children.size()){
        fprintf(stderr, "TofinoMerge: Could not find \"main\" Switch instantiation\n");
        return false;
    }

    delete ast->children[i];
    ast->children.erase(ast->children.begin() + i);
    return true;
}

bool TofinoMerge::GetPipelines(AST *ast, std::vector<AST*> *Pipelines){
    int i = 0;
    while(i < (int) ast->children.size() && (ast->children[i]->nType != NT_INSTANTIATION || ast->children[i]->value != "main"))
        i++;
    if(i == (int) ast->children.size()){
        fprintf(stderr, "TofinoMerge: Could not find \"main\" Switch instantiation\n");
        return false;
    }

    for(AST *argument : ast->children[i]->getChildNT(NT_ARGUMENT_LIST)->children){
        std::string instName = argument->children[0]->value;
        int j = 0;
        while(j < (int) ast->children.size() && (ast->children[j]->nType != NT_INSTANTIATION || ast->children[j]->value != instName))
            j++;
        if(j == (int) ast->children.size()){
            fprintf(stderr, "TofinoMerge: Could not find \"%s\" Pipeline instantiation\n", instName.c_str());
            return false;
        }

        if(ast->children[j]->getChildNT(NT_ARGUMENT_LIST) == NULL || ast->children[j]->getChildNT(NT_ARGUMENT_LIST)->children.size() != 6){
            fprintf(stderr, "TofinoMerge: \"%s\" Pipeline instantiation does not have 6 arguments\n", instName.c_str());
            return false;
        }

        Pipelines->push_back(ast->children[j]);
    }

    return true;
}

void TofinoMerge::RemoveSwitchCaseEmitsFromDeparser(AST* deparser, const size_t nPrograms){
    AST *parameterList = deparser->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST);
    Assert(parameterList != NULL);
    Assert(parameterList->children.size() >= 3);

    const std::string PacketOutParamName = parameterList->children[0]->value;
    
    AST *applyStatOrDecList = deparser->getChildNT(NT_STAT_OR_DEC_LIST);
    if(applyStatOrDecList == NULL)
        return;

    std::vector<std::vector<AST*>> emitStatementsClones(nPrograms);

    size_t idx = 0;
    while(idx < applyStatOrDecList->children.size()){
        AST *statement = applyStatOrDecList->children[idx];
        if(statement->nType == NT_METHOD_CALL_STATEMENT){
            if(statement->children[0]->value == "." && statement->children[0]->children[0]->value == PacketOutParamName && statement->children[0]->children[1]->value == "emit"){
                for(size_t i = 0; i < emitStatementsClones.size(); i++){
                    emitStatementsClones[i].push_back(statement->Clone());
                }
                delete statement;
                applyStatOrDecList->children.erase(applyStatOrDecList->children.begin() + idx);
                idx--;
            }
        }
        else if(statement->nType == NT_SWITCH_STATEMENT){
            AST *SCL = statement->getChildNT(NT_SWITCH_CASE_LIST);
            for(size_t si = 0; si < SCL->children.size(); si++){
                AST *switchCase = SCL->children[si];
                
                Assert(switchCase->getChildNT(NT_EXPRESSION) != NULL && IPLBase::intFromStr(switchCase->getChildNT(NT_EXPRESSION)->str()) == (int) si + 1);

                //Check for fall through
                size_t sj = si;
                if(switchCase->children.size() == 1){
                    size_t sj = si + 1;
                    while(sj < SCL->children.size() && SCL->children[sj]->children.size() == 1)
                        sj++;
                    Assert(sj < SCL->children.size());
                }

                switchCase = SCL->children[sj];

                AST *caseSODList = switchCase->getChildNT(NT_STAT_OR_DEC_LIST);
                size_t sodI = 0;
                while(sodI < caseSODList->children.size()){
                    AST *curChild = caseSODList->children[sodI];
                    if(curChild->nType == NT_METHOD_CALL_STATEMENT){
                        if(curChild->children[0]->value == "." && curChild->children[0]->children[0]->value == PacketOutParamName && curChild->children[0]->children[1]->value == "emit"){
                            emitStatementsClones[si].push_back(curChild->Clone());
                            delete curChild;
                            caseSODList->children.erase(caseSODList->children.begin() + sodI);
                            sodI--;
                        }
                    }

                    sodI++;
                }
            }
        }

        idx++;
    }

    std::vector<AST*> emitOrder;
    for(size_t i = 0; i < emitStatementsClones.size(); i++){
        size_t nextEmitPos = 0;
        for(size_t k = 0; k < emitStatementsClones[i].size(); k++){
            AST *emit = emitStatementsClones[i][k];
            size_t j = 0;
            while(j < emitOrder.size() && !AST::Equals(emitOrder[j], emit))
                j++;
            if(j != emitOrder.size()){
                if(j < nextEmitPos){
                    std::string msg = "Emit order error in deparser: " + deparser->value + "\n";
                    Log::MSG(msg, LOG_LEVEL_SHORT);

                    for(size_t i = 0; i < emitStatementsClones.size(); i++){
                        for(AST *ast : emitStatementsClones[i])
                            delete ast;
                    }

                    for(size_t i = 0; i < emitOrder.size(); i++)
                        delete emitOrder[i];
                    
                    Log::Reset();
                    throw ExceptionWrongEmitOrder {(int) i, deparser->value, (int) k - 1};
                }
                else{
                    nextEmitPos = j + 1;
                }
            }
            else{
                emitOrder.insert(emitOrder.begin() + nextEmitPos, emit->Clone());
                nextEmitPos++;
            }
        }
    }

    for(AST *emit : emitOrder)
        applyStatOrDecList->addChild(emit);

    for(size_t i = 0; i < emitStatementsClones.size(); i++){
        for(AST *ast : emitStatementsClones[i])
            delete ast;
    }
}

void TofinoMerge::RemoveConditionalChecksum(AST *deparser){
    AST *applyBlock = deparser->getChildNT(NT_STAT_OR_DEC_LIST);
    if(applyBlock == NULL)
        return;

    size_t i = 0;
    while(i < applyBlock->children.size()){
        AST *statement = applyBlock->children[i];
        if(statement->nType == NT_CONDITIONAL_STATEMENT && AST::isConditionVirtProgramIDCheck(statement->children[0])){
            size_t k = i + 1;
            if(statement->children[1] != NULL){
                if(statement->children[1]->nType == NT_STAT_OR_DEC_LIST){
                    for(size_t j = 0; j < statement->children[1]->children.size(); j++){
                        if(statement->children[1]->children[j] != NULL){
                            applyBlock->children.insert(applyBlock->children.begin() + k, statement->children[1]->children[j]->Clone());
                            applyBlock->children[k]->parent = applyBlock;
                            k++;
                        }
                    }
                }
                else{
                    applyBlock->children.insert(applyBlock->children.begin() + k, statement->children[1]->Clone());
                    applyBlock->children[k]->parent = applyBlock;
                    k++;
                }
            }
            if(statement->children[2] != NULL){
                if(statement->children[2]->nType == NT_STAT_OR_DEC_LIST){
                    for(size_t j = 0; j < statement->children[2]->children.size(); j++){
                        if(statement->children[2]->children[j] != NULL){
                            applyBlock->children.insert(applyBlock->children.begin() + k, statement->children[2]->children[j]->Clone());
                            applyBlock->children[k]->parent = applyBlock;
                            k++;
                        }
                    }
                }
                else{
                    applyBlock->children.insert(applyBlock->children.begin() + k, statement->children[2]->Clone());
                    applyBlock->children[k]->parent = applyBlock;
                    k++;
                }
            }
            delete statement;
            applyBlock->children.erase(applyBlock->children.begin() + i);
        }
        i++;
    }
}

void TofinoMerge::AddVirtPacketOutAdvanceToParser(const std::vector<AST*>& headersDec, AST *parser, const int cpuPort){
    const static int TOFINO_METADATA_SIZE = 128;

    AST *startState = parser->getChildNT(NT_PARSER_STATE_LIST)->FindChildrenByValue("start", {NT_PARSER_STATE});
    Assert(startState != NULL);

    std::unordered_set<AST*> visitedStates;
    TofinoMerge::_AddVirtPacketOutAdvaceToParserRec(TOFINO_METADATA_SIZE, headersDec, parser, startState, visitedStates, cpuPort);
}

void TofinoMerge::_AddVirtPacketOutAdvaceToParserRec(int remainingBits, const std::vector<AST*>& headersDec, AST *parser, AST *state, std::unordered_set<AST*>& visitedStates, const int cpuPort){
    if(remainingBits <= 0)
        return;

    visitedStates.insert(state);

    AST *statementList = state->getChildNT(NT_PARSER_STATEMENT_LIST);
    if(statementList != NULL){
        size_t i = 0;
        while(i < statementList->children.size()){
            AST *statement = statementList->children[i];
            if(statement->nType == NT_METHOD_CALL_STATEMENT){
                if(statement->FindDescendantByValue("extract", {}) != NULL){
                    AST *argumentList = statement->getChildNT(NT_ARGUMENT_LIST);
                    Assert(argumentList != NULL && argumentList->children.size() == 1);
                    const std::string programType = argumentList->children[0]->children[0]->programType;
                    int sizeExtracted = AST::CalculateStructSize(headersDec, programType);
                    remainingBits = remainingBits - sizeExtracted;
                    Assert(remainingBits >= 0);
                }
                else if(statement->FindDescendantByValue("advance", {}) != NULL){
                    AST *argumentList = statement->getChildNT(NT_ARGUMENT_LIST);
                    Assert(argumentList != NULL && argumentList->children.size() == 1);
                    const std::string programType = argumentList->children[0]->children[0]->programType;
                    int sizeAdvanced = IPLBase::intFromStr(argumentList->children[0]->children[0]->value.c_str());
                    remainingBits = remainingBits - sizeAdvanced;
                    Assert(remainingBits >= 0);
                }
            }

            if(statement->FindDescendantByValue("port_metadata_unpack", {})){        
                remainingBits = 0;
            }

            //Skips the VirtPacketOutBits, considering they are appended to every packet
            if(remainingBits == 0){
                const int bitsToAdvance = AST::CalculateStructSize(headersDec, VIRT_PACKET_OUT_HEADER_TYPE);
                char buffer[128];
                sprintf(buffer, "%d", bitsToAdvance);
                
                AST *newStatement = new AST("", NT_METHOD_CALL_STATEMENT);
                
                AST *lhs = new AST(".", NT_NONE);
                lhs->addChild(new AST(PI_PARAM_NAME, NT_EXPRESSION));
                lhs->children[0]->programType = PI_PARAM_TYPE;
                lhs->addChild(new AST("advance", NT_NONE));

                AST *argumentList = new AST("", NT_ARGUMENT_LIST);
                argumentList->addChild(new AST("", NT_ARGUMENT));
                argumentList->children[0]->addChild(new AST(std::string(buffer), NT_EXPRESSION));

                newStatement->addChild(lhs);
                newStatement->addChild(NULL); //typeArgumentList
                newStatement->addChild(argumentList);
                
                //statementList->children.insert(statementList->children.begin() + i + 1, newStatement);

                AST *newState = new AST(state->value + "_advance_packet_out", NT_PARSER_STATE);
                newState->addChild(NULL); //optAnnotations

                newState->addChild(new AST("", NT_PARSER_STATEMENT_LIST));
                newState->children[1]->addChild(newStatement);

                newState->addChild(new AST(state->value + "_after_advance", NT_NAME_STATE_EXPRESSION));

                AST *newState2 = new AST(state->value + "_after_advance", NT_PARSER_STATE);
                newState2->addChild(NULL); //optAnnotations

                if(i + 1 < statementList->children.size()){
                    newState2->addChild(new AST("", NT_PARSER_STATEMENT_LIST));
                    for(size_t j = i + 1; j < statementList->children.size(); j++){
                        newState2->children[1]->addChild(statementList->children[j]->Clone());
                        delete statementList->children[j];
                    }
                    statementList->children.erase(statementList->children.begin() + i + 1, statementList->children.end());
                }
                else{
                    newState2->addChild(NULL); //parserStatementList
                }
                newState2->addChild(state->children[2]->Clone());

                delete state->children[2];

                AST *newTransition = new AST("select", NT_SELECT_EXPRESSION);
                newTransition->addChild(new AST("", NT_EXPRESSION_LIST));

                newTransition->children[0]->addChild(new AST(".", NT_EXPRESSION));

                AST *transitionSelectExpression = newTransition->children[0]->children[0];
                transitionSelectExpression->addChild(new AST(INGRESS_LOOKAHEAD_VAR_NAME, NT_NONE));
                transitionSelectExpression->addChild(new AST("ingress_port", NT_NONE));

                newTransition->addChild(new AST("", NT_SELECT_CASE_LIST));
                newTransition->children[1]->addChild(new AST(state->value + "_advance_packet_out", NT_SELECT_CASE));
                sprintf(buffer, "%d", cpuPort);
                newTransition->children[1]->children[0]->addChild(new AST(buffer, NT_EXPRESSION));
                newTransition->children[1]->addChild(new AST(state->value + "_after_advance", NT_SELECT_CASE));
                newTransition->children[1]->children[1]->addChild(new AST("default", NT_SIMPLE_KEYSET_DEFAULT));
                
                state->children[2] = newTransition;

                //Insert the new states after the current state
                AST *stateList = parser->getChildNT(NT_PARSER_STATE_LIST);
                size_t idx = 0;
                while(idx < stateList->children.size() && stateList->children[idx]->value != state->value)
                    idx++;
                Assert(idx < stateList->children.size());
                stateList->children.insert(stateList->children.begin() + idx + 1, newState);
                stateList->children.insert(stateList->children.begin() + idx + 2, newState2);

                return;
            }
            i++;
        }
    }

    AST *nameStateExpression = state->getChildNT(NT_NAME_STATE_EXPRESSION);
    if(nameStateExpression != NULL){
        if(nameStateExpression->value != "accept" && nameStateExpression->value != "reject"){
            AST *nextState = parser->getChildNT(NT_PARSER_STATE_LIST)->FindChildrenByValue(nameStateExpression->value, {NT_PARSER_STATE});
            Assert(nextState != NULL);
            if(visitedStates.find(nextState) == visitedStates.end())
                TofinoMerge::_AddVirtPacketOutAdvaceToParserRec(remainingBits, headersDec, parser, nextState, visitedStates, cpuPort);
        }
    }
    else {
        AST *transitionSelectExpression = state->getChildNT(NT_SELECT_EXPRESSION);
        Assert(transitionSelectExpression != NULL);
        AST *scl = transitionSelectExpression->getChildNT(NT_SELECT_CASE_LIST);
        for(AST *sc : scl->children){
            if(sc->value != "accept" && sc->value != "reject"){
                AST *nextState = parser->getChildNT(NT_PARSER_STATE_LIST)->FindChildrenByValue(sc->value, {NT_PARSER_STATE});
                Assert(nextState != NULL);
                if(visitedStates.find(nextState) == visitedStates.end())
                    TofinoMerge::_AddVirtPacketOutAdvaceToParserRec(remainingBits, headersDec, parser, nextState, visitedStates, cpuPort);
            }
        }
    }
}