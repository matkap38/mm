#ifndef __INC_MESSENGER_MANAGER_H
#define __INC_MESSENGER_MANAGER_H

#include "db.h"

class MessengerManager : public singleton<MessengerManager>
{
	public:
		typedef std::string keyT;
		#ifdef ENABLE_MESSENGER_BLOCK
		typedef std::string keyBL;
		#endif
		typedef const std::string & keyA;

		MessengerManager();
		virtual ~MessengerManager();

	public:
		void	P2PLogin(keyA account);
		void	P2PLogout(keyA account);

		void	Login(keyA account);
		void	Logout(keyA account);

		void	RequestToAdd(LPCHARACTER ch, LPCHARACTER target);
		bool	AuthToAdd(keyA account, keyA companion, bool bDeny); // @fixme130 void -> bool

		void	__AddToList(keyA account, keyA companion);
		void	AddToList(keyA account, keyA companion);

		void	__RemoveFromList(keyA account, keyA companion);
		void	RemoveFromList(keyA account, keyA companion);

		#ifdef ENABLE_MESSENGER_BLOCK
		void	__AddToBlockList(keyA account, keyA companion);	// ?? m_Relation, m_InverseRelation ???? ???
		void	AddToBlockList(keyA account, keyA companion);
		bool	IsBlocked(keyA account, keyA companion);
		bool	IsBlocked_Target(keyA account, keyA companion);
		bool	IsBlocked_Me(keyA account, keyA companion);
		bool	IsFriend(keyA account, keyA companion);
		void	__RemoveFromBlockList(keyA account, keyA companion); // ?? m_Relation, m_InverseRelation ???? ???
		void	RemoveFromBlockList(keyA account, keyA companion);
		void	RemoveAllBlockList(keyA account);
		void	EkleLan(keyA account, keyA companion);
		#endif

		void	RemoveAllList(keyA account);

		void	Initialize();

	private:
		void	SendList(keyA account);
		void	SendLogin(keyA account, keyA companion);
		void	SendLogout(keyA account, keyA companion);

		void	LoadList(SQLMsg * pmsg);
		void	Destroy();

		#ifdef ENABLE_MESSENGER_BLOCK
		void	SendBlockLogin(keyA account, keyA companion);
		void	SendBlockLogout(keyA account, keyA companion);
		void	LoadBlockList(SQLMsg * pmsg);
		void	SendBlockList(keyA account);
		#endif

		#ifdef ENABLE_MESSENGER_TEAM_SYSTEM
		void	SendTeamLogin(keyA account, keyA companion);
		void	SendTeamLogout(keyA account, keyA companion);
		void	LoadTeamList(SQLMsg* pmsg);
		void	SendTeamList(keyA account);
		#endif

		std::set<keyT>			m_set_loginAccount;
		std::map<keyT, std::set<keyT> >	m_Relation;
		std::map<keyT, std::set<keyT> >	m_InverseRelation;
		std::set<DWORD>			m_set_requestToAdd;

		#ifdef ENABLE_MESSENGER_BLOCK
		std::map<keyT, std::set<keyT> >	m_BlockRelation;
		std::map<keyT, std::set<keyT> >	m_InverseBlockRelation;
		std::set<DWORD>			m_set_requestToBlockAdd;
		#endif

		#ifdef ENABLE_MESSENGER_TEAM_SYSTEM
		std::map<keyT, std::set<keyT> >	m_TeamRelation;
		std::map<keyT, std::set<keyT> >	m_InverseTeamRelation;
		#endif
};

#endif

