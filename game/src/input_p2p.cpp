#include "stdafx.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "party.h"
#include "messenger_manager.h"
#include "empire_text_convert.h"
#include "unique_item.h"
#include "xmas_event.h"
#include "affect.h"
#include "castle.h"
#include "locale_service.h"
#include "questmanager.h"
#include "skill.h"
#include "threeway_war.h"
#include "LoadTxtToTable.h"

#ifdef ENABLE_RELOAD_COMMANDS
#include "item_manager.h"
#include "shop_manager.h"
#endif

#ifdef ENABLE_OFFER_SYSTEM
#include "OfferSystem.h"
#endif

#ifdef ENABLE_CUBE_RENEWAL
#include "cuberenewal.h"
#endif

#ifdef ENABLE_TELEPORTION_WIZARD_SYSTEM
#include "teleportion_wizard.h"
#endif

#ifdef ENABLE_TANAKA_EVENT
#include "NewTanakaEvent.h"
#endif

#ifdef ENABLE_BLACK_MARKET_SYSTEM
#include "blackmarket.h"
#endif

#ifdef NEW_MOB_TIMER
#include "new_mob_timer.h"
#endif

#ifdef ENABLE_LOGIN_WARNING_MESSAGE_SYSTEM
#include "login_warning_message.h"
#endif

#ifdef ENABLE_WHEEL_OF_FORTUNE_SYSTEM
#include "wheel_of_fortune.h"
#endif

#ifdef ENABLE_IKASHOP_RENEWAL
#include "ikarus_shop.h"
#include "ikarus_shop_manager.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// Input Processor
CInputP2P::CInputP2P()
{
	BindPacketInfo(&m_packetInfoGG);
}

void CInputP2P::Login(LPDESC d, const char * c_pData)
{
	P2P_MANAGER::instance().Login(d, (TPacketGGLogin *) c_pData);
}

void CInputP2P::Logout(LPDESC d, const char * c_pData)
{
	TPacketGGLogout * p = (TPacketGGLogout *) c_pData;
	P2P_MANAGER::instance().Logout(p->szName);
}

int CInputP2P::Relay(LPDESC d, const char * c_pData, size_t uiBytes)
{
	TPacketGGRelay * p = (TPacketGGRelay *) c_pData;

	if (uiBytes < sizeof(TPacketGGRelay) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	sys_log(0, "InputP2P::Relay : %s size %d", p->szName, p->lSize);

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(p->szName);

	const BYTE* c_pbData = (const BYTE *) (c_pData + sizeof(TPacketGGRelay));

	if (!pkChr)
		return p->lSize;

	if (*c_pbData == HEADER_GC_WHISPER)
	{
		if (pkChr->IsBlockMode(BLOCK_WHISPER))
		{

			return p->lSize;
		}

		char buf[1024];
		memcpy(buf, c_pbData, MIN(p->lSize, sizeof(buf)));

		TPacketGCWhisper* p2 = (TPacketGCWhisper*) buf;

		BYTE bToEmpire = (p2->bType >> 4);
		p2->bType = p2->bType & 0x0F;
		if(p2->bType == 0x0F) {

			p2->bType = WHISPER_TYPE_SYSTEM;
		} else {
			if (!pkChr->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
				if (bToEmpire >= 1 && bToEmpire <= 3 && pkChr->GetEmpire() != bToEmpire)
				{
					ConvertEmpireText(bToEmpire,
							buf + sizeof(TPacketGCWhisper),
							p2->wSize - sizeof(TPacketGCWhisper),
							10+2*pkChr->GetSkillPower(SKILL_LANGUAGE1 + bToEmpire - 1));
				}
		}

		pkChr->GetDesc()->Packet(buf, p->lSize);
	}
	else
		pkChr->GetDesc()->Packet(c_pbData, p->lSize);

	return (p->lSize);
}

#ifdef ENABLE_FULL_NOTICE
int CInputP2P::Notice(LPDESC d, const char * c_pData, size_t uiBytes, bool bBigFont)
#else
int CInputP2P::Notice(LPDESC d, const char * c_pData, size_t uiBytes)
#endif
{
	TPacketGGNotice * p = (TPacketGGNotice *) c_pData;

	if (uiBytes < sizeof(TPacketGGNotice) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256+1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGNotice), MIN(p->lSize + 1, sizeof(szBuf)));
	#ifdef ENABLE_FULL_NOTICE
	SendNotice(szBuf, bBigFont);
	#else
	SendNotice(szBuf);
	#endif
	return (p->lSize);
}

int CInputP2P::MonarchNotice(LPDESC d, const char * c_pData, size_t uiBytes)
{
	TPacketGGMonarchNotice * p = (TPacketGGMonarchNotice *) c_pData;

	if (uiBytes < p->lSize + sizeof(TPacketGGMonarchNotice))
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256+1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGMonarchNotice), MIN(p->lSize + 1, sizeof(szBuf)));
	SendMonarchNotice(p->bEmpire, szBuf);
	return (p->lSize);
}

