#include "stdafx.h"
#include "Log.h"

#include <iostream>

void ThreadCallHistory::DumpOut(std::ostream& ost)
{
	//todo: ���� �������� call history�� ost ��Ʈ���� ����
	uint64_t count = mCounter < MAX_HISTORY ? mCounter : MAX_HISTORY;

	ost << "===== Recent Call history [Thread:" << mThreadId << "]" << std::endl;

	for ( int i = 1; i <= count; ++i )
	{
		ost << " HISTORY:" << mHistory[( mCounter - i ) % MAX_HISTORY] << std::endl;
	}
	ost << "===== End of Recent Call History" << std::endl << std::endl;
}
	

void ThreadCallElapsedRecord::DumpOut(std::ostream& ost)
{
	uint64_t count = mCounter < MAX_ELAPSED_RECORD ? mCounter : MAX_ELAPSED_RECORD;

	ost << "===== Recent Call Performance [Thread:" << mThreadId << "]" << std::endl;

	for (int i = 1; i <= count; ++i)
	{
		ost << "  FUNC:" << mElapsedFuncSig[(mCounter - i) % MAX_ELAPSED_RECORD] 
			<< "ELAPSED: " << mElapsedTime[(mCounter - i) % MAX_ELAPSED_RECORD] << std::endl;
	}
	ost << "===== End of Recent Call Performance" << std::endl << std::endl;

}


namespace LoggerUtil
{
	LogEvent gLogEvents[MAX_LOG_SIZE];
	__int64 gCurrentLogIndex = 0;

	void EventLogDumpOut(std::ostream& ost)
	{
		//todo: gLogEvents���� ost ��Ʈ���� ����
		uint64_t count = gCurrentLogIndex < MAX_LOG_SIZE ? gCurrentLogIndex : MAX_LOG_SIZE;

		for ( int i = 1; i <= count; ++i )
		{
			ost << " THREAD ID:" << gLogEvents[count - i].mThreadId
				<< " EVENT MSG:" << gLogEvents[count - i].mMessage
				<< " ADDTIONAL INFO:" << gLogEvents[count - i].mAdditionalInfo << std::endl;
		}
		ost << "===== End of Recent Global Event" << std::endl << std::endl;
	}
}
