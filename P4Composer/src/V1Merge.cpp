#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../include/AST.h"
#include "../include/V1Merge.h"
#include "../include/Log.h"
#include "../include/CFG.h"

#include "../include/IPLBase.hpp"

#define PROGRAM_ID_TYPE_BITS_STR "8"
#define METADATA_NAME "VirtMetadata"
#define METADATA_TYPE_NAME "VirtMetadataType"
#define METADATA_PROG_ID_FIELD_NAME "VirtProgramID"

#define PI_PARAM_TYPE "packet_in"
#define PI_PARAM_NAME "pkt"
#define PO_PARAM_TYPE "packet_out"
#define PO_PARAM_NAME "pkt"
#define STD_METADATA_PARAM_TYPE "standard_metadata_t"
#define STD_METADATA_PARAM_NAME "standard_metadata"

#define VIRT_ACTION_SEND_TO_CPU_NAME "VirtSendToCPU"

void V1RenameVirtMetadataVariables(AST *ast){
    if(ast->getProgramType() == METADATA_TYPE_NAME)
        ast->setValue(METADATA_NAME);
    for(AST *c : ast->getChildren())
        if(c != NULL)
            V1RenameVirtMetadataVariables(c);
}

void V1RemoveTempProgramCode(AST *ast){
    if(ast->getNodeType() == NT_EXPRESSION && ast->getValue() == TEMP_PROGRAM_CODE){
        ast->setValue(".");
        ast->addChild(new AST(METADATA_NAME, NT_EXPRESSION));
        ast->addChild(new AST(METADATA_PROG_ID_FIELD_NAME, NT_NONE));
        ast->getChildren()[1]->setProgramType(METADATA_TYPE_NAME);
    }
    for(AST *c : ast->getChildren())
        if(c != NULL)
            V1RemoveTempProgramCode(c);
}

