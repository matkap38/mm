#pragma once

#ifdef ENABLE_BLACK_MARKET_SYSTEM
class CBlackMarket : public singleton<CBlackMarket>
{
	private:
		typedef std::vector<TBlackMarketData> TBlackMarketVec;
		TBlackMarketVec	m_vec_pkBlackMarket;

	public:
		typedef struct SBlackMarket
		{
			// Functions
			SBlackMarket();

			DWORD				dwVnum;
			#ifdef ENABLE_FULL_YANG
			int64_t				llPrice;
			#else
			long				llPrice;
			#endif
			int					iStock;
			bool				bStockP2p;

		} TBlackMarket;

		typedef std::map<DWORD, TBlackMarket> TBlackMarketMap;

		CBlackMarket();
		virtual ~CBlackMarket();

		void	Initialize();

		bool	GetPriceData(DWORD dwVnum, TBlackMarket ** pPriceData);

		#ifdef ENABLE_FULL_YANG
		int64_t	GetGoldPrice(DWORD dwVnum);
		#else
		long	GetGoldPrice(DWORD dwVnum);
		#endif

		int		GetStock(DWORD dwVnum);
		void	SetStock(DWORD dwVnum, int iStock, bool bP2p = true);

		void	GetPriceList(LPCHARACTER ch);

	protected:
		TBlackMarketMap	m_map_pkBlackMarket;
};
#endif//End
