#include "stdafx.h"
#ifdef ENABLE_TANAKA_EVENT
#include "NewTanakaEvent.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "desc_manager.h"
#include "entity.h"
#include "config.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "sectree_manager.h"

static auto SendClient(LPCHARACTER ch, const uint8_t subHeader, const void* c_pvData, const size_t iSize) -> void
{
	if (!ch || !ch->GetDesc()) { return; }

	TEMP_BUFFER buf;
	if (iSize > 0)
	{
		buf.write(c_pvData, iSize);
	}

	TDynamicPacketInfo packet{};
	packet.bHeader = HEADER_GC_TANAKA_EVENT;
	packet.bSubHeader = subHeader;
	packet.wSize = sizeof(packet) + iSize > 0 ? buf.size() : 0;

	ch->GetDesc()->BufferedPacket(&packet, sizeof(packet));
	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}

EVENTINFO(tanaka_purge_timer_info)
{
	CNewTanakaEventManager* pClass;
	BYTE bSpawn;
	tanaka_purge_timer_info() : pClass(nullptr), bSpawn(0) {}
};

EVENTFUNC(tanaka_purge_timer_event)
{
	auto* info = dynamic_cast<tanaka_purge_timer_info *>(event->info);
	if (!info) { return 0; }
	auto pClass = info->pClass;
	if (!pClass) { return 0; }

	SECTREE_MANAGER::instance().PurgeMonstersInMap(TANAKA_EVENT_MAP_INDEX);

	if (info->bSpawn == 1)
	{
		info->bSpawn = 2;
		return PASSES_PER_SEC(1);
	}
	else if (info->bSpawn == 2)
	{
		pClass->SpawnTanaka();
		return 0;
	}

	return 0;
}

EVENTINFO(tanaka_exit_timer_info)
{
	CNewTanakaEventManager* pClass;
	tanaka_exit_timer_info() : pClass(nullptr) {}
};

EVENTFUNC(tanaka_exit_timer_event)
{
	auto* info = dynamic_cast<tanaka_exit_timer_info *>( event->info );
	if (!info) { return 0; }

	auto pClass = info->pClass;
	if (!pClass) { return 0; }

	pClass->ExitAllPlayer();
	pClass->Initialize();
	return 0;
}

EVENTINFO(tanaka_start_timer_info)
{
	CNewTanakaEventManager* pClass;
	uint8_t left_min;
	tanaka_start_timer_info() : pClass(nullptr), left_min(0) {}
};

EVENTFUNC(tanaka_start_timer_event)
{
	auto* info = dynamic_cast<tanaka_start_timer_info *>( event->info );
	if (!info) { return 0; }

	auto pClass = info->pClass;
	if (!pClass) { return 0; }

	char szNotice[256];
	if (info->left_min == 0)
	{
		snprintf(szNotice, sizeof(szNotice), "%s", LC_TEXT("Tanaka event has started."));
		BroadcastNotice(szNotice);
		pClass->StartEvent();
		return 0;
	}

	snprintf(szNotice, sizeof(szNotice), LC_TEXT("Tanaka event will start in %d minutes."), info->left_min);
	BroadcastNotice(szNotice);
	--info->left_min;
	return PASSES_PER_SEC(60);
}

auto CNewTanakaEventManager::IsTanakaRace(DWORD dwRace) -> bool
{
	if ((dwRace == TANAKA_MOB_VNUM_1) || (dwRace == TANAKA_MOB_VNUM_2) || (dwRace == TANAKA_MOB_VNUM_3)) { return true; }
	return false;
}

auto CNewTanakaEventManager::Initialize() -> void
{
	m_byEventLevel = 0;
	m_State = Eventstate::EVENT_STATE_NONE;
	m_bySpawnCount = 0;
	m_byKillCount = 0;
	m_pEvent = nullptr;
	m_pSpawnEvent = nullptr;
	m_mapPlayerInfo.clear();
	character_vec.clear();
}

auto CNewTanakaEventManager::SpawnEvent(BYTE bSpawn, long lTime) -> void
{
	if (m_pSpawnEvent)
	{
		event_cancel(&m_pSpawnEvent);
		m_pSpawnEvent = nullptr;
	}

	SECTREE_MANAGER::instance().StopDamageMonstersInMap(TANAKA_EVENT_MAP_INDEX);
	auto* info = AllocEventInfo<tanaka_purge_timer_info>();
	info->pClass = this;
	info->bSpawn = bSpawn;
	m_pSpawnEvent = event_create(tanaka_purge_timer_event, info, lTime);
}

