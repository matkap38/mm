#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "char.h"
#include "item.h"
#include "item_manager.h"
#include "packet.h"
#include "log.h"
#include "db.h"
#include "locale_service.h"
#include "../../common/length.h"
#include "exchange.h"
#include "DragonSoul.h"
#include "config.h"
#include "pvp.h"
#include "char_manager.h"
#include "utils.h"

#ifdef ENABLE_MESSENGER_BLOCK
#include "messenger_manager.h"
#endif

#ifdef ENABLE_EXCHANGE_LOG
#include "quest.h"
#include "questmanager.h"
#endif

#include "GetEventFlagSystem.h"

#ifdef ENABLE_FULL_YANG
void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, uint64_t arg1, TItemPos arg2, DWORD arg3, void * pvData = NULL);
void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, uint64_t arg1, TItemPos arg2, DWORD arg3, void * pvData)
#else
void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, DWORD arg1, TItemPos arg2, DWORD arg3, void * pvData = NULL);
void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, DWORD arg1, TItemPos arg2, DWORD arg3, void * pvData)
#endif
{
	if (!ch->GetDesc())
		return;

	struct packet_exchange pack_exchg;

	pack_exchg.header		= HEADER_GC_EXCHANGE;
	pack_exchg.sub_header	= sub_header;
	pack_exchg.is_me		= is_me;
	pack_exchg.arg1			= arg1;
	pack_exchg.arg2			= arg2;
	pack_exchg.arg3			= arg3;

	if (sub_header == EXCHANGE_SUBHEADER_GC_ITEM_ADD && pvData)
	{
		#ifdef __ENABLE_TRADABLE_ICON__
		pack_exchg.arg4 = TItemPos(((LPITEM) pvData)->GetWindow(), ((LPITEM) pvData)->GetCell());
		#endif
		thecore_memcpy(&pack_exchg.alSockets, ((LPITEM) pvData)->GetSockets(), sizeof(pack_exchg.alSockets));
		thecore_memcpy(&pack_exchg.aAttr, ((LPITEM) pvData)->GetAttributes(), sizeof(pack_exchg.aAttr));
		#ifdef ENABLE_EVOLUTION_SYSTEM
		pack_exchg.iEvolution = ((LPITEM) pvData)->GetEvolution();
		#endif
		#ifdef ENABLE_CHANGELOOK_SYSTEM
		pack_exchg.dwTransmutation = ((LPITEM) pvData)->GetTransmutation();
		#endif
	}
	else
	{
		#ifdef __ENABLE_TRADABLE_ICON__
		pack_exchg.arg4 = TItemPos(RESERVED_WINDOW, 0);
		#endif
		memset(&pack_exchg.alSockets, 0, sizeof(pack_exchg.alSockets));
		memset(&pack_exchg.aAttr, 0, sizeof(pack_exchg.aAttr));
		#ifdef ENABLE_EVOLUTION_SYSTEM
		pack_exchg.iEvolution = 0;
		#endif
		#ifdef ENABLE_CHANGELOOK_SYSTEM
		pack_exchg.dwTransmutation = 0;
		#endif
	}

	ch->GetDesc()->Packet(&pack_exchg, sizeof(pack_exchg));
}

