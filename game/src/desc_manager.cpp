#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "crc32.h"
#include "desc.h"
#include "desc_p2p.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "protocol.h"
#include "messenger_manager.h"
#include "p2p.h"
#include "ip_ban.h"
#include "ClientPackageCryptInfo.h"
#include <msl/msl.h>

struct valid_ip
{
	const char *	ip;
	BYTE	network;
	BYTE	mask;
};

static struct valid_ip admin_ip[] =
{
	{ "210.123.10",     128,    128     },
	{ "\n",             0,      0       }
};

int IsValidIP(struct valid_ip* ip_table, const char *host)
{
	int         i, j;
	char        ip_addr[256];

	for (i = 0; *(ip_table + i)->ip != '\n'; ++i)
	{
		j = 255 - (ip_table + i)->mask;

		do
		{
			snprintf(ip_addr, sizeof(ip_addr), "%s.%d", (ip_table + i)->ip, (ip_table + i)->network + j);

			if (!strcmp(ip_addr, host))
				return TRUE;

			if (!j)
				break;
		}
		while (j--);
	}

	return FALSE;
}

DESC_MANAGER::DESC_MANAGER() : m_bDestroyed(false)
{
	Initialize();


	m_pPackageCrypt = new CClientPackageCryptInfo;
}

DESC_MANAGER::~DESC_MANAGER()
{
	Destroy();
	delete m_pPackageCrypt;
}

void DESC_MANAGER::Initialize()
{
	m_iSocketsConnected = 0;
	m_iHandleCount = 0;
	m_iLocalUserCount = 0;
	memset(m_aiEmpireUserCount, 0, sizeof(m_aiEmpireUserCount));
	m_bDisconnectInvalidCRC = false;
}

void DESC_MANAGER::Destroy()
{
	if (m_bDestroyed) {
		return;
	}
	m_bDestroyed = true;

#ifdef ENABLE_BAKIM_DUYURU
	event_cancel(&m_pMaintenanceEvent);
#endif


	DESC_SET::iterator i = m_set_pkDesc.begin();

	while (i != m_set_pkDesc.end())
	{
		LPDESC d = *i;
		DESC_SET::iterator ci = i;
		++i;

		if (d->GetType() == DESC_TYPE_CONNECTOR)
			continue;

		if (d->IsPhase(PHASE_P2P))
			continue;

		DestroyDesc(d, false);
		m_set_pkDesc.erase(ci);
	}

	i = m_set_pkDesc.begin();

	while (i != m_set_pkDesc.end())
	{
		LPDESC d = *i;
		DESC_SET::iterator ci = i;
		++i;

		DestroyDesc(d, false);
		m_set_pkDesc.erase(ci);
	}

	m_set_pkClientDesc.clear();

	//m_AccountIDMap.clear();
	m_map_loginName.clear();
	m_map_handle.clear();

	Initialize();
}

DWORD DESC_MANAGER::CreateHandshake()
{
	char crc_buf[8];
	crc_t crc;
	DESC_HANDSHAKE_MAP::iterator it;

RETRY:
	do
	{
		DWORD val = msl::random_int(0, 1024 * 1024);

		*(DWORD *) (crc_buf    ) = val;
		*((DWORD *) crc_buf + 1) = get_global_time();

		crc = GetCRC32(crc_buf, 8);
		it = m_map_handshake.find(crc);
	}
	while (it != m_map_handshake.end());

	if (crc == 0)
		goto RETRY;

	return (crc);
}

