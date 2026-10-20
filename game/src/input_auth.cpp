#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "input.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "protocol.h"
#include "locale_service.h"
#include "db.h"

#ifndef __WIN32__
	#include "limit_time.h"
#endif

#include "utils.h"

// #define ENABLE_ACCOUNT_W_SPECIALCHARS
bool FN_IS_VALID_LOGIN_STRING(const char *str)
{
	const char*	tmp;

	if (!str || !*str)
		return false;

	if (strlen(str) < 2)
		return false;

	for (tmp = str; *tmp; ++tmp)
	{

		if (isdigit(*tmp) || isalpha(*tmp))
			continue;

		#ifdef ENABLE_ACCOUNT_W_SPECIALCHARS
		switch (*tmp)
		{
			case ' ':
			case '_':
			case '-':
			case '.':
			case '!':
			case '@':
			case '#':
			case '$':
			case '%':
			case '^':
			case '&':
			case '*':
			case '(':
			case ')':
				continue;
		}
		#endif

		return false;
	}

	return true;
}

bool Login_IsInChannelService(const char* c_login)
{
	if (c_login[0] == '[')
		return true;
	return false;
}

CInputAuth::CInputAuth()
{
}

void CInputAuth::Login(LPDESC d, const char * c_pData)
{
	TPacketCGLogin3 * pinfo = (TPacketCGLogin3 *) c_pData;

	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.",
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return;
	}

	#ifdef RENEWAL_HWID_BLOCK
	{
		char szQueryCheck[124];
		char szRoutherHwid[(18+1)*2 + 1];
		DBManager::instance().EscapeString(szRoutherHwid, sizeof(szRoutherHwid), pinfo->hwid_router, strlen(pinfo->hwid_router));
		snprintf(szQueryCheck, sizeof(szQueryCheck), "SELECT hwid FROM server_settings.hwid_banned WHERE hwid = '%s'", szRoutherHwid);
		auto pMsgCheck(DBManager::instance().DirectQuery(szQueryCheck));
		if (pMsgCheck->Get()->uiNumRows != 0)
		{
			LoginFailure(d, "HWID");
			return;
		}
	}

	{
		char szQueryCheckDc[124];
		char szRoutherDc[(18+1)*2 + 1];
		DBManager::instance().EscapeString(szRoutherDc, sizeof(szRoutherDc), pinfo->discord_id, strlen(pinfo->discord_id));
		snprintf(szQueryCheckDc, sizeof(szQueryCheckDc), "SELECT discordid FROM server_settings.dcid_banned WHERE discordid = '%s'", szRoutherDc);
		auto pMsgCheck2(DBManager::instance().DirectQuery(szQueryCheckDc));
		if (pMsgCheck2->Get()->uiNumRows != 0)
		{
			LoginFailure(d, "HWID");
			return;
		}
	}
	#endif

	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login, login, sizeof(login));

	char passwd[PASSWD_MAX_LEN + 1];
	strlcpy(passwd, pinfo->passwd, sizeof(passwd));

	#ifdef ENABLE_SERVER_PIN
	char passwdpin[PIN_MAX_NUM + 1];
	strlcpy(passwdpin, pinfo->pin, sizeof(passwdpin));
	#endif

	sys_log(0, "InputAuth::Login : %s(%d) desc %p", login, strlen(login), get_pointer(d));

	// check login string
	if (false == FN_IS_VALID_LOGIN_STRING(login))
	{
		sys_log(0, "InputAuth::Login : IS_NOT_VALID_LOGIN_STRING(%s) desc %p",
				login, get_pointer(d));
		LoginFailure(d, "NOID");
		return;
	}

	if (g_bNoMoreClient)
	{
		TPacketGCLoginFailure failurePacket;

		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));

		d->Packet(&failurePacket, sizeof(failurePacket));
		return;
	}

#ifdef NOA_WORK // 04.02.2023
	char szQueryStat[1024];
	snprintf(szQueryStat, sizeof(szQueryStat), "SELECT * FROM account.server_status WHERE status = 0");
	std::unique_ptr<SQLMsg> pMsgStat(DBManager::Instance().DirectQuery(szQueryStat));
	if (pMsgStat->Get()->uiNumRows > 0) {
		char szQueryAdmin[1024];
		snprintf(szQueryAdmin, sizeof(szQueryAdmin), "SELECT * FROM common.gmlist WHERE mAccount='%s'", pinfo->login);
		std::unique_ptr<SQLMsg> pMsgAdmin(DBManager::Instance().DirectQuery(szQueryAdmin));
		if (pMsgAdmin->Get()->uiNumRows == 0) {
			LoginFailure(d, "SHUTDOWN");
			return;
		}
	}
