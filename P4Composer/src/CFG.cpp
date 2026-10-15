#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../include/AST.h"
#include "../include/SymbolTable.h"
#include "../include/CFG.h"

#define VIRTMAP_FILE "VirtMap.cfg"
#define P4INFO_FILE "merged.p4.p4info.txt"

#define ACTION_P4INFO_MSB 0x01000000
#define TABLE_P4INFO_MSB 0x02000000
#define CONTROLLER_HEADER_P4INFO_MSB 0x04000000

std::string CFGRemovePrefix(const std::string& s){
    return s.substr(s.find("_") + 1);
}

int CFGfindPrefix(const std::string& str){
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

std::string CFGRemoveAllPrefixes(std::string str){
    std::vector<std::string> separations = IPLBase::split(str, '.');
    std::string result;
    for(std::string& s : separations){
        int prefix = CFGfindPrefix(s);
        s = s.substr(prefix + 1);
        result = result + s + ".";
    }
    if(result.back() == '.')
        result.pop_back();
    return result;
}

CFG::CFG(){
    m_architecture = P4A_UNDEFINED;
    m_outputDir = "output/";
}

void CFG::SetOutputDir(const std::string& outputDir){
    CFG::Get().m_outputDir = outputDir;
    while(CFG::Get().m_outputDir.back() == '/')
        CFG::Get().m_outputDir.pop_back();
}

CFG::~CFG(){
    //Check if every VirtShared table has an owner
    for(const SharedTableData& std : m_sharedTableDatas){
        if(std.owner == -1){
            fprintf(stderr, "Error: %s table \"%s\" does not have an owner\n", VIRT_SHARED_ANNOTATION, std.alias.c_str());
            exit(1);
        }
    }

    //Check if no program asked to write on a virtual table without permission
    for(int program = 0; program < (int) m_programTableDatas.size(); program++){
        for(int ptd = 0; ptd < (int) m_programTableDatas[program].size(); ptd++){
            if(!m_programTableDatas[program][ptd].owner && m_programTableDatas[program][ptd].canWrite){
                int i = 0;
                while(i < (int) m_vTableDatas.size() && !(m_vTableDatas[i].controlBlockName == m_programTableDatas[program][ptd].vTableControlBL && m_vTableDatas[i].name == m_programTableDatas[program][ptd].vTableName)){
                    i++;
                }
                if(i < (int) m_vTableDatas.size() && m_vTableDatas[i].vtype == VTT_SHARED){
                    int j = 0;
                    while(j < (int) m_sharedTableDatas.size() && m_sharedTableDatas[j].vtdIDX != i)
                        j++;
                    if(j < (int) m_sharedTableDatas.size() && !m_sharedTableDatas[j].nonOwnerCanWrite){
                        fprintf(stderr, "Error: Program \"%s\" requested permission to write on %s table \"%s\" but it is not granted\n", m_programNames[program].c_str(), VIRT_SHARED_ANNOTATION, m_sharedTableDatas[j].alias.c_str());
                        exit(1);
                    }
                }
            }
        }
    }

    FILE *fp = fopen((m_outputDir + "/" + VIRTMAP_FILE).c_str(), "w");
    
    fprintf(fp, "VTables:\n\n");
    for(const VirtTableData& data : m_vTableDatas){
        fprintf(fp, "%s.%s [", data.controlBlockName.c_str(), data.name.c_str());
        switch(data.vtype){
            case VTT_SHARED:
                fprintf(fp, "SHARED, ");
                break;
            case VTT_HIDDEN_SHARED:
                fprintf(fp, "HIDDEN_SHARED, ");
                break;
            case VTT_NOT_SHARED:
                fprintf(fp, "NOT_SHARED, ");
                break;
        }
        fprintf(fp, "[");
        int pc = 0;
        for(int p : data.programs){
            fprintf(fp, "%d", p);
            if(pc != (int) data.programs.size() - 1)
                fprintf(fp, ", ");
            pc++;
        }
        fprintf(fp, "], %d]\n", data.totalSize);
    }

    fprintf(fp, "\n%s Tables:\n", VIRT_SHARED_ANNOTATION);
    for(const SharedTableData& std : m_sharedTableDatas){
        fprintf(fp, "%s [%s.%s, %d, %s]\n", std.alias.c_str(), m_vTableDatas[std.vtdIDX].controlBlockName.c_str(), m_vTableDatas[std.vtdIDX].name.c_str(), std.owner, std.nonOwnerCanWrite ? "true" : "false");
    }

    for(int p = 0; p < (int) m_programTableDatas.size(); p++){
        fprintf(fp, "\n%d:\n", p);
        for(const ProgramTableData& ptd : m_programTableDatas[p]){
            fprintf(fp, "%s.%s [%s, %s.%s, %s, %s, %d]\n", CFGRemovePrefix(ptd.controlBlockName).c_str(), ptd.programName.c_str(), ptd.annotatedName.c_str(), ptd.vTableControlBL.c_str(), ptd.vTableName.c_str(), ptd.owner ? "true" : "false", ptd.canWrite ? "true" : "false", ptd.programSize);
        }
    }

    fprintf(fp, "\nActions:\n");
    for(int p = 0; p < (int) m_programActionDatas.size(); p++){
        fprintf(fp, "\n%d:\n", p);
        for(const ProgramActionData& pad : m_programActionDatas[p]){
            fprintf(fp, "%s%s [%s]: %s%s\n", (pad.controlBlockName == "") ? "" : (CFGRemovePrefix(pad.controlBlockName) + ".").c_str(), pad.originalProgramName.c_str(), pad.annotatedName.c_str(), (pad.vActionControlBL == "" ? "" : pad.vActionControlBL + ".").c_str(), pad.vActionName.c_str());
        }
    }

    fclose(fp);

    for(SharedTableData& std : m_sharedTableDatas)
        delete std.tableCopy;

    for(AST* ast : m_programASTs)
        delete ast;
}

CFG& CFG::Get(){
    static CFG instance;
    return instance;
}

void CFG::SetProgramNames(const std::vector<std::string>& programNames){
    CFG::Get().m_programNames = programNames;
}

void CFG::SetProgramASTs(const std::vector<AST*> programASTs){
    for(AST* ast : CFG::Get().m_programASTs)
        delete ast;
    
    CFG::Get().m_programASTs.resize(programASTs.size());
    for(int i = 0; i < (int) programASTs.size(); i++)
        CFG::Get().m_programASTs[i] = programASTs[i]->Clone();
}

void CFG::SetArchitecture(const P4Architecture arch){
    CFG::Get().m_architecture = arch;
}

void CFG::SetMetadataProgIDName(const std::string& metadataProgIDFullName){
    CFG::Get().m_metadataProgIDFullName = metadataProgIDFullName;
}

void CFG::AddVirtTableMatchFields(AST *merged){
    for(AST *c : merged->children){
        if(c->nType == NT_CONTROL_DECLARATION && c->getChildNT(NT_CONTROL_LOCAL_DEC_LIST) != NULL){
            for(AST *table : c->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->children){
                if(table->nType == NT_TABLE_DECLARATION){
                    for(VirtTableData& vtd : CFG::Get().m_vTableDatas){
                        if(vtd.controlBlockName == c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value && vtd.name == table->value){
                            if(table->getChildNT(NT_TABLE_PROPERTY_LIST) != NULL){
                                for(AST *property : table->getChildNT(NT_TABLE_PROPERTY_LIST)->children){
                                    if(property->value == "key" && property->getChildNT(NT_KEY_ELEMENT_LIST) != NULL){
                                        for(AST *keyElement : property->getChildNT(NT_KEY_ELEMENT_LIST)->children){
                                            vtd.matchFieldList.push_back(TableMatchField{keyElement->getChildNT(NT_EXPRESSION)->getValueExpression(), keyElement->value});
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void CFG::AddProgramTable(int program, AST *table, const std::string& vTableName, const std::string& programControlBL, const std::string& virtualControlBL){
    while((int) CFG::Get().m_programTableDatas.size() < program + 1)
        CFG::Get().m_programTableDatas.push_back({});

    auto it = CFG::Get().m_virtControlsToProgControls.find(virtualControlBL);
    if(it == CFG::Get().m_virtControlsToProgControls.end()){
        CFG::Get().m_virtControlsToProgControls.emplace(virtualControlBL, std::vector<std::string>{});
        it = CFG::Get().m_virtControlsToProgControls.find(virtualControlBL);
    }
    while((int) it->second.size() < program + 1)
        it->second.push_back("");
    it->second[program] = programControlBL;

    //Get table size
    int ts = 0;
    const AST* TablePL = table->getChildNT(NT_TABLE_PROPERTY_LIST);
    while(ts < (int) TablePL->children.size() && TablePL->children[ts]->value != "size")
        ts++;
    if(ts == (int) TablePL->children.size()){
        fprintf(stderr, "Error: Table \"%s\" does not declare a size (%s, line %d)\n", AST::RemoveMergePrefix(table->value).c_str(), CFG::Get().m_programASTs[program]->str(), table->lineNumber);
        exit(1);
    }
    const int TableSize = IPLBase::intFromStr(TablePL->children[ts]->children[0]->value.c_str());
    if(TableSize == -1){
        fprintf(stderr, "Error: Table \"%s\" has invalid size \"%s\" (%s, line %d)\n", AST::RemoveMergePrefix(table->value).c_str(), TablePL->children[ts]->children[0]->value.c_str(), CFG::Get().m_programASTs[program]->str(), table->lineNumber);
        exit(1);
    }

    //Check if the table supports idle_timeout_notification
    bool supportTimeout = false;
    ts = 0;
    while(ts < (int) TablePL->children.size() && (TablePL->children[ts]->value != "idle_timeout" && TablePL->children[ts]->value != "support_timeout"))
        ts++;
    if(ts < (int) TablePL->children.size() && TablePL->children[ts]->getChildNT(NT_EXPRESSION) != NULL && TablePL->children[ts]->getChildNT(NT_EXPRESSION)->value == "true")
        supportTimeout = true;

    //Get table @name annotation
    std::string TableNameAnnotation = AST::RemoveMergePrefix(table->value);
    const AST* TableAL = table->getChildNT(NT_ANNOTATION_LIST);
    if(TableAL != NULL){
        int tan = 0;
        while(tan < (int) TableAL->children.size() && TableAL->children[tan]->value != "name")
            tan++;
        if(tan < (int) TableAL->children.size()){
            if(TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST) != NULL && TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children.size() == 1){
                TableNameAnnotation = TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children[0]->value;
                while(TableNameAnnotation[TableNameAnnotation.size() - 1] == '"')
                    TableNameAnnotation.pop_back();
                while(TableNameAnnotation[0] == '"')
                    TableNameAnnotation.erase(TableNameAnnotation.begin());
            }
        }
    }

    //Get table @id annotation
    std::string TableIDAnnotation = "";
    uint32_t annotatedTableID = 0;
    if(TableAL != NULL){
        int tan = 0;
        while(tan < (int) TableAL->children.size() && TableAL->children[tan]->value != "id")
            tan++;
        if(tan < (int) TableAL->children.size()){
            if(TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST) != NULL && TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children.size() == 1){
                TableIDAnnotation = TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children[0]->value;
            }
        }
    }
    if(TableIDAnnotation != ""){
        if(!IPLBase::isInt(TableIDAnnotation.c_str())){
            fprintf(stderr, "Error: Malformed ID annotation for table \"%s\"\n", table->str());
            exit(1);
        }

        annotatedTableID = static_cast<uint32_t>(IPLBase::intFromStr(TableIDAnnotation.c_str()));
        if((annotatedTableID & 0xFF000000) != 0 && (annotatedTableID & 0xFF000000) != TABLE_P4INFO_MSB){
            fprintf(stderr, "Error: Non compliant ID annotation for table \"%s\": %x\n", table->str(), annotatedTableID);
            exit(1);
        }
        annotatedTableID = (annotatedTableID & 0x00FFFFFF) | TABLE_P4INFO_MSB;
    }

    //Check if table is shared
    bool shared = false;
    bool malformedSharedAnnotation = false;
    bool sharedOwner = false;
    bool sharedCanWrite = true;
    std::string sharedName = "NONE";
    int tan = 0;
    if(TableAL != NULL){
        while(tan < (int) TableAL->children.size() && TableAL->children[tan]->value != VIRT_SHARED_ANNOTATION)
            tan++;
        if(tan < (int) TableAL->children.size()){
            shared = true;
            malformedSharedAnnotation = true;
            const AST* ATL = TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST);
            if(ATL != NULL && ATL->children.size() == 5){
                malformedSharedAnnotation = false;

                sharedName = ATL->children[0]->value;
                if(sharedName[sharedName.size() - 1] == '\"')
                    sharedName.pop_back();
                else
                    malformedSharedAnnotation = true;
                if(sharedName[0] == '\"')
                    sharedName.erase(sharedName.begin());
                else
                    malformedSharedAnnotation = true;
                if(ATL->children[2]->value == "true")
                    sharedOwner = true;
                else if(ATL->children[2]->value == "false")
                    sharedOwner = false;
                else 
                    malformedSharedAnnotation = true;
                if(ATL->children[4]->value == "\"R\"")
                    sharedCanWrite = false;
                else if(ATL->children[4]->value == "\"RW\"")
                    sharedCanWrite = true;
                else
                    malformedSharedAnnotation = true;
            }
        }
    }

    if(malformedSharedAnnotation){
        fprintf(stderr, "Error: Malformed %s annotation in program \"%s\" - ", VIRT_SHARED_ANNOTATION, CFG::Get().m_programNames[program].c_str());
        //TableAL->children[tan]->writeCode(stderr);
        fprintf(stderr, "\n");
        exit(1);
    }

    //Get table actions list
    std::vector<std::string> tableActionList;
    int tal = 0;
    while(tal < (int) TablePL->children.size() && TablePL->children[tal]->value != "actions")
        tal++;
    if(tal != (int) TablePL->children.size()){
        if(TablePL->children[tal]->getChildNT(NT_ACTION_LIST) != NULL){
            for(AST* actionRef : TablePL->children[tal]->getChildNT(NT_ACTION_LIST)->children){
                if(actionRef != NULL && actionRef->nType == NT_ACTION_REF){
                    //int pfx = CFGfindPrefix(actionRef->value) + 1;
                    //std::string ProgramActionName = actionRef->value.substr(pfx);
                    std::string ProgramActionName = actionRef->value;
                    tableActionList.push_back(ProgramActionName);

                    bool found = ProgramActionName == "NoAction";
                    for(int y = 0; y < (int) CFG::Get().m_programActionDatas[program].size(); y++){
                        ProgramActionData pad = CFG::Get().m_programActionDatas[program][y];
                        if((pad.controlBlockName == programControlBL || pad.controlBlockName == "") && (pad.programName == ProgramActionName))
                            found = true;
                        if((pad.controlBlockName == programControlBL || pad.controlBlockName == "") && (pad.vActionName == actionRef->value))
                            found = true;
                    }
                    if(!found){
                        fprintf(stderr, "Error: Could not find action declaration \"%s\" for table \"%s.%s\" in program \"%s\"\n", ProgramActionName.c_str(), programControlBL.substr(CFGfindPrefix(programControlBL) + 1).c_str(), AST::RemoveMergePrefix(table->value).c_str(), CFG::Get().m_programNames[program].c_str());
                        exit(1);
                    }
                }
            }
        }
    }

    /*
    //Get table match field list
    std::vector<TableMatchField> tableMatchFieldList;
    int tmfl = 0;
    while(tmfl < (int) TablePL->children.size() && TablePL->children[tmfl]->value != "key")
        tmfl++;
    if(tmfl != (int) TablePL->children.size()){
        if(TablePL->children[tmfl]->getChildNT(NT_KEY_ELEMENT_LIST) != NULL){
            for(AST* keyElement : TablePL->children[tmfl]->getChildNT(NT_KEY_ELEMENT_LIST)->children){
                const std::string KeyElementName = CFGRemoveAllPrefixes(keyElement->getChildNT(NT_EXPRESSION)->getValueExpression());
                tableMatchFieldList.push_back(TableMatchField{KeyElementName, keyElement->value});
            }
        }
    }
    */

    ProgramTableData newPTD = {programControlBL, AST::RemoveMergePrefix(table->value), TableNameAnnotation, virtualControlBL, vTableName, tableActionList, sharedOwner, sharedCanWrite, TableSize, annotatedTableID == 0 ? GetRandomUniqueID() + TABLE_P4INFO_MSB : annotatedTableID, annotatedTableID != 0};
    
    int i = 0;
    while(i < (int) CFG::Get().m_vTableDatas.size() && CFG::Get().m_vTableDatas[i].name != vTableName)
        i++;
    if(i == (int) CFG::Get().m_vTableDatas.size()){
        CFG::Get().m_vTableDatas.push_back({virtualControlBL, vTableName, shared ? VTT_SHARED : VTT_NOT_SHARED, {program}, TableSize, supportTimeout, {}, newPTD.p4infoID});
    }
    else{
        if(CFG::Get().m_vTableDatas[i].vtype == VTT_NOT_SHARED)
            CFG::Get().m_vTableDatas[i].vtype = VTT_HIDDEN_SHARED;
        CFG::Get().m_vTableDatas[i].programs.insert(program);
        if(CFG::Get().m_vTableDatas[i].vtype != VTT_SHARED)
            CFG::Get().m_vTableDatas[i].totalSize = CFG::Get().m_vTableDatas[i].totalSize + TableSize;
        if(supportTimeout)
            CFG::Get().m_vTableDatas[i].supportTimeout = true;
        if(!newPTD.p4infoIDisAnnotated)
            newPTD.p4infoID = CFG::Get().m_vTableDatas[i].p4infoID;
    }

    //Detects and fixes id conflicts
    for(int itr = 0; itr < (int) CFG::Get().m_programTableDatas[program].size(); itr++){
        ProgramTableData *ptd = &CFG::Get().m_programTableDatas[program][itr];
        if(ptd->p4infoID == newPTD.p4infoID){
            if(ptd->p4infoIDisAnnotated && newPTD.p4infoIDisAnnotated){
                fprintf(stderr, "Error: Conflicting ID annotations for tables \"%s.%s\" and \"%s.%s\" in program \"%s\"\n", CFGRemovePrefix(ptd->controlBlockName).c_str(), ptd->programName.c_str(), CFGRemovePrefix(newPTD.controlBlockName).c_str(), newPTD.programName.c_str(), CFG::Get().m_programNames[program].c_str());
                exit(1);
            }
            else if(newPTD.p4infoIDisAnnotated){
                ptd->p4infoID = CFG::GetRandomUniqueID() + TABLE_P4INFO_MSB;
            }
            else{
                newPTD.p4infoID = CFG::GetRandomUniqueID() + TABLE_P4INFO_MSB;
            }
        }
    }

    CFG::Get().m_programTableDatas[program].push_back(newPTD);

    if(shared){
        int j = 0;
        while(j < (int) CFG::Get().m_sharedTableDatas.size() && CFG::Get().m_sharedTableDatas[j].alias != sharedName)
            j++;

        if(j == (int) CFG::Get().m_sharedTableDatas.size()){
            CFG::Get().m_sharedTableDatas.push_back({sharedName, i, table->Clone(), sharedOwner ? program : -1, sharedOwner ? sharedCanWrite : false});
        }
        else{
            SharedTableData *STD = &CFG::Get().m_sharedTableDatas[j];
            if(AST::TablesMergeable(table, STD->tableCopy)){
                if(sharedOwner){
                    if(STD->owner == -1){
                        STD->owner = program;
                        STD->nonOwnerCanWrite = sharedCanWrite;
                    }
                    else{
                        fprintf(stderr, "Error: Multiple programs declared as owner of %s table \"%s\"\n", VIRT_SHARED_ANNOTATION, STD->alias.c_str());
                        exit(1);
                    }
                }
            }
            else{
                fprintf(stderr, "Error: Cannot merge all versions of %s table \"%s\"\n", VIRT_SHARED_ANNOTATION, STD->alias.c_str());
                exit(1);
            }
        }
    }
}

void CFG::ChangeVTableName(const std::string& virtControlName, const std::string& oldName, const std::string& newName){
    auto it = CFG::Get().m_virtControlsToProgControls.find(virtControlName);
    if(it == CFG::Get().m_virtControlsToProgControls.end())
        return;
    
    for(int i = 0; i < (int) CFG::Get().m_vTableDatas.size(); i++){
        if(CFG::Get().m_vTableDatas[i].controlBlockName == virtControlName && CFG::Get().m_vTableDatas[i].name == oldName)
            CFG::Get().m_vTableDatas[i].name = newName;
    }
    for(int p = 0; p < (int) CFG::Get().m_programTableDatas.size(); p++){
        for(int pt = 0; pt < (int) CFG::Get().m_programTableDatas[p].size(); pt++)
            if(CFG::Get().m_programTableDatas[p][pt].controlBlockName == it->second[p] && CFG::Get().m_programTableDatas[p][pt].vTableName == oldName)
                CFG::Get().m_programTableDatas[p][pt].vTableName = newName;
    }
}

void CFG::AddProgramAction(int program, AST *action, const std::string& originalProgramName, const std::string& vActionName, const std::string& programControlBL, const std::string& virtualControlBL){
    while((int) CFG::Get().m_programActionDatas.size() < program + 1)
        CFG::Get().m_programActionDatas.push_back({});

    auto it = CFG::Get().m_virtControlsToProgControls.find(virtualControlBL);
    if(it == CFG::Get().m_virtControlsToProgControls.end()){
        CFG::Get().m_virtControlsToProgControls.emplace(virtualControlBL, std::vector<std::string>{});
        it = CFG::Get().m_virtControlsToProgControls.find(virtualControlBL);
    }
    while((int) it->second.size() < program + 1)
        it->second.push_back("");
    it->second[program] = programControlBL;

    //Get action @name annotation
    std::string ActionNameAnnotation = AST::RemoveMergePrefix(action->value);
    const AST* ActionAL = action->getChildNT(NT_ANNOTATION_LIST);
    if(ActionAL != NULL){
        int tan = 0;
        while(tan < (int) ActionAL->children.size() && ActionAL->children[tan]->value != "name")
            tan++;
        if(tan < (int) ActionAL->children.size()){
            if(ActionAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST) != NULL && ActionAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children.size() == 1){
                ActionNameAnnotation = ActionAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children[0]->value;
                while(ActionNameAnnotation[ActionNameAnnotation.size() - 1] == '"')
                    ActionNameAnnotation.pop_back();
                while(ActionNameAnnotation[0] == '"')
                    ActionNameAnnotation.erase(ActionNameAnnotation.begin());
            }
        }
    }

    //Get action @id annotation
    std::string ActionIDAnnotation = "";
    uint32_t annotatedActionID = 0;
    if(ActionAL != NULL){
        int tan = 0;
        while(tan < (int) ActionAL->children.size() && ActionAL->children[tan]->value != "id")
            tan++;
        if(tan < (int) ActionAL->children.size()){
            if(ActionAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST) != NULL && ActionAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children.size() == 1){
                ActionIDAnnotation = ActionAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children[0]->value;
            }
        }
    }
    if(ActionIDAnnotation != ""){
        if(!IPLBase::isInt(ActionIDAnnotation.c_str())){
            fprintf(stderr, "Error: Malformed ID annotation for action \"%s\"\n", action->str());
            exit(1);
        }

        annotatedActionID = static_cast<uint32_t>(IPLBase::intFromStr(ActionIDAnnotation.c_str()));
        if((annotatedActionID & 0xFF000000) != 0 && (annotatedActionID & 0xFF000000) != ACTION_P4INFO_MSB){
            fprintf(stderr, "Error: Non compliant ID annotation for action \"%s\": %x\n", action->str(), annotatedActionID);
            exit(1);
        }
        annotatedActionID = (annotatedActionID & 0x00FFFFFF) | ACTION_P4INFO_MSB;
    }

    std::vector<ProgramActionParamData> paramsData;
    //Get Parameters Data
    if(action->getChildNT(NT_PARAMETER_LIST) != NULL){
        for(AST* parameter : action->getChildNT(NT_PARAMETER_LIST)->children){
            /*
            if(parameter->getChildNT(NT_BASE_TYPE) == NULL){
                Assert(false);
            }
            else if(parameter->getChildNT(NT_BASE_TYPE)->value != "bit"){
                Assert(false);
            }
            else if(parameter->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL) == NULL){
                Assert(false);
            }
            else if(!IPLBase::isInt(parameter->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL)->value.c_str())){
                Assert(false);
            }

            ProgramActionParamData data {parameter->value, static_cast<uint32_t>(IPLBase::intFromStr(parameter->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL)->value.c_str()))};
            paramsData.push_back(data);
            */

            if(parameter->getChildNT(NT_BASE_TYPE) != NULL){
                uint32_t bw = 0;
                
                if(parameter->getChildNT(NT_BASE_TYPE)->value == "bool")
                    bw = 1;
                else if(parameter->getChildNT(NT_BASE_TYPE)->value == "bit"){
                    if(parameter->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL) == NULL)
                        Assert(false /*TODO: Display error msg*/);
                    if(!IPLBase::isInt(parameter->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL)->value.c_str()))
                        Assert(false /*TODO: Display error msg*/);
                    bw = static_cast<uint32_t>(IPLBase::intFromStr(parameter->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL)->value.c_str()));
                }

                ProgramActionParamData data {parameter->value, bw};
                paramsData.push_back(data);
            }
        }
    }

    ProgramActionData newPAD = {programControlBL, AST::RemoveMergePrefix(action->value), originalProgramName, ActionNameAnnotation, paramsData, virtualControlBL, vActionName, annotatedActionID == 0 ? GetRandomUniqueID() + ACTION_P4INFO_MSB : annotatedActionID, annotatedActionID != 0};
    CFG::Get().m_programActionDatas[program].push_back(newPAD);
}

void CFG::ChangeVActionName(const std::string& virtControlName, const std::string& oldName, const std::string& newName){
    auto it = CFG::Get().m_virtControlsToProgControls.find(virtControlName);
    if(it == CFG::Get().m_virtControlsToProgControls.end()){
        if(virtControlName == ""){
            for(int p = 0; p < (int) CFG::Get().m_programActionDatas.size(); p++){
                for(int pt = 0; pt < (int) CFG::Get().m_programActionDatas[p].size(); pt++)
                    if(CFG::Get().m_programActionDatas[p][pt].controlBlockName == "" && CFG::Get().m_programActionDatas[p][pt].vActionName == oldName)
                        CFG::Get().m_programActionDatas[p][pt].vActionName = newName;
            }
        }
        return;
    }
    
    for(int p = 0; p < (int) CFG::Get().m_programActionDatas.size() && p < (int) it->second.size(); p++){
        for(int pt = 0; pt < (int) CFG::Get().m_programActionDatas[p].size(); pt++)
            if(CFG::Get().m_programActionDatas[p][pt].controlBlockName == it->second[p] && CFG::Get().m_programActionDatas[p][pt].vActionName == oldName)
                CFG::Get().m_programActionDatas[p][pt].vActionName = newName;
    }
}

void CFG::GenerateP4Infos(AST *merged){
    if(merged != NULL){
        CFG::AddVirtTableMatchFields(merged);
        CFG::Get().GenerateMergedP4Info(merged);
    }
    for(int i = 0; i < (int) CFG::Get().m_programASTs.size(); i++)
        CFG::Get().GenerateProgramP4Info(i);
}

void CFG::GenerateMergedP4Info(AST *merged) const{
    typedef struct vactiondata{
        std::string fullname;
        std::vector<ProgramActionParamData> paramsData;
        uint64_t id;
    } VirtActionData;
    std::map<std::string, VirtActionData> vActionDataMap;

    const char TabSpaces[] = "  ";

    std::set<uint32_t> IDsUsed;
    SymbolTable symbolTable;
    for(AST *c : merged->children)
        symbolTable.addEntry(c);

    std::string archName = "";
    switch(m_architecture){
        case P4A_V1MODEL:
            archName = "v1model";
            break;
        case P4A_TNA:
            archName = "tna";
            break;
        default:
            break;
    }
    Assert(archName != "");
    Assert(m_metadataProgIDFullName != "");
    
    FILE *fp = fopen((m_outputDir + "/" + P4INFO_FILE).c_str(), "w");
    if(!fp){
        fprintf(stderr, "Could not open file to write: \"%s\"\n", (m_outputDir + "/" + P4INFO_FILE).c_str());
        exit(1);
    }

    fprintf(fp, "pkg_info {\n%sarch: \"%s\"\n}\n", TabSpaces, archName.c_str());

    //Actions
    for(int p = 0; p < (int) m_programActionDatas.size(); p++){
        for(const ProgramActionData& pad : m_programActionDatas[p]){
            std::string vActionFullName = pad.vActionControlBL + "." + pad.vActionName;
            if(vActionFullName[0] == '.')
                vActionFullName.erase(vActionFullName.begin());
            if(vActionDataMap.find(vActionFullName) == vActionDataMap.end()){
                uint32_t vActionID = pad.p4infoID;
                if(IDsUsed.find(vActionID) != IDsUsed.end())
                    vActionID = GetRandomUniqueID() + ACTION_P4INFO_MSB;
                IDsUsed.insert(vActionID);
                vActionDataMap.insert(std::make_pair(vActionFullName, VirtActionData{vActionFullName, pad.paramsData, vActionID}));
            }
        }
    }

    //Tables
    for(const VirtTableData& vtd : m_vTableDatas){
        std::set<std::string> ActionsCopied;
        IDsUsed.insert(vtd.p4infoID);

        fprintf(fp, "tables {\n%spreamble {\n", TabSpaces);
        fprintf(fp, "%s%sid: %d\n", TabSpaces, TabSpaces, vtd.p4infoID);
        fprintf(fp, "%s%sname: \"%s\"\n", TabSpaces, TabSpaces, (vtd.controlBlockName + "." + AST::GetVirtInstanceP4InfoName(vtd.name)).c_str());
        fprintf(fp, "%s%salias: \"%s\"\n", TabSpaces, TabSpaces, (vtd.controlBlockName + "." + AST::GetVirtInstanceP4InfoName(vtd.name)).c_str());
        fprintf(fp, "%s}\n", TabSpaces);

        for(int i = 0; i < (int) vtd.matchFieldList.size(); i++){
            fprintf(fp, "%smatch_fields {\n", TabSpaces);
            fprintf(fp, "%s%sid: %d\n", TabSpaces, TabSpaces, i + 1);
            fprintf(fp, "%s%sname: \"%s\"\n", TabSpaces, TabSpaces, vtd.matchFieldList[i].fieldName.c_str());
            fprintf(fp, "%s%sbitwidth: %d\n%s%smatch_type: ", TabSpaces, TabSpaces, symbolTable.getBitwidthFromExpression(vtd.matchFieldList[i].fieldName, vtd.controlBlockName), TabSpaces, TabSpaces);
            for(int j = 0; j < (int) vtd.matchFieldList[i].fieldType.size(); j++)
                fprintf(fp, "%c", toupper(vtd.matchFieldList[i].fieldType[j]));
            fprintf(fp, "\n%s}\n", TabSpaces);
        }

        for(int i = 0; i < (int) m_programTableDatas.size(); i++){
            for(int j = 0; j < (int) m_programTableDatas[i].size(); j++){
                ProgramTableData ptd = m_programTableDatas[i][j];
                if((ptd.vTableControlBL + "." + ptd.vTableName) == (vtd.controlBlockName + "." + vtd.name)){
                    for(std::string actionName : ptd.actionList){
                        std::string virtActionName = "";
                        for(auto _pad: m_programActionDatas[i]){
                            if(AST::RemoveMergePrefix(_pad.programName) ==  AST::RemoveMergePrefix(actionName))
                                virtActionName = _pad.vActionName;
                        }
                        Assert(virtActionName != "");
                        std::string actionFullName;
                        for(auto vad : vActionDataMap){
                            if(vad.second.fullname == (ptd.vTableControlBL + "." + virtActionName)){
                                actionFullName = vad.second.fullname;
                            }
                            else if(actionFullName == "" && vad.second.fullname == virtActionName){
                                actionFullName = vad.second.fullname;
                            }
                        }
                        Assert(actionFullName != "");
                        if(ActionsCopied.find(actionFullName) == ActionsCopied.end()){
                            ActionsCopied.emplace(actionFullName);
                            fprintf(fp, "%saction_refs {\n%s%sid: %lu\n%s}\n", TabSpaces, TabSpaces, TabSpaces, vActionDataMap.find(actionFullName)->second.id, TabSpaces);
                        }
                    }
                }
            }
        }

        fprintf(fp, "%ssize: %d\n", TabSpaces, vtd.totalSize);
        if(vtd.supportTimeout)
            fprintf(fp, "%sidle_timeout_behavior: NOTIFY_CONTROL\n", TabSpaces);
        fprintf(fp, "}\n");
    }

    //Actions
    for(auto it : vActionDataMap){
        VirtActionData vad = it.second;
        fprintf(fp, "actions {\n%spreamble {\n", TabSpaces);
        fprintf(fp, "%s%sid: %lu\n", TabSpaces, TabSpaces, vad.id);
        fprintf(fp, "%s%sname: \"%s\"\n", TabSpaces, TabSpaces, AST::GetVirtInstanceP4InfoName(vad.fullname).c_str());
        fprintf(fp, "%s%salias: \"%s\"\n", TabSpaces, TabSpaces, AST::GetVirtInstanceP4InfoName(vad.fullname).c_str());
        fprintf(fp, "%s}\n", TabSpaces);
        int pid = 1;
        for(ProgramActionParamData papd : vad.paramsData){
            fprintf(fp, "%sparams {\n", TabSpaces);
            fprintf(fp, "%s%sid: %d\n", TabSpaces, TabSpaces, pid);
            fprintf(fp, "%s%sname: \"%s\"\n", TabSpaces, TabSpaces, papd.name.c_str());
            fprintf(fp, "%s%sbitwidth: %d\n", TabSpaces, TabSpaces, papd.bitwidth);
            fprintf(fp, "%s}\n", TabSpaces);
            pid++;
        }
        fprintf(fp, "}\n");
    }

    fprintf(fp, "type_info {\n}\n");

    fclose(fp);
}

void CFG::GenerateProgramP4Info(const int progN) const{
    const char TabSpaces[] = "  ";
    char buffer[BUFFER_SIZE];
    
    Assert(progN < (int) m_programASTs.size() && m_programASTs.size() == m_programNames.size());

    AST *ast = m_programASTs[progN];
    
    std::set<uint32_t> IDsUsed;
    std::set<uint32_t> DeclaredIDs = GetDeclaredIDs(ast);

    SymbolTable symbolTable;
    for(AST * c : ast->children)
        symbolTable.addEntry(c);

    std::string archName = "";
    switch(m_architecture){
        case P4A_V1MODEL:
            archName = "v1model";
            break;
        case P4A_TNA:
            archName = "tna";
            break;
        case P4A_NVDA_DOCA:
            archName = "nvda_doca";
            break;
        default:
            break;
    }
    Assert(archName != "");
    
    int pos = m_programNames[progN].size() - 1;
    while(pos >= 0 && m_programNames[progN][pos] != '/')
        pos--;
    pos++;
    sprintf(buffer, "%s/%s.p4info.txt", m_outputDir.c_str(), m_programNames[progN].substr(pos).c_str());

    FILE *fp = fopen(buffer, "w");
    if(!fp){
        fprintf(stderr, "Could not open file to write: \"%s\"\n", buffer);
        exit(1);
    }

    fprintf(fp, "pkg_info {\n%sarch: \"%s\"\n}\n", TabSpaces, archName.c_str());

    std::vector<ProgramActionData> PADs;
    for(AST* c : ast->children){
        if(c->nType == NT_CONTROL_DECLARATION){
            if(c->getChildNT(NT_CONTROL_LOCAL_DEC_LIST) != NULL){
                for(AST *localD : c->getChildNT(NT_CONTROL_LOCAL_DEC_LIST)->children){
                    if(localD->nType == NT_TABLE_DECLARATION){
                        //Get table @name annotation
                        std::string TableNameAnnotation = localD->value;
                        const AST* TableAL = localD->getChildNT(NT_ANNOTATION_LIST);
                        if(TableAL != NULL){
                            int tan = 0;
                            while(tan < (int) TableAL->children.size() && TableAL->children[tan]->value != "name")
                                tan++;
                            if(tan < (int) TableAL->children.size()){
                                if(TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST) != NULL && TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children.size() == 1){
                                    TableNameAnnotation = TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children[0]->value;
                                    while(TableNameAnnotation[TableNameAnnotation.size() - 1] == '"')
                                        TableNameAnnotation.pop_back();
                                    while(TableNameAnnotation[0] == '"')
                                        TableNameAnnotation.erase(TableNameAnnotation.begin());
                                }
                            }
                        }
                        
                        //Get table @id annotation
                        std::string TableIDAnnotation = "";
                        uint32_t TableID = 0;
                        if(TableAL != NULL){
                            int tan = 0;
                            while(tan < (int) TableAL->children.size() && TableAL->children[tan]->value != "id")
                                tan++;
                            if(tan < (int) TableAL->children.size()){
                                if(TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST) != NULL && TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children.size() == 1){
                                    TableIDAnnotation = TableAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children[0]->value;
                                }
                            }
                        }
                        if(TableIDAnnotation != ""){
                            if(!IPLBase::isInt(TableIDAnnotation.c_str())){
                                fprintf(stderr, "Error: Malformed ID annotation for table \"%s\"\n", localD->str());
                                exit(1);
                            }

                            TableID = static_cast<uint32_t>(IPLBase::intFromStr(TableIDAnnotation.c_str()));
                            if((TableID & 0xFF000000) != 0 && (TableID & 0xFF000000) != TABLE_P4INFO_MSB){
                                fprintf(stderr, "Error: Non compliant ID annotation for table \"%s\": %x\n", localD->str(), TableID);
                                exit(1);
                            }
                            TableID = (TableID & 0x00FFFFFF) | TABLE_P4INFO_MSB;
                        }
                        else{
                            if((int) m_programTableDatas.size() > progN){
                                for(const ProgramTableData& ptd : m_programTableDatas[progN]){
                                    if(ptd.controlBlockName == c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value && ptd.programName == localD->value)
                                        TableID = ptd.p4infoID;
                                }
                            }

                            if(TableID == 0){
                                TableID = GetRandomUniqueID() + TABLE_P4INFO_MSB;
                                while(IDsUsed.find(TableID) != IDsUsed.end() || DeclaredIDs.find(TableID) != DeclaredIDs.end())
                                    TableID = GetRandomUniqueID() + TABLE_P4INFO_MSB;
                            }
                        }

                        IDsUsed.insert(TableID);
                        
                        fprintf(fp, "tables {\n%spreamble {\n", TabSpaces);
                        fprintf(fp, "%s%sid: %d\n", TabSpaces, TabSpaces, TableID);
                        fprintf(fp, "%s%sname: \"%s\"\n", TabSpaces, TabSpaces, (c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value + "." + AST::GetVirtInstanceP4InfoName(localD->value)).c_str());
                        fprintf(fp, "%s%salias: \"%s\"\n", TabSpaces, TabSpaces, AST::GetVirtInstanceP4InfoName(TableNameAnnotation).c_str());
                        fprintf(fp, "%s}\n", TabSpaces);

                        std::vector<TableMatchField> matchFieldList;
                        bool supportTimeout = false;
                        if(localD->getChildNT(NT_TABLE_PROPERTY_LIST) != NULL){
                            for(AST *property : localD->getChildNT(NT_TABLE_PROPERTY_LIST)->children){
                                if(property->value == "key" && property->getChildNT(NT_KEY_ELEMENT_LIST) != NULL){
                                    for(AST *keyElement : property->getChildNT(NT_KEY_ELEMENT_LIST)->children){
                                        matchFieldList.push_back(TableMatchField{keyElement->getChildNT(NT_EXPRESSION)->getValueExpression(), keyElement->value});
                                    }
                                }
                                else if((property->value == "support_timeout" || property->value == "idle_timeout") && (property->getChildNT(NT_EXPRESSION) != NULL && property->getChildNT(NT_EXPRESSION)->value == "true")){
                                    supportTimeout = true;
                                }
                            }
                        }
                        
                        for(int i = 0; i < (int) matchFieldList.size(); i++){
                            fprintf(fp, "%smatch_fields {\n", TabSpaces);
                            fprintf(fp, "%s%sid: %d\n", TabSpaces, TabSpaces, i + 1);
                            fprintf(fp, "%s%sname: \"%s\"\n", TabSpaces, TabSpaces, matchFieldList[i].fieldName.c_str());
                            fprintf(fp, "%s%sbitwidth: %d\n%s%smatch_type: ", TabSpaces, TabSpaces, symbolTable.getBitwidthFromExpression(matchFieldList[i].fieldName, c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value), TabSpaces, TabSpaces);
                            for(int j = 0; j < (int) matchFieldList[i].fieldType.size(); j++)
                                fprintf(fp, "%c", toupper(matchFieldList[i].fieldType[j]));
                            fprintf(fp, "\n%s}\n", TabSpaces);
                        }

                        if(localD->getChildNT(NT_TABLE_PROPERTY_LIST) != NULL){
                            for(AST *actionList : localD->getChildNT(NT_TABLE_PROPERTY_LIST)->children){
                                if(actionList->value == "actions" && actionList->getChildNT(NT_ACTION_LIST) != NULL){
                                    for(AST *actionRef : actionList->getChildNT(NT_ACTION_LIST)->children){
                                        if(actionRef != NULL && actionRef->nType == NT_ACTION_REF){
                                            uint32_t ref = 0;
                                            for(const ProgramActionData& pad : PADs){
                                                if((pad.controlBlockName == c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value && pad.programName == actionRef->value) || (ref == 0 && pad.controlBlockName == "" && pad.programName == actionRef->value))
                                                    ref = pad.p4infoID;
                                            }
                                            Assert(ref != 0);
                                            fprintf(fp, "%saction_refs {\n%s%sid: %d\n%s}\n", TabSpaces, TabSpaces, TabSpaces, ref, TabSpaces);
                                        }
                                    }
                                }
                            }

                            int size = -1;
                            for(AST *sizeP : localD->getChildNT(NT_TABLE_PROPERTY_LIST)->children){
                                if(sizeP->value == "size" && sizeP->getChildNT(NT_EXPRESSION) != NULL){
                                    if(size != -1){
                                        fprintf(stderr, "Error: Multiple size properties for table \"%s.%s\" in program \"%s\"\n", c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->str(), localD->str(), ast->str());
                                        fclose(fp);
                                        exit(1);
                                    }
                                    size = IPLBase::intFromStr(sizeP->getChildNT(NT_EXPRESSION)->str());
                                    if(size < 1){
                                        fprintf(stderr, "Error: Invalid size for table \"%s.%s\" in program \"%s\" (size = %s)\n", c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->str(), localD->str(), ast->str(), sizeP->getChildNT(NT_EXPRESSION)->str());
                                        fclose(fp);
                                        exit(1);
                                    }
                                }
                            }
                            if(size == -1){
                                fprintf(stderr, "Error: Undeclared size for table \"%s.%s\" in program \"%s\"\n", c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->str(), localD->str(), ast->str());
                                fclose(fp);
                                exit(1);
                            }
                            fprintf(fp, "%ssize: %d\n", TabSpaces, size);
                            if(supportTimeout)
                                fprintf(fp, "%sidle_timeout_behavior: NOTIFY_CONTROL\n", TabSpaces);
                            fprintf(fp, "}\n");
                        }
                    }
                    else if(localD->nType == NT_ACTION_DECLARATION){
                        ProgramActionData newPAD = GetPad(localD, c->getChildNT(NT_CONTROL_TYPE_DECLARATION)->value, progN);
                        PADs.push_back(newPAD);
                        IDsUsed.insert(newPAD.p4infoID);
                    }
                }
            }
        }
        else if(c->nType == NT_ACTION_DECLARATION){
            ProgramActionData newPAD = GetPad(c, "", progN);
            PADs.push_back(newPAD);
            IDsUsed.insert(newPAD.p4infoID);
        }
    }

    for(const ProgramActionData& pad : PADs){
        fprintf(fp, "actions {\n%spreamble {\n", TabSpaces);
        fprintf(fp, "%s%sid: %u\n", TabSpaces, TabSpaces, pad.p4infoID);
        fprintf(fp, "%s%sname: \"%s\"\n", TabSpaces, TabSpaces, (pad.controlBlockName == "" ? pad.programName : pad.controlBlockName + "." + AST::GetVirtInstanceP4InfoName(pad.programName)).c_str());
        fprintf(fp, "%s%salias: \"%s\"\n", TabSpaces, TabSpaces, AST::GetVirtInstanceP4InfoName(pad.annotatedName).c_str());
        fprintf(fp, "%s}\n", TabSpaces);
        int pid = 1;
        for(ProgramActionParamData papd : pad.paramsData){
            fprintf(fp, "%sparams {\n", TabSpaces);
            fprintf(fp, "%s%sid: %d\n", TabSpaces, TabSpaces, pid);
            fprintf(fp, "%s%sname: \"%s\"\n", TabSpaces, TabSpaces, AST::GetVirtInstanceP4InfoName(papd.name).c_str());
            fprintf(fp, "%s%sbitwidth: %d\n", TabSpaces, TabSpaces, papd.bitwidth);
            fprintf(fp, "%s}\n", TabSpaces);
            pid++;
        }
        fprintf(fp, "}\n");
    }

    //Get @controller_header("xxxxx")
    std::set<std::string> ControllerHeaderNamesUsed;
    for(AST *c : ast->children){
        if(c != NULL && c->nType == NT_HEADER_TYPE_DECLARATION && c->getChildNT(NT_ANNOTATION_LIST) != NULL){
            uint32_t id = 0;
            bool isControllerHeader = false;
            std::string token;
            for(AST *annotation : c->getChildNT(NT_ANNOTATION_LIST)->children){
                if(annotation->value == "controller_header" && annotation->getChildNT(NT_ANNOTATION_TOKEN_LIST) != NULL && annotation->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children.size() == 1){
                    token = annotation->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children[0]->value;
                    isControllerHeader = true;
                    if(ControllerHeaderNamesUsed.find(token) != ControllerHeaderNamesUsed.end()){
                        fprintf(stderr, "Error: Multiple controller headers with the same name (%s) on program \"%s\"\n", token.c_str(), ast->str());
                        fclose(fp);
                        exit(1);
                    }
                }
                else if(annotation->value == "id" && annotation->getChildNT(NT_ANNOTATION_TOKEN_LIST) != NULL && annotation->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children.size() == 1){
                    if(!IPLBase::isInt(annotation->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children[0]->value.c_str())){
                        fprintf(stderr, "Error: Invalid ID annotation \"%s\"\n", annotation->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children[0]->value.c_str());
                        fclose(fp);
                        exit(1);
                    }
                    id = IPLBase::intFromStr(annotation->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children[0]->value.c_str());
                }
            }
            if(isControllerHeader){
                if(id == 0){
                    id = GetRandomUniqueID() | CONTROLLER_HEADER_P4INFO_MSB;
                }
                else if(((id & 0xFF000000) != 0) && ((id & 0xFF000000) != CONTROLLER_HEADER_P4INFO_MSB)){
                    fprintf(stderr, "Error: Invalid ID for controller header \"%d\"\n", id);
                    fclose(fp);
                    exit(1);
                }
                else{
                    id = id | CONTROLLER_HEADER_P4INFO_MSB;
                }

                fprintf(fp, "controller_packet_metadata {\n");
                fprintf(fp, "%spreamble {\n%s%sid: %d\n", TabSpaces, TabSpaces, TabSpaces, id);
                fprintf(fp, "%s%sname: %s\n%s%salias: %s\n", TabSpaces, TabSpaces, token.c_str(), TabSpaces, TabSpaces, token.c_str());
                fprintf(fp, "%s}\n", TabSpaces);

                if(c->getChildNT(NT_STRUCT_FIELD_LIST) != NULL){
                    int metadataID = 1;
                    for(AST *sf : c->getChildNT(NT_STRUCT_FIELD_LIST)->children){
                        if(sf->getChildNT(NT_BASE_TYPE) == NULL || sf->getChildNT(NT_BASE_TYPE)->value != "bit" || sf->getChildNT(NT_BASE_TYPE)->children.size() != 1 || !IPLBase::isInt(sf->getChildNT(NT_BASE_TYPE)->children[0]->str())){
                            fprintf(stderr, "Error: Cannot determine bitwidth for field \"%s\" in header \"%s\"\n", sf->str(), c->str());
                            fclose(fp);
                            exit(1);
                        }
                        const int bitwidth = IPLBase::intFromStr(sf->getChildNT(NT_BASE_TYPE)->children[0]->str());
                        fprintf(fp, "%smetadata {\n%s%sid: %d\n", TabSpaces, TabSpaces, TabSpaces, metadataID);
                        fprintf(fp, "%s%sname: \"%s\"\n", TabSpaces, TabSpaces, sf->str());
                        fprintf(fp, "%s%sbitwidth: %d\n", TabSpaces, TabSpaces, bitwidth);
                        fprintf(fp, "%s}\n", TabSpaces);
                        metadataID++;
                    }
                }

                fprintf(fp, "}\n");
            }
        }
    }

    fclose(fp);
}

ProgramActionData CFG::GetPad(AST *ActionD, const std::string& controlBlockName, const int progN){
    //Get action @name annotation
    std::string ActionNameAnnotation = ActionD->value;
    const AST* ActionAL = ActionD->getChildNT(NT_ANNOTATION_LIST);
    if(ActionAL != NULL){
        int tan = 0;
        while(tan < (int) ActionAL->children.size() && ActionAL->children[tan]->value != "name")
            tan++;
        if(tan < (int) ActionAL->children.size()){
            if(ActionAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST) != NULL && ActionAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children.size() == 1){
                ActionNameAnnotation = ActionAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children[0]->value;
                while(ActionNameAnnotation[ActionNameAnnotation.size() - 1] == '"')
                    ActionNameAnnotation.pop_back();
                while(ActionNameAnnotation[0] == '"')
                    ActionNameAnnotation.erase(ActionNameAnnotation.begin());
            }
        }
    }

    //Get action @id annotation
    std::string ActionIDAnnotation = "";
    uint32_t annotatedActionID = 0;
    if(ActionAL != NULL){
        int tan = 0;
        while(tan < (int) ActionAL->children.size() && ActionAL->children[tan]->value != "id")
            tan++;
        if(tan < (int) ActionAL->children.size()){
            if(ActionAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST) != NULL && ActionAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children.size() == 1){
                ActionIDAnnotation = ActionAL->children[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->children[0]->value;
            }
        }
    }
    if(ActionIDAnnotation != ""){
        if(!IPLBase::isInt(ActionIDAnnotation.c_str())){
            fprintf(stderr, "Error: Malformed ID annotation for action \"%s\"\n", ActionD->str());
            exit(1);
        }

        annotatedActionID = static_cast<uint32_t>(IPLBase::intFromStr(ActionIDAnnotation.c_str()));
        if((annotatedActionID & 0xFF000000) != 0 && (annotatedActionID & 0xFF000000) != ACTION_P4INFO_MSB){
            fprintf(stderr, "Error: Non compliant ID annotation for action \"%s\": %x\n", ActionD->str(), annotatedActionID);
            exit(1);
        }
        annotatedActionID = (annotatedActionID & 0x00FFFFFF) | ACTION_P4INFO_MSB;
    }
    else{
        if((int) CFG::Get().m_programActionDatas.size() > progN){
            for(const ProgramActionData& pad : CFG::Get().m_programActionDatas[progN]){
                if(pad.controlBlockName == controlBlockName && pad.programName == ActionD->value){
                    annotatedActionID = pad.p4infoID;
                }
            }
        }
        if(annotatedActionID == 0)
            annotatedActionID = GetRandomUniqueID() + ACTION_P4INFO_MSB;
    }

    std::vector<ProgramActionParamData> paramsData;
    //Get Parameters Data
    if(ActionD->getChildNT(NT_PARAMETER_LIST) != NULL){
        for(AST* parameter : ActionD->getChildNT(NT_PARAMETER_LIST)->children){
            if(parameter->getChildNT(NT_BASE_TYPE) != NULL){
                uint32_t bw = 0;
                
                if(parameter->getChildNT(NT_BASE_TYPE)->value == "bool")
                    bw = 1;
                else if(parameter->getChildNT(NT_BASE_TYPE)->value == "bit"){
                    if(parameter->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL) == NULL)
                        Assert(false /*TODO: Display error msg*/);
                    if(!IPLBase::isInt(parameter->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL)->value.c_str()))
                        Assert(false /*TODO: Display error msg*/);
                    bw = static_cast<uint32_t>(IPLBase::intFromStr(parameter->getChildNT(NT_BASE_TYPE)->getChildNT(NT_LITERAL)->value.c_str()));
                }

                ProgramActionParamData data {parameter->value, bw};
                paramsData.push_back(data);
            }
        }
    }

    ProgramActionData newPAD = {controlBlockName, ActionD->value, ActionD->value, ActionNameAnnotation, paramsData, "", "", annotatedActionID, annotatedActionID != 0};
    return newPAD;
}

