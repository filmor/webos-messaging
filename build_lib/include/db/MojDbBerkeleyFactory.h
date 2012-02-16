/* ============================================================
 * Date  : Dec 10, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBBERKELEYFACTORY_H_
#define MOJDBBERKELEYFACTORY_H_

#include "db/MojDbDefs.h"
#include "db/MojDbStorageEngine.h"

class MojDbBerkeleyFactory : public MojDbStorageEngineFactory
{
public:
	static const MojChar* const Name;

	virtual MojErr create(MojRefCountedPtr<MojDbStorageEngine>& engineOut);
};

#endif /* MOJDBBERKELEYFACTORY_H_ */
