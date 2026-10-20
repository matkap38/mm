#include "stdafx.h"
#include "war_map.h"
#include "sectree_manager.h"
#include "char.h"
#include "char_manager.h"
#include "affect.h"
#include "item.h"
#include "config.h"
#include "desc.h"
#include "desc_manager.h"
#include "guild_manager.h"
#include "buffer_manager.h"
#include "db.h"
#include "packet.h"
#include "locale_service.h"
#include "p2p.h"
#include "regen.h"

EVENTINFO(war_map_info)
{
	int iStep;
	CWarMap * pWarMap;

	war_map_info()
	: iStep( 0 )
	, pWarMap( 0 )
	{
	}
};

EVENTFUNC(war_begin_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "war_begin_event> <Factor> Null pointer" );
		return 0;
	}

	CWarMap * pMap = info->pWarMap;
	pMap->CheckWarEnd();
	return PASSES_PER_SEC(10);
}

#ifdef CIT_SYSTEM
EVENTFUNC(war_begin_protection_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>(event->info);
	if (info == NULL)
	{
		sys_err( "war_begin_protection_event> <Factor> Null pointer" );
		return 0;
	}

	CWarMap* pMap = info->pWarMap;

	if (--info->iStep > 0)
	{
		char szNoticeText[512];
		snprintf(szNoticeText, sizeof(szNoticeText), "Savas %u saniye icinde baslayacak.", info->iStep);
		pMap->Notice(szNoticeText);
		return PASSES_PER_SEC(1);
	}

	if (pMap->GetBeginProtectionStatus() == false)
	{
		pMap->SetBeginProtectionStatus(true);
		pMap->Notice("Savaþ Baþladý!");
		SECTREE_MANAGER::instance().PurgeNPCsInMap(pMap->GetMapIndex());
	}

	return 0;
}
#endif

EVENTFUNC(war_end_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "war_end_event> <Factor> Null pointer" );
		return 0;
	}

	CWarMap * pMap = info->pWarMap;

	if (info->iStep == 0)
	{
		++info->iStep;
		pMap->ExitAll();
		return PASSES_PER_SEC(5);
	}
	#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
	else if (info->iStep == 1)
	{
		++info->iStep;
		pMap->SaveCounterData();
		return PASSES_PER_SEC(5);
	}
	#endif
	else
	{
		pMap->SetEndEvent(NULL);
		CWarMapManager::instance().DestroyWarMap(pMap);
		return 0;
	}
}

EVENTFUNC(war_timeout_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "war_timeout_event> <Factor> Null pointer" );
		return 0;
	}

	CWarMap * pMap = info->pWarMap;
	pMap->Timeout();
	return 0;
}

void CWarMap::STeamData::Initialize()
{
	dwID = 0;
	pkGuild = NULL;
	iMemberCount = 0;
	iUsePotionPrice = 0;
	iScore = 0;
	pkChrFlag = NULL;
	pkChrFlagBase = NULL;

	#ifdef CIT_SYSTEM
	bReady = false;
	#endif

	set_pidJoiner.clear();
}

CWarMap::CWarMap(long lMapIndex, const TGuildWarInfo & r_info, TWarMapInfo * pkWarMapInfo, DWORD dwGuildID1, DWORD dwGuildID2
	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	, int iScore
	#endif
)
{
	m_kMapInfo = *pkWarMapInfo;
	m_kMapInfo.lMapIndex = lMapIndex;

	memcpy(&m_WarInfo, &r_info, sizeof(TGuildWarInfo));

	m_TeamData[0].Initialize();
	m_TeamData[0].dwID = dwGuildID1;
	m_TeamData[0].pkGuild = CGuildManager::instance().TouchGuild(dwGuildID1);

	m_TeamData[1].Initialize();
	m_TeamData[1].dwID = dwGuildID2;
	m_TeamData[1].pkGuild = CGuildManager::instance().TouchGuild(dwGuildID2);
	m_iObserverCount = 0;

	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	m_iEndScore = iScore;
	#endif

	war_map_info* info = AllocEventInfo<war_map_info>();
	info->pWarMap = this;
	SetBeginEvent(event_create(war_begin_event, info, PASSES_PER_SEC(60)));

	m_pkEndEvent = NULL;
	m_pkTimeoutEvent = NULL;
	m_pkResetFlagEvent = NULL;
	m_bTimeout = false;
	m_dwStartTime = get_dword_time();
	m_bEnded = false;

	if (GetType() == WAR_MAP_TYPE_FLAG)
	{
		AddFlagBase(0);
		AddFlagBase(1);
		AddFlag(0);
		AddFlag(1);
	}

	#ifdef CIT_SYSTEM
	m_bBeginProtectionCompleted = false;
	m_pkBeginProtectionEvent = NULL;

	auto pMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());
	if (pMap)
	{
		regen_load_in_file("locale/turkey/map/metin2_map_t3/cit.txt", GetMapIndex(), pMap->m_setting.iBaseX, pMap->m_setting.iBaseY);
	}
	#endif

	#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
	m_dwWarID = 0;
	InsertWarStatistics();
	#endif
}

CWarMap::~CWarMap()
{
	event_cancel(&m_pkBeginEvent);
	event_cancel(&m_pkEndEvent);
	event_cancel(&m_pkTimeoutEvent);
	event_cancel(&m_pkResetFlagEvent);
	#ifdef CIT_SYSTEM
	event_cancel(&m_pkBeginProtectionEvent);
	#endif

	sys_log(0, "WarMap::~WarMap : map index %d", GetMapIndex());

	itertype(m_set_pkChr) it = m_set_pkChr.begin();

	while (it != m_set_pkChr.end())
	{
		LPCHARACTER ch = *(it++);

		if (ch->GetDesc())
		{
			sys_log(0, "WarMap::~WarMap : disconnecting %s", ch->GetName());
			DESC_MANAGER::instance().DestroyDesc(ch->GetDesc());
		}
	}

	m_set_pkChr.clear();
}