bool V1Merge::MergeV1(const std::vector<AST*> programs, const std::vector<AST*>& headersDec, const std::vector<std::set<int>>& virtualSwitchPorts, const int cpuPort){
    CFG::SetMetadataProgIDName(std::string(METADATA_NAME "." METADATA_PROG_ID_FIELD_NAME));
    
    std::string msg = "Merging V1 Programs:";
    for(AST *program : programs)
        msg = msg + " \"" + program->value + "\"";
    Log::MSG(msg, LOG_LEVEL_SHORT);
    Log::Push();

    std::vector<std::vector<AST*>> Pipelines(programs.size());

    for(int i = 0; i < (int) programs.size(); i++){
        if(!V1Merge::GetPipeline(programs[i], &Pipelines[i])){
            Log::Pop();
            return false;
        }
    }
    
    bool result = V1Merge::MergePipelines(programs, Pipelines, headersDec, virtualSwitchPorts, cpuPort);
    if(!result){
        Log::Pop();
        return false;
    }

    AST* LastProg = programs[programs.size() - 1];

    //Change Switch Local Metadata parameters to METADATA_NAME
    const std::unordered_map<std::string, int> VirtPipelineStructNames = {{"VirtParser", 2}, {"VirtVerifyChecksum", 1}, {"VirtIngress", 1}, {"VirtEgress", 1}, {"VirtComputeChecksum", 1}};
    for(int i = 0; i < (int) LastProg->children.size(); i++){
        if(LastProg->children[i]->nType == NT_PARSER_DECLARATION && VirtPipelineStructNames.find(LastProg->children[i]->getChildNT(NT_PARSER_TYPE_DECLARATION)->value) != VirtPipelineStructNames.end()){
            const std::string vMetadataHeaderName = LastProg->children[i]->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[VirtPipelineStructNames.find(LastProg->children[i]->getChildNT(NT_PARSER_TYPE_DECLARATION)->value)->second]->getChildNT(NT_TYPE_NAME)->value;
            LastProg->children[i]->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[VirtPipelineStructNames.find(LastProg->children[i]->getChildNT(NT_PARSER_TYPE_DECLARATION)->value)->second]->value = METADATA_NAME;
            
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
                        headersDec[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->addChild(newField);
                    }
                }
                else{
                    AST *newSFL = new AST("", NT_STRUCT_FIELD_LIST);
                    newSFL->addChild(newField);
                    headersDec[pp]->children[headersDec[pp]->children.size() - 1] = newSFL;
                }
                LastProg->SubstituteTypeName(headersDec[pp]->value, METADATA_TYPE_NAME);
                headersDec[pp]->value = METADATA_TYPE_NAME;
            }
            else{
                if(LastProg->children[pp]->getChildNT(NT_STRUCT_FIELD_LIST) == NULL)
                    LastProg->children[pp]->children[2] = new AST("", NT_STRUCT_FIELD_LIST);
                
                //LastProg->children[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->addChild(newField);
                bool alreadyHas = false;
                for(int sf = 0; sf < (int) LastProg->children[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->children.size(); sf++){
                    if(LastProg->children[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->children[sf]->value == METADATA_PROG_ID_FIELD_NAME)
                        alreadyHas = true;
                }
                if(!alreadyHas)
                    LastProg->children[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->addChild(newField);

                LastProg->SubstituteTypeName(LastProg->children[pp]->value, METADATA_TYPE_NAME);
                LastProg->children[pp]->value = METADATA_TYPE_NAME;
            }
        }
        else if(LastProg->children[i]->nType == NT_CONTROL_DECLARATION && VirtPipelineStructNames.find(LastProg->children[i]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value) != VirtPipelineStructNames.end()){
            const std::string vMetadataHeaderName = LastProg->children[i]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[VirtPipelineStructNames.find(LastProg->children[i]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value)->second]->getChildNT(NT_TYPE_NAME)->value;
            LastProg->children[i]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[VirtPipelineStructNames.find(LastProg->children[i]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value)->second]->value = METADATA_NAME;
            
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
                    if(!alreadyHas)
                        headersDec[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->addChild(newField->Clone());
                }
                else{
                    AST *newSFL = new AST("", NT_STRUCT_FIELD_LIST);
                    newSFL->addChild(newField->Clone());
                    headersDec[pp]->children[headersDec[pp]->children.size() - 1] = newSFL;
                }
                LastProg->SubstituteTypeName(headersDec[pp]->value, METADATA_TYPE_NAME);
                headersDec[pp]->value = METADATA_TYPE_NAME;
            }
            else{
                if(LastProg->children[pp]->getChildNT(NT_STRUCT_FIELD_LIST) == NULL)
                    LastProg->children[pp]->children[2] = new AST("", NT_STRUCT_FIELD_LIST);

                LastProg->children[pp]->getChildNT(NT_STRUCT_FIELD_LIST)->addChild(newField->Clone());
                LastProg->SubstituteTypeName(LastProg->children[pp]->value, METADATA_TYPE_NAME);
                LastProg->children[pp]->value = METADATA_TYPE_NAME;
            }

            delete newField;
        }
    }
    V1RenameVirtMetadataVariables(LastProg);
    //Change generic TEMP_PROGRAM_CODE to METADATA_NAME.METADATA_PROG_ID_FIELD_NAME
    V1RemoveTempProgramCode(LastProg);

    Log::Pop();
    return true;
}

