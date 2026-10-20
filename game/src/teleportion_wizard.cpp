#include "stdafx.h"
#include "constants.h"
#include "char.h"
#include "cmd.h"
#include "utils.h"
#include "db.h"
#include "config.h"
#include "packet.h"
#include "p2p.h"
#include "questmanager.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char_manager.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "sectree_manager.h"
#include "teleportion_wizard.h"
#include <boost/algorithm/string.hpp>
#include "../../libgame/include/grid.h"

#ifdef ENABLE_TELEPORTION_WIZARD_SYSTEM
CTeleportionWizard::CTeleportionWizard() : m_bActiveStatus(false) {}
CTeleportionWizard::~CTeleportionWizard() { Destroy(); }

void CTeleportionWizard::Destroy()
{
	m_TeleportListVec.clear();
	m_mapDropItemList.clear();
	m_mapDungeonCooldownList.clear();
}

bool CTeleportionWizard::Initialize()
{
	sys_log(0, "Initialize: Loading.");

	Destroy();

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "%s",
		"SELECT id, type+0, comment, min_level_limit, max_level_limit, alignment, item_vnum, item_count, "
		"image_num, race_num, element+0, mapIndex, coor_x, coor_y, status, view, gold, cooldown"
		" FROM server_settings.teleportion_wizard ORDER BY id;"
	);
	auto pkMsg(DBManager::instance().DirectQuery(szQuery));
	if (!pkMsg)
	{
		sys_err("Initialize::szQuery:!pkMsg");
		return false;
	}

	SQLResult * pRes = pkMsg->Get();
	if (pRes->uiNumRows != 0)
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL)
		{
			int con = 0;

			TWizardDataInfoTable tmp;
			memset(&tmp, 0, sizeof(tmp));
			tmp.id = atoi(row[con++]);
			tmp.type = atoi(row[con++]);
			strlcpy(tmp.comment, row[con++], sizeof(tmp.comment));
			tmp.min_level = atoi(row[con++]);
			tmp.max_level = atoi(row[con++]);
			tmp.alignment = atoi(row[con++]);
			tmp.item_vnum = atoi(row[con++]);
			tmp.item_count = atoi(row[con++]);
			tmp.image_num = atoi(row[con++]);
			tmp.race_num = atoi(row[con++]);
			tmp.element = atoi(row[con++])-1;
			tmp.mapIndex = atoi(row[con++]);
			tmp.coor_x = atoi(row[con++]);
			tmp.coor_y = atoi(row[con++]);
			tmp.status = atoi(row[con++]);
			tmp.show = atoi(row[con++]);
			#ifdef ENABLE_FULL_YANG
			tmp.gold = strtoull(row[con++], NULL, 10);
			#else
			tmp.gold = strtol(row[con++], NULL, 10);
			#endif
			m_TeleportListVec.emplace_back(tmp);

			if (m_mapDungeonCooldownList.find(tmp.id) == m_mapDungeonCooldownList.end())
			{
				char szCooldownText[256+1];
				strlcpy(szCooldownText, row[con++], sizeof(szCooldownText));
				if (strlen(szCooldownText) > 4) { m_mapDungeonCooldownList.emplace(tmp.id, std::string(szCooldownText)); }
			}
		}
	}

	if (!m_TeleportListVec.empty())
	{
		sort(begin(m_TeleportListVec), end(m_TeleportListVec), [](TWizardDataInfoTable p1, TWizardDataInfoTable p2){return p1.id < p2.id;});

		for (auto iter : m_TeleportListVec)
		{
			DWORD dwRaceNum = iter.race_num;
			auto iterDropItem = m_mapDropItemList.find(dwRaceNum);
			if (iterDropItem == m_mapDropItemList.end())
			{
				std::vector<TWizardDropInfoTable> vec_ItemList;
				vec_ItemList.clear();
				GetDropItemList(dwRaceNum, vec_ItemList);
				m_mapDropItemList.emplace(dwRaceNum, vec_ItemList);
			}
		}
	}

	return true;
}

void CTeleportionWizard::Reload()
{
	m_bActiveStatus = true;
	if (Initialize()) { m_bActiveStatus = false; }
}

void CTeleportionWizard::GetDropItemList(DWORD dwRaceNum, std::vector<TWizardDropInfoTable>& vec_item)
{
	static std::vector<std::pair<DWORD,int>> s_vec_item;
	s_vec_item.clear();
	if (!ITEM_MANAGER::instance().CreateDropItemVectorTW(dwRaceNum, s_vec_item))
	{
		s_vec_item.clear();
		return;
	}

	if (s_vec_item.size() < 1)
	{
		s_vec_item.clear();
		return;
	}

	TWizardDropInfoTable kTempTab;
	constexpr BYTE dwChestDropPageCount = 5;
	CGrid* pGrids[dwChestDropPageCount];
	for (int i = 0; i < dwChestDropPageCount; ++i)
	{
		pGrids[i] = new CGrid(5, 8);
		pGrids[i]->Clear();
	}

	for (auto item : s_vec_item)
	{
		DWORD dwVnum = item.first;
		int iCount = item.second;

		const TItemTable* itemTable = ITEM_MANAGER::instance().GetTable(dwVnum);
		if (!itemTable)
			continue;

		for (int iPage = 0; iPage < dwChestDropPageCount; ++iPage)
		{
			int iPos = pGrids[iPage]->FindBlank(1, itemTable->bSize);
			if (iPos >= 0)
			{
				pGrids[iPage]->Put(iPos, 1, itemTable->bSize);

				kTempTab.bPageIndex = iPage + 1;
				kTempTab.bSlotIndex = iPos;

				kTempTab.dwItemVnum = dwVnum;
				kTempTab.iItemCount = iCount;

				vec_item.emplace_back(kTempTab);
				break;
			}
		}
	}

	for (int i = 0; i < dwChestDropPageCount; ++i)
	{
		pGrids[i]->Clear();
		M2_DELETE(pGrids[i]);
	}

	s_vec_item.clear();
}