bool CHARACTER::ExchangeStart(LPCHARACTER victim)
{
	if (this == victim)
		return false;

	if (IsObserverMode())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("관전 상태에서는 교환을 할 수 없습니다."));
		return false;
	}

	if (victim->IsNPC())
		return false;

	#ifdef ENABLE_MESSENGER_BLOCK
	if (MessengerManager::instance().IsBlocked_Target(GetName(), victim->GetName()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s blokkk"), victim->GetName());
		return false;
	}

	if (MessengerManager::instance().IsBlocked_Me(GetName(), victim->GetName()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s blokkk_me"), victim->GetName());
		return false;
	}
	#endif

	//PREVENT_TRADE_WINDOW
	if (IsOpenSafebox() || GetShopOwner() || GetMyShop() || IsCubeOpen())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래창이 열려있을경우 거래를 할수 없습니다."));
		return false;
	}

	if (victim->IsOpenSafebox() || victim->GetShopOwner() || victim->GetMyShop() || victim->IsCubeOpen())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 다른 거래중이라 거래를 할수 없습니다."));
		return false;
	}
	//END_PREVENT_TRADE_WINDOW

	if (PreventTradeWindow(WND_EXCHANGE, true))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return false;
	}

	if (victim->PreventTradeWindow(WND_EXCHANGE, true))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return false;
	}

	// @fixme150 BEGIN
	if (quest::CQuestManager::instance().GetPCForce(this->GetPlayerID())->IsRunning() == true)
	{
		this->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if you're using quests"));
		victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if the other part using quests"));
		return false;
	}
	else if (quest::CQuestManager::instance().GetPCForce(victim->GetPlayerID())->IsRunning() == true)
	{
		victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if you're using quests"));
		this->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if the other part using quests"));
		return false;
	}
	// @fixme150 END

	// Check time.
	if (m_pkTimedEvent || victim->m_pkTimedEvent)
		return false;

	int iDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	if (iDist >= EXCHANGE_MAX_DISTANCE)
		return false;

	if (GetExchange())
		return false;

	if (victim->GetExchange())
	{
		exchange_packet(this, EXCHANGE_SUBHEADER_GC_ALREADY, 0, 0, NPOS, 0);
		return false;
	}

	if (victim->IsBlockMode(BLOCK_EXCHANGE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 교환 거부 상태입니다."));
		return false;
	}

	SetExchange(M2_NEW CExchange(this));
	victim->SetExchange(M2_NEW CExchange(victim));

	victim->GetExchange()->SetCompany(GetExchange());
	GetExchange()->SetCompany(victim->GetExchange());

	//
	SetExchangeTime();
	victim->SetExchangeTime();

	exchange_packet(victim, EXCHANGE_SUBHEADER_GC_START, 0, GetVID(), NPOS, 0);
	exchange_packet(this, EXCHANGE_SUBHEADER_GC_START, 0, victim->GetVID(), NPOS, 0);

	return true;
}

CExchange::CExchange(LPCHARACTER pOwner)
{
	m_pCompany = NULL;

	m_bAccept = false;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		m_apItems[i] = NULL;
		m_aItemPos[i] = NPOS;
		m_abItemDisplayPos[i] = 0;
	}

	m_lGold = 0;

	#ifdef ENABLE_EXCHANGE_LOG
	logIndex = 0;
	#endif

	m_pOwner = pOwner;
	pOwner->SetExchange(this);

	m_pGrid = M2_NEW CGrid(6,4);
}

CExchange::~CExchange()
{
	M2_DELETE(m_pGrid);
}

