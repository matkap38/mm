#include "stdafx.h"
#include <sstream>

#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "buffer_manager.h"
#include "config.h"
#include "profiler.h"
#include "p2p.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "login_sim.h"
#include "fishing.h"
#include "priv_manager.h"
#include "castle.h"

#ifndef __WIN32__
	#include "limit_time.h"
#endif

#include "../../common/CommonDefines.h"
#include "../../common/PulseManager.h"

#include "utils.h"

bool IsEmptyAdminPage()
{
	return g_stAdminPageIP.empty();
}

bool IsAdminPage(const char * ip)
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
	{
		if (g_stAdminPageIP[n] == ip)
			return 1;
	}
	return 0;
}

void ClearAdminPages()
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
		g_stAdminPageIP[n].clear();

	g_stAdminPageIP.clear();
}

CInputProcessor::CInputProcessor() : m_pPacketInfo(NULL), m_iBufferLeft(0)
{
	if (!m_pPacketInfo)
		BindPacketInfo(&m_packetInfoCG);
}

void CInputProcessor::BindPacketInfo(CPacketInfo * pPacketInfo)
{
	m_pPacketInfo = pPacketInfo;
}

bool CInputProcessor::Process(LPDESC lpDesc, const void * c_pvOrig, int iBytes, int & r_iBytesProceed)
{
	const char * c_pData = (const char *) c_pvOrig;

	BYTE	bLastHeader = 0;
	int		iLastPacketLen = 0;
	int		iPacketLen;

	if (!m_pPacketInfo)
	{
		sys_err("No packet info has been binded to");
		return true;
	}

	for (m_iBufferLeft = iBytes; m_iBufferLeft > 0;)
	{
		BYTE bHeader = (BYTE) *(c_pData);
		const char * c_pszName;

		if (bHeader == 0)
			iPacketLen = 1;
		else if (!m_pPacketInfo->Get(bHeader, &iPacketLen, &c_pszName))
		{
			sys_err("UNKNOWN HEADER: %d, LAST HEADER: %d(%d), REMAIN BYTES: %d, fd: %d",
					bHeader, bLastHeader, iLastPacketLen, m_iBufferLeft, lpDesc->GetSocket());
			//printdata((BYTE *) c_pvOrig, m_iBufferLeft);
			lpDesc->SetPhase(PHASE_CLOSE);
			return true;
		}

		if (m_iBufferLeft < iPacketLen)
			return true;

		if (bHeader)
		{
			if (test_server && bHeader != HEADER_CG_MOVE)
				sys_log(0, "Packet Analyze [Header %d][bufferLeft %d] ", bHeader, m_iBufferLeft);

			m_pPacketInfo->Start();

			int iExtraPacketSize = Analyze(lpDesc, bHeader, c_pData);
			if (iExtraPacketSize < 0)
				return true;

			iPacketLen += iExtraPacketSize;
			lpDesc->Log("%s %d", c_pszName, iPacketLen);
			m_pPacketInfo->End();
		}

		#ifdef ENABLE_SEQUENCE_SYSTEM
		if (bHeader == HEADER_CG_PONG)
			sys_log(0, "PONG! %u %u", m_pPacketInfo->IsSequence(bHeader), *(BYTE *) (c_pData + iPacketLen - sizeof(BYTE)));

		if (m_pPacketInfo->IsSequence(bHeader))
		{
			BYTE bSeq = lpDesc->GetSequence();
			BYTE bSeqReceived = *(BYTE *) (c_pData + iPacketLen - sizeof(BYTE));

			if (bSeq != bSeqReceived)
			{
				sys_err("SEQUENCE %x mismatch 0x%x != 0x%x header %u", get_pointer(lpDesc), bSeq, bSeqReceived, bHeader);

				LPCHARACTER ch = lpDesc->GetCharacter();

				char buf[1024];
				int offset, len;

				offset = snprintf(buf, sizeof(buf), "SEQUENCE_LOG [%s]-------------\n", ch ? ch->GetName() : "UNKNOWN");

				if (offset < 0 || offset >= (int) sizeof(buf))
					offset = sizeof(buf) - 1;

				for (size_t i = 0; i < lpDesc->m_seq_vector.size(); ++i)
				{
					len = snprintf(buf + offset, sizeof(buf) - offset, "\t[%03d : 0x%x]\n",
							lpDesc->m_seq_vector[i].hdr,
							lpDesc->m_seq_vector[i].seq);

					if (len < 0 || len >= (int) sizeof(buf) - offset)
						offset += (sizeof(buf) - offset) - 1;
					else
						offset += len;
				}

				snprintf(buf + offset, sizeof(buf) - offset, "\t[%03d : 0x%x]\n", bHeader, bSeq);
				sys_err("%s", buf);

				lpDesc->SetPhase(PHASE_CLOSE);
				return true;
			}
			else
			{
				lpDesc->push_seq(bHeader, bSeq);
				lpDesc->SetNextSequence();
				//sys_err("SEQUENCE %x match %u next %u header %u", lpDesc, bSeq, lpDesc->GetSequence(), bHeader);
			}
		}
		#endif

		c_pData += iPacketLen;
		m_iBufferLeft -= iPacketLen;
		r_iBytesProceed += iPacketLen;

		iLastPacketLen = iPacketLen;
		bLastHeader = bHeader;

		if (GetType() != lpDesc->GetInputProcessor()->GetType())
			return false;
	}

	return true;
}

