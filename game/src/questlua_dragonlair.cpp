#include "stdafx.h"

#include "questmanager.h"
#include "DragonLair.h"
#include "char.h"
#include "guild.h"

#ifdef ENABLE_DRAGONLAIR_DUNGEON
WORD dragonState = 1;
DWORD dragonPass = 0;
std::set<DWORD> m_registeredPlayers;
#endif

namespace quest
{
	ALUA(dl_startRaid)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		long baseMapIndex = lua_tonumber(L, -1);

		CDragonLairManager::instance().Start(ch->GetMapIndex(), baseMapIndex, ch->GetGuild()->GetID());

		return 0;
	}

#ifdef ENABLE_DRAGONLAIR_DUNGEON
	ALUA(dl_set_state)
	{
		dragonState = (WORD)lua_tonumber(L, 1);
		m_registeredPlayers.clear();
		return 0;
	}
	ALUA(dl_get_state)
	{
		lua_pushnumber(L, dragonState);
		return 1;
	}
	ALUA(dl_set_pass)
	{
		dragonPass = (DWORD)lua_tonumber(L, 1);

		return 0;
	}
	ALUA(dl_get_pass)
	{
		lua_pushnumber(L, dragonPass);
		return 1;
	}
	ALUA(dl_register_user)
	{
		m_registeredPlayers.insert(lua_tonumber(L, 1));

		return 0;
	}
	ALUA(dl_is_registered_user)
	{
		lua_pushboolean(L, std::find(m_registeredPlayers.begin(), m_registeredPlayers.end(), lua_tonumber(L, 1)) != m_registeredPlayers.end());

		return 1;
	}
#endif

	void RegisterDragonLairFunctionTable()
	{
		luaL_reg dl_functions[] =
		{
			{	"startRaid",	dl_startRaid	},
#ifdef ENABLE_DRAGONLAIR_DUNGEON
			{	"get_state",	dl_get_state	},
			{	"set_state",	dl_set_state	},
			{	"get_pass",		dl_get_pass	},
			{	"set_pass",		dl_set_pass	},
			{	"register_user",			dl_register_user	},
			{	"is_registered_user",		dl_is_registered_user	},
#endif
			{	NULL,			NULL			}
		};

		CQuestManager::instance(). AddLuaFunctionTable("DragonLair", dl_functions);
	}
}

