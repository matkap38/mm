#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "log.h"
#include "char.h"
#include "locale_service.h"
#include "item.h"
#include "item_manager.h"
#include "questmanager.h"
#include <sstream>
#include "packet.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "db.h"
#include "p2p.h"
#include "fstream"
#include "GetEventFlagSystem.h"
#include "LoadTxtToTable.h"
#include "config.h"

void LoadTheSystems()
{
	#ifdef ENABLE_ALIGNMENT_BONUS
	LoadAlignmentBonus();
	#endif

	#ifdef ENABLE_PLAYER_TAG_SYSTEM
	LoadPlayerTags();
	#endif

	#ifdef ENABLE_SPECIAL_STORAGE
	InitializeUpgradeItems();
	InitializeBookItems();
	InitializeStoneItems();
	InitializeChestItems();
	#endif

	#ifdef __AUTO_HUNT__
	InitializeAutoHuntMaps();
	#endif

	#ifdef ENABLE_DEFENCE_SHIELD
	InitializeDefenceShieldMaps();
	#endif

	#ifdef ENABLE_PCBAN_SYSTEM
	LoadPcBanKey();
	#endif
}

void LoadTheDbSystems()
{
	#ifdef ENABLE_PUBLISHER_SYSTEM
	InitializePublisherSystem();
	#endif

	#ifdef BOSS_DMG_RANKING
	if (!Boss_Dmg_Ranking_Load())
	{
		sys_err("Boss_Dmg_Ranking_Load: Cannot!");
	}
	#endif
}

#ifdef ENABLE_ALIGNMENT_BONUS
std::vector<TAlignmentTable> m_mapAlignmentList;

void LoadAlignmentBonus()
{
	sys_log(0, "LoadAlignmentBonus: Loading.");

	/********************************************************************************************************************/
	char query[8192];
	snprintf(query, sizeof(query), 
		"SELECT level, point, "
		"type0, value0, "
		"type1, value1, "
		"type2, value2 "
		"FROM server_settings.alignment;"
	);

	auto pkMsg(DBManager::instance().DirectQuery(query));
	if (!pkMsg)
	{
		sys_err("LoadAlignmentBonus::query:!pkMsg");
		return;
	}

	m_mapAlignmentList.clear();

	SQLResult * pRes = pkMsg->Get();
	if (pRes->uiNumRows != 0)
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL)
		{
			int con = 0;

			TAlignmentTable V{};
			str_to_number(V.Level, row[con++]);
			str_to_number(V.Point, row[con++]);
			V.Point *= 10;

			for (BYTE x = 0; x < 3; x++)
			{
				str_to_number(V.Type[x], row[con++]);
				str_to_number(V.Value[x], row[con++]);
			}

			m_mapAlignmentList.emplace_back(V);
		}
	}
}
#endif

#ifdef ENABLE_PLAYER_TAG_SYSTEM
std::vector<TPlayerTags> SendPlayerTags;

void LoadPlayerTags()
{
	sys_log(0, "LoadPlayerTags: Refresh.");
	SendPlayerTags.clear();

	auto pkMsg(DBManager::instance().DirectQuery("SELECT pid, tag+0 FROM server_settings.player_tags"));
	if (pkMsg->Get()->uiNumRows != 0)
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pkMsg->Get()->pSQLResult)) != NULL)
		{
			TPlayerTags V{};
			str_to_number(V.Pid, row[0]);
			str_to_number(V.Tag, row[1]);
			SendPlayerTags.emplace_back(V);
		}
	}
}
#endif

#ifdef ENABLE_SPECIAL_STORAGE
///********************************************************///
TR1_NS::unordered_set<DWORD> m_setUpgradeItem;

void InitializeUpgradeItems()
{
	FILE		*fp;
	char		one_line[256];
	DWORD		dwItemVnum;
	const char	*delimit = " \t\r\n";
	char		*token_string;

	char file_name[256+1];
	snprintf(file_name, sizeof(file_name), "%s/systems/special_inventory/upgrade_inventory.txt", LocaleService_GetBasePath().c_str());
	fp = fopen(file_name, "r");

	m_setUpgradeItem.clear();

	while (fgets(one_line, 256, fp))
	{
		dwItemVnum = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delimit);
		if (NULL == token_string)
			continue;

		str_to_number(dwItemVnum, token_string);

		if (m_setUpgradeItem.find(dwItemVnum) != m_setUpgradeItem.end())
			continue;

		m_setUpgradeItem.insert(dwItemVnum);
	}
	fclose(fp);
}

bool CheckUpgradeItems(DWORD dwVnum)
{
	if (m_setUpgradeItem.empty())
		return false;

	if (m_setUpgradeItem.find(dwVnum) != m_setUpgradeItem.end())
		return true;

	return false;
}
///********************************************************///

