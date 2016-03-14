#include "stdafx.h"
#include "ThreadLocal.h"
#include "Exception.h"
#include "Log.h"
#include <fstream>
#include <DbgHelp.h>
#include <TlHelp32.h>
#include <strsafe.h>
#include "StackWalker.h"

#define MAX_BUFF_SIZE 1024

void MakeDump(EXCEPTION_POINTERS* e)
{
	TCHAR tszFileName[MAX_BUFF_SIZE] = { 0 };
	SYSTEMTIME stTime = { 0 };
	GetSystemTime(&stTime);
	StringCbPrintf(tszFileName,
		_countof(tszFileName),
		_T("%s_%4d%02d%02d_%02d%02d%02d.dmp"),
		_T("EduServerDump"),
		stTime.wYear,
		stTime.wMonth,
		stTime.wDay,
		stTime.wHour,
		stTime.wMinute,
		stTime.wSecond);

	HANDLE hFile = CreateFile(tszFileName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
	exceptionInfo.ThreadId = GetCurrentThreadId();
	exceptionInfo.ExceptionPointers = e;
	exceptionInfo.ClientPointers = FALSE;

	//todo: MiniDumpWriteDump를 사용하여 hFile에 덤프 기록
	MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)( MiniDumpWithPrivateReadWriteMemory |
										 MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithFullMemoryInfo |
										 MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules );

	MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, mdt, ( e != 0 ) ? &exceptionInfo : 0, 0, NULL );

	
	if (hFile)
	{
		CloseHandle(hFile);
		hFile = NULL;
	}
	
}


LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
	if ( IsDebuggerPresent() )
		return EXCEPTION_CONTINUE_SEARCH ;

	
	THREADENTRY32 te32;
	DWORD myThreadId = GetCurrentThreadId();
	DWORD myProcessId = GetCurrentProcessId();

	HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap != INVALID_HANDLE_VALUE)
	{
		te32.dwSize = sizeof(THREADENTRY32);

		if (Thread32First(hThreadSnap, &te32))
		{
			do
			{
				//todo: 내 프로세스 내의 스레드중 나 자신 스레드만 빼고 멈추게..
				if (te32.th32ThreadID != myThreadId)
				{
					//이건 어떤지?
					SuspendThread( (HANDLE)te32.th32ThreadID );

// 					HANDLE hThread = OpenThread( THREAD_ALL_ACCESS, false, te32.th32ThreadID );
// 					if ( !hThread )
// 					{
// 						printf_s( "Openthread error %d\n", GetLastError() );
// 						continue;
// 					}
// 					
// 					SuspendThread( hThread );
				}

			} while (Thread32Next(hThreadSnap, &te32));

		}

		CloseHandle(hThreadSnap);
	}
		
	
	std::ofstream historyOut("EduServer_exception.txt", std::ofstream::out);
	
	/// 콜히스토리 남기고
	historyOut << "========== WorkerThread Call History ==========" << std::endl << std::endl;
	for (int i = 0; i < MAX_WORKER_THREAD; ++i)
	{
		if (GThreadCallHistory[i])
		{
			GThreadCallHistory[i]->DumpOut(historyOut);
		}
	}

	/// 콜성능 남기고
	historyOut << "========== WorkerThread Call Performance ==========" << std::endl << std::endl;
	for (int i = 0; i < MAX_WORKER_THREAD; ++i)
	{
		if (GThreadCallElapsedRecord[i])
		{
			GThreadCallElapsedRecord[i]->DumpOut(historyOut);
		}
	}

	/// 콜스택도 남기고
	historyOut << "========== Exception Call Stack ==========" << std::endl << std::endl;

	//todo: StackWalker를 사용하여 historyOut에 현재 스레드의 콜스택 정보 남기기
	//exceptionInfo->ContextRecord
	// 허허 갓동찬님 이걸 다 만드시다니
	/*
	STACKFRAME64 stk;
	memset( &stk, 0, sizeof( stk ) );

	stk.AddrPC.Offset = exceptionInfo->ContextRecord->Rip;
	stk.AddrPC.Mode = AddrModeFlat;
	stk.AddrStack.Offset = exceptionInfo->ContextRecord->Rsp;
	stk.AddrStack.Mode = AddrModeFlat;
	stk.AddrFrame.Offset = exceptionInfo->ContextRecord->Rbp;
	stk.AddrFrame.Mode = AddrModeFlat;


	for ( ULONG Frame = 0; ; Frame++ )
	{
		BOOL result = StackWalk64(
			IMAGE_FILE_MACHINE_I386,   // __in      DWORD MachineType,
			GetCurrentProcess(),        // __in      HANDLE hProcess,
			GetCurrentThread(),         // __in      HANDLE hThread,
			&stk,                       // __inout   LP STACKFRAME64 StackFrame,
			&exceptionInfo->ContextRecord,                  // __inout   PVOID ContextRecord,
			NULL,                     // __in_opt  PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
			SymFunctionTableAccess64,                      // __in_opt  PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
			SymGetModuleBase64,                     // __in_opt  PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
			NULL                       // __in_opt  PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
			);

		//fprintf( gApplSetup.TraceFile, "*** %2d called from %016LX   STACK %016LX    FRAME %016LX\n", Frame, (ULONG64)stk.AddrPC.Offset, (ULONG64)stk.AddrStack.Offset, (ULONG64)stk.AddrFrame.Offset );
		historyOut << "***" << Frame << "called from" << (ULONG64)stk.AddrPC.Offset << "STACK" << (ULONG64)stk.AddrStack.Offset << "FRAME" << (ULONG64)stk.AddrFrame.Offset << std::endl;

		if ( !result )
			break;
	}
	*/

	StackWalker sw; //= StackWalker( myProcessId, OpenProcess( PROCESS_ALL_ACCESS, TRUE, myProcessId) );
	sw.LoadModules();
	sw.SetOutputStream( &historyOut );
	sw.ShowCallstack();
 	
	/// 이벤트 로그 남기고
	historyOut << "========== Exception Event Log ==========" << std::endl << std::endl;

	LoggerUtil::EventLogDumpOut(historyOut);

	historyOut << "========== Exception Event End ==========" << std::endl << std::endl;

	historyOut.flush();
	historyOut.close();

	/// 마지막으로 dump file 남기자.
	MakeDump(exceptionInfo);


	ExitProcess(1);
	/// 여기서 쫑

	return EXCEPTION_EXECUTE_HANDLER;
	
}