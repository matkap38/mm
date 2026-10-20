#pragma once

#ifndef __INC_METIN_II_BATTLE_PASS_H__
#define __INC_METIN_II_BATTLE_PASS_H__

#include <boost/unordered_map.hpp>

#include "../../common/stl.h"
#include "../../common/length.h"
#include "../../common/tables.h"
#include "group_text_parse_tree.h"

#include "packet.h"

class CBattlePass : public singleton<CBattlePass>
{
public:
	CBattlePass();
	virtual ~CBattlePass();

	bool ReadBattlePassFile();
	bool ReadBattlePassGroup();
	bool ReadBattlePassMissions();

	uint8_t GetMissionTypeByName(std::string);
	std::string GetMissionNameByType(uint8_t);
	std::string GetBattlePassNameByID(uint8_t);

	void GetMissionSearchName(uint8_t, std::string*, std::string*);

	void BattlePassRequestOpen(LPCHARACTER);
	bool BattlePassCheckMissions(LPCHARACTER, uint8_t, uint8_t, DWORD = 0, DWORD = 1);
	void BattlePassRewardMission(LPCHARACTER, uint8_t, DWORD);

	void BattlePassRequestReward(LPCHARACTER);
	void BattlePassForceFinish(LPCHARACTER, int32_t);

private:
	CGroupTextParseTreeLoader* m_pLoader;

	struct sBattlePassNameInfo { std::string groupName; std::string battlePassName; };

	typedef std::map <uint8_t, sBattlePassNameInfo> TMapBattlePassName;
	typedef std::map <std::string, std::vector<TBattlePassRewardInfo>> TMapBattlePassReward;
	typedef std::map <std::string, std::vector<TBattlePassMissionInfo>> TMapBattleMissionInfo;

	TMapBattlePassName m_map_battle_pass_name;
	TMapBattlePassReward m_map_battle_pass_reward;
	TMapBattleMissionInfo m_map_battle_pass_mission_info;
};

#endif
