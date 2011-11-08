/* ============================================================
 * Date  : Jul 16, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJREACTOR_H_
#define MOJREACTOR_H_

#include "core/MojCoreDefs.h"
#include "core/MojSignal.h"

class MojReactor : private MojNoCopy
{
public:
	typedef MojSignal<MojSockT> SockSignal;

	virtual ~MojReactor() {}
	virtual MojErr init() = 0;
	virtual MojErr run() = 0;
	virtual MojErr stop() = 0;
	virtual MojErr dispatch() = 0;
	virtual MojErr addSock(MojSockT sock) = 0;
	virtual MojErr removeSock(MojSockT sock) = 0;
	virtual MojErr notifyReadable(MojSockT sock, SockSignal::SlotRef slot) = 0;
	virtual MojErr notifyWriteable(MojSockT sock, SockSignal::SlotRef slot) = 0;
};

#endif /* MOJREACTOR_H_ */
