﻿#pragma once


#include "Exception.h"
#include "FastSpinlock.h"


template <class TOBJECT, int ALLOC_COUNT = 100>
class ObjectPool : public ClassTypeLock<ObjectPool<TOBJECT>>
{
public:

	static void* operator new(size_t objSize)
	{
		//TODO: TOBJECT 타입 단위로 lock 잠금
		// FastSpinlockGuard criticalSection( mLock );
		ClassTypeLock<ObjectPool<TOBJECT>>::LockGuard criticalSection;

		if (!mFreeList)
		{
			mFreeList = new uint8_t[sizeof(TOBJECT)*ALLOC_COUNT];

			uint8_t* pNext = mFreeList;
			uint8_t** ppCurr = reinterpret_cast<uint8_t**>(mFreeList);

			for (int i = 0; i < ALLOC_COUNT - 1; ++i)
			{				
				/// OBJECT의 크기가 반드시 포인터 크기보다 커야 한다
				pNext += sizeof( TOBJECT );
				*ppCurr = pNext;
				ppCurr = reinterpret_cast<uint8_t**>( pNext );
			}

			// 이 부분 추가했음
			*ppCurr = 0;
			mTotalAllocCount += ALLOC_COUNT;
		}

		CRASH_ASSERT( mCurrentUseCount < mTotalAllocCount );
		// printf_s( "할당 %d / %d \n", mCurrentUseCount, mTotalAllocCount );

		uint8_t* pAvailable = mFreeList;
		mFreeList = *reinterpret_cast<uint8_t**>( pAvailable );
		++mCurrentUseCount;

		return pAvailable;
	}

	static void	operator delete(void* obj)
	{
		//TODO: TOBJECT 타입 단위로 lock 잠금
		// FastSpinlockGuard criticalSection( mLock );
		ClassTypeLock<ObjectPool<TOBJECT>>::LockGuard criticalSection;
		
		CRASH_ASSERT( mCurrentUseCount > 0 );

		--mCurrentUseCount;

		*reinterpret_cast<uint8_t**>( obj ) = mFreeList;
		mFreeList = static_cast<uint8_t*>( obj );
	}


private:
	static uint8_t*	mFreeList;
	static int		mTotalAllocCount; ///< for tracing
	static int		mCurrentUseCount; ///< for tracing
};

template <class TOBJECT, int ALLOC_COUNT>
uint8_t* ObjectPool<TOBJECT, ALLOC_COUNT>::mFreeList = nullptr;

template <class TOBJECT, int ALLOC_COUNT>
int ObjectPool<TOBJECT, ALLOC_COUNT>::mTotalAllocCount = 0;

template <class TOBJECT, int ALLOC_COUNT>
int ObjectPool<TOBJECT, ALLOC_COUNT>::mCurrentUseCount = 0;
