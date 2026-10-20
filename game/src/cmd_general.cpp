#include "stdafx.h"
#ifdef __FreeBSD__
#include <md5.h>
#else
#include "../../libthecore/include/xmd5.h"
#endif

#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "shop_manager.h"
#include "char.h"
#include "char_manager.h"
#include "motion.h"
#include "packet.h"
#include "affect.h"
#include "pvp.h"
#include "start_position.h"
#include "party.h"
#include "guild_manager.h"
#include "p2p.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "war_map.h"
#include "questmanager.h"
#include "item_manager.h"
#include "monarch.h"
#include "mob_manager.h"
#include "item.h"
#include "arena.h"
#include "buffer_manager.h"
#include "unique_item.h"
#include "threeway_war.h"
#include "log.h"
#include "LoadTxtToTable.h"
#include "../../common/VnumHelper.h"

#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
#include "GetEventFlagSystem.h"
#endif

ACMD(do_user_horse_ride)
{
	if (ch->IsObserverMode())
		return;

	if (ch->IsDead() || ch->IsStun())
		return;

	if (ch->IsHorseRiding() == false)
	{
		#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		if (ch->GetProtectTime("mountcostumeH") > get_global_time())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Yavaþ olmalýsýn! %d saniye sonra deneyebilirsiniz.", ch->GetProtectTime("mountcostumeH") - get_global_time());
			return;
		}
		#endif

		if (ch->GetMountVnum())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì Å»°ÍÀ» ÀÌ¿ëÁßÀÔ´Ï´Ù."));
			return;
		}

		if (ch->GetHorse() == NULL)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»À» ¸ÕÀú ¼ÒÈ¯ÇØÁÖ¼¼¿ä."));
			return;
		}

		ch->StartRiding();
		#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		ch->SetProtectTime("mountcostumeH", get_global_time() + 2);
		#endif
	}
	else
	{
		#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		if (ch->GetProtectTime("mountcostumeH") > get_global_time())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Yavaþ olmalýsýn! %d saniye sonra deneyebilirsiniz.", ch->GetProtectTime("mountcostumeH") - get_global_time());
			return;
		}
		#endif

		ch->StopRiding();
		#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		ch->SetProtectTime("mountcostumeH", get_global_time() + 2);
		#endif
	}
}

ACMD(do_user_horse_back)
{
	if (ch->GetHorse() != NULL)
	{
		ch->HorseSummon(false);
		#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		LPITEM pkCostumeMount = ch->GetWear(WEAR_COSTUME_MOUNT);
		if (pkCostumeMount)
			ch->UnequipItem(pkCostumeMount);
		#endif
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»À» µ¹·Áº¸³Â½À´Ï´Ù."));
	}
	else if (ch->IsHorseRiding() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»¿¡¼­ ¸ÕÀú ³»·Á¾ß ÇÕ´Ï´Ù."));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»À» ¸ÕÀú ¼ÒÈ¯ÇØÁÖ¼¼¿ä."));
	}
}

ACMD(do_user_horse_feed)
{
	if (ch->GetMyShop())
		return;

	if (ch->GetHorse() == NULL)
	{
		if (ch->IsHorseRiding() == false)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»À» ¸ÕÀú ¼ÒÈ¯ÇØÁÖ¼¼¿ä."));
		else
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»À» Åº »óÅÂ¿¡¼­´Â ¸ÔÀÌ¸¦ ÁÙ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	DWORD dwFood = ch->GetHorseGrade() + 50054 - 1;

	auto foodTable = ITEM_MANAGER::instance().GetTable(dwFood);
	auto foodName = (foodTable) ? foodTable->szLocaleName : "NONAME"; // @fixme307
	if (ch->CountSpecifyItem(dwFood) > 0)
	{
		ch->RemoveSpecifyItem(dwFood, 1);
		ch->FeedHorse();
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»¿¡°Ô %s%s ÁÖ¾ú½À´Ï´Ù."), foodName, "");
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ¾ÆÀÌÅÛÀÌ ÇÊ¿äÇÕ´Ï´Ù"), foodName);
	}
}

#define MAX_REASON_LEN		128

EVENTINFO(TimedEventInfo)
{
	DynamicCharacterPtr ch;
	int		subcmd;
	int		left_second;
	char	szReason[MAX_REASON_LEN];

	TimedEventInfo()
	: ch()
	, subcmd( 0 )
	, left_second( 0 )
	{
		::memset( szReason, 0, MAX_REASON_LEN );
	}
};

struct SendDisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetCharacter())
		{
			#ifdef ENABLE_FLUSH_AT_SHUTDOWN
			d->GetCharacter()->SaveReal();
			DWORD pid = d->GetCharacter()->GetPlayerID();
			db_clientdesc->DBPacketHeader(HEADER_GD_FLUSH_CACHE, 0, sizeof(DWORD));
			db_clientdesc->Packet(&pid, sizeof(DWORD));
			#endif

			if (d->GetCharacter()->GetGMLevel() == GM_PLAYER)
				d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "quit Shutdown(SendDisconnectFunc)");
		}
	}
};

struct DisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetType() == DESC_TYPE_CONNECTOR)
			return;

		if (d->IsPhase(PHASE_P2P))
			return;

		if (d->GetCharacter())
			d->GetCharacter()->Disconnect("Shutdown(DisconnectFunc)");

		d->SetPhase(PHASE_CLOSE);
	}
};

EVENTINFO(shutdown_event_data)
{
	int seconds;

	shutdown_event_data()
	: seconds( 0 )
	{
	}
};

EVENTFUNC(shutdown_event)
{
	shutdown_event_data* info = dynamic_cast<shutdown_event_data*>( event->info );

	if ( info == NULL )
	{
		sys_err( "shutdown_event> <Factor> Null pointer" );
		return 0;
	}

	int * pSec = & (info->seconds);

	if (*pSec < 0)
	{
		sys_log(0, "shutdown_event sec %d", *pSec);

		if (--*pSec == -10)
		{
			const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), DisconnectFunc());
			return passes_per_sec;
		}
		else if (*pSec < -10)
			return 0;

		return passes_per_sec;
	}
	else if (*pSec == 0)
	{
		const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_set_desc.begin(), c_set_desc.end(), SendDisconnectFunc());
		g_bNoMoreClient = true;
		--*pSec;
		return passes_per_sec;
	}
	else
	{
		char buf[64];
		snprintf(buf, sizeof(buf), LC_TEXT("¼Ë´Ù¿îÀÌ %dÃÊ ³²¾Ò½À´Ï´Ù."), *pSec);
		SendNotice(buf);

		--*pSec;
		return passes_per_sec;
	}
}

void Shutdown(int iSec)
{
	if (g_bNoMoreClient)
	{
		thecore_shutdown();
		return;
	}

	CWarMapManager::instance().OnShutdown();

	char buf[64];
	snprintf(buf, sizeof(buf), LC_TEXT("%dÃÊ ÈÄ °ÔÀÓÀÌ ¼Ë´Ù¿î µË´Ï´Ù."), iSec);

	SendNotice(buf);

	shutdown_event_data* info = AllocEventInfo<shutdown_event_data>();
	info->seconds = iSec;

	event_create(shutdown_event, info, 1);
}

ACMD(do_shutdown)
{
	sys_err("Accept shutdown command from %s.", (ch) ? ch->GetName() : "NONAME");

	TPacketGGShutdown p;
	p.bHeader = HEADER_GG_SHUTDOWN;
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShutdown));

	Shutdown(10);
}

EVENTFUNC(timed_event)
{
	TimedEventInfo * info = dynamic_cast<TimedEventInfo *>( event->info );

	if ( info == NULL )
	{
		sys_err( "timed_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}
	LPDESC d = ch->GetDesc();

	if (info->left_second <= 0)
	{
		ch->m_pkTimedEvent = NULL;

		switch (info->subcmd)
		{
			case SCMD_LOGOUT:
			case SCMD_QUIT:
			case SCMD_PHASE_SELECT:
				{
					TPacketNeedLoginLogInfo acc_info;
					acc_info.dwPlayerID = ch->GetDesc()->GetAccountTable().id;

					db_clientdesc->DBPacket( HEADER_GD_VALID_LOGOUT, 0, &acc_info, sizeof(acc_info) );

					LogManager::instance().DetailLoginLog( false, ch );
				}
				break;
		}

		switch (info->subcmd)
		{
			case SCMD_LOGOUT:
				if (d)
					d->SetPhase(PHASE_CLOSE);
				break;

			case SCMD_QUIT:
				ch->ChatPacket(CHAT_TYPE_COMMAND, "quit");
				if (d) // @fixme197
					d->DelayedDisconnect(1);
				break;

			case SCMD_PHASE_SELECT:
				ch->Disconnect("timed_event - SCMD_PHASE_SELECT");
				if (d)
					d->SetPhase(PHASE_SELECT);
				break;
		}

		return 0;
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%dÃÊ ³²¾Ò½À´Ï´Ù."), info->left_second);
		--info->left_second;
	}

	return PASSES_PER_SEC(1);
}

ACMD(do_cmd)
{
	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ãë¼Ò µÇ¾ú½À´Ï´Ù."));
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

	switch (subcmd)
	{
		case SCMD_LOGOUT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("·Î±×ÀÎ È­¸éÀ¸·Î µ¹¾Æ °©´Ï´Ù. Àá½Ã¸¸ ±â´Ù¸®¼¼¿ä."));
			break;

		case SCMD_QUIT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°ÔÀÓÀ» Á¾·á ÇÕ´Ï´Ù. Àá½Ã¸¸ ±â´Ù¸®¼¼¿ä."));
			break;

		case SCMD_PHASE_SELECT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ä³¸¯ÅÍ¸¦ ÀüÈ¯ ÇÕ´Ï´Ù. Àá½Ã¸¸ ±â´Ù¸®¼¼¿ä."));
			break;
	}

	int nExitLimitTime = 10;

	if (ch->IsHack(false, true, nExitLimitTime) &&
		false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()) &&
	   	(!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		return;
	}

	switch (subcmd)
	{
		case SCMD_LOGOUT:
		case SCMD_QUIT:
		case SCMD_PHASE_SELECT:
			{
				TimedEventInfo* info = AllocEventInfo<TimedEventInfo>();

				{
					if (ch->IsPosition(POS_FIGHTING))
						info->left_second = 10;
					else
						info->left_second = 3;
				}

				info->ch		= ch;
				info->subcmd		= subcmd;
				strlcpy(info->szReason, argument, sizeof(info->szReason));

				ch->m_pkTimedEvent	= event_create(timed_event, info, 1);
			}
			break;
	}
}

ACMD(do_mount)
{

}

ACMD(do_fishing)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	ch->SetRotation(atof(arg1));
	ch->fishing();
}

ACMD(do_console)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ConsoleEnable");
}

ACMD(do_restart)
{
	if (false == ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
		ch->StartRecoveryEvent();
		return;
	}

	if (NULL == ch->m_pkDeadEvent)
		return;

	int iTimeToDead = (event_time(ch->m_pkDeadEvent) / passes_per_sec);

	if (subcmd != SCMD_RESTART_TOWN && (!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		if (!test_server)
		{
			if (ch->IsHack())
			{
				if (false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ Àç½ÃÀÛ ÇÒ ¼ö ¾ø½À´Ï´Ù. (%dÃÊ ³²À½)"), iTimeToDead - (180 - g_nPortalLimitTime));
					return;
				}
			}

			#define eFRS_HERESEC	170
			if (iTimeToDead > eFRS_HERESEC)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ Àç½ÃÀÛ ÇÒ ¼ö ¾ø½À´Ï´Ù. (%dÃÊ ³²À½)"), iTimeToDead - eFRS_HERESEC);
				return;
			}
		}
	}

	//PREVENT_HACK
	if (subcmd == SCMD_RESTART_TOWN)
	{
		if (ch->IsHack())
		{

			if ((!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG) ||
			   	false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ Àç½ÃÀÛ ÇÒ ¼ö ¾ø½À´Ï´Ù. (%dÃÊ ³²À½)"), iTimeToDead - (180 - g_nPortalLimitTime));
				return;
			}
		}

		#define eFRS_TOWNSEC	173
		if (iTimeToDead > eFRS_TOWNSEC)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ ¸¶À»¿¡¼­ Àç½ÃÀÛ ÇÒ ¼ö ¾ø½À´Ï´Ù. (%d ÃÊ ³²À½)"), iTimeToDead - eFRS_TOWNSEC);
			return;
		}
	}
	//END_PREVENT_HACK

	ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");

	ch->GetDesc()->SetPhase(PHASE_GAME);
	ch->SetPosition(POS_STANDING);
	ch->StartRecoveryEvent();

	//FORKED_LOAD

	if (1 == quest::CQuestManager::instance().GetEventFlag("threeway_war"))
	{
		if (subcmd == SCMD_RESTART_TOWN || subcmd == SCMD_RESTART_HERE)
		{
			if (true == CThreeWayWar::instance().IsThreeWayWarMapIndex(ch->GetMapIndex()) &&
				false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));

				ch->ReviveInvisible(5);
				ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				ch->ResetAffectFlag();
				return;
			}

			if (true == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				if (CThreeWayWar::instance().GetReviveTokenForPlayer(ch->GetPlayerID()) <= 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ºÁö¿¡¼­ ºÎÈ° ±âÈ¸¸¦ ¸ðµÎ ÀÒ¾ú½À´Ï´Ù! ¸¶À»·Î ÀÌµ¿ÇÕ´Ï´Ù!"));
					ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
				}
				else
				{
					ch->Show(ch->GetMapIndex(), GetSungziStartX(ch->GetEmpire()), GetSungziStartY(ch->GetEmpire()));
				}

				ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				ch->ReviveInvisible(5);
				ch->ResetAffectFlag();
				return;
			}
		}
	}
	//END_FORKED_LOAD

	if (ch->GetDungeon())
		ch->GetDungeon()->UseRevive(ch);

	if (ch->GetWarMap() && !ch->IsObserverMode())
	{
		CWarMap * pMap = ch->GetWarMap();
		DWORD dwGuildOpponent = pMap ? pMap->GetGuildOpponent(ch) : 0;

		if (dwGuildOpponent)
		{
			switch (subcmd)
			{
				case SCMD_RESTART_TOWN:
					sys_log(0, "do_restart: restart town");
					PIXEL_POSITION pos;

					if (CWarMapManager::instance().GetStartPosition(ch->GetMapIndex(), ch->GetGuild()->GetID() < dwGuildOpponent ? 0 : 1, pos))
						ch->Show(ch->GetMapIndex(), pos.x, pos.y);
					else
						ch->ExitToSavedLocation();

					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
					ch->ResetAffectFlag();
					break;

				case SCMD_RESTART_HERE:
					sys_log(0, "do_restart: restart here");
					ch->RestartAtSamePos();
					//ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY());
					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
					ch->ResetAffectFlag();
					break;
			}
			return;
		}
	}

	switch (subcmd)
	{
		case SCMD_RESTART_TOWN:
			sys_log(0, "do_restart: restart town");
			PIXEL_POSITION pos;

			if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
				ch->WarpSet(pos.x, pos.y);
			else
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
			ch->PointChange(POINT_HP, 50 - ch->GetHP());
			ch->DeathPenalty(1);
			ch->ResetAffectFlag();
			break;

		case SCMD_RESTART_HERE:
			sys_log(0, "do_restart: restart here");
			ch->RestartAtSamePos();
			ch->PointChange(POINT_HP, 50 - ch->GetHP());
			ch->DeathPenalty(0);
			ch->ReviveInvisible(5);
			ch->ResetAffectFlag();
			break;
	}
}

#define MAX_STAT g_iStatusPointSetMaxValue

ACMD(do_stat_reset)
{
	ch->PointChange(POINT_STAT_RESET_COUNT, 12 - ch->GetPoint(POINT_STAT_RESET_COUNT));
}

ACMD(do_stat_minus)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Oluyken statu veremezsiniz.!");
		return;
	}

	if (ch->IsHorseRiding())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Binek uzerindeyken bunu yapamazsin!");
		return;
	}

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (ch->GetPoint(POINT_STAT_RESET_COUNT) <= 0)
		return;

	if (!strcmp(arg1, "st"))
	{
		if (ch->GetRealPoint(POINT_ST) <= JobInitialPoints[ch->GetJob()].st)
			return;

		ch->SetRealPoint(POINT_ST, ch->GetRealPoint(POINT_ST) - 1);
		ch->SetPoint(POINT_ST, ch->GetPoint(POINT_ST) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_ST, 0);
	}
	else if (!strcmp(arg1, "dx"))
	{
		if (ch->GetRealPoint(POINT_DX) <= JobInitialPoints[ch->GetJob()].dx)
			return;

		ch->SetRealPoint(POINT_DX, ch->GetRealPoint(POINT_DX) - 1);
		ch->SetPoint(POINT_DX, ch->GetPoint(POINT_DX) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_DX, 0);
	}
	else if (!strcmp(arg1, "ht"))
	{
		if (ch->GetRealPoint(POINT_HT) <= JobInitialPoints[ch->GetJob()].ht)
			return;

		ch->SetRealPoint(POINT_HT, ch->GetRealPoint(POINT_HT) - 1);
		ch->SetPoint(POINT_HT, ch->GetPoint(POINT_HT) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_HT, 0);
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (!strcmp(arg1, "iq"))
	{
		if (ch->GetRealPoint(POINT_IQ) <= JobInitialPoints[ch->GetJob()].iq)
			return;

		ch->SetRealPoint(POINT_IQ, ch->GetRealPoint(POINT_IQ) - 1);
		ch->SetPoint(POINT_IQ, ch->GetPoint(POINT_IQ) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_IQ, 0);
		ch->PointChange(POINT_MAX_SP, 0);
	}
	else
		return;

	ch->PointChange(POINT_STAT, +1);
	ch->PointChange(POINT_STAT_RESET_COUNT, -1);
	ch->ComputePoints();
}

