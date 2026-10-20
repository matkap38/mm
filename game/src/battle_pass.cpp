#include "stdafx.h"
#include "battle_pass.h"

#include "p2p.h"
#include "locale_service.h"
#include "char.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "questmanager.h"
#include "questlua.h"
#include "start_position.h"
#include "char_manager.h"
#include "item_manager.h"
#include "sectree_manager.h"
#include "regen.h"
#include "log.h"
#include "db.h"
#include "target.h"
#include "party.h"
#include "item.h"
#include <string>
#include <algorithm>

//Bitirice item = 50415

const std::string g_astMissionType[BP_MT_MAX] = {
	"",
	"MONSTER_KILL_LEVEL",
	"MONSTER_KILL_VNUM",
	"PLAYER_KILL",
	"MONSTER_DAMAGE_LEVEL",
	"MONSTER_DAMAGE_VNUM",
	"PLAYER_DAMAGE",
	"MONSTER_RECORD_DAMAGE",
	"PLAYER_RECORD_DAMAGE",
	"USE_ITEM",
	"SELL_ITEM",
	"CRAFT_ITEM",
	"REFINE_ITEM",
	"DESTROY_ITEM",
	"COLLECT_ITEM",
	"FRY_FISH",
	"CATCH_FISH",
	"SPENT_YANG",
	"FARM_YANG",
	"COMPLETE_DUNGEON",
	"COMPLETE_MINIGAME",
	"CHAT_ON_SHOUT",
	"OFFLINE_SHOP",
	"PLAYER_PLAY_TIME",
};

CBattlePass::CBattlePass()
{
	m_pLoader = nullptr;
}

CBattlePass::~CBattlePass()
{
	if (m_pLoader) delete m_pLoader;
}

bool CBattlePass::ReadBattlePassFile()
{
	char szBattlePassFileName[256];
	snprintf(szBattlePassFileName, sizeof(szBattlePassFileName), "%s/savas_bileti.txt", LocaleService_GetBasePath().c_str());

	m_pLoader = new CGroupTextParseTreeLoader;
	CGroupTextParseTreeLoader& loader = *m_pLoader;

	if (false == loader.Load(szBattlePassFileName)) { sys_err("savas_bileti.txt load error"); return false; }
	if (!ReadBattlePassGroup()) { sys_err("savas_bileti.txt ReadBattlePassGroup error"); return false; }
	if (!ReadBattlePassMissions()) { sys_err("savas_bileti.txt ReadBattlePassMissions error"); return false; }
	return true;
}

bool CBattlePass::ReadBattlePassGroup()
{
	std::string stName;
	CGroupNode* pGroupNode = m_pLoader->GetGroup("savasbileti");
	if (nullptr == pGroupNode) { sys_err("savas_bileti.txt need SavasBileti group."); return false; }

	int32_t n = pGroupNode->GetRowCount();
	if (0 == n) { sys_err("Group SavasBileti is Empty."); return false; }

	std::set<uint8_t> setIDs;

	for (int32_t i = 0; i < n; i++)
	{
		const CGroupNode::CGroupNodeRow* pRow;
		pGroupNode->GetRow(i, &pRow);
		std::string stBattlePassGroupName;
		std::string stBattlePassName;
		BYTE battlePassId;
		if (!pRow->GetValue("groupname", stBattlePassGroupName)) { sys_err("In Group SavasBileti, No groupName column."); return false; }
		if (!pRow->GetValue("bpid", battlePassId)) { sys_err("In Group SavasBileti, %s's bpID is invalid", stBattlePassGroupName.c_str()); return false; }
		if (!pRow->GetValue("bpname", stBattlePassName)) { sys_err("In Group SavasBileti, %s's bpName is invalid", stBattlePassGroupName.c_str()); return false; }
		if (setIDs.end() != setIDs.find(battlePassId)) { sys_err("In Group SavasBileti, duplicated BiletID exist."); return false; }
		sBattlePassNameInfo tempStruct;
		tempStruct.groupName = stBattlePassGroupName;
		tempStruct.battlePassName = stBattlePassName;
		setIDs.insert(battlePassId);
		m_map_battle_pass_name.insert(TMapBattlePassName::value_type(battlePassId, tempStruct));
	}

	return true;
}