bool CExchange::AddItem(TItemPos item_pos, BYTE display_pos)
{
	assert(m_pOwner != NULL && GetCompany());

	if (!item_pos.IsValidItemPosition())
		return false;

	if (item_pos.IsEquipPosition())
		return false;

	// @fixme150 BEGIN
	LPCHARACTER me = GetOwner();
	LPCHARACTER victim = GetCompany()->GetOwner();

	if (quest::CQuestManager::instance().GetPCForce(me->GetPlayerID())->IsRunning() == true)
	{
		me->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if you're using quests"));
		victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if the other part using quests"));
		return false;
	}
	else if (quest::CQuestManager::instance().GetPCForce(victim->GetPlayerID())->IsRunning() == true)
	{
		victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if you're using quests"));
		me->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if the other part using quests"));
		return false;
	}
	// @fixme150 END

	LPITEM item;
	if (!(item = m_pOwner->GetItem(item_pos)))
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE))
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템을 건네줄 수 없습니다."));
		return false;
	}

	if (true == item->isLocked())
	{
		return false;
	}

	#ifdef ENABLE_LOCKED_SYSTEM
	if (true == item->IsItemLocked())
	{
		return false;
	}
	#endif

	if (item->IsExchanging())
	{
		sys_log(0, "EXCHANGE under exchanging");
		return false;
	}

	if (!m_pGrid->IsEmpty(display_pos, 1, item->GetSize()))
	{
		sys_log(0, "EXCHANGE not empty item_pos %d %d %d", display_pos, 1, item->GetSize());
		return false;
	}

	Accept(false);
	GetCompany()->Accept(false);

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			continue;

		m_apItems[i]			= item;
		m_aItemPos[i]			= item_pos;
		m_abItemDisplayPos[i]	= display_pos;
		m_pGrid->Put(display_pos, 1, item->GetSize());

		item->SetExchanging(true);

		exchange_packet(m_pOwner,
				EXCHANGE_SUBHEADER_GC_ITEM_ADD,
				true,
				item->GetVnum(),
				TItemPos(RESERVED_WINDOW, display_pos),
				item->GetCount(),
				item);

		exchange_packet(GetCompany()->GetOwner(),
				EXCHANGE_SUBHEADER_GC_ITEM_ADD,
				false,
				item->GetVnum(),
				TItemPos(RESERVED_WINDOW, display_pos),
				item->GetCount(),
				item);

		sys_log(0, "EXCHANGE AddItem success %s pos(%d, %d) %d", item->GetName(), item_pos.window_type, item_pos.cell, display_pos);
		return true;
	}

	return false;
}

bool CExchange::RemoveItem(BYTE pos)
{
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return false;

	if (!m_apItems[pos])
		return false;

	TItemPos PosOfInventory = m_aItemPos[pos];
	m_apItems[pos]->SetExchanging(false);

	m_pGrid->Get(m_abItemDisplayPos[pos], 1, m_apItems[pos]->GetSize());

	exchange_packet(GetOwner(),	EXCHANGE_SUBHEADER_GC_ITEM_DEL, true, pos, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_ITEM_DEL, false, pos, PosOfInventory, 0);

	Accept(false);
	GetCompany()->Accept(false);

	m_apItems[pos]	= NULL;
	m_aItemPos[pos]	= NPOS;
	m_abItemDisplayPos[pos] = 0;
	return true;
}

#ifdef ENABLE_FULL_YANG
bool CExchange::AddGold(int64_t gold)
#else
bool CExchange::AddGold(long gold)
#endif
{
	if (gold <= 0)
		return false;

	if (GetOwner()->GetGold() < gold)
	{
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_LESS_GOLD, 0, 0, NPOS, 0);
		return false;
	}

	if (m_lGold > 0)
		return false;

	// @fixme150 BEGIN
	LPCHARACTER me = GetOwner();
	LPCHARACTER victim = GetCompany()->GetOwner();

	if (quest::CQuestManager::instance().GetPCForce(me->GetPlayerID())->IsRunning() == true)
	{
		me->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if you're using quests"));
		victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if the other part using quests"));
		return false;
	}
	else if (quest::CQuestManager::instance().GetPCForce(victim->GetPlayerID())->IsRunning() == true)
	{
		victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if you're using quests"));
		me->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if the other part using quests"));
		return false;
	}
	// @fixme150 END

	Accept(false);
	GetCompany()->Accept(false);

	m_lGold = gold;

	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_GOLD_ADD, true, m_lGold, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_GOLD_ADD, false, m_lGold, NPOS, 0);
	return true;
}

bool CExchange::Check(int * piItemCount)
{
	if (GetOwner()->GetGold() < m_lGold)
		return false;

	int item_count = 0;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!m_apItems[i])
			continue;

		if (!m_aItemPos[i].IsValidItemPosition())
			return false;

		if (m_apItems[i] != GetOwner()->GetItem(m_aItemPos[i]))
			return false;

		++item_count;
	}

	*piItemCount = item_count;
	return true;
}

