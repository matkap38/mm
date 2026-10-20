#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "desc_manager.h"
#include "mob_manager.h"
#include "party.h"
#include "regen.h"
#include "p2p.h"
#include "dungeon.h"
#include "db.h"
#include "packet.h"
#include "config.h"
#include "xmas_event.h"
#include "questmanager.h"
#include "questlua.h"
#include "locale_service.h"
#include "shutdown_manager.h"
#include "../../common/CommonDefines.h"

#ifdef ENABLE_RONARK_SYSTEM
#include "ronark_map.h"
#endif

#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
#include "GetEventFlagSystem.h"
#endif

#if (!defined(__GNUC__) || defined(__clang__)) && !defined(CXX11_ENABLED)
#	include <boost/bind/bind.hpp>

#elif defined(CXX11_ENABLED)
#	include <functional>
	template <typename T>
	decltype(std::bind(&T::second, std::placeholders::_1)) select2nd()
	{
		return std::bind(&T::second, std::placeholders::_1);
	}
#endif

CHARACTER_MANAGER::CHARACTER_MANAGER() :
	m_iVIDCount(0),
	m_pkChrSelectedStone(NULL),
	m_bUsePendingDestroy(false)
{
	RegisterRaceNum(xmas::MOB_XMAS_FIRWORK_SELLER_VNUM);
	RegisterRaceNum(xmas::MOB_SANTA_VNUM);
	RegisterRaceNum(xmas::MOB_XMAS_TREE_VNUM);

	m_iMobItemRate = 100;
	m_iMobDamageRate = 100;
	m_iMobGoldAmountRate = 100;
	m_iMobGoldDropRate = 100;
	m_iMobExpRate = 80;

	m_iMobItemRatePremium = 100;
	m_iMobGoldAmountRatePremium = 100;
	m_iMobGoldDropRatePremium = 100;
	m_iMobExpRatePremium = 80;

	m_iUserDamageRate = 100;
	m_iUserDamageRatePremium = 100;

	#ifdef __RANKING_SYSTEM__
	m_map_ranks.clear();
	#endif

	#ifdef ENABLE_EVENT_MANAGER
	m_eventData.clear();
	#endif

	#ifdef ENABLE_COSTUME_SET_BONUS
	m_vecCostumeSetBonus.clear();
	#endif

	#ifdef ENABLE_REWARD_SYSTEM
	m_rewardData.clear();
	m_rewardStoneList.clear();
	m_rewardBossList.clear();
	m_rewardLoadStatus = false;
	#endif
}

CHARACTER_MANAGER::~CHARACTER_MANAGER()
{
	Destroy();
}

void CHARACTER_MANAGER::Destroy()
{
	#ifdef ENABLE_EVENT_MANAGER
	m_eventData.clear();
	#endif

	#ifdef ENABLE_COSTUME_SET_BONUS
	m_vecCostumeSetBonus.clear();
	#endif

	#ifdef ENABLE_REWARD_SYSTEM
	m_rewardData.clear();
	m_rewardStoneList.clear();
	m_rewardBossList.clear();
	#endif

	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.begin();
	while (it != m_map_pkChrByVID.end())
	{
		LPCHARACTER ch = it->second;
		M2_DESTROY_CHARACTER(ch); // m_map_pkChrByVID is changed here
		it = m_map_pkChrByVID.begin();
	}
}

void CHARACTER_MANAGER::GracefulShutdown()
{
	NAME_MAP::iterator it = m_map_pkPCChr.begin();

	while (it != m_map_pkPCChr.end())
		(it++)->second->Disconnect("GracefulShutdown");
}

DWORD CHARACTER_MANAGER::AllocVID()
{
	++m_iVIDCount;
	return m_iVIDCount;
}

LPCHARACTER CHARACTER_MANAGER::CreateCharacter(const char * name, DWORD dwPID)
{
	DWORD dwVID = AllocVID();

	#ifdef M2_USE_POOL
	LPCHARACTER ch = pool_.Construct();
	#else
	LPCHARACTER ch = M2_NEW CHARACTER;
	#endif

	ch->Create(name, dwVID, dwPID ? true : false);

	m_map_pkChrByVID.emplace(dwVID, ch);

	if (dwPID)
	{
		char szName[CHARACTER_NAME_MAX_LEN + 1];
		str_lower(name, szName, sizeof(szName));

		m_map_pkPCChr.emplace(szName, ch);
		m_map_pkChrByPID.emplace(dwPID, ch);
	}

	return (ch);
}

#ifndef DEBUG_ALLOC
void CHARACTER_MANAGER::DestroyCharacter(LPCHARACTER ch)
#else
void CHARACTER_MANAGER::DestroyCharacter(LPCHARACTER ch, const char* file, size_t line)
#endif
{
	if (!ch)
		return;

	// <Factor> Check whether it has been already deleted or not.
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.find(ch->GetVID());
	if (it == m_map_pkChrByVID.end())
	{
		sys_err("[CHARACTER_MANAGER::DestroyCharacter] <Factor> %d not found", (long)(ch->GetVID()));
		return; // prevent duplicated destrunction
	}

	if (ch->IsNPC() && !ch->IsPet() && ch->GetRider() == NULL)
	{
		if (ch->GetDungeon())
		{
			ch->GetDungeon()->DeadCharacter(ch);
		}

		#ifdef BOSS_DMG_RANKING
		if (ch->IsSpecialBoss()) { ch->StopPacketTimer("BOSS_DESTROY"); }
		#endif
	}

	if (m_bUsePendingDestroy)
	{
		m_set_pkChrPendingDestroy.emplace(ch);
		return;
	}

	m_map_pkChrByVID.erase(it);

	if (true == ch->IsPC())
	{
		char szName[CHARACTER_NAME_MAX_LEN + 1];

		str_lower(ch->GetName(), szName, sizeof(szName));

		NAME_MAP::iterator it = m_map_pkPCChr.find(szName);

		if (m_map_pkPCChr.end() != it)
			m_map_pkPCChr.erase(it);
	}

	if (0 != ch->GetPlayerID())
	{
		itertype(m_map_pkChrByPID) it = m_map_pkChrByPID.find(ch->GetPlayerID());

		if (m_map_pkChrByPID.end() != it)
		{
			m_map_pkChrByPID.erase(it);
		}
	}

	UnregisterRaceNumMap(ch);

	RemoveFromStateList(ch);

#ifdef M2_USE_POOL
	pool_.Destroy(ch);
#else
	#ifndef DEBUG_ALLOC
	M2_DELETE(ch);
	#else
	M2_DELETE_EX(ch, file, line);
	#endif
#endif
}

LPCHARACTER CHARACTER_MANAGER::Find(DWORD dwVID)
{
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.find(dwVID);

	if (m_map_pkChrByVID.end() == it)
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && dwVID != (DWORD)found->GetVID()) {
		sys_err("[CHARACTER_MANAGER::Find] <Factor> %u != %u", dwVID, (DWORD)found->GetVID());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::Find(const VID & vid)
{
	LPCHARACTER tch = Find((DWORD) vid);

	if (!tch || tch->GetVID() != vid)
		return NULL;

	return tch;
}

LPCHARACTER CHARACTER_MANAGER::FindByPID(DWORD dwPID)
{
	itertype(m_map_pkChrByPID) it = m_map_pkChrByPID.find(dwPID);

	if (m_map_pkChrByPID.end() == it)
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && dwPID != found->GetPlayerID()) {
		sys_err("[CHARACTER_MANAGER::FindByPID] <Factor> %u != %u", dwPID, found->GetPlayerID());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::FindPC(const char * name)
{
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	str_lower(name, szName, sizeof(szName));
	NAME_MAP::iterator it = m_map_pkPCChr.find(szName);

	if (it == m_map_pkPCChr.end())
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && strncasecmp(szName, found->GetName(), CHARACTER_NAME_MAX_LEN) != 0) {
		sys_err("[CHARACTER_MANAGER::FindPC] <Factor> %s != %s", name, found->GetName());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::SpawnMobRandomPosition(DWORD dwVnum, long lMapIndex, bool is_aggressive)
{
	{
		if (dwVnum == 5001 && !quest::CQuestManager::instance().GetEventFlag("japan_regen"))
		{
			sys_log(1, "WAEGU[5001] regen disabled.");
			return NULL;
		}
	}

	{
		if (dwVnum == 5002 && !quest::CQuestManager::instance().GetEventFlag("newyear_mob"))
		{
			sys_log(1, "HAETAE (new-year-mob) [5002] regen disabled.");
			return NULL;
		}
	}

	{
		if (dwVnum == 5004 && !quest::CQuestManager::instance().GetEventFlag("independence_day"))
		{
			sys_log(1, "INDEPENDECE DAY [5004] regen disabled.");
			return NULL;
		}
	}

	const CMob * pkMob = CMobManager::instance().Get(dwVnum);
	if (!pkMob)
	{
		sys_err("no mob data for vnum %u", dwVnum);
		return NULL;
	}

	if (!map_allow_find(lMapIndex))
	{
		sys_err("not allowed map %u", lMapIndex);
		return NULL;
	}

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
	if (pkSectreeMap == NULL)
	{
		return NULL;
	}

	int i;
	long x, y;
	for (i=0; i<2000; i++)
	{
		x = number(1, (pkSectreeMap->m_setting.iWidth / 100)  - 1) * 100 + pkSectreeMap->m_setting.iBaseX;
		y = number(1, (pkSectreeMap->m_setting.iHeight / 100) - 1) * 100 + pkSectreeMap->m_setting.iBaseY;

		//LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);
		LPSECTREE tree = pkSectreeMap->Find(x, y);
		if (!tree)
			continue;

		DWORD dwAttr = tree->GetAttribute(x, y);

		if (IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT))
			continue;

		if (IS_SET(dwAttr, ATTR_BANPK))
			continue;

		break;
	}

	if (i == 2000)
	{
		sys_err("cannot find valid location");
		return NULL;
	}

	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);
	if (!sectree)
	{
		sys_log(0, "SpawnMobRandomPosition: cannot create monster at non-exist sectree %d x %d (map %d)", x, y, lMapIndex);
		return NULL;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pkMob->m_table.szLocaleName);
	if (!ch)
	{
		sys_log(0, "SpawnMobRandomPosition: cannot create new character");
		return NULL;
	}

	ch->SetProto(pkMob);

	// if mob is npc with no empire assigned, assign to empire of map
	if (pkMob->m_table.bType == CHAR_TYPE_NPC)
		if (ch->GetEmpire() == 0)
			ch->SetEmpire(SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex));

	ch->SetRotation(number(0, 360));
	if (is_aggressive) //@fixme195
		ch->SetAggressive();

	#ifdef ENABLE_REDUCED_ENTITY_VIEW
	if (m_pkChrSelectedStone) { ch->SetStoneInMob(true); }
	if (lMapIndex >= 10000) { ch->SetNoReducedEntity(true); }
	#endif

	if (!ch->Show(lMapIndex, x, y, 0, false))
	{
		M2_DESTROY_CHARACTER(ch);
		sys_err(0, "SpawnMobRandomPosition: cannot show monster");
		return NULL;
	}

	#ifdef ENABLE_ENTITY_PRELOADING
	//@Amun: or be specific, like if (ch->ispet, mount, whatever)
	if (!ch->IsPC() && !ch->IsGoto() && !ch->IsWarp() && !ch->IsDoor())
		SECTREE_MANAGER::Instance().ExtendPreloadedEntitiesMap(lMapIndex, pkMob->m_table.dwVnum);
	#endif

	char buf[512+1];
	long local_x = x - pkSectreeMap->m_setting.iBaseX;
	long local_y = y - pkSectreeMap->m_setting.iBaseY;
	snprintf(buf, sizeof(buf), "spawn %s[%d] random position at %ld %ld %ld %ld (time: %d)", ch->GetName(), dwVnum, x, y, local_x, local_y, get_global_time());

	if (test_server)
		SendNotice(buf);

	sys_log(0, buf);
	return (ch);
}

