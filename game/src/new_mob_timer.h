#ifndef __NEW_MOB_TIMER__
#define __NEW_MOB_TIMER__

//#define DISABLE_IN_CH99

/*Macro defines*/
#define EVERY_HOUR 999
namespace NewMobTimer
{
	enum SysConf
	{
		MAIN_UPDATE_INTERVAL = 60,
	};

	typedef struct SMobTimerInfo
	{
		DWORD	dwVnum;
		DWORD	dwMapIDX;
		int		posX;
		int		posY;
		BYTE	bChannel;
		int		perHour;
		int		perMin;
	} TMobTimerInfo;

	class CNewMobTimer : public singleton<CNewMobTimer>
	{
		private:
			typedef std::map<int, TMobTimerInfo> TMapMobTimerInfoList;
			typedef std::map<int, DWORD> TMapSpawnList;

		protected:
			LPEVENT					updateTimer;
			TMapMobTimerInfoList	m_mapMobTimerInfoList;
			TMapSpawnList			spawnedVIDS;

		public:
			CNewMobTimer();

			void		Initialize();
			void		Destroy();

			TMobTimerInfo* GetData(int iID);
			WORD		GetSize();
			DWORD		GetVnum(int iID);

			void		Update();
			bool		CanSpawnNow(WORD bossID, int timeAsMin);
			bool		CanSpawn(WORD bossID);
			void		SpawnBoss(WORD bossID);
			void		BossIsDead(DWORD dwVID);
	};
};
#endif
