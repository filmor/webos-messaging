/* ============================================================
 * Date  : Jan 5, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

template<class REACTOR>
MojErr MojReactorApp<REACTOR>::init()
{
	MojLogTrace(s_log);

	MojErr err = MojServiceApp::init();
	MojErrCheck(err);
	err = m_reactor.init();
	MojErrCheck(err);

	return MojErrNone;
}

template<class REACTOR>
MojErr MojReactorApp<REACTOR>::run()
{
	MojLogTrace(s_log);

	MojErr err = m_reactor.run();
	MojErrCheck(err);

	return MojErrNone;
}

template<class REACTOR>
void MojReactorApp<REACTOR>::shutdown()
{
	MojLogTrace(s_log);

	MojErr err = m_reactor.stop();
	MojErrCatchAll(err);
	MojServiceApp::shutdown();
}