bool CExchange::CheckSpace()
{
	static CGrid s_grid1(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 1
	static CGrid s_grid2(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 2
	#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	static CGrid s_grid3(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 3
	static CGrid s_grid4(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 4
	#endif

	s_grid1.Clear();
	s_grid2.Clear();
	#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	s_grid3.Clear();
	s_grid4.Clear();
	#endif

	LPCHARACTER victim = GetCompany()->GetOwner();
	LPITEM item;

	int i;
	for (i = 0; i < INVENTORY_PAGE_SIZE*1; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid1.Put(i, 1, item->GetSize());
	}
	for (i = INVENTORY_PAGE_SIZE*1; i < INVENTORY_PAGE_SIZE*2; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid2.Put(i - INVENTORY_PAGE_SIZE*1, 1, item->GetSize());
	}
	#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	for (i = INVENTORY_PAGE_SIZE*2; i < INVENTORY_PAGE_SIZE*3; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid3.Put(i - INVENTORY_PAGE_SIZE*2, 1, item->GetSize());
	}
	for (i = INVENTORY_PAGE_SIZE*3; i < INVENTORY_PAGE_SIZE*4; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid4.Put(i - INVENTORY_PAGE_SIZE*3, 1, item->GetSize());
	}
	#endif

	static std::vector <WORD> s_vDSGrid(DRAGON_SOUL_INVENTORY_MAX_NUM);

	bool bDSInitialized = false;

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (item->IsDragonSoul())
		{
			if (!victim->DragonSoul_IsQualified())
			{
				return false;
			}

			if (!bDSInitialized)
			{
				bDSInitialized = true;
				victim->CopyDragonSoulItemGrid(s_vDSGrid);
			}

			bool bExistEmptySpace = false;
			WORD wBasePos = DSManager::instance().GetBasePosition(item);
			if (wBasePos >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

			for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; i++)
			{
				WORD wPos = wBasePos + i;
				if (0 == s_vDSGrid[wPos]) // @fixme159 (wBasePos to wPos)
				{
					bool bEmpty = true;
					for (int j = 1; j < item->GetSize(); j++)
					{
						if (s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM])
						{
							bEmpty = false;
							break;
						}
					}
					if (bEmpty)
					{
						for (int j = 0; j < item->GetSize(); j++)
						{
							s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM] =  wPos + 1;
						}
						bExistEmptySpace = true;
						break;
					}
				}
				if (bExistEmptySpace)
					break;
			}
			if (!bExistEmptySpace)
				return false;
		}
		else
		{
			int iPos;

			if ((iPos = s_grid1.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid1.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_grid2.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid2.Put(iPos, 1, item->GetSize());
			}
			#ifdef ENABLE_EXTEND_INVEN_SYSTEM
			else if ((iPos = s_grid3.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid3.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_grid4.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid4.Put(iPos, 1, item->GetSize());
			}
			#endif
			else
				return false;
		}
	}

	return true;
}

#ifdef ENABLE_SPECIAL_STORAGE
bool CExchange::CheckSpecialStorageSpace(BYTE bStorageType)
{
	static CGrid s_grid1(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW);
	static CGrid s_grid2(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW);
	static CGrid s_grid3(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW);
	static CGrid s_grid4(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW);

	s_grid1.Clear();
	s_grid2.Clear();
	s_grid3.Clear();
	s_grid4.Clear();

	LPCHARACTER victim = GetCompany()->GetOwner();
	LPITEM item;

	int i;

	const int perPageSlotCount = 45;

	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if (bStorageType < 0 || bStorageType > 3)
			continue;

		if (bStorageType == 0)
			item = victim->GetUpgradeInventoryItem(i);
		else if (bStorageType == 1)
			item = victim->GetBookInventoryItem(i);
		else if (bStorageType == 2)
			item = victim->GetStoneInventoryItem(i);
		else if (bStorageType == 3)
			item = victim->GetChestInventoryItem(i);

		if (!item)
			continue;

		BYTE itemSize = item->GetSize();

		if (i < perPageSlotCount)
			s_grid1.Put(i, 1, itemSize);
		else if (i < perPageSlotCount * 2)
			s_grid2.Put(i - perPageSlotCount, 1, itemSize);
		else if (i < perPageSlotCount * 3)
			s_grid3.Put(i - perPageSlotCount * 2, 1, itemSize);
		else
			s_grid4.Put(i - perPageSlotCount * 3, 1, itemSize);
	}

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (bStorageType < 0 || bStorageType > 3)
			continue;

		if ((bStorageType == 0 && (!item->IsUpgradeItem())) || (bStorageType == 1 && (!item->IsBook())) || (bStorageType == 2 && (!item->IsStone())) || (bStorageType == 3 && (!item->IsChest())))
			continue;

		BYTE itemSize = item->GetSize();

		int iPos = s_grid1.FindBlank(1, itemSize);
		if (iPos >= 0)
		{
			s_grid1.Put(iPos, 1, itemSize);
			continue;
		}

		iPos = s_grid2.FindBlank(1, itemSize);
		if (iPos >= 0)
		{
			s_grid2.Put(iPos, 1, itemSize);
			continue;
		}

		iPos = s_grid3.FindBlank(1, itemSize);
		if (iPos >= 0)
		{
			s_grid3.Put(iPos, 1, itemSize);
			continue;
		}

		iPos = s_grid4.FindBlank(1, itemSize);
		if (iPos >= 0)
		{
			s_grid4.Put(iPos, 1, itemSize);
			continue;
		}

		return false;  // No space left in inventory
	}

	return true;
}
#endif