void CWarMap::SetBeginEvent(LPEVENT pkEv)
{
	if (m_pkBeginEvent != NULL)
	{
		event_cancel(&m_pkBeginEvent);
	}

	if (pkEv != NULL)
	{
		m_pkBeginEvent = pkEv;
	}
}

#ifdef CIT_SYSTEM
void CWarMap::SetBeginProtectionEvent(LPEVENT pkEv)
{
	if (m_pkBeginProtectionEvent != NULL)
	{
		event_cancel(&m_pkBeginProtectionEvent);
	}

	if (pkEv != NULL)
	{
		m_pkBeginProtectionEvent = pkEv;
	}
}
#endif

void CWarMap::SetEndEvent(LPEVENT pkEv)
{
	if (m_pkEndEvent != NULL)
	{
		event_cancel(&m_pkEndEvent);
	}

	if (pkEv != NULL)
	{
		m_pkEndEvent = pkEv;
	}
}

void CWarMap::SetTimeoutEvent(LPEVENT pkEv)
{
	if (m_pkTimeoutEvent != NULL)
	{
		event_cancel(&m_pkTimeoutEvent);
	}

	if (pkEv != NULL)
	{
		m_pkTimeoutEvent = pkEv;
	}
}

void CWarMap::SetResetFlagEvent(LPEVENT pkEv)
{
	if (m_pkResetFlagEvent != NULL)
	{
		event_cancel(&m_pkResetFlagEvent);
	}

	if (pkEv != NULL) 
	{
		m_pkResetFlagEvent = pkEv;
	}
}

bool CWarMap::GetTeamIndex(DWORD dwGuildID, BYTE & bIdx)
{
	if (m_TeamData[0].dwID == dwGuildID)
	{
		bIdx = 0;
		return true;
	}
	else if (m_TeamData[1].dwID == dwGuildID)
	{
		bIdx = 1;
		return true;
	}

	return false;
}

DWORD CWarMap::GetGuildID(BYTE bIdx)
{
	assert(bIdx < 2);
	return m_TeamData[bIdx].dwID;
}

CGuild * CWarMap::GetGuild(BYTE bIdx)
{
	return m_TeamData[bIdx].pkGuild;
}

#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
int CWarMap::GetCurrentPlayer(BYTE bIdx)
{
	assert(bIdx < 2);
	return m_TeamData[bIdx].iMemberCount;
}

void CWarMap::SendMemberCount(DWORD dwGID, int iMemCount)
{
	CGuild * opp_g = CGuildManager::instance().FindGuild(dwGID);
	if (opp_g) { opp_g->SetWarMemberCount(iMemCount); }

	TPacketGGGuildWarMember pgg;
	pgg.bHeader = HEADER_GG_GUILD_WAR_MEMBER;
	pgg.dwGuild = dwGID;
	pgg.iMemCount = iMemCount;
	P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGGuildWarMember));
}

void CWarMap::STeamData::SendMemberCount(DWORD dwGID, int iMemCount)
{
	CGuild * opp_g = CGuildManager::instance().FindGuild(dwGID);
	if (opp_g) { opp_g->SetWarMemberCount(iMemCount); }

	TPacketGGGuildWarMember pgg;
	pgg.bHeader = HEADER_GG_GUILD_WAR_MEMBER;
	pgg.dwGuild = dwGID;
	pgg.iMemCount = iMemCount;
	P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGGuildWarMember));
}
#endif

long CWarMap::GetMapIndex()
{
	return m_kMapInfo.lMapIndex;
}

BYTE CWarMap::GetType()
{
	return m_kMapInfo.bType;
}

DWORD CWarMap::GetGuildOpponent(LPCHARACTER ch)
{
	if (ch->GetGuild())
	{
		DWORD gid = ch->GetGuild()->GetID();
		BYTE idx;

		if (GetTeamIndex(gid, idx))
			return m_TeamData[!idx].dwID;
	}
	return 0;
}

DWORD CWarMap::GetWinnerGuild()
{
	DWORD win_gid = 0;

	if (m_TeamData[1].iScore > m_TeamData[0].iScore)
	{
		win_gid = m_TeamData[1].dwID;
	}
	else if (m_TeamData[0].iScore > m_TeamData[1].iScore)
	{
		win_gid = m_TeamData[0].dwID;
	}

	return (win_gid);
}

void CWarMap::UsePotion(LPCHARACTER ch, LPITEM item)
{
	if (m_pkEndEvent)
		return;

	if (ch->IsObserverMode())
		return;

	if (!ch->GetGuild())
		return;

	if (!item->GetProto())
		return;

	int iPrice = item->GetProto()->dwGold;

	DWORD gid = ch->GetGuild()->GetID();

	if (gid == m_TeamData[0].dwID)
		m_TeamData[0].iUsePotionPrice += iPrice;
	else if (gid == m_TeamData[1].dwID)
		m_TeamData[1].iUsePotionPrice += iPrice;
}

int CWarMap::STeamData::GetAccumulatedJoinerCount()
{
	return set_pidJoiner.size();
}

int CWarMap::STeamData::GetCurJointerCount()
{
	return iMemberCount;
}

void CWarMap::STeamData::AppendMember(LPCHARACTER ch)
{
	set_pidJoiner.emplace(ch->GetPlayerID());
	++iMemberCount;
	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	SendMemberCount(dwID, iMemberCount);
	#endif
}

void CWarMap::STeamData::RemoveMember(LPCHARACTER ch)
{
	set_pidJoiner.erase(ch->GetPlayerID()); // @Lonca RemoveMember fix
	--iMemberCount;
	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	SendMemberCount(dwID, iMemberCount);
	#endif
}

struct FSendUserCount
{
	char buf1[30];
	char buf2[128];

	FSendUserCount(DWORD g1, int g1_count, DWORD g2, int g2_count, int observer)
	{
		snprintf(buf1, sizeof(buf1), "ObserverCount %d", observer);
		snprintf(buf2, sizeof(buf2), "WarUC %u %d %u %d %d", g1, g1_count, g2, g2_count, observer);
	}

