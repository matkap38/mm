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
#include "wheel_of_fortune.h"
#include "locale_service.h"
#include <boost/algorithm/string.hpp>
#include "../../libgame/include/grid.h"

#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
#include "GetEventFlagSystem.h"
#endif

#ifdef ENABLE_WHEEL_OF_FORTUNE_SYSTEM
const std::string g_astWheelType[WHEEL_TYPE_MAX_NUM] = {
	"none",				//WHEEL_TYPE_NONE
	"ep",				//WHEEL_TYPE_EP
	"mp",				//WHEEL_TYPE_MP
	"bronze_coin",		//WHEEL_TYPE_BRONZE_COIN
	"silver_coin",		//WHEEL_TYPE_SILVER_COIN
	"gold_coin",		//WHEEL_TYPE_GOLD_COIN
	"emerald_coin",		//WHEEL_TYPE_EMERALD_COIN
	"bronze_yang",		//WHEEL_TYPE_BRONZE_YANG
	"silver_yang",		//WHEEL_TYPE_SILVER_YANG
	"gold_yang",		//WHEEL_TYPE_GOLD_YANG
	"emerald_yang",		//WHEEL_TYPE_EMERALD_YANG
};

const std::string g_astWheelName[WHEEL_TYPE_MAX_NUM] = {
	"Yok",				//WHEEL_TYPE_NONE
	"Ep",				//WHEEL_TYPE_EP
	"Mp",				//WHEEL_TYPE_MP
	"Bronze Coin",		//WHEEL_TYPE_BRONZE_COIN
	"Silver Coin",		//WHEEL_TYPE_SILVER_COIN
	"Gold Coin",		//WHEEL_TYPE_GOLD_COIN
	"Zümrüt Coin",		//WHEEL_TYPE_EMERALD_COIN
	"Bronze Yang",		//WHEEL_TYPE_BRONZE_YANG
	"Silver Yang",		//WHEEL_TYPE_SILVER_YANG
	"Gold Yang",		//WHEEL_TYPE_GOLD_YANG
	"Zümrüt Yang",		//WHEEL_TYPE_EMERALD_YANG
};

CWheelOfFortune::CWheelOfFortune() : m_bActiveStatus(false) {}
CWheelOfFortune::~CWheelOfFortune() { Destroy(); }

void CWheelOfFortune::Destroy()
{
	m_mapTypeList.clear();
	//------------------------//
	for (auto it = m_mapWheelList.begin(); it != m_mapWheelList.end(); ++it)
	{
		it->second.clear();
	}
	m_mapWheelList.clear();
	//------------------------//
	m_vecWheelList.clear();
}

void CWheelOfFortune::Initialize()
{
	sys_log(0, "Initialize: Loading.");
	Destroy();
	for (BYTE i = WHEEL_TYPE_NONE; i < WHEEL_TYPE_MAX_NUM; i++) { LoadWheel(i); }
}

void CWheelOfFortune::Reload()
{
	m_bActiveStatus = true;
	Initialize();
	m_bActiveStatus = false;
}

#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
bool CWheelOfFortune::GetWheelStatus(BYTE bType)
{
	switch (bType)
	{
		case WHEEL_TYPE_EP:				return (gef_wof_ep == false);
		case WHEEL_TYPE_MP:				return (gef_wof_mp == false);
		case WHEEL_TYPE_BRONZE_COIN:	return (gef_wof_bronze_c == false);
		case WHEEL_TYPE_SILVER_COIN:	return (gef_wof_silver_c == false);
		case WHEEL_TYPE_GOLD_COIN:		return (gef_wof_gold_c == false);
		case WHEEL_TYPE_EMERALD_COIN:	return (gef_wof_emerald_c == false);
		case WHEEL_TYPE_BRONZE_YANG:	return (gef_wof_bronze_y == false);
		case WHEEL_TYPE_SILVER_YANG:	return (gef_wof_silver_y == false);
		case WHEEL_TYPE_GOLD_YANG:		return (gef_wof_gold_y == false);
		case WHEEL_TYPE_EMERALD_YANG:	return (gef_wof_emerald_y == false);
	}
	return false;
}
#endif

int CWheelOfFortune::NeedItemVnum(BYTE bType)
{
	if (m_mapTypeList.empty()) { return 0; }
	const auto& iter = m_mapTypeList.find(bType);
	if (iter == m_mapTypeList.end()) { return 0; }
	return iter->second.first;
}

int CWheelOfFortune::NeedItemCount(BYTE bType)
{
	if (m_mapTypeList.empty()) { return 1; }
	const auto& iter = m_mapTypeList.find(bType);
	if (iter == m_mapTypeList.end()) { return 1; }
	return iter->second.second;
}

bool CWheelOfFortune::IsBusy(LPCHARACTER ch)
{
	if (!ch || !ch->GetDesc()) { return true; }
	if (ch->IsHack(true, true) || ch->IsHack(true, false)) { return true; }

	if (!ch->CanWarp())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("TICARETTENSONRABIRSUREBIRYEREGIDEMEZSIN"));
		return true;
	}

	if (ch->GetExchange() || ch->GetMyShop() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen()
		 || ch->IsOpenWindows()
		#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		 || ch->IsAcceOpened()
		#endif
	)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("HATABIRPENCEREACIK"));
		return true;
	}

	return false;
}