bool CExchange::Done()
{
	int		empty_pos, i;
	LPITEM	item;

	LPCHARACTER victim = GetCompany()->GetOwner();

	#ifdef ENABLE_FULL_YANG
	if (m_lGold)
	{
		int64_t vic_gold = victim->GetGold() + m_lGold;
		if (vic_gold > GetOwner()->GetAllowedGold())
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Yang miktari cok fazla ticaret yapamazsin."));
			return false;
		}
	}
	#endif

	#ifdef ENABLE_EXCHANGE_LOG
	char szQuery[QUERY_MAX_LEN];
	#endif

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i])) { continue; }

		empty_pos = victim->GetEmptyAllInventory(item);
		if (empty_pos < 0)
		{
			sys_err("Exchange::Done : Cannot find blank position in inventory %s <-> %s item %s", m_pOwner->GetName(), victim->GetName(), item->GetName());
			continue;
		}

		assert(empty_pos >= 0);
		m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), 255);
		item->SetExchanging(false);

		#ifdef ENABLE_EXCHANGE_LOG
		if (logIndex > 0)
		{
			int iLen = snprintf(szQuery, sizeof(szQuery), "INSERT INTO log.exchange_log_items(id, pid, item_id, pos, vnum, count");

			for (BYTE j = 0; j < ITEM_SOCKET_MAX_NUM; ++j) { iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen,", socket%u", j); }
			for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j) { iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ", attrtype%u, attrvalue%u", j, j); }

			iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ") VALUES(%d, %u, %u, %d, %d, %d", logIndex, GetOwner()->GetPlayerID(), item->GetID(), m_abItemDisplayPos[i], item->GetVnum(), item->GetCount());

			for (BYTE j = 0; j < ITEM_SOCKET_MAX_NUM; ++j) { iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ", %ld", item->GetSocket(j)); }
			for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j) { iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ", %u, %d", item->GetAttributeType(j), item->GetAttributeValue(j)); }

			iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ")");
			DBManager::instance().DirectQuery(szQuery);
		}
		#endif

		{
			char exchange_buf[51];

			snprintf(exchange_buf, sizeof(exchange_buf), "%s %u %u", item->GetName(), GetOwner()->GetPlayerID(), item->GetCount());
			LogManager::instance().ItemLog(victim, item, "EXCHANGE_TAKE", exchange_buf);

			snprintf(exchange_buf, sizeof(exchange_buf), "%s %u %u", item->GetName(), victim->GetPlayerID(), item->GetCount());
			LogManager::instance().ItemLog(GetOwner(), item, "EXCHANGE_GIVE", exchange_buf);

			if (item->GetVnum() >= 80003 && item->GetVnum() <= 80007)
			{
				LogManager::instance().GoldBarLog(victim->GetPlayerID(), item->GetID(), EXCHANGE_TAKE, "");
				LogManager::instance().GoldBarLog(GetOwner()->GetPlayerID(), item->GetID(), EXCHANGE_GIVE, "");
			}
		}

		item->SetSkipSave(false);
		item->RemoveFromCharacter();
		item->AddToItem(victim, empty_pos);
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		DWORD dwID = item->GetID();
		db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_FLUSH, 0, sizeof(DWORD));
		db_clientdesc->Packet(&dwID, sizeof(DWORD));

		m_apItems[i] = NULL;
	}

	if (m_lGold)
	{
		#ifdef ENABLE_FULL_YANG
		GetOwner()->GoldChange(-m_lGold);
		victim->GoldChange(m_lGold);
		#else
		GetOwner()->PointChange(POINT_GOLD, -m_lGold, true);
		victim->PointChange(POINT_GOLD, m_lGold, true);
		#endif

		if (m_lGold > 1000)
		{
			char exchange_buf[51];
			snprintf(exchange_buf, sizeof(exchange_buf), "%u %s", GetOwner()->GetPlayerID(), GetOwner()->GetName());
			LogManager::instance().CharLog(victim, m_lGold, "EXCHANGE_GOLD_TAKE", exchange_buf);

			snprintf(exchange_buf, sizeof(exchange_buf), "%u %s", victim->GetPlayerID(), victim->GetName());
			LogManager::instance().CharLog(GetOwner(), m_lGold, "EXCHANGE_GOLD_GIVE", exchange_buf);
		}
	}

	GetOwner()->SetMyShopTime();
	GetOwner()->SetMyExchangeTime();
	victim->SetMyShopTime();
	victim->SetMyExchangeTime();

	m_pGrid->Clear();
	return true;
}

