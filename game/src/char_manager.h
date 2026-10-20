#ifndef __INC_METIN_II_GAME_CHARACTER_MANAGER_H__
#define __INC_METIN_II_GAME_CHARACTER_MANAGER_H__

#ifdef M2_USE_POOL
#include "pool.h"
#endif

#ifdef ENABLE_EVENT_MANAGER
#include "buffer_manager.h"
#endif

#include "../../common/stl.h"
#include "../../common/length.h"

#include "vid.h"
#include "packet.h"

class CDungeon;
class CHARACTER;
class CharacterVectorInteractor;

class CHARACTER_MANAGER : public singleton<CHARACTER_MANAGER>
{
	public:
		typedef std::unordered_map<std::string, LPCHARACTER> NAME_MAP;

		CHARACTER_MANAGER();
		virtual ~CHARACTER_MANAGER();

		void			Destroy();

		void			GracefulShutdown();

		DWORD			AllocVID();

		LPCHARACTER		CreateCharacter(const char * name, DWORD dwPID = 0);

		#ifndef DEBUG_ALLOC
		void DestroyCharacter(LPCHARACTER ch);
		#else
		void DestroyCharacter(LPCHARACTER ch, const char* file, size_t line);
		#endif

		void			Update(int iPulse);

		#ifdef ENABLE_SKILL_RENEWAL
		const			DWORD* GetUsableSkillList(BYTE bJob, BYTE bSkillGroup) const;
		#endif

