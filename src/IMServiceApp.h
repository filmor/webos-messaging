/*
 * IMServiceApp.h
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

#ifndef IMSERVICEAPP_H_
#define IMSERVICEAPP_H_

#include "core/MojGmainReactor.h"
#include "core/MojReactorApp.h"
#include "luna/MojLunaService.h"

class IMServiceApp : public MojReactorApp<MojGmainReactor>
{
public:
	static const char* const ServiceName;
	static MojLogger s_log;
	static IMServiceApp* s_instance;

	static void Shutdown();

	// listener interface to keep track of active processes so we can shut down
	class Listener
	{
	public:
		virtual void ProcessStarting() = 0;
		virtual void ProcessDone() = 0;
	};

	IMServiceApp();
	virtual MojErr open();
	virtual MojErr close();


private:
	typedef MojReactorApp<MojGmainReactor> Base;

	MojLunaService  m_service;
};

#endif /* IMSERVICEAPP_H_ */
