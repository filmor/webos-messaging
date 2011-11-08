/* ============================================================
 * Date  : Feb 10, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJOBJECTBUILDER_H_
#define MOJOBJECTBUDILER_H_

#include "core/MojCoreDefs.h"
#include "core/MojObject.h"

class MojObjectBuilder : public MojObjectVisitor
{
public:
	MojObjectBuilder();

	virtual MojErr reset();
	virtual MojErr beginObject();
	virtual MojErr endObject();
	virtual MojErr beginArray();
	virtual MojErr endArray();
	virtual MojErr propName(const MojChar* name, MojSize len);
	virtual MojErr nullValue();
	virtual MojErr boolValue(bool val);
	virtual MojErr intValue(MojInt64 val);
	virtual MojErr decimalValue(const MojDecimal& val);
	virtual MojErr stringValue(const MojChar* val, MojSize len);

	const MojObject& object() const { return m_obj; }
	MojObject& object() { return m_obj; }

private:
	struct Rec
	{
		Rec(MojObject::Type type, MojString& propName) : m_obj(type), m_propName(propName) {}
		MojObject m_obj;
		MojString m_propName;
	};
	typedef MojVector<Rec> ObjStack;

	MojErr push(MojObject::Type type);
	MojErr pop();
	MojErr value(const MojObject& val);
	MojObject& back() { return const_cast<MojObject&>(m_stack.back().m_obj); }

	ObjStack m_stack;
	MojObject m_obj;
	MojString m_propName;
};

#endif /* MOJOBJECTBUILDER_H_ */