///********************************************************///
TR1_NS::unordered_set<DWORD> m_setBookItem;

void InitializeBookItems()
{
	FILE 	*fp;
	char	one_line[256];
	DWORD	dwItemVnum;
	const char	*delimit = " \t\r\n";
	char		*token_string;

	char file_name[256+1];
	snprintf(file_name, sizeof(file_name), "%s/systems/special_inventory/book_inventory.txt", LocaleService_GetBasePath().c_str());
	fp = fopen(file_name, "r");

	m_setBookItem.clear();

	while (fgets(one_line, 256, fp))
	{
		dwItemVnum = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delimit);
		if (NULL == token_string)
			continue;

		str_to_number(dwItemVnum, token_string);

		if (m_setBookItem.find(dwItemVnum) != m_setBookItem.end())
			continue;

		m_setBookItem.insert(dwItemVnum);
	}
	fclose(fp);
}

bool CheckBookItems(DWORD dwVnum)
{
	if (m_setBookItem.empty())
		return false;

	if (m_setBookItem.find(dwVnum) != m_setBookItem.end())
		return true;

	return false;
}
///********************************************************///

///********************************************************///
TR1_NS::unordered_set<DWORD> m_setStoneItem;

void InitializeStoneItems()
{
	FILE		*fp;
	char		one_line[256];
	DWORD		dwItemVnum;
	const char	*delimit = " \t\r\n";
	char		*token_string;

	char file_name[256+1];
	snprintf(file_name, sizeof(file_name), "%s/systems/special_inventory/stone_inventory.txt", LocaleService_GetBasePath().c_str());
	fp = fopen(file_name, "r");

	m_setStoneItem.clear();

	while (fgets(one_line, 256, fp))
	{
		dwItemVnum = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delimit);
		if (NULL == token_string)
			continue;

		str_to_number(dwItemVnum, token_string);

		if (m_setStoneItem.find(dwItemVnum) != m_setStoneItem.end())
			continue;

		m_setStoneItem.insert(dwItemVnum);
	}
	fclose(fp);
}

bool CheckStoneItems(DWORD dwVnum)
{
	if (m_setStoneItem.empty())
		return false;

	if (m_setStoneItem.find(dwVnum) != m_setStoneItem.end())
		return true;

	return false;
}
///********************************************************///

///********************************************************///
TR1_NS::unordered_set<DWORD> m_setChestItem;

void InitializeChestItems()
{
	FILE		*fp;
	char		one_line[256];
	DWORD		dwItemVnum;
	const char	*delimit = " \t\r\n";
	char		*token_string;

	char file_name[256+1];
	snprintf(file_name, sizeof(file_name), "%s/systems/special_inventory/chest_inventory.txt", LocaleService_GetBasePath().c_str());
	fp = fopen(file_name, "r");

	m_setChestItem.clear();

	while (fgets(one_line, 256, fp))
	{
		dwItemVnum = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delimit);
		if (NULL == token_string)
			continue;

		str_to_number(dwItemVnum, token_string);

		if (m_setChestItem.find(dwItemVnum) != m_setChestItem.end())
			continue;

		m_setChestItem.insert(dwItemVnum);
	}
	fclose(fp);
}

bool CheckChestItems(DWORD dwVnum)
{
	if (m_setChestItem.empty())
		return false;

	if (m_setChestItem.find(dwVnum) != m_setChestItem.end())
		return true;

	return false;
}
///********************************************************///
#endif

#ifdef __AUTO_HUNT__
///********************************************************///
TR1_NS::unordered_set<long> m_setAutoHuntMap;

void InitializeAutoHuntMaps()
{
	FILE		*fp;
	char		one_line[256];
	long		lMapIndex;
	const char	*delimit = " \t\r\n";
	char		*token_string;

	char file_name[256+1];
	snprintf(file_name, sizeof(file_name), "%s/systems/auto_hunt/maps.txt", LocaleService_GetBasePath().c_str());
	fp = fopen(file_name, "r");

	m_setAutoHuntMap.clear();

	while (fgets(one_line, 256, fp))
	{
		lMapIndex = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delimit);
		if (NULL == token_string)
			continue;

		str_to_number(lMapIndex, token_string);

		if (m_setAutoHuntMap.find(lMapIndex) != m_setAutoHuntMap.end())
			continue;

		m_setAutoHuntMap.insert(lMapIndex);
	}
	fclose(fp);
}

bool CheckAutoHuntMaps(long lMapIndex)
{
	if (m_setAutoHuntMap.empty())
		return false;

	if (m_setAutoHuntMap.find(lMapIndex) != m_setAutoHuntMap.end())
		return true;

	return false;
}
///********************************************************///
#endif

