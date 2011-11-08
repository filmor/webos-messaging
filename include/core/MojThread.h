/* ============================================================
 * Date  : Apr 27, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJTHREAD_H_
#define MOJTHREAD_H_

#include "core/MojCoreDefs.h"
#include "core/MojAtomicInt.h"

class MojThreadMutex : private MojNoCopy
{
public:
	MojThreadMutex();
	~MojThreadMutex();

	void lock();
	void unlock();
	bool tryLock();

#ifdef MOJ_DEBUG
	MojThreadIdT owner() const { return m_owner; }
#endif

private:
	friend class MojThreadCond;

	MojThreadMutexT m_mutex;
#ifdef MOJ_DEBUG
	MojThreadIdT m_owner;
#endif
};

class MojThreadRwLock : private MojNoCopy
{
public:
	MojThreadRwLock();
	~MojThreadRwLock();

	void readLock();
	void writeLock();
	void unlock();
	bool tryReadLock();
	bool tryWriteLock();

#ifdef MOJ_DEBUG
	MojThreadIdT writer() const { return m_writer; }
#endif

private:
	MojThreadRwLockT m_lock;
#ifdef MOJ_DEBUG
	MojAtomicInt m_readerCount;
	MojThreadIdT m_writer;
#endif
};

class MojThreadCond : private MojNoCopy
{
public:
	MojThreadCond();
	~MojThreadCond();

	MojErr signal() { return MojThreadCondSignal(&m_cond); }
	MojErr broadcast() { return MojThreadCondBroadcast(&m_cond); }
	MojErr wait(MojThreadMutex& mutex);

private:
	MojThreadCondT m_cond;
};

class MojThreadGuard : private MojNoCopy
{
public:
	MojThreadGuard(MojThreadMutex& mutex);
	MojThreadGuard(MojThreadMutex& mutex, bool lockNow);
	~MojThreadGuard();

	void lock();
	void unlock();
	bool tryLock();

private:
	MojThreadMutex& m_mutex;
	bool m_locked;
};

class MojThreadReadGuard : private MojNoCopy
{
public:
	MojThreadReadGuard(MojThreadRwLock& lock);
	MojThreadReadGuard(MojThreadRwLock& lock, bool lockNow);
	~MojThreadReadGuard();

	void lock();
	void unlock();
	bool tryLock();

private:
	MojThreadRwLock& m_lock;
	bool m_locked;
};

class MojThreadWriteGuard : private MojNoCopy
{
public:
	MojThreadWriteGuard(MojThreadRwLock& lock);
	MojThreadWriteGuard(MojThreadRwLock& lock, bool lockNow);
	~MojThreadWriteGuard();

	void lock();
	void unlock();
	bool tryLock();

private:
	MojThreadRwLock& m_lock;
	bool m_locked;
};

template <class T>
struct MojThreadLocalValueDefaultCtor
{
	T* operator()() { return new T; }
};

template <class T>
struct MojThreadLocalValueZeroCtor
{
	T* operator()() { return new T(0); }
};

template <class T, class CTOR = MojThreadLocalValueDefaultCtor<T> >
class MojThreadLocalValue : private MojNoCopy
{
public:
	MojThreadLocalValue();
	~MojThreadLocalValue();

	MojErr get(T*& valOut);

private:
	static void destroy(void* val);

	MojThreadKeyT m_key;
};

#define MojAssertMutexLocked(MUTEX) MojAssert((MUTEX).owner() == MojThreadCurrentId())
#define MojAssertMutexUnlocked(MUTEX) MojAssert((MUTEX).owner() != MojThreadCurrentId())
#define MojAssertWriteLocked(LOCK) MojAssert((LOCK).writer() == MojThreadCurrentId())

#include "core/internal/MojThreadInternal.h"

#endif /* MOJTHREAD_H_ */