ACMD(do_stat)
{
	char arg1[256];
	#ifdef ENABLE_STATUS_UP_RENEWAL
	char arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	#else
	one_argument(argument, arg1, sizeof(arg1));
	#endif

	if (!*arg1)
		return;

	#ifdef ENABLE_STATUS_UP_RENEWAL
	int iStatUp = 1;
	if (*arg2)
		iStatUp = atoi(arg2);
	#endif

	if (ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Oluyken statu veremezsiniz.!");
		return;
	}

	/*if (ch->IsHorseRiding())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Binek uzerindeyken bunu yapamazsin!");
		return;
	}*/

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	#ifdef ENABLE_STATUS_UP_RENEWAL
	if (ch->GetPoint(POINT_STAT) < iStatUp)
		iStatUp = ch->GetPoint(POINT_STAT);
	#else
	if (ch->GetPoint(POINT_STAT) <= 0)
		return;
	#endif

	BYTE idx = 0;
	if (!strcmp(arg1, "st"))
		idx = POINT_ST;
	else if (!strcmp(arg1, "dx"))
		idx = POINT_DX;
	else if (!strcmp(arg1, "ht"))
		idx = POINT_HT;
	else if (!strcmp(arg1, "iq"))
		idx = POINT_IQ;
	else
		return;

	// ch->ChatPacket(CHAT_TYPE_INFO, "%s GRP(%d) idx(%u), MAX_STAT(%d), expr(%d)", __FUNCTION__, ch->GetRealPoint(idx), idx, MAX_STAT, ch->GetRealPoint(idx) >= MAX_STAT);

	#ifdef ENABLE_STATUS_UP_RENEWAL
	if ((ch->GetRealPoint(idx) + iStatUp) > MAX_STAT)
		iStatUp = MAX_STAT - ch->GetRealPoint(idx);

	if (iStatUp < 1)
	#else
	if (ch->GetRealPoint(idx) >= MAX_STAT)
	#endif
		return;

	#ifdef ENABLE_STATUS_UP_RENEWAL
	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + iStatUp);
	ch->SetPoint(idx, ch->GetPoint(idx) + iStatUp);
	#else
	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + 1);
	ch->SetPoint(idx, ch->GetPoint(idx) + 1);
	#endif

	ch->ComputePoints();
	ch->PointChange(idx, 0);

	if (idx == POINT_IQ)
	{
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (idx == POINT_HT)
	{
		ch->PointChange(POINT_MAX_SP, 0);
	}

	#ifdef ENABLE_STATUS_UP_RENEWAL
	ch->PointChange(POINT_STAT, -iStatUp);
	#else
	ch->PointChange(POINT_STAT, -1);
	#endif
	ch->ComputePoints();
}

ACMD(do_pvp)
{
	if (ch->GetArena() != NULL || CArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD vid = 0;
	str_to_number(vid, arg1);

	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(vid);
	if (!pkVictim)
		return;

	if (pkVictim->IsNPC())
		return;

	if (pkVictim->GetArena() != NULL)
	{
		pkVictim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ó´ë¹æÀÌ ´ë·ÃÁßÀÔ´Ï´Ù."));
		return;
	}

	#ifdef ENABLE_MESSENGER_BLOCK
	if (MessengerManager::instance().IsBlocked_Target(ch->GetName(), pkVictim->GetName()))
		return;

	if (MessengerManager::instance().IsBlocked_Me(ch->GetName(), pkVictim->GetName()))
		return;
	#endif

	CPVPManager::instance().Insert(ch, pkVictim);
}

ACMD(do_guildskillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch->GetGuild())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> ±æµå¿¡ ¼ÓÇØÀÖÁö ¾Ê½À´Ï´Ù."));
		return;
	}

	CGuild* g = ch->GetGuild();
	TGuildMember* gm = g->GetMember(ch->GetPlayerID());
	if (gm->grade == GUILD_LEADER_GRADE)
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		g->SkillLevelUp(vnum);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> ±æµå ½ºÅ³ ·¹º§À» º¯°æÇÒ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
	}
}

ACMD(do_skillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vnum = 0;
	str_to_number(vnum, arg1);

	if (true == ch->CanUseSkill(vnum))
	{
		ch->SkillLevelUp(vnum);
	}
	else
	{
		switch(vnum)
		{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:

			case SKILL_7_A_ANTI_TANHWAN:
			case SKILL_7_B_ANTI_AMSEOP:
			case SKILL_7_C_ANTI_SWAERYUNG:
			case SKILL_7_D_ANTI_YONGBI:

			case SKILL_8_A_ANTI_GIGONGCHAM:
			case SKILL_8_B_ANTI_YEONSA:
			case SKILL_8_C_ANTI_MAHWAN:
			case SKILL_8_D_ANTI_BYEURAK:

			case SKILL_ADD_HP:
			case SKILL_RESIST_PENETRATE:
				ch->SkillLevelUp(vnum);
				break;
		}
	}
}

//
//
ACMD(do_safebox_close)
{
	ch->CloseSafebox();
}

//
//
ACMD(do_safebox_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	ch->ReqSafeboxLoad(arg1);
}

ACMD(do_safebox_change_password)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || strlen(arg1)>6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Ã¢°í> Àß¸øµÈ ¾ÏÈ£¸¦ ÀÔ·ÂÇÏ¼Ì½À´Ï´Ù."));
		return;
	}

	if (!*arg2 || strlen(arg2)>6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Ã¢°í> Àß¸øµÈ ¾ÏÈ£¸¦ ÀÔ·ÂÇÏ¼Ì½À´Ï´Ù."));
		return;
	}

	TSafeboxChangePasswordPacket p;

	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szOldPassword, arg1, sizeof(p.szOldPassword));
	strlcpy(p.szNewPassword, arg2, sizeof(p.szNewPassword));

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_CHANGE_PASSWORD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || strlen(arg1) > 6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Ã¢°í> Àß¸øµÈ ¾ÏÈ£¸¦ ÀÔ·ÂÇÏ¼Ì½À´Ï´Ù."));
		return;
	}

	int iPulse = thecore_pulse();

	if (ch->GetMall())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Ã¢°í> Ã¢°í°¡ ÀÌ¹Ì ¿­·ÁÀÖ½À´Ï´Ù."));
		return;
	}

	if (iPulse - ch->GetMallLoadTime() < passes_per_sec * 10)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Ã¢°í> Ã¢°í¸¦ ´ÝÀºÁö 10ÃÊ ¾È¿¡´Â ¿­ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	ch->SetMallLoadTime(iPulse);

	TSafeboxLoadPacket p;
	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szLogin, ch->GetDesc()->GetAccountTable().login, sizeof(p.szLogin));
	strlcpy(p.szPassword, arg1, sizeof(p.szPassword));

	db_clientdesc->DBPacket(HEADER_GD_MALL_LOAD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_close)
{
	if (ch->GetMall())
	{
		ch->SetMallLoadTime(thecore_pulse());
		ch->CloseMall();
		ch->Save();
	}
}

ACMD(do_ungroup)
{
	if (!ch->GetParty())
		return;

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<ÆÄÆ¼> ¼­¹ö ¹®Á¦·Î ÆÄÆ¼ °ü·Ã Ã³¸®¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<ÆÄÆ¼> ´øÀü ¾È¿¡¼­´Â ÆÄÆ¼¿¡¼­ ³ª°¥ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	LPPARTY pParty = ch->GetParty();

	if (pParty->GetMemberCount() == 2)
	{
		// party disband
		CPartyManager::instance().DeleteParty(pParty);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<ÆÄÆ¼> ÆÄÆ¼¿¡¼­ ³ª°¡¼Ì½À´Ï´Ù."));
		//pParty->SendPartyRemoveOneToAll(ch);
		pParty->Quit(ch->GetPlayerID());
		//pParty->SendPartyRemoveAllToOne(ch);
	}
}

ACMD(do_close_shop)
{
	if (ch->GetMyShop())
	{
		ch->CloseMyShop();
		return;
	}
}

ACMD(do_set_walk_mode)
{
	ch->SetNowWalking(true);
	ch->SetWalking(true);
}

ACMD(do_set_run_mode)
{
	ch->SetNowWalking(false);
	ch->SetWalking(false);
}

ACMD(do_war)
{
	#ifdef ENABLE_GUILD_TOURNAMENT_MODE
	if (quest::CQuestManager::instance().GetEventFlag("guild_tournament_status") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild Tournament> Lonca turnuvasi aktifken bunu yapamazsiniz!"));
		return;
	}
	#endif

	CGuild * g = ch->GetGuild();
	if (!g)
		return;

	if (g->UnderAnyWar())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> ÀÌ¹Ì ´Ù¸¥ ÀüÀï¿¡ ÂüÀü Áß ÀÔ´Ï´Ù."));
		return;
	}

	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	char arg1[256], arg2[256], arg3[256], arg4[256], arg5[256], arg6[256], arg7[256], arg8[256], arg9[256];
	nine_arguments(argument
		, arg1, sizeof(arg1)
		, arg2, sizeof(arg2)
		, arg3, sizeof(arg3)
		, arg4, sizeof(arg4)
		, arg5, sizeof(arg5)
		, arg6, sizeof(arg6)
		, arg7, sizeof(arg7)
		, arg8, sizeof(arg8)
		, arg9, sizeof(arg9)
	);
	#else
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	#endif

	if (!*arg1)
		return;

	DWORD gm_pid = g->GetMasterPID();
	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> ±æµåÀü¿¡ ´ëÇÑ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
		return;
	}

	CGuild * opp_g = CGuildManager::instance().FindGuildByName(arg1);
	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> ±×·± ±æµå°¡ ¾ø½À´Ï´Ù."));
		return;
	}
	DWORD type = GUILD_WAR_TYPE_FIELD; //fixme102 base int modded uint
	if (*arg2)
	{
		str_to_number(type, arg2);

		if ((type < GUILD_WAR_TYPE_FIELD) || (type >= GUILD_WAR_TYPE_MAX_NUM))
			type = GUILD_WAR_TYPE_FIELD;
	}

	switch (g->GetGuildWarState(opp_g->GetID()))
	{
		case GUILD_WAR_NONE:
			{
				if (opp_g->UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> »ó´ë¹æ ±æµå°¡ ÀÌ¹Ì ÀüÀï Áß ÀÔ´Ï´Ù."));
					return;
				}

				int iWarPrice = KOR_aGuildWarInfo[type].iWarPrice;

				if (g->GetGuildMoney() < iWarPrice)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> Àüºñ°¡ ºÎÁ·ÇÏ¿© ±æµåÀüÀ» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return;
				}

				if (opp_g->GetGuildMoney() < iWarPrice)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> »ó´ë¹æ ±æµåÀÇ Àüºñ°¡ ºÎÁ·ÇÏ¿© ±æµåÀüÀ» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return;
				}
			}
			break;

		case GUILD_WAR_SEND_DECLARE:
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì ¼±ÀüÆ÷°í ÁßÀÎ ±æµåÀÔ´Ï´Ù."));
				return;
			}
			break;

		case GUILD_WAR_RECV_DECLARE:
			{
				if (opp_g->UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> »ó´ë¹æ ±æµå°¡ ÀÌ¹Ì ÀüÀï Áß ÀÔ´Ï´Ù."));
					g->RequestRefuseWar(opp_g->GetID());
					return;
				}
			}
			break;

		case GUILD_WAR_RESERVE:
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> ÀÌ¹Ì ÀüÀïÀÌ ¿¹¾àµÈ ±æµå ÀÔ´Ï´Ù."));
				return;
			}
			break;

		case GUILD_WAR_END:
			return;

		default:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> ÀÌ¹Ì ÀüÀï ÁßÀÎ ±æµåÀÔ´Ï´Ù."));
			g->RequestRefuseWar(opp_g->GetID());
			return;
	}

	if (!g->CanStartWar(type))
	{
		if (g->GetLadderPoint() == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> ·¹´õ Á¡¼ö°¡ ¸ðÀÚ¶ó¼­ ±æµåÀüÀ» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			sys_log(0, "GuildWar.StartError.NEED_LADDER_POINT");
		}
		else if (g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> ±æµåÀüÀ» ÇÏ±â À§ÇØ¼± ÃÖ¼ÒÇÑ %d¸íÀÌ ÀÖ¾î¾ß ÇÕ´Ï´Ù."), GUILD_WAR_MIN_MEMBER_COUNT);
			sys_log(0, "GuildWar.StartError.NEED_MINIMUM_MEMBER[%d]", GUILD_WAR_MIN_MEMBER_COUNT);
		}
		else
		{
			sys_log(0, "GuildWar.StartError.UNKNOWN_ERROR");
		}
		return;
	}

	if (!opp_g->CanStartWar(GUILD_WAR_TYPE_FIELD))
	{
		if (opp_g->GetLadderPoint() == 0)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> »ó´ë¹æ ±æµåÀÇ ·¹´õ Á¡¼ö°¡ ¸ðÀÚ¶ó¼­ ±æµåÀüÀ» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		else if (opp_g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> »ó´ë¹æ ±æµåÀÇ ±æµå¿ø ¼ö°¡ ºÎÁ·ÇÏ¿© ±æµåÀüÀ» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	do
	{
		if (g->GetMasterCharacter() != NULL)
			break;

		CCI *pCCI = P2P_MANAGER::instance().FindByPID(g->GetMasterPID());
		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> »ó´ë¹æ ±æµåÀÇ ±æµåÀåÀÌ Á¢¼ÓÁßÀÌ ¾Æ´Õ´Ï´Ù."));
		g->RequestRefuseWar(opp_g->GetID());
		return;

	} while (false);

	do
	{
		if (opp_g->GetMasterCharacter() != NULL)
			break;

		CCI *pCCI = P2P_MANAGER::instance().FindByPID(opp_g->GetMasterPID());
		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> »ó´ë¹æ ±æµåÀÇ ±æµåÀåÀÌ Á¢¼ÓÁßÀÌ ¾Æ´Õ´Ï´Ù."));
		g->RequestRefuseWar(opp_g->GetID());
		return;

	} while (false);

	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	if (*arg3 && *arg4 && *arg5 && *arg6 && *arg7 && *arg8 && *arg9)
	{
		int iScore = 100;
		int iMinLevel = 1;
		int iMaxPlayer = 20;

		str_to_number(iScore, arg3);
		str_to_number(iMinLevel, arg4);
		str_to_number(iMaxPlayer, arg5);

		bool bWarrior = (LOWER(*arg6) == 'y') ? true : false;
		bool bAssassin = (LOWER(*arg7) == 'y') ? true : false;
		bool bSura = (LOWER(*arg8) == 'y') ? true : false;
		bool bShaman = (LOWER(*arg9) == 'y') ? true : false;

		// <<<
		if (iScore < 10) { iScore = 10; }
		if (iMinLevel < 1) { iMinLevel = 1; }
		if (iMaxPlayer < 1) { iMaxPlayer = 1; }
		// <<<
		// >>>
		if (iScore > 500) { iScore = 500; }
		if (iMinLevel > gPlayerMaxLevel) { iMinLevel = gPlayerMaxLevel; }
		if (iMaxPlayer > 250) { iMaxPlayer = 250; }
		// >>>

		g->SetWarScore(iScore);
		g->SetWarMinLevel(iMinLevel);
		g->SetWarMaxPlayer(iMaxPlayer);
		g->SetWarWarriorStatus(bWarrior);
		g->SetWarAssassinStatus(bAssassin);
		g->SetWarSuraStatus(bSura);
		g->SetWarShamanStatus(bShaman);
		opp_g->SetWarScore(iScore);
		opp_g->SetWarMinLevel(iMinLevel);
		opp_g->SetWarMaxPlayer(iMaxPlayer);
		opp_g->SetWarWarriorStatus(bWarrior);
		opp_g->SetWarAssassinStatus(bAssassin);
		opp_g->SetWarSuraStatus(bSura);
		opp_g->SetWarShamanStatus(bShaman);

		TPacketGGGuildWarOption pgg;
		pgg.bHeader = HEADER_GG_GUILD_WAR_OPTION;
		pgg.dwGuild1 = g->GetID();
		pgg.dwGuild2 = opp_g->GetID();
		pgg.iScore = iScore;
		pgg.iMinLevel = iMinLevel;
		pgg.iMaxPlayer = iMaxPlayer;
		pgg.bWarrior = bWarrior;
		pgg.bAssassin = bAssassin;
		pgg.bSura = bSura;
		pgg.bShaman = bShaman;
		P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGGuildWarOption));
	}
	#endif

	g->RequestDeclareWar(opp_g->GetID(), type);
}

ACMD(do_nowar)
{
	#ifdef ENABLE_GUILD_TOURNAMENT_MODE
	if (quest::CQuestManager::instance().GetEventFlag("guild_tournament_status") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild Tournament> Lonca turnuvasi aktifken bunu yapamazsiniz!"));
		return;
	}
	#endif

	CGuild* g = ch->GetGuild();
	if (!g)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD gm_pid = g->GetMasterPID();
	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> ±æµåÀü¿¡ ´ëÇÑ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
		return;
	}

	CGuild* opp_g = CGuildManager::instance().FindGuildByName(arg1);
	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<±æµå> ±×·± ±æµå°¡ ¾ø½À´Ï´Ù."));
		return;
	}

	g->RequestRefuseWar(opp_g->GetID());
}

