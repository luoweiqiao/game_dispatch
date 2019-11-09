// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: error_code.proto

#ifndef PROTOBUF_error_5fcode_2eproto__INCLUDED
#define PROTOBUF_error_5fcode_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2005000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2005000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_enum_reflection.h>
// @@protoc_insertion_point(includes)

namespace net {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_error_5fcode_2eproto();
void protobuf_AssignDesc_error_5fcode_2eproto();
void protobuf_ShutdownFile_error_5fcode_2eproto();


enum RESULT_CODE {
  RESULT_CODE_FAIL = 0,
  RESULT_CODE_SUCCESS = 1,
  RESULT_CODE_NOVIP = 2,
  RESULT_CODE_CION_ERROR = 3,
  RESULT_CODE_PASSWD_ERROR = 4,
  RESULT_CODE_NEED_INLOBBY = 5,
  RESULT_CODE_REPEAT_GET = 6,
  RESULT_CODE_NOT_COND = 7,
  RESULT_CODE_ERROR_PARAM = 8,
  RESULT_CODE_NOT_TABLE = 9,
  RESULT_CODE_NOT_OWER = 10,
  RESULT_CODE_BLACKLIST = 11,
  RESULT_CODE_NOT_DIAMOND = 12,
  RESULT_CODE_ERROR_PLAYERID = 13,
  RESULT_CODE_TABLE_FULL = 14,
  RESULT_CODE_GAMEING = 15,
  RESULT_CODE_ERROR_STATE = 16,
  RESULT_CODE_LOGIN_OTHER = 17,
  RESULT_CODE_SVR_REPAIR = 18,
  RESULT_CODE_CDING = 19,
  RESULT_CODE_IP_LIMIT = 20
};
bool RESULT_CODE_IsValid(int value);
const RESULT_CODE RESULT_CODE_MIN = RESULT_CODE_FAIL;
const RESULT_CODE RESULT_CODE_MAX = RESULT_CODE_IP_LIMIT;
const int RESULT_CODE_ARRAYSIZE = RESULT_CODE_MAX + 1;

const ::google::protobuf::EnumDescriptor* RESULT_CODE_descriptor();
inline const ::std::string& RESULT_CODE_Name(RESULT_CODE value) {
  return ::google::protobuf::internal::NameOfEnum(
    RESULT_CODE_descriptor(), value);
}
inline bool RESULT_CODE_Parse(
    const ::std::string& name, RESULT_CODE* value) {
  return ::google::protobuf::internal::ParseNamedEnum<RESULT_CODE>(
    RESULT_CODE_descriptor(), name, value);
}
enum MISSION_TYPE {
  MISSION_TYPE_PLAY = 101,
  MISSION_TYPE_WIN = 102,
  MISSION_TYPE_KILL = 103,
  MISSION_TYPE_PRESS = 104,
  MISSION_TYPE_FEEWIN = 105,
  MISSION_TYPE_FEELOSE = 106
};
bool MISSION_TYPE_IsValid(int value);
const MISSION_TYPE MISSION_TYPE_MIN = MISSION_TYPE_PLAY;
const MISSION_TYPE MISSION_TYPE_MAX = MISSION_TYPE_FEELOSE;
const int MISSION_TYPE_ARRAYSIZE = MISSION_TYPE_MAX + 1;

const ::google::protobuf::EnumDescriptor* MISSION_TYPE_descriptor();
inline const ::std::string& MISSION_TYPE_Name(MISSION_TYPE value) {
  return ::google::protobuf::internal::NameOfEnum(
    MISSION_TYPE_descriptor(), value);
}
inline bool MISSION_TYPE_Parse(
    const ::std::string& name, MISSION_TYPE* value) {
  return ::google::protobuf::internal::ParseNamedEnum<MISSION_TYPE>(
    MISSION_TYPE_descriptor(), name, value);
}
enum MISSION_CYCLE_TYPE {
  MISSION_CYCLE_TYPE_DAY = 1,
  MISSION_CYCLE_TYPE_WEEK = 2,
  MISSION_CYCLE_TYPE_MONTH = 3
};
bool MISSION_CYCLE_TYPE_IsValid(int value);
const MISSION_CYCLE_TYPE MISSION_CYCLE_TYPE_MIN = MISSION_CYCLE_TYPE_DAY;
const MISSION_CYCLE_TYPE MISSION_CYCLE_TYPE_MAX = MISSION_CYCLE_TYPE_MONTH;
const int MISSION_CYCLE_TYPE_ARRAYSIZE = MISSION_CYCLE_TYPE_MAX + 1;

const ::google::protobuf::EnumDescriptor* MISSION_CYCLE_TYPE_descriptor();
inline const ::std::string& MISSION_CYCLE_TYPE_Name(MISSION_CYCLE_TYPE value) {
  return ::google::protobuf::internal::NameOfEnum(
    MISSION_CYCLE_TYPE_descriptor(), value);
}
inline bool MISSION_CYCLE_TYPE_Parse(
    const ::std::string& name, MISSION_CYCLE_TYPE* value) {
  return ::google::protobuf::internal::ParseNamedEnum<MISSION_CYCLE_TYPE>(
    MISSION_CYCLE_TYPE_descriptor(), name, value);
}
enum REWARD_FLAG {
  REWARD_CLOGIN = 1,
  REWARD_WLOGIN3 = 2,
  REWARD_WLOGIN5 = 3,
  REWARD_WLOGIN6 = 4
};
bool REWARD_FLAG_IsValid(int value);
const REWARD_FLAG REWARD_FLAG_MIN = REWARD_CLOGIN;
const REWARD_FLAG REWARD_FLAG_MAX = REWARD_WLOGIN6;
const int REWARD_FLAG_ARRAYSIZE = REWARD_FLAG_MAX + 1;

const ::google::protobuf::EnumDescriptor* REWARD_FLAG_descriptor();
inline const ::std::string& REWARD_FLAG_Name(REWARD_FLAG value) {
  return ::google::protobuf::internal::NameOfEnum(
    REWARD_FLAG_descriptor(), value);
}
inline bool REWARD_FLAG_Parse(
    const ::std::string& name, REWARD_FLAG* value) {
  return ::google::protobuf::internal::ParseNamedEnum<REWARD_FLAG>(
    REWARD_FLAG_descriptor(), name, value);
}
enum GAME_CATE_TYPE {
  GAME_CATE_LAND = 1,
  GAME_CATE_SHOWHAND = 2,
  GAME_CATE_BULLFIGHT = 3,
  GAME_CATE_TEXAS = 4,
  GAME_CATE_ZAJINHUA = 5,
  GAME_CATE_NIUNIU = 6,
  GAME_CATE_BACCARAT = 7,
  GAME_CATE_SANGONG = 8,
  GAME_CATE_PAIJIU = 9,
  GAME_CATE_EVERYCOLOR = 10,
  GAME_CATE_DICE = 11,
  GAME_CATE_TWO_PEOPLE_MAJIANG = 12,
  GAME_CATE_FRUIT_MACHINE = 13,
  GAME_CATE_WAR = 14,
  GAME_CATE_FIGHT = 15,
  GAME_CATE_ROBNIU = 16,
  GAME_CATE_FISHING = 17,
  GAME_CATE_MAX_TYPE = 18
};
bool GAME_CATE_TYPE_IsValid(int value);
const GAME_CATE_TYPE GAME_CATE_TYPE_MIN = GAME_CATE_LAND;
const GAME_CATE_TYPE GAME_CATE_TYPE_MAX = GAME_CATE_MAX_TYPE;
const int GAME_CATE_TYPE_ARRAYSIZE = GAME_CATE_TYPE_MAX + 1;

const ::google::protobuf::EnumDescriptor* GAME_CATE_TYPE_descriptor();
inline const ::std::string& GAME_CATE_TYPE_Name(GAME_CATE_TYPE value) {
  return ::google::protobuf::internal::NameOfEnum(
    GAME_CATE_TYPE_descriptor(), value);
}
inline bool GAME_CATE_TYPE_Parse(
    const ::std::string& name, GAME_CATE_TYPE* value) {
  return ::google::protobuf::internal::ParseNamedEnum<GAME_CATE_TYPE>(
    GAME_CATE_TYPE_descriptor(), name, value);
}
enum GAME_SUB_TYPE {
  GAME_SUB_COMMON = 1,
  GAME_SUB_MATCH = 2,
  GAME_SUB_PRIVATE = 3
};
bool GAME_SUB_TYPE_IsValid(int value);
const GAME_SUB_TYPE GAME_SUB_TYPE_MIN = GAME_SUB_COMMON;
const GAME_SUB_TYPE GAME_SUB_TYPE_MAX = GAME_SUB_PRIVATE;
const int GAME_SUB_TYPE_ARRAYSIZE = GAME_SUB_TYPE_MAX + 1;

const ::google::protobuf::EnumDescriptor* GAME_SUB_TYPE_descriptor();
inline const ::std::string& GAME_SUB_TYPE_Name(GAME_SUB_TYPE value) {
  return ::google::protobuf::internal::NameOfEnum(
    GAME_SUB_TYPE_descriptor(), value);
}
inline bool GAME_SUB_TYPE_Parse(
    const ::std::string& name, GAME_SUB_TYPE* value) {
  return ::google::protobuf::internal::ParseNamedEnum<GAME_SUB_TYPE>(
    GAME_SUB_TYPE_descriptor(), name, value);
}
enum EXCHANGE_TYPE {
  EXCHANGE_TYPE_SCORE = 1,
  EXCHANGE_TYPE_COIN = 2
};
bool EXCHANGE_TYPE_IsValid(int value);
const EXCHANGE_TYPE EXCHANGE_TYPE_MIN = EXCHANGE_TYPE_SCORE;
const EXCHANGE_TYPE EXCHANGE_TYPE_MAX = EXCHANGE_TYPE_COIN;
const int EXCHANGE_TYPE_ARRAYSIZE = EXCHANGE_TYPE_MAX + 1;

const ::google::protobuf::EnumDescriptor* EXCHANGE_TYPE_descriptor();
inline const ::std::string& EXCHANGE_TYPE_Name(EXCHANGE_TYPE value) {
  return ::google::protobuf::internal::NameOfEnum(
    EXCHANGE_TYPE_descriptor(), value);
}
inline bool EXCHANGE_TYPE_Parse(
    const ::std::string& name, EXCHANGE_TYPE* value) {
  return ::google::protobuf::internal::ParseNamedEnum<EXCHANGE_TYPE>(
    EXCHANGE_TYPE_descriptor(), name, value);
}
enum ROOM_CONSUME_TYPE {
  ROOM_CONSUME_TYPE_SCORE = 1,
  ROOM_CONSUME_TYPE_COIN = 2
};
bool ROOM_CONSUME_TYPE_IsValid(int value);
const ROOM_CONSUME_TYPE ROOM_CONSUME_TYPE_MIN = ROOM_CONSUME_TYPE_SCORE;
const ROOM_CONSUME_TYPE ROOM_CONSUME_TYPE_MAX = ROOM_CONSUME_TYPE_COIN;
const int ROOM_CONSUME_TYPE_ARRAYSIZE = ROOM_CONSUME_TYPE_MAX + 1;

const ::google::protobuf::EnumDescriptor* ROOM_CONSUME_TYPE_descriptor();
inline const ::std::string& ROOM_CONSUME_TYPE_Name(ROOM_CONSUME_TYPE value) {
  return ::google::protobuf::internal::NameOfEnum(
    ROOM_CONSUME_TYPE_descriptor(), value);
}
inline bool ROOM_CONSUME_TYPE_Parse(
    const ::std::string& name, ROOM_CONSUME_TYPE* value) {
  return ::google::protobuf::internal::ParseNamedEnum<ROOM_CONSUME_TYPE>(
    ROOM_CONSUME_TYPE_descriptor(), name, value);
}
enum ROOM_DEAL_TYPE {
  ROOM_DEAL_TYPE_SOLO = 1,
  ROOM_DEAL_TYPE_THREE = 2,
  ROOM_DEAL_TYPE_SEVEN = 3
};
bool ROOM_DEAL_TYPE_IsValid(int value);
const ROOM_DEAL_TYPE ROOM_DEAL_TYPE_MIN = ROOM_DEAL_TYPE_SOLO;
const ROOM_DEAL_TYPE ROOM_DEAL_TYPE_MAX = ROOM_DEAL_TYPE_SEVEN;
const int ROOM_DEAL_TYPE_ARRAYSIZE = ROOM_DEAL_TYPE_MAX + 1;

const ::google::protobuf::EnumDescriptor* ROOM_DEAL_TYPE_descriptor();
inline const ::std::string& ROOM_DEAL_TYPE_Name(ROOM_DEAL_TYPE value) {
  return ::google::protobuf::internal::NameOfEnum(
    ROOM_DEAL_TYPE_descriptor(), value);
}
inline bool ROOM_DEAL_TYPE_Parse(
    const ::std::string& name, ROOM_DEAL_TYPE* value) {
  return ::google::protobuf::internal::ParseNamedEnum<ROOM_DEAL_TYPE>(
    ROOM_DEAL_TYPE_descriptor(), name, value);
}
enum TABLE_FEE_TYPE {
  TABLE_FEE_TYPE_NO = 0,
  TABLE_FEE_TYPE_ALLBASE = 1,
  TABLE_FEE_TYPE_WIN = 2
};
bool TABLE_FEE_TYPE_IsValid(int value);
const TABLE_FEE_TYPE TABLE_FEE_TYPE_MIN = TABLE_FEE_TYPE_NO;
const TABLE_FEE_TYPE TABLE_FEE_TYPE_MAX = TABLE_FEE_TYPE_WIN;
const int TABLE_FEE_TYPE_ARRAYSIZE = TABLE_FEE_TYPE_MAX + 1;

const ::google::protobuf::EnumDescriptor* TABLE_FEE_TYPE_descriptor();
inline const ::std::string& TABLE_FEE_TYPE_Name(TABLE_FEE_TYPE value) {
  return ::google::protobuf::internal::NameOfEnum(
    TABLE_FEE_TYPE_descriptor(), value);
}
inline bool TABLE_FEE_TYPE_Parse(
    const ::std::string& name, TABLE_FEE_TYPE* value) {
  return ::google::protobuf::internal::ParseNamedEnum<TABLE_FEE_TYPE>(
    TABLE_FEE_TYPE_descriptor(), name, value);
}
enum TABLE_STATE {
  TABLE_STATE_FREE = 1,
  TABLE_STATE_CALL = 2,
  TABLE_STATE_PLAY = 3,
  TABLE_STATE_WAIT = 4,
  TABLE_STATE_GAME_END = 5
};
bool TABLE_STATE_IsValid(int value);
const TABLE_STATE TABLE_STATE_MIN = TABLE_STATE_FREE;
const TABLE_STATE TABLE_STATE_MAX = TABLE_STATE_GAME_END;
const int TABLE_STATE_ARRAYSIZE = TABLE_STATE_MAX + 1;

const ::google::protobuf::EnumDescriptor* TABLE_STATE_descriptor();
inline const ::std::string& TABLE_STATE_Name(TABLE_STATE value) {
  return ::google::protobuf::internal::NameOfEnum(
    TABLE_STATE_descriptor(), value);
}
inline bool TABLE_STATE_Parse(
    const ::std::string& name, TABLE_STATE* value) {
  return ::google::protobuf::internal::ParseNamedEnum<TABLE_STATE>(
    TABLE_STATE_descriptor(), name, value);
}
enum TABLE_STATE_NIUNIU {
  TABLE_STATE_NIUNIU_FREE = 1,
  TABLE_STATE_NIUNIU_PLACE_JETTON = 2,
  TABLE_STATE_NIUNIU_GAME_END = 3
};
bool TABLE_STATE_NIUNIU_IsValid(int value);
const TABLE_STATE_NIUNIU TABLE_STATE_NIUNIU_MIN = TABLE_STATE_NIUNIU_FREE;
const TABLE_STATE_NIUNIU TABLE_STATE_NIUNIU_MAX = TABLE_STATE_NIUNIU_GAME_END;
const int TABLE_STATE_NIUNIU_ARRAYSIZE = TABLE_STATE_NIUNIU_MAX + 1;

const ::google::protobuf::EnumDescriptor* TABLE_STATE_NIUNIU_descriptor();
inline const ::std::string& TABLE_STATE_NIUNIU_Name(TABLE_STATE_NIUNIU value) {
  return ::google::protobuf::internal::NameOfEnum(
    TABLE_STATE_NIUNIU_descriptor(), value);
}
inline bool TABLE_STATE_NIUNIU_Parse(
    const ::std::string& name, TABLE_STATE_NIUNIU* value) {
  return ::google::protobuf::internal::ParseNamedEnum<TABLE_STATE_NIUNIU>(
    TABLE_STATE_NIUNIU_descriptor(), name, value);
}
enum TABLE_STATE_DICE {
  TABLE_STATE_DICE_FREE = 1,
  TABLE_STATE_DICE_PLACE_JETTON = 2,
  TABLE_STATE_DICE_GAME_END = 3
};
bool TABLE_STATE_DICE_IsValid(int value);
const TABLE_STATE_DICE TABLE_STATE_DICE_MIN = TABLE_STATE_DICE_FREE;
const TABLE_STATE_DICE TABLE_STATE_DICE_MAX = TABLE_STATE_DICE_GAME_END;
const int TABLE_STATE_DICE_ARRAYSIZE = TABLE_STATE_DICE_MAX + 1;

const ::google::protobuf::EnumDescriptor* TABLE_STATE_DICE_descriptor();
inline const ::std::string& TABLE_STATE_DICE_Name(TABLE_STATE_DICE value) {
  return ::google::protobuf::internal::NameOfEnum(
    TABLE_STATE_DICE_descriptor(), value);
}
inline bool TABLE_STATE_DICE_Parse(
    const ::std::string& name, TABLE_STATE_DICE* value) {
  return ::google::protobuf::internal::ParseNamedEnum<TABLE_STATE_DICE>(
    TABLE_STATE_DICE_descriptor(), name, value);
}
enum TABLE_STATE_WAR {
  TABLE_STATE_WAR_FREE = 1,
  TABLE_STATE_WAR_PLACE_JETTON = 2,
  TABLE_STATE_WAR_GAME_END = 3
};
bool TABLE_STATE_WAR_IsValid(int value);
const TABLE_STATE_WAR TABLE_STATE_WAR_MIN = TABLE_STATE_WAR_FREE;
const TABLE_STATE_WAR TABLE_STATE_WAR_MAX = TABLE_STATE_WAR_GAME_END;
const int TABLE_STATE_WAR_ARRAYSIZE = TABLE_STATE_WAR_MAX + 1;

const ::google::protobuf::EnumDescriptor* TABLE_STATE_WAR_descriptor();
inline const ::std::string& TABLE_STATE_WAR_Name(TABLE_STATE_WAR value) {
  return ::google::protobuf::internal::NameOfEnum(
    TABLE_STATE_WAR_descriptor(), value);
}
inline bool TABLE_STATE_WAR_Parse(
    const ::std::string& name, TABLE_STATE_WAR* value) {
  return ::google::protobuf::internal::ParseNamedEnum<TABLE_STATE_WAR>(
    TABLE_STATE_WAR_descriptor(), name, value);
}
enum TABLE_STATE_FIGHT {
  TABLE_STATE_FIGHT_FREE = 1,
  TABLE_STATE_FIGHT_PLACE_JETTON = 2,
  TABLE_STATE_FIGHT_GAME_END = 3
};
bool TABLE_STATE_FIGHT_IsValid(int value);
const TABLE_STATE_FIGHT TABLE_STATE_FIGHT_MIN = TABLE_STATE_FIGHT_FREE;
const TABLE_STATE_FIGHT TABLE_STATE_FIGHT_MAX = TABLE_STATE_FIGHT_GAME_END;
const int TABLE_STATE_FIGHT_ARRAYSIZE = TABLE_STATE_FIGHT_MAX + 1;

const ::google::protobuf::EnumDescriptor* TABLE_STATE_FIGHT_descriptor();
inline const ::std::string& TABLE_STATE_FIGHT_Name(TABLE_STATE_FIGHT value) {
  return ::google::protobuf::internal::NameOfEnum(
    TABLE_STATE_FIGHT_descriptor(), value);
}
inline bool TABLE_STATE_FIGHT_Parse(
    const ::std::string& name, TABLE_STATE_FIGHT* value) {
  return ::google::protobuf::internal::ParseNamedEnum<TABLE_STATE_FIGHT>(
    TABLE_STATE_FIGHT_descriptor(), name, value);
}
enum TABLE_STATE_APBNIU {
  TABLE_STATE_APBNIU_FREE = 1,
  TABLE_STATE_APBNIU_READY_START = 2,
  TABLE_STATE_APBNIU_GAME_START = 3,
  TABLE_STATE_APBNIU_APPLY_BRANKER = 4,
  TABLE_STATE_APBNIU_MAKE_BRANKER = 5,
  TABLE_STATE_APBNIU_PLACE_JETTON = 6,
  TABLE_STATE_APBNIU_SEND_CARD = 7,
  TABLE_STATE_APBNIU_CHANGE_CARD = 8,
  TABLE_STATE_APBNIU_GAME_END = 9
};
bool TABLE_STATE_APBNIU_IsValid(int value);
const TABLE_STATE_APBNIU TABLE_STATE_APBNIU_MIN = TABLE_STATE_APBNIU_FREE;
const TABLE_STATE_APBNIU TABLE_STATE_APBNIU_MAX = TABLE_STATE_APBNIU_GAME_END;
const int TABLE_STATE_APBNIU_ARRAYSIZE = TABLE_STATE_APBNIU_MAX + 1;

const ::google::protobuf::EnumDescriptor* TABLE_STATE_APBNIU_descriptor();
inline const ::std::string& TABLE_STATE_APBNIU_Name(TABLE_STATE_APBNIU value) {
  return ::google::protobuf::internal::NameOfEnum(
    TABLE_STATE_APBNIU_descriptor(), value);
}
inline bool TABLE_STATE_APBNIU_Parse(
    const ::std::string& name, TABLE_STATE_APBNIU* value) {
  return ::google::protobuf::internal::ParseNamedEnum<TABLE_STATE_APBNIU>(
    TABLE_STATE_APBNIU_descriptor(), name, value);
}
enum TABLE_STATE_ZAJINHUA {
  TABLE_STATE_ZAJINHUA_FREE = 1,
  TABLE_STATE_ZAJINHUA_READY_START = 2,
  TABLE_STATE_ZAJINHUA_PLAY = 3,
  TABLE_STATE_ZAJINHUA_WAIT = 4,
  TABLE_STATE_ZAJINHUA_GAME_END = 5
};
bool TABLE_STATE_ZAJINHUA_IsValid(int value);
const TABLE_STATE_ZAJINHUA TABLE_STATE_ZAJINHUA_MIN = TABLE_STATE_ZAJINHUA_FREE;
const TABLE_STATE_ZAJINHUA TABLE_STATE_ZAJINHUA_MAX = TABLE_STATE_ZAJINHUA_GAME_END;
const int TABLE_STATE_ZAJINHUA_ARRAYSIZE = TABLE_STATE_ZAJINHUA_MAX + 1;

const ::google::protobuf::EnumDescriptor* TABLE_STATE_ZAJINHUA_descriptor();
inline const ::std::string& TABLE_STATE_ZAJINHUA_Name(TABLE_STATE_ZAJINHUA value) {
  return ::google::protobuf::internal::NameOfEnum(
    TABLE_STATE_ZAJINHUA_descriptor(), value);
}
inline bool TABLE_STATE_ZAJINHUA_Parse(
    const ::std::string& name, TABLE_STATE_ZAJINHUA* value) {
  return ::google::protobuf::internal::ParseNamedEnum<TABLE_STATE_ZAJINHUA>(
    TABLE_STATE_ZAJINHUA_descriptor(), name, value);
}
enum TABLE_STATE_FISHING {
  TABLE_STATE_FISHING_START = 1,
  TABLE_STATE_FISHING_PAUSE = 2
};
bool TABLE_STATE_FISHING_IsValid(int value);
const TABLE_STATE_FISHING TABLE_STATE_FISHING_MIN = TABLE_STATE_FISHING_START;
const TABLE_STATE_FISHING TABLE_STATE_FISHING_MAX = TABLE_STATE_FISHING_PAUSE;
const int TABLE_STATE_FISHING_ARRAYSIZE = TABLE_STATE_FISHING_MAX + 1;

const ::google::protobuf::EnumDescriptor* TABLE_STATE_FISHING_descriptor();
inline const ::std::string& TABLE_STATE_FISHING_Name(TABLE_STATE_FISHING value) {
  return ::google::protobuf::internal::NameOfEnum(
    TABLE_STATE_FISHING_descriptor(), value);
}
inline bool TABLE_STATE_FISHING_Parse(
    const ::std::string& name, TABLE_STATE_FISHING* value) {
  return ::google::protobuf::internal::ParseNamedEnum<TABLE_STATE_FISHING>(
    TABLE_STATE_FISHING_descriptor(), name, value);
}
enum TABLE_STATE_ROBNIU {
  TABLE_STATE_ROBNIU_FREE = 1,
  TABLE_STATE_ROBNIU_READY_START = 2,
  TABLE_STATE_ROBNIU_GAME_START = 3,
  TABLE_STATE_ROBNIU_FOUR_SHOW_CARD = 4,
  TABLE_STATE_ROBNIU_APPLY_BRANKER = 5,
  TABLE_STATE_ROBNIU_MAKE_BRANKER = 6,
  TABLE_STATE_ROBNIU_PLACE_JETTON = 7,
  TABLE_STATE_ROBNIU_LAST_SHOW_CARD = 8,
  TABLE_STATE_ROBNIU_CHANGE_CARD = 9,
  TABLE_STATE_ROBNIU_GAME_END = 10
};
bool TABLE_STATE_ROBNIU_IsValid(int value);
const TABLE_STATE_ROBNIU TABLE_STATE_ROBNIU_MIN = TABLE_STATE_ROBNIU_FREE;
const TABLE_STATE_ROBNIU TABLE_STATE_ROBNIU_MAX = TABLE_STATE_ROBNIU_GAME_END;
const int TABLE_STATE_ROBNIU_ARRAYSIZE = TABLE_STATE_ROBNIU_MAX + 1;

const ::google::protobuf::EnumDescriptor* TABLE_STATE_ROBNIU_descriptor();
inline const ::std::string& TABLE_STATE_ROBNIU_Name(TABLE_STATE_ROBNIU value) {
  return ::google::protobuf::internal::NameOfEnum(
    TABLE_STATE_ROBNIU_descriptor(), value);
}
inline bool TABLE_STATE_ROBNIU_Parse(
    const ::std::string& name, TABLE_STATE_ROBNIU* value) {
  return ::google::protobuf::internal::ParseNamedEnum<TABLE_STATE_ROBNIU>(
    TABLE_STATE_ROBNIU_descriptor(), name, value);
}
// ===================================================================


// ===================================================================


// ===================================================================


// @@protoc_insertion_point(namespace_scope)

}  // namespace net