bool CBattlePass::ReadBattlePassMissions()
{
	TMapBattlePassName::iterator it = m_map_battle_pass_name.begin();
	while (it != m_map_battle_pass_name.end()) {
		std::string battlePassName = (it++)->second.groupName;
		CGroupNode* pGroupNode = m_pLoader->GetGroup(battlePassName.c_str());
		if (nullptr == pGroupNode) { sys_err("savas_bileti.txt need group %s.", battlePassName.c_str()); return false; }

		DWORD n = pGroupNode->GetChildNodeCount();
		if (n < 2) { sys_err("Group %s need to have at least one grup for Reward and one Mission. Row: %d", battlePassName.c_str(), n); return false; }

		{
			CGroupNode* pChild;
			if (nullptr == (pChild = pGroupNode->GetChildNode("odul"))) { sys_err("In Group %s, Reward group is not defined.", battlePassName.c_str()); return false; }

			int32_t m = pChild->GetRowCount();
			std::vector<TBattlePassRewardInfo> rewardVector;

			for (int32_t j = 1; j <= m; j++)
			{
				std::stringstream ss;
				ss << j;
				const CGroupNode::CGroupNodeRow* pRow = NULL;

				pChild->GetRow(ss.str(), &pRow);
				if (nullptr == pRow) { sys_err("In Group %s, subgroup Reward, No %d row.", battlePassName.c_str(), j); return false; }

				TBattlePassRewardInfo itemReward;
				if (!pRow->GetValue("type", itemReward.bRewardType)) { sys_err("In Group %s, subgroup Reward, RewardType is empty.", battlePassName.c_str()); return false; }
				if (!pRow->GetValue("vnum", itemReward.dwVnum)) { sys_err("In Group %s, subgroup Reward, Vnum is empty.", battlePassName.c_str()); return false; }
				if (!pRow->GetValue("value", itemReward.wCount)) { sys_err("In Group %s, subgroup Reward, Value is empty.", battlePassName.c_str()); return false; }

				rewardVector.emplace_back(itemReward);
			}
			m_map_battle_pass_reward.insert(TMapBattlePassReward::value_type(battlePassName.c_str(), rewardVector));
		}

		std::vector<TBattlePassMissionInfo> missionInfoVector;

		for (int32_t i = 1; i < n; i++) {
			std::stringstream ss;
			ss << "gorev_" << i;

			CGroupNode* pChild;
			if (nullptr == (pChild = pGroupNode->GetChildNode(ss.str()))) { sys_err("In Group %s, %s subgroup is not defined.", battlePassName.c_str(), ss.str().c_str()); return false; }
			int32_t m = pChild->GetRowCount();

			std::string stMissionSearch[] = { "", "" };
			bool bAlreadySearched = false;
			uint8_t bRewardContor = 0;
			TBattlePassMissionInfo missionInfo;
			memset(&missionInfo, 0, sizeof(TBattlePassMissionInfo));
			missionInfo.bMissionID = i;

			for (int32_t j = 0; j < m; j++)
			{
				const CGroupNode::CGroupNodeRow* pRow = nullptr;

				pChild->GetRow(j, &pRow);
				if (nullptr == pRow) { sys_err("In Group %s and subgroup %s null row.", battlePassName.c_str(), ss.str().c_str()); return false; }

				// InfoDesc = ItemVnum from reward
				// InfoName = ItemCount from reward

				std::string stInfoFirst;
				if (!pRow->GetValue("infofirst", stInfoFirst)) { sys_err("In Group %s and subgroup %s infofirst does not exist.", battlePassName.c_str(), ss.str().c_str()); return false; }
				if (stInfoFirst == "name")
				{
					std::string stMissionName;
					if (!pRow->GetValue("infosecond", stMissionName)) { sys_err("In Group %s and subgroup %s InfoName does not exist.", battlePassName.c_str(), ss.str().c_str()); return false; }
					strlcpy(missionInfo.szMissionName, stMissionName.c_str(), sizeof(missionInfo.szMissionName));
					continue;
				}

				if (stInfoFirst == "type")
				{
					std::string stInfoName;
					if (!pRow->GetValue("infosecond", stInfoName)) { sys_err("In Group %s and subgroup %s InfoName does not exist.", battlePassName.c_str(), ss.str().c_str()); return false; }
					missionInfo.bMissionType = GetMissionTypeByName(stInfoName);
				}

				if (missionInfo.bMissionType <= BP_MT_NONE || missionInfo.bMissionType >= BP_MT_MAX) { sys_err("In Group %s Gorev%d mission type unknown.", battlePassName.c_str(), i); return false; }
				if (!bAlreadySearched)
				{
					GetMissionSearchName(missionInfo.bMissionType, &stMissionSearch[0], &stMissionSearch[1]);
					bAlreadySearched = true;
				}

				for (uint8_t k = 0; k < 2; k++)
				{
					if (stMissionSearch[k] != "")
					{
						if (stInfoFirst == stMissionSearch[k])
						{
							if (!pRow->GetValue("infosecond", missionInfo.dwMissionInfo[k])) { sys_err("In Group %s and subgroup %s infofirst %s infosecond does not exist.", battlePassName.c_str(), ss.str().c_str(), stMissionSearch[k].c_str()); return false; }
							stMissionSearch[k] = "";
						}
					}
				}

				if (stInfoFirst == "time" && missionInfo.bMissionType == BP_MT_PLAYER_PLAY_TIME)
				{
					missionInfo.dwMissionInfo[1] *= 3600;//(60*60=3600)
				}

				if (bRewardContor >= MISSION_REWARD_COUNT) { sys_err("In Group %s and subgroup %s More than 3 rewards.", battlePassName.c_str(), ss.str().c_str()); return false; }

				if (stInfoFirst == "reward_item")
				{
					std::string stItemVnum;
					if (!pRow->GetValue("infosecond", stItemVnum)) { sys_err("In Group %s and subgroup %s InfoName does not exist.", battlePassName.c_str(), ss.str().c_str()); return false; }
					DWORD dwVnum = atoi(stItemVnum.c_str());
					uint16_t wCount = 1;
					if (!pRow->GetValue("infothird", wCount)) { sys_err("In Group %s and subgroup %s Wrong ItemCount.", battlePassName.c_str(), ss.str().c_str()); return false; }

					missionInfo.aRewardList[bRewardContor].bRewardType = 1;
					missionInfo.aRewardList[bRewardContor].dwVnum = dwVnum;
					missionInfo.aRewardList[bRewardContor].wCount = wCount;
					bRewardContor++;
				}

				if (stInfoFirst == "reward_bonus")
				{
					std::string szBonusID;
					if (!pRow->GetValue("infosecond", szBonusID)) { sys_err("In Group %s and subgroup %s InfoName does not exist.", battlePassName.c_str(), ss.str().c_str()); return false; }
					int32_t dwVnum = atoi(szBonusID.c_str());
					int32_t bonusValue = 1;
					if (!pRow->GetValue("infothird", bonusValue)) { sys_err("In Group %s and subgroup %s Wrong ItemCount.", battlePassName.c_str(), ss.str().c_str()); return false; }

					missionInfo.aRewardList[bRewardContor].bRewardType = 2;
					missionInfo.aRewardList[bRewardContor].dwVnum = dwVnum;
					missionInfo.aRewardList[bRewardContor].wCount = bonusValue;
					bRewardContor++;
				}

				if (stInfoFirst == "reward_money")
				{
					std::string szMoney;
					if (!pRow->GetValue("infosecond", szMoney)) { sys_err("In Group %s and subgroup %s InfoName does not exist.", battlePassName.c_str(), ss.str().c_str()); return false; }
					#ifdef ENABLE_FULL_YANG
					int64_t dwMoney = strtoull(szMoney.c_str(), NULL, 10);
					#else
					int32_t dwMoney = atoi(szMoney.c_str());
					#endif
					int32_t wCount = 1;
					if (!pRow->GetValue("infothird", wCount)) { sys_err("In Group %s and subgroup %s Wrong ItemCount.", battlePassName.c_str(), ss.str().c_str()); return false; }

					missionInfo.aRewardList[bRewardContor].bRewardType = 3;
					missionInfo.aRewardList[bRewardContor].dwVnum = dwMoney;
					missionInfo.aRewardList[bRewardContor].wCount = 1;//wCount
					bRewardContor++;
				}
			}
			missionInfoVector.emplace_back(missionInfo);
		}
		m_map_battle_pass_mission_info.insert(TMapBattleMissionInfo::value_type(battlePassName.c_str(), missionInfoVector));
	}
	return true;
}

