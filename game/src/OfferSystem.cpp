#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "packet.h"
#include "desc.h"
#include "buffer_manager.h"
#include "start_position.h"
#include "questmanager.h"
#include "char.h"
#include "char_manager.h"
#include "sectree_manager.h"
#include "OfferSystem.h"
#include "utils.h"
#include "db.h"
#include "cmd.h"
#include <algorithm>
#include "../../libgame/include/grid.h"
#include "item_manager.h"
#include "item.h"

#ifdef ENABLE_YAYINCI_SISTEMI
	#include "yayinci.h"
#endif

#ifdef ENABLE_OFFER_SYSTEM
//---------------------//
#define MAX_ATTRIBUTE 7
//---------------------//

COfferSystem::COfferSystem()
{
	m_OfferStatus = true;
}

COfferSystem::~COfferSystem()
{
	m_OfferStatus = true;

	Destroy();
}

void COfferSystem::Destroy()
{
	m_offerList.clear();
	m_offerItemList.clear();
}

void COfferSystem::Install()
{
	LoadFirsatList();
	LoadFirsatItemList();
}

bool COfferSystem::LoadFirsatList()
{
	m_offerList.clear();

	char szQuery[2048];
	snprintf(szQuery, sizeof(szQuery), 
		"SELECT offer_id"
		", UNIX_TIMESTAMP(offer_start)"
		", UNIX_TIMESTAMP(offer_end)"
		", old_price"
		", new_price"
		" FROM server_settings.firsat_liste WHERE show_in_table = 1 ORDER BY offer_id DESC;"
	);

	auto pMsg_Item(DBManager::Instance().DirectQuery(szQuery));
	if (pMsg_Item->Get()->uiNumRows == 0)
	{
		return false;
	}

	MYSQL_ROW mRow;
	while (NULL != (mRow = mysql_fetch_row(pMsg_Item->Get()->pSQLResult)))
	{
		BYTE cur = 0;

		TOfferInfo addToList;
		str_to_number(addToList.offerID, mRow[cur++]);

		//---------------------------------------------//
		int iAddTime = 0;
		int iOfferTime = 0;
		str_to_number(iAddTime, mRow[cur++]);
		str_to_number(iOfferTime, mRow[cur++]);
		addToList.offerStartRemainSec = DWORD((iAddTime > 0) ? iAddTime : 0);
		addToList.offerEndRemainSec = DWORD((iOfferTime > 0) ? iOfferTime : 0);
		//---------------------------------------------//

		str_to_number(addToList.offerOldPrice, mRow[cur++]);
		str_to_number(addToList.offerNewPrice, mRow[cur++]);

		m_offerList.emplace_back(addToList);
	}

	return true;
}

bool COfferSystem::LoadFirsatItemList()
{
	if (m_offerList.empty())
	{
		return false;
	}

	m_offerItemList.clear();

	for (itertype(m_offerList) it = m_offerList.begin(); it != m_offerList.end(); it++)
	{
		char szQuery[2048];
		snprintf(szQuery, sizeof(szQuery), "SELECT * FROM server_settings.firsat_items WHERE offer_id = %d;", it->offerID);

		auto pMsg(DBManager::Instance().DirectQuery(szQuery));
		if (pMsg->Get()->uiNumRows == 0)
		{
			continue;
		}

		CGrid offerGrid = CGrid(OFFER_ITEM_GRID_X, OFFER_ITEM_GRID_Y);
		offerGrid.Clear();

		MYSQL_ROW mRow;
		while (NULL != (mRow = mysql_fetch_row(pMsg->Get()->pSQLResult)))
		{
			BYTE cur = 0;
			TOfferItemInfo addToList;
			str_to_number(addToList.itemID, mRow[cur++]);
			str_to_number(addToList.itemOfferID, mRow[cur++]);
			str_to_number(addToList.itemVnum, mRow[cur++]);
			str_to_number(addToList.itemCount, mRow[cur++]);

			for (BYTE x = 0; x < ITEM_SOCKET_MAX_NUM; x++)
			{
				str_to_number(addToList.alSockets[x], mRow[cur++]);
			}

			for (BYTE j = 0; j < MAX_ATTRIBUTE; j++)
			{
				str_to_number(addToList.aAttr[j].bType, mRow[cur++]);
				str_to_number(addToList.aAttr[j].sValue, mRow[cur++]);
			}

			str_to_number(addToList.timeSocket, mRow[cur++]);
			str_to_number(addToList.timeType, mRow[cur++]);

			TItemTable* iTable = NULL;
			if ((iTable = ITEM_MANAGER::Instance().GetTable(addToList.itemVnum)))
			{
				for (BYTE i = 0; i < OFFER_ITEM_GRID_MAX; i++)
				{
					if (offerGrid.IsEmpty(i, 1, iTable->bSize))
					{ 
						offerGrid.Put(i, 1, iTable->bSize); 

						addToList.itemPos = i;

						m_offerItemList.emplace_back(addToList);
						break;
					}
					else
					{
						sys_err("%d icin tabloda yer bulunamadi?", addToList.itemVnum);
					}
				}
			}
		}
	}

	return true;
}