void CFGGetDeclaredIDsRec(AST *ast, std::set<uint32_t>& ids){
    std::string IDAnnotation = "";
    
    AST* AL = ast->getChildNT(NT_ANNOTATION_LIST);
    if(AL != NULL){
        int tan = 0;
        while(tan < (int) AL->getChildren().size() && AL->getChildren()[tan]->getValue() != "id")
            tan++;
        if(tan < (int) AL->getChildren().size()){
            if(AL->getChildren()[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST) != NULL && AL->getChildren()[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->getChildren().size() == 1){
                IDAnnotation = AL->getChildren()[tan]->getChildNT(NT_ANNOTATION_TOKEN_LIST)->getChildren()[0]->getValue();
            }
        }
    }
    if(IDAnnotation != ""){
        if(!IPLBase::isInt(IDAnnotation.c_str())){
            throw IDAnnotation;
        }

        uint32_t ID = static_cast<uint32_t>(IPLBase::intFromStr(IDAnnotation.c_str()));
        switch(ast->getNodeType()){
            case NT_ACTION_DECLARATION:
                ID = ID | ACTION_P4INFO_MSB;
                break;
            case NT_TABLE_DECLARATION:
                ID = ID | TABLE_P4INFO_MSB;
                break;
            default:
                break;
        }
        
        if(ids.find(ID) != ids.end()){
            throw ID;
        }
        ids.insert(ID);
    }

    for(AST *c : ast->getChildren())
        if(c != NULL)
            CFGGetDeclaredIDsRec(c, ids);
}

std::set<uint32_t> CFG::GetDeclaredIDs(AST *ast){
    std::set<uint32_t> ids;
    try{
        CFGGetDeclaredIDsRec(ast, ids);
    }
    catch (uint32_t repeatedID){
        fprintf(stderr, "Error: Multiple elements with same id \"%u\" in program \"%s\"\n", repeatedID, ast->str());
        exit(1);
    }
    catch (std::string malformedAnnotation){
        fprintf(stderr, "Error: Malformed ID annotation \"%s\"\n", malformedAnnotation.c_str());
        exit(1);
    }
    return ids;
}

uint32_t CFG::GetRandomUniqueID(){
    const int pseudoRandomSize = 10000;
    const int SHUFFLE_COUNT = pseudoRandomSize * 4;
    static int base = -pseudoRandomSize + 1;
    static int ptr = pseudoRandomSize;
    static uint32_t arr[pseudoRandomSize];

    if(ptr == pseudoRandomSize){
        ptr = 0;
        base = base + pseudoRandomSize;
        for(int i = 0; i < pseudoRandomSize; i++)
            arr[i] = base + i;
        for(int i = 0; i < SHUFFLE_COUNT; i++){
            int a = rand() % pseudoRandomSize;
            int b = rand() % pseudoRandomSize;
            uint32_t aux = arr[a];
            arr[a] = arr[b];
            arr[b] = aux;
        }
    }

    ptr++;
    if(arr[ptr-1] >= 0x01000000){
        fprintf(stderr, "Error: Unique p4info ids exausted! Resulting program is too big!\n");
        exit(1);
    }
    return arr[ptr-1];
}