uint8_t CBattlePass::GetMissionTypeByName(std::string stMissionName)
{
	for (uint8_t i = BP_MT_NONE; i < BP_MT_MAX; i++) { if (g_astMissionType[i] == stMissionName) return i; }
	return 0;
}

std::string CBattlePass::GetMissionNameByType(uint8_t bType)
{
	for (uint8_t i = BP_MT_NONE; i < BP_MT_MAX; i++) { if (i == bType) return g_astMissionType[i]; }
	return "";
}

std::string CBattlePass::GetBattlePassNameByID(uint8_t bID)
{
	TMapBattlePassName::iterator it = m_map_battle_pass_name.find(bID);
	if (it == m_map_battle_pass_name.end()) { return ""; }
	return it->second.groupName;
}

void CBattlePass::GetMissionSearchName(uint8_t bMissionType, std::string* st_name_2, std::string* st_name_3)
{
	switch (bMissionType)
	{
		case BP_MT_MONSTER_KILL_VNUM:
		case BP_MT_USE_ITEM:
		case BP_MT_SELL_ITEM:
		case BP_MT_CRAFT_ITEM:
		case BP_MT_REFINE_ITEM:
		case BP_MT_DESTROY_ITEM:
		case BP_MT_COLLECT_ITEM:
			*st_name_2 = "vnum";
			*st_name_3 = "count";
			break;

		case BP_MT_PLAYER_KILL:
		case BP_MT_MONSTER_DAMAGE_LEVEL:
		case BP_MT_MONSTER_KILL_LEVEL:
			*st_name_2 = "min_level";
			*st_name_3 = "count";
			break;

		case BP_MT_MONSTER_DAMAGE_VNUM:
		case BP_MT_MONSTER_RECORD_DAMAGE:
			*st_name_2 = "vnum";
			*st_name_3 = "value";
			break;

		case BP_MT_PLAYER_DAMAGE:
		case BP_MT_PLAYER_RECORD_DAMAGE:
			*st_name_2 = "min_level";
			*st_name_3 = "value";
			break;

		case BP_MT_FRY_FISH:
		case BP_MT_CATCH_FISH:
		case BP_MT_CHAT_ON_SHOUT:
		case BP_MT_OFFLINE_SHOP:
			*st_name_2 = "";
			*st_name_3 = "count";
			break;

		case BP_MT_SPENT_YANG:
		case BP_MT_FARM_YANG:
			*st_name_2 = "";
			*st_name_3 = "value";
			break;

		case BP_MT_COMPLETE_DUNGEON:
		case BP_MT_COMPLETE_MINIGAME:
			*st_name_2 = "id";
			*st_name_3 = "count";
			break;

		case BP_MT_PLAYER_PLAY_TIME:
			*st_name_2 = "";
			*st_name_3 = "time";
			break;

		default:
			*st_name_2 = "";
			*st_name_3 = "";
			break;
	}
}

