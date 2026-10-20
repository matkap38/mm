#ifndef __INC_PACKET_H__
#define __INC_PACKET_H__
#include "stdafx.h"

enum
{
	HEADER_CG_HANDSHAKE							= 0xff,
	HEADER_CG_PONG								= 0xfe,
	HEADER_CG_TIME_SYNC							= 0xfc,
	HEADER_CG_KEY_AGREEMENT						= 0xfb, // _IMPROVED_PACKET_ENCRYPTION_

	HEADER_CG_LOGIN								= 1,
	HEADER_CG_ATTACK							= 2,
	HEADER_CG_CHAT								= 3,
	HEADER_CG_CHARACTER_CREATE					= 4,
	HEADER_CG_CHARACTER_DELETE					= 5,
	HEADER_CG_CHARACTER_SELECT					= 6,
	HEADER_CG_MOVE								= 7,
	HEADER_CG_SYNC_POSITION						= 8,
	HEADER_CG_ENTERGAME							= 10,

	HEADER_CG_ITEM_USE							= 11,
	#ifndef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
	HEADER_CG_ITEM_DROP							= 12,
	#endif

	HEADER_CG_ITEM_MOVE							= 13,
	HEADER_CG_ITEM_PICKUP						= 15,

	HEADER_CG_QUICKSLOT_ADD						= 16,
	HEADER_CG_QUICKSLOT_DEL						= 17,
	HEADER_CG_QUICKSLOT_SWAP					= 18,
	HEADER_CG_WHISPER							= 19,

	#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
	HEADER_CG_ITEM_DELETE						= 20,
	HEADER_CG_ITEM_SELL							= 21,
	#else
	HEADER_CG_ITEM_DROP2						= 20,
	#endif

	HEADER_CG_ON_CLICK							= 26,
	HEADER_CG_EXCHANGE							= 27,
	HEADER_CG_CHARACTER_POSITION				= 28,
	HEADER_CG_SCRIPT_ANSWER						= 29,
	HEADER_CG_QUEST_INPUT_STRING				= 30,
	HEADER_CG_QUEST_CONFIRM						= 31,

	HEADER_CG_SHOP								= 50,
	HEADER_CG_FLY_TARGETING						= 51,
	HEADER_CG_USE_SKILL							= 52,
	HEADER_CG_ADD_FLY_TARGETING					= 53,
	HEADER_CG_SHOOT								= 54,
	HEADER_CG_MYSHOP							= 55,

	#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
	HEADER_CG_TARGET_INFO_LOAD					= 59,
	#endif

	HEADER_CG_ITEM_USE_TO_ITEM					= 60,
	HEADER_CG_TARGET							= 61,

	HEADER_CG_TEXT								= 64,
	HEADER_CG_WARP								= 65,
	HEADER_CG_SCRIPT_BUTTON						= 66,
	HEADER_CG_MESSENGER							= 67,

	HEADER_CG_MALL_CHECKOUT						= 69,
	HEADER_CG_SAFEBOX_CHECKIN					= 70,
	HEADER_CG_SAFEBOX_CHECKOUT					= 71,

	HEADER_CG_PARTY_INVITE						= 72,
	HEADER_CG_PARTY_INVITE_ANSWER				= 73,
	HEADER_CG_PARTY_REMOVE						= 74,
	HEADER_CG_PARTY_SET_STATE					= 75,
	HEADER_CG_PARTY_USE_SKILL					= 76,
	HEADER_CG_SAFEBOX_ITEM_MOVE					= 77,
	HEADER_CG_PARTY_PARAMETER					= 78,

	HEADER_CG_GUILD								= 80,
	HEADER_CG_ANSWER_MAKE_GUILD					= 81,

	HEADER_CG_FISHING							= 82,

	HEADER_CG_ITEM_GIVE							= 83,

	HEADER_CG_EMPIRE							= 90,

	HEADER_CG_REFINE							= 96,

	HEADER_CG_MARK_LOGIN						= 100,
	HEADER_CG_MARK_CRCLIST						= 101,
	HEADER_CG_MARK_UPLOAD						= 102,
	HEADER_CG_MARK_IDXLIST						= 104,

	HEADER_CG_HACK								= 105,
	HEADER_CG_CHANGE_NAME						= 106,
	HEADER_CG_LOGIN2							= 109,
	HEADER_CG_DUNGEON							= 110,
	HEADER_CG_LOGIN3							= 111,

	HEADER_CG_GUILD_SYMBOL_UPLOAD				= 112,
	HEADER_CG_SYMBOL_CRC						= 113,

	// SCRIPT_SELECT_ITEM
	HEADER_CG_SCRIPT_SELECT_ITEM				= 114,
	// END_OF_SCRIPT_SELECT_ITEM

	#ifdef ENABLE_IKASHOP_RENEWAL
	HEADER_CG_NEW_OFFLINESHOP					= 119,
	#endif

	#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
	HEADER_CG_ITEMSHOP_PROCESS					= 160,
	#endif

	#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
	HEADER_CG_ITEMSHOP_ADMIN_PROCESS			= 161,
	#endif

	#ifdef ENABLE_BATTLE_PASS
	HEADER_CG_BATTLE_PASS						= 162,
	#endif

	#ifdef ENABLE_CHANGELOOK_SYSTEM
	HEADER_CG_CL								= 163,
	#endif

	HEADER_CG_DRAGON_SOUL_REFINE				= 205,
	HEADER_CG_STATE_CHECKER						= 206,

	#ifdef ENABLE_SHOW_CHEST_DROP
	HEADER_CG_CHEST_DROP_INFO					= 207,
	#endif

	#ifdef ENABLE_SWITCHBOT
	HEADER_CG_SWITCHBOT							= 208,
	#endif

	#ifdef ENABLE_FAST_OPEN_CHEST
	HEADER_CG_INSTANT_OPEN						= 209,
	#endif

	#ifdef ENABLE_STYLE_ATTRIBUTE_SYSTEM
	HEADER_CG_ITEM_USE_NEW_ATTRIBUTE			= 210,
	#endif

	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	HEADER_CG_ACCE								= 211,
	#endif

	#ifdef ENABLE_HIDE_COSTUME_PART
	HEADER_CG_CHANGE_COSTUME_VISIBILITY_STATUS	= 212,
	#endif

	#ifdef ENABLE_CUBE_RENEWAL
	HEADER_CG_CUBE_RENEWAL						= 213,
	#endif

	#ifdef ENABLE_PM_ALL_SEND_SYSTEM
	HEADER_CG_BULK_WHISPER						= 216,
	#endif

	#ifdef ENABLE_OFFER_SYSTEM
	HEADER_CG_OFFER_REQUEST						= 217,
	#endif

	#ifdef ENABLE_BLACK_MARKET_SYSTEM
	HEADER_CG_BLACKMARKET						= 218,
	#endif
	#if defined(__BL_LUCKY_BOX__)
	HEADER_CG_LUCKY_BOX							= 220,
	#endif

	#ifdef ENABLE_AUTO_ITEM_SELL_SYSTEM
	HEADER_CG_AUTO_ITEM_SELL					= 221,
	#endif

	HEADER_CG_CLIENT_VERSION					= 0xfd,
	HEADER_CG_CLIENT_VERSION2					= 0xf1,

	/********************************************************/
	HEADER_GC_KEY_AGREEMENT_COMPLETED			= 0xfa, // _IMPROVED_PACKET_ENCRYPTION_
	HEADER_GC_KEY_AGREEMENT						= 0xfb, // _IMPROVED_PACKET_ENCRYPTION_
	HEADER_GC_TIME_SYNC							= 0xfc,
	HEADER_GC_PHASE								= 0xfd,
	HEADER_GC_BINDUDP							= 0xfe,
	HEADER_GC_HANDSHAKE							= 0xff,

	HEADER_GC_CHARACTER_ADD						= 1,
	HEADER_GC_CHARACTER_DEL						= 2,
	HEADER_GC_MOVE								= 3,
	HEADER_GC_CHAT								= 4,
	HEADER_GC_SYNC_POSITION						= 5,

	HEADER_GC_LOGIN_SUCCESS						= 6,
	HEADER_GC_LOGIN_SUCCESS_NEWSLOT				= 32,
	HEADER_GC_LOGIN_FAILURE						= 7,

	HEADER_GC_CHARACTER_CREATE_SUCCESS			= 8,
	HEADER_GC_CHARACTER_CREATE_FAILURE			= 9,
	HEADER_GC_CHARACTER_DELETE_SUCCESS			= 10,
	HEADER_GC_CHARACTER_DELETE_WRONG_SOCIAL_ID	= 11,

	HEADER_GC_ATTACK							= 12,
	HEADER_GC_STUN								= 13,
	HEADER_GC_DEAD								= 14,

	HEADER_GC_MAIN_CHARACTER_OLD				= 15,
	HEADER_GC_CHARACTER_POINTS					= 16,
	HEADER_GC_CHARACTER_POINT_CHANGE			= 17,
	HEADER_GC_CHANGE_SPEED						= 18,
	HEADER_GC_CHARACTER_UPDATE					= 19,

	HEADER_GC_ITEM_DEL							= 20,
	HEADER_GC_ITEM_SET							= 21,
	HEADER_GC_ITEM_USE							= 22,
	HEADER_GC_ITEM_DROP							= 23,
	HEADER_GC_ITEM_UPDATE						= 25,

	HEADER_GC_ITEM_GROUND_ADD					= 26,
	HEADER_GC_ITEM_GROUND_DEL					= 27,

	HEADER_GC_QUICKSLOT_ADD						= 28,
	HEADER_GC_QUICKSLOT_DEL						= 29,
	HEADER_GC_QUICKSLOT_SWAP					= 30,

	HEADER_GC_ITEM_OWNERSHIP					= 31,

	HEADER_GC_WHISPER							= 34,

	HEADER_GC_MOTION							= 36,
	HEADER_GC_PARTS								= 37,

	HEADER_GC_SHOP								= 38,
	HEADER_GC_SHOP_SIGN							= 39,

	HEADER_GC_DUEL_START						= 40,
	HEADER_GC_PVP								= 41,
	HEADER_GC_EXCHANGE							= 42,
	HEADER_GC_CHARACTER_POSITION				= 43,

	HEADER_GC_PING								= 44,
	HEADER_GC_SCRIPT							= 45,
	HEADER_GC_QUEST_CONFIRM						= 46,

	HEADER_GC_AFFECT_RESET						= 57,

	#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
	HEADER_GC_TARGET_INFO						= 58,
	#endif

	HEADER_GC_MOUNT								= 61,
	HEADER_GC_OWNERSHIP							= 62,
	HEADER_GC_TARGET							= 63,

	HEADER_GC_WARP								= 65,

	HEADER_GC_ADD_FLY_TARGETING					= 69,
	HEADER_GC_CREATE_FLY						= 70,
	HEADER_GC_FLY_TARGETING						= 71,
	HEADER_GC_SKILL_LEVEL_OLD					= 72,
	HEADER_GC_SKILL_LEVEL						= 76,

	HEADER_GC_MESSENGER							= 74,
	HEADER_GC_GUILD								= 75,

	HEADER_GC_PARTY_INVITE						= 77,
	HEADER_GC_PARTY_ADD							= 78,
	HEADER_GC_PARTY_UPDATE						= 79,
	HEADER_GC_PARTY_REMOVE						= 80,
	HEADER_GC_QUEST_INFO						= 81,
	HEADER_GC_REQUEST_MAKE_GUILD				= 82,
	HEADER_GC_PARTY_PARAMETER					= 83,

	HEADER_GC_SAFEBOX_SET						= 85,
	HEADER_GC_SAFEBOX_DEL						= 86,
	HEADER_GC_SAFEBOX_WRONG_PASSWORD			= 87,
	HEADER_GC_SAFEBOX_SIZE						= 88,

	HEADER_GC_FISHING							= 89,

	HEADER_GC_EMPIRE							= 90,

	HEADER_GC_PARTY_LINK						= 91,
	HEADER_GC_PARTY_UNLINK						= 92,

	HEADER_GC_REFINE_INFORMATION_OLD			= 95,

	HEADER_GC_VIEW_EQUIP						= 99,

	HEADER_GC_MARK_BLOCK						= 100,
	HEADER_GC_MARK_IDXLIST						= 102,

	HEADER_GC_TIME								= 106,
	HEADER_GC_CHANGE_NAME						= 107,

	HEADER_GC_DUNGEON							= 110,

	HEADER_GC_WALK_MODE							= 111,
	HEADER_GC_SKILL_GROUP						= 112,
	HEADER_GC_MAIN_CHARACTER					= 113,

	HEADER_GC_SEPCIAL_EFFECT					= 114,

	HEADER_GC_NPC_POSITION						= 115,

	HEADER_GC_LOGIN_KEY							= 118,
	HEADER_GC_REFINE_INFORMATION				= 119,
	HEADER_GC_CHANNEL							= 121,

	HEADER_GC_TARGET_UPDATE						= 123,
	HEADER_GC_TARGET_DELETE						= 124,
	HEADER_GC_TARGET_CREATE						= 125,

	HEADER_GC_AFFECT_ADD						= 126,
	HEADER_GC_AFFECT_REMOVE						= 127,

	HEADER_GC_MALL_OPEN							= 122,
	HEADER_GC_MALL_SET							= 128,
	HEADER_GC_MALL_DEL							= 129,

	HEADER_GC_LAND_LIST							= 130,
	HEADER_GC_LOVER_INFO						= 131,
	HEADER_GC_LOVE_POINT_UPDATE					= 132,

	HEADER_GC_SYMBOL_DATA						= 133,

	// MINING
	HEADER_GC_DIG_MOTION						= 134,
	// END_OF_MINING

	HEADER_GC_DAMAGE_INFO						= 135,
	HEADER_GC_CHAR_ADDITIONAL_INFO				= 136,

	// SUPPORT_BGM
	HEADER_GC_MAIN_CHARACTER3_BGM				= 137,
	HEADER_GC_MAIN_CHARACTER4_BGM_VOL			= 138,
	// END_OF_SUPPORT_BGM

	#ifdef ENABLE_BATTLE_PASS
	HEADER_GC_BATTLE_PASS_OPEN					= 148,
	HEADER_GC_BATTLE_PASS_UPDATE				= 149,
	#endif

	HEADER_GC_AUTH_SUCCESS						= 150,

	HEADER_GC_PANAMA_PACK						= 151,

	//HYBRID CRYPT
	HEADER_GC_HYBRIDCRYPT_KEYS					= 152,
	HEADER_GC_HYBRIDCRYPT_SDB					= 153, // SDB means Supplmentary Data Blocks
	//HYBRID CRYPT

	#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
	HEADER_GC_ITEMSHOP_GENERAL					= 160,
	#endif

	#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
	HEADER_GC_ITEMSHOP_ADMIN_GENERAL			= 161,
	#endif

	#ifdef ENABLE_TANAKA_EVENT
	HEADER_GC_TANAKA_EVENT						= 162,
	#endif

	#ifdef ENABLE_CHANGELOOK_SYSTEM
	HEADER_GC_CL								= 163,
	#endif