LPDESC DESC_MANAGER::AcceptDesc(LPFDWATCH fdw, socket_t s)
{
	socket_t                    desc;
	LPDESC						newd;
	static struct sockaddr_in   peer;
	static char					host[MAX_HOST_LENGTH + 1];

	if ((desc = socket_accept(s, &peer)) == -1)
		return NULL;

	strlcpy(host, inet_ntoa(peer.sin_addr), sizeof(host));

	if (g_bAuthServer)
	{
		if (IsBanIP(peer.sin_addr))
		{
			sys_log(0, "connection from %s was banned.", host);
			socket_close(desc);
			return NULL;
		}
	}

	if (!IsValidIP(admin_ip, host))
	{
		if (m_iSocketsConnected >= MAX_ALLOW_USER)
		{
			sys_err("max connection reached. MAX_ALLOW_USER = %d", MAX_ALLOW_USER);
			socket_close(desc);
			return NULL;
		}
	}

	newd = M2_NEW DESC;
	crc_t handshake = CreateHandshake();

	if (!newd->Setup(fdw, desc, peer, ++m_iHandleCount, handshake))
	{
		socket_close(desc);
		M2_DELETE(newd);
		return NULL;
	}

	m_map_handshake.emplace(handshake, newd);
	m_map_handle.emplace(newd->GetHandle(), newd);

	m_set_pkDesc.emplace(newd);
	++m_iSocketsConnected;
	return (newd);
}

LPDESC DESC_MANAGER::AcceptP2PDesc(LPFDWATCH fdw, socket_t bind_fd)
{
	socket_t           fd;
	struct sockaddr_in peer;
	char               host[MAX_HOST_LENGTH + 1];

	if ((fd = socket_accept(bind_fd, &peer)) == -1)
		return NULL;

	strlcpy(host, inet_ntoa(peer.sin_addr), sizeof(host));

	LPDESC_P2P pkDesc = M2_NEW DESC_P2P;

	if (!pkDesc->Setup(fdw, fd, host, peer.sin_port))
	{
		sys_err("DESC_MANAGER::AcceptP2PDesc : Setup failed");
		socket_close(fd);
		M2_DELETE(pkDesc);
		return NULL;
	}

	m_set_pkDesc.emplace(pkDesc);
	++m_iSocketsConnected;

	sys_log(0, "DESC_MANAGER::AcceptP2PDesc  %s:%u", host, peer.sin_port);
	P2P_MANAGER::instance().RegisterAcceptor(pkDesc);
	return (pkDesc);
}

void DESC_MANAGER::ConnectAccount(const std::string& login, LPDESC d)
{
	m_map_loginName.emplace(login,d);
}

void DESC_MANAGER::DisconnectAccount(const std::string& login)
{
	m_map_loginName.erase(login);
}

void DESC_MANAGER::DestroyLoginKey(const std::string_view msg) const
{
	if (msg.empty())
		return;

	TPacketDC p{};
	strlcpy(p.login, msg.data(), sizeof(p.login));
	db_clientdesc->DBPacket(HEADER_GD_DC, 0, p);
}

void DESC_MANAGER::DestroyLoginKey(const LPDESC d) const
{
	if (!d)
		return;

	DestroyLoginKey(d->GetAccountTable().login);
}

void DESC_MANAGER::DestroyDesc(LPDESC d, bool bEraseFromSet)
{
	if (bEraseFromSet)
		m_set_pkDesc.erase(d);

	if (d->GetHandshake())
		m_map_handshake.erase(d->GetHandshake());

	if (d->GetHandle() != 0)
		m_map_handle.erase(d->GetHandle());
	else
		m_set_pkClientDesc.erase((LPCLIENT_DESC) d);

	// Explicit call to the virtual function Destroy()
	d->Destroy();

	M2_DELETE(d);
	--m_iSocketsConnected;
}

void DESC_MANAGER::DestroyClosed()
{
	DESC_SET::iterator i = m_set_pkDesc.begin();

	while (i != m_set_pkDesc.end())
	{
		LPDESC d = *i;
		DESC_SET::iterator ci = i;
		++i;

		if (d->IsPhase(PHASE_CLOSE))
		{
			if (d->GetType() == DESC_TYPE_CONNECTOR)
			{
				LPCLIENT_DESC client_desc = (LPCLIENT_DESC)d;

				if (client_desc->IsRetryWhenClosed())
				{
					client_desc->Reset();
					continue;
				}
			}

			DestroyDesc(d, false);
			m_set_pkDesc.erase(ci);
		}
	}
}

