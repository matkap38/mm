#ifndef __INC_METIN_II_CHAR_H__
#define __INC_METIN_II_CHAR_H__

#include <boost/unordered_map.hpp>
#include "../../common/stl.h"
#include "entity.h"
#include "FSM.h"
#include "horse_rider.h"
#include "vid.h"
#include "constants.h"
#include "affect.h"
#include "affect_flag.h"
#ifndef ENABLE_CUBE_RENEWAL
#include "cube.h"
#else
#include "cuberenewal.h"
#endif
#include "mining.h"
#include "../../common/CommonDefines.h"

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
#include <vector>
#endif

#if defined(__BL_LUCKY_BOX__)
#include "item_manager.h"
#endif

#define ENABLE_ANTI_CMD_FLOOD
#define ENABLE_OPEN_SHOP_WITH_ARMOR
// #define ENABLE_SKILL_COOLDOWN_CHECK

enum eMountType {MOUNT_TYPE_NONE=0, MOUNT_TYPE_NORMAL=1, MOUNT_TYPE_COMBAT=2, MOUNT_TYPE_MILITARY=3};
eMountType GetMountLevelByVnum(DWORD dwMountVnum, bool IsNew);
const DWORD GetRandomSkillVnum(BYTE bJob = JOB_MAX_NUM);

class CBuffOnAttributes;
class CPetSystem;

#ifdef ENABLE_IKASHOP_RENEWAL
namespace ikashop
{
	class CShop;
	class CShopSafebox;
	class CAuction;
}
#endif

#define INSTANT_FLAG_DEATH_PENALTY		(1 << 0)
#define INSTANT_FLAG_SHOP			(1 << 1)
#define INSTANT_FLAG_EXCHANGE			(1 << 2)
#define INSTANT_FLAG_STUN			(1 << 3)
#define INSTANT_FLAG_NO_REWARD			(1 << 4)

#define AI_FLAG_NPC				(1 << 0)
#define AI_FLAG_AGGRESSIVE			(1 << 1)
#define AI_FLAG_HELPER				(1 << 2)
#define AI_FLAG_STAYZONE			(1 << 3)

#define SET_OVER_TIME(ch, time)	(ch)->SetOverTime(time)

#ifdef ENABLE_OKEY_CARD_SYSTEM
	#define MAX_CARDS_IN_HAND			5
	#define MAX_CARDS_IN_FIELD			3
#endif

extern int g_nPortalLimitTime;

enum
{
	MAIN_RACE_WARRIOR_M,
	MAIN_RACE_ASSASSIN_W,
	MAIN_RACE_SURA_M,
	MAIN_RACE_SHAMAN_W,
	MAIN_RACE_WARRIOR_W,
	MAIN_RACE_ASSASSIN_M,
	MAIN_RACE_SURA_W,
	MAIN_RACE_SHAMAN_M,
	#ifdef ENABLE_WOLFMAN_CHARACTER
	MAIN_RACE_WOLFMAN_M,
	#endif
	MAIN_RACE_MAX_NUM,
};

enum
{
	POISON_LENGTH = 30,
	#ifdef ENABLE_WOLFMAN_CHARACTER
	BLEEDING_LENGTH = 30,
	#endif
	STAMINA_PER_STEP = 1,
	SAFEBOX_PAGE_SIZE = 9,
	AI_CHANGE_ATTACK_POISITION_TIME_NEAR = 10000,
	AI_CHANGE_ATTACK_POISITION_TIME_FAR = 1000,
	AI_CHANGE_ATTACK_POISITION_DISTANCE = 100,
	SUMMON_MONSTER_COUNT = 3,
};

enum
{
	FLY_NONE,
	FLY_EXP,
	FLY_HP_MEDIUM,
	FLY_HP_BIG,
	FLY_SP_SMALL,
	FLY_SP_MEDIUM,
	FLY_SP_BIG,
	FLY_FIREWORK1,
	FLY_FIREWORK2,
	FLY_FIREWORK3,
	FLY_FIREWORK4,
	FLY_FIREWORK5,
	FLY_FIREWORK6,
	FLY_FIREWORK_CHRISTMAS,
	FLY_CHAIN_LIGHTNING,
	FLY_HP_SMALL,
	FLY_SKILL_MUYEONG,
};

enum EDamageType
{
	DAMAGE_TYPE_NONE,
	DAMAGE_TYPE_NORMAL,
	DAMAGE_TYPE_NORMAL_RANGE,
	DAMAGE_TYPE_MELEE,
	DAMAGE_TYPE_RANGE,
	DAMAGE_TYPE_FIRE,
	DAMAGE_TYPE_ICE,
	DAMAGE_TYPE_ELEC,
	DAMAGE_TYPE_MAGIC,
	DAMAGE_TYPE_POISON,
	DAMAGE_TYPE_SPECIAL,
	#ifdef ENABLE_WOLFMAN_CHARACTER
	DAMAGE_TYPE_BLEEDING,
	#endif
};

enum DamageFlag
{
	DAMAGE_NORMAL	= (1 << 0),
	DAMAGE_POISON	= (1 << 1),
	DAMAGE_DODGE	= (1 << 2),
	DAMAGE_BLOCK	= (1 << 3),
	DAMAGE_PENETRATE= (1 << 4),
	DAMAGE_CRITICAL = (1 << 5),
	#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_MOB_BLEEDING_AS_POISON)
	DAMAGE_BLEEDING	= (1 << 6),
	#endif
};

enum EPointTypes
{
	POINT_NONE,						// 0
	POINT_LEVEL,					// 1
	POINT_VOICE,					// 2
	POINT_EXP,						// 3
	POINT_NEXT_EXP,					// 4
	POINT_HP,						// 5
	POINT_MAX_HP,					// 6
	POINT_SP,						// 7
	POINT_MAX_SP,					// 8
	POINT_STAMINA,					// 9
	POINT_MAX_STAMINA,				// 10

	POINT_GOLD,						// 11
	POINT_ST,						// 12
	POINT_HT,						// 13
	POINT_DX,						// 14
	POINT_IQ,						// 15
	POINT_DEF_GRADE,				// 16 ...
	POINT_ATT_SPEED,				// 17
	POINT_ATT_GRADE,				// 18
	POINT_MOV_SPEED,				// 19
	POINT_CLIENT_DEF_GRADE,			// 20
	POINT_CASTING_SPEED,			// 21
	POINT_MAGIC_ATT_GRADE,			// 22
	POINT_MAGIC_DEF_GRADE,			// 23
	POINT_EMPIRE_POINT,				// 24
	POINT_LEVEL_STEP,				// 25
	POINT_STAT,						// 26
	POINT_SUB_SKILL,				// 27
	POINT_SKILL,					// 28
	POINT_WEAPON_MIN,				// 29
	POINT_WEAPON_MAX,				// 30
	POINT_PLAYTIME,					// 31
	POINT_HP_REGEN,					// 32
	POINT_SP_REGEN,					// 33

	POINT_BOW_DISTANCE,				// 34

	POINT_HP_RECOVERY,				// 35
	POINT_SP_RECOVERY,				// 36

	POINT_POISON_PCT,				// 37
	POINT_STUN_PCT,					// 38
	POINT_SLOW_PCT,					// 39
	POINT_CRITICAL_PCT,				// 40
	POINT_PENETRATE_PCT,			// 41
	POINT_CURSE_PCT,				// 42

	POINT_ATTBONUS_HUMAN,			// 43
	POINT_ATTBONUS_ANIMAL,			// 44
	POINT_ATTBONUS_ORC,				// 45
	POINT_ATTBONUS_MILGYO,			// 46
	POINT_ATTBONUS_UNDEAD,			// 47
	POINT_ATTBONUS_DEVIL,			// 48
	POINT_ATTBONUS_INSECT,			// 49
	POINT_ATTBONUS_FIRE,			// 50
	POINT_ATTBONUS_ICE,				// 51
	POINT_ATTBONUS_DESERT,			// 52
	POINT_ATTBONUS_MONSTER,			// 53
	POINT_ATTBONUS_WARRIOR,			// 54
	POINT_ATTBONUS_ASSASSIN,		// 55
	POINT_ATTBONUS_SURA,			// 56
	POINT_ATTBONUS_SHAMAN,			// 57
	POINT_ATTBONUS_TREE,			// 58

	POINT_RESIST_WARRIOR,			// 59
	POINT_RESIST_ASSASSIN,			// 60
	POINT_RESIST_SURA,				// 61
	POINT_RESIST_SHAMAN,			// 62

	POINT_STEAL_HP,					// 63
	POINT_STEAL_SP,					// 64

	POINT_MANA_BURN_PCT,			// 65
	POINT_DAMAGE_SP_RECOVER,		// 66

	POINT_BLOCK,					// 67
	POINT_DODGE,					// 68

	POINT_RESIST_SWORD,				// 69
	POINT_RESIST_TWOHAND,			// 70
	POINT_RESIST_DAGGER,			// 71
	POINT_RESIST_BELL,				// 72
	POINT_RESIST_FAN,				// 73
	POINT_RESIST_BOW,				// 74
	POINT_RESIST_FIRE,				// 75
	POINT_RESIST_ELEC,				// 76
	POINT_RESIST_MAGIC,				// 77
	POINT_RESIST_WIND,				// 78

	POINT_REFLECT_MELEE,			// 79

	POINT_REFLECT_CURSE,			// 80
	POINT_POISON_REDUCE,			// 81

	POINT_KILL_SP_RECOVER,			// 82
	POINT_EXP_DOUBLE_BONUS,			// 83
	POINT_GOLD_DOUBLE_BONUS,		// 84
	POINT_ITEM_DROP_BONUS,			// 85

	POINT_POTION_BONUS,				// 86
	POINT_KILL_HP_RECOVERY,			// 87

	POINT_IMMUNE_STUN,				// 88
	POINT_IMMUNE_SLOW,				// 89
	POINT_IMMUNE_FALL,				// 90
	//////////////////

	POINT_PARTY_ATTACKER_BONUS,		// 91
	POINT_PARTY_TANKER_BONUS,		// 92

	POINT_ATT_BONUS,				// 93
	POINT_DEF_BONUS,				// 94

	POINT_ATT_GRADE_BONUS,			// 95
	POINT_DEF_GRADE_BONUS,			// 96
	POINT_MAGIC_ATT_GRADE_BONUS,	// 97
	POINT_MAGIC_DEF_GRADE_BONUS,	// 98

	POINT_RESIST_NORMAL_DAMAGE,		// 99

	POINT_HIT_HP_RECOVERY,			// 100
	POINT_HIT_SP_RECOVERY,			// 101
	POINT_MANASHIELD,				// 102

	POINT_PARTY_BUFFER_BONUS,		// 103
	POINT_PARTY_SKILL_MASTER_BONUS,	// 104

	POINT_HP_RECOVER_CONTINUE,		// 105
	POINT_SP_RECOVER_CONTINUE,		// 106

	POINT_STEAL_GOLD,				// 107
	POINT_POLYMORPH,				// 108
	POINT_MOUNT,					// 109

	POINT_PARTY_HASTE_BONUS,		// 110
	POINT_PARTY_DEFENDER_BONUS,		// 111
	POINT_STAT_RESET_COUNT,			// 112

	POINT_HORSE_SKILL,				// 113

	POINT_MALL_ATTBONUS,			// 114
	POINT_MALL_DEFBONUS,			// 115
	POINT_MALL_EXPBONUS,			// 116
	POINT_MALL_ITEMBONUS,			// 117
	POINT_MALL_GOLDBONUS,			// 118

	POINT_MAX_HP_PCT,				// 119
	POINT_MAX_SP_PCT,				// 120

	POINT_SKILL_DAMAGE_BONUS,		// 121
	POINT_NORMAL_HIT_DAMAGE_BONUS,	// 122

	// DEFEND_BONUS_ATTRIBUTES
	POINT_SKILL_DEFEND_BONUS,		// 123
	POINT_NORMAL_HIT_DEFEND_BONUS,	// 124
	// END_OF_DEFEND_BONUS_ATTRIBUTES

	POINT_PC_BANG_EXP_BONUS,		// 125
	POINT_PC_BANG_DROP_BONUS,		// 126

	POINT_RAMADAN_CANDY_BONUS_EXP,	// 127

	POINT_ENERGY					= 128,

	POINT_ENERGY_END_TIME			= 129,

	POINT_COSTUME_ATTR_BONUS		= 130,
	POINT_MAGIC_ATT_BONUS_PER		= 131,
	POINT_MELEE_MAGIC_ATT_BONUS_PER	= 132,

	POINT_RESIST_ICE				= 133,
	POINT_RESIST_EARTH				= 134,
	POINT_RESIST_DARK				= 135,

	POINT_RESIST_CRITICAL			= 136,
	POINT_RESIST_PENETRATE			= 137,

	#ifdef ENABLE_WOLFMAN_CHARACTER
	POINT_BLEEDING_REDUCE			= 138,
	POINT_BLEEDING_PCT				= 139,

	POINT_ATTBONUS_WOLFMAN			= 140,
	POINT_RESIST_WOLFMAN			= 141,
	POINT_RESIST_CLAW				= 142,
	#endif

	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	POINT_ACCEDRAIN_RATE			= 143,
	#endif

	#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	POINT_RESIST_MAGIC_REDUCTION	= 144,
	#endif

	POINT_ENCHANT_ELECT				= 145,
	POINT_ENCHANT_FIRE				= 146,
	POINT_ENCHANT_ICE				= 147,
	POINT_ENCHANT_WIND				= 148,
	POINT_ENCHANT_EARTH				= 149,
	POINT_ENCHANT_DARK				= 150,

	#ifdef ENABLE_OTO_TOPLAMA
	POINT_OTO_METIN					= 151,
	POINT_OTO_CANAVAR				= 152,
	POINT_OTO_PATRON				= 153,
	#endif

	#ifdef ENABLE_ANTI_EXP
	POINT_ANTI_EXP					= 154,
	#endif

	#ifdef ENABLE_NEW_ATTRIBUTE_TYPES
	POINT_ATTBONUS_STONE			= 155,
	POINT_ATTBONUS_BOSS				= 156,
	POINT_RESIST_HUMAN				= 157,
	POINT_RESIST_MONSTER			= 158,
	#endif

	// POINT_NONE					= 159,
	// POINT_NONE					= 160,

