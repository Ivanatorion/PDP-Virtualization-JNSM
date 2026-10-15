#include "../include/pcheaders.h"
#include "../proto/p4runtime.grpc.pb.h"
#include "../include/IPLBase.hpp"
#include "../include/PropertyList.h"
#include "../include/P4InfoDesc.h"

P4InfoDesc::P4InfoDesc(){

}

P4InfoDesc::P4InfoDesc(const std::string& filePath){
    this->LoadFromFile(filePath);
}

PropertyList* P4InfoDesc::getTablePropertyList(const std::string& tableName){
    std::vector<Property>* tablesP = this->m_propertyList.getProperty("tables");
    Assert(tablesP != NULL);

    int i = 0;
    while(i < (int) tablesP->size()){
        const std::vector<Property>* preambleP = tablesP->at(i).propList->getProperty("preamble");
        Assert(preambleP != NULL && preambleP->size() == 1);

        const std::vector<Property>* nameP = preambleP->at(0).propList->getProperty("name");
        Assert(nameP != NULL && nameP->size() == 1);

        if(("\"" + tableName + "\"") == nameP->at(0).propString.c_str())
            return tablesP->at(i).propList;
        i++;
    }
    return NULL;
}

PropertyList* P4InfoDesc::getTablePropertyList(const int id){
    std::vector<Property>* tablesP = this->m_propertyList.getProperty("tables");
    Assert(tablesP != NULL);

    int i = 0;
    while(i < (int) tablesP->size()){
        const std::vector<Property>* preambleP = tablesP->at(i).propList->getProperty("preamble");
        Assert(preambleP != NULL && preambleP->size() == 1);

        const std::vector<Property>* idP = preambleP->at(0).propList->getProperty("id");
        Assert(idP != NULL && idP->size() == 1);

        if(id == IPLBase::intFromStr(idP->at(0).propString.c_str()))
            return tablesP->at(i).propList;
        i++;
    }
    return NULL;
}

std::string P4InfoDesc::getTableNameFromID(const int id){
    PropertyList *tpl = this->getTablePropertyList(id);
    if(tpl == NULL)
        return "";
    const std::vector<Property>* preambleP = tpl->getProperty("preamble");
    Assert(preambleP != NULL);
    const std::vector<Property>* nameP = preambleP->at(0).propList->getProperty("name");
    Assert(nameP != NULL);
    std::string tableName = nameP->at(0).propString;
    if(tableName.size() > 1){
        if(tableName[tableName.size() - 1] == '"')
            tableName.pop_back();
        if(tableName[0] == '"')
            tableName.erase(tableName.begin());
    }
    return tableName;
}

int P4InfoDesc::getTableIDFromName(const std::string& name){
    PropertyList *tpl = this->getTablePropertyList(name);
    if(tpl == NULL)
        return -1;
    const std::vector<Property>* preambleP = tpl->getProperty("preamble");
    Assert(preambleP != NULL);
    const std::vector<Property>* idP = preambleP->at(0).propList->getProperty("id");
    Assert(idP != NULL);
    int id = IPLBase::intFromStr(idP->at(0).propString.c_str());
    Assert(id > 0);
    return id;
}

PropertyList* P4InfoDesc::getActionPropertyList(const std::string& actionName){
    const std::vector<Property>* actionsP = this->m_propertyList.getProperty("actions");
    Assert(actionsP != NULL);

    int i = 0;
    while(i < (int) actionsP->size()){
        const std::vector<Property>* preambleP = actionsP->at(i).propList->getProperty("preamble");
        Assert(preambleP != NULL && preambleP->size() == 1);

        const std::vector<Property>* nameP = preambleP->at(0).propList->getProperty("name");
        Assert(nameP != NULL && nameP->size() == 1);

        if(("\"" + actionName + "\"") == nameP->at(0).propString.c_str())
            return actionsP->at(i).propList;
        i++;
    }
    return NULL;
}

PropertyList* P4InfoDesc::getActionPropertyList(const int id){
    const std::vector<Property>* actionsP = this->m_propertyList.getProperty("actions");
    Assert(actionsP != NULL);

    int i = 0;
    while(i < (int) actionsP->size()){
        const std::vector<Property>* preambleP = actionsP->at(i).propList->getProperty("preamble");
        Assert(preambleP != NULL && preambleP->size() == 1);

        const std::vector<Property>* idP = preambleP->at(0).propList->getProperty("id");
        Assert(idP != NULL && idP->size() == 1);

        if(id == IPLBase::intFromStr(idP->at(0).propString.c_str()))
            return actionsP->at(i).propList;
        i++;
    }
    return NULL;
}