#ifdef ENABLE_GUILD_TOURNAMENT_MODE
ACMD(do_war_tournament)
{
	if (quest::CQuestManager::instance().GetEventFlag("guild_tournament_status") == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild Tournament> Oncelikle lonca turnuva modunu aktiflestirmelisiniz!"));
		return;
	}

	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);

	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	if (vecArgs.size() < 10) { return; }
	#else
	if (vecArgs.size() < 3) { return; }
	#endif

	CGuild * opp_o = CGuildManager::instance().FindGuildByName(vecArgs[1]);
	if (!opp_o)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild Tournament> 1.Lonca mevcut degil!"));
		return;
	}

	CGuild * opp_g = CGuildManager::instance().FindGuildByName(vecArgs[2]);
	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild Tournament> 2.Lonca mevcut degil!"));
		return;
	}

	if (opp_o->UnderAnyWar())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild Tournament> 1.Lonca suan savasta!"));
		return;
	}

	if (opp_g->UnderAnyWar())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild Tournament> 2.Lonca suan savasta!"));
		return;
	}

	DWORD type = GUILD_WAR_TYPE_FIELD; //fixme102 base int modded uint
	str_to_number(type, vecArgs[3].c_str());

	if ((type < GUILD_WAR_TYPE_FIELD) || (type >= GUILD_WAR_TYPE_MAX_NUM))
		type = GUILD_WAR_TYPE_FIELD;

	do
	{
		if (opp_o->GetMasterCharacter() != NULL)
			break;

		CCI *pCCI = P2P_MANAGER::instance().FindByPID(opp_o->GetMasterPID());
		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild Tournament> 1.Lonca lideri cevrimdisi!"));
		return;

	} while (false);

	do
	{
		if (opp_g->GetMasterCharacter() != NULL)
			break;

		CCI *pCCI = P2P_MANAGER::instance().FindByPID(opp_g->GetMasterPID());
		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild Tournament> 2.Lonca lideri cevrimdisi!"));
		return;

	} while (false);

	#ifdef ENABLE_GUILD_WAR_OPTION_SYSTEM
	{
		int iScore = 100;
		int iMinLevel = 1;
		int iMaxPlayer = 20;

		str_to_number(iScore, vecArgs[4].c_str());
		str_to_number(iMinLevel, vecArgs[5].c_str());
		str_to_number(iMaxPlayer, vecArgs[6].c_str());

		bool bWarrior = (LOWER(*vecArgs[7].c_str()) == 'y') ? true : false;
		bool bAssassin = (LOWER(*vecArgs[8].c_str()) == 'y') ? true : false;
		bool bSura = (LOWER(*vecArgs[9].c_str()) == 'y') ? true : false;
		bool bShaman = (LOWER(*vecArgs[10].c_str()) == 'y') ? true : false;

		// <<<
		if (iScore < 10) { iScore = 10; }
		if (iMinLevel < 1) { iMinLevel = 1; }
		if (iMaxPlayer < 1) { iMaxPlayer = 1; }
		// <<<
		// >>>
		if (iScore > 500) { iScore = 500; }
		if (iMinLevel > gPlayerMaxLevel) { iMinLevel = gPlayerMaxLevel; }
		if (iMaxPlayer > 250) { iMaxPlayer = 250; }
		// >>>

		opp_o->SetWarScore(iScore);
		opp_o->SetWarMinLevel(iMinLevel);
		opp_o->SetWarMaxPlayer(iMaxPlayer);
		opp_o->SetWarWarriorStatus(bWarrior);
		opp_o->SetWarAssassinStatus(bAssassin);
		opp_o->SetWarSuraStatus(bSura);
		opp_o->SetWarShamanStatus(bShaman);
		opp_g->SetWarScore(iScore);
		opp_g->SetWarMinLevel(iMinLevel);
		opp_g->SetWarMaxPlayer(iMaxPlayer);
		opp_g->SetWarWarriorStatus(bWarrior);
		opp_g->SetWarAssassinStatus(bAssassin);
		opp_g->SetWarSuraStatus(bSura);
		opp_g->SetWarShamanStatus(bShaman);

		TPacketGGGuildWarOption pgg;
		pgg.bHeader = HEADER_GG_GUILD_WAR_OPTION;
		pgg.dwGuild1 = opp_o->GetID();
		pgg.dwGuild2 = opp_g->GetID();
		pgg.iScore = iScore;
		pgg.iMinLevel = iMinLevel;
		pgg.iMaxPlayer = iMaxPlayer;
		pgg.bWarrior = bWarrior;
		pgg.bAssassin = bAssassin;
		pgg.bSura = bSura;
		pgg.bShaman = bShaman;
		P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGGuildWarOption));
	}
	#endif

	opp_o->RequestDeclareWar(opp_g->GetID(), type);
	opp_g->RequestDeclareWar(opp_o->GetID(), type);
}

ACMD(do_guild_tournament)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	const std::string& strArg1 = std::string(arg1);
	if (strArg1 == "start")
	{
		quest::CQuestManager::instance().RequestSetEventFlag("guild_tournament_status", 1);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild Tournament> Lonca turnuva modu aktif edildi!"));
	}
	else if (strArg1 == "stop")
	{
		quest::CQuestManager::instance().RequestSetEventFlag("guild_tournament_status", 0);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild Tournament> Lonca turnuva modu deaktif edildi!"));
	}
}
#endif

ACMD(do_detaillog)
{
	ch->DetailLog();
}

ACMD(do_monsterlog)
{
	ch->ToggleMonsterLog();
}

ACMD(do_pkmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	BYTE mode = 0;
	str_to_number(mode, arg1);

	#ifdef ENABLE_VSTURNUVA
	if (ch->GetMapIndex() == 28)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Bu haritada bunu yapamazsin!");
		return;
	}
	#endif

	#ifdef ENABLE_CHECK_SECURE_ZONE
	if (gef_check_secure_zone)
	{
		switch (ch->GetMapIndex())
		{
			/*
			case 67://Hayalet Orman
			case 68://Kýzýl Orman
			case 123://ValentineMap2020
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu haritada bunu yapamazsin!"));
				return;

			case 65:
			{
				int iDist = gef_devil_tower_dist;
				if (iDist <= 0) { iDist = 500; }
				int iSecureDist = DISTANCE_APPROX(ch->GetX() - 589700, ch->GetY() - 113300);
				if (iSecureDist < iDist)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu bolgede bunu yapamazsin!"));
					return;
				}
			}*/

			case 73:
			{
				int iDist = gef_blue_dragon_dist;
				if (iDist <= 0) { iDist = 500; }
				int iSecureDist = DISTANCE_APPROX(ch->GetX() - 180185, ch->GetY() - 1220693);
				if (iSecureDist < iDist)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bu bolgede bunu yapamazsin!"));
					return;
				}
			}
		}
	}
	#endif

	if (mode == PK_MODE_PROTECT)
		return;

	if (ch->GetLevel() < PK_PROTECT_LEVEL && mode != 0)
		return;

	ch->SetPKMode(mode);
}

ACMD(do_messenger_auth)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	char answer = LOWER(*arg1);
	// @fixme130 AuthToAdd void -> bool
	bool bIsDenied = answer != 'y';
	bool bIsAdded = MessengerManager::instance().AuthToAdd(ch->GetName(), arg2, bIsDenied); // DENY
	if (bIsAdded && bIsDenied)
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg2);
		if (tch)
			tch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ´ÔÀ¸·Î ºÎÅÍ Ä£±¸ µî·ÏÀ» °ÅºÎ ´çÇß½À´Ï´Ù."), ch->GetName());
	}

}

ACMD(do_setblockmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		BYTE flag = 0;
		str_to_number(flag, arg1);
		ch->SetBlockMode(flag);
	}
}

ACMD(do_unmount)
{
	if (true == ch->UnEquipSpecialRideUniqueItem())
	{
		ch->RemoveAffect(AFFECT_MOUNT);
		ch->RemoveAffect(AFFECT_MOUNT_BONUS);

		if (ch->IsHorseRiding())
		{
			ch->StopRiding();
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÎº¥Åä¸®°¡ ²Ë Â÷¼­ ³»¸± ¼ö ¾ø½À´Ï´Ù."));
	}
}

ACMD(do_observer_exit)
{
	if (ch->IsObserverMode())
	{
		if (ch->GetWarMap())
			ch->SetWarMap(NULL);

		if (ch->GetArena() != NULL || ch->GetArenaObserverMode() == true)
		{
			ch->SetArenaObserverMode(false);

			if (ch->GetArena() != NULL)
				ch->GetArena()->RemoveObserver(ch->GetPlayerID());

			ch->SetArena(NULL);
			ch->WarpSet(ARENA_RETURN_POINT_X(ch->GetEmpire()), ARENA_RETURN_POINT_Y(ch->GetEmpire()));
		}
		else
		{
			ch->ExitToSavedLocation();
		}
		ch->SetObserverMode(false);
	}
}

ACMD(do_view_equip)
{
	if (ch->GetGMLevel() <= GM_PLAYER)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD vid = 0;
		str_to_number(vid, arg1);
		LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

		if (!tch)
			return;

		if (!tch->IsPC())
			return;

		tch->SendEquipment(ch);
	}
}

ACMD(do_party_request)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (ch->GetParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì ÆÄÆ¼¿¡ ¼ÓÇØ ÀÖÀ¸¹Ç·Î °¡ÀÔ½ÅÃ»À» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		if (!ch->RequestToParty(tch))
			ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

ACMD(do_party_request_accept)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->AcceptToParty(tch);
}

ACMD(do_party_request_deny)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->DenyToParty(tch);
}

ACMD(do_monarch_warpto)
{
	if (!CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±ºÁÖ¸¸ÀÌ »ç¿ë °¡´ÉÇÑ ±â´ÉÀÔ´Ï´Ù"));
		return;
	}


	if (!ch->IsMCOK(CHARACTER::MI_WARP))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d ÃÊ°£ ÄðÅ¸ÀÓÀÌ Àû¿ëÁßÀÔ´Ï´Ù."), ch->GetMCLTime(CHARACTER::MI_WARP));
		return;
	}


	const int WarpPrice = 10000;


	if (!CMonarch::instance().IsMoneyOk(WarpPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±¹°í¿¡ µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù. ÇöÀç : %u ÇÊ¿ä±Ý¾× : %u"), NationMoney, WarpPrice);
		return;
	}

	int x = 0, y = 0;
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç¿ë¹ý: warpto <character name>"));
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);
		if (pkCCI)
		{
			if (pkCCI->bEmpire != ch->GetEmpire())
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("Å¸Á¦±¹ À¯Àú¿¡°Ô´Â ÀÌµ¿ÇÒ¼ö ¾ø½À´Ï´Ù"));
				return;
			}

			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÇØ´ç À¯Àú´Â %d Ã¤³Î¿¡ ÀÖ½À´Ï´Ù. (ÇöÀç Ã¤³Î %d)"), pkCCI->bChannel, g_bChannel);
				return;
			}
			if (!IsMonarchWarpZone(pkCCI->lMapIndex))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("ÇØ´ç Áö¿ªÀ¸·Î ÀÌµ¿ÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return;
			}

			PIXEL_POSITION pos;

			if (!SECTREE_MANAGER::instance().GetCenterPositionOfMap(pkCCI->lMapIndex, pos))
				ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map (index %d)", pkCCI->lMapIndex);
			else
			{
				//ch->ChatPacket(CHAT_TYPE_INFO, "You warp to (%d, %d)", pos.x, pos.y);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ¿¡°Ô·Î ÀÌµ¿ÇÕ´Ï´Ù"), arg1);
				ch->WarpSet(pos.x, pos.y);


				CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);


				ch->SetMC(CHARACTER::MI_WARP);
			}
		}
		else if (NULL == CHARACTER_MANAGER::instance().FindPC(arg1))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no one by that name");
		}

		return;
	}
	else
	{
		if (tch->GetEmpire() != ch->GetEmpire())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Å¸Á¦±¹ À¯Àú¿¡°Ô´Â ÀÌµ¿ÇÒ¼ö ¾ø½À´Ï´Ù"));
			return;
		}
		if (!IsMonarchWarpZone(tch->GetMapIndex()))
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("ÇØ´ç Áö¿ªÀ¸·Î ÀÌµ¿ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;
		}
		x = tch->GetX();
		y = tch->GetY();
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ¿¡°Ô·Î ÀÌµ¿ÇÕ´Ï´Ù"), arg1);
	ch->WarpSet(x, y);
	ch->Stop();

	CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);
	ch->SetMC(CHARACTER::MI_WARP);
}

ACMD(do_monarch_transfer)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç¿ë¹ý: transfer <name>"));
		return;
	}

	if (!CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±ºÁÖ¸¸ÀÌ »ç¿ë °¡´ÉÇÑ ±â´ÉÀÔ´Ï´Ù"));
		return;
	}

	if (!ch->IsMCOK(CHARACTER::MI_TRANSFER))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d ÃÊ°£ ÄðÅ¸ÀÓÀÌ Àû¿ëÁßÀÔ´Ï´Ù."), ch->GetMCLTime(CHARACTER::MI_TRANSFER));
		return;
	}

	const int WarpPrice = 10000;

	if (!CMonarch::instance().IsMoneyOk(WarpPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±¹°í¿¡ µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù. ÇöÀç : %u ÇÊ¿ä±Ý¾× : %u"), NationMoney, WarpPrice);
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);
		if (pkCCI)
		{
			if (pkCCI->bEmpire != ch->GetEmpire())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´Ù¸¥ Á¦±¹ À¯Àú´Â ¼ÒÈ¯ÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return;
			}

			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ´ÔÀº %d Ã¤³Î¿¡ Á¢¼Ó Áß ÀÔ´Ï´Ù. (ÇöÀç Ã¤³Î: %d)"), arg1, pkCCI->bChannel, g_bChannel);
				return;
			}

			if (!IsMonarchWarpZone(pkCCI->lMapIndex))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("ÇØ´ç Áö¿ªÀ¸·Î ÀÌµ¿ÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return;
			}

			if (!IsMonarchWarpZone(ch->GetMapIndex()))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("ÇØ´ç Áö¿ªÀ¸·Î ¼ÒÈ¯ÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return;
			}

			TPacketGGTransfer pgg;
			strlcpy(pgg.szName, arg1, sizeof(pgg.szName));
			pgg.lX = ch->GetX();
			pgg.lY = ch->GetY();
			P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGTransfer));

			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ´ÔÀ» ¼ÒÈ¯ÇÏ¿´½À´Ï´Ù."), arg1);
			CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);
			ch->SetMC(CHARACTER::MI_TRANSFER);
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÔ·ÂÇÏ½Å ÀÌ¸§À» °¡Áø »ç¿ëÀÚ°¡ ¾ø½À´Ï´Ù."));
		}

		return;
	}

	if (ch == tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÚ½ÅÀ» ¼ÒÈ¯ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (tch->GetEmpire() != ch->GetEmpire())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´Ù¸¥ Á¦±¹ À¯Àú´Â ¼ÒÈ¯ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}
	if (!IsMonarchWarpZone(tch->GetMapIndex()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("ÇØ´ç Áö¿ªÀ¸·Î ÀÌµ¿ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}
	if (!IsMonarchWarpZone(ch->GetMapIndex()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("ÇØ´ç Áö¿ªÀ¸·Î ¼ÒÈ¯ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	//tch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
	tch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());

	CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

	ch->SetMC(CHARACTER::MI_TRANSFER);
}

ACMD(do_monarch_info)
{
	if (CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("³ªÀÇ ±ºÁÖ Á¤º¸"));
		TMonarchInfo * p = CMonarch::instance().GetMonarch();
		for (int n = 1; n < 4; ++n)
		{
			if (n == ch->GetEmpire())
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[%s±ºÁÖ] : %s  º¸À¯±Ý¾× %lld "), EMPIRE_NAME(n), p->name[n], p->money[n]);
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[%s±ºÁÖ] : %s  "), EMPIRE_NAME(n), p->name[n]);

		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±ºÁÖ Á¤º¸"));
		TMonarchInfo * p = CMonarch::instance().GetMonarch();
		for (int n = 1; n < 4; ++n)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[%s±ºÁÖ] : %s  "), EMPIRE_NAME(n), p->name[n]);

		}
	}

}

ACMD(do_elect)
{
	db_clientdesc->DBPacketHeader(HEADER_GD_COME_TO_VOTE, ch->GetDesc()->GetHandle(), 0);
}

// LUA_ADD_GOTO_INFO
struct GotoInfo
{
	std::string 	st_name;

	BYTE 	empire;
	int 	mapIndex;
	DWORD 	x, y;

	GotoInfo()
	{
		st_name 	= "";
		empire 		= 0;
		mapIndex 	= 0;

		x = 0;
		y = 0;
	}

	GotoInfo(const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void operator = (const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void __copy__(const GotoInfo& c_src)
	{
		st_name 	= c_src.st_name;
		empire 		= c_src.empire;
		mapIndex 	= c_src.mapIndex;

		x = c_src.x;
		y = c_src.y;
	}
};

ACMD(do_monarch_tax)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: monarch_tax <1-50>");
		return;
	}


	if (!ch->IsMonarch())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±ºÁÖ¸¸ÀÌ »ç¿ëÇÒ¼ö ÀÖ´Â ±â´ÉÀÔ´Ï´Ù"));
		return;
	}


	int tax = 0;
	str_to_number(tax,  arg1);

	if (tax < 1 || tax > 50)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1-50 »çÀÌÀÇ ¼öÄ¡¸¦ ¼±ÅÃÇØÁÖ¼¼¿ä"));

	quest::CQuestManager::instance().SetEventFlag("trade_tax", tax);


	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼¼±ÝÀÌ %d %·Î ¼³Á¤µÇ¾ú½À´Ï´Ù"));


	char szMsg[1024];

	snprintf(szMsg, sizeof(szMsg), "±ºÁÖÀÇ ¸íÀ¸·Î ¼¼±ÝÀÌ %d %% ·Î º¯°æµÇ¾ú½À´Ï´Ù", tax);
	BroadcastNotice(szMsg);

	snprintf(szMsg, sizeof(szMsg), "¾ÕÀ¸·Î´Â °Å·¡ ±Ý¾×ÀÇ %d %% °¡ ±¹°í·Î µé¾î°¡°ÔµË´Ï´Ù.", tax);
	BroadcastNotice(szMsg);


	ch->SetMC(CHARACTER::MI_TAX);
}

static const DWORD cs_dwMonarchMobVnums[] =
{
	191,
	192,
	193,
	194,
	391,
	392,
	393,
	394,
	491,
	492,
	493,
	494,
	591,
	691,
	791,
	1304,
	1901,
	2091,
	2191,
	2206,
	0,
};

ACMD(do_monarch_mob)
{
	char arg1[256];
	LPCHARACTER	tch;

	one_argument(argument, arg1, sizeof(arg1));

	if (!ch->IsMonarch())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±ºÁÖ¸¸ÀÌ »ç¿ëÇÒ¼ö ÀÖ´Â ±â´ÉÀÔ´Ï´Ù"));
		return;
	}

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mmob <mob name>");
		return;
	}

#ifdef ENABLE_MONARCH_MOB_CMD_MAP_CHECK // @warme006
	BYTE pcEmpire = ch->GetEmpire();
	BYTE mapEmpire = SECTREE_MANAGER::instance().GetEmpireFromMapIndex(ch->GetMapIndex());
	if (mapEmpire != pcEmpire && mapEmpire != 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÚ±¹ ¿µÅä¿¡¼­¸¸ »ç¿ëÇÒ ¼ö ÀÖ´Â ±â´ÉÀÔ´Ï´Ù"));
		return;
	}
#endif

	const int SummonPrice = 5000000;

	if (!ch->IsMCOK(CHARACTER::MI_SUMMON))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d ÃÊ°£ ÄðÅ¸ÀÓÀÌ Àû¿ëÁßÀÔ´Ï´Ù."), ch->GetMCLTime(CHARACTER::MI_SUMMON));
		return;
	}

	if (!CMonarch::instance().IsMoneyOk(SummonPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±¹°í¿¡ µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù. ÇöÀç : %u ÇÊ¿ä±Ý¾× : %u"), NationMoney, SummonPrice);
		return;
	}

	const CMob * pkMob;
	DWORD vnum = 0;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	DWORD count;


	for (count = 0; cs_dwMonarchMobVnums[count] != 0; ++count)
		if (cs_dwMonarchMobVnums[count] == vnum)
			break;

	if (0 == cs_dwMonarchMobVnums[count])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÈ¯ÇÒ¼ö ¾ø´Â ¸ó½ºÅÍ ÀÔ´Ï´Ù. ¼ÒÈ¯°¡´ÉÇÑ ¸ó½ºÅÍ´Â È¨ÆäÀÌÁö¸¦ ÂüÁ¶ÇÏ¼¼¿ä"));
		return;
	}

	tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum,
			ch->GetMapIndex(),
			ch->GetX() - number(200, 750),
			ch->GetY() - number(200, 750),
			ch->GetX() + number(200, 750),
			ch->GetY() + number(200, 750),
			true,
			pkMob->m_table.bType == CHAR_TYPE_STONE,
			true);

	if (tch)
	{

		CMonarch::instance().SendtoDBDecMoney(SummonPrice, ch->GetEmpire(), ch);


		ch->SetMC(CHARACTER::MI_SUMMON);
	}
}

