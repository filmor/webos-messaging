/* ============================================================
 * Date  : May 11, 2010
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJFILE_H_
#define MOJFILE_H_

#include "core/MojCoreDefs.h"

class MojFile {
public:
	static const MojSize MojFileBufSize = 2048;

	MojFile(MojFileT file = MojInvalidFile) : m_file(file) {}
	~MojFile() { (void) close(); }

	MojErr close();
	MojErr lock(int op) { return MojFileLock(m_file, op); }
	bool open() { return m_file != MojInvalidFile; }
	MojErr open(const MojChar* path, int flags, MojModeT mode = 0) { MojAssert(m_file == MojInvalidFile); return MojFileOpen(m_file, path, flags, mode); }
	MojErr readString(MojString& strOut);
	MojErr read(void* buf, MojSize bufSize, MojSize& sizeOut) { return MojFileRead(m_file, buf, bufSize, sizeOut); }
	MojErr writeString(const MojChar* data, MojSize& sizeOut);
	MojErr write(const void* data, MojSize size, MojSize& sizeOut) { return MojFileWrite(m_file, data, size, sizeOut); }

private:
	MojFileT m_file;
};

#endif /* MOJFILE_H_ */
