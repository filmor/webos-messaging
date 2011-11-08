/* ============================================================
 * Date  : Dec 7, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBINNOQUERY_H_
#define MOJDBINNOQUERY_H_

#include "db/MojDbDefs.h"
#include "db/MojDbInnoEngine.h"
#include "db/MojDbIsamQuery.h"

class MojDbInnoQuery : public MojDbIsamQuery
{
public:
	MojDbInnoQuery();
	~MojDbInnoQuery();

	MojErr open(MojDbInnoDatabase* db, MojDbInnoDatabase* joinDb,
				MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn);

	virtual MojErr close();

private:
	virtual MojErr seekImpl(const ByteVec& key, bool desc, bool& foundOut);
	virtual MojErr next(bool& foundOut);
	virtual MojErr getVal(MojDbStorageItem*& itemOut);
	MojErr loadKey(bool& foundOut);

	MojDbInnoItem m_item;
	MojDbInnoItem m_primaryItem;
	MojDbInnoDatabase* m_db;
};

#endif /* MOJDBINNOQUERY_H_ */