void CBattlePass::BattlePassRequestOpen(LPCHARACTER pkChar)
{
	if (!pkChar) return;
	if (!pkChar->GetDesc()) return;
	if (!pkChar->IsLoadedBattlePass())
	{
		pkChar->ChatPacket(CHAT_TYPE_INFO, "<Sistem> 5 saniye bekleyin ve tekrar deneyin.");
		return;
	}

	uint8_t bBattlePassId = pkChar->GetBattlePassId();
	if (!bBattlePassId) {
		pkChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("aktif bp yok."));
		return;
	}
	TMapBattlePassName::iterator it = m_map_battle_pass_name.find(bBattlePassId);
	if (it == m_map_battle_pass_name.end()) { pkChar->ChatPacket(CHAT_TYPE_INFO, "<Sistem> Sistemsel bir hata (1)"); return; }

	std::string battlePassName = it->second.groupName;
	TMapBattleMissionInfo::iterator itInfo = m_map_battle_pass_mission_info.find(battlePassName);

	if (itInfo == m_map_battle_pass_mission_info.end()) { pkChar->ChatPacket(CHAT_TYPE_INFO, "<Sistem> Sistemsel bir hata (2)"); return; }

	TMapBattlePassReward::iterator itReward = m_map_battle_pass_reward.find(battlePassName);
	if (itReward == m_map_battle_pass_reward.end()) { pkChar->ChatPacket(CHAT_TYPE_INFO, "<Sistem> Sistemsel bir hata (3)"); return; }

	std::vector<TBattlePassRewardInfo> rewardInfo = itReward->second;
	std::vector<TBattlePassMissionInfo> missionInfo = itInfo->second;

	for (size_t i = 0; i < missionInfo.size(); i++) { missionInfo[i].dwMissionInfo[2] = pkChar->GetMissionProgress(missionInfo[i].bMissionID, bBattlePassId); }

	if (!missionInfo.empty())
	{
		TPacketGCBattlePass packet;
		packet.bHeader = HEADER_GC_BATTLE_PASS_OPEN;
		packet.wSize = sizeof(packet) + sizeof(TBattlePassMissionInfo) * missionInfo.size();
		packet.wRewardSize = sizeof(TBattlePassRewardInfo) * rewardInfo.size();
		strlcpy(packet.battlePassName, it->second.battlePassName.c_str(), sizeof(packet.battlePassName));

		pkChar->GetDesc()->BufferedPacket(&packet, sizeof(packet));
		pkChar->GetDesc()->BufferedPacket(&missionInfo[0], sizeof(TBattlePassMissionInfo) * missionInfo.size());
		pkChar->GetDesc()->Packet(&rewardInfo[0], sizeof(TBattlePassRewardInfo) * rewardInfo.size());
	}
}