LPCHARACTER CHARACTER_MANAGER::SpawnMob(DWORD dwVnum, long lMapIndex, long x, long y, long z, bool bSpawnMotion, int iRot, bool bShow)
{
	const CMob * pkMob = CMobManager::instance().Get(dwVnum);
	if (!pkMob)
	{
		sys_err("SpawnMob: no mob data for vnum %u", dwVnum);
		return NULL;
	}

	if (!(pkMob->m_table.bType == CHAR_TYPE_NPC
		|| pkMob->m_table.bType == CHAR_TYPE_WARP
		|| pkMob->m_table.bType == CHAR_TYPE_GOTO
		#ifdef __PET_SYSTEM__
		|| pkMob->m_table.bType == CHAR_TYPE_PET
		#endif
		)
		|| mining::IsVeinOfOre (dwVnum))
	{
		LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);
		if (!tree)
		{
			sys_log(0, "no sectree for spawn at %d %d mobvnum %d mapindex %d", x, y, dwVnum, lMapIndex);
			return NULL;
		}

		DWORD dwAttr = tree->GetAttribute(x, y);

		bool is_set = false;

		if ( mining::IsVeinOfOre (dwVnum) ) is_set = IS_SET(dwAttr, ATTR_BLOCK);
		else is_set = IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT);

		if ( is_set )
		{
			// SPAWN_BLOCK_LOG
			static bool s_isLog=quest::CQuestManager::instance().GetEventFlag("spawn_block_log");
			static DWORD s_nextTime=get_global_time()+10000;

			DWORD curTime=get_global_time();

			if (curTime>s_nextTime)
			{
				s_nextTime=curTime;
				s_isLog=quest::CQuestManager::instance().GetEventFlag("spawn_block_log");

			}

			if (s_isLog)
				sys_log(0, "SpawnMob: BLOCKED position for spawn %s %u at %d %d (attr %u)", pkMob->m_table.szName, dwVnum, x, y, dwAttr);
			// END_OF_SPAWN_BLOCK_LOG
			return NULL;
		}

		if (IS_SET(dwAttr, ATTR_BANPK))
		{
			sys_log(0, "SpawnMob: BAN_PK position for mob spawn %s %u at %d %d", pkMob->m_table.szName, dwVnum, x, y);
			return NULL;
		}
	}

	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);
	if (!sectree)
	{
		sys_log(0, "SpawnMob: cannot create monster at non-exist sectree %d x %d (map %d)", x, y, lMapIndex);
		return NULL;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pkMob->m_table.szLocaleName);
	if (!ch)
	{
		sys_log(0, "SpawnMob: cannot create new character");
		return NULL;
	}

	if (iRot == -1)
		iRot = number(0, 360);

	ch->SetProto(pkMob);

	// if mob is npc with no empire assigned, assign to empire of map
	if (pkMob->m_table.bType == CHAR_TYPE_NPC)
		if (ch->GetEmpire() == 0)
			ch->SetEmpire(SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex));

	ch->SetRotation(iRot);

	#ifdef ENABLE_REDUCED_ENTITY_VIEW
	if (m_pkChrSelectedStone) { ch->SetStoneInMob(true); }
	if (lMapIndex >= 10000) { ch->SetNoReducedEntity(true); }
	#endif

	if (bShow && !ch->Show(lMapIndex, x, y, z, bSpawnMotion))
	{
		M2_DESTROY_CHARACTER(ch);
		sys_log(0, "SpawnMob: cannot show monster");
		return NULL;
	}

	#ifdef ENABLE_ENTITY_PRELOADING
	//@Amun: or be specific, like if (ch->ispet, mount, whatever)
	if (!ch->IsPC() && !ch->IsGoto() && !ch->IsWarp() && !ch->IsDoor())
		SECTREE_MANAGER::Instance().ExtendPreloadedEntitiesMap(lMapIndex, pkMob->m_table.dwVnum);
	#endif

	return (ch);
}

LPCHARACTER CHARACTER_MANAGER::SpawnMobRange(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, bool bIsException, bool bSpawnMotion, bool bAggressive)
{
	const CMob * pkMob = CMobManager::instance().Get(dwVnum);
	if (!pkMob)
		return NULL;

	if (pkMob->m_table.bType == CHAR_TYPE_STONE)
		bSpawnMotion = false;//@Fin01 metin spawn effekti kapatıldi

	int i = 16;

	while (i--)
	{
		int x = number(sx, ex);
		int y = number(sy, ey);
		/*
		   if (bIsException)
		   if (is_regen_exception(x, y))
		   continue;
		 */

		LPCHARACTER ch = SpawnMob(dwVnum, lMapIndex, x, y, 0, bSpawnMotion);
		if (ch)
		{
			sys_log(1, "MOB_SPAWN: %s(%d) %dx%d", ch->GetName(), (DWORD) ch->GetVID(), ch->GetX(), ch->GetY());
			if (bAggressive)
				ch->SetAggressive();
			return (ch);
		}
	}

	return NULL;
}

void CHARACTER_MANAGER::SelectStone(LPCHARACTER pkChr)
{
	m_pkChrSelectedStone = pkChr;
}

bool CHARACTER_MANAGER::SpawnMoveGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, int tx, int ty, LPREGEN pkRegen, bool bAggressive_)
{
	CMobGroup * pkGroup = CMobManager::Instance().GetGroup(dwVnum);

	if (!pkGroup)
	{
		sys_err("NOT_EXIST_GROUP_VNUM(%u) Map(%u) ", dwVnum, lMapIndex);
		return false;
	}

	LPCHARACTER pkChrMaster = NULL;
	LPPARTY pkParty = NULL;

	const std::vector<DWORD> & c_rdwMembers = pkGroup->GetMemberVector();

	bool bSpawnedByStone = false;
	bool bAggressive = bAggressive_;

	if (m_pkChrSelectedStone)
	{
		bSpawnedByStone = true;
		if (m_pkChrSelectedStone->GetDungeon())
			bAggressive = true;
	}

	for (DWORD i = 0; i < c_rdwMembers.size(); ++i)
	{
		LPCHARACTER tch = SpawnMobRange(c_rdwMembers[i], lMapIndex, sx, sy, ex, ey, true, bSpawnedByStone);
		if (!tch)
		{
			if (i == 0)
				return false;
			continue;
		}

		sx = tch->GetX() - number(300, 500);
		sy = tch->GetY() - number(300, 500);
		ex = tch->GetX() + number(300, 500);
		ey = tch->GetY() + number(300, 500);

		if (m_pkChrSelectedStone)
			tch->SetStone(m_pkChrSelectedStone);
		else if (pkParty)
		{
			pkParty->Join(tch->GetVID());
			pkParty->Link(tch);
		}
		else if (!pkChrMaster)
		{
			pkChrMaster = tch;
			pkChrMaster->SetRegen(pkRegen);

			pkParty = CPartyManager::instance().CreateParty(pkChrMaster);
		}
		if (bAggressive)
			tch->SetAggressive();

		if (tch->Goto(tx, ty))
			tch->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	}

	return true;
}

LPCHARACTER CHARACTER_MANAGER::SpawnGroupGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen, bool bAggressive_, LPDUNGEON pDungeon)
{
	const DWORD dwGroupID = CMobManager::Instance().GetGroupFromGroupGroup(dwVnum);
	if (dwGroupID != 0)
	{
		return SpawnGroup(dwGroupID, lMapIndex, sx, sy, ex, ey, pkRegen, bAggressive_, pDungeon);
	}
	else
	{
		sys_err( "NOT_EXIST_GROUP_GROUP_VNUM(%u) MAP(%ld)", dwVnum, lMapIndex );
		return NULL;
	}
}

LPCHARACTER CHARACTER_MANAGER::SpawnGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen, bool bAggressive_, LPDUNGEON pDungeon)
{
	CMobGroup * pkGroup = CMobManager::Instance().GetGroup(dwVnum);
	if (!pkGroup)
	{
		sys_err("NOT_EXIST_GROUP_VNUM(%u) Map(%u) ", dwVnum, lMapIndex);
		return NULL;
	}

	LPCHARACTER pkChrMaster = NULL;
	LPPARTY pkParty = NULL;

	const std::vector<DWORD> & c_rdwMembers = pkGroup->GetMemberVector();

	bool bSpawnedByStone = false;
	bool bAggressive = bAggressive_;

	if (m_pkChrSelectedStone)
	{
		bSpawnedByStone = true;

		if (m_pkChrSelectedStone->GetDungeon())
			bAggressive = true;
	}

	LPCHARACTER chLeader = NULL;

	for (DWORD i = 0; i < c_rdwMembers.size(); ++i)
	{
		LPCHARACTER tch = SpawnMobRange(c_rdwMembers[i], lMapIndex, sx, sy, ex, ey, true, bSpawnedByStone);
		if (!tch)
		{
			if (i == 0)
				return NULL;

			continue;
		}

		if (i == 0)
			chLeader = tch;

		tch->SetDungeon(pDungeon);

		sx = tch->GetX() - number(300, 500);
		sy = tch->GetY() - number(300, 500);
		ex = tch->GetX() + number(300, 500);
		ey = tch->GetY() + number(300, 500);

		if (m_pkChrSelectedStone)
			tch->SetStone(m_pkChrSelectedStone);
		else if (pkParty)
		{
			pkParty->Join(tch->GetVID());
			pkParty->Link(tch);
		}
		else if (!pkChrMaster)
		{
			pkChrMaster = tch;
			pkChrMaster->SetRegen(pkRegen);

			pkParty = CPartyManager::instance().CreateParty(pkChrMaster);
		}

		if (bAggressive)
			tch->SetAggressive();
	}

	return chLeader;
}

struct FuncUpdateAndResetChatCounter
{
	void operator () (LPCHARACTER ch)
	{
		ch->ResetChatCounter();
		ch->CFSM::Update();
		#ifdef ENABLE_RONARK_SYSTEM
		CRonarkMapManager::instance().SendScorePacket(ch);
		#endif
	}
};

void CHARACTER_MANAGER::Update(int iPulse)
{
	using namespace std;
	#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
	using namespace __gnu_cxx;
	#endif

	BeginPendingDestroy();

	{
		if (!m_map_pkPCChr.empty())
		{
			CHARACTER_VECTOR v;
			v.reserve(m_map_pkPCChr.size());
			#if (defined(__GNUC__) && !defined(__clang__)) || defined(CXX11_ENABLED)
			transform(m_map_pkPCChr.begin(), m_map_pkPCChr.end(), back_inserter(v), select2nd<NAME_MAP::value_type>());
			#else
			transform(m_map_pkPCChr.begin(), m_map_pkPCChr.end(), back_inserter(v), boost::bind(&NAME_MAP::value_type::second, _1));
			#endif

			if (0 == (iPulse % PASSES_PER_SEC(5)))
			{
				FuncUpdateAndResetChatCounter f;
				for_each(v.begin(), v.end(), f);
			}
			else
			{
				for_each(v.begin(), v.end(), msl::bind2nd(std::mem_fn(&CHARACTER::UpdateCharacter), iPulse));
			}
		}
	}

	{
		if (!m_set_pkChrState.empty())
		{
			CHARACTER_VECTOR v;
			v.reserve(m_set_pkChrState.size());
			#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
			transform(m_set_pkChrState.begin(), m_set_pkChrState.end(), back_inserter(v), identity<CHARACTER_SET::value_type>());
			#else
			v.insert(v.end(), m_set_pkChrState.begin(), m_set_pkChrState.end());
			#endif
			for_each(v.begin(), v.end(), msl::bind2nd(std::mem_fn(&CHARACTER::UpdateStateMachine), iPulse));
		}
	}

	{
		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(xmas::MOB_SANTA_VNUM, i))
		{
			for_each(i.begin(), i.end(),
					msl::bind2nd(std::mem_fn(&CHARACTER::UpdateStateMachine), iPulse));
		}
	}

	if (0 == (iPulse % PASSES_PER_SEC(3600)))
	{
		for (itertype(m_map_dwMobKillCount) it = m_map_dwMobKillCount.begin(); it != m_map_dwMobKillCount.end(); ++it)
			DBManager::instance().SendMoneyLog(MONEY_LOG_MONSTER_KILL, it->first, it->second);

		m_map_dwMobKillCount.clear();
	}

	if (test_server && 0 == (iPulse % PASSES_PER_SEC(60)))
		sys_log(0, "CHARACTER COUNT vid %zu pid %zu", m_map_pkChrByVID.size(), m_map_pkChrByPID.size());

	FlushPendingDestroy();

	// ShutdownManager Update
	CShutdownManager::Instance().Update();
}