	void operator() (LPCHARACTER ch)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, buf1);
		ch->ChatPacket(CHAT_TYPE_COMMAND, buf2);
	}
};

void CWarMap::UpdateUserCount()
{
	FSendUserCount f(
			m_TeamData[0].dwID,
			m_TeamData[0].GetAccumulatedJoinerCount(),
			m_TeamData[1].dwID,
			m_TeamData[1].GetAccumulatedJoinerCount(),
			m_iObserverCount);

	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
}

void CWarMap::IncMember(LPCHARACTER ch)
{
	if (!ch->IsPC())
		return;

	sys_log(0, "WarMap::IncMember");
	DWORD gid = 0;

	if (ch->GetGuild())
		gid = ch->GetGuild()->GetID();

	bool isWarMember = ch->GetQuestFlag("war.is_war_member") > 0 ? true : false;

	if (isWarMember && gid != m_TeamData[0].dwID && gid != m_TeamData[1].dwID)
	{
		ch->SetQuestFlag("war.is_war_member", 0);
		isWarMember = false;
	}

	if (isWarMember)
	{
		if (gid == m_TeamData[0].dwID)
		{
			m_TeamData[0].AppendMember(ch);
			#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
			NewPlayerStatistics(ch->GetPlayerID(), ch->GetRaceNum(), m_TeamData[0].dwID, m_TeamData[1].dwID, ch->GetName());
			#endif
		}
		else if (gid == m_TeamData[1].dwID)
		{
			m_TeamData[1].AppendMember(ch);
			#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
			NewPlayerStatistics(ch->GetPlayerID(), ch->GetRaceNum(), m_TeamData[1].dwID, m_TeamData[0].dwID, ch->GetName());
			#endif
		}

		event_cancel(&m_pkTimeoutEvent);

		sys_log(0, "WarMap +m %u(cur:%d, acc:%d) vs %u(cur:%d, acc:%d)",
				m_TeamData[0].dwID, m_TeamData[0].GetCurJointerCount(), m_TeamData[0].GetAccumulatedJoinerCount(),
				m_TeamData[1].dwID, m_TeamData[1].GetCurJointerCount(), m_TeamData[1].GetAccumulatedJoinerCount());
	}
	else
	{
		++m_iObserverCount;
		sys_log(0, "WarMap +o %d", m_iObserverCount);
		ch->SetObserverMode(true);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°üÀü ¸ðµå·Î ±æµåÀü¿¡ Âü°¡ÇÏ¼Ì½À´Ï´Ù."));
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÚ½ÅÀ» ¼±ÅÃÇÏ½Ã¸é ¹ÛÀ¸·Î ³ª°¥ ¼ö ÀÖ´Â <°ü¶÷ Á¾·á> ¹öÆ°ÀÌ ³ª¿É´Ï´Ù."));
	}

	UpdateUserCount();

	m_set_pkChr.emplace(ch);

	LPDESC d = ch->GetDesc();

	SendWarPacket(d);
	SendScorePacket(0, d);
	SendScorePacket(1, d);
}

void CWarMap::DecMember(LPCHARACTER ch)
{
	if (!ch->IsPC())
		return;

	sys_log(0, "WarMap::DecMember");
	DWORD gid = 0;

	if (ch->GetGuild())
		gid = ch->GetGuild()->GetID();

	if (!ch->IsObserverMode())
	{
		if (gid == m_TeamData[0].dwID)
			m_TeamData[0].RemoveMember(ch);
		else if (gid == m_TeamData[1].dwID)
			m_TeamData[1].RemoveMember(ch);

		if (m_kMapInfo.bType == WAR_MAP_TYPE_FLAG)
		{
			CAffect * pkAff = ch->FindAffect(AFFECT_WAR_FLAG);

			if (pkAff)
			{
				BYTE idx;

				if (GetTeamIndex(pkAff->lApplyValue, idx))
					AddFlag(idx, ch->GetX(), ch->GetY());

				ch->RemoveAffect(AFFECT_WAR_FLAG);
			}
		}

		sys_log(0, "WarMap -m %u(cur:%d, acc:%d) vs %u(cur:%d, acc:%d)",
				m_TeamData[0].dwID, m_TeamData[0].GetCurJointerCount(), m_TeamData[0].GetAccumulatedJoinerCount(),
				m_TeamData[1].dwID, m_TeamData[1].GetCurJointerCount(), m_TeamData[1].GetAccumulatedJoinerCount());

		CheckWarEnd();
		ch->SetQuestFlag("war.is_war_member", 0);
	}
	else
	{
		--m_iObserverCount;

		sys_log(0, "WarMap -o %d", m_iObserverCount);
		ch->SetObserverMode(false);
	}

	UpdateUserCount();

	m_set_pkChr.erase(ch);
}

struct FExitGuildWar
{
	void operator() (LPCHARACTER ch)
	{
		if (ch->IsPC())
		{
			ch->ExitToSavedLocation();
		}
	}
};

#ifdef CIT_SYSTEM
bool CWarMap::GetBeginProtectionStatus()
{
	return m_bBeginProtectionCompleted;
}

void CWarMap::SetBeginProtectionStatus(bool bNewStat)
{
	m_bBeginProtectionCompleted = bNewStat;
}
#endif

void CWarMap::ExitAll()
{
	FExitGuildWar f;
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
}

void CWarMap::CheckWarEnd()
{
	if (m_bEnded)
		return;

	if (m_TeamData[0].iMemberCount == 0 || m_TeamData[1].iMemberCount == 0)
	{
		if (m_bTimeout)
			return;

		if (m_pkTimeoutEvent)
			return;

		Notice(LC_TEXT("±æµåÀü¿¡ Âü°¡ÇÑ »ó´ë¹æ ±æµå¿øÀÌ ¾Æ¹«µµ ¾ø½À´Ï´Ù."));
		Notice(LC_TEXT("1ºÐ ÀÌ³»¿¡ ¾Æ¹«µµ Á¢¼ÓÇÏÁö ¾ÊÀ¸¸é ±æµåÀüÀÌ ÀÚµ¿ Á¾·áµË´Ï´Ù."));

		sys_log(0, "CheckWarEnd: Timeout begin %u vs %u", m_TeamData[0].dwID, m_TeamData[1].dwID);

		war_map_info* info = AllocEventInfo<war_map_info>();
		info->pWarMap = this;

		SetTimeoutEvent(event_create(war_timeout_event, info, PASSES_PER_SEC(60)));
	}
	else
		CheckScore();
}

