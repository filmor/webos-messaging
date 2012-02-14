/* ============================================================
 * Date  : Apr 29, 2010
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBSERVICEHANDLERBASE_H_
#define MOJDBSERVICEHANDLERBASE_H_

#include "db/MojDbDefs.h"
#include "db/MojDb.h"
#include "core/MojService.h"
#include "core/MojServiceMessage.h"

class MojDbServiceHandlerBase : public MojService::CategoryHandler
{
public:
	static const MojUInt32 DeadlockSleepMillis = 20;
	static const MojUInt32 MaxDeadlockRetries = 20;

	typedef MojErr (CategoryHandler::* DbCallback)(MojServiceMessage* msg, const MojObject& payload, MojDbReq& req);

	MojDbServiceHandlerBase(MojDb& db, MojReactor& reactor);

	virtual MojErr open() = 0;
	virtual MojErr close() = 0;

protected:

	virtual MojErr invoke(Callback method, MojServiceMessage* msg, MojObject& payload);
	MojErr invokeImpl(Callback method, MojServiceMessage* msg, MojObject& payload);

	static MojErr formatCount(MojServiceMessage* msg, MojUInt32 count);
	static MojErr formatPut(MojServiceMessage* msg, const MojObject* begin, const MojObject* end);
	static MojErr formatPutAppend(MojObjectVisitor& writer, const MojObject& result);

	MojDb& m_db;
	MojReactor& m_reactor;
	static MojLogger s_log;
};

#endif /* MOJDBSERVICEHANDLERBASE_H_ */
