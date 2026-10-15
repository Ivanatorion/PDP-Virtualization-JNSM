#include "../include/pcheaders.h"
#include "../proto/bfruntime.grpc.pb.h"
#include "../proto/p4runtime.grpc.pb.h"
#include "../include/Utils.h"
#include "../include/PropertyList.h"
#include "../include/BFRTInfoDesc.h"

bool Utils::EnablePrints = true;
std::string Utils::TabSpaces = "";
int Utils::TabIncrease = 4;
FILE* Utils::OutputStream = stdout;
BFRTInfoDesc* Utils::bfrtInfo = NULL;

void UtilsIncreaseTabSpaces(){
    for(int i = 0; i < Utils::TabIncrease; i++)
        Utils::TabSpaces.push_back(' ');
}

void UtilsDecreaseTabSpaces(){
    for(int i = 0; i < Utils::TabIncrease; i++)
        Utils::TabSpaces.pop_back();
}

std::string Utils::StringBytes(const std::string& str){
    const int slen = ((int) str.length()) * 4 + 1;
    char* buffer = (char*) malloc(sizeof(char) * slen);
    buffer[slen - 1] = '\0';

    for(int i = 0; i < (int) str.length(); i++){
        const uint8_t c = static_cast<uint8_t>(str[i]);
        
        buffer[i*4] = '\\';
        buffer[1 + i*4] = (c / 64) + '0';
        buffer[2 + i*4] = ((c % 64) / 8) + '0';
        buffer[3 + i*4] = (c % 8) + '0';
    }

    std::string result(buffer);
    free(buffer);
    return result;
}

std::string Utils::UpdateTypeToStringBFRT(const bfrt_proto::Update_Type update){
    switch(update){
        case bfrt_proto::Update_Type::Update_Type_INSERT:
            return "INSERT";
        case bfrt_proto::Update_Type::Update_Type_MODIFY:
            return "MODIFY";
        case bfrt_proto::Update_Type::Update_Type_DELETE:
            return "DELETE";
        case bfrt_proto::Update_Type::Update_Type_UNSPECIFIED:
            return "UNSPECIFIED";
        default:
            break;
    }
    return "";
}

std::string Utils::UpdateTypeToString(const p4::v1::Update_Type update){
    switch(update){
        case p4::v1::Update_Type::Update_Type_INSERT:
            return "INSERT";
        case p4::v1::Update_Type::Update_Type_MODIFY:
            return "MODIFY";
        case p4::v1::Update_Type::Update_Type_DELETE:
            return "DELETE";
        case p4::v1::Update_Type::Update_Type_UNSPECIFIED:
            return "UNSPECIFIED";
        default:
            break;
    }
    return "";
}

std::string Utils::WriteRequestAtomicityToString(const p4::v1::WriteRequest_Atomicity atomicity){
    switch(atomicity){
        case p4::v1::WriteRequest_Atomicity::WriteRequest_Atomicity_CONTINUE_ON_ERROR:
            return "CONTINUE_ON_ERROR";
        case p4::v1::WriteRequest_Atomicity::WriteRequest_Atomicity_DATAPLANE_ATOMIC:
            return "DATAPLANE_ATOMIC";
        case p4::v1::WriteRequest_Atomicity::WriteRequest_Atomicity_ROLLBACK_ON_ERROR:
            return "ROLLBACK_ON_ERROR";
        default:
            break;
    }
    return "";
}

