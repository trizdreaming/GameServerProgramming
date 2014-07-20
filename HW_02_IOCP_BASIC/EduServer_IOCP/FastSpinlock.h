#pragma once

class FastSpinlock
{
public:
	FastSpinlock();
	~FastSpinlock();

	void EnterLock();
	void LeaveLock();
	
private:
	//���� �����ڿ� ���� �����ڸ� �����ֱ� ���ؼ� �ܺ� ���� ����
	//FastSpinLock fs3(fs); ����
	//FastSpinLock fs2 = fs;����
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
//�ϴ� ��Ŵ� ���� ������ �ʴ� ��
//�پ��� Ŭ������ �޾Ƽ� �� Ŭ���� Ÿ�Կ� ���� lock ó���� �ϴ� �κ�
//template Ŭ������ �̿���
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
//���ø� Ŭ���� ���� ���� ���� �ʱ�ȭ ����
//FastSpinlock�� ���� ��� �����ڰ� �����ϴ� ������ ��ü�̱� ������ �ʱ�ȭ�� ���� ���� ����
template <class TargetClass>
FastSpinlock ClassTypeLock<TargetClass>::mLock;