	#ifdef ENABLE_REWARD_SYSTEM
	HEADER_GC_REWARD_INFO						= 164,
	#endif

	#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
	HEADER_GC_GUILD_STATISTICS					= 165,
	#endif

	#ifdef ENABLE_AUTO_ITEM_SELL_SYSTEM
	HEADER_GC_AUTO_ITEM_SELL					= 166,
	#endif

	// ROULETTE
	HEADER_GC_ROULETTE							= 200,
	// END_ROULETTE

	HEADER_GC_SPECIFIC_EFFECT					= 208,

	HEADER_GC_DRAGON_SOUL_REFINE				= 209,
	HEADER_GC_RESPOND_CHANNELSTATUS				= 210,

	#ifdef ENABLE_SHOW_CHEST_DROP
	HEADER_GC_CHEST_DROP_INFO					= 211,
	#endif

	#ifdef ENABLE_SWITCHBOT
	HEADER_GC_SWITCHBOT							= 212,
	#endif

	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	HEADER_GC_ACCE								= 213,
	#endif

	#ifdef ENABLE_HIDE_COSTUME_PART
	HEADER_GC_UPDATE_COSTUME_VISIBILITY_STATUS	= 214,
	#endif

	#ifdef ENABLE_FULL_YANG
	HEADER_GC_GOLD								= 215,
	HEADER_GC_GOLD_CHANGE						= 216,
	#endif

	#ifdef ENABLE_CUBE_RENEWAL
	HEADER_GC_CUBE_RENEWAL						= 217,
	#endif

	#ifdef ENABLE_WHEEL_OF_FORTUNE_SYSTEM
	HEADER_GC_WHEEL_OF_FORTUNE					= 218,
	#endif

	#ifdef __RANKING_SYSTEM__
	HEADER_GC_RANKING_UPDATE					= 219,
	#endif

	#ifdef ENABLE_TELEPORTION_WIZARD_SYSTEM
	HEADER_GC_TELEPORTION_WIZARD_INFO			= 220,
	#endif

	#ifdef ENABLE_EVENT_MANAGER
	HEADER_GC_EVENT_MANAGER						= 221,
	#endif

	#ifdef ENABLE_SKILL_RENEWAL
	HEADER_GC_SKILL_BOOK_COUNT					= 222,
	#endif

	#ifdef ENABLE_PM_ALL_SEND_SYSTEM
	HEADER_GC_BULK_WHISPER						= 225,
	#endif

	#ifdef ENABLE_BAKIM_DUYURU
	HEADER_GC_ENABLE_BAKIM_DUYURU				= 226,
	#endif

	#ifdef BOSS_DMG_RANKING
	HEADER_GC_BOSS_RANKING						= 227,
	#endif

	#ifdef ENABLE_OFFER_SYSTEM
	HEADER_GC_OFFER								= 228,
	#endif

	#ifdef ENABLE_EXCHANGE_LOG
	HEADER_GC_EXCHANGE_LOG						= 229,
	#endif

	#ifdef ENABLE_ENTITY_PRELOADING
	HEADER_GC_PRELOAD_ENTITIES					= 230,
	#endif

	#ifdef ENABLE_IKASHOP_RENEWAL
	HEADER_GC_NEW_OFFLINESHOP					= 231,
	#endif

	#ifdef ENABLE_BLACK_MARKET_SYSTEM
	HEADER_GC_BLACKMARKET						= 232,
	#endif

	#if defined(__BL_LUCKY_BOX__)
	HEADER_GC_LUCKY_BOX							= 234,
	#endif

	#ifdef ENABLE_PUBLISHER_SYSTEM
	HEADER_GC_PUBLISHER_INFO					= 235,
	#endif

	#ifdef ENABLE_RONARK_SYSTEM
	HEADER_GC_RONARK_INFO						= 236,
	#endif
#ifdef SPIN_WHEEL
	HEADER_GC_SPIN_WHEEL = 237,
	HEADER_GC_SPINWHEEL_REWARD = 238,
#endif
	/////////////////////////////////////////////////////////////////////////////

	HEADER_GG_LOGIN								= 1,
	HEADER_GG_LOGOUT							= 2,
	HEADER_GG_RELAY								= 3,
	HEADER_GG_NOTICE							= 4,
	HEADER_GG_SHUTDOWN							= 5,
	HEADER_GG_GUILD								= 6,
	HEADER_GG_DISCONNECT						= 7,
	HEADER_GG_SHOUT								= 8,
	HEADER_GG_SETUP								= 9,
	HEADER_GG_MESSENGER_ADD						= 10,
	HEADER_GG_MESSENGER_REMOVE					= 11,
	HEADER_GG_FIND_POSITION						= 12,
	HEADER_GG_WARP_CHARACTER					= 13,
	HEADER_GG_MESSENGER_MOBILE					= 14,
	HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX			= 15,
	HEADER_GG_TRANSFER							= 16,
	HEADER_GG_XMAS_WARP_SANTA					= 17,
	HEADER_GG_XMAS_WARP_SANTA_REPLY				= 18,
	HEADER_GG_RELOAD_CRC_LIST					= 19,
	HEADER_GG_LOGIN_PING						= 20,
	HEADER_GG_CHECK_CLIENT_VERSION				= 21,
	HEADER_GG_BLOCK_CHAT						= 22,

	HEADER_GG_BLOCK_EXCEPTION					= 24,
	HEADER_GG_SIEGE								= 25,
	HEADER_GG_MONARCH_NOTICE					= 26,
	HEADER_GG_MONARCH_TRANSFER					= 27,
	HEADER_GG_DISCONNECT_PLAYER					= 28,
	HEADER_GG_CHECK_AWAKENESS					= 29,

	#ifdef ENABLE_FULL_NOTICE
	HEADER_GG_BIG_NOTICE						= 30,
	#endif

	#ifdef ENABLE_SWITCHBOT
	HEADER_GG_SWITCHBOT							= 31,
	#endif

	#ifdef ENABLE_MESSENGER_BLOCK
	HEADER_GG_MESSENGER_BLOCK_ADD				= 32,
	HEADER_GG_MESSENGER_BLOCK_REMOVE			= 33,
	#endif

	#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
	HEADER_GG_DC_SHOP							= 34,
	HEADER_GG_DC_SHOP_BUY						= 35,
	#endif

	#ifdef ENABLE_MULTI_FARM_BLOCK
	HEADER_GG_MULTI_FARM						= 36,
	#endif

	#ifdef ENABLE_MULTI_HUNT_BLOCK
	HEADER_GG_MULTI_HUNT						= 37,
	#endif

	#ifdef ENABLE_MULTI_FISH_BLOCK
	HEADER_GG_MULTI_FISH						= 38,
	#endif

	#ifdef ENABLE_PM_ALL_SEND_SYSTEM
	HEADER_GG_BULK_WHISPER						= 39,
	#endif

	#ifdef ENABLE_BAKIM_DUYURU
	HEADER_GG_BAKIM								= 40,
	#endif

	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	HEADER_GG_GUILD_WAR_OPTION					= 41,
	HEADER_GG_GUILD_WAR_MEMBER					= 42,
	#endif

	#ifdef ENABLE_TANAKA_EVENT
	HEADER_GG_TANAKA_EVENT						= 43,
	#endif

	#ifdef __BACK_DUNGEON__
	HEADER_GG_BACK_DUNGEON						= 44,
	#endif

	// HEADER_GG								= 45,

	#ifdef ENABLE_DISCONNECT_ALL_PLAYERS
	HEADER_GG_DISCONNECT_ALL					= 46,
	#endif

	#ifdef ENABLE_BLACK_MARKET_SYSTEM
	HEADER_GG_BLACKMARKET						= 47,
	#endif

	#ifdef ENABLE_COLLECTIVE_RELOAD
	HEADER_GG_COLLECTIVE_RELOAD					= 48,
	#endif

	#ifdef ENABLE_DC_TO_CHR_OTHER_CH
	HEADER_GG_DC_TO_CHR							= 49,
	#endif

	#ifdef ENABLE_REWARD_SYSTEM
	HEADER_GG_REWARD_INFO						= 50,
	#endif

	#ifdef ENABLE_RONARK_SYSTEM
	HEADER_GG_RONARK							= 51,
	#endif
};

#pragma pack(1)
typedef struct SPacketGGSetup
{
	BYTE	bHeader;
	WORD	wPort;
	BYTE	bChannel;
} TPacketGGSetup;

typedef struct SPacketGGLogin
{
	BYTE	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	DWORD	dwPID;
	BYTE	bEmpire;
	long	lMapIndex;
	BYTE	bChannel;
} TPacketGGLogin;

