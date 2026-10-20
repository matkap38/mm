#include "stdafx.h"
#include "../../common/tables.h"
#include "packet.h"
#include "item.h"
#include "char.h"
#include "item_manager.h"
#include "desc.h"
#include "char_manager.h"
#include "banword.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "config.h"
#include "event.h"
#include "locale_service.h"
#include <fstream>
#include "questmanager.h"
#include "sectree_manager.h"
#include "sectree.h"
#include "config.h"
#include "db.h"
#include <msl/random.h>

#ifdef ENABLE_BATTLE_PASS
#include "battle_pass.h"
#endif

#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
#include "GetEventFlagSystem.h"
#endif

#ifdef ENABLE_IKASHOP_RENEWAL

#include "ikarus_shop.h"
#include "ikarus_shop_manager.h"

#define SHOP_DISTANCE
#define SHOP_ONLY_CHANNEL1

// static const std::set<int> ALLOWED_SPAWN_MAPS = { 1,21,41 };//Sadece 1.köyler
static const std::set<int> ALLOWED_SPAWN_MAPS = { 1,3,21,23,41,43 };//1 ve 2.köyler

template<class DerivedType, class BaseType>
DerivedType DerivedFromBase(const BaseType& base)
{
	static_assert(std::is_base_of<BaseType, DerivedType>::value); // DerivedType must be inherited by BaseType
	DerivedType temporary{};
	static_cast<BaseType&>(temporary) = base;
	return temporary;
}

// #define ENABLE_IKASHOP_GM_PROTECTION
static bool CheckGMLevel(LPCHARACTER ch) 
{
	return
		#ifdef ENABLE_IKASHOP_GM_PROTECTION
		ch->GetGMLevel() == GM_PLAYER || test_server;
		#else
		true;
		#endif
}

bool MatchAttributes(const TPlayerItemAttribute* pAttributesFilter, const TPlayerItemAttribute* pAttributesItem)
{
	static auto findAttribute = [](auto type, auto val, auto attr)
	{
		for (int j = 0; j < ITEM_ATTRIBUTE_NORM_NUM; j++)
			if (attr[j].bType == type)
				return attr[j].sValue >= val;
		return false;
	};

	auto matchFilterAttribute = [&](auto filter)
	{
		return filter.bType == 0 || findAttribute(filter.bType, filter.sValue, pAttributesItem);
	};

	return std::all_of(pAttributesFilter, pAttributesFilter + ITEM_ATTRIBUTE_NORM_NUM, matchFilterAttribute);
}

std::string StringToLower(const char* name, size_t len)
{
	// std::tolower handles ints so transform reject it as operand
	static auto lower = [](auto&& ch){ return std::tolower(ch); };
	// filling string inserting lower characters
	std::string res = {name, len};
	std::transform(res.begin(), res.end(), res.begin(), lower);
	return res;
}

bool IsGoodSalePrice(const ikashop::TPriceInfo price)
{
	#ifdef ENABLE_FULL_YANG
	if (static_cast<int64_t>(price.yang) >= 999000000000000001LL || price.yang < 0)
		return false;
	#else
	if (price.yang >= GOLD_MAX || price.yang < 0)
		return false;
	#endif
	return true;
}

const std::map<BYTE, std::string> m_bookText = {
	{ 1, "Üç Yönlü Kesme" },
	{ 2, "Kýlýç Çevirme" },
	{ 3, "Öfke" },
	{ 4, "Hava Kýlýcý" },
	{ 5, "Hamle" },
	{ 16, "Ruh Vuruþu" },
	{ 17, "Þiddetli Vuruþ" },
	{ 18, "Güçlü Vuruþ" },
	{ 19, "Güçlü Beden" },
	{ 20, "Kýlýç Darbesi" },
	{ 31, "Suikast" },
	{ 32, "Hýzlý Saldýrý" },
	{ 33, "Býçak Çevirme" },
	{ 34, "Kamuflaj" },
	{ 35, "Zehirli Bulut" },
	{ 46, "Tekrarlanan Atýþ" },
	{ 47, "Ok Yaðmuru" },
	{ 48, "Ateþli Ok" },
	{ 49, "Hafif Adým" },
	{ 50, "Zehirli Ok" },
	{ 61, "Parmak Darbesi" },
	{ 62, "Ejderha Dönüþü" },
	{ 63, "Büyülü Keskinlik" },
	{ 64, "Dehþet" },
	{ 65, "Büyülü Zýrh" },
	{ 66, "Büyü Çözme" },
	{ 76, "Karanlýk Vuruþ" },
	{ 77, "Ateþ Vuruþu" },
	{ 78, "Ateþ Hayaleti" },
	{ 79, "Karanlýk Koruma" },
	{ 80, "Hayalet Vuruþ" },
	{ 81, "Karanlýk Küre" },
	{ 91, "Uçan Týlsým" },
	{ 92, "Ejderha Atýþý" },
	{ 93, "Ejderha Kükremesi" },
	{ 94, "Kutsama" },
	{ 95, "Yansýtma" },
	{ 96, "Ejderha Yardýmý" },
	{ 106, "Þimþek Atma" },
	{ 107, "Þimþek Çaðýrma" },
	{ 108, "Þimþek Pençesi" },
	{ 109, "Ýyileþtirme" },
	{ 110, "Hýz" },
	{ 111, "Yüksek Saldýrý" },
};

bool MatchBookName(std::string stName, long lSocket)
{
	if (lSocket == 0) { return false; }
	if (m_bookText.empty()) { return false; }
	for (auto it = m_bookText.begin(); it != m_bookText.end(); ++it)
	{
		const auto lower = StringToLower(it->second.c_str(), strlen(it->second.c_str()));
		if (lower.find(stName) != std::string::npos)
		{
			if (lSocket == long(it->first)) { return true; }
		}
	}
	return false;
}

bool MatchItemName(std::string stName, const char* table , const size_t tablelen)
{
	const auto lower = StringToLower(table, tablelen);
	return lower.find(stName) != std::string::npos;
}

bool CheckCharacterActions(LPCHARACTER ch)
{
	if (!ch || !ch->CanHandleItem())
		return false;

	if (ch->IsWarping())
		return false;

	if (ch->PreventTradeWindow(WND_TIME | WND_OFFLINE_SHOP, true))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return false;
	}

	if (quest::CQuestManager::Instance().GetPCForce(ch->GetPlayerID())->IsRunning())
		return false;

	if (ch->m_pkTimedEvent)
		return false;

	return true;
}

bool MatchFilterType(const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table)
{
	if (filter.type == 0)
		return true;

	static auto MatchWeapon = [](const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table) -> bool
	{
		if (table->bType != ITEM_WEAPON)
			return false;
		if (filter.subtype == 0)
			return true;
		switch(filter.subtype)
		{
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_SWORD:
				return table->bSubType == WEAPON_SWORD
					&& IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_SHAMAN) 
					&& !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_WARRIOR) 
					&& !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_ASSASSIN) 
					&& !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_SURA);
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_BLADE:
				return table->bSubType == WEAPON_SWORD 
					&& IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_SHAMAN)
					&& IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_WARRIOR)
					&& IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_ASSASSIN)
					&& !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_SURA);
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_TWOHANDED:
				return table->bSubType == WEAPON_TWO_HANDED;
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_DAGGER:
				return table->bSubType == WEAPON_DAGGER;
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_BOW:
				return table->bSubType == WEAPON_BOW;
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_BELL:
				return table->bSubType == WEAPON_BELL;
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_FAN:
				return table->bSubType == WEAPON_FAN;
			#ifdef ENABLE_WOLFMAN_CHARACTER
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_CLAW:
				return table->bSubType == WEAPON_CLAW;
			#endif
		}
		return false;
	};

	static auto MatchArmor = [](const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table)
	{
		if (table->bType != ITEM_ARMOR || table->bSubType != ARMOR_BODY)
			return false;
		if (filter.subtype == 0)
			return true;
		switch(filter.subtype)
		{
			case ikashop::SEARCH_SHOP_SUBTYPE_ARMOR_WARRIOR:
				return !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_WARRIOR);
			case ikashop::SEARCH_SHOP_SUBTYPE_ARMOR_SURA:
				return !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_SURA);
			case ikashop::SEARCH_SHOP_SUBTYPE_ARMOR_ASSASSIN:
				return !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_ASSASSIN);
			case ikashop::SEARCH_SHOP_SUBTYPE_ARMOR_SHAMAN:
				return !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_SHAMAN);
			#ifdef ENABLE_WOLFMAN_CHARACTER
			case ikashop::SEARCH_SHOP_SUBTYPE_ARMOR_WOLFMAN:
				return !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_WOLFMAN);
			#endif
		}
		return false;
	};

	static auto MatchJewel = [](const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table)
	{
		if (table->bType != ITEM_ARMOR || table->bSubType == ARMOR_BODY)
			return false;
		if (filter.subtype == 0)
			return true;
		switch(filter.subtype)
		{
			case ikashop::SEARCH_SHOP_SUBTYPE_JEWEL_HELM:
				return table->bSubType == ARMOR_HEAD;
			case ikashop::SEARCH_SHOP_SUBTYPE_JEWEL_SHIELD:
				return table->bSubType == ARMOR_SHIELD;
			case ikashop::SEARCH_SHOP_SUBTYPE_JEWEL_EAR:
				return table->bSubType == ARMOR_EAR;
			case ikashop::SEARCH_SHOP_SUBTYPE_JEWEL_NECK:
				return table->bSubType == ARMOR_NECK;
			case ikashop::SEARCH_SHOP_SUBTYPE_JEWEL_WRIST:
				return table->bSubType == ARMOR_WRIST;
			case ikashop::SEARCH_SHOP_SUBTYPE_JEWEL_SHOES:
				return table->bSubType == ARMOR_FOOTS;
		}
		return false;
	};

	static auto MatchCostume = [](const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table)
	{
		if (table->bType != ITEM_COSTUME)
			return false;
		if (filter.subtype == 0)
			return true;
		switch(filter.subtype)
		{
		case ikashop::SEARCH_SHOP_SUBTYPE_COSTUME_BODY:
			return table->bSubType == COSTUME_BODY;
		case ikashop::SEARCH_SHOP_SUBTYPE_COSTUME_WEAPON:
			return table->bSubType == COSTUME_WEAPON;
		case ikashop::SEARCH_SHOP_SUBTYPE_COSTUME_HAIR:
			return table->bSubType == COSTUME_HAIR;
		#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		case ikashop::SEARCH_SHOP_SUBTYPE_COSTUME_SASH:
			return table->bSubType == COSTUME_ACCE;
		#endif
		}
		return false;
	};

	static auto MatchPet = [](const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table)
	{
		if (table->bType != ITEM_PET || (table->bSubType != PET_EGG && table->bSubType != PET_UPBRINGING && table->bSubType != PET_PAY))
			return false;
		if (filter.subtype == 0)
			return true;
		switch(filter.subtype)
		{
		case ikashop::SEARCH_SHOP_SUBTYPE_PET_EGGS:
			return table->bSubType == PET_EGG;
		case ikashop::SEARCH_SHOP_SUBTYPE_PET_SEALS:
			return table->bSubType == PET_UPBRINGING || table->bSubType == PET_PAY;
		}
		return false;
	};

	switch(filter.type)
	{
		case ikashop::SEARCH_SHOP_TYPE_WEAPON:
			return MatchWeapon(info, filter, table);
		case ikashop::SEARCH_SHOP_TYPE_ARMOR:
			return MatchArmor(info, filter, table);
		case ikashop::SEARCH_SHOP_TYPE_JEWEL:
			return MatchJewel(info, filter, table);
		case ikashop::SEARCH_SHOP_TYPE_COSTUME:
			return MatchCostume(info, filter, table);
		case ikashop::SEARCH_SHOP_TYPE_PET:
			return MatchPet(info, filter, table);
		case ikashop::SEARCH_SHOP_TYPE_ALCHEMY:
			return table->bType == ITEM_DS;
		case ikashop::SEARCH_SHOP_TYPE_BOOK:
			return (table->bType == ITEM_SKILLBOOK);
		case ikashop::SEARCH_SHOP_TYPE_STONE:
			return table->bType == ITEM_METIN;
		case ikashop::SEARCH_SHOP_TYPE_FISH:
			return table->bType == ITEM_FISH;
		case ikashop::SEARCH_SHOP_TYPE_MINERALS:
			return table->bType == ITEM_USE && (table->bSubType == USE_PUT_INTO_ACCESSORY_SOCKET || table->bSubType == USE_PUT_INTO_BELT_SOCKET);
		case ikashop::SEARCH_SHOP_TYPE_MOUNT:
			return table->bType == ITEM_COSTUME && table->bSubType == COSTUME_MOUNT;
	}

	return false;
}