int CTeleportionWizard::GetCooldown(LPCHARACTER ch, int id)
{
	if (m_bActiveStatus) return 0;
	if (!ch && !ch->GetDesc()) return 0;
	if (m_mapDungeonCooldownList.empty()) return 0;

	auto iter = m_mapDungeonCooldownList.find(id);
	if (iter == m_mapDungeonCooldownList.end())
		return 0;

	int iTime = ch->GetQuestFlag(iter->second);
	if (get_global_time() > iTime)
		return 0;

	return iTime;
}
//###################################################################################################################//

void CTeleportionWizard::Warp(LPCHARACTER ch, int id)
{
	if (m_bActiveStatus) return;
	if (!ch && !ch->GetDesc()) return;
	if (m_TeleportListVec.empty()) return;

	auto iter = std::find_if(m_TeleportListVec.begin(), m_TeleportListVec.end(), [&](const TWizardDataInfoTable & sR) { return (sR.id == id); });
	if (iter == m_TeleportListVec.end())
		return;

	TWizardDataInfoTable& regen = *iter;

	if (regen.status == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu bolge henuz aktif edilmedi!"));
		return;
	}

	if (ch->GetLevel() < regen.min_level)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Leveliniz yetersiz!"));
		return;
	}
	else if (ch->GetLevel() > regen.max_level)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Leveliniz yuksek!"));
		return;
	}

	if ((regen.alignment > 0) && (ch->GetAlignment() < (regen.alignment*10)))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Dereceniz dusuk!"));
		return;
	}

	/*if (GetCooldown(ch, regen.id) > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("sureniz henuz dolmamis"));
		return;
	}*/

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	if (ch->IsMyShopTime())
	{
		return;
	}

	if (ch->IsExchangeTime())
	{
		return;
	}

	if (ch->IsUnderRefine())
	{
		return;
	}

	#ifdef ENABLE_FULL_YANG
	if (ch->GetGold() < static_cast<int64_t>(regen.gold))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Yeterli yang yok!"));
		return;
	}
	ch->GoldChange(-regen.gold);
	#else
	if (ch->GetGold() < (INT)(regen.gold))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Yeterli yang yok!"));
		return;
	}
	ch->PointChange(POINT_GOLD, -regen.gold, false);
	#endif

	long x = regen.coor_x;
	long y = regen.coor_y;

	x *= 100;
	y *= 100;

	// patch with warp check
	ch->SetMyShopTime();
	ch->SetMyExchangeTime();
	ch->SetWarpStatus(true);

	ch->WarpSet(x, y);
	ch->Stop();
}

void CTeleportionWizard::DropInfo(LPCHARACTER ch, int id)
{
	if (m_bActiveStatus) return;
	if (!ch && !ch->GetDesc()) return;
	if (m_TeleportListVec.empty()) return;

	auto iter = std::find_if(m_TeleportListVec.begin(), m_TeleportListVec.end(), [&](const TWizardDataInfoTable & sR) { return (sR.id == id); });
	if (iter == m_TeleportListVec.end())
		return;

	TWizardDataInfoTable& regen = *iter;
	DWORD dwRaceNum = regen.race_num;
	if (dwRaceNum <= 0)
		return;

	auto iterDropItem = m_mapDropItemList.find(dwRaceNum);
	if (iterDropItem == m_mapDropItemList.end())
		return;

	auto vec_ItemList = iterDropItem->second;

	TPacketGCTpWizardInfo packet;
	packet.bHeader = HEADER_GC_TELEPORTION_WIZARD_INFO;
	packet.bSubHeader = TP_WIZARD_DROP_INFO;
	packet.wSize = sizeof(packet) + sizeof(TWizardDropInfoTable) * vec_ItemList.size();
	packet.dwRaceNum = dwRaceNum;

	ch->GetDesc()->BufferedPacket(&packet, sizeof(packet));
	ch->GetDesc()->Packet(&vec_ItemList[0], sizeof(TWizardDropInfoTable) * vec_ItemList.size());
}

void CTeleportionWizard::SendPacket(LPCHARACTER ch)
{
	if (m_bActiveStatus) return;
	if (!ch && !ch->GetDesc()) return;
	if (m_TeleportListVec.empty()) return;

	for (auto iter = m_TeleportListVec.begin(); iter != m_TeleportListVec.end(); ++iter)
	{
		TWizardDataInfoTable& list = *iter;
		if (list.status == 0) continue;
		list.cooldown = GetCooldown(ch, list.id);
	}

	TPacketGCTpWizardInfo packet;
	packet.bHeader = HEADER_GC_TELEPORTION_WIZARD_INFO;
	packet.bSubHeader = TP_WIZARD_DATA_INFO;
	packet.wSize = sizeof(packet) + sizeof(TWizardDataInfoTable) * m_TeleportListVec.size();
	packet.dwRaceNum = 0;

	ch->GetDesc()->BufferedPacket(&packet, sizeof(packet));
	ch->GetDesc()->Packet(&m_TeleportListVec[0], sizeof(TWizardDataInfoTable) * m_TeleportListVec.size());
}
#endif
