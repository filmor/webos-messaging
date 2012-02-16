/* ============================================================
 * Date  : Mar 19, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDATAWRITER_H_
#define MOJDATAWRITER_H_

#include "core/MojCoreDefs.h"
#include "core/MojBuffer.h"

class MojDataWriter
{
public:
	MojDataWriter(MojBuffer& buf) : m_buf(buf) {}

	void clear() { m_buf.clear(); }
	MojErr writeUInt8(MojByte val) { return m_buf.writeByte(val); }
	MojErr writeUInt16(MojUInt16 val);
	MojErr writeUInt32(MojUInt32 val);
	MojErr writeInt64(MojInt64 val);
	MojErr writeDecimal(const MojDecimal& val);
	MojErr writeChars(const MojChar* chars, MojSize len);

	static MojSize sizeChars(const MojChar* chars, MojSize len);
	static MojSize sizeDecimal(const MojDecimal& val);

	MojBuffer& buf() { return m_buf; }

private:
	MojBuffer& m_buf;
};

class MojDataReader
{
public:
	MojDataReader();
	MojDataReader(const MojByte* data, MojSize size);

	void data(const MojByte* data, MojSize size);
	MojErr readUInt8(MojByte& val);
	MojErr readUInt16(MojUInt16& val);
	MojErr readUInt32(MojUInt32& val);
	MojErr readInt64(MojInt64& val);
	MojErr readDecimal(MojDecimal& val);
	MojErr skip(MojSize len);

	const MojByte* begin() const { return m_begin; }
	const MojByte* end() const { return m_end; }
	const MojByte* pos() const { return m_pos; }

	MojSize available() const { return m_end - m_pos; }

private:

	const MojByte* m_begin;
	const MojByte* m_end;
	const MojByte* m_pos;
};

#endif /* MOJDATAWRITER_H_ */