int CWarMap::GetRewardGold(BYTE bWinnerIdx)
{
	int iRewardGold = m_WarInfo.iWarPrice;
	iRewardGold += (m_TeamData[bWinnerIdx].iUsePotionPrice * m_WarInfo.iWinnerPotionRewardPctToWinner) / 100;
	iRewardGold += (m_TeamData[bWinnerIdx ? 0 : 1].iUsePotionPrice * m_WarInfo.iLoserPotionRewardPctToWinner) / 100;
	return iRewardGold;
}

void CWarMap::Draw()
{
	CGuildManager::instance().RequestWarOver(m_TeamData[0].dwID, m_TeamData[1].dwID, 0, 0);
}

void CWarMap::Timeout()
{
	SetTimeoutEvent(NULL);

	if (m_bTimeout)
		return;

	if (m_bEnded)
		return;

	DWORD dwWinner = 0;
	DWORD dwLoser = 0;
	int iRewardGold = 0;

	if (get_dword_time() - m_dwStartTime < 60000 * 5)
	{
		Notice(LC_TEXT("±æµåÀüÀÌ ÀÏÂï Á¾·áµÇ¾î ¹«½ÂºÎ·Î ÆÇÁ¤ µÇ¾ú½À´Ï´Ù. (5ºÐÀÌ Áö³ªÁö ¾ÊÀ½)"));
		dwWinner = 0;
		dwLoser = 0;
	}
	else
	{
		int iWinnerIdx = -1;

		if (m_TeamData[0].iMemberCount == 0)
			iWinnerIdx = 1;
		else if (m_TeamData[1].iMemberCount == 0)
			iWinnerIdx = 0;

		if (iWinnerIdx == -1)
		{
			dwWinner = GetWinnerGuild();

			if (dwWinner == m_TeamData[0].dwID)
			{
				iRewardGold = GetRewardGold(0);
				dwLoser = m_TeamData[1].dwID;
			}
			else if (dwWinner == m_TeamData[1].dwID)
			{
				iRewardGold = GetRewardGold(1);
				dwLoser = m_TeamData[0].dwID;
			}

			sys_err("WarMap: member count is not zero, guild1 %u %d guild2 %u %d, winner %u",
					m_TeamData[0].dwID, m_TeamData[0].iMemberCount,
					m_TeamData[1].dwID, m_TeamData[1].iMemberCount,
					dwWinner);
		}
		else
		{
			dwWinner = m_TeamData[iWinnerIdx].dwID;
			dwLoser = m_TeamData[iWinnerIdx == 0 ? 1 : 0].dwID;

			iRewardGold = GetRewardGold(iWinnerIdx);
		}
	}

	sys_log(0, "WarMap: Timeout %u %u winner %u loser %u reward %d map %d",
			m_TeamData[0].dwID, m_TeamData[1].dwID, dwWinner, dwLoser, iRewardGold, m_kMapInfo.lMapIndex);

	if (dwWinner)
		CGuildManager::instance().RequestWarOver(dwWinner, dwLoser, dwWinner, iRewardGold);
	else
		CGuildManager::instance().RequestWarOver(m_TeamData[0].dwID, m_TeamData[1].dwID, dwWinner, iRewardGold);

	m_bTimeout = true;

	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	SendMemberCount(m_TeamData[0].dwID, 0);
	SendMemberCount(m_TeamData[1].dwID, 0);
	#endif
}

namespace
{
	struct FPacket
	{
		FPacket(const void * p, int size) : m_pvData(p), m_iSize(size)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			ch->GetDesc()->Packet(m_pvData, m_iSize);
		}

		const void * m_pvData;
		int m_iSize;
	};

	struct FNotice
	{
		FNotice(const char * psz) : m_psz(psz)
		{
		}

		void operator() (LPCHARACTER ch)
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_psz);
		}

		const char * m_psz;
	};
};

void CWarMap::Notice(const char * psz)
{
	FNotice f(psz);
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
}

void CWarMap::Packet(const void * p, int size)
{
	FPacket f(p, size);
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
}

void CWarMap::SendWarPacket(LPDESC d)
{
	TPacketGCGuild pack;
	TPacketGCGuildWar pack2;

	pack.header		= HEADER_GC_GUILD;
	pack.subheader	= GUILD_SUBHEADER_GC_WAR;
	pack.size		= sizeof(pack) + sizeof(pack2);

	pack2.dwGuildSelf	= m_TeamData[0].dwID;
	pack2.dwGuildOpp	= m_TeamData[1].dwID;
	pack2.bType			= CGuildManager::instance().TouchGuild(m_TeamData[0].dwID)->GetGuildWarType(m_TeamData[1].dwID);
	pack2.bWarState		= CGuildManager::instance().TouchGuild(m_TeamData[0].dwID)->GetGuildWarState(m_TeamData[1].dwID);
	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	pack2.iScore		= CGuildManager::instance().TouchGuild(m_TeamData[0].dwID)->GetWarScore();
	pack2.iMinLevel		= CGuildManager::instance().TouchGuild(m_TeamData[0].dwID)->GetWarMinLevel();
	pack2.iMaxPlayer	= CGuildManager::instance().TouchGuild(m_TeamData[0].dwID)->GetWarMaxPlayer();
	pack2.bWarrior		= CGuildManager::instance().TouchGuild(m_TeamData[0].dwID)->GetWarWarriorStatus();
	pack2.bAssassin		= CGuildManager::instance().TouchGuild(m_TeamData[0].dwID)->GetWarAssassinStatus();
	pack2.bSura			= CGuildManager::instance().TouchGuild(m_TeamData[0].dwID)->GetWarSuraStatus();
	pack2.bShaman		= CGuildManager::instance().TouchGuild(m_TeamData[0].dwID)->GetWarShamanStatus();
	#endif

	d->BufferedPacket(&pack, sizeof(pack));
	d->Packet(&pack2, sizeof(pack2));
}

