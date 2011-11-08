/* ============================================================
 * Date  : Jan 12, 2010
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJTOKENSET_H_
#define MOJTOKENSET_H_

#include "core/MojCoreDefs.h"
#include "core/MojHashMap.h"
#include "core/MojSharedTokenSet.h"

class MojTokenSet : public MojNoCopy
{
public:
	static const MojUInt8 InvalidToken = 0;
	typedef MojSharedTokenSet::TokenVec TokenVec;

	MojTokenSet();

	MojErr init(MojSharedTokenSet* sharedSet);
	MojErr tokenFromString(const MojChar* str, MojUInt8& tokenOut, bool add);
	MojErr stringFromToken(MojUInt8 token, MojString& propNameOut) const;

private:
	MojRefCountedPtr<MojSharedTokenSet> m_sharedTokenSet;

	TokenVec m_tokenVec;
	MojObject m_obj;
};

#endif /* MOJTOKENSET_H_ */
