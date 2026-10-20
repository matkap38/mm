#pragma once
class CDragonCoinShopManager : public singleton<CDragonCoinShopManager>
{
	typedef struct SIShopStatus
	{
		DWORD	iID;
		bool	isSocketStatus;
		bool	isAttrStatus;
	} TIShopStatus;

	typedef std::vector<TGCIShopItem> TDcShopItemVec;
	typedef std::vector<TGCIShopCategory> TDcShopCateVec;
	typedef std::vector<TIShopStatus> TDcShopStatVec;

	typedef std::unordered_map<DWORD, std::vector<TDcShopLog>> TMapDcShopBuyLog;

	#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
	typedef std::vector<TGCIShopItemAdmin> TDcShopItemAdminVec;
	#endif

	public:
		CDragonCoinShopManager(void);
		virtual ~CDragonCoinShopManager(void);

		void	Destroy();
		void	Install();
		void	ReInstall();

		bool	LoadDcShopItemList();
		bool	LoadDcShopCateList();
		bool	LoadDcShopLogList();

		void	SendPopupMessage(LPCHARACTER ch, const std::string& message, int iData = 0);

		void	OpenItemShop(LPCHARACTER ch);
		void	SendCategoryItemShop(LPCHARACTER ch);
		void	BuyItemShop(LPCHARACTER ch, DWORD ItemID, BYTE ItemCount, BYTE bItemPriceType = ITEMSHOP_SELL_TYPE_COINS);

		void	SetDcShopStatus(bool status) { m_DcShopStatus = status; }
		bool	GetDcShopStatus() const { return m_DcShopStatus; }

		void	SetBuyLog(LPCHARACTER ch, DWORD itemVnum, BYTE itemCount, DWORD itemCoins, BYTE bItemPriceType);
		void	SetBuyLogP2p(DWORD dwAID, TDcShopLog * table);
		void	OpenLog(LPCHARACTER ch);

		#ifdef ENABLE_DRAGON_COIN_EM_MARKET
		bool	LoadDcShopEmItemList();
		bool	LoadDcShopEmCateList();

		void	OpenEmItemShop(LPCHARACTER ch);
		void	SendCategoryEmItemShop(LPCHARACTER ch);
		void	BuyEmItemShop(LPCHARACTER ch, DWORD ItemID, BYTE ItemCount);
		#endif

		#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
		bool	LoadDcShopItemListAdmin();

		void	SendCategoryItemShopAdmin(LPCHARACTER ch);
		void	OpenItemShopAdmin(LPCHARACTER ch);

		void	AddCateItemShopAdmin(const char * c_pszCateName);
		void	AddSubCateItemShopAdmin(int iCateID, const char * c_pszCateName);

		void	DeleteCateItemShopAdmin(int iCateID);
		void	DeleteSubCateItemShopAdmin(int iSubCateID);

		void	RenameCateItemShopAdmin(int iCateID, const char * c_pszCateName);

		void	DeleteItemShopAdmin(int iID);
		void	AddItemShopAdmin(TGCIShopItemAdmin * t);
		void	UpdateItemShopAdmin(TGCIShopItemAdmin * t);
		#endif

	protected:
		TDcShopItemVec		m_DcShopItemVec;
		TDcShopCateVec		m_DcShopCateVec;
		TDcShopStatVec		m_DcShopStatVec;

		#ifdef ENABLE_DRAGON_COIN_EM_MARKET
		TDcShopItemVec		m_DcShopEmItemVec;
		TDcShopCateVec		m_DcShopEmCateVec;
		TDcShopStatVec		m_DcShopEmStatVec;
		#endif

		bool				m_DcShopStatus;

		TMapDcShopBuyLog	m_mapDcShopBuyLog;

		#ifdef ENABLE_DRAGON_COIN_SHOP_ADMIN_SYSTEM
		TDcShopItemAdminVec	m_DcShopItemAdminVec;
		#endif
};