int CInputP2P::MonarchTransfer(LPDESC d, const char* c_pData)
{
	TPacketMonarchGGTransfer* p = (TPacketMonarchGGTransfer*) c_pData;
	LPCHARACTER pTargetChar = CHARACTER_MANAGER::instance().FindByPID(p->dwTargetPID);

	if (pTargetChar != NULL)
	{
		unsigned int qIndex = quest::CQuestManager::instance().GetQuestIndexByName("monarch_transfer");
		if (qIndex != 0)
		{
			pTargetChar->SetQuestFlag("monarch_transfer.x", p->x);
			pTargetChar->SetQuestFlag("monarch_transfer.y", p->y);
			quest::CQuestManager::instance().Letter(pTargetChar->GetPlayerID(), qIndex, 0);
		}
	}

	return 0;
}

int CInputP2P::Guild(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGGuild * p = (TPacketGGGuild *) c_pData;
	uiBytes -= sizeof(TPacketGGGuild);
	c_pData += sizeof(TPacketGGGuild);

	CGuild * g = CGuildManager::instance().FindGuild(p->dwGuild);

	switch (p->bSubHeader)
	{
		case GUILD_SUBHEADER_GG_CHAT:
			{
				if (uiBytes < sizeof(TPacketGGGuildChat))
					return -1;

				TPacketGGGuildChat * p = (TPacketGGGuildChat *) c_pData;

				if (g)
					g->P2PChat(p->szText);

				return sizeof(TPacketGGGuildChat);
			}

		case GUILD_SUBHEADER_GG_SET_MEMBER_COUNT_BONUS:
			{
				if (uiBytes < sizeof(int))
					return -1;

				int iBonus = *((int *) c_pData);
				CGuild* pGuild = CGuildManager::instance().FindGuild(p->dwGuild);
				if (pGuild)
				{
					pGuild->SetMemberCountBonus(iBonus);
				}
				return sizeof(int);
			}

		default:
			sys_err ("UNKNOWN GUILD SUB PACKET");
			break;
	}

	return 0;
}

struct FuncShout
{
	const char * m_str;
	BYTE m_bEmpire;

	FuncShout(const char * str, BYTE bEmpire) : m_str(str), m_bEmpire(bEmpire)
	{
	}

	void operator () (LPDESC d)
	{
		#ifdef ENABLE_NEWSTUFF
		if (!d->GetCharacter() || (!g_bGlobalShoutEnable && d->GetCharacter()->GetGMLevel() == GM_PLAYER && d->GetEmpire() != m_bEmpire))
			return;
		#else
		if (!d->GetCharacter() || (d->GetCharacter()->GetGMLevel() == GM_PLAYER && d->GetEmpire() != m_bEmpire))
			return;
		#endif

		if (!d->IsPhase(PHASE_GAME))//Fix Chat
			return;

		d->GetCharacter()->ChatPacket(CHAT_TYPE_SHOUT, "%s", m_str);
	}
};

void SendShout(const char * szText, BYTE bEmpire)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), FuncShout(szText, bEmpire));
}

void CInputP2P::Shout(const char * c_pData)
{
	TPacketGGShout * p = (TPacketGGShout *) c_pData;
	SendShout(p->szText, p->bEmpire);
}

void CInputP2P::Disconnect(const char * c_pData)
{
	TPacketGGDisconnect * p = (TPacketGGDisconnect *) c_pData;

	LPDESC d = DESC_MANAGER::instance().FindByLoginName(p->szLogin);
	if (!d)
		return;

	if (!d->GetCharacter())
	{
		d->SetPhase(PHASE_CLOSE);
	}
	else
		d->DisconnectOfSameLogin();
}

void CInputP2P::DisconnectPlayer(const char * c_pData)
{
	TPacketGGDisconnectPlayer * p = (TPacketGGDisconnectPlayer *) c_pData;
	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(p->szName);
	if (tch) { tch->GetDesc()->DelayedDisconnect(p->iSecond); }
}