static const char* FN_point_string(int apply_number)
{
	switch (apply_number)
	{
		case POINT_MAX_HP:	return LC_TEXT("ÃÖ´ë »ý¸í·Â +%d");
		case POINT_MAX_SP:	return LC_TEXT("ÃÖ´ë Á¤½Å·Â +%d");
		case POINT_HT:		return LC_TEXT("Ã¼·Â +%d");
		case POINT_IQ:		return LC_TEXT("Áö´É +%d");
		case POINT_ST:		return LC_TEXT("±Ù·Â +%d");
		case POINT_DX:		return LC_TEXT("¹ÎÃ¸ +%d");
		case POINT_ATT_SPEED:	return LC_TEXT("°ø°Ý¼Óµµ +%d");
		case POINT_MOV_SPEED:	return LC_TEXT("ÀÌµ¿¼Óµµ %d");
		case POINT_CASTING_SPEED:	return LC_TEXT("ÄðÅ¸ÀÓ -%d");
		case POINT_HP_REGEN:	return LC_TEXT("»ý¸í·Â È¸º¹ +%d");
		case POINT_SP_REGEN:	return LC_TEXT("Á¤½Å·Â È¸º¹ +%d");
		case POINT_POISON_PCT:	return LC_TEXT("µ¶°ø°Ý %d");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_BLEEDING_PCT:	return LC_TEXT("µ¶°ø°Ý %d");
#endif
		case POINT_STUN_PCT:	return LC_TEXT("½ºÅÏ +%d");
		case POINT_SLOW_PCT:	return LC_TEXT("½½·Î¿ì +%d");
		case POINT_CRITICAL_PCT:	return LC_TEXT("%d%% È®·ü·Î Ä¡¸íÅ¸ °ø°Ý");
		case POINT_RESIST_CRITICAL:	return LC_TEXT("»ó´ëÀÇ Ä¡¸íÅ¸ È®·ü %d%% °¨¼Ò");
		case POINT_PENETRATE_PCT:	return LC_TEXT("%d%% È®·ü·Î °üÅë °ø°Ý");
		case POINT_RESIST_PENETRATE: return LC_TEXT("»ó´ëÀÇ °üÅë °ø°Ý È®·ü %d%% °¨¼Ò");
		case POINT_ATTBONUS_HUMAN:	return LC_TEXT("ÀÎ°£·ù ¸ó½ºÅÍ Å¸°ÝÄ¡ +%d%%");
		case POINT_ATTBONUS_ANIMAL:	return LC_TEXT("µ¿¹°·ù ¸ó½ºÅÍ Å¸°ÝÄ¡ +%d%%");
		case POINT_ATTBONUS_ORC:	return LC_TEXT("¿õ±ÍÁ· Å¸°ÝÄ¡ +%d%%");
		case POINT_ATTBONUS_MILGYO:	return LC_TEXT("¹Ð±³·ù Å¸°ÝÄ¡ +%d%%");
		case POINT_ATTBONUS_UNDEAD:	return LC_TEXT("½ÃÃ¼·ù Å¸°ÝÄ¡ +%d%%");
		case POINT_ATTBONUS_DEVIL:	return LC_TEXT("¾Ç¸¶·ù Å¸°ÝÄ¡ +%d%%");
		case POINT_STEAL_HP:		return LC_TEXT("Å¸°ÝÄ¡ %d%% ¸¦ »ý¸í·ÂÀ¸·Î Èí¼ö");
		case POINT_STEAL_SP:		return LC_TEXT("Å¸·ÂÄ¡ %d%% ¸¦ Á¤½Å·ÂÀ¸·Î Èí¼ö");
		case POINT_MANA_BURN_PCT:	return LC_TEXT("%d%% È®·ü·Î Å¸°Ý½Ã »ó´ë Àü½Å·Â ¼Ò¸ð");
		case POINT_DAMAGE_SP_RECOVER:	return LC_TEXT("%d%% È®·ü·Î ÇÇÇØ½Ã Á¤½Å·Â È¸º¹");
		case POINT_BLOCK:			return LC_TEXT("¹°¸®Å¸°Ý½Ã ºí·° È®·ü %d%%");
		case POINT_DODGE:			return LC_TEXT("È° °ø°Ý È¸ÇÇ È®·ü %d%%");
		case POINT_RESIST_SWORD:	return LC_TEXT("ÇÑ¼Õ°Ë ¹æ¾î %d%%");
		case POINT_RESIST_TWOHAND:	return LC_TEXT("¾ç¼Õ°Ë ¹æ¾î %d%%");
		case POINT_RESIST_DAGGER:	return LC_TEXT("µÎ¼Õ°Ë ¹æ¾î %d%%");
		case POINT_RESIST_BELL:		return LC_TEXT("¹æ¿ï ¹æ¾î %d%%");
		case POINT_RESIST_FAN:		return LC_TEXT("ºÎÃ¤ ¹æ¾î %d%%");
		case POINT_RESIST_BOW:		return LC_TEXT("È°°ø°Ý ÀúÇ× %d%%");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_CLAW:		return LC_TEXT("µÎ¼Õ°Ë ¹æ¾î %d%%");
#endif
		case POINT_RESIST_FIRE:		return LC_TEXT("È­¿° ÀúÇ× %d%%");
		case POINT_RESIST_ELEC:		return LC_TEXT("Àü±â ÀúÇ× %d%%");
		case POINT_RESIST_MAGIC:	return LC_TEXT("¸¶¹ý ÀúÇ× %d%%");
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
		case POINT_RESIST_MAGIC_REDUCTION:	return LC_TEXT("¸¶¹ý ÀúÇ× %d%%");
#endif
		case POINT_RESIST_WIND:		return LC_TEXT("¹Ù¶÷ ÀúÇ× %d%%");
		case POINT_RESIST_ICE:		return LC_TEXT("³Ã±â ÀúÇ× %d%%");
		case POINT_RESIST_EARTH:	return LC_TEXT("´ëÁö ÀúÇ× %d%%");
		case POINT_RESIST_DARK:		return LC_TEXT("¾îµÒ ÀúÇ× %d%%");
		case POINT_REFLECT_MELEE:	return LC_TEXT("Á÷Á¢ Å¸°ÝÄ¡ ¹Ý»ç È®·ü : %d%%");
		case POINT_REFLECT_CURSE:	return LC_TEXT("ÀúÁÖ µÇµ¹¸®±â È®·ü %d%%");
		case POINT_POISON_REDUCE:	return LC_TEXT("µ¶ ÀúÇ× %d%%");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_BLEEDING_REDUCE:	return LC_TEXT("µ¶ ÀúÇ× %d%%");
#endif
		case POINT_KILL_SP_RECOVER:	return LC_TEXT("%d%% È®·ü·Î ÀûÅðÄ¡½Ã Á¤½Å·Â È¸º¹");
		case POINT_EXP_DOUBLE_BONUS:	return LC_TEXT("%d%% È®·ü·Î ÀûÅðÄ¡½Ã °æÇèÄ¡ Ãß°¡ »ó½Â");
		case POINT_GOLD_DOUBLE_BONUS:	return LC_TEXT("%d%% È®·ü·Î ÀûÅðÄ¡½Ã µ· 2¹è µå·Ó");
		case POINT_ITEM_DROP_BONUS:	return LC_TEXT("%d%% È®·ü·Î ÀûÅðÄ¡½Ã ¾ÆÀÌÅÛ 2¹è µå·Ó");
		case POINT_POTION_BONUS:	return LC_TEXT("¹°¾à »ç¿ë½Ã %d%% ¼º´É Áõ°¡");
		case POINT_KILL_HP_RECOVERY:	return LC_TEXT("%d%% È®·ü·Î ÀûÅðÄ¡½Ã »ý¸í·Â È¸º¹");
		case POINT_ATT_GRADE_BONUS:	return LC_TEXT("°ø°Ý·Â +%d");
		case POINT_DEF_GRADE_BONUS:	return LC_TEXT("¹æ¾î·Â +%d");
		case POINT_MAGIC_ATT_GRADE:	return LC_TEXT("¸¶¹ý °ø°Ý·Â +%d");
		case POINT_MAGIC_DEF_GRADE:	return LC_TEXT("¸¶¹ý ¹æ¾î·Â +%d");
		case POINT_MAX_STAMINA:	return LC_TEXT("ÃÖ´ë Áö±¸·Â +%d");
		case POINT_ATTBONUS_WARRIOR:	return LC_TEXT("¹«»ç¿¡°Ô °­ÇÔ +%d%%");
		case POINT_ATTBONUS_ASSASSIN:	return LC_TEXT("ÀÚ°´¿¡°Ô °­ÇÔ +%d%%");
		case POINT_ATTBONUS_SURA:		return LC_TEXT("¼ö¶ó¿¡°Ô °­ÇÔ +%d%%");
		case POINT_ATTBONUS_SHAMAN:		return LC_TEXT("¹«´ç¿¡°Ô °­ÇÔ +%d%%");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_ATTBONUS_WOLFMAN:	return LC_TEXT("¹«´ç¿¡°Ô °­ÇÔ +%d%%");
#endif
		case POINT_ATTBONUS_MONSTER:	return LC_TEXT("¸ó½ºÅÍ¿¡°Ô °­ÇÔ +%d%%");
		case POINT_MALL_ATTBONUS:		return LC_TEXT("°ø°Ý·Â +%d%%");
		case POINT_MALL_DEFBONUS:		return LC_TEXT("¹æ¾î·Â +%d%%");
		case POINT_MALL_EXPBONUS:		return LC_TEXT("°æÇèÄ¡ %d%%");
		case POINT_MALL_ITEMBONUS:		return LC_TEXT("¾ÆÀÌÅÛ µå·ÓÀ² %d¹è"); // @fixme180 float to int
		case POINT_MALL_GOLDBONUS:		return LC_TEXT("µ· µå·ÓÀ² %d¹è"); // @fixme180 float to int
		case POINT_MAX_HP_PCT:			return LC_TEXT("ÃÖ´ë »ý¸í·Â +%d%%");
		case POINT_MAX_SP_PCT:			return LC_TEXT("ÃÖ´ë Á¤½Å·Â +%d%%");
		case POINT_SKILL_DAMAGE_BONUS:	return LC_TEXT("½ºÅ³ µ¥¹ÌÁö %d%%");
		case POINT_NORMAL_HIT_DAMAGE_BONUS:	return LC_TEXT("ÆòÅ¸ µ¥¹ÌÁö %d%%");
		case POINT_SKILL_DEFEND_BONUS:		return LC_TEXT("½ºÅ³ µ¥¹ÌÁö ÀúÇ× %d%%");
		case POINT_NORMAL_HIT_DEFEND_BONUS:	return LC_TEXT("ÆòÅ¸ µ¥¹ÌÁö ÀúÇ× %d%%");
		case POINT_RESIST_WARRIOR:	return LC_TEXT("¹«»ç°ø°Ý¿¡ %d%% ÀúÇ×");
		case POINT_RESIST_ASSASSIN:	return LC_TEXT("ÀÚ°´°ø°Ý¿¡ %d%% ÀúÇ×");
		case POINT_RESIST_SURA:		return LC_TEXT("¼ö¶ó°ø°Ý¿¡ %d%% ÀúÇ×");
		case POINT_RESIST_SHAMAN:	return LC_TEXT("¹«´ç°ø°Ý¿¡ %d%% ÀúÇ×");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_WOLFMAN:	return LC_TEXT("¹«´ç°ø°Ý¿¡ %d%% ÀúÇ×");
#endif
		default:					return "UNK_ID %d%%"; // @fixme180
	}
}

static bool FN_hair_affect_string(LPCHARACTER ch, char *buf, size_t bufsiz)
{
	if (NULL == ch || NULL == buf)
		return false;

	CAffect* aff = NULL;
	time_t expire = 0;
	struct tm ltm;
	int	year, mon, day;
	int	offset = 0;

	aff = ch->FindAffect(AFFECT_HAIR);

	if (NULL == aff)
		return false;

	expire = ch->GetQuestFlag("hair.limit_time");

	if (expire < get_global_time())
		return false;

	// set apply string
	offset = snprintf(buf, bufsiz, FN_point_string(aff->bApplyOn), aff->lApplyValue);

	if (offset < 0 || offset >= (int) bufsiz)
		offset = bufsiz - 1;

	localtime_r(&expire, &ltm);

	year	= ltm.tm_year + 1900;
	mon		= ltm.tm_mon + 1;
	day		= ltm.tm_mday;

	snprintf(buf + offset, bufsiz - offset, LC_TEXT(" (¸¸·áÀÏ : %d³â %d¿ù %dÀÏ)"), year, mon, day);

	return true;
}

ACMD(do_costume)
{
	char buf[1024]; // @warme015
	const size_t bufferSize = sizeof(buf);

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	CItem* pBody = ch->GetWear(WEAR_COSTUME_BODY);
	CItem* pHair = ch->GetWear(WEAR_COSTUME_HAIR);
	#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	CItem* pMount = ch->GetWear(WEAR_COSTUME_MOUNT);
	#endif
	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	CItem* pAcce = ch->GetWear(WEAR_COSTUME_ACCE);
	#endif
	#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	CItem* pWeapon = ch->GetWear(WEAR_COSTUME_WEAPON);
	#endif

	ch->ChatPacket(CHAT_TYPE_INFO, "COSTUME status:");

	if (pHair)
	{
		const char* itemName = pHair->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  HAIR : %s", itemName);

		for (int i = 0; i < pHair->GetAttributeCount(); ++i)
		{
			const TPlayerItemAttribute& attr = pHair->GetAttribute(i);
			if (0 < attr.bType)
			{
				snprintf(buf, bufferSize, FN_point_string(attr.bType), attr.sValue);
				ch->ChatPacket(CHAT_TYPE_INFO, "     %s", buf);
			}
		}

		if (pHair->IsEquipped() && arg1[0] == 'h')
			ch->UnequipItem(pHair);
	}

	if (pBody)
	{
		const char* itemName = pBody->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  BODY : %s", itemName);

		if (pBody->IsEquipped() && arg1[0] == 'b')
			ch->UnequipItem(pBody);
	}

	#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (pMount)
	{
		const char* itemName = pMount->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  MOUNT : %s", itemName);

		if (pMount->IsEquipped() && arg1[0] == 'm')
			ch->UnequipItem(pMount);
	}
	#endif

	#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if (pAcce)
	{
		const char* itemName = pAcce->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  ACCE : %s", itemName);

		if (pAcce->IsEquipped() && arg1[0] == 'a')
			ch->UnequipItem(pAcce);
	}
	#endif

	#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (pWeapon)
	{
		const char* itemName = pWeapon->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  WEAPON : %s", itemName);

		if (pWeapon->IsEquipped() && arg1[0] == 'w')
			ch->UnequipItem(pWeapon);
	}
	#endif
}

