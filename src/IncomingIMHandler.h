/*
 * IncomingIMHandler.h
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
 * IncomingIMHandler class handles incoming IM messages
 */

#ifndef INCOMINGIMHANDLER_H_
#define INCOMINGIMHANDLER_H_

#include "core/MojService.h"
#include "db/MojDbServiceClient.h"
#include "IMMessage.h"
#include "LibpurpleAdapter.h"

class IncomingIMHandler : public MojSignalHandler
{
public:

	static const int errCannotProcessInput = 100;
	static const int errGenericDBError = 101;

	IncomingIMHandler(MojService* service);
	virtual ~IncomingIMHandler();
	MojErr saveNewIMMessage(MojRefCountedPtr<IMMessage>);


private:

	MojDbClient::Signal::Slot<IncomingIMHandler> m_IMSaveMessageSlot;
	MojErr IMSaveMessageResult(MojObject& result, MojErr err);

	// IncomingIMMessage Object that is in process
	MojRefCountedPtr<IMMessage> m_IMMessage;

	// Pointer to the service used for creating/sending requests.
	MojService*	m_service;

	// Database client used to make any requests (put, watch, find, etc.) to Mojo DB
	MojDbServiceClient m_dbClient;
};

#endif /* INCOMINGIMHANDLER_H_ */
