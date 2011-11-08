/* ============================================================
 * Date  : May 17, 2010
 * Copyright 2010 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBTEXTCOLLATOR_H_
#define MOJDBTEXTCOLLATOR_H_

#include "db/MojDbDefs.h"
#include "db/MojDbTextUtils.h"
#include "core/MojHashMap.h"
#include "core/MojRefCount.h"
#include "core/MojThread.h"

struct UCollator;

class MojDbTextCollator : public MojRefCounted
{
public:
	MojDbTextCollator();
	virtual ~MojDbTextCollator();

	MojErr init(const MojChar* locale, MojDbCollationStrength strength);
	MojErr sortKey(const MojString& str, MojDbKey& keyOut) const;
	MojErr sortKey(const UChar* chars, MojSize size, MojDbKey& keyOut) const;

private:
	UCollator* m_ucol;
};

#endif /* MOJDBTEXTCOLLATOR_H_ */
