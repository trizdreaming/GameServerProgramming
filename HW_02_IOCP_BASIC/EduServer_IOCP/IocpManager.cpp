#include "stdafx.h"
#include "IocpManager.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"

#define GQCS_TIMEOUT	20

__declspec(thread) int LIoThreadId = 0;
IocpManager* GIocpManager = nullptr;

IocpManager::IocpManager() : mCompletionPort(NULL), mIoThreadCount(2), mListenSocket(NULL)
{
}


IocpManager::~IocpManager()
{
}

bool IocpManager::Initialize()
{
	//TODO: mIoThreadCount = ...;GetSystemInfo����ؼ� set num of I/O threads
	//�ϴ� �ý��� �ھ� ������ 2��� �սô�
	SYSTEM_INFO si;
	GetSystemInfo( &si );
	mIoThreadCount = (int)si.dwNumberOfProcessors * 2;
	printf_s( "ThreadCount = %d\n", mIoThreadCount );

	/// winsock initializing
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	/// Create I/O Completion Port
	//TODO: mCompletionPort = CreateIoCompletionPort(...)
	mCompletionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
	if (mCompletionPort == NULL)
	{
		return false;
	}
	
	/// create TCP socket
	//TODO: mListenSocket = ...
	mListenSocket = WSASocket( PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED );
	if (mListenSocket == INVALID_SOCKET)
	{
		return false;
	}
	
	//���̱� �˰�����
	int opt = TRUE;
	setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	//TODO:  bind
	SOCKADDR_IN serveraddr = { 0, };
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(LISTEN_PORT);
	serveraddr.sin_addr.s_addr = htonl( INADDR_ANY );

	if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)))
		return false;


	return true;
}


bool IocpManager::StartIoThreads()
{
	/// I/O Thread
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		DWORD dwThreadId;
		//TODO: HANDLE hThread = (HANDLE)_beginthreadex...);
		HANDLE hThread = (HANDLE)_beginthreadex( NULL, 0, IoWorkerThread, (LPVOID)i, 0, (unsigned*)&dwThreadId );
	}

	return true;
}


bool IocpManager::StartAcceptLoop()
{
	/// listen
	if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN))
		return false;


	/// accept loop
	while (true)
	{
		SOCKET acceptedSock = accept(mListenSocket, NULL, NULL);
		if (acceptedSock == INVALID_SOCKET)
		{
			printf_s("accept: invalid socket\n");
			continue;
		}

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(acceptedSock, (SOCKADDR*)&clientaddr, &addrlen);

		/// ���� ���� ����ü �Ҵ�� �ʱ�ȭ
		ClientSession* client = GSessionManager->CreateClientSession(acceptedSock);

		/// Ŭ�� ���� ó��
		if (false == client->OnConnect(&clientaddr))
		{
			client->Disconnect(DR_ONCONNECT_ERROR);
			GSessionManager->DeleteClientSession(client);
		}
	}

	return true;
}

void IocpManager::Finalize()
{
	CloseHandle(mCompletionPort);

	/// winsock finalizing
	WSACleanup();

}


unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	LThreadType = THREAD_IO_WORKER;

	//TreadID�� �����ϴ� ���� ������ ���� ������ idx�� �޾� �Ѱܿ�
	LIoThreadId = reinterpret_cast<int>(lpParam);
	HANDLE hComletionPort = GIocpManager->GetComletionPort();

	while (true)
	{
		DWORD dwTransferred = 0;
		OverlappedIOContext* context = nullptr;
		ClientSession* asCompletionKey = nullptr;

		int ret = GetQueuedCompletionStatus( hComletionPort, &dwTransferred, (PULONG_PTR)&asCompletionKey, (LPOVERLAPPED*)&context, GQCS_TIMEOUT );

		/// check time out first 
		if (ret == 0 && GetLastError()==WAIT_TIMEOUT)
			continue;

		if (ret == 0 || dwTransferred == 0)
		{
			/// connection closing
			asCompletionKey->Disconnect(DR_RECV_ZERO);
			GSessionManager->DeleteClientSession(asCompletionKey);
			continue;
		}

		//TODO::
		//Ŀ�ؼ��� ��� �ִ� ����, ���ǵ� ����ִ� ����
		//�׷��� Overlapped ����ü�� �������� �̰�� �񵿱�� �ް� ���� �� Ȯ���� �� ���ݾ�?
		//������ �츮�� �̹� ���ɾ�� ������
		if (nullptr == context)
		{
			continue;
		}

		bool completionOk = true;
		switch (context->mIoType)
		{
		case IO_SEND:
			completionOk = SendCompletion(asCompletionKey, context, dwTransferred);
			break;

		case IO_RECV:
			completionOk = ReceiveCompletion(asCompletionKey, context, dwTransferred);
			break;

		default:
			printf_s("Unknown I/O Type: %d\n", context->mIoType);
			break;
		}

		if ( !completionOk )
		{
			/// connection closing
			asCompletionKey->Disconnect(DR_COMPLETION_ERROR);
			GSessionManager->DeleteClientSession(asCompletionKey);
		}

	}

	return 0;
}

bool IocpManager::ReceiveCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{

	/// echo back ó�� client->PostSend()���.
	if ( !(client->PostSend( context->mBuffer, dwTransferred )) )
	{
		delete context;
		return false;
	}

	delete context;

	return client->PostRecv();
}

bool IocpManager::SendCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	/// ���� �� �Ǿ����� Ȯ���ϴ� �� ó��..
	//if (context->mWsaBuf.len != dwTransferred) {...}
	if (context->mWsaBuf.len != dwTransferred)
	{
		printf_s( "���� ������ �ȵǾ��� \n" );
		delete context;
		return false;
	}

	delete context;
	return true;
}