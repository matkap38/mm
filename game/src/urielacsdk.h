#pragma once

#include "char.h"

#pragma pack(push, 1)
typedef struct PacketUrielHeartBeat
{
	BYTE	bHeader;
	BYTE	bPacketData[64];

} TPacketUrielHeartBeat;

typedef struct PacketUrielHeartBeatResponse
{
	BYTE	bHeader;
	BYTE	bPacketData[32];
} TPacketUrielHeartBeatResponse;
#pragma pack(pop)

class CAntiCheat : public singleton<CAntiCheat>
{
public:
	CAntiCheat();
	virtual ~CAntiCheat();

	bool VerifyLoginToken(std::string username, char* token_buff);
	void CreateSession(LPCHARACTER lpCharacter);
	void DestroySession(LPCHARACTER lpCharacter);

	void VerifyHeartbeatResponse(LPCHARACTER lpCharacter, const char* data);

	bool VerifyAttackOrder(DWORD dwVID, DWORD dwOrder, unsigned int attackSpeed, DWORD dwRandom, unsigned char* verifyHash);
};
