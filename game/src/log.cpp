#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "log.h"

#include "char.h"
#include "desc.h"
#include "item.h"

#ifdef ENABLE_GAME_IN_DISABLE_SYSTEM
#include "GetEventFlagSystem.h"
#endif

static char	__escape_hint[1024];

LogManager::LogManager() : m_bIsConnect(false)
{
}

LogManager::~LogManager()
{
}

bool LogManager::Connect(const char * host, const int port, const char * user, const char * pwd, const char * db)
{
	if (m_sql.Setup(host, user, pwd, db, g_stLocale.c_str(), false, port))
		m_bIsConnect = true;

	return m_bIsConnect;
}

void LogManager::Query(const char * c_pszFormat, ...)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);

	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

	if (test_server)
		sys_log(0, "LOG: %s", szQuery);

	#ifdef ENABLE_SQL_INJECT_CONTROL_ON_QUERY
	std::string sQuery(szQuery);
	m_sql.AsyncQuery(sQuery.substr(0, sQuery.find_first_of(";") == -1 ? sQuery.length() : sQuery.find_first_of(";")).c_str());
	#else
	m_sql.AsyncQuery(szQuery);
	#endif
}

bool LogManager::IsConnected()
{
	return m_bIsConnect;
}

size_t LogManager::EscapeString(char* dst, size_t dstSize, const char *src, size_t srcSize)
{
	return m_sql.EscapeString(dst, dstSize, src, srcSize);
}

void LogManager::ItemLog(DWORD dwPID, DWORD x, DWORD y, DWORD dwItemID, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, DWORD dwVnum)
{
	if (gef_item_log_disable) { return; }
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHint, strlen(c_pszHint));

	Query("INSERT INTO log%s (type, time, who, x, y, what, how, hint, ip, vnum) VALUES('ITEM', NOW(), %u, %u, %u, %u, '%s', '%s', '%s', %u)",
			get_table_postfix(), dwPID, x, y, dwItemID, c_pszText, __escape_hint, c_pszIP, dwVnum);
}

void LogManager::ItemLog(LPCHARACTER ch, LPITEM item, const char * c_pszText, const char * c_pszHint)
{
	if (gef_item_log_disable) { return; }
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	if (NULL == ch || NULL == item)
	{
		sys_err("character or item nil (ch %p item %p text %s)", get_pointer(ch), get_pointer(item), c_pszText);
		return;
	}

	ItemLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), item->GetID(),
			NULL == c_pszText ? "" : c_pszText,
			c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "",
			item->GetOriginalVnum());
}

void LogManager::ItemLog(LPCHARACTER ch, int itemID, int itemVnum, const char * c_pszText, const char * c_pszHint)
{
	if (gef_item_log_disable) { return; }
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	ItemLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), itemID, c_pszText, c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "", itemVnum);
}

///#########################################################################################################################
void LogManager::ItemRemoveLog(DWORD dwPID, DWORD x, DWORD y, DWORD dwItemID, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, DWORD dwVnum)
{
	if (gef_item_log_disable) { return; }
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHint, strlen(c_pszHint));

	Query("INSERT INTO item_remove%s (type, time, who, x, y, what, how, hint, ip, vnum) VALUES('ITEM', NOW(), %u, %u, %u, %u, '%s', '%s', '%s', %u)",
			get_table_postfix(), dwPID, x, y, dwItemID, c_pszText, __escape_hint, c_pszIP, dwVnum);
}

void LogManager::ItemRemoveLog(LPCHARACTER ch, LPITEM item, const char * c_pszText, const char * c_pszHint)
{
	if (gef_item_log_disable) { return; }
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	if (NULL == ch || NULL == item)
	{
		sys_err("character or item nil (ch %p item %p text %s)", get_pointer(ch), get_pointer(item), c_pszText);
		return;
	}

	ItemRemoveLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), item->GetID(),
			NULL == c_pszText ? "" : c_pszText,
			c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "",
			item->GetOriginalVnum());
}

void LogManager::ItemRemoveLog(LPCHARACTER ch, int itemID, int itemVnum, const char * c_pszText, const char * c_pszHint)
{
	if (gef_item_log_disable) { return; }
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	ItemRemoveLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), itemID, c_pszText, c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "", itemVnum);
}
///#########################################################################################################################

void LogManager::CharLog(DWORD dwPID, DWORD x, DWORD y, DWORD dwValue, const char * c_pszText, const char * c_pszHint, const char * c_pszIP)
{
	return;
}

void LogManager::CharLog(LPCHARACTER ch, DWORD dw, const char * c_pszText, const char * c_pszHint)
{
	return;
}

void LogManager::LoginLog(bool isLogin, DWORD dwAccountID, DWORD dwPID, BYTE bLevel, BYTE bJob, DWORD dwPlayTime)
{
	return;
}