bool MatchSpecial(const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table)
{
	if (table->bType == ITEM_DS)
	{
		if (filter.alchemyGrade != 0)
		{
			const auto grade = (table->dwVnum / 1000) % 10;
			return grade+1 >= filter.alchemyGrade;
		}
	}

	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if (table->bType == ITEM_COSTUME && table->bSubType == COSTUME_ACCE)
	{
		if (filter.sashGrade != 0)
		{
			switch(filter.sashGrade)
			{
			case ikashop::SEARCH_SHOP_SASH_GRADE_VALUE0:
				return info.alSockets[ACCE_ABSORPTION_SOCKET] >= 10;
			case ikashop::SEARCH_SHOP_SASH_GRADE_VALUE1:
				return info.alSockets[ACCE_ABSORPTION_SOCKET] >= 15;
			case ikashop::SEARCH_SHOP_SASH_GRADE_VALUE2:
				return info.alSockets[ACCE_ABSORPTION_SOCKET] >= 20;
			case ikashop::SEARCH_SHOP_SASH_GRADE_VALUE3:
				return info.alSockets[ACCE_ABSORPTION_SOCKET] == 25;
			}
			return false;
		}
	}
	#endif

	return true;
}

bool MatchFilter(const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, const std::string& filtername)
{
	auto table = ITEM_MANAGER::instance().GetTable(info.vnum);
	if (!table)
	{
		sys_err("CANNOT FIND ITEM TABLE [%d]", info.vnum);
		return false;
	}

	// filter by type / subtype
	if (!MatchFilterType(info, filter, table))
		return false;

	// filter by level
	if ((filter.levelstart != 0 || filter.levelend != 0))
	{
		const auto level = table->aLimits[0].bType == LIMIT_LEVEL ?
			table->aLimits[0].lValue :
			table->aLimits[1].bType == LIMIT_LEVEL ?
			table->aLimits[1].lValue : 0;
		if (filter.levelstart != 0 && filter.levelstart > level)
			return false;
		if (filter.levelend != 0 && filter.levelend < level)
			return false;
	}

	// filter by price
	if (filter.pricestart.yang != 0 || filter.pricend.yang != 0)
	{
		const auto total = info.price.GetTotalYangAmount();

		if (filter.pricestart.yang != 0 && total < filter.pricestart.yang)
			return false;

		if (filter.pricend.yang != 0 && total > filter.pricend.yang)
			return false;
	}

	// filter by count
	if (filter.countstart != 0 || filter.countend != 0)
	{
		const auto total = info.count;

		if (filter.countstart != 0 && total < filter.countstart)
			return false;

		if (filter.countend != 0 && total > filter.countend)
			return false;
	}

	if (info.vnum == 50300)
	{
		// filter by book name
		if (!filtername.empty() && !MatchBookName(filtername, info.alSockets[0]))
			return false;
	}
	else
	{
		// filter by name
		if (!filtername.empty() && !MatchItemName(filtername, table->szLocaleName, strnlen(table->szLocaleName, ITEM_NAME_MAX_LEN)))
			return false;
	}

	// filter by attrs
	if (!MatchAttributes(filter.attrs, info.aAttr))
		return false;

	// filter bu special
	if (!MatchSpecial(info, filter, table))
		return false;

	return true;
}

#ifdef ENABLE_SHOP_SEARCH_SYSTEM
bool MatchFilterSearch(const ikashop::TShopItem& info, const ikashop::TFilterName& filter, const std::string& filtername)
{
	auto table = ITEM_MANAGER::instance().GetTable(info.vnum);
	if (!table)
	{
		sys_err("CANNOT FIND ITEM TABLE [%d]", info.vnum);
		return false;
	}

	if (filtername.length() > 2)
	{
		// filter by name
		if (!filtername.empty() && !MatchItemName(filtername, table->szLocaleName, strnlen(table->szLocaleName, ITEM_NAME_MAX_LEN)))
			return false;
	}
	else
	{
		// filter by vnum
		if (filter.dwVnum != info.vnum)
			return false;

		if (info.vnum == 50300)
		{
			// filter by book socket
			if (filter.lSocket != info.alSockets[0])
				return false;
		}
	}

	return true;
}
#endif

long long GetAuctionMinRaise(ikashop::CShopManager::AUCTION_HANDLE auction)
{
	auto bestoff = auction->GetBestOffer();
	if (!bestoff)
		return auction->GetInfo().price.GetTotalYangAmount();

	const auto minRaise = bestoff->price;
	const auto money = minRaise.GetTotalYangAmount();
	if (money < 1000)
		return money + 1000;

	const auto percentage = 1.0 + static_cast<double>(OFFLINESHOP_AUCTION_RAISE_PERCENTAGE) / 100.0;
	return static_cast<long long>(static_cast<double>(money) * percentage);
}

void MoveAndDestroyInstance(LPITEM item, BYTE window, WORD pos)
{
	auto ch = item->GetOwner();
	item->RemoveFromCharacter();
	item->SetWindow(window);
	item->SetCell(ch, pos);
	item->SetSkipSave(false);
	item->Save();
	ITEM_MANAGER::instance().FlushDelayedSave(item);
	item->SetSkipSave(true);
	M2_DESTROY_ITEM(item->RemoveFromCharacter());
}

bool CheckOfferPrice(const ikashop::TPriceInfo& offerPrice, const ikashop::TPriceInfo& itemPrice)
{
	const auto offer = offerPrice.GetTotalYangAmount();
	const auto item = itemPrice.GetTotalYangAmount();
	const auto minOffer = item * OFFLINESHOP_MIN_OFFER_PERCENTAGE / 100;
	return minOffer <= offer && offer < item;
}

namespace ikashop
{
	EVENTFUNC(func_offlineshop_update_duration)
	{
		auto& manager = ikashop::GetManager();
		manager.UpdateShopsDuration();
		manager.UpdateAuctionsDuration();
		manager.ClearSearchTimeMap();
		return OFFLINESHOP_DURATION_UPDATE_TIME;
	}

	ikashop::CShopManager& GetManager()
	{
		return ikashop::CShopManager::instance();
	}

	CShopManager::SHOP_HANDLE CShopManager::PutsNewShop(const TShopInfo& info)
	{
		auto& shop = m_mapShops[info.ownerid] = std::make_shared<ikashop::CShop>();
		shop->SetDuration(info.duration);
		shop->SetOwnerPID(info.ownerid);
		shop->SetName(info.name);
		#ifdef ENABLE_IKASHOP_ENTITIES
		shop->SetSpawn(info.spawn);
		CreateShopEntity(shop);
		#endif
		return shop;
	}

	void CShopManager::PutsAuction(const TAuctionInfo& auction)
	{
		auto obj = std::make_shared<CAuction>();
		obj->SetInfo(auction);
		m_mapAuctions[auction.owner] = obj;
	}

	void CShopManager::PutsAuctionOffer(const TAuctionOfferInfo& offer)
	{
		if (auto cache = GetAuctionByOwnerID(offer.ownerid))
			cache->AddOffer(offer);
	}

	CShopManager::SHOP_HANDLE CShopManager::GetShopByOwnerID(DWORD pid)
	{
		auto it = m_mapShops.find(pid);
		return it != m_mapShops.end() ? it->second : nullptr;
	}

	CShopManager::AUCTION_HANDLE CShopManager::GetAuctionByOwnerID(DWORD pid)
	{
		auto it = m_mapAuctions.find(pid);
		return it != m_mapAuctions.end() ? it->second : nullptr;
	}

	CShopManager::CShopManager()
	{
		auto* info = AllocEventInfo<event_info_data>();
		m_eventShopDuration = event_create(func_offlineshop_update_duration, info, OFFLINESHOP_DURATION_UPDATE_TIME);
	}

	CShopManager::~CShopManager()
	{
		Destroy();
	}

	void CShopManager::Destroy()
	{
		// deleting event
		if (m_eventShopDuration)
			event_cancel(&m_eventShopDuration);
		m_eventShopDuration = nullptr;

		// clearing containers
		m_mapOffersByBuyer.clear();
		m_mapSafeboxs.clear();
		m_mapShops.clear();
		m_mapOffer.clear();
	}

	void CShopManager::_RemoveShopOffers(SHOP_HANDLE shop)
	{
		// storing all buyers's pids
		std::set<DWORD> buyers;

		// iterating over shop's offers
		for (auto& iter : shop->GetOffers())
		{
			auto& offer = iter.second;

			// for each offer removing from buyer's offer list
			DWORD buyer = offer->buyerid;
			buyers.emplace(buyer);

			// searching buyer into map
			auto it = m_mapOffersByBuyer.find(buyer);
			if (it != m_mapOffersByBuyer.end())
			{
				// removing offer from vector
				auto& offers = it->second;
				std::erase_if (offers, [=](auto&& boffer) { return boffer->offerid == offer->offerid; });
			}

			// removing offer from offer map
			m_mapOffer.erase(offer->offerid);
		}

		// refreshing offers page for buyers
		for (auto& pid : buyers)
			if (auto chBuyer = CHARACTER_MANAGER::instance().FindByPID(pid))
				if (chBuyer->IsLookingIkarusShopOfferList())
					RecvOfferListRequestPacket(chBuyer);
	}

	#ifdef ENABLE_IKASHOP_ENTITIES
	void CShopManager::CreateShopEntity(SHOP_HANDLE shop)
	{
		auto& spawn = shop->GetSpawn();
		if (spawn.bChannel == g_bChannel)
		{
			if (auto sectree = SECTREE_MANAGER::Instance().Get(spawn.map, spawn.x, spawn.y))
			{
				auto entity = std::make_shared<ShopEntity>();
				entity->SetShopName(shop->GetName());
				entity->SetMapIndex(spawn.map);
				entity->SetXYZ(spawn.x, spawn.y, 0);
				entity->SetShop(shop);
				sectree->InsertEntity(entity.get());
				entity->UpdateSectree();
				shop->SetEntity(entity);
				#ifdef ENABLE_SHOP_SEARCH_SYSTEM
				shop->SetShopVID(entity->GetVID());
				#endif
				m_entityByVID[entity->GetVID()] = entity;
			}
		}
	}

	void CShopManager::DestroyShopEntity(SHOP_HANDLE shop)
	{
		if (auto entity = shop->GetEntity())
		{
			m_entityByVID.erase(entity->GetVID());
			if (entity->GetSectree())
			{
				entity->ViewCleanup();
				entity->GetSectree()->RemoveEntity(entity.get());
			}
			entity->Destroy();
		}
	}

