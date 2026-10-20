#include "stdafx.h"

#ifdef ENABLE_RONARK_SYSTEM
#include "questlua.h"
#include "questmanager.h"
#include "desc_client.h"
#include "char.h"
#include "char_manager.h"
#include "utils.h"
#include "guild.h"
#include "guild_manager.h"
#include "ronark_map.h"
#include "config.h"
#include "p2p.h"

namespace quest
{
	ALUA(ronark_start)
	{
		int iStatus = (int)lua_tonumber(L, 1);
		int iMap = (int)lua_tonumber(L, 2);
		CRonarkMapManager::instance().SetRonarkStatus(iStatus, iMap);
		return 0;
	}

	ALUA(ronark_get_status)
	{
		lua_pushnumber(L, CRonarkMapManager::instance().GetRonarkStatus());
		return 1;
	}

	ALUA(ronark_set_member_limit)
	{
		int iMember = (int)lua_tonumber(L, 1);
		CRonarkMapManager::instance().SetMemberLimitCount(iMember);
		return 0;
	}

	ALUA(ronark_get_total_member_count)
	{
		lua_pushnumber(L, CRonarkMapManager::instance().GetTotalMemberCount());
		return 1;
	}

	ALUA(ronark_get_before_war_list)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			lua_pushnumber(L, 1);
			return 1;
		}

		DWORD dwPID = ch->GetPlayerID();
		if (!m_globalRonarkWarMemberList.empty())
		{
			auto iter = m_globalRonarkWarMemberList.find(dwPID);
			if (iter != m_globalRonarkWarMemberList.end())
			{
				lua_pushnumber(L, 1);
				return 1;
			}
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	ALUA(ronark_set_before_war_list)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch) { return 0; }

		DWORD dwPID = ch->GetPlayerID();
		if (!m_globalRonarkWarMemberList.empty())
		{
			auto iter = m_globalRonarkWarMemberList.find(dwPID);
			if (iter != m_globalRonarkWarMemberList.end())
			{
				return 0;
			}
		}
		m_globalRonarkWarMemberList.emplace(dwPID);

		TPacketGGRonark p;
		p.bProcess = 1;
		p.dwPID = dwPID;
		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGRonark));
		return 0;
	}

	ALUA(ronark_remove_before_war_list)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		if (!lua_isnumber(L, 1))
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		DWORD dwPID = (DWORD)lua_tonumber(L, 1);

		if (!m_globalRonarkWarMemberList.empty())
		{
			auto iter = m_globalRonarkWarMemberList.find(dwPID);
			if (iter != m_globalRonarkWarMemberList.end())
			{
				m_globalRonarkWarMemberList.erase(dwPID);

				TPacketGGRonark p;
				p.bProcess = 2;
				p.dwPID = dwPID;
				P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGRonark));

				lua_pushnumber(L, 1);
				return 1;
			}
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	ALUA(ronark_clear_before_war_list)
	{
		TPacketGGRonark p;
		p.bProcess = 3;
		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGRonark));
		m_globalRonarkWarMemberList.clear();
		lua_pushnumber(L, 1);
		return 1;
	}

	ALUA(ronark_destroy)
	{
		CRonarkMapManager::instance().ResetEvent();
		return 0;
	}

	ALUA(ronark_get_winner_guild)
	{
		lua_pushnumber(L, CRonarkMapManager::instance().GetWinnerGuild());
		return 1;
	}

	ALUA(ronark_izle)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if(!ch)
		{
			return 0;
		}

		if (ch->GetEmpire() == 1)
		{
			ch->WarpSet(20974, 55767, 0);
		}
		else if (ch->GetEmpire() == 2)
		{
			ch->WarpSet(46926, 45679, 0);
		}
		else if (ch->GetEmpire() == 3)
		{
			ch->WarpSet(45104, 83323, 0);
		}

		ch->SetQuestFlag("ronark.izleyici", 1);
		ch->SetObserverMode(false);
		return 0;
	}

	void RegisterRonarkFunctionTable()
	{
		luaL_reg ronark_functions[] =
		{
			{	"start",					ronark_start					},
			{	"get_status",				ronark_get_status				},
			{	"set_member_limit",			ronark_set_member_limit			},
			{	"get_total_member_count",	ronark_get_total_member_count	},
			{	"get_before_war_list",		ronark_get_before_war_list		},
			{	"set_before_war_list",		ronark_set_before_war_list		},
			{	"remove_before_war_list",	ronark_remove_before_war_list	},
			{	"clear_before_war_list",	ronark_clear_before_war_list	},
			{	"get_winner_guild",			ronark_get_winner_guild			},
			{	"izle",						ronark_izle						},
			{	"destroy",					ronark_destroy					},
			{	NULL,						NULL							}
		};

		CQuestManager::instance().AddLuaFunctionTable("ronark", ronark_functions);
	}
}
#endif
