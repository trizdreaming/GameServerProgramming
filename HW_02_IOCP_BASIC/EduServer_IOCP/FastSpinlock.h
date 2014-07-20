#pragma once

class FastSpinlock
{
public:
	FastSpinlock();
	~FastSpinlock();

	void EnterLock();
	void LeaveLock();
	
private:
	//복사 생성자와 대입 생성자를 막아주기 위해서 외부 접근 차단
	//FastSpinLock fs3(fs); 차단
	//FastSpinLock fs2 = fs;차단
	FastSpinlock(const FastSpinlock& rhs);
	FastSpinlock& operator=(const FastSpinlock& rhs);

	volatile long mLockFlag;
};

class FastSpinlockGuard
{
public:
	FastSpinlockGuard(FastSpinlock& lock) : mLock(lock)
	{
		mLock.EnterLock();
	}

	~FastSpinlockGuard()
	{
		mLock.LeaveLock();
	}

private:
	FastSpinlock& mLock;
};

//////////////////////////////////////////////////////////////////////////
//일단 요거는 현재 사용되지 않는 듯
//다양한 클래스를 받아서 각 클래스 타입에 대한 lock 처리를 하는 부분
//template 클래스를 이용함
template <class TargetClass>
class ClassTypeLock
{
public:
	struct LockGuard
	{
		LockGuard()
		{
			TargetClass::mLock.EnterLock();
		}

		~LockGuard()
		{
			TargetClass::mLock.LeaveLock();
		}

	};

private:
	static FastSpinlock mLock;
	
	//friend struct LockGuard;
};

//////////////////////////////////////////////////////////////////////////
//템플릿 클래스 내부 전역 변수 초기화 영역
//FastSpinlock은 선언 즉시 생성자가 동작하는 온전한 객체이기 때문에 초기화를 따로 하지 않음
template <class TargetClass>
FastSpinlock ClassTypeLock<TargetClass>::mLock;