#include "stdafx.h"

#include <stack>

#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "skill.h"
#include "start_position.h"
#include "mob_manager.h"
#include "db.h"
#include "log.h"
#include "vector.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "fishing.h"
#include "party.h"
#include "dungeon.h"
#include "refine.h"
#include "unique_item.h"
#include "war_map.h"
#include "xmas_event.h"
#include "marriage.h"
#include "monarch.h"
#include "polymorph.h"
#include "blend_item.h"
#include "castle.h"
#include "BattleArena.h"
#include "arena.h"
#include "threeway_war.h"

#include "safebox.h"
#include "shop.h"

#ifdef ENABLE_NEWSTUFF
#include "pvp.h"
#include "../../common/PulseManager.h"
#endif

#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#endif

#ifdef ENABLE_COSTUME_PET_SYSTEM
#include "PetSystem.h"
#endif

#ifdef ENABLE_BATTLE_PASS
#include "battle_pass.h"
#endif

#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
#include "GetEventFlagSystem.h"
#endif

#ifdef ENABLE_BLACK_MARKET_SYSTEM
#include "blackmarket.h"
#endif

#include "../../common/VnumHelper.h"
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"
#include "../../common/CommonDefines.h"

const int ITEM_BROKEN_METIN_VNUM = 28960;
#define ENABLE_EFFECT_EXTRAPOT
#define ENABLE_BOOKS_STACKFIX
#define ENABLE_ITEM_RARE_ATTR_LEVEL_PCT

#ifdef ENABLE_BIYOLOG
extern ACMD(do_bio);
#endif

// CHANGE_ITEM_ATTRIBUTES
// const DWORD CHARACTER::msc_dwDefaultChangeItemAttrCycle = 10;
const char CHARACTER::msc_szLastChangeItemAttrFlag[] = "Item.LastChangeItemAttr";
// const char CHARACTER::msc_szChangeItemAttrCycleFlag[] = "change_itemattr_cycle";
// END_OF_CHANGE_ITEM_ATTRIBUTES
const BYTE g_aBuffOnAttrPoints[] = { POINT_ENERGY, POINT_COSTUME_ATTR_BONUS };

struct FFindStone
{
	std::map<DWORD, LPCHARACTER> m_mapStone;

	void operator()(LPENTITY pEnt)
	{
		if (pEnt->IsType(ENTITY_CHARACTER) == true)
		{
			LPCHARACTER pChar = (LPCHARACTER)pEnt;

			if (pChar->IsStone() == true)
			{
				m_mapStone[(DWORD)pChar->GetVID()] = pChar;
			}
		}
	}
};

static bool IS_SUMMON_ITEM(int vnum)
{
	switch (vnum)
	{
		case 22000:
		case 22010:
		case 22011:
		case 22020:
		case ITEM_MARRIAGE_RING:
			return true;
	}

	return false;
}

static bool IS_MONKEY_DUNGEON(int map_index)
{
	switch (map_index)
	{
		case 5:
		case 25:
		case 45:
		case 108:
		case 109:
			return true;;
	}

	return false;
}

bool IS_SUMMONABLE_ZONE(int map_index)
{
	if (IS_MONKEY_DUNGEON(map_index))
		return false;

	if (IS_CASTLE_MAP(map_index))
		return false;

	switch (map_index)
	{
		case 66 :
		case 71 :
		case 72 :
		case 73 :
		case 193 :

		#if 0
		case 184 :
		case 185 :
		case 186 :
		case 187 :
		case 188 :
		case 189 :
		#endif

		case 216 :
		case 217 :
		case 208 :

		case 113 :
			return false;
	}

	if (CBattleArena::IsBattleArenaMap(map_index)) return false;


	if (map_index > 10000) return false;

	return true;
}

bool IS_BOTARYABLE_ZONE(int nMapIndex)
{
	if (!g_bEnableBootaryCheck) return true;

	switch (nMapIndex)
	{
		case 1 :
		case 3 :
		case 21 :
		case 23 :
		case 41 :
		case 43 :
			return true;
	}

	return false;
}

bool IS_CHECK_SUCCES_STONE(DWORD dwVnum)
{
	switch (dwVnum)
	{
		case 28046:
		case 28146:
		case 28246:
		case 28346:
		case 28446:
		case 28546:
		case 28646:
		case 28746:
		case 28846:
		case 28946:
		//-------//
		case 28047:
		case 28147:
		case 28247:
		case 28347:
		case 28447:
		case 28547:
		case 28647:
		case 28747:
		case 28847:
		case 28947:
		//-------//
		case 28048:
		case 28148:
		case 28248:
		case 28348:
		case 28448:
		case 28548:
		case 28648:
		case 28748:
		case 28848:
		case 28948:
		//-------//
		case 28049:
		case 28149:
		case 28249:
		case 28349:
		case 28449:
		case 28549:
		case 28649:
		case 28749:
		case 28849:
		case 28949:
		case 28444:
		case 28445:
			return true;
	}

	return false;
}

static bool FN_check_item_socket(LPITEM item)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		if (item->GetSocket(i) != item->GetProto()->alSockets[i])
			return false;
	}

	return true;
}

static void FN_copy_item_socket(LPITEM dest, LPITEM src)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		dest->SetSocket(i, src->GetSocket(i));
	}
}
static bool FN_check_item_sex(LPCHARACTER ch, LPITEM item)
{

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_MALE))
	{
		if (SEX_MALE==GET_SEX(ch))
			return false;
	}

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_FEMALE))
	{
		if (SEX_FEMALE==GET_SEX(ch))
			return false;
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// ITEM HANDLING
/////////////////////////////////////////////////////////////////////////////
bool CHARACTER::CanHandleItem(bool bSkipCheckRefine, bool bSkipObserver)
{
	if (!bSkipObserver)
		if (m_bIsObserver)
			return false;

	if (GetMyShop())
		return false;

	if (!bSkipCheckRefine)
		if (m_bUnderRefine)
			return false;

	if (IsCubeOpen() || NULL != DragonSoul_RefineWindow_GetOpener())
		return false;

	if (IsWarping())
		return false;

	#if defined(__BL_LUCKY_BOX__)
	if (IsLuckyBoxOpen())
		return false;
	#endif

	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if ((m_bAcceCombination) || (m_bAcceAbsorption))
		return false;
	#endif

	#ifdef ENABLE_CHANGELOOK_SYSTEM
	if (m_bChangeLook)
		return false;
	#endif

	return true;
}

LPITEM CHARACTER::GetInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}

#ifdef ENABLE_SPECIAL_STORAGE
LPITEM CHARACTER::GetUpgradeInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(UPGRADE_INVENTORY, wCell));
}

LPITEM CHARACTER::GetBookInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(BOOK_INVENTORY, wCell));
}

LPITEM CHARACTER::GetStoneInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(STONE_INVENTORY, wCell));
}

LPITEM CHARACTER::GetChestInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(CHEST_INVENTORY, wCell));
}
#endif

#ifdef ENABLE_SWITCHBOT
LPITEM CHARACTER::GetSwitchBotItem(WORD wCell) const
{
	return GetItem(TItemPos(SWITCHBOT, wCell));
}
#endif

LPITEM CHARACTER::GetItem(TItemPos Cell) const
{
	if (!m_PlayerSlots)
		return nullptr;
	if (!IsValidItemPosition(Cell))
		return NULL;

	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;

	switch (window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid item cell %d", wCell);
			return NULL;
		}
		return m_PlayerSlots->pItems[wCell];

	case DRAGON_SOUL_INVENTORY:
		if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid DS item cell %d", wCell);
			return NULL;
		}
		return m_PlayerSlots->pDSItems[wCell];

	#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSU item cell %d", wCell);
			return NULL;
		}
		return m_PlayerSlots->pSSUItems[wCell];

	case BOOK_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSB item cell %d", wCell);
			return NULL;
		}
		return m_PlayerSlots->pSSBItems[wCell];

	case STONE_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSS item cell %d", wCell);
			return NULL;
		}
		return m_PlayerSlots->pSSSItems[wCell];

	case CHEST_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSS item cell %d", wCell);
			return NULL;
		}
		return m_PlayerSlots->pSSCItems[wCell];
	#endif

	#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid switchbot item cell %d", wCell);
			return NULL;
		}
		return m_PlayerSlots->pSwitchbotItems[wCell];
	#endif

	default:
		return NULL;
	}

	return NULL;
}

void CHARACTER::SetItem(TItemPos Cell, LPITEM pItem
	#ifdef ENABLE_HIGHLIGHT_NEW_ITEM
	, bool bWereMine
	#endif
)
{
	if (!m_PlayerSlots)
		return;
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	if ((unsigned long)((CItem*)pItem) == 0xff || (unsigned long)((CItem*)pItem) == 0xffffffff)
	{
		sys_err("!!! FATAL ERROR !!! item == 0xff (char: %s cell: %u)", GetName(), wCell);
		core_dump();
		return;
	}

	if (pItem && pItem->GetOwner())
	{
		assert(!"GetOwner exist");
		return;
	}

	switch(window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		{
			if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
			{
				sys_err("CHARACTER::SetItem: invalid item cell %d", wCell);
				return;
			}

			LPITEM pOld = m_PlayerSlots->pItems[wCell];
			if (pOld)
			{
				if (wCell < INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= INVENTORY_MAX_NUM)
							continue;

						if (m_PlayerSlots->pItems[p] && m_PlayerSlots->pItems[p] != pOld)
							continue;

						m_PlayerSlots->bItemGrid[p] = 0;
					}
				}
				else
					m_PlayerSlots->bItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell < INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= INVENTORY_MAX_NUM)
							continue;

						m_PlayerSlots->bItemGrid[p] = wCell + 1;
					}
				}
				else
					m_PlayerSlots->bItemGrid[wCell] = wCell + 1;
			}

			m_PlayerSlots->pItems[wCell] = pItem;
		}
		break;

	case DRAGON_SOUL_INVENTORY:
		{
			LPITEM pOld = m_PlayerSlots->pDSItems[wCell];
			if (pOld)
			{
				if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						if (m_PlayerSlots->pDSItems[p] && m_PlayerSlots->pDSItems[p] != pOld)
							continue;

						m_PlayerSlots->wDSItemGrid[p] = 0;
					}
				}
				else
					m_PlayerSlots->wDSItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid DS item cell %d", wCell);
					return;
				}

				if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						m_PlayerSlots->wDSItemGrid[p] = wCell + 1;
					}
				}
				else
					m_PlayerSlots->wDSItemGrid[wCell] = wCell + 1;
			}

			m_PlayerSlots->pDSItems[wCell] = pItem;
		}
		break;

	#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
		{
			LPITEM pOld = m_PlayerSlots->pSSUItems[wCell];
			if (pOld)
			{
				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						if (m_PlayerSlots->pSSUItems[p] && m_PlayerSlots->pSSUItems[p] != pOld)
							continue;

						m_PlayerSlots->wSSUItemGrid[p] = 0;
					}
				}
				else
					m_PlayerSlots->wSSUItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid SSU item cell %d", wCell);
					return;
				}

				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						m_PlayerSlots->wSSUItemGrid[p] = wCell + 1;
					}
				}
				else
					m_PlayerSlots->wSSUItemGrid[wCell] = wCell + 1;
			}

			m_PlayerSlots->pSSUItems[wCell] = pItem;
		}
		break;

	case BOOK_INVENTORY:
		{
			LPITEM pOld = m_PlayerSlots->pSSBItems[wCell];
			if (pOld)
			{
				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						if (m_PlayerSlots->pSSBItems[p] && m_PlayerSlots->pSSBItems[p] != pOld)
							continue;

						m_PlayerSlots->wSSBItemGrid[p] = 0;
					}
				}
				else
					m_PlayerSlots->wSSBItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid SSB item cell %d", wCell);
					return;
				}

				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						m_PlayerSlots->wSSBItemGrid[p] = wCell + 1;
					}
				}
				else
					m_PlayerSlots->wSSBItemGrid[wCell] = wCell + 1;
			}

			m_PlayerSlots->pSSBItems[wCell] = pItem;
		}
		break;

	case STONE_INVENTORY:
		{
			LPITEM pOld = m_PlayerSlots->pSSSItems[wCell];
			if (pOld)
			{
				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						if (m_PlayerSlots->pSSSItems[p] && m_PlayerSlots->pSSSItems[p] != pOld)
							continue;

						m_PlayerSlots->wSSSItemGrid[p] = 0;
					}
				}
				else
					m_PlayerSlots->wSSSItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid SSB item cell %d", wCell);
					return;
				}

				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						m_PlayerSlots->wSSSItemGrid[p] = wCell + 1;
					}
				}
				else
					m_PlayerSlots->wSSSItemGrid[wCell] = wCell + 1;
			}

			m_PlayerSlots->pSSSItems[wCell] = pItem;
		}
		break;

	case CHEST_INVENTORY:
		{
			LPITEM pOld = m_PlayerSlots->pSSCItems[wCell];
			if (pOld)
			{
				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						if (m_PlayerSlots->pSSCItems[p] && m_PlayerSlots->pSSCItems[p] != pOld)
							continue;

						m_PlayerSlots->wSSCItemGrid[p] = 0;
					}
				}
				else
					m_PlayerSlots->wSSCItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid SSB item cell %d", wCell);
					return;
				}

				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						m_PlayerSlots->wSSCItemGrid[p] = wCell + 1;
					}
				}
				else
					m_PlayerSlots->wSSCItemGrid[wCell] = wCell + 1;
			}

			m_PlayerSlots->pSSCItems[wCell] = pItem;
		}
		break;
	#endif

	#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		{
			LPITEM pOld = m_PlayerSlots->pSwitchbotItems[wCell];
			if (pItem && pOld)
			{
				return;
			}

			if (wCell >= SWITCHBOT_SLOT_COUNT)
			{
				sys_err("CHARACTER::SetItem: invalid switchbot item cell %d", wCell);
				return;
			}

			if (pItem)
			{
				CSwitchbotManager::Instance().RegisterItem(GetPlayerID(), pItem->GetID(), wCell);
			}
			else
			{
				CSwitchbotManager::Instance().UnregisterItem(GetPlayerID(), wCell);
			}

			m_PlayerSlots->pSwitchbotItems[wCell] = pItem;
		}
		break;
	#endif

	default:
		sys_err ("Invalid Inventory type %d", window_type);
		return;
	}

	if (GetDesc())
	{
		if (pItem)
		{
			TPacketGCItemSet pack;
			pack.header = HEADER_GC_ITEM_SET;
			pack.Cell = Cell;

			pack.count = pItem->GetCount();
			pack.vnum = pItem->GetVnum();
			pack.flags = pItem->GetFlag();
			pack.anti_flags = pItem->GetAntiFlag();

			#ifdef ENABLE_HIGHLIGHT_NEW_ITEM
			pack.highlight = !bWereMine || (Cell.window_type == DRAGON_SOUL_INVENTORY);
			#else
			pack.highlight = (Cell.window_type == DRAGON_SOUL_INVENTORY);
			#endif

			thecore_memcpy(pack.alSockets, pItem->GetSockets(), sizeof(pack.alSockets));
			thecore_memcpy(pack.aAttr, pItem->GetAttributes(), sizeof(pack.aAttr));

			#ifdef ENABLE_EVOLUTION_SYSTEM
			pack.iEvolution = pItem->GetEvolution();
			#endif
			#ifdef ENABLE_CHANGELOOK_SYSTEM
			pack.dwTransmutation = pItem->GetTransmutation();
			#endif
			#ifdef ENABLE_LOCKED_SYSTEM
			pack.bLocked = pItem->GetLocked();
			#endif

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemSet));
		}
		else
		{
			TPacketGCItemDelDeprecated pack;
			pack.header = HEADER_GC_ITEM_DEL;
			pack.Cell = Cell;
			GetDesc()->Packet(&pack, sizeof(TPacketGCItemDelDeprecated));
		}
	}

	if (pItem)
	{
		pItem->SetCell(this, wCell);
		switch (window_type)
		{
		case INVENTORY:
		case EQUIPMENT:
			if ((wCell < INVENTORY_MAX_NUM) || (BELT_INVENTORY_SLOT_START <= wCell && BELT_INVENTORY_SLOT_END > wCell))
				pItem->SetWindow(INVENTORY);
			else
				pItem->SetWindow(EQUIPMENT);
			break;
		case DRAGON_SOUL_INVENTORY:
			pItem->SetWindow(DRAGON_SOUL_INVENTORY);
			break;
		#ifdef ENABLE_SPECIAL_STORAGE
		case UPGRADE_INVENTORY:
			pItem->SetWindow(UPGRADE_INVENTORY);
			break;
		case BOOK_INVENTORY:
			pItem->SetWindow(BOOK_INVENTORY);
			break;
		case STONE_INVENTORY:
			pItem->SetWindow(STONE_INVENTORY);
			break;
		case CHEST_INVENTORY:
			pItem->SetWindow(CHEST_INVENTORY);
			break;
		#endif
		#ifdef ENABLE_SWITCHBOT
		case SWITCHBOT:
			pItem->SetWindow(SWITCHBOT);
			break;
		#endif
		}
	}
}

LPITEM CHARACTER::GetWear(BYTE bCell) const
{
	if (!m_PlayerSlots)
		return nullptr;
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::GetWear: invalid wear cell %d", bCell);
		return NULL;
	}
	return m_PlayerSlots->pItems[INVENTORY_MAX_NUM + bCell];
}

void CHARACTER::SetWear(BYTE bCell, LPITEM item)
{
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::SetItem: invalid item cell %d", bCell);
		return;
	}

	SetItem(TItemPos (INVENTORY, INVENTORY_MAX_NUM + bCell), item);

	#ifdef ENABLE_COSTUME_SET_BONUS
	if (IsLoadedAffect())
	{
		switch (bCell)
		{
			case WEAR_COSTUME_BODY:
			case WEAR_COSTUME_HAIR:
			case WEAR_COSTUME_WEAPON:
			case WEAR_COSTUME_PET:
				CHARACTER_MANAGER::Instance().CheckCostumeSetBonus(this);
				break;

			default:
				break;
		}
	}
	#endif

	// Büyülü Silah && Hava Kilici sönmesin.
	/*if (!item && bCell == WEAR_WEAPON)
	{
		if (IsAffectFlag(AFF_GWIGUM))
			RemoveAffect(SKILL_GWIGEOM);

		if (IsAffectFlag(AFF_GEOMGYEONG))
			RemoveAffect(SKILL_GEOMKYUNG);
	}*/
}

void CHARACTER::ClearItem()
{
	int		i;
	LPITEM	item;

	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = GetInventoryItem(i)))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);
			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
			SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
		}
	}

	for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);
			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}

	#ifdef ENABLE_SPECIAL_STORAGE
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(UPGRADE_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);
			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}

	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(BOOK_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);
			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}

	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(STONE_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);
			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}

	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(CHEST_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);
			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	#endif

	#ifdef ENABLE_SWITCHBOT
	for (i = 0; i < SWITCHBOT_SLOT_COUNT; ++i)
	{
		if ((item = GetItem(TItemPos(SWITCHBOT, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);
			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	#endif
}

bool CHARACTER::IsEmptyItemGrid(TItemPos Cell, BYTE bSize, int iExceptionCell) const
{
	if (!m_PlayerSlots)
		return false;
	switch (Cell.window_type)
	{
	case INVENTORY:
		{
			WORD bCell = Cell.cell;

			++iExceptionCell;

			if (Cell.IsBeltInventoryPosition())
			{
				LPITEM beltItem = GetWear(WEAR_BELT);

				if (NULL == beltItem)
					return false;

				if (false == CBeltInventoryHelper::IsAvailableCell(bCell - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
					return false;

				if (m_PlayerSlots->bItemGrid[bCell])
				{
					if (m_PlayerSlots->bItemGrid[bCell] == iExceptionCell)
						return true;

					return false;
				}

				if (bSize == 1)
					return true;

			}
			else if (bCell >= INVENTORY_MAX_NUM)
				return false;

			if (m_PlayerSlots->bItemGrid[bCell])
			{
				if (m_PlayerSlots->bItemGrid[bCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = bCell / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT);

					do
					{
						BYTE p = bCell + (5 * j);

						if (p >= INVENTORY_MAX_NUM)
							return false;

						if (p / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT) != bPage)
							return false;

						if (m_PlayerSlots->bItemGrid[p])
							if (m_PlayerSlots->bItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;
				BYTE bPage = bCell / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT);

				do
				{
					BYTE p = bCell + (5 * j);

					if (p >= INVENTORY_MAX_NUM)
						return false;

					if (p / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_PlayerSlots->bItemGrid[p])
						if (m_PlayerSlots->bItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;

	case DRAGON_SOUL_INVENTORY:
		{
			WORD wCell = Cell.cell;
			if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

			iExceptionCell++;

			if (m_PlayerSlots->wDSItemGrid[wCell])
			{
				if (m_PlayerSlots->wDSItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							return false;

						if (m_PlayerSlots->wDSItemGrid[p])
							if (m_PlayerSlots->wDSItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

					if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
						return false;

					if (m_PlayerSlots->wDSItemGrid[p]) // @fixme159 (from bItemGrid)
						if (m_PlayerSlots->wDSItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;

	#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
		{
			WORD wCell = Cell.cell;
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				return false;

			iExceptionCell++;

			if (m_PlayerSlots->wSSUItemGrid[wCell])
			{
				if (m_PlayerSlots->wSSUItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

					do
					{
						int p = wCell + (5 * j);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							return false;

						if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
							return false;

						if (m_PlayerSlots->wSSUItemGrid[p])
							if (m_PlayerSlots->wSSUItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;
				BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_PlayerSlots->wSSUItemGrid[p])
						if (m_PlayerSlots->wSSUItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;

	case BOOK_INVENTORY:
		{
			WORD wCell = Cell.cell;
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				return false;

			iExceptionCell++;

			if (m_PlayerSlots->wSSBItemGrid[wCell])
			{
				if (m_PlayerSlots->wSSBItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

					do
					{
						int p = wCell + (5 * j);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							return false;

						if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
							return false;

						if (m_PlayerSlots->wSSBItemGrid[p])
							if (m_PlayerSlots->wSSBItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;
				BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_PlayerSlots->wSSBItemGrid[p])
						if (m_PlayerSlots->wSSBItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;

	case STONE_INVENTORY:
		{
			WORD wCell = Cell.cell;
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				return false;

			iExceptionCell++;

			if (m_PlayerSlots->wSSSItemGrid[wCell])
			{
				if (m_PlayerSlots->wSSSItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

					do
					{
						int p = wCell + (5 * j);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							return false;

						if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
							return false;

						if (m_PlayerSlots->wSSSItemGrid[p])
							if (m_PlayerSlots->wSSSItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;
				BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_PlayerSlots->wSSSItemGrid[p])
						if (m_PlayerSlots->wSSSItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;

	case CHEST_INVENTORY:
		{
			WORD wCell = Cell.cell;
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				return false;

			iExceptionCell++;

			if (m_PlayerSlots->wSSCItemGrid[wCell])
			{
				if (m_PlayerSlots->wSSCItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

					do
					{
						int p = wCell + (5 * j);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							return false;

						if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
							return false;

						if (m_PlayerSlots->wSSCItemGrid[p])
							if (m_PlayerSlots->wSSCItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;
				BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_PlayerSlots->wSSCItemGrid[p])
						if (m_PlayerSlots->wSSCItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;
	#endif

	#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		{
			WORD wCell = Cell.cell;
			if (wCell >= SWITCHBOT_SLOT_COUNT)
			{
				return false;
			}

			if (m_PlayerSlots->pSwitchbotItems[wCell])
			{
				return false;
			}

			return true;
		}
		break;
	#endif
	}

	return false;
}

#ifdef ENABLE_SPLIT_ITEMS_SYSTEM
int CHARACTER::GetEmptyInventoryFromIndex(WORD index, BYTE itemSize) const //SPLIT ITEMS
{
	if (index > INVENTORY_MAX_NUM)
		return -1;

	for (WORD i = index; i < INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(INVENTORY, i), itemSize))
			return i;
	return -1;
}

#ifdef ENABLE_SPECIAL_STORAGE
int CHARACTER::GetEmptyUpgradeInventoryFromIndex(WORD index, BYTE itemSize) const //SPLIT ITEMS
{
	if (index > SPECIAL_INVENTORY_MAX_NUM)
		return -1;

	for (WORD i = index; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(UPGRADE_INVENTORY, i), itemSize))
			return i;
	return -1;
}

int CHARACTER::GetEmptyBookInventoryFromIndex(WORD index, BYTE itemSize) const //SPLIT ITEMS
{
	if (index > SPECIAL_INVENTORY_MAX_NUM)
		return -1;

	for (WORD i = index; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(BOOK_INVENTORY, i), itemSize))
			return i;
	return -1;
}

int CHARACTER::GetEmptyStoneInventoryFromIndex(WORD index, BYTE itemSize) const //SPLIT ITEMS
{
	if (index > SPECIAL_INVENTORY_MAX_NUM)
		return -1;

	for (WORD i = index; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(STONE_INVENTORY, i), itemSize))
			return i;
	return -1;
}

int CHARACTER::GetEmptyChestInventoryFromIndex(WORD index, BYTE itemSize) const //SPLIT ITEMS
{
	if (index > SPECIAL_INVENTORY_MAX_NUM)
		return -1;

	for (WORD i = index; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(CHEST_INVENTORY, i), itemSize))
			return i;
	return -1;
}
#endif
#endif

int CHARACTER::GetEmptyAllInventory(LPITEM item) const
{
	if (NULL == item)
		return -1;

	if (item->IsDragonSoul())
	{
		return GetEmptyDragonSoulInventory(item);
	}
	#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsUpgradeItem())
	{
		return GetEmptyUpgradeInventory(item);
	}
	else if (item->IsBook())
	{
		return GetEmptyBookInventory(item);
	}
	else if (item->IsStone())
	{
		return GetEmptyStoneInventory(item);
	}
	else if (item->IsChest())
	{
		return GetEmptyChestInventory(item);
	}
	#endif
	else // Normal Inventory
	{
		return GetEmptyInventory(item->GetSize());
	}

	return -1;
}

int CHARACTER::GetEmptyInventory(BYTE size) const
{
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), size))
			return i;
	return -1;
}

int CHARACTER::GetEmptyDragonSoulInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsDragonSoul())
		return -1;

	if (!DragonSoul_IsQualified())
	{
		return -1;
	}
	BYTE bSize = pItem->GetSize();
	WORD wBaseCell = DSManager::instance().GetBasePosition(pItem);

	if (WORD_MAX == wBaseCell)
		return -1;

	for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
		if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, i + wBaseCell), bSize))
			return i + wBaseCell;

	return -1;
}

void CHARACTER::CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const
{
	vDragonSoulItemGrid.resize(DRAGON_SOUL_INVENTORY_MAX_NUM);
	std::copy(m_PlayerSlots->wDSItemGrid.data(), m_PlayerSlots->wDSItemGrid.data() + DRAGON_SOUL_INVENTORY_MAX_NUM, vDragonSoulItemGrid.begin());
}

#ifdef ENABLE_SPECIAL_STORAGE
int CHARACTER::GetEmptyUpgradeInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsUpgradeItem())
		return -1;

	BYTE bSize = pItem->GetSize();
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(UPGRADE_INVENTORY, i), bSize))
			return i;

	return -1;
}

int CHARACTER::GetEmptyBookInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsBook())
		return -1;

	BYTE bSize = pItem->GetSize();
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(BOOK_INVENTORY, i), bSize))
			return i;

	return -1;
}

int CHARACTER::GetEmptyStoneInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsStone())
		return -1;

	BYTE bSize = pItem->GetSize();
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(STONE_INVENTORY, i), bSize))
			return i;

	return -1;
}

int CHARACTER::GetEmptyChestInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsChest())
		return -1;

	BYTE bSize = pItem->GetSize();
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(CHEST_INVENTORY, i), bSize))
			return i;

	return -1;
}
#endif

int CHARACTER::CountEmptyInventory() const
{
	int	count = 0;

	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (GetInventoryItem(i))
			count += GetInventoryItem(i)->GetSize();

	return (INVENTORY_MAX_NUM - count);
}

void TransformRefineItem(LPITEM pkOldItem, LPITEM pkNewItem)
{
	// ACCESSORY_REFINE
	if (pkOldItem->IsAccessoryForSocket())
	{
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			pkNewItem->SetSocket(i, pkOldItem->GetSocket(i));
		}
		//pkNewItem->StartAccessorySocketExpireEvent();
	}
	// END_OF_ACCESSORY_REFINE
	else
	{

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			if (!pkOldItem->GetSocket(i))
				break;
			else
				pkNewItem->SetSocket(i, 1);
		}

		int slot = 0;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			long socket = pkOldItem->GetSocket(i);

			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				pkNewItem->SetSocket(slot++, socket);
		}
	}

	pkOldItem->CopyAttributeTo(pkNewItem);

	#ifdef ENABLE_EVOLUTION_SYSTEM
	pkNewItem->SetEvolution(pkOldItem->GetEvolution());
	#endif

	#ifdef ENABLE_CHANGELOOK_SYSTEM
	pkNewItem->SetTransmutation(pkOldItem->GetTransmutation());
	#endif

	#ifdef ENABLE_LOCKED_SYSTEM
	pkNewItem->SetLocked(pkOldItem->GetLocked());
	#endif
}

void NotifyRefineSuccess(LPCHARACTER ch, LPITEM item, const char* way)
{
	if (NULL != ch && item != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineSuceeded");
		#ifdef ENABLE_SMITH_EFFECT_SYSTEM
		if (!ch->IsGM()) { ch->EffectPacket(SE_FR_SUCCESS); }
		#endif
		LogManager::instance().RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetRefineLevel(), 1, way);
	}
}

void NotifyRefineFail(LPCHARACTER ch, LPITEM item, const char* way, int success = 0)
{
	if (NULL != ch && NULL != item)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineFailed");
		#ifdef ENABLE_SMITH_EFFECT_SYSTEM
		if (!ch->IsGM()) { ch->EffectPacket(SE_FR_FAIL); }
		#endif
		LogManager::instance().RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetRefineLevel(), success, way);
	}
}

void CHARACTER::SetRefineNPC(LPCHARACTER ch)
{
	if ( ch != NULL )
	{
		m_dwRefineNPCVID = ch->GetVID();
	}
	else
	{
		m_dwRefineNPCVID = 0;
	}
}

bool CHARACTER::DoRefine(LPITEM item, bool bMoneyOnly)
{
	if (g_bReloadStatus) { return false; }
	if (IsWarpStatus()) { return false; }

	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	if (quest::CQuestManager::instance().GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::instance().GetEventFlag("update_refine_time") + (60 * 5))
		{
			sys_log(0, "can't refine %d %s", GetPlayerID(), GetName());
			return false;
		}
	}

	const TRefineTable * prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());
	if (!prt)
		return false;

	DWORD result_vnum = item->GetRefinedVnum();

	// REFINE_COST
	#ifdef ENABLE_FULL_YANG
	int64_t cost = ComputeRefineFee(prt->cost);
	#else
	int cost = ComputeRefineFee(prt->cost);
	#endif

	int RefineChance = GetQuestFlag("main_quest_lv7.refine_chance");
	if (RefineChance > 0)
	{
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹«·á °³·® ±âÈ¸´Â 20 ÀÌÇÏÀÇ ¹«±â¸¸ °¡´ÉÇÕ´Ï´Ù"));
			return false;
		}

		cost = 0;
		SetQuestFlag("main_quest_lv7.refine_chance", RefineChance - 1);
	}
	// END_OF_REFINE_COST

	if (result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
		return false;

	TItemTable * pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefine NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	// REFINE_COST
	if (GetGold() < cost)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³·®À» ÇÏ±â À§ÇÑ µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù."));
		return false;
	}

	if (!bMoneyOnly && !RefineChance)
	{
		item->SetRefine(true);

		for (int i = 0; i < prt->material_count; ++i)
		{
			if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
			{
				if (test_server)
				{
					ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
				}
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³·®À» ÇÏ±â À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
				return false;
			}
		}

		for (int i = 0; i < prt->material_count; ++i)
			RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);
	}

	int prob = number(1, 100);

	if (IsRefineThroughGuild() || bMoneyOnly)
		prob -= 10;

	#ifdef ENABLE_REFINE_LUCK
	if (GetRefineLuck())
		prob -= 5;

	if (prob <= 0)
		prob = 0;

	if (IsGM())
		ChatPacket(CHAT_TYPE_INFO, "DoRefine protodaki oran %d gecme sans %d", prt->prob, prob);
	#endif

	item->SetRefine(false);

	if (prob <= prt->prob)
	{
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);
		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE SUCCESS", pkNewItem->GetName());

			BYTE bCell = item->GetCell();

			// DETAIL_REFINE_LOG
			NotifyRefineSuccess(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");
			// END_OF_DETAIL_REFINE_LOG

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

			#ifdef ENABLE_REWARD_SYSTEM
			CHARACTER_MANAGER::Instance().SetRewardData(REWARD_REFINE, GetName(), true, result_vnum, 1);
			#endif

			sys_log(0, "Refine Success %d", cost);
			pkNewItem->AttrLog();

			sys_log(0, "PayPee %d", cost);
			PayRefineFee(cost);
			sys_log(0, "PayPee End %d", cost);

			#ifdef ENABLE_BATTLE_PASS
			uint8_t bBattlePassId = GetBattlePassId();
			if (bBattlePassId)
			{
				CBattlePass::instance().BattlePassCheckMissions(this, bBattlePassId, BP_MT_PLAYER_PLAY_TIME, item->GetVnum());
			}
			#endif
		}
		else
		{
			// DETAIL_REFINE_LOG
			sys_err("cannot create item %u", result_vnum);
			NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			// END_OF_DETAIL_REFINE_LOG
		}
	}
	else
	{
		DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
		NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
		item->AttrLog();
		ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");
		PayRefineFee(cost);
	}

	return true;
}

enum enum_RefineScrolls
{
	CHUKBOK_SCROLL	= 0,
	HYUNIRON_CHN	= 1,
	YONGSIN_SCROLL	= 2,
	MUSIN_SCROLL	= 3,
	YAGONG_SCROLL	= 4,
	MEMO_SCROLL		= 5,
	BDRAGON_SCROLL	= 6,
	#ifdef ENABLE_RITUALS_SCROLL
	RITUALS_SCROLL	= 11,
	#endif
};

bool CHARACTER::DoRefineWithScroll(LPITEM item)
{
	if (g_bReloadStatus) { return false; }
	if (IsWarpStatus()) { return false; }

	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	ClearRefineMode();

	if (quest::CQuestManager::instance().GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::instance().GetEventFlag("update_refine_time") + (60 * 5))
		{
			sys_log(0, "can't refine %d %s", GetPlayerID(), GetName());
			return false;
		}
	}

	const TRefineTable * prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());
	if (!prt)
		return false;

	LPITEM pkItemScroll;

	if (m_iRefineAdditionalCell < 0)
		return false;

	pkItemScroll = GetInventoryItem(m_iRefineAdditionalCell);

	if (!pkItemScroll)
		return false;

	if (!(pkItemScroll->GetType() == ITEM_USE && pkItemScroll->GetSubType() == USE_TUNING))
		return false;

	if (pkItemScroll->GetVnum() == item->GetVnum())
		return false;

	DWORD result_vnum = item->GetRefinedVnum();
	DWORD result_fail_vnum = item->GetRefineFromVnum();

	if (result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	// MUSIN_SCROLL
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL)
	{
		if (item->GetRefineLevel() >= 4)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ °³·®¼­·Î ´õ ÀÌ»ó °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}
	// END_OF_MUSIC_SCROLL

	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		if (item->GetRefineLevel() != pkItemScroll->GetValue(1))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ °³·®¼­·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		if (item->GetType() != ITEM_METIN || item->GetRefineLevel() != 4)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}
	#ifdef ENABLE_RITUALS_SCROLL
	else if (pkItemScroll->GetValue(0) == RITUALS_SCROLL)
	{
		if (item->GetLevelLimit() < pkItemScroll->GetValue(1))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("you can only upgrade items above level %d"), pkItemScroll->GetValue(1));
			return false;
		}
	}
	#endif

	TItemTable * pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefineWithScroll NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (GetGold() < prt->cost)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³·®À» ÇÏ±â À§ÇÑ µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù."));
		return false;
	}

	item->SetRefine(true);
	
	// 18.04.2025 Refine scroll as material fix -Tunga 
	for (int i = 0; i < prt->material_count; ++i)
	{
		bool isScrollMaterial = (pkItemScroll->GetVnum() == prt->materials[i].vnum);
		int materialCount = CountSpecifyItem(prt->materials[i].vnum);

		if (isScrollMaterial)
			materialCount += 1;
		
		if (materialCount < prt->materials[i].count)
		{
			if (test_server)
			{
				ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
			}
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³·®À» ÇÏ±â À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
			return false;
		}
		int countToRemove = prt->materials[i].count;
		if (isScrollMaterial)
			countToRemove--;
		if (countToRemove > 0)
			RemoveSpecifyItem(prt->materials[i].vnum, countToRemove);
	}

	int prob = number(1, 100);

	#ifdef ENABLE_REFINE_LUCK
	if (GetRefineLuck())
		prob -= 5;

	if (prob <= 0)
		prob = 0;

	if (IsGM())
		ChatPacket(CHAT_TYPE_INFO, "DoRefineWithScroll protodaki oran %d gecme sans %d", prt->prob, prob);
	#endif

	int success_prob = prt->prob;
	bool bDestroyWhenFail = false;

	const char* szRefineType = "SCROLL";

	if (pkItemScroll->GetValue(0) == HYUNIRON_CHN || 
		pkItemScroll->GetValue(0) == YONGSIN_SCROLL || 
		pkItemScroll->GetValue(0) == YAGONG_SCROLL
		#ifdef ENABLE_RITUALS_SCROLL
		|| pkItemScroll->GetValue(0) == RITUALS_SCROLL
		#endif
		)
	{
		const char hyuniron_prob[9] = { 100, 75, 65, 55, 45, 40, 35, 25, 20 };
		const char yagong_prob[9] = { 100, 100, 90, 80, 70, 60, 50, 30, 20 };

		if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			success_prob = hyuniron_prob[MINMAX(0, item->GetRefineLevel(), 8)];
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			success_prob = yagong_prob[MINMAX(0, item->GetRefineLevel(), 8)];
		}
		else if (pkItemScroll->GetValue(0) == HYUNIRON_CHN) {} // @fixme121
		#ifdef ENABLE_RITUALS_SCROLL
		else if (pkItemScroll->GetValue(0) == RITUALS_SCROLL) {} 
		#endif
		else
		{
			sys_err("REFINE : Unknown refine scroll item. Value0: %d", pkItemScroll->GetValue(0));
		}

		if (test_server)
		{
			ChatPacket(CHAT_TYPE_INFO, "[Only Test] Success_Prob %d, RefineLevel %d ", success_prob, item->GetRefineLevel());
		}

		#ifdef ENABLE_RITUALS_SCROLL
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN || pkItemScroll->GetValue(0) == RITUALS_SCROLL)
		#else
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN)
		#endif
			bDestroyWhenFail = true;

		// DETAIL_REFINE_LOG
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN)
		{
			szRefineType = "HYUNIRON";
		}
		else if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			szRefineType = "GOD_SCROLL";
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			szRefineType = "YAGONG_SCROLL";
		}
		#ifdef ENABLE_RITUALS_SCROLL
		else if (pkItemScroll->GetValue(0) == RITUALS_SCROLL)
		{
			success_prob += pkItemScroll->GetValue(2);
			szRefineType = "RITUALS_SCROLL";
		}
		#endif
		// END_OF_DETAIL_REFINE_LOG
	}

	// DETAIL_REFINE_LOG
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL)
	{
		success_prob = 100;
		szRefineType = "MUSIN_SCROLL";
	}
	// END_OF_DETAIL_REFINE_LOG
	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		success_prob = 100;
		szRefineType = "MEMO_SCROLL";
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		success_prob = 80;
		szRefineType = "BDRAGON_SCROLL";
	}

	pkItemScroll->SetCount(pkItemScroll->GetCount() - 1);

	item->SetRefine(false);

	if (prob <= success_prob)
	{
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);
		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE SUCCESS", pkNewItem->GetName());

			BYTE bCell = item->GetCell();

			NotifyRefineSuccess(this, item, szRefineType);
			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			pkNewItem->AttrLog();

			PayRefineFee(prt->cost);

			#ifdef ENABLE_REWARD_SYSTEM
			CHARACTER_MANAGER::Instance().SetRewardData(REWARD_REFINE, GetName(), true, result_vnum, 1);
			#endif

			#ifdef ENABLE_BATTLE_PASS
			uint8_t bBattlePassId = GetBattlePassId();
			if (bBattlePassId)
			{
				CBattlePass::instance().BattlePassCheckMissions(this, bBattlePassId, BP_MT_PLAYER_PLAY_TIME, item->GetVnum());
			}
			#endif
		}
		else
		{
			sys_err("cannot create item %u", result_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}
	else if (!bDestroyWhenFail && result_fail_vnum)
	{
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_fail_vnum, 1, 0, false);
		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE FAIL", pkNewItem->GetName());

			BYTE bCell = item->GetCell();

			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost);
			NotifyRefineFail(this, item, szRefineType, -1);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			pkNewItem->AttrLog();

			PayRefineFee(prt->cost);

			#ifdef ENABLE_BATTLE_PASS
			uint8_t bBattlePassId = GetBattlePassId();
			if (bBattlePassId)
			{
				CBattlePass::instance().BattlePassCheckMissions(this, bBattlePassId, BP_MT_REFINE_ITEM, item->GetVnum());
			}
			#endif
		}
		else
		{
			sys_err("cannot create item %u", result_fail_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}
	else
	{
		NotifyRefineFail(this, item, szRefineType);
		PayRefineFee(prt->cost);
	}

	return true;
}

bool CHARACTER::RefineInformation(BYTE bCell, BYTE bType, int iAdditionalCell)
{
	if (bCell > INVENTORY_MAX_NUM)
		return false;

	LPITEM item = GetInventoryItem(bCell);

	if (!item)
		return false;

	// REFINE_COST
	if (bType == REFINE_TYPE_MONEY_ONLY && !GetQuestFlag("deviltower_zone.can_refine"))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç±Í Å¸¿ö ¿Ï·á º¸»óÀº ÇÑ¹ø±îÁö »ç¿ë°¡´ÉÇÕ´Ï´Ù."));
		return false;
	}
	// END_OF_REFINE_COST

	TPacketGCRefineInformation p;

	p.header = HEADER_GC_REFINE_INFORMATION;
	p.pos = bCell;
	p.src_vnum = item->GetVnum();
	p.result_vnum = item->GetRefinedVnum();
	p.type = bType;

	if (p.result_vnum == 0)
	{
		sys_err("RefineInformation p.result_vnum == 0");
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
	{
		if (bType == 0)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº ÀÌ ¹æ½ÄÀ¸·Î´Â °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
		else
		{
			LPITEM itemScroll = GetInventoryItem(iAdditionalCell);
			if (!itemScroll || item->GetVnum() == itemScroll->GetVnum())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°°Àº °³·®¼­¸¦ ÇÕÄ¥ ¼ö´Â ¾ø½À´Ï´Ù."));
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ãàº¹ÀÇ ¼­¿Í ÇöÃ¶À» ÇÕÄ¥ ¼ö ÀÖ½À´Ï´Ù."));
				return false;
			}
		}
	}

	CRefineManager & rm = CRefineManager::instance();

	const TRefineTable* prt = rm.GetRefineRecipe(item->GetRefineSet());
	if (!prt)
	{
		sys_err("RefineInformation NOT GET REFINE SET %d", item->GetRefineSet());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}
	// REFINE_COST

	//MAIN_QUEST_LV7
	if (GetQuestFlag("main_quest_lv7.refine_chance") > 0)
	{
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹«·á °³·® ±âÈ¸´Â 20 ÀÌÇÏÀÇ ¹«±â¸¸ °¡´ÉÇÕ´Ï´Ù"));
			return false;
		}
		p.cost = 0;
	}
	else
		p.cost = ComputeRefineFee(prt->cost);

	//END_MAIN_QUEST_LV7
	p.prob = prt->prob;
	if (bType == REFINE_TYPE_MONEY_ONLY)
	{
		p.material_count = 0;
		memset(p.materials, 0, sizeof(p.materials));
	}
	else
	{
		p.material_count = prt->material_count;
		thecore_memcpy(&p.materials, prt->materials, sizeof(prt->materials));
	}
	// END_OF_REFINE_COST

	GetDesc()->Packet(&p, sizeof(TPacketGCRefineInformation));

	//PREVENT_REFINE_WINDOW
	SetOpenRefine(true);
	//END_PREVENT_REFINE_WINDOW

	SetRefineMode(iAdditionalCell);
	return true;
}

bool CHARACTER::RefineItem(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
		return false;

	if (pkItem->GetSubType() == USE_TUNING)
	{
		//PREVENT_REFINE_WINDOW
		SetOpenRefine(true);
		//END_PREVENT_REFINE_WINDOW

		// MUSIN_SCROLL
		if (pkItem->GetValue(0) == MUSIN_SCROLL)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_MUSIN, pkItem->GetCell());
		// END_OF_MUSIN_SCROLL
		else if (pkItem->GetValue(0) == HYUNIRON_CHN)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_HYUNIRON, pkItem->GetCell());
		else if (pkItem->GetValue(0) == BDRAGON_SCROLL)
		{
			if (pkTarget->GetRefineSet() != 702) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_BDRAGON, pkItem->GetCell());
		}
		#ifdef ENABLE_RITUALS_SCROLL
		else if (pkItem->GetValue(0) == RITUALS_SCROLL)
		{
			if (pkTarget->GetLevelLimit() <= pkItem->GetValue(1)) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_RITUALS_SCROLL, pkItem->GetCell());
		}
		#endif
		else
		{
			if (pkTarget->GetRefineSet() == 501) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell());
		}
	}
	else if (pkItem->GetSubType() == USE_DETACHMENT && IS_SET(pkTarget->GetFlag(), ITEM_FLAG_REFINEABLE))
	{
		LogManager::instance().ItemLog(this, pkTarget, "USE_DETACHMENT", pkTarget->GetName());

		bool bHasMetinStone = false;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		{
			long socket = pkTarget->GetSocket(i);
			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
			{
				bHasMetinStone = true;
				break;
			}
		}

		if (bHasMetinStone)
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				long socket = pkTarget->GetSocket(i);
				if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				{
					AutoGiveItem(socket);
					pkTarget->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
				}
			}
			pkItem->SetCount(pkItem->GetCount() - 1);
			return true;
		}
		else
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»©³¾ ¼ö ÀÖ´Â ¸ÞÆ¾¼®ÀÌ ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	return false;
}

EVENTFUNC(kill_campfire_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("kill_campfire_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}
	ch->m_pkMiningEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

#ifdef ENABLE_MULTI_FARM_BLOCK
EVENTFUNC(multi_farm_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("multi_farm_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (ch == NULL) { return 0; }// <Factor>
	if (!ch->GetDesc()) { return 0; }
	ch->m_pkCheckMultiFarmEvent = NULL;
	CHARACTER_MANAGER::Instance().CheckMultiFarmAccount(ch->GetDesc()->GetHostName(), ch->GetPlayerID(), ch->GetName(), !ch->GetRewardStatus());
	return 0;
}
#endif

#ifdef ENABLE_MULTI_HUNT_BLOCK
EVENTFUNC(multi_hunt_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("multi_hunt_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (ch == NULL) { return 0; }// <Factor>
	if (!ch->GetDesc()) { return 0; }
	ch->m_pkCheckMultiHuntEvent = NULL;
	CHARACTER_MANAGER::Instance().CheckMultiHuntAccount(ch->GetDesc()->GetHostName(), ch->GetPlayerID(), ch->GetName(), !ch->GetMultiHuntStatus());
	return 0;
}
#endif

#ifdef ENABLE_MULTI_FISH_BLOCK
EVENTFUNC(multi_fish_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("multi_fish_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (ch == NULL) { return 0; }// <Factor>
	if (!ch->GetDesc()) { return 0; }
	ch->m_pkCheckMultiFishEvent = NULL;
	CHARACTER_MANAGER::Instance().CheckMultiFishAccount(ch->GetDesc()->GetHostName(), ch->GetPlayerID(), ch->GetName(), !ch->GetMultiFishStatus());
	return 0;
}
#endif

bool CHARACTER::GiveRecallItem(LPITEM item)
{
	int idx = GetMapIndex();
	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±â¾ïÇØ µÑ ¼ö ¾ø´Â À§Ä¡ ÀÔ´Ï´Ù."));
		return false;
	}

	int pos;

	if (item->GetCount() == 1)
	{
		item->SetSocket(0, GetX());
		item->SetSocket(1, GetY());
	}
	else if ((pos = GetEmptyInventory(item->GetSize())) != -1)
	{
		LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), 1);

		/*if (NULL != item2)
		{
			item2->SetSocket(0, GetX());
			item2->SetSocket(1, GetY());
			item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

			item->SetCount(item->GetCount() - 1);
		}*/

		if (NULL != item2)//MartyFixed
		{
			item->SetCount(item->GetCount() - 1);
			item2->SetSocket(0, GetX());
			item2->SetSocket(1, GetY());

			AutoGiveItem(item2);
		}
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
		return false;
	}

	return true;
}

void CHARACTER::ProcessRecallItem(LPITEM item)
{
	int idx;

	if ((idx = SECTREE_MANAGER::instance().GetMapIndex(item->GetSocket(0), item->GetSocket(1))) == 0)
		return;

	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;

		case 301:
		case 302:
		case 303:
		case 304:
			if ( GetLevel() < 90 )
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛÀÇ ·¹º§ Á¦ÇÑº¸´Ù ·¹º§ÀÌ ³·½À´Ï´Ù."));
				return;
			}
			else
				break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±â¾ïµÈ À§Ä¡°¡ Å¸Á¦±¹¿¡ ¼ÓÇØ ÀÖ¾î¼­ ±ÍÈ¯ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		item->SetSocket(0, 0);
		item->SetSocket(1, 0);
	}
	else
	{
		sys_log(1, "Recall: %s %d %d -> %d %d", GetName(), GetX(), GetY(), item->GetSocket(0), item->GetSocket(1));
		WarpSet(item->GetSocket(0), item->GetSocket(1));
		item->SetCount(item->GetCount() - 1);
	}
}

void CHARACTER::__OpenPrivateShop()
{
	#ifdef ENABLE_OPEN_SHOP_WITH_ARMOR
	ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
	#else
	unsigned bodyPart = GetPart(PART_MAIN);
	switch (bodyPart)
	{
		case 0:
		case 1:
		case 2:
			ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
			break;
		default:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°©¿ÊÀ» ¹þ¾î¾ß °³ÀÎ »óÁ¡À» ¿­ ¼ö ÀÖ½À´Ï´Ù."));
			break;
	}
	#endif
}

#ifdef ENABLE_FULL_YANG
void CHARACTER::SendMyShopPriceListCmd(DWORD dwItemVnum, int64_t dwItemPrice)
{
	char szLine[256];
	snprintf(szLine, sizeof(szLine), "MyShopPriceList %u %lld", dwItemVnum, dwItemPrice);
	ChatPacket(CHAT_TYPE_COMMAND, szLine);
	sys_log(0, szLine);
}
#else
void CHARACTER::SendMyShopPriceListCmd(DWORD dwItemVnum, DWORD dwItemPrice)
{
	char szLine[256];
	snprintf(szLine, sizeof(szLine), "MyShopPriceList %u %u", dwItemVnum, dwItemPrice);
	ChatPacket(CHAT_TYPE_COMMAND, szLine);
	sys_log(0, szLine);
}
#endif

void CHARACTER::UseSilkBotaryReal(const TPacketMyshopPricelistHeader* p)
{
	const TItemPriceInfo* pInfo = (const TItemPriceInfo*)(p + 1);

	if (!p->byCount)

		SendMyShopPriceListCmd(1, 0);
	else {
		for (int idx = 0; idx < p->byCount; idx++)
			SendMyShopPriceListCmd(pInfo[ idx ].dwVnum, pInfo[ idx ].dwPrice);
	}

	__OpenPrivateShop();
}
//

//
void CHARACTER::UseSilkBotary(void)
{
	if (m_bNoOpenedShop)
	{
		DWORD dwPlayerID = GetPlayerID();
		db_clientdesc->DBPacket(HEADER_GD_MYSHOP_PRICELIST_REQ, GetDesc()->GetHandle(), &dwPlayerID, sizeof(DWORD));
		m_bNoOpenedShop = false;
	}
	else
	{
		__OpenPrivateShop();
	}
}
// END_OF_MYSHOP_PRICE_LIST

int CalculateConsume(LPCHARACTER ch)
{
	static const int WARP_NEED_LIFE_PERCENT	= 30;
	static const int WARP_MIN_LIFE_PERCENT	= 10;
	// CONSUME_LIFE_WHEN_USE_WARP_ITEM
	int consumeLife = 0;
	{
		// CheckNeedLifeForWarp
		const int curLife		= ch->GetHP();
		const int needPercent	= WARP_NEED_LIFE_PERCENT;
		const int needLife = ch->GetMaxHP() * needPercent / 100;
		if (curLife < needLife)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("³²Àº »ý¸í·Â ¾çÀÌ ¸ðÀÚ¶ó »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return -1;
		}

		consumeLife = needLife;

		const int minPercent	= WARP_MIN_LIFE_PERCENT;
		const int minLife	= ch->GetMaxHP() * minPercent / 100;
		if (curLife - needLife < minLife)
			consumeLife = curLife - minLife;

		if (consumeLife < 0)
			consumeLife = 0;
	}
	// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM
	return consumeLife;
}

int CalculateConsumeSP(LPCHARACTER lpChar)
{
	static const int NEED_WARP_SP_PERCENT = 30;

	const int curSP = lpChar->GetSP();
	const int needSP = lpChar->GetMaxSP() * NEED_WARP_SP_PERCENT / 100;

	if (curSP < needSP)
	{
		lpChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("³²Àº Á¤½Å·Â ¾çÀÌ ¸ðÀÚ¶ó »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return -1;
	}

	return needSP;
}

// #define ENABLE_FIREWORK_STUN
#define ENABLE_ADDSTONE_FAILURE
bool CHARACTER::UseItemEx(LPITEM item, TItemPos DestCell)
{
	if (!item) { return false; }

	int iLimitRealtimeStartFirstUseFlagIndex = -1;
	//int iLimitTimerBasedOnWearFlagIndex = -1;

	WORD wDestCell = DestCell.cell;
	BYTE bDestInven = DestCell.window_type;

	#ifdef ENABLE_VSTURNUVA
	if (GetMapIndex() == 28)
	{
		if (item->IsCostumeMount())
			return false;

		if (GetJob() == JOB_ASSASSIN)
		{
			if (item->GetType() == ITEM_WEAPON && item->GetSubType() == WEAPON_SWORD)
				return false;
		}
	}

	if ((GetMapIndex() == 28) || GetWarMap())
	{
		switch (item->GetVnum())
		{
			case 50816:
			case 39011:
			case 39012:
			case 39013:
			case 71018:
			case 71019:
			case 71020:
			case 27863:
			case 27865:
			case 27869:
			case 27874:
			case 27875:
			case 27878:
				return false;
		}
	}
	#endif

	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limitValue = item->GetProto()->aLimits[i].lValue;

		switch (item->GetProto()->aLimits[i].bType)
		{
			case LIMIT_LEVEL:
				if (GetLevel() < limitValue)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛÀÇ ·¹º§ Á¦ÇÑº¸´Ù ·¹º§ÀÌ ³·½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_REAL_TIME_START_FIRST_USE:
				iLimitRealtimeStartFirstUseFlagIndex = i;
				break;

			case LIMIT_TIMER_BASED_ON_WEAR:
				//iLimitTimerBasedOnWearFlagIndex = i;
				break;
		}
	}

	if (test_server)
	{
		sys_log(0, "USE_ITEM %d %s, Inven %d, Cell %d, DestInven %d, DestCell %d, ItemType %d, SubType %d", item->GetVnum(), item->GetName(), item->GetWindow(), item->GetCell(), bDestInven, wDestCell, item->GetType(), item->GetSubType());
	}

	if ( CArenaManager::instance().IsLimitedItem( GetMapIndex(), item->GetVnum() ) == true )
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
		return false;
	}
	#ifdef ENABLE_NEWSTUFF
	else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && IsLimitedPotionOnPVP(item->GetVnum()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
		return false;
	}
	#endif

	// @fixme402 (IsLoadedAffect to block affect hacking)
	if (!IsLoadedAffect())
	{
		ChatPacket(CHAT_TYPE_INFO, "Affects are not loaded yet!");
		return false;
	}

	// @fixme141 BEGIN
	if (TItemPos(item->GetWindow(), item->GetCell()).IsBeltInventoryPosition())
	{
		LPITEM beltItem = GetWear(WEAR_BELT);

		if (NULL == beltItem)
		{
			ChatPacket(CHAT_TYPE_INFO, "<Belt> You can't use this item if you have no equipped belt.");
			return false;
		}

		if (false == CBeltInventoryHelper::IsAvailableCell(item->GetCell() - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
		{
			ChatPacket(CHAT_TYPE_INFO, "<Belt> You can't use this item if you don't upgrade your belt.");
			return false;
		}
	}
	// @fixme141 END

	if (-1 != iLimitRealtimeStartFirstUseFlagIndex)
	{
		if (0 == item->GetSocket(1))
		{
			long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[iLimitRealtimeStartFirstUseFlagIndex].lValue;
			if (0 == duration)
				duration = 60 * 60 * 24 * 7;

			item->SetSocket(0, time(0) + duration);
			item->StartRealTimeExpireEvent();
		}

		if (false == item->IsEquipped())
			item->SetSocket(1, item->GetSocket(1) + 1);
	}

	#ifdef ENABLE_BATTLE_PASS
	uint8_t bBattlePassId = GetBattlePassId();
	if (bBattlePassId)
	{
		CBattlePass::instance().BattlePassCheckMissions(this, bBattlePassId, BP_MT_USE_ITEM, item->GetVnum());
	}
	#endif

	switch (item->GetType())
	{
		case ITEM_HAIR:
			return ItemProcess_Hair(item, wDestCell);

		case ITEM_POLYMORPH:
			return ItemProcess_Polymorph(item);

		case ITEM_QUEST:
			{
				if (GetExchange()) { return false; }

				#ifdef ENABLE_IKASHOP_RENEWAL
				if (GetIkarusShopGuest()
					|| GetIkarusAuctionGuest()
					|| GetIkarusShopSafebox()
					|| IsLookingShopOwner()
					|| IsLookingIkarusShopOfferList()
					|| IsLookingAuctionOwner()
				)
				{
					return false;
				}
				#endif

				#ifdef __AUTO_HUNT__
				if (item->GetVnum() == 38014)
				{
					if (FindAffect(AFFECT_AUTO_HUNT))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Zaten aktif."));
						return false;
					}

					AddAffect(AFFECT_AUTO_HUNT, POINT_NONE, 0, AFF_NONE, 60 * 60 * 24 * item->GetValue(0), 0, false);
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Etkisi %d gunu icin eklendi."), item->GetValue(0));
					item->SetCount(item->GetCount() - 1);
					return true;
				}
				#endif

				#ifdef __RENEWAL_BRAVE_CAPE__
				if (item->GetVnum() >= 20570 && item->GetVnum() <= 20575)
				{
					if (FindAffect(AFFECT_BRAVE_CAPE))
					{
						ChatPacket(CHAT_TYPE_INFO, "<Sistem> Zaten bu etkiye sahipsiniz!");
						return false;
					}

					const int timeAdd = item->GetValue(0) > 30 ? INFINITE_AFFECT_DURATION : 60 * 60 * 24 * item->GetValue(0);
					if (!timeAdd)
					{
						ChatPacket(CHAT_TYPE_INFO, "<Sistem> Oyun yetkilisiyle iletisime gecin. Bu ogenin zaman verileri yok!");
						return false;
					}

					AddAffect(AFFECT_BRAVE_CAPE, POINT_NONE, 0, AFF_NONE, timeAdd, 0, 0, false);
					item->SetCount(item->GetCount() - 1);
					return true;
				}
				#endif

				#ifdef ENABLE_QUEST_DND_EVENT
				if (IS_SET(item->GetFlag(), ITEM_FLAG_APPLICABLE))
				{
					LPITEM item2;

					if (!GetItem(DestCell) || !(item2 = GetItem(DestCell)))
						return false;

					if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
						return false;

					quest::CQuestManager::instance().DND(GetPlayerID(), item, item2, false);
					return true;
				}
				#endif

				if (GetArena() != NULL || IsObserverMode() == true)
				{
					if (item->GetVnum() == 50051 || item->GetVnum() == 50052 || item->GetVnum() == 50053)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
						return false;
					}
				}

				if (!IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE | ITEM_FLAG_QUEST_USE_MULTIPLE))
				{
					if (item->GetSIGVnum() == 0)
					{
						quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
					}
					else
					{
						quest::CQuestManager::instance().SIGUse(GetPlayerID(), item->GetSIGVnum(), item, false);
					}
				}
			}
			break;

		case ITEM_CAMPFIRE:
			{
				if (CWarMapManager::instance().IsWarMap(GetMapIndex()))
					return false;

				if (GetProtectTime("campates") > get_global_time())
				{
					ChatPacket(CHAT_TYPE_INFO, "Yavaþ olmalýsýn! %d saniye sonra deneyebilirsiniz.", GetProtectTime("campates") - get_global_time());
					return false;
				}

				float fx, fy;
				GetDeltaByDegree(GetRotation(), 100.0f, &fx, &fy);

				LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), (long)(GetX()+fx), (long)(GetY()+fy));
				if (!tree)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸ð´ÚºÒÀ» ÇÇ¿ï ¼ö ¾ø´Â ÁöÁ¡ÀÔ´Ï´Ù."));
					return false;
				}

				if (tree->IsAttr((long)(GetX()+fx), (long)(GetY()+fy), ATTR_WATER))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹° ¼Ó¿¡ ¸ð´ÚºÒÀ» ÇÇ¿ï ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}

				LPCHARACTER campfire = CHARACTER_MANAGER::instance().SpawnMob(fishing::CAMPFIRE_MOB, GetMapIndex(), (long)(GetX()+fx), (long)(GetY()+fy), 0, false, number(0, 359));

				char_event_info* info = AllocEventInfo<char_event_info>();
				info->ch = campfire;
				campfire->m_pkMiningEvent = event_create(kill_campfire_event, info, PASSES_PER_SEC(40));

				item->SetCount(item->GetCount() - 1);

				SetProtectTime("campates", get_global_time() + 60);
			}
			break;

		case ITEM_UNIQUE:
			{
				switch (item->GetSubType())
				{
					case USE_ABILITY_UP:
						{
							switch (item->GetValue(0))
							{
								case APPLY_MOV_SPEED:
									if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED))// @fixme171
										return false;
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;

								case APPLY_ATT_SPEED:
									if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_SPEED))// @fixme171
										return false;
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;

								case APPLY_STR:
									if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_ST))// @fixme171
										return false;
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_DEX:
									if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_DX))// @fixme171
										return false;
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CON:
									if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_HT))// @fixme171
										return false;
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_INT:
									if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_IQ))// @fixme171
										return false;
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CAST_SPEED:
									if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_CASTING_SPEED))// @fixme171
										return false;
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_RESIST_MAGIC:
									if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_RESIST_MAGIC))// @fixme171
										return false;
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_RESIST_MAGIC, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_ATT_GRADE_BONUS:
									if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_GRADE_BONUS))// @fixme171
										return false;
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_DEF_GRADE_BONUS:
									if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_DEF_GRADE_BONUS))// @fixme171
										return false;
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DEF_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					default:
						{
							if (item->GetSubType() == USE_SPECIAL)
							{
								sys_log(0, "ITEM_UNIQUE: USE_SPECIAL %u", item->GetVnum());

								switch (item->GetVnum())
								{
									case 71049:
										if (g_bEnableBootaryCheck)
										{
											if (IS_BOTARYABLE_ZONE(GetMapIndex()) == true)
											{
												UseSilkBotary();
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³ÀÎ »óÁ¡À» ¿­ ¼ö ¾ø´Â Áö¿ªÀÔ´Ï´Ù"));
											}
										}
										else
										{
											UseSilkBotary();
										}
										break;
								}
							}
							#ifdef ENABLE_USE_STACK_UNIQUE_ITEM
							else if (item->GetVnum() == UNIQUE_ITEM_DOUBLE_EXP
									|| item->GetVnum() == UNIQUE_ITEM_DOUBLE_ITEM
							)
							{
								if (item->GetCount() == 1)
								{
									if (!item->IsEquipped())
										EquipItem(item);
									else
										UnequipItem(item);
									return true;
								}
								else if (item->GetCount() > 1)
								{
									if ((GetWear(WEAR_UNIQUE1) != NULL) && (GetWear(WEAR_UNIQUE2) != NULL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Yeterli bosluk yok!"));
										return false;
									}

									if (false == CanEquipNow(item))
										return false;

									LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), 1);
									if (!pkItem) { return false; }
									if (pkItem->IsEquipped()) { return false; }
									EquipItem(pkItem);
									ITEM_MANAGER::instance().FlushDelayedSave(pkItem);

									item->SetCount(item->GetCount()-1);
									return true;
								}
							}
							#endif
							else
							{
								if (!item->IsEquipped())
									EquipItem(item);
								else
									UnequipItem(item);
							}
						}
						break;
				}
			}
			break;

		case ITEM_COSTUME:
		case ITEM_WEAPON:
		case ITEM_ARMOR:
		case ITEM_ROD:
		case ITEM_RING:
		case ITEM_BELT:
		case ITEM_PICK:
			if (!item->IsEquipped())
			{
				#ifdef ENABLE_COSTUME_PET_SYSTEM
				if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_PET)
				{
					if (GetPetSystem() && GetPetSystem()->CountSummoned() > 0)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Already summoned pet!"));
						return false;
					}
				}
				#endif

				EquipItem(item);
			}
			else
				UnequipItem(item);
			break;

// #ifdef ENABLE_PET_SYSTEM_EX
		// case ITEM_PET:
			// switch (item->GetSubType())
			// {
				// case PET_UPBRINGING: //1
				// case PET_PAY: //12
				// {
					// auto mobVnum = item->GetValue(0);
					// if (!mobVnum)
					// {
						// ChatPacket(CHAT_TYPE_INFO, "Item Pet %d with no mob in value0", item->GetVnum());
						// return false;
					// }
					// auto * petSystem = GetPetSystem();
					// if (!petSystem)
						// return false;

					// if (petSystem->CountSummoned())
						// petSystem->UnsummonAll();
					// else
					// {
						// summon
						// constexpr bool bFromFar = false;
						// const auto * pkMob = CMobManager::instance().Get(mobVnum);
						// const auto * petName = (pkMob) ? pkMob->GetLocaleName() : "";
						// petSystem->Summon(mobVnum, item, petName, bFromFar);
						// spawn effect
						// constexpr const char * spawn_effect_file_name = "d:\\ymir work\\effect\\etc\\appear_die\\npc2_appear.mse";
						// auto * petActor = petSystem->GetByVnum(mobVnum);
						// if (petActor && petActor->GetCharacter() && item->GetSubType() == PET_PAY)
							// petActor->GetCharacter()->SpecificEffectPacket(spawn_effect_file_name);
					// }
				// }
				// break;
// #endif

				// case PET_EGG: //0
				// case PET_BAG: //2
				// case PET_FEEDSTUFF: //3
				// case PET_SKILL: //4
				// case PET_SKILL_DEL_BOOK: //5
				// case PET_NAME_CHANGE: //6
				// case PET_EXPFOOD: //7
				// case PET_SKILL_ALL_DEL_BOOK: //8
				// case PET_EXPFOOD_PER: //9
				// case PET_ATTR_DETERMINE: //10
				// case PET_ATTR_CHANGE: //11
				// case PET_PRIMIUM_FEEDSTUFF: //13
					// break;
			// }
			// break;

		case ITEM_DS:
			{
				if (!item->IsEquipped())
					return false;
				return DSManager::instance().PullOut(this, NPOS, item);
			break;
			}
		case ITEM_SPECIAL_DS:
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;

		case ITEM_FISH:
			{
				if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
					return false;
				}
#ifdef ENABLE_NEWSTUFF
				else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
					return false;
				}
#endif

				if (item->GetSubType() == FISH_ALIVE)
					fishing::UseFish(this, item);
			}
			break;

		case ITEM_TREASURE_BOX:
			{
				return false;
			}
			break;

		case ITEM_TREASURE_KEY:
			{
				LPITEM item2;

				if (!GetItem(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					return false;

				if (item2->GetType() != ITEM_TREASURE_BOX)
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¿­¼è·Î ¿©´Â ¹°°ÇÀÌ ¾Æ´Ñ°Í °°´Ù."));
					return false;
				}

				if (item->GetValue(0) == item2->GetValue(0))
				{
					DWORD dwBoxVnum = item2->GetVnum();
					std::vector <DWORD> dwVnums;
					std::vector <DWORD> dwCounts;
					std::vector <LPITEM> item_gets(0);
					int count = 0;

					if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count
						#ifdef ENABLE_LOCKED_SYSTEM
						, item2->GetLocked()
						#endif
					))
					{
						item->SetCount(item->GetCount()-1);
						item2->SetCount(item2->GetCount()-1);

						for (int i = 0; i < count; i++)
						{
							switch (dwVnums[i])
							{
								case CSpecialItemGroup::GOLD:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
									break;
								case CSpecialItemGroup::EXP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ºÎÅÍ ½ÅºñÇÑ ºûÀÌ ³ª¿É´Ï´Ù."));
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%dÀÇ °æÇèÄ¡¸¦ È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
									break;
								case CSpecialItemGroup::MOB:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
									break;
								case CSpecialItemGroup::SLOW:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â »¡°£ ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ ¿òÁ÷ÀÌ´Â ¼Óµµ°¡ ´À·ÁÁ³½À´Ï´Ù!"));
									break;
								case CSpecialItemGroup::DRAIN_HP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ°¡ °©ÀÚ±â Æø¹ßÇÏ¿´½À´Ï´Ù! »ý¸í·ÂÀÌ °¨¼ÒÇß½À´Ï´Ù."));
									break;
								case CSpecialItemGroup::POISON:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
									break;
								#ifdef ENABLE_WOLFMAN_CHARACTER
								case CSpecialItemGroup::BLEEDING:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
									break;
								#endif
								case CSpecialItemGroup::MOB_GROUP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
									break;
								default:
									if (item_gets[i])
									{
										if (dwCounts[i] > 1)
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ %d °³ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(), dwCounts[i]);
										else
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName());
									}
							}
						}
					}
					else
					{
						ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¿­¼è°¡ ¸ÂÁö ¾Ê´Â °Í °°´Ù."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¿­¼è°¡ ¸ÂÁö ¾Ê´Â °Í °°´Ù."));
					return false;
				}
			}
			break;

		case ITEM_GIFTBOX:
			{
				#ifdef ENABLE_RELOAD_COMMANDS
				if (g_bReloadStatus)
				{
					sys_err("ITEM_GIFTBOX Suan reload cekiliyor...");
					return false;
				}
				#endif

#ifdef ENABLE_NEWSTUFF
				if (g_BoxUseTimeLimitValue && !PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::BoxOpening, std::chrono::milliseconds(g_BoxUseTimeLimitValue)))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ °ñµå¸¦ ¹ö¸± ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
#endif

				DWORD dwBoxVnum = item->GetVnum();
				std::vector <DWORD> dwVnums;
				std::vector <DWORD> dwCounts;
				std::vector <LPITEM> item_gets(0);
				int count = 0;

				if (dwBoxVnum > 51500 && dwBoxVnum < 52000)
				{
					if ( !(this->DragonSoul_IsQualified()) )
					{
						ChatPacket(CHAT_TYPE_INFO,LC_TEXT("¸ÕÀú ¿ëÈ¥¼® Äù½ºÆ®¸¦ ¿Ï·áÇÏ¼Å¾ß ÇÕ´Ï´Ù."));
						return false;
					}
				}

				if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count
					#ifdef ENABLE_LOCKED_SYSTEM
					, item->GetLocked()
					#endif
				))
				{
					item->SetCount(item->GetCount()-1);
					#ifdef __RANKING_SYSTEM__
					SetRankManuel(RANK_CHEST, 1);
					#endif

					for (int i = 0; i < count; i++)
					{
						switch (dwVnums[i])
						{
						case CSpecialItemGroup::GOLD:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
							break;
						case CSpecialItemGroup::EXP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ºÎÅÍ ½ÅºñÇÑ ºûÀÌ ³ª¿É´Ï´Ù."));
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%dÀÇ °æÇèÄ¡¸¦ È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
							break;
						case CSpecialItemGroup::MOB:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
							break;
						case CSpecialItemGroup::SLOW:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â »¡°£ ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ ¿òÁ÷ÀÌ´Â ¼Óµµ°¡ ´À·ÁÁ³½À´Ï´Ù!"));
							break;
						case CSpecialItemGroup::DRAIN_HP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ°¡ °©ÀÚ±â Æø¹ßÇÏ¿´½À´Ï´Ù! »ý¸í·ÂÀÌ °¨¼ÒÇß½À´Ï´Ù."));
							break;
						case CSpecialItemGroup::POISON:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
							break;
						#ifdef ENABLE_WOLFMAN_CHARACTER
						case CSpecialItemGroup::BLEEDING:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
							break;
						#endif
						case CSpecialItemGroup::MOB_GROUP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
							break;
						default:
							if (item_gets[i])
							{
								if (dwCounts[i] > 1)
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ %d °³ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(), dwCounts[i]);
								else
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName());
							}
						}
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¾Æ¹«°Íµµ ¾òÀ» ¼ö ¾ø¾ú½À´Ï´Ù."));
					return false;
				}
			}
			break;

		case ITEM_SKILLFORGET:
			{
				if (!item->GetSocket(0))
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					return false;
				}

				DWORD dwVnum = item->GetSocket(0);

				if (SkillLevelDown(dwVnum))
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("½ºÅ³ ·¹º§À» ³»¸®´Âµ¥ ¼º°øÇÏ¿´½À´Ï´Ù."));
				}
				else
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("½ºÅ³ ·¹º§À» ³»¸± ¼ö ¾ø½À´Ï´Ù."));
			}
			break;

		case ITEM_SKILLBOOK:
			{
				if (IsPolymorphed())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
					return false;
				}

				DWORD dwVnum = 0;

				if (item->GetVnum() == 50300)
				{
					dwVnum = item->GetSocket(0);
				}
				else
				{

					dwVnum = item->GetValue(0);
				}

				if (0 == dwVnum)
				{
					ITEM_MANAGER::instance().RemoveItem(item);

					return false;
				}

				if (true == LearnSkillByBook(dwVnum))
				{
					#ifdef ENABLE_BOOKS_STACKFIX
					item->SetCount(item->GetCount() - 1);
					#else
					ITEM_MANAGER::instance().RemoveItem(item);
					#endif

					int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

					if (distribution_test_server)
						iReadDelay /= 3;

					SetSkillNextReadTime(dwVnum, get_global_time() + iReadDelay);
				}
			}
			break;

		case ITEM_USE:
			{
				//18.04.2025 Toggle Potion -Tunga
				if (item->GetVnum() >= 84100 && item->GetVnum() <= 84105)
				{
					if (!PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::ItemAffectUse, std::chrono::seconds(3)))
					{
						ChatPacket(CHAT_TYPE_INFO, "Bekle %d", PULSEMANAGER_CLOCK_TO_SEC2(GetPlayerID(), ePulse::ItemAffectUse));
						return false;
					}
					//activate
					const int iPotion = item->GetVnum()-84100;
					const DWORD dwAffect = AFFECT_TOGGLE_POTION+iPotion;
					if (FindAffect(dwAffect))
					{
						if (item->GetSocket(1) == false)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Toggle potion close other one first."));
							return false;
						}
						while (FindAffect(dwAffect))
							RemoveAffect(dwAffect);
						item->Lock(false);
						item->SetSocket(1, false);
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Toggle potion deactivated."));
					}
					else 
					{
						item->SetSocket(1, true);
						item->Lock(true);
						const long lRemainingTime = item->GetSocket(0);
						const TItemTable* itemTable = item->GetProto();
						for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
						{
							if (itemTable->aApplies[i].bType == APPLY_NONE || itemTable->aApplies[i].lValue == 0)
								continue;
							AddAffect(dwAffect, aApplyInfo[itemTable->aApplies[i].bType].bPointType, itemTable->aApplies[i].lValue, AFF_NONE, lRemainingTime, 0, false);
						}
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Toggle potion activated."));
					}
					return true;
				}
				//End of Toggle Potion
				if (item->GetVnum() > 50800 && item->GetVnum() <= 50820)
				{
					if (test_server)
						sys_log (0, "ADD addtional effect : vnum(%d) subtype(%d)", item->GetOriginalVnum(), item->GetSubType());

					#ifdef ENABLE_PERMA_BLEND
					BYTE bWaterType = 0;
					switch (item->GetVnum())
					{
						case ITEM_NOMAL_WATER_PEACH: { if (FindAffect(AFFECT_WATER_PEACH)) bWaterType = 1; } break;
						case ITEM_NOMAL_WATER_BELL: { if (FindAffect(AFFECT_WATER_BELL)) bWaterType = 2; } break;
						case ITEM_NOMAL_WATER_SIM: { if (FindAffect(AFFECT_WATER_SIM)) bWaterType = 3; } break;
						case ITEM_NOMAL_WATER_DOK: { if (FindAffect(AFFECT_WATER_DOK)) bWaterType = 4; } break;
						case ITEM_NOMAL_WATER_ZIN: { if (FindAffect(AFFECT_WATER_ZIN)) bWaterType = 5; } break;
						case ITEM_NOMAL_WATER_SAMBO: { if (FindAffect(AFFECT_WATER_SAMBO)) bWaterType = 6; } break;
						case ITEM_NOMAL_WATER_HWAL: { if (FindAffect(AFFECT_WATER_HWAL)) bWaterType = 7; } break;
						case ITEM_NOMAL_WATER_MONG: { if (FindAffect(AFFECT_WATER_MONG)) bWaterType = 8; } break;
					}
					if (bWaterType > 0)
					{
							 if (bWaterType == 1) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma water enable. peach")); }
						else if (bWaterType == 2) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma water enable. bell")); }
						else if (bWaterType == 3) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma water enable. sim")); }
						else if (bWaterType == 4) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma water enable. dok")); }
						else if (bWaterType == 5) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma water enable. zin")); }
						else if (bWaterType == 6) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma water enable. sambo")); }
						else if (bWaterType == 7) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma water enable. hwal")); }
						else if (bWaterType == 8) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma water enable. mong")); }
						return false;
					}
					#endif

					int affect_type = AFFECT_EXP_BONUS_EURO_FREE;
					int apply_type = aApplyInfo[item->GetValue(0)].bPointType;
					int apply_value = item->GetValue(2);
					int apply_duration = item->GetValue(1);

					switch (item->GetSubType())
					{
						case USE_ABILITY_UP:
							if (FindAffect(affect_type, apply_type))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
								return false;
							}

							{
								switch (item->GetValue(0))
								{
									case APPLY_MOV_SPEED:
										AddAffect(affect_type, apply_type, apply_value, AFF_MOV_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case APPLY_ATT_SPEED:
										AddAffect(affect_type, apply_type, apply_value, AFF_ATT_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case APPLY_STR:
									case APPLY_DEX:
									case APPLY_CON:
									case APPLY_INT:
									case APPLY_CAST_SPEED:
									case APPLY_RESIST_MAGIC:
									case APPLY_ATT_GRADE_BONUS:
									case APPLY_DEF_GRADE_BONUS:
										AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, true, true);
										break;
								}
							}

							if (GetDungeon())
								GetDungeon()->UsePotion(this);

							if (GetWarMap())
								GetWarMap()->UsePotion(this, item);

							item->SetCount(item->GetCount() - 1);
							break;

					case USE_AFFECT :
						{
							#ifdef ENABLE_SKILL_TIME 
							if (item->GetVnum() >= 84001 || item->GetVnum() <= 84002) //Esya kodu bu knka
							{
								long lAffect = item->GetValue(0);
								if (FindAffect(lAffect))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu etki zaten calisiyor."));
									return false;
								}
							
								long lDuration = item->GetValue(3);
								if (lDuration == 0)
								{
									lDuration = INFINITE_AFFECT_DURATION;
								}
							
								AddAffect(lAffect, 0, 0, lDuration, 0, false, true);
								item->SetCount(item->GetCount() - 1);
								return true;
							}
							#endif

							long lAffectType = item->GetValue(0);
							if (item->GetVnum() < 84000)
								lAffectType = AFFECT_EXP_BONUS_EURO_FREE;
							if (FindAffect(lAffectType, aApplyInfo[item->GetValue(1)].bPointType))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
							}
							else
							{
								AddAffect(lAffectType, aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false, true);
								item->SetCount(item->GetCount() - 1);
							}
						}
						break;

					case USE_POTION_NODELAY:
						{
							if (GetProtectTime("use_potion_nodelay1") > get_global_time())
								return false;

							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç¿ë Á¦ÇÑ·®À» ÃÊ°úÇÏ¿´½À´Ï´Ù."));
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
										return false;
										break;
								}
							}
							#ifdef ENABLE_NEWSTUFF
							else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
								return false;
							}
							#endif

							bool used = false;

							if (item->GetValue(0) != 0)
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0)
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0)
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¿ùº´ ¶Ç´Â Á¾ÀÚ ¸¦ »ç¿ëÇÏ¿´½À´Ï´Ù"));
									SetUseSeedOrMoonBottleTime();
								}

								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								SetProtectTime("use_potion_nodelay1", get_global_time() + 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;
					}

					return true;
				}


				if (item->GetVnum() >= 27863 && item->GetVnum() <= 27883)
				{
					if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
						return false;
					}
					#ifdef ENABLE_NEWSTUFF
					else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
						return false;
					}
					#endif
				}

				if (test_server)
				{
					 sys_log (0, "USE_ITEM %s Type %d SubType %d vnum %d", item->GetName(), item->GetType(), item->GetSubType(), item->GetOriginalVnum());
				}

				switch (item->GetSubType())
				{
					case USE_TIME_CHARGE_PER:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (NULL == pDestItem)
							{
								return false;
							}

							if (pDestItem->IsDragonSoul())
							{
								int ret;
								char buf[128];
								if (item->GetVnum() == DRAGON_HEART_VNUM)
								{
									ret = pDestItem->GiveMoreTime_Per((float)item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
								}
								else
								{
									ret = pDestItem->GiveMoreTime_Per((float)item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
								}
								if (ret > 0)
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "Inc %ds by item{VN:%d SOC%d:%ld}", ret, item->GetVnum(), ITEM_SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "Inc %ds by item{VN:%d VAL%d:%ld}", ret, item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%dÃÊ ¸¸Å­ ÃæÀüµÇ¾ú½À´Ï´Ù."), ret);
									item->SetCount(item->GetCount() - 1);
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_SUCCESS", buf);
									return true;
								}
								else
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "No change by item{VN:%d SOC%d:%ld}", item->GetVnum(), ITEM_SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "No change by item{VN:%d VAL%d:%ld}", item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÃæÀüÇÒ ¼ö ¾ø½À´Ï´Ù."));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_FAILED", buf);
									return false;
								}
							}
							else
								return false;
						}
						break;
					case USE_TIME_CHARGE_FIX:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (NULL == pDestItem)
							{
								return false;
							}

							if (pDestItem->IsDragonSoul())
							{
								int ret = pDestItem->GiveMoreTime_Fix(item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
								char buf[128];
								if (ret)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%dÃÊ ¸¸Å­ ÃæÀüµÇ¾ú½À´Ï´Ù."), ret);
									sprintf(buf, "Increase %ds by item{VN:%d VAL%d:%ld}", ret, item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_SUCCESS", buf);
									item->SetCount(item->GetCount() - 1);
									return true;
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÃæÀüÇÒ ¼ö ¾ø½À´Ï´Ù."));
									sprintf(buf, "No change by item{VN:%d VAL%d:%ld}", item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_FAILED", buf);
									return false;
								}
							}
							else
								return false;
						}
						break;

					case USE_SPECIAL:

						switch (item->GetVnum())
						{
							case ITEM_NOG_POCKET:
								{
									if (FindAffect(AFFECT_NOG_ABILITY))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}

									long time = item->GetValue(0);
									long moveSpeedPer = item->GetValue(1);
									long attPer = item->GetValue(2);
									long expPer = item->GetValue(3);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_RAMADAN_CANDY:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_RAMADAN_ABILITY))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer = item->GetValue(1);
									long attPer = item->GetValue(2);
									long expPer = item->GetValue(3);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_KURABIYE_K:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_KURABIYE_K)
										|| FindAffect(AFFECT_KURABIYE_B)
										|| FindAffect(AFFECT_KURABIYE_P)
									)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer = item->GetValue(1);
									AddAffect(AFFECT_KURABIYE_K, POINT_ATTBONUS_MONSTER, moveSpeedPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_CIKOLATA_K:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_CIKOLATA_K)
										|| FindAffect(AFFECT_CIKOLATA_B)
										|| FindAffect(AFFECT_CIKOLATA_P)
									)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer = item->GetValue(1);
									AddAffect(AFFECT_CIKOLATA_K, POINT_ATTBONUS_BOSS, moveSpeedPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_SEKER_K:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_SEKER_K)
										|| FindAffect(AFFECT_SEKER_B)
										|| FindAffect(AFFECT_SEKER_P)
									)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer = item->GetValue(1);
									AddAffect(AFFECT_SEKER_K, POINT_ATTBONUS_STONE, moveSpeedPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_KEK_K:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_KEK_K)
										|| FindAffect(AFFECT_KEK_B)
										|| FindAffect(AFFECT_KEK_P)
									)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									AddAffect(AFFECT_KEK_K, POINT_HIT_DAMAGE_BONUS_PVM, moveSpeedPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_KURABIYE_B:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_KURABIYE_K)
										|| FindAffect(AFFECT_KURABIYE_B)
										|| FindAffect(AFFECT_KURABIYE_P)
									)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer = item->GetValue(1);
									AddAffect(AFFECT_KURABIYE_B, POINT_ATTBONUS_MONSTER, moveSpeedPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_CIKOLATA_B:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_CIKOLATA_K)
										|| FindAffect(AFFECT_CIKOLATA_B)
										|| FindAffect(AFFECT_CIKOLATA_P)
									)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer = item->GetValue(1);
									AddAffect(AFFECT_CIKOLATA_B, POINT_ATTBONUS_BOSS, moveSpeedPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_SEKER_B:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_SEKER_K)
										|| FindAffect(AFFECT_SEKER_B)
										|| FindAffect(AFFECT_SEKER_P)
									)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer = item->GetValue(1);
									AddAffect(AFFECT_SEKER_B, POINT_ATTBONUS_STONE, moveSpeedPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_KEK_B:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_KEK_K)
										|| FindAffect(AFFECT_KEK_B)
										|| FindAffect(AFFECT_KEK_P)
									)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer = item->GetValue(1);
									AddAffect(AFFECT_KEK_B, POINT_HIT_DAMAGE_BONUS_PVM, moveSpeedPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_KURABIYE_P:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_KURABIYE_K)
										|| FindAffect(AFFECT_KURABIYE_B)
										|| FindAffect(AFFECT_KURABIYE_P)
									)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer = item->GetValue(1);
									AddAffect(AFFECT_KURABIYE_P, POINT_ATTBONUS_MONSTER, moveSpeedPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_CIKOLATA_P:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_CIKOLATA_K)
										|| FindAffect(AFFECT_CIKOLATA_B)
										|| FindAffect(AFFECT_CIKOLATA_P)
									)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer = item->GetValue(1);
									AddAffect(AFFECT_CIKOLATA_P, POINT_ATTBONUS_BOSS, moveSpeedPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_SEKER_P:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_SEKER_K)
										|| FindAffect(AFFECT_SEKER_B)
										|| FindAffect(AFFECT_SEKER_P)
									)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									AddAffect(AFFECT_SEKER_P, POINT_ATTBONUS_STONE, moveSpeedPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_KEK_P:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_KEK_K)
										|| FindAffect(AFFECT_KEK_B)
										|| FindAffect(AFFECT_KEK_P)
									)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									// @fixme147 END

									long time = item->GetValue(0);
									long moveSpeedPer = item->GetValue(1);
									AddAffect(AFFECT_KEK_P, POINT_HIT_DAMAGE_BONUS_PVM, moveSpeedPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							// case ITEM_MARRIAGE_RING:   /evlilik yuzugu
								// {
									// if (GetProtectTime("evliliksure") > get_global_time())
									// {
										// ChatPacket(CHAT_TYPE_INFO, "Yavaþ olmalýsýn! %d saniye sonra deneyebilirsiniz.", GetProtectTime("evliliksure") - get_global_time());
										// return false;
									// }
									// SetProtectTime("evliliksure", get_global_time() + 10);

									// if (PreventTradeWindow(WND_ALL))
									// {
										// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										// return false;
									// }

									// marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());
									// if (pMarriage)
									// {
										// if (pMarriage->ch1 != NULL)
										// {
											// if (CArenaManager::instance().IsArenaMap(pMarriage->ch1->GetMapIndex()) == true)
											// {
												// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
												// break;
											// }

											// if (pMarriage->ch1->GetDungeon())
											// {
												// ChatPacket(CHAT_TYPE_INFO, "Zindandayken bunu yapamazsin.");
												// break;
											// }

											// if (pMarriage->ch1->PreventTradeWindow(WND_ALL))
											// {
												// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
												// return false;
											// }
										// }

										// if (pMarriage->ch2 != NULL)
										// {
											// if (CArenaManager::instance().IsArenaMap(pMarriage->ch2->GetMapIndex()) == true)
											// {
												// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
												// break;
											// }

											// if (pMarriage->ch2->GetDungeon())
											// {
												// ChatPacket(CHAT_TYPE_INFO, "Zindandayken bunu yapamazsin.");
												// break;
											// }

											// if (pMarriage->ch2->PreventTradeWindow(WND_ALL))
											// {
												// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
												// return false;
											// }
										// }

										// int consumeSP = CalculateConsumeSP(this);

										// if (consumeSP < 0)
											// return false;

										// PointChange(POINT_SP, -consumeSP, false);

										// WarpToPID(pMarriage->GetOther(GetPlayerID()));
									// }
									// else
										// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°áÈ¥ »óÅÂ°¡ ¾Æ´Ï¸é °áÈ¥¹ÝÁö¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
								// }
								// break;

							case UNIQUE_ITEM_CAPE_OF_COURAGE:
							case 70057:
							case REWARD_BOX_UNIQUE_ITEM_CAPE_OF_COURAGE:
								AggregateMonster();
								// item->SetCount(item->GetCount()-1); //@Fin02 Cesaret pelerini s©¥n©¥rs©¥z.
								break;

							#ifdef ENABLE_ACCE_COSTUME_SYSTEM
							case ACCE_REVERSAL_VNUM_1:
							case ACCE_REVERSAL_VNUM_2:
							{
								LPITEM item2;
								if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
									return false;

								if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
									return false;

								if (!CleanAcceAttr(item, item2))
									return false;
								item->SetCount(item->GetCount()-1);
								break;
							}
							#endif

							case UNIQUE_ITEM_WHITE_FLAG:
								ForgetMyAttacker();
								item->SetCount(item->GetCount()-1);
								break;

							case UNIQUE_ITEM_TREASURE_BOX:
								break;

							#ifdef ENABLE_MULTI_FARM_BLOCK
							case ITEM_MULTI_FARM_BLOCK_1:
							case ITEM_MULTI_FARM_BLOCK_2:
							case ITEM_MULTI_FARM_BLOCK_3:
							case ITEM_MULTI_FARM_BLOCK_4:
							case ITEM_MULTI_FARM_BLOCK_5:
							case ITEM_MULTI_FARM_BLOCK_6:
							{
								if (FindAffect(AFFECT_MULTI_FARM_PREMIUM))
								{
									ChatPacket(CHAT_TYPE_INFO, "Zaten bu etkiye sahipsiniz!");
									return false;
								}
								else
								{
									long lDuration = item->GetValue(0);
									if (lDuration == 1)
									{
										lDuration = INFINITE_AFFECT_DURATION;
									}

									AddAffect(AFFECT_MULTI_FARM_PREMIUM, POINT_NONE, item->GetValue(1), AFF_NONE, lDuration, 0, false, false);
									item->SetCount(item->GetCount() - 1);
									ChatPacket(CHAT_TYPE_INFO, "Etki karakterinize baþarýyla eklendi!");
									ChatPacket(CHAT_TYPE_INFO, "Bu etkiyi kullanmak istiyorsanýz, bu karakterin aktif düþme durumuna ihtiyacý vardýr!");

									if (!GetRewardStatus())
									{
										char_event_info* info = AllocEventInfo<char_event_info>();
										info->ch = this;
										m_pkCheckMultiFarmEvent = event_create(multi_farm_event, info, PASSES_PER_SEC(2));
									}
									return true;
								}
							}
							break;
							#endif

							#ifdef ENABLE_MULTI_HUNT_BLOCK
							case ITEM_MULTI_HUNT_BLOCK_1:
							case ITEM_MULTI_HUNT_BLOCK_2:
							case ITEM_MULTI_HUNT_BLOCK_3:
							case ITEM_MULTI_HUNT_BLOCK_4:
							case ITEM_MULTI_HUNT_BLOCK_5:
							case ITEM_MULTI_HUNT_BLOCK_6:
							{
								if (FindAffect(AFFECT_MULTI_HUNT_PREMIUM))
								{
									ChatPacket(CHAT_TYPE_INFO, "Zaten bu etkiye sahipsiniz!");
									return false;
								}
								else
								{
									long lDuration = item->GetValue(0);
									if (lDuration == 1)
									{
										lDuration = INFINITE_AFFECT_DURATION;
									}

									AddAffect(AFFECT_MULTI_HUNT_PREMIUM, POINT_NONE, item->GetValue(1), AFF_NONE, lDuration, 0, false, false);
									item->SetCount(item->GetCount() - 1);
									ChatPacket(CHAT_TYPE_INFO, "Etki karakterinize baþarýyla eklendi!");
									ChatPacket(CHAT_TYPE_INFO, "Bu etkiyi kullanmak istiyorsanýz, bu karakterin aktif düþme durumuna ihtiyacý vardýr!");

									if (!GetMultiHuntStatus())
									{
										char_event_info* info = AllocEventInfo<char_event_info>();
										info->ch = this;
										m_pkCheckMultiHuntEvent = event_create(multi_hunt_event, info, PASSES_PER_SEC(2));
									}
									return true;
								}
							}
							break;
							#endif

							#ifdef ENABLE_MULTI_FISH_BLOCK
							case ITEM_MULTI_FISH_BLOCK_1:
							case ITEM_MULTI_FISH_BLOCK_2:
							case ITEM_MULTI_FISH_BLOCK_3:
							case ITEM_MULTI_FISH_BLOCK_4:
							case ITEM_MULTI_FISH_BLOCK_5:
							case ITEM_MULTI_FISH_BLOCK_6:
							{
								if (FindAffect(AFFECT_MULTI_FISH_PREMIUM))
								{
									ChatPacket(CHAT_TYPE_INFO, "Zaten bu etkiye sahipsiniz!");
									return false;
								}
								else
								{
									long lDuration = item->GetValue(0);
									if (lDuration == 1)
									{
										lDuration = INFINITE_AFFECT_DURATION;
									}

									AddAffect(AFFECT_MULTI_FISH_PREMIUM, POINT_NONE, item->GetValue(1), AFF_NONE, lDuration, 0, false, false);
									item->SetCount(item->GetCount() - 1);
									ChatPacket(CHAT_TYPE_INFO, "Etki karakterinize baþarýyla eklendi!");
									ChatPacket(CHAT_TYPE_INFO, "Bu etkiyi kullanmak istiyorsanýz, bu karakterin aktif düþme durumuna ihtiyacý vardýr!");

									if (!GetMultiFishStatus())
									{
										char_event_info* info = AllocEventInfo<char_event_info>();
										info->ch = this;
										m_pkCheckMultiFishEvent = event_create(multi_fish_event, info, PASSES_PER_SEC(2));
									}
									return true;
								}
							}
							break;
							#endif

							#ifdef ENABLE_BATTLE_PASS
							case ITEM_BATTLE_PASS:
							case ITEM_BATTLE_PASS + 1:
							case ITEM_BATTLE_PASS + 2:
							case ITEM_BATTLE_PASS + 3:
							case ITEM_BATTLE_PASS + 4:
							{
								char szQuery[1024];
								snprintf(szQuery, sizeof(szQuery), "SELECT * FROM player.battle_pass_ranking WHERE player_name = '%s' AND battle_pass_id = %ld;", GetName(), item->GetValue(0));

								auto pMsg(DBManager::instance().DirectQuery(szQuery));
								if (pMsg->Get()->uiNumRows > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have already completed this battle pass."));
									return false;
								}

								if (FindAffect(AFFECT_BATTLE_PASS))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You already have a battle pass activated on this character."));
									return false;
								}
								else
								{
									AddAffect(AFFECT_BATTLE_PASS, POINT_BATTLE_PASS_ID, item->GetValue(0), 0, item->GetValue(1), 0, true);
									item->SetCount(item->GetCount() - 1);
									CBattlePass::instance().BattlePassRequestOpen(this);
								}
							}
							break;
							#endif

							#ifdef ENABLE_PERMA_BLEND
							case ITEM_PERMA_BLEND_RED:
							case ITEM_PERMA_BLEND_ORANGE:
							case ITEM_PERMA_BLEND_YELLOW:
							case ITEM_PERMA_BLEND_GREEN:
							case ITEM_PERMA_BLEND_BLUE:
							case ITEM_PERMA_BLEND_WHITE:
							case ITEM_PERMA_WATER_PEACH:
							case ITEM_PERMA_WATER_BELL:
							case ITEM_PERMA_WATER_SIM:
							case ITEM_PERMA_WATER_DOK:
							case ITEM_PERMA_WATER_ZIN:
							case ITEM_PERMA_WATER_SAMBO:
							case ITEM_PERMA_WATER_HWAL:
							case ITEM_PERMA_WATER_MONG:
								{
									DWORD type = AFFECT_NONE;
									type = item->GetValue(0);

									if (AFFECT_NONE == type)
										return false;

									if (!(type >= AFFECT_BLEND_RED && type <= AFFECT_WATER_MONG))
										return false;

									CAffect * pAffect = FindAffect(type);
									if (NULL != pAffect)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}

									//Süreli þebnem varken kullanma fixi
									switch (item->GetVnum())
									{
										case ITEM_PERMA_BLEND_RED: { if (FindAffect(AFFECT_BLEND, POINT_CRITICAL_PCT)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal blend enable. red")); return false; } } break;
										case ITEM_PERMA_BLEND_ORANGE: { if (FindAffect(AFFECT_BLEND, POINT_PENETRATE_PCT)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal blend enable. orange")); return false; } } break;
										case ITEM_PERMA_BLEND_YELLOW: { if (FindAffect(AFFECT_BLEND, POINT_ATT_SPEED)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal blend enable. yellow")); return false; } } break;
										case ITEM_PERMA_BLEND_GREEN: { if (FindAffect(AFFECT_BLEND, POINT_RESIST_MAGIC)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal blend enable. green")); return false; } } break;
										case ITEM_PERMA_BLEND_BLUE: { if (FindAffect(AFFECT_BLEND, POINT_ATT_GRADE_BONUS)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal blend enable. blue")); return false; } } break;
										case ITEM_PERMA_BLEND_WHITE: { if (FindAffect(AFFECT_BLEND, POINT_DEF_GRADE_BONUS)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal blend enable. white")); return false; } } break;
										//-----------------//
										case ITEM_PERMA_WATER_PEACH: { if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_MOV_SPEED)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal water enable. peach")); return false; } } break;
										case ITEM_PERMA_WATER_BELL: { if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_ST)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal water enable. bell")); return false; } } break;
										case ITEM_PERMA_WATER_SIM: { if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_PENETRATE_PCT)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal water enable. sim")); return false; } } break;
										case ITEM_PERMA_WATER_DOK: { if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_CRITICAL_PCT)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal water enable. dok")); return false; } } break;
										case ITEM_PERMA_WATER_ZIN: { if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_ATT_GRADE_BONUS)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal water enable. zin")); return false; } } break;
										case ITEM_PERMA_WATER_SAMBO: { if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_DEF_GRADE_BONUS)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal water enable. sambo")); return false; } } break;
										case ITEM_PERMA_WATER_HWAL: { if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_ATT_SPEED)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal water enable. hwal")); return false; } } break;
										case ITEM_PERMA_WATER_MONG: { if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_RESIST_MAGIC)) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Normal water enable. mong")); return false; } } break;
									}

									AddAffect(type, aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), AFF_NONE, INFINITE_AFFECT_DURATION, 0, true);
									ITEM_MANAGER::instance().RemoveItem(item);
								}
								break;
							#endif

							#ifdef ENABLE_FARM_BLEND
							case ITEM_FARM_BLEND_1:
							case ITEM_FARM_BLEND_2:
								{
									DWORD type = AFFECT_NONE;
									type = item->GetValue(0);

									if (AFFECT_NONE == type)
										return false;

									if (!(type >= AFFECT_BLEND_FARM_1 && type <= AFFECT_BLEND_FARM_2))
										return false;

									CAffect * pAffect = FindAffect(type);
									if (NULL != pAffect)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}

									long lTime = item->GetValue(3);
									if (lTime == 99) { lTime = INFINITE_AFFECT_DURATION; }

									AddAffect(type, aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), AFF_NONE, lTime, 0, true);
									ITEM_MANAGER::instance().RemoveItem(item);
								}
								break;
							#endif

							#if defined(__BL_LUCKY_BOX__)
							case BLUE_FORTUNE_CHEST:
							case RED_FORTUNE_CHEST:
							case GREEN_FORTUNE_CHEST:
								SetLuckyBoxSrcItem(item);
								break;
							#endif

							case 30093:
							case 30094:
							case 30095:
							case 30096:
								{
									const int MAX_BAG_INFO = 26;
									static struct LuckyBagInfo
									{
										DWORD count;
										int prob;
										DWORD vnum;
									} b1[MAX_BAG_INFO] =
									{
										{ 1000,	302,	1 },
										{ 10,	150,	27002 },
										{ 10,	75,	27003 },
										{ 10,	100,	27005 },
										{ 10,	50,	27006 },
										{ 10,	80,	27001 },
										{ 10,	50,	27002 },
										{ 10,	80,	27004 },
										{ 10,	50,	27005 },
										{ 1,	10,	50300 },
										{ 1,	6,	92 },
										{ 1,	2,	132 },
										{ 1,	6,	1052 },
										{ 1,	2,	1092 },
										{ 1,	6,	2082 },
										{ 1,	2,	2122 },
										{ 1,	6,	3082 },
										{ 1,	2,	3122 },
										{ 1,	6,	5052 },
										{ 1,	2,	5082 },
										{ 1,	6,	7082 },
										{ 1,	2,	7122 },
										{ 1,	1,	11282 },
										{ 1,	1,	11482 },
										{ 1,	1,	11682 },
										{ 1,	1,	11882 },
									};

									LuckyBagInfo * bi = NULL;
									bi = b1;

									int pct = number(1, 1000);

									int i;
									for (i=0;i<MAX_BAG_INFO;i++)
									{
										if (pct <= bi[i].prob)
											break;
										pct -= bi[i].prob;
									}
									if (i>=MAX_BAG_INFO)
										return false;

									if (bi[i].vnum == 50300)
									{
										GiveRandomSkillBook();
									}
									else if (bi[i].vnum == 1)
									{
										#ifdef ENABLE_FULL_YANG
										GoldChange(1000);
										#else
										PointChange(POINT_GOLD, 1000, true);
										#endif
									}
									else
									{
										AutoGiveItem(bi[i].vnum, bi[i].count);
									}
									ITEM_MANAGER::instance().RemoveItem(item);
								}
								break;

							case 50004:
								{
									if (item->GetSocket(0))
									{
										item->SetSocket(0, item->GetSocket(0) + 1);
									}
									else
									{

										int iMapIndex = GetMapIndex();

										PIXEL_POSITION pos;

										if (SECTREE_MANAGER::instance().GetRandomLocation(iMapIndex, pos, 700))
										{
											item->SetSocket(0, 1);
											item->SetSocket(1, pos.x);
											item->SetSocket(2, pos.y);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ °÷¿¡¼± ÀÌº¥Æ®¿ë °¨Áö±â°¡ µ¿ÀÛÇÏÁö ¾Ê´Â°Í °°½À´Ï´Ù."));
											return false;
										}
									}

									int dist = 0;
									float distance = (DISTANCE_SQRT(GetX()-item->GetSocket(1), GetY()-item->GetSocket(2)));

									if (distance < 1000.0f)
									{

										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌº¥Æ®¿ë °¨Áö±â°¡ ½Åºñ·Î¿î ºûÀ» ³»¸ç »ç¶óÁý´Ï´Ù."));


										struct TEventStoneInfo
										{
											DWORD dwVnum;
											int count;
											int prob;
										};
										const int EVENT_STONE_MAX_INFO = 15;
										TEventStoneInfo info_10[EVENT_STONE_MAX_INFO] =
										{
											{ 27001, 10,  8 },
											{ 27004, 10,  6 },
											{ 27002, 10, 12 },
											{ 27005, 10, 12 },
											{ 27100,  1,  9 },
											{ 27103,  1,  9 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 12 },

											{ 25040,  1,  4 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1,  0 },

											{ 50001,  1,  0 },
											{ 50003,  1,  1 },
										};
										TEventStoneInfo info_7[EVENT_STONE_MAX_INFO] =
										{
											{ 27001, 10,  1 },
											{ 27004, 10,  1 },
											{ 27004, 10,  9 },
											{ 27005, 10,  9 },
											{ 27100,  1,  5 },
											{ 27103,  1,  5 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 14 },

											{ 25040,  1,  5 },

											{ 27410,  1,  5 },
											{ 27600,  1,  5 },
											{ 25100,  1,  5 },

											{ 50001,  1,  0 },
											{ 50003,  1,  5 },

										};
										TEventStoneInfo info_4[EVENT_STONE_MAX_INFO] =
										{
											{ 27001, 10,  0 },
											{ 27004, 10,  0 },
											{ 27002, 10,  0 },
											{ 27005, 10,  0 },
											{ 27100,  1,  0 },
											{ 27103,  1,  0 },
											{ 27101,  1,  0 },
											{ 27104,  1,  0 },
											{ 27999,  1, 25 },

											{ 25040,  1,  0 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1, 15 },

											{ 50001,  1, 10 },
											{ 50003,  1, 50 },

										};

										{
											TEventStoneInfo* info;
											if (item->GetSocket(0) <= 4)
												info = info_4;
											else if (item->GetSocket(0) <= 7)
												info = info_7;
											else
												info = info_10;

											int prob = number(1, 100);

											for (int i = 0; i < EVENT_STONE_MAX_INFO; ++i)
											{
												if (!info[i].prob)
													continue;

												if (prob <= info[i].prob)
												{
													if (info[i].dwVnum == 50001)
													{
														DWORD * pdw = M2_NEW DWORD[2];

														pdw[0] = info[i].dwVnum;
														pdw[1] = info[i].count;


														DBManager::instance().ReturnQuery(QID_LOTTO, GetPlayerID(), pdw,
																"INSERT INTO lotto_list VALUES(0, 'server%s', %u, NOW())",
																get_table_postfix(), GetPlayerID());
													}
													else
														AutoGiveItem(info[i].dwVnum, info[i].count);

													break;
												}
												prob -= info[i].prob;
											}
										}

										char chatbuf[CHAT_MAX_LEN + 1];
										int len = snprintf(chatbuf, sizeof(chatbuf), "StoneDetect %u 0 0", (DWORD)GetVID());

										if (len < 0 || len >= (int) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;

										TPacketGCChat pack_chat;
										pack_chat.header	= HEADER_GC_CHAT;
										pack_chat.size		= sizeof(TPacketGCChat) + len;
										pack_chat.type		= CHAT_TYPE_COMMAND;
										pack_chat.id		= 0;
										pack_chat.bEmpire	= GetDesc()->GetEmpire();
										//pack_chat.id	= vid;

										TEMP_BUFFER buf;
										buf.write(&pack_chat, sizeof(TPacketGCChat));
										buf.write(chatbuf, len);

										PacketAround(buf.read_peek(), buf.size());

										ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 1");
										return true;
									}
									else if (distance < 20000)
										dist = 1;
									else if (distance < 70000)
										dist = 2;
									else
										dist = 3;


									const int STONE_DETECT_MAX_TRY = 10;
									if (item->GetSocket(0) >= STONE_DETECT_MAX_TRY)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌº¥Æ®¿ë °¨Áö±â°¡ ÈçÀûµµ ¾øÀÌ »ç¶óÁý´Ï´Ù."));
										ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 0");
										AutoGiveItem(27002);
										return true;
									}

									if (dist)
									{
										char chatbuf[CHAT_MAX_LEN + 1];
										int len = snprintf(chatbuf, sizeof(chatbuf),
												"StoneDetect %u %d %d",
											   	(DWORD)GetVID(), dist, (int)GetDegreeFromPositionXY(GetX(), item->GetSocket(2), item->GetSocket(1), GetY()));

										if (len < 0 || len >= (int) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;

										TPacketGCChat pack_chat;
										pack_chat.header	= HEADER_GC_CHAT;
										pack_chat.size		= sizeof(TPacketGCChat) + len;
										pack_chat.type		= CHAT_TYPE_COMMAND;
										pack_chat.id		= 0;
										pack_chat.bEmpire	= GetDesc()->GetEmpire();
										//pack_chat.id		= vid;

										TEMP_BUFFER buf;
										buf.write(&pack_chat, sizeof(TPacketGCChat));
										buf.write(chatbuf, len);

										PacketAround(buf.read_peek(), buf.size());
									}

								}
								break;

							case 27989:
							case 76006:
								{
									LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());

									if (pMap != NULL)
									{
										item->SetSocket(0, item->GetSocket(0) + 1);

										FFindStone f;

										// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
										pMap->for_each(f);

										if (f.m_mapStone.size() > 0)
										{
											std::map<DWORD, LPCHARACTER>::iterator stone = f.m_mapStone.begin();

											DWORD max = UINT_MAX;
											LPCHARACTER pTarget = stone->second;

											while (stone != f.m_mapStone.end())
											{
												DWORD dist = (DWORD)DISTANCE_SQRT(GetX()-stone->second->GetX(), GetY()-stone->second->GetY());

												if (dist != 0 && max > dist)
												{
													max = dist;
													pTarget = stone->second;
												}
												stone++;
											}

											if (pTarget != NULL)
											{
												int val = 3;

												if (max < 10000) val = 2;
												else if (max < 70000) val = 1;

												ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u %d %d", (DWORD)GetVID(), val,
														(int)GetDegreeFromPositionXY(GetX(), pTarget->GetY(), pTarget->GetX(), GetY()));
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°¨Áö±â¸¦ ÀÛ¿ëÇÏ¿´À¸³ª °¨ÁöµÇ´Â ¿µ¼®ÀÌ ¾ø½À´Ï´Ù."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°¨Áö±â¸¦ ÀÛ¿ëÇÏ¿´À¸³ª °¨ÁöµÇ´Â ¿µ¼®ÀÌ ¾ø½À´Ï´Ù."));
										}

										if (item->GetSocket(0) >= 6)
										{
											ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u 0 0", (DWORD)GetVID());
											// ITEM_MANAGER::instance().RemoveItem(item); @Fin03 Metin ta?©¥ dedektoru s©¥n©¥rs©¥z.
										}
									}
									break;
								}
								break;

							#ifdef __AUTO_SKILL_READER__
							case 50513:
							case 50300:
								ChatPacket(CHAT_TYPE_COMMAND, "OpenAutoSkillReader");
								break;
							#endif

							case 27996:
								item->SetCount(item->GetCount() - 1);
								AttackedByPoison(NULL); // @warme008
								break;

							#ifdef ENABLE_MOB_RANGE_ITEM
							case 38113:
							case 38115:
							{
								int iTime = item->GetValue(0);
								if (iTime <= 0)
								{
									sys_err("Sure bulunamadi! Vnum: %u", item->GetVnum());
									return false;
								}

								if (FindAffect(AFFECT_ANTI_MOB))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
									return false;
								}

								AddAffect(AFFECT_ANTI_MOB, POINT_NONE, 0, AFF_NONE, 60*60*24*iTime, 0, true, true);
								item->SetCount(item->GetCount()-1);
							}
							break;
							#endif

							#ifdef ENABLE_REFINE_LUCK
							case 38114:
							{
								if (FindAffect(AFFECT_REFINE_LUCK))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
									return false;
								}

								AddAffect(AFFECT_REFINE_LUCK, POINT_NONE, 0, AFF_NONE, 60*60*24*7, 0, true, true);
								item->SetCount(item->GetCount()-1);
							}
							break;
							#endif

							case 27987:
								{
									item->SetCount(item->GetCount() - 1);

									int r = number(1, 100);

									if (r <= 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³¿¡¼­ µ¹Á¶°¢ÀÌ ³ª¿Ô½À´Ï´Ù."));
										AutoGiveItem(27990);
									}
									else
									{
										const int prob_table_gb2312[] =
										{
											95, 97, 99
										};

										const int * prob_table = prob_table_gb2312;

										if (r <= prob_table[0])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³°¡ ÈçÀûµµ ¾øÀÌ »ç¶óÁý´Ï´Ù."));
										}
										else if (r <= prob_table[1])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³¿¡¼­ ¹éÁøÁÖ°¡ ³ª¿Ô½À´Ï´Ù."));
											AutoGiveItem(27992);
										}
										else if (r <= prob_table[2])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³¿¡¼­ Ã»ÁøÁÖ°¡ ³ª¿Ô½À´Ï´Ù."));
											AutoGiveItem(27993);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³¿¡¼­ ÇÇÁøÁÖ°¡ ³ª¿Ô½À´Ï´Ù."));
											AutoGiveItem(27994);
										}
									}
								}
								break;

							case 71013:
								CreateFly(number(FLY_FIREWORK1, FLY_FIREWORK6), this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50100:
							case 50101:
							case 50102:
							case 50103:
							case 50104:
							case 50105:
							case 50106:
								CreateFly(item->GetVnum() - 50100 + FLY_FIREWORK1, this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50200:
								if (g_bEnableBootaryCheck)
								{
									if (IS_BOTARYABLE_ZONE(GetMapIndex()) == true)
									{
										__OpenPrivateShop();
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³ÀÎ »óÁ¡À» ¿­ ¼ö ¾ø´Â Áö¿ªÀÔ´Ï´Ù"));
									}
								}
								else
								{
									__OpenPrivateShop();
								}
								break;

							case fishing::FISH_MIND_PILL_VNUM:
								AddAffect(AFFECT_FISH_MIND_PILL, POINT_NONE, 0, AFF_FISH_MIND, 20*60, 0, true);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50301:
							case 50302:
							case 50303:
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									int lv = GetSkillLevel(SKILL_LEADERSHIP);

									if (lv < item->GetValue(0))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥Àº ³Ê¹« ¾î·Á¿ö ÀÌÇØÇÏ±â°¡ Èûµì´Ï´Ù."));
										return false;
									}

									if (lv >= item->GetValue(1))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥Àº ¾Æ¹«¸® ºÁµµ µµ¿òÀÌ µÉ °Í °°Áö ¾Ê½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(SKILL_LEADERSHIP))
									{
										#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
										#else
										ITEM_MANAGER::instance().RemoveItem(item);
										#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(SKILL_LEADERSHIP, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50304:
							case 50305:
							case 50306:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;

									}
									if (GetSkillLevel(SKILL_COMBO) == 0 && GetLevel() < 30)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("·¹º§ 30ÀÌ µÇ±â Àü¿¡´Â ½ÀµæÇÒ ¼ö ÀÖÀ» °Í °°Áö ¾Ê½À´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) == 1 && GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("·¹º§ 50ÀÌ µÇ±â Àü¿¡´Â ½ÀµæÇÒ ¼ö ÀÖÀ» °Í °°Áö ¾Ê½À´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) >= 2)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¿¬°è±â´Â ´õÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									int iPct = item->GetValue(0);

									if (LearnSkillByBook(SKILL_COMBO, iPct))
									{
										#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
										#else
										ITEM_MANAGER::instance().RemoveItem(item);
										#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(SKILL_COMBO, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50311:
							case 50312:
							case 50313:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;

									}
									DWORD dwSkillVnum = item->GetValue(0);
									int iPct = MINMAX(0, item->GetValue(1), 100);
									if (GetSkillLevel(dwSkillVnum)>=20 || dwSkillVnum-SKILL_LANGUAGE1+1 == GetEmpire())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì ¿Ïº®ÇÏ°Ô ¾Ë¾ÆµéÀ» ¼ö ÀÖ´Â ¾ð¾îÀÌ´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
										#else
										ITEM_MANAGER::instance().RemoveItem(item);
										#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50061 :
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									DWORD dwSkillVnum = item->GetValue(0);
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum) >= 10)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
										#else
										ITEM_MANAGER::instance().RemoveItem(item);
										#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50314: case 50315: case 50316:
							case 50323: case 50324:
							case 50325: case 50326:
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									int iSkillLevelLowLimit = item->GetValue(0);
									int iSkillLevelHighLimit = item->GetValue(1);
									int iPct = MINMAX(0, item->GetValue(2), 100);
									int iLevelLimit = item->GetValue(3);
									DWORD dwSkillVnum = 0;

									switch (item->GetVnum())
									{
										case 50314: case 50315: case 50316:
											dwSkillVnum = SKILL_POLYMORPH;
											break;

										case 50323: case 50324:
											dwSkillVnum = SKILL_ADD_HP;
											break;

										case 50325: case 50326:
											dwSkillVnum = SKILL_RESIST_PENETRATE;
											break;

										default:
											return false;
									}

									if (0 == dwSkillVnum)
										return false;

									if (GetLevel() < iLevelLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥À» ÀÐÀ¸·Á¸é ·¹º§À» ´õ ¿Ã·Á¾ß ÇÕ´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= 40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) < iSkillLevelLowLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥Àº ³Ê¹« ¾î·Á¿ö ÀÌÇØÇÏ±â°¡ Èûµì´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= iSkillLevelHighLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥À¸·Î´Â ´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
										#else
										ITEM_MANAGER::instance().RemoveItem(item);
										#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50902:
							case 50903:
							case 50904:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;

									}
									DWORD dwSkillVnum = SKILL_CREATE;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
										#else
										ITEM_MANAGER::instance().RemoveItem(item);
										#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);

										if (test_server)
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Success to learn skill ");
										}
									}
									else
									{
										if (test_server)
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Failed to learn skill ");
										}
									}
								}
								break;

								// MINING
							case ITEM_MINING_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;

									}
									DWORD dwSkillVnum = SKILL_MINING;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
										#else
										ITEM_MANAGER::instance().RemoveItem(item);
										#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;
								// END_OF_MINING

							case ITEM_HORSE_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;

									}
									DWORD dwSkillVnum = SKILL_HORSE;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ ½Â¸¶ ½ºÅ³À» ¼ö·ÃÇÒ ¼ö ÀÖ´Â ·¹º§ÀÌ ¾Æ´Õ´Ï´Ù."));
										return false;
									}

									if (!test_server && get_global_time() < GetSkillNextReadTime(dwSkillVnum))
									{
										if (FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
										{

											RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÁÖ¾È¼ú¼­¸¦ ÅëÇØ ÁÖÈ­ÀÔ¸¶¿¡¼­ ºüÁ®³ª¿Ô½À´Ï´Ù."));
										}
										else
										{
											SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
											return false;
										}
									}

									if (GetPoint(POINT_HORSE_SKILL) >= 20 ||
											GetSkillLevel(SKILL_HORSE_WILDATTACK) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60 ||
											GetSkillLevel(SKILL_HORSE_WILDATTACK_RANGE) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ½Â¸¶ ¼ö·Ã¼­¸¦ ÀÐÀ» ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (number(1, 100) <= iPct)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("½Â¸¶ ¼ö·Ã¼­¸¦ ÀÐ¾î ½Â¸¶ ½ºÅ³ Æ÷ÀÎÆ®¸¦ ¾ò¾ú½À´Ï´Ù."));
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾òÀº Æ÷ÀÎÆ®·Î´Â ½Â¸¶ ½ºÅ³ÀÇ ·¹º§À» ¿Ã¸± ¼ö ÀÖ½À´Ï´Ù."));
										PointChange(POINT_HORSE_SKILL, 1);

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										if (!test_server)
											SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("½Â¸¶ ¼ö·Ã¼­ ÀÌÇØ¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
									}

									#ifdef ENABLE_BOOKS_STACKFIX
									item->SetCount(item->GetCount() - 1);
									#else
									ITEM_MANAGER::instance().RemoveItem(item);
									#endif
								}
								break;

							case 70102:
							case 70103:
								{
									if (GetAlignment() >= 0)
										return false;

									int delta = MIN(-GetAlignment(), item->GetValue(0));

									sys_log(0, "%s ALIGNMENT ITEM %d", GetName(), delta);

									UpdateAlignment(delta);
									item->SetCount(item->GetCount() - 1);

									if (delta / 10 > 0)
									{
										ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¸¶À½ÀÌ ¸¼¾ÆÁö´Â±º. °¡½¿À» Áþ´©¸£´ø ¹«¾ð°¡°¡ Á» °¡º­¿öÁø ´À³¦ÀÌ¾ß."));
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼±¾ÇÄ¡°¡ %d Áõ°¡ÇÏ¿´½À´Ï´Ù."), delta/10);
									}
								}
								break;

							case 71107:
							case 71418:
							case 71419:
							case 71420:
							case 71421:
							case 39032: // @fixme169 mythical peach alternative vnum
							case 95208: // @fixme169 mythical peach alternative vnum
							{
								int val = item->GetValue(0);
								int interval = item->GetValue(1);

								quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID());
								if (!pPC) // @fixme169 missing check
									return false;

								int last_use_time = pPC->GetFlag("mythical_peach.last_use_time");
								if (get_global_time() - last_use_time < interval * 60 * 60)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu nesneyi henüz tekrar kullanamazsýnýz."));
									return false;
								}

#ifdef ENABLE_ALIGNMENT_BONUS
								int AlignmentValue = ALIGNMENT_NEW_MAX_POINT;
#else
								int AlignmentValue = ALIGNMENT_MAX_POINT;
#endif

								// Derece puaný kontrolü eklendi
								if (GetAlignment() < 199999)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu nesneyi kullanmak icin derece puaniniz en az 19.999 olmalidir."));
									return false;
								}

								if (GetAlignment() >= AlignmentValue)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Derece puanýnýz zaten maksimum seviyede."));
									return false;
								}

								int old_alignment = GetAlignment() / 10;

								if ((AlignmentValue - GetAlignment()) < (val * 10))
									UpdateAlignment((AlignmentValue - GetRealAlignment()), true);
								else
									UpdateAlignment(val * 10, true);

								item->SetCount(item->GetCount() - 1);
								pPC->SetFlag("mythical_peach.last_use_time", get_global_time());

								ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("Bir meyve yediniz. Ýçinizdeki iyilik güçlendi."));
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Derece puanýnýz %d arttý."), val);

								char buf[256 + 1];
								snprintf(buf, sizeof(buf), "%d %d", old_alignment, GetAlignment() / 10);
								LogManager::instance().CharLog(this, val, "MYTHICAL_PEACH", buf);
							}
							break;

							case 71422: // Yaþam Meyvesi Özel
							case 71423:
							case 71424:
							case 71425:
							case 71426:
							{
								int val = item->GetValue(0);

#ifdef ENABLE_ALIGNMENT_BONUS
								int AlignmentValue = ALIGNMENT_NEW_MAX_POINT;
#else
								int AlignmentValue = ALIGNMENT_MAX_POINT;
#endif

								// Yeni kontrol: derece puaný düþükse kullanýlamasýn
								if (GetAlignment() < 199999)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu nesneyi kullanmak icin derece puaniniz en az 19.999 olmalidir."));
									return false;
								}

								if (GetAlignment() < ALIGNMENT_MAX_POINT)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Derece puanýnýz bu nesne için yetersiz."));
									return false;
								}

								if (GetAlignment() >= AlignmentValue)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Derece puanýnýz zaten maksimum seviyede."));
									return false;
								}

								if ((AlignmentValue - GetAlignment()) < (val * 10))
									UpdateAlignment((AlignmentValue - GetRealAlignment()), true);
								else
									UpdateAlignment(val * 10, true);

								item->SetCount(item->GetCount() - 1);

								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bir özel yaþam meyvesi yediniz."));
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Derece puanýnýz %d arttý."), val);

								int iNewAlignment = GetAlignment() / 10;
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Yeni derece puanýnýz: %d"), iNewAlignment);
							}
							break;


							case 71109:
							case 72719:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
										return false;

									if (item2->GetSocketCount() == 0)
										return false;

									switch( item2->GetType() )
									{
										case ITEM_WEAPON:
											break;
										case ITEM_ARMOR:
											switch (item2->GetSubType())
											{
											case ARMOR_EAR:
											case ARMOR_WRIST:
											case ARMOR_NECK:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»©³¾ ¿µ¼®ÀÌ ¾ø½À´Ï´Ù"));
												return false;
											}
											break;

										default:
											return false;
									}

									std::stack<long> socket;

									for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										socket.push(item2->GetSocket(i));

									int idx = ITEM_SOCKET_MAX_NUM - 1;

									while (socket.size() > 0)
									{
										if (socket.top() > 2 && socket.top() != ITEM_BROKEN_METIN_VNUM)
											break;

										idx--;
										socket.pop();
									}

									if (socket.size() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»©³¾ ¿µ¼®ÀÌ ¾ø½À´Ï´Ù"));
										return false;
									}

									LPITEM pItemReward = AutoGiveItem(socket.top());

									if (pItemReward != NULL)
									{
										item2->SetSocket(idx, 1);

										char buf[256+1];
										snprintf(buf, sizeof(buf), "%s(%u) %s(%u)",
												item2->GetName(), item2->GetID(), pItemReward->GetName(), pItemReward->GetID());
										LogManager::instance().ItemLog(this, item, "USE_DETACHMENT_ONE", buf);

										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case 70201:
							case 70202:
							case 70203:
							case 70204:
							case 70205:
							case 70206:
								{
									// NEW_HAIR_STYLE_ADD
									if (GetPart(PART_HAIR) >= 1001)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÇöÀç Çì¾î½ºÅ¸ÀÏ¿¡¼­´Â ¿°»ö°ú Å»»öÀÌ ºÒ°¡´ÉÇÕ´Ï´Ù."));
									}
									// END_NEW_HAIR_STYLE_ADD
									else
									{
										quest::CQuestManager& q = quest::CQuestManager::instance();
										quest::PC* pPC = q.GetPC(GetPlayerID());

										if (pPC)
										{
											int last_dye_level = pPC->GetFlag("dyeing_hair.last_dye_level");

											if (last_dye_level == 0 ||
													last_dye_level+3 <= GetLevel() ||
													item->GetVnum() == 70201)
											{
												SetPart(PART_HAIR, item->GetVnum() - 70201);

												if (item->GetVnum() == 70201)
													pPC->SetFlag("dyeing_hair.last_dye_level", 0);
												else
													pPC->SetFlag("dyeing_hair.last_dye_level", GetLevel());

												item->SetCount(item->GetCount() - 1);
												UpdatePacket();
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d ·¹º§ÀÌ µÇ¾î¾ß ´Ù½Ã ¿°»öÇÏ½Ç ¼ö ÀÖ½À´Ï´Ù."), last_dye_level+3);
											}
										}
									}
								}
								break;

							#ifdef ENABLE_PVM_BONUS
							case 78010:
								{
									LPITEM item2;

									BYTE bApply = APPLY_HIT_DAMAGE_BONUS_PVM;

									short sMaxValue = 10;
									short sValue = 1;

									int changeValue = number(1,150);
									if (changeValue <= 12)
										sValue = number(6,10);
									else if (changeValue >= 100 && changeValue <= 115)
										sValue = number(1,8);
									else
										sValue = number(1,5);

									if (!IsValidItemPosition(DestCell) || !(item2 = GetInventoryItem(wDestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("esya_efsunlanamaz"));
										return false;
									}

									if (item2->GetAttributeCount() < 5)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("en_az_5_efsun"));
										return false;
									}

									if (item2->GetType() == ITEM_WEAPON || item2->GetType() == ITEM_ARMOR)
									{
										short sCheckValue = item2->GetAttributeValue(6);
										if (sCheckValue >= sMaxValue)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("son_seviyeye_ulasmis"));
											return false;
										}

										if (item2->GetAttributeType(6) == bApply)
										{
											item2->RemoveAttributeAt(6);
											item2->SetForceAttribute(6, bApply, sValue);
										}
										else
										{
											item2->SetForceAttribute(6, bApply, sValue);
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Pvm efsun silah, zirh, kask, kalkan ve takýlara uygulanir."));
										return false;
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("efsun_basariyla_degistirildi"));
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 78011:
								{
									LPITEM item2;

									BYTE bApply = APPLY_HIT_DAMAGE_BONUS_PVM;

									short sMaxValue = 20;
									short sValue = 1;

									int changeValue = number(1,150);
									if (changeValue >= 80 && changeValue <= 85)
										sValue = number(16,20);
									else if (changeValue >= 90 && changeValue <= 110)
										sValue = number(10,17);
									else
										sValue = number(10,15);

									if (!IsValidItemPosition(DestCell) || !(item2 = GetInventoryItem(wDestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("esya_efsunlanamaz"));
										return false;
									}

									if (item2->GetAttributeCount() < 5)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("en_az_5_efsun"));
										return false;
									}

									short sCheckValue = item2->GetAttributeValue(6);
									if (sCheckValue < 10)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Esyanizda enaz 10 pvm olmasi gerekiyor"));
										return false;
									}

									if (item2->GetType() == ITEM_WEAPON || item2->GetType() == ITEM_ARMOR)
									{
										short sCheckValue = item2->GetAttributeValue(6);
										if (sCheckValue >= sMaxValue)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("son_seviyeye_ulasmis"));
											return false;
										}

										if (item2->GetAttributeType(6) == bApply)
										{
											item2->RemoveAttributeAt(6);
											item2->SetForceAttribute(6, bApply, sValue);
										}
										else
										{
											item2->SetForceAttribute(6, bApply, sValue);
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Pvm efsun silah, zirh, kask, kalkan ve takýlara uygulanir."));
										return false;
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("efsun_basariyla_degistirildi"));
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 78012:
								{
									LPITEM item2;

									BYTE bApply = APPLY_HIT_DAMAGE_BONUS_PVM;

									short sMaxValue = 30;
									short sValue = 1;

									int changeValue = number(1,150);
									if (changeValue >= 80 && changeValue <= 85)
										sValue = number(26,30);
									else if (changeValue >= 90 && changeValue <= 110)
										sValue = number(20,27);
									else
										sValue = number(20,25);

									if (!IsValidItemPosition(DestCell) || !(item2 = GetInventoryItem(wDestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("esya_efsunlanamaz"));
										return false;
									}

									if (item2->GetAttributeCount() < 5)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("en_az_5_efsun"));
										return false;
									}

									short sCheckValue = item2->GetAttributeValue(6);
									if (sCheckValue < 20)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Esyanizda enaz 20 pvm olmasi gerekiyor"));
										return false;
									}

									if (item2->GetType() == ITEM_WEAPON || item2->GetType() == ITEM_ARMOR)
									{
										short sCheckValue = item2->GetAttributeValue(6);
										if (sCheckValue >= sMaxValue)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("son_seviyeye_ulasmis"));
											return false;
										}

										if (item2->GetAttributeType(6) == bApply)
										{
											item2->RemoveAttributeAt(6);
											item2->SetForceAttribute(6, bApply, sValue);
										}
										else
										{
											item2->SetForceAttribute(6, bApply, sValue);
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Pvm efsun silah, zirh, kask, kalkan ve takýlara uygulanir."));
										return false;
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("efsun_basariyla_degistirildi"));
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 25055:
								{
									LPITEM item2;
									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped()) // her ihtimale karsi :)
										return false;

									if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_engel"));
										return false;
									}

									#ifdef ENABLE_IKASHOP_RENEWAL
									if (GetIkarusShopGuest() || GetIkarusAuctionGuest() || GetIkarusShopSafebox())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_engel"));
										return false;
									}
									#endif

									if (!(item2->GetType() == ITEM_WEAPON || item2->GetType() == ITEM_ARMOR))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Pvm efsun silah, zirh, kask, kalkan ve takýlara uygulanir."));
										return false;
									}

									short sMaxValue = 10;

									short sValue = item2->GetAttributeValue(6);
									if (sValue >= sMaxValue)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Son pvm bonusuna ulastin!"));
										return false;
									}

									item2->SetForceAttribute(6, APPLY_HIT_DAMAGE_BONUS_PVM, (sValue+1));
									item->SetCount(item->GetCount() - 1);
									return true;
								}
								break;

							case 25056:
								{
									LPITEM item2;
									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped()) // her ihtimale karsi :)
										return false;

									if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_engel"));
										return false;
									}

									#ifdef ENABLE_IKASHOP_RENEWAL
									if (GetIkarusShopGuest() || GetIkarusAuctionGuest() || GetIkarusShopSafebox())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_engel"));
										return false;
									}
									#endif

									if (!(item2->GetType() == ITEM_WEAPON || item2->GetType() == ITEM_ARMOR))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Pvm efsun silah, zirh, kask, kalkan ve takýlara uygulanir."));
										return false;
									}

									short sMaxValue = 20;

									short sValue = item2->GetAttributeValue(6);
									if (sValue >= sMaxValue)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Son pvm bonusuna ulastin!"));
										return false;
									}
									else if (sValue < 10)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bunu kullanabilmek icin esyanizda en az 10 pvm efsunu gereklidir!"));
										return false;
									}

									item2->SetForceAttribute(6, APPLY_HIT_DAMAGE_BONUS_PVM, (sValue+1));
									item->SetCount(item->GetCount() - 1);
									return true;
								}
								break;

							case 25057:
								{
									LPITEM item2;
									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped()) // her ihtimale karsi :)
										return false;

									if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_engel"));
										return false;
									}

									#ifdef ENABLE_IKASHOP_RENEWAL
									if (GetIkarusShopGuest() || GetIkarusAuctionGuest() || GetIkarusShopSafebox())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_engel"));
										return false;
									}
									#endif

									if (!(item2->GetType() == ITEM_WEAPON || item2->GetType() == ITEM_ARMOR))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Pvm efsun silah, zirh, kask, kalkan ve takýlara uygulanir."));
										return false;
									}

									short sMaxValue = 30;

									short sValue = item2->GetAttributeValue(6);
									if (sValue >= sMaxValue)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Son pvm bonusuna ulastin!"));
										return false;
									}
									else if (sValue < 20)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bunu kullanabilmek icin esyanizda en az 20 pvm efsunu gereklidir!"));
										return false;
									}

									item2->SetForceAttribute(6, APPLY_HIT_DAMAGE_BONUS_PVM, (sValue+1));
									item->SetCount(item->GetCount() - 1);
									return true;
								}
								break;
							#endif

							#ifdef ENABLE_BECERI_ORTALAMA_EFSUN
							case 78055:
								{
									if (quest::CQuestManager::instance().GetEventFlag("ortalamaefsun") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									LPITEM item2;
									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped()) // her ihtimale karsi :)
										return false;

									if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_engel"));
										return false;
									}

									#ifdef ENABLE_IKASHOP_RENEWAL
									if (GetIkarusShopGuest() || GetIkarusAuctionGuest() || GetIkarusShopSafebox())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_engel"));
										return false;
									}
									#endif

									if (item2->GetType() != ITEM_WEAPON)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("sadece_silahlara_bu_islemi_yapabilirsiniz"));
										return false;
									}

									BYTE bS = APPLY_SKILL_DAMAGE_BONUS;
									BYTE bH = APPLY_NORMAL_HIT_DAMAGE_BONUS;

									if (item2->GetAttributeType(0) != bS && item2->GetAttributeType(0) != bH)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu nesneyi sadece ortalama hasara sahip olabilecek esyalarda kullanabilirsin !");
										return false;
									}

									if (item2->GetAttributeType(0) != bH && item2->GetAttributeType(1) != bS)
									{
										ChatPacket(CHAT_TYPE_INFO, "Esyanizda ortalama zarar ve beceri hasari efsunu yok bu sekilde bu esyayi kullanmak efsun kaybina sebep olur !");
										return false;
									}

									short attrvalue2 = number(15, 28); // - beceri hasari ayarligi min-max
									short attrvalue1 = 0;

									int sayi1 = number(1, 60);
									if (sayi1 == 60)
										attrvalue1 = 60;
									else if (sayi1 == 49)
										attrvalue1 = 59;
									else if (sayi1 == 48)
										attrvalue1 = 58;
									else if (sayi1 == 47)
										attrvalue1 = 56;
									else if (sayi1 == 46)
										attrvalue1 = 55;
									else
										attrvalue1 = number(45, 54);

									short becerihasari = attrvalue2;
									becerihasari -= attrvalue2;
									becerihasari -= attrvalue2;

									item2->ChangeAttribute();

									item2->SetForceAttribute(0, bH, attrvalue1);
									item2->SetForceAttribute(1, bS, becerihasari);

									item->SetCount(item->GetCount() - 1);
									return true;
								}
								break;

							case 78056:
								{
									if (quest::CQuestManager::instance().GetEventFlag("beceriefsun") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									LPITEM item2;
									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped()) // her ihtimale karsi :)
										return false;

									if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_engel"));
										return false;
									}

									#ifdef ENABLE_IKASHOP_RENEWAL
									if (GetIkarusShopGuest() || GetIkarusAuctionGuest() || GetIkarusShopSafebox())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_engel"));
										return false;
									}
									#endif

									if (item2->GetType() != ITEM_WEAPON)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("sadece_silahlara_bu_islemi_yapabilirsiniz"));
										return false;
									}

									BYTE bS = APPLY_SKILL_DAMAGE_BONUS;
									BYTE bH = APPLY_NORMAL_HIT_DAMAGE_BONUS;

									if (item2->GetAttributeType(0) != bS && item2->GetAttributeType(0) != bH)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu nesneyi sadece beceri hasara sahip olabilecek esyalarda kullanabilirsin !");
										return false;
									}

									if (item2->GetAttributeType(0) != bH && item2->GetAttributeType(1) != bS)
									{
										ChatPacket(CHAT_TYPE_INFO, "Esyanizda ortalama zarar ve beceri hasari efsunu yok bu sekilde bu esyayi kullanmak efsun kaybina sebep olur !");
										return false;
									}

									short attrvalue1 = number(20, 27); // esyaya gelecebilecek  - ortalama hasar araligi min-max
									short attrvalue2 = 0;

									int sayi1 = number(1, 60);
									if (sayi1 == 60)
										attrvalue2 = 30;
									else if (sayi1 == 49)
										attrvalue2 = 28;
									else if (sayi1 == 48)
										attrvalue2 = 27;
									else if (sayi1 == 47)
										attrvalue2 = 26;
									else if (sayi1 == 46)
										attrvalue2 = 25;
									else
										attrvalue2 = number(18, 24);

									short ortalamazarar = attrvalue1;
									ortalamazarar -= attrvalue1;
									ortalamazarar -= attrvalue1;

									item2->ChangeAttribute();

									item2->SetForceAttribute(0, bH, ortalamazarar);
									item2->SetForceAttribute(1, bS, attrvalue2);

									item->SetCount(item->GetCount() - 1);
									return true;
								}
								break;
							#endif

							#ifdef ENABLE_DEFENCE_SHIELD
							case 90406:
								{
									if (!FindAffect(AFFECT_DEFENCE_SHIELD))
									{
										AddAffect(AFFECT_DEFENCE_SHIELD, POINT_NONE, 0, AFF_DEFENCE_SHIELD, 60 * 60 * 24 * 7, 0, false);
										item->SetCount(item->GetCount() - 1);
										ChatPacket(CHAT_TYPE_INFO, "Koruma aktif edildi.");
										return true;
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Zaten koruman mevcut.");
										return false;
									}
								}
								break;

							case 90407:
								{
									if (!FindAffect(AFFECT_DEFENCE_SHIELD))
									{
										AddAffect(AFFECT_DEFENCE_SHIELD, POINT_NONE, 0, AFF_DEFENCE_SHIELD, 60 * 60 * 24 * 1, 0, false);
										item->SetCount(item->GetCount() - 1);
										ChatPacket(CHAT_TYPE_INFO, "Koruma aktif edildi.");
										return true;
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Zaten koruman mevcut.");
										return false;
									}
								}
								break;
							#endif

							#ifdef ENABLE_IKASHOP_RENEWAL
							case ITEM_SHOP_SEARCH_NORMAL:
								{
									SetPremiumSearch(false);
									ChatPacket(CHAT_TYPE_COMMAND, "OpenShopSearch 0");
								}
								break;

							case ITEM_SHOP_SEARCH_PREMIUM:
								{
									SetPremiumSearch(true);
									ChatPacket(CHAT_TYPE_COMMAND, "OpenShopSearch 1");
								}
								break;
							#endif

							case ITEM_NEW_YEAR_GREETING_VNUM:
								{
									DWORD dwBoxVnum = ITEM_NEW_YEAR_GREETING_VNUM;
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets;
									int count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count
										#ifdef ENABLE_LOCKED_SYSTEM
										, item->GetLocked()
										#endif
									))
									{
										for (int i = 0; i < count; i++)
										{
											if (dwVnums[i] == CSpecialItemGroup::GOLD)
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
										}

										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case ITEM_VALENTINE_ROSE:
							case ITEM_VALENTINE_CHOCOLATE:
								{
									DWORD dwBoxVnum = item->GetVnum();
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int count = 0;

									if (((item->GetVnum() == ITEM_VALENTINE_ROSE) && (SEX_MALE==GET_SEX(this))) ||
										((item->GetVnum() == ITEM_VALENTINE_CHOCOLATE) && (SEX_FEMALE==GET_SEX(this))))
									{

										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» ¿­ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count
										#ifdef ENABLE_LOCKED_SYSTEM
										, item->GetLocked()
										#endif
									))
									{
										item->SetCount(item->GetCount()-1);
									}
								}
								break;

							case ITEM_WHITEDAY_CANDY:
							case ITEM_WHITEDAY_ROSE:
								{
									DWORD dwBoxVnum = item->GetVnum();
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int count = 0;

									if (((item->GetVnum() == ITEM_WHITEDAY_CANDY) && (SEX_MALE==GET_SEX(this))) ||
										((item->GetVnum() == ITEM_WHITEDAY_ROSE) && (SEX_FEMALE==GET_SEX(this))))
									{

										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» ¿­ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count
										#ifdef ENABLE_LOCKED_SYSTEM
										, item->GetLocked()
										#endif
									))
									{
										item->SetCount(item->GetCount()-1);
									}
								}
								break;

							case 50011:
								{
									DWORD dwBoxVnum = 50011;
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count
										#ifdef ENABLE_LOCKED_SYSTEM
										, item->GetLocked()
										#endif
									))
									{
										for (int i = 0; i < count; i++)
										{
											char buf[50 + 1];
											snprintf(buf, sizeof(buf), "%u %u", dwVnums[i], dwCounts[i]);
											LogManager::instance().ItemLog(this, item, "MOONLIGHT_GET", buf);

											//ITEM_MANAGER::instance().RemoveItem(item);
											item->SetCount(item->GetCount() - 1);

											switch (dwVnums[i])
											{
											case CSpecialItemGroup::GOLD:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
												break;

											case CSpecialItemGroup::EXP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ºÎÅÍ ½ÅºñÇÑ ºûÀÌ ³ª¿É´Ï´Ù."));
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%dÀÇ °æÇèÄ¡¸¦ È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
												break;

											case CSpecialItemGroup::MOB:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
												break;

											case CSpecialItemGroup::SLOW:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â »¡°£ ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ ¿òÁ÷ÀÌ´Â ¼Óµµ°¡ ´À·ÁÁ³½À´Ï´Ù!"));
												break;

											case CSpecialItemGroup::DRAIN_HP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ°¡ °©ÀÚ±â Æø¹ßÇÏ¿´½À´Ï´Ù! »ý¸í·ÂÀÌ °¨¼ÒÇß½À´Ï´Ù."));
												break;

											case CSpecialItemGroup::POISON:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
												break;

											#ifdef ENABLE_WOLFMAN_CHARACTER
											case CSpecialItemGroup::BLEEDING:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
												break;
											#endif

											case CSpecialItemGroup::MOB_GROUP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
												break;

											default:
												if (item_gets[i])
												{
													if (dwCounts[i] > 1)
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ %d °³ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(), dwCounts[i]);
													else
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName());
												}
												break;
											}
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¾Æ¹«°Íµµ ¾òÀ» ¼ö ¾ø¾ú½À´Ï´Ù."));
										return false;
									}
								}
								break;

							case ITEM_GIVE_STAT_RESET_COUNT_VNUM:
								{
									if (GetProtectTime("kanhapi") > get_global_time())
									{
										ChatPacket(CHAT_TYPE_INFO, "Yavaþ olmalýsýn! %d saniye sonra deneyebilirsiniz.", GetProtectTime("kanhapi") - get_global_time());
										return false;
									}

									PointChange(POINT_STAT_RESET_COUNT, 1);
									item->SetCount(item->GetCount()-1);
									SetProtectTime("kanhapi", get_global_time() + 5);
								}
								break;

							case 50107:
								{
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
										return false;
									}
									#ifdef ENABLE_NEWSTUFF
									else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
										return false;
									}
									#endif

									EffectPacket(SE_CHINA_FIREWORK);
									#ifdef ENABLE_FIREWORK_STUN
									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
									#endif
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50108:
								{
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
										return false;
									}
									#ifdef ENABLE_NEWSTUFF
									else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
										return false;
									}
									#endif

									EffectPacket(SE_SPIN_TOP);
									#ifdef ENABLE_FIREWORK_STUN
									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
									#endif
									item->SetCount(item->GetCount()-1);
								}
								break;

							case ITEM_WONSO_BEAN_VNUM:
								PointChange(POINT_HP, GetMaxHP() - GetHP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_SUGAR_VNUM:
								PointChange(POINT_SP, GetMaxSP() - GetSP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_FRUIT_VNUM:
								PointChange(POINT_STAMINA, GetMaxStamina()-GetStamina());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_ELK_VNUM:
								{
									int iGold = item->GetSocket(0);
									ITEM_MANAGER::instance().RemoveItem(item);
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), iGold);
									#ifdef ENABLE_FULL_YANG
									GoldChange(iGold);
									#else
									PointChange(POINT_GOLD, iGold);
									#endif
								}
								break;

							case 70021:
								{
									int HealPrice = quest::CQuestManager::instance().GetEventFlag("MonarchHealGold");
									if (HealPrice == 0)
										HealPrice = 2000000;

									if (CMonarch::instance().HealMyEmpire(this, HealPrice))
									{
										char szNotice[256];
										snprintf(szNotice, sizeof(szNotice), LC_TEXT("±ºÁÖÀÇ Ãàº¹À¸·Î ÀÌÁö¿ª %s À¯Àú´Â HP,SP°¡ ¸ðµÎ Ã¤¿öÁý´Ï´Ù."), EMPIRE_NAME(GetEmpire()));
										SendNoticeMap(szNotice, GetMapIndex(), false);

										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±ºÁÖÀÇ Ãàº¹À» »ç¿ëÇÏ¿´½À´Ï´Ù."));
									}
								}
								break;

							case 27995:
								{
								}
								break;

							case 71092 :
								{
									if (m_pkChrTarget != NULL)
									{
										if (m_pkChrTarget->IsPolymorphed())
										{
											m_pkChrTarget->SetPolymorph(0);
											m_pkChrTarget->RemoveAffect(AFFECT_POLYMORPH);
										}
									}
									else
									{
										if (IsPolymorphed())
										{
											SetPolymorph(0);
											RemoveAffect(AFFECT_POLYMORPH);
										}
									}
								}
								break;

							#ifdef ENABLE_EXTRA_SOCKET_SYSTEM
							case ITEM_UPGRADE_SOCKET_WEAPON:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped())
										return false;

									if (item2->GetType() == ITEM_WEAPON)
									{
										if (item2->GetLevelLimit() < 55)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("USE_OPEN_SOCKET_WARNING_LV55"));
											return false;
										}

										if (item2->GetSocket(3) > 0)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("USE_OPEN_SOCKET_WARNING_SOCKET3"));
											return false;
										}

										if (number(1,100) <= 40)
										{
											item2->SetSocket(3,1);
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SUCCESS_OPEN_SOCKET"));
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "Soket ekleme basarisiz.");
										}
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("USE_OPEN_SOCKET_ONLY_WEAPON"));
										return false;
									}
								}
								break;

							case ITEM_UPGRADE_SOCKET_ARMOR:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped())
										return false;

									if (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY)
									{
										if (item2->GetLevelLimit() < 55)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("USE_OPEN_SOCKET_WARNING_LV55"));
											return false;
										}

										if (item2->GetSocket(3) > 0)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("USE_OPEN_SOCKET_WARNING_SOCKET3"));
											return false;
										}

										if (number(1,100) <= 30)
										{
											item2->SetSocket(3,1);
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SUCCESS_OPEN_SOCKET"));
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "Soket ekleme basarisiz.");
										}

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("USE_OPEN_SOCKET_ONLY_ARMOR"));
										return false;
									}
								}
								break;

							case ITEM_UPGRADE_SOCKET_MIX:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped())
										return false;

									if ((item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_WRIST)
										|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_EAR)
										|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_NECK)
										|| (item2->GetType() == ITEM_BELT)
									)
									{
										if (item2->GetAccessorySocketMaxGrade() > 3)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("USE_OPEN_SOCKET_WARNING_SOCKET3"));
											return false;
										}

										if (number(1,100) <= 30) // drop
										{
											item2->SetAccessorySocketMaxGrade(item2->GetAccessorySocketMaxGrade() + 1);
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SUCCESS_OPEN_SOCKET"));
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "Soket ekleme basarisiz.");
										}

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu nesneye yapamazsiniz.");
										return false;
									}
								}
								break;
							#endif

							#ifdef ENABLE_EVOLUTION_SYSTEM
							case 81504:
								{
									LPITEM item2;
									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped())
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (IS_SET(item2->GetFlag(), ITEM_FLAG_ITEM_RARE_ABILITY) && item2->GetType() == ITEM_WEAPON)
									{
										if (item2->GetEvolution() >= ITEM_EVOLUTION_MAX)
										{
											ChatPacket(CHAT_TYPE_INFO, "Max Evrim Puanina ulastiniz.");
											return false;
										}

										item2->SetEvolution(8750);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu silaha evrim ekleyemezsiniz.");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81505:
								{
									LPITEM item2;
									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped())
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (IS_SET(item2->GetFlag(), ITEM_FLAG_ITEM_RARE_ABILITY) && item2->GetType() == ITEM_ARMOR)
									{
										if (item2->GetEvolution() >= ITEM_EVOLUTION_MAX)
										{
											ChatPacket(CHAT_TYPE_INFO, "Max Evrim Puanina ulastiniz.");
											return false;
										}

										item2->SetEvolution(8750);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu silaha evrim ekleyemezsiniz.");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81506:
								{
									LPITEM item2;
									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped())
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (IS_SET(item2->GetFlag(), ITEM_FLAG_ITEM_RARE_ABILITY) && item2->GetType() == ITEM_ARMOR)
									{
										if (item2->GetEvolution() >= ITEM_EVOLUTION_MAX)
										{
											ChatPacket(CHAT_TYPE_INFO, "Max Evrim Puanina ulastiniz.");
											return false;
										}

										BYTE bValue = number(1, 98);

										item2->SetEvolution(item2->GetEvolution() + bValue);
										ChatPacket(CHAT_TYPE_INFO, "Evrim Puanina Kazandiniz. %d", bValue);

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu silaha evrim ekleyemezsiniz.");
										return false;
									}
								}
								break;
							#endif

#ifdef ENABLE_GIVE_TIME_COSTUME_SYSTEM
							case 11118:
							{
								LPITEM targetWeapon;
								if (!IsValidItemPosition(DestCell) || !(targetWeapon = GetItem(DestCell)))
									return false;

								if (PreventTradeWindow(WND_ALL))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
									return false;
								}

								if (targetWeapon->IsExchanging() == true)
									return false;

								if (targetWeapon->IsEquipped())
									return false;

								if (targetWeapon->GetVnum() == 45005
									|| targetWeapon->GetVnum() == 41315
									|| targetWeapon->GetVnum() == 45006
									|| targetWeapon->GetVnum() == 41024
									|| targetWeapon->GetVnum() == 41023
									|| targetWeapon->GetVnum() == 45009
									|| targetWeapon->GetVnum() == 45010
									|| targetWeapon->GetVnum() == 53020
									|| targetWeapon->GetVnum() == 53021
									|| targetWeapon->GetVnum() == 71226
									|| targetWeapon->GetVnum() == 71227
									|| targetWeapon->GetVnum() == 41900
									|| targetWeapon->GetVnum() == 41901
									|| targetWeapon->GetVnum() == 45902
									|| targetWeapon->GetVnum() == 45903
									|| targetWeapon->GetVnum() == 71226
									|| targetWeapon->GetVnum() == 53021
									|| targetWeapon->GetVnum() == 41316)
									return false;

								if (targetWeapon->GetSubType() == COSTUME_WEAPON)
								{
									targetWeapon->SetSocket(0, (targetWeapon->GetSocket(0) + 259200));
									ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
									item->SetCount(item->GetCount() - 1);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
									return false;
								}
								break;
							}
							break;

							case 11119:
								{
									LPITEM targetHair;
									if (!IsValidItemPosition(DestCell) || !(targetHair = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetHair->IsExchanging() == true)
										return false;

									if (targetHair->IsEquipped())
										return false;

									if (targetHair->GetVnum() == 45005
										|| targetHair->GetVnum() == 41315
										|| targetHair->GetVnum() == 45006
										|| targetHair->GetVnum() == 41024
										|| targetHair->GetVnum() == 41023
										|| targetHair->GetVnum() == 45009
										|| targetHair->GetVnum() == 45010
										|| targetHair->GetVnum() == 53020
										|| targetHair->GetVnum() == 53021
										|| targetHair->GetVnum() == 71226
										|| targetHair->GetVnum() == 71227
										|| targetHair->GetVnum() == 41900
										|| targetHair->GetVnum() == 41901
										|| targetHair->GetVnum() == 45902
										|| targetHair->GetVnum() == 45903
										|| targetHair->GetVnum() == 71226
										|| targetHair->GetVnum() == 53021
										|| targetHair->GetVnum() == 41316)
										return false;

									if (targetHair->GetSubType() == COSTUME_HAIR)
									{
										targetHair->SetSocket(0, (targetHair->GetSocket(0) + 259200));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;

							case 11120:
								{
									LPITEM targetCostume;
									if (!IsValidItemPosition(DestCell) || !(targetCostume = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetCostume->IsExchanging() == true)
										return false;

									if (targetCostume->IsEquipped())
										return false;

									if (targetCostume->GetVnum() == 45005
										|| targetCostume->GetVnum() == 41315
										|| targetCostume->GetVnum() == 45006
										|| targetCostume->GetVnum() == 41024
										|| targetCostume->GetVnum() == 41023
										|| targetCostume->GetVnum() == 45009
										|| targetCostume->GetVnum() == 45010
										// || targetCostume->GetVnum() == 53020
										|| targetCostume->GetVnum() == 53021
										|| targetCostume->GetVnum() == 71226
										|| targetCostume->GetVnum() == 71227
										|| targetCostume->GetVnum() == 41900
										|| targetCostume->GetVnum() == 41901
										|| targetCostume->GetVnum() == 45902
										|| targetCostume->GetVnum() == 45903
										|| targetCostume->GetVnum() == 71226
										|| targetCostume->GetVnum() == 53021
										|| targetCostume->GetVnum() == 41316)
										return false;

									if (targetCostume->GetType() == ITEM_COSTUME && targetCostume->GetSubType() == COSTUME_BODY)
									{
										targetCostume->SetSocket(0, (targetCostume->GetSocket(0) + 259200));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;

							case 11121:
								{
									LPITEM targetAll;
									if (!IsValidItemPosition(DestCell) || !(targetAll = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetAll->IsExchanging() == true)
										return false;

									if (targetAll->IsEquipped())
										return false;

									if (targetAll->GetVnum() == 45005
										|| targetAll->GetVnum() == 41315
										|| targetAll->GetVnum() == 45006
										|| targetAll->GetVnum() == 41024
										|| targetAll->GetVnum() == 41023
										|| targetAll->GetVnum() == 45009
										|| targetAll->GetVnum() == 45010
										|| targetAll->GetVnum() == 53020
										|| targetAll->GetVnum() == 53021
										|| targetAll->GetVnum() == 71226
										|| targetAll->GetVnum() == 71227
										|| targetAll->GetVnum() == 41900
										|| targetAll->GetVnum() == 41901
										|| targetAll->GetVnum() == 45902
										|| targetAll->GetVnum() == 45903
										|| targetAll->GetVnum() == 71226
										|| targetAll->GetVnum() == 53021
										|| targetAll->GetVnum() == 41316)
										return false;
									

									if (targetAll->GetType() == ITEM_COSTUME
									#ifdef ENABLE_ACCE_COSTUME_SYSTEM
									&& targetAll->GetSubType() != COSTUME_ACCE
									#endif
									)
									{
										targetAll->SetSocket(0, (targetAll->GetSocket(0) + 259200));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;

							case 11128:
							{
								LPITEM targetWeapon;
								if (!IsValidItemPosition(DestCell) || !(targetWeapon = GetItem(DestCell)))
									return false;

								if (PreventTradeWindow(WND_ALL))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
									return false;
								}

								if (targetWeapon->IsExchanging() == true)
									return false;

								if (targetWeapon->IsEquipped())
									return false;

								if (targetWeapon->GetVnum() == 45005
									|| targetWeapon->GetVnum() == 41315
									|| targetWeapon->GetVnum() == 45006
									|| targetWeapon->GetVnum() == 41024
									|| targetWeapon->GetVnum() == 41023
									|| targetWeapon->GetVnum() == 45009
									|| targetWeapon->GetVnum() == 45010
									|| targetWeapon->GetVnum() == 53020
									|| targetWeapon->GetVnum() == 53021
									|| targetWeapon->GetVnum() == 71226
									|| targetWeapon->GetVnum() == 71227
									|| targetWeapon->GetVnum() == 41900
									|| targetWeapon->GetVnum() == 41901
									|| targetWeapon->GetVnum() == 45902
									|| targetWeapon->GetVnum() == 45903
									|| targetWeapon->GetVnum() == 71226
									|| targetWeapon->GetVnum() == 53021
									|| targetWeapon->GetVnum() == 41316)
									return false;


								if (targetWeapon->GetSubType() == COSTUME_WEAPON)
								{
									targetWeapon->SetSocket(0, (targetWeapon->GetSocket(0) + 604800));
									ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
									item->SetCount(item->GetCount() - 1);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
									return false;
								}
								break;
							}
							break;

							case 11129:
								{
									LPITEM targetHair;
									if (!IsValidItemPosition(DestCell) || !(targetHair = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetHair->IsExchanging() == true)
										return false;

									if (targetHair->IsEquipped())
										return false;

									if (targetHair->GetVnum() == 45005
										|| targetHair->GetVnum() == 41315
										|| targetHair->GetVnum() == 45006
										|| targetHair->GetVnum() == 41024
										|| targetHair->GetVnum() == 41023
										|| targetHair->GetVnum() == 45009
										|| targetHair->GetVnum() == 45010
										|| targetHair->GetVnum() == 53020
										|| targetHair->GetVnum() == 53021
										|| targetHair->GetVnum() == 71226
										|| targetHair->GetVnum() == 71227
										|| targetHair->GetVnum() == 41900
										|| targetHair->GetVnum() == 41901
										|| targetHair->GetVnum() == 45902
										|| targetHair->GetVnum() == 45903
										|| targetHair->GetVnum() == 71226
										|| targetHair->GetVnum() == 53021
										|| targetHair->GetVnum() == 41316)
										return false;

									if (targetHair->GetSubType() == COSTUME_HAIR)
									{
										targetHair->SetSocket(0, (targetHair->GetSocket(0) + 604800));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;

							case 11130:
								{
									LPITEM targetCostume;
									if (!IsValidItemPosition(DestCell) || !(targetCostume = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetCostume->IsExchanging() == true)
										return false;

									if (targetCostume->IsEquipped())
										return false;

									if (targetCostume->GetVnum() == 45005
										|| targetCostume->GetVnum() == 41315
										|| targetCostume->GetVnum() == 45006
										|| targetCostume->GetVnum() == 41024
										|| targetCostume->GetVnum() == 41023
										|| targetCostume->GetVnum() == 45009
										|| targetCostume->GetVnum() == 45010
										// || targetCostume->GetVnum() == 53020
										|| targetCostume->GetVnum() == 53021
										|| targetCostume->GetVnum() == 71226
										|| targetCostume->GetVnum() == 71227
										|| targetCostume->GetVnum() == 41900
										|| targetCostume->GetVnum() == 41901
										|| targetCostume->GetVnum() == 45902
										|| targetCostume->GetVnum() == 45903
										|| targetCostume->GetVnum() == 71226
										|| targetCostume->GetVnum() == 53021
										|| targetCostume->GetVnum() == 41316)
										return false;

									if (targetCostume->GetType() == ITEM_COSTUME && targetCostume->GetSubType() == COSTUME_BODY)
									{
										targetCostume->SetSocket(0, (targetCostume->GetSocket(0) + 604800));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;

							case 11131:
								{
									LPITEM targetAll;
									if (!IsValidItemPosition(DestCell) || !(targetAll = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetAll->IsExchanging() == true)
										return false;

									if (targetAll->IsEquipped())
										return false;


									if (targetAll->GetVnum() == 45005
										|| targetAll->GetVnum() == 41315
										|| targetAll->GetVnum() == 45006
										|| targetAll->GetVnum() == 41024
										|| targetAll->GetVnum() == 41023
										|| targetAll->GetVnum() == 45009
										|| targetAll->GetVnum() == 45010
										|| targetAll->GetVnum() == 53020
										|| targetAll->GetVnum() == 53021
										|| targetAll->GetVnum() == 71226
										|| targetAll->GetVnum() == 71227
										|| targetAll->GetVnum() == 41900
										|| targetAll->GetVnum() == 41901
										|| targetAll->GetVnum() == 45902
										|| targetAll->GetVnum() == 45903
										|| targetAll->GetVnum() == 71226
										|| targetAll->GetVnum() == 53021
										|| targetAll->GetVnum() == 41316)
										return false;

									if (targetAll->GetType() == ITEM_COSTUME
									#ifdef ENABLE_ACCE_COSTUME_SYSTEM
									&& targetAll->GetSubType() != COSTUME_ACCE
									#endif
									)
									{
										targetAll->SetSocket(0, (targetAll->GetSocket(0) + 604800));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;

							case 95600:
							{
								LPITEM targetWeapon;
								if (!IsValidItemPosition(DestCell) || !(targetWeapon = GetItem(DestCell)))
									return false;

								if (PreventTradeWindow(WND_ALL))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
									return false;
								}

								if (targetWeapon->IsExchanging() == true)
									return false;

								if (targetWeapon->IsEquipped())
									return false;

								if (targetWeapon->GetVnum() == 45005
									|| targetWeapon->GetVnum() == 41315
									|| targetWeapon->GetVnum() == 45006
									|| targetWeapon->GetVnum() == 41024
									|| targetWeapon->GetVnum() == 41023
									|| targetWeapon->GetVnum() == 45009
									|| targetWeapon->GetVnum() == 45010
									|| targetWeapon->GetVnum() == 53020
									|| targetWeapon->GetVnum() == 53021
									|| targetWeapon->GetVnum() == 71226
									|| targetWeapon->GetVnum() == 71227
									|| targetWeapon->GetVnum() == 41900
									|| targetWeapon->GetVnum() == 41901
									|| targetWeapon->GetVnum() == 45902
									|| targetWeapon->GetVnum() == 45903
									|| targetWeapon->GetVnum() == 71226
									|| targetWeapon->GetVnum() == 53021
									|| targetWeapon->GetVnum() == 41316)
									return false;

								if (targetWeapon->GetSubType() == COSTUME_WEAPON)
								{
									targetWeapon->SetSocket(0, (targetWeapon->GetSocket(0) + 1209600));
									ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
									item->SetCount(item->GetCount() - 1);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
									return false;
								}
								break;
							}
							break;

							case 95601:
								{
									LPITEM targetHair;
									if (!IsValidItemPosition(DestCell) || !(targetHair = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetHair->IsExchanging() == true)
										return false;

									if (targetHair->IsEquipped())
										return false;

									if (targetHair->GetVnum() == 45005
										|| targetHair->GetVnum() == 41315
										|| targetHair->GetVnum() == 45006
										|| targetHair->GetVnum() == 41024
										|| targetHair->GetVnum() == 41023
										|| targetHair->GetVnum() == 45009
										|| targetHair->GetVnum() == 45010
										|| targetHair->GetVnum() == 53020
										|| targetHair->GetVnum() == 53021
										|| targetHair->GetVnum() == 71226
										|| targetHair->GetVnum() == 71227
										|| targetHair->GetVnum() == 41900
										|| targetHair->GetVnum() == 41901
										|| targetHair->GetVnum() == 45902
										|| targetHair->GetVnum() == 45903
										|| targetHair->GetVnum() == 71226
										|| targetHair->GetVnum() == 53021
										|| targetHair->GetVnum() == 41316)
										return false;
									
									if (targetHair->GetSubType() == COSTUME_HAIR)
									{
										targetHair->SetSocket(0, (targetHair->GetSocket(0) + 1209600));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;

							case 95602:
								{
									LPITEM targetCostume;
									if (!IsValidItemPosition(DestCell) || !(targetCostume = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetCostume->IsExchanging() == true)
										return false;

									if (targetCostume->IsEquipped())
										return false;

									if (targetCostume->GetVnum() == 45005
										|| targetCostume->GetVnum() == 41315
										|| targetCostume->GetVnum() == 45006
										|| targetCostume->GetVnum() == 41024
										|| targetCostume->GetVnum() == 41023
										|| targetCostume->GetVnum() == 45009
										|| targetCostume->GetVnum() == 45010
										// || targetCostume->GetVnum() == 53020
										|| targetCostume->GetVnum() == 53021
										|| targetCostume->GetVnum() == 71226
										|| targetCostume->GetVnum() == 71227
										|| targetCostume->GetVnum() == 41900
										|| targetCostume->GetVnum() == 41901
										|| targetCostume->GetVnum() == 45902
										|| targetCostume->GetVnum() == 45903
										|| targetCostume->GetVnum() == 71226
										|| targetCostume->GetVnum() == 53021
										|| targetCostume->GetVnum() == 41316)
										return false;
										
									if (targetCostume->GetType() == ITEM_COSTUME && targetCostume->GetSubType() == COSTUME_BODY)
									{
										targetCostume->SetSocket(0, (targetCostume->GetSocket(0) + 1209600));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;

							case 95603:
								{
									LPITEM targetAll;
									if (!IsValidItemPosition(DestCell) || !(targetAll = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetAll->IsExchanging() == true)
										return false;

									if (targetAll->IsEquipped())
										return false;

									if (targetAll->GetVnum() == 45005 ||
										targetAll->GetVnum() == 41315 ||
										targetAll->GetVnum() == 45006 ||
										targetAll->GetVnum() == 41316)
										return false;

									if (targetAll->GetType() == ITEM_COSTUME
									#ifdef ENABLE_ACCE_COSTUME_SYSTEM
									&& targetAll->GetSubType() != COSTUME_ACCE
									#endif
									)
									{
										targetAll->SetSocket(0, (targetAll->GetSocket(0) + 1209600));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;

							case 95700:
							{
								LPITEM targetWeapon;
								if (!IsValidItemPosition(DestCell) || !(targetWeapon = GetItem(DestCell)))
									return false;

								if (PreventTradeWindow(WND_ALL))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
									return false;
								}

								if (targetWeapon->IsExchanging() == true)
									return false;

								if (targetWeapon->IsEquipped())
									return false;

								if (targetWeapon->GetVnum() == 45005
									|| targetWeapon->GetVnum() == 41315
									|| targetWeapon->GetVnum() == 45006
									|| targetWeapon->GetVnum() == 41024
									|| targetWeapon->GetVnum() == 41023
									|| targetWeapon->GetVnum() == 45009
									|| targetWeapon->GetVnum() == 45010
									|| targetWeapon->GetVnum() == 53020
									|| targetWeapon->GetVnum() == 53021
									|| targetWeapon->GetVnum() == 71226
									|| targetWeapon->GetVnum() == 71227
									|| targetWeapon->GetVnum() == 41900
									|| targetWeapon->GetVnum() == 41901
									|| targetWeapon->GetVnum() == 45902
									|| targetWeapon->GetVnum() == 45903
									|| targetWeapon->GetVnum() == 71226
									|| targetWeapon->GetVnum() == 53021
									|| targetWeapon->GetVnum() == 41316)
									return false;


								if (targetWeapon->GetSubType() == COSTUME_WEAPON)
								{
									targetWeapon->SetSocket(0, (targetWeapon->GetSocket(0) + 2592000));
									ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
									item->SetCount(item->GetCount() - 1);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
									return false;
								}
								break;
							}
							break;

							case 95701:
								{
									LPITEM targetHair;
									if (!IsValidItemPosition(DestCell) || !(targetHair = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetHair->IsExchanging() == true)
										return false;

									if (targetHair->IsEquipped())
										return false;

									if (targetHair->GetVnum() == 45005
										|| targetHair->GetVnum() == 41315
										|| targetHair->GetVnum() == 45006
										|| targetHair->GetVnum() == 41024
										|| targetHair->GetVnum() == 41023
										|| targetHair->GetVnum() == 45009
										|| targetHair->GetVnum() == 45010
										|| targetHair->GetVnum() == 53020
										|| targetHair->GetVnum() == 53021
										|| targetHair->GetVnum() == 71226
										|| targetHair->GetVnum() == 71227
										|| targetHair->GetVnum() == 41900
										|| targetHair->GetVnum() == 41901
										|| targetHair->GetVnum() == 45902
										|| targetHair->GetVnum() == 45903
										|| targetHair->GetVnum() == 71226
										|| targetHair->GetVnum() == 53021
										|| targetHair->GetVnum() == 41316)
										return false;


									if (targetHair->GetSubType() == COSTUME_HAIR)
									{
										targetHair->SetSocket(0, (targetHair->GetSocket(0) + 2592000));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;

							case 95702:
								{
									LPITEM targetCostume;
									if (!IsValidItemPosition(DestCell) || !(targetCostume = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetCostume->IsExchanging() == true)
										return false;

									if (targetCostume->IsEquipped())
										return false;

									if (targetCostume->GetVnum() == 45005
										|| targetCostume->GetVnum() == 41315
										|| targetCostume->GetVnum() == 45006
										|| targetCostume->GetVnum() == 41024
										|| targetCostume->GetVnum() == 41023
										|| targetCostume->GetVnum() == 45009
										|| targetCostume->GetVnum() == 45010
										// || targetCostume->GetVnum() == 53020
										|| targetCostume->GetVnum() == 53021
										|| targetCostume->GetVnum() == 71226
										|| targetCostume->GetVnum() == 71227
										|| targetCostume->GetVnum() == 41900
										|| targetCostume->GetVnum() == 41901
										|| targetCostume->GetVnum() == 45902
										|| targetCostume->GetVnum() == 45903
										|| targetCostume->GetVnum() == 71226
										|| targetCostume->GetVnum() == 53021
										|| targetCostume->GetVnum() == 41316)
										return false;

									if (targetCostume->GetType() == ITEM_COSTUME && targetCostume->GetSubType() == COSTUME_BODY)
									{
										targetCostume->SetSocket(0, (targetCostume->GetSocket(0) + 2592000));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;

							case 95703:
								{
									LPITEM targetAll;
									if (!IsValidItemPosition(DestCell) || !(targetAll = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetAll->IsExchanging() == true)
										return false;

									if (targetAll->IsEquipped())
										return false;

									if (targetAll->GetVnum() == 45005
										|| targetAll->GetVnum() == 41315
										|| targetAll->GetVnum() == 45006
										|| targetAll->GetVnum() == 41024
										|| targetAll->GetVnum() == 41023
										|| targetAll->GetVnum() == 45009
										|| targetAll->GetVnum() == 45010
										|| targetAll->GetVnum() == 53020
										|| targetAll->GetVnum() == 53021
										|| targetAll->GetVnum() == 71226
										|| targetAll->GetVnum() == 71227
										|| targetAll->GetVnum() == 41900
										|| targetAll->GetVnum() == 41901
										|| targetAll->GetVnum() == 45902
										|| targetAll->GetVnum() == 45903
										|| targetAll->GetVnum() == 71226
										|| targetAll->GetVnum() == 53021
										|| targetAll->GetVnum() == 41316)
										return false;

									if (targetAll->GetType() == ITEM_COSTUME
									#ifdef ENABLE_ACCE_COSTUME_SYSTEM
									&& targetAll->GetSubType() != COSTUME_ACCE
									#endif
									)
									{
										targetAll->SetSocket(0, (targetAll->GetSocket(0) + 2592000));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;
								
							case 95704:
								{
									LPITEM targetAll;
									if (!IsValidItemPosition(DestCell) || !(targetAll = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetAll->IsExchanging() == true)
										return false;

									if (targetAll->IsEquipped())
										return false;

									if (targetAll->GetType() == ITEM_RING)
									{
										targetAll->SetSocket(0, (targetAll->GetSocket(0) + 2592000));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;
								
							case 95705:
								{
									LPITEM targetAll;
									if (!IsValidItemPosition(DestCell) || !(targetAll = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetAll->IsExchanging() == true)
										return false;

									if (targetAll->IsEquipped())
										return false;

									if (targetAll->GetType() == ITEM_RING)
									{
										targetAll->SetSocket(0, (targetAll->GetSocket(0) + 1209600));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;

							case 95706:
								{
									LPITEM targetAll;
									if (!IsValidItemPosition(DestCell) || !(targetAll = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetAll->IsExchanging() == true)
										return false;

									if (targetAll->IsEquipped())
										return false;

									if (targetAll->GetType() == ITEM_RING)
									{
										targetAll->SetSocket(0, (targetAll->GetSocket(0) + 604800));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;		
								
								case 95707:
								{
									LPITEM targetAll;
									if (!IsValidItemPosition(DestCell) || !(targetAll = GetItem(DestCell)))
										return false;

									if (PreventTradeWindow(WND_ALL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
										return false;
									}

									if (targetAll->IsExchanging() == true)
										return false;

									if (targetAll->IsEquipped())
										return false;

									if (targetAll->GetType() == ITEM_RING)
									{
										targetAll->SetSocket(0, (targetAll->GetSocket(0) + 259200));
										ChatPacket(CHAT_TYPE_INFO, "Sure uzatildi");
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu islemi bu esyada gerceklestiremezsin.");
										return false;
									}
									break;
								}
								break;		
								
#endif

							#ifdef ENABLE_PASIF_SKILL
							case 50341:
							case 50342:
							case 50343:
							case 50344:
							case 50345:
							case 50346:
							case 50347:
							case 50348:
							case 50349:
							case 50350:
							case 50351:
							case 50352:
							case 50355:
							case 50358:
							case 50361:
							case 50364:
							{
								if (PreventTradeWindow(WND_ALL))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
									return false;
								}

								if (IsPolymorphed() == true)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
									return false;
								}

								int skillkodu = item->GetValue(0);
								int skilllv = GetSkillLevel(item->GetValue(0));
								int levelaralik1 = item->GetValue(1);
								int levelaralik2 = item->GetValue(2);
								int gecmeorani = item->GetValue(3);

								// ChatPacket(1, "%d %d %d %d %d", skillkodu, skilllv, levelaralik1, levelaralik2, gecmeorani);

								if (skilllv >= levelaralik1 && skilllv < levelaralik2)
								{
									int pct = number(1,100);
									if (pct >= gecmeorani)
									{
										ChatPacket(CHAT_TYPE_INFO, "Gücü içimde hissedebiliyorum! ");
										SetSkillLevel(skillkodu, skilllv+1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Eðitim baþarýsýz! ");
									}

									ComputePoints();
									SkillLevelPacket();

									item->SetCount(item->GetCount() - 1);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO,"Bu beceriyi geliþtirdin! ");
								}
							}
							break;
							#endif

							#ifdef ENABLE_RUZGARIN_AYAKKABI
							case 72701: // Rüzgarýn Ayakkabýlarý
								if (FindAffect(AFFECT_RUZGAR))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
									return false;
								}
								AddAffect(AFFECT_RUZGAR, POINT_MOV_SPEED, 30, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, true);
								item->SetCount(item->GetCount() - 1);
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("AFFECT_RUZGAR"));
								break;
							#endif

							#ifdef ENABLE_BIYOLOG
							case 81800:
								{
									if (quest::CQuestManager::instance().GetEventFlag("biyologbitirici") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									int biolevel = GetQuestFlag("bio.level");
									if (GetLevel() < 30)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog bitiriciye leveliniz uygun degil");
										return false;
									}

									if (biolevel >= bio_max)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog gorevler bitmis");
										return false;
									}

									if (biolevel == 1)
									{
										SetQuestFlag("bio.count", bio_data[biolevel][2]);
										SetQuestFlag("bio.time", 0);
										SetQuestFlag("bio.bitirici", 1);
										do_bio(this, "bio_dra mission 0 0", 0, 0);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO,"Biyolog gorevinize uygun degil. ");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81801:
								{
									if (quest::CQuestManager::instance().GetEventFlag("biyologbitirici") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									int biolevel = GetQuestFlag("bio.level");
									if (GetLevel() < 40)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog bitiriciye leveliniz uygun degil");
										return false;
									}

									if (biolevel >= bio_max)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog gorevler bitmis");
										return false;
									}

									if (biolevel == 2)
									{
										SetQuestFlag("bio.count", bio_data[biolevel][2]);
										SetQuestFlag("bio.time", 0);
										SetQuestFlag("bio.bitirici", 1);
										do_bio(this, "bio_dra mission 0 0", 0, 0);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO,"Biyolog gorevinize uygun degil. ");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81802:
								{
									if (quest::CQuestManager::instance().GetEventFlag("biyologbitirici") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									int biolevel = GetQuestFlag("bio.level");
									if (GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog bitiriciye leveliniz uygun degil");
										return false;
									}

									if (biolevel >= bio_max)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog gorevler bitmis");
										return false;
									}

									if (biolevel == 3)
									{
										SetQuestFlag("bio.count", bio_data[biolevel][2]);
										SetQuestFlag("bio.time", 0);
										SetQuestFlag("bio.bitirici", 1);
										do_bio(this, "bio_dra mission 0 0", 0, 0);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO,"Biyolog gorevinize uygun degil. ");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81803:
								{

									if (quest::CQuestManager::instance().GetEventFlag("biyologbitirici") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									int biolevel = GetQuestFlag("bio.level");
									if (GetLevel() < 60)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog bitiriciye leveliniz uygun degil");
										return false;
									}

									if (biolevel >= bio_max)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog gorevler bitmis");
										return false;
									}

									if (biolevel == 4)
									{
										SetQuestFlag("bio.count", bio_data[biolevel][2]);
										SetQuestFlag("bio.time", 0);
										SetQuestFlag("bio.bitirici", 1);
										do_bio(this, "bio_dra mission 0 0", 0, 0);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO,"Biyolog gorevinize uygun degil. ");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81804:
								{
									if (quest::CQuestManager::instance().GetEventFlag("biyologbitirici") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									int biolevel = GetQuestFlag("bio.level");
									if (GetLevel() < 70)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog bitiriciye leveliniz uygun degil");
										return false;
									}

									if (biolevel >= bio_max)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog gorevler bitmis");
										return false;
									}

									if (biolevel == 5)
									{
										SetQuestFlag("bio.count", bio_data[biolevel][2]);
										SetQuestFlag("bio.time", 0);
										SetQuestFlag("bio.bitirici", 1);
										do_bio(this, "bio_dra mission 0 0", 0, 0);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO,"Biyolog gorevinize uygun degil. ");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81805:
								{
									if (quest::CQuestManager::instance().GetEventFlag("biyologbitirici") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									int biolevel = GetQuestFlag("bio.level");
									if (GetLevel() < 80)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog bitiriciye leveliniz uygun degil");
										return false;
									}

									if (biolevel >= bio_max)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog gorevler bitmis");
										return false;
									}

									if (biolevel == 6)
									{
										SetQuestFlag("bio.count", bio_data[biolevel][2]);
										SetQuestFlag("bio.time", 0);
										SetQuestFlag("bio.bitirici", 1);
										do_bio(this, "bio_dra mission 0 0", 0, 0);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO,"Biyolog gorevinize uygun degil. ");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81806:
								{
									if (quest::CQuestManager::instance().GetEventFlag("biyologbitirici") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									int biolevel = GetQuestFlag("bio.level");
									if (GetLevel() < 85)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog bitiriciye leveliniz uygun degil");
										return false;
									}

									if (biolevel >= bio_max)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog gorevler bitmis");
										return false;
									}

									if (biolevel == 7)
									{
										SetQuestFlag("bio.count", bio_data[biolevel][2]);
										SetQuestFlag("bio.time", 0);
										SetQuestFlag("bio.bitirici", 1);
										do_bio(this, "bio_dra mission 0 0", 0, 0);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO,"Biyolog gorevinize uygun degil. ");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81807:
								{
									if (quest::CQuestManager::instance().GetEventFlag("biyologbitirici") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									int biolevel = GetQuestFlag("bio.level");
									if (GetLevel() < 90)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog bitiriciye leveliniz uygun degil");
										return false;
									}

									if (biolevel >= bio_max)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog gorevler bitmis");
										return false;
									}

									if (biolevel == 8)
									{
										SetQuestFlag("bio.count", bio_data[biolevel][2]);
										SetQuestFlag("bio.time", 0);
										SetQuestFlag("bio.bitirici", 1);
										do_bio(this, "bio_dra mission 0 0", 0, 0);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO,"Biyolog gorevinize uygun degil. ");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81808:
								{
									if (quest::CQuestManager::instance().GetEventFlag("biyologbitirici") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									int biolevel = GetQuestFlag("bio.level");
									if (GetLevel() < 92)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog bitiriciye leveliniz uygun degil");
										return false;
									}

									if (biolevel >= bio_max)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog gorevler bitmis");
										return false;
									}

									if (biolevel == 9)
									{
										SetQuestFlag("bio.count", bio_data[biolevel][2]);
										SetQuestFlag("bio.time", 0);
										SetQuestFlag("bio.bitirici", 1);
										do_bio(this, "bio_dra mission 0 0", 0, 0);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO,"Biyolog gorevinize uygun degil. ");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81809:
								{
									if (quest::CQuestManager::instance().GetEventFlag("biyologbitirici") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									int biolevel = GetQuestFlag("bio.level");
									if (GetLevel() < 94)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog bitiriciye leveliniz uygun degil");
										return false;
									}

									if (biolevel >= bio_max)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog gorevler bitmis");
										return false;
									}

									if (biolevel == 10)
									{
										SetQuestFlag("bio.count", bio_data[biolevel][2]);
										SetQuestFlag("bio.time", 0);
										SetQuestFlag("bio.bitirici", 1);
										do_bio(this, "bio_dra mission 0 0", 0, 0);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO,"Biyolog gorevinize uygun degil. ");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81810:
								{
									if (quest::CQuestManager::instance().GetEventFlag("biyologbitirici") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									int biolevel = GetQuestFlag("bio.level");
									if (GetLevel() < 96)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog bitiriciye leveliniz uygun degil");
										return false;
									}

									if (biolevel >= bio_max)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog gorevler bitmis");
										return false;
									}

									if (biolevel == 11)
									{
										SetQuestFlag("bio.count", bio_data[biolevel][2]);
										SetQuestFlag("bio.time", 0);
										SetQuestFlag("bio.bitirici", 1);
										do_bio(this, "bio_dra mission 0 0", 0, 0);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO,"Biyolog gorevinize uygun degil. ");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 81811:
								{
									if (quest::CQuestManager::instance().GetEventFlag("biyologbitirici") == 1)
									{
										ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
										return false;
									}

									int biolevel = GetQuestFlag("bio.level");
									if (GetLevel() < 99)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog bitiriciye leveliniz uygun degil");
										return false;
									}

									if (biolevel >= bio_max)
									{
										ChatPacket(CHAT_TYPE_INFO, "Biyolog gorevler bitmis");
										return false;
									}

									if (biolevel == 12)
									{
										SetQuestFlag("bio.count", bio_data[biolevel][2]);
										SetQuestFlag("bio.time", 0);
										SetQuestFlag("bio.bitirici", 1);
										do_bio(this, "bio_dra mission 0 0", 0, 0);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO,"Biyolog gorevinize uygun degil. ");
										return false;
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;
							#endif

							case ITEM_AUTO_HP_RECOVERY_S:
							case ITEM_AUTO_HP_RECOVERY_M:
							case ITEM_AUTO_HP_RECOVERY_L:
							case ITEM_AUTO_HP_RECOVERY_X:
							case ITEM_AUTO_SP_RECOVERY_S:
							case ITEM_AUTO_SP_RECOVERY_M:
							case ITEM_AUTO_SP_RECOVERY_L:
							case ITEM_AUTO_SP_RECOVERY_X:

							case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
							case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
							case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
							case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
							case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
							case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
								{
									if (IsPolymorphed()) // Donusum Fix
									{
										ChatPacket(CHAT_TYPE_INFO, "Donusum sirasinda bunu yapamazsin.");
										return false;
									}

									#ifdef ENABLE_ITEM_TIME
									if (FindAffect(AFFECT_ITEM_BLOCK))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("hizliyaptin!"));
										return false;
									}
									#endif

									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}
									#ifdef ENABLE_NEWSTUFF
									else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
										return false;
									}
									#endif

									EAffectTypes type = AFFECT_NONE;
									bool isSpecialPotion = false;

									switch (item->GetVnum())
									{
										case ITEM_AUTO_HP_RECOVERY_X:
											isSpecialPotion = true;

										case ITEM_AUTO_HP_RECOVERY_S:
										case ITEM_AUTO_HP_RECOVERY_M:
										case ITEM_AUTO_HP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
											type = AFFECT_AUTO_HP_RECOVERY;
											break;

										case ITEM_AUTO_SP_RECOVERY_X:
											isSpecialPotion = true;

										case ITEM_AUTO_SP_RECOVERY_S:
										case ITEM_AUTO_SP_RECOVERY_M:
										case ITEM_AUTO_SP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
											type = AFFECT_AUTO_SP_RECOVERY;
											break;
									}

									if (AFFECT_NONE == type)
										break;

									if (item->GetCount() > 1)
									{
										int pos = GetEmptyInventory(item->GetSize());
										if (-1 == pos)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
											break;
										}

										item->SetCount(item->GetCount() - 1);

										LPITEM item2 = ITEM_MANAGER::instance().CreateItem( item->GetVnum(), 1 );
										item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

										if (item->GetSocket(1) != 0)
										{
											item2->SetSocket(1, item->GetSocket(1));
										}

										item = item2;
									}

									CAffect* pAffect = FindAffect(type);
									if (NULL == pAffect)
									{
										EPointTypes bonus = POINT_NONE;

										if (true == isSpecialPotion)
										{
											if (type == AFFECT_AUTO_HP_RECOVERY)
											{
												bonus = POINT_MAX_HP_PCT;
											}
											else if (type == AFFECT_AUTO_SP_RECOVERY)
											{
												bonus = POINT_MAX_SP_PCT;
											}
										}

										AddAffect(type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

										item->Lock(true);
										item->SetSocket(0, true);

										AutoRecoveryItemProcess(type);
									}
									else
									{
										if (item->GetID() == pAffect->dwFlag)
										{
											RemoveAffect(pAffect);

											item->Lock(false);
											item->SetSocket(0, false);
											#ifdef ENABLE_ITEM_TIME
											AddAffect(AFFECT_ITEM_BLOCK, POINT_NONE, 0, 0, 10, 0, true, false);
											#endif
										}
										else
										{
											LPITEM old = FindItemByID(pAffect->dwFlag);
											if (NULL != old)
											{
												old->Lock(false);
												old->SetSocket(0, false);
											}

											RemoveAffect(pAffect);

											#ifdef ENABLE_ITEM_TIME
											AddAffect(AFFECT_ITEM_BLOCK, POINT_NONE, 0, 0, 10, 0, true, false);
											#endif

											EPointTypes bonus = POINT_NONE;

											if (true == isSpecialPotion)
											{
												if (type == AFFECT_AUTO_HP_RECOVERY)
												{
													bonus = POINT_MAX_HP_PCT;
												}
												else if (type == AFFECT_AUTO_SP_RECOVERY)
												{
													bonus = POINT_MAX_SP_PCT;
												}
											}

											AddAffect(type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

											item->Lock(true);
											item->SetSocket(0, true);

											AutoRecoveryItemProcess(type);
										}
									}
								}
								break;
						}
						break;

					case USE_CLEAR:
						{
							if (GetProtectTime("baliksure") > get_global_time())
								return false;

							switch (item->GetVnum())
							{
								#ifdef ENABLE_WOLFMAN_CHARACTER
								case 27124: // Bandage
									RemoveBleeding();
									break;
								#endif
								case 27874: // Grilled Perch
								default:
									RemoveBadAffect();
									break;
							}

							item->SetCount(item->GetCount() - 1);
							SetProtectTime("baliksure", get_global_time() + 1);
						}
						break;

					case USE_INVISIBILITY:
						{
							if (item->GetVnum() == 70026)
							{
								quest::CQuestManager& q = quest::CQuestManager::instance();
								quest::PC* pPC = q.GetPC(GetPlayerID());

								if (pPC != NULL)
								{
									int last_use_time = pPC->GetFlag("mirror_of_disapper.last_use_time");

									if (get_global_time() - last_use_time < 10*60)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									pPC->SetFlag("mirror_of_disapper.last_use_time", get_global_time());
								}
							}

							AddAffect(AFFECT_INVISIBILITY, POINT_NONE, 0, AFF_INVISIBILITY, 300, 0, true);
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_POTION_NODELAY:
						{
							if (GetProtectTime("use_potion_nodelay2") > get_global_time())
								return false;

							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç¿ë Á¦ÇÑ·®À» ÃÊ°úÇÏ¿´½À´Ï´Ù."));
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
										return false;
								}
							}
							#ifdef ENABLE_NEWSTUFF
							else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
								return false;
							}
							#endif

							bool used = false;

							if (item->GetValue(0) != 0)
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0)
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0)
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¿ùº´ ¶Ç´Â Á¾ÀÚ ¸¦ »ç¿ëÇÏ¿´½À´Ï´Ù"));
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								SetProtectTime("use_potion_nodelay2", get_global_time() + 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;

					case USE_POTION:
						if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
						{
							if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							switch (item->GetVnum())
							{
								case 27001 :
								case 27002 :
								case 27003 :
								case 27004 :
								case 27005 :
								case 27006 :
									if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
									{
										if (m_nPotionLimit <= 0)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç¿ë Á¦ÇÑ·®À» ÃÊ°úÇÏ¿´½À´Ï´Ù."));
											return false;
										}
									}
									break;

								default :
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
									return false;
							}
						}
						#ifdef ENABLE_NEWSTUFF
						else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
							return false;
						}
						#endif

						if (item->GetValue(1) != 0)
						{
							if (GetPoint(POINT_SP_RECOVERY) + GetSP() >= GetMaxSP())
							{
								return false;
							}

							PointChange(POINT_SP_RECOVERY, item->GetValue(1) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							StartAffectEvent();
							EffectPacket(SE_SPUP_BLUE);
						}

						if (item->GetValue(0) != 0)
						{
							if (GetPoint(POINT_HP_RECOVERY) + GetHP() >= GetMaxHP())
							{
								return false;
							}

							PointChange(POINT_HP_RECOVERY, item->GetValue(0) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							StartAffectEvent();
							EffectPacket(SE_HPUP_RED);
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						m_nPotionLimit--;
						break;

					case USE_POTION_CONTINUE:
						{
							if (item->GetValue(0) != 0)
							{
								AddAffect(AFFECT_HP_RECOVER_CONTINUE, POINT_HP_RECOVER_CONTINUE, item->GetValue(0), 0, item->GetValue(2), 0, true);
							}
							else if (item->GetValue(1) != 0)
							{
								AddAffect(AFFECT_SP_RECOVER_CONTINUE, POINT_SP_RECOVER_CONTINUE, item->GetValue(1), 0, item->GetValue(2), 0, true);
							}
							else
								return false;
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					case USE_ABILITY_UP:
						{
							if (item->GetValue(4) == 3838)
							{
								BYTE bPointType = POINT_NONE;

								switch (item->GetValue(0))
								{
									case APPLY_MOV_SPEED:
										bPointType = POINT_MOV_SPEED;
										break;

									case APPLY_ATT_SPEED:
										bPointType = POINT_ATT_SPEED;
										break;

									case APPLY_STR:
										bPointType = POINT_ST;
										break;

									case APPLY_DEX:
										bPointType = POINT_DX;
										break;

									case APPLY_CON:
										bPointType = POINT_HT;
										break;

									case APPLY_INT:
										bPointType = POINT_IQ;
										break;

									case APPLY_CAST_SPEED:
										bPointType = POINT_CASTING_SPEED;
										break;

									case APPLY_ATT_GRADE_BONUS:
										bPointType = POINT_ATT_GRADE_BONUS;
										break;

									case APPLY_DEF_GRADE_BONUS:
										bPointType = POINT_DEF_GRADE_BONUS;
										break;

									default:
										return false;
								}

								CAffect* pAffect = FindAffect(AFFECT_FISH_POTION, bPointType);
								if (pAffect)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
									return false;
								}

								AddAffect(AFFECT_FISH_POTION, bPointType, item->GetValue(2), AFF_NONE, item->GetValue(1), 0, false);
								item->SetCount(item->GetCount() - 1);
								return true;
							}

							if (item->GetValue(4) == 5858)
							{
								BYTE bPointType = POINT_NONE;

								switch (item->GetValue(0))
								{
									case APPLY_ATTBONUS_MONSTER:
										bPointType = POINT_ATTBONUS_MONSTER;
										break;

									case APPLY_ATTBONUS_STONE:
										bPointType = POINT_ATTBONUS_STONE;
										break;

									case APPLY_ATTBONUS_BOSS:
										bPointType = POINT_ATTBONUS_BOSS;
										break;

									case APPLY_HIT_DAMAGE_BONUS_PVM:
										bPointType = POINT_HIT_DAMAGE_BONUS_PVM;
										break;

									case APPLY_ATTBONUS_HUMAN:
										bPointType = POINT_ATTBONUS_HUMAN;
										break;

									case APPLY_RESIST_HUMAN:
										bPointType = POINT_RESIST_HUMAN;
										break;

									default:
										return false;
								}

								CAffect* pAffect = FindAffect(AFFECT_FISH_NEW_POTION, bPointType);
								if (pAffect)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
									return false;
								}

								AddAffect(AFFECT_FISH_NEW_POTION, bPointType, item->GetValue(2), AFF_NONE, item->GetValue(1), 0, false);
								item->SetCount(item->GetCount() - 1);
								return true;
							}

							#ifdef ENABLE_PERMA_SET
							if ((item->GetValue(4) == 4248) || (item->GetValue(4) == 4250))
							{
								#ifdef ENABLE_ITEM_TIME
								if (FindAffect(AFFECT_ITEM_BLOCK))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("hizliyaptin!"));
									return false;
								}
								#endif

								bool bStatus = item->GetSocket(0);

								CAffect* pAffect = FindAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType);
								if (pAffect)
								{
									if (!bStatus)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}

									item->Lock(false);
									item->SetSocket(0, false);

									RemoveAffect(pAffect);
									#ifdef ENABLE_ITEM_TIME
									AddAffect(AFFECT_ITEM_BLOCK, POINT_NONE, 0, 0, 10, 0, true, false);
									#endif
									return false;
								}

								item->Lock(true);
								item->SetSocket(0, true);
								AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), item->GetValue(3), INFINITE_AFFECT_DURATION, 0, true);

								if (item->GetValue(4) == 4250)
								{
									item->SetCount(item->GetCount() - 1);
								}
								break;

								/************************************************************************
								Bilgilendirme
								AddAffect(DWORD dwType, BYTE bApplyOn, long lApplyValue, DWORD dwFlag, long lDuration, long lSPCost, bool bOverride, bool IsCube = false);
								dwType = affectkodu
								bApplyOn = affectin vericeði özellik
								lApplyValue = affectin vericeði puan
								dwFlag = affecte atanan seçili olma
								bOverride = affecti tekrar sildiri verdirir yani removeaffect kullanmýyorsanýz affecte bOverride bunu truelayýp affecti sildirip tekrar verdirirsiniz. & clientede sildindiði bilgisi gönderir.
								IsCube = bu deðer otomatik false olarak döner bu kodun amacý affectin içinde affect kodu ve verilen bonusun özelliðin karakter üzerinde varmý yok mu aratmasý yani FindAffect(type) bu tarz kullanýyorsanýz bunu true yapabilirsiniz
								eðer kod baþlangýcýnda FindAffect(type,özellik) þeklinde ise tekrar sorgulatmanýza gerek yok
								*************************************************************************/
							}
							#endif

							switch (item->GetValue(0))
							{
								case APPLY_MOV_SPEED:
									if (FindAffect(AFFECT_MOV_SPEED))// @fixme171
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									AddAffect(AFFECT_MOV_SPEED, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true);
									#ifdef ENABLE_EFFECT_EXTRAPOT
									EffectPacket(SE_DXUP_PURPLE);
									#endif
									break;

								case APPLY_ATT_SPEED:
									if (FindAffect(AFFECT_ATT_SPEED))// @fixme171
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									AddAffect(AFFECT_ATT_SPEED, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true);
									#ifdef ENABLE_EFFECT_EXTRAPOT
									EffectPacket(SE_SPEEDUP_GREEN);
									#endif
									break;

								case APPLY_STR:
									if (FindAffect(AFFECT_STR/*, POINT_ST*/))// @fixme171
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									AddAffect(AFFECT_STR, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_DEX:
									if (FindAffect(AFFECT_DEX/*, POINT_DX*/))// @fixme171
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									AddAffect(AFFECT_DEX, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_CON:
									if (FindAffect(AFFECT_CON/*, POINT_HT*/))// @fixme171
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									AddAffect(AFFECT_CON, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_INT:
									if (FindAffect(AFFECT_INT/*, POINT_IQ*/))// @fixme171
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									AddAffect(AFFECT_INT, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_CAST_SPEED:
									if (FindAffect(AFFECT_CAST_SPEED/*, POINT_CASTING_SPEED*/))// @fixme171
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									AddAffect(AFFECT_CAST_SPEED, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_ATT_GRADE_BONUS:
									if (FindAffect(AFFECT_ATT_GRADE/*, POINT_ATT_GRADE_BONUS*/))// @fixme171
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									AddAffect(AFFECT_ATT_GRADE, POINT_ATT_GRADE_BONUS, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_DEF_GRADE_BONUS:
									if (FindAffect(AFFECT_DEF_GRADE/*, POINT_DEF_GRADE_BONUS*/))// @fixme171
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									AddAffect(AFFECT_DEF_GRADE, POINT_DEF_GRADE_BONUS, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					case USE_TALISMAN:
						{
							const int TOWN_PORTAL	= 1;
							const int MEMORY_PORTAL = 2;

							if (GetMapIndex() == 200 || GetMapIndex() == 113)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÇöÀç À§Ä¡¿¡¼­ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
								return false;
							}
							#ifdef ENABLE_NEWSTUFF
							else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
								return false;
							}
							#endif

							if (m_pkWarpEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌµ¿ÇÒ ÁØºñ°¡ µÇ¾îÀÖÀ½À¸·Î ±ÍÈ¯ºÎ¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù"));
								return false;
							}

							// CONSUME_LIFE_WHEN_USE_WARP_ITEM
							int consumeLife = CalculateConsume(this);

							if (consumeLife < 0)
								return false;
							// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

							if (item->GetValue(0) == TOWN_PORTAL)
							{
								if (item->GetSocket(0) == 0)
								{
									if (!GetDungeon())
										if (!GiveRecallItem(item))
											return false;

									PIXEL_POSITION posWarp;

									if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp))
									{
										// CONSUME_LIFE_WHEN_USE_WARP_ITEM
										PointChange(POINT_HP, -consumeLife, false);
										// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

										WarpSet(posWarp.x, posWarp.y);
									}
									else
									{
										sys_err("CHARACTER::UseItem : cannot find spawn position (name %s, %d x %d)", GetName(), GetX(), GetY());
									}
								}
								else
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¿ø·¡ À§Ä¡·Î º¹±Í"));

									ProcessRecallItem(item);
								}
							}
							else if (item->GetValue(0) == MEMORY_PORTAL)
							{
								if (item->GetSocket(0) == 0)
								{
									if (GetDungeon())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´øÀü ¾È¿¡¼­´Â %s%s »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."),
												item->GetName(),
												"");
										return false;
									}

									if (!GiveRecallItem(item))
										return false;
								}
								else
								{
									// CONSUME_LIFE_WHEN_USE_WARP_ITEM
									PointChange(POINT_HP, -consumeLife, false);
									// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

									ProcessRecallItem(item);
								}
							}
						}
						break;

					case USE_TUNING:
					case USE_DETACHMENT:
						{
							LPITEM item2;

							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
								return false;

							if (item2->GetVnum() >= 28330 && item2->GetVnum() <= 28343)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("+3 ¿µ¼®Àº ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù"));
								return false;
							}

							#ifdef ENABLE_RITUALS_SCROLL
							if (item->GetValue(0) == RITUALS_SCROLL)
							{
								if (item2->GetLevelLimit() < item->GetValue(1))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("you can only upgrade items above level %d"), item->GetValue(1));
									return false;
								}
								else
									RefineItem(item, item2);
							}
							#endif

							#ifdef ENABLE_ACCE_COSTUME_SYSTEM
							if (item->GetValue(0) == ACCE_CLEAN_ATTR_VALUE0
								|| item->GetVnum() == ACCE_REVERSAL_VNUM_1
								|| item->GetVnum() == ACCE_REVERSAL_VNUM_2
							)
							{
								if (!CleanAcceAttr(item, item2))
									return false;
								item->SetCount(item->GetCount()-1);
								return true;
							}
							#endif

							#ifdef ENABLE_CHANGELOOK_SYSTEM
							if ((item->GetValue(0) == CL_CLEAN_ATTR_VALUE0) && item2->GetTransmutation())
							{
								TItemTable * table = ITEM_MANAGER::instance().GetTable(item2->GetVnum());
								if (table)
								{
									for (int i = 0; i < ITEM_LIMIT_MAX_NUM ; i++)
									{
										if (LIMIT_REAL_TIME == table->aLimits->bType)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("cannot use detachment time item."));
											return false;
										}
									}
								}
							}
							#endif

							#ifdef ENABLE_CHANGELOOK_SYSTEM
							if (item->GetValue(0) == CL_CLEAN_ATTR_VALUE0)
							{
								if (!CleanTransmutation(item, item2))
									return false;

								return true;
							}
							#endif

							if (item2->GetVnum() >= 28430 && item2->GetVnum() <= 28443)
							{
								if (item->GetVnum() == 71056)
								{
									RefineItem(item, item2);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¿µ¼®Àº ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù"));
								}
							}
							else
							{
								RefineItem(item, item2);
							}
						}
						break;

					#ifdef ENABLE_KOSTUM_SAC_BINEK_PET_EFSUNLAMA
					case USE_CHANGE_COSTUME_ATTR:
					case USE_RESET_COSTUME_ATTR:
					case USE_ATTRIBUTE_MOUNT:
					case USE_ATTRIBUTE_PET:
					#ifdef ENABLE_COSTUME_WEAPON_ATTR
					case USE_ATTRIBUTE_COSTUME_WEAPON:
					#endif
						{
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}

							if (!(item2->GetType() == ITEM_COSTUME) && !(item2->GetSubType() == COSTUME_PET))
							{
								if (ITEM_COSTUME != item2->GetType())
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
									return false;
								}
							}

							if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
								return false;

							if (item2->GetVnum() >= 74001 && item->GetVnum() <= 75632)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯°æÇÒ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù."));
								return false;
							}

							if (item2->GetVnum() == 50201
								|| item2->GetVnum() == 50202
								|| item2->GetVnum() == 11901
								|| item2->GetVnum() == 11902
								|| item2->GetVnum() == 11903
								|| item2->GetVnum() == 11904
								|| item2->GetVnum() == 11911
								|| item2->GetVnum() == 11912
								|| item2->GetVnum() == 11913
								|| item2->GetVnum() == 11914
								|| item2->GetVnum() == 71128
								|| item2->GetVnum() == 53020
								|| item2->GetVnum() == 71224
								|| item2->GetVnum() == 71225
								|| item2->GetVnum() == 71224
								|| item2->GetVnum() == 52202
								|| item2->GetVnum() == 71828
								|| item2->GetVnum() == 29700
								|| item2->GetVnum() == 29701
								|| item2->GetVnum() == 29702
								|| item2->GetVnum() == 29703
								|| item2->GetVnum() == 40101
								|| item2->GetVnum() == 40102
								|| item2->GetVnum() == 40103
								|| item2->GetVnum() == 40104
								|| item2->GetVnum() == 40105
								|| item2->GetVnum() == 40106
								|| item2->GetVnum() == 40107
								|| item2->GetVnum() == 40121
								|| item2->GetVnum() == 40122
								|| item2->GetVnum() == 40123
								|| item2->GetVnum() == 40124
								|| item2->GetVnum() == 40125
								|| item2->GetVnum() == 40126
								|| item2->GetVnum() == 40127
								|| item2->GetVnum() == 40128
							)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯°æÇÒ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù."));
								return false;
							}

							switch (item->GetSubType())
							{
								case USE_CHANGE_COSTUME_ATTR: // sac
									if ((item2->GetType() == ITEM_COSTUME) && (item2->GetSubType() == COSTUME_HAIR))
									{
										if (item2->GetAttributeCount() < 3)
										{
											if (item2->HasAttr(item->GetValue(0)))
											{
												ChatPacket(CHAT_TYPE_INFO, "Ayni bonusu iki kez ekleyemezsiniz.");
												return false;
											}
											item2->AddAttribute(item->GetValue(0), item->GetValue(1));
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "Bonus eklenemedi. Kostumunuze sadece uc bonus ekleyebilirsiniz!");
											return false;
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bonus sadece sac icin gecerlidir!");
										return false;
									}
									break;

								case USE_RESET_COSTUME_ATTR: // kostum
									if ((item2->GetType() == ITEM_COSTUME) && (item2->GetSubType() == COSTUME_BODY))
									{

										if (item2->GetAttributeCount() < 3)
										{
											if (item2->HasAttr(item->GetValue(0)))
											{
												ChatPacket(CHAT_TYPE_INFO, "Ayni bonusu iki kez ekleyemezsiniz.");
												return false;
											}
											item2->AddAttribute(item->GetValue(0), item->GetValue(1));
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "Bonus eklenemedi. Kostumunuze sadece uc bonus ekleyebilirsiniz!");
											return false;
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bonus sadece kostum icin gecerlidir!");
										return false;
									}
									break;

								case USE_ATTRIBUTE_MOUNT: //binek
									if ((item2->GetType() == ITEM_COSTUME) && (item2->GetSubType() == COSTUME_MOUNT))
									{
										if (item2->GetAttributeCount() < 3)
										{
											if (item2->HasAttr(item->GetValue(0)))
											{
												ChatPacket(CHAT_TYPE_INFO, "Ayni bonusu iki kez ekleyemezsiniz.");
												return false;
											}
											item2->AddAttribute(item->GetValue(0), item->GetValue(1));
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "Bonus eklenemedi. Kostumunuze sadece uc bonus ekleyebilirsiniz!");
											return false;
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bonus sadece binek icin gecerlidir!");
										return false;
									}
									break;

								case USE_ATTRIBUTE_PET: //pet
									if ((item2->GetType() == ITEM_COSTUME) && (item2->GetSubType() == COSTUME_PET))
									{
										if (item2->GetAttributeCount() < 3)
										{
											if (item2->HasAttr(item->GetValue(0)))
											{
												ChatPacket(CHAT_TYPE_INFO, "Ayni bonusu iki kez ekleyemezsiniz.");
												return false;
											}
											item2->AddAttribute(item->GetValue(0), item->GetValue(1));
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "Bonus eklenemedi. Kostumunuze sadece uc bonus ekleyebilirsiniz!");
											return false;
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bonus sadece pet icin gecerlidir!");
										return false;
									}
									break;

								#ifdef ENABLE_COSTUME_WEAPON_ATTR
								case USE_ATTRIBUTE_COSTUME_WEAPON: // weapon
									if ((item2->GetType() == ITEM_COSTUME) && (item2->GetSubType() == COSTUME_WEAPON))
									{
										if (item2->GetAttributeCount() < 3)
										{
											if (item2->HasAttr(item->GetValue(0)))
											{
												ChatPacket(CHAT_TYPE_INFO, "Ayni bonusu iki kez ekleyemezsiniz.");
												return false;
											}
											item2->AddAttribute(item->GetValue(0), item->GetValue(1));
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "Bonus eklenemedi. Silah fisinize sadece uc bonus ekleyebilirsiniz!");
											return false;
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "Bonus sadece silah fisi icin gecerlidir!");
										return false;
									}
									break;
								#endif
							}

							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÏ¿´½À´Ï´Ù."));

							item->SetCount(item->GetCount() - 1);
							break;
						}
					#else
					case USE_CHANGE_COSTUME_ATTR:
					case USE_RESET_COSTUME_ATTR:
						{
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}

							if (ITEM_COSTUME != item2->GetType())
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
								return false;
							}

							if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
								return false;

							if (item2->GetAttributeSetIndex() == -1)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
								return false;
							}

							if (item2->GetAttributeCount() == 0)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯°æÇÒ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù."));
								return false;
							}

							switch (item->GetSubType())
							{
								case USE_CHANGE_COSTUME_ATTR:
									item2->ChangeAttribute();
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_COSTUME_ATTR", buf);
									}
									break;
								case USE_RESET_COSTUME_ATTR:
									item2->ClearAttribute();
									item2->AlterToMagicItem();
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "RESET_COSTUME_ATTR", buf);
									}
									break;
							}

							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÏ¿´½À´Ï´Ù."));

							item->SetCount(item->GetCount() - 1);
							break;
						}
					#endif

					// ACCESSORY_REFINE & ADD/CHANGE_ATTRIBUTES
					case USE_PUT_INTO_BELT_SOCKET:
					case USE_PUT_INTO_RING_SOCKET:
					case USE_PUT_INTO_ACCESSORY_SOCKET:
					case USE_ADD_ACCESSORY_SOCKET:
					case USE_CLEAN_SOCKET:
					case USE_CHANGE_ATTRIBUTE:
					case USE_CHANGE_ATTRIBUTE2 :
					case USE_ADD_ATTRIBUTE:
					case USE_ADD_ATTRIBUTE2:
						{
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}

							if (ITEM_COSTUME == item2->GetType())
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
								return false;
							}

							if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
								return false;

							switch (item->GetSubType())
							{
								case USE_CLEAN_SOCKET:
									{
										int i;
										for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										{
											if (item2->GetSocket(i) == ITEM_BROKEN_METIN_VNUM)
												break;
										}

										if (i == ITEM_SOCKET_MAX_NUM)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ã»¼ÒÇÒ ¼®ÀÌ ¹ÚÇôÀÖÁö ¾Ê½À´Ï´Ù."));
											return false;
										}

										int j = 0;

										for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										{
											if (item2->GetSocket(i) != ITEM_BROKEN_METIN_VNUM && item2->GetSocket(i) != 0)
												item2->SetSocket(j++, item2->GetSocket(i));
										}

										for (; j < ITEM_SOCKET_MAX_NUM; ++j)
										{
											if (item2->GetSocket(j) > 0)
												item2->SetSocket(j, 1);
										}

										{
											char buf[21];
											snprintf(buf, sizeof(buf), "%u", item2->GetID());
											LogManager::instance().ItemLog(this, item, "CLEAN_SOCKET", buf);
										}

										item->SetCount(item->GetCount() - 1);

									}
									break;

								case USE_CHANGE_ATTRIBUTE :
								case USE_CHANGE_ATTRIBUTE2 : // @fixme123
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										return false;
									}

									if (item2->GetAttributeCount() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯°æÇÒ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù."));
										return false;
									}

									if ((GM_PLAYER == GetGMLevel()) && (false == test_server) && (g_dwItemBonusChangeTime > 0))
									{
										DWORD dwChangeItemAttrCycle = g_dwItemBonusChangeTime;

										quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID());

										if (pPC)
										{
											DWORD dwNowSec = get_global_time();

											DWORD dwLastChangeItemAttrSec = pPC->GetFlag(msc_szLastChangeItemAttrFlag);

											if (dwLastChangeItemAttrSec + dwChangeItemAttrCycle > dwNowSec)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» ¹Ù²ÛÁö %dºÐ ÀÌ³»¿¡´Â ´Ù½Ã º¯°æÇÒ ¼ö ¾ø½À´Ï´Ù.(%d ºÐ ³²À½)"),
														dwChangeItemAttrCycle, dwChangeItemAttrCycle - (dwNowSec - dwLastChangeItemAttrSec));
												return false;
											}

											pPC->SetFlag(msc_szLastChangeItemAttrFlag, dwNowSec);
										}
									}

									if (item->GetSubType() == USE_CHANGE_ATTRIBUTE2)
									{
										int aiChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
										{
											0, 0, 30, 40, 3
										};

										item2->ChangeAttribute(aiChangeProb);
									}
									else if (item->GetVnum() == 76014)
									{
										int aiChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
										{
											0, 10, 50, 39, 1
										};

										item2->ChangeAttribute(aiChangeProb);
									}
									#ifdef ENABLE_BECERI_ORTALAMA_EFSUN
									else if (item->GetVnum() == 11122)
									{
										if (quest::CQuestManager::instance().GetEventFlag("ortalamaefsun") == 1)
										{
											ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
											return false;
										}

										BYTE bS = APPLY_SKILL_DAMAGE_BONUS;
										BYTE bH = APPLY_NORMAL_HIT_DAMAGE_BONUS;

										if (item2->GetAttributeType(0) != bS && item2->GetAttributeType(0) != bH)
										{
											ChatPacket(CHAT_TYPE_INFO, "Bu nesneyi sadece ortalama hasara sahip olabilecek esyalarda kullanabilirsin !");
											return false;
										}

										if (item2->GetAttributeType(0) != bH && item2->GetAttributeType(1) != bS)
										{
											ChatPacket(CHAT_TYPE_INFO, "Esyanizda ortalama zarar ve beceri hasari efsunu yok bu sekilde bu esyayi kullanmak efsun kaybina sebep olur !");
											return false;
										}
										else
										{
											short attrvalue2 = number(15, 28); // - beceri hasari ayarligi min-max
											short attrvalue1 = 0;

											int sayi1 = number(1, 60);
											if (sayi1 == 60)
												attrvalue1 = 60;
											else if (sayi1 == 49)
												attrvalue1 = 59;
											else if (sayi1 == 48)
												attrvalue1 = 58;
											else if (sayi1 == 47)
												attrvalue1 = 56;
											else if (sayi1 == 46)
												attrvalue1 = 55;
											else
												attrvalue1 = number(45, 54);

											short becerihasari = attrvalue2;
											becerihasari -= attrvalue2;
											becerihasari -= attrvalue2;

											// item2->ChangeAttribute();

											item2->SetForceAttribute(0, bH, attrvalue1);
											item2->SetForceAttribute(1, bS, becerihasari);
										}
									}
									else if (item->GetVnum() == 11123)
									{
										if (quest::CQuestManager::instance().GetEventFlag("beceriefsun") == 1)
										{
											ChatPacket(CHAT_TYPE_INFO, "Bu Sistem Bakimdadir !");
											return false;
										}

										BYTE bS = APPLY_SKILL_DAMAGE_BONUS;
										BYTE bH = APPLY_NORMAL_HIT_DAMAGE_BONUS;

										if (item2->GetAttributeType(0) != bS && item2->GetAttributeType(0) != bH)
										{
											ChatPacket(CHAT_TYPE_INFO, "Bu nesneyi sadece beceri hasara sahip olabilecek esyalarda kullanabilirsin !");
											return false;
										}

										if (item2->GetAttributeType(0) != bH && item2->GetAttributeType(1) != bS)
										{
											ChatPacket(CHAT_TYPE_INFO, "Esyanizda ortalama zarar ve beceri hasari efsunu yok bu sekilde bu esyayi kullanmak efsun kaybina sebep olur !");
											return false;
										}
										else
										{
											short attrvalue1 = number(20, 27); // esyaya gelecebilecek  - ortalama hasar araligi min-max
											short attrvalue2 = 0;

											int sayi1 = number(1, 60);
											if (sayi1 == 60)
												attrvalue2 = 30;
											else if (sayi1 == 49)
												attrvalue2 = 28;
											else if (sayi1 == 48)
												attrvalue2 = 27;
											else if (sayi1 == 47)
												attrvalue2 = 26;
											else if (sayi1 == 46)
												attrvalue2 = 25;
											else
												attrvalue2 = number(18, 24);

											short ortalamazarar = attrvalue1;
											ortalamazarar -= attrvalue1;
											ortalamazarar -= attrvalue1;

											// item2->ChangeAttribute();

											item2->SetForceAttribute(0, bH, ortalamazarar);
											item2->SetForceAttribute(1, bS, attrvalue2);
										}
									}
									#endif
									#ifdef ENABLE_KADIM_EFSUN
									else if (item->GetVnum() == 11125)
									{
										// if (item2->GetType() == ITEM_WEAPON)
										// {
											// bool bCanUse = false;
											// for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
											// {
												// if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) == 75)
												// {
													// bCanUse = true;
													// break;
												// }
											// }
											// if (false == bCanUse)
											// {
												// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
												// break;
											// }
										// }
										// else
										// {
											// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
											// break;
										// }

										// if (item2->GetAttributeSetIndex() == -1)
										// {
											// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
											// return false;
										// }

										if (item2->GetAttributeSetIndex() == -1)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
											return false;
										}

										if (item2->GetAttributeCount() != 5)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯°æÇÒ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù."));
											return false;
										}

										efsunudegis(2, item2->GetID());
									}
									else if (item->GetVnum() == 11126)
									{
										if (item2->GetAttributeSetIndex() == -1)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
											return false;
										}

										if (item2->GetAttributeCount() != 5)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯°æÇÒ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù."));
											return false;
										}

										efsunudegis(3, item2->GetID());
									}
									else if (item->GetVnum() == 11127)
									{
										if (item2->GetAttributeSetIndex() == -1)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
											return false;
										}

										if (item2->GetAttributeCount() != 5)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯°æÇÒ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù."));
											return false;
										}

										item2->ChangeAttribute5TH();
									}
									#endif
									else
									{
										if (item->GetVnum() == 71151 || item->GetVnum() == 76023)
										{
											if ((item2->GetType() == ITEM_WEAPON)
												|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY))
											{
												bool bCanUse = true;
												for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
													{
														bCanUse = false;
														break;
													}
												}
												if (false == bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àû¿ë ·¹º§º¸´Ù ³ô¾Æ »ç¿ëÀÌ ºÒ°¡´ÉÇÕ´Ï´Ù."));
													break;
												}
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹«±â¿Í °©¿Ê¿¡¸¸ »ç¿ë °¡´ÉÇÕ´Ï´Ù."));
												break;
											}
										}
										item2->ChangeAttribute();
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÏ¿´½À´Ï´Ù."));
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_ATTRIBUTE", buf);
									}

									item->SetCount(item->GetCount() - 1);
									break;

								case USE_ADD_ATTRIBUTE :
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										return false;
									}

									if (item2->GetAttributeCount() < 4)
									{
										if (item->GetVnum() == 71152 || item->GetVnum() == 76024)
										{
											if ((item2->GetType() == ITEM_WEAPON)
												|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY))
											{
												bool bCanUse = true;
												for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
													{
														bCanUse = false;
														break;
													}
												}
												if (false == bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àû¿ë ·¹º§º¸´Ù ³ô¾Æ »ç¿ëÀÌ ºÒ°¡´ÉÇÕ´Ï´Ù."));
													break;
												}
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹«±â¿Í °©¿Ê¿¡¸¸ »ç¿ë °¡´ÉÇÕ´Ï´Ù."));
												break;
											}
										}
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
										{
											item2->AddAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));

											int iAddedIdx = item2->GetAttributeCount() - 1;
											LogManager::instance().ItemLog(
													GetPlayerID(),
													item2->GetAttributeType(iAddedIdx),
													item2->GetAttributeValue(iAddedIdx),
													item->GetID(),
													"ADD_ATTRIBUTE_SUCCESS",
													buf,
													GetDesc()->GetHostName(),
													item->GetOriginalVnum());
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼º Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
											LogManager::instance().ItemLog(this, item, "ADD_ATTRIBUTE_FAIL", buf);
										}

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õÀÌ»ó ÀÌ ¾ÆÀÌÅÛÀ» ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
									}
									break;

								case USE_ADD_ATTRIBUTE2 :
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										return false;
									}

									if (item2->GetAttributeCount() == 4)
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
										{
											item2->AddAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));

											int iAddedIdx = item2->GetAttributeCount() - 1;
											LogManager::instance().ItemLog(
													GetPlayerID(),
													item2->GetAttributeType(iAddedIdx),
													item2->GetAttributeValue(iAddedIdx),
													item->GetID(),
													"ADD_ATTRIBUTE2_SUCCESS",
													buf,
													GetDesc()->GetHostName(),
													item->GetOriginalVnum());
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼º Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
											LogManager::instance().ItemLog(this, item, "ADD_ATTRIBUTE2_FAIL", buf);
										}

										item->SetCount(item->GetCount() - 1);
									}
									else if (item2->GetAttributeCount() == 5)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ÀÌ ¾ÆÀÌÅÛÀ» ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
									}
									else if (item2->GetAttributeCount() < 4)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸ÕÀú Àç°¡ºñ¼­¸¦ ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡½ÃÄÑ ÁÖ¼¼¿ä."));
									}
									else
									{
										// wtf ?!
										sys_err("ADD_ATTRIBUTE2 : Item has wrong AttributeCount(%d)", item2->GetAttributeCount());
									}
									break;

								case USE_ADD_ACCESSORY_SOCKET:
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->IsAccessoryForSocket())
										{
											if (item2->GetAccessorySocketMaxGrade() < 3)// ENABLE_EXTRA_SOCKET_SYSTEM
											{
												#ifdef ENABLE_ADDSTONE_FAILURE
												if (number(1, 100) <= 50)
												#else
												if (1)
												#endif
												{
													item2->SetAccessorySocketMaxGrade(item2->GetAccessorySocketMaxGrade() + 1);
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÄÏÀÌ ¼º°øÀûÀ¸·Î Ãß°¡µÇ¾ú½À´Ï´Ù."));
													LogManager::instance().ItemLog(this, item, "ADD_SOCKET_SUCCESS", buf);
												}
												else
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÄÏ Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
													LogManager::instance().ItemLog(this, item, "ADD_SOCKET_FAIL", buf);
												}

												item->SetCount(item->GetCount() - 1);
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó ¼ÒÄÏÀ» Ãß°¡ÇÒ °ø°£ÀÌ ¾ø½À´Ï´Ù."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀ¸·Î ¼ÒÄÏÀ» Ãß°¡ÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										}
									}
									break;

								case USE_PUT_INTO_BELT_SOCKET:
								case USE_PUT_INTO_ACCESSORY_SOCKET:
									{
										#ifdef __PERMA_ACCESSORY__
										if (item->GetValue(0) == 31)
										{
											if (!item2->IsAccessoryForSocket())
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu esyadan cevher sokemezsiniz!"));
												return false;
											}

											if (item2->GetSocket(0) <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Sokulecek cevher yok!"));
												return false;
											}

											item2->SetSocket(0, item2->GetSocket(0) - 1);
											item->SetCount(item->GetCount() - 1);
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cevher basariyla sokuldu."));
											return true;
										}
										#endif

										if (item2->IsAccessoryForSocket() && (item->CanPutInto(item2)
											#ifdef __PERMA_ACCESSORY__
											 || item->CanPutIntoPerma(item2)
											#endif
											)
										)
										{
											char buf[21];
											snprintf(buf, sizeof(buf), "%u", item2->GetID());

											if (item2->GetAccessorySocketGrade() < item2->GetAccessorySocketMaxGrade())
											{
												#ifdef __PERMA_ACCESSORY__
												if ((item->GetValue(0) == 99) && (item2->GetSocket(0) > 0) && (item2->GetSocket(3) != 99))
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Once sureli cevherleri sokmelisiniz!"));
													return false;
												}

												if ((item->GetValue(0) != 99) && (item2->GetSocket(0) > 0) && (item2->GetSocket(3) == 99))
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma cevher ekli esyaya sureli cevher ekleyemezsiniz!"));
													return false;
												}
												#endif

												#ifdef __PERMA_ACCESSORY__
												if (item->GetValue(0) == 99)
												{
													item2->SetSocket(3, 99);
													item2->SetAccessorySocketGrade(item2->GetAccessorySocketGrade() + 1);
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀåÂø¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));
													LogManager::instance().ItemLog(this, item, "PUT_SOCKET_SUCCESS", buf);
													item->SetCount(item->GetCount() - 1);
													return true;
												}
												#endif

												if (number(1, 100) <= aiAccessorySocketPutPct[item2->GetAccessorySocketGrade()])
												{
													#ifdef __PERMA_ACCESSORY__
													if (item->GetValue(0) == 99) { item2->SetSocket(3, 99); } else { item2->SetSocket(3, 0); }
													#endif
													item2->SetAccessorySocketGrade(item2->GetAccessorySocketGrade() + 1);
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀåÂø¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));
													LogManager::instance().ItemLog(this, item, "PUT_SOCKET_SUCCESS", buf);
												}
												else
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀåÂø¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
													LogManager::instance().ItemLog(this, item, "PUT_SOCKET_FAIL", buf);
												}

												item->SetCount(item->GetCount() - 1);
											}
											else
											{
												if (item2->GetAccessorySocketMaxGrade() == 0)
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸ÕÀú ´ÙÀÌ¾Æ¸óµå·Î ¾Ç¼¼¼­¸®¿¡ ¼ÒÄÏÀ» Ãß°¡ÇØ¾ßÇÕ´Ï´Ù."));
												else if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó ÀåÂøÇÒ ¼ÒÄÏÀÌ ¾ø½À´Ï´Ù."));
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ÙÀÌ¾Æ¸óµå·Î ¼ÒÄÏÀ» Ãß°¡ÇØ¾ßÇÕ´Ï´Ù."));
												}
												else
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó º¸¼®À» ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀ» ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
										}
									}
									break;
							}

							if (item2->IsEquipped())
							{
								BuffOnAttr_AddBuffsFromItem(item2);
							}
						}
						break;
						// END_OF_ACCESSORY_REFINE & END_OF_ADD_ATTRIBUTES & END_OF_CHANGE_ATTRIBUTES

					case USE_BAIT:
						{

							if (m_pkFishingEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("³¬½Ã Áß¿¡ ¹Ì³¢¸¦ °¥¾Æ³¢¿ï ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							LPITEM weapon = GetWear(WEAR_WEAPON);

							if (!weapon || weapon->GetType() != ITEM_ROD)
								return false;

							if (weapon->GetSocket(2))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì ²ÈÇôÀÖ´ø ¹Ì³¢¸¦ »©°í %s¸¦ ³¢¿ó´Ï´Ù."), item->GetName());
							}
							else
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("³¬½Ã´ë¿¡ %s¸¦ ¹Ì³¢·Î ³¢¿ó´Ï´Ù."), item->GetName());
							}

							weapon->SetSocket(2, item->GetValue(0));
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_MOVE:
					case USE_TREASURE_BOX:
					case USE_MONEYBAG:
						break;

					case USE_AFFECT:
						{
							#ifdef ENABLE_PERMA_SET
							if ((item->GetValue(4) == 4248) || (item->GetValue(4) == 4250))
							{
								#ifdef ENABLE_ITEM_TIME
								if (FindAffect(AFFECT_ITEM_BLOCK))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("hizliyaptin!"));
									return false;
								}
								#endif

								bool bStatus = item->GetSocket(0);

								CAffect* pAffect = FindAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType);
								if (pAffect)
								{
									if (!bStatus)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}

									item->Lock(false);
									item->SetSocket(0, false);

									RemoveAffect(pAffect);
									#ifdef ENABLE_ITEM_TIME
									AddAffect(AFFECT_ITEM_BLOCK, POINT_NONE, 0, 0, 10, 0, true, false);
									#endif
									return false;
								}

								item->Lock(true);
								item->SetSocket(0, true);
								AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), item->GetValue(3), INFINITE_AFFECT_DURATION, 0, false);
								break;

								/************************************************************************
								Bilgilendirme
								AddAffect(DWORD dwType, BYTE bApplyOn, long lApplyValue, DWORD dwFlag, long lDuration, long lSPCost, bool bOverride, bool IsCube = false);

								dwType = affectkodu
								bApplyOn = affectin vericeði özellik
								lApplyValue = affectin vericeði puan
								dwFlag = affecte atanan seçili olma
								bOverride = affecti tekrar sildiri verdirir yani removeaffect kullanmýyorsanýz affecte bOverride bunu truelayýp affecti sildirip tekrar verdirirsiniz. & clientede sildindiði bilgisi gönderir.
								IsCube = bu deðer otomatik false olarak döner bu kodun amacý affectin içinde affect kodu ve verilen bonusun özelliðin karakter üzerinde varmý yok mu aratmasý yani FindAffect(type) bu tarz kullanýyorsanýz bunu true yapabilirsiniz
								eðer kod baþlangýcýnda FindAffect(type,özellik) þeklinde ise tekrar sorgulatmanýza gerek yok
								*************************************************************************/
							}
							#endif

							if (item->GetValue(4) == 3874)
							{
								DWORD type = item->GetValue(0);
								if (AFFECT_NONE == type)
									return false;

								// if (!(type >= AFFECT_WISDOM_ELIXIR && type <= AFFECT_WISDOM_ELIXIR))
								if (type != AFFECT_WISDOM_ELIXIR)
									return false;

								CAffect * pAffect = FindAffect(type);
								if (NULL != pAffect)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
									return false;
								}

								long lTime = item->GetValue(3);
								if (lTime == 99) { lTime = INFINITE_AFFECT_DURATION; }

								AddAffect(type, aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), AFF_NONE, lTime, 0, true);
								item->SetCount(item->GetCount() - 1);
								break;
							}

							if (FindAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
							}
							else
							{
								AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false);
								item->SetCount(item->GetCount() - 1);
							}
						}
						break;

					case USE_CREATE_STONE:
						AutoGiveItem(number(28000, 28013));
						item->SetCount(item->GetCount() - 1);
						break;

					case USE_RECIPE :
						{
							LPITEM pSource1 = FindSpecifyItem(item->GetValue(1));
							DWORD dwSourceCount1 = item->GetValue(2);

							LPITEM pSource2 = FindSpecifyItem(item->GetValue(3));
							DWORD dwSourceCount2 = item->GetValue(4);

							if (dwSourceCount1 != 0)
							{
								if (pSource1 == NULL)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹°¾à Á¶ÇÕÀ» À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
									return false;
								}
							}

							if (dwSourceCount2 != 0)
							{
								if (pSource2 == NULL)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹°¾à Á¶ÇÕÀ» À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
									return false;
								}
							}

							if (pSource1 != NULL)
							{
								if (pSource1->GetCount() < dwSourceCount1)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àç·á(%s)°¡ ºÎÁ·ÇÕ´Ï´Ù."), pSource1->GetName());
									return false;
								}

								pSource1->SetCount(pSource1->GetCount() - dwSourceCount1);
							}

							if (pSource2 != NULL)
							{
								if (pSource2->GetCount() < dwSourceCount2)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àç·á(%s)°¡ ºÎÁ·ÇÕ´Ï´Ù."), pSource2->GetName());
									return false;
								}

								pSource2->SetCount(pSource2->GetCount() - dwSourceCount2);
							}

							LPITEM pBottle = FindSpecifyItem(50901);

							if (!pBottle || pBottle->GetCount() < 1)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ºó º´ÀÌ ¸ðÀÚ¸¨´Ï´Ù."));
								return false;
							}

							pBottle->SetCount(pBottle->GetCount() - 1);

							if (number(1, 100) > item->GetValue(5))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹°¾à Á¦Á¶¿¡ ½ÇÆÐÇß½À´Ï´Ù."));
								return false;
							}

							AutoGiveItem(item->GetValue(0));
						}
						break;
				}
			}
			break;

		case ITEM_METIN:
			{
				LPITEM item2;

				if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					return false;

				if (item2->GetType() == ITEM_PICK) return false;
				if (item2->GetType() == ITEM_ROD) return false;

				int i;
				for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				{
					DWORD dwVnum;
					if ((dwVnum = item2->GetSocket(i)) <= 2)
						continue;

					TItemTable * p = ITEM_MANAGER::instance().GetTable(dwVnum);
					if (!p)
						continue;

					if (item->GetValue(5) == p->alValues[5])
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°°Àº Á¾·ùÀÇ ¸ÞÆ¾¼®Àº ¿©·¯°³ ºÎÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
						return false;
					}
				}

				if (item2->GetType() == ITEM_ARMOR)
				{
					if (!IS_SET(item->GetWearFlag(), WEARABLE_BODY) || !IS_SET(item2->GetWearFlag(), WEARABLE_BODY))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¸ÞÆ¾¼®Àº Àåºñ¿¡ ºÎÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
						return false;
					}
				}
				else if (item2->GetType() == ITEM_WEAPON)
				{
					if (!IS_SET(item->GetWearFlag(), WEARABLE_WEAPON))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¸ÞÆ¾¼®Àº ¹«±â¿¡ ºÎÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ºÎÂøÇÒ ¼ö ÀÖ´Â ½½·ÔÀÌ ¾ø½À´Ï´Ù."));
					return false;
				}

				for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				{
					if (item2->GetSocket(i) >= 1 && item2->GetSocket(i) <= 2 && item2->GetSocket(i) >= item->GetValue(2))
					{
						int pct = 30;

						if (IS_CHECK_SUCCES_STONE(item->GetVnum()))
						{
							pct = 100;
						}
						else if (item->GetValue(4) == 99)
						{
							pct = 100;
						}

						#ifdef ENABLE_ADDSTONE_FAILURE
						if (number(1, 100) <= pct)
						#else
						if (1)
						#endif
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸ÞÆ¾¼® ºÎÂø¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));
							item2->SetSocket(i, item->GetVnum());
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸ÞÆ¾¼® ºÎÂø¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
							item2->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
						}

						LogManager::instance().ItemLog(this, item2, "SOCKET", item->GetName());
						item->SetCount(item->GetCount()-1);
						break;
					}
				}

				if (i == ITEM_SOCKET_MAX_NUM)
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ºÎÂøÇÒ ¼ö ÀÖ´Â ½½·ÔÀÌ ¾ø½À´Ï´Ù."));
			}
			break;

		case ITEM_AUTOUSE:
		case ITEM_MATERIAL:
		case ITEM_SPECIAL:
		case ITEM_TOOL:
		case ITEM_LOTTERY:
			break;

		case ITEM_TOTEM:
			{
				if (!item->IsEquipped())
					EquipItem(item);
			}
			break;

		case ITEM_BLEND:

			sys_log(0,"ITEM_BLEND!!");
			if (Blend_Item_find(item->GetVnum()))
			{
				int		affect_type		= AFFECT_BLEND;

				// @fixme209 BEGIN
				if (item->GetSocket(0) >= _countof(aApplyInfo))
				{
					sys_err("INVALID BLEND ITEM(id : %d, vnum : %d). APPLY TYPE IS %d.", item->GetID(), item->GetVnum(), item->GetSocket(0));
					return false;
				}
				// @fixme209 END

				int		apply_type		= aApplyInfo[item->GetSocket(0)].bPointType;
				int		apply_value		= item->GetSocket(1);
				int		apply_duration	= item->GetSocket(2);

				#ifdef ENABLE_PERMA_BLEND
				BYTE bBlendType = 0;
				switch (item->GetVnum())
				{
					case ITEM_NOMAL_BLEND_RED: { if (FindAffect(AFFECT_BLEND_RED)) bBlendType = 1; } break;
					case ITEM_NOMAL_BLEND_ORANGE: { if (FindAffect(AFFECT_BLEND_ORANGE)) bBlendType = 2; } break;
					case ITEM_NOMAL_BLEND_YELLOW: { if (FindAffect(AFFECT_BLEND_YELLOW)) bBlendType = 3; } break;
					case ITEM_NOMAL_BLEND_GREEN: { if (FindAffect(AFFECT_BLEND_GREEN)) bBlendType = 4; } break;
					case ITEM_NOMAL_BLEND_BLUE: { if (FindAffect(AFFECT_BLEND_BLUE)) bBlendType = 5; } break;
					case ITEM_NOMAL_BLEND_WHITE: { if (FindAffect(AFFECT_BLEND_WHITE)) bBlendType = 6; } break;
				}
				if (bBlendType > 0)
				{
						 if (bBlendType == 1) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma blend enable. red")); }
					else if (bBlendType == 2) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma blend enable. orange")); }
					else if (bBlendType == 3) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma blend enable. yellow")); }
					else if (bBlendType == 4) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma blend enable. green")); }
					else if (bBlendType == 5) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma blend enable. blue")); }
					else if (bBlendType == 6) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Perma blend enable. white")); }
					return false;
				}
				#endif

				if (FindAffect(affect_type, apply_type))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
				}
				else
				{
					if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_RESIST_MAGIC))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
					}
					else
					{
						AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, false);
						item->SetCount(item->GetCount() - 1);
					}
				}
			}
			break;

		case ITEM_EXTRACT:
			{
				LPITEM pDestItem = GetItem(DestCell);
				if (NULL == pDestItem)
				{
					return false;
				}
				switch (item->GetSubType())
				{
				case EXTRACT_DRAGON_SOUL:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().PullOut(this, NPOS, pDestItem, item);
					}
					return false;
				case EXTRACT_DRAGON_HEART:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().ExtractDragonHeart(this, pDestItem, item);
					}
					return false;
				default:
					return false;
				}
			}
			break;

		case ITEM_NONE:
			sys_err("Item type NONE %s", item->GetName());
			break;

		default:
			sys_log(0, "UseItemEx: Unknown type %s %d", item->GetName(), item->GetType());
			return false;
	}

	return true;
}

int g_nPortalLimitTime = TRADE_CHECK_TIME;

bool CHARACTER::UseItem(TItemPos Cell, TItemPos DestCell)
{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	//WORD wDestCell = DestCell.cell;
	//BYTE bDestInven = DestCell.window_type;
	LPITEM item;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
			return false;

	#ifdef ENABLE_VSTURNUVA
	if (GetMapIndex() == 28)
	{
		if (item->IsCostumeMount())
			return false;

		if (GetJob() == JOB_ASSASSIN)
		{
			if (item->GetType() == ITEM_WEAPON && item->GetSubType() == WEAPON_SWORD)
				return false;
		}
	}

	if ((GetMapIndex() == 28) || GetWarMap())
	{
		switch (item->GetVnum())
		{
			case 50816:
			case 39011:
			case 39012:
			case 39013:
			case 71018:
			case 71019:
			case 71020:
			case 27863:
			case 27865:
			case 27869:
			case 27874:
			case 27875:
			case 27878:
				return false;
		}
	}
	#endif

	sys_log(0, "%s: USE_ITEM %s (inven %d, cell: %d)", GetName(), item->GetName(), window_type, wCell);

	if (item->IsExchanging())
		return false;

	#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition())
	{
		CSwitchbot* pkSwitchbot = CSwitchbotManager::Instance().FindSwitchbot(GetPlayerID());
		if (pkSwitchbot && pkSwitchbot->IsActive(Cell.cell))
		{
			return false;
		}

		int iEmptyCell = GetEmptyInventory(item->GetSize());

		if (iEmptyCell == -1)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot remove item from switchbot. Inventory is full."));
			return false;
		}

		MoveItem(Cell, TItemPos(INVENTORY, iEmptyCell), item->GetCount());
		return true;
	}
	#endif

	if (!item->CanUsedBy(this))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±ºÁ÷ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (IsStun())
		return false;

	if (false == FN_check_item_sex(this, item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	//PREVENT_TRADE_WINDOW
	if (IS_SUMMON_ITEM(item->GetVnum()))
	{
		if (false == IS_SUMMONABLE_ZONE(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		if (CThreeWayWar::instance().IsThreeWayWarMapIndex(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ï°Å¸® ÀüÅõ Âü°¡Áß¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		int iPulse = thecore_pulse();
		if (iPulse - GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ã¢°í¸¦ ¿¬ÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);

			if (test_server)
				ChatPacket(CHAT_TYPE_INFO, "[TestOnly]Pulse %d LoadTime %d PASS %d", iPulse, GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
			return false;
		}

		if (PreventTradeWindow(WND_ALL))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
			return false;
		}

		//PREVENT_REFINE_HACK
		{
			if (iPulse - GetRefineTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ °³·®ÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);
				return false;
			}
		}
		//END_PREVENT_REFINE_HACK

		//PREVENT_ITEM_COPY
		{
			if (iPulse - GetMyShopTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³ÀÎ»óÁ¡ »ç¿ëÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);
				return false;
			}
		}
		//END_PREVENT_ITEM_COPY

		if (item->GetVnum() != 70302)
		{
			PIXEL_POSITION posWarp;

			int x = 0;
			int y = 0;

			double nDist = 0;
			const double nDistant = 5000.0;

			if (item->GetVnum() == 22010)
			{
				x = item->GetSocket(0) - GetX();
				y = item->GetSocket(1) - GetY();
			}

			else if (item->GetVnum() == 22000)
			{
				SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp);

				if (item->GetSocket(0) == 0)
				{
					x = posWarp.x - GetX();
					y = posWarp.y - GetY();
				}
				else
				{
					x = item->GetSocket(0) - GetX();
					y = item->GetSocket(1) - GetY();
				}
			}

			nDist = sqrt(pow((float)x,2) + pow((float)y,2));

			if (nDistant > nDist)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌµ¿ µÇ¾îÁú À§Ä¡¿Í ³Ê¹« °¡±î¿ö ±ÍÈ¯ºÎ¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
				if (test_server)
					ChatPacket(CHAT_TYPE_INFO, "PossibleDistant %f nNowDist %f", nDistant,nDist);
				return false;
			}
		}

		//PREVENT_PORTAL_AFTER_EXCHANGE

		if (iPulse - GetExchangeTime()  < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°Å·¡ ÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎµîÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);
			return false;
		}
		//END_PREVENT_PORTAL_AFTER_EXCHANGE
	}

	if ((item->GetVnum() == 50200) || (item->GetVnum() == 71049))
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°Å·¡Ã¢,Ã¢°í µîÀ» ¿¬ »óÅÂ¿¡¼­´Â º¸µû¸®,ºñ´Üº¸µû¸®¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}
	//END_PREVENT_TRADE_WINDOW

	// @fixme150 BEGIN
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item if you're using quests"));
		return false;
	}
	// @fixme150 END

	if (IS_SET(item->GetFlag(), ITEM_FLAG_LOG))
	{
		DWORD vid = item->GetVID();
		DWORD oldCount = item->GetCount();
		DWORD vnum = item->GetVnum();

		char hint[ITEM_NAME_MAX_LEN + 32 + 1];
		int len = snprintf(hint, sizeof(hint) - 32, "%s", item->GetName());

		if (len < 0 || len >= (int) sizeof(hint) - 32)
			len = (sizeof(hint) - 32) - 1;

		bool ret = UseItemEx(item, DestCell);

		if (NULL == ITEM_MANAGER::instance().FindByVID(vid))
		{
			LogManager::instance().ItemLog(this, vid, vnum, "REMOVE", hint);
		}
		else if (oldCount != item->GetCount())
		{
			snprintf(hint + len, sizeof(hint) - len, " %u", oldCount - 1);
			LogManager::instance().ItemLog(this, vid, vnum, "USE_ITEM", hint);
		}
		return (ret);
	}
	else
		return UseItemEx(item, DestCell);
}

#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
bool CHARACTER::DeleteItem(TItemPos Cell)
{
	LPITEM item = NULL;

	if (!CanHandleItem())
		return false;

	if (IsDead() || IsStun())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (true == item->isLocked() || item->IsExchanging() || item->IsEquipped())
		return false;

	// #ifdef ENABLE_LOCKED_SYSTEM
	// if (true == item->IsItemLocked())
		// return false;
	// #endif

	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
		return false;

	if (item->GetVnum() == 76042 || item->GetVnum() == 84001)
		return false;

	if (PreventTradeWindow(WND_ALL))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return false;
	}

	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s has been deleted successfully."), item->GetName());

	ITEM_MANAGER::instance().RemoveItem(item);
	return true;
}

bool CHARACTER::SellItem(TItemPos Cell, bool isAuto)
{
	LPITEM item = NULL;

	if (!CanHandleItem())
		return false;

	if (IsDead() || IsStun())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (true == item->isLocked() || item->IsExchanging() || item->IsEquipped())
		return false;

	#ifdef ENABLE_LOCKED_SYSTEM
	if (!(item->GetVnum() >= 80003 && item->GetVnum() <= 80007))
	{
		if (true == item->IsItemLocked())
			return false;
	}
	#endif

	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_SELL))
		return false;

	if (PreventTradeWindow(WND_ALL))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return false;
	}

	if (isAuto)
	{
		if ((item->GetType() == ITEM_WEAPON) || (item->GetType() == ITEM_ARMOR))
		{
			int iAttrCount = item->GetAttributeCount();
			// if (iAttrCount >= 3)
			if (iAttrCount >= 4)
			{
				sys_err("Cannot be sell more attr 4. ItemName: %s - UserName: %s", item->GetName(), this->GetName());
				return false;
			}
		}
	}

	#ifdef ENABLE_FULL_YANG
	int64_t dwPrice;
	#else
	DWORD dwPrice;
	#endif

	#ifdef ENABLE_NEW_MAX_COUNT
	short bCount;
	#else
	BYTE bCount;
	#endif

	bCount = item->GetCount();
	dwPrice = item->GetShopBuyPrice();

	if (IS_SET(item->GetFlag(), ITEM_FLAG_COUNT_PER_1GOLD))
	{
		if (dwPrice == 0)
			dwPrice = bCount;
		else
			dwPrice = bCount / dwPrice;
	}
	else
		dwPrice *= bCount;

	const int64_t nTotalMoney = static_cast<int64_t>(GetGold()) + static_cast<int64_t>(dwPrice);
	#ifdef ENABLE_FULL_YANG
	if (GetAllowedGold() <= nTotalMoney)
	#else
	if (GOLD_MAX <= nTotalMoney)
	#endif
	{
		sys_err("[OVERFLOW_GOLD] id %u name %s gold %lld", GetPlayerID(), GetName(), GetGold());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("20¾ï³ÉÀÌ ÃÊ°úÇÏ¿© ¹°Ç°À» ÆÈ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s has been sold successfully."), item->GetName());

	ITEM_MANAGER::instance().RemoveItem(item);

	#ifdef ENABLE_FULL_YANG
	GoldChange(dwPrice);
	#else
	PointChange(POINT_GOLD, dwPrice, false);
	#endif
	return true;
}
#else
bool CHARACTER::DropItem(TItemPos Cell
	#ifdef ENABLE_NEW_MAX_COUNT
	, short bCount
	#else
	, BYTE bCount
	#endif
)
{
	LPITEM item = NULL;

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°­È­Ã¢À» ¿¬ »óÅÂ¿¡¼­´Â ¾ÆÀÌÅÛÀ» ¿Å±æ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

#ifdef ENABLE_NEWSTUFF
	if (g_ItemDropTimeLimitValue && !PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::ItemDrop, std::chrono::milliseconds(g_ItemDropTimeLimitValue)))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ °ñµå¸¦ ¹ö¸± ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}
#endif

	if (IsDead())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (true == item->isLocked())
		return false;

	#ifdef ENABLE_LOCKED_SYSTEM
	if (true == item->IsItemLocked())
		return false;
	#endif

	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP | ITEM_ANTIFLAG_GIVE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹ö¸± ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		return false;
	}

	if (PreventTradeWindow(WND_ALL))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return false;
	}

	if (bCount == 0 || bCount > item->GetCount())
		bCount = item->GetCount();

	SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, 255);

	LPITEM pkItemToDrop;

	if (bCount == item->GetCount())
	{
		item->RemoveFromCharacter();
		pkItemToDrop = item;
	}
	else
	{
		if (bCount == 0)
		{
			if (test_server)
				sys_log(0, "[DROP_ITEM] drop item count == 0");
			return false;
		}

		item->SetCount(item->GetCount() - bCount);
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		pkItemToDrop = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), bCount);

		// copy item socket -- by mhh
		FN_copy_item_socket(pkItemToDrop, item);

		char szBuf[51 + 1];
		snprintf(szBuf, sizeof(szBuf), "%u %u", pkItemToDrop->GetID(), pkItemToDrop->GetCount());
		LogManager::instance().ItemLog(this, item, "ITEM_SPLIT", szBuf);
	}

	PIXEL_POSITION pxPos = GetXYZ();

	if (pkItemToDrop->AddToGround(GetMapIndex(), pxPos))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¶³¾îÁø ¾ÆÀÌÅÛÀº 3ºÐ ÈÄ »ç¶óÁý´Ï´Ù."));
		#ifdef ENABLE_NEWSTUFF
		pkItemToDrop->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_DROPITEM]);
		#else
		pkItemToDrop->StartDestroyEvent();
		#endif

		ITEM_MANAGER::instance().FlushDelayedSave(pkItemToDrop);

		char szHint[32 + 1];
		snprintf(szHint, sizeof(szHint), "%s %u %u", pkItemToDrop->GetName(), pkItemToDrop->GetCount(), pkItemToDrop->GetOriginalVnum());
		LogManager::instance().ItemLog(this, pkItemToDrop, "DROP", szHint);
		//Motion(MOTION_PICKUP);
	}

	return true;
}
#endif

#ifdef ENABLE_FULL_YANG
bool CHARACTER::DropGold(int64_t gold)
#else
bool CHARACTER::DropGold(int gold)
#endif
{
	return false;

	if (gold <= 0 || gold > GetGold())
		return false;

	if (!CanHandleItem())
		return false;

#ifdef ENABLE_NEWSTUFF
	if (g_GoldDropTimeLimitValue && !PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::BoxOpening, std::chrono::milliseconds(g_GoldDropTimeLimitValue)))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ °ñµå¸¦ ¹ö¸± ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}
#endif

	LPITEM item = ITEM_MANAGER::instance().CreateItem(1, gold);

	if (item)
	{
		PIXEL_POSITION pos = GetXYZ();

		if (item->AddToGround(GetMapIndex(), pos))
		{
			#ifdef ENABLE_FULL_YANG
			GoldChange(-gold);
			#else
			PointChange(POINT_GOLD, -gold, true);
			#endif

			if (gold > 1000)
				LogManager::instance().CharLog(this, gold, "DROP_GOLD", "");

			#ifdef ENABLE_NEWSTUFF
			item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_DROPGOLD]);
			#else
			item->StartDestroyEvent();
			#endif
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¶³¾îÁø ¾ÆÀÌÅÛÀº %dºÐ ÈÄ »ç¶óÁý´Ï´Ù."), 150/60);
		}

		Save();
		return true;
	}

	return false;
}

#ifdef ENABLE_NEW_MAX_COUNT
bool CHARACTER::MoveItem(TItemPos Cell, TItemPos DestCell, short count)
#else
bool CHARACTER::MoveItem(TItemPos Cell, TItemPos DestCell, BYTE count)
#endif
{
	// kopyalama fixi
	if (m_pkTimedEvent)
	{
		return false;
	}
	// kopyalama fixi

	if (Cell.IsSamePosition(DestCell)) // @fixme196 (check same slot n same window aliases)
		return false;

	if (!IsValidItemPosition(Cell))
		return false;

	LPITEM item = NULL;
	if (!(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (item->GetCount() < count)
		return false;

	if (INVENTORY == Cell.window_type && Cell.cell >= INVENTORY_MAX_NUM && IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

	#ifdef ENABLE_SPECIAL_STORAGE
	if (item->IsUpgradeItem() && DestCell.window_type != UPGRADE_INVENTORY)
		return false;

	if (item->IsBook() && DestCell.window_type != BOOK_INVENTORY)
		return false;

	if (item->IsStone() && DestCell.window_type != STONE_INVENTORY)
		return false;

	if (item->IsChest() && DestCell.window_type != CHEST_INVENTORY)
		return false;
	#endif

	#ifdef ENABLE_VSTURNUVA
	if (GetMapIndex() == 28 && item->IsCostumeMount())
		return false;
	#endif

	if (true == item->isLocked())
		return false;

	// #ifdef ENABLE_LOCKED_SYSTEM
	// if (true == item->IsItemLocked())
		// return false;
	// #endif

	if (!IsValidItemPosition(DestCell))
	{
		return false;
	}

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°­È­Ã¢À» ¿¬ »óÅÂ¿¡¼­´Â ¾ÆÀÌÅÛÀ» ¿Å±æ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (IsOpenSafebox())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot move it while the warehouse window is open."));
		return false;
	}

	if (DestCell.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº º§Æ® ÀÎº¥Åä¸®·Î ¿Å±æ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition() && CSwitchbotManager::Instance().IsActive(GetPlayerID(), Cell.cell))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot move active switchbot item."));
		return false;
	}

	if ((DestCell.IsSwitchbotPosition() && item->IsEquipped()) || (Cell.IsSwitchbotPosition() && DestCell.IsEquipPosition()))
	{
		return false;
	}

	if (DestCell.IsSwitchbotPosition() && !SwitchbotHelper::IsValidItem(item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Invalid item type for switchbot."));
		return false;
	}

	if (Cell.IsSwitchbotPosition())
	{
		if (CSwitchbotManager::Instance().IsActive(GetPlayerID(), Cell.cell))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot move active switchbot item."));
			return false;
		}

		if (DestCell.IsEquipPosition())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±ºÁ÷ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	if (DestCell.IsSwitchbotPosition())
	{
		if (item->IsEquipped())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±ºÁ÷ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		if (!SwitchbotHelper::IsValidItem(item))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Invalid item type for switchbot."));
			return false;
		}
	}

	// copy randomise item problem solved. (dupe fix)
	if (Cell.window_type == SWITCHBOT && DestCell.window_type != INVENTORY)
		return false;
	#endif

	#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (DestCell.IsDefaultInventoryPosition() && Cell.IsEquipPosition())
	{
		if (!CanUnequipNow(item))
			return false;

		if (item->IsCostumeMount() && GetHorse())
		{
			if (IsRiding())
				StopRiding();

			HorseSummon(false);
		}
	}
	#endif

	if (Cell.IsEquipPosition())
	{
		if (!CanUnequipNow(item))
			return false;

		#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		int iWearCell = item->FindEquipCell(this);
		if (iWearCell == WEAR_WEAPON)
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !UnequipItem(costumeWeapon))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot unequip the costume weapon. Not enough space."));
				return false;
			}

			if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
				return UnequipItem(item);
		}
		#endif
	}

	if (DestCell.IsEquipPosition())
	{
		if (GetItem(DestCell))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì Àåºñ¸¦ Âø¿ëÇÏ°í ÀÖ½À´Ï´Ù."));
			return false;
		}

		EquipItem(item, DestCell.cell - INVENTORY_MAX_NUM);
	}
	else
	{
		if (item->IsDragonSoul())
		{
			if (item->IsEquipped())
			{
				return DSManager::instance().PullOut(this, DestCell, item);
			}
			else
			{
				if (DestCell.window_type != DRAGON_SOUL_INVENTORY)
				{
					return false;
				}

				if (!DSManager::instance().IsValidCellForThisItem(item, DestCell))
					return false;
			}
		}

		else if (DRAGON_SOUL_INVENTORY == DestCell.window_type)
			return false;

		#ifdef ENABLE_SPECIAL_STORAGE
		if (!item->IsUpgradeItem() && UPGRADE_INVENTORY == DestCell.window_type)
			return false;

		if (!item->IsBook() && BOOK_INVENTORY == DestCell.window_type)
			return false;

		if (!item->IsStone() && STONE_INVENTORY == DestCell.window_type)
			return false;

		if (!item->IsChest() && CHEST_INVENTORY == DestCell.window_type)
			return false;
		#endif

		LPITEM item2;
		if ((item2 = GetItem(DestCell)) && item != item2 && item2->IsStackable() &&
				!IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_STACK) &&
				item2->GetVnum() == item->GetVnum())
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				if (item2->GetSocket(i) != item->GetSocket(i))
					return false;

			if (true == item2->isLocked())
				return false;

			#ifdef ENABLE_LOCKED_SYSTEM
			if (true == item->IsItemLocked() && false == item2->IsItemLocked())
				return false;
			if (false == item->IsItemLocked() && true == item2->IsItemLocked())
				return false;
			#endif

			// ori to fix (remove item stack bug fix)
			if (item->GetOriginalVnum() != item2->GetOriginalVnum())
				return false;

			if (count == 0)
				count = item->GetCount();

			sys_log(0, "%s: ITEM_STACK %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell,
				DestCell.window_type, DestCell.cell, count);

			count = MIN(g_bItemCountLimit - item2->GetCount(), count);

			item->SetCount(item->GetCount() - count);
			item2->SetCount(item2->GetCount() + count);
			return true;
		}

		if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
			return false;

		if (count == 0 || count >= item->GetCount() || !item->IsStackable() || IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			sys_log(0, "%s: ITEM_MOVE %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell,
				DestCell.window_type, DestCell.cell, count);

			item->RemoveFromCharacter();

			#ifdef ENABLE_HIGHLIGHT_NEW_ITEM
			SetItem(DestCell, item, true);
			#else
			SetItem(DestCell, item);
			#endif

			if (INVENTORY == Cell.window_type && INVENTORY == DestCell.window_type)
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, DestCell.cell);
		}
		else if (count < item->GetCount())
		{
			sys_log(0, "%s: ITEM_SPLIT %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell,
				DestCell.window_type, DestCell.cell, count);

			item->SetCount(item->GetCount() - count);
			LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), count);

			// copy socket -- by mhh
			FN_copy_item_socket(item2, item);

			#ifdef ENABLE_LOCKED_SYSTEM
			if (item->IsItemLocked())
			{
				item2->SetLocked(item->GetLocked());
			}
			#endif

			item2->AddToCharacter(this, DestCell);

			char szBuf[51+1];
			snprintf(szBuf, sizeof(szBuf), "%u %u %u %u ", item2->GetID(), item2->GetCount(), item->GetCount(), item->GetCount() + item2->GetCount());
			LogManager::instance().ItemLog(this, item, "ITEM_SPLIT", szBuf);
		}
	}

	return true;
}

namespace NPartyPickupDistribute
{
	struct FFindOwnership
	{
		LPITEM item;
		LPCHARACTER owner;

		FFindOwnership(LPITEM item)
			: item(item), owner(NULL)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (item->IsOwnership(ch))
				owner = ch;
		}
	};

	struct FCountNearMember
	{
		int		total;
		int		x, y;

		FCountNearMember(LPCHARACTER center )
			: total(0), x(center->GetX()), y(center->GetY())
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				total += 1;
		}
	};

	struct FMoneyDistributor
	{
		int		total;
		LPCHARACTER	c;
		int		x, y;
		int		iMoney;

		FMoneyDistributor(LPCHARACTER center, int iMoney)
			: total(0), c(center), x(center->GetX()), y(center->GetY()), iMoney(iMoney)
		{
		}

		void operator ()(LPCHARACTER ch)
		{
			if (ch!=c)
				if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				{
					#ifdef ENABLE_FULL_YANG
					ch->GoldChange(iMoney);
					#else
					ch->PointChange(POINT_GOLD, iMoney, true);
					#endif

					if (iMoney > 1000)
					{
						LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().CharLog(ch, iMoney, "GET_GOLD", ""));
					}
				}
		}
	};
}

#ifdef ENABLE_FULL_YANG
void CHARACTER::GiveGold(int64_t iAmount)
#else
void CHARACTER::GiveGold(int iAmount)
#endif
{
	if (iAmount <= 0)
		return;

	if (iAmount > 100000)
	{
		#ifdef ENABLE_FULL_YANG
		sys_log(0, "GIVE_GOLD: %s %lld", GetName(), iAmount);
		#else
		sys_log(0, "GIVE_GOLD: %s %d", GetName(), iAmount);
		#endif
	}

	#ifdef ENABLE_BATTLE_PASS
	uint8_t bBattlePassId = GetBattlePassId();
	if (bBattlePassId)
	{
		CBattlePass::instance().BattlePassCheckMissions(this, bBattlePassId, BP_MT_FARM_YANG, 0, iAmount);
	}
	#endif

	if (GetParty())
	{
		LPPARTY pParty = GetParty();

		#ifdef ENABLE_FULL_YANG
		auto dwTotal = iAmount;
		auto dwMyAmount = dwTotal;
		#else
		DWORD dwTotal = iAmount;
		DWORD dwMyAmount = dwTotal;
		#endif

		NPartyPickupDistribute::FCountNearMember funcCountNearMember(this);
		pParty->ForEachOnlineMember(funcCountNearMember);

		if (funcCountNearMember.total > 1)
		{
			#ifdef ENABLE_FULL_YANG
			uint64_t dwShare = dwTotal / funcCountNearMember.total;
			#else
			DWORD dwShare = dwTotal / funcCountNearMember.total;
			#endif
			dwMyAmount -= dwShare * (funcCountNearMember.total - 1);

			NPartyPickupDistribute::FMoneyDistributor funcMoneyDist(this, dwShare);

			pParty->ForEachOnlineMember(funcMoneyDist);
		}

		#ifdef ENABLE_FULL_YANG
		GoldChange(dwMyAmount);
		#else
		PointChange(POINT_GOLD, dwMyAmount, true);
		#endif

		if (dwMyAmount > 1000)
		{
			LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().CharLog(this, dwMyAmount, "GET_GOLD", ""));
		}
	}
	else
	{
		#ifdef ENABLE_FULL_YANG
		GoldChange(iAmount);
		#else
		PointChange(POINT_GOLD, iAmount, true);
		#endif

		if (iAmount > 1000)
		{
			LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().CharLog(this, iAmount, "GET_GOLD", ""));
		}
	}
}

bool CHARACTER::PickupItem(DWORD dwVID)
{
	if (IsPC() && IsDead())// @fixme243
		return false;

	#ifdef ENABLE_MULTI_FARM_BLOCK
	if (!GetRewardStatus())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bu_hesaptan_drop_alamazsin"));
		return false;
	}
	#endif

	LPITEM item = ITEM_MANAGER::instance().FindByVID(dwVID);

	if (IsObserverMode())
		return false;

	if (!item || !item->GetSectree())
		return false;

	if (item->DistanceValid(this))
	{
		// @fixme150 BEGIN
		if (item->GetType() == ITEM_QUEST)
		{
			if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot pickup this item if you're using quests"));
				return false;
			}
		}
		// @fixme150 END

		if (item->IsOwnership(this))
		{
			#ifdef ENABLE_AUTO_ITEM_SELL_SYSTEM
			if (IsAutoItemSell(item->GetVnum(), item->GetCount()))
			{
				item->RemoveFromGround();
				M2_DESTROY_ITEM(item);
				Save();
				return true;
			}
			#endif

			if (item->GetType() == ITEM_ELK)
			{
				GiveGold(item->GetCount());
				item->RemoveFromGround();
				M2_DESTROY_ITEM(item);
				Save();
			}
			else
			{
				if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					auto bCount = item->GetCount();

					#ifdef ENABLE_SPECIAL_STORAGE
					if (item->IsUpgradeItem())
					{
						for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
						{
							LPITEM item2 = GetUpgradeInventoryItem(i);
							if (!item2) { continue; }
							#ifdef ENABLE_LOCKED_SYSTEM
							if (item2->IsItemLocked()) { continue; }
							#endif

							// ori to fix
							if (item->GetOriginalVnum() != item2->GetOriginalVnum())
								continue;

							if (item2->GetVnum() == item->GetVnum())
							{
								if (item2->GetCount() == g_bItemCountLimit)//Fix
									continue;

								auto bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
								bCount -= bCount2;

								if (bCount2 > 0)
									item2->SetCount(item2->GetCount() + bCount2);

								if (bCount == 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("arti_env_%s"), item2->GetName());
									M2_DESTROY_ITEM(item);
									return true;
								}
							}
						}
						item->SetCount(bCount);
					}
					else if (item->IsBook())
					{
						for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
						{
							LPITEM item2 = GetBookInventoryItem(i);
							if (!item2) { continue; }
							#ifdef ENABLE_LOCKED_SYSTEM
							if (item2->IsItemLocked()) { continue; }
							#endif

							// ori to fix
							if (item->GetOriginalVnum() != item2->GetOriginalVnum())
								continue;

							if (item2->GetVnum() == item->GetVnum())
							{
								if (item2->GetCount() == g_bItemCountLimit)//Fix
									continue;

								int j;
								for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
									if (item2->GetSocket(j) != item->GetSocket(j))
										break;

								if (j != ITEM_SOCKET_MAX_NUM)
									continue;

								auto bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
								bCount -= bCount2;

								if (bCount2 > 0)
									item2->SetCount(item2->GetCount() + bCount2);

								if (bCount == 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bk_env_%s"), item2->GetName());
									M2_DESTROY_ITEM(item);
									return true;
								}
							}
						}
						item->SetCount(bCount);
					}
					else if (item->IsStone())
					{
						for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
						{
							LPITEM item2 = GetStoneInventoryItem(i);
							if (!item2) { continue; }
							#ifdef ENABLE_LOCKED_SYSTEM
							if (item2->IsItemLocked()) { continue; }
							#endif

							// ori to fix
							if (item->GetOriginalVnum() != item2->GetOriginalVnum())
								continue;

							if (item2->GetVnum() == item->GetVnum())
							{
								if (item2->GetCount() == g_bItemCountLimit)//Fix
									continue;

								auto bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
								bCount -= bCount2;

								if (bCount2 > 0)
									item2->SetCount(item2->GetCount() + bCount2);

								if (bCount == 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("tas_env_%s"), item2->GetName());
									M2_DESTROY_ITEM(item);
									return true;
								}
							}
						}
						item->SetCount(bCount);
					}
					else if (item->IsChest())
					{
						for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
						{
							LPITEM item2 = GetChestInventoryItem(i);
							if (!item2) { continue; }
							#ifdef ENABLE_LOCKED_SYSTEM
							if (item2->IsItemLocked()) { continue; }
							#endif

							// ori to fix
							if (item->GetOriginalVnum() != item2->GetOriginalVnum())
								continue;

							if (item2->GetVnum() == item->GetVnum())
							{
								if (item2->GetCount() == g_bItemCountLimit)//Fix
									continue;

								auto bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
								bCount -= bCount2;

								if (bCount2 > 0)
									item2->SetCount(item2->GetCount() + bCount2);

								if (bCount == 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("sandik_env_%s"), item2->GetName());
									M2_DESTROY_ITEM(item);
									return true;
								}
							}
						}
						item->SetCount(bCount);
					}
					else
					#endif
					{
						for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
						{
							LPITEM item2 = GetInventoryItem(i);
							if (!item2) { continue; }
							#ifdef ENABLE_LOCKED_SYSTEM
							if (item2->IsItemLocked()) { continue; }
							#endif

							if (item2->GetVnum() == item->GetVnum())
							{
								if (item2->GetCount() == g_bItemCountLimit)//Fix
									continue;

								int j;
								for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
									if (item2->GetSocket(j) != item->GetSocket(j))
										break;

								if (j != ITEM_SOCKET_MAX_NUM)
									continue;

								// ori to fix
								if (item->GetOriginalVnum() != item2->GetOriginalVnum())
									continue;

								auto bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
								bCount -= bCount2;

								if (bCount2 > 0)
									item2->SetCount(item2->GetCount() + bCount2);

								if (bCount == 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("normal_env_%s"), item2->GetName());
									M2_DESTROY_ITEM(item);
									if (item2->GetType() == ITEM_QUEST)
										quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
									return true;
								}
							}
						}
						item->SetCount(bCount);
					}
				}

				int iEmptyCell;
				if (item->IsDragonSoul())
				{
					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						sys_log(0, "No empty ds inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
				#ifdef ENABLE_SPECIAL_STORAGE
				else if (item->IsUpgradeItem())
				{
					if ((iEmptyCell = GetEmptyUpgradeInventory(item)) == -1)
					{
						sys_log(0, "No empty ssu inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
				else if (item->IsBook())
				{
					if ((iEmptyCell = GetEmptyBookInventory(item)) == -1)
					{
						sys_log(0, "No empty ssu inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
				else if (item->IsStone())
				{
					if ((iEmptyCell = GetEmptyStoneInventory(item)) == -1)
					{
						sys_log(0, "No empty ssu inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
				else if (item->IsChest())
				{
					if ((iEmptyCell = GetEmptyChestInventory(item)) == -1)
					{
						sys_log(0, "No empty ssc inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
				#endif
				else
				{
					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						sys_log(0, "No empty inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}

				item->RemoveFromGround();

				item->AddToItem(this, iEmptyCell);

				char szHint[32+1];
				snprintf(szHint, sizeof(szHint), "%s %u %u", item->GetName(), item->GetCount(), item->GetOriginalVnum());
				LogManager::instance().ItemLog(this, item, "GET", szHint);

				#ifdef ENABLE_SPECIAL_STORAGE
				if (item->IsUpgradeItem())
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("arti_env_%s"), item->GetName());
				else if (item->IsBook())
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bk_env_%s"), item->GetName());
				else if (item->IsStone())
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("tas_env_%s"), item->GetName());
				else if (item->IsChest())
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("sandik_env_%s"), item->GetName());
				else
				#endif
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("normal_env_%s"), item->GetName());

				if (item->GetType() == ITEM_QUEST)
					quest::CQuestManager::instance().PickupItem (GetPlayerID(), item);
			}

			//Motion(MOTION_PICKUP);
			return true;
		}
		else if (!IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_DROP) && GetParty())
		{
			NPartyPickupDistribute::FFindOwnership funcFindOwnership(item);
			GetParty()->ForEachOnlineMember(funcFindOwnership);

			LPCHARACTER owner = funcFindOwnership.owner;
			// @fixme115
			if (!owner) { return false; }

			#ifdef ENABLE_AUTO_ITEM_SELL_SYSTEM
			if (owner->IsAutoItemSell(item->GetVnum(), item->GetCount()))
			{
				if (item->GetType() == ITEM_QUEST)
				{
					quest::CQuestManager::instance().PickupItem(owner->GetPlayerID(), item);
				}
				item->RemoveFromGround();
				M2_DESTROY_ITEM(item);
				owner->Save();
				return true;
			}
			#endif

			int iEmptyCell;
			if (item->IsDragonSoul())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyDragonSoulInventory(item)) != -1))
				{
					owner = this;
					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}
			#ifdef ENABLE_SPECIAL_STORAGE
			else if (item->IsUpgradeItem())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyUpgradeInventory(item)) != -1))
				{
					owner = this;
					if ((iEmptyCell = GetEmptyUpgradeInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}
			else if (item->IsBook())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyBookInventory(item)) != -1))
				{
					owner = this;
					if ((iEmptyCell = GetEmptyBookInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}
			else if (item->IsStone())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyStoneInventory(item)) != -1))
				{
					owner = this;
					if ((iEmptyCell = GetEmptyStoneInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}
			else if (item->IsChest())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyChestInventory(item)) != -1))
				{
					owner = this;
					if ((iEmptyCell = GetEmptyChestInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}
			#endif
			else
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyInventory(item->GetSize())) != -1))
				{
					owner = this;
					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}

			item->RemoveFromGround();

			item->AddToItem(owner, iEmptyCell);

			char szHint[32+1];
			snprintf(szHint, sizeof(szHint), "%s %u %u", item->GetName(), item->GetCount(), item->GetOriginalVnum());
			LogManager::instance().ItemLog(owner, item, "GET", szHint);

			if (owner == this)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ È¹µæ: %s"), item->GetName());
			else
			{
				owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ È¹µæ: %s ´ÔÀ¸·ÎºÎÅÍ %s"), GetName(), item->GetName());
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ Àü´Þ: %s ´Ô¿¡°Ô %s"), owner->GetName(), item->GetName());
			}

			if (item->GetType() == ITEM_QUEST)
			{
				quest::CQuestManager::instance().PickupItem(owner->GetPlayerID(), item);
			}
			return true;
		}
	}

	return false;
}

bool CHARACTER::SwapItem(BYTE bCell, BYTE bDestCell)
{
	if (!CanHandleItem())
		return false;

	TItemPos srcCell(INVENTORY, bCell), destCell(INVENTORY, bDestCell);

	//if (bCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM || bDestCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM)
	if (srcCell.IsDragonSoulEquipPosition() || destCell.IsDragonSoulEquipPosition())
		return false;

	if (bCell == bDestCell)
		return false;

	if (srcCell.IsEquipPosition() && destCell.IsEquipPosition())
		return false;

	LPITEM item1, item2;

	if (srcCell.IsEquipPosition())
	{
		item1 = GetInventoryItem(bDestCell);
		item2 = GetInventoryItem(bCell);
	}
	else
	{
		item1 = GetInventoryItem(bCell);
		item2 = GetInventoryItem(bDestCell);
	}

	if (!item1 || !item2)
		return false;

	if (item1 == item2)
	{
		sys_log(0, "[WARNING][WARNING][HACK USER!] : %s %d %d", m_stName.c_str(), bCell, bDestCell);
		return false;
	}

	if (!IsEmptyItemGrid(TItemPos (INVENTORY, item1->GetCell()), item2->GetSize(), item1->GetCell()))
		return false;

	if (TItemPos(EQUIPMENT, item2->GetCell()).IsEquipPosition())
	{
		BYTE bEquipCell = item2->GetCell() - INVENTORY_MAX_NUM;
		BYTE bInvenCell = item1->GetCell();

		if (item2->IsDragonSoul() || item2->GetType() == ITEM_BELT) // @fixme117
		{
			if (false == CanUnequipNow(item2) || false == CanEquipNow(item1))
				return false;
		}

		if (bEquipCell != item1->FindEquipCell(this))
			return false;

		item2->RemoveFromCharacter();

		if (item1->EquipTo(this, bEquipCell))
			item2->AddToCharacter(this, TItemPos(INVENTORY, bInvenCell));
		else
			sys_err("SwapItem cannot equip %s! item1 %s", item2->GetName(), item1->GetName());
	}
	else
	{
		BYTE bCell1 = item1->GetCell();
		BYTE bCell2 = item2->GetCell();

		item1->RemoveFromCharacter();
		item2->RemoveFromCharacter();

		item1->AddToCharacter(this, TItemPos(INVENTORY, bCell2));
		item2->AddToCharacter(this, TItemPos(INVENTORY, bCell1));
	}

	return true;
}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
void CHARACTER::Dismount()
{
	RemoveAffect(AFFECT_MOUNT);
	RemoveAffect(AFFECT_MOUNT_BONUS);

	MountVnum(0);

	PointChange(POINT_ST, 0);
	PointChange(POINT_DX, 0);
	PointChange(POINT_HT, 0);
	PointChange(POINT_IQ, 0);

	if (IsRiding())
		StopRiding();

	if (GetHorse())
		HorseSummon(false);

	// SetProtectTime("mountcostume", get_global_time() + 2);
}
#endif

bool CHARACTER::UnequipItem(LPITEM item)
{
	#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	int iWearCell = item->FindEquipCell(this);
	if (iWearCell == WEAR_WEAPON)
	{
		LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
		if (costumeWeapon && !UnequipItem(costumeWeapon))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot unequip the costume weapon. Not enough space."));
			return false;
		}
	}
	#endif

	#ifdef ENABLE_VSTURNUVA
	if (GetMapIndex() == 28)
	{
		if (item->IsCostumeMount())
			return false;

		if (int(GetQuestFlag("pvp.item_block")) == 1)
			return false;

		if (GetJob() == JOB_ASSASSIN)
		{
			if (item->GetType() == ITEM_WEAPON && item->GetSubType() == WEAPON_SWORD)
				return false;
		}
	}
	#endif

	if (false == CanUnequipNow(item))
		return false;

	int pos = GetEmptyAllInventory(item);

	// HARD CODING
	if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		ShowAlignment(true);

	item->RemoveFromCharacter();
	item->AddToItem(this, pos);

	/*#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (item->IsCostumeMount())
	{
		if (IsRiding())
			StopRiding();

		if (GetHorse())
			HorseSummon(false);

		SetProtectTime("mountcostume", get_global_time() + 2);
	}
	#endif*/

	CheckMaximumPoints();
	return true;
}

bool CHARACTER::EquipItem(LPITEM item, int iCandidateCell)
{
	if (item->IsExchanging())
		return false;

	if (false == item->IsEquipable())
		return false;

	if (false == CanEquipNow(item))
		return false;

	#ifdef ENABLE_VSTURNUVA
	if (GetMapIndex() == 28)
	{
		if (item->IsCostumeMount())
			return false;

		if (int(GetQuestFlag("pvp.item_block")) == 1)
			return false;

		if (GetJob() == JOB_ASSASSIN)
		{
			if (item->GetType() == ITEM_WEAPON && item->GetSubType() == WEAPON_SWORD)
				return false;
		}
	}
	#endif

	int iWearCell = item->FindEquipCell(this, iCandidateCell);
	if (iWearCell < 0)
		return false;

	char szWearBuf[32+1];
	snprintf(szWearBuf, sizeof(szWearBuf), "equip_wear_%d.block", iWearCell);
	// if (IsRiding() && (get_global_time() < GetProtectTime(szWearBuf)))
	if (get_global_time() < GetProtectTime(szWearBuf))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu islemi bu kadar hizli yapamassin!"));
		return false;
	}

	// @fixme210 BEGIN
	if (GetWear(WEAR_COSTUME_BODY))
	{
		if (item->GetVnum() >= 11901 && item->GetVnum() <= 11914)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Kostum varken bu esyalari giyemezsin"));
			return false;
		}
	}
	// @fixme210 END

	if (iWearCell == WEAR_BODY && IsRiding() && (item->GetVnum() >= 11901 && item->GetVnum() <= 11904))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»À» Åº »óÅÂ¿¡¼­ ¿¹º¹À» ÀÔÀ» ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (iWearCell != WEAR_ARROW && IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°© Áß¿¡´Â Âø¿ëÁßÀÎ Àåºñ¸¦ º¯°æÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (FN_check_item_sex(this, item) == false)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	/*DWORD dwCurTime = get_dword_time();
	if (iWearCell != WEAR_ARROW && (dwCurTime - GetLastAttackTime() <= 1500 || dwCurTime - m_dwLastSkillTime <= 1500))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°¡¸¸È÷ ÀÖÀ» ¶§¸¸ Âø¿ëÇÒ ¼ö ÀÖ½À´Ï´Ù."));
		return false;
	}*/

	#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (iWearCell == WEAR_WEAPON)
	{
		if (item->GetType() == ITEM_WEAPON)
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && costumeWeapon->GetValue(3) != item->GetSubType() && !UnequipItem(costumeWeapon))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot unequip the costume weapon. Not enough space."));
				return false;
			}
		}
		else //fishrod/pickaxe
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !UnequipItem(costumeWeapon))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot unequip the costume weapon. Not enough space."));
				return false;
			}
		}
	}
	else if (iWearCell == WEAR_COSTUME_WEAPON)
	{
		if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_WEAPON)
		{
			LPITEM pkWeapon = GetWear(WEAR_WEAPON);
			if (!pkWeapon || pkWeapon->GetType() != ITEM_WEAPON || item->GetValue(3) != pkWeapon->GetSubType())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot equip the costume weapon. Wrong equipped weapon."));
				return false;
			}
		}
	}
	#endif

	if (item->IsDragonSoul())
	{
		if (GetInventoryItem(INVENTORY_MAX_NUM + iWearCell))
		{
			ChatPacket(CHAT_TYPE_INFO, "ÀÌ¹Ì °°Àº Á¾·ùÀÇ ¿ëÈ¥¼®À» Âø¿ëÇÏ°í ÀÖ½À´Ï´Ù.");
			return false;
		}

		if (!item->EquipTo(this, iWearCell))
		{
			return false;
		}
	}
	else
	{
		if (GetWear(iWearCell) && !IS_SET(GetWear(iWearCell)->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		{
			if (item->GetWearFlag() == WEARABLE_ABILITY)
				return false;

			if (false == SwapItem(item->GetCell(), INVENTORY_MAX_NUM + iWearCell))
			{
				return false;
			}
		}
		else
		{
			BYTE bOldCell = item->GetCell();

			if (item->EquipTo(this, iWearCell))
			{
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, bOldCell, iWearCell);
			}
		}
	}

	if (true == item->IsEquipped())
	{
		if (-1 != item->GetProto()->cLimitRealTimeFirstUseIndex)
		{
			if (0 == item->GetSocket(1))
			{
				long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[(unsigned char)(item->GetProto()->cLimitRealTimeFirstUseIndex)].lValue;

				if (0 == duration)
					duration = 60 * 60 * 24 * 7;

				item->SetSocket(0, time(0) + duration);
				item->StartRealTimeExpireEvent();
			}

			item->SetSocket(1, item->GetSocket(1) + 1);
		}

		if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
			ShowAlignment(false);

		const DWORD& dwVnum = item->GetVnum();

		if (true == CItemVnumHelper::IsRamadanMoonRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_RAMADAN_RING);
		}
		else if (true == CItemVnumHelper::IsHalloweenCandy(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HALLOWEEN_CANDY);
		}
		else if (true == CItemVnumHelper::IsHappinessRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HAPPINESS_RING);
		}
		else if (true == CItemVnumHelper::IsLovePendant(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_LOVE_PENDANT);
		}
		else if (ITEM_UNIQUE == item->GetType() && 0 != item->GetSIGVnum())
		{
			const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(item->GetSIGVnum());
			if (NULL != pGroup)
			{
				const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(pGroup->GetAttrVnum(item->GetVnum()));
				if (NULL != pAttrGroup)
				{
					const std::string& std = pAttrGroup->m_stEffectFileName;
					SpecificEffectPacket(std.c_str());
				}
			}
		}
		#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		else if ((item->GetType() == ITEM_COSTUME) && (item->GetSubType() == COSTUME_ACCE))
			this->EffectPacket(SE_EFFECT_ACCE_EQUIP);
		#endif

		// if (item->IsNewMountItem()) // @fixme152
			// quest::CQuestManager::instance().SIGUse(GetPlayerID(), quest::QUEST_NO_NPC, item, false);
	}

	#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (item->IsCostumeMount())
	{
		#ifdef ENABLE_BLOCK_ITEMS_ON_WAR_MAP
		switch(GetMapIndex())
		{
			case 28:
			case 103:
			case 110:
			case 111:
			case 113:
				return false;
		}

		if (GetWarMap()) { return false; }
		#endif

		/*if (GetProtectTime("mountcostume") > get_global_time())
		{
			ChatPacket(CHAT_TYPE_INFO, "Yavaþ olmalýsýn! %d saniye sonra deneyebilirsiniz.", GetProtectTime("mountcostume") - get_global_time());
			return false;
		}*/

		if (IsRiding())
			StopRiding();

		if (GetHorse())
			HorseSummon(false);

		StartRiding();
		// SetProtectTime("mountcostume", get_global_time() + 2);
	}
	#endif

	time_t now = get_global_time();
	SetProtectTime(szWearBuf, (IsRiding() ? now+2 : now+1));

	return true;
}

void CHARACTER::BuffOnAttr_AddBuffsFromItem(LPITEM pItem)
{
	for (size_t i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->AddBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem)
{
	for (size_t i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->RemoveBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_ClearAll()
{
	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin(); it != m_map_buff_on_attrs.end(); it++)
	{
		CBuffOnAttributes* pBuff = it->second;
		if (pBuff)
		{
			pBuff->Initialize();
		}
	}
}

void CHARACTER::BuffOnAttr_ValueChange(BYTE bType, BYTE bOldValue, BYTE bNewValue)
{
	TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(bType);

	if (0 == bNewValue)
	{
		if (m_map_buff_on_attrs.end() == it)
			return;
		else
			it->second->Off();
	}
	else if (0 == bOldValue)
	{
		CBuffOnAttributes* pBuff = NULL;
		if (m_map_buff_on_attrs.end() == it)
		{
			switch (bType)
			{
			case POINT_ENERGY:
				{
					static BYTE abSlot[] = { WEAR_BODY, WEAR_HEAD, WEAR_FOOTS, WEAR_WRIST, WEAR_WEAPON, WEAR_NECK, WEAR_EAR, WEAR_SHIELD };
					static std::vector <BYTE> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			case POINT_COSTUME_ATTR_BONUS:
				{
					static BYTE abSlot[] = {
						WEAR_COSTUME_BODY,
						WEAR_COSTUME_HAIR,
						#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
						WEAR_COSTUME_MOUNT,
						#endif
						#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
						WEAR_COSTUME_WEAPON,
						#endif
					};
					static std::vector <BYTE> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			default:
				break;
			}
			m_map_buff_on_attrs.emplace(bType, pBuff);

		}
		else
			pBuff = it->second;
		if (pBuff != NULL)
			pBuff->On(bNewValue);
	}
	else
	{
		assert (m_map_buff_on_attrs.end() != it);
		it->second->ChangeBuffValue(bNewValue);
	}
}

LPITEM CHARACTER::FindSpecifyItem(DWORD vnum) const
{
	TItemTable * p = ITEM_MANAGER::instance().GetTable(vnum);
	if (!p)
		return NULL;

	#ifdef ENABLE_SPECIAL_STORAGE
	if (CItem::IsUpgradeItemVnum(vnum))
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if (GetUpgradeInventoryItem(i) && GetUpgradeInventoryItem(i)->GetVnum() == vnum)
				return GetUpgradeInventoryItem(i);
		}
	}
	else if (CItem::IsBookItemVnum(vnum))
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if (GetBookInventoryItem(i) && GetBookInventoryItem(i)->GetVnum() == vnum)
				return GetBookInventoryItem(i);
		}
	}
	else if ((p->bType == ITEM_METIN && p->bSubType == METIN_NORMAL) || CItem::IsStoneItemVnum(vnum))
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if (GetStoneInventoryItem(i) && GetStoneInventoryItem(i)->GetVnum() == vnum)
				return GetStoneInventoryItem(i);
		}
	}
	else if (p->bType == ITEM_GIFTBOX || CItem::IsChestItemVnum(vnum))
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if (GetChestInventoryItem(i) && GetChestInventoryItem(i)->GetVnum() == vnum)
				return GetChestInventoryItem(i);
		}
	}
	else
	#endif
	{
		for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			if (GetInventoryItem(i) && GetInventoryItem(i)->GetVnum() == vnum)
				return GetInventoryItem(i);
		}
	}

	return NULL;
}

#ifdef ENABLE_SWITCHBOT
bool CHARACTER::RemoveAttributeItem(BYTE bType) const
{
	DWORD itemVnum = 71084;

	if (bType == SWITCHBOT_ATTR_TYPE_GREEN)
	{
		DWORD itemVnum1 = 71151, itemVnum2 = 76023;
		for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			LPITEM pkItem = GetInventoryItem(i);
			if (pkItem && (pkItem->GetCount() >= SWITCHBOT_PRICE_AMOUNT))
			{
				DWORD dwVnum = pkItem->GetVnum();
				if ((dwVnum == itemVnum) || (dwVnum == itemVnum1) || (dwVnum == itemVnum2))
				{
					pkItem->SetCount(pkItem->GetCount() - SWITCHBOT_PRICE_AMOUNT);
					return true;
				}
			}
		}
	}
	else if (bType == SWITCHBOT_ATTR_TYPE_COSTUME)
	{
		itemVnum = 70064;
	}

	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		LPITEM pkItem = GetInventoryItem(i);
		if (pkItem && (pkItem->GetVnum() == itemVnum) && (pkItem->GetCount() >= SWITCHBOT_PRICE_AMOUNT))
		{
			pkItem->SetCount(pkItem->GetCount() - SWITCHBOT_PRICE_AMOUNT);
			return true;
		}
	}

	return false;
}
#endif

LPITEM CHARACTER::FindItemByID(DWORD id) const
{
	for (int i=0 ; i < INVENTORY_MAX_NUM ; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	for (int i=BELT_INVENTORY_SLOT_START; i < BELT_INVENTORY_SLOT_END ; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	return NULL;
}

int CHARACTER::CountSpecifyItem(DWORD vnum) const
{
	TItemTable * p = ITEM_MANAGER::instance().GetTable(vnum);
	if (!p)
		return 0;

	int count = 0;
	LPITEM item;

	#ifdef ENABLE_SPECIAL_STORAGE
	if (CItem::IsUpgradeItemVnum(vnum))
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			item = GetUpgradeInventoryItem(i);
			if (NULL != item && item->GetVnum() == vnum)
			{
				if (item->GetRefine()) { continue; }
				count += item->GetCount();
			}
		}
	}
	else if (CItem::IsBookItemVnum(vnum))
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			item = GetBookInventoryItem(i);
			if (NULL != item && item->GetVnum() == vnum)
			{
				if (item->GetRefine()) { continue; }
				count += item->GetCount();
			}
		}
	}
	else if ((p->bType == ITEM_METIN && p->bSubType == METIN_NORMAL) || CItem::IsStoneItemVnum(vnum)) 
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			item = GetStoneInventoryItem(i);
			if (NULL != item && item->GetVnum() == vnum)
			{
				if (item->GetRefine()) { continue; }
				count += item->GetCount();
			}
		}
	}
	else if (p->bType == ITEM_GIFTBOX || CItem::IsChestItemVnum(vnum))
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			item = GetChestInventoryItem(i);
			if (NULL != item && item->GetVnum() == vnum)
			{
				if (item->GetRefine()) { continue; }
				count += item->GetCount();
			}
		}
	}
	else
	#endif
	{
		for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			item = GetInventoryItem(i);
			if (NULL != item && item->GetVnum() == vnum)
			{
				if (item->GetRefine()) { continue; }
				count += item->GetCount();
			}
		}
	}

	return count;
}

void CHARACTER::RemoveSpecifyItem(DWORD vnum, DWORD count)
{
	if (0 == count)
		return;

	TItemTable * p = ITEM_MANAGER::instance().GetTable(vnum);
	if (!p)
		return;

	#ifdef ENABLE_SPECIAL_STORAGE
	if (CItem::IsUpgradeItemVnum(vnum))
	{
		for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if (NULL == GetUpgradeInventoryItem(i))
				continue;

			if (GetUpgradeInventoryItem(i)->GetVnum() != vnum)
				continue;

			if (GetUpgradeInventoryItem(i)->GetRefine()) { continue; }

			if (count >= GetUpgradeInventoryItem(i)->GetCount())
			{
				count -= GetUpgradeInventoryItem(i)->GetCount();
				GetUpgradeInventoryItem(i)->SetCount(0);

				if (0 == count)
					return;
			}
			else
			{
				GetUpgradeInventoryItem(i)->SetCount(GetUpgradeInventoryItem(i)->GetCount() - count);
				return;
			}
		}
	}
	else if (CItem::IsBookItemVnum(vnum))
	{
		for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if (NULL == GetBookInventoryItem(i))
				continue;

			if (GetBookInventoryItem(i)->GetVnum() != vnum)
				continue;

			if (GetBookInventoryItem(i)->GetRefine()) { continue; }

			if (count >= GetBookInventoryItem(i)->GetCount())
			{
				count -= GetBookInventoryItem(i)->GetCount();
				GetBookInventoryItem(i)->SetCount(0);

				if (0 == count)
					return;
			}
			else
			{
				GetBookInventoryItem(i)->SetCount(GetBookInventoryItem(i)->GetCount() - count);
				return;
			}
		}
	}
	else if ((p->bType == ITEM_METIN && p->bSubType == METIN_NORMAL) || CItem::IsStoneItemVnum(vnum))
	{
		for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if (NULL == GetStoneInventoryItem(i))
				continue;

			if (GetStoneInventoryItem(i)->GetVnum() != vnum)
				continue;

			if (GetStoneInventoryItem(i)->GetRefine()) { continue; }

			if (count >= GetStoneInventoryItem(i)->GetCount())
			{
				count -= GetStoneInventoryItem(i)->GetCount();
				GetStoneInventoryItem(i)->SetCount(0);

				if (0 == count)
					return;
			}
			else
			{
				GetStoneInventoryItem(i)->SetCount(GetStoneInventoryItem(i)->GetCount() - count);
				return;
			}
		}
	}
	else if (p->bType == ITEM_GIFTBOX || CItem::IsChestItemVnum(vnum))
	{
		for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if (NULL == GetChestInventoryItem(i))
				continue;

			if (GetChestInventoryItem(i)->GetVnum() != vnum)
				continue;

			if (GetChestInventoryItem(i)->GetRefine()) { continue; }

			if (count >= GetChestInventoryItem(i)->GetCount())
			{
				count -= GetChestInventoryItem(i)->GetCount();
				GetChestInventoryItem(i)->SetCount(0);

				if (0 == count)
					return;
			}
			else
			{
				GetChestInventoryItem(i)->SetCount(GetChestInventoryItem(i)->GetCount() - count);
				return;
			}
		}
	}
	else
	#endif
	{
		for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			if (NULL == GetInventoryItem(i))
				continue;

			if (GetInventoryItem(i)->GetVnum() != vnum)
				continue;

			if (GetInventoryItem(i)->GetRefine()) { continue; }

			if (m_pkMyShop)
			{
				bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
				if (isItemSelling)
					continue;
			}

			if (vnum >= 80003 && vnum <= 80007)
				LogManager::instance().GoldBarLog(GetPlayerID(), GetInventoryItem(i)->GetID(), QUEST, "RemoveSpecifyItem");

			if (count >= GetInventoryItem(i)->GetCount())
			{
				count -= GetInventoryItem(i)->GetCount();
				GetInventoryItem(i)->SetCount(0);

				if (0 == count)
					return;
			}
			else
			{
				GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
				return;
			}
		}
	}

	if (count)
		sys_log(0, "CHARACTER::RemoveSpecifyItem cannot remove enough item vnum %u, still remain %d", vnum, count);
}

int CHARACTER::CountSpecifyTypeItem(BYTE type) const
{
	int count = 0;
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		LPITEM pItem = GetInventoryItem(i);
		if (pItem != NULL && pItem->GetType() == type)
		{
			if (pItem->GetRefine()) { continue; }
			count += pItem->GetCount();
		}
	}

	return count;
}

void CHARACTER::RemoveSpecifyTypeItem(BYTE type, DWORD count)
{
	if (0 == count)
		return;

	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetType() != type)
			continue;

		if (GetInventoryItem(i)->GetRefine()) { continue; }

		if (m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}
}

void CHARACTER::AutoGiveItem(LPITEM item, bool longOwnerShip)
{
	if (NULL == item)
	{
		sys_err ("NULL point.");
		return;
	}

	if (item->GetOwner())
	{
		sys_err ("item %d 's owner exists!",item->GetID());
		return;
	}

	int cell = GetEmptyAllInventory(item);
	if (cell != -1)
	{
		item->AddToItem(this, cell);

		LogManager::instance().ItemLog(this, item, "SYSTEM", item->GetName());

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = cell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround (GetMapIndex(), GetXYZ());
		#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
		#else
		item->StartDestroyEvent();
		#endif

		if (longOwnerShip)
			item->SetOwnership (this, 300);
		else
			item->SetOwnership (this, 60);

		LogManager::instance().ItemLog(this, item, "SYSTEM_DROP", item->GetName());
	}
}

LPITEM CHARACTER::AutoGiveItem(
	DWORD dwItemVnum
	#ifdef ENABLE_NEW_MAX_COUNT
	, short bCount
	#else
	, BYTE bCount
	#endif
	, int iRarePct
	, bool bMsg
	#ifdef ENABLE_LOCKED_SYSTEM
	, bool bIsLocked
	#endif
)
{
	TItemTable * p = ITEM_MANAGER::instance().GetTable(dwItemVnum);
	if (!p) { return NULL; }

	DBManager::instance().SendMoneyLog(MONEY_LOG_DROP, dwItemVnum, bCount);

	if (p->dwFlags & ITEM_FLAG_STACKABLE && p->bType != ITEM_BLEND)
	{
		#ifdef ENABLE_SPECIAL_STORAGE
		if (CItem::IsUpgradeItemVnum(dwItemVnum)) //upgrade item
		{
			for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item = GetUpgradeInventoryItem(i);
				if (!item) { continue; }
				#ifdef ENABLE_LOCKED_SYSTEM
				if (item->IsItemLocked() && !bIsLocked) { continue; }
				if (!item->IsItemLocked() && bIsLocked) { continue; }
				#endif

				if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
				{
					if (item->GetCount() == g_bItemCountLimit)//Fix
						continue;

					if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
					{
						if (bCount < p->alValues[1])
							bCount = p->alValues[1];
					}

					auto bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					bCount -= bCount2;

					if (bCount2 > 0)
						item->SetCount(item->GetCount() + bCount2);

					if (bCount == 0)
					{
						if (bMsg) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("arti_env_%s"), item->GetName()); }
						return item;
					}
				}
			}
		}
		else if (CItem::IsBookItemVnum(dwItemVnum))
		{
			long lSkillVnum = 0;
			if (dwItemVnum == 50300)
			{
				extern const DWORD GetRandomSkillVnum(BYTE bJob = JOB_MAX_NUM);
				lSkillVnum = GetRandomSkillVnum();
			}

			for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item = GetBookInventoryItem(i);
				if (!item) { continue; }
				#ifdef ENABLE_LOCKED_SYSTEM
				if (item->IsItemLocked() && !bIsLocked) { continue; }
				if (!item->IsItemLocked() && bIsLocked) { continue; }
				#endif

				if (item->GetVnum() == dwItemVnum && item->GetSocket(0) == lSkillVnum)
				{
					if (item->GetCount() == g_bItemCountLimit)//Fix
						continue;

					if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
					{
						if (bCount < p->alValues[1])
							bCount = p->alValues[1];
					}

					auto bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					bCount -= bCount2;

					if (bCount2 > 0)
						item->SetCount(item->GetCount() + bCount2);

					if (bCount == 0)
					{
						if (bMsg) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bk_env_%s"), item->GetName()); }
						return item;
					}
				}
			}
		}
		else if ((p->bType == ITEM_METIN && p->bSubType == METIN_NORMAL) || CItem::IsStoneItemVnum(dwItemVnum)) //stone item
		{
			for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item = GetStoneInventoryItem(i);
				if (!item) { continue; }
				#ifdef ENABLE_LOCKED_SYSTEM
				if (item->IsItemLocked() && !bIsLocked) { continue; }
				if (!item->IsItemLocked() && bIsLocked) { continue; }
				#endif

				if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
				{
					if (item->GetCount() == g_bItemCountLimit)//Fix
						continue;

					if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
					{
						if (bCount < p->alValues[1])
							bCount = p->alValues[1];
					}

					auto bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					bCount -= bCount2;

					if (bCount2 > 0)
						item->SetCount(item->GetCount() + bCount2);

					if (bCount == 0)
					{
						if (bMsg) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("tas_env_%s"), item->GetName()); }
						return item;
					}
				}
			}
		}
		else if (p->bType == ITEM_GIFTBOX || CItem::IsChestItemVnum(dwItemVnum)) //chest item
		{
			for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item = GetChestInventoryItem(i);
				if (!item) { continue; }
				#ifdef ENABLE_LOCKED_SYSTEM
				if (item->IsItemLocked() && !bIsLocked) { continue; }
				if (!item->IsItemLocked() && bIsLocked) { continue; }
				#endif

				if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
				{
					if (item->GetCount() == g_bItemCountLimit)//Fix
						continue;

					if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
					{
						if (bCount < p->alValues[1])
							bCount = p->alValues[1];
					}

					auto bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					bCount -= bCount2;

					if (bCount2 > 0)
						item->SetCount(item->GetCount() + bCount2);

					if (bCount == 0)
					{
						if (bMsg) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("sandik_env_%s"), item->GetName()); }
						return item;
					}
				}
			}
		}
		else
		#endif
		{
			for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item = GetInventoryItem(i);
				if (!item) { continue; }
				#ifdef ENABLE_LOCKED_SYSTEM
				if (item->IsItemLocked() && !bIsLocked) { continue; }
				if (!item->IsItemLocked() && bIsLocked) { continue; }
				#endif

				if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
				{
					if (item->GetCount() == g_bItemCountLimit)//Fix
						continue;

					if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
					{
						if (bCount < p->alValues[1])
							bCount = p->alValues[1];
					}

					auto bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					bCount -= bCount2;

					if (bCount2 > 0)
						item->SetCount(item->GetCount() + bCount2);

					if (bCount == 0)
					{
						if (bMsg) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("normal_env_%s"), item->GetName()); }
						return item;
					}
				}
			}
		}
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwItemVnum, bCount, 0, true);
	if (!item)
	{
		sys_err("cannot create item by vnum %u (name: %s)", dwItemVnum, GetName());
		return NULL;
	}

	if (item->GetType() == ITEM_BLEND)
	{
		for (int i=0; i < INVENTORY_MAX_NUM; i++)
		{
			LPITEM inv_item = GetInventoryItem(i);
			if (inv_item == NULL) continue;
			if (inv_item->GetType() == ITEM_BLEND)
			{
				if (inv_item->GetVnum() == item->GetVnum())
				{
					if (inv_item->GetSocket(0) == item->GetSocket(0) &&
							inv_item->GetSocket(1) == item->GetSocket(1) &&
							inv_item->GetSocket(2) == item->GetSocket(2) &&
							inv_item->GetCount() < g_bItemCountLimit)
					{
						inv_item->SetCount(inv_item->GetCount() + item->GetCount());
						M2_DESTROY_ITEM(item);	//@fixme472
						return inv_item;
					}
				}
			}
		}
	}

	int iEmptyCell = GetEmptyAllInventory(item);
	if (iEmptyCell != -1)
	{
		if (bMsg)
		{
			#ifdef ENABLE_SPECIAL_STORAGE
			if (item->IsUpgradeItem())
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("arti_env_%s"), item->GetName());
			else if (item->IsBook())
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bk_env_%s"), item->GetName());
			else if (item->IsStone())
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("tas_env_%s"), item->GetName());
			else if (item->IsChest())
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("sandik_env_%s"), item->GetName());
			else
			#endif
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("normal_env_%s"), item->GetName());
		}

		item->AddToItem(this, iEmptyCell);

		LogManager::instance().ItemLog(this, item, "SYSTEM", item->GetName());

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = iEmptyCell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround(GetMapIndex(), GetXYZ());

		#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
		#else
		item->StartDestroyEvent();
		#endif

		if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP))
			item->SetOwnership(this, 300);
		else
			item->SetOwnership(this, 60);

		LogManager::instance().ItemLog(this, item, "SYSTEM_DROP", item->GetName());
	}

	sys_log(0, "7: %d %d", dwItemVnum, bCount);
	return item;
}

bool CHARACTER::GiveItem(LPCHARACTER victim, TItemPos Cell)
{
	if (!CanHandleItem())
		return false;

	// @fixme150 BEGIN
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot take this item if you're using quests"));
		return false;
	}
	// @fixme150 END

	LPITEM item = GetItem(Cell);

	if (item && !item->IsExchanging())
	{
		if (victim->CanReceiveItem(this, item))
		{
			victim->ReceiveItem(this, item);
			return true;
		}
	}

	return false;
}

bool CHARACTER::CanReceiveItem(LPCHARACTER from, LPITEM item) const
{
	if (IsPC())
		return false;

	// TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX
	if (DISTANCE_APPROX(GetX() - from->GetX(), GetY() - from->GetY()) > 2000)
		return false;
	// END_OF_TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX

	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH &&
					(item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				return true;
			break;

		case fishing::FISHER_MOB:
			if (item->GetType() == ITEM_ROD)
				return true;
			break;

			// BUILDING_NPC
		case BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
			if (item->GetType() == ITEM_WEAPON &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
			if (item->GetType() == ITEM_ARMOR &&
					(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ACCESSORY_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
			if (item->GetType() == ITEM_ARMOR &&
					!(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;
			// END_OF_BUILDING_NPC

		case BLACKSMITH_MOB:
			if (item->GetRefinedVnum() && item->GetRefineSet() < 500)
			{
				return true;
			}
			else
			{
				return false;
			}

		case BLACKSMITH2_MOB:
			if (item->GetRefineSet() >= 500)
			{
				return true;
			}
			else
			{
				return false;
			}

		case ALCHEMIST_MOB:
			if (item->GetRefinedVnum())
				return true;
			break;

		case 20101:
		case 20102:
		case 20103:

			if (item->GetVnum() == ITEM_REVIVE_HORSE_1)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á×Áö ¾ÊÀº ¸»¿¡°Ô ¼±ÃÊ¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á×Àº ¸»¿¡°Ô »ç·á¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20104:
		case 20105:
		case 20106:

			if (item->GetVnum() == ITEM_REVIVE_HORSE_2)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á×Áö ¾ÊÀº ¸»¿¡°Ô ¼±ÃÊ¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á×Àº ¸»¿¡°Ô »ç·á¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20107:
		case 20108:
		case 20109:

			if (item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á×Áö ¾ÊÀº ¸»¿¡°Ô ¼±ÃÊ¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á×Àº ¸»¿¡°Ô »ç·á¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				return false;
			}
			break;
	}

	//if (IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_GIVE))
	{
		return true;
	}

	return false;
}

void CHARACTER::ReceiveItem(LPCHARACTER from, LPITEM item)
{
	if (IsPC())
		return;

	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH && (item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				fishing::Grill(from, item);
			else
			{
				// TAKE_ITEM_BUG_FIX
				from->SetQuestNPCID(GetVID());
				// END_OF_TAKE_ITEM_BUG_FIX
				quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			}
			break;

			// DEVILTOWER_NPC
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
			if (item->GetRefinedVnum() != 0 && item->GetRefineSet() != 0 && item->GetRefineSet() < 500)
			{
				from->SetRefineNPC(this);
				from->RefineInformation(item->GetCell(), REFINE_TYPE_MONEY_ONLY);
			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			}
			break;
			// END_OF_DEVILTOWER_NPC

		case BLACKSMITH_MOB:
		case BLACKSMITH2_MOB:
		case BLACKSMITH_WEAPON_MOB:
		case BLACKSMITH_ARMOR_MOB:
		case BLACKSMITH_ACCESSORY_MOB:
			if (item->GetRefinedVnum())
			{
				from->SetRefineNPC(this);
				from->RefineInformation(item->GetCell(), REFINE_TYPE_NORMAL);
			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			}
			break;

		case 20101:
		case 20102:
		case 20103:
		case 20104:
		case 20105:
		case 20106:
		case 20107:
		case 20108:
		case 20109:
			if (item->GetVnum() == ITEM_REVIVE_HORSE_1 ||
					item->GetVnum() == ITEM_REVIVE_HORSE_2 ||
					item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				from->ReviveHorse();
				item->SetCount(item->GetCount()-1);
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»¿¡°Ô ¼±ÃÊ¸¦ ÁÖ¾ú½À´Ï´Ù."));
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 ||
					item->GetVnum() == ITEM_HORSE_FOOD_2 ||
					item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				from->FeedHorse();
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»¿¡°Ô »ç·á¸¦ ÁÖ¾ú½À´Ï´Ù."));
				item->SetCount(item->GetCount()-1);
				EffectPacket(SE_HPUP_RED);
			}
			break;

		default:
			sys_log(0, "TakeItem %s %d %s", from->GetName(), GetRaceNum(), item->GetName());
			from->SetQuestNPCID(GetVID());
			quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			break;
	}
}

bool CHARACTER::IsEquipUniqueItem(DWORD dwItemVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	if (dwItemVnum == UNIQUE_ITEM_RING_OF_LANGUAGE)
		return IsEquipUniqueItem(UNIQUE_ITEM_RING_OF_LANGUAGE_SAMPLE);

	return false;
}

// CHECK_UNIQUE_GROUP
bool CHARACTER::IsEquipUniqueGroup(DWORD dwGroupVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetSpecialGroup() == (int) dwGroupVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetSpecialGroup() == (int) dwGroupVnum)
			return true;
	}

	return false;
}
// END_OF_CHECK_UNIQUE_GROUP

void CHARACTER::SetRefineMode(int iAdditionalCell)
{
	m_iRefineAdditionalCell = iAdditionalCell;
	SetUnderRefine(true);
}

void CHARACTER::ClearRefineMode()
{
	SetUnderRefine(false);
	SetRefineNPC(NULL);
	//PREVENT_REFINE_WINDOW
	SetOpenRefine(false);
	//END_PREVENT_REFINE_WINDOW
}

bool CHARACTER::GiveItemFromSpecialItemGroup(
	DWORD dwGroupNum
	, std::vector<DWORD> &dwItemVnums
	, std::vector<DWORD> &dwItemCounts
	, std::vector <LPITEM> &item_gets
	, int &count
	#ifdef ENABLE_LOCKED_SYSTEM
	, BYTE bItemLocked
	#endif
)
{
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(dwGroupNum);
	if (!pGroup)
	{
		sys_err("cannot find special item group %d", dwGroupNum);
		return false;
	}

	std::vector <int> idxes;
	int n = pGroup->GetMultiIndex(idxes);

	bool bSuccess;

	for (int i = 0; i < n; i++)
	{
		bSuccess = false;
		int idx = idxes[i];
		DWORD dwVnum = pGroup->GetVnum(idx);
		DWORD dwCount = pGroup->GetCount(idx);
		int iRarePct = pGroup->GetRarePct(idx);
		LPITEM item_get = NULL;
		switch (dwVnum)
		{
			case CSpecialItemGroup::GOLD:
				#ifdef ENABLE_FULL_YANG
				GoldChange(dwCount);
				#else
				PointChange(POINT_GOLD, dwCount);
				#endif
				LogManager::instance().CharLog(this, dwCount, "TREASURE_GOLD", "");

				bSuccess = true;
				break;

			case CSpecialItemGroup::EXP:
				{
					PointChange(POINT_EXP, dwCount);
					LogManager::instance().CharLog(this, dwCount, "TREASURE_EXP", "");

					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::MOB:
				{
					sys_log(0, "CSpecialItemGroup::MOB %d", dwCount);
					int x = GetX() + number(-500, 500);
					int y = GetY() + number(-500, 500);

					LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(dwCount, GetMapIndex(), x, y, 0, true, -1);
					if (ch)
						ch->SetAggressive();
					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::SLOW:
				{
					sys_log(0, "CSpecialItemGroup::SLOW %d", -(int)dwCount);
					AddAffect(AFFECT_SLOW, POINT_MOV_SPEED, -(int)dwCount, AFF_SLOW, 300, 0, true);
					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::DRAIN_HP:
				{
					int iDropHP = GetMaxHP()*dwCount/100;
					sys_log(0, "CSpecialItemGroup::DRAIN_HP %d", -iDropHP);
					iDropHP = MIN(iDropHP, GetHP()-1);
					sys_log(0, "CSpecialItemGroup::DRAIN_HP %d", -iDropHP);
					PointChange(POINT_HP, -iDropHP);
					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::POISON:
				{
					AttackedByPoison(NULL);
					bSuccess = true;
				}
				break;

			#ifdef ENABLE_WOLFMAN_CHARACTER
			case CSpecialItemGroup::BLEEDING:
				{
					AttackedByBleeding(NULL);
					bSuccess = true;
				}
				break;
			#endif

			case CSpecialItemGroup::MOB_GROUP:
				{
					int sx = GetX() - number(300, 500);
					int sy = GetY() - number(300, 500);
					int ex = GetX() + number(300, 500);
					int ey = GetY() + number(300, 500);
					CHARACTER_MANAGER::instance().SpawnGroup(dwCount, GetMapIndex(), sx, sy, ex, ey, NULL, true);

					bSuccess = true;
				}
				break;

			default:
				{
					item_get = AutoGiveItem(dwVnum, dwCount, iRarePct, true
								#ifdef ENABLE_LOCKED_SYSTEM
								, (bItemLocked > 0 ? true : false)
								#endif
					);
					if (item_get)
					{
						bSuccess = true;
						#ifdef ENABLE_LOCKED_SYSTEM
						item_get->SetLocked(bItemLocked);
						#endif
					}
				}
				break;
		}

		if (bSuccess)
		{
			dwItemVnums.emplace_back(dwVnum);
			dwItemCounts.emplace_back(dwCount);
			item_gets.emplace_back(item_get);
			count++;
		}
		else
		{
			return false;
		}
	}

	return bSuccess;
}

// NEW_HAIR_STYLE_ADD
bool CHARACTER::ItemProcess_Hair(LPITEM item, int iDestCell)
{
	if (item->CheckItemUseLevel(GetLevel()) == false)
	{

		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ ÀÌ ¸Ó¸®¸¦ »ç¿ëÇÒ ¼ö ¾ø´Â ·¹º§ÀÔ´Ï´Ù."));
		return false;
	}

	DWORD hair = item->GetVnum();

	switch (GetJob())
	{
		case JOB_WARRIOR :
			hair -= 72000;
			break;

		case JOB_ASSASSIN :
			hair -= 71250;
			break;

		case JOB_SURA :
			hair -= 70500;
			break;

		case JOB_SHAMAN :
			hair -= 69750;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			break;
#endif
		default :
			return false;
			break;
	}

	if (hair == GetPart(PART_HAIR))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ¿ÀÏÇÑ ¸Ó¸® ½ºÅ¸ÀÏ·Î´Â ±³Ã¼ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return true;
	}

	item->SetCount(item->GetCount() - 1);

	SetPart(PART_HAIR, hair);
	UpdatePacket();

	return true;
}
// END_NEW_HAIR_STYLE_ADD

bool CHARACTER::ItemProcess_Polymorph(LPITEM item)
{
	if (IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì µÐ°©ÁßÀÎ »óÅÂÀÔ´Ï´Ù."));
		return false;
	}

	#ifdef ENABLE_VSTURNUVA
	if (GetMapIndex() == 28)
	{
		ChatPacket(CHAT_TYPE_INFO, "Burada donusmezsin");
		return false;
	}
	#endif

	if (true == IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°©ÇÒ ¼ö ¾ø´Â »óÅÂÀÔ´Ï´Ù."));
		return false;
	}

	DWORD dwVnum = item->GetSocket(0);

	if (dwVnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àß¸øµÈ µÐ°© ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		item->SetCount(item->GetCount()-1);
		return false;
	}

	const CMob* pMob = CMobManager::instance().Get(dwVnum);

	if (pMob == NULL)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àß¸øµÈ µÐ°© ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		item->SetCount(item->GetCount()-1);
		return false;
	}

	switch (item->GetVnum())
	{
		case 70104 :
		case 70105 :
		case 70106 :
		case 70107 :
		case 71093 :
			{

				sys_log(0, "USE_POLYMORPH_BALL PID(%d) vnum(%d)", GetPlayerID(), dwVnum);


				int iPolymorphLevelLimit = MAX(0, 20 - GetLevel() * 3 / 10);
				if (pMob->m_table.bLevel >= GetLevel() + iPolymorphLevelLimit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("³ªº¸´Ù ³Ê¹« ³ôÀº ·¹º§ÀÇ ¸ó½ºÅÍ·Î´Â º¯½Å ÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}

				int iDuration = GetSkillLevel(POLYMORPH_SKILL_ID) == 0 ? 5 : (5 + (5 + GetSkillLevel(POLYMORPH_SKILL_ID)/40 * 25));
				iDuration *= 60;

				DWORD dwBonus = 0;

				dwBonus = (2 + GetSkillLevel(POLYMORPH_SKILL_ID)/40) * 100;

				AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, dwVnum, AFF_POLYMORPH, iDuration, 0, true);
				AddAffect(AFFECT_POLYMORPH, POINT_ATT_BONUS, dwBonus, AFF_POLYMORPH, iDuration, 0, false);

				item->SetCount(item->GetCount()-1);
			}
			break;

		case 50322:
			{
				sys_log(0, "USE_POLYMORPH_BOOK: %s(%u) vnum(%u)", GetName(), GetPlayerID(), dwVnum);

				if (CPolymorphUtils::instance().PolymorphCharacter(this, item, pMob) == true)
				{
					CPolymorphUtils::instance().UpdateBookPracticeGrade(this, item);
				}
				else
				{
				}
			}
			break;

		default :
			sys_err("POLYMORPH invalid item passed PID(%d) vnum(%d)", GetPlayerID(), item->GetOriginalVnum());
			return false;
	}

	return true;
}

bool CHARACTER::CanDoCube() const
{
	if (m_bIsObserver)	return false;
	if (GetShop())		return false;
	if (GetMyShop())	return false;
	if (m_bUnderRefine)	return false;
	if (IsWarping())	return false;

	return true;
}

bool CHARACTER::UnEquipSpecialRideUniqueItem()
{
	#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	LPITEM MountCostume = GetWear(WEAR_COSTUME_MOUNT);
	if (MountCostume)
		return UnequipItem(MountCostume);
	#endif

	return true;
}

void CHARACTER::AutoRecoveryItemProcess(const EAffectTypes type)
{
	if (true == IsDead() || true == IsStun())
		return;

	if (false == IsPC())
		return;

	if (AFFECT_AUTO_HP_RECOVERY != type && AFFECT_AUTO_SP_RECOVERY != type)
		return;

	if (NULL != FindAffect(AFFECT_STUN))
		return;

	{
		const DWORD stunSkills[] = { SKILL_TANHWAN, SKILL_GEOMPUNG, SKILL_BYEURAK, SKILL_GIGUNG };

		for (size_t i=0 ; i < sizeof(stunSkills)/sizeof(DWORD) ; ++i)
		{
			const CAffect* p = FindAffect(stunSkills[i]);

			if (NULL != p && AFF_STUN == p->dwFlag)
				return;
		}
	}

	const CAffect* pAffect = FindAffect(type);
	const size_t idx_of_amount_of_used = 1;
	const size_t idx_of_amount_of_full = 2;

	if (NULL != pAffect)
	{
		LPITEM pItem = FindItemByID(pAffect->dwFlag);

		if (NULL != pItem && true == pItem->GetSocket(0))
		{
			if (!CArenaManager::instance().IsArenaMap(GetMapIndex())
#ifdef ENABLE_NEWSTUFF
				&& !(g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(pItem->GetVnum()))
#endif
			)
			{
				const long amount_of_used = pItem->GetSocket(idx_of_amount_of_used);
				const long amount_of_full = pItem->GetSocket(idx_of_amount_of_full);

				const int32_t avail = amount_of_full - amount_of_used;

				int32_t amount = 0;

				if (AFFECT_AUTO_HP_RECOVERY == type)
				{
					amount = GetMaxHP() - (GetHP() + GetPoint(POINT_HP_RECOVERY));
				}
				else if (AFFECT_AUTO_SP_RECOVERY == type)
				{
					amount = GetMaxSP() - (GetSP() + GetPoint(POINT_SP_RECOVERY));
				}

				if (amount > 0)
				{
					if (avail > amount)
					{
						const int pct_of_used = amount_of_used * 100 / amount_of_full;
						const int pct_of_will_used = (amount_of_used + amount) * 100 / amount_of_full;

						bool bLog = false;


						if ((pct_of_will_used / 10) - (pct_of_used / 10) >= 1)
							bLog = true;
						pItem->SetSocket(idx_of_amount_of_used, amount_of_used + amount, bLog);
					}
					else
					{
						amount = avail;

						ITEM_MANAGER::instance().RemoveItem( pItem );
					}

					if (AFFECT_AUTO_HP_RECOVERY == type)
					{
						PointChange( POINT_HP_RECOVERY, amount );
						EffectPacket( SE_AUTO_HPUP );
					}
					else if (AFFECT_AUTO_SP_RECOVERY == type)
					{
						PointChange( POINT_SP_RECOVERY, amount );
						EffectPacket( SE_AUTO_SPUP );
					}
				}
			}
			else
			{
				pItem->Lock(false);
				pItem->SetSocket(0, false);
				RemoveAffect( const_cast<CAffect*>(pAffect) );
			}
		}
		else
		{
			RemoveAffect( const_cast<CAffect*>(pAffect) );
		}
	}
}

bool CHARACTER::IsValidItemPosition(TItemPos Pos) const
{
	BYTE window_type = Pos.window_type;
	WORD cell = Pos.cell;

	switch (window_type)
	{
	case RESERVED_WINDOW:
		return false;

	case INVENTORY:
	case EQUIPMENT:
		return cell < (INVENTORY_AND_EQUIP_SLOT_MAX);

	case DRAGON_SOUL_INVENTORY:
		return cell < (DRAGON_SOUL_INVENTORY_MAX_NUM);

	#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
	case BOOK_INVENTORY:
	case STONE_INVENTORY:
	case CHEST_INVENTORY:
		return cell < (SPECIAL_INVENTORY_MAX_NUM);
	#endif

	case SAFEBOX:
		if (NULL != m_pkSafebox)
			return m_pkSafebox->IsValidPosition(cell);
		else
			return false;

	case MALL:
		if (NULL != m_pkMall)
			return m_pkMall->IsValidPosition(cell);
		else
			return false;

	#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		return cell < SWITCHBOT_SLOT_COUNT;
	#endif

	default:
		return false;
	}
}

#define VERIFY_MSG(exp, msg)  \
	if (true == (exp)) { \
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(msg)); \
			return false; \
	}

bool CHARACTER::CanEquipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{
	const TItemTable* itemTable = item->GetProto();
	//BYTE itemType = item->GetType();
	//BYTE itemSubType = item->GetSubType();

	switch (GetJob())
	{
		case JOB_WARRIOR:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SURA)
				return false;
			break;

		#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN)
				return false;
			break;
		#endif
	}

	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limit = itemTable->aLimits[i].lValue;
		switch (itemTable->aLimits[i].bType)
		{
			case LIMIT_LEVEL:
				if (GetLevel() < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("·¹º§ÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_STR:
				if (GetPoint(POINT_ST) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±Ù·ÂÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_INT:
				if (GetPoint(POINT_IQ) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Áö´ÉÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_DEX:
				if (GetPoint(POINT_DX) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹ÎÃ¸ÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_CON:
				if (GetPoint(POINT_HT) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ã¼·ÂÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;
		}
	}

	if (item->GetWearFlag() & WEARABLE_UNIQUE)
	{
		if ((GetWear(WEAR_UNIQUE1) && GetWear(WEAR_UNIQUE1)->IsSameSpecialGroup(item)) ||
			(GetWear(WEAR_UNIQUE2) && GetWear(WEAR_UNIQUE2)->IsSameSpecialGroup(item)))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°°Àº Á¾·ùÀÇ À¯´ÏÅ© ¾ÆÀÌÅÛ µÎ °³¸¦ µ¿½Ã¿¡ ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		if (marriage::CManager::instance().IsMarriageUniqueItem(item->GetVnum()) &&
			!marriage::CManager::instance().IsMarried(GetPlayerID()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°áÈ¥ÇÏÁö ¾ÊÀº »óÅÂ¿¡¼­ ¿¹¹°À» Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	#ifdef ENABLE_NEW_RING
	if (item->GetType() == ITEM_RING)
	{
		LPITEM ringItems[6] = { GetWear(WEAR_RING1), GetWear(WEAR_RING2), GetWear(WEAR_RING3), GetWear(WEAR_RING4), GetWear(WEAR_RING5), GetWear(WEAR_RING6) };
		for (int i = 0; i < 6; i++)
		{
			if (ringItems[i])
			{
				if (ringItems[i]->GetVnum() == item->GetVnum())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°°Àº Á¾·ùÀÇ À¯´ÏÅ© ¾ÆÀÌÅÛ µÎ °³¸¦ µ¿½Ã¿¡ ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
			}
		}
	}
	#endif

	if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_BODY)
	{
		LPITEM arge = GetWear(WEAR_BODY);
		if (arge && (arge->GetVnum() >= 11901 && arge->GetVnum() <= 11914))
		{
			ChatPacket(CHAT_TYPE_INFO, "Uzerinde gelinlik veya smokin varken kostum giyemezsin.");
			return false;
		}
	}

	if (item->GetVnum() >= 11901 && item->GetVnum() <= 11914)
	{
		LPITEM arge1 = GetWear(WEAR_COSTUME_BODY);
		if (arge1 && (arge1->GetType() == ITEM_COSTUME && arge1->GetSubType() == COSTUME_BODY))
		{
			ChatPacket(CHAT_TYPE_INFO, "Uzerinde kostum varken smokin veya gelinlik giyemezsin.");
			return false;
		}
	}

	// @fixme174 BEGIN
	if (item->GetType() == ITEM_UNIQUE)
	{
		LPITEM UniqueItems[2] = { GetWear(WEAR_UNIQUE1), GetWear(WEAR_UNIQUE2) };
		for (int i = 0; i < 2; i++)
		{
			if (UniqueItems[i])
			{
				if (UniqueItems[i]->GetVnum() == item->GetVnum())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("iki kez bu objeyi takamazsin!"));
					return false;
				}
			}
		}
	}
	// @fixme174 END

	#ifdef ENABLE_VSTURNUVA
	if (GetMapIndex() == 28)
	{
		if (int(GetQuestFlag("pvp.item_block")) == 1)
			return false;

		if (GetJob() == JOB_ASSASSIN)
		{
			if (item->GetType() == ITEM_WEAPON && item->GetSubType() == WEAPON_SWORD)
				return false;
		}
	}
	#endif

	return true;
}

bool CHARACTER::CanUnequipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{
	if (ITEM_BELT == item->GetType())
		VERIFY_MSG(CBeltInventoryHelper::IsExistItemInBeltInventory(this), "º§Æ® ÀÎº¥Åä¸®¿¡ ¾ÆÀÌÅÛÀÌ Á¸ÀçÇÏ¸é ÇØÁ¦ÇÒ ¼ö ¾ø½À´Ï´Ù.");

	if (IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

	/*#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (item->IsCostumeMount())
	{
		if (IsRiding())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("BINEK_HATA"));
			return false;
		}
	}
	#endif*/

	#ifdef ENABLE_VSTURNUVA
	if (GetMapIndex() == 28)
	{
		if (int(GetQuestFlag("pvp.item_block")) == 1)
			return false;

		if (GetJob() == JOB_ASSASSIN)
		{
			if (item->GetType() == ITEM_WEAPON && item->GetSubType() == WEAPON_SWORD)
				return false;
		}
	}
	#endif

	{
		int pos = GetEmptyAllInventory(item);
		VERIFY_MSG(-1 == pos, "¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù.");
	}

	// Iþýnlanma hava kýlýcý & büyülü silah sönmesin.
	if (item->GetType() == ITEM_WEAPON)
	{
		if (IsAffectFlag(AFF_GWIGUM))
			RemoveAffect(SKILL_GWIGEOM);

		if (IsAffectFlag(AFF_GEOMGYEONG))
			RemoveAffect(SKILL_GEOMKYUNG);
	}

	return true;
}

#ifdef ENABLE_OTO_TOPLAMA
void CHARACTER::AutoGiveItemNew(LPITEM pkItem)
{
	if (!pkItem)
		return;

	const DWORD dwItemVnum = pkItem->GetVnum();
	short bCount = pkItem->GetCount();

	if (pkItem->IsStackable() && pkItem->GetType() != ITEM_BLEND)
	{
		#ifdef ENABLE_SPECIAL_STORAGE
		if (CItem::IsUpgradeItemVnum(dwItemVnum)) //upgrade item
		{
			for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item = GetUpgradeInventoryItem(i);
				if (!item) { continue; }
				#ifdef ENABLE_LOCKED_SYSTEM
				if (item->IsItemLocked()) { continue; }
				#endif

				if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
				{
					if (item->GetCount() == g_bItemCountLimit)
						continue;

					if (IS_SET(pkItem->GetFlag(), ITEM_FLAG_MAKECOUNT))
					{
						if (bCount < pkItem->GetValue(1))
							bCount = pkItem->GetValue(1);
					}

					#ifdef ENABLE_NEW_MAX_COUNT
					const short bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					#else
					const BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					#endif

					bCount -= bCount2;

					if (bCount2 > 0)
						item->SetCount(item->GetCount() + bCount2);

					if (bCount == 0)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("arti_env_%s"), item->GetName());
						M2_DESTROY_ITEM(pkItem);
						return;
					}
				}
			}
		}
		else if (pkItem->GetType() == ITEM_SKILLBOOK || CItem::IsBookItemVnum(dwItemVnum))
		{
			for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item = GetBookInventoryItem(i);
				if (!item) { continue; }
				#ifdef ENABLE_LOCKED_SYSTEM
				if (item->IsItemLocked()) { continue; }
				#endif

				if (item->GetVnum() == dwItemVnum && item->GetSocket(0) == pkItem->GetSocket(0))
				{
					if (item->GetCount() == g_bItemCountLimit)
						continue;

					if (IS_SET(pkItem->GetFlag(), ITEM_FLAG_MAKECOUNT))
					{
						if (bCount < pkItem->GetValue(1))
							bCount = pkItem->GetValue(1);
					}

					#ifdef ENABLE_NEW_MAX_COUNT
					const short bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					#else
					const BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					#endif

					bCount -= bCount2;

					if (bCount2 > 0)
						item->SetCount(item->GetCount() + bCount2);

					if (bCount == 0)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bk_env_%s"), item->GetName());
						M2_DESTROY_ITEM(pkItem);
						return;
					}
				}
			}
		}
		else if ((pkItem->GetType() == ITEM_METIN && pkItem->GetType() == METIN_NORMAL) || CItem::IsStoneItemVnum(dwItemVnum)) //stone item
		{
			for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item = GetStoneInventoryItem(i);
				if (!item) { continue; }
				#ifdef ENABLE_LOCKED_SYSTEM
				if (item->IsItemLocked()) { continue; }
				#endif

				if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
				{
					if (item->GetCount() == g_bItemCountLimit)
						continue;

					if (IS_SET(pkItem->GetFlag(), ITEM_FLAG_MAKECOUNT))
					{
						if (bCount < pkItem->GetValue(1))
							bCount = pkItem->GetValue(1);
					}

					#ifdef ENABLE_NEW_MAX_COUNT
					const short bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					#else
					const BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					#endif

					bCount -= bCount2;

					if (bCount2 > 0)
						item->SetCount(item->GetCount() + bCount2);

					if (bCount == 0)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("tas_env_%s"), item->GetName());
						M2_DESTROY_ITEM(pkItem);
						return;
					}
				}
			}
		}
		else if (pkItem->GetType() == ITEM_GIFTBOX || CItem::IsChestItemVnum(dwItemVnum)) //chest item
		{
			for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item = GetChestInventoryItem(i);
				if (!item) { continue; }
				#ifdef ENABLE_LOCKED_SYSTEM
				if (item->IsItemLocked()) { continue; }
				#endif

				if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
				{
					if (item->GetCount() == g_bItemCountLimit)
						continue;

					if (IS_SET(pkItem->GetFlag(), ITEM_FLAG_MAKECOUNT))
					{
						if (bCount < pkItem->GetValue(1))
							bCount = pkItem->GetValue(1);
					}

					#ifdef ENABLE_NEW_MAX_COUNT
					const short bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					#else
					const BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					#endif

					bCount -= bCount2;

					if (bCount2 > 0)
						item->SetCount(item->GetCount() + bCount2);

					if (bCount == 0)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("sandik_env_%s"), item->GetName());
						M2_DESTROY_ITEM(pkItem);
						return;
					}
				}
			}
		}
		else
		{
			for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item = GetInventoryItem(i);
				if (!item) { continue; }
				#ifdef ENABLE_LOCKED_SYSTEM
				if (item->IsItemLocked()) { continue; }
				#endif

				if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
				{
					if (item->GetCount() == g_bItemCountLimit)
						continue;

					if (IS_SET(pkItem->GetFlag(), ITEM_FLAG_MAKECOUNT))
					{
						if (bCount < pkItem->GetValue(1))
							bCount = pkItem->GetValue(1);
					}

					#ifdef ENABLE_NEW_MAX_COUNT
					const short bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					#else
					const BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
					#endif

					bCount -= bCount2;

					if (bCount2 > 0)
						item->SetCount(item->GetCount() + bCount2);

					if (bCount == 0)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("normal_env_%s"), item->GetName());
						M2_DESTROY_ITEM(pkItem);
						return;
					}
				}
			}
		}
		#else
		for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = GetInventoryItem(i);
			if (!item) { continue; }
			#ifdef ENABLE_LOCKED_SYSTEM
			if (item->IsItemLocked()) { continue; }
			#endif

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (item->GetCount() == g_bItemCountLimit)
					continue;

				if (IS_SET(pkItem->GetFlag(), ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < pkItem->GetValue(1))
						bCount = pkItem->GetValue(1);
				}

				#ifdef ENABLE_NEW_MAX_COUNT
				const short bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
				#else
				const BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
				#endif

				bCount -= bCount2;

				if (bCount2 > 0)
					item->SetCount(item->GetCount() + bCount2);

				if (bCount == 0)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("normal_env_%s"), item->GetName());
					M2_DESTROY_ITEM(pkItem);
					return;
				}
			}
		}
		#endif
	}

	if (pkItem->GetType() == ITEM_BLEND)
	{
		for (int i = 0; i < INVENTORY_MAX_NUM; i++)
		{
			LPITEM inv_item = GetInventoryItem(i);
			if (inv_item == NULL)
				continue;

			if (inv_item->GetType() == ITEM_BLEND)
			{
				if (inv_item->GetVnum() == pkItem->GetVnum())
				{
					if (inv_item->GetSocket(0) == pkItem->GetSocket(0) && inv_item->GetSocket(1) == pkItem->GetSocket(1) && inv_item->GetSocket(2) == pkItem->GetSocket(2) && inv_item->GetCount() < g_bItemCountLimit)
					{
						const short bCount2 = MIN(g_bItemCountLimit - inv_item->GetCount(), bCount);
						bCount -= bCount2;
						if (bCount2 > 0)
							inv_item->SetCount(inv_item->GetCount() + bCount2);

						if (bCount == 0)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ È¹µæ: %s"), pkItem->GetName());
							M2_DESTROY_ITEM(pkItem);
							return;
						}
					}
				}
			}
		}
	}

	pkItem->SetCount(bCount);

	int iEmptyCell;
	if (pkItem->IsDragonSoul())
		iEmptyCell = GetEmptyDragonSoulInventory(pkItem);
	#ifdef ENABLE_SPECIAL_STORAGE
	else if (pkItem->IsUpgradeItem())
		iEmptyCell = GetEmptyUpgradeInventory(pkItem);
	else if (pkItem->IsBook())
		iEmptyCell = GetEmptyBookInventory(pkItem);
	else if (pkItem->IsStone())
		iEmptyCell = GetEmptyStoneInventory(pkItem);
	else if (pkItem->IsChest())
		iEmptyCell = GetEmptyChestInventory(pkItem);
	#endif
	else
		iEmptyCell = GetEmptyInventory(pkItem->GetSize());

	if (iEmptyCell != -1)
	{
		#ifdef ENABLE_SPECIAL_STORAGE
		if (pkItem->IsUpgradeItem())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("arti_env_%s"), pkItem->GetName());
		else if (pkItem->IsBook())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bk_env_%s"), pkItem->GetName());
		else if (pkItem->IsStone())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("tas_env_%s"), pkItem->GetName());
		else if (pkItem->IsChest())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("sandik_env_%s"), pkItem->GetName());
		else
		#endif
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("normal_env_%s"), pkItem->GetName());

		if (pkItem->IsDragonSoul())
			pkItem->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
		#ifdef ENABLE_SPECIAL_STORAGE
		else if (pkItem->IsUpgradeItem())
			pkItem->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, iEmptyCell));
		else if (pkItem->IsBook())
			pkItem->AddToCharacter(this, TItemPos(BOOK_INVENTORY, iEmptyCell));
		else if (pkItem->IsStone())
			pkItem->AddToCharacter(this, TItemPos(STONE_INVENTORY, iEmptyCell));
		else if (pkItem->IsChest())
			pkItem->AddToCharacter(this, TItemPos(CHEST_INVENTORY, iEmptyCell));
		#endif
		else
			pkItem->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));

		LogManager::instance().ItemLog(this, pkItem, "SYSTEM", pkItem->GetName());

		if (pkItem->GetType() == ITEM_USE && pkItem->GetSubType() == USE_POTION)
		{
			TQuickslot* pSlot;
			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = iEmptyCell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		pkItem->AddToGround(GetMapIndex(), GetXYZ());
		pkItem->StartDestroyEvent();

		if (IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_DROP))
			pkItem->SetOwnership(this, 300);
		else
			pkItem->SetOwnership(this, 60);

		LogManager::instance().ItemLog(this, pkItem, "SYSTEM_DROP", pkItem->GetName());
	}

	sys_log(0, "7: %d %d", dwItemVnum, bCount);
}
#endif

#ifdef ENABLE_FAST_OPEN_CHEST
bool CHARACTER::UseItemAll(TItemPos Cell)
{
	#ifdef ENABLE_RELOAD_COMMANDS
	if (g_bReloadStatus)
	{
		sys_err("ITEM_GIFTBOX Suan reload cekiliyor...");
		return false;
	}
	#endif

	if (quest::CQuestManager::instance().GetEventFlag("instant_chest_disable") == 1)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Sistem> Bu islem suanlik devredisi"));
		return false;
	}

	if (GetMapIndex() == 1 || GetMapIndex() == 21 || GetMapIndex() == 41 ||  GetMapIndex() >= 1000)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("TOPLU_KOYLERDE_SANDIK"));
		return false;
	}

	if (PreventTradeWindow(WND_TIME, true))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return false;
	}

	if (!CanHandleItem())
		return false;

	LPITEM pkItem;

	if (!IsValidItemPosition(Cell) || !(pkItem = GetItem(Cell)))
		return false;

	if (pkItem->isLocked())
		return false;

	// #ifdef ENABLE_LOCKED_SYSTEM
	// if (pkItem->IsItemLocked())
		// return false;
	// #endif

	if (!pkItem->IsChest())
		return false;

	if (pkItem->IsExchanging())
		return false;

	if (!pkItem->CanUsedBy(this))
		return false;

	if (false == FN_check_item_sex(this, pkItem))
		return false;

	SetExchangeTime();

	int itemCount = pkItem->GetCount();
	if (itemCount == 1)
	{
		UseItem(Cell);
		return false;
	}

	const CSpecialItemGroup * pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(pkItem->GetVnum());
	if (!pGroup || pGroup->IsEmpty())
		return false;

	pkItem->Lock(true);

	WORD wRowLimit	= 1000;
	WORD wRow		= 0;
	WORD wSucces	= 0;

	bool bStatus	= true;
	std::map<DWORD, int> mGiveList;

	#ifdef ENABLE_LOCKED_SYSTEM
	BYTE bGetLocked = pkItem->GetLocked();
	#endif

	while (itemCount--)
	{
		// Check open limit.
		if (wRow >= wRowLimit)
		{
			pkItem->Lock(false);
			pkItem->UpdatePacket();
			break;
		}

		pkItem->SetCount(pkItem->GetCount() - 1, (itemCount < 1) ? false : true);

		std::vector <int> idxes;
		int n = pGroup->GetMultiIndex(idxes);

		for (int i = 0; i < n; i++)
		{
			int idx			= idxes[i];
			DWORD dwVnum	= pGroup->GetVnum(idx);
			DWORD dwCount	= pGroup->GetCount(idx);

			TItemTable * pTable = ITEM_MANAGER::Instance().GetTable(dwVnum);
			if (!pTable) { return false; }

			if (pTable->IsStackable())
			{
				itertype(mGiveList) it = mGiveList.find(dwVnum);
				if (it == mGiveList.end())
					mGiveList.insert(std::make_pair(dwVnum, dwCount));
				else
					it->second += dwCount;
			}
			else
			{
				LPITEM pkGift = ITEM_MANAGER::Instance().CreateItem(dwVnum, dwCount);
				if (!pkGift)
					return false;

				int iEmptyCell = GetEmptyAllInventory(pkGift);
				if (iEmptyCell != -1)
				{
					pkGift->AddToItem(this, iEmptyCell);

					#ifdef ENABLE_LOCKED_SYSTEM
					pkGift->SetLocked(bGetLocked);
					#endif

					ChatPacket(CHAT_TYPE_INFO, "%d adet %s kazandiniz.", pkGift->GetCount(), pkGift->GetName());
				}
				else
				{
					if (pkItem)
					{
						pkItem->Lock(false);
						pkItem->SetCount(pkItem->GetCount() + 1);
					}

					if (pkGift) { M2_DESTROY_ITEM(pkGift); }

					bStatus = false;
					ChatPacket(1, "Envanterin dolu!");
					break;
				}
			}
		}

		wRow++;

		if (bStatus == false)
		{
			break;
		}

		wSucces++;
	}

	#ifdef __RANKING_SYSTEM__
	if (wSucces > 0)
	{
		SetRankManuel(RANK_CHEST, wSucces);
	}
	#endif

	for (itertype(mGiveList) it = mGiveList.begin(); it != mGiveList.end(); it++)
	{
		DWORD dwVnum	= it->first;
		DWORD iCount	= it->second;

		if (iCount >= g_bItemCountLimit)
		{
			do
			{
				short wCount = MIN(iCount, g_bItemCountLimit);
				iCount -= wCount;
				LPITEM pkGift = AutoGiveItem(dwVnum, wCount, -1, true
								#ifdef ENABLE_LOCKED_SYSTEM
								, (bGetLocked > 0 ? true : false)
								#endif
				);
				if (pkGift)
				{
					#ifdef ENABLE_LOCKED_SYSTEM
					pkGift->SetLocked(bGetLocked);
					#endif
				}
			}
			while (iCount != 0);
		}
		else
		{
			LPITEM pkGift = AutoGiveItem(dwVnum, iCount, -1, true
							#ifdef ENABLE_LOCKED_SYSTEM
							, (bGetLocked > 0 ? true : false)
							#endif
			);
			if (pkGift)
			{
				#ifdef ENABLE_LOCKED_SYSTEM
				pkGift->SetLocked(bGetLocked);
				#endif
			}
		}

		TItemTable * pTable = ITEM_MANAGER::Instance().GetTable(it->first);
		if (!pTable)
			return false;

		ChatPacket(CHAT_TYPE_INFO, "%d adet %s kazandiniz.", it->second, pTable->szLocaleName);
	}

	mGiveList.clear();
	return true;
}
#endif

#ifdef ENABLE_STYLE_ATTRIBUTE_SYSTEM
bool CHARACTER::UseItemNewAttribute(TItemPos source_pos, TItemPos target_pos, BYTE * bValues)
{
	if (quest::CQuestManager::instance().GetEventFlag("style_attribute") == 1)
	{
		ChatPacket(CHAT_TYPE_INFO, "Kadim efsun nesnesi þuanda deaktif.");
		return false;
	}


	LPITEM item;
	LPITEM item_target;

	for (int i = 0; i < 4; ++i)
	{
		if (bValues[i] < 0 || bValues[i] > 255)
			return false;
	}

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(source_pos) || !(item = GetItem(source_pos)))
		return false;

	if (!IsValidItemPosition(target_pos) || !(item_target = GetItem(target_pos)))
		return false;

	if (PreventTradeWindow(WND_ALL))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return false;
	}

	if (item->GetVnum() != 11124)
		return false;

	if (item->IsExchanging())
		return false;

	if (item_target->IsExchanging())
		return false;

	if (item_target->IsEquipped())
		return false;

	if (ITEM_COSTUME == item_target->GetType())
		return false;

	switch (item_target->GetVnum())
	{
	case 50201:
	case 50202:
	case 11901:
	case 11902:
	case 11903:
	case 11904:
	case 11911:
	case 11912:
	case 11913:
	case 11914:
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't apply this attribute on this item."));
		return false;
	}
	}
	if (item_target->GetAddonType() != 0)
		return false;

	if (item_target->NewStyleAttributeFullRate())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu esya 5/5 oranda!"));
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Eger efsunu degismek istiyorsan Efsun nesnesi ile degisip tekrar deneyin."));
		return false;
	}

	int processNewAttr = item_target->AddNewStyleAttribute(bValues);
	if (processNewAttr == 1)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't apply this attribute on this item."));
		return false;
	}
	else if (processNewAttr == 2)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't apply an attribute twice."));
		return false;
	}
	else if (processNewAttr == 3)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Attributes has been added successfully!"));
		item->SetCount(item->GetCount() - 1);
		return true;
	}

	//sys_log(0, "%s: UseItemNewAttribute %s (inven %d, cell: %d)", GetName(), item->GetName(), window_type, wCell);
	return false;
}
#endif

#ifdef ENABLE_BLACK_MARKET_SYSTEM
bool CHARACTER::BlackMarket(TItemPos Cell)
{
	if (!gef_black_market_status)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Sistem henuz aktif degil!"));
		return false;
	}

	LPITEM item = NULL;

	if (!CanHandleItem() || IsDead() || IsStun())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (item->IsEquipped() || item->isLocked() || item->IsExchanging() || (this != item->GetOwner()))
		return false;

	#ifdef ENABLE_LOCKED_SYSTEM
	if (true == item->IsItemLocked())
		return false;
	#endif

	if (PreventTradeWindow(WND_ALL))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return false;
	}

	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning())
		return false;

	/*if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_SELL))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu itemi satamazsin."));
		return false;
	}*/

	if (item->GetCount() <= 0)
		return false;

	int iPulse = thecore_pulse();
	if (iPulse		- GetSafeboxLoadTime()		< PASSES_PER_SEC(g_nPortalLimitTime)
		|| iPulse	- GetRefineTime()			< PASSES_PER_SEC(g_nPortalLimitTime)
		|| iPulse	- GetMyShopTime()			< PASSES_PER_SEC(g_nPortalLimitTime))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu islem icin biraz beklemelisin."));
		return false;
	}

	#ifdef ENABLE_FULL_YANG
	uint64_t goldprice = CBlackMarket::instance().GetGoldPrice(item->GetVnum());
	#else
	long goldprice = CBlackMarket::instance().GetGoldPrice(item->GetVnum());
	#endif

	if (goldprice <= 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s iteminin piyasa bilgisi mevcut degil."), item->GetName());
		return false;
	}

	int iStock = CBlackMarket::instance().GetStock(item->GetVnum());
	if (iStock == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Butun stoklar tukendi!"));
		return false;
	}
	else if (iStock > 0)
	{
		if (iStock < item->GetCount())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Yeterli stok kalmadi! Kalan stok: %d"), iStock);
			return false;
		}
	}

	if (item->GetCount() > 1)
	{
		#ifdef ENABLE_FULL_YANG
		uint64_t llMoney = (goldprice * item->GetCount());
		if (llMoney >= FULL_YANG_MAX)
		#else
		long llMoney = (goldprice * item->GetCount());
		if (llMoney >= GOLD_MAX)
		#endif
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Uzerinde bu kadar fazla yang tasiyamazsin."));
			return false;
		}
		else
		{
			goldprice = (goldprice * item->GetCount());
		}
	}

	const int64_t nTotalMoney = static_cast<int64_t>(GetGold()) + static_cast<int64_t>(goldprice);
	#ifdef ENABLE_FULL_YANG
	if (FULL_YANG_MAX <= nTotalMoney)
	#else
	if (GOLD_MAX <= nTotalMoney)
	#endif
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Uzerinde bu kadar fazla yang tasiyamazsin."));
		return false;
	}

	#ifdef ENABLE_FULL_YANG
	uint64_t givegold = goldprice;
	#else
	long givegold = goldprice;
	#endif

	if (givegold <= 0)
	{
		return false;
	}

	if (iStock > 0)
	{
		CBlackMarket::instance().SetStock(item->GetVnum(), item->GetCount());
	}

	ITEM_MANAGER::instance().RemoveItem(item);

	#ifdef ENABLE_FULL_YANG
	GoldChange(givegold);
	#else
	PointChange(POINT_GOLD, givegold, false);
	#endif

	return true;
}
#endif

#if defined(__BL_LUCKY_BOX__)
void CHARACTER::SetLuckyBoxSrcItem(LPITEM item)
{
	if (!item) { return; }

	if (PreventTradeWindow(WND_ALL))
	{
		ChatPacket(CHAT_TYPE_INFO, "Diger pencereleri kapatmanýz gerekiyor.");
		return;
	}

	ResetLuckyBoxData();
	switch (item->GetVnum())
	{
	case BLUE_FORTUNE_CHEST:
		m_sLuckyBox.type = CLuckyBoxGroup::ELUCKY_BOX_TYPE::LUCKY_BOX_TYPE_BLUE;
		break;
	case RED_FORTUNE_CHEST:
		m_sLuckyBox.type = CLuckyBoxGroup::ELUCKY_BOX_TYPE::LUCKY_BOX_TYPE_RED;
		break;
	case GREEN_FORTUNE_CHEST:
		m_sLuckyBox.type = CLuckyBoxGroup::ELUCKY_BOX_TYPE::LUCKY_BOX_TYPE_GREEN;
		break;
	default:
		return;
	}
	m_sLuckyBox.src_item_id = item->GetID();

	SendLuckyBoxInfo();
}

void CHARACTER::SendLuckyBoxInfo()
{
	if (!IsLuckyBoxOpen()) { return; }
	if (!GetDesc()) { return; }

	CLuckyBoxGroup* pLuckyBox = ITEM_MANAGER::Instance().GetLuckyBoxGroup();
	if (!pLuckyBox) { return; }

	m_sLuckyBox.try_count++;

	BYTE bWhileCount = 0;
	while (true)
	{
		if (bWhileCount > 250) { break; }
		bWhileCount++;

		const CLuckyBoxGroup::SLuckyBoxItemInfo& item = pLuckyBox->GetRandomItem(m_sLuckyBox.type);
		if (pLuckyBox->GetItemCount(m_sLuckyBox.type) > 1 && item.vnum == m_sLuckyBox.item_vnum)
		{
			continue;
		}

		m_sLuckyBox.item_vnum = item.vnum;
		m_sLuckyBox.item_count = item.count;
		break;
	}

	TPacketGCLuckyBox p;
	p.header = HEADER_GC_LUCKY_BOX;
	p.vnum = m_sLuckyBox.item_vnum;
	p.count = m_sLuckyBox.item_count;
	p.need_money = GetLuckyBoxPrice();
	GetDesc()->Packet(&p, sizeof(p));
}

void CHARACTER::LuckyBoxRetry()
{
	if (!IsLuckyBoxOpen()) { return; }

	CLuckyBoxGroup* pLuckyBox = ITEM_MANAGER::Instance().GetLuckyBoxGroup();
	if (!pLuckyBox) { return; }

	if (m_sLuckyBox.try_count >= pLuckyBox->GetMaxTryCount())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Sistem> Artik bunu yapamazsiniz esyayi kabul ediniz"));
		return;
	}

	#ifdef ENABLE_FULL_YANG
	const int64_t iPrice = GetLuckyBoxPrice();
	#else
	const int iPrice = GetLuckyBoxPrice();
	#endif
	if (iPrice > GetGold())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT( "<Sistem> Yeterli yanginiz yok."));
		return;
	}

	#ifdef ENABLE_FULL_YANG
	GoldChange(-iPrice);
	#else
	PointChange(POINT_GOLD, -iPrice);
	#endif

	ChatPacket(CHAT_TYPE_INFO, "<Sistem> Karakterinizden %d yang eksildi.", iPrice);
	SendLuckyBoxInfo();
}

void CHARACTER::LuckyBoxReceive()
{
	if (!IsLuckyBoxOpen()) { return; }

	LPITEM src_item = ITEM_MANAGER::Instance().Find(m_sLuckyBox.src_item_id);
	if (src_item)
	{
		ITEM_MANAGER::Instance().RemoveItem(src_item);
		AutoGiveItem(m_sLuckyBox.item_vnum, m_sLuckyBox.item_count);
	}

	ResetLuckyBoxData();
}

#ifdef ENABLE_FULL_YANG
int64_t CHARACTER::GetLuckyBoxPrice() const
#else
int CHARACTER::GetLuckyBoxPrice() const
#endif
{
	CLuckyBoxGroup* pLuckyBox = ITEM_MANAGER::Instance().GetLuckyBoxGroup();
	if (!pLuckyBox) { return 0; }

	#ifdef ENABLE_FULL_YANG
	int64_t ret = pLuckyBox->GetPrice();
	#else
	int ret = pLuckyBox->GetPrice();
	#endif

	for (int i = 1; i < m_sLuckyBox.try_count; i++)
	{
		ret *= 2;
	}

	return ret;
}

bool CHARACTER::IsLuckyBoxOpen() const
{
	return m_sLuckyBox.src_item_id != 0;
}

void CHARACTER::ResetLuckyBoxData()
{
	memset(&m_sLuckyBox, 0, sizeof(m_sLuckyBox));
}
#endif
