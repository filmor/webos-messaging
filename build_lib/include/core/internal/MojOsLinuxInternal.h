/* ============================================================
 * Date  : Jan 5, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJOSLINUXINTERNAL_H_
#define MOJOSLINUXINTERNAL_H_

#if defined(MOJ_X86)
inline MojInt32 MojAtomicAdd(MojAtomicT* a, MojInt32 incr)
{
	MojAssert(a);
	MojInt32 i = incr;
	asm volatile(
			"lock xaddl %0, %1"
				: "+r" (i), "+m" (a->val)
				: : "memory");
	return incr + i;
}
#elif defined(MOJ_ARM)
inline MojInt32 MojAtomicAdd(MojAtomicT* a, MojInt32 incr)
{
	MojAssert(a);
	MojUInt32 tmp;
	MojInt32 res;

	asm volatile(
			"1:	ldrex %0, [%2]\n"
			"add %0, %0, %3\n"
			"strex %1, %0, [%2]\n"
			"teq %1, #0\n"
			"bne 1b"
				: "=&r" (res), "=&r" (tmp)
				: "r" (&a->val), "Ir" (incr)
				: "cc");
	return res;
}
#endif

inline MojInt32 MojAtomicIncrement(MojAtomicT* a)
{
	return MojAtomicAdd(a, 1);
}

inline MojInt32 MojAtomicDecrement(MojAtomicT* a)
{
	return MojAtomicAdd(a, -1);
}

#endif /* MOJOSLINUXINTERNAL_H_ */