void CHARACTER_MANAGER::ProcessDelayedSave()
{
	CHARACTER_SET::iterator it = m_set_pkChrForDelayedSave.begin();

	while (it != m_set_pkChrForDelayedSave.end())
	{
		LPCHARACTER pkChr = *it++;
		pkChr->SaveReal();
	}

	m_set_pkChrForDelayedSave.clear();
}

bool CHARACTER_MANAGER::AddToStateList(LPCHARACTER ch)
{
	assert(ch != NULL);

	CHARACTER_SET::iterator it = m_set_pkChrState.find(ch);
	if (it == m_set_pkChrState.end())
	{
		m_set_pkChrState.emplace(ch);
		return true;
	}

	return false;
}

void CHARACTER_MANAGER::RemoveFromStateList(LPCHARACTER ch)
{
	CHARACTER_SET::iterator it = m_set_pkChrState.find(ch);
	if (it != m_set_pkChrState.end())
	{
		//sys_log(0, "RemoveFromStateList %p", ch);
		m_set_pkChrState.erase(it);
	}
}

void CHARACTER_MANAGER::DelayedSave(LPCHARACTER ch)
{
	m_set_pkChrForDelayedSave.emplace(ch);
}

bool CHARACTER_MANAGER::FlushDelayedSave(LPCHARACTER ch)
{
	CHARACTER_SET::iterator it = m_set_pkChrForDelayedSave.find(ch);
	if (it == m_set_pkChrForDelayedSave.end())
		return false;

	m_set_pkChrForDelayedSave.erase(it);
	ch->SaveReal();
	return true;
}

void CHARACTER_MANAGER::RegisterForMonsterLog(LPCHARACTER ch)
{
	m_set_pkChrMonsterLog.emplace(ch);
}

void CHARACTER_MANAGER::UnregisterForMonsterLog(LPCHARACTER ch)
{
	m_set_pkChrMonsterLog.erase(ch);
}

void CHARACTER_MANAGER::PacketMonsterLog(LPCHARACTER ch, const void* buf, int size)
{
	itertype(m_set_pkChrMonsterLog) it;
	for (it = m_set_pkChrMonsterLog.begin(); it != m_set_pkChrMonsterLog.end(); ++it)
	{
		LPCHARACTER c = *it;
		if (ch && DISTANCE_APPROX(c->GetX()-ch->GetX(), c->GetY()-ch->GetY())>6000)
			continue;

		LPDESC d = c->GetDesc();
		if (d)
			d->Packet(buf, size);
	}
}

void CHARACTER_MANAGER::KillLog(DWORD dwVnum)
{
	const DWORD SEND_LIMIT = 10000;

	itertype(m_map_dwMobKillCount) it = m_map_dwMobKillCount.find(dwVnum);
	if (it == m_map_dwMobKillCount.end())
		m_map_dwMobKillCount.emplace(dwVnum, 1);
	else
	{
		++it->second;

		if (it->second > SEND_LIMIT)
		{
			DBManager::instance().SendMoneyLog(MONEY_LOG_MONSTER_KILL, it->first, it->second);
			m_map_dwMobKillCount.erase(it);
		}
	}
}

void CHARACTER_MANAGER::RegisterRaceNum(DWORD dwVnum)
{
	m_set_dwRegisteredRaceNum.emplace(dwVnum);
}

void CHARACTER_MANAGER::RegisterRaceNumMap(LPCHARACTER ch)
{
	DWORD dwVnum = ch->GetRaceNum();

	if (m_set_dwRegisteredRaceNum.find(dwVnum) != m_set_dwRegisteredRaceNum.end())
	{
		sys_log(0, "RegisterRaceNumMap %s %u", ch->GetName(), dwVnum);
		m_map_pkChrByRaceNum[dwVnum].emplace(ch);
	}
}

void CHARACTER_MANAGER::UnregisterRaceNumMap(LPCHARACTER ch)
{
	DWORD dwVnum = ch->GetRaceNum();

	itertype(m_map_pkChrByRaceNum) it = m_map_pkChrByRaceNum.find(dwVnum);
	if (it != m_map_pkChrByRaceNum.end())
		it->second.erase(ch);
}

bool CHARACTER_MANAGER::GetCharactersByRaceNum(DWORD dwRaceNum, CharacterVectorInteractor & i)
{
	std::map<DWORD, CHARACTER_SET>::iterator it = m_map_pkChrByRaceNum.find(dwRaceNum);
	if (it == m_map_pkChrByRaceNum.end())
		return false;

	i = it->second;
	return true;
}

#define FIND_JOB_WARRIOR_0	(1 << 3)
#define FIND_JOB_WARRIOR_1	(1 << 4)
#define FIND_JOB_WARRIOR_2	(1 << 5)
#define FIND_JOB_WARRIOR	(FIND_JOB_WARRIOR_0 | FIND_JOB_WARRIOR_1 | FIND_JOB_WARRIOR_2)
#define FIND_JOB_ASSASSIN_0	(1 << 6)
#define FIND_JOB_ASSASSIN_1	(1 << 7)
#define FIND_JOB_ASSASSIN_2	(1 << 8)
#define FIND_JOB_ASSASSIN	(FIND_JOB_ASSASSIN_0 | FIND_JOB_ASSASSIN_1 | FIND_JOB_ASSASSIN_2)
#define FIND_JOB_SURA_0		(1 << 9)
#define FIND_JOB_SURA_1		(1 << 10)
#define FIND_JOB_SURA_2		(1 << 11)
#define FIND_JOB_SURA		(FIND_JOB_SURA_0 | FIND_JOB_SURA_1 | FIND_JOB_SURA_2)
#define FIND_JOB_SHAMAN_0	(1 << 12)
#define FIND_JOB_SHAMAN_1	(1 << 13)
#define FIND_JOB_SHAMAN_2	(1 << 14)
#define FIND_JOB_SHAMAN		(FIND_JOB_SHAMAN_0 | FIND_JOB_SHAMAN_1 | FIND_JOB_SHAMAN_2)
#ifdef ENABLE_WOLFMAN_CHARACTER
#define FIND_JOB_WOLFMAN_0	(1 << 15)
#define FIND_JOB_WOLFMAN_1	(1 << 16)
#define FIND_JOB_WOLFMAN_2	(1 << 17)
#define FIND_JOB_WOLFMAN		(FIND_JOB_WOLFMAN_0 | FIND_JOB_WOLFMAN_1 | FIND_JOB_WOLFMAN_2)
#endif

//
// (job+1)*3+(skill_group)
//
LPCHARACTER CHARACTER_MANAGER::FindSpecifyPC(unsigned int uiJobFlag, long lMapIndex, LPCHARACTER except, int iMinLevel, int iMaxLevel)
{
	LPCHARACTER chFind = NULL;
	itertype(m_map_pkChrByPID) it;
	int n = 0;

	for (it = m_map_pkChrByPID.begin(); it != m_map_pkChrByPID.end(); ++it)
	{
		LPCHARACTER ch = it->second;

		if (ch == except)
			continue;

		if (ch->GetLevel() < iMinLevel)
			continue;

		if (ch->GetLevel() > iMaxLevel)
			continue;

		if (ch->GetMapIndex() != lMapIndex)
			continue;

		if (uiJobFlag)
		{
			unsigned int uiChrJob = (1 << ((ch->GetJob() + 1) * 3 + ch->GetSkillGroup()));

			if (!IS_SET(uiJobFlag, uiChrJob))
				continue;
		}

		if (!chFind || number(1, ++n) == 1)
			chFind = ch;
	}

	return chFind;
}

int CHARACTER_MANAGER::GetMobItemRate(LPCHARACTER ch)
{
	//PREVENT_TOXICATION_FOR_CHINA
	if (g_bChinaIntoxicationCheck)
	{
		if (ch->IsOverTime(OT_3HOUR))
		{
			if (ch && ch->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0)
				return m_iMobItemRatePremium/2;
			return m_iMobItemRate/2;
		}
		else if (ch->IsOverTime(OT_5HOUR))
		{
			return 0;
		}
	}
	//END_PREVENT_TOXICATION_FOR_CHINA

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0)
		return m_iMobItemRatePremium;
	return m_iMobItemRate;
}

int CHARACTER_MANAGER::GetMobDamageRate(LPCHARACTER ch)
{
	return m_iMobDamageRate;
}

int CHARACTER_MANAGER::GetMobGoldAmountRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iMobGoldAmountRate;

	//PREVENT_TOXICATION_FOR_CHINA
	if (g_bChinaIntoxicationCheck)
	{
		if (ch->IsOverTime(OT_3HOUR))
		{
			if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
				return m_iMobGoldAmountRatePremium/2;
			return m_iMobGoldAmountRate/2;
		}
		else if (ch->IsOverTime(OT_5HOUR))
		{
			return 0;
		}
	}
	//END_PREVENT_TOXICATION_FOR_CHINA

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		return m_iMobGoldAmountRatePremium;
	return m_iMobGoldAmountRate;
}

int CHARACTER_MANAGER::GetMobGoldDropRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iMobGoldDropRate;

	//PREVENT_TOXICATION_FOR_CHINA
	if (g_bChinaIntoxicationCheck)
	{
		if (ch->IsOverTime(OT_3HOUR))
		{
			if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
				return m_iMobGoldDropRatePremium/2;
			return m_iMobGoldDropRate/2;
		}
		else if ( ch->IsOverTime( OT_5HOUR ) )
		{
			return 0;
		}
	}
	//END_PREVENT_TOXICATION_FOR_CHINA

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		return m_iMobGoldDropRatePremium;
	return m_iMobGoldDropRate;
}

int CHARACTER_MANAGER::GetMobExpRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iMobExpRate;

	//PREVENT_TOXICATION_FOR_CHINA
	if (g_bChinaIntoxicationCheck)
	{
		if (ch->IsOverTime(OT_3HOUR))
		{
			if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
				return m_iMobExpRatePremium/2;
			return m_iMobExpRate/2;
		}
		else if (ch->IsOverTime(OT_5HOUR))
		{
			return 0;
		}
	}
	//END_PREVENT_TOXICATION_FOR_CHINA

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		return m_iMobExpRatePremium;
	return m_iMobExpRate;
}

int CHARACTER_MANAGER::GetUserDamageRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iUserDamageRate;

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		return m_iUserDamageRatePremium;

	return m_iUserDamageRate;
}

void CHARACTER_MANAGER::SendScriptToMap(long lMapIndex, const std::string & s)
{
	LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
	if (NULL == pSecMap)
		return;

	struct packet_script p;

	p.header = HEADER_GC_SCRIPT;
	p.skin = 1;
	p.src_size = s.size();

	quest::FSendPacket f;
	p.size = p.src_size + sizeof(struct packet_script);
	f.buf.write(&p, sizeof(struct packet_script));
	f.buf.write(&s[0], s.size());

	pSecMap->for_each(f);
}

bool CHARACTER_MANAGER::BeginPendingDestroy()
{
	if (m_bUsePendingDestroy)
		return false;

	m_bUsePendingDestroy = true;
	return true;
}

void CHARACTER_MANAGER::FlushPendingDestroy()
{
	using namespace std;

	m_bUsePendingDestroy = false;

	if (!m_set_pkChrPendingDestroy.empty())
	{
		sys_log(0, "FlushPendingDestroy size %d", m_set_pkChrPendingDestroy.size());

		CHARACTER_SET::iterator it = m_set_pkChrPendingDestroy.begin(), end = m_set_pkChrPendingDestroy.end();
		for (; it != end; ++it)
		{
			M2_DESTROY_CHARACTER(*it);
		}

		m_set_pkChrPendingDestroy.clear();
	}
}

