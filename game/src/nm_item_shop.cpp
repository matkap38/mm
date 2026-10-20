#include "stdafx.h"

#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
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
#include "p2p.h"
#include "item.h"
#include "item_manager.h"
#include "nm_item_shop.h"
#include "questmanager.h"

using namespace std;
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#define ISHOP_MAIN_TABLE "server_settings"
#define ISHOP_ITEM_TABLE "nesnemarket_item"
#define ISHOP_CATE_TABLE "nesnemarket_category"

#define ISHOP_ITEM_ROAD "server_settings.nesnemarket_item"
#define ISHOP_CATE_ROAD "server_settings.nesnemarket_category"

#ifdef ENABLE_DRAGON_COIN_EM_MARKET
	#define ISHOP_EM_ITEM_TABLE "nesnemarket_em_item"
	#define ISHOP_EM_CATE_TABLE "nesnemarket_em_category"

	#define ISHOP_EM_ITEM_ROAD "server_settings.nesnemarket_em_item"
	#define ISHOP_EM_CATE_ROAD "server_settings.nesnemarket_em_category"
#endif

CDragonCoinShopManager::CDragonCoinShopManager()
{
	m_DcShopStatus = true;
}

CDragonCoinShopManager::~CDragonCoinShopManager()
{
	m_DcShopStatus = true;
	Destroy();
}

void CDragonCoinShopManager::Destroy()
{
	m_DcShopItemVec.clear();
	m_DcShopCateVec.clear();
	m_DcShopStatVec.clear();
	m_mapDcShopBuyLog.clear();
	#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
	m_DcShopItemAdminVec.clear();
	#endif
	#ifdef ENABLE_DRAGON_COIN_EM_MARKET
	m_DcShopEmItemVec.clear();
	m_DcShopEmCateVec.clear();
	m_DcShopEmStatVec.clear();
	#endif
}

void CDragonCoinShopManager::Install()
{
	LoadDcShopItemList();
	LoadDcShopCateList();
	LoadDcShopLogList();
	#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
	LoadDcShopItemListAdmin();
	#endif
	#ifdef ENABLE_DRAGON_COIN_EM_MARKET
	LoadDcShopEmItemList();
	LoadDcShopEmCateList();
	#endif
}

void CDragonCoinShopManager::ReInstall()
{
	SetDcShopStatus(false);
	Destroy();
	Install();
	SetDcShopStatus(true);
}

bool CDragonCoinShopManager::LoadDcShopItemList()
{
	m_DcShopItemVec.clear();
	m_DcShopStatVec.clear();

	sys_err("Load data...");

	char szQuery[2048];
	snprintf(szQuery, sizeof(szQuery), 
		"SELECT id, vnum, count, price, discount, category_id"
		", time_type"
		", stock"
		", sell_count"
		", sell_type+0"
		", UNIX_TIMESTAMP(add_time)"
		", UNIX_TIMESTAMP(offer_time)"
		", socket0"
		", socket1"
		", socket2"
		", socket3"
		", attrtype0, attrvalue0"
		", attrtype1, attrvalue1"
		", attrtype2, attrvalue2"
		", attrtype3, attrvalue3"
		", attrtype4, attrvalue4"
		", attrtype5, attrvalue5"
		", attrtype6, attrvalue6"
		" FROM %s WHERE %s.status = 1;"
		, ISHOP_ITEM_ROAD
		, ISHOP_ITEM_TABLE
	);

	auto pMsg_Item(DBManager::Instance().DirectQuery(szQuery));
	if (pMsg_Item->Get()->uiNumRows == 0)
	{
		sys_err("pMsg NULL!");
		return false;
	}

	MYSQL_ROW mRow;
	while (NULL != (mRow = mysql_fetch_row(pMsg_Item->Get()->pSQLResult)))
	{
		BYTE iCur = 0;

		TGCIShopItem isItem;
		str_to_number(isItem.iID, mRow[iCur++]);
		str_to_number(isItem.iVnum, mRow[iCur++]);
		str_to_number(isItem.iCount, mRow[iCur++]);
		str_to_number(isItem.iPrice, mRow[iCur++]);
		str_to_number(isItem.iDiscount, mRow[iCur++]);
		str_to_number(isItem.categoryID, mRow[iCur++]);
		str_to_number(isItem.bTimeType, mRow[iCur++]);
		str_to_number(isItem.iStock, mRow[iCur++]);
		str_to_number(isItem.wSellCount, mRow[iCur++]);
		isItem.bSellType = atoi(mRow[iCur++])-1;

		//---------------------------------------------//
		int iAddTime = 0;
		int iOfferTime = 0;
		str_to_number(iAddTime, mRow[iCur++]);
		str_to_number(iOfferTime, mRow[iCur++]);
		isItem.dwAddTime = DWORD((iAddTime > 0) ? iAddTime : 0);
		isItem.dwOfferTime = DWORD((iOfferTime > 0) ? iOfferTime : 0);
		//---------------------------------------------//

		for (BYTE i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		{
			str_to_number(isItem.alSockets[i], mRow[iCur++]);
		}

		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++)
		{
			str_to_number(isItem.aAttr[j].bType, mRow[iCur++]);
			str_to_number(isItem.aAttr[j].sValue, mRow[iCur++]);
		}

		m_DcShopItemVec.emplace_back(isItem);
	}

	for (auto it : m_DcShopItemVec)
	{
		TIShopStatus isTable;
		isTable.iID = it.iID;
		isTable.isSocketStatus = false;
		isTable.isAttrStatus = false;

		for (BYTE i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		{
			if (it.alSockets[i] == 0)
				continue;

			isTable.isSocketStatus = true;
		}

		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++)
		{
			if (it.aAttr[j].bType == 0 && it.aAttr[j].sValue == 0)
				continue;

			isTable.isAttrStatus = true;
		}

		m_DcShopStatVec.emplace_back(isTable);
	}

	return true;
}

