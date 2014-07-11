// GSP_WSAAsyncSelectEchoServer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "GSP_WSAAsyncSelectEchoServer.h"
#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define WM_SOCKET (WM_APP+1)
#define MAX_LOADSTRING 100
#define BUFFER_SIZE 1024*4

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass( HINSTANCE hInstance );
BOOL				InitInstance( HINSTANCE, int );
LRESULT CALLBACK	WndProc( HWND, UINT, WPARAM, LPARAM );

//server 관련 데이터
SOCKET g_ServerSocket;
SOCKET g_ConnSocket;

// server 관련 전방 선언
BOOL ServerInit( HWND hWnd );
void EndSocket();
// void OnAccept( HWND hWnd, SOCKET instSock );
// void OnRead( SOCKET instSock );
// void OnClose( HWND hWnd, SOCKET instSock );

int APIENTRY _tWinMain( _In_ HINSTANCE hInstance,
						_In_opt_ HINSTANCE hPrevInstance,
						_In_ LPTSTR    lpCmdLine,
						_In_ int       nCmdShow )
{
	//필요 없어서 막아 둔 거
	UNREFERENCED_PARAMETER( hPrevInstance );
	UNREFERENCED_PARAMETER( lpCmdLine );

	//콘솔 창 alloc
	AllocConsole();
	FILE* pFile;
	freopen_s( &pFile, "CONOUT$", "wb", stdout );

// #ifdef _DEBUG
// 	printf_s( "뿅" );
// #endif

	// TODO: Place code here.
	MSG msg;

	// Initialize global strings
	//LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString( hInstance, IDC_GSP_WSAASYNCSELECTECHOSERVER, szWindowClass, MAX_LOADSTRING );
	MyRegisterClass( hInstance );

	// Perform application initialization:
	if ( !InitInstance( hInstance, nCmdShow ) )
	{
		return FALSE;
	}


	// Main message loop:
	while ( TRUE )
	{
		GetMessage( &msg, NULL, 0, 0 );
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	//WSACleanup();
	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass( HINSTANCE hInstance )
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof( WNDCLASSEX );

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE( IDC_GSP_WSAASYNCSELECTECHOSERVER ) );
	wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
	wcex.lpszMenuName = MAKEINTRESOURCE( IDC_GSP_WSAASYNCSELECTECHOSERVER );
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon( wcex.hInstance, MAKEINTRESOURCE( IDI_SMALL ) );

	return RegisterClassEx( &wcex );
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance( HINSTANCE hInstance, int nCmdShow )
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow( szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
						 CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL );

	if ( !hWnd )
	{
		return FALSE;
	}

	//윈도우 창은 필요가 없다
	//ShowWindow(hWnd, nCmdShow);
	//UpdateWindow(hWnd);

	//서버 Start
	printf_s( "Server Init Start \n" );
	if ( !ServerInit( hWnd ) )
	{
		return FALSE;
	}
	//printf_s( "Server Init Complete \n" );

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//

typedef struct ConnectSocket
{
	SOCKET connSocket;
	char buffer[BUFFER_SIZE];
}Connects;

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	SOCKADDR_IN connAddr;
	int rc;
	char buffer[BUFFER_SIZE] = { 0, };
	//Connects a;

	switch ( message )
	{
		case WM_SOCKET:
			if (WSAGETSELECTERROR(lParam))
			{
				closesocket( (SOCKET)wParam );
				break;
			}
			switch ( WSAGETSELECTEVENT(lParam) )
			{
				case FD_ACCEPT:
					
					rc = sizeof( connAddr );
					g_ConnSocket = accept( (SOCKET)wParam, (PSOCKADDR)&connAddr, &rc );
					if (g_ConnSocket == INVALID_SOCKET)
					{
						EndSocket();
						printf_s( "Socket Accept err No: %d", WSAGetLastError() );
						break;
					}
					printf_s( "Socket Accept OK [%d, %s : %d] \n", g_ConnSocket, inet_ntoa(connAddr.sin_addr), ntohs(connAddr.sin_port) );

					rc = WSAAsyncSelect( g_ConnSocket, hWnd, WM_SOCKET, FD_READ | FD_CLOSE );
					if (rc == SOCKET_ERROR)
					{
						EndSocket();
						printf_s( "WSAAsyncSelect err No: %d", WSAGetLastError() );
					}

					break;

				case  FD_READ:
					rc = recv( (SOCKET)wParam, buffer, BUFFER_SIZE, 0 );
					
#ifdef _DEBUG
					printf_s( "우왕 데이터 왔땅! \n" );
#endif
					
					if ( rc != SOCKET_ERROR)
					{
						send( (SOCKET)wParam, buffer, rc, 0 );
#ifdef _DEBUG
						printf_s( "반사! \n" );
#endif
					}
					else
					{
						printf_s( "Socket Recv err No: %d", WSAGetLastError() );
					}

					break;

				case FD_CLOSE:
					EndSocket();

					break;

				default:
					break;
			}

		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
	}
	return 0;
}

BOOL ServerInit( HWND hWnd )
{
	WSADATA wsaData = { 0, };
	SOCKADDR_IN serverAddr = { 0, };

	int err = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
	if (err != 0)
	{
		printf_s( "WSAStartup failed with error: %d \n", err );
		return FALSE;
	}

	//서버 소켓 생성
	g_ServerSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if (g_ServerSocket == INVALID_SOCKET)
	{
		printf_s( "Server Socket err No: %d", WSAGetLastError() );
		return FALSE;
	}

	//서버 소켓 설정 하기
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl( INADDR_ANY );
	//////////////////////////////////////////////////////////////////////////
	//참고_특정 주소 받을 때
	//serverAddr.sin_addr.s_addr = inet_addr( "xxx.xxx.xxx.xxx" );
	//////////////////////////////////////////////////////////////////////////
	serverAddr.sin_port = htons( 9001 ); //제공된 client port 번호를 따름
	
	err = bind( g_ServerSocket, (PSOCKADDR)&serverAddr, sizeof( serverAddr ) );
	if (err == SOCKET_ERROR)
	{
		EndSocket();
		printf_s( "Server Socket Bind err No: %d", WSAGetLastError() );
		return FALSE;
	}
	printf_s( "Server Socket Bind OK [%s : %d] \n", inet_ntoa( serverAddr.sin_addr ), ntohs( serverAddr.sin_port ) );

	err = WSAAsyncSelect( g_ServerSocket, hWnd, WM_SOCKET, FD_ACCEPT | FD_CLOSE );
	if (err == SOCKET_ERROR)
	{
		EndSocket();
		printf_s( "WSAAsyncSelect err No: %d", WSAGetLastError() );
		return FALSE;
	}
	printf_s( "WSAAsyncSelect OK \n" );

	err = listen( g_ServerSocket, SOMAXCONN );
	if (err == SOCKET_ERROR)
	{
		EndSocket();
		printf_s( "Server Socket Listen err No: %d", WSAGetLastError() );
		return FALSE;
	}
	printf_s( "Now Listen... \n" );

	return TRUE;
}

void EndSocket()
{
	if ( g_ConnSocket )
	{
		shutdown( g_ConnSocket, SD_BOTH );
		closesocket( g_ConnSocket );
		g_ConnSocket = NULL;
	}

	if ( g_ServerSocket )
	{
		closesocket( g_ServerSocket );
		g_ServerSocket = NULL;
	}
}