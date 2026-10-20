#include "stdafx.h"
#include "constants.h"
#include "gm.h"
#include "locale_service.h"
#include "config.h"

//ADMIN_MANAGER
std::map<std::string, tGM> g_map_GM;
void gm_new_clear()
{
	g_map_GM.clear();
}

void gm_new_insert(const tAdminInfo& rAdminInfo)
{
	sys_log(0, "InsertGMList(account:%s, player:%s, contact_ip:%s, server_ip:%s, auth:%d)",
		rAdminInfo.m_szAccount,
		rAdminInfo.m_szName,
		rAdminInfo.m_szContactIP,
		rAdminInfo.m_szServerIP,
		rAdminInfo.m_Authority);

	tGM t;

	if (rAdminInfo.m_szContactIP[0] == '\0')
	{
		sys_log(0, "GM Use ContactIP");
	}
	else
	{
		sys_log(0, "GM Use Default Host List");
	}

	memcpy(&t.Info, &rAdminInfo, sizeof(rAdminInfo));

	g_map_GM[rAdminInfo.m_szName] = t;
}



BYTE gm_new_get_level(const char* name, const char* account)
{
	if (test_server) return GM_IMPLEMENTOR;

	std::map<std::string, tGM >::iterator it = g_map_GM.find(name);

	if (g_map_GM.end() == it)
		return GM_PLAYER;

	if (account && strcmp(it->second.Info.m_szAccount, account) != 0)
	{
		sys_log(0, "GM_NEW_GET_LEVEL : BAD ACCOUNT [ACCOUNT:%s/%s", it->second.Info.m_szAccount, account);
		return GM_PLAYER;
	}

	return it->second.Info.m_Authority;
}

//END_ADMIN_MANAGER
BYTE gm_get_level(const char* name, const char* account)
{
	return gm_new_get_level(name, account);
}

