/* ============================================================
 * Date  : Jan 13, 2010
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBOBJECTHEADER_H_
#define MOJDBOBJECTHEADER_H_

#include "core/MojNoCopy.h"
#include "core/MojObject.h"
#include "core/MojObjectSerialization.h"

class MojDbObjectHeader : private MojNoCopy
{
public:
	MojDbObjectHeader();
	MojDbObjectHeader(const MojObject& id);

	MojErr addTo(MojObject& obj) const;
	MojErr extractFrom(MojObject& obj);

	MojErr read(MojDbKindEngine& kindEngine);
	MojErr write(MojBuffer& buf, MojDbKindEngine& kindEngine);
	MojErr visit(MojObjectVisitor& visitor, MojDbKindEngine& kindEngine);
	void reset();

	void id(const MojObject& id) { m_id = id; }
	const MojObject& id() const { return m_id; }
	const MojString& kindId() const { return m_kindId; }
	MojObjectReader& reader() { return m_reader; }

private:
	static const MojUInt8 Version = 1;

	MojErr readHeader();
	MojErr readRev();
	MojErr readSize();
	
	MojObjectReader m_reader;
	MojObject m_id;
	MojString m_kindId;
	MojInt64 m_rev;
	bool m_del;
	bool m_read;
};

#endif /* MOJDBOBJECTHEADER_H_ */
