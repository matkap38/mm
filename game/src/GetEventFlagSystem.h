#ifndef __INC_METIN_II_GET_EVENT_FLAG_SYSTEM_H
#define __INC_METIN_II_GET_EVENT_FLAG_SYSTEM_H

#include "../../common/service.h"
#include "../../common/CommonDefines.h"

///----------------------------------------------------------------------------------------------------///
///----------------------------------------------------------------------------------------------------///
///----------------------------------------------------------------------------------------------------///
extern bool gef_pet_disable;

#ifdef ENABLE_CHANGELOOK_SYSTEM
extern bool gef_changelook_disable;
#endif

#ifdef ENABLE_WHEEL_OF_FORTUNE_SYSTEM
extern bool gef_wof_ep;
extern bool gef_wof_mp;
extern bool gef_wof_bronze_c;
extern bool gef_wof_silver_c;
extern bool gef_wof_gold_c;
extern bool gef_wof_emerald_c;
extern bool gef_wof_bronze_y;
extern bool gef_wof_silver_y;
extern bool gef_wof_gold_y;
extern bool gef_wof_emerald_y;
extern bool gef_wof_system_enable;
extern bool gef_wof_event_enable;
#endif

#ifdef ENABLE_IKASHOP_RENEWAL
extern bool gef_ika_item_add;
extern bool gef_ika_item_del;
extern bool gef_ika_item_edit;
extern bool gef_ika_item_buy;
extern bool gef_ika_create;
extern bool gef_ika_search;
extern bool gef_ika_safebox;
#endif

extern bool gef_battle_reward_disable;

extern bool gef_item_log_disable;

extern bool gef_advanced_log_disable;

#ifdef ENABLE_REDUCED_ENTITY_VIEW
extern bool gef_reduced_entity_disable;
extern int gef_reduced_view_range;
#endif

#ifdef ENABLE_CHECK_SECURE_ZONE
extern int gef_two_village_dist;
extern int gef_devil_tower_dist;
extern int gef_blue_dragon_dist;
extern bool gef_check_secure_zone;
#endif

#ifdef ENABLE_OFFER_SYSTEM
extern int gef_happyhours;
#endif

#ifdef __ARE_YOU_IN_OR_NOT__
extern int gef_are_you_in_or_not;
#endif

#ifdef ENABLE_BLACK_MARKET_SYSTEM
extern bool gef_black_market_status;
#endif

#ifdef ENABLE_OKEY_CARD_SYSTEM
extern bool gef_okey_card;
#endif

#ifdef NEW_MOB_TIMER
extern int gef_mob_secure_range;
#endif

extern int gef_fishing_ch_count;
extern std::string gef_fishing_ch_str;
//----------------------------------------------------------------------------------------------------///
//----------------------------------------------------------------------------------------------------///

#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
class CEventFlagSystem : public singleton<CEventFlagSystem>
{
	public:
		CEventFlagSystem();
		virtual ~CEventFlagSystem();

		void		DisableSystem(const std::string & name, int value);
		void		OnLogin(LPCHARACTER ch);
};
#endif

#endif//__INC_METIN_II_GET_EVENT_FLAG_SYSTEM_H
