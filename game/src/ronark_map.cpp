#include "stdafx.h"

#ifdef ENABLE_RONARK_SYSTEM
#include "char.h"
#include "utils.h"
#include "log.h"
#include "db.h"
#include "config.h"
#include "desc.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "desc_client.h"
#include "guild.h"
#include "guild_manager.h"
#include "char_manager.h"
#include "questmanager.h"
#include "start_position.h"
#include "sectree_manager.h"
#include "ronark_map.h"
#include <fmt/fmt.h>
#include "cmd.h"
#include "LoadTxtToTable.h"

//--------------------------------------------------------------------------------------------//
EVENTINFO(ronark_info) { ronark_info(){} };
EVENTFUNC(ronark_destroy_event)
{
	ronark_info* info = dynamic_cast<ronark_info*>(event->info);
	if (info == NULL)
	{
		sys_err("ronark_destroy_event> <Factor> NULL pointer");
		return 0;
	}

	CRonarkMapManager::instance().DelEvent();
	CRonarkMapManager::instance().SendHome();
	CRonarkMapManager::instance().ResetEvent();
	return 0;
}

EVENTINFO(Ronark_event_data) { Ronark_event_data(){} };
EVENTFUNC(Ronark_event_data_start)
{
	Ronark_event_data* pInfo = dynamic_cast<Ronark_event_data*>(event->info);
	if (pInfo == NULL)
	{
		sys_err("Ronark_event_data> <Factor> Null pointer");
		return 0;
	}

	if (!CRonarkMapManager::instance().IsRonarkActivate()) { return 0; }
	CRonarkMapManager::instance().Update();
	return PASSES_PER_SEC(CRonarkMapManager::ERonarkStatus::RONARK_REFRESH_TIME);
}
//--------------------------------------------------------------------------------------------//

bool SortGuilds(CRonarkMapManager::TeamData i, CRonarkMapManager::TeamData j)
{
	return (i.iDamage > j.iDamage);
}

bool SortScore(TRonarkTeamData i, TRonarkTeamData j)
{
	return (i.dwDamage > j.dwDamage);
}

int CRonarkMapManager::STeamData::GetAccumulatedJoinerCount()
{
	return memberlist.size();
}

int CRonarkMapManager::STeamData::GetCurJointerCount()
{
	return iMemberCount;
}

void CRonarkMapManager::STeamData::AppendMember(LPCHARACTER ch)
{
	memberlist.emplace(ch->GetPlayerID());
	++iMemberCount;
}

void CRonarkMapManager::STeamData::RemoveMember(LPCHARACTER ch)
{
	memberlist.erase(ch->GetPlayerID());
	--iMemberCount;
}

void CRonarkMapManager::STeamData::AppendDamage(int64_t damage)
{
	iDamage += damage;
}

void CRonarkMapManager::STeamData::Initialize()
{
	dwID = 0;
	pkGuild = NULL;
	iMemberCount = 0;
	iDamage = 0;
	memberlist.clear();
}

CRonarkMapManager::TeamData* CRonarkMapManager::GetTable(DWORD dwID)
{
	TeamDataMap::iterator itor = m_TeamDataMap.find(dwID);
	if (itor == m_TeamDataMap.end())
	{
		return NULL;
	}
	return itor->second;
}

DWORD CRonarkMapManager::GetGuildDamage(DWORD dwID)
{
	CRonarkMapManager::TeamData* c_pTable = GetTable(dwID);
	if (!c_pTable)
	{
		return 0;
	}
	return c_pTable->iDamage;
}

DWORD CRonarkMapManager::GetGuildMemberCount(DWORD dwID)
{
	CRonarkMapManager::TeamData* c_pTable = GetTable(dwID);
	if (!c_pTable)
	{
		return 0;
	}
	return c_pTable->iMemberCount;
}