CharacterVectorInteractor::CharacterVectorInteractor(const CHARACTER_SET & r)
{
	using namespace std;
	#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
	using namespace __gnu_cxx;
	#endif

	reserve(r.size());
	#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
	transform(r.begin(), r.end(), back_inserter(*this), identity<CHARACTER_SET::value_type>());
	#else
	insert(end(), r.begin(), r.end());
	#endif

	if (CHARACTER_MANAGER::instance().BeginPendingDestroy())
		m_bMyBegin = true;
}

CharacterVectorInteractor::~CharacterVectorInteractor()
{
	if (m_bMyBegin)
		CHARACTER_MANAGER::instance().FlushPendingDestroy();
}

#ifdef ENABLE_MULTI_FARM_BLOCK
void CHARACTER_MANAGER::CheckMultiFarmAccounts(const char* szIP)
{
	auto it = m_mapmultiFarm.find(szIP);
	if (it != m_mapmultiFarm.end())
	{
		auto itVec = it->second.begin();
		while(itVec != it->second.end())
		{
			LPCHARACTER ch = FindByPID(itVec->playerID);
			CCI* chP2P = P2P_MANAGER::Instance().FindByPID(itVec->playerID);
			if (!ch && !chP2P)
				itVec = it->second.erase(itVec);
			else
				++itVec;
		}
		if (!it->second.size())
			m_mapmultiFarm.erase(szIP);
	}
}

void CHARACTER_MANAGER::RemoveMultiFarm(const char* szIP, const DWORD playerID, const bool isP2P)
{
	if (!isP2P)
	{
		TPacketGGMultiFarm p;
		p.header = HEADER_GG_MULTI_FARM;
		p.subHeader = MULTI_FARM_REMOVE;
		p.playerID = playerID;
		strlcpy(p.playerIP, szIP, sizeof(p.playerIP));
		P2P_MANAGER::Instance().Send(&p, sizeof(TPacketGGMultiFarm));
	}

	auto it = m_mapmultiFarm.find(szIP);
	if (it != m_mapmultiFarm.end())
	{
		for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
		{
			if (itVec->playerID == playerID)
			{
				it->second.erase(itVec);
				break;
			}
		}
		if (!it->second.size())
			m_mapmultiFarm.erase(szIP);
	}

	std::map<DWORD, std::pair<std::string, bool>> m_mapNames;
	for (auto it = m_mapNames.begin(); it != m_mapNames.end(); ++it)
	{
		LPCHARACTER newCh = FindByPID(it->first);
		if (newCh)
		{
			newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiFarmAffect %d 0", newCh->GetRewardStatus());
			for (auto itEx = m_mapNames.begin(); itEx != m_mapNames.end(); ++itEx)
			{
				if (itEx->second.second)
					newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiFarmPlayer %s", itEx->second.first.c_str());
			}
		}
	}
}

void CHARACTER_MANAGER::SetMultiFarm(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, const BYTE affectType, const long affectTime)
{
	const auto it = m_mapmultiFarm.find(szIP);
	if (it != m_mapmultiFarm.end())
	{
		for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
		{
			if (itVec->playerID == playerID)
			{
				itVec->farmStatus = bStatus;
				itVec->affectType = affectType;
				itVec->affectTime = affectTime;
				return;
			}
		}
		it->second.emplace_back(TMultiFarm(playerID, playerName, bStatus, affectType, affectTime));
	}
	else
	{
		std::vector<TMultiFarm> m_vecFarmList;
		m_vecFarmList.emplace_back(TMultiFarm(playerID, playerName, bStatus, affectType, affectTime));
		m_mapmultiFarm.emplace(szIP, m_vecFarmList);
	}
}

int CHARACTER_MANAGER::GetMultiFarmCount(const char* playerIP, std::map<DWORD, std::pair<std::string, bool>>& m_mapNames)
{
	int accCount = 0;
	bool affectTimeHas = false;
	BYTE affectType = 0;

	const auto it = m_mapmultiFarm.find(playerIP);
	if (it != m_mapmultiFarm.end())
	{
		for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
		{
			if (itVec->farmStatus)
				accCount++;

			if (itVec->affectTime > get_global_time())
				affectTimeHas = true;

			if (itVec->affectType > affectType)
				affectType = itVec->affectType;

			m_mapNames.emplace(itVec->playerID, std::make_pair(itVec->playerName, itVec->farmStatus));
		}
	}

	if (affectTimeHas && affectType > 0)
		accCount -= affectType;

	if (accCount < 0)
		accCount = 0;

	return accCount;
}

void CHARACTER_MANAGER::CheckMultiFarmAccount(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, BYTE affectType, long affectDuration, bool isP2P)
{
	CheckMultiFarmAccounts(szIP);

	LPCHARACTER ch = FindByPID(playerID);
	if (ch)
	{
		CAffect * pAffect = ch->FindAffect(AFFECT_MULTI_FARM_PREMIUM);
		if (NULL != pAffect)
		{
			affectDuration = pAffect->lDuration;
			affectType = pAffect->lApplyValue;

			if (uint64_t(get_global_time()+affectDuration) >= INFINITE_AFFECT_DURATION)
			{
				affectDuration = INFINITE_AFFECT_DURATION;
			}
			else
			{
				affectDuration += get_global_time();
			}
		}

		// ch->ChatPacket(CHAT_TYPE_INFO, "Dur[%ld]-Type[%u]", affectDuration, affectType);
	}

	std::map<DWORD, std::pair<std::string, bool>> m_mapNames;
	int farmPlayerCount = GetMultiFarmCount(szIP, m_mapNames);

	if (bStatus)
	{
		if (farmPlayerCount >= MULTI_FARM_CLIENT_COUNT)
		{
			CheckMultiFarmAccount(szIP, playerID, playerName, false);
			return;
		}
	}

	if (!isP2P)
	{
		TPacketGGMultiFarm p;
		p.header = HEADER_GG_MULTI_FARM;
		p.subHeader = MULTI_FARM_SET;
		p.playerID = playerID;
		strlcpy(p.playerIP, szIP, sizeof(p.playerIP));
		strlcpy(p.playerName, playerName, sizeof(p.playerIP));
		p.farmStatus = bStatus;
		p.affectType = affectType;
		p.affectTime = affectDuration;
		P2P_MANAGER::Instance().Send(&p, sizeof(TPacketGGMultiFarm));
	}

	SetMultiFarm(szIP, playerID, playerName, bStatus, affectType, affectDuration);

	if (ch)
	{
		ch->SetRewardStatus(bStatus);
	}

	m_mapNames.clear();
	farmPlayerCount = GetMultiFarmCount(szIP, m_mapNames);

	for (auto it = m_mapNames.begin(); it != m_mapNames.end(); ++it)
	{
		LPCHARACTER newCh = FindByPID(it->first);
		if (newCh)
		{
			newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiFarmAffect %d %d", newCh->GetRewardStatus(), newCh == ch ? true : false);
			for (auto itEx = m_mapNames.begin(); itEx != m_mapNames.end(); ++itEx)
			{
				if (itEx->second.second)
					newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiFarmPlayer %s", itEx->second.first.c_str());
			}
		}
	}
}
#endif

#ifdef ENABLE_MULTI_HUNT_BLOCK
void CHARACTER_MANAGER::CheckMultiHuntAccounts(const char* szIP)
{
	auto it = m_mapmultiHunt.find(szIP);
	if (it != m_mapmultiHunt.end())
	{
		auto itVec = it->second.begin();
		while(itVec != it->second.end())
		{
			LPCHARACTER ch = FindByPID(itVec->playerID);
			CCI* chP2P = P2P_MANAGER::Instance().FindByPID(itVec->playerID);
			if (!ch && !chP2P)
				itVec = it->second.erase(itVec);
			else
				++itVec;
		}
		if (!it->second.size())
			m_mapmultiHunt.erase(szIP);
	}
}

void CHARACTER_MANAGER::RemoveMultiHunt(const char* szIP, const DWORD playerID, const bool isP2P)
{
	if (!isP2P)
	{
		TPacketGGMultiHunt p;
		p.header = HEADER_GG_MULTI_HUNT;
		p.subHeader = MULTI_HUNT_REMOVE;
		p.playerID = playerID;
		strlcpy(p.playerIP, szIP, sizeof(p.playerIP));
		P2P_MANAGER::Instance().Send(&p, sizeof(TPacketGGMultiHunt));
	}

	auto it = m_mapmultiHunt.find(szIP);
	if (it != m_mapmultiHunt.end())
	{
		for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
		{
			if (itVec->playerID == playerID)
			{
				it->second.erase(itVec);
				break;
			}
		}
		if (!it->second.size())
			m_mapmultiHunt.erase(szIP);
	}

	std::map<DWORD, std::pair<std::string, bool>> m_mapNames;
	for (auto it = m_mapNames.begin(); it != m_mapNames.end(); ++it)
	{
		LPCHARACTER newCh = FindByPID(it->first);
		if (newCh)
		{
			newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiHuntAffect %d 0", newCh->GetMultiHuntStatus());
			for (auto itEx = m_mapNames.begin(); itEx != m_mapNames.end(); ++itEx)
			{
				if (itEx->second.second)
					newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiHuntPlayer %s", itEx->second.first.c_str());
			}
		}
	}
}

void CHARACTER_MANAGER::SetMultiHunt(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, const BYTE affectType, const long affectTime)
{
	const auto it = m_mapmultiHunt.find(szIP);
	if (it != m_mapmultiHunt.end())
	{
		for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
		{
			if (itVec->playerID == playerID)
			{
				itVec->huntStatus = bStatus;
				itVec->affectType = affectType;
				itVec->affectTime = affectTime;
				return;
			}
		}
		it->second.emplace_back(TMultiHunt(playerID, playerName, bStatus, affectType, affectTime));
	}
	else
	{
		std::vector<TMultiHunt> m_vecHuntList;
		m_vecHuntList.emplace_back(TMultiHunt(playerID, playerName, bStatus, affectType, affectTime));
		m_mapmultiHunt.emplace(szIP, m_vecHuntList);
	}
}

int CHARACTER_MANAGER::GetMultiHuntCount(const char* playerIP, std::map<DWORD, std::pair<std::string, bool>>& m_mapNames)
{
	int accCount = 0;
	bool affectTimeHas = false;
	BYTE affectType = 0;

	const auto it = m_mapmultiHunt.find(playerIP);
	if (it != m_mapmultiHunt.end())
	{
		for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
		{
			if (itVec->huntStatus)
				accCount++;

			if (itVec->affectTime > get_global_time())
				affectTimeHas = true;

			if (itVec->affectType > affectType)
				affectType = itVec->affectType;

			m_mapNames.emplace(itVec->playerID, std::make_pair(itVec->playerName, itVec->huntStatus));
		}
	}

	if (affectTimeHas && affectType > 0)
		accCount -= affectType;

	if (accCount < 0)
		accCount = 0;

	return accCount;
}

