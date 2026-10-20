#include "stdafx.h"
#include "constants.h"
#include "char.h"
#include "cmd.h"
#include "utils.h"
#include "db.h"
#include "config.h"
#include "packet.h"
#include "p2p.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "blackmarket.h"

#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
#include "GetEventFlagSystem.h"
#endif

#ifdef ENABLE_BLACK_MARKET_SYSTEM
CBlackMarket::CBlackMarket() { Initialize(); }
CBlackMarket::~CBlackMarket() { m_map_pkBlackMarket.clear(); m_vec_pkBlackMarket.clear(); }
CBlackMarket::SBlackMarket::SBlackMarket() { dwVnum = 0; llPrice = 0; iStock = -1; bStockP2p = false; }

void CBlackMarket::Initialize()
{
	m_map_pkBlackMarket.clear();
	m_vec_pkBlackMarket.clear();

	auto pMsg(DBManager::instance().DirectQuery("SELECT vnum, yang, stock, stock_p2p FROM server_settings.blackmarket;"));
	if (pMsg->Get()->uiNumRows == 0)
	{
		// sys_err("sql query null");
		return;
	}

	MYSQL_ROW row;
	DWORD	Vnum = 0;
	#ifdef ENABLE_FULL_YANG
	int64_t	Gold = 0;
	#else
	long	Gold = 0;
	#endif
	int		Stock = 0;
	bool	StockP2p = false;

	while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		int con = 0;

		str_to_number(Vnum, row[con++]);
		#ifdef ENABLE_FULL_YANG
		Gold = strtoull(row[con++], NULL, 10);
		#else
		str_to_number(Gold, row[con++]);
		#endif
		str_to_number(Stock, row[con++]);
		str_to_number(StockP2p, row[con++]);

		m_map_pkBlackMarket.emplace(Vnum, SBlackMarket());

		TBlackMarket & rPriceData = m_map_pkBlackMarket[Vnum];
		rPriceData.dwVnum = Vnum;
		rPriceData.llPrice = Gold;
		rPriceData.iStock = Stock;
		rPriceData.bStockP2p = StockP2p;

		m_vec_pkBlackMarket.emplace_back(TBlackMarketData(Vnum, Gold, Stock));
	}
}

bool CBlackMarket::GetPriceData(DWORD dwVnum, TBlackMarket ** pPriceData)
{
	TBlackMarketMap::iterator it = m_map_pkBlackMarket.find(dwVnum);
	if (m_map_pkBlackMarket.end() == it) { return false; }
	*pPriceData = &(it->second);
	return true;
}

#ifdef ENABLE_FULL_YANG
int64_t CBlackMarket::GetGoldPrice(DWORD dwVnum)
#else
long CBlackMarket::GetGoldPrice(DWORD dwVnum)
#endif
{
	SBlackMarket * c_pPriceData;
	if (!GetPriceData(dwVnum, &c_pPriceData)) { return 0; }
	return c_pPriceData->llPrice;
}

int CBlackMarket::GetStock(DWORD dwVnum)
{
	SBlackMarket * c_pPriceData;
	if (!GetPriceData(dwVnum, &c_pPriceData)) { return -1; }
	return c_pPriceData->iStock;
}

void CBlackMarket::SetStock(DWORD dwVnum, int iStock, bool bP2p)
{
	SBlackMarket * c_pPriceData;
	if (!GetPriceData(dwVnum, &c_pPriceData)) { return; }
	if (c_pPriceData->iStock <= 0) { return; }

	if ((c_pPriceData->iStock - iStock) <= 0)
	{
		c_pPriceData->iStock = 0;
	}
	else
	{
		c_pPriceData->iStock -= iStock;
	}

	if (c_pPriceData->bStockP2p && bP2p)
	{
		TPacketGGBlackMarket pggblack;
		pggblack.bHeader = HEADER_GG_BLACKMARKET;
		pggblack.bSubHeader = BM_SUBHEADER_REFRESH;
		pggblack.dwVnum = dwVnum;
		pggblack.iStock = iStock;
		P2P_MANAGER::instance().Send(&pggblack, sizeof(TPacketGGBlackMarket));
	}
}

void CBlackMarket::GetPriceList(LPCHARACTER ch)
{
	if (!gef_black_market_status) { return; }
	if (!ch || !ch->GetDesc()) { return; }
	if (m_vec_pkBlackMarket.empty()) { return; }

	TPacketGCItemBlackMarket packet;
	packet.bHeader = HEADER_GC_BLACKMARKET;
	packet.wSize = sizeof(packet) + sizeof(TBlackMarketData) * m_vec_pkBlackMarket.size();
	ch->GetDesc()->BufferedPacket(&packet, sizeof(packet));
	ch->GetDesc()->Packet(&m_vec_pkBlackMarket[0], sizeof(TBlackMarketData) * m_vec_pkBlackMarket.size());
}

#endif//End
