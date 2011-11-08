/* ============================================================
 * Date  : May 24, 2010
 * Copyright 2010 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJMESSAGE_H_
#define MOJMESSAGE_H_

#include "core/MojCoreDefs.h"
#include "core/MojSignal.h"

class MojMessage : public MojSignalHandler
{
public:
	virtual ~MojMessage() {}
	virtual MojErr dispatch() = 0;
	virtual const MojChar* queue() const = 0;

protected:
	friend class MojMessageDispatcher;

	MojListEntry m_queueEntry;
};

#endif /* MOJMESSAGE_H_ */
