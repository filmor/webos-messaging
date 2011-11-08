/* ============================================================
 * Date  : Apr 02, 2010
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBREVISIONSET_H_
#define MOJDBREVISIONSET_H_

#include "db/MojDbDefs.h"
#include "db/MojDbExtractor.h"

class MojDbRevisionSet : public MojRefCounted
{
public:
	static const MojChar* const PropsKey;  // props
	static const MojChar* const NameKey;  // name

	MojDbRevisionSet();

	const MojString& name() { return m_name; }

	MojErr fromObject(const MojObject& obj);
	MojErr addProp(const MojObject& propObj);
	MojErr update(MojObject* newObj, const MojObject* oldObj) const;

private:
	typedef MojSet<MojDbKey> KeySet;
	typedef MojVector<MojRefCountedPtr<MojDbPropExtractor> > PropVec;

	MojErr diff(MojObject& newObj, const MojObject& oldObj) const;
	MojErr updateRev(MojObject& obj) const;

	MojString m_name;
	PropVec m_props;
	static MojLogger s_log;
};

#endif /* MOJDBREVISIONSET_H_ */
