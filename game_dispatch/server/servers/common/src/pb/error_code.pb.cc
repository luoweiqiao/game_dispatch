// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: error_code.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "error_code.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace net {

namespace {

const ::google::protobuf::EnumDescriptor* RESULT_CODE_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* MISSION_TYPE_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* MISSION_CYCLE_TYPE_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* REWARD_FLAG_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* GAME_CATE_TYPE_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* GAME_SUB_TYPE_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* EXCHANGE_TYPE_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* ROOM_CONSUME_TYPE_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* ROOM_DEAL_TYPE_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* TABLE_FEE_TYPE_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* TABLE_STATE_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* TABLE_STATE_NIUNIU_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* TABLE_STATE_DICE_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* TABLE_STATE_WAR_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* TABLE_STATE_FIGHT_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* TABLE_STATE_APBNIU_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* TABLE_STATE_ZAJINHUA_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* TABLE_STATE_FISHING_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* TABLE_STATE_ROBNIU_descriptor_ = NULL;

}  // namespace


void protobuf_AssignDesc_error_5fcode_2eproto() {
  protobuf_AddDesc_error_5fcode_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "error_code.proto");
  GOOGLE_CHECK(file != NULL);
  RESULT_CODE_descriptor_ = file->enum_type(0);
  MISSION_TYPE_descriptor_ = file->enum_type(1);
  MISSION_CYCLE_TYPE_descriptor_ = file->enum_type(2);
  REWARD_FLAG_descriptor_ = file->enum_type(3);
  GAME_CATE_TYPE_descriptor_ = file->enum_type(4);
  GAME_SUB_TYPE_descriptor_ = file->enum_type(5);
  EXCHANGE_TYPE_descriptor_ = file->enum_type(6);
  ROOM_CONSUME_TYPE_descriptor_ = file->enum_type(7);
  ROOM_DEAL_TYPE_descriptor_ = file->enum_type(8);
  TABLE_FEE_TYPE_descriptor_ = file->enum_type(9);
  TABLE_STATE_descriptor_ = file->enum_type(10);
  TABLE_STATE_NIUNIU_descriptor_ = file->enum_type(11);
  TABLE_STATE_DICE_descriptor_ = file->enum_type(12);
  TABLE_STATE_WAR_descriptor_ = file->enum_type(13);
  TABLE_STATE_FIGHT_descriptor_ = file->enum_type(14);
  TABLE_STATE_APBNIU_descriptor_ = file->enum_type(15);
  TABLE_STATE_ZAJINHUA_descriptor_ = file->enum_type(16);
  TABLE_STATE_FISHING_descriptor_ = file->enum_type(17);
  TABLE_STATE_ROBNIU_descriptor_ = file->enum_type(18);
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_error_5fcode_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
}

}  // namespace

void protobuf_ShutdownFile_error_5fcode_2eproto() {
}

