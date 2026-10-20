#ifndef __INC_METIN_II_GAME_SHOP_H__
#define __INC_METIN_II_GAME_SHOP_H__

enum
{
	SHOP_MAX_DISTANCE = 1000
};

class CGrid;

/* ---------------------------------------------------------------------------------- */
class CShop
{
	public:
		typedef struct shop_item
		{
			DWORD	vnum;
#ifdef ENABLE_FULL_YANG
			uint64_t	price;
#else
			long	price;
#endif
#ifdef ENABLE_NEW_MAX_COUNT
			short	count;
#else
			BYTE	count;
#endif
			LPITEM	pkItem;
			int		itemid;

			shop_item()
			{
				vnum = 0;
				price = 0;
				count = 0;
				itemid = 0;
				pkItem = NULL;
			}
		} SHOP_ITEM;

		CShop();
		virtual ~CShop(); // @fixme139 (+virtual)

		bool			Create(DWORD dwVnum, DWORD dwNPCVnum, TShopItemTable * pItemTable);

#ifdef ENABLE_NEW_MAX_COUNT
		void			SetShopItems(TShopItemTable * pItemTable, short bItemCount);
#else
		void			SetShopItems(TShopItemTable * pItemTable, BYTE bItemCount);
#endif

		virtual void	SetPCShop(LPCHARACTER ch);
		virtual bool	IsPCShop()	{ return m_pkPC ? true : false; }

		virtual bool	AddGuest(LPCHARACTER ch,DWORD owner_vid, bool bOtherEmpire);
		void			RemoveGuest(LPCHARACTER ch);
		virtual int		Buy(LPCHARACTER ch, BYTE pos);
		void			BroadcastUpdateItem(BYTE pos);
		int				GetNumberByVnum(DWORD dwVnum);
		virtual bool	IsSellingItem(DWORD itemID);

#ifdef ENABLE_RELOAD_COMMANDS
		void			RemoveAllGuests();
#endif

		DWORD	GetVnum() { return m_dwVnum; }
		DWORD	GetNPCVnum() { return m_dwNPCVnum; }

	protected:
		void	Broadcast(const void * data, int bytes);

	protected:
		DWORD				m_dwVnum;
		DWORD				m_dwNPCVnum;

		CGrid *				m_pGrid;

		typedef TR1_NS::unordered_map<LPCHARACTER, bool> GuestMapType;
		GuestMapType m_map_guest;
		std::vector<SHOP_ITEM>		m_itemVector;

		LPCHARACTER			m_pkPC;
};

#endif