void CRonarkMapManager::IncMember(LPCHARACTER ch)
{
	if (!ch) { return; }
	if (!ch->IsPC()) { return; }
	if (!IsRonarkActivate()) { return; }

	if (!ch->IsObserverMode())
	{
		DWORD gid = 0;

		if (ch->GetGuild())
			gid = ch->GetGuild()->GetID();

		bool isWarMember = gid != 0;

		if (ch->GetGMLevel() >= GM_HIGH_WIZARD)
		{
			ch->ChatPacket(CHAT_TYPE_INFO,"Ronark Debug ->> Mapdeki Oyuncu Sayýsý %d, Oyuncu Sýnýrý %d", GetTotalMemberCount(), m_iMemberLimitCount);
		}

		if (ch->GetQuestFlag("ronark.izleyici") == 1)
		{
			ch->ChatPacket(CHAT_TYPE_INFO,"Ronark Ýzleyicisiniz.");
			isWarMember = false;
		}
		else
		{
			if (gid != 0)
			{
				for (TeamDataMap::iterator itor = m_TeamDataMap.begin(); itor != m_TeamDataMap.end(); ++itor)
				{
					TeamData* team = itor->second;
					if (team->dwID == gid)
					{
						if ((int)team->memberlist.size() >= m_iMemberLimitCount)
						{
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("RONARKSINIR_%d"), m_iMemberLimitCount);
							isWarMember = false;
							break;
						}
					}
				}
			}
		}

		if (isWarMember)
		{
			ch->SetPKMode(PK_MODE_GUILD);

			TeamData* pCheck = GetTable(gid);
			if (!pCheck)
			{
				AppendGuild(ch->GetGuild());
			}

			TeamData* pGuild = GetTable(gid);

			if (pGuild)
			{
				pGuild->AppendMember(ch);
				sys_log(0, "CRonarkMapManager:: Ronark Append Member: %s %d", ch->GetName(), gid);
			}
		}
		else
		{
			ch->SetObserverMode(true);
		}
	}

	m_set_pkChr.emplace(ch);
}

void CRonarkMapManager::DecMember(LPCHARACTER ch)
{
	if (!ch) { return; }
	if (!ch->IsPC()) { return; }

	if (ch->IsObserverMode())
	{
		ch->SetObserverMode(false);
		sys_log(0, "CRonarkMapManager:: Ronark Remove ObserverMode");
	}
	else
	{
		DWORD gid = 0;

		if (ch->GetGuild())
			gid = ch->GetGuild()->GetID();

		TeamData* pGuild = GetTable(gid);
		if (pGuild)
		{
			pGuild->RemoveMember(ch);
			sys_log(0, "CRonarkMapManager:: Ronark Remove Member: %s %d", ch->GetName(), gid);
		}
	}

	m_set_pkChr.erase(ch);
}

void CRonarkMapManager::AppendGuild(CGuild* guild)
{
	TeamData* pGuildData = new TeamData;
	pGuildData->dwID = guild->GetID();
	pGuildData->pkGuild = guild;
	pGuildData->iMemberCount = 0;
	pGuildData->iDamage = 0;
	m_TeamDataMap.emplace(guild->GetID(), pGuildData);
}

void CRonarkMapManager::OnDamage(CGuild* guild, int64_t damage)
{
	TeamData* pGuild = GetTable(guild->GetID());
	if (pGuild)
	{
		pGuild->AppendDamage(damage);
	}
}

DWORD CRonarkMapManager::GetWinnerGuild()
{
	std::vector<CRonarkMapManager::TeamData> sendLists;
	for (TeamDataMap::iterator itor = m_TeamDataMap.begin(); itor != m_TeamDataMap.end(); ++itor)
	{
		sendLists.emplace_back(*itor->second);
	}

	DWORD dwID = 0;

	if (!sendLists.empty())
	{
		std::stable_sort(sendLists.begin(), sendLists.end(), SortGuilds);
		dwID = sendLists[0].dwID;
		sendLists.clear();
	}

	return dwID;
}

