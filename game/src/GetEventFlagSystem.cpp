#include "stdafx.h"
#include "GetEventFlagSystem.h"
#include "char.h"
#include "char_manager.h"
#include "packet.h"
#include "p2p.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "config.h"
#include "item.h"
#include "item_manager.h"
#include "utils.h"
#include "questmanager.h"
#include "entity.h"
#include "sectree_manager.h"
#include "buffer_manager.h"
#include "cmd.h"
#include "priv_manager.h"
#include "shop_manager.h"
#include "locale_service.h"
#include "LoadTxtToTable.h"
#include "fstream"
#include <boost/algorithm/string.hpp>

///----------------------------------------------------------------------------------------------------///
bool gef_pet_disable = false;

#ifdef ENABLE_CHANGELOOK_SYSTEM
bool gef_changelook_disable = false;
#endif

#ifdef ENABLE_WHEEL_OF_FORTUNE_SYSTEM
bool gef_wof_ep = false;
bool gef_wof_mp = false;
bool gef_wof_bronze_c = false;
bool gef_wof_silver_c = false;
bool gef_wof_gold_c = false;
bool gef_wof_emerald_c = false;
bool gef_wof_bronze_y = false;
bool gef_wof_silver_y = false;
bool gef_wof_gold_y = false;
bool gef_wof_emerald_y = false;
bool gef_wof_system_enable = false;
bool gef_wof_event_enable = false;
#endif

#ifdef ENABLE_IKASHOP_RENEWAL
bool gef_ika_item_add = false;
bool gef_ika_item_del = false;
bool gef_ika_item_edit = false;
bool gef_ika_item_buy = false;
bool gef_ika_create = false;
bool gef_ika_search = false;
bool gef_ika_safebox = false;
#endif

bool gef_battle_reward_disable = false;

bool gef_item_log_disable = false;

bool gef_advanced_log_disable = false;

#ifdef ENABLE_REDUCED_ENTITY_VIEW
bool gef_reduced_entity_disable = false;
int gef_reduced_view_range = 3000;
#endif

#ifdef ENABLE_CHECK_SECURE_ZONE
int gef_two_village_dist = 500;
int gef_devil_tower_dist = 500;
int gef_blue_dragon_dist = 500;
bool gef_check_secure_zone = true;
#endif

#ifdef ENABLE_OFFER_SYSTEM
int gef_happyhours = 0;
#endif

#ifdef __ARE_YOU_IN_OR_NOT__
int gef_are_you_in_or_not = 0;
#endif

#ifdef ENABLE_BLACK_MARKET_SYSTEM
bool gef_black_market_status = false;
#endif

#ifdef ENABLE_OKEY_CARD_SYSTEM
bool gef_okey_card = false;
#endif

#ifdef NEW_MOB_TIMER
int gef_mob_secure_range = 2000;
#endif

int gef_fishing_ch_count = 1;
std::string gef_fishing_ch_str = "";

//----------------------------------------------------------------------------------------------------///
//----------------------------------------------------------------------------------------------------///

#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
CEventFlagSystem::CEventFlagSystem()
{
}

CEventFlagSystem::~CEventFlagSystem()
{
}

