#include "stdafx.h"

#include "urielacsdk.h"

#include <cryptopp/hmac.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha.h>

#include <ctime>

#include "char.h"
#include "config.h"
#include "desc.h"
#include "event.h"
#include "log.h"
#include "packet.h"

#define URIEL_HEARTBEAT_CYCLE PASSES_PER_SEC(120)

using namespace CryptoPP;

CAntiCheat::CAntiCheat()
{}

CAntiCheat::~CAntiCheat()
{}

bool
CAntiCheat::VerifyLoginToken(std::string username, char* token_buff)
{
	unsigned char hmac_key[32] = {
	  0x8D, 0x25, 0x3D, 0xED, 0x00, 0x1E, 0x73, 0x1D, 0x31, 0xFB, 0xCF,
	  0x77, 0x49, 0xDA, 0x6D, 0x75, 0x5E, 0xDD, 0xD4, 0x38, 0xD9, 0xAF,
	  0x3F, 0xC5, 0xF9, 0xE2, 0x7A, 0x97, 0xD2, 0x5D, 0x2E, 0x13
	};

	HMAC<SHA256> sha(hmac_key, sizeof(hmac_key));
	sha.Update((byte*)username.data(), username.length());

	return sha.VerifyDigest((byte*)&token_buff[4], (byte*)token_buff, 4);
}

EVENTINFO(URIEL_HEARTBEAT_INFO)
{
	DynamicCharacterPtr ptrPC;
};

EVENTFUNC(URIEL_HEARTBEAT_EVENT)
{
	URIEL_HEARTBEAT_INFO* info = dynamic_cast<URIEL_HEARTBEAT_INFO*>(event->info);
	if (info == nullptr)
	{
		sys_err("<URIEL_HEARTBEAT_EVENT> info null pointer");
		return 0;
	}

	LPCHARACTER lpCharacter = info->ptrPC.Get();
	if (!lpCharacter)
	{
		sys_err("<URIEL_HEARTBEAT_EVENT> lpCharacter is invalid");
		return 0;
	}

	//lpCharacter->ChatPacket(CHAT_TYPE_INFO, "HEADER_GC_URIEL_HEARTBEAT 1");

	LPDESC lpClientDesc = lpCharacter->GetDesc();
	if (!lpClientDesc)
	{
		sys_err("<URIEL_HEARTBEAT_EVENT> client session is invalid");
		return 0;
	}

	//lpCharacter->ChatPacket(CHAT_TYPE_INFO, "HEADER_GC_URIEL_HEARTBEAT 2");
	if (lpCharacter->uriel_WaitForResponse)
	{
		DWORD dwTimer = time(0) - lpCharacter->uriel_LastVerifiedTime;
		if (dwTimer > (120 * 2))
		{
			lpCharacter->Disconnect("URIEL_HEARTBEAT_EXPIRED");
			lpClientDesc->SetPhase(PHASE_CLOSE);
			return 0;
		}

		return URIEL_HEARTBEAT_CYCLE;
	}

	//lpCharacter->ChatPacket(CHAT_TYPE_INFO, "HEADER_GC_URIEL_HEARTBEAT 3");

	AutoSeededRandomPool rng;
	rng.GenerateBlock((byte*)lpCharacter->uriel_HeartBeatData,
					  sizeof(lpCharacter->uriel_HeartBeatData));

	TPacketUrielHeartBeat packet{};

	packet.bHeader = HEADER_GC_URIEL_HEARTBEAT;
	memcpy(packet.bPacketData,
		   lpCharacter->uriel_HeartBeatData,
		   sizeof(lpCharacter->uriel_HeartBeatData));

	lpClientDesc->Packet(&packet, sizeof(packet));

	lpCharacter->uriel_WaitForResponse = true;

	return URIEL_HEARTBEAT_CYCLE;
}

void
CAntiCheat::CreateSession(LPCHARACTER lpCharacter)
{
	if (lpCharacter == nullptr)
		return;

	URIEL_HEARTBEAT_INFO* event_info = AllocEventInfo<URIEL_HEARTBEAT_INFO>();
	event_info->ptrPC = lpCharacter;

	lpCharacter->uriel_LastVerifiedTime = time(0);
	lpCharacter->uriel_WaitForResponse = false;
	lpCharacter->uriel_lpEvent = event_create(
		URIEL_HEARTBEAT_EVENT, event_info, URIEL_HEARTBEAT_CYCLE);
	//lpCharacter->ChatPacket(CHAT_TYPE_INFO, "HEADER_GC_URIEL_HEARTBEAT start");
}

void
CAntiCheat::DestroySession(LPCHARACTER lpCharacter)
{
	if (lpCharacter == nullptr)
		return;

	if (lpCharacter->uriel_lpEvent == nullptr)
		return;

	event_cancel(&lpCharacter->uriel_lpEvent);
	lpCharacter->uriel_lpEvent = nullptr;
}

void
CAntiCheat::VerifyHeartbeatResponse(LPCHARACTER lpCharacter, const char* data)
{
	if (lpCharacter == nullptr)
		return;

	TPacketUrielHeartBeatResponse* packet = (TPacketUrielHeartBeatResponse*)data;

	unsigned char hmac_key[32] = {
	  0xB7, 0xC9, 0x50, 0x7E, 0xD4, 0xA2, 0x04, 0x28, 0xC5, 0x52, 0xEE,
	  0x2F, 0xEE, 0xD6, 0x01, 0x11, 0x85, 0x3C, 0x65, 0x45, 0x46, 0xEE,
	  0x36, 0x43, 0x5F, 0x7A, 0xD7, 0xF9, 0x5E, 0xBC, 0x5F, 0x0A
	};

	HMAC<SHA256> sha(hmac_key, sizeof(hmac_key));
	if (!sha.VerifyDigest((byte*)packet->bPacketData,
						  (byte*)lpCharacter->uriel_HeartBeatData,
						  sizeof(lpCharacter->uriel_HeartBeatData)))
		return;

	lpCharacter->uriel_LastVerifiedTime = time(0);
	lpCharacter->uriel_WaitForResponse = false;
	memset(lpCharacter->uriel_HeartBeatData,
		   0,
		   sizeof(lpCharacter->uriel_HeartBeatData));
}

bool
CAntiCheat::VerifyAttackOrder(DWORD dwVID,
							  DWORD dwOrder,
							  unsigned int attackSpeed,
							  DWORD dwRandom,
							  unsigned char* verifyHash)
{
	const unsigned char hmac_key[20] = { 0x90, 0x96, 0x3E, 0x92, 0x97, 0x5E, 0x9E,
										 0x21, 0x3C, 0xBB, 0xDB, 0xC8, 0xD4, 0xF4,
										 0x88, 0x66, 0x51, 0x10, 0x51, 0xFD };

	HMAC<SHA1> sha(hmac_key, sizeof(hmac_key));
	sha.Update((byte*)&dwVID, sizeof(dwVID));
	sha.Update((byte*)&dwOrder, sizeof(dwOrder));
	sha.Update((byte*)&attackSpeed, sizeof(attackSpeed));

	return sha.VerifyDigest(
		(byte*)verifyHash, (byte*)&dwRandom, sizeof(dwRandom));
}