void CBattlePass::BattlePassRewardMission(LPCHARACTER pkChar, uint8_t bMissionID, DWORD bBattlePassId)
{
	if (!pkChar) return;
	if (!pkChar->GetDesc()) return;
	if (!bBattlePassId)
	{
		pkChar->ChatPacket(CHAT_TYPE_INFO, "<Sistem> Aktif BattlePass yok.");
		return;
	}

	TMapBattlePassName::iterator it = m_map_battle_pass_name.find(bBattlePassId);
	if (it == m_map_battle_pass_name.end()) { return; }

	std::string battlePassName = it->second.groupName;
	TMapBattleMissionInfo::iterator itInfo = m_map_battle_pass_mission_info.find(battlePassName);
	if (itInfo == m_map_battle_pass_mission_info.end()) { return; }

	std::vector<TBattlePassMissionInfo> missionInfo = itInfo->second;

	for (size_t i = 0; i < missionInfo.size(); i++)
	{
		if (missionInfo[i].bMissionID == bMissionID)
		{
			for (uint8_t j = 0; j < MISSION_REWARD_COUNT; j++)
			{
				if (missionInfo[i].aRewardList[j].dwVnum && missionInfo[i].aRewardList[j].wCount > 0)
				{
					if (missionInfo[i].aRewardList[j].bRewardType == 1)
					{
						LPITEM giveItem = pkChar->AutoGiveItem(missionInfo[i].aRewardList[j].dwVnum, missionInfo[i].aRewardList[j].wCount);
						if (giveItem) { DBManager::instance().DirectQuery("INSERT INTO log.log (type, time, who, x, y, what, how) VALUES('CHARACTER', NOW(), %u, %u, %u, %u, 'BATTLEPASS_REWARD')", pkChar->GetPlayerID(), pkChar->GetX(), pkChar->GetY(), giveItem->GetVnum()); }
					}
					else if (missionInfo[i].aRewardList[j].bRewardType == 2)
					{
						auto pAffect = pkChar->FindAffect(AFFECT_BATTLE_PASS_BONUS, aApplyInfo[missionInfo[i].aRewardList[j].dwVnum].bPointType);
						int32_t bonusValuePlus = 0;
						if (pAffect) { bonusValuePlus = pAffect->lApplyValue; }
						pkChar->AddAffect(AFFECT_BATTLE_PASS_BONUS, aApplyInfo[missionInfo[i].aRewardList[j].dwVnum].bPointType, bonusValuePlus + missionInfo[i].aRewardList[j].wCount, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, true);
					}
					else if (missionInfo[i].aRewardList[j].bRewardType == 3)
					{
						int64_t gold = missionInfo[i].aRewardList[j].dwVnum;
						
						sys_err("MoneyGive1: %lld", gold);
						
						if ((pkChar->GetGold() + gold) >= pkChar->GetAllowedGold()) { continue; }
						pkChar->GoldChange(gold);
					}
				}
			}
			break;
		}
	}
}

