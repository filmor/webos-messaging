/* ============================================================
 * Date  : May 18, 2010
 * Copyright 2010 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBTEXTUTILS_H_
#define MOJDBTEXTUTILS_H_

#include "db/MojDbDefs.h"
#include "core/MojVector.h"
#include "unicode/utypes.h"

class MojDbTextUtils
{
public:
	typedef MojVector<UChar> UnicodeVec;

	static MojErr strToUnicode(const MojString& src, UnicodeVec& destOut);
	static MojErr unicodeToStr(const UChar* src, MojSize len, MojString& destOut);
};

#define MojUnicodeErrCheck(STATUS) if (U_FAILURE(STATUS)) MojErrThrowMsg(MojErrDbUnicode, _T("icu: %s"), u_errorName(STATUS))

#endif /* MOJDBTEXTUTILS_H_ */