#ifdef CIT_SYSTEM
void CWarMap::BeReady(BYTE iIdx)
{
	if (m_pkBeginProtectionEvent || GetBeginProtectionStatus()) { return; }
	if (m_TeamData[iIdx].bReady) { return; }

	const BYTE checkOther = (iIdx == 1) ? 0 : 1;
	m_TeamData[iIdx].bReady = true;

	if (m_TeamData[checkOther].bReady)
	{
		war_map_info* info = AllocEventInfo<war_map_info>();
		info->pWarMap = this;
		info->iStep = 30;
		SetBeginProtectionEvent(event_create(war_begin_protection_event, info, PASSES_PER_SEC(1)));

		char szText[128];
		snprintf(szText, sizeof(szText), "%s ve %s arasindaki savas 30 saniye sonra otomatik baþlayacaktýr.", m_TeamData[0].pkGuild->GetName(), m_TeamData[1].pkGuild->GetName());
		BroadcastNotice(szText);
		return;
	}

	char szNoticeText[128];
	snprintf(szNoticeText, sizeof(szNoticeText), "%s hazýr. Rakip lider bekleniyor.", m_TeamData[iIdx].pkGuild->GetName());
	Notice(szNoticeText);
}
#endif

void CWarMap::SendScorePacket(BYTE bIdx, LPDESC d)
{
	TPacketGCGuild p;

	p.header = HEADER_GC_GUILD;
	p.subheader = GUILD_SUBHEADER_GC_WAR_SCORE;
	p.size = sizeof(p) + sizeof(DWORD) + sizeof(DWORD) + sizeof(long);

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(p));
	buf.write(&m_TeamData[bIdx].dwID, sizeof(DWORD));
	buf.write(&m_TeamData[bIdx ? 0 : 1].dwID, sizeof(DWORD));
	buf.write(&m_TeamData[bIdx].iScore, sizeof(long));

	if (d)
		d->Packet(buf.read_peek(), buf.size());
	else
		Packet(buf.read_peek(), buf.size());
}

void CWarMap::UpdateScore(DWORD g1, int score1, DWORD g2, int score2)
{
	BYTE idx;

	if (GetTeamIndex(g1, idx))
	{
		if (m_TeamData[idx].iScore != score1)
		{
			m_TeamData[idx].iScore = score1;
			SendScorePacket(idx);
		}
	}

	if (GetTeamIndex(g2, idx))
	{
		if (m_TeamData[idx].iScore != score2)
		{
			m_TeamData[idx].iScore = score2;
			SendScorePacket(idx);
		}
	}

	CheckScore();
}

bool CWarMap::CheckScore()
{
	if (m_bEnded)
		return true;

	if (get_dword_time() - m_dwStartTime < 30000)
		return false;

	if (m_TeamData[0].iScore == m_TeamData[1].iScore)
		return false;

	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	int iEndScore = m_iEndScore;
	#else
	int iEndScore = m_WarInfo.iEndScore;
	#endif

	DWORD dwWinner;
	DWORD dwLoser;

	if (m_TeamData[0].iScore >= iEndScore)
	{
		dwWinner = m_TeamData[0].dwID;
		dwLoser = m_TeamData[1].dwID;
	}
	else if (m_TeamData[1].iScore >= iEndScore)
	{
		dwWinner = m_TeamData[1].dwID;
		dwLoser = m_TeamData[0].dwID;
	}
	else
		return false;

	int iRewardGold = 0;

	if (dwWinner == m_TeamData[0].dwID)
		iRewardGold = GetRewardGold(0);
	else if (dwWinner == m_TeamData[1].dwID)
		iRewardGold = GetRewardGold(1);

	sys_log(0, "WarMap::CheckScore end score %d guild1 %u score guild2 %d %u score %d winner %u reward %d",
			iEndScore,
			m_TeamData[0].dwID,
			m_TeamData[0].iScore,
			m_TeamData[1].dwID,
			m_TeamData[1].iScore,
			dwWinner,
			iRewardGold);

	CGuildManager::instance().RequestWarOver(dwWinner, dwLoser, dwWinner, iRewardGold);

	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	SendMemberCount(m_TeamData[0].dwID, 0);
	SendMemberCount(m_TeamData[1].dwID, 0);
	#endif

	return true;
}

bool CWarMap::SetEnded()
{
	sys_log(0, "WarMap::SetEnded %d", m_kMapInfo.lMapIndex);

	if (m_pkEndEvent)
		return false;

	if (m_TeamData[0].pkChrFlag)
	{
		M2_DESTROY_CHARACTER(m_TeamData[0].pkChrFlag);
		m_TeamData[0].pkChrFlag = NULL;
	}

	if (m_TeamData[0].pkChrFlagBase)
	{
		M2_DESTROY_CHARACTER(m_TeamData[0].pkChrFlagBase);
		m_TeamData[0].pkChrFlagBase = NULL;
	}

	if (m_TeamData[1].pkChrFlag)
	{
		M2_DESTROY_CHARACTER(m_TeamData[1].pkChrFlag);
		m_TeamData[1].pkChrFlag = NULL;
	}

	if (m_TeamData[1].pkChrFlagBase)
	{
		M2_DESTROY_CHARACTER(m_TeamData[1].pkChrFlagBase);
		m_TeamData[1].pkChrFlagBase = NULL;
	}

	event_cancel(&m_pkResetFlagEvent);
	m_bEnded = true;

	war_map_info* info = AllocEventInfo<war_map_info>();

	info->pWarMap = this;
	info->iStep = 0;
	SetEndEvent(event_create(war_end_event, info, PASSES_PER_SEC(10)));
	return true;
}

