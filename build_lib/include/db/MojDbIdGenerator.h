/* ============================================================
 * Date  : Jul 8, 2010
 * Copyright 2010 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBIDGENERATOR_H_
#define MOJDBIDGENERATOR_H_

#include "db/MojDbDefs.h"
#include "core/MojThread.h"

class MojDbIdGenerator
{
public:
	MojDbIdGenerator();

	MojErr init();
	MojErr id(MojObject& idOut);

private:
	char m_randStateBuf[8];
	MojRandomDataT m_randBuf;
	MojThreadMutex m_mutex;
};

#endif /* MOJDBIDGENERATOR_H_ */