void CInputP2P::Setup(LPDESC d, const char * c_pData)
{
	TPacketGGSetup * p = (TPacketGGSetup *) c_pData;
	sys_log(0, "P2P: Setup %s:%d", d->GetHostName(), p->wPort);
	d->SetP2P(d->GetHostName(), p->wPort, p->bChannel);
}

void CInputP2P::MessengerAdd(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	sys_log(0, "P2P: Messenger Add %s %s", p->szAccount, p->szCompanion);
	MessengerManager::instance().__AddToList(p->szAccount, p->szCompanion);
}

void CInputP2P::MessengerRemove(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	sys_log(0, "P2P: Messenger Remove %s %s", p->szAccount, p->szCompanion);
	MessengerManager::instance().__RemoveFromList(p->szAccount, p->szCompanion);
}

#ifdef ENABLE_MESSENGER_BLOCK
void CInputP2P::MessengerBlockAdd(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	MessengerManager::instance().__AddToBlockList(p->szAccount, p->szCompanion);
}

void CInputP2P::MessengerBlockRemove(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	MessengerManager::instance().__RemoveFromBlockList(p->szAccount, p->szCompanion);
}
#endif

void CInputP2P::FindPosition(LPDESC d, const char* c_pData)
{
	TPacketGGFindPosition* p = (TPacketGGFindPosition*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->dwTargetPID);
	#ifdef ENABLE_CMD_WARP_IN_DUNGEON
	if (ch)
	#else
	if (ch && ch->GetMapIndex() < 10000)
	#endif
	{
		TPacketGGWarpCharacter pw;
		pw.pid = p->dwFromPID;
		pw.x = ch->GetX();
		pw.y = ch->GetY();

		if (p->IsGmWarp)
		{
			#ifdef ENABLE_CMD_WARP_IN_DUNGEON
			pw.mapIndex = ch->GetMapIndex();
			#endif
			#ifdef ENABLE_WARP_REGULATION
			pw.port = mother_port;
			#endif
		}
		else
		{
			#ifdef ENABLE_CMD_WARP_IN_DUNGEON
			pw.mapIndex = (ch->GetMapIndex() < 10000) ? 0 : ch->GetMapIndex();
			#endif
		}

		d->Packet(&pw, sizeof(pw));
	}
}

void CInputP2P::WarpCharacter(const char* c_pData)
{
	TPacketGGWarpCharacter* p = (TPacketGGWarpCharacter*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->pid);
	if (ch)
	{
		ch->WarpSet(p->x, p->y
			#ifdef ENABLE_CMD_WARP_IN_DUNGEON
			, p->mapIndex
			#endif
			#ifdef ENABLE_WARP_REGULATION
			, p->port
			#endif
		);
	}
}

void CInputP2P::GuildWarZoneMapIndex(const char* c_pData)
{
	TPacketGGGuildWarMapIndex * p = (TPacketGGGuildWarMapIndex*) c_pData;
	CGuildManager & gm = CGuildManager::instance();

	sys_log(0, "P2P: GuildWarZoneMapIndex g1(%u) vs g2(%u), mapIndex(%d)", p->dwGuildID1, p->dwGuildID2, p->lMapIndex);

	CGuild * g1 = gm.FindGuild(p->dwGuildID1);
	CGuild * g2 = gm.FindGuild(p->dwGuildID2);

	if (g1 && g2)
	{
		g1->SetGuildWarMapIndex(p->dwGuildID2, p->lMapIndex);
		g2->SetGuildWarMapIndex(p->dwGuildID1, p->lMapIndex);
	}
}

void CInputP2P::Transfer(const char * c_pData)
{
	TPacketGGTransfer * p = (TPacketGGTransfer *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);
	if (ch)
	{
		ch->WarpSet(p->lX, p->lY
			#ifdef ENABLE_CMD_WARP_IN_DUNGEON
			, p->lMapIndex
			#endif
			#ifdef ENABLE_WARP_REGULATION
			, p->wPort
			#endif
		);
	}
}

void CInputP2P::XmasWarpSanta(const char * c_pData)
{
	TPacketGGXmasWarpSanta * p =(TPacketGGXmasWarpSanta *) c_pData;

	if (p->bChannel == g_bChannel && map_allow_find(p->lMapIndex))
	{
		int iNextSpawnDelay = 50 * 60;

		xmas::SpawnSanta(p->lMapIndex, iNextSpawnDelay);

		TPacketGGXmasWarpSantaReply pack_reply;
		pack_reply.bHeader = HEADER_GG_XMAS_WARP_SANTA_REPLY;
		pack_reply.bChannel = g_bChannel;
		P2P_MANAGER::instance().Send(&pack_reply, sizeof(pack_reply));
	}
}