#endif

	if (DESC_MANAGER::instance().FindByLoginName(login))
	{
		LoginFailure(d, "ALREADY");
		return;
	}

	DWORD dwKey = DESC_MANAGER::instance().CreateLoginKey(d);
	DWORD dwPanamaKey = dwKey ^ pinfo->adwClientKey[0] ^ pinfo->adwClientKey[1] ^ pinfo->adwClientKey[2] ^ pinfo->adwClientKey[3];
	d->SetPanamaKey(dwPanamaKey);

	sys_log(0, "InputAuth::Login : key %u:0x%x login %s", dwKey, dwPanamaKey, login);

	TPacketCGLogin3 * p = M2_NEW TPacketCGLogin3;
	thecore_memcpy(p, pinfo, sizeof(TPacketCGLogin3));

	char szPasswd[PASSWD_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szPasswd, sizeof(szPasswd), passwd, strlen(passwd));

	char szLogin[LOGIN_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szLogin, sizeof(szLogin), login, strlen(login));

	#ifdef ENABLE_SERVER_PIN
	char szPasswdPin[PIN_MAX_NUM * 2 + 1];
	DBManager::instance().EscapeString(szPasswdPin, sizeof(szPasswdPin), passwdpin, strlen(passwdpin));
	#endif

	// CHANNEL_SERVICE_LOGIN
	if (Login_IsInChannelService(szLogin))
	{
		sys_log(0, "ChannelServiceLogin [%s]", szLogin);

		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
				"SELECT '%s',password,"
				#ifdef ENABLE_SERVER_PIN
				"'%s',pin,"
				#endif
				#ifdef ENABLE_PCBAN_SYSTEM
				"pcban,'%d',"
				#endif
				"securitycode,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'",
				szPasswd,
				#ifdef ENABLE_SERVER_PIN
				szPasswdPin,
				#endif
				#ifdef ENABLE_PCBAN_SYSTEM
				pinfo->pcban,
				#endif
				szLogin);
	}
	// END_OF_CHANNEL_SERVICE_LOGIN
	else
	{
#ifdef __WIN32__
		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
				"SELECT PASSWORD('%s'),password,"
				#ifdef ENABLE_SERVER_PIN
				"'%s',pin,"
				#endif
				#ifdef ENABLE_PCBAN_SYSTEM
				"pcban,'%d',"
				#endif
				"securitycode,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'", 
				szPasswd,
				#ifdef ENABLE_SERVER_PIN
				szPasswdPin,
				#endif
				#ifdef ENABLE_PCBAN_SYSTEM
				pinfo->pcban,
				#endif
				szLogin
			);
#else
		// @fixme138 1. PASSWORD('%s') -> %s 2. szPasswd wrapped inside mysql_hash_password(%s).c_str()
		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
				"SELECT '%s',password,"
				#ifdef ENABLE_SERVER_PIN
				"'%s',pin,"
				#endif
				#ifdef ENABLE_PCBAN_SYSTEM
				"pcban,'%d',"
				#endif
				"securitycode,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'",
				mysql_hash_password(szPasswd).c_str(),
				#ifdef ENABLE_SERVER_PIN
				szPasswdPin,
				#endif
				#ifdef ENABLE_PCBAN_SYSTEM
				pinfo->pcban,
				#endif
				szLogin);
#endif
	}
}

int CInputAuth::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.", inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return 0;
	}

	int iExtraLen = 0;

	if (test_server)
		sys_log(0, "InputAuth Analyze Header[%d] ", bHeader);

	switch (bHeader)
	{
		case HEADER_CG_PONG:
			Pong(d);
			break;

		case HEADER_CG_LOGIN3:
			Login(d, c_pData);
			break;

		case HEADER_CG_HANDSHAKE:
			break;

		default:
			sys_err("This phase does not handle this header %d (0x%x)(phase: AUTH)", bHeader, bHeader);
			d->SetPhase(PHASE_CLOSE);
			return -1;
	}

	return iExtraLen;
}
