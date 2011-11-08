/* ============================================================
 * Date  : Apr 7, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBBERKELEYQUERY_H_
#define MOJDBBERKELEYQUERY_H_

#include "db/MojDbDefs.h"
#include "db/MojDbBerkeleyEngine.h"
#include "db/MojDbIsamQuery.h"

class MojDbBerkeleyQuery : public MojDbIsamQuery
{
public:
	MojDbBerkeleyQuery();
	~MojDbBerkeleyQuery();

	MojErr open(MojDbBerkeleyDatabase* db, MojDbBerkeleyDatabase* joinDb,
			MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn);
	MojErr getById(const MojObject& id, MojDbStorageItem*& itemOut, bool& foundOut);

	virtual MojErr close();

private:
	static const MojUInt32 OpenFlags;
	static const MojUInt32 SeekFlags;
	static const MojUInt32 SeekIdFlags;
	static const MojUInt32 SeekEmptyFlags[2];
	static const MojUInt32 NextFlags[2];

	virtual MojErr seekImpl(const ByteVec& key, bool desc, bool& foundOut);
	virtual MojErr next(bool& foundOut);
	virtual MojErr getVal(MojDbStorageItem*& itemOut, bool& foundOut);
	MojErr getKey(bool& foundOut, MojUInt32 flags);

	MojDbBerkeleyCursor m_cursor;
	MojDbBerkeleyItem m_key;
	MojDbBerkeleyItem m_val;
	MojDbBerkeleyItem m_primaryVal;
	MojDbBerkeleyDatabase* m_db;
};

#endif /* MOJDBBERKELEYQUERY_H_ */