bool V1Merge::MergePipelines(const std::vector<AST*>& programs, const std::vector<std::vector<AST*>>& pipelines, const std::vector<AST*>& headersDec, const std::vector<std::set<int>>& virtualSwitchPorts, const int cpuPort){
    std::vector<AST*> nodesToAdd;

    const std::vector<std::string> VirtPipeArgNames = {"VirtParser", "VirtVerifyChecksum", "VirtIngress", "VirtEgress", "VirtComputeChecksum", "VirtDeparser"};

    //Parser
    AST *newExpression = new AST(".", NT_EXPRESSION);
    AST *newExpressionL = new AST(STD_METADATA_PARAM_NAME, NT_EXPRESSION);
    AST *newExpressionR = new AST("ingress_port", NT_NONE);
    newExpression->addChild(newExpressionL);
    newExpression->addChild(newExpressionR);

    if(cpuPort > 0){
        for(AST *a : headersDec){
            if(a->value == pipelines[pipelines.size() - 1][0]->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[1]->getChildNT(NT_TYPE_NAME)->value){
                AST *SF = new AST(std::string(VIRT_PACKET_IN_HEADER_NAME), NT_STRUCT_FIELD);
                SF->addChild(NULL); //optAnnotations
                SF->addChild(new AST(VIRT_PACKET_IN_HEADER_TYPE, NT_TYPE_NAME));
                a->getChildNT(NT_STRUCT_FIELD_LIST)->addChild(SF);
            }
        }
    }

    std::vector<AST*> elements;
    for(int pc = 0; pc < (int) pipelines.size(); pc++)
        elements.push_back(pipelines[pc][0]);
    std::vector<AST*> newNodes = AST::MergeParsers(programs, elements, headersDec, VirtPipeArgNames[0], virtualSwitchPorts, newExpression, true);
    delete newExpression;

    if(newNodes.size() == 0) {
        exit(1);
    }

    for(AST* n: newNodes)
        nodesToAdd.push_back(n);

    //Change the second parameter type to "VirtHeaders"
    AST* Parser = newNodes[newNodes.size() - 1];
    Assert(Parser->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST) != NULL && Parser->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children.size() > 2);
    const std::string virtTypeHeadersName = Parser->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[1]->getChildNT(NT_TYPE_NAME)->value;
    Parser->getChildNT(NT_PARSER_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[1]->getChildNT(NT_TYPE_NAME)->value = V1MODEL_VIRT_HEADER_TYPE;
    AST* VirtHeadersAST = NULL;
    for(AST *n : newNodes){
        if(n->nType == NT_STRUCT_TYPE_DECLARATION && n->value == virtTypeHeadersName)
            VirtHeadersAST = n;
    }
    Assert(VirtHeadersAST != NULL);
    VirtHeadersAST->value = V1MODEL_VIRT_HEADER_TYPE;

    if(Parser->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST) != NULL)
        Parser->getChildNT(NT_PARSER_LOCAL_ELEMENT_LIST)->RenameProgramTypes(virtTypeHeadersName, V1MODEL_VIRT_HEADER_TYPE);  
    if(Parser->getChildNT(NT_PARSER_STATE_LIST) != NULL)
        Parser->getChildNT(NT_PARSER_STATE_LIST)->RenameProgramTypes(virtTypeHeadersName, V1MODEL_VIRT_HEADER_TYPE);

    //Controls
    for(int elementC = 1; elementC < (int) pipelines[0].size(); elementC++){
        elements.clear();
        for(int pc = 0; pc < (int) pipelines.size(); pc++)
            elements.push_back(pipelines[pc][elementC]);
        newNodes =  AST::MergeControls(programs, elements, headersDec, VirtPipeArgNames[elementC], virtualSwitchPorts);
        
        if(newNodes.size() == 0)
            return false;

        AST* mergedControl = newNodes[newNodes.size() - 1];

        for(int p = 0; p < (int) programs.size(); p++){
            int i = 0;
            while(i < (int) programs[p]->children.size()){
                if(programs[p]->children[i] == elements[p]){
                    programs[p]->children.erase(programs[p]->children.begin() + i);
                    i--;
                }
                i++;
            }
            delete elements[p];
        }

        //Modify the checksum calculation / verification to comply with v1model restrictions
        if(elementC == 1 || elementC == 4){
            if(mergedControl->getChildNT(NT_STAT_OR_DEC_LIST) != NULL){
                if(mergedControl->getChildNT(NT_STAT_OR_DEC_LIST)->getChildNT(NT_SWITCH_STATEMENT) != NULL){
                    AST *SCL = mergedControl->getChildNT(NT_STAT_OR_DEC_LIST)->getChildNT(NT_SWITCH_STATEMENT)->getChildNT(NT_SWITCH_CASE_LIST);
                    int pc = 0;
                    while(pc < (int) SCL->children.size() && SCL->children[pc]->getChildNT(NT_STAT_OR_DEC_LIST) == NULL){
                        pc++;
                    }
                    Assert(pc != (int) SCL->children.size());
                    AST *newStatList = SCL->children[pc]->getChildNT(NT_STAT_OR_DEC_LIST)->Clone();
                    pc = 0;
                    while(mergedControl->children[pc]== NULL || mergedControl->children[pc]->nType != NT_STAT_OR_DEC_LIST)
                        pc++;
                    delete mergedControl->children[pc];
                    mergedControl->children[pc] = newStatList;
                }
            }
        }
        //Add metadata programID table on the ingress pipeline
        else if(elementC == 2){
            if(mergedControl->children[2] == NULL)
                mergedControl->children[2] = new AST("", NT_CONTROL_LOCAL_DEC_LIST);
            if(mergedControl->children[3] == NULL)
                mergedControl->children[3] = new AST("", NT_STAT_OR_DEC_LIST);
            AST *controlLDs = mergedControl->children[2];
            AST *controlBody = mergedControl->children[3];

            AST *actionSetProgramID = new AST("VirtSetProgramID", NT_ACTION_DECLARATION);
            controlLDs->addChild(actionSetProgramID);
            actionSetProgramID->addChild(new AST("", NT_ANNOTATION_LIST));
            actionSetProgramID->children[0]->addChild(new AST("hidden", NT_ANNOTATION));
            actionSetProgramID->children[0]->children[0]->addChild(NULL);
            actionSetProgramID->addChild(new AST("", NT_PARAMETER_LIST));
            actionSetProgramID->addChild(new AST("", NT_STAT_OR_DEC_LIST));
            AST *parameter = new AST("programID", NT_PARAMETER);
            actionSetProgramID->children[1]->addChild(parameter);
            parameter->addChild(NULL); //optAnnotations
            parameter->addChild(NULL); //direction
            parameter->addChild(new AST("bit", NT_BASE_TYPE));
            parameter->children[2]->addChild(new AST(PROGRAM_ID_TYPE_BITS_STR, NT_LITERAL));
            AST *assignStatement = new AST("=", NT_ASSIGN_STATEMENT);
            actionSetProgramID->children[2]->addChild(assignStatement);
            assignStatement->addChild(new AST(".", NT_NONE));
            assignStatement->children[0]->addChild(new AST(METADATA_NAME, NT_NONE));
            assignStatement->children[0]->addChild(new AST(METADATA_PROG_ID_FIELD_NAME, NT_NONE));
            assignStatement->addChild(new AST("programID", NT_EXPRESSION));

            AST *actionDropUnmappedPort = new AST("VirtDropUnmappedPort", NT_ACTION_DECLARATION);
            controlLDs->addChild(actionDropUnmappedPort);
            actionDropUnmappedPort->addChild(new AST("", NT_ANNOTATION_LIST));
            actionDropUnmappedPort->children[0]->addChild(new AST("hidden", NT_ANNOTATION));
            actionDropUnmappedPort->children[0]->children[0]->addChild(NULL);
            actionDropUnmappedPort->addChild(NULL); //parameterList
            actionDropUnmappedPort->addChild(new AST("", NT_STAT_OR_DEC_LIST));
            
            AST* callDrop = new AST("", NT_METHOD_CALL_STATEMENT);
            actionDropUnmappedPort->children[2]->addChild(callDrop);
            callDrop->addChild(new AST("mark_to_drop", NT_NONE));
            callDrop->addChild(NULL);
            callDrop->addChild(new AST("", NT_ARGUMENT_LIST));
            callDrop->children[2]->addChild(new AST("", NT_ARGUMENT));
            callDrop->children[2]->children[0]->addChild(new AST(STD_METADATA_PARAM_NAME, NT_EXPRESSION));

            AST *tableProgramID = new AST("VirtTableSetProgramID", NT_TABLE_DECLARATION);
            controlLDs->addChild(tableProgramID);
            AST *annotationList = new AST("", NT_ANNOTATION_LIST);
            annotationList->addChild(new AST("noWarn", NT_ANNOTATION));
            annotationList->children[0]->addChild(new AST("", NT_ANNOTATION_TOKEN_LIST));
            annotationList->children[0]->children[0]->addChild(new AST("\"mismatch\"", NT_ANNOTATION_TOKEN));
            annotationList->addChild(new AST("hidden", NT_ANNOTATION));
            annotationList->children[1]->addChild(NULL);
            tableProgramID->addChild(annotationList);
            AST *tablePL = new AST("", NT_TABLE_PROPERTY_LIST);
            tableProgramID->addChild(tablePL);
            tablePL->addChild(new AST("key", NT_TABLE_PROPERTY));
            AST *key = new AST("exact", NT_KEY_ELEMENT);
            key->addChild(new AST(".", NT_EXPRESSION));
            key->children[0]->addChild(new AST(STD_METADATA_PARAM_NAME, NT_EXPRESSION));
            key->children[0]->addChild(new AST("ingress_port", NT_NONE));
            key->addChild(NULL); //optAnnotations
            tablePL->children[0]->addChild(new AST("", NT_KEY_ELEMENT_LIST));
            tablePL->children[0]->children[0]->addChild(key);
            tablePL->addChild(new AST("actions", NT_TABLE_PROPERTY));
            tablePL->children[1]->addChild(new AST("", NT_ACTION_LIST));
            tablePL->children[1]->children[0]->addChild(new AST(actionSetProgramID->value, NT_ACTION_REF));
            tablePL->children[1]->children[0]->addChild(new AST(actionDropUnmappedPort->value, NT_ACTION_REF));
            tablePL->children[1]->children[0]->children[0]->addChild(NULL);
            tablePL->children[1]->children[0]->children[1]->addChild(NULL);

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
                    sprintf(buffer, "%d", port);
                    entry->addChild(new AST(buffer, NT_EXPRESSION));
                    entry->addChild(new AST(actionSetProgramID->value, NT_ACTION_REF));
                    sprintf(buffer, "%d", vsN + 1);
                    entry->children[1]->addChild(new AST(buffer, NT_EXPRESSION));
                    entry->addChild(NULL); //optAnnotations
                }
            }

            sprintf(buffer, "%d", tSize);
            tablePL->addChild(new AST("size", NT_TABLE_PROPERTY));
            tablePL->children[4]->addChild(new AST(buffer, NT_LITERAL));

            AST* tableApplyStatement = new AST("", NT_METHOD_CALL_STATEMENT);
            controlBody->addChildStart(tableApplyStatement);
            tableApplyStatement->addChild(new AST(".", NT_NONE));
            tableApplyStatement->children[0]->addChild(new AST("VirtTableSetProgramID", NT_NONE));
            tableApplyStatement->children[0]->addChild(new AST("apply", NT_NONE));
            tableApplyStatement->addChild(NULL);
            tableApplyStatement->addChild(NULL);
        }
        //Add Egress Port-Check
        else if(elementC == 3){
            if(mergedControl->children[2] == NULL)
                mergedControl->children[2] = new AST("", NT_CONTROL_LOCAL_DEC_LIST);
            if(mergedControl->children[3] == NULL)
                mergedControl->children[3] = new AST("", NT_STAT_OR_DEC_LIST);
            AST *controlLDs = mergedControl->children[2];

            AST *actionOK = new AST("VirtOutPortOK", NT_ACTION_DECLARATION);
            controlLDs->addChild(actionOK);
            actionOK->addChild(new AST("", NT_ANNOTATION_LIST));
            actionOK->children[0]->addChild(new AST("hidden", NT_ANNOTATION));
            actionOK->children[0]->children[0]->addChild(NULL);
            actionOK->addChild(NULL); //parameterList
            actionOK->addChild(NULL);

            if(cpuPort > 0){
                AST *actionSendToCPU = new AST(VIRT_ACTION_SEND_TO_CPU_NAME, NT_ACTION_DECLARATION);
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
            AST* callDrop = new AST("", NT_METHOD_CALL_STATEMENT);
            callDrop->addChild(new AST("mark_to_drop", NT_NONE));
            callDrop->addChild(NULL);
            callDrop->addChild(new AST("", NT_ARGUMENT_LIST));
            callDrop->children[2]->addChild(new AST("", NT_ARGUMENT));
            callDrop->children[2]->children[0]->addChild(new AST(STD_METADATA_PARAM_NAME, NT_EXPRESSION));
            actionDropUnmappedPort->children[2]->addChild(callDrop);

            AST *tableProgramID = new AST("VirtTableCheckProgramID", NT_TABLE_DECLARATION);
            controlLDs->addChild(tableProgramID);
            AST *annotationList = new AST("", NT_ANNOTATION_LIST);
            annotationList->addChild(new AST("noWarn", NT_ANNOTATION));
            annotationList->children[0]->addChild(new AST("", NT_ANNOTATION_TOKEN_LIST));
            annotationList->children[0]->children[0]->addChild(new AST("\"mismatch\"", NT_ANNOTATION_TOKEN));
            annotationList->addChild(new AST("hidden", NT_ANNOTATION));
            annotationList->children[1]->addChild(NULL);
            tableProgramID->addChild(annotationList);
            AST *tablePL = new AST("", NT_TABLE_PROPERTY_LIST);
            tableProgramID->addChild(tablePL);
            tablePL->addChild(new AST("key", NT_TABLE_PROPERTY));
            AST *key = new AST("exact", NT_KEY_ELEMENT);
            key->addChild(new AST(".", NT_EXPRESSION));
            key->children[0]->addChild(new AST(STD_METADATA_PARAM_NAME, NT_EXPRESSION));
            key->children[0]->addChild(new AST("egress_port", NT_NONE));
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
            tablePL->children[1]->children[0]->addChild(new AST(actionDropUnmappedPort->value, NT_ACTION_REF));
            tablePL->children[1]->children[0]->children[0]->addChild(NULL);
            tablePL->children[1]->children[0]->children[1]->addChild(NULL);
            
            if(cpuPort > 0){
                tablePL->children[1]->children[0]->addChild(new AST(VIRT_ACTION_SEND_TO_CPU_NAME, NT_ACTION_REF));
                tablePL->children[1]->children[0]->children[2]->addChild(NULL);
            }

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
                    //sprintf(buffer, "%d", cpuPort);
                    sprintf(buffer, "%s", "CPU_PORT");
                    entry->children[0]->children[0]->addChild(new AST(buffer, NT_EXPRESSION));
                    entry->addChild(new AST(VIRT_ACTION_SEND_TO_CPU_NAME, NT_ACTION_REF));
                    sprintf(buffer, "%d", vsN + 1);
                    entry->children[1]->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                    entry->addChild(NULL); //optAnnotations
                }
            }

            sprintf(buffer, "%d", tSize);
            tablePL->addChild(new AST("size", NT_TABLE_PROPERTY));
            tablePL->children[4]->addChild(new AST(buffer, NT_LITERAL));

            AST *newControlVar = new AST("VirtEgressOk", NT_VARIABLE_DECLARATION);
            newControlVar->addChild(NULL); //annotations
            newControlVar->addChild(new AST("bool", NT_BASE_TYPE));
            newControlVar->addChild(new AST(".", NT_EXPRESSION));
            newControlVar->children[2]->addChild(new AST(".", NT_EXPRESSION));
            newControlVar->children[2]->children[0]->addChild(new AST("VirtTableCheckProgramID", NT_NONE));
            newControlVar->children[2]->children[0]->addChild(new AST("apply", NT_NONE));
            newControlVar->children[2]->children[0]->addChild(NULL);
            newControlVar->children[2]->addChild(new AST("hit", NT_NONE));
            AST *newCondition = new AST("IF", NT_CONDITIONAL_STATEMENT);
            newCondition->addChild(new AST("VirtEgressOk", NT_EXPRESSION));
            newCondition->addChild(mergedControl->children[3]);
            newCondition->addChild(NULL);
            mergedControl->children[3] = new AST("", NT_STAT_OR_DEC_LIST);
            mergedControl->children[3]->addChild(newControlVar);
            mergedControl->children[3]->addChild(newCondition);
        }
        //We add invalidation statements to egress to mantain program logic
        else if(elementC == 5 && mergedControl->getChildNT(NT_STAT_OR_DEC_LIST) != NULL){
            //If there is no switch statement then all deparsers are equal and we are good
            if(mergedControl->getChildNT(NT_STAT_OR_DEC_LIST)->getChildNT(NT_SWITCH_STATEMENT) != NULL){
                std::vector<std::vector<AST*>> programEmitStatements(programs.size());
                std::vector<AST*> uniqueStatements;
                AST *SCL = mergedControl->getChildNT(NT_STAT_OR_DEC_LIST)->getChildNT(NT_SWITCH_STATEMENT)->getChildNT(NT_SWITCH_CASE_LIST);
                for(int sc = 0; sc < (int) SCL->children.size(); sc++){
                    int prog = IPLBase::intFromStr(SCL->children[sc]->getChildNT(NT_EXPRESSION)->value.c_str()) - 1;
                    Assert(prog >= 0);
                    int effSC = sc;
                    while(effSC < (int) SCL->children.size() && SCL->children[effSC]->getChildNT(NT_STAT_OR_DEC_LIST) == NULL)
                        effSC++;
                    if(effSC < (int) SCL->children.size()){
                        for(AST *statement : SCL->children[effSC]->getChildNT(NT_STAT_OR_DEC_LIST)->children){
                            //Deparser can only contain emit calls.
                            if(statement->nType != NT_METHOD_CALL_STATEMENT || statement->children[0]->children[1]->value != "emit"){
                                fprintf(stderr, "Error: Illegal statement (\"%s\" line %d - V1Model only allow \"emit\" method calls on deparser)\n", programs[sc]->str(), statement->lineNumber);
                                exit(1);
                            }
                            programEmitStatements[prog].push_back(statement->Clone());

                            bool unique = true;
                            for(size_t x = 0; x < uniqueStatements.size(); x++)
                                if(AST::Equals(statement, uniqueStatements[x]))
                                    unique = false;
                                
                            if(unique)
                                uniqueStatements.push_back(statement->Clone());
                        }
                    }
                }

                int dps = 0;
                while(dps < (int) nodesToAdd.size() && (nodesToAdd[dps]->nType != NT_CONTROL_DECLARATION || nodesToAdd[dps]->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value != VirtPipeArgNames[3]))
                    dps++;
                Assert(dps < (int) nodesToAdd.size());
                AST *EgressControl = nodesToAdd[dps];

                AST* newEgressSwitchStatement = new AST("", NT_SWITCH_STATEMENT);
                newEgressSwitchStatement->addChild(new AST(".", NT_EXPRESSION));
                newEgressSwitchStatement->children[0]->addChild(new AST(METADATA_NAME, NT_NONE));
                newEgressSwitchStatement->children[0]->addChild(new AST(METADATA_PROG_ID_FIELD_NAME, NT_NONE));
                newEgressSwitchStatement->addChild(new AST("", NT_SWITCH_CASE_LIST));
                for(int p = 0; p < (int) programs.size(); p++){
                    AST *SC = new AST("", NT_SWITCH_CASE);
                    newEgressSwitchStatement->children[1]->addChild(SC);
                    char buffer[32];
                    sprintf(buffer, "%d", p+1);
                    SC->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                    SC->addChild(new AST("", NT_STAT_OR_DEC_LIST));
                    for(AST *us : uniqueStatements){
                        bool hasStatement = false;
                        for(AST *statement : programEmitStatements[p])
                            if(AST::Equals(statement, us))
                                hasStatement = true;
                        
                        if(!hasStatement){
                            AST* invalidateStatement = new AST("", NT_METHOD_CALL_STATEMENT);
                            invalidateStatement->addChild(new AST(".", NT_NONE));
                            invalidateStatement->addChild(NULL);
                            invalidateStatement->addChild(NULL); //setInvalid arguments
                            invalidateStatement->children[0]->addChild(us->getChildNT(NT_ARGUMENT_LIST)->getChildNT(NT_ARGUMENT)->getChildNT(NT_EXPRESSION)->Clone());
                            invalidateStatement->children[0]->children[0]->children[0]->value = EgressControl->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getChildNT(NT_PARAMETER_LIST)->children[0]->value;
                            invalidateStatement->children[0]->addChild(new AST("setInvalid", NT_NONE));
                            SC->children[1]->addChild(invalidateStatement);
                        }
                    }
                }
                EgressControl->getChildNT(NT_STAT_OR_DEC_LIST)->getChildNT(NT_CONDITIONAL_STATEMENT)->children[1]->addChild(newEgressSwitchStatement);
                
                V1Merge::OrderUniqueEmitStatements(uniqueStatements, programEmitStatements);

                for(auto vec : programEmitStatements)
                    for(AST *s : vec)
                        delete s;

                delete mergedControl->getChildNT(NT_STAT_OR_DEC_LIST)->children[0];
                mergedControl->getChildNT(NT_STAT_OR_DEC_LIST)->children.clear();
                for(AST *us : uniqueStatements)
                    mergedControl->getChildNT(NT_STAT_OR_DEC_LIST)->addChild(us);
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
        }

        for(AST* n: newNodes)
            nodesToAdd.push_back(n);
    }

    //Construct new pipeline
    AST *newPipe = new AST("main", NT_INSTANTIATION);
    newPipe->addChild(NULL); //annotations
    newPipe->addChild(new AST("V1Switch", NT_TYPE_NAME));
    newPipe->addChild(new AST("", NT_ARGUMENT_LIST));
    for(int i = 0; i < (int) VirtPipeArgNames.size(); i++){
        newPipe->children[2]->addChild(new AST("", NT_ARGUMENT));
        newPipe->children[2]->children[i]->addChild(new AST(VirtPipeArgNames[i], NT_EXPRESSION));
        newPipe->children[2]->children[i]->children[0]->addChild(NULL);
    }
    nodesToAdd.push_back(newPipe);

    for(AST* n : nodesToAdd)
        programs[programs.size() - 1]->addChild(n);

    return true;
}