void CInputP2P::XmasWarpSantaReply(const char* c_pData)
{
	TPacketGGXmasWarpSantaReply* p = (TPacketGGXmasWarpSantaReply*) c_pData;

	if (p->bChannel == g_bChannel)
	{
		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(xmas::MOB_SANTA_VNUM, i))
		{
			CharacterVectorInteractor::iterator it = i.begin();

			while (it != i.end())
			{
				M2_DESTROY_CHARACTER(*it++);
			}
		}
	}
}

void CInputP2P::LoginPing(LPDESC d, const char * c_pData)
{
	TPacketGGLoginPing * p = (TPacketGGLoginPing *) c_pData;

	if (!g_pkAuthMasterDesc) // If I am master, I have to broadcast
		P2P_MANAGER::instance().Send(p, sizeof(TPacketGGLoginPing), d);
}

// BLOCK_CHAT
void CInputP2P::BlockChat(const char * c_pData)
{
	TPacketGGBlockChat * p = (TPacketGGBlockChat *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
	{
		sys_log(0, "BLOCK CHAT apply name %s dur %d", p->szName, p->lBlockDuration);
		ch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, p->lBlockDuration, 0, true);
	}
	else
	{
		sys_log(0, "BLOCK CHAT fail name %s dur %d", p->szName, p->lBlockDuration);
	}
}
// END_OF_BLOCK_CHAT
//

#ifdef ENABLE_MULTI_FARM_BLOCK
void CInputP2P::MultiFarm(const char* c_pData)
{
	TPacketGGMultiFarm* p = (TPacketGGMultiFarm*)c_pData;
	if(p->subHeader == MULTI_FARM_SET)
		CHARACTER_MANAGER::Instance().CheckMultiFarmAccount(p->playerIP, p->playerID, p->playerName, p->farmStatus, p->affectType, p->affectTime, true);
	else if (p->subHeader == MULTI_FARM_REMOVE)
		CHARACTER_MANAGER::Instance().RemoveMultiFarm(p->playerIP, p->playerID, true);
}
#endif

#ifdef ENABLE_MULTI_HUNT_BLOCK
void CInputP2P::MultiHunt(const char* c_pData)
{
	TPacketGGMultiHunt* p = (TPacketGGMultiHunt*)c_pData;
	if(p->subHeader == MULTI_HUNT_SET)
		CHARACTER_MANAGER::Instance().CheckMultiHuntAccount(p->playerIP, p->playerID, p->playerName, p->huntStatus, p->affectType, p->affectTime, true);
	else if (p->subHeader == MULTI_HUNT_REMOVE)
		CHARACTER_MANAGER::Instance().RemoveMultiHunt(p->playerIP, p->playerID, true);
}
#endif

#ifdef ENABLE_MULTI_FISH_BLOCK
void CInputP2P::MultiFish(const char* c_pData)
{
	TPacketGGMultiFish* p = (TPacketGGMultiFish*)c_pData;
	if(p->subHeader == MULTI_FISH_SET)
		CHARACTER_MANAGER::Instance().CheckMultiFishAccount(p->playerIP, p->playerID, p->playerName, p->fishStatus, p->affectType, p->affectTime, true);
	else if (p->subHeader == MULTI_FISH_REMOVE)
		CHARACTER_MANAGER::Instance().RemoveMultiFish(p->playerIP, p->playerID, true);
}
#endif

#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
void CInputP2P::Switchbot(LPDESC d, const char* c_pData)
{
	const TPacketGGSwitchbot* p = reinterpret_cast<const TPacketGGSwitchbot*>(c_pData);
	if (p->wPort != mother_port)
	{
		return;
	}

	CSwitchbotManager::Instance().P2PReceiveSwitchbot(p->table);
}
#endif

#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
#include "nm_item_shop.h"

void CInputP2P::DcShopRefresh(const char* c_pData)
{
	const TPacketGGDcShopRefresh* p = reinterpret_cast<const TPacketGGDcShopRefresh*>(c_pData);

	if (p->bProcess == 1)
	{
		CDragonCoinShopManager::instance().SetDcShopStatus(true);
	}
	else if (p->bProcess == 2)
	{
		CDragonCoinShopManager::instance().SetDcShopStatus(false);
	}
	else if (p->bProcess == 3)
	{
		CDragonCoinShopManager::instance().SetDcShopStatus(false);
		CDragonCoinShopManager::instance().Destroy();
		CDragonCoinShopManager::instance().Install();
		CDragonCoinShopManager::instance().SetDcShopStatus(true);
	}
}