void CBattlePass::BattlePassRequestReward(LPCHARACTER pkChar)
{
	if (!pkChar) return;
	if (!pkChar->GetDesc()) return;
	auto bBattlePassId = pkChar->GetBattlePassId();
	if (!bBattlePassId) return;

	auto it = m_map_battle_pass_name.find(bBattlePassId);
	if (it == m_map_battle_pass_name.end()) return;

	auto battlePassName = it->second.groupName;
	auto itInfo = m_map_battle_pass_mission_info.find(battlePassName);
	if (itInfo == m_map_battle_pass_mission_info.end()) return;

	auto missionInfo = itInfo->second;

	bool bIsCompleted = true;
	for (size_t i = 0; i < missionInfo.size(); i++)
	{
		if (!pkChar->IsCompletedBattlePassID(itInfo->second[i].bMissionID, bBattlePassId)) { bIsCompleted = false; break; }
	}

	if (bIsCompleted)
	{
		TMapBattlePassReward::iterator itReward = m_map_battle_pass_reward.find(battlePassName);
		if (itReward == m_map_battle_pass_reward.end()) { return; }
		pkChar->RemoveAffect(AFFECT_BATTLE_PASS);

		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "REPLACE INTO player.battle_pass_ranking (player_name, battle_pass_id, finish_time) VALUES ('%s', %d, NOW())", pkChar->GetName(), bBattlePassId);
		DBManager::instance().DirectQuery(szQuery);

		std::vector<TBattlePassRewardInfo> rewardInfo = itReward->second;
		for (size_t i = 0; i < rewardInfo.size(); i++)
		{
			if (rewardInfo[i].bRewardType == 1)
			{
				LPITEM giveItem = pkChar->AutoGiveItem(rewardInfo[i].dwVnum, rewardInfo[i].wCount);
				if (giveItem) { DBManager::instance().DirectQuery("INSERT INTO log.log (type, time, who, x, y, what, how) VALUES('CHARACTER', NOW(), %u, %u, %u, %u, 'BATTLEPASS_REWARD')", pkChar->GetPlayerID(), pkChar->GetX(), pkChar->GetY(), giveItem->GetVnum()); }
			}
			else if (rewardInfo[i].bRewardType == 2)
			{
				auto pAffect = pkChar->FindAffect(AFFECT_BATTLE_PASS_BONUS, aApplyInfo[rewardInfo[i].dwVnum].bPointType);
				int32_t bonusValuePlus = 0;
				if (pAffect) { bonusValuePlus = pAffect->lApplyValue; }
				pkChar->AddAffect(AFFECT_BATTLE_PASS_BONUS, aApplyInfo[rewardInfo[i].dwVnum].bPointType, bonusValuePlus + rewardInfo[i].wCount, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, true);
			}
			else if (rewardInfo[i].bRewardType == 3)
			{
				int64_t gold = rewardInfo[i].dwVnum;
				
				sys_err("MoneyGive2: %lld", gold);
				
				if ((pkChar->GetGold() + gold) >= pkChar->GetAllowedGold()) { continue; }
				pkChar->GoldChange(gold);
			}
		}
		char buf[256] = { 0 };
		snprintf(buf, sizeof(buf), "<Sistem> %s isimli oyunucumuz BattlePass bitirdi. ", pkChar->GetName());
		BroadcastNotice(buf);
	}
}