bool CDragonCoinShopManager::LoadDcShopCateList()
{
	m_DcShopCateVec.clear();

	char szQuery[2048];
	snprintf(szQuery, sizeof(szQuery), "SELECT * FROM %s ORDER BY cat_main_id DESC, cat_id ASC;", ISHOP_CATE_ROAD);

	auto pMsg(DBManager::Instance().DirectQuery(szQuery));
	if (pMsg->Get()->uiNumRows == 0)
	{
		return false;
	}

	MYSQL_ROW mRow;
	while (NULL != (mRow = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		BYTE iCur = 0;

		TGCIShopCategory isCat;
		str_to_number(isCat.catID, mRow[iCur++]);
		strlcpy(isCat.categoryName, mRow[iCur++], sizeof(isCat.categoryName));
		str_to_number(isCat.mainID, mRow[iCur++]);

		m_DcShopCateVec.emplace_back(isCat);
	}

	std::sort(m_DcShopCateVec.begin(), m_DcShopCateVec.end(), [](const TGCIShopCategory T1, const TGCIShopCategory T2){ return T1.catID < T2.catID; });
	return true;
}

bool CDragonCoinShopManager::LoadDcShopLogList()
{
	char szQuery[2048];
	snprintf(szQuery, sizeof(szQuery), "SELECT accountID, playerName, buyDate, buyTime, ipAdress, itemVnum, itemCount, itemPrice, itemPriceType FROM log.itemshop_log;");

	auto pMsgLog(DBManager::Instance().DirectQuery(szQuery));
	if (pMsgLog->Get()->uiNumRows == 0) { return false; }

	MYSQL_ROW row;
	while (NULL != (row = mysql_fetch_row(pMsgLog->Get()->pSQLResult)))
	{
		int col = 0; DWORD dwAID = 0;

		TDcShopLog ishopData;
		str_to_number(dwAID, row[col++]);
		ishopData.accountID = dwAID;
		strlcpy(ishopData.playerName, row[col++], sizeof(ishopData.playerName));
		strlcpy(ishopData.buyDate, row[col++], sizeof(ishopData.buyDate));
		str_to_number(ishopData.buyTime, row[col++]);
		strlcpy(ishopData.ipAdress, row[col++], sizeof(ishopData.ipAdress));
		str_to_number(ishopData.itemVnum, row[col++]);
		str_to_number(ishopData.itemCount, row[col++]);
		str_to_number(ishopData.itemPrice, row[col++]);
		str_to_number(ishopData.itemPriceType, row[col++]);

		auto iter = m_mapDcShopBuyLog.find(dwAID);
		if (iter == m_mapDcShopBuyLog.end())
		{
			std::vector<TDcShopLog> mVec;
			mVec.emplace_back(ishopData);
			m_mapDcShopBuyLog.emplace(dwAID, mVec);
		}
		else
		{
			iter->second.emplace_back(ishopData);
		}
	}

	return true;
}

void CDragonCoinShopManager::SendPopupMessage(LPCHARACTER ch, const std::string& message, int iData)
{
	if (!GetDcShopStatus() || !ch || !ch->GetDesc())
		return;

	TItemShopPopupMessage subpack{};
	str_to_cstring(subpack.localeString, message.data());
	subpack.iData = iData;

	TGCItemShopSize pack{};
	pack.bHeader = HEADER_GC_ITEMSHOP_GENERAL;
	pack.dataSize = sizeof(pack) + sizeof(subpack);
	pack.bSubHeader = DC_SUB_HEADER_POPUP_MESSAGE;

	ch->GetDesc()->BufferedPacket(pack);
	ch->GetDesc()->Packet(subpack);
}

void CDragonCoinShopManager::OpenItemShop(LPCHARACTER ch)
{
	if (!GetDcShopStatus() || !ch || !ch->GetDesc())
		return;

	if (m_DcShopItemVec.empty())
	{
		sys_err("m_DcShopItemVec is empty!");
		return;
	}

	TGCItemShopSize gPacket;
	gPacket.bHeader = HEADER_GC_ITEMSHOP_GENERAL;
	gPacket.dataSize = sizeof(gPacket) + sizeof(TGCIShopItem) * m_DcShopItemVec.size();
	gPacket.bSubHeader = DC_SUB_HEADER_ITEMS;

	ch->GetDesc()->BufferedPacket(&gPacket, sizeof(gPacket));
	ch->GetDesc()->Packet(&m_DcShopItemVec[0], sizeof(TGCIShopItem) * m_DcShopItemVec.size());
}

void CDragonCoinShopManager::SendCategoryItemShop(LPCHARACTER ch)
{
	if (!GetDcShopStatus())
		return;

	if (!ch || !ch->GetDesc())
		return;

	if (m_DcShopCateVec.empty())
	{
		sys_err("m_DcShopCateVec is empty!");
		return;
	}

	TGCItemShopSize gPacket;
	gPacket.bHeader = HEADER_GC_ITEMSHOP_GENERAL;
	gPacket.dataSize = sizeof(gPacket) + sizeof(TGCIShopCategory) * m_DcShopCateVec.size();
	gPacket.bSubHeader = DC_SUB_HEADER_CATEGORYS;

	ch->GetDesc()->BufferedPacket(&gPacket, sizeof(gPacket));
	ch->GetDesc()->Packet(&m_DcShopCateVec[0], sizeof(TGCIShopCategory) * m_DcShopCateVec.size());

	ch->RefreshCoinAndCash();
}

void CDragonCoinShopManager::BuyItemShop(LPCHARACTER ch, DWORD dwItemID, BYTE bItemCount, BYTE bItemPriceType)
{
	if ((bItemCount <= 0) || (bItemCount > 50)) { return; }
	if (!GetDcShopStatus() || !ch || !ch->GetDesc() || m_DcShopItemVec.empty()) { return; }

	if (ch->PreventTradeWindow(WND_ALL))
	{
		SendPopupMessage(ch, "ISHOP_CLOSE_THE_WINDOWS");
		return;
	}

	auto it = std::find_if (m_DcShopItemVec.begin(), m_DcShopItemVec.end(), [&](const TGCIShopItem & sR) { return (sR.iID == dwItemID); });
	if (it == m_DcShopItemVec.end()) { return; }

	TGCIShopItem& itemInfo = *it;

	if ((bItemPriceType != ITEMSHOP_SELL_TYPE_CASH) && (itemInfo.bSellType == ITEMSHOP_SELL_TYPE_CASH))
	{
		SendPopupMessage(ch, "ISHOP_CANNOT_COINS_BUY_ITEM");
		return;
	}
	else if ((bItemPriceType != ITEMSHOP_SELL_TYPE_COINS) && (itemInfo.bSellType == ITEMSHOP_SELL_TYPE_COINS))
	{
		SendPopupMessage(ch, "ISHOP_CANNOT_CASH_BUY_ITEM");
		return;
	}

	//####################//
	const DWORD itemVnum = itemInfo.iVnum;
	const DWORD itemCount = itemInfo.iCount;
	DWORD itemPrice = itemInfo.iPrice;
	const DWORD iDiscount = itemInfo.iDiscount;
	const BYTE timeType = itemInfo.bTimeType;
	int iOfferTime = itemInfo.dwOfferTime;
	bool isSocketStatus = true;
	bool isAttrStatus = true;
	//####################//

	if (iOfferTime > 0)
	{
		if (get_global_time() > time_t(iOfferTime))
		{
			SendPopupMessage(ch, "ISHOP_TIMES_IS_UP");
			return;
		}
	}

	DWORD dwGetCoins = (itemPrice * bItemCount);

	if (iDiscount > 0)
	{
		dwGetCoins = DWORD((float(dwGetCoins) / 100.0) * float(100 - iDiscount));
	}

	if (bItemPriceType == ITEMSHOP_SELL_TYPE_COINS)
	{
		if (ch->GetCoinNm() < dwGetCoins)
		{
			SendPopupMessage(ch, "ISHOP_NOT_ENOUGH_EP");
			return;
		}
	}
	else if (bItemPriceType == ITEMSHOP_SELL_TYPE_CASH)
	{
		if (ch->GetCashNm() < dwGetCoins)
		{
			SendPopupMessage(ch, "ISHOP_NOT_ENOUGH_MP");
			return;
		}
	}
	else
	{
		return;
	}

	const TItemTable* pTable = ITEM_MANAGER::instance().GetTable(itemVnum);
	if (!pTable)
	{
		SendPopupMessage(ch, "ISHOP_NOT_IS_ITEM");
		return;
	}

	const DWORD dwRealCount = (itemCount * bItemCount);
	const int iStock = itemInfo.iStock;

	if (iStock > -1)
	{
		if ((int)(dwRealCount) > iStock)
		{
			SendPopupMessage(ch, "ISHOP_NOT_ENOUGH_STOCK", iStock);
			ch->ChatPacket(CHAT_TYPE_COMMAND, "DcItemShopUpdateItem %d %d", itemInfo.iID, iStock);
			return;
		}
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(itemVnum, dwRealCount, 0, true, -1);
	if (!item) { return; }

	int iEmptyPos = ch->GetEmptyAllInventory(item);
	if (iEmptyPos < 0)
	{
		M2_DESTROY_ITEM(item);
		SendPopupMessage(ch, "ISHOP_NOT_ENOUGH_INV");
		return;
	}

	if (bItemPriceType == ITEMSHOP_SELL_TYPE_COINS)
	{
		if (ch->UpdateCoin(ch->GetAID(), -dwGetCoins))
		{
			ch->RefreshCoinAndCash();
		}
		else
		{
			M2_DESTROY_ITEM(item);
			return;
		}
	}
	else if (bItemPriceType == ITEMSHOP_SELL_TYPE_CASH)
	{
		if (ch->UpdateCash(ch->GetAID(), -dwGetCoins))
		{
			ch->RefreshCoinAndCash();
		}
		else
		{
			M2_DESTROY_ITEM(item);
			return;
		}

		#ifdef ENABLE_LOCKED_SYSTEM
		item->SetLocked(LOCKED_ISHOP_MP);
		#endif
	}
	else
	{
		return;
	}

	if (!m_DcShopStatVec.empty())
	{
		auto it2 = std::find_if (m_DcShopStatVec.begin(), m_DcShopStatVec.end(), [&](const TIShopStatus & sR) { return (sR.iID == dwItemID); });
		if (it2 != m_DcShopStatVec.end())
		{
			TIShopStatus& itemStat = *it2;
			isSocketStatus = itemStat.isSocketStatus;
			isAttrStatus = itemStat.isAttrStatus;
		}
	}

	if (isSocketStatus)
	{
		item->SetSockets(itemInfo.alSockets);

		const bool hasTime = (timeType != 0);
		if (hasTime)
		{
			item->SetSocket(0, (get_global_time()+itemInfo.alSockets[0]));
		}
	}

	if (isAttrStatus)
	{
		item->SetAttributes(itemInfo.aAttr);
	}

	item->AddToItem(ch, iEmptyPos);
	ITEM_MANAGER::instance().FlushDelayedSave(item);

	DBManager::instance().DirectQuery("UPDATE %s SET sell_count = sell_count + 1 WHERE id = '%u';", ISHOP_ITEM_ROAD, dwItemID);
	itemInfo.wSellCount += 1;

	if (itemInfo.iStock > -1)
	{
		itemInfo.iStock -= dwRealCount;
		DBManager::instance().DirectQuery("UPDATE %s SET stock = stock - '%u' WHERE id = '%u';", ISHOP_ITEM_ROAD, dwRealCount, dwItemID);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "DcItemShopUpdateItem %d %d", itemInfo.iID, itemInfo.iStock);
	}

	SetBuyLog(ch, itemVnum, bItemCount, dwGetCoins, bItemPriceType);

	SendPopupMessage(ch, "ISHOP_SHOPPING_SUCCESSFUL");

	#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
	if (!m_DcShopItemAdminVec.empty())
	{
		auto it3 = std::find_if (m_DcShopItemAdminVec.begin(), m_DcShopItemAdminVec.end(), [&](const TGCIShopItemAdmin & sR) { return (sR.iID == dwItemID); });
		if (it3 != m_DcShopItemAdminVec.end())
		{
			TGCIShopItemAdmin& itemAdminInfo = *it3;
			itemAdminInfo.wSellCount = itemInfo.wSellCount;
			itemAdminInfo.iStock = itemInfo.iStock;
		}
	}
	#endif
}

void CDragonCoinShopManager::SetBuyLog(LPCHARACTER ch, DWORD itemVnum, BYTE itemCount, DWORD itemCoins, BYTE itemPriceType)
{
	if (!ch || !ch->GetDesc())
		return;

	char ipAdress[16];
	strlcpy(ipAdress, ch->GetDesc()->GetHostName(), sizeof(ipAdress));

	DWORD dwAID = ch->GetAID();

	DBManager::instance().DirectQuery("INSERT INTO log.itemshop_log (accountID, playerID, playerName, buyDate, buyTime, ipAdress, itemVnum, itemCount, itemPrice, itemPriceType) VALUES(%u, %u, '%s', NOW(), %d, '%s', %u, %u, %u, %u)", dwAID, ch->GetPlayerID(), ch->GetName(), time(0), ipAdress, itemVnum, itemCount, itemCoins, itemPriceType);

	char       timeText[21];
	time_t     now = time(0);
	struct tm  tstruct = *localtime(&now);
	strftime(timeText, sizeof(timeText), "%Y-%m-%d %X", &tstruct);

	TDcShopLog logData;
	logData.accountID = dwAID;
	strlcpy(logData.playerName, ch->GetName(), sizeof(logData.playerName));
	strlcpy(logData.buyDate, timeText, sizeof(logData.buyDate));
	logData.buyTime = time(0);
	strlcpy(logData.ipAdress, ipAdress, sizeof(logData.ipAdress));
	logData.itemVnum = itemVnum;
	logData.itemCount = itemCount;
	logData.itemPrice = itemCoins;
	logData.itemPriceType = itemPriceType;

	auto iter = m_mapDcShopBuyLog.find(dwAID);
	if (iter == m_mapDcShopBuyLog.end())
	{
		std::vector<TDcShopLog> mVec;
		mVec.emplace_back(logData);
		m_mapDcShopBuyLog.emplace(dwAID, mVec);
	}
	else
	{
		iter->second.emplace_back(logData);
	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "ItemShopLogRefresh");

	TPacketGGDcShopBuy pack;
	pack.bHeader = HEADER_GG_DC_SHOP_BUY;
	pack.dwAID = dwAID;

	TEMP_BUFFER buf;
	buf.write(&pack, sizeof(TPacketGGDcShopBuy));
	buf.write(&logData, sizeof(TDcShopLog));
	P2P_MANAGER::instance().Send(buf.read_peek(), buf.size());
}

void CDragonCoinShopManager::SetBuyLogP2p(DWORD dwAID, TDcShopLog * table)
{
	if (dwAID <= 0)
		return;

	TDcShopLog logData;
	thecore_memcpy(&logData, table, sizeof(logData));

	auto iter = m_mapDcShopBuyLog.find(dwAID);
	if (iter == m_mapDcShopBuyLog.end())
	{
		std::vector<TDcShopLog> mVec;
		mVec.emplace_back(logData);
		m_mapDcShopBuyLog.emplace(dwAID, mVec);
	}
	else
	{
		iter->second.emplace_back(logData);
	}
}

void CDragonCoinShopManager::OpenLog(LPCHARACTER ch)
{
	if (!ch || !ch->GetDesc())
		return;

	DWORD dwAID = ch->GetAID();

	auto it = m_mapDcShopBuyLog.find(dwAID);
	if (it == m_mapDcShopBuyLog.end()) { return; }

	auto mVector = it->second;
	if (mVector.empty()) { return; }

	TGCItemShopSize gPacket;
	gPacket.bHeader = HEADER_GC_ITEMSHOP_GENERAL;
	gPacket.dataSize = sizeof(gPacket) + sizeof(TDcShopLog) * mVector.size();
	gPacket.bSubHeader = DC_SUB_HEADER_LOGS;

	ch->GetDesc()->BufferedPacket(&gPacket, sizeof(gPacket));
	ch->GetDesc()->Packet(&mVector[0], sizeof(TDcShopLog) * mVector.size());
}

#ifdef ENABLE_DRAGON_COIN_EM_MARKET
bool CDragonCoinShopManager::LoadDcShopEmItemList()
{
	m_DcShopEmItemVec.clear();
	m_DcShopEmStatVec.clear();

	sys_err("Load data...");

	char szQuery[2048];
	snprintf(szQuery, sizeof(szQuery), 
		"SELECT id, vnum, count, price, discount, category_id"
		", time_type"
		", stock"
		", sell_count"
		", UNIX_TIMESTAMP(add_time)"
		", UNIX_TIMESTAMP(offer_time)"
		", socket0"
		", socket1"
		", socket2"
		", socket3"
		", attrtype0, attrvalue0"
		", attrtype1, attrvalue1"
		", attrtype2, attrvalue2"
		", attrtype3, attrvalue3"
		", attrtype4, attrvalue4"
		", attrtype5, attrvalue5"
		", attrtype6, attrvalue6"
		" FROM %s WHERE %s.status = 1;"
		, ISHOP_EM_ITEM_ROAD
		, ISHOP_EM_ITEM_TABLE
	);

	auto pMsg_Item(DBManager::Instance().DirectQuery(szQuery));
	if (pMsg_Item->Get()->uiNumRows == 0)
	{
		sys_err("pMsg NULL!");
		return false;
	}

	MYSQL_ROW mRow;
	while (NULL != (mRow = mysql_fetch_row(pMsg_Item->Get()->pSQLResult)))
	{
		BYTE iCur = 0;

		TGCIShopItem isItem;
		str_to_number(isItem.iID, mRow[iCur++]);
		str_to_number(isItem.iVnum, mRow[iCur++]);
		str_to_number(isItem.iCount, mRow[iCur++]);
		str_to_number(isItem.iPrice, mRow[iCur++]);
		str_to_number(isItem.iDiscount, mRow[iCur++]);
		str_to_number(isItem.categoryID, mRow[iCur++]);
		str_to_number(isItem.bTimeType, mRow[iCur++]);
		str_to_number(isItem.iStock, mRow[iCur++]);
		str_to_number(isItem.wSellCount, mRow[iCur++]);
		isItem.bSellType = ITEMSHOP_SELL_TYPE_EM;

		//---------------------------------------------//
		int iAddTime = 0;
		int iOfferTime = 0;
		str_to_number(iAddTime, mRow[iCur++]);
		str_to_number(iOfferTime, mRow[iCur++]);
		isItem.dwAddTime = DWORD((iAddTime > 0) ? iAddTime : 0);
		isItem.dwOfferTime = DWORD((iOfferTime > 0) ? iOfferTime : 0);
		//---------------------------------------------//

		for (BYTE i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		{
			str_to_number(isItem.alSockets[i], mRow[iCur++]);
		}

		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++)
		{
			str_to_number(isItem.aAttr[j].bType, mRow[iCur++]);
			str_to_number(isItem.aAttr[j].sValue, mRow[iCur++]);
		}

		m_DcShopEmItemVec.emplace_back(isItem);
	}

	for (auto it : m_DcShopEmItemVec)
	{
		TIShopStatus isTable;
		isTable.iID = it.iID;
		isTable.isSocketStatus = false;
		isTable.isAttrStatus = false;

		for (BYTE i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		{
			if (it.alSockets[i] == 0)
				continue;

			isTable.isSocketStatus = true;
		}

		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++)
		{
			if (it.aAttr[j].bType == 0 && it.aAttr[j].sValue == 0)
				continue;

			isTable.isAttrStatus = true;
		}

		m_DcShopEmStatVec.emplace_back(isTable);
	}

	return true;
}

bool CDragonCoinShopManager::LoadDcShopEmCateList()
{
	m_DcShopEmCateVec.clear();

	char szQuery[2048];
	snprintf(szQuery, sizeof(szQuery), "SELECT * FROM %s ORDER BY cat_main_id DESC, cat_id ASC;", ISHOP_EM_CATE_ROAD);
	auto pMsg(DBManager::Instance().DirectQuery(szQuery));
	if (pMsg->Get()->uiNumRows == 0)
	{
		return false;
	}

	MYSQL_ROW mRow;
	while (NULL != (mRow = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		BYTE iCur = 0;

		TGCIShopCategory isCat;
		str_to_number(isCat.catID, mRow[iCur++]);
		strlcpy(isCat.categoryName, mRow[iCur++], sizeof(isCat.categoryName));
		str_to_number(isCat.mainID, mRow[iCur++]);

		m_DcShopEmCateVec.emplace_back(isCat);
	}

	std::sort(m_DcShopEmCateVec.begin(), m_DcShopEmCateVec.end(), [](const TGCIShopCategory T1, const TGCIShopCategory T2){ return T1.catID < T2.catID; });
	return true;
}

void CDragonCoinShopManager::OpenEmItemShop(LPCHARACTER ch)
{
	if (!GetDcShopStatus() || !ch || !ch->GetDesc())
		return;

	if (m_DcShopEmItemVec.empty())
	{
		sys_err("m_DcShopEmItemVec is empty!");
		return;
	}

	TGCItemShopSize gPacket;
	gPacket.bHeader = HEADER_GC_ITEMSHOP_GENERAL;
	gPacket.dataSize = sizeof(gPacket) + sizeof(TGCIShopItem) * m_DcShopEmItemVec.size();
	gPacket.bSubHeader = DC_SUB_HEADER_EM_ITEMS;

	ch->GetDesc()->BufferedPacket(&gPacket, sizeof(gPacket));
	ch->GetDesc()->Packet(&m_DcShopEmItemVec[0], sizeof(TGCIShopItem) * m_DcShopEmItemVec.size());
}

void CDragonCoinShopManager::SendCategoryEmItemShop(LPCHARACTER ch)
{
	if (!GetDcShopStatus())
		return;

	if (!ch || !ch->GetDesc())
		return;

	if (m_DcShopEmCateVec.empty())
	{
		sys_err("m_DcShopEmCateVec is empty!");
		return;
	}

	TGCItemShopSize gPacket;
	gPacket.bHeader = HEADER_GC_ITEMSHOP_GENERAL;
	gPacket.dataSize = sizeof(gPacket) + sizeof(TGCIShopCategory) * m_DcShopEmCateVec.size();
	gPacket.bSubHeader = DC_SUB_HEADER_EM_CATEGORYS;

	ch->GetDesc()->BufferedPacket(&gPacket, sizeof(gPacket));
	ch->GetDesc()->Packet(&m_DcShopEmCateVec[0], sizeof(TGCIShopCategory) * m_DcShopEmCateVec.size());

	ch->RefreshCoinAndCash();
}

void CDragonCoinShopManager::BuyEmItemShop(LPCHARACTER ch, DWORD dwItemID, BYTE bItemCount)
{
	if ((bItemCount <= 0) || (bItemCount > 50)) { return; }
	if (!GetDcShopStatus() || !ch || !ch->GetDesc() || m_DcShopEmItemVec.empty()) { return; }

	if (ch->PreventTradeWindow(WND_ALL))
	{
		SendPopupMessage(ch, "ISHOP_CLOSE_THE_WINDOWS");
		return;
	}

	auto it = std::find_if (m_DcShopEmItemVec.begin(), m_DcShopEmItemVec.end(), [&](const TGCIShopItem & sR) { return (sR.iID == dwItemID); });
	if (it == m_DcShopEmItemVec.end())
		return;

	TGCIShopItem& itemInfo = *it;

	if (itemInfo.bSellType != ITEMSHOP_SELL_TYPE_EM)
	{
		SendPopupMessage(ch, "ISHOP_CANNOT_EM_BUY_ITEM");
		return;
	}

	//####################//
	const DWORD itemVnum = itemInfo.iVnum;
	const DWORD itemCount = itemInfo.iCount;
	DWORD itemPrice = itemInfo.iPrice;
	const DWORD iDiscount = itemInfo.iDiscount;
	const BYTE timeType = itemInfo.bTimeType;
	int iOfferTime = itemInfo.dwOfferTime;
	bool isSocketStatus = true;
	bool isAttrStatus = true;
	//####################//

	if (iOfferTime > 0)
	{
		if (get_global_time() > time_t(iOfferTime))
		{
			SendPopupMessage(ch, "ISHOP_TIMES_IS_UP");
			return;
		}
	}

	DWORD dwGetCoins = (itemPrice * bItemCount);

	if (iDiscount > 0)
	{
		dwGetCoins = DWORD((float(dwGetCoins) / 100.0) * float(100 - iDiscount));
	}

	if (ch->GetEmNm() < dwGetCoins)
	{
		SendPopupMessage(ch, "ISHOP_NOT_ENOUGH_EM");
		return;
	}

	const TItemTable* pTable = ITEM_MANAGER::instance().GetTable(itemVnum);
	if (!pTable)
	{
		SendPopupMessage(ch, "ISHOP_NOT_IS_ITEM");
		return;
	}
 
	const DWORD dwRealCount = (itemCount * bItemCount);
	const int iStock = itemInfo.iStock;

	if (iStock > -1)
	{
		if ((int)(dwRealCount) > iStock)
		{
			SendPopupMessage(ch, "ISHOP_NOT_ENOUGH_STOCK", iStock);
			ch->ChatPacket(CHAT_TYPE_COMMAND, "DcItemShopUpdateItem %d %d", itemInfo.iID, iStock);
			return;
		}
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(itemVnum, dwRealCount, 0, true, -1);
	if (!item)
		return;

	int iEmptyPos = ch->GetEmptyAllInventory(item);
	if (iEmptyPos < 0)
	{
		M2_DESTROY_ITEM(item);
		SendPopupMessage(ch, "ISHOP_NOT_ENOUGH_INV");
		return;
	}

	if (ch->UpdateEm(ch->GetAID(), -dwGetCoins))
	{
		ch->RefreshCoinAndCash();
	}
	else
	{
		M2_DESTROY_ITEM(item);
		return;
	}

	if (!m_DcShopEmStatVec.empty())
	{
		auto it2 = std::find_if (m_DcShopEmStatVec.begin(), m_DcShopEmStatVec.end(), [&](const TIShopStatus & sR) { return (sR.iID == dwItemID); });
		if (it2 != m_DcShopEmStatVec.end())
		{
			TIShopStatus& itemStat = *it2;
			isSocketStatus = itemStat.isSocketStatus;
			isAttrStatus = itemStat.isAttrStatus;
		}
	}

	if (isSocketStatus)
	{
		item->SetSockets(itemInfo.alSockets);

		const bool hasTime = (timeType != 0);
		if (hasTime)
		{
			item->SetSocket(0, (get_global_time()+itemInfo.alSockets[0]));
		}
	}

	if (isAttrStatus)
	{
		item->SetAttributes(itemInfo.aAttr);
	}

	#ifdef ENABLE_LOCKED_SYSTEM
	item->SetLocked(LOCKED_ISHOP_MP);
	#endif

	item->AddToItem(ch, iEmptyPos);
	ITEM_MANAGER::instance().FlushDelayedSave(item);

	DBManager::instance().DirectQuery("UPDATE %s SET sell_count = sell_count + 1 WHERE id = '%u';", ISHOP_EM_ITEM_ROAD, dwItemID);
	itemInfo.wSellCount += 1;

	if (itemInfo.iStock > -1)
	{
		itemInfo.iStock -= dwRealCount;
		DBManager::instance().DirectQuery("UPDATE %s SET stock = stock - '%u' WHERE id = '%u';", ISHOP_EM_ITEM_ROAD, dwRealCount, dwItemID);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "DcItemShopUpdateItem %d %d", itemInfo.iID, itemInfo.iStock);
	}

	SetBuyLog(ch, itemVnum, bItemCount, dwGetCoins, ITEMSHOP_SELL_TYPE_EM);

	SendPopupMessage(ch, "ISHOP_SHOPPING_SUCCESSFUL");
}
#endif

#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
bool CDragonCoinShopManager::LoadDcShopItemListAdmin()
{
	m_DcShopItemAdminVec.clear();

	char szQuery[2048];
	snprintf(szQuery, sizeof(szQuery), 
		"SELECT id, vnum, count, price, discount, category_id"
		", time_type"
		", status"
		", stock"
		", sell_count"
		", sell_type+0"
		", UNIX_TIMESTAMP(add_time)"
		", UNIX_TIMESTAMP(offer_time)"
		", add_time"
		", offer_time"
		", socket0"
		", socket1"
		", socket2"
		", socket3"
		", attrtype0, attrvalue0"
		", attrtype1, attrvalue1"
		", attrtype2, attrvalue2"
		", attrtype3, attrvalue3"
		", attrtype4, attrvalue4"
		", attrtype5, attrvalue5"
		", attrtype6, attrvalue6"
		" FROM %s;"
		, ISHOP_ITEM_ROAD
	);

	auto pMsg_Item(DBManager::Instance().DirectQuery(szQuery));
	if (pMsg_Item->Get()->uiNumRows == 0)
	{
		return false;
	}

	MYSQL_ROW mRow;
	while (NULL != (mRow = mysql_fetch_row(pMsg_Item->Get()->pSQLResult)))
	{
		BYTE iCur = 0;

		TGCIShopItemAdmin isItem;
		str_to_number(isItem.iID, mRow[iCur++]);
		str_to_number(isItem.iVnum, mRow[iCur++]);
		str_to_number(isItem.iCount, mRow[iCur++]);
		str_to_number(isItem.iPrice, mRow[iCur++]);
		str_to_number(isItem.iDiscount, mRow[iCur++]);
		str_to_number(isItem.categoryID, mRow[iCur++]);
		str_to_number(isItem.bTimeType, mRow[iCur++]);
		str_to_number(isItem.bStatus, mRow[iCur++]);
		str_to_number(isItem.iStock, mRow[iCur++]);
		str_to_number(isItem.wSellCount, mRow[iCur++]);
		isItem.bSellType = atoi(mRow[iCur++])-1;

		//---------------------------------------------//
		int iAddTime = 0;
		int iOfferTime = 0;
		str_to_number(iAddTime, mRow[iCur++]);
		str_to_number(iOfferTime, mRow[iCur++]);
		isItem.dwAddTime = DWORD((iAddTime > 0) ? iAddTime : 0);
		isItem.dwOfferTime = DWORD((iOfferTime > 0) ? iOfferTime : 0);
		//---------------------------------------------//
		strlcpy(isItem.szAddTime, mRow[iCur++], sizeof(isItem.szAddTime));
		strlcpy(isItem.szOfferTime, mRow[iCur++], sizeof(isItem.szOfferTime));
		//---------------------------------------------//

		for (BYTE i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		{
			str_to_number(isItem.alSockets[i], mRow[iCur++]);
		}

		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++)
		{
			str_to_number(isItem.aAttr[j].bType, mRow[iCur++]);
			str_to_number(isItem.aAttr[j].sValue, mRow[iCur++]);
		}

		m_DcShopItemAdminVec.emplace_back(isItem);
	}

	return true;
}

void CDragonCoinShopManager::SendCategoryItemShopAdmin(LPCHARACTER ch)
{
	if (!GetDcShopStatus())
		return;

	if (!ch || !ch->GetDesc())
		return;

	if (m_DcShopCateVec.empty())
		return;

	TGCItemShopSize gPacket;
	gPacket.bHeader = HEADER_GC_ITEMSHOP_ADMIN_GENERAL;
	gPacket.dataSize = sizeof(gPacket) + sizeof(TGCIShopCategory) * m_DcShopCateVec.size();
	gPacket.bSubHeader = DC_SUB_HEADER_ADMIN_CATEGORYS;

	ch->GetDesc()->BufferedPacket(&gPacket, sizeof(gPacket));
	ch->GetDesc()->Packet(&m_DcShopCateVec[0], sizeof(TGCIShopCategory) * m_DcShopCateVec.size());

	ch->RefreshCoinAndCash();
}

void CDragonCoinShopManager::OpenItemShopAdmin(LPCHARACTER ch)
{
	if (!GetDcShopStatus())
		return;

	if (!ch || !ch->GetDesc())
		return;

	if (m_DcShopItemAdminVec.empty())
		return;

	TGCItemShopSize gPacket;
	gPacket.bHeader = HEADER_GC_ITEMSHOP_ADMIN_GENERAL;
	gPacket.dataSize = sizeof(gPacket) + sizeof(TGCIShopItemAdmin) * m_DcShopItemAdminVec.size();
	gPacket.bSubHeader = DC_SUB_HEADER_ADMIN_ITEMS;

	ch->GetDesc()->BufferedPacket(&gPacket, sizeof(gPacket));
	ch->GetDesc()->Packet(&m_DcShopItemAdminVec[0], sizeof(TGCIShopItemAdmin) * m_DcShopItemAdminVec.size());
}

void CDragonCoinShopManager::AddCateItemShopAdmin(const char * c_pszCateName)
{
	char szCateName[32+1];
	DBManager::instance().EscapeString(szCateName, sizeof(szCateName), c_pszCateName, strlen(c_pszCateName));

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "INSERT INTO %s(cat_name, cat_main_id) VALUES ('%s', '255');", ISHOP_CATE_ROAD, szCateName);
	DBManager::instance().DirectQuery(szQuery);

	ReInstall();
}

void CDragonCoinShopManager::AddSubCateItemShopAdmin(int iCateID, const char * c_pszCateName)
{
	char szCateName[32+1];
	DBManager::instance().EscapeString(szCateName, sizeof(szCateName), c_pszCateName, strlen(c_pszCateName));

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "INSERT INTO %s(cat_name, cat_main_id) VALUES ('%s', '%d');", ISHOP_CATE_ROAD, szCateName, iCateID);
	DBManager::instance().DirectQuery(szQuery);

	ReInstall();
}