auto CNewTanakaEventManager::OpenEvent() -> void
{
	m_byEventLevel = 1;
	m_bySpawnCount = spawn_tanaka_table[m_byEventLevel];
	m_byKillCount = 0;
	m_State = Eventstate::EVENT_STATE_PREPARING;
	SendP2PLoginRequest();

	TTanakaGGEventState packet{};
	packet.bHeader = HEADER_GG_TANAKA_EVENT;
	packet.eventState = static_cast<uint8_t>(m_State);
	P2P_MANAGER::instance().Send(&packet, sizeof(TTanakaGGEventState));

	char szNotice[256];
	snprintf(szNotice, sizeof(szNotice), "%s", LC_TEXT("TANAKA_EVENT_OPEN"));
	BroadcastNotice(szNotice);
}

auto CNewTanakaEventManager::StartEvent() -> void
{
	m_byEventLevel = 1;
	m_bySpawnCount = spawn_tanaka_table[m_byEventLevel];
	m_byKillCount = 0;
	m_State = Eventstate::EVENT_STATE_OPEN;
	SpawnTanaka();
	Update();
}

auto CNewTanakaEventManager::StartExitEvent() -> void
{
	if (m_pEvent)
	{
		event_cancel(&m_pEvent);
		m_pEvent = nullptr;
	}

	auto* info = AllocEventInfo<tanaka_exit_timer_info>();
	info->pClass = this;
	m_pEvent = event_create(tanaka_exit_timer_event, info, PASSES_PER_SEC(30));
}

auto CNewTanakaEventManager::EndEvent() -> void
{
	SpawnEvent(0, PASSES_PER_SEC(3));
	StartExitEvent();

	char szNotice[256];
	snprintf(szNotice, sizeof(szNotice), "%s", LC_TEXT("Tanaka event has ended."));
	SendNotice(szNotice);
}

auto CNewTanakaEventManager::P2PAction(const uint8_t state) -> void
{
	switch(state)
	{
		case 0: // EVENT_STATE_NONE
		{
			Initialize();
		}
		break;

		case 1: // -> EVENT_STATE_PREPARING
		{
			m_State = Eventstate::EVENT_STATE_PREPARING;
			SendP2PLoginRequest();
		}
		break;

		case 2: // EVENT_STATE_OPEN
		{
			m_State = Eventstate::EVENT_STATE_OPEN;
			// StartEvent();
		}
		break;
	}
}

auto CNewTanakaEventManager::SendP2PLoginRequest() -> void
{
	const auto& c_ref_desc_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_desc_set.begin(), c_ref_desc_set.end(), [&](const LPDESC& d)
	{
		if (!d) { return; }
		auto ch = d->GetCharacter();
		if (!ch || !ch->GetDesc()) { return; }
		if (ch->GetLevel() >= TANAKA_EVENT_REGISTER_MIN_LEVEL)
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "tanaka_event_request");
		}
	});
}

auto CNewTanakaEventManager::UpdateEventLevel() -> void
{
	if (GetEventLevel() >= TANAKA_EVENT_MAX_LEVEL)
	{
		EndEvent();
		return;
	}

	++m_byEventLevel;

	if (m_byEventLevel > TANAKA_EVENT_MAX_LEVEL)
	{
		m_byEventLevel = (TANAKA_EVENT_MAX_LEVEL+1);
		m_bySpawnCount = 0;
		m_byKillCount = 0;
		Update();
		return;
	}

	m_bySpawnCount = spawn_tanaka_table[m_byEventLevel];
	m_byKillCount = 0;

	SpawnEvent(1, PASSES_PER_SEC(1.5));
	Update();
}

auto CNewTanakaEventManager::SpawnTanaka() -> void
{
	if (m_bySpawnCount < 1) { return; }

	DWORD dwTanaka = 0;
	if (m_byEventLevel == 3)
		dwTanaka = TANAKA_MOB_VNUM_3;
	else if (m_byEventLevel == 2)
		dwTanaka = TANAKA_MOB_VNUM_2;
	else if (m_byEventLevel == 1)
		dwTanaka = TANAKA_MOB_VNUM_1;
	else
		return;

	for (int32_t i = 0; i < m_bySpawnCount; ++i)
	{
		CHARACTER_MANAGER::instance().SpawnMob(dwTanaka, TANAKA_EVENT_MAP_INDEX, TANAKA_EVENT_SPAWN_X * 100, TANAKA_EVENT_SPAWN_Y * 100, 0, false, 360);
	}

	char szNotice[256];
	snprintf(szNotice, sizeof(szNotice), "%s", LC_TEXT("tanaka spawn ok"));
	BroadcastNotice(szNotice);
}

auto CNewTanakaEventManager::KillTanaka(const LPCHARACTER ch) -> void
{
	if (m_byEventLevel > TANAKA_EVENT_MAX_LEVEL) { return; }
	if (!ch || !ch->GetDesc()) { return; }
	const auto dwPID = ch->GetPlayerID();
	const auto& it = m_mapPlayerInfo.find(dwPID);
	if (it != m_mapPlayerInfo.end())
	{
		it->second.wKillCount++;

		++m_byKillCount;
		if (m_byKillCount >= kill_tanaka_table[m_byEventLevel])
		{
			UpdateEventLevel();
		}
		Update();
	}
}
 
