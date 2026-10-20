#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "horse_rider.h"
#include "config.h"
#include "char_manager.h"

const int HORSE_HEALTH_DROP_INTERVAL = 3 * 24 * 60 * 60;
const int HORSE_STAMINA_CONSUME_INTERVAL = 6 * 60;
const int HORSE_STAMINA_REGEN_INTERVAL = 12 * 60;
//const int HORSE_HP_DROP_INTERVAL = 60;
//const int HORSE_STAMINA_CONSUME_INTERVAL = 3;
//const int HORSE_STAMINA_REGEN_INTERVAL = 6;

//iMinLevel	iNPCRace	iMaxHealth	iMaxStamina	iST	iDX	iHT	iIQ	iDamMean	iDamMin	iDamMax	iArmor
THorseStat c_aHorseStat[HORSE_MAX_LEVEL+1] =
{
	{  0,	0,	1,	1,	0,	0,	0,	0,	0,	0,	0,	0  },
	{ 25,	20104,	3,	4,	9,	45,	18,	9,	54,	43,	64,	32 },//20101
	{ 25,	20104,	4,	4,	10,	46,	18,	9,	55,	44,	66,	33 },//20101
	{ 25,	20104,	5,	5,	11,	48,	19,	9,	56,	44,	67,	33 },//20101
	{ 25,	20104,	7,	5,	12,	49,	19,	10,	57,	45,	68,	34 },//20101
	{ 25,	20104,	8,	6,	13,	50,	20,	10,	58,	46,	69,	34 },//20101
	{ 25,	20104,	9,	6,	14,	51,	21,	10,	59,	47,	70,	35 },//20101
	{ 25,	20104,	11,	7,	15,	52,	21,	11,	60,	48,	72,	36 },//20101
	{ 25,	20104,	12,	7,	16,	54,	22,	11,	61,	48,	73,	36 },//20101
	{ 25,	20104,	13,	8,	17,	55,	22,	11,	62,	49,	74,	37 },//20101
	{ 25,	20104,	15,	10,	18,	56,	23,	12,	63,	50,	75,	37 },//20101
	{ 35,	20104,	18,	30,	23,	63,	27,	13,	69,	55,	82,	41 },
	{ 35,	20104,	19,	35,	24,	64,	27,	14,	70,	56,	84,	42 },
	{ 35,	20104,	21,	40,	25,	66,	28,	14,	71,	56,	85,	42 },
	{ 35,	20104,	22,	50,	26,	67,	28,	14,	72,	57,	86,	43 },
	{ 35,	20104,	24,	55,	27,	68,	29,	15,	73,	58,	87,	43 },
	{ 35,	20104,	25,	60,	27,	69,	30,	15,	74,	59,	88,	44 },
	{ 35,	20104,	27,	65,	28,	70,	30,	15,	75,	60,	90,	45 },
	{ 35,	20104,	28,	70,	29,	72,	31,	15,	76,	60,	91,	45 },
	{ 35,	20104,	30,	80,	30,	73,	31,	16,	77,	61,	92,	46 },
	{ 35,	20104,	32,	100,	31,	74,	32,	16,	78,	62,	93,	46 },
	{ 50,	20107,	35,	120,	36,	81,	36,	18,	84,	67,	100,	50 },
	{ 50,	20107,	36,	125,	38,	84,	37,	18,	86,	68,	103,	51 },
	{ 50,	20107,	37,	130,	40,	86,	38,	19,	88,	70,	105,	52 },
	{ 50,	20107,	38,	135,	42,	88,	39,	20,	90,	72,	108,	54 },
	{ 50,	20107,	40,	140,	43,	90,	40,	20,	91,	72,	109,	54 },
	{ 50,	20107,	42,	145,	44,	91,	40,	20,	92,	73,	110,	55 },
	{ 50,	20107,	44,	150,	45,	93,	42,	21,	94,	75,	112,	56 },
	{ 50,	20107,	46,	160,	46,	94,	42,	21,	95,	76,	114,	57 },
	{ 50,	20107,	48,	170,	48,	97,	43,	22,	97,	77,	116,	58 },
	{ 50,	20107,	50,	200,	50,	99,	45,	22,	99,	79,	118,	59 }
};

CHorseRider::CHorseRider()
{
	Initialize();
}

CHorseRider::~CHorseRider()
{
	Destroy();
}

