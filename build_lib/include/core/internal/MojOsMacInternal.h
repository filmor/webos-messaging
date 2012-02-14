/* ============================================================
 * Date  : Jan 5, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJOSMACINTERNAL_H_
#define MOJOSMACINTERNAL_H_

#include <libkern/OSAtomic.h>

inline MojInt32 MojAtomicAdd(MojAtomicT* a, MojInt32 incr)
{
	MojAssert(a);
	return OSAtomicAdd32(incr, &a->val);
}

inline MojInt32 MojAtomicIncrement(MojAtomicT* a)
{
	MojAssert(a);
	return OSAtomicIncrement32(&a->val);
}

inline MojInt32 MojAtomicDecrement(MojAtomicT* a)
{
	MojAssert(a);
	return OSAtomicDecrement32(&a->val);
}

#endif /* MOJOSMACINTERNAL_H_ */