#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
void CWarMap::ClearCounterData()
{
	if (m_GuildPlayerTableDataMap.empty()) { return; }
	m_GuildPlayerTableDataMap.clear();
}

void CWarMap::SaveCounterData()
{
	for (const auto& it : m_GuildPlayerTableDataMap)
	{
		const auto& pGuildStaticsTable = it.second;
		if (pGuildStaticsTable)
		{
			CGuild * pGuildOne = CGuildManager::instance().FindGuild(pGuildStaticsTable->dwOwnerGuildID);
			CGuild * pGuildTwo = CGuildManager::instance().FindGuild(pGuildStaticsTable->dwTargetGuildID);

			if (pGuildOne && pGuildTwo)
			{
				DBManager::instance().DirectQuery(
					"INSERT INTO player.lonca_istatistik(war_id, loncam, rakip, isim, olum, oldurme, damage) "
					"VALUES('%u', '%s', '%s', '%s', '%d', '%d', '%lld')"
					, m_dwWarID
					, pGuildOne->GetName()
					, pGuildTwo->GetName()
					, pGuildStaticsTable->szName
					, pGuildStaticsTable->bDead
					, pGuildStaticsTable->bKill
					, pGuildStaticsTable->ullDamage
				);
			}
		}
	}

	if (m_dwWarID > 0)
	{
		DBManager::instance().DirectQuery("UPDATE player.lonca_savas SET durum = 0 WHERE id = '%u';", m_dwWarID);
	}
	m_dwWarID = 0;
}
#endif

void CWarMap::OnKill(LPCHARACTER killer, LPCHARACTER ch)
{
	if (m_bEnded)
		return;

	DWORD dwKillerGuild = 0;
	DWORD dwDeadGuild = 0;

	if (killer->GetGuild())
		dwKillerGuild = killer->GetGuild()->GetID();

	if (ch->GetGuild())
		dwDeadGuild = ch->GetGuild()->GetID();

	BYTE idx;

	sys_log(0, "WarMap::OnKill %u %u", dwKillerGuild, dwDeadGuild);

	if (!GetTeamIndex(dwKillerGuild, idx))
		return;

	if (!GetTeamIndex(dwDeadGuild, idx))
		return;

	#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
	if (ch->GetGuild() && killer->GetGuild())
	{
		const auto& pGuildPlayerStatistics = GetStatistics(ch->GetPlayerID());
		if (pGuildPlayerStatistics)
			pGuildPlayerStatistics->bDead += 1;

		const auto& pGuildKillerStatistics = GetStatistics(killer->GetPlayerID());
		if (pGuildKillerStatistics)
			pGuildKillerStatistics->bKill += 1;

		if (pGuildPlayerStatistics)
		{
			if (pGuildPlayerStatistics->assister.size() > 0)
			{
				for (const auto& i : pGuildPlayerStatistics->assister)
				{
					if (i == killer->GetPlayerID()) { continue; } // oldurene asist yazma
					const auto& assister = GetStatistics(i);
					if (assister) { assister->bAssist += 1; }
				}
				pGuildPlayerStatistics->assister.clear();
			}
		}
	}
	#endif

	switch (m_kMapInfo.bType)
	{
		case WAR_MAP_TYPE_NORMAL:
			SendGuildWarScore(dwKillerGuild, dwDeadGuild, 1, ch->GetLevel());
			break;

		case WAR_MAP_TYPE_FLAG:
			{
				CAffect * pkAff = ch->FindAffect(AFFECT_WAR_FLAG);

				if (pkAff)
				{
					if (GetTeamIndex(pkAff->lApplyValue, idx))
						AddFlag(idx, ch->GetX(), ch->GetY());

					ch->RemoveAffect(AFFECT_WAR_FLAG);
				}
			}
			break;

		default:
			sys_err("unknown war map type %u index %d", m_kMapInfo.bType, m_kMapInfo.lMapIndex);
			break;
	}
}

#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
CWarMap::TGuildPlayerTable* CWarMap::GetStatistics(DWORD dwPID)
{
	const auto it = m_GuildPlayerTableDataMap.find(dwPID);
	if (it == m_GuildPlayerTableDataMap.end()) { return nullptr; }
	return it->second.get();
}

void CWarMap::NewPlayerStatistics(DWORD dwPID, WORD wRace, DWORD dwOwnerGuildID, DWORD dwTargetGuildID, const char * szName)
{
	const auto& pStatistics = GetStatistics(dwPID);
	if (pStatistics)
	{
		sys_err("Already Inserted Player : ID: %d", dwPID);
		return;
	}

	TGuildPlayerTable pGuildPlayerData;
	pGuildPlayerData.wRace = wRace;
	pGuildPlayerData.dwOwnerGuildID = dwOwnerGuildID;
	pGuildPlayerData.dwTargetGuildID = dwTargetGuildID;
	strlcpy(pGuildPlayerData.szName, szName, sizeof(pGuildPlayerData.szName));
	pGuildPlayerData.bKill = 0;
	pGuildPlayerData.bDead = 0;
	pGuildPlayerData.bAssist = 0;
	pGuildPlayerData.ullDamage = 0;
	std::unique_ptr<TGuildPlayerTable> pGuildPlayer = std::make_unique<TGuildPlayerTable>(pGuildPlayerData);
	m_GuildPlayerTableDataMap.emplace(dwPID, std::move(pGuildPlayer));
}

