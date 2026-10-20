#ifndef __GUILD_WAR_MAP_MANAGER_H
#define __GUILD_WAR_MAP_MANAGER_H

#include "constants.h"
#include "guild.h"

enum EWarMapTypes
{
	WAR_MAP_TYPE_NORMAL,
	WAR_MAP_TYPE_FLAG,
};

typedef struct SWarMapInfo
{
	BYTE		bType;
	long		lMapIndex;
	PIXEL_POSITION	posStart[3];
} TWarMapInfo;

namespace warmap
{
	enum
	{
		WAR_FLAG_VNUM_START = 20035,
		WAR_FLAG_VNUM_END = 20037,

		WAR_FLAG_VNUM0 = 20035,
		WAR_FLAG_VNUM1 = 20036,
		WAR_FLAG_VNUM2 = 20037,

		WAR_FLAG_BASE_VNUM = 20038
	};

	inline bool IsWarFlag(DWORD dwVnum)
	{
		if (dwVnum >= WAR_FLAG_VNUM_START && dwVnum <= WAR_FLAG_VNUM_END)
			return true;

		return false;
	}

	inline bool IsWarFlagBase(DWORD dwVnum)
	{
		return dwVnum == WAR_FLAG_BASE_VNUM ? true : false;
	}
};

class CWarMap
{
	public:
		friend class CGuild;

		CWarMap(long lMapIndex, const TGuildWarInfo & r_info, TWarMapInfo * pkWarMapInfo, DWORD dwGuildID1, DWORD dwGuildID2
			#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
			, int iScore
			#endif
		);
		~CWarMap();

		bool	GetTeamIndex(DWORD dwGuild, BYTE & bIdx);

		void	IncMember(LPCHARACTER ch);
		void	DecMember(LPCHARACTER ch);

		#ifdef CIT_SYSTEM
		bool	GetBeginProtectionStatus();
		void	SetBeginProtectionStatus(bool bNewStat);
		void	SetBeginProtectionEvent(LPEVENT pkEv);
		void	BeReady(BYTE iIdx);
		#endif

		CGuild * GetGuild(BYTE bIdx);
		DWORD	GetGuildID(BYTE bIdx);

		#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
		int		GetCurrentPlayer(BYTE bIdx);
		void	SendMemberCount(DWORD dwGID, int iMemCount);
		#endif

		BYTE	GetType();
		long	GetMapIndex();
		DWORD	GetGuildOpponent(LPCHARACTER ch);

		DWORD	GetWinnerGuild();
		void	UsePotion(LPCHARACTER ch, LPITEM item);

		void	Draw();
		void	Timeout();
		void	CheckWarEnd();
		bool	SetEnded();
		void	ExitAll();

		void	SetBeginEvent(LPEVENT pkEv);
		void	SetTimeoutEvent(LPEVENT pkEv);
		void	SetEndEvent(LPEVENT pkEv);
		void	SetResetFlagEvent(LPEVENT pkEv);

		void	UpdateScore(DWORD g1, int score1, DWORD g2, int score2);
		bool	CheckScore();

		int		GetRewardGold(BYTE bWinnerIdx);

		bool	GetGuildIndex(DWORD dwGuild, int& iIndex);

		void	Packet(const void * pv, int size);
		void	Notice(const char * psz);
		void	SendWarPacket(LPDESC d);
		void	SendScorePacket(BYTE bIdx, LPDESC d = NULL);

		void	OnKill(LPCHARACTER killer, LPCHARACTER ch);

		void	AddFlag(BYTE bIdx, DWORD x=0, DWORD y=0);
		void	AddFlagBase(BYTE bIdx, DWORD x=0, DWORD y=0);
		void	RemoveFlag(BYTE bIdx);
		bool	IsFlagOnBase(BYTE bIdx);
		void	ResetFlag();
		#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
		void	ClearCounterData();
		void	SaveCounterData();
		void	InsertWarStatistics();
		#endif

	private:
		void	UpdateUserCount();