void CWheelOfFortune::LoadWheel(BYTE bType)
{
	if ((bType <= WHEEL_TYPE_NONE) || (bType >= WHEEL_TYPE_MAX_NUM)) { return; }

	FILE		*fp;
	char		one_line[256];
	int			value1, value2, value3;
	const char	*delimit = " \t\r\n";
	char		*v, *token_string;

	sys_err("Loading: %s", g_astWheelType[bType].c_str());

	TMapItemList mMapList; mMapList.clear();

	char file_name[256+1];
	snprintf(file_name, sizeof(file_name), "%s/systems/wheel_of_fortune/%s.txt", LocaleService_GetBasePath().c_str(), g_astWheelType[bType].c_str());
	fp = fopen(file_name, "r");

	while (fgets(one_line, 256, fp))
	{
		value1 = value2 = value3 = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delimit);
		if (NULL == token_string)
			continue;

		if ((v = strtok(NULL, delimit)))
			str_to_number(value1, v);

		if ((v = strtok(NULL, delimit)))
			str_to_number(value2, v);

		if ((v = strtok(NULL, delimit)))
			str_to_number(value3, v);

		TOKEN("need")
		{
			m_mapTypeList.emplace(bType, std::make_pair(value1, value2));
		}
		else TOKEN("item")
		{
			mMapList.emplace(std::make_pair(value1, value2), value3);
		}
	}
	fclose(fp);

	if (!mMapList.empty()) { m_mapWheelList.emplace(bType, mMapList); }
	GetItemList(bType);
}

void CWheelOfFortune::GetItemList(BYTE bType)
{
	if (m_mapWheelList.empty()) { return; }
	const auto& iter = m_mapWheelList.find(bType);
	if (iter == m_mapWheelList.end()) { return; }
	const auto& mMapList = iter->second;
	if (mMapList.empty() || (mMapList.size() < 10)) { return; }
	std::vector<TWheelOfFortuneInfoTable> vec_item; vec_item.clear();
	for (const auto& iter : mMapList)
	{
		TWheelOfFortuneInfoTable kTempTab;
		kTempTab.iVnum = iter.first.first;
		kTempTab.iCount = iter.first.second;
		vec_item.emplace_back(kTempTab);
	}
	m_vecWheelList.emplace(bType, vec_item);
}

void CWheelOfFortune::Open(LPCHARACTER ch, BYTE bType)
{
	if (m_bActiveStatus) return;
	if (!ch && !ch->GetDesc()) return;

	#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
	if (GetWheelStatus(bType))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s carki henuz aktif degil!"), g_astWheelName[bType].c_str());
		return;
	}
	#endif

	if (m_vecWheelList.empty()) { return; }
	const auto& iter = m_vecWheelList.find(bType);
	if (iter == m_vecWheelList.end()) { return; }
	const auto& mVecList = iter->second;
	if (mVecList.empty() || (mVecList.size() < 10)) { return; }

	TPacketGCWheelOfFortune packet;
	packet.bHeader = HEADER_GC_WHEEL_OF_FORTUNE;
	packet.bSubHeader = bType;
	packet.iNeedVnum = NeedItemVnum(bType);
	packet.iNeedCount = NeedItemCount(bType);
	packet.wSize = sizeof(packet) + sizeof(TWheelOfFortuneInfoTable) * mVecList.size();
	ch->GetDesc()->BufferedPacket(&packet, sizeof(packet));
	ch->GetDesc()->Packet(&mVecList[0], sizeof(TWheelOfFortuneInfoTable) * mVecList.size());
}

void CWheelOfFortune::Fortune(LPCHARACTER ch, BYTE bType)
{
	if (m_bActiveStatus) return;
	if (!ch && !ch->GetDesc()) return;
	if (m_mapWheelList.empty()) { return; }

	const auto& iter = m_mapWheelList.find(bType);
	if (iter == m_mapWheelList.end()) { return; }
	const auto& mMapList = iter->second;
	if (mMapList.empty() || (mMapList.size() < 10)) { return; }

	std::vector<std::pair<int, int>> m_send_items; m_send_items.clear();
	while (true)
	{
		for (auto it = mMapList.begin(); it != mMapList.end(); ++it)
		{
			int random = number(1,100);
			if (it->second >= random)
			{
				const auto& itFind = std::find(m_send_items.begin(), m_send_items.end(), it->first);
				if (itFind == m_send_items.end())
				{
					m_send_items.emplace_back(it->first.first, it->first.second);
					if (m_send_items.size() >= 10)
						break;
				}
			}
		}
		if (m_send_items.size() >= 10)
			break;
	}

	int luckyWheel = number(0, 9);
	if (luckyWheel == 0)
		if (number(0,1) == 0)
			luckyWheel = number(0, 9);

	ch->SetProtectTime("WOFLuckyItemVnum", m_send_items[luckyWheel].first);
	ch->SetProtectTime("WOFLuckyItemCount", m_send_items[luckyWheel].second);
	ch->SetProtectTime("WOFType", bType);
	ch->SetProtectTime("WOFWorking", 1);

	std::string cmd_wheel = "";
	if (m_send_items.size())
	{
		for (auto it = m_send_items.begin(); it != m_send_items.end(); ++it)
		{
			cmd_wheel += std::to_string(it->first);
			cmd_wheel += "|";
			cmd_wheel += std::to_string(it->second);
			cmd_wheel += "#";
		}
	}
	ch->ChatPacket(CHAT_TYPE_COMMAND, "WofSetItemData %s", cmd_wheel.c_str());
	ch->ChatPacket(CHAT_TYPE_COMMAND, "WofOnSetWhell %d", luckyWheel);

	m_send_items.clear();
}
#endif
