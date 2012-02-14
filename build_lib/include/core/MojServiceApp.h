/* ============================================================
 * Date  : Apr 15, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJSERVICEAPP_H_
#define MOJSERVICEAPP_H_

#include "core/MojCoreDefs.h"
#include "core/MojApp.h"

class MojServiceApp : public MojApp
{
public:
	virtual ~MojServiceApp();

protected:
	MojServiceApp(MojUInt32 majorVersion = MajorVersion, MojUInt32 minorVersion = MinorVersion, const MojChar* versionString = NULL);
	virtual MojErr open();
	virtual void shutdown();
	MojErr initPmLog();

	bool m_shutdown;

private:
	static void shutdownHandler(int sig);
	static MojServiceApp* s_instance;

	MojString m_name;
};

#endif /* MOJSERVICEAPP_H_ */