void V1Merge::OrderUniqueEmitStatements(std::vector<AST*>& uniqueStatements, const std::vector<std::vector<AST*>>& programEmitStatements){
    std::unordered_map<AST*, std::set<size_t>> statementsToPrograms;
    for(AST *a : uniqueStatements)
        statementsToPrograms.emplace(a , std::set<size_t>());

    for(size_t p = 0; p < programEmitStatements.size(); p++){
        size_t lastHeaderPos = 0;
        for(size_t i = 0; i < programEmitStatements[p].size(); i++){
            size_t j = 0;
            while(j < uniqueStatements.size() && !AST::Equals(programEmitStatements[p][i], uniqueStatements[j]))
                j++;
            Assert(j < uniqueStatements.size());
            statementsToPrograms.find(uniqueStatements[j])->second.insert(p);

            if(j >= lastHeaderPos){
                lastHeaderPos = j;
            }
            else{
                while(j < lastHeaderPos){
                    bool canPass = true;
                    for(size_t pn : statementsToPrograms.find(uniqueStatements[j])->second)
                        if(pn != p && statementsToPrograms.find(uniqueStatements[j + 1])->second.find(pn) != statementsToPrograms.find(uniqueStatements[j + 1])->second.end())
                            canPass = false;
                    
                    if(canPass){
                        AST *aux = uniqueStatements[j];
                        uniqueStatements[j] = uniqueStatements[j + 1];
                        uniqueStatements[j + 1] = aux;
                    }
                    else{
                        fprintf(stderr, "Error: Deparser emit order incompatible between programs!\n");
                        exit(1);
                    }
                    j++;
                }
            }
        }
    }
}

