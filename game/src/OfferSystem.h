#pragma once

#ifdef ENABLE_OFFER_SYSTEM
typedef struct SOfferInfo
{
	uint32_t	offerID;
	long		offerStartRemainSec;
	long		offerEndRemainSec;
	DWORD		offerOldPrice;
	DWORD		offerNewPrice;
}TOfferInfo;

typedef struct SOfferItemInfo
{
	DWORD	itemID;
	DWORD	itemOfferID;
	BYTE	itemPos;
	DWORD	itemCount;
	DWORD	itemVnum;
	long	alSockets[4];
	TPlayerItemAttribute    aAttr[7];
	BYTE	timeType;
	BYTE	timeSocket;
}TOfferItemInfo;

class COfferSystem : public singleton<COfferSystem>
{
	enum OSConfig
	{
		OFFER_ITEM_GRID_X = 6,
		OFFER_ITEM_GRID_Y = 3,
		OFFER_ITEM_GRID_MAX = OFFER_ITEM_GRID_X * OFFER_ITEM_GRID_Y
	};

	protected:
		std::vector	<TOfferInfo> m_offerList;
		std::vector	<TOfferItemInfo> m_offerItemList;
		
		bool				m_OfferStatus;

	public:
		COfferSystem(void);
		virtual ~COfferSystem(void);

		void	Destroy();
		void	Install();

		bool	LoadFirsatList();
		bool	LoadFirsatItemList();

		void	RecvOfferPacket(LPCHARACTER ch, BYTE reqID, DWORD offerID, DWORD offerOldPrice, DWORD offerNewPrice);
		void	BuyOffer(LPCHARACTER ch, DWORD offerID, DWORD offerOldPrice, DWORD offerNewPrice);
		bool	IsActiveOffer(DWORD offerID, DWORD offerOldPrice, DWORD offerNewPrice, LPCHARACTER ch = NULL);

		void	CheckFirsat(LPCHARACTER ch);

		void	SetOfferStatus(bool status) { m_OfferStatus = status; }
		bool	GetOfferStatus() const { return m_OfferStatus; }
};
#endif

