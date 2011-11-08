/* ============================================================
 * Date  : Sep 1, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJLISTENTRY_H_
#define MOJLISTENTRY_H_

#include "core/MojNoCopy.h"

class MojListEntry : private MojNoCopy
{
public:
	MojListEntry() { reset(); }
	~MojListEntry();

	bool inList() const { return m_next != NULL; }
	void insert(MojListEntry* elem);
	void erase();
	void fixup();
	void swap(MojListEntry& entry);
	void reset() { m_prev = NULL; m_next = NULL; }

	MojListEntry* m_prev;
	MojListEntry* m_next;
};

#endif /* MOJLISTENTRY_H_ */
