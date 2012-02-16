/*
 * IMServiceApp.cpp
 *
 * Copyright 2010 Palm, Inc. All rights reserved.
 *
 * This program is free software and licensed under the terms of the GNU
 * General Public License Version 2 as published by the Free
 * Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-
 * 1301, USA
 *
 * IMLibpurpleservice uses libpurple.so to implement a fully functional IM
 * Transport service for use on a mobile device.
 *
 * IMServiceApp class handles the application main event loop
 */

#include "IMServiceApp.h"
#include "luna/MojLunaService.h"
#include "LibpurpleAdapter.h"

const char* const IMServiceApp::ServiceName = _T("org.webosinternals.imlibpurple");
MojLogger IMServiceApp::s_log(_T("org.webosinternals.imlibpurple.serviceApp"));

int main(int argc, char** argv)
{
purple_debug_set_enabled(TRUE);
	IMServiceApp app;
	LibpurpleAdapter::init();
	int mainResult = app.main(argc, argv);
	return mainResult;
}

IMServiceApp::IMServiceApp()
{
}

MojErr IMServiceApp::open()
{
	MojLogNotice(s_log, _T("%s starting..."), name().data());

	MojErr err = Base::open();
	MojErrCheck(err);

	// open service
	err = m_service.open(ServiceName);
	MojErrCheck(err);
	err = m_service.attach(m_reactor.impl());
	MojErrCheck(err);

	// create and attach service handler
	m_handler.reset(new IMServiceHandler(&m_service));
	MojAllocCheck(m_handler.get());

	err = m_handler->init();
	MojErrCheck(err);

	err = m_service.addCategory(MojLunaService::DefaultCategory, m_handler.get());
	MojErrCheck(err);

	MojLogNotice(s_log, _T("%s started."), name().data());

	return MojErrNone;
}

MojErr IMServiceApp::close()
{
	MojLogNotice(s_log, _T("%s stopping..."), name().data());

	MojErr err = MojErrNone;
	MojErr errClose = m_service.close();
	MojErrAccumulate(err, errClose);
	errClose = Base::close();
	MojErrAccumulate(err, errClose);

	MojLogNotice(s_log, _T("%s stopped."), name().data());

	return err;
}
