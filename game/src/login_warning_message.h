#pragma once

#ifdef ENABLE_LOGIN_WARNING_MESSAGE_SYSTEM
typedef struct SMessageInfoTable
{
	WORD	id;
	char	message[CHAT_MAX_LEN+1];
} TMessageInfoTable;

class CLoginWarningMsg : public singleton<CLoginWarningMsg>
{
	private:
		typedef std::vector<TMessageInfoTable> TMessageListVec;
		TMessageListVec m_MessageListVec;

	public:
		CLoginWarningMsg();
		virtual ~CLoginWarningMsg();

		void Destroy();
		bool Initialize();
		void Reload();
		void Login(LPCHARACTER ch);
		void SendPacket(LPCHARACTER ch, const char * szText);

	protected:
		bool m_bActiveStatus;
};
#endif
