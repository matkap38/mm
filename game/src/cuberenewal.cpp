#define _cube_cpp_

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
#include "char_manager.h"

#ifdef ENABLE_BATTLE_PASS
#include "battle_pass.h"
#endif

static std::vector<CUBE_RENEWAL_DATA*>	s_cube_proto;
#ifdef CUBE_RENEWAL_RELOAD_FIX
static bool s_isInitializedCubeMaterialInformation = false;
#endif

typedef std::vector<CUBE_RENEWAL_VALUE> TCubeValueVector;

struct SCubeMaterialInfo
{
	SCubeMaterialInfo()
	{
		bHaveComplicateMaterial = false;
	};

	CUBE_RENEWAL_VALUE	reward;
	TCubeValueVector	material;

	#ifdef ENABLE_FULL_YANG
	int64_t				gold;
	#else
	int					gold;
	#endif

	int					percent;
	std::string			category;

	TCubeValueVector	complicateMaterial;

	std::string			infoText;
	bool				bHaveComplicateMaterial;
};

struct SItemNameAndLevel
{
	SItemNameAndLevel() { level = 0; }

	std::string		name;
	int				level;
};


typedef std::vector<SCubeMaterialInfo>								TCubeResultList;
typedef boost::unordered_map<DWORD, TCubeResultList>				TCubeMapByNPC;

TCubeMapByNPC cube_info_map;

static bool FN_check_valid_npc(WORD vnum)
{
	for (std::vector<CUBE_RENEWAL_DATA*>::iterator iter = s_cube_proto.begin(); iter != s_cube_proto.end(); iter++)
	{
		if (std::find((*iter)->npc_vnum.begin(), (*iter)->npc_vnum.end(), vnum) != (*iter)->npc_vnum.end())
			return true;
	}
	return false;
}

static bool FN_check_cube_data(CUBE_RENEWAL_DATA* cube_data)
{
	DWORD	i = 0;
	DWORD	end_index = 0;

	end_index = cube_data->npc_vnum.size();
	for (i = 0; i < end_index; ++i)
	{
		if (cube_data->npc_vnum[i] == 0)	return false;
	}

	end_index = cube_data->item.size();
	for (i = 0; i < end_index; ++i)
	{
		if (cube_data->item[i].vnum == 0)		return false;
		if (cube_data->item[i].count == 0)	return false;
	}

	end_index = cube_data->reward.size();
	for (i = 0; i < end_index; ++i)
	{
		if (cube_data->reward[i].vnum == 0)	return false;
		if (cube_data->reward[i].count == 0)	return false;
	}
	return true;
}

static int FN_check_cube_item_vnum_material(const SCubeMaterialInfo& materialInfo, int index)
{
	if (index <= materialInfo.material.size())
	{
		return materialInfo.material[index - 1].vnum;
	}
	return 0;
}

static int FN_check_cube_item_count_material(const SCubeMaterialInfo& materialInfo, int index)
{
	if (index <= materialInfo.material.size())
	{
		return materialInfo.material[index - 1].count;
	}
	return 0;
}

CUBE_RENEWAL_DATA::CUBE_RENEWAL_DATA()
{
	this->gold = 0;
	this->category = "WORLDARD";
}

#if defined(CUBE_RENEWAL_RELOAD_FIX)
void Cube_init(bool bReload)
#else
void Cube_init()
#endif
{
	CUBE_RENEWAL_DATA* p_cube = NULL;
	std::vector<CUBE_RENEWAL_DATA*>::iterator iter;

	char file_name[256 + 1];
	snprintf(file_name, sizeof(file_name), "%s/cube.txt", LocaleService_GetBasePath().c_str());

	sys_log(0, "Cube_Init %s", file_name);

	for (iter = s_cube_proto.begin(); iter != s_cube_proto.end(); iter++)
	{
		p_cube = *iter;
		M2_DELETE(p_cube);
	}

	s_cube_proto.clear();

	if (false == Cube_load(file_name))
		sys_err("Cube_Init failed");

	#if defined(CUBE_RENEWAL_RELOAD_FIX)
	if (bReload)
	{
		s_isInitializedCubeMaterialInformation = true;
		CubeReload();
	}
	#endif
}

