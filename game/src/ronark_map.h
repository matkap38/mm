#pragma once

#ifdef ENABLE_RONARK_SYSTEM
class CRonarkMapManager : public singleton<CRonarkMapManager>
{
public:
	typedef struct STeamData
	{
		DWORD	dwID;
		CGuild*	pkGuild;
		int		iMemberCount;
		DWORD	iDamage;

		std::set<DWORD> memberlist;

		void	Initialize();

		int		GetAccumulatedJoinerCount();
		int		GetCurJointerCount();

		void	AppendMember(LPCHARACTER ch);
		void	RemoveMember(LPCHARACTER ch);
		void	AppendDamage(int64_t damage);
	} TeamData;

	enum ERonarkStatus
	{
		RONARK_STATUS_CLOSED = 0,
		RONARK_STATUS_OPENED = 1,
		RONARK_STATUS_REWARD = 2,
		RONARK_REFRESH_TIME = 5,
	};

	typedef std::map<DWORD, TeamData*> TeamDataMap;
	typedef std::vector<TRonarkTeamData> TeamDataVec;

public:
	CRonarkMapManager(void);
	virtual ~CRonarkMapManager(void);

	TeamData*		GetTable(DWORD dwID);

	DWORD			GetGuildDamage(DWORD dwID);
	DWORD			GetGuildMemberCount(DWORD dwID);

	void			IncMember(LPCHARACTER ch);
	void			DecMember(LPCHARACTER ch);

	void			AppendGuild(CGuild* guild);
	void			OnDamage(CGuild* guild, int64_t damage);

	void			Update();
	void			SendScorePacket(LPCHARACTER pkChar);

	void			SetRonarkStatus(int iStatus, int iMap);
	int				GetRonarkStatus();
	bool			IsRonarkMap(int mapindex);
	bool			IsRonarkActivate();

	void			SetMemberLimitCount(int memberlimit);
	int				GetTotalMemberCount() { return m_set_pkChr.size(); }
	DWORD			GetWinnerGuild();

	void			SetBossVID(DWORD dwVID) { m_ibossVID = dwVID; }
	DWORD			GetBossVID() { return m_ibossVID; }

	void			Packet(const void* pv, int size);
	void			Notice(const char* psz);
	void			SendHome();

	void			DelEvent();

	void			Reward();
	void			ResetEvent();

	void			Destroy();

protected:
	TeamDataMap		m_TeamDataMap;
	TeamDataVec		m_TeamDataVec;
	int64_t			ronark_hp;
	int				m_iRonarkMapIndex;
	int				m_iMemberLimitCount;
	int				m_iActivateStatus;
	DWORD			m_ibossVID;
	LPEVENT			destroyEvent;
	LPEVENT			updateEvent;

private:
	CHARACTER_SET	m_set_pkChr;
};
#endif