void CDragonCoinShopManager::DeleteCateItemShopAdmin(int iCateID)
{
	DBManager::instance().DirectQuery("DELETE FROM %s WHERE cat_id = '%d';", ISHOP_CATE_ROAD, iCateID);
	DBManager::instance().DirectQuery("DELETE FROM %s WHERE cat_main_id = '%d';", ISHOP_CATE_ROAD, iCateID);
	DBManager::instance().DirectQuery("DELETE FROM %s WHERE category_id = '%d';", ISHOP_ITEM_ROAD, iCateID);
	ReInstall();
}

void CDragonCoinShopManager::DeleteSubCateItemShopAdmin(int iSubCateID)
{
	DBManager::instance().DirectQuery("DELETE FROM %s WHERE cat_id = '%d';", ISHOP_CATE_ROAD, iSubCateID);
	DBManager::instance().DirectQuery("DELETE FROM %s WHERE category_id = '%d';", ISHOP_ITEM_ROAD, iSubCateID);
	ReInstall();
}

void CDragonCoinShopManager::RenameCateItemShopAdmin(int iCateID, const char * c_pszCateName)
{
	char szCateName[32+1];
	DBManager::instance().EscapeString(szCateName, sizeof(szCateName), c_pszCateName, strlen(c_pszCateName));
	DBManager::instance().DirectQuery("UPDATE %s SET cat_name = '%s' WHERE cat_id = '%d';", ISHOP_CATE_ROAD, szCateName, iCateID);
	ReInstall();
}