#if defined(CUBE_RENEWAL_RELOAD_FIX)
void CubeReload()
{
	for (DESC_MANAGER::DESC_SET::const_iterator it = DESC_MANAGER::instance().GetClientSet().begin(); it != DESC_MANAGER::instance().GetClientSet().end(); ++it)
	{
		LPCHARACTER ch = (*it)->GetCharacter();
		if (ch && ch->IsCubeOpen())
		{
			Cube_close(ch);
			SendDateCubeRenewalPackets(ch, CUBE_RENEWAL_SUB_HEADER_DATES_RELOAD);
		}
	}

	cube_info_map.clear();
	Cube_InformationInitialize();
}
#endif

bool Cube_load(const char* file)
{
	FILE* fp;

	const char* value_string;

	char	one_line[256];
	#ifdef ENABLE_FULL_YANG
	int64_t value1;
	#else
	int value1;
	#endif
	int value2;

	const char* delim = " \t\r\n";
	char* v, * token_string;

	CUBE_RENEWAL_DATA* cube_data = NULL;
	CUBE_RENEWAL_VALUE	cube_value = {0,0};

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
		    value_string = v;

		if ((v = strtok(NULL, delim)))
			str_to_number(value2, v);

		TOKEN("section")
		{
			cube_data = M2_NEW CUBE_RENEWAL_DATA;
		}
		else TOKEN("npc")
		{
			cube_data->npc_vnum.push_back((WORD)value1);
		}
		else TOKEN("item")
		{
			cube_value.vnum = value1;
			cube_value.count = value2;
			cube_data->item.push_back(cube_value);
		}
		else TOKEN("reward")
		{
			cube_value.vnum = value1;
			cube_value.count = value2;
			cube_data->reward.push_back(cube_value);
		}
		else TOKEN("percent")
		{
			cube_data->percent = value1;
		}
		else TOKEN("category")
		{
			cube_data->category = value_string;
		}
		else TOKEN("gold")
		{
			cube_data->gold = value1;
		}
		else TOKEN("end")
		{
			// TODO : check cube data
			if (false == FN_check_cube_data(cube_data))
			{
				if (test_server)
					sys_log(0, "something wrong");

				M2_DELETE(cube_data);
				continue;
			}

			s_cube_proto.push_back(cube_data);
		}
	}

	fclose(fp);
	return true;
}

SItemNameAndLevel SplitItemNameAndLevelFromName(const std::string& name)
{
	int level = 0;
	SItemNameAndLevel info;
	info.name = name;

	size_t pos = name.find("+");

	if (std::string::npos != pos)
	{
		const std::string levelStr = name.substr(pos + 1, name.size() - pos - 1);
		str_to_number(level, levelStr.c_str());

		info.name = name.substr(0, pos);
	}

	info.level = level;
	return info;
};

bool Cube_InformationInitialize()
{
	if (s_cube_proto.size() <= 0)
		return false;

	for (int i = 0; i < s_cube_proto.size(); ++i)
	{
		CUBE_RENEWAL_DATA* cubeData = s_cube_proto[i];

		const std::vector<CUBE_RENEWAL_VALUE>& rewards = cubeData->reward;

		if (1 != rewards.size())
		{
			sys_err("[CubeInfo] WARNING! Does not support multiple rewards (count: %d)", rewards.size());
			continue;
		}

		const CUBE_RENEWAL_VALUE& reward = rewards.at(0);
		const WORD& npcVNUM = cubeData->npc_vnum.at(0);

		TCubeMapByNPC& cubeMap = cube_info_map;
		TCubeResultList& resultList = cubeMap[npcVNUM];
		SCubeMaterialInfo materialInfo;

		materialInfo.reward = reward;
		materialInfo.gold = cubeData->gold;
		materialInfo.percent = cubeData->percent;
		materialInfo.material = cubeData->item;
		materialInfo.category = cubeData->category;

		resultList.push_back(materialInfo);
	}

	#ifdef CUBE_RENEWAL_RELOAD_FIX
	if (s_isInitializedCubeMaterialInformation) { s_isInitializedCubeMaterialInformation = false; }
	#endif

	return true;
}

