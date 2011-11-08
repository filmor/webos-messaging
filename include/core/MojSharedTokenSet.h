/* ============================================================
 * Date  : Jan 11, 2010
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJSHAREDTOKENSET_H_
#define MOJSHAREDTOKENSET_H_

#include "core/MojHashMap.h"
#include "core/MojObject.h"
#include "core/MojThread.h"
#include "core/MojVector.h"
#include "db/MojDbStorageEngine.h"

class MojSharedTokenSet : public MojRefCounted
{
public:
	typedef MojVector<MojString> TokenVec;

	virtual MojErr addToken(const MojChar* str, MojUInt8& tokenOut, TokenVec& vecOut, MojObject& tokenObjOut) = 0;
	virtual MojErr tokenSet(TokenVec& vecOut, MojObject& tokenObjOut) const = 0;
};

#endif /* MOJSHAREDTOKENSET_H_ */