bool CBattlePass::BattlePassCheckMissions(LPCHARACTER pkChar, uint8_t bBattlePassId, uint8_t bMissionType, DWORD dwFirstInfo, DWORD dwSecondInfo)
{
	if (!pkChar) return false;
	if (!pkChar->GetDesc()) return false;
	if (!pkChar->IsLoadedBattlePass())
	{
		pkChar->ChatPacket(CHAT_TYPE_INFO, "<Sistem>");
		return false;
	}

	TMapBattlePassName::iterator it = m_map_battle_pass_name.find(bBattlePassId);
	if (it == m_map_battle_pass_name.end()) return false;

	std::string battlePassName = it->second.groupName;
	TMapBattleMissionInfo::iterator itInfo = m_map_battle_pass_mission_info.find(battlePassName);
	if (itInfo == m_map_battle_pass_mission_info.end()) return false;

	std::vector<TBattlePassMissionInfo> missionInfo = itInfo->second;
	for (size_t i = 0; i < missionInfo.size(); i++)
	{
		if (missionInfo[i].bMissionType == bMissionType)
		{
			const DWORD dwCurrentMissionValue = pkChar->GetMissionProgress(missionInfo[i].bMissionID, bBattlePassId);
			if (dwCurrentMissionValue >= missionInfo[i].dwMissionInfo[1]) { continue; }

			switch (bMissionType)
			{
				case BP_MT_MONSTER_DAMAGE_LEVEL:
				{
					if (((dwFirstInfo != 0 && missionInfo[i].dwMissionInfo[0] <= dwFirstInfo) || missionInfo[i].dwMissionInfo[0] == 0) && dwCurrentMissionValue <= dwSecondInfo)
					{
						pkChar->UpdateMissionProgress(missionInfo[i].bMissionID, bBattlePassId, dwSecondInfo, missionInfo[i].dwMissionInfo[1], true);
					}
					break;
				}

				case BP_MT_PLAYER_KILL:
				case BP_MT_MONSTER_KILL_LEVEL:
				{
					if ((dwFirstInfo != 0 && missionInfo[i].dwMissionInfo[0] <= dwFirstInfo) || missionInfo[i].dwMissionInfo[0] == 0)
					{
						pkChar->UpdateMissionProgress(missionInfo[i].bMissionID, bBattlePassId, dwSecondInfo, missionInfo[i].dwMissionInfo[1]);
					}
					break;
				}

				case BP_MT_PLAYER_DAMAGE:
				{
					if (((dwFirstInfo != 0 && missionInfo[i].dwMissionInfo[0] <= dwFirstInfo) || missionInfo[i].dwMissionInfo[0] == 0) && dwCurrentMissionValue <= dwSecondInfo)
					{
						pkChar->UpdateMissionProgress(missionInfo[i].bMissionID, bBattlePassId, dwSecondInfo, missionInfo[i].dwMissionInfo[1], true);
					}
					break;
				}

				case BP_MT_MONSTER_DAMAGE_VNUM:
				{
					if (((dwFirstInfo != 0 && missionInfo[i].dwMissionInfo[0] == dwFirstInfo) || missionInfo[i].dwMissionInfo[0] == 0) && dwCurrentMissionValue <= dwSecondInfo)
					{
						pkChar->UpdateMissionProgress(missionInfo[i].bMissionID, bBattlePassId, dwSecondInfo, missionInfo[i].dwMissionInfo[1], true);
					}
					break;
				}

				case BP_MT_PLAYER_RECORD_DAMAGE:
				case BP_MT_MONSTER_RECORD_DAMAGE:
				{
					if (((dwFirstInfo != 0 && missionInfo[i].dwMissionInfo[0] == dwFirstInfo) || missionInfo[i].dwMissionInfo[0] == 0) && (dwSecondInfo != 0 && missionInfo[i].dwMissionInfo[1] <= dwSecondInfo))
					{
						pkChar->UpdateMissionProgress(missionInfo[i].bMissionID, bBattlePassId, dwSecondInfo, missionInfo[i].dwMissionInfo[1], true);
					}
					break;
				}

				case BP_MT_MONSTER_KILL_VNUM:
				{
					if ((dwFirstInfo != 0 && missionInfo[i].dwMissionInfo[0] == dwFirstInfo) || missionInfo[i].dwMissionInfo[0] == 0)
					{
						pkChar->UpdateMissionProgress(missionInfo[i].bMissionID, bBattlePassId, dwSecondInfo, missionInfo[i].dwMissionInfo[1]);
						return true;
					}
					break;
				}

				case BP_MT_PLAYER_PLAY_TIME:
				{
					pkChar->UpdateMissionProgress(missionInfo[i].bMissionID, bBattlePassId, dwSecondInfo, missionInfo[i].dwMissionInfo[1]);
					return true;
				}

				default:
				{
					if ((dwFirstInfo != 0 && missionInfo[i].dwMissionInfo[0] == dwFirstInfo) || missionInfo[i].dwMissionInfo[0] == 0)
					{
						pkChar->UpdateMissionProgress(missionInfo[i].bMissionID, bBattlePassId, dwSecondInfo, missionInfo[i].dwMissionInfo[1]);
					}
					break;
				}
			}
		}
	}

	return true;
}