	#ifdef ENABLE_BATTLE_PASS
	POINT_BATTLE_PASS_ID			= 161,
	#endif

	#ifdef ENABLE_PVM_BONUS
	POINT_SKILL_DAMAGE_BONUS_PVM	= 162,
	POINT_HIT_DAMAGE_BONUS_PVM		= 163,
	POINT_ATT_GRADE_BONUS_PVM		= 164,
	POINT_DEF_GRADE_BONUS_PVM		= 165,
	POINT_BERSERKER_BONUS_PVM		= 166,
	#endif

	POINT_MAX,
};

enum EPKModes
{
	PK_MODE_PEACE,
	PK_MODE_REVENGE,
	PK_MODE_FREE,
	PK_MODE_PROTECT,
	PK_MODE_GUILD,
	PK_MODE_MAX_NUM
};

enum EPositions
{
	POS_DEAD,
	POS_SLEEPING,
	POS_RESTING,
	POS_SITTING,
	POS_FISHING,
	POS_FIGHTING,
	POS_MOUNTING,
	POS_STANDING
};

enum EBlockAction
{
	BLOCK_EXCHANGE			= (1 << 0),
	BLOCK_PARTY_INVITE		= (1 << 1),
	BLOCK_GUILD_INVITE		= (1 << 2),
	BLOCK_WHISPER			= (1 << 3),
	BLOCK_MESSENGER_INVITE	= (1 << 4),
	BLOCK_PARTY_REQUEST		= (1 << 5),
};

// <Factor> Dynamically evaluated CHARACTER* equivalent.
// Referring to SCharDeadEventInfo.
struct DynamicCharacterPtr {
	DynamicCharacterPtr() : is_pc(false), id(0) {}
	DynamicCharacterPtr(const DynamicCharacterPtr& o)
		: is_pc(o.is_pc), id(o.id) {}

	// Returns the LPCHARACTER found in CHARACTER_MANAGER.
	LPCHARACTER Get() const;
	// Clears the current settings.
	void Reset() {
		is_pc = false;
		id = 0;
	}

	// Basic assignment operator.
	DynamicCharacterPtr& operator=(const DynamicCharacterPtr& rhs) {
		is_pc = rhs.is_pc;
		id = rhs.id;
		return *this;
	}
	// Supports assignment with LPCHARACTER type.
	DynamicCharacterPtr& operator=(LPCHARACTER character);
	// Supports type casting to LPCHARACTER.
	operator LPCHARACTER() const {
		return Get();
	}

	bool is_pc;
	uint32_t id;
};

typedef struct character_point
{
	int64_t			points[POINT_MAX_NUM];

	BYTE			job;
	BYTE			voice;

	BYTE			level;
	DWORD			exp;

	#ifdef ENABLE_FULL_YANG
	int64_t			gold;
	#else
	long			gold;
	#endif

	int64_t			hp; //Fixme mobs Hp limit
	int				sp;

	int				iRandomHP;
	int				iRandomSP;

	int				stamina;

	BYTE			skill_group;

	#ifdef ENABLE_OTO_TOPLAMA
	BYTE			oto_metin;
	BYTE			oto_canavar;
	BYTE			oto_boss;
	#endif

	#ifdef ENABLE_ANTI_EXP
	BYTE			anti_exp;
	#endif

	#ifdef ENABLE_ALIGNMENT_BONUS
	BYTE			alignment_level;
	#endif
} CHARACTER_POINT;

typedef struct character_point_instant
{
	int64_t			points[POINT_MAX_NUM];

	float			fRot;

	int64_t			iMaxHP; //Fixme mobs Hp limit
	int				iMaxSP;

	long			position;

	long			instant_flag;
	DWORD			dwAIFlag;
	DWORD			dwImmuneFlag;
	DWORD			dwLastShoutPulse;

	DWORD			parts[PART_MAX_NUM]; // @fixme502



	// by mhh

	LPCHARACTER		pCubeNpc;


	LPCHARACTER		battle_victim;

	BYTE			gm_level;

	BYTE			bBasePart;

	int				iMaxStamina;

	BYTE			bBlockMode;

	int				iDragonSoulActiveDeck;
	LPENTITY		m_pDragonSoulRefineWindowOpener;
} CHARACTER_POINT_INSTANT;
// @fixme199 BEGIN
struct PlayerSlotT {
	std::array<LPITEM,INVENTORY_AND_EQUIP_SLOT_MAX> pItems;
	std::array<BYTE,INVENTORY_AND_EQUIP_SLOT_MAX> bItemGrid;

	std::array<LPITEM,DRAGON_SOUL_INVENTORY_MAX_NUM> pDSItems;
	std::array<WORD,DRAGON_SOUL_INVENTORY_MAX_NUM> wDSItemGrid;

	#ifdef ENABLE_SPECIAL_STORAGE
	std::array<LPITEM,SPECIAL_INVENTORY_MAX_NUM> pSSUItems;
	std::array<WORD,SPECIAL_INVENTORY_MAX_NUM> wSSUItemGrid;

	std::array<LPITEM,SPECIAL_INVENTORY_MAX_NUM> pSSBItems;
	std::array<WORD,SPECIAL_INVENTORY_MAX_NUM> wSSBItemGrid;

	std::array<LPITEM,SPECIAL_INVENTORY_MAX_NUM> pSSSItems;
	std::array<WORD,SPECIAL_INVENTORY_MAX_NUM> wSSSItemGrid;

	std::array<LPITEM,SPECIAL_INVENTORY_MAX_NUM> pSSCItems;
	std::array<WORD,SPECIAL_INVENTORY_MAX_NUM> wSSCItemGrid;
	#endif

	#ifdef ENABLE_SWITCHBOT
	std::array<LPITEM,SWITCHBOT_SLOT_COUNT> pSwitchbotItems;
	#endif
	std::array<LPITEM,CUBE_MAX_NUM> pCubeItems;
	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	std::array<TItemPosEx,ACCE_WINDOW_MAX_MATERIALS> pAcceMaterials;
	#endif

	#ifdef ENABLE_CHANGELOOK_SYSTEM
	std::array<LPITEM,CL_WINDOW_MAX_MATERIALS> pClMaterials;
	#endif
	std::array<TQuickslot,QUICKSLOT_MAX_NUM> pQuickslot;

};
// @fixme199 END

#define TRIGGERPARAM		LPCHARACTER ch, LPCHARACTER causer

typedef struct trigger
{
	BYTE	type;
	int		(*func) (TRIGGERPARAM);
	long	value;
} TRIGGER;

class CTrigger
{
	public:
		CTrigger() : bType(0), pFunc(NULL)
		{
		}

		BYTE	bType;
		int	(*pFunc) (TRIGGERPARAM);
};

EVENTINFO(char_event_info)
{
	DynamicCharacterPtr ch;
};

#ifdef BOSS_DMG_RANKING
EVENTINFO(SendRankPacketEvent)
{
	DynamicCharacterPtr bossCh;
	SendRankPacketEvent() : bossCh(){}
};
#endif

typedef std::map<VID, size_t> target_map;
struct TSkillUseInfo
{
	int		iHitCount;
	int		iMaxHitCount;
	int		iSplashCount;
	DWORD	dwNextSkillUsableTime;
	int		iRange;
	bool	bUsed;
	DWORD	dwVID;
	bool	isGrandMaster;
	#ifdef ENABLE_SKILL_COOLDOWN_CHECK
	bool	cooldown{false};
	int		skillCount{0};
	#endif

	target_map TargetVIDMap;

	TSkillUseInfo()
		: iHitCount(0), iMaxHitCount(0), iSplashCount(0), dwNextSkillUsableTime(0), iRange(0), bUsed(false),
		dwVID(0), isGrandMaster(false)
		{}

	bool	HitOnce(DWORD dwVnum = 0);

	bool	UseSkill(bool isGrandMaster, DWORD vid, DWORD dwCooltime, int splashcount = 1, int hitcount = -1, int range = -1);
	DWORD	GetMainTargetVID() const	{ return dwVID; }
	void	SetMainTargetVID(DWORD vid) { dwVID=vid; }
	void	ResetHitCount() { if (iSplashCount) { iHitCount = iMaxHitCount; iSplashCount--; } }
	#ifdef ENABLE_SKILL_COOLDOWN_CHECK
	bool	IsOnCooldown(int vnum);
	#endif
};

typedef struct packet_party_update TPacketGCPartyUpdate;
class CExchange;
class CSkillProto;
class CParty;
class CDungeon;
class CWarMap;
class CAffect;
class CGuild;
class CSafebox;
class CArena;

class CShop;
typedef class CShop * LPSHOP;

class CMob;
class CMobInstance;
typedef struct SMobSkillInfo TMobSkillInfo;

//SKILL_POWER_BY_LEVEL
extern int GetSkillPowerByLevelFromType(int job, int skillgroup, int skilllevel);
//END_SKILL_POWER_BY_LEVEL

namespace marriage
{
	class WeddingMap;
}
enum e_overtime
{
	OT_NONE,
	OT_3HOUR,
	OT_5HOUR,
};

#define NEW_ICEDAMAGE_SYSTEM
class CHARACTER : public CEntity, public CFSM, public CHorseRider
{
	protected:
		//////////////////////////////////////////////////////////////////////////////////
		virtual void	EncodeInsertPacket(LPENTITY entity);
		virtual void	EncodeRemovePacket(LPENTITY entity);
		//////////////////////////////////////////////////////////////////////////////////

	#ifdef __ARE_YOU_IN_OR_NOT__
	public:
		enum EAreYouInOrNot
		{
			STAGE_SELECT_OWN,
			STAGE_SELECT_CHEST,
			STAGE_SELECT_OFFER,
			STAGE_SELECT_LAST,

			AYIRN_CHEST_COUNT = 24,
		};

		long long			GetDragonCoin();
		void				SetDragonCoin(long long amount);

		void	ClearAyirnGame();
		void	ArgumentAyirnGame(const char* szCommand);
		BYTE	GetAyirnLeftChestCount();
		BYTE	CalculateAyirnGameOffer();
		void	CompareAyirnGameList(BYTE chestIdx);

	protected:
		int		m_iAyirnGameID, m_iAyirnGameStage, m_iAyirnSelectedChest;
		BYTE	m_iAyirnLeftChestCount, m_iAyirnOffer;
		BYTE	m_iAyirnChestList[AYIRN_CHEST_COUNT][2];
	#endif

	#ifdef __AUTO_SKILL_READER__
	public:
		void			GetAutoSkill(BYTE skillIdx, bool status, bool isFromEvent = false);
		bool			ReadSkill(BYTE skillIdx);
		bool			SkillToBook(BYTE skillIdx, DWORD& itemIdx, DWORD& socket0, DWORD& exorcismIdx, DWORD& concentratedIdx);
		void			SetAutoSkillEvent(LPEVENT _event) { m_pkAutoSkill = _event; }
		BYTE			GetSelectedSkillIndex() { return m_bSelectedSkillIdx; }

	protected:
		enum EAutoSkillRead
		{
			// ! Don't touch !
			SKILL_GRADE_MASTER = 0,
			SKILL_GRADE_GRAND_MASTER = 1,
			SKILL_GRADE_PERFECT_MASTER = 2,
			SKILL_GRADE_L_MASTER = 3,
			// ! Don't touch !

			USE_YMIR_50300_SKILLBOOK = 1,// if the 0 system will search the manuel books example: 50401

			USE_COOLTIME_ON_BOOKS = 1,
			USE_COOLTIME_ON_SOULSTONE = 1,

			USE_DECREASE_ALIGNMENT_SOULSTONE = 1,
			USE_ZEN_BEAN_SOULSTONE = 1,
		};

		bool			m_bAutoSkillStatus;
		BYTE			m_bSelectedSkillIdx;
		LPEVENT			m_pkAutoSkill;
	#endif

	#ifdef __RANKING_SYSTEM__
	public:
		void		SetRankManuel(BYTE rankIndex, long long value);
		void		SendRanks(BYTE subIndex, BYTE categoryIndex, BYTE raceIndex, BYTE rankIndex);
		void		GetRankManuel(BYTE categoryIndex, BYTE raceIndex, BYTE rankIndex, std::vector<TRanking>& list);

		void		SetKillStone(int p){m_ranking_k_s=p;}
		int			GetKillStone(){return m_ranking_k_s;}

		void		SetKillBoss(int p){m_ranking_k_b=p;}
		int			GetKillBoss(){return m_ranking_k_b;}

		void		SetDamageStone(long long p){m_ranking_dm_s=p;}
		long long	GetDamageStone(){return m_ranking_dm_s;}

		void		SetDamageBoss(long long p){m_ranking_dm_b=p;}
		long long	GetDamageBoss(){return m_ranking_dm_b;}

		void		SetCompletedDungeon(int p){m_ranking_c_d=p;}
		int			GetCompletedDungeon(){return m_ranking_c_d;}

		void		SetOpenedChest(int p){m_ranking_o_c=p;}
		int			GetOpenedChest(){return m_ranking_o_c;}

		void		SetDamageSkill(long long p){m_ranking_dm_skill=p;}
		long long	GetDamageSkill(){return m_ranking_dm_skill;}

		void		SetDamageNormal(long long p){m_ranking_dm_norm=p;}
		long long	GetDamageNormal(){return m_ranking_dm_norm;}

		///-----------------------------------------------------------///
		void		SetRankBoss(int iType, int iRank, int iKill);
		int			GetRankBoss(int iType, int iRank);

		void		LoadRankBoss(DWORD dwCount, TPlayerRankingBoss* data);
		void		SaveRankBoss(bool isDcSave = false);

		bool		IsLoadedRandBoss() const { return m_bIsLoadedRankBoss; }
		///-----------------------------------------------------------///

	protected:
		int			m_ranking_k_s;
		int			m_ranking_k_b;
		long long	m_ranking_dm_s;
		long long	m_ranking_dm_b;
		int			m_ranking_c_d;
		int			m_ranking_o_c;
		long long	m_ranking_dm_skill;
		long long	m_ranking_dm_norm;
		std::map<std::pair<int, int>, int>	m_ranking_kill_boss;
		bool		m_bIsLoadedRankBoss;
	#endif

	public:
		LPCHARACTER		FindCharacterInView(const char * name, bool bFindPCOnly);
		void			UpdatePacket();