void CWarMap::LoadGuildWarStatistics(LPCHARACTER ch)
{
	if (!ch || !ch->GetGuild() || !ch->GetDesc()) { return; }

	const DWORD dwGuildID1 = ch->GetGuild()->GetID();

	std::vector<TGuildPlayerTable*> mVec; mVec.clear();
	for (const auto& it : m_GuildPlayerTableDataMap)
	{
		const auto& pGuildStaticsTable = it.second.get();
		if (pGuildStaticsTable)
		{
			const DWORD dwGuildID2 = pGuildStaticsTable->dwOwnerGuildID;
			if (dwGuildID1 == dwGuildID2)
			{
				mVec.push_back(it.second.get());
			}
		}
	}

	if (mVec.empty()) { return; }

	// std::sort(mVec.begin(), mVec.end(), [](TGuildPlayerTable* p1, TGuildPlayerTable* p2) {return (p1->bKill > p2->bKill) && (p1->ullDamage > p2->ullDamage); });
	std::sort(mVec.begin(), mVec.end(), [](TGuildPlayerTable* p1, TGuildPlayerTable* p2) {return (p1->ullDamage > p2->ullDamage); });

	BYTE bRank = 0;
	for (const auto& iter : mVec)
	{
		const auto& pGuildStaticsTable = iter;
		if (pGuildStaticsTable)
		{
			TPacketGCGuildStaticsInfo table{};
			strlcpy(table.szName, pGuildStaticsTable->szName, sizeof(table.szName));
			table.ullDamage = pGuildStaticsTable->ullDamage;
			table.bRank = bRank;
			table.wRace = pGuildStaticsTable->wRace;
			table.bKill = pGuildStaticsTable->bKill;
			table.bDead = pGuildStaticsTable->bDead;
			table.bAssist = pGuildStaticsTable->bAssist;
			ch->GetDesc()->Packet(&table, sizeof(TPacketGCGuildStaticsInfo));
			bRank++;

			if (mVec.size() == bRank)
			{
				TPacketGCGuildStaticsInfo table{};
				strlcpy(table.szName, pGuildStaticsTable->szName, sizeof(table.szName));
				table.ullDamage = pGuildStaticsTable->ullDamage;
				table.bRank = BYTE(250);
				table.wRace = pGuildStaticsTable->wRace;
				table.bKill = pGuildStaticsTable->bKill;
				table.bDead = pGuildStaticsTable->bDead;
				table.bAssist = pGuildStaticsTable->bAssist;
				ch->GetDesc()->Packet(&table, sizeof(TPacketGCGuildStaticsInfo));
			}
		}
	}

	mVec.clear();
}
#endif

void CWarMap::AddFlagBase(BYTE bIdx, DWORD x, DWORD y)
{
	if (m_bEnded)
		return;

	assert(bIdx < 2);

	TeamData & r = m_TeamData[bIdx];

	if (r.pkChrFlagBase)
		return;

	if (x == 0)
	{
		x = m_kMapInfo.posStart[bIdx].x;
		y = m_kMapInfo.posStart[bIdx].y;
	}

	r.pkChrFlagBase = CHARACTER_MANAGER::instance().SpawnMob(warmap::WAR_FLAG_BASE_VNUM, m_kMapInfo.lMapIndex, x, y, 0);
	sys_log(0, "WarMap::AddFlagBase %u %p id %u", bIdx, get_pointer(r.pkChrFlagBase), r.dwID);

	r.pkChrFlagBase->SetPoint(POINT_STAT, r.dwID);
	r.pkChrFlagBase->SetWarMap(this);
}

void CWarMap::AddFlag(BYTE bIdx, DWORD x, DWORD y)
{
	if (m_bEnded)
		return;

	assert(bIdx < 2);

	TeamData & r = m_TeamData[bIdx];

	if (r.pkChrFlag)
		return;

	if (x == 0)
	{
		x = m_kMapInfo.posStart[bIdx].x;
		y = m_kMapInfo.posStart[bIdx].y;
	}

	r.pkChrFlag = CHARACTER_MANAGER::instance().SpawnMob(bIdx == 0 ? warmap::WAR_FLAG_VNUM0 : warmap::WAR_FLAG_VNUM1, m_kMapInfo.lMapIndex, x, y, 0);
	sys_log(0, "WarMap::AddFlag %u %p id %u", bIdx, get_pointer(r.pkChrFlag), r.dwID);

	r.pkChrFlag->SetPoint(POINT_STAT, r.dwID);
	r.pkChrFlag->SetWarMap(this);
}

void CWarMap::RemoveFlag(BYTE bIdx)
{
	assert(bIdx < 2);

	TeamData & r = m_TeamData[bIdx];

	if (!r.pkChrFlag)
		return;

	sys_log(0, "WarMap::RemoveFlag %u %p", bIdx, get_pointer(r.pkChrFlag));

	r.pkChrFlag->Dead(NULL, true);
	r.pkChrFlag = NULL;
}

bool CWarMap::IsFlagOnBase(BYTE bIdx)
{
	assert(bIdx < 2);

	TeamData & r = m_TeamData[bIdx];

	if (!r.pkChrFlag)
		return false;

	const PIXEL_POSITION & pos = r.pkChrFlag->GetXYZ();

	if (pos.x == m_kMapInfo.posStart[bIdx].x && pos.y == m_kMapInfo.posStart[bIdx].y)
		return true;

	return false;
}

EVENTFUNC(war_reset_flag_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>(event->info);
	if (info == NULL)
	{
		sys_err("war_reset_flag_event> <Factor> Null pointer");
		return 0;
	}

	CWarMap * pMap = info->pWarMap;

	pMap->AddFlag(0);
	pMap->AddFlag(1);

	pMap->SetResetFlagEvent(NULL);
	return 0;
}

struct FRemoveFlagAffect
{
	void operator() (LPCHARACTER ch)
	{
		if (ch->FindAffect(AFFECT_WAR_FLAG))
			ch->RemoveAffect(AFFECT_WAR_FLAG);
	}
};

void CWarMap::ResetFlag()
{
	if (m_kMapInfo.bType != WAR_MAP_TYPE_FLAG)
		return;

	if (m_pkResetFlagEvent)
		return;

	if (m_bEnded)
		return;

	FRemoveFlagAffect f;
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);

	RemoveFlag(0);
	RemoveFlag(1);

	war_map_info* info = AllocEventInfo<war_map_info>();
	info->pWarMap = this;
	info->iStep = 0;
	SetResetFlagEvent(event_create(war_reset_flag_event, info, PASSES_PER_SEC(10)));
}

