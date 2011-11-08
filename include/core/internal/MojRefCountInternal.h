/* ============================================================
 * Date  : Jan 9, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJREFCOUNTINTERNAL_H_
#define MOJREFCOUNTINTERNAL_H_

#define MojRefCountPtrToAtomic(P) (((MojAtomicT*) P) - 1)

template<class T>
T* MojRefCountNew(MojSize numElems)
{
	T* t = (T*) MojRefCountAlloc(numElems * sizeof(T));
	if (t)
		MojConstruct(t, numElems);
	return t;
}

inline MojInt32 MojRefCountGet(void* p)
{
	MojAssert(p);
	return MojAtomicGet(MojRefCountPtrToAtomic(p));
}

inline void MojRefCountRetain(void* p)
{
	MojAssert(p);
	MojAtomicIncrement(MojRefCountPtrToAtomic(p));
}

template<class T>
void MojRefCountRelease(T* p, MojSize numElems)
{
	if (p) {
		MojAtomicT* a = MojRefCountPtrToAtomic(p);
		if (MojAtomicDecrement(a) == 0) {
			MojAtomicDestroy(a);
			MojDestroy(p, numElems);
			MojFree(a);
		}
	}
}

#endif /* MOJREFCOUNTINTERNAL_H_ */