		//////////////////////////////////////////////////////////////////////////////////
	protected:
		CStateTemplate<CHARACTER>	m_stateMove;
		CStateTemplate<CHARACTER>	m_stateBattle;
		CStateTemplate<CHARACTER>	m_stateIdle;

	public:
		virtual void		StateMove();
		virtual void		StateBattle();
		virtual void		StateIdle();
		virtual void		StateFlag();
		virtual void		StateFlagBase();
		void				StateHorse();

	protected:
		// STATE_IDLE_REFACTORING
		void				__StateIdle_Monster();
		void				__StateIdle_Stone();
		void				__StateIdle_NPC();
		// END_OF_STATE_IDLE_REFACTORING

	public:
		DWORD GetAIFlag() const	{ return m_pointsInstant.dwAIFlag; }

		void				SetAggressive();
		bool				IsAggressive() const;

		void				SetCoward();
		bool				IsCoward() const;
		void				CowardEscape();

		void				SetNoAttackShinsu();
		bool				IsNoAttackShinsu() const;

		void				SetNoAttackChunjo();
		bool				IsNoAttackChunjo() const;

		void				SetNoAttackJinno();
		bool				IsNoAttackJinno() const;

		void				SetAttackMob();
		bool				IsAttackMob() const;

		virtual void		BeginStateEmpty();
		virtual void		EndStateEmpty() {}

		void				RestartAtSamePos();

	protected:
		DWORD				m_dwStateDuration;
		//////////////////////////////////////////////////////////////////////////////////

	public:
		CHARACTER();
		virtual ~CHARACTER();

		void			Create(const char * c_pszName, DWORD vid, bool isPC);
		void			Destroy();

		void			Disconnect(const char * c_pszReason);

	protected:
		void			Initialize();

		//////////////////////////////////////////////////////////////////////////////////
		// Basic Points

	#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
	private:
		DWORD			dwLastTargetInfoPulse;

	public:
		DWORD			GetLastTargetInfoPulse() const	{ return dwLastTargetInfoPulse; }
		void			SetLastTargetInfoPulse(DWORD pulse) { dwLastTargetInfoPulse = pulse; }
	#endif

	public:
		DWORD			GetPlayerID() const	{ return m_dwPlayerID; }

		void			SetPlayerProto(const TPlayerTable * table);
		void			CreatePlayerProto(TPlayerTable & tab);

		void			SetProto(const CMob * c_pkMob);
		DWORD			GetRaceNum() const; // @fixme501

		void			Save();		// DelayedSave
		void			SaveReal();
		void			FlushDelayedSaveItem();

		const char *	GetName() const;
		const VID &		GetVID() const		{ return m_vid;		}

		void			SetName(const std::string& name) { m_stName = name; }

		void			SetRace(BYTE race);
		bool			ChangeSex();

		DWORD			GetAID() const;
		int				GetChangeEmpireCount() const;
		void			SetChangeEmpireCount();
		int				ChangeEmpire(BYTE empire);

		BYTE			GetJob() const;
		BYTE			GetCharType() const;

		bool			IsPC() const		{ return GetDesc() ? true : false; }
		bool			IsNPC()	const		{ return m_bCharType != CHAR_TYPE_PC; }
		bool			IsMonster()	const	{ return m_bCharType == CHAR_TYPE_MONSTER; }
		bool			IsStone() const		{ return m_bCharType == CHAR_TYPE_STONE; }
		bool			IsDoor() const		{ return m_bCharType == CHAR_TYPE_DOOR; }
		bool			IsBuilding() const	{ return m_bCharType == CHAR_TYPE_BUILDING;  }
		bool			IsWarp() const		{ return m_bCharType == CHAR_TYPE_WARP; }
		bool			IsGoto() const		{ return m_bCharType == CHAR_TYPE_GOTO; }

		DWORD			GetLastShoutPulse() const	{ return m_pointsInstant.dwLastShoutPulse; }
		void			SetLastShoutPulse(DWORD pulse) { m_pointsInstant.dwLastShoutPulse = pulse; }
		int				GetLevel() const		{ return m_points.level;	}
		void			SetLevel(BYTE level);

		BYTE			GetGMLevel() const;
		BOOL 			IsGM() const;
		void			SetGMLevel();

		DWORD			GetExp() const		{ return m_points.exp;	}
		void			SetExp(DWORD exp)	{ m_points.exp = exp;	}
		DWORD			GetNextExp() const;
		LPCHARACTER		DistributeExp();
		void			DistributeHP(LPCHARACTER pkKiller);
		void			DistributeSP(LPCHARACTER pkKiller, int iMethod=0);

		#ifdef ENABLE_KILL_EVENT_FIX
		LPCHARACTER		GetMostAttacked();
		#endif

		void			SetPosition(int pos);
		bool			IsPosition(int pos) const	{ return m_pointsInstant.position == pos ? true : false; }
		int				GetPosition() const		{ return m_pointsInstant.position; }

		void			SetPart(BYTE bPartPos, DWORD wVal); // @fixme502
		DWORD			GetPart(BYTE bPartPos) const; // @fixme502
		DWORD			GetOriginalPart(BYTE bPartPos) const; // @fixme502

		void			SetHP(int64_t hp)	{ m_points.hp = hp; }
		int64_t			GetHP() const		{ return m_points.hp; }

		void			SetSP(int sp)		{ m_points.sp = sp; }
		int				GetSP() const		{ return m_points.sp; }

		void			SetStamina(int stamina)	{ m_points.stamina = stamina; }
		int				GetStamina() const		{ return m_points.stamina; }

		void			SetMaxHP(int64_t iVal)	{ m_pointsInstant.iMaxHP = iVal; }
		int64_t			GetMaxHP() const	{ return m_pointsInstant.iMaxHP; }

		void			SetMaxSP(int iVal)	{ m_pointsInstant.iMaxSP = iVal; }
		int				GetMaxSP() const	{ return m_pointsInstant.iMaxSP; }

		void			SetMaxStamina(int iVal)	{ m_pointsInstant.iMaxStamina = iVal; }
		int				GetMaxStamina() const	{ return m_pointsInstant.iMaxStamina; }

		void			SetRandomHP(int v)	{ m_points.iRandomHP = v; }
		void			SetRandomSP(int v)	{ m_points.iRandomSP = v; }

		int				GetRandomHP() const	{ return m_points.iRandomHP; }
		int				GetRandomSP() const	{ return m_points.iRandomSP; }

		int				GetHPPct() const;

		void			SetRealPoint(BYTE idx, int64_t val);
		int64_t			GetRealPoint(BYTE idx) const;

		void			SetPoint(BYTE idx, int64_t val);
		int64_t			GetPoint(BYTE idx) const;

		int				GetLimitPoint(BYTE idx) const;
		int				GetPolymorphPoint(BYTE idx) const;

		const TMobTable &	GetMobTable() const;
		BYTE				GetMobRank() const;
		BYTE				GetMobBattleType() const;
		BYTE				GetMobSize() const;
		DWORD				GetMobDamageMin() const;
		DWORD				GetMobDamageMax() const;
		WORD				GetMobAttackRange() const;
		DWORD				GetMobDropItemVnum() const;
		float				GetMobDamageMultiply() const;

		// NEWAI
		bool			IsBerserker() const;
		bool			IsBerserk() const;
		void			SetBerserk(bool mode);

		bool			IsStoneSkinner() const;

		bool			IsGodSpeeder() const;
		bool			IsGodSpeed() const;
		void			SetGodSpeed(bool mode);

		bool			IsDeathBlower() const;
		bool			IsDeathBlow() const;

		bool			IsReviver() const;
		bool			HasReviverInParty() const;
		bool			IsRevive() const;
		void			SetRevive(bool mode);
		// NEWAI END

		bool			IsRaceFlag(DWORD dwBit) const;
		bool			IsSummonMonster() const;
		DWORD			GetSummonVnum() const;

		DWORD			GetPolymorphItemVnum() const;
		DWORD			GetMonsterDrainSPPoint() const;

		void			MainCharacterPacket();

		void			ComputePoints();
		void			ComputeBattlePoints();

		void			PointChange(BYTE type, int64_t amount, bool bAmount = false, bool bBroadcast = false);

		void			PointsPacket();
		void			ApplyPoint(BYTE bApplyType, int iVal);
		void			CheckMaximumPoints();

		bool			Show(long lMapIndex, long x, long y, long z = LONG_MAX, bool bShowSpawnMotion = false);

		void			Sitdown(int is_ground);
		void			Standup();

		void			SetRotation(float fRot);
		void			SetRotationToXY(long x, long y);
		float			GetRotation() const	{ return m_pointsInstant.fRot; }

		void			MotionPacketEncode(BYTE motion, LPCHARACTER victim, struct packet_motion * packet);
		void			Motion(BYTE motion, LPCHARACTER victim = NULL);

		void			ChatPacket(BYTE type, const char *format, ...);
		void			MonsterChat(BYTE bMonsterChatType);
		void			SendGreetMessage();

		void			ResetPoint(int iLv);

		void			SetBlockMode(BYTE bFlag);
		void			SetBlockModeForce(BYTE bFlag);
		bool			IsBlockMode(BYTE bFlag) const	{ return (m_pointsInstant.bBlockMode & bFlag)?true:false; }

		bool			IsPolymorphed() const		{ return m_dwPolymorphRace>0; }
		bool			IsPolyMaintainStat() const	{ return m_bPolyMaintainStat; }
		void			SetPolymorph(DWORD dwRaceNum, bool bMaintainStat = false);
		DWORD			GetPolymorphVnum() const	{ return m_dwPolymorphRace; }
		int				GetPolymorphPower() const;
		bool			IsNearWater() const;// @fixme219

		// FISING
		void			fishing();
		void			fishing_cancel();
		void			fishing_take();
		// END_OF_FISHING

		// MINING
		void			mining(LPCHARACTER chLoad);
		void			mining_cancel();
		void			mining_take();
		// END_OF_MINING

		void			ResetPlayTime(DWORD dwTimeRemain = 0);

		void			CreateFly(BYTE bType, LPCHARACTER pkVictim);

		void			ResetChatCounter();
		BYTE			IncreaseChatCounter();
		BYTE			GetChatCounter() const;

	protected:
		DWORD			m_dwPolymorphRace;
		bool			m_bPolyMaintainStat;
		DWORD			m_dwLoginPlayTime;
		DWORD			m_dwPlayerID;
		VID				m_vid;
		std::string		m_stName;
		BYTE			m_bCharType;

		CHARACTER_POINT		m_points;
		CHARACTER_POINT_INSTANT	m_pointsInstant;
		std::unique_ptr<PlayerSlotT> m_PlayerSlots; // @fixme199
		#ifdef ENABLE_MOB_RANGE_ITEM
		bool			m_antimobrange;
		#endif

		#ifdef ENABLE_REFINE_LUCK
		bool			m_refineluck;
		#endif

		int				m_iMoveCount;
		DWORD			m_dwPlayStartTime;
		#ifdef ENABLE_BATTLE_PASS
		DWORD			m_dwOnLoginTime;
		#endif
		BYTE			m_bAddChrState;
		bool			m_bSkipSave;
		std::string		m_stMobile;
		char			m_szMobileAuth[5];
		BYTE			m_bChatCounter;

		// End of Basic Points

		//////////////////////////////////////////////////////////////////////////////////
		// Move & Synchronize Positions
		//////////////////////////////////////////////////////////////////////////////////
#ifdef SPIN_WHEEL
public:
	void			SetSpinReward(DWORD dwSpin, DWORD dwCount) {
		dwResultSpinWheel = dwSpin;
		dwResultSpinWheelCount = dwCount;
	};
	DWORD			GetSpinReward() const { return dwResultSpinWheel; }
	DWORD			GetSpinRewardCount() const { return dwResultSpinWheelCount; }
	void			SendMetinstoneCount();
protected:
	DWORD			dwResultSpinWheel;
	DWORD			dwResultSpinWheelCount;
#endif
	public:
		bool			IsStateMove() const			{ return IsState((CState&)m_stateMove); }
		bool			IsStateIdle() const			{ return IsState((CState&)m_stateIdle); }
		bool			IsWalking() const			{ return m_bNowWalking || GetStamina()<=0; }
		void			SetWalking(bool bWalkFlag)	{ m_bWalking=bWalkFlag; }
		void			SetNowWalking(bool bWalkFlag);
		void			ResetWalking()			{ SetNowWalking(m_bWalking); }

		bool			Goto(long x, long y);
		void			Stop();

		bool			CanMove() const;

		void			SyncPacket();
		bool			Sync(long x, long y);
		bool			Move(long x, long y);
		void			OnMove(bool bIsAttack = false);
		DWORD			GetMotionMode() const;
		float			GetMoveMotionSpeed() const;
		float			GetMoveSpeed() const;
		void			CalculateMoveDuration();
		void			SendMovePacket(BYTE bFunc, BYTE bArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime=0, int iRot=-1);
		DWORD			GetCurrentMoveDuration() const	{ return m_dwMoveDuration; }
		DWORD			GetWalkStartTime() const	{ return m_dwWalkStartTime; }
		DWORD			GetLastMoveTime() const		{ return m_dwLastMoveTime; }
		DWORD			GetLastAttackTime() const	{ return m_dwLastAttackTime; }

		void			SetLastAttacked(DWORD time);

		void			SetLastMoved(DWORD time) { m_dwLastMovedTime = time; }
		DWORD			GetLastMovedTime() const	{ return m_dwLastMovedTime; }

		bool			SetSyncOwner(LPCHARACTER ch, bool bRemoveFromList = true);
		bool			IsSyncOwner(LPCHARACTER ch) const;
		#ifdef ENABLE_CHANGE_CHANNEL_SYSTEM
		bool			SwitchChannel(long newAddr, WORD newPort);
		bool			StartChannelSwitch(long newAddr, WORD newPort);
		#endif
		bool			WarpSet(long x, long y, long lRealMapIndex = 0, WORD wNewPort = 0);
		void			SetWarpLocation(long lMapIndex, long x, long y);
		void			WarpEnd();
		const PIXEL_POSITION & GetWarpPosition() const { return m_posWarp; }

		#ifdef ENABLE_WARP_REGULATION
		bool			WarpToPID(DWORD dwPID, bool IsGM = false);
		#else
		bool			WarpToPID(DWORD dwPID);
		#endif