void CInputProcessor::Pong(LPDESC d)
{
	d->SetPong(true);
}

void CInputProcessor::Handshake(LPDESC d, const char * c_pData)
{
	TPacketCGHandshake * p = (TPacketCGHandshake *) c_pData;

	if (d->GetHandshake() != p->dwHandshake)
	{
		sys_err("Invalid Handshake on %d", d->GetSocket());
		d->SetPhase(PHASE_CLOSE);
	}
	else
	{
		if (d->IsPhase(PHASE_HANDSHAKE))
		{
			if (d->HandshakeProcess(p->dwTime, p->lDelta, false))
			{
				#ifdef _IMPROVED_PACKET_ENCRYPTION_
				d->SendKeyAgreement();
				#else
				if (g_bAuthServer)
					d->SetPhase(PHASE_AUTH);
				else
					d->SetPhase(PHASE_LOGIN);
				#endif // #ifdef _IMPROVED_PACKET_ENCRYPTION_
			}
		}
		else
			d->HandshakeProcess(p->dwTime, p->lDelta, true);
	}
}

void CInputProcessor::Version(LPCHARACTER ch, const char* c_pData)
{
	if (!ch)
		return;

	TPacketCGClientVersion * p = (TPacketCGClientVersion *) c_pData;
	sys_log(0, "VERSION: %s %s %s", ch->GetName(), p->timestamp, p->filename);
	ch->GetDesc()->SetClientVersion(p->timestamp);
}

#ifdef ENABLE_PM_ALL_SEND_SYSTEM
struct SWhisperPacketFunc
{
	const char * c_pszText;

	SWhisperPacketFunc(const char * text) :
		c_pszText(text)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d || !d->GetCharacter())
			return;

		struct packet_bulk_whisper bulk_whisper_pack;
		bulk_whisper_pack.header = HEADER_GC_BULK_WHISPER;
		bulk_whisper_pack.size = sizeof(struct packet_bulk_whisper) + strlen(c_pszText);

		TEMP_BUFFER buf;
		buf.write(&bulk_whisper_pack, sizeof(struct packet_bulk_whisper));
		buf.write(c_pszText, strlen(c_pszText));

		d->Packet(buf.read_peek(), buf.size());
	}
};

void CInputProcessor::SendBulkWhisper(const char * c_pszText)
{
	const DESC_MANAGER::DESC_SET & f = DESC_MANAGER::instance().GetClientSet();
	std::for_each(f.begin(), f.end(), SWhisperPacketFunc(c_pszText));
}
#endif

void LoginFailure(LPDESC d, const char * c_pszStatus)
{
	if (!d)
		return;

	TPacketGCLoginFailure failurePacket;

	failurePacket.header = HEADER_GC_LOGIN_FAILURE;
	strlcpy(failurePacket.szStatus, c_pszStatus, sizeof(failurePacket.szStatus));

	d->Packet(&failurePacket, sizeof(failurePacket));
}

CInputHandshake::CInputHandshake()
{
	CPacketInfoCG * pkPacketInfo = M2_NEW CPacketInfoCG;
	#ifdef ENABLE_SEQUENCE_SYSTEM
	pkPacketInfo->SetSequence(HEADER_CG_PONG, false);
	#endif
	m_pMainPacketInfo = m_pPacketInfo;
	BindPacketInfo(pkPacketInfo);
}

