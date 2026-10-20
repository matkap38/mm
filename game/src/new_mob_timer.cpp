#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "desc.h"
#include "db.h"
#include "buffer_manager.h"
#include "start_position.h"
#include "questmanager.h"
#include "char.h"
#include "char_manager.h"
#include "sectree_manager.h"
#include "new_mob_timer.h"
#include "utils.h"
#include <algorithm>
#include "mob_manager.h"
#include "dungeon.h"

namespace NewMobTimer {
//namespace
/*
	//ID > VNUM > MAP_INDEX > X_POS > Y_POS > PER_HOUR > PER_MIN >
	sıralama	bossid	mapindex	x kordi	y kordi	belli saat aralığı	belli dakika aralığı
	EVERY_HOUR = her saat demek PEER_HOUR a EVERY_HOUR bunu koyarsan her saat çıkar genelde dk için yapılır
	örnek PEER_HOUR EVERY_HOUR yaptın PER_MIN 10 yaptın 10 dkda bir çıkar
*/

#define CODING_PHASE	//player ID'si 1 olan oyuncuya chatpacket gecer.
#ifdef CODING_PHASE
void ChatToAdmin_NMT(BYTE type, const char * format, ...)
{
	LPCHARACTER adminCh = CHARACTER_MANAGER::Instance().FindByPID(1);
	if (!adminCh) { return; }
	LPDESC d = adminCh->GetDesc();

	if (!d || !format) { return; }

	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	int len = vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	struct packet_chat pack_chat;

	pack_chat.header = HEADER_GC_CHAT;
	pack_chat.size = sizeof(struct packet_chat) + len;
	pack_chat.type = type;
	pack_chat.id = 0;
	pack_chat.bEmpire = d->GetEmpire();

	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(struct packet_chat));
	buf.write(chatbuf, len);

	d->Packet(buf.read_peek(), buf.size());
}
#endif

EVENTINFO(main_event_info)
{
	WORD updateDelay;
	main_event_info() :updateDelay(0) {}
};

EVENTFUNC(main_timer)
{
	main_event_info* info = dynamic_cast<main_event_info*>(event->info);
	if (info == NULL) { return 0; }
	CNewMobTimer::Instance().Update();
	return PASSES_PER_SEC(info->updateDelay);
}

CNewMobTimer::CNewMobTimer() { }
void CNewMobTimer::Destroy() { }