void CHARACTER_MANAGER::CheckMultiHuntAccount(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, BYTE affectType, long affectDuration, bool isP2P)
{
	CheckMultiHuntAccounts(szIP);

	LPCHARACTER ch = FindByPID(playerID);
	if (ch)
	{
		CAffect * pAffect = ch->FindAffect(AFFECT_MULTI_HUNT_PREMIUM);
		if (NULL != pAffect)
		{
			affectDuration = pAffect->lDuration;
			affectType = pAffect->lApplyValue;

			if (uint64_t(get_global_time()+affectDuration) >= INFINITE_AFFECT_DURATION)
			{
				affectDuration = INFINITE_AFFECT_DURATION;
			}
			else
			{
				affectDuration += get_global_time();
			}
		}

		// ch->ChatPacket(CHAT_TYPE_INFO, "Dur[%ld]-Type[%u]", affectDuration, affectType);
	}

	std::map<DWORD, std::pair<std::string, bool>> m_mapNames;
	int huntPlayerCount = GetMultiHuntCount(szIP, m_mapNames);

	if (bStatus)
	{
		if (huntPlayerCount >= MULTI_HUNT_CLIENT_COUNT)
		{
			CheckMultiHuntAccount(szIP, playerID, playerName, false);
			return;
		}
	}

	if (!isP2P)
	{
		TPacketGGMultiHunt p;
		p.header = HEADER_GG_MULTI_HUNT;
		p.subHeader = MULTI_HUNT_SET;
		p.playerID = playerID;
		strlcpy(p.playerIP, szIP, sizeof(p.playerIP));
		strlcpy(p.playerName, playerName, sizeof(p.playerIP));
		p.huntStatus = bStatus;
		p.affectType = affectType;
		p.affectTime = affectDuration;
		P2P_MANAGER::Instance().Send(&p, sizeof(TPacketGGMultiHunt));
	}

	SetMultiHunt(szIP, playerID, playerName, bStatus, affectType, affectDuration);

	if (ch)
	{
		ch->SetMultiHuntStatus(bStatus);
	}

	m_mapNames.clear();
	huntPlayerCount = GetMultiHuntCount(szIP, m_mapNames);

	for (auto it = m_mapNames.begin(); it != m_mapNames.end(); ++it)
	{
		LPCHARACTER newCh = FindByPID(it->first);
		if (newCh)
		{
			newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiHuntAffect %d %d", newCh->GetMultiHuntStatus(), newCh == ch ? true : false);
			for (auto itEx = m_mapNames.begin(); itEx != m_mapNames.end(); ++itEx)
			{
				if (itEx->second.second)
					newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiHuntPlayer %s", itEx->second.first.c_str());
			}
		}
	}
}
#endif

#ifdef ENABLE_MULTI_FISH_BLOCK
void CHARACTER_MANAGER::CheckMultiFishAccounts(const char* szIP)
{
	auto it = m_mapmultiFish.find(szIP);
	if (it != m_mapmultiFish.end())
	{
		auto itVec = it->second.begin();
		while(itVec != it->second.end())
		{
			LPCHARACTER ch = FindByPID(itVec->playerID);
			CCI* chP2P = P2P_MANAGER::Instance().FindByPID(itVec->playerID);
			if (!ch && !chP2P)
				itVec = it->second.erase(itVec);
			else
				++itVec;
		}
		if (!it->second.size())
			m_mapmultiFish.erase(szIP);
	}
}

void CHARACTER_MANAGER::RemoveMultiFish(const char* szIP, const DWORD playerID, const bool isP2P)
{
	if (!isP2P)
	{
		TPacketGGMultiFish p;
		p.header = HEADER_GG_MULTI_FISH;
		p.subHeader = MULTI_FISH_REMOVE;
		p.playerID = playerID;
		strlcpy(p.playerIP, szIP, sizeof(p.playerIP));
		P2P_MANAGER::Instance().Send(&p, sizeof(TPacketGGMultiFish));
	}

	auto it = m_mapmultiFish.find(szIP);
	if (it != m_mapmultiFish.end())
	{
		for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
		{
			if (itVec->playerID == playerID)
			{
				it->second.erase(itVec);
				break;
			}
		}
		if (!it->second.size())
			m_mapmultiFish.erase(szIP);
	}

	std::map<DWORD, std::pair<std::string, bool>> m_mapNames;
	for (auto it = m_mapNames.begin(); it != m_mapNames.end(); ++it)
	{
		LPCHARACTER newCh = FindByPID(it->first);
		if (newCh)
		{
			newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiFishAffect %d 0", newCh->GetMultiFishStatus());
			for (auto itEx = m_mapNames.begin(); itEx != m_mapNames.end(); ++itEx)
			{
				if (itEx->second.second)
					newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiFishPlayer %s", itEx->second.first.c_str());
			}
		}
	}
}

void CHARACTER_MANAGER::SetMultiFish(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, const BYTE affectType, const long affectTime)
{
	const auto it = m_mapmultiFish.find(szIP);
	if (it != m_mapmultiFish.end())
	{
		for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
		{
			if (itVec->playerID == playerID)
			{
				itVec->fishStatus = bStatus;
				itVec->affectType = affectType;
				itVec->affectTime = affectTime;
				return;
			}
		}
		it->second.emplace_back(TMultiFish(playerID, playerName, bStatus, affectType, affectTime));
	}
	else
	{
		std::vector<TMultiFish> m_vecFishList;
		m_vecFishList.emplace_back(TMultiFish(playerID, playerName, bStatus, affectType, affectTime));
		m_mapmultiFish.emplace(szIP, m_vecFishList);
	}
}

int CHARACTER_MANAGER::GetMultiFishCount(const char* playerIP, std::map<DWORD, std::pair<std::string, bool>>& m_mapNames)
{
	int accCount = 0;
	bool affectTimeHas = false;
	BYTE affectType = 0;

	const auto it = m_mapmultiFish.find(playerIP);
	if (it != m_mapmultiFish.end())
	{
		for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
		{
			if (itVec->fishStatus)
				accCount++;

			if (itVec->affectTime > get_global_time())
				affectTimeHas = true;

			if (itVec->affectType > affectType)
				affectType = itVec->affectType;

			m_mapNames.emplace(itVec->playerID, std::make_pair(itVec->playerName, itVec->fishStatus));
		}
	}

	if (affectTimeHas && affectType > 0)
		accCount -= affectType;

	if (accCount < 0)
		accCount = 0;

	return accCount;
}

void CHARACTER_MANAGER::CheckMultiFishAccount(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, BYTE affectType, long affectDuration, bool isP2P)
{
	CheckMultiFishAccounts(szIP);

	LPCHARACTER ch = FindByPID(playerID);
	if (ch)
	{
		CAffect * pAffect = ch->FindAffect(AFFECT_MULTI_FISH_PREMIUM);
		if (NULL != pAffect)
		{
			affectDuration = pAffect->lDuration;
			affectType = pAffect->lApplyValue;

			if (uint64_t(get_global_time()+affectDuration) >= INFINITE_AFFECT_DURATION)
			{
				affectDuration = INFINITE_AFFECT_DURATION;
			}
			else
			{
				affectDuration += get_global_time();
			}
		}

		// ch->ChatPacket(CHAT_TYPE_INFO, "Dur[%ld]-Type[%u]", affectDuration, affectType);
	}

	std::map<DWORD, std::pair<std::string, bool>> m_mapNames;
	int fishPlayerCount = GetMultiFishCount(szIP, m_mapNames);

	if (bStatus)
	{
		if (fishPlayerCount >= MULTI_FISH_CLIENT_COUNT)
		{
			CheckMultiFishAccount(szIP, playerID, playerName, false);
			return;
		}
	}

	if (!isP2P)
	{
		TPacketGGMultiFish p;
		p.header = HEADER_GG_MULTI_FISH;
		p.subHeader = MULTI_FISH_SET;
		p.playerID = playerID;
		strlcpy(p.playerIP, szIP, sizeof(p.playerIP));
		strlcpy(p.playerName, playerName, sizeof(p.playerIP));
		p.fishStatus = bStatus;
		p.affectType = affectType;
		p.affectTime = affectDuration;
		P2P_MANAGER::Instance().Send(&p, sizeof(TPacketGGMultiFish));
	}

	SetMultiFish(szIP, playerID, playerName, bStatus, affectType, affectDuration);

	if (ch)
	{
		ch->SetMultiFishStatus(bStatus);
	}

	m_mapNames.clear();
	fishPlayerCount = GetMultiFishCount(szIP, m_mapNames);

	for (auto it = m_mapNames.begin(); it != m_mapNames.end(); ++it)
	{
		LPCHARACTER newCh = FindByPID(it->first);
		if (newCh)
		{
			newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiFishAffect %d %d", newCh->GetMultiFishStatus(), newCh == ch ? true : false);
			for (auto itEx = m_mapNames.begin(); itEx != m_mapNames.end(); ++itEx)
			{
				if (itEx->second.second)
					newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiFishPlayer %s", itEx->second.first.c_str());
			}
		}
	}
}
#endif

#ifdef __RANKING_SYSTEM__
bool CHARACTER_MANAGER::GetRankData(BYTE categoryIndex, BYTE raceIndex, BYTE rankIndex, std::vector<TRanking>& list)
{
	list.clear();

	auto outerIt = m_map_ranks.find({categoryIndex, raceIndex});
	if (outerIt != m_map_ranks.end())
	{
		auto& innerMap = outerIt->second;
		auto innerIt = innerMap.find(rankIndex);
		if (innerIt != innerMap.end())
		{
			for (const auto& val : innerIt->second)
			{
				list.emplace_back(val);
			}
		}
	}

	return true;
}

bool CompareValue(TRanking i, TRanking j)
{
	return (i.value > j.value);
}

void CHARACTER_MANAGER::SetRankData(const char* c_pData)
{
	m_map_ranks.clear();
	const BYTE rankTotalSize = *(BYTE*)c_pData;
	c_pData += sizeof(BYTE);

	for (BYTE j = 0; j < rankTotalSize; ++j)
	{
		const BYTE categoryIndex = *(BYTE*)c_pData;//CategoryId
		c_pData += sizeof(BYTE);
		const BYTE raceIndex = *(BYTE*)c_pData;//RaceId
		c_pData += sizeof(BYTE);
		const BYTE mapSize = *(BYTE*)c_pData;//MapSize
		c_pData += sizeof(BYTE);

		for (BYTE x = 0; x < mapSize; ++x)
		{
			const BYTE rankIndex = *(BYTE*)c_pData;//RankId
			c_pData += sizeof(BYTE);

			const BYTE vecSize = *(BYTE*)c_pData;//VectorSize
			c_pData += sizeof(BYTE);

			if (vecSize > 0)
			{
				std::vector<TRanking> m_vec;
				for (DWORD x = 0; x < vecSize; ++x)
				{
					const TRanking rankData = *(TRanking*)c_pData;
					m_vec.emplace_back(rankData);
					c_pData += sizeof(TRanking);
				}

				std::stable_sort(m_vec.begin(), m_vec.end(), CompareValue);
				for (DWORD x = 0; x < m_vec.size(); ++x)
					m_vec[x].index = x+1;

				m_map_ranks[{categoryIndex, raceIndex}].emplace(rankIndex, m_vec);
			}
		}
	}
}
#endif

#ifdef ENABLE_EVENT_MANAGER
#include "item_manager.h"
#include "item.h"
#include "desc_client.h"
#include "desc_manager.h"

void CHARACTER_MANAGER::ClearEventData()
{
	m_eventData.clear();
}

/*void CHARACTER_MANAGER::CheckBonusEvent(LPCHARACTER ch)
{
	const TEventManagerData* eventPtr = CheckEventIsActive(BONUS_EVENT, ch->GetEmpire());
	if (eventPtr)
		ch->ApplyPoint(eventPtr->value[0], eventPtr->value[1]);
}*/

void CHARACTER_MANAGER::CheckBonusEvent(LPCHARACTER ch)
{
	if (!ch || !ch->GetDesc())
		return;

	BYTE eventIndex = BONUS_EVENT;
	BYTE empireIndex = ch->GetEmpire();

	for (auto it = m_eventData.begin(); it != m_eventData.end(); ++it)
	{
		const auto& dayVector = it->second;

		for (DWORD j = 0; j < dayVector.size(); ++j)
		{
			const auto & eventData = dayVector[j];

			if (eventData.eventIndex == eventIndex)
			{
				if (eventData.channelFlag != 0)
					if (eventData.channelFlag != g_bChannel)
						continue;

				if (eventData.empireFlag != 0 && empireIndex != 0)
					if (eventData.empireFlag != empireIndex)
						continue;

				if (eventData.eventStatus == false)
					continue;

				ch->ApplyPoint(eventData.value[0], eventData.value[1]);
			}
		}
	}
}

const TEventManagerData* CHARACTER_MANAGER::CheckEventIsActive(BYTE eventIndex, BYTE empireIndex)
{
	for (auto it = m_eventData.begin(); it != m_eventData.end(); ++it)
	{
		// const auto& dayIndex = it->first;
		const auto& dayVector = it->second;
		for (DWORD j = 0; j < dayVector.size(); ++j)
		{
			const auto & eventData = dayVector[j];
			if (eventData.eventIndex == eventIndex)
			{
				if (eventData.channelFlag != 0)
					if (eventData.channelFlag != g_bChannel)
						continue;

				if (eventData.empireFlag != 0 && empireIndex != 0)
					if (eventData.empireFlag != empireIndex)
						continue;

				if (eventData.eventStatus == true)
					return &eventData;
			}
		}
	}

	return NULL;
}