LPDESC DESC_MANAGER::FindByLoginName(const std::string& login)
{
	DESC_LOGINNAME_MAP::iterator it = m_map_loginName.find(login);

	if (m_map_loginName.end() == it)
		return NULL;

	return (it->second);
}

LPDESC DESC_MANAGER::FindByHandle(DWORD handle)
{
	DESC_HANDLE_MAP::iterator it = m_map_handle.find(handle);

	if (m_map_handle.end() == it)
		return NULL;

	return (it->second);
}

const DESC_MANAGER::DESC_SET & DESC_MANAGER::GetClientSet()
{
	return m_set_pkDesc;
}

struct name_with_desc_func
{
	const char * m_name;

	name_with_desc_func(const char * name) : m_name(name)
	{
	}

	bool operator () (LPDESC d)
	{
		if (d->GetCharacter() && !strcmp(d->GetCharacter()->GetName(), m_name))
			return true;

		return false;
	}
};

LPDESC DESC_MANAGER::FindByCharacterName(const char *name)
{
	DESC_SET::iterator it = std::find_if (m_set_pkDesc.begin(), m_set_pkDesc.end(), name_with_desc_func(name));
	return (it == m_set_pkDesc.end()) ? NULL : (*it);
}

LPCLIENT_DESC DESC_MANAGER::CreateConnectionDesc(LPFDWATCH fdw, const char * host, WORD port, int iPhaseWhenSucceed, bool bRetryWhenClosed)
{
	LPCLIENT_DESC newd;

	newd = M2_NEW CLIENT_DESC;

	newd->Setup(fdw, host, port);
	newd->Connect(iPhaseWhenSucceed);
	newd->SetRetryWhenClosed(bRetryWhenClosed);

	m_set_pkDesc.emplace(newd);
	m_set_pkClientDesc.emplace(newd);

	++m_iSocketsConnected;
	return (newd);
}

struct FuncTryConnect
{
	void operator () (LPDESC d)
	{
		((LPCLIENT_DESC)d)->Connect();
	}
};

void DESC_MANAGER::TryConnect()
{
	FuncTryConnect f;
	std::for_each(m_set_pkClientDesc.begin(), m_set_pkClientDesc.end(), f);
}

bool DESC_MANAGER::IsP2PDescExist(const char * szHost, WORD wPort)
{
	CLIENT_DESC_SET::iterator it = m_set_pkClientDesc.begin();

	while (it != m_set_pkClientDesc.end())
	{
		LPCLIENT_DESC d = *(it++);

		if (!strcmp(d->GetP2PHost(), szHost) && d->GetP2PPort() == wPort)
			return true;
	}

	return false;
}

LPDESC DESC_MANAGER::FindByHandshake(DWORD dwHandshake)
{
	DESC_HANDSHAKE_MAP::iterator it = m_map_handshake.find(dwHandshake);

	if (it == m_map_handshake.end())
		return NULL;

	return (it->second);
}

class FuncWho
{
	public:
		int iTotalCount;
		int aiEmpireUserCount[EMPIRE_MAX_NUM];

		FuncWho()
		{
			iTotalCount = 0;
			memset(aiEmpireUserCount, 0, sizeof(aiEmpireUserCount));
		}

		void operator() (LPDESC d)
		{
			if (d->GetCharacter())
			{
				++iTotalCount;
				++aiEmpireUserCount[d->GetEmpire()];
			}
		}
};

void DESC_MANAGER::UpdateLocalUserCount()
{
	const DESC_SET & c_ref_set = GetClientSet();
	FuncWho f;
	f = std::for_each(c_ref_set.begin(), c_ref_set.end(), f);

	m_iLocalUserCount = f.iTotalCount;
	thecore_memcpy(m_aiEmpireUserCount, f.aiEmpireUserCount, sizeof(m_aiEmpireUserCount));

	m_aiEmpireUserCount[1] += P2P_MANAGER::instance().GetEmpireUserCount(1);
	m_aiEmpireUserCount[2] += P2P_MANAGER::instance().GetEmpireUserCount(2);
	m_aiEmpireUserCount[3] += P2P_MANAGER::instance().GetEmpireUserCount(3);
}