ACMD(do_hair)
{
	char buf[256];

	if (false == FN_hair_affect_string(ch, buf, sizeof(buf)))
		return;

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
}

ACMD(do_inventory)
{
	int	index = 0;
	int	count		= 1;

	char arg1[256];
	char arg2[256];

	LPITEM	item;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: inventory <start_index> <count>");
		return;
	}

	if (!*arg2)
	{
		index = 0;
		str_to_number(count, arg1);
	}
	else
	{
		str_to_number(index, arg1); index = MIN(index, INVENTORY_MAX_NUM);
		str_to_number(count, arg2); count = MIN(count, INVENTORY_MAX_NUM);
	}

	for (int i = 0; i < count; ++i)
	{
		if (index >= INVENTORY_MAX_NUM)
			break;

		item = ch->GetInventoryItem(index);

		ch->ChatPacket(CHAT_TYPE_INFO, "inventory [%d] = %s",
						index, item ? item->GetName() : "<NONE>");
		++index;
	}
}

//gift notify quest command
ACMD(do_gift)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "gift");
}

#ifdef ENABLE_CUBE_RENEWAL
ACMD(do_cube)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (0 == arg1[0])
	{
		return;
	}

	switch (LOWER(arg1[0]))
	{
	case 'o':	// open
		Cube_open(ch);
		break;

	case 'n':	// open
		{
			if (0 == arg2[0] || !isdigit(*arg2))
				return;

			DWORD dwNpcRace = 0;
			str_to_number(dwNpcRace, arg2);
			Cube_open(ch, dwNpcRace);
		}
		break;

	default:
		return;
	}
}
#else
ACMD(do_cube)
{
	if (!ch->CanDoCube())
		return;

	sys_log(1, "CUBE COMMAND <%s>: %s", ch->GetName(), argument);
	int cube_index = 0, inven_index = 0;
	#ifdef ENABLE_SPECIAL_STORAGE
	int inven_type = 0;
	char arg1[256], arg2[256], arg3[256], arg4[256];
	four_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3), arg4, sizeof(arg4));
	#else
	const char *line;
	char arg1[256], arg2[256], arg3[256];
	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));
	#endif

	if (0 == arg1[0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: cube open");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube close");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube add <inveltory_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube delete <cube_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube list");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube cancel");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube make [all]");
		return;
	}

	const std::string& strArg1 = std::string(arg1);

	if (strArg1 == "r_info")
	{
		if (0 == arg2[0])
			Cube_request_result_list(ch);
		else
		{
			if (isdigit(*arg2))
			{
				int listIndex = 0, requestCount = 1;
				str_to_number(listIndex, arg2);

				if (0 != arg3[0] && isdigit(*arg3))
					str_to_number(requestCount, arg3);

				Cube_request_material_info(ch, listIndex, requestCount);
			}
		}

		return;
	}

	switch (LOWER(arg1[0]))
	{
		case 'o':	// open
			Cube_open(ch);
			break;

		case 'c':	// close
			Cube_close(ch);
			break;

		case 'l':	// list
			Cube_show_list(ch);
			break;

		case 'a':	// add cue_index inven_index
			{
				#ifdef ENABLE_SPECIAL_STORAGE
				if (0 == arg2[0] || !isdigit(*arg2) || 0 == arg3[0] || !isdigit(*arg3) || 0 == arg4[0] || !isdigit(*arg4))
				#else
				if (0 == arg2[0] || !isdigit(*arg2) || 0 == arg3[0] || !isdigit(*arg3))
				#endif
					return;

				str_to_number(cube_index, arg2);
				str_to_number(inven_index, arg3);
				#ifdef ENABLE_SPECIAL_STORAGE
				str_to_number(inven_type, arg4);
				Cube_add_item(ch, cube_index, inven_index, inven_type);
				#else
				Cube_add_item(ch, cube_index, inven_index);
				#endif
			}
			break;

		case 'd':	// delete
			{
				if (0 == arg2[0] || !isdigit(*arg2))
					return;

				str_to_number(cube_index, arg2);
				Cube_delete_item (ch, cube_index);
			}
			break;

		case 'm':	// make
			if (0 != arg2[0])
			{
				while (true == Cube_make(ch))
					sys_log(1, "cube make success");
			}
			else
				Cube_make(ch);
			break;

		default:
			return;
	}
}
#endif

ACMD(do_in_game_mall)
{
	if (LC_IsEurope() == true)
	{
		char country_code[3];

		switch (LC_GetLocalType())
		{
			case LC_GERMANY:	country_code[0] = 'd'; country_code[1] = 'e'; country_code[2] = '\0'; break;
			case LC_FRANCE:		country_code[0] = 'f'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_ITALY:		country_code[0] = 'i'; country_code[1] = 't'; country_code[2] = '\0'; break;
			case LC_SPAIN:		country_code[0] = 'e'; country_code[1] = 's'; country_code[2] = '\0'; break;
			case LC_UK:			country_code[0] = 'e'; country_code[1] = 'n'; country_code[2] = '\0'; break;
			case LC_TURKEY:		country_code[0] = 't'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_POLAND:		country_code[0] = 'p'; country_code[1] = 'l'; country_code[2] = '\0'; break;
			case LC_PORTUGAL:	country_code[0] = 'p'; country_code[1] = 't'; country_code[2] = '\0'; break;
			case LC_GREEK:		country_code[0] = 'g'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_RUSSIA:		country_code[0] = 'r'; country_code[1] = 'u'; country_code[2] = '\0'; break;
			case LC_DENMARK:	country_code[0] = 'd'; country_code[1] = 'k'; country_code[2] = '\0'; break;
			case LC_BULGARIA:	country_code[0] = 'b'; country_code[1] = 'g'; country_code[2] = '\0'; break;
			case LC_CROATIA:	country_code[0] = 'h'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_MEXICO:		country_code[0] = 'm'; country_code[1] = 'x'; country_code[2] = '\0'; break;
			case LC_ARABIA:		country_code[0] = 'a'; country_code[1] = 'e'; country_code[2] = '\0'; break;
			case LC_CZECH:		country_code[0] = 'c'; country_code[1] = 'z'; country_code[2] = '\0'; break;
			case LC_ROMANIA:	country_code[0] = 'r'; country_code[1] = 'o'; country_code[2] = '\0'; break;
			case LC_HUNGARY:	country_code[0] = 'h'; country_code[1] = 'u'; country_code[2] = '\0'; break;
			case LC_NETHERLANDS: country_code[0] = 'n'; country_code[1] = 'l'; country_code[2] = '\0'; break;
			case LC_USA:		country_code[0] = 'u'; country_code[1] = 's'; country_code[2] = '\0'; break;
			case LC_CANADA:	country_code[0] = 'c'; country_code[1] = 'a'; country_code[2] = '\0'; break;
			default:
				if (test_server == true)
				{
					country_code[0] = 'd'; country_code[1] = 'e'; country_code[2] = '\0';
				}
				break;
		}

		char buf[512+1];
		char sas[33];
		MD5_CTX ctx;
		const char sas_key[] = "GF6931";

		snprintf(buf, sizeof(buf), "%u%u%s", ch->GetPlayerID(), ch->GetAID(), sas_key);

		MD5Init(&ctx);
		MD5Update(&ctx, (const unsigned char *) buf, strlen(buf));
		#ifdef __FreeBSD__
		MD5End(&ctx, sas);
		#else
		static const char hex[] = "0123456789abcdef";
		unsigned char digest[16];
		MD5Final(digest, &ctx);
		int i;
		for (i = 0; i < 16; ++i) {
			sas[i+i] = hex[digest[i] >> 4];
			sas[i+i+1] = hex[digest[i] & 0x0f];
		}
		sas[i+i] = '\0';
		#endif

		snprintf(buf, sizeof(buf), "mall http://%s/ishop?pid=%u&c=%s&sid=%d&sas=%s",
				g_strWebMallURL.c_str(), ch->GetPlayerID(), country_code, g_server_id, sas);

		ch->ChatPacket(CHAT_TYPE_COMMAND, buf);
	}
}

ACMD(do_dice)
{
	char arg1[256], arg2[256];
	int start = 1, end = 100;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1 && *arg2)
	{
		start = atoi(arg1);
		end = atoi(arg2);
	}
	else if (*arg1 && !*arg2)
	{
		start = 1;
		end = atoi(arg1);
	}

	end = MAX(start, end);
	start = MIN(start, end);

	int n = number(start, end);

	#ifdef ENABLE_DICE_SYSTEM
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_DICE_INFO, LC_TEXT("%s´ÔÀÌ ÁÖ»çÀ§¸¦ ±¼·Á %d°¡ ³ª¿Ô½À´Ï´Ù. (%d-%d)"), ch->GetName(), n, start, end);
	else
		ch->ChatPacket(CHAT_TYPE_DICE_INFO, LC_TEXT("´ç½ÅÀÌ ÁÖ»çÀ§¸¦ ±¼·Á %d°¡ ³ª¿Ô½À´Ï´Ù. (%d-%d)"), n, start, end);
	#else
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_INFO, LC_TEXT("%s´ÔÀÌ ÁÖ»çÀ§¸¦ ±¼·Á %d°¡ ³ª¿Ô½À´Ï´Ù. (%d-%d)"), ch->GetName(), n, start, end);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ç½ÅÀÌ ÁÖ»çÀ§¸¦ ±¼·Á %d°¡ ³ª¿Ô½À´Ï´Ù. (%d-%d)"), n, start, end);
	#endif
}

#ifdef ENABLE_NEWSTUFF
ACMD(do_click_safebox)
{
	if ((ch->GetGMLevel() <= GM_PLAYER) && (ch->GetDungeon() || ch->GetWarMap()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot open the safebox in dungeon or at war."));
		return;
	}

	ch->SetSafeboxOpenPosition();
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeSafeboxPassword");
}
ACMD(do_force_logout)
{
	LPDESC pDesc=DESC_MANAGER::instance().FindByCharacterName(ch->GetName());
	if (!pDesc)
		return;
	pDesc->DelayedDisconnect(0);
}
#endif

ACMD(do_click_mall)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeMallPassword");
}

ACMD(do_ride)
{
	sys_log(1, "[DO_RIDE] start");

	if (ch->IsDead() || ch->IsStun() || ch->IsPolymorphed())
		return;

	#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (ch->GetProtectTime("mountcostumer") > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Yavaþ olmalýsýn! %d saniye sonra deneyebilirsiniz.", ch->GetProtectTime("mountcostumer") - get_global_time());
		return;
	}
	#endif

	if (ch->IsHorseRiding())
	{
		// sys_log(1, "[DO_RIDE] stop riding");
		ch->StopRiding();
		return;
	}

	if (ch->GetMountVnum())
	{
		// sys_log(1, "[DO_RIDE] unmount");
		do_unmount(ch, NULL, 0, 0);
		return;
	}

	if (ch->GetHorse() != NULL)
	{
		// sys_log(1, "[DO_RIDE] start riding");
		ch->StartRiding();
		#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		ch->SetProtectTime("mountcostumer", get_global_time() + 2);
		#endif
		return;
	}

	for (BYTE i=0; i<INVENTORY_MAX_NUM; ++i)
	{
		LPITEM item = ch->GetInventoryItem(i);
		if (NULL == item)
			continue;

		/*if (item->IsRideItem())
		{
			if (
				NULL==ch->GetWear(WEAR_UNIQUE1)
				|| NULL==ch->GetWear(WEAR_UNIQUE2)
				#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
				|| NULL==ch->GetWear(WEAR_COSTUME_MOUNT)
				#endif
			)
			{
				sys_log(1, "[DO_RIDE] USE UNIQUE ITEM");
				//ch->EquipItem(item);
				ch->UseItem(TItemPos (INVENTORY, i));
				return;
			}
		}*/
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»À» ¸ÕÀú ¼ÒÈ¯ÇØÁÖ¼¼¿ä."));
}

#if defined(ENABLE_RENEWAL_AFFECT_SHOWER)
ACMD(do_remove_affect)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	DWORD affect = 0;
	str_to_number(affect, vecArgs[1].c_str());
	if (AFFECT_POLYMORPH == affect)
	{
		if (!ch->IsPolymorphed())
			return;
		ch->SetPolymorph(0);
	}
	if (ch->FindAffect(affect))
		ch->RemoveAffect(affect);
}
#endif

#ifdef ENABLE_OPEN_RANGE_NPC
DWORD shopvid;
int shopvnum;

struct NPCBul
{
	NPCBul() {};
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch->IsNPC())
			{
				shopvid = ch->GetVID();
			}
		}
	}
};

ACMD(do_npcac)
{
	if (quest::CQuestManager::instance().GetEventFlag("uzak_market_2_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Sistem suan icin devre disi!");
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	str_to_number(shopvnum, arg1);

	if(!shopvnum || shopvnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("uzaknpchata"));
		return;
	}

	if (!ch->IsPC())
		return;

	if (ch->IsHack())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Zindanda bu islemi yapamazsin.");
		return;
	}

	LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());
	if (NULL != pSecMap)
	{
		NPCBul f;
		pSecMap->for_each(f);

		LPCHARACTER yeninpc = CHARACTER_MANAGER::instance().Find(shopvid);
		if (yeninpc)
		{
			CShopManager::instance().NPCAC(ch, yeninpc, shopvnum);
		}
	}

	return;
}
#endif

#ifdef ENABLE_SPLIT_ITEMS_SYSTEM
ACMD(do_split_items)
{
	if (!ch)
		return;

	if (quest::CQuestManager::instance().GetEventFlag("toplu_ayirma") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Sistem suan icin devre disi!");
		return;
	}

	if (ch->GetProtectTime("split_items") > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Yavaþ olmalýsýn! %d saniye sonra deneyebilirsiniz.", ch->GetProtectTime("split_items") - get_global_time());
		return;
	}
	ch->SetProtectTime("split_items", get_global_time() + 5);

	if (ch->IsHack())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	if (!ch->CanWarp())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	char arg1[256], arg2[256], arg3[256], arg4[256];
	two_arguments(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg1 || !*arg2 || !*arg3 || !*arg4)
	{
		return;
	}

	WORD cell = 0;
	short count = 0;
	WORD destCell = 0;
	WORD window_type = 0;

	str_to_number(cell, arg1);
	str_to_number(count, arg2);
	str_to_number(destCell, arg3);
	str_to_number(window_type, arg4);

	#ifdef ENABLE_SPECIAL_STORAGE
	if (cell < 0)
		return;

	if ((window_type == INVENTORY && INVENTORY_MAX_NUM <= cell) || 
		(window_type == UPGRADE_INVENTORY && SPECIAL_INVENTORY_MAX_NUM <= cell) || 
		(window_type == BOOK_INVENTORY && SPECIAL_INVENTORY_MAX_NUM <= cell) || 
		(window_type == STONE_INVENTORY && SPECIAL_INVENTORY_MAX_NUM <= cell) || 
		(window_type == CHEST_INVENTORY && SPECIAL_INVENTORY_MAX_NUM <= cell))
		return;

	if (window_type != INVENTORY && 
		window_type != UPGRADE_INVENTORY &&
		window_type != BOOK_INVENTORY &&
		window_type != STONE_INVENTORY &&
		window_type != CHEST_INVENTORY)
		return;
	#else
	if (cell < 0 || INVENTORY_MAX_NUM <= cell)
		return;
	#endif

	LPITEM item = ch->GetItem(TItemPos(window_type, cell));
	if (item != NULL)
	{
		short itemCount = item->GetCount();
		if ((count <= 0) || (itemCount == 1) || (itemCount <= count) || (item->GetWindow() != window_type) || (cell == destCell)) // Fix
			return;

		while (itemCount > 0)
		{
			if (itemCount <= count)
				break;

			if (count > itemCount)
				count = itemCount;

			int iEmptyPosition = -1;
			if (window_type == INVENTORY)
				iEmptyPosition = ch->GetEmptyInventoryFromIndex(destCell, item->GetSize());
			#ifdef ENABLE_SPECIAL_STORAGE
			else if (window_type == UPGRADE_INVENTORY)
				iEmptyPosition = ch->GetEmptyUpgradeInventoryFromIndex(destCell, item->GetSize());
			else if (window_type == BOOK_INVENTORY)
				iEmptyPosition = ch->GetEmptyBookInventoryFromIndex(destCell, item->GetSize());
			else if (window_type == STONE_INVENTORY)
				iEmptyPosition = ch->GetEmptyStoneInventoryFromIndex(destCell, item->GetSize());
			else if (window_type == CHEST_INVENTORY)
				iEmptyPosition = ch->GetEmptyChestInventoryFromIndex(destCell, item->GetSize());
			#endif

			if (iEmptyPosition == -1)
				break;

			itemCount -= count;
			ch->MoveItem(TItemPos(window_type, cell), TItemPos(window_type, iEmptyPosition), count);
		}
	}
}
#endif