void CEventFlagSystem::DisableSystem(const std::string & name, int value)
{
	///---------------------------------------------------------------------------///
	if (name == "pet_system_disable")
	{
		gef_pet_disable = (value > 0) ? true : false;
	}
	///---------------------------------------------------------------------------///
	#ifdef ENABLE_CHANGELOOK_SYSTEM
	else if (name == "changelook_disable") 		{ gef_changelook_disable = (value > 0) ? true : false; }
	#endif
	///---------------------------------------------------------------------------///
	#ifdef ENABLE_WHEEL_OF_FORTUNE_SYSTEM
	else if (name == "wof_ep_enable") 			{ gef_wof_ep = (value > 0) ? true : false; }
	else if (name == "wof_mp_enable") 			{ gef_wof_mp = (value > 0) ? true : false; }
	else if (name == "wof_bronze_c_enable") 	{ gef_wof_bronze_c = (value > 0) ? true : false; }
	else if (name == "wof_silver_c_enable") 	{ gef_wof_silver_c = (value > 0) ? true : false; }
	else if (name == "wof_gold_c_enable") 		{ gef_wof_gold_c = (value > 0) ? true : false; }
	else if (name == "wof_emerald_c_enable") 	{ gef_wof_emerald_c = (value > 0) ? true : false; }
	else if (name == "wof_bronze_y_enable") 	{ gef_wof_bronze_y = (value > 0) ? true : false; }
	else if (name == "wof_silver_y_enable") 	{ gef_wof_silver_y = (value > 0) ? true : false; }
	else if (name == "wof_gold_y_enable") 		{ gef_wof_gold_y = (value > 0) ? true : false; }
	else if (name == "wof_emerald_y_enable") 	{ gef_wof_emerald_y = (value > 0) ? true : false; }
	else if (name == "wof_system_enable")
	{
		gef_wof_system_enable = (value > 0) ? true : false;

		const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
		for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
		{
			LPCHARACTER ch = (*it)->GetCharacter();
			if (!ch) { continue; }
			if (value == 1)
			{
				ch->ChatPacket(CHAT_TYPE_COMMAND, "WofIconShow");
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_COMMAND, "WofIconHide");
			}
		}
	}
	else if (name == "wof_event_enable")
	{
		gef_wof_event_enable = (value > 0) ? true : false;

		const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
		for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
		{
			LPCHARACTER ch = (*it)->GetCharacter();
			if (!ch) { continue; }
			if (value == 1)
			{
				ch->ChatPacket(CHAT_TYPE_COMMAND, "WofEventIconShow");
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_COMMAND, "WofEventIconHide");
			}
		}
	}
	#endif
	///---------------------------------------------------------------------------///
	#ifdef ENABLE_IKASHOP_RENEWAL
	else if (name == "ika_add_disable") 		{ gef_ika_item_add = (value > 0) ? true : false; }
	else if (name == "ika_del_disable") 		{ gef_ika_item_del = (value > 0) ? true : false; }
	else if (name == "ika_edit_disable") 		{ gef_ika_item_edit = (value > 0) ? true : false; }
	else if (name == "ika_buy_disable") 		{ gef_ika_item_buy = (value > 0) ? true : false; }
	else if (name == "ika_create_disable") 		{ gef_ika_create = (value > 0) ? true : false; }
	else if (name == "ika_search_disable") 		{ gef_ika_search = (value > 0) ? true : false; }
	else if (name == "ika_safebox_disable")		{ gef_ika_safebox = (value > 0) ? true : false; }
	#endif
	///---------------------------------------------------------------------------///
	else if (name == "battle_reward_disable") 	{ gef_battle_reward_disable = (value > 0) ? true : false; }
	///---------------------------------------------------------------------------///
	else if (name == "item_log_disable") 		{ gef_item_log_disable = (value > 0) ? true : false; }
	///---------------------------------------------------------------------------///
	else if (name == "advanced_log_disable") 	{ gef_advanced_log_disable = (value > 0) ? true : false; }
	///---------------------------------------------------------------------------///
	#ifdef ENABLE_REDUCED_ENTITY_VIEW
	else if (name == "reduced_entity_disable") 	{ gef_reduced_entity_disable = (value > 0) ? true : false; }
	else if (name == "reduced_view_range") 		{ gef_reduced_view_range = value; }
	#endif
	///---------------------------------------------------------------------------///
	#ifdef ENABLE_CHECK_SECURE_ZONE
	else if (name == "two_village_dist") 		{ gef_two_village_dist = value; }
	else if (name == "deviltower_dist") 		{ gef_devil_tower_dist = value; }
	else if (name == "blue_dragon_dist") 		{ gef_blue_dragon_dist = value; }
	else if (name == "check_secure_zone") 		{ gef_check_secure_zone = (value > 0) ? false : true; }
	#endif
	///---------------------------------------------------------------------------///
	#ifdef ENABLE_OFFER_SYSTEM
	else if (name == "happyhours")
	{
		gef_happyhours = value;

		const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
		for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
		{
			LPCHARACTER ch = (*it)->GetCharacter();
			if (!ch) { continue; }
			ch->ChatPacket(CHAT_TYPE_COMMAND, "IndirimNesne %d", value);
		}
	}
	#endif
	///---------------------------------------------------------------------------///
	#ifdef __ARE_YOU_IN_OR_NOT__
	else if (name == "event_varmisin")
	{
		gef_are_you_in_or_not = value;

		const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
		for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
		{
			LPCHARACTER ch = (*it)->GetCharacter();
			if (!ch) { continue; }
			ch->ChatPacket(CHAT_TYPE_COMMAND, "VarmisinYokmusun %d", value);
		}
	}
	#endif
	///---------------------------------------------------------------------------///
	#ifdef ENABLE_BLACK_MARKET_SYSTEM
	else if (name == "blackmarket")
	{
		gef_black_market_status = (value > 0) ? true : false;

		const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
		for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
		{
			LPCHARACTER ch = (*it)->GetCharacter();
			if (!ch) { continue; }
			if (value == 1)
			{
				ch->ChatPacket(CHAT_TYPE_COMMAND, "blackmarket_icon_show");
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_COMMAND, "blackmarket_icon_hide");
			}
		}
	}
	#endif
	///---------------------------------------------------------------------------///
	#ifdef ENABLE_OKEY_CARD_SYSTEM
	else if (name == "cards_event")
	{
		gef_okey_card = (value > 0) ? true : false;

		const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
		for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
		{
			LPCHARACTER ch = (*it)->GetCharacter();
			if (!ch) { continue; }
			ch->ChatPacket(CHAT_TYPE_COMMAND, "cards_icon %d", value);
		}
	}
	#endif
	///---------------------------------------------------------------------------///
	#ifdef NEW_MOB_TIMER
	else if (name == "mob_secure_range") 		{ gef_mob_secure_range = value; }
	#endif
	///---------------------------------------------------------------------------///
	else if (name == "fishing_ch_count")
	{
		gef_fishing_ch_count = value;

		if ((value > 0) && (value < 21))
		{
			gef_fishing_ch_str = "";
			for (int x = 1; x <= value; x++)
			{
				gef_fishing_ch_str += ("CH"+std::to_string(x));
				if (x < value) { gef_fishing_ch_str += ", "; }
			}
		}
	}
	///---------------------------------------------------------------------------///
	else if (name == "player_max_level") 		{ gPlayerMaxLevel = MINMAX(1, value, PLAYER_MAX_LEVEL_CONST); }
	///---------------------------------------------------------------------------///
	#ifdef ENABLE_PUBLISHER_SYSTEM
	else if (name == "publisher_enable")
	{
		const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
		for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
		{
			LPCHARACTER ch = (*it)->GetCharacter();
			if (!ch) { continue; }
			ch->ChatPacket(CHAT_TYPE_COMMAND, "publisher_system %d", value);
			if (value > 0) { OnLoginPublisher(ch); }
		}
	}
	#endif
	///---------------------------------------------------------------------------///
}