		void			SaveExitLocation();
		void			ExitToSavedLocation();

		void			StartStaminaConsume();
		void			StopStaminaConsume();
		bool			IsStaminaConsume() const;
		bool			IsStaminaHalfConsume() const;

		void			ResetStopTime();
		DWORD			GetStopTime() const;

	protected:
		void			ClearSync();

		float			m_fSyncTime;
		LPCHARACTER		m_pkChrSyncOwner;
		CHARACTER_LIST	m_kLst_pkChrSyncOwned;

		PIXEL_POSITION	m_posDest;
		PIXEL_POSITION	m_posStart;
		PIXEL_POSITION	m_posWarp;
		long			m_lWarpMapIndex;

		PIXEL_POSITION	m_posExit;
		long			m_lExitMapIndex;

		DWORD			m_dwMoveStartTime;
		DWORD			m_dwMoveDuration;

		DWORD			m_dwLastMoveTime;
		DWORD			m_dwLastAttackTime;
		DWORD			m_dwLastMovedTime;
		DWORD			m_dwWalkStartTime;
		DWORD			m_dwStopTime;

		bool			m_bWalking;
		bool			m_bNowWalking;
		bool			m_bStaminaConsume;
		// End

	public:
		void			SyncQuickslot(BYTE bType, BYTE bOldPos, BYTE bNewPos);
		bool			GetQuickslot(BYTE pos, TQuickslot ** ppSlot);
		bool			SetQuickslot(BYTE pos, TQuickslot & rSlot);
		bool			DelQuickslot(BYTE pos);
		bool			SwapQuickslot(BYTE a, BYTE b);
		void			ChainQuickslotItem(LPITEM pItem, BYTE bType, BYTE bOldPos);

	#ifdef ENABLE_IKASHOP_RENEWAL
	private:
		// watcher
		using ShopActionWeight = uint16_t;
		using ShopActionTimestamp = double;
		using ShopActionData = std::pair<ShopActionWeight, ShopActionTimestamp>;
		using ShopActionRegister = std::deque<ShopActionData>;

	public:
		using SHOP_HANDLE = std::shared_ptr<ikashop::CShop>;
		using SAFEBOX_HANDLE = std::shared_ptr<ikashop::CShopSafebox>;
		using AUCTION_HANDLE = std::shared_ptr<ikashop::CAuction>;

		SHOP_HANDLE GetIkarusShop() {return m_pkIkarusShop;}
		void SetIkarusShop(SHOP_HANDLE pkShop) {m_pkIkarusShop = pkShop;}

		SHOP_HANDLE	GetIkarusShopGuest() const {return m_pkIkarusShopGuest;}
		void SetIkarusShopGuest(SHOP_HANDLE pkShop);

		SAFEBOX_HANDLE GetIkarusShopSafebox() {return m_pkIkarusShopSafebox;}
		void SetIkarusShopSafebox(SAFEBOX_HANDLE pk);

		void SetIkarusAuction(AUCTION_HANDLE pk) {m_pkIkarusAuction = pk;}
		void SetIkarusAuctionGuest(AUCTION_HANDLE pk);

		AUCTION_HANDLE GetIkarusAuction() {return m_pkIkarusAuction;}
		AUCTION_HANDLE GetIkarusAuctionGuest() const {return m_pkIkarusAuctionGuest;}

		// looking offerlist
		void SetLookingIkarusShopOfferList(bool state) { m_bIsLookingIkarusShopOfferList = state; }
		bool IsLookingIkarusShopOfferList() const { return m_bIsLookingIkarusShopOfferList; }

		// looking shop
		void SetLookingShopOwner(bool state) { m_bIsLookingShopOwner  = state;}
		bool IsLookingShopOwner() const { return m_bIsLookingShopOwner; }

		// looking auction
		void SetLookingAuctionOwner(bool state) { m_bIsLookingAuctionOwner = state;}
		bool IsLookingAuctionOwner() const { return m_bIsLookingAuctionOwner; }

		bool CanTakeInventoryItem(LPITEM item, TItemPos* pos);

		//watcher
		bool IkarusShopFloodCheck(ShopActionWeight weight);

		// Premium Serach
		void SetPremiumSearch(bool state) { m_bIsPremiumSearch = state;}
		bool IsPremiumSearch() const { return m_bIsPremiumSearch; }

	private:
		//watcher
		ShopActionRegister m_IkarusShopActionRegister;

		// handles
		SHOP_HANDLE	m_pkIkarusShop;
		SHOP_HANDLE	m_pkIkarusShopGuest;
		SAFEBOX_HANDLE m_pkIkarusShopSafebox;
		AUCTION_HANDLE m_pkIkarusAuction;
		AUCTION_HANDLE m_pkIkarusAuctionGuest;

		bool m_bIsLookingIkarusShopOfferList = false;
		bool m_bIsLookingShopOwner = false;
		bool m_bIsLookingAuctionOwner = false;
		bool m_bIsPremiumSearch = false;
	#endif

	#if defined(__BL_LUCKY_BOX__)
	public:
		void			SetLuckyBoxSrcItem(LPITEM item);
		void			SendLuckyBoxInfo();
		void			LuckyBoxRetry();
		void			LuckyBoxReceive();
		#ifdef ENABLE_FULL_YANG
		int64_t			GetLuckyBoxPrice() const;
		#else
		int				GetLuckyBoxPrice() const;
		#endif
		bool			IsLuckyBoxOpen() const;
		void			ResetLuckyBoxData();

	private:
		struct
		{
			CLuckyBoxGroup::ELUCKY_BOX_TYPE	type;
			DWORD		src_item_id;
			BYTE		try_count;
			DWORD		item_vnum;
			#ifdef ENABLE_NEW_MAX_COUNT
			short		item_count;
			#else
			BYTE		item_count;
			#endif
		} m_sLuckyBox;
	#endif

	protected:
		TQuickslot		m_quickslot[QUICKSLOT_MAX_NUM];

	////////////////////////////////////////////////////////////////////////////////////////
	// Affect
	public:
		void			StartAffectEvent();
		void			ClearAffect(bool bSave = false, bool bSkillRemove = false);
		void			ComputeAffect(CAffect * pkAff, bool bAdd);
		bool			AddAffect(DWORD dwType, BYTE bApplyOn, long lApplyValue, DWORD dwFlag, long lDuration, long lSPCost, bool bOverride, bool IsCube = false);
		void			RefreshAffect();
		bool			RemoveAffect(DWORD dwType);
		bool			IsAffectFlag(DWORD dwAff) const;

		bool			UpdateAffect();	// called from EVENT
		int				ProcessAffect();

		void			LoadAffect(DWORD dwCount, TPacketAffectElement * pElements);
		void			SaveAffect();

		bool			IsLoadedAffect() const	{ return m_bIsLoadedAffect; }

		bool			IsGoodAffect(BYTE bAffectType) const;

		void			RemoveSkillAffect();
		void			RemoveGoodAffect();
		void			RemoveBadAffect();

		CAffect *		FindAffect(DWORD dwType, BYTE bApply=APPLY_NONE) const;
		const std::list<CAffect *> & GetAffectContainer() const	{ return m_list_pkAffect; }
		bool			RemoveAffect(CAffect * pkAff);

		void			CheckLoadAffect();

		void			ResetAffectFlag();

	protected:
		bool			m_bIsLoadedAffect;
		TAffectFlag		m_afAffectFlag;
		std::list<CAffect *>	m_list_pkAffect;

	public:
		// PARTY_JOIN_BUG_FIX
		void			SetParty(LPPARTY pkParty);
		LPPARTY			GetParty() const	{ return m_pkParty; }

		bool			RequestToParty(LPCHARACTER leader);
		void			DenyToParty(LPCHARACTER member);
		void			AcceptToParty(LPCHARACTER member);

		void			PartyInvite(LPCHARACTER pchInvitee);

		void			PartyInviteAccept(LPCHARACTER pchInvitee);

		void			PartyInviteDeny(DWORD dwPID);

		bool			BuildUpdatePartyPacket(TPacketGCPartyUpdate & out);
		int				GetLeadershipSkillLevel() const;

		bool			CanSummon(int iLeaderShip);

		void			SetPartyRequestEvent(LPEVENT pkEvent) { m_pkPartyRequestEvent = pkEvent; }

	protected:
		void			PartyJoin(LPCHARACTER pkLeader);

		enum PartyJoinErrCode {
			PERR_NONE		= 0,
			PERR_SERVER,
			PERR_DUNGEON,
			PERR_OBSERVER,
			PERR_LVBOUNDARY,
			PERR_LOWLEVEL,
			PERR_HILEVEL,
			#ifdef ENABLE_VSTURNUVA
			PERR_DUEL,
			#endif
			PERR_ALREADYJOIN,
			PERR_PARTYISFULL,
			PERR_SEPARATOR,			///< Error type separator.
			PERR_DIFFEMPIRE,
			PERR_MAX
		};

