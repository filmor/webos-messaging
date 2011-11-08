/* ============================================================
 * Date  : Apr 15, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJTIMEINTERNAL_H_
#define MOJTIMEINTERNAL_H_

inline void MojTime::fromTimeval(const MojTimevalT* tv)
{
	MojAssert(tv);
	m_val = tv->tv_sec * UnitsPerSec + tv->tv_usec;
}

inline void MojTime::toTimeval(MojTimevalT* tvOut) const
{
	MojAssert(tvOut);
	tvOut->tv_sec = (MojTimeT) secs();
	tvOut->tv_usec = microsecsPart();
}

inline void MojTime::fromTimespec(const MojTimespecT* ts)
{
	MojAssert(ts);
	m_val = ts->tv_sec * UnitsPerSec + ts->tv_nsec / 1000;
}

inline void MojTime::toTimespec(MojTimespecT* tsOut) const
{
	MojAssert(tsOut);
	tsOut->tv_sec = (MojTimeT) secs();
	tsOut->tv_nsec = microsecsPart() * 1000;
}

#endif /* MOJTIMEINTERNAL_H_ */