#ifdef ENABLE_OTO_TOPLAMA
ACMD(do_oto_toplama)
{
	char arg1[256];
	char arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	BYTE mode = 0;
	str_to_number(mode, arg1);

	BYTE status = 0;
	str_to_number(status, arg2);

		 if (status > 1) { status = 1; }
	else if (status < 0) { status = 0; }

	if (mode == 0) // Metin
	{
		ch->PointChange(POINT_OTO_METIN, status);
	}
	else if (mode == 1) // Canavar
	{
		ch->PointChange(POINT_OTO_CANAVAR, status);
	}
	else if (mode == 2) // Boss
	{
		ch->PointChange(POINT_OTO_PATRON, status);
	}
}
#endif

#ifdef ENABLE_ANTI_EXP
ACMD(do_anti_exp)
{
	if (ch->GetProtectTime("antiexp") > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Yavaþ olmalýsýn! %d saniye sonra deneyebilirsiniz.", ch->GetProtectTime("antiexp") - get_global_time());
		return;
	}

	ch->SetProtectTime("antiexp", get_global_time() + 2);

	if (ch->GetAntiExp() == 0)
		ch->PointChange(POINT_ANTI_EXP, 1);
	else
		ch->PointChange(POINT_ANTI_EXP, -1);

	ch->ChatPacket(CHAT_TYPE_COMMAND, "SetAntiExp");
}
#endif

#ifdef ENABLE_ENV_DUZENLEME
ACMD(do_sort_special_storage)
{
	if (quest::CQuestManager::instance().GetEventFlag("env_duzenleme") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Sistem suan icin devre disi!");
		return;
	}

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	for (int m = 0; m < 4; ++m)
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item;

			switch(m)
			{
				case 0:
					item = ch->GetUpgradeInventoryItem(i);
					break;
				case 1:
					item = ch->GetBookInventoryItem(i);
					break;
				case 2:
					item = ch->GetStoneInventoryItem(i);
					break;
				default:
					item = ch->GetChestInventoryItem(i);
					break;
			}

			if(!item)
				continue;

			if (item->isLocked())
				continue;

			#ifdef ENABLE_LOCKED_SYSTEM
			if (item->IsItemLocked())
				continue;
			#endif

			if (item->GetCount() == g_bItemCountLimit)
				continue;

			if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
			{
				for (int j = i; j < SPECIAL_INVENTORY_MAX_NUM; ++j)
				{
					LPITEM item2;

					switch (m)
					{
						case 0:
							item2 = ch->GetUpgradeInventoryItem(j);
							break;
						case 1:
							item2 = ch->GetBookInventoryItem(j);
							break;
						case 2:
							item2 = ch->GetStoneInventoryItem(j);
							break;
						default:
							item2 = ch->GetChestInventoryItem(j);
							break;
					}

					if (!item2)
						continue;

					if (item2->isLocked())
						continue;

					#ifdef ENABLE_LOCKED_SYSTEM
					if (item2->IsItemLocked())
						continue;
					#endif

					if (item2->GetVnum() == item->GetVnum())
					{
						bool bStopSockets = false;

						for (int k = 0; k < ITEM_SOCKET_MAX_NUM; ++k)
						{
							if (item2->GetSocket(k) != item->GetSocket(k))
							{
								bStopSockets = true;
								break;
							}
						}

						if (bStopSockets)
							continue;

						#ifdef ENABLE_NEW_MAX_COUNT
						short bAddCount = MIN(g_bItemCountLimit - item->GetCount(), item2->GetCount());
						#else
						BYTE bAddCount = MIN(g_bItemCountLimit - item->GetCount(), item2->GetCount());
						#endif

						item->SetCount(item->GetCount() + bAddCount);
						item2->SetCount(item2->GetCount() - bAddCount);
						continue;
					}
				}
			}
		}
	}
}

ACMD(do_sort_items)
{
	if (quest::CQuestManager::instance().GetEventFlag("env_duzenleme") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Sistem suan icin devre disi!");
		return;
	}

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		LPITEM item = ch->GetInventoryItem(i);

		if (!item)
			continue;

		if (item->isLocked())
			continue;

		#ifdef ENABLE_LOCKED_SYSTEM
		if (item->IsItemLocked())
			continue;
		#endif

		if (item->GetCount() == g_bItemCountLimit)
			continue;

		if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			for (int j = i; j < INVENTORY_MAX_NUM; ++j)
			{
				LPITEM item2 = ch->GetInventoryItem(j);

				if (!item2)
					continue;

				if (item2->isLocked())
					continue;

				#ifdef ENABLE_LOCKED_SYSTEM
				if (item2->IsItemLocked())
					continue;
				#endif

				if (item2->GetVnum() == item->GetVnum())
				{
					bool bStopSockets = false;

					for (int k = 0; k < ITEM_SOCKET_MAX_NUM; ++k)
					{
						if (item2->GetSocket(k) != item->GetSocket(k))
						{
							bStopSockets = true;
							break;
						}
					}

					if (bStopSockets)
						continue;

					#ifdef ENABLE_NEW_MAX_COUNT
					short bAddCount = MIN(g_bItemCountLimit - item->GetCount(), item2->GetCount());
					#else
					BYTE bAddCount = MIN(g_bItemCountLimit - item->GetCount(), item2->GetCount());
					#endif

					item->SetCount(item->GetCount() + bAddCount);
					item2->SetCount(item2->GetCount() - bAddCount);
					continue;
				}
			}
		}
	}
}
#endif

#ifdef ENABLE_CHANGE_CHANNEL_SYSTEM
ACMD(do_change_channel)
{
	if (!ch)
		return;

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ae¨ùO ¥ìC¨úu¨öA¢¥I¢¥U."));
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Usage: channel <new channel>"));
		return;
	}

	short channel;
	str_to_number(channel, arg1);

	if (channel < 1 || channel > 8)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Please enter a valid channel."));
		return;
	}

	if (channel == g_bChannel)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are already on channel %d."), g_bChannel);
		return;
	}

	if (g_bChannel == 99)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The map you are at is cross-channel, changing won't have any effect."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change channel while in a dungeon."));
		return;
	}

	TPacketChangeChannel p;
	p.channel = channel;
	p.lMapIndex = ch->GetMapIndex();
	db_clientdesc->DBPacket(HEADER_GD_FIND_CHANNEL, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}
#endif

#ifdef ENABLE_MOVE_CHANNEL
ACMD(DoChangeChannel)
{
	if (!ch)
		return;

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ae¨ùO ¥ìC¨úu¨öA¢¥I¢¥U."));
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change channel while in a dungeon."));
		return;
	}

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	if (ch->IsExchangeTime())
	{
		return;
	}

	if (ch->IsHack(false, true, TRADE_CHECK_TIME))
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;

	WORD channel = 0;
	str_to_number(channel, arg1);
	if (!channel)
		return;

	ch->ChangeChannel(channel);
}
#endif

#ifdef __RANKING_SYSTEM__
ACMD(do_rank_update)
{
	char arg1[256], arg2[256], arg3[256];
	three_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3));
	if (!*arg1 || !*arg2 || !*arg3) { return; }
	int categoryIndex = atoi(arg1);
	int raceIndex = atoi(arg2);
	int rankIndex = atoi(arg3);
	ch->SendRanks(RANK_LOAD, categoryIndex, raceIndex, rankIndex);
}
#endif

#ifdef ENABLE_EVENT_MANAGER
ACMD(do_event_manager)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "info")
	{
		CHARACTER_MANAGER::Instance().SendDataPlayer(ch);
	}
	else if (vecArgs[1] == "remove")
	{
		if (!ch->IsGM())
			return;

		if (vecArgs.size() < 3)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "event indexi yanlis!!");
			return; 
		}

		BYTE removeIndex = 0;
		str_to_number(removeIndex, vecArgs[2].c_str());

		if (CHARACTER_MANAGER::Instance().CloseEventManuel(removeIndex))
			ch->ChatPacket(CHAT_TYPE_INFO, "Kapatýldý!");
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "Herhangi bir event yok!");
	}
	else if (vecArgs[1] == "update")
	{
		if (!ch->IsGM())
			return;

		const BYTE subHeader = EVENT_MANAGER_UPDATE;
		db_clientdesc->DBPacket(HEADER_GD_EVENT_MANAGER, 0, &subHeader, sizeof(BYTE));

		ch->ChatPacket(CHAT_TYPE_INFO, "Eventler guncellendi!");
	}
}
#endif

#ifdef ENABLE_DRAGON_COIN_SHOP_SYSTEM
#include "nm_item_shop.h"
ACMD(do_dc_shop_open_log)
{
	CDragonCoinShopManager::instance().OpenLog(ch);
}
#endif

#ifdef ENABLE_MULTI_FARM_BLOCK
ACMD(do_multi_farm)
{
	if (!ch->GetDesc())
		return;

	if (ch->GetProtectTime("multi-farm") > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Yavaþ olmalýsýn! %d saniye sonra deneyebilirsiniz.", ch->GetProtectTime("multi-farm") - get_global_time());
		return;
	}

	if (ch->IsGM())
		ch->SetProtectTime("multi-farm", get_global_time() + 1);
	else
		ch->SetProtectTime("multi-farm", get_global_time() + 10);

	CHARACTER_MANAGER::Instance().CheckMultiFarmAccount(ch->GetDesc()->GetHostName(), ch->GetPlayerID(), ch->GetName(), !ch->GetRewardStatus());
}
#endif

#ifdef ENABLE_MULTI_HUNT_BLOCK
ACMD(do_multi_hunt)
{
	if (!ch->GetDesc())
		return;

	if (ch->GetProtectTime("multi-hunt") > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Yavaþ olmalýsýn! %d saniye sonra deneyebilirsiniz.", ch->GetProtectTime("multi-hunt") - get_global_time());
		return;
	}

	if (ch->IsGM())
		ch->SetProtectTime("multi-hunt", get_global_time() + 1);
	else
		ch->SetProtectTime("multi-hunt", get_global_time() + 10);

	CHARACTER_MANAGER::Instance().CheckMultiHuntAccount(ch->GetDesc()->GetHostName(), ch->GetPlayerID(), ch->GetName(), !ch->GetMultiHuntStatus());
}
#endif

#ifdef ENABLE_MULTI_FISH_BLOCK
ACMD(do_multi_fish)
{
	if (!ch->GetDesc())
		return;

	if (ch->GetProtectTime("multi-fish") > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Yavaþ olmalýsýn! %d saniye sonra deneyebilirsiniz.", ch->GetProtectTime("multi-fish") - get_global_time());
		return;
	}

	if (ch->IsGM())
		ch->SetProtectTime("multi-fish", get_global_time() + 1);
	else
		ch->SetProtectTime("multi-fish", get_global_time() + 10);

	CHARACTER_MANAGER::Instance().CheckMultiFishAccount(ch->GetDesc()->GetHostName(), ch->GetPlayerID(), ch->GetName(), !ch->GetMultiFishStatus());
}
#endif

#ifdef ENABLE_BIYOLOG
ACMD(do_bio)
{
	// Check if the biyolog system is disabled
	if (quest::CQuestManager::instance().GetEventFlag("biyolog_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Sistem suan icin devre disi!");
		return;
	}

	// Check if trade window is open
	if (ch->PreventTradeWindow(WND_TIME, true))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("pencere_kapat"));
		return;
	}

	// Level requirement check
	if (ch->GetLevel() < 30)
		return;

	// Parse arguments
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2)
		return;

	// Get quest flags
	const int level = ch->GetQuestFlag("bio.level");
	const int bitirici = ch->GetQuestFlag("bio.bitirici");
	int count = ch->GetQuestFlag("bio.count");
	int time = ch->GetQuestFlag("bio.time");

	// Various level checks
	if (level >= bio_max || level < 1)
		return;

	if (ch->GetLevel() < bio_data[level][0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Bir görev atamak için 30. seviyeye sahip olmalisiniz.");
		return;
	}

	auto item_name = [](auto vnum) { return ITEM_MANAGER::instance().GetTable(vnum)->szLocaleName; };

	if (vecArgs[1] == "mission" && vecArgs.size() >= 4)
	{
		// Parse mission parameters
		BYTE isOzut = 0;
		BYTE isUnutkanlik = 0;
		str_to_number(isOzut, vecArgs[2].c_str());
		str_to_number(isUnutkanlik, vecArgs[3].c_str());

		// Process mission if count is less than required
		if (count < bio_data[level][2])
		{
			// Check if player has required item
			const auto requiredItemVnum = bio_data[level][1];
			if (ch->CountSpecifyItem(requiredItemVnum) < 1 && bitirici == 0)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "Görevi bitirmek için yeterli %s yok.", item_name(requiredItemVnum));
				return;
			}

			// Handle forgetfulness item usage
			const auto currentTime = get_global_time();
			const auto forgetItemVnum = bio_data[level][16];
			if (isUnutkanlik)
			{
				if (ch->CountSpecifyItem(forgetItemVnum) > 0 && time > currentTime)
				{
					ch->RemoveSpecifyItem(forgetItemVnum, 1);
				}
				else if (time > currentTime)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Bekleme süresi geçmediyse bir nesne teslim edemezsiniz. %s kullanarak zamani azaltabilirsiniz.", item_name(forgetItemVnum));
					return;
				}
			}
			else if (time > currentTime && bitirici == 0)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "Bekleme süresi geçmediyse bir nesne teslim edemezsiniz. %s kullanarak zamani azaltabilirsiniz.", item_name(forgetItemVnum));
				return;
			}

			// Handle essence item usage
			bool isOzutDrink = false;
			const auto essenceItemVnum = bio_data[level][15];
			if (isOzut && ch->CountSpecifyItem(essenceItemVnum) > 0)
			{
				ch->RemoveSpecifyItem(essenceItemVnum, 1);
				isOzutDrink = true;
			}

			// Calculate success probability
			const int prob = isOzutDrink ? bio_data[level][4] + 100 : bio_data[level][4];

			// Remove required item if not using finisher
			if (bitirici == 0)
				ch->RemoveSpecifyItem(requiredItemVnum, 1);

			// Process mission result
			time = currentTime + bio_data[level][3];
			if (prob >= number(1, 100))
			{
				// Success case
				count += 1;
				ch->SetQuestFlag("bio.count", count);
				ch->SetQuestFlag("bio.time", time);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biodata %d %d %d", level, count, time);
				ch->ChatPacket(CHAT_TYPE_INFO, "%s nesnesi kaliteli ve olumlu sonuçlar verdi!", item_name(requiredItemVnum));
			}
			else
			{
				ch->SetQuestFlag("bio.time", time);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biodata %d %d %d", level, count, time);
				ch->ChatPacket(CHAT_TYPE_INFO, "%s nesnesi kötü kosullardaydi, bana daha fazlasini getirdi.", item_name(requiredItemVnum));
			}

			// Check if sacred stone is required
			if (bio_data[level][5] != 0 && count == bio_data[level][2])
			{
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biostone %d", level);
			}
			return; // Buradaki return, orijinal kodla uyumluluk için eklendi
		}

		// Handle sacred stone requirement when count matches required count
		if (bio_data[level][5] != 0)
		{
			if (count == bio_data[level][2])
			{
				const auto sacredStoneVnum = bio_data[level][5];
				if (ch->CountSpecifyItem(sacredStoneVnum) < 1 && bitirici == 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Bana kutsal nesne getirmeden görevi bitiremezsin: %s!", item_name(sacredStoneVnum));
					return;
				}
				else
				{
					if (bitirici == 0)
						ch->RemoveSpecifyItem(sacredStoneVnum, 1);

					ch->SetQuestFlag("bio.count", count + 1);

					if (bio_data[level][14] == 0)
					{
						ch->SetQuestFlag("bio.count", 0);
						ch->SetQuestFlag("bio.level", level + 1);
						ch->SetQuestFlag("bio.time", 0);
						ch->SetQuestFlag("bio.bitirici", 0);

						auto applyBioAffect = [&ch](int applyType, long applyValue)
						{
							if (applyType != 0)
							{
								long value = applyValue;
								if (CAffect* affect = ch->FindAffect(AFFECT_BIO_1, applyType))
								{
									value += affect->lApplyValue;
									ch->RemoveAffect(affect);
								}
								ch->AddAffect(AFFECT_BIO_1, applyType, value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);
							}
						};

						applyBioAffect(bio_data[level][6], bio_data[level][7]);
						applyBioAffect(bio_data[level][8], bio_data[level][9]);
						applyBioAffect(bio_data[level][10], bio_data[level][11]);
						applyBioAffect(bio_data[level][12], bio_data[level][13]);

						const int newLevel = level + 1;
						if (newLevel >= bio_max)
						{
							#ifdef ENABLE_REWARD_SYSTEM
							CHARACTER_MANAGER::Instance().SetRewardData(REWARD_BIOLOG, ch->GetName(), true);
							#endif
							ch->ChatPacket(CHAT_TYPE_INFO, "Tüm görevlerinizi bitirdin. Umarim yeni görevlerle çok yakinda görmeyi umuyorum.");
							ch->ChatPacket(CHAT_TYPE_COMMAND, "bioempty");
							return;
						}

						ch->ChatPacket(CHAT_TYPE_COMMAND, "biodata %d %d %d", newLevel, 0, 0);
					}
					else
					{
						ch->ChatPacket(CHAT_TYPE_COMMAND, "bioodul %d", level);
						return;
					}
				}
			}
			else if (count == bio_data[level][2] + 1)
				return;
		}
		else
		{
			if (count == bio_data[level][2])
			{
				if (bio_data[level][14] == 0)
				{
					ch->SetQuestFlag("bio.count", 0);
					ch->SetQuestFlag("bio.level", level + 1);
					ch->SetQuestFlag("bio.time", 0);
					ch->SetQuestFlag("bio.bitirici", 0);

					auto applyBioAffect = [&ch](int applyType, long applyValue)
					{
						if (applyType != 0)
						{
							long value = applyValue;
							if (CAffect* affect = ch->FindAffect(AFFECT_BIO_1, applyType))
							{
								value += affect->lApplyValue;
								ch->RemoveAffect(affect);
							}
							ch->AddAffect(AFFECT_BIO_1, applyType, value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);
						}
					};

					applyBioAffect(bio_data[level][6], bio_data[level][7]);
					applyBioAffect(bio_data[level][8], bio_data[level][9]);
					applyBioAffect(bio_data[level][10], bio_data[level][11]);
					applyBioAffect(bio_data[level][12], bio_data[level][13]);

					const int newLevel = level + 1;
					if (newLevel >= bio_max)
					{
						#ifdef ENABLE_REWARD_SYSTEM
						CHARACTER_MANAGER::Instance().SetRewardData(REWARD_BIOLOG, ch->GetName(), true);
						#endif
						ch->ChatPacket(CHAT_TYPE_INFO, "Tüm görevlerinizi bitirdin. Umarim yeni görevlerle çok yakinda görmeyi umuyorum.");
						ch->ChatPacket(CHAT_TYPE_COMMAND, "bioempty");
						return;
					}

					ch->ChatPacket(CHAT_TYPE_COMMAND, "biodata %d %d %d", newLevel, 0, 0);
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_COMMAND, "bioodul %d", level);
					return;
				}
			}
		}
	}
	else if (vecArgs[1] == "gift" && vecArgs.size() >= 3)
	{
		BYTE index = 0;
		str_to_number(index, vecArgs[2].c_str());

		if (index > 3)
			return;

		const bool hasSacredStone = bio_data[level][5] != 0;
		const int requiredCount = hasSacredStone ? bio_data[level][2] + 1 : bio_data[level][2];

		if (count != requiredCount)
			return;

		ch->SetQuestFlag("bio.count", 0);
		ch->SetQuestFlag("bio.level", level + 1);
		ch->SetQuestFlag("bio.time", 0);
		ch->SetQuestFlag("bio.bitirici", 0);

		auto applyBioAffect = [&ch](int applyType, long applyValue, int affectIndex, int selectedIndex)
		{
			if (applyType != 0 && affectIndex == selectedIndex)
			{
				long value = applyValue;
				if (CAffect* affect = ch->FindAffect(AFFECT_BIO_1, applyType))
				{
					value += affect->lApplyValue;
					ch->RemoveAffect(affect);
				}
				ch->AddAffect(AFFECT_BIO_1, applyType, value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);
			}
		};

		applyBioAffect(bio_data[level][6], bio_data[level][7], 0, index);
		applyBioAffect(bio_data[level][8], bio_data[level][9], 1, index);
		applyBioAffect(bio_data[level][10], bio_data[level][11], 2, index);

		char flag[100];
		sprintf(flag, "bio.bonus%d", level);
		ch->SetQuestFlag(flag, index + 1);

		const int newLevel = level + 1;
		if (newLevel >= bio_max)
		{
			#ifdef ENABLE_REWARD_SYSTEM
			CHARACTER_MANAGER::Instance().SetRewardData(REWARD_BIOLOG, ch->GetName(), true);
			#endif
			ch->ChatPacket(CHAT_TYPE_INFO, "Tüm görevlerinizi bitirdin. Umarim yeni görevlerle çok yakinda görmeyi umuyorum.");
			ch->ChatPacket(CHAT_TYPE_COMMAND, "bioempty");
			return;
		}

		ch->ChatPacket(CHAT_TYPE_COMMAND, "biodata %d %d %d", newLevel, 0, 0);
	}
}
#endif
#ifdef ENABLE_ITEMCI_LOG
std::string forbiddenList[2] = { "Ilan Ekle", "Ilan Duzenle" };
const char* __GetHwidForPlayer(uint32_t accID)
{
	if (accID == 0)
		return "";

	auto pMsg(DBManager::instance().DirectQuery("SELECT hwid_routher FROM account.account WHERE id = %d LIMIT 0,1;", accID));
	if (pMsg->Get()->uiNumRows > 0)
	{
		MYSQL_ROW mRow = mysql_fetch_row(pMsg->Get()->pSQLResult);
		if (strlen(mRow[0]))
			return mRow[0];
	}
	return "";
}