#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
void CWarMap::InsertWarStatistics()
{
	CGuild * pGuildOne = CGuildManager::instance().FindGuild(m_TeamData[0].dwID);
	CGuild * pGuildTwo = CGuildManager::instance().FindGuild(m_TeamData[1].dwID);

	if (pGuildOne && pGuildTwo)
	{
		auto pkMsg(DBManager::instance().DirectQuery("INSERT INTO player.lonca_savas(loncam, rakip, tarih, durum) VALUES('%s', '%s', NOW(), 1);", pGuildOne->GetName(), pGuildTwo->GetName()));
		if (!pkMsg)
		{
			sys_err("InsertWarStatistics::szQuery:!pkMsg");
			return;
		}

		DWORD dwIstID = pkMsg->Get()->uiInsertID;
		if (dwIstID > 0)
		{
			m_dwWarID = dwIstID;
		}
	}
}
#endif

/////////////////////////////////////////////////////////////////////////////////
// WarMapManager
/////////////////////////////////////////////////////////////////////////////////
CWarMapManager::CWarMapManager()
{
}

CWarMapManager::~CWarMapManager()
{
	for( std::map<long, TWarMapInfo *>::const_iterator iter = m_map_kWarMapInfo.begin() ; iter != m_map_kWarMapInfo.end() ; ++iter )
	{
		M2_DELETE(iter->second);
	}

	m_map_kWarMapInfo.clear();
}

bool CWarMapManager::LoadWarMapInfo(const char * c_pszFileName)
{
	TWarMapInfo * k;

	k = M2_NEW TWarMapInfo;
	k->bType = WAR_MAP_TYPE_NORMAL;

	k->lMapIndex = 110;
	k->posStart[0].x = 48 * 100 + 32000;
	k->posStart[0].y = 52 * 100 + 0;
	k->posStart[1].x = 183 * 100 + 32000;
	k->posStart[1].y = 206 * 100 + 0;
	k->posStart[2].x = 141 * 100 + 32000;
	k->posStart[2].y = 117 * 100 + 0;

	m_map_kWarMapInfo.emplace(std::make_pair(k->lMapIndex, k));

	k = M2_NEW TWarMapInfo;
	k->bType = WAR_MAP_TYPE_FLAG;

	k->lMapIndex = 111;
	k->posStart[0].x = 68 * 100 + 57600;
	k->posStart[0].y = 69 * 100 + 0;
	k->posStart[1].x = 171 * 100 + 57600;
	k->posStart[1].y = 182 * 100 + 0;
	k->posStart[2].x = 122 * 100 + 57600;
	k->posStart[2].y = 131 * 100 + 0;

	m_map_kWarMapInfo.emplace(std::make_pair(k->lMapIndex, k));
	return true;
}

bool CWarMapManager::IsWarMap(long lMapIndex)
{
	return GetWarMapInfo(lMapIndex) ? true : false;
}

bool CWarMapManager::GetStartPosition(long lMapIndex, BYTE bIdx, PIXEL_POSITION & pos)
{
	assert(bIdx < 3);

	TWarMapInfo * pi = GetWarMapInfo(lMapIndex);
	if (!pi)
	{
		sys_log(0, "GetStartPosition FAILED [%d] WarMapInfoSize(%d)", lMapIndex, m_map_kWarMapInfo.size());

		itertype(m_map_kWarMapInfo) it;
		for (it	= m_map_kWarMapInfo.begin(); it != m_map_kWarMapInfo.end(); ++it)
		{
			PIXEL_POSITION& cur=it->second->posStart[bIdx];
			sys_log(0, "WarMap[%d]=Pos(%d, %d)", it->first, cur.x, cur.y);
		}
		return false;
	}

	pos = pi->posStart[bIdx];
	return true;
}

long CWarMapManager::CreateWarMap(const TGuildWarInfo& guildWarInfo, DWORD dwGuildID1, DWORD dwGuildID2
	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	, int iScore
	#endif
)
{
	TWarMapInfo * pkInfo = GetWarMapInfo(guildWarInfo.lMapIndex);
	if (!pkInfo)
	{
		sys_err("GuildWar.CreateWarMap.NOT_FOUND_MAPINFO[%d]", guildWarInfo.lMapIndex);
		return 0;
	}

	DWORD lMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(guildWarInfo.lMapIndex);
	if (lMapIndex)
	{
		m_mapWarMap.emplace(std::make_pair(lMapIndex, M2_NEW CWarMap(lMapIndex, guildWarInfo, pkInfo, dwGuildID1, dwGuildID2
			#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
			, iScore
			#endif
		)));
	}

	return lMapIndex;
}

TWarMapInfo * CWarMapManager::GetWarMapInfo(long lMapIndex)
{
	if (lMapIndex >= 10000)
		lMapIndex /= 10000;

	itertype(m_map_kWarMapInfo) it = m_map_kWarMapInfo.find(lMapIndex);

	if (m_map_kWarMapInfo.end() == it)
		return NULL;

	return it->second;
}

void CWarMapManager::DestroyWarMap(CWarMap* pMap)
{
	long mapIdx = pMap->GetMapIndex();

	sys_log(0, "WarMap::DestroyWarMap : %d", mapIdx);

	#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
	pMap->ClearCounterData();
	#endif

	m_mapWarMap.erase(pMap->GetMapIndex());
	M2_DELETE(pMap);

	SECTREE_MANAGER::instance().DestroyPrivateMap(mapIdx);
}

CWarMap * CWarMapManager::Find(long lMapIndex)
{
	itertype(m_mapWarMap) it = m_mapWarMap.find(lMapIndex);

	if (it == m_mapWarMap.end())
		return NULL;

	return it->second;
}

void CWarMapManager::OnShutdown()
{
	itertype(m_mapWarMap) it = m_mapWarMap.begin();

	while (it != m_mapWarMap.end())
		(it++)->second->Draw();
}