std::string P4InfoDesc::getActionNameFromID(const int id){
    PropertyList *apl = this->getActionPropertyList(id);
    if(apl == NULL)
        return "";
    const std::vector<Property>* preambleP = apl->getProperty("preamble");
    Assert(preambleP != NULL);
    const std::vector<Property>* nameP = preambleP->at(0).propList->getProperty("name");
    Assert(nameP != NULL);
    std::string actionName = nameP->at(0).propString;
    if(actionName.size() > 1){
        if(actionName[actionName.size() - 1] == '"')
            actionName.pop_back();
        if(actionName[0] == '"')
            actionName.erase(actionName.begin());
    }
    return actionName;
}

int P4InfoDesc::getActionIDFromName(const std::string& name){
    PropertyList *apl = this->getActionPropertyList(name);
    if(apl == NULL)
        return -1;
    const std::vector<Property>* preambleP = apl->getProperty("preamble");
    Assert(preambleP != NULL);
    const std::vector<Property>* idP = preambleP->at(0).propList->getProperty("id");
    Assert(idP != NULL);
    int id = IPLBase::intFromStr(idP->at(0).propString.c_str());
    Assert(id > 0);
    return id;
}

p4::config::v1::P4Info P4InfoDesc::GetP4Info(){
    p4::config::v1::P4Info result;

    std::vector<Property> *pkg_info = this->m_propertyList.getProperty("pkg_info");
    if(pkg_info != NULL){
        for(const Property &p : *pkg_info){
            Assert(p.propType == RT_LIST);
            std::vector<Property> *arch = p.propList->getProperty("arch");
            if(arch != NULL){
                for(const Property &ap : *arch){
                    Assert(ap.propType == RT_STRING);
                    result.mutable_pkg_info()->set_arch(IPLBase::stripQuotes(ap.propString));
                }
            }
        }
    }

    std::vector<Property> *tables = this->m_propertyList.getProperty("tables");
    if(tables != NULL){
        for(const Property &t : *tables){
            Assert(t.propType == RT_LIST);
            p4::config::v1::Table *table = result.add_tables();
            
            //Preamble
            std::vector<Property> *preamble = t.propList->getProperty("preamble");
            if(preamble != NULL){
                for(const Property &pp : *preamble){
                    Assert(pp.propType == RT_LIST);
                    std::vector<Property> *id = pp.propList->getProperty("id");
                    if(id != NULL){
                        Assert(id->size() == 1);
                        Assert(IPLBase::isInt(id->at(0).propString.c_str()));
                        table->mutable_preamble()->set_id((uint32_t) IPLBase::intFromStr(id->at(0).propString.c_str()));
                    }
                    std::vector<Property> *name = pp.propList->getProperty("name");
                    if(name != NULL){
                        Assert(name->size() == 1);
                        table->mutable_preamble()->set_name(IPLBase::stripQuotes(name->at(0).propString));
                    }
                    std::vector<Property> *alias = pp.propList->getProperty("alias");
                    if(alias != NULL){
                        Assert(alias->size() == 1);
                        table->mutable_preamble()->set_alias(IPLBase::stripQuotes(alias->at(0).propString));
                    }
                }
            }

            //MatchFields
            std::vector<Property> *match_fields = t.propList->getProperty("match_fields");
            if(match_fields != NULL){
                for(const Property &pp : *match_fields){
                    p4::config::v1::MatchField *match_field = table->add_match_fields();
                    Assert(pp.propType == RT_LIST);
                    std::vector<Property> *id = pp.propList->getProperty("id");
                    if(id != NULL){
                        Assert(id->size() == 1);
                        Assert(IPLBase::isInt(id->at(0).propString.c_str()));
                        match_field->set_id((uint32_t) IPLBase::intFromStr(id->at(0).propString.c_str()));
                    }
                    std::vector<Property> *name = pp.propList->getProperty("name");
                    if(name != NULL){
                        Assert(name->size() == 1);
                        match_field->set_name(IPLBase::stripQuotes(name->at(0).propString));
                    }
                    std::vector<Property> *bitwidth = pp.propList->getProperty("bitwidth");
                    if(bitwidth != NULL){
                        Assert(bitwidth->size() == 1);
                        Assert(IPLBase::isInt(bitwidth->at(0).propString.c_str()));
                        match_field->set_bitwidth((uint32_t) IPLBase::intFromStr(bitwidth->at(0).propString.c_str()));
                    }
                    std::vector<Property> *match_type = pp.propList->getProperty("match_type");
                    if(match_type != NULL){
                        Assert(match_type->size() == 1);
                        if(match_type->at(0).propString == "UNSPECIFIED")
                            match_field->set_match_type(p4::config::v1::MatchField_MatchType_UNSPECIFIED);
                        else if(match_type->at(0).propString == "EXACT")
                            match_field->set_match_type(p4::config::v1::MatchField_MatchType_EXACT);
                        else if(match_type->at(0).propString == "LPM")
                            match_field->set_match_type(p4::config::v1::MatchField_MatchType_LPM);
                        else if(match_type->at(0).propString == "TERNARY")
                            match_field->set_match_type(p4::config::v1::MatchField_MatchType_TERNARY);
                        else if(match_type->at(0).propString == "RANGE")
                            match_field->set_match_type(p4::config::v1::MatchField_MatchType_RANGE);
                        else if(match_type->at(0).propString == "OPTIONAL")
                            match_field->set_match_type(p4::config::v1::MatchField_MatchType_OPTIONAL);
                        else Assert(false);
                    }
                }
            }

            //ActionRefs
            std::vector<Property> *action_refs = t.propList->getProperty("action_refs");
            if(action_refs != NULL){
                for(const Property &pp : *action_refs){
                    p4::config::v1::ActionRef *action_ref = table->add_action_refs();
                    Assert(pp.propType == RT_LIST);
                    std::vector<Property> *id = pp.propList->getProperty("id");
                    if(id != NULL){
                        Assert(id->size() == 1);
                        Assert(IPLBase::isInt(id->at(0).propString.c_str()));
                        action_ref->set_id((uint32_t) IPLBase::intFromStr(id->at(0).propString.c_str()));
                    }
                }
            }

            //const_default_action_id
            std::vector<Property> *cdaid = t.propList->getProperty("const_default_action_id");
            if(cdaid != NULL){
                Assert(cdaid->size() == 1 && cdaid->at(0).propType == RT_STRING);
                Assert(IPLBase::isInt(cdaid->at(0).propString.c_str()));
                table->set_const_default_action_id((uint32_t) IPLBase::intFromStr(cdaid->at(0).propString.c_str()));
            }

            //size
            std::vector<Property> *sizep = t.propList->getProperty("size");
            if(sizep != NULL){
                Assert(sizep->size() == 1 && sizep->at(0).propType == RT_STRING);
                Assert(IPLBase::isInt(sizep->at(0).propString.c_str()));
                table->set_size(IPLBase::intFromStr64(sizep->at(0).propString.c_str()));
            }

            //is_const_table
            std::vector<Property> *isConstTable = t.propList->getProperty("is_const_table");
            if(isConstTable != NULL){
                Assert(isConstTable->size() == 1 && isConstTable->at(0).propType == RT_STRING);
                table->set_is_const_table(isConstTable->at(0).propString == "true");
            }

            //idle_timeout_behavior
            std::vector<Property> *idleTimeoutBehavior = t.propList->getProperty("idle_timeout_behavior");
            if(idleTimeoutBehavior != NULL){
                Assert(idleTimeoutBehavior->size() == 1 && idleTimeoutBehavior->at(0).propType == RT_STRING);
                if(!strcmp(idleTimeoutBehavior->at(0).propString.c_str(), "NO_TIMEOUT"))
                    table->set_idle_timeout_behavior(p4::config::v1::Table_IdleTimeoutBehavior::Table_IdleTimeoutBehavior_NO_TIMEOUT);
                else if(!strcmp(idleTimeoutBehavior->at(0).propString.c_str(), "NOTIFY_CONTROL"))
                    table->set_idle_timeout_behavior(p4::config::v1::Table_IdleTimeoutBehavior::Table_IdleTimeoutBehavior_NOTIFY_CONTROL);
            }

        }
    }

    std::vector<Property> *actions = this->m_propertyList.getProperty("actions");
    if(actions != NULL){
        for(const Property &a : *actions){
            Assert(a.propType == RT_LIST);
            p4::config::v1::Action *action = result.add_actions();

            //Preamble
            std::vector<Property> *preamble = a.propList->getProperty("preamble");
            if(preamble != NULL){
                for(const Property &pp : *preamble){
                    Assert(pp.propType == RT_LIST);
                    std::vector<Property> *id = pp.propList->getProperty("id");
                    if(id != NULL){
                        Assert(id->size() == 1);
                        Assert(IPLBase::isInt(id->at(0).propString.c_str()));
                        action->mutable_preamble()->set_id((uint32_t) IPLBase::intFromStr(id->at(0).propString.c_str()));
                    }
                    std::vector<Property> *name = pp.propList->getProperty("name");
                    if(name != NULL){
                        Assert(name->size() == 1);
                        action->mutable_preamble()->set_name(IPLBase::stripQuotes(name->at(0).propString));
                    }
                    std::vector<Property> *alias = pp.propList->getProperty("alias");
                    if(alias != NULL){
                        Assert(alias->size() == 1);
                        action->mutable_preamble()->set_alias(IPLBase::stripQuotes(alias->at(0).propString));
                    }
                    std::vector<Property> *annotations = pp.propList->getProperty("annotations");
                    if(annotations != NULL){
                        for(const Property& ap : *annotations){
                            Assert(ap.propType == RT_STRING)
                            std::string *annotation = action->mutable_preamble()->add_annotations();
                            *annotation = IPLBase::stripQuotes(ap.propString);
                        }
                    }
                }
            }

            //Params
            std::vector<Property> *params = a.propList->getProperty("params");
            if(params != NULL){
                for(const Property &pp : *params){
                    p4::config::v1::Action_Param *action_param = action->add_params();
                    Assert(pp.propType == RT_LIST);
                    std::vector<Property> *id = pp.propList->getProperty("id");
                    if(id != NULL){
                        Assert(id->size() == 1);
                        Assert(IPLBase::isInt(id->at(0).propString.c_str()));
                        action_param->set_id((uint32_t) IPLBase::intFromStr(id->at(0).propString.c_str()));
                    }
                    std::vector<Property> *name = pp.propList->getProperty("name");
                    if(name != NULL){
                        Assert(name->size() == 1);
                        action_param->set_name(IPLBase::stripQuotes(name->at(0).propString));
                    }
                    std::vector<Property> *bitwidth = pp.propList->getProperty("bitwidth");
                    if(bitwidth != NULL){
                        Assert(bitwidth->size() == 1);
                        Assert(IPLBase::isInt(bitwidth->at(0).propString.c_str()));
                        action_param->set_bitwidth((uint32_t) IPLBase::intFromStr(bitwidth->at(0).propString.c_str()));
                    }
                }
            }
        }
    }

    std::vector<Property> *controllerPacketMetadatas = this->m_propertyList.getProperty("controller_packet_metadata");
    if(controllerPacketMetadatas != NULL){
        for(const Property &cpm : *controllerPacketMetadatas){
            Assert(cpm.propType == RT_LIST);

            p4::config::v1::ControllerPacketMetadata *cpMetadata = result.add_controller_packet_metadata();

            //Preamble
            std::vector<Property> *preamble = cpm.propList->getProperty("preamble");
            if(preamble != NULL){
                for(const Property &pp : *preamble){
                    Assert(pp.propType == RT_LIST);
                    std::vector<Property> *id = pp.propList->getProperty("id");
                    if(id != NULL){
                        Assert(id->size() == 1);
                        Assert(IPLBase::isInt(id->at(0).propString.c_str()));
                        cpMetadata->mutable_preamble()->set_id((uint32_t) IPLBase::intFromStr(id->at(0).propString.c_str()));
                    }
                    std::vector<Property> *name = pp.propList->getProperty("name");
                    if(name != NULL){
                        Assert(name->size() == 1);
                        cpMetadata->mutable_preamble()->set_name(IPLBase::stripQuotes(name->at(0).propString));
                    }
                    std::vector<Property> *alias = pp.propList->getProperty("alias");
                    if(alias != NULL){
                        Assert(alias->size() == 1);
                        cpMetadata->mutable_preamble()->set_alias(IPLBase::stripQuotes(alias->at(0).propString));
                    }
                    std::vector<Property> *annotations = pp.propList->getProperty("annotations");
                    if(annotations != NULL){
                        for(const Property& ap : *annotations){
                            Assert(ap.propType == RT_STRING)
                            std::string *annotation = cpMetadata->mutable_preamble()->add_annotations();
                            *annotation = IPLBase::stripQuotes(ap.propString);
                        }
                    }
                }
            }

            //Params
            std::vector<Property> *metadatas = cpm.propList->getProperty("metadata");
            if(metadatas != NULL){
                for(const Property &md : *metadatas){
                    p4::config::v1::ControllerPacketMetadata_Metadata *cpm_metadata = cpMetadata->add_metadata();
                    Assert(md.propType == RT_LIST);
                    std::vector<Property> *id = md.propList->getProperty("id");
                    if(id != NULL){
                        Assert(id->size() == 1);
                        Assert(IPLBase::isInt(id->at(0).propString.c_str()));
                        cpm_metadata->set_id((uint32_t) IPLBase::intFromStr(id->at(0).propString.c_str()));
                    }
                    std::vector<Property> *name = md.propList->getProperty("name");
                    if(name != NULL){
                        Assert(name->size() == 1);
                        cpm_metadata->set_name(IPLBase::stripQuotes(name->at(0).propString));
                    }
                    std::vector<Property> *bitwidth = md.propList->getProperty("bitwidth");
                    if(bitwidth != NULL){
                        Assert(bitwidth->size() == 1);
                        Assert(IPLBase::isInt(bitwidth->at(0).propString.c_str()));
                        cpm_metadata->set_bitwidth((uint32_t) IPLBase::intFromStr(bitwidth->at(0).propString.c_str()));
                    }
                }
            }
        }
    }

    return result;
}