void CNewMobTimer::Initialize()
{
	#ifdef DISABLE_IN_CH99
	if (g_bChannel == 99) { return; }
	#endif

	const auto pkMsg(DBManager::instance().DirectQuery("SELECT id, vnum, map_index, x, y, channel, per_hour, per_min FROM server_settings.mob_timer"));
	if (pkMsg->Get()->uiNumRows == 0)
	{
		sys_err("table is empty");
		return;
	}

	if (!spawnedVIDS.empty())
	{
		for (const auto& iter : spawnedVIDS)
		{
			if (iter.second == 0) { continue; }
			LPCHARACTER pkMob = CHARACTER_MANAGER::instance().Find(iter.second);
			if (pkMob && !pkMob->IsDead())
			{
				// pkMob->Dead();
				pkMob->DeadNoReward(); // @fixme188 from Dead()
			}
		}
	}
	spawnedVIDS.clear();

	m_mapMobTimerInfoList.clear();
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(pkMsg->Get()->pSQLResult)) != nullptr)
	{
		int col = 0;
		int iID = 0;
		str_to_number(iID, row[col++]);

		TMobTimerInfo info = {};
		str_to_number(info.dwVnum, row[col++]);

		const auto* pMob = CMobManager::instance().Get(info.dwVnum);
		if (!pMob)
		{
			sys_err("have wrong boss! vnum : %u pls check it!", info.dwVnum);
			continue;
		}

		str_to_number(info.dwMapIDX, row[col++]);
		str_to_number(info.posX, row[col++]);
		str_to_number(info.posY, row[col++]);
		str_to_number(info.bChannel, row[col++]);
		str_to_number(info.perHour, row[col++]);
		str_to_number(info.perMin, row[col++]);

		m_mapMobTimerInfoList.emplace(iID, info);
		spawnedVIDS[iID] = static_cast<DWORD>(0);

		#ifdef ENABLE_DUNGEON_BOSS_ICON_IN_MAP
		const SECTREE_MAP* sectreeMap = SECTREE_MANAGER::instance().GetMap(info.dwMapIDX);
		if (sectreeMap)
		{
			const long x = info.posX, y = info.posY;

			int sx = x, ex = 0, sy = y, ey = 0;
			ex = sx + 0 * 2;
			sx *= 100;
			ex *= 100;
			ey = sy + 0 * 2;
			sy *= 100;
			ey *= 100;
			sx += sectreeMap->m_setting.iBaseX;
			ex += sectreeMap->m_setting.iBaseX;
			sy += sectreeMap->m_setting.iBaseY;
			ey += sectreeMap->m_setting.iBaseY;

			if (sx > ex)
			{
				sx ^= ex;
				ex ^= sx;
				sx ^= ex;
			}

			if (sy > ey)
			{
				sy ^= ey;
				ey ^= sy;
				sy ^= ey;
			}

			int timeHour = info.perHour;
			int timeMin = info.perMin;
			int checkMin = timeMin;
			if (timeHour != EVERY_HOUR) { checkMin += (timeHour * 60); }
			checkMin *= 60;

			SECTREE_MANAGER::instance().InsertNPCPosition(info.dwMapIDX, pMob->m_table.bType, info.dwVnum,
				(sx+ex) / 2 - sectreeMap->m_setting.iBaseX,
				(sy+ey) / 2 - sectreeMap->m_setting.iBaseY,
				checkMin);
		}
		#endif
	}

	if (updateTimer)
	{
		event_cancel(&updateTimer);
	}

	main_event_info * info;
	info = AllocEventInfo<main_event_info>();
	info->updateDelay = MAIN_UPDATE_INTERVAL;
	updateTimer = event_create(main_timer, info, PASSES_PER_SEC(info->updateDelay));

	sys_log(0, "Zamanlayici baslatildi.");
}

WORD CNewMobTimer::GetSize()
{
	return m_mapMobTimerInfoList.size();
}

TMobTimerInfo* CNewMobTimer::GetData(int iID)
{
	if (m_mapMobTimerInfoList.empty()) { return NULL; }
	auto it = m_mapMobTimerInfoList.find(iID);
	return (it != m_mapMobTimerInfoList.end() ? &it->second : NULL);
}

DWORD CNewMobTimer::GetVnum(int iID)
{
	if (m_mapMobTimerInfoList.empty()) { return 0; }
	const auto& it = m_mapMobTimerInfoList.find(iID);
	return (it != m_mapMobTimerInfoList.end()) ? it->second.dwVnum : 0;
}

