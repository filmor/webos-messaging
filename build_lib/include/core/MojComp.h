/* ============================================================
 * Date  : Jan 12, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJCOMP_H_
#define MOJCOMP_H_

#include "core/MojCoreDefs.h"

template<class T>
struct MojEq
{
	bool operator()(const T& val1, const T& val2) const
	{
		return val1 == val2;
	}
};

template<class T>
struct MojLessThan
{
	bool operator()(const T& val1, const T& val2) const
	{
		return val1 < val2;
	}
};

template<class T>
struct MojLessThanEq
{
	bool operator()(const T& val1, const T& val2) const
	{
		return val1 <= val2;
	}
};

template<class T>
struct MojGreaterThan
{
	bool operator()(const T& val1, const T& val2) const
	{
		return val1 > val2;
	}
};

template<class T>
struct MojGreaterThanEq
{
	bool operator()(const T& val1, const T& val2) const
	{
		return val1 >= val2;
	}
};

template<class T>
struct MojComp
{
	int operator()(const T& val1, const T& val2) const
	{
		if (val1 < val2)
			return -1;
		if (val2 < val1)
			return 1;
		return 0;
	}
};

template<class T>
struct MojCompAddr
{
	int operator()(const T& val1, const T& val2) const
	{
		if (&val1 < &val2)
			return -1;
		if (&val2 < &val1)
			return 1;
		return 0;
	}
};

// SPECIALIZATIONS
template<>
struct MojEq<const MojChar*>
{
	bool operator()(const MojChar* str1, const MojChar* str2) const
	{
		return MojStrCmp(str1, str2) == 0;
	}
};

template<>
struct MojEq<MojChar*> : public MojEq<const MojChar*> {};
template<>
struct MojEq<MojString> : public MojEq<const MojChar*> {};

template<>
struct MojComp<const MojChar*>
{
	int operator()(const MojChar* str1, const MojChar* str2) const
	{
		return MojStrCmp(str1, str2);
	}
};

template<>
struct MojComp<MojChar*> : public MojComp<const MojChar*> {};

template<>
struct MojComp<int>
{
	int operator()(int val1, int val2) const
	{
		return val1 - val2;
	}
};

#endif /* MOJCOMP_H_ */