void CEventFlagSystem::OnLogin(LPCHARACTER ch)
{
	if (!ch || !ch->GetDesc()) return;

	#ifdef ENABLE_OFFER_SYSTEM
	if (gef_happyhours > 0) { ch->ChatPacket(CHAT_TYPE_COMMAND, "IndirimNesne %d", gef_happyhours); }
	#endif
	#ifdef __ARE_YOU_IN_OR_NOT__
	if (gef_are_you_in_or_not > 0) { ch->ChatPacket(CHAT_TYPE_COMMAND, "VarmisinYokmusun %d", gef_are_you_in_or_not); }
	#endif
	#ifdef ENABLE_BLACK_MARKET_SYSTEM
	if (gef_black_market_status) { ch->ChatPacket(CHAT_TYPE_COMMAND, "blackmarket_icon"); }
	#endif
	#ifdef ENABLE_OKEY_CARD_SYSTEM
	if (gef_okey_card) { ch->ChatPacket(CHAT_TYPE_COMMAND, "cards_icon 1"); }
	#endif
	#ifdef ENABLE_WHEEL_OF_FORTUNE_SYSTEM
	if (gef_wof_system_enable) { ch->ChatPacket(CHAT_TYPE_COMMAND, "WofIconShow"); }
	if (gef_wof_event_enable) { ch->ChatPacket(CHAT_TYPE_COMMAND, "WofEventIconShow"); }
	#endif
}
#endif