void DESC_MANAGER::GetUserCount(int & iTotal, int ** paiEmpireUserCount, int & iLocalCount)
{
	*paiEmpireUserCount = &m_aiEmpireUserCount[0];

	int iCount = P2P_MANAGER::instance().GetCount();
	if (iCount < 0)
	{
		sys_err("P2P_MANAGER::instance().GetCount() == -1");
	}

	iTotal = m_iLocalUserCount + iCount;
	iLocalCount = m_iLocalUserCount;
}

DWORD DESC_MANAGER::MakeRandomKey(DWORD dwHandle)
{
	DWORD random_key = msl::random_int<uint32_t>();
	m_map_handle_random_key.emplace(dwHandle, random_key);
	return random_key;
}

bool DESC_MANAGER::GetRandomKey(DWORD dwHandle, DWORD* prandom_key)
{
	DESC_HANDLE_RANDOM_KEY_MAP::iterator it = m_map_handle_random_key.find(dwHandle);

	if (it == m_map_handle_random_key.end())
		return false;

	*prandom_key = it->second;
	return true;
}

LPDESC DESC_MANAGER::FindByLoginKey(DWORD dwKey)
{
	std::map<DWORD, CLoginKey *>::iterator it = m_map_pkLoginKey.find(dwKey);

	if (it == m_map_pkLoginKey.end())
		return NULL;

	return it->second->m_pkDesc;
}


DWORD DESC_MANAGER::CreateLoginKey(LPDESC d)
{
	DWORD dwKey = 0;

	do
	{
		dwKey = number(1, INT_MAX);

		if (m_map_pkLoginKey.find(dwKey) != m_map_pkLoginKey.end())
			continue;

		CLoginKey * pkKey = M2_NEW CLoginKey(dwKey, d);
		d->SetLoginKey(pkKey);
		m_map_pkLoginKey.emplace(dwKey, pkKey);
		break;
	} while (1);

	return dwKey;
}

void DESC_MANAGER::ProcessExpiredLoginKey()
{
	DWORD dwCurrentTime = get_dword_time();

	std::map<DWORD, CLoginKey *>::iterator it, it2;

	it = m_map_pkLoginKey.begin();

	while (it != m_map_pkLoginKey.end())
	{
		it2 = it++;

		if (it2->second->m_dwExpireTime == 0)
			continue;

		if (dwCurrentTime - it2->second->m_dwExpireTime > 60000)
		{
			M2_DELETE(it2->second);
			m_map_pkLoginKey.erase(it2);
		}
	}
}

bool DESC_MANAGER::LoadClientPackageCryptInfo(const char* pDirName)
{
	return m_pPackageCrypt->LoadPackageCryptInfo(pDirName);
}
#ifdef __FreeBSD__
void DESC_MANAGER::NotifyClientPackageFileChanged( const std::string& dirName, eFileUpdatedOptions eUpdateOption )
{
	 Instance().LoadClientPackageCryptInfo(dirName.c_str());
}
#endif


void DESC_MANAGER::SendClientPackageCryptKey( LPDESC desc )
{
	if( !desc )
	{
		return;
	}

	TPacketGCHybridCryptKeys packet;
	{
		packet.bHeader = HEADER_GC_HYBRIDCRYPT_KEYS;
		m_pPackageCrypt->GetPackageCryptKeys( &(packet.pDataKeyStream), packet.KeyStreamLen );
	}

	if( packet.KeyStreamLen > 0 )
	{
		if (test_server)
		{

			sys_log(0, "[PackageCryptInfo] send to %s. (keys: %s, len: %d)", desc->GetAccountTable().login, std::string((char*)packet.pDataKeyStream).c_str(), packet.KeyStreamLen);
		}
		desc->Packet( packet.GetStreamData(), packet.GetStreamSize() );
	}
}