		static PartyJoinErrCode	IsPartyJoinableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest);

		static PartyJoinErrCode	IsPartyJoinableMutableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest);

		LPPARTY			m_pkParty;
		DWORD			m_dwLastDeadTime;
		LPEVENT			m_pkPartyRequestEvent;

		typedef std::map< DWORD, LPEVENT >	EventMap;
		EventMap		m_PartyInviteEventMap;
		// END_OF_PARTY_JOIN_BUG_FIX

		////////////////////////////////////////////////////////////////////////////////////////
		// Dungeon
	public:
		void			SetDungeon(LPDUNGEON pkDungeon);
		LPDUNGEON		GetDungeon() const	{ return m_pkDungeon; }
		LPDUNGEON		GetDungeonForce() const;

	protected:
		LPDUNGEON		m_pkDungeon;
		int				m_iEventAttr;

		////////////////////////////////////////////////////////////////////////////////////////
		// Guild
	public:
		void			SetGuild(CGuild * pGuild);
		CGuild*			GetGuild() const	{ return m_pGuild; }

		void			SetWarMap(CWarMap* pWarMap);
		CWarMap*		GetWarMap() const	{ return m_pWarMap; }

	protected:
		CGuild *		m_pGuild;
		DWORD			m_dwUnderGuildWarInfoMessageTime;
		CWarMap *		m_pWarMap;
		////////////////////////////////////////////////////////////////////////////////////////
		// Item related
	public:
		bool			CanHandleItem(bool bSkipRefineCheck = false, bool bSkipObserver = false);

		bool			IsItemLoaded() const	{ return m_bItemLoaded; }
		void			SetItemLoaded()	{ m_bItemLoaded = true; }

		void			ClearItem();

		#ifdef ENABLE_HIGHLIGHT_NEW_ITEM
		void			SetItem(TItemPos Cell, LPITEM item, bool bWereMine = false);
		#else
		void			SetItem(TItemPos Cell, LPITEM item);
		#endif

		LPITEM			GetItem(TItemPos Cell) const;
		LPITEM			GetInventoryItem(WORD wCell) const;

		#ifdef ENABLE_SPECIAL_STORAGE
		LPITEM			GetUpgradeInventoryItem(WORD wCell) const;
		LPITEM			GetBookInventoryItem(WORD wCell) const;
		LPITEM			GetStoneInventoryItem(WORD wCell) const;
		LPITEM			GetChestInventoryItem(WORD wCell) const;
		#endif

		#ifdef ENABLE_SWITCHBOT
		LPITEM			GetSwitchBotItem(WORD wCell) const;
		#endif

		bool			IsEmptyItemGrid(TItemPos Cell, BYTE size, int iExceptionCell = -1) const;

		void			SetWear(BYTE bCell, LPITEM item);
		LPITEM			GetWear(BYTE bCell) const;

		// MYSHOP_PRICE_LIST
		void			UseSilkBotary(void);

		void			UseSilkBotaryReal(const TPacketMyshopPricelistHeader* p);
		// END_OF_MYSHOP_PRICE_LIST

		bool			UseItemEx(LPITEM item, TItemPos DestCell);
		bool			UseItem(TItemPos Cell, TItemPos DestCell = NPOS);

		// ADD_REFINE_BUILDING
		bool			IsRefineThroughGuild() const;
		CGuild *		GetRefineGuild() const;

		#ifdef ENABLE_FULL_YANG
		int64_t			ComputeRefineFee(int64_t iCost, int iMultiply = 5) const;
		void			PayRefineFee(int64_t iTotalMoney);
		#else
		int				ComputeRefineFee(int iCost, int iMultiply = 5) const;
		void			PayRefineFee(int iTotalMoney);
		#endif

		void			SetRefineNPC(LPCHARACTER ch);
		// END_OF_ADD_REFINE_BUILDING

		bool			RefineItem(LPITEM pkItem, LPITEM pkTarget);

	#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
		bool			DeleteItem(TItemPos Cell);
		bool			SellItem(TItemPos Cell, bool isAuto = false);
	#else
		#ifdef ENABLE_NEW_MAX_COUNT
		bool			DropItem(TItemPos Cell, short bCount = 0);
		#else
		bool			DropItem(TItemPos Cell, BYTE bCount = 0);
		#endif
	#endif

		bool			GiveRecallItem(LPITEM item);
		void			ProcessRecallItem(LPITEM item);

		//	void			PotionPacket(int iPotionType);
		void			EffectPacket(int enumEffectType);
		void			SpecificEffectPacket(const char filename[128]);

		// ADD_MONSTER_REFINE
		bool			DoRefine(LPITEM item, bool bMoneyOnly = false);
		// END_OF_ADD_MONSTER_REFINE

		bool			DoRefineWithScroll(LPITEM item);
		bool			RefineInformation(BYTE bCell, BYTE bType, int iAdditionalCell = -1);

		void			SetRefineMode(int iAdditionalCell = -1);
		void			ClearRefineMode();

		bool			GiveItem(LPCHARACTER victim, TItemPos Cell);
		bool			CanReceiveItem(LPCHARACTER from, LPITEM item) const;
		void			ReceiveItem(LPCHARACTER from, LPITEM item);

		bool			GiveItemFromSpecialItemGroup(
							DWORD dwGroupNum
							, std::vector <DWORD> &dwItemVnums
							, std::vector <DWORD> &dwItemCounts
							, std::vector <LPITEM> &item_gets
							, int &count
							#ifdef ENABLE_LOCKED_SYSTEM
							, BYTE bItemLocked = 0
							#endif
		);

		#ifdef ENABLE_NEW_MAX_COUNT
		bool			MoveItem(TItemPos pos, TItemPos change_pos, short num);
		#else
		bool			MoveItem(TItemPos pos, TItemPos change_pos, BYTE num);
		#endif

		bool			PickupItem(DWORD vid);
		bool			EquipItem(LPITEM item, int iCandidateCell = -1);
		bool			UnequipItem(LPITEM item);

		#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		void			Dismount();
		#endif

		bool			CanEquipNow(const LPITEM item, const TItemPos& srcCell = NPOS, const TItemPos& destCell = NPOS);

		#ifdef ENABLE_FAST_OPEN_CHEST
		bool			UseItemAll(TItemPos Cell);
		#endif

		bool			CanUnequipNow(const LPITEM item, const TItemPos& srcCell = NPOS, const TItemPos& destCell = NPOS);

		bool			SwapItem(BYTE bCell, BYTE bDestCell);

		LPITEM			AutoGiveItem(DWORD dwItemVnum
									#ifdef ENABLE_NEW_MAX_COUNT
									, short bCount = 1
									#else
									, BYTE bCount = 1
									#endif
									, int iRarePct = -1
									, bool bMsg = true
									#ifdef ENABLE_LOCKED_SYSTEM
									, bool bIsLocked = false
									#endif
		);

		void			AutoGiveItem(LPITEM item, bool longOwnerShip = false);

		int				GetEmptyInventory(BYTE size) const;
		int				GetEmptyDragonSoulInventory(LPITEM pItem) const;
		void			CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const;

		int				GetEmptyAllInventory(LPITEM item) const;

	#ifdef ENABLE_SPLIT_ITEMS_SYSTEM
		int				GetEmptyInventoryFromIndex(WORD index, BYTE itemSize) const; //SPLIT ITEMS
		#ifdef ENABLE_SPECIAL_STORAGE
		int				GetEmptyUpgradeInventoryFromIndex(WORD index, BYTE itemSize) const; //SPLIT ITEMS
		int				GetEmptyBookInventoryFromIndex(WORD index, BYTE itemSize) const; //SPLIT ITEMS
		int				GetEmptyStoneInventoryFromIndex(WORD index, BYTE itemSize) const; //SPLIT ITEMS
		int				GetEmptyChestInventoryFromIndex(WORD index, BYTE itemSize) const; //SPLIT ITEMS
		#endif
	#endif

		#ifdef ENABLE_SPECIAL_STORAGE
		int				GetEmptyUpgradeInventory(LPITEM pItem) const;
		int				GetEmptyBookInventory(LPITEM pItem) const;
		int				GetEmptyStoneInventory(LPITEM pItem) const;
		int				GetEmptyChestInventory(LPITEM pItem) const;
		#endif

		int				CountEmptyInventory() const;

		int				CountSpecifyItem(DWORD vnum) const;
		void			RemoveSpecifyItem(DWORD vnum, DWORD count = 1);

		LPITEM			FindSpecifyItem(DWORD vnum) const;
		LPITEM			FindItemByID(DWORD id) const;

		int				CountSpecifyTypeItem(BYTE type) const;
		void			RemoveSpecifyTypeItem(BYTE type, DWORD count = 1);

		#ifdef ENABLE_SWITCHBOT
		bool			RemoveAttributeItem(BYTE bType) const;
		#endif

		bool			IsEquipUniqueItem(DWORD dwItemVnum) const;

		bool			IsEquipUniqueGroup(DWORD dwGroupVnum) const;

		void			SendEquipment(LPCHARACTER ch);

	public:
		bool			IsUnderRefine() const { return m_bUnderRefine; }
		void			SetUnderRefine(bool bState) { m_bUnderRefine = bState; }

	protected:
		#ifdef ENABLE_FULL_YANG
		void			SendMyShopPriceListCmd(DWORD dwItemVnum, int64_t dwItemPrice);
		#else
		void			SendMyShopPriceListCmd(DWORD dwItemVnum, DWORD dwItemPrice);
		#endif

		bool			m_bNoOpenedShop;

		bool			m_bItemLoaded;
		int				m_iRefineAdditionalCell;
		bool			m_bUnderRefine;
		DWORD			m_dwRefineNPCVID;

	public:
		////////////////////////////////////////////////////////////////////////////////////////
		// Money related
		#ifdef ENABLE_FULL_YANG
		int64_t			GetGold() const { return m_points.gold; }
		void			SetGold(int64_t gold) { m_points.gold = gold; }

		static uint64_t GetAllowedGold();
		void			GoldChange(int64_t ldGold);
		void			GiveGold(int64_t iAmount);
		bool			DropGold(int64_t gold);
		#else
		INT				GetGold() const		{ return m_points.gold;	}
		void			SetGold(INT gold)	{ m_points.gold = gold;	}
		bool			DropGold(INT gold);
		INT				GetAllowedGold() const;
		void			GiveGold(INT iAmount);
		#endif
		// End of Money

	#ifdef ENABLE_OTO_TOPLAMA
	public:
		BYTE			GetOtoMetin() const			{ return m_points.oto_metin;	}
		void			SetOtoMetin(BYTE level)		{ m_points.oto_metin = level;	}

		BYTE			GetOtoCanavar() const		{ return m_points.oto_canavar;	}
		void			SetOtoCanavar(BYTE level)	{ m_points.oto_canavar = level;	}

		BYTE			GetOtoBoss() const			{ return m_points.oto_boss;	}
		void			SetOtoBoss(BYTE level)		{ m_points.oto_boss = level;	}

		void			AutoGiveItemNew(LPITEM pkItem);
		bool			IsBossNew();
	#endif

	#ifdef ENABLE_EXCHANGE_LOG
	public:
		void			LoadExchangeLog();
		bool			LoadExchangeLogItem(DWORD logID);
		void			DeleteExchangeLog(DWORD logID);
		void			SendExchangeLogPacket(BYTE subHeader, DWORD id = 0, const TExchangeLog* exchangeLog = NULL);

	protected:
		std::map<DWORD, std::pair<TExchangeLog, std::vector<TExchangeLogItem>>> m_mapExchangeLog;
	#endif

	#ifdef ENABLE_ANTI_EXP
	public:
		BYTE			GetAntiExp() const		{ return m_points.anti_exp;	}
		void			SetAntiExp(BYTE level)	{ m_points.anti_exp = level;	}
	#endif

		////////////////////////////////////////////////////////////////////////////////////////
		// Shop related
	public:
		void			SetShop(LPSHOP pkShop);
		LPSHOP			GetShop() const { return m_pkShop; }
		void			ShopPacket(BYTE bSubHeader);

		void			SetShopOwner(LPCHARACTER ch) { m_pkChrShopOwner = ch; }
		LPCHARACTER		GetShopOwner() const { return m_pkChrShopOwner;}

		void			OpenMyShop(const char * c_pszSign, TShopItemTable * pTable, BYTE bItemCount);
		LPSHOP			GetMyShop() const { return m_pkMyShop; }
		void			CloseMyShop();

	protected:

		LPSHOP			m_pkShop;
		LPSHOP			m_pkMyShop;
		std::string		m_stShopSign;
		LPCHARACTER		m_pkChrShopOwner;
		// End of shop

		////////////////////////////////////////////////////////////////////////////////////////
		// Exchange related
	public:
		bool			ExchangeStart(LPCHARACTER victim);
		void			SetExchange(CExchange * pkExchange);
		CExchange *		GetExchange() const	{ return m_pkExchange;	}

	protected:
		CExchange *		m_pkExchange;
		// End of Exchange

		////////////////////////////////////////////////////////////////////////////////////////
		// Battle
	public:
		struct TBattleInfo
		{
			DamagePoint iTotalDamage;
			DamagePoint iAggro;

			TBattleInfo(DamagePoint iTot, DamagePoint iAggr)
				: iTotalDamage(iTot), iAggro(iAggr)
				{}
		};
		typedef std::map<VID, TBattleInfo> TDamageMap;

		typedef struct SAttackLog
		{
			DWORD	dwVID;
			DWORD	dwTime;
		} AttackLog;

		bool				Damage(LPCHARACTER pAttacker, int dam, EDamageType type = DAMAGE_TYPE_NORMAL);
		bool				__Profile__Damage(LPCHARACTER pAttacker, int dam, EDamageType type = DAMAGE_TYPE_NORMAL);
		void				DeathPenalty(BYTE bExpLossPercent);
		void				ReviveInvisible(int iDur);

		bool				Attack(LPCHARACTER pkVictim, BYTE bType = 0);
		bool				IsAlive() const		{ return m_pointsInstant.position == POS_DEAD ? false : true; }
		bool				CanFight() const;

		bool				CanBeginFight() const;
		void				BeginFight(LPCHARACTER pkVictim);

		bool				CounterAttack(LPCHARACTER pkChr);

		bool				IsStun() const;
		void				Stun();
		bool				IsDead() const;
		void				Dead(LPCHARACTER pkKiller = NULL, bool bImmediateDead=true); // @fixme188 instant death false to true

		#ifdef ENABLE_MOB_RANGE_ITEM
		void				SetAntiMobRange(bool status = false) { m_antimobrange = status; };
		bool				GetAntiMobRange() const { return m_antimobrange; } ;
		#endif

		#ifdef ENABLE_REFINE_LUCK
		void				SetRefineLuck(bool status = false) { m_refineluck = status; };
		bool				GetRefineLuck() const { return m_refineluck; } ;
		#endif

		void				Reward(bool bItemDrop);
		void				RewardGold(LPCHARACTER pkAttacker);

		bool				Shoot(BYTE bType);
		void				FlyTarget(DWORD dwTargetVID, long x, long y, BYTE bHeader);

		void				ForgetMyAttacker();
		void				AggregateMonster();
		void				AttractRanger();
		void				PullMonster();

		int					GetArrowAndBow(LPITEM * ppkBow, LPITEM * ppkArrow, int iArrowCount = 1);
		void				UseArrow(LPITEM pkArrow, DWORD dwArrowCount);

		void				AttackedByPoison(LPCHARACTER pkAttacker);
		void				RemovePoison();

		#ifdef ENABLE_WOLFMAN_CHARACTER
		void				AttackedByBleeding(LPCHARACTER pkAttacker);
		void				RemoveBleeding();
		#endif

		void				AttackedByFire(LPCHARACTER pkAttacker, int amount, int count);
		void				RemoveFire();

		void				UpdateAlignment(int iAmount
		#ifdef ENABLE_ALIGNMENT_BONUS
							, bool bStatus = false
		#endif
		);
		int					GetAlignment() const;

		int					GetRealAlignment() const;
		void				ShowAlignment(bool bShow);

		void				SetKillerMode(bool bOn);
		bool				IsKillerMode() const;
		void				UpdateKillerMode();

		BYTE				GetPKMode() const;
		void				SetPKMode(BYTE bPKMode);

		void				ItemDropPenalty(LPCHARACTER pkKiller);

		void				UpdateAggrPoint(LPCHARACTER ch, EDamageType type, int dam);

		//
		// HACK
		//
	public:
		void SetComboSequence(BYTE seq);
		BYTE GetComboSequence() const;

		void SetLastComboTime(DWORD time);
		DWORD GetLastComboTime() const;

		int GetValidComboInterval() const;
		void SetValidComboInterval(int interval);

		BYTE GetComboIndex() const;

		void IncreaseComboHackCount(int k = 1);
		void ResetComboHackCount();
		void SkipComboAttackByTime(int interval);
		DWORD GetSkipComboAttackByTime() const;

	protected:
		BYTE m_bComboSequence;
		DWORD m_dwLastComboTime;
		int m_iValidComboInterval;
		BYTE m_bComboIndex;
		int m_iComboHackCount;
		DWORD m_dwSkipComboAttackByTime;

	protected:
		void				UpdateAggrPointEx(LPCHARACTER ch, EDamageType type, int dam, TBattleInfo & info);
		void				ChangeVictimByAggro(int iNewAggro, LPCHARACTER pNewVictim);

		DWORD				m_dwFlyTargetID;
		std::vector<DWORD>	m_vec_dwFlyTargets;
		TDamageMap			m_map_kDamage;