void CBattlePass::BattlePassForceFinish(LPCHARACTER pkChar, int32_t missionID)
{
	if (!pkChar) return;
	if (!pkChar->GetDesc()) return;
	if (!pkChar->IsLoadedBattlePass()) { pkChar->ChatPacket(CHAT_TYPE_INFO, "[Yukleniyor]"); return; }
	
	uint8_t bBattlePassId = pkChar->GetBattlePassId();
	if (!bBattlePassId) { pkChar->ChatPacket(CHAT_TYPE_INFO, "<Sistem> BattlePass yok."); return; }
	auto it = m_map_battle_pass_name.find(bBattlePassId);
	if (it == m_map_battle_pass_name.end()) { pkChar->ChatPacket(CHAT_TYPE_INFO, "<Sistem> Sistemsel bir hata (1)"); return; }
	auto battlePassName = it->second.groupName;
	auto itInfo = m_map_battle_pass_mission_info.find(battlePassName);
	if (itInfo == m_map_battle_pass_mission_info.end()) { pkChar->ChatPacket(CHAT_TYPE_INFO, "<Sistem> Sistemsel bir hata (2)"); return; }
	
	for (size_t i = 0; i < itInfo->second.size(); i++) {
		if (itInfo->second[i].bMissionID == missionID)
		{
			if (!pkChar->CountSpecifyItem(50415))
			{
				pkChar->ChatPacket(CHAT_TYPE_INFO, "<Sistem> BP bitirme nesnesi envanterinde yok!");
				return;
			}

			if (pkChar->IsCompletedBattlePassID(itInfo->second[i].bMissionID, bBattlePassId))
			{
				pkChar->ChatPacket(CHAT_TYPE_INFO, "<Sistem> Bu BP zaten bitirildi.");
				return;
			}

			sys_log(0, "Gorev bitirici kisi: %s gorev id: %d", pkChar->GetName(), missionID);
			pkChar->UpdateMissionProgress(missionID, pkChar->GetBattlePassId(), itInfo->second[i].dwMissionInfo[1], itInfo->second[i].dwMissionInfo[1], true);
			pkChar->RemoveSpecifyItem(50415);
			break;
		}
	}
}