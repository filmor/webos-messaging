/* ============================================================
 * Date  : Sep 9, 2010
 * Copyright 2010 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBPUTHANDLER_H_
#define MOJDBPUTHANDLER_H_

#include "db/MojDbDefs.h"
#include "core/MojComp.h"

class MojDbPutHandler : private MojNoCopy
{
public:
	MojDbPutHandler(const MojChar* kindId, const MojChar* confProp);
	virtual ~MojDbPutHandler();
	virtual MojErr open(const MojObject& conf, MojDb* db, MojDbReq& req);
	virtual MojErr close();
	virtual MojErr put(MojObject& obj, MojDbReq& req, bool putObj = true) = 0;

protected:
	struct LengthComp
	{
		// sort longest strings first so we can do longest match for wildcards
		int operator()(const MojChar* val1, const MojChar* val2) const
		{
			MojSize len1 = MojStrLen(val1);
			MojSize len2 = MojStrLen(val2);
			if (len1 == len2)
				return MojStrCmp(val1, val2);
			return MojComp<MojSize>()(len2, len1);
		}
	};

	virtual MojErr configure(const MojObject& conf, MojDbReq& req);
	static MojErr validateWildcard(const MojString& val, MojErr errToThrow);
	static bool matchWildcard(const MojString& expr, const MojChar* val, MojSize len);

	const MojChar* m_kindId;
	const MojChar* m_confProp;
	MojDb* m_db;

	static MojLogger s_log;
};

#endif /* MOJDBPUTHANDLER_H_ */