void CHorseRider::Initialize()
{
	m_eventStaminaRegen = NULL;
	m_eventStaminaConsume = NULL;
	memset(&m_Horse, 0, sizeof(m_Horse));
}

void CHorseRider::Destroy()
{
	event_cancel(&m_eventStaminaRegen);
	event_cancel(&m_eventStaminaConsume);
}

void CHorseRider::EnterHorse()
{
	if (GetHorseLevel() <= 0)
		return;

	if (GetHorseHealth() <= 0)
		return;

	if (IsHorseRiding())
	{
		m_Horse.bRiding = !m_Horse.bRiding;
		StartRiding();
	}
	else
	{
		StartStaminaRegenEvent();
	}
	CheckHorseHealthDropTime(false);
}

bool CHorseRider::ReviveHorse()
{
	if (GetHorseLevel() <= 0)
		return false;

	if (GetHorseHealth()>0)
		return false;

	int level = GetHorseLevel();

	m_Horse.sHealth = c_aHorseStat[level].iMaxHealth;
	m_Horse.sStamina = c_aHorseStat[level].iMaxStamina;

	ResetHorseHealthDropTime();

	StartStaminaRegenEvent();
	return true;
}

short CHorseRider::GetHorseMaxHealth() const
{
	int level = GetHorseLevel();
	return c_aHorseStat[level].iMaxHealth;
}

short CHorseRider::GetHorseMaxStamina() const
{
	int level = GetHorseLevel();
	return c_aHorseStat[level].iMaxStamina;
}

void CHorseRider::FeedHorse()
{
	if (GetHorseLevel() > 0 && GetHorseHealth() > 0)
	{
		UpdateHorseHealth(+1);
		ResetHorseHealthDropTime();
	}
}

void CHorseRider::SetHorseData(const THorseInfo& crInfo)
{
	m_Horse = crInfo;
}

// Stamina
void CHorseRider::UpdateHorseDataByLogoff(DWORD dwLogoffTime)
{
	if (GetHorseLevel() <= 0)
		return;

	if (dwLogoffTime >= 12 * 60)
		UpdateHorseStamina(dwLogoffTime / 12 / 60, false);
}

void CHorseRider::UpdateHorseStamina(int iStamina, bool bSend)
{
	int level = GetHorseLevel();

	m_Horse.sStamina = MINMAX(0, m_Horse.sStamina + iStamina, c_aHorseStat[level].iMaxStamina);

	if (GetHorseStamina() == 0 && IsHorseRiding())
	{
		StopRiding();
	}

	if (bSend)
		SendHorseInfo();
}

bool CHorseRider::StartRiding()
{
	if (m_Horse.bRiding)
		return false;

	if (GetHorseLevel() <= 0)
		return false;

	if (GetHorseHealth() <= 0)
		return false;

	if (GetHorseStamina() <= 0)
		return false;

	m_Horse.bRiding = true;
	// StartStaminaConsumeEvent();//Horse Stamina Decline
	SendHorseInfo();
	return true;
}

bool CHorseRider::StopRiding()
{
	if (!m_Horse.bRiding)
		return false;

	m_Horse.bRiding = false;
	StartStaminaRegenEvent();
	return true;
}

EVENTINFO(horserider_info)
{
	CHorseRider* hr;

	horserider_info()
	: hr( 0 )
	{
	}
};

EVENTFUNC(horse_stamina_consume_event)
{
	horserider_info* info = dynamic_cast<horserider_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "horse_stamina_consume_event> <Factor> Null pointer" );
		return 0;
	}

	CHorseRider* hr = info->hr;

	if (hr->GetHorseHealth() <= 0)
	{
		hr->m_eventStaminaConsume = NULL;
		return 0;
	}

	hr->UpdateHorseStamina(-1);
	hr->UpdateRideTime(HORSE_STAMINA_CONSUME_INTERVAL);

	int delta = PASSES_PER_SEC(HORSE_STAMINA_CONSUME_INTERVAL);

	if (hr->GetHorseStamina() == 0)
	{
		hr->m_eventStaminaConsume = NULL;
		delta = 0;
	}

	hr->CheckHorseHealthDropTime();
	sys_log(0, "HORSE STAMINA - %p", get_pointer(event));
	return delta;
}