int CInputP2P::DcShopBuy(const char *c_pData, size_t uiBytes)
{
	TPacketGGDcShopBuy *p = (TPacketGGDcShopBuy*)c_pData;
	uiBytes -= sizeof(TPacketGGDcShopBuy);
	c_pData += sizeof(TPacketGGDcShopBuy);

	if (uiBytes < sizeof(TDcShopLog))
		return -1;

	CDragonCoinShopManager::instance().SetBuyLogP2p(p->dwAID, (TDcShopLog*)c_pData);
	return sizeof(TDcShopLog);
}
#endif

#ifdef ENABLE_PM_ALL_SEND_SYSTEM
int CInputP2P::BulkWhisperSend(LPDESC d, const char * c_pData, size_t uiBytes)
{
	TPacketGGBulkWhisper * p = (TPacketGGBulkWhisper *)c_pData;

	if (uiBytes < sizeof(TPacketGGBulkWhisper) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[CHAT_MAX_LEN + 1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGBulkWhisper), MIN(p->lSize + 1, sizeof(szBuf)));
	SendBulkWhisper(szBuf);

	return (p->lSize);
}
#endif

#ifdef ENABLE_BAKIM_DUYURU
void CInputP2P::MaintenanceInfo(const char* c_pData)
{
	TPacketGGMaintenanceInfo* p = (TPacketGGMaintenanceInfo*) c_pData;
	DESC_MANAGER::Instance().SetMaintenanceInfo(p->info);
}
#endif

void CInputP2P::IamAwake(LPDESC d, const char * c_pData)
{
	std::string hostNames;
	P2P_MANAGER::instance().GetP2PHostNames(hostNames);
	sys_log(0, "P2P Awakeness check from %s. My P2P connection number is %d. and details...\n%s", d->GetHostName(), P2P_MANAGER::instance().GetDescCount(), hostNames.c_str());
}

#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
void CInputP2P::GuildWarOption(const char* c_pData)
{
	const TPacketGGGuildWarOption* p = reinterpret_cast<const TPacketGGGuildWarOption*>(c_pData);

	CGuild * g = CGuildManager::instance().FindGuild(p->dwGuild1);
	if (g)
	{
		g->SetWarScore(p->iScore);
		g->SetWarMinLevel(p->iMinLevel);
		g->SetWarMaxPlayer(p->iMaxPlayer);
		g->SetWarWarriorStatus(p->bWarrior);
		g->SetWarAssassinStatus(p->bAssassin);
		g->SetWarSuraStatus(p->bSura);
		g->SetWarShamanStatus(p->bShaman);
	}

	CGuild * opp_g = CGuildManager::instance().FindGuild(p->dwGuild2);
	if (opp_g)
	{
		opp_g->SetWarScore(p->iScore);
		opp_g->SetWarMinLevel(p->iMinLevel);
		opp_g->SetWarMaxPlayer(p->iMaxPlayer);
		opp_g->SetWarWarriorStatus(p->bWarrior);
		opp_g->SetWarAssassinStatus(p->bAssassin);
		opp_g->SetWarSuraStatus(p->bSura);
		opp_g->SetWarShamanStatus(p->bShaman);
	}
}

void CInputP2P::GuildWarMember(const char* c_pData)
{
	const TPacketGGGuildWarMember* p = reinterpret_cast<const TPacketGGGuildWarMember*>(c_pData);
	CGuild * g = CGuildManager::instance().FindGuild(p->dwGuild);
	if (g) { g->SetWarMemberCount(p->iMemCount); }
}
#endif

#ifdef __BACK_DUNGEON__
void CInputP2P::BackDungeon(const char* c_pData)
{
	const TPacketGGBackDungeon* p = (TPacketGGBackDungeon*)c_pData;
	CHARACTER_MANAGER::Instance().SetBackDungeon(false, p->bIsSave, p->wPort, p->bChannel, p->dwMapIdx, p->dwPID, p->lX, p->lY, p->iLeftTime);
}
#endif

#ifdef ENABLE_DISCONNECT_ALL_PLAYERS
void CInputP2P::DisconnectAll(const char* c_pData)
{
	const TPacketGGDisconnectAll* p = reinterpret_cast<const TPacketGGDisconnectAll*>(c_pData);
	bool bQuit = (p->bProcess == 1) ? true : false;
	CheckDisconnectAll(bQuit);
}
#endif

