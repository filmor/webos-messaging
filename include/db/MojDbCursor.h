/* ============================================================
 * Date  : Jan 5, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBCURSOR_H_
#define MOJDBCURSOR_H_

#include "db/MojDbDefs.h"
#include "db/MojDbStorageEngine.h"
#include "db/MojDbQueryFilter.h"
#include "core/MojObjectFilter.h"
#include "core/MojNoCopy.h"

class MojDbCursor : private MojNoCopy
{
public:
	MojDbCursor();
	virtual ~MojDbCursor();
	virtual MojErr close();
	virtual MojErr get(MojDbStorageItem*& itemOut, bool& foundOut);
	virtual MojErr get(MojObject& objOut, bool& foundOut);
	virtual MojErr visit(MojObjectVisitor& visitor);
	virtual MojErr count(MojUInt32& countOut);
	virtual MojErr nextPage(MojDbQuery::Page& pageOut);

	bool isOpen() const { return m_storageQuery.get() != NULL; }
	const MojDbQuery& query() const { return m_query; }
	MojDbStorageTxn* txn() { return m_txn.get(); }

protected:
	friend class MojDb;
	friend class MojDbIndex;
	friend class MojDbKindEngine;
	friend class MojDbKind;

	virtual MojErr init(const MojDbQuery& query);
	MojErr initImpl(const MojDbQuery& query);
	MojErr visitObject(MojObjectVisitor& visitor, bool& foundOut);
	void txn(MojDbStorageTxn* txn, bool ownTxn);
	void kindEngine(MojDbKindEngine* kindEngine) { m_kindEngine = kindEngine; }
	void excludeKinds(const MojSet<MojString>& toExclude);

	bool m_ownTxn;
	MojErr m_lastErr;
	MojDbKindEngine* m_kindEngine;
	MojDbQuery m_query;
	MojRefCountedPtr<MojDbStorageTxn> m_txn;
	MojRefCountedPtr<MojDbStorageQuery> m_storageQuery;
	MojAutoPtr<MojObjectFilter> m_objectFilter;
	MojAutoPtr<MojDbQueryFilter> m_queryFilter;
	MojRefCountedPtr<MojDbWatcher> m_watcher;
};

#endif /* MOJDBCURSOR_H_ */