void CDragonCoinShopManager::DeleteItemShopAdmin(int iID)
{
	DBManager::instance().DirectQuery("DELETE FROM %s WHERE id = '%d';", ISHOP_ITEM_ROAD, iID);
	ReInstall();
}

void CDragonCoinShopManager::AddItemShopAdmin(TGCIShopItemAdmin * t)
{
	char szQuery[QUERY_MAX_LEN];
	sprintf(szQuery, 
		"INSERT INTO %s SET "
		"vnum = %u"
		", count = %u"
		", price = %u"
		", discount = %u"
		", category_id = %u"
		", time_type = %u"
		", status = %u"
		", stock = %d"
		", sell_count = %u"
		", sell_type = %u"
		", add_time = '%s'"
		", offer_time = '%s'"
		", socket0 = %lu"
		", socket1 = %lu"
		", socket2 = %lu"
		", socket3 = %lu"
		", attrtype0 = %d, attrvalue0 = %d"
		", attrtype1 = %d, attrvalue1 = %d"
		", attrtype2 = %d, attrvalue2 = %d"
		", attrtype3 = %d, attrvalue3 = %d"
		", attrtype4 = %d, attrvalue4 = %d"
		", attrtype5 = %d, attrvalue5 = %d"
		", attrtype6 = %d, attrvalue6 = %d"
		, ISHOP_ITEM_ROAD
		, t->iVnum
		, t->iCount
		, t->iPrice
		, t->iDiscount
		, t->categoryID
		, t->bTimeType
		, t->bStatus
		, t->iStock
		, t->wSellCount
		, t->bSellType
		, t->szAddTime
		, t->szOfferTime
		, t->alSockets[0]
		, t->alSockets[1]
		, t->alSockets[2]
		, t->alSockets[3]
		, t->aAttr[0].bType, t->aAttr[0].sValue
		, t->aAttr[1].bType, t->aAttr[1].sValue
		, t->aAttr[2].bType, t->aAttr[2].sValue
		, t->aAttr[3].bType, t->aAttr[3].sValue
		, t->aAttr[4].bType, t->aAttr[4].sValue
		, t->aAttr[5].bType, t->aAttr[5].sValue
		, t->aAttr[6].bType, t->aAttr[6].sValue
	);
	DBManager::instance().DirectQuery(szQuery);
	ReInstall();
}