void CHARACTER_MANAGER::CheckEventForDrop(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM>& vec_item)
{
	const BYTE killerEmpire = pkKiller->GetEmpire();
	const TEventManagerData* eventPtr = NULL;
	LPITEM rewardItem = NULL;

	if (pkChr->IsStone())
	{
		eventPtr = CheckEventIsActive(DOUBLE_METIN_LOOT_EVENT, killerEmpire);
		if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
		{
			const int prob = number(1, 100);
			const int success_prob = eventPtr->value[3];
			if (success_prob >= prob)
			{
				std::vector<LPITEM> m_cache;
				for (DWORD j = 0; j < vec_item.size(); ++j)
				{
					const auto vItem = vec_item[j];
					rewardItem = ITEM_MANAGER::Instance().CreateItem(vItem->GetVnum(), vItem->GetCount(), 0, true);
					if (rewardItem) m_cache.emplace_back(rewardItem);
				}
				for (DWORD j = 0; j < m_cache.size(); ++j)
					vec_item.emplace_back(m_cache[j]);
			}
		}
	}
	else if (pkChr->GetMobRank() >= MOB_RANK_BOSS)
	{
		eventPtr = CheckEventIsActive(DOUBLE_BOSS_LOOT_EVENT, killerEmpire);
		if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
		{
			const int prob = number(1, 100);
			const int success_prob = eventPtr->value[3];
			if (success_prob >= prob)
			{
				std::vector<LPITEM> m_cache;
				for (DWORD j = 0; j < vec_item.size(); ++j)
				{
					const auto vItem = vec_item[j];
					rewardItem = ITEM_MANAGER::Instance().CreateItem(vItem->GetVnum(), vItem->GetCount(), 0, true);
					if (rewardItem) m_cache.emplace_back(rewardItem);
				}
				for (DWORD j = 0; j < m_cache.size(); ++j)
					vec_item.emplace_back(m_cache[j]);
			}
		}
	}

	eventPtr = CheckEventIsActive(DOUBLE_MISSION_BOOK_EVENT, killerEmpire);
	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 100);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			// If your moonlight item vnum is different change 50011!
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(77917, 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}

	eventPtr = CheckEventIsActive(DUNGEON_TICKET_LOOT_EVENT, killerEmpire);
	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 100);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			// If you have different book index put here!
			constexpr DWORD m_lticketItems[] = { 25163, 25164, 25165, 25166 , 25167 , 25168 , 25169 };
			std::vector<LPITEM> m_cache;
			for (DWORD j = 0; j < vec_item.size();++j)
			{
				const auto vItem = vec_item[j];
				const DWORD itemVnum = vItem->GetVnum();
				for (DWORD x = 0; x < _countof(m_lticketItems); ++x)
				{
					if (m_lticketItems[x] == itemVnum)
					{
						rewardItem = ITEM_MANAGER::Instance().CreateItem(itemVnum, vItem->GetCount(), 0, true);
						if (rewardItem) m_cache.emplace_back(rewardItem);
						break;
					}
				}
				for (DWORD j = 0; j < m_cache.size();++j)
					vec_item.emplace_back(m_cache[j]);
			}
		}
	}

	/*eventPtr = CheckEventIsActive(MOONLIGHT_EVENT, killerEmpire);
	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 100);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			// If your moonlight item vnum is different change 50011!
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(50011, 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}*/

	eventPtr = CheckEventIsActive(CARD_EVENT, killerEmpire);
	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 100);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(50037, 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}

	eventPtr = CheckEventIsActive(TREASURE_KEY, killerEmpire);
	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 100);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(50096, 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}

	for (int event = MOONLIGHT_EVENT; event < EVENT_MANAGER_MAX_NUM; event++)
	{
		eventPtr = CheckEventIsActive(event, killerEmpire);
		if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
		{
			int prob = number(1, 100);
			int success_prob = eventPtr->value[3];

			if (pkChr->IsStone() || (pkChr->GetMobRank() >= MOB_RANK_BOSS))
				success_prob = eventPtr->value[2];

			if (success_prob >= prob)
			{
				DWORD itemVnum = eventPtr->value[0];
				DWORD itemCount = eventPtr->value[1];

				LPITEM item = ITEM_MANAGER::Instance().CreateItem(itemVnum, itemCount, 0, true);
				if (item) vec_item.emplace_back(item);
			}
		}
	}
}

void CHARACTER_MANAGER::CompareEventSendData(TEMP_BUFFER* buf)
{
	const BYTE dayCount = m_eventData.size();
	const BYTE subIndex = EVENT_MANAGER_LOAD;
	const int cur_Time = time(NULL);

	TPacketGCEventManager p;
	p.header = HEADER_GC_EVENT_MANAGER;
	p.size = sizeof(TPacketGCEventManager) + sizeof(BYTE)+sizeof(BYTE)+sizeof(int);

	for(auto it = m_eventData.begin();it != m_eventData.end();++it)
	{
		// const auto& dayIndex = it->first;
		const auto& dayData = it->second;
		const BYTE dayEventCount = dayData.size();
		p.size += sizeof(BYTE) + sizeof(BYTE) + (dayEventCount * sizeof(TEventManagerData));
	}

	buf->write(&p, sizeof(TPacketGCEventManager));
	buf->write(&subIndex, sizeof(BYTE));
	buf->write(&dayCount, sizeof(BYTE));
	buf->write(&cur_Time, sizeof(int));

	for(auto it = m_eventData.begin(); it != m_eventData.end(); ++it)
	{
		const auto& dayIndex = it->first;
		const auto& dayData = it->second;
		const BYTE dayEventCount = dayData.size();
		buf->write(&dayIndex, sizeof(BYTE));
		buf->write(&dayEventCount, sizeof(BYTE));

		if (dayEventCount > 0)
			buf->write(dayData.data(), dayEventCount * sizeof(TEventManagerData));
	}
}

void CHARACTER_MANAGER::UpdateAllPlayerEventData()
{
	TEMP_BUFFER buf;
	CompareEventSendData(&buf);
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
	{
		LPCHARACTER ch = (*it)->GetCharacter();
		if (!ch)
			continue;

		(*it)->Packet(buf.read_peek(), buf.size());
	}
}

void CHARACTER_MANAGER::SendDataPlayer(LPCHARACTER ch)
{
	auto desc = ch->GetDesc();
	if (!desc)
		return;

	TEMP_BUFFER buf;
	CompareEventSendData(&buf);
	desc->Packet(buf.read_peek(), buf.size());

	const auto event = CheckEventIsActive(BEGGINER_EVENT, ch->GetEmpire());
	if (event != NULL && ch->GetQuestFlag("event_manager.is_new_player") == 0 && ch->GetLevel() == 0)
	{
		//value0 - itemvnum / value1 - itemcount
		ch->AutoGiveItem(event->value[0], event->value[1]);
		ch->ChatPacket(CHAT_TYPE_INFO, "1117");
		ch->SetQuestFlag("event_manager.is_new_player", 1);
	}

	if (ch->GetLevel() == 0)
		ch->PointChange(POINT_LEVEL, 1);
}

bool CHARACTER_MANAGER::CloseEventManuel(BYTE eventIndex)
{
	auto eventPtr = CheckEventIsActive(eventIndex);
	if (eventPtr != NULL)
	{
		const BYTE subHeader = EVENT_MANAGER_REMOVE_EVENT;
		db_clientdesc->DBPacketHeader(HEADER_GD_EVENT_MANAGER, 0, sizeof(BYTE)+sizeof(WORD));
		db_clientdesc->Packet(&subHeader, sizeof(BYTE));
		db_clientdesc->Packet(&eventPtr->eventID, sizeof(WORD));
		return true;
	}
	return false;
}

void CHARACTER_MANAGER::SetEventStatus(const WORD eventID, const bool eventStatus, const int endTime, const char* endTimeText)
{
	//eventStatus - 0-deactive  // 1-active

	TEventManagerData* eventData = NULL;
	for (auto it = m_eventData.begin(); it != m_eventData.end(); ++it)
	{
		if (it->second.size())
		{
			for (DWORD j = 0; j < it->second.size(); ++j)
			{
				TEventManagerData& pData = it->second[j];
				if (pData.eventID == eventID)
				{
					eventData = &pData;
					break;
				}
			}
		}
	}

	if (eventData == NULL)
		return;

	eventData->eventStatus = eventStatus;
	eventData->endTime = endTime;
	strlcpy(eventData->endTimeText, endTimeText, sizeof(eventData->endTimeText));

	// Auto open&close notice
	const std::map<BYTE, std::pair<std::string, std::string>> m_eventText = {
		{BONUS_EVENT,{"1078","1079"}},
		{DOUBLE_BOSS_LOOT_EVENT,{"1080","1081"}},
		{DOUBLE_METIN_LOOT_EVENT,{"1082","1083"}},
		{DOUBLE_MISSION_BOOK_EVENT,{"1084","1085"}},
		{DUNGEON_COOLDOWN_EVENT,{"1086","1087"}},
		{DUNGEON_TICKET_LOOT_EVENT,{"1088","1089"}},
		{MOONLIGHT_EVENT,{"1090","1091"}},

		{CARD_EVENT,{"1111","1112"}},//AFTERSET
		{TREASURE_KEY,{"1115","1116"}},
		{BEGGINER_EVENT,{"1118","1119"}},
	};

	const auto it = m_eventText.find(eventData->eventIndex);
	if (it != m_eventText.end())
	{
		if (eventStatus)
			SendNotice(it->second.first.c_str());
		else
			SendNotice(it->second.second.c_str());
	}

	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();

	// Bonus event update status
	if (eventData->eventIndex == BONUS_EVENT)
	{
		for (auto itDesc = c_ref_set.begin(); itDesc != c_ref_set.end(); ++itDesc)
		{
			LPCHARACTER ch = (*itDesc)->GetCharacter();
			if (!ch)
				continue;

			if (eventData->empireFlag != 0)
				if (eventData->empireFlag != ch->GetEmpire())
					continue;

			if (eventData->channelFlag != 0)
				if (eventData->channelFlag != g_bChannel)
					return;

			if (!eventStatus)
			{
				const long value = eventData->value[1];
				ch->ApplyPoint(eventData->value[0], -value);
			}

			ch->ComputePoints();
		}
	}

	const int now = time(0);
	const BYTE subIndex = EVENT_MANAGER_EVENT_STATUS;

	TPacketGCEventManager p;
	p.header = HEADER_GC_EVENT_MANAGER;
	p.size = sizeof(TPacketGCEventManager)+sizeof(BYTE)+sizeof(WORD)+sizeof(bool)+sizeof(int)+sizeof(int)+sizeof(eventData->endTimeText);

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(TPacketGCEventManager));
	buf.write(&subIndex, sizeof(BYTE));
	buf.write(&eventData->eventID, sizeof(WORD));
	buf.write(&eventData->eventStatus, sizeof(bool));
	buf.write(&eventData->endTime, sizeof(int));
	buf.write(&eventData->endTimeText, sizeof(eventData->endTimeText));
	buf.write(&now, sizeof(int));

	for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
	{
		LPCHARACTER ch = (*it)->GetCharacter();
		if (!ch)
			continue;

		(*it)->Packet(buf.read_peek(), buf.size());
	}
}

void CHARACTER_MANAGER::SetEventData(BYTE dayIndex, const std::vector<TEventManagerData>& m_data)
{
	const auto it = m_eventData.find(dayIndex);
	if (it == m_eventData.end())
		m_eventData.emplace(dayIndex, m_data);
	else
	{
		it->second.clear();
		for (DWORD j = 0; j < m_data.size();++j)
			it->second.emplace_back(m_data[j]);
	}
}
#endif