auto CNewTanakaEventManager::DamageTanaka(LPCHARACTER ch, uint32_t dwDamage) -> void
{
	if (!ch || !ch->GetDesc()) { return; }
	const auto& it = m_mapPlayerInfo.find(ch->GetPlayerID());
	if (it != m_mapPlayerInfo.end())
	{
		it->second.totalDamage += dwDamage;
		Update(ch);
	}
}

auto CNewTanakaEventManager::WarpPlayer(LPCHARACTER ch) -> void
{
	if (!ch || !ch->GetDesc()) { return; }

	BYTE bEmpire = ch->GetEmpire();
	if (bEmpire == 1)//Red
		ch->WarpSet(TANAKA_EVENT_WARP_RED_X * 100, TANAKA_EVENT_WARP_RED_Y * 100);
	else if (bEmpire == 2)//Yellow
		ch->WarpSet(TANAKA_EVENT_WARP_YELLOW_X * 100, TANAKA_EVENT_WARP_YELLOW_Y * 100);
	else if (bEmpire == 3)//Blue
		ch->WarpSet(TANAKA_EVENT_WARP_BLUE_X * 100, TANAKA_EVENT_WARP_BLUE_Y * 100);
}

auto CNewTanakaEventManager::AddPlayer(LPCHARACTER ch) -> void
{
	if (!ch || !ch->GetDesc()) { return; }

	if (m_State == Eventstate::EVENT_STATE_NONE && !ch->IsGM())
	{
		ch->GoHome();
		return;
	}

	const auto& it = m_mapPlayerInfo.find(ch->GetPlayerID());
	if (it == m_mapPlayerInfo.end())
	{
		TPlayerInfo info{};
		info.dwPID = ch->GetPlayerID();
		strlcpy(info.szName, ch->GetName(), sizeof(info.szName));
		info.wKillCount = 0;
		info.totalDamage = 0;
		m_mapPlayerInfo.insert(std::make_pair(info.dwPID, info));
	}

	ch->SetPKMode(PK_MODE_FREE);
	character_vec.emplace_back(ch);
	SendClient(ch, TANAKA_EVENT_SUBHEADER_GC_OPEN_UI, nullptr, 0);
	Update(ch);
	Update();
}

auto CNewTanakaEventManager::ExitPlayer(LPCHARACTER ch) -> void
{
	if (!ch || !ch->GetDesc()) { return; }
	character_vec.erase(std::remove_if(character_vec.begin(), character_vec.end(), [&](const LPCHARACTER& c) { return c->GetPlayerID() == ch->GetPlayerID(); }), character_vec.end());
}

auto CNewTanakaEventManager::ExitAllPlayer() -> void
{
	for (const auto& ch : character_vec)
	{
		if (!ch || !ch->GetDesc()) { continue; }
		ch->GoHome();
	}
	Initialize();
}

auto CNewTanakaEventManager::Update() -> void
{
	for (const auto& ch : character_vec)
	{
		if (!ch || !ch->GetDesc()) { continue; }

		TTanakaGCUpdate packet{};
		if (m_byEventLevel > TANAKA_EVENT_MAX_LEVEL)
		{
			packet.eventLevel = TANAKA_EVENT_MAX_LEVEL;
			packet.tanakaCount = 0;
			packet.leftTanakaCount = 0;
		}
		else
		{
			packet.eventLevel = m_byEventLevel;
			packet.tanakaCount = (m_bySpawnCount - m_byKillCount);
			packet.leftTanakaCount = (kill_tanaka_table[m_byEventLevel] - m_byKillCount);
		}
		SendClient(ch, TANAKA_EVENT_SUBHEADER_GC_UPDATE, &packet, sizeof(packet));
	}
}

auto CNewTanakaEventManager::Update(LPCHARACTER ch) -> void
{
	if (!ch || !ch->GetDesc()) { return; }
	const auto& it = m_mapPlayerInfo.find(ch->GetPlayerID());
	if (it != m_mapPlayerInfo.end())
	{
		TTanakaGCPlayerInfo packet{};
		packet.totalDamage = it->second.totalDamage;
		SendClient(ch, TANAKA_EVENT_SUBHEADER_GC_UPDATE_PLAYER_INFO, &packet, sizeof(packet));
	}
}

auto CNewTanakaEventManager::SendGivedItemInfo(const char* name,  const uint32_t vnum, const uint8_t count) -> void
{
	for (const auto& ch : character_vec)
	{
		if (!ch || !ch->GetDesc()) { continue; }
		TTanakaGCUpdateItemInfo packet{};
		strlcpy(packet.szName, name, sizeof(packet.szName));
		packet.vnum = vnum;
		packet.count = count;
		SendClient(ch, TANAKA_EVENT_SUBHEADER_GC_UPDATE_ITEM_INFO, &packet, sizeof(packet));
	}
}
#endif
