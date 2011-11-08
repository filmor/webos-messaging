/* ============================================================
 * Date  : Jul 21, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBSERVICEHANDLER_H_
#define MOJDBSERVICEHANDLER_H_

#include "db/MojDbServiceHandlerBase.h"

class MojDbServiceHandler : public MojDbServiceHandlerBase
{
public:
	static const MojUInt32 MaxQueryLimit = MojDbQuery::MaxQueryLimit;
	static const MojUInt32 MaxReserveIdCount = MaxQueryLimit * 2;

	MojDbServiceHandler(MojDb& db, MojReactor& reactor);

	virtual MojErr open();
	virtual MojErr close();

private:
	static const MojChar* const BatchSchema;
	static const MojChar* const CompactSchema;
	static const MojChar* const DelSchema;
	static const MojChar* const DelKindSchema;
	static const MojChar* const DumpSchema;
	static const MojChar* const FindSchema;
	static const MojChar* const GetSchema;
	static const MojChar* const LoadSchema;
	static const MojChar* const MergeSchema;
	static const MojChar* const PurgeSchema;
	static const MojChar* const PurgeStatusSchema;
	static const MojChar* const PutSchema;
	static const MojChar* const PutKindSchema;
	static const MojChar* const PutPermissionsSchema;
	static const MojChar* const PutQuotasSchema;
	static const MojChar* const QuotaStatsSchema;
	static const MojChar* const ReserveIdsSchema;
	static const MojChar* const SearchSchema;
	static const MojChar* const StatsSchema;
	static const MojChar* const WatchSchema;

	typedef MojMap<MojString, DbCallback, const MojChar*, MojComp<const MojChar*>, MojCompAddr<DbCallback> > BatchMap;

	class Watcher : public MojSignalHandler
	{
	public:
		Watcher(MojServiceMessage* msg);

		MojErr handleWatch();
		MojErr handleCancel(MojServiceMessage* msg);

		MojRefCountedPtr<MojServiceMessage> m_msg;
		MojDb::WatchSignal::Slot<Watcher> m_watchSlot;
		MojServiceMessage::CancelSignal::Slot<Watcher> m_cancelSlot;
	};

	MojErr handleBatch(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleCompact(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleDel(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleDelKind(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleDump(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleFind(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleGet(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleLoad(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleMerge(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handlePurge(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handlePurgeStatus(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handlePut(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handlePutKind(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handlePutPermissions(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handlePutQuotas(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleQuotaStats(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleReserveIds(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleSearch(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleStats(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleWatch(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);

	MojErr findImpl(MojServiceMessage* msg, MojObject& payload, MojDbReq& req, MojDbCursor& cursor, bool doCount);

	BatchMap m_batchCallbacks;

	static const SchemaMethod s_pubMethods[];
	static const SchemaMethod s_privMethods[];
	static const Method s_batchMethods[];
};

#endif /* MOJDBSERVICEHANDLER_H_ */