bool CExchange::Accept(bool bAccept)
{
	if (m_bAccept == bAccept)
		return true;

	m_bAccept = bAccept;

	if (m_bAccept && GetCompany()->m_bAccept)
	{
		int iItemCount;

		LPCHARACTER victim = GetCompany()->GetOwner();

		//PREVENT_PORTAL_AFTER_EXCHANGE
		GetOwner()->SetExchangeTime();
		victim->SetExchangeTime();
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

		// @fixme150 BEGIN
		if (quest::CQuestManager::instance().GetPCForce(GetOwner()->GetPlayerID())->IsRunning() == true)
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if you're using quests"));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if the other part using quests"));
			goto EXCHANGE_END;
		}
		else if (quest::CQuestManager::instance().GetPCForce(victim->GetPlayerID())->IsRunning() == true)
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if you're using quests"));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if the other part using quests"));
			goto EXCHANGE_END;
		}
		// @fixme150 END

		if (!Check(&iItemCount))
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("돈이 부족하거나 아이템이 제자리에 없습니다."));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 돈이 부족하거나 아이템이 제자리에 없습니다."));
			goto EXCHANGE_END;
		}

		if (!CheckSpace())
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 소지품에 빈 공간이 없습니다."));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지품에 빈 공간이 없습니다."));
			goto EXCHANGE_END;
		}

		#ifdef ENABLE_SPECIAL_STORAGE
		for (int i = 0; i < 3; i++)
		{
			if (!CheckSpecialStorageSpace(i))
			{
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 소지품에 빈 공간이 없습니다."));
				victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지품에 빈 공간이 없습니다."));
				goto EXCHANGE_END;
			}

			if (!GetCompany()->CheckSpecialStorageSpace(i))
			{
				victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 소지품에 빈 공간이 없습니다."));
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지품에 빈 공간이 없습니다."));
				goto EXCHANGE_END;
			}
		}
		#endif

		if (!GetCompany()->Check(&iItemCount))
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("돈이 부족하거나 아이템이 제자리에 없습니다."));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 돈이 부족하거나 아이템이 제자리에 없습니다."));
			goto EXCHANGE_END;
		}

		if (!GetCompany()->CheckSpace())
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 소지품에 빈 공간이 없습니다."));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지품에 빈 공간이 없습니다."));
			goto EXCHANGE_END;
		}

		if (db_clientdesc->GetSocket() == INVALID_SOCKET)
		{
			sys_err("Cannot use exchange feature while DB cache connection is dead.");
			victim->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			goto EXCHANGE_END;
		}

		{
			#ifdef ENABLE_EXCHANGE_LOG
			TExchangeLog p;
			strlcpy(p.owner, victim->GetName(), sizeof(p.owner));
			p.ownerPID = victim->GetPlayerID();
			p.ownerGold = GetCompany()->m_lGold;
			strlcpy(p.ownerIP, victim->GetDesc()->GetHostName(), sizeof(p.ownerIP));

			strlcpy(p.target, GetOwner()->GetName(), sizeof(p.target));
			p.targetPID = GetOwner()->GetPlayerID();
			p.targetGold = m_lGold;
			strlcpy(p.targetIP, GetOwner()->GetDesc()->GetHostName(), sizeof(p.targetIP));
			p.itemsLoaded = false;

			const time_t curr_time = time(0);
			const tm* curr_tm = localtime(&curr_time);
			strftime(p.date, 50, "%T - %d/%m/%y", curr_tm);

			//const bool isGameMasterExchange = (GetOwner()->IsGm() || victim->IsGm()) ? true: false;
			const bool isGameMasterExchange = false;//SHOW GAMEMASTER EXCHANGE IN GAME

			char szQuery[QUERY_MAX_LEN];
			snprintf(szQuery, sizeof(szQuery), "INSERT INTO log.exchange_log(owner, owner_pid, owner_gold, owner_ip, target, target_pid, target_gold, target_ip, date, owner_delete, target_delete) VALUES('%s', %u, %lld, '%s', '%s', %u, %lld, '%s', '%s', %d, %d)", p.owner, p.ownerPID, p.ownerGold, p.ownerIP, p.target, p.targetPID, p.targetGold, p.targetIP, p.date, isGameMasterExchange, isGameMasterExchange);

			auto pMsgMk(DBManager::instance().DirectQuery(szQuery));
			if (pMsgMk->Get()->uiInsertID > 0)
			{
				const int newLogID = pMsgMk->Get()->uiInsertID;
				// sys_err("newLogID: %d", newLogID);

				logIndex = newLogID;
				GetCompany()->logIndex = newLogID;

				if (!isGameMasterExchange)
				{
					if (GetOwner()->GetProtectTime("ExchangeLogLoaded") == 1)
						GetOwner()->SendExchangeLogPacket(SUB_EXCHANGELOG_LOAD, newLogID, &p);
					if (victim->GetProtectTime("ExchangeLogLoaded") == 1)
						victim->SendExchangeLogPacket(SUB_EXCHANGELOG_LOAD, newLogID, &p);
				}
			}
			#endif

			if (Done())
			{
				if (m_lGold)
					GetOwner()->Save();

				if (GetCompany()->Done())
				{
					if (GetCompany()->m_lGold)
						victim->Save();

					GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ??? ??? ?? ?????."), victim->GetName());
					victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ??? ??? ?? ?????."), GetOwner()->GetName());
				}
			}
		}

EXCHANGE_END:
		Cancel();
		return false;
	}
	else
	{
		// accept.
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_ACCEPT, true, m_bAccept, NPOS, 0);
		exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_ACCEPT, false, m_bAccept, NPOS, 0);
		return true;
	}
}

void CExchange::Cancel()
{
	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_END, 0, 0, NPOS, 0);
	GetOwner()->SetExchange(NULL);

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			m_apItems[i]->SetExchanging(false);
	}

	if (GetCompany())
	{
		GetCompany()->SetCompany(NULL);
		GetCompany()->Cancel();
	}

	M2_DELETE(this);
}
