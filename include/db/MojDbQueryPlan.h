/* ============================================================
 * Date  : May 4, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBQUERYPLAN_H_
#define MOJDBQUERYPLAN_H_

#include "db/MojDbDefs.h"
#include "db/MojDbQuery.h"
#include "db/MojDbKey.h"

class MojDbQueryPlan : private MojNoCopy
{
public:
	typedef MojVector<MojDbKeyRange> RangeVec;
	typedef MojVector<MojString> StringVec;

	MojDbQueryPlan(MojDbKindEngine& kindEngine);
	~MojDbQueryPlan();

	MojErr init(const MojDbQuery& query, const MojDbIndex& index);
	void limit(MojUInt32 val) { m_query.limit(val); }
	void desc(bool val) { m_query.desc(val); }

	const RangeVec& ranges() const { return m_ranges; }
	const MojDbKey& pageKey() const { return m_query.page().key(); }
	MojUInt32 groupCount() const { return m_groupCount; }
	MojUInt32 limit() const { return m_query.limit(); }
	MojSize idIndex() const { return m_idPropIndex; }
	bool desc() const { return m_query.desc(); }
	const MojDbQuery& query() const { return m_query; }
	MojDbKindEngine& kindEngine() const { return m_kindEngine; }

private:
	typedef MojSet<MojDbKey> KeySet;
	typedef MojVector<MojByte> ByteVec;

	MojErr buildRanges(const MojDbIndex& index);
	MojErr rangesFromKeySets(const KeySet& lowerKeys, const KeySet& upperKeys, const KeySet& prefixKeys,
			const MojDbQuery::WhereClause* clause);
	MojErr rangesFromKeys(MojDbKey lowerKey, MojDbKey upperKey, MojDbKey prefix, MojUInt32 index,
			const MojDbQuery::WhereClause* clause);
	MojErr addRange(const MojDbKey& lowerKey, const MojDbKey& upperKey, MojUInt32 group = 0);
	MojErr pushSearch(MojDbKeyBuilder& lowerBuilder, MojDbKeyBuilder& upperBuilder, const MojObject& val, MojDbTextCollator* collator);
	static MojErr pushVal(MojDbKeyBuilder& builder, const MojObject& val, MojDbTextCollator* collator);

	RangeVec m_ranges;
	MojDbQuery m_query;
	MojString m_locale;
	MojSize m_idPropIndex;
	MojUInt32 m_groupCount;
	MojDbKindEngine& m_kindEngine;
};

#endif /* MOJDBQUERYPLAN_H_	 */
