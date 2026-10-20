#pragma once

#ifdef ENABLE_TELEPORTION_WIZARD_SYSTEM
class CTeleportionWizard : public singleton<CTeleportionWizard>
{
	private:
		typedef std::vector<TWizardDataInfoTable> TMapTeleportList;
		TMapTeleportList m_TeleportListVec;

		typedef std::map<DWORD, std::vector<TWizardDropInfoTable>> TMapDropItemList;
		TMapDropItemList m_mapDropItemList;

		typedef std::map<WORD, std::string> TMapDungeonCooldownList;
		TMapDungeonCooldownList m_mapDungeonCooldownList;

	public:
		CTeleportionWizard();
		virtual ~CTeleportionWizard();

		void Destroy();
		bool Initialize();
		void Reload();
		void GetDropItemList(DWORD dwRaceNum, std::vector<TWizardDropInfoTable>& vec_item);
		int  GetCooldown(LPCHARACTER ch, int id);

		void Warp(LPCHARACTER ch, int id);
		void DropInfo(LPCHARACTER ch, int id);
		void SendPacket(LPCHARACTER ch);

	protected:
		bool m_bActiveStatus;
};
#endif