void V1Merge::RenameParams(AST *program){
    const static std::unordered_map<std::string, std::string> ParamNames = {
        {PI_PARAM_TYPE, PI_PARAM_NAME},
        {PO_PARAM_TYPE, PO_PARAM_NAME},
        {STD_METADATA_PARAM_TYPE, STD_METADATA_PARAM_NAME}
    };
    
    for(AST *c : program->children){
        if((c->nType == NT_PARSER_DECLARATION || c->nType == NT_CONTROL_DECLARATION) && c->children[0]->getChildNT(NT_PARAMETER_LIST) != NULL){
            for(AST *param : c->children[0]->getChildNT(NT_PARAMETER_LIST)->children){
                auto it = ParamNames.find(param->children[2]->value);
                if(it != ParamNames.end()){
                    for(int i = 1; i < (int) c->children.size(); i++){
                        if(c->children[i] != NULL)
                            c->children[i]->SubstituteIdentifierWithType(param->value, it->second, it->first);
                    }
                    param->value = it->second;
                }
            }
        }
    }
}

bool V1Merge::GetPipeline(AST *ast, std::vector<AST*> *Pipeline){
    int i = 0;
    while(i < (int) ast->children.size() && (ast->children[i]->nType != NT_INSTANTIATION || ast->children[i]->value != "main"))
        i++;

    if(i == (int) ast->children.size()){
        fprintf(stderr, "V1Merge: Could not find \"main\" V1Switch instantiation in program %s\n", ast->str());
        return false;
    }

    if(ast->children[i]->getChildNT(NT_ARGUMENT_LIST) == NULL || ast->children[i]->getChildNT(NT_ARGUMENT_LIST)->children.size() != 6){
        fprintf(stderr, "V1Merge: (\"main\" - %s, line %d) Pipeline instantiation does not have 6 arguments\n", ast->str(), ast->children[i]->lineNumber);
        return false;
    }

    for(AST *argument : ast->children[i]->getChildNT(NT_ARGUMENT_LIST)->children){
        std::string instName = argument->children[0]->value;
        int j = 0;
        while(j < (int) ast->children.size() && ((ast->children[j]->nType != NT_PARSER_DECLARATION && ast->children[j]->nType != NT_CONTROL_DECLARATION) || ast->children[j]->children[0]->value != instName))
            j++;
        if(j == (int) ast->children.size()){
            fprintf(stderr, "V1Merge: Could not find \"%s\" instantiation in program %s\n", instName.c_str(), ast->str());
            return false;
        }

        Pipeline->push_back(ast->children[j]);
    }

    delete ast->children[i];
    ast->children.erase(ast->children.begin() + i);

    return true;
}