#ifndef SWIG
namespace google {
namespace protobuf {

template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::RESULT_CODE>() {
  return ::net::RESULT_CODE_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::MISSION_TYPE>() {
  return ::net::MISSION_TYPE_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::MISSION_CYCLE_TYPE>() {
  return ::net::MISSION_CYCLE_TYPE_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::REWARD_FLAG>() {
  return ::net::REWARD_FLAG_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::GAME_CATE_TYPE>() {
  return ::net::GAME_CATE_TYPE_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::GAME_SUB_TYPE>() {
  return ::net::GAME_SUB_TYPE_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::EXCHANGE_TYPE>() {
  return ::net::EXCHANGE_TYPE_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::ROOM_CONSUME_TYPE>() {
  return ::net::ROOM_CONSUME_TYPE_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::ROOM_DEAL_TYPE>() {
  return ::net::ROOM_DEAL_TYPE_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::TABLE_FEE_TYPE>() {
  return ::net::TABLE_FEE_TYPE_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::TABLE_STATE>() {
  return ::net::TABLE_STATE_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::TABLE_STATE_NIUNIU>() {
  return ::net::TABLE_STATE_NIUNIU_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::TABLE_STATE_DICE>() {
  return ::net::TABLE_STATE_DICE_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::TABLE_STATE_WAR>() {
  return ::net::TABLE_STATE_WAR_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::TABLE_STATE_FIGHT>() {
  return ::net::TABLE_STATE_FIGHT_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::TABLE_STATE_APBNIU>() {
  return ::net::TABLE_STATE_APBNIU_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::TABLE_STATE_ZAJINHUA>() {
  return ::net::TABLE_STATE_ZAJINHUA_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::TABLE_STATE_FISHING>() {
  return ::net::TABLE_STATE_FISHING_descriptor();
}
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::TABLE_STATE_ROBNIU>() {
  return ::net::TABLE_STATE_ROBNIU_descriptor();
}

}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_error_5fcode_2eproto__INCLUDED