void CNewMobTimer::Update()
{
	struct timeval tv;
	struct tm ltm;
	int	nowHour = 0, nowMin = 0;
	gettimeofday(&tv, NULL);
	localtime_r((time_t*)&tv.tv_sec, &ltm);
	nowHour = ltm.tm_hour;
	nowMin = ltm.tm_min;

	int timeAsMin = nowMin + (nowHour * 60);

	#ifdef CODING_PHASE
	ChatToAdmin_NMT(1, "Guncellenmis > %d:%d | (%d)", nowHour, nowMin, timeAsMin);
	ChatToAdmin_NMT(1, " vidDurumu(%d|%d) >> ", spawnedVIDS.size(), GetSize());
	for (const auto& it : spawnedVIDS) { ChatToAdmin_NMT(1, "  %d - %d", it.first, it.second); }
	#endif

	if (m_mapMobTimerInfoList.empty()) { return; }

	for (const auto& iter : m_mapMobTimerInfoList)
	{
		WORD i = iter.first;
		auto info = iter.second;

		if (CanSpawnNow(i, timeAsMin))
		{
			#ifdef CODING_PHASE
			DWORD bossVnum = GetVnum(i);
			ChatToAdmin_NMT(1, "%d- %d simdi dogabilir (1)", i, bossVnum);
			#endif

			if (g_bChannel > info.bChannel)
			{
				#ifdef CODING_PHASE
				ChatToAdmin_NMT(1, "%d - %d CH%u: Bu chde dogamaz! MaxCH: %u", i, bossVnum, g_bChannel, info.bChannel);
				#endif
				continue;
			}

			if (CanSpawn(i))
			{
				SpawnBoss(i);
				#ifdef CODING_PHASE
				ChatToAdmin_NMT(1, "%d dogabilir (2)", bossVnum);
				#endif
			}
		}
	}
}

bool CNewMobTimer::CanSpawnNow(WORD bossID, int timeAsMin)
{
	if (bossID > GetSize()) { return false; }
	const auto& iter = GetData(bossID);
	if (iter == NULL) { sys_err("CanSpawnNow::Null:%d", bossID); return false; }
	int timeHour = iter->perHour;
	int timeMin = iter->perMin;
	int checkMin = timeMin;
	if (timeHour != EVERY_HOUR)
	{
		checkMin += (timeHour * 60);
	}	//eer every_hour degilse mod aldgmz saniyeye saat*60 ekle
	return (timeAsMin % checkMin) == 0;
}

bool CNewMobTimer::CanSpawn(WORD bossID)
{
	if (bossID > GetSize()) { return false; }
	if (spawnedVIDS.empty()) { return false; }
	const auto& iter = spawnedVIDS.find(bossID);
	if (iter == spawnedVIDS.end()) { return false; }
	if (iter->second > 0)
	{
		LPCHARACTER pkMob = CHARACTER_MANAGER::instance().Find(iter->second);
		if (pkMob && !pkMob->IsDead())
		{
			iter->second = 0;
			pkMob->DeadNoReward(); // @fixme188 from Dead()
			return true;
		}
	}
	return (iter->second == 0);
}

void CNewMobTimer::SpawnBoss(WORD bossID)
{
	const auto& iter = GetData(bossID);
	if (iter == NULL) { sys_err("SpawnBoss::Null:%d", bossID); return; }
	DWORD mobVnum = iter->dwVnum;
	DWORD mapIndex = iter->dwMapIDX;
	if (mobVnum == 0 || mapIndex == 0) { return; }
	SECTREE_MAP* sectreeMap = SECTREE_MANAGER::instance().GetMap(mapIndex);
	if (sectreeMap)
	{
		LPCHARACTER spawnMob = CHARACTER_MANAGER::instance().SpawnMob(mobVnum, mapIndex, sectreeMap->m_setting.iBaseX + iter->posX * 100, sectreeMap->m_setting.iBaseY + iter->posY * 100, 0);
		if (spawnMob)
		{
			spawnedVIDS[bossID] = static_cast<DWORD>(spawnMob->GetVID());
			spawnMob->SetSpawnX(spawnMob->GetX());
			spawnMob->SetSpawnY(spawnMob->GetY());
		}
	}
}

void CNewMobTimer::BossIsDead(DWORD dwVID)
{
	#ifdef CODING_PHASE
	ChatToAdmin_NMT(1, "Patron olduruldu %u", dwVID);
	#endif

	for (auto& iter : spawnedVIDS)
	{
		if (iter.second == dwVID)
		{
			iter.second = 0;
			#ifdef CODING_PHASE
			ChatToAdmin_NMT(1, "%u, bulundu ve 0 olarak degistirildi", dwVID);
			#endif
			break;
		}
	}
}

};	//Namespace