typedef struct SPacketGGLogout
{
	BYTE	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGGLogout;

typedef struct SPacketGGRelay
{
	BYTE	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	long	lSize;
} TPacketGGRelay;

typedef struct SPacketGGNotice
{
	BYTE	bHeader;
	long	lSize;
} TPacketGGNotice;

typedef struct SPacketGGMonarchNotice
{
	BYTE	bHeader;
	BYTE	bEmpire;
	long	lSize;
} TPacketGGMonarchNotice;

//FORKED_ROAD
typedef struct SPacketGGForkedMapInfo
{
	BYTE	bHeader;
	BYTE	bPass;
	BYTE	bSungzi;
} TPacketGGForkedMapInfo;
//END_FORKED_ROAD

typedef struct SPacketGGShutdown
{
	BYTE	bHeader;
} TPacketGGShutdown;

typedef struct SPacketGGGuild
{
	BYTE	bHeader;
	BYTE	bSubHeader;
	DWORD	dwGuild;
} TPacketGGGuild;

enum
{
	GUILD_SUBHEADER_GG_CHAT,
	GUILD_SUBHEADER_GG_SET_MEMBER_COUNT_BONUS,
};

typedef struct SPacketGGGuildChat
{
	BYTE	bHeader;
	BYTE	bSubHeader;
	DWORD	dwGuild;
	char	szText[CHAT_MAX_LEN + 1];
} TPacketGGGuildChat;

typedef struct SPacketGGParty
{
	BYTE	header;
	BYTE	subheader;
	DWORD	pid;
	DWORD	leaderpid;
} TPacketGGParty;

enum
{
	PARTY_SUBHEADER_GG_CREATE,
	PARTY_SUBHEADER_GG_DESTROY,
	PARTY_SUBHEADER_GG_JOIN,
	PARTY_SUBHEADER_GG_QUIT,
};

typedef struct SPacketGGDisconnect
{
	BYTE	bHeader;
	char	szLogin[LOGIN_MAX_LEN + 1];
} TPacketGGDisconnect;

typedef struct SPacketGGShout
{
	BYTE	bHeader;
	BYTE	bEmpire;
	char	szText[CHAT_MAX_LEN + 1];
} TPacketGGShout;

typedef struct SPacketGGXmasWarpSanta
{
	BYTE	bHeader;
	BYTE	bChannel;
	long	lMapIndex;
} TPacketGGXmasWarpSanta;

typedef struct SPacketGGXmasWarpSantaReply
{
	BYTE	bHeader;
	BYTE	bChannel;
} TPacketGGXmasWarpSantaReply;

typedef struct SMessengerData
{
	char		szMobile[MOBILE_MAX_LEN + 1];
} TMessengerData;

typedef struct SPacketGGMessenger
{
	BYTE		bHeader;
	char		szAccount[CHARACTER_NAME_MAX_LEN + 1];
	char		szCompanion[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGGMessenger;

typedef struct SPacketGGMessengerMobile
{
	BYTE		bHeader;
	char		szName[CHARACTER_NAME_MAX_LEN + 1];
	char		szMobile[MOBILE_MAX_LEN + 1];
} TPacketGGMessengerMobile;

typedef struct SPacketGGFindPosition
{
	SPacketGGFindPosition() : header(HEADER_GG_FIND_POSITION)
	#ifdef ENABLE_WARP_REGULATION
	, IsGmWarp(false)
	#endif
	{}
	BYTE header;
	DWORD dwFromPID;
	DWORD dwTargetPID;
	#ifdef ENABLE_WARP_REGULATION
	bool IsGmWarp;
	#endif
} TPacketGGFindPosition;

typedef struct SPacketGGWarpCharacter
{
	SPacketGGWarpCharacter() : header(HEADER_GG_WARP_CHARACTER)
	#ifdef ENABLE_CMD_WARP_IN_DUNGEON
	, mapIndex(0)
	#endif
	#ifdef ENABLE_WARP_REGULATION
	, port(0)
	#endif
	{}

	BYTE header;
	DWORD pid;
	long x;
	long y;
	#ifdef ENABLE_CMD_WARP_IN_DUNGEON
	int mapIndex;
	#endif
	#ifdef ENABLE_WARP_REGULATION
	WORD port;
	#endif
} TPacketGGWarpCharacter;

//  HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX	    = 15,

typedef struct SPacketGGGuildWarMapIndex
{
	BYTE bHeader;
	DWORD dwGuildID1;
	DWORD dwGuildID2;
	long lMapIndex;
} TPacketGGGuildWarMapIndex;

typedef struct SPacketGGTransfer
{
	SPacketGGTransfer() : bHeader(HEADER_GG_TRANSFER)
	#ifdef ENABLE_CMD_WARP_IN_DUNGEON
	, lMapIndex(0)
	#endif
	#ifdef ENABLE_WARP_REGULATION
	, wPort(0)
	#endif
	{}

	BYTE	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	long	lX, lY;
	#ifdef ENABLE_CMD_WARP_IN_DUNGEON
	int		lMapIndex;
	#endif
	#ifdef ENABLE_WARP_REGULATION
	WORD	wPort;
	#endif
} TPacketGGTransfer;

typedef struct SPacketGGLoginPing
{
	BYTE	bHeader;
	char	szLogin[LOGIN_MAX_LEN + 1];
} TPacketGGLoginPing;

typedef struct SPacketGGBlockChat
{
	BYTE	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	long	lBlockDuration;
} TPacketGGBlockChat;



typedef struct command_text
{
	BYTE	bHeader;
} TPacketCGText;


typedef struct command_handshake
{
	BYTE	bHeader;
	DWORD	dwHandshake;
	DWORD	dwTime;
	long	lDelta;
} TPacketCGHandshake;

typedef struct command_login
{
	BYTE	header;
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
} TPacketCGLogin;

typedef struct command_login2
{
	BYTE	header;
	char	login[LOGIN_MAX_LEN + 1];
	DWORD	dwLoginKey;
	DWORD	adwClientKey[4];
} TPacketCGLogin2;

typedef struct command_login3
{
	BYTE	header;
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
	#ifdef ENABLE_SERVER_PIN
	char	pin[PIN_MAX_NUM + 1];
	#endif
	#ifdef ENABLE_PCBAN_SYSTEM
	int		pcban;
	#endif
	#ifdef ENABLE_VERSION_SYSTEM
	int		version;
	#endif
	#ifdef RENEWAL_HWID_BLOCK
	char	hwid_router[18+1];
	#endif
	#ifdef RENEWAL_DISCORD_BLOCK
	char	discord_nick[128 + 1];
	char	discord_id[128 + 1];
	#endif
	DWORD	adwClientKey[4];
} TPacketCGLogin3;

typedef struct packet_login_key
{
	BYTE	bHeader;
	DWORD	dwLoginKey;
} TPacketGCLoginKey;

typedef struct command_player_select
{
	BYTE	header;
	BYTE	index;
} TPacketCGPlayerSelect;

typedef struct command_player_delete
{
	BYTE	header;
	BYTE	index;
	// char	private_code[8];
	char	private_code[SOCIAL_ID_MAX_LEN + 1];
} TPacketCGPlayerDelete;

typedef struct command_player_create
{
	BYTE	header;
	BYTE	index;
	char	name[CHARACTER_NAME_MAX_LEN + 1];
	WORD	job;
	BYTE	shape;
	BYTE	Con;
	BYTE	Int;
	BYTE	Str;
	BYTE	Dex;
} TPacketCGPlayerCreate;

typedef struct command_player_create_success
{
	BYTE		header;
	BYTE		bAccountCharacterIndex;
	TSimplePlayer	player;
} TPacketGCPlayerCreateSuccess;

typedef struct command_attack
{
	BYTE	bHeader;
	BYTE	bType;
	DWORD	dwVID;
	BYTE	bCRCMagicCubeProcPiece;
	BYTE	bCRCMagicCubeFilePiece;

} TPacketCGAttack;

enum EMoveFuncType
{
	FUNC_WAIT,
	FUNC_MOVE,
	FUNC_ATTACK,
	FUNC_COMBO,
	FUNC_MOB_SKILL,
	_FUNC_SKILL,
	FUNC_MAX_NUM,
	FUNC_SKILL = 0x80,
};


typedef struct command_move
{
	BYTE	bHeader;
	BYTE	bFunc;
	BYTE	bArg;
	BYTE	bRot;
	long	lX;
	long	lY;
	DWORD	dwTime;
} TPacketCGMove;

typedef struct command_sync_position_element
{
	DWORD	dwVID;
	long	lX;
	long	lY;
} TPacketCGSyncPositionElement;

typedef struct command_sync_position
{
	BYTE	bHeader;
	WORD	wSize;
} TPacketCGSyncPosition;

typedef struct command_chat
{
	BYTE	header;
	WORD	size;
	BYTE	type;
} TPacketCGChat;

typedef struct command_whisper
{
	BYTE	bHeader;
	WORD	wSize;
	char 	szNameTo[CHARACTER_NAME_MAX_LEN + 1];
} TPacketCGWhisper;

typedef struct command_entergame
{
	BYTE	header;
} TPacketCGEnterGame;

typedef struct command_item_use
{
	BYTE 	header;
	TItemPos 	Cell;
} TPacketCGItemUse;

typedef struct command_item_use_to_item
{
	BYTE	header;
	TItemPos	Cell;
	TItemPos	TargetCell;
} TPacketCGItemUseToItem;

#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
typedef struct command_item_delete
{
	BYTE		header;
	TItemPos	Cell;
} TPacketCGItemDelete;

typedef struct command_item_sell
{
	BYTE		header;
	TItemPos	Cell;
	bool		IsAuto;
} TPacketCGItemSell;
#else
typedef struct command_item_drop
{
	BYTE 	header;
	TItemPos 	Cell;
	DWORD	gold;
} TPacketCGItemDrop;

typedef struct command_item_drop2
{
	BYTE		header;
	TItemPos	Cell;
	DWORD		gold;
	#ifdef ENABLE_NEW_MAX_COUNT
	short		count;
	#else
	BYTE		count;
	#endif
} TPacketCGItemDrop2;
#endif

typedef struct command_item_move
{
	BYTE 	header;
	TItemPos	Cell;
	TItemPos	CellTo;
#ifdef ENABLE_NEW_MAX_COUNT
	short	count;
#else
	BYTE	count;
#endif
} TPacketCGItemMove;

typedef struct command_item_pickup
{
	BYTE 	header;
	DWORD	vid;
} TPacketCGItemPickup;

typedef struct command_quickslot_add
{
	BYTE	header;
	BYTE	pos;
	TQuickslot	slot;
} TPacketCGQuickslotAdd;

typedef struct command_quickslot_del
{
	BYTE	header;
	BYTE	pos;
} TPacketCGQuickslotDel;

typedef struct command_quickslot_swap
{
	BYTE	header;
	BYTE	pos;
	BYTE	change_pos;
} TPacketCGQuickslotSwap;

enum
{
	SHOP_SUBHEADER_CG_END,
	SHOP_SUBHEADER_CG_BUY,
	SHOP_SUBHEADER_CG_SELL,
	SHOP_SUBHEADER_CG_SELL2
};

typedef struct command_shop_buy
{
	BYTE	count;
} TPacketCGShopBuy;

typedef struct shop_item_detail
{
	TItemPos	itemPos;
#ifdef ENABLE_NEW_MAX_COUNT
	short		byCount;
#else
	BYTE		byCount;
#endif
} TPacketGCShopDetail;

typedef struct command_shop_sell
{
	BYTE	pos;
#ifdef ENABLE_NEW_MAX_COUNT
	short	byCount;
#else
	BYTE	byCount;
#endif
} TPacketCGShopSell;

typedef struct command_shop
{
	BYTE	header;
#ifdef ENABLE_NEW_MAX_COUNT
	short	subheader;
#else
	BYTE	subheader;
#endif
} TPacketCGShop;

typedef struct command_on_click
{
	BYTE	header;
	DWORD	vid;
} TPacketCGOnClick;

enum
{
	EXCHANGE_SUBHEADER_CG_START,	/* arg1 == vid of target character */
	EXCHANGE_SUBHEADER_CG_ITEM_ADD,	/* arg1 == position of item */
	EXCHANGE_SUBHEADER_CG_ITEM_DEL,	/* arg1 == position of item */
	EXCHANGE_SUBHEADER_CG_ELK_ADD,	/* arg1 == amount of gold */
	EXCHANGE_SUBHEADER_CG_ACCEPT,	/* arg1 == not used */
	EXCHANGE_SUBHEADER_CG_CANCEL,	/* arg1 == not used */
};

typedef struct command_exchange
{
	BYTE	header;
	BYTE	sub_header;
#ifdef ENABLE_FULL_YANG
	uint64_t	arg1;
#else
	DWORD	arg1;
#endif
	BYTE	arg2;
	TItemPos	Pos;
} TPacketCGExchange;

typedef struct command_position
{
	BYTE	header;
	BYTE	position;
} TPacketCGPosition;

typedef struct command_script_answer
{
	BYTE	header;
	BYTE	answer;
	//char	file[32 + 1];
	//BYTE	answer[16 + 1];
} TPacketCGScriptAnswer;


typedef struct command_script_button
{
	BYTE        header;
	unsigned int	idx;
} TPacketCGScriptButton;

typedef struct command_quest_input_string
{
	BYTE header;
	char msg[64+1];
} TPacketCGQuestInputString;

typedef struct command_quest_confirm
{
	BYTE header;
	BYTE answer;
	DWORD requestPID;
} TPacketCGQuestConfirm;


typedef struct packet_quest_confirm
{
	BYTE header;
	char msg[64+1];
	long timeout;
	DWORD requestPID;
} TPacketGCQuestConfirm;

typedef struct packet_handshake
{
	BYTE	bHeader;
	DWORD	dwHandshake;
	DWORD	dwTime;
	long	lDelta;
} TPacketGCHandshake;

enum EPhase
{
	PHASE_CLOSE,
	PHASE_HANDSHAKE,
	PHASE_LOGIN,
	PHASE_SELECT,
	PHASE_LOADING,
	PHASE_GAME,
	PHASE_DEAD,

	PHASE_CLIENT_CONNECTING,
	PHASE_DBCLIENT,
	PHASE_P2P,
	PHASE_AUTH,
};

typedef struct packet_phase
{
	BYTE	header;
	BYTE	phase;
} TPacketGCPhase;

typedef struct packet_bindudp
{
	BYTE	header;
	DWORD	addr;
	WORD	port;
} TPacketGCBindUDP;

enum
{
	LOGIN_FAILURE_ALREADY		= 1,
	LOGIN_FAILURE_ID_NOT_EXIST	= 2,
	LOGIN_FAILURE_WRONG_PASS	= 3,
	LOGIN_FAILURE_FALSE			= 4,
	LOGIN_FAILURE_NOT_TESTOR	= 5,
	LOGIN_FAILURE_NOT_TEST_TIME	= 6,
	LOGIN_FAILURE_FULL			= 7
};

typedef struct packet_login_success
{
	BYTE			bHeader;
	TSimplePlayer	players[PLAYER_PER_ACCOUNT];
	DWORD			guild_id[PLAYER_PER_ACCOUNT];
	char			guild_name[PLAYER_PER_ACCOUNT][GUILD_NAME_MAX_LEN+1];
	DWORD			handle;
	DWORD			random_key;
} TPacketGCLoginSuccess;

typedef struct packet_auth_success
{
	BYTE	bHeader;
	DWORD	dwLoginKey;
	BYTE	bResult;
} TPacketGCAuthSuccess;

typedef struct packet_login_failure
{
	BYTE	header;
	char	szStatus[ACCOUNT_STATUS_MAX_LEN + 1];
} TPacketGCLoginFailure;

typedef struct packet_create_failure
{
	BYTE	header;
	BYTE	bType;
} TPacketGCCreateFailure;

enum
{
	ADD_CHARACTER_STATE_DEAD		= (1 << 0),
	ADD_CHARACTER_STATE_SPAWN		= (1 << 1),
	ADD_CHARACTER_STATE_GUNGON		= (1 << 2),
	ADD_CHARACTER_STATE_KILLER		= (1 << 3),
	ADD_CHARACTER_STATE_PARTY		= (1 << 4),
};

enum ECharacterEquipmentPart
{
	CHR_EQUIPPART_ARMOR,
	CHR_EQUIPPART_WEAPON,
	CHR_EQUIPPART_HEAD,
	CHR_EQUIPPART_HAIR,
	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	CHR_EQUIPPART_ACCE,
	#endif
	CHR_EQUIPPART_NUM,
};

typedef struct packet_add_char
{
	BYTE	header;
	DWORD	dwVID;

	float	angle;
	long	x;
	long	y;
	long	z;

	BYTE	bType;
	DWORD	wRaceNum; // @fixme501
	BYTE	bMovingSpeed;
	BYTE	bAttackSpeed;

	BYTE	bStateFlag;
	DWORD	dwAffectFlag[2];
} TPacketGCCharacterAdd;

typedef struct packet_char_additional_info
{
	BYTE	header;
	DWORD	dwVID;
	char	name[CHARACTER_NAME_MAX_LEN + 1];
	DWORD	awPart[CHR_EQUIPPART_NUM]; // @fixme502
	BYTE	bEmpire;
	DWORD	dwGuildID;
	DWORD	dwLevel;
	int		iAlignment;
	BYTE	bPKMode;
	DWORD	dwMountVnum;
	#ifdef ENABLE_EVOLUTION_SYSTEM
	int		iEvolutionWeapon;
	int		iEvolutionBody;
	#endif
} TPacketGCCharacterAdditionalInfo;

typedef struct packet_update_char
{
	BYTE	header;
	DWORD	dwVID;

	DWORD	awPart[CHR_EQUIPPART_NUM]; // @fixme502
	BYTE	bMovingSpeed;
	BYTE	bAttackSpeed;

	BYTE	bStateFlag;
	DWORD	dwAffectFlag[2];

	DWORD	dwGuildID;
	int		iAlignment;
	BYTE	bPKMode;
	DWORD	dwMountVnum;
	DWORD	dwLevel;
	#ifdef ENABLE_EVOLUTION_SYSTEM
	int		iEvolutionWeapon;
	int		iEvolutionBody;
	#endif
} TPacketGCCharacterUpdate;

typedef struct packet_del_char
{
	BYTE	header;
	DWORD	id;
} TPacketGCCharacterDelete;

typedef struct packet_chat
{
	BYTE	header;
	WORD	size;
	BYTE	type;
	DWORD	id;
	BYTE	bEmpire;
} TPacketGCChat;

typedef struct packet_whisper
{
	BYTE	bHeader;
	WORD	wSize;
	BYTE	bType;
	char	szNameFrom[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCWhisper;

typedef struct packet_main_character
{
	BYTE        header;
	DWORD	dwVID;
	DWORD	wRaceNum; // @fixme501
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	long	lx, ly, lz;
	BYTE	empire;
	BYTE	skill_group;
} TPacketGCMainCharacter;

// SUPPORT_BGM
typedef struct packet_main_character3_bgm
{
	enum
	{
		MUSIC_NAME_LEN = 24,
	};

	BYTE    header;
	DWORD	dwVID;
	DWORD	wRaceNum; // @fixme501
	char	szChrName[CHARACTER_NAME_MAX_LEN + 1];
	char	szBGMName[MUSIC_NAME_LEN + 1];
	long	lx, ly, lz;
	BYTE	empire;
	BYTE	skill_group;
} TPacketGCMainCharacter3_BGM;

typedef struct packet_main_character4_bgm_vol
{
	enum
	{
		MUSIC_NAME_LEN = 24,
	};

	BYTE    header;
	DWORD	dwVID;
	DWORD	wRaceNum; // @fixme501
	char	szChrName[CHARACTER_NAME_MAX_LEN + 1];
	char	szBGMName[MUSIC_NAME_LEN + 1];
	float	fBGMVol;
	long	lx, ly, lz;
	BYTE	empire;
	BYTE	skill_group;
} TPacketGCMainCharacter4_BGM_VOL;
// END_OF_SUPPORT_BGM

typedef struct packet_points
{
	BYTE	header;
	int64_t	points[POINT_MAX_NUM];
} TPacketGCPoints;

typedef struct packet_skill_level
{
	BYTE		bHeader;
	TPlayerSkill	skills[SKILL_MAX_NUM];
} TPacketGCSkillLevel;

typedef struct packet_point_change
{
	int		header;
	DWORD	dwVID;
	BYTE	type;
	int64_t	amount;
	int64_t	value;
} TPacketGCPointChange;

typedef struct packet_stun
{
	BYTE	header;
	DWORD	vid;
} TPacketGCStun;

typedef struct packet_dead
{
	BYTE	header;
	DWORD	vid;
} TPacketGCDead;

typedef struct packet_item_del_deprecated
{
	BYTE	header;
	TItemPos Cell;
} TPacketGCItemDelDeprecated;

typedef struct packet_item_set
{
	BYTE		header;
	TItemPos	Cell;
	DWORD		vnum;
	#ifdef ENABLE_NEW_MAX_COUNT
	short		count;
	#else
	BYTE		count;
	#endif
	DWORD		flags;
	DWORD		anti_flags;
	bool		highlight;
	long		alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
	#ifdef ENABLE_EVOLUTION_SYSTEM
	int			iEvolution;
	#endif
	#ifdef ENABLE_CHANGELOOK_SYSTEM
	DWORD		dwTransmutation;
	#endif
	#ifdef ENABLE_LOCKED_SYSTEM
	BYTE		bLocked;
	#endif
} TPacketGCItemSet;

typedef struct packet_item_del
{
	BYTE	header;
	BYTE	pos;
} TPacketGCItemDel;

struct packet_item_use
{
	BYTE	header;
	TItemPos Cell;
	DWORD	ch_vid;
	DWORD	victim_vid;
	DWORD	vnum;
};

struct packet_item_move
{
	BYTE	header;
	TItemPos Cell;
	TItemPos CellTo;
};

typedef struct packet_item_update
{
	BYTE	header;
	TItemPos Cell;
	#ifdef ENABLE_NEW_MAX_COUNT
	short	count;
	#else
	BYTE	count;
	#endif
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
	#ifdef ENABLE_EVOLUTION_SYSTEM
	int		iEvolution;
	#endif
	#ifdef ENABLE_CHANGELOOK_SYSTEM
	DWORD	dwTransmutation;
	#endif
	#ifdef ENABLE_LOCKED_SYSTEM
	BYTE	bLocked;
	#endif
} TPacketGCItemUpdate;

typedef struct packet_item_ground_add
{
	BYTE	bHeader;
	long 	x, y, z;
	DWORD	dwVID;
	DWORD	dwVnum;
} TPacketGCItemGroundAdd;

typedef struct packet_item_ownership
{
	BYTE	bHeader;
	DWORD	dwVID;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCItemOwnership;

typedef struct packet_item_ground_del
{
	BYTE	bHeader;
	DWORD	dwVID;
} TPacketGCItemGroundDel;

struct packet_quickslot_add
{
	BYTE	header;
	BYTE	pos;
	TQuickslot	slot;
};

struct packet_quickslot_del
{
	BYTE	header;
	BYTE	pos;
};

struct packet_quickslot_swap
{
	BYTE	header;
	BYTE	pos;
	BYTE	pos_to;
};

struct packet_motion
{
	BYTE	header;
	DWORD	vid;
	DWORD	victim_vid;
	WORD	motion;
};

enum EPacketShopSubHeaders
{
	SHOP_SUBHEADER_GC_START,
	SHOP_SUBHEADER_GC_END,
	SHOP_SUBHEADER_GC_UPDATE_ITEM,
	SHOP_SUBHEADER_GC_UPDATE_PRICE,
	SHOP_SUBHEADER_GC_OK,
	SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY,
	SHOP_SUBHEADER_GC_SOLDOUT,
	SHOP_SUBHEADER_GC_INVENTORY_FULL,
	SHOP_SUBHEADER_GC_INVALID_POS,
	SHOP_SUBHEADER_GC_SOLD_OUT,
	SHOP_SUBHEADER_GC_START_EX,
	SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY_EX,
};

struct packet_shop_item
{
	DWORD       vnum;
#ifdef ENABLE_FULL_YANG
	uint64_t	price;
#else
	DWORD       price;
#endif
#ifdef ENABLE_NEW_MAX_COUNT
	short		count;
#else
	BYTE		count;
#endif
	BYTE		display_pos;
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
};

typedef struct packet_shop_start
{
	DWORD   owner_vid;
	struct packet_shop_item	items[SHOP_HOST_ITEM_MAX_NUM];
} TPacketGCShopStart;

typedef struct packet_shop_start_ex
{
	typedef struct sub_packet_shop_tab
	{
		char name[SHOP_TAB_NAME_MAX];
		BYTE coin_type;
		packet_shop_item items[SHOP_HOST_ITEM_MAX_NUM];
	} TSubPacketShopTab;
	DWORD owner_vid;
	BYTE shop_tab_count;
} TPacketGCShopStartEx;

typedef struct packet_shop_update_item
{
	BYTE			pos;
	struct packet_shop_item	item;
} TPacketGCShopUpdateItem;

typedef struct packet_shop_update_price
{
	int				iPrice;
} TPacketGCShopUpdatePrice;

typedef struct packet_shop
{
	BYTE			header;
	WORD			size;
	BYTE			subheader;
} TPacketGCShop;

struct packet_exchange
{
	BYTE		header;
	BYTE		sub_header;
	BYTE		is_me;
	#ifdef ENABLE_FULL_YANG
	uint64_t 	arg1;
	#else
	DWORD		arg1;	// vnum
	#endif
	TItemPos	arg2;	// cell
	DWORD		arg3;	// count
	#ifdef __ENABLE_TRADABLE_ICON__
	TItemPos	arg4;	// srccell
	#endif
	long		alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
	#ifdef ENABLE_EVOLUTION_SYSTEM
	int			iEvolution;
	#endif
	#ifdef ENABLE_CHANGELOOK_SYSTEM
	DWORD		dwTransmutation;
	#endif
};

enum EPacketTradeSubHeaders
{
	EXCHANGE_SUBHEADER_GC_START,	/* arg1 == vid */
	EXCHANGE_SUBHEADER_GC_ITEM_ADD,	/* arg1 == vnum  arg2 == pos  arg3 == count */
	EXCHANGE_SUBHEADER_GC_ITEM_DEL,
	EXCHANGE_SUBHEADER_GC_GOLD_ADD,	/* arg1 == gold */
	EXCHANGE_SUBHEADER_GC_ACCEPT,	/* arg1 == accept */
	EXCHANGE_SUBHEADER_GC_END,		/* arg1 == not used */
	EXCHANGE_SUBHEADER_GC_ALREADY,	/* arg1 == not used */
	EXCHANGE_SUBHEADER_GC_LESS_GOLD,	/* arg1 == not used */
};

struct packet_position
{
	BYTE	header;
	DWORD	vid;
	BYTE	position;
};

typedef struct packet_ping
{
	BYTE	header;
} TPacketGCPing;

struct packet_script
{
	BYTE	header;
	WORD	size;
	BYTE	skin;
	WORD	src_size;
#ifdef ENABLE_QUEST_CATEGORY
	BYTE	quest_flag;
#endif
};

typedef struct packet_change_speed
{
	BYTE		header;
	DWORD		vid;
	WORD		moving_speed;
} TPacketGCChangeSpeed;

struct packet_mount
{
	BYTE	header;
	DWORD	vid;
	DWORD	mount_vid;
	BYTE	pos;
	DWORD	x, y;
};

typedef struct packet_move
{
	BYTE		bHeader;
	BYTE		bFunc;
	BYTE		bArg;
	BYTE		bRot;
	DWORD		dwVID;
	long		lX;
	long		lY;
	DWORD		dwTime;
	DWORD		dwDuration;
} TPacketGCMove;


typedef struct packet_ownership
{
	BYTE		bHeader;
	DWORD		dwOwnerVID;
	DWORD		dwVictimVID;
} TPacketGCOwnership;

typedef struct packet_sync_position_element
{
	DWORD	dwVID;
	long	lX;
	long	lY;
} TPacketGCSyncPositionElement;

typedef struct packet_sync_position
{
	BYTE	bHeader;
	WORD	wSize;
} TPacketGCSyncPosition;

typedef struct packet_fly
{
	BYTE	bHeader;
	BYTE	bType;
	DWORD	dwStartVID;
	DWORD	dwEndVID;
} TPacketGCCreateFly;

typedef struct command_fly_targeting
{
	BYTE		bHeader;
	DWORD		dwTargetVID;
	long		x, y;
} TPacketCGFlyTargeting;

typedef struct packet_fly_targeting
{
	BYTE		bHeader;
	DWORD		dwShooterVID;
	DWORD		dwTargetVID;
	long		x, y;
} TPacketGCFlyTargeting;

typedef struct packet_shoot
{
	BYTE		bHeader;
	BYTE		bType;
} TPacketCGShoot;

typedef struct packet_duel_start
{
	BYTE	header;
	WORD	wSize;
} TPacketGCDuelStart;

enum EPVPModes
{
	PVP_MODE_NONE,
	PVP_MODE_AGREE,
	PVP_MODE_FIGHT,
	PVP_MODE_REVENGE
};

typedef struct packet_pvp
{
	BYTE        bHeader;
	DWORD       dwVIDSrc;
	DWORD       dwVIDDst;
	BYTE        bMode;
} TPacketGCPVP;

typedef struct command_use_skill
{
	BYTE	bHeader;
	DWORD	dwVnum;
	DWORD	dwVID;
} TPacketCGUseSkill;

typedef struct command_target
{
	BYTE	header;
	DWORD	dwVID;
} TPacketCGTarget;

typedef struct packet_target
{
	BYTE	header;
	DWORD	dwVID;
	BYTE	bHPPercent;
} TPacketGCTarget;

typedef struct packet_warp
{
	BYTE	bHeader;
	long	lX;
	long	lY;
	long	lAddr;
	WORD	wPort;
} TPacketGCWarp;

typedef struct command_warp
{
	BYTE	bHeader;
} TPacketCGWarp;

struct packet_quest_info
{
	BYTE header;
	WORD size;
	WORD index;
	#ifdef ENABLE_QUEST_CATEGORY
	WORD c_index;
	#endif
	BYTE flag;
};

enum
{
	#ifdef ENABLE_MESSENGER_BLOCK
	MESSENGER_SUBHEADER_GC_BLOCK_LIST,
	MESSENGER_SUBHEADER_GC_BLOCK_LOGIN,
	MESSENGER_SUBHEADER_GC_BLOCK_LOGOUT,
	MESSENGER_SUBHEADER_GC_BLOCK_INVITE,//not used
	#endif
	MESSENGER_SUBHEADER_GC_LIST,
	MESSENGER_SUBHEADER_GC_LOGIN,
	MESSENGER_SUBHEADER_GC_LOGOUT,
	#ifdef ENABLE_MESSENGER_TEAM_SYSTEM
	MESSENGER_SUBHEADER_GC_TEAM_LIST,
	MESSENGER_SUBHEADER_GC_TEAM_LOGIN,
	MESSENGER_SUBHEADER_GC_TEAM_LOGOUT,
	#endif
	MESSENGER_SUBHEADER_GC_INVITE,
	MESSENGER_SUBHEADER_GC_MOBILE
};

typedef struct packet_messenger
{
	BYTE header;
	WORD size;
	BYTE subheader;
} TPacketGCMessenger;

typedef struct packet_messenger_guild_list
{
	BYTE connected;
	BYTE length;
	//char login[LOGIN_MAX_LEN+1];
} TPacketGCMessengerGuildList;

typedef struct packet_messenger_guild_login
{
	BYTE length;
	//char login[LOGIN_MAX_LEN+1];
} TPacketGCMessengerGuildLogin;

typedef struct packet_messenger_guild_logout
{
	BYTE length;

	//char login[LOGIN_MAX_LEN+1];
} TPacketGCMessengerGuildLogout;

typedef struct packet_messenger_list_offline
{
	BYTE connected; // always 0
	BYTE length;
} TPacketGCMessengerListOffline;

typedef struct packet_messenger_list_online
{
	BYTE connected; // always 1
	BYTE length;
} TPacketGCMessengerListOnline;

#ifdef ENABLE_MESSENGER_BLOCK
typedef struct packet_messenger_block_list_offline
{
	BYTE connected; // always 0
	BYTE length;
} TPacketGCMessengerBlockListOffline;

typedef struct packet_messenger_block_list_online
{
	BYTE connected; // always 1
	BYTE length;
} TPacketGCMessengerBlockListOnline;
#endif

#ifdef ENABLE_MESSENGER_TEAM_SYSTEM
typedef struct packet_messenger_team_list_offline
{
	BYTE connected; // always 0
	BYTE length;
} TPacketGCMessengerTeamListOffline;

typedef struct packet_messenger_team_list_online
{
	BYTE connected; // always 1
	BYTE length;
} TPacketGCMessengerTeamListOnline;
#endif

enum
{
	#ifdef ENABLE_MESSENGER_BLOCK
	MESSENGER_SUBHEADER_CG_ADD_BLOCK_BY_VID,
	MESSENGER_SUBHEADER_CG_ADD_BLOCK_BY_NAME,
	MESSENGER_SUBHEADER_CG_REMOVE_BLOCK,
	#endif
	MESSENGER_SUBHEADER_CG_ADD_BY_VID,
	MESSENGER_SUBHEADER_CG_ADD_BY_NAME,
	MESSENGER_SUBHEADER_CG_REMOVE,
	MESSENGER_SUBHEADER_CG_INVITE_ANSWER,
};

#ifdef ENABLE_MESSENGER_BLOCK
typedef struct command_messenger_add_block_by_vid
{
	DWORD vid;
} TPacketCGMessengerAddBlockByVID;

typedef struct command_messenger_add_block_by_name
{
	BYTE length;
} TPacketCGMessengerAddBlockByName;

typedef struct command_messenger_remove_block
{
	char login[LOGIN_MAX_LEN+1];
} TPacketCGMessengerRemoveBlock;
#endif

typedef struct command_messenger
{
	BYTE header;
	BYTE subheader;
} TPacketCGMessenger;

typedef struct command_messenger_add_by_vid
{
	DWORD vid;
} TPacketCGMessengerAddByVID;

typedef struct command_messenger_add_by_name
{
	BYTE length;
	//char login[LOGIN_MAX_LEN+1];
} TPacketCGMessengerAddByName;

typedef struct command_messenger_remove
{
	char login[LOGIN_MAX_LEN+1];
	//DWORD account;
} TPacketCGMessengerRemove;

typedef struct command_safebox_checkout
{
	BYTE	bHeader;
	BYTE	bSafePos;
	TItemPos	ItemPos;
} TPacketCGSafeboxCheckout;

typedef struct command_safebox_checkin
{
	BYTE	bHeader;
	BYTE	bSafePos;
	TItemPos	ItemPos;
} TPacketCGSafeboxCheckin;

///////////////////////////////////////////////////////////////////////////////////
// Party

typedef struct command_party_parameter
{
	BYTE	bHeader;
	BYTE	bDistributeMode;
} TPacketCGPartyParameter;

typedef struct paryt_parameter
{
	BYTE	bHeader;
	BYTE	bDistributeMode;
} TPacketGCPartyParameter;

typedef struct packet_party_add
{
	BYTE	header;
	DWORD	pid;
	char	name[CHARACTER_NAME_MAX_LEN+1];
} TPacketGCPartyAdd;

typedef struct command_party_invite
{
	BYTE	header;
	DWORD	vid;
} TPacketCGPartyInvite;

typedef struct packet_party_invite
{
	BYTE	header;
	DWORD	leader_vid;
} TPacketGCPartyInvite;

typedef struct command_party_invite_answer
{
	BYTE	header;
	DWORD	leader_vid;
	BYTE	accept;
} TPacketCGPartyInviteAnswer;

typedef struct packet_party_update
{
	BYTE	header;
	DWORD	pid;
	BYTE	role;
	BYTE	percent_hp;
	short	affects[7];
} TPacketGCPartyUpdate;

typedef struct packet_party_remove
{
	BYTE header;
	DWORD pid;
} TPacketGCPartyRemove;

typedef struct packet_party_link
{
	BYTE header;
	DWORD pid;
	DWORD vid;
} TPacketGCPartyLink;

typedef struct packet_party_unlink
{
	BYTE header;
	DWORD pid;
	DWORD vid;
} TPacketGCPartyUnlink;

typedef struct command_party_remove
{
	BYTE header;
	DWORD pid;
} TPacketCGPartyRemove;

typedef struct command_party_set_state
{
	BYTE header;
	DWORD pid;
	BYTE byRole;
	BYTE flag;
} TPacketCGPartySetState;

enum
{
	PARTY_SKILL_HEAL = 1,
	PARTY_SKILL_WARP = 2
};

typedef struct command_party_use_skill
{
	BYTE header;
	BYTE bySkillIndex;
	DWORD vid;
} TPacketCGPartyUseSkill;

typedef struct packet_safebox_size
{
	BYTE bHeader;
	BYTE bSize;
} TPacketCGSafeboxSize;

typedef struct packet_safebox_wrong_password
{
	BYTE	bHeader;
} TPacketCGSafeboxWrongPassword;

typedef struct command_empire
{
	BYTE	bHeader;
	BYTE	bEmpire;
} TPacketCGEmpire;

typedef struct packet_empire
{
	BYTE	bHeader;
	BYTE	bEmpire;
} TPacketGCEmpire;

enum
{
	SAFEBOX_MONEY_STATE_SAVE,
	SAFEBOX_MONEY_STATE_WITHDRAW,
};

typedef struct command_safebox_money
{
	BYTE        bHeader;
	BYTE        bState;
	long	lMoney;
} TPacketCGSafeboxMoney;

typedef struct packet_safebox_money_change
{
	BYTE	bHeader;
	long	lMoney;
} TPacketGCSafeboxMoneyChange;

// Guild

enum
{
	GUILD_SUBHEADER_GC_LOGIN,
	GUILD_SUBHEADER_GC_LOGOUT,
	GUILD_SUBHEADER_GC_LIST,
	GUILD_SUBHEADER_GC_GRADE,
	GUILD_SUBHEADER_GC_ADD,
	GUILD_SUBHEADER_GC_REMOVE,
	GUILD_SUBHEADER_GC_GRADE_NAME,
	GUILD_SUBHEADER_GC_GRADE_AUTH,
	GUILD_SUBHEADER_GC_INFO,
	GUILD_SUBHEADER_GC_COMMENTS,
	GUILD_SUBHEADER_GC_CHANGE_EXP,
	GUILD_SUBHEADER_GC_CHANGE_MEMBER_GRADE,
	GUILD_SUBHEADER_GC_SKILL_INFO,
	GUILD_SUBHEADER_GC_CHANGE_MEMBER_GENERAL,
	GUILD_SUBHEADER_GC_GUILD_INVITE,
	GUILD_SUBHEADER_GC_WAR,
	GUILD_SUBHEADER_GC_GUILD_NAME,
	GUILD_SUBHEADER_GC_GUILD_WAR_LIST,
	GUILD_SUBHEADER_GC_GUILD_WAR_END_LIST,
	GUILD_SUBHEADER_GC_WAR_SCORE,
	GUILD_SUBHEADER_GC_MONEY_CHANGE,
};

enum GUILD_SUBHEADER_CG
{
	GUILD_SUBHEADER_CG_ADD_MEMBER,
	GUILD_SUBHEADER_CG_REMOVE_MEMBER,
	GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME,
	GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY,
	GUILD_SUBHEADER_CG_OFFER,
	GUILD_SUBHEADER_CG_POST_COMMENT,
	GUILD_SUBHEADER_CG_DELETE_COMMENT,
	GUILD_SUBHEADER_CG_REFRESH_COMMENT,
	GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE,
	GUILD_SUBHEADER_CG_USE_SKILL,
	GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL,
	GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER,
	GUILD_SUBHEADER_CG_CHARGE_GSP,
	GUILD_SUBHEADER_CG_DEPOSIT_MONEY,
	GUILD_SUBHEADER_CG_WITHDRAW_MONEY,
};

typedef struct packet_guild
{
	BYTE header;
	WORD size;
	BYTE subheader;
} TPacketGCGuild;

typedef struct packet_guild_name_t
{
	BYTE header;
	WORD size;
	BYTE subheader;
	DWORD	guildID;
	char	guildName[GUILD_NAME_MAX_LEN];
} TPacketGCGuildName;

typedef struct packet_guild_war
{
	DWORD	dwGuildSelf;
	DWORD	dwGuildOpp;
	BYTE	bType;
	BYTE	bWarState;
	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	int		iScore;
	int		iMinLevel;
	int		iMaxPlayer;
	bool	bWarrior;
	bool	bAssassin;
	bool	bSura;
	bool	bShaman;
	#endif
} TPacketGCGuildWar;

typedef struct command_guild
{
	BYTE header;
	BYTE subheader;
} TPacketCGGuild;

typedef struct command_guild_answer_make_guild
{
	BYTE header;
	char guild_name[GUILD_NAME_MAX_LEN+1];
} TPacketCGAnswerMakeGuild;

typedef struct command_guild_use_skill
{
	DWORD	dwVnum;
	DWORD	dwPID;
} TPacketCGGuildUseSkill;

// Guild Mark
typedef struct command_mark_login
{
	BYTE    header;
	DWORD   handle;
	DWORD   random_key;
} TPacketCGMarkLogin;

typedef struct command_mark_upload
{
	BYTE	header;
	DWORD	gid;
	BYTE	image[16*12*4];
} TPacketCGMarkUpload;

typedef struct command_mark_idxlist
{
	BYTE	header;
} TPacketCGMarkIDXList;

typedef struct command_mark_crclist
{
	BYTE	header;
	BYTE	imgIdx;
	DWORD	crclist[80];
} TPacketCGMarkCRCList;

typedef struct packet_mark_idxlist
{
	BYTE    header;
	DWORD	bufSize;
	WORD	count;

} TPacketGCMarkIDXList;

typedef struct packet_mark_block
{
	BYTE	header;
	DWORD	bufSize;
	BYTE	imgIdx;
	DWORD	count;
} TPacketGCMarkBlock;

typedef struct command_symbol_upload
{
	BYTE	header;
	WORD	size;
	DWORD	guild_id;
} TPacketCGGuildSymbolUpload;

typedef struct command_symbol_crc
{
	BYTE header;
	DWORD guild_id;
	DWORD crc;
	DWORD size;
} TPacketCGSymbolCRC;

typedef struct packet_symbol_data
{
	BYTE header;
	WORD size;
	DWORD guild_id;
} TPacketGCGuildSymbolData;

// Fishing

typedef struct command_fishing
{
	BYTE header;
	BYTE dir;
} TPacketCGFishing;

typedef struct packet_fishing
{
	BYTE header;
	BYTE subheader;
	DWORD info;
	BYTE dir;
} TPacketGCFishing;

enum
{
	FISHING_SUBHEADER_GC_START,
	FISHING_SUBHEADER_GC_STOP,
	FISHING_SUBHEADER_GC_REACT,
	FISHING_SUBHEADER_GC_SUCCESS,
	FISHING_SUBHEADER_GC_FAIL,
	FISHING_SUBHEADER_GC_FISH,
};

typedef struct command_give_item
{
	BYTE byHeader;
	DWORD dwTargetVID;
	TItemPos ItemPos;
#ifdef ENABLE_NEW_MAX_COUNT
	short	byItemCount;
#else
	BYTE	byItemCount;
#endif
} TPacketCGGiveItem;

typedef struct SPacketCGHack
{
	BYTE	bHeader;
	char	szBuf[255 + 1];
} TPacketCGHack;

// SubHeader - Dungeon
enum
{
	DUNGEON_SUBHEADER_GC_TIME_ATTACK_START = 0,
	DUNGEON_SUBHEADER_GC_DESTINATION_POSITION = 1,
};

typedef struct packet_dungeon
{
	BYTE bHeader;
	WORD size;
	BYTE subheader;
} TPacketGCDungeon;

typedef struct packet_dungeon_dest_position
{
	long x;
	long y;
} TPacketGCDungeonDestPosition;

typedef struct SPacketGCShopSign
{
	BYTE	bHeader;
	DWORD	dwVID;
	char	szSign[SHOP_SIGN_MAX_LEN + 1];
} TPacketGCShopSign;

typedef struct SPacketCGMyShop
{
	BYTE	bHeader;
	char	szSign[SHOP_SIGN_MAX_LEN + 1];
#ifdef ENABLE_NEW_MAX_COUNT
	short	bCount;
#else
	BYTE	bCount;
#endif
} TPacketCGMyShop;

typedef struct SPacketGCTime
{
	BYTE	bHeader;
	time_t	time;
} TPacketGCTime;

enum
{
	WALKMODE_RUN,
	WALKMODE_WALK,
};

typedef struct SPacketGCWalkMode
{
	BYTE	header;
	DWORD	vid;
	BYTE	mode;
} TPacketGCWalkMode;

typedef struct SPacketGCChangeSkillGroup
{
	BYTE        header;
	BYTE        skill_group;
} TPacketGCChangeSkillGroup;

typedef struct SPacketCGRefine
{
	BYTE	header;
	BYTE	pos;
	BYTE	type;
} TPacketCGRefine;

typedef struct SPacketCGRequestRefineInfo
{
	BYTE	header;
	BYTE	pos;
} TPacketCGRequestRefineInfo;

typedef struct SPacketGCRefineInformaion
{
	BYTE	header;
	BYTE	type;
	BYTE	pos;
	DWORD	src_vnum;
	DWORD	result_vnum;
	#ifdef ENABLE_NEW_MAX_COUNT
	short	material_count;
	#else
	BYTE	material_count;
	#endif
	#ifdef ENABLE_FULL_YANG
	int64_t	cost;
	#else
	int		cost;
	#endif
	int		prob;
	TRefineMaterial materials[REFINE_MATERIAL_MAX_NUM];
} TPacketGCRefineInformation;

#ifdef ENABLE_DUNGEON_BOSS_ICON_IN_MAP
struct TNPCPosition
{
	BYTE bType;
	DWORD vnum;
	long x;
	long y;
	DWORD time;
};
#else
struct TNPCPosition
{
	BYTE bType;
	char name[CHARACTER_NAME_MAX_LEN + 1];
	long x;
	long y;
};
#endif

typedef struct SPacketGCNPCPosition
{
	BYTE header;
	WORD size;
	WORD count;

	// array of TNPCPosition
} TPacketGCNPCPosition;

typedef struct SPacketGCSpecialEffect
{
	BYTE header;
	BYTE type;
	DWORD vid;
} TPacketGCSpecialEffect;

typedef struct SPacketCGChangeName
{
	BYTE header;
	BYTE index;
	char name[CHARACTER_NAME_MAX_LEN+1];
} TPacketCGChangeName;

typedef struct SPacketGCChangeName
{
	BYTE header;
	DWORD pid;
	char name[CHARACTER_NAME_MAX_LEN+1];
} TPacketGCChangeName;


typedef struct command_client_version
{
	BYTE header;
	char filename[32+1];
	char timestamp[32+1];
} TPacketCGClientVersion;

typedef struct command_client_version2
{
	BYTE header;
	char filename[32+1];
	char timestamp[32+1];
} TPacketCGClientVersion2;

typedef struct packet_channel
{
	BYTE header;
	BYTE channel;
} TPacketGCChannel;

typedef struct SEquipmentItemSet
{
	DWORD	vnum;
	#ifdef ENABLE_NEW_MAX_COUNT
	short	count;
	#else
	BYTE	count;
	#endif
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
} TEquipmentItemSet;

typedef struct pakcet_view_equip
{
	BYTE  header;
	DWORD vid;
	TEquipmentItemSet equips[WEAR_MAX_NUM];
} TPacketViewEquip;

typedef struct SLandPacketElement
{
	DWORD	dwID;
	long	x, y;
	long	width, height;
	DWORD	dwGuildID;
} TLandPacketElement;

typedef struct packet_land_list
{
	BYTE	header;
	WORD	size;
} TPacketGCLandList;

typedef struct SPacketGCTargetCreate
{
	BYTE	bHeader;
	long	lID;
	char	szName[32+1];
	DWORD	dwVID;
	BYTE	bType;
} TPacketGCTargetCreate;

typedef struct SPacketGCTargetUpdate
{
	BYTE	bHeader;
	long	lID;
	long	lX, lY;
} TPacketGCTargetUpdate;

typedef struct SPacketGCTargetDelete
{
	BYTE	bHeader;
	long	lID;
} TPacketGCTargetDelete;

typedef struct SPacketGCAffectAdd
{
	BYTE		bHeader;
	TPacketAffectElement elem;
} TPacketGCAffectAdd;

typedef struct SPacketGCAffectRemove
{
	BYTE	bHeader;
	DWORD	dwType;
	BYTE	bApplyOn;
} TPacketGCAffectRemove;

typedef struct packet_lover_info
{
	BYTE header;
	char name[CHARACTER_NAME_MAX_LEN + 1];
	BYTE love_point;
} TPacketGCLoverInfo;

typedef struct packet_love_point_update
{
	BYTE header;
	BYTE love_point;
} TPacketGCLovePointUpdate;

// MINING
typedef struct packet_dig_motion
{
	BYTE header;
	DWORD vid;
	DWORD target_vid;
	BYTE count;
} TPacketGCDigMotion;
// END_OF_MINING

// SCRIPT_SELECT_ITEM
typedef struct command_script_select_item
{
	BYTE header;
	DWORD selection;
} TPacketCGScriptSelectItem;
// END_OF_SCRIPT_SELECT_ITEM

typedef struct packet_damage_info
{
	BYTE header;
	DWORD dwVID;
	BYTE flag;
	int damage;
} TPacketGCDamageInfo;

typedef struct tag_GGSiege
{
	BYTE	bHeader;
	BYTE	bEmpire;
	BYTE	bTowerCount;
} TPacketGGSiege;

typedef struct SPacketGGMonarchTransfer
{
	BYTE	bHeader;
	DWORD	dwTargetPID;
	long	x;
	long	y;
} TPacketMonarchGGTransfer;

typedef struct SPacketGGCheckAwakeness
{
	BYTE bHeader;
} TPacketGGCheckAwakeness;

typedef struct SPacketGCPanamaPack
{
	BYTE	bHeader;
	char	szPackName[256];
	BYTE	abIV[32];
} TPacketGCPanamaPack;


typedef struct SPacketGCHybridCryptKeys
{
	SPacketGCHybridCryptKeys() : m_pStream(NULL) {}
	~SPacketGCHybridCryptKeys()
	{

		if( m_pStream )
		{
			delete[] m_pStream;
			m_pStream = NULL;
		}
	}

	DWORD GetStreamSize()
	{
		return sizeof(bHeader) + sizeof(WORD) + sizeof(int) + KeyStreamLen;
	}

	BYTE* GetStreamData()
	{
		if( m_pStream )
			delete[] m_pStream;

		uDynamicPacketSize = (WORD)GetStreamSize();

		m_pStream = new BYTE[ uDynamicPacketSize ];

		memcpy( m_pStream, &bHeader, 1 );
		memcpy( m_pStream+1, &uDynamicPacketSize, 2 );
		memcpy( m_pStream+3, &KeyStreamLen, 4 );

		if( KeyStreamLen > 0 )
			memcpy( m_pStream+7, pDataKeyStream, KeyStreamLen );

		return m_pStream;
	}

	BYTE	bHeader;
	WORD    uDynamicPacketSize;
	int		KeyStreamLen;
	BYTE*   pDataKeyStream;

private:
	BYTE* m_pStream;


} TPacketGCHybridCryptKeys;


typedef struct SPacketGCPackageSDB
{
	SPacketGCPackageSDB() : m_pDataSDBStream(NULL), m_pStream(NULL) {}
	~SPacketGCPackageSDB()
	{
		if( m_pStream )
		{
			delete[] m_pStream;
			m_pStream = NULL;
		}
	}

	DWORD GetStreamSize()
	{
		return sizeof(bHeader) + sizeof(WORD) + sizeof(int) + iStreamLen;
	}

	BYTE* GetStreamData()
	{
		if( m_pStream )
			delete[] m_pStream;

		uDynamicPacketSize =  GetStreamSize();

		m_pStream = new BYTE[ uDynamicPacketSize ];

		memcpy( m_pStream, &bHeader, 1 );
		memcpy( m_pStream+1, &uDynamicPacketSize, 2 );
		memcpy( m_pStream+3, &iStreamLen, 4 );

		if( iStreamLen > 0 )
			memcpy( m_pStream+7, m_pDataSDBStream, iStreamLen );

		return m_pStream;
	}

	BYTE	bHeader;
	WORD    uDynamicPacketSize;
	int		iStreamLen;
	BYTE*   m_pDataSDBStream;

private:
	BYTE* m_pStream;


} TPacketGCPackageSDB;

#ifdef _IMPROVED_PACKET_ENCRYPTION_
struct TPacketKeyAgreement
{
	static const int MAX_DATA_LEN = 256;
	BYTE bHeader;
	WORD wAgreedLength;
	WORD wDataLength;
	BYTE data[MAX_DATA_LEN];
};

struct TPacketKeyAgreementCompleted
{
	BYTE bHeader;
	BYTE data[3]; // dummy (not used)
};

#endif // _IMPROVED_PACKET_ENCRYPTION_

#define MAX_EFFECT_FILE_NAME 128
typedef struct SPacketGCSpecificEffect
{
	BYTE header;
	DWORD vid;
	char effect_file[MAX_EFFECT_FILE_NAME];
} TPacketGCSpecificEffect;


enum EDragonSoulRefineWindowRefineType
{
	DragonSoulRefineWindow_UPGRADE,
	DragonSoulRefineWindow_IMPROVEMENT,
	DragonSoulRefineWindow_REFINE,
};

enum EPacketCGDragonSoulSubHeaderType
{
	DS_SUB_HEADER_OPEN,
	DS_SUB_HEADER_CLOSE,
	DS_SUB_HEADER_DO_REFINE_GRADE,
	DS_SUB_HEADER_DO_REFINE_STEP,
	DS_SUB_HEADER_DO_REFINE_STRENGTH,
	DS_SUB_HEADER_REFINE_FAIL,
	DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE,
	DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL,
	DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY,
	DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL,
	DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL,
	DS_SUB_HEADER_REFINE_SUCCEED,
};
typedef struct SPacketCGDragonSoulRefine
{
	SPacketCGDragonSoulRefine() : header (HEADER_CG_DRAGON_SOUL_REFINE)
	{}
	BYTE header;
	BYTE bSubType;
	TItemPos ItemGrid[DRAGON_SOUL_REFINE_GRID_SIZE];
} TPacketCGDragonSoulRefine;

typedef struct SPacketGCDragonSoulRefine
{
	SPacketGCDragonSoulRefine() : header(HEADER_GC_DRAGON_SOUL_REFINE)
	{}
	BYTE header;
	BYTE bSubType;
	TItemPos Pos;
} TPacketGCDragonSoulRefine;

typedef struct SPacketCGStateCheck
{
	BYTE header;
	unsigned long key;
	unsigned long index;
} TPacketCGStateCheck;

typedef struct SPacketGCStateCheck
{
	BYTE header;
	unsigned long key;
	unsigned long index;
	unsigned char state;
} TPacketGCStateCheck;

#ifdef ENABLE_IKASHOP_RENEWAL
//ACTIONS PACKETS
struct TPacketGCNewIkarusShop
{
	BYTE header;
	#ifdef ENABLE_LARGE_DYNAMIC_PACKETS
	int size;
	#else
	WORD size;
	#endif
	BYTE subheader;
};

struct TPacketCGNewIkarusShop {
	BYTE header;
	WORD size;
	BYTE subheader;
};

namespace ikashop
{
	//common
	struct TFilterInfo
	{
		BYTE type;
		BYTE subtype;
		char name[ITEM_NAME_MAX_LEN];
		TPriceInfo pricestart;
		TPriceInfo pricend;
		int levelstart;
		int levelend;
		#ifdef ENABLE_NEW_MAX_COUNT
		short countstart;
		short countend;
		#else
		BYTE countstart;
		BYTE countend;
		#endif
		TPlayerItemAttribute attrs[ITEM_ATTRIBUTE_NORM_NUM];
		int sashGrade;
		int alchemyGrade;
	};

	struct TShopItemInfo
	{
		TItemPos pos;
		TPriceInfo price;
	};

	struct TSearchResultInfo : public ikashop::TShopItem
	{
		bool auction;
		char seller_name[OFFLINE_SHOP_NAME_MAX_LEN];
		int duration;
	};

	#ifdef ENABLE_SHOP_SEARCH_SYSTEM
	struct TFilterName
	{
		char szName[ITEM_NAME_MAX_LEN];
		DWORD dwVnum;
		long lSocket;
	};

	struct TSearchNameResultInfo
	{
		DWORD dwOwnerID;
		DWORD dwShopVID;
	};
	#endif

	//AUCTION
	struct TAuctionListElement 
	{
		TAuctionInfo auction;
		TPriceInfo best;
		DWORD offercount;
	};

	//ikashop-updated 03/08/19
	struct TOfferListElement : public TOfferInfo 
	{
		TShopItem item;
		char shopname[OFFLINE_SHOP_NAME_MAX_LEN];
		bool incoming;
	};

	//GAME TO CLIENT
	enum eSubHeaderGC
	{
		SUBHEADER_GC_SHOP_OPEN,
		SUBHEADER_GC_SHOP_OPEN_OWNER,
		SUBHEADER_GC_SHOP_OPEN_OWNER_NO_SHOP,
		SUBHEADER_GC_SHOP_EXPIRED_GUESTING,
		SUBHEADER_GC_SHOP_REMOVE_ITEM_OWNER,
		SUBHEADER_GC_SHOP_REMOVE_ITEM_GUEST,
		SUBHEADER_GC_SHOP_EDIT_ITEM_OWNER,
		SUBHEADER_GC_SHOP_EDIT_ITEM_GUEST,

		SUBHEADER_GC_OFFER_LIST,

		SUBHEADER_GC_SHOP_FILTER_RESULT,

		#ifdef ENABLE_SHOP_SEARCH_SYSTEM
		SUBHEADER_GC_SHOP_FILTER_NAME_RESULT,
		#endif

		SUBHEADER_GC_SHOP_SAFEBOX_REFRESH,
		SUBHEADER_GC_SHOP_SAFEBOX_REMOVE_ITEM,
		SUBHEADER_GC_SHOP_SAFEBOX_ADD_ITEM,

		//AUCTION
		SUBHEADER_GC_AUCTION_LIST,
		SUBHEADER_GC_OPEN_MY_AUCTION,
		SUBHEADER_GC_OPEN_MY_AUCTION_NO_AUCTION,
		SUBHEADER_GC_OPEN_AUCTION,

		// ERROR HANDLING
		SUBHEADER_GC_POPUP_MESSAGE,

		#ifdef ENABLE_IKASHOP_ENTITIES
		SUBHEADER_GC_INSERT_SHOP_ENTITY,
		SUBHEADER_GC_REMOVE_SHOP_ENTITY,
		#endif

		SUBHEADER_GC_BOARD_COUNTERS,
		SUBHEADER_GC_SEARCH_RESULT_DELETE,

		#ifdef ENABLE_IKASHOP_AVG_AND_MINMAX_PRICE
		SUBHEADER_GC_SHOP_AVARAGE_AND_MINMAX_PRICE,
		#endif
	};

	struct TSubPacketGCShopOpen 
	{
		TShopInfo shop{};
	};

	struct TSubPacketGCShopOpenOwner 
	{
		TShopInfo shop{};
	};

	struct TSubPacketGCShopExpiredGuesting 
	{
		DWORD ownerid;
	};

	struct TSubPacketGCShopRemoveItem
	{
		DWORD itemid;
	};

	struct TSubPacketGCShopEditItem
	{
		DWORD itemid;
		TPriceInfo price;
	};

	struct TSubPacketGCShopFilterResult 
	{
		DWORD count;
	};

	struct TSubPacketGCShopOfferList 
	{
		DWORD offercount;
	};

	struct TSubPacketGCShopSafeboxRefresh 
	{
		TValutesInfo valute;
		DWORD itemcount;
	};

	struct TSubPacketGCShopSafeboxRemoveItem
	{
		DWORD itemid;
	};

	struct TSubPacketGCShoppingSafeboxAddItem
	{
		TShopPlayerItem item;
	};

	//AUCTION
	struct TSubPacketGCAuctionList 
	{
		DWORD count;
	};

	struct TSubPacketGCAuctionOpen 
	{
		TAuctionInfo auction;
		DWORD offercount;
	};

	struct TSubPacketGCPopupMessage
	{
		static constexpr inline auto MESSAGE_LEN = 80;
		char localeString[MESSAGE_LEN];
	};

	#ifdef ENABLE_IKASHOP_ENTITIES
	struct TSubPacketGCInsertShopEntity 
	{
		DWORD vid;
		char name[OFFLINE_SHOP_NAME_MAX_LEN];
		int type;
		int x;
		int y;
		int z;
	};

	struct TSubPacketGCRemoveShopEntity 
	{
		DWORD vid;
	};
	#endif

	struct TSubPacketGCBoardCounters
	{
		int safebox;
		int offers;
		int auction;
	};


	// CLIENT TO GAME
	enum eSubHeaderCG
	{
		SUBHEADER_CG_SHOP_CREATE_NEW,
		SUBHEADER_CG_SHOP_FORCE_CLOSE,
		SUBHEADER_CG_SHOP_REQUEST_SHOPLIST,
		SUBHEADER_CG_SHOP_OPEN,
		SUBHEADER_CG_SHOP_OPEN_OWNER,
		SUBHEADER_CG_SHOP_BUY_ITEM,

		SUBHEADER_CG_SHOP_ADD_ITEM,
		SUBHEADER_CG_SHOP_REMOVE_ITEM,
		SUBHEADER_CG_SHOP_EDIT_ITEM,

		SUBHEADER_CG_SHOP_FILTER_REQUEST,
		SUBHEADER_CG_SHOP_SEARCH_FILL_REQUEST,

		#ifdef ENABLE_SHOP_SEARCH_SYSTEM
		SUBHEADER_CG_SHOP_FILTER_NAME_REQUEST,
		#endif

		SUBHEADER_CG_SHOP_OFFER_CREATE,
		SUBHEADER_CG_SHOP_OFFER_ACCEPT,
		SUBHEADER_CG_SHOP_OFFER_CANCEL,
		SUBHEADER_CG_SHOP_REQUEST_OFFER_LIST,

		SUBHEADER_CG_SHOP_SAFEBOX_OPEN,
		SUBHEADER_CG_SHOP_SAFEBOX_GET_ITEM,
		SUBHEADER_CG_SHOP_SAFEBOX_GET_VALUTES,
		SUBHEADER_CG_SHOP_SAFEBOX_CLOSE,

		//AUCTION
		SUBHEADER_CG_AUCTION_LIST_REQUEST,
		SUBHEADER_CG_AUCTION_OPEN_REQUEST,
		SUBHEADER_CG_MY_AUCTION_OPEN_REQUEST,
		SUBHEADER_CG_MY_AUCTION_CLOSE,
		SUBHEADER_CG_MY_AUCTION_CANCEL,
		SUBHEADER_CG_CREATE_AUCTION,
		SUBHEADER_CG_AUCTION_ADD_OFFER,
		SUBHEADER_CG_EXIT_FROM_AUCTION,

		SUBHEADER_CG_CLOSE_MY_SHOP_BOARD,
		SUBHEADER_CG_CLOSE_OFFER_LIST,
		SUBHEADER_CG_CLOSE_SHOP_GUEST,

		#ifdef ENABLE_IKASHOP_ENTITIES
		SUBHEADER_CG_CLICK_ENTITY,
		SUBHEADER_CG_MOVE_SHOP_ENTITY,
		#endif

		#ifdef ENABLE_IKASHOP_AVG_AND_MINMAX_PRICE
		SUBHEADER_CG_SHOP_AVARAGE_AND_MINMAX_PRICE,
		#endif
	};

	struct TSubPacketCGShopCreate
	{
		TShopInfo shop{};
	};

	struct TSubPacketCGShopOpen 
	{
		DWORD ownerid;
	};

	struct TSubPacketCGAddItem 
	{
		TItemPos pos;
		TPriceInfo price;
		int destpos;
	};

	struct TSubPacketCGRemoveItem 
	{
		DWORD itemid;
	};

	struct TSubPacketCGEditItem
	{
		DWORD itemid;
		TPriceInfo price;
	};

	struct TSubPacketCGFilterRequest
	{
		TFilterInfo filter;
	};

	#ifdef ENABLE_SHOP_SEARCH_SYSTEM
	struct TSubPacketCGFilterNameRequest
	{
		TFilterName filter;
	};
	#endif

	struct TSubPacketCGOfferCreate
	{
		TOfferInfo offer;
	};

	struct TSubPacketCGOfferAccept
	{
		DWORD offerid;
	};

	struct TSubPacketCGOfferCancel
	{
		DWORD offerid;
		DWORD ownerid;
	};

	struct TSubPacketCGShopSafeboxGetItem
	{
		DWORD itemid;
	};

	struct TSubPacketCGShopBuyItem
	{
		DWORD ownerid;
		DWORD itemid;
		bool searching;
		long long seenprice;
	};

	//AUCTION
	struct TSubPacketCGAuctionOpenRequest
	{
		DWORD ownerid;
	};

	struct TSubPacketCGAuctionCreate
	{
		TItemPos pos;
		TPriceInfo startprice;
	};

	struct TSubPacketCGAuctionAddOffer 
	{
		DWORD ownerid;
		TPriceInfo price;
	};

	struct TSubPacketCGAuctionExitFrom
	{
		DWORD ownerid;
	};

	#ifdef ENABLE_IKASHOP_ENTITIES
	struct TSubPacketCGShopClickEntity
	{
		DWORD vid;
	};
	#endif

	#ifdef ENABLE_IKASHOP_AVG_AND_MINMAX_PRICE
	struct TPacketCGShopAvgAndMinMaxPrice
	{
		uint32_t vnum;
	};

	struct TSubPacketGCShopAvarageAndMinMaxPrice
	{
		long long avgPrice;
		long long minPrice;
		long long maxPrice;
	};
	#endif
}
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
enum
{
	ACCE_SUBHEADER_GC_OPEN = 0,
	ACCE_SUBHEADER_GC_CLOSE,
	ACCE_SUBHEADER_GC_ADDED,
	ACCE_SUBHEADER_GC_REMOVED,
	ACCE_SUBHEADER_CG_REFINED,
	ACCE_SUBHEADER_CG_CLOSE = 0,
	ACCE_SUBHEADER_CG_ADD,
	ACCE_SUBHEADER_CG_REMOVE,
	ACCE_SUBHEADER_CG_REFINE,
};

typedef struct SPacketAcce
{
	BYTE	header;
	BYTE	subheader;
	bool	bWindow;
	DWORD	dwPrice;
	BYTE	bPos;
	TItemPos	tPos;
	DWORD	dwItemVnum;
	DWORD	dwMinAbs;
	DWORD	dwMaxAbs;
} TPacketAcce;
#endif

#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
typedef struct SGCTargetItem
{
	DWORD	dwVnum;
	#ifdef ENABLE_NEW_MAX_COUNT
	short	iCount;
	#else
	BYTE	iCount;
	#endif
	long	lSocket;
} TGCTargetItem;

typedef struct packet_target_info
{
	BYTE	header;
	WORD	wSize;
	DWORD	dwVID;
	DWORD	race;
} TPacketGCTargetInfo;

typedef struct packet_target_info_load
{
	BYTE header;
	DWORD dwVID;
} TPacketCGTargetInfoLoad;
#endif

#ifdef ENABLE_SHOW_CHEST_DROP
enum EPacketCGChestDropInfo
{
	CDI_SUB_HEADER_INV,
	CDI_SUB_HEADER_VNUM,
};

typedef struct SPacketCGChestDropInfo
{
	BYTE	header;
	BYTE	subheader;
	int		arg;
} TPacketCGChestDropInfo;

typedef struct SChestDropInfoTable
{
	BYTE	bPageIndex;
	BYTE	bSlotIndex;
	DWORD	dwItemVnum;
	#ifdef ENABLE_NEW_MAX_COUNT
	short	bItemCount;
	#else
	BYTE	bItemCount;
	#endif
} TChestDropInfoTable;

typedef struct SPacketGCChestDropInfo
{
	BYTE	bHeader;
	WORD	wSize;
	DWORD	dwChestVnum;
} TPacketGCChestDropInfo;
#endif

#ifdef ENABLE_SWITCHBOT
struct TPacketGGSwitchbot
{
	BYTE bHeader;
	WORD wPort;
	TSwitchbotTable table;

	TPacketGGSwitchbot() : bHeader(HEADER_GG_SWITCHBOT), wPort(0)
	{
		table = {};
	}
};

enum ECGSwitchbotSubheader
{
	SUBHEADER_CG_SWITCHBOT_START,
	SUBHEADER_CG_SWITCHBOT_STOP,
};

struct TPacketCGSwitchbot
{
	BYTE header;
	int size;
	BYTE subheader;
	BYTE slot;
};

enum EGCSwitchbotSubheader
{
	SUBHEADER_GC_SWITCHBOT_UPDATE,
	SUBHEADER_GC_SWITCHBOT_UPDATE_ITEM,
	SUBHEADER_GC_SWITCHBOT_SEND_ATTRIBUTE_INFORMATION,
};

struct TPacketGCSwitchbot
{
	BYTE header;
	int size;
	BYTE subheader;
	BYTE slot;
};

struct TSwitchbotUpdateItem
{
	BYTE	slot;
	BYTE	vnum;
#ifdef ENABLE_NEW_MAX_COUNT
	short		count;
#else
	BYTE		count;
#endif
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
};
#endif

#ifdef ENABLE_FAST_OPEN_CHEST
typedef struct SInstantChestOpen
{
	BYTE	bHeader;
	TItemPos iPos;
}TInstantChestOpen;
#endif

#ifdef ENABLE_STYLE_ATTRIBUTE_SYSTEM
typedef struct command_item_new_attribute
{
	BYTE  header;
	TItemPos source_pos;
	TItemPos target_pos;
	BYTE bValues[3 + 1];
} TPacketCGItemNewAttribute;
#endif

#ifdef ENABLE_HIDE_COSTUME_PART
typedef struct SPacketCGChangeCostumeVisibilityStatus
{
	BYTE bHeader;
	BYTE type;
	bool status;
} TPacketCGChangeCostumeVisibilityStatus;

typedef struct SPacketGCUpdateCostumeVisibilityStatus
{
	BYTE bHeader;
	bool type[ECostumePartTypes::HIDE_COSTUME_COUNT];
} TPacketGCUpdateCostumeVisibilityStatus;
#endif
#ifdef ENABLE_FULL_YANG
typedef struct packet_gold
{
	BYTE	bHeader;
	uint64_t	lGold;
} TPacketGCGold;

typedef struct packet_gold_change
{
	BYTE	header;
	DWORD	dwVID;
	int64_t	ldAmount;
	uint64_t	ldValue;
} TPacketGCGoldChange;
#endif

#ifdef ENABLE_CUBE_RENEWAL
enum
{
	CUBE_RENEWAL_SUB_HEADER_OPEN_RECEIVE,
	CUBE_RENEWAL_SUB_HEADER_CLEAR_DATES_RECEIVE,
	CUBE_RENEWAL_SUB_HEADER_DATES_RECEIVE,
	CUBE_RENEWAL_SUB_HEADER_DATES_LOADING,
	#if defined(CUBE_RENEWAL_RELOAD_FIX)
	CUBE_RENEWAL_SUB_HEADER_DATES_RELOAD,
	#endif

	CUBE_RENEWAL_SUB_HEADER_MAKE_ITEM,
	CUBE_RENEWAL_SUB_HEADER_CLOSE,
};

typedef struct  packet_send_cube_renewal
{
	BYTE header;
	BYTE subheader;
	DWORD index_item;
	DWORD count_item;
	DWORD index_item_improve;
}TPacketCGCubeRenewalSend;

typedef struct dates_cube_renewal
{
	DWORD	npc_vnum;
	DWORD	index;

	DWORD	vnum_reward;
	int		count_reward;

	bool	item_reward_stackable;

	DWORD	vnum_material[CUBE_MATERIAL_MAX_NUM];
	int		count_material[CUBE_MATERIAL_MAX_NUM];

	#ifdef ENABLE_FULL_YANG
	int64_t	gold;
	#else
	int		gold;
	#endif

	int		percent;

	char	category[100];
}TInfoDateCubeRenewal;

typedef struct packet_receive_cube_renewal
{
	packet_receive_cube_renewal() : header(HEADER_GC_CUBE_RENEWAL)
	{}

	BYTE header;
	BYTE subheader;
	TInfoDateCubeRenewal	date_cube_renewal;
}TPacketGCCubeRenewalReceive;
#endif

#ifdef __RANKING_SYSTEM__
typedef struct SPacketRank
{
	BYTE	header;
	DWORD	size;
} TPacketRank;
#endif

#ifdef ENABLE_EVENT_MANAGER
typedef struct SPacketGCEventManager
{
	BYTE	header;
	DWORD	size;
} TPacketGCEventManager;
#endif

#ifdef ENABLE_SKILL_RENEWAL
typedef struct SPacketGCSkillBookCount
{
	BYTE header;
	BYTE readCount[6];
	BYTE leftCount[6];
} TPacketGCSkillBookCount;
#endif


#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
enum
{
	ITEMSHOP_PROCESS_NONE,
	ITEMSHOP_PROCESS_REFRESH,
	ITEMSHOP_PROCESS_BUY,
	#ifdef ENABLE_DRAGON_COIN_EM_MARKET
	ITEMSHOP_PROCESS_EM_REFRESH,
	#endif
};

enum EPacketGCDragonCoinSubHeaderType
{
	DC_SUB_HEADER_CATEGORYS,
	DC_SUB_HEADER_ITEMS,
	DC_SUB_HEADER_LOGS,
	DC_SUB_HEADER_POPUP_MESSAGE,
	#ifdef ENABLE_DRAGON_COIN_EM_MARKET
	DC_SUB_HEADER_EM_CATEGORYS,
	DC_SUB_HEADER_EM_ITEMS,
	#endif
};

enum
{
	ITEMSHOP_SELL_TYPE_ALL,
	ITEMSHOP_SELL_TYPE_COINS,
	ITEMSHOP_SELL_TYPE_CASH,
	#ifdef ENABLE_DRAGON_COIN_EM_MARKET
	ITEMSHOP_SELL_TYPE_EM,
	#endif
};

typedef struct packet_ishop_size
{
	BYTE	bHeader;
	#ifdef ENABLE_LARGE_DYNAMIC_PACKETS
	int		dataSize;
	#else
	WORD	dataSize;
	#endif
	BYTE	bSubHeader;
}TGCItemShopSize;

typedef struct SGCIShopCategory
{
	WORD	catID;
	char	categoryName[32+1];
	BYTE	mainID;
}TGCIShopCategory;

typedef struct SGCIShopItem
{
	DWORD	iID;
	DWORD	iVnum;
	INT		iCount;
	DWORD	iPrice;
	INT		iDiscount;
	WORD	categoryID;
	DWORD	dwAddTime;
	DWORD	dwOfferTime;
	BYTE	bTimeType;
	int		iStock;
	WORD	wSellCount;
	BYTE	bSellType;
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
}TGCIShopItem;

typedef struct SCGProcess
{
	BYTE	bHeader;
	BYTE	bSubHeader;
	DWORD	itemID;
	BYTE	itemCount;
	BYTE	itemPriceType;
	char	szCategoryStr[32+1];
}TCGItemShopProcess;

typedef struct SPacketGGDcShopRefresh
{
	BYTE	bHeader;
	BYTE	bProcess;
} TPacketGGDcShopRefresh;

typedef struct SPacketGGDcShopBuy
{
	BYTE	bHeader;
	DWORD	dwAID;
} TPacketGGDcShopBuy;

struct TItemShopPopupMessage
{
	static constexpr inline auto MESSAGE_LEN = 80;
	char localeString[MESSAGE_LEN];
	int iData;
};
#endif

#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
enum
{
	ITEMSHOP_PROCESS_ADMIN_PAGE_OPEN,
	ITEMSHOP_PROCESS_CATE_ADD,
	ITEMSHOP_PROCESS_SUBCATE_ADD,
	ITEMSHOP_PROCESS_CATE_DEL,
	ITEMSHOP_PROCESS_SUBCATE_DEL,
	ITEMSHOP_PROCESS_RENAME,
	ITEMSHOP_PROCESS_ITEM_DEL,
	ITEMSHOP_PROCESS_ITEM_ADD,
	ITEMSHOP_PROCESS_ITEM_UPDATE,
};

enum EPacketGCDragonCoinAdminSubHeaderType
{
	DC_SUB_HEADER_ADMIN_CATEGORYS,
	DC_SUB_HEADER_ADMIN_ITEMS,
};

typedef struct SCGAdminProcess
{
	BYTE	header;
	BYTE	subheader;
} TCGItemShopAdminProcess;

typedef struct SGCIShopCateAdmin
{
	int		iCateID;
	char	szCategory[32+1];
}TGCIShopCateAdmin;

typedef struct SGCIShopItemAdmin
{
	DWORD	iID;
	DWORD	iVnum;
	INT		iCount;
	DWORD	iPrice;
	INT		iDiscount;
	WORD	categoryID;
	DWORD	dwAddTime;
	DWORD	dwOfferTime;
	char	szAddTime[32+1];
	char	szOfferTime[32+1];
	BYTE	bTimeType;
	BYTE	bStatus;
	int		iStock;
	WORD	wSellCount;
	BYTE	bSellType;
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
}TGCIShopItemAdmin;
#endif

#ifdef ENABLE_MULTI_FARM_BLOCK
enum
{
	MULTI_FARM_SET,
	MULTI_FARM_REMOVE,
};

typedef struct SPacketGGMultiFarm
{
	BYTE	header;
	DWORD	size;
	BYTE	subHeader;
	char	playerIP[IP_ADDRESS_LENGTH + 1];
	DWORD	playerID;
	char	playerName[CHARACTER_NAME_MAX_LEN+1];
	bool	farmStatus;
	BYTE	affectType;
	long	affectTime;
} TPacketGGMultiFarm;
#endif

#ifdef ENABLE_MULTI_HUNT_BLOCK
enum
{
	MULTI_HUNT_SET,
	MULTI_HUNT_REMOVE,
};

typedef struct SPacketGGMultiHunt
{
	BYTE	header;
	DWORD	size;
	BYTE	subHeader;
	char	playerIP[IP_ADDRESS_LENGTH + 1];
	DWORD	playerID;
	char	playerName[CHARACTER_NAME_MAX_LEN+1];
	bool	huntStatus;
	BYTE	affectType;
	long	affectTime;
} TPacketGGMultiHunt;
#endif

#ifdef ENABLE_MULTI_FISH_BLOCK
enum
{
	MULTI_FISH_SET,
	MULTI_FISH_REMOVE,
};

typedef struct SPacketGGMultiFish
{
	BYTE	header;
	DWORD	size;
	BYTE	subHeader;
	char	playerIP[IP_ADDRESS_LENGTH + 1];
	DWORD	playerID;
	char	playerName[CHARACTER_NAME_MAX_LEN+1];
	bool	fishStatus;
	BYTE	affectType;
	long	affectTime;
} TPacketGGMultiFish;
#endif

#ifdef ENABLE_PM_ALL_SEND_SYSTEM
typedef struct SPacketCGBulkWhisper
{
	BYTE	header;
	char	szText[512 + 1];
} TPacketCGBulkWhisper;

typedef struct packet_bulk_whisper
{
	BYTE	header;
	WORD	size;
} TPacketGCBulkWhisper;

typedef struct SPacketGGBulkWhisper
{
	BYTE	bHeader;
	long	lSize;
} TPacketGGBulkWhisper;
#endif

#ifdef ENABLE_BAKIM_DUYURU
typedef struct SPacketGCMaintenanceInfo
{
	BYTE header;
	WORD size;
	DWORD remaining_time;
	BYTE cause_len;
	DWORD duration;
} TPacketGCMaintenanceInfo;

typedef struct SServerMaintenanceInfo
{
	SServerMaintenanceInfo()
	{
		shutdown_time = 0;
		cause[0] = '\0';
		duration = 0;
	}

	DWORD shutdown_time;
	char cause[100];
	DWORD duration;
	DWORD end_time;
} TServerMaintenanceInfo;

typedef struct SPacketGGMaintenanceInfo
{
	BYTE header;
	TServerMaintenanceInfo info;
} TPacketGGMaintenanceInfo;
#endif

#ifdef ENABLE_OFFER_SYSTEM
typedef struct packet_cg_offer
{
	BYTE	bHeader;
	BYTE	reqID;
	DWORD	offerID;
	DWORD	offerOldPrice;
	DWORD	offerNewPrice;
}TCGOfferRequest;

typedef struct packet_offer_size
{
	BYTE	bHeader;
	WORD	dataSize;
	BYTE	dataType;
}TGCOfferListSize;
#endif

#ifdef ENABLE_EXCHANGE_LOG
typedef struct SPacketGCExchangeLog
{
	BYTE	header;
	#ifdef ENABLE_LARGE_DYNAMIC_PACKETS
	int		size;
	#else
	WORD	size;
	#endif
} TPacketGCExchangeLog;

enum
{
	SUB_EXCHANGELOG_LOAD,
	SUB_EXCHANGELOG_LOAD_ITEM,
};
#endif

#ifdef ENABLE_ENTITY_PRELOADING
typedef struct packet_preload_entities
{
	uint8_t header;
	uint16_t size;
	uint16_t count;
} TPacketGCPreloadEntities;
#endif

#ifdef ENABLE_WHEEL_OF_FORTUNE_SYSTEM
enum EWheelOfFortuneTypes
{
	WHEEL_TYPE_NONE,
	//-------------//
	WHEEL_TYPE_EP,
	WHEEL_TYPE_MP,
	//-------------//
	WHEEL_TYPE_BRONZE_COIN,
	WHEEL_TYPE_SILVER_COIN,
	WHEEL_TYPE_GOLD_COIN,
	WHEEL_TYPE_EMERALD_COIN,
	//-------------//
	WHEEL_TYPE_BRONZE_YANG,
	WHEEL_TYPE_SILVER_YANG,
	WHEEL_TYPE_GOLD_YANG,
	WHEEL_TYPE_EMERALD_YANG,
	//-------------//
	WHEEL_TYPE_MAX_NUM,
};

typedef struct SWheelOfFortuneInfoTable
{
	int		iVnum;
	int		iCount;
} TWheelOfFortuneInfoTable;

typedef struct SPacketGCWheelOfFortune
{
	BYTE	bHeader;
	WORD	wSize;
	BYTE	bSubHeader;
	int		iNeedVnum;
	int		iNeedCount;
} TPacketGCWheelOfFortune;
#endif

#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
typedef struct SPacketGGGuildWarOption
{
	BYTE	bHeader;
	DWORD	dwGuild1;
	DWORD	dwGuild2;
	int		iScore;
	int		iMinLevel;
	int		iMaxPlayer;
	bool	bWarrior;
	bool	bAssassin;
	bool	bSura;
	bool	bShaman;
} TPacketGGGuildWarOption;

typedef struct SPacketGGGuildWarMember
{
	BYTE	bHeader;
	DWORD	dwGuild;
	int		iMemCount;
} TPacketGGGuildWarMember;
#endif

#ifdef ENABLE_BATTLE_PASS
typedef struct SPacketGCBattlePass
{
	uint8_t bHeader;
	#ifdef ENABLE_LARGE_DYNAMIC_PACKETS
	int32_t wSize;
	#else
	uint16_t wSize;
	#endif
	uint16_t wRewardSize;
	char battlePassName[64];
} TPacketGCBattlePass;

typedef struct SPacketGCBattlePassUpdate
{
	uint8_t bHeader;
	uint8_t bMissionID;
	uint32_t dwNewProgress;
} TPacketGCBattlePassUpdate;

typedef struct SPacketCGBattlePassAction
{
	uint8_t bHeader;
	uint8_t bAction;
	int32_t iArg;
} TPacketCGBattlePassAction;
#endif

//Global Dynamic Function
typedef struct SDynamicPacketInfo
{
	BYTE	bHeader;
	WORD	wSize;
	BYTE	bSubHeader;
} TDynamicPacketInfo;
//Global Dynamic Function

#ifdef ENABLE_TANAKA_EVENT
enum ETanakaGCPacketSubheaders
{
	TANAKA_EVENT_SUBHEADER_GC_OPEN_UI,
	TANAKA_EVENT_SUBHEADER_GC_UPDATE,
	TANAKA_EVENT_SUBHEADER_GC_UPDATE_PLAYER_INFO,
	TANAKA_EVENT_SUBHEADER_GC_UPDATE_ITEM_INFO,
};

typedef struct STanakaGCUpdate
{
	uint8_t eventLevel;
	uint8_t tanakaCount;
	uint8_t leftTanakaCount;
} TTanakaGCUpdate;

typedef struct STanakaGCPlayerInfo
{
	unsigned long totalDamage;
} TTanakaGCPlayerInfo;

typedef struct STanakaGCUpdateItemInfo
{
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	uint32_t vnum;
	uint8_t count;
} TTanakaGCUpdateItemInfo;

typedef struct STanakaGGEventState
{
	BYTE	bHeader;
	uint8_t eventState;
} TTanakaGGEventState;
#endif

#ifdef ENABLE_TELEPORTION_WIZARD_SYSTEM
enum
{
	TP_WIZARD_DATA_INFO,
	TP_WIZARD_DROP_INFO,
};

typedef struct SWizardDataInfoTable
{
	WORD	id;
	BYTE	type;
	char	comment[128+1];
	BYTE	min_level;
	BYTE	max_level;
	int		alignment;
	DWORD	item_vnum;
	DWORD	item_count;
	int		image_num;
	DWORD	race_num;
	BYTE	element;
	long	mapIndex;
	long	coor_x;
	long	coor_y;
	int		cooldown;
	BYTE	status;
	BYTE	show;
	#ifdef ENABLE_FULL_YANG
	int64_t	gold;
	#else
	long	gold;
	#endif
} TWizardDataInfoTable;

typedef struct SWizardDropInfoTable
{
	BYTE		bPageIndex;
	BYTE		bSlotIndex;
	DWORD		dwItemVnum;
	int			iItemCount;
} TWizardDropInfoTable;

typedef struct SPacketGCTpWizardInfo
{
	BYTE	bHeader;
	WORD	wSize;
	BYTE	bSubHeader;
	DWORD	dwRaceNum;
} TPacketGCTpWizardInfo;
#endif

#ifdef ENABLE_CHANGELOOK_SYSTEM
enum
{
	CL_SUBHEADER_OPEN = 0,
	CL_SUBHEADER_CLOSE,
	CL_SUBHEADER_ADD,
	CL_SUBHEADER_REMOVE,
	CL_SUBHEADER_REFINE,
};

typedef struct SPacketChangeLook
{
	BYTE		header;
	BYTE		subheader;
	#ifdef ENABLE_FULL_YANG
	int64_t		dwCost;
	#else
	DWORD		dwCost;
	#endif
	BYTE		bPos;
	TItemPos	tPos;
} TPacketChangeLook;
#endif

#ifdef __BACK_DUNGEON__
typedef struct SPacketGGBackDungeon
{
	BYTE	header;
	bool	bIsSave;
	WORD	wPort;
	DWORD	dwMapIdx, dwPID;
	long	lX, lY;
	int		iLeftTime;
	BYTE	bChannel;
} TPacketGGBackDungeon;
#endif

#ifdef BOSS_DMG_RANKING
typedef struct SPacketGCDamageRanking
{
	BYTE		bHeader;
	char		szNames[CHARACTER_NAME_MAX_LEN + 1][MAX_PLAYERS_IN_TABLE];
	BYTE		bJobs[MAX_PLAYERS_IN_TABLE];
	BYTE		bEmpire[MAX_PLAYERS_IN_TABLE];
	BYTE		bLevels[MAX_PLAYERS_IN_TABLE];
	long long	lDamages[MAX_PLAYERS_IN_TABLE];
	float		fPercents[MAX_PLAYERS_IN_TABLE];
}TPacketGCDamageRanking;
#endif

typedef struct packet_affect_reset
{
	BYTE	header;
	DWORD	dwVID;
	DWORD	dwAffectFlag[2];
} TPacketGCAffectReset;

#ifdef ENABLE_DISCONNECT_ALL_PLAYERS
typedef struct SPacketGGDisconnectAll
{
	BYTE	bHeader;
	BYTE	bProcess;
} TPacketGGDisconnectAll;
#endif

#ifdef ENABLE_BLACK_MARKET_SYSTEM
enum
{
	BM_SUBHEADER_OPEN,
	BM_SUBHEADER_SELL,
};

enum
{
	BM_SUBHEADER_RELOAD,
	BM_SUBHEADER_REFRESH,
};

typedef struct command_item_blackmarket
{
	BYTE		header;
	BYTE		subHeader;
	TItemPos	Cell;
} TPacketCGItemBlackMarket;

typedef struct SPacketGCItemBlackMarket
{
	BYTE		bHeader;
	WORD		wSize;
} TPacketGCItemBlackMarket;

typedef struct SBlackMarketData
{
	DWORD		dwVnum;
	#ifdef ENABLE_FULL_YANG
	int64_t		llPrice;
	#else
	long		llPrice;
	#endif
	int			iStock;
} TBlackMarketData;

typedef struct SPacketBlackMarket
{
	BYTE		bHeader;
	BYTE		bSubHeader;
	DWORD		dwVnum;
	int			iStock;
} TPacketGGBlackMarket;
#endif

#ifdef ENABLE_COLLECTIVE_RELOAD
typedef struct SPacketGGCollectiveReload
{
	SPacketGGCollectiveReload() : bHeader(HEADER_GG_COLLECTIVE_RELOAD) {}
	BYTE	bHeader;
	BYTE	bSubHeader;
} TPacketGGCollectiveReload;
#endif

#ifdef ENABLE_DC_TO_CHR_OTHER_CH
typedef struct SPacketGGDcToChr
{
	SPacketGGDcToChr() : header(HEADER_GG_DC_TO_CHR){}
	BYTE header;
	char szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGGDcToChr;
#endif

#ifdef ENABLE_REWARD_SYSTEM
#include <vector>

typedef struct reward_infos
{
	BYTE rewardIndex;
	BYTE deskIndex;
	char lc_text[100];
	char playerName[CHARACTER_NAME_MAX_LEN+1];
	DWORD value;
	DWORD count;
	std::vector<std::pair<DWORD, DWORD>> m_rewardItems;
} TRewardInfo;

typedef struct SPacketGCRewardMain
{
	BYTE bHeader;
	WORD wSize;
} TPacketGCRewardMain;

typedef struct SPacketRewardInfo
{
	BYTE deskIndex;
	char playerName[CHARACTER_NAME_MAX_LEN+1];
} TPacketRewardInfo;

enum
{
	REWARD_NONE						= 0,
	REWARD_LEVEL					= 1,
	REWARD_REFINE					= 2,
	REWARD_CRAFT					= 3,
	REWARD_AVERAGE					= 4,
	REWARD_BIOLOG					= 5,
	REWARD_ALIGNMENT				= 6,
	REWARD_PASSIVE_SKILL			= 7,

	REWARD_STONE_DEAD				= 8,
	REWARD_BOSS_DEAD				= 9,

	REWARD_STONE_ALL_DEAD			= 10,
	REWARD_BOSS_ALL_DEAD			= 11,

	REWARD_MAX,
};

typedef struct SPacketGGRewardInfo
{
	BYTE	bHeader;
	BYTE	rewardIndex;
	char	playerName[CHARACTER_NAME_MAX_LEN+1];
	DWORD	value;
} TPacketGGRewardInfo;
#endif

typedef struct SPacketGGDisconnectPlayer
{
	BYTE	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	int		iSecond;
} TPacketGGDisconnectPlayer;

#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
enum
{
	GUILD_STATISTICS_ADD,
	GUILD_STATISTICS_UPDATE,
};

typedef struct SPacketGCGuildStaticsInfo
{
	SPacketGCGuildStaticsInfo() : bHeader(HEADER_GC_GUILD_STATISTICS), bSubHeader(GUILD_STATISTICS_ADD) {}
	BYTE	bHeader;
	BYTE	bSubHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	unsigned long long ullDamage;
	BYTE	bRank;
	WORD	wRace;
	BYTE	bKill;
	BYTE	bDead;
	BYTE	bAssist;
} TPacketGCGuildStaticsInfo;
#endif

#if defined(__BL_LUCKY_BOX__)
enum ELUCKY_BOX_ACTION
{
	LUCKY_BOX_ACTION_RETRY,
	LUCKY_BOX_ACTION_RECEIVE,
};

typedef struct SPacketCGLuckyBox
{
	BYTE		header;
	BYTE		action;
} TPacketCGLuckyBox;

typedef struct SPacketGCLuckyBox
{
	BYTE		header;
	DWORD		vnum;
	#ifdef ENABLE_NEW_MAX_COUNT
	short		count;
	#else
	BYTE		count;
	#endif
	#ifdef ENABLE_FULL_YANG
	int64_t		need_money;
	#else
	int			need_money;
	#endif
} TPacketGCLuckyBox;
#endif

#ifdef ENABLE_PUBLISHER_SYSTEM
typedef struct SPublisherInfo
{
	int		iOrder;
	char	szTitle[128];
	char	szLink[256];
} TPublisherInfo;

typedef struct SPacketGCPublisherInfo
{
	BYTE	bHeader;
	WORD	wSize;
} TPacketGCPublisherInfo;
#endif

#ifdef ENABLE_RONARK_SYSTEM
typedef struct SRonarkTeamData
{
	DWORD	dwID;
	DWORD	dwDamage;
	int		iJoinerCount;
	int		iMemberCount;
	int64_t	iRonarkHP;
} TRonarkTeamData;

typedef struct SPacketGCRonarkInfo
{
	BYTE	bHeader;
	WORD	wSize;
} TPacketGCRonarkInfo;

typedef struct SPacketGGRonark
{
	SPacketGGRonark() : bHeader(HEADER_GG_RONARK), bProcess(0), dwPID(0){}
	BYTE	bHeader;
	BYTE	bProcess;
	DWORD	dwPID;
} TPacketGGRonark;
#endif

#ifdef ENABLE_AUTO_ITEM_SELL_SYSTEM
enum EAUTO_ITEM_SELL_SYSTEM
{
	SUBHEADER_CG_AUTO_SELL_OPEN,
	SUBHEADER_CG_AUTO_SELL_ADD,
	SUBHEADER_CG_AUTO_SELL_REMOVE,
	SUBHEADER_CG_AUTO_SELL_RESET,
	SUBHEADER_CG_AUTO_SELL_ACTIVE,
	SUBHEADER_CG_AUTO_SELL_PASSIVE,
};

typedef struct SPacketCGAutoItemSellData
{
	BYTE	header;
	BYTE	subHeader;
	DWORD	value;
} TPacketCGAutoItemSellData;

typedef struct SPacketGCAutoItemSellData
{
	BYTE	header;
	WORD	size;
	WORD	itemCount;
	bool	status;
} TPacketGCAutoItemSellData;
#endif
#ifdef SPIN_WHEEL

typedef struct SPacketGCSpinWheel
{
	BYTE bHeader;
	DWORD itemVnums[spinWheelMaxItem];
	DWORD itemCounts[spinWheelMaxItem];
	DWORD itemRarities[spinWheelMaxItem];
	DWORD dwRewardItem;
}TPacketGCSpinWheel;

typedef struct SPacketGCSpinWheelRewards
{
	BYTE bHeader;
	BYTE bIndex;
	DWORD itemVnums[spinWheelMaxItem];
	DWORD itemCounts[spinWheelMaxItem];
	DWORD itemRarities[spinWheelMaxItem];
}TPacketGCSpinWheelRewards;
#endif

#pragma pack()
#endif