void CRonarkMapManager::SetRonarkStatus(int iStatus, int iMap)
{
	m_iRonarkMapIndex = iMap;
	m_iActivateStatus = iStatus;

	if (iStatus == RONARK_STATUS_OPENED)
	{
		LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(m_iRonarkMapIndex);
		if (!pMap) { return; }

		LPCHARACTER ronarkBoss = CHARACTER_MANAGER::instance().SpawnMob(RONARK_BOSS, m_iRonarkMapIndex, pMap->m_setting.iBaseX + 383 * 100, pMap->m_setting.iBaseY + 383 * 100, 0, false, -1, true);
		if (ronarkBoss)
		{
			SetBossVID(ronarkBoss->GetVID());
			ronark_hp = ronarkBoss->GetMaxHP();
			sys_log(0, "CRonarkMapManager:: Ronark Boss Summoned In The Map VID(%d) ", GetBossVID());

			if (updateEvent) { event_cancel(&updateEvent); }
			Ronark_event_data* pInfo = AllocEventInfo<Ronark_event_data>();
			updateEvent = event_create(Ronark_event_data_start, pInfo, PASSES_PER_SEC(1));
		}

		sys_log(0, "CRonarkMapManager:: Ronark has start.");
	}
}

int CRonarkMapManager::GetRonarkStatus()
{
	return m_iActivateStatus;
}

bool CRonarkMapManager::IsRonarkMap(int mapindex)
{
	return m_iRonarkMapIndex == mapindex ? true : false;
}

bool CRonarkMapManager::IsRonarkActivate()
{
	return m_iActivateStatus == quest::CQuestManager::instance().GetEventFlag("ronark_event");
}

void CRonarkMapManager::SetMemberLimitCount(int memberlimit)
{
	m_iMemberLimitCount = memberlimit;
}

namespace
{
	struct FPacket
	{
		FPacket(const void* p, int size) : m_pvData(p), m_iSize(size)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			ch->GetDesc()->Packet(m_pvData, m_iSize);
		}

		const void* m_pvData;
		int m_iSize;
	};

	struct FNotice
	{
		FNotice(const char* psz) : m_psz(psz)
		{
		}

		void operator() (LPCHARACTER ch)
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_psz);
		}

		const char* m_psz;
	};

	struct FGoToVillage
	{
		void operator() (LPCHARACTER ch)
		{
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
	};
};

void CRonarkMapManager::Notice(const char* psz)
{
	FNotice f(psz);
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
}

void CRonarkMapManager::Packet(const void* p, int size)
{
	FPacket f(p, size);
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
}

void CRonarkMapManager::SendHome()
{
	FGoToVillage f;
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
}

void CRonarkMapManager::Update()
{
	if (!IsRonarkActivate()) { return; }
	m_TeamDataVec.clear();
	for (TeamDataMap::iterator itor = m_TeamDataMap.begin(); itor != m_TeamDataMap.end(); ++itor)
	{
		auto info = *itor->second;
		TRonarkTeamData table{};
		table.dwID = info.dwID;
		table.dwDamage = GetGuildDamage(info.dwID);
		table.iJoinerCount = info.GetAccumulatedJoinerCount();
		table.iMemberCount = m_iMemberLimitCount;
		table.iRonarkHP = ronark_hp;
		m_TeamDataVec.emplace_back(table);
	}
	if (!m_TeamDataVec.empty())
	{
		std::stable_sort(m_TeamDataVec.begin(), m_TeamDataVec.end(), SortScore);
	}
}

