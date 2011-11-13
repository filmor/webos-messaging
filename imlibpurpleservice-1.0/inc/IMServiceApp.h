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
#include "IMServiceHandler.h"

class IMServiceApp : public MojReactorApp<MojGmainReactor>
{
public:
	static const char* const ServiceName;
	static MojLogger s_log;

	IMServiceApp();
	virtual MojErr open();
	virtual MojErr close();

private:
	typedef MojReactorApp<MojGmainReactor> Base;

	MojRefCountedPtr<IMServiceHandler> m_handler;
	MojLunaService  m_service;
};

#endif /* IMSERVICEAPP_H_ */