void CDragonCoinShopManager::UpdateItemShopAdmin(TGCIShopItemAdmin * t)
{
	char szQuery[QUERY_MAX_LEN];
	sprintf(szQuery, 
		"UPDATE %s SET "
		"vnum = %u"
		", count = %u"
		", price = %u"
		", discount = %u"
		", category_id = %u"
		", time_type = %u"
		", status = %u"
		", stock = %d"
		", sell_count = %u"
		", sell_type = %u"
		", add_time = '%s'"
		", offer_time = '%s'"
		", socket0 = %lu"
		", socket1 = %lu"
		", socket2 = %lu"
		", socket3 = %lu"
		", attrtype0 = %d, attrvalue0 = %d"
		", attrtype1 = %d, attrvalue1 = %d"
		", attrtype2 = %d, attrvalue2 = %d"
		", attrtype3 = %d, attrvalue3 = %d"
		", attrtype4 = %d, attrvalue4 = %d"
		", attrtype5 = %d, attrvalue5 = %d"
		", attrtype6 = %d, attrvalue6 = %d"
		" WHERE id = '%u';"
		, ISHOP_ITEM_ROAD
		, t->iVnum
		, t->iCount
		, t->iPrice
		, t->iDiscount
		, t->categoryID
		, t->bTimeType
		, t->bStatus
		, t->iStock
		, t->wSellCount
		, t->bSellType
		, t->szAddTime
		, t->szOfferTime
		, t->alSockets[0]
		, t->alSockets[1]
		, t->alSockets[2]
		, t->alSockets[3]
		, t->aAttr[0].bType, t->aAttr[0].sValue
		, t->aAttr[1].bType, t->aAttr[1].sValue
		, t->aAttr[2].bType, t->aAttr[2].sValue
		, t->aAttr[3].bType, t->aAttr[3].sValue
		, t->aAttr[4].bType, t->aAttr[4].sValue
		, t->aAttr[5].bType, t->aAttr[5].sValue
		, t->aAttr[6].bType, t->aAttr[6].sValue
		, t->iID
	);
	DBManager::instance().DirectQuery(szQuery);
	ReInstall();
}
#endif
///------------------------------------------------------------------------------------------------------------//
#endif