//anticheat
void LogManager::HackRaporLog(const char * c_pszHackName, const char * c_pszLogin, const char * c_pszName, const char * c_pszIP)
{
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHackName, strlen(c_pszHackName));
	Query("INSERT INTO server_settings.hack_log (kullaniciadi, karakteradi, ip, rapor, tarih) VALUES('%s', '%s', '%s', '%s', NOW())", c_pszLogin, c_pszName, c_pszIP, __escape_hint);
}

void LogManager::HackRaporLog(const char * c_pszHackName, LPCHARACTER ch)
{
	if (!ch || !ch->GetDesc())
	{
		sys_err("HackRaporLog");
		return;
	}

	if (ch->GetDesc())
	{
		HackRaporLog(c_pszHackName,
				ch->GetDesc()->GetAccountTable().login,
				ch->GetName(),
				ch->GetDesc()->GetHostName());
	}
}
//anticheat

#ifdef ENABLE_FULL_YANG
void LogManager::MoneyLog(BYTE type, DWORD vnum, int64_t gold)
#else
void LogManager::MoneyLog(BYTE type, DWORD vnum, int gold)
#endif
{
	return;
}

void LogManager::HackLog(const char * c_pszHackName, const char * c_pszLogin, const char * c_pszName, const char * c_pszIP)
{
	return;
}

void LogManager::HackLog(const char * c_pszHackName, LPCHARACTER ch)
{
	return;
}

void LogManager::HackCRCLog(const char * c_pszHackName, const char * c_pszLogin, const char * c_pszName, const char * c_pszIP, DWORD dwCRC)
{
	return;
}

void LogManager::GoldBarLog(DWORD dwPID, DWORD dwItemID, GOLDBAR_HOW eHow, const char* c_pszHint)
{
	return;
}

void LogManager::CubeLog(DWORD dwPID, DWORD x, DWORD y, DWORD item_vnum, DWORD item_uid, int item_count, bool success)
{
	return;
}

void LogManager::SpeedHackLog(DWORD pid, DWORD x, DWORD y, int hack_count)
{
	return;
}

void LogManager::ChangeNameLog(DWORD pid, const char *old_name, const char *new_name, const char *ip)
{
	return;
}

void LogManager::GMCommandLog(DWORD dwPID, const char* szName, const char* szIP, BYTE byChannel, const char* szCommand)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), szCommand, strlen(szCommand));

	Query("INSERT INTO command_log%s (userid, server, ip, port, username, command, date ) "
			"VALUES(%u, 999, '%s', %u, '%s', '%s', NOW()) ",
			get_table_postfix(), dwPID, szIP, byChannel, szName, __escape_hint);
}

void LogManager::RefineLog(DWORD pid, const char* item_name, DWORD item_id, int item_refine_level, int is_success, const char* how)
{
	return;
}

void LogManager::ShoutLog(BYTE bChannel, BYTE bEmpire, const char * pszText)
{
	return;
}

void LogManager::LevelLog(LPCHARACTER pChar, unsigned int level, unsigned int playhour)
{
	return;
}

void LogManager::BootLog(const char * c_pszHostName, BYTE bChannel)
{
	return;
}

void LogManager::FishLog(DWORD dwPID, int prob_idx, int fish_id, int fish_level, DWORD dwMiliseconds, DWORD dwVnum, DWORD dwValue)
{
	return;
}

void LogManager::QuestRewardLog(const char * c_pszQuestName, DWORD dwPID, DWORD dwLevel, int iValue1, int iValue2)
{
	return;
}

void LogManager::DetailLoginLog(bool isLogin, LPCHARACTER ch)
{
	return;
}

void LogManager::DragonSlayLog(DWORD dwGuildID, DWORD dwDragonVnum, DWORD dwStartTime, DWORD dwEndTime)
{
	return;
}

void LogManager::ChatLog(DWORD where, DWORD who_id, const char* who_name, DWORD whom_id, const char* whom_name, const char* type, const char* msg, const char* ip)
{
	return;
}

void LogManager::InvalidServerLog(enum eLocalization eLocaleType, const char* pcszIP, const char* pszRevision)
{
	// revision sql inject exploit fix
	// @fixme101 last %s to '%s'
	return;
}

#ifdef ENABLE_OKEY_CARD_SYSTEM
void LogManager::OkayEventLog(int dwPID, const char * c_pszText, int points)
{
	Query("INSERT INTO log.okay_event(pid, name, points) VALUES(%d, '%s', %d)", dwPID, c_pszText, points);
}
#endif

void LogManager::ItemErrorLog(DWORD dwID, DWORD dwPID, DWORD dwLastPID, const char * c_pszText, const char * c_pszHint)
{
	Query("INSERT INTO log.item_error(id, pid, lastpid, how, hint, time) VALUES(%u, %u, %u, '%s', '%s', NOW())", dwID, dwPID, dwLastPID, c_pszText, c_pszHint);
}
