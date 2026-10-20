#pragma once

#ifdef ENABLE_WHEEL_OF_FORTUNE_SYSTEM
typedef std::map<BYTE, std::pair<int, int>> T_MAP_TYPE_LIST;
typedef std::map<std::pair<int, int>, int> T_MAP_ITEM_LIST;
typedef std::map<BYTE, T_MAP_ITEM_LIST> T_MAP_WHEEL_LIST;

typedef std::map<BYTE, std::vector<TWheelOfFortuneInfoTable>> T_VEC_ITEM_LIST;
// typedef std::vector<TWheelOfFortuneInfoTable> T_VEC_ITEM_LIST;

class CWheelOfFortune : public singleton<CWheelOfFortune>
{
	private:
		typedef T_MAP_TYPE_LIST TMapTypeList;
		TMapTypeList m_mapTypeList;

		typedef T_MAP_ITEM_LIST TMapItemList;
		typedef T_MAP_WHEEL_LIST TMapWheelList;
		TMapWheelList m_mapWheelList;

		typedef T_VEC_ITEM_LIST TVecItemList;
		TVecItemList m_vecWheelList;

	public:
		CWheelOfFortune();
		virtual ~CWheelOfFortune();

		#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
		bool GetWheelStatus(BYTE bType);
		#endif

		int NeedItemVnum(BYTE bType);
		int NeedItemCount(BYTE bType);

		bool IsBusy(LPCHARACTER ch);

		void Destroy();
		void LoadWheel(BYTE bType);
		void GetItemList(BYTE bType);
		void Initialize();
		void Reload();
		void Status(bool status) { m_bActiveStatus = status; }

		void Open(LPCHARACTER ch, BYTE bType);
		void Fortune(LPCHARACTER ch, BYTE bType);

	protected:
		bool m_bActiveStatus;
};
#endif
