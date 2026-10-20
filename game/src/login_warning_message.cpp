#include "stdafx.h"
#include "constants.h"
#include "char.h"
#include "cmd.h"
#include "utils.h"
#include "db.h"
#include "config.h"
#include "packet.h"
#include "p2p.h"
#include "questmanager.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char_manager.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "sectree_manager.h"
#include "login_warning_message.h"

#ifdef ENABLE_LOGIN_WARNING_MESSAGE_SYSTEM
CLoginWarningMsg::CLoginWarningMsg() : m_bActiveStatus(false) {}
CLoginWarningMsg::~CLoginWarningMsg() { Destroy(); }

void CLoginWarningMsg::Destroy()
{
	m_MessageListVec.clear();
}

bool CLoginWarningMsg::Initialize()
{
	sys_log(0, "Initialize: Loading.");

	Destroy();

	char szQuery[8192];
	snprintf(szQuery, sizeof(szQuery), "%s", "SELECT id, message FROM server_settings.login_warning_message ORDER BY id;");
	auto pkMsg = DBManager::instance().DirectQuery(szQuery);
	if (!pkMsg)
	{
		sys_err("Initialize::szQuery:!pkMsg");
		return false;
	}

	SQLResult * pRes = pkMsg->Get();
	if (pRes->uiNumRows != 0)
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL)
		{
			int con = 0;

			TMessageInfoTable tmp;
			memset(&tmp, 0, sizeof(tmp));
			tmp.id = atoi(row[con++]);
			strlcpy(tmp.message, row[con++], sizeof(tmp.message));
			m_MessageListVec.emplace_back(tmp);
		}
	}

	return true;
}

void CLoginWarningMsg::Reload()
{
	m_bActiveStatus = true;
	if (Initialize()) { m_bActiveStatus = false; }
}

void CLoginWarningMsg::Login(LPCHARACTER ch)
{
	if (m_bActiveStatus) { return; }
	if (!ch && !ch->GetDesc()) { return; }
	if (m_MessageListVec.empty()) { return; }

	int iSize = m_MessageListVec.size();
	int iChance = number(1, iSize);
	const auto& table = m_MessageListVec[iChance-1];
	SendPacket(ch, table.message);
}

void CLoginWarningMsg::SendPacket(LPCHARACTER ch, const char * szText)
{
	if (m_bActiveStatus) return;
	if (!ch && !ch->GetDesc()) return;

	char msg[CHAT_MAX_LEN + 1];
	snprintf(msg, sizeof(msg), "%s", szText);
	int len = MIN(CHAT_MAX_LEN, strlen(msg) + 1);

	TPacketGCWhisper pack;
	pack.bHeader = HEADER_GC_WHISPER;
	pack.wSize = sizeof(TPacketGCWhisper) + len;
	pack.bType = WHISPER_TYPE_NORMAL;
	strlcpy(pack.szNameFrom, "[DUYURU]", sizeof(pack.szNameFrom));

	TEMP_BUFFER buf;
	buf.write(&pack, sizeof(TPacketGCWhisper));
	buf.write(msg, len);
	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}
#endif