	void CShopManager::EncodeInsertShopEntity(const ShopEntity& shop, LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_INSERT_SHOP_ENTITY;
		pack.size = sizeof(pack)+ sizeof(TSubPacketGCInsertShopEntity);

		const auto& pos = shop.GetXYZ();

		TSubPacketGCInsertShopEntity subpack{};
		subpack.vid = shop.GetVID();
		subpack.type = shop.GetShopType();
		subpack.x = pos.x;
		subpack.y = pos.y;
		subpack.z = pos.z;
		str_to_cstring(subpack.name, shop.GetShopName());

		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&subpack, sizeof(subpack));
	}

	void CShopManager::EncodeRemoveShopEntity(const ShopEntity& shop, LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_REMOVE_SHOP_ENTITY;
		pack.size = sizeof(pack)+ sizeof(TSubPacketGCRemoveShopEntity);

		TSubPacketGCRemoveShopEntity subpack{};
		subpack.vid = shop.GetVID();

		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&subpack, sizeof(subpack));
	}
	#endif

	CShopManager::SAFEBOX_HANDLE CShopManager::GetShopSafeboxByOwnerID(DWORD pid)
	{
		auto it = m_mapSafeboxs.find(pid);
		return it != m_mapSafeboxs.end() ? it->second : nullptr;
	}

	//offers
	bool CShopManager::PutsNewOffer(OFFER_HANDLE offer)
	{
		// registering offer by buyer
		auto& offers = m_mapOffersByBuyer[offer->buyerid];
		for(auto& boffer : offers)
			if (boffer->offerid == offer->offerid)
				return false;

		// adding offer to buyer list
		offers.emplace_back(offer);
		// registering offer by offer id
		m_mapOffer[offer->offerid] = offer;
		return true;
	}

	//db packets exchanging
	void CShopManager::SendShopBuyDBPacket(DWORD buyerid, DWORD ownerid, DWORD itemid, bool success)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_BUY_ITEM;

		TSubPacketGDBuyItem subpack{};
		subpack.guestid = buyerid;
		subpack.ownerid = ownerid;
		subpack.itemid = itemid;
		subpack.success = success;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopBuyDBPacket(DWORD buyerid, DWORD ownerid,DWORD itemid)
	{
		// searching shop
		auto shop = GetShopByOwnerID(ownerid);
		if (!shop)
			return false;

		// searching item
		auto shopItem = shop->GetItem(itemid);
		if (!shopItem)
			return false;

		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(buyerid))
		{
			auto itemInstance = shopItem->CreateItem();
			if (!itemInstance)
			{
				sys_err("cannot create item ( itemid %u , vnum %u, shop owner %u, buyer %u )", itemid, shopItem->GetInfo().vnum, ownerid, buyerid );
				return false;
			}

			TItemPos pos;
			if (!ch->CanTakeInventoryItem(itemInstance, &pos))
			{
				itemInstance->SetSkipSave(true);
				M2_DESTROY_ITEM(itemInstance);

				SendShopSafeboxAddItemDBPacket(ch->GetPlayerID(), *shopItem);
				SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_SAFEBOX_RECV_ITEM");
			}
			else
			{
				itemInstance->AddToCharacter(ch, pos);
				itemInstance->SetSkipSave(false);
				itemInstance->Save();
				ITEM_MANAGER::instance().FlushDelayedSave(itemInstance);
			}

			ch->SetMyShopTime();
			ch->SetMyExchangeTime();

			shop->BuyItem(shopItem->GetID());
			SendSearchResultDeleteClientPacket(ch, itemid);
		}
		else
		{
			shop->BuyItem(shopItem->GetID());
		}

		return true;
	}

	void CShopManager::SendShopEditItemDBPacket(DWORD ownerid, DWORD itemid, const TPriceInfo& price)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_EDIT_ITEM;

		TSubPacketGDEditItem subpack{};
		subpack.ownerid = ownerid;
		subpack.itemid = itemid;
		subpack.price = price;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopEditItemClientPacket(LPCHARACTER ch, DWORD itemid, const TPriceInfo& price)
	{
		if (!ch || !ch->GetIkarusShop())
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_EDIT_ITEM))
			return false;

		if (!IsGoodSalePrice(price))
			return false;

		auto shop = ch->GetIkarusShop();
		auto shopItem = shop->GetItem(itemid);
		if (!shopItem)
			return false;

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		auto& itemPrice = shopItem->GetPrice();
		if (price.yang == itemPrice.yang)
			return true;

		SendShopEditItemDBPacket(shop->GetOwnerPID(), itemid, price);
		return true;
	}

	void CShopManager::SendShopRemoveItemDBPacket(DWORD ownerid, DWORD itemid)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = SUBHEADER_GD_REMOVE_ITEM;

		TSubPacketGDRemoveItem subpack{};
		subpack.ownerid = ownerid;
		subpack.itemid = itemid;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopRemoveItemDBPacket(DWORD ownerid, DWORD itemid, bool requester)
	{
		// searching shop
		auto shop = GetShopByOwnerID(ownerid);
		if (!shop)
			return false;

		// searching item
		auto shopItem = shop->GetItem(itemid);
		if (!shopItem)
			return false;

		// removing from shop
		shop->RemoveItem(itemid);

		// checking for requester
		if (requester)
		{
			static auto pos = TItemPos{};
			// checking if character can obtain the item
			auto ch = shop->FindOwnerCharacter();
			auto item = shopItem->CreateItem();

			if (!item || !ch || !ch->CanTakeInventoryItem(item, &pos))
			{
				// cleaning up item
				if (item)
				{
					item->SetSkipSave(true);
					M2_DESTROY_ITEM(item);
				}

				// sending item to safebox
				SendShopSafeboxAddItemDBPacket(ownerid, *shopItem);
			}
			else
			{
				item->AddToCharacter(ch, pos);
				item->SetSkipSave(false);
				item->Save();
				ITEM_MANAGER::instance().FlushDelayedSave(item);
			}
		}

		return true;
	}

	void CShopManager::SendShopAddItemDBPacket(DWORD ownerid, const TShopItem& iteminfo)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_ADD_ITEM;

		TSubPacketGDAddItem subpack{};
		subpack.ownerid = ownerid;
		subpack.sitem = iteminfo;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopAddItemDBPacket(DWORD ownerid, const TShopItem& iteminfo)
	{
		// searching shop
		auto shop = GetShopByOwnerID(ownerid);
		if (!shop)
			return false;

		// adding item to shop
		return shop->AddItem(iteminfo);
	}

	//SHOPS
	void CShopManager::SendShopForceCloseDBPacket(DWORD pid, bool gm = false)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_SHOP_FORCE_CLOSE;

		TSubPacketGDShopForceClose subpack{};
		subpack.ownerid = pid;
		subpack.gm = gm;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopForceCloseDBPacket(DWORD pid)
	{
		// getting shop
		auto shop = GetShopByOwnerID(pid);
		if (!shop)
			return false;

		// kicking out guests
		shop->KickAllGuests();
		_RemoveShopOffers(shop);
		#ifdef ENABLE_IKASHOP_ENTITIES
		DestroyShopEntity(shop);
		#endif
		shop->Clear();

		// searching character
		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(pid))
		{
			ch->SetIkarusShop(nullptr);
			SendShopOpenMyShopNoShopClientPacket(ch);
		}

		m_mapShops.erase(shop->GetOwnerPID());
		return true;
	}

	void CShopManager::SendShopLockBuyItemDBPacket(LPCHARACTER buyer, DWORD ownerid, ITEM_HANDLE item, long long TotalPriceSeen) //patch seen price check
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = SUBHEADER_GD_BUY_LOCK_ITEM;

		TSubPacketGDLockBuyItem subpack{};
		subpack.guestid = buyer->GetPlayerID();
		subpack.ownerid = ownerid;
		subpack.itemid = item->GetID();
		subpack.seenprice = TotalPriceSeen;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopLockedBuyItemDBPacket(DWORD buyerid, DWORD ownerid,DWORD itemid)
	{
		auto SendDBFeedback = [&](auto success){
			SendShopBuyDBPacket(buyerid, ownerid, itemid, success);
			return success;
		};

		auto shop = GetShopByOwnerID(ownerid);
		auto ch = CHARACTER_MANAGER::instance().FindByPID(buyerid);

		if (!ch || !shop)
			return SendDBFeedback(false);

		auto item = shop->GetItem(itemid);
		if (!item || !item->CanBuy(ch))
			return SendDBFeedback(false);

		const auto& price = item->GetPrice();

		ch->GoldChange(-price.yang);

		return SendDBFeedback(true);
	}

	bool CShopManager::RecvShopExpiredDBPacket(DWORD pid)
	{
		auto shop = GetShopByOwnerID(pid);
		if (!shop)
			return false;

		// kicking out guests
		shop->KickAllGuests();
		_RemoveShopOffers(shop);

		#ifdef ENABLE_IKASHOP_ENTITIES
		DestroyShopEntity(shop);
		#endif

		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(pid))
			ch->SetIkarusShop(NULL);

		shop->Clear();
		m_mapShops.erase(m_mapShops.find(shop->GetOwnerPID()));
		return true;
	}

	void CShopManager::SendShopCreateNewDBPacket(const TShopInfo& shop)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_SHOP_CREATE_NEW;

		TSubPacketGDShopCreateNew subpack{};
		subpack.shop = shop;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopCreateNewDBPacket(const TShopInfo& info)
	{
		// checking shop already exists
		if (m_mapShops.contains(info.ownerid))
			return false;

		// making shop
		auto shop = PutsNewShop(info);

		if (auto owner = shop->FindOwnerCharacter())
		{
			owner->SetIkarusShop(shop);
			SendShopOpenMyShopClientPacket(owner);
		}

		m_mapShops[info.ownerid] = shop;
		return true;
	}

	void CShopManager::SendShopChangeNameDBPacket(DWORD ownerid, const char* name)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_SHOP_CHANGE_NAME;

		TSubPacketGDShopChangeName subpack{};
		subpack.ownerid	= ownerid;
		strncpy(subpack.name, name, sizeof(subpack.name));

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopChangeNameDBPacket(DWORD ownerid, const char* name)
	{
		auto shop = GetShopByOwnerID(ownerid);
		if (!shop)
			return false;

		shop->SetName(name);
		shop->RefreshToOwner();

		return true;
	}

	//OFFER
	void CShopManager::SendShopOfferNewDBPacket(const TOfferInfo& offer, ITEM_HANDLE item)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_OFFER_CREATE;

		TSubPacketGDOfferCreate subpack{};
		subpack.ownerid = offer.ownerid;
		subpack.itemid = offer.itemid;
		subpack.offer = offer;
		subpack.vnum = item->GetInfo().vnum;
		subpack.count = item->GetInfo().count;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopOfferNewDBPacket(const TOfferInfo& offer)
	{
		// searching shop
		auto shop = GetShopByOwnerID(offer.ownerid);
		if (!shop)
			return false;

		// making shared offer
		auto soffer = std::make_shared<TOfferInfo>(offer);

		// registering offer into shop

		if (!shop->AddOffer(soffer))
			return false;

		// registering offer into map
		if (!PutsNewOffer(soffer))
			return false;

		// notify buyer
		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(offer.buyerid))
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_PRIVATE_OFFER_CREATED");

		// updating counters
		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(offer.ownerid))
			if (ch->IsLookingShopOwner())
				SendBoardCountersClientPacket(ch);

		return true;
	}

	void CShopManager::SendShopOfferNotifiedDBPacket(DWORD offerid, DWORD ownerid)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_OFFER_NOTIFIED;

		TSubPacketGDOfferNotified subpack{};
		subpack.offerid	= offerid;
		subpack.ownerid	= ownerid;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopOfferNotifiedDBPacket(DWORD offerid, DWORD ownerid)
	{
		// searching shop
		auto shop = GetShopByOwnerID(ownerid);
		if (!shop)
			return false;

		// updating offer
		for (auto& iter : shop->GetOffers())
		{
			auto& offer = iter.second;

			if (offer->offerid == offerid)
			{				
				offer->notified = true;
				break;
			}
		}

		return true;
	}

	void CShopManager::SendShopOfferAcceptDBPacket(const TOfferInfo& offer)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_OFFER_ACCEPT;

		TSubPacketGDOfferNotified subpack{};
		subpack.ownerid = offer.ownerid;
		subpack.offerid = offer.offerid;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopOfferCancelDBPacket(const TOfferInfo& offer, DWORD requester)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_OFFER_CANCEL;

		TSubPacketGDOfferCancel subpack{};
		subpack.ownerid = offer.ownerid;
		subpack.offerid = offer.offerid;
		subpack.requester = requester;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopOfferCancelDBPacket(DWORD offerid, DWORD ownerid, bool removing) //ikashop-updated 05/08/19
	{
		// searching offer handle
		auto it = m_mapOffer.find(offerid);

		// checking it's found
		if (it == m_mapOffer.end())
			return false;

		// copying handle
		auto currentOffer = it->second;

		// removing from map offer by id
		m_mapOffer.erase(it);

		// removing offer from shop
		if (auto shop = GetShopByOwnerID(ownerid))
			shop->RemoveOffer(offerid);

		// removing offer from map by buyer
		const auto buyiter = m_mapOffersByBuyer.find(currentOffer->buyerid);
		if (buyiter != m_mapOffersByBuyer.end())
		{
			auto& vec = buyiter->second;
			std::erase_if (vec, [&](auto&& offer){ return offer->offerid == offerid; });

			// removing from map if empty
			if (buyiter->second.empty())
				m_mapOffersByBuyer.erase(buyiter);
		}

		// refreshing owner view
		if (!removing)
		{
			if (auto chOwner = CHARACTER_MANAGER::Instance().FindByPID(ownerid)){
				if (chOwner->IsLookingIkarusShopOfferList())
					RecvOfferListRequestPacket(chOwner);
				// updating counters
				if (chOwner->IsLookingShopOwner())
					SendBoardCountersClientPacket(chOwner);
			}
		}

		// refreshing buyer view
		if (auto chBuyer = CHARACTER_MANAGER::Instance().FindByPID(currentOffer->buyerid))
			if (chBuyer->IsLookingIkarusShopOfferList())
				RecvOfferListRequestPacket(chBuyer);

		return true;
	}

	bool CShopManager::RecvShopOfferAcceptDBPacket(DWORD offerid, DWORD ownerid)
	{
		// searching shop
		auto shop = GetShopByOwnerID(ownerid);
		if (!shop)
			return false;

		// searching offer handle
		auto it = m_mapOffer.find(offerid);
		if (it == m_mapOffer.end())
			return false;

		// copying handle & removing from map
		auto offer = it->second;
		m_mapOffer.erase(it);

		// accepting the offer		
		shop->AcceptOffer(offer);

		//removing offer from offer by buyer
		auto buyIter = m_mapOffersByBuyer.find(offer->buyerid);
		if (buyIter != m_mapOffersByBuyer.end())
		{
			// removing using find which interrupts the search at first match
			auto& offerVec = buyIter->second;
			auto offerIt = std::find_if (offerVec.begin(), offerVec.end(), [&](auto&& current) { return current->offerid == offerid; });
			if (offerIt != offerVec.end())
				offerVec.erase(offerIt);

			// removing from map if vector is empty
			if (offerVec.empty())
				m_mapOffersByBuyer.erase(buyIter);
		}
		
		// notifying to buyer
		if (auto chBuyer = CHARACTER_MANAGER::Instance().FindByPID(offer->buyerid))
			if (chBuyer->IsLookingIkarusShopOfferList())
				RecvOfferListRequestPacket(chBuyer);

		// notifying to owner
		if (auto chOwner = CHARACTER_MANAGER::Instance().FindByPID(ownerid)){
			if (chOwner->IsLookingIkarusShopOfferList())
				RecvOfferListRequestPacket(chOwner);
			// updating counters
			if (chOwner->IsLookingShopOwner())
				SendBoardCountersClientPacket(chOwner);
		}

		return true;
	}

	bool CShopManager::RecvShopSafeboxAddItemDBPacket(DWORD ownerid, DWORD itemid, const TShopPlayerItem& item)
	{
		auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid);
		auto safebox = ch && ch->GetIkarusShopSafebox() ? ch->GetIkarusShopSafebox() : GetShopSafeboxByOwnerID(ownerid);
		if (!safebox)
			return false;

		safebox->AddItem(item);

		if (ch && ch->GetIkarusShopSafebox() && ch->GetDesc())
		{
			if (auto insertedItem = safebox->GetItem(itemid))
			{
				TSubPacketGCShoppingSafeboxAddItem subpack{};
				subpack.item = static_cast<const TShopPlayerItem&>(insertedItem->GetInfo());

				TPacketGCNewIkarusShop pack{};
				pack.header = HEADER_GC_NEW_OFFLINESHOP;
				pack.size = sizeof(pack) + sizeof(subpack);
				pack.subheader = SUBHEADER_GC_SHOP_SAFEBOX_ADD_ITEM;

				ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
				ch->GetDesc()->Packet(&subpack, sizeof(subpack));
			}
		}

		if (ch && ch->IsLookingShopOwner())
			SendBoardCountersClientPacket(ch);

		return true;
	}

	bool CShopManager::SendShopSafeboxAddItemDBPacket(DWORD ownerid, const CShopItem& item) {
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = SUBHEADER_GD_SAFEBOX_ADD_ITEM;


		TSubPacketGDSafeboxAddItem subpack{};
		subpack.ownerid = ownerid;
		subpack.itemid = item.GetID();
		subpack.item = static_cast<const TShopPlayerItem&>(item.GetInfo());

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
		return true;
	}

	bool CShopManager::RecvShopSafeboxAddValutesDBPacket(DWORD ownerid, const TValutesInfo& valute)
	{
		auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid);
		auto safebox = ch && ch->GetIkarusShopSafebox() ? ch->GetIkarusShopSafebox() : GetShopSafeboxByOwnerID(ownerid);

		if (!safebox)
			return false;

		safebox->AddValute(valute);
		if (ch && ch->GetIkarusShopSafebox())
			safebox->RefreshToOwner(ch);

		if (ch && ch->IsLookingShopOwner())
			SendBoardCountersClientPacket(ch);

		return true;
	}

	void CShopManager::SendShopSafeboxGetItemDBPacket(DWORD ownerid, DWORD itemid)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_SAFEBOX_GET_ITEM;

		TSubPacketGDSafeboxGetItem subpack{};
		subpack.ownerid	= ownerid;
		subpack.itemid	= itemid;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopSafeboxGetValutesDBPacket(DWORD ownerid, const TValutesInfo& valutes)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_SAFEBOX_GET_VALUTES;

		TSubPacketGDSafeboxGetValutes subpack{};
		subpack.ownerid	= ownerid;
		subpack.valute = valutes;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopSafeboxLoadDBPacket(DWORD ownerid, const TValutesInfo& valute, const std::vector<TShopPlayerItem>& items)
	{
		// resetting/inserting safebox
		auto& safebox = m_mapSafeboxs[ownerid] = std::make_shared<CShopSafebox>();
		safebox->SetValuteAmount(valute);

		// adding items
		for (auto& item : items)
			safebox->AddItem(item);

		return true;
	}

	bool CShopManager::RecvShopSafeboxExpiredItemDBPacket(DWORD ownerid, DWORD itemid) {
		if (auto data = GetShopSafeboxByOwnerID(ownerid)) 
		{
			data->RemoveItem(itemid);
			data->RefreshToOwner();
			// updating counters
			if (auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid))
				if (ch->IsLookingShopOwner())
					SendBoardCountersClientPacket(ch);
		}

		return true;
	}

	bool CShopManager::RecvShopSafeboxGetItemConfirm(DWORD ownerid, DWORD itemid)
	{
		auto _SendFeedbackToDB = [&](auto result)
		{
			TPacketGDNewIkarusShop pack{};
			pack.bSubHeader = ikashop::SUBHEADER_GD_SAFEBOX_GET_TIME_FEEDBACK;

			ikashop::TSubPacketGDSafeboxGetItemFeedback subpack{};
			subpack.ownerid = ownerid;
			subpack.itemid = itemid;
			subpack.result = result;

			TEMP_BUFFER buffer;
			buffer.write(&pack, sizeof(pack));
			buffer.write(&subpack, sizeof(subpack));
			db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buffer.read_peek(), buffer.size());
			return result;
		};

		// searching safebox
		auto safebox = GetShopSafeboxByOwnerID(ownerid);
		if (!safebox)
			return _SendFeedbackToDB(false);

		// searching item
		auto item = safebox->GetItem(itemid);
		if (!item)
			return _SendFeedbackToDB(false);

		// searching character
		auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid);
		if (!ch)
			return _SendFeedbackToDB(false);

		// creating item
		auto itemInstance = item->CreateItem();
		if (!itemInstance)
			return _SendFeedbackToDB(false);

		// checking item can be moved to inventory
		static TItemPos pos{};
		if (!ch->CanTakeInventoryItem(itemInstance, &pos))
		{
			itemInstance->SetSkipSave(true);
			M2_DESTROY_ITEM(itemInstance);
			return _SendFeedbackToDB(false);
		}

		// giving item to character
		if (!safebox->RemoveItem(itemid))
		{
			itemInstance->SetSkipSave(true);
			M2_DESTROY_ITEM(itemInstance);
			return _SendFeedbackToDB(false);
		}

		if (ch->IsLookingShopOwner())
			SendBoardCountersClientPacket(ch);

		// refreshing to owner
		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid))
		{
			TPacketGCNewIkarusShop pack{};
			pack.header = HEADER_GC_NEW_OFFLINESHOP;
			pack.subheader = SUBHEADER_GC_SHOP_SAFEBOX_REMOVE_ITEM;
			pack.size = sizeof(pack) + sizeof(TSubPacketGCShopSafeboxRemoveItem);

			TSubPacketGCShopSafeboxRemoveItem subpack{};
			subpack.itemid = itemid;

			if (ch->GetDesc())
			{
				ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
				ch->GetDesc()->Packet(&subpack, sizeof(subpack));
			}
		}

		itemInstance->AddToCharacter(ch, pos);
		itemInstance->SetSkipSave(false);
		itemInstance->Save();
		ITEM_MANAGER::Instance().FlushDelayedSave(itemInstance);
		return _SendFeedbackToDB(true);
	}

	//AUCTION
	void CShopManager::SendAuctionCreateDBPacket(const TAuctionInfo& auction)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = SUBHEADER_GD_AUCTION_CREATE;

		TSubPacketGDAuctionCreate subpack{};
		subpack.auction = auction;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP , 0 , buff.read_peek() , buff.size());
	}

	void CShopManager::SendAuctionAddOfferDBPacket(const TAuctionOfferInfo& offer)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = SUBHEADER_GD_AUCTION_ADD_OFFER;

		TSubPacketGDAuctionAddOffer subpack{};
		subpack.offer = offer;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP , 0 , buff.read_peek() , buff.size());
	}

	bool CShopManager::RecvAuctionCreateDBPacket(const TAuctionInfo& auction)
	{
		//check if exist
		if (m_mapAuctions.find(auction.owner) != m_mapAuctions.end())
			return false;

		//set info
		auto& obj = m_mapAuctions[auction.owner] = std::make_shared<CAuction>();
		obj->SetInfo(auction);


		//check about owner
		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(auction.owner))
		{
			ch->SetIkarusAuction(obj);
			SendAuctionOpenAuctionClientPacket(ch, *obj);
		}

		return true;
	}

	bool CShopManager::RecvAuctionAddOfferDBPacket(const TAuctionOfferInfo& offer)
	{
		//check if exists
		auto auction = GetAuctionByOwnerID(offer.ownerid);
		if (!auction)
			return false;

		//adding offer
		auction->AddOffer(offer);

		//adding 1 minute time when new offer is done in the last minute
		if (auction->GetInfo().duration == 0)
			auction->IncreaseDuration();

		// updating counters
		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(offer.ownerid))
			if (ch->IsLookingShopOwner())
				SendBoardCountersClientPacket(ch);

		return true;
	}

	//updated 30/09/19
	bool CShopManager::RecvAuctionExpiredDBPacket(DWORD id)
	{
		// searching auction inside map
		auto it = m_mapAuctions.find(id);
		if (it == m_mapAuctions.end())
			return false;


		// copying guest list & removing auction from auctionlist
		auto guests{ it->second->GetGuests() };
		m_mapAuctions.erase(it);

		// kicking auction guests
		for (auto& guest : guests){
			if (auto chGuest = CHARACTER_MANAGER::instance().FindByPID(guest))
			{
				chGuest->SetIkarusAuctionGuest(NULL);
				RecvAuctionListRequestClientPacket(chGuest);
			}
		}

		// setting auction to null on owner character
		if (auto owner = CHARACTER_MANAGER::instance().FindByPID(id)){
			owner->SetIkarusAuction(NULL);
			// updating counters
			if (owner->IsLookingShopOwner())
				SendBoardCountersClientPacket(owner);
		}


		return true;
	}

	bool CShopManager::RecvAuctionOfferSeenDBPacket(DWORD ownerid, DWORD buyerid, long long price)
	{
		if (auto auction = GetAuctionByOwnerID(ownerid))
		{
			for(auto& offer : auction->GetOffers())
			{
				if (offer->buyerid == buyerid && offer->ownerid == ownerid && offer->price.yang == price){
					offer->seenflag = true;
					break;
				}
			}

			if (auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid))
				if (ch->IsLookingShopOwner())
					SendBoardCountersClientPacket(ch);
		}

		return true;
	}

	#ifdef SHOP_DISTANCE
	#include "utils.h"

	struct FFindNearlyShop
	{
		bool bIsExist;
		LPCHARACTER pChar;

		FFindNearlyShop(LPCHARACTER pkChar) : pChar(pkChar)
		{
			bIsExist = false;
		}

		void operator()(LPENTITY ent)
		{
			if (!ent->IsType(ENTITY_NEWSHOPS))
				return;

			if (bIsExist)
				return;

			LPCHARACTER pMe = static_cast<LPCHARACTER>(ent);
			if (DISTANCE_APPROX(pChar->GetX() - pMe->GetX(), pChar->GetY() - pMe->GetY()) < 200)
				bIsExist = true;
		}
	};
	#endif

	//client packets exchanging
	bool CShopManager::RecvShopCreateNewClientPacket(LPCHARACTER ch, const char* szTitle)
	{
		if (!ch)
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_CREATE_NEW))
			return false;

		if (!CheckCharacterActions(ch))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_DO_NOW");
			return true;
		}

		// if (ch->IsExchangeTime())
		if (ch->IsWarpStatus())
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_DO_NOW");
			return true;
		}

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		if (ch->GetGold() < OFFLINESHOP_RESTORE_DURATION_COST)
			return false;

		#ifdef SHOP_ONLY_CHANNEL1
		if (g_bChannel != 1)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CHANNEL_ONE");
			return false;
		}
		#endif

		#ifdef ENABLE_IKASHOP_ENTITIES
		if (!ch->GetIkarusShop() && !ALLOWED_SPAWN_MAPS.contains(ch->GetMapIndex()))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_CREATE_SHOP_ON_WRONG_PLACE");
			return false;
		}
		#endif

		#ifdef SHOP_DISTANCE
		LPSECTREE pSec = ch->GetSectree();
		if (pSec)
		{
			FFindNearlyShop f(ch);
			pSec->ForEachAround(f);
			if (f.bIsExist)
			{
				SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_CREATE_SHOP_ON_WRONG_DISTANCE");
				return false;
			}
		}
		#endif

		ch->GoldChange(-static_cast<int>(OFFLINESHOP_RESTORE_DURATION_COST));

		ikashop::TShopInfo shopinfo{};
		shopinfo.ownerid = ch->GetPlayerID();
		shopinfo.duration = OFFLINESHOP_DURATION_MAX_MINUTES;

		//cheching about bandword
		if (CBanwordManager::instance().CheckString(szTitle, strlen(szTitle)))
			return false;

		static char szNameChecked[OFFLINE_SHOP_NAME_MAX_LEN * 2 + 1];
		DBManager::instance().EscapeString(szNameChecked, sizeof(szNameChecked), szTitle, strlen(szTitle));

		//making full name
		snprintf(shopinfo.name, sizeof(shopinfo.name), "%s@%s", ch->GetName(), szNameChecked);

		#ifdef ENABLE_IKASHOP_ENTITIES
		shopinfo.spawn.map = ch->GetMapIndex();
		shopinfo.spawn.x = ch->GetX();
		shopinfo.spawn.y = ch->GetY();
		shopinfo.spawn.bChannel = g_bChannel;
		#endif

		#ifdef ENABLE_BATTLE_PASS
		uint8_t bBattlePassId = ch->GetBattlePassId();
		if (bBattlePassId)
		{
			CBattlePass::instance().BattlePassCheckMissions(ch, bBattlePassId, BP_MT_OFFLINE_SHOP);
		}
		#endif

		SendShopCreateNewDBPacket(shopinfo);
		return true;
	}

	bool CShopManager::RecvShopForceCloseClientPacket(LPCHARACTER ch)
	{
		if (!ch || !ch->GetIkarusShop())
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_FORCE_CLOSE_SHOP))
			return false;

		if (!CheckSafeboxSize(ch))
			return false;

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		// requesting of remove item to each item
		auto shop = ch->GetIkarusShop();
		for(auto& [id, item] : shop->GetItems())
			SendShopRemoveItemDBPacket(ch->GetPlayerID(), id);

		SendShopForceCloseDBPacket(ch->GetPlayerID());
		return true;
	}

	bool CShopManager::RecvShopOpenClientPacket(LPCHARACTER ch, DWORD ownerid)
	{
		if (!ch || !ch->GetDesc())
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_OPEN_SHOP))
			return false;

		auto shop = GetShopByOwnerID(ownerid);
		if (!shop)
			return false;

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		// skipping owner
		if (ch->GetPlayerID() == ownerid)
		{
			// sending board counters
			SendBoardCountersClientPacket(ch);
			GetManager().SendShopOpenMyShopClientPacket(ch);
			return true;
		}

		ch->SetIkarusShopGuest(shop);
		SendShopOpenClientPacket(ch, shop.get());
		return true;
	}

	bool CShopManager::RecvShopOpenMyShopClientPacket(LPCHARACTER ch)
	{
		if (!ch || !ch->GetDesc())
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_OPEN_MY_SHOP))
			return false;

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		// sending board counters
		SendBoardCountersClientPacket(ch);

		if (!ch->GetIkarusShop())
			SendShopOpenMyShopNoShopClientPacket(ch);
		else
			SendShopOpenMyShopClientPacket(ch);

		return true;
	}

	bool CShopManager::RecvShopBuyItemClientPacket(LPCHARACTER ch, DWORD ownerid, DWORD itemid, bool searching, long long seenprice)  //patch seen price check
	{
		if (!ch || !CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_BUY_ITEM))
			return false;

		if (!CheckCharacterActions(ch))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_DO_NOW");
			return true;
		}

		// if (ch->IsExchangeTime())
		if (ch->IsWarpStatus())
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_DO_NOW");
			return true;
		}

		// searching shop
		SHOP_HANDLE shop = GetShopByOwnerID(ownerid);
		if (!shop)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_GENERIC_ERROR");
			return false;
		}

		// searching item
		auto pitem = shop->GetItem(itemid);
		if (!pitem)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_ITEM_NOT_AVAILABLE");
			SendSearchResultDeleteClientPacket(ch, itemid);
			return false;
		}

		if (!pitem->CanBuy(ch))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_BUY_NOT_ENOUGH_MONEY");
			return false;
		}

		//patch seen price check
		if (pitem->GetPrice().GetTotalYangAmount() != seenprice)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_BUY_PRICE_CHANGED");
			return false;
		}

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		//patch seen price check
		SendShopLockBuyItemDBPacket(ch, ownerid, pitem, seenprice);
		return true;
	}

	#ifdef ENABLE_IKASHOP_ENTITIES
	bool CShopManager::RecvShopClickEntityClientPacket(LPCHARACTER ch, DWORD evid)
	{
		auto iter = m_entityByVID.find(evid);
		if (iter != m_entityByVID.end())
		{
			auto entity = iter->second;
			RecvShopOpenClientPacket(ch, entity->GetShop()->GetOwnerPID());
		}
		return true;
	}
	#endif

	void CShopManager::SendSearchResultDeleteClientPacket(LPCHARACTER ch, DWORD itemid)
	{
		if (!ch || !ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack;
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.size = sizeof(pack) + sizeof(DWORD);
		pack.subheader = ikashop::SUBHEADER_GC_SEARCH_RESULT_DELETE;

		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&itemid, sizeof(itemid));
	}

	void CShopManager::SendShopOpenClientPacket(LPCHARACTER ch, ikashop::CShop* shop)
	{
		if (!ch || !ch->GetDesc())
			return;

		const auto& items = shop->GetItems();

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader	= SUBHEADER_GC_SHOP_OPEN;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopOpen) + sizeof(TShopItem)* items.size();

		TSubPacketGCShopOpen subpack{};
		subpack.shop.count = shop->GetItems().size();
		subpack.shop.duration = shop->GetDuration();
		subpack.shop.ownerid = shop->GetOwnerPID();
		str_to_cstring(subpack.shop.name, shop->GetName());

		// ch->ChatPacket(1, "subpack %s shop %s", subpack.shop.name, shop->GetName());

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		for (auto& [id, item] : items)
		{
			auto& itemInfo = item->GetInfo();
			buff.write(&itemInfo, sizeof(itemInfo));
		}

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopOpenMyShopNoShopClientPacket(LPCHARACTER ch)
	{
		if (!ch || !ch->GetDesc())
			return;

		ch->SetLookingShopOwner(false);

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader	= SUBHEADER_GC_SHOP_OPEN_OWNER_NO_SHOP;
		pack.size = sizeof(pack);

		ch->GetDesc()->Packet(&pack, sizeof(pack));
	}

	void CShopManager::SendShopExpiredGuesting(DWORD ownerid, LPCHARACTER ch)
	{
		if (!ch || !ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_SHOP_EXPIRED_GUESTING;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopExpiredGuesting);

		TSubPacketGCShopExpiredGuesting subpack{};
		subpack.ownerid = ownerid;

		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&subpack, sizeof(subpack));
	}

	void CShopManager::SendShopOpenMyShopClientPacket(LPCHARACTER ch)
	{
		// validating character and shop
		if (!ch || !ch->GetDesc() || !ch->GetIkarusShop())
			return;

		// storing looking state
		ch->SetLookingShopOwner(true);

		// getting shop
		auto shop = ch->GetIkarusShop();
		auto owner = ch->GetPlayerID();

		const auto& items = shop->GetItems();

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_SHOP_OPEN_OWNER;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopOpenOwner) + sizeof(TShopItem) * items.size();

		TSubPacketGCShopOpenOwner subpack{};
		subpack.shop.count = items.size();
		subpack.shop.duration = shop->GetDuration();
		subpack.shop.ownerid = owner;
		str_to_cstring(subpack.shop.name, shop->GetName());

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		for (auto& [id, item] : items)
		{
			auto& itemInfo = item->GetInfo();
			buff.write(&itemInfo, sizeof(itemInfo));
		}

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopForceClosedClientPacket(DWORD ownerid)
	{
		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid))
		{
			if (ch->GetDesc())
			{
				TPacketGCNewIkarusShop pack{};
				pack.header = HEADER_GC_NEW_OFFLINESHOP;
				pack.size = sizeof(pack);
				pack.subheader = SUBHEADER_GC_SHOP_OPEN_OWNER_NO_SHOP;
				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
		}
	}

	//ITEMS
	bool CShopManager::RecvShopAddItemClientPacket(LPCHARACTER ch, const TItemPos& pos, const TPriceInfo& price, int destPos)
	{
		if (!ch || !ch->GetIkarusShop())
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_ADD_ITEM))
			return false;

		if (!CheckSafeboxSize(ch))
			return false;

		if (!CheckCharacterActions(ch))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_DO_NOW");
			return true;
		}

		// if (ch->IsExchangeTime())
		if (ch->IsWarpStatus())
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_DO_NOW");
			return true;
		}

		if (ch->GetIkarusShop()->GetOwnerPID() != ch->GetPlayerID())
		{
			sys_err("owner does not match %u, %u", ch->GetIkarusShop()->GetOwnerPID(), ch->GetPlayerID());
			return false;
		}

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		auto item = ch->GetItem(pos);
		if (!item)
			return false;

		if (item->isLocked() || item->IsEquipped() || item->IsExchanging())
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_ADD_USED_ITEM");
			return true;
		}

		#ifdef ENABLE_SOULBIND_SYSTEM
		if (item->IsSealed())
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_ADD_SEALED_ITEM");
			return true;
		}
		#endif

		#ifdef ENABLE_LOCKED_SYSTEM
		if (item->IsItemLocked())
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_ADD_SEALED_ITEM");
			return true;
		}
		#endif

		if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE) || IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_MYSHOP))
			return false;

		if (!IsGoodSalePrice(price))
			return false;

		// checking space
		if (!ch->GetIkarusShop()->ReserveSpace(destPos, item->GetSize()))
			return false;

		TShopItem sitem{};
		sitem.pos = destPos;
		sitem.id = item->GetID();
		sitem.owner = ch->GetPlayerID();
		sitem.vnum = item->GetVnum();
		sitem.count = item->GetCount();
		sitem.expiration = GetItemExpiration(item);
		sitem.price = price;

		memcpy(sitem.aAttr, item->GetAttributes(), sizeof(sitem.aAttr));
		memcpy(sitem.alSockets, item->GetSockets(), sizeof(sitem.alSockets));

		#ifdef ENABLE_EVOLUTION_SYSTEM
		sitem.evolution = item->GetEvolution();
		#endif
		#ifdef ENABLE_CHANGELOOK_SYSTEM
		sitem.transmutation = item->GetTransmutation();
		#endif
		#ifdef ENABLE_LOCKED_SYSTEM
		sitem.locked = item->GetLocked();
		#endif

		MoveAndDestroyInstance(item, IKASHOP_OFFLINESHOP, destPos);
		SendShopAddItemDBPacket(ch->GetPlayerID(), sitem);
		return true;
	}

	bool CShopManager::RecvShopRemoveItemClientPacket(LPCHARACTER ch, DWORD itemid)
	{
		if (!ch || !ch->GetIkarusShop())
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_REMOVE_ITEM))
			return false;

		if (!CheckSafeboxSize(ch))
			return false;

		// checking if it is the last item
		auto shop = ch->GetIkarusShop();

		// checking shop has the item
		if (!shop->GetItem(itemid))
			return false;

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		SendShopRemoveItemDBPacket(shop->GetOwnerPID(), itemid);
		return true;
	}

	bool CShopManager::RecvShopEditItemDBPacket(DWORD ownerid, DWORD itemid, const TPriceInfo& price)
	{
		auto shop = GetShopByOwnerID(ownerid);
		if (!shop)
			return false;

		shop->ModifyItemPrice(itemid, price);
		return true;
	}

	//FILTER
	bool CShopManager::RecvShopFilterRequestClientPacket(LPCHARACTER ch, const TFilterInfo& filter)
	{
		if (!ch)
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_FILTER_REQUEST))
			return false;

		if (!CheckSearchCooldown(ch->GetPlayerID()))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_SEARCH_COOLDOWN_STILL_ACTIVED");
			return true;
		}

		if (ch->FindSpecifyItem(60004) == NULL && ch->FindSpecifyItem(60005) == NULL/* && ch->FindSpecifyItem(60006) == NULL*/)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_SEARCH_CANNOT_ITEM");
			return true;
		}

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		std::vector<TSearchResultInfo> matches;
		const auto filtername = StringToLower(filter.name, strnlen(filter.name, sizeof(filter.name)));

		for (auto& [id, auction] : m_mapAuctions)
		{
			// skipping my own items
			if (auction->GetInfo().owner == ch->GetPlayerID())
				continue;

			/*if (!ch->IsPremiumSearch())
			{
				auto& spawn = auction->GetSpawn();
				if (spawn.bChannel != g_bChannel)
					continue;

				if (!ALLOWED_SPAWN_MAPS.contains(ch->GetMapIndex()))
					continue;
			}*/

			const auto& info = auction->GetInfo();
			if (!MatchFilter(info, filter, filtername))
				continue;

			auto& result = matches.emplace_back(DerivedFromBase<TSearchResultInfo, TShopItem>(info));
			result.auction = true;
			result.duration = auction->GetInfo().duration;
			str_to_cstring(result.seller_name, auction->GetInfo().ownername);
		}

		for (auto& [id, shop] : m_mapShops)
		{
			// skipping my own items
			if (shop->GetOwnerPID() == ch->GetPlayerID())
				continue;

			if (!ch->IsPremiumSearch())
			{
				auto& spawn = shop->GetSpawn();
				if (spawn.bChannel != g_bChannel) 
					continue;

				if (!ALLOWED_SPAWN_MAPS.contains(ch->GetMapIndex()))
					continue;
			}

			for (auto& [id, sitem] : shop->GetItems())
			{
				const auto& info = sitem->GetInfo();
				if (!MatchFilter(info, filter, filtername))
					continue;

				auto& result = matches.emplace_back(DerivedFromBase<TSearchResultInfo>(info));
				result.auction = false;
				result.duration = shop->GetDuration();

				std::string shopOwnerName = shop->GetName();
				shopOwnerName = shopOwnerName.substr(0, shopOwnerName.find("@"));
				str_to_cstring(result.seller_name, shopOwnerName.c_str());

				if (matches.size() >= OFFLINESHOP_MAX_SEARCH_RESULT)
					break;
			}

			if (matches.size() >= OFFLINESHOP_MAX_SEARCH_RESULT)
				break;
		}

		SendShopFilterResultClientPacket(ch, matches);
		return true;
	}

	#ifdef ENABLE_SHOP_SEARCH_SYSTEM
	bool CShopManager::RecvShopFilterNameRequestClientPacket(LPCHARACTER ch, const TFilterName& filter)
	{
		if (!ch)
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_FILTER_REQUEST))
			return false;

		if (!CheckSearchCooldown(ch->GetPlayerID()))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_SEARCH_COOLDOWN_STILL_ACTIVED");
			return true;
		}

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		std::vector<TSearchNameResultInfo> matches;
		const auto filtername = StringToLower(filter.szName, strnlen(filter.szName, sizeof(filter.szName)));

		for (auto& [id, shop] : m_mapShops)
		{
			// skipping my own items
			if (shop->GetOwnerPID() == ch->GetPlayerID())
				continue;

			if (!ch->IsPremiumSearch())
			{
				auto& spawn = shop->GetSpawn();
				if (spawn.bChannel != g_bChannel) 
					continue;

				if (!ALLOWED_SPAWN_MAPS.contains(ch->GetMapIndex()))
					continue;
			}

			for (auto& [id, sitem] : shop->GetItems())
			{
				const auto& info = sitem->GetInfo();
				if (!MatchFilterSearch(info, filter, filtername))
					continue;

				TSearchNameResultInfo searchInfo;
				searchInfo.dwOwnerID = shop->GetOwnerPID();
				searchInfo.dwShopVID = shop->GetShopVID();
				matches.emplace_back(DerivedFromBase<TSearchNameResultInfo>(searchInfo));

				if (matches.size() >= OFFLINESHOP_MAX_SEARCH_RESULT)
					break;
			}

			if (matches.size() >= OFFLINESHOP_MAX_SEARCH_RESULT)
				break;
		}

		SendShopFilterNameResultClientPacket(ch, matches);
		return true;
	}
	#endif

	bool CShopManager::RecvShopSearchFillRequestClientPacket(LPCHARACTER ch)
	{
		if (!ch)
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_FILTER_REQUEST))
			return false;

		if (!CheckSearchCooldown(ch->GetPlayerID()))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_SEARCH_COOLDOWN_STILL_ACTIVED");
			return true;
		}

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		std::vector<TSearchResultInfo> matches;
		std::set<DWORD> matchedIds;

		for (int i = 0; i < 20; i++)
		{
			const auto shopCount = m_mapShops.size();
			if (shopCount == 0)
				break;

			auto shopIter = std::next(m_mapShops.begin(), number(0, shopCount-1));
			auto& shop = shopIter->second;

			// skipping my own items
			if (shop->GetOwnerPID() == ch->GetPlayerID())
				continue;

			if (!ch->IsPremiumSearch())
			{
				auto& spawn = shop->GetSpawn();
				if (spawn.bChannel != g_bChannel)
					continue;

				if (!ALLOWED_SPAWN_MAPS.contains(ch->GetMapIndex()))
					continue;
			}

			auto& items = shop->GetItems();
			const auto itemCount = items.size();
			if (itemCount == 0)
				continue;

			auto itemIter = std::next(items.begin(), number(0, itemCount-1));
			auto& item = itemIter->second;

			if (matchedIds.contains(item->GetInfo().id))
				continue;

			const auto& info = item->GetInfo();
			auto& result = matches.emplace_back(DerivedFromBase<TSearchResultInfo>(info));
			result.auction = false;
			result.duration = shop->GetDuration();

			std::string shopOwnerName = shop->GetName();
			shopOwnerName = shopOwnerName.substr(0, shopOwnerName.find("@"));
			str_to_cstring(result.seller_name, shopOwnerName.c_str());

			matchedIds.emplace(info.id);
		}

		for(int i = 0; i < 5; i++)
		{
			const auto auctionCount = m_mapAuctions.size();
			if (auctionCount == 0)
				break;

			auto auctionIter = std::next(m_mapAuctions.begin(), number(0, auctionCount - 1));
			auto& auction = auctionIter->second;

			// skipping my own items
			if (auction->GetInfo().owner == ch->GetPlayerID())
				continue;

			/*if (!ch->IsPremiumSearch())
			{
				auto& spawn = auction->GetSpawn();
				if (spawn.bChannel != g_bChannel)
					continue;

				if (!ALLOWED_SPAWN_MAPS.contains(ch->GetMapIndex()))
					continue;
			}*/

			auto bestoff = auction->GetBestOffer();

			if (matchedIds.contains(auction->GetInfo().id))
				continue;

			const auto& info = auction->GetInfo();
			auto& result = matches.emplace_back(DerivedFromBase<TSearchResultInfo, TShopItem>(info));
			result.auction = true;
			result.price.yang = GetAuctionMinRaise(auction);
			result.duration = auction->GetInfo().duration;
			str_to_cstring(result.seller_name, auction->GetInfo().ownername);
			matchedIds.emplace(info.id);
		}

		SendShopFilterResultClientPacket(ch, matches);
		return true;
	}

	void CShopManager::SendShopFilterResultClientPacket(LPCHARACTER ch, const std::vector<TSearchResultInfo>& items)
	{
		if (!ch || !ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_SHOP_FILTER_RESULT;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopFilterResult) + sizeof(TSearchResultInfo)*items.size();

		TSubPacketGCShopFilterResult subpack{};
		subpack.count = items.size();

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		if (!items.empty())
			buff.write(items.data(), items.size() * sizeof(items[0]));

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	#ifdef ENABLE_SHOP_SEARCH_SYSTEM
	void CShopManager::SendShopFilterNameResultClientPacket(LPCHARACTER ch, const std::vector<TSearchNameResultInfo>& shops)
	{
		if (!ch || !ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_SHOP_FILTER_NAME_RESULT;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopFilterResult) + sizeof(TSearchNameResultInfo)*shops.size();

		TSubPacketGCShopFilterResult subpack{};
		subpack.count = shops.size();

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		if (!shops.empty())
			buff.write(shops.data(), shops.size() * sizeof(shops[0]));

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}
	#endif

	//OFFERS
	bool CShopManager::RecvShopCreateOfferClientPacket(LPCHARACTER ch, TOfferInfo offer)
	{
		if (!ch)
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!CheckSafeboxSize(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_CREATE_OFFER))
			return false;

		// checking pending offer limit
		auto offersIter = m_mapOffersByBuyer.find(ch->GetPlayerID());
		if (offersIter != m_mapOffersByBuyer.end() && offersIter->second.size() >= OFFLINESHOP_MAX_PENDING_OFFERS)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_PENDING_PRIVATE_OFFER_LIMIT");
			return false;
		}

		// checking for existing offers on this item
		if (offersIter != m_mapOffersByBuyer.end())
		{
			auto& offers = offersIter->second;
			auto iter = std::find_if (offers.begin(), offers.end(),
				[&offer](const OFFER_HANDLE& handle){ return handle->itemid == offer.itemid; });
			if (iter != offers.end())
			{
				SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_PENDING_PRIVATE_OFFER_ALREADY_EXIST");
				return false;
			}
		}

		// stopping me to offer to myself
		if (ch->GetPlayerID() == offer.ownerid)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_GENERIC_ERROR");
			return false;
		}

		// searching shop
		auto shop = GetShopByOwnerID(offer.ownerid);
		if (!shop)
			return false;

		// searching item
		auto item = shop->GetItem(offer.itemid);
		if (!item)
			return false;

		// checking price min
		if (item->GetPrice().GetTotalYangAmount() < 10000)
			return false;

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		// fix flooding offers
		if (!CheckOfferCooldown(ch->GetPlayerID()))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CREATE_OFFER_COOLDOWN_STILL_ACTIVED");
			return false;
		}

		if (!CheckOfferPrice(offer.price, item->GetPrice()))
			return false;

		if (offer.price.yang < 0)
			return false;

		if (offer.price.GetTotalYangAmount() <= 0)
			return false;

		if ((long long) ch->GetGold() < offer.price.yang)
			return false;

		ch->GoldChange(-offer.price.yang);

		offer.accepted = false;
		offer.notified = false;
		offer.buyerid = ch->GetPlayerID();

		str_to_cstring(offer.buyername, ch->GetName());

		SendShopOfferNewDBPacket(offer, item);
		return true;
	}

	bool CShopManager::RecvShopAcceptOfferClientPacket(LPCHARACTER ch, DWORD offerid)
	{
		if (!ch || !ch->GetIkarusShop())
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_ACCEPT_OFFER))
			return false;

		auto shop = ch->GetIkarusShop();
		auto& offers = shop->GetOffers();

		// searching offer
		auto offerIter = offers.find(offerid);
		if (offerIter == offers.end())
			return false;

		// chekcing item was already accepted
		auto info = offerIter->second;
		if (info->accepted)
			return false;

		// checking that accepting character is the owner of the shop
		if (ch->GetPlayerID() != info->ownerid)
			return false;

		// checking item exists
		if (!shop->GetItem(info->itemid))
			return false;

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		// forwarding to db
		info->accepted = true;
		SendShopOfferAcceptDBPacket(*info);
		return true;
	}

	bool CShopManager::RecvShopCancelOfferClientPacket(LPCHARACTER ch, DWORD offerid, DWORD ownerid)
	{
		if (!ch || !CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_CANCEL_OFFER))
			return false;

		// searching shop
		auto shop = GetShopByOwnerID(ownerid);
		if (!shop)
			return false;

		// searching offer
		auto& offers = shop->GetOffers();
		auto offerIter = offers.find(offerid);
		if (offerIter == offers.end())
			return false;

		// checking offer wasnt accepted
		auto offer = offerIter->second;
		if (offer->accepted)
			return false;

		// checking item exists
		if (!shop->GetItem(offer->itemid))
			return false;

		// checking who is rejecting the offer is the owner of the shop or the buyer
		if (ch->GetPlayerID() != shop->GetOwnerPID() && ch->GetPlayerID() != offer->buyerid)
			return false;

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		SendShopOfferCancelDBPacket(*offer, ch->GetPlayerID());
		return true;
	}

	bool CShopManager::RecvOfferListRequestPacket(LPCHARACTER ch) //ikashop-updated 03/08/19
	{
		if (!ch->GetDesc())
			return false;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_OFFER_LIST;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopOfferList);

		TSubPacketGCShopOfferList subpack{};
		subpack.offercount = 0;

		// storing offers inside a vector
		std::vector<TOfferListElement> offerlist;

		// searching offers as buyer
		auto it = m_mapOffersByBuyer.find(ch->GetPlayerID());
		if (it != m_mapOffersByBuyer.end())
		{
			auto& offers = it->second;
			for(const auto& offer : offers)
			{
				auto shop = GetShopByOwnerID(offer->ownerid);
				if (!shop)
					continue;

				auto item = shop->GetItem(offer->itemid);
				if (!item)
					continue;

				auto& listElement = offerlist.emplace_back(DerivedFromBase<TOfferListElement>(*offer));
				listElement.item = item->GetInfo();
				listElement.incoming = false;
				str_to_cstring(listElement.shopname, shop->GetName());
			}
		}

		// searching offers as seller
		if (auto shop = ch->GetIkarusShop())
		{
			for (const auto& [id, offer] : shop->GetOffers())
			{
				auto item = shop->GetItem(offer->itemid);
				if (!item)
					continue;

				auto& listElement = offerlist.emplace_back(DerivedFromBase<TOfferListElement>(*offer));
				listElement.item = item->GetInfo();
				listElement.incoming = true;
				str_to_cstring(listElement.shopname, shop->GetName());
			}
		}

		// updating pack & subpack
		pack.size += sizeof(TOfferListElement) * offerlist.size();
		subpack.offercount = offerlist.size();

		// writing bytes into a buffer
		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));
		for(auto& offer : offerlist)
			buff.write(&offer, sizeof(offer));

		// forwarding bytes
		ch->SetLookingIkarusShopOfferList(true);
		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
		return true;
	}

	//SAFEBOX
	bool CShopManager::RecvShopSafeboxOpenClientPacket(LPCHARACTER ch)
	{
		if (!ch || ch->GetIkarusShopSafebox())
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_SAFEBOX_OPEN))
			return false;

		auto safebox = GetShopSafeboxByOwnerID(ch->GetPlayerID());
		if (!safebox)
			return false;

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		ch->SetIkarusShopSafebox(safebox);
		safebox->RefreshToOwner(ch);
		return true;
	}

	bool CShopManager::RecvShopSafeboxGetItemClientPacket(LPCHARACTER ch, DWORD itemid)
	{
		if (!ch || !ch->GetIkarusShopSafebox())
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_SAFEBOX_GET_ITEM))
			return false;

		// searching safebox & item
		auto safebox = ch->GetIkarusShopSafebox();
		auto sitem = safebox->GetItem(itemid);
		if (!sitem)
			return false;

		// creating temporary item
		auto item = sitem->CreateItem();
		if (!item)
			return false;

		// checking item can be stored
		TItemPos itemPos;
		auto ret = ch->CanTakeInventoryItem(item, &itemPos);

		// destroying temporary item
		item->SetSkipSave(true);
		M2_DESTROY_ITEM(item);

		// if item can't be store returning false
		if (!ret)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_SAFEBOX_CANNOT_GET_ITEM");
			return false;
		}

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		SendShopSafeboxGetItemDBPacket(ch->GetPlayerID(), itemid);
		return true;
	}

	bool CShopManager::RecvShopSafeboxGetValutesClientPacket(LPCHARACTER ch)
	{
		if (!ch || !ch->GetIkarusShopSafebox())
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_SAFEBOX_GET_VALUTES))
			return false;

		// getting safebox
		auto safebox = ch->GetIkarusShopSafebox();
		if (!safebox)
			return false;

		const auto& ownedValute = safebox->GetValutes();
		auto claimingValute = ownedValute;

		#ifdef ENABLE_FULL_YANG
		if (claimingValute.yang + static_cast<int64_t>(ch->GetGold()) >= ch->GetAllowedGold())
		#else
		if (claimingValute.yang + static_cast<int64_t>(ch->GetGold()) >= GOLD_MAX)
		#endif
		{
			claimingValute.yang = ch->GetAllowedGold() - ch->GetGold() - 1;
			if (claimingValute.yang <= 0)
				return false;
		}

		if (!safebox->RemoveValute(claimingValute))
			return false;

		// updating db
		SendShopSafeboxGetValutesDBPacket(ch->GetPlayerID(), claimingValute);

		if (ch->IsLookingShopOwner())
			SendBoardCountersClientPacket(ch);

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		safebox->RefreshToOwner();

		ch->GoldChange(claimingValute.yang);
		return true;
	}

	bool CShopManager::RecvShopSafeboxCloseClientPacket(LPCHARACTER ch)
	{
		if (!ch)
			return false;

		ch->SetIkarusShopSafebox(NULL);
		return true;
	}

	void CShopManager::SendShopSafeboxRefreshClientPacket(LPCHARACTER ch, const TValutesInfo& valute, const std::vector<ITEM_HANDLE>& vec)
	{
		if (!ch || !ch->GetDesc() || !ch->GetIkarusShopSafebox())
			return;

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopSafeboxRefresh) + sizeof(TShopPlayerItem) * vec.size();
		pack.subheader = SUBHEADER_GC_SHOP_SAFEBOX_REFRESH;

		TSubPacketGCShopSafeboxRefresh subpack{};
		subpack.itemcount = vec.size();
		subpack.valute = valute;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));
		
		for (auto& sitem : vec)
		{
			auto& info = static_cast<const TShopPlayerItem&>(sitem->GetInfo());
			buff.write(&info, sizeof(info));
		}

		ch->GetDesc()->Packet(buff.read_peek() , buff.size());
	}

	bool CShopManager::RecvAuctionListRequestClientPacket(LPCHARACTER ch)
	{
		if (!ch)
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_AUCTION_LIST_REQUEST))
			return false;

		std::vector<TAuctionListElement> vec;
		vec.reserve(m_mapAuctions.size());

		for (auto& [id, auction] : m_mapAuctions)
		{
			auto bestOffer = auction->GetBestOffer();

			static TAuctionListElement temp;
			temp.auction = auction->GetInfo();
			temp.best = bestOffer ? bestOffer->price : temp.auction.price;
			temp.offercount = auction->GetOffers().size();
			vec.push_back(temp);
		}

		SendAuctionListClientPacket(ch, vec);
		return true;
	}

	bool CShopManager::RecvAuctionOpenRequestClientPacket(LPCHARACTER ch, DWORD ownerid)
	{
		auto it = m_mapAuctions.find(ownerid);
		if (it == m_mapAuctions.end())
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_AUCTION_OPEN_REQUEST))
			return false;

		if (ch->GetPlayerID() == ownerid)
			return RecvMyAuctionOpenRequestClientPacket(ch);

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		auto& auction = it->second;
		ch->SetIkarusAuctionGuest(auction);
		return true;
	}

	bool CShopManager::RecvMyAuctionOpenRequestClientPacket(LPCHARACTER ch)
	{
		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_MY_ACUTION_OPEN_REQUEST))
			return false;

		// refreshing to owner
		if (auto auction = ch->GetIkarusAuction())
		{
			ch->SetLookingAuctionOwner(true);
			auction->RefreshToOwner(ch);
		}
		else
			SendAuctionOpenMyAuctionNoAuctionClientPacket(ch);

		return true;
	}

	bool CShopManager::RecvMyAuctionCloseClientPacket(LPCHARACTER ch)
	{
		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		ch->SetLookingAuctionOwner(false);
		return true;
	}

	bool CShopManager::RecvMyAuctionCancelClientPacket(LPCHARACTER ch)
	{
		if (auto auction = ch->GetIkarusAuction())
		{
			if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_MY_AUCTION_CANCEL))
				return false;

			if (!auction->GetOffers().empty())
				return false;

			TPacketGDNewIkarusShop pack;
			pack.bSubHeader = ikashop::SUBHEADER_GD_AUCTION_CANCEL;

			DWORD ownerid = ch->GetPlayerID();

			TEMP_BUFFER buff;
			buff.write(&pack, sizeof(pack));
			buff.write(&ownerid, sizeof(ownerid));

			db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
		}
		return true;
	}

	bool CShopManager::RecvAuctionCreateClientPacket(LPCHARACTER ch, const TPriceInfo& startprice, const TItemPos& pos)
	{
		if (!ch || ch->GetIkarusAuction())
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_AUCTION_CREATE))
			return false;

		if (!CheckCharacterActions(ch))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_DO_NOW");
			return true;
		}

		// if (ch->IsExchangeTime())
		if (ch->IsWarpStatus())
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_DO_NOW");
			return true;
		}

		if (!IsGoodSalePrice(startprice))
			return false;

		//checking about duration limit
		auto duration = OFFLINESHOP_AUCTION_DURATION;
		auto item = ch->GetItem(pos);
		if (!item)
			return false;

		if (item->IsEquipped() || item->IsExchanging() || item->isLocked())
			return false;

		auto table= ITEM_MANAGER::instance().GetTable(item->GetVnum());
		if (!table)
			return false;

		if (IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_GIVE) || IS_SET(table->dwAntiFlags , ITEM_ANTIFLAG_MYSHOP))
			return false;

		#ifdef ENABLE_SOULBIND_SYSTEM
		if (item->IsSealed())
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_CREATE_AUCTION_ITEM_SEALED");
			return false;
		}
		#endif

		#ifdef ENABLE_LOCKED_SYSTEM
		if (item->IsItemLocked())
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_CREATE_AUCTION_ITEM_SEALED");
			return false;
		}
		#endif

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		//making info
		TAuctionInfo auction;
		auction.duration = duration;
		auction.owner = ch->GetPlayerID();
		auction.id = item->GetID();
		auction.price = startprice;
		auction.count = item->GetCount();
		auction.vnum = item->GetVnum();
		auction.expiration = GetItemExpiration(item);

		#ifdef ENABLE_EVOLUTION_SYSTEM
		auction.evolution = item->GetEvolution();
		#endif
		#ifdef ENABLE_CHANGELOOK_SYSTEM
		auction.transmutation = item->GetTransmutation();
		#endif
		#ifdef ENABLE_LOCKED_SYSTEM
		auction.locked = item->GetLocked();
		#endif

		str_to_cstring(auction.ownername, ch->GetName());
		std::memcpy(auction.aAttr, item->GetAttributes(), sizeof(auction.aAttr));
		std::memcpy(auction.alSockets, item->GetSockets(), sizeof(auction.alSockets));

		//removing from character and deleting item instance
		MoveAndDestroyInstance(item, IKASHOP_AUCTION, 0);

		SendAuctionCreateDBPacket(auction);
		return true;
	}

	bool CShopManager::RecvAuctionAddOfferClientPacket(LPCHARACTER ch, DWORD ownerid, const TPriceInfo& price)
	{
		//checking about guesting
		if (!ch || !ch->GetIkarusAuctionGuest() || ch->GetIkarusAuctionGuest()->GetInfo().owner != ownerid)
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_AUCTION_ADD_OFFER))
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!CheckSafeboxSize(ch))
			return false;

		//check anti auto-offer
		if (ch->GetPlayerID() == ownerid)
			return false;

		//check about enough money
		if (static_cast<long long>(ch->GetGold()) < price.yang)
			return false;

		//checking about raise from best buyer
		auto auction = ch->GetIkarusAuctionGuest();
		if (auction->GetBestBuyer() == ch->GetPlayerID())
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_RAISE_OVER_YOURSELF");
			return false;
		}

		// patch with warp check
		ch->SetMyShopTime();
		ch->SetMyExchangeTime();

		//checking about min raise amount (+10% by default)
		auto minRaise = GetAuctionMinRaise(auction);
		if (price.GetTotalYangAmount() < minRaise)
			return false;

		ch->GoldChange(-price.yang);

		TAuctionOfferInfo offer;
		offer.buyerid = ch->GetPlayerID();
		offer.ownerid = ownerid;
		offer.datetime = static_cast<int64_t>(std::time(nullptr));
		offer.price = price;
		offer.seenflag = false;

		str_to_cstring(offer.buyername, ch->GetName());

		SendAuctionAddOfferDBPacket(offer);
		return true;
	}

	bool CShopManager::RecvAuctionExitFromAuction(LPCHARACTER ch, DWORD ownerid)
	{
		if (ch->GetIkarusAuctionGuest() && ch->GetIkarusAuctionGuest()->GetInfo().owner == ownerid)
			ch->SetIkarusAuctionGuest(nullptr);

		ch->SetMyShopTime();
		ch->SetMyExchangeTime();
		return true;
	}

	void CShopManager::SendAuctionListClientPacket(LPCHARACTER ch, const std::vector<TAuctionListElement>& auctionVec)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_AUCTION_LIST;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCAuctionList) + (sizeof(TAuctionListElement)*auctionVec.size());

		TSubPacketGCAuctionList subpack{};
		subpack.count = auctionVec.size();

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));
		if (!auctionVec.empty())
			buff.write(&auctionVec[0], sizeof(auctionVec[0]) * auctionVec.size());

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	void CShopManager::SendAuctionOpenAuctionClientPacket(LPCHARACTER ch, const CAuction& auction)
	{
		if (!ch->GetDesc())
			return;

		const auto& info = auction.GetInfo();
		const auto& offers = auction.GetOffers();

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = ch->GetPlayerID() != info.owner ? SUBHEADER_GC_OPEN_AUCTION : SUBHEADER_GC_OPEN_MY_AUCTION;
		pack.size = sizeof(pack)+ sizeof(TSubPacketGCAuctionOpen) + (sizeof(TAuctionOfferInfo) * offers.size());

		TSubPacketGCAuctionOpen subpack{};
		subpack.auction = info;
		subpack.offercount = offers.size();

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		for(auto& offer : offers)
			buff.write(offer.get(), sizeof(TAuctionOfferInfo));

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());

		// checking for new auction offers forwarded to owner
		if (pack.subheader == SUBHEADER_GC_OPEN_MY_AUCTION)
		{
			for(auto& offer : auction.GetOffers())
			{
				if (offer->seenflag == false)
				{
					TPacketGDNewIkarusShop pack{};
					pack.bSubHeader = ikashop::SUBHEADER_GD_AUCTION_OFFER_SEEN;

					ikashop::TSubPacketGDAuctionOfferSeen subpack{};
					subpack.ownerid = auction.GetInfo().owner;
					subpack.buyerid = offer->buyerid;
					subpack.price = offer->price.yang;

					TEMP_BUFFER buffer;
					buffer.write(&pack, sizeof(pack));
					buffer.write(&subpack, sizeof(subpack));

					db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buffer.read_peek(), buffer.size());
				}
			}
		}
	}

	void CShopManager::SendAuctionOpenMyAuctionNoAuctionClientPacket(LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_OPEN_MY_AUCTION_NO_AUCTION;
		pack.size = sizeof(pack);

		ch->GetDesc()->Packet(&pack, sizeof(pack));
	}

	void CShopManager::RecvCloseMyShopBoardClientPacket(LPCHARACTER ch)
	{
		if (ch)
			ch->SetLookingShopOwner(false);
	}

	void CShopManager::RecvCloseOfferListClientPacket(LPCHARACTER ch)
	{
		if (ch)
			ch->SetLookingIkarusShopOfferList(false);
	}

	void CShopManager::RecvCloseShopGuestClientPacket(LPCHARACTER ch)
	{
		if (ch)
			ch->SetIkarusShopGuest(nullptr);
	}

	void CShopManager::SendBoardCountersClientPacket(LPCHARACTER ch)
	{
		if (!ch || !ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.size = sizeof(pack) + sizeof(ikashop::TSubPacketGCBoardCounters);
		pack.subheader = ikashop::SUBHEADER_GC_BOARD_COUNTERS;

		ikashop::TSubPacketGCBoardCounters subpack{};

		// checking safebox items counter
		if (const auto safebox = ch->GetIkarusShopSafebox()){
			subpack.safebox = safebox->GetItems().size();
			if (safebox->GetValutes().yang != 0)
				subpack.safebox++;
		}

		// checking auction offers counter
		if (const auto auction = ch->GetIkarusAuction())
			subpack.auction = auction->GetNewOffersCounter();

		// checking incoming offers counter
		if (const auto shop = ch->GetIkarusShop())
			subpack.offers = shop->GetNewOffersCounter();

		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&subpack, sizeof(subpack));
	}

	void CShopManager::SendPopupMessage(LPCHARACTER ch, const std::string& message)
	{
		ikashop::TSubPacketGCPopupMessage subpack{};
		str_to_cstring(subpack.localeString, message.data());

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.size = sizeof(pack) + sizeof(subpack);
		pack.subheader = ikashop::SUBHEADER_GC_POPUP_MESSAGE;

		ch->GetDesc()->BufferedPacket(pack);
		ch->GetDesc()->Packet(subpack);
	}

	void CShopManager::UpdateShopsDuration()
	{
		for(auto& [id, shop] : m_mapShops)
			if (shop->GetDuration() > 0)
				shop->DecreaseDuration();
	}

	void CShopManager::UpdateAuctionsDuration()
	{
		for(auto& [id, auction] : m_mapAuctions)
			auction->DecreaseDuration();
	}

	void CShopManager::ClearSearchTimeMap()
	{
		m_searchCooldownMap.clear();
		m_offerCooldownMap.clear();
	}

	bool CShopManager::CheckOfferCooldown(DWORD pid)
	{
		static const auto cooldown_seconds = OFFLINESHOP_SECONDS_PER_OFFER * 1000;
		const auto now = get_dword_time();

		auto it = m_offerCooldownMap.find(pid);
		if (it == m_offerCooldownMap.end())
		{
			m_offerCooldownMap[pid] = now + cooldown_seconds ;
			return true;
		}

		auto& cooldown = it->second;
		if (cooldown > now)
			return false;

		cooldown = now + cooldown_seconds;
		return true;
	}

	bool CShopManager::CheckSafeboxSize(LPCHARACTER ch)
	{
		// validating character pointer
		if (!ch)
			return false;

		// searching for safebox
		auto safebox = ch->GetIkarusShopSafebox() ? ch->GetIkarusShopSafebox() : GetShopSafeboxByOwnerID(ch->GetPlayerID());
		if (!safebox)
			return false;

		// checking safebox size
		if (safebox->GetItems().size() >= OFFLINESHOP_MAX_SAFEBOX_SIZE)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_SAFEBOX_FULL");
			return false;
		}

		return true;
	}

	bool CShopManager::CheckSearchCooldown(DWORD pid)
	{
		static const auto cooldown_seconds = OFFLINESHOP_SECONDS_PER_SEARCH * 1000;
		const auto now = get_dword_time();

		auto it = m_searchCooldownMap.find(pid);
		if (it == m_searchCooldownMap.end())
		{
			m_searchCooldownMap[pid] = now + cooldown_seconds;
			return true;
		}

		auto& cooldown = it->second;
		if (cooldown > now)
			return false;

		cooldown = now + cooldown_seconds;
		return true;
	}

	#ifdef ENABLE_IKASHOP_AVG_AND_MINMAX_PRICE
	void CShopManager::LoadMinMaxPriceTable()
	{
		auto pMsg(DBManager::instance().DirectQuery("SELECT * FROM player.ikashop_minmaxprice"));
		if (pMsg->Get()->uiNumRows == 0)
		{
			sys_err("Offlineshop minmax price table is empty!");
			return;
		}

		m_MinMaxPriceMap.clear();
		MYSQL_ROW row;
		while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
		{
			int col = 0;
			DWORD item_vnum = 0;
			TItemMinMaxPriceInfo info{};
			str_to_number(item_vnum, row[col++]);
			str_to_number(info.minPrice, row[col++]);
			str_to_number(info.maxPrice, row[col++]);
			m_MinMaxPriceMap.emplace(item_vnum, info);
		}
	}

	void CShopManager::AvarageAndMinMaxPrice(LPCHARACTER ch, const uint32_t dwVnum)
	{
		if (!ch || !ch->GetDesc()) { return; }

		uint64_t totalPrice = 0;
		uint64_t totalCount = 0;

		for (auto& [id, shop] : m_mapShops)
		{
			for (auto& [id, sitem] : shop->GetItems())
			{
				const auto& info = sitem->GetInfo();
				auto& itemPrice = sitem->GetPrice();

				if (info.vnum == dwVnum)
				{
					totalPrice += itemPrice.yang;
					totalCount += info.count;
				}
			}
		}

		long long avgPrice = 0;
		if (totalPrice && totalCount)
		{
			avgPrice = (totalPrice / totalCount);
		}

		long long minPrice = 0, maxPrice = 0;
		const auto& it = m_MinMaxPriceMap.find(dwVnum);
		if (it != m_MinMaxPriceMap.end())
		{
			minPrice = it->second.minPrice;
			maxPrice = it->second.maxPrice;
		}

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_SHOP_AVARAGE_AND_MINMAX_PRICE;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopAvarageAndMinMaxPrice);

		TSubPacketGCShopAvarageAndMinMaxPrice subpack{};
		subpack.avgPrice = avgPrice;
		subpack.minPrice = minPrice;
		subpack.maxPrice = maxPrice;

		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&subpack, sizeof(subpack));
	}
	#endif

}
#endif//----