#ifdef ENABLE_DEFENCE_SHIELD
///********************************************************///
TR1_NS::unordered_set<long> m_setDefenceShieldMap;

void InitializeDefenceShieldMaps()
{
	FILE		*fp;
	char		one_line[256];
	long		lMapIndex;
	const char	*delimit = " \t\r\n";
	char		*token_string;

	char file_name[256+1];
	snprintf(file_name, sizeof(file_name), "%s/systems/defence_shield/maps.txt", LocaleService_GetBasePath().c_str());
	fp = fopen(file_name, "r");

	m_setDefenceShieldMap.clear();

	while (fgets(one_line, 256, fp))
	{
		lMapIndex = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delimit);
		if (NULL == token_string)
			continue;

		str_to_number(lMapIndex, token_string);

		if (m_setDefenceShieldMap.find(lMapIndex) != m_setDefenceShieldMap.end())
			continue;

		m_setDefenceShieldMap.insert(lMapIndex);
	}
	fclose(fp);
}

bool CheckDefenceShieldMaps(long lMapIndex)
{
	if (m_setDefenceShieldMap.empty())
		return false;

	if (m_setDefenceShieldMap.find(lMapIndex) != m_setDefenceShieldMap.end())
		return true;

	return false;
}
///********************************************************///
#endif

#ifdef ENABLE_PCBAN_SYSTEM
int g_iPcBanKey = 0;
int g_iPcDefaultKey = 0;

void LoadPcBanKey()
{
	auto pkMsg(DBManager::instance().DirectQuery("SELECT ban, bdefault FROM server_settings.pc_ban_key;"));
	if (pkMsg->uiSQLErrno != 0)
	{
		sys_err("LoadPcBanKey Query Failed, Error code: %ld", pkMsg->uiSQLErrno);
		return;
	}

	if (!pkMsg->Get()->uiNumRows)
	{
		sys_err("LoadPcBanKey Query Failed, Rows couldn't get");
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(pkMsg->Get()->pSQLResult);
	str_to_number(g_iPcBanKey, row[0]);
	str_to_number(g_iPcDefaultKey, row[1]);
}
#endif

#ifdef ENABLE_URIEL_BAN
void CheckUrielBan()
{
	char query[8192];
	snprintf(query, sizeof(query), "SELECT name FROM account.urielbanlist WHERE status = '0';");

	auto pkMsg(DBManager::instance().DirectQuery(query));
	if (!pkMsg)
	{
		sys_err("CheckUrielBan::query:!pkMsg");
		return;
	}

	SQLResult * pRes = pkMsg->Get();
	if (pRes->uiNumRows != 0)
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL)
		{
			char szLogin[LOGIN_MAX_LEN + 1];
			strlcpy(szLogin, row[0], sizeof(szLogin));

			char szEsLogin[LOGIN_MAX_LEN * 2 + 1];
			DBManager::instance().EscapeString(szEsLogin, sizeof(szEsLogin), szLogin, strlen(szLogin));

			DBManager::instance().DirectQuery("UPDATE account.urielbanlist SET status = '1' WHERE name = '%s';", szEsLogin);
			DBManager::instance().DirectQuery("UPDATE account.account SET status = 'BAN' WHERE login = '%s';", szEsLogin);

			///---------------------------------------------------------------------------------------------------------////
			snprintf(query, sizeof(query), "SELECT id FROM account.account WHERE login = '%s';", szEsLogin);
			auto pkMsg1(DBManager::instance().DirectQuery(query));
			if (!pkMsg1) { continue; }

			SQLResult * pRes1 = pkMsg1->Get();
			if (pRes1->uiNumRows != 0)
			{
				MYSQL_ROW row1;
				while ((row1 = mysql_fetch_row(pRes1->pSQLResult)) != NULL)
				{
					DWORD dwAID = 0;
					str_to_number(dwAID, row1[0]);

					snprintf(query, sizeof(query), "SELECT name FROM player.player WHERE account_id = '%u';", dwAID);
					auto pkMsg2(DBManager::instance().DirectQuery(query));
					if (!pkMsg2) { continue; }

					SQLResult * pRes2 = pkMsg2->Get();
					if (pRes2->uiNumRows != 0)
					{
						MYSQL_ROW row2;
						while ((row2 = mysql_fetch_row(pRes2->pSQLResult)) != NULL)
						{
							char szName[CHARACTER_NAME_MAX_LEN + 1];
							strlcpy(szName, row2[0], sizeof(szName));

							LPDESC d = DESC_MANAGER::instance().FindByCharacterName(szName);
							LPCHARACTER tch = d ? d->GetCharacter() : NULL;

							if (!tch)
							{
								#ifdef ENABLE_DC_TO_CHR_OTHER_CH
								const CCI* pkCCI = P2P_MANAGER::instance().Find(szName);
								if (NULL != pkCCI)
								{
									if (pkCCI->pkDesc)
									{
										TPacketGGDcToChr p;
										strlcpy(p.szName, szName, sizeof(p.szName));
										pkCCI->pkDesc->Packet(&p, sizeof(TPacketGGDcToChr));
										continue;
									}
								}
								#endif
								continue;
							}

							DESC_MANAGER::instance().DestroyLoginKey(d);
							DESC_MANAGER::instance().DestroyDesc(d);
						}
					}
				}
			}
		}
	}
}
#endif

