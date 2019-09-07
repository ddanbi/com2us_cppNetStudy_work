#include "User.h"


namespace ChatServerLib
{
	//TODO ����� User Ŭ������ �ڵ� ũ��� �����Ƿ� .cpp�� .h ���� ��ġ��
	//User�� ũ�Ⱑ �۴ٸ� �������� ����ϴ� header�� �ִ°��� ����.


	void User::Init(const int index) 
	{
		m_UserIndex = index;
	}

	int User::SetLogin(int conn_idx, char* login_id) 
	{

		m_NetConnIdx = conn_idx;
		m_CurDomainState = DOMAIN_STATE::LOGIN;
		m_UserId = login_id;

		return 0;
	}

	void User::EnterRoom(int roomIndex) 
	{
		m_RoomIndex = roomIndex;
		m_CurDomainState = DOMAIN_STATE::ROOM;
	}


	void User::Clear() 
	{
		m_UserIndex = -1;
		m_SessionIndex = -1;
		m_RoomIndex = -1;

		m_UserId = "";
		m_IsConfirm = false;

		m_NetConnIdx = -1;

		m_CurDomainState = DOMAIN_STATE::NONE;
	}

	void User::SetDomainState(DOMAIN_STATE val) 
	{
		m_CurDomainState = val;
	}
}