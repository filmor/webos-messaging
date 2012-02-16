/* ============================================================
 * Date  : Dec 10, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBINNOFACTORY_H_
#define MOJDBINNOFACTORY_H_

#include "db/MojDbDefs.h"
#include "db/MojDbStorageEngine.h"

class MojDbInnoFactory : public MojDbStorageEngineFactory
{
public:
	static const MojChar* const Name;

	virtual MojErr create(MojAutoPtr<MojDbStorageEngine>& engineOut);
};

#endif /* MOJDBINNOFACTORY_H_ */