CInputHandshake::~CInputHandshake()
{
	if( NULL != m_pPacketInfo )
	{
		M2_DELETE(m_pPacketInfo);
		m_pPacketInfo = NULL;
	}
}

std::map<DWORD, CLoginSim *> g_sim;
std::map<DWORD, CLoginSim *> g_simByPID;
std::vector<TPlayerTable> g_vec_save;

// BLOCK_CHAT
ACMD(do_block_chat);
// END_OF_BLOCK_CHAT

int CInputHandshake::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	if (bHeader == 10)
		return 0;

	if (bHeader == HEADER_CG_TEXT)
	{
		#ifdef ENABLE_PORT_SECURITY
		if (IsEmptyAdminPage() || !IsAdminPage(inet_ntoa(d->GetAddr().sin_addr))) // block if adminpage is not set or if not admin
		{
			sys_log(0, "SOCKET_CMD: BLOCK FROM(%s)", d->GetHostName());
			d->SetPhase(PHASE_CLOSE);
			return -1;
		}
		#endif

		++c_pData;
		const char * c_pSep;

		if (!(c_pSep = strchr(c_pData, '\n')))
		{
			d->SetPhase(PHASE_CLOSE); // @fixme187 setphase+ret0
			return 0;
		}

		if (*(c_pSep - 1) == '\r')
			--c_pSep;

		std::string stResult;
		std::string stBuf;
		stBuf.assign(c_pData, 0, c_pSep - c_pData);

		sys_log(0, "SOCKET_CMD: FROM(%s) CMD(%s)", d->GetHostName(), stBuf.c_str());

		sys_log(1, "TEXT %s RESULT %s", stBuf.c_str(), stResult.c_str());
		stResult += "\n";
		d->Packet(stResult.c_str(), stResult.length());
		return (c_pSep - c_pData) + 1;
	}
	else if (bHeader == HEADER_CG_MARK_LOGIN)
	{
		if (!guild_mark_server)
		{
			sys_err("Guild Mark login requested but i'm not a mark server!");
			d->SetPhase(PHASE_CLOSE);
			return 0;
		}

		sys_log(0, "MARK_SERVER: Login");
		d->SetPhase(PHASE_LOGIN);
		return 0;
	}
	else if (bHeader == HEADER_CG_STATE_CHECKER)
	{
		if (d->isChannelStatusRequested())
		{
			return 0;
		}

		d->SetChannelStatusRequested(true);
		db_clientdesc->DBPacket(HEADER_GD_REQUEST_CHANNELSTATUS, d->GetHandle(), NULL, 0);
	}
	else if (bHeader == HEADER_CG_PONG)
		Pong(d);
	else if (bHeader == HEADER_CG_HANDSHAKE)
		Handshake(d, c_pData);
	#ifdef _IMPROVED_PACKET_ENCRYPTION_
	else if (bHeader == HEADER_CG_KEY_AGREEMENT)
	{
		// Send out the key agreement completion packet first
		// to help client to enter encryption mode
		d->SendKeyAgreementCompleted();
		// Flush socket output before going encrypted
		d->ProcessOutput();

		TPacketKeyAgreement* p = (TPacketKeyAgreement*)c_pData;
		if (!d->IsCipherPrepared())
		{
			sys_err ("Cipher isn't prepared. %s maybe a Hacker.", inet_ntoa(d->GetAddr().sin_addr));
			d->DelayedDisconnect(5);
			return 0;
		}

		if (d->FinishHandshake(p->wAgreedLength, p->data, p->wDataLength))
		{
			// Handshaking succeeded
			if (g_bAuthServer)
			{
				d->SetPhase(PHASE_AUTH);
			}
			else
			{
				d->SetPhase(PHASE_LOGIN);
			}
		}
		else
		{
			sys_log(0, "[CInputHandshake] Key agreement failed: al=%u dl=%u", p->wAgreedLength, p->wDataLength);
			d->SetPhase(PHASE_CLOSE);
		}
	}
	#endif // _IMPROVED_PACKET_ENCRYPTION_
	else
	{
		sys_err("Handshake phase does not handle packet %d (fd %d) from %s:%u", bHeader, d->GetSocket(), d->GetHostName(), d->GetPort()); //@warme016 host and port
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	return 0;
}

