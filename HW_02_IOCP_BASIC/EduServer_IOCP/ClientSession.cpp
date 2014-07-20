#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"


bool ClientSession::OnConnect(SOCKADDR_IN* addr)
{
	//TODO: 이 영역 lock으로 보호 할 것
	mLock.EnterLock();
	//mLock.LeaveLock();

	CRASH_ASSERT(LThreadType == THREAD_MAIN_ACCEPT);

	/// make socket non-blocking
	u_long arg = 1 ;
	ioctlsocket(mSocket, FIONBIO, &arg) ;

	/// turn off nagle
	int opt = 1 ;
	setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)) ;

	opt = 0;
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)) )
	{
		printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError()) ;
		return false;
	}
	
	//TODO: 여기에서 CreateIoCompletionPort((HANDLE)mSocket, ...);사용하여 연결할 것
	//세션 자체를 키로 하고 있는 상태라 this로 연결
	HANDLE handle = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetComletionPort(), ( ULONG_PTR )this, 0 );
	//이건 각 소켓을 comletionPort에 연결하는 거기 때문에 당연히 기존 IOCP Port는 유지 되기 때문
	//신규로 생성하는 것이 아니다
	//생성과 바인딩을 별도 처리하는 것
	if (handle != GIocpManager->GetComletionPort())
	{
		printf_s("[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError());
		return false;
	}

	memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
	mConnected = true ;

	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	GSessionManager->IncreaseConnectionCount();

	mLock.LeaveLock();

	return PostRecv() ;
}

void ClientSession::Disconnect(DisconnectReason dr)
{
	//TODO: 이 영역 lock으로 보호할 것
	mLock.EnterLock();

	if ( !IsConnected() )
		return ;
	
	LINGER lingerOption ;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)) )
	{
		printf_s("[DEBUG] setsockopt linger option error: %d\n", GetLastError());
	}

	printf_s("[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));
	
	GSessionManager->DecreaseConnectionCount();

	closesocket(mSocket) ;

	mConnected = false ;

	mLock.LeaveLock();
}

bool ClientSession::PostRecv() const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* recvContext = new OverlappedIOContext(this, IO_RECV);

	//TODO: WSARecv 사용하여 구현할 것
	DWORD recvBytes = 0;
	recvContext->mWsaBuf.len = BUFSIZE;
	recvContext->mWsaBuf.buf = recvContext->mBuffer;

	DWORD flags = 0;
	if ( SOCKET_ERROR == WSARecv( mSocket, &recvContext->mWsaBuf, 1, &recvBytes, &flags, &( recvContext->mOverlapped ), NULL ))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			printf_s( "%d", WSAGetLastError() );
			delete recvContext;
			return false;
		}

	}

	return true;
}

bool ClientSession::PostSend(const char* buf, int len) const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* sendContext = new OverlappedIOContext(this, IO_SEND);

	/// copy for echoing back..
	memcpy_s(sendContext->mBuffer, BUFSIZE, buf, len);

	//TODO: WSASend 사용하여 구현할 것
	DWORD sendBytes = 0;
	sendContext->mWsaBuf.len = len;
	sendContext->mWsaBuf.buf = sendContext->mBuffer;
	
	//윈도우 소켓 버퍼에 대한 정보 포함
	//http://yongho1037.tistory.com/531 참고
	//지금은 에코 서버니까 바로 복사해서 PostSend처리
	if (SOCKET_ERROR== WSASend(mSocket, &sendContext->mWsaBuf, 1, (LPDWORD)& sendBytes, NULL, &(sendContext->mOverlapped),NULL))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			delete sendContext;
			return false;
		}
	}

	return true;
}