void Cube_open(LPCHARACTER ch, DWORD dwNpcRace)
{
	#ifdef CUBE_RENEWAL_RELOAD_FIX
	if (s_isInitializedCubeMaterialInformation) { return; }
	#endif
	if (NULL == ch || !ch) { return; }
	if (!ch->GetDesc()) { return; }

	DWORD npcVNUM = dwNpcRace;
	LPCHARACTER npc = NULL;

	if (npcVNUM == 0)
	{
		npc = ch->GetQuestNPC();
		if (!npc) { return; }
		npcVNUM = npc->GetRaceNum();
	}
	else
	{
		npc = ch;
		ch->SetCubeNpcRace(npcVNUM);
	}

	if (NULL == npc || !npc) { return; }
	if (FN_check_valid_npc(npcVNUM) == false) { return; }

	if (quest::CQuestManager::instance().GetEventFlag("cube_kapat") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "<Sistem> cube suan aktif degil.");
		return;
	}

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	/*long distance = DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY());
	if (distance >= CUBE_MAX_DISTANCE)
	{
		sys_log(1, "CUBE: TOO_FAR: %s distance %d", ch->GetName(), distance);
		return;
	}*/

	SendDateCubeRenewalPackets(ch, CUBE_RENEWAL_SUB_HEADER_CLEAR_DATES_RECEIVE);
	SendDateCubeRenewalPackets(ch, CUBE_RENEWAL_SUB_HEADER_DATES_RECEIVE, npcVNUM);
	SendDateCubeRenewalPackets(ch, CUBE_RENEWAL_SUB_HEADER_DATES_LOADING);
	SendDateCubeRenewalPackets(ch, CUBE_RENEWAL_SUB_HEADER_OPEN_RECEIVE);

	ch->SetCubeNpc(npc);
}

void Cube_close(LPCHARACTER ch)
{
	if (NULL == ch || !ch)
		return;

	ch->SetCubeNpc(NULL);
	ch->SetCubeNpcRace(0);
}

