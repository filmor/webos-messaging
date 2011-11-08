/* ============================================================
 * Date  : Jul 12, 2010
 * Copyright 2010 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBQUERYFILTER_H_
#define MOJDBQUERYFILTER_H_

#include "db/MojDbDefs.h"
#include "db/MojDbQuery.h"

class MojDbQueryFilter
{
public:
	MojDbQueryFilter();
	~MojDbQueryFilter();

	MojErr init(const MojDbQuery& query);
	bool test(const MojObject& obj) const;

private:
	static bool testLower(const MojDbQuery::WhereClause& clause, const MojObject& val);
	static bool testUpper(const MojDbQuery::WhereClause& clause, const MojObject& val);

	MojDbQuery::WhereMap m_clauses;
};

#endif /* MOJDBQUERYFILTER_H_ */