void protobuf_AddDesc_error_5fcode_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\020error_code.proto\022\003net*\307\004\n\013RESULT_CODE\022"
    "\024\n\020RESULT_CODE_FAIL\020\000\022\027\n\023RESULT_CODE_SUC"
    "CESS\020\001\022\025\n\021RESULT_CODE_NOVIP\020\002\022\032\n\026RESULT_"
    "CODE_CION_ERROR\020\003\022\034\n\030RESULT_CODE_PASSWD_"
    "ERROR\020\004\022\034\n\030RESULT_CODE_NEED_INLOBBY\020\005\022\032\n"
    "\026RESULT_CODE_REPEAT_GET\020\006\022\030\n\024RESULT_CODE"
    "_NOT_COND\020\007\022\033\n\027RESULT_CODE_ERROR_PARAM\020\010"
    "\022\031\n\025RESULT_CODE_NOT_TABLE\020\t\022\030\n\024RESULT_CO"
    "DE_NOT_OWER\020\n\022\031\n\025RESULT_CODE_BLACKLIST\020\013"
    "\022\033\n\027RESULT_CODE_NOT_DIAMOND\020\014\022\036\n\032RESULT_"
    "CODE_ERROR_PLAYERID\020\r\022\032\n\026RESULT_CODE_TAB"
    "LE_FULL\020\016\022\027\n\023RESULT_CODE_GAMEING\020\017\022\033\n\027RE"
    "SULT_CODE_ERROR_STATE\020\020\022\033\n\027RESULT_CODE_L"
    "OGIN_OTHER\020\021\022\032\n\026RESULT_CODE_SVR_REPAIR\020\022"
    "\022\025\n\021RESULT_CODE_CDING\020\023\022\030\n\024RESULT_CODE_I"
    "P_LIMIT\020\024*\235\001\n\014MISSION_TYPE\022\025\n\021MISSION_TY"
    "PE_PLAY\020e\022\024\n\020MISSION_TYPE_WIN\020f\022\025\n\021MISSI"
    "ON_TYPE_KILL\020g\022\026\n\022MISSION_TYPE_PRESS\020h\022\027"
    "\n\023MISSION_TYPE_FEEWIN\020i\022\030\n\024MISSION_TYPE_"
    "FEELOSE\020j*k\n\022MISSION_CYCLE_TYPE\022\032\n\026MISSI"
    "ON_CYCLE_TYPE_DAY\020\001\022\033\n\027MISSION_CYCLE_TYP"
    "E_WEEK\020\002\022\034\n\030MISSION_CYCLE_TYPE_MONTH\020\003*\\"
    "\n\013REWARD_FLAG\022\021\n\rREWARD_CLOGIN\020\001\022\022\n\016REWA"
    "RD_WLOGIN3\020\002\022\022\n\016REWARD_WLOGIN5\020\003\022\022\n\016REWA"
    "RD_WLOGIN6\020\004*\267\003\n\016GAME_CATE_TYPE\022\022\n\016GAME_"
    "CATE_LAND\020\001\022\026\n\022GAME_CATE_SHOWHAND\020\002\022\027\n\023G"
    "AME_CATE_BULLFIGHT\020\003\022\023\n\017GAME_CATE_TEXAS\020"
    "\004\022\026\n\022GAME_CATE_ZAJINHUA\020\005\022\024\n\020GAME_CATE_N"
    "IUNIU\020\006\022\026\n\022GAME_CATE_BACCARAT\020\007\022\025\n\021GAME_"
    "CATE_SANGONG\020\010\022\024\n\020GAME_CATE_PAIJIU\020\t\022\030\n\024"
    "GAME_CATE_EVERYCOLOR\020\n\022\022\n\016GAME_CATE_DICE"
    "\020\013\022 \n\034GAME_CATE_TWO_PEOPLE_MAJIANG\020\014\022\033\n\027"
    "GAME_CATE_FRUIT_MACHINE\020\r\022\021\n\rGAME_CATE_W"
    "AR\020\016\022\023\n\017GAME_CATE_FIGHT\020\017\022\024\n\020GAME_CATE_R"
    "OBNIU\020\020\022\025\n\021GAME_CATE_FISHING\020\021\022\026\n\022GAME_C"
    "ATE_MAX_TYPE\020\022*N\n\rGAME_SUB_TYPE\022\023\n\017GAME_"
    "SUB_COMMON\020\001\022\022\n\016GAME_SUB_MATCH\020\002\022\024\n\020GAME"
    "_SUB_PRIVATE\020\003*@\n\rEXCHANGE_TYPE\022\027\n\023EXCHA"
    "NGE_TYPE_SCORE\020\001\022\026\n\022EXCHANGE_TYPE_COIN\020\002"
    "*L\n\021ROOM_CONSUME_TYPE\022\033\n\027ROOM_CONSUME_TY"
    "PE_SCORE\020\001\022\032\n\026ROOM_CONSUME_TYPE_COIN\020\002*]"
    "\n\016ROOM_DEAL_TYPE\022\027\n\023ROOM_DEAL_TYPE_SOLO\020"
    "\001\022\030\n\024ROOM_DEAL_TYPE_THREE\020\002\022\030\n\024ROOM_DEAL"
    "_TYPE_SEVEN\020\003*[\n\016TABLE_FEE_TYPE\022\025\n\021TABLE"
    "_FEE_TYPE_NO\020\000\022\032\n\026TABLE_FEE_TYPE_ALLBASE"
    "\020\001\022\026\n\022TABLE_FEE_TYPE_WIN\020\002*\177\n\013TABLE_STAT"
    "E\022\024\n\020TABLE_STATE_FREE\020\001\022\024\n\020TABLE_STATE_C"
    "ALL\020\002\022\024\n\020TABLE_STATE_PLAY\020\003\022\024\n\020TABLE_STA"
    "TE_WAIT\020\004\022\030\n\024TABLE_STATE_GAME_END\020\005*w\n\022T"
    "ABLE_STATE_NIUNIU\022\033\n\027TABLE_STATE_NIUNIU_"
    "FREE\020\001\022#\n\037TABLE_STATE_NIUNIU_PLACE_JETTO"
    "N\020\002\022\037\n\033TABLE_STATE_NIUNIU_GAME_END\020\003*o\n\020"
    "TABLE_STATE_DICE\022\031\n\025TABLE_STATE_DICE_FRE"
    "E\020\001\022!\n\035TABLE_STATE_DICE_PLACE_JETTON\020\002\022\035"
    "\n\031TABLE_STATE_DICE_GAME_END\020\003*k\n\017TABLE_S"
    "TATE_WAR\022\030\n\024TABLE_STATE_WAR_FREE\020\001\022 \n\034TA"
    "BLE_STATE_WAR_PLACE_JETTON\020\002\022\034\n\030TABLE_ST"
    "ATE_WAR_GAME_END\020\003*s\n\021TABLE_STATE_FIGHT\022"
    "\032\n\026TABLE_STATE_FIGHT_FREE\020\001\022\"\n\036TABLE_STA"
    "TE_FIGHT_PLACE_JETTON\020\002\022\036\n\032TABLE_STATE_F"
    "IGHT_GAME_END\020\003*\317\002\n\022TABLE_STATE_APBNIU\022\033"
    "\n\027TABLE_STATE_APBNIU_FREE\020\001\022\"\n\036TABLE_STA"
    "TE_APBNIU_READY_START\020\002\022!\n\035TABLE_STATE_A"
    "PBNIU_GAME_START\020\003\022$\n TABLE_STATE_APBNIU"
    "_APPLY_BRANKER\020\004\022#\n\037TABLE_STATE_APBNIU_M"
    "AKE_BRANKER\020\005\022#\n\037TABLE_STATE_APBNIU_PLAC"
    "E_JETTON\020\006\022 \n\034TABLE_STATE_APBNIU_SEND_CA"
    "RD\020\007\022\"\n\036TABLE_STATE_APBNIU_CHANGE_CARD\020\010"
    "\022\037\n\033TABLE_STATE_APBNIU_GAME_END\020\t*\274\001\n\024TA"
    "BLE_STATE_ZAJINHUA\022\035\n\031TABLE_STATE_ZAJINH"
    "UA_FREE\020\001\022$\n TABLE_STATE_ZAJINHUA_READY_"
    "START\020\002\022\035\n\031TABLE_STATE_ZAJINHUA_PLAY\020\003\022\035"
    "\n\031TABLE_STATE_ZAJINHUA_WAIT\020\004\022!\n\035TABLE_S"
    "TATE_ZAJINHUA_GAME_END\020\005*S\n\023TABLE_STATE_"
    "FISHING\022\035\n\031TABLE_STATE_FISHING_START\020\001\022\035"
    "\n\031TABLE_STATE_FISHING_PAUSE\020\002*\373\002\n\022TABLE_"
    "STATE_ROBNIU\022\033\n\027TABLE_STATE_ROBNIU_FREE\020"
    "\001\022\"\n\036TABLE_STATE_ROBNIU_READY_START\020\002\022!\n"
    "\035TABLE_STATE_ROBNIU_GAME_START\020\003\022%\n!TABL"
    "E_STATE_ROBNIU_FOUR_SHOW_CARD\020\004\022$\n TABLE"
    "_STATE_ROBNIU_APPLY_BRANKER\020\005\022#\n\037TABLE_S"
    "TATE_ROBNIU_MAKE_BRANKER\020\006\022#\n\037TABLE_STAT"
    "E_ROBNIU_PLACE_JETTON\020\007\022%\n!TABLE_STATE_R"
    "OBNIU_LAST_SHOW_CARD\020\010\022\"\n\036TABLE_STATE_RO"
    "BNIU_CHANGE_CARD\020\t\022\037\n\033TABLE_STATE_ROBNIU"
    "_GAME_END\020\n", 3411);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "error_code.proto", &protobuf_RegisterTypes);
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_error_5fcode_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_error_5fcode_2eproto {
  StaticDescriptorInitializer_error_5fcode_2eproto() {
    protobuf_AddDesc_error_5fcode_2eproto();
  }
} static_descriptor_initializer_error_5fcode_2eproto_;
const ::google::protobuf::EnumDescriptor* RESULT_CODE_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return RESULT_CODE_descriptor_;
}
bool RESULT_CODE_IsValid(int value) {
  switch(value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* MISSION_TYPE_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return MISSION_TYPE_descriptor_;
}
bool MISSION_TYPE_IsValid(int value) {
  switch(value) {
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* MISSION_CYCLE_TYPE_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return MISSION_CYCLE_TYPE_descriptor_;
}
bool MISSION_CYCLE_TYPE_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* REWARD_FLAG_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return REWARD_FLAG_descriptor_;
}
bool REWARD_FLAG_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
    case 4:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* GAME_CATE_TYPE_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return GAME_CATE_TYPE_descriptor_;
}
bool GAME_CATE_TYPE_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* GAME_SUB_TYPE_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return GAME_SUB_TYPE_descriptor_;
}
bool GAME_SUB_TYPE_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* EXCHANGE_TYPE_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return EXCHANGE_TYPE_descriptor_;
}
bool EXCHANGE_TYPE_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* ROOM_CONSUME_TYPE_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return ROOM_CONSUME_TYPE_descriptor_;
}
bool ROOM_CONSUME_TYPE_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* ROOM_DEAL_TYPE_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return ROOM_DEAL_TYPE_descriptor_;
}
bool ROOM_DEAL_TYPE_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* TABLE_FEE_TYPE_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return TABLE_FEE_TYPE_descriptor_;
}
bool TABLE_FEE_TYPE_IsValid(int value) {
  switch(value) {
    case 0:
    case 1:
    case 2:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* TABLE_STATE_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return TABLE_STATE_descriptor_;
}
bool TABLE_STATE_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* TABLE_STATE_NIUNIU_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return TABLE_STATE_NIUNIU_descriptor_;
}
bool TABLE_STATE_NIUNIU_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* TABLE_STATE_DICE_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return TABLE_STATE_DICE_descriptor_;
}
bool TABLE_STATE_DICE_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* TABLE_STATE_WAR_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return TABLE_STATE_WAR_descriptor_;
}
bool TABLE_STATE_WAR_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* TABLE_STATE_FIGHT_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return TABLE_STATE_FIGHT_descriptor_;
}
bool TABLE_STATE_FIGHT_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* TABLE_STATE_APBNIU_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return TABLE_STATE_APBNIU_descriptor_;
}
bool TABLE_STATE_APBNIU_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* TABLE_STATE_ZAJINHUA_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return TABLE_STATE_ZAJINHUA_descriptor_;
}
bool TABLE_STATE_ZAJINHUA_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* TABLE_STATE_FISHING_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return TABLE_STATE_FISHING_descriptor_;
}
bool TABLE_STATE_FISHING_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* TABLE_STATE_ROBNIU_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return TABLE_STATE_ROBNIU_descriptor_;
}
bool TABLE_STATE_ROBNIU_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
      return true;
    default:
      return false;
  }
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace net

// @@protoc_insertion_point(global_scope)