ACMD(do_trader_found)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
	{
		return;
	}

	int forbiddenID = 0;
	str_to_number(forbiddenID, arg1);

	std::string catchCause = "LIST_SIZES_NOT_EQUAL";

	if (forbiddenID < _countof(forbiddenList))
		catchCause = forbiddenList[forbiddenID];

	const TAccountTable& accTable = ch->GetDesc()->GetAccountTable();

	auto pMsg2(DBManager::instance().DirectQuery("INSERT INTO server_settings.itemci_log (catch_cause, account_id, account_name, account_hwid, account_ip, player_id, player_name, date) VALUES('%s',%d, '%s','%s','%s',%d, '%s', NOW());",
		catchCause.c_str(), accTable.id, accTable.login, __GetHwidForPlayer(accTable.id), ch->GetDesc()->GetHostName(), ch->GetPlayerID(), ch->GetName()));

}
#endif

#ifdef __ARE_YOU_IN_OR_NOT__
ACMD(do_ayirn_game)
{
	ch->ArgumentAyirnGame(argument);
}
#endif

#ifdef __AUTO_SKILL_READER__
ACMD(do_auto_skill_reader)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "status")
	{
		if (vecArgs.size() < 4) { return; }
		BYTE skillIdx, status;
		if (!str_to_number(skillIdx, vecArgs[2].c_str()) || !str_to_number(status, vecArgs[3].c_str()))
			return;
		ch->GetAutoSkill(skillIdx, status ? true : false);
	}
}
#endif

#ifdef __AUTO_SELL__
ACMD(do_autosell)
{
	if (!ch->CanHandleItem())
		return;
	else if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID()) && quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
		return;

	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "drop")
	{
		return;
	}
	else if (vecArgs[1] == "destroy")
	{
		if (vecArgs.size() < 3) { return; }
		std::vector<std::string> vecArgsEx;
		split_argument(vecArgs[2].c_str(), vecArgsEx, "#");
		std::map<DWORD, long> m_mapItems;
		for (WORD j = 0; j < vecArgsEx.size(); ++j)
		{
			std::vector<std::string> vecArgsData;
			split_argument(vecArgsEx[j].c_str(), vecArgsData, "?");
			if (vecArgsData.size() != 3)
				continue;

			DWORD dwItemIdx;
			BYTE bPageIdx;
			WORD wSlotIdx;
			if (!str_to_number(dwItemIdx, vecArgsData[0].c_str()) || !str_to_number(bPageIdx, vecArgsData[1].c_str()) || !str_to_number(wSlotIdx, vecArgsData[2].c_str()))
				continue;

			const TItemTable* p = ITEM_MANAGER::instance().GetTable(dwItemIdx);
			if (!p)
				continue;

			LPITEM item = ch->GetItem(TItemPos(bPageIdx, wSlotIdx));
			if (!item || item->GetVnum() != dwItemIdx || item->IsEquipped() || item->IsExchanging() || item->isLocked() || IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_PKDROP))
				continue;

			#ifdef ENABLE_LOCKED_SYSTEM
			if (item->IsItemLocked())
				continue;
			#endif

			auto it = m_mapItems.find(item->GetVnum());
			if (it != m_mapItems.end())
				it->second += item->GetCount();
			else
				m_mapItems.emplace_hint(it, item->GetVnum(), item->GetCount());

			ITEM_MANAGER::Instance().RemoveItem(item);
		}

		for (const auto& [itemIdx, itemCount] : m_mapItems)
		{
			const TItemTable* p = ITEM_MANAGER::instance().GetTable(itemIdx);
			ch->ChatPacket(CHAT_TYPE_INFO, "Auto Destroyed: %s x%d", p->szLocaleName, itemCount);
		}
	}
	else if (vecArgs[1] == "sell")
	{
		if (vecArgs.size() < 4) { return; }
		const bool check3Bonus = vecArgs[2] == "1" ? true : false;
		std::vector<std::string> vecArgsEx;
		split_argument(vecArgs[3].c_str(), vecArgsEx, "#");
		long long totalmoney = 0;
		std::map<DWORD, std::pair<long long, long>> m_mapRemovedItems;
		for (WORD j = 0; j < vecArgsEx.size(); ++j)
		{
			std::vector<std::string> vecArgsData;
			split_argument(vecArgsEx[j].c_str(), vecArgsData, "?");
			if (vecArgsData.size() != 3)
				continue;

			DWORD dwItemIdx;
			BYTE bPageIdx;
			WORD wSlotIdx;
			if (!str_to_number(dwItemIdx, vecArgsData[0].c_str()) || !str_to_number(bPageIdx, vecArgsData[1].c_str()) || !str_to_number(wSlotIdx, vecArgsData[2].c_str()))
				continue;

			const TItemTable* p = ITEM_MANAGER::instance().GetTable(dwItemIdx);
			if (!p)
				continue;

			LPITEM item = ch->GetItem(TItemPos(bPageIdx, wSlotIdx));
			if (!item || item->GetVnum() != dwItemIdx || item->IsEquipped() || item->IsExchanging() || item->isLocked())
				continue;

			#ifdef ENABLE_LOCKED_SYSTEM
			if (item->IsItemLocked())
				continue;
			#endif

			if (check3Bonus && item->GetAttributeCount() >= 3)
				continue;

			auto it = m_mapRemovedItems.find(item->GetVnum());
			if (it != m_mapRemovedItems.end())
				it->second.second += item->GetCount();
			else
				m_mapRemovedItems.emplace_hint(it, item->GetVnum(), std::make_pair(p->dwShopBuyPrice, item->GetCount()));

			const long long price = p->dwShopBuyPrice * item->GetCount();
			totalmoney += price;

			ITEM_MANAGER::Instance().RemoveItem(item);
		}

		if (totalmoney != 0)
			ch->PointChange(POINT_GOLD, totalmoney);

		for (const auto& [itemIdx, itemPair] : m_mapRemovedItems)
		{
			const TItemTable* p = ITEM_MANAGER::instance().GetTable(itemIdx);
			ch->ChatPacket(CHAT_TYPE_INFO, "Auto selled: %s x%d for %lld yang.", p->szLocaleName, itemPair.second, itemPair.second * itemPair.first);
		}
	}
}
#endif

#ifdef ENABLE_EXCHANGE_LOG
ACMD(do_ex_log)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "load")
		ch->SendExchangeLogPacket(SUB_EXCHANGELOG_LOAD);
	else if (vecArgs[1] == "load_item")
	{
		if (vecArgs.size() < 3) { return; }
		DWORD logID;
		if(!str_to_number(logID, vecArgs[2].c_str()))
			return;
		ch->SendExchangeLogPacket(SUB_EXCHANGELOG_LOAD_ITEM, logID);
	}
	else if (vecArgs[1] == "delete")
	{
		if (vecArgs.size() < 4) { return; }
		const std::string playerCode(ch->GetDesc()->GetAccountTable().social_id);
		if (playerCode != vecArgs[2])
			return;
		if (vecArgs[3] == "all")
			ch->DeleteExchangeLog(0);
		else
		{
			for (DWORD j = 3; j < vecArgs.size(); ++j)
			{
				DWORD logID;
				if(!str_to_number(logID, vecArgs[j].c_str()))
					return;
				ch->DeleteExchangeLog(logID);
			}
		}
	}
}
#endif

#ifdef ENABLE_WHEEL_OF_FORTUNE_SYSTEM
#include "wheel_of_fortune.h"

ACMD(do_wheel_of_fortune)
{
	// if (quest::CQuestManager::instance().GetEventFlag("wof_system_enable") == 0) { return; }

	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "open")
	{
		time_t now = get_global_time();
		time_t tTime = ch->GetProtectTime("wof.cooldown");
		if (tTime > get_global_time())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "%d saniye beklemelisin!", int(tTime-now));
			return;
		}
		ch->SetProtectTime("wof.cooldown", (now+3));
		if (CWheelOfFortune::instance().IsBusy(ch)) { return; }
		if (vecArgs.size() < 3) { return; }
		BYTE bType = WHEEL_TYPE_NONE;
		str_to_number(bType, vecArgs[2].c_str());
		CWheelOfFortune::instance().Open(ch, bType);
	}
	else if (vecArgs[1] == "start")
	{
		time_t now = get_global_time();
		time_t tTime = ch->GetProtectTime("wof.cooldown");
		if (tTime > get_global_time())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "%d saniye beklemelisin!", int(tTime-now));
			return;
		}
		ch->SetProtectTime("wof.cooldown", (now+1));

		if (CWheelOfFortune::instance().IsBusy(ch)) { return; }
		if (vecArgs.size() < 3) { return; }

		if (ch->GetProtectTime("WOFWorking") != 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("wof_is_working"));
			return;
		}

		BYTE bType = WHEEL_TYPE_NONE;
		str_to_number(bType, vecArgs[2].c_str());

		int iVnum = CWheelOfFortune::instance().NeedItemVnum(bType);
		int iCount = CWheelOfFortune::instance().NeedItemCount(bType);

		if (bType == WHEEL_TYPE_EP)
		{
			if (ch->GetCoinNm() < iCount)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("cark_yeterli_ep_yok_%d"), iCount);
				return;
			}
			if (ch->UpdateCoin(ch->GetAID(), -iCount)) { ch->RefreshCoinAndCash(); } else { return; }
		}
		else if (bType == WHEEL_TYPE_MP)
		{
			if (ch->GetCashNm() < iCount)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("cark_yeterli_mp_yok_%d"), iCount);
				return;
			}
			if (ch->UpdateCash(ch->GetAID(), -iCount)) { ch->RefreshCoinAndCash(); } else { return; }
		}
		else if ((bType >= WHEEL_TYPE_BRONZE_YANG) && (bType <= WHEEL_TYPE_EMERALD_YANG))
		{
			if (ch->GetGold() < (int64_t)iCount)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("cark_yeterli_yang_yok_%d"), iCount);
				return;
			}
			ch->GoldChange(-iCount);
		}
		else
		{
			if ((iVnum < 1) || (iCount < 1)) { return; }

			const TItemTable * pTable = ITEM_MANAGER::instance().GetTable(iVnum);
			if (!pTable) { return; }

			if (ch->CountSpecifyItem(iVnum) < iCount)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("wof_cannot_item %s x%d"), pTable->szLocaleName, iCount);
				return;
			}
			ch->RemoveSpecifyItem(iVnum, iCount);
		}

		CWheelOfFortune::instance().Fortune(ch, bType);
	}
	else if (vecArgs[1] == "done")
	{
		if (ch->GetProtectTime("WOFWorking") == 0) { return; }
		LPITEM pkItem = ch->AutoGiveItem(ch->GetProtectTime("WOFLuckyItemVnum"), ch->GetProtectTime("WOFLuckyItemCount"), -1, true
						#ifdef ENABLE_LOCKED_SYSTEM
						, (ch->GetProtectTime("WOFType") == WHEEL_TYPE_MP ? true : false)
						#endif
		);
		#ifdef ENABLE_LOCKED_SYSTEM
		if (pkItem && (ch->GetProtectTime("WOFType") == WHEEL_TYPE_MP))
		{
			pkItem->SetLocked(LOCKED_ISHOP_MP);
		}
		#endif
		ch->ChatPacket(CHAT_TYPE_COMMAND, "WofGetGiftData %d %d", ch->GetProtectTime("WOFLuckyItemVnum"), ch->GetProtectTime("WOFLuckyItemCount"));
		ch->SetProtectTime("WOFLuckyItemVnum", 0);
		ch->SetProtectTime("WOFLuckyItemCount", 0);
		ch->SetProtectTime("WOFWorking", 0);
	}
}

