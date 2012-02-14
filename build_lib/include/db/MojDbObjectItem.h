/* ============================================================
 * Date  : Apr 20, 2010
 * Copyright 2010 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBOBJECTITEM_H_
#define MOJDBOBJECTITEM_H_

#include "db/MojDbDefs.h"
#include "db/MojDbStorageEngine.h"

class MojDbObjectItem : public MojDbStorageItem
{
public:
	MojDbObjectItem(const MojObject& obj);

	virtual MojErr close();
	virtual MojErr kindId(MojString& kindIdOut, MojDbKindEngine& kindEngine);
	virtual MojErr visit(MojObjectVisitor& visitor, MojDbKindEngine& kindEngine, bool headerExpected = true) const;
	virtual const MojObject& id() const;
	virtual MojSize size() const;

	const MojObject& obj() const { return m_obj; }
	const MojDbKeyBuilder::KeySet sortKeys() const { return m_sortKeys; }
	void sortKeys(const MojDbKeyBuilder::KeySet& keys) { m_sortKeys = keys; }

private:
	MojObject m_obj;
	MojDbKeyBuilder::KeySet m_sortKeys;
};

#endif /* MOJDBOBJECTITEM_H_ */