#ifdef ENABLE_SKILL_RENEWAL
const DWORD* CHARACTER_MANAGER::GetUsableSkillList(BYTE bJob, BYTE bSkillGroup) const
{
	if (bSkillGroup == 0)
		return NULL;

	static const DWORD SkillList[JOB_MAX_NUM][SKILL_GROUP_MAX_NUM][6] =
	{
		{ {	1,	2,	3,	4,	5,	6	}, {	16,	17,	18,	19,	20,	21	} },
		{ {	31,	32,	33,	34,	35,	36	}, {	46,	47,	48,	49,	50,	51	} },
		{ {	61,	62,	63,	64,	65,	66	}, {	76,	77,	78,	79,	80,	81	} },
		{ {	91,	92,	93,	94,	95,	96	}, {	106,107,108,109,110,111	} },
		#ifdef ENABLE_WOLFMAN_CHARACTER
		{ {	170,171,172,173,174,175	}, {	0,	0,	0,	0,	0,	0	} },
		#endif
	};
	return SkillList[bJob][bSkillGroup - 1];
}
#endif

#ifdef __BACK_DUNGEON__
int CHARACTER_MANAGER::GetBackDungeonTime(const DWORD dwMapIdx)
{
	switch (dwMapIdx)
	{
	case 206:
		return 60 * 60;
	}

	return 60 * 5;
}

void CHARACTER_MANAGER::SendBackDungeonInfo(LPCHARACTER ch)
{
	if (ch->GetDungeon())
		return;

	const DWORD dwPID = ch->GetPlayerID();
	const int now = time(0);
	for (itertype(m_mapBackDungeon) it = m_mapBackDungeon.begin(); it != m_mapBackDungeon.end(); ++it)
	{
		for (itertype(it->second) itEx = it->second.begin(); itEx != it->second.end(); ++itEx)
		{
			itertype(itEx->second) itVec = itEx->second.begin();
			while (itVec != itEx->second.end())
			{
				if (itVec->iLeftTime <= now)
				{
					SetBackDungeon(true, false, it->first, itVec->bChannel, itEx->first, itVec->dwPID);
					itVec = itEx->second.begin();
				}
				else
				{
					if (itVec->dwPID == dwPID)
						ch->ChatPacket(CHAT_TYPE_COMMAND, "ReJoinDungeonDialog %u %u %u %d", it->first, itVec->bChannel, itEx->first, itVec->iLeftTime - now);
					++itVec;
				}
			}
		}
	}
}

void CHARACTER_MANAGER::WarpBackDungeon(LPCHARACTER ch, const WORD wPort, const BYTE bChannel, const DWORD dwMapIdx)
{
	if (ch->GetDungeon())
		return;

	const DWORD dwPID = ch->GetPlayerID();
	const auto it = m_mapBackDungeon.find(wPort);
	if (it != m_mapBackDungeon.end())
	{
		const auto itEx = it->second.find(dwMapIdx);
		if (itEx != it->second.end())
		{
			itertype(itEx->second) itVec = itEx->second.begin();
			while (itVec != itEx->second.end())
			{
				if (itVec->dwPID == dwPID && itVec->bChannel == bChannel)
				{
					if (itVec->iLeftTime <= time(0))
						ch->ChatPacket(CHAT_TYPE_INFO, "Dungeon back time left.");
					else
						ch->WarpSet(itVec->lX, itVec->lY, dwMapIdx, wPort);
					SetBackDungeon(true, false, it->first, itVec->bChannel, itEx->first, itVec->dwPID);
					return;
				}

				++itVec;
			}
		}
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "Can find this request.");
}

void CHARACTER_MANAGER::SetBackDungeon(const bool isP2P, const bool bIsSave, const WORD wPort, const BYTE bChannel, const DWORD dwMapIdx, const DWORD dwPID, const long lX, const long lY, int iLeftTime)
{
	if (bIsSave)
	{
		if (isP2P)
		{
			iLeftTime = time(0) + GetBackDungeonTime(dwMapIdx / 10000);

			LPDUNGEON dungeon = CDungeonManager::Instance().FindByMapIndex(dwMapIdx);
			if (!dungeon || dungeon->GetFlag("can_back_dungeon") != 1)
				return;

			if (dungeon->GetPlayerCount() == 1)
				dungeon->SetFlag("back_dungeon_destroy", iLeftTime + 5);
		}

		auto it = m_mapBackDungeon.find(wPort);
		if (it == m_mapBackDungeon.end())
		{
			std::map<DWORD, std::vector<SBackDungeon>> m_map;
			m_mapBackDungeon.emplace(wPort, m_map);
			it = m_mapBackDungeon.find(wPort);
		}

		auto itEx = it->second.find(dwMapIdx);
		if (itEx == it->second.end())
		{
			const std::vector<SBackDungeon> m_vec{SBackDungeon(dwPID, lX, lY, iLeftTime, bChannel)};
			it->second.emplace(dwMapIdx, m_vec);
		}
		else
		{
			bool isHas = false;
			itertype(itEx->second) itVec = itEx->second.begin();
			while (itVec != itEx->second.end())
			{
				if (itVec->dwPID == dwPID && itVec->bChannel == bChannel)
				{
					itVec->lX = lX;
					itVec->lY = lY;
					itVec->iLeftTime = iLeftTime;
					isHas = true;
					break;
				}
				itVec++;
			}

			if (!isHas)
				itEx->second.emplace_back(SBackDungeon(dwPID, lX, lY, iLeftTime, bChannel));
		}
	}
	else
	{
		bool isHas = false;

		auto it = m_mapBackDungeon.find(wPort);
		if (it != m_mapBackDungeon.end())
		{
			auto itEx = it->second.find(dwMapIdx);
			if (itEx != it->second.end())
			{
				itertype(itEx->second) itVec = itEx->second.begin();
				while (itVec != itEx->second.end())
				{
					if (dwPID == 0)
					{
						itVec = itEx->second.erase(itVec);
						continue;
					}
					else if (itVec->dwPID == dwPID && itVec->bChannel == bChannel)
					{
						isHas = true;
						itEx->second.erase(itVec);
						break;
					}
					itVec++;
				}
			}
		}

		if (isP2P && !isHas && dwPID > 0)
			return;
	}

	if (isP2P)
	{
		TPacketGGBackDungeon p;
		p.header = HEADER_GG_BACK_DUNGEON;
		p.bIsSave = bIsSave;
		p.wPort = wPort;
		p.bChannel = bChannel;
		p.dwMapIdx = dwMapIdx;
		p.dwPID = dwPID;
		p.lX = lX;
		p.lY = lY;
		p.iLeftTime = iLeftTime;
		P2P_MANAGER::Instance().Send(&p, sizeof(TPacketGGBackDungeon));
	}
}
#endif

#ifdef ENABLE_COSTUME_SET_BONUS
void CHARACTER_MANAGER::InitializeCostumeSetBonus()
{
	m_vecCostumeSetBonus.clear();

	int iID = 0;
	DWORD dwBody = 0;
	DWORD dwHead = 0;
	DWORD dwWeapon = 0;
	DWORD dwPet = 0;
	TPlayerItemAttribute aAttr[5];

	char szQuery[8192];
	snprintf(szQuery, sizeof(szQuery), 
		"SELECT id, body, head, weapon, pet"
		", attrtype0, attrvalue0"
		", attrtype1, attrvalue1"
		", attrtype2, attrvalue2"
		", attrtype3, attrvalue3"
		", attrtype4, attrvalue4"
		" FROM server_settings.costume_set_bonus;"
	);
	auto pkMsg(DBManager::instance().DirectQuery(szQuery));
	if (!pkMsg)
	{
		sys_err("Initialize::szQuery:!pkMsg");
		return;
	}

	SQLResult * pRes = pkMsg->Get();
	if (pRes->uiNumRows != 0)
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL)
		{
			int con = 0;

			iID = atoi(row[con++]);
			dwBody = atoi(row[con++]);
			dwHead = atoi(row[con++]);
			dwWeapon = atoi(row[con++]);
			dwPet = atoi(row[con++]);

			for (BYTE at = 0; at < 5; at++)
			{
				aAttr[at].bType = atoi(row[con++]);
				aAttr[at].sValue = atoi(row[con++]);
			}

			m_vecCostumeSetBonus.emplace_back(SCostumeSetBonus(iID, dwBody, dwHead, dwWeapon, dwPet, aAttr));
		}
	}
}

void CHARACTER_MANAGER::CheckCostumeSetBonus(LPCHARACTER ch)
{
	if (!ch || !ch->GetDesc()) { return; }
	if (m_vecCostumeSetBonus.empty()) { return; }

	DWORD dwBody = 0;
	DWORD dwHead = 0;
	DWORD dwWeapon = 0;
	DWORD dwPet = 0;

	LPITEM pkBody = ch->GetWear(WEAR_COSTUME_BODY);
	LPITEM pkHear = ch->GetWear(WEAR_COSTUME_HAIR);
	LPITEM pkWeapon = ch->GetWear(WEAR_COSTUME_WEAPON);
	LPITEM pkPet = ch->GetWear(WEAR_COSTUME_PET);

	if (pkBody && pkHear && pkWeapon && pkPet)
	{
		dwBody = pkBody->GetVnum();
		dwHead = pkHear->GetVnum();
		dwWeapon = pkWeapon->GetVnum();
		dwPet = pkPet->GetVnum();
	}
	else
	{
		if (ch->FindAffect(AFFECT_COSTUME_SET_BONUS))
		{
			ch->RemoveAffect(AFFECT_COSTUME_SET_BONUS);
		}
		return;
	}

	for (itertype(m_vecCostumeSetBonus) it = m_vecCostumeSetBonus.begin(); it != m_vecCostumeSetBonus.end(); it++)
	{
		if ((it->dwBody == dwBody) && (it->dwHead == dwHead) && (it->dwWeapon == dwWeapon) && (it->dwPet == dwPet))
		{
			if (!ch->FindAffect(AFFECT_COSTUME_SET_BONUS))
			{
				ch->AddAffect(AFFECT_COSTUME_SET_BONUS, POINT_NONE, 0, it->iID, INFINITE_AFFECT_DURATION, 0, true);
			}
			return;
		}
	}

	if (ch->FindAffect(AFFECT_COSTUME_SET_BONUS))
	{
		ch->RemoveAffect(AFFECT_COSTUME_SET_BONUS);
	}
}

void CHARACTER_MANAGER::GiveCostumeSetBonus(LPCHARACTER ch, int iID, bool bAdd)
{
	if (!ch || !ch->GetDesc()) { return; }
	if (m_vecCostumeSetBonus.empty()) { return; }

	for (itertype(m_vecCostumeSetBonus) it = m_vecCostumeSetBonus.begin(); it != m_vecCostumeSetBonus.end(); it++)
	{
		if (it->iID == iID)
		{
			for (BYTE at = 0; at < 5; at++)
			{
				BYTE bType = it->aAttr[at].bType; short sValue = it->aAttr[at].sValue;
				if ((bType > 0) && (sValue > 0))
				{
					ch->PointChange(aApplyInfo[bType].bPointType, bAdd ? sValue : -sValue);
					// ch->ChatPacket(CHAT_TYPE_INFO, "SetBonus: T[%u]-V[%d]", bType, sValue);
				}
			}
			break;
		}
	}
}
#endif

#ifdef ENABLE_REWARD_SYSTEM
const char* GetRewardIndexToString(BYTE rewardIndex)
{
	std::map<BYTE, std::string> rewardNames = {
		{REWARD_LEVEL, "REWARD_LEVEL"},
		{REWARD_REFINE, "REWARD_REFINE"},
		{REWARD_CRAFT, "REWARD_CRAFT"},
		{REWARD_AVERAGE, "REWARD_AVERAGE"},
		{REWARD_BIOLOG, "REWARD_BIOLOG"},
		{REWARD_ALIGNMENT, "REWARD_ALIGNMENT"},
		{REWARD_PASSIVE_SKILL, "REWARD_PASSIVE_SKILL"},
		{REWARD_STONE_DEAD, "REWARD_STONE_DEAD"},
		{REWARD_BOSS_DEAD, "REWARD_BOSS_DEAD"},
		{REWARD_STONE_ALL_DEAD, "REWARD_STONE_ALL_DEAD"},
		{REWARD_BOSS_ALL_DEAD, "REWARD_BOSS_ALL_DEAD"},
	};

	auto it = rewardNames.find(rewardIndex);
	if (it != rewardNames.end())
		return it->second.c_str();

	return 0;
}

