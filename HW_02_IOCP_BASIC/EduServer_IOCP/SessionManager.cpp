#include "stdafx.h"
#include "ClientSession.h"
#include "SessionManager.h"

SessionManager* GSessionManager = nullptr;

ClientSession* SessionManager::CreateClientSession(SOCKET sock)
{
	ClientSession* client = new ClientSession(sock);

	//TODO: lock으로 보호할 것
	mLock.EnterLock();
	{
		mClientList.insert(ClientList::value_type(sock, client));
	}
	mLock.LeaveLock();

	return client;
}


void SessionManager::DeleteClientSession(ClientSession* client)
{
	//TODO: lock으로 보호할 것
	mLock.EnterLock();
	{
		mClientList.erase(client->mSocket);
	}
	mLock.LeaveLock();
	
	delete client;
}