		LPCHARACTER		SpawnMob(DWORD dwVnum, long lMapIndex, long x, long y, long z, bool bSpawnMotion = false, int iRot = -1, bool bShow = true);
		LPCHARACTER		SpawnMobRange(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, bool bIsException=false, bool bSpawnMotion = false , bool bAggressive = false);
		LPCHARACTER		SpawnGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen = NULL, bool bAggressive_ = false, LPDUNGEON pDungeon = NULL);
		LPCHARACTER		SpawnGroupGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen = NULL, bool bAggressive_ = false, LPDUNGEON pDungeon = NULL);
		bool			SpawnMoveGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, int tx, int ty, LPREGEN pkRegen = NULL, bool bAggressive_ = false);
		LPCHARACTER		SpawnMobRandomPosition(DWORD dwVnum, long lMapIndex, bool is_aggressive = false);

		void			SelectStone(LPCHARACTER pkChrStone);

		NAME_MAP &		GetPCMap() { return m_map_pkPCChr; }

		LPCHARACTER		Find(DWORD dwVID);
		LPCHARACTER		Find(const VID & vid);
		LPCHARACTER		FindPC(const char * name);
		LPCHARACTER		FindByPID(DWORD dwPID);

		bool			AddToStateList(LPCHARACTER ch);
		void			RemoveFromStateList(LPCHARACTER ch);

		void			DelayedSave(LPCHARACTER ch);
		bool			FlushDelayedSave(LPCHARACTER ch);
		void			ProcessDelayedSave();

		template<class Func>	Func for_each_pc(Func f);

		void			RegisterForMonsterLog(LPCHARACTER ch);
		void			UnregisterForMonsterLog(LPCHARACTER ch);
		void			PacketMonsterLog(LPCHARACTER ch, const void* buf, int size);

		void			KillLog(DWORD dwVnum);

		void			RegisterRaceNum(DWORD dwVnum);
		void			RegisterRaceNumMap(LPCHARACTER ch);
		void			UnregisterRaceNumMap(LPCHARACTER ch);
		bool			GetCharactersByRaceNum(DWORD dwRaceNum, CharacterVectorInteractor & i);

		LPCHARACTER		FindSpecifyPC(unsigned int uiJobFlag, long lMapIndex, LPCHARACTER except=NULL, int iMinLevel = 1, int iMaxLevel = PLAYER_MAX_LEVEL_CONST);

		void			SetMobItemRate(int value)	{ m_iMobItemRate = value; }
		void			SetMobDamageRate(int value)	{ m_iMobDamageRate = value; }
		void			SetMobGoldAmountRate(int value)	{ m_iMobGoldAmountRate = value; }
		void			SetMobGoldDropRate(int value)	{ m_iMobGoldDropRate = value; }
		void			SetMobExpRate(int value)	{ m_iMobExpRate = value; }

		void			SetMobItemRatePremium(int value)	{ m_iMobItemRatePremium = value; }
		void			SetMobGoldAmountRatePremium(int value)	{ m_iMobGoldAmountRatePremium = value; }
		void			SetMobGoldDropRatePremium(int value)	{ m_iMobGoldDropRatePremium = value; }
		void			SetMobExpRatePremium(int value)		{ m_iMobExpRatePremium = value; }

		void			SetUserDamageRatePremium(int value)	{ m_iUserDamageRatePremium = value; }
		void			SetUserDamageRate(int value ) { m_iUserDamageRate = value; }

		int				GetMobItemRate(LPCHARACTER ch);
		int				GetMobDamageRate(LPCHARACTER ch);
		int				GetMobGoldAmountRate(LPCHARACTER ch);
		int				GetMobGoldDropRate(LPCHARACTER ch);
		int				GetMobExpRate(LPCHARACTER ch);

		int				GetUserDamageRate(LPCHARACTER ch);
		void			SendScriptToMap(long lMapIndex, const std::string & s);

		bool			BeginPendingDestroy();
		void			FlushPendingDestroy();

		#ifdef ENABLE_RENEWAL_PLAYER_COUNT
		DWORD			TotalPlayerCount() { return m_map_pkChrByPID.size(); }
		#endif

	#ifdef __RANKING_SYSTEM__
	public:
		bool		GetRankData(BYTE categoryIndex, BYTE raceIndex, BYTE rankIndex, std::vector<TRanking>& list);
		void		SetRankData(const char* c_pData);

	protected:
		//CategoryId -- RaceId -- RankId -- Vector
		std::map<std::pair<BYTE, BYTE>, std::map<BYTE, std::vector<TRanking>>> m_map_ranks;
		// std::map<BYTE, std::vector<TRanking>> rank_data;
	#endif

	#ifdef ENABLE_MULTI_FARM_BLOCK
	public:
		int		GetMultiFarmCount(const char* playerIP, std::map<DWORD, std::pair<std::string, bool>>& m_mapNames);
		void	CheckMultiFarmAccount(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, BYTE affectType = 0, long affectDuration = 0, bool isP2P = false);
		void	SetMultiFarm(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, const BYTE affectType, const long affectTime);
		void	RemoveMultiFarm(const char* szIP, const DWORD playerID, const bool isP2P);
		void	CheckMultiFarmAccounts(const char* szIP);

	protected:
		std::map<std::string, std::vector<TMultiFarm>> m_mapmultiFarm;
	#endif

	#ifdef ENABLE_MULTI_HUNT_BLOCK
	public:
		int		GetMultiHuntCount(const char* playerIP, std::map<DWORD, std::pair<std::string, bool>>& m_mapNames);
		void	CheckMultiHuntAccount(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, BYTE affectType = 0, long affectDuration = 0, bool isP2P = false);
		void	SetMultiHunt(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, const BYTE affectType, const long affectTime);
		void	RemoveMultiHunt(const char* szIP, const DWORD playerID, const bool isP2P);
		void	CheckMultiHuntAccounts(const char* szIP);

	protected:
		std::map<std::string, std::vector<TMultiHunt>> m_mapmultiHunt;
	#endif

	#ifdef ENABLE_MULTI_FISH_BLOCK
	public:
		int		GetMultiFishCount(const char* playerIP, std::map<DWORD, std::pair<std::string, bool>>& m_mapNames);
		void	CheckMultiFishAccount(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, BYTE affectType = 0, long affectDuration = 0, bool isP2P = false);
		void	SetMultiFish(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, const BYTE affectType, const long affectTime);
		void	RemoveMultiFish(const char* szIP, const DWORD playerID, const bool isP2P);
		void	CheckMultiFishAccounts(const char* szIP);

	protected:
		std::map<std::string, std::vector<TMultiFish>> m_mapmultiFish;
	#endif

	private:
		int					m_iMobItemRate;
		int					m_iMobDamageRate;
		int					m_iMobGoldAmountRate;
		int					m_iMobGoldDropRate;
		int					m_iMobExpRate;

		int					m_iMobItemRatePremium;
		int					m_iMobGoldAmountRatePremium;
		int					m_iMobGoldDropRatePremium;
		int					m_iMobExpRatePremium;

		int					m_iUserDamageRate;
		int					m_iUserDamageRatePremium;
		int					m_iVIDCount;

		std::unordered_map<DWORD, LPCHARACTER> m_map_pkChrByVID;
		std::unordered_map<DWORD, LPCHARACTER> m_map_pkChrByPID;
		NAME_MAP			m_map_pkPCChr;

		CHARACTER_SET		m_set_pkChrState;
		CHARACTER_SET		m_set_pkChrForDelayedSave;
		CHARACTER_SET		m_set_pkChrMonsterLog;

		LPCHARACTER			m_pkChrSelectedStone;

		std::map<DWORD, DWORD> m_map_dwMobKillCount;

		std::set<DWORD>		m_set_dwRegisteredRaceNum;
		std::map<DWORD, CHARACTER_SET> m_map_pkChrByRaceNum;

		bool				m_bUsePendingDestroy;
		CHARACTER_SET		m_set_pkChrPendingDestroy;

		#ifdef M2_USE_POOL
		ObjectPool<CHARACTER> pool_;
		#endif

	#ifdef ENABLE_EVENT_MANAGER
	public:
		void			ClearEventData();
		bool			CloseEventManuel(BYTE eventIndex);
		void			SetEventData(BYTE dayIndex, const std::vector<TEventManagerData>& m_data);
		void			SetEventStatus(const WORD eventID, const bool eventStatus, const int endTime, const char* endTimeText);
		void			SendDataPlayer(LPCHARACTER ch);
		void			CheckBonusEvent(LPCHARACTER ch);
		void			UpdateAllPlayerEventData();
		void			CompareEventSendData(TEMP_BUFFER* buf);
		const TEventManagerData* CheckEventIsActive(BYTE eventIndex, BYTE empireIndex = 0);
		void			CheckEventForDrop(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM>& vec_item);

	protected:
		std::map<BYTE, std::vector<TEventManagerData>>	m_eventData;
	#endif

	#ifdef __BACK_DUNGEON__
	public:
		struct SBackDungeon
		{
			DWORD dwPID;
			long lX, lY;
			int iLeftTime;
			BYTE bChannel;
			SBackDungeon(const DWORD _dwPID, const long _lX, const long _lY, const int _iLeftTime, const BYTE _bChannel) : dwPID(_dwPID), lX(_lX), lY(_lY), iLeftTime(_iLeftTime), bChannel(_bChannel){}
		};

		int		GetBackDungeonTime(const DWORD dwMapIdx);
		void	SendBackDungeonInfo(LPCHARACTER ch);
		void	WarpBackDungeon(LPCHARACTER ch, const WORD wPort, const BYTE bChannel, const DWORD dwMapIdx);
		void	SetBackDungeon(const bool isP2P, const bool bIsSave, const WORD wPort, const BYTE bChannel, const DWORD dwMapIdx, const DWORD dwPID, const long lX = 0, const long lY = 0, int iLeftTime = 0);

	protected:
		std::map<WORD, std::map<DWORD, std::vector<SBackDungeon>>> m_mapBackDungeon;
	#endif

	#ifdef ENABLE_COSTUME_SET_BONUS
	public:
		struct SCostumeSetBonus
		{
			int iID;
			DWORD dwBody;
			DWORD dwHead;
			DWORD dwWeapon;
			DWORD dwPet;
			TPlayerItemAttribute aAttr[5];

			SCostumeSetBonus(
				const int _iID
				, const DWORD _dwBody
				, const DWORD _dwHead
				, const DWORD _dwWeapon
				, const DWORD _dwPet
				, const TPlayerItemAttribute* _aAttr
			) : 
				iID(_iID)
				, dwBody(_dwBody)
				, dwHead(_dwHead)
				, dwWeapon(_dwWeapon)
				, dwPet(_dwPet)
			{
				thecore_memcpy(aAttr, _aAttr, sizeof(aAttr));
			}
		};

		void	InitializeCostumeSetBonus();
		void	CheckCostumeSetBonus(LPCHARACTER ch);
		void	GiveCostumeSetBonus(LPCHARACTER ch, int iID, bool bAdd);

	protected:
		std::vector<SCostumeSetBonus> m_vecCostumeSetBonus;
	#endif

	#ifdef ENABLE_REWARD_SYSTEM
	public:
		void			LoadRewardData();
		void			SetRewardData(BYTE rewardIndex, const char* playerName, bool isP2P, DWORD value = 0, DWORD count = 0);
		void			SetRewardData2(BYTE rewardIndex, const char* playerName, bool isP2P, DWORD value = 0, DWORD count = 0);
		bool			IsRewardEmpty(BYTE rewardIndex, DWORD value);
		void			SendRewardInfo(LPCHARACTER ch);

		void			SetRewardStone(DWORD dwRaceNum, int iCount);
		int				GetRewardStone(DWORD dwRaceNum = 0);

		void			SetRewardBoss(DWORD dwRaceNum, int iCount);
		int				GetRewardBoss(DWORD dwRaceNum = 0);

	protected:
		std::vector<TRewardInfo> m_rewardData;
		std::vector<std::pair<DWORD, int>> m_rewardStoneList;
		std::vector<std::pair<DWORD, int>> m_rewardBossList;
		bool			m_rewardLoadStatus;
	#endif
};
template<class Func>
Func CHARACTER_MANAGER::for_each_pc(Func f)
{
	for (auto it = m_map_pkChrByPID.begin(); it != m_map_pkChrByPID.end(); ++it)
		f(it->second);

	return f;
}

class CharacterVectorInteractor : public CHARACTER_VECTOR
{
	public:
		CharacterVectorInteractor() : m_bMyBegin(false) { }

		CharacterVectorInteractor(const CHARACTER_SET & r);
		virtual ~CharacterVectorInteractor();

	private:
		bool m_bMyBegin;
};

#ifndef DEBUG_ALLOC
#define M2_DESTROY_CHARACTER(ptr) CHARACTER_MANAGER::instance().DestroyCharacter(ptr)
#else
#define M2_DESTROY_CHARACTER(ptr) CHARACTER_MANAGER::instance().DestroyCharacter(ptr, __FILE__, __LINE__)
#endif

#define M2_DESTROY_CHARACTER_EX(ptr) if (ptr) { M2_DESTROY_CHARACTER(ptr); ptr = nullptr; }
#endif