BYTE GetRewardIndex(const char* szRewardName)
{
	std::map<std::string, BYTE> rewardNames = {
		{"REWARD_LEVEL", REWARD_LEVEL},
		{"REWARD_REFINE", REWARD_REFINE},
		{"REWARD_CRAFT", REWARD_CRAFT},
		{"REWARD_AVERAGE", REWARD_AVERAGE},
		{"REWARD_BIOLOG", REWARD_BIOLOG},
		{"REWARD_ALIGNMENT", REWARD_ALIGNMENT},
		{"REWARD_PASSIVE_SKILL", REWARD_PASSIVE_SKILL},
		{"REWARD_STONE_DEAD", REWARD_STONE_DEAD},
		{"REWARD_BOSS_DEAD", REWARD_BOSS_DEAD},
		{"REWARD_STONE_ALL_DEAD", REWARD_STONE_ALL_DEAD},
		{"REWARD_BOSS_ALL_DEAD", REWARD_BOSS_ALL_DEAD},
	};

	auto it = rewardNames.find(szRewardName);
	if (it != rewardNames.end())
		return it->second;

	return 0;
}

void CHARACTER_MANAGER::LoadRewardData()
{
	m_rewardLoadStatus = true;

	m_rewardData.clear();
	m_rewardStoneList.clear();
	m_rewardBossList.clear();

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), 
		"SELECT rewardIndex, deskIndex, lc_text, playerName, value, count, "
		"itemVnum0, itemCount0, "
		"itemVnum1, itemCount1, "
		"itemVnum2, itemCount2"
		" FROM server_settings.reward_table ORDER BY deskIndex ASC;"
	);
	auto pMsg(DBManager::instance().DirectQuery(szQuery));

	if (pMsg->Get()->uiNumRows != 0)
	{
		MYSQL_ROW row = NULL;
		while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)) != NULL)
		{
			int col = 0;

			char rewardName[50];
			strlcpy(rewardName, row[col++], sizeof(rewardName));
			BYTE rewardIndex = GetRewardIndex(rewardName);

			TRewardInfo p;
			p.rewardIndex = rewardIndex;
			str_to_number(p.deskIndex, row[col++]);
			strlcpy(p.lc_text, row[col++], sizeof(p.lc_text));
			strlcpy(p.playerName, row[col++], sizeof(p.playerName));
			str_to_number(p.value, row[col++]);
			str_to_number(p.count, row[col++]);

			sys_log(0, "RewardLoad: Index: %u - Value: %u - Count: %u", p.deskIndex, p.value, p.count);

			p.m_rewardItems.clear();
			for (BYTE j = 0; j < 3; ++j)
			{
				DWORD itemVnum, itemCount;
				str_to_number(itemVnum, row[col++]);
				str_to_number(itemCount, row[col++]);
				p.m_rewardItems.emplace_back(itemVnum, itemCount);
			}

			m_rewardData.emplace_back(p);

			if (strlen(p.playerName) < 1)
			{
				if (rewardIndex == REWARD_STONE_DEAD) { m_rewardStoneList.emplace_back(p.value, p.count); }
				else if (rewardIndex == REWARD_BOSS_DEAD) { m_rewardBossList.emplace_back(p.value, p.count); }
			}
		}
	}

	m_rewardLoadStatus = false;
}

void CHARACTER_MANAGER::SetRewardStone(DWORD dwRaceNum, int iCount)
{
	if (m_rewardLoadStatus) { return; }
	if (dwRaceNum <= 0) { return; }
	if (m_rewardStoneList.empty()) { return; }
	for (auto iter : m_rewardStoneList)
	{
		if (iter.first == dwRaceNum)
		{
			iter.second = iCount;
			break;
		}
	}
}

int CHARACTER_MANAGER::GetRewardStone(DWORD dwRaceNum)
{
	if (m_rewardLoadStatus) { return 0; }
	if (dwRaceNum <= 0) { return 0; }
	if (m_rewardStoneList.empty()) { return 0; }
	for (auto iter : m_rewardStoneList) { if (iter.first == dwRaceNum) { return iter.second; } }
	return 0;
}

void CHARACTER_MANAGER::SetRewardBoss(DWORD dwRaceNum, int iCount)
{
	if (m_rewardLoadStatus) { return; }
	if (dwRaceNum <= 0) { return; }
	if (m_rewardBossList.empty()) { return; }
	for (auto iter : m_rewardBossList)
	{
		if (iter.first == dwRaceNum)
		{
			iter.second = iCount;
			break;
		}
	}
}

int CHARACTER_MANAGER::GetRewardBoss(DWORD dwRaceNum)
{
	if (m_rewardLoadStatus) { return 0; }
	if (dwRaceNum <= 0) { return 0; }
	if (m_rewardBossList.empty()) { return 0; }
	for (auto iter : m_rewardBossList) { if (iter.first == dwRaceNum) { return iter.second; } }
	return 0;
}

bool CHARACTER_MANAGER::IsRewardEmpty(BYTE rewardIndex, DWORD value)
{
	if (m_rewardLoadStatus) { return false; }
	if (m_rewardData.empty()) { return false; }

	auto it = std::find_if(m_rewardData.begin(), m_rewardData.end(), [&](const TRewardInfo & sR) { return ((sR.rewardIndex == rewardIndex) && (sR.value == value)); });
	if (it != m_rewardData.end())
	{
		if (strcmp(it->playerName, "") == 0)
			return true;
	}

	return false;
}

void CHARACTER_MANAGER::SendRewardInfo(LPCHARACTER ch)
{
	if (m_rewardLoadStatus) { return; }

	if (!ch || !ch->GetDesc()) { return; }
	ch->SetProtectTime("RewardInfo", 1);
	if (m_rewardData.empty()) { return; }

	std::vector<std::pair<BYTE, std::string>> vec_send;
	vec_send.clear();

	for (auto it : m_rewardData)
	{
		if (strlen(it.playerName) > 1)
		{
			vec_send.emplace_back(it.deskIndex, it.playerName);
		}
	}

	if (vec_send.size() > 0)
	{
		TPacketGCRewardMain pack;
		pack.bHeader = HEADER_GC_REWARD_INFO;
		pack.wSize = sizeof(pack) + sizeof(TPacketRewardInfo)*vec_send.size();

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));

		for (auto iter : vec_send)
		{
			TPacketRewardInfo info;
			info.deskIndex = iter.first;
			strlcpy(info.playerName, iter.second.c_str(), sizeof(info.playerName));
			buff.write(&info, sizeof(info));
		}

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	vec_send.clear();
}

struct RewardForEach
{
	void operator() (LPDESC d)
	{
		LPCHARACTER ch = d->GetCharacter();
		if (ch == NULL)
			return;
		else if (ch->GetProtectTime("RewardInfo") != 1)
			return;

		CHARACTER_MANAGER::Instance().SendRewardInfo(ch);
	}
};

void CHARACTER_MANAGER::SetRewardData2(BYTE rewardIndex, const char* playerName, bool isP2P, DWORD value, DWORD count)
{
	if (m_rewardLoadStatus) { return; }
	if (m_rewardData.empty()) { return; }
	for (const auto& iter : m_rewardData)
	{
		if ((iter.rewardIndex == rewardIndex) && (value >= iter.value))
		{
			CHARACTER_MANAGER::Instance().SetRewardData(rewardIndex, playerName, isP2P, iter.value, count);
		}
	}
}

void CHARACTER_MANAGER::SetRewardData(BYTE rewardIndex, const char* playerName, bool isP2P, DWORD value, DWORD count)
{
	if (m_rewardLoadStatus) { return; }

	//Gm-Control//
	if (isP2P)
	{
		if ((playerName != nullptr) && strlen(playerName) > 1)
		{
			LPCHARACTER pkGm = FindPC(playerName);
			if (pkGm) { if (pkGm->IsGM()) { return; } }
		}
	}
	//Gm-Control//

	if (!IsRewardEmpty(rewardIndex, value))
		return;

	if (m_rewardData.empty())
		return;

	auto it = std::find_if(m_rewardData.begin(), m_rewardData.end(), [&](const TRewardInfo & sR) { return ((sR.rewardIndex == rewardIndex) && (sR.value == value)); });
	if (it == m_rewardData.end())
		return;

	if ((it->count > 0) && isP2P)
	{
		if (it->count != count)
			return;
	}

	if (strcmp(it->playerName, ""))
		return;

	if (rewardIndex == REWARD_STONE_DEAD)
	{
		SetRewardStone(value, 0);
	}
	else if (rewardIndex == REWARD_BOSS_DEAD)
	{
		SetRewardBoss(value, 0);
	}

	TRewardInfo& rewardInfo = *it;
	strlcpy(rewardInfo.playerName, playerName, sizeof(rewardInfo.playerName));

	if (isP2P)
	{
		LPCHARACTER ch = FindPC(playerName);
		if (ch)
		{
			for (DWORD j = 0; j < rewardInfo.m_rewardItems.size(); ++j)
			{
				DWORD dwVnum = rewardInfo.m_rewardItems[j].first;
				DWORD dwCount = rewardInfo.m_rewardItems[j].second;

				TItemTable * pTable = ITEM_MANAGER::instance().GetTable(dwVnum);
				if (pTable && !(pTable->dwFlags & ITEM_FLAG_STACKABLE))
				{
					for (DWORD rCount = 0; rCount < dwCount; rCount++)
						ch->AutoGiveItem(dwVnum, 1);
				}
				else
				{
					ch->AutoGiveItem(dwVnum, dwCount);
				}
			}
		}

		TPacketGGRewardInfo p;
		p.bHeader = HEADER_GG_REWARD_INFO;
		p.rewardIndex = rewardIndex;
		strlcpy(p.playerName, playerName, sizeof(p.playerName));
		p.value = value;
		P2P_MANAGER::Instance().Send(&p, sizeof(p));

		char msg[CHAT_MAX_LEN+1];
		switch (rewardIndex)
		{
			case REWARD_LEVEL:
			case REWARD_ALIGNMENT:
			case REWARD_STONE_ALL_DEAD:
			case REWARD_BOSS_ALL_DEAD:
			{
				///---------------------///
				snprintf(msg, sizeof(msg), LC_TEXT(rewardInfo.lc_text), (int)(value), playerName);
				BroadcastNotice(msg);
				///---------------------///
			}
			break;

			case REWARD_REFINE:
			case REWARD_CRAFT:
			{
				TItemTable * pTable = ITEM_MANAGER::instance().GetTable(value);
				if (pTable)
				{
					///---------------------///
					snprintf(msg, sizeof(msg), LC_TEXT(rewardInfo.lc_text), pTable->szLocaleName, playerName);
					BroadcastNotice(msg);
					///---------------------///
				}
			}
			break;

			case REWARD_STONE_DEAD:
			case REWARD_BOSS_DEAD:
			{
				const CMob * pkMob = CMobManager::instance().Get(value);
				if (pkMob)
				{
					///---------------------///
					snprintf(msg, sizeof(msg), LC_TEXT(rewardInfo.lc_text), (int)(count), pkMob->m_table.szLocaleName, playerName);
					BroadcastNotice(msg);
					///---------------------///
				}
			}
			break;

			default:
			{
				///---------------------///
				snprintf(msg, sizeof(msg), LC_TEXT(rewardInfo.lc_text), playerName);
				BroadcastNotice(msg);
				///---------------------///
			}
			break;
		}

		char szQuery[1024];
		snprintf(szQuery, sizeof(szQuery), "UPDATE server_settings.reward_table SET playerName = '%s' WHERE deskIndex = '%d';", playerName, rewardInfo.deskIndex);
		DBManager::instance().DirectQuery(szQuery);
	}

	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), RewardForEach());
}
#endif
