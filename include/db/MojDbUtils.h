/* ============================================================
 * Date  : May 19, 2010
 * Copyright 2010 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBUTILS_H_
#define MOJDBUTILS_H_

#include "db/MojDbDefs.h"

class MojDbUtils
{
public:
	static const MojChar* collationToString(MojDbCollationStrength coll);
	static MojErr collationFromString(const MojString& str, MojDbCollationStrength& collOut);
};

#endif /* MOJDBUTILS_H_ */