void COfferSystem::RecvOfferPacket(LPCHARACTER ch, BYTE reqID, DWORD offerID, DWORD offerOldPrice, DWORD offerNewPrice)
{
	if (!ch || !ch->GetDesc() || !GetOfferStatus() || m_offerList.empty() || m_offerItemList.empty())
	{
		return;
	}

	if (quest::CQuestManager::instance().GetEventFlag("teklif_ac") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "1000");
		return;
	}

	if (reqID != 0)
	{ 
		BuyOffer(ch, offerID, offerOldPrice, offerNewPrice); 
		return;
	}

	TGCOfferListSize sendPacket;
	sendPacket.bHeader = HEADER_GC_OFFER;
	sendPacket.dataType = 0;	//offer list
	sendPacket.dataSize = sizeof(sendPacket) + sizeof(TOfferInfo) * m_offerList.size();

	ch->GetDesc()->BufferedPacket(&sendPacket, sizeof(sendPacket));
	ch->GetDesc()->Packet(&m_offerList[0], sizeof(TOfferInfo) * m_offerList.size());

	TGCOfferListSize sendPacketItems;
	sendPacketItems.bHeader = HEADER_GC_OFFER;
	sendPacketItems.dataType = 1;	//item list
	sendPacketItems.dataSize = sizeof(sendPacketItems) + sizeof(TOfferItemInfo) * m_offerItemList.size();

	ch->GetDesc()->BufferedPacket(&sendPacketItems, sizeof(sendPacketItems));
	ch->GetDesc()->Packet(&m_offerItemList[0], sizeof(TOfferItemInfo) * m_offerItemList.size());
}

void COfferSystem::BuyOffer(LPCHARACTER ch, DWORD offerID, DWORD offerOldPrice, DWORD offerNewPrice)
{
	if (!ch || !ch->GetDesc() || !GetOfferStatus() || m_offerList.empty() || m_offerItemList.empty())
	{
		return;
	}

	if (quest::CQuestManager::instance().GetEventFlag("teklif_ac") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "1000");
		return;
	}

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	if (!IsActiveOffer(offerID, offerOldPrice, offerNewPrice, ch))
	{ 
		ch->ChatPacket(CHAT_TYPE_INFO, "Nesne bulunamadi.");
		return;
	}

	if (ch->GetCoinNm() < offerNewPrice)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Yeterli EP yok.");
		return;
	}

	if (ch->UpdateCoin(ch->GetDesc()->GetAccountTable().id, -offerNewPrice))
	{
		ch->RefreshCoinAndCash();
	}

	for (itertype(m_offerItemList) it = m_offerItemList.begin(); it != m_offerItemList.end(); it++)
	{
		if (it->itemOfferID == offerID)
		{
			LPITEM item = ITEM_MANAGER::instance().CreateItem(it->itemVnum, it->itemCount);
			if (!item)
			{
				sys_err("cannot create item vnum %d", it->itemVnum);
				continue;
			}

			int iEmptyPos = ch->GetEmptyAllInventory(item);
			if(iEmptyPos < 0)
			{
				M2_DESTROY_ITEM(item);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("nesne_env_yer_yok"));
				continue;
			}

			item->SetSockets(it->alSockets);

			const bool hasTime = (it->timeType != 0);
			if (hasTime)
			{
				item->SetSocket(0, (get_global_time() + it->alSockets[0]));
			}

			for (int at = 0; at < ITEM_ATTRIBUTE_MAX_NUM; at++)
			{
				if (at < 7)
					item->SetForceAttribute(at, it->aAttr[at].bType, it->aAttr[at].sValue);
				else
					item->SetForceAttribute(at, 0, 0);
			}

			#ifdef ENABLE_YAYINCI_SISTEMI
			if (CYayinci::instance().GetYayinci(ch))
			{
				item->SetSealBind(3162);
			}
			#endif

			item->AddToItem(ch, iEmptyPos);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			auto sqlLog(DBManager::instance().DirectQuery("INSERT INTO server_settings.firsat_log(item_vnum, item_count, player_pid, player_name, item_price, buy_date) VALUES(%d, %d, %d, '%s', %d, NOW())", it->itemVnum, it->itemCount, ch->GetPlayerID(), ch->GetName(), offerNewPrice));
		}
	}
}

bool COfferSystem::IsActiveOffer(DWORD offerID, DWORD offerOldPrice, DWORD offerNewPrice, LPCHARACTER ch)
{
	if (!ch || !ch->GetDesc() || !GetOfferStatus() || m_offerList.empty() || m_offerItemList.empty())
	{
		return false;
	}

	if (quest::CQuestManager::instance().GetEventFlag("teklif_ac") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO,  "Teklif sistemi gecici olarak devredisi");
		return false;
	}

	auto it = std::find_if(m_offerList.begin(), m_offerList.end(), [&](const TOfferInfo & sR) { return (sR.offerID == offerID); });
	if (it == m_offerList.end())
		return false;

	TOfferInfo& OfferInfo = *it;

	if (OfferInfo.offerID == offerID)
	{
		if (OfferInfo.offerStartRemainSec - get_global_time() > 0 || OfferInfo.offerEndRemainSec <= get_global_time())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Teklif su anda aktif degil.");

			return false;
		}

		if (OfferInfo.offerNewPrice != offerNewPrice || OfferInfo.offerOldPrice != offerOldPrice)
		{
			return false;
		}
	}

	return true;
}

void COfferSystem::CheckFirsat(LPCHARACTER ch)
{
	if (!ch || !ch->GetDesc() || !GetOfferStatus() || m_offerList.empty() || m_offerItemList.empty())
	{
		return;
	}

	if (quest::CQuestManager::instance().GetEventFlag("teklif_ac") == 1)
	{
		return;
	}

    ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowOfferIcon");
}
#endif