//		AttackLog			m_kAttackLog;
		DWORD				m_dwKillerPID;

		int					m_iAlignment;		// Lawful/Chaotic value -200000 ~ 200000
		int					m_iRealAlignment;
		int					m_iKillerModePulse;
		BYTE				m_bPKMode;

		// Aggro
		DWORD				m_dwLastVictimSetTime;
		int					m_iMaxAggro;
		// End of Battle

		// Stone
	public:
		void				SetStone(LPCHARACTER pkChrStone);
		void				ClearStone();
		void				DetermineDropMetinStone();
		DWORD				GetDropMetinStoneVnum() const { return m_dwDropMetinStone; }
		BYTE				GetDropMetinStonePct() const { return m_bDropMetinStonePct; }

	protected:
		LPCHARACTER			m_pkChrStone;
		CHARACTER_SET		m_set_pkChrSpawnedBy;
		DWORD				m_dwDropMetinStone;
		BYTE				m_bDropMetinStonePct;
		// End of Stone

	public:
		enum
		{
			SKILL_UP_BY_POINT,
			SKILL_UP_BY_BOOK,
			SKILL_UP_BY_TRAIN,

			// ADD_GRANDMASTER_SKILL
			SKILL_UP_BY_QUEST,
			// END_OF_ADD_GRANDMASTER_SKILL
		};

		void				SkillLevelPacket();
		void				SkillLevelUp(DWORD dwVnum, BYTE bMethod = SKILL_UP_BY_POINT);
		bool				SkillLevelDown(DWORD dwVnum);
		// ADD_GRANDMASTER_SKILL
		bool				UseSkill(DWORD dwVnum, LPCHARACTER pkVictim, bool bUseGrandMaster = true);
		void				ResetSkill();
		void				SetSkillLevel(DWORD dwVnum, BYTE bLev);
		int					GetUsedSkillMasterType(DWORD dwVnum);

		bool				IsLearnableSkill(DWORD dwSkillVnum) const;
		// END_OF_ADD_GRANDMASTER_SKILL

		bool				CheckSkillHitCount(const BYTE SkillID, const VID dwTargetVID);
		bool				CanUseSkill(DWORD dwSkillVnum) const;
		bool				IsUsableSkillMotion(DWORD dwMotionIndex) const;
		int					GetSkillLevel(DWORD dwVnum) const;
		int					GetSkillMasterType(DWORD dwVnum) const;
		int					GetSkillPower(DWORD dwVnum, BYTE bLevel = 0) const;

		time_t				GetSkillNextReadTime(DWORD dwVnum) const;
		void				SetSkillNextReadTime(DWORD dwVnum, time_t time);
		void				SkillLearnWaitMoreTimeMessage(DWORD dwVnum);

		void				ComputePassiveSkill(DWORD dwVnum);
		int					ComputeSkill(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel = 0);
		#ifdef ENABLE_WOLFMAN_CHARACTER
		int					ComputeSkillParty(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel = 0);
		#endif
		int					ComputeSkillAtPosition(DWORD dwVnum, const PIXEL_POSITION& posTarget, BYTE bSkillLevel = 0);
		void				ComputeSkillPoints();

		void				SetSkillGroup(BYTE bSkillGroup);
		BYTE				GetSkillGroup() const		{ return m_points.skill_group; }

		int					ComputeCooltime(int time);

		void				GiveRandomSkillBook();

		void				DisableCooltime();
		bool				LearnSkillByBook(DWORD dwSkillVnum, BYTE bProb = 0);
		bool				LearnGrandMasterSkill(DWORD dwSkillVnum);

		#ifdef ENABLE_SKILL_COOLDOWN_CHECK
		bool				SkillIsOnCooldown(int vnum) { return m_SkillUseInfo[vnum].IsOnCooldown(vnum); }
		#endif
	private:
		bool				m_bDisableCooltime;
		DWORD				m_dwLastSkillTime;
		// End of Skill

		// MOB_SKILL
	public:
		bool				HasMobSkill() const;
		size_t				CountMobSkill() const;
		const TMobSkillInfo * GetMobSkill(unsigned int idx) const;
		bool				CanUseMobSkill(unsigned int idx) const;
		bool				UseMobSkill(unsigned int idx);
		void				ResetMobSkillCooltime();
	protected:
		DWORD				m_adwMobSkillCooltime[MOB_SKILL_MAX_NUM];
		// END_OF_MOB_SKILL

		// for SKILL_MUYEONG
	public:
		void				StartMuyeongEvent();
		void				StopMuyeongEvent();

	private:
		LPEVENT				m_pkMuyeongEvent;

		// for SKILL_CHAIN lighting
	public:
		int					GetChainLightningIndex() const { return m_iChainLightingIndex; }
		void				IncChainLightningIndex() { ++m_iChainLightingIndex; }
		void				AddChainLightningExcept(LPCHARACTER ch) { m_setExceptChainLighting.emplace(ch); }
		void				ResetChainLightningIndex() { m_iChainLightingIndex = 0; m_setExceptChainLighting.clear(); }
		int					GetChainLightningMaxCount() const;
		const CHARACTER_SET& GetChainLightingExcept() const { return m_setExceptChainLighting; }

	private:
		int					m_iChainLightingIndex;
		CHARACTER_SET m_setExceptChainLighting;

		// for SKILL_EUNHYUNG
	public:
		void				SetAffectedEunhyung();
		void				ClearAffectedEunhyung() { m_dwAffectedEunhyungLevel = 0; }
		bool				GetAffectedEunhyung() const { return m_dwAffectedEunhyungLevel; }

	private:
		DWORD				m_dwAffectedEunhyungLevel;

		//
		// Skill levels
		//
	protected:
		TPlayerSkill*					m_pSkillLevels;
		boost::unordered_map<BYTE, int>		m_SkillDamageBonus;
		std::map<int, TSkillUseInfo>	m_SkillUseInfo;

		////////////////////////////////////////////////////////////////////////////////////////
		// AI related
	public:
		void			AssignTriggers(const TMobTable * table);
		LPCHARACTER		GetVictim() const;
		void			SetVictim(LPCHARACTER pkVictim);
		LPCHARACTER		GetNearestVictim(LPCHARACTER pkChr);
		LPCHARACTER		GetProtege() const;

		bool			Follow(LPCHARACTER pkChr, float fMinimumDistance = 150.0f);
		bool			Return();
		bool			IsGuardNPC() const;
		bool			IsChangeAttackPosition(LPCHARACTER target) const;
		void			ResetChangeAttackPositionTime() { m_dwLastChangeAttackPositionTime = get_dword_time() - AI_CHANGE_ATTACK_POISITION_TIME_NEAR;}
		void			SetChangeAttackPositionTime() { m_dwLastChangeAttackPositionTime = get_dword_time();}

		bool			OnIdle();

		void			OnAttack(LPCHARACTER pkChrAttacker);
		void			OnClick(LPCHARACTER pkChrCauser);

		VID				m_kVIDVictim;

	protected:
		DWORD			m_dwLastChangeAttackPositionTime;
		CTrigger		m_triggerOnClick;
		// End of AI

		////////////////////////////////////////////////////////////////////////////////////////
		// Target
	protected:
		LPCHARACTER			m_pkChrTarget;
		CHARACTER_SET		m_set_pkChrTargetedBy;

	public:
		void				SetTarget(LPCHARACTER pkChrTarget);
		void				BroadcastTargetPacket();
		void				ClearTarget();
		void				CheckTarget();
		LPCHARACTER			GetTarget() const { return m_pkChrTarget; }

		////////////////////////////////////////////////////////////////////////////////////////
		// Safebox
	public:
		int					GetSafeboxSize() const;
		void				QuerySafeboxSize();
		void				SetSafeboxSize(int size);

		CSafebox *			GetSafebox() const;
		void				LoadSafebox(int iSize, DWORD dwGold, int iItemCount, TPlayerItem * pItems);
		void				ChangeSafeboxSize(BYTE bSize);
		void				CloseSafebox();

		void				ReqSafeboxLoad(const char* pszPassword);

		void				CancelSafeboxLoad( void ) { m_bOpeningSafebox = false; }

		void				SetMallLoadTime(int t) { m_iMallLoadTime = t; }
		int					GetMallLoadTime() const { return m_iMallLoadTime; }

		CSafebox *			GetMall() const;
		void				LoadMall(int iItemCount, TPlayerItem * pItems);
		void				CloseMall();

		void				SetSafeboxOpenPosition();
		float				GetDistanceFromSafeboxOpen() const;

	protected:
		CSafebox *			m_pkSafebox;
		int					m_iSafeboxSize;
		int					m_iSafeboxLoadTime;
		bool				m_bOpeningSafebox;

		CSafebox *			m_pkMall;
		int					m_iMallLoadTime;

		PIXEL_POSITION		m_posSafeboxOpen;

		////////////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////////////////////
		// Mounting
	public:
		void				MountVnum(DWORD vnum);
		DWORD				GetMountVnum() const { return m_dwMountVnum; }
		DWORD				GetLastMountTime() const { return m_dwMountTime; }

		bool				CanUseHorseSkill();

		// Horse
		virtual	void		SetHorseLevel(int iLevel);

		virtual	bool		StartRiding();
		virtual	bool		StopRiding();

		virtual	DWORD		GetMyHorseVnum() const;

		virtual	void		HorseDie();
		virtual bool		ReviveHorse();

		virtual void		SendHorseInfo();
		virtual	void		ClearHorseInfo();

		void				HorseSummon(bool bSummon, bool bFromFar = false, DWORD dwVnum = 0, const char* name = 0);

		LPCHARACTER			GetHorse() const			{ return m_chHorse; }
		LPCHARACTER			GetRider() const; // rider on horse
		void				SetRider(LPCHARACTER ch);

		bool				IsRiding() const;

	#ifdef __PET_SYSTEM__
	public:
		CPetSystem*			GetPetSystem()	{ return m_petSystem; }
		void				SetPet()		{ m_bIsPet = true; }
		bool				IsPet()			{ return m_bIsPet; }
		bool				PetBlockMap();

	protected:
		CPetSystem*			m_petSystem;
		bool				m_bIsPet;
	#endif

	protected:
		LPCHARACTER			m_chHorse;
		LPCHARACTER			m_chRider;

		DWORD				m_dwMountVnum;
		DWORD				m_dwMountTime;

		BYTE				m_bSendHorseLevel;
		BYTE				m_bSendHorseHealthGrade;
		BYTE				m_bSendHorseStaminaGrade;

		////////////////////////////////////////////////////////////////////////////////////////
		// Detailed Log
	public:
		void				DetailLog() { m_bDetailLog = !m_bDetailLog; }
		void				ToggleMonsterLog();
		void				MonsterLog(const char* format, ...);
	private:
		bool				m_bDetailLog;
		bool				m_bMonsterLog;

		////////////////////////////////////////////////////////////////////////////////////////
		// Empire

	public:
		void 				SetEmpire(BYTE bEmpire);
		BYTE				GetEmpire() const { return m_bEmpire; }

	protected:
		BYTE				m_bEmpire;

		////////////////////////////////////////////////////////////////////////////////////////
		// Regen
	public:
		void				SetRegen(LPREGEN pkRegen);

	protected:
		PIXEL_POSITION			m_posRegen;
		float				m_fRegenAngle;
		LPREGEN				m_pkRegen;
		size_t				regen_id_; // to help dungeon regen identification
		// End of Regen

		////////////////////////////////////////////////////////////////////////////////////////
		// Resists & Proofs
	public:
		bool				CannotMoveByAffect() const;
		bool				IsImmune(DWORD dwImmuneFlag);
		void				SetImmuneFlag(DWORD dw) { m_pointsInstant.dwImmuneFlag = dw; }

	protected:
		void				ApplyMobAttribute(const TMobTable* table);
		// End of Resists & Proofs

		////////////////////////////////////////////////////////////////////////////////////////
		// QUEST
		//
	public:
		void				SetQuestNPCID(DWORD vid);
		DWORD				GetQuestNPCID() const { return m_dwQuestNPCVID; }
		LPCHARACTER			GetQuestNPC() const;

		void				SetQuestItemPtr(LPITEM item);
		void				ClearQuestItemPtr();
		LPITEM				GetQuestItemPtr() const;

		#ifdef ENABLE_QUEST_DND_EVENT
		void				SetQuestDNDItemPtr(LPITEM item);
		void				ClearQuestDNDItemPtr();
		LPITEM				GetQuestDNDItemPtr() const;
		#endif

		void				SetQuestBy(DWORD dwQuestVnum)	{ m_dwQuestByVnum = dwQuestVnum; }
		DWORD				GetQuestBy() const			{ return m_dwQuestByVnum; }

		int					GetQuestFlag(const std::string& flag) const;
		void				SetQuestFlag(const std::string& flag, int value);

		void				ConfirmWithMsg(const char* szMsg, int iTimeout, DWORD dwRequestPID);

	private:
		DWORD				m_dwQuestNPCVID;
		DWORD				m_dwQuestByVnum;
		LPITEM				m_pQuestItem;
		#ifdef ENABLE_QUEST_DND_EVENT
		LPITEM				m_pQuestDNDItem{nullptr};
		#endif

	// Events
	public:
		bool				StartStateMachine(int iPulse = 1);
		void				StopStateMachine();
		void				UpdateStateMachine(DWORD dwPulse);
		void				SetNextStatePulse(int iPulseNext);
		void				UpdateCharacter(DWORD dwPulse);

	protected:
		DWORD				m_dwNextStatePulse;

		// Marriage
	public:
		LPCHARACTER			GetMarryPartner() const;
		void				SetMarryPartner(LPCHARACTER ch);
		int					GetMarriageBonus(DWORD dwItemVnum, bool bSum = true);

		void				SetWeddingMap(marriage::WeddingMap* pMap);
		marriage::WeddingMap* GetWeddingMap() const { return m_pWeddingMap; }

	private:
		marriage::WeddingMap* m_pWeddingMap;
		LPCHARACTER			m_pkChrMarried;

		// Warp Character
	public:
		void				StartWarpNPCEvent();

	public:
		void				StartSaveEvent();
		void				StartRecoveryEvent();
		void				StartCheckSpeedHackEvent();
		void				StartDestroyWhenIdleEvent();

		LPEVENT				m_pkDeadEvent;
		LPEVENT				m_pkStunEvent;
		LPEVENT				m_pkSaveEvent;
		LPEVENT				m_pkRecoveryEvent;
		LPEVENT				m_pkTimedEvent;
		LPEVENT				m_pkFishingEvent;
		LPEVENT				m_pkAffectEvent;
		LPEVENT				m_pkPoisonEvent;
		#ifdef ENABLE_WOLFMAN_CHARACTER
		LPEVENT				m_pkBleedingEvent;
		#endif
		LPEVENT				m_pkFireEvent;
		LPEVENT				m_pkWarpNPCEvent;
		//DELAYED_WARP
		//END_DELAYED_WARP

		// MINING
		LPEVENT				m_pkMiningEvent;
		// END_OF_MINING
		LPEVENT				m_pkWarpEvent;
		LPEVENT				m_pkCheckSpeedHackEvent;
		LPEVENT				m_pkDestroyWhenIdleEvent;
		LPEVENT				m_pkPetSystemUpdateEvent;

		bool IsWarping() const { return m_pkWarpEvent ? true : false; }

		bool				m_bHasPoisoned;
		#ifdef ENABLE_WOLFMAN_CHARACTER
		bool				m_bHasBled;
		#endif

		const CMob *		m_pkMobData;
		CMobInstance *		m_pkMobInst;

		std::map<int, LPEVENT> m_mapMobSkillEvent;

		friend struct FuncSplashDamage;
		friend struct FuncSplashAffect;
		friend class CFuncShoot;

	public:
		int				GetPremiumRemainSeconds(BYTE bType) const;

	private:
		int				m_aiPremiumTimes[PREMIUM_MAX_NUM];

		// CHANGE_ITEM_ATTRIBUTES
		static const char		msc_szLastChangeItemAttrFlag[];
		// END_OF_CHANGE_ITEM_ATTRIBUTES

		// NEW_HAIR_STYLE_ADD
	public :
		bool ItemProcess_Hair(LPITEM item, int iDestCell);
		// END_NEW_HAIR_STYLE_ADD

	public :
		void ClearSkill();
		void ClearSubSkill();

		// RESET_ONE_SKILL
		bool ResetOneSkill(DWORD dwVnum);
		// END_RESET_ONE_SKILL

	private :
		void SendDamagePacket(LPCHARACTER pAttacker, int Damage, BYTE DamageFlag);

	// ARENA
	private :
		CArena *m_pArena;
		bool m_ArenaObserver;
		int m_nPotionLimit;

	public :
		void 	SetArena(CArena* pArena) { m_pArena = pArena; }
		void	SetArenaObserverMode(bool flag) { m_ArenaObserver = flag; }

		CArena* GetArena() const { return m_pArena; }
		bool	GetArenaObserverMode() const { return m_ArenaObserver; }

		void	SetPotionLimit(int count) { m_nPotionLimit = count; }
		int		GetPotionLimit() const { return m_nPotionLimit; }
	// END_ARENA

		//PREVENT_TRADE_WINDOW
	public:
		bool	IsOpenSafebox() const { return m_isOpenSafebox ? true : false; }
		void 	SetOpenSafebox(bool b) { m_isOpenSafebox = b; }

		bool	IsOpenMall() const { return GetMall() ? true : false; }

		int		GetSafeboxLoadTime() const { return m_iSafeboxLoadTime; }
		void	SetSafeboxLoadTime() { m_iSafeboxLoadTime = thecore_pulse(); }
		//END_PREVENT_TRADE_WINDOW

	private:
		bool	m_isOpenSafebox;

	//PREVENT_REFINE_WINDOW
	public:
		bool	IsOpenRefine() const { return m_isOpenRefine ? true : false; }
		void	SetOpenRefine(bool b) { m_isOpenRefine = b; }

	private:
		bool	m_isOpenRefine;
	//END_PREVENT_REFINE_WINDOW

	public:
		int		GetSkillPowerByLevel(int level, bool bMob = false) const;

		//PREVENT_REFINE_HACK
		int		GetRefineTime() const { return m_iRefineTime; }
		void	SetRefineTime() { m_iRefineTime = thecore_pulse(); }
		int		m_iRefineTime;
		//END_PREVENT_REFINE_HACK

		//RESTRICT_USE_SEED_OR_MOONBOTTLE
		int 	GetUseSeedOrMoonBottleTime() const { return m_iSeedTime; }
		void  	SetUseSeedOrMoonBottleTime() { m_iSeedTime = thecore_pulse(); }
		int 	m_iSeedTime;
		//END_RESTRICT_USE_SEED_OR_MOONBOTTLE

		//PREVENT_PORTAL_AFTER_EXCHANGE
		int		GetExchangeTime() const { return m_iExchangeTime; }
		void	SetExchangeTime() { m_iExchangeTime = thecore_pulse(); }
		int		m_iExchangeTime;
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

		int		m_iMyShopTime;
		int		GetMyShopTime() const	{ return m_iMyShopTime; }
		void	SetMyShopTime() { m_iMyShopTime = thecore_pulse(); }
		bool	IsMyShopTime();

		int		m_iMyExchangeTime;
		int		GetMyExchangeTime() const	{ return m_iMyExchangeTime; }
		void	SetMyExchangeTime() { m_iMyExchangeTime = thecore_pulse(); }
		bool	IsExchangeTime();

		bool	PreventTradeWindow(int flags, bool except = false) const;

		bool	m_bWarpStatus;
		bool	IsWarpStatus() const	{ return m_bWarpStatus; }
		void	SetWarpStatus(bool status) { m_bWarpStatus = status; }

		bool	IsHack(bool bSendMsg = true, bool bCheckShopOwner = true, int limittime = g_nPortalLimitTime);

		// MONARCH
		BOOL	IsMonarch() const;
		// END_MONARCH
		void Say(const std::string & s);

		enum MONARCH_COOLTIME
		{
			MC_HEAL = 10,
			MC_WARP	= 60,
			MC_TRANSFER = 60,
			MC_TAX = (60 * 60 * 24 * 7),
			MC_SUMMON = (60 * 60),
		};

		enum MONARCH_INDEX
		{
			MI_HEAL = 0,
			MI_WARP,
			MI_TRANSFER,
			MI_TAX,
			MI_SUMMON,
			MI_MAX
		};

		DWORD m_dwMonarchCooltime[MI_MAX];
		DWORD m_dwMonarchCooltimelimit[MI_MAX];

		void InitMC();
		DWORD GetMC(enum MONARCH_INDEX e) const;
		void SetMC(enum MONARCH_INDEX e);
		bool IsMCOK(enum MONARCH_INDEX e) const;
		DWORD GetMCL(enum MONARCH_INDEX e) const;
		DWORD GetMCLTime(enum MONARCH_INDEX e) const;

	public:
		bool ItemProcess_Polymorph(LPITEM item);

		LPITEM*	GetCubeItem() { return (m_PlayerSlots) ? m_PlayerSlots->pCubeItems.data() : nullptr; }
		bool IsCubeOpen () const	{ return (m_pointsInstant.pCubeNpc); }
		void SetCubeNpc(LPCHARACTER npc)	{ m_pointsInstant.pCubeNpc = npc; }
		bool CanDoCube() const;

		#ifdef ENABLE_CUBE_RENEWAL
		DWORD GetCubeNpcRace() const	{ return m_dwCubeNpcRaceNum; }
		void SetCubeNpcRace(DWORD dwRace)	{ m_dwCubeNpcRaceNum = dwRace; }
		#endif

	public:
		bool IsSiegeNPC() const;

	private:
		e_overtime m_eOverTime;
		#ifdef ENABLE_CUBE_RENEWAL
		DWORD m_dwCubeNpcRaceNum;
		#endif

	public:
		bool IsOverTime(e_overtime e) const { return (e == m_eOverTime); }
		void SetOverTime(e_overtime e) { m_eOverTime = e; }

	private:
		int		m_deposit_pulse;

	public:
		void	UpdateDepositPulse();
		bool	CanDeposit() const;

	private:
		void	__OpenPrivateShop();

	public:
		struct AttackedLog
		{
			DWORD 	dwPID;
			DWORD	dwAttackedTime;

			AttackedLog() : dwPID(0), dwAttackedTime(0)
			{
			}
		};

		AttackLog	m_kAttackLog;
		AttackedLog m_AttackedLog;
		int			m_speed_hack_count;

	private :
		std::string m_strNewName;

	public :
		const std::string GetNewName() const { return this->m_strNewName; }
		void SetNewName(const std::string name) { this->m_strNewName = name; }

	public :
		void GoHome();

	private :
		std::set<DWORD>	m_known_guild;

	public :
		void SendGuildName(CGuild* pGuild);
		void SendGuildName(DWORD dwGuildID);

	private :
		DWORD m_dwLogOffInterval;

	public :
		DWORD GetLogOffInterval() const { return m_dwLogOffInterval; }

	public:
		bool UnEquipSpecialRideUniqueItem ();
		bool CanWarp() const;

	private:
		DWORD m_dwLastGoldDropTime;

	#ifdef ENABLE_NEWSTUFF
		DWORD m_dwLastItemDropTime;
		DWORD m_dwLastBoxUseTime;
		DWORD m_dwLastBuySellTime;

	public:
		DWORD GetLastBuySellTime() const { return m_dwLastBuySellTime; }
		void SetLastBuySellTime(DWORD dwLastBuySellTime) { m_dwLastBuySellTime = dwLastBuySellTime; }
	#endif

	public:
		void AutoRecoveryItemProcess (const EAffectTypes);

	public:
		void BuffOnAttr_AddBuffsFromItem(LPITEM pItem);
		void BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem);

	private:
		void BuffOnAttr_ValueChange(BYTE bType, BYTE bOldValue, BYTE bNewValue);
		void BuffOnAttr_ClearAll();

		typedef std::map <BYTE, CBuffOnAttributes*> TMapBuffOnAttrs;
		TMapBuffOnAttrs m_map_buff_on_attrs;

	public:
		void SetArmada() { cannot_dead = true; }
		void ResetArmada() { cannot_dead = false; }

	private:
		bool cannot_dead;

	#ifdef NEW_ICEDAMAGE_SYSTEM
	private:
		DWORD m_dwNDRFlag;
		std::set<DWORD> m_setNDAFlag;

	public:
		const DWORD GetNoDamageRaceFlag();
		void SetNoDamageRaceFlag(DWORD dwRaceFlag);
		void UnsetNoDamageRaceFlag(DWORD dwRaceFlag);
		void ResetNoDamageRaceFlag();
		const std::set<DWORD> & GetNoDamageAffectFlag();
		void SetNoDamageAffectFlag(DWORD dwAffectFlag);
		void UnsetNoDamageAffectFlag(DWORD dwAffectFlag);
		void ResetNoDamageAffectFlag();
	#endif

	private:
		float m_fAttMul;
		float m_fDamMul;

	public:
		float GetAttMul() { return this->m_fAttMul; }
		void SetAttMul(float newAttMul) {this->m_fAttMul = newAttMul; }
		float GetDamMul() { return this->m_fDamMul; }
		void SetDamMul(float newDamMul) {this->m_fDamMul = newDamMul; }

	private:
		bool IsValidItemPosition(TItemPos Pos) const;

	public:
		void	DragonSoul_Initialize();
		bool	DragonSoul_IsQualified() const;
		void	DragonSoul_GiveQualification();
		int		DragonSoul_GetActiveDeck() const;
		bool	DragonSoul_IsDeckActivated() const;
		bool	DragonSoul_ActivateDeck(int deck_idx);
		void	DragonSoul_DeactivateAll();
		void	DragonSoul_CleanUp();

	public:
		bool		DragonSoul_RefineWindow_Open(LPENTITY pEntity);
		bool		DragonSoul_RefineWindow_Close();
		LPENTITY	DragonSoul_RefineWindow_GetOpener() { return  m_pointsInstant.m_pDragonSoulRefineWindowOpener; }
		bool		DragonSoul_RefineWindow_CanRefine();

	private:
		unsigned int itemAward_vnum;
		char		 itemAward_cmd[20];
		//bool		 itemAward_flag;

	public:
		unsigned int GetItemAward_vnum() { return itemAward_vnum; }
		char*		 GetItemAward_cmd() { return itemAward_cmd;	  }
		//bool		 GetItemAward_flag() { return itemAward_flag; }
		void		 SetItemAward_vnum(unsigned int vnum) { itemAward_vnum = vnum; }
		void		 SetItemAward_cmd(char* cmd) { strcpy(itemAward_cmd,cmd); }
		//void		 SetItemAward_flag(bool flag) { itemAward_flag = flag; }

	#ifdef ENABLE_ANTI_CMD_FLOOD
	private:
		int m_dwCmdAntiFloodPulse;
		DWORD m_dwCmdAntiFloodCount;

	public:
		int GetCmdAntiFloodPulse(){return m_dwCmdAntiFloodPulse;}
		DWORD GetCmdAntiFloodCount(){return m_dwCmdAntiFloodCount;}
		DWORD IncreaseCmdAntiFloodCount(){return ++m_dwCmdAntiFloodCount;}
		void SetCmdAntiFloodPulse(int dwPulse){m_dwCmdAntiFloodPulse=dwPulse;}
		void SetCmdAntiFloodCount(DWORD dwCount){m_dwCmdAntiFloodCount=dwCount;}
	#endif

	#ifdef ENABLE_MULTI_FARM_BLOCK
	public:
		bool				GetRewardStatus() { return m_bmultiFarmStatus; }
		void				SetRewardStatus(bool bValue) { m_bmultiFarmStatus = bValue; }
		LPEVENT				m_pkCheckMultiFarmEvent;

	protected:
		bool				m_bmultiFarmStatus;
	#endif

	#ifdef ENABLE_MULTI_HUNT_BLOCK
	public:
		bool				GetMultiHuntStatus() { return m_bmultiHuntStatus; }
		void				SetMultiHuntStatus(bool bValue);
		LPEVENT				m_pkCheckMultiHuntEvent;

	protected:
		bool				m_bmultiHuntStatus;
	#endif

	#ifdef ENABLE_MULTI_FISH_BLOCK
	public:
		bool				GetMultiFishStatus() { return m_bmultiFishStatus; }
		void				SetMultiFishStatus(bool bValue) { m_bmultiFishStatus = bValue; }
		LPEVENT				m_pkCheckMultiFishEvent;

	protected:
		bool				m_bmultiFishStatus;
	#endif

	#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
	public:
		bool			UpdateCoin(DWORD dwAID, long val);
		DWORD			GetCoinNm();

		bool			UpdateCash(DWORD dwAID, long val);
		DWORD			GetCashNm();

		#ifdef ENABLE_DRAGON_COIN_EM_MARKET
		bool			UpdateEm(DWORD dwAID, long val);
		DWORD			GetEmNm();
		#endif

		void			RefreshCoinAndCash(bool isNow = true);

		time_t			GetDcShopLastTime() const { return m_iDcShopLastTime; }
		void			SetDcShopLastTime(time_t time) { m_iDcShopLastTime = time; }

		#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
		time_t			GetDcShopAdminLastTime() const { return m_iDcShopAdminLastTime; }
		void			SetDcShopAdminLastTime(time_t time) { m_iDcShopAdminLastTime = time; }
		#endif

	private:
		time_t			m_iDcShopLastTime;
		#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
		time_t			m_iDcShopAdminLastTime;
		#endif
		LPEVENT			m_pkRefreshCoinAndCash;
	#endif

	public:
		void			SetLastSyncTime(const timeval &tv) { memcpy(&m_tvLastSyncTime, &tv, sizeof(timeval)); }
		const timeval&	GetLastSyncTime() { return m_tvLastSyncTime; }
		void			SetSyncHackCount(int iCount) { m_iSyncHackCount = iCount;}
		int				GetSyncHackCount() { return m_iSyncHackCount; }

	private:
		timeval			m_tvLastSyncTime;
		int				m_iSyncHackCount;

	// @fixme188 BEGIN
	public:
		void ResetRewardInfo()
		{
			m_map_kDamage.clear();
			if (!IsPC())
				SetExp(0);
		}

		void DeadNoReward()
		{
			if (!IsDead())
				ResetRewardInfo();
			Dead();
		}
	// @fixme188 END

	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	protected:
		bool	m_bAcceCombination, m_bAcceAbsorption;

	public:
		bool	IsAcceOpened(bool bCombination) const {return bCombination ? m_bAcceCombination : m_bAcceAbsorption;}
		bool	IsAcceOpened() const {return (m_bAcceCombination || m_bAcceAbsorption);}
		void	OpenAcce(bool bCombination);
		void	CloseAcce();
		void	ClearAcceMaterials();
		bool	CleanAcceAttr(LPITEM pkItem, LPITEM pkTarget);
		std::vector<LPITEM>	GetAcceMaterials();
		const TItemPosEx*	GetAcceMaterialsInfo();
		void	SetAcceMaterial(int pos, LPITEM ptr);
		bool	AcceIsSameGrade(long lGrade);
		DWORD	GetAcceCombinePrice(long lGrade);
		void	GetAcceCombineResult(DWORD & dwItemVnum, DWORD & dwMinAbs, DWORD & dwMaxAbs);
		BYTE	CheckEmptyMaterialSlot();
		void	AddAcceMaterial(TItemPos tPos, BYTE bPos);
		void	RemoveAcceMaterial(BYTE bPos);
		BYTE	CanRefineAcceMaterials();
		void	RefineAcceMaterials();
	#endif

	#ifdef ENABLE_STYLE_ATTRIBUTE_SYSTEM
	public:
		bool	UseItemNewAttribute(TItemPos source_pos, TItemPos target_pos, BYTE * bValues);
	#endif

	#ifdef ENABLE_KADIM_EFSUN
	public:
		void	efsunudegis(int iEfsun, DWORD dwItemID);
	#endif

	#ifdef ENABLE_HIDE_COSTUME_PART
	public:
		void	UpdateCostumeVisibilityStatus();
		void SetBodyCostumeHidden(bool hidden);
		bool IsBodyCostumeHidden() const { return m_bHideBodyCostume; };

		void SetHairCostumeHidden(bool hidden);
		bool IsHairCostumeHidden() const { return m_bHideHairCostume; };
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		void SetWeaponCostumeHidden(bool hidden);
		bool IsWeaponCostumeHidden() const { return m_bHideWeaponCostume; };