#ifdef ENABLE_BLACK_MARKET_SYSTEM
void CInputP2P::BlackMarket(const char * c_pData)
{
	TPacketGGBlackMarket * p = (TPacketGGBlackMarket *) c_pData;
	if (p->bSubHeader == BM_SUBHEADER_RELOAD)
	{
		CBlackMarket::instance().Initialize();
	}
	else if (p->bSubHeader == BM_SUBHEADER_REFRESH)
	{
		CBlackMarket::instance().SetStock(p->dwVnum, p->iStock, false);
	}
}
#endif

#ifdef ENABLE_COLLECTIVE_RELOAD
void CInputP2P::CollectiveReload(const char * c_pData)
{
	TPacketGGCollectiveReload * p = (TPacketGGCollectiveReload *) c_pData;
	switch (p->bSubHeader)
	{
		#ifdef ENABLE_COSTUME_SET_BONUS
		case CRELOAD_COSTUME_SET_BONUS: { CHARACTER_MANAGER::instance().InitializeCostumeSetBonus(); } return;
		#endif
		#ifdef ENABLE_BLACK_MARKET_SYSTEM
		case CRELOAD_BLACKMARKET: { CBlackMarket::instance().Initialize(); } return;
		#endif
		#ifdef ENABLE_RELOAD_COMMANDS
		case CRELOAD_MOB_DROP: { ITEM_MANAGER::Instance().ReloadMobDropItem(); } return;
		case CRELOAD_SPECIAL_DROP: { ITEM_MANAGER::Instance().ReloadSpecialDropItem(); } return;
		#endif
		#ifdef ENABLE_ALIGNMENT_BONUS
		case CRELOAD_ALIGNMENT_BONUS: { LoadAlignmentBonus(); } return;
		#endif
		#ifdef ENABLE_PLAYER_TAG_SYSTEM
		case CRELOAD_PLAYER_TAG: { LoadPlayerTags(); } return;
		#endif
		#ifdef NEW_MOB_TIMER
		case CRELOAD_MOB_TIMER: { NewMobTimer::CNewMobTimer::Instance().Initialize(); } return;
		#endif
		#ifdef ENABLE_TELEPORTION_WIZARD_SYSTEM
		case CRELOAD_TELEPORTION_WIZARD: { CTeleportionWizard::instance().Reload(); } return;
		#endif
		#ifdef ENABLE_LOGIN_WARNING_MESSAGE_SYSTEM
		case CRELOAD_LWM: { CLoginWarningMsg::instance().Reload(); } return;
		#endif
		#ifdef ENABLE_WHEEL_OF_FORTUNE_SYSTEM
		case CRELOAD_WHEEL_OF_FORTUNE: { CWheelOfFortune::instance().Reload(); } return;
		#endif

		#ifdef ENABLE_CUBE_RENEWAL
		case CRELOAD_CUBE:
			{
				#if defined(CUBE_RENEWAL_RELOAD_FIX)
				Cube_init(true);
				#else
				Cube_init();
				#endif
			}
			return;
		#endif

		#ifdef ENABLE_OFFER_SYSTEM
		case CRELOAD_OFFER:
			{
				COfferSystem::instance().SetOfferStatus(false);
				COfferSystem::instance().Destroy();
				COfferSystem::instance().Install();
				COfferSystem::instance().SetOfferStatus(true);

				const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
				for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
				{
					LPCHARACTER ch = (*it)->GetCharacter();
					if (!ch) { continue; }
					ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowOfferIcon");
				}
			}
			return;
		#endif

		#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
		case CRELOAD_DRAGON_COIN_SHOP:
			{
				CDragonCoinShopManager::instance().SetDcShopStatus(false);
				CDragonCoinShopManager::instance().Destroy();
				CDragonCoinShopManager::instance().Install();
				CDragonCoinShopManager::instance().SetDcShopStatus(true);
			}
			return;
		#endif

		#ifdef ENABLE_SPECIAL_STORAGE
		case CRELOAD_SPECIAL_STORAGE:
			{
				InitializeUpgradeItems();
				InitializeBookItems();
				InitializeStoneItems();
				InitializeChestItems();
			}
			return;
		#endif

		#ifdef __AUTO_HUNT__
		case CRELOAD_AUTO_HUNT:
			{
				InitializeAutoHuntMaps();
			}
			return;
		#endif

		#ifdef ENABLE_DEFENCE_SHIELD
		case CRELOAD_DEFENCE_SHIELD:
			{
				InitializeDefenceShieldMaps();
			}
			return;
		#endif

		#ifdef ENABLE_PCBAN_SYSTEM
		case CRELOAD_PC_BAN:
			{
				LoadPcBanKey();
			}
			return;
		#endif

		#ifdef ENABLE_PUBLISHER_SYSTEM
		case CRELOAD_PUBLISHER:
			{
				InitializePublisherSystem(true);
			}
			return;
		#endif

		#ifdef BOSS_DMG_RANKING
		case CRELOAD_BOSS_DMG_RANKING:
			{
				Boss_Dmg_Ranking_Load();
			}
			return;
		#endif

		#ifdef ENABLE_REWARD_SYSTEM
		case CRELOAD_REWARD:
			{
				CHARACTER_MANAGER::instance().LoadRewardData();
			}
			return;
		#endif

		#ifdef ENABLE_IKASHOP_AVG_AND_MINMAX_PRICE
		case CRELOAD_IKASHOP_MINMAX:
			{
				ikashop::GetManager().LoadMinMaxPriceTable();
			}
			return;
		#endif
	}
}
#endif