ACMD(do_wheel_of_fortune_gm)
{
	char arg1[256];
	char arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "/wof_gm (open or close) wheel_type");
		return;
	}

	const std::string& strArg1 = std::string(arg1);
	const std::string& strArg2 = std::string(arg2);

	const char * c_apszWheelList[10][2] = 
	{
		{ "ep", "wof_ep_enable" },
		{ "mp", "wof_mp_enable" },
		{ "bronze_coin", "wof_bronze_c_enable" },
		{ "silver_coin", "wof_silver_c_enable" },
		{ "gold_coin", "wof_gold_c_enable" },
		{ "emerald_coin", "wof_emerald_c_enable" },
		{ "bronze_yang", "wof_bronze_y_enable" },
		{ "silver_yang", "wof_silver_y_enable" },
		{ "gold_yang", "wof_gold_y_enable" },
		{ "emerald_yang", "wof_emerald_y_enable" },
	};

	if (strArg1 == "open")
	{
		if (strArg2 == "all")
		{
			quest::CQuestManager::instance().RequestSetEventFlag("wof_system_enable", 1);
			quest::CQuestManager::instance().RequestSetEventFlag("wof_event_enable", 1);
			for (BYTE i = 0; i < _countof(c_apszWheelList); ++i)
			{
				quest::CQuestManager::instance().RequestSetEventFlag(c_apszWheelList[i][1], 1);
			}
			CWheelOfFortune::instance().Status(false);
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Butun carklar kullanima acildi."));
			return;
		}

		BYTE bType = 0;
		for (BYTE i = 0; i < _countof(c_apszWheelList); ++i)
		{
			if (strArg2 == c_apszWheelList[i][0])
			{
				quest::CQuestManager::instance().RequestSetEventFlag(c_apszWheelList[i][1], 1);
				CWheelOfFortune::instance().Status(false);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("cark_kullanima_acildi"));
				bType = i;
				break;
			}
		}

		switch(bType)
		{
			case 2:
			case 3:
			case 4:
			case 5:
				{
					if (quest::CQuestManager::instance().GetEventFlag("wof_system_enable") >= 1) break;
					quest::CQuestManager::instance().RequestSetEventFlag("wof_system_enable", 1); 
				}
				break;

			case 6:
			case 7:
			case 8:
			case 9:
				{
					if (quest::CQuestManager::instance().GetEventFlag("wof_event_enable") >= 1) break;
					quest::CQuestManager::instance().RequestSetEventFlag("wof_event_enable", 1);
				}
				break;

			default:
				break;
		}
	}
	else if (strArg1 == "close")
	{
		if (strArg2 == "all")
		{
			quest::CQuestManager::instance().RequestSetEventFlag("wof_system_enable", 0);
			quest::CQuestManager::instance().RequestSetEventFlag("wof_event_enable", 0);
			for (BYTE i = 0; i < _countof(c_apszWheelList); ++i)
			{
				quest::CQuestManager::instance().RequestSetEventFlag(c_apszWheelList[i][1], 0);
			}
			CWheelOfFortune::instance().Status(true);
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Butun carklar kullanima kapatildi."));
			return;
		}

		BYTE bType = 0;
		for (BYTE i = 0; i < _countof(c_apszWheelList); ++i)
		{
			if (strArg2 == c_apszWheelList[i][0])
			{
				quest::CQuestManager::instance().RequestSetEventFlag(c_apszWheelList[i][1], 0);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("cark_kullanima_kapatildi"));
				bType = i;
				break;
			}
		}

		switch(bType)
		{
			case 2:
			case 3:
			case 4:
			case 5:
				{
					if (quest::CQuestManager::instance().GetEventFlag("wof_system_enable") <= 0) break;
					if (quest::CQuestManager::instance().GetEventFlag(c_apszWheelList[2][1]) >= 1) break;
					if (quest::CQuestManager::instance().GetEventFlag(c_apszWheelList[3][1]) >= 1) break;
					if (quest::CQuestManager::instance().GetEventFlag(c_apszWheelList[4][1]) >= 1) break;
					if (quest::CQuestManager::instance().GetEventFlag(c_apszWheelList[5][1]) >= 1) break;
					quest::CQuestManager::instance().RequestSetEventFlag("wof_system_enable", 0); 
				}
				break;

			case 6:
			case 7:
			case 8:
			case 9:
				{
					if (quest::CQuestManager::instance().GetEventFlag("wof_event_enable") <= 0) break;
					if (quest::CQuestManager::instance().GetEventFlag(c_apszWheelList[6][1]) >= 1) break;
					if (quest::CQuestManager::instance().GetEventFlag(c_apszWheelList[7][1]) >= 1) break;
					if (quest::CQuestManager::instance().GetEventFlag(c_apszWheelList[8][1]) >= 1) break;
					if (quest::CQuestManager::instance().GetEventFlag(c_apszWheelList[9][1]) >= 1) break;
					quest::CQuestManager::instance().RequestSetEventFlag("wof_event_enable", 0);
				}
				break;

			default:
				break;
		}
	}
}
#endif

#ifdef __RENEWAL_BRAVE_CAPE__
ACMD(do_brave_cape)
{
	ch->GetBraveCapeCMDCompare(argument);
}
#endif

#ifdef ENABLE_TANAKA_EVENT
#include "NewTanakaEvent.h"
ACMD(do_tanaka_event)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;

	if (!ch || !ch->IsPC())
		return;

	if (0 == arg1[0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bunu yapman engellendi."));
		sys_log(0, "do_tanaka_event:1:Gelen argument deger sifir deneyen karakter: %s denedigi karakterler: %s", ch->GetName(), arg1);
		return;
	}

	const auto& strArg1 = std::string(arg1);
	if (strArg1 == "arena")
	{
		CNewTanakaEventManager::instance().WarpPlayer(ch);
	}
}

ACMD(do_tanaka_test)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;

	const auto& strArg1 = std::string(arg1);
	if (strArg1 == "start")
	{
		CNewTanakaEventManager::instance().OpenEvent();
	}
	else if(strArg1 == "end")
	{
		CNewTanakaEventManager::instance().EndEvent();
	}
	else if(strArg1 == "spawn")
	{
		CNewTanakaEventManager::instance().StartEvent();
	}
}
#endif

#ifdef ENABLE_TELEPORTION_WIZARD_SYSTEM
#include "teleportion_wizard.h"
ACMD(do_teleportion_wizard)
{
	char arg1[256];
	char arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
		return;

	const std::string& strArg1 = std::string(arg1);
	if (strArg1 == "open")
	{
		CTeleportionWizard::instance().SendPacket(ch);
	}
	else if (strArg1 == "warp")
	{
		if (!*arg2)
			return;

		int id = 0;
		str_to_number(id, arg2);
		CTeleportionWizard::instance().Warp(ch, id);
	}
	else if (strArg1 == "drop")
	{
		if (!*arg2)
			return;

		int id = 0;
		str_to_number(id, arg2);
		CTeleportionWizard::instance().DropInfo(ch, id);
	}
}
#endif

#ifdef ENABLE_CAPTCHA_SELECT_SYSTEM
ACMD(do_captcha_select_gm)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;

	const std::string& strArg1 = std::string(arg1);
	if (strArg1 == "open")
	{
		quest::CQuestManager::instance().RequestSetEventFlag("captcha_disable", 0);
	}
	else if (strArg1 == "close")
	{
		quest::CQuestManager::instance().RequestSetEventFlag("captcha_disable", 1);
	}
	else if (strArg1 == "test")
	{
		ch->SetQuestFlag("captchanew.time", 0);
		ch->SetQuestFlag("captchanew.status", 0);
		ch->SetCaptchaTime(0);
		ch->SetCaptchaAttackStatus(false);
		ch->CaptchaQuestion();
	}
}

ACMD(do_captcha_select_reply)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	int iData = 0;
	str_to_number(iData, arg1);
	ch->CaptchaReply(iData);
}
#endif

#ifdef __AUTO_HUNT__
ACMD(do_auto_hunt)
{
	ch->GetAutoHuntCommand(argument);
}
#endif

#ifdef __BACK_DUNGEON__
ACMD(do_back_dungeon)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "warp")
	{
		if (vecArgs.size() < 5) { return; }

		WORD wPort;
		BYTE bChannel;
		DWORD dwMapIdx;
		if (!str_to_number(wPort, vecArgs[2].c_str()) || !str_to_number(bChannel, vecArgs[3].c_str()) || !str_to_number(dwMapIdx, vecArgs[4].c_str()))
			return;

		CHARACTER_MANAGER::Instance().WarpBackDungeon(ch, wPort, bChannel, dwMapIdx);
	}
}
#endif

#if defined(ENABLE_DEFENCE_SHIELD)
ACMD(do_defence_shield_status)
{
	if (!ch || !ch->GetDesc())
		return;

	int iTime = ch->GetProtectTime("defenceshield.time");
	if (iTime > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Yavas olmalisin! %d saniye sonra deneyebilirsiniz."), (iTime - get_global_time()));
		return;
	}
	ch->SetProtectTime("defenceshield.time", get_global_time()+1);

	CAffect* pAffect = ch->FindAffect(AFFECT_DEFENCE_SHIELD);
	if (NULL == pAffect)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Aktif bir korumaniz yok!"));
		return;
	}

	if (pAffect->dwFlag == AFF_DEFENCE_SHIELD)
	{
		pAffect->dwFlag = 0;
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Koruma kalkani deaktif edildi!"));
	}
	else
	{
		pAffect->dwFlag = AFF_DEFENCE_SHIELD;
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Koruma kalkani aktif edildi!"));
	}
}
#endif

#ifdef ENABLE_REWARD_SYSTEM
ACMD(do_update_reward_data)
{
	if (!ch)
		return;

	CHARACTER_MANAGER::Instance().SendRewardInfo(ch);
}
#endif

#ifdef ENABLE_GUILD_STATISTICS_SYSTEM
//--TAB_TUSU--//
ACMD(do_guild_statistics)
{
	if (!ch) { return; }
	if (!ch->GetDesc()) { return; }
	if (!ch->GetGuild()) { return; }
	if (ch->IsObserverMode()) { return; }
	auto pMap = CWarMapManager::instance().Find(ch->GetMapIndex());
	if (pMap) { pMap->LoadGuildWarStatistics(ch); }
}
#endif

#ifdef ENABLE_OKEY_CARD_SYSTEM
ACMD(do_cards)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	switch (LOWER(arg1[0]))
	{
		case 'o':    // open
			if (isdigit(*arg2))
			{
				DWORD safemode = 0;
				str_to_number(safemode, arg2);
				ch->Cards_open(safemode);
			}
			break;
		case 'p':    // open
			ch->Cards_pullout();
			break;
		case 'e':    // open
			ch->CardsEnd();
			break;
		case 'd':    // open
			if (isdigit(*arg2))
			{
				DWORD destroy_index = 0;
				str_to_number(destroy_index, arg2);
				ch->CardsDestroy(destroy_index);
			}
			break;
		case 'a':    // open
			if (isdigit(*arg2))
			{
				DWORD accpet_index = 0;
				str_to_number(accpet_index, arg2);
				ch->CardsAccept(accpet_index);
			}
			break;
		case 'r':    // open
			if (isdigit(*arg2))
			{
				DWORD restore_index = 0;
				str_to_number(restore_index, arg2);
				ch->CardsRestore(restore_index);
			}
			break;
		default:
			return;
	}
}
#endif

#ifdef CIT_SYSTEM
ACMD(do_war_ready)
{
	if (!ch || !ch->GetDesc() || !ch->GetGuild()) { return; }

	if (!CWarMapManager::instance().IsWarMap(ch->GetMapIndex())) { return; }

	if (ch->GetGuild()->GetMasterPID() != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Sadece lider yapabilir.");
		return;
	}

	CWarMap * pMap = ch->GetWarMap();
	BYTE bIdx = 0;

	if (!pMap || !pMap->GetTeamIndex(ch->GetGuild()->GetID(), bIdx))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Bir hata olustu.");
		return;
	}

	pMap->BeReady(bIdx);
}
#endif
ACMD(do_set_rate)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	switch (LOWER(arg1[0]))
	{
		case 'g':    // gold
			if (isdigit(*arg2))
			{
				DWORD safemode = 0;
				str_to_number(safemode, arg2);
				CHARACTER_MANAGER::instance().SetMobGoldDropRate(safemode);
				CHARACTER_MANAGER::instance().SetMobGoldDropRatePremium(safemode);
				
			}
			break;
		case 'e':    // exp
			if (isdigit(*arg2))
			{
				DWORD safemode = 0;
				str_to_number(safemode, arg2);
				CHARACTER_MANAGER::instance().SetMobExpRate(safemode);
				CHARACTER_MANAGER::instance().SetMobExpRatePremium(safemode);
			}
		break;
		default:
			return;
	}
}
#ifdef SPIN_WHEEL
constexpr DWORD RewardItems[3][spinWheelMaxItem][3] = {
	{// level range 1-55
		//vnum     count   rarity(0-2)
		{30006,    1, 0},
		{30005,    1, 0},
		{30021,    1, 0},
		{27870,    2, 0},
		{27873,    2,  0},
		{71151,    5,  0},
		{50113,    4,  0},
		{50111,    4,  0},
		{71094,    4,  0},
		{71001,    4,  0},
		{51502,    1,  0},
		{71084,    10,  0},
		{71085,    5,  0},
		{27799,    1,  0},
		{27987,    1,  1},
		{70024,    2,  1},
		{38053,    1,  1},
		{30220,    1,  1},
		{30221,    1,  1},
		{30222,    1,  1},
		{81506,    1,  1},
		{50050,    2,  1},
		{39022,    1,  1},
		{70039,    1,  1},
		{50216,    1,  1},
		{80013,    1,  2},
	},
	{// level range 56-75
		//vnum   count  rarity(0-2)
		{30047,    1, 0},
		{27870,    3, 0},
		{27873,    3, 0},
		{71085,    5, 0},
		{71084,    15, 0},
		{50111,    6, 0},
		{50115,    6, 0},
		{71094,    6, 0},
		{71001,    6,  0},
		{70102,    10,  0},
		{51523,    25,  0},
		{38053,    2,  0},
		{51503,    1, 1},
		{50513,    2, 1},
		{70024,    3, 1},
		{30223,    1, 1},
		{30224,    1, 1},
		{30225,    1, 1},
		{50050,    3,  1},
		{81506,    2,  1},
		{25040,    5,  1},
		{39022,    2,  1},
		{70039,    1,  1},
		{70043,    1,  1},
		{50183,    2,  1},
		{80017,    1,  2},
	},

	{// level range 76-99
		//vnum   count  rarity(0-2)
		{27870,    3,  0},
		{27873,    3,  0},
		{76034,    3,  0},
		{71001,    3,  0},
		{50115,    3,  0},
		{50513,    2,  0},
		{70024,    2,  0},
		{70102,    3,  0},
		{25040,    3,  0},
		{50825,    3,  0},
		{50826,    3,  1},
		{11124,    1,  1},
		{27799,    2,  1},
		{27987,    3,  1},
		{71419,    4,  1},
		{39022,    3,  1},
		{70039,    3,  1},
		{38053,    3,  1},
		{70005,    1,  1},
		{70043,    1, 1},
		{50216,    1, 1},
		{50183,    1,  1},
		{81506,    3,  1},
		{30226,    1,  1},
		{30227,    1,  1},
		{80017,    1, 2}
	},
};

ACMD(do_spin_wheel)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!ch || !*arg1 || !ch->GetDesc())
		return;
	if (ch->IsHack())
		return;

	if (quest::CQuestManager::instance().GetEventFlag("metin_spin") == 0)
		return;

	BYTE subHeader = 0;
	str_to_number(subHeader, arg1);
	switch (subHeader)
	{
	case 0:
	{
		quest::PC* pPc = quest::CQuestManager::instance().GetPC(ch->GetPlayerID());
		if (!pPc)
			return;
		const auto pCount = pPc->GetFlag("spin_wheel.metinstone_count");
		if (pCount < SPINWHEEL_NEEDCOUNT)
			return;

		pPc->SetFlag("spin_wheel.metinstone_count", 0);

		if (ch->GetSpinReward() != 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "You already spinning");
			return;
		}
		constexpr std::array<int, 3> LEVEL_RANGE = { { 1, 56, 76 } };
		auto lIndex = std::distance(LEVEL_RANGE.begin(), std::lower_bound(LEVEL_RANGE.begin(), LEVEL_RANGE.end(), ch->GetLevel())) - 1;
		if (lIndex < 0 || lIndex >= std::size(RewardItems))
			return;
		std::size_t arrayLength = 0;
		for (const auto& item : RewardItems[lIndex]) {
			if (item[0] != 0)
				++arrayLength;
		}
		auto rIndex = number(0, arrayLength - 1);

		DWORD dwItemVnum = RewardItems[lIndex][rIndex][0];
		DWORD dwItemCount = RewardItems[lIndex][rIndex][1];

		ch->SetSpinReward(dwItemVnum, dwItemCount);
		TPacketGCSpinWheel p;
		p.bHeader = HEADER_GC_SPIN_WHEEL;
		for (int i = 0; i < spinWheelMaxItem; i++)
		{
			p.itemVnums[i] = RewardItems[lIndex][i][0];
			p.itemCounts[i] = RewardItems[lIndex][i][1];
			p.itemRarities[i] = RewardItems[lIndex][i][2];
		}
		p.dwRewardItem = dwItemVnum;

		ch->GetDesc()->Packet(&p, sizeof(p));
	}
	break;

	case 1:
	{
		const auto aReward = ch->GetSpinReward();
		const auto aCount = ch->GetSpinRewardCount();
		if (aReward == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Error");
			return;
		}

		ch->SetSpinReward(0, 0);
		LPITEM item = ITEM_MANAGER::instance().CreateItem(aReward, aCount, 0, true);
		if (!item)
			return;
		ch->AutoGiveItem(item);
		ch->SetExchangeTime();
		ch->ChatPacket(CHAT_TYPE_COMMAND, "SpinReward %u", aReward);
		sys_log(0, "SpinWheel PlayerName %s, Reward %u", ch->GetName(), aReward);
	}
	break;
	}
}

ACMD(do_get_spin_rewards)
{
	for (int i = 0; i < std::size(RewardItems); i++)
	{
		TPacketGCSpinWheelRewards p;
		p.bHeader = HEADER_GC_SPINWHEEL_REWARD;
		p.bIndex = i;

		for (int j = 0; j < spinWheelMaxItem; j++)
		{
			p.itemVnums[j] = RewardItems[i][j][0];
			p.itemCounts[j] = RewardItems[i][j][1];
			p.itemRarities[j] = RewardItems[i][j][2];
		}
		ch->GetDesc()->Packet(&p, sizeof(p));
	}
	ch->ChatPacket(CHAT_TYPE_COMMAND, "OpenSpinWheelReward");
}

ACMD(do_set_spin_wheel)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD count = 0;
	str_to_number(count, arg1);
	count = MINMAX(0, count, 100);

	if (!ch || !*arg1 || !ch->GetDesc())
		return;

	quest::PC* pPc = quest::CQuestManager::instance().GetPC(ch->GetPlayerID());
	if (!pPc)
		return;
	pPc->SetFlag("spin_wheel.metinstone_count", count);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "SetPercentage %d", count);
}
#endif