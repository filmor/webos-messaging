/* ============================================================
 * Date  : Dec 7, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBISAMQUERY_H_
#define MOJDBISAMQUERY_H_

#include "db/MojDbDefs.h"
#include "db/MojDbStorageEngine.h"

class MojDbIsamQuery : public MojDbStorageQuery
{
public:
	typedef MojVector<MojString> StringVec;

	virtual ~MojDbIsamQuery();
	virtual MojErr close();
	virtual MojErr get(MojDbStorageItem*& itemOut, bool& foundOut);
	virtual MojErr getId(MojObject& idOut, MojUInt32& groupOut, bool& foundOut);
	virtual MojErr count(MojUInt32& countOut);
	virtual MojErr nextPage(MojDbQuery::Page& pageOut);
	virtual MojUInt32 groupCount() const;

protected:
	enum State {
		StateInvalid,
		StateSeek,
		StateNext
	};

	typedef MojVector<MojByte> ByteVec;
	typedef MojVector<MojDbKeyRange> RangeVec;

	virtual MojErr seekImpl(const ByteVec& key, bool desc, bool& foundOut) = 0;
	virtual MojErr next(bool& foundOut) = 0;
	virtual MojErr getVal(MojDbStorageItem*& itemOut, bool& foundOut) = 0;

	MojDbIsamQuery();
	MojErr open(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn);
	MojErr getImpl(MojDbStorageItem*& itemOut, bool& foundOut, bool getItem);
	void init();
	bool match();
	bool limitEnforced() { return m_count >= m_plan->limit(); }
	int compareKey(const ByteVec& key);
	MojErr incrementCount();
	MojErr seek(bool& foundOut);
	MojErr getKey(MojUInt32& groupOut, bool& foundOut);
	MojErr saveEndKey();
	MojErr parseId(MojObject& idOut);
	MojErr checkExclude(MojDbStorageItem* item, bool& excludeOut);

	bool m_isOpen;
	MojUInt32 m_count;
	State m_state;
	RangeVec::ConstIterator m_iter;
	MojDbStorageTxn* m_txn;
	MojSize m_keySize;
	const MojByte* m_keyData;
	MojAutoPtr<MojDbQueryPlan> m_plan;
};

#endif /* MOJDBISAMQUERY_H_ */
