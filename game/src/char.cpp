#include "stdafx.h"

#include "../../common/VnumHelper.h"

#include "char.h"

#include "config.h"
#include "utils.h"
#include "crc32.h"
#include "char_manager.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "item_manager.h"
#include "motion.h"
#include "vector.h"
#include "packet.h"
#include "cmd.h"
#include "fishing.h"
#include "exchange.h"
#include "battle.h"
#include "affect.h"
#include "shop.h"
#include "shop_manager.h"
#include "safebox.h"
#include "regen.h"
#include "pvp.h"
#include "party.h"
#include "start_position.h"
#include "questmanager.h"
#include "log.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "unique_item.h"
#include "priv_manager.h"
#include "war_map.h"
#include "xmas_event.h"
#include "banword.h"
#include "target.h"
#include "wedding.h"
#include "mob_manager.h"
#include "mining.h"
#include "monarch.h"
#include "castle.h"
#include "arena.h"
#include "horsename_manager.h"
#include "gm.h"
#include "map_location.h"
#include "BlueDragon_Binder.h"
#include "skill_power.h"
#include "buff_on_attributes.h"
#include "LoadTxtToTable.h"

#ifdef __PET_SYSTEM__
#include "PetSystem.h"
#endif

#ifdef ENABLE_IKASHOP_RENEWAL
#include <numeric>
#include <chrono>
#include "ikarus_shop.h"
#include "ikarus_shop_manager.h"
#endif

#ifdef ENABLE_BATTLE_PASS
#include "battle_pass.h"
#endif

#ifdef ENABLE_TANAKA_EVENT
#include "NewTanakaEvent.h"
#endif

#ifdef ENABLE_RONARK_SYSTEM
#include "ronark_map.h"
#endif

#include "DragonSoul.h"
#include "../../common/CommonDefines.h"

#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
#include <algorithm>
#include <iterator>
using namespace std;
#endif

#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#endif

#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
#include "GetEventFlagSystem.h"
#endif

extern const BYTE g_aBuffOnAttrPoints;
extern bool RaceToJob(unsigned race, unsigned *ret_job);

extern bool IS_SUMMONABLE_ZONE(int map_index); // char_item.cpp
bool CAN_ENTER_ZONE(const LPCHARACTER& ch, int map_index);

bool CAN_ENTER_ZONE(const LPCHARACTER& ch, int map_index)
{
	switch (map_index)
	{
	case 301:
	case 302:
	case 303:
	case 304:
		if (ch->GetLevel() < 90)
			return false;
	}
	return true;
}

#ifdef NEW_ICEDAMAGE_SYSTEM
const DWORD CHARACTER::GetNoDamageRaceFlag()
{
	return m_dwNDRFlag;
}

void CHARACTER::SetNoDamageRaceFlag(DWORD dwRaceFlag)
{
	if (dwRaceFlag>=MAIN_RACE_MAX_NUM) return;
	if (IS_SET(m_dwNDRFlag, 1<<dwRaceFlag)) return;
	SET_BIT(m_dwNDRFlag, 1<<dwRaceFlag);
}

void CHARACTER::UnsetNoDamageRaceFlag(DWORD dwRaceFlag)
{
	if (dwRaceFlag>=MAIN_RACE_MAX_NUM) return;
	if (!IS_SET(m_dwNDRFlag, 1<<dwRaceFlag)) return;
	REMOVE_BIT(m_dwNDRFlag, 1<<dwRaceFlag);
}

void CHARACTER::ResetNoDamageRaceFlag()
{
	m_dwNDRFlag = 0;
}

const std::set<DWORD> & CHARACTER::GetNoDamageAffectFlag()
{
	return m_setNDAFlag;
}

void CHARACTER::SetNoDamageAffectFlag(DWORD dwAffectFlag)
{
	m_setNDAFlag.emplace(dwAffectFlag);
}

void CHARACTER::UnsetNoDamageAffectFlag(DWORD dwAffectFlag)
{
	m_setNDAFlag.erase(dwAffectFlag);
}

void CHARACTER::ResetNoDamageAffectFlag()
{
	m_setNDAFlag.clear();
}
#endif

// <Factor> DynamicCharacterPtr member function definitions

LPCHARACTER DynamicCharacterPtr::Get() const
{
	LPCHARACTER p = NULL;
	if (is_pc)
	{
		p = CHARACTER_MANAGER::instance().FindByPID(id);
	}
	else
	{
		p = CHARACTER_MANAGER::instance().Find(id);
	}
	return p;
}

DynamicCharacterPtr& DynamicCharacterPtr::operator=(LPCHARACTER character)
{
	if (character == NULL)
	{
		Reset();
		return *this;
	}
	if (character->IsPC())
	{
		is_pc = true;
		id = character->GetPlayerID();
	}
	else
	{
		is_pc = false;
		id = character->GetVID();
	}
	return *this;
}

CHARACTER::CHARACTER()
{
	m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateIdle, &CHARACTER::EndStateEmpty);
	m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateMove, &CHARACTER::EndStateEmpty);
	m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateBattle, &CHARACTER::EndStateEmpty);

	Initialize();
}

CHARACTER::~CHARACTER()
{
	Destroy();
}

void CHARACTER::Initialize()
{
	CEntity::Initialize(ENTITY_CHARACTER);

	#ifdef ENABLE_PACKET_FLOOD
	analyze_protect = 0; 
	analyze_protect_other_count = 0;
	#endif

	#ifdef __ARE_YOU_IN_OR_NOT__
	ClearAyirnGame();
	#endif

	m_bNoOpenedShop = true;

	m_bOpeningSafebox = false;

	m_fSyncTime = get_float_time()-3;
	m_dwPlayerID = 0;
	m_dwKillerPID = 0;

	#ifdef __AUTO_SKILL_READER__
	m_pkAutoSkill = NULL;
	m_bSelectedSkillIdx = 0;
	m_bAutoSkillStatus = 0;
	#endif

	m_iMoveCount = 0;

	#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
	dwLastTargetInfoPulse = 0;
	#endif

	m_pkRegen = NULL;
	regen_id_ = 0;
	m_posRegen.x = m_posRegen.y = m_posRegen.z = 0;
	m_posStart.x = m_posStart.y = 0;
	m_posDest.x = m_posDest.y = 0;
	m_fRegenAngle = 0.0f;

	m_pkMobData		= NULL;
	m_pkMobInst		= NULL;

	m_pkShop		= NULL;
	m_pkChrShopOwner	= NULL;
	m_pkMyShop		= NULL;
	m_pkExchange	= NULL;
	m_pkParty		= NULL;
	m_pkPartyRequestEvent = NULL;

	m_pGuild = NULL;

	#if defined(__BL_LUCKY_BOX__)
	ResetLuckyBoxData();
	#endif

	m_pkChrTarget = NULL;

	m_pkMuyeongEvent = NULL;

	m_pkWarpNPCEvent = NULL;
	m_pkDeadEvent = NULL;
	m_pkStunEvent = NULL;
	m_pkSaveEvent = NULL;
	m_pkRecoveryEvent = NULL;
	m_pkTimedEvent = NULL;
	m_pkFishingEvent = NULL;
	m_pkWarpEvent = NULL;

	// MINING
	m_pkMiningEvent = NULL;
	// END_OF_MINING

#ifdef SPIN_WHEEL
	dwResultSpinWheel = 0;
	dwResultSpinWheelCount = 0;
#endif
	m_pkPoisonEvent = NULL;

	#ifdef ENABLE_WOLFMAN_CHARACTER
	m_pkBleedingEvent = NULL;
	#endif

	m_pkFireEvent = NULL;
	m_pkCheckSpeedHackEvent	= NULL;
	m_speed_hack_count	= 0;

	m_pkAffectEvent = NULL;
	m_afAffectFlag = TAffectFlag(0, 0);

	m_pkDestroyWhenIdleEvent = NULL;
	m_pkChrSyncOwner = NULL;

	m_points = {};
	m_pointsInstant = {};

	#ifdef ENABLE_OKEY_CARD_SYSTEM
	memset(&character_cards, 0, sizeof(character_cards));
	memset(&randomized_cards, 0, sizeof(randomized_cards));
	#endif

	m_bCharType = CHAR_TYPE_MONSTER;

	SetPosition(POS_STANDING);

	m_dwPlayStartTime = m_dwLastMoveTime = get_dword_time();

	#ifdef ENABLE_BATTLE_PASS
	m_dwOnLoginTime = get_global_time();
	m_dwBattlePassTime = 0;
	m_bBattlePassSave = false;
	#endif

	GotoState(m_stateIdle);
	m_dwStateDuration = 1;

	m_dwLastAttackTime = get_dword_time() - 20000;

	m_dwLastMovedTime = 0;

	m_bAddChrState = 0;

	m_pkChrStone = NULL;

	m_pkSafebox = NULL;
	m_iSafeboxSize = -1;
	m_iSafeboxLoadTime = 0;

	m_pkMall = NULL;
	m_iMallLoadTime = 0;

	m_posWarp.x = m_posWarp.y = m_posWarp.z = 0;
	m_lWarpMapIndex = 0;

	m_posExit.x = m_posExit.y = m_posExit.z = 0;
	m_lExitMapIndex = 0;

	m_pSkillLevels = NULL;

	m_dwMoveStartTime = 0;
	m_dwMoveDuration = 0;

	m_dwFlyTargetID = 0;

	m_dwNextStatePulse = 0;

	m_dwLastDeadTime = get_dword_time()-180000;

	m_bSkipSave = false;

	m_bItemLoaded = false;

	m_bHasPoisoned = false;

	#ifdef ENABLE_WOLFMAN_CHARACTER
	m_bHasBled = false;
	#endif

	m_pkDungeon = NULL;
	m_iEventAttr = 0;

	m_kAttackLog.dwVID = 0;
	m_kAttackLog.dwTime = 0;

	m_bNowWalking = m_bWalking = false;
	ResetChangeAttackPositionTime();

	m_bDetailLog = false;
	m_bMonsterLog = false;

	m_bDisableCooltime = false;

	m_iAlignment = 0;
	m_iRealAlignment = 0;

	m_iKillerModePulse = 0;
	m_bPKMode = PK_MODE_PEACE;

	m_dwQuestNPCVID = 0;
	m_dwQuestByVnum = 0;
	m_pQuestItem = NULL;

	m_szMobileAuth[0] = '\0';

	m_dwUnderGuildWarInfoMessageTime = get_dword_time()-60000;

	m_bUnderRefine = false;

	#ifdef __RANKING_SYSTEM__
	m_ranking_k_s = 0;
	m_ranking_k_b = 0;
	m_ranking_dm_s = 0;
	m_ranking_dm_b = 0;
	m_ranking_c_d = 0;
	m_ranking_o_c = 0;
	m_ranking_dm_skill = 0;
	m_ranking_dm_norm = 0;
	m_ranking_kill_boss.clear();
	m_bIsLoadedRankBoss = false;
	SetProtectTime("saverankboss", get_global_time()+(60*3));
	#endif

	// REFINE_NPC
	m_dwRefineNPCVID = 0;
	// END_OF_REFINE_NPC

	m_dwPolymorphRace = 0;

	m_bStaminaConsume = false;

	ResetChainLightningIndex();

	m_dwMountVnum = 0;
	m_chHorse = nullptr;
	m_chRider = nullptr;

	#ifdef ENABLE_IKASHOP_RENEWAL
	m_pkIkarusShop			= NULL;
	m_pkIkarusShopSafebox	= NULL;
	m_pkIkarusAuction		= NULL;
	m_pkIkarusAuctionGuest	= NULL;
	//ikashop-updated 03/08/19
	m_pkIkarusShopGuest		= NULL;

	//ikashop-updated 05/08/19
	m_bIsLookingIkarusShopOfferList = false;
	#endif

	m_pWarMap = NULL;
	m_pWeddingMap = NULL;
	m_bChatCounter = 0;

	ResetStopTime();

	m_dwLastVictimSetTime = get_dword_time() - 3000;
	m_iMaxAggro = -100;

	m_bSendHorseLevel = 0;
	m_bSendHorseHealthGrade = 0;
	m_bSendHorseStaminaGrade = 0;

	m_dwLoginPlayTime = 0;

	m_pkChrMarried = NULL;

	m_posSafeboxOpen.x = -1000;
	m_posSafeboxOpen.y = -1000;

	// EQUIP_LAST_SKILL_DELAY
	m_dwLastSkillTime = get_dword_time();
	// END_OF_EQUIP_LAST_SKILL_DELAY

	// MOB_SKILL_COOLTIME
	memset(m_adwMobSkillCooltime, 0, sizeof(m_adwMobSkillCooltime));
	// END_OF_MOB_SKILL_COOLTIME

	// ARENA
	m_pArena = NULL;
	m_nPotionLimit = quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count");
	// END_ARENA

	//PREVENT_TRADE_WINDOW
	m_isOpenSafebox = 0;
	//END_PREVENT_TRADE_WINDOW

	//PREVENT_REFINE_WINDOW
	m_isOpenRefine = 0;
	//END_PREVENT_REFINE_WINDOW

	//PREVENT_REFINE_HACK
	m_iRefineTime = 0;
	//END_PREVENT_REFINE_HACK

	//RESTRICT_USE_SEED_OR_MOONBOTTLE
	m_iSeedTime = 0;
	//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
	//PREVENT_PORTAL_AFTER_EXCHANGE
	m_iExchangeTime = 0;
	//END_PREVENT_PORTAL_AFTER_EXCHANGE
	//

	m_iSafeboxLoadTime = 0;

	m_iMyShopTime = 0;
	m_iMyExchangeTime = 0;
	m_bWarpStatus = false;

	InitMC();

	m_deposit_pulse = 0;

	SET_OVER_TIME(this, OT_NONE);

	m_strNewName = "";

	m_known_guild.clear();

	m_dwLogOffInterval = 0;

	m_bComboSequence = 0;
	m_dwLastComboTime = 0;
	m_bComboIndex = 0;
	m_iComboHackCount = 0;
	m_dwSkipComboAttackByTime = 0;

	m_dwMountTime = 0;

	m_dwLastGoldDropTime = 0;

	#ifdef ENABLE_NEWSTUFF
	m_dwLastItemDropTime = 0;
	m_dwLastBoxUseTime = 0;
	m_dwLastBuySellTime = 0;
	#endif

	m_bIsLoadedAffect = false;
	cannot_dead = false;

	#ifdef __PET_SYSTEM__
	m_petSystem = 0;
	m_bIsPet = false;
	#endif

	#ifdef NEW_ICEDAMAGE_SYSTEM
	m_dwNDRFlag = 0;
	m_setNDAFlag.clear();
	#endif

	m_fAttMul = 1.0f;
	m_fDamMul = 1.0f;

	m_pointsInstant.iDragonSoulActiveDeck = -1;

	#ifdef ENABLE_ANTI_CMD_FLOOD
	m_dwCmdAntiFloodCount = 0;
	m_dwCmdAntiFloodPulse = 0;
	#endif

	memset(&m_tvLastSyncTime, 0, sizeof(m_tvLastSyncTime));
	m_iSyncHackCount = 0;

	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	m_bAcceCombination	= false;
	m_bAcceAbsorption	= false;
	#endif

	#ifdef ENABLE_MOB_RANGE_ITEM
	m_antimobrange = false;
	#endif

	#ifdef ENABLE_REFINE_LUCK
	m_refineluck = false;
	#endif

	#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
	m_iDcShopLastTime = 0;
	m_pkRefreshCoinAndCash = NULL;
	#endif

	#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
	m_iDcShopAdminLastTime = 0;
	#endif

	#ifdef ENABLE_CUBE_RENEWAL
	m_dwCubeNpcRaceNum = 0;
	#endif

	#ifdef __RENEWAL_BRAVE_CAPE__
	m_pkBraveCapeEvent = NULL;
	#endif

	#ifdef ENABLE_FISHING_SECURE
	m_bFishingSecure = false;
	m_bOldPKMode = PK_MODE_PROTECT;
	#endif

	#ifdef ENABLE_CHECK_SECURE_ZONE
	m_bDamageSecure = false;
	#endif

	m_bStopDamage = false;

	#ifdef ENABLE_CAPTCHA_SELECT_SYSTEM
	m_pkCaptchaEvent = NULL;
	m_iCaptchaReplyImage = 0;
	m_iCaptchaTime = 0;
	m_bCaptchaStatus = false;
	m_bCaptchaFalseReplyCount = 0;
	m_bCaptchaSkip = false;
	#endif

	#ifdef __AUTO_HUNT__
	m_bAutoHuntStatus = false;
	m_dwAutoHuntStartTime = 0;
	#endif

	#ifdef ENABLE_CHANGELOOK_SYSTEM
	m_bChangeLook = false;
	#endif

	#ifdef BOSS_DMG_RANKING
	m_bSpecialBoss = false;
	packetTimer = NULL;
	m_dwDmgRankingFirstPID = 0;
	#endif

	#ifdef ENABLE_EVOLUTION_SYSTEM
	m_bEvoStoneCounter = 0;
	#endif

	#ifdef ENABLE_REDUCED_ENTITY_VIEW
	m_bIsStoneInMob = false;
	m_bIsNoReducedEntity = false;
	m_lMapIDX = 0;
	#endif

	#ifdef ENABLE_MULTI_FARM_BLOCK
	m_pkCheckMultiFarmEvent = NULL;
	#endif

	#ifdef ENABLE_MULTI_HUNT_BLOCK
	m_pkCheckMultiHuntEvent = NULL;
	#endif

	#ifdef ENABLE_MULTI_FISH_BLOCK
	m_pkCheckMultiFishEvent = NULL;
	#endif

	#ifdef ENABLE_DEFENCE_SHIELD
	m_bDefenceShield = false;
	#endif

	#ifdef NEW_MOB_TIMER
	m_lSpawnX = 0;
	m_lSpawnY = 0;
	#endif

	#ifdef ENABLE_AUTO_ITEM_SELL_SYSTEM
	m_autoSellStatus = false;
	m_autoSellList.clear();
	#endif
#ifdef ENABLE_HIDE_COSTUME_PART
	m_bHideBodyCostume = false;
	m_bHideHairCostume = false;
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	m_bHideWeaponCostume = false;
#endif
#endif //__HIDE_COSTUME_SYSTEM__
}

void CHARACTER::Create(const char * c_pszName, DWORD vid, bool isPC)
{
	static int s_crc = 172814;

	char crc_string[128+1];
	snprintf(crc_string, sizeof(crc_string), "%s%p%d", c_pszName, this, ++s_crc);
	m_vid = VID(vid, GetCRC32(crc_string, strlen(crc_string)));

	if (isPC)
		m_stName = c_pszName;
}

void CHARACTER::Destroy()
{
	CloseMyShop();

	#ifdef ENABLE_PACKET_FLOOD
	analyze_protect = 0;
	analyze_protect_other_count = 0;
	#endif

	#ifdef ENABLE_TANAKA_EVENT
	if (IsPC())
	{
		CNewTanakaEventManager::instance().ExitPlayer(this);
	}
	#endif

	if (m_pkRegen)
	{
		if (m_pkDungeon)
		{
			// Dungeon regen may not be valid at this point
			if (m_pkDungeon->IsValidRegen(m_pkRegen, regen_id_))
			{
				--m_pkRegen->count;
			}
		}
		else
		{
			// Is this really safe?
			#ifdef ENABLE_RELOAD_REGEN
			if (is_valid_regen(m_pkRegen))
			#endif
			{
				--m_pkRegen->count;
			}
		}

		m_pkRegen = NULL;
	}

	if (m_pkDungeon)
	{
		SetDungeon(NULL);
	}

	#ifdef __PET_SYSTEM__
	if (m_petSystem)
	{
		m_petSystem->Destroy();
		delete m_petSystem;

		m_petSystem = 0;
	}
	#endif

	HorseSummon(false);

	if (GetRider())
		GetRider()->ClearHorseInfo();

	if (GetDesc())
	{
		GetDesc()->BindCharacter(NULL);
//		BindDesc(NULL);
	}

	if (m_pkExchange)
		m_pkExchange->Cancel();

	SetVictim(NULL);

	if (GetShop())
	{
		GetShop()->RemoveGuest(this);
		SetShop(NULL);
	}

	ClearStone();
	ClearSync();
	ClearTarget();

	if (NULL == m_pkMobData)
	{
		DragonSoul_CleanUp();
		ClearItem();
	}

	// <Factor> m_pkParty becomes NULL after CParty destructor call!
	LPPARTY party = m_pkParty;
	if (party)
	{
		if (party->GetLeaderPID() == GetVID() && !IsPC())
		{
			M2_DELETE(party);
		}
		else
		{
			party->Unlink(this);

			if (!IsPC())
				party->Quit(GetVID());
		}

		SetParty(NULL);
	}

	if (m_pkMobInst)
	{
		M2_DELETE(m_pkMobInst);
		m_pkMobInst = NULL;
	}

	m_pkMobData = NULL;

	#ifdef ENABLE_MULTI_FARM_BLOCK
	m_bmultiFarmStatus = false;
	event_cancel(&m_pkCheckMultiFarmEvent);
	#endif

	#ifdef ENABLE_MULTI_HUNT_BLOCK
	m_bmultiHuntStatus = false;
	event_cancel(&m_pkCheckMultiHuntEvent);
	#endif

	#ifdef ENABLE_MULTI_FISH_BLOCK
	m_bmultiFishStatus = false;
	event_cancel(&m_pkCheckMultiFishEvent);
	#endif

	if (m_pkSafebox)
	{
		M2_DELETE(m_pkSafebox);
		m_pkSafebox = NULL;
	}

	if (m_pkMall)
	{
		M2_DELETE(m_pkMall);
		m_pkMall = NULL;
	}

	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin();  it != m_map_buff_on_attrs.end(); it++)
	{
		if (NULL != it->second)
		{
			M2_DELETE(it->second);
		}
	}
	m_map_buff_on_attrs.clear();

	m_set_pkChrSpawnedBy.clear();

	#ifdef __AUTO_SKILL_READER__
	if (m_pkAutoSkill)
		event_cancel(&m_pkAutoSkill);
	#endif

	StopMuyeongEvent();
	event_cancel(&m_pkWarpNPCEvent);
	event_cancel(&m_pkRecoveryEvent);
	event_cancel(&m_pkDeadEvent);
	event_cancel(&m_pkSaveEvent);
	event_cancel(&m_pkTimedEvent);
	event_cancel(&m_pkStunEvent);
	event_cancel(&m_pkFishingEvent);
	event_cancel(&m_pkPoisonEvent);
	#ifdef ENABLE_WOLFMAN_CHARACTER
	event_cancel(&m_pkBleedingEvent);
	#endif
	event_cancel(&m_pkFireEvent);
	event_cancel(&m_pkPartyRequestEvent);
	//DELAYED_WARP
	event_cancel(&m_pkWarpEvent);
	event_cancel(&m_pkCheckSpeedHackEvent);
	//END_DELAYED_WARP

	#ifdef ENABLE_CAPTCHA_SELECT_SYSTEM
	event_cancel(&m_pkCaptchaEvent);
	#endif

	#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
	event_cancel(&m_pkRefreshCoinAndCash);
	#endif

	// RECALL_DELAY
	//event_cancel(&m_pkRecallEvent);
	// END_OF_RECALL_DELAY

	// MINING
	event_cancel(&m_pkMiningEvent);
	// END_OF_MINING

	#ifdef __RENEWAL_BRAVE_CAPE__
	event_cancel(&m_pkBraveCapeEvent);
	#endif

	#ifdef BOSS_DMG_RANKING
	event_cancel(&packetTimer);
	#endif

	for (itertype(m_mapMobSkillEvent) it = m_mapMobSkillEvent.begin(); it != m_mapMobSkillEvent.end(); ++it)
	{
		LPEVENT pkEvent = it->second;
		event_cancel(&pkEvent);
	}
	m_mapMobSkillEvent.clear();

	//event_cancel(&m_pkAffectEvent);
	ClearAffect();

	event_cancel(&m_pkDestroyWhenIdleEvent);

	if (m_pSkillLevels)
	{
		M2_DELETE_ARRAY(m_pSkillLevels);
		m_pSkillLevels = NULL;
	}

	CEntity::Destroy();

	if (GetSectree())
		GetSectree()->RemoveEntity(this);

	if (m_bMonsterLog)
		CHARACTER_MANAGER::instance().UnregisterForMonsterLog(this);

	#ifdef ENABLE_IKASHOP_RENEWAL
	SetIkarusShop(nullptr);
	SetIkarusShopGuest(nullptr);
	SetIkarusShopSafebox(nullptr);
	SetIkarusAuction(nullptr);
	SetIkarusAuctionGuest(nullptr);
	#endif

}

const char * CHARACTER::GetName() const
{
	return m_stName.empty() ? (m_pkMobData ? m_pkMobData->m_table.szLocaleName : "") : m_stName.c_str();
}

void CHARACTER::OpenMyShop(const char * c_pszSign, TShopItemTable * pTable, BYTE bItemCount)
{
	if (!CanHandleItem()) // @fixme149
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래중(창고,교환,상점)에는 개인상점을 사용할 수 없습니다."));
		return;
	}

	#ifndef ENABLE_OPEN_SHOP_WITH_ARMOR
	if (GetPart(PART_MAIN) > 2)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("갑옷을 벗어야 개인 상점을 열 수 있습니다."));
		return;
	}
	#endif

	if (GetMyShop())
	{
		CloseMyShop();
		return;
	}

	quest::PC * pPC = quest::CQuestManager::instance().GetPCForce(GetPlayerID());

	if (pPC->IsRunning())
		return;

	if (bItemCount == 0)
		return;

	int64_t nTotalMoney = 0;

	for (int n = 0; n < bItemCount; ++n)
	{
		nTotalMoney += static_cast<int64_t>((pTable+n)->price);
	}

	nTotalMoney += static_cast<int64_t>(GetGold());

	if (GOLD_MAX <= nTotalMoney)
	{
		sys_err("[OVERFLOW_GOLD] Overflow (GOLD_MAX) id %u name %s", GetPlayerID(), GetName());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("20억 냥을 초과하여 상점을 열수가 없습니다"));
		return;
	}

	char szSign[SHOP_SIGN_MAX_LEN+1];
	strlcpy(szSign, c_pszSign, sizeof(szSign));

	m_stShopSign = szSign;

	if (m_stShopSign.length() == 0)
		return;

	if (CBanwordManager::instance().CheckString(m_stShopSign.c_str(), m_stShopSign.length()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("비속어나 은어가 포함된 상점 이름으로 상점을 열 수 없습니다."));
		return;
	}

	// MYSHOP_PRICE_LIST
	std::map<DWORD, DWORD> itemkind;
	// END_OF_MYSHOP_PRICE_LIST

	std::set<TItemPos> cont;
	for (BYTE i = 0; i < bItemCount; ++i)
	{
		if (cont.find((pTable + i)->pos) != cont.end())
		{
			sys_err("MYSHOP: duplicate shop item detected! (name: %s)", GetName());
			return;
		}

		// ANTI_GIVE, ANTI_MYSHOP check
		LPITEM pkItem = GetItem((pTable + i)->pos);
		if (pkItem)
		{
			const TItemTable * item_table = pkItem->GetProto();

			if (item_table && (IS_SET(item_table->dwAntiFlags, ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_MYSHOP)))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("유료화 아이템은 개인상점에서 판매할 수 없습니다."));
				return;
			}

			if (pkItem->IsEquipped() == true)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("장비중인 아이템은 개인상점에서 판매할 수 없습니다."));
				return;
			}

			if (true == pkItem->isLocked())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("사용중인 아이템은 개인상점에서 판매할 수 없습니다."));
				return;
			}

			#ifdef ENABLE_LOCKED_SYSTEM
			if (true == pkItem->IsItemLocked())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("사용중인 아이템은 개인상점에서 판매할 수 없습니다."));
				return;
			}
			#endif

			// MYSHOP_PRICE_LIST
			itemkind[pkItem->GetVnum()] = (pTable + i)->price / pkItem->GetCount();
			// END_OF_MYSHOP_PRICE_LIST
		}

		cont.emplace((pTable + i)->pos);
	}

	// MYSHOP_PRICE_LIST

	if (CountSpecifyItem(71049)) {

		// @fixme403 BEGIN
		TItemPriceListTable header;
		memset(&header, 0, sizeof(TItemPriceListTable));

		header.dwOwnerID = GetPlayerID();
		header.byCount = itemkind.size();

		size_t idx=0;
		for (itertype(itemkind) it = itemkind.begin(); it != itemkind.end(); ++it)
		{
			header.aPriceInfo[idx].dwVnum = it->first;
			header.aPriceInfo[idx].dwPrice = it->second;
			idx++;
		}

		db_clientdesc->DBPacket(HEADER_GD_MYSHOP_PRICELIST_UPDATE, GetDesc()->GetHandle(), &header, sizeof(TItemPriceListTable));
		// @fixme403 END
	}
	// END_OF_MYSHOP_PRICE_LIST
	else if (CountSpecifyItem(50200))
		RemoveSpecifyItem(50200, 1);
	else
		return;

	if (m_pkExchange)
		m_pkExchange->Cancel();

	TPacketGCShopSign p;

	p.bHeader = HEADER_GC_SHOP_SIGN;
	p.dwVID = GetVID();
	strlcpy(p.szSign, c_pszSign, sizeof(p.szSign));

	PacketAround(p);

	m_pkMyShop = CShopManager::instance().CreatePCShop(this, pTable, bItemCount);

	if (IsPolymorphed() == true)
	{
		RemoveAffect(AFFECT_POLYMORPH);
	}

	if (GetHorse())
	{
		HorseSummon( false, true );
	}
	else if (GetMountVnum())
	{
		RemoveAffect(AFFECT_MOUNT);
		RemoveAffect(AFFECT_MOUNT_BONUS);
	}

	SetPolymorph(30000, true);

}

void CHARACTER::CloseMyShop()
{
	if (GetMyShop())
	{
		m_stShopSign.clear();
		CShopManager::instance().DestroyPCShop(this);
		m_pkMyShop = NULL;

		TPacketGCShopSign p;

		p.bHeader = HEADER_GC_SHOP_SIGN;
		p.dwVID = GetVID();
		p.szSign[0] = '\0';

		PacketAround(p);
		#ifdef ENABLE_WOLFMAN_CHARACTER
		SetPolymorph(m_points.job, true);
		// SetPolymorph(0, true);
		#else
		SetPolymorph(GetJob(), true);
		#endif
	}
}

void EncodeMovePacket(TPacketGCMove & pack, DWORD dwVID, BYTE bFunc, BYTE bArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime, BYTE bRot)
{
	pack.bHeader = HEADER_GC_MOVE;
	pack.bFunc   = bFunc;
	pack.bArg    = bArg;
	pack.dwVID   = dwVID;
	pack.dwTime  = dwTime ? dwTime : get_dword_time();
	pack.bRot    = bRot;
	pack.lX		= x;
	pack.lY		= y;
	pack.dwDuration	= dwDuration;
}

void CHARACTER::RestartAtSamePos()
{
	if (m_bIsObserver)
		return;

	EncodeRemovePacket(this);
	EncodeInsertPacket(this);

	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = (it++)->first;

		EncodeRemovePacket(entity);
		if (!m_bIsObserver)
			EncodeInsertPacket(entity);

		if ( entity->IsType(ENTITY_CHARACTER) )
		{
			LPCHARACTER lpChar = (LPCHARACTER)entity;
			if ( lpChar->IsPC() || lpChar->IsNPC() || lpChar->IsMonster() )
			{
				if (!entity->IsObserverMode())
					entity->EncodeInsertPacket(this);
			}
		}
		else
		{
			if ( !entity->IsObserverMode())
			{
				entity->EncodeInsertPacket(this);
			}
		}
	}
}

void CHARACTER::EncodeInsertPacket(LPENTITY entity)
{
	LPDESC d;
	if (!(d = entity->GetDesc()))
		return;

	LPCHARACTER ch = (LPCHARACTER) entity;
	ch->SendGuildName(GetGuild());

	TPacketGCCharacterAdd pack;

	pack.header		= HEADER_GC_CHARACTER_ADD;
	pack.dwVID		= m_vid;
	pack.bType		= GetCharType();
	pack.angle		= GetRotation();
	pack.x			= GetX();
	pack.y			= GetY();
	pack.z			= GetZ();
	pack.wRaceNum	= GetRaceNum();

	if (IsPet())
	{
		pack.bMovingSpeed	= 255;//150
	}
	else
	{
		pack.bMovingSpeed	= GetLimitPoint(POINT_MOV_SPEED);
	}

	pack.bAttackSpeed = GetLimitPoint(POINT_ATT_SPEED);
	pack.dwAffectFlag[0] = m_afAffectFlag.bits[0];
	pack.dwAffectFlag[1] = m_afAffectFlag.bits[1];

	pack.bStateFlag = m_bAddChrState;

	int iDur = 0;

	if (m_posDest.x != pack.x || m_posDest.y != pack.y)
	{
		iDur = (m_dwMoveStartTime + m_dwMoveDuration) - get_dword_time();

		if (iDur <= 0)
		{
			pack.x = m_posDest.x;
			pack.y = m_posDest.y;
		}
	}

	d->Packet(pack);

	if (IsPC() == true || m_bCharType == CHAR_TYPE_NPC
		#ifdef __PET_SYSTEM__
		|| m_bCharType == CHAR_TYPE_PET
		#endif
	)
	{
		TPacketGCCharacterAdditionalInfo addPacket;
		memset(&addPacket, 0, sizeof(TPacketGCCharacterAdditionalInfo));

		addPacket.header = HEADER_GC_CHAR_ADDITIONAL_INFO;
		addPacket.dwVID = m_vid;

		addPacket.awPart[CHR_EQUIPPART_ARMOR] = GetPart(PART_MAIN);
		addPacket.awPart[CHR_EQUIPPART_WEAPON] = GetPart(PART_WEAPON);
		addPacket.awPart[CHR_EQUIPPART_HEAD] = GetPart(PART_HEAD);
		addPacket.awPart[CHR_EQUIPPART_HAIR] = GetPart(PART_HAIR);
		#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		addPacket.awPart[CHR_EQUIPPART_ACCE] = GetPart(PART_ACCE);
		#endif

		addPacket.bPKMode = m_bPKMode;
		addPacket.dwMountVnum = GetMountVnum();
		addPacket.bEmpire = m_bEmpire;
		addPacket.dwLevel = (IsPC()) ? GetLevel() : 0;

		if (false)
		{
			LPCHARACTER ch = (LPCHARACTER) entity;

			if (GetEmpire() == ch->GetEmpire() || ch->GetGMLevel() > GM_PLAYER || m_bCharType == CHAR_TYPE_NPC)
			{
				goto show_all_info;
			}
			else
			{
				memset(addPacket.name, 0, CHARACTER_NAME_MAX_LEN);
				addPacket.dwGuildID = 0;
				addPacket.iAlignment = 0;
			}
		}
		else
		{
		show_all_info:
			strlcpy(addPacket.name, GetName(), sizeof(addPacket.name));

			if (GetGuild() != NULL)
			{
				addPacket.dwGuildID = GetGuild()->GetID();
			}
			else
			{
				addPacket.dwGuildID = 0;
			}

			addPacket.iAlignment = m_iAlignment / 10;

			#ifdef ENABLE_EVOLUTION_SYSTEM
			addPacket.iEvolutionWeapon = (IsPC() && GetWear(WEAR_WEAPON)) ? GetWear(WEAR_WEAPON)->GetEvolution() : 0;
			addPacket.iEvolutionBody = (IsPC() && GetWear(WEAR_BODY)) ? GetWear(WEAR_BODY)->GetEvolution() : 0;
			#endif
		}

		d->Packet(addPacket);
	}

	if (iDur)
	{
		TPacketGCMove pack;
		EncodeMovePacket(pack, GetVID(), FUNC_MOVE, 0, m_posDest.x, m_posDest.y, iDur, 0, (BYTE) (GetRotation() / 5));
		d->Packet(pack);

		TPacketGCWalkMode p;
		p.vid = GetVID();
		p.header = HEADER_GC_WALK_MODE;
		p.mode = m_bNowWalking ? WALKMODE_WALK : WALKMODE_RUN;

		d->Packet(p);
	}

	if (entity->IsType(ENTITY_CHARACTER) && GetDesc())
	{
		LPCHARACTER ch = (LPCHARACTER) entity;
		if (ch->IsWalking())
		{
			TPacketGCWalkMode p;
			p.vid = ch->GetVID();
			p.header = HEADER_GC_WALK_MODE;
			p.mode = ch->m_bNowWalking ? WALKMODE_WALK : WALKMODE_RUN;
			GetDesc()->Packet(p);
		}
	}

	if (GetMyShop())
	{
		TPacketGCShopSign p;

		p.bHeader = HEADER_GC_SHOP_SIGN;
		p.dwVID = GetVID();
		strlcpy(p.szSign, m_stShopSign.c_str(), sizeof(p.szSign));

		d->Packet(p);
	}

	if (entity->IsType(ENTITY_CHARACTER))
	{
		sys_log(3, "EntityInsert %s (RaceNum %d) (%d %d) TO %s",
				GetName(), GetRaceNum(), GetX() / SECTREE_SIZE, GetY() / SECTREE_SIZE, ((LPCHARACTER)entity)->GetName());
	}
}

void CHARACTER::EncodeRemovePacket(LPENTITY entity)
{
	if (entity->GetType() != ENTITY_CHARACTER)
		return;

	LPDESC d;

	if (!(d = entity->GetDesc()))
		return;

	TPacketGCCharacterDelete pack;

	pack.header	= HEADER_GC_CHARACTER_DEL;
	pack.id	= m_vid;

	d->Packet(pack);

	if (entity->IsType(ENTITY_CHARACTER))
		sys_log(3, "EntityRemove %s(%d) FROM %s", GetName(), (DWORD) m_vid, ((LPCHARACTER) entity)->GetName());
}

void CHARACTER::UpdatePacket()
{
	if (GetSectree() == NULL) return;

	TPacketGCCharacterUpdate pack;
	TPacketGCCharacterUpdate pack2;

	pack.header = HEADER_GC_CHARACTER_UPDATE;
	pack.dwVID = m_vid;

	pack.awPart[CHR_EQUIPPART_ARMOR] = GetPart(PART_MAIN);
	pack.awPart[CHR_EQUIPPART_WEAPON] = GetPart(PART_WEAPON);
	pack.awPart[CHR_EQUIPPART_HEAD] = GetPart(PART_HEAD);
	pack.awPart[CHR_EQUIPPART_HAIR] = GetPart(PART_HAIR);
	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	pack.awPart[CHR_EQUIPPART_ACCE] = GetPart(PART_ACCE);
	#endif

	pack.bMovingSpeed		= GetLimitPoint(POINT_MOV_SPEED);
	pack.bAttackSpeed		= GetLimitPoint(POINT_ATT_SPEED);

	pack.bStateFlag			= m_bAddChrState;
	pack.dwAffectFlag[0]	= m_afAffectFlag.bits[0];
	pack.dwAffectFlag[1]	= m_afAffectFlag.bits[1];
	pack.dwGuildID			= 0;
	pack.iAlignment			= m_iAlignment / 10;
	pack.bPKMode			= m_bPKMode;

	if (GetGuild())
		pack.dwGuildID		= GetGuild()->GetID();

	pack.dwMountVnum		= GetMountVnum();
	pack.dwLevel			= GetLevel();

	#ifdef __PET_SYSTEM__
	if (IsPet())
	{
		pack.bMovingSpeed	= 255;
	}
	#endif

	#ifdef ENABLE_EVOLUTION_SYSTEM
	pack.iEvolutionWeapon	= (IsPC() && GetWear(WEAR_WEAPON)) ? GetWear(WEAR_WEAPON)->GetEvolution() : 0;
	pack.iEvolutionBody		= (IsPC() && GetWear(WEAR_BODY)) ? GetWear(WEAR_BODY)->GetEvolution() : 0;
	#endif

	pack2 = pack;
	pack2.dwGuildID = 0;
	pack2.iAlignment = 0;

	if (false)
	{
		if (m_bIsObserver != true)
		{
			for (ENTITY_MAP::iterator iter = m_map_view.begin(); iter != m_map_view.end(); iter++)
			{
				LPENTITY pEntity = iter->first;

				if (pEntity != NULL)
				{
					if (pEntity->IsType(ENTITY_CHARACTER) == true)
					{
						if (pEntity->GetDesc() != NULL)
						{
							LPCHARACTER pChar = (LPCHARACTER)pEntity;

							if (GetEmpire() == pChar->GetEmpire() || pChar->GetGMLevel() > GM_PLAYER)
							{
								pEntity->GetDesc()->Packet(pack);
							}
							else
							{
								pEntity->GetDesc()->Packet(pack2);
							}
						}
					}
					else
					{
						if (pEntity->GetDesc() != NULL)
						{
							pEntity->GetDesc()->Packet(pack);
						}
					}
				}
			}
		}

		if (GetDesc() != NULL)
		{
			GetDesc()->Packet(pack);
		}
	}
	else
	{
		PacketAround(pack);
	}
}

LPCHARACTER CHARACTER::FindCharacterInView(const char * c_pszName, bool bFindPCOnly)
{
	ENTITY_MAP::iterator it = m_map_view.begin();

	for (; it != m_map_view.end(); ++it)
	{
		if (!it->first->IsType(ENTITY_CHARACTER))
			continue;

		LPCHARACTER tch = (LPCHARACTER) it->first;

		if (bFindPCOnly && tch->IsNPC())
			continue;

		if (!strcasecmp(tch->GetName(), c_pszName))
			return (tch);
	}

	return NULL;
}

void CHARACTER::SetPosition(int pos)
{
	if (pos == POS_STANDING)
	{
		REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_DEAD);
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN);

		event_cancel(&m_pkDeadEvent);
		event_cancel(&m_pkStunEvent);
	}
	else if (pos == POS_DEAD)
		SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_DEAD);

	if (!IsStone())
	{
		switch (pos)
		{
			case POS_FIGHTING:
				if (!IsState(m_stateBattle))
					MonsterLog("[BATTLE] 싸우는 상태");

				GotoState(m_stateBattle);
				break;

			default:
				if (!IsState(m_stateIdle))
					MonsterLog("[IDLE] 쉬는 상태");

				GotoState(m_stateIdle);
				break;
		}
	}

	m_pointsInstant.position = pos;
}

void CHARACTER::Save()
{
	if (!m_bSkipSave)
		CHARACTER_MANAGER::instance().DelayedSave(this);

	#ifdef ENABLE_BATTLE_PASS
	if (IsPC())
	{
		uint8_t bBattlePassId = this->GetBattlePassId();
		if (bBattlePassId)
		{
			DWORD val = (get_global_time() - m_dwOnLoginTime);
			m_dwOnLoginTime = get_global_time();
			CBattlePass::instance().BattlePassCheckMissions(this, bBattlePassId, BP_MT_PLAYER_PLAY_TIME, 0, val);
		}
	}
	#endif
}

void CHARACTER::CreatePlayerProto(TPlayerTable & tab)
{
	memset(&tab, 0, sizeof(TPlayerTable));

	if (GetNewName().empty())
	{
		strlcpy(tab.name, GetName(), sizeof(tab.name));
	}
	else
	{
		strlcpy(tab.name, GetNewName().c_str(), sizeof(tab.name));
	}

	strlcpy(tab.ip, GetDesc()->GetHostName(), sizeof(tab.ip));

	tab.id			= m_dwPlayerID;
	tab.voice		= GetPoint(POINT_VOICE);
	tab.level		= GetLevel();
	tab.level_step	= GetPoint(POINT_LEVEL_STEP);
	tab.exp			= GetExp();
	tab.gold		= GetGold();
	tab.job			= m_points.job;
	tab.part_base	= m_pointsInstant.bBasePart;
	tab.skill_group	= m_points.skill_group;

	DWORD dwPlayedTime = (get_dword_time() - m_dwPlayStartTime);
	if (dwPlayedTime > 60000)
	{
		if (GetSectree() && !GetSectree()->IsAttr(GetX(), GetY(), ATTR_BANPK))
		{
			if (GetRealAlignment() < 0)
			{
				if (IsEquipUniqueItem(UNIQUE_ITEM_FASTER_ALIGNMENT_UP_BY_TIME))
					UpdateAlignment(120 * (dwPlayedTime / 60000));
				else
					UpdateAlignment(60 * (dwPlayedTime / 60000));
			}
			else
				UpdateAlignment(5 * (dwPlayedTime / 60000));
		}

		SetRealPoint(POINT_PLAYTIME, GetRealPoint(POINT_PLAYTIME) + dwPlayedTime / 60000);
		ResetPlayTime(dwPlayedTime % 60000);
	}

	tab.playtime = GetRealPoint(POINT_PLAYTIME);
	tab.lAlignment = m_iRealAlignment;

	if (m_posWarp.x != 0 || m_posWarp.y != 0)
	{
		tab.x = m_posWarp.x;
		tab.y = m_posWarp.y;
		tab.z = 0;
		tab.lMapIndex = m_lWarpMapIndex;
	}
	else
	{
		tab.x = GetX();
		tab.y = GetY();
		tab.z = GetZ();
		tab.lMapIndex = GetMapIndex();
	}

	if (m_lExitMapIndex == 0)
	{
		tab.lExitMapIndex = tab.lMapIndex;
		tab.lExitX = tab.x;
		tab.lExitY = tab.y;
	}
	else
	{
		tab.lExitMapIndex = m_lExitMapIndex;
		tab.lExitX = m_posExit.x;
		tab.lExitY = m_posExit.y;
	}

	sys_log(0, "SAVE: %s %dx%d", GetName(), tab.x, tab.y);

	tab.st = GetRealPoint(POINT_ST);
	tab.ht = GetRealPoint(POINT_HT);
	tab.dx = GetRealPoint(POINT_DX);
	tab.iq = GetRealPoint(POINT_IQ);

	tab.stat_point = GetPoint(POINT_STAT);
	tab.skill_point = GetPoint(POINT_SKILL);
	tab.sub_skill_point = GetPoint(POINT_SUB_SKILL);
	tab.horse_skill_point = GetPoint(POINT_HORSE_SKILL);

	tab.stat_reset_count = GetPoint(POINT_STAT_RESET_COUNT);

	tab.hp = GetHP();
	tab.sp = GetSP();

	tab.stamina = GetStamina();

	tab.sRandomHP = m_points.iRandomHP;
	tab.sRandomSP = m_points.iRandomSP;

	if (m_PlayerSlots)
	{
		for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
			tab.quickslot[i] = m_PlayerSlots->pQuickslot[i];
	}

	#ifdef ENABLE_AUTO_ITEM_SELL_SYSTEM
	memset(tab.autoSellSlot, 0, sizeof(tab.autoSellSlot));

	if (!m_autoSellList.empty())
	{
		BYTE bSellCount = 0;
		for (const auto& iter : m_autoSellList)
		{
			if (bSellCount >= AUTO_ITEM_SELL_MAX) { break; }
			tab.autoSellSlot[bSellCount] = iter;
			++bSellCount;
		}
	}
	#endif

	if (m_stMobile.length() && !*m_szMobileAuth)
		strlcpy(tab.szMobile, m_stMobile.c_str(), sizeof(tab.szMobile));

	thecore_memcpy(tab.parts, m_pointsInstant.parts, sizeof(tab.parts));

	#ifdef __RANKING_SYSTEM__
	tab.killed_stone		= GetKillStone();
	tab.killed_boss			= GetKillBoss();
	tab.damage_stone		= GetDamageStone();
	tab.damage_boss			= GetDamageBoss();
	tab.success_dungeon		= GetCompletedDungeon();
	tab.open_chest			= GetOpenedChest();
	tab.damage_skill		= GetDamageSkill();
	tab.damage_normal		= GetDamageNormal();
	#endif

	#ifdef ENABLE_ALIGNMENT_BONUS
	tab.alignment_level		= GetAlignmentLevel();
	#endif

	#ifdef ENABLE_AUTO_ITEM_SELL_SYSTEM
	tab.autoSellStatus		= GetAutoSellItem();
	#endif

	// REMOVE_REAL_SKILL_LEVLES
	thecore_memcpy(tab.skills, m_pSkillLevels, sizeof(TPlayerSkill) * SKILL_MAX_NUM);
	// END_OF_REMOVE_REAL_SKILL_LEVLES

	tab.horse				= GetHorseData();

	#ifdef ENABLE_OTO_TOPLAMA
	tab.oto_metin			= GetOtoMetin();
	tab.oto_canavar			= GetOtoCanavar();
	tab.oto_boss			= GetOtoBoss();
	#endif

	#ifdef ENABLE_ANTI_EXP
	tab.anti_exp			= GetAntiExp();
	#endif
}

void CHARACTER::SaveReal()
{
	if (m_bSkipSave)
		return;

	if (!GetDesc())
	{
		sys_err("Character::Save : no descriptor when saving (name: %s)", GetName());
		return;
	}

	TPlayerTable table;
	CreatePlayerProto(table);

	db_clientdesc->DBPacket(HEADER_GD_PLAYER_SAVE, GetDesc()->GetHandle(), &table, sizeof(TPlayerTable));

	quest::PC * pkQuestPC = quest::CQuestManager::instance().GetPCForce(GetPlayerID());

	if (!pkQuestPC)
		sys_err("CHARACTER::Save : null quest::PC pointer! (name %s)", GetName());
	else
	{
		pkQuestPC->Save();
	}

	marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());
	if (pMarriage)
		pMarriage->Save();
}

void CHARACTER::FlushDelayedSaveItem()
{
	LPITEM item;
	for (int i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = GetInventoryItem(i)))
			ITEM_MANAGER::instance().FlushDelayedSave(item);
	}

	#ifdef ENABLE_SPECIAL_STORAGE
	LPITEM iUpdate; LPITEM iBook; LPITEM iStone; LPITEM iChest;
	for (int x = 0; x < SPECIAL_INVENTORY_MAX_NUM; ++x)
	{
		if ((iUpdate = GetUpgradeInventoryItem(x)))
			ITEM_MANAGER::instance().FlushDelayedSave(iUpdate);

		if ((iBook = GetBookInventoryItem(x)))
			ITEM_MANAGER::instance().FlushDelayedSave(iBook);

		if ((iStone = GetStoneInventoryItem(x)))
			ITEM_MANAGER::instance().FlushDelayedSave(iStone);

		if ((iChest = GetChestInventoryItem(x)))
			ITEM_MANAGER::instance().FlushDelayedSave(iChest);
	}
	#endif

	#ifdef ENABLE_SWITCHBOT
	LPITEM iBot;
	for (int s = 0; s < SWITCHBOT_SLOT_COUNT; ++s)
	{
		if ((iBot = GetSwitchBotItem(s)))
			ITEM_MANAGER::instance().FlushDelayedSave(iBot);
	}
	#endif
}

void CHARACTER::Disconnect(const char * c_pszReason)
{
	assert(GetDesc() != NULL);

	sys_log(0, "DISCONNECT: %s (%s)", GetName(), c_pszReason ? c_pszReason : "unset" );

	#ifdef ENABLE_BATTLE_PASS
	uint8_t bBattlePassId = this->GetBattlePassId();
	if (bBattlePassId)
	{
		DWORD val = (get_global_time() - m_dwOnLoginTime);
		m_dwOnLoginTime = get_global_time();
		CBattlePass::instance().BattlePassCheckMissions(this, bBattlePassId, BP_MT_PLAYER_PLAY_TIME, 0, val);
	}
	#endif

	#ifdef __RANKING_SYSTEM__
	if (IsPC()) { SaveRankBoss(true); }
	m_bIsLoadedRankBoss = false;
	#endif

	if (GetShop())
	{
		GetShop()->RemoveGuest(this);
		SetShop(NULL);
	}

	if (GetArena() != NULL)
	{
		GetArena()->OnDisconnect(GetPlayerID());
	}

	if (GetParty() != NULL)
	{
		GetParty()->UpdateOfflineState(GetPlayerID());
	}

	#ifdef __BACK_DUNGEON__
	CHARACTER_MANAGER::Instance().SetBackDungeon(true, true, mother_port, g_bChannel, GetMapIndex(), GetPlayerID(), GetX(), GetY());
	#endif

	marriage::CManager::instance().Logout(this);

	#ifdef ENABLE_MULTI_FARM_BLOCK
	CHARACTER_MANAGER::Instance().RemoveMultiFarm(GetDesc() ? GetDesc()->GetHostName() : "", GetPlayerID(), false);
	#endif

	#ifdef ENABLE_MULTI_HUNT_BLOCK
	CHARACTER_MANAGER::Instance().RemoveMultiHunt(GetDesc() ? GetDesc()->GetHostName() : "", GetPlayerID(), false);
	#endif

	#ifdef ENABLE_MULTI_FISH_BLOCK
	CHARACTER_MANAGER::Instance().RemoveMultiFish(GetDesc() ? GetDesc()->GetHostName() : "", GetPlayerID(), false);
	#endif

	#if defined(__BL_LUCKY_BOX__)
	LuckyBoxReceive();
	#endif

#ifdef __AUTO_HUNT__
	SetAutoHuntStatus(false);
#endif

	// switch bot leak
	CSwitchbot * pkSwitchbot = CSwitchbotManager::Instance().FindSwitchbot(GetPlayerID());
	if (pkSwitchbot && pkSwitchbot->HasActiveSlots() && !pkSwitchbot->IsSwitching())
		pkSwitchbot->Stop();

	// P2P Logout
	TPacketGGLogout p;
	p.bHeader = HEADER_GG_LOGOUT;
	strlcpy(p.szName, GetName(), sizeof(p.szName));
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGLogout));
	LogManager::instance().CharLog(this, 0, "LOGOUT", "");

	if (m_pWarMap)
		SetWarMap(NULL);

	#ifdef ENABLE_RONARK_SYSTEM
	if (CRonarkMapManager::instance().IsRonarkMap(GetMapIndex()))
	{
		CRonarkMapManager::instance().DecMember(this);
	}
	#endif

	if (m_pWeddingMap)
	{
		SetWeddingMap(NULL);
	}

	if (GetGuild())
		GetGuild()->LogoutMember(this);

	quest::CQuestManager::instance().LogoutPC(this);

	if (GetParty())
		GetParty()->Unlink(this);

	if (IsStun() || IsDead())
	{
		DeathPenalty(0);
		PointChange(POINT_HP, 50 - GetHP());
	}

	if (!CHARACTER_MANAGER::instance().FlushDelayedSave(this))
	{
		SaveReal();
	}

	FlushDelayedSaveItem();

	SaveAffect();
	m_bIsLoadedAffect = false;

	#ifdef ENABLE_BATTLE_PASS
	ListBattlePassMap::iterator it = m_listBattlePass.begin();
	while (it != m_listBattlePass.end())
	{
		TPlayerBattlePassMission* pkMission = *it++;
		if (!pkMission->bIsUpdated)
			continue;
		db_clientdesc->DBPacket(HEADER_GD_SAVE_BATTLE_PASS, 0, pkMission, sizeof(TPlayerBattlePassMission));
	}
	m_bIsLoadedBattlePass = false;
	#endif

	m_bSkipSave = true;

	quest::CQuestManager::instance().DisconnectPC(this);

	CloseSafebox();

	CloseMall();

	CPVPManager::instance().Disconnect(this);

	CTargetManager::instance().Logout(GetPlayerID());

	MessengerManager::instance().Logout(GetName());

	if (GetDesc())
		GetDesc()->BindCharacter(NULL);

	M2_DESTROY_CHARACTER(this);
}

bool CHARACTER::Show(long lMapIndex, long x, long y, long z, bool bShowSpawnMotion/* = false */)
{
	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);
	if (!sectree)
	{
		sys_log(0, "cannot find sectree by %dx%d mapindex %d", x, y, lMapIndex);
		return false;
	}

	SetMapIndex(lMapIndex);
	#ifdef ENABLE_REDUCED_ENTITY_VIEW
	SetMapIDX(lMapIndex);
	#endif

	bool bChangeTree = false;

	if (!GetSectree() || GetSectree() != sectree)
		bChangeTree = true;

	if (bChangeTree)
	{
		if (GetSectree())
			GetSectree()->RemoveEntity(this);

		#ifdef ENABLE_GOTO_LAG_FIX
		ViewCleanup(IsPC());
		#else
		ViewCleanup();
		#endif
	}

	if (!IsNPC())
	{
		sys_log(0, "SHOW: %s %dx%dx%d", GetName(), x, y, z);
		if (GetStamina() < GetMaxStamina())
			StartAffectEvent();
	}
	else if (m_pkMobData)
	{
		m_pkMobInst->m_posLastAttacked.x = x;
		m_pkMobInst->m_posLastAttacked.y = y;
		m_pkMobInst->m_posLastAttacked.z = z;
	}

	if (bShowSpawnMotion)
	{
		SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);
		m_afAffectFlag.Set(AFF_SPAWN);
	}

	SetXYZ(x, y, z);

	m_posDest.x = x;
	m_posDest.y = y;
	m_posDest.z = z;

	m_posStart.x = x;
	m_posStart.y = y;
	m_posStart.z = z;

	if (bChangeTree)
	{
		EncodeInsertPacket(this);
		sectree->InsertEntity(this);

		UpdateSectree();
	}
	else
	{
		ViewReencode();
		sys_log(0, "      in same sectree");
	}

	REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);

	SetValidComboInterval(0);
	return true;
}

// BGM_INFO
struct BGMInfo
{
	std::string	name;
	float		vol;
};

typedef std::map<unsigned, BGMInfo> BGMInfoMap;

static BGMInfoMap 	gs_bgmInfoMap;
static bool		gs_bgmVolEnable = false;

void CHARACTER_SetBGMVolumeEnable()
{
	gs_bgmVolEnable = true;
	sys_log(0, "bgm_info.set_bgm_volume_enable");
}

void CHARACTER_AddBGMInfo(unsigned mapIndex, const char* name, float vol)
{
	BGMInfo newInfo;
	newInfo.name = name;
	newInfo.vol = vol;

	gs_bgmInfoMap[mapIndex] = newInfo;

	sys_log(0, "bgm_info.add_info(%d, '%s', %f)", mapIndex, name, vol);
}

const BGMInfo& CHARACTER_GetBGMInfo(unsigned mapIndex)
{
	BGMInfoMap::iterator f = gs_bgmInfoMap.find(mapIndex);
	if (gs_bgmInfoMap.end() == f)
	{
		static BGMInfo s_empty = {"", 0.0f};
		return s_empty;
	}
	return f->second;
}

bool CHARACTER_IsBGMVolumeEnable()
{
	return gs_bgmVolEnable;
}
// END_OF_BGM_INFO

void CHARACTER::MainCharacterPacket()
{
	const unsigned mapIndex = GetMapIndex();
	const BGMInfo& bgmInfo = CHARACTER_GetBGMInfo(mapIndex);

	// SUPPORT_BGM
	if (!bgmInfo.name.empty())
	{
		if (CHARACTER_IsBGMVolumeEnable())
		{
			sys_log(1, "bgm_info.play_bgm_vol(%d, name='%s', vol=%f)", mapIndex, bgmInfo.name.c_str(), bgmInfo.vol);
			TPacketGCMainCharacter4_BGM_VOL mainChrPacket;
			mainChrPacket.header = HEADER_GC_MAIN_CHARACTER4_BGM_VOL;
			mainChrPacket.dwVID = m_vid;
			mainChrPacket.wRaceNum = GetRaceNum();
			mainChrPacket.lx = GetX();
			mainChrPacket.ly = GetY();
			mainChrPacket.lz = GetZ();
			mainChrPacket.empire = GetDesc()->GetEmpire();
			mainChrPacket.skill_group = GetSkillGroup();
			strlcpy(mainChrPacket.szChrName, GetName(), sizeof(mainChrPacket.szChrName));

			mainChrPacket.fBGMVol = bgmInfo.vol;
			strlcpy(mainChrPacket.szBGMName, bgmInfo.name.c_str(), sizeof(mainChrPacket.szBGMName));
			GetDesc()->Packet(mainChrPacket);
		}
		else
		{
			sys_log(1, "bgm_info.play(%d, '%s')", mapIndex, bgmInfo.name.c_str());
			TPacketGCMainCharacter3_BGM mainChrPacket;
			mainChrPacket.header = HEADER_GC_MAIN_CHARACTER3_BGM;
			mainChrPacket.dwVID = m_vid;
			mainChrPacket.wRaceNum = GetRaceNum();
			mainChrPacket.lx = GetX();
			mainChrPacket.ly = GetY();
			mainChrPacket.lz = GetZ();
			mainChrPacket.empire = GetDesc()->GetEmpire();
			mainChrPacket.skill_group = GetSkillGroup();
			strlcpy(mainChrPacket.szChrName, GetName(), sizeof(mainChrPacket.szChrName));
			strlcpy(mainChrPacket.szBGMName, bgmInfo.name.c_str(), sizeof(mainChrPacket.szBGMName));
			GetDesc()->Packet(mainChrPacket);
		}
		//if (m_stMobile.length())
		//		ChatPacket(CHAT_TYPE_COMMAND, "sms");
	}
	// END_OF_SUPPORT_BGM
	else
	{
		sys_log(0, "bgm_info.play(%d, DEFAULT_BGM_NAME)", mapIndex);

		TPacketGCMainCharacter pack;
		pack.header = HEADER_GC_MAIN_CHARACTER;
		pack.dwVID = m_vid;
		pack.wRaceNum = GetRaceNum();
		pack.lx = GetX();
		pack.ly = GetY();
		pack.lz = GetZ();
		pack.empire = GetDesc()->GetEmpire();
		pack.skill_group = GetSkillGroup();
		strlcpy(pack.szName, GetName(), sizeof(pack.szName));
		GetDesc()->Packet(pack);

		if (m_stMobile.length())
			ChatPacket(CHAT_TYPE_COMMAND, "sms");
	}
}

void CHARACTER::PointsPacket()
{
	if (!GetDesc())
		return;

	TPacketGCPoints pack;

	pack.header	= HEADER_GC_CHARACTER_POINTS;

	pack.points[POINT_LEVEL]		= GetLevel();
	pack.points[POINT_EXP]			= GetExp();
	pack.points[POINT_NEXT_EXP]		= GetNextExp();
	pack.points[POINT_HP]			= GetHP();
	pack.points[POINT_MAX_HP]		= GetMaxHP();
	pack.points[POINT_SP]			= GetSP();
	pack.points[POINT_MAX_SP]		= GetMaxSP();
	#ifndef ENABLE_FULL_YANG
	pack.points[POINT_GOLD]			= GetGold();
	#endif
	pack.points[POINT_STAMINA]		= GetStamina();
	pack.points[POINT_MAX_STAMINA]	= GetMaxStamina();

	for (int i = POINT_ST; i < POINT_MAX_NUM; ++i)
		pack.points[i] = GetPoint(i);

	#ifdef ENABLE_OTO_TOPLAMA
	pack.points[POINT_OTO_METIN]	= GetOtoMetin();
	pack.points[POINT_OTO_CANAVAR]	= GetOtoCanavar();
	pack.points[POINT_OTO_PATRON]	= GetOtoBoss();
	#endif

	#ifdef ENABLE_ANTI_EXP
	pack.points[POINT_ANTI_EXP]		= GetAntiExp();
	#endif

	GetDesc()->Packet(&pack, sizeof(TPacketGCPoints));

	#ifdef ENABLE_FULL_YANG
	TPacketGCGold pGold;
	pGold.bHeader = HEADER_GC_GOLD;
	pGold.lGold = GetGold();
	GetDesc()->Packet(&pGold, sizeof(TPacketGCGold));
	#endif
}

bool CHARACTER::ChangeSex()
{
	if (GetWear(WEAR_COSTUME_BODY) || GetWear(WEAR_COSTUME_HAIR) || GetWear(WEAR_COSTUME_MOUNT) || GetWear(WEAR_COSTUME_PET) || GetWear(WEAR_COSTUME_WEAPON))// @fixme191
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Kostum varken bu islemi gerceklestiremezsin"));
		return false;
	}
 
	int src_race = GetRaceNum();

	switch (src_race)
	{
		case MAIN_RACE_WARRIOR_M:
			m_points.job = MAIN_RACE_WARRIOR_W;
			break;

		case MAIN_RACE_WARRIOR_W:
			m_points.job = MAIN_RACE_WARRIOR_M;
			break;

		case MAIN_RACE_ASSASSIN_M:
			m_points.job = MAIN_RACE_ASSASSIN_W;
			break;

		case MAIN_RACE_ASSASSIN_W:
			m_points.job = MAIN_RACE_ASSASSIN_M;
			break;

		case MAIN_RACE_SURA_M:
			m_points.job = MAIN_RACE_SURA_W;
			break;

		case MAIN_RACE_SURA_W:
			m_points.job = MAIN_RACE_SURA_M;
			break;

		case MAIN_RACE_SHAMAN_M:
			m_points.job = MAIN_RACE_SHAMAN_W;
			break;

		case MAIN_RACE_SHAMAN_W:
			m_points.job = MAIN_RACE_SHAMAN_M;
			break;

		#ifdef ENABLE_WOLFMAN_CHARACTER
		case MAIN_RACE_WOLFMAN_M:
			m_points.job = MAIN_RACE_WOLFMAN_M;
			break;
		#endif

		default:
			sys_err("CHANGE_SEX: %s unknown race %d", GetName(), src_race);
			return false;
	}

	sys_log(0, "CHANGE_SEX: %s (%d -> %d)", GetName(), src_race, m_points.job);
	return true;
}

DWORD CHARACTER::GetRaceNum() const // @fixme501
{
	if (m_dwPolymorphRace)
		return m_dwPolymorphRace;

	if (m_pkMobData)
		return m_pkMobData->m_table.dwVnum;

	return m_points.job;
}

void CHARACTER::SetRace(BYTE race)
{
	if (race >= MAIN_RACE_MAX_NUM)
	{
		sys_err("CHARACTER::SetRace(name=%s, race=%d).OUT_OF_RACE_RANGE", GetName(), race);
		return;
	}

	m_points.job = race;
}

BYTE CHARACTER::GetJob() const
{
	unsigned race = m_points.job;
	unsigned job;

	if (RaceToJob(race, &job))
		return job;

	sys_err("CHARACTER::GetJob(name=%s, race=%d).OUT_OF_RACE_RANGE", GetName(), race);
	return JOB_WARRIOR;
}

void CHARACTER::SetLevel(BYTE level)
{
	m_points.level = level;

	if (IsPC())
	{
		if (level < PK_PROTECT_LEVEL)
			SetPKMode(PK_MODE_PROTECT);
		else if (GetGMLevel() != GM_PLAYER)
			SetPKMode(PK_MODE_PROTECT);
		else if (m_bPKMode == PK_MODE_PROTECT)
			SetPKMode(PK_MODE_PEACE);
	}
}

void CHARACTER::SetEmpire(BYTE bEmpire)
{
	#ifdef ENABLE_VSTURNUVA
	if (GetMapIndex() == 28)
	{
		m_bEmpire = 3;
	}
	else
	#endif
	{
		m_bEmpire = bEmpire;
	}
}

#define ENABLE_GM_FLAG_IF_TEST_SERVER
#define ENABLE_GM_FLAG_FOR_LOW_WIZARD
void CHARACTER::SetPlayerProto(const TPlayerTable * t)
{
	if (!GetDesc() || !*GetDesc()->GetHostName())
		sys_err("cannot get desc or hostname");
	else
		SetGMLevel();

	m_PlayerSlots = std::make_unique<PlayerSlotT>(); // @fixme199
	m_bCharType = CHAR_TYPE_PC;

	m_dwPlayerID = t->id;

	m_iAlignment = t->lAlignment;
	m_iRealAlignment = t->lAlignment;

	m_points.voice = t->voice;

	m_points.skill_group = t->skill_group;

	m_pointsInstant.bBasePart = t->part_base;
	SetPart(PART_HAIR, t->parts[PART_HAIR]);
	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	SetPart(PART_ACCE, t->parts[PART_ACCE]);
	#endif

	m_points.iRandomHP = t->sRandomHP;
	m_points.iRandomSP = t->sRandomSP;

	// REMOVE_REAL_SKILL_LEVLES
	if (m_pSkillLevels)
		M2_DELETE_ARRAY(m_pSkillLevels);

	m_pSkillLevels = M2_NEW TPlayerSkill[SKILL_MAX_NUM];
	thecore_memcpy(m_pSkillLevels, t->skills, sizeof(TPlayerSkill) * SKILL_MAX_NUM);
	// END_OF_REMOVE_REAL_SKILL_LEVLES

	#ifdef ENABLE_AUTO_ITEM_SELL_SYSTEM
	m_autoSellList.clear();
	for (size_t i = 0; i < AUTO_ITEM_SELL_MAX; ++i)
	{
		if (t->autoSellSlot[i] > 0)
		{
			m_autoSellList.emplace(t->autoSellSlot[i]);
		}
	}
	#endif

	if (t->lMapIndex >= 10000)
	{
		m_posWarp.x = t->lExitX;
		m_posWarp.y = t->lExitY;
		m_lWarpMapIndex = t->lExitMapIndex;
	}

	SetRealPoint(POINT_PLAYTIME, t->playtime);
	m_dwLoginPlayTime = t->playtime;
	SetRealPoint(POINT_ST, t->st);
	SetRealPoint(POINT_HT, t->ht);
	SetRealPoint(POINT_DX, t->dx);
	SetRealPoint(POINT_IQ, t->iq);

	SetPoint(POINT_ST, t->st);
	SetPoint(POINT_HT, t->ht);
	SetPoint(POINT_DX, t->dx);
	SetPoint(POINT_IQ, t->iq);

	SetPoint(POINT_STAT, t->stat_point);
	SetPoint(POINT_SKILL, t->skill_point);
	SetPoint(POINT_SUB_SKILL, t->sub_skill_point);
	SetPoint(POINT_HORSE_SKILL, t->horse_skill_point);

	SetPoint(POINT_STAT_RESET_COUNT, t->stat_reset_count);

	SetPoint(POINT_LEVEL_STEP, t->level_step);
	SetRealPoint(POINT_LEVEL_STEP, t->level_step);

	SetRace(t->job);

	SetLevel(t->level);
	SetExp(t->exp);
	SetGold(t->gold);

	SetMapIndex(t->lMapIndex);
	SetXYZ(t->x, t->y, t->z);

	#ifdef ENABLE_OTO_TOPLAMA
	SetOtoMetin(t->oto_metin);
	SetOtoCanavar(t->oto_canavar);
	SetOtoBoss(t->oto_boss);
	#endif

	#ifdef ENABLE_ANTI_EXP
	SetAntiExp(t->anti_exp);
	#endif

	#ifdef __RANKING_SYSTEM__
	SetKillStone(t->killed_stone);
	SetKillBoss(t->killed_boss);
	SetDamageStone(t->damage_stone);
	SetDamageBoss(t->damage_boss);
	SetCompletedDungeon(t->success_dungeon);
	SetOpenedChest(t->open_chest);
	SetDamageSkill(t->damage_skill);
	SetDamageNormal(t->damage_normal);
	#endif

	#ifdef ENABLE_ALIGNMENT_BONUS
	SetAlignmentLevel(t->alignment_level);
	#endif

	#ifdef ENABLE_AUTO_ITEM_SELL_SYSTEM
	SetAutoSellItem(t->autoSellStatus);
	#endif

	ComputePoints();

	SetHP(t->hp);
	SetSP(t->sp);
	SetStamina(t->stamina);

	#ifndef ENABLE_GM_FLAG_IF_TEST_SERVER
	if (!test_server)
	#endif
	{
		#ifdef ENABLE_GM_FLAG_FOR_LOW_WIZARD
		if (GetGMLevel() > GM_PLAYER)
		#else
		if (GetGMLevel() > GM_LOW_WIZARD)
		#endif
		{
			m_afAffectFlag.Set(AFF_YMIR);
			m_bPKMode = PK_MODE_PROTECT;
		}
	}

	if (GetLevel() < PK_PROTECT_LEVEL)
		m_bPKMode = PK_MODE_PROTECT;

	m_stMobile = t->szMobile;

	SetHorseData(t->horse);

	if (GetHorseLevel() > 0)
		UpdateHorseDataByLogoff(t->logoff_interval);

	thecore_memcpy(m_aiPremiumTimes, t->aiPremiumTimes, sizeof(t->aiPremiumTimes));

	m_dwLogOffInterval = t->logoff_interval;

	sys_log(0, "PLAYER_LOAD: %s PREMIUM %d %d, LOGGOFF_INTERVAL %u PTR: %p", t->name, m_aiPremiumTimes[0], m_aiPremiumTimes[1], t->logoff_interval, this);

	if (GetGMLevel() != GM_PLAYER)
	{
		LogManager::instance().CharLog(this, GetGMLevel(), "GM_LOGIN", "");
		sys_log(0, "GM_LOGIN(gmlevel=%d, name=%s(%d), pos=(%d, %d)", GetGMLevel(), GetName(), GetPlayerID(), GetX(), GetY());
	}

	#ifdef __PET_SYSTEM__
	if (m_petSystem)
	{
		m_petSystem->Destroy();
		delete m_petSystem;
	}

	m_petSystem = M2_NEW CPetSystem(this);
	#endif
}

EVENTFUNC(kill_ore_load_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "kill_ore_load_even> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	ch->m_pkMiningEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

void CHARACTER::SetProto(const CMob * pkMob)
{
	if (m_pkMobInst)
		M2_DELETE(m_pkMobInst);

	m_pkMobData = pkMob;
	m_pkMobInst = M2_NEW CMobInstance;

	m_bPKMode = PK_MODE_FREE;

	const TMobTable * t = &m_pkMobData->m_table;

	m_bCharType = t->bType;

	SetLevel(t->bLevel);
	SetEmpire(t->bEmpire);

	SetExp(t->dwExp);
	SetRealPoint(POINT_ST, t->bStr);
	SetRealPoint(POINT_DX, t->bDex);
	SetRealPoint(POINT_HT, t->bCon);
	SetRealPoint(POINT_IQ, t->bInt);

	ComputePoints();

	SetHP(GetMaxHP());
	SetSP(GetMaxSP());

	////////////////////
	m_pointsInstant.dwAIFlag = t->dwAIFlag;
	SetImmuneFlag(t->dwImmuneFlag);

	AssignTriggers(t);

	ApplyMobAttribute(t);

	#ifdef BOSS_DMG_RANKING
	for (BYTE i = 0; i < sizeof(specialBossVnum) / sizeof(specialBossVnum[0]); i++)
	{
		if (specialBossVnum[i] == GetRaceNum())
		{
			MarkAsSpecialBoss(true); 
			CreatePacketTimer();
			break;
		}
	}
	#endif

	if (IsStone())
	{
		DetermineDropMetinStone();
	}

	if (IsWarp() || IsGoto())
	{
		StartWarpNPCEvent();
	}

	CHARACTER_MANAGER::instance().RegisterRaceNumMap(this);

	// XXX X-mas santa hardcoding
	if (GetRaceNum() == xmas::MOB_SANTA_VNUM)
	{
		SetPoint(POINT_ATT_GRADE_BONUS, 10);
		SetPoint(POINT_DEF_GRADE_BONUS, 6);

		//m_dwPlayStartTime = get_dword_time() + 10 * 60 * 1000;
		m_dwPlayStartTime = get_dword_time() + 30 * 1000;
		if (test_server)
			m_dwPlayStartTime = get_dword_time() + 30 * 1000;
	}

	// XXX CTF GuildWar hardcoding
	if (warmap::IsWarFlag(GetRaceNum()))
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlag, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlag, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlag, &CHARACTER::EndStateEmpty);
	}

	if (warmap::IsWarFlagBase(GetRaceNum()))
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlagBase, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlagBase, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlagBase, &CHARACTER::EndStateEmpty);
	}

	if (m_bCharType == CHAR_TYPE_HORSE ||
			GetRaceNum() == 20101 ||
			GetRaceNum() == 20102 ||
			GetRaceNum() == 20103 ||
			GetRaceNum() == 20104 ||
			GetRaceNum() == 20105 ||
			GetRaceNum() == 20106 ||
			GetRaceNum() == 20107 ||
			GetRaceNum() == 20108 ||
			GetRaceNum() == 20109
	  )
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateHorse, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateMove, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateHorse, &CHARACTER::EndStateEmpty);
	}

	// MINING
	if (mining::IsVeinOfOre (GetRaceNum()))
	{
		char_event_info* info = AllocEventInfo<char_event_info>();

		info->ch = this;

		m_pkMiningEvent = event_create(kill_ore_load_event, info, PASSES_PER_SEC(number(7 * 60, 15 * 60)));
	}
	// END_OF_MINING
}

const TMobTable & CHARACTER::GetMobTable() const
{
	return m_pkMobData->m_table;
}

bool CHARACTER::IsRaceFlag(DWORD dwBit) const
{
	return m_pkMobData ? IS_SET(m_pkMobData->m_table.dwRaceFlag, dwBit) : 0;
}

DWORD CHARACTER::GetMobDamageMin() const
{
	return m_pkMobData->m_table.dwDamageRange[0];
}

DWORD CHARACTER::GetMobDamageMax() const
{
	return m_pkMobData->m_table.dwDamageRange[1];
}

float CHARACTER::GetMobDamageMultiply() const
{
	float fDamMultiply = GetMobTable().fDamMultiply;

	if (IsBerserk())
		fDamMultiply = fDamMultiply * 2.0f;

	return fDamMultiply;
}

DWORD CHARACTER::GetMobDropItemVnum() const
{
	return m_pkMobData->m_table.dwDropItemVnum;
}

bool CHARACTER::IsSummonMonster() const
{
	return GetSummonVnum() != 0;
}

DWORD CHARACTER::GetSummonVnum() const
{
	return m_pkMobData ? m_pkMobData->m_table.dwSummonVnum : 0;
}

DWORD CHARACTER::GetPolymorphItemVnum() const
{
	return m_pkMobData ? m_pkMobData->m_table.dwPolymorphItemVnum : 0;
}

DWORD CHARACTER::GetMonsterDrainSPPoint() const
{
	return m_pkMobData ? m_pkMobData->m_table.dwDrainSP : 0;
}

BYTE CHARACTER::GetMobRank() const
{
	if (!m_pkMobData)
		return MOB_RANK_KNIGHT;

	return m_pkMobData->m_table.bRank;
}

BYTE CHARACTER::GetMobSize() const
{
	if (!m_pkMobData)
		return MOBSIZE_MEDIUM;

	return m_pkMobData->m_table.bSize;
}

WORD CHARACTER::GetMobAttackRange() const
{
	switch (GetMobBattleType())
	{
		case BATTLE_TYPE_RANGE:
		case BATTLE_TYPE_MAGIC:
		{
			#ifdef ENABLE_MOB_RANGE_ITEM
			return GetAntiMobRange() ? 175 : m_pkMobData->m_table.wAttackRange + GetPoint(POINT_BOW_DISTANCE);
			#else
			return m_pkMobData->m_table.wAttackRange + GetPoint(POINT_BOW_DISTANCE);
			#endif
		}

		default:
		{
			#ifdef ENABLE_MOB_RANGE_ITEM
			return GetAntiMobRange() ? 175 : m_pkMobData->m_table.wAttackRange;
			#else
			return m_pkMobData->m_table.wAttackRange;
			#endif
		}
	}
}

BYTE CHARACTER::GetMobBattleType() const
{
	if (!m_pkMobData)
		return BATTLE_TYPE_MELEE;

	return (m_pkMobData->m_table.bBattleType);
}

void CHARACTER::ComputeBattlePoints()
{
	if (IsPolymorphed())
	{
		DWORD dwMobVnum = GetPolymorphVnum();
		const CMob * pMob = CMobManager::instance().Get(dwMobVnum);
		int iAtt = 0;
		int iDef = 0;

		if (pMob)
		{
			iAtt = GetLevel() * 2 + GetPolymorphPoint(POINT_ST) * 2;
			// lev + con
			iDef = GetLevel() + GetPolymorphPoint(POINT_HT) + pMob->m_table.wDef;
		}

		SetPoint(POINT_ATT_GRADE, iAtt);
		SetPoint(POINT_DEF_GRADE, iDef);
		SetPoint(POINT_MAGIC_ATT_GRADE, GetPoint(POINT_ATT_GRADE));
		SetPoint(POINT_MAGIC_DEF_GRADE, GetPoint(POINT_DEF_GRADE));
	}
	else if (IsPC())
	{
		SetPoint(POINT_ATT_GRADE, 0);
		SetPoint(POINT_DEF_GRADE, 0);
		SetPoint(POINT_CLIENT_DEF_GRADE, 0);
		SetPoint(POINT_MAGIC_ATT_GRADE, GetPoint(POINT_ATT_GRADE));
		SetPoint(POINT_MAGIC_DEF_GRADE, GetPoint(POINT_DEF_GRADE));

		//
		// ATK = 2lev + 2str
		//
		int iAtk = GetLevel() * 2;
		int iStatAtk = 0;

		switch (GetJob())
		{
			case JOB_WARRIOR:
			case JOB_SURA:
				iStatAtk = (2 * GetPoint(POINT_ST));
				break;

			case JOB_ASSASSIN:
				iStatAtk = (4 * GetPoint(POINT_ST) + 2 * GetPoint(POINT_DX)) / 3;
				break;

			case JOB_SHAMAN:
				iStatAtk = (4 * GetPoint(POINT_ST) + 2 * GetPoint(POINT_IQ)) / 3;
				break;

			#ifdef ENABLE_WOLFMAN_CHARACTER
			case JOB_WOLFMAN:
				iStatAtk = (2 * GetPoint(POINT_ST));
				break;
			#endif

			default:
				sys_err("invalid job %d", GetJob());
				iStatAtk = (2 * GetPoint(POINT_ST));
				break;
		}

		if (GetMountVnum() && iStatAtk < 2 * GetPoint(POINT_ST))
			iStatAtk = (2 * GetPoint(POINT_ST));

		iAtk += iStatAtk;

		if (GetMountVnum())
		{
			if (GetJob() == JOB_SURA && GetSkillGroup() == 1)
			{
				iAtk += (iAtk * GetHorseLevel()) / 60;
			}
			else
			{
				iAtk += (iAtk * GetHorseLevel()) / 30;
			}
		}

		//
		// ATK Setting
		//
		iAtk += GetPoint(POINT_ATT_GRADE_BONUS);

		PointChange(POINT_ATT_GRADE, iAtk);

		// DEF = LEV + CON + ARMOR
		int iShowDef = GetLevel() + GetPoint(POINT_HT);
		int iDef = GetLevel() + (int) (GetPoint(POINT_HT) / 1.25); // For Other
		int iArmor = 0;

		LPITEM pkItem;

		for (int i = 0; i < WEAR_MAX_NUM; ++i)
			if ((pkItem = GetWear(i)) && pkItem->GetType() == ITEM_ARMOR)
			{
				if (pkItem->GetSubType() == ARMOR_BODY || pkItem->GetSubType() == ARMOR_HEAD || pkItem->GetSubType() == ARMOR_FOOTS || pkItem->GetSubType() == ARMOR_SHIELD)
				{
					iArmor += pkItem->GetValue(1);
					iArmor += (2 * pkItem->GetValue(5));
				}
			}

		if ( true == IsHorseRiding() )
		{
			if (iArmor < GetHorseArmor())
				iArmor = GetHorseArmor();

			const char* pHorseName = CHorseNameManager::instance().GetHorseName(GetPlayerID());

			if (pHorseName != NULL && strlen(pHorseName))
			{
				iArmor += 20;
			}
		}

		iArmor += GetPoint(POINT_DEF_GRADE_BONUS);
		iArmor += GetPoint(POINT_PARTY_DEFENDER_BONUS);

		// INTERNATIONAL_VERSION
		PointChange(POINT_DEF_GRADE, iDef + iArmor);
		PointChange(POINT_CLIENT_DEF_GRADE, (iShowDef + iArmor) - GetPoint(POINT_DEF_GRADE));
		// END_OF_INTERNATIONAL_VERSION

		PointChange(POINT_MAGIC_ATT_GRADE, GetLevel() * 2 + GetPoint(POINT_IQ) * 2 + GetPoint(POINT_MAGIC_ATT_GRADE_BONUS));
		PointChange(POINT_MAGIC_DEF_GRADE, GetLevel() + (GetPoint(POINT_IQ) * 3 + GetPoint(POINT_HT)) / 3 + iArmor / 2 + GetPoint(POINT_MAGIC_DEF_GRADE_BONUS));
	}
	else
	{
		// 2lev + str * 2
		int iAtt = GetLevel() * 2 + GetPoint(POINT_ST) * 2;
		// lev + con
		int iDef = GetLevel() + GetPoint(POINT_HT) + GetMobTable().wDef;

		SetPoint(POINT_ATT_GRADE, iAtt);
		SetPoint(POINT_DEF_GRADE, iDef);
		SetPoint(POINT_MAGIC_ATT_GRADE, GetPoint(POINT_ATT_GRADE));
		SetPoint(POINT_MAGIC_DEF_GRADE, GetPoint(POINT_DEF_GRADE));
	}
}

void CHARACTER::ComputePoints()
{
	long lStat = GetPoint(POINT_STAT);
	long lStatResetCount = GetPoint(POINT_STAT_RESET_COUNT);
	long lSkillActive = GetPoint(POINT_SKILL);
	long lSkillSub = GetPoint(POINT_SUB_SKILL);
	long lSkillHorse = GetPoint(POINT_HORSE_SKILL);
	long lLevelStep = GetPoint(POINT_LEVEL_STEP);

	long lAttackerBonus = GetPoint(POINT_PARTY_ATTACKER_BONUS);
	long lTankerBonus = GetPoint(POINT_PARTY_TANKER_BONUS);
	long lBufferBonus = GetPoint(POINT_PARTY_BUFFER_BONUS);
	long lSkillMasterBonus = GetPoint(POINT_PARTY_SKILL_MASTER_BONUS);
	long lHasteBonus = GetPoint(POINT_PARTY_HASTE_BONUS);
	long lDefenderBonus = GetPoint(POINT_PARTY_DEFENDER_BONUS);

	const long lHPRecovery = GetPoint(POINT_HP_RECOVERY);
	const long lSPRecovery = GetPoint(POINT_SP_RECOVERY);

	memset(m_pointsInstant.points, 0, sizeof(m_pointsInstant.points));
	BuffOnAttr_ClearAll();
	m_SkillDamageBonus.clear();

	SetPoint(POINT_STAT, lStat);
	SetPoint(POINT_SKILL, lSkillActive);
	SetPoint(POINT_SUB_SKILL, lSkillSub);
	SetPoint(POINT_HORSE_SKILL, lSkillHorse);
	SetPoint(POINT_LEVEL_STEP, lLevelStep);
	SetPoint(POINT_STAT_RESET_COUNT, lStatResetCount);

	SetPoint(POINT_ST, GetRealPoint(POINT_ST));
	SetPoint(POINT_HT, GetRealPoint(POINT_HT));
	SetPoint(POINT_DX, GetRealPoint(POINT_DX));
	SetPoint(POINT_IQ, GetRealPoint(POINT_IQ));

	SetPart(PART_MAIN, GetOriginalPart(PART_MAIN));
	SetPart(PART_WEAPON, GetOriginalPart(PART_WEAPON));
	SetPart(PART_HEAD, GetOriginalPart(PART_HEAD));
	SetPart(PART_HAIR, GetOriginalPart(PART_HAIR));
	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	SetPart(PART_ACCE, GetOriginalPart(PART_ACCE));
	#endif

	SetPoint(POINT_PARTY_ATTACKER_BONUS, lAttackerBonus);
	SetPoint(POINT_PARTY_TANKER_BONUS, lTankerBonus);
	SetPoint(POINT_PARTY_BUFFER_BONUS, lBufferBonus);
	SetPoint(POINT_PARTY_SKILL_MASTER_BONUS, lSkillMasterBonus);
	SetPoint(POINT_PARTY_HASTE_BONUS, lHasteBonus);
	SetPoint(POINT_PARTY_DEFENDER_BONUS, lDefenderBonus);

	SetPoint(POINT_HP_RECOVERY, lHPRecovery);
	SetPoint(POINT_SP_RECOVERY, lSPRecovery);

	int64_t iMaxHP; //Fixme mobs Hp limit
	int iMaxSP;
	int iMaxStamina;

	if (IsPC())
	{
		iMaxHP = JobInitialPoints[GetJob()].max_hp + m_points.iRandomHP + GetPoint(POINT_HT) * JobInitialPoints[GetJob()].hp_per_ht;
		iMaxSP = JobInitialPoints[GetJob()].max_sp + m_points.iRandomSP + GetPoint(POINT_IQ) * JobInitialPoints[GetJob()].sp_per_iq;
		iMaxStamina = JobInitialPoints[GetJob()].max_stamina + GetPoint(POINT_HT) * JobInitialPoints[GetJob()].stamina_per_con;

		{
			CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_ADD_HP);
			if (NULL != pkSk)
			{
				pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_ADD_HP) / 100.0f);
				iMaxHP += static_cast<int64_t>(pkSk->kPointPoly.Eval());
			}
		}

		#ifdef ENABLE_PASIF_SKILL
		{
			CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_PASSIVE_HP);
			if (NULL != pkSk)
			{
				pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_PASSIVE_HP) / 100.0f);
				int ekle = static_cast<int>(pkSk->kPointPoly.Eval());
				PointChange(POINT_MAX_HP, ekle);
			}
		}

		{
			CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_PASSIVE_METINS);
			if (NULL != pkSk)
			{
				pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_PASSIVE_METINS) / 100.0f);
				int ekle = static_cast<int>(pkSk->kPointPoly.Eval());
				PointChange(POINT_ATTBONUS_STONE, ekle);
			}
		}

		{
			CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_PASSIVE_BOSSES);
			if (NULL != pkSk)
			{
				pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_PASSIVE_BOSSES) / 100.0f);
				int ekle = static_cast<int>(pkSk->kPointPoly.Eval());
				PointChange(POINT_ATTBONUS_BOSS, ekle);
			}
		}

		{
			CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_PASSIVE_MONSTERS);
			if (NULL != pkSk)
			{
				pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_PASSIVE_MONSTERS) / 100.0f);
				int ekle = static_cast<int>(pkSk->kPointPoly.Eval());
				PointChange(POINT_ATTBONUS_MONSTER, ekle);
			}
		}

		{
			CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_PASSIVE_HUMANS);
			if (NULL != pkSk)
			{
				pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_PASSIVE_HUMANS) / 100.0f);
				int ekle = static_cast<int>(pkSk->kPointPoly.Eval());
				PointChange(POINT_RESIST_HUMAN, ekle);
			}
		}
		{
			CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_PASSIVE_PVM);
			if (NULL != pkSk)
			{
				pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_PASSIVE_PVM) / 100.0f);
				int ekle = static_cast<int>(pkSk->kPointPoly.Eval());
				PointChange(POINT_HIT_DAMAGE_BONUS_PVM, ekle);
			}
		}
		{
			CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_PASSIVE_DROP);
			if (NULL != pkSk)
			{
				pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_PASSIVE_DROP) / 100.0f);
				int ekle = static_cast<int>(pkSk->kPointPoly.Eval());
				PointChange(POINT_ITEM_DROP_BONUS, ekle);
			}
		}
		// {
			// CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_PASSIVE_SANS);
			// if (NULL != pkSk)
			// {
				// pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_PASSIVE_SANS) / 100.0f);
				// int ekle = static_cast<int>(pkSk->kPointPoly.Eval());
				// PointChange(POINT_ITEM_DROP_BONUS, ekle);
			// }
		// }
		#endif

		#ifdef ENABLE_STANDING_MOUNT
		if (GetMountVnum() == SURFBOARD)
		{
			SetPoint(POINT_MOV_SPEED,	150);
		}
		else
		#endif
		{
			SetPoint(POINT_MOV_SPEED,	100);
		}

		SetPoint(POINT_ATT_SPEED,	100);
		PointChange(POINT_ATT_SPEED, GetPoint(POINT_PARTY_HASTE_BONUS));
		SetPoint(POINT_CASTING_SPEED,	100);
	}
	else
	{
		iMaxHP = m_pkMobData->m_table.dwMaxHP;
		iMaxSP = 0;
		iMaxStamina = 0;

		SetPoint(POINT_ATT_SPEED, m_pkMobData->m_table.sAttackSpeed);
		SetPoint(POINT_MOV_SPEED, m_pkMobData->m_table.sMovingSpeed);
		SetPoint(POINT_CASTING_SPEED, m_pkMobData->m_table.sAttackSpeed);
	}

	if (IsPC())
	{
		#ifdef ENABLE_STANDING_MOUNT
		if (GetMountVnum() && (GetMountVnum() != SURFBOARD))
		#else
		if (GetMountVnum())
		#endif
		{
			if (GetHorseST() > GetPoint(POINT_ST))
				PointChange(POINT_ST, GetHorseST() - GetPoint(POINT_ST));

			if (GetHorseDX() > GetPoint(POINT_DX))
				PointChange(POINT_DX, GetHorseDX() - GetPoint(POINT_DX));

			if (GetHorseHT() > GetPoint(POINT_HT))
				PointChange(POINT_HT, GetHorseHT() - GetPoint(POINT_HT));

			if (GetHorseIQ() > GetPoint(POINT_IQ))
				PointChange(POINT_IQ, GetHorseIQ() - GetPoint(POINT_IQ));
		}
	}

	ComputeBattlePoints();

	if (iMaxHP != GetMaxHP())
	{
		SetRealPoint(POINT_MAX_HP, iMaxHP);
	}

	PointChange(POINT_MAX_HP, 0);

	if (iMaxSP != GetMaxSP())
	{
		SetRealPoint(POINT_MAX_SP, iMaxSP);
	}

	PointChange(POINT_MAX_SP, 0);

	SetMaxStamina(iMaxStamina);

	// @fixme118 part1
	int64_t iCurHP = this->GetHP(); //Fixme mobs Hp limit
	int iCurSP = this->GetSP();

	m_pointsInstant.dwImmuneFlag = 0;

	if (IsPC())
	{
		for (int i = 0 ; i < WEAR_MAX_NUM; i++)
		{
			LPITEM pItem = GetWear(i);
			if (pItem)
			{
				pItem->ModifyPoints(true);
				SET_BIT(m_pointsInstant.dwImmuneFlag, GetWear(i)->GetImmuneFlag());
			}
		}

		if (DragonSoul_IsDeckActivated())
		{
			for (int i = WEAR_MAX_NUM + DS_SLOT_MAX * DragonSoul_GetActiveDeck();
				i < WEAR_MAX_NUM + DS_SLOT_MAX * (DragonSoul_GetActiveDeck() + 1); i++)
			{
				LPITEM pItem = GetWear(i);
				if (pItem)
				{
					if (DSManager::instance().IsTimeLeftDragonSoul(pItem))
						pItem->ModifyPoints(true);
				}
			}
		}

		#ifdef ENABLE_EVENT_MANAGER
		CHARACTER_MANAGER::Instance().CheckBonusEvent(this);
		#endif

		#ifdef ENABLE_ALIGNMENT_BONUS
		CheckAlignmentBonus();
		#endif
	}

	if (GetHP() > GetMaxHP())
		PointChange(POINT_HP, GetMaxHP() - GetHP());

	if (GetSP() > GetMaxSP())
		PointChange(POINT_SP, GetMaxSP() - GetSP());

	ComputeSkillPoints();

	RefreshAffect();

	if (IsPC())
	{
	#ifndef ENABLE_COSTUME_PET_SYSTEM
		#ifdef __PET_SYSTEM__
		CPetSystem * pPetSystem = GetPetSystem();
		if (pPetSystem)
			pPetSystem->RefreshBuff();
		#endif
	#endif

		// @fixme118 part2 (after petsystem stuff)
		if (this->GetHP() != iCurHP)
			this->PointChange(POINT_HP, iCurHP-this->GetHP());

		if (this->GetSP() != iCurSP)
			this->PointChange(POINT_SP, iCurSP-this->GetSP());
	}

	UpdatePacket();
}

void CHARACTER::ResetPlayTime(DWORD dwTimeRemain)
{
	m_dwPlayStartTime = get_dword_time() - dwTimeRemain;
}

const int aiRecoveryPercents[10] = { 1, 5, 5, 5, 5, 5, 5, 5, 5, 5 };

EVENTFUNC(recovery_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "recovery_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (!ch->IsPC())
	{
		if (ch->IsAffectFlag(AFF_POISON))
			return PASSES_PER_SEC(MAX(1, ch->GetMobTable().bRegenCycle));

		#ifdef ENABLE_WOLFMAN_CHARACTER
		if (ch->IsAffectFlag(AFF_BLEEDING))
			return PASSES_PER_SEC(MAX(1, ch->GetMobTable().bRegenCycle));
		#endif

		if (2493 == ch->GetMobTable().dwVnum)
		{
			int regenPct = BlueDragon_GetRangeFactor("hp_regen", ch->GetHPPct());
			regenPct += ch->GetMobTable().bRegenPercent;

			for (int i=1 ; i <= 4 ; ++i)
			{
				if (REGEN_PECT_BONUS == BlueDragon_GetIndexFactor("DragonStone", i, "effect_type"))
				{
					DWORD dwDragonStoneID = BlueDragon_GetIndexFactor("DragonStone", i, "vnum");
					size_t val = BlueDragon_GetIndexFactor("DragonStone", i, "val");
					size_t cnt = SECTREE_MANAGER::instance().GetMonsterCountInMap( ch->GetMapIndex(), dwDragonStoneID );

					regenPct += (val*cnt);

					break;
				}
			}

			ch->PointChange(POINT_HP, MAX(1, (ch->GetMaxHP() * regenPct) / 100));
		}
		else if (!ch->IsDoor())
		{
			ch->MonsterLog("HP_REGEN +%d", MAX(1, (ch->GetMaxHP() * ch->GetMobTable().bRegenPercent) / 100));
			ch->PointChange(POINT_HP, MAX(1, (ch->GetMaxHP() * ch->GetMobTable().bRegenPercent) / 100));
		}

		if (ch->GetHP() >= ch->GetMaxHP())
		{
			ch->m_pkRecoveryEvent = NULL;
			return 0;
		}

		if (2493 == ch->GetMobTable().dwVnum)
		{
			for (int i=1 ; i <= 4 ; ++i)
			{
				if (REGEN_TIME_BONUS == BlueDragon_GetIndexFactor("DragonStone", i, "effect_type"))
				{
					DWORD dwDragonStoneID = BlueDragon_GetIndexFactor("DragonStone", i, "vnum");
					size_t val = BlueDragon_GetIndexFactor("DragonStone", i, "val");
					size_t cnt = SECTREE_MANAGER::instance().GetMonsterCountInMap( ch->GetMapIndex(), dwDragonStoneID );

					return PASSES_PER_SEC(MAX(1, (ch->GetMobTable().bRegenCycle - (val*cnt))));
				}
			}
		}

		return PASSES_PER_SEC(MAX(1, ch->GetMobTable().bRegenCycle));
	}
	else
	{
		ch->CheckTarget();
		ch->UpdateKillerMode();

		if (ch->IsAffectFlag(AFF_POISON) == true)
		{
			return 3;
		}

		#ifdef ENABLE_WOLFMAN_CHARACTER
		if (ch->IsAffectFlag(AFF_BLEEDING))
			return 3;
		#endif

		int iSec = (get_dword_time() - ch->GetLastMoveTime()) / 3000;

		ch->DistributeSP(ch);

		if (ch->GetMaxHP() <= ch->GetHP())
			return PASSES_PER_SEC(3);

		int iPercent = 0;
		int iAmount = 0;

		{
			iPercent = aiRecoveryPercents[MIN(9, iSec)];
			iAmount = 15 + (ch->GetMaxHP() * iPercent) / 100;
		}

		iAmount += (iAmount * ch->GetPoint(POINT_HP_REGEN)) / 100;

		sys_log(1, "RECOVERY_EVENT: %s %d HP_REGEN %d HP +%d", ch->GetName(), iPercent, ch->GetPoint(POINT_HP_REGEN), iAmount);

		ch->PointChange(POINT_HP, iAmount, false);
		return PASSES_PER_SEC(3);
	}
}

void CHARACTER::StartRecoveryEvent()
{
	if (m_pkRecoveryEvent)
		return;

	if (IsDead() || IsStun())
		return;

	if (IsNPC() && GetHP() >= GetMaxHP())
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	int iSec = IsPC() ? 3 : (MAX(1, GetMobTable().bRegenCycle));
	m_pkRecoveryEvent = event_create(recovery_event, info, PASSES_PER_SEC(iSec));
}

void CHARACTER::Standup()
{
	struct packet_position pack_position;

	if (!IsPosition(POS_SITTING))
		return;

	SetPosition(POS_STANDING);

	sys_log(1, "STANDUP: %s", GetName());

	pack_position.header	= HEADER_GC_CHARACTER_POSITION;
	pack_position.vid		= GetVID();
	pack_position.position	= POSITION_GENERAL;

	PacketAround(pack_position);
}

void CHARACTER::Sitdown(int is_ground)
{
	struct packet_position pack_position;

	if (IsPosition(POS_SITTING))
		return;

	SetPosition(POS_SITTING);
	sys_log(1, "SITDOWN: %s", GetName());

	pack_position.header	= HEADER_GC_CHARACTER_POSITION;
	pack_position.vid		= GetVID();
	pack_position.position	= POSITION_SITTING_GROUND;
	PacketAround(pack_position);
}

void CHARACTER::SetRotation(float fRot)
{
	m_pointsInstant.fRot = fRot;
}

void CHARACTER::SetRotationToXY(long x, long y)
{
	SetRotation(GetDegreeFromPositionXY(GetX(), GetY(), x, y));
}

bool CHARACTER::CannotMoveByAffect() const
{
	return (IsAffectFlag(AFF_STUN));
}

bool CHARACTER::CanMove() const
{
	if (CannotMoveByAffect())
		return false;

	if (GetMyShop())
		return false;

	/*
	   if (get_float_time() - m_fSyncTime < 0.2f)
	   return false;
	 */
	return true;
}

bool CHARACTER::Sync(long x, long y)
{
	if (!GetSectree())
		return false;

	if (IsPC() && IsDead())// @fixme192
		return false;

	LPSECTREE new_tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), x, y);
	if (!new_tree)
	{
		if (GetDesc())
		{
			sys_err("cannot find tree at %d %d (name: %s)", x, y, GetName());
			// @fixme313 BEGIN
			new_tree = GetSectree();
			x = GetX();
			y = GetY();
			if (!new_tree)
			{
				GetDesc()->SetPhase(PHASE_CLOSE);
				sys_err("cannot find old tree at %d %d (name: %s)", x, y, GetName());
			}
			// @fixme313 END
		}
		else
		{
			sys_err("no tree: %s %d %d %d", GetName(), x, y, GetMapIndex());
			Dead();
		}

		return false;
	}

	SetRotationToXY(x, y);
	SetXYZ(x, y, 0);

	if (GetDungeon())
	{
		const int iLastEventAttr = m_iEventAttr;
		m_iEventAttr = new_tree->GetEventAttribute(x, y);

		if (m_iEventAttr != iLastEventAttr)
		{
			if (GetParty())
			{
				quest::CQuestManager::instance().AttrOut(GetParty()->GetLeaderPID(), this, iLastEventAttr);
				quest::CQuestManager::instance().AttrIn(GetParty()->GetLeaderPID(), this, m_iEventAttr);
			}
			else
			{
				quest::CQuestManager::instance().AttrOut(GetPlayerID(), this, iLastEventAttr);
				quest::CQuestManager::instance().AttrIn(GetPlayerID(), this, m_iEventAttr);
			}
		}
	}

	if (GetSectree() != new_tree)
	{
		if (!IsNPC())
		{
			const SECTREEID id = new_tree->GetID();
			const SECTREEID old_id = GetSectree()->GetID();

			const float fDist = DISTANCE_SQRT(id.coord.x - old_id.coord.x, id.coord.y - old_id.coord.y);
			sys_log(0, "SECTREE DIFFER: %s %dx%d was %dx%d dist %.1fm",
					GetName(),
					id.coord.x,
					id.coord.y,
					old_id.coord.x,
					old_id.coord.y,
					fDist);
		}

		new_tree->InsertEntity(this);
	}

	return true;
}

void CHARACTER::Stop()
{
	if (!IsState(m_stateIdle))
		MonsterLog("[IDLE] 정지");

	GotoState(m_stateIdle);

	m_posDest.x = m_posStart.x = GetX();
	m_posDest.y = m_posStart.y = GetY();
}

bool CHARACTER::Goto(long x, long y)
{
	if (GetX() == x && GetY() == y)
		return false;

	if (m_posDest.x == x && m_posDest.y == y)
	{
		if (!IsState(m_stateMove))
		{
			m_dwStateDuration = 4;
			GotoState(m_stateMove);
		}
		return false;
	}

	m_posDest.x = x;
	m_posDest.y = y;

	CalculateMoveDuration();

	m_dwStateDuration = 4;

	if (!IsState(m_stateMove))
	{
		MonsterLog("[MOVE] %s", GetVictim() ? "대상추적" : "그냥이동");

		if (GetVictim())
		{
			//MonsterChat(MONSTER_CHAT_CHASE);
			MonsterChat(MONSTER_CHAT_ATTACK);
		}
	}

	GotoState(m_stateMove);
	return true;
}

DWORD CHARACTER::GetMotionMode() const
{
	DWORD dwMode = MOTION_MODE_GENERAL;

	if (IsPolymorphed())
		return dwMode;

	LPITEM pkItem;

	if ((pkItem = GetWear(WEAR_WEAPON)))
	{
		switch (pkItem->GetProto()->bSubType)
		{
			case WEAPON_SWORD:
				dwMode = MOTION_MODE_ONEHAND_SWORD;
				break;

			case WEAPON_TWO_HANDED:
				dwMode = MOTION_MODE_TWOHAND_SWORD;
				break;

			case WEAPON_DAGGER:
				dwMode = MOTION_MODE_DUALHAND_SWORD;
				break;

			case WEAPON_BOW:
				dwMode = MOTION_MODE_BOW;
				break;

			case WEAPON_BELL:
				dwMode = MOTION_MODE_BELL;
				break;

			case WEAPON_FAN:
				dwMode = MOTION_MODE_FAN;
				break;

			#ifdef ENABLE_WOLFMAN_CHARACTER
			case WEAPON_CLAW:
				dwMode = MOTION_MODE_CLAW;
				break;
			#endif
		}
	}

	return dwMode;
}

float CHARACTER::GetMoveMotionSpeed() const
{
	DWORD dwMode = GetMotionMode();

	const CMotion * pkMotion = NULL;

	if (!GetMountVnum())
		pkMotion = CMotionManager::instance().GetMotion(GetRaceNum(), MAKE_MOTION_KEY(dwMode, (IsWalking() && IsPC()) ? MOTION_WALK : MOTION_RUN));
	else
	{
		pkMotion = CMotionManager::instance().GetMotion(GetMountVnum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, (IsWalking() && IsPC()) ? MOTION_WALK : MOTION_RUN));

		if (!pkMotion)
			pkMotion = CMotionManager::instance().GetMotion(GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_HORSE, (IsWalking() && IsPC()) ? MOTION_WALK : MOTION_RUN));
	}

	if (pkMotion)
		return -pkMotion->GetAccumVector().y / pkMotion->GetDuration();
	else
	{
		// sys_err("cannot find motion (name %s race %d mode %d)", GetName(), GetRaceNum(), dwMode);
		return 300.0f;
	}
}

float CHARACTER::GetMoveSpeed() const
{
	return GetMoveMotionSpeed() * 10000 / CalculateDuration(GetLimitPoint(POINT_MOV_SPEED), 10000);
}

void CHARACTER::CalculateMoveDuration()
{
	m_posStart.x = GetX();
	m_posStart.y = GetY();

	float fDist = DISTANCE_SQRT(m_posStart.x - m_posDest.x, m_posStart.y - m_posDest.y);

	float motionSpeed = GetMoveMotionSpeed();

	m_dwMoveDuration = CalculateDuration(GetLimitPoint(POINT_MOV_SPEED),
			(int) ((fDist / motionSpeed) * 1000.0f));

	if (IsNPC())
		sys_log(1, "%s: GOTO: distance %f, spd %u, duration %u, motion speed %f pos %d %d -> %d %d",
				GetName(), fDist, GetLimitPoint(POINT_MOV_SPEED), m_dwMoveDuration, motionSpeed,
				m_posStart.x, m_posStart.y, m_posDest.x, m_posDest.y);

	m_dwMoveStartTime = get_dword_time();
}

bool CHARACTER::Move(long x, long y)
{
	if (IsPC() && IsDead())// @fixme226
		return false;

	if (GetX() == x && GetY() == y)
		return true;

	if (test_server)
		if (m_bDetailLog)
			sys_log(0, "%s position %u %u", GetName(), x, y);

	#ifdef NEW_MOB_TIMER
	if (GetSpawnX() > 0 || GetSpawnY() > 0) { CheckSecurityMobRange(); }
	#endif

	OnMove();
	return Sync(x, y);
}

void CHARACTER::SendMovePacket(BYTE bFunc, BYTE bArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime, int iRot)
{
	TPacketGCMove pack;

	if (bFunc == FUNC_WAIT)
	{
		x = m_posDest.x;
		y = m_posDest.y;
		dwDuration = m_dwMoveDuration;
	}

	EncodeMovePacket(pack, GetVID(), bFunc, bArg, x, y, dwDuration, dwTime, iRot == -1 ? (int) GetRotation() / 5 : iRot);
	PacketView(&pack, sizeof(TPacketGCMove), this);
}

int64_t CHARACTER::GetRealPoint(BYTE type) const
{
	return m_points.points[type];
}

void CHARACTER::SetRealPoint(BYTE type, int64_t val)
{
	m_points.points[type] = val;
}

int CHARACTER::GetPolymorphPoint(BYTE type) const
{
	if (IsPolymorphed() && !IsPolyMaintainStat())
	{
		DWORD dwMobVnum = GetPolymorphVnum();
		const CMob * pMob = CMobManager::instance().Get(dwMobVnum);
		int iPower = GetPolymorphPower();

		if (pMob)
		{
			switch (type)
			{
				case POINT_ST:
					if ((GetJob() == JOB_SHAMAN) || ((GetJob() == JOB_SURA) && (GetSkillGroup() == 2)))
						return pMob->m_table.bStr * iPower / 100 + GetPoint(POINT_IQ);
					return pMob->m_table.bStr * iPower / 100 + GetPoint(POINT_ST);

				case POINT_HT:
					return pMob->m_table.bCon * iPower / 100 + GetPoint(POINT_HT);

				case POINT_IQ:
					return pMob->m_table.bInt * iPower / 100 + GetPoint(POINT_IQ);

				case POINT_DX:
					return pMob->m_table.bDex * iPower / 100 + GetPoint(POINT_DX);
			}
		}
	}

	return GetPoint(type);
}

int64_t CHARACTER::GetPoint(BYTE type) const
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return 0;
	}

	int64_t val = m_pointsInstant.points[type];
	int64_t max_val = INT_MAX;

	switch (type)
	{
		case POINT_STEAL_HP:
		case POINT_STEAL_SP:
			max_val = 50;
			break;
	}

	if ((type != POINT_HP) && (type != POINT_MAX_HP))
	{
		if (val > max_val)
		{
			sys_err("POINT_ERROR: %s type %d val %lld (max: %lld)", GetName(), type, val, max_val); //@fixme225
		}
	}

	return (val);
}

int CHARACTER::GetLimitPoint(BYTE type) const
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return 0;
	}

	int val = static_cast<int>(m_pointsInstant.points[type]);
	int max_val = INT_MAX;
	int limit = INT_MAX;
	int min_limit = -INT_MAX;

	switch (type)
	{
		case POINT_ATT_SPEED:
			min_limit = 0;

			if (IsPC())
				limit = 170;
			else
				limit = 250;
			break;

		case POINT_MOV_SPEED:
			min_limit = 0;

			if (IsPC())
				limit = 200;
			else
				limit = 250;
			break;

		case POINT_STEAL_HP:
		case POINT_STEAL_SP:
			limit = 50;
			max_val = 50;
			break;

		case POINT_MALL_ATTBONUS:
		case POINT_MALL_DEFBONUS:
			limit = 20;
			max_val = 50;
			break;
	}

	if (val > max_val)
		sys_err("POINT_ERROR: %s type %d val %d (max: %d)", GetName(), val, max_val);

	if (val > limit)
		val = limit;

	if (val < min_limit)
		val = min_limit;

	return (val);
}

void CHARACTER::SetPoint(BYTE type, int64_t val)
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return;
	}

	m_pointsInstant.points[type] = val;

	if (type == POINT_MOV_SPEED && get_dword_time() < m_dwMoveStartTime + m_dwMoveDuration)
	{
		CalculateMoveDuration();
	}
}

#ifdef ENABLE_FULL_YANG
uint64_t CHARACTER::GetAllowedGold()
{
	// return 999000000000000001LL;
	return FULL_YANG_MAX;
}

void CHARACTER::GoldChange(int64_t ldGold)
{
	int64_t nTotalMoney = GetGold() + ldGold;

	if (GetAllowedGold() <= nTotalMoney)
	{
		sys_err("[OVERFLOW_GOLD] OriGold %lld AddedGold %lld id %u Name %s ", GetGold(), ldGold, GetPlayerID(), GetName());
		LogManager::instance().CharLog(this, GetGold() + ldGold, "OVERFLOW_GOLD", "");
		return;
	}

	SetGold(GetGold() + ldGold);

	if (GetDesc())
	{
		TPacketGCGoldChange pack;

		pack.header = HEADER_GC_GOLD_CHANGE;
		pack.dwVID = m_vid;

		if (ldGold <= 0)
			pack.ldAmount = 0;
		else
			pack.ldAmount = ldGold;


		pack.ldValue = GetGold();
		GetDesc()->Packet(&pack, sizeof(TPacketGCGoldChange));
	}
}
#else
INT CHARACTER::GetAllowedGold() const
{
	if (GetLevel() <= 10)
		return 100000;
	else if (GetLevel() <= 20)
		return 500000;
	else
		return 50000000;
}
#endif

void CHARACTER::CheckMaximumPoints()
{
	if (GetMaxHP() < GetHP())
		PointChange(POINT_HP, GetMaxHP() - GetHP());

	if (GetMaxSP() < GetSP())
		PointChange(POINT_SP, GetMaxSP() - GetSP());
}

void CHARACTER::PointChange(BYTE type, int64_t amount, bool bAmount, bool bBroadcast)
{
	int64_t val = 0;
	//sys_log(0, "PointChange %d %d | %d -> %d cHP %d mHP %d", type, amount, GetPoint(type), GetPoint(type)+amount, GetHP(), GetMaxHP());

	switch (type)
	{
		case POINT_NONE:
		#ifdef ENABLE_BATTLE_PASS
		case POINT_BATTLE_PASS_ID:
		#endif
			return;

		case POINT_LEVEL:
			if ((GetLevel() + amount) > gPlayerMaxLevel)
				return;

			SetLevel(GetLevel() + amount);
			val = GetLevel();

			sys_log(0, "LEVELUP: %s %d NEXT EXP %d", GetName(), GetLevel(), GetNextExp());

			#ifdef ENABLE_WOLFMAN_CHARACTER
			if (GetJob() == JOB_WOLFMAN)
			{
				if ((5 <= val) && (GetSkillGroup()!=1))
				{
					ClearSkill();
					// set skill group
					SetSkillGroup(1);
					// set skill points
					SetRealPoint(POINT_SKILL, GetLevel()-1);
					SetPoint(POINT_SKILL, GetRealPoint(POINT_SKILL));
					PointChange(POINT_SKILL, 0);
					// update points (not required)
					// ComputePoints();
					// PointsPacket();
				}
			}
			#endif

			PointChange(POINT_NEXT_EXP, GetNextExp(), false);

			if (amount)
			{
				quest::CQuestManager::instance().LevelUp(GetPlayerID());

				#ifdef ENABLE_BIYOLOG
				CheckBio();
				#endif

				LogManager::instance().LevelLog(this, val, GetRealPoint(POINT_PLAYTIME) + (get_dword_time() - m_dwPlayStartTime) / 60000);

				if (GetGuild())
				{
					GetGuild()->LevelChange(GetPlayerID(), GetLevel());
				}

				if (GetParty())
				{
					GetParty()->RequestSetMemberLevel(GetPlayerID(), GetLevel());
				}
			}
			break;

		case POINT_NEXT_EXP:
			val = GetNextExp();
			bAmount = false;
			break;

		case POINT_EXP:
			{
				DWORD exp = GetExp();
				DWORD next_exp = GetNextExp();

				if (g_bChinaIntoxicationCheck)
				{
					if (IsOverTime(OT_NONE))
					{
					}
					else if (IsOverTime(OT_3HOUR))
					{
						amount = (amount / 2);
					}
					else if (IsOverTime(OT_5HOUR))
					{
						amount = 0;
					}
				}

				if ((amount < 0) && (exp < (DWORD)(-amount)))
				{
					sys_log(1, "%s AMOUNT < 0 %lld, CUR EXP: %d", GetName(), -amount, exp);
					amount = -exp;

					SetExp(exp + amount);
					val = GetExp();
				}
				else
				{
					if (gPlayerMaxLevel <= GetLevel())
						return;

					#ifdef ENABLE_ANTI_EXP
					if (GetAntiExp() == 1 && amount > 0)
						return;
					#endif

					if (test_server)
						ChatPacket(CHAT_TYPE_INFO, "You have gained %lld exp.", amount);

					DWORD iExpBalance = 0;

					if (exp + amount >= next_exp)
					{
						iExpBalance = (exp + amount) - next_exp;
						amount = next_exp - exp;

						SetExp(0);
						exp = next_exp;
					}
					else
					{
						SetExp(exp + amount);
						exp = GetExp();
					}

					DWORD q = DWORD(next_exp / 4.0f);
					int iLevStep = GetRealPoint(POINT_LEVEL_STEP);

					if (iLevStep >= 4)
					{
						sys_err("%s LEVEL_STEP bigger than 4! (%d)", GetName(), iLevStep);
						iLevStep = 4;
					}

					if (exp >= next_exp && iLevStep < 4)
					{
						for (int i = 0; i < 4 - iLevStep; ++i)
							PointChange(POINT_LEVEL_STEP, 1, false, true);
					}
					else if (exp >= q * 3 && iLevStep < 3)
					{
						for (int i = 0; i < 3 - iLevStep; ++i)
							PointChange(POINT_LEVEL_STEP, 1, false, true);
					}
					else if (exp >= q * 2 && iLevStep < 2)
					{
						for (int i = 0; i < 2 - iLevStep; ++i)
							PointChange(POINT_LEVEL_STEP, 1, false, true);
					}
					else if (exp >= q && iLevStep < 1)
						PointChange(POINT_LEVEL_STEP, 1);

					if (iExpBalance)
					{
						PointChange(POINT_EXP, iExpBalance);
					}

					val = GetExp();
				}
			}
			break;

		case POINT_LEVEL_STEP:
			if (amount > 0)
			{
				val = GetPoint(POINT_LEVEL_STEP) + amount;

				switch (val)
				{
					case 1:
					case 2:
					case 3:
						if ((GetLevel() <= g_iStatusPointGetLevelLimit) &&
							(GetLevel() <= gPlayerMaxLevel) ) // @fixme104
							PointChange(POINT_STAT, 1);
						break;

					case 4:
						{
							int iHP = number(JobInitialPoints[GetJob()].hp_per_lv_begin, JobInitialPoints[GetJob()].hp_per_lv_end);
							int iSP = number(JobInitialPoints[GetJob()].sp_per_lv_begin, JobInitialPoints[GetJob()].sp_per_lv_end);

							m_points.iRandomHP += iHP;
							m_points.iRandomSP += iSP;

							if (GetSkillGroup())
							{
								if (GetLevel() >= 5)
									PointChange(POINT_SKILL, 1);

								if (GetLevel() >= 9)
									PointChange(POINT_SUB_SKILL, 1);
							}

							PointChange(POINT_MAX_HP, iHP);
							PointChange(POINT_MAX_SP, iSP);
							PointChange(POINT_LEVEL, 1, false, true);

							val = 0;
						}
						break;
				}



				PointChange(POINT_HP, GetMaxHP() - GetHP());
				PointChange(POINT_SP, GetMaxSP() - GetSP());
				PointChange(POINT_STAMINA, GetMaxStamina() - GetStamina());

				SetPoint(POINT_LEVEL_STEP, val);
				SetRealPoint(POINT_LEVEL_STEP, val);

				Save();
			}
			else
				val = GetPoint(POINT_LEVEL_STEP);

			break;

		case POINT_HP:
			{
				if (IsDead() || IsStun())
					return;

				int64_t prev_hp = GetHP();

				amount = MIN(GetMaxHP() - GetHP(), amount);
				SetHP(GetHP() + amount);
				val = GetHP();

				BroadcastTargetPacket();

				if (GetParty() && IsPC() && val != prev_hp)
					GetParty()->SendPartyInfoOneToAll(this);
			}
			break;

		case POINT_SP:
			{
				if (IsDead() || IsStun())
					return;

				amount = MIN(GetMaxSP() - GetSP(), amount);
				SetSP(GetSP() + amount);
				val = GetSP();
			}
			break;

		case POINT_STAMINA:
			{
				if (IsDead() || IsStun())
					return;

				int prev_val = GetStamina();
				amount = MIN(GetMaxStamina() - GetStamina(), amount);
				SetStamina(GetStamina() + amount);
				val = GetStamina();

				if (val == 0)
				{
					// Stamina
					SetNowWalking(true);
				}
				else if (prev_val == 0)
				{
					ResetWalking();
				}

				if (amount < 0 && val != 0)
					return;
			}
			break;

		case POINT_MAX_HP:
			{
				SetPoint(type, GetPoint(type) + amount);

				//SetMaxHP(GetMaxHP() + amount);
				int64_t hp = GetRealPoint(POINT_MAX_HP);
				int64_t add_hp = MIN(3500, hp * GetPoint(POINT_MAX_HP_PCT) / 100);
				add_hp += GetPoint(POINT_MAX_HP);
				add_hp += GetPoint(POINT_PARTY_TANKER_BONUS);

				SetMaxHP(hp + add_hp);

				val = GetMaxHP();
			}
			break;

		case POINT_MAX_SP:
			{
				SetPoint(type, GetPoint(type) + amount);

				//SetMaxSP(GetMaxSP() + amount);
				int sp = GetRealPoint(POINT_MAX_SP);
				int add_sp = MIN(800, sp * GetPoint(POINT_MAX_SP_PCT) / 100);
				add_sp += GetPoint(POINT_MAX_SP);
				add_sp += GetPoint(POINT_PARTY_SKILL_MASTER_BONUS);

				SetMaxSP(sp + add_sp);

				val = GetMaxSP();
			}
			break;

		case POINT_MAX_HP_PCT:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);

			PointChange(POINT_MAX_HP, 0);
			break;

		case POINT_MAX_SP_PCT:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);

			PointChange(POINT_MAX_SP, 0);
			break;

		case POINT_MAX_STAMINA:
			SetMaxStamina(GetMaxStamina() + amount);
			val = GetMaxStamina();
			break;

		#ifndef ENABLE_FULL_YANG
		case POINT_GOLD:
			{
				const int64_t nTotalMoney = static_cast<int64_t>(GetGold()) + static_cast<int64_t>(amount);
				if (GOLD_MAX <= nTotalMoney)
				{
					sys_err("[OVERFLOW_GOLD] OriGold %lld AddedGold %lld id %u Name %s ", GetGold(), amount, GetPlayerID(), GetName());
					LogManager::instance().CharLog(this, GetGold() + amount, "OVERFLOW_GOLD", "");
					return;
				}

				if (g_bChinaIntoxicationCheck && amount > 0)
				{
					if (IsOverTime(OT_NONE))
					{
						sys_log(1, "<GOLD_LOG> %s = NONE", GetName());
					}
					else if (IsOverTime(OT_3HOUR))
					{
						amount = (amount / 2);
						sys_log(1, "<GOLD_LOG> %s = 3HOUR", GetName());
					}
					else if (IsOverTime(OT_5HOUR))
					{
						amount = 0;
						sys_log(1, "<GOLD_LOG> %s = 5HOUR", GetName());
					}
				}

				SetGold(GetGold() + amount);
				val = GetGold();
			}
			break;
		#endif

		#ifdef ENABLE_OTO_TOPLAMA
		case POINT_OTO_METIN:
		{
				 if (amount > 1) { amount = 1; }
			else if (amount < 0) { amount = 0; }
			SetOtoMetin(amount);
			val = GetOtoMetin();
		}
		break;

		case POINT_OTO_CANAVAR:
		{
				 if (amount > 1) { amount = 1; }
			else if (amount < 0) { amount = 0; }
			SetOtoCanavar(amount);
			val = GetOtoCanavar();
		}
		break;

		case POINT_OTO_PATRON:
		{
				 if (amount > 1) { amount = 1; }
			else if (amount < 0) { amount = 0; }
			SetOtoBoss(amount);
			val = GetOtoBoss();
		}
		break;
		#endif

		#ifdef ENABLE_ANTI_EXP
		case POINT_ANTI_EXP:
			SetAntiExp(GetAntiExp() + amount);
			val = GetAntiExp();
			break;
		#endif

		case POINT_SKILL:
		case POINT_STAT:
		case POINT_SUB_SKILL:
		case POINT_STAT_RESET_COUNT:
		case POINT_HORSE_SKILL:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);

			SetRealPoint(type, val);
			break;

		case POINT_DEF_GRADE:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);

			PointChange(POINT_CLIENT_DEF_GRADE, amount);
			break;

		case POINT_CLIENT_DEF_GRADE:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			break;

		case POINT_ST:
		case POINT_HT:
		case POINT_DX:
		case POINT_IQ:
		case POINT_HP_REGEN:
		case POINT_SP_REGEN:
		case POINT_ATT_SPEED:
		case POINT_ATT_GRADE:
		case POINT_MOV_SPEED:
		case POINT_CASTING_SPEED:
		case POINT_MAGIC_ATT_GRADE:
		case POINT_MAGIC_DEF_GRADE:
		case POINT_BOW_DISTANCE:
		case POINT_HP_RECOVERY:
		case POINT_SP_RECOVERY:

		case POINT_ATTBONUS_HUMAN:	// 42
		case POINT_ATTBONUS_ANIMAL:	// 43
		case POINT_ATTBONUS_ORC:	// 44
		case POINT_ATTBONUS_MILGYO:	// 45
		case POINT_ATTBONUS_UNDEAD:	// 46
		case POINT_ATTBONUS_DEVIL:	// 47

		case POINT_ATTBONUS_MONSTER:
		case POINT_ATTBONUS_SURA:
		case POINT_ATTBONUS_ASSASSIN:
		case POINT_ATTBONUS_WARRIOR:
		case POINT_ATTBONUS_SHAMAN:
		#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_ATTBONUS_WOLFMAN:
		#endif
		#ifdef ENABLE_NEW_ATTRIBUTE_TYPES
		case POINT_ATTBONUS_STONE:
		case POINT_ATTBONUS_BOSS:
		case POINT_RESIST_HUMAN:
		case POINT_RESIST_MONSTER:
		#endif
		case POINT_POISON_PCT:
		#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_BLEEDING_PCT:
		#endif
		case POINT_STUN_PCT:
		case POINT_SLOW_PCT:

		case POINT_BLOCK:
		case POINT_DODGE:

		case POINT_CRITICAL_PCT:
		case POINT_RESIST_CRITICAL:
		case POINT_PENETRATE_PCT:
		case POINT_RESIST_PENETRATE:
		case POINT_CURSE_PCT:

		case POINT_STEAL_HP:		// 48
		case POINT_STEAL_SP:		// 49

		case POINT_MANA_BURN_PCT:	// 50
		case POINT_DAMAGE_SP_RECOVER:	// 51
		case POINT_RESIST_NORMAL_DAMAGE:
		case POINT_RESIST_SWORD:
		case POINT_RESIST_TWOHAND:
		case POINT_RESIST_DAGGER:
		case POINT_RESIST_BELL:
		case POINT_RESIST_FAN:
		case POINT_RESIST_BOW:
		#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_CLAW:
		#endif
		case POINT_RESIST_FIRE:
		case POINT_RESIST_ELEC:
		case POINT_RESIST_MAGIC:
		case POINT_RESIST_WIND:
		case POINT_RESIST_ICE:
		case POINT_RESIST_EARTH:
		case POINT_RESIST_DARK:
		case POINT_REFLECT_MELEE:	// 67
		case POINT_REFLECT_CURSE:	// 68
		case POINT_POISON_REDUCE:	// 69
		#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_BLEEDING_REDUCE:
		#endif
		case POINT_KILL_SP_RECOVER:	// 70
		case POINT_KILL_HP_RECOVERY:	// 75
		case POINT_HIT_HP_RECOVERY:
		case POINT_HIT_SP_RECOVERY:
		case POINT_MANASHIELD:
		case POINT_ATT_BONUS:
		case POINT_DEF_BONUS:
		case POINT_SKILL_DAMAGE_BONUS:
		case POINT_NORMAL_HIT_DAMAGE_BONUS:

		// DEPEND_BONUS_ATTRIBUTES
		case POINT_SKILL_DEFEND_BONUS:
		case POINT_NORMAL_HIT_DEFEND_BONUS:
		#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		case POINT_ACCEDRAIN_RATE:
		#endif
		#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
		case POINT_RESIST_MAGIC_REDUCTION:
		#endif
		case POINT_ENCHANT_ELECT:
		case POINT_ENCHANT_FIRE:
		case POINT_ENCHANT_ICE:
		case POINT_ENCHANT_WIND:
		case POINT_ENCHANT_EARTH:
		case POINT_ENCHANT_DARK:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			break;
			// END_OF_DEPEND_BONUS_ATTRIBUTES

		case POINT_PARTY_ATTACKER_BONUS:
		case POINT_PARTY_TANKER_BONUS:
		case POINT_PARTY_BUFFER_BONUS:
		case POINT_PARTY_SKILL_MASTER_BONUS:
		case POINT_PARTY_HASTE_BONUS:
		case POINT_PARTY_DEFENDER_BONUS:

		case POINT_RESIST_WARRIOR:
		case POINT_RESIST_ASSASSIN:
		case POINT_RESIST_SURA:
		case POINT_RESIST_SHAMAN:
		#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_WOLFMAN:
		#endif
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			break;

		#ifdef ENABLE_PVM_BONUS
		case POINT_SKILL_DAMAGE_BONUS_PVM:
		case POINT_HIT_DAMAGE_BONUS_PVM:
		case POINT_ATT_GRADE_BONUS_PVM:
		case POINT_DEF_GRADE_BONUS_PVM:
		case POINT_BERSERKER_BONUS_PVM:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			break;
		#endif

		case POINT_MALL_ATTBONUS:
		case POINT_MALL_DEFBONUS:
		case POINT_MALL_EXPBONUS:
		case POINT_MALL_ITEMBONUS:
		case POINT_MALL_GOLDBONUS:
		case POINT_MELEE_MAGIC_ATT_BONUS_PER:
			if (GetPoint(type) + amount > 100)
			{
				// sys_err("MALL_BONUS exceeded over 100!! point type: %d name: %s amount %d", type, GetName(), amount);
				amount = 100 - GetPoint(type);
			}

			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			break;

		case POINT_RAMADAN_CANDY_BONUS_EXP:
			SetPoint(type, amount);
			val = GetPoint(type);
			break;

		case POINT_EXP_DOUBLE_BONUS:	// 71
		case POINT_GOLD_DOUBLE_BONUS:	// 72
		case POINT_ITEM_DROP_BONUS:		// 73
		case POINT_POTION_BONUS:		// 74
			if (GetPoint(type) + amount > 100)
			{
				// sys_err("BONUS exceeded over 100!! point type: %d name: %s amount %d", type, GetName(), amount);
				amount = 100 - GetPoint(type);
			}

			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			break;

		case POINT_IMMUNE_STUN:		// 76
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			if (val)
			{
				// ChatPacket(CHAT_TYPE_INFO, "IMMUNE_STUN SET_BIT type(%u) amount(%d)", type, amount);
				SET_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_STUN);
			}
			else
			{
				// ChatPacket(CHAT_TYPE_INFO, "IMMUNE_STUN REMOVE_BIT type(%u) amount(%d)", type, amount);
				REMOVE_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_STUN);
			}
			break;

		case POINT_IMMUNE_SLOW:		// 77
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			if (val)
			{
				SET_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_SLOW);
			}
			else
			{
				REMOVE_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_SLOW);
			}
			break;

		case POINT_IMMUNE_FALL:	// 78
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			if (val)
			{
				SET_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_FALL);
			}
			else
			{
				REMOVE_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_FALL);
			}
			break;

		case POINT_ATT_GRADE_BONUS:
			SetPoint(type, GetPoint(type) + amount);
			PointChange(POINT_ATT_GRADE, amount);
			val = GetPoint(type);
			break;

		case POINT_DEF_GRADE_BONUS:
			SetPoint(type, GetPoint(type) + amount);
			PointChange(POINT_DEF_GRADE, amount);
			val = GetPoint(type);
			break;

		case POINT_MAGIC_ATT_GRADE_BONUS:
			SetPoint(type, GetPoint(type) + amount);
			PointChange(POINT_MAGIC_ATT_GRADE, amount);
			val = GetPoint(type);
			break;

		case POINT_MAGIC_DEF_GRADE_BONUS:
			SetPoint(type, GetPoint(type) + amount);
			PointChange(POINT_MAGIC_DEF_GRADE, amount);
			val = GetPoint(type);
			break;

		case POINT_VOICE:
		case POINT_EMPIRE_POINT:
			//sys_err("CHARACTER::PointChange: %s: point cannot be changed. use SetPoint instead (type: %d)", GetName(), type);
			val = GetRealPoint(type);
			break;

		case POINT_POLYMORPH:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			SetPolymorph(val);
			break;

		case POINT_MOUNT:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			MountVnum(val);
			break;

		case POINT_ENERGY:
		case POINT_COSTUME_ATTR_BONUS:
			{
				int old_val = GetPoint(type);
				SetPoint(type, old_val + amount);
				val = GetPoint(type);
				BuffOnAttr_ValueChange(type, old_val, val);
			}
			break;

		default:
			sys_err("CHARACTER::PointChange: %s: unknown point change type %d", GetName(), type);
			return;
	}

	switch (type)
	{
		case POINT_LEVEL:
		case POINT_ST:
		case POINT_DX:
		case POINT_IQ:
		case POINT_HT:
			ComputeBattlePoints();
			break;
		case POINT_MAX_HP:
		case POINT_MAX_SP:
		case POINT_MAX_STAMINA:
			break;
	}

	if (type == POINT_HP && amount == 0)
		return;

	if (GetDesc())
	{
		struct packet_point_change pack;

		pack.header = HEADER_GC_CHARACTER_POINT_CHANGE;
		pack.dwVID = m_vid;
		pack.type = type;
		pack.value = val;

		if (bAmount)
			pack.amount = amount;
		else
			pack.amount = 0;

		if (!bBroadcast)
			GetDesc()->Packet(pack);
		else
			PacketAround(pack);
	}
}

void CHARACTER::ApplyPoint(BYTE bApplyType, int iVal)
{
	switch (bApplyType)
	{
		case APPLY_NONE:			// 0
			break;

		case APPLY_CON:
			PointChange(POINT_HT, static_cast<int64_t>(iVal));
			PointChange(POINT_MAX_HP, static_cast<int64_t>(iVal * JobInitialPoints[GetJob()].hp_per_ht));
			PointChange(POINT_MAX_STAMINA, static_cast<int64_t>(iVal * JobInitialPoints[GetJob()].stamina_per_con));
			break;

		case APPLY_INT:
			PointChange(POINT_IQ, static_cast<int64_t>(iVal));
			PointChange(POINT_MAX_SP, static_cast<int64_t>(iVal * JobInitialPoints[GetJob()].sp_per_iq));
			break;

		case APPLY_SKILL:
			// SKILL_DAMAGE_BONUS
			{
				// 00000000 00000000 00000000 00000000

				// vnum     ^ add       change
				BYTE bSkillVnum = (BYTE) (((DWORD)iVal) >> 24);
				int iAdd = iVal & 0x00800000;
				int iChange = iVal & 0x007fffff;

				sys_log(1, "APPLY_SKILL skill %d add? %d change %d", bSkillVnum, iAdd ? 1 : 0, iChange);

				if (0 == iAdd)
					iChange = -iChange;

				boost::unordered_map<BYTE, int>::iterator iter = m_SkillDamageBonus.find(bSkillVnum);

				if (iter == m_SkillDamageBonus.end())
					m_SkillDamageBonus.emplace(bSkillVnum, iChange);
				else
					iter->second += iChange;
			}
			// END_OF_SKILL_DAMAGE_BONUS
			break;

		case APPLY_MAX_HP:
		case APPLY_MAX_HP_PCT:
			{
				int i = GetMaxHP(); if (i == 0) break;
				PointChange(aApplyInfo[bApplyType].bPointType, static_cast<int64_t>(iVal));
				float fRatio = (float)GetMaxHP() / (float)i;
				PointChange(POINT_HP, static_cast<int64_t>(GetHP() * fRatio - GetHP()));
			}
			break;

		case APPLY_MAX_SP:
		case APPLY_MAX_SP_PCT:
			{
				int i = GetMaxSP(); if (i == 0) break;
				PointChange(aApplyInfo[bApplyType].bPointType, static_cast<int64_t>(iVal));
				float fRatio = (float)GetMaxSP() / (float)i;
				PointChange(POINT_SP, static_cast<int64_t>(GetSP() * fRatio - GetSP()));
			}
			break;

		case APPLY_STR:
		case APPLY_DEX:
		case APPLY_ATT_SPEED:
		case APPLY_MOV_SPEED:
		case APPLY_CAST_SPEED:
		case APPLY_HP_REGEN:
		case APPLY_SP_REGEN:
		case APPLY_POISON_PCT:
		#ifdef ENABLE_WOLFMAN_CHARACTER
		case APPLY_BLEEDING_PCT:
		#endif
		case APPLY_STUN_PCT:
		case APPLY_SLOW_PCT:
		case APPLY_CRITICAL_PCT:
		case APPLY_PENETRATE_PCT:
		case APPLY_ATTBONUS_HUMAN:
		case APPLY_ATTBONUS_ANIMAL:
		case APPLY_ATTBONUS_ORC:
		case APPLY_ATTBONUS_MILGYO:
		case APPLY_ATTBONUS_UNDEAD:
		case APPLY_ATTBONUS_DEVIL:
		case APPLY_ATTBONUS_WARRIOR:	// 59
		case APPLY_ATTBONUS_ASSASSIN:	// 60
		case APPLY_ATTBONUS_SURA:		// 61
		case APPLY_ATTBONUS_SHAMAN:		// 62
		#ifdef ENABLE_WOLFMAN_CHARACTER
		case APPLY_ATTBONUS_WOLFMAN:
		#endif
		case APPLY_ATTBONUS_MONSTER:	// 63
		case APPLY_STEAL_HP:
		case APPLY_STEAL_SP:
		case APPLY_MANA_BURN_PCT:
		case APPLY_DAMAGE_SP_RECOVER:
		case APPLY_BLOCK:
		case APPLY_DODGE:
		case APPLY_RESIST_SWORD:
		case APPLY_RESIST_TWOHAND:
		case APPLY_RESIST_DAGGER:
		case APPLY_RESIST_BELL:
		case APPLY_RESIST_FAN:
		case APPLY_RESIST_BOW:
		#ifdef ENABLE_WOLFMAN_CHARACTER
		case APPLY_RESIST_CLAW:
		#endif
		case APPLY_RESIST_FIRE:
		case APPLY_RESIST_ELEC:
		case APPLY_RESIST_MAGIC:
		case APPLY_RESIST_WIND:
		case APPLY_RESIST_ICE:
		case APPLY_RESIST_EARTH:
		case APPLY_RESIST_DARK:
		case APPLY_REFLECT_MELEE:
		case APPLY_REFLECT_CURSE:
		case APPLY_ANTI_CRITICAL_PCT:
		case APPLY_ANTI_PENETRATE_PCT:
		case APPLY_POISON_REDUCE:
		#ifdef ENABLE_WOLFMAN_CHARACTER
		case APPLY_BLEEDING_REDUCE:
		#endif
		case APPLY_KILL_SP_RECOVER:
		case APPLY_EXP_DOUBLE_BONUS:
		case APPLY_GOLD_DOUBLE_BONUS:
		case APPLY_ITEM_DROP_BONUS:
		case APPLY_POTION_BONUS:
		case APPLY_KILL_HP_RECOVER:
		case APPLY_IMMUNE_STUN:
		case APPLY_IMMUNE_SLOW:
		case APPLY_IMMUNE_FALL:
		case APPLY_BOW_DISTANCE:
		case APPLY_ATT_GRADE_BONUS:
		case APPLY_DEF_GRADE_BONUS:
		case APPLY_MAGIC_ATT_GRADE:
		case APPLY_MAGIC_DEF_GRADE:
		case APPLY_CURSE_PCT:
		case APPLY_MAX_STAMINA:
		case APPLY_MALL_ATTBONUS:
		case APPLY_MALL_DEFBONUS:
		case APPLY_MALL_EXPBONUS:
		case APPLY_MALL_ITEMBONUS:
		case APPLY_MALL_GOLDBONUS:
		case APPLY_SKILL_DAMAGE_BONUS:
		case APPLY_NORMAL_HIT_DAMAGE_BONUS:

			// DEPEND_BONUS_ATTRIBUTES
		case APPLY_SKILL_DEFEND_BONUS:
		case APPLY_NORMAL_HIT_DEFEND_BONUS:
			// END_OF_DEPEND_BONUS_ATTRIBUTES

		case APPLY_RESIST_WARRIOR :
		case APPLY_RESIST_ASSASSIN :
		case APPLY_RESIST_SURA :
		case APPLY_RESIST_SHAMAN :
		#ifdef ENABLE_WOLFMAN_CHARACTER
		case APPLY_RESIST_WOLFMAN :
		#endif
		case APPLY_ENERGY:						// 82
		case APPLY_DEF_GRADE:					// 83
		case APPLY_COSTUME_ATTR_BONUS:			// 84
		case APPLY_MAGIC_ATTBONUS_PER:			// 85
		case APPLY_MELEE_MAGIC_ATTBONUS_PER:	// 86
		#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		case APPLY_ACCEDRAIN_RATE:				//97
		#endif
		#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
		case APPLY_RESIST_MAGIC_REDUCTION:		//98
		#endif
		case APPLY_ENCHANT_ELECT:
		case APPLY_ENCHANT_FIRE:
		case APPLY_ENCHANT_ICE:
		case APPLY_ENCHANT_WIND:
		case APPLY_ENCHANT_EARTH:
		case APPLY_ENCHANT_DARK:
		#ifdef ENABLE_NEW_ATTRIBUTE_TYPES
		case APPLY_ATTBONUS_STONE:
		case APPLY_ATTBONUS_BOSS:
		case APPLY_RESIST_HUMAN:
		case APPLY_RESIST_MONSTER:
		#endif
		#ifdef ENABLE_PVM_BONUS
		case APPLY_SKILL_DAMAGE_BONUS_PVM:
		case APPLY_HIT_DAMAGE_BONUS_PVM:
		case APPLY_ATT_GRADE_BONUS_PVM:
		case APPLY_DEF_GRADE_BONUS_PVM:
		case APPLY_BERSERKER_BONUS_PVM:
		#endif
			PointChange(aApplyInfo[bApplyType].bPointType, static_cast<int64_t>(iVal));
			break;

		default:
			sys_err("Unknown apply type %d name %s", bApplyType, GetName());
			break;
	}
}

void CHARACTER::MotionPacketEncode(BYTE motion, LPCHARACTER victim, struct packet_motion * packet)
{
	packet->header	= HEADER_GC_MOTION;
	packet->vid		= m_vid;
	packet->motion	= motion;

	if (victim)
		packet->victim_vid = victim->GetVID();
	else
		packet->victim_vid = 0;
}

void CHARACTER::Motion(BYTE motion, LPCHARACTER victim)
{
	struct packet_motion pack_motion;
	MotionPacketEncode(motion, victim, &pack_motion);
	PacketAround(pack_motion);
}

EVENTFUNC(save_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "save_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	sys_log(1, "SAVE_EVENT: %s", ch->GetName());

	ch->Save();
	ch->FlushDelayedSaveItem();
	return (save_event_second_cycle);
}

void CHARACTER::StartSaveEvent()
{
	if (m_pkSaveEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;
	m_pkSaveEvent = event_create(save_event, info, save_event_second_cycle);
}

void CHARACTER::MonsterLog(const char* format, ...)
{
	if (!test_server)
		return;

	if (IsPC())
		return;

	char chatbuf[CHAT_MAX_LEN + 1];
	int len = snprintf(chatbuf, sizeof(chatbuf), "%u)", (DWORD)GetVID());

	if (len < 0 || len >= (int) sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	va_list args;

	va_start(args, format);

	int len2 = vsnprintf(chatbuf + len, sizeof(chatbuf) - len, format, args);

	if (len2 < 0 || len2 >= (int) sizeof(chatbuf) - len)
		len += (sizeof(chatbuf) - len) - 1;
	else
		len += len2;

	++len;

	va_end(args);

	TPacketGCChat pack_chat;

	pack_chat.header    = HEADER_GC_CHAT;
	pack_chat.size		= sizeof(TPacketGCChat) + len;
	pack_chat.type      = CHAT_TYPE_TALKING;
	pack_chat.id        = (DWORD)GetVID();
	pack_chat.bEmpire	= 0;

	TEMP_BUFFER buf;
	buf.write(pack_chat);
	buf.write(chatbuf, len);

	CHARACTER_MANAGER::instance().PacketMonsterLog(this, buf.read_peek(), buf.size());
}

void CHARACTER::ChatPacket(BYTE type, const char * format, ...)
{
	LPDESC d = GetDesc();

	if (!d || !format)
		return;

	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	int len = vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	struct packet_chat pack_chat;

	pack_chat.header    = HEADER_GC_CHAT;
	pack_chat.size      = sizeof(struct packet_chat) + len;
	pack_chat.type      = type;
	pack_chat.id        = 0;
	pack_chat.bEmpire   = d->GetEmpire();

	TEMP_BUFFER buf;
	buf.write(pack_chat);
	buf.write(chatbuf, len);

	d->Packet(buf.read_peek(), buf.size());

	if (type == CHAT_TYPE_COMMAND && test_server)
		sys_log(0, "SEND_COMMAND %s %s", GetName(), chatbuf);
}

// MINING
void CHARACTER::mining_take()
{
	m_pkMiningEvent = NULL;
}

void CHARACTER::mining_cancel()
{
	if (m_pkMiningEvent)
	{
		sys_log(0, "XXX MINING CANCEL");
		event_cancel(&m_pkMiningEvent);
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("채광을 중단하였습니다."));
	}
}

void CHARACTER::mining(LPCHARACTER chLoad)
{
	if (m_pkMiningEvent)
	{
		mining_cancel();
		return;
	}

	if (!chLoad)
		return;

	// @fixme128
	if (GetMapIndex() != chLoad->GetMapIndex() || DISTANCE_APPROX(GetX() - chLoad->GetX(), GetY() - chLoad->GetY()) > 1000)
		return;

	if (mining::GetRawOreFromLoad(chLoad->GetRaceNum()) == 0)
		return;

	LPITEM pick = GetWear(WEAR_WEAPON);

	if (!pick || pick->GetType() != ITEM_PICK)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("곡괭이를 장착하세요."));
		return;
	}

	// #ifdef ENABLE_CAPTCHA_SELECT_SYSTEM
	// if (CaptchaQuestion())
		// return;
	// #endif

	int count = number(5, 15);

	TPacketGCDigMotion p;
	p.header = HEADER_GC_DIG_MOTION;
	p.vid = GetVID();
	p.target_vid = chLoad->GetVID();
	p.count = count;
	PacketAround(p);

	m_pkMiningEvent = mining::CreateMiningEvent(this, chLoad, count);
}
// END_OF_MINING

// @fixme219 BEGIN
bool CHARACTER::IsNearWater() const
{
	if (!GetSectree())
		return false;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			if (IS_SET(GetSectree()->GetAttribute(GetX() + x * 100, GetY() + y * 100), ATTR_WATER))
				return true;
		}
	}

	return false;
}
// @fixme219 END

#ifdef ENABLE_FISHING_SECURE
bool CHARACTER::IsCheckWater() const
{
	LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), GetX(), GetY());
	if (!tree) { return false; }
	if (tree->IsAttr(GetX(), GetY(), ATTR_WATER)) { return true; }
	return false;
}

void CHARACTER::SecureFishing()
{
	switch (GetMapIndex())
	{
		case 1:
		case 21:
		case 41:
			break;

		default:
			return;
	}

	if (IsCheckWater())
	{
		if (GetFishingSecureStatus()) { return; }
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Balikcinin korumasi altindasin."));
		SetFishingSecureStatus(true);
		SetOldPkMode(GetPKMode());
		SetPKMode(PK_MODE_PROTECT);
	}
	else
	{
		if (!GetFishingSecureStatus()) { return; }
		SetFishingSecureStatus(false);
		SetPKMode(GetOldPkMode());
	}
}
#endif

void CHARACTER::fishing()
{
	// #ifdef ENABLE_CAPTCHA_SELECT_SYSTEM
	// if (CaptchaQuestion())
		// return;
	// #endif

	#ifdef ENABLE_MULTI_FISH_BLOCK
	if (!GetMultiFishStatus())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bu_hesaptan_balik_tutamazsin"));
		return;
	}
	#endif

	if (m_pkFishingEvent)
	{
		fishing_take();
		return;
	}

	if (!IsNearWater())// @fixme219
		return;

	/*for (int i = 1; i <= gef_fishing_ch_count; i++)
	{
		if (g_bChannel == i) { continue; }

		if (strlen(gef_fishing_ch_str.c_str()) > 0)
		{
			ChatPacket(1, "Yalnizca %s 'de balik tutabilirsin.", gef_fishing_ch_str.c_str());
		}
		return;
	}*/

	{
		LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());
		if (!pkSectreeMap) { return; }

		int x = GetX();
		int y = GetY();

		LPSECTREE tree = pkSectreeMap->Find(x, y);
		if (!tree) { return; }

		DWORD dwAttr = tree->GetAttribute(x, y);
		if (IS_SET(dwAttr, ATTR_BLOCK))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("낚시를 할 수 있는 곳이 아닙니다"));
			return;
		}
	}

	LPITEM rod = GetWear(WEAR_WEAPON);
	if (!rod || rod->GetType() != ITEM_ROD)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("낚시대를 장착 하세요."));
		return;
	}

	if (0 == rod->GetSocket(2))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("미끼를 끼고 던져 주세요."));
		return;
	}

	float fx, fy;
	GetDeltaByDegree(GetRotation(), 400.0f, &fx, &fy);

	m_pkFishingEvent = fishing::CreateFishingEvent(this);
}

void CHARACTER::fishing_cancel()
{
	if (m_pkFishingEvent)
	{
		sys_log(0, "XXX FISHING CANCEL");
		event_cancel(&m_pkFishingEvent);
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("채광을 중단하였습니다."));
	}
}

void CHARACTER::fishing_take()
{
	LPITEM rod = GetWear(WEAR_WEAPON);
	if (rod && rod->GetType() == ITEM_ROD)
	{
		using fishing::fishing_event_info;
		if (m_pkFishingEvent)
		{
			struct fishing_event_info* info = dynamic_cast<struct fishing_event_info*>(m_pkFishingEvent->info);

			if (info)
				fishing::Take(info, this);
		}
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("낚시대가 아닌 물건으로 낚시를 할 수 없습니다!"));
	}

	event_cancel(&m_pkFishingEvent);
}

bool CHARACTER::StartStateMachine(int iNextPulse)
{
	if (CHARACTER_MANAGER::instance().AddToStateList(this))
	{
		m_dwNextStatePulse = thecore_heart->pulse + iNextPulse;
		return true;
	}

	return false;
}

void CHARACTER::StopStateMachine()
{
	CHARACTER_MANAGER::instance().RemoveFromStateList(this);
}

void CHARACTER::UpdateStateMachine(DWORD dwPulse)
{

	if (IsPC() && (!GetDesc() || !GetDesc()->GetCharacter()))
	{
		sys_err("UpdateStateMachine::Dusmeyi Fixledik a.q");
		return;
	}

	if (dwPulse < m_dwNextStatePulse)
		return;

	if (IsDead())
		return;

	Update();
	m_dwNextStatePulse = dwPulse + m_dwStateDuration;
}

void CHARACTER::SetNextStatePulse(int iNextPulse)
{
	CHARACTER_MANAGER::instance().AddToStateList(this);
	m_dwNextStatePulse = iNextPulse;

	if (iNextPulse < 10)
		MonsterLog("다음상태로어서가자");
}

void CHARACTER::UpdateCharacter(DWORD dwPulse)
{
	CFSM::Update();

	#ifdef ENABLE_BATTLE_PASS
	if (IsPC()) { SaveBattlePass(); }
	#endif
	#ifdef __RANKING_SYSTEM__
	if (IsPC()) { SaveRankBoss(); }
	#endif
}

void CHARACTER::SetShop(LPSHOP pkShop)
{
	if ((m_pkShop = pkShop))
		SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_SHOP);
	else
	{
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_SHOP);
		SetShopOwner(NULL);
	}
}

void CHARACTER::SetExchange(CExchange * pkExchange)
{
	m_pkExchange = pkExchange;
}

void CHARACTER::SetPart(BYTE bPartPos, DWORD wVal) // @fixme502
{
	assert(bPartPos < PART_MAX_NUM);
	m_pointsInstant.parts[bPartPos] = wVal;
}

DWORD CHARACTER::GetPart(BYTE bPartPos) const // @fixme502
{
	assert(bPartPos < PART_MAX_NUM);

#ifdef ENABLE_HIDE_COSTUME_PART
	if (bPartPos == PART_HAIR && GetWear(WEAR_COSTUME_HAIR) && IsHairCostumeHidden())
		return 0;
	else if (bPartPos == PART_MAIN && GetWear(WEAR_COSTUME_BODY) && IsBodyCostumeHidden())
	{
		if (const LPITEM pArmor = GetWear(WEAR_BODY))
			return pArmor->GetVnum();
		else
			return 0;
	}
	#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	else if (bPartPos == PART_WEAPON && GetWear(WEAR_COSTUME_WEAPON) && IsWeaponCostumeHidden())
	{
		if (const LPITEM pWeapon = GetWear(WEAR_WEAPON))
		{
			return pWeapon->GetVnum();
		}
		else
		{
			return 0;
		}
	}
	#endif
	/*#ifdef ENABLE_SHOULDER_SASH_SYSTEM
	else if (bPartPos == PART_SASH && GetWear(WEAR_COSTUME_SASH) && GetQuestFlag("HIDE.COSTUME_SASH") == 1)
	{
		return 0;
	}
	#endif*/
#endif

	return m_pointsInstant.parts[bPartPos];
}

DWORD CHARACTER::GetOriginalPart(BYTE bPartPos) const // @fixme502
{
	switch (bPartPos)
	{
		case PART_MAIN:
			#ifdef ENABLE_HIDE_COSTUME_PART
			if (GetWear(WEAR_COSTUME_BODY) && IsBodyCostumeHidden())
			{
				if (const LPITEM pArmor = GetWear(WEAR_BODY))
				{
					return pArmor->GetVnum();
				}
			}
			#endif

			if (!IsPC())
				return GetPart(PART_MAIN);
			else
				return m_pointsInstant.bBasePart;

		case PART_HAIR:
			#ifdef ENABLE_HIDE_COSTUME_PART
			if (GetWear(WEAR_COSTUME_HAIR) && IsHairCostumeHidden())
			{
				return 0;
			}
			#endif
			return GetPart(PART_HAIR);

		#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		case PART_ACCE:
			return GetPart(PART_ACCE);
		#endif

		#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		case PART_WEAPON:
			#ifdef ENABLE_HIDE_COSTUME_PART
			if (GetWear(WEAR_COSTUME_WEAPON) && IsWeaponCostumeHidden())
			{
				if (const LPITEM pWeapon = GetWear(WEAR_WEAPON))
				{
					return pWeapon->GetVnum();
				}
			}
			#endif
			return GetPart(PART_WEAPON);
		#endif

		default:
			return 0;
	}
}

BYTE CHARACTER::GetCharType() const
{
	return m_bCharType;
}

bool CHARACTER::SetSyncOwner(LPCHARACTER ch, bool bRemoveFromList)
{
	// TRENT_MONSTER
	if (IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
		return false;
	// END_OF_TRENT_MONSTER

	if (ch) // @fixme131
	{
		if (!battle_is_attackable(ch, this))
		{
			SendDamagePacket(ch, 0, DAMAGE_BLOCK);
			return false;
		}
	}

	if (ch == this)
	{
		sys_err("SetSyncOwner owner == this (%p)", this);
		return false;
	}

	if (!ch)
	{
		if (bRemoveFromList && m_pkChrSyncOwner)
		{
			m_pkChrSyncOwner->m_kLst_pkChrSyncOwned.remove(this);
		}

		if (m_pkChrSyncOwner)
			sys_log(1, "SyncRelease %s %p from %s", GetName(), this, m_pkChrSyncOwner->GetName());

		m_pkChrSyncOwner = NULL;
	}
	else
	{
		if (!IsSyncOwner(ch))
			return false;

		if (DISTANCE_APPROX(GetX() - ch->GetX(), GetY() - ch->GetY()) > 250)
		{
			sys_log(1, "SetSyncOwner distance over than 250 %s %s", GetName(), ch->GetName());

			if (m_pkChrSyncOwner == ch)
				return true;

			return false;
		}

		if (m_pkChrSyncOwner != ch)
		{
			if (m_pkChrSyncOwner)
			{
				sys_log(1, "SyncRelease %s %p from %s", GetName(), this, m_pkChrSyncOwner->GetName());
				m_pkChrSyncOwner->m_kLst_pkChrSyncOwned.remove(this);
			}

			m_pkChrSyncOwner = ch;
			m_pkChrSyncOwner->m_kLst_pkChrSyncOwned.emplace_back(this);

			static const timeval zero_tv = {0, 0};
			SetLastSyncTime(zero_tv);

			sys_log(1, "SetSyncOwner set %s %p to %s", GetName(), this, ch->GetName());
		}

		m_fSyncTime = get_float_time();
	}

	TPacketGCOwnership pack;

	pack.bHeader	= HEADER_GC_OWNERSHIP;
	pack.dwOwnerVID	= ch ? ch->GetVID() : 0;
	pack.dwVictimVID	= GetVID();

	PacketAround(pack);
	return true;
}

struct FuncClearSync
{
	void operator () (LPCHARACTER ch)
	{
		assert(ch != NULL);
		ch->SetSyncOwner(NULL, false);
	}
};

void CHARACTER::ClearSync()
{
	SetSyncOwner(NULL);

	std::for_each(m_kLst_pkChrSyncOwned.begin(), m_kLst_pkChrSyncOwned.end(), FuncClearSync());
	m_kLst_pkChrSyncOwned.clear();
}

bool CHARACTER::IsSyncOwner(LPCHARACTER ch) const
{
	if (m_pkChrSyncOwner == ch)
		return true;

	if (get_float_time() - m_fSyncTime >= 3.0f)
		return true;

	return false;
}

void CHARACTER::SetParty(LPPARTY pkParty)
{
	if (pkParty == m_pkParty)
		return;

	if (pkParty && m_pkParty)
		sys_err("%s is trying to reassigning party (current %p, new party %p)", GetName(), get_pointer(m_pkParty), get_pointer(pkParty));

	sys_log(1, "PARTY set to %p", get_pointer(pkParty));

	/*if (m_pkDungeon && IsPC() && !pkParty)// @Grup core fix
		SetDungeon(NULL);*/

	m_pkParty = pkParty;

	if (IsPC())
	{
		if (m_pkParty)
			SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_PARTY);
		else
			REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_PARTY);

		UpdatePacket();
	}
}

// PARTY_JOIN_BUG_FIX
EVENTINFO(TPartyJoinEventInfo)
{
	DWORD	dwGuestPID;
	DWORD	dwLeaderPID;

	TPartyJoinEventInfo()
	: dwGuestPID( 0 )
	, dwLeaderPID( 0 )
	{
	}
} ;

EVENTFUNC(party_request_event)
{
	TPartyJoinEventInfo * info = dynamic_cast<TPartyJoinEventInfo *>(event->info);
	if (info == NULL)
	{
		sys_err("party_request_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(info->dwGuestPID);
	if (ch)
	{
		sys_log(0, "PartyRequestEvent %s", ch->GetName());
		ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
		ch->SetPartyRequestEvent(NULL);
	}

	return 0;
}

bool CHARACTER::RequestToParty(LPCHARACTER leader)
{
	if (leader->GetParty())
		leader = leader->GetParty()->GetLeaderCharacter();

	if (!leader)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("파티장이 접속 상태가 아니라서 요청을 할 수 없습니다."));
		return false;
	}

	if (m_pkPartyRequestEvent)
		return false;

	if (!IsPC() || !leader->IsPC())
		return false;

	if (leader->IsBlockMode(BLOCK_PARTY_REQUEST))
		return false;

	PartyJoinErrCode errcode = IsPartyJoinableCondition(leader, this);

	switch (errcode)
	{
		case PERR_NONE:
			break;

		case PERR_SERVER:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
			return false;

		case PERR_DIFFEMPIRE:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 다른 제국과 파티를 이룰 수 없습니다."));
			return false;

		case PERR_DUNGEON:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던전 안에서는 파티 초대를 할 수 없습니다."));
			return false;

		case PERR_OBSERVER:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 관전 모드에선 파티 초대를 할 수 없습니다."));
			return false;

		case PERR_LVBOUNDARY:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> -30 ~ +30 레벨 이내의 상대방만 초대할 수 있습니다."));
			return false;

		case PERR_LOWLEVEL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티내 최고 레벨 보다 30레벨이 낮아 초대할 수 없습니다."));
			return false;

		case PERR_HILEVEL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티내 최저 레벨 보다 30레벨이 높아 초대할 수 없습니다."));
			return false;

		case PERR_ALREADYJOIN:
			return false;

		case PERR_PARTYISFULL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 더 이상 파티원을 초대할 수 없습니다."));
			return false;

		default:
			sys_err("Do not process party join error(%d)", errcode);
			return false;
	}

	TPartyJoinEventInfo* info = AllocEventInfo<TPartyJoinEventInfo>();

	info->dwGuestPID = GetPlayerID();
	info->dwLeaderPID = leader->GetPlayerID();

	SetPartyRequestEvent(event_create(party_request_event, info, PASSES_PER_SEC(10)));

	leader->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequest %u", (DWORD) GetVID());
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 님에게 파티가입 신청을 했습니다."), leader->GetName());
	return true;
}

void CHARACTER::DenyToParty(LPCHARACTER member)
{
	sys_log(1, "DenyToParty %s member %s %p", GetName(), member->GetName(), get_pointer(member->m_pkPartyRequestEvent));

	if (!member->m_pkPartyRequestEvent)
		return;

	TPartyJoinEventInfo * info = dynamic_cast<TPartyJoinEventInfo *>(member->m_pkPartyRequestEvent->info);

	if (!info)
	{
		sys_err( "CHARACTER::DenyToParty> <Factor> Null pointer" );
		return;
	}

	if (info->dwGuestPID != member->GetPlayerID())
		return;

	if (info->dwLeaderPID != GetPlayerID())
		return;

	event_cancel(&member->m_pkPartyRequestEvent);

	member->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

void CHARACTER::AcceptToParty(LPCHARACTER member)
{
	sys_log(1, "AcceptToParty %s member %s %p", GetName(), member->GetName(), get_pointer(member->m_pkPartyRequestEvent));

	if (!member->m_pkPartyRequestEvent)
		return;

	TPartyJoinEventInfo * info = dynamic_cast<TPartyJoinEventInfo *>(member->m_pkPartyRequestEvent->info);

	if (!info)
	{
		sys_err( "CHARACTER::AcceptToParty> <Factor> Null pointer" );
		return;
	}

	if (info->dwGuestPID != member->GetPlayerID())
		return;

	if (info->dwLeaderPID != GetPlayerID())
		return;

	event_cancel(&member->m_pkPartyRequestEvent);

	if (!GetParty())
		member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 파티에 속해있지 않습니다."));
	else
	{
		if (GetPlayerID() != GetParty()->GetLeaderPID())
			return;

		PartyJoinErrCode errcode = IsPartyJoinableCondition(this, member);
		switch (errcode)
		{
			case PERR_NONE: 		member->PartyJoin(this); return;
			case PERR_SERVER:		member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다.")); break;
			case PERR_DUNGEON:		member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던전 안에서는 파티 초대를 할 수 없습니다.")); break;
			case PERR_OBSERVER: 	member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 관전 모드에선 파티 초대를 할 수 없습니다.")); break;
			case PERR_LVBOUNDARY:	member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> -30 ~ +30 레벨 이내의 상대방만 초대할 수 있습니다.")); break;
			case PERR_LOWLEVEL: 	member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티내 최고 레벨 보다 30레벨이 낮아 초대할 수 없습니다.")); break;
			case PERR_HILEVEL: 		member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티내 최저 레벨 보다 30레벨이 높아 초대할 수 없습니다.")); break;
			#ifdef ENABLE_VSTURNUVA
			case PERR_DUEL:
				member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu haritada grup kuramazsiniz."));
				break;
			#endif
			case PERR_ALREADYJOIN: 	break;
			case PERR_PARTYISFULL: {
									   ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 더 이상 파티원을 초대할 수 없습니다."));
									   member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티의 인원제한이 초과하여 파티에 참가할 수 없습니다."));
									   break;
								   }
			default: sys_err("Do not process party join error(%d)", errcode);
		}
	}

	member->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

EVENTFUNC(party_invite_event)
{
	TPartyJoinEventInfo * pInfo = dynamic_cast<TPartyJoinEventInfo *>(  event->info );

	if ( pInfo == NULL )
	{
		sys_err( "party_invite_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER pchInviter = CHARACTER_MANAGER::instance().FindByPID(pInfo->dwLeaderPID);

	if (pchInviter)
	{
		sys_log(1, "PartyInviteEvent %s", pchInviter->GetName());
		pchInviter->PartyInviteDeny(pInfo->dwGuestPID);
	}

	return 0;
}

void CHARACTER::PartyInvite(LPCHARACTER pchInvitee)
{
	if (GetParty() && GetParty()->GetLeaderPID() != GetPlayerID())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티원을 초대할 수 있는 권한이 없습니다."));
		return;
	}
	else if (pchInvitee->IsBlockMode(BLOCK_PARTY_INVITE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> %s 님이 파티 거부 상태입니다."), pchInvitee->GetName());
		return;
	}

	PartyJoinErrCode errcode = IsPartyJoinableCondition(this, pchInvitee);

	switch (errcode)
	{
		case PERR_NONE:
			break;

		case PERR_SERVER:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
			return;

		case PERR_DIFFEMPIRE:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 다른 제국과 파티를 이룰 수 없습니다."));
			return;

		case PERR_DUNGEON:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던전 안에서는 파티 초대를 할 수 없습니다."));
			return;

		case PERR_OBSERVER:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 관전 모드에선 파티 초대를 할 수 없습니다."));
			return;

		case PERR_LVBOUNDARY:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> -30 ~ +30 레벨 이내의 상대방만 초대할 수 있습니다."));
			return;

		case PERR_LOWLEVEL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티내 최고 레벨 보다 30레벨이 낮아 초대할 수 없습니다."));
			return;

		case PERR_HILEVEL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티내 최저 레벨 보다 30레벨이 높아 초대할 수 없습니다."));
			return;

		#ifdef ENABLE_VSTURNUVA
		case PERR_DUEL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bu haritada grup kuramazsiniz")); 
			return;
		#endif

		case PERR_ALREADYJOIN:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 이미 %s님은 파티에 속해 있습니다."), pchInvitee->GetName());
			return;

		case PERR_PARTYISFULL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 더 이상 파티원을 초대할 수 없습니다."));
			return;

		default:
			sys_err("Do not process party join error(%d)", errcode);
			return;
	}

	if (m_PartyInviteEventMap.end() != m_PartyInviteEventMap.find(pchInvitee->GetPlayerID()))
		return;

	TPartyJoinEventInfo* info = AllocEventInfo<TPartyJoinEventInfo>();

	info->dwGuestPID = pchInvitee->GetPlayerID();
	info->dwLeaderPID = GetPlayerID();

	m_PartyInviteEventMap.emplace(pchInvitee->GetPlayerID(), event_create(party_invite_event, info, PASSES_PER_SEC(10)));


	TPacketGCPartyInvite p;
	p.header = HEADER_GC_PARTY_INVITE;
	p.leader_vid = GetVID();
	pchInvitee->GetDesc()->Packet(p);
}

void CHARACTER::PartyInviteAccept(LPCHARACTER pchInvitee)
{
	EventMap::iterator itFind = m_PartyInviteEventMap.find(pchInvitee->GetPlayerID());

	if (itFind == m_PartyInviteEventMap.end())
	{
		sys_log(1, "PartyInviteAccept from not invited character(%s)", pchInvitee->GetName());
		return;
	}

	event_cancel(&itFind->second);
	m_PartyInviteEventMap.erase(itFind);

	if (GetParty() && GetParty()->GetLeaderPID() != GetPlayerID())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티원을 초대할 수 있는 권한이 없습니다."));
		return;
	}

	PartyJoinErrCode errcode = IsPartyJoinableMutableCondition(this, pchInvitee);

	switch (errcode)
	{
		case PERR_NONE:
			break;

		case PERR_SERVER:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
			return;

		case PERR_DUNGEON:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던전 안에서는 파티 초대에 응할 수 없습니다."));
			return;

		case PERR_OBSERVER:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 관전 모드에선 파티 초대를 할 수 없습니다."));
			return;

		case PERR_LVBOUNDARY:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> -30 ~ +30 레벨 이내의 상대방만 초대할 수 있습니다."));
			return;

		case PERR_LOWLEVEL:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티내 최고 레벨 보다 30레벨이 낮아 초대할 수 없습니다."));
			return;

		case PERR_HILEVEL:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티내 최저 레벨 보다 30레벨이 높아 초대할 수 없습니다."));
			return;

		#ifdef ENABLE_VSTURNUVA
		case PERR_DUEL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("bu haritada grup kuramazsiniz")); 
			return;
		#endif

		case PERR_ALREADYJOIN:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티 초대에 응할 수 없습니다."));
			return;

		case PERR_PARTYISFULL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 더 이상 파티원을 초대할 수 없습니다."));
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티의 인원제한이 초과하여 파티에 참가할 수 없습니다."));
			return;

		default:
			sys_err("ignore party join error(%d)", errcode);
			return;
	}


	if (GetParty())
		pchInvitee->PartyJoin(this);
	else
	{
		LPPARTY pParty = CPartyManager::instance().CreateParty(this);

		pParty->Join(pchInvitee->GetPlayerID());
		pParty->Link(pchInvitee);
		pParty->SendPartyInfoAllToOne(this);
	}
}

void CHARACTER::PartyInviteDeny(DWORD dwPID)
{
	EventMap::iterator itFind = m_PartyInviteEventMap.find(dwPID);

	if (itFind == m_PartyInviteEventMap.end())
	{
		sys_log(1, "PartyInviteDeny to not exist event(inviter PID: %d, invitee PID: %d)", GetPlayerID(), dwPID);
		return;
	}

	event_cancel(&itFind->second);
	m_PartyInviteEventMap.erase(itFind);

	LPCHARACTER pchInvitee = CHARACTER_MANAGER::instance().FindByPID(dwPID);
	if (pchInvitee)
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> %s님이 파티 초대를 거절하셨습니다."), pchInvitee->GetName());
}

void CHARACTER::PartyJoin(LPCHARACTER pLeader)
{
	pLeader->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> %s님이 파티에 참가하셨습니다."), GetName());
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> %s님의 파티에 참가하셨습니다."), pLeader->GetName());

	pLeader->GetParty()->Join(GetPlayerID());
	pLeader->GetParty()->Link(this);
}

CHARACTER::PartyJoinErrCode CHARACTER::IsPartyJoinableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest)
{
	if (pchLeader->GetEmpire() != pchGuest->GetEmpire())
		return PERR_DIFFEMPIRE;

	return IsPartyJoinableMutableCondition(pchLeader, pchGuest);
}

static bool __party_can_join_by_level(LPCHARACTER leader, LPCHARACTER quest)
{
	int	level_limit = 30;
	return (abs(leader->GetLevel() - quest->GetLevel()) <= level_limit);
}

CHARACTER::PartyJoinErrCode CHARACTER::IsPartyJoinableMutableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest)
{
	if (!CPartyManager::instance().IsEnablePCParty())
		return PERR_SERVER;
	else if (pchLeader->GetDungeon())
		return PERR_DUNGEON;
	else if (pchGuest->IsObserverMode())
		return PERR_OBSERVER;
	#ifdef ENABLE_VSTURNUVA
	else if (pchLeader->GetMapIndex() == 28)
		return PERR_DUEL;
	#endif
	else if (false == __party_can_join_by_level(pchLeader, pchGuest))
		return PERR_LVBOUNDARY;
	else if (pchGuest->GetParty())
		return PERR_ALREADYJOIN;
	else if (pchLeader->GetParty())
	{
		if (pchLeader->GetParty()->GetMemberCount() == PARTY_MAX_MEMBER)
			return PERR_PARTYISFULL;
	}

	return PERR_NONE;
}
// END_OF_PARTY_JOIN_BUG_FIX

void CHARACTER::SetDungeon(LPDUNGEON pkDungeon)
{
	// if (pkDungeon && m_pkDungeon)
		// sys_err("%s is trying to reassigning dungeon (current %p, new party %p)", GetName(), get_pointer(m_pkDungeon), get_pointer(pkDungeon));

	if (m_pkDungeon == pkDungeon)
	{
		return;
	}

	if (m_pkDungeon)
	{
		if (IsPC())
		{
			if (GetParty())
				m_pkDungeon->DecPartyMember(GetParty(), this);
			else
				m_pkDungeon->DecMember(this);
		}
		else if (IsMonster() || IsStone())
		{
			m_pkDungeon->DecMonster();
		}
	}

	m_pkDungeon = pkDungeon;

	if (pkDungeon)
	{
		sys_log(0, "%s DUNGEON set to %p, PARTY is %p", GetName(), get_pointer(pkDungeon), get_pointer(m_pkParty));

		if (IsPC())
		{
			if (GetParty())
				m_pkDungeon->IncPartyMember(GetParty(), this);
			else
				m_pkDungeon->IncMember(this);
		}
		else if (IsMonster() || IsStone())
		{
			m_pkDungeon->IncMonster();
		}
	}
}

void CHARACTER::SetWarMap(CWarMap * pWarMap)
{
	if (m_pWarMap)
		m_pWarMap->DecMember(this);

	m_pWarMap = pWarMap;

	if (m_pWarMap)
		m_pWarMap->IncMember(this);
}

void CHARACTER::SetWeddingMap(marriage::WeddingMap* pMap)
{
	if (m_pWeddingMap)
		m_pWeddingMap->DecMember(this);

	m_pWeddingMap = pMap;

	if (m_pWeddingMap)
		m_pWeddingMap->IncMember(this);
}

void CHARACTER::SetRegen(LPREGEN pkRegen)
{
	m_pkRegen = pkRegen;
	if (pkRegen != NULL)
	{
		regen_id_ = pkRegen->id;
	}

	m_fRegenAngle = GetRotation();
	m_posRegen = GetXYZ();
}

bool CHARACTER::OnIdle()
{
	return false;
}

void CHARACTER::OnMove(bool bIsAttack)
{
	m_dwLastMoveTime = get_dword_time();

	if (bIsAttack)
	{
		m_dwLastAttackTime = m_dwLastMoveTime;

		if (IsAffectFlag(AFFECT_REVIVE_INVISIBLE) || IsAffectFlag(AFF_REVIVE_INVISIBLE)) // @Gorunmezlik bugu fix
			RemoveAffect(AFFECT_REVIVE_INVISIBLE);

		if (IsAffectFlag(AFF_EUNHYUNG))
		{
			RemoveAffect(SKILL_EUNHYUNG);
			SetAffectedEunhyung();
		}
		else
		{
			ClearAffectedEunhyung();
		}

		/*if (IsAffectFlag(AFF_JEONSIN))
		  RemoveAffect(SKILL_JEONSINBANGEO);*/
	}

	/*if (IsAffectFlag(AFF_GUNGON))
	  RemoveAffect(SKILL_GUNGON);*/

	// MINING
	mining_cancel();
	// END_OF_MINING
}

void CHARACTER::OnClick(LPCHARACTER pkChrCauser)
{
	if (!pkChrCauser)
	{
		sys_err("OnClick %s by NULL", GetName());
		return;
	}

	DWORD vid = GetVID();
	sys_log(0, "OnClick %s[vnum %d ServerUniqueID %d, pid %d] by %s", GetName(), GetRaceNum(), vid, GetPlayerID(), pkChrCauser->GetName());

	{
		if (pkChrCauser->GetMyShop() && pkChrCauser != this)
		{
			sys_err("OnClick Fail (%s->%s) - pc has shop", pkChrCauser->GetName(), GetName());
			return;
		}
	}

	{
		if (pkChrCauser->GetExchange())
		{
			sys_err("OnClick Fail (%s->%s) - pc is exchanging", pkChrCauser->GetName(), GetName());
			return;
		}
	}

	if (IsPC())
	{
		if (!CTargetManager::instance().GetTargetInfo(pkChrCauser->GetPlayerID(), TARGET_TYPE_VID, GetVID()))
		{
			if (GetMyShop())
			{
				if (pkChrCauser->IsDead() == true) return;

				//PREVENT_TRADE_WINDOW
				if (pkChrCauser == this)
				{
					if ((GetExchange() || IsOpenSafebox() || GetShopOwner()) || IsCubeOpen())
					{
						pkChrCauser->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래중(창고,교환,상점)에는 개인상점을 사용할 수 없습니다."));
						return;
					}
				}
				else
				{

					if ((pkChrCauser->GetExchange() || pkChrCauser->IsOpenSafebox() || pkChrCauser->GetMyShop() || pkChrCauser->GetShopOwner()) || pkChrCauser->IsCubeOpen() )
					{
						pkChrCauser->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래중(창고,교환,상점)에는 개인상점을 사용할 수 없습니다."));
						return;
					}

					//if ((GetExchange() || IsOpenSafebox() || GetShopOwner()))
					if ((GetExchange() || IsOpenSafebox() || IsCubeOpen()))
					{
						pkChrCauser->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 다른 거래를 하고 있는 중입니다."));
						return;
					}
				}
				//END_PREVENT_TRADE_WINDOW

				if (pkChrCauser->GetShop())
				{
					pkChrCauser->GetShop()->RemoveGuest(pkChrCauser);
					pkChrCauser->SetShop(NULL);
				}

				GetMyShop()->AddGuest(pkChrCauser, GetVID(), false);
				pkChrCauser->SetShopOwner(this);
				return;
			}

			if (test_server)
				sys_err("%s.OnClickFailure(%s) - target is PC", pkChrCauser->GetName(), GetName());

			return;
		}
	}

	if (g_bChinaIntoxicationCheck)
	{
		if (pkChrCauser->IsOverTime(OT_3HOUR))
		{
			sys_log(0, "Play OverTime : name = %s, hour = %d)", pkChrCauser->GetName(), 3);
			return;
		}
		else if (pkChrCauser->IsOverTime(OT_5HOUR))
		{
			sys_log(0, "Play OverTime : name = %s, hour = %d)", pkChrCauser->GetName(), 5);
			return;
		}
	}

	pkChrCauser->SetQuestNPCID(GetVID());

	if (quest::CQuestManager::instance().Click(pkChrCauser->GetPlayerID(), this))
	{
		return;
	}

	if (!IsPC())
	{
		if (!m_triggerOnClick.pFunc)
		{
			//sys_err("%s.OnClickFailure(%s) : triggerOnClick.pFunc is EMPTY(pid=%d)",
			//			pkChrCauser->GetName(),
			//			GetName(),
			//			pkChrCauser->GetPlayerID());
			return;
		}

		m_triggerOnClick.pFunc(this, pkChrCauser);
	}
}

BYTE CHARACTER::GetGMLevel() const
{
	if (test_server)
		return GM_IMPLEMENTOR;
	return m_pointsInstant.gm_level;
}

void CHARACTER::SetGMLevel()
{
	if (GetDesc())
	{
		m_pointsInstant.gm_level = gm_get_level(GetName(), GetDesc()->GetAccountTable().login);
	}
	else
	{
		m_pointsInstant.gm_level = GM_PLAYER;
	}
}

BOOL CHARACTER::IsGM() const
{
	if (m_pointsInstant.gm_level != GM_PLAYER)
		return true;
	if (test_server)
		return true;
	return false;
}

void CHARACTER::SetStone(LPCHARACTER pkChrStone)
{
	m_pkChrStone = pkChrStone;

	if (m_pkChrStone)
	{
		if (pkChrStone->m_set_pkChrSpawnedBy.find(this) == pkChrStone->m_set_pkChrSpawnedBy.end())
			pkChrStone->m_set_pkChrSpawnedBy.emplace(this);
	}
}

struct FuncDeadSpawnedByStone
{
	void operator () (LPCHARACTER ch)
	{
		if (ch)
		{
			ch->Dead(NULL);
			ch->SetStone(NULL);
		}
	}
};

void CHARACTER::ClearStone()
{
	if (!m_set_pkChrSpawnedBy.empty())
	{
		FuncDeadSpawnedByStone f;
		std::for_each(m_set_pkChrSpawnedBy.begin(), m_set_pkChrSpawnedBy.end(), f);
		m_set_pkChrSpawnedBy.clear();
	}

	if (!m_pkChrStone)
		return;

	m_pkChrStone->m_set_pkChrSpawnedBy.erase(this);
	m_pkChrStone = NULL;
}

void CHARACTER::ClearTarget()
{
	if (m_pkChrTarget)
	{
		m_pkChrTarget->m_set_pkChrTargetedBy.erase(this);
		m_pkChrTarget = NULL;
	}

	TPacketGCTarget p;

	p.header = HEADER_GC_TARGET;
	p.dwVID = 0;
	p.bHPPercent = 0;

	CHARACTER_SET::iterator it = m_set_pkChrTargetedBy.begin();

	while (it != m_set_pkChrTargetedBy.end())
	{
		LPCHARACTER pkChr = *(it++);
		pkChr->m_pkChrTarget = NULL;

		if (!pkChr->GetDesc())
		{
			sys_err("%s %p does not have desc", pkChr->GetName(), get_pointer(pkChr));
			abort();
		}

		pkChr->GetDesc()->Packet(p);
	}

	m_set_pkChrTargetedBy.clear();
}

void CHARACTER::SetTarget(LPCHARACTER pkChrTarget)
{
	if (m_pkChrTarget == pkChrTarget)
		return;

	// CASTLE
	if (IS_CASTLE_MAP(GetMapIndex()) && !IsGM())
		return;
	// CASTLE

	if (m_pkChrTarget)
		m_pkChrTarget->m_set_pkChrTargetedBy.erase(this);

	m_pkChrTarget = pkChrTarget;

	TPacketGCTarget p;

	p.header = HEADER_GC_TARGET;

	if (m_pkChrTarget)
	{
		m_pkChrTarget->m_set_pkChrTargetedBy.emplace(this);

		p.dwVID	= m_pkChrTarget->GetVID();

		if ((m_pkChrTarget->IsPC() && !m_pkChrTarget->IsPolymorphed()) || (m_pkChrTarget->GetMaxHP() <= 0))
			p.bHPPercent = 0;
		else
		{
			if (m_pkChrTarget->GetRaceNum() == 20101 ||
					m_pkChrTarget->GetRaceNum() == 20102 ||
					m_pkChrTarget->GetRaceNum() == 20103 ||
					m_pkChrTarget->GetRaceNum() == 20104 ||
					m_pkChrTarget->GetRaceNum() == 20105 ||
					m_pkChrTarget->GetRaceNum() == 20106 ||
					m_pkChrTarget->GetRaceNum() == 20107 ||
					m_pkChrTarget->GetRaceNum() == 20108 ||
					m_pkChrTarget->GetRaceNum() == 20109)
			{
				LPCHARACTER owner = m_pkChrTarget->GetVictim();

				if (owner)
				{
					int iHorseHealth = owner->GetHorseHealth();
					int iHorseMaxHealth = owner->GetHorseMaxHealth();

					if (iHorseMaxHealth)
						p.bHPPercent = MINMAX(0,  iHorseHealth * 100 / iHorseMaxHealth, 100);
					else
						p.bHPPercent = 100;
				}
				else
					p.bHPPercent = 100;
			}
			else
			{
				if (m_pkChrTarget->GetMaxHP() <= 0) // @fixme136
					p.bHPPercent = 0;
				else
					p.bHPPercent = MINMAX(0, (m_pkChrTarget->GetHP() * 100) / m_pkChrTarget->GetMaxHP(), 100);
			}
		}
	}
	else
	{
		p.dwVID = 0;
		p.bHPPercent = 0;
	}

	GetDesc()->Packet(p);
}

void CHARACTER::BroadcastTargetPacket()
{
	if (m_set_pkChrTargetedBy.empty())
		return;

	TPacketGCTarget p;

	p.header = HEADER_GC_TARGET;
	p.dwVID = GetVID();

	if (IsPC())
		p.bHPPercent = 0;
	else if (GetMaxHP() <= 0) // @fixme136
		p.bHPPercent = 0;
	else
		p.bHPPercent = MINMAX(0, (GetHP() * 100) / GetMaxHP(), 100);

	CHARACTER_SET::iterator it = m_set_pkChrTargetedBy.begin();

	while (it != m_set_pkChrTargetedBy.end())
	{
		LPCHARACTER pkChr = *it++;

		if (!pkChr->GetDesc())
		{
			sys_err("%s %p does not have desc", pkChr->GetName(), get_pointer(pkChr));
			abort();
		}

		pkChr->GetDesc()->Packet(p);
	}
}

void CHARACTER::CheckTarget()
{
	if (!m_pkChrTarget)
		return;

	if (DISTANCE_APPROX(GetX() - m_pkChrTarget->GetX(), GetY() - m_pkChrTarget->GetY()) >= 4800)
		SetTarget(NULL);
}

void CHARACTER::SetWarpLocation(long lMapIndex, long x, long y)
{
	m_posWarp.x = x * 100;
	m_posWarp.y = y * 100;
	m_lWarpMapIndex = lMapIndex;
}

void CHARACTER::SaveExitLocation()
{
	m_posExit = GetXYZ();
	m_lExitMapIndex = GetMapIndex();
}

void CHARACTER::ExitToSavedLocation()
{
	sys_log (0, "ExitToSavedLocation");
	WarpSet(m_posWarp.x, m_posWarp.y, m_lWarpMapIndex);

	m_posExit.x = m_posExit.y = m_posExit.z = 0;
	m_lExitMapIndex = 0;
}

bool CHARACTER::WarpSet(long x, long y, long lPrivateMapIndex, WORD wNewPort)
{
	if (!IsPC())
		return false;

	long lAddr;
	long lMapIndex;
	WORD wPort;

	if (!CMapLocation::instance().Get(x, y, lMapIndex, lAddr, wPort))
	{
		sys_err("cannot find map location index %d x %d y %d name %s", lMapIndex, x, y, GetName());
		return false;
	}

	//Send Supplementary Data Block if new map requires security packages in loading this map
	{
		long lCurAddr;
		long lCurMapIndex = 0;
		WORD wCurPort;

		CMapLocation::instance().Get(GetX(), GetY(), lCurMapIndex, lCurAddr, wCurPort);

		//do not send SDB files if char is in the same map
		if ( lCurMapIndex != lMapIndex )
		{
			const TMapRegion * rMapRgn = SECTREE_MANAGER::instance().GetMapRegion(lMapIndex);
			{
				DESC_MANAGER::instance().SendClientPackageSDBToLoadMap( GetDesc(), rMapRgn->strMapName.c_str() );
			}
		}
	}

	if (lPrivateMapIndex >= 10000)
	{
		if (lPrivateMapIndex / 10000 != lMapIndex)
		{
			sys_err("Invalid map index %d, must be child of %d", lPrivateMapIndex, lMapIndex);
			return false;
		}

		lMapIndex = lPrivateMapIndex;
	}

	SetWarpStatus(true);//Fixme

	Stop();
	Save();

	if (GetSectree())
	{
		GetSectree()->RemoveEntity(this);
		ViewCleanup();

		EncodeRemovePacket(this);
	}

	m_lWarpMapIndex = lMapIndex;
	m_posWarp.x = x;
	m_posWarp.y = y;

	sys_log(0, "WarpSet %s %d %d current map %d target map %d", GetName(), x, y, GetMapIndex(), lMapIndex);

	TPacketGCWarp p;

	p.bHeader	= HEADER_GC_WARP;
	p.lX		= x;
	p.lY		= y;
	p.lAddr		= lAddr;
	#ifdef ENABLE_NEWSTUFF
	if (!g_stProxyIP.empty())
		p.lAddr	= inet_addr(g_stProxyIP.c_str());
	#endif
	p.wPort		= wNewPort ? wNewPort : wPort;

	#ifdef ENABLE_SWITCHBOT
	CSwitchbotManager::Instance().SetIsWarping(GetPlayerID(), true);

	if (p.wPort != mother_port)
	{
		CSwitchbotManager::Instance().P2PSendSwitchbot(GetPlayerID(), p.wPort);
	}
	#endif

	GetDesc()->Packet(p);

	char buf[256];
	snprintf(buf, sizeof(buf), "%s MapIdx %ld DestMapIdx%ld DestX%ld DestY%ld Empire%d", GetName(), GetMapIndex(), lPrivateMapIndex, x, y, GetEmpire());
	LogManager::instance().CharLog(this, 0, "WARP", buf);

	return true;
}

#define ENABLE_GOHOME_IF_MAP_NOT_ALLOWED
void CHARACTER::WarpEnd()
{
	if (test_server)
		sys_log(0, "WarpEnd %s", GetName());

	if (m_posWarp.x == 0 && m_posWarp.y == 0)
		return;

	int index = m_lWarpMapIndex;

	if (index > 10000)
		index /= 10000;

	if (!map_allow_find(index))
	{
		sys_err("location %d %d not allowed to login this server", m_posWarp.x, m_posWarp.y);
		#ifdef ENABLE_GOHOME_IF_MAP_NOT_ALLOWED
		GoHome();
		#else
		GetDesc()->SetPhase(PHASE_CLOSE);
		#endif
		return;
	}

	sys_log(0, "WarpEnd %s %d %u %u", GetName(), m_lWarpMapIndex, m_posWarp.x, m_posWarp.y);

	Show(m_lWarpMapIndex, m_posWarp.x, m_posWarp.y, 0);
	Stop();

	m_lWarpMapIndex = 0;
	m_posWarp.x = m_posWarp.y = m_posWarp.z = 0;

	{
		// P2P Login
		TPacketGGLogin p;

		p.bHeader = HEADER_GG_LOGIN;
		strlcpy(p.szName, GetName(), sizeof(p.szName));
		p.dwPID = GetPlayerID();
		p.bEmpire = GetEmpire();
		p.lMapIndex = SECTREE_MANAGER::instance().GetMapIndex(GetX(), GetY());
		p.bChannel = g_bChannel;

		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGLogin));
	}
}

bool CHARACTER::Return()
{
	if (!IsNPC())
		return false;

	SetVictim(NULL);

	int x = m_pkMobInst->m_posLastAttacked.x;
	int y = m_pkMobInst->m_posLastAttacked.y;

	SetRotationToXY(x, y);

	if (!Goto(x, y))
		return false;

	SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	if (test_server)
		sys_log(0, "%s %p 포기하고 돌아가자! %d %d", GetName(), this, x, y);

	if (GetParty())
		GetParty()->SendMessage(this, PM_RETURN, x, y);

	return true;
}

bool CHARACTER::Follow(LPCHARACTER pkChr, float fMinDistance)
{
	if (IsPC())
	{
		sys_err("CHARACTER::Follow : PC cannot use this method", GetName());
		return false;
	}

	if (IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
	{
		if (pkChr->IsPC())
		{
			// If i'm in a party. I must obey party leader's AI.
			if (!GetParty() || !GetParty()->GetLeader() || GetParty()->GetLeader() == this)
			{
				if (get_dword_time() - m_pkMobInst->m_dwLastAttackedTime >= 15000)
				{
					if (m_pkMobData->m_table.wAttackRange < DISTANCE_APPROX(pkChr->GetX() - GetX(), pkChr->GetY() - GetY()))
						if (Return())
							return true;
				}
			}
		}
		return false;
	}
	// END_OF_TRENT_MONSTER

	long x = pkChr->GetX();
	long y = pkChr->GetY();

	if (pkChr->IsPC())
	{
		// If i'm in a party. I must obey party leader's AI.
		if (!GetParty() || !GetParty()->GetLeader() || GetParty()->GetLeader() == this)
		{
			if (get_dword_time() - m_pkMobInst->m_dwLastAttackedTime >= 15000)
			{
				if (5000 < DISTANCE_APPROX(m_pkMobInst->m_posLastAttacked.x - GetX(), m_pkMobInst->m_posLastAttacked.y - GetY()))
					if (Return())
						return true;
			}
		}
	}

	/*if (IsGuardNPC())
	{
		if (5000 < DISTANCE_APPROX(m_pkMobInst->m_posLastAttacked.x - GetX(), m_pkMobInst->m_posLastAttacked.y - GetY()))
			if (Return())
				return true;
	}*/

	if (pkChr->IsState(pkChr->m_stateMove) &&
		GetMobBattleType() != BATTLE_TYPE_RANGE &&
		GetMobBattleType() != BATTLE_TYPE_MAGIC &&
		false == IsPet())
	{
		float rot = pkChr->GetRotation();
		float rot_delta = GetDegreeDelta(rot, GetDegreeFromPositionXY(GetX(), GetY(), pkChr->GetX(), pkChr->GetY()));

		float yourSpeed = pkChr->GetMoveSpeed();
		float mySpeed = GetMoveSpeed();

		float fDist = DISTANCE_SQRT(x - GetX(), y - GetY());
		float fFollowSpeed = mySpeed - yourSpeed * cos(rot_delta * M_PI / 180);

		if (fFollowSpeed >= 0.1f)
		{
			float fMeetTime = fDist / fFollowSpeed;
			float fYourMoveEstimateX, fYourMoveEstimateY;

			if ( fMeetTime * yourSpeed <= 100000.0f )
			{
				GetDeltaByDegree(pkChr->GetRotation(), fMeetTime * yourSpeed, &fYourMoveEstimateX, &fYourMoveEstimateY);

				x += (long) fYourMoveEstimateX;
				y += (long) fYourMoveEstimateY;

				float fDistNew = sqrt(((double)x - GetX())*(x-GetX())+((double)y - GetY())*(y-GetY()));
				if (fDist < fDistNew)
				{
					x = (long)(GetX() + (x - GetX()) * fDist / fDistNew);
					y = (long)(GetY() + (y - GetY()) * fDist / fDistNew);
				}
			}
		}
	}

	SetRotationToXY(x, y);

	float fDist = DISTANCE_SQRT(x - GetX(), y - GetY());

	if (fDist <= fMinDistance)
		return false;

	float fx, fy;

	if (IsChangeAttackPosition(pkChr) && GetMobRank() < MOB_RANK_BOSS)
	{
		SetChangeAttackPositionTime();

		int retry = 16;
		int dx, dy;
		int rot = (int) GetDegreeFromPositionXY(x, y, GetX(), GetY());

		while (--retry)
		{
			if (fDist < 500.0f)
				GetDeltaByDegree((rot + number(-90, 90) + number(-90, 90)) % 360, fMinDistance, &fx, &fy);
			else
				GetDeltaByDegree(number(0, 359), fMinDistance, &fx, &fy);

			dx = x + (int) fx;
			dy = y + (int) fy;

			LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), dx, dy);

			if (NULL == tree)
				break;

			if (0 == (tree->GetAttribute(dx, dy) & (ATTR_BLOCK | ATTR_OBJECT)))
				break;
		}

		if (!Goto(dx, dy))
			return false;
	}
	else
	{
		float fDistToGo = fDist - fMinDistance;
		GetDeltaByDegree(GetRotation(), fDistToGo, &fx, &fy);

		if (!Goto(GetX() + (int) fx, GetY() + (int) fy))
			return false;
	}

	SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	return true;
}

float CHARACTER::GetDistanceFromSafeboxOpen() const
{
	return DISTANCE_APPROX(GetX() - m_posSafeboxOpen.x, GetY() - m_posSafeboxOpen.y);
}

void CHARACTER::SetSafeboxOpenPosition()
{
	m_posSafeboxOpen = GetXYZ();
}

CSafebox * CHARACTER::GetSafebox() const
{
	return m_pkSafebox;
}

void CHARACTER::ReqSafeboxLoad(const char* pszPassword)
{
	if (!*pszPassword || strlen(pszPassword) > SAFEBOX_PASSWORD_MAX_LEN)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 잘못된 암호를 입력하셨습니다."));
		return;
	}
	else if (m_pkSafebox)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고가 이미 열려있습니다."));
		return;
	}

	int iPulse = thecore_pulse();
	if (iPulse - GetSafeboxLoadTime() < PASSES_PER_SEC(TRADE_CHECK_TIME))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고를 닫은지 10초 안에는 열 수 없습니다."));
		return;
	}
	/* @fixme196
		else if (GetDistanceFromSafeboxOpen() > 1000)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<A￠°i> °A¸®°¡ ¸O¾i¼­ A￠°i¸| ¿­ ¼o ¾ø½A´I´U."));
			return;
		}
	*/
	else if (m_bOpeningSafebox)
	{
		sys_log(0, "Overlapped safebox load request from %s", GetName());
		return;
	}

	SetSafeboxLoadTime();
	m_bOpeningSafebox = true;

	TSafeboxLoadPacket p;
	p.dwID = GetDesc()->GetAccountTable().id;
	strlcpy(p.szLogin, GetDesc()->GetAccountTable().login, sizeof(p.szLogin));
	strlcpy(p.szPassword, pszPassword, sizeof(p.szPassword));

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_LOAD, GetDesc()->GetHandle(), &p, sizeof(p));
}

void CHARACTER::LoadSafebox(int iSize, DWORD dwGold, int iItemCount, TPlayerItem * pItems)
{
	bool bLoaded = false;

	//PREVENT_TRADE_WINDOW
	SetOpenSafebox(true);
	//END_PREVENT_TRADE_WINDOW

	if (m_pkSafebox)
		bLoaded = true;

	if (!m_pkSafebox)
		m_pkSafebox = M2_NEW CSafebox(this, iSize, dwGold);
	else
		m_pkSafebox->ChangeSize(iSize);

	m_iSafeboxSize = iSize;

	TPacketCGSafeboxSize p;

	p.bHeader = HEADER_GC_SAFEBOX_SIZE;
	p.bSize = iSize;

	GetDesc()->Packet(p);

	if (!bLoaded)
	{
		for (int i = 0; i < iItemCount; ++i, ++pItems)
		{
			if (!m_pkSafebox->IsValidPosition(pItems->pos))
				continue;

			LPITEM item = ITEM_MANAGER::instance().CreateItem(pItems->vnum, pItems->count, pItems->id);

			if (!item)
			{
				sys_err("cannot create item vnum %d id %u (name: %s)", pItems->vnum, pItems->id, GetName());
				continue;
			}

			item->SetSkipSave(true);
			item->SetSockets(pItems->alSockets);
			item->SetAttributes(pItems->aAttr);
			#ifdef ENABLE_EVOLUTION_SYSTEM
			item->SetEvolution(pItems->evolution);
			#endif
			#ifdef ENABLE_CHANGELOOK_SYSTEM
			item->SetTransmutation(pItems->transmutation);
			#endif
			#ifdef ENABLE_LOCKED_SYSTEM
			item->SetLocked(pItems->locked);
			#endif

			if (!m_pkSafebox->Add(pItems->pos, item))
			{
				M2_DESTROY_ITEM(item);
			}
			else
			{
				item->OnAfterCreatedItem(); // @fixme306
				item->SetSkipSave(false);
			}
		}
	}
}

void CHARACTER::ChangeSafeboxSize(BYTE bSize)
{
	//if (!m_pkSafebox)
	//return;

	TPacketCGSafeboxSize p;

	p.bHeader = HEADER_GC_SAFEBOX_SIZE;
	p.bSize = bSize;

	GetDesc()->Packet(p);

	if (m_pkSafebox)
		m_pkSafebox->ChangeSize(bSize);

	m_iSafeboxSize = bSize;
}

void CHARACTER::CloseSafebox()
{
	if (!m_pkSafebox)
		return;

	//PREVENT_TRADE_WINDOW
	SetOpenSafebox(false);
	//END_PREVENT_TRADE_WINDOW

	m_pkSafebox->Save();

	M2_DELETE(m_pkSafebox);
	m_pkSafebox = NULL;

	ChatPacket(CHAT_TYPE_COMMAND, "CloseSafebox");

	SetSafeboxLoadTime();
	m_bOpeningSafebox = false;

	Save();
}

CSafebox * CHARACTER::GetMall() const
{
	return m_pkMall;
}

void CHARACTER::LoadMall(int iItemCount, TPlayerItem * pItems)
{
	bool bLoaded = false;

	if (m_pkMall)
		bLoaded = true;

	if (!m_pkMall)
		m_pkMall = M2_NEW CSafebox(this, 3 * SAFEBOX_PAGE_SIZE, 0);
	else
		m_pkMall->ChangeSize(3 * SAFEBOX_PAGE_SIZE);

	m_pkMall->SetWindowMode(MALL);

	TPacketCGSafeboxSize p;

	p.bHeader = HEADER_GC_MALL_OPEN;
	p.bSize = 3 * SAFEBOX_PAGE_SIZE;

	GetDesc()->Packet(p);

	if (!bLoaded)
	{
		for (int i = 0; i < iItemCount; ++i, ++pItems)
		{
			if (!m_pkMall->IsValidPosition(pItems->pos))
				continue;

			LPITEM item = ITEM_MANAGER::instance().CreateItem(pItems->vnum, pItems->count, pItems->id);

			if (!item)
			{
				sys_err("cannot create item vnum %d id %u (name: %s)", pItems->vnum, pItems->id, GetName());
				continue;
			}

			item->SetSkipSave(true);
			item->SetSockets(pItems->alSockets);
			item->SetAttributes(pItems->aAttr);
			#ifdef ENABLE_EVOLUTION_SYSTEM
			item->SetEvolution(pItems->evolution);
			#endif
			#ifdef ENABLE_CHANGELOOK_SYSTEM
			item->SetTransmutation(pItems->transmutation);
			#endif
			#ifdef ENABLE_LOCKED_SYSTEM
			item->SetLocked(pItems->locked);
			#endif

			if (!m_pkMall->Add(pItems->pos, item))
				M2_DESTROY_ITEM(item);
			else
				item->SetSkipSave(false);
		}
	}
}

void CHARACTER::CloseMall()
{
	if (!m_pkMall)
		return;

	m_pkMall->Save();

	M2_DELETE(m_pkMall);
	m_pkMall = NULL;

	ChatPacket(CHAT_TYPE_COMMAND, "CloseMall");
}

bool CHARACTER::BuildUpdatePartyPacket(TPacketGCPartyUpdate & out)
{
	if (!GetParty())
		return false;

	memset(&out, 0, sizeof(out));

	out.header		= HEADER_GC_PARTY_UPDATE;
	out.pid		= GetPlayerID();
	if (GetMaxHP() <= 0) // @fixme136
		out.percent_hp	= 0;
	else
		out.percent_hp	= MINMAX(0, GetHP() * 100 / GetMaxHP(), 100);
	out.role		= GetParty()->GetRole(GetPlayerID());

	sys_log(1, "PARTY %s role is %d", GetName(), out.role);

	LPCHARACTER l = GetParty()->GetLeaderCharacter();

	if (l && DISTANCE_APPROX(GetX() - l->GetX(), GetY() - l->GetY()) < PARTY_DEFAULT_RANGE)
	{
		out.affects[0] = GetParty()->GetPartyBonusExpPercent();
		out.affects[1] = GetPoint(POINT_PARTY_ATTACKER_BONUS);
		out.affects[2] = GetPoint(POINT_PARTY_TANKER_BONUS);
		out.affects[3] = GetPoint(POINT_PARTY_BUFFER_BONUS);
		out.affects[4] = GetPoint(POINT_PARTY_SKILL_MASTER_BONUS);
		out.affects[5] = GetPoint(POINT_PARTY_HASTE_BONUS);
		out.affects[6] = GetPoint(POINT_PARTY_DEFENDER_BONUS);
	}

	return true;
}

int CHARACTER::GetLeadershipSkillLevel() const
{
	return GetSkillLevel(SKILL_LEADERSHIP);
}

void CHARACTER::QuerySafeboxSize()
{
	if (m_iSafeboxSize == -1)
	{
		DBManager::instance().ReturnQuery(QID_SAFEBOX_SIZE,
				GetPlayerID(),
				NULL,
				"SELECT size FROM safebox%s WHERE account_id = %u",
				get_table_postfix(),
				GetDesc()->GetAccountTable().id);
	}
}

void CHARACTER::SetSafeboxSize(int iSize)
{
	sys_log(1, "SetSafeboxSize: %s %d", GetName(), iSize);
	m_iSafeboxSize = iSize;
	DBManager::instance().Query("UPDATE safebox%s SET size = %d WHERE account_id = %u", get_table_postfix(), iSize / SAFEBOX_PAGE_SIZE, GetDesc()->GetAccountTable().id);
}

int CHARACTER::GetSafeboxSize() const
{
	return m_iSafeboxSize;
}

void CHARACTER::SetNowWalking(bool bWalkFlag)
{
	//if (m_bNowWalking != bWalkFlag || IsNPC())
	if (m_bNowWalking != bWalkFlag)
	{
		if (bWalkFlag)
		{
			m_bNowWalking = true;
			m_dwWalkStartTime = get_dword_time();
		}
		else
		{
			m_bNowWalking = false;
		}

		//if (m_bNowWalking)
		{
			TPacketGCWalkMode p;
			p.vid = GetVID();
			p.header = HEADER_GC_WALK_MODE;
			p.mode = m_bNowWalking ? WALKMODE_WALK : WALKMODE_RUN;

			PacketView(&p, sizeof(p));
		}

		if (IsNPC())
		{
			if (m_bNowWalking)
				MonsterLog("걷는다");
			else
				MonsterLog("뛴다");
		}

		//sys_log(0, "%s is now %s", GetName(), m_bNowWalking?"walking.":"running.");
	}
}

void CHARACTER::StartStaminaConsume()
{
	if (m_bStaminaConsume)
		return;
	PointChange(POINT_STAMINA, 0);
	m_bStaminaConsume = true;
	//ChatPacket(CHAT_TYPE_COMMAND, "StartStaminaConsume %d %d", STAMINA_PER_STEP * passes_per_sec, GetStamina());
	if (IsStaminaHalfConsume())
		ChatPacket(CHAT_TYPE_COMMAND, "StartStaminaConsume %d %d", STAMINA_PER_STEP * passes_per_sec / 2, GetStamina());
	else
		ChatPacket(CHAT_TYPE_COMMAND, "StartStaminaConsume %d %d", STAMINA_PER_STEP * passes_per_sec, GetStamina());
}

void CHARACTER::StopStaminaConsume()
{
	if (!m_bStaminaConsume)
		return;
	PointChange(POINT_STAMINA, 0);
	m_bStaminaConsume = false;
	ChatPacket(CHAT_TYPE_COMMAND, "StopStaminaConsume %d", GetStamina());
}

bool CHARACTER::IsStaminaConsume() const
{
	return m_bStaminaConsume;
}

bool CHARACTER::IsStaminaHalfConsume() const
{
	return IsEquipUniqueItem(UNIQUE_ITEM_HALF_STAMINA);
}

void CHARACTER::ResetStopTime()
{
	m_dwStopTime = get_dword_time();
}

DWORD CHARACTER::GetStopTime() const
{
	return m_dwStopTime;
}

void CHARACTER::ResetPoint(int iLv)
{
	BYTE bJob = GetJob();

	PointChange(POINT_LEVEL, iLv - GetLevel());

	SetRealPoint(POINT_ST, JobInitialPoints[bJob].st);
	SetPoint(POINT_ST, GetRealPoint(POINT_ST));

	SetRealPoint(POINT_HT, JobInitialPoints[bJob].ht);
	SetPoint(POINT_HT, GetRealPoint(POINT_HT));

	SetRealPoint(POINT_DX, JobInitialPoints[bJob].dx);
	SetPoint(POINT_DX, GetRealPoint(POINT_DX));

	SetRealPoint(POINT_IQ, JobInitialPoints[bJob].iq);
	SetPoint(POINT_IQ, GetRealPoint(POINT_IQ));

	SetRandomHP((iLv - 1) * number(JobInitialPoints[GetJob()].hp_per_lv_begin, JobInitialPoints[GetJob()].hp_per_lv_end));
	SetRandomSP((iLv - 1) * number(JobInitialPoints[GetJob()].sp_per_lv_begin, JobInitialPoints[GetJob()].sp_per_lv_end));

	// @fixme104
	PointChange(POINT_STAT, (MINMAX(1, iLv, g_iStatusPointGetLevelLimit) * 3) + GetPoint(POINT_LEVEL_STEP) - GetPoint(POINT_STAT));

	ComputePoints();

	// PointChange(POINT_HP, GetMaxHP() - GetHP());
	// PointChange(POINT_SP, GetMaxSP() - GetSP());

	PointsPacket();

	LogManager::instance().CharLog(this, 0, "RESET_POINT", "");
}

bool CHARACTER::IsChangeAttackPosition(LPCHARACTER target) const
{
	if (!IsNPC())
		return true;

	DWORD dwChangeTime = AI_CHANGE_ATTACK_POISITION_TIME_NEAR;

	if (DISTANCE_APPROX(GetX() - target->GetX(), GetY() - target->GetY()) >
		AI_CHANGE_ATTACK_POISITION_DISTANCE + GetMobAttackRange())
		dwChangeTime = AI_CHANGE_ATTACK_POISITION_TIME_FAR;

	return get_dword_time() - m_dwLastChangeAttackPositionTime > dwChangeTime;
}

void CHARACTER::GiveRandomSkillBook()
{
	LPITEM item = AutoGiveItem(50300);

	if (NULL != item)
	{
		extern const DWORD GetRandomSkillVnum(BYTE bJob = JOB_MAX_NUM);
		DWORD dwSkillVnum = 0;
		// 50% of getting random books or getting one of the same player's race
		if (!number(0, 1))
			dwSkillVnum = GetRandomSkillVnum(GetJob());
		else
			dwSkillVnum = GetRandomSkillVnum();
		item->SetSocket(0, dwSkillVnum);
	}
}

void CHARACTER::ReviveInvisible(int iDur)
{
	AddAffect(AFFECT_REVIVE_INVISIBLE, POINT_NONE, 0, AFF_REVIVE_INVISIBLE, iDur, 0, true);
}

void CHARACTER::ToggleMonsterLog()
{
	m_bMonsterLog = !m_bMonsterLog;

	if (m_bMonsterLog)
	{
		CHARACTER_MANAGER::instance().RegisterForMonsterLog(this);
	}
	else
	{
		CHARACTER_MANAGER::instance().UnregisterForMonsterLog(this);
	}
}

void CHARACTER::SetGuild(CGuild* pGuild)
{
	if (m_pGuild != pGuild)
	{
		m_pGuild = pGuild;
		UpdatePacket();
	}
}

void CHARACTER::SendGreetMessage()
{
	typeof(DBManager::instance().GetGreetMessage()) v = DBManager::instance().GetGreetMessage();

	for (itertype(v) it = v.begin(); it != v.end(); ++it)
	{
		ChatPacket(CHAT_TYPE_NOTICE, it->c_str());
	}
}

void CHARACTER::BeginStateEmpty()
{
	MonsterLog("!");
}

void CHARACTER::EffectPacket(int enumEffectType)
{
	TPacketGCSpecialEffect p;

	p.header = HEADER_GC_SEPCIAL_EFFECT;
	p.type = enumEffectType;
	p.vid = GetVID();

	PacketAround(p);
}

void CHARACTER::SpecificEffectPacket(const char filename[MAX_EFFECT_FILE_NAME])
{
	TPacketGCSpecificEffect p;

	p.header = HEADER_GC_SPECIFIC_EFFECT;
	p.vid = GetVID();
	memcpy (p.effect_file, filename, MAX_EFFECT_FILE_NAME);

	PacketAround(p);
}

void CHARACTER::MonsterChat(BYTE bMonsterChatType)
{
	if (IsPC())
		return;

	char sbuf[256+1];

	if (IsMonster())
	{
		if (number(0, 60))
			return;

		snprintf(sbuf, sizeof(sbuf),
				"(locale.monster_chat[%i] and locale.monster_chat[%i][%d] or '')",
				GetRaceNum(), GetRaceNum(), bMonsterChatType*3 + number(1, 3));
	}
	else
	{
		if (bMonsterChatType != MONSTER_CHAT_WAIT)
			return;

		if (IsGuardNPC())
		{
			if (number(0, 6))
				return;
		}
		else
		{
			if (number(0, 30))
				return;
		}

		snprintf(sbuf, sizeof(sbuf), "(locale.monster_chat[%i] and locale.monster_chat[%i][number(1, table.getn(locale.monster_chat[%i]))] or '')", GetRaceNum(), GetRaceNum(), GetRaceNum());
	}

	std::string text = quest::ScriptToString(sbuf);

	if (text.empty())
		return;

	struct packet_chat pack_chat;

	pack_chat.header    = HEADER_GC_CHAT;
	pack_chat.size	= sizeof(struct packet_chat) + text.size() + 1;
	pack_chat.type      = CHAT_TYPE_TALKING;
	pack_chat.id        = GetVID();
	pack_chat.bEmpire	= 0;

	TEMP_BUFFER buf;
	buf.write(pack_chat);
	buf.write(text.c_str(), text.size() + 1);

	PacketAround(buf.read_peek(), buf.size());
}

void CHARACTER::SetQuestNPCID(DWORD vid)
{
	/*if (vid && m_dwQuestNPCVID)
	{
		quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(GetPlayerID());
		if (pPC && pPC->IsRunning())
		{
			sys_err("cannot reset quest npc id - already running quest [%u %s]", GetPlayerID(), GetName());
			return;
		}
	}*/

	m_dwQuestNPCVID = vid;
}

LPCHARACTER CHARACTER::GetQuestNPC() const
{
	return CHARACTER_MANAGER::instance().Find(m_dwQuestNPCVID);
}

void CHARACTER::SetQuestItemPtr(LPITEM item)
{
	m_pQuestItem = item;
}

void CHARACTER::ClearQuestItemPtr()
{
	m_pQuestItem = NULL;
}

LPITEM CHARACTER::GetQuestItemPtr() const
{
	return m_pQuestItem;
}

#ifdef ENABLE_QUEST_DND_EVENT
void CHARACTER::SetQuestDNDItemPtr(LPITEM item)
{
	m_pQuestDNDItem = item;
}

void CHARACTER::ClearQuestDNDItemPtr()
{
	m_pQuestDNDItem = NULL;
}

LPITEM CHARACTER::GetQuestDNDItemPtr() const
{
	return m_pQuestDNDItem;
}
#endif

LPDUNGEON CHARACTER::GetDungeonForce() const
{
	if (m_lWarpMapIndex > 10000)
		return CDungeonManager::instance().FindByMapIndex(m_lWarpMapIndex);

	return m_pkDungeon;
}

void CHARACTER::SetBlockMode(BYTE bFlag)
{
	m_pointsInstant.bBlockMode = bFlag;

	ChatPacket(CHAT_TYPE_COMMAND, "setblockmode %d", m_pointsInstant.bBlockMode);

	SetQuestFlag("game_option.block_exchange", bFlag & BLOCK_EXCHANGE ? 1 : 0);
	SetQuestFlag("game_option.block_party_invite", bFlag & BLOCK_PARTY_INVITE ? 1 : 0);
	SetQuestFlag("game_option.block_guild_invite", bFlag & BLOCK_GUILD_INVITE ? 1 : 0);
	SetQuestFlag("game_option.block_whisper", bFlag & BLOCK_WHISPER ? 1 : 0);
	SetQuestFlag("game_option.block_messenger_invite", bFlag & BLOCK_MESSENGER_INVITE ? 1 : 0);
	SetQuestFlag("game_option.block_party_request", bFlag & BLOCK_PARTY_REQUEST ? 1 : 0);
}

void CHARACTER::SetBlockModeForce(BYTE bFlag)
{
	m_pointsInstant.bBlockMode = bFlag;
	ChatPacket(CHAT_TYPE_COMMAND, "setblockmode %d", m_pointsInstant.bBlockMode);
}

bool CHARACTER::IsGuardNPC() const
{
	return IsNPC() && (GetRaceNum() == 11000 || GetRaceNum() == 11002 || GetRaceNum() == 11004);
}

int CHARACTER::GetPolymorphPower() const
{
	if (test_server)
	{
		int value = quest::CQuestManager::instance().GetEventFlag("poly");
		if (value)
			return value;
	}
	return aiPolymorphPowerByLevel[MINMAX(0, GetSkillLevel(SKILL_POLYMORPH), 40)];
}

void CHARACTER::SetPolymorph(DWORD dwRaceNum, bool bMaintainStat)
{
#ifdef ENABLE_WOLFMAN_CHARACTER
	if (dwRaceNum < MAIN_RACE_MAX_NUM)
#else
	if (dwRaceNum < JOB_MAX_NUM)
#endif
	{
		dwRaceNum = 0;
		bMaintainStat = false;
	}

	if (m_dwPolymorphRace == dwRaceNum)
		return;

	m_bPolyMaintainStat = bMaintainStat;
	m_dwPolymorphRace = dwRaceNum;

	sys_log(0, "POLYMORPH: %s race %u ", GetName(), dwRaceNum);

	if (dwRaceNum != 0)
		StopRiding();

	SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);
	m_afAffectFlag.Set(AFF_SPAWN);

	ViewReencode();

	REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);

	if (!bMaintainStat)
	{
		PointChange(POINT_ST, 0);
		PointChange(POINT_DX, 0);
		PointChange(POINT_IQ, 0);
		PointChange(POINT_HT, 0);
	}

	SetValidComboInterval(0);
	SetComboSequence(0);

	ComputeBattlePoints();
}

int CHARACTER::GetQuestFlag(const std::string& flag) const
{
	// @fixme198 BEGIN
	if (!IsPC())
	{
		sys_err("Trying to get qf %s from non player character", flag.c_str());
		return 0;
	}
	// @fixme198 END

	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());

	// @fixme198 BEGIN
	if (!pPC)
	{
		sys_err("Nullpointer when trying to access questflag %s for player with pid %u", flag.c_str(), GetPlayerID());
		return 0;
	}
	// @fixme198 END
	return pPC->GetFlag(flag);
}

void CHARACTER::SetQuestFlag(const std::string& flag, int value)
{
	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());
	// @fixme198 BEGIN
	if (!pPC)
	{
		sys_err("Nullpointer when trying to set questflag %s for player with pid %u", flag.c_str(), GetPlayerID());
		return;
	}
	// @fixme198 END
	pPC->SetFlag(flag, value);
}

void CHARACTER::DetermineDropMetinStone()
{
	#ifdef ENABLE_NEWSTUFF
	if (g_NoDropMetinStone)
	{
		m_dwDropMetinStone = 0;
		return;
	}
	#endif

	static const DWORD c_adwMetin[] =
	{
		#if defined(ENABLE_WOLFMAN_CHARACTER) && defined(USE_WOLFMAN_STONES)
		28012,
		#endif
		28030,
		28031,
		28032,
		28033,
		28034,
		28035,
		28036,
		28037,
		28038,
		28039,
		28040,
		28041,
		28042,
		28043,
		#if defined(ENABLE_MAGIC_REDUCTION_SYSTEM) && defined(USE_MAGIC_REDUCTION_STONES)
		28044,
		28045,
		#endif
	};

	DWORD stone_num = GetRaceNum();
	int idx = std::lower_bound(aStoneDrop, aStoneDrop+STONE_INFO_MAX_NUM, stone_num) - aStoneDrop;
	if (idx >= STONE_INFO_MAX_NUM || aStoneDrop[idx].dwMobVnum != stone_num)
	{
		m_dwDropMetinStone = 0;
	}
	else
	{
		const SStoneDropInfo & info = aStoneDrop[idx];
		m_bDropMetinStonePct = info.iDropPct;
		{
			m_dwDropMetinStone = c_adwMetin[number(0, sizeof(c_adwMetin)/sizeof(DWORD) - 1)];
			int iGradePct = number(1, 100);
			for (int iStoneLevel = 0; iStoneLevel < STONE_LEVEL_MAX_NUM; iStoneLevel ++)
			{
				int iLevelGradePortion = info.iLevelPct[iStoneLevel];
				if (iGradePct <= iLevelGradePortion)
				{
					break;
				}
				else
				{
					iGradePct -= iLevelGradePortion;
					m_dwDropMetinStone += 100;
				}
			}
		}
	}
}

void CHARACTER::SendEquipment(LPCHARACTER ch)
{
	TPacketViewEquip p;
	p.header = HEADER_GC_VIEW_EQUIP;
	p.vid    = GetVID();
	for (int i = 0; i<WEAR_MAX_NUM; i++)
	{
		LPITEM item = GetWear(i);
		if (item)
		{
			p.equips[i].vnum = item->GetVnum();
			p.equips[i].count = item->GetCount();

			thecore_memcpy(p.equips[i].alSockets, item->GetSockets(), sizeof(p.equips[i].alSockets));
			thecore_memcpy(p.equips[i].aAttr, item->GetAttributes(), sizeof(p.equips[i].aAttr));
		}
		else
		{
			p.equips[i].vnum = 0;
		}
	}
	ch->GetDesc()->Packet(p);
}

bool CHARACTER::CanSummon(int iLeaderShip)
{
	return ((iLeaderShip >= 20) || ((iLeaderShip >= 12) && ((m_dwLastDeadTime + 180) > get_dword_time())));
}

// #define ENABLE_MOUNT_ENTITY_REFRESH
void CHARACTER::MountVnum(DWORD vnum)
{
	if (m_dwMountVnum == vnum)
		return;

	if ((m_dwMountVnum != 0) && (vnum != 0)) //@fixme108 set recursively to 0 for eventuality
		MountVnum(0);

	m_dwMountVnum = vnum;
	m_dwMountTime = get_dword_time();

	if (m_bIsObserver)
		return;

	m_posDest.x = m_posStart.x = GetX();
	m_posDest.y = m_posStart.y = GetY();

	#ifdef ENABLE_MOUNT_ENTITY_REFRESH
	// EncodeRemovePacket(this); // commented, otherwise it may warp you back
	#endif

	EncodeInsertPacket(this);

	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = (it++)->first;

		#ifdef ENABLE_MOUNT_ENTITY_REFRESH
		if (entity->IsType(ENTITY_CHARACTER))
		{
			EncodeRemovePacket(entity);
			if (!m_bIsObserver)
				EncodeInsertPacket(entity);

			if (!entity->IsObserverMode())
					entity->EncodeInsertPacket(this);
		}
		else
			EncodeInsertPacket(entity);
		#else
		EncodeInsertPacket(entity);
		#endif
	}

	SetValidComboInterval(0);
	SetComboSequence(0);

	ComputePoints();
}

namespace {
	class FuncCheckWarp
	{
		public:
			FuncCheckWarp(LPCHARACTER pkWarp)
			{
				m_lTargetY = 0;
				m_lTargetX = 0;

				m_lX = pkWarp->GetX();
				m_lY = pkWarp->GetY();

				m_bInvalid = false;
				m_bEmpire = pkWarp->GetEmpire();

				char szTmp[64];

				if (3 != sscanf(pkWarp->GetName(), " %s %ld %ld ", szTmp, &m_lTargetX, &m_lTargetY))
				{
					if (number(1, 100) < 5)
						sys_err("Warp NPC name wrong : vnum(%d) name(%s)", pkWarp->GetRaceNum(), pkWarp->GetName());

					m_bInvalid = true;

					return;
				}

				m_lTargetX *= 100;
				m_lTargetY *= 100;

				m_bUseWarp = true;

				if (pkWarp->IsGoto())
				{
					LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(pkWarp->GetMapIndex());
					m_lTargetX += pkSectreeMap->m_setting.iBaseX;
					m_lTargetY += pkSectreeMap->m_setting.iBaseY;
					m_bUseWarp = false;
				}
			}

			bool Valid()
			{
				return !m_bInvalid;
			}

			void operator () (LPENTITY ent)
			{
				if (!Valid())
					return;

				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				LPCHARACTER pkChr = (LPCHARACTER) ent;

				if (!pkChr->IsPC())
					return;

				int iDist = DISTANCE_APPROX(pkChr->GetX() - m_lX, pkChr->GetY() - m_lY);

				if (iDist > 300)
					return;

				if (m_bEmpire && pkChr->GetEmpire() && m_bEmpire != pkChr->GetEmpire())
					return;

				if (pkChr->IsHack())
					return;

				if (!pkChr->CanHandleItem(false, true))
					return;

				if (m_bUseWarp)
					pkChr->WarpSet(m_lTargetX, m_lTargetY);
				else
				{
					pkChr->Show(pkChr->GetMapIndex(), m_lTargetX, m_lTargetY);
					pkChr->Stop();
				}
			}

			bool m_bInvalid;
			bool m_bUseWarp;

			long m_lX;
			long m_lY;
			long m_lTargetX;
			long m_lTargetY;

			BYTE m_bEmpire;
	};
}

EVENTFUNC(warp_npc_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "warp_npc_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (!ch->GetSectree())
	{
		ch->m_pkWarpNPCEvent = NULL;
		return 0;
	}

	FuncCheckWarp f(ch);
	if (f.Valid())
		ch->GetSectree()->ForEachAround(f);

	return passes_per_sec / 2;
}


void CHARACTER::StartWarpNPCEvent()
{
	if (m_pkWarpNPCEvent)
		return;

	if (!IsWarp() && !IsGoto())
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkWarpNPCEvent = event_create(warp_npc_event, info, passes_per_sec / 2);
}

void CHARACTER::SyncPacket()
{
	TEMP_BUFFER buf;

	TPacketCGSyncPositionElement elem;

	elem.dwVID = GetVID();
	elem.lX = GetX();
	elem.lY = GetY();

	TPacketGCSyncPosition pack;

	pack.bHeader = HEADER_GC_SYNC_POSITION;
	pack.wSize = sizeof(TPacketGCSyncPosition) + sizeof(elem);

	buf.write(pack);
	buf.write(elem);

	PacketAround(buf.read_peek(), buf.size());
}

LPCHARACTER CHARACTER::GetMarryPartner() const
{
	return m_pkChrMarried;
}

void CHARACTER::SetMarryPartner(LPCHARACTER ch)
{
	m_pkChrMarried = ch;
}

int CHARACTER::GetMarriageBonus(DWORD dwItemVnum, bool bSum)
{
	if (IsNPC())
		return 0;

	marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());

	if (!pMarriage)
		return 0;

	return pMarriage->GetBonus(dwItemVnum, bSum, this);
}

void CHARACTER::ConfirmWithMsg(const char* szMsg, int iTimeout, DWORD dwRequestPID)
{
	if (!IsPC())
		return;

	TPacketGCQuestConfirm p;

	p.header = HEADER_GC_QUEST_CONFIRM;
	p.requestPID = dwRequestPID;
	p.timeout = iTimeout;
	strlcpy(p.msg, szMsg, sizeof(p.msg));

	GetDesc()->Packet(p);
}

int CHARACTER::GetPremiumRemainSeconds(BYTE bType) const
{
	if (bType >= PREMIUM_MAX_NUM)
		return 0;

	return m_aiPremiumTimes[bType] - get_global_time();
}

#ifdef ENABLE_WARP_REGULATION
bool CHARACTER::WarpToPID(DWORD dwPID, bool IsGM)
#else
bool CHARACTER::WarpToPID(DWORD dwPID)
#endif
{
	LPCHARACTER victim;
	if ((victim = (CHARACTER_MANAGER::instance().FindByPID(dwPID))))
	{
		int mapIdx = victim->GetMapIndex();

		#ifdef ENABLE_WARP_REGULATION
		if (IsGM)
		{
			#ifdef ENABLE_CMD_WARP_IN_DUNGEON
			WarpSet(victim->GetX(), victim->GetY(), mapIdx);
			#else
			WarpSet(victim->GetX(), victim->GetY());
			#endif
			return true;
		}
		#endif

		if (IS_SUMMONABLE_ZONE(mapIdx))
		{
			if (CAN_ENTER_ZONE(this, mapIdx))
			{
				WarpSet(victim->GetX(), victim->GetY());
			}
			else
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 있는 곳으로 워프할 수 없습니다."));
				return false;
			}
		}
		else
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 있는 곳으로 워프할 수 없습니다."));
			return false;
		}
	}
	else
	{
		CCI * pcci = P2P_MANAGER::instance().FindByPID(dwPID);
		if (!pcci)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 온라인 상태가 아닙니다."));
			return false;
		}

		#ifdef ENABLE_WARP_REGULATION
		if (IsGM && pcci->pkDesc)
		{
			TPacketGGFindPosition p;
			p.dwFromPID = GetPlayerID();
			p.dwTargetPID = dwPID;
			p.IsGmWarp = true;
			pcci->pkDesc->Packet(&p, sizeof(TPacketGGFindPosition));
			return true;
		}
		#endif

		if (pcci->bChannel != g_bChannel)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 %d 채널에 있습니다. (현재 채널 %d)"), pcci->bChannel, g_bChannel);
			return false;
		}
		else if (false == IS_SUMMONABLE_ZONE(pcci->lMapIndex))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 있는 곳으로 워프할 수 없습니다."));
			return false;
		}
		else
		{
			if (!CAN_ENTER_ZONE(this, pcci->lMapIndex))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 있는 곳으로 워프할 수 없습니다."));
				return false;
			}

			TPacketGGFindPosition p;
			p.header = HEADER_GG_FIND_POSITION;
			p.dwFromPID = GetPlayerID();
			p.dwTargetPID = dwPID;
			pcci->pkDesc->Packet(p);

			if (test_server)
				ChatPacket(CHAT_TYPE_PARTY, "sent find position packet for teleport");
		}
	}

	return true;
}

// ADD_REFINE_BUILDING
CGuild* CHARACTER::GetRefineGuild() const
{
	LPCHARACTER chRefineNPC = CHARACTER_MANAGER::instance().Find(m_dwRefineNPCVID);

	return (chRefineNPC ? chRefineNPC->GetGuild() : NULL);
}

bool CHARACTER::IsRefineThroughGuild() const
{
	return GetRefineGuild() != NULL;
}

#ifdef ENABLE_FULL_YANG
int64_t CHARACTER::ComputeRefineFee(int64_t iCost, int iMultiply) const
#else
int CHARACTER::ComputeRefineFee(int iCost, int iMultiply) const
#endif
{
	CGuild* pGuild = GetRefineGuild();
	if (pGuild)
	{
		if (pGuild == GetGuild())
			return iCost * iMultiply * 9 / 10;

		LPCHARACTER chRefineNPC = CHARACTER_MANAGER::instance().Find(m_dwRefineNPCVID);
		if (chRefineNPC && chRefineNPC->GetEmpire() != GetEmpire())
			return iCost * iMultiply * 3;

		return iCost * iMultiply;
	}
	else
		return iCost;
}

#ifdef ENABLE_FULL_YANG
void CHARACTER::PayRefineFee(int64_t iTotalMoney)
{
	int64_t iFee = iTotalMoney / 10;
	CGuild* pGuild = GetRefineGuild();

	int64_t iRemain = iTotalMoney;
#else
void CHARACTER::PayRefineFee(int iTotalMoney)
{
	int iFee = iTotalMoney / 10;
	CGuild* pGuild = GetRefineGuild();

	int iRemain = iTotalMoney;
#endif
	if (pGuild)
	{
		if (pGuild != GetGuild())
		{
			pGuild->RequestDepositMoney(this, iFee);
			iRemain -= iFee;
		}
	}

	#ifdef ENABLE_FULL_YANG
	GoldChange(-iRemain);
	#else
	PointChange(POINT_GOLD, -iRemain);
	#endif
}
// END_OF_ADD_REFINE_BUILDING

bool CHARACTER::IsHack(bool bSendMsg, bool bCheckShopOwner, int limittime)
{
	const int iPulse = thecore_pulse();

	if (test_server)
		bSendMsg = true;

	if (iPulse - GetSafeboxLoadTime() < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("창고를 연후 %d초 이내에는 다른곳으로 이동할수 없습니다."), limittime);

		if (test_server)
			ChatPacket(CHAT_TYPE_INFO, "[TestOnly]Pulse %d LoadTime %d PASS %d", iPulse, GetSafeboxLoadTime(), PASSES_PER_SEC(limittime));
		return true;
	}

	if (bCheckShopOwner)
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
		{
			if (bSendMsg)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래창,창고 등을 연 상태에서는 다른곳으로 이동,종료 할수 없습니다"));

			return true;
		}
	}
	else
	{
		if (GetExchange() || GetMyShop() || IsOpenSafebox() || IsCubeOpen())
		{
			if (bSendMsg)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래창,창고 등을 연 상태에서는 다른곳으로 이동,종료 할수 없습니다"));

			return true;
		}
	}

	//PREVENT_PORTAL_AFTER_EXCHANGE
	if (iPulse - GetExchangeTime() < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래 후 %d초 이내에는 다른지역으로 이동 할 수 없습니다."), limittime );
		return true;
	}
	//END_PREVENT_PORTAL_AFTER_EXCHANGE

	//PREVENT_ITEM_COPY
	if (iPulse - GetMyShopTime() < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래 후 %d초 이내에는 다른지역으로 이동 할 수 없습니다."), limittime);
		return true;
	}

	if (iPulse - GetRefineTime() < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 개량후 %d초 이내에는 귀환부,귀환기억부를 사용할 수 없습니다."), limittime);
		return true;
	}
	//END_PREVENT_ITEM_COPY

	return false;
}

BOOL CHARACTER::IsMonarch() const
{
	//MONARCH_LIMIT
	if (CMonarch::instance().IsMonarch(GetPlayerID(), GetEmpire()))
		return true;

	return false;

	//END_MONARCH_LIMIT
}
void CHARACTER::Say(const std::string & s)
{
	struct ::packet_script packet_script;

	packet_script.header = HEADER_GC_SCRIPT;
	packet_script.skin = 1;
	packet_script.src_size = s.size();
	packet_script.size = packet_script.src_size + sizeof(struct packet_script);

	TEMP_BUFFER buf;

	buf.write(packet_script);
	buf.write(s.data(), s.size());

	if (IsPC())
	{
		GetDesc()->Packet(buf.read_peek(), buf.size());
	}
}

//
// Monarch
//
void CHARACTER::InitMC()
{
	for (int n = 0; n < MI_MAX; ++n)
	{
		m_dwMonarchCooltime[n] = thecore_pulse();
	}

	m_dwMonarchCooltimelimit[MI_HEAL] = PASSES_PER_SEC(MC_HEAL);
	m_dwMonarchCooltimelimit[MI_WARP] = PASSES_PER_SEC(MC_WARP);
	m_dwMonarchCooltimelimit[MI_TRANSFER] = PASSES_PER_SEC(MC_TRANSFER);
	m_dwMonarchCooltimelimit[MI_TAX] = PASSES_PER_SEC(MC_TAX);
	m_dwMonarchCooltimelimit[MI_SUMMON] = PASSES_PER_SEC(MC_SUMMON);

	m_dwMonarchCooltime[MI_HEAL] -= PASSES_PER_SEC(GetMCL(MI_HEAL));
	m_dwMonarchCooltime[MI_WARP] -= PASSES_PER_SEC(GetMCL(MI_WARP));
	m_dwMonarchCooltime[MI_TRANSFER] -= PASSES_PER_SEC(GetMCL(MI_TRANSFER));
	m_dwMonarchCooltime[MI_TAX] -= PASSES_PER_SEC(GetMCL(MI_TAX));
	m_dwMonarchCooltime[MI_SUMMON] -= PASSES_PER_SEC(GetMCL(MI_SUMMON));
}

DWORD CHARACTER::GetMC(enum MONARCH_INDEX e) const
{
	return m_dwMonarchCooltime[e];
}

void CHARACTER::SetMC(enum MONARCH_INDEX e)
{
	m_dwMonarchCooltime[e] = thecore_pulse();
}

bool CHARACTER::IsMCOK(enum MONARCH_INDEX e) const
{
	int iPulse = thecore_pulse();

	if ((iPulse -  GetMC(e)) <  GetMCL(e))
	{
		if (test_server)
			sys_log(0, " Pulse %d cooltime %d, limit %d", iPulse, GetMC(e), GetMCL(e));

		return false;
	}

	if (test_server)
		sys_log(0, " Pulse %d cooltime %d, limit %d", iPulse, GetMC(e), GetMCL(e));

	return true;
}

DWORD CHARACTER::GetMCL(enum MONARCH_INDEX e) const
{
	return m_dwMonarchCooltimelimit[e];
}

DWORD CHARACTER::GetMCLTime(enum MONARCH_INDEX e) const
{
	int iPulse = thecore_pulse();

	if (test_server)
		sys_log(0, " Pulse %d cooltime %d, limit %d", iPulse, GetMC(e), GetMCL(e));

	return  (GetMCL(e)) / passes_per_sec   -  (iPulse - GetMC(e)) / passes_per_sec;
}

bool CHARACTER::IsSiegeNPC() const
{
	return IsNPC() && (GetRaceNum() == 11000 || GetRaceNum() == 11002 || GetRaceNum() == 11004);
}

//------------------------------------------------
void CHARACTER::UpdateDepositPulse()
{
	m_deposit_pulse = thecore_pulse() + PASSES_PER_SEC(60*5);
}

bool CHARACTER::CanDeposit() const
{
	return (m_deposit_pulse == 0 || (m_deposit_pulse < thecore_pulse()));
}
//------------------------------------------------

ESex GET_SEX(LPCHARACTER ch)
{
	switch (ch->GetRaceNum())
	{
		case MAIN_RACE_WARRIOR_M:
		case MAIN_RACE_SURA_M:
		case MAIN_RACE_ASSASSIN_M:
		case MAIN_RACE_SHAMAN_M:
		#ifdef ENABLE_WOLFMAN_CHARACTER
		case MAIN_RACE_WOLFMAN_M:
		#endif
			return SEX_MALE;

		case MAIN_RACE_ASSASSIN_W:
		case MAIN_RACE_SHAMAN_W:
		case MAIN_RACE_WARRIOR_W:
		case MAIN_RACE_SURA_W:
			return SEX_FEMALE;
	}

	/* default sex = male */
	return SEX_MALE;
}

int CHARACTER::GetHPPct() const
{
	if (GetMaxHP() <= 0) // @fixme136
		return 0;
	return (GetHP() * 100) / GetMaxHP();
}

bool CHARACTER::IsBerserk() const
{
	if (m_pkMobInst != NULL)
		return m_pkMobInst->m_IsBerserk;
	else
		return false;
}

void CHARACTER::SetBerserk(bool mode)
{
	if (m_pkMobInst != NULL)
		m_pkMobInst->m_IsBerserk = mode;
}

bool CHARACTER::IsGodSpeed() const
{
	if (m_pkMobInst != NULL)
	{
		return m_pkMobInst->m_IsGodSpeed;
	}
	else
	{
		return false;
	}
}

void CHARACTER::SetGodSpeed(bool mode)
{
	if (m_pkMobInst != NULL)
	{
		m_pkMobInst->m_IsGodSpeed = mode;

		if (mode == true)
		{
			SetPoint(POINT_ATT_SPEED, 250);
		}
		else
		{
			SetPoint(POINT_ATT_SPEED, m_pkMobData->m_table.sAttackSpeed);
		}
	}
}

bool CHARACTER::IsDeathBlow() const
{
	if (number(1, 100) <= m_pkMobData->m_table.bDeathBlowPoint)
	{
		return true;
	}
	else
	{
		return false;
	}
}

struct FFindReviver
{
	FFindReviver()
	{
		pChar = NULL;
		HasReviver = false;
	}

	void operator() (LPCHARACTER ch)
	{
		if (ch->IsMonster() != true)
		{
			return;
		}

		if (ch->IsReviver() == true && pChar != ch && ch->IsDead() != true)
		{
			if (number(1, 100) <= ch->GetMobTable().bRevivePoint)
			{
				HasReviver = true;
				pChar = ch;
			}
		}
	}

	LPCHARACTER pChar;
	bool HasReviver;
};

bool CHARACTER::HasReviverInParty() const
{
	LPPARTY party = GetParty();

	if (party != NULL)
	{
		if (party->GetMemberCount() == 1) return false;

		FFindReviver f;
		party->ForEachMemberPtr(f);
		return f.HasReviver;
	}

	return false;
}

bool CHARACTER::IsRevive() const
{
	if (m_pkMobInst != NULL)
	{
		return m_pkMobInst->m_IsRevive;
	}

	return false;
}

void CHARACTER::SetRevive(bool mode)
{
	if (m_pkMobInst != NULL)
	{
		m_pkMobInst->m_IsRevive = mode;
	}
}

#define IS_SPEED_HACK_PLAYER(ch) (ch->m_speed_hack_count > SPEEDHACK_LIMIT_COUNT)

EVENTFUNC(check_speedhack_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "check_speedhack_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (NULL == ch || ch->IsNPC())
		return 0;

	if (IS_SPEED_HACK_PLAYER(ch))
	{
		// write hack log
		LogManager::instance().SpeedHackLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), ch->m_speed_hack_count);

		if (g_bEnableSpeedHackCrash)
		{
			// close connection
			LPDESC desc = ch->GetDesc();

			if (desc)
			{
				DESC_MANAGER::instance().DestroyDesc(desc);
				return 0;
			}
		}
	}

	ch->m_speed_hack_count = 0;

	ch->ResetComboHackCount();
	return PASSES_PER_SEC(60);
}

void CHARACTER::StartCheckSpeedHackEvent()
{
	if (m_pkCheckSpeedHackEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkCheckSpeedHackEvent = event_create(check_speedhack_event, info, PASSES_PER_SEC(60));
}

void CHARACTER::GoHome()
{
	WarpSet(EMPIRE_START_X(GetEmpire()), EMPIRE_START_Y(GetEmpire()));
}

void CHARACTER::SendGuildName(CGuild* pGuild)
{
	if (NULL == pGuild) return;

	DESC	*desc = GetDesc();

	if (NULL == desc) return;
	if (m_known_guild.find(pGuild->GetID()) != m_known_guild.end()) return;

	m_known_guild.emplace(pGuild->GetID());

	TPacketGCGuildName	pack;
	memset(&pack, 0x00, sizeof(pack));

	pack.header		= HEADER_GC_GUILD;
	pack.subheader	= GUILD_SUBHEADER_GC_GUILD_NAME;
	pack.size		= sizeof(TPacketGCGuildName);
	pack.guildID	= pGuild->GetID();
	memcpy(pack.guildName, pGuild->GetName(), GUILD_NAME_MAX_LEN);

	desc->Packet(pack);
}

void CHARACTER::SendGuildName(DWORD dwGuildID)
{
	SendGuildName(CGuildManager::instance().FindGuild(dwGuildID));
}

EVENTFUNC(destroy_when_idle_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "destroy_when_idle_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (ch->GetVictim())
	{
		return PASSES_PER_SEC(300);
	}

	sys_log(1, "DESTROY_WHEN_IDLE: %s", ch->GetName());

	ch->m_pkDestroyWhenIdleEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

void CHARACTER::StartDestroyWhenIdleEvent()
{
	if (m_pkDestroyWhenIdleEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkDestroyWhenIdleEvent = event_create(destroy_when_idle_event, info, PASSES_PER_SEC(300));
}

void CHARACTER::SetComboSequence(BYTE seq)
{
	m_bComboSequence = seq;
}

BYTE CHARACTER::GetComboSequence() const
{
	return m_bComboSequence;
}

void CHARACTER::SetLastComboTime(DWORD time)
{
	m_dwLastComboTime = time;
}

DWORD CHARACTER::GetLastComboTime() const
{
	return m_dwLastComboTime;
}

void CHARACTER::SetValidComboInterval(int interval)
{
	m_iValidComboInterval = interval;
}

int CHARACTER::GetValidComboInterval() const
{
	return m_iValidComboInterval;
}

BYTE CHARACTER::GetComboIndex() const
{
	return m_bComboIndex;
}

void CHARACTER::IncreaseComboHackCount(int k)
{
	m_iComboHackCount += k;

	if (m_iComboHackCount >= 10)
	{
		if (GetDesc())
			if (GetDesc()->DelayedDisconnect(number(2, 7)))
			{
				sys_log(0, "COMBO_HACK_DISCONNECT: %s count: %d", GetName(), m_iComboHackCount);
				LogManager::instance().HackLog("Combo", this);
			}
	}
}

void CHARACTER::ResetComboHackCount()
{
	m_iComboHackCount = 0;
}

void CHARACTER::SkipComboAttackByTime(int interval)
{
	m_dwSkipComboAttackByTime = get_dword_time() + interval;
}

DWORD CHARACTER::GetSkipComboAttackByTime() const
{
	return m_dwSkipComboAttackByTime;
}

void CHARACTER::ResetChatCounter()
{
	m_bChatCounter = 0;
}

BYTE CHARACTER::IncreaseChatCounter()
{
	return ++m_bChatCounter;
}

BYTE CHARACTER::GetChatCounter() const
{
	return m_bChatCounter;
}

bool CHARACTER::IsRiding() const
{
	return IsHorseRiding() || GetMountVnum();
}

bool CHARACTER::CanWarp() const
{
	const int iPulse = thecore_pulse();
	const int limit_time = PASSES_PER_SEC(g_nPortalLimitTime);

	if ((iPulse - GetSafeboxLoadTime()) < limit_time)
		return false;

	if ((iPulse - GetExchangeTime()) < limit_time)
		return false;

	if ((iPulse - GetMyShopTime()) < limit_time)
		return false;

	if ((iPulse - GetRefineTime()) < limit_time)
		return false;

	if (PreventTradeWindow(WND_ALL))
		return false;

	return true;
}

DWORD CHARACTER::GetNextExp() const
{
	if (PLAYER_MAX_LEVEL_CONST < GetLevel())
		return 2500000000u;
	else
		return exp_table[GetLevel()];
}

int	CHARACTER::GetSkillPowerByLevel(int level, bool bMob) const
{
	return CTableBySkill::instance().GetSkillPowerByLevelFromType(GetJob(), GetSkillGroup(), MINMAX(0, level, SKILL_MAX_LEVEL), bMob);
}

#ifdef ENABLE_IKASHOP_RENEWAL
void CHARACTER::SetIkarusShopSafebox(SAFEBOX_HANDLE pk)
{
	if (m_pkIkarusShopSafebox && pk == nullptr)
		m_pkIkarusShopSafebox->SetOwner(nullptr);

	else if (m_pkIkarusShopSafebox == nullptr && pk)
		pk->SetOwner(this);

	m_pkIkarusShopSafebox  = pk;
}

void CHARACTER::SetIkarusShopGuest(SHOP_HANDLE pkShop)
{
	if (m_pkIkarusShopGuest)
		m_pkIkarusShopGuest->RemoveGuest(this);
	m_pkIkarusShopGuest = pkShop;
	if (pkShop)
		pkShop->AddGuest(this);
}

void CHARACTER::SetIkarusAuctionGuest(AUCTION_HANDLE pk)
{
	if (m_pkIkarusAuctionGuest)
		m_pkIkarusAuctionGuest->RemoveGuest(this);
	m_pkIkarusAuctionGuest = pk;
	if (pk)
		pk->AddGuest(this);
}

bool CHARACTER::CanTakeInventoryItem(LPITEM item, TItemPos* pos)
{
	// storing result in signed value
	int emptyPos = -1;

	if (item->IsDragonSoul())
	{
		pos->cell = emptyPos = GetEmptyDragonSoulInventory(item);
		pos->window_type = DRAGON_SOUL_INVENTORY;
	}
	#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsUpgradeItem())
	{
		pos->cell = emptyPos = GetEmptyUpgradeInventory(item);
		pos->window_type = UPGRADE_INVENTORY;
	}
	else if (item->IsBook())
	{
		pos->cell = emptyPos = GetEmptyBookInventory(item);
		pos->window_type = BOOK_INVENTORY;
	}
	else if (item->IsStone())
	{
		pos->cell = emptyPos = GetEmptyStoneInventory(item);
		pos->window_type = STONE_INVENTORY;
	}
	else if (item->IsChest())
	{
		pos->cell = emptyPos = GetEmptyChestInventory(item);
		pos->window_type = CHEST_INVENTORY;
	}
	#endif
	else
	{
		pos->cell = emptyPos = GetEmptyInventory(item->GetSize());
		pos->window_type = INVENTORY;
	}

	return emptyPos != -1;
}

bool CHARACTER::IkarusShopFloodCheck(ShopActionWeight weight)
{
	// making timestamps
	using namespace std::chrono;
	const auto now = duration<double>(high_resolution_clock::now().time_since_epoch()).count();
	const auto elapsedTime = now - 60.0; // 1 minute ago

	// cleaning up register
	std::erase_if(m_IkarusShopActionRegister, [&](auto& pair){ return pair.second < elapsedTime; });

	// getting total weight
	const auto totalWeight = std::accumulate(m_IkarusShopActionRegister.begin(), m_IkarusShopActionRegister.end(), 0UL,
		[](auto total, auto& pair){ return total + pair.first; });

	// checking weight value
	if (totalWeight + weight > OFFLINESHOP_MAX_WEIGHT_PER_PLAYER){
		ikashop::GetManager().SendPopupMessage(this, "IKASHOP_SERVER_POPUP_MESSAGE_PLEASE_SLOW_DOWN");
		return false;
	}

	// registering the action
	m_IkarusShopActionRegister.emplace_back(std::make_pair(weight, now));
	return true;
}
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
std::vector<LPITEM> CHARACTER::GetAcceMaterials()
{
	if (!m_PlayerSlots)
		return std::vector<LPITEM>{nullptr, nullptr};
	return std::vector<LPITEM>{ITEM_MANAGER::instance().Find(m_PlayerSlots->pAcceMaterials[0].id), ITEM_MANAGER::instance().Find(m_PlayerSlots->pAcceMaterials[1].id)};
}

const TItemPosEx* CHARACTER::GetAcceMaterialsInfo()
{
	if (!m_PlayerSlots)
		return nullptr;
	return m_PlayerSlots->pAcceMaterials.data();
}

void CHARACTER::SetAcceMaterial(int pos, LPITEM ptr)
{
	if (!m_PlayerSlots)
		return;
	if (pos < 0 || pos >= ACCE_WINDOW_MAX_MATERIALS)
		return;
	if (!ptr)
		m_PlayerSlots->pAcceMaterials[pos] = {};
	else
	{
		m_PlayerSlots->pAcceMaterials[pos].id = ptr->GetID();
		m_PlayerSlots->pAcceMaterials[pos].pos.cell = ptr->GetCell();
		m_PlayerSlots->pAcceMaterials[pos].pos.window_type = ptr->GetWindow();
	}
}

void CHARACTER::OpenAcce(bool bCombination)
{
	if (IsAcceOpened(bCombination))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The acce window it's already opened."));
		return;
	}

	if (bCombination)
	{
		if (m_bAcceAbsorption)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Before you may close the acce absorption window."));
			return;
		}

		m_bAcceCombination = true;
	}
	else
	{
		if (m_bAcceCombination)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Before you may close the acce combine window."));
			return;
		}

		m_bAcceAbsorption = true;
	}

	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;

	TPacketAcce sPacket;
	sPacket.header = HEADER_GC_ACCE;
	sPacket.subheader = ACCE_SUBHEADER_GC_OPEN;
	sPacket.bWindow = bCombination;
	sPacket.dwPrice = 0;
	sPacket.bPos = 0;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = 0;
	sPacket.dwMinAbs = 0;
	sPacket.dwMaxAbs = 0;
	GetDesc()->Packet(sPacket);

	ClearAcceMaterials();
}

void CHARACTER::CloseAcce()
{
	if ((!m_bAcceCombination) && (!m_bAcceAbsorption))
		return;

	bool bWindow = m_bAcceCombination;

	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;

	TPacketAcce sPacket;
	sPacket.header = HEADER_GC_ACCE;
	sPacket.subheader = ACCE_SUBHEADER_GC_CLOSE;
	sPacket.bWindow = bWindow;
	sPacket.dwPrice = 0;
	sPacket.bPos = 0;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = 0;
	sPacket.dwMinAbs = 0;
	sPacket.dwMaxAbs = 0;
	GetDesc()->Packet(sPacket);

	if (bWindow)
		m_bAcceCombination = false;
	else
		m_bAcceAbsorption = false;

	ClearAcceMaterials();
}

void CHARACTER::ClearAcceMaterials()
{
	auto pkItemMaterial = GetAcceMaterials();
	for (int i = 0; i < ACCE_WINDOW_MAX_MATERIALS; ++i)
	{
		if (!pkItemMaterial[i])
			continue;

		pkItemMaterial[i]->Lock(false);
		pkItemMaterial[i] = NULL;
		SetAcceMaterial(i, nullptr);
	}
}

bool CHARACTER::AcceIsSameGrade(long lGrade)
{
	auto pkItemMaterial = GetAcceMaterials();
	if (!pkItemMaterial[0])
		return false;
	return pkItemMaterial[0]->GetValue(ACCE_GRADE_VALUE_FIELD) == lGrade;
}

DWORD CHARACTER::GetAcceCombinePrice(long lGrade)
{
	DWORD dwPrice = 0;
	switch (lGrade)
	{
	case 2:
	{
		dwPrice = ACCE_GRADE_2_PRICE;
	}
	break;
	case 3:
	{
		dwPrice = ACCE_GRADE_3_PRICE;
	}
	break;
	case 4:
	{
		dwPrice = ACCE_GRADE_4_PRICE;
	}
	break;
	default:
	{
		dwPrice = ACCE_GRADE_1_PRICE;
	}
	break;
	}

	return dwPrice;
}

BYTE CHARACTER::CheckEmptyMaterialSlot()
{
	auto pkItemMaterial = GetAcceMaterials();
	for (int i = 0; i < ACCE_WINDOW_MAX_MATERIALS; ++i)
	{
		if (!pkItemMaterial[i])
			return i;
	}

	return 255;
}

void CHARACTER::GetAcceCombineResult(DWORD & dwItemVnum, DWORD & dwMinAbs, DWORD & dwMaxAbs)
{
	auto pkItemMaterial = GetAcceMaterials();

	if (m_bAcceCombination)
	{
		if ((pkItemMaterial[0]) && (pkItemMaterial[1]))
		{
			long lVal = pkItemMaterial[0]->GetValue(ACCE_GRADE_VALUE_FIELD);
			if (lVal == 4)
			{
				dwItemVnum = pkItemMaterial[0]->GetOriginalVnum();
				dwMinAbs = pkItemMaterial[0]->GetSocket(ACCE_ABSORPTION_SOCKET);
				DWORD dwMaxAbsCalc = (dwMinAbs + ACCE_GRADE_4_ABS_RANGE > ACCE_GRADE_4_ABS_MAX ? ACCE_GRADE_4_ABS_MAX : (dwMinAbs + ACCE_GRADE_4_ABS_RANGE));
				dwMaxAbs = dwMaxAbsCalc;
			}
			else
			{
				DWORD dwMaskVnum = pkItemMaterial[0]->GetOriginalVnum();
				TItemTable * pTable = ITEM_MANAGER::instance().GetTable(dwMaskVnum + 1);
				if (pTable)
					dwMaskVnum += 1;

				dwItemVnum = dwMaskVnum;
				switch (lVal)
				{
				case 2:
				{
					dwMinAbs = ACCE_GRADE_3_ABS;
					dwMaxAbs = ACCE_GRADE_3_ABS;
				}
				break;
				case 3:
				{
					dwMinAbs = ACCE_GRADE_4_ABS_MIN;
					dwMaxAbs = ACCE_GRADE_4_ABS_MAX_COMB;
				}
				break;
				default:
				{
					dwMinAbs = ACCE_GRADE_2_ABS;
					dwMaxAbs = ACCE_GRADE_2_ABS;
				}
				break;
				}
			}
		}
		else
		{
			dwItemVnum = 0;
			dwMinAbs = 0;
			dwMaxAbs = 0;
		}
	}
	else
	{
		if ((pkItemMaterial[0]) && (pkItemMaterial[1]))
		{
			dwItemVnum = pkItemMaterial[0]->GetOriginalVnum();
			dwMinAbs = pkItemMaterial[0]->GetSocket(ACCE_ABSORPTION_SOCKET);
			dwMaxAbs = dwMinAbs;
		}
		else
		{
			dwItemVnum = 0;
			dwMinAbs = 0;
			dwMaxAbs = 0;
		}
	}
}

void CHARACTER::AddAcceMaterial(TItemPos tPos, BYTE bPos)
{
	if (bPos >= ACCE_WINDOW_MAX_MATERIALS)
	{
		if (bPos == 255)
		{
			bPos = CheckEmptyMaterialSlot();
			if (bPos >= ACCE_WINDOW_MAX_MATERIALS)
				return;
		}
		else
			return;
	}

	LPITEM pkItem = GetItem(tPos);
	if (!pkItem)
		return;
	else if ((pkItem->GetCell() >= INVENTORY_MAX_NUM) || (pkItem->IsEquipped()) || (tPos.IsBeltInventoryPosition()) || (pkItem->IsDragonSoul()))
		return;
	else if ((pkItem->GetType() != ITEM_COSTUME) && (m_bAcceCombination))
		return;
	else if ((pkItem->GetType() != ITEM_COSTUME) && (bPos == 0) && (m_bAcceAbsorption))
		return;
	else if (pkItem->isLocked())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't add locked items."));
		return;
	}
	#ifdef ENABLE_LOCKED_SYSTEM
	else if (pkItem->IsItemLocked())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't add locked items."));
		return;
	}
	#endif
	#ifdef __SOULBINDING_SYSTEM__
	else if ((pkItem->IsBind()) || (pkItem->IsUntilBind()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't add binded items."));
		return;
	}
	#endif
	else if ((m_bAcceCombination) && (bPos == 1) && (!AcceIsSameGrade(pkItem->GetValue(ACCE_GRADE_VALUE_FIELD))))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can combine just accees of same grade."));
		return;
	}
	else if ((m_bAcceCombination) && (pkItem->GetSocket(ACCE_ABSORPTION_SOCKET) >= ACCE_GRADE_4_ABS_MAX))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This acce got already maximum absorption chance."));
		return;
	}
	else if ((bPos == 1) && (m_bAcceAbsorption))
	{
		if ((pkItem->GetType() != ITEM_WEAPON) && (pkItem->GetType() != ITEM_ARMOR))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can absorb just the bonuses from armors and weapons."));
			return;
		}
		else if ((pkItem->GetType() == ITEM_ARMOR) && (pkItem->GetSubType() != ARMOR_BODY))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can absorb just the bonuses from armors and weapons."));
			return;
		}
	}
	else if ((pkItem->GetSubType() != COSTUME_ACCE) && (m_bAcceCombination))
		return;
	else if ((pkItem->GetSubType() != COSTUME_ACCE) && (bPos == 0) && (m_bAcceAbsorption))
		return;
	else if ((pkItem->GetSocket(ACCE_ABSORBED_SOCKET) > 0) && (bPos == 0) && (m_bAcceAbsorption))
		return;

	auto pkItemMaterial = GetAcceMaterials();
	if ((bPos == 1) && (!pkItemMaterial[0]))
		return;

	if (pkItemMaterial[bPos])
		return;

	SetAcceMaterial(bPos, pkItem);
	pkItemMaterial[bPos] = pkItem;
	pkItemMaterial[bPos]->Lock(true);

	DWORD dwItemVnum, dwMinAbs, dwMaxAbs;
	GetAcceCombineResult(dwItemVnum, dwMinAbs, dwMaxAbs);

	TPacketAcce sPacket;
	sPacket.header = HEADER_GC_ACCE;
	sPacket.subheader = ACCE_SUBHEADER_GC_ADDED;
	sPacket.bWindow = m_bAcceCombination;
	sPacket.dwPrice = GetAcceCombinePrice(pkItem->GetValue(ACCE_GRADE_VALUE_FIELD));
	sPacket.bPos = bPos;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = dwItemVnum;
	sPacket.dwMinAbs = dwMinAbs;
	sPacket.dwMaxAbs = dwMaxAbs;
	GetDesc()->Packet(sPacket);
}

void CHARACTER::RemoveAcceMaterial(BYTE bPos)
{
	if (bPos >= ACCE_WINDOW_MAX_MATERIALS)
		return;

	auto pkItemMaterial = GetAcceMaterials();

	DWORD dwPrice = 0;

	if (bPos == 1)
	{
		if (pkItemMaterial[bPos])
		{
			pkItemMaterial[bPos]->Lock(false);
			pkItemMaterial[bPos] = NULL;
			SetAcceMaterial(bPos, nullptr);
		}

		if (pkItemMaterial[0])
			dwPrice = GetAcceCombinePrice(pkItemMaterial[0]->GetValue(ACCE_GRADE_VALUE_FIELD));
	}
	else
		ClearAcceMaterials();

	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;

	TPacketAcce sPacket;
	sPacket.header = HEADER_GC_ACCE;
	sPacket.subheader = ACCE_SUBHEADER_GC_REMOVED;
	sPacket.bWindow = m_bAcceCombination;
	sPacket.dwPrice = dwPrice;
	sPacket.bPos = bPos;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = 0;
	sPacket.dwMinAbs = 0;
	sPacket.dwMaxAbs = 0;
	GetDesc()->Packet(sPacket);
}

BYTE CHARACTER::CanRefineAcceMaterials()
{
	BYTE bReturn = 0;
	if (!GetDesc())
		return bReturn;

	if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
		return bReturn;

	#ifdef ENABLE_IKASHOP_RENEWAL
	if (GetIkarusShopGuest() || GetIkarusAuctionGuest())
		return bReturn;
	#endif

	auto materialInfo = GetAcceMaterialsInfo();
	auto pkItemMaterial = GetAcceMaterials();
	if (!pkItemMaterial[0] || !pkItemMaterial[1])
	{
		sys_err("CanRefineAcceMaterials: pkItemMaterial null");
		return bReturn;
	}
	else if (pkItemMaterial[0]->GetOwner()!=this || pkItemMaterial[1]->GetOwner() != this)
	{
		sys_err("CanRefineAcceMaterials: pkItemMaterial different ownership");
		return bReturn;
	}
	else if (pkItemMaterial[0]->IsEquipped() || pkItemMaterial[1]->IsEquipped())
	{
		sys_err("CanRefineAcceMaterials: pkItemMaterial equipped");
		return bReturn;
	}
	else if (pkItemMaterial[0]->GetWindow() != INVENTORY || pkItemMaterial[1]->GetWindow() != INVENTORY)
	{
		sys_err("CanRefineAcceMaterials: pkItemMaterial not in INVENTORY");
		return bReturn;
	}
	else if (!materialInfo[0].id || !materialInfo[1].id)
	{
		sys_err("CanRefineAcceMaterials: materialInfo id 0");
		return bReturn;
	}
	else if (materialInfo[0].pos.cell != pkItemMaterial[0]->GetCell() || materialInfo[1].pos.cell != pkItemMaterial[1]->GetCell())
	{
		sys_err("CanRefineAcceMaterials: pkItemMaterial wrong cell");
		return bReturn;
	}
	else if (materialInfo[0].pos.window_type != pkItemMaterial[0]->GetWindow() || materialInfo[1].pos.window_type != pkItemMaterial[1]->GetWindow())
	{
		sys_err("CanRefineAcceMaterials: pkItemMaterial wrong window_type");
		return bReturn;
	}

	if (m_bAcceCombination)
	{
		if (!AcceIsSameGrade(pkItemMaterial[1]->GetValue(ACCE_GRADE_VALUE_FIELD)))
		{
			sys_err("CanRefineAcceMaterials: pkItemMaterial different acce grade");
			return bReturn;
		}

		for (int i = 0; i < ACCE_WINDOW_MAX_MATERIALS; ++i)
		{
			if (pkItemMaterial[i])
			{
				if ((pkItemMaterial[i]->GetType() == ITEM_COSTUME) && (pkItemMaterial[i]->GetSubType() == COSTUME_ACCE))
					bReturn = 1;
				else
				{
					bReturn = 0;
					break;
				}
			}
			else
			{
				bReturn = 0;
				break;
			}
		}
	}
	else if (m_bAcceAbsorption)
	{
		if ((pkItemMaterial[0]) && (pkItemMaterial[1]))
		{
			if ((pkItemMaterial[0]->GetType() == ITEM_COSTUME) && (pkItemMaterial[0]->GetSubType() == COSTUME_ACCE))
				bReturn = 2;
			else
				bReturn = 0;

			if ((pkItemMaterial[1]->GetType() == ITEM_WEAPON) || ((pkItemMaterial[1]->GetType() == ITEM_ARMOR) && (pkItemMaterial[1]->GetSubType() == ARMOR_BODY)))
				bReturn = 2;
			else
				bReturn = 0;

			if (pkItemMaterial[0]->GetSocket(ACCE_ABSORBED_SOCKET) > 0)
				bReturn = 0;
		}
		else
			bReturn = 0;
	}

	return bReturn;
}

void CHARACTER::RefineAcceMaterials()
{
	BYTE bCan = CanRefineAcceMaterials();
	if (bCan == 0)
		return;

	auto pkItemMaterial = GetAcceMaterials();

	DWORD dwItemVnum, dwMinAbs, dwMaxAbs;
	GetAcceCombineResult(dwItemVnum, dwMinAbs, dwMaxAbs);
	DWORD dwPrice = GetAcceCombinePrice(pkItemMaterial[0]->GetValue(ACCE_GRADE_VALUE_FIELD));

	if (bCan == 1)
	{
		int iSuccessChance = 0;
		long lVal = pkItemMaterial[0]->GetValue(ACCE_GRADE_VALUE_FIELD);
		switch (lVal)
		{
		case 2:
		{
			iSuccessChance = ACCE_COMBINE_GRADE_2;
		}
		break;
		case 3:
		{
			iSuccessChance = ACCE_COMBINE_GRADE_3;
		}
		break;
		case 4:
		{
			iSuccessChance = ACCE_COMBINE_GRADE_4;
		}
		break;
		default:
		{
			iSuccessChance = ACCE_COMBINE_GRADE_1;
		}
		break;
		}

		if (GetGold() < dwPrice)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough Yang."));
			return;
		}

		int iChance = number(1, 100);
		bool bSucces = (iChance <= iSuccessChance);
		if (bSucces)
		{
			LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(dwItemVnum, 1, 0, false);
			if (!pkItem)
			{
				sys_err("%d can't be created.", dwItemVnum);
				return;
			}

			ITEM_MANAGER::CopyAllAttrTo(pkItemMaterial[0], pkItem);
			LogManager::instance().ItemLog(this, pkItem, "COMBINE SUCCESS", pkItem->GetName());
			DWORD dwAbs = (dwMinAbs == dwMaxAbs ? dwMinAbs : number(dwMinAbs + 1, dwMaxAbs));
			pkItem->SetSocket(ACCE_ABSORPTION_SOCKET, dwAbs);
			pkItem->SetSocket(ACCE_ABSORBED_SOCKET, pkItemMaterial[0]->GetSocket(ACCE_ABSORBED_SOCKET));

			#ifdef ENABLE_FULL_YANG
			GoldChange(-dwPrice);
			#else
			PointChange(POINT_GOLD, -dwPrice);
			#endif

			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, pkItemMaterial[0]->GetVnum(), -dwPrice);

			WORD wCell = pkItemMaterial[0]->GetCell();
			ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[0], "COMBINE (REFINE SUCCESS)");
			ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[1], "COMBINE (REFINE SUCCESS)");

			pkItem->AddToCharacter(this, TItemPos(INVENTORY, wCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
			pkItem->AttrLog();

			if (lVal == 4)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("New absorption rate: %d%"), dwAbs);
			else
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Success."));

			EffectPacket(SE_EFFECT_ACCE_SUCCEDED);

			ClearAcceMaterials();
		}
		else
		{
			#ifdef ENABLE_FULL_YANG
			GoldChange(-dwPrice);
			#else
			PointChange(POINT_GOLD, -dwPrice);
			#endif

			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, pkItemMaterial[0]->GetVnum(), -dwPrice);

			ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[1], "COMBINE (REFINE FAIL)");

			if (lVal == 4)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("New absorption rate: %d%"), pkItemMaterial[0]->GetSocket(ACCE_ABSORPTION_SOCKET));
			else
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Failed."));


			pkItemMaterial[1] = NULL;
		}

		TItemPos tPos;
		tPos.window_type = INVENTORY;
		tPos.cell = 0;

		TPacketAcce sPacket;
		sPacket.header = HEADER_GC_ACCE;
		sPacket.subheader = ACCE_SUBHEADER_CG_REFINED;
		sPacket.bWindow = m_bAcceCombination;
		sPacket.dwPrice = dwPrice;
		sPacket.bPos = 0;
		sPacket.tPos = tPos;
		sPacket.dwItemVnum = 0;
		sPacket.dwMinAbs = 0;
		if (bSucces)
			sPacket.dwMaxAbs = 100;
		else
			sPacket.dwMaxAbs = 0;

		GetDesc()->Packet(sPacket);
	}
	else
	{
		pkItemMaterial[1]->CopyAttributeTo(pkItemMaterial[0]);
		LogManager::instance().ItemLog(this, pkItemMaterial[0], "ABSORB (REFINE SUCCESS)", pkItemMaterial[0]->GetName());
		pkItemMaterial[0]->SetSocket(ACCE_ABSORBED_SOCKET, pkItemMaterial[1]->GetOriginalVnum());
		#ifdef USE_ACCE_ABSORB_WITH_NO_NEGATIVE_BONUS
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			if (pkItemMaterial[0]->GetAttributeValue(i) < 0)
				pkItemMaterial[0]->SetForceAttribute(i, pkItemMaterial[0]->GetAttributeType(i), 0);
		}
		#endif
		ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[1], "ABSORBED (REFINE SUCCESS)");

		ITEM_MANAGER::instance().FlushDelayedSave(pkItemMaterial[0]);
		pkItemMaterial[0]->AttrLog();

		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Success."));
		EffectPacket(SE_EFFECT_ACCE_SUCCEDED);

		ClearAcceMaterials();

		TItemPos tPos;
		tPos.window_type = INVENTORY;
		tPos.cell = 0;

		TPacketAcce sPacket;
		sPacket.header = HEADER_GC_ACCE;
		sPacket.subheader = ACCE_SUBHEADER_CG_REFINED;
		sPacket.bWindow = m_bAcceCombination;
		sPacket.dwPrice = dwPrice;
		sPacket.bPos = 255;
		sPacket.tPos = tPos;
		sPacket.dwItemVnum = 0;
		sPacket.dwMinAbs = 0;
		sPacket.dwMaxAbs = 1;
		GetDesc()->Packet(sPacket);
	}
}

bool CHARACTER::CleanAcceAttr(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close the other windows."));
		return false;
	}
	else if ((!pkItem) || (!pkTarget))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pkItem or pkTarget nullptr."));
		return false;
	}
	else if ((pkTarget->GetType() != ITEM_COSTUME) && (pkTarget->GetSubType() != COSTUME_ACCE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't use it on non-sash items."));
		return false;
	}

	if (pkTarget->GetSocket(ACCE_ABSORBED_SOCKET) <= 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The sash item has no absorbed socket."));
		return false;
	}

	pkTarget->SetSocket(ACCE_ABSORBED_SOCKET, 0);
	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		pkTarget->SetForceAttribute(i, 0, 0);

	LogManager::instance().ItemLog(this, pkTarget, "USE_DETACHMENT (CLEAN ATTR)", pkTarget->GetName());
	return true;
}
#endif

#ifdef ENABLE_OTO_TOPLAMA
bool CHARACTER::IsBossNew()
{
	switch (GetRaceNum())
	{
		case 591:
		case 691:
		case 692:
		case 791:
		case 793:
		case 1091:
		case 1092:
		case 1093:
		case 1095:
		case 1096:
		case 1191:
		case 1192:
		case 1304:
		case 1306:
		case 1308:
		case 1309:
		case 1310:
		case 1334:
		case 1901:
		case 1902:
		case 1904:
		case 1905:
		case 1906:
		case 2091:
		case 2092:
		case 2093:
		case 2191:
		case 2206:
		case 2207:
		case 2306:
		case 2492:
		case 2493:
		case 2495:
		case 2598:
		case 2691:
		case 3090:
		case 3190:
		case 3290:
		case 3390:
		case 3490:
		case 3590:
		case 3595:
		case 3690:
		case 3790:
		case 3791:
		case 3890:
		case 3910:
		case 3911:
		case 3912:
		case 3913:
		case 5001:
		case 5002:
		case 5161:
		case 5162:
		case 5163:
		case 6091:
		case 6191:
		case 6116:
		case 6151:
		case 6207:
		case 6391:
		case 6393:
		case 6407:
		case 6408:
		case 6193:
			return true;
	}

	return false;
}
#endif

#ifdef ENABLE_CHANGE_CHANNEL_SYSTEM
bool CHARACTER::SwitchChannel(long newAddr, WORD newPort)
{
	if (!IsPC() || !GetDesc())
		return false;

	if (PreventTradeWindow(WND_FAST_CH, true))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return false;
	}

	long x = GetX();
	long y = GetY();

	long lAddr = newAddr;
	long lMapIndex = GetMapIndex();
	WORD wPort = newPort;

	// If we currently are in a dungeon.
	if (lMapIndex >= 10000)
	{
		sys_err("Invalid change channel request from dungeon %d!", lMapIndex);
		return false;
	}

	// If we are on CH99.
	if (g_bChannel == 99)
	{
		sys_err("%s attempted to change channel from CH99, ignoring req.", GetName());
		return false;
	}

	Stop();
	Save();

	if (GetSectree())
	{
		GetSectree()->RemoveEntity(this);
		ViewCleanup();
		EncodeRemovePacket(this);
	}

	m_lWarpMapIndex = lMapIndex;
	m_posWarp.x = x;
	m_posWarp.y = y;

	// TODO: This log message should mention channel we are changing to instead of port.
	sys_log(0, "ChangeChannel %s, %ld %ld map %ld to port %d", GetName(), x, y, GetMapIndex(), wPort);

	TPacketGCWarp p;

	p.bHeader = HEADER_GC_WARP;
	p.lX = x;
	p.lY = y;
	p.lAddr = lAddr;
	p.wPort = wPort;

	GetDesc()->Packet(&p, sizeof(p));

	char buf[256];
	// TODO: This log message should mention channel we are changing to instead of port
	snprintf(buf, sizeof(buf), "%s Port%d Map%ld x%ld y%ld", GetName(), wPort, GetMapIndex(), x, y);
	LogManager::instance().CharLog(this, 0, "CHANGE_CH", buf);

	return true;
}

EVENTINFO(switch_channel_info)
{
	DynamicCharacterPtr ch;
	int secs;
	long newAddr;
	WORD newPort;

	switch_channel_info()
		: ch(),
		secs(0),
		newAddr(0),
		newPort(0)
	{
	}
};

EVENTFUNC(switch_channel)
{
	switch_channel_info* info = dynamic_cast<switch_channel_info*>(event->info);
	if (!info)
	{
		sys_err("No switch channel event info!");
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (!ch)
	{
		sys_err("No char to work on for the switch.");
		return 0;
	}

	if (!ch->GetDesc())
		return 0;

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change channel while in a dungeon."));
		return 0;
	}

	if (ch->PreventTradeWindow(WND_FAST_CH, true))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return 0;
	}

	if (info->secs > 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Channel switch in %d seconds."), info->secs);
		--info->secs;
		return PASSES_PER_SEC(1);
	}

	ch->SwitchChannel(info->newAddr, info->newPort);
	ch->m_pkTimedEvent = NULL;
	return 0;
}

bool CHARACTER::StartChannelSwitch(long newAddr, WORD newPort)
{
	if (IsHack(false, true, TRADE_CHECK_TIME))
		return false;

	if (GetDungeon())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change channel while in a dungeon."));
		return false;
	}

	if (PreventTradeWindow(WND_ALL))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return false;
	}

	switch_channel_info* info = AllocEventInfo<switch_channel_info>();
	info->ch = this;
	// info->secs = (CanWarp() && !IsPosition(POS_FIGHTING)) ? 3 : 10;
	info->secs = 3;
	info->newAddr = newAddr;
	info->newPort = newPort;
	m_pkTimedEvent = event_create(switch_channel, info, 1);

	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Channel switch starting."));
	return true;
}
#endif

#ifdef ENABLE_MOVE_CHANNEL
bool CHARACTER::ChangeChannel(BYTE newChannel)
{
	if (!CanWarp() || IsDead())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to wait few seconds before changing channel."));
		return false;
	}

	if (newChannel == 99 || g_bChannel == 99)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change channel from this map."));
		return false;
	}

	if (!IsPC())
		return false;

	if (newChannel == g_bChannel)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are already in this channel. %d"), newChannel);
		return false;
	}

	long newMapIndex;
	long newAddr;
	WORD newPort;
	if (!CMapLocation::instance().Get(GetX(), GetY(), newMapIndex, newAddr, newPort, newChannel))
	{
		sys_err("cannot find map location index %d x %d y %d name %s", newMapIndex, GetX(), GetY(), GetName());
		return false;
	}

	long curMapIndex;
	long curAddr;
	WORD curPort;
	CMapLocation::instance().Get(GetX(), GetY(), curMapIndex, curAddr, curPort, g_bChannel);

	if (curMapIndex != newMapIndex)
	{
		const TMapRegion *rMapRgn = SECTREE_MANAGER::instance().GetMapRegion(newMapIndex);
		DESC_MANAGER::instance().SendClientPackageSDBToLoadMap(GetDesc(), rMapRgn->strMapName.c_str());
	}

	Stop();
	Save();

	if (GetSectree())
	{
		GetSectree()->RemoveEntity(this);
		ViewCleanup();
		EncodeRemovePacket(this);
	}

	m_lWarpMapIndex = GetMapIndex();
	m_posWarp.x = GetX();
	m_posWarp.y = GetY();

	sys_log(0, "ChangeChannel %s %d %d current map %d target map %d (%d %d)", GetName(), GetX(), GetY(), GetMapIndex(), GetMapIndex(), newAddr, newPort);

	TPacketGCWarp p{};
	p.bHeader	= HEADER_GC_WARP;
	p.lX		= GetX();
	p.lY		= GetY();
	p.lAddr		= newAddr;
	#ifdef ENABLE_NEWSTUFF
	if (!g_stProxyIP.empty())
		p.lAddr	= inet_addr(g_stProxyIP.c_str());
	#endif
	p.wPort		= newPort;
	GetDesc()->Packet(p);

	Stop();
	return true;
}
#endif

#ifdef __PET_SYSTEM__
bool CHARACTER::PetBlockMap()
{
	if (IsObserverMode())
		return true;

	if (GetWarMap())
		return true;

	switch (GetMapIndex())
	{
		case 103:
		case 113:
		#ifdef ENABLE_TANAKA_EVENT
		case TANAKA_EVENT_MAP_INDEX:
		#endif
		case 999:
			return true;
	}

	return false;
}
#endif

#ifdef ENABLE_KADIM_EFSUN
void CHARACTER::efsunudegis(int iEfsun, DWORD dwItemID)
{
	LPITEM item = ITEM_MANAGER::instance().Find(dwItemID);
	if (!item)
		return;

	item->SetForceAttribute(iEfsun, 0, 0);
	item->AddAttribute();
}
#endif

#ifdef ENABLE_HIDE_COSTUME_PART
void CHARACTER::UpdateCostumeVisibilityStatus()
{
	if (!IsPC())
		return;

	LPDESC d = GetDesc();

	if (d)
	{
		TPacketGCUpdateCostumeVisibilityStatus pack;
		pack.bHeader = HEADER_GC_UPDATE_COSTUME_VISIBILITY_STATUS;
		pack.type[ECostumePartTypes::HIDE_COSTUME_BODY] = IsBodyCostumeHidden();
		pack.type[ECostumePartTypes::HIDE_COSTUME_HAIR] = IsHairCostumeHidden();
		#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		pack.type[ECostumePartTypes::HIDE_COSTUME_WEAPON] = IsWeaponCostumeHidden();
		#endif
		d->Packet(&pack, sizeof(TPacketGCUpdateCostumeVisibilityStatus));
	}
}

void CHARACTER::SetBodyCostumeHidden(const bool hidden)
{
	m_bHideBodyCostume = hidden;
	SetQuestFlag("costume_option.hide_body", m_bHideBodyCostume ? 1 : 0);
}

void CHARACTER::SetHairCostumeHidden(const bool hidden)
{
	m_bHideHairCostume = hidden;
	SetQuestFlag("costume_option.hide_hair", m_bHideHairCostume ? 1 : 0);
}
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
void CHARACTER::SetWeaponCostumeHidden(const bool hidden)
{
	m_bHideWeaponCostume = hidden;
	SetQuestFlag("costume_option.hide_weapon", m_bHideWeaponCostume ? 1 : 0);
}
#endif
#endif



void CHARACTER::SetProtectTime(const std::string& flagname, int value)
{
	itertype(m_protection_Time) it = m_protection_Time.find(flagname);
	if (it != m_protection_Time.end())
		it->second = value;
	else
		m_protection_Time.insert(make_pair(flagname, value));
}

int CHARACTER::GetProtectTime(const std::string& flagname) const
{
	itertype(m_protection_Time) it = m_protection_Time.find(flagname);
	if (it != m_protection_Time.end())
		return it->second;
	return 0;
}

bool CHARACTER::IsOpenWindows() const
{
	if (IsOpenRefine())
		return true;

	#ifdef ENABLE_IKASHOP_RENEWAL
	if (GetIkarusShopGuest() || GetIkarusAuctionGuest())
		return true;
	#endif

	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if (IsAcceOpened())
		return true;
	#endif

	#ifdef ENABLE_CHANGELOOK_SYSTEM
	if (isChangeLookOpened())
		return true;
	#endif

	return false;
}

#ifdef __RANKING_SYSTEM__
void CHARACTER::SetRankManuel(BYTE rankIndex, long long value)
{
	switch (rankIndex)
	{
	case RANK_K_STONE:
		SetKillStone(GetKillStone()+value);
		break;
	case RANK_K_BOSS:
		SetKillBoss(GetKillBoss()+value);
		break;
	case RANK_D_STONE:
		SetDamageStone(value);
		break;
	case RANK_D_BOSS:
		SetDamageBoss(value);
		break;
	case RANK_DUNGEON:
		SetCompletedDungeon(GetCompletedDungeon()+value);
		break;
	case RANK_CHEST:
		SetOpenedChest(GetOpenedChest()+value);
		break;
	case RANK_D_SKILL:
		SetDamageSkill(value);
		break;
	case RANK_D_NORMAL:
		SetDamageNormal(value);
		break;
	}

	#ifdef ENABLE_REWARD_SYSTEM
	if (rankIndex == RANK_K_STONE)
	{
		CHARACTER_MANAGER::Instance().SetRewardData(REWARD_STONE_ALL_DEAD, GetName(), true, GetKillStone());
	}
	else if (rankIndex == RANK_K_BOSS)
	{
		CHARACTER_MANAGER::Instance().SetRewardData(REWARD_BOSS_ALL_DEAD, GetName(), true, GetKillBoss());
	}
	#endif

	char flag[45];
	sprintf(flag, "rank_%d", rankIndex);
	if (GetProtectTime(flag) == 1)
		SendRanks(RANK_SELF, RANK_TYPE_PLAYER, RANK_RACE_ALL, rankIndex);
}

void CHARACTER::GetRankManuel(BYTE categoryIndex, BYTE raceIndex, BYTE rankIndex, std::vector<TRanking>& list)
{
	TRanking p;
	p.empire = GetEmpire();
	p.index = 11;
	p.value = 0;
	strlcpy(p.name, GetName(), sizeof(p.name));

	if (categoryIndex == RANK_TYPE_PLAYER)
	{
		switch (rankIndex)
		{
			case RANK_LEVEL: 	p.value = GetLevel(); 					break;
			case RANK_K_STONE: 	p.value = GetKillStone(); 				break;
			case RANK_K_BOSS: 	p.value = GetKillBoss(); 				break;
			case RANK_D_STONE: 	p.value = GetDamageStone(); 			break;
			case RANK_D_BOSS: 	p.value = GetDamageBoss(); 				break;
			case RANK_DUNGEON: 	p.value = GetCompletedDungeon(); 		break;
			case RANK_PLAYTIME: p.value = GetRealPoint(POINT_PLAYTIME); break;
			case RANK_CHEST: 	p.value = GetOpenedChest(); 			break;
			case RANK_D_SKILL: 	p.value = GetDamageSkill(); 			break;
			case RANK_D_NORMAL: p.value = GetDamageNormal(); 			break;
 			default: 			return;
		}
	}
	else if ((categoryIndex == RANK_TYPE_DUNGEON) || (categoryIndex == RANK_TYPE_BOSS))
	{
		p.value = GetRankBoss(categoryIndex, rankIndex);
	}

	list.emplace_back(p);
}

void CHARACTER::SendRanks(BYTE subIndex, BYTE categoryIndex, BYTE raceIndex, BYTE rankIndex)
{
	if (!GetDesc()) { return; }

	TEMP_BUFFER buf;
	TPacketRank pack;
	pack.header = HEADER_GC_RANKING_UPDATE;

	char flag[45];
	sprintf(flag, "rank_%d", rankIndex);

	if (subIndex == RANK_LOAD)
	{
		//if (GetProtectTime(flag) == 1)
		//	return;

		SetProtectTime(flag, 1);

		std::vector<TRanking> rank_data;
		CHARACTER_MANAGER& chrMngr = CHARACTER_MANAGER::Instance();
		chrMngr.GetRankData(categoryIndex, raceIndex, rankIndex, rank_data);
		GetRankManuel(categoryIndex, raceIndex, rankIndex, rank_data);

		BYTE vec_size = rank_data.size();

		pack.size = (
			sizeof(pack)//pack
			+ sizeof(BYTE)//subIndex
			+ sizeof(BYTE)//categoryIndex
			+ sizeof(BYTE)//raceIndex
			+ sizeof(BYTE)//rankIndex
			+ sizeof(BYTE)//vec_size
			+ (sizeof(TRanking)*vec_size)//vec_data
		);

		buf.write(&pack, sizeof(TPacketRank));
		buf.write(&subIndex, sizeof(BYTE));
		buf.write(&categoryIndex, sizeof(BYTE));
		buf.write(&raceIndex, sizeof(BYTE));
		buf.write(&rankIndex, sizeof(BYTE));
		buf.write(&vec_size, sizeof(BYTE));
		if (vec_size > 0)
			buf.write(rank_data.data(), sizeof(TRanking)*vec_size);
	}
	else
	{
		if (GetProtectTime(flag) != 1)
			return;

		std::vector<TRanking> rank_data;
		GetRankManuel(categoryIndex, raceIndex, rankIndex, rank_data);

		if (rank_data.size() == 0)
			return;
		else if (rank_data[0].value == 0)
			return;

		pack.size = (
			sizeof(pack)//pack
			+sizeof(BYTE)//subIndex
			+sizeof(BYTE)//categoryIndex
			+sizeof(BYTE)//raceIndex
			+sizeof(BYTE)//rankIndex
			+sizeof(long long)//rank_data
		);

		buf.write(&pack, sizeof(TPacketRank));
		buf.write(&subIndex, sizeof(BYTE));
		buf.write(&categoryIndex, sizeof(BYTE));
		buf.write(&raceIndex, sizeof(BYTE));
		buf.write(&rankIndex, sizeof(BYTE));
		buf.write(&rank_data[0].value, sizeof(long long));
	}

	GetDesc()->Packet(buf.read_peek(), buf.size());
}

void CHARACTER::SetRankBoss(int iType, int iRank, int iKill)
{
	if ((iType < 0) || (iRank < 0)) { return; }
	auto iter = m_ranking_kill_boss.find({iType, iRank});
	if (iter != m_ranking_kill_boss.end())
	{
		if (iKill > 0)
		{
			iter->second += iKill;
		}
		else
		{
			iter->second -= abs(iKill);
		}
	}
	else
	{
		m_ranking_kill_boss.emplace(std::make_pair(iType, iRank), abs(iKill));
	}
}

int CHARACTER::GetRankBoss(int iType, int iRank)
{
	auto iter = m_ranking_kill_boss.find({iType, iRank});
	if (iter != m_ranking_kill_boss.end())
	{
		return iter->second;
	}
	return 0;
}

void CHARACTER::LoadRankBoss(DWORD dwCount, TPlayerRankingBoss* data)
{
	m_bIsLoadedRankBoss = false;
	for (int32_t i = 0; i < dwCount; ++i, ++data)
	{
		SetRankBoss(RANK_TYPE_DUNGEON, RANK_DUNGEON_LUCIFER, data->iLucifer);
		SetRankBoss(RANK_TYPE_DUNGEON, RANK_DUNGEON_CATACOMB, data->iAzrail);
		SetRankBoss(RANK_TYPE_DUNGEON, RANK_DUNGEON_BERAN_SETAOU, data->iBeranSetaou);
		SetRankBoss(RANK_TYPE_DUNGEON, RANK_DUNGEON_BARONES, data->iBarones);
		SetRankBoss(RANK_TYPE_DUNGEON, RANK_DUNGEON_RAZADOR, data->iRazador);
		SetRankBoss(RANK_TYPE_DUNGEON, RANK_DUNGEON_NEMERE, data->iNemere);
		//--------------------------------------------------------//
		SetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_ORK, data->iOrk);
		SetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_QUEEN_SPIDER, data->iQueenSpider);
		SetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_DARK_GHOST_LEADER, data->iDarkGhostLeader);
		SetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_GIANT_DESERT_TORTOISE, data->iGiantDesertTortoise);
		SetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_NINE_TAILS, data->iNineTails);
		SetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_FLAME_KING, data->iFlameKing);
		SetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_YELLOW_TIGER_GHOST, data->iYellowTigerGhost);
		SetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_ICE_WITCH, data->iIceWitch);
		SetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_GENERAL, data->iGeneral);
		SetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_KING_VOBA, data->iKingVoba);
	}
	m_bIsLoadedRankBoss = true;
}

void CHARACTER::SaveRankBoss(bool isDcSave)
{
	if (m_bIsLoadedRankBoss == false) { return; }

	if (!isDcSave)
	{
		if (GetProtectTime("saverankboss") > get_global_time()) { return; }
		SetProtectTime("saverankboss", get_global_time()+(60*3));
	}

	TPlayerRankingBoss pkTable{};
	pkTable.dwId = GetPlayerID();
	pkTable.dwAccountId = GetAID();
	strlcpy(pkTable.szName, GetName(), sizeof(pkTable.szName));
	pkTable.iJob = GetJob();
	//------------------------------------//
	pkTable.iLucifer = GetRankBoss(RANK_TYPE_DUNGEON, RANK_DUNGEON_LUCIFER);
	pkTable.iAzrail = GetRankBoss(RANK_TYPE_DUNGEON, RANK_DUNGEON_CATACOMB);
	pkTable.iBeranSetaou = GetRankBoss(RANK_TYPE_DUNGEON, RANK_DUNGEON_BERAN_SETAOU);
	pkTable.iBarones = GetRankBoss(RANK_TYPE_DUNGEON, RANK_DUNGEON_BARONES);
	pkTable.iRazador = GetRankBoss(RANK_TYPE_DUNGEON, RANK_DUNGEON_RAZADOR);
	pkTable.iNemere = GetRankBoss(RANK_TYPE_DUNGEON, RANK_DUNGEON_NEMERE);
	//------------------------------------//
	pkTable.iOrk = GetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_ORK);
	pkTable.iQueenSpider = GetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_QUEEN_SPIDER);
	pkTable.iDarkGhostLeader = GetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_DARK_GHOST_LEADER);
	pkTable.iGiantDesertTortoise = GetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_GIANT_DESERT_TORTOISE);
	pkTable.iNineTails = GetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_NINE_TAILS);
	pkTable.iFlameKing = GetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_FLAME_KING);
	pkTable.iYellowTigerGhost = GetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_YELLOW_TIGER_GHOST);
	pkTable.iIceWitch = GetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_ICE_WITCH);
	pkTable.iGeneral = GetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_GENERAL);
	pkTable.iKingVoba = GetRankBoss(RANK_TYPE_BOSS, RANK_BOSS_KING_VOBA);
	//------------------------------------//
	db_clientdesc->DBPacket(HEADER_GD_SAVE_RANK_BOSS, 0, &pkTable, sizeof(TPlayerRankingBoss));
}
#endif

#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
bool CHARACTER::UpdateCoin(DWORD dwAID, long val)
{
	if (val == 0)
	{
		sys_err("Value is empty");
		return false;
	}

	if (!dwAID)
	{
		sys_err("dwAID = 0, returned");
		return false;
	}

	char szQuery[256 + 1];
	long lValue = abs(val);

	if (val > 0)
	{
		sprintf(szQuery, "UPDATE account.account SET coins = coins + '%ld' WHERE id = '%u';", lValue, dwAID);
	}
	else
	{
		sprintf(szQuery, "UPDATE account.account SET coins = coins - '%ld' WHERE id = '%u';", lValue, dwAID);
	}

	auto msg(DBManager::instance().DirectQuery(szQuery));
	if (msg->uiSQLErrno != 0)
	{
		sys_err("UpdateCoin Query Failed, Error code: %ld", msg->uiSQLErrno);
		return false;
	}

	sys_log(0, "NAME: %s Coin Updated, Amount: %d", GetName(), val);
	return true;
}

DWORD CHARACTER::GetCoinNm()
{
	if (!GetDesc())
	{
		return 0;
	}

	auto msg(DBManager::instance().DirectQuery("SELECT coins FROM account.account WHERE id = '%u';", GetAID()));
	if (msg->uiSQLErrno != 0)
	{
		sys_err("GetCoinNm Query Failed, Error code: %ld", msg->uiSQLErrno);
		return 0;
	}

	if (!msg->Get()->uiNumRows)
	{
		sys_err("GetCoinNm Query Failed, Rows couldn't get");
		return 0;
	}

	MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);
	const char * cMyCoin = row[0];

	long lMyCoin = 0;
	str_to_number(lMyCoin, cMyCoin);
	return lMyCoin;
}

bool CHARACTER::UpdateCash(DWORD dwAID, long val)
{
	if (val == 0)
	{
		sys_err("Value is empty");
		return false;
	}

	if (!dwAID)
	{
		sys_err("dwAID = 0, returned");
		return false;
	}

	char szQuery[256 + 1];
	long lValue = abs(val);

	if (val > 0)
	{
		sprintf(szQuery, "UPDATE account.account SET cash = cash + '%ld' WHERE id = '%u';", lValue, dwAID);
	}
	else
	{
		sprintf(szQuery, "UPDATE account.account SET cash = cash - '%ld' WHERE id = '%u';", lValue, dwAID);
	}

	auto msg(DBManager::instance().DirectQuery(szQuery));
	if (msg->uiSQLErrno != 0)
	{
		sys_err("UpdateCash Query Failed, Error code: %ld", msg->uiSQLErrno);
		return false;
	}

	sys_log(0, "NAME: %s Cash Updated, Amount: %d", GetName(), val);
	return true;
}

DWORD CHARACTER::GetCashNm()
{
	if (!GetDesc())
	{
		return 0;
	}

	auto msg(DBManager::instance().DirectQuery("SELECT cash FROM account.account WHERE id = '%u';", GetAID()));
	if (msg->uiSQLErrno != 0)
	{
		sys_err("GetCashNm Query Failed, Error code: %ld", msg->uiSQLErrno);
		return 0;
	}

	if (!msg->Get()->uiNumRows)
	{
		sys_err("GetCashNm Query Failed, Rows couldn't get");
		return 0;
	}

	MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);
	const char * cMyCash = row[0];

	long lMyCash = 0;
	str_to_number(lMyCash, cMyCash);
	return lMyCash;
}

#ifdef ENABLE_DRAGON_COIN_EM_MARKET
bool CHARACTER::UpdateEm(DWORD dwAID, long val)
{
	if (val == 0)
	{
		sys_err("Value is empty");
		return false;
	}

	if (!dwAID)
	{
		sys_err("dwAID = 0, returned");
		return false;
	}

	char szQuery[256 + 1];
	long lValue = abs(val);

	if (val > 0)
	{
		sprintf(szQuery, "UPDATE account.account SET emark = emark + '%ld' WHERE id = '%u';", lValue, dwAID);
	}
	else
	{
		sprintf(szQuery, "UPDATE account.account SET emark = emark - '%ld' WHERE id = '%u';", lValue, dwAID);
	}

	auto msg(DBManager::instance().DirectQuery(szQuery));
	if (msg->uiSQLErrno != 0)
	{
		sys_err("UpdateCash Query Failed, Error code: %ld", msg->uiSQLErrno);
		return false;
	}

	sys_log(0, "NAME: %s EM Updated, Amount: %d", GetName(), val);
	return true;
}

DWORD CHARACTER::GetEmNm()
{
	if (!GetDesc())
	{
		return 0;
	}

	auto msg(DBManager::instance().DirectQuery("SELECT emark FROM account.account WHERE id = '%u';", GetAID()));
	if (msg->uiSQLErrno != 0)
	{
		sys_err("GetEmNm Query Failed, Error code: %ld", msg->uiSQLErrno);
		return 0;
	}

	if (!msg->Get()->uiNumRows)
	{
		sys_err("GetEmNm Query Failed, Rows couldn't get");
		return 0;
	}

	MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);
	const char * cMyEMark = row[0];

	long lMyEMark = 0;
	str_to_number(lMyEMark, cMyEMark);
	return lMyEMark;
}
#endif

EVENTFUNC(refresh_cash_and_coin_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("refresh_cash_and_coin_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (ch == NULL)
		return 0;

	ch->RefreshCoinAndCash(true);
	return 0;
}

void CHARACTER::RefreshCoinAndCash(bool isNow)
{
	if (isNow)
	{
		ChatPacket(CHAT_TYPE_COMMAND, "RefreshMyCoin %d", GetCoinNm());
		ChatPacket(CHAT_TYPE_COMMAND, "RefreshMyCash %d", GetCashNm());
		#ifdef ENABLE_DRAGON_COIN_EM_MARKET
		ChatPacket(CHAT_TYPE_COMMAND, "RefreshMyEm %d", GetEmNm());
		#endif
		return;
	}

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_pkRefreshCoinAndCash = event_create(refresh_cash_and_coin_event, info, PASSES_PER_SEC(1.2));
}
#endif

#ifdef ENABLE_BIYOLOG
void CHARACTER::CheckBio()
{
	int level = GetQuestFlag("bio.level");
	if (level == 0)
	{
		level += 1;
		SetQuestFlag("bio.level", level);
	}
	else if (level >= bio_max)
		return;

	if ((level == 1) && (GetLevel() >= 30) && (GetQuestFlag("bio.firstpm") == 0))
	{
		LPDESC desc = this->GetDesc();
		if (desc)
		{
			char buf[255];
			int len = snprintf(buf, sizeof(buf), "%s", LC_TEXT("biofirstpm"));

			TPacketGCWhisper pack;
			pack.bHeader = HEADER_GC_WHISPER;
			pack.bType = WHISPER_TYPE_SYSTEM;
			pack.wSize = sizeof(TPacketGCWhisper) + len;
			strlcpy(pack.szNameFrom, "[Biyolog]", sizeof(pack.szNameFrom));
			this->GetDesc()->BufferedPacket(&pack, sizeof(pack));
			this->GetDesc()->Packet(buf, len);
			SetQuestFlag("bio.firstpm", 1);
		}
	}

	int count = GetQuestFlag("bio.count");
	if (count >= bio_data[level][2])
	{
		if (count == bio_data[level][2])
		{
			if (bio_data[level][5] != 0)
				ChatPacket(CHAT_TYPE_COMMAND, "biostone %d", level);
			else
				if (bio_data[level][14] == 1)
					ChatPacket(CHAT_TYPE_COMMAND, "bioodul %d", level);
		}
		else
			if (bio_data[level][14] == 1)
				ChatPacket(CHAT_TYPE_COMMAND, "bioodul %d", level);
	}
	else
		ChatPacket(CHAT_TYPE_COMMAND, "biodata %d %d %d", level, count, GetQuestFlag("bio.time"));
}
#endif

#ifdef ENABLE_EXCHANGE_LOG
bool CHARACTER::LoadExchangeLogItem(DWORD logID)
{
	const auto it = m_mapExchangeLog.find(logID);
	if (it != m_mapExchangeLog.end())
	{
		if (!it->second.first.itemsLoaded)
		{
			it->second.second.clear();

			char szQuery[124];
			snprintf(szQuery, sizeof(szQuery), "SELECT * FROM log.exchange_log_items WHERE id = %u AND (pid = %u OR pid = %u)", logID, it->second.first.ownerPID, it->second.first.targetPID);
			auto pMsg(DBManager::instance().DirectQuery(szQuery));
			if (pMsg && pMsg->Get()->uiNumRows != 0)
			{
				MYSQL_ROW row;
				while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
				{
					DWORD itemOwnerPID;
					TExchangeLogItem logItem;
					BYTE col = 0;
					col++;//log_id
					str_to_number(itemOwnerPID, row[col++]);
					col++;//item_id
					str_to_number(logItem.pos, row[col++]);
					str_to_number(logItem.vnum, row[col++]);
					str_to_number(logItem.count, row[col++]);

					for (BYTE j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
						str_to_number(logItem.alSockets[j], row[col++]);

					for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
					{
						str_to_number(logItem.aAttr[j].bType, row[col++]);
						str_to_number(logItem.aAttr[j].sValue, row[col++]);
					}

					logItem.isOwnerItem = (it->second.first.ownerPID == itemOwnerPID);
					it->second.second.emplace_back(logItem);
				}
			}

			const WORD logItemCount = it->second.second.size();
			const BYTE subHeader = SUB_EXCHANGELOG_LOAD_ITEM;

			TPacketGCExchangeLog p;
			p.header = HEADER_GC_EXCHANGE_LOG;
			p.size = sizeof(TPacketGCExchangeLog) + sizeof(BYTE) + sizeof(WORD) + sizeof(DWORD) + (logItemCount*sizeof(TExchangeLogItem));

			TEMP_BUFFER buf;
			buf.write(&p, sizeof(TPacketGCExchangeLog));
			buf.write(&subHeader, sizeof(BYTE));
			buf.write(&logItemCount, sizeof(WORD));
			buf.write(&logID, sizeof(DWORD));
			if (logItemCount)
				buf.write(it->second.second.data(), logItemCount * sizeof(TExchangeLogItem));
			GetDesc()->Packet(buf.read_peek(), buf.size());

			return true;
		}
	}

	return false;
}

void CHARACTER::DeleteExchangeLog(DWORD logID)
{
	if (logID == 0)
	{
		for (auto it = m_mapExchangeLog.begin(); it != m_mapExchangeLog.end(); ++it)
		{
			char szQuery[124];
			snprintf(szQuery, sizeof(szQuery), "UPDATE log.exchange_log SET %s = 1 WHERE id = %u", it->second.first.ownerPID == GetPlayerID() ? "owner_delete" : "target_delete", it->first);
			DBManager::instance().DirectQuery(szQuery);
		}
		m_mapExchangeLog.clear();
	}
	else
	{
		auto it = m_mapExchangeLog.find(logID);
		if (it != m_mapExchangeLog.end())
		{
			char szQuery[124];
			snprintf(szQuery, sizeof(szQuery), "UPDATE log.exchange_log SET %s = 1 WHERE id = %u", it->second.first.ownerPID == GetPlayerID() ? "owner_delete" : "target_delete", logID);
			DBManager::instance().DirectQuery(szQuery);
			m_mapExchangeLog.erase(it);
		}
	}
}

void CHARACTER::LoadExchangeLog()
{
	if (GetProtectTime("ExchangeLogLoaded") == 1)
		return;

	m_mapExchangeLog.clear();

	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), 
		"SELECT * FROM log.exchange_log "
		"WHERE (owner_pid = %d AND owner_delete = 0) OR (target_pid = %d AND target_delete = 0)"
		, GetPlayerID()
		, GetPlayerID()
	);

	auto pMsg = DBManager::instance().DirectQuery(szQuery);
	if (pMsg && pMsg->Get() && pMsg->Get()->uiNumRows != 0)
	{
		std::vector<TExchangeLogItem> logItemVector;
		MYSQL_ROW row;
		while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
		{
			TExchangeLog exchangeLog;
			BYTE col = 0;
			DWORD logID;
			str_to_number(logID, row[col++]);
			strlcpy(exchangeLog.owner, row[col++], sizeof(exchangeLog.owner));
			str_to_number(exchangeLog.ownerPID, row[col++]);
			str_to_number(exchangeLog.ownerGold, row[col++]);
			strlcpy(exchangeLog.ownerIP, row[col++], sizeof(exchangeLog.ownerIP));

			strlcpy(exchangeLog.target, row[col++], sizeof(exchangeLog.target));
			str_to_number(exchangeLog.targetPID, row[col++]);
			str_to_number(exchangeLog.targetGold, row[col++]);
			strlcpy(exchangeLog.targetIP, row[col++], sizeof(exchangeLog.targetIP));
			strlcpy(exchangeLog.date, row[col++], sizeof(exchangeLog.date));
			col++;
			col++;
			exchangeLog.itemsLoaded = false;
			m_mapExchangeLog.emplace(logID, std::make_pair(exchangeLog, logItemVector));
		}
	}

	const WORD logCount = m_mapExchangeLog.size();
	const BYTE subHeader = SUB_EXCHANGELOG_LOAD;
	const bool isNeedClean = true;
	char playerCode[19];
	snprintf(playerCode, sizeof(playerCode), "%s", GetDesc()->GetAccountTable().social_id);

	TPacketGCExchangeLog p;
	p.header = HEADER_GC_EXCHANGE_LOG;
	p.size = sizeof(TPacketGCExchangeLog) + sizeof(BYTE) + sizeof(WORD) + (logCount*(sizeof(DWORD) + sizeof(TExchangeLog)));

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(TPacketGCExchangeLog));
	buf.write(&subHeader, sizeof(BYTE));
	buf.write(&playerCode, sizeof(playerCode));
	buf.write(&isNeedClean, sizeof(bool));
	buf.write(&logCount, sizeof(WORD));
	for (auto it = m_mapExchangeLog.begin(); it != m_mapExchangeLog.end(); ++it)
	{
		buf.write(&it->first, sizeof(DWORD));
		buf.write(&it->second.first, sizeof(TExchangeLog));
	}
	GetDesc()->Packet(buf.read_peek(), buf.size());

	SetProtectTime("ExchangeLogLoaded", 1);
}

void CHARACTER::SendExchangeLogPacket(BYTE subHeader, DWORD id, const TExchangeLog* exchangeLog)
{
	if (!GetDesc())
		return;

	if (SUB_EXCHANGELOG_LOAD_ITEM == subHeader)
		LoadExchangeLogItem(id);
	else if (SUB_EXCHANGELOG_LOAD == subHeader)
	{
		if (exchangeLog)
		{
			if (GetProtectTime("ExchangeLogLoaded") != 1)
				return;

			std::vector<TExchangeLogItem> logItemVector;
			TExchangeLog exchangeLogEx;
			thecore_memcpy(&exchangeLogEx, exchangeLog, sizeof(exchangeLogEx));
			m_mapExchangeLog.emplace(id, std::make_pair(exchangeLogEx, logItemVector));

			const WORD logCount = 1;
			const bool isNeedClean = false;
			char playerCode[19];
			snprintf(playerCode, sizeof(playerCode), "%s", GetDesc()->GetAccountTable().social_id);

			TPacketGCExchangeLog p;
			p.header = HEADER_GC_EXCHANGE_LOG;
			p.size = sizeof(TPacketGCExchangeLog) + sizeof(BYTE) + sizeof(WORD) + (logCount*(sizeof(DWORD) + sizeof(TExchangeLog)));

			TEMP_BUFFER buf;
			buf.write(&p, sizeof(TPacketGCExchangeLog));
			buf.write(&subHeader, sizeof(BYTE));
			buf.write(&playerCode, sizeof(playerCode));
			buf.write(&isNeedClean, sizeof(bool));
			buf.write(&logCount, sizeof(WORD));
			buf.write(&id, sizeof(DWORD));
			buf.write(&exchangeLogEx, sizeof(TExchangeLog));
			GetDesc()->Packet(buf.read_peek(), buf.size());
		}
		else
			LoadExchangeLog();
	}
}
#endif

#ifdef __ARE_YOU_IN_OR_NOT__
#include <ctime>
void CHARACTER::ClearAyirnGame()
{
	m_iAyirnGameID = -1;
	m_iAyirnGameStage = -1;
	m_iAyirnSelectedChest = -1;
	m_iAyirnLeftChestCount = 0;
	m_iAyirnOffer = 0;
	memset(m_iAyirnChestList, 0, sizeof(m_iAyirnChestList));
}
void shuffle_list(BYTE lst[24][2], int size) {
	std::srand(std::time(nullptr));
	int seed = std::rand();
	for (int i = size - 1; i > 0; i--) {
		int j = seed % (i + 1);
		std::swap(lst[i][0], lst[j][0]);
		std::swap(lst[i][1], lst[j][1]);
		seed = (seed + i + j) % static_cast<int>(std::pow(2, 32));
	}
}
void shuffle_list_ex(BYTE lst[24][2], int lst_size, std::vector<BYTE> exclude_indices) {
	std::srand(std::time(nullptr));
	int seed = std::rand();
	for (int i = lst_size - 1; i > 0; i--) {
		if (std::find(exclude_indices.begin(), exclude_indices.end(), i) == exclude_indices.end()) {
			int j = seed % (i + 1);
			while (std::find(exclude_indices.begin(), exclude_indices.end(), j) != exclude_indices.end()) {
				j = (j + 1) % (i + 1);
			}
			std::swap(lst[i][0], lst[j][0]);
			std::swap(lst[i][1], lst[j][1]);
			seed = (seed + i + j) % static_cast<int>(pow(2, 32));
		}
	}
}
BYTE CHARACTER::CalculateAyirnGameOffer()
{
	int openedCount = 0, openedVal = 0, totalVal = 0;
	for (BYTE j = 0; j < AYIRN_CHEST_COUNT; ++j)
	{
		if (m_iAyirnChestList[j][1])
		{
			openedCount += 1;
			openedVal += m_iAyirnChestList[j][0];
		}
		totalVal += m_iAyirnChestList[j][0];
	}
	int result = (totalVal - openedVal) / (AYIRN_CHEST_COUNT - openedCount);
	if (result < 0)
		result = 0;
	return (BYTE)result;
}
BYTE CHARACTER::GetAyirnLeftChestCount()
{
	BYTE count = 0;
	for (BYTE j = 0; j < AYIRN_CHEST_COUNT; ++j)
		if (m_iAyirnChestList[j][1] == 0)
			count += 1;
	return count;
}
void CHARACTER::CompareAyirnGameList(BYTE chestIdx)
{
	const BYTE playerLucky = 2;
	if (playerLucky > number(1, 100) || m_iAyirnChestList[chestIdx][0] >= 100 || GetAyirnLeftChestCount() <= 2)
		return;

	std::vector<BYTE> m_vecExpectList;
	for (BYTE j = 0; j < AYIRN_CHEST_COUNT; ++j)
	{
		if (m_iAyirnChestList[j][1])
			m_vecExpectList.emplace_back(j);
	}

	BYTE continueCount = 0;
	BYTE cacheList[24][2];
	while (true)
	{
		thecore_memcpy(cacheList, m_iAyirnChestList, sizeof(cacheList));
		shuffle_list_ex(cacheList, AYIRN_CHEST_COUNT, m_vecExpectList);
		if (cacheList[chestIdx][0] >= 70)
		{
			thecore_memcpy(m_iAyirnChestList, cacheList, sizeof(m_iAyirnChestList));
			return;
		}
		continueCount += 1;
		if (continueCount >= 50)
			break;
	}
}
void CHARACTER::ArgumentAyirnGame(const char* szCommand)
{
	if (quest::CQuestManager::instance().GetEventFlag("event_varmisin") == 0)
		return;
	std::vector<std::string> vecArgs;
	split_argument(szCommand, vecArgs);
	if (vecArgs.size() < 2)
		return;
	else if (vecArgs[1] == "open")
	{
		if (m_iAyirnGameID != -1)
		{
			ChatPacket(CHAT_TYPE_INFO, "<Game> Already active.");
			return;
		}
		
		if (GetDragonCoin() < 15)
		{
			ChatPacket(CHAT_TYPE_INFO, "You not has enought dragon coin");
			return;
		}
		SetDragonCoin(GetDragonCoin() - 15);
		ClearAyirnGame();

		m_iAyirnGameID = number(1, 1000);
		m_iAyirnGameStage = STAGE_SELECT_OWN;
		
		BYTE chestList[24][2] = { {5, 0}, {10, 0}, {15, 0}, {20, 0}, {25, 0}, {30, 0}, {35, 0}, {40, 0}, {45, 0}, {50, 0}, {55, 0}, {60, 0}, {70, 0}, {80, 0}, {90, 0}, {100, 0}, {120, 0}, {150, 0}, {160, 0}, {170, 0}, {180, 0}, {190, 0}, {200, 0}, {250, 0} };
		shuffle_list(chestList, AYIRN_CHEST_COUNT);
		memcpy(m_iAyirnChestList, chestList, sizeof(m_iAyirnChestList));

		ChatPacket(CHAT_TYPE_COMMAND, "AyirnPrepareEvent %d %d", m_iAyirnGameID, m_iAyirnGameStage);
	}
	else if (vecArgs[1] == "select_own")
	{
		int gameID, selectedBox;
		if (vecArgs.size() < 4)
			return; 
		else if (!str_to_number(gameID, vecArgs[2].c_str()) || !str_to_number(selectedBox, vecArgs[3].c_str()))
			return;
		else if (m_iAyirnSelectedChest != -1 || m_iAyirnGameStage != STAGE_SELECT_OWN || m_iAyirnGameID != gameID || selectedBox < 0 || selectedBox > 23)
			return;

		m_iAyirnSelectedChest = selectedBox;
		m_iAyirnLeftChestCount = 6;
		m_iAyirnGameStage = STAGE_SELECT_CHEST;

		ChatPacket(CHAT_TYPE_COMMAND, "AyirnSetSelectedChestIdx %d %d %d", selectedBox, m_iAyirnLeftChestCount, m_iAyirnGameStage);
	}
	else if (vecArgs[1] == "select")
	{
		int gameID, newChestIdx;
		if (vecArgs.size() < 4)
			return;
		else if (!str_to_number(gameID, vecArgs[2].c_str()) || !str_to_number(newChestIdx, vecArgs[3].c_str()))
			return;
		else if (newChestIdx < 0 || newChestIdx > 23)
			return;

		const BYTE leftCount = GetAyirnLeftChestCount();
		if (leftCount == 2)
		{
			if (m_iAyirnGameStage != STAGE_SELECT_LAST)
				return;
		}
		else
		{
			if (m_iAyirnGameStage != STAGE_SELECT_CHEST)
				return;
		}
		if (m_iAyirnChestList[newChestIdx][1] == 1)
			return;

		//ChatPacket(1, "");
		//for (BYTE j = 0; j < _countof(m_iAyirnChestList); ++j)
		//	ChatPacket(1, "j: %u -> %d - %d", j, m_iAyirnChestList[j][0], m_iAyirnChestList[j][1]);

		m_iAyirnLeftChestCount -= 1;

		if (leftCount > 2)
		{
			if (newChestIdx == m_iAyirnSelectedChest)
				return;
			CompareAyirnGameList(newChestIdx);
		}

		m_iAyirnChestList[newChestIdx][1] = 1;
		ChatPacket(CHAT_TYPE_COMMAND, "AyirnSetChestData %d %d %d %d", newChestIdx, m_iAyirnChestList[newChestIdx][0], m_iAyirnLeftChestCount, m_iAyirnGameStage);

		if (leftCount == 2)
		{
			SetDragonCoin(GetDragonCoin() + m_iAyirnChestList[newChestIdx][0]);
			
			ChatPacket(CHAT_TYPE_INFO, "You earn %d dragon coin.", m_iAyirnChestList[newChestIdx][0]);
			for (BYTE j = 0; j < AYIRN_CHEST_COUNT; ++j)
			{
				if (m_iAyirnChestList[j][1] == 0)
					ChatPacket(CHAT_TYPE_COMMAND, "AyirnSetChestData %d %d %d %d", j, m_iAyirnChestList[j][0], 0, -1);
			}
			ClearAyirnGame();
		}
		else
		{
			if (m_iAyirnLeftChestCount == 0)
			{
				m_iAyirnOffer = CalculateAyirnGameOffer();
				m_iAyirnGameStage = STAGE_SELECT_OFFER;
				ChatPacket(CHAT_TYPE_COMMAND, "AyirnSetBotOffer %d %u", m_iAyirnGameStage, m_iAyirnOffer);
			}
		}
	}
	else if (vecArgs[1] == "accept_offer")
	{
		int gameID;
		if (vecArgs.size() < 3)
			return;
		else if (!str_to_number(gameID, vecArgs[2].c_str()))
			return;
		else if (m_iAyirnGameStage != STAGE_SELECT_OFFER)
			return;
		else if (m_iAyirnOffer == 0)
			return;

		SetDragonCoin(GetDragonCoin() + m_iAyirnOffer);

		ChatPacket(CHAT_TYPE_INFO, "You earn %d dragon coin.", m_iAyirnOffer);
		for (BYTE j = 0; j < AYIRN_CHEST_COUNT; ++j)
		{
			if (m_iAyirnChestList[j][1] == 0)
				ChatPacket(CHAT_TYPE_COMMAND, "AyirnSetChestData %d %d %d %d", j, m_iAyirnChestList[j][0], 0, -1);
		}
		ClearAyirnGame();
		ChatPacket(CHAT_TYPE_COMMAND, "AyirnSetContinueEvent -1 0");
	}
	else if (vecArgs[1] == "decline_offer")
	{
		int gameID;
		if (vecArgs.size() < 3)
			return;
		else if (!str_to_number(gameID, vecArgs[2].c_str()))
			return;
		else if (m_iAyirnGameStage != STAGE_SELECT_OFFER)
			return;
		
		const BYTE leftCount = GetAyirnLeftChestCount();

		m_iAyirnLeftChestCount = (leftCount <= 6) ? 1 : 6;
		m_iAyirnGameStage = (leftCount == 2) ? STAGE_SELECT_LAST : STAGE_SELECT_CHEST;

		ChatPacket(CHAT_TYPE_COMMAND, "AyirnSetContinueEvent %d %u", m_iAyirnGameStage, m_iAyirnLeftChestCount);
	}
	else if (vecArgs[1] == "close")
	{
		if (m_iAyirnGameStage == -1 || m_iAyirnGameID == -1)
			return;
		ClearAyirnGame();
		ChatPacket(CHAT_TYPE_COMMAND, "AyirnSetContinueEvent -1 0");
	}
}
long long CHARACTER::GetDragonCoin()
{
	auto pMsg(DBManager::instance().DirectQuery("SELECT coins FROM account.account WHERE id = '%d';", GetDesc()->GetAccountTable().id));
	if (pMsg->Get()->uiNumRows == 0)
		return 0;

	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
	long long dc = 0;
	str_to_number(dc, row[0]);
	return dc;
}
void CHARACTER::SetDragonCoin(long long amount)
{
	DBManager::instance().DirectQuery("UPDATE account.account SET coins = '%lld' WHERE id = '%d';", amount, GetDesc()->GetAccountTable().id);
}
#endif

#ifdef __AUTO_SKILL_READER__
BYTE GetSkillGradeByLevel(BYTE bLevel)
{
	BYTE bGrade = SKILL_NORMAL;
	if (bLevel >= 40)
		bGrade = SKILL_PERFECT_MASTER;
	else if (bLevel >= 30)
		bGrade = SKILL_GRAND_MASTER;
	else if (bLevel >= 20)
		bGrade = SKILL_MASTER;

	return bGrade;
}

bool CHARACTER::SkillToBook(BYTE skillIdx, DWORD& itemIdx, DWORD& socket0, DWORD& exorcismIdx, DWORD& concentratedIdx)
{
	const BYTE skillLevel = GetSkillLevel(skillIdx);
	const BYTE skillGrade = GetSkillGradeByLevel(skillLevel);

	if (skillIdx <= 111)
	{
		if (skillGrade <= 2)
		{
			exorcismIdx = 71001;
			concentratedIdx = 76034;
		}

		if (skillGrade == 1)
		{
			itemIdx = USE_YMIR_50300_SKILLBOOK ? 50300 : 50400 + skillIdx;
			socket0 = skillIdx;
			return true;
		}
		else if (skillGrade == 2)
		{
			itemIdx = 50513;
			return true;
		}
	}
	else
	{
		const std::map<BYTE, std::pair<DWORD, DWORD>> m_mapExorcismandConcentrated = {
			//{SKILL_IDX, {exorcismIdx, exorcismIdx}},
			{SKILL_COMBO, {71001, 76034}},
			{SKILL_LEADERSHIP, {71001, 76034}},
			{SKILL_POLYMORPH, {71001, 76034}},
			{SKILL_MINING, {71001, 76034}},
		};

		const auto itNew = m_mapExorcismandConcentrated.find(skillIdx);
		if (itNew != m_mapExorcismandConcentrated.end())
		{
			exorcismIdx = itNew->second.first;
			concentratedIdx = itNew->second.second;
		}

		//if skill has 1 book put here
		const std::map<BYTE, DWORD> __skillBookItemList ={
			//{ SKILL_IDX, BOOK_ITEM_IDX },
			{ SKILL_COMBO, 50304 },
			{ SKILL_MINING, 50600 },
		};

		const auto it = __skillBookItemList.find(skillIdx);
		if (it != __skillBookItemList.end())
		{
			itemIdx = it->second;
			return true;
		}

		//#if skill different book for every stage(M | G | P)
		const std::map<BYTE, std::map<BYTE, DWORD>> __skillBookItemListEx = {
			{
				SKILL_LEADERSHIP, //skillIdx
				{
					//book via grade
					{SKILL_GRADE_MASTER, 50301},
					{SKILL_GRADE_GRAND_MASTER, 50302},
					{SKILL_GRADE_PERFECT_MASTER, 50303},
				}
			},
			{
				SKILL_POLYMORPH,//skillIdx
				{
					//book via grade
					{SKILL_GRADE_MASTER, 50314},
					{SKILL_GRADE_GRAND_MASTER, 50315},
					{SKILL_GRADE_PERFECT_MASTER, 50316},
				}
			},
		};

		const auto itEx = __skillBookItemListEx.find(skillIdx);
		if (itEx != __skillBookItemListEx.end())
		{
			const auto itExEx = itEx->second.find(skillGrade);
			if (itExEx != itEx->second.end())
			{
				itemIdx = itExEx->second;
				return true;
			}
		}
	}

	return false;
}

bool CHARACTER::ReadSkill(BYTE skillIdx)
{
	if (!GetSkillGroup() || IsPolymorphed())
		return false;

	const CSkillProto* pkSk = CSkillManager::instance().Get(skillIdx);
	if (!pkSk)
		return false;

	DWORD itemIdx = 0, exorcismIdx = 0, concentratedIdx = 0, socket0 = 0;
	if (!SkillToBook(skillIdx, itemIdx, socket0, exorcismIdx, concentratedIdx))
		return false;

	LPITEM book = NULL;

	if (itemIdx == 50300)
	{
		for (WORD i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			book = GetBookInventoryItem(i);
			if (!book || (book->GetVnum() != itemIdx) || (socket0 != 0 && socket0 != book->GetSocket(0)))
				continue;
			break;
		}
	}
	else
		book = FindSpecifyItem(itemIdx);

	if (!book)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Gerekli Beceri Kitabi yok!"));
		return false;
	}

	if (itemIdx == 50300)
	{
		if (socket0 != book->GetSocket(0))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Gerekli Beceri Kitabi yok!"));
			return false;
		}
	}

	const time_t now = get_global_time();
	const BYTE skillLevel = GetSkillLevel(skillIdx);
	// const BYTE skillGrade = GetSkillGradeByLevel(skillLevel);

	// SOUL_STONE
	if (itemIdx == 50513)
	{
		int needAlignment = (1000 + 500 * (skillLevel - 30));
		const int currentAlignment = (GetRealAlignment() / 10);
		int resultAlignment = (-19000 + needAlignment);
		int needZenBeanCount = 0;

		if (USE_DECREASE_ALIGNMENT_SOULSTONE)
		{
			if (currentAlignment < 0)
			{
				needAlignment *= 2;
				resultAlignment = (-19000 - needAlignment);
			}

			if (currentAlignment > resultAlignment)
			{
				needZenBeanCount = (needAlignment / 500);
				if (needZenBeanCount < 0)
					needZenBeanCount = 0 - needZenBeanCount;

				if (currentAlignment < 0)
				{
					if (!USE_ZEN_BEAN_SOULSTONE)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't do that with this alignment."));
						return false;
					}

					if (CountSpecifyItem(70102) < needZenBeanCount)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You not has enought zen bean."));
						return false;
					}
				}
			}
		}

		if (exorcismIdx > 0)
		{
			const TItemTable* itemTable = ITEM_MANAGER::Instance().GetTable(exorcismIdx);
			if (!itemTable || CountSpecifyItem(exorcismIdx) <= 0)
			{
				SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(skillIdx) - now);
				return false;
			}
			RemoveSpecifyItem(exorcismIdx);

			if (!FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
			{
				AddAffect(itemTable->alValues[0], aApplyInfo[itemTable->alValues[1]].bPointType, itemTable->alValues[2], 0, itemTable->alValues[3], 0, false);
			}
		}

		if (concentratedIdx > 0)
		{
			const TItemTable* itemTable = ITEM_MANAGER::Instance().GetTable(concentratedIdx);
			if (itemTable && CountSpecifyItem(concentratedIdx))
			{
				RemoveSpecifyItem(concentratedIdx);

				if (!FindAffect(AFFECT_SKILL_BOOK_BONUS))
				{
					AddAffect(itemTable->alValues[0], aApplyInfo[itemTable->alValues[1]].bPointType, itemTable->alValues[2], 0, itemTable->alValues[3], 0, false);
				}
			}
		}

		if ((currentAlignment < 0) && (needZenBeanCount > 0))
		{
			UpdateAlignment(MIN(-GetAlignment(), 500) * needZenBeanCount, true);
			RemoveSpecifyItem(70102, needZenBeanCount);
		}

		book->SetCount(book->GetCount() - 1);

		if (USE_DECREASE_ALIGNMENT_SOULSTONE)
			UpdateAlignment((needAlignment < 0) ? needAlignment * 10 : -needAlignment * 10, true);

		LearnGrandMasterSkill(skillIdx);
		SetSkillNextReadTime(skillIdx, USE_COOLTIME_ON_SOULSTONE ?  now + number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX) : 0);
		return true;
	}

	int iPct = 0;
	switch (skillIdx)
	{
		case SKILL_COMBO:
		{
			const int playerLevel = GetLevel();
			if (skillLevel == 0 && playerLevel < 30)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to have a minimum level of 30 to understand this book."));
				return false;
			}
			else if (skillLevel == 1 && playerLevel < 50)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need a minimum level of 50 to understand this book."));
				return false;
			}
			else if (skillLevel >= 2)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't train any more Combos."));
				return false;
			}
			iPct = book->GetValue(0);
		}
		break;

		case SKILL_LEADERSHIP:
		{
			if (skillLevel < book->GetValue(0))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("It isn't easy to understand this book."));
				return false;
			}
			else if (skillLevel >= book->GetValue(1))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This book will not help you."));
				return false;
			}
		}
		break;

		case SKILL_POLYMORPH:
		{
			if (GetLevel() < book->GetValue(3))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to improve your Level to read this Book."));
				return false;
			}
			else if (skillLevel >= 40)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill."));
				return false;
			}
			else if (skillLevel < book->GetValue(0))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("It isn't easy to understand this book."));
				return false;
			}
			else if (skillLevel >= book->GetValue(1))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train with this Book any more."));
				return false;
			}
			iPct = MINMAX(0, book->GetValue(2), 100);
		}
		break;

		case SKILL_MINING:
		{
			if (skillLevel >= 40)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill."));
				return false;
			}
			iPct = MINMAX(0, book->GetValue(1), 100);
		}
		break;
	}

	if (exorcismIdx > 0)
	{
		const TItemTable* itemTable = ITEM_MANAGER::Instance().GetTable(exorcismIdx);
		if (!itemTable || CountSpecifyItem(exorcismIdx) <= 0)
		{
			SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(skillIdx) - now);
			return false;
		}
		RemoveSpecifyItem(exorcismIdx);

		if (!FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
		{
			AddAffect(itemTable->alValues[0], aApplyInfo[itemTable->alValues[1]].bPointType, itemTable->alValues[2], 0, itemTable->alValues[3], 0, false);
		}
	}

	if (concentratedIdx > 0)
	{
		const TItemTable* itemTable = ITEM_MANAGER::Instance().GetTable(concentratedIdx);
		if (itemTable && CountSpecifyItem(concentratedIdx))
		{
			RemoveSpecifyItem(concentratedIdx);

			if (!FindAffect(AFFECT_SKILL_BOOK_BONUS))
			{
				AddAffect(itemTable->alValues[0], aApplyInfo[itemTable->alValues[1]].bPointType, itemTable->alValues[2], 0, itemTable->alValues[3], 0, false);
			}
		}
	}

	if (LearnSkillByBook(skillIdx, iPct))
	{
		book->SetCount(book->GetCount() - 1);
		SetSkillNextReadTime(skillIdx, USE_COOLTIME_ON_BOOKS ? now + number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX) : 0);
		if (GetSkillLevel(skillIdx) >= 30) { return false; }
		return true;
	}

	return false;
}

EVENTFUNC(auto_skill_read_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("auto_skill_read_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (ch == NULL)
		return 0;

	if (!ch->GetDesc())
		return 0;

	if (ch->PreventTradeWindow(WND_TIME, true))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return 0;
	}

	if (ch->ReadSkill(ch->GetSelectedSkillIndex()))
		return PASSES_PER_SEC(0.5);

	ch->GetAutoSkill(ch->GetSelectedSkillIndex(), false, true);
	return 0;
}

void CHARACTER::GetAutoSkill(BYTE skillIdx, bool status, bool isFromEvent)
{
	if (PreventTradeWindow(WND_TIME, true))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	const CSkillProto* pkSk = CSkillManager::instance().Get(skillIdx);
	if (!pkSk)
		return;

	if (status)
	{
		ChatPacket(CHAT_TYPE_COMMAND, "AutoSkillStatus 1");
		m_bSelectedSkillIdx = skillIdx;
		char_event_info* info = AllocEventInfo<char_event_info>();
		info->ch = this;
		m_pkAutoSkill = event_create(auto_skill_read_event, info, PASSES_PER_SEC(0.0));
	}
	else
	{
		m_bSelectedSkillIdx = 0;
		if (m_pkAutoSkill)
		{
			if (!isFromEvent)
				event_cancel(&m_pkAutoSkill);
			m_pkAutoSkill = NULL;
		}
		ChatPacket(CHAT_TYPE_COMMAND, "AutoSkillStatus 0");
	}
}
#endif

#ifdef __RENEWAL_BRAVE_CAPE__
struct FuncAggregateMonsterNew
{
	LPCHARACTER m_ch;
	int m_dwRange;
	FuncAggregateMonsterNew(LPCHARACTER ch, int range) : m_ch(ch), m_dwRange(range){}
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER)ent;
			if (!ch)
				return;

			if (!ch->IsMonster() || ch->GetVictim())
				return;

			if (DISTANCE_APPROX(ch->GetX() - m_ch->GetX(), ch->GetY() - m_ch->GetY()) < m_dwRange)
			{
				if (ch->CanBeginFight())
					ch->BeginFight(m_ch);
			}
		}
	}
};

EVENTFUNC(brave_cape_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("<brave_cape_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (ch == NULL)
		return 0;

	if (!ch->FindAffect(AFFECT_BRAVE_CAPE))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cesur etki yapilir, dongu olayi kapatilir."));
		ch->m_pkBraveCapeEvent = NULL;
		return 0;
	}

	LPSECTREE pSec = ch->GetSectree();
	if (pSec)
	{
		FuncAggregateMonsterNew f(ch, ch->GetProtectTime("brave_cape_range"));
		pSec->ForEachAround(f);
	}
	return PASSES_PER_SEC(ch->GetProtectTime("brave_cape_time"));
}

void CHARACTER::GetBraveCapeCMDCompare(const char* c_pszArgument)
{
	std::vector<std::string> vecArgs;
	split_argument(c_pszArgument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "active")
	{
		if (vecArgs.size() < 4) { return; }
		if (!FindAffect(AFFECT_BRAVE_CAPE))
		{
			ChatPacket(CHAT_TYPE_INFO, "<Sistem> Cesur pelerin etkiniz yok.");
			return;
		}

		if (m_pkBraveCapeEvent)
		{
			ChatPacket(CHAT_TYPE_INFO, "<Sistem> Zaten cesur pelerin aktif.");
			return;
		}

		const std::vector<WORD> m_dwBlockedMapIdx = { 1, 21, 41 };
		if (std::find(m_dwBlockedMapIdx.begin(), m_dwBlockedMapIdx.end(), GetMapIndex()) != m_dwBlockedMapIdx.end())
		{
			ChatPacket(CHAT_TYPE_INFO, "<Sistem> Bu haritada cesur pelerini kullanimi yasak.!");
			return;
		}

		int loopTime, eventRange;
		if (!str_to_number(loopTime, vecArgs[2].c_str()) || !str_to_number(eventRange, vecArgs[3].c_str()))
			return;

		if (loopTime < 1 || loopTime > 60 || eventRange < 1000 || eventRange > 15000)
			return;

		SetProtectTime("brave_cape_time", loopTime);
		SetProtectTime("brave_cape_range", eventRange);

		char_event_info* info = AllocEventInfo<char_event_info>();
		info->ch = this;
		m_pkBraveCapeEvent = event_create(brave_cape_event, info, 0);

		ChatPacket(CHAT_TYPE_COMMAND, "SetBraveCapeStatus 1");
		ChatPacket(CHAT_TYPE_INFO, "<Sistem> Cesur Pelerin etkisi aktif edildi..");

		#ifdef ENABLE_CAPTCHA_SELECT_SYSTEM
		SetCaptchaSkip(true);
		#endif
	}
	else if (vecArgs[1] == "deactive")
	{
		if (!m_pkBraveCapeEvent)
		{
			ChatPacket(CHAT_TYPE_INFO, "<Sistem> Cesur pelerin etkinliginiz yok!");
			return;
		}

		SetProtectTime("brave_cape_time", 0);
		SetProtectTime("brave_cape_range", 0);

		event_cancel(&m_pkBraveCapeEvent);
		m_pkBraveCapeEvent = NULL;

		ChatPacket(CHAT_TYPE_INFO, "<Sistem> Cesur Pelerin etkisi kapatildi..");
		ChatPacket(CHAT_TYPE_COMMAND, "SetBraveCapeStatus 0");

		#ifdef __AUTO_HUNT__
		if (!m_bAutoHuntStatus)
		#endif
		#ifdef ENABLE_CAPTCHA_SELECT_SYSTEM
		{
			SetCaptchaSkip(false);
		}
		#endif
	}
}
#endif

#ifdef ENABLE_BATTLE_PASS
void CHARACTER::LoadBattlePass(DWORD dwCount, TPlayerBattlePassMission* data)
{
	m_bIsLoadedBattlePass = false;
	for (int32_t i = 0; i < dwCount; ++i, ++data)
	{
		TPlayerBattlePassMission* newMission = new TPlayerBattlePassMission;
		newMission->dwPlayerId = data->dwPlayerId;
		newMission->dwMissionId = data->dwMissionId;
		newMission->dwBattlePassId = data->dwBattlePassId;
		newMission->dwExtraInfo = data->dwExtraInfo;
		newMission->bCompleted = data->bCompleted;
		newMission->bIsUpdated = data->bIsUpdated;
		m_listBattlePass.emplace_back(newMission);
	}
	m_bIsLoadedBattlePass = true;
}

DWORD CHARACTER::GetMissionProgress(uint8_t dwMissionID, DWORD dwBattlePassID)
{
	ListBattlePassMap::iterator it = m_listBattlePass.begin();
	while (it != m_listBattlePass.end())
	{
		TPlayerBattlePassMission* pkMission = *it++;
		if (pkMission->dwMissionId == dwMissionID && pkMission->dwBattlePassId == dwBattlePassID)
			return pkMission->dwExtraInfo;
	}
	return 0;
}

bool CHARACTER::IsCompletedMission(uint8_t bMissionID)
{
	ListBattlePassMap::iterator it = m_listBattlePass.begin();
	while (it != m_listBattlePass.end())
	{
		TPlayerBattlePassMission* pkMission = *it++;
		if (pkMission->dwMissionId == bMissionID)
			return (pkMission->bCompleted ? true : false);
	}
	return false;
}

bool CHARACTER::IsCompletedBattlePassID(uint8_t bMissionID, uint8_t bBattlePassID)
{
	ListBattlePassMap::iterator it = m_listBattlePass.begin();
	while (it != m_listBattlePass.end())
	{
		TPlayerBattlePassMission* pkMission = *it++;
		if (pkMission->dwMissionId == bMissionID && pkMission->dwBattlePassId == bBattlePassID)
			return (pkMission->bCompleted ? true : false);
	}
	return false;
}

void CHARACTER::ForceCompleteBPMission(uint8_t bMissionID, DWORD dwTotalValue)
{
	return;
}

void CHARACTER::UpdateMissionProgress(uint8_t dwMissionID, DWORD dwBattlePassID, DWORD dwUpdateValue, DWORD dwTotalValue, bool isOverride)
{
	if (!m_bIsLoadedBattlePass)
		return;

	bool foundMission = false;
	DWORD dwSaveProgress = 0;

	ListBattlePassMap::iterator it = m_listBattlePass.begin();
	while (it != m_listBattlePass.end())
	{
		TPlayerBattlePassMission* pkMission = *it++;

		if (pkMission->dwMissionId == dwMissionID && pkMission->dwBattlePassId == dwBattlePassID)
		{
			pkMission->bIsUpdated = 1;

			if (isOverride)
				pkMission->dwExtraInfo = dwUpdateValue;
			else
				pkMission->dwExtraInfo += dwUpdateValue;

			if (pkMission->dwExtraInfo >= dwTotalValue)
			{
				pkMission->dwExtraInfo = dwTotalValue;
				pkMission->bCompleted = 1;

				std::string stMissionName = CBattlePass::instance().GetMissionNameByType(pkMission->dwMissionId);
				std::string stBattlePassName = CBattlePass::instance().GetBattlePassNameByID(pkMission->dwBattlePassId);
				ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("Bulundugun gorevi basariyla tamamladin."));

				CBattlePass::instance().BattlePassRewardMission(this, dwMissionID, dwBattlePassID);
			}

			dwSaveProgress = pkMission->dwExtraInfo;
			foundMission = true;
			break;
		}
	}

	if (!foundMission)
	{
		TPlayerBattlePassMission* newMission = new TPlayerBattlePassMission;
		newMission->dwPlayerId = GetPlayerID();
		newMission->dwMissionId = dwMissionID;
		newMission->dwBattlePassId = dwBattlePassID;

		if (dwUpdateValue >= dwTotalValue)
		{
			newMission->dwExtraInfo = dwTotalValue;
			newMission->bCompleted = 1;

			CBattlePass::instance().BattlePassRewardMission(this, dwMissionID, dwBattlePassID);

			dwSaveProgress = dwTotalValue;
		}
		else
		{
			newMission->dwExtraInfo = dwUpdateValue;
			newMission->bCompleted = 0;

			dwSaveProgress = dwUpdateValue;
		}

		newMission->bIsUpdated = 1;

		m_listBattlePass.emplace_back(newMission);
	}

	m_bBattlePassSave = true;

	if (!GetDesc())
		return;

	TPacketGCBattlePassUpdate packet;
	packet.bHeader = HEADER_GC_BATTLE_PASS_UPDATE;
	packet.bMissionID = dwMissionID;
	packet.dwNewProgress = dwSaveProgress;
	GetDesc()->Packet(&packet, sizeof(TPacketGCBattlePassUpdate));
}

BYTE CHARACTER::GetBattlePassId()
{
	CAffect* pAffect = FindAffect(AFFECT_BATTLE_PASS, POINT_BATTLE_PASS_ID);
	if (!pAffect) return 0;
	return pAffect->lApplyValue;
}

void CHARACTER::ClearBattlePass()
{
	char queryStrBP[QUERY_MAX_LEN];
	snprintf(queryStrBP, sizeof(queryStrBP), "DELETE FROM player.battle_pass WHERE player_id = %d", GetPlayerID());
	DBManager::instance().DirectQuery(queryStrBP);
	m_listBattlePass.clear();
	m_bIsLoadedBattlePass = false;
}

void CHARACTER::SaveBattlePass()
{
	if (!m_bBattlePassSave) { return; }
	if (m_dwBattlePassTime > get_global_time()) { return; }

	ListBattlePassMap::iterator it = m_listBattlePass.begin();
	while (it != m_listBattlePass.end())
	{
		TPlayerBattlePassMission* pkMission = *it++;
		if (!pkMission->bIsUpdated) { continue; }
		db_clientdesc->DBPacket(HEADER_GD_SAVE_BATTLE_PASS, 0, pkMission, sizeof(TPlayerBattlePassMission));
	}

	DWORD dwSaveTime = (mother_port == 30007) ? 30 : (60*3);//Ch1Core1 30sn diger coreler 3dk
	m_dwBattlePassTime = (get_global_time()+dwSaveTime);
	m_bBattlePassSave = false;
}
#endif

#ifdef ENABLE_CAPTCHA_SELECT_SYSTEM
EVENTFUNC(captcha_quit_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("captcha_quit_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (ch == NULL) // <Factor>
	{
		return 0;
	}

	ch->m_pkCaptchaEvent = NULL;

	if (ch->GetDesc())
		ch->GetDesc()->DelayedDisconnect(2);
	return 0;
}

void CHARACTER::CaptchaOpen()
{
	/*
	std::vector<BYTE> NumList; NumList.clear();
	for (BYTE bNum = 0; bNum < 18; bNum++) { NumList.emplace_back(bNum+1); }
	if (NumList.empty()) return;
	std::shuffle(NumList.begin(), NumList.end(), std::random_device());
	*/

	std::set<BYTE> NumList;
	NumList.clear();

	do
	{
		BYTE bRandNum = number(1,18);
		if (NumList.find(bRandNum) != NumList.end())
			continue;

		NumList.emplace(bRandNum);
		if (NumList.size() >= 6)
			break;
	}
	while (true);

	int iCount = 0; int iChance = number(1,6);
	std::string cmd_order = ""; std::string cmd_image = "";
	char szCaptchaReplyName[10];
	for (auto iter : NumList)
	{
		if (iter == 0)
			return;

		iCount++;

		TCaptchaTable table = CaptchaDataList[iter];
		cmd_order += std::to_string(table.iOrder);
		cmd_image += std::to_string(table.iImageNum);
		if (iCount < 6) { cmd_order += "|"; cmd_image += "|"; }

		if (iChance == iCount)
		{
			m_iCaptchaReplyImage = table.iImageNum;
			strlcpy(szCaptchaReplyName, table.c_pszName, sizeof(szCaptchaReplyName));
		}
	}
	NumList.clear();

	ChatPacket(CHAT_TYPE_COMMAND, "CaptchaOpenWindow %s %s %s", cmd_order.c_str(), cmd_image.c_str(), szCaptchaReplyName);

	if (m_pkCaptchaEvent)
		event_cancel(&m_pkCaptchaEvent);

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_pkCaptchaEvent = event_create(captcha_quit_event, info, PASSES_PER_SEC(15));
}

void CHARACTER::CaptchaReply(int iReply)
{
	if (iReply == m_iCaptchaReplyImage)
	{
		SetQuestFlag("captchanew.status", 0);
		SetCaptchaAttackStatus(false);

		int iChance = number(25,35);
		int iTime = get_global_time()+(60*iChance);

		SetQuestFlag("captchanew.time", iTime);
		SetCaptchaTime(iTime);
		SetCaptchaFalseReplyCount(0);

		if (m_pkCaptchaEvent)
			event_cancel(&m_pkCaptchaEvent);

		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Captcha cevap dogru!"));
		return;
	}

	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Captcha cevap yanlis!"));

	if (GetCaptchaFalseReplyCount() >= 2)
	{
		SetCaptchaFalseReplyCount(0);
		if (GetDesc()) GetDesc()->DelayedDisconnect(2);
	}
	else
	{
		SetCaptchaFalseReplyCount(GetCaptchaFalseReplyCount()+1);

		if (m_pkCaptchaEvent)
			event_cancel(&m_pkCaptchaEvent);

		CaptchaOpen();
	}
}

bool CHARACTER::CaptchaQuestion(bool bLogin)
{
	if (quest::CQuestManager::instance().GetEventFlag("captcha_disable") == 1) { return false; }
	if (GetMapIndex() >= 10000) { return false; }
	if (m_bCaptchaSkip) { return false; }

	if (!bLogin)
	{
		if (get_global_time() > int(GetQuestFlag("captchanew.time")))
		{
			if (int(GetQuestFlag("captchanew.status")) == 0)
			{
				SetQuestFlag("captchanew.status", 1);
				SetCaptchaTime(int(GetQuestFlag("captchanew.time")));
				SetCaptchaAttackStatus(true);
				CaptchaOpen();
				return true;
			}
		}
	}

	if (int(GetQuestFlag("captchanew.status")) == 1)
	{
		if (bLogin)
		{
			SetCaptchaTime(int(GetQuestFlag("captchanew.time")));
			SetCaptchaAttackStatus(true);
			CaptchaOpen();
		}
		return true;
	}

	if (bLogin) { SetCaptchaTime(int(GetQuestFlag("captchanew.time"))); }

	return false;
}

bool CHARACTER::CaptchaQuestionDamage()
{
	if (quest::CQuestManager::instance().GetEventFlag("captcha_disable") == 1) { return false; }
	if (GetMapIndex() >= 10000) { return false; }
	if (m_bCaptchaSkip) { return false; }

	if (get_global_time() > GetCaptchaTime())
	{
		if (!GetCaptchaAttackStatus())
		{
			SetCaptchaAttackStatus(true);
			SetQuestFlag("captchanew.status", 1);
			CaptchaOpen();
		}
	}

	if (GetCaptchaAttackStatus())
	{
		return true;
	}

	return false;
}
#endif

#ifdef ENABLE_MULTI_HUNT_BLOCK
void CHARACTER::SetMultiHuntStatus(bool bValue)
{
	m_bmultiHuntStatus = bValue;

	if (!bValue)
	{
		GetAutoHuntCommand(" end");
	}
}
#endif

#ifdef __AUTO_HUNT__
bool CHARACTER::SetAutoHuntStatus(bool bStatus, bool bMobFarm, bool bMetinFarm)
{
	// Limited auto-hunt begin.
	if (g_autoHuntTimeLimit == 0)//auto hunt disabled
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Auto hunt disabled."));
		return false;
	}

	int remainingTime = INFINITE_AFFECT_DURATION;
	if (g_autoHuntTimeLimit > 0)//auto hunt limited
	{
		// Constants
		const std::string qfRemainingTime = "autohunt.remaining_time";
		const std::string qfLastResetDay = "autohunt.last_reset_day";
		
		// Get current day.
		time_t currentTime = time(NULL);
		struct tm* timeInfo = localtime(&currentTime);
		time_t currentDay = timeInfo->tm_yday + timeInfo->tm_year * 366;
		// Reset daily auto-hunt time.
		remainingTime = GetQuestFlag(qfRemainingTime);
		if (GetQuestFlag(qfLastResetDay) != currentDay)//Reset daily auto-hunt time.
		{
			remainingTime = g_autoHuntTimeLimit;
			SetQuestFlag(qfRemainingTime, g_autoHuntTimeLimit);
			SetQuestFlag(qfLastResetDay, currentDay);
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Auto-hunt time given."));
			m_dwAutoHuntStartTime = currentTime;
		}
		else if (!bStatus && m_bAutoHuntStatus && m_dwAutoHuntStartTime > 0)//Close and update remaining time.
		{
			int usedTime = currentTime - m_dwAutoHuntStartTime;
			remainingTime = std::max(0, remainingTime - usedTime);
			SetQuestFlag(qfRemainingTime, remainingTime);
			m_dwAutoHuntStartTime = 0;
		}
		else if (bStatus && !m_bAutoHuntStatus)//Open and update remaining time.
		{
			if (remainingTime <= 0)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Daily auto-hunt limit reached."));
				m_dwAutoHuntStartTime = 0;
				return false;
			}
			m_dwAutoHuntStartTime = currentTime;
		}
	}
	// Limited auto-hunt end.
	
	m_bAutoHuntStatus = bStatus;
	RemoveAffect(AFFECT_AUTO_HUNT_AFFECT);

	if (bStatus)
		AddAffect(AFFECT_AUTO_HUNT_AFFECT, POINT_NONE, 0, AFF_AUTO_HUNT, remainingTime, 0, false);

	ChatPacket(CHAT_TYPE_COMMAND, "AutoHuntStatus %d %d %d", bStatus, m_dwAutoHuntStartTime, remainingTime);
	return true;
}

void CHARACTER::GetAutoHuntCommand(const char* szArgument)
{
	std::vector<std::string> vecArgs;
	split_argument(szArgument, vecArgs);
	if (vecArgs.size() < 2)
	{
		return;
	}
	else if (vecArgs[1] == "end")
	{
		SetAutoHuntStatus(false);
		#ifdef ENABLE_CAPTCHA_SELECT_SYSTEM
		SetCaptchaSkip(false);
		#endif
	}
	else if (vecArgs[1] == "start")
	{
		if (vecArgs.size() < 7)
		{
			return;
		}

		if (!IsAutoHuntAffectHas())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have auto hunt affect."));
			return;
		}

		#ifdef ENABLE_MULTI_HUNT_BLOCK
		if (!GetMultiHuntStatus())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Tek client ile otoav kullanabilirsiniz!"));
			return;
		}
		#endif

		if (GetDungeon() || (GetMapIndex() >= 10000))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Zindanlarda oto-av kullanamazsiniz!"));
			return;
		}

		if (GetWarMap())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu mapta oto-av kullanamazsiniz!"));
			return;
		}

		if (!CheckAutoHuntMaps(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu mapta oto-av kullanamazsiniz!"));
			return;
		}

		#ifdef ENABLE_CAPTCHA_SELECT_SYSTEM
		if ((vecArgs[5] == "1") || (vecArgs[6] == "1"))
		{
			SetCaptchaSkip(true);
		}
		#endif

		if(!SetAutoHuntStatus(true, vecArgs[5] == "1", vecArgs[6] == "1"))
			return;

	}
	ChatPacket(CHAT_TYPE_COMMAND, "auto_hunt %s", szArgument);
}

bool CHARACTER::IsAutoHuntAffectHas()
{
	return FindAffect(AFFECT_AUTO_HUNT) != NULL;
}
#endif

#ifdef ENABLE_CHANGELOOK_SYSTEM
void CHARACTER::ChangeLookWindow(bool bOpen, bool bRequest)
{
	if ((bOpen) && (isChangeLookOpened()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] The window is already opened."));
		return;
	}

	if ((!bOpen) && (!isChangeLookOpened()))
	{
		if (!bRequest)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] The window is not opened."));
		return;
	}

	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;

	TPacketChangeLook sPacket;
	sPacket.header = HEADER_GC_CL;
	sPacket.subheader = CL_SUBHEADER_OPEN;
	sPacket.subheader = bOpen == true ? CL_SUBHEADER_OPEN : CL_SUBHEADER_CLOSE;
	#ifdef ENABLE_FULL_YANG
	sPacket.dwCost = (int64_t)(bOpen == true ? CL_TRANSMUTATION_PRICE : 0);
	#else
	sPacket.dwCost = bOpen == true ? CL_TRANSMUTATION_PRICE : 0;
	#endif
	sPacket.bPos = 0;
	sPacket.tPos = tPos;
	GetDesc()->Packet(&sPacket, sizeof(TPacketChangeLook));

	m_bChangeLook = bOpen;
	ClearClWindowMaterials();
}

void CHARACTER::ClearClWindowMaterials()
{
	LPITEM * pkItemMaterial;
	pkItemMaterial = GetClWindowMaterials();
	for (int i = 0; i < CL_WINDOW_MAX_MATERIALS; ++i)
	{
		if (!pkItemMaterial[i])
			continue;

		pkItemMaterial[i]->Lock(false);
		pkItemMaterial[i] = NULL;
	}
}

BYTE CHARACTER::CheckClEmptyMaterialSlot()
{
	LPITEM * pkItemMaterial;
	pkItemMaterial = GetClWindowMaterials();
	for (int i = 0; i < CL_WINDOW_MAX_MATERIALS; ++i)
	{
		if (!pkItemMaterial[i])
			return i;
	}

	return 255;
}

void CHARACTER::AddClMaterial(TItemPos tPos, BYTE bPos)
{
	if (!isChangeLookOpened())
		return;
	else if (bPos >= CL_WINDOW_MAX_MATERIALS)
	{
		if (bPos != 255)
			return;

		LPITEM pkScroll = GetItem(tPos);
		if (!pkScroll)
			return;

		if (pkScroll->GetVnum() == CL_SCROLL_VNUM)
			bPos = 2;
		else
			bPos = CheckClEmptyMaterialSlot();

		if (bPos >= CL_WINDOW_MAX_MATERIALS)
			return;
	}

	LPITEM pkItem = GetItem(tPos);
	if (!pkItem)
		return;

	if ((pkItem->GetCell() >= INVENTORY_MAX_NUM) || (tPos.IsBeltInventoryPosition()))
		return;

	if (pkItem->IsEquipped())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] You cannot transmute an item while it is equipped."));
		return;
	}

	#ifdef ENABLE_LOCKED_SYSTEM
	if (pkItem->IsItemLocked())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("item_engel"));
		return;
	}
	#endif

	if ((bPos == 2 && pkItem->GetVnum() != CL_SCROLL_VNUM) || (bPos != 2 && pkItem->GetVnum() == CL_SCROLL_VNUM))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] This item cannot be transmuted."));
		return;
	}

	if (pkItem->GetVnum() != CL_SCROLL_VNUM)
	{
		if ((pkItem->GetType() != ITEM_WEAPON) && (pkItem->GetType() != ITEM_ARMOR) && (pkItem->GetType() != ITEM_COSTUME) && (pkItem->GetType() != ITEM_PET))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] This item cannot be transmuted."));
			return;
		}
		else if ((pkItem->GetType() == ITEM_WEAPON) && ((pkItem->GetSubType() == WEAPON_ARROW) || (pkItem->GetSubType() == WEAPON_MOUNT_SPEAR)))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] This item cannot be transmuted."));
			return;
		}
		else if ((pkItem->GetType() == ITEM_ARMOR) && (pkItem->GetSubType() != ARMOR_BODY))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] This item cannot be transmuted."));
			return;
		}
		else if ((pkItem->GetType() == ITEM_COSTUME) && (pkItem->GetSubType() != COSTUME_BODY) && (pkItem->GetSubType() != COSTUME_HAIR) && (pkItem->GetSubType() != COSTUME_WEAPON))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] This item cannot be transmuted."));
			return;
		}
		// else if ((pkItem->GetType() == ITEM_PET) && (pkItem->GetSubType() != PET_MOUNT))
		// {
			// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] This item cannot be transmuted."));
			// return;
		// }
		// else if (pkItem->GetType() == ITEM_COSTUME && pkItem->GetSubType() == COSTUME_WEAPON)
		// {
			// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] You cannot submit this item."));
			// return;
		// }
		else if (pkItem->isLocked())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] You can't add locked items."));
			return;
		}
		else if (pkItem->GetTransmutation() != 0)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] You can't add items which are transmuted yet."));
			return;
		}
		#ifdef ENABLE_LOCKED_SYSTEM
		else if (pkItem->IsItemLocked())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] You can't add locked items."));
			return;
		}
		#endif
	}

	LPITEM * pkItemMaterial;
	pkItemMaterial = GetClWindowMaterials();
	if ((bPos == 1) && (!pkItemMaterial[0]))
		return;

	if (pkItemMaterial[bPos])
		return;

	if (bPos == 1)
	{
		bool bStop = false;

		if ((pkItem->GetType() != pkItemMaterial[0]->GetType()))
		{
			if ((pkItem->GetType() == ITEM_COSTUME && pkItem->GetSubType() == COSTUME_BODY) && (pkItemMaterial[0]->GetType() == ITEM_ARMOR && pkItemMaterial[0]->GetSubType() == ARMOR_BODY))
				bStop = false;
			else if ((pkItem->GetType() == ITEM_COSTUME && pkItem->GetSubType() == COSTUME_WEAPON) && (pkItemMaterial[0]->GetType() == ITEM_WEAPON))
				bStop = false;
			else if ((pkItem->GetType() == ITEM_ARMOR && pkItem->GetSubType() == ARMOR_BODY) && (pkItemMaterial[0]->GetType() == ITEM_COSTUME && pkItemMaterial[0]->GetSubType() == COSTUME_BODY))
				bStop = false;
			else if ((pkItem->GetType() == ITEM_WEAPON) && (pkItemMaterial[0]->GetType() == ITEM_COSTUME && pkItemMaterial[0]->GetSubType() == COSTUME_WEAPON))
				bStop = false;
			else
				bStop = true;
		}
		else
		{
			if (pkItemMaterial[0]->GetType() != pkItem->GetType())
				bStop = true;
			else if (pkItemMaterial[0]->GetSubType() != pkItem->GetSubType())
				bStop = true;
		}

		if (pkItemMaterial[0]->GetOriginalVnum() == pkItem->GetOriginalVnum())
			bStop = true;
		else if (((IS_SET(pkItemMaterial[0]->GetAntiFlag(), ITEM_ANTIFLAG_FEMALE)) && (!IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_FEMALE))) || ((IS_SET(pkItemMaterial[0]->GetAntiFlag(), ITEM_ANTIFLAG_MALE)) && (!IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_MALE))))
			bStop = true;
		else if ((pkItem->GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR) && (!IS_SET(pkItemMaterial[0]->GetAntiFlag(), ITEM_ANTIFLAG_WARRIOR)))
			bStop = true;
		else if ((pkItem->GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN) && (!IS_SET(pkItemMaterial[0]->GetAntiFlag(), ITEM_ANTIFLAG_ASSASSIN)))
			bStop = true;
		else if ((pkItem->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN) && (!IS_SET(pkItemMaterial[0]->GetAntiFlag(), ITEM_ANTIFLAG_SHAMAN)))
			bStop = true;
		else if ((pkItem->GetAntiFlag() & ITEM_ANTIFLAG_SURA) && (!IS_SET(pkItemMaterial[0]->GetAntiFlag(), ITEM_ANTIFLAG_SURA)))
			bStop = true;
		else if ((pkItem->GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN) && (!IS_SET(pkItemMaterial[0]->GetAntiFlag(), ITEM_ANTIFLAG_WOLFMAN)))
			bStop = true;
		else if ((pkItem->GetType() == ITEM_COSTUME && pkItemMaterial[0]->GetType() == ITEM_COSTUME) && ((pkItem->GetType() == ITEM_COSTUME && pkItem->GetSubType() == COSTUME_WEAPON) && (pkItemMaterial[0]->GetType() == ITEM_COSTUME && pkItemMaterial[0]->GetSubType() == COSTUME_WEAPON)))
		{
			if (pkItem->GetValue(3) != pkItemMaterial[0]->GetValue(3))
				bStop = true;

			if (pkItem->GetSize() != pkItemMaterial[0]->GetSize())
				bStop = true;
		}
		else if //Cift eli, Tek ele yansitma bugu fix.
		(
			((pkItem->GetType() == ITEM_COSTUME && pkItem->GetSubType() == COSTUME_WEAPON) && (pkItemMaterial[0]->GetType() == ITEM_WEAPON)) || 
			((pkItemMaterial[0]->GetType() == ITEM_COSTUME && pkItemMaterial[0]->GetSubType() == COSTUME_WEAPON) && (pkItem->GetType() == ITEM_WEAPON))
		)
		{
			if (pkItem->GetSize() != pkItemMaterial[0]->GetSize())
				bStop = true;

			if (!bStop)
			{
				BYTE bWeaponSubType = 0;
				BYTE bCostumeSubType = 0;

				if (pkItem->GetType() == ITEM_COSTUME)
				{
					bWeaponSubType = pkItemMaterial[0]->GetSubType();
					bCostumeSubType = pkItem->GetValue(3);
				}
				else
				{
					bWeaponSubType = pkItem->GetSubType();
					bCostumeSubType = pkItemMaterial[0]->GetValue(3);
				}

				switch (bWeaponSubType)
				{
					case WEAPON_SWORD:
						{
							if (bCostumeSubType != 0)
								bStop = true;
						}
						break;

					case WEAPON_DAGGER:
						{
							if (bCostumeSubType != 1)
								bStop = true;
						}
						break;

					case WEAPON_BOW:
						{
							if (bCostumeSubType != 2)
								bStop = true;
						}
						break;

					case WEAPON_TWO_HANDED:
						{
							if (bCostumeSubType != 3)
								bStop = true;
						}
						break;

					case WEAPON_BELL:
						{
							if (bCostumeSubType != 4)
								bStop = true;
						}
						break;

					case WEAPON_FAN:
						{
							if (bCostumeSubType != 5)
								bStop = true;
						}
						break;

					case WEAPON_CLAW:
						{
							if (bCostumeSubType != 8)
								bStop = true;
						}
						break;
				}
			}
		}
		///-------------PET_SYSTEM-------------///
		/*
		else if ((pkItem->GetType() == ITEM_PET && pkItem->GetSubType() != PET_MOUNT) ||
				 (pkItemMaterial[0]->GetType() == ITEM_PET && pkItemMaterial[0]->GetSubType() != PET_MOUNT)
		)
		{
			bStop = true;
		}
		else if ((pkItem->GetType() == ITEM_PET && pkItemMaterial[0]->GetType() != ITEM_PET) ||
				 (pkItem->GetType() != ITEM_PET && pkItemMaterial[0]->GetType() == ITEM_PET)
		)
		{
			bStop = true;
		}
		else if ((pkItem->GetSubType() == PET_MOUNT && pkItemMaterial[0]->GetSubType() != PET_MOUNT) ||
				 (pkItem->GetSubType() != PET_MOUNT && pkItemMaterial[0]->GetSubType() == PET_MOUNT)
		)
		{
			bStop = true;
		}
		*/
		///-------------PET_SYSTEM-------------///

		if (bStop)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] You cannot submit this item."));
			return;
		}
	}

	pkItemMaterial[bPos] = pkItem;
	pkItemMaterial[bPos]->Lock(true);

	TPacketChangeLook sPacket;
	sPacket.header = HEADER_GC_CL;
	sPacket.subheader = CL_SUBHEADER_ADD;
	sPacket.dwCost = 0;
	sPacket.bPos = bPos;
	sPacket.tPos = tPos;
	GetDesc()->Packet(&sPacket, sizeof(TPacketChangeLook));
}

void CHARACTER::RemoveClMaterial(BYTE bPos)
{
	if (bPos >= CL_WINDOW_MAX_MATERIALS)
		return;

	LPITEM * pkItemMaterial;
	pkItemMaterial = GetClWindowMaterials();

	if (!pkItemMaterial[bPos])
		return;

	if (bPos == 1 || bPos == 2)
	{
		pkItemMaterial[bPos]->Lock(false);
		pkItemMaterial[bPos] = NULL;
	}
	else
		ClearClWindowMaterials();

	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;

	TPacketChangeLook sPacket;
	sPacket.header = HEADER_GC_CL;
	sPacket.subheader = CL_SUBHEADER_REMOVE;
	sPacket.dwCost = 0;
	sPacket.bPos = bPos;
	sPacket.tPos = tPos;
	GetDesc()->Packet(&sPacket, sizeof(TPacketChangeLook));
}

void CHARACTER::RefineClMaterials()
{
	LPITEM * pkItemMaterial;
	pkItemMaterial = GetClWindowMaterials();
	if (!pkItemMaterial[0])
		return;
	else if (!pkItemMaterial[1])
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] Please submit the item you want to transmute."));
		return;
	}

	#ifdef ENABLE_FULL_YANG
	int64_t dwPrice = CL_TRANSMUTATION_PRICE;
	#else
	DWORD dwPrice = CL_TRANSMUTATION_PRICE;
	#endif

	bool isNeedGold = true;
	if (pkItemMaterial[2] && pkItemMaterial[2]->GetVnum() == CL_SCROLL_VNUM)
		isNeedGold = false;

	if (isNeedGold == true)
	{
		if (GetGold() < dwPrice)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Transmutation] You don't have enough Yang."));
			return;
		}
	}

	DWORD dwVnum = pkItemMaterial[1]->GetVnum();
	if (isNeedGold == false)
		ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[2], "SPECIFY ITEM TRANSMUTED");
	else
	{
		#ifdef ENABLE_FULL_YANG
		GoldChange(-dwPrice);
		#else
		PointChange(POINT_GOLD, -int(dwPrice));
		#endif
	}

	DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, pkItemMaterial[0]->GetVnum(), -dwPrice);
	ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[1], "TRANSMUTED (SUCCESSFULLY)");

	pkItemMaterial[0]->SetTransmutation(dwVnum, true);
	ClearClWindowMaterials();

	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;

	TPacketChangeLook sPacket;
	sPacket.header = HEADER_GC_CL;
	sPacket.subheader = CL_SUBHEADER_REFINE;
	sPacket.dwCost = 0;
	sPacket.bPos = 0;
	sPacket.tPos = tPos;
	GetDesc()->Packet(&sPacket, sizeof(TPacketChangeLook));
}

bool CHARACTER::CleanTransmutation(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
		return false;
	else if ((!pkItem) || (!pkTarget))
		return false;
	else if ((pkTarget->GetType() != ITEM_WEAPON)
			 && (pkTarget->GetType() != ITEM_ARMOR)
			 && (pkTarget->GetType() != ITEM_COSTUME)
			 // && ((pkTarget->GetType() != ITEM_PET) && (pkTarget->GetSubType() != PET_MOUNT))
	)
	{
		return false;
	}
	else if (pkTarget->isLocked())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't remove the transmute because item is locked."));
		return false;
	}
	#ifdef ENABLE_LOCKED_SYSTEM
	else if (pkTarget->IsItemLocked())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't remove the transmute because item is locked."));
		return false;
	}
	#endif

	if (pkTarget->GetTransmutation() == 0)
		return false;

	pkTarget->SetTransmutation(0);
	pkItem->SetCount(pkItem->GetCount() - 1);
	LogManager::instance().ItemLog(this, pkTarget, "USE_DETACHMENT (CLEAN TRANSMUTED)", pkTarget->GetName());
	return true;
}
#endif

#ifdef BOSS_DMG_RANKING
EVENTFUNC(send_ranking_packet)
{
	SendRankPacketEvent* info = dynamic_cast<SendRankPacketEvent*>(event->info);
	if (info == NULL)
	{
		sys_err("SendRankPacketEvent <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER bossCh = info->bossCh;
	if (bossCh == NULL) { return 0; }
	bossCh->SendRankingPacket();

	return PASSES_PER_SEC(10);
}

void CHARACTER::CreatePacketTimer()
{
	StopPacketTimer("CREATE_NEW");

	SendRankPacketEvent* info = AllocEventInfo<SendRankPacketEvent>();
	info->bossCh = this;
	packetTimer = event_create(send_ranking_packet, info, PASSES_PER_SEC(10));
}

void CHARACTER::StopPacketTimer(const char * szReason)
{
	if (packetTimer)
	{
		event_cancel(&packetTimer); 
		TPacketGCDamageRanking sendPacket;
		memset(&sendPacket, 0, sizeof(sendPacket));
		sendPacket.bHeader = HEADER_GC_BOSS_RANKING;
		sendPacket.fPercents[0] = -5.0f;
		PacketAround(&sendPacket, sizeof(TPacketGCDamageRanking));
	}
}
#endif

#ifdef ENABLE_ALIGNMENT_BONUS
void CHARACTER::UpdateAlignmentBonus(int iAlign)
{
	if (m_mapAlignmentList.empty()) { return; }
	BYTE aLevel = 0, align_level = GetAlignmentLevel();
	for (auto iter = m_mapAlignmentList.begin(); iter != m_mapAlignmentList.end(); iter++)
	{
		const TAlignmentTable & List = *iter;
		if (iAlign >= List.Point) { aLevel++; }
	}
	if (align_level == aLevel) { return; }
	SetAlignmentLevel(aLevel);
	ComputePoints();
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Tebrikler %u.level derece bonusu kazandiniz."), align_level);
}

void CHARACTER::CheckAlignmentBonus()
{
	BYTE align_level = GetAlignmentLevel();
	if (align_level <= 0) { return; }
	if (m_mapAlignmentList.empty()) { return; }
	for (auto iter = m_mapAlignmentList.begin(); iter != m_mapAlignmentList.end(); iter++)
	{
		const TAlignmentTable & List = *iter;
		if (List.Level == align_level)
		{
			for (BYTE x = 0; x < 3; x++)
			{
				if (List.Type[x] > 0 && List.Value[x] > 0)
				{
					PointChange(aApplyInfo[List.Type[x]].bPointType, List.Value[x]);
				}
			}
			return;
		}
	}
}
#endif

#ifdef ENABLE_PLAYER_TAG_SYSTEM
void CHARACTER::LoadPlayerTags()
{
	if (!IsPC()) { return; }
	if (SendPlayerTags.empty()) { return; }
	for (auto R : SendPlayerTags)
	{
		if (R.Pid == GetPlayerID())
		{
			switch (R.Tag)
			{
				case TAG_YOUTUBE: { m_afAffectFlag.Set(AFF_TAG_YOUTUBE); UpdatePacket(); } break;
				case TAG_TWITCH: { m_afAffectFlag.Set(AFF_TAG_TWITCH); UpdatePacket(); } break;
				case TAG_TIKTOK: { m_afAffectFlag.Set(AFF_TAG_TIKTOK); UpdatePacket(); } break;
				case TAG_KICK: { m_afAffectFlag.Set(AFF_TAG_KICK); UpdatePacket(); } break;
				case TAG_KING: { m_afAffectFlag.Set(AFF_TAG_KING); UpdatePacket(); } break;
			}
		}

		if ((R.Tag == TAG_KING_GUILD) && (GetGuild() != NULL))
		{
			if (R.Pid == GetGuild()->GetID())
			{
				m_afAffectFlag.Set(AFF_TAG_KING_GUILD); UpdatePacket();
			}
		}

		#ifdef ENABLE_RONARK_SYSTEM
		if ((R.Tag == TAG_KING_RONARK) && (GetGuild() != NULL))
		{
			if (R.Pid == GetGuild()->GetID())
			{
				m_afAffectFlag.Set(AFF_TAG_KING_RONARK); UpdatePacket();
			}
		}
		#endif
	}
}
#endif

#ifdef ENABLE_CHECK_SECURE_ZONE
void CHARACTER::CheckSecurityZone()
{
	if (GetMapIndex() == 43)
	{
		int iDist = gef_two_village_dist;
		if (iDist <= 0) { iDist = 500; }
		int iSecureDist = DISTANCE_APPROX(GetX() - 873100, GetY() - 242600);
		if (iSecureDist < iDist)
		{
			if (GetDamageSecureStatus()) { return; }
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Gardiyanin korumasi altindasin."));
			SetDamageSecureStatus(true);
			SetOldPkMode(GetPKMode());
			SetPKMode(PK_MODE_PROTECT);
		}
		else
		{
			if (!GetDamageSecureStatus()) { return; }
			SetDamageSecureStatus(false);
			SetPKMode(GetOldPkMode());
		}
	}
	/*else if (GetMapIndex() == 65)
	{
		int iDist = gef_devil_tower_dist;
		if (iDist <= 0) { iDist = 500; }
		int iSecureDist = DISTANCE_APPROX(GetX() - 589700, GetY() - 113300);
		if (iSecureDist < iDist)
		{
			if (GetDamageSecureStatus()) { return; }
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Seytan kulesi bekcisinin korumasi altindasin."));
			SetDamageSecureStatus(true);
			SetOldPkMode(GetPKMode());
			SetPKMode(PK_MODE_PROTECT);
		}
		else
		{
			if (!GetDamageSecureStatus()) { return; }
			SetDamageSecureStatus(false);
			SetPKMode(GetOldPkMode());
		}
	}*/
	else if (GetMapIndex() == 73)
	{
		int iDist = gef_blue_dragon_dist;
		if (iDist <= 0) { iDist = 500; }
		int iSecureDist = DISTANCE_APPROX(GetX() - 180185, GetY() - 1220693);
		if (iSecureDist < iDist)
		{
			if (GetDamageSecureStatus()) { return; }
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Sura ruhunun korumasi altindasin."));
			SetDamageSecureStatus(true);
			SetOldPkMode(GetPKMode());
			SetPKMode(PK_MODE_PROTECT);
		}
		else
		{
			if (!GetDamageSecureStatus()) { return; }
			SetDamageSecureStatus(false);
			SetPKMode(GetOldPkMode());
		}
	}
}

void CHARACTER::CheckSecurityZone2()
{
	return;

	switch (GetMapIndex())
	{
		case 67://Hayalet Orman
		case 68://Kizil Orman
		case 123://ValentineMap2020
			SetPKMode(PK_MODE_PROTECT);
			break;
	}
}
#endif

#ifdef NEW_MOB_TIMER
void CHARACTER::CheckSecurityMobRange()
{
	if (IsPC()) { return; }
	int iDist = gef_mob_secure_range;
	if (iDist <= 0) { return; }
	int iSecureDist = DISTANCE_APPROX(GetX() - GetSpawnX(), GetY() - GetSpawnY());
	if (iSecureDist > iDist)
	{
		SetVictim(NULL);
		Return();
		Show(GetMapIndex(), GetSpawnX(), GetSpawnY(), GetZ());
	}
}
#endif

bool CHARACTER::IsMyShopTime()
{
	if (thecore_pulse() - GetMyShopTime() < PASSES_PER_SEC(TRADE_CHECK_TIME))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ticaret yaptiktan sonra bir sure beklemelisiniz!"));
		return true;
	}
	return false;
}

bool CHARACTER::IsExchangeTime()
{
	if (thecore_pulse() - GetMyExchangeTime() < PASSES_PER_SEC(EXCHANGE_CHECK_TIME))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ticaret yaptiktan sonra bir sure beklemelisiniz!"));
		return true;
	}
	return false;
}

// PREVENT_TRADE_WINDOW
bool CHARACTER::PreventTradeWindow(int flags, bool except) const
{
	if (WND_ALL & flags)
	{
		flags = 0x0;
		except = true;
	}

	if ((except && !(WND_TIME & flags)) || (!except && (WND_TIME & flags)))
	{
		if (thecore_pulse() - GetMyShopTime() < PASSES_PER_SEC(TRADE_CHECK_TIME))
			return true;
	}

	if ((except && !(WND_FAST_CH & flags)) || (!except && (WND_FAST_CH & flags)))
	{
		if (m_pkTimedEvent)
			return true;
	}

	if ((except && !(WND_DEAD & flags)) || (!except && (WND_DEAD & flags)))
	{
		if (IsDead())
			return true;
	}

	if ((except && !(WND_EXCHANGE & flags)) || (!except && (WND_EXCHANGE & flags)))
	{
		if (GetExchange())
			return true;
	}

	if ((except && !(WND_SAFEBOX & flags)) || (!except && (WND_SAFEBOX & flags)))
	{
		if (GetSafebox() || IsOpenSafebox())
			return true;
	}

	if ((except && !(WND_MALLBOX & flags)) || (!except && (WND_MALLBOX & flags)))
	{
		if (IsOpenMall())
			return true;
	}

	if ((except && !(WND_SHOP & flags)) || (!except && (WND_SHOP & flags)))
	{
		if (GetShop())
			return true;
	}

	if ((except && !(WND_MYSHOP & flags)) || (!except && (WND_MYSHOP & flags)))
	{
		if (GetMyShop())
			return true;
	}

	if ((except && !(WND_SHOPOWNER & flags)) || (!except && (WND_SHOPOWNER & flags)))
	{
		if (GetShopOwner())
			return true;
	}

	if ((except && !(WND_CUBE & flags)) || (!except && (WND_CUBE & flags)))
	{
		if (IsCubeOpen())
			return true;
	}

	if ((except && !(WND_REFINE & flags)) || (!except && (WND_REFINE & flags)))
	{
		if (IsUnderRefine())
			return true;
	}

	#ifdef ENABLE_IKASHOP_RENEWAL
	if ((except && !(WND_OFFLINE_SHOP & flags)) || (!except && (WND_OFFLINE_SHOP & flags)))
	{
		if (GetIkarusShopGuest() || IsLookingShopOwner() || IsLookingIkarusShopOfferList() || IsLookingAuctionOwner())
			return true;
	}
	#endif

	#if defined(ENABLE_ACCE_COSTUME_SYSTEM)
	if ((except && !(WND_ACCE_COMBINE & flags)) || (!except && (WND_ACCE_COMBINE & flags)))
	{
		if (IsAcceOpened(true))
			return true;
	}

	if ((except && !(WND_ACCE_ABSORB & flags)) || (!except && (WND_ACCE_ABSORB & flags)))
	{
		if (IsAcceOpened(false))
			return true;
	}
	#endif

	#if defined(ENABLE_CHANGELOOK_SYSTEM)
	if ((except && !(WND_CHANGELOOK & flags)) || (!except && (WND_CHANGELOOK & flags)))
	{
		if (isChangeLookOpened())
			return true;
	}
	#endif

	#if defined(__BL_LUCKY_BOX__)
	if ((except && !(WND_LUCKY_BOX & flags)) || (!except && (WND_LUCKY_BOX & flags)))
	{
		if (IsLuckyBoxOpen())
			return true;
	}
	#endif

	return false;
}
// END_PREVENT_TRADE_WINDOW

#ifdef ENABLE_AUTO_ITEM_SELL_SYSTEM
void CHARACTER::SendAutoItemSell()
{
	if (!GetDesc()) { return; }
	if (m_autoSellList.empty()) { return; }
	TEMP_BUFFER buff;
	TPacketGCAutoItemSellData p{};
	p.header = HEADER_GC_AUTO_ITEM_SELL;
	p.size = sizeof(TPacketGCAutoItemSellData) + (sizeof(DWORD)*m_autoSellList.size());
	p.itemCount = m_autoSellList.size();
	p.status = GetAutoSellItem();
	buff.write(&p, sizeof(TPacketGCAutoItemSellData));
	for (const auto& iter : m_autoSellList) { buff.write(&iter, sizeof(DWORD)); }
	GetDesc()->Packet(buff.read_peek(), buff.size());
}

void CHARACTER::AddAutoItemSell(const DWORD vnum)
{
	if (!m_autoSellList.empty())
	{
		const auto& getArrElmSize = [&]() -> size_t
		{
			size_t count = 0;
			for (const auto& iter : m_autoSellList)
			{
				if (iter <= 0) { continue; }
				++count;
			}
			return count;
		};

		if (getArrElmSize() > AUTO_ITEM_SELL_MAX)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't add more automatic item process"));
			return;
		}
	}

	// check same item vnum
	auto it = m_autoSellList.find(vnum);
	if (it != m_autoSellList.end())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You already added this item"));
		return;
	}
	m_autoSellList.emplace(vnum);

	const TItemTable * pTable = ITEM_MANAGER::instance().GetTable(vnum);
	if (pTable) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s item added successfully"), pTable->szLocaleName); }
}

void CHARACTER::RemoveAutoItemSell(const DWORD vnum)
{
	if (m_autoSellList.empty()) { return; }
	auto it = m_autoSellList.find(vnum);
	if (it == m_autoSellList.end()) { return; }
	m_autoSellList.erase(vnum);
	const TItemTable * pTable = ITEM_MANAGER::instance().GetTable(vnum);
	if (pTable) { ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s item removed successfully"), pTable->szLocaleName); }
}

void CHARACTER::ResetAutoItemSell()
{
	m_autoSellList.clear();
}

void CHARACTER::InvSellAutoItemSell()
{
	int		i;
	LPITEM	item;

	for (i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetInventoryItem(i)))
		{
			if (IsAutoItemSell(item->GetVnum(), item->GetCount()))
			{
				ITEM_MANAGER::instance().RemoveItem(item);
			}
		}
	}

	#ifdef ENABLE_SPECIAL_STORAGE
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(UPGRADE_INVENTORY, i))))
		{
			if (IsAutoItemSell(item->GetVnum(), item->GetCount()))
			{
				ITEM_MANAGER::instance().RemoveItem(item);
			}
		}
	}

	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(BOOK_INVENTORY, i))))
		{
			if (IsAutoItemSell(item->GetVnum(), item->GetCount()))
			{
				ITEM_MANAGER::instance().RemoveItem(item);
			}
		}
	}

	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(STONE_INVENTORY, i))))
		{
			if (IsAutoItemSell(item->GetVnum(), item->GetCount()))
			{
				ITEM_MANAGER::instance().RemoveItem(item);
			}
		}
	}

	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(CHEST_INVENTORY, i))))
		{
			if (IsAutoItemSell(item->GetVnum(), item->GetCount()))
			{
				ITEM_MANAGER::instance().RemoveItem(item);
			}
		}
	}
	#endif
}

bool CHARACTER::IsAutoItemSell(
	const DWORD vnum
	#ifdef ENABLE_NEW_MAX_COUNT
	, const short count
	#else
	, const BYTE count
	#endif
	, bool isSell
)
{
	if (!GetAutoSellItem()) { return false; }
	if (m_autoSellList.empty()) { return false; }
	auto it = m_autoSellList.find(vnum);
	if (it != m_autoSellList.end())
	{
		if (isSell)
		{
			const TItemTable * pTable = ITEM_MANAGER::instance().GetTable(vnum);
			if (!pTable) { return false; }

			#ifdef ENABLE_FULL_YANG
			int64_t dwPrice = static_cast<int64_t>(pTable->dwShopBuyPrice * count);
			#else
			int dwPrice = static_cast<int>(pTable->dwShopBuyPrice * count);
			#endif

			const int64_t nTotalMoney = static_cast<int64_t>(GetGold()) + static_cast<int64_t>(dwPrice);
			#ifdef ENABLE_FULL_YANG
			if (GetAllowedGold() <= nTotalMoney)
			#else
			if (GOLD_MAX <= nTotalMoney)
			#endif
			{
				return false;
			}

			#ifdef ENABLE_FULL_YANG
			GoldChange(dwPrice);
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s[x%d] item %s yang sell successfully"), pTable->szLocaleName, count, formatWithDots(dwPrice).c_str());
			#else
			PointChange(POINT_GOLD, dwPrice, false);
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s[x%d] item %s yang sell successfully"), pTable->szLocaleName, count, formatWithDots(dwPrice).c_str());
			#endif
		}
		return true;
	}
	return false;
}
#endif
#ifdef SPIN_WHEEL
void CHARACTER::SendMetinstoneCount()
{
	if (!GetDesc())
		return;

	quest::PC* pPc = quest::CQuestManager::instance().GetPC(GetPlayerID());

	if (!pPc)
		return;

	auto pCount = pPc->GetFlag("spin_wheel.metinstone_count");

	ChatPacket(CHAT_TYPE_COMMAND, "SetPercentage %d", pCount);
}
#endif