void Cube_Make(LPCHARACTER ch, int index, int count_item, int index_item_improve)
{
	#ifdef CUBE_RENEWAL_RELOAD_FIX
	if (s_isInitializedCubeMaterialInformation) { return; }
	#endif
	if (NULL == ch || !ch) { return; }
	if (!ch->GetDesc()) { return; }

	// if ((index < 0) || (count_item <= 0) || (count_item > 200) || (index_item_improve != -1)) { return; }
	if ((index < 0) || (count_item <= 0) || (count_item > 200)) { return; }

	DWORD dwNpcRace = ch->GetCubeNpcRace();
	if (dwNpcRace == 0)
	{
		LPCHARACTER npc = ch->GetQuestNPC();
		if (!npc) { return; }
		dwNpcRace = npc->GetRaceNum();
	}

	if (dwNpcRace == 0) { return; }

	if (!ch->IsCubeOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("To create an item you have to have the refinement window open"));
		return;
	}

	int index_value = 0;
	bool material_check = true;

	bool bBonusStatus = false;
	DWORD copyAttr[ITEM_ATTRIBUTE_MAX_NUM][2]{};
	DWORD copySocket[ITEM_SOCKET_MAX_NUM]{};
	#ifdef ENABLE_EVOLUTION_SYSTEM
	int copyEvolution = 0;
	#endif
	#ifdef ENABLE_CHANGELOOK_SYSTEM
	DWORD copyTransmutation = 0;
	#endif
	#ifdef ENABLE_LOCKED_SYSTEM
	BYTE copyLocked = 0;
	#endif

	const TCubeResultList& resultList = cube_info_map[dwNpcRace];
	for (TCubeResultList::const_iterator iter = resultList.begin(); resultList.end() != iter; ++iter)
	{
		if (index_value == index)
		{
			const SCubeMaterialInfo& materialInfo = *iter;

			if (materialInfo.material.size() > 0)
			{
				const TItemTable* pTable = ITEM_MANAGER::instance().GetTable(materialInfo.reward.vnum);
				if (NULL != pTable)
				{
					if (!(pTable->dwFlags & ITEM_FLAG_STACKABLE) || IS_SET(pTable->dwAntiFlags, ITEM_ANTIFLAG_STACK))
					{
						count_item = 1;
					}

					#ifdef ENABLE_FULL_YANG
					int64_t pCheck = static_cast<int64_t>(materialInfo.gold * count_item);
					#else
					INT pCheck = static_cast<INT>(materialInfo.gold * count_item);
					#endif
					if (pCheck < 0) { return; }

					if (ch->GetGold() < pCheck)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, "[CUBE] Yeterli yang yok.");
						return;
					}

					//BonusAktarma//
					for (unsigned int i = 0; i < materialInfo.material.size(); ++i)
					{
						const TItemTable* pTableMaterial = ITEM_MANAGER::instance().GetTable(materialInfo.material[i].vnum);
						if (NULL != pTableMaterial)
						{
							if (pTableMaterial->bType == ITEM_WEAPON || pTableMaterial->bType == ITEM_ARMOR || pTableMaterial->bType == ITEM_BELT)
							{
								bool hasElement = false;

								if (materialInfo.reward.vnum == materialInfo.material[i].vnum)
								{
									hasElement = true;
								}
								else if (pTable->bType == pTableMaterial->bType && pTable->bSubType == pTableMaterial->bSubType)
								{
									hasElement = true;
								}

								if (hasElement == false)
									continue;

								LPITEM pkItem = ch->FindSpecifyItem(materialInfo.material[i].vnum);
								if (!pkItem)
									continue;

								for (int a = 0; a < ITEM_ATTRIBUTE_MAX_NUM; a++)
								{
									copyAttr[a][0] = pkItem->GetAttributeType(a);
									copyAttr[a][1] = pkItem->GetAttributeValue(a);
								}

								for (int s = 0; s < ITEM_SOCKET_MAX_NUM; s++)
								{
									copySocket[s] = pkItem->GetSocket(s);
								}

								#ifdef ENABLE_EVOLUTION_SYSTEM
								copyEvolution = pkItem->GetEvolution();
								#endif

								#ifdef ENABLE_CHANGELOOK_SYSTEM
								copyTransmutation = pkItem->GetTransmutation();
								#endif

								#ifdef ENABLE_LOCKED_SYSTEM
								copyLocked = pkItem->GetLocked();
								#endif

								bBonusStatus = true;
								break;
							}
						}
					}
					//BonusAktarma//
				}
			}

			for (unsigned int i = 0; i < materialInfo.material.size(); ++i)
			{
				if (materialInfo.material[i].vnum > 0)
				{
					if (ch->CountSpecifyItem(materialInfo.material[i].vnum) < (DWORD)(materialInfo.material[i].count * count_item))
					{
						material_check = false;
					}
				}
			}

			if (materialInfo.gold != 0)
			{
				#ifdef ENABLE_FULL_YANG
				if (ch->GetGold() < static_cast<int64_t>(materialInfo.gold * count_item))
				#else
				if (ch->GetGold() < (INT)(materialInfo.gold * count_item))
				#endif
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "[CUBE] Yeterli yang yok.");
					return;
				}
			}

			if (material_check)
			{
				int the_percent_status = 0;
				int porcent_item_improve = 0;

				if (index_item_improve != -1)
				{
					LPITEM item = ch->GetInventoryItem(index_item_improve);
					if (item != NULL)
					{
						if (item->GetVnum() == NEW_CUBE_RENEWAL_ITEM_IMPROVE)
						{
							if (item->GetCount() <= 40)
							{
								if (materialInfo.percent + item->GetCount() <= 100)
								{
									porcent_item_improve = item->GetCount();
								}

								if (materialInfo.percent < 100)
								{
									if (materialInfo.percent + item->GetCount() > 100)
									{
										porcent_item_improve = 100 - materialInfo.percent;
									}
								}
							}

							if (porcent_item_improve != 0)
							{
								item->SetCount(item->GetCount() - porcent_item_improve);
							}
						}
					}
				}

				for (int i = 0; i < count_item; ++i)
				{
					int percent_number = number(1, 100);
					if (percent_number <= (materialInfo.percent + porcent_item_improve))
					{
						the_percent_status++;
					}
				}

				int the_percent_count = 1;
				if (the_percent_status > 0) { the_percent_count = the_percent_status; }

				for (unsigned int i = 0; i < materialInfo.material.size(); ++i)
				{
					if (materialInfo.material[i].vnum > 0)
					{
						ch->RemoveSpecifyItem(materialInfo.material[i].vnum, (materialInfo.material[i].count * the_percent_count));
					}
				}

				if (materialInfo.gold > 0)
				{
					#ifdef ENABLE_FULL_YANG
					ch->GoldChange(-static_cast<int64_t>(materialInfo.gold * the_percent_count));
					#else
					ch->PointChange(POINT_GOLD, -static_cast<long long>(materialInfo.gold * the_percent_count), false);
					#endif
				}

				if (the_percent_status <= 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "[CUBE] Geliþtirme baþarýsýz.");
					return;
				}

				LPITEM pItem = ITEM_MANAGER::instance().CreateItem(materialInfo.reward.vnum, (materialInfo.reward.count * the_percent_count));
				if (!pItem)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "[CUBE] Bir hata olustu!!");
					return;
				}

				//BonusAktarma//
				if (bBonusStatus)
				{
					pItem->ClearAttribute();

					for (int a = 0; a < ITEM_ATTRIBUTE_MAX_NUM; a++)
					{
						if (copyAttr[a][0])
							pItem->SetForceAttribute(a, copyAttr[a][0], copyAttr[a][1]);
					}

					for (int s = 0; s < ITEM_SOCKET_MAX_NUM; s++)
					{
						if (copySocket[s])
							pItem->SetSocket(s, copySocket[s]);
					}

					#ifdef ENABLE_EVOLUTION_SYSTEM
					pItem->SetEvolution(copyEvolution);
					#endif

					#ifdef ENABLE_CHANGELOOK_SYSTEM
					pItem->SetTransmutation(copyTransmutation);
					#endif

					#ifdef ENABLE_LOCKED_SYSTEM
					pItem->SetLocked(copyLocked);
					#endif
				}
				//BonusAktarma//

				int iEmptyPos = ch->GetEmptyAllInventory(pItem);
				if (iEmptyPos < 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "[CUBE] Envanterinizde yeterli yer yok.");
					pItem->AddToGround(ch->GetMapIndex(), ch->GetXYZ());
					pItem->StartDestroyEvent();
					pItem->SetOwnership(ch, 300);
				}
				else
				{
					pItem->AddToItem(ch, iEmptyPos);
					ITEM_MANAGER::instance().FlushDelayedSave(pItem);
				}

				#ifdef ENABLE_BATTLE_PASS
				uint8_t bBattlePassId = ch->GetBattlePassId();
				if (bBattlePassId)
				{
					CBattlePass::instance().BattlePassCheckMissions(ch, bBattlePassId, BP_MT_CRAFT_ITEM, pItem->GetVnum());
				}
				#endif

				#ifdef ENABLE_REWARD_SYSTEM
				CHARACTER_MANAGER::Instance().SetRewardData(REWARD_CRAFT, ch->GetName(), true, pItem->GetVnum(), 1);
				#endif

				ch->ChatPacket(CHAT_TYPE_INFO, "[CUBE] Geliþtirme baþarýlý.");
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "[CUBE] Yeterli malzemeniz yok.");
			}
		}

		index_value++;
	}
}