void DESC_MANAGER::SendClientPackageSDBToLoadMap( LPDESC desc, const char* pMapName )
{
	if( !desc )
	{
		return;
	}

	TPacketGCPackageSDB packet;
	{
		packet.bHeader      = HEADER_GC_HYBRIDCRYPT_SDB;
		if( !m_pPackageCrypt->GetRelatedMapSDBStreams( pMapName, &(packet.m_pDataSDBStream), packet.iStreamLen ) )
			return;
		if (test_server)
			sys_log(0, "[PackageCryptInfo] send to %s from map %s. (SDB len: %d)", desc->GetAccountTable().login, pMapName, packet.iStreamLen);
	}

	if( packet.iStreamLen > 0 )
	{
		desc->Packet( packet.GetStreamData(), packet.GetStreamSize());
	}
}


#ifdef ENABLE_BAKIM_DUYURU
EVENTFUNC(maintenance_event)
{
	DESC_MANAGER::Instance().ClearMaintenanceInfo();
	Shutdown(10);
	return 0;
}

bool DESC_MANAGER::BuildClientMaintenance(TEMP_BUFFER& buf)
{
	if (!m_pMaintenanceInfo)
	{
		return false;
	}

	DWORD remaining_time = 0;

	if (m_pMaintenanceInfo->shutdown_time > 0)
	{
		// remaining_time = m_pMaintenanceInfo->shutdown_time;
		remaining_time = m_pMaintenanceInfo->end_time - get_global_time();
	}

	TPacketGCMaintenanceInfo pack;
	pack.header = HEADER_GC_ENABLE_BAKIM_DUYURU;
	pack.remaining_time = remaining_time;
	pack.duration = m_pMaintenanceInfo->duration;
	pack.cause_len = strlen(m_pMaintenanceInfo->cause) + 1;
	pack.size = sizeof(pack) + pack.cause_len;
	buf.write(&pack, sizeof(pack));
	buf.write(m_pMaintenanceInfo->cause, pack.cause_len);
	return true;
}

void DESC_MANAGER::SetMaintenanceInfo(const TServerMaintenanceInfo& info)
{
	event_cancel(&m_pMaintenanceEvent);

	if (!m_pMaintenanceInfo)
	{
		m_pMaintenanceInfo = std::unique_ptr<TServerMaintenanceInfo>(new TServerMaintenanceInfo());
	}

	thecore_memcpy(m_pMaintenanceInfo.get(), &info, sizeof(TServerMaintenanceInfo));

	DWORD remaining_time = 0;

	if (info.shutdown_time > 0)
	{
		remaining_time = info.shutdown_time;
		m_pMaintenanceEvent = event_create(maintenance_event, AllocEventInfo<event_info_data>(), PASSES_PER_SEC(remaining_time));
	}

	TEMP_BUFFER buf;
	BuildClientMaintenance(buf);

	for (auto desc : m_set_pkDesc)
	{
		if (desc->IsPhase(PHASE_GAME) || desc->IsPhase(PHASE_DEAD))
		{
			desc->Packet(buf.read_peek(), buf.size());
		}
	}
}

void DESC_MANAGER::SendMaintenanceInfo(LPDESC desc)
{
	TEMP_BUFFER buf;
	if (!BuildClientMaintenance(buf))
	{
		return;
	}
	desc->Packet(buf.read_peek(), buf.size());
}

void DESC_MANAGER::ClearMaintenanceInfo()
{
	SetMaintenanceInfo(TServerMaintenanceInfo());
}
#endif

#ifdef ENABLE_MAINTENANCE_GM_LOGIN
bool DESC_MANAGER::MaintenanceGmLoginStatus(const char * login)
{
	if (login == NULL)
	{
		sys_log(0, "Null geliyor!");
		return false;
	}

	char szLogin[LOGIN_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szLogin, sizeof(szLogin), login, strlen(login));

	auto pMsg(DBManager::instance().DirectQuery("SELECT mAccount FROM common.gmlist;"));
	if (!pMsg->uiSQLErrno && pMsg->Get()->uiNumRows)
	{
		while (MYSQL_ROW row_login = mysql_fetch_row(pMsg->Get()->pSQLResult))
		{
			if (!strcmp(row_login[0], szLogin))
			{
				return true;
			}
		}
	}

	return false;
}
#endif
