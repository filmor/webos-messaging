/* ============================================================
 * Date  : Jan 5, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJREACTORAPP_H_
#define MOJREACTORAPP_H_

#include "core/MojCoreDefs.h"
#include "core/MojServiceApp.h"

template<class REACTOR>
class MojReactorApp : public MojServiceApp
{
public:
	virtual MojErr init();
	virtual MojErr run();
	virtual void shutdown();

protected:
	typedef REACTOR Reactor;

	MojReactorApp(MojUInt32 majorVersion = MajorVersion, MojUInt32 minorVersion = MinorVersion, const MojChar* versionString = NULL)
	: MojServiceApp(majorVersion, minorVersion, versionString) {}

	Reactor m_reactor;
};

#include "core/internal/MojReactorAppInternal.h"

#endif /* MOJREACTORAPP_H_ */
