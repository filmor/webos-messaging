/* ============================================================
 * Date  : Jan 12, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJHASHER_H_
#define MOJHASHER_H_

#include "core/MojCoreDefs.h"
#include "core/MojUtil.h"

template<class T>
struct MojHasher
{
};

template<>
struct MojHasher<const MojChar*>
{
	MojSize operator()(const MojChar* str)
	{
		return MojHash(str);
	}
};

template<>
struct MojHasher<MojChar*> : public MojHasher<const MojChar*> {};

template<class T>
struct MojIntHasher
{
	MojSize operator()(T i)
	{
		return MojHash(&i, sizeof(i));
	}
};

template<>
struct MojHasher<MojInt8> : public MojIntHasher<MojInt8> {};
template<>
struct MojHasher<MojUInt8> : public MojIntHasher<MojUInt8> {};
template<>
struct MojHasher<MojInt16> : public MojIntHasher<MojInt16> {};
template<>
struct MojHasher<MojUInt16> : public MojIntHasher<MojUInt16> {};
template<>
struct MojHasher<MojInt32> : public MojIntHasher<MojInt32> {};
template<>
struct MojHasher<MojUInt32> : public MojIntHasher<MojUInt32> {};
template<>
struct MojHasher<MojInt64> : public MojIntHasher<MojInt64> {};
template<>
struct MojHasher<MojUInt64> : public MojIntHasher<MojUInt64> {};
template<>
struct MojHasher<class T*> : public MojIntHasher<T*> {};
template<>
struct MojHasher<long unsigned int> : public MojIntHasher<long unsigned int> {};

#endif /* MOJHASHER_H_ */
