#pragma once
constexpr WORD TANAKA_EVENT_MAP_INDEX = 234; // -> tanaka event map index
constexpr uint8_t TANAKA_EVENT_MAX_LEVEL = 3; // -> tanaka event max level 

class CNewTanakaEventManager : public singleton<CNewTanakaEventManager>
{
	enum class Eventstate
	{
		EVENT_STATE_NONE,
		EVENT_STATE_PREPARING,
		EVENT_STATE_OPEN,
	};

	enum eEventInfo
	{
		TANAKA_MOB_VNUM_1 = 5001, // -> tanaka mob vnum1
		TANAKA_MOB_VNUM_2 = 5005, // -> tanaka mob vnum2
		TANAKA_MOB_VNUM_3 = 5006, // -> tanaka mob vnum3

		TANAKA_EVENT_REGISTER_MIN_LEVEL = 1, // -> tanaka event register min level (tanaka evente katilim min level)

		TANAKA_EVENT_WARP_RED_X = 3950260, // -> tanaka event warp x (tanaka event oyuncu spawn noktasi x)
		TANAKA_EVENT_WARP_RED_Y = 7086, // -> tanaka event warp y (tanaka event oyuncu spawn noktasi y)

		TANAKA_EVENT_WARP_YELLOW_X = 3950110, // -> tanaka event warp x (tanaka event oyuncu spawn noktasi x)
		TANAKA_EVENT_WARP_YELLOW_Y = 7217, // -> tanaka event warp y (tanaka event oyuncu spawn noktasi y)

		TANAKA_EVENT_WARP_BLUE_X = 3950405, // -> tanaka event warp x (tanaka event oyuncu spawn noktasi x)
		TANAKA_EVENT_WARP_BLUE_Y = 7240, // -> tanaka event warp y (tanaka event oyuncu spawn noktasi y)

		TANAKA_EVENT_SPAWN_X = 3950250, // -> tanaka event spawn x (tanaka event spawn x)
		TANAKA_EVENT_SPAWN_Y = 7222, // -> tanaka event spawn y (tanaka event spawn y)
	};

	const int32_t spawn_tanaka_table[TANAKA_EVENT_MAX_LEVEL + 1] = // -> tanaka spawn count level by level (bu turda kac tanaka spawn edilecegi)
	{
		0,		//	0
		250,		//	Level1 - SpawnCount
		300,		//	Level2 - SpawnCount
		350,		//	Level3 - SpawnCount
	};

	const int32_t kill_tanaka_table[TANAKA_EVENT_MAX_LEVEL + 1] = // -> tanaka kill count level by level (bu turda kac tanaka oldurulecegi)
	{
		0,		//	0
		200,		//	Level1 - KillCount
		250,		//	Level2 - KillCount
		300,		//	Level3 - KillCount
	};

	using TPlayerInfo = struct SPlayerInfo
	{
		SPlayerInfo() : dwPID(0), wKillCount(0), totalDamage(0)
		{
			memset(szName, 0, sizeof(szName));
		}

		uint32_t dwPID;
		char szName[24 + 1];
		uint16_t wKillCount;
		unsigned long totalDamage;
	};

public:
	CNewTanakaEventManager() = default;
	~CNewTanakaEventManager() = default;

	auto IsTanakaRace(DWORD dwRace) -> bool;
	auto Initialize() -> void;
	auto SpawnEvent(BYTE bSpawn, long lTime) -> void;
	auto OpenEvent() -> void;
	auto StartExitEvent() -> void;
	auto StartEvent() -> void;
	auto EndEvent() -> void;
	auto P2PAction(uint8_t) -> void;
	auto SendP2PLoginRequest() -> void;
	auto IsEventOpen() const { return m_State == Eventstate::EVENT_STATE_OPEN; }
	auto UpdateEventLevel() -> void;
	auto SpawnTanaka() -> void;
	auto KillTanaka(LPCHARACTER) -> void;
	auto DamageTanaka(LPCHARACTER ch, uint32_t dwDamage) -> void;
	auto WarpPlayer(LPCHARACTER) -> void;
	auto AddPlayer(LPCHARACTER) -> void;
	auto ExitPlayer(LPCHARACTER) -> void;
	auto ExitAllPlayer() -> void;
	auto Update() -> void;
	auto Update(LPCHARACTER) -> void;
	auto SendGivedItemInfo(const char* ,uint32_t, uint8_t) -> void;
	auto GetEventLevel() const { return m_byEventLevel; }

private:
	uint8_t m_byEventLevel;
	Eventstate m_State;
	int32_t m_bySpawnCount;
	int32_t m_byKillCount;
	CHARACTER_VECTOR character_vec;
	LPEVENT m_pEvent;
	LPEVENT m_pSpawnEvent;
	std::map<uint32_t, TPlayerInfo> m_mapPlayerInfo;
	std::map<uint32_t, uint8_t> m_mapEventItemInfo;
};