void SendDateCubeRenewalPackets(LPCHARACTER ch, BYTE subheader, DWORD npcVNUM)
{
	if (!ch) { return; }

	TPacketGCCubeRenewalReceive pack;
	pack.subheader = subheader;

	if (subheader == CUBE_RENEWAL_SUB_HEADER_DATES_RECEIVE)
	{
		const TCubeResultList& resultList = cube_info_map[npcVNUM];
		for (TCubeResultList::const_iterator iter = resultList.begin(); resultList.end() != iter; ++iter)
		{
			const SCubeMaterialInfo& materialInfo = *iter;

			pack.date_cube_renewal.vnum_reward = materialInfo.reward.vnum;
			pack.date_cube_renewal.count_reward = materialInfo.reward.count;

			const TItemTable * pTable = ITEM_MANAGER::instance().GetTable(materialInfo.reward.vnum);
			if (NULL != pTable)
			{
				if ((pTable->dwFlags & ITEM_FLAG_STACKABLE) || !IS_SET(pTable->dwAntiFlags, ITEM_ANTIFLAG_STACK))
				{
					pack.date_cube_renewal.item_reward_stackable = true;
				}
				else
				{
					pack.date_cube_renewal.item_reward_stackable = false;
				}
			}
			else
			{
				pack.date_cube_renewal.item_reward_stackable = false;
			}

			for (int x = 0; x < CUBE_MATERIAL_MAX_NUM; x++)
			{
				pack.date_cube_renewal.vnum_material[x] = FN_check_cube_item_vnum_material(materialInfo, (x+1));
				pack.date_cube_renewal.count_material[x] = FN_check_cube_item_count_material(materialInfo, (x+1));
			}

			pack.date_cube_renewal.gold = materialInfo.gold;

			pack.date_cube_renewal.percent = materialInfo.percent;

			// memcpy(pack.date_cube_renewal.category, materialInfo.category.c_str(), sizeof(pack.date_cube_renewal.category));
			strlcpy(pack.date_cube_renewal.category, materialInfo.category.c_str(), sizeof(pack.date_cube_renewal.category));

			LPDESC d = ch->GetDesc();
			if (NULL == d)
			{
				sys_err("User SendDateCubeRenewalPackets (%s)'s DESC is NULL POINT.", ch->GetName());
				return;
			}

			d->Packet(&pack, sizeof(pack));
		}
	}
	else
	{
		LPDESC d = ch->GetDesc();
		if (NULL == d)
		{
			sys_err("User SendDateCubeRenewalPackets (%s)'s DESC is NULL POINT.", ch->GetName());
			return;
		}

		d->Packet(&pack, sizeof(pack));
	}
}