#endif

	private:
		bool m_bHideBodyCostume;
		bool m_bHideHairCostume;
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		bool m_bHideWeaponCostume;
	#endif
	#endif
	#ifdef ENABLE_SKILL_RENEWAL
	public:
		const	DWORD* GetUsableSkillList() const;
		BYTE	GetSkillBookReadCount(DWORD skillVnum);
		BYTE	GetSkillRemainCount(DWORD skillVnum);
		void	SendSkillBookCount();
	#endif

	public:
		void			SetProtectTime(const std::string& flagname, int value);
		int				GetProtectTime(const std::string& flagname) const;

	protected:
		std::map<std::string, int>	m_protection_Time;

	public:
		bool			IsOpenWindows() const;

	#ifdef ENABLE_BIYOLOG
	public:
		void			CheckBio();
		void			CheckBiologDropItem(DWORD dwRace, int iGetLevel);
	#endif

	#ifdef ENABLE_PACKET_FLOOD
	public:
		int				analyze_protect;
		int				analyze_protect_other_count;
	#endif

	#ifdef __RENEWAL_BRAVE_CAPE__
	public:
		void			GetBraveCapeCMDCompare(const char* c_pszArgument);
		LPEVENT			m_pkBraveCapeEvent;
	#endif

	#ifdef ENABLE_BATTLE_PASS
	public:
		typedef std::list<TPlayerBattlePassMission*> ListBattlePassMap;
		void			LoadBattlePass(DWORD dwCount, TPlayerBattlePassMission* data);
		DWORD			GetMissionProgress(uint8_t dwMissionID, DWORD dwBattlePassID);
		void			UpdateMissionProgress(uint8_t dwMissionID, DWORD dwBattlePassID, DWORD dwUpdateValue, DWORD dwTotalValue, bool isOverride = false);
		bool			IsCompletedMission(uint8_t bMissionID);
		bool			IsCompletedBattlePassID(uint8_t bMissionID, uint8_t bBattlePassID);
		void			ForceCompleteBPMission(uint8_t bMissionID, DWORD dwTotalValue);
		bool			IsLoadedBattlePass() const { return m_bIsLoadedBattlePass; }
		uint8_t			GetBattlePassId();
		void			ClearBattlePass();
		void			SaveBattlePass();

	private:
		bool			m_bIsLoadedBattlePass;
		ListBattlePassMap	m_listBattlePass;

		DWORD			m_dwBattlePassTime;
		bool			m_bBattlePassSave;
	#endif

	#ifdef ENABLE_FISHING_SECURE
	public:
		bool			IsCheckWater() const;
		void			SecureFishing();

		void			SetFishingSecureStatus(bool status) { m_bFishingSecure = status; }
		bool			GetFishingSecureStatus() { return m_bFishingSecure; }

		void			SetOldPkMode(BYTE value) { m_bOldPKMode = value; }
		BYTE			GetOldPkMode() { return m_bOldPKMode; }

	private:
		bool			m_bFishingSecure;
		BYTE			m_bOldPKMode;
	#endif

	#ifdef ENABLE_CHECK_SECURE_ZONE
	public:
		void			CheckSecurityZone();
		void			CheckSecurityZone2();

		void			SetDamageSecureStatus(bool status) { m_bDamageSecure = status; }
		bool			GetDamageSecureStatus() { return m_bDamageSecure; }

	private:
		bool			m_bDamageSecure;
	#endif

	public:
		void			SetStopDamage(bool status) { m_bStopDamage = status; }
		bool			GetStopDamage() { return m_bStopDamage; }

	private:
		bool			m_bStopDamage;

	#ifdef ENABLE_CAPTCHA_SELECT_SYSTEM
	public:
		void	CaptchaOpen();
		void	CaptchaReply(int iReply);
		bool	CaptchaQuestion(bool bLogin = false);
		bool	CaptchaQuestionDamage();

		LPEVENT	m_pkCaptchaEvent;

		void	SetCaptchaTime(int time) { m_iCaptchaTime = time; }
		int		GetCaptchaTime() { return m_iCaptchaTime; }

		void	SetCaptchaAttackStatus(bool b) { m_bCaptchaStatus = b; }
		bool	GetCaptchaAttackStatus() { return m_bCaptchaStatus; }

		void	SetCaptchaFalseReplyCount(BYTE count) { m_bCaptchaFalseReplyCount = count; }
		BYTE	GetCaptchaFalseReplyCount() { return m_bCaptchaFalseReplyCount; }

		void	SetCaptchaSkip(bool b) { m_bCaptchaSkip = b; }
		bool	GetCaptchaSkip() { return m_bCaptchaSkip; }

	protected:
		int		m_iCaptchaReplyImage;
		int		m_iCaptchaTime;
		bool	m_bCaptchaStatus;
		BYTE	m_bCaptchaFalseReplyCount;
		bool	m_bCaptchaSkip;
	#endif

	#ifdef __AUTO_HUNT__
	public:
		void			GetAutoHuntCommand(const char* szArgument);
		bool			SetAutoHuntStatus(bool bStatus, bool bMobFarm = false, bool bMetinFarm = false);
		bool			IsAutoHuntAffectHas();

	protected:
		bool			m_bAutoHuntStatus;
		time_t			m_dwAutoHuntStartTime;
	#endif

	#ifdef ENABLE_CHANGELOOK_SYSTEM
	protected:
		bool	m_bChangeLook;

	public:
		bool	isChangeLookOpened() const {return m_bChangeLook;}
		void	ChangeLookWindow(bool bOpen = false, bool bRequest = false);
		void	ClearClWindowMaterials();

		LPITEM*	GetClWindowMaterials() { return (m_PlayerSlots) ? m_PlayerSlots->pClMaterials.data() : nullptr; }
		BYTE	CheckClEmptyMaterialSlot();
		void	AddClMaterial(TItemPos tPos, BYTE bPos);
		void	RemoveClMaterial(BYTE bPos);
		void	RefineClMaterials();
		bool	CleanTransmutation(LPITEM pkItem, LPITEM pkTarget);
	#endif

	#ifdef BOSS_DMG_RANKING
	public:
		BYTE			GetOriginalRace() { return m_points.job; }
		void			CreatePacketTimer();
		void			StopPacketTimer(const char * szReason);
		void			MarkAsSpecialBoss(bool isMark) { m_bSpecialBoss = isMark; }
		bool			IsSpecialBoss() { return m_bSpecialBoss; }
		void			SendRankingPacket();

		void			SetDmgRankingFirstPID(DWORD pid) { m_dwDmgRankingFirstPID = pid; }
		DWORD			GetDmgRankingFirstPID() const { return m_dwDmgRankingFirstPID; }

		void			RewardDmgRankingFirstPID();

	private:
		bool			m_bSpecialBoss;
		LPEVENT			packetTimer;
		DWORD			m_dwDmgRankingFirstPID;
	#endif

	#ifdef ENABLE_EVOLUTION_SYSTEM
	public:
		void			ResetEvoStoneCounter() { m_bEvoStoneCounter = 0; }
		BYTE			IncreaseEvoStoneCounter() { return ++m_bEvoStoneCounter; }

	private:
		BYTE			m_bEvoStoneCounter;
	#endif

	#ifdef ENABLE_ACCELERATED_DEATH
	public:
		void			RemoveMobs();
	#endif

	#ifdef ENABLE_REDUCED_ENTITY_VIEW
	public:
		bool			IsMobShow(LPCHARACTER pkTarget) const;

		bool			IsStoneInMob() const { return m_bIsStoneInMob; }
		void			SetStoneInMob(bool b) { m_bIsStoneInMob = b; }

		bool			IsNoReducedEntity() const { return m_bIsNoReducedEntity; }
		void			SetNoReducedEntity(bool b) { m_bIsNoReducedEntity = b; }

		void			SetMapIDX(long l)	{ m_lMapIDX = l; }
		long			GetMapIDX() const	{ return m_lMapIDX; }
		int				GetRealMapIDX() const { return (m_lMapIDX >= 10000) ? (m_lMapIDX / 10000) : m_lMapIDX; }

	private:
		bool			m_bIsStoneInMob;
		bool			m_bIsNoReducedEntity;
		long			m_lMapIDX;
	#endif

	#ifdef ENABLE_ALIGNMENT_BONUS
	public:
		BYTE			GetAlignmentLevel() const			{ return m_points.alignment_level; }
		void			SetAlignmentLevel(BYTE align_level)	{ m_points.alignment_level = align_level; }

		void			UpdateAlignmentBonus(int iAlign);
		void			CheckAlignmentBonus();
	#endif

	#ifdef ENABLE_PLAYER_TAG_SYSTEM
	public:
		void			LoadPlayerTags();
	#endif

	#ifdef ENABLE_BLACK_MARKET_SYSTEM
	public:
		bool			BlackMarket(TItemPos Cell);
	#endif

	#ifdef ENABLE_DEFENCE_SHIELD
	public:
		void			SetDefenceShield(bool status) { m_bDefenceShield = status; };
		bool			GetDefenceShield() const;

	private:
		bool			m_bDefenceShield;
	#endif

	#ifdef ENABLE_MOVE_CHANNEL
	public:
		bool			ChangeChannel(BYTE newChannel);
	#endif

	#ifdef NEW_MOB_TIMER
	public:
		void			CheckSecurityMobRange();

		void			SetSpawnX(long lX)	{ m_lSpawnX = lX; }
		void			SetSpawnY(long lY)	{ m_lSpawnY = lY; }
		long			GetSpawnX() const	{ return m_lSpawnX; }
		long			GetSpawnY() const	{ return m_lSpawnY; }

	private:
		long			m_lSpawnX;
		long			m_lSpawnY;
	#endif

	#ifdef ENABLE_BOSS_HUMAN_ATTR
	public:
		bool			IsBossHumanAttr() const;
	#endif

	#ifdef ENABLE_REWARD_SYSTEM
	public:
		void			UpdateStoneReward(DWORD dwRaceNum);
		void			UpdateBossReward(DWORD dwRaceNum);
		#ifdef ENABLE_PASIF_SKILL
		void			SetPassiveSkillAllMaxLevel();
		#endif
	#endif
	#ifdef ENABLE_OKEY_CARD_SYSTEM
	public:
		struct S_CARD
		{
			DWORD	type;
			DWORD	value;
		};

		struct CARDS_INFO
		{
			S_CARD cards_in_hand[MAX_CARDS_IN_HAND];
			S_CARD cards_in_field[MAX_CARDS_IN_FIELD];
			DWORD	cards_left;
			DWORD	field_points;
			DWORD	points;
		};

		void			Cards_open(DWORD safemode);
		void			Cards_clean_list();
		DWORD			GetEmptySpaceInHand();
		void			Cards_pullout();
		void			RandomizeCards();
		bool			CardWasRandomized(DWORD type, DWORD value);
		void			SendUpdatedInformations();
		void			SendReward();
		void			CardsDestroy(DWORD reject_index);
		void			CardsAccept(DWORD accept_index);
		void			CardsRestore(DWORD restore_index);
		DWORD			GetEmptySpaceInField();
		DWORD			GetAllCardsCount();
		bool			TypesAreSame();
		bool			ValuesAreSame();
		bool			CardsMatch();
		DWORD			GetLowestCard();
		bool			CheckReward();
		void			CheckCards();
		void			RestoreField();
		void			ResetField();
		void			CardsEnd();
		void			GetGlobalRank(char * buffer, size_t buflen);
		void			GetRundRank(char * buffer, size_t buflen);

	protected:
		CARDS_INFO		character_cards;
		S_CARD			randomized_cards[24];
	#endif

	#ifdef ENABLE_AUTO_ITEM_SELL_SYSTEM
	public:
		void			SendAutoItemSell();
		void			AddAutoItemSell(const DWORD itemVnum);
		void			RemoveAutoItemSell(const DWORD itemVnum);
		void			ResetAutoItemSell();
		void			InvSellAutoItemSell();
		bool			IsAutoItemSell(
							const DWORD itemVnum
							#ifdef ENABLE_NEW_MAX_COUNT
							, const short itemCount
							#else
							, const BYTE itemCount
							#endif
							, bool isSell = true
						);

		void			SetAutoSellItem(bool status) { m_autoSellStatus = status; }
		bool			GetAutoSellItem() const { return m_autoSellStatus; }

	private:
		bool			m_autoSellStatus;
		std::set<DWORD>	m_autoSellList;
	#endif
};

ESex GET_SEX(LPCHARACTER ch);

#endif