EVENTFUNC(horse_stamina_regen_event)
{
	horserider_info* info = dynamic_cast<horserider_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "horse_stamina_regen_event> <Factor> Null pointer" );
		return 0;
	}

	CHorseRider* hr = info->hr;

	if (hr->GetHorseHealth()<=0)
	{
		hr->m_eventStaminaRegen = NULL;
		return 0;
	}

	hr->UpdateHorseStamina(+1);
	int delta = PASSES_PER_SEC(HORSE_STAMINA_REGEN_INTERVAL);
	if (hr->GetHorseStamina() == hr->GetHorseMaxStamina())
	{
		delta = 0;
		hr->m_eventStaminaRegen = NULL;
	}

	hr->CheckHorseHealthDropTime();
	sys_log(0, "HORSE STAMINA + %p", get_pointer(event));


	return delta;
}

void CHorseRider::StartStaminaConsumeEvent()
{
	if (GetHorseLevel() <= 0)
		return;

	if (GetHorseHealth() <= 0)
		return;

	sys_log(0,"HORSE STAMINA REGEN EVENT CANCEL %p", get_pointer(m_eventStaminaRegen));
	event_cancel(&m_eventStaminaRegen);

	if (m_eventStaminaConsume)
		return;

	horserider_info* info = AllocEventInfo<horserider_info>();

	info->hr = this;
	m_eventStaminaConsume = event_create(horse_stamina_consume_event, info, PASSES_PER_SEC(HORSE_STAMINA_CONSUME_INTERVAL));
	sys_log(0,"HORSE STAMINA CONSUME EVENT CREATE %p", get_pointer(m_eventStaminaConsume));
}

void CHorseRider::StartStaminaRegenEvent()
{
	if (GetHorseLevel() <= 0)
		return;

	if (GetHorseHealth() <= 0)
		return;

	sys_log(0,"HORSE STAMINA CONSUME EVENT CANCEL %p", get_pointer(m_eventStaminaConsume));
	event_cancel(&m_eventStaminaConsume);

	if (m_eventStaminaRegen)
		return;

	horserider_info* info = AllocEventInfo<horserider_info>();

	info->hr = this;
	m_eventStaminaRegen = event_create(horse_stamina_regen_event, info, PASSES_PER_SEC(HORSE_STAMINA_REGEN_INTERVAL));
	sys_log(0,"HORSE STAMINA REGEN EVENT CREATE %p", get_pointer(m_eventStaminaRegen));
}

// Health
void CHorseRider::ResetHorseHealthDropTime()
{
	m_Horse.dwHorseHealthDropTime = get_global_time() + HORSE_HEALTH_DROP_INTERVAL;
}

void CHorseRider::CheckHorseHealthDropTime(bool bSend)
{
	DWORD now = get_global_time();

	while (m_Horse.dwHorseHealthDropTime < now)
	{
		m_Horse.dwHorseHealthDropTime += HORSE_HEALTH_DROP_INTERVAL;
		UpdateHorseHealth(-1, bSend);
	}
}

void CHorseRider::UpdateHorseHealth(int iHealth, bool bSend)
{
	int level = GetHorseLevel();

	m_Horse.sHealth = MINMAX(0, m_Horse.sHealth + iHealth, c_aHorseStat[level].iMaxHealth);

	if (level && m_Horse.sHealth == 0)
		HorseDie();

	if (bSend)
		SendHorseInfo();
}

void CHorseRider::HorseDie()
{
	sys_log(0, "HORSE DIE %p %p", get_pointer(m_eventStaminaRegen), get_pointer(m_eventStaminaConsume));
	UpdateHorseStamina(-m_Horse.sStamina);
	event_cancel(&m_eventStaminaRegen);
	event_cancel(&m_eventStaminaConsume);
}

void CHorseRider::SetHorseLevel(int iLevel)
{
	m_Horse.bLevel = iLevel = MINMAX(0, iLevel, HORSE_MAX_LEVEL);

	m_Horse.sStamina = c_aHorseStat[iLevel].iMaxStamina;
	m_Horse.sHealth = c_aHorseStat[iLevel].iMaxHealth;
	m_Horse.dwHorseHealthDropTime = 0;

	ResetHorseHealthDropTime();

	SendHorseInfo();
}

BYTE CHorseRider::GetHorseGrade()
{
	return 3;//At 1 levelden itibaren atak

	BYTE grade = 0;

	if (GetHorseLevel())
		grade = (GetHorseLevel() - 1) / 10 + 1;

	return grade;
}