#ifdef ENABLE_PUBLISHER_SYSTEM
std::map<int, TPublisherInfo> m_mapPublisherInfo;

void InitializePublisherSystem(bool bReload)
{
	sys_log(0, "InitializePublisherSystem: Refresh.");

	/********************************************************************************************************************/
	char query[2048];
	snprintf(query, sizeof(query), "SELECT id, title, link FROM server_settings.publisher;");

	auto pkMsg(DBManager::instance().DirectQuery(query));
	if (!pkMsg)
	{
		sys_err("InitializePublisherSystem::query:!pkMsg");
		return;
	}

	SQLResult * pRes = pkMsg->Get();
	if (pRes->uiNumRows != 0)
	{
		m_mapPublisherInfo.clear();

		int iOrder = 1;

		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL)
		{
			int col = 0, iID = 0;

			str_to_number(iID, row[col++]);
			if (iID <= 0)
				continue;

			TPublisherInfo prt{};
			prt.iOrder = iOrder++;
			strlcpy(prt.szTitle, row[col++], sizeof(prt.szTitle));
			strlcpy(prt.szLink, row[col++], sizeof(prt.szLink));
			m_mapPublisherInfo.emplace(iID, prt);
		}
	}
	/********************************************************************************************************************/

	if (bReload)
	{
		int value = quest::CQuestManager::instance().GetEventFlag("publisher_enable");
		const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
		for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
		{
			LPCHARACTER ch = (*it)->GetCharacter();
			if (!ch) { continue; }
			ch->ChatPacket(CHAT_TYPE_COMMAND, "publisher_system %d", value);
			if (value > 0) { OnLoginPublisher(ch); }
		}
	}
}

void OnLoginPublisher(LPCHARACTER ch)
{
	if (quest::CQuestManager::instance().GetEventFlag("publisher_enable") == 0) { return; }
	if (!ch || !ch->GetDesc()) { return; }
	if (m_mapPublisherInfo.empty()) { return; }
	ch->ChatPacket(CHAT_TYPE_COMMAND, "publisher_system %d", int(1));
	std::vector<TPublisherInfo> mVecList; mVecList.clear();
	for (auto iter : m_mapPublisherInfo) { mVecList.emplace_back(iter.second); }
	TPacketGCPublisherInfo packet;
	packet.bHeader = HEADER_GC_PUBLISHER_INFO;
	packet.wSize = sizeof(packet) + sizeof(TPublisherInfo) * mVecList.size();
	ch->GetDesc()->BufferedPacket(&packet, sizeof(packet));
	ch->GetDesc()->Packet(&mVecList[0], sizeof(TPublisherInfo) * mVecList.size());
	mVecList.clear();
}
#endif

#ifdef BOSS_DMG_RANKING
std::map<DWORD, std::vector<std::pair<DWORD, short>>> m_mapBossDmgRankingReward;

bool Boss_Dmg_Ranking_Load()
{
	char file[256 + 1];
	snprintf(file, sizeof(file), "%s/systems/dmg_ranking/boss.txt", LocaleService_GetBasePath().c_str());

	FILE* fp;
	char	one_line[256];
	int		value1;
	int		value2;
	DWORD	raceNum = 0;

	const char* delim = " \t\r\n";
	char* v, * token_string;

	m_mapBossDmgRankingReward.clear();

	if (0 == file || 0 == file[0])
		return false;

	if ((fp = fopen(file, "r")) == 0)
		return false;

	while (fgets(one_line, 256, fp))
	{
		value1 = value2 = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delim);

		if (NULL == token_string)
			continue;

		// set value1, value2
		if ((v = strtok(NULL, delim)))
			str_to_number(value1, v);

		if ((v = strtok(NULL, delim)))
			str_to_number(value2, v);

		TOKEN("section")
		{
		}
		else TOKEN("boss")
		{
			raceNum = static_cast<DWORD>(value1);
		}
		else TOKEN("reward")
		{
			if (raceNum > 0)
			{
				m_mapBossDmgRankingReward[raceNum].emplace_back(static_cast<DWORD>(value1), static_cast<short>(value2));
			}
		}
		else TOKEN("end")
		{
		}
	}

	fclose(fp);
	return true;
}
#endif