#ifdef ENABLE_DC_TO_CHR_OTHER_CH
void CInputP2P::DcToChrOtherCh(const char* c_pData)
{
	TPacketGGDcToChr* p = (TPacketGGDcToChr*) c_pData;
	LPDESC d = DESC_MANAGER::instance().FindByCharacterName(p->szName);
	LPCHARACTER tch = d ? d->GetCharacter() : NULL;
	if (!tch) { return; }
	DESC_MANAGER::instance().DestroyLoginKey(d);
	DESC_MANAGER::instance().DestroyDesc(d);
}
#endif

int CInputP2P::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	if (test_server)
		sys_log(0, "CInputP2P::Anlayze[Header %d]", bHeader);

	int iExtraLen = 0;

	switch (bHeader)
	{
		case HEADER_GG_SETUP:
			Setup(d, c_pData);
			break;

		case HEADER_GG_LOGIN:
			Login(d, c_pData);
			break;

		case HEADER_GG_LOGOUT:
			Logout(d, c_pData);
			break;

		case HEADER_GG_RELAY:
			if ((iExtraLen = Relay(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		#ifdef ENABLE_FULL_NOTICE
		case HEADER_GG_BIG_NOTICE:
			if ((iExtraLen = Notice(d, c_pData, m_iBufferLeft, true)) < 0)
				return -1;
			break;
		#endif

		case HEADER_GG_NOTICE:
			if ((iExtraLen = Notice(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_SHUTDOWN:
			sys_err("Accept shutdown p2p command from %s.", d->GetHostName());
			Shutdown(10);
			break;

		case HEADER_GG_GUILD:
			if ((iExtraLen = Guild(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_SHOUT:
			Shout(c_pData);
			break;

		case HEADER_GG_DISCONNECT:
			Disconnect(c_pData);
			break;

		case HEADER_GG_DISCONNECT_PLAYER:
			DisconnectPlayer(c_pData);
			break;

		case HEADER_GG_MESSENGER_ADD:
			MessengerAdd(c_pData);
			break;

		case HEADER_GG_MESSENGER_REMOVE:
			MessengerRemove(c_pData);
			break;

		case HEADER_GG_FIND_POSITION:
			FindPosition(d, c_pData);
			break;

		case HEADER_GG_WARP_CHARACTER:
			WarpCharacter(c_pData);
			break;

		case HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX:
			GuildWarZoneMapIndex(c_pData);
			break;

		case HEADER_GG_TRANSFER:
			Transfer(c_pData);
			break;

		case HEADER_GG_XMAS_WARP_SANTA:
			XmasWarpSanta(c_pData);
			break;

		case HEADER_GG_XMAS_WARP_SANTA_REPLY:
			XmasWarpSantaReply(c_pData);
			break;

		case HEADER_GG_RELOAD_CRC_LIST:
			LoadValidCRCList();
			break;

		case HEADER_GG_CHECK_CLIENT_VERSION:
			CheckClientVersion();
			break;

		case HEADER_GG_LOGIN_PING:
			LoginPing(d, c_pData);
			break;

		case HEADER_GG_BLOCK_CHAT:
			BlockChat(c_pData);
			break;

		case HEADER_GG_SIEGE:
			{
				TPacketGGSiege* pSiege = (TPacketGGSiege*)c_pData;
				castle_siege(pSiege->bEmpire, pSiege->bTowerCount);
			}
			break;

		case HEADER_GG_MONARCH_NOTICE:
			if ((iExtraLen = MonarchNotice(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_MONARCH_TRANSFER :
			MonarchTransfer(d, c_pData);
			break;

		case HEADER_GG_CHECK_AWAKENESS:
			IamAwake(d, c_pData);
			break;

		#ifdef ENABLE_BAKIM_DUYURU
		case HEADER_GG_BAKIM:
			MaintenanceInfo(c_pData);
			break;
		#endif

		#ifdef ENABLE_MULTI_FARM_BLOCK
		case HEADER_GG_MULTI_FARM:
			MultiFarm(c_pData);
			break;
		#endif

		#ifdef ENABLE_MULTI_HUNT_BLOCK
		case HEADER_GG_MULTI_HUNT:
			MultiHunt(c_pData);
			break;
		#endif

		#ifdef ENABLE_MULTI_FISH_BLOCK
		case HEADER_GG_MULTI_FISH:
			MultiFish(c_pData);
			break;
		#endif

		#ifdef ENABLE_SWITCHBOT
		case HEADER_GG_SWITCHBOT:
			Switchbot(d, c_pData);
			break;
		#endif

		#ifdef ENABLE_MESSENGER_BLOCK
		case HEADER_GG_MESSENGER_BLOCK_ADD:
			MessengerBlockAdd(c_pData);
			break;

		case HEADER_GG_MESSENGER_BLOCK_REMOVE:
			MessengerBlockRemove(c_pData);
			break;
		#endif

		#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
		case HEADER_GG_DC_SHOP:
			DcShopRefresh(c_pData);
			break;

		case HEADER_GG_DC_SHOP_BUY:
			if ((iExtraLen = DcShopBuy(c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
		#endif

		#ifdef ENABLE_PM_ALL_SEND_SYSTEM
		case HEADER_GG_BULK_WHISPER:
			if ((iExtraLen = BulkWhisperSend(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
		#endif

		#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
		case HEADER_GG_GUILD_WAR_OPTION:
			GuildWarOption(c_pData);
			break;

		case HEADER_GG_GUILD_WAR_MEMBER:
			GuildWarMember(c_pData);
			break;
		#endif

		#ifdef ENABLE_TANAKA_EVENT
		case HEADER_GG_TANAKA_EVENT:
		{
			const auto* p = (TTanakaGGEventState*)c_pData;
			CNewTanakaEventManager::instance().P2PAction(p->eventState);
		}
		break;
		#endif

		#ifdef __BACK_DUNGEON__
		case HEADER_GG_BACK_DUNGEON:
			BackDungeon(c_pData);
			break;
		#endif

		#ifdef ENABLE_DISCONNECT_ALL_PLAYERS
		case HEADER_GG_DISCONNECT_ALL:
			DisconnectAll(c_pData);
			break;
		#endif

		#ifdef ENABLE_BLACK_MARKET_SYSTEM
		case HEADER_GG_BLACKMARKET:
			BlackMarket(c_pData);
			break;
		#endif

		#ifdef ENABLE_COLLECTIVE_RELOAD
		case HEADER_GG_COLLECTIVE_RELOAD:
			CollectiveReload(c_pData);
			break;
		#endif

		#ifdef ENABLE_DC_TO_CHR_OTHER_CH
		case HEADER_GG_DC_TO_CHR:
			DcToChrOtherCh(c_pData);
			break;
		#endif

		#ifdef ENABLE_REWARD_SYSTEM
		case HEADER_GG_REWARD_INFO:
		{
			TPacketGGRewardInfo* data = (TPacketGGRewardInfo*)c_pData;
			CHARACTER_MANAGER::Instance().SetRewardData(data->rewardIndex, data->playerName, false, data->value);
		}
		break;
		#endif

		#ifdef ENABLE_RONARK_SYSTEM
		case HEADER_GG_RONARK:
		{
			TPacketGGRonark* data = (TPacketGGRonark*)c_pData;
			DWORD dwPID = data->dwPID;

			if (data->bProcess == 1)
			{
				auto iter = m_globalRonarkWarMemberList.find(dwPID);
				if (iter == m_globalRonarkWarMemberList.end())
				{
					m_globalRonarkWarMemberList.emplace(dwPID);
				}
			}
			else if (data->bProcess == 2)
			{
				auto iter = m_globalRonarkWarMemberList.find(dwPID);
				if (iter != m_globalRonarkWarMemberList.end())
				{
					m_globalRonarkWarMemberList.erase(dwPID);
				}
			}
			else if (data->bProcess == 3)
			{
				m_globalRonarkWarMemberList.clear();
			}
		}
		break;
		#endif
	}

	return (iExtraLen);
}