	#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
	private:
		typedef struct SGuildPlayerTable
		{
			WORD	wRace;
			DWORD	dwOwnerGuildID;
			DWORD	dwTargetGuildID;
			char	szName[CHARACTER_NAME_MAX_LEN + 1];
			BYTE	bKill;
			BYTE	bDead;
			BYTE	bAssist;
			unsigned long long ullDamage;

			std::set<DWORD> assister;
			void	AddAssister(const DWORD id) { assister.emplace(id); }
			// void	AddAssister(const DWORD id) { if (assister.find(id) == assister.end()) { assister.emplace(id); } }
		} TGuildPlayerTable;

		typedef std::map<DWORD, std::unique_ptr<TGuildPlayerTable>> TGuildPlayerDataMap;
		// typedef std::map<DWORD, TGuildPlayerTable *> TGuildPlayerDataMap;

	public:
		void	NewPlayerStatistics(DWORD dwPID, WORD wRace, DWORD dwOwnerGuildID, DWORD dwTargetGuildID, const char * szName);
		void	LoadGuildWarStatistics(LPCHARACTER ch);
		TGuildPlayerTable * GetStatistics(DWORD dwPID);

	protected:
		TGuildPlayerDataMap	m_GuildPlayerTableDataMap;
	#endif

	private:
		TWarMapInfo	m_kMapInfo;
		bool		m_bEnded;

		LPEVENT		m_pkBeginEvent;
		LPEVENT		m_pkTimeoutEvent;
		LPEVENT		m_pkEndEvent;
		LPEVENT		m_pkResetFlagEvent;

		#ifdef CIT_SYSTEM
		bool		m_bBeginProtectionCompleted;
		LPEVENT		m_pkBeginProtectionEvent;
		#endif

		#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
		DWORD		m_dwWarID;
		#endif

		typedef struct STeamData
		{
			DWORD		dwID;
			CGuild *	pkGuild;
			int			iMemberCount;
			int			iUsePotionPrice;
			int			iScore;
			LPCHARACTER	pkChrFlag;
			LPCHARACTER	pkChrFlagBase;

			#ifdef CIT_SYSTEM
			bool bReady;
			#endif

			std::set<DWORD> set_pidJoiner;

			void Initialize();

			int GetAccumulatedJoinerCount();
			int GetCurJointerCount();

			void AppendMember(LPCHARACTER ch);
			void RemoveMember(LPCHARACTER ch);
			#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
			void SendMemberCount(DWORD dwGID, int iMemCount);
			#endif
		} TeamData;

		TeamData		m_TeamData[2];
		int				m_iObserverCount;
		DWORD			m_dwStartTime;
		BYTE			m_bTimeout;

		#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
		int				m_iEndScore;
		#endif

		TGuildWarInfo	m_WarInfo;
		CHARACTER_SET	m_set_pkChr;
};

class CWarMapManager : public singleton<CWarMapManager>
{
	public:
		CWarMapManager();
		virtual ~CWarMapManager();

		bool		LoadWarMapInfo(const char * c_pszFileName);
		bool		IsWarMap(long lMapIndex);
		TWarMapInfo *	GetWarMapInfo(long lMapIndex);
		bool		GetStartPosition(long lMapIndex, BYTE bIdx, PIXEL_POSITION & pos);

		template <typename Func> Func for_each(Func f);

		long		CreateWarMap(const TGuildWarInfo & r_WarInfo, DWORD dwGuildID1, DWORD dwGuildID2
								#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
								, int iScore
								#endif
		);

		void		DestroyWarMap(CWarMap* pMap);
		CWarMap *	Find(long lMapIndex);
		int			CountWarMap() { return m_mapWarMap.size(); }

		void		OnShutdown();

	private:
		std::map<long, TWarMapInfo *> m_map_kWarMapInfo;
		std::map<long, CWarMap *> m_mapWarMap;
};

template <typename Func> Func CWarMapManager::for_each(Func f)
{
	for (itertype(m_mapWarMap) it = m_mapWarMap.begin(); it != m_mapWarMap.end(); ++it)
		f(it->second);

	return f;
}
#endif
