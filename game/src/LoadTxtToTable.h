#ifndef _load_txt_to_table_h_
#define _load_txt_to_table_h_

extern void LoadTheSystems();
extern void LoadTheDbSystems();

#ifdef ENABLE_ALIGNMENT_BONUS
extern std::vector<TAlignmentTable> m_mapAlignmentList;
extern void LoadAlignmentBonus();
#endif

#ifdef ENABLE_PLAYER_TAG_SYSTEM
enum EPlayerTags
{
	TAG_NONE,
	TAG_YOUTUBE,
	TAG_TWITCH,
	TAG_TIKTOK,
	TAG_KICK,
	TAG_KING,
	TAG_KING_GUILD,
	#ifdef ENABLE_RONARK_SYSTEM
	TAG_KING_RONARK,
	#endif
};

typedef struct SPlayerTags
{
	DWORD	Pid;
	BYTE	Tag;
} TPlayerTags;

extern std::vector<TPlayerTags> SendPlayerTags;
extern void LoadPlayerTags();
#endif
//---------------------------VOIDS-BOOLS---------------------------//

#ifdef ENABLE_SPECIAL_STORAGE
extern TR1_NS::unordered_set<DWORD> m_setUpgradeItem;
extern TR1_NS::unordered_set<DWORD> m_setBookItem;
extern TR1_NS::unordered_set<DWORD> m_setStoneItem;
extern TR1_NS::unordered_set<DWORD> m_setChestItem;

extern void InitializeUpgradeItems();
extern bool CheckUpgradeItems(DWORD dwVnum);

extern void InitializeBookItems();
extern bool CheckBookItems(DWORD dwVnum);

extern void InitializeStoneItems();
extern bool CheckStoneItems(DWORD dwVnum);

extern void InitializeChestItems();
extern bool CheckChestItems(DWORD dwVnum);
#endif

#ifdef __AUTO_HUNT__
extern TR1_NS::unordered_set<long> m_setAutoHuntMap;
extern void InitializeAutoHuntMaps();
extern bool CheckAutoHuntMaps(long lMapIndex);
#endif

#ifdef ENABLE_DEFENCE_SHIELD
extern TR1_NS::unordered_set<long> m_setDefenceShieldMap;
extern void InitializeDefenceShieldMaps();
extern bool CheckDefenceShieldMaps(long lMapIndex);
#endif

#ifdef ENABLE_PCBAN_SYSTEM
extern int g_iPcBanKey;
extern int g_iPcDefaultKey;
extern void LoadPcBanKey();
#endif

#ifdef ENABLE_URIEL_BAN
extern void CheckUrielBan();
#endif

#ifdef ENABLE_PUBLISHER_SYSTEM
extern std::map<int, TPublisherInfo> m_mapPublisherInfo;
extern void InitializePublisherSystem(bool bReload = false);
extern void OnLoginPublisher(LPCHARACTER ch);
#endif

#ifdef BOSS_DMG_RANKING
extern std::map<DWORD, std::vector<std::pair<DWORD, short>>> m_mapBossDmgRankingReward;
extern bool Boss_Dmg_Ranking_Load();
#endif

#endif