void Utils::PrintFieldMatch(const p4::v1::FieldMatch& match){
    fprintf(Utils::OutputStream, "%sfield_id: %d\n", Utils::TabSpaces.c_str(), match.field_id());
    if(match.has_exact()){
        fprintf(Utils::OutputStream, "%sexact {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        fprintf(Utils::OutputStream, "%svalue: %s\n", Utils::TabSpaces.c_str(), Utils::StringBytes(match.exact().value()).c_str());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(match.has_ternary()){
        fprintf(Utils::OutputStream, "%sternary {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        fprintf(Utils::OutputStream, "%svalue: %s\n", Utils::TabSpaces.c_str(), Utils::StringBytes(match.ternary().value()).c_str());
        fprintf(Utils::OutputStream, "%smask: %s\n", Utils::TabSpaces.c_str(), match.ternary().mask().c_str());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(match.has_lpm()){
        fprintf(Utils::OutputStream, "%slpm {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        fprintf(Utils::OutputStream, "%svalue: %s\n", Utils::TabSpaces.c_str(), Utils::StringBytes(match.lpm().value()).c_str());
        fprintf(Utils::OutputStream, "%sprefix_len: %d\n", Utils::TabSpaces.c_str(), match.lpm().prefix_len());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(match.has_range()){
        fprintf(Utils::OutputStream, "%srange {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        fprintf(Utils::OutputStream, "%slow: %s\n", Utils::TabSpaces.c_str(), match.range().low().c_str());
        fprintf(Utils::OutputStream, "%shigh: %s\n", Utils::TabSpaces.c_str(), match.range().high().c_str());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(match.has_optional()){
        fprintf(Utils::OutputStream, "%soptional {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        fprintf(Utils::OutputStream, "%svalue: %s\n", Utils::TabSpaces.c_str(), Utils::StringBytes(match.optional().value()).c_str());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
}

void Utils::PrintAction(const p4::v1::Action& action){
    fprintf(Utils::OutputStream, "%saction_id: %d\n", Utils::TabSpaces.c_str(), action.action_id());
    fprintf(Utils::OutputStream, "%sparams {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    for(p4::v1::Action_Param param : action.params()){
        fprintf(Utils::OutputStream, "%sparam {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        fprintf(Utils::OutputStream, "%sparam_id: %d\n", Utils::TabSpaces.c_str(), param.param_id());
        fprintf(Utils::OutputStream, "%svalue: %s\n", Utils::TabSpaces.c_str(), Utils::StringBytes(param.value()).c_str());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintTableAction(const p4::v1::TableAction& table_action){
    if(table_action.has_action()){
        fprintf(Utils::OutputStream, "%saction {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintAction(table_action.action());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(table_action.has_action_profile_member_id())
        fprintf(Utils::OutputStream, "%saction_profile_member_id: %d\n", Utils::TabSpaces.c_str(), table_action.action_profile_member_id());
    else if(table_action.has_action_profile_group_id())
        fprintf(Utils::OutputStream, "%saction_profile_group_id: %d\n", Utils::TabSpaces.c_str(), table_action.action_profile_group_id());
}

void Utils::PrintExternEntry(const p4::v1::ExternEntry& extern_entry){
    //TODO
}

void Utils::PrintTableEntry(const p4::v1::TableEntry& table_entry){
    fprintf(Utils::OutputStream, "%stable_id: %d\n", Utils::TabSpaces.c_str(), table_entry.table_id());
    fprintf(Utils::OutputStream, "%sidle_timeout_ns: %lu\n", Utils::TabSpaces.c_str(), table_entry.idle_timeout_ns());
    fprintf(Utils::OutputStream, "%saction {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    Utils::PrintTableAction(table_entry.action());
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    fprintf(Utils::OutputStream, "%sMatches: {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    for(p4::v1::FieldMatch fieldMatch : table_entry.match()){
        fprintf(Utils::OutputStream, "%smatch {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintFieldMatch(fieldMatch);
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintActionProfileMember(const p4::v1::ActionProfileMember& action_profile_member){
    //TODO
}

void Utils::PrintActionProfileGroup(const p4::v1::ActionProfileGroup& action_profile_group){
    //TODO
}

void Utils::PrintMeterEntry(const p4::v1::MeterEntry& meter_entry){
    //TODO
}

void Utils::PrintDirectMeterEntry(const p4::v1::DirectMeterEntry& direct_meter_entry){
    //TODO
}

void Utils::PrintCounterEntry(const p4::v1::CounterEntry& counter_entry){
    //TODO
}

void Utils::PrintDirectCounterEntry(const p4::v1::DirectCounterEntry& direct_counter_entry){
    //TODO
}

void Utils::PrintPacketReplicationEngineEntry(const p4::v1::PacketReplicationEngineEntry& packet_replication_engine_entry){
    //TODO
}

void Utils::PrintValueSetEntry(const p4::v1::ValueSetEntry& value_set_entry){
    //TODO
}

void Utils::PrintRegisterEntry(const p4::v1::RegisterEntry& register_entry){
    //TODO
}

void Utils::PrintDigestEntry(const p4::v1::DigestEntry& digest_entry){
    //TODO
}

void Utils::PrintEntity(const p4::v1::Entity& entity){
    if(entity.has_extern_entry()){
        fprintf(Utils::OutputStream, "%sextern_entry {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintExternEntry(entity.extern_entry());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(entity.has_table_entry()){
        fprintf(Utils::OutputStream, "%stable_entry {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintTableEntry(entity.table_entry());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(entity.has_action_profile_member()){
        fprintf(Utils::OutputStream, "%saction_profile_member {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintActionProfileMember(entity.action_profile_member());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(entity.has_action_profile_group()){
        fprintf(Utils::OutputStream, "%saction_profile_group {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintActionProfileGroup(entity.action_profile_group());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(entity.has_meter_entry()){
        fprintf(Utils::OutputStream, "%smeter_entry {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintMeterEntry(entity.meter_entry());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(entity.has_direct_meter_entry()){
        fprintf(Utils::OutputStream, "%sdirect_meter_entry {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintDirectMeterEntry(entity.direct_meter_entry());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(entity.has_counter_entry()){
        fprintf(Utils::OutputStream, "%scounter_entry {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintCounterEntry(entity.counter_entry());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(entity.has_direct_counter_entry()){
        fprintf(Utils::OutputStream, "%sdirect_counter_entry {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintDirectCounterEntry(entity.direct_counter_entry());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(entity.has_packet_replication_engine_entry()){
        fprintf(Utils::OutputStream, "%spacket_replication_engine_entry {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintPacketReplicationEngineEntry(entity.packet_replication_engine_entry());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(entity.has_value_set_entry()){
        fprintf(Utils::OutputStream, "%svalue_set_entry {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintValueSetEntry(entity.value_set_entry());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(entity.has_register_entry()){
        fprintf(Utils::OutputStream, "%sregister_entry {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintRegisterEntry(entity.register_entry());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else if(entity.has_digest_entry()){
        fprintf(Utils::OutputStream, "%sdigest_entry {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintDigestEntry(entity.digest_entry());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
}

void Utils::PrintPreamble(const p4::config::v1::Preamble& preamble){
    fprintf(Utils::OutputStream, "%spreamble {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    if(preamble.id() != 0)
        fprintf(Utils::OutputStream, "%sid: %d\n", Utils::TabSpaces.c_str(), preamble.id());
    if(preamble.name() != "")
        fprintf(Utils::OutputStream, "%sname: \"%s\"\n", Utils::TabSpaces.c_str(), preamble.name().c_str());
    if(preamble.alias() != "")
        fprintf(Utils::OutputStream, "%salias: \"%s\"\n", Utils::TabSpaces.c_str(), preamble.alias().c_str());
    for(const std::string& annotation : preamble.annotations())
        fprintf(Utils::OutputStream, "%sannotations: \"%s\"\n", Utils::TabSpaces.c_str(), annotation.c_str());
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintMatchField(const p4::config::v1::MatchField& match_field){
    fprintf(Utils::OutputStream, "%smatch_fields {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    if(match_field.id() != 0)
        fprintf(Utils::OutputStream, "%sid: %d\n", Utils::TabSpaces.c_str(), match_field.id());
    if(match_field.name() != "")
        fprintf(Utils::OutputStream, "%sname: \"%s\"\n", Utils::TabSpaces.c_str(), match_field.name().c_str());
    if(match_field.bitwidth() != 0)
        fprintf(Utils::OutputStream, "%sbitwidth: %d\n", Utils::TabSpaces.c_str(), match_field.bitwidth());
    if(match_field.has_match_type()){
        std::string mts;
        switch(match_field.match_type()){
            case p4::config::v1::MatchField_MatchType_UNSPECIFIED:
                mts = "UNSPECIFIED";
                break;
            case p4::config::v1::MatchField_MatchType_EXACT:
                mts = "EXACT";
                break;
            case p4::config::v1::MatchField_MatchType_LPM :
                mts = "LPM";
                break;
            case p4::config::v1::MatchField_MatchType_TERNARY:
                mts = "TERNARY";
                break;
            case p4::config::v1::MatchField_MatchType_RANGE:
                mts = "RANGE";
                break;
            case p4::config::v1::MatchField_MatchType_OPTIONAL:
                mts = "OPTIONAL";
                break;
            default:
                break;
        }
        fprintf(Utils::OutputStream, "%smatch_type: %s\n", Utils::TabSpaces.c_str(), mts.c_str());
    }
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintActionRef(const p4::config::v1::ActionRef& action_ref){
    fprintf(Utils::OutputStream, "%saction_refs {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    fprintf(Utils::OutputStream, "%sid: %d\n", Utils::TabSpaces.c_str(), action_ref.id());
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintActionParam(const p4::config::v1::Action_Param& param){
    fprintf(Utils::OutputStream, "%sparams {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    fprintf(Utils::OutputStream, "%sid: %d\n", Utils::TabSpaces.c_str(), param.id());
    fprintf(Utils::OutputStream, "%sname: \"%s\"\n", Utils::TabSpaces.c_str(), param.name().c_str());
    fprintf(Utils::OutputStream, "%sbitwidth: %d\n", Utils::TabSpaces.c_str(), param.bitwidth());
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintAction(const p4::config::v1::Action& action){
    fprintf(Utils::OutputStream, "%sactions {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    if(action.has_preamble())
        Utils::PrintPreamble(action.preamble());
    for(const p4::config::v1::Action_Param& param : action.params())
        Utils::PrintActionParam(param);
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintTable(const p4::config::v1::Table& table){
    fprintf(Utils::OutputStream, "%stables {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    if(table.has_preamble())
        Utils::PrintPreamble(table.preamble());
    for(const auto& mf : table.match_fields())
        PrintMatchField(mf);
    for(const auto& ar : table.action_refs())
        PrintActionRef(ar);
    if(table.const_default_action_id() != 0)
        fprintf(Utils::OutputStream, "%sconst_default_action_id: %d\n", Utils::TabSpaces.c_str(), table.const_default_action_id());
    if(table.size() != 0)
        fprintf(Utils::OutputStream, "%ssize: %ld\n", Utils::TabSpaces.c_str(), table.size());
    if(table.is_const_table())
        fprintf(Utils::OutputStream, "%sis_const_table: %s\n", Utils::TabSpaces.c_str(), table.is_const_table() ? "true" : "false");
    if(table.idle_timeout_behavior() == p4::config::v1::Table_IdleTimeoutBehavior_NO_TIMEOUT)
        fprintf(Utils::OutputStream, "%sidle_timeout_behavior: NO_TIMEOUT\n", Utils::TabSpaces.c_str());
    else if(table.idle_timeout_behavior() == p4::config::v1::Table_IdleTimeoutBehavior_NOTIFY_CONTROL)
        fprintf(Utils::OutputStream, "%sidle_timeout_behavior: NOTIFY_CONTROL\n", Utils::TabSpaces.c_str());
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintPkgInfo(const p4::config::v1::PkgInfo& pkginfo){
    fprintf(Utils::OutputStream, "%spkg_info {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    if(pkginfo.arch() != "")
        fprintf(Utils::OutputStream, "%sarch: \"%s\"\n", Utils::TabSpaces.c_str(), pkginfo.arch().c_str());
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintControllerPacketMetadataMetadata(const p4::config::v1::ControllerPacketMetadata_Metadata& metadata){
    fprintf(Utils::OutputStream, "%smetadata {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    fprintf(Utils::OutputStream, "%sid: %d\n", Utils::TabSpaces.c_str(), metadata.id());
    fprintf(Utils::OutputStream, "%sname: \"%s\"\n", Utils::TabSpaces.c_str(), metadata.name().c_str());
    fprintf(Utils::OutputStream, "%sbitwidth: %d\n", Utils::TabSpaces.c_str(), metadata.bitwidth());
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintControllerPacketMetadata(const p4::config::v1::ControllerPacketMetadata& cpm){
    fprintf(Utils::OutputStream, "%scontroller_packet_metadata {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    Utils::PrintPreamble(cpm.preamble());
    for(const p4::config::v1::ControllerPacketMetadata_Metadata& m : cpm.metadata())
        Utils::PrintControllerPacketMetadataMetadata(m);
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintP4Info(const p4::config::v1::P4Info& p4info){
    Utils::PrintPkgInfo(p4info.pkg_info());
    for(const p4::config::v1::Table& table : p4info.tables())
        Utils::PrintTable(table);
    for(const p4::config::v1::Action& action : p4info.actions())
        Utils::PrintAction(action);
    for(const p4::config::v1::ControllerPacketMetadata& cpm : p4info.controller_packet_metadata())
        Utils::PrintControllerPacketMetadata(cpm);
}

void Utils::PrintUpdate(const p4::v1::Update& update){
    fprintf(Utils::OutputStream, "%stype: %s\n", Utils::TabSpaces.c_str(), Utils::UpdateTypeToString(update.type()).c_str());
    fprintf(Utils::OutputStream, "%sentity {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    Utils::PrintEntity(update.entity());
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintWriteRequest(const p4::v1::WriteRequest* request){
    if(!Utils::EnablePrints)
        return;

    if(request->has_election_id()){
        fprintf(Utils::OutputStream, "%selection_id {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        fprintf(Utils::OutputStream, "%slow: %lu\n%shigh: %lu\n", Utils::TabSpaces.c_str(), request->election_id().low(), Utils::TabSpaces.c_str(), request->election_id().high());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    fprintf(Utils::OutputStream, "%sUpdates: {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    for(const p4::v1::Update& update : request->updates()){
        fprintf(Utils::OutputStream, "%supdate {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintUpdate(update);
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n%sAtomicity: %s\n", Utils::TabSpaces.c_str(), Utils::TabSpaces.c_str(), Utils::WriteRequestAtomicityToString(request->atomicity()).c_str());
}

void Utils::PrintTableDataBFRT(const bfrt_proto::TableData& table_data){
    fprintf(Utils::OutputStream, "%saction_id: %d", Utils::TabSpaces.c_str(), table_data.action_id());
    if(Utils::bfrtInfo != NULL){
        const std::string name = Utils::bfrtInfo->GetActionName(table_data.action_id());
        fprintf(Utils::OutputStream, " (%s)", name.c_str());
    }
    fprintf(Utils::OutputStream, "\n");
    for(const bfrt_proto::DataField& df : table_data.fields()){
        fprintf(Utils::OutputStream, "%sdata_field {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        fprintf(Utils::OutputStream, "%sfield_id: %d\n", Utils::TabSpaces.c_str(), df.field_id());
        if(df.has_stream()){
            fprintf(Utils::OutputStream, "%sstream: %s\n", Utils::TabSpaces.c_str(), Utils::StringBytes(df.stream()).c_str());
        }
        else{
            fprintf(Utils::OutputStream, "PRINT NOT IMPLEMENTED\n");
        }
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
}

void Utils::PrintTableEntryBFRT(const bfrt_proto::TableEntry& table_entry){
    fprintf(Utils::OutputStream, "%stable_id: %d", Utils::TabSpaces.c_str(), table_entry.table_id());
    if(Utils::bfrtInfo != NULL){
        const std::string name = Utils::bfrtInfo->GetTableName(table_entry.table_id());
        fprintf(Utils::OutputStream, " (%s)", name.c_str());
    }
    fprintf(Utils::OutputStream, "\n");

    if(table_entry.has_key()){
        fprintf(Utils::OutputStream, "%skey {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        for(const bfrt_proto::KeyField& field_match : table_entry.key().fields()){
            fprintf(Utils::OutputStream, "%sfield {\n", Utils::TabSpaces.c_str());
            UtilsIncreaseTabSpaces();
            fprintf(Utils::OutputStream, "%sfield_id: %d\n", Utils::TabSpaces.c_str(), field_match.field_id());
            if(field_match.has_exact()){
                fprintf(Utils::OutputStream, "%sexact {\n", Utils::TabSpaces.c_str());
                UtilsIncreaseTabSpaces();
                fprintf(Utils::OutputStream, "%svalue: %s\n", Utils::TabSpaces.c_str(), Utils::StringBytes(field_match.exact().value()).c_str());
                UtilsDecreaseTabSpaces();
                fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
            }
            else{
                fprintf(Utils::OutputStream, "PRINT NOT IMPLEMENTED\n");
            }
            UtilsDecreaseTabSpaces();
        }
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    
    fprintf(Utils::OutputStream, "%sdata {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    Utils::PrintTableDataBFRT(table_entry.data());
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    fprintf(Utils::OutputStream, "%sis_default_entry: %s\n", Utils::TabSpaces.c_str(), table_entry.is_default_entry() ? "true" : "false");
}

void Utils::PrintEntityBFRT(const bfrt_proto::Entity& entity){
    if(entity.has_table_entry()){
        fprintf(Utils::OutputStream, "%stable_entry {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintTableEntryBFRT(entity.table_entry());
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    else{
        fprintf(Utils::OutputStream, "PRINT NOT IMPLEMENTED\n");
    }
}

void Utils::PrintUpdateBFRT(const bfrt_proto::Update& update){
    fprintf(Utils::OutputStream, "%stype: %s\n", Utils::TabSpaces.c_str(), Utils::UpdateTypeToStringBFRT(update.type()).c_str());
    fprintf(Utils::OutputStream, "%sentity {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    Utils::PrintEntityBFRT(update.entity());
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintWriteRequestBFRT(const bfrt_proto::WriteRequest* request, BFRTInfoDesc* bfrtInfoDesc){
    if(!Utils::EnablePrints)
        return;

    bfrtInfo = bfrtInfoDesc;
    fprintf(Utils::OutputStream, "%sUpdates: {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    for(const bfrt_proto::Update& update : request->updates()){
        fprintf(Utils::OutputStream, "%supdate {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintUpdateBFRT(update);
        UtilsDecreaseTabSpaces();
    }
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    bfrtInfo = NULL;
}

void Utils::PrintWriteRequestBFRT(const bfrt_proto::WriteRequest* request){
    Utils::PrintWriteRequestBFRT(request, NULL);
}

void Utils::PrintReadRequest(const p4::v1::ReadRequest* request){
    if(!Utils::EnablePrints)
        return;

    fprintf(Utils::OutputStream, "%sEntities: {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    for(const p4::v1::Entity& entity : request->entities()){
        fprintf(Utils::OutputStream, "%sentity {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintEntity(entity);
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

void Utils::PrintReadResponse(const p4::v1::ReadResponse* response){
    if(!Utils::EnablePrints)
        return;

    fprintf(Utils::OutputStream, "%sEntities: {\n", Utils::TabSpaces.c_str());
    UtilsIncreaseTabSpaces();
    for(const p4::v1::Entity& entity : response->entities()){
        fprintf(Utils::OutputStream, "%sentity {\n", Utils::TabSpaces.c_str());
        UtilsIncreaseTabSpaces();
        Utils::PrintEntity(entity);
        UtilsDecreaseTabSpaces();
        fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
    }
    UtilsDecreaseTabSpaces();
    fprintf(Utils::OutputStream, "%s}\n", Utils::TabSpaces.c_str());
}

std::string Utils::binaryToBytes(const std::string& binaryString) {
    std::string paddedString;
    if(binaryString.length() % 8 != 0)
        for(size_t i = 0; i < 8 - (binaryString.length() % 8); i++)
            paddedString.push_back('0');
    paddedString = paddedString + binaryString;
   
    const int NumBytes = (int) (paddedString.size() / 8);
    char *buffer = new char[NumBytes];

    for (int i = 0; i < NumBytes; i++)
        buffer[i] = binaryStringToByte(paddedString.substr(i * 8, 8));

    std::string result(buffer, NumBytes);
    delete[] buffer;

    return result;
}

char Utils::binaryStringToByte(const std::string& binaryGroup) {
    char result = 0;
    for(int i = 0; i < 8; i++){
        if(binaryGroup[7 - i] == '1')
            result = result | ((char) (1 << i));
    }
    return result;
}

int Utils::byteStringToInt(const std::string& byteString){
    int result = 0;

    for(int i = 0; i < (int) byteString.size(); i++)
        result = result * 256 + (byteString[i] >= 0 ? byteString[i] : 256 + byteString[i]);

    return result;
}