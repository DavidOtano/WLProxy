#ifndef __THREADLOCK_H__
#define __THREADLOCK_H__

#include <WinSock2.h>
#include <Windows.h>

class ThreadLock {
public:
	ThreadLock() { InitializeCriticalSection(&m_cs); }
	CRITICAL_SECTION* GetLock() { return &m_cs; }
	~ThreadLock() { DeleteCriticalSection(&m_cs); }
private:
protected:
	CRITICAL_SECTION m_cs;
};

class SetLock {
public:
	SetLock(ThreadLock& lock) { m_plock = &lock; EnterCriticalSection(lock.GetLock()); }
	~SetLock() { LeaveCriticalSection(m_plock->GetLock()); }
private:
protected:
	ThreadLock* m_plock;
};

#endif