void CRonarkMapManager::SendScorePacket(LPCHARACTER pkChar)
{
	if (!IsRonarkActivate()) { return; }
	if (m_TeamDataVec.empty()) { return; }
	if (!pkChar) { return; }
	if (!pkChar->IsPC()) { return; }
	if (!pkChar->GetDesc()) { return; }
	if (pkChar->GetMapIndex() != m_iRonarkMapIndex) { return; }

	TPacketGCRonarkInfo packet;
	packet.bHeader = HEADER_GC_RONARK_INFO;
	packet.wSize = sizeof(packet) + sizeof(TRonarkTeamData) * m_TeamDataVec.size();
	pkChar->GetDesc()->BufferedPacket(&packet, sizeof(packet));
	pkChar->GetDesc()->Packet(&m_TeamDataVec[0], sizeof(TRonarkTeamData) * m_TeamDataVec.size());
}

void CRonarkMapManager::DelEvent()
{
	if (destroyEvent) { event_cancel(&destroyEvent); }
	if (updateEvent) { event_cancel(&updateEvent); }
	destroyEvent = NULL;
	updateEvent = NULL;
}

void CRonarkMapManager::Reward()
{
	SetRonarkStatus(CRonarkMapManager::RONARK_STATUS_REWARD, m_iRonarkMapIndex);

	DWORD dwWinGuildID = GetWinnerGuild();
	if (dwWinGuildID > 0)
	{
		CGuild* g = CGuildManager::instance().TouchGuild(dwWinGuildID);
		if (g)
		{
			char szNotice[512 + 1];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("RONARK_%s"), g->GetName());
			BroadcastNotice(szNotice);
			// g->RequestRonarkKing(true);
		}

		#ifdef ENABLE_PLAYER_TAG_SYSTEM
		{
			const auto setQuery = fmt::format(FMT_COMPILE("pid={}, tag={}"), dwWinGuildID, TAG_KING_RONARK);
			const auto itemQuery = fmt::format(FMT_COMPILE("INSERT INTO server_settings.player_tags SET {} ON DUPLICATE KEY UPDATE {}"), setQuery, setQuery);
			DBManager::instance().DirectQuery(itemQuery.c_str());
			LoadPlayerTags();
			#ifdef ENABLE_COLLECTIVE_RELOAD
			CollectiveReload(CRELOAD_PLAYER_TAG);
			#endif
		}
		#endif
	}

	LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(m_iRonarkMapIndex);
	if (!pMap) { return; }

	if (destroyEvent) { event_cancel(&destroyEvent); }
	ronark_info* info = AllocEventInfo<ronark_info>();
	destroyEvent = event_create(ronark_destroy_event, info, PASSES_PER_SEC(300));
}

void CRonarkMapManager::ResetEvent()
{
	LPCHARACTER pkBoss = CHARACTER_MANAGER::instance().Find(GetBossVID());
	if (pkBoss)
	{
		M2_DESTROY_CHARACTER(pkBoss);
	}

	for (TeamDataMap::iterator itor = m_TeamDataMap.begin(); itor != m_TeamDataMap.end(); ++itor)
	{
		delete itor->second;
	}

	m_TeamDataMap.clear();
	m_ibossVID = 0;
	m_iRonarkMapIndex = 0;
	m_iActivateStatus = 0;
	m_iMemberLimitCount = 30;
	ronark_hp = 0;
	CRonarkMapManager::SendHome();
	if (destroyEvent) { event_cancel(&destroyEvent); }
	if (updateEvent) { event_cancel(&updateEvent); }
	m_set_pkChr.clear();
}

void CRonarkMapManager::Destroy()
{
	for (TeamDataMap::iterator itor = m_TeamDataMap.begin(); itor != m_TeamDataMap.end(); ++itor)
	{
		delete itor->second;
	}

	m_TeamDataMap.clear();
	m_ibossVID = 0;
	m_iRonarkMapIndex = 0;
	m_iActivateStatus = 0;
	m_iMemberLimitCount = 0;
	destroyEvent = NULL;
	updateEvent = NULL;
	m_set_pkChr.clear();
}

CRonarkMapManager::CRonarkMapManager()
{
	ResetEvent();
	m_iMemberLimitCount = 30;
}

CRonarkMapManager::~CRonarkMapManager()
{
	Destroy();